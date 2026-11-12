# V32 — 実 ESM から抽出した MGEF 効果コード（EFID fourcc）全一覧と EXE 側集合との突合

> 作成日: 2026-10-23 / 対象: `Oblivion.esm` (Steam GOTY・SI統合、277,504,985 bytes)
> 手法: 自作バイナリパーサ（Python 標準ライブラリのみ、pefile 等の外部ライブラリ不使用）
> 走査スクリプト: `%TEMP%\opencode\v32_walk_final.py`（統計 `v32_stats_final.json`）
> 成果物 CSV: `tmp\re\v32_esm_efid_codes.csv`

## 1. 自作パーサの構造前提と実測での検証結果

### 1.1 構造前提の実測判定

| 構造 | 依頼時の前提 | 実測判定 | 検証根拠 |
|---|---|---|---|
| レコードヘッダ | 20 bytes | **20 bytes [確定]** | `TES4`@0x00、size=0x2E8 → 0x14+0x2E8=0x2FC に次レコード/`GRUP` 実在。0x14 に `HEDR` 実在 |
| GRUP ヘッダ | 24 bytes | **20 bytes（前提を訂正）** | GRUP@0x2FC の +20=0x310 に `GMST` レコードシグ+u32 size=50 が完全整合。+24=0x314 は `32 00 00 00` でシグ不成立 → 24B 説は棄却。20B 前提で全 85,079 GRUP / 1,167,017 レコードを **0 errors** 走査できること自体が最大の証拠 |
| GRUP size | 自身を含む | **自身を含む [確定]** | 0x2FC + 0x6B43 = 0x6E3F に次 `GRUP`(GLOB) 実在、groupContent 終端と完全一致 |
| サブレコードヘッダ | 6B（tag4+size2）要検証 | **6B + size は u16 [確定]** | §2 の根拠 hex 参照。u32 仮説は TES4 内で次タグが非 ASCII になり不整合 |

### 1.2 パーサが対応した例外機構（0 errors に必須）

1. **XXXX サブレコード（1 件のみ）**: `XXXX`(size=4) のデータ u32 が**次のサブレコードの実サイズ**を宣告する。
   実測: WRLD レコード内 `0x2388169` = `XXXX 04 00` + u32=70200 → 直後の `OFST` は size フィールド u16=0 だが実体 70200 bytes
   hex: `58 58 58 58 04 00` / `XXXX` データ u32=`38 12 01 00` / 次タグ `4F 46 53 54 00 00`
   XXXX を無視するとこの 1 レコードでサブレコード走査が破綻（依頼文の「0 errors」との差分要因）。
2. **圧縮レコード 41,789 件**（flag 0x00040000 = LAND 31,083 / PGRD 8,224 / NPC_ 2,482）: ボディ先頭が
   `u32 伸長後サイズ` + `zlib(78 9C ...)` の形式。41,789/41,789 成功、伸長サイズ宣言値と全件一致。
   例: NPC_@0x93A65C ボディ先頭 `0E 04 00 00 78 9C 3D 92` = size 16398 + zlib ヘッダ `78 9C`

### 1.3 走査結果 vs 過去実測（`tmp\re\copilot_archive\content_inventory_final.txt`）

