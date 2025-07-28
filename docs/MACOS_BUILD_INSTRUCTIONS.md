# macOS OBS Stabilizer Plugin ビルド手順

## 問題の要約

現在、プラグインが実行ファイルとしてビルドされ、OBSに認識されていません。正しい`.plugin`バンドル形式でビルドする必要があります。

## 解決手順

### 1. 既存の間違ったファイルを削除

```bash
# 現在OBSプラグインフォルダにある間違ったファイルを削除
sudo rm -f "/Applications/OBS.app/Contents/PlugIns/obs-stabilizer"
sudo rm -f "/Applications/OBS.app/Contents/PlugIns/obs-stabilizer-0.1.0"
```

### 2. プラグインをビルド

```bash
# プロジェクトディレクトリに移動
cd /Users/azumag/work/obs-stabilizer

# ビルドディレクトリをクリーンアップ
rm -rf build
mkdir build
cd build

# CMakeで設定（OBSヘッダーが見つかるはず）
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# ビルド実行
make -j$(sysctl -n hw.ncpu)
```

### 3. ビルド結果の確認

成功すると以下のような出力が表示されるはずです：
```
-- Found OBS headers at: /Users/azumag/work/obs-stabilizer/include/obs
-- Building as OBS plugin (shared library)
```

そして `build/obs-stabilizer.plugin/` ディレクトリが作成されるはずです。

### 4. プラグインバンドルをOBSにインストール

```bash
# プラグインバンドルをOBSプラグインフォルダにコピー
sudo cp -r build/obs-stabilizer.plugin "/Applications/OBS.app/Contents/PlugIns/"

# 正しい権限を設定
sudo chmod -R 755 "/Applications/OBS.app/Contents/PlugIns/obs-stabilizer.plugin"
```

### 5. OBS Studioを再起動

OBS Studioを完全に終了してから再起動します。

### 6. プラグインの確認

1. OBS Studioで適当なビデオソースを追加
2. ソースを右クリック → "フィルタ"
3. "+" → "Video Filter" → "Stabilizer" が表示されるはず

## トラブルシューティング

### プラグインが表示されない場合

1. **ログを確認**：
   - OBS Studio → Help → Log Files → Current Log
   - プラグインロードエラーがないか確認

2. **ファイル構造を確認**：
   ```bash
   ls -la "/Applications/OBS.app/Contents/PlugIns/obs-stabilizer.plugin"
   ```
   以下のような構造になっているはず：
   ```
   obs-stabilizer.plugin/
   ├── Contents/
   │   ├── Info.plist
   │   ├── MacOS/
   │   │   └── obs-stabilizer
   │   └── Resources/
   │       └── locale/
   │           └── en-US.ini
   ```

3. **権限を確認**：
   ```bash
   ls -la "/Applications/OBS.app/Contents/PlugIns/obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer"
   ```
   実行可能権限があることを確認

### ビルドが実行ファイルになってしまう場合

CMake出力で以下が表示される場合：
```
-- Building as standalone executable for testing
```

これは OBS ヘッダーが見つからない場合です。以下を確認：

1. `include/obs/obs-module.h` ファイルが存在するか
2. CMake設定でヘッダーパスが正しく設定されているか

## 必要なファイル

プロジェクトには以下のファイルが追加されています：

- `include/obs/obs-module.h` - OBS基本ヘッダー
- `include/obs/obs-data.h` - データ処理ヘッダー  
- `include/obs/obs-properties.h` - プロパティシステムヘッダー
- `Info.plist.in` - macOSプラグインバンドル設定
- `build-plugin.sh` - ビルドスクリプト（実行権限が必要）

## 設定内容

- **プラグインタイプ**: Video Filter
- **プラグインID**: obs-stabilizer
- **バンドルID**: com.obsproject.obs-stabilizer
- **対応フォーマット**: NV12, I420
- **スタビライゼーション**: Point Feature Matching + Lucas-Kanade Optical Flow