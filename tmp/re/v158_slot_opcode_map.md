# V158: CC/slot ⇔ opcode 書手の双方向マップ＋残点束 8 件の裁定

- 日付: 2026-10-27（opencode サブエージェント / **分析専任・docs/TODO 編集なし**）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000 / VA=0x400C00+off(.text), 0x400E00(.rdata), 0x401400(.data)）
- 出典（読取のみ）: `v_data_relmap.md` F4 後半 ＋ `v138_b16_alarm_residuals.md` 残点 V-138a/c/d/e ＋ `v139_b17_table2_residuals.md` 残点 V143-a/d ＋ `b24_misc_residuals.md` 残点1 ＋ `b18/b25/b27`（CC/slot 軸）／`cache/{opcode_table.json, call_targets.json, vtable_index.tsv, named_funcs.txt}` ／`docs/Oblivion_decompiled.c`（部分検索）
- 成果物: 本ファイル（新規 1）＋ 表 CSV `%TEMP%\opencode\v158_map.csv`（690 行 + header, 178,270 B, UTF-8）
- 中間証跡: `%TEMP%\opencode/v158_common.py`, `v158_1..v158_22`（.py/.txt/.json、既存ファイル非編集）
- 制約遵守: git / docs / TODO / session-memory / plan / func_list_new / cache / 既存 `tmp/re/*.md` は**無変更**、ESM 未アクセス、新規スクリプトは `%TEMP%\opencode` のみ。

---

## §0 判定サマリ（8 項目）

| # | 残点 | 判定 |
|---|---|---|
| 1 | emit `this`（0x60d950 param_2）のクラス・vtable と `[this+0x1C]` の正体 | **裁定: param_2 は非仮想の参照ラッパ（POD・vtable 不持）で、`[this+0x1C]` は formID でも baseForm でもなく `(16bit serial << 16) \| kind` のパック DWORD**。書手 9 箇所は全て `P=[owner+8]; P+=0x14; [P+0x1c]=(serial<<16)\|kind`（kind=4/6/7/0xF）。GetBaseForm(0x4D9B40) は REFR 家族 primary vtable の実装であり、param_2 は REFR 家族ではない（+0x1C の意味が別）＝ v138 の「矛盾」は解消。**残: 所有者 `[owner+8]` の正体（V-158a）** |
| 2 | extra 0x13 フラグバイト仕様と OnActivate ラッチ、0x4D8270(set) caller 区分 | **解明: type0x13 = `ExtraAction`（RTTI 実測）、payload `node+0xC` は bit0..3 の4ビット（既定 1）。bit0/1 = アクティベート待機ラッチ、bit2 = ドア開閉、bit3 = ドア default-open。OnActivate =「test(bit1) → 成立時 1.0 → 無条件 clear(bit0) → clear(bit1)」の one-shot 消費、Activate opcode 側が set(1)+set(2) → 実行 → clear×2 で二重保証。set thunk 7 caller を 3 エンジン系 + 2 opcode 系に区分** |
| 3 | extra type 0x12 の登録元 0x428BB8 と 0x4FBDC0 の関係 | **解明: type0x12 = `ExtraScript`（RTTI 実測、`+0xC`=script/form、`+0x10`=event list）。0x428BB8 周辺 = `ExtraDataList::CopyListForReference`（文字列 0xA3576C）の type 0x12 case＝**コピー時に新ノードを作り event list はコピーせず `0x4FBDC0` で再生成**。0x4FBDC0 = 0x14B ノード工場 `{+0=self, +0xC=FUN_004FA910(self), +0x10=0}`、cluster 内 23 呼出し = event list 生成/再生成系** |
| 4 | `FUN_00607B60` カウンタの正体 | **解明: `[0xB2EB3C]` = 16bit 単調 serial カウンタ（.data 初値 `0x0A`、wrap→0 は 0x0A へフォールスルー）、`[0xB3B7D4]` = メモ化キャッシュ。bss クリア書手が .text に不在（8 参照 = 読4/書4）→ 初回のみ採番（=0x0B）して以後固定。** **mod index / process id ではない**。`call 0x607B60` は 2 箇所（pack 0x60A3EB / 比較 0x60DA3D）。下限 0x0A は `uVar6==1` 特別経路との衝突回避 |
| 5 | bit14 名称の最終確定（配列用途・0x58 REFR・62 読手） | **配列は save 除外判定ではない**（[0xB0614C]=動的配列コンテナ、[0xB06150]/[0xB06158]/[0xB0615C]=buffer/count/非NULL数、[0xB0613C]=`[form+0xC]` キー hash map。走査者は register/dereg/ClearData リーク検査/CRT init-atexit のみ）。0x58 = **TESObjectREFR インスタンス**（ctor 0x4D9A70、28 malloc サイト）。62 読手のうち直接 save/load serializer を呼ぶのは 0 件（depth-2 proxy 17/62）。dispatcher 0x5169A0 の bit14 は「未立なら 0x441670(TES,ref) 実行」ゲート → **名称は暫定「管理外/一時（not-in-world）」を維持・強化** |
| 6 | World::Update 内のスクリプト/AI 前後＋描画 bit11 | **0x440400 は 12B 尾分岐ディスパッチ**（`[TES+0x34]?→jmp 0x4CCC50 : jmp 0x482470`）。0x4CCC50 は cell+0x48 の参照リストへ `vtable+0x190` ゲート → `vtable+0x160`（Actor/REFR=0x4E3F30, **Player=0x65FF40=FUN_0065FDA0 領域＝AI 対象選別**）を**間接委譲**するだけで、スクリプト/AI を直呼びしない → 前後問題は「同一 vcall 内」に還元。PerFrameUpdate 実測順 = **World::Update → Render::Frame → 0x4F5DB0（b10 expression getter 帯） → flush → EndFrame**。描画側 bit11 読手 = **0x674A20(EndFrame) 2 箇所＋0x4D7E90（Render 内部 0x6AD030 経由）1 箇所のみ**、Render は保留集合を読まず（v139 補強） |
| 7 | MessageHandler ライタ 0x494730 の `[0xB34D8C]` | **[0xB34D8C] の abs32 参照はファイル全体で 1 件のみ＝読手 0x494768 だけで書手不在（近傍 0xB34C00..0xB34D8C も走査済、`[reg+0xC]` 間接書手も 0）→ 恒 NULL bss**。よって `(%s %s) - `（0xA3DA34）＋ vtable+0x28 再帰 virtual call（`[0xB34E80]` reent ゲード付き）は**到達不能（死コード）**。b24 §3.1「retail ベースは no-op」の結論と整合。**クラスは構築経路不在のため特定不能** |
| 8 | opcode → CC/combat フィールド逆引き | **双方向マップ成立**（§2）。strong（hop≤2）で **opcode→field = 175 ペア / 126 opcode / 29 field**、field←opcode = 同 175 ペアを field 側から集計（29 field に ≥1 opcode、relay3 を含む全件では 555 ペア / 269 opcode / 35 field）。direct_call 34 ペアは**境界跨ぎ 0 件で全クリーン検証済** |

---

## §1 手法と入力

