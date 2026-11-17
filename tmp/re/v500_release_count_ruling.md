# v500 — release の count デクリメント・key stack 引数確定・RTDynamicCast 経路・writer 絞り込み否定側の裁定（V-500-1..3 完遂・旧残点 V-406-3 継続第30弾）

- 採番: **v500**（旧残点群消化・probe prefix **v500_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-500-1..3（v499 残点）** — 1=Player `+0x18` writer 絞り込みと bit24 semantic／2=`unaff_retaddr` key の実渡し方（machine push 確認）と gated mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-500-2: `FUN_00496F50` の key 実渡し方と release 操作** | **[確定]（machine）** | **machine 0x496F50-0x496F95 全 decode [確定(機械)]: `sub esp,8; push ebx/esi/edi; mov esi,ecx` → **`lea ebx,[esi+0x80]`（critical section ＝ this+0x80）** → `push 0xa2f830; mov ecx,ebx; call 0x401020`（enter critical）→ **`mov edi,[esp+0x18]`（**key ＝ stack 引数**）** → `lea eax,[esp+0xc]; push eax; push edi; mov ecx,esi; call 0x496df0`（lookup(this, key, out)）→ `test al,al; je` → **`mov eax,[esp+0x10]; sub eax,1; jne 0x496faa`（**count==1 のときだけクリアへ／count>1 なら減算へ分岐**）** → `push edi; mov ecx,esi; call 0x496d50`（set）** → **`FUN_00496F50` は decompiled `void(void)` と宣言されるが machine では key を stack 引数（`[esp+0x18]`）で受ける [確定] → v499「decompiler の stack 認識制約 [推定]」を機械で裏付け（推定→確定）**。**さらに `count==1` クリア／`count>1` 減算の分岐を獲得 → release は「再帰 count のデクリメント（1なら owner クリア、2以上なら減算のみ）[確定相当]」＝v499「`0xffffffff` で全クリア」を精緻化（`FUN_00496d50(key, 0, -1)` は count==1 分岐の先のみ [確定]）** |
| **V-500-2: `FUN_00496EA0`（acquire）の key 渡し** | **[確定]（machine）** | **machine 0x496EA0-0x496EDC [確定(機械)]: `mov edi,[esp+0x1c]`（**key＝stack 引数（第2引数）**・v498 signature `param_2` と一致）→ `lea ebp,[esi+0x80]`（critical＝this+0x80・release と同一）→ enter → `lea eax,[esp+0x10]; push eax; push edi; mov ecx,esi; call 0x496df0`（lookup 同型）** → **acquire/release が同一 this（`+0x80` critical・`param_1` map）と同一 key（stack 引数）を使う対であることを machine で確定**（v498/v499 docs 構造の機械裏付け） |
| **V-500-1: Player `+0x18` writer 絞り込み** | **[確定]（否定側）** | **「`mov reg,[0xB33B00]` 直後（14命令内）に `[reg+0x18]` write」方式で絞り込み → **0件 [確定(否定側)]** → **Player `+0x18` への write は「Player ptr を load した fn 内」では行われない（別の経路: 別 object の `+0x18` か、ptr を引数で受ける fn）**。v499 の122候補は `0xB33B00` ref と `[reg+0x18]` write が**同一 fn 内に別々に存在する**もの（stack local・他 object を含む）で、**「Player 本体への write」とは限らない [確定]** → **絞り込み法（load→write 同一 reg）は否定され、次の手は「`[reg+0x18]` write の reg が Player かどうかを caller/param 追跡で判定」へ [確定（経路転換）]** |
| **V-500-1: bit24 semantic の経路** | **[確定相当]（cast 経路）** | **`FUN_0045D030` 内 `L73502: iVar7 = FUN_009832e6(uVar6);` [確定(decompiled)] → **`iVar7`＝`FUN_009832E6`（`__RTDynamicCast`・v478/v486 と同 fn）の戻り object** → **`param_1+0x60` list（count/base・v497）の要素（`piVar3[uVar5]`）を RTDynamicCast して得た object の lazy node に bit24（`0x1000000`）を OR 設定 [確定相当]**（`FUN_00452C20(iVar7, 0x1000000)` の第1引数＝cast 結果）。**`param_1+0x60` list が何の object 群か（cast 先 type）は `FUN_009832E6` の引数（`uVar6`）次第＝残点 [部分確定]** |
| **V-500-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v500_1）

### §2.1 `FUN_00496F50` machine（v500_1b）
```
496F50: sub esp, 8 / push ebx / push esi / mov esi, ecx / push edi
496F58: lea ebx, [esi + 0x80]              ; critical = this+0x80
496F5E: push 0xa2f830 / mov ecx, ebx / call 0x401020   ; enter critical
496F6A: mov edi, dword ptr [esp + 0x18]     ; KEY = stack arg  (not void(void)!)
496F6E: xor eax, eax / mov [esp+0xc], eax / mov [esp+0x10], eax   ; out init
496F78: lea eax, [esp + 0xc] / push eax / push edi / mov ecx, esi
496F80: call 0x496df0                       ; lookup(this, key, out)
496F85: test al, al / je 0x496fb8
496F89: mov eax, [esp + 0x10] / sub eax, 1 / jne 0x496faa  ; count==1?
496F92: push edi / mov ecx, esi ...          ; -> set(key, 0, -1) clear path
=> key via stack arg; count==1 clear / count>1 decrement  [確定]
```

