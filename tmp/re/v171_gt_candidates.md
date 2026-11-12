# v171_gt_candidates.md — GT 裁定待ち A–D 4 件の vtable 実測裁定（TODO serial 158 / V158・中）

- 作成: 2026-10-28（opencode サブエージェント / **分析専任**。`docs/`・`GT_CHEATSHEET.md` は**バイト 1 つも変更していない**）
- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B / ImageBase 0x400000 / VA=0x400C00+off(.text), 0x400E00(.rdata), 0x401400(.data)）
- 手法: capstone **5.0.7**（`skipdata=True`, `md.detail`）、`cache/vtable_index.tsv`（1,715 行）× 全 slot 実読み → impl 本文 → 呼出し元文脈 → decompiled 部分検索（`enumerate(f,1)`）→ 文字列/定数実測
- 着手時・完了時 selftest: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS ×2 回**
- 禁止遵守: git / docs / GT_CHEATSHEET / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` **編集 0 件**（docs 実測 1,038,004 B・GT 19,734 B で事前値と一致）。新規は本 1 ファイル＋`%TEMP%\opencode\v171_*.py/.txt` のみ。
- 入力（読取のみ）: `v168_v143_apply_log.md` / `v121_gt_corrections.md` / `v158_slot_opcode_map.md` / `b14_slot_method_map.md` / `b27_slot_residuals.md` / `GT_CHEATSHEET.md` / `docs/RE_EXE_REPORT.md`（§17.2・§20.2・§41.2・§41.3・L9674・L1568-1583）

---

## §0 裁定サマリ

| # | 対象 slot | 結論 | 要点 |
|---|---|---|---|
| **A** | `+0x170` | **確定 = GetBaseForm**（report §41.3「種別」は誤り） | `0x4D9B40` = `mov eax,[ecx+0x1C]; ret`（REFR 系 7 クラス）。結果の `[+4]`（formType）を読むのが usage（＝「種別」の正体）。§20.2 L1669・§17.2 L1071 と整合 |
| **B** | `+0x164` | **確定 = GetParentCell**（report「戦闘状態」は誤り） | `0x4D8370`（REFR 系 11 クラス）: process High/MidHigh なら `[process+0x17C]`、他は EDL extra type 0x15 の payload `[node+0xC]`。Player `0x65D720` は委譲。process 系は `[process+0x28]` setter で別物 |
| **C** | `+0xEC` | **確定 = GetScale（系統別。report「レベル／HP 係数」は誤り）** | Actor/REFR 系 `0x4D7260`（`[this+0x38]` × base formType==0x24 の `[base+0x114]`）。§41.3 L7469 の難易度倍率式は decompiled で **`× vt+0xEC() = × GetScale`** と実証。TESForm 系 `0x4B3750` は「GetScale を呼ぶ側」（b27 §1 再確認）、process 系は別物（`0x64B240` 新規同定） |
| **D** | `+0x2C0` | **系統別に 3 語義で確定**（1 名称では書けない） | process 系 = **GetFlags(u16)**（handover 記載は process 系として**正しい**）/ Actor・Creature = **空 `ret`（no-op）** / Character・Player = **`[this+0x108] := -1.0f`**（GT 候補の「0.0f」は**転記誤り**） |

---

## §1 裁定詳細

### A. slot `+0x170` — **GetBaseForm に確定**（「種別」は誤り）

**(1) vtable 全数実測**（一次 1,481 本中、slot 保持 21 本 / distinct impl 8）

| impl | n | クラス | 扱い |
|---|---:|---|---|
| **`0x4D9B40`** | **7** | Actor, ArrowProjectile, Character, Creature, MobileObject, PlayerCharacter, TESObjectREFR | **本裁定の対象（GetBaseForm）** |
| `0x69F100` | 5 | MagicBall/Bolt/Fog/Spray/MagicProjectile | **別物**: this 不使用のグローバル `0xB3C0CC` get-or-create シングルトン（`malloc(0x84)` → ctor `0x4A9430` → `0x44D950` 登録 → 返却。SEH prolog 実エントリを実測） |
| `0x6481B0` | 4 | High/Low/MidLow/MidHighProcess | **別物**: arg1(Actor) の `vt+0x284`(AV **0x41=武器種別**) / `vt+0x348` / AV2 / AV0x40 を用いた float 計算 + `[actor.process +0xEC](1)` 参照 → `0x5F4880` → AV0/7/0x11 → `0x547280`（「種別」語彙の近傍＝process 系の計算スロット） |
| `0x60D040` | 1 | BaseProcess | `fldz; ret 8`（0 返し） |
| `0x4AF190` / `0x4B9F80` / `0x765990` / `0x982D85` | 各1 | TESGrass / TESObjectTREE / NiDX9Renderer / NiRenderer | 無関係ノイズ |

**(2) impl 本文（本体）**
```
0x4D9B40: mov eax, dword ptr [ecx + 0x1c]
0x4D9B43: ret
```
- `[TESObjectREFR+0x1C]` は ctor `0x4D9A70` で **0 初期化**（`mov [esi+0x1c],eax`（eax=0）、+0x20..+0x34 / +0x40 も同時ゼロ。formType `[esi+4]=0x31`、vtable `0xA46C44` も同 ctor で実測）＝ baseForm フィールド。

**(3) 呼出し元文脈（「種別」の正体）**
- 直接 `call 0x4D9B40` = **2**、厳格ディスパッチ（v153 同一ロジック）= **871 箇所**（うち `PROC[+0x58]` 由来は 17 のみ → 主用途は REFR 系オブジェクト）。
- 結果の **`[+4]`（formType）読取**が多用＝report §41.3「種別」の出所:
  - decompiled L366948-51（FUN_005febf0 内）: `iVar5 = (*param_1 + 0x170)(); if ((*(char*)(iVar5+4) != '#') && (... == '$'))`（`'#'=0x23` / `'$'=0x24`）
  - L365614-786（同関数）: `iVar7 = (*piVar26 + 0x170)()` 従 0x24 判定、L42487 `== '\x1a'`
  - `0x4D7260`（GetScale）内部: `mov edx,[eax+0x170]; call; cmp byte [eax+4],0x24` — **結果はフォームポインタ**として扱われる
- IsDead `0x4D7DD0` は結果を `__RTDynamicCast(TESForm→TESHealthForm)` → 現在体力（b14 §1.1）。emit key 経由 script opcode 20 件（v158 §2.2: GetSelf/IsOwner/SetOwnership…）。

**(4) report 内部矛盾の裁定**
| 場所 | 記述 | 整合 |
|---|---|---|
| §41.3 L7505 | `+0x170 \| 種別` | **不成立**（slot は form ポインタを返す。種別は「結果の [+4] を読む」usage） |
| §20.2 L1669 | `0x4d9b40 = return *(param_1+0x1c)`、67 vtable 共有 | ✓ 本文と一致 |
| §17.2 L1071 | `+0x170 基本NPC取得` | ✓（base form = 基本 NPC/オブジェクト） |
| b14 §1.1 | GetBaseForm [確定] | ✓ |

→ **A = 確定: GetBaseForm**。§41.3「種別」は呼出し元の formType 読取から生じた誤認。
（付随: report L1568-1583 の「`+0x170` = 装備魔法ハンドル」も同一 `0x4d9b40` への別解釈で、ctor 実測（+0x1C=baseForm）と語義矛盾 → **訂正候補（本タスク範囲外・残点 5）**）

---

### B. slot `+0x164` — **GetParentCell に確定**（「戦闘状態」は誤り）

**(1) vtable 全数実測**（slot 保持 23 本 / distinct impl 10）

| impl | n | クラス | 扱い |
|---|---:|---|---|
| **`0x4D8370`** | **11** | Actor, ArrowProjectile, Character, Creature, Magic×5, MobileObject, TESObjectREFR | **本裁定の対象（GetParentCell）** |
| `0x65D720` | 1 | PlayerCharacter | オーバーライド（下記 (2)） |
| `0x6292B0` | 4 | High/Low/MidLow/MidHighProcess | **別物**: `fld [esp+4]; fstp [ecx+0x28]; ret 4`（`[process+0x28]` へ 32bit 値格納 setter） |
| `0x68F970` | 1 | BaseProcess | `ret 4` 空スタブ |
| `0x4A9890` | 1 | TESCombatStyle | **別物**: `fld [ecx+0x8C]; ret`（float getter） |
| `0x625BB0` | 1 | DefaultCombatStyle | **別物**: `fld [0xB35780]; ret`（グローバル float getter） |
| `0x4AED60` / `0x4B9F60` / Ni 系 2 | 各1 | TESGrass / TESObjectTREE / NiDX9 / NiRenderer | 無関係 |

**(2) impl 本文**
```
0x4D8370: mov esi,ecx
          movzx eax, byte [esi+4]      ; formType
          add eax,-0x32 / cmp eax,1 / ja 0x4d839d     ; formType ∈ {0x32,0x33}（Actor 系）のみ process 経路
          mov ecx,[esi+0x58] / test / je 0x4d839d     ; this->process
          mov eax,[ecx] / mov edx,[eax+8] / call edx   ; process[vt+8] = レベル(0..3)
          cmp eax,1 / ja 0x4d839d                      ; High(0)/MidHigh(1) のみ
          mov eax,[esi+0x58] / mov eax,[eax+0x17c] / pop esi / ret   ; → [process+0x17C] を返す
