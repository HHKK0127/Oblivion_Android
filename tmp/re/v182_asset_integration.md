# v182_asset_integration.md — 束B: アセット形式統合（Q9–Q12 統合）

- **serial**: TODO 170 / V170・中（`v180_letter_rows_audit.md` 束Bの実行）
- **作成日**: 2026-10-29 / 担当: 成果物統合サブエージェント（分析専任）
- **編集範囲**: **本新規 1 ファイルのみ**。docs/・TODO・GT・台帳・plan・session-memory・既存 tmp/re 成果物は一切変更していない（読取のみ）。git 不使用。**ESM/ESP/DLC/BSA 本体へのアクセス 0 回（再スキャン禁止遵守）**。Ghidra 呼出なし。python スクリプト実行なし（既存出力の読取のみ）。
- **入力（読取専用）**: `tmp/re/copilot_archive/` の bsa_probe*.py / bsa_paths.py / bsa_entries.txt / textures_bsa_index.txt / dds_decode_probe.py / x_tel_census.py・x_tel_pairs.py・x_tel_cellkind.py / xtel_census.cpp / nif_oblivion_layout_spec.md / `tmp/re/` の s76_bsa_v104.*・s76_file_hash_*・s73_bsa_dds_tel.*・s69_nif.*・s69_wrld.md + s69_wrld_a6/a8/a15/a16・s77_ft_1..3・s77_oth_1_fasttravel.py・s77_oth.md / `docs/BSA_HASH_ALGORITHM.md`（読取のみ）/ `TODO_re49.md` L125–L128 / `v180_letter_rows_audit.md` 束B節 / `v181_esm_integration.md`（書式参考）。
- **方法**: 既存成果物の要約・数値突合・参照先明記による統合（新規走査なし）。**新規解析は「統合中に矛盾/欠落を発見した場合のみ」**（§5 に 3 件記載、いずれも再走査なしの整理）。
- **転記ポリシー**: `docs/RE_ASSET_REPORT.md`（未開設）への転記は要承認のため行わない。本ファイルは**ドラフト**であり、承認後に親セッションが docs へ転記する。

---

## §0 全体サマリ

| 節 | 対象 Q | 統合した元ファイル数 | 主要数値（実測） | 状態 |
|---|---|---:|---|---|
| §1 | Q9 BSA 全形式文書化 | 24 | 17 BSA 全 v103 / v104 拒否確定 / bias 17/17 100% / hash 147,629/147,629 | 統合済 |
| §2 | Q10 NIF 仕様全文書化 | 4（+元データ不在 1） | 20.0.0.4/user 2817/BSVER 11 / 17 アセット byte 検証 / EXE 登録 237 | 統合済（残点付き） |
| §3 | Q11 DDS デコード確認 | 3 | DXT1 1024×1024 mips=10 実デコード / BC1・BC3 アルゴリズム確定 | 統合済（BC2/BC4/BC5 未確認） |
| §4 | Q12 ドア/テレポート国勢調査 | 16 | DOOR 501 / door refs 6,949 / XTEL 4,397（len28・100%）/ 相互リンク 4,396 | 統合済 |

- 統合元合計 **47 ファイル**（重複計上含む。§各節の表を正とする）。
- 本束の統合先 `docs/RE_ASSET_REPORT.md` は**未開設**（v180 束B節 L158 と同じ）。

---

## §1 Q9: BSA 全形式文書化（v103/v104・圧縮/非圧縮）の統合

### 1.1 統合した元ファイル

