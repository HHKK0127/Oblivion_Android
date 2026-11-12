# v320_ess_dynamic_phaseA.md — 動的照合 Phase A: 実 .ess サンプルのファイル側解析（L1・日本語報告）

- 作成: 2026-11-07（ai-todo #86 / 動的照合 Phase A・サブエージェント lane）
- 正式項目: `tmp/re/SAVE_LOAD_RE_FOLLOWUP_v313.md` §1 優先解析項目 1「実 `.ess` サンプル有無確認」に付随する**動的照合5項目**（①ヘッダ size↔実ファイル総長照合 ②offset38 以降のフィールド境界照合 ③圧縮データの物理位置特定 ④change form / global / quest / cell / AI 領域の境界推定 ⑤`FUN_0045D5F0` の name/thumb 読取順と実データ照合）＋ 委任分 ai-todo #85（段階2 file 部）・#84（file 部）。
- 判定: **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**。旧結論（v307/v309/v312/v313/v315/v317/v316）は削除せず、矛盾は両論併記＋裁定理由を §6 に記録。仕様と実データの差は**仕様側を直さず「実測差」として記録**。
- **本 lane の作業範囲 = ファイル＋既存 artifact 読取のみ。EXE プローブ（動的実行・静的 disasm）は禁止（Phase B＝v319 完了後の別 lane）** → 本成果物に VA 由来の新規検証はなく、VA/命令は既存 artifact の引用。
- クリーンルーム: OpenTES4 / xOBSE / COEF 等の逆コンパイル由来コード・.ess 構造知識の持ち込みなし。仕様テンプレートは**本 repo の artifact のみ**（v283 / v303 / v307 / v309 / v312 / v313 / v315 / v316 / v317）。

---

## 1. 解析対象（サンプル情報・読み取り専用宣言）

### 1.1 サンプル（初の実 .ess データ）

| 項目 | 値 |
|---|---|
| 原本 | `C:\Users\hiroki.kogarumai\OneDrive - 関東航空計器株式会社\ドキュメント\My Games\Oblivion\Saves\Save 1 - None - Imperial Prison, Level 1, Playing Time 00.01.11.ess` |
| サイズ | **836,742 B**（0xCC486） |
| MD5 | **FC32EFAEB90D26141D3DC4A325AC10E7** |
| mtime | **2026-11-07 14:20:22**（JST・ローカル表示） |
| ファイル名書式 | `'Save N - name1 - name2, Level L, Playing Time HH.MM.SS'`（v316 確定書式）と**動的一致**（N=1 / name1='None' / name2='Imperial Prison' / Level 1 / 00.01.11） |
| 解析対象 | **%TEMP%\opencode\v320_sample.ess**（`Copy-Item` によるコピー・MD5 一致確認済） |

### 1.2 読み取り専用・禁止事項の遵守宣言

