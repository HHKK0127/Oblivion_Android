# v426 — `0x724700` wrapper・`FUN_0070B780` NiNode ctor・「virtual `[+0xC]` setter」誤認の裁定（V-426-1/V-426-2 完遂・派生残点第18弾）

- 採番: **v426**（派生残点群・probe prefix **v426_1・v426_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v425 の残点 **V-426-1【低】（self-attach 完全否定（`0x724700` virtual callers）と `FUN_0070B780` factory の作成クラス）／V-426-2【低】（NiAVObject `[+0xC]` identity＝virtual setter 群 `0x6C0B80` 等の vtable 所属）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0x724700`（wrapper）の identity** | **[確定(RTTI)]** | **`NiLODNode`（vtable 0xA7F97C）と `NiSwitchNode`（vtable 0xA7FA3C）の slot33(+0x84)**（data refs 全2件）— `FUN_0070AF00`（NiNode 系 slot33・v425）へ透過する **subclass override wrapper**（obj=arg1・flag=arg2） |
| **`FUN_0070B780` の identity** | **[確定]**（v423_1 の find_head 归属を**訂正**） | **NiNode in-place ctor** — SEH prologue `0x70B780`（＝独立 fn・annotation `// FUN_0070b780 @ 0x70b780` L550247）: `FUN_00708450`（sub-ctor・v423 の runtime attach 3フィールド0初期化）→ **`*this = 0xA7E38C`（NiNode vtable）** → **`FUN_004b2d30(&this[+0xAC], param_2, 1)`（embedded array 初期化）** → `[+0xC8]=0/[+0xC0]=0/[+0xC4]=0`／**`[+0xBC] = 0xA7E378`（副 vtable・sub-object）** → return this。**v423_1「NiNode ctor head=0x70B6A0・vptr store 0x70B7C3」は find_head の境界誤りで訂正（vptr store 0x70B7C1 は FUN_0070B780 内）** |
| **attach 対象の最終像（V-425-1 の補強）** | **[確定相当]** | site1＝**新規作成 NiNode**（`FUN_0070B780` ctor 経由）を `this[+0x1C]` へ・site2＝**既存 child** を `this[+0x1C]` へ → **`[+0x1C]` には child NiNode（作成/既存）が入る**・**direct callers に self-attach なし** → **epilogue（child の `[+0xC]`）と fallback（node 自身の `[+0xC]`）は別オブジェクトの同一構造リスト [推定高→強化]**（V-423-1 の回答） |
| **「virtual `[+0xC]` setter 群（`0x6C0B80`/`0x6C0BF0`/`0x6C1FC0`）」** | **[確定(否定側)]（v423 の誤認を撤回）** | **virtual setter ではない**: ①data refs 全0件（vtable 登登録なし）②E8 も0件（当該アドレスへの直接呼出しなし）③**body ＝ struct copy routine** — `0x6C0B80`: `src=[esp+8]`→`dst=[esp+4]` へ float/dword を `0x00..0x14`（20〜24B）コピー（`mov [ecx+0xc],edx` はコピーの一部）／`0x6C0BF0`: `lea` 群の構造体展開／`0x6C1FC0`: `fld1/fcomp` 条件付き3フィールドコピー → **「`[+0xC]` への bucket-head 付与 fn」は存在しない＝v423 §1 の該当記述は撤回** |
| **node `[+0xC]` chain head の書手** | **[未確定]（残点 V-427-1）** | 誤認撤回により**書手未特定に再設定** — fallback walk が `[node+0xC]` を `{vtable, next@+0x34}` chain 先頭として読むこと自体は v421/v422 で確定済み。書手は別経路（未走査） |

## §2 証拠（v426_1/v426_2）

### §2.1 `0x724700`（v426_1(a)）
```
.rdata VA=0xA7FA00 -> vtable 0xA7F97C slot 33 (+0x84) RTTI=.?AVNiLODNode@@
.rdata VA=0xA7FAC0 -> vtable 0xA7FA3C slot 33 (+0x84) RTTI=.?AVNiSwitchNode@@
total: 2
```

