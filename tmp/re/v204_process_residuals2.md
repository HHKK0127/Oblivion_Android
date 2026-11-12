# v204_process_residuals2.md — V187 process 細部束 残留7件（V204 サブ分析）

- 日付: 2026-10-30 / 作成: opencode サブエージェント（TODO serial 191 / V191・低）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B / SHA256 A8F3…DC6）
- 出典: `tmp/re/v200_process_list_residuals.md` §4（残留7件）
- 手法: capstone 5.0.7（`v172_common.py` 経由: dump / disasm / raw E8・imm32 走査）＋ `docs/Oblivion_decompiled.c`（`probe_harness.Ctx` 索引）＋ `cache/{vtable_index.tsv,opcode_table.json}`
- 中間物: `%TEMP%\opencode\v204_1..9_out.txt` / `v204_a..c_out.txt`（12 プローブ）＋対応 `.py`
- selftest: 着手時・完了時とも **4/4 PASS**
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**、ESM 本体へは**未アクセス**。新規は本 1 ファイル＋`%TEMP%\opencode/v204_*` のみ

---

## §1 値1 生成元（L417366 の asm 算術）— **解明**

v200 §4-1 の残点: `(old==4)*2+1` の算術そのもの（setcc/lea 命令列）が disasm ウィンドウずれで未取得だった。

### 実測（`FUN_006545E0` 全文 disasm、`v204_1_out.txt` §B-§D）

```
0x654641  push ebx                        ; state==0 パス（ノックダウン開始）
0x65464F  mov  bl, 4                      ; ★ bl=4 ロード（case 2/4 の比較キー）
   ...（vt+0x1A0 テスト → 0x654717: [edi+0x11c]=bl(=4)、0x6546D0: [edi+0x11c]=3 等）
0x6547E9  movsx eax, byte ptr [edi+0x11c] ; switch 入力（更新後の state）
0x6547F0  add   eax, -1
0x6547F3  cmp   eax, 5
0x6547F6  ja    0x654c4d
0x6547FC  jmp   dword ptr [eax*4 + 0x654c58]   ; jump table
   ...（case 2 / case 4 = 同一ブロック）
0x654824  mov   ecx, esi
0x654826  call  0x5ef930                  ; FUN_005EF930()
0x65482B  cmp   byte ptr [edi + 0x11c], bl  ; ★ old と bl(=4) の比較
0x654831  sete  cl                        ; ★ cl = (old==4) ? 1 : 0
0x654834  lea   ecx, [ecx + ecx + 1]      ; ★ ecx = cl*2 + 1
0x654838  mov   byte ptr [edi + 0x11c], cl  ; 書戻し
0x65483E  mov   ecx, esi
0x654840  call  0x5e12b0                  ; → vt+0x9C(1,0)
```

### 裁定

- **`(old==4)*2+1` は `cmp [reg+0x11c], bl`（bl=4 固定・`0x65464F` ロード）→ `sete cl` → `lea ecx,[ecx+ecx+1]` の 3 命令**。
- **値 1 の生成経路は確定**: state==2 のとき 1 へ（`(0)*2+1=1`）、state==4 のとき 3 へ（`(1)*2+1=3`）。bl=4 は state==0 パスでロードされ switch フォールスルーでも保持される。
- byte 書手カタログ（v200 §2-1 の14箇所）に他の値1 書手は不在＝state machine 内の生成元は本1箇所のみ。
- **v200 §4-1 の留保（「setter 0x629440 経由でも生成し得る」）は残点として維持**（外部 setter の実行時引数は動的のため）。

**判定: 解明**（証拠 `v204_1_out.txt` §A/§B/§C/§D）。

---

## §2 `FUN_005E0DC0` の [+0xB0] 正式名 — **解明: `TESObjectREFR+0xB0 = LifeState`**

v200 §4-2 の残点。本タスクで**名称テーブルを直接発見**して確定。

### 証拠1: ShowAnim（opid `0x0112`）の名前テーブル（`v204_4_out.txt` §A/§C/§D）

`FUN_00509D70`（ShowAnim impl）は `mov ecx,esi; call 0x5e0dc0` の戻り値で **`[eax*4 + 0xB09EF8]`** を引き、**`"Life State: %s"`（0xA4C51C）** として印字する。テーブル `0xB09EF8` 実測:

