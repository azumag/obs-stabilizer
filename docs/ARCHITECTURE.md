# OBS Stabilizer Architecture

**Snapshot date**: 2026-08-12  
**Baseline**: `main` at `5eb1763c7b96d9f4b77387b8967b5f7a326b8d39`  
**Status**: Current runtime architecture and extension seams

この文書は、現在の `main` で実際にビルド・実行される経路と、すでに存在するが実行経路には未接続の拡張用コンポーネントを分けて記述する。

---

## 1. System overview

OBS Stabilizer は OBS Studio の video filter として動作し、OBS の `obs_source_frame` を OpenCV の `cv::Mat` に変換してから、特徴点追跡・変換推定・平滑化・画像変形を行い、結果を OBS frame として返す。

```text
OBS Studio
   |
   | filter lifecycle / properties / video frame callbacks
   v
src/stabilizer_opencv.cpp
   |-- settings <-> StabilizerParams
   |-- parameter validation
   |-- OBS <-> OpenCV conversion adapter
   |
   v
StabilizerWrapper
   |-- RAII ownership of StabilizerCore
   |-- mutex boundary between OBS UI/video access
   |
   v
StabilizerCore
   |-- grayscale conversion
   |-- feature detection
   |-- pyramidal Lucas-Kanade optical flow
   |-- affine transform estimation
   |-- transform-history smoothing
   |-- correction + edge handling
   |
   v
FrameUtils / FrameBuffer
   |
   v
OBS output frame
```

### Current runtime boundary

`CMakeLists.txt` の plugin target に直接入る主要 source は次のとおり。

- `src/stabilizer_opencv.cpp`
- `src/core/stabilizer_core.cpp`
- `src/core/stabilizer_wrapper.cpp`
- `src/core/preset_manager.cpp`
- `src/core/frame_utils.cpp`（real OBS headers が利用可能なビルド）

`src/core` や `src/ui` にはこのほかにも policy/helper が存在するが、ファイルが存在することと runtime pipeline に接続済みであることは同義ではない。

---

## 2. Runtime modules

### 2.1 Plugin interface — `src/stabilizer_opencv.cpp`

OBS との境界を担当する。

主な責務:

- `obs_source_info` の登録
- filter の create / destroy / update
- properties panel の構築
- preset callback と settings の読み書き
- OBS frame の受け取りと結果 frame の返却
- processing time の簡易集計
- OBS API 境界での例外ログとフォールバック

UI property の一部には `src/ui` の再利用可能な schema/policy も存在するが、現時点では properties 構築ロジックの多くが `stabilizer_opencv.cpp` に残っている。

### 2.2 Thread-safety boundary — `StabilizerWrapper`

`StabilizerWrapper` は `StabilizerCore` を `std::unique_ptr` で所有する RAII wrapper であり、公開操作を mutex で直列化する。

OBS の UI thread から設定変更が入る一方で video thread が frame を処理する可能性があるため、同期責任は wrapper に置く。これにより `StabilizerCore` 自体は lock を持たず、アルゴリズム実装に集中できる。

```text
OBS UI thread ----- update/initialize ----+
                                         |
                                         v
                                  StabilizerWrapper
                                         ^
                                         |
OBS video thread -- process_frame -------+

                  single mutex boundary
                         |
                         v
                  StabilizerCore
```

### 2.3 Core engine — `StabilizerCore`

`StabilizerCore` は single-threaded な stabilization engine で、内部同期は行わない。現在の public API は OpenCV 型を直接使用する。

主要処理:

1. 入力 frame の妥当性確認
2. BGRA/BGR/GRAY から grayscale への変換
3. `goodFeaturesToTrack()` による特徴点検出
4. `calcOpticalFlowPyrLK()` による追跡
5. tracked points から affine motion を推定
6. transform history の平滑化
7. correction transform を適用
8. Padding / Crop / Scale の edge handling
9. performance metrics と tracking state の更新

保持する temporal state:

- previous grayscale frame
- previous feature points
- recent transform history
- tracking failure count
- performance metrics

scene cut、強い motion blur、特徴点不足などでは、tracking state を再構築しつつ入力 frame をそのまま返すことがある。

### 2.4 Frame conversion — `FrameUtils`

