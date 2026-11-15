# v379 — `0x6C9BA0`（ActivateSequence）の flag 引数（s6）の実体の裁定（#150 残点・V-378-1）

- 採番: **v379**（#150 残点 V-378-1 消化、probe prefix v379_1..v379_2）
- 対象: Oblivion.exe v1.2.0416 ＋先行裁定 v187_anim_residuals.md
- 依頼経緯: v378 の残点 **V-378-1（flag 引数（`[esp+0x20]`＝s6）の実体）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| flag 引数（s6＝`[esp+0x20]`）の実体 | **[確定(命令)]** | **「ブレンド遷移を要求するか」フラグ** — **s6=1 → state 5（他 sequence とのブレンド遷移）**（wrapper `0x6C9D30`/`0x6C9E00` が `push 1` を s6 位置に配置）／**s6=0 → state 2（単独フェード）**（caller `0x47491C`/`0x47603A`/`0x4DAB0B`/`0x4E3087` 等が s6 位置に `push 0`）。s6=動的なのは `0x6C4049`（`push ecx`＝s6）のみ |
| `0x6C9BA0` のスタック引数レイアウト | **[確定(命令)]** | **`ret 0x18`＝6スタック引数（this=ecx 以外）**: **s1（`[esp+0xC]`）＝相手/ターゲット sequence ptr**（`push edi`/`push ebx` が s1 位置）／**s2（`[esp+0x10]`）＝byte（0/1）**／**s3・s4（float）＝fade duration 系**／**s5（`[esp+0x18]`）＝sequence ptr（`push ecx` が s5 位置）**／**s6（`[esp+0x20]`）＝flag byte（0→state2 / 1→state5）** |
| wrapper `0x6C9D30`/`0x6C9E00` の identity | **[部分確定]** | **ActivateSequence 系 wrapper（ret 0x1c＝7引数）** — 先頭で **`Deactivate(fade=0.0, flag=1)`＝state 4** を呼んだ後、state==0 を確認してから **`Activate(..., s6=1)`＝state 5** を呼ぶ。`0x6C9D30` は `[esi+0x44]=1`（state1 即時再生）を先に置き、その後 Activate(s6=1) でブレンド遷移へ [確定(命令)]＋[部分確定] |
| v187 の「真の ActivateSequence 候補 0x6C4890-0x6C4BD0 帯」 | **[確定(否定側)]** | `0x6C4890`＝**dtor**（`mov [esi],0xA7973C`＝vtable → `call 0x7C2A40` → delete）／`0x6C4BD0`＝別物（`cmp word [ebx+0x46],di` → `call 0x6CAC60` → `[esi+0x40]=0`）。**Activate は不在** [確定(否定側)]。**真の Activate は `0x6C9BA0`**（v378 確定） |

**state 2/5 の完全図（v378＋v379）**: **`0x6C9BA0`＝Activate（s6 flag で分岐）** — s6=0 → **state 2（単独フェード）**／s6=1 → **state 5（ブレンド遷移・Manager 通知 `0x6C4480` 付き）**。**wrapper `0x6C9D30`/`0x6C9E00` は常に s6=1（ブレンド）**で Activate を呼ぶ。

---

## §2 証拠

### §2.1 `0x6C9BA0` のスタック引数レイアウト（v379_1(a)/v379_2(a)(b)）

`ret 0x18`＝24バイト＝**6スタック引数**（this＝ecx）。push esi/edi 後のオフセットで:
- **s5＝`[esp+0x1C]`＝相手 sequence**（`0x6C9BB0: edi=[esp+0x1C]`・`[edi+0x58]` 記録）
- **s6＝`[esp+0x20]`＝flag byte**（`0x6C9C03: cmp byte [esp+0x20],0; je 0x6C9C2A`）
- **s3＝`[esp+0x14]`＝duration**（`0x6C9BEF: [esi+0x1C]=[esp+0x14]`）
- **s4＝`[esp+0x18]`＝fade**（`0x6C9BF6: fldz; fcomp [esp+0x18]`）
- **s2＝`[esp+0x10]`＝byte**（`0x6C9C8A: cmp byte [esp+0x10],0`）
- **s1＝`[esp+0xC]`**（未使用箇所あり [部分確定]）

### §2.2 s6=1（state5）の2 wrapper（v379_2(a)(b) unfiltered）