0x4d839d: lea ecx,[esi+0x44] / pop esi / jmp 0x41e630  ; EDL 経路
```
- **EDL 経路の戻り値を実測**: `0x41E630` = `push 0x15; call 0x41E210; test eax; je →0; mov eax,[eax+0xC]; ret` ＝ **extra type 0x15 の payload（`[node+0xC]`）を返す**（`0x41E210` = presence bitmap `this+8..` ゲートの node アクセサ、`ret 4`）。→ 両分支とも**親セル相当のポインタを返す getter**。
- **Player `0x65D720`**: `[+0x5DC]`≠0 → それを返す / byte `[+0x588]`≠0 → `jmp 0x4D8370`（委譲）/ `[+0x5CC]`≠0 → それを返す / 他 → `jmp 0x4D8370`。**固有フィールドのオーバーライド＋基本版委譲で語義は GetParentCell 系**（フィールド正体は[要深掘]）。末尾の兄弟関数 `0x65D760` も `call 0x4d8370`。
- process setter の呼出し実例: `0x5EBA39`（FUN_005eb520 内、フロア除算 float を push → `[process+0x28]` へ格納、`PROC[+0x58]` 由来）。

**(3) 呼出し元・命名手掛かり**
- 直接 `call 0x4D8370` = **32**、厳格ディスパッチ = **214 箇所**（`PROC[+0x58]` 由来は 7 のみ）。
- 結果の使われ方: `push 3/0x1b/ebx → call vt+0x164 → mov ecx,eax → call 0x470720/0x470750`（cell query helper、direct 54/22 件、戻り値は `cmp eax,2` 等の int）→ **結果が「セル」ポインタ**として後続ヘルパの this になる（push 値は stack scheduling で **後続ヘルパの引数**であり +0x164 本体 `0x4D8370` は 0 引数＝v153 の narg=1 は先行 push の読み違い注意）。
- script opcode（v158 §2.2）: **strong 9 / all 16 = ShowAnim / SetAngle / SetAtStart / PickIdle …** ＝ GetParentCell 利用として整合。
- report 内 `+0x164`=「戦闘状態」の言及は **§41.3 L7504 の 1 行のみ**（孤発記載・内部矛盾なし、impl と不成立）。

→ **B = 確定: GetParentCell**（process 系・TESCombatStyle 系は別物として系統別に併記）。

---

### C. slot `+0xEC` — **GetScale に確定（系統別）**。b27 §①（0x4B3750）との関係を実測で確認

**(1) vtable 全数実測**（slot 保持 57 本 / distinct impl 15）

| impl | n | クラス | 語義 |
|---|---:|---|---|
| **`0x4D7260`** | **10** | Actor, ArrowProjectile, Creature, Magic×5, MobileObject, TESObjectREFR | **GetScale（本裁定の中心）** |
| **`0x611D00`** | 2 | Character, PlayerCharacter | **GetScale + race 系補正**（下記 (2)） |
| **`0x4B3750`** | 29 (+1 二次: TESFlora) | TESForm 系（AlchemyItem…TESSubSpace） | **scale 検証＋一時 3D ノード生成（GetScale を呼ぶ側・別物）** |
| `0x4943A0` | 3 | Base/Low/MidLowProcess | `xor eax,eax; ret 4`（0 返し） |
| **`0x64B240`** | 2 | **HighProcess, MiddleHighProcess** | **新規同定: `[process+0xE4]` 連結リスト先頭 getter（下記 (4)）** |
| `0x4A9720` | 1 | TESCombatStyle | `fld [ecx+0x20]; ret`（float getter・別物） |
| `0x4A9560` | 1 | TESAmmo | `call 0x4B3750` ラッパ（SEH prolog 実測） |
| `0x4B2980` | 2 | TESBoundTreeObject, TESObjectTREE | 別物 |
| `0x47CFC0` | 1 | TESObject | `xor eax,eax; ret 8`（0） |
| `0x6245B0` / `0x6259D0` / Ni 系 3 | 各1 | CombatController / DefaultCombatStyle / Ni 系 | 無関係・別物 |

**(2) GetScale 本体**
```
0x4D7260: fld [esi+0x38]                      ; this->scale（REFR+0x38 = scale f32）
          this->vt+0x170() → base             ; GetBaseForm
          if base && base->formType(+4)==0x24 → scale *= [base+0x114]
          return scale
