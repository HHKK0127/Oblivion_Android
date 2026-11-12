# V24 — MGEF fourcc コード空間の確定と名空間区分 (Oblivion.exe v1.2.0416)

> 作成日: 2026-10-23 / 手法: `push imm32`(opcode `0x68`) のバイト走査（int3 パディングを跨いで可）
> セルフテスト: `bytes@0x504050`=`518b4c24188b5424` / `bytes@0x4FB580`=`578b7908` / `string@0xA7035C`=`.\AI\CombatController.cpp` = **3/3 PASS**

> ## ⚠️ 訂正（2026-10-23、V32/V35 により本ファイルの結論を上書き）
>
> **本ファイルの §1 と §3 は古い。正本は以下のとおり。**
>
> 1. **MGEF コード空間の正本は実 ESM（`Oblivion.esm` 277,504,985 bytes）で 150 種**（EFID distinct）。
>    うち **MGEF レコード 145件の EDID が効果コードの正本**（4文字・全 distinct）。本ファイルの「EXE 側 115 種 / 最低 82 種」は**その部分集合**（共有113）。
> 2. **「Z\* 20種 / BA\*・BW\*・MY\* 13種は別名空間」は誤り。** 実 ESM の EFID に**全33種が実在**（Z\* 45回 / BA\*・BW\*・MY\* 64回）＝MGEF コード。
>    独立経路の裏付け: Supermemory `UAFXrL9SqCDFf6465KZiVM`（BW01-08=Bound Weapons、Z001-19=Summons、BABO-BASH=Bound Armor の全 Conjuration が FormID リンク）。
> 3. **§4 の ctor 列はすべて0だが、これは読み取り方向のミス。** 登録形式は **`push <ctor>; push <fourcc>; call 0x68DA10`**（**ctor が fourcc の 5バイト前**）。正しい 23組の `code → ctor` は
>    `tmp\re\v35_mgef_code_table.csv` / `v35_mgef_code_report.md` を参照（例: CALM→0x691CA0、INVI→0x693FE0、SUDG→0x6A6890、LGHT→0x694A30 は SEH wrapper で本体 0x694420）。
> 4. **登録23 ⊂ ESM145 = 100% 全一致**（不一致0）。未登録122種の disposition は `cmp_branch 8 / default_path 114 / undetermined 0`。
> 5. 既知の「FormID リンク62件」は**実測60件**（残り2件 REAN/TURN は actorValue=0）。
>
> 詳細: `tmp\re\v32_esm_efid_report.md`（ESM走査） / `tmp\re\v35_mgef_code_report.md`（対応表） / TODO_re49.md V34/V36/V37/V38/V39。

## 1. 結論

- **`docs/RE_ANALYSIS_ROADMAP.md` の「MGEF 32 コード」は実測と合わない。**
  本バイナリの `push <fourcc>` テーブル群には **distinct 115 種**の fourcc が存在し、
  うち MGEF 系と判定できるものだけで **82 種**（s91 の 27 を大幅に超過）。
- s91 (`tmp\re\s91_magic_m3_mgef_dispatch.md`) は**登録テーブル `0x9FEA90`（23件）のみ**を扱い、
  ESM リーダ側の**効果定義テーブル（`0x417551` ほか）は未収録**だった。
- コードは **動詞(2字) × 名詞(2字)** の規則的構造: 動詞 `AB`/`FO`/`RE`/`DR`/`DG` =
  absorb/fortify/restore/damage(+系)、`RS`/`WK` = resist/weakness、名詞 `AT`/`SK`/`HE`/`SP`/`FA` =
  attribute/skill/health/magicka/fatigue、`FI`/`FR`/`SH`/`NW` = fire/frost/shock/normal-weapons。

## 2. 検出したテーブル（`push <fourcc>` 件数）

| テーブル先頭 | 件数 | 役割 [推定] |
|---|---|---|
| `0x9FEAA0` | 23 | 効果クラス ctor の登録（23件、`call 0x68DA10` 経由） |
| `0x417551` | 94 | ESM リーダ側 効果定義テーブル A（損傷/吸収/強化/回復 系の中心） |
| `0x417AF0` | 69 | ESM リーダ側 効果定義テーブル B（精神/状態系 + 抵抗） |
| `0x4180C9` | 31 | ESM リーダ側 効果定義テーブル C（抵抗/治癒 + Z\* 名空間） |
| `0x418651` | 23 | ESM リーダ側 効果定義テーブル D（Command 系 + BA\*/BW\*/MY\*） |
| `0x6126B0` | 21 | 補助: 強化/抵抗の別系統（21件） |
| `0x613880` | 0 | 補助: 回復/強化の別系統（6件） |
| `0x68EA50` | 0 | 補助: 治癒/吸収（10件） |

## 3. 名空間別の distinct コード