| ファイル | サイズ | 役割 |
|---|---:|---|
| `tmp/re/copilot_archive/bsa_probe.py` / `bsa_probe2.py` / `bsa_probe3.py` / `bsa_probe5.py` | 2,901–3,121 B ×4 | Misc.bsa / Textures-Compressed.bsa のヘッダ・folder/file レコード逐步確定プローブ |
| `tmp/re/copilot_archive/bsa_paths.py` | 2,797 B | フォルダ chain bias 検証 + 実格納パス列挙（embedded-names レイアウト） |
| `tmp/re/copilot_archive/bsa_entries.txt` | 5,853 B | bsa_paths.py 出力: Misc.bsa 全 **115 エントリ**（menus/fonts/facegen、size/off 付き） |
| `tmp/re/copilot_archive/textures_bsa_index.txt` | 484,153 B | Textures-Compressed.bsa 全インデックス: **1,090 フォルダ / 18,040 ファイル / 19,130 行** |
| （s 系正本）`tmp/re/s76_bsa_v104.md` | 13,946 B | BSA v103 フォーマット全文 + v104 拒否確定 + EXE ローダ 0x42EE80 内部 |
| （s 系正本）`tmp/re/s76_bsa_v104.txt` | 2,708 B | 17 BSA 全数のヘッダ/bias 検証生表（Ver/Flags/Folders/Files/Bias/FnLen） |
| （s 系）`tmp/re/s76_bsa_v104.py` | — | 上記生成プローブ |
| （s 系土台）`tmp/re/s73_bsa_dds_tel.md` §1 + `s73_bsa_dds_tel.txt` | — | BSA 形式の第 1 次文書化（s76 の土台） |
| （hash 正本）`tmp/re/s76_file_hash_analysis.md` + `s76_file_hash_1..9.txt` | 11,478 B + 9 件 | BSA ハッシュ関数の完全復元と全数検証 |
| （docs 化済）`docs/BSA_HASH_ALGORITHM.md` | 6,610 B | hash は docs 側化済み（v180 束B注記と一致・読取のみ） |
| （関連）`tmp/re/copilot_archive/list_bsa_landscape.py` | 2,955 B | Textures BSA 内 landscape テクスチャ列挙 |

### 1.2 主要知見

**形式確定（v103 = Oblivion 実データの全形式）**
- **全 17 BSA が v103 (0x67)**。Steam / DISC / GOTY のいずれにも v104 BSA は **0 本**。同名 BSA は Steam・DISC でバイト一致（s76 §1.3）。
- **v104 は EXE が拒否**: `0x42EFA1: cmp [esi+0x158], 0x67; ja 0x42F481` → `[esi+0x194] |= 1`（invalid フラグ）→ 呼び出し元 `0x42F587` がチェックして登録スキップ → return NULL。**v104 パーサは EXE v1.2.0416 に存在しない**（s76 §1.2）。→ 「圧縮/非圧縮を含む全形式文書化」は **v103 の圧縮/非圧縮 2 系統で完結**する。
- ヘッダ 36B: `magic "BSA\0" / version / header_size(=36) / arch_flags / folder_count / file_count / folder_name_len / file_name_len / file_flags`。
- Folder record 16B（hash u64 / file_count u32 / folder_offset u32）、File record 16B（hash u64 / size_and_flags u32 / data_offset u32）。
- **size_and_flags**: `stored = & 0x3FFFFFFF`、`compressed = (>>30) & 3`、`arch_flags & 0x04` で**エントリ毎に反ントリ毎に反転**。圧縮データ = 先頭 u32（展開サイズ）+ zlib ストリーム。
- **bias 不変条件**: `folder_offset[0] − chain_start == file_name_len` が **17/17 = 100%** 検証（chain_start = header_size + folder_count×16）。実オフセット = `folder_offset[i] − file_name_len + chain_start`。
- **圧縮 BSA（arch_flags bit2）= 5 本**: Textures - Compressed (0x707) / Meshes (0x787) / DLCFrostcrag / DLCOrrery / DLCVileLair (0x707)。SI 4 本・Voices 系・その他 DLC は非圧縮 (0x703/0x713)。
- フォルダ名は 1B 長プレフィックス（長さは `\0` 含む）、ファイル名は null 区切りブロック。全パス = `folder_name + "\" + file_name`。
- センサス（s76 表）: Base 計 117,655 ファイル / SI 計 24,474。最大は Voices2 44,582・Textures 18,040・Meshes 20,182。
- **textures_bsa_index.txt は s76 census と完全一致**（1,090 フォルダ / 18,040 ファイル）— 独立 2 系統のクロスバリデーション。

