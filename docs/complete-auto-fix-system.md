# OBS Stabilizer - 自動結合テストシステム完成

## 概要

3つのテストモードを提供する完全な自動結合テストシステム：

1. **Quick Test**: 単純なテスト実行（一回のみ）
2. **Interactive Auto-Fix**: tmux + opencodeによる対話的修正
3. **Automated Auto-Fix Loop**: 完全自動修正ループ

## ディレクトリ構造

```
obs-stabilizer/
├── test.sh                                   # メイン実行スクリプト（3つのモード選択）
├── run_interactive_fix.sh                     # インタラクティブ自動修正
├── run_auto_fix.sh                           # 完全自動修正ループ
└── tests/integration/
    ├── README.md                              # 詳細ドキュメント
    ├── README_ja.md                           # 日本語ドキュメント
    ├── run_integration_tests.sh                # 基本的なテストスイート
    ├── auto_fix.py                            # Python自動修正
    ├── auto_fix_enhanced.py                   # 強化版自動修正
    ├── test_00_preflight.sh                   # テストスクリプト群
    ├── test_01_build.sh
    ├── test_02_plugin_loading.sh
    ├── test_03_basic_functionality.sh
    ├── test_04_crash_detection.sh
    ├── test_99_cleanup.sh
    └── fix_patterns/                          # 修正スクリプト群
        ├── fix_preflight.sh
        ├── fix_build.sh
        ├── fix_plugin_loading.sh
        ├── fix_basic_functionality.sh
        └── fix_crash.sh
```

## 使用方法

### モード選択

```bash
./test.sh
```

メニューが表示されます：

```
Choose test mode:
1) Quick test (run tests once)
2) Interactive auto-fix (tmux + opencode)
3) Automated auto-fix loop (tmux, no opencode interaction)
```

### 各モードの説明

#### モード1: Quick Test

単純なテスト実行です。

```bash
./test.sh
# 選択: 1
```

**特徴**:
- テストを一回実行
- 結果を表示
- 終了

**使用例**:
- プラグインが正しくビルド・ロードできるかを確認
- 修正後の動作確認

#### モード2: Interactive Auto-Fix（推奨）

tmuxでopencodeと対話的に修正を行います。

```bash
./test.sh
# 選択: 2
```

**tmuxレイアウト**:
```
┌─────────────────────────────────────────┐
│  Control Panel                         │
│  - 反復回数、ステータス                 │
├──────────────────────┬────────────────┤
│  Test Results      │  opencode       │
│  - テスト結果        │  - 修正指示     │
│  - JSON形式の詳細    │  - 修正実行     │
└──────────────────────┴────────────────┘
```

**動作フロー**:
1. テスト実行
2. 失敗した場合、opencodeに修正指示を送信
3. opencodeが修正を実行
4. プラグインを再ビルド
5. テストを再実行
6. 全テスト合格まで繰り返し

**tmux操作**:
```bash
# セッションに接続
tmux attach -t obs-stabilizer-opencode-fix

# セッションから切り離す（tmux内で）
Ctrl+B, D

# セッションを閉じる
tmux kill-session -t obs-stabilizer-opencode-fix
```

**特徴**:
- opencodeが実際にtmuxペーンで起動
- 対話的な修正
- 手動介入なしでループ実行
- 全テスト合格まで継続

**修正コマンドの種類**:

| テスト | 修正コマンド |
|--------|-------------|
| Pre-flight | 環境問題の修正指示 |
| Build | CMakeLists.txtと依存関係の確認指示 |
| Plugin Loading | プラグインバイナリとライブラリパスの確認指示 |
| Basic Functionality | 実行時エラーの修正指示 |
| Crash Detection | クラッシュログの分析と修正指示 |

#### モード3: Automated Auto-Fix Loop

完全自動の修正ループです。

```bash
./test.sh
# 選択: 3
```

**tmuxレイアウト**:
```
┌─────────────────────────────────────────┐
│  Control Panel                         │
├──────────────────────┬────────────────┤
│  Test Results      │  Auto-Fix       │
│  - テスト結果        │  - 修正ログ     │
│  - JSON形式の詳細    │  - 自動修正     │
└──────────────────────┴────────────────┘
```

**特徴**:
- opencodeの対話なし
- 自動修正スクリプトのみ使用
- 最大10回の反復
- 10回失敗で終了

## 推奨使用フロー

### 初回修正開始

```bash
./test.sh
# 選択: 2 (Interactive Auto-Fix)
```

### 進行状況の監視

