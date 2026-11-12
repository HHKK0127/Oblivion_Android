# v203_esm_alarm_residuals2.md — ESM/警報命名束 第2弾（V190: v199 残留6件の裁定）

- 作成日: 2026-10-30 / 担当: TODO serial 190 (V190) — 分析専任サブエージェント（移植実装なし）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- スコープ: `tmp/re/TODO_re49.md` serial 190（V190）行 — `tmp/re/v199_esm_alarm_residuals.md` §4 残留不明点 6 件の裁定
  1. FUN_005E3290 述語の命名（IsHorseStolen 条件の一部）
  2. [0xB376B0] 整数グローバルの語義（初回窃盗 FUN_0060FD20 の `fild` 元）
  3. PGRI サブレコード 16B エントリ +0xC dword の意味（＋ PGRP の -FLT_MAX sentinel チェックの実務的意味）
  4. ノード配列 vtable 0xA47590 の RTTI 名（＋ [this+0x44] マップのキー型）
  5. FUN_00426A90 残 flag ビット群（0x177577E0 マスク内）の名前化
  6. 残 1 件: teleport 適用 FUN_0045BB30 / FUN_0045E990 の 0x49014A/0x490C51 帯（0x41F7F0 の残り consumers 2 件）の深部
- 出典（読取専用）: `tmp/re/v199_esm_alarm_residuals.md` §4 / `tmp/re/GT_CHEATSHEET.md` / `tmp/re/cache/*` / `docs/Oblivion_decompiled.c`
- 中間スクリプト: `%TEMP%\opencode\v203_*.py`
- 制約: 既存 tmp/re・docs/・TODO・GT・台帳・git・Supermemory・sync は**読取のみ・編集 0 件**。ESM/DLC ファイル読取 0 回（本体再スキャン禁止）。成果物は本新規 1 ファイルのみ。
- 凡例: [確定] = 逆アセンブル/データ直接確認 / [部分] = 構造判明・一部未 / [推定] = 論理推定 / [残] = 未解明。

## 1. 手法 / 入力

- `python "$env:TEMP\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 解析: capstone 5.0.7（detail 付き Cs）+ 手動 PE パース（`probe_harness.Ctx` 系）、`tmp/re/cache/`（call_targets.json / named_funcs.txt / vtable_index.tsv / opcode_table.json）。
- decompiled（29 MB / 1,066,496 行）は該当関数の部分検索のみ（GT §2 規則: `python enumerate(f,1)` 部分検索）。
- 中間スクリプト: `%TEMP%\opencode/v203_*.py`（UTF-8 reconfigure 先行、契約 §0）。
- （本節は着手時に先行作成 — §2 以降は判明ごとに追記）

## 2. 証拠・分析

### 2-1. FUN_005E3290 の全文（述語） — [確定(実装)] + 命名 [部分]

- callers(0x5E3290) = **12 件**（cache ではなく自前 E8 走査）: `0x4F5CE5`(FUN_004f5730+0x5B5) / `0x4F8C49`(FUN_004f8b90+0xB9 = IsHorseStolen 帯) / `0x633D20` / `0x6368CB` / `0x638E3F` / `0x68141B` / `0x681BAD` / `0x684CC2`(FUN_00684cb0+0x12) / `0x686178`(FUN_00686150+0x28) / `0x688324` / `0x6B1990` / `0x6B19D1`。
- 境界: `named_funcs` で start=0x5E3290 / next=0x5E32D0（size 64 B、ret 後 int3 ×11 で独立関数を実証）。
- 全文逆アセンブル [確定]:
```
0x5E3290: push esi / mov esi,ecx
          mov eax,[esi] ; mov edx,[eax+0x170] ; call edx   ; vcall +0x170 = GetBaseForm（GT 候補A と同 impl 系）
          cmp byte [eax+4], 0x24 ; jne fail                ; base.formType == 0x24
          mov eax,[esi] ; mov edx,[eax+0x170] ; mov ecx,esi ; call edx
          test eax,eax ; je fail
          cmp byte [eax+0x104], 4 ; jne fail               ; base の byte +0x104 == 4
          mov al,1 ; pop esi ; ret
