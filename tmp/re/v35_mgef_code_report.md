# V35 — MGEF コード → EXE ハンドラ/ctor 対応表 (Oblivion.exe v1.2.0416 / Oblivion.esm)

> 作成日: 2026-10-24 / 手法: struct 自前 PE 解析 + Capstone 5.0.7 + ESM レベル走査 (zlib 展開)
> 成果物: `tmp\re\v35_mgef_code_table.csv` (header + 145行, UTF-8) / 本レポート
> VA→file: ImageBase=0x400000, .text RVA=0x1000 raw=0x400 → `VA = 0x400C00 + file_off` (pefile 未使用)

## 1. セルフテスト 5件 (全 PASS)

| # | 項目 | 実測値 | 期待値 | 結果 |
|---|------|--------|--------|------|
| ST1 | EXE bytes@0x504050 | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| ST2 | EXE bytes@0x4FB580 | `578b7908` | `578b7908` | PASS |
| ST3 | EXE string@0xA7035C | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| ST4 | EXE opcode table opid 0x0015 | `entry=0x00B0B290 impl=0x00504560` | `impl=0x504560` | PASS |
| ST5 | ESM レコード数 | `1167017` | `1167017` | PASS |

- ST4: オペコードテーブル (基底 `0xB0AF48`, stride `0x28`, 532件) で opid `0x0015` のエントリは `0xB0B290`、impl = `0x504560`。

## 2. ESM 走査統計

| 項目 | 値 |
|------|----|
| ファイルサイズ | 277,504,985 bytes |
| 走査レコード数 | **1,167,017** (GRUP は含まず) |
| GRUP 数 | 85,079 |
| 圧縮レコード (flag 0x40000) | 41,789 件 (zlib `78 9C`、u32 伸長後サイズ前置) — 既知値 41,789 と一致 |
| XXXX 例外 | 1 件 — WRLD レコード@`0x23880D9` 内の `XXXX`(size=4, real=70,200) → 次タグ `OFST` (XXX サブレコード位置 `0x2388155`)。既知記載 `WRLD@0x2388169` とは同一 WRLD 例外 (ランドマーク差異) |
| パースエラー | 0 件 |
| MGEF レコード | **145 件 / EDID distinct 145** (4文字・重複なし) |
| FULL あり | 145/145 |
| DESC サブレコード | 145/145 に存在するが、実文字列は **1件のみ** (ABAT「This is the big description for Absorb A...」、他144件はサイズ1の空文字) |
| MGEF DATA size | 64 bytes × 144 件 / 36 bytes × 1 件 (DARK のみ) |

- MGEF DATA フィールドは hexdump で実地検証済: off4=baseCost float (例 REAN=10.0, BW08=1.0, ABAT≈0.95, CALM≈0.47), off8=actorValue u32, off12=school u32, off16=flags u32。
- flags 値の分布: `0xFFFFFFFF` × 104, `0x00000040` × 35, `0x3D/0x3E/0x3F/0x42/0x43/0x44` 各1 (バニラ ESM の実バイト値をそのまま記載)。

## 3. 登録23組の完全表 (`code → ctor VA`、push 位置つき)

登録形式 (0x20 バイト間隔 + int3 パディングの個別スロット):

```
push <ctor>          ; 例 0x9FEAB0: push 0x691CA0
push <fourcc>        ; 例 0x9FEAB5: push "CALM" (0x4D4C4143)
call 0x68DA10        ; EffectSettingCollection::registerEffect(code, ctor)
add  esp, 8
mov  byte ptr [0xB3C0xx], al   ; 成功フラグ
ret
```

※ 既知情報の訂正: V24 の表では ctor 列が全て `0x00000000` だったが、これは読み取り方向の誤り。**ctor の push は fourcc の push の 5 バイト前にある** (fourcc push +5 は `call` の `0xE8`)。V35 で 23組すべてを実測取得。