```bash
# 別のターミナルからtmuxセッションに接続
tmux attach -t obs-stabilizer-opencode-fix
```

### 全テスト合格の確認

コントロールパネルに「ALL TESTS PASSED!」と表示されます。

### 結果の確認

```bash
# テスト結果（最新）
cat tests/integration/results/results_*.json | jq .

# テストログ
cat tests/integration/results/test_*.log

# 修正履歴
cat .fix_history.json
```

## 修正履歴

修正が適用されるたびに、`.fix_history.json` に記録されます：

```json
{
  "cmake_obs_path": 1,
  "cmake_rpath": 2,
  "symbol_bridging": 1,
  "settings_crash": 1,
  "null_pointer": 1,
  "exception_handling": 1,
  "thread_safety": 1,
  "mutex_usage": 1
}
```

これにより、同じ修正が何度も適用されるのを防ぎます。

## 自動終了しない

**重要**: システムは手動で停止するまで実行し続けます！

### 停止方法

```bash
# tmuxセッションを閉じる
tmux kill-session -t obs-stabilizer-opencode-fix

# または、コントロールペーンでCtrl+C
```

### 自動終了の条件

システムが自動終了するのは以下の場合のみ：

1. **全テスト合格**: すべてのテストがパスした場合
2. **最大反復回数到達**: 自動修正ループのみ（10回）

## 手動介入を要求しない

このシステムは**決して手動修正を要求しません**。

### 完全に自動化された処理

1. **テスト失敗検出**
2. **自動修正スクリプト実行**:
   - bashスクリプトによる環境修正
   - Pythonスクリプトによるコード修正
3. **opencodeによる修正**（インタラクティブモードのみ）
4. **自動リビルド**
5. **自動再テスト**
6. **繰り返し**

### 修正できない問題

自動修正できない問題でも、システムは継続します：
- 次の反復で試みる
- 異なる修正パターンを試す
- 最大反復回数まで続ける

手動介入は**不要**です。

## ログファイル

### すべてのログ

```bash
# テスト結果
ls -t tests/integration/results/results_*.json | head -1

# テストログ
ls -t tests/integration/results/test_*.log | head -1

# 反復ごとのログ
ls /tmp/test_iteration_*.log

# 修正ログ
cat /tmp/fix_log.txt

# リビルドログ
cat /tmp/rebuild_log.txt

# 自動修正出力
cat /tmp/autofix_output.log
```

### 最新の修正サマリー

```bash
cat last_fix_summary.json
```

## トラブルシューティング

### tmuxセッションがハングした場合

```bash
# 強制終了
tmux kill-session -t obs-stabilizer-opencode-fix

# 再起動
./test.sh
# 選択: 2
```

### テストが失敗し続ける場合

1. ログを確認
2. 修正履歴を確認
3. システムは継続します（手動介入不要）

### opencodeが応答しない場合

```bash
# tmuxセッションに接続
tmux attach -t obs-stabilizer-opencode-fix

# opencodeペーンで応答を確認
# 必要に応じて手動で修正（通常不要）
```

### ビルドが失敗し続ける場合

システムは継続して異なる修正パターンを試みます。

手動で確認する場合：

```bash
# 最新のリビルドログ
cat /tmp/rebuild_log.txt

# テストログ
ls -t tests/integration/results/test_*.log | head -1
```

## 完成したシステムの特徴

### ✅ 完全に自動化
- 手動介入なし
- 対話的修正（opencode経由）
- 自動リビルド
- 自動再テスト

### ✅ 永続的な実行
- 手動停止まで継続
- 無制限の反復
- 全テスト合格まで

### ✅ 対話的な可視化
- tmuxによるペーン分割
- リアルタイムの進行表示
- 詳細なテスト結果

### ✅ インテリジェントな修正
- 修正履歴の追跡
- 同じ修正の重複回避
- 複数の修正パターン

### ✅ 網羅的なテスト
- 環境チェック
- ビルド検証
- プラグインロード
- 基本機能
- クラッシュ検出

## 詳細ドキュメント

- `tests/integration/README.md` - 詳細な使用方法
- `docs/integration-test-system-setup.md` - システムセットアップ
- `docs/interactive-auto-fix-system.md` - インタラクティブ修正システム

## 次のステップ

```bash
# インタラクティブ自動修正を開始
./test.sh
# 選択: 2

# tmuxセッションに接続して監視
tmux attach -t obs-stabilizer-opencode-fix

# 進行を観察
# 全テスト合格を待つ
```