| idx | 文字列 | 命名 |
|---|---|---|
| 0 | `Alive` | 生存 |
| 1 | `Dying` | 死にかけ |
| 2 | `Dead` | 死亡 |
| 3 | `Unconscious` | 気絶 |
| 4 | `Reanimate` | 蘇生中 |
| 5 | `Restrained` | 拘束 |
| 6 | `Normal` | 通常 |

### 証拠2: ゲッタ本文と vtable

- `FUN_005E0DC0` = `mov eax,[ecx+0xB0]; ret`（2命令）。
- **Character/Creature/PlayerCharacter vtable slot +0x198 = `0x5E33B0`**: `cmp [esp+4],0; mov eax,[ecx+0xB0]; je …` — flag=0 なら `{Dead,Dying,Normal}→1`、flag≠0 なら `{Dead,Dying}→1` の **LifeState 述語**（＝FSM 入口ガードで呼ばれるもの）。
- TESObjectREFR vtable slot **+0x190 = IsActor**（base `0x69D990`=`xor eax,eax;ret`=0 / Character・Creature・Player は `0x977C50`=`mov al,1;ret`=1）— v200 の「vt+0x190=IsActor」を実装レベルで確認。
- 直接 caller 56 件の実測（`v204_3_out.txt` §T2-A3）: `mov ecx,[0xb333c4](g_thePlayer); call 0x5e0dc0; cmp eax,2`（`0x452330`=FUN_00452330「player が Dying/Dead でなければ…」述語）、`cmp eax,1` 等 — **呼び出し先が「参照オブジェクト」そのもの**であること（＝REFR のフィールド）を多重確認。

### 証拠3: スクリプト述語（`v204_6_out.txt` §C/§D）

- `0x4F7370`（cond 関数）: IsActor ガード → `call 0x5e0dc0; cmp eax,3; jne; fld1; fstp [edi]` = **LifeState==3 → 1.0**。opcode 表で **opid `0x10F2` GetUnconscious**（impl `0x505B50` → `call 0x4f7370`、cond=`0x4F7370`）と一致。
- `0x4F73F0`: 同型で **`cmp eax,5`** = **LifeState==5 → 1.0** = **opid `0x10F4` GetRestrained**（impl `0x505C00` → `call 0x4f73f0`）。
- ⇒ **3=Unconscious / 5=Restrained がオペコード名と二重照合**。

### `==6` ガードの意味（v200 §2-1「6（死亡）なら無変更」の訂正）

`FUN_006545E0` 入口（`0x654613..0x654631`）:

```
vt+0x198(0)（=LifeState 述語: {Dying,Dead,Normal}→1）が真 ∧ GetLifeState != 6 → [+0x11c]=0 で knockdown クリア
```

- 実効: **LifeState ∈ {1(Dying), 2(Dead)} のとき knockdown 状態がクリアされる**。{0(Alive), 3, 4, 5, 6(Normal)} では FSM が続行する。
- つまり **==6 ガードは「Normal も Knockdown 適格として扱うための例外」**。v200 の「6＝死亡」は誤り（**死亡=2**、6=Normal）。
- case 3（Knocked Out）内 `FUN_005e0dc0()==6 → return`（L417381）も同じ語義（Normal 中は GetUp 進行しない＝リセット/通常状態への脱却待ち）。

### 副産物

- `FUN_005E0EE0` = **GetAnimAction**: `[ref+0x58]`（process）→ `process->vt+0x2D0()`、無いなら -1。名前テーブル `0xB14C80` = `{"Equip Weapon","Unequip Weapon","Attack","Attack Follow Through","Attack Bow","Attack Bow Arrow Attached","Block","Recoil","Stagger","Dodge","Lower Body Anim","Special Idle"}`（`"Animation Action: %s"` 0xA4C52C）。
- `FUN_00659A00` = **GetProcessLevel**（テーブル `0xB14998`: High / Middle High / Middle Low / Low、`"Process Level: %s"` 0xA4C56C）。
- 0x47 帯の `+0xb0` 配列走査（L90776 等）は **NiAVObject/NiNode の children 配列**＝別クラスの同名オフセット（LifeState と混同しないこと）。

**判定: 解明**（証拠 `v204_4_out.txt` / `v204_5_out.txt` §A-B / `v204_6_out.txt` / `v204_7_out.txt` §A）。

---