| 指標 | 過去実測 | 本走査 | 判定 / 乖離原因 |
|---|---|---|---|
| レコード総数 | 1,167,017 | 1,167,017 | **一致** |
| レコード種類数 | 64 | 64 | **一致** |
| GRUP 数 | 85,079 | 85,079 | **一致** |
| errors | 0 | 0（構造）+ 0（サブレコード）= 0 | **一致**（XXXX/圧縮の2機構を実装後） |
| max depth | 5 | 6 | 数え方の差: トップレベル GRUP を 1 起点で数えた値。0 起点なら 5 で一致（深さ別 {'1': 56, '2': 3147, '3': 473, '4': 2782, '5': 35520, '6': 43101}） |
| SPEL レコード数 | 1,137 | 1,137 | **一致** |
| SPIT 出現数 | 1,138 | raw 1138 / サブレコード 1137 | 生バイナリ 1,138 は 1 件が偽陽性（0x434d357 は他データ中の `SPIT` 列、非サブレコード）→ 実体 1,137 は SPEL 1,137 と 1:1 |
| EFID 出現数 | 5,553 | raw 5553 / サブレコード 5553 | **完全一致**（生文字列 5,553 は全て真のサブレコード） |
| MGEF 文字列 | 147 | raw 147 / レコード 145 | 余剰 2 = GRUP ラベル `MGEF`@0x4743C(1) + TES4 内 OFST 表@0xC0(1) → レコード 145 で整合 |
| GRUP 文字列（参考） | - | raw 85080 / ヘッダ 85079 | 余剰 1 = TES4 内 OFST 表@0x48 の名称フィールド |

深い階層（深さ=6 の連鎖、トップレベル=1 起点）: GRUP@0x23880c5(type 0, label 'WRLD') → GRUP@0x23993b1(type 1, label '<\x00\x00\x00') → GRUP@0x23e4261(type 4, label '\x00\x00\x00\x00') → GRUP@0x23e4275(type 5, label '\x00\x00\x00\x00') → GRUP@0x23e42cc(type 6, label 'Æ_\x00\x00') → GRUP@0x23e42e0(type 10, label 'Æ_\x00\x00')

## 2. サブレコード size の u16 / u32 確定と根拠 hex

**結論: サブレコードヘッダ = `tag(4) + size(u16)` = 6 bytes。**

- 根拠 A（TES4 ヘッダ内 HEDR）:

  ```
  0x0014: 48 45 44 52   <- tag 'HEDR'
  0x0018: 0C 00          <- size = 12 (u16)
  0x001A: 00 00 80 3F FF 1A 13 00 4D 76 09 FF   <- HEDR data (12B: float 1.0, numRecords, nextObjectID)
  0x0026: 4F 46 53 54   <- 次サブレコード 'OFST' が正確にここに出現
  ```

  u32 仮説なら size 読みが `0C 00 00 00`、data は 0x1C からで次タグが 0x28 =
  `53 54 AC 02`（非 ASCII・不正タグ）→ **u32 は不成立**。

- 根拠 B（最初の GMST レコード内 EDID → DATA 間の境界 0x338）:

  ```
  0x0324: 45 44 49 44        <- tag 'EDID'
  0x0328: 0E 00               <- size = 14 (u16)
  0x032A: 73 4D 69 73 63 53 45 42 6F 75 6E 74 79 00 <- data 'sMiscSEBounty\0' (14B: 0x32A..0x337)
  0x0338: 44 41 54 41            <- 次サブレコード 'DATA' が正確にここに出現
  ```

- 例外: 唯一の `XXXX`(0x2388169) が次サブの size を u32 で宣言（§1.2-1）。

## 3. EFID distinct コード総数と頻度上位 40

- **EFID サブレコード総出現: 5,553 / distinct: 150**（size は全て 4 bytes）
- 内訳: ENCH 2,411 + SPEL 1,856 + INGR 656 + ALCH 330 + SGST 300 = 5,553
- `[A-Z]{4}` で表せるコード: 118 / 150

