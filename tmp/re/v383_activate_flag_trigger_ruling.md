# v383 — Activate の flag（s6）設定元とブレンド要求の発火条件の裁定（#152 ②・V-379-2）

- 採番: **v383**（ai-todo #152 束の第2弾、probe prefix v383_1..v383_3）
- 対象: Oblivion.exe v1.2.0416
- 依頼経緯: v379 の残点 **V-379-2（`0x6C4049`（s6=ecx 動的）の caller 側 flag 設定元＝ブレンド要求の発火条件）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| s6 の動的な設定元 | **[確定(否定側)]＋v379 訂正** | **s6 は全15 callers で定数**。`0x6C4000`（crossfade wrapper）は **`6C4034: push 0`（s6=0 固定）**。**v379 の「`0x6C4049` の s6=ecx＝動的 [部分確定]」は切り詰め ctx の読み違いで誤り** — unfiltered では `push ecx` は **s2 位置**（`6C4046: push edx`＝s1、`6C4045: push ecx`＝s2）、s6 は `6C4034: push 0`。動的 s6 は**存在しない** |
| ブレンド要求（state5）の発火条件 | **[確定(命令)]** | **2系統**: ①fn `0x474530` 内の **`47494C: cmp byte [esp+0x38],0`** — **≠0 → `0x47496D: call 0x6C4060`（blend wrapper → `0x6C9E00` → s6=1）**／**==0 → `0x474999: call 0x6C4000`（crossfade wrapper → s6=0 → state2）** ②`0x474A2F: call 0x6C5C70`（同 fn 内の第三経路）→ `0x6C5C70`（ret 0x14・`0x6C5860` lookup 経由）→ **`0x6C9D30`（blend wrapper → s6=1）**。後者 callers＝`0x474A2F`（fn 0x474530 内）と `0x5109E7`（0x51xxxx 帯） |
| wrapper 系統の全体図 | **[確定(命令)]** | **s6=0（state2 単独フェード）**: `0x6C4000`（crossfade: **a1=seq A が active・a2=seq B が inactive のときのみ**、`Deactivate(A,time,flag=0)` → `Activate(B, s1=a4, s2=a5, s3=a6, s4=time, s5=a7, s6=0)`）← E8 callers=`0x474999` の1本。**s6=1（state5 ブレンド）**: `0x6C4060`（←`0x47496D`）→ `0x6C9E00`／`0x6C5C70`（←`0x474A2F`/`0x5109E7`）→ `0x6C9D30` |
| fn `0x474530` の第三経路 | **[確定(命令)]** | `0x47491C: call 0x6C9BA0`（**s1=0, s2=1, s3=1.0, s4=0.0, s5=0, s6=0 → state1 即時再生**・`jp 0x47494C` の false 側）＝**3経路（即時/crossfade/blend）の選択 fn** [確定] |

**#152② への回答**: flag 設定元は**存在しない（全定数）**。ブレンド要求の発火条件は **fn `0x474530` の `byte [esp+0x38]` フラグ**（≠0=blend／0=crossfade）と **`0x6C5C70` 経由の manager ブレンド**の2系統 [確定(命令)]。

---

## §2 証拠

### §2.1 `0x6C4000`（crossfade wrapper）の s6=固定 0（v383_1 unfiltered）

```
6C4000: ecx = [esp+4]（a1 = seq A）
6C4004: cmp [ecx+0x44],0; je fail        ; ★A が active（state≠0）でなければ失敗
6C400B: esi = [esp+0xC]（a2 = seq B）
6C400F: cmp [esi+0x44],0; jne fail       ; ★B が inactive（state==0）でなければ失敗
6C4015: fld [esp+0x10]（a3 = fade time）
6C4019..6C401F: Deactivate(A, time, flag=0)        ; push 0 → 0x6C9CB0
6C4024: eax = [esp+0x20]（a7 → s5）
6C4028: fld [esp+0x10]（a3 → s4）
6C402C: ecx = [esp+0x18]（a5 → s2）
6C4030: edx = [esp+0x14]（a4 → s1）
6C4034: push 0                          ; ★★★s6 = 0（固定）
6C4036: push eax; sub esp,8; fstp [esp+4]（s4=a3）; fld [esp+0x2C]（a6）→ fstp [esp]（s3）
6C4045: push ecx（s2）; push edx（s1）
6C4047: mov ecx,esi（this = seq B）
6C4049: call 0x6C9BA0                  ; Activate(B, s1..s6)
6C404E: pop esi; ret 0x1c               ; 7 引数
```
- **v379 の訂正**: v379_1 は ctx を300字で切り詰め「`push ecx; push edx`＝s6=ecx 動的」としたが、**unfiltered 全 decode では `push ecx`/`push edx` は s2/s1 位置**（`ret 0x1c`=7 引数の末尾2本ではなく先頭2本に対応）で、**s6 は `6C4034: push 0`** [確定(否定側)]。
- E8 callers of `0x6C4000` = **`0x474999` の1本**（v383_2(b)/v383_1(b)）。

