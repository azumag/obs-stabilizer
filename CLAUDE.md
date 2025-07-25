# OBS Stabilizer Plugin Project

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

### Phase 1: 基盤構築 (Week 1-2)
- [x] OBSプラグインテンプレート設定 (#1) ✅
- [ ] OpenCV統合 (#2)
- [ ] 基本的なVideo Filter実装 (#3)
- [ ] **性能検証プロトタイプ作成** (#17) ← **リスク軽減のため前倒し**
- [ ] テストフレームワーク設定 (#10)

**依存関係**: #1 → #2 → #3, #17（並行実行可能）

### Phase 2: コア機能実装 (Week 3-6) ← **期間延長**
- [ ] Point Feature Matching実装 (#4) [Week 3-4]
- [ ] スムージングアルゴリズム実装 (#5) [Week 4-5]
- [ ] エラーハンドリング標準化 (#14) [Week 5-6]
- [ ] 単体テスト実装 (#11) [継続的実装]

**依存関係**: Phase 1完了 → #4 → #5, #14（部分的並行可能）
**性能目標検証**: Phase 1の性能プロトタイプ結果に基づき目標値調整

### Phase 3: UI/UX・品質保証 (Week 7-8)
- [ ] 設定パネル作成 (#6)
- [ ] パフォーマンステスト自動化 (#12)
- [ ] メモリ管理・リソース最適化 (#15)
- [ ] 統合テスト環境構築 (#13)

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

## Issue管理方針

### ラベル体系
- `type:feature` - 新機能実装
- `type:bug` - バグ修正
- `type:enhancement` - 既存機能改善
- `type:docs` - ドキュメント関連
- `priority:high` - 高優先度
- `priority:medium` - 中優先度
- `priority:low` - 低優先度
- `platform:windows` - Windows固有
- `platform:macos` - macOS固有
- `platform:linux` - Linux固有

### Issue参照方法
```bash
# 新しいissueを作成
gh issue create --title "タイトル" --body "詳細" --label "type:feature,priority:high"

# issueの確認
gh issue list
gh issue view [issue_number]

# issueの完了
gh issue close [issue_number] --comment "実装完了"
```

### 開発フロー
1. **Issue作成**: 機能/バグをissueに登録
2. **ブランチ作成**: `feature/issue-XX` or `bugfix/issue-XX`
3. **実装**: コミットメッセージに `#XX` でissue参照
4. **PR作成**: レビュー後マージ
5. **Issue完了**: PRマージ時に自動クローズ

## 品質基準

### コーディング規約
- C++17/20 Modern C++スタイル
- OBS Studio コーディングガイドライン準拠
- RAII、スマートポインタ使用
- const correctness

### テスト方針
- **単体テスト（Google Test）**: コアロジック、アルゴリズム検証
- **統合テスト（実際のOBS環境）**: プラグイン動作、長時間安定性
- **パフォーマンステスト（各解像度での処理時間計測）**: リアルタイム処理目標達成
- **メモリリークテスト**: Valgrind/AddressSanitizer使用

### CI/CD
- GitHub Actions使用
- 各プラットフォームでの自動ビルド
- リリース自動化

## エラーハンドリング・例外処理仕様

### 統一エラー処理戦略

#### OpenCV例外処理
```cpp
// 統一ラッパーでOpenCV例外を処理
class CVExceptionHandler {
    template<typename Func>
    static bool safe_execute(Func&& func, const char* operation_name) {
        try {
            func();
            return true;
        } catch (const cv::Exception& e) {
            log_error("OpenCV error in %s: %s", operation_name, e.what());
            return false;
        }
    }
};
```

#### 特徴点検出失敗時のフォールバック
1. **検出点数不足** (<50点): 
   - 検出パラメータを緩和して再試行
   - 前フレームの変換行列を使用
2. **連続検出失敗** (5フレーム以上):
   - スタビライゼーション一時停止
   - パススルーモードに切り替え

#### メモリ・リソース不足対応
```cpp
// グレースフルデグラデーション戦略
enum class ProcessingLevel {
    FULL_QUALITY,    // 通常処理
    REDUCED_FEATURES, // 特徴点数削減
    SIMPLIFIED,      // 簡易アルゴリズム
    PASSTHROUGH     // 処理停止
};
```

#### ログレベル仕様
- **ERROR**: 処理継続不可能なエラー
- **WARN**: 品質低下を伴う処理継続
- **INFO**: 状態変化・設定変更の記録
- **DEBUG**: 詳細な処理状況（開発時のみ）

### リソース管理原則
- **RAII**: 全リソースの自動管理
- **スマートポインタ**: 生ポインタ使用禁止
- **例外安全性**: Strong exception safety guarantee

## ライセンス

GPL-2.0（OBS Studio互換）

## 参考資料

- [LiveVisionKit GitHub](https://github.com/Crowsinc/LiveVisionKit)
- [OBS Plugin Template](https://github.com/obsproject/obs-plugintemplate)
- [OpenCV Video Stabilization](https://learnopencv.com/video-stabilization-using-point-feature-matching-in-opencv/)