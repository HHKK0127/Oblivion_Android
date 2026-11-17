# v477 — `form+0x64` 系譜完結・TESObjectSTAT vtable 短縮と elem 否定側・`FUN_004D0C20` 消費の裁定（V-477-1..3 完遂・旧残点 V-406-3/V-406-4 継続第7弾）

- 採番: **v477**（旧残点群消化・probe prefix **v477_1 の1本 EXIT=0**（初回は `fn` 範囲外 ValueError→範囲チェック追加で再実行））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-477-1..3（v476 残点）** — 1=旧残点 V-406-3 継続（elem class 再特定＝ptr を返す `[vt+0x170]` impl 別法と `elem[+0x28]` float の語）／2=旧残点 V-406-4 semantic（`self+0x104` の form 種別特定と `0xFF` の語）／3=V-476-3 引継ぎ（`FUN_004D0C20` identity 等）を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-477-2: `form+0x64` 系譜（donor→transfer 完結）** | **[確定相当]** | **docs L216913-216914（`FUN_005222D0` 内）: `if (*(param_1+0x104) != 0) { FUN_00468230(*(undefined4*)(*(param_1+0x104) + 100)); }` [確定(decompiled)]** — **`+100` は decimal＝`0x64`（v475 の machine `mov ecx,[eax+0x64]` と完全一致）** → **`form+0x64` dword ＝「donor form（`*(self+0x104)`）の `+0x64` を `FUN_00468230` で `self+0x68` subobject の `+8`（`*(self+0x70)`）へ転送」[確定相当]（V-406-4 の form+0x64 系譜はこれで donor/transfer 完結）**。donor form の `+0x64` 中身の語は donor 種別確定待ち＝残点 |
| **V-477-2: donor form（`self+0x104`）の手掛かり** | **[部分確定]** | docs 4件 [確定(decompiled)]: **L216780 `if ((*(param_1+0xe8)!=0) && (*(param_1+0x104)!=0))`（2 ptr 併存 gate）**／**L216783 `if (bVar18 && DAT_00b37d00 == *(*(param_1+0x104)+0xc))`（donor の `+0xC` が `DAT_00B37D00` と比較）**（`bVar18` は L216781 の `*(param_1+0xc)==7` 結果＝**self `+0xC`==7 gate と連結**）／L216913/216914（上記転送）→ **donor form は「`+0xC`（int）が `DAT_00B37D00` singleton と比較される form」[確定相当]**、`DAT_00B37D00` identity と donor form 種別は [未確定]＝残点。**`self[+0xC]==7`（v475）と `FUN_004B2A00` の `100..119`（v476）が別 usage で併存＝`+0xC` の type 依存性が再確認 [確定]** |
| **V-477-1: elem class 再特定（TESObjectSTAT vtable 逆試し）** | **[確定]（否定側）＋[未確定]（残点）** | **`[TESObjectSTAT vt=0xA44DEC + 0x170]` の raw＝`0x6472616F`（ASCII `"oard"`、ctx `es\Billboard s...`）＝ **TESObjectSTAT の vtable は `0x170` 未満（slot 外・後続文字列）[確定(否定側)]** → **elem の vtable（`[ebx]`）は slot `0x170` を持つ別 class（TESObjectSTAT 本体でない）[確定(否定側)]**。walk の比較は `cmp eax,[0xB35EB8]`（＝singleton object ptr）なので **elem の `+0x170` getter 返り値が singleton STAT form ptr と等しい elem のみ対象 [確定(構造)]** → **elem class は [未確定] に維持**（v476 で撤回済みの再確認） |
| **V-477-3: `FUN_004D0C20`（A1 singleton load owner）** | **[部分確定]（消費経路）＋[未確定]（identity）** | body L163719-163963 から **L163835: `iVar6 = FUN_004CBA80(DAT_00b35eb8, 1);`（singleton STAT form を第1引数に渡す fn 呼び出し）[確定(decompiled)]** → **`FUN_004D0C20` は `DAT_00B35EB8`（singleton STAT）を引数消費する大型 fn [確定相当]**（`FUN_004CBA80` identity と本文の用途は [未確定]＝残点） |
| **V-477-3: V-476-3 引継ぎ（一語名別法・predicate 英名）** | **[未確定]（残点継続）** | 本 probe 未着手（budget は V-477-1/2 に集中） |

## §2 証拠（v477_1）