### §2.2 fn `0x474530` の3経路選択（v383_2(a)/v383_3(a) unfiltered）

```
474900: jp 0x47494C（float 比較の結果で分岐）
…（path1）
474918: push 1; push 0; sub esp,8; fstp[esp+4]=0.0; fld1→fstp[esp]=1.0; mov ecx,ebp
47491C: call 0x6C9BA0          ; ★path1: (s1=0,s2=1,s3=1.0,s4=0.0,s5=0,s6=0) → state1 即時
474921: jmp 0x474A34
47494C: cmp byte [esp+0x38],0  ; ★★分岐元 byte
474951: je 0x474977            ; ==0 → path2（crossfade）
474953..47496D: (s3=1.0, s4=1.0, s5=0, ...); mov ecx,[esi+0x98]
47496D: call 0x6C4060          ; ★path3: blend wrapper（→0x6C9E00 → s6=1 → state5）
474972: jmp 0x474A34
474977: (path2) test edi,edi; je; cmp [edi+0x44],0; je → 0x4749A8
474981..474999: 7引数構築（a4=0, a5=1, a6=1.0, a7=0）; call 0x6C4000   ; ★crossfade → state2
47499E: test al,al; jne 0x474A34
```
- **`byte [esp+0x38]` が blend/crossfade を選択** [確定(命令)]。fn `0x474530` は v380 で weight 正規化経路（255 割）と同 fn と裁定済み＝**weight/fade の入力計算 fn**。
- fn `0x474530` の E8 callers＝`0x474BA1`/`0x474C34`/`0x474F6C`/`0x47629A`/`0x476EF3`（game 側 animation 帯）。

### §2.3 blend wrapper 系統（v383_2(c)/v383_3(b)）

| wrapper | E8 callers | 経路 |
|---|---|---|
| `0x6C4060`（ret 0x18） | `0x47496D`（fn 0x474530・`[esp+0x38]≠0` 側） | → **`0x6C9E00`**（Deactivate flag=1→state4 → Activate s6=1→state5・v379 §2.2） |
| `0x6C9E00` | `0x6C4088`（＝0x6C4060 内） | state5 ブレンド |
| `0x6C5C70`（ret 0x14・`call 0x6C5860` lookup 経由） | **`0x474A2F`（fn 0x474530 内・第三経路）／`0x5109E7`（0x51xxxx 帯）** | → **`0x6C9D30`**（v379 §2.2 の blend wrapper） |
| `0x6C9D30` | `0x6C5CA8`（＝0x6C5C70 内） | state1 設定→state5 ブレンド |

## §3 残点 **V-383-1..3**
1. **fn `0x474530` の `byte [esp+0x38]` の元**（blend/crossfade 選択の真の入力＝引数か前段計算か。fn 先頭からの decode で確定可）【中】
2. **`0x6C5C70` の identity**（`0x6C5860` lookup＋ret 0x14・callers `0x474A2F`/`0x5109E7`＝NiControllerManager のブレンド系 [推定]）【中】
3. **`0x5109E7`（0x51xxxx 帯）の所属**（どのシステムがブレンドを要求するか）【低】

## §4 検証・自己批評
- probe: **v383_1..v383_3 全 EXIT=0**。
- **決定打の作り**: (i) `0x6C4049` の fn head `0x6C4000` を特定 → (ii) **unfiltered 全 decode で7引数の push 並びを確定（s6=`push 0`）** → (iii) E8 callers census（crossfade=1本/blend wrapper=系統別）→ (iv) `0x474530` の分岐 byte を unfiltered で直接確認。
- 妥当性チェック: ①s6=0⇔`0x6C4000`⇔state2⇔`0x474999`（[esp+0x38]==0 側）— ②s6=1⇔`0x6C4060`/`0x6C5C70`⇔`[esp+0x38]≠0`/第三経路 — ③`0x6C4000` の前提（A active/B inactive）⇔ `0x47497B: cmp [edi+0x44],0` — **三方向で矛盾なし**。
- docs との不整合: **v379 §2.3 の「`0x6C4049` s6=ecx＝動的 [部分確定]」を訂正**（s6=固定0。動的 s6 は不在）。v379 本文は歴史記録として温存、本 v383 §2.1 が正。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