| # | code | code push | ctor push | ctor VA | RTTI クラス | vtable | vtable store | 備考 |
|---|------|-----------|-----------|---------|-------------|--------|--------------|------|
| 1 | CALM | `0x009FEAB5` | `0x009FEAB0` | `0x00691CA0` | CalmEffect | `0x00A753A4` | `0x00691CF2` |  |
| 2 | CHML | `0x009FEAD5` | `0x009FEAD0` | `0x00692040` | ChameleonEffect | `0x00A753F4` | `0x00692092` |  |
| 3 | COCR | `0x009FEAF5` | `0x009FEAF0` | `0x00692300` | CommandCreatureEffect | `0x00A75444` | `0x00692352` |  |
| 4 | COHU | `0x009FEB15` | `0x009FEB10` | `0x006928D0` | CommandHumanoidEffect | `0x00A754D4` | `0x00692922` |  |
| 5 | DARK | `0x009FEB35` | `0x009FEB30` | `0x00692C90` | DarknessEffect | `0x00A75564` | `0x00692CE2` |  |
| 6 | DEMO | `0x009FEB55` | `0x009FEB50` | `0x00692E40` | DemoralizeEffect | `0x00A755B4` | `0x00692E94` |  |
| 7 | DTCT | `0x009FEB75` | `0x009FEB70` | `0x00693450` | DetectLifeEffect | `0x00A755FC` | `0x006934A2` |  |
| 8 | DIAR | `0x009FEB95` | `0x009FEB90` | `0x00693550` | DisintegrateArmorEffect | `0x00A7564C` | `0x006935A4` |  |
| 9 | DIWE | `0x009FEBB5` | `0x009FEBB0` | `0x00693880` | DisintegrateWeaponEffect | `0x00A756C4` | `0x006938D2` |  |
| 10 | DSPL | `0x009FEBD5` | `0x009FEBD0` | `0x00693A30` | DispelEffect | `0x00A7573C` | `0x00693A82` |  |
| 11 | FRNZ | `0x009FEBF5` | `0x009FEBF0` | `0x00693DD0` | FrenzyEffect | `0x00A75784` | `0x00693E24` |  |
| 12 | INVI | `0x009FEC15` | `0x009FEC10` | `0x00693FE0` | InvisibilityEffect | `0x00A757D4` | `0x00694032` |  |
| 13 | LGHT | `0x009FEC35` | `0x009FEC30` | `0x00694A30` | LightEffect | `0x00A75824` | `0x0069445F` | SEH付きファクトリラッパ、本 ctor=0x00694420 |
| 14 | LOCK | `0x009FEC55` | `0x009FEC50` | `0x00694BD0` | LockEffect | `0x00A758F4` | `0x00694C22` |  |
| 15 | NEYE | `0x009FECE5` | `0x009FECE0` | `0x006A2E30` | NightEyeEffect | `0x00A768A4` | `0x006A2E82` |  |
| 16 | OPEN | `0x009FED05` | `0x009FED00` | `0x006A3550` | OpenEffect | `0x00A76984` | `0x006A35A2` |  |
| 17 | PARA | `0x009FED25` | `0x009FED20` | `0x006A37B0` | ParalysisEffect | `0x00A769CC` | `0x006A3802` |  |
| 18 | REAN | `0x009FED45` | `0x009FED40` | `0x006A4030` | ReanimateEffect | `0x00A76A1C` | `0x006A39C7` | SEH付きファクトリラッパ、本 ctor=0x006A3980 |
| 19 | STRP | `0x009FED65` | `0x009FED60` | `0x006A4D10` | SoulTrapEffect | `0x00A76B44` | `0x006A4D62` |  |
| 20 | SUDG | `0x009FED85` | `0x009FED80` | `0x006A6890` | SunDamageEffect | `0x00A76C54` | `0x006A68E9` |  |
| 21 | TELE | `0x009FEDA5` | `0x009FEDA0` | `0x006A79B0` | TelekinesisEffect | `0x00A76C9C` | `0x006A73F2` | SEH付きファクトリラッパ、本 ctor=0x006A73B0 |
| 22 | TURN | `0x009FEDC5` | `0x009FEDC0` | `0x006A7F30` | TurnUndeadEffect | `0x00A76CEC` | `0x006A7F84` |  |
| 23 | VAMP | `0x009FEDE5` | `0x009FEDE0` | `0x006A8AD0` | VampirismEffect | `0x00A76D84` | `0x006A8B22` |  |

- s91 (`s91_magic_m3_mgef_dispatch.md`) との突合: **ctor 値 23/23 が完全一致**。クラス名も 23/23 一致。
- LGHT / REAN / TELE の登録値は SEH 付きの new-wrapper (operator new 0x401F00 → 本 ctor 呼び出し)。vtable は本 ctor 内で設定され、クラス名はそちらから同定 (LightEffect / ReanimateEffect / TelekinesisEffect)。