`FRAME_UTILS` は OBS と OpenCV の間の変換・検証・buffer ownership を集約する。

主な責務:

- OBS frame validation
- `obs_source_frame` -> `cv::Mat`
- packed frame 用 borrowed view API
- color conversion
- `cv::Mat` -> OBS frame
- metadata copy
- output buffer lifecycle

現在の `main` の plugin adapter は centralized `FrameUtils` を下層で利用している。変換方針を plugin file に複製せず、format 判定や buffer lifetime の責任を `FrameUtils` 側へ寄せるのが設計方針である。

### 2.5 Parameter validation — `parameter_validation.hpp`

OBS settings から得た値をそのまま core に渡さず、範囲を検証・clamp する。geometry validation と algorithm parameter validation を分離し、不正値が OpenCV 呼び出しまで到達することを防ぐ。

### 2.6 Constants — `stabilizer_constants.hpp`

feature count、smoothing radius、correction limit、image size などの named constants を集約する。magic number を処理本体から切り離す。

### 2.7 Presets — `PresetManager` and preset policies

built-in preset は `StabilizerCore` の factory と settings callback から利用される。custom preset の永続化用 `PresetManager` と UI-side の preset selection policy も存在し、preset の保存・読み込みと選択規則を独立してテストできるようにしている。

---

## 3. Frame data flow

### 3.1 Per-frame processing

```text
obs_source_frame
   |
   | validate pointer / geometry / format
   v
FrameUtils conversion
   |
   v
cv::Mat
   |
   v
StabilizerWrapper::process_frame()
   |
   | mutex acquired
   v
StabilizerCore::process_frame()
   |
   |-- validate frame
   |-- grayscale conversion
   |-- feature detection / tracking
   |-- estimate affine transform
   |-- smooth transform history
   |-- apply correction
   |-- edge handling
   v
stabilized cv::Mat
   |
   v
FrameUtils / FrameBuffer
   |
   | copy output pixels + reference metadata
   v
obs_source_frame returned to OBS
```

### 3.2 Ownership rules

- OBS が渡した input frame の lifetime は OBS callback の外へ延長しない。
- borrowed `cv::Mat` view を使用する場合、その storage を core state や別 thread に保持してはいけない。
- temporal grayscale data など callback 後も必要な state は独立した storage を所有する。
- output OBS frame は `FrameBuffer` 側で lifecycle を管理する。

この ownership contract を破る最適化は、コピー回数を減らせても採用しない。

---

## 4. Configuration flow

```text
OBS properties panel
   |
   v
obs_data_t settings
   |
   v
settings_to_params()
   |
   v
VALIDATION::validate_parameters()
   |
   v
StabilizerCore::StabilizerParams
   |
   v
StabilizerWrapper::initialize/update_parameters()
```

設定変更と frame 処理が重なる可能性があるため、core state を変更する操作は wrapper の mutex boundary を通す。

---

## 5. Adaptive stabilization status and integration design

初期設計では独立した `AdaptiveStabilizer` と `MotionClassifier` を runtime pipeline に置く構成が想定されていた。しかし現在の `main` にはそれらの runtime module は存在せず、実際の plugin target は `StabilizerCore` を直接利用する。

したがって、現在の architecture を「adaptive pipeline 実装済み」と表現するのは正確ではない。

一方で、将来の adaptive stabilization を実装するための extension seam はすでに複数存在する。

```text
                       +----------------------+
                       | Motion observations  |
                       | from StabilizerCore  |
                       +----------+-----------+
                                  |
                                  v
+------------------+    +----------------------+    +------------------+
| ResolutionPolicy |--->| Adaptive policy      |<---| Performance data |
+------------------+    | (future integration) |    +------------------+
                                  |
                 +----------------+----------------+
                 |                                 |
                 v                                 v
        KalmanTransformFilter             GPU backend policy
                 |                                 |
                 +----------------+----------------+
                                  |
                                  v
                         Stabilization params
```

現在確認できる extension-oriented components:

- `kalman_transform_filter.*`: transform smoothing に利用可能な reusable Kalman filter
- `resolution_profile.hpp`: resolution-aware parameter profile
- `gpu_backend_policy.hpp`: CPU/OpenCL/CUDA/Metal の deterministic selection policy
- `performance_metrics.hpp`: processing time と feature success の snapshot/model
- `rolling_average.hpp`: lightweight rolling statistic helper
- `frame_analyzer.hpp`: stateless frame validation/content analysis
- `image_view.hpp`: image view abstraction の基礎

重要: これらは個別に存在・テストされていても、すべてが現在の OBS runtime path に接続されているわけではない。

### Adaptive integration contract

将来 adaptive layer を追加する場合は、次の境界を維持する。

1. core から observation を取得するが、OBS API 型を adaptive layer に持ち込まない。
2. policy は `StabilizerParams` など明示的な値で core に指示する。
3. UI/video concurrency は引き続き `StabilizerWrapper` で同期する。
4. unsupported GPU backend は必ず CPU に deterministic fallback する。
5. policy 切替で temporal state の意味が変わる場合は reset/reinitialize の条件を明示する。
6. adaptive optimization は frame ownership contract を変更しない。

---

## 6. Auxiliary and policy components

### `FrameAnalyzer`

frame validation と content bounds の stateless helper。core から責務を分離する方向の再利用可能な部品だが、現在の runtime core のすべての validation がここへ移行済みという意味ではない。

### `ImageView`

OpenCV coupling を減らすための view abstraction の基礎。`StabilizerCore` の public API は現在も `cv::Mat` を使用しているため、完全な image abstraction layer への移行は未完了。

### `GpuBackendPolicy`

requested backend と runtime capabilities から backend を選ぶ policy。GPU image-processing implementation 自体ではない。現状の production stabilization path は CPU/OpenCV が基準である。

### `PerformanceMetrics`

processing time、estimated FPS、feature success rate、status/recommendation を表現する model。model が存在することと、OBS properties panel へリアルタイム表示済みであることは別である。

### UI schema/policies

`src/ui/stabilizer_property_schema.hpp` と `preset_selection.hpp` は UI 設定の一部を OBS API から切り離してテストするための seam。properties builder 全体の抽出は今後の refactor 対象である。

---

## 7. Design decisions

### 7.1 Synchronization lives above the core

`StabilizerCore` に mutex を入れず、`StabilizerWrapper` が concurrency boundary を担当する。

理由:

- per-frame algorithm を単純に保つ
- UI/video concurrency の責任箇所を一つにする
- unit test で core を OBS threading から独立させる

### 7.2 OpenCV remains the processing backend

current core は `cv::Mat` と OpenCV primitives を直接利用する。abstraction helper は存在するが、backend-independent core への全面移行はまだ行わない。

### 7.3 Conversion and ownership are centralized

OBS/OpenCV conversion、format handling、buffer lifetime は `FrameUtils` に集約する。plugin callback 側で独自変換を増やさない。

### 7.4 CPU is the guaranteed fallback

GPU backend selection policy が存在しても、GPU acceleration availability を runtime の必須条件にしない。unsupported backend では CPU に戻れることを契約とする。

### 7.5 ABI compatibility is explicit

macOS の distributable plugin は OBS 30.0 の official source headers を基準にビルドし、newer OBS でもロードできる ABI boundary を維持する。macOS bundle は `@loader_path` 基準で bundled dependencies を解決する。

### 7.6 Performance claims require benchmark coverage

frame conversion、resolution profile、sanitizer、integration などは dedicated workflow で回帰を検出する。ドキュメントに固定の FPS/CPU 数値を「保証値」として書かず、CI benchmark と実機計測を source of truth とする。

---

## 8. Build and test architecture

### Build

- CMake 3.16+
- C++17
- OpenCV components: core, imgproc, video, calib3d, features2d, flann
- OBS Studio integration
- Windows / macOS / Linux build paths
- macOS `.plugin` bundle packaging and bundled OpenCV verification

### CI

主要 workflow:

- Build OBS Stabilizer
- Quality Assurance
- Performance Tests
- Feature Implementation Flow
- Integration Tests
- Memory Sanitizer
- Thread Sanitizer
- macOS Plugin Package

変更対象に応じて frame conversion、frame lifecycle、Kalman、GPU policy、resolution profile などの dedicated workflow も実行する。

テスト数や coverage は継続的に変化するため、この文書では固定値を architecture contract としない。