**ハッシュ関数（docs 化済み・統合接続のみ）**
- `h = h*0x1003F + tolower(c)`（32-bit）、シグネチャバイト `[last, penult, len, first]`、拡張子 tweak テーブル `['', '.NIF', '.KF', '.DDS', '.WAV']` @`0xB25374`。EXE `0x6FA040`/`0x6FA080` から復元、**folders 1,621/1,621 + files 147,629/147,629 = 100% 検証**（s76_file_hash_analysis.md §1）。→ 正本は `docs/BSA_HASH_ALGORITHM.md`（v180 束Bの「hash は docs 側化済」と一致）。lookup は `0x42E85A`（先頭 `\` と `Data\` 除去）→ splitpath `0x6FA1B0` → u64 2-dword スキャン `0x42BC10`（同 §2）。
- v180 行 Q11 の「拡張テーブル 0xB25380: NIF/KF/DDS/WAV」は本 hash tweak テーブル（0xB25374 起点）と同一物。

**EXE ローダ（0x42EE80）**
- BSA マネージャ オブジェクト ~0x1B0 B（vtable 0xA35D74、header copy @+0x154、version @+0x158、テクスチャアーカイブ判定 bit3 @+0x194）。texture 系は別パス `0x42F359`（memory-map 方式）、通常系は `0x404EE0` memory-mapped I/O。単一呼び出し元 = `0x42F567`（archive manager init）。

### 1.3 統合中の口径差（新規解析なし・整理のみ）

- **Misc.bsa のサンプル値が 2 成果物で不一致**: `s73_bsa_dds_tel.txt` のサンプル（Steam 版と見）は `folders=8 / flags=0x00000000 / folderNameLen=221 / fileNameLen=1900 / filesize=7,049,011`、`s76_bsa_v104.txt` census は `folders=10 / flags=0x00000703 / 114 / 2084 / 7,229,551`。**fileCount=115 は両者一致**。→ 別インストールの同名 BSA（ディレクトリ構成差）と解されるが、**未突合のまま残す**（本体再読禁止のため）。
- s76_bsa_v104.txt 内に自己注記 `folder names: consumed=124 vs hdr=114 ... Re-evaluate` — folder_name_len の counting variant に関する未整理点。

### 1.4 残点と参照先

- 残点: TODO **R5**（v104 圧縮形式の細部）は「実 v104 ファイル不在 + EXE 拒否」が確定済みのため、**フォアグラウンドの調査対象外**（FO3 以降フォーマットとして別扱いが正当化される）。s76 Remaining: GOTY installer .cab 内 BSA の版検証 / zlib windowBits・level の実測 / `0x42F567` の multi-BSA 登録ポリシー全文書化 — いずれも本体アクセス前提。
- 参照先: `docs/BSA_HASH_ALGORITHM.md`（hash 正本）/ `tmp/re/s76_bsa_v104.md`（形式正本）/ 統合先 `docs/RE_ASSET_REPORT.md`（未開設・要承認）。

---

## §2 Q10: NIF 仕様の全文書化（20.0.0.4 / user 2817 / BSVER 11）の統合

### 2.1 統合した元ファイル

| ファイル | サイズ | 役割 |
|---|---:|---|
| `tmp/re/copilot_archive/nif_oblivion_layout_spec.md` | 13,375 B | **NIF ブロックレイアウト仕様の正本**（17 実アセット byte 検証済み・dumpfields.py end offsets 方式） |
| `tmp/re/s69_nif.md` | 10,813 B | EXE 側突合: NiObject 登録機構 0x712590 / 237 クラス / NiStream::Load ローダ特定 |
| `tmp/re/s69_nif_table.txt` | 22,586 B | 全 237 ブロックタイプ × EXE handler の完全表 |
| `tmp/re/s69_nif_evidence.txt` | 4,223 B | 登録クラスタ（VA 範囲別）証跡 |
| （元データ・**現在不在**）`%TEMP%\bsa_real_extract\`（real_assets / sample_assets） | 0 ファイル | 検証元の 17 実アセット + nif_reference.xml。**ディレクトリは残るが空**（§5 R-i） |

### 2.2 主要知見

**ファイルヘッダ（Oblivion 本命形式）**
- `20.0.0.4 / user 2817 / bsver 11`: HeaderString @0 → u32 version @39 → u8 endian @43 → u32 userVersion=2817 @44 → u32 numBlocks @48 → BSStreamHeader @52（bsVersion + author + process + export + maxFilepath）→ u16 numBlockTypes @96 → SizedString×N → u16 blockTypeIndices[numBlocks] → u32 numGroups → **block 0 @287（4B prefix なし）**。
- 旧版フォーム `10.1.0.106 / user 10 / bsver 5` も同一仕様書に収録（block 0 @756、**4B 0 prefix あり**。`_1stperson/skeleton.nif` のみ）。
- 版ゲート: `since <= 20.0.0.4 < until`。vercond false: #BS202# / #BS_GT_FO3# / #BS_GTE_FO3# / #DIVINITY2# / #BS_FO4# / #BS_SSE#。true: #NI_BS_LTE_FO3# / #BSVER# < 16 / <= 26 / #BSSTREAM#。
- `NiNode.Properties` は `#NI_BS_LTE_FO3#` = true で **PRESENT**（BSVER 11 < 16）— EXE 側でも確認済み（s69_nif §4.1）。