## §3 UI キー入力クラス（0x57 帯）の正式名 — **解明: `InterfaceManager`（RTTI なし・シングルトン `0xB3A6E0`）**

v200 §4-3 の残点。クラス名の RTTI は存在しない（`cache/vtable_index.tsv` 全 1715 行を走査して **vtable スロットから FUN_0057F7A0/0x57DC30/0x57DC60/0x4032D0/0x403CF0 への参照は 0 件**＝direct-call のみ）。同定は以下で確定:

1. **TU 文字列**: 0x57D-0x586 帯に `".\Interface\InterfaceManager.cpp"`(0x57EE7C)、`"InterfaceManager: Main Root/Cursor Root/Menus Root/DebugText Root"`(0x57EE91/0x57F006/0x581D40/0x581EC1)、`MenuRoot`/`Menu3DRoot`/`Menus\Misc\cursor.dds`、メニューコマンド群（`reload`/`delete`/`close all menus`/`playerpos`/`repair`/`bat`…）— 0x57F7A0 近傍（`v204_3_out.txt` §T3-A4）。
2. **唯一の呼び出し元**: `FUN_0057F7A0` 直接 caller は **4 件すべて `FUN_005821F0` 内**（`0x5834E3/507/595/5B9`）。`FUN_005821F0` = メニュー ID（`FUN_00589b70(0x3EC/0x3ED)`）・カーソル・`MenuRoot` を扱うマネージャのイベント関数。
3. **インスタンス取得系**: `FUN_005821F0` の caller 3 件はすべて `FUN_00582160(0,1)` の結果を `ecx`（this）へ。**`FUN_00582160` = 遅延シングルトン getter**: `[0xB3A6E0]==0` なら `malloc(0x134)` → ctor `FUN_005802B0` → `[0xB3A6E0]=obj` → 返却（`v204_c_out.txt` §D）。
4. **フィールド整合**: キー入力系の this が持つ `+0x114`(shift byte)/`+0x118`(Shift・Ctrl・Alt ビット)/`+0x11c`(pendingKey)/`+0x120/+0x124`(tick) はシングルトン size 0x134 の範囲内。`FUN_005821F0` 本体も `param_1+0x10c/+0x110`(メニュー要求フラグ) 等を同一オブジェクトとして操作。

### 裁定

> **UI キー入力クラス = `InterfaceManager`（RTTI なし・ソースファイル名由来の同定）**。シングルトン **`0xB3A6E0`（size 0x134、ctor `FUN_005802B0`）**、取得は `FUN_00582160(mode, create)`。`FUN_0057F7A0` = キーイベント処理、`FUN_0057DC30` = pendingKey セッタ、`FUN_0057DC60` = タイムアウト更新。v200 §2-3 の「オーナークラス正式名 [残]」はこれで解消（**ただし RTTI 実文字列は存在しない**＝正式な mangled 名は取得不能）。

**判定: 解明（実名は "InterfaceManager"、RTTI は不在）**（証拠 `v204_3_out.txt` §T3-A2/A4・`v204_c_out.txt` §D）。

---

## §4 `form+0x90` byte 4/5 と process `+0xF4/+0xF5` の語義 — **解明: weapon type byte / Bow・Staff 装備フラグ**

### 証拠1: 検証コードと型名文字列（`v204_b_out.txt` §A/§C、`v204_a_out.txt` §A）

- ESM ロード側検証（`0x4BB300`、TESObjectWEAP 系）:
  ```
  cmp byte [esi+0x90], 6 ; jl ok
  → sprintf("Clearing invalid type on weapon '%s' (%08X).", form名, formID) ; [esi+0x90] = 0
  ```
  ⇒ **`form+0x90` = weapon type byte、有効値 0..5**。
- 型名文字列クラスタ（`v204_b_out.txt` §A）: `Bow`(0xA451F8) / `BluntTwoHand`(0xA451FC) / `BluntOneHand`(0xA4520C) / `BladeTwoHand`(0xA4521C) / `BladeOneHand`(0xA4522C)。MSVC 逆順プーリング（Staff は他所とプール）と合わせ、**enum 順は `0=BladeOneHand, 1=BladeTwoHand, 2=BluntOneHand, 3=BluntTwoHand, 4=Staff, 5=Bow`** [確定寄り]。

### 証拠2: 使い分け（3 サイトで一貫）