| # | code | total | SPEL | ENCH | INGR | ALCH | SGST/other | first_seen |
|---|---|---|---|---|---|---|---|---|
| 1 | FOAT | 517 | 136 | 292 | 32 | 32 | 25 | ENCH |
| 2 | FOSK | 485 | 86 | 376 | 0 | 13 | 10 | ENCH |
| 3 | DRAT | 333 | 108 | 155 | 3 | 67 | 0 | ENCH |
| 4 | REAT | 200 | 112 | 16 | 48 | 24 | 0 | ENCH |
| 5 | DGAT | 199 | 21 | 79 | 74 | 25 | 0 | ENCH |
| 6 | FIDG | 179 | 70 | 83 | 8 | 3 | 15 | ENCH |
| 7 | SHDG | 167 | 66 | 78 | 7 | 1 | 15 | ENCH |
| 8 | FRDG | 155 | 63 | 68 | 8 | 1 | 15 | ENCH |
| 9 | DGHE | 154 | 55 | 48 | 43 | 3 | 5 | ENCH |
| 10 | SEFF | 143 | 89 | 40 | 5 | 9 | 0 | ENCH |
| 11 | REHE | 137 | 92 | 13 | 27 | 5 | 0 | ENCH |
| 12 | RSMA | 97 | 44 | 40 | 0 | 3 | 10 | ENCH |
| 13 | FOHE | 96 | 23 | 27 | 17 | 19 | 10 | ENCH |
| 14 | FOSP | 95 | 42 | 26 | 14 | 3 | 10 | ENCH |
| 15 | DGSP | 79 | 15 | 31 | 25 | 3 | 5 | ENCH |
| 16 | SHLD | 72 | 18 | 35 | 14 | 0 | 5 | ENCH |
| 17 | RSDI | 69 | 23 | 29 | 9 | 3 | 5 | ENCH |
| 18 | REFA | 67 | 12 | 2 | 43 | 10 | 0 | ENCH |
| 19 | RFLC | 67 | 14 | 44 | 6 | 3 | 0 | ENCH |
| 20 | BRDN | 65 | 8 | 22 | 27 | 3 | 5 | ENCH |
| 21 | LGHT | 63 | 11 | 33 | 11 | 3 | 5 | ENCH |
| 22 | SLNC | 63 | 13 | 18 | 19 | 3 | 10 | ENCH |
| 23 | ABSK | 61 | 19 | 42 | 0 | 0 | 0 | ENCH |
| 24 | ABAT | 60 | 9 | 26 | 0 | 0 | 25 | ENCH |
| 25 | DTCT | 59 | 12 | 26 | 12 | 4 | 5 | ENCH |
| 26 | CHML | 56 | 15 | 21 | 12 | 3 | 5 | ENCH |
| 27 | PARA | 56 | 29 | 16 | 8 | 3 | 0 | ENCH |
| 28 | RSPO | 56 | 20 | 26 | 6 | 4 | 0 | ENCH |
| 29 | DSPL | 55 | 13 | 21 | 12 | 4 | 5 | ENCH |
| 30 | RSFR | 52 | 15 | 22 | 6 | 4 | 5 | ENCH |
| 31 | RSPA | 49 | 28 | 12 | 9 | 0 | 0 | ENCH |
| 32 | REDG | 48 | 10 | 32 | 6 | 0 | 0 | ENCH |
| 33 | RSFI | 48 | 9 | 22 | 9 | 3 | 5 | ENCH |
| 34 | WABR | 48 | 15 | 21 | 9 | 3 | 0 | ENCH |
| 35 | SABS | 47 | 17 | 22 | 0 | 3 | 5 | ENCH |
| 36 | FTHR | 46 | 6 | 25 | 7 | 3 | 5 | ENCH |
| 37 | ABHE | 45 | 24 | 16 | 0 | 0 | 5 | ENCH |
| 38 | DGFA | 42 | 8 | 8 | 17 | 4 | 5 | ENCH |
| 39 | DRSK | 42 | 11 | 31 | 0 | 0 | 0 | ENCH |
| 40 | WKFI | 42 | 26 | 15 | 1 | 0 | 0 | ENCH |

（上表の最終列は CSV の `first_seen_record_type`。全 150 コードの完全な表は CSV 参照）

## 4. 非 ASCII EFID

**非 ASCII EFID: 0 件（判定: NO）**

- 全 150 distinct / 5,553 出現の EFID は size=4 かつ 4 バイト全てが印刷 ASCII（0x20..0x7E）だった。
- EFID サブレコードの size 分布: {'4': 5553}（= 4 のみ）。4 字以外の EFID は存在しない。
- hex 化する対象が無いため「非 ASCII hex 上位 10」は **該当なし**。