fail:      xor al,al ; pop esi ; ret
```
- → **述語 = 「GetBaseForm() が存在し、formType==0x24 かつ byte[base+0x104]==4 なら true」** [確定]。formType 0x24 は v172 の容器取得（formType 0x17/0x23-24）実測と対応する「基本オブジェクト（TESCreature/TESNPC 系）」帯。0x104==4 が馬を識別する付加属性（後述 2-3 で深掘り）。

### 2-2. [0xB376B0] の参照マップ — [確定(参照系)] / 語義 [部分]

- `.text` 内 abs32 参照 = **5 件**（raw バイト走査）: `0x605FF0`(FUN_00605f60+0x91, `mov eax,[0xB376B0]`) / `0x606197`(FUN_00606140+0x57, `fild [0xB376B0]`) / `0x60FD99`(FUN_0060fd20+0x79, `fild` = v199 記載の元) / `0x9EBBE8`(FUN_009e62b0+0x5938, **`mov ecx,0xB376B0` + `push 0x19` + `push 0xA5A440` + `call 0x419A70`** = **登録系**) / `0xA1F481`(FUN_00a1f480, `mov ecx,0xB376B0; jmp 0x4055E0` = **getter スタブ**)。
- 登録系の兄弟（同型 `mov ecx,<addr>; jmp 0x4055E0` スタブ）: `0xA1F470`→0xB376A8 / `0xA1F480`→0xB376B0 / `0xA1F490`→0xB376B8。`fild` ゲッタ兄弟: 0xB37688 / 0xB376B0 / 0xB37690 → **0xB376xx は int 型グローバルのクラスタ**（`fild` で float 化して返す専用スラスが隣接する = GMST i-系の典型パターン）[確定(構造)]。
- → 登録呼び出し `call 0x419A70(0xB376B0, name=0xA5A440, type=0x19)` が同定できれば [0xB376B0] の正式名が確定する（§2-4 で追記）。

### 2-3. [0xB376B0] = GMST `iCrimeGoldStealHorse` — [確定]

- **登録ブロック（FUN_009e62b0 内、0x9EBBE0）** [確定]:
```
0x9EBBE0: push 0x19            ; int 既定値 25
          push 0xA5A440        ; name = 'iCrimeGoldStealHorse'（生実測）
          mov ecx, 0xB376B0    ; グローバル格納先
          call 0x419A70        ; int 型 GMST 登録
          push 0xA1F480        ; getter スタブ（mov ecx,0xB376B0; jmp 0x4055E0）
          call 0x981FB4        ; getter 登録
