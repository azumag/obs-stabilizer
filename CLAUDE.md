# OBS Stabilizer Plugin Project

Detailed comments must be included in the source code to justify the implementation of such logic

## トラブルシューティング履歴

### OBS起動時の問題
- Pkillでプロセス終了した際の異常終了に関する注意点
  - Pkillでプロセス終了すると前回のセッション中にOBSが正しくシャットダウンされなかった
  - サーフモード（サードパーティプラグイン、スクリプト、WebSocketが無効）での起動を促される
  - 正常終了を行うための対策が必要
  - scripts/以下にosascriptが用意してある

# important
- YAGNI（You Aren't Gonna Need It）：今必要じゃない機能は作らない
- DRY（Don't Repeat Yourself）：同じコードを繰り返さない
- KISS（Keep It Simple Stupid）：シンプルに保つ
- t-wada TDD: テスト駆動開発
- 絵文字を使うな
- 無駄にファイルを作りまくるな
- 一時ファイルは一箇所のディレクトリにまとめよ

## プロジェクト概要

OBS Studioのリアルタイム映像スタビライゼーションプラグインの開発プロジェクト。
既存のLiveVisionKitプロジェクトが開発停止したため、新しいオープンソース実装を作成する。

## 技術仕様

### アーキテクチャ
- **言語**: C++17/20 (Modern C++)
- **映像処理**: OpenCV 4.5+
- **ビルドシステム**: CMake
- **GUI**: Qt (OBS Studio標準)
- **プラットフォーム**: Windows, macOS, Linux

### スタビライゼーション技術

#### アルゴリズム選択肢
1. **Point Feature Matching** (推奨)
   - `goodFeaturesToTrack()` + Lucas-Kanade オプティカルフロー
   - リアルタイム性能に優れる（1-4ms/frame on HD）
   - メモリ使用量が少ない

2. **Feature-Based (SURF/ORB)**
   - より高精度だが計算コストが高い
   - GPU加速対応可能

3. **Transform-Based**
   - `estimateRigidTransform()` + `warpAffine()`
   - シンプルだが機能制限あり

#### 実装方針
- **段階的実装**: まずPoint Feature Matchingで基本機能を実装
- **パフォーマンス最適化**: SIMD、マルチスレッド活用
- **設定可能パラメータ**: 
  - Smoothing Radius
  - Feature検出閾値
  - 補正強度
  - クロップ vs パディング

### OBSプラグイン仕様

#### 基本構造
```
obs-stabilizer/
├── src/
│   ├── plugin-main.cpp      # プラグインエントリーポイント
│   ├── stabilizer-filter.cpp # メインフィルター実装
│   ├── stabilizer-core.cpp   # スタビライゼーションエンジン
│   └── ui/
│       └── stabilizer-ui.cpp # 設定UI
├── data/
│   └── locale/              # 多言語対応
├── CMakeLists.txt
└── buildspec.json
```

#### プラグイン機能
- **フィルタータイプ**: Video Filter
- **リアルタイム処理**: 30fps以上対応
- **設定UI**: プロパティパネル統合
- **プリセット機能**: 用途別設定保存

## 開発計画

### Phase 1: 基盤構築 (Week 1-2) ✅ **COMPLETE**
- [x] OBSプラグインテンプレート設定 (#1) ✅
- [x] OpenCV統合 (#2) ✅
- [x] 基本的なVideo Filter実装 (#3) ✅
- [x] **性能検証プロトタイプ作成** (#17) ✅
- [x] テストフレームワーク設定 (#10) ✅

**依存関係**: #1 → #2 → #3, #17（並行実行可能）

### Phase 2: コア機能実装 (Week 3-6) ✅ **COMPLETE**
- [x] Point Feature Matching実装 (#4) ✅ [Week 3-4]
- [x] スムージングアルゴリズム実装 (#5) ✅ [Week 4-5]
- [x] エラーハンドリング標準化 (#14) ✅ [Week 5-6]
- [x] 単体テスト実装 (#11) ✅ [継続的実装]

**依存関係**: Phase 1完了 → #4 → #5, #14（部分的並行可能）
**性能目標検証**: Phase 1の性能プロトタイプ結果に基づき目標値調整

### Phase 3: UI/UX・品質保証 (Week 7-8) ✅ **COMPLETE**
- [x] 設定パネル作成 (#6) ✅
- [x] パフォーマンステスト自動化 (#12) ✅
- [x] メモリ管理・リソース最適化 (#15) ✅
- [x] 統合テスト環境構築 (#13) ✅

**依存関係**: Phase 2コア機能 → UI実装, テスト並行実行

### Phase 4: 最適化・リリース準備 (Week 9-10)
- [ ] パフォーマンス調整 (#7)
- [ ] クロスプラットフォーム対応 (#8)
- [ ] デバッグ・診断機能実装 (#16)
- [ ] ドキュメント整備 (#9)

### Phase 5: 本格運用準備 (Week 11-12)
- [ ] CI/CD パイプライン構築 (#18)
- [ ] プラグイン配布・インストール機能 (#19)
- [ ] セキュリティ・脆弱性対応 (#21)
- [ ] コミュニティ・コントリビューション体制構築 (#20)

**総開発期間**: 6週間 → **12週間** (本格的なOSS運用まで含む現実的スケジュール)

## プラグイン構成トラブルシューティング

### メモリ
- 他のプラグインはインストール出来て読み込めているから問題ない。このプラグインの構成に問題がある

### ビルド
- 現在macようにbuildしたプラグインがうまく読み込めていない。他のプラグインは読み込めている。
- ビルド方法を検討し、読み込まれる様にすること。
- qtやopencvを排除したシンプルなプラグインでは読み込めているので、おそらくその辺りが原因だろう。-> @docs/plugin-loading-fix-report.md
- qtはobsの内臓のものを使うこと。
- 今読み込めている既存のライブラリを参考にすること。