0x611D00: s = call 0x4D7260
          base = this->vt+0x170(); if base && [base+0xE8]≠0:
             idx = 0x519D20(base)（≤1 のときのみ乗算、他は 0）
             s *= [base+0xE8] 内 ObjArr(+0x60+idx*4)
          return s                             ; Character/Player 版（b14 の race 乗算記載と一致）
```

**(3) `0x4B3750`（TESForm 系）との関係【突合課題に回答】** — b27 §1 を独立再確認
- `0x4B3829: mov ecx,esi / 0x4B382B: call 0x4D7260` → 結果を `fucomp` で base ノード `[edi+0x60]` と比較（**GetScale を直接呼んで scale 検証**）。
- 警告文字列 `0xA43BE0` = `"%s has a scale of 0.  Change it in the editor."`（`0x4B3956` 呼出し）。
- `0x4B3ABC: fstp [ebp+0x60]` に `|scale|` 替入、key は `sprintf("%s%i", path, (int)(ref->vt+0xEC()×K))`（b27 と同一）。
- `0x4B3B51: mov eax,[ebx] / mov edx,[eax+0xec] / push ecx / push esi / mov ecx,ebx / call edx` ＝ **`this->vt+0xEC(ref,flag)` 自己呼び出し**（30 クラスでは即 `0x4B3750` 自身＝終端は既存残点 V-27a 継続）。
- ⇒ **`+0xEC` の TESForm 系 impl は「スケール検証・ノード生成のため GetScale を消費する側」であり、GetScale そのものではない**（b14 §1.2 の指摘を維持）。30 エントリすべて `+0xEC` にのみ登録（`+0x154` ではない）ことも b27 §1.3 と再一致。

**(4) process 系 `0x64B240`（新規）**
```
0x64B240: cmp byte [esp+4],0 / je → return [this+0xE4]            ; flag==0: 生ポインタ
          ecx=[this+0xE4] / test / je → 0
          call 0x484E80([this+0xE4], 0) → 真なら [this+0xE4] / 伪なら 0