## 4. 「登録23 ⊂ ESM145」検証結果

- 登録23 code のうち、ESM MGEF 145件の EDID に存在しないもの: **0 件** → **一致率 23/23 = 100%**
- 逆方向の差分 (EXE 側にのみ存在する code = **16 件**): `BA01`, `BA02`, `BA03`, `BA04`, `BA05`, `BA06`, `BA07`, `BA08`, `BA09`, `BA10`, `BW09`, `BW10`, `DISE`, `DUMY`, `RSWD`, `Z020`  **【確定 (V39, 2026-10-24)】正体**: 全16件は `FUN_00417420` の組み込み EffectSetting 登録（161件）に所在。EFID 参照8件（BA01-06/DUMY/RSWD）＝フォールバック供給、純予備8件（BA07-10/BW09/BW10/Z020）、`DISE`＝実行時 lookup 専用（`FUN_00419b30`, type==1, 対抗 CUDI）。`DUMY`=Mehrunes Dagon Custom Effect／`RSWD`=Resist Water Damage。出典 `tmp/re/v39_exe_only_codes.md`
  - ESM リーダーテーブル F0-F4 には登録されているが、Oblivion.esm に対応する MGEF レコードが無い (未使用定義/内部用/カット内容)。Z020 は Z001-Z019 の範囲外、BW09/BW10 は BW01-BW08 の範囲外、BA01-BA10 は ESM の Bound Armor 系 (BABO/BACU/BAGA/BAGR/BAHE/BASH) とは別系統。 **【訂正 (V39, 2026-10-24)】**「未使用定義/内部用/カット内容」の推測は**撤回**: 供給元は EXE 内蔵登録（フォールバック8件／純予備8件／DISE は実行時 lookup）で確定。

## 5. 未登録122種の disposition 内訳

| disposition | 件数 | 定義 |
|-------------|------|------|
| registered_ctor | 23 | 登録テーブル23組に該当 (§3) |
| cmp_branch | 8 | 未登録だが `0x413A90` の cmp 分岐に載る (§6)。cpp_branch_va に一致時ターゲットを記載 |
| default_path | 114 | ESM リーダーテーブル F0-F4 に登録あり。専用クラス ctor が無いため汎用 ActiveEffect 経由の既定処理に振り分け |
| undetermined | 0 | EXE 内に fourcc 参照が一切無し |

- 未登録122種 = cmp_branch 8 + default_path 114 + undetermined 0。**全145 code が EXE 内に参照を持つ**。
- cmp_branch 8件: `ABAT DGAT DRAT FOAT REAT ABSK DRSK FOSK` (attribute/skill 系の8コード、いずれも reader テーブルにも併載)。
- default_path 代表例:

| code | full_name | ref_reader_tables | 備考 |
|------|-----------|-------------------|------|
| ABHE | Absorb Health | `F1@0x00417678;F1@0x00417790;F1@0x00417AC9;F2@0x00417FCB;F2@0x00417FEB` | s91: 吸収系ダメージタイプ (0x6A8272 に線形比較) |
| FIDG | Fire Damage | `F1@0x004175C8` | s91: ダイレクト効果タイプ (0x695631) |
| SLNC | Silence | `F2@0x00417B96` | s91: 特殊ハンドラ (0x5EE630) |
| SEFF | Script Effect | `F4@0x00418D2E` | s91: 別の初期化経路 (0x413094 他)、dispatcher 0x6A2CF0 で特別扱い |
| BW08 | Bound Priest Dagger | `F4@0x00418C8B` | Bound Priest Dagger、actor_value はリンク FormID |
| Z019 | Summon Sewn Flesh Atronach | `F4@0x004187E1` | Summon Flesh Atronach 系、actor_value はリンク FormID |
| CHRM | Charm | `F2@0x00417BC0` | actor_value=6 (Personality) |

## 6. `0x413A90` の cmp 8件 — 実測 jcc と分岐先

- 関数 `0x413A90` は capstone リニア解析で 86 命令 (`0x413A90`-`0x413BA4`)。`mov eax,[ecx]` の fourcc に対する二分探索型ディスパッチ。
- 注意: 全てが `jne` というわけではなく `jg`/`je`/`jne` が混在する (二分探索木)。下表の「一致時分岐」は `je` のターゲット (無ければ `jne` の fall-through)。

