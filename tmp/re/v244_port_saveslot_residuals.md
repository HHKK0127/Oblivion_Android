# v244_port_saveslot_residuals.md — 移植セーブ経路残点束（N-24..N-27 裁定）

- 作成: 2026-11-02 / 担当: TODO serial 237（V237・中）/ **分析専任（docs/GT/TODO/ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v241_port_mgef_residuals6.md` §4 新規残点 N-24..N-27（本体は読取のみ・無編集）。最上流=FUN_00465130・.ess 本体=slot19(+0x4C)/slot20(+0x50)・slot8/11=dead code は確定済み（v241 §1）を前提とする。
- 手法: 前回実行はサーバ障害中断（v244 成果物未生成）だが `%TEMP%\opencode\v244_scanA..G.py` と各 `_out.txt` が残存していたため **それらの出力読み取りを起点**に、scanH..N を新規実行して補完。capstone 5.0.7 再逆アセンブル（PE 手動解析・VA=0x400C00+file_off）＋ `docs/Oblivion_decompiled.c` 部分読取（読取のみ）＋ `tmp/re/cache/vtable_index.tsv` / `named_funcs.txt` 突合。ESM 本体は未アクセス（再スキャン禁止遵守）。git 不使用。
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**
- 結論早見: 項目1 **【解明】slot19/20 のフォーム型実装は TESForm 系 86 クラス（継承57/独自29）。Ni 系の同スロットは無関係** ／項目2 **【解明】FUN_00518000 は起動期の既定フォーム生成ルーチン。0x519320 サンクは Script/BirthSign の vtable slot#11(+0x2C) エントリ（→TESForm slot8）** ／項目3 **【解明・v241 仮説訂正】+0x174 と +0x2C の両命令が実在。0x4CBCED は skipdata 誤読ではなく `call [esp+0x2C]`＝引数 param_6（コールバック）呼び出し** ／項目4 **【解明＋v241 訂正】FUN_005D3230 は引数なしの SaveMenu 内部セーブルーチン、0x40F は「メッセージ」ではなく SaveMenu の menu ID。コンソール "save" の実ハンドラは FUN_005023D0（テーブル 0xB0BF38）で FUN_00465130 を直接呼ぶ**

（分析結果は項ごとに記載済み＝本ファイルは即時書込みで完成）

---

## §1 項目1（N-24）: slot19(GetSaveSize)/slot20(SaveGame) の全クラス実装列挙・分類

**判定: 【解明】.ess セーブ本体が per-form ディスパッチする slot19(+0x4C)/slot20(+0x50) の実装は、TESForm 系フォーム型 86 クラスで全数を列挙・分類できた。TESForm 基底（0x46AC50/0x46B5E0）を継承する 57 クラス＋独自実装 29 クラス。NiObject 系クラスの同オフセットは Ni ストリーム（Load/Save）用で .ess セーブとは無関係（slot 番号の一致は位置的偶然）。**

### §1-1 手法と全体像

- `tmp/re/cache/vtable_index.tsv`（1715 rows）。off=0 プライマリ vtable かつ ≥21 スロット: **459 本**、その (slot19,slot20) ペアは **144 種**（scanA A1）。144 ペアの大半（115 ペア）は Ni/gamebryo 系（例: controllers=0x715540/0x715570、NiAVObject 系=0x7075F0/0x60D0A0、ExtraData 系=0x977C50/0x977C50）＝**NiObject リフレクションのストリーム Load/Save** であり、TESForm の GetSaveSize/SaveGame とは機能的に別物。
- フォーム型の認定は RTTI BCA（flattened）に `.?AVTESForm@@` を含むもの。off=0 主 vtable 判定では 85 クラス（scanA4 F1: OWN 29 / INH 56）。**max-slot 行 ≥21 かつ primary 行 <21 のクラスを全クラス横断で走査した結果、取りこぼしは TESFlora の1件のみ**（scanH H2）→ これを加えて **86 クラス**。
- vtable 値は vtable_index の実アドレスをそのまま採用（例: CELL slot19=0x4D1EB0 / slot20=0x4D2020、TESForm 基底 slot19=0x46AC50 / slot20=0x46B5E0）。

### §1-2 実装本体の意味（decompiled.c）

| 実装 | 内容 |
|---|---|
| TESForm slot19 = `FUN_0046AC50`（:84349） | `return (flags & 1) ? 4 : 0;` — GetSaveSize 基底（flags bit0 のとき 4B） |
| TESForm slot20 = `FUN_0046B5E0`（:84994） | `if (flags & 1) FUN_0045B9A0(this + 8, 4);` — SaveGame 基底（this+8 のワード書き出し） |
| TESObjectCELL slot20 = `FUN_004D2020`（:164592） | 先に基底 `FUN_0046B5E0(uVar3)` を呼出し、その後にセル固有ブロック（flags bit 依存: `0x4B4F4C42` マジック＋グリッド short、bit3 フラグバイト、bit4 名前文字列、bit5 水関連、bit 0x10000000 / 0x8000000 系特殊）を追記 |
| TESObjectCELL slot19 = `0x4D1EB0` | **FUN_004D1E40 とは別の関数**（scanH H6: 0x4D1EAC の `ret 4` ＋int3 の後、0x4D1EB0 で `push ecx… test ebx,0x8000000` と新規エントリ）。decompiled の FUN_004d1e40 本文（0x4D1E40-、slot+0x200 ディスパッチ）は slot19 実装ではない ⇒ **v241 §1-3 の「0x4D1EB0(FUN_004D1E40)」の括弧は誤り**（本項で訂正） |

### §1-3 一覧表 A: 独自実装（OWN）29 クラス

| クラス | vtable | slots | slot19 | slot20 | 分類 |
|---|---|---|---|---|---|
| TESObjectMISC | 0xA44CA4 | 72 | 0x4B4560 (FUN_004b4560) | 0x4B95B0 (FUN_004b9570 内) | 共有実装の源（MISC系5クラス） |
| TESKey | 0xA42A4C | 72 | 0x4B4560 | 0x4B95B0 | 共有（MISC系） |
| TESObjectAPPA | 0xA43FB4 | 72 | 0x4B4560 | 0x4B95B0 | 共有（MISC系） |
| TESSigilStone | 0xA45534 | 72 | 0x4B4560 | 0x4B95B0 | 共有（MISC系） |
| TESSoulGem | 0xA456F4 | 72 | 0x4B4560 | 0x4B95B0 | 共有（MISC系） |
| TESObjectARMO | 0xA441BC | 72 | 0x4B5BE0 (FUN_004b5be0) | 0x4B4B50 (FUN_004b4a70 内) | 共有（ARMO/CLOT） |
| TESObjectCLOT | 0xA44644 | 72 | 0x4B5BE0 | 0x4B4B50 | 共有（ARMO/CLOT） |
| TESObjectBOOK | 0xA44454 | 72 | 0x4B52F0 (FUN_004b52d0) | 0x4B5330 (同関数別エントリ) | 独自 |
| TESObjectLIGH | 0xA4319C | 72 | 0x4B0B40 (FUN_004b0b40) | 0x4B0B70 (同関数別エントリ) | 独自 |
| TESObjectWEAP | 0xA45354 | 72 | 0x4BB070 (FUN_004bb070) | 0x4BB0A0 (同関数別エントリ) | 独自 |
| TESAmmo | 0xA4068C | 72 | 0x4A8F60 (FUN_004a8f60) | 0x4A8F90 (同関数別エントリ) | 独自 |
| TESObjectCELL | 0xA469D4 | 55 | 0x4D1EB0（独立関数・§1-2） | 0x4D2020 (FUN_004d2020) | 独自 |
| TESObjectREFR | 0xA46C44 | 105 | 0x4E2BF0 (FUN_004e2bf0) | 0x4E2D60 (FUN_004e2d60) | 独自 |
| MobileObject | 0xA732FC | 129 | 0x659BE0 (FUN_00659be0) | 0x659C30 (FUN_00659c30) | 独自 |
| Actor | 0xA6E074 | 239 | 0x5E7D00 (FUN_005e7d00) | 0x5E7EF0 (FUN_005e7ef0) | 共有（Actor/Creature） |
| Creature | 0xA710F4 | 239 | 0x5E7D00 | 0x5E7EF0 | 共有（Actor/Creature） |
| Character | 0xA6FC9C | 239 | 0x60E360 (thunk) | 0x60E370 (thunk) | **thunk → Actor 実装** |
| PlayerCharacter | 0xA73A0C | 239 | 0x6610F0 (FUN_00661080) | 0x661540 (FUN_00661540) | 独自 |
| TESActorBase | 0xA52F84 | 80 | 0x51A3B0 (FUN_0051a3b0) | 0x51A520 (FUN_0051a520) | 独自 |
| TESCreature | 0xA5324C | 80 | 0x51C6E0 (FUN_0051c650) | 0x51DA50 (FUN_0051d4c0) | 独自 |
| TESNPC | 0xA53DD4 | 80 | 0x521BA0 (FUN_00521a40) | 0x523370 (FUN_005232d0) | 独自 |
| TESFaction | 0xA53524 | 55 | 0x51F120 (FUN_0051f120) | 0x51F160 (FUN_0051f160) | 独自 |
| TESQuest | 0xA541EC | 55 | 0x529BE0 (FUN_00529b70) | 0x529D20 (FUN_00529d20) | 独自 |
| MagicProjectile | 0xA76284 | 137 | 0x69F740 (FUN_0069f740) | 0x69F770 (同関数別エントリ) | 共有（Mag.Proj/Spray） |
| MagicSprayProjectile | 0xA76594 | 137 | 0x69F740 | 0x69F770 | 共有（Mag.Proj/Spray） |
| MagicBallProjectile | 0xA75944 | 137 | 0x694E40 (FUN_00694d20) | 0x694E80 (同関数別エントリ) | 独自 |
| MagicBoltProjectile | 0xA75BC4 | 137 | 0x696760 (FUN_006965c0) | 0x6967B0 (同関数別エントリ) | 独自 |
| MagicFogProjectile | 0xA75EFC | 137 | 0x69C6C0 (FUN_0069c620) | 0x69C710 (同関数別エントリ) | 独自 |
| ArrowProjectile | 0xA6F08C | 129 | 0x608310 (FUN_00608280) | 0x6099D0 (FUN_006099d0) | 独自 |

### §1-4 一覧表 B: TESForm 継承（INH）57 クラス

全クラス共通: **slot19 = 0x46AC50 (FUN_0046AC50) / slot20 = 0x46B5E0 (FUN_0046B5E0)**（＝§1-2 の基底実装を override なしで使用）。

AlarmPackage(0xA6EF94/59), AlchemyItem(0xA3217C/72), BirthSign(0xA52524/55), CombatController(0xA70A14/60), DefaultCombatStyle(0xA4109C/92), DialoguePackage(0xA7151C/59), EffectSetting(0xA32B14/55), EnchantmentItem(0xA33C84/55), FleePackage(0xA71664/59), IngredientItem(0xA33F5C/72), MagicItemForm(0xA33DBC/55), MagicItemObject(0xA32364/72), Script(0xA49DA4/55), SpectatorPackage(0xA746AC/59), SpellItem(0xA34C3C/55), TESBoundAnimObject(0xA43984/72), TESBoundObject(0xA4385C/72), TESBoundTreeObject(0xA43AAC/73), TESClass(0xA52C6C/55), TESClimate(0xA45C9C/55), TESCombatStyle(0xA407F4/92), TESEffectShader(0xA419EC/55), TESEyes(0xA53414/55), TESForm(0xA3BE3C/55), TESFlora(0xA41B1C/72・off=0xC・§1-5), TESFurniture(0xA41D1C/72), TESGlobal(0xA4981C/55), TESGrass(0xA4280C/98), TESHair(0xA53654/55), TESIdleForm(0xA53774/55), TESLandTexture(0xA4656C/55), TESLevCreature(0xA42B9C/72), TESLevItem(0xA42DC4/72), TESLevSpell(0xA42F1C/72), TESLoadScreen(0xA4997C/55), TESObject(0xA43644/69), TESObjectACTI(0xA43CB4/72), TESObjectANIO(0xA43DFC/55), TESObjectCONT(0xA44824/72), TESObjectDOOR(0xA44A54/72), TESObjectLAND(0xA46064/55), TESObjectSTAT(0xA44DEC/72), TESObjectTREE(0xA44FB4/93), TESPackage(0xA6743C/59), TESPathGrid(0xA47644/55), TESRace(0xA548FC/55), TESRegion(0xA3FEE4/57), TESRoad(0xA4788C/55), TESSkill(0xA54EFC/55), TESSound(0xA5219C/72), TESSubSpace(0xA4592C/72), TESTopic(0xA5555C/55), TESTopicInfo(0xA5568C/55), TESWaterForm(0xA47F0C/55), TESWeather(0xA47FEC/55), TESWorldSpace(0xA4829C/57), TrespassPackage(0xA747BC/59)

（形式: クラス名(vtable/slots)。スクリプト全文は `%TEMP%\opencode\v244_scanA4_out.txt` A4-F1。）

### §1-5 TESFlora の特殊レイアウト（網羅性の裏取り）

- off=0 プライマリ行は 4 スロット（vt 0xA41C40: 0x46E0A0/0x60D0A0/0x46E110/0x46E150）で、**直後に ASCII 文字列が接続**（"ITMPOrganic"、"sFloraSuccessMessage"、" You successfully harvests %s."＝scanL L1）。72 スロット行（vt 0xA41B1C）の COL offset は 0xC。
- ctor `FUN_004ADE70`/`FUN_004ADFC0`（scanK）: `mov [ecx],0xA41C40; mov [ecx+0xC],0xA41B1C; mov [ecx+0x30],0xA41B04; mov [ecx+0x3C],0xA41AE4; mov [ecx+0x54],0xA41AD0` ＝ **MI で5つのポリモーフィック・サブオブジェクト**（+0=TESProduceForm 系 4 virtuals、+0xC=TESObjectACTI→TESBoundObject→…→TESForm チェーン 72 virtuals、+0x30/+0x3C/+0x54=BaseFormComponent 系 mixin）。
- ⇒ TESFlora の `TESForm*` は **obj+0xC**（upcast 調整）で、slot19/20 は 0xA41B1C 経由＝TESForm 基底実装を継承（INH）。+0 vptr 経由の slot19/20 は文字列バイト列を読みに行くため到達設計されていない。
- [PORT] 含意: TESFlora 移植時は上記 MI オフセット（+0/+0xC/+0x30/+0x3C/+0x54）と「TESForm* = +0xC」の再現が必須（検証残点 N-31）。

### §1-6 [PORT] 含意（v239 §2-6 / v241 §3-3 の更新材料）

- .ess の per-form 出力は「TESForm 基底実装（0/4B）＋クラス別拡張」の2段。port で再現すべき実装は **OWN 29 クラス＋TESForm 基底＋slot19/20 ディスパッチ**（＋ヘッダ・ZLib・TESFile flush）。INH 57 クラスは基底実装で足りる。
- vtable 上の slot19/20 値は「mid-entry（同一関数の別エントリ）」や「親実装への thunk（Character）」を含む。移植時はアドレスではなく機能単位で写像すること。
- Ni/gamebryo 系の slot19/20（144 ペア中 115 ペア）は移植対象外（Ni ストリーム系）。

---

## §2 項目2（N-25）: FUN_00518000（0x519320 尾分岐 E9→TESForm slot8）の呼び出し文脈

**判定: 【解明】FUN_00518000 本体はエンジン起動期の「既定フォーム生成」ルーチンで、直接呼出しは初期化チェーン1本のみ。v241 の「0x519320 尾分岐」は FUN_00518000 の ret 直後に置かれた独立 5B サンクであり、参照は Script/BirthSign の vtable slot#11(+0x2C) の2件だけ ＝「slot8 への非 CELL 系到達」の正体は vtable エントリであり静的呼出しではない。**

### §2-1 FUN_00518000 本体（decompiled :209000-・scanC C2）

- `FUN_00518000`（0x518000..0x51931A `ret`）: 既定フォーム ID 群（0x212="FootSoundDirt"、0x22B、…）を `FUN_004471A0(id)`→`FUN_009832E6` で解決し、未所持なら `FUN_00517F40` で生成、`FUN_00446CB0`/`FUN_00412D30("FootSoundDirt", …)` へ登録する起動期ルーチン。
- direct 呼出し: **1件のみ** `E8 @0x44CD53 in FUN_0044AC20`（scanC C1）。
- 上流（scanB B3 / scanD D1）: `FUN_00406360`(0x4067C7) / `FUN_00464440`(0x4645B0) → `FUN_0044f3d0` → `FUN_0044ac20`(0x44F6CB) → `FUN_00518000`。いずれも起動初期化帯。

### §2-2 0x519320 サンクの正体（scanC C2 / scanD D1 / scanG G3）

- `0x51931A: ret` の後、int3 塗り（0x51931B..0x51931F）を挟んで **`0x519320: jmp 0x46B340`**（5B E9 サンク）。0x46B340 は **TESForm vtable slot#8(+0x20)** のエントリ値。
- direct E8/E9 呼出し **0 件**、.text 内 imm32 参照 **0 件**。参照は **vtable 2 件のみ**:
  - `0xA49DD0` = Script vtable slot#11(+0x2C)（vt 0xA49DA4、slot#8 は TESForm 同値 0x46B340、slot#11 のみ 0x519320）
  - `0xA52550` = BirthSign vtable slot#11(+0x2C)（同上）
  - 参考: TESForm 自身の slot#11 = 0x46B4C0（FUN_0046B370 の mid-entry）
- ⇒ v239/v241 の記述「slot8 への唯一の非 CELL 系到達＝0x519320」を具体化: **Script と BirthSign の2クラスだけ、slot11(+0x2C＝SaveEditedForm_c 位置) が TESForm slot8（SaveEditedForm_a 側）へjmp するサンクになっている**。retail では slot11 の仮想ディスパッチが到達不能（v241 §1-4）なので実害なし。
- 未解明（残点 N-28）: なぜこの2クラスのみ slot8 実装へ退避するのか（SaveEditedForm のガード差の使い分け）は未走査。

---

## §3 項目3（N-26）: FUN_004CBC20 の 0x4CBCED capstone(+0x2C) vs decompiled(+0x174) 不一致

**判定: 【解明・v241 の仮説を訂正】両方が正しい＝+0x174 ディスパッチと `call [esp+0x2C]` は別々に実在する2つの命令。capstone の skipdata 誤読ではない。0x4CBCED は vtable slot11 ではなく「引数として渡された関数ポインタ param_6 の呼び出し」（base=esp のスタック間接呼び出し）。エンジン帯の slot11(+0x2C vtable) ディスパッチ0件という v241 の結論は維持。**

### §3-1 機械語の実測（scanB B2 / scanC C3・skipdata OFF でクリーン再逆アセンブル）

- `0x4CBC7C: mov eax,[edx+0x174]; … 0x4CBC84: call eax`（1箇所目）
- `0x4CBCC5: mov eax,[edx+0x174]; … 0x4CBCD2: call eax`（2箇所目）
- `0x4CBCE7: mov ecx,[esp+0x28]; push ecx; push esi; 0x4CBCED: call dword ptr [esp+0x2C]; add esp,8`（raw bytes `8b4c24285156ff54242c83c408` で確認）

### §3-2 decompiled との1対1対応（:159854-159901）

- `uVar5 = (**(code **)(*piVar2 + 0x174))(param_2,param_3);`（:159885）↔ 0x4CBC7C 帯
- `uVar5 = (**(code **)(*piVar2 + 0x174))(param_4,param_5);`（:159891）↔ 0x4CBCC5 帯
- `cVar4 = (*param_6)(piVar2,param_7);`（:159895）↔ **0x4CBCED `call [esp+0x2C]`**（param_6 はスタック引数。宣言も `code *param_6`）

### §3-3 裁定

- FUN_004CBC20 は `this+0x48` のリスト走査で、各要素に対して +0x174 の仮想呼び出し（条件/サイズ系）を行い、続けて**呼び出し元から渡されたコールバック param_6** を実行して戻り値で打ち切るルーチン。
- scanB のバケット判定「call [reg+disp]」は **base=esp のスタック読みも同一视**したため、0x4CBCED がエンジン帯唯一の「slot11 相当」に見える罠だった。skipdata による誤デコードではなく判定規則の問題。
- ⇒ v241 §1-4 の件数結論（エンジン帯 slot11 ディスパッチ0件、slot8/11 は retail 到達不能）は **維持**。ただし根拠を「skipdata 誤読」から「スタック間接呼び出し＝vtable ではない」へ訂正。

---

## §4 項目4（N-27）: FUN_005D3230 の引数/0x40f 対応・Menu 決定系 menu-ID 到達

**判定: 【解明＋v241 訂正1件】FUN_005D3230 は引数を取らない SaveMenu 専用の即時セーブルーチン。0x40F は「コンソールメッセージ」ではなく SaveMenu の menu ID で、保存後に FUN_00589B70 でメニュー表を引いてクローズ判定に使う。コンソール "save" の実ハンドラは FUN_005023D0（コマンドテーブル 0xB0BF38）で FUN_00465130 を直接呼ぶ（＝v241 §1-2 の「コンソール→FUN_005D3230」は誤り）。**

### §4-1 FUN_005D3230: 引数なし（scanF F3）

```
0x5D3230: push 0xb ; call 0x57de50        ; UI サウンド "UIMessage"（decompiled :285701 case 0xb/0x14）
0x5D3237: mov ecx,[0xb33b00]              ; TESSaveLoadGame グローバル
0x5D3240: push 0 / push 0 / push 0
0x5D3246: call 0x465130                   ; Save(0,0,0)＝既定名
0x5D324B: push 0x40f ; call 0x589b70      ; menu 表引き（下記 §4-2）
0x5D3255: test eax,eax ; je ret
          （非0のとき）sound 0x57acc0 → call 0x5d2cf0 → jmp 0x5bda20（メニューコーズ系）
