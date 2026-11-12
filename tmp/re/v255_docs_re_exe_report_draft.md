# RE_EXE_REPORT.md 追記ドラフト（V255 / オブジェクト LOD）
# 適用対象セクション案: 「54.8 DistantLOD（オブジェクト LOD .lod/.cmp）」として新設
# 本ファイルはドラフトのみ。docs/RE_EXE_REPORT.md 本体は未編集（人間承認待ち）。

## 54.8 DistantLOD — オブジェクト LOD（.lod / .cmp）

解析対象: Oblivion.exe v1.2.0416 (Steam) / 実素材 `distantlod` コーパス 921 件 + 1 件。
詳細証拠・全数走査: `tmp/re/v255_object_lod_structure.md`（関数表・命令 VA 全記録）、
`tmp/re/v255_lod_records.csv`（レコード 3,202 行台帳）、`tmp/re/v255_field_census.txt`。

### ファイル形式（.lod）— [確定]

- パーサ = `FUN_004D16D0`（VA 0x4D16D0、唯一の .lod ファイル読取り関数。
  呼出し鎖: `0x483D60`→`0x4BD430`(DistantLODLoaderTask ctor)→`0x4BCC60`(Task::Run)→`0x4F1990`→`0x4D16D0`）。
- 構成（リーダブル/リトルエンディアン、seek なしの逐次読取り `0x4D1734..0x4D1912`）:

  | 構造 | フィールド | 証拠 |
  |---|---|---|
  | ヘッダ | `u32 count`（読取り件数上限） | `0x4D1754` 4B 読取・`0x4D177A` ループ境界 |
  | レコード (count 個、**可変長 = 8+28n**) | `u32 formId`（TESObjectSTAT 限定） | `0x4D179B`、`0x4D1800 cmp [form+4],0x1E` |
  |  | `u32 n`（インスタンス数、実測 1..11） | `0x4D17EE`、配列 resize `0x4D1860..0x4D188F` |
  |  | `float3 pos[n]`（ワールド絶対座標） | `0x4D18B9` read 12n → entry+0x14 |
  |  | `float3 rot[n]`（非ゼロは z のみ実測・ラジアン推定） | `0x4D18E4` read 12n → entry+0x04 |
  |  | `float scale[n]`（50.2..136.0 実測） | `0x4D190C` read 4n → entry+0x24 |

- 1 ファイル = **1 セル**（`DistantLOD\<worldspace>_<cellX>_<cellY>.lod`、str 0xA45B58 /
  タスク名 0xA45A98）。ブロック(32×32)単位の入れ子は存在しない [確定]。
- ジオメトリ（頂点/インデックス/NIF 断片）は含まない。fourcc 0/921 件。
  参照は FormID のみで、NIF は TESObjectSTAT 側のモデル（ESM の MODL 経路）[確定]。
- コーパス整合: 89% は count で EOF に到達。11%（101 件）は count 後もレコード形式の末尾バイト列が残る
  （エンジン読取範囲外・`0x4D177A` の上限命令で確定）。移植実装は EOF 歩行採用を推奨（§54.8.3）。

### .cmp（ワールドスペース索引）— [確定]

- ローダ = `FUN_004F28B0`（`Data\DistantLOD\%s.cmp` str 0xA483EC・site 0x4F2904）。
  圧縮なし。**u32 配列** = `(uint16 cellX | int16 cellY<<16)` キー集合（size/4−1 個）
  → NiTMap `mgr+0xC8`（find `0x4D6760`/insert `0x452570`）+ **末尾 u32 = kind マスク** → `mgr+0xDC`
  （消費者 `0x4EF2D0`: `return (mgr+0xDC & kind) != 0`）。seworld.cmp の末尾値=7（kind 1/2/4 全有効）。
- 用途: `DistantLODLoaderTask` ctor の **mode==2 ゲート**（`0x4BD4C7 cmp 2`→`0x4BD4D5 call 0x4F0DA0`、
  不在ならタスク生成中止）。`.cmp` 未ロード（`mgr+0xD8==0`）時は常に許可 [確定]。
- **worldspace ごとに 1 ファイル**（base BSA=55 本 = worldspace 数、SI=1 本）[確定]。
  key 集合は .lod ファイル名集合の部分集合だが「存在索引」ではない（seworld 41 ≪ 921）— 実体意味 [未確定]。

### INI / グローバル — [確定]

- `DistantLODLoaderTask` の生成条件: `byte [0xB09DB0] != 0`（.data 既定値 **1**、サイト `0x4BD553`/`0x4F19A4`）。
  有効時にのみファイル open（`0x4BD58A [0xB33A04]→vft+4`）と `.lod` パース（`0x4F19BF call 0x4D16D0`）。
  無効時 `0x4F1990` は ESM 内 **DistantRefLOD**（record type 0x30/0x31・subrecord NAME/DATA/XLOD/XSLC を
  `0x4D43F0`/`0x4D8440` が列挙）経路へ分岐 — .lod ファイルとは別系統 [確定]。
- kind 有効化: `0x4EF2D0`（上記 .cmp マスク）・`0xB09AE8/0xB09AF0`（既定 1、書手 `FUN_005DF160@0x5DFF36`）。
- `iDistantLODGroupWidth:DistantLOD`（str 0xA8D308・INI レコード .data 0xB2C360・既定 8=0xB2C35C）と
  `fFadeDistance:DistantLOD`（str 0xA3D888・.data 0xB06AB4）は LOD パーサ群から imm32 参照ゼロ（適用点は別系統 [未確定]）。

### 供給経路 — [確定]

- BSA v103 索引実走査: `Oblivion - Meshes.bsa`(flags 0x787) に `distantlod/` **.lod 9,889 + .cmp 55**、
  `DLCShiveringIsles - Meshes.bsa`(0x703) に **.lod 921（seworld 920 + tamriel 1）+ .cmp 1**。他 BSA に該当ゼロ。
  loose 配置不要（本環境 Data 直下に DistantLOD ディレクトリなし）。
- **`meshes\distantlod\` という経路は実在しない**（folder 名は `distantlod`。依頼文の前提は誤り）。
- エンジン論理パス: `.lod` = `DistantLOD\%s_%i_%i.lod`（0xA45B58、アーカイブマネージャ経由 open `0x4BD58A`）、
  `.cmp` = `Data\DistantLOD\%s.cmp`（0xA483EC、`0x431690` 経由 open）。

### 移植実装仕様（要約）— 詳細は `tmp/re/v255_object_lod_structure.md` §8

1. 読み込み単位: 1 セル 1 ファイル全体（40..1072B）。遅延/ブロック分割不要。
2. レコード走査: `count` はエンジン境界だが 11% で末尾に追加レコードがあるため、移植は
   **EOF 歩行＋（8+28n 整合・formId 妥当性）検証で全件採用**を推奨。
3. メモリ表現: `ObjectLodEntry{formId, vector<inst{pos,zRot,scale}>}`。既存 `ActiveBlock`/ランドスケープ LOD の
   仕組みとは別体系（`OriginalLodProvider` 系はパス解決・失敗抑止のみ流用可）。
4. 描画: formId → TESObjectSTAT の通常 NIF（移植側 ESM リーダで解決）をインスタンス変換で複製。
   .lod 内にジオメトリなし。
5. フォールバック: ファイル/フォーム解決失敗はレコード（またはセル）破棄で継続。エンジンも
   open 失敗時にタスク生成を中止するだけでクラッシュ経路なし（`0x4BD59B`）。
