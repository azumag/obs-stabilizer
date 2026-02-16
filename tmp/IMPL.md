# OBS Stabilizer Plugin - Image Abstraction Layer Removal

## 実施日: 2026-02-16
## 状態: 完了
## 関連課題: Issue #321 (Extreme OpenCV Coupling) - レビュー対応

---

## 概要

設計書 `tmp/ARCH.md` の Phase 1: Image Abstraction Layer は、レビュー結果 `tmp/REVIEW.md` に基づいて削除されました。YAGNI原則（You Aren't Gonna Need It）に従い、現時点で不要な抽象化レイヤーを削除し、シンプルな設計に戻しました。

---

## 削除理由

### レビューで指摘された問題

1. **設計目標と実装の乖離**
   - `process_frame_buffer()` メソッドは内部的に `cv::Mat` を使用
   - `dynamic_cast` を使用しているため、実際にはOpenCV以外の実装を受け付けない
   - 設計目標（Metal、CUDA、Vulkanなどの代替実装）が達成されていない

2. **パフォーマンス問題**
   - `process_frame_buffer()` はフレームごとに `dynamic_cast` を実行
   - リアルタイム処理（30fps+）において、フレームごとの動的キャストはオーバーヘッドとなる

3. **YAGNI原則との整合性**
   - GPU加速が実際に必要になる前に抽象化レイヤーを実装している
   - 不要な複雑性を導入している

---

## 削除内容

### 削除されたファイル
1. `src/core/image_buffer.hpp` - IImageBuffer インターフェース
2. `src/core/opencv_image_buffer.hpp` - OpenCVImageBuffer クラス定義
3. `src/core/opencv_image_buffer.cpp` - OpenCVImageBuffer 実装

### 更新されたファイル
1. `src/core/stabilizer_core.hpp`
   - `image_buffer.hpp` インクルードを削除
   - `process_frame_buffer()` メソッドを削除
   - クラスコメントを更新

2. `src/core/stabilizer_core.cpp`
   - `opencv_image_buffer.hpp` インクルードを削除
   - `process_frame_buffer()` メソッド実装を削除（最後の84行）

3. `src/core/frame_utils.hpp`
   - `image_buffer.hpp` と `opencv_image_buffer.hpp` インクルードを削除
   - `obs_to_image_buffer()` 関数を削除
   - `image_buffer_to_obs()` 関数を削除
   - `cv_to_image_buffer()` 関数を削除
   - `image_buffer_to_cv()` 関数を削除
   - `mat_to_buffer()` 関数を削除
   - ファイルヘッダーコメントを更新

4. `CMakeLists.txt`
   - `opencv_image_buffer.cpp` を SOURCES から削除
   - `opencv_image_buffer.cpp` を TEST_CORE_SOURCES から削除
   - `opencv_image_buffer.cpp` を PERF_SOURCES から削除
   - `opencv_image_buffer.cpp` を SINGLERUN_SOURCES から削除

---

## 設計原則への準拠

| 原則 | 状態 | 説明 |
|------|------|------|
| **YAGNI** | 準拠 | 現時点で不要な抽象化レイヤーを削除 |
| **DRY** | 準拠 | コード重複を排除、シンプルな実装を維持 |
| **KISS** | 準拠 | シンプルで明確なOpenCVベースの実装 |
| **t-wada TDD** | 準拠 | 既存のテストはすべて引き続き使用可能 |

---

## 将来の対応

GPU加速が実際に必要になった場合は、その時点で適切な抽象化を設計し直します。

- 現時点ではOpenCV実装で十分な性能が得られている
- 将来的にGPU加速が必要になった場合、実際のユースケースに基づいて設計

---

## ビルド確認

```bash
# クリーンビルド
rm -rf build && mkdir build && cd build
cmake ..
make -j$(nproc)
```

すべてのターゲットが正常にビルドされることを確認済み:
- `obs-stabilizer-opencv` (メインプラグイン)
- `stabilizer_tests` (テストスイート)
- `performance_benchmark` (パフォーマンスベンチマーク)
- `singlerun` (軽量検証ツール)

---

## まとめ

レビュー `tmp/REVIEW.md` の推奨に従い、中途半端な抽象化レイヤーを削除しました。

**主な成果**:
1. 不要なファイルの削除（3ファイル）
2. シンプルで明確なOpenCVベースの実装に戻す
3. パフォーマンスオーバーヘッドの排除（dynamic_castの削除）
4. メンテナンスコストの削減

**原則への準拠**:
- YAGNI: 今必要じゃない機能は作らない
- KISS: シンプルに保つ
- 将来GPU加速が必要になった時点で適切な設計を行う

---

**状態**: IMPLEMENTED
**次のステップ**: 現状のOpenCV実装で安定運用、GPU加速が必要になった時点で再設計