## 5. レコード種別別内訳

EFID を含むレコード型は 5 種のみ（**MGEF レコード自体は EFID を持たない**）:

| レコード型 | レコード数 | EFID を含むレコード数 | EFID 総数 | distinct | EFIT | SPIT |
|---|---|---|---|---|---|---|
| SPEL | 1,137 | 1,137 | 1,856 | 149 | EFIT 1,856 / 24B | SPIT 1,137 / 16B |
| ENCH | 1,542 | 1,542 | 2,411 | 111 | EFIT 2,411 / 24B | - |
| INGR | 173 | 173 | 656 | 48 | EFIT 656 / 24B | - |
| ALCH | 253 | 253 | 330 | 49 | EFIT 330 / 24B | - |
| SGST | 150 | 150 | 300 | 39 | EFIT 300 / 24B | - |
| MGEF | 145 | 0 | 0 | 0 | - | -（コードは EDID に格納: §6.3） |
| 合計 | 1,167,017 | 3,255 | 5,553 | 150 | EFIT 合計 5,553 | SPIT 合計 1,137 |

- EFIT size 分布: {'24': 5553} → **24 bytes [確定]**（5,553/5,553）
- SPIT size 分布: {'16': 1137} → **16 bytes [確定]**（1,137/1,137、SPEL のみ）
- 「EFID を含むレコード数」= 全レコード数と一致 → ENCH/SPEL/INGR/ALCH/SGST の**全レコード**が EFID を 1 つ以上持つ。

## 6. 3 点突合（ESM EFID vs EXE 側既知集合）

- ESM 側: 本次抽出の EFID distinct **150 種**
- EXE 側 A: `v24_mgef_codes.md` §3 の表 **115 種**（12 名空間行の合計）
- EXE 側 B: `s91_magic_m3_mgef_dispatch.md` = 登録 23 + 未登録 4 = **27 種**

### 6.1 プライマリ突合表（ESM EFID 150 vs EXE v24 115）

| 区分 | 件数 | コード一覧 |
|---|---|---|
| 両方にある（共有） | 113 | ABAT ABFA ABHE ABSK ABSP BABO BACU BAGA BAGR BAHE BASH BWAX BWBO BWDA BWMA BWSW CALM CHML CHRM COCR COHU CUDI CUPA CUPO DEMO DGAT DGFA DGHE DGSP DIAR DIWE DRAT DRFA DRHE DRSK DRSP DSPL DTCT DUMY FIDG FISH FOAT FOFA FOHE FOMM FOSK FOSP FRDG FRNZ FRSH INVI LGHT LISH LOCK MYHL MYTH NEYE OPEN PARA RALY REAN REAT REDG REFA REHE RESP RFLC RSDI RSFI RSFR RSMA RSNW RSPA RSPO RSSH RSWD SABS SEFF SHDG SHLD SLNC STRP SUDG TELE TURN VAMP WKDI WKFI WKFR WKMA WKNW WKPO WKSH ZCLA ZDAE ZDRE ZDRL ZFIA ZFRA ZGHO ZHDZ ZLIC ZSCA ZSKA ZSKC ZSKE ZSKH ZSPD ZSTA ZWRA ZWRL ZXIV ZZOM |
| ESM にあって EXE に無い | 37 | BA01 BA02 BA03 BA04 BA05 BA06 BRDN BW01 BW02 BW03 BW04 BW05 BW06 BW08 FTHR STMA WABR WAWA Z001 Z002 Z003 Z004 Z005 Z006 Z007 Z008 Z009 Z010 Z011 Z012 Z013 Z014 Z015 Z016 Z017 Z018 Z019 |
| EXE にあって ESM の EFID に無い | 2 | DARK FOAB |

EXE 側のみのコードを名空間別にみると: MGEF-fortify: FOAB, s91-known: DARK

内訳メモ: `DARK` は MGEF EDID に効果定義が実在するがアイテム/呪文の EFID 参照は 0 件。`FOAB` は EFID にも MGEF EDID にも無く EXE テーブル側のみに存在。