| サイト | 条件 | 動作 |
|---|---|---|
| `FUN_00658550`（High vtable slot+0x104 setter、`v204_8_out.txt` §A） | type==4 (Staff) | `[this+0xF5]=1` |
| 同上 | type==5 (Bow) | `[this+0xF4]=1` |
| `0x6508D0`（slot+0x124、`v204_8_out.txt` §F） | type ∈ {1,3,4,5}＝2H 系（Blade2H/Blunt2H/Staff/Bow） | code **4** へ tail-call `FUN_00477EC0` |
| 同上 | type ∈ {0,2}＝1H 系 | code **5** |
| `FUN_00479780` L97227 / `FUN_004E1900` L177150（`v204_9_out.txt` §F） | type==5 (Bow) | アタッチコード **0xE**＋`FUN_006ff420("Bow")` ログ、否则 code 9 |

### 裁定

> **`form+0x90` = TESObjectWEAP の weapon type byte（0=BladeOneHand … 4=Staff, 5=Bow）**。ARMO/WEAP の「手扱い系 byte [推定]」（v200 §4-4）は**訂正** — 実体は武器種別。
> **process `+0xF4` = 「Bow 装備中」フラグ、`+0xF5` = 「Staff 装備中」フラグ**（setter `FUN_00658550` が装備時のみ設定、装備替えでクリア）。用途: `0x64B110`（slot+0x118）が `[+0xF4]==0 ? this[+0x100] : this[+0xFC]` と **Bow 与否でアドオン・スロットを切り替え**、`0x6508D0` が type の 1H/2H 判定でアタッチコードを振り分ける。Bow は専用アタッチコード 0xE を持つ（弓は通常武器と別モデル経路）。

**判定: 解明**（証拠 `v204_a_out.txt` / `v204_b_out.txt` / `v204_8_out.txt`）。

---

## §5 `+0xFC..+0x110` 第2ファミリの直接証拠 — **解明: 「装備アイテム 3D アドオン記録」ファミリ（v200「エフェクト/魔法系 [推定]」を訂正）**

### 直接証拠（4 系統）

1. **ctor 分離**: `FUN_0064B400`（MidHigh ctor）は `+0xE4..+0xF0`（4 dword＝装備 4 リスト）と **`+0xFC/+0x100/+0x104/+0x108/+0x10C/+0x110`（6 dword 連続、`0x64B4A4..0x64B4C2`）を別バッチでゼロ**（`v204_9_out.txt` §B）。
2. **専用 vtable ゲッタ群（slot +0x118..+0x134、`v204_8_out.txt` §F）** — 装備 4 リストのゲッタ帯（+0xEC..+0xF8）とは**別のスロット帯・別のアクセス・パターン**:

   | slot | impl | flag==0 の戻り | flag≠0 |
   |---|---|---|---|
   | +0x118 | `0x64B110` | `[+0xF4] ? this[+0x100] : this[+0xFC]` | `FUN_00477EC0(arg, 3)` |
   | +0x11C | `0x64B150` | `this[+0x100]` | code 8 |
   | +0x120 | `0x64B1B0` | `this[+0x104]` | code 7 |
   | +0x128 | `0x64B1D0` | `this[+0x10C]` | code 6 |
   | +0x130 | `0x64B170` | `this[+0xFC]` | — |
   | +0x134 | `0x64B190` | `this[+0x100]` | — |
   | +0x12C | `0x64B1F0` | `arg==FUN_006600D0(player,1) ? [0xB3BA98](グローバル) : this[+0x110]` | — |