| 名空間 | 件数 | コード一覧 |
|---|---|---|
| s91-known | 27 | ABHE CALM CHML COCR COHU DARK DEMO DIAR DIWE DSPL DTCT FIDG FRNZ INVI LGHT LOCK NEYE OPEN PARA REAN SEFF SLNC STRP SUDG TELE TURN VAMP |
| MGEF-absorb | 4 | ABAT ABFA ABSK ABSP |
| MGEF-fortify | 7 | FOAB FOAT FOFA FOHE FOMM FOSK FOSP |
| MGEF-restore | 5 | REAT REDG REFA REHE RESP |
| MGEF-damage | 5 | DRAT DRFA DRHE DRSK DRSP |
| MGEF-damage2 | 4 | DGAT DGFA DGHE DGSP |
| MGEF-resist | 9 | RSDI RSFI RSFR RSMA RSNW RSPA RSPO RSSH RSWD |
| MGEF-weakness | 7 | WKDI WKFI WKFR WKMA WKNW WKPO WKSH |
| MGEF-cure | 3 | CUDI CUPA CUPO |
| MGEF-other | 11 | CHRM DUMY FISH FRDG FRSH LISH RALY RFLC SABS SHDG SHLD |
| Z-namespace | 20 | ZCLA ZDAE ZDRE ZDRL ZFIA ZFRA ZGHO ZHDZ ZLIC ZSCA ZSKA ZSKC ZSKE ZSKH ZSPD ZSTA ZWRA ZWRL ZXIV ZZOM |
| BA/BW/MY-namespace | 13 | BABO BACU BAGA BAGR BAHE BASH BWAX BWBO BWDA BWMA BWSW MYHL MYTH |

## 4. 登録テーブル `0x9FEA90`（23件）— s91 と23/23一致

| # | コード | `push` 位置 | ctor |
|---|---|---|---|
| 1 | CALM | `0x9FEAB5` | `0x00000000` |
| 2 | CHML | `0x9FEAD5` | `0x00000000` |
| 3 | COCR | `0x9FEAF5` | `0x00000000` |
| 4 | COHU | `0x9FEB15` | `0x00000000` |
| 5 | DARK | `0x9FEB35` | `0x00000000` |
| 6 | DEMO | `0x9FEB55` | `0x00000000` |
| 7 | DTCT | `0x9FEB75` | `0x00000000` |
| 8 | DIAR | `0x9FEB95` | `0x00000000` |
| 9 | DIWE | `0x9FEBB5` | `0x00000000` |
| 10 | DSPL | `0x9FEBD5` | `0x00000000` |
| 11 | FRNZ | `0x9FEBF5` | `0x00000000` |
| 12 | INVI | `0x9FEC15` | `0x00000000` |
| 13 | LGHT | `0x9FEC35` | `0x00000000` |
| 14 | LOCK | `0x9FEC55` | `0x00000000` |
| 15 | NEYE | `0x9FECE5` | `0x00000000` |
| 16 | OPEN | `0x9FED05` | `0x00000000` |
| 17 | PARA | `0x9FED25` | `0x00000000` |
| 18 | REAN | `0x9FED45` | `0x00000000` |
| 19 | STRP | `0x9FED65` | `0x00000000` |
| 20 | SUDG | `0x9FED85` | `0x00000000` |
| 21 | TELE | `0x9FEDA5` | `0x00000000` |
| 22 | TURN | `0x9FEDC5` | `0x00000000` |
| 23 | VAMP | `0x9FEDE5` | `0x00000000` |

## 5. 表 `0x417551` の並び（規則性の証拠）

```
LOCK OPEN FISH RSFI DSPL FIDG LISH RSSH DSPL SHDG FRSH RSFR DSPL FRDG ABAT FOAT REAT DSPL DGAT ABHE FOHE REHE DSPL DGHE ABSP FOSP RESP DSPL DGSP ABFA FOFA REFA DSPL DGFA ABAT FOAT REAT DSPL DRAT ABSK FOSK DSPL DRSK ABHE FOHE REHE DSPL DRHE ABSP FOSP RESP DSPL DRSP ABFA FOFA REFA DSPL DRFA DSPL RSFI FISH WKFI DSPL RSFR FRSH WKFR DSPL RSSH LISH WKSH DSPL RSMA WKMA DSPL RSDI WKDI DSPL RSPO WKPO DSPL RSNW WKNW FRSH LISH FISH SHLD DSPL DIAR FRSH LISH FISH SHLD DSPL DIWE
```

`DSPL` が 3〜4個ごとに挟まる区切りとして現れ、`(AB|FO|RE|DG|DR)×(AT|HE|SP|FA)` の
セルが列単位で並ぶ行列構造を示す。`RSFI/RSFR/RSSH/RSNW` と `WKFI/WKFR/WKSH/WKNW`
が対で現れることも resist/weakness の対応を裏付ける。

## 6. 残課題

1. `Z*` 名空間（20 種）の意味 — 開発用/デバッグ用コードか、別システム（シナリオ/地区）か。
2. `BA*`/`BW*`/`MY*` 名空間（13 種）の帰属 — Command 系テーブル `0x418651` に混在。
3. `ESM-tag-or-other`（0 種）からの MGEF コードの二次分離（`cmp` 分岐側 `0x413A90` と対応付ける等）。
4. ROADMAP の「32 コード」の出所特定と訂正文の作成。
