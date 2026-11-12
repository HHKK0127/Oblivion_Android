# v263_video_settings_labels.md — 依頼2: ビデオ設定 ID 9/10 の UI ラベル文言（serial 259 / V259・低）

- 作成: 2026-11-03 / 担当: TODO serial 259（Copilot 依頼バンドル `tmp/re/handoff_20261103_re_request_bundle.md` 依頼2・優先4）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam）＋実メニューデータ
- 判定: **確定**（ただし依頼の前提「名称ポインタ＝UI ラベル文字列」は**否定**。3 ポインタは INI キー文字列で、UI ラベル文言は strings.xml 側で別経路。以下両方を確定した）
- selftest: 本束はバイナリ文字列読取＋XML 実読中心（capstone 不要范围为中心）。走査規約（セクション個別 VA 換算）遵守

## §0 結論サマリ

1. `0xA6DD94` / `0xA6DDC8` / `0xA6DDE0` の指す文字列は **UI ラベルではなく INI キー（GMST 名形式）** である:
   - `0xA6DD94` = `fTreeLODDefault:LOD`
   - `0xA6DDC8` = `fActorLODDefault:LOD`
   - `0xA6DDE0` = `fActorLODMin:LOD`
2. これらは `.data 0xB14800` の**12 エントリ×8B テーブル**（`{u32 名前 ptr, float}`）の要素で、Tree/Actor/Item/Object の LOD × Default/Min/Max の組。[確定]
3. 設定メニューでの**正確なラベル文言**（英語・原文）は `menus\strings.xml` の trait 値で確定:
   - `_treelod` = **`Tree Fade:`** / `_actorlod` = **`Actor Fade:`** / `_itemlod` = **`Item Fade:`** / **`_objectlod` = `Object Fade:`**
   - `_distantlod` = **`Distant Land:`** / `_self_shadows` = **`Self Shadows:`** [確定]
4. **v258 件3 との接続（重要）**: v258 が「video settings ID 9/10 の反転トグル」と裁定したゲート `0xB09AE8`/`0xB09AF0` は、docs RE_EXE_REPORT L2615/L2620 の対応表どおりなら **ID 9 = `_distantlod`（Distant Land ボタン）・ID 10 = `_self_shadows`（Self Shadows ボタン）** であり、**Object Fade（スライダー）は ID 18/19**（L2612）。したがって「`_objectlod` 系ゲート」という表現は緩く、**移植側が ID 9/10 を "Object Fade" と命名すると誤り**。ID 9 のゲートが要求級ゲート（`0x483D60`）に接続する意味は「Distant Land 全体 OFF でオブジェクト LOD ロードも停止」と解釈するのが整合的。[確定]（対応表出典: docs L2604-2623 ＋ v258 §件3 の VA 群）

## §1 3 ポインタの実体（EXE 実測）

読み出し: `.rdata`（VA=off-0x627200+0xA28000）C 文字列として cp1252 実読。

| VA | 文字列（原文） | 備考 |
|---|---|---|
| 0xA6DD94 | `fTreeLODDefault:LOD` | `[LOD] fTreeLODDefault` |
| 0xA6DDA8 | `fTreeLODMin:LOD` | 隣接（同一テーブル） |
| 0xA6DDB8 | `fTreeLODMax:LOD` | 同上 |
| 0xA6DDC8 | `fActorLODDefault:LOD` | 依頼の2点目 |
| 0xA6DDE0 | `fActorLODMin:LOD` | 依頼の3点目 |
| 0xA6DDF4 | `fActorLODMax:LOD` | 隣接 |
| 0xA6DE08 | `fItemLODDefault:LOD` | |
| 0xA6DE1C | `fItemLODMin:LOD` | |
| 0xA6DE2C | `fItemLODMax:LOD` | |
| 0xA6DE3C | `fObjectLODDefault:LOD` | Object LOD の INI キー |
| 0xA6DE54 | `fObjectLODMin:LOD` | |
| 0xA6DE68 | `fObjectLODMax:LOD` | |

. 横断検索: .text の imm32 参照 **0 件**（セクション個別走査）。参照は .data のテーブルのみ ↓

## §2 テーブル .data 0xB14800（stride 8）

エントリ = `{u32 名前 ptr, u32/float 値}`。12 連続（0xB14800..0xB1485F）。後方 0xB14860 以降は fGamma/iActorShadow シリーズが同型で連なる（同一機構の別群）。