**byte 検証済みレイアウト（17 実アセット）**
- Geometry: `headhuman.nif` NiTriShapeData @737..60077 = 59,340 B / `middlechair01.nif` NiTriStripsData @668..3952 = 3,284 B（Strip Lengths[0]=181、Points=362 B）。
- Skin: `headhuman.nif` NiSkinData @60113..70000 = 9,887 B（**Num Bones=5** — 4-bone cap 説は誤り）/ NiSkinPartition @70000..106000 = 36,000 B（1 partition, Vertices 1275, Strips 1, StripLen 3960）。
- Animation: `stonewallgatedoor01.nif` NiControllerSequence @898..1068 = 170 B / NiControllerManager 43 B / NiTextKeyExtraData 32 B / NiTransformData 168 B / NiStringPalette 100 B。**ControlledBlock = 33 B**（Oblivion 版）。
- Collision: `middlechair01.nif` bhkNiTriStripsShape 64 B / bhkMoppBvTreeShape 473 B / bhkRigidBodyT 236 B（= 4+4+20+4+196+4+4）/ bhkCollisionObject 10 B。**collision は 17 中 15 ファイル**（bhkRigidBody 61 instance / bhkBlendCollisionObject 56）。
- Skin/Animation/Collision の feature→asset 対応表も仕様書末尾に確定（headhuman/horse = skin、stonewallgatedoor01/fireshield/shockshield = animation）。

**EXE 側突合（s69_nif）**
- 登録関数 `0x712590`（NiObject::RegisterClass、static initializer 経由）。call sites 241 中 **237 が有効登録**、5 クラスタ: NiObj-gfx ~80 / NiObj-anim ~70 / BS-gfx ~10 / PSys ~50 / bhk ~40。
- 仕様書の全 23 種は EXE 登録表に**存在**し、17 アセットの byte 検証と矛盾なし。
- NiStream::Load 関連文字列: "Not a NIF file" @0x712C43 / "Unknown NIF version" @0x712D05 / "invalid block type" @0x742C74（zlib 展開後のブロックタイプ照合失敗）。ブロックデータは zlib 圧縮（同梱 zlib 1.2.1）。
- **仕様書未カバーだが EXE 使用**: BSFadeNode（RTTI 0xA3FB04、NiNode 同一レイアウト + fade フラグ）/ BSXFlags（int 1 個、ファイルヒント）/ BSPackedAdditionalGeometryData（NiTriShapeData.Additional Data 参照先）/ bhkBlendCollisionObject。BSShaderProperty 系は文字列のみで登録なし = FO3 以降用の未使用コード。
- 17 アセット出現率: NiNode 80+ / NiTriShape(Data) 45 / NiSkinInstance 6 / NiControllerSequence 7 / bhkRigidBodyT 15 / BSFadeNode 6 / BSXFlags 12 等（s69_nif §5）。

### 2.3 残点と参照先