---

## 9. Failure handling

### OBS boundary

OBS callback では C++ exception を OBS 側へ漏らさない。exception はログへ記録し、可能な場合は元 frame を返して OBS session を継続する。

### Core

core は invalid input や processing failure を `last_error_` と戻り値で表現する。tracking failure のような回復可能状態と、invalid geometry のような initialization failure を区別する。

### Frame conversion

invalid pointer、unsupported format、invalid stride/geometry を処理前に拒否する。output allocation に失敗した場合は元 frame へ fallback できるようにする。

---

## 10. Troubleshooting

### Plugin does not load on macOS

確認順:

1. `obs-stabilizer.plugin/Contents/Info.plist` が存在し妥当か
2. `Contents/MacOS/obs-stabilizer` が生成されているか
3. bundled OpenCV libraries が `Contents/Frameworks` に存在するか
4. dependency path が `@loader_path` 基準になっているか
5. `codesign --verify --deep --strict` が通るか
6. CI の macOS Plugin Package / Build artifact が成功しているか

### Filter loads but stabilization is not visible

- `enabled` が true か
- input frame geometry と format が妥当か
- feature_count / quality_level / min_distance が極端な値でないか
- texture の少ない scene で feature tracking が失敗していないか
- OBS log の `[obs-stabilizer]` diagnostics を確認する

### Performance is poor

- resolution を確認する
- feature_count を下げる
- smoothing radius を必要以上に大きくしない
- Performance Tests / frame conversion benchmark の結果を比較する
- GPU policy の存在だけを理由に GPU acceleration が有効だと判断しない

### CI failure after refactor

- general Build / QA だけでなく変更した subsystem の dedicated workflow を確認する
- sanitizer failure は再現性のある ownership/threading defect として優先して扱う
- conversion optimization では pixel integrity と metadata preservation の両方を確認する

---

## 11. Known limitations

- properties construction の多くはまだ `stabilizer_opencv.cpp` にあり、UI integration と OBS lifecycle が完全分離されていない。
- `StabilizerCore` public API は OpenCV 型に依存している。
- standalone GPU backend policy は存在するが、GPU accelerated stabilization pipeline は production runtime に未接続。
- standalone Kalman filter は存在するが、default runtime smoothing path を置き換えてはいない。
- dedicated `AdaptiveStabilizer` / `MotionClassifier` runtime layer は current `main` には存在しない。
- performance は input content、resolution、platform、OpenCV build に依存するため、固定の CPU/FPS 値を保証しない。

---

## 12. Source-of-truth files

architecture を変更した場合は少なくとも次のファイルとの整合性を確認する。

- `CMakeLists.txt` — runtime build graph
- `src/stabilizer_opencv.cpp` — OBS integration and UI callbacks
- `src/core/stabilizer_wrapper.*` — concurrency boundary
- `src/core/stabilizer_core.*` — algorithm and temporal state
- `src/core/frame_utils.*` — conversion and frame ownership
- `src/core/parameter_validation.hpp` — parameter contract
- `src/core/stabilizer_constants.hpp` — algorithm constants
- `src/core/preset_manager.*` — preset persistence
- `src/core/frame_analyzer.hpp` — frame-analysis helper seam
- `src/core/gpu_backend_policy.hpp` — backend selection policy
- `src/core/kalman_transform_filter.*` — optional transform smoother
- `src/core/performance_metrics.hpp` — performance model
- `src/ui/*` — UI policy/schema seams
- `.github/workflows/*` — executable CI contracts

関連ドキュメント:

- `docs/CURRENT_ARCHITECTURE.md`
- `docs/ARCHITECTURE_DECISIONS.md`
- `docs/STABILIZATION_ALGORITHM.md`
- `docs/ERROR_HANDLING.md`
- `docs/GPU_ACCELERATION_EVALUATION.md`
- `docs/DEVELOPER_GUIDE.md`
- `docs/testing/`

---

## 13. Architecture maintenance rule

新しい helper/policy を追加しただけでは「runtime integration 完了」と記述しない。`CMakeLists.txt`、runtime call graph、tests/workflows の3点を確認し、実際に production path から到達可能になった時点で本書の runtime section を更新する。