| code | full_name | cmp VA | jcc 実測 | 一致時分岐 VA (= CSV cpp_branch_va) |
|------|-----------|--------|----------|-------------------------------------|
| REAT | Restore Attribute | `0x00413A95` | jg@0x413A9B → `0x413B24`; je@0x413AA1 → `0x413AFA` | `0x00413AFA` |
| DRSK | Drain Skill | `0x00413AA3` | jg@0x413AA8 → `0x413AE8`; je@0x413AAA → `0x413B72` | `0x00413B72` |
| ABSK | Absorb Skill | `0x00413AB0` | je@0x413AB5 → `0x413AEF` | `0x00413AEF` |
| FOSK | Fortify Skill | `0x00413AB7` | jne@0x413ABC → `0x413B39` | `0x00413ABE` |
| ABAT | Absorb Attribute | `0x00413AE8` | jne@0x413AED → `0x413B39` | `0x00413AEF` |
| DGAT | Damage Attribute | `0x00413B24` | je@0x413B29 → `0x413B7A` | `0x00413B7A` |
| FOAT | Fortify Attribute | `0x00413B2B` | je@0x413B30 → `0x413ABE` | `0x00413ABE` |
| DRAT | Drain Attribute | `0x00413B32` | je@0x413B37 → `0x413B72` | `0x00413B72` |

- 一致時分岐先は **5種類** に集約される: `0x413AFA` (REAT) / `0x413B72` (DRSK, DRAT) / `0x413AEF` (ABSK, ABAT) / `0x413ABE` (FOSK, FOAT) / `0x413B7A` (DGAT)。skill 系と attribute 系が同一コードブロックを共有。
- 分岐先コードブロックの実体: `[ecx+0x14]` を `0x565CC0` で解決後、書式文字列 `'%s %s'` (`0xA32564`) を渡して `0x98208B` を呼ぶ → **文字列組み立て系 (表示/検証) の可能性が高く、実行時エフェクト適用ハンドラではない** (§8)。
- 8件はいずれも登録23に含まれない (registered=no)。145件中、この8 code が cmp_branch として対応。

## 7. school / actorValue 分布

### school (DATA off12)

| school | 名称 | 件数 |
|--------|------|------|
| 0 | Alteration | 11 |
| 1 | Conjuration | 62 |
| 2 | Destruction | 25 |
| 3 | Illusion | 14 |
| 4 | Mysticism | 7 |
| 5 | Restoration | 26 |
| - | 合計 | 145 |

### actorValue (DATA off8、閾値 200 で分割)

| 区分 | 件数 |
|------|------|
| 0 = special (LOCK/OPEN/DSPL/TELE/VAMP/REAN/TURN 等) | 26 |
| 6 = Personality (CHRM) | 1 |
| 8 = Health | 8 |
| 9 = Magicka | 5 |
| 10 = Fatigue | 5 |
| 11 = Speed (BRDN/FTHR) | 2 |
| 33-70 = skills/attributes | 38 |
| 101-200 = (該当なし) | 0 |
| >200 = FormID リンク | 60 |

- **FormID リンク (>200) = 60 件、全て school=1 (Conjuration)**。例: BW08 → actor_value=606528 (`0x00094140` = Bound Priest Dagger のリンク先フォーム), Z019 → 523715 (`0x0007FDC3` = Summon 系のリンク先フォーム)。
- 既知情報「FormID リンク Conjuration 62件」の精査: school=1 の MGEF は **62件** だが、そのうち FormID リンクは **60件**。残り2件は **REAN (Reanimate) と TURN (Turn Undead)** で actor_value=0 (special)。→ 「62件」は Conjuration MGEF の総数であり、FormID リンク数は60件が実測値。

## 8. 判定できなかった項目と理由