- 仕様書 Remaining open items: `prefix=4` は skeleton.nif のみ / census.py の `NiNode.Properties implausible array cnt=196608` は 4B prefix 未対応ツールの誤検出。
- s69_nif 残件 10 件: NiStream::Load 完全解析 / 版判定ロジック確定 / BSPackedAdditionalGeometryData・BSFadeNode・BSXFlags レイアウト / NiPSys 50 種 / bhk 40 種 等（§6）。
- **元データ不在**（§5 R-i）: `%TEMP%\bsa_real_extract` が空のため 17 アセットの再 byte 検証は不可。md 内の byte 記録が現存唯一の証跡。
- 参照先: `tmp/re/s69_nif_table.txt`（237 種全表）/ 統合先 `docs/RE_ASSET_REPORT.md`（未開設・要承認）。

---

## §3 Q11: DDS 形式（DXT1/3/5 等）のデコード確認の統合

### 3.1 統合した元ファイル

| ファイル | サイズ | 役割 |
|---|---:|---|
| `tmp/re/copilot_archive/dds_decode_probe.py` | 7,434 B | BSA から DDS 抽出 + dds_loader.cpp 相当の BC1/BC3 Python ポートによる独立デコード検証（デバイス非依存） |
| `tmp/re/s73_bsa_dds_tel.md` §2 | — | DDS ヘッダ 128B / BC1・BC3 ブロックデコード仕様 + EXE テクスチャロードパス |
| `tmp/re/s73_bsa_dds_tel.txt`（"DDS Decode Probe Results" 節） | — | **プローブ実行結果の保存版**（本タスクでの「dds_decode_probe.py 出力」は本保存版で代替。生ランログは残存せず） |
| （関連）`tmp/re/s76_bsa_v104.md` §S4（hash tweak テーブル） | — | 拡張子テーブル `['', '.NIF', '.KF', '.DDS', '.WAV']` @0xB25374（v180 行 Q11 の「0xB25380 テーブル」と同一） |

### 3.2 主要知見

**実デコード検証（dds_decode_probe.py 保存出力）**
- 対象: `Oblivion - Textures - Compressed.bsa` 内 `terrainhdgrass01su`。
- **DDS: 1024×1024 / mips=10 / fourCC='DXT1' / bitCount=0 / masks=(0,0,0,0)**。
- **デコード結果: mean RGB = (132, 97, 67)**（地形グラスらしい土色）— bit packing が dds_loader.cpp と一致することを出力側で確認。
- BSA 側の前提情報も併記: arch_flags=0x4（archive 圧縮）、`stored = size & 0x3FFFFFFF / compressed = (size>>30)&3; arch_flags&4 で反転` を使用。

**形式仕様（s73 §2）**
- DDS ヘッダ 128B: `"DDS "` magic / dwSize=124 / dwFlags / dwHeight / dwWidth / dwPitch / dwDepth / dwMipMapCount / pixel format @76（pf_flags + pf_fourcc @80 = "DXT1"/"DXT5" 等 + bitcount + masks 16B）/ payload @128（mipmap data）。
- **BC1 (DXT1)**: 8 B / 4×4 ブロック = color0 u16 (RGB565) + color1 u16 + 32-bit index（2 bit/texel）。**c0 > c1: 4-color（2/3, 1/3 補間）/ c0 ≤ c1: 3-color + 透明黒**。RGB565 展開 = `r5<<3|r5>>2, g6<<2|g6>>4, b5<<3|b5>>2`。
- **BC3 (DXT5)**: 16 B / ブロック = alpha0 u8 + alpha1 u8 + 48-bit alpha index（3 bit/texel）+ BC1 color ブロック（常時 4-color モード）。**a0 > a1: 8-step（step/7）/ a0 ≤ a1: 6-step + 0x00 + 0xFF（step/5）**。
- **EXE パス**: `0x760DA0`（DDS→IDirect3DTexture9 エントリ、大スタック 0x594）→ `0x748760`（decode helper）。**BC1/BC3 デコードは in-process**（D3DX DLL 経由ではなく、dds_loader.cpp の C++ ソース構造と Python ポートが一致）。

### 3.3 残点と参照先