- **原本は一切書換しない**（作業は %TEMP% のコピーに対してのみ。`Copy-Item` は読取のみ）。
- ESM 未接触・git 未実行・`tmp/re/re_util.py` 変更なし・`tmp/re/ess_sample_inspect.py` 読取のみ（再実行せず、観測事実は指示書 §観測済み事実を踏襲。dow 異常のみソース読取で裁定、§6-1）。
- **成果物は本ファイルのみ新規作成**。既存 tmp/re 成果物・台帳・STATE・TODO・docs・GT・ソース（app/**）・.github/** は読み取りのみ。
- 中間物: `%TEMP%\opencode\v320_*`（.py 12 本 + .out 12 本 + v320_sample.ess コピー）のみ新規。

---

## 2. probe 一覧

実行形: `chcp 65001` → `python $env:TEMP\opencode\v320_<n>.py *> $env:TEMP\opencode\v320_<n>.out`。全本 EXIT=0。

| script | 対応項 | 内容 / 主結果 |
|---|---|---|
| v320_01 | 項1 | 固定ヘッダ実バイト・F1..F8 チェーン走査・size 式検算・file_size−X の全 u32 探索（**0 件**）・payload 先頭 hexdump・代表オフセット u32 |
| v320_02 | 項2/5 | 2 SYSTEMTIME 帯の raw words と正順解釈・dow 検証・mtime 照合・ファイル名↔ヘッダ全フィールド照合・境界マップ総括 |
| v320_03 | 項3 | リージョン別 78xx カウント・有効 zlib ヘッダ **44 候補全 decompress 試行（成功 0）**・raw deflate **4,273 オフセット試行（成功 0）**・64KB エントロピー・代表オフセット hexdump |
| v320_03b | 項3 | 全域 512B エントロピー走査（1,635 ブロック）: **≥7.0 = 0 件**・payload 内 ≥6.0 連続区間 = 0 |
| v320_04a | 項4 探査 | マスタ列解釈（count 有無の 2 候補比較）・payload 内 ASCII 地図・末尾 dump |
| v320_04b | 項1/4 | マスタ後 u32（822,982 / 5,527）の候補検算・末尾ペア走査（初版検証） |
| v320_04c | 項4 | frame パターン分布（6,576 ヒット）＋ [12B][tail] テンプレ V1/V2/V3 連鎖走査 → **V3 が 5,516 records 連鎖** |
| v320_04d | 項4 | 連鎖起点の後方拡張 → **起点 150,364 / 5,527 records / end 822,976（count 欄 5,527 と完全一致）**・formType/+9/flags 統計・index 表 3,437 読取（footer 到達一致） |
| v320_04e | 項4 | index エントリ指し先 delta 解析（3 基準いずれも一様分散）・record-start u32 全ファイル探索（dir のみ）・マスタ後セクション hexdump |
| v320_04f | 項4/#85 | 5,527 件の tail payload 内 frame 候補位置統計（digit 出現率・u16==tailSize−k）・代表 9 件 head・bit23 8 件・flags ビット分布 |
| v320_04g | #85 | bit3 経路×formType クロス集計（flags==8 単独 187 件生存・bit11=0・rewrite 候補 5 件） |
| v320_05 | 項5/#84 | filename↔ヘッダ照合・thumb 構造検算（8+w*h*3）・pixel 行サンプル・JPEG SOI 探索・size 式検算・時刻総括 |

（`ess_sample_inspect.py` の既存観測は重複実行せず、dow 表記のみソース読取で裁定 §6-1。）

---

## 3. 判定表（契約書 §1 の5項 ＋ 委任分）

### 3.1 主判定表

| §1 項 | 項目 | 判定 | 一句话裁定 |
|---|---|---|---|
| **1** | ヘッダ size ↔ 実ファイル総長照合・u32@38 裁定 | **[確定]** | **size field = u32@34（147,521）**で `38 + size = 147,559 = ヘッダ+thumb ブロック終端` と完全一致（v307 の書出式 `0x24+len+len+thumb` を実測確定）。ただし **`38+size` はファイル総長（836,742）とは不一致**（差 689,183 = ペイロード）→「size↔総長」は**総長一致の u32 がファイル内に 1 件も存在しない [確定負]**。**u32@38 = counter（=1 = ファイル名 'Save 1'）で count ではない [確定]** |
| **2** | offset38 以降のフィールド境界照合・dow 異常・2 時刻帯 | **[確定]** | F1..F8 の全境界を実バイトで特定（§4 表）。**dow=0x13E 異常は観測スクリプトの field 順誤り**（wDayOfWeek は [18..19]=**6=土曜=2026-11-07 と完全整合**、[28..29]=318 は wMilliseconds）。**mtime 不整合も同一誤りで解消**（真値 05:20:22 UTC = 14:20:22 JST = mtime 秒一致）。**2 つ目の時刻帯 = F7 = GetLocalTime**（@75..90、band1 と +9h・ms 同値318） |
| **3** | 圧縮データの物理位置特定 | **[確定]** | **本サンプルは無圧縮（選択肢①を採用）**。有効 zlib ヘッダ 44 候補は**全 decompress 失敗**・raw deflate 4,273 試行**成功 0**・512B エントロピー ≥7.0 が**全域 0 件**（payload 実測 3.4〜4.2）・文字列は平文（'Oblivion.esm' 等）。②raw deflate ③カスタム/分割チャンク ④場所限定圧縮は **[確定負]**。78 9C=0 件は「zlib ストリームが存在しない」帰結 [確定]、78 01×42 / 78 DA×1(@493552) は payload 内**偶発 2byte 列** [確定] |
| **4** | change form / global / quest / cell / AI 領域の境界推定 | **[確定]**（境界・連鎖） | **[12B entry][tailSize B payload] テンプレで 150,364..822,976 を 5,527 records 連続歩行、崩れゼロ・count 欄（u32@147,757=5,527）と完全一致**。quest/cell/AI は独立セクションを持たず**全てチェーン内で formType 区分**（0x3B quest=274 / 0x30 cell=186 / 0x31..33 created=3,387 / 0x23 actorbase=1,680、**0x3D 等の AI 系レコードは 0 件 [確定負(本サンプル)]**）。global 候補2表 [推定]（§4-2）。v283 frame `<u16 len><char '0'..'3'><typeID>` は **payload 先頭に一様出現せず＝実測差**（§6-4） |
| **5** | name/thumb 実データ照合（FUN_45D5F0 と実データ） | **[確定]** | filename↔ヘッダ name 帯・Level・PlayTime（秒切捨）**全一致**。thumb: **F8=147,464 = 8+256×192×3**（u32 w=256 @95 / u32 h=192 @99 / pixels @103..147,559 / stride=768 / JPEG SOI なし）→ **v313 の thumb tail [部分確定] を実データで [確定] に昇格**。ただし v313 推定 256×256 は **256×192 = 実測差**。name1='None' は v316 の %s#1=TESFullName と実データ一致 [推定]（名前の由来クラスは EXE 側既確定を引用） |

### 3.2 委任分

| 諸番 | 項目 | 判定 | 一句话裁定 |
|---|---|---|---|
| **#85 段階2 file 部** | tail Δ=0x1C/0x2C 実測・+9 byte 実値 | **[確定]（+9・0x2C 該当なし）＋ [未確定]（Δ 内容）** | **+9 byte = 0x7D（version）が 5,527/5,527 全件** [確定]。**flags bit31 立ち = 0 件 → Δ=0x2C の実 entry は本サンプルに存在しない [確定負(該当なし)]**。pass2 再発行（Δ=0x1C 削除）は **bit11=0・rewrite 候補 5 件も tail==0 ゲートで未発火 → Δ 切り抜きは本ファイルでは観測不能 [未確定]**（再保存サンプル or Phase B EXE 突合へ） |
| **#84 file 部** | seek 0xD000 / 0x4000・0x2000 実レイアウト・name2 語彙・thumb 実位置 | **[確定]（レイアウト・thumb）＋ [未確定]（name2 語彙）** | **0xD000=53,248 / 0x4000=16,384 / 0x2000=8,192（および 0x11000・0x13000）はすべて thumb ピクセル帯（95..147,559）の内部で、53,248 位置に構造切替はなし [確定(本サンプル)]** → PC .ess は CON 経路の seek 基準を持たず v307「'CON ' 一致時のみ seek」既確定と整合。thumb 実位置 = size フィールド @91 / w,h @95,@99 / pixels @103 [確定]。name2 語彙 = 本サンプル 1 件 'Imperial Prison' のみで **語彙の一般化は不可 [未確定]** |

### 3.3 5 段階集計（主判定 7 件: 5 項＋委任 2）

| 判定 | 件数 | 項番 |
|---|---|---|
| **[確定]** | **7** | 項1, 項2, 項3, 項4（境界・連鎖）, 項5, #85（+9 と 0x2C 該当なし）, #84（レイアウト・thumb） |
| [部分確定] | 0（主判定） | —（副次: 項4 の dir/index/trailer 構造は [部分確定]、§6 に明記） |
| [推定] | 0（主判定） | —（副次: global 候補2表の意味・位置 float・2 SYSTEMTIME の用途差・name1=TESFullName・サイズ 0x24 内訳の用途付記） |
| [未確定] | 0（主判定） | —（副次: Δ0x1C 内容・index 表の意味・trailer・中間セクション・F5 float・name2 語彙・RGB/BGR・再保存時の時刻更新 = §7） |
| [確定負] | 0（主判定） | —（副次: ファイル総長一致 u32 なし・圧縮（②③④）・bit31 entry・AI 系 formType・flags==0 レコード・tailSize==0 レコード） |

---

## 4. オフセット表

### 4-1. ヘッダ境界マップ（0..147,559・全フィールド実バイト）

| offset | size | field（artifact 対応） | 実測値 |
|---|---|---|---|
| 0 | 12 | magic `TES4SAVEGAME` | 一致 |
| 12 | 1 | `this+0x70`（=0） | 0x00 |
| 13 | 1 | `this+0x71` = file version byte → `[edi+0x7c]` | 0x7D (125) |
| 14 | 16 | **SYSTEMTIME = `+0x94`（GetSystemTime・v307 残点4）** | words = (2026, 11, **dow=6**, 7, 5, 20, 22, **ms=318**) = **2026-11-07 05:20:22.318 UTC** |
| 30 | 4 | u32 version = `+0xA4`（v307 残点4） | 125 |
| **34** | **4** | **u32 size ＝★size field**（writer 0x464DEC / reader FUN_45DBC0 0x45DD69 の 4B） | **147,521（0x24041）** |
| 38 | 4 | **F1 u32 counter = `[SaveGame+0x88]`**（writer 0x464E74） | **1**（= ファイル名 'Save 1'） |
| 42 | 1 | F2 u8 len(name1)（strlen+1） | 5 |
| 43 | 5 | F2 name1 | `"None\0"` |
| 48 | 2 | F3 u16 Level（0x5E1FD0→0x4677F0 経路） | 1 |
| 50 | 1 | F4 u8 len(name2)（strlen+1） | 16 |
| 51 | 16 | F4 name2 | `"Imperial Prison\0"` |
| 67 | 4 | F5 f32（writer 0x464FCF「日数 float」[推定]） | 1.042361（= 25.0167 時間） |
| 71 | 4 | F6 u32 遊戯時間 ms（writer 0x465000） | **71,015 ms = 71.015s → filename 00.01.11（秒切捨）と一致** |
| **75** | **16** | **F7 SYSTEMTIME（GetLocalTime・0x465044）** | words = (2026, 11, 6, 7, **14, 20, 22, 318**) = **2026-11-07 14:20:22.318 JST** |
| 91 | 4 | F8 u32 thumbSize（writer = pixels+8） | **147,464** |
| 95 | 4 | thumb w (u32) | **256** |
| 99 | 4 | thumb h (u32) | **192** |
| 103 | 147,456 | thumb pixels RGB24 候補（=256×192×3） | 平文写真データ（64KB エントロピー約 4.7〜6.2・JPEG SOI なし・行 stride 768） |
| **147,559** | — | **ヘッダ+thumb ブロック終端 = 38 + size（完全一致）** | 以降 payload |

**検算（すべて完全一致）**:
- `38 + u32@34 = 147,559 = 95 + thumbSize = 103 + 256×192×3` ✓
- `0x24 + len(name1) + len(name2) + thumbSize = 36 + 5 + 16 + 147,464 = 147,521 = u32@34` ✓（`0x24=36` の内訳 = counter4 + lenbyte1 + level2 + lenbyte1 + float4 + playtime4 + SYSTEMTIME16 + thumbSize4 = 36 ✓）
- gate 整合: version=125 ≥ 0x52 → 14..29 SYSTEMTIME が読まれる（v307 残点4 の gate と一致）/ ver≥0x38 → F1・F7 常時 / ver≥0x34 → F3 / ver≥0x1D → F6（本ファイルは全 gate 通過＝全フィールド存在）✓

### 4-2. payload（147,559..836,742 = 689,183 B）セクション境界表

| offset 範囲 | size | セクション（テンプレート実測） | 判定 |
|---|---|---|---|
| 147,559..147,753 | 194 | **masters**: u8 count=**11** + 11×(u8 len + 名前)。Oblivion.esm + DLC 10 種（DLCShiveringIsles.esp 等、11/11 すべて ASCII 連続読了） | **[確定(観測)]**（u16/u32 count 解釈は不整合 = 11 件連鎖が唯一成立） |
| 147,753..147,761 | 8 | **dir**: u32=**822,982**（＝index 表開始位置 822,982 と完全一致）＋ u32=**5,527**（＝レコード数と完全一致） | **[部分確定]**（2 値とも実測一致・読取タイミング/命名は Phase B [未確定]） |
| 147,761..147,793 | 32 | `00 08 00 FF` / u32 60 / u32 0 / u32 −1 / formID 0x0001FBB9 / **f32×3 = (1406.9, −54.42, −107.59)**（位置候補） | **[未確定]**（float 3 連は位置 [推定]） |
| 147,793..148,555 | 762 | **global 候補表①**: u16 count=95 + 95×(u32 id, f32 val)、id 1..95 連続（例 id7=1.0, id8=1000.0, id10=509.0, id95=432.0） | 表境界 **[確定]**（count×8 で終端一致）/ 意味 **[推定]**（global 値表） |
| 148,555..148,666 | 111 | 中間: u32 8 / f32≈2.0 / u16 16 / f32 1.938 / 0 / −1 … / u32 1003 / u16 1700 等の misc scalar | **[未確定]** |
| 148,666..150,364 | 1,698 | **global 候補表②**: u16 count=**212** @148,666 + 212×(u32 id 97..308 連続, f32 値=−1.0 中心) @148,668 → **終端 = 150,364 がチェーン起点とピッタリ一致（212×8 検算）** | 表境界 **[確定]** / 意味 **[推定]** |
| **150,364..822,976** | 672,612 | **★change レコード連鎖 = 5,527 records**（テンプレ `[formID u32][formType u8][changeFlags u32][+9 u8][tailSize u16][tail payload tailSize B]`） | **[確定]**（§4-3） |
| 822,976..822,982 | 6 | trailer: `02 00 00 00 00 00` → 候補 A `[u16 2][u32 0]` ＋ B `[u32 2][u16 0]`（直後に u32 3437 = index count） | **[未確定]**（算術整合のみ。B 推定: 「nTables=2 + 空表 count=0」） |
| 822,982..836,734 | 13,752 | **index 表**: u32 count=**3,437** @822,982 + 3,437×u32 entries @822,986..836,734（=13,748B → **footer 開始 836,734 と完全一致**） | 構造 **[部分確定]** / 意味 **[未確定]**（下記） |
| 836,734..836,742 | 8 | footer `01 00 00 00 00 00 00 00` | **[未確定]** |

**index entries の意味裁定（証拠列挙）**:
- payload offset 説: base=P0 で inside 2,885 / outside 552、delta 分布が（12,19,27,30,2,13…）と**一様分散＝特定オフセットへ収束せず [推定に至らず]**。
- formID 説: byte3（上位）分布 = {0:3248, 3:87, 1:40, 2:16, 10:13, 4:12, …} は mod index 形と整合するが、**チェーン内 formID と一致するのは 220/3,437** で populations が異なる。
- → entries は「チェーン外の別 formID 群 or 別意味の u32 配列」＝**[未確定]**（Phase B の writer/reader 突合対象 P3）。

### 4-3. change レコード歩行結果（テンプレ実測）

**テンプレ（v317 N1 の writer 列 10B+2B+payload と実データ一致）**:
```
+0  formID      u32
+4  formType    u8   (0x06..0x3D の formatter 範囲内)
+5  changeFlags u32
+9  byte        u8   (=0x7D「version byte」・v317 残件4 の tail==0 経路実測)
+10 tailSize    u16  (payload 長・frame 4B を含む実測総長)
+12 payload     tailSize B
```
**サンプルレコード（先頭 rec#0 @150,364）**:
```
76 26 08 00 | 32 | 00 00 00 08 | 7D | 91 00 | 03 02 00 36 01 00 00 01 00 00 00 01 ...
formID=0x00082676  type=0x32  flags=0x08000000  +9=0x7D  tailSize=145  payload...
```

**歩行統計（5,527/5,527 records・[150,364, 822,976) は崩れなし）**:

| 項目 | 実測 |
|---|---|
| レコード数 | **5,527 = u32@147,757 と完全一致**（起点 150,364 は後方拡張で決定、end 822,976） |
| formType 分布 | 0x23=1,680 / 0x30=186 / 0x31=1,015 / 0x32=2,038 / 0x33=334 / 0x3B=274（計 5,527、formatter 範囲 0x06..0x3D 内・**0x3D 等 AI 系 0 件**） |
| +9 byte | **0x7D のみ（5,527/5,527）** |
| tailSize | min 1 / max 2,718 / sum 606,288 / avg 109.7 / **==0 は 0 件** |
| 一意 formID | 5,527/5,527 |
| flags ビット出現 | bit0=6, bit2=3,447, **bit3=1,871**, bit4=1,679, bit5=1, bit6=205, bit9=1,680, bit20=3, **bit23=31**, bit24=1, bit25=28, bit26=26, bit27=3,010, bit28=4, bit29=4, bit30=2 / **bit1=0・bit11=0・bit31=0** / flags==0 は 0 件 |
| bit3 クロス集計 | bit3∧(bit1\|2)=1,679（全 type 0x23）/ **flags==8 単独 = 187（type 0x30=185, 0x31=2）** / bit3+他・bit1\|2 無（rewrite 候補）=5（type 0x23:1, 0x31:4）/ no bit3 = 3,656 |
| bit9/bit4/bit6 | bit9=1,680 は **type 0x23 と完全一致** / bit4=1,679 ⊂ 0x23 / bit6=205 は type 0x31 のみ |

**歩行が仕様と整合する区間 / 崩れる区間**:
- **整合区間: [150,364, 822,976) 全域（5,527 連続・崩れ 0・count 欄一致）[確定]**
- **崩れる区間**: 147,559（masters: formType byte=0x4F='O' で不成立）、147,753（dir: 0x97 で不成立）、globals 表帯（連続 8B 単位・tailSize>0x4000 不成立）、822,976 以降（trailer: formType=0x00 で不成立）→ テンプレは change 領域に**排他的**。
- 起点・終点を指す u32（150,364 / 822,976）は**ファイル内に存在しない**（全 u32 探索）→ レコード列は**直列読取（count 欄 5,527 を先読み）**で走査される構造 [推定-強い]。

**領域マッピング（契約書「global / quest / cell / AI」への答）**:
- **global**: 候補表候補表①②（§4-2、独立セクション）[推定]
- **quest (0x3B)=274 / cell (0x30)=186**: 独立セクションなし・**チェーン内 formType 区分**（境界=各レコード）[確定]
- **AI**: 該当 formType レコードなし [確定負(本サンプル)]
- **pool 採番 0x453500 系**: ファイル側に採番痕跡なし（tail は tailSize で連続表現のみ）[確定負(ファイル面)]

---

## 5. 圧縮裁定（項目3）

**判定: [確定] 本サンプルは無圧縮（選択肢①）／②raw deflate・③カスタム/分割チャンク・④場所限定圧縮は [確定負]**

| 証拠 | 結果 |
|---|---|
| 有効 zlib ヘッダ（CMF=0x78, checksum 適合）全探索 | **44 候補 → decompress 成功 0 / 44（全 exception）** |
| 内訳 | 78 9C=**0 件**（全域）/ 78 01=**42 件（すべて payload 内）** / 78 DA=**1 件（@493552）** / 78 5E=1 件（@34548・thumb 内） |
| raw deflate（wbits=−15, eof 要求, len≥64） | **4,273 オフセット（payload 開頭 1B 走査＋4KB 粒度＋代表点）→ 成功 0** |
| エントロピー（512B×1,635 ブロック） | **≥7.0 = 0 件**（全域 max 6.717=thumb 帯）/ payload 内 ≥6.0 連続区間 **0** / payload 64KB 帯 = 3.36〜4.22 |
| 平文性 | payload 先頭から `0B 0C "Oblivion.esm" …` と **ASCII 文字列がそのまま出現**、レコード・index も平文可読 |

**各観測の裁定**:
- **78 9C = 0 件の意味**: 圧縮zlib ストリーム（level 既定）が**1 本も存在しない**ため、既定 2byte 列が現れないだけ [確定]。「78 9C が無い＝別の圧縮」ではなく「圧縮そのものが無い」。
- **78 01×42 / 78 DA×1（@493552）**: 44 候補の decompress 全滅＋該当帯エントロピー 3.4〜4.2 ＝**非圧縮バイナリ中の偶発 2byte 列** [確定]。
- **オフセット群 0xD000=53,248 / 0x4000 / 0x2000 / 0x11000 / 0x13000**: 5 つとも thumb ピクセル帯（95..147,559）**内部**。@0xD000 は `0a 0a 09 0a 0b…` の暗いグラデーション画素で**構造切替なし** → PC .ess は CON 容器の 0xD000 基準を**持たない**（v307「'CON ' 一致時のみ seek 0xD000」と整合）[確定(本サンプル)]。
- **38 以降の u32 候補 / 階層構造**: size=u32@34・counter=u32@38・dir（822,982/5,527）・globals u16 count・records count 5,527・index count 3,437 — **階層は count/directory 型**であり、`<u32 size><data>` の**サイズ連鎖型チャンク構造は検出されず [確定負]**（size 連鎖の代わりに dir が index 開始を指す）。

---

## 6. artifact との整合・実測差の列挙（旧結論は不削除）

| # | artifact の記載 | 出典 | 本件の裁定 | 裁定理由（実データ根拠） |
|---|---|---|---|---|
| 1 | 「dow 位置 [28..29]=0x13E=318（異常）」＋「mtime と不整合」 | 観測済み事実 2（`ess_sample_inspect.py`） | **[確定] 観測スクリプトの field 順誤り。ファイル自体に異常なし（両論併記: (a) 異常説、(b) 誤読説 → (b) 採用）** | スクリプト line42 が `<8H>` を (year,month,**day,hour,min,sec,ms,dow) として解釈。Windows SYSTEMTIME 正順は (year,month,**dow**,day,hour,min,sec,ms)。**正順では [18..19] wDayOfWeek=6=土曜=2026-11-07（当日 Sat と一致）**、[28..29]=318 は wMilliseconds（妥当範囲）。mtime 不整合も同誤りで消滅: 真値 **2026-11-07 05:20:22.318 UTC + 9h = 14:20:22 JST = mtime 14:20:22 と秒一致** |
| 2 | 「size 式 `total = 38 + u32@38` は不一致（u32@38=1・差 836,703）→ v312 の候補式は実データで否定」 | 観測済み事実 5 / v312 | **[確定] 字句どおり否定＋補正**: (a) u32@38 は size ではなく counter [確定負]、(b) **size field = u32@34**、(c) `38+size` はヘッダ+thumb ブロック終端 147,559 と完全一致するが**ファイル総長とは 689,183B 差で不一致** | u32@34=147,521 → 38+ = 147,559 = 95+thumbSize（§4-1 検算）。u32@38=1 は v313 項目3 の counter＝ファイル名 N=1 と一致。全ファイル探索で file_size−{0,4,38,42} を持つ u32 は **0 件 [確定負]** |
| 3 | 「書出 size 式 `0x24+len+len+thumb` とヘッダ 38B の 2 バイト差が残点」 | v307 残点6 | **[確定] 解消** | size は **offset38 起点**の残長であり、定数部 0x24=36 は「counter4+lenbyte1+level2+lenbyte1+float4+playtime4+SYSTEMTIME16+thumbSize4」、両 len は NUL 含み → 36+5+16+147,464=147,521=u32@34 が**完全一致**。2B 差は「ヘッダ全体 38B（=magic..size）」と「size の計上範囲（38..末尾）」の基準差で説明可能 |
| 4 | 「offset34..37 = 4B、読取側未使用（FUN_45DBC0 内で値は戻されない）[部分]」 | v307 残点4 | **[確定] 補足**: ローカル内では未使用でも、**FUN_45DBC0 の戻り値として FUN_45D5F0 の arg2（fill バイト数=size）へ供給**される構造と実測が一致 | FUN_464320「ヘッダ読（size 返）」→ FUN_45D5F0(arg2=size, new[] 容量・fill バイト数) = v313 項目5 確定列。実測 size=147,521 = F1..F10+thumb 全体のバイト数 |
| 5 | v313 項目5: thumb tail「F8=u32 size(=8+w*h*3), u32 w, u32 h, RGB24 pixels [推定〜部分確定]」「w/h=256×256 [推定]」 | v313 | **[確定] に昇格（実測）**、ただし **256×256 は 256×192 = 実測差** | thumbSize=147,464 = 8+256×192×3、w@95=256 / h@99=192、pixels 終端 103+147,456=147,559=38+size（三条件一致）。RGB24 の**チャンネル順（RGB/BGR）は実測不能 [未確定]** |
| 6 | v313 項目3: offset38 u32 = counter | v313 | **[確定] 動的昇格** | 値=1、ファイル名 'Save 1' の N=1 と一致、count 説候補（masters 11 / globals 95,212 / records 5,527 / index 3,437）はいずれも不一致 [確定負] |
| 7 | v317 N1: 「bit3 単独 entry は pass2 で削除され非書出 [確定]」 | v317 §4-1 | **(a) 命令事実（削除経路 0x461CC5 の存在）は維持**／**(b) 一般化「単独 bit3 はファイルに現れない」は本実データで反例 187 件 → 実測差として記録**（適用範囲が tail/型/順序で限定される可能性 [未確定]） | **flags==8 単独が 187 件生存**（type 0x30=185, 0x31=2）。あわせて **bit11(0x800)=0 件 = pass2 再発行の痕跡なし**、rewrite 候補 5 件も tail==0 ゲート（v317 0x461CEE je 0x461dbc）で skip → **本セッション（新規ゲーム・ロード済み .ess なし）では pass2 の bit3 機構が未発火**という整合解釈を併記。旧結論は削除しない |
| 8 | v317 N8: Δ=0x1C/0x2C の内容は「実 .ess 待ち [未確定]」 | v317 §6-1/§7 | **[未確定] 維持（本サンプルでは観測不能）** | bit31 立ち 0 件 → 0x2C 例なし [確定負(該当)]。0x1C は pass2 再発行時のみ除去されるが再発行が未発火（#7 同様）→ 切り抜き前後を比較できる実データなし |
| 9 | v283: tail frame `<u16 len><byte type '0'..'3'><byte extra typeID>` | v283/v315 R3/v317 N8 | **実測差（仕様側は直さない）**: 本サンプルの fresh（tail==0 経路）レコード 5,527 件では **payload 先頭に frame が一様出現しない** | payload[2] の digit 出現率が全 pos 通じ最大でも 6.3%（pos7=27.2%含め散布）、`u16@pos == tailSize−k` の優位位置なし（max 4.9%）。payload 先頭は serializer（v317 N1 経路B: vtable+0x4C→0x460710→0x45C4A0）出力が占める。frame 仕様と実データの対応は Phase B（P2）へ |
| 10 | v313 項目7 seek 0xD000 / step 0x4000・0x2000 は「CON チャンク構造上の理由は実 .ess 待ち [推定]」 | v313 §7-11 | **[確定(本サンプル)] PC .ess では該当オフセットに構造が存在しない**（全て thumb 内）→ CON 専用基準という旧推定を実測で裏付け | §5 表（0xD000/0x4000/0x2000/0x11000/0x13000 すべて pixels 帯・切替なし） |
| 11 | v316: ファイル名書式 `'Save N - name1 - name2, Level L, Playing Time HH.MM.SS'` | v316 項目6 | **[確定] 動的完全一致** | 正規表現マッチ・N/name1/name2/Level/PlayTime（71,015ms→71s 秒切捨）すべて一致 |
| 12 | v307 残点2（機構）: 'CON ' 不一致時は現在位置から TES4SAVEGAME 検査＝PC 経路は seek 非通過 | v307 | **整合（訂正なし）** | offset0 の magic 一致・0xD000 が thumb であることと両立 |
| 13 | v312 件4/v315 R3: 12B = `<formID><formType><changeFlags><u8 +9><u16 tailSize>`、+10=tail payload size、alloc=+4 | v312/v315 | **[確定] ファイル側で完全一致**（10B+2B 書出 = 12B、payload は tailSize 内包） | §4-3 歩行（5,527 連続・count 一致） |
| 14 | v313 項目5: F7 ver≥0x38 = ファイル 16B（GetLocalTime 0x465044） | v313 | **[確定] 動的**（2 バンド目の正体） | @75..90 = ローカル 14:20:22.318、@14..29（GetSystemTime）＝UTC 05:20:22.318、**差は厳密 +9h・ms は同値 318** |

---

## 7. 新規残件・Phase B に残す EXE 突合項目

### 7-1. file 側新規残件（追加サンプル・追加観測で解けるもの含む）

1. **[未確定] index 表（3,437×u32 @822,986）の意味** — formID 候補（byte3=mod 分布）だが chain formID 一致 220/3,437・payload offset としての収束もなし（§4-2）。
2. **[未確定] trailer `02 00 00 00 00 00`（822,976..982）の候補 A/B の裁定と命名** — 候補 B `[u16 nTables=2][u32 0][u32 3437]` が算術的に整合。
3. **[未確定] dir 8B（u32 822,982 / u32 5,527）の読取タイミング・命名**（値一致は実測済）。
4. **[未確定] 中間セクション 148,555..148,666**（u32 8 テーブル候補・u16 1700・misc scalar）の同定。
5. **[推定] global 候補2表（95 件 ids1..95 / 212 件 ids97..308・f32 値）の正体**（GLOB or GMST）。
6. **[未確定] F5 float = 1.042361 の意味**（writer「日数」[推定]・playtime 71s と非対応・25.0 時間相当）。
7. **[未確定] 再保存時に +0x94（GetSystemTime）が更新されるか** — v307「+0xA4==0 のときのみ GetSystemTime」[推定] だと2回目保存で時刻が古いままになる。**要 save#2 サンプル（動的追加）**。
8. **[未確定] thumb ピクセルのチャンネル順（RGB/BGR）** — ファイル単独では判定不能。
9. **[未確定] name2 語彙の一般化** — 本サンプルは 'Imperial Prison' 1 件のみ（#84 依頼の「実語彙サンプル」＝1 件）。
10. **[未確定] 2 SYSTEMTIME を二重書きする意図**（UTC=+0x94 と local=F7）— 存在・値は [確定]、用途差は [推定]。
11. **[未確定] Δ=0x1C の実データ内容** — pass2 再発行が発火した実例（ロード→再セーブした .ess）が無い。**追加サンプル or Phase B で決着**。
12. **[未確定] footer `01 00 00 00 00 00 00 00`** の意味（count=1 + 0?）。

### 7-2. Phase B（EXE 突合）残項目 — **12 件**

| # | 項目 | 実データ側の申し送り |
|---|---|---|
| P1 | LoadGame 読取列（v317 の 45BB00 callers: push 4 @0x465D97 / push 6 @0x465DC0 / push 0xC @0x465DFF(ver<0x5B) / 0x465ED6 / push 0x2C @0x466113 / 0x46667E / count=tailSize @0x466816）と実レイアウト `[formID4][type1][flags4][+9 1][tailSize2][payload]` の対応 | **4+6+2=12 の分割読取 [推定]**（6B = type+flags+9byte）。0x2C=44 と 6 の意味も要突合 |
| P2 | serializer 0x460710 / 0x45C4A0（tail==0 経路）出力構造と v283 frame の関係 | payload 先頭は frame 非出現＝**実測差**（§6-9） |
| P3 | dir 8B / trailer / index 3,437 の writer・reader 突合 | 値・算術一致済み、名称・読取順序 [未確定] |
| P4 | pass2 の bit3 削除/翻訳の発火ゲート（tail 経路か型ゲートか） | **flags==8 単独 187 件生存・bit11=0** が N1 一般化の実測反例（§6-7） |
| P5 | FUN_45DBC0 が size 返却 = u32@34 である reader 側の確定（v307 残点4「0x45DD83 の値の用途」/ 残点6） | writer 0x464DEC と完全一致済み・reader は返却経路のみ |
| P6 | +0x94 GetSystemTime の更新ゲート（+0xA4==0 [推定]） | save#2 サンプルと併せて裁定（§7-1-7） |
| P7 | F5 float の型・getter（0x464FCF 前後） | 実測値 1.042361 |
| P8 | 中間セクション（148,555..148,666）の writer 突合 | u32 8 / u16 1700 等 |
| P9 | global 候補2表の正式名（GLOB/GMST 判定・writer 呼出列） | count×8 完全一致済み |
| P10 | 2 SYSTEMTIME 二重書きの意図（GetSystemTime vs GetLocalTime） | UTC/local・ms 同値実測済み |
| P11 | Δ=0x1C 切り抜きの writer（0x461D94 append 元）と実データ対応 | 再発行未発火のため実データ比較不能 |
| P12 | thumb 書出（×3B コピー元＝D3D サーフェス形式）→ RGB/BGR | 実測 256×192 RGB24 候補 |

（Phase B 残項目数 = **12**）

---

## 8. 再現手順

1. **サンプル準備（原本は読取のみ）**:
   ```powershell
   $src = 'C:\Users\hiroki.kogarumai\OneDrive - 関東航空計器株式会社\ドキュメント\My Games\Oblivion\Saves\Save 1 - None - Imperial Prison, Level 1, Playing Time 00.01.11.ess'
   Copy-Item -LiteralPath $src -Destination "$env:TEMP\opencode\v320_sample.ess" -Force
   (Get-FileHash -LiteralPath "$env:TEMP\opencode\v320_sample.ess" -Algorithm MD5).Hash   # FC32EFAEB90D26141D3DC4A325AC10E7
   ```
2. **probe 実行（全12本）**:
   ```powershell
   chcp 65001
   python $env:TEMP\opencode\v320_<n>.py *> $env:TEMP\opencode\v320_<n>.out
   ```
   `<n>` = 01 / 02 / 03 / 03b / 04a / 04b / 04c / 04d / 04e / 04f / 04g / 05（§2 表）。
3. 依存は標準ライブラリのみ（struct / io / zlib / datetime / math / collections / re / bisect）。`tmp/re/re_util.py`・`v307_common.py` 不使用・ESM/Ghidra/EXE 不接触。
4. 判定はすべて .out のバイト値・検算出力に基づく（本文の数値はすべて .out からの転記）。

## 9. selftest 結果

- **初回実行**: v320_01..v320_05, 03b, 04a..04g, 05 全 **12 本 EXIT=0**（v320_01 は NameError 1 回を修正後に EXIT=0・修正後のみ再実行）。
- **selftest（決定的検証）**: 全12本の .out の MD5 を採取 → 全本再実行 → **hash-differences = 0**（Traceback / Error / Warning = 0 件）。実行コマンド:
   ```powershell
   $dir="$env:TEMP\opencode"; $before=@{}; Get-ChildItem "$dir\v320_*.out" | %{ $before[$_.Name]=(Get-FileHash $_.FullName -Algorithm MD5).Hash }
   01,02,03,03b,04a,04b,04c,04d,04e,04f,04g,05 | %{ python "$dir\v320_$_.py" *> "$dir\v320_$_.out" }
   # MD5 再比較 → 差分 0
   ```
- 内容別 selftest ポイント:
  - v320_01: `38+u34 == thumb_end` True / v307 式 == u32@34 True / file_size−X ヒット 0。
  - v320_02: band2−band1 = 9:00:00 True / dow=6 が 2026-11-07(土) と一致 / mtime 秒一致 True。
  - v320_03/03b: zlib 成功 0/44・raw deflate 成功 0・≥7.0 ブロック 0。
  - v320_04d: 連鎖 5,527 = u32@147,757 完全一致 / index 読了 end=836,734=footer start。
  - v320_04f: digit・u16 相関の一様散布（frame 非出現の根拠）。
  - v320_05: 8+w*h*3 == thumbSize True / pixels 終端 == 147,559 True。