1. **未登録122種の実行時適用ハンドラの個別同定**: F0-F4 は ESM 読み込み時の効果定義構築テーブルであり、実行時の適用は dispatcher `0x6A2CF0` + ActiveEffect 汎用経路が担う (s91 の結論と整合)。本スコープ (登録テーブル + リーダー4テーブル + cmp8) では非登録 code の個別適用処理までは特定できず → `default_path` で確定 (undetermined=0)。
2. **`0x413A90` の意味論**: 分岐先 VA は実測で確定済みだが、各ブロックが `'%s %s'` 書式の文字列組み立て (`0x98208B` 呼び出し) であるため、適用ハンドラではなく表示/検証系の可能性が高い。呼び出し元の特定は未実施。
3. **LGHT/REAN/TELE の登録値は new-wrapper**: 登録テーブルの VA は SEH 付きファクトリ (例 LGHT `0x694A30` → 本 ctor `0x694420`)。ctor_va 列には登録実値 (wrapper) を記載し、本 ctor は §3 備考に記載。
4. **EXE-only 16 code の性格** (BA01-BA10 / BW09 / BW10 / DISE / DUMY / RSWD / Z020): ESM MGEF に該当レコードが無いため本表145行には現れない。未使用定義か別システム用かは未解析。 **【訂正 (V39, 2026-10-24)】**「未使用定義か別システム用か」の二択は**撤回して確定**: BA01-06/DUMY/RSWD=EFID 参照ありのフォールバック供給、BA07-10/BW09-10/Z020=純予備スロット、DISE=実行時 lookup 専用。本表145行に現れない理由＝ESM MGEF 定義が無いだけでシステムとしては実在。
5. **F0 領域の関数先頭**: F0 (`0x417438`-`0x417551`, 11 push: WABR/WAWA/SHLD/FISH/LISH/FRSH/FTHR/DSPL/BRDN/DSPL/FTHR) は既知4テーブルより前の第5領域。先頭 push 位置から境界を設定しており、関数の真の先頭は僅かに前 (int3 パディング) の可能性。
6. **push 件数の既知値との差異の説明**: F1=105 / F2=69 は prompt 値と一致。F3=39 / F4=55 は既知 31/23 と異なるが、これは既知値が **英字のみ** の fourcc を数えたため (差分 = 数字を含む code: F3 の Z001-Z008 の8件、F4 の Z009-Z020/BA01-10/BW01-10 の32件)。v24 の表 A=94 は tail 11件 (VAMP..DSPL) の取りこぼしで、本測定 105 が正しい。
7. **補助テーブル**: v24 が検出した `0x6126B0`(21) / `0x613880`(6) / `0x68EA50`(10) の補助 push テーブルは本表の対象外 (ref_reader_tables は F0-F4 のみ)。

---

## 付録: 測定手法と CSV 列定義

- **登録テーブル走査**: `0x9FEA90`-`0x9FF100` を `68 <[A-Z0-9]{4}>` で走査し、5バイト前が `68 <.text内 imm32>`、+5 が `E8` (call 目的地 = `0x68DA10`) である23組を抽出。
- **リーダーテーブル走査**: `0x417300`-`0x419600` の `push fourcc` を5領域に分割: F0=`[0x417438,0x417551)` 11件 / F1=`[0x417551,0x417AF0)` 105件 / F2=`[0x417AF0,0x4180C9)` 69件 / F3=`[0x4180C9,0x418651)` 39件 (英字31) / F4=`[0x418651,0x418DAB)` 55件 (英字23)。計 279 push。
- **cmp 分岐**: capstone で `0x413A90` をリニア解析し、`cmp eax,<fourcc>` の従続 jcc のターゲットを実測。
- **ctor→クラス**: 登録 ctor 先頭 0x100 の `C7 /0` vtable ストアを走査 (無ければ E8 内部コール先を 0x140 探索)。imm32 は .rdata 範囲に限定し `rtti_vtables.txt` (2,172件) / `s57_magic_ctormap.txt` (55件) で逆引き。23/23 同定。
- **ESM 走査**: レコードヘッダ 20B / GRUP ヘッダ 20B (size 自身込み) / サブレコード 6B (tag4+u16)。GRUP はスタック降下。圧縮レコードは flag 0x40000 検出後 zlib 展開。XXXX は real size (u32) で次サブレコードを飛ばす。
- **CSV 列**: `code,esm_formid,full_name,school,actor_value,flags,data_size,registered,ctor_va,ref_reader_tables,ref_cmp_dispatch,cpp_branch_va,rtti_class,disposition`
  - `actor_value` は DATA off8 の生値 (decimal)。>200 はリンク FormID (§7 で解釈)。
  - `ref_reader_tables` は `領域@pushVA` のセミコール区切り (複数回出現を全て列挙)。
  - `cpp_branch_va` は `0x413A90` の一致時分岐先 (§6)。
