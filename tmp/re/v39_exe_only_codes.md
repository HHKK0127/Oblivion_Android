# V39 — EXE リーダ表のみに存在する 16 MGEF コードの正体特定

> 作成日: 2026-10-24 / 対象: `Oblivion.exe` (Steam v1.2.0416, ImageBase=0x400000)
> 解析対象16コード: `BA01`–`BA10`, `BW09`, `BW10`, `DISE`, `DUMY`, `RSWD`, `Z020`
> 手法: 自作 PE パーサ + capstone 5.0.7 バイナリ走査 / `docs\Oblivion_decompiled.c` (1,066,496行) グレップ / 実 ESM 再走査
> スクリプト: `%TEMP%\opencode\v39_step1_selftest.py` / `v39_step2_scan.py` / `v39_step2b_utf16.py` / `v39_step4_decode.py` / `v39_step6b_esm.py`

## 0. 結論（先に要点）

**16 コードは全て、EXE 起動時に組み込まれる「組み込み EffectSetting 登録表」(`FUN_00417420` @0x417420 → `FUN_00417220` @0x417220 → 全局マップ `0x00B33508`) の一部**。デコンパイル上の表示名ラベルがコードの正体を直接示す:

| コード | EXE 内蔵表示名（=正体） |
|---|---|
| BA01–BA10 | **Bound Armor Extra 01–10**（追加縛り鎧スロット） |
| BW09, BW10 | **Bound Weapon Extra 09, 10**（追加縛り武器スロット） |
| Z020 | **Extra Summon 20**（追加召喚スロット 20 番目） |
| DISE | **Disease Info**（疾患表示用マーカー効果） |
| DUMY | **Mehrunes Dagon Custom Effect**（ダゴン戦用カスタムダメージ効果） |
| RSWD | **Resist Water Damage**（水ダメージ耐性） |

バニラ ESM 参照の実態: **BA01–BA06 / DUMY / RSWD の 8 コードはバニラ呪文の EFID が参照するが MGEF 定義レコードが無い** → 定義は EXE 内蔵登録が供給（フォールバック）。**BA07–BA10 / BW09 / BW10 / Z020 は ESM 参照ゼロ・コード参照ゼロの予備スロット**。**DISE は ESM 参照ゼロだが EXE コードが実行時 lookup する**（疾患アイテム表示）。

---

## 1. セルフテスト 4 件（全 PASS）

| # | 検査 | 期待 | 実測 | 結果 |
|---|---|---|---|---|
| T1 | `bytes@VA0x504050` (off=0x103450, .text) | `518b4c24188b5424` | `518b4c24188b5424` | **PASS** |
| T2 | `bytes@VA0x4FB580` (off=0xFA980, .text) | `578b7908` | `578b7908` | **PASS** |
| T3 | `string@VA0xA7035C` (off=0x66F55C, .rdata) | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | **PASS** |
| T4 | オペコードテーブル 基底0xB0AF48 stride0x28: opid `0x0015` の impl | `0x504560` | entry#21 @0xB0B290 impl=0x504560 | **PASS** |

---

## 2. fourcc のバイナリ出現走査（全セクション・全走査）

- 走査方式: ファイル全体を 4 バイト ASCII パターン（=`u32 リトルエンディアン即値` と同一バイト列）で linear search。ASCII 文字列出現と `push imm32` 即値出現を一括検出。
- 補助走査: UTF-16LE パターン（`B\x00A\x000\x001\x00` 等）→ **16 コードとも 0 件**。
- NUL 終端 ASCII 文字列（`\0` 境界）→ **0 件**（fourcc が独立文字列として .rdata に存在しない）。

**結果: 出現は全て `.text` のみ（.rdata 0 件 / .data 0 件）。総出現 18 箇所。**