### 6.2 s91（登録 23 + 未登録 4 = 27）との突合

| 区分 | 件数 | コード一覧 |
|---|---|---|
| s91 かつ ESM EFID にある | 26 | ABHE CALM CHML COCR COHU DEMO DIAR DIWE DSPL DTCT FIDG FRNZ INVI LGHT LOCK NEYE OPEN PARA REAN SEFF SLNC STRP SUDG TELE TURN VAMP |
| s91 にあって ESM EFID に無い | 1 | DARK |

### 6.3 補足: MGEF レコードのコードは EFID ではなく EDID に格納

実測: トップレベル GRUP `MGEF`@0x47434 の全 145 レコードは **EDID が 4 文字コードそのもの**（全 distinct 145、長さは全て 4）。
先頭例: BW08 Z019 Z018 Z017 Z016 Z015 BW06 BW05 BW04 BW03 BW02 BW01 BW07 Z011 Z010 Z008 ...

つまり ESM の効果コード空間は 2 層: **EFID = アイテム/呪文の効果リスト参照（150 種）**、**EDID = MGEF 効果定義（145 種）**。

| 区分 | 件数 | コード一覧 |
|---|---|---|
| MGEF EDID ∩ EXE v24 | 112 | ABAT ABFA ABHE ABSK ABSP BABO BACU BAGA BAGR BAHE BASH BWAX BWBO BWDA BWMA BWSW CALM CHML CHRM COCR COHU CUDI CUPA CUPO DARK DEMO DGAT DGFA DGHE DGSP DIAR DIWE DRAT DRFA DRHE DRSK DRSP DSPL DTCT FIDG FISH FOAT FOFA FOHE FOMM FOSK FOSP FRDG FRNZ FRSH INVI LGHT LISH LOCK MYHL MYTH NEYE OPEN PARA RALY REAN REAT REDG REFA REHE RESP RFLC RSDI RSFI RSFR RSMA RSNW RSPA RSPO RSSH SABS SEFF SHDG SHLD SLNC STRP SUDG TELE TURN VAMP WKDI WKFI WKFR WKMA WKNW WKPO WKSH ZCLA ZDAE ZDRE ZDRL ZFIA ZFRA ZGHO ZHDZ ZLIC ZSCA ZSKA ZSKC ZSKE ZSKH ZSPD ZSTA ZWRA ZWRL ZXIV ZZOM |
| MGEF EDID にあって EXE に無い | 33 | BRDN BW01 BW02 BW03 BW04 BW05 BW06 BW07 BW08 FTHR POSN STMA WABR WAWA Z001 Z002 Z003 Z004 Z005 Z006 Z007 Z008 Z009 Z010 Z011 Z012 Z013 Z014 Z015 Z016 Z017 Z018 Z019 |
| EXE にあって MGEF EDID に無い | 3 | DUMY FOAB RSWD |

EFID と MGEF EDID の関係:

| 区分 | 件数 | コード一覧 |
|---|---|---|
| EFID にあって MGEF EDID に無い（定義欠落候補） | 8 | BA01 BA02 BA03 BA04 BA05 BA06 DUMY RSWD |
| MGEF EDID にあって EFID に無い（アイテム側未使用） | 3 | BW07 DARK POSN |

## 7. Z* / BA* / BW* / MY* 名空間の実 ESM 出現有無

### 7.1 EFID（依頼の主対象）での出現