- 未確認: **DXT2/DXT3 (BC2) / DXT4 / BC4/BC5 の扱い**（s73 Outstanding #2 — likely D3DX passthrough）。本命テクスチャの大半は DXT1/DXT5 で賄えるが、拡張フォーマットの可否は移植時に要確認。
- 未確認: 実行時に解決される D3DX 関数の正確な名前（D3DX9 DLL import vs manual decode の最終裁定 — s73 Outstanding #6）。
- 参照先: `tmp/re/s73_bsa_dds_tel.md` §2 / 統合先 `docs/RE_ASSET_REPORT.md`（未開設・要承認）。

---

## §4 Q12: ドア/テレポート国勢調査（x_tel_census.py + exe）の統合

### 4.1 統合した元ファイル

| ファイル | サイズ | 役割 |
|---|---:|---|
| `tmp/re/copilot_archive/x_tel_census.py` | 6,995 B | **ESM 全走査 DOOR/XTEL 国勢調査ツール**（Oblivion3.esm 直読: door ref 数 / XTEL 有無 / len 分布 / dest 解決先の cell 種別） |
| `tmp/re/copilot_archive/xtel_census.cpp` | 15,363 B | **同調査のスタンドアロン C++ 版（TODO 記載の "+exe"）**。esm_reader.cpp 非リンク（repo 無改変）、ESMFile::open の GRUP-stack attribution をミラー、REFR バッファ → 全ファイル走査後に doorSet/cellSet で分類（レコード順非依存） |
| `tmp/re/copilot_archive/x_tel_pairs.py` | 5,408 B | テレポート pair の cell→cell 遷移マトリクス生成 |
| `tmp/re/copilot_archive/x_tel_cellkind.py` | 3,378 B | cell 分類（XCLC 有無 × WRLD 親の 3 種） |
| `tmp/re/s73_bsa_dds_tel.md` §3–§4 + `s73_bsa_dds_tel.txt`（"XTEL Census" 節） | — | **x_tel_census.py 出力の保存版**（census 全数値 + サンプル hex + EXE call sites）。生ランログは残存せず |
| `tmp/re/s69_wrld.md` §3–§5 | — | XTEL/CELL データモデルと EXE 側テレポート系の総合文書 |
| `tmp/re/s69_wrld_a6_teleport_entry.txt` / `a8_teleport_extended.txt` | 2,762 / 8,312 B | 0x66EAF0（teleport driver）本体・拡張 disasm 証跡 |
| `tmp/re/s69_wrld_a15_cell_sub_xtel.txt` / `a16_refr_sub_xtel.txt` | 5,313 / 8,612 B | CELL/REFR サブレコード経路（0x4CD600 の X-tag 分岐比較ツリー / REFR 側 XTEL 書込み @0x4D9E00 系） |
| `tmp/re/s77_ft_1_teleport_entry.txt` / `s77_ft_2_check_fasttravel.txt` / `s77_ft_3_postcheck.txt` | 16,261 / 4,907 / 4,097 B | 0x66EAF0 全体 / 0x6AB160（FastTravel check）/ 0x66EBF2 呼び出し後経路の disasm |
| `tmp/re/s77_oth_1_fasttravel.py` | 1,717 B | 上記生成 + fFastTravel* GMST 参照調査 |
| `tmp/re/s77_oth.md` §1 | — | FastTravel フロー確定版（WorldMap→queue→LoadingMenu） |

### 4.2 主要知見

