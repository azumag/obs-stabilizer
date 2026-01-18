# 自動結合テストシステム構築完了

## 概要

ローカルで自動結合テストを実行し、テスト失敗時に自動修正するシステムを構築しました。

## ディレクトリ構造

```
obs-stabilizer/
├── test.sh                                    # クイックテスト実行スクリプト（プロジェクトルート）
└── tests/integration/
    ├── README.md                              # 詳細ドキュメント
    ├── README_ja.md                           # 日本語ドキュメント
    ├── run_integration_tests.sh                # メインテストランナー
    ├── auto_fix.py                            # Pythonによるコード自動修正
    ├── test_00_preflight.sh                   # 事前チェック
    ├── test_01_build.sh                       # ビルドテスト
    ├── test_02_plugin_loading.sh              # プラグインロードテスト
    ├── test_03_basic_functionality.sh          # 基本機能テスト
    ├── test_04_crash_detection.sh             # クラッシュ検出テスト
    ├── test_99_cleanup.sh                     # クリーンアップ
    ├── fix_patterns/                          # 自動修正スクリプト
    │   ├── fix_preflight.sh
    │   ├── fix_build.sh
    │   ├── fix_plugin_loading.sh
    │   ├── fix_basic_functionality.sh
    │   └── fix_crash.sh
    └── results/                              # テスト結果とログ
```

## 使用方法

### クイックスタート

```bash
# プロジェクトルートから
./test.sh

# または直接実行
./tests/integration/run_integration_tests.sh
```

### 詳細なテスト実行

1. **全テスト実行（自動修正付き）**
   ```bash
   ./tests/integration/run_integration_tests.sh
   ```

2. **結果の確認**
   ```bash
   cat tests/integration/results/results_*.json | jq .
   ```

3. **ログの確認**
   ```bash
   cat tests/integration/results/test_*.log
   ```

4. **個別の自動修正実行**
   ```bash
   python3 tests/integration/auto_fix.py
   ```

## 機能

### 自動結合テスト

| テスト | 内容 |
|--------|------|
| Pre-flight | 環境チェック、問題のあるプラグインの検出 |
| Build | CMake設定、コンパイル、リンクの検証 |
| Plugin Loading | OBSへのプラグインロード確認 |
| Basic Functionality | OBSでの実行、エラー検出、メモリ使用量 |
| Crash Detection | クラッシュ監視、クラッシュログ分析 |

### 自動修正機能

テスト失敗時に自動的に以下を試みます：

1. **環境修正（Bashスクリプト）**
   - 問題のあるプラグインの削除
   - OBS設定のリセット
   - 動的ライブラリパスの修正
   - 依存関係のインストール

2. **コード修正（Pythonスクリプト）**
   - NULLポインタチェックの追加
   - 例外ハンドリングの追加
   - スレッドセーフティの強化
   - CMakeLists.txtの修正
   - シンボルブリッジの追加
   - 自動リビルド

## テスト結果

### JSON形式

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

### 修正可能な問題

| 問題 | 自動修正 |
|------|----------|
| ビルドエラー | ✅ 依存関係インストール、CMake修正 |
| シンボル未定義エラー | ✅ シンボルブリッジ追加 |
| プラグインロード失敗 | ✅ ライブラリパス修正 |
| 設定アクセスクラッシュ | ✅ コードパターン修正 |
| NULLポインタクラッシュ | ✅ NULLチェック追加 |
| OpenCV例外 | ✅ 例外ハンドリング追加 |
| スレッド競合 | ✅ Mutex追加 |

## 注意点

1. **自動修正の制限**
   - 自動修正は一般的な問題に対応します
   - 複雑なバグは手動修正が必要です

2. **バックアップ**
   - 修正前のファイルは `.backup` として保存されます

3. **OBSの状態**
   - テスト実行中はOBSが閉じている必要があります

4. **結果の保存**
   - テスト結果は `tests/integration/results/` に保存されます

## 改善可能な点

1. より高度なコード分析
2. より多くのバグパターンの検出
3. クラッシュログの詳細分析
4. より良いエラーレポートの生成

## 次のステップ

1. テストを実行して現状を確認
2. 失敗するテストがある場合は修正を確認
3. 自動修正できない問題は手動で修正
4. 新しいバグパターンを見つけたら自動修正に追加

## 参考

- 詳細ドキュメント: `tests/integration/README.md`
- 各テストスクリプトのヘッダーコメント
- 修正スクリプトの実装を確認
