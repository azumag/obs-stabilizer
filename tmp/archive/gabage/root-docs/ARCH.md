# 設計書: NEONFeatureDetector setter methods self-assignment bug fix

## 対象Issue

- Issue: #300 BUG: NEONFeatureDetector setter methods have self-assignment bug
- URL: https://github.com/azumag/obs-stabilizer/issues/300

## 問題の概要

`NEONFeatureDetector` クラスの setter メソッドにて、パラメータ名がメンバ変数と同名であるため、自己代入が発生し、メンバ変数が更新されないバグがある。

影響を受けるメソッド:
- `set_block_size(int block_size)` 
- `set_ksize(int ksize)`

これらのメソッドは呼び出されてもメンバ変数を変更せず、設定変更が無視されたように見える。

## 解決方針

パラメータ名とメンバ変数名の衝突を解決する。最もシンプルかつ明確な方法は、`this->` プリフィックスを使用してメンバ変数を明示的に参照すること。

## 詳細設計

### 変更対象ファイル

| ファイル | 変更種別 | 変更内容 |
|----------|----------|----------|
| src/core/neon_feature_detection.cpp | 修正 | `set_block_size()` と `set_ksize()` で `this->` を追加 |
| src/core/neon_feature_detection.hpp | 修正 | スタブ実装の setter で `this->` を追加 |

### 変更内容の詳細

#### src/core/neon_feature_detection.cpp (Apple ARM64実装)

**現在のコード (lines 33-39):**
```cpp
void set_block_size(int block_size) {
    block_size = std::max(1, std::min(31, block_size));
}

void set_ksize(int ksize) {
    ksize = std::max(1, std::min(31, ksize));
}
```

**修正後:**
```cpp
void set_block_size(int block_size) {
    this->block_size = std::max(1, std::min(31, block_size));
}

void set_ksize(int ksize) {
    this->ksize = std::max(1, std::min(31, ksize));
}
```

#### src/core/neon_feature_detection.hpp (スタブ実装)

**現在のコード (lines 77-78):**
```cpp
void set_block_size(int block_size) { block_size = block_size; }
void set_ksize(int ksize) { ksize = ksize; }
```

**修正後:**
```cpp
void set_block_size(int block_size) { this->block_size = block_size; }
void set_ksize(int ksize) { this->ksize = ksize; }
```

### データモデルの変更

なし

### API の変更

なし - インターフェースは変更せず、バグ修正のみ

## テスト計画

### 単体テスト

現在 `NEONFeatureDetector` 用のテストが存在しないため、以下のテストを新規作成:

- [ ] `test_neon_feature_detection.cpp` を作成
- [ ] `set_block_size()` が正しく値を設定することを検証
- [ ] `set_ksize()` が正しく値を設定することを検証
- [ ] 境界値テスト (1, 31, 0, 32 など)
- [ ] getter がないため、`detect_features_opencv()` の挙動変化で間接的に検証

### 影響を受ける既存テスト

- [ ] 既存の全テストがパスすることを確認

## 影響範囲

- `NEONFeatureDetector` クラスのみ
- このクラスは `stabilizer_core.cpp` で使用されている
- 設定値の変更が実際に反映されるようになる（以前は無視されていた）

## 注意事項

1. **両方の実装を修正**: .cpp (Apple ARM64) と .hpp (スタブ) の両方に同じバグがあるため、両方を修正すること
2. **動作変更**: バグ修正後、setter メソッドが実際に動作するようになるため、設定変更が反映される
3. **後方互換性**: これはバグ修正であり、意図した動作への修正なので後方互換性の問題はない
4. **コンパイラ警告**: 自己代入に関するコンパイラ警告が消えるはず