| 項目 | 内容 |
|---|---|
| selftest | `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時、bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560） |
| 共通基盤 | `%TEMP%\opencode/v158_common.py` — EXE 遅延読取・台帳（named_funcs）・関数境界・**call target 起点逆アセンブル**・operand ベース解析（vcall = `mov reg,[vtable+slot]; call reg` 検出、直接ストア = write mnemonics×非 esp/ebp base）・call/jmp 直走査 |
| 双方向走査 | `v158_1.py`（opcode impl → depth≤3 前方 BFS、**fan-in ≤60 ゲート**で共有ハブ非展開）＋ `v158_2.py`（`vtable_index.tsv` 全 1,715 本を実読みして vcall slot → impl/クラス解決、CSV 生成） |
| 検証 | `v158_19.py`（direct_call 行の call site が impl 本体内か／int3 パディング跨ぎ無し）、`v158_21.py`（統計） |
| 残点プローブ | `v158_3..v158_18`, `v158_22`（emit this / serial / extra ctor RTTI / caller 区分 / bit14 配列 / World::Update / bit11 / [0xB34D8C]） |
| capstone | 5.0.7（skipdata=True, detail=True）／raw E8 生バイト走査（skipdata 境界罠を回避） |

### 発見・是正した分析基盤のバグ（重要・GT 反映候補）

1. **`func_range` の off-by-one**:「次エントリ」を `_FUNCS[i][1]`（＝**その次**の次）で取っており、**台帳関数を1本多く含む**範囲を返していた → 呼び出しの隣接関数帰属（例: Activate の set/clear が ClearOwnership に誤帰属、OnActivate の clear が OnDrop に誤帰属）。本タスクで `_FSTARTS[i]` に修正。
2. **台帳結合（1 台帳エントリに複数の実体関数）**: `disasm_func` を「va 起点＋va が台帳先頭でないなら最初の int3×3 で打ち切り」に変更し、**呼び出しターゲット自身の実体関数だけ**を解析する方式に統一。
3. **vcall の形態**: 本バイナリの vcall は `mov reg,[eax+disp]` →（push 群）→ `call reg`。`call dword ptr [mem]` はほぼ不在 → op_str ベースの `call [...]` 検出では **0 件**になる罠（operand ベースの load→call 一致で是正）。
4. capstone の op_str は `dword ptr [eax + 0x254]` 形（空白・末尾オペランド付き）→ 単純 prefix 判定でストア/リードを取りこぼす（これも operand ベースで是正）。

---

## §2 双方向マップ（本体）

### 2.1 規模

| 方向 | strong（hop ≤2, relay3 除く） | 全（relay3 = weak を含む） |
|---|---|---|
| **opcode → field**（ペア数） | **175** | 555 |
| 到達 opcode 数 | **126**（impl あり 527 中） | 269 |
| field 数 | **29** | 35 |
| **field ← opcode** | **175 ペア / 29 field**（各 field ≥1 opcode） | 555 ペア / 35 field |

CSV 行数 **690**（evidence 内訳: `direct_call` 36 / `relay2` 50 / `relay3` 99 / `vcall_direct` 116 / `vcall_relay2` 384 / `direct_store` 5）。

CSV 列: `opid, opcode, impl, field, target_va, writer_label, evidence, hop, path, resolved_impls, confidence`。
confidence: 空 = strong 直接、`weak` = relay3（fan-in ゲート通過の深経路）、`unverified` = 直接ストア（**ベースレジスタのクラス帰属未検証**。同 offset の系統混在のため）。

### 2.2 field ← opcode（strong / 全）

| field | strong | all | strong な opcode 例 |
|---|---:|---:|---|
| event check（0x4FB5F0） | 24 | 24 | OnAdd/OnEquip/OnUnequip/OnDrop/OnHit/OnHitWith/OnDeath/OnMurder/OnKnockout/OnAlarm/OnAlarmVictim/OnPackage×3/OnLoad/OnMagicEffectHit/OnSell/OnTrigger×3/OnStartCombat/OnActorEquip/OnActorUnequip/OnReset（＝b16 の 22 ハンドラ＋共有 impl 2。**NO-CHECK 群 0x2/0x7/0x11/0x12/0x13 等は不在＝b16 と一致**） |
| emit key（vt+0x170 = GetBaseForm 0x4D9B40） | 20 | 77 | GetSelf/GetActionRef/IsOwner/SetOwnership/IsCellOwner/SetCellOwnership/SetClass/SetLevel/SetBarterGold/SetItemValue/SendTrespassAlarm/SexChange/BetaComment… |
| vt+0x154 | 20 | 40 | SetPos/SetAngle/Rotate/PlayGroup/AddDecal/SetActorFullName… |
| vt+0x174（位置） | 15 | 36 | SetPos/CalcPathToPoint/Autosave/CreateFullActorCopy… |
| vt+0x40（変化通知） | 14 | 33 | PlayGroup/ShowMap/QuitGame/SetPCFaction*/ModFactionRank/SetDoorDefaultOpen/SetForceSneak… |
| **REFR+0x44/extra0x12（ExtraScript）** | 10 | 28 | **AddScriptPackage[direct] / RemoveScriptPackage[direct]**、AddItem / EquipItem / CompleteQuest / EvaluatePackage / GoToJail / ShowVars / SetActorAlpha / SetActorRefraction |
| vt+0x190 | 10 | 136 | Disable/PlayGroup/SetPackDuration/ResetHealth/StartCombat… |
| vt+0x164（GetParentCell） | 9 | 16 | ShowAnim/SetAngle/SetAtStart/PickIdle… |
| **REFR+0x44/extra0x13（ExtraAction）** | 7 | 10 | **OnActivate[direct] / Activate[direct] / Lock[direct] / SetDoorDefaultOpen[direct]**、GetDoorDefaultOpen[relay2] / SetOpenState[relay2] / IsSpellTarget[relay2] |
| vt+0x90（bit1/bit14 登録系 FUN_0046c640） | 6 | 7 | PlayGroup/LoopGroup/SetScale/ModScale/GetSecondsPassed/IsIdlePlaying |
| Actor+0xB0（死亡状態） | 5 | 14 | KillActor / KillAllActors / SetUnconscious / SetRestrained / RevertWorld |
| CC getter（vt+0x330 → CombatController） | 4 | 10 | **StartCombat / Yield / ForceFlee / ForceTakeCover**（vcall_direct） |
| IsDead slot（vt+0x198） | 3 | 19 | StartCombat/AddScriptPackage/ShowQuestTargets… |
| **REFR+0x44/extra0x3D** | 3 | 3 | **SetCrimeGold[direct 0x4269E0]**、IsCellOwner / IsHorseStolen（共有 0x4F8B90 経由の読手） |
| disable queue（0xB361CC） | 3 | 5 | **Enable[direct 0x4FA540]**、GetDisabled[relay2]、GetActorValue[relay2] |
| **slot+0x250（meter getter）** | 3 | 9 | **SetCrimeGold / ModCrimeGold / PayFineThief（vcall_direct）** ＋ weak: GetCrimeGold / PayFine / GoToJail / CanPayCrimeGold ほか（共有 evaluator 0x4F5230 経由） |
| **slot+0x254（meter setter）** | 2 | 5 | **ModCrimeGold / PayFineThief（vcall_direct）** ＋ weak: SpeakSound / PayFine / GoToJail |
| **CC+0x40（target 容器）** | 2 | 2 | **Yield / ForceFlee**（relay2: 0x61DA10/0x6210D0 → 0x6162D0） |
| **CC+0x6C（アクション状態 SetState 0x619920）** | 2 | 3 | **Yield / ForceFlee**（relay2）＋ weak: ForceTakeCover（relay3） |
| **CC+0x70（intent setter 0x612DE0）** | 1 | 2 | **ForceFlee**（relay2）＋ weak: Enable（relay3） |
| form flag bit11（0x46ABA0/0x4E4690） | 1 | 2 | **Enable**（relay2: 0x50A150→0x4DD850→0x46ABA0） |
| form flag bit14（0x46B590） | 1 | 3 | **AddItem**（relay2、＝v139 の「AddItem → 0x46A020 → bit14」経路）＋ weak: PlaceAtMe / CreateFullActorCopy |
| vt+0xEC / vt+0x2A4 / vt+0x184 / vt+0x188 / vt+0x284 / vt+0x304 / vt+0xF8 | 3/2/1/1/1/1/1 | （合計 64） | スケール・HP 修正・process getter・slot59 ゲート等 |
| CC slot58 dispatch（vt+0xE8） | 0 | 2 | weak のみ: KillActor / KillAllActors（relay3） |
| process death（vt+0x364/+0x370） | 0 | 3 | weak のみ: KillActor / KillAllActors / EvaluatePackage |
| +0x58 (direct store, class unverified) | 0 | 3 | SetPos/SayTo/PositionCell 系（**クラス帰属未検証**） |

### 2.3 opcode → field の代表経路（relay の実例、`path` 列より）

| opcode | field | 経路 |
|---|---|---|
| `0x100D Activate` | extra0x13 SET/CLEAR | `0x507650 > 0x4D8270 (push 1 / push 2)` → `0x507650 > 0x4D82E0`、その間に `0x4DD260`（`'%s' activated %s '%s'`） |
| `0x2 OnActivate` | extra0x13 TEST/CLEAR | `0x503F90 > 0x4D8260(push 2)` / `0x4D82E0(push 1)` / `0x4D82E0(push 2)` |
| `0x10D8 SetDoorDefaultOpen` | extra0x13 SET/CLEAR/TEST | `0x50C430 > 0x4D8260/0x4D8270/0x4D82E0`（いずれも push 8 = bit3） |
| `0x1075 SetCrimeGold` | extra0x3D / slot+0x250 | `0x50B4A0 > 0x4269E0`（直）＋ `vt+0x250` vcall（直） |
| `0x1076 ModCrimeGold` | slot+0x254/+0x250 | `0x504D00` 内で `vt+0x254` / `vt+0x250`（vcall_direct） |
| `0x105F Yield` | CC+0x6C/+0x40/CC getter | `0x501DD0 > 0x61DA10 > 0x619920(SetState)` ／ `> 0x6162D0` ／ `vt+0x330` vcall（直） |
| `0x10F5 ForceFlee` | CC+0x70/+0x6C/+0x40 | `0x50C790 > 0x6210D0 > 0x612DE0 / 0x619920 / 0x6162D0` |
| `0x1002 AddItem` | extra0x12 / bit14 | `0x507320 > … > 0x46A020 > 0x46B590(bit14)` と event list 登録系 |
| `0x1021 Enable` | disable queue / bit11 | `0x50A150 > 0x4FA540(erase)` 直＋`> 0x4DD850 > 0x46ABA0 / 0x4E4690` |
| `0x1097/0x1098 Add/RemoveScriptPackage` | extra0x12 emit | `impl > 0x4FBF90`（直、mask 付 emit） |

### 2.4 マップの信頼度と落とし穴

1. **strong の `direct_call`(hop1) は 34 ペアすべて call site が impl 本体内・境界跨ぎ 0 件（`v158_19` 検証）**。
2. `relay2` は中間関数が fan-in ≤60 である場合のみ展開。中間が共有 evaluator（0x4F5230 / 0x4F8B90 / 0x4F5730 帯）だと **opcode 固有の書手ではない**場合がある（例: GetCrimeGold の meter vcall は共有 evaluator 内）。
3. `relay3` は一律 `weak`。大ハブ（0x579B60=516, 0x4FAE80=278, 0x9832E6=1819, 0x401F20=7573, 0x401F00=4845, 0x446CB0=347, 0x6ECC80=715 …）は非展開。
4. **vtable slot はクラス依存**（例 vt+0x250 = Character/Player float / HighProcess byte / Actor 定数）。CSV の `resolved_impls` 列に vtable_index 実測の impl→クラス列を付与済み。
5. **同 offset の系統混在**: 直接ストア行は `class unverified`（Actor+0x58=process と CC+0x58=latch の別物等）。クラス判定にはベースレジスタの由来追跡が必要（V-158f）。

---

## §3 残点 8 項目の詳細

### §3-1 (1) emit `this`（V-138a）— **クラス: 非仮想ラッパ / `[+0x1C]`: パック serial**

**出所（2 呼出しのみ、raw E8 実測）**

```
call 0x60d950 = 2 : 0x60C8DF（FUN_0060C5B0 内）, 0x60DBD9（FUN_0060DBB0 内, ret 4）
0x60C81C  mov ecx, [esi+eax+0x10]     ; レコード +0x10
0x60C814  fld dword ptr [esi+eax+0x14] ;            +0x14 (float)
0x60C81C  mov eax, [esi+eax+0x20]      ; ★ +0x20 = wrapper
0x60C820  push eax  →  call 0x4806E0 / 0x4DC270 → 引数解決
0x60C8DD  push eax (=arg2) ; push edi (=arg1) ; call 0x60d950
2nd caller: param_2 = [arg1+0x28]（push 順序から厳密に確認）
```

**wrapper の構造（`0x60d950` / `0x4806E0` / `0x47DE00` / `0x8AFCE0` で一致）**

| offset | 意味 | 根拠 |
|---|---|---|
| `+0x10` | **相対ポインタ**（`*(+0x10) + self` = 解決先） | `mov eax,[ecx+0x10]; add eax,ecx`（0x47DE04..0x47DE0D、mode==1） |
| `+0x18` | **モード byte（1 / 2）** | `cmp byte [ecx+0x18],1` / `,2`（4 ヘルパ共通） |
| `+0x1C` | **`(serial<<16) \| kind`** | 読: 0x60DA37 `mov ebp,[ebx+0x1c]; shr ebp,0x10` → 0x60DA3D `call 0x607b60; cmp ebp,eax; je 早返り`。書: 下表 9 箇所 |

**パック書手 9 箇所（全 `.text` 走査、いずれも `shl reg,0x10` → `or reg,kind` → store）**

| site | kind | store 形 |
|---|---|---|
| 0x609594 / 0x60966F / 0x60998A | 0xF / 4 / 4 | `ecx=[esi+8]; add ecx,0x14; mov [ecx+0x1c], eax` |
| 0x5354A3 / 0x538ACC / 0x89F503 | （reg 由来 kind） | `eax=[ecx+8]; add eax,0x14; mov [eax+0x1c], edx` |
| 0x695D77 / 0x6977BB / 0x69D0F2 | 7 | `eax=[ebx+0x364] → [eax+8] → add 0x14 → mov [eax+0x1c], esi`（`shl esi,0x10`） |

→ 書手は **全て `P = [X+8]; P += 0x14` の P+0x1C（= X+8 の指すオブジェクトの +0x30）** へ書く。0x60d950 の読手は `[param_2+0x1C]`。同一フィールドとみなすには **param_2 = (X+8)+0x14（入れ子構造の内側オブジェクト）** である必要があり、これは `[+0x1C]` が **formID でも baseForm ポインタでもない**ことの直接証拠になる。

**裁定**

- **`[this+0x1C]` = パック DWORD（serial 16bit + kind 4〜16bit）**。formID なら `0xAABBCCDD` 形、baseForm ならヒープポインタであり、`(serial<<16)|4` のような構成は取り得ない（kind 実測値 4/6/7/0xF、serial は 0x0A 以上の 16bit 値）。
- **class / vtable**: wrapper は 0x60d950・0x4806E0・0x47DE00・0x8AFCE0 のいずれでも **`[wrapper]` を vtable として使わない**（0 引数 vcall ゼロ）。dynamic_cast（`__RTDynamicCast` 0xB03054→0xB03070 = TESObjectREFR→Actor）は**解決後オブジェクト**に対してのみ実行 → **param_2 自体は非仮想 POD ラッパであり、RTTI vtable は存在しない（vtable 同定は成立しない）** と裁定。
- したがって **GetBaseForm（0x4D9B40 = `mov eax,[ecx+0x1c]; ret`、slot +0x170 impl、7 クラス共通）と param_2 の +0x1C は別フィールド**であり、v138 §2-2(c) の「矛盾」は「**REFR 家族と wrapper で +0x1C の意味が違う**」という形で解消。
- emit は wrapper の解決先（Actor/REFR）に vcall +0x170 で key を作る（0 引数）→ param_2 は **key 生成元ではなく解決用ハンドル**。

### §3-2 (2) extra 0x13（V-138c）— **`ExtraAction` / 4bit フラグ / one-shot ラッチ**

**クラス同定（新規・決定的）**

```
ctor 0x429D30:  [eax+4]=0x13 (type) / [eax]=0xA357F4 (vtable) / [eax+0xC]=1 (既定 payload) / [eax+0x10]=0
vtable 0xA357F4 → vtable_index #650 = ExtraAction, `.?AVExtraAction@@`, td 0xB03E58, slots=2
```

**ビット仕様（mask 実測 = {1,2,4,8} のみ）**

| bit | 値 | 使う側（実測） | 暫定意味 |
|---|---|---|---|
| bit0 | 0x1 | 既定 payload（ctor）。Activate が **set** / OnActivate・Activate が **clear** | ラッチ基底（常に立ったままの状態ビット） |
| bit1 | 0x2 | **Activate が set**、**OnActivate が test→clear**、0x5FBB10/0x660910 系が test | **アクティベート待機（one-shot）ラッチ** |
| bit2 | 0x4 | door 開閉系 `0x4B8A40`('Open'/'Close'/'DRSLocked')・`0x4DE460`(base formType 0x18) が set/clear、`0x4DA3E0` が set、**Lock(0x1072) が test(4)** | **ドア開閉状態** |
| bit3 | 0x8 | **SetDoorDefaultOpen(0x10D8) が set/clear/test(8)**、`0x4D6E60`/`0x4E00F0`/`0x4E0580`/`0x4D9C30`/`0x4DF1A0`/`0x4F5730` が test(8) | **ドア default-open** |

**ヘルパ本体（新規実測）**

- `0x41F830` TEST core: `push 0x13; call 0x41E210; test eax; je → al=0; movzx eax,[eax+0xc]; test mask,al; setne al; ret 4`
- `0x423DF0` SET core: `flags = (node? node[+0xC] : 1) | arg`、node 無く flags≠1 なら `0x41EB90`(GetOrCreate) → `[new+0xC]=flags`
- `0x4D8270` SET thunk: `arg==4` のとき `[0xB33B00]` 経由 `0x4533F0` の bit0x40000 と **bit3 の状態**を判定して `vtable+0x44(0x80000)` 通知 → `0x423DF0`。それ以外は単純 `0x423DF0`
- `0x4D82E0` CLEAR thunk: 同様の arg==4 特殊系 + `0x423E50`

**caller 区分（件数 = cache と一致）**

| helper | 件数 | 区分 |
|---|---:|---|
| `0x41F830` TEST core（直） | **17** | thunk 内部 6（0x4D8270×2, 0x4D82E0×2, 0x4D8260→は別 thunk 経由）＋ engine 直 11（0x4D6E60, 0x4D9C30, 0x4DA3E0×2, 0x4DD260, 0x4DE660, 0x4DF1A0, 0x4E00F0×3, 0x4E0580×2 …） |
| `0x423DF0` SET core（直） | **6** | 0x4D8270×2 ＋ 0x4D9D00, 0x4DD260, 0x4E00F0, 0x4E0580 |
| `0x423E50` CLEAR core（直） | **4** | 0x4D82E0×2 ＋ 0x4E00F0, 0x4E0580 |
| `0x4D8260` TEST thunk | **9** | door 0x4B8A40(4), 0x4F5730(8), **OnActivate(2)**, **Lock 0x504A10(4)**, **SetDoorDefaultOpen(8)×2**, 0x5FBB10(2), 0x660910(2)×2 |
| **`0x4D8270` SET thunk** | **7** | **①door 開閉 `0x4B8A40`（4, reg）②`0x4DA3E0`（4, flag0x8000000+`0x4d6d40`+`0x485E00`＝bit14 系 0x58 REFR 群）③`0x4DE460`（4, `vt+0x170`→base formType 0x18＝ドア）④opcode `Activate`(0x100D)（1, 2）⑤opcode `SetDoorDefaultOpen`(0x10D8)（8）** |
| `0x4D82E0` CLEAR thunk | **9** | 0x4B8A40×2(4), 0x4DA3E0(4), 0x4DE460(4), **OnActivate(1,2)**, **Activate(2,1)**, **SetDoorDefaultOpen(8)** |

**OnActivate「test → clear×2」ラッチ設計（0x503F90 全文、新規）**

```
call 0x4FAE80 (gate) → 不成立なら result=0.0 のまま return
if (esi == 0) → test/clear をスキップして al=1
push 2 ; ecx=esi ; call 0x4D8260      ; test bit1
  成立時: fld1 → [out] = 1.0           ; ← イベント成立