### §2.1 `form+0x64` donor→transfer（v477_1b）
```
FUN_005222D0 body (L216747..L216927): +0x104 mentions = 4
L216780: if ((*(param_1+0xe8) != 0) && (*(param_1+0x104) != 0)) {
L216783:   if ((bVar18) && (DAT_00b37d00 == *(int *)(*(int *)(param_1+0x104) + 0xc))) {
L216913: if (*(param_1 + 0x104) != 0) {
L216914:   FUN_00468230(*(undefined4 *)(*(int *)(param_1 + 0x104) + 100));
  -> +100 == 0x64 == v475 machine `mov ecx,[eax+0x64]`
  -> donor form *(self+0x104) -> transfer to self+0x68 sub (+8) = *(self+0x70)
```

### §2.2 TESObjectSTAT vtable length (v477_1a)
```
[vt=0xA44DEC + 0x170] raw = 0x6472616F  ('oard', data not .text)
ctx: 0x425C7365(es\B) 0x626C6C69(illb) 0x6472616F(oard) 0x00000073(s...)
-> TESObjectSTAT vtable shorter than 0x170 (string follows)
-> elem's vtable (with slot 0x170) is a DIFFERENT class  [否定側]
```

### §2.3 FUN_004D0C20 singleton consumption (v477_1c)
```
fn body L163719..L163963:
L163835: iVar6 = FUN_004CBA80(DAT_00b35eb8, 1);
  -> singleton STAT form passed as arg 1  [確定(decompiled)]
```

## §3 残点 **V-478-1..3**
1. 【中】**旧残点 V-406-3 継続** — **elem class の再特定**（`+0x170` slot を持つ class を「walk 命令の `mov edx,[ebx]` 直後の vtable 逆引き」ではなく **donor/条件側（`DAT_00B35EB8` singleton を GetForm で返す class）の調査**へ転換）＋ `elem[+0x28]` float の語。
2. 【低】**旧残点 V-406-4 semantic 完遂** — **`DAT_00B37D00`（donor の `+0xC` 比較相手）identity** と donor form（`self+0x104`）の種別 → `+0x64` 中身の語。
3. 【低】**V-477-3 引継ぎ** — `FUN_004CBA80` identity と `FUN_004D0C20` 本文用途、`FUN_005222D0` 一語名別法と predicate slot35 英名裏取り。

## §4 検証・自己批評
- probe: **v477_1 の1本 EXIT=0**（初回 `ValueError: VA out of sections: 0x6472616F`（ASCII `"oard"`＝vtable 外 data）→ **fn/vtable 値の `.text` 範囲チェックを追加して再実行**）。
- **決定打の作り**: (i) **`[0xA44DEC+0x170]` を読んだ結果が ASCII（vtable 外）** → 「TESObjectSTAT vtable は slot 外」＝**否定側として即価値**（elem≠TESObjectSTAT 本体）→ (ii) **docs `+100`（decimal）表記と machine `0x64` の一致**で `form+0x64` donor→transfer 完結（**docs の decimal 表記（`100`）と machine hex（`0x64`）を対応させる読み替えが決め手**）→ (iii) `FUN_004D0C20` は docs grep 1行（`FUN_004CBA80(DAT_00b35eb8,1)`）で singleton 消費を確定。
- 妥当性チェック: ①`+100`⇔`0x64`⇔v475 `mov ecx,[eax+0x64]` — ②`FUN_00468230` receiver⇔v475 `lea esi,[edi+0x68]`（sub `+8`＝`self+0x70`）— ③`*(donor+0xc)==DAT_00B37D00`⇔`+0xC` int field（v476 range gate と同 field 型）— ④vtable<`0x170`⇔ASCII 隣接（`"...es\Billboard..."` 文字列＝vtable 後の data）— ⑤`FUN_004CBA80(DAT_00b35eb8,1)`⇔singleton 引数消費 — **五方向矛盾なし**。
- docs との不整合: **なし**（L216914 の `+100` 表記は decimal、machine は `0x64`＝同一で整合）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **docs の offset は decimal 表記もあり得る**（`+100`＝`0x64`）— **machine の hex disp と対応させる**（本裁定の決定打）。
  2. **vtable 値の範囲外チェックが新たな否定側を生む** — `[vt+0x170]` が ASCII なら「vtable が短い」こと自体が class 情報（**エラー（`ValueError`）を範囲チェックに変換**）。
  3. **donor→transfer の系譜は「docs の代入/呼び出し1行」で完結する**（`FUN_00468230(*(donor+100))` の1行が v475 machine と一致）。
  4. **singleton 消費 fn は `FUN_x(DAT_..., 1)` 形の docs grep で即当たる**（引数として global を渡す呼び出し）。
  5. **`self[+0xC]==7` gate と donor の `+0xC==DAT_00B37D00` 比較が同一 fn 内で連結** — `+0xC` の type 依存性（v475）を「同一 field 名でも donor/self で別物」と再確認。
