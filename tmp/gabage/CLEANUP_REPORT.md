# 掃除屋タスク報告

## 実施日時
2026年2月10日

## 隔離対象ファイル一覧

### 1. tmp/reports から隔離
- `tmp/SUMMARY.md`
- `tmp/IMPL.md`
- `tmp/ARCH.md`
- `tmp/QA_REPORT.md`
- `tmp/STATE.md`
- `tmp/security-audits/`

### 2. 未使用コードから隔離
- `src/core/performance_regression.hpp`
- `src/core/performance_regression.cpp`

### 3. 設定ファイルから隔離
- `opencode.json` (opencode.ai の設定ファイル)
- `flow-implement.sh` (一時的な実装フロースクリプト)

### 4. ビルド生成物から隔離
- `build/` (ビルド生成物)

### 5. 未使用ドキュメントから隔離
- `docs/IMPLEMENTED.md`
- `docs/motion-classifier-design.md`
- `docs/motion-classifier-threshold-results.md`
- `docs/motion-classifier-threshold-tuning.md`
- `docs/QA.md`
- `ARCH.md` (バグ修正設計書)
- `AUDIT_REPORT.md` (コード監査レポート)

### 6. GitHub ワークフローから隔離
- `.github/workflows/flow-implement.yml`

## CMakeLists.txt 修正
- `PERF_SOURCES` から `performance_regression.cpp` を削除

## 統計
- 隔離ファイル数: 314 ファイル
- 隔離サイズ: 約 35MB
- 隔離ディレクトリ数: 9 個

## 残留確認
- すべてのヘッダーファイルは使用されていることを確認
- ビルドファイル、テストファイル、スクリプトは使用中であることを確認
- docs/testing ディレクトリは README.md から参照されているため、隔離せず

## STATE.md 更新
- STATE.md を IDLE に更新