**XTEL 国勢調査（x_tel_census.py 出力・保存版 = s73_bsa_dds_tel.txt、対象 Oblivion3.esm）**
- 走査規模: **Records 1,167,016 / Cells 35,494（外装 33,560 / 内装 1,934）/ Refs 1,029,280（ACHR+REFR+ACRE）**。
- **DOOR ベース 501 / door refs 計 6,949 / XTEL 付き 4,397（len=28 が 100%）/ なし 2,552**（内装 2,237 / 外装 315）。
- **XTEL dest 型: 4,397 × REFR = 100%**（未解決 0）。dest top byte は **0x00 のみ**。
- **相互リンク 4,396 / 4,397（99.98%）— 片方向は 1 件（0.02%）**。
- door ref の cell 分布: 内装 3,781（XTEL 付き）/ 外装 616（XTEL 付き）。dest cell 分解能: 内装 3,781 / 外装 616。
- **XTEL 28B レイアウト（サンプル hex 2 件で byte 確認）**: `[0..3]` destRefrFormID u32 / `[4..15]` 位置 f32×3 / `[16..19]` flags u32（実測 0）/ `[20..23]` flags2 u32（**0x80000000 or 0x00000000**）/ `[24..27]` heading f32（z-rotation）。
- cell 分類（x_tel_cellkind）: 内装+WRLD なし 1,855 / 内装+WRLD 内 79 / 外装 33,560（合計 35,494 ✓）。
- 遷移マトリクス（x_tel_pairs、s73 §3.6）: int→int 2,361 / int→int(WRLD) 407 / int(WRLD)→int(WRLD) 26 / int→ext 556 / int(WRLD)→ext 24 / ext→ext 36（対称行列）。

**EXE 側（s69_wrld §4 + s77_oth §1）**
- **teleport driver `0x66EAF0`（call sites 15 件）**: 0x40DE3A（main loop activation）/ 0x446279（UI door）/ 0x462E5F・0x463061（player interaction）/ 0x4B7CD5（script command）/ 0x508D8E・0x508F11（AI package）/ 0x51271D（AI pathing）/ 0x66F413・0x66FEB0・0x66FFED・0x670664・0x670B2D・0x670F26・0x67108F（helpers）。
- フロー: `0x4D6670` 現在 cell → `0x4C9CF0` DOOR ベース → `0x4835D0` activation flag [0xB34420] → `0x4D89A0` cell 到達可否 → vtable[0x380] worldspace 取得（global [0xB33398] 経由）→ `0x4D8AA0` dest cell 解決 → `0x4CAD00` dest 位置 → **`0x6AB160` FastTravel 判定** → `0x4455E0` cell load キュー（SceneGraph [0xB333A0]）→ `0x560110` flush → `0x442630` cell activation。分岐: dest 未ロード → キュー / dest 内装 → XTEL 位置採用 / worldspace 有効 → FT check。
- **X サブレコードハンドラ `0x4259E0`**（call sites: 0x4CD928 cell dispatch / 0x4DA01C deferred）のタグ表で **XTEL → `0x4D40D0`（28B を cell subrecord storage [ecx+0x28] へコピー）**、XCLC → `0x4D4B20`。
- **FastTravel check `0x6AB160`**（s77 確定）: `[0xB16180]==0` → return 0。`[edi+0x1E4]` を文字列照合（0xA77140 / 0xA77138）→ `[edi+0xB0] vs 8`。pass 経路 = `0x6A8D50` + `0x6AB420`（時間経過処理）、フラグ [esp+0x12]=1 で FT path。**FT フロー**: WorldMap → dest 選択 → 0x66EAF0 → 0x66EBF2 で 0x6AB160 → queue → `0x65F770`（1h pass）→ LoadingMenu `0x578E90` → 到着。
- データフロー図（s69_wrld §5）: ESM GRUP type6（CELL children）→ CELL 0x58B → XTEL → REFR → dest、EXE 側は CELL vtable[7] (0x4CD600) → 0x4259E0 で収束。

### 4.3 統合中の口径差（新規解析なし・整理のみ）

- **Records 数**: x_tel_census 出力の 1,167,016 は v181 §1 の GT **1,167,017 と 1 件差**（TES4 ヘッダレコード等の数え方差と推定。Refs 1,029,280 は GT の REFR 1,025,617 + ACHR 2,190 + ACRE 1,473 と**完全一致**）。main レポート転記時は GT（1,167,017）を正とし本値に注記すること。

### 4.4 残点と参照先

- 未解明（既存 Outstanding のまま）: **XTEL[20] = 0x80000000 の意味**（load-state flag? teleport-type flag?）/ `0x4D40D0` の完全 disasm / FT 到着最終ロジック（0x6A8D50 + 0x6AB420 + 0x65F770 の time→load→arrive 全連鎖、s77 §7-1）。
- 参照先: `tmp/re/s73_bsa_dds_tel.md` §3–§4（XTEL 形式 + EXE フロー正本）/ `tmp/re/s69_wrld.md` §S4 / `tmp/re/s77_oth.md` §S1 / 統合先 `docs/RE_ASSET_REPORT.md`（未開設・要承認）。

