# OBS Stabilizer Architecture Document

## 機能要件

### 1.1 コア機能
- **リアルタイム映像スタビライゼーション**: OBS Studio の動画ソースにリアルタイムで映像ブレ補正を適用
- **マルチソース対応**: 複数の動画ソースに同時にフィルターを適用可能
- **パラメータ調整機能**: ユーザーが補正強度、スムージング範囲などをリアルタイムで調整可能
- **プリセット保存**: 用途に応じた設定プリセットの保存・読み込み

### 1.2 拡張機能
- **アダプティブスタビライゼーション**: 動きの激しさに応じて補正強度を自動調整
- **モーション分類**: 手振れ、パン、ズーム等の動きを分類して適切に処理
- **パフォーマンスモニタリング**: 処理時間、CPU使用率等のリアルタイム監視

## 非機能要件

### 2.1 パフォーマンス
- **処理遅延**: フレーム処理時間 < 33ms（30fps対応）
- **CPU使用率**: フィルター適用時のCPU使用率増加 < 5%
- **メモリ使用量**: ビルドインメモリ管理、メモリリークなし
- **リアルタイム性**: 最低30fpsでの処理対応

### 2.2 セキュリティ
- **外部ライブラリの脆弱性対応**: OpenCV等の依存ライブラリの脆弱性パッチ適用
- **入力検証**: 無効な入力値に対する頑健性
- **バッファオーバーフロー防止**: 適切な境界チェック

### 2.3 互換性
- **クロスプラットフォーム**: Windows, macOS, Linux対応
- **OBSバージョン互換**: 最新版OBS Studioおよびメジャーバージョン対応
- **アーキテクチャ対応**: x86_64, arm64 (Apple Silicon)対応

### 2.4 保守性
- **モジュラ化**: 機能追加・バグ修正が容易なモジュール構造
- **テストカバレッジ**: 単体テストカバレッジ > 80%
- **ドキュメント**: インラインコメント、アーキテクチャドキュメント整備

## 受け入れ基準

### 3.1 機能面
- [ ] 映像ブレが視覚的に低減できること
- [ ] 設定画面から補正レベルを調整でき、リアルタイムに反映されること
- [ ] 複数の動画ソースにフィルターを適用してもOBSがクラッシュしないこと
- [ ] 設定プリセットの保存・読み込みが正しく動作すること

### 3.2 パフォーマンス面
- [ ] HD解像度で処理遅延 < 33msであること
- [ ] フィルター適用時のCPU使用率増加 < 5%であること
- [ ] 長時間連続稼動でメモリリークが発生しないこと

### 3.3 テスト面
- [ ] 全テストケースがパスすること
- [ ] 単体テストカバレッジ > 80%であること
- [ ] 統合テストで実際のOBS環境での動作が確認できること

### 3.4 プラットフォーム面
- [ ] 最新版OBS on Windowsで基本動作確認できること
- [ ] 最新版OBS on macOSで基本動作確認できること
- [ ] 最新版OBS on Linuxで基本動作確認できること

## 設計方針

