# Oblivion Steam版 リバースエンジニアリングレポート：フォント情報

解析日: 2026-10-04
対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版`

## 1. 概要

Oblivion のゲーム内フォント情報を確定するため、Android版のフォント実装を確認し、オリジナル版（Steam版）のフォントデータを解析した。

**結論**: Android版のフォント5種（.fnt + アトラス）は、オリジナル版の `Oblivion - Misc.bsa` から抽出したデータと**ピクセル単位で完全一致**（max diff = 0）。フォントの本物化は完了済みで、追加の差し替えは不要。

## 2. フォント定義（Oblivion_default.ini）

`Oblivion_default.ini` の `[Fonts]` セクションで5つのフォントが定義されている。

```ini
[Fonts]
SFontFile_1=Data\Fonts\Kingthings_Regular.fnt
SFontFile_2=Data\Fonts\Kingthings_Shadowed.fnt
SFontFile_3=Data\Fonts\Tahoma_Bold_Small.fnt
SFontFile_4=Data\Fonts\Daedric_Font.fnt
SFontFile_5=Data\Fonts\Handwritten.fnt
```

## 3. フォントファイルの所在（Oblivion - Misc.bsa）

`Data\Oblivion - Misc.bsa`（BSA103、7.2 MB）の `fonts\` フォルダに10ファイルが格納されている。

| ファイル | サイズ | 種類 |
|----------|--------|------|
| kingthings_regular.fnt | 14,632 B | グリフ定義 |
| kingthings_regular_0_lod_a.tex | 1,048,584 B | アトラス（512x512 RGBA） |
| kingthings_shadowed.fnt | 14,632 B | グリフ定義 |
| kingthings_shadowed_0_lod_a.tex | 1,048,584 B | アトラス（512x512 RGBA） |
| tahoma_bold_small.fnt | 14,632 B | グリフ定義 |
| tahoma_bold_small_0_lod_a.tex | 262,152 B | アトラス（256x256 RGBA） |
| daedric_font.fnt | 14,632 B | グリフ定義 |
| daedric_font_0_lod_a.tex | 1,048,584 B | アトラス（512x512 RGBA） |
| handwritten.fnt | 14,632 B | グリフ定義 |
| handwritten_0_lod_a.tex | 1,048,584 B | アトラス（512x512 RGBA） |

## 4. .fnt ファイル形式

- サイズ: 14,632 B（全5フォント共通）
- ヘッダ 76 B:
  - `[0:4]` float fontSize（例: Kingthings Regular = 28.0）
  - `[12:76]` char[64] texName（例: `Kingthings_Regular_0_Lod_A`）
- グリフレコード: オフセット 344 から 56 B x 255 レコード（14 float）
  - `[0]` bearing_x（常に 0 扱い）
  - `[1]` advance（未使用、f[11] を advance として使用）
  - `[3]` u0 / `[4]` v0 / `[5]` u1 / `[8]` v1（テクスチャUV）
  - `[11]` width / `[12]` height（ピクセル寸法）
- レコードインデックス = ASCIIコード - 1（レコード32 → ASCII 33 '!'）
- スペース（ASCII 32）はレコードなし。advance = fontSize x 0.35 で合成

## 5. .tex アトラス形式（新規解明）

- **DDSではない**。ヘッダ 8 B + RGBA 生ピクセルデータ
- ヘッダ: `00 02 00 00 00 02 00 00`（tahoma_bold_small のみ `00 01 00 00 00 01 00 00`）
- データ: 512x512 RGBA = 1,048,576 B / 256x256 RGBA = 262,144 B
- テクスチャサイズは .fnt の UV 座標最大値から逆算可能（512 / 256）

## 6. Android版との一致検証

Android版のフォント実装は `app/src/main/cpp/ui/text_renderer.cpp` の `TextRenderer` クラス。

- `FontType` enum: Roboto / Daedric / KingthingsRegular / KingthingsShadowed / Handwritten / TahomaBoldSmall
- `.fnt` パーサー: オフセット 344、56 B x 255 レコード、14 float（オリジナル版にそのまま適用可能）
- アトラス: `assets/fonts/*.png`（RGBA8 テクスチャとしてアップロード）

### 一致検証結果

| フォント | .fnt 一致 | アトラス一致 | サイズ |
|----------|-----------|--------------|--------|
| Kingthings Regular | 完全一致 | 完全一致（512x512） | 28 px |
| Kingthings Shadowed | 完全一致 | 完全一致（512x512） | 35 px |
| Tahoma Bold Small | 完全一致 | 完全一致（256x256） | 22 px |
| Daedric | 完全一致 | 完全一致（512x512） | 55 px |
| Handwritten | 完全一致 | 完全一致（512x512） | 57 px |

- .fnt 5ファイル: バイト単位で完全一致
- アトラス5枚: ピクセル単位で完全一致（max diff = 0）

## 7. フォントの出自

| フォント | 出自 |
|----------|------|
| Daedric | ゲームオリジナル。Oblivion用に作られた架空の文字体系（デイドラ文字） |
| Kingthings Regular / Shadowed | フリーフォント「Kingthings」シリーズ（Kevin King氏デザイン）を採用 |
| Tahoma Bold Small | Microsoft製フォント「Tahoma」（Matthew Carterデザイン） |
| Handwritten | 手書き風フォント（既存デザインの採用） |

## 8. 成果物

- `tmp/oblivion_fonts/` - Misc.bsa から抽出した10ファイル + デコードPNG（`*_decoded.png`）
- 抽出パイプライン: BSA103 フォルダ名ブロックパース → ファイルレコード（hash/size/offset）→ データ領域から直接読み出し

## 9. 備考

- Misc.bsa の `archiveFlags=0x703` は圧縮なし。データ領域のファイルはそのままのバイト列
- Textures BSA（`Oblivion - Textures - Compressed.bsa`）は `[u32 展開後サイズ][zlib]` 形式で圧縮されている（前回レポート参照）