0x484E80: リスト走査（[head] → node: [node]=value, [node+4]=next）で 0x41DF10(value, arg)
0x41DF10: arg==0 → test byte [value+0xB],0x18（flags bit27|bit28）/ arg≠0 → ([value+0xB]>>4)&1（bit28）
```
- ⇒ process 系は「**フォーム連結リスト先頭の取得（flags ガード付き）**」で **スケールとは完全に別物**。
- FUN_005febf0 での使用（decompiled L365567-71）: `[actor.process.vt+0xEC](1)` → 結果 `[+8]` → そこから `[+0x90]` byte（＝report L9674「武器種別 byte」）/ `[+0x98]` float（難易度ヘルパ `FUN_00547540` の入力）へ到達 → **report の「武器 REF 経路」と整合**（フィールド正体は[要深掘]＝残点 2）。

**(5) 呼出し元・§41.3 L7469 の裁定【決定的】**
- 直接 `call 0x4D7260` = **7**、`call 0x4B3750` = **3**（b27 §9.3 と一致）。厳格ディスパッチ **201 箇所**（`PROC[+0x58]`+1 引数 = **94**（process 版主導）/ 0 引数（GetScale 系）= 54。top: FUN_0063d0c0=19, FUN_005fcab0=11, **FUN_005febf0=11**, FUN_00690af0=10）。
- **report §41.3 L7469** `uStack_1a8 = (vtable+0x26c)() * (vtable+0xec)()` の実体（decompiled L365573-82、FUN_005febf0 冒頭）:
  ```
  if (iVar7 == 0) fVar20 = param_1->vt+0x26c();                 // 第1因子（fallback）
  else            fVar20 = FUN_00547540(*(float*)(iVar7+0x98));  // iVar7 = [[process.vt+0xEC](1)+8]（武器系）
  uStack_1a8 = (float)fVar20;
  fVar20 = param_1->vt+0xec();                                   // ★ Actor 版 = GetScale (0x4D7260)
  uStack_1a8 *= (float)fVar20;                                   // → 難易度倍率 = 係数 × スケール
  ```
  → 乗じられている `vt+0xEC()` は **GetScale（スケール倍率）**。同一パターンは FUN_00620290（`0x6202F8` +0x26c → `0x620308` +0xEC → `0x620312 fmul`）にも存在。
- ⇒ report「レベル／HP 係数」は **slot の語義として不成立**（「難易度倍率式の被乗数の 1 つがスケール」という usage 事実としてのみ残る。第1因子 `+0x26C`＝`0x5E40A0` = `FUN_00547540([0xB36E90])`（Creature 版 `0x625220` は base 統計補正つき）の名称確定は範囲外＝残点 7）。

→ **C = 確定: GetScale（系統別内訳つき。TESForm 系・process 系は別物）**。

---

### D. slot `+0x2C0` — **系統別 3 語義で確定**（1 名称では書けない）

**(1) vtable 全数実測**（slot 保持 = 9 クラス。GT §4「206 スロット超は 9 クラス」と一致）

| 系統 | impl | 本文 | 語義（確定） |
|---|---|---|---|
| Base / Low / MidLow / **MidHigh** | `0x60CF50` | `xor ax,ax; ret` | **u16 恒 0 スタブ**（GetFlags 系の既定） |
| **High** | `0x6285A0` | `mov ax,[ecx+0x1FC]; ret` | **`[process+0x1FC]` u16 flags 取得 = GetFlags** |
| **Actor / Creature** | `0x60D0A0` | `ret` | **空 no-op**（§41.2 の呼出しは実行されない） |
| **Character / PlayerCharacter** | `0x60E260` | `fld [0xA30634]; fstp [ecx+0x108]; ret` | **`[this+0x108] := -1.0f`**（定数 `0xA30634` は生バイト `000080bf` = **f32 -1.0**。GT 候補 D の「0.0f」は**転記誤り**） |

> ⚠️ b14 §1.1 の「High/**MidHigh** → `0x6285A0`」表記は実測と不一致（MidHigh → `0x60CF50`）。b14 は編集禁止 → 訂正候補（残点 6）。

**(2) handover「GetFlags()→word (bits 0x100/0x200)」の裁定 — process 系として正しい**
- `0x5F10E0`（GetPos override）: `mov ecx,[esi+0x58] → vt+0x2C0 call → test ax,0x800 → jne`（走行中フラグ、b14 §1.1 と一致）。
- FUN_00407ae0（debug 状態ビルダ。decompiled L6189-99）: `uVar9 = process[vt+0x2C0](); if (uVar9 & 0x100) → 文字列 "Walk "（0xA307F0）追加 / if (uVar9 & 0x200) → 追加`、L6365-75 は騎乗馬 process でも同テスト。→ **bit0x100 = Walk 系 / bit0x200 = Move 系 / bit0x800 = 走行**（厳密語義は[要深掘]）。
- 厳格ディスパッチ **90 箇所**（`PROC[+0x58]` 由来 61 / field-load 1 のみ）→ **process 系利用が支配的**。

**(3) report §41.2 の呼出し（`0x5F39C7`）の実測**
- FUN_005f3870 = **`+0x2C4` の Actor/Character/Creature 実装本体**（slot `+0x2C4` 実測: Actor/Character/Creature → `0x5F3870` / Player → `0x65FF10` / High → `0x631B90` / Base 系 → `0x60CF60`）＝ **HP 変動（ダメージ適用）関数**。report の「+0x2c4 = HP 変動」は妥当。
- 文脈: `mov esi,ecx`（this）→ ダメージ計算 → トレース文字列 **`"%.20s's %s takes %.2f points of damage (%.2f/%.2f)!"`（0xA6E76C）** 出力（`0x579B60`）→ イベント emit → `test ebp,ebp / je skip` → **`mov edx,[esi]; mov eax,[edx+0x2c0]; mov ecx,esi; call eax`**（ebp = arg1 の formType 0x14 フォーム）。
- ⇒ **呼び出し元の this は Actor 系オブジェクト**。よって:
  - Actor/Creature 受けなら **no-op**（「HP 更新通知」は**発火しない**）
  - Character/Player 受けなら **`[this+0x108] = -1.0f`**（下記 (4)）

**(4) `[Character+0x108]` の実態（GT 候補の「0.0f setter」訂正）**
- Character ctor `FUN_0060E4F0`（vtable `0xA6FC9C`・formType `0x32`・二次 vtable 群をセットして Character と確定）が **`fld [0xa30634]; fstp [esi+0x108]` = -1.0f で初期化**。
- `FUN_0060E580`（decompiled L376307 `float10 __fastcall FUN_0060e580(int param_1)`）: `if ([param_1+0x108] < 0.0) { [+0x108] = 0; EDL extra type 0..0xd を走査（`0x486790` → `__RTDynamicCast(0xB03248→0xB03778)` → `FUN_004B4C70` 判別で 2 バケットに `FUN_00488CB0` の float を加算）} [+0x108] = 合計`（asm `0x60E705: fstp [ebp+0x108]`）。
- ⇒ `[Character+0x108]` = **HP/ダメージ関連の float 計算キャッシュ（-1.0 = dirty センチネル / 0 以上 = キャッシュ値）**。`0x60E260`（+0x2C0）は「HP 変動の通知後にキャッシュを dirty 化する」＝ report「HP 更新通知」とは**部分的に**対応するが、**Actor/Creature は空・process 系は GetFlags で 1 名称には書けない**。[正面の語義＝要深掘]（残点 1）。

→ **D = 確定（系統別）**: `+0x2C0` = **process: GetFlags(u16) / Actor・Creature: no-op / Character・Player: `[+0x108]=-1.0f` リセット**、`+0x2C4` = HP 変動（ダメージ適用 `0x5F3870`）。GT 候補の「0.0f」は **-1.0f** へ訂正。

---

## §2 GT §4.1.1 確定反映ドラフト（候補行 → 確定行・**反映は承認制**）

> 置換対象は `tmp/re/GT_CHEATSHEET.md` L132（見出し）＋ L136-139（A–D 行）の 5 箇所。E 行（L140）・F 行（L141）は現状維持。**LF・BOM なしを保持すること。**

### G-A. 見出し（L132）

**before**
```
#### 4.1.1 不整合候補 A–F（V121 §4 走査。E は V163 で確定、A–D は裁定待ち＝候補のまま・GT は断定しない）
```
**after**
```
#### 4.1.1 不整合候補 A–F（V121 §4 走査。E は V163 で、A–D は V171（v171_gt_candidates.md）で確定）
```

### G-B. A 行（L136）

**before**
```
| A | `+0x170` | report §41.3「種別」 | impl `0x4D9B40` = `mov eax,[ecx+0x1C]; ret`（**GetBaseForm**）。report 自身の §20.2（0x4D9B40 を 67 vtable 共有と記載）・§17.2「基本NPC取得」と内部矛盾 | **裁定待ち**（候補のまま。GT は断定しない） |
```
**after**
```
| A | `+0x170` | report §41.3「種別」 | impl `0x4D9B40` = `mov eax,[ecx+0x1C]; ret`（REFR 系 7 クラス共通。`[ref+0x1C]`=baseForm、ctor `0x4D9A70` で 0 初期化）。呼出し元は結果の `[+4]`(formType) を読む（FUN_005febf0 decompiled L366948-51 で `'#'(0x23)/$('0x24)` 判定、GetScale `0x4D7260` 内 `cmp byte[eax+4],0x24`）＝「種別」は usage 由来の誤認 | **確定（V171）: GetBaseForm**。系統別: MagicProjectile 系 5 = `0x69F100`（グローバル `0xB3C0CC` シングルトン accessor・別物）/ Process 系 = `0x6481B0`（AV0x41 武器種別ほかの float 計算系・別物）/ Base = `0x60D040`(`fldz; ret 8`)。§20.2 L1669・§17.2 L1071 と整合 |
```

### G-C. B 行（L137）

**before**
```
| B | `+0x164` | report §41.3「戦闘状態」 | Actor/Character impl `0x4D8370` = formType 0x32/0x33 → `process[vt+8]` → `*(process+0x17C)` ＝ **GetParentCell**（b14 §1.1 [確定]）。Player のみ別物 `0x65D720` | **裁定待ち** |
```
**after**
```
| B | `+0x164` | report §41.3「戦闘状態」 | `0x4D8370`（REFR 系 11 クラス）: formType ∈{0x32,0x33} かつ process が High/MidHigh（`process[vt+8] ≤1`）なら `[process+0x17C]`、それ以外は EDL extra type 0x15 の payload（`0x41E630` = `push 0x15; call 0x41E210 → [node+0xC]`）を返す＝**親セル getter**。script opcode（ShowAnim/SetAngle/SetAtStart/PickIdle、v158 strong9）が利用 | **確定（V171）: GetParentCell**。Player `0x65D720` = `[+0x5DC]`/`[+0x5CC]` 非 0 ならそれを返し `0x4D8370` へ委譲（語義は同一系）。process 系 `0x6292B0` = `[process+0x28]` 32bit setter で**別物**、Base `0x68F970` = `ret 4` 空、TESCombatStyle `0x4A9890` = `fld[+0x8C]` 別物。「戦闘状態」は不成立（report 孤発記載） |
```

### G-D. C 行（L138）

**before**
```
| C | `+0xEC` | report §41.3「レベル／HP 係数」 | Actor impl `0x4D7260` = `[this+0x38]`(scale) × base formType==0x24 判定 ＝ **GetScale** [有力]。難易度倍率（`(vt+0x26c)*(vt+0xec)`）解釈も成立し得る | **裁定待ち**（[有力]・要深掘） |
```
**after**
```
| C | `+0xEC` | report §41.3「レベル／HP 係数」 | Actor/REFR 系 10 = `0x4D7260` = `[this+0x38]`（base formType==0x24 なら `[base+0x114]` を乗算）＝**GetScale**（0 引数 float、直呼び 7）。Character/Player `0x611D00` = 0x4D7260 × race 系（`[base+0xE8]`→`[+0x60+idx*4]`）。§41.3 L7469 の難易度倍率式は decompiled L365581-82 で `param_1->vt+0xec()`（=GetScale）を乗じる実測 → 第1因子（`+0x26c` or 武器 `[+0x98]` 経由 `FUN_00547540`）× **スケール** | **確定（V171）: GetScale（系統別）**。TESForm 系 29 = `0x4B3750` は scale 検証＋一時 3D ノード生成（`0x4B382B: call 0x4D7260`、警告 `"%s has a scale of 0…"`、`0x4B3B51` で `this->vt+0xEC` 自己呼び出し＝**GetScale を呼ぶ側・別物**、b27 §1 確定を再確認）。process 系: Base/Low/MidLow `0x4943A0`=0 / **High/MidHigh `0x64B240`（新）= `[process+0xE4]` リスト先頭 getter**（flag≠0 なら `0x484E80` の flags bit27/28 検査通過分のみ。FUN_005febf0 は `[+8]`→`[+0x90]` 武器種別 byte へ到達＝別物）。TESCombatStyle `0x4A9720` = `fld[+0x20]` 別物 |
```

### G-E. D 行（L139）

**before**
```
| D | `+0x2C0` | report §41.3「HP 更新通知」＋ handover の GetFlags 記載 | 系統別 3 通り: Process `0x60CF50`（u16 返し＝GT GetFlags に整合）/ Actor・Creature `0x60D0A0`（**`ret` 空**＝report §41.2 の呼出しは no-op）/ Character・Player `0x60E260`（`[this+0x108]` へ 0.0f の setter）。同一 offset 二重意味の典型 | **裁定待ち**（系統別に書く） |
```
**after**
```
| D | `+0x2C0` | report §41.3「HP 更新通知」＋ handover の GetFlags 記載 | **系統別 3 語義で確定**: ①process 系 = **GetFlags(u16)**（High `0x6285A0` = `mov ax,[ecx+0x1FC]; ret` / Base・Low・MidLow・**MidHigh** `0x60CF50` = `xor ax,ax; ret` 恒 0。テスト実測: `test ax,0x800`（0x5F10F9, 走行）/ `&0x100`→"Walk "（FUN_00407ae0 L6189-99）/ `&0x200`）＝**handover の GetFlags→word(bits 0x100/0x200) は process 系として正しい** ②Actor・Creature `0x60D0A0` = **`ret` 空 no-op**（report §41.2 `0x5F39C7` の呼出しは実行されない。呼び出し元 FUN_005f3870 = `+0x2C4` 実装＝ダメージ適用、文字列 `"%.20s's %s takes %.2f points of damage…"`）③Character・Player `0x60E260` = **`[this+0x108] := -1.0f`**（定数 `0xA30634` 生バイト `000080bf`＝**-1.0**。旧「0.0f」は転記誤り。`+0x108` は ctor `FUN_0060E4F0` で -1.0f 初期化、`FUN_0060E580` が <0 のとき再計算して合計を格納する HP/ダメージ系 float キャッシュ＝dirty リセット） | **確定（V171・系統別に書く）**。`+0x2C4` = HP 変動（Actor/Character/Creature = `0x5F3870` ダメージ適用）で report「HP 変動」は妥当 |
```

### G-F. （任意）GT §4.1 正本表への追記行案（§6 ルール「新 slot 確定時は §4.1 へ追記」準拠・**承認制**）

> §4.1 表（L122-128）末尾へ 4 行追加（既存行と同型・「1 行 1 系統」は inline 注記で充足、+0x2C0 は systems が 3 に分かれるため 1 行に統合せず 2 行に分割案）。

```
| `+0x170` | **GetBaseForm**（`mov eax,[ecx+0x1C]; ret`。`[ref+0x1C]`=baseForm） | REFR 系 7 クラス → `0x4D9B40`。結果の `[+4]`(formType) を読むのが usage（§41.3「種別」の正体）。MagicProjectile 系 `0x69F100`（グローバルシングルトン accessor）/ process 系 `0x6481B0`（AV 計算）は**別物**（v171 §1-A） |
| `+0x164` | **GetParentCell** | REFR 系 11 → `0x4D8370`（High/MidHigh process なら `[process+0x17C]`、他は EDL extra 0x15 payload `[node+0xC]`）。Player `0x65D720` = `[+0x5DC]`/`[+0x5CC]` オーバーライド→委譲。process 系 `0x6292B0` = `[process+0x28]` setter、TESCombatStyle `0x4A9890` = `fld[+0x8C]` は**別物**（v171 §1-B） |
| `+0xEC` | **GetScale**（0 引数 float） | Actor/Creature/Mobile/REFR/Projectile 系 10 → `0x4D7260`（`[this+0x38]`、base formType 0x24 なら `[base+0x114]` 乗算）/ Character・Player `0x611D00`（×race 系）。TESForm 系 29 = `0x4B3750`（scale 検証＋一時 3D ノード生成＝**GetScale を呼ぶ側・別物**）/ process 系（High/MidHigh `0x64B240` = `[process+0xE4]` リストゲッタ）**別物**（v171 §1-C） |
| `+0x2C0` | **系統別: GetFlags(u16) / no-op / -1.0f リセット** | process 系 = GetFlags（High `0x6285A0` = `[+0x1FC]`、Base/Low/MidLow/MidHigh `0x60CF50` = 恒 0。bit0x100/0x200/0x800 テスト実測）/ Actor・Creature `0x60D0A0` = 空 no-op / Character・Player `0x60E260` = `[+0x108]:=-1.0f`。同 offset は**系統ごとに別語義**（v171 §1-D） |
| `+0x2C4` | **HP 変動（ダメージ適用）** | Actor/Character/Creature = `0x5F3870`（`"…takes %.2f points of damage…"` 出力→イベント→`vt+0x2C0` 呼出）/ Player `0x65FF10` / High `0x631B90` / Base 系 `0x60CF60`（v171 §1-D(3)） |
```

### G-G. （任意）GT §5 訂正履歴 追記 1 行案

```
| 2026-10-28 | V171: §4.1.1 候補 A–D を裁定確定 — `+0x170`=**GetBaseForm**（`0x4D9B40`、§41.3「種別」は formType 読取由来の誤認）/ `+0x164`=**GetParentCell**（`0x4D8370`、EDL extra 0x15 payload 実測。旧「戦闘状態」は誤り）/ `+0xEC`=**GetScale**（`0x4D7260`、§41.3 難易度倍率式の第2因子がスケールと decompiled 実証。旧「レベル／HP 係数」は誤り。TESForm 系 `0x4B3750`=呼ぶ側・process 系 `0x64B240`(新)=リストゲッタで別物）/ `+0x2C0`=**系統別**（process=GetFlags(u16) `0x6285A0`/`0x60CF50`・Actor/Creature=no-op `0x60D0A0`・Character/Player=`[+0x108]:=-1.0f` `0x60E260`。旧 GT「0.0f」は -1.0f の転記誤り）。出典 `tmp/re/v171_gt_candidates.md` |
```

---

## §3 docs §41.3 訂正ドラフト（**編集禁止・アンカー行の特定と before→after のみ**）

> 対象: `docs/RE_EXE_REPORT.md`（現行 1,038,004 B / 12,352 行 / **CRLF** / BOM なし / `.bak17`）。以下 4 行は**現行ファイルの実測アンカー**（行内容 1 行完全一致・V143 適用後）。置換は CRLF を LF 化しない文字単位置換のみ（`.bak18` 排番は適用時フローに従う）。

### D-A. L7501（`+0xec` 行・アンカー実測 1）

**before**
```
| +0xec | レベル／HP 係数 |
```
**after**
```
| +0xec | **スケール取得 GetScale**（Actor/Creature/Mobile/REFR/Projectile 系 `0x4D7260` = `[this+0x38]`、base formType 0x24 なら `[base+0x114]` を乗算／Character・Player `0x611D00` = race 系補正つき。TESForm 系 `0x4B3750` は scale 検証＋一時 3D ノード生成で**別物**（内部 `0x4B382B` で 0x4D7260 を呼出、警告 `"%s has a scale of 0.  Change it in the editor."`）。process 系も**別物**（High/MidHigh `0x64B240` = `[process+0xE4]` リスト先頭ゲッタ（flag 付き・flags ガード）、Base/Low/MidLow `0x4943A0` = 0 返し）。本節冒頭の難易度倍率は「×`vt+0xEC()`＝×スケール」（**訂正 2026-10-28 (V171)**。旧「レベル／HP 係数」は誤り） |
```

### D-B. L7504（`+0x164` 行・アンカー実測 1）

**before**
```
| +0x164 | 戦闘状態 |
```
**after**
```
| +0x164 | **親セル取得 GetParentCell**（REFR 系 11 クラス `0x4D8370`: Actor 系 formType 0x32/0x33 かつ process が High/MidHigh なら `[process+0x17C]`、他は EDL extra type 0x15 の payload `[node+0xC]`（`0x41E630`）。Player `0x65D720` は `[+0x5DC]`/`[+0x5CC]` オーバーライド→`0x4D8370` 委譲。process 系 `0x6292B0` = `[process+0x28]` setter、TESCombatStyle `0x4A9890` = `fld[+0x8C]` は**別物**。旧「戦闘状態」は誤り（**訂正 2026-10-28 (V171)**） |
```

### D-C. L7505（`+0x170` 行・アンカー実測 1）

**before**
```
| +0x170 | 種別 |
```
**after**
```
| +0x170 | **基本フォーム取得 GetBaseForm**（REFR 系 7 クラス `0x4D9B40` = `mov eax,[ecx+0x1C]; ret`。`[ref+0x1C]`=baseForm（ctor `0x4D9A70` で 0 初期化）。本節・§20.2「0x4d9b40 = return *(param_1+0x1c)」・§17.2「基本NPC取得」と整合。結果の `[+4]`(formType) を読む usage（`$('0x24)`/`'#'` 判定等）が旧「種別」の出所。MagicProjectile 系 `0x69F100`／process 系 `0x6481B0` は別物（**訂正 2026-10-28 (V171)**） |
```

### D-D. L7515（`+0x2c0 / +0x2c4` 行・アンカー実測 1）

**before**
```
| +0x2c0 / +0x2c4 | HP 更新通知 / HP 変動 |
```
**after**
```
| +0x2c0 / +0x2c4 | +0x2c4 = **HP 変動（ダメージ適用）** `0x5F3870`（Actor/Character/Creature。`"%.20s's %s takes %.2f points of damage (%.2f/%.2f)!"` 出力 → イベント emit → `vt+0x2C0` 呼出）。**+0x2c0 は系統別**: process 系＝**GetFlags(u16)**（High `0x6285A0` = `[process+0x1FC]`、Base/Low/MidLow/MidHigh `0x60CF50` = 恒 0。`test ax,0x800`（走行）・`&0x100`（"Walk "）・`&0x200` テスト実測）／ Actor・Creature `0x60D0A0` = `ret` **空（§41.2 の呼出しは no-op）** ／ Character・Player `0x60E260` = **`[this+0x108] := -1.0f`**（`0xA30634`＝-1.0、HP 関連 float キャッシュの dirty リセット。旧 GT「0.0f」は -1.0f の転記誤り）。旧「HP 更新通知」は Character 版にのみ部分成立（**訂正 2026-10-28 (V171)**） |
```

### D-E. （範囲外・任意候補）§41.2 L7429 / L7439 のコメント

- L7429: `005f39c7  MOV EDX,[ESI] / MOV EAX,[EDX+0x2c0] / CALL EAX   ; HP 更新通知（vtable +0x2c0）`
  - 任意 after 補足: `; +0x2c0 呼出（Actor/Creature は no-op・Character/Player は +0x108=-1.0f リセット・V171）`
- 本タスクの指示は §41.3 訂正ドラフトのため**上記は含めない**（親セッション判断の残件として §4 残点に記載）。

---

## §4 残点（新規・本タスクで解消できなかった点）

| # | 内容 | 優先度 |
|---|---|---|
| 1 | **`[Character/Player+0x108]` の正面の語義**: `FUN_0060E580` が EDL extra type 0..0xd を走査し `FUN_004B4C70` 判別で 2 バケットに `FUN_00488CB0`（float）を加算して合計を格納するが、「armor 合計 / ダメージ差分 / HUD フラッシュ値」のいずれか未確定。`FUN_005e0cd0`・`FUN_005f23b0(this,0x1b)` 後続の意味と併せ確定させる | 中 |
| 2 | **`[HighProcess+0xE4]` の正体**: `+0xE4/+0xE8/+0xEC/+0xF0` 族（`0x64B240/270/2A0/2D0`）が返す「flags bit27/28 検査（`0x41DF10`）を通すフォーム連結リスト」が装備品/アクティブエフェクト/魔法のどれか。FUN_005febf0 の `[+8]`→`[+0x90]`(byte 武器種別)/`[+0x98]`(float) 到達先の型決定を含む | 中 |
| 3 | **process `+0x164` の `[process+0x28]` 語義**（float setter。`FUN_005eb520` 系がフロア除算値を書く） | 低 |
| 4 | **MagicProjectile 系 `+0x170` = `0x69F100`**（`[0xB3C0CC]` シングルトン accessor・this 不使用）が「GetBaseForm 相当」を返さない理由と用途 | 低 |
| 5 | **report L1568-1583「`+0x170` = 装備魔法ハンドル」記述（§19 域）**: A 裁定（baseForm）と語義矛盾。`[actor+0x1C]` = base form（TESNPC 等が AV を保持する構造）への読み替え候補 → 訂正候補（**本タスクは §41.3 のみ裁定**・親セッション判断） | 中 |
| 6 | **b14 §1.1 の `0x2C0` 行の転記不一致**: 「High/**MidHigh** → `0x6285A0`」は実測と不一致（MidHigh → `0x60CF50`）。b14 は編集禁止 → 親セッションが b14 へ訂正 | 中 |
| 7 | `+0x26C`（`0x5E40A0` = `FUN_00547540([0xB36E90])`、Creature 版 `0x625220` は base 統計補正）の正式名称（report「難易度倍率」の確定）— A–D の裁定に必須でないため保留 | 低 |
| 8 | process `+0x2C0` flags の bit 単位の厳密語義（bit0x100="Walk " / bit0x200=… / bit0x800=走行 の対応表） | 低 |
| 9 | 既存 **V-27a**（`0x4B3750` の `this->vt+0xEC` 自己呼び出しの終端根拠）は継続（本タスクでは関係の再確認のみ） | （既存・高） |
| 10 | §41.2 L7429/L7439 コメントの「HP 更新通知」注記更新（範囲外・任意） | 低 |

---

## §5 自己検証・証跡

### 検証

| 項目 | 結果 |
|---|---|
| `probe_harness.py selftest`（着手時・完了時） | **4/4 PASS ×2** |
| vtable 全数実測 | 4 slot × 一次 1,481 本（+ 二次分離）。A: distinct 8 / B: 10 / C: 15（+二次1）/ D: 4。`bad=0` |
| b14/V121/b27 との一致 | `0x4D9B40`=7 クラス ✓ / `0x4D8370`=11 ✓ / `0x4D7260`=10・`0x4B3750`=30(+二次1) ✓ / `call 0x4D7260`=7・`call 0x4B3750`=3（b27 §9.3 と 2/2 一致）✓ / +0x2C0 = 9 クラス ✓ |
| 新規検出 | process `+0xEC` = **`0x64B240`**（b14 は「Process 系 → `0x4943A0`」のみ記載で High/MidHigh を欠落）/ `+0x2C4` 系統別 impl / `0xA30634` = **-1.0f**（GT 候補 D の「0.0f」訂正）/ MidHigh `+0x2C0` = `0x60CF50`（b14 表記と不一致） |
| docs / GT 非変更 | `RE_EXE_REPORT.md` = 1,038,004 B・`GT_CHEATSHEET.md` = 19,734 B（事前値と一致、LastWriteTime も V143 適用時のまま） |
| 禁止対象の編集 | git / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` = **0 件** |