3. **`FUN_00477EC0` = `return [arg + code*8 + 8]`**（`v204_a_out.txt` §C）— flag≠0 経路は「**アドオン記録オブジェクトの code-indexed サブスロット**（code3..8 → 記録+0x20..+0x48）」を返す dispatcher。
4. **破棄ヘルパ系（`v204_a_out.txt` §B、`v204_b_out.txt` §C）**:
   - `FUN_00478780(this, field_ptr, flag, arg)` = **スロット破棄**: `[field+8]`（Ni/add-on）を Ni シーンから除去（`0x7B4280`/`0x7C5E70`/`0x481350`）＋ refcount リリース、`[field+4]` を `vt+0x14` 経由で解放、`[field+0..]` クリア。`field == &this[+0xDC]` の場合さらに `[this+0x28]/[this+0x30]` の `FUN_00897A90`＋`[this+0x150]==g_thePlayer ∧ this==FUN_006600D0(0)` なら **HUD アイコン更新**（`FUN_0057B190`）。
   - `FUN_00478DB0` = `FUN_00478780(this, &this[+0x10C], 1, 0)` = **+0x10C スロット破棄**（呼び出し元 `0x4DCD1A/0x4DCD3F`）。
   - 同型: `+0xDC`（WEAP・`FUN_00478CE0`）、`+0x12C/+0x130`（**LIGH**・`FUN_00478DF0` が formType `0x1A` ガードで `[this+0x12C]=form; [this+0x130]=form+0x30`）、`+0xCC`（`FUN_00478EA0`）。
   - `FUN_00484470` = 12B エントリ破棄（`[e+0]`=list head → `FUN_004526E0`（`{value,next}` 連鎖 free）→ `free(head)`）、`FUN_00484420` = 12B エントリ構築（`malloc(8)` head ノード、`[e+8]=form`）— v200 のエントリ構造をアセンブラレベルで再確認。

### 裁定

> **`+0xFC..+0x110` は「装備アイテムの 3D アドオン記録」6 スロット**（Bow/Staff/1H/2H 等のアタッチ先バリエーションを code3..8 で按分）。v200 §4-5 の「エフェクト/魔法系 [推定]」は**証拠と反するため訂正**。エフェクト/魔法系という直接証拠は本調査では**1 件も検出されず**（負の証拠）。

**判定: 解明（第2ファミリ＝アドオン記録系）＋ v200 推定を訂正**（証拠 `v204_8_out.txt` §F / `v204_9_out.txt` §B / `v204_a_out.txt` §B-C / `v204_b_out.txt` §C）。

---

## §6 `FUN_00479780` param_1+0xe4 の process 同一性 — **解明: 同一（process = High/MidHigh）**

v200 §4-6 の残点（「0x47 帯の他クラス同名オフセットの可能性 [有力]」）。本タスクで**否定して確定**。

### 証拠

1. **this の出所**: `FUN_00479780` 直接 caller は `FUN_004E1900` の 2 箇所（`0x4E196D`/`0x4E1995`）。後者は `call 0x6600d0`（結果を `edi` へ）→ `mov ecx,edi; call 0x479780`。**`FUN_006600D0(player, flag)` = `flag ? [player+0x5C8] : [player+0x104]`（player の perspective 別 process）**（`v204_7_out.txt` §H）。比較パターンも多数: `cmp esi/ebx/ebp, FUN_006600D0(...)`（`0x478D3A`/`0x479865`/`0x479BA4`/`0x47C260`/`0x478CE0`/`0x478780` 内）— すべて「this 系 == process」比較。
2. **`FUN_00478780` 内の直接比較**（`v204_b_out.txt` §C L96342-46）: `if ([this+0x150] == g_thePlayer && this == FUN_006600D0(0)) → HUD アイコン更新` — **破棄ヘルパの this も同一クラス**。
3. **`[this+0x150] = 所有 TESObjectREFR**`: `IsActor`（vt+0x190）呼び出し（L97356）、`[owner+0x58]`（process）の逆参照（L97357 `piVar9[0x16]`、asm `0x47C248: mov esi,[ebp+0x150]; cmp [esi+0x58],0`）、`g_thePlayer` 比較（L97232）。HighProcess copy 関数 `FUN_0064B650` も `param_1+0x150 = param_2[0x54]`（同オフセットコピー、L412506）。
4. **`[this+0xDC]/[+0xE0] = 装備武器フォーム`**: formType `0x21`（WEAP）ガード（L97221 `*(param_2+4) != '!' → goto`）、`[this+0xDC]=form; [this+0xE0]=form+0x30`（asm `0x4797E7-0x4797ED`）。
5. **`+0xE4` の同一スロット性**: `FUN_00479780` は `mov [ebx+0xe4], FUN_00479450(...)`（`0x479858`、アライナス disasm で再検証）— これは High vtable slot+0x104 setter `FUN_00658550` の書先 `[esi+0xe4]` と**同一オフセット・同一クラス**。

### 裁定

> **`FUN_00479780` の param_1（this）は High/MidHigh process で確定**（perspective 別 player process 比較・owner-ref 逆参照・WEAP form ガードの三重証拠）。`param_1+0xE4` は **process+0xE4（WEAP 装備/アドオン・スロット）と同一フィールド**。0x47 帯の他クラス混在説は否定。