### §2.2 `FUN_0070B780` body（machine 0x70B780..0x70B7FB＋decompiled L550247-550271）
```
70B780: push -1 / push 0x9c9a08 / fs:[0]...        ; SEH prologue（独立 fn）
70B7A3: mov esi, ecx ; this
70B7A9: call 0x708450                              ; sub-ctor（[+0x9C]/[+0x1C]/[+0x24] 0初期化＝v423）
70B7B7: lea ecx, [esi + 0xac] ; embedded array base
70B7C1: mov dword [esi], 0xa7e38c                  ; ★ NiNode vtable
70B7C7: call 0x4b2d30                              ; array init(&this[+0xAC], param_2, 1)
70B7CC/70B7D2/70B7D8: [+0xc8]/[+0xc0]/[+0xc4] = 0
70B7DE: mov dword [esi + 0xbc], 0xa7e378          ; 副 vtable（sub-object）
70B7E8: mov eax, esi / ret 4
```
- decompiled: `*param_1 = &PTR_FUN_00a7e38c; FUN_004b2d30(param_2,1); param_1[0x32]=0; [0x30]=0; [0x31]=0; [0x2f]=&PTR_FUN_00a7e378`（`[0x2f]` dword＝+0xBC ✓）。

### §2.3 `FUN_0070B780` の callers と v423_1 訂正（v425_2/v426_1）
- `FUN_004112E0` site `0x4115D1: call 0x70b780; mov edi, eax`（＝ctor 経由で NiNode 作成 → attach）。
- **v423_1 の「vptr store @0x70B7C3 → head=0x70B6A0」は find_head の境界誤り** — 実際の vptr store `0x70B7C1` は **SEH prologue 付きの `FUN_0070B780`（0x70B780 起点）** 内（0x70B6A0 は別 fn）。＝**v410「find_head 境界飛び越し」教訓の再発事例**。

### §2.4 「virtual setter」撤回（v426_2）
- data refs: `0x6C0B80`/`0x6C0BF0`/`0x6C1FC0` すべて **0件**。
- body: `0x6C0B80`＝`mov eax,[esp+8]; fld [eax]; mov ecx,[esp+4]; fstp [ecx]` → `[+4]`/`[+8]`/`[+0xc]`/`[+0x10]` dword ＋ `[+0x14]` float を **src→dst コピー**（stdcall 2引数 struct copy）／`0x6C1FC0`＝`fld1/fcomp` 条件で src を選んで3フィールドコピー。

## §3 残点 **V-427-1..2**
1. **【低】node `[+0xC]` chain head の書手（誤認撤回後の再設定）** — fallback walk の container `[node+0xC]` に `{vtable, next@+0x34}` entry 先頭を代入する経路（未走査。ctor0クリア＋direct 書手0件＝v423 確定の範囲から更に外側: `add [reg+0xc]`/indirect/別 fn 帯）。
2. 【低】`0x724700`（NiLODNode/NiSwitchNode slot33）の実 callers（virtual dispatch・self-attach の最後の残可能性）と `FUN_004b2d30`（array ctor・growBy 引数）の identity。

## §4 検証・自己批評
- probe: **v426_1・v426_2 の2本全 EXIT=0**（V-426-1 完遂＋V-426-2 は[確定(否定側)]で誤認撤回）。
- **決定打の作り**: (i) **data refs の uncapped RTTI scan**（v425 教訓の即適用）で wrapper の owner2クラスを一撃特定 → (ii) **SEH prologue（`push -1; push handler; fs:[0]`）が独立 fn の証明**となり find_head 归属を訂正 → (iii) **「virtual setter」仮説を data refs/E8/body の3系統で同時否定**。
- 妥当性チェック: ①0x724700＝slot33 ⇔ FUN_0070AF00 への透過（v424 の push 形と一致）— ②FUN_0070B780＝NiNode ctor ⇔ vtable 0xA7E38C⇔embedded array `FUN_004b2d30(&[+0xAC])` ⇔v425 の fields — ③struct copy body ⇔data refs/E8 全0 — **三方向矛盾なし**。
- docs との不整合: **①v423 ruling §1「`[+0xC]` の virtual setter 群（0x6C0B80 等）」は本裁定で撤回（＝struct copy）②v423 §2.1「NiNode ctor head 0x70B6A0」は FUN_0070B780（0x70B780）へ訂正**。docs/GT 反映時は v423 注記を併せて整理・#47/#20 一括。
- **教訓**:
  1. **SEH prologue（`push -1; push handler; mov eax,fs:[0]`）は「独立 fn の開始」の決定打** — find_head の境界誤りをこれで検証・訂正できる（v410/v425 に続く3件目）。
  2. **「fn が virtual setter」の仮説は data refs・E8・body の3系統で同時に検証する** — v423 は `mov [reg+disp]; ret` の形だけを見て誤認した（**形ではなく登録系で判定**）。
  3. **struct copy routine の `mov [dst+disp], src` は setter census の最頻出偽陽性** — dst ベースレジスタ（ecx=stack arg 由来）で切り分ける。