### 証跡（すべて `%TEMP%\opencode\`・既存ファイル非変更）

| ファイル | 役割 |
|---|---|
| `v171_1_vt.py` / `v171_1.txt` | 4 slot の vtable 全数実測（impl→クラス群） |
| `v171_2_disasm.py` / `v171_2.txt` | impl 本文 19 本＋§41.2/§41.3/L9674 呼出し文脈＋`call_targets` 件数 |
| `v171_3.py` / `v171_3.txt` | 件数（decimal キー）/ `+0x2C4`・`+0x2C8` 実測 / FUN_005f3870 prologue / `0x5E40A0`・`0x625220`・`0x484E80` |
| `v171_4.py` / `v171_4.txt` | 文字列/定数実測・decompiled FUN_005febf0 抽出・`+0x108`/`+0xE4` 走査・`0x5F10E0`・ctor `0x4D9A70` |
| `v171_5_dispatch.py` / `v171_5.txt` | 厳格ディスパッチ走査（4 slot / 871・214・201・90 箇所・provenance 付き全件リスト） |
| `v171_6.py` / `v171_6.txt` | `FUN_0060E4F0/E580`・§41.3 乗算 decompiled 上下・`0x4B3750` 再呼出し域・TESCombatStyle `+0x164`・`0x41DF10` |
| `v171_7.py` / `v171_7.txt` | `+0x108` 保存値・件数・FUN_00620290 decompiled・process flags マスク文脈（"Walk "）・FUN_0060E580 decompiled 本文 |
| `v171_8.py` / `v171_8.txt` | bit 文字列（`0xA307F0`="Walk " 等）/ FUN_00407ae0 同定 / `0x62035C` stack 整形 / `0x620D4A` 文脈 |
| `v171_9.py` / `v171_9.txt` | EDL thunk 構造（`0x41E630`/`0x41E650`/`0x41E6F0` = `push type; call 0x41E210 → [node+0xC]`）＝ B の戻り値確定 |

再現: `python "%TEMP%\opencode\v171_1_vt.py"` → `v171_2_disasm.py` → `v171_3.py` → `v171_4.py` → `v171_5_dispatch.py` → `v171_6.py` → `v171_7.py` → `v171_8.py` → `v171_9.py`。

---

## §6 Android 移植への示唆（実装しない）

1. `+0x170` は **base form ポインタ**として実装（formType 読取は呼び出し側）。MagicProjectile 系・process 系は別インターフェイスに分離。
2. `+0x164` は **親セル getter**（Actor 系は process キャッシュ優先、他は extra 0x15 payload）。`[process+0x28]` setter は別メソッド。
3. `+0xEC` は **スケール getter**（Character/Player の race 補正を含む）。「難易度倍率 = 係数 × スケール」は式側の話として再現。
4. `+0x2C0` は **3 メソッドに分離**: `process.getFlags():u16` / `actor.onHpChangedNoop()` / `character.resetHealthCache()`（-1.0f センチネル）。1 API にまとめないこと。