push 1 ; call 0x4D82E0                ; clear bit0（無条件）
push 2 ; call 0x4D82E0                ; clear bit1（無条件）
al=1; ret
```

対になる **`Activate`(0x507650) 側**: gate → 対象/bit11 ゲート → **`set(1)` → `set(2)` → `0x4DD260`（`'%s' activated %s '%s'` = アクティベート実行）→ `clear(2)` → `clear(1)`**（引数非 0 経路はラッチを通さず `0x4DD260` のみ）。
→ **設計**: ラッチは「アクティベート実行の同期中に OnActivate ハンドラが1回だけ消費する」ワンショット。OnActivate が consume 済みでも Activate 側の後続 clear で**冪等に**片付く。v138 の report 訂正（OnActivate は set ではなく clear）を維持し、**set 側の出所 = Activate opcode** を新たに確定。

### §3-3 (3) extra 0x12（V-138d）— **`ExtraScript` / CopyListForReference / event list 再生成**

```
ctor 0x429F20: [eax+4]=0x12 / [eax]=0xA35884 (vtable) / [+0xC]=arg / [+0x10]=0
vtable 0xA35884 → vtable_index #711 = ExtraScript, `.?AVExtraScript@@`, td 0xB03BB8, slots=2
```

- **0x428BB8 の所属 = `FUN_00428B10` = `ExtraDataList::CopyListForReference`**（`push 0xA3576C` → 文字列実測）。`movzx eax, byte[esi+4]; add eax,-0x12; cmp eax,0x36; ja; jmp [eax*4+0x428C74]` = **type 0x12..0x48 の extra コピー分岐**（先頭3エントリ `0x428BA2 / 0x428B78 / 0x428C3C`）。
- **type 0x12 case（0x428BC5..）**: 旧 ExtraScript の `+0xC` を読 → `malloc(0x14)` → `0x429F20(+0xC=旧値)` → `0x41E0A0` でリンク → **旧 `+0xC` を this にして `0x4FBDC0` 実行** → 新ノードの `+0x10` へ格納。
  ⇒ **フォームコピー時、ExtraScript の script payload は複製、event list（+0x10）は複製されず 0x4FBDC0 で作り直される**（参照ごとに独立）。
- **`FUN_004FBDC0` 本体（新規）**: `node = malloc(0x14); {+0 = param_1(this), +4=0, +8=0, +0xC = FUN_004FA910(this), +0x10 = 0}; return node`。decompiled L195420 と一致。**cluster 内 23 呼出し / 13 関数**（cache 一致）＝ event list の初回生成・再生成工場。
- `FUN_004FA910(this)`: `malloc(8)` ゼロペア生成 → `lea ebx,[this+0x48]` のリストを走査し `malloc(0x18)` 複製（`[elem+0x10]` byte を読む）＝ **オーナーの +0x48 リスト複製ヘルパ**（0x4FBDC0 の `+0xC` 供給源）。
- 関連 caller 計測（cache 一致）: `0x41E920`=25(13 funcs) / `0x41F130`=20 / `0x41F670`=4 / `0x41F0B0`=**0**（外部 caller 不在）/ `0x4FBE00`=20 / `0x4FBDC0`=23。

### §3-4 (4) `FUN_00607B60` カウンタ（V-138e）— **16bit serial + メモ化キャッシュ**

```
0x607B60: eax=[0xB3B7D4]; if(eax) return eax          ; キャッシュ
          eax=[0xB2EB3C]+1 & 0xFFFF; [0xB2EB3C]=eax
          if(eax==0){ eax=0x0A; [0xB2EB3C]=0x0A }     ; wrap は 0x0A へ
          [0xB3B7D4]=eax; return eax