**判定: 解明**（証拠 `v204_2_out.txt` §T6-A/B/C、`v204_7_out.txt` §C-D/G、`v204_8_out.txt` §C-D/F、`v204_9_out.txt` §C/§E、`v204_b_out.txt` §C）。

---

## §7 GetKnockedState bool と GECK int 0..6 の対応 — **解明（＋KnockedState 公式名を新発見で v200 値名を訂正）**

### 実測1: GetKnockedState 本体（`v204_3_out.txt` §T7-A/B）

`opid 0x106B GetKnockedState`（impl `0x504950` → `FUN_004F76B0`）:

```
*out = 0.0; if (!ref) return; if (!IsActor(ref)) return;
process = [ref+0x58] → RTDynamicCast(High/MidHigh) → process->vt+0x2E4()（KnockedState int）
cmp eax,6; ja → 0.0                       ; 0..6 範囲外は 0.0
movzx ecx, byte [eax + 0x4F7748]          ; byte table 実測: {0, 2, 2, 1, 1, 0, 0}
jmp [ecx*4 + 0x4F773C]                    ; case0=0x4F770F fldz / case1=0x4F7713 fld1 / case2=0x4F7717(維持=0.0)
```

⇒ **state ∈ {3,4} のとき 1.0、{0,1,2,5,6} は 0.0**（bool 投影）。v200 §2-4(5) の jump table 解釈と完全一致。

### 実測2: KnockedState 公式名テーブル（`v204_5_out.txt` §B — 本タスクの新発見）

ShowAnim が `process->vt+0x2E4()` の値で **`[eax*4 + 0xB09F3C]`** を引き **`"Knock State: %s"`** として印字。テーブル実測:

| state | 公式名 | v200 §2-2 の旧命名 | 裁定 |
|---|---|---|---|
| 0 | **Normal** | None | ほぼ一致 |
| 1 | **Explode** | KnockedDown（倒れ中） | **訂正: 爆発反応** |
| 2 | **Explode Lead In** | KnockBack（吹き飛ばし直後） | **訂正: 爆発リードイン** |
| 3 | **Knocked Out** | GettingUpPrep（GetUp アニム探索中） | **訂正: 気絶中** |
| 4 | **Knock Out Lead In** | KnockDown 開始（anim なし ragdoll） | **訂正: 気絶リードイン** |
| 5 | **Queued** | GettingUp（起床アニメ再生） | **訂正: 起き上がり待ち** |
| 6 | **Getting Up** | Recovered（復帰処理中） | **訂正: 起き上がり中** |

- 遷移は `(old==4)*2+1` = **Lead-In(偶数) → Main(奇数)**（2→1 / 4→3）として完全に整合: impulse → 2(Explode Lead In) → 1(Explode)、trigger → 4(KO Lead In) → 3(Knocked Out) → 5(Queued) → 6(Getting Up) → 0(Normal)。
- **GetKnockedState の bool {3,4}→1.0 は「Knocked Out ペア（3/4）中」の意味**で、GECK ドキュメントの int 0..6（内部 enum）に対する**真の bool 投影**。

### 対応関係の裁定

> - 本 EXE の `GetKnockedState`（opid `0x106B`）は **double 0.0/1.0 を返す bool 表現**（jump table {3,4}→1.0）。VM は全値 double 型であり、int↔double 変換パスは存在しない（impl 内の jump table が投影を完結）。
> - **GECK ドキュメントの「int 0..6」は内部 KnockedState enum（`process+0x11C`、vt+0x2E4 getter）の説明**であり、スクリプト・オペコードの戻り値型ではない。int のまま状態値を返す公開経路は**本 EXE には存在しない**（`ShowAnim` の "Knock State" トレースと vt+0x2E4 のみが内部値を露出）。
> - bool の意味: **「Knocked Out（気絶）状態か」**（Explode 系 1/2 や Getting Up 6 は false）。

**判定: 解明**（証拠 `v204_3_out.txt` §T7-A/B/E、`v204_5_out.txt` §B）。

---

## §8 新規残点（本タスクから起票）