| コード | u32 LE | 件数 | 出現 VA（fourcc バイト位置） | 性質 |
|---|---|---|---|---|
| BA01 | 0x31304142 | 1 | 0x418A17 | `push imm32`（登録呼び出し） |
| BA02 | 0x32304142 | 1 | 0x418A3C | 同上 |
| BA03 | 0x33304142 | 1 | 0x418A61 | 同上 |
| BA04 | 0x34304142 | 1 | 0x418A86 | 同上 |
| BA05 | 0x35304142 | 1 | 0x418AAB | 同上 |
| BA06 | 0x36304142 | 1 | 0x418AD0 | 同上 |
| BA07 | 0x37304142 | 1 | 0x418AF5 | 同上 |
| BA08 | 0x38304142 | 1 | 0x418B1A | 同上 |
| BA09 | 0x39304142 | 1 | 0x418B3F | 同上 |
| BA10 | 0x30314142 | 1 | 0x418B64 | 同上 |
| BW09 | 0x39305742 | 1 | 0x418CB1 | 同上 |
| BW10 | 0x30315742 | 1 | 0x418CD6 | 同上 |
| DISE | 0x45534944 | 2 | 0x417ABB / **0x419B71** | 登録呼び出し / **実行時 lookup** |
| DUMY | 0x594D5544 | 2 | 0x417AF4 / **0x417FBD** | 登録呼び出し / **Absorb Health 対抗リスト引数** |
| RSWD | 0x44575352 | 1 | 0x4181AB | 登録呼び出し |
| Z020 | 0x3032305A | 1 | 0x418807 | 登録呼び出し |

**出現の性質の総括: 「cmp 群」でも「テーブル定数配列」でもなく、`push imm32` 即値（登録ヘルパへの引数）が支配的**（18/18）。登録呼び出しは全て同一関数 `FUN_00417420`（旧呼称「ESM リーダ表 0x417551/0x417AF0/0x4180C9/0x418651 群」の正体）に属する。唯一の例外は DISE の実行時 lookup（0x419B71）と DUMY の対抗リスト引数（0x417FBD）。

---

## 3. decompiled.c 内の出現

- `"BA01"` 等の**文字列リテラルとしての出現: 0 件**（fourcc は `0x31304142` 形の整数リテラル）。
- `FUN_00417220(0x` 登録呼び出し: **161 件**（`FUN_00417420` @0x417420、L15717–L15896）。161 = v35 の 145（ESM に MGEF 定義あり）+ 今回の 16。
- 16 コードの登録行（表示名つき・抜粋）:

```c
// L15766  DISE
FUN_00417220(0x45534944,"Disease Info",2,0,0,0x800000,0x3f,1,0x49445543);
// L15767  DUMY  (引数続きは L15768)
FUN_00417220(0x594d5544,"Mehrunes Dagon Custom Effect",2,0,8,0x21000075,0x40,4,0x4c505344, ...);
// L15814  RSWD
FUN_00417220(0x44575352,"Resist Water Damage",5,0,0x47,0x100017a,0xffffffff,0);
// L15858  Z020
FUN_00417220(0x3032305a,"Extra Summon 20",1,0,0,0x40112,0xffffffff,0);
// L15872-L15881  BA01-BA10
FUN_00417220(0x31304142,"Bound Armor Extra 01",1,0,0,0x20112,0xffffffff,0);
...  // "Bound Armor Extra 02".."10"、flags は全て 0x20112
// L15890/L15891  BW09/BW10
FUN_00417220(0x39305742,"Bound Weapon Extra 09",1,0,0,0x10112,0xffffffff,0);
FUN_00417220(0x30315742,"Bound Weapon Extra 10",1,0,0,0x10112,0xffffffff,0);
```