```

| 検査 | 結果 |
|---|---|
| `[0xB2EB3C]` 初値（raw .data 実測） | **`0x0A`**（生バイト `f4f8a200 0a000000 d878a900…`） |
| `[0xB3B7D4]` 初値 | bss = 0 |
| 全ファイル imm32 参照 | `[0xB2EB3C]` = **42**（14 系統 × read/add/write トリプレット）＋ 0xB2EB40 隣接1、`[0xB3B7D4]` = **8**（= 0x607B60, 0x6094C0, 0x6095B0, 0x6098C0 の各 read+write のみ） |
| **キャッシュのクリア書手** | **不在**（8 参照はすべて read/test と非 0 write）→ 初回採番後は恒 nonzero = **serial はプロセス内で1回のみ変わり得ない** |
| `call 0x607B60` | **2 箇所**: `0x60A3EB`（`shl eax,0x10; or eax,6` → pack）、`0x60DA3D`（`cmp ebp,eax` → 同値なら早返り） |
| 非メモ化インクリメント系統 | 0x531D80 / 0x5323xx / 0x5F0590 / 0x695A50 / 0x6970A0 / 0x69A490(+0x69A8xx) / 0x69CDF0 / 0x889C20 / 0x890C00 帯（毎回 +1、下限 0x0A 同一） |

**裁定**: `[0xB2EB3C]` = **"event/alarm stamp" 用の 16bit 単調カウンタ（mod index でも process id でもない）**。`[0xB3B7D4]` は **「同一処理ブロック内で serial を固定するためのメモ化」**（0x607B60/0x6094C0/0x6095B0/0x6098C0 の4系統のみ使用）。下限 0x0A は 0x60d950 の `uVar6 == 1` 特別経路（および 0=未スタンプ）と衝突しない値域を保証。emit 側 dedup は「`[param_2+0x1c]>>16` == 今採番値なら処理済み」として働く（§3-1）。

**注意（隣接変数の混同回避）**: `[0xB3B7D0]` は**別変数**（0x60796D `+1` / 0x608250 `-1` / 0x60CD2A で `[0xB370D0]` 超過時 `0x608120` 投入 / 0x4E471E で `>0` のとき `0x607B90` 呼び出し ＝ **alarm 系オブジェクト生存数**）。

### §3-5 (5) bit14（V143-a）— **配列の実体と save 非関与、0x58 REFR、62 読手**

**配列/マップの実体（abs32 全走査）**

| 地址 | 参照数 | 実体 |
|---|---:|---|
| `0xB0613C` | 96 | **hash map（`[form+0xC]` キー）**。`0x46B2C0`=`0x4524C0` で削除、`0x44AC20`（ESM ロード初期化系、文字列 'eyeReanimate'/'VampireRace' 等）ほか |
| `0xB0614C` | 2 | **動的配列 container オブジェクト本体**（`mov ecx,0xB0614C; call 0x449070` = 挿入／`0xA18508` atexit） |
| `0xB06150` | 6 | container **buffer ポインタ**（0x4492E0 ClearData 走査 / 0x46B2C0 除去 / 0x46C640 二重検査 / 0x9DBE90 init / 0xA18500 atexit） |
| `0xB06158` | 10 | **count** |
| `0xB0615C` | 3 | **非 NULL 要素数**（0x46B2C0 が除去時に `-1`） |

**用途の裁定**: `FUN_0046C640`（vt+0x90, arg≠0）= bit1/bit14 **未立なら `0x449070` で container へ挿入 → `[form+8] |= 2`**（bit14 立ちは**挿入スキップ**）、`FUN_0046B2C0` = **bit14 立ちは early-return**、未立なら map 削除＋配列除去＋`[0xB33A98]` 有り時に `0x446B40([form+0xC])` 通知。配列を**走査する唯一の本体**は `FUN_004492E0`（`"Form '%s' (%08X) of type %s in file '%s' was not freed."` / `"Forms were leaked during ClearData."`）＝ **ClearData 時のリーク検査**。
⇒ **「save 書出除外判定」ではない**（[0xB06150] の参照6件に save 系 writer/走査は不在）。実体は「**bit1 登録＝管理下フォームの master 配列＋FormID 系 key map**」。

**bit14 読手62件の save 経路該当（`shr reg,0xe` = 62、v139 と一致）**

- **`0x45B9A0`（save serializer）/ `0x4534D0`（load serializer）を直接呼ぶ読手 = 0 件**。隣接系統で該当するのは `0x46C280`（TESForm dtor）が `0x45B780`（通知）を呼ぶのみ。
- depth-2 proxy（serializer 到達）で **17/62** が save/load 系に接続（0x4433A0, 0x4610F0, 0x464060, 0x4CD3B0, 0x4D56A0, 0x4DDCB0, 0x4DF1A0×2, 0x4E0F80, 0x4E2920, 0x5169A0, 0x5F13D0, 0x612150, 0x625140, 0x62EAA0, 0x646A80, 0x659F90）。ただし**これらは「DTOR/更新系が serializer を呼ぶ」側の到達**で、bit14 を「書かない」分岐として読んでいるわけではない。
- 決定的な1件: **dispatcher `0x5169A0` の 0x516E56** は `dynamic_cast<TESForm→TESObjectREFR>` 後、`shr 0xe; test; jne skip` → **bit14 立ちは `0x441670(TES, ref)`（＝world 接続系処理）をスキップ**（未立なら `[ref+0x40]`＝親セルの `+0x26==6` 検査を経て実行）。
- ⇒ **62 読手の save 書出該当は「書出そのものの除外条件としては不成立」**。save/load 経路での bit14 は DTOR/更新/ディスパッチ側のガードとして現れる。

**0x58 REFR**: `FUN_00401f00(0x58)` = **28 サイト**（decompiled 計測）。ctor **`FUN_004D9A70`** = `[esi]=0xA46C44`（TESObjectREFR primary）/ `[esi+0x18]=0xA46C38`（secondary）/ `formType(+4)=0x31` / `+0x3C=0` / `lea ecx,[esi+0x44]; call 0x422EE0`（BaseExtraList 初期化）→ **0x58 バイト = TESObjectREFR インスタンスの sizeof**。v139 の「malloc(0x58)+0x4D9A70 → bit14 → イベントリスト登録（AddItem/0x58 REFR 群）」と整合 → **実行時生成・ワールド未登録の一時 REF**。

**bit14 名称**: 配列（登録リスト）に**登録されない／解除されない**フラグである以上、v139 の暫定 **「管理外/一時（not-in-world / unmanaged）」を維持・強化**。`Deleted` 説は引き続き棄却寄り（`DeleteForm()` は setter 経路に不在＝v139 §6-3(d)）。

### §3-6 (6) World::Update と描画 bit11（V143-d）

**World::Update = `0x440400`（12B 尾分岐、台帳は FUN_004402F0 と結合）**

```
0x440400  mov eax,ecx ; mov [0xB33C28],0        ; フレーム状態リセット
          mov ecx,[eax+0x34] ; test ; jne → jmp 0x4CCC50 (ecx=TES+0x34)
          mov ecx,[eax+8]                    → jmp 0x482470 (ecx=TES+8)