| code (EXE v24 名空間) | EFID 出現数 | 実 ESM EFID に出現 |
|---|---|---|
| ZCLA | 3 | YES |
| ZDAE | 2 | YES |
| ZDRE | 2 | YES |
| ZDRL | 3 | YES |
| ZFIA | 2 | YES |
| ZFRA | 2 | YES |
| ZGHO | 2 | YES |
| ZHDZ | 2 | YES |
| ZLIC | 2 | YES |
| ZSCA | 4 | YES |
| ZSKA | 3 | YES |
| ZSKC | 2 | YES |
| ZSKE | 2 | YES |
| ZSKH | 2 | YES |
| ZSPD | 2 | YES |
| ZSTA | 2 | YES |
| ZWRA | 2 | YES |
| ZWRL | 2 | YES |
| ZXIV | 2 | YES |
| ZZOM | 2 | YES |
| BABO | 5 | YES |
| BACU | 8 | YES |
| BAGA | 7 | YES |
| BAGR | 4 | YES |
| BAHE | 4 | YES |
| BASH | 2 | YES |
| BWAX | 5 | YES |
| BWBO | 4 | YES |
| BWDA | 6 | YES |
| BWMA | 6 | YES |
| BWSW | 4 | YES |
| MYHL | 4 | YES |
| MYTH | 5 | YES |

- **Z* 20 種の EFID 出現: YES（計 45 回）**
- **BA*/BW*/MY* 13 種の EFID 出現: YES（計 64 回）**

### 7.2 補足: MGEF EDID（効果定義）側での出現

- Z*: 20 / 20 種が MGEF EDID として実在 → ZCLA ZDAE ZDRE ZDRL ZFIA ZFRA ZGHO ZHDZ ZLIC ZSCA ZSKA ZSKC ZSKE ZSKH ZSPD ZSTA ZWRA ZWRL ZXIV ZZOM
- BA*/BW*/MY*: 13 / 13 種が MGEF EDID として実在 → BABO BACU BAGA BAGR BAHE BASH BWAX BWBO BWDA BWMA BWSW MYHL MYTH
- v24 に無い追加の Z 系 MGEF EDID（19 種）: Z001 Z002 Z003 Z004 Z005 Z006 Z007 Z008 Z009 Z010 Z011 Z012 Z013 Z014 Z015 Z016 Z017 Z018 Z019
- v24 に無い追加の BA/BW/MY 系 MGEF EDID（8 種）: BW01 BW02 BW03 BW04 BW05 BW06 BW07 BW08

## 8. 判定できなかった点

1. **EXE テーブル（0x417551 等）が比較対象とする ESM 側の格納場所**は未判定。EFID（アイテム効果リスト）と MGEF EDID（効果定義）のどちらを読んでいるかは、EXE 側のリーダコード再解析（未実施）が必要。
2. **EXE 側 115 種のうち MGEF 効果本体か否か**の再分類は v24 の名空間ラベルをそのまま採用（再判定なし）。
3. **max depth の差（5 vs 6）**は深さの数え方（トップレベル GRUP を 0 か 1 か）と推定するが、過去計測ツールの実装を再実行していないため確定していない（深さ別ヒストグラムは §1.3）。
4. **EFID の効果コードと MGEF レコードの結合キー**（EDID 全文字一致か先頭 4 文字か）は未検証。今回は EDID が全件 4 文字であるため区別が付かない。
5. **EFID のうち 8 種は MGEF EDID に無い**（§6.3）の原因は未判定（参照先不明 / 未実装効果 / 別システムの可能性）。コード: BA01 BA02 BA03 BA04 BA05 BA06 DUMY RSWD  **【訂正 (V39, 2026-10-24)】原因は特定済み**: 8件（BA01-06/DUMY/RSWD）は全て EXE 内蔵の組み込み EffectSetting 登録 `FUN_00417420`(0x417420, 全161件) から供給される**フォールバック**。「未実装効果/別システム」は撤回 — 同一 MGEF 空間の2層構造（登録マップ `0xB33508` を共有）。出典 `tmp/re/v39_exe_only_codes.md`
6. **EFIT(24B) / SPIT(16B) の内部フィールド構成**（magnitude/area/duration 等の並びと型）は本タスクでは未解析。サイズだけを確定させた。

---

検証サマリ: records=1,167,017 / types=64 / GRUPs=85,079 / errors=0+0 / EFID=5,553 (distinct 150) / EFIT=24B×5,553 / SPIT=16B×1,137