**`0x6C9D30`（ret 0x1c＝7引数）**:
```
6C9D30: fldz; push esi; push edi
6C9D34: push 1; push ecx; fstp [esp]     ; Deactivate(fade=0.0, flag=1) → state 4
6C9D3C: call 0x6C9CB0
6C9D58: cmp [esi+0x44],0; jne fail
6C9D80: [esi+0x44] = 1                    ; ★state 1（即時再生）を先に置く
6C9DAA: push 1; push ecx                  ; s7=1, s6=ecx? → 実際は s6=1
6C9DAD: sub esp,8; fstp [esp+4]; fld [esp+0x30]; fstp [esp]   ; s5,s4 = fade float
6C9DBB: push 0; push edi                  ; s3=0? s1=edi
6C9DC4: call 0x6C9BA0                     ; ★Activate（s6=1 → state5）
6C9DC9: test al,al; je fail
```
**`0x6C9E00`（ret 0x1c）**: 同型 — `push 1; push ecx; fstp [esp]`（Deactivate flag=1 → state4）→ `push 1; push esi; sub esp,8; ...; push 0; push ebx; call 0x6C9BA0`（**s6=1 → state5**）。

### §2.3 s6=0（state2）の callers（v379_1(a)/v379_2(c) unfiltered）

- **`0x47491C`**: `push 0`(s6) → `push 0`(s5) → `sub esp,8; fstp[esp+4]=1.0; fstp[esp]=0.0`(s3,s4) → `push 1`(s2) → `push 0`(s1) → call → **s6=0 → state2**。
- **`0x47603A`**: `push 0`(s6) → `push 0`(s5) → `sub esp,8;1.0/0.0` → `push 1`(s2) → `push 0x64`(s1=100) → call → **s6=0 → state2**。
- `0x4DAB0B`/`0x4E3087`/`0x4E31A7`/`0x570C6E`/`0x5EDE56`/`0x695102`/`0x6956D5`/`0x69CC42`: 全て `push 0; push 0`（s6=0, s5=0）→ **state2**。
- **`0x6C4049`**: `push ecx; push edx`（**s6=ecx＝動的**）[部分確定]。

### §2.4 v187 候補帯の否定（v379_1(c)）

- `0x6C4890`: `push esi; mov [esi],0xA7973C`（vtable）→ `call 0x7C2A40` → `push esi; call 0x401F20`(delete) → `ret 4` ＝ **dtor** [確定]。
- `0x6C4BD0`: `cmp word [ebx+0x46],di` → `call 0x6CAC60` → `[esi+0x40]=0` ＝ 別物 [確定]。
- E8 callers of `0x6C4890` = **0件**（virtual のみ）。
- → **v187 L211 の「真の ActivateSequence（非仮想候補 0x6C4890-0x6C4BD0 帯）」は否定**。**真の Activate は `0x6C9BA0`**（v378 確定）。

## §3 残点 **V-379-1..2**
1. **s1/s2 の厳密な意味**（s1＝`[esp+0xC]`＝ターゲット sequence ptr [推定高]／s2＝byte 0/1 [未確定]。`0x47603A` の s1=100（0x64）は weight 値 [推定]）【中】
2. **`0x6C4049`（s6=ecx 動的）の caller 側 flag 設定元**（ブレンド要求の発火条件）【中】

## §4 検証・自己批評
- probe: **v379_1・v379_2 全 EXIT=0**。
- **決定打の作り**: (i) `0x6C9BA0` の E8 callers 15本 census → (ii) **各 caller の push 並びを unfiltered で取得し s1..s6 の位置を特定** → (iii) **s6=1/0 で state5/state2 に分岐することを callers の値で実証** → (iv) v187 候補帯の dtor 同定で否定。
- 妥当性チェック: ①s6=1⇔wrapper `0x6C9D30`/`0x6C9E00`⇔ caller `0x471A7B` の state5 専用 Manager 通知 — ②s6=0⇔`0x47491C`/`0x47603A`⇔ state2 に通知経路なし — **矛盾なし**。
- docs との不整合: **v187 L211 の候補帯（0x6C4890-0x6C4BD0）を否定**（0x6C4890＝dtor）。v187 本文は歴史記録として温存、本 v379 §2.4 が正。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