```
- 呼び出し側（0x5D3DC2 / 0x5D3438）とも **push なし・thiscall 不使用**＝実質引数0。この関数はコンソールハンドラではなく **SaveMenu の slot3 ハンドラ（FUN_005D3D20）から cmd==0x65(101) のとき呼ばれる「即時セーブ」**（§4-3）。

### §4-2 0x40F ＝ SaveMenu の menu ID（scanE E1 / scanF F3 / decompiled :294974）

- SaveMenu vtable(0xA6D624) slot#13(+0x34) = `0x5D3310: mov eax,0x40F; ret` ＝ **GetMenuID が 0x40F(1039) を返す**（Menu 基底の slot#13 は `xor eax,eax; ret`）。
- `FUN_00589B70(id)`: `if (id-0x3E9 < 0x33 && id-0x3E9 < count) return base[id-0x3E9];` ＝ **menu-ID→エントリのランタイム表引き**（ID 帯 0x3E9..0x41B＝51個。base=[0xB13974]、count=[0xB1397A]。静的画像では count=0＝起動時に構築、書手候補は FUN_0057F170/FUN_005821F0/FUN_005AC990/FUN_00A24910・scanD D2）。返り値は FUN_005898F0（menu-info→オープン中 Menu インスタンス、scanI I4）へ渡される。
- 保存後の流れ: `FUN_005D2CF0` は `push 0x40f; call 0x589b70` → instance → FUN_005898F0 → 仮想呼び出しで **SaveMenu クローズ**（scanI I1）＋ `jmp FUN_005BDA20`。

### §4-3 呼び出し文脈: SaveMenu の2系統（scanF F3/F4・scanH H4・scanI I2/J2）

- direct 呼出しは2件のみ（scanF F1）:
  1. `FUN_005D3D20`（SaveMenu slot#3(+0xC)＝コマンドハンドラ、`ret 8`）@0x5D3DC2: `cmp edi,1`→クローズ系、`cmp edi,0x65`→bit テスト（`FUN_00582160` 経由 obj+0x118>>2&1）。bit クリア時に **cmd==0x65 → FUN_005D3230() 即時セーブ**。bit セットかつ cmd!=0x65（＝スロット指定系コマンド）→ `[esi+0x58]` へ引数保存＋コールバック登録（bit セット時 0x5D3B70、bit クリア時 0x5D3390 を `push imm32` で FUN_00579C10 へ・scanH H4/scanJ J2）＋ flag [esi+0x5C]=1。
  2. `0x5D3390`（int3 境界で分離された独立関数。named_funcs 上は FUN_005d3320 帯）@0x5D3438: **コールバックとして実行される名前付きセーブ**。`FUN_00578D70`(状態)→`cmp bl,2`→`FUN_00589B70(0x40F)`→`FUN_005898F0`→`FUN_009832E6` でスロットエントリ解決→メッセージ `0xFA1`(4001)（`FUN_0058CEB0`, [edi+0x40]）→ `mov ecx,[0xB33B00]; push 0; push 0; push eax; call 0x465130`（**第1引数にスロットエントリ値**＝名前付き）→ sound → `FUN_005D2CF0`/`FUN_005BDA20`。0x5D3B70 も同型（scanI I2b）。
- ⇒ 0x40F 対応まとめ: 「メッセージ 0x40f」＝**SaveMenu の menu ID**。FUN_005D3230 は保存後にこの ID でメニュー表を引いて「SaveMenu が開いているならクローズ」する。

### §4-4 Menu 決定系の menu-ID 到達（scanD D5 / scanE E1/E4 / scanH H5 / 直接呼出し走査）

- **生成**: SaveMenu ctor `FUN_005D3290`（vtable 0xA6D624 代入）の direct 呼出しは **`FUN_00587D20` @0x588556 の1件**（MenuManager バンド＝メニューファクトリ）。dtor `FUN_005D3320` は vtable slot#0 `FUN_005D3630` から（@0x5D3633）。ctor/dtor/FUN_005d39a0 への **imm32 参照はイメージ全体で0件**（表登録ではなく direct 呼出しで生成）。
- **決定ロジック**: `FUN_005821F0` 内に `push 0x40F` が4箇所（0x5837C7 / 0x583851 / 0x583889 / 0x5838A3）。0x40E（LoadGameMenu 相当）・0x3F5 と並列で `FUN_00587B20`→`FUN_005878B0`（オープン可否確認）を走らせ、0x5838A3 ブロックでは `FUN_00589B70(0x40F)`→`FUN_005898F0`→**slot#13(+0x34) 仮想呼び出し→`cmp eax,0x40F`** でインスタンスを検証（scanH H5/scanI I5）。
- その他 0x40F 参照: `FUN_0058A1C0`(0x58A688)、`FUN_005D39A0`（`FUN_005BDDE0` @0x5BDF2C から呼出し＝メニューコーズ系）。

### §4-5 コンソール "save" 経路（v241 訂正材料・scanN N1 と .data 0xB0BF38 ダンプ）

- コンソールコマンドテーブル（.data 0xB0BF38..）の SaveGame エントリ: `{name "SaveGame"/"save", id 0x147, help "SaveGame <filename>", flags 0x20000, sub 0xB0A678, handler 0x5023D0, 0x4FDE30}`。ハンドラは imm32 0x5023D0 が 0xB0BF50 に格納（E8/E9 直接呼出しは0件＝テーブル経由）。
- `FUN_005023D0`: 多数スタック引数を受け、`FUN_004FAE80`（検証）→ 文字列操作（`call [0xA28164]`／`FUN_00982EC0`(0x2E='.' 区切り)）→ **`FUN_00465130` を直接呼出し**（0x5024BA）。
- ⇒ FUN_00465130 への direct 呼出しは全6件で確定（scanF F1: `FUN_00466AD0` quicksave / `FUN_00466B70` autosave / `FUN_00466BE0` 名前付き / **`FUN_005023D0` コンソール** / `FUN_005D3230` SaveMenu 即時 / `0x5D3438`(0x5D3390 帯) SaveMenu スロット）。
- **v241 §1-2 の行「コンソール/メニュー → FUN_005D3230」は訂正**: コンソールは FUN_005023D0 直叩き、FUN_005D3230 は SaveMenu 経路。「全経路が FUN_00465130 に収束」という v241 の結論自体は正しい。

---

## §5 新規残点（本タスクで新たに発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-28 | Script/BirthSign のみ slot11(+0x2C) が `0x519320` サンク（→TESForm slot8）を持つ理由（SaveEditedForm_a/c の使い分けがこの2クラスだけ slot8 に退避する機構）が未走査。retail では slot11 仮想ディスパッチ到達不能のため実害なし | 低 |
| N-29 | コンソール "save" ハンドラ `FUN_005023D0` の内部（filename/デフォルト名構築、`FUN_004FAE80` 検証、`0xB0A678` サブテーブル、0x50244D..0x5024D9 帯）とコマンド id 0x147 の配線が未詳細 | 低 |
| N-30 | `FUN_00589B70` のメニュー表（base [0xB13974] / count [0xB1397A]）の実際の書手（候補: FUN_0057F170 / FUN_005821F0 / FUN_005AC990 / FUN_00A24910）とエントリ構造（menu-ID→Menu インスタンス）の確定 | 低 |
| N-31 | TESFlora の MI レイアウト（+0/+0xC/+0x30/+0x3C/+0x54、TESForm* = obj+0xC）を [PORT] 再現する際のフォームマップ登録キー（upcast）検証。+0 vptr 経由 slot19/20 は文字列データ到達のため禁止であることの裏取り | 低 |

（N-24: **§1 で解明＋表化**（TESFlora 追補・CELL 括弧訂正）。N-25: **§2 で解明**（vtable エントリ2件を特定）。N-26: **§3 で解明＋v241 仮説訂正**（skipdata 誤読説の否定）。N-27: **§4 で解明＋v241 訂正**（コンソール経路・0x40F=menu ID。）本タスクの残点は上記 N-28..N-31。）

---

## §6 検証情報

- **selftest**: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → ALL PASS (4/4)。
- **中断対策実施**: 前回中断分の残存物（%TEMP%\opencode\v244_scanA/A2/A3/A4/B/C/D/E/F/G.py + _out.txt）を全読みしてから続きを実行。新規スキャンは `v244_scanH.py`..`v244_scanN.py`（各 _out.txt 付き）:
  - scanH: TESFlora 全 vtable 行＋網羅性（max-slot≥21 未分類 form=1件=TESFlora）＋フォーム実装アドレス使用数＋0x5D3360..0x5D3470 disasm＋.text imm32 0x40F 全22件＋CELL slot19 入口 0x4D1EB0
  - scanI: FUN_005d2cf0 / FUN_005d39a0 頭部 / 0x5D3B70 / direct callers(0x5D3390・0x5D3B70・0x5898F0) / FUN_005898F0 / FUN_005821F0 の 0x40F 帯
  - scanJ: TESFlora vtable imm32 参照 / 0x5D3390・0x5D39A0 の登録 / FUN_00579C10 頭部
  - scanK: TESFlora ctor（FUN_004ADE70 / FUN_004ADFC0）の vptr 代入
  - scanL: 0xA41C30..0xA41CB0 生ダンプ（4エントリ直後に文字列）＋TESObjectACTI slot19/20 確認
  - scanM: FUN_005023D0（FUN_00465130 の direct 呼出し元）callers/ disasm
  - scanN: imm32 0x5023D0 の .data 参照（0xB0BF50）＝コンソールコマンドテーブル特定
- **EXE**: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・PE 手動解析: ImageBase 0x400000、.text VA 0x401000 raw 0x400 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00）。ESM 本体は未アクセス。web 資料は未取得。
- **入力（読取のみ）**: `tmp/re/v241_port_mgef_residuals6.md`（本体）/ `tmp/re/v239_port_mgef_residuals5.md`（参照）/ `tmp/re/cache/vtable_index.tsv`（**注: タスク記載の `cache/vtable_index.tsv` の実体は `tmp/re/cache/` 配下**）/ `tmp/re/cache/named_funcs.txt` / `docs/Oblivion_decompiled.c`（:4809 / :57509 / :60404 / :78901 / :84349 / :84994 / :159854 / :164556 / :164592 / :209000 / :285660 / :294974 / :337936 / :337975 等を部分読取）。
- **git / ソース**: git 不使用。docs・GT・TODO・cache・移植ソースの編集なし（本ファイルは tmp/re/ への新規作成のみ）。

## §7 本ドキュメントと v241 の相違点（訂正一覧）

1. v241 §1-2「コンソール/メニュー → FUN_005D3230」→ 正: コンソール "save" はテーブル 0xB0BF38 のハンドラ **FUN_005023D0** が FUN_00465130 を直接呼ぶ。FUN_005D3230 は SaveMenu（slot3 cmd 0x65）経路。
2. v241 §1-3「TESObjectCELL: slot19(+0x4C)=0x4D1EB0(FUN_004D1E40)」→ 正: 0x4D1EB0 は FUN_004D1E40（0x4D1E40-0x4D1EAC）とは int3 境界で別関数。decompiled の FUN_004d1e40 本文は slot19 実装ではない。
3. v241 §1-4「0x4CBCED…capstone skipdata 誤読」→ 正: 実在の `call [esp+0x2C]`（param_6 コールバック）。+0x174 ディスパッチ（2箇所）も実在。skipdata 誤読ではなく判定規則（base=esp）の問題。結論（slot11 相当0件）は不変。
4. v241 §1-4「尾分岐 E9 FUN_00518000(0x519320)」→ 補足: 0x519320 は FUN_00518000 の ret 後に置かれた独立 5B サンクで、参照は Script/BirthSign の vtable slot#11 の2件のみ（静的呼出し文脈は存在しない）。