### 4.1 基本方針
- **YAGNI (You Aren't Gonna Need It)**: 今必要な機能のみを実装
- **DRY (Don't Repeat Yourself)**: 重複コードを排除、共通化
- **KISS (Keep It Simple, Stupid)**: シンプルな実装を優先
- **TDD (Test-Driven Development)**: テスト駆動開発

### 4.2 技術選定
- **言語**: C++17 (Modern C++)
- **映像処理**: OpenCV 4.5+ (外部ライブラリ利用で開発効率化)
- **ビルドシステム**: CMake
- **GUI**: OBS標準UIフレームワーク (Qt依存を最小化)
- **プラットフォーム**: Windows, macOS, Linux

### 4.3 コーディング規約
- **絵文字不使用**: コメント・ドキュメントは英語のみ
- **詳細コメント**: 実装の意図・根拠を記述
- **RAIIパターン**: リソース管理でRAIIを活用
- **一時ファイル一元化**: tmp/ディレクトリに集約

## アーキテクチャ決定

### 5.1 全体構成

```
obs-stabilizer/
├── src/
│   ├── core/                    # コア処理レイヤー（OBS非依存）
│   │   ├── stabilizer_core.hpp/cpp       # スタビライゼーションコア
│   │   ├── stabilizer_wrapper.hpp/cpp    # RAIIラッパー
│   │   ├── adaptive_stabilizer.hpp/cpp  # アダプティブ処理
│   │   ├── motion_classifier.hpp/cpp     # モーション分類
│   │   ├── feature_detection.hpp/cpp    # 特徴点検出
│   │   ├── frame_utils.hpp/cpp         # フレーム操作ユーティリティ
│   │   ├── parameter_validation.hpp/cpp # パラメータ検証
│   │   ├── logging.hpp                 # ロギング
│   │   ├── stabilizer_constants.hpp     # 定数定義
│   │   ├── platform_optimization.hpp    # プラットフォーム最適化
│   │   └── benchmark.hpp/cpp          # ベンチマーク
│   └── stabilizer_opencv.cpp     # OBS統合レイヤー
├── tests/                     # テスト
│   ├── test_basic.cpp
│   ├── test_stabilizer_core.cpp
│   ├── test_adaptive_stabilizer.cpp
│   ├── test_motion_classifier.cpp
│   ├── test_feature_detection.cpp
│   ├── test_edge_cases.cpp
│   ├── test_integration.cpp
│   └── test_memory_leaks.cpp
├── docs/                      # ドキュメント
│   └── ARCHITECTURE.md
├── tmp/                       # 一時ファイル（一元化）
├── CMakeLists.txt
└── README.md
```

### 5.2 レイヤー設計

#### 5.2.1 OBS統合レイヤー (stabilizer_opencv.cpp)
- **責務**: OBS APIとのやり取り、プラグインエントリーポイント
- **依存**: OBS API, coreレイヤー
- **機能**:
  - `obs_source_info` の実装
  - プラグイン登録
  - OBS設定データとパラメータのマッピング

#### 5.2.2 コア処理レイヤー (src/core/)
- **責務**: 映像スタビライゼーション処理（OBS非依存）
- **依存**: OpenCVのみ
- **機能**:
  - `StabilizerCore`: 基本スタビライゼーション処理
  - `StabilizerWrapper`: RAIIラッパー、リソース管理
  - `AdaptiveStabilization`: アダプティブ処理
  - `MotionClassifier`: モーション分類
  - `FeatureDetection`: 特徴点検出

### 5.3 主要コンポーネント

#### 5.3.1 StabilizerCore
```cpp
class StabilizerCore {
public:
    struct StabilizerParams {
        double smoothing_radius;
        double correction_strength;
        double crop_border;
        int feature_count;
        double feature_quality;
        // ...他のパラメータ
    };

    void initialize(const StabilizerParams& params);
    cv::Mat processFrame(const cv::Mat& input_frame);
    void reset();

private:
    StabilizerParams params_;
    std::vector<cv::Point2f> prev_features_;
    cv::Mat prev_frame_;
    // ...内部状態
};
```

#### 5.3.2 AdaptiveStabilization
```cpp
class AdaptiveStabilization {
public:
    struct AdaptiveConfig {
        double motion_sensitivity;
        double transition_rate;
        double min_correction;
        double max_correction;
    };

    double adaptCorrection(double base_correction, double motion_intensity);

private:
    AdaptiveConfig config_;
    // ...内部状態
};
```

#### 5.3.3 StabilizerWrapper
```cpp
class StabilizerWrapper {
public:
    StabilizerWrapper();
    ~StabilizerWrapper();  // RAII: リソース自動解放
    void initialize(const StabilizerCore::StabilizerParams& params);
    cv::Mat process(const cv::Mat& frame);

private:
    std::unique_ptr<StabilizerCore> core_;
    bool initialized_;
    // ...内部状態
};
```

### 5.4 データフロー

```
OBS Source Frame
    ↓
[OBS Integration Layer]
    (stabilizer_opencv.cpp)
    ↓ obs_source_frame -> cv::Mat
[Core Processing Layer]
    ├─ [Feature Detection] → 特徴点抽出
    ├─ [Motion Calculation] → 動きベクトル計算
    ├─ [Motion Classification] → 動き分類
    ├─ [Adaptive Correction] → アダプティブ補正
    └─ [Frame Transformation] → フレーム変換
    ↓ cv::Mat
[Frame Utils]
    ↓ cv::Mat -> obs_source_frame
OBS Output Frame
```

## トレードオフの検討

### 6.1 性能 vs 精度
- **課題**: 高精度の振れ検出は計算量増加
- **選択**: パラメータ調整可能にし、ユーザーが状況に応じて選択
- **妥協点**: デフォルト設定はリアルタイム性能優先（1-4ms/frame on HD）

### 6.2 ライブラリ利用 vs 自作
- **課題**: OpenCV等ライブラリ使用は開発効率向上だが、柔軟性低下
- **選択**: 開発速度優先でOpenCV採用
- **妥協点**: モジュラ化で将来の置き換えを容易に

### 6.3 クロップ vs パディング
- **課題**: 映像ブレ補正で黒帯発生（パディング）vs 画角損失（クロップ）
- **選択**: ユーザー選択可能に
- **妥協点**: デフォルトは軽微なクロップ推奨

### 6.4 複雑性 vs 機能性
- **課題**: アダプティブ処理等の高度機能は複雑化
- **選択**: モジュラ化で複雑性を局所化
- **妥協点**: 基本機能はシンプルに、高度機能は別モジュール

## 実装計画

### Phase 4: 最適化・リリース準備 (Week 9-10)
- [ ] パフォーマンス調整 (#7)
  - SIMD最適化
  - マルチスレッド化
- [ ] クロスプラットフォーム対応 (#8)
  - Windows検証
  - macOS (arm64)検証
  - Linux検証
- [ ] デバッグ・診断機能実装 (#16)
  - パフォーマンスモニタリングUI
  - ログレベル調整
- [ ] ドキュメント整備 (#9)
  - ユーザーマニュアル
  - 開発者ドキュメント

### Phase 5: 本格運用準備 (Week 11-12)
- [ ] CI/CD パイプライン構築 (#18)
  - 自動リリース
  - バイナリ配布
- [ ] プラグイン配布・インストール機能 (#19)
  - OBSプラグインインストーラー
  - アップデート通知
- [ ] セキュリティ・脆弱性対応 (#21)
  - 依存ライブラリスキャン
  - 脆弱性パッチ
- [ ] コミュニティ・コントリビューション体制構築 (#20)
  - コントリビューションガイド
  - Issue/PRテンプレート

## まとめ

本アーキテクチャは以下の原則に基づいて設計されている：

1. **YAGNI**: 今必要な機能のみ実装、将来拡張はモジュール追加で対応
2. **DRY**: 共通処理はユーティリティクラスとして抽出
3. **KISS**: シンプルな実装、明確な責務分離
4. **モジュラ化**: レイヤー設計で疎結合、高凝集
5. **テスト駆動開発**: 単体テストカバレッジ > 80%目標

これにより、保守性、拡張性、パフォーマンスのバランスが取れた設計となる。