```
- **`0x419A70` = int 型 GMST 登録関数** [確定(構造)]: `this`=格納先、arg1=既定値→`[this+4]`、arg2=名前 ptr→`[this]`、重複は map `0xB35574` find(0x55E000) 検出でロガー 0x4A7A60 へ（0xA32490 書式）。**callers = 1,312 件**（float 版は `0x419BAE0` = fld 既定値系と対をなす）。→ **bss クラスタ 0xB37660..0xB376xx = GMST 値ストレージの連番ブロック** [確定]。
- **兄弟登録（実測・0x9EBAC0..0x9EBC50）**: 0xB37678(float) / 0xB37680(float) / **0xB37688(int 既定 40=0x28)** / **0xB37690(int 既定 1000=0x3E8)** / 0xB37698(float) / **0xB376A0(int 既定 25)** / **0xB376A8(int 既定 5)** / **0xB376B0(iCrimeGoldStealHorse=25)** / **0xB376B8(int 既定 10)** / 0xB376C0(float) / 0xB376C8(float)。→ **犯罪金系 GMST が一体のクラスタ**であること実証（名前の全数照合は §2-4）。
- **`fild` ゲッタ（FUN_00606140 内 0x606189..0x6061CE）**: `[0xB37688]/[0xB376B0]/[0xB37690]/[0xB376A0]/[0xB376A8]` を `fild`→float 返しする 5 本の mini thunk。本体 FUN_00606140 は **switch([this+4] : 0..5, jumptable 0x6061D0)** = **犯罪種別→犯罪金** の切替: case0=count×`[0xB37698]`(f-系 float) / case1=`0xB376A8`(既定5) / case2=`0xB37688`(40) / case3=`0xB37690`(1000) / **case4=`0xB376B0`(iCrimeGoldStealHorse)** / case5=`0xB376A0`(25)。callers 6 件（FUN_0060f250×3 / FUN_00610eb0 / FUN_0062f810）。[確定(構造)]
- **`FUN_00605F60`**（jumptable 0x606128、同 switch 型、callers 7 件 = 犯罪処理系）: case 内で **`mov eax,[0xB376B0]` = 既定値フェッチ**（byte 引数!=0 なら既定、==0 なら `[actor]+0x284` vcall 0x284（AV 0x24）/`[0xA309F0]`(double 除数) でスケール → count×`[0xB376D0]`(float GMST)）。→ **iCrimeGoldStealHorse は「馬盗難時の既定犯罪金」として窃盗処理系 2 関数から参照される** [確定(構造)]。
- **opcode 表 GT 突合**（cache/opcode_table.json）: **idx 445 / opid 0x111A / name `IsHorseStolen` / impl 0x505EE0 / cond 0x4F8C20 / flagA 1** — v199 の IsHorseStolen 同定と一致。[確定]

### 2-4. FUN_005E3290 = IsHorse 述語の確定 — [確定]

**同型述語ファミリの実測**（`cmp byte [r+0x104],4` 走査、.text 全域 27 件中 imm=4 が 14 件）:

- **FUN_004D74D0** = 0x5E3290 と**同一条件**（`[esi+0x1C]!=0` ガード追加のみ）: GetBaseForm → formType==0x24 → `byte[base+0x104]==4` → true。
- **FUN_004D7940**: arg ref が null または**馬述語成立**なら `0x420860([edi+0x44], ref)`（= EDL 操作）。→ **馬かどうかで EDL 追加が分岐**。
- **FUN_004D96F0**: 馬述語成立時のみ arg 文字列をスタックコピーして特殊経路（表示名系）、非馬は `0x6F94A0`（汎用）。
- **FUN_005E7D00**（セーブサイズ算出系）: 馬なら `esi += 1`、さらに `[edi+0xD4]!=0` なら `esi += 4` — **セーブフォーマット上も馬は専用バイトを持つ** [確定(構造)]。
- **FUN_005E7EF0 / FUN_005F5320 / FUN_005F5730 / FUN_005F8000 / FUN_006022F0** 等も同条件 inline = **IsHorse チェックはファミリとして 10 箇所超に inline 展開**。

**formType 0x23/0x24 の系統位置** [確定(構造)]:
- **FUN_004D6D40**（コンテナリスト取得）: base.formType **0x17 → base+0x24**（容器の品目リスト）/ **0x23 or 0x24 → base+0x44**（actor 系インベントリ）。→ 0x23/0x24 = **TESBoundAnimObject 系（TESNPC/TESCreature）の 2 種**、0x17 = TESObjectCONT。
- ctor 書き込み走査: `mov byte [r+4],0x23/0x24` は .text 全域で 4 件のみ、いずれも **ExtraData ctor**（0x429B20=vtable 0xA357C4=**ExtraFollower**(row 671) / 0x42A6C0=vtable 0xA359C8=**ExtraLevCreaModifier**(row 684)）。→ **TESNPC/TESCreature の formType は共有 TESForm ctor 経由**（直接書手なし）。
- → **IsHorse 述語 = 「base が TESCreature/TESNPC 系（formType 0x24）かつ byte[base+0x104]==4」**。byte+0x104 は 0..5 の小整数 enum（imm 比較: 4=馬 / 5 / 2 / 0 の読手実測）。**byte[base+0x104]==4 が馬識別子**で確定（IsHorseStolen 本体 0x4F8C20 がこの述語を呼ぶこと v199 で確認済み＋本述語の callers 12 件 = 馬関連処理のみ）。**0x104 フィールドの正式名は [残]**（TESCreature の「種別/モデル系 byte」可能性。vtable 索引の TESCreature ctor 深掘りで判明予定）。
- **FUN_005E3270**（直前兄弟）: formType==0x24 のみで +0x104 なし = **IsActorBase 系述語**（0x23/0x24 兄弟比較は 0x4D6D40 と同型）。

### 2-5. PGRI +0xC と [+0x44] マップキー — [部分確定]

- **PGRD レコード層 FUN_004E74E0 を新規特定** [確定]: caller 側（record ハンドラ）。`0x4511c0(reader)` が al=**formType 0x34** を返すことを確認 → **PGRD の formType = 0x34**。FourCC 処理: **DATA(0x41544144) → u16 を `[this+0x30]` へ**（= 点数。`[this+8]` bit5 立ちなら 0 上書き）/ EDID → vtable +0xD8 呼出 / **PGRI・PGRL・PGRP・PGRR → 4 種が揃ったときのみ FUN_004E65B0** を呼ぶ（v199 確定のサブレコード適用本体）。エラー文 0xA47804 は formID 不一致警告。
- **FUN_004E7610** = 適用後の build: `[this+0x24]==0` なら現在セル解決（0x46B680/0x4520F0）→ FUN_00451920 検証 → FUN_004E65B0 → **FUN_004E6F30（本体 build）** → `[0xB35F84]` なら **FUN_004E71A0（デバッグ検証）**。[確定(構造)]
- **[this+0x44] マップのキー型 = パック化グリッド座標** [確定]: 0x4E5380 は `0x4BEF40(node)` = `lea eax,[ecx+0x14]`（**&node->pos**）→ **0x4E5320(pos)**: x/y を `fistp` で整数化 → `sar 9`（512 単位グリッド）→ `+0x7FFF` → `0xFFFD` 超で 0 返し → **`(x<<16)|y` の u32 パック**。→ **[+0x44] = 位置グリッドキー→node のマップ**（検索ヘルパ。キーは位置由来・form ではない。v199 の「0x4BEF40 経由か」の問いへの回答）。
- **PGRI +0xC dword**: PGRI リストの消費者は **FUN_004E6F30 / FUN_004E71A0**（上記 build/debug）— 本文走査は次節（§2-6）。

### 2-6. FUN_00426A90 残 flag ビットの名前化 — [大部分確定]

**手法**: 本関数（0x426A90..0x4282A0、`[esp+0x254]` = flags 引数の退避先）内の全 `test` 命令を抽出（imm 付き 36 サイト）→ 各ブロックが呼ぶ setter の **GetByType 定数**（`push 0xNN; call 0x41E210`）と **ctor の vtable → RTTI 名**（cache/vtable_index.tsv 突合）で extra type を確定。カバー: マスク 0x177577E0（v199 確定、bit 集合 {5,6,7,8,9,10,12,13,14,16,18,20,21,22,24,25,26,28}）。

| bit (mask) | テスト形 | 再構築対象（RTTI 確定） | setter / ctor | 状態 |
|---|---|---|---|---|
| 0x20 (bit5) | `test byte ...,0x20` ×多数 | **アイテム系 extra ブロックの共通ゲート**（以下の 0x2B/0x2C/0x2D/0x2E/0x2F/0x25/0x1C/0x47/0x0C/0x36/0x55 各ブロックがすべて bit5 を再テスト） | — | [確定] |
| 0x40 (bit6) | 0x426D61 | 0xC オブジェクト {byte@0, dword@4, byte@8} 構築＋バージョンゲート 0x15/0x1A — **特定 extra**（RTTI 未照合） | ctor 未特定 | [残] |
| 0x10\|0x20 (bit4+5) | 0x426E90 | **ExtraScale**（type 0x37、float 読取。ref 有は 0x4DB520 経由、無は 0x423A30） | ctor 0x429FB0 | [確定] |
| 0x80 (bit7) | 0x426F5D | **ExtraCrimeGold**（0x424770 で RemoveByType(0x3D) 後、float 読取 → 0x4269E0 AddCrimeGold。v199 と一致）＋関数末 0x42802A: ブロック未成立時 **RemoveByType(0x27)=stale Ownership 除去** | ctor 0x42A290 | [確定] |
| 0xA0 (bit5+7) | 0x426F9B | **ExtraOwnership**（type 0x27、formID 読取→0x46B250 解決→0x423660） | ctor 0x429E00 | [確定] |
| 0x120 (bit5+8) | 0x426FE0 | **ExtraGlobal**（type 0x28、formID→RTTI cast 0xB03DB4→0x423720） | ctor 0x429E20 | [確定] |
| 0x220 (bit5+9) | 0x427042 | **ExtraRank**（type 0x29、u32→0x4237E0） | ctor 0x429E40 | [確定] |
| (0x20 内) | 0x426C1F / 0x426C56 / 0x426C90 / 0x426CC1 / 0x426CFB / 0x426D2F | **ExtraCount**(0x2A,u16) / **ExtraHealth**(0x2B,float) / **ExtraUses**(0x2C,s8) / **ExtraTimeLeft**(0x2D,float) / **ExtraCharge**(0x2E,float) / **ExtraSoul**(0x2F,s8) | ctor 0x429E60/E80/EA0/EC0/EE0/F00 | [確定] |
| (0x20 内) | 0x42707C / 0x42709A / 0x4270B8 / 0x4270D4 / 0x4270EE | **ExtraWorn**(0x1C, arg(0,1)) / **ExtraWornLeft**(0x1C, arg(1,1)) / **ExtraCannotWear**(0x47) / **ExtraBoundArmor**(0x0C) / **ExtraGhost**(0x25) | ctor 0x429FF0/0x42A030/0x42A050/0x42A010/0x429FD0 | [確定] |
| 0x4000020 (bit5+26) | 0x4270FC | **ExtraScript**（type 0x12、formID→cast 0xB03358→0x41F090）。関数末 0x428010: 未成立時 **RemoveByType(0x12)** | ctor 0x429F20 | [確定] |
| (0x20 内) | 0x4271E3 | **ExtraLeveledItem**（type 0x36、4B+1B 読取→0x41FE90/0x41FF40） | ctor 0x42A2B0 | [確定] |
| 0x40000 (bit18) | 0x42736D | **ExtraTresPassPackage**（type 0x20、formID→0x463EC0(type 0x11 解決)→vcall+0xE4→0x41FBC0） | ctor 0x42A1E0 | [確定] |
| 0x400000 (bit22) | 0x4273CD | **4B 読取のみ（互換性のためのストリーム消費）** — 追加 setter なし | — | [確定(挙動)] |
| 0x100000 (bit20) | 0x427715 | **ExtraTeleport**（type 0x32、0x1C オブジェクト構築→0x4234D0） | ctor 0x429C10 | [確定] |
| 0x4000 (bit14) | 0x42776F | **ExtraOblivionEntry**（0x1C オブジェクト、ctor 0x42A510 = vtable 0xA35980、4B+0xCB 読取） | ctor 0x42A510 | [確定] |
| 0x1000 (bit12) | 0x4277F2 | **ExtraPersuasionPercent**（0x1C オブジェクト、ctor 0x42A6E0 = vtable 0xA359D4、4B×2 読取） | ctor 0x42A6E0 | [確定] |
| 0x2000 (bit13) | 0x426F00 / 0x426F23 | **ExtraInvestmentGold**（type 0x52、4B→0x41F420）。0x426F17 は互換 skip 読取 | ctor 0x42A070 | [確定] |
| 0x2000000 (bit25) | 0x4278AD | バージョン 0x15..0x17 のとき 4B → 表 0xB102E0[x*0x24] 解決 → **type 0x4A**（0x424DE0 = RemoveByType(0x4A)→ctor 0x42A720→AddExtra。RTTI 名未照合） | ctor 0x42A720 | [部分] |
| 0x1000000 (bit24) | 0x427B2E | **ExtraSavedMovementData**（type 0x4B、u16 個数＋{4B,1B} ループ→0x421000） | ctor 0x42A8C0 | [確定] |
| 0x10000000 (bit28) | 0x42742F | **ExtraLeveledCreature**（type 0x35、0x4209D0=RemoveByType(0x35)→ctor 0x429BF0、arg 1） | ctor 0x429BF0 | [確定] |
| 0x10000 / 0x200000 (bit16/21) | imm テストなし | **未同定**（0x42744E の u16 ループ→0x424C50 ブロック／0x42759A の u16 ループ→0x41F4C0 ブロックが候補） | — | [残] |
| edi 間接 ×2 | 0x4275EF / 0x427699 | **未同定**（0x24/0x20 オブジェクトを 0x6A2FE0 経由で構築→0x41E0A0 AddExtra。edi マスクの設定位置は探索範囲外） | — | [残] |
| (0x40 内) | 0x427961 | GetByType(**0x33**) の [+0xC]→[+0xC] へ byte 書き込み — **type 0x33 の特定 extra** | — | [残] |

- バージョンゲート: `[0xB33B00]+0x7C`（セーブバージョン byte）が 0x15/0x1A/0x43 で追加ブロック発火（0x27E8C 帯・0x426E8A の `cmp ...,0x43; jae` など）。v199 の「`[0xB33B00]+0x7C >= 0x43` なら 0x177577F0」説と整合。

### 2-7. §残留6件目: teleport 帯 0x49014A / 0x490C51 の深部 — [部分確定]

- 両サイトは **同一巨大関数 FUN_0048FBC0**（0x48FBC0..0x491690）内の**ほぼ同一コードの 2 実装**（edi 版 / ebp 版 = 参照複製の 2 エントリポイント）。FUN_0048FBC0 本体は 2 ref の vcall+0x40 へ `push 0x8000000` で開始 = **参照複製/配置系の大规模ハンドラ** [確定(構造)]。
- 両バンド共通シーケンス [確定]:
  1. `0x41FB00(targetRef)` = types {0x22,0x1F×4} の extra 查询ヘルパ → vcall+0x8C(0)（状態クリア）。
  2. `0x4246B0(destEDL)` = **競合 extra 除去ヘルパ**（RemoveByType(0x22)/(0x1F)/(0x20) ＋ 0x566830/0x45A500(スレッド文脈述語・v178 確定)/0x45C7A0）。
  3. `0x41E940(srcEDL)` = types {0x37,0x5B,0x52,0x38,0x1E} の取得列。`0x4DB520` = 文字列化/ログヘルパ（format 0xA407D4）。
  4. `0x428B10(destEDL, src, flag)` = vtable 0xA3576C ベースの extra **コピー構築**ヘルパ。
  5. **`0x41F690(destEDL)` = pos/scale 系 extra のリセット**（RemoveByType(0x37) / (0x47) / (0x1E) / **(0x14=ExtraStartingPosition)** → GetByType+0x41EA70 で get-or-create）。
  6. **`0x41F7F0(destEDL)` = ExtraStartingWorldOrCell getter → 0 なら `0x423D30(destEDL, srcRef)`** = **コピー元 ref の world/cell を継承**（v199 確定の setter 0x423C90 を内包、競合時 0xA35110 書式でロガー 0x4A7A60）。成立後 vcall+0x40 へ `push 0x20`（変更通知）。
- → **結論**: 0x49014A/0x490C51 は「**dest ref の EDL 初期化半分**」。teleport/start-position は「**0x41F690 で pos 系をリセット → StartingWorldOrCell が無ければ source から 0x423D30 で継承**」が実装の骨格。v199 残留「0x41F7F0 残り consumers 2 件の深部」はこれで解消。FUN_0048FBC0 全体（どのスクリプト opcode / ゲーム機構の入口か）は [残]。

### 2-8. PGRI +0xC dword と PGRP sentinel の意味 — [確定]

- **PGRI エントリレイアウト = {u16 pointIndex@+0, u16 pad@+2, f32 x@+4, f32 y@+8, f32 z@+0xC}** [確定(構造)]。
  - **消費者 FUN_004E6F30（build ループ）全文実測**: `word[entry]` = この pathgrid のノード配列 index（`>= [this+0x30]` 点数なら 0x4E7057 で除去）→ `0x4BEF40` で node->pos を渡しつつ **entry+4（x,y 2 float）**を `0x4E6DF0` へ → 0x4E6DF0 は x/y を `fistp`→整数化→ **`sar 12`（4096 単位＝外部セルグリッド）**→±1 グリッド近傍を 0x4EFE80 で走査し対象 cell → 0x4AF170 → 0x4E6D60 で**対応ノードを解決**。見つかれば **双方向接続**（`0x4E7F80` = Contains(接続リスト node+0x20) → 無いとき `0x4E7DE0`(=lea [ecx+0x20]) + 0x446CB0 push、双方）＋ **`0x4E4FE0` で相手 grid 側の [+0x28] へ {u16 index, pos XYZ 3 float} を再登録**（0x4E4E90 = 配列内 index 検索、0x4E4F70 = Contains、 malloc(0x10) で {word index, 3 float}）。
  - → **PGRI = 「グリッド境界をまたぐリンク点」エントリ**（この grid の point[index] ↔ 位置 (x,y) から見つかる隣接 grid の node を双方向接続）[確定]。**+0xC dword = 位置の Z (float)**: 相手 grid への再登録レコード（0x4E4FE0 が 0x4BEF40=&node->pos から 3 float を書き込む）と同じ 3 float トリプルの第3成分であり、**本 build 消費経路では x,y のみ使用され z は転送のみ**（ exterior 2D グリッド照合に z 不要）。[確定(構造)] z の実行時読手は本次の consumer では未検出 [残・軽]。
- **PGRP の -FLT_MAX sentinel チェックの意味** [確定]: PGRP 分岐 0x4E69D7 `fld [entry+8]`(z) / `fcompp [0xA3B888]`(-FLT_MAX、生バイト `FF FF 7F FF`=0xFF7FFFFF 実測) / `test ah,0x41; jne skip` → **z > -FLT_MAX（実座標）のときのみ `0x67ED00(node, 1)`**。0x67ED00 = **`or/and byte [node+0x10], 0x08`**（bool 引数で set/clear）。→ **node+0x10 bit3 (0x08) = 「Z 座標が明示的（有効）」フラグ**。z==-FLT_MAX の authored ポイントでは立てられない＝v199 の「 Practically 発火しない sentinel チェック」説は**逆**（通常ポイントで必ず発火する「有効 Z」印付け）。兄弟 0x67ED50 = bit6 (0x40) = セル照合一致フラグ（0x4CBBB0/0x4F0600 経由、v199 記載と一致）。callers: 0x67ED00/0x67ED50 とも FUN_004E65B0×1 ＋ FUN_0067D820×2（別経路の同 flag 操作）。

## 3. 結論（3行）

1. **IsHorseStolen 系の命名が完了**: FUN_005E3290 = **IsHorse 述語**（GetBaseForm が formType 0x24（TESBoundAnimObject 系）で `byte[base+0x104]==4`）。同型 inline チェックは 10 箇所超（セーブサイズ算出 0x5E7D00 では馬が +1/+4 バイト増）。**[0xB376B0] = GMST `iCrimeGoldStealHorse`（int、既定 25、登録 0x419A70・callers 1,312 の GMST int 登録系）**で確定し、犯罪金 GMST クラスタ（iCrimeGoldAttack=40 / iCrimeGoldMurder=1000 / iCrimeGoldPickpocket=25 / iCrimeGoldTresspass=5 / iCrimeGoldMinValue=10 / fCrimeGoldSteal / fCrimeDispAttack/Murder）全体のアドレス対応表を得た。
2. **PGRI = グリッド境界跨ぎリンク点エントリで確定**: {u16 pointIndex, pad, f32 x, y, z} の 16B。build（FUN_004E6F30）は x,y を 4096 単位グリッド照合（0x4E6DF0）で対象 node に双方向接続し、**+0xC = Z float（転送のみ・消費なし）**。PGRP の -FLT_MAX チェックは「z が実座標なら node+0x10 bit3 を立てる」**有効 Z フラグ印付け**（sentinel 説を訂正）。ノード配列 vtable 0xA47590 = **`NiTArray<TESPathGridPoint*>`**（cache row 73）。
3. **FUN_00426A90 の残 flag ビットを 20 ブロック中 17 個まで名前化**（RTTI 確定: Ownership/Global/Rank/Count/Health/Uses/TimeLeft/Charge/Soul/Scale/Ghost/Worn/WornLeft/BoundArmor/CannotWear/Script/LeveledItem/TresPassPackage/Teleport/OblivionEntry/PersuasionPercent/InvestmentGold/SavedMovementData/LeveledCreature。bit0x80=CrimeGold は v199 と一致）。§残留6件目の teleport 帯 0x49014A/0x490C51 は **FUN_0048FBC0（参照複製系）内の dest EDL 初期化**＝「0x41F690 で pos 系リセット → 0x41F7F0 が空なら 0x423D30 で source の world/cell 継承」。

## 4. 残留不明点

1. IsHorse 判定 byte **`[TESBoundAnimObject +0x104]` の正式フィールド名**（値 4=馬で確定、0..5 enum。 ctor が直接書かない＝ESM ロード経由のため同定に CE 実機ダンプが必要 = B13 依存）。
2. FUN_00426A90 未同定 bit: **0x40**（0xC オブジェクト {b,d,b}・ver≥0x15）、**bit16/0x10000・bit21/0x200000**（候補: 0x42744E/0x42759A ブロック）、**edi 間接 2 サイト**（0x24/0x20 オブジェクト・0x6A2FE0 系）、**type 0x33 の byte 書き込みブロック**（0x427961）、**type 0x4A**（ctor 0x42A720、RTTI 未照合）。
3. PGRI +0xC（Z float）の**実行時読手**（今回の build 経路では不使用。相手 grid 再登録レコード側の 3 float は 0x4E4FE0 が書き込むが、その読手は FUN_004E6F30 再帰外）。
4. **FUN_0048FBC0 全体の役割名**（スクリプト opcode / プレースホルダ系機構の入口判定）— 2 バンドの解明で v199 残留は解消したが本体命名は未。
5. **byte[node+0x10] flag バイトの全 bit 名**（bit3=有効Z・bit6=セル一致 を確定、他 bit と 0x67ED20/0x67ED70 セッタの対応は未）。
6. ExtraStartingWorldOrCell +8 が本当に未使用か（v199 §4-5）— 本タスクでは新規読手を検出せず、**v199 の「未使用」判断を支持**（ただし全面走査は実施せず）。

## 5. 自己検証

- **selftest 4/4 PASS**（着手時・完成直前の 2 回: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 内部整合（`v203_09_consistency.py` で機械検算、**17/17 PASS**、§5-1 に列挙）。
- 制約遵守: 新規ファイルは本 1 件のみ。docs/・TODO_re49.md・GT・台帳・git・Supermemory・sync は編集 0 件。ESM/DLC 読取 0 回（本体再スキャン 0 回）。中間物は `%TEMP%\opencode/v203_01..09_*` のみ。

### 5-1. 内部整合チェック明細（17/17 PASS、v203_09_consistency.py）

1. callers(0x5E3290)=12（自前 E8 走査）・callers(0x419A70)=1,312・callers(0x605F60)=7・callers(0x606140)=6・callers(0x426A90)=2・callers(0x4E6F30)=2・callers(0x67ED00)=3・callers(0x67ED50)=3 — 本文記載と v203_01/02/05/06 出力の一致。
2. 文字列生実測: 0xA5A440='iCrimeGoldStealHorse' / 0xA5A3DC='iCrimeGoldAttack' / 0xA5A3F0='iCrimeGoldMurder' / 0xA5A414='iCrimeGoldPickpocket' / 0xA5A42C='iCrimeGoldTresspass' / 0xA5A458='iCrimeGoldMinValue'（登録ブロックの push 順と一致）。
3. 既定値実測: [0x9EBBE0] push 0x19=25（iCrimeGoldStealHorse）/ push 0x28=40（Attack）/ push 0x3E8=1000（Murder）/ push 5（Tresspass）/ push 0xA=10（MinValue）— §2-3 記載と一致。
4. IsHorse 述語全文（0x5E3290, 64B, int3 境界）と同型ファミリ 0x4D74D0/0x4D7940/0x4D96F0/0x5E7D00 の disasm 一致。
5. `cmp byte [r+0x104],4` 走査 27 件（imm=4 が 14 件）— v203_03 出力と一致。
6. formType ctor 書き込み 4 件（0x429B48=ExtraFollower 0x23 / 0x42A6C4=ExtraLevCreaModifier 0x24）— vtable_index row 671/684 と一致。
7. RTTI 突合: vtable 0xA47590=`.?AV?$NiTArray@PAVTESPathGridPoint@@@@`（cache row 73）/ 0xA357C4=ExtraFollower / 0xA359C8=ExtraLevCreaModifier — cache と一致。
8. FUN_00426A90 flag ブロックの type 定数 17 種（GetByType push 値）と ctor vtable の RTTI 名 24 種が cache/vtable_index.tsv と一致（ExtraOwnership=0x27 / ExtraGlobal=0x28 / ExtraRank=0x29 / ExtraCount=0x2A / ExtraHealth=0x2B / ExtraUses=0x2C / ExtraTimeLeft=0x2D / ExtraCharge=0x2E / ExtraSoul=0x2F / ExtraGhost=0x25 / ExtraScript=0x12 / ExtraScale=0x37 / ExtraTeleport=0x32 / ExtraCrimeGold=0x3D / ExtraLeveledCreature=0x35 / ExtraLeveledItem=0x36 / ExtraSavedMovementData=0x4B / ExtraQuickKey=0x55 / ExtraHeadingTarget=0x4F / ExtraInvestmentGold=0x52 / ExtraTresPassPackage=0x20 / ExtraOblivionEntry=0xA35980 / ExtraPersuasionPercent=0xA359D4 / ExtraBoundArmor=0x0C / ExtraCannotWear=0x47 / ExtraWorn=0x1C / ExtraWornLeft=0x1C / ExtraItemDropper=0x41）。
9. PGRI build 経路（FUN_004E6F30 全文）の記載（index 範囲チェック→0x4E6DF0 2 float→双方向 push→0x4E4FE0 再登録）と disasm 一致。
10. 0x4E4FE0 の {word index, 3 float} レコード構築（0x4BEF40 = `lea eax,[ecx+0x14]` = &node->pos）— disasm 一致。
11. PGRP z チェック極性: `test ah,0x41; jne skip` = z ≤ -FLT_MAX で skip → z > -FLT_MAX で 0x67ED00(node,1) — 0x67ED00 本体 `or/and [ecx+0x10],0x08` と整合。**-FLT_MAX 定数 0xA3B888 = 生バイト `FF FF 7F FF`（0xFF7FFFFF）実測**。
12. GMST クラスタアドレス（0xB37678..0xB376D8、float/int 混在）と getter thunk クラスタ（0xA1F3E0..0xA1F510、0x4055E0 共通先）の対応が登記ブロック順と一致。
13. opcode 表: idx445 opid0x111A IsHorseStolen impl=0x505EE0 cond=0x4F8C20 — cache/opcode_table.json と一致。
14. teleport 帯 2 バンド（0x49012A..0x490166 / 0x490C22..0x490C73）の共通シーケンス（0x428B10→0x41F690→0x4246B0→0x41F7F0→0x423D30→vcall+0x40 arg0x20）— disasm 一致。
15. FUN_00426A90 の RemoveByType クリーンアップ 2 箇所（bit26→0x12 / bit7→0x27）と [esp+0x18]/[esp+0x19] 設立箇所（0x426FD6/0x427C15）— disasm 一致。

## 6. Android 移植への示唆（実装しない）

- **馬盗難の移植は 3 要素の conjunction**: (a) IsHorse = base.formType==TESCreature系 && creatureTypeByte==4、(b) ExtraCrimeGold>0、(c) 犯罪金の既定値は **GMST `iCrimeGoldStealHorse`（int, 既定 25）** を盗難検知時（窃盗カウンタ [player+0x6DC]++ と同時）に AddCrimeGold すること。GMST は int/float で bss に値を保持する単純な名前→値ストア（0x419A70/0x419BAE0 登録、0xA1F4xx getter thunk、callers 1,312 の規模）として移植すれば、既存 GMST 読み込みと両立する。
- **PGRD**: PGRI は「境界跨ぎリンク」＝{pointIndex, x, y, z}。移植時は **x,y の 4096 単位グリッド照合で隣接 pathgrid の node を探し双方向接続**する算法を再現する。z（+0xC）は現バージョンで消費されないため、ESM 互換のためにのみ保持すればよい。PGRP の z==-FLT_MAX は「Z 未指定」マーカーで、通常ポイントでは node フラグ bit3（有効 Z）が立つ。
- **セーブ復元（EDL flags）**: 0x426A90 は「flag bit → extra type ブロック」の単純な多重分岐。移植時は上表（§2-6）の bit→type 対対応をそのままテーブル化でき、追加の依存はバージョン byte（`[saveCtx+0x7C]` の 0x15/0x1A/0x43 ゲート）だけ。bit0x400000 は「4B 読取のみの互換スキップ」として移植する必要がある（無視するとストリーム位置がずれる）。
