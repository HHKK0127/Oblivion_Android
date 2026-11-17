# v471 — 3 singleton registration・dword setter callers・`+0x48` sentinel の裁定（V-471-1..3 完遂・旧残点 V-406-3/V-406-4 継続）

- 採番: **v471**（旧残点群消化・probe prefix **v471_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-471-1..3（v470 残点）** — 1=旧残点 V-406-3 継続（`elem[10]` の語＋`0xB364E8/F0/F8` 指し先 identity）／2=旧残点 V-406-4 残り（`form+0x64`/`+0x69` semantic）／3=V-470-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-471-1: `0xB364E8/F0/F8`（3 ptr 指し先）の identity** | **[確定相当]** | **3つとも同型 pattern [確定(機械)]**: `0x9E4189/0x9E41A9/0x9E41C9` 帯（`FUN_009E4120` L1029823 内）で **`call 0x419A70`（init 系）→ `push 0xA1C4A0/B0/C0; call 0x981FB4`（registration）**／各 thunk **`FUN_00A1C4A0/B0/C0`（L1044138/1044149/1044160）: `mov ecx,0xb364e8/f0/f8; jmp 0x4055E0`（thiscall wrapper）** [確定(機械)]。**docs mentions 0件（`DAT_00b364e8/f0/f8` は docs に無い）→ docs 未収録 global [確定(否定側)]** → **`0xB364E8/F0/F8`＝`FUN_009E4120` が registration する3つの runtime singleton object（spec 名前テーブル [推定]）[確定相当]**、各 thunk が thiscall accessor [確定相当] |
| **V-471-2: `form+0x64`（`FUN_00468230` dword setter）の callers** | **[確定]（6件列挙）＋[未確定]（semantic）** | **E8 census＝6件 [確定]**: **`FUN_005222D0`×2**（v462 derived AV 初期適用・class 適用）／**`FUN_005F5170`（L359901）**／**`FUN_005F8170`（L361776）**／**`FUN_005F94B0`（L362550）**／**`FUN_005F9EB0`（L362958）: `push 1; mov ecx,esi; call 0x566830`（value=1 → `FUN_00566830`）** → **`form+0x64` の dword は `FUN_005222D0`（class 適用）と `FUN_005Fxxxx` 帯5件から actor `self+8` へ転送される [確定]**。**何の値か（semantic）は [未確定]＝残点**（`FUN_00566830` の identity が次の手） |
| **V-471-1: `+0x48` field の docs 系統** | **[確定]（sentinel 初期化多数）＋[部分確定]（意味）** | **docs mentions 837件 [確定]**: `param_1[0x12]=0`（L319）／**`*(param_1+0x48)=0x7fffffff`（L48386/L52072/L52112）＝INT_MAX sentinel 初期化 [確定]**／`param_1[0x12]=0x7fffffff`（L50913）／`if (*(param_1+0x48)==0x7fffffff)`（L53532）→ `*(param_1+0x48)=*(param_1+0x20)`（L53533）→ **`+0x48`（＝`param_1[0x12]`）は `0x7fffffff` を初期値/上限 sentinel とする汎用 field [確定相当]**（`FUN_004CD960` の list head 用途（v470）と同一 field かの整合は [部分確定]） |
| **V-471-3: V-470-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v471_1）

### §2.1 3 singleton（v471_1a）
```
0xB364E8: 0x9E4189 push imm (FUN_009E4120, L1029823) / call 0x419a70 /
          push 0xa1c4a0 / call 0x981fb4 (registration)
          thunk FUN_00A1C4A0 (L1044138): mov ecx, 0xb364e8 / jmp 0x4055e0
0xB364F0: same pattern -> thunk FUN_00A1C4B0 (L1044149)
0xB364F8: same pattern -> thunk FUN_00A1C4C0 (L1044160)
docs mentions of DAT_00b364e8/f0/f8: 0 each   [否定側 = docs 未収録]
```

### §2.2 `FUN_00468230` callers（v471_1c）
```
E8 census: 6
  0x52271B / 0x522753  owning=FUN_005222d0 (v462 derived-AV apply)
  0x5F526F  owning=FUN_005f5170 (L359901)
  0x5F823C  owning=FUN_005f8170 (L361776)
  0x5F95D1  owning=FUN_005f94b0 (L362550)
  0x5F9FF8  owning=FUN_005f9eb0 (L362958): push 1 / mov ecx,esi / call 0x566830
```

### §2.3 `+0x48` sentinel（v471_1b）
```
docs mentions: 837
  L319   param_1[0x12] = 0;
  L48386 *(param_1 + 0x48) = 0x7fffffff;   (INT_MAX sentinel)
  L50913 param_1[0x12] = 0x7fffffff;
  L53532 if (*(param_1 + 0x48) == 0x7fffffff) ... -> L53533 = *(param_1 + 0x20)
```

## §3 残点 **V-472-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`elem[10]`（byte +40 float）の語**（`TESObjectSTAT` singleton と比較される elem の何の property か）＋ `FUN_00566830`（`form+0x64` 転送先）の identity。
2. 【低】**旧残点 V-406-4 残り** — `form+0x64`（dword）と `form+0x69`（byte）の semantic（`FUN_005Fxxxx` 帯5件の文脈と `FUN_00566830` identity）。
3. 【低】**V-471-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v471_1 の1本 EXIT=0**（V-471-1 は3 singleton registration 確定相当＋docs 未収録を否定側確定／V-471-2 callers 6件列挙・semantic は残点／V-471-3 引継ぎ）。
- **決定打の作り**: (i) **3 global の refs を並列 census すると同型 pattern（init→registration＋thunk）が揃い「3つの対」だと即分かる** → (ii) **`push addr; call 0x981FB4`（registration）と `mov ecx,addr; jmp`（thunk）の2形式が singleton の定型** → (iii) **`FUN_00468230` の E8 census 6件を context decode**（`push 1`→`FUN_00566830` を発見）→ (iv) `+0x48` は **`0x7fffffff` 初期化＋`==` 比較の docs grep（837件）**で sentinel field と確定。
- 妥当性チェック: ①3 thunk⇔3 global（1対1）— ②`call 0x419A70`⇔init（`FUN_009E4120` 内）— ③`FUN_005222D0` callers⇔v406 既知 — ④`0x7fffffff`⇔INT_MAX sentinel（`==` 比較と `param_1[0x20]` への代入）— ⑤docs mentions 0⇔docs 未収録（v437 annotation 欠落と別類型＝global 未収録）— **五方向矛盾なし**。
- docs との不整合: **①`DAT_00b364e8/f0/f8` は docs に mentions 0件（docs 未収録 global）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **複数 global を並列 census すると「3つの対」の同型 pattern が見える**（init→registration＋thunk の定型が3つ同時に現れる）。
  2. **`push <addr>; call <reg_fn>`（registration）と `mov ecx,<addr>; jmp <thunk>`（accessor）が runtime singleton の2定型**（v466/v471 で再現）。
  3. **`0x7fffffff`（INT_MAX）の初期化と `==` 比較が「sentinel field」の目印**（docs grep 837件で field の性格が確定）。
  4. **docs mentions 0件の global は「docs 未収録 global」として別類型で記録**（annotation 欠落（v437）とは別物）。
  5. **setter の callers context decode で転送先 fn（`FUN_00566830`）が見つかる** — semantic 調査の入口。