- 2 箇所目の出現:
  - **DUMY @L15801**: `Absorb Health` 登録（L15800）の可変長「対抗/連鎖エフェクト」引数の末尾 = `...,0x4c505344,0x45484241,0x45485244,0x45484744,0x594d5544`（DSPL/ABHE/DRHE/DGHE/**DUMY**）。
  - **DISE @L16541**: `FUN_00419b30` 内 `iVar4 = FUN_00416870(0x45534944);`（実行時 lookup）。

---

## 4. 登録機構の全体像と capstone デコード

### 4.1 アーキテクチャ

```
起動時: FUN_00418db0 @0x418db0 (global ctor, L15904)  ─┐
         (再初期化経路: FUN_00418e50 @0x418e50, L79065 呼出) ─┤
                                                          v
                                       FUN_00417420 @0x417420  (161件の組み込み登録)
                                                          │ 各 call
                                                          v
 FUN_00417220 @0x417220: EffectSetting(0xa8B) 生成:
   +0x98 = fourcc (param_1) / 名前 = param_2 (.rdata 文字列) / +0x64 = カテゴリ(param_3)
   +0x5c,+0x60 = param_4,param_5 / +0x58 = flags(param_6) / +0x68 = actorValue(param_7)
   対抗リスト = param_8 以降 (FUN_00415d40)
                                                          │ insert
                                                          v
                        全局 MGEF マップ @0x00B33508  (FUN_00452570 @0x452570)
                                                          ^
実行時 lookup: FUN_00416870 @0x416870 → FUN_0055e000 @0x55e000 (hash find)
```

- 登録対象 161 件のうち 145 件は ESM の MGEF レコード（EDID=fourcc, FormID 付与 = v35 全 145 行）と対応し、**残る 16 件が本文の対象**（= ESM に MGEF 定義レコードが存在しない唯一の組み込み登録）。
- マップ本体: `DAT_00b33508`(vtable/ハッシュ) / `DAT_00b3350c`(bucket数) / `DAT_00b33510`(bucket配列) — `FUN_004167e0` @0x4167e0 が全走査イテレータ。

### 4.2 capstone デコード（代表 4 箇所）

**(1) BA01 登録呼び出し @0x418A00（`FUN_00417420` 内・fourcc=即値 push、cmp ではない）**

```
0x00418A00: push     0              ; param_8 tail = 0 (対抗なし)
0x00418A02: push     -1             ; param_7 actorValue = 0xFFFFFFFF
0x00418A04: push     0x20112        ; param_6 flags (Bound Armor 系と同一値)
0x00418A09: push     0              ; (param_5 float 0.0 を ecx/fstp 経由で)
0x00418A0B: push     ecx
0x00418A0C: fstp     dword ptr [esp]
0x00418A0F: push     1              ; param_3 カテゴリ=1 (Summon/Bound 系)
0x00418A11: push     0xa3336c       ; param_2 = "Bound Armor Extra 01" (.rdata)
0x00418A16: push     0x31304142     ; param_1 = 'BA01'
0x00418A1B: call     0x417220       ; FUN_00417220
```

登録名ポインタ（.rdata）: BA01=0xA3336C, BA02=0xA33354, BA03=0xA3333C, BA04=0xA33324, BA05=0xA3330C, BA06=0xA332F4, BA07=0xA332DC, BA08=0xA332C4, BA09=0xA332AC, BA10=0xA33294（各 0x18 間隔）, BW09=0xA331BC, BW10=0xA331A4, Z020=0xA33448, RSWD=0xA3375C, DISE=0xA339EC, DUMY=0xA339CC。

**(2) DISE 実行時 lookup @0x419B55（`FUN_00419b30` @0x419b30 内）**

```
0x00419B62: mov      eax, dword ptr [esi]        ; vtable
0x00419B64: mov      edx, dword ptr [eax + 0x18] ; GetObjectType()
0x00419B67: mov      ecx, esi
0x00419B69: call     edx
0x00419B6B: cmp      eax, 1                      ; type == 1 (Disease 呪文)
0x00419B6E: jne      0x419b82
0x00419B70: push     0x45534944                  ; 'DISE'
0x00419B75: call     0x416870                    ; FUN_00416870 = EffectSetting lookup
0x00419B7A: add      esp, 4
0x00419B7D: pop      esi
0x00419B7E: pop      ecx
0x00419B7F: ret      4
```

`FUN_00419b30` は type==5 または (type==7 かつ `FUN_00414eb0()`) なら `PONI`(Poison Info, 0x4E534F50)、type==1 なら `DISE` を返す。**DISE はバニラ実行時コードが参照する実在効果**。

**(3) DUMY の 2 箇所目 @0x417FBC（Absorb Health の対抗リスト先頭 push = 引数の末尾要素）**

```
0x00417FBC: push     0x594d5544   ; 'DUMY'  (vararg 末尾 = 対抗5個目)
0x00417FC1: push     0x45484744   ; 'DGHE'
0x00417FC6: push     0x45485244   ; 'DRHE'
0x00417FCB: push     0x45484241   ; 'ABHE'
0x00417FD0: push     0x4c505344   ; 'DSPL'
0x00417FD5: push     4            ; param_8 = 対抗数
```

**(4) lookup/insert 実装 @0x416870 / @0x4173DA**

```
; FUN_00416870 (fourcc lookup)
0x0041687A: mov      ecx, 0xb33508     ; 全局 MGEF マップ
0x00416887: call     0x55e000          ; FUN_0055e000 hash-find → *out = found EffectSetting*
; FUN_00417220 tail (insert)
0x004173D5: call     0x415d40          ; 対抗リスト構築
0x004173DC: mov      ecx, 0xb33508     ; 同一マップ
0x004173E1: call     0x452570          ; FUN_00452570 map-insert
```

→ **登録と lookup が同一マップ `0x00B33508` を共有**。ESM の MGEF レコード 145 件も同じ fourcc 空間に載る（v35 の `ref_reader_tables` 列が `FUN_00417420` の push 群を指す）ため、**EXE 内蔵登録＝既定値、ESM MGEF レコード＝上書き/供給元**という 2 層構造が確定。

---

## 5. 16 コード分類表

分類ラベル: (a)=読取テーブル（ESM 記録の仮想フォールバック/既定値） / (b)=未実装/ダミーのプレースホルダ / (c)=ゲーム内生成専用（ESM 非収録で実行時生成/参照） / (d)=その他

| # | コード | EXE 表示名 | カテゴリ/flags/AV | 分類 | 根拠 |
|---|---|---|---|---|---|
| 1 | BA01 | Bound Armor Extra 01 | 1 / 0x20112 / -1 | **(a) 既定値** | ESM の SI 呪文 `SEAbOrderKnight1BoundArmorWeapon` (0x00041F37) が EFID 参照するが MGEF 定義なし → 定義は内蔵登録が供給。flags は BABO(Bound Boots) 系と同一 |
| 2 | BA02 | Bound Armor Extra 02 | 1 / 0x20112 / -1 | **(a) 既定値** | `SEAbOrderKnight2BoundArmorWeapon` (0x00041F1B) 同上 |
| 3 | BA03 | Bound Armor Extra 03 | 1 / 0x20112 / -1 | **(a) 既定値** | `SEAbOrderKnight3BoundArmorWeapon` (0x00041F38) 同上 |
| 4 | BA04 | Bound Armor Extra 04 | 1 / 0x20112 / -1 | **(a) 既定値** | `SEAbOrderKnight4BoundArmorWeapon` (0x00041F39) 同上 |
| 5 | BA05 | Bound Armor Extra 05 | 1 / 0x20112 / -1 | **(a) 既定値** | `SEAbOrderKnight5BoundArmorWeapon` (0x00041F3A) 同上 |
| 6 | BA06 | Bound Armor Extra 06 | 1 / 0x20112 / -1 | **(a) 既定値** | `SEAbOrderKnight6BoundArmorWeapon` (0x00041F3B) 同上 |
| 7 | BA07 | Bound Armor Extra 07 | 1 / 0x20112 / -1 | **(d) 予備スロット** | 登録のみ。ESM EFID 0 件・MGEF EDID 0 件・EXE コード参照 0 件 |
| 8 | BA08 | Bound Armor Extra 08 | 1 / 0x20112 / -1 | **(d) 予備スロット** | 同上 |
| 9 | BA09 | Bound Armor Extra 09 | 1 / 0x20112 / -1 | **(d) 予備スロット** | 同上 |
| 10 | BA10 | Bound Armor Extra 10 | 1 / 0x20112 / -1 | **(d) 予備スロット** | 同上 |
| 11 | BW09 | Bound Weapon Extra 09 | 1 / 0x10112 / -1 | **(d) 予備スロット** | 登録のみ（flags は BWAX 系と同一）。ESM は BW01–08 まで定義・参照あり、09/10 は不在 |
| 12 | BW10 | Bound Weapon Extra 10 | 1 / 0x10112 / -1 | **(d) 予備スロット** | 同上 |
| 13 | DISE | Disease Info | 2 / 0x800000 / 0x3F(DiseaseResist) | **(c) 実行時生成/参照専用** | ESM 参照ゼロ。`FUN_00419b30` が type==1(疾患) アイテムに対して実行時 lookup（0x419B70）。対抗=CUDI。Poison Info(PONI) の対になる情報表示用マーカー |
| 14 | DUMY | Mehrunes Dagon Custom Effect | 2 / 0x21000075 / 0x40 | **(a) 既定値** | バニラ MQ16 ダゴン戦呪文 `MQ16DagonAreaAttack1..5`（15 EFID 参照）が使用、MGEF 定義なし。flags は DGHE(Damage Health) と同一。Absorb Health の対抗リストにも登録 |
| 15 | RSWD | Resist Water Damage | 5 / 0x100017A / 0x47 | **(a) 既定値** | バニラ `SESheogorathImmunity`(0x00095266) / `AbImmuneToWaterDamage`(0x00066C43) が EFID 参照、MGEF 定義なし |
| 16 | Z020 | Extra Summon 20 | 1 / 0x40112 / -1 | **(d) 予備スロット** | 登録のみ。Z001–Z019 は ESM に MGEF 定義+参照あり、Z020 のみ不在 |

---

## 6. 既存解釈の支持/否定（v24 / v32 / v35 と突合）

| 既存解釈 | 判定 | EXE 証拠 |
|---|---|---|
| BA01–BA10 = Bound Armor? | **支持（さらに精緻化）** | 登録表示名 "Bound Armor Extra 01–10"、カテゴリ=1、flags=0x20112（BABO–BASH と同一）。ただしバニラ使用は BA01–06 のみ（SI Order Knight 呪文）、BA07–10 は予備 |
| BW09/BW10 = Bound Weapon? | **支持** | 登録表示名 "Bound Weapon Extra 09/10"、flags=0x10112（BWAX/BWSW 系と同一）。バニラ不使用 |
| DISE = Disease? | **支持（精緻化）** | 登録表示名 "Disease Info"、AV=0x3F(DiseaseResist)、対抗=CUDI(Cure Disease)。疾患表示/情報用マーカーであり、`FUN_00419b30` が実行時取得 |
| DUMY = Dummy? | **否定** | 表示名は "Mehrunes Dagon Custom Effect"。Damage Health 系 flags(0x21000075)、バニラ MQ16 ダゴン戦で 15 箇所の実使用。ダミーではなく実装済みのカスタムダメージ効果 |
| RSWD = Readied Spell/Weapon? | **否定** | 表示名は "Resist Water Damage"。AV=0x47、Resist 系 flags、バニラの水ダメ免疫アビリティ 2 件が実使用。「耐性」系であり Readied ではない |
| Z020 = Summon 系 Z コード? | **支持** | 登録表示名 "Extra Summon 20"、flags=0x40112（Z001–Z019 と同一）。ただしバニラ不使用の予備 1 枠 |

補足（v24 の既存分類との差分）: v24 の 115 コード集合は DUMY=`MGEF-other`・RSWD=`MGEF-resist` としており方向性は正しいが、**BA01–BA10/BW09/BW10/Z020/DISE の 16 コードは v24 集合に含まれていなかった**（Z 名空間は ZCLA 等の文字コード 20 種、BA/BW 名空間は BABO 等の 13 種のみ）。本タスクで「数値系 Extra コード + DISE」が同一の `FUN_00417420` 登録テーブルに居ることが確定した。

---

## 7. CSV 突合表（v32 / v35 実読み + 本次 ESM 再走査）

| コード | v32 ESM EFID（件数/記録型） | v32 §6.3 MGEF EDID | v35（145 行・esm_formid 付与） | 本次 ESM 再走査（FormID/EDID） |
|---|---|---|---|---|
| BA01 | 1 / SPEL | なし | なし | SPEL 0x00041F37 `SEAbOrderKnight1BoundArmorWeapon` |
| BA02 | 1 / SPEL | なし | なし | SPEL 0x00041F1B `SEAbOrderKnight2BoundArmorWeapon` |
| BA03 | 1 / SPEL | なし | なし | SPEL 0x00041F38 `SEAbOrderKnight3BoundArmorWeapon` |
| BA04 | 1 / SPEL | なし | なし | SPEL 0x00041F39 `SEAbOrderKnight4BoundArmorWeapon` |
| BA05 | 1 / SPEL | なし | なし | SPEL 0x00041F3A `SEAbOrderKnight5BoundArmorWeapon` |
| BA06 | 1 / SPEL | なし | なし | SPEL 0x00041F3B `SEAbOrderKnight6BoundArmorWeapon` |
| BA07 | 0（行なし） | なし | なし | 0 件 |
| BA08 | 0（行なし） | なし | なし | 0 件 |
| BA09 | 0（行なし） | なし | なし | 0 件 |
| BA10 | 0（行なし） | なし | なし | 0 件 |
| BW09 | 0（行なし） | なし | なし | 0 件 |
| BW10 | 0（行なし） | なし | なし | 0 件 |
| DISE | 0（行なし） | なし | なし | 0 件 |
| DUMY | 15 / SPEL | なし（v32 が「EXE にあって MGEF EDID に無い 3 種」に列挙） | なし | SPEL 0x00165B38/0x00176190–93 `MQ16DagonAreaAttack1..5`（各 3 EFID） |
| RSWD | 2 / SPEL | なし（同上 3 種に列挙） | なし | SPEL 0x00095266 `SESheogorathImmunity` / 0x00066C43 `AbImmuneToWaterDamage` |
| Z020 | 0（行なし） | なし | なし | 0 件 |

- v32 実 ESM 走査の EFID 列（`mgef` 列）は 16 コード全て 0 → **ESM の MGEF 効果定義レコード（EDID=fourcc）に 16 コードは存在しない**（145 レコード全てが v35 で esm_formid 付与済み、16 は除外）。
- したがって「EXE リーダ表のみ」の正確な内訳は:
  - **EFID 参照はあるが MGEF 定義が無い**（= 定義供給元が EXE 内蔵登録のみ）: BA01–BA06, DUMY, RSWD（8 コード）
  - **ESM のどこにも無い**（純粋な予備/実行時専用）: BA07–BA10, BW09, BW10, DISE, Z020（8 コード）
- 以前の「16 コードは ESM に存在せず」という表現は、**「ESM の MGEF 定義(EDID)に存在しない」が正確**（EFID 参照は 8 コードにある）。

---

## 8. 未解決点

1. `FUN_00419b30` の 28 呼び出し箇所の用途別内訳（DISE が具体的にどの UI/処理で使われるか：マジックメニュー表示・疾患継続処理等の特定）は未細分。
2. BA07–BA10 / BW09 / BW10 / Z020 を Mod/CS 経由で EFID 参照した場合の挙動（汎用 Bound/Summon ハンドラが associated item 無しで何を生成するか）は未検証。
3. `FUN_00417220` の param_3（0–6）の正確な名称（カテゴリ/ハンドラ系統のエンユニ名）は仮称。BA/BW/Z=1・DISE/DUMY=2・RSWD=5 は登録順での系統一致のみ確認。
4. DUMY を含む「対抗/連鎖エフェクト」可変引数（DSPL/ABHE/DRHE/DGHE/DUMY 等）の意味づけ（耐性チェーン vs 成分対抗）は本タスクの範囲外。
5. ESM MGEF レコード読取（`FUN_00416100` 系）が `0x00B33508` マップへ既定値を上書きする具体経路（insert or in-place update）は直接追跡していない。

---

検証サマリ: セルフテスト 4/4 PASS / fourcc 出現 18 箇所（.text のみ、UTF-16 0 件、ASCII 文字列 0 件）/ 登録呼び出し 161 件中 16 件が対象 / 実 ESM 再走査 1,167,017 レコード 0 errors で参照元特定完了