| .data VA | 名前 ptr → キー | 値（float 解釈） |
|---|---|---|
| 0xB14800 | fTreeLODDefault:LOD | 0.02 |
| 0xB14808 | fTreeLODMin:LOD | 2.0 |
| 0xB14810 | fTreeLODMax:LOD | 5.0 |
| 0xB14818 | fActorLODDefault:LOD | 2.0 |
| 0xB14820 | fActorLODMin:LOD | 15.0 |
| 0xB14828 | fActorLODMax:LOD | 2.0 |
| 0xB14830 | fItemLODDefault:LOD | 1.0 |
| 0xB14838 | fItemLODMin:LOD | 15.0 |
| 0xB14840 | fItemLODMax:LOD | 5.0 |
| 0xB14848 | fObjectLODDefault:LOD | 1.0 |
| 0xB14850 | fObjectLODMin:LOD | 15.0 |
| 0xB14858 | fObjectLODMax:LOD | 1.4 |

- [確定] 格納構造（.data 静的領域・名前 ptr 4B＋float 4B・12 項連続）
- [推定] Default/Min/Max 表記に対する値の対応（Actor の Min15/Max2 など不自然な組があり、「値」スロットが初期化後のランタイム更新対象の可能性）。消費コードの個別追跡は本束の依頼範囲外＝残点 R-263-1
- [確定] この 12 項に**文字列ラベルは含まれない**（すべて INI キー名の `.rdata` 文字列 ptr）

## §3 UI ラベルの実経路（video_menu.xml → strings.xml）

実データ（本環境）: `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\menus\options\video_menu.xml`

```
<text name="video_object_lod_label">
  <string> <copy src="strings()" trait="_objectlod"/> </string>
</text>
```

ラベル語義は `menus\strings.xml`（抽出済み・11,024B）の trait 値。原文引用:

| trait | 文言（原文・そのまま） |
|---|---|
| `_treelod` | `Tree Fade:` |
| `_actorlod` | `Actor Fade:` |
| `_itemlod` | `Item Fade:` |
| `_objectlod` | `Object Fade:` |
| `_distantlod` | `Distant Land:` |
| `_self_shadows` | `Self Shadows:` |
| `_viewdist` | `View Distance:` |
| `_grass` | `Grass Distance:` |
| `_distbuild` | `Distant Buildings:` |
| `_disttrees` | `Distant Trees:` |

- [確定] コントロール名も video_menu.xml に実在: `video_object_lod_slider` / `video_object_lod_slider_marker` / `video_LOD_button`（Distant Land 側）— docs L2612/L2615 の行表と一致
- [確定] 依頼文の推定（"Object LOD?"/"Tree LOD?"）はいずれも**不正確**。正しくは「Object Fade:」「Tree Fade:」（Fade が LOD 系スライダーの Bethesda 命名）

## §4 settings ID 対応（docs 突合）

docs `RE_EXE_REPORT.md` L2604-2623（既存確定）より LOD 群のみ抜粋:

| 行 | trait | コントロール | 種別 | id | マーカー id |
|---|---|---|---|---|---|
| 3 | `_treelod` | video_tree_lod_slider | slider | 12 | 13 |
| 4 | `_actorlod` | video_actor_lod_slider | slider | 14 | 15 |
| 5 | `_itemlod` | video_item_lod_slider | slider | 16 | 17 |
| 6 | **`_objectlod`** | video_object_lod_slider | slider | **18** | **19** |
| 9 | **`_distantlod`** | video_LOD_button | button | **9** | - |
| 14 | `_self_shadows` | video_self_shadows_button | button | **10** | - |

→ v258 件3 の「ID 9/10 の反転トグル＝B09AE8/B09AF0」は、上表により **Distant Land(9) と Self Shadows(10)**。**Object LOD スライダーは ID 18** で、B09AE8/AF0 とは別物。移植側の設定 UI 命名は本文 §3 の文言どおり（Object Fade / Distant Land）を使うこと。

## §5 証跡

- スクリプト（本セッション規約により `%TEMP%\opencode` 配置・依頼文は tmp/re 希望＝注記）: `%TEMP%\opencode\v263_probe1.py` / `v263_probe1_out.txt`（ポインタ窓走査）、in-band で参照走査・テーブル dump・XML 読取（出力本文に転記）
- 実データ: `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\menus\options\video_menu.xml`（69,619 字）・`menus\strings.xml`（11,024B）
- EXE: `.rdata` 文字列 band `0xA6DD94..0xA6DE68`、`.data` テーブル `0xB14800..0xB1485F`
- 文書とコードの不一致（AGENTS.md ルールにより指摘）: (a) 依頼文「名称ポインタ」＝実体は INI キー ptr（§1）。(b) v258 件3 の「`_objectlod` 系ゲート」呼称と settings ID 9/10 の docs 対応（§4）のズレ。

## §6 残点

- R-263-1（低）: §2 テーブル消費コード（INI 読みクランプ or スライダー min/max バインド）の個別経路。Actor の Min/Max 不整合値の解釈
- R-263-2（低）: 非英語ロケール版ラベル（Strings 翻訳 TAF）は本環境に抽出物なし（[未確定]）
