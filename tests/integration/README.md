# OBS Stabilizer - 自動結合テストシステム

OBS Stabilizerプラグインの自動結合テストと自動修正システム。

## 概要

このシステムは以下の機能を提供します：

1. **自動結合テスト**: プラグインのビルド、ロード、機能、クラッシュ検出をテスト
2. **自動修正**: テスト失敗時に一般的な問題を自動的に修正
3. **結果追跡**: JSON形式でテスト結果を記録

## ディレクトリ構造

```
tests/integration/
 ├── auto_fix.py                   # Pythonによるコード自動修正
 ├── test_00_preflight.sh          # 事前チェックテスト
 ├── test_01_build.sh              # ビルドテスト
 ├── test_02_plugin_loading.sh     # プラグインロードテスト
 ├── test_03_basic_functionality.sh # 基本機能テスト
 ├── test_04_crash_detection.sh    # クラッシュ検出テスト
 ├── test_99_cleanup.sh            # クリーンアップ
 ├── fix_patterns/                 # 自動修正スクリプト
 │   ├── fix_preflight.sh
 │   ├── fix_build.sh
 │   ├── fix_plugin_loading.sh
 │   ├── fix_basic_functionality.sh
 │   └── fix_crash.sh
 └── results/                      # テスト結果とログ
```

Note: 各テストスクリプトは個別に実行可能です。一括実行スクリプトはまだ実装されていません。

## 使用方法

### 1. テストの実行

```bash
# プロジェクトのルートから実行
./tests/integration/run_integration_tests.sh
```

このコマンドは以下を実行します：

- 各テストを順次実行
- テスト失敗時に自動修正スクリプトを試行
- 修正後にテストを再実行
- 結果をJSONで保存

### 2. テスト結果の確認

```bash
# 最新の結果を表示
cat tests/integration/results/results_*.json | jq .

# テストログを確認
cat tests/integration/results/test_*.log
```

### 3. 自動修正の実行

テストが失敗した後、個別に自動修正を実行できます：

```bash
# Pythonによるコード修正
python3 tests/integration/auto_fix.py
```

## テスト内容

### Test 0: Pre-flight Checks
環境設定の事前チェック
- OBS Studioがインストールされているか
- プラグインディレクトリが存在するか
- ビルドツールが利用可能か
- 問題のあるプラグインがインストールされていないか

### Test 1: Build Verification
プラグインのビルド検証
- CMake設定の確認
- コンパイルとリンク
- バイナリの検証

### Test 2: Plugin Loading
プラグインのロードテスト
- OBSへのプラグインインストール
- OBS起動時のプラグインロード確認
- ログによるロード成功の検証

### Test 3: Basic Functionality
基本機能テスト
- OBSでの安定実行
- プラグインエラーの検出
- メモリ使用量の確認

### Test 4: Crash Detection
クラッシュ検出テスト
- OBSの安定性監視
- クラッシュログの検出
- クラッシュ原因の分析

## 自動修正機能

### 修正可能な問題

1. **ビルド問題**
   - 欠損している依存関係のインストール
   - CMakeLists.txtの修正
   - シンボルブリッジの追加

2. **プラグインロード問題**
   - 動的ライブラリパスの修正
   - OBS設定のリセット
   - パーミッションの修正

3. **機能問題**
   - NULLポインタチェックの追加
   - 例外ハンドリングの追加
   - スレッドセーフティの強化

4. **クラッシュ問題**
   - クラッシュログの分析
   - メモリアクセス違反の特定
   - OpenCV関連問題の検出

### 修正の仕組み

修正は2段階で行われます：

1. **Bashスクリプトによる環境修正**
   - システム設定の修正
   - パッケージのインストール
   - OBS設定のリセット

2. **Pythonスクリプトによるコード修正**
   - ソースコードの解析
   - バグパターンの検出
   - 自動コード修正
   - プラグインの再ビルド

## 詳細設定

### 環境変数

```bash
# OBSのパス（必要な場合）
export OBS_APP="/path/to/OBS.app/Contents/MacOS/OBS"

# プラグインディレクトリ
export OBS_PLUGINS_DIR="$HOME/Library/Application Support/obs-studio/plugins"
```

### テストのスキップ

特定のテストをスキップするには、テスト番号をコメントアウトします：

```bash
# ビルドテストをスキップ
# run_test "Build Verification" \
#     "$SCRIPT_DIR/test_01_build.sh" \
#     "$SCRIPT_DIR/fix_patterns/fix_build.sh"
```

## トラブルシューティング

### テストが失敗する場合

1. **ログを確認する**
   ```bash
   cat tests/integration/results/test_*.log
   ```

2. **手動で修正スクリプトを実行**
   ```bash
   ./tests/integration/fix_patterns/fix_build.sh
   ```

3. **クラッシュログを確認**
   ```bash
   ls -t ~/Library/Logs/DiagnosticReports/OBS*.crash | head -1
   ```

### 自動修正が失敗する場合

自動修正できない問題の場合は、以下を確認してください：

1. **ソースコードのバグ**
   - ログからエラー原因を特定
   - 手動でコードを修正

2. **環境の問題**
   - OBSのバージョン確認
   - macOSのバージョン確認
   - 依存関係の確認

### OBSがクラッシュし続ける場合

```bash
# プラグインを無効化
mv ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer \
   ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer.disabled

# OBS設定をリセット
./tests/integration/fix_patterns/fix_preflight.sh
```

## 結果のフォーマット

### JSON結果の例

```json
{
  "timestamp": "2025-01-15T10:30:00Z",
  "tests": [
    {
      "name": "Build Verification",
      "status": "passed",
      "message": "Test completed successfully",
      "fix_attempted": false,
      "fix_successful": false
    },
    {
      "name": "Plugin Loading",
      "status": "failed",
      "message": "Plugin failed to load",
      "fix_attempted": true,
      "fix_successful": true
    }
  ],
  "summary": {
    "total": 5,
    "passed": 4,
    "failed": 0,
    "fixed": 1
  }
}
```

## 継続的改善

このテストシステムは以下の目的で継続的に改善されます：

- より多くのバグパターンの検出
- より高度な自動修正の実装
- より良いエラーレポートの生成

## 貢献

新しいテストケースや修正パターンを追加する場合：

1. 新しいテストスクリプトを `test_xx_name.sh` として追加
2. 対応する修正スクリプトを `fix_patterns/fix_name.sh` に追加
3. メインのテストランナーにテストを追加
4. 動作を確認

## ライセンス

このテストシステムはプロジェクトと同じライセンスで提供されます。