```

**`0x4CCC50`（参照更新本体）**: `0x496EA0(0xB35C80, key)` → `[cell+0x26]==6` のとき **`cell+0x48` の連結リスト**を走査 → 各要素へ **`vtable+0x190`（ゲート）→ `vtable+0x160`（Update）** を間接呼び出し → 末尾 `0x496F50(0xB35C80, key)`。

| slot | REFR 家族 impl（vtable_index 実測） | 本体 |
|---|---|---|
| `+0x190` | **Actor/Character/Creature/Player = `0x977C50`** | **`mov al,1; ret`（常に真の no-op スタブ）**。同一アドレスは opcode `TestCode/BeginTrace/ShowNameMenu/TrapUpdate/AddAchievement/…`（7 opcode）の impl でもある |
| `+0x190` | TESObjectREFR/Projectile 系 = `0x69D990`、Base/Low = `0x60D880`、High/MidHigh = `0x634910` | — |
| `+0x160` | **Actor/Character/Creature/REFR = `0x4E3F30`**、**PlayerCharacter = `0x65FF40`**、HighProcess = `0x629810`、Base 系 = `0x68F970` | **Player の `0x65FF40` は台帳上 `FUN_0065FDA0` の領域**（`lea edi,[ecx+0x704]` ＋ `shr eax,0xb` ＋ `call 0x4FA560` ＝ v139 §2-6 の **AI 対象選別**） |

⇒ **World::Update 本体はスクリプト実行も AI tick も直呼びしない**。両者は参照ごとの **vtable+0x160 実装**へ間接委譲され、**Player の +0x160 内に AI 対象選別が入る**（＝「World::Update 内でスクリプト実行 → AI tick」の**直列順位は存在せず、同一仮想スロット経由の逐次処理に還元**）。スクリプト実行（event 発火・opcode 実行）は `+0x160` 実装側の深い階層にあり、本タスクの depth 到達では未特定 → **V-158d に継続**。

**PerFrameUpdate の実測 call 順（s80 §C へ追記すべき新事実）**

```
0x40DB7B call 0x445DF0
0x40DBA5 call 0x440400  ; World::Update（参照更新）
0x40DBB4 call 0x6AE860  ; Render::Frame
0x40DBD1 call 0x4F5DB0  ; 台帳上 FUN_004F5730 = b10 の expression getter 帯
                         ;   （'Is Spell Target >> %0.2f' 等の文字列実測）