| ID | 内容 | 優先 |
|---|---|---|
| **V-204-1** | `process+0xE4` スロットの二重性: setter `FUN_00658550` は [+0xE4] を「12B エントリ（head/x/form・`0x484470`+`free` 破棄）」として扱う一方、`FUN_00478780`/`FUN_00479780` は [+0xE4] を Ni アドオン（`0x7B4280`/refcount 処理）として扱う。同一オフセット・同一クラス（process）で確定したため、**エントリとアドオンの実体的な関係**（アドオンがエントリを兼ねる？ヘッド連鎖がアドオンノード？）の詰めが必要 | 中 |
| **V-204-2** | Character vt+0x198（`0x5E33B0`）の正式名（flag=0 → {Dying,Dead,Normal}→1 / flag≠0 → {Dying,Dead}→1。"IsDeadish?" 系述語だが命名未）と vt+0x1A0（`0x5E17E0` → `vt+0x284(0x30)` 委譲）の意味 | 低 |
| **V-204-3** | 武器 type enum の 4 番を Staff と確定する独立証拠（Staff 文字列のプール先 xref。"Clearing invalid type" 検証（<6）と 5=Bow は確定、4 の名称は推定） | 低 |
| **V-204-4** | `FUN_006600D0` が返す `[player+0x104]`（3rd person process）/`[player+0x5C8]`（1st person process）の格納経路（書手）と TESObjectREFR/PlayerCharacter とのレイアウト境界（+0x104 が REFR 内か Player 専用か） | 低 |
| **V-204-5** | LifeState の書手（1=Dying/2=Dead/4=Reanimate/6=Normal を書く関数群）と ctor 既定値（TESObjectREFR ctor `0x4D9A92` 帯の +0xB0 初期化） | 低 |
| （v200 継続） | 外部 setter `0x629440` 経由の KnockedState 値1 生成の実在性（動的引数のため本タスクでは不変） | 低 |

---

## §9 自己検証・証跡

- **`probe_harness.py selftest` 4/4 PASS**（着手時・完了時）。
- 内部整合（独立一致 10 件）:
  1. LifeState 名前テーブル（ShowAnim）と GetUnconscious/GetRestrained オペコード（impl→cond→`cmp eax,3/5`）が 3/5 で一致。
  2. LifeState==6 ガードの極性が Character vt+0x198（`0x5E33B0`）の `{Dying,Dead,Normal}→1` と FSM 入口 `test al,al → je / cmp eax,6 → je` で整合（クリア条件 = {1,2}）。
  3. KnockedState 名前テーブル（"Knock State: %s"）と `(old==4)*2+1` の Lead-In→Main 映射（2→1, 4→3）、および GetKnockedState bool {3,4}=Knocked Out ペアが整合。
  4. 武器 type 検証（`[+0x90] >= 6 → clear`）と型名文字列の逆順プーリング（Bow 最低アドレス）が enum 0..5・5=Bow と整合。
  5. `FUN_00658550`（4→+0xF5, 5→+0xF4）、`0x6508D0`（{1,3,4,5}=2H vs {0,2}=1H）、`FUN_00479780`（5→code 0xE+"Bow"ログ）の 3 サイトが type 分岐で一貫。
  6. +0xFC..+0x110 のゲッタ帯（slot+0x118..+0x134）が装備リスト帯（+0xEC..+0xF8）と分離、かつ ctor のゼロ化バッチも分離。
  7. `FUN_00479780`/`FUN_00478780`/`fun_00478ce0` の this がいずれも `FUN_006600D0` 結果と比較・等価であり、`[this+0x150]`=owner-ref（IsActor 呼び出し・`[+0x58]` 逆参照）で一致。
  8. `FUN_006600D0` 本体（`flag ? [this+0x5C8] : [this+0x104]`）と caller 38 件の比較パターンが整合。
  9. 12B エントリ構造（`FUN_00484420` 構築 / `FUN_00484470`+`0x4526E0` 破棄）が asm と decompiled（v200 §2-4(3)）で一致。
  10. `FUN_00582160` のシングルトン（`0xB3A6E0`/size 0x134/ctor 0x5802B0）と `FUN_005821F0` caller 3 件の `FUN_00582160(0,1)` パターンが一致。
- 制約遵守: 新規ファイルは本 1 件のみ。docs/・既存 tmp/re・GT・TODO・session-memory・plan・cache・git は**編集 0 件**。ESM 読取 0 回。中間物は `%TEMP%\opencode/v204_*`（12 プローブ＋12 出力）。
