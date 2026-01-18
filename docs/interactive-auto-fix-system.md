# インタラクティブ自動修正システム

## 概要

tmuxを使ってopencodeと対話的にプラグインの問題を修正するシステム。

## 特徴

1. **tmuxセッション**: 3つのペーンに分割
   - 上: コントロールパネル
   - 左下: テスト結果
   - 右下: opencodeセッション

2. **対話的な修正**:
   - テストが失敗すると、opencodeに修正指示が送られる
   - opencodeが修正を実行
   - 修正完了後、テストが再実行される
   - これを繰り返す

3. **自動化**:
   - 手動介入なしでループを実行
   - 必要な修正を自動的に識別
   - opencodeを通じて自動修正を適用

## 使用方法

### 開始

```bash
# プロジェクトルートから実行
./run_interactive_fix.sh
```

### tmuxセッションへの接続

```bash
# 別のターミナルからセッションに接続
tmux attach -t obs-stabilizer-opencode-fix

# セッションを一時的に切り離す（tmux内で）
Ctrl+B, D

# セッションを閉じる
tmux kill-session -t obs-stabilizer-opencode-fix
```

## 動作フロー

1. **初期化**
   - tmuxセッションの作成
   - ペーンの分割
   - opencodeセッションの開始

2. **テスト実行**
   - 統合テストの実行
   - 結果の表示

3. **修正サイクル**
   - テストが失敗した場合:
     - 失敗したテストの分析
     - opencodeに修正指示を送信
     - opencodeが修正を実行
     - プラグインの再ビルド
     - テストの再実行

4. **成功時**
   - 全テスト合格時、ループ終了
   - セッションは開いたまま（結果確認用）

## 修正コマンドの種類

| テスト | 修正コマンド |
|--------|-------------|
| Pre-flight | 環境問題の修正 |
| Build | CMakeLists.txtと依存関係の確認 |
| Plugin Loading | プラグインバイナリとライブラリパスの確認 |
| Basic Functionality | 実行時エラーの修正 |
| Crash Detection | クラッシュログの分析と修正 |

## tmuxペーンの使用

### Controlペーン（上）
- メインのコントロールとログ
- 反復回数の表示
- ステータスの表示

### Testsペーン（左下）
- テスト結果の表示
- JSON形式の詳細な結果

### opencodeペーン（右下）
- opencodeセッション
- 修正指示の受信
- 修正の実行

## 自動修正の仕組み

修正は2段階で行われます：

### 1. Bashスクリプトによる自動修正

以下のbashスクリプトが実行されます：
- `fix_preflight.sh` - 環境問題の修正
- `fix_build.sh` - ビルド問題の修正
- `fix_plugin_loading.sh` - プラグインロード問題の修正
- `fix_basic_functionality.sh` - 機能問題の修正
- `fix_crash.sh` - クラッシュ問題の修正

### 2. opencodeによるコード修正

bashスクリプトで修正できない場合、opencodeに以下のコマンドが送られます：

```bash
# 例：ビルド失敗
Build failed. Please check CMakeLists.txt and dependencies.

# 例：プラグインロード失敗
Plugin loading failed. Please check plugin binary and library paths.

# 例：クラッシュ検出
Crash detected. Please analyze crash logs and fix the issue.
```

## 手動介入の必要なし

このシステムは以下の理由で完全に自動化されています：

1. **手動修正を要求しない**
   - 可能な限り自動修正
   - opencodeを通じて自動適用

2. **反復実行**
   - 最大反復回数の制限なし
   - 全テスト合格まで継続

3. **自動リビルド**
   - 修正後の自動リビルド
   - 自動再テスト

## ログと結果

### ログの場所

```bash
# テストログ
ls -t tests/integration/results/test_*.log

# テスト結果（JSON）
ls -t tests/integration/results/results_*.json

# 修正ログ
cat /tmp/fix_log.txt

# リビルドログ
cat /tmp/rebuild_log.txt

# 反復ごとのテストログ
ls /tmp/test_iteration_*.log
```

### 結果の確認

```bash
# 最新の結果
cat tests/integration/results/results_*.json | jq .

# 反復回数ごとのログ
cat /tmp/test_iteration_*.log
```

## トラブルシューティング

### opencodeが応答しない場合

1. tmuxセッションに接続
2. opencodeペーンで応答を確認
3. 必要に応じて手動で修正

### テストが失敗し続ける場合

1. 最新のテストログを確認
2. 失敗したテストを特定
3. 手動でコードを修正

### セッションがハングした場合

```bash
# セッションを強制終了
tmux kill-session -t obs-stabilizer-opencode-fix

# 再起動
./run_interactive_fix.sh
```

## 設定

スクリプトの先頭で設定を変更可能：

```bash
TMUX_SESSION="obs-stabilizer-opencode-fix"  # セッション名
MAX_ITERATIONS=100                          # 最大反復回数（無制限）
```

## 注意点

1. **永続的な実行**
   - 手動で停止するまで実行し続けます
   - 全テスト合格時に停止します

2. **リソース使用**
   - tmux、opencode、OBSが同時に実行されます
   - 十分なCPUとメモリが必要です

3. **バックアップ**
   - 修正前のファイルは `.backup` として保存されます
   - 必要に応じて復元可能です

## 比較

### 従来の方法
- テスト実行
- 結果確認
- 手動修正
- 再テスト
- 繰り返し...

### インタラクティブシステム
- テスト実行
- 自動修正（opencode経由）
- 自動再テスト
- 繰り返し...

## 次のステップ

1. `./run_interactive_fix.sh` を実行
2. tmuxセッションに接続して進行を監視
3. 全テスト合格を待つ
4. 結果を確認

## 開発

新しい修正パターンを追加する場合：

1. `fix_patterns/` に新しいbashスクリプトを追加
2. `run_interactive_fix.sh` の `send_fix_command()` 関数を更新
3. テストして動作を確認