0x40DBDC call 0x4402F0  ; flush（保留集合適用）
0x40DBEC call 0x674A20  ; EndFrame / present
0x40DC2A call 0x678510 / 0x40DC3E call 0x674A20 / 0x40DC52 call 0x674950
```

⇒ **World::Update（スクリプト/参照更新）→ Render → 条件評価帯(0x4F5DB0) → flush → EndFrame**。s80 §C の番号24〜28 に 0x4F5DB0 の帰属（b10 getter 帯・Render の後・flush の前）を追記する価値あり。

**描画側 bit11 ルート（`shr reg,0xb` 走査: 174 サイト / 136 関数）**

| 経路 | 実測 |
|---|---|
| Render::Frame(`0x6AE860`) の forward 到達集合（depth4, fan-in≤400、122 関数）に含まれる bit11 読手 | **`0x4D7E90` の1関数のみ**（逆方向: Render 内部 `0x6AD030` から ≤3 ホップで到達） |
| **`0x674A20`（EndFrame 段）自体** | **2 箇所** `0x674AF7` / `0x674D05`：`[obj+8]` の bit5(0x20) と併用で、**bit11 立ち（＝disabled）または bit5 立ちなら当該要素をスキップ**、`[obj+0x58]`（process）生存確認つき |
| `0x4D7E90` の意味 | 述語: `[ref+8]` bit11 立ちで許可 byte 0 なら偽 → bit5 → `[ref+0x40]`（親セル）照合 ⇒ **無効/別セル参照を除外するフィルタ** |
| vtable slot impl として bit11 を読む仮想到達可能な「IsDisabled 相当」 | **12 関数**（TESObjectREFR slot21 `0x4E00F0` / slot22 `0x4DA2D0` / slot24 `0x4E0580` / slot83 `0x4E4080`、Actor slot7/22/23/27/36/45/84/205、MobileObject slot22/24、Process 系（HighProcess slot3/27/42/102/104/107/324〜350 等）、**CombatController slot58 `0x617E80`**、**ScriptEffect slot15 `0x6A4620`**、EffectSetting slot45 ほか — `v158_12.txt` 全一覧） |
| 保留集合（`0xB361CC`） | Render::Frame 到達集合に **読手なし**（`0x4FA560` 不在）→ **描画は bit11 だけを見て保留集合は見ない**（v139 §2-6 の結論を裏付け） |

### §3-7 (7) `[0xB34D8C]`（b24 残点1）— **書手不在＝恒 NULL（該当経路は死コード）**

| 検査 | 結果 |
|---|---|
| `0xB34D8C` の全セクション abs32 参照 | **1 件のみ = `.text 0x494768`（= 0x494730 内の読手）** |
| 近傍 `0xB34C00..0xB34D8C` の abs32 走査 | `0xB34C80`（path builder 0x494480/0x4944F0/0x494560/0x4945E0）、`0xB34D84`（0x497B20 が read/clear/restore）、`0xB34D88`（0x4A57E0/0x4BECF0/0x4CD960/0x4DF820/0x52B0C0/0x590420 のロールバック群）は存在するが、**`0xB34D8C` への書手および base+off 間接書手（0xB34D80 系）は 0 件** |
| 使用箇所（0x494730） | `eax=[0xB34D8C]; if(!eax) goto 0x4947BF` → 以降（`[0xB34E80]` reent ゲード → `sprintf("(%s %s) - ", [obj+0xE0], [obj+8]or0xA309A4)` → `vtable+0x28` 再帰 virtual call）は**到達不能** |

**裁定**: `[0xB34D8C]` は **構築経路を持たない bss ポインタ（恒 NULL）** → MessageHandler ライタの「コンテキスト接頭辞付き本文書き」は retail では**死コード**。b24 §3.1 の「retail ベース実装は sev1/7 no-op、シンク 0xB333F80 の vtable 参照は init/dtor の2件のみ」と**完全に整合**（`[0xB34E00]`＝ライタ用静的オブジェクト、`0x9DC921`/`0xA18981` の2参照も init/dtor）。
**コンテキストオブジェクトのクラス**: 書手不在＝生成コードが存在しないため**クラス同定は原理的に不能**。到達不能経路の記述として `+8` = 埋込み文字列（先頭 byte 0 判定）、`+0xE0` = 2 つ目の文字列、を仮定として記録するにとどめる。

### §3-8 (8) opcode → CC/combat フィールド逆引き — 見出しは §2

relay リレー（opcode impl → vcall/書手 → field）を **690 行 CSV** に機械化。コア結論:

- **CC 状態へ直接書く opcode は極少数**: `+0x6C` へは **Yield / ForceFlee**（relay2: 0x61DA10・0x6210D0 → 0x619920）、ForceTakeCover（relay3）。`+0x70` へは **ForceFlee** のみ。`+0x40` へは **Yield / ForceFlee**。**CC へは 4/4 とも「戦闘状態の強制遷移系」opcode のみ**で、汎用 opcode から到達しない（SetState 44 caller は全て 0x61C941–0x624F7C の CC 列＝b18 と整合）。
- **CC getter（vt+0x330）直呼び**: StartCombat / Yield / ForceFlee / ForceTakeCover の 4 件（b25 の CC getter 経路と一致）。
- **メーター（slot+0x250/+0x254）**: 直書は **SetCrimeGold / ModCrimeGold / PayFineThief**、読は GetCrimeGold・CanPayCrimeGold 等（共有 evaluator 0x4F5230 経由 relay）。**payload 実体 = form+0x44 の ExtraData type 0x3D float**（`0x4269E0` 書 / `0x41FC90` 読、いずれも `push 0x3d; call 0x41E210` で型固定）→ **b27 V-27e「+0x250/254 の用途名」は crime gold 系メーターで確定寄り**。ただし IsCellOwner/IsHorseStolen/IsSneaking/IsRunning も共有 0x4F8B90 経由で type0x3D 読手に到達し、**同型 extra の多用途性は V-158c として残す**。
- **event/extra 系**: emit(0x4FBF90) へは **AddScriptPackage / RemoveScriptPackage が直、AddItem・EquipItem・GoToJail 等が relay2**。extra0x12(ExtraScript) 書手へ relay2 = 10 opcode、extra0x13(ExtraAction) 直 = OnActivate/Activate/Lock/SetDoorDefaultOpen。
- **Disable/Enable**: Enable のみが 保留集合 erase・bit11 set・flush 適用（0x4E4690）へ relay2 到達（v139 の機構と一致）。bit14 へは **AddItem** のみ strong（v139 §6-3(b) の経路と一致）。

---

## §4 訂正・GT 反映候補（親セッションへ）

| 対象 | 本調査の内容 |
|---|---|
| v138 §4 起票 V-138a | 「クラス・vtable 同定」は **非仮想 POD ラッパ（vtable 不持）** と裁定へ。"[this+0x1C] が formID か baseForm か" は **両方でない（パック serial）** へ確定。残は所有者 `[owner+8]` の正体のみ（V-158a） |
| v138 §4 起票 V-138c | **type0x13 = ExtraAction（RTTI）**、4bit 意味、set thunk 7 caller 区分、Activate 側の set 経路まで確定 |
| v138 §4 起票 V-138d | **type0x12 = ExtraScript（RTTI）**、0x428BB8 = `ExtraDataList::CopyListForReference`、0x4FBDC0 = event list 再生成工場で確定 |
| v138 §4 起票 V-138e | 16bit serial カウンタ（初値 0x0A）＋メモ化キャッシュで確定。**`[0xB3B7D0]`（別変数）と混同しないこと** |
| v139 §3 V143-a | 配列は save 除外ではなく**登録 master 配列**、62 読手の直接 save 該当 0 件、0x58=TESObjectREFR で縮小 |
| v139 §3 V143-d | World::Update の実体（尾分岐→参照 vcall 遷移）、PerFrameUpdate 順、描画 bit11 = 2 関数3サイトで縮小 |
| b24 §8 残点1 | **書手不在＝死コード**で閉（クラス特定は不能） |
| GT/`func_list_new` 系 | **`func_range` off-by-one と台帳結合（1 エントリに複数実体関数）**は分析ハーネス側の罠 → `SUBAGENT_CONTRACT`/GT へ追記候補 |
| capstone 罠（再掲） | vcall は `mov reg,[vtable+slot]; call reg` 形（`call [mem]` はほぼ不在）、op_str は空白・末尾オペランド付き → operand ベース解析が必須（b27 §9.4 の skipdata 罠に並ぶ第2の罠） |

---

## §5 新規残点（V158-… 起票粒度）

| ID | 内容 | 優先度 |
|---|---|---|
| **V-158a** | emit wrapper（param_2）の**所有者 `[owner+8]` の正体**と「param_2 = owner+0x14 の入れ子オブジェクト」の実証。array 詰め側（`0x60C81C mov eax,[esi+eax+0x20]`）の `esi`（= `[player+0x40]` = 親セル系）とレコード生成元の特定 | 中 |
| **V-158b** | `[0xB3B7D4]` メモ化が**本当に無クリアか**（`[0xB3B7D0+4]` 等の間接 base+off 書手の再走査）＋「serial が1回固定」なら 0x60d950 の dedup が実際に機能するのか（非メモ化系 serial との関係） | 中 |
| **V-158c** | type0x3D float extra の多用途性: IsCellOwner/IsHorseStolen/**IsSneaking/IsRunning** が共有 `0x4F8B90` → `0x41FC90` で読む引数文脈（crime gold と同一 extra を使うのか、別物か） | 中 |
| **V-158d** | 参照 Update 実装 `0x4E3F30`（Actor/REFR）と `0x65FF40`（Player、台帳は `FUN_0065FDA0` と結合）の**内部順序＝スクリプト実行 vs AI** の確定 | 中 |
| **V-158e** | expression getter 帯（`0x4F5730` 以降）と evaluator `0x4F5230` の**台帳結合粒度**（opcode impl から見た relay2 の帰属精度） | 低-中 |
| **V-158f** | 直接ストア行 `+0x58 (class unverified)` 5 件（SetPos/SayTo/PositionCell/TestSeenData 等）のベースレジスタ由来＝クラス帰属 | 低 |
| **V-158g** | `0x977C50`（Actor 家族 slot+0x190 実装 = `mov al,1; ret`）と 7 個の no-op opcode impl の共有が意図的スタブか、それとも slot+0x190 の真意（Update ゲート）の再確認 | 低 |
| （転記のみ） | §4 の GT 反映候補は親セッション判断（本タスクは編集禁止） | 高 |

---

## §6 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時、同一 4 セル）。
- 内部整合（本タスクの実測）:
  1. **direct_call 検証 34/34 clean**（call site が impl 本体内・int3 境界跨ぎ 0 件、`v158_19.txt`）。
  2. CSV 行数 **690** = evidence 内訳 36+50+99+116+384+5 = **690** ✓。strong 175 ペア ⊂ 全 555 ペア ✓。
  3. field←opcode の strong 合計 = opcode→field strong 合計 = **175**（双方向同一集合）✓。
  4. 件数の cache/GT 突合: `call 0x41F830`=17 / `0x423DF0`=6 / `0x423E50`=4 / `0x4D8260`=9 / `0x4D8270`=7 / `0x4D82E0`=9 / `0x41E920`=25 / `0x41F130`=20 / `0x41F670`=4 / `0x41F0B0`=0 / `0x4FBDC0`=23 / `0x4FBE00`=20 / `call 0x60D950`=2 / `call 0x607B60`=2 / `shr 0xe` 読手=62 / `call 0x46B590`=19（v139 既存）— **全一致**。
  5. RTTI 直読: ExtraAction `0xA357F4/.?AVExtraAction@@/td 0xB03E58`、ExtraScript `0xA35884/.?AVExtraScript@@/td 0xB03BB8`（vtable_index #650/#711 と 2/2 一致）。
  6. raw 値: `[0xB2EB3C]=0x0000000A`（.data 初値）、`0xB34D8C` abs32=1 件、`0xB06150`=6/`0xB06158`=10/`0xB0614C`=2/`0xB0613C`=96 — 3 系統一致。
  7. vtable slot 実装: `+0x190`→Actor 家族 `0x977C50`、`+0x160`→Actor/REFR `0x4E3F30`・Player `0x65FF40`（vtable_index 1,715 本 実読み）。
  8. PerFrameUpdate 順序は生バイト window 逆アセンブルで addresses を直接採寸（0x40DBA5/0x40DBB4/0x40DBD1/0x40DBDC/0x40DBEC）。
- 禁止事項: docs / GT / TODO / 台帳 / session-memory / plan / func_list_new / cache / git の**編集 0 件**、ESM 未アクセス、既存 `tmp/re/*.md` は読取のみ。

---

## §7 証跡一覧（すべて `%TEMP%\opencode\`）

| ファイル | 役割 | B |
|---|---|---:|
| `v158_common.py` | 共通基盤（台帳・関数範囲・call target 起点逆アセンブル・vcall/ストア解析） | 9,495 |
| `v158_1.py/.txt/.json` | opcode 前方 BFS（fan-in ゲート）＋ writer/slot カタログ集計 | 9,976 / 5,632 / 227,039 |
| `v158_2.py/.txt` | vtable 実読みによる vcall 解決・**双方向マップ CSV 生成** | 6,881 / 19,512 |
| `v158_map.csv` | **成果物表（690 行 + header, 11 列）** | **178,270** |
| `v158_3..v158_10.py/.txt` | emit this / serial / extra ctor RTTI / caller 区分 / 0x4FBDC0 / [0xB34D8C] 初動 | 30,687 / 150,703 |
| `v158_11..v158_18.py/.txt` | OnActivate 全文・bit14 配列・World::Update・描画 bit11・PerFrameUpdate 順・参照 Update | 38,810 / 98,640 |
| `v158_19.py/.txt` | **direct_call 境界検証（34/34 clean）** | 2,670 / 105 |
| `v158_21.py/.txt` | CSV 最終統計・field←opcode 一覧 | 2,887 / 11,617 |
| `v158_22.py/.txt` | extra ヘルパ本体・関数範囲整合 | 1,492 / 4,023 |
| （読取のみ） | `v138_b16_alarm_residuals.md` / `v139_b17_table2_residuals.md` / `b24_misc_residuals.md` / `b18/b25/b27` / `v_data_relmap.md` / `cache/*` / `docs/Oblivion_decompiled.c` | 無変更 |

---

## §8 Android 移植への示唆（実装しない）

1. **ExtraAction（type0x13）は 1 byte の 4bit フィールド**として実装し、`Activate`＝set bit0/bit1 → activate 実行 → clear、`OnActivate`＝test bit1 → 成立時 1.0 → clear bit0/bit1 の**冪等 one-shot** を再現する。door 系（bit2/bit3）は SetDoorDefaultOpen/Lock/OpenClose と分離した4ビットとして持つ。
2. **ExtraScript（type0x12）は `+0xC`=script owner / `+0x10`=event list の 2 ポインタ**。フォーム複製時は **event list を複製せず再生成**（`CopyListForReference` 相当）する。
3. **slot+0x250/+0x254 は crime gold 系メーター = ExtraData type0x3D の float**（Character/Player のみ有効、HighProcess は byte、他は定数）→ 系統別実装（b27 §8 と同一方針）。
4. **CC は 4 opcode（StartCombat/Yield/ForceFlee/ForceTakeCover）からしか状態へ到達しない**構造を保ち、SetState 経由（+0x6C）/intent（+0x70）/target（+0x40）を経路ごとに分ける。
5. **Disable は「登録 master 配列＋dedup 保留集合＋bit11」の三層**（配列 = 管理下フォーム、保留集合 = 最長1フレーム遅延、bit11 = 即時フラグ）で、bit14 は**配列に登録されない一時/管理外オブジェクト**として再現する。

---

## §9 追記（V148 サブエージェント・2026-10-27 継続セッション）

> 本ファイルは既に 411 行で §0〜§8 完結済み（他セッション作成）。契約どおり**上書きせず追記**。本セッションは独立再検証と裏取りのみを実施し、§0〜§8 の結論に**訂正 0 件**。

### §9.1 独立再検証（`%TEMP%\opencode\v148_verify.py` / `v148_verify.txt`）

§6 の内部整合を**別スクリプト・別ロジック**で再計測 → **56/56 全一致（FAIL 0）**。

| 検査群 | 件数 | 結果 |
|---|---:|---|
| cache 基本（opcode 533 行・opid 重複 0・両フェンス・vtable_index 1,715 行） | 6 | 全一致 |
| call 件数 GT 突合（0x446CB0=347 / 0x4468B0=0 / 0x4FBF90=77 / 0x4FB5F0=22 / 0x46B590=19 / extra ヘルパ 0x41F830=17・0x4D8270=7・0x4D82E0=9 / 0x60D950=2・0x607B60=2 / 0x4FBDC0=23 ほか） | 22 | 全一致 |
| CSV 双方向（690 行 = evidence 内訳 36+50+99+116+384+5、strong 175 ペア = 126 opcode / 29 field、全 555 ペア = 269 / 35、strong ⊂ 全、field←opcode 再集計 = 同一集合、field ≥1 opcode = 29） | 13 | 全一致 |
| raw 値（`[0xB2EB3C]`=0x0A、ExtraAction/ExtraScript RTTI 名、vtable_index 突合） | 7 | 全一致 |
| direct_call 境界（distinct (impl,writer) 34 ペア、E8/E9 サイト再探索 → impl 本体内・int3 跨ぎ 0） | 4 | 34/34 clean |
| 0x607B60 命令文言（0xB3B7D4 / 0xB2EB3C / cmp 0xA を capstone 逆読） | 3 | 全一致 |

### §9.2 RTTI 参照規約（本再検証で確定した読み方・GT 追記候補）

- **vtable の RTTI ヘッダは `vtable-8`/`vtable-4` に在る**（`vtable+0` は slot0 impl 本体）:
  `col = dword[vt-4]`（= `vtable_index.tsv` の col 列）、`td = dword[col+12]`、**name = `td+8`**（MSVC TypeDescriptor: +0 pVFTable / +4 spare / +8 name）。
- 実測: ExtraAction `vt 0xA357F4` → `col 0xAB4FE8` → `td 0xB03E58` → `.?AVExtraAction@@`、ExtraScript `vt 0xA35884` → `col 0xAB5378` → `td 0xB03BB8` → `.?AVExtraScript@@`（**vtable_index #650/#711 の col/td 列と 2/2 一致**）。
- 罠: `dword[vt+4]` は **COL ではなく slot1 impl**（0x428F60 / 0x41DF70）→ `vt+4` を COL とみなす読み方は**誤り**（本セッションで一度踏んだ）。

### §9.3 裏取り（契約 §4）

- §0 の判定・§2 の件数・§3 の addresses は**すべて v148_verify の 56 項で再測定済み**（cache/GT の転記ではない独立計測）。
- 本追記で新規に引用した地址: `vt-4`/`col+12`/`td+8` の3式と `0x428F60`/`0x41DF70`（slot1 impl）、いずれも生バイト逆読で確認。
- selftest: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（継続セッション着手時・完了時の2回）。

### §9.4 追記時点の残点状態

§5 の V-158a〜g は**変更なし**（本セッションで解消・増設なし）。TODO serial 148 の出典5系統（F4 後半 / V-138a・c・d・e / V143-a・d / b24 残点1）は §0 の 8 項目で**全数カバー済み**を確認。