### §2.2 `FUN_00496EA0` machine head（v500_1b2）
```
496EA0: sub esp,8 / push ebx/ebp/esi / mov esi,ecx / push edi
496EA9: mov edi, dword ptr [esp + 0x1c]     ; KEY = stack arg (2nd)
496EAF: lea ebp, [esi + 0x80]               ; critical = this+0x80 (same)
496EC0: push 0xa2f830 / mov ecx, ebp / call 0x401020   ; enter
496ED6: lea eax, [esp + 0x10] / push eax / push edi / mov ecx, esi
        -> call 0x496df0                    ; lookup same shape
=> acquire/release share this+0x80 critical and stack-arg key  [確定]
```

### §2.3 writer narrowing + bit24 cast（v500_1a/c）
```
narrowed (load 0xB33B00 -> [reg+0x18] write within 14 insns): 0   [否定側]
  -> Player +0x18 writes do not happen in a fn that loads the Player ptr
  -> v499 122 candidates = same-fn coexistence (stack locals / other objects)
  -> next: trace whether the written reg is the Player via caller/param  [経路転換]
FUN_0045d030: L73502 iVar7 = FUN_009832e6(uVar6);   // __RTDynamicCast
  L73475/73478/73493: param_1+0x60 {count, base, ...} walk
  -> iVar7 = cast result object; FUN_00452C20(iVar7, 0x1000000)  [確定相当]
  -> param_1+0x60 list identity = cast target type (uVar6)  [残点]
```

## §3 残点 **V-501-1..3**
1. 【中】**Player `+0x18` writer の経路転換**（122候補の write について「その reg が Player か」を caller/param 追跡で判定する方法へ）＋ **`param_1+0x60` list の型（RTDynamicCast 先 type・`uVar6` の出所）** → bit24 semantic 完遂。
2. 【低】**`FUN_00496D50` の value 引数の意味**（`(key, thread, 1)`／`(key, 0, -1)` の各 field＝owner/count の対応を machine で確定）＋ gated 側 mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v500_1 の1本 EXIT=0**（V-500-2 key stack 引数＋count 分岐を machine 確定・v499 推定を裏付け／V-500-1 writer 絞り込み0件の否定側と bit24 の RTDynamicCast 経路を確定相当／V-500-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_00496F50` machine を `int3` まで decode** → `mov edi,[esp+0x18]`（key）と `sub eax,1; jne`（count 分岐）を発見 → **v499 の「unaff_retaddr＝制約 [推定]」を machine で確定に引き上げ**＋release の精密化（count==1 のみ clear）→ (ii) **`FUN_00496EA0` head でも `mov edi,[esp+0x1c]`（key）を確認** → acquire/release 対の machine 確定 → (iii) **絞り込み0件の否定側**と **`iVar7=FUN_009832e6()`＝RTDynamicCast** を同時に獲得。
- 妥当性チェック: ①`[esp+0x18]`（release）/`[esp+0x1c]`（acquire）⇔ともに stack 引数（retaddr＋0x10/0x14 分の差＝push 数の差）— ②`lea ebx/ebp,[esi+0x80]`⇔両 fn とも critical＝this+0x80（同一）— ③`call 0x401020`/`0x401040`⇔enter/leave（v498 docs と一致）— ④`FUN_009832E6`⇔`__RTDynamicCast`（v478/v486 と同 fn）— ⑤`param_1+0x60`⇔v497 count/base 構造 — **五方向矛盾なし**。
- docs との不整合: **①`FUN_00496F50` の decompiled signature `void(void)` は machine（stack 引数 key 受け）と不一致＝decompiler の stack 認識欠落として確定記録**（docs 表示の制約）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`void(void)` signature は machine の `[esp+N]` 読みで否定できる** — **retaddr 相当の stack 読み（`[esp+0x18]`/`[esp+0x1c]`）が「引数受取」の証拠**（v499 推定→v500 確定の連鎖）。
  2. **release の操作を `sub eax,1; jne` で見ると「count==1 clear／count>1 decrement」の再帰解除と分かる** — **値（`0, -1`）だけでなく分岐まで読むと運用が確定**。
  3. **「load→write 同一 reg」絞り込みが0件＝否定側** → **経路転換（caller/param 追跡）を次の手として明記**（**絞り込み法の失敗も裁定の価値**）。
  4. **`iVar7 = FUN_009832e6(uVar6)` の1行が bit24 の対象（cast 結果 object）を語る** — **`FUN_009832E6`＝`__RTDynamicCast` の同定が v478/v486 で付いたことの再利用**。
  5. **docs の `+100` decimal（v477）に続く「signature の不一致」も制約の型として記録する**（docs 表示と machine の差を rulings に残す）。