---

## §5 統合サマリ・新規残点・遵守事項

### 5.1 統合サマリ

| 節 | 統合元ファイル数 | 主要成果 |
|---|---:|---|
| §1 Q9 BSA | 24（probe 5 + 出力 2 + textures index 1 + s76 3 + hash 10 + docs 1 + s73 2） | 17 BSA 全 v103・v104 拒否確定 / bias 不変 100% / 圧縮 5 本の特定 / hash 147,629/147,629（docs 化済みと接続） |
| §2 Q10 NIF | 4 | 20.0.0.4/user 2817/BSVER 11 ヘッダ + 全ブロックレイアウト（17 アセット byte 検証）+ EXE 登録 237 種突合 |
| §3 Q11 DDS | 3 | DXT1 1024×1024 mips=10 の実デコード成功 / BC1・BC3 アルゴリズム確定 / EXE in-process デコード経路 |
| §4 Q12 国勢調査 | 16 | DOOR 501 / door refs 6,949 / XTEL 4,397（len28 100%・相互リンク 4,396）/ XTEL 28B レイアウト byte 確認 / 0x66EAF0・0x6AB160 フロー確定 |

### 5.2 新規残点（本統合で発見）

| # | 内容 | 優先 | 行先 |
|---|---|---|---|
| R-i | `%TEMP%\bsa_real_extract`（nif_reference.xml + 17 実アセット）が**空ディレクトリ**（0 ファイル）。Q10 の byte 検証再現・追加検証ができない。md 内記録が唯一の証跡 | 中 | BSA 抽出の再実行要否を親セッションで判断（本体 BSA アクセス前提） |
| R-ii | Misc.bsa のヘッダが s73 サンプル（folders 8 / flags 0x0）と s76 census（folders 10 / flags 0x703）で**不一致**（fileCount 115 は一致）。別インストールの同名 BSA と推定だが未突合 | 低 | 次回 BSA アクセス時の突合（本タスクでは再読禁止で保留） |
| R-iii | x_tel_census の Records=1,167,016 vs Q1 GT 1,167,017 の **1 件差**（数え方差と推定）。docs 転記時に GT 正・本値注記が必要 | 低 | docs 転記時（§4.3 に記載済み） |
| R-iv | DDS の **DXT2/3 (BC2)・DXT4・BC4/BC5 未確認**（s73 Outstanding #2 のまま）。移植で不要ならクローズ可能 | 低 | 移植要件決定後 |
| R-v | Q9 の s76 Remaining（GOTY .cab 内 BSA 版検証 / zlib windowBits・level 実測 / 0x42F567 multi-BSA 登録ポリシー）、Q12 の XTEL[20] 0x80000000 意味・0x4D40D0 完全 disasm・FT 到着ロジック完全化 — いずれも本体/深掘り前提で既存のまま | 低 | 既存タスク（TODO R5 は v104 拒否確定により対象外化可） |
| R-vi | `docs/RE_ASSET_REPORT.md`（統合先・未開設）への転記は**要承認バッチ**（本ドラフトでは未実施） | 中 | 親セッション |

### 5.3 遵守事項（自己検証）

- git 不使用 / docs・TODO_re49・GT・session-memory・plan・台帳の**編集 0 件** / 既存 tmp/re 上書き 0 件（本ファイル v182 のみ新規）。
- **ESM・ESP・DLC・BSA 本体への読み取り 0 回**（再スキャン禁止遵守。入力は copilot_archive / tmp/re / docs の既存成果物のみ）。
- Ghidra 呼出 0 回 / python スクリプト実行なし（既存出力の読取のみ）。
- selftest 4/4 は本タスクのスコープ外（本束は統合専任のため未着手）。
- 中斷対策: §0+§1 → §2 → §3 → §4 → §5 の順で**節ごとに即書込み**済み。

（了 — v182_asset_integration.md / 束B: Q9–Q12 統合。docs 転記は要承認のためドラフト留め）
