# v479 — list owning class＝`TESObjectCELL` の確定・`DAT_00B37D00` trigger・`FUN_004CBA80` signature の裁定（V-479-1..3 完遂・旧残点 V-406-3/V-406-4 継続第9弾）

- 採番: **v479**（旧残点群消化・probe prefix **v479_1..v479_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-479-1..3（v478 残点）** — 1=旧残点 V-406-3 継続（elem class 本体＝NorthMarker 消費側逆引きと `elem[+0x28]` float の語）／2=旧残点 V-406-4 semantic 完遂（`DAT_00B37D00` 正体と donor form 入手元）／3=V-478-3 引継ぎ（`FUN_004CBA80` identity）を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-479-1: list（`param_1+0x48`）の owning class** | **[確定]（vtable 逆引き）** | **`FUN_004CD960`（walk fn）の vtable 登録を `.rdata` dword search で特定 [確定(機械)]**: raw hit 1件 `0xA46A40` → **vtable base `0xA469D4`、slot `+0x6C`、RTTI class=`.?AVTESObjectCELL@@` [確定]** → **`FUN_004CD960` ＝ `TESObjectCELL` の vtable method（slot `+0x6C`）[確定] → walk の self（`esi`）＝`TESObjectCELL` → list（`+0x48`）・flag（`+0x24`）・donor ptr（`+0x104`）は全て `TESObjectCELL` の field [確定相当]** |
| **V-479-1: elem と `+0x170` の接続（v477/v478 整合）** | **[確定]（構造）＋[推定高]（elem 系統）** | **`[TESObjectCELL vt=0xA469D4 + 0x170] = 0x00000000`（NULL data）[確定(実読)] → walk の `mov eax,[edx+0x170]` の `edx=[ebx]`＝**elem 自身の vtable**（self でない）[確定(構造)] → **elem は `+0x6C`（＝`FUN_004CD960` と同 slot）と `+0x170`（GetForm）を持つ object → elem は `TESObjectCELL` 系／Cell reference 系 object [推定高]**（`+0x170` が NorthMarker singleton を返す方（v478）＝elem の GetForm）→ **`[ebx+0x28]` float の語は elem class 完全特定待ち＝残点継続** |
| **V-479-2: `DAT_00B37D00` の trigger 構造（正体の手掛かり）** | **[確定相当]（比較構造）＋[未確定]（正体）** | docs 4件 context 実読 [確定(decompiled)]: **L211974（`FUN_0051C090`）`if (*(param_1+0xc) == DAT_00b37d00) return;`**／**L329513（`FUN_005C28F0`）`iVar3=FUN_005e3320(); if (DAT_00b37d00 == *(iVar3+0xc)) { FUN_005222d0(1); }`**／**L425636（`FUN_00662750`）`iVar1=FUN_005e3320(); if (DAT_00b37d00 != *(iVar1+0xc)) goto LAB_00662a61;`**／**L426544（`FUN_00663C50`）`iVar5=FUN_005e3320(); if (*(iVar5+0xc) != DAT_00b37d00) …`** → **3件が `FUN_005E3320()` 返り object の `+0xc`（ptr）と `DAT_00B37D00` を等値比較 [確定]** → **`object[+0xc] == DAT_00B37D00` が `FUN_005222D0(1)`（char=1＝class 適用 trigger・v462 の `param_2==0` の逆分支）を起動する [確定相当]**（L329514）→ **`DAT_00B37D00`＝「`object+0xc` field が指す特定 object の singleton（識別子）」[推定相当]**、donor（L216783 `*(*(param_1+0x104)+0xc)==DAT_00b37d00`）と同一比較＝**donor form の `+0xc` が `DAT_00B37D00` singleton を指す class であることが class 適用の条件 [推定高]**。**正体（何の singleton か）は [未確定]＝残点** |
| **V-479-3: `FUN_004CBA80`（NorthMarker 消費 fn）signature** | **[確定]（signature）＋[部分確定]（機能）** | L159742: **`int * __thiscall FUN_004cba80(int param_1, int param_2, char param_3)`（3引数）[確定(decompiled)]**: `if (param_2 == 0) return 0;` → `FUN_00496ea0(param_1)` → **`piVar1 = (param_1 + 0x48)`（＋`+0x48`＝byte 0x48 list head アドレス＝v479-1 の list field と同 offset）[確定(構造)]** → **`FUN_004CBA80(NorthMarker_singleton, 1)`（v478 L163835）は「NorthMarker を `+0x48` list 系に登録/処理する3引数 fn」[推定高]**（`TESObjectCELL+0x48` list への insert/refresh 経路 [推定]）。`FUN_004CD960` の E8 callers＝0件・docs direct call も0件 → **`FUN_004CD960` は vtable 登録（virtual method）経由のみ [確定(否定側・vtable 逆引きで裏付け)]** |
| **V-479-3: V-478-3 引継ぎ（一語名別法・predicate 英名）** | **[未確定]（残点継続）** | 本 probe 未着手 |

## §2 証拠（v479_1/v479_2）

### §2.1 walk fn の vtable 登録（v479_2a）
```
vtables containing FUN_004cd960 (0x4CD960):
  raw hits: 1 -> 0xA46A40
  ref@0xA46A40 -> vt=0xA469D4 slot=+0x6C class=.?AVTESObjectCELL@@   [確定]
  [TESObjectCELL vt+0x6C]   = 0x4CD960 (FUN_004cd960)   (walk)
  [TESObjectCELL vt+0x170]  = 0x00000000 (NULL data)     [否定側: self has no +0x170]
vtables containing FUN_004cba80 (0x4CBA80): raw hits: 0  (direct-call fn)
```

### §2.2 DAT_00B37D00 trigger（v479_1b）
```
L211974 (FUN_0051c090, sig void __thiscall(int,int)):
  if (*(param_1 + 0xc) == DAT_00b37d00) return;
L329513 (FUN_005c28f0, void(void)):
  L329511 virtual[*DAT_00B333C4 + 0x170]();      // Player singleton vtable+0x170 call!
  L329512 iVar3 = FUN_005e3320();
  L329513 if (DAT_00b37d00 == *(int *)(iVar3 + 0xc)) { L329514 FUN_005222d0(1); }
L425636 (FUN_00662750): iVar1=FUN_005e3320(); if (DAT_00b37d00 != *(iVar1+0xc)) goto LAB_00662a61;
L426544 (FUN_00663c50): iVar5=FUN_005e3320(); if (*(iVar5+0xc) != DAT_00b37d00) { ... }
  -> 3/4 usages compare FUN_005E3320()'s +0xc against the singleton;
     equality launches FUN_005222D0(1)
```

### §2.3 FUN_004CBA80（v479_1c）
```
L159742: int * __thiscall FUN_004cba80(int param_1, int param_2, char param_3) {
L159749:   if (param_2 == 0) return 0;
L159752:   FUN_00496ea0(param_1);
L159753:   piVar1 = (param_1 + 0x48);   // == the list-head offset (byte 0x48)
```

## §3 残点 **V-480-1..3**
1. 【中】**旧残点 V-406-3 継続** — **elem class の完全特定**（Cell reference 系の vtable 逆引き＝`+0x6C` が `FUN_004CD960` ではなく別 impl かつ `+0x170` が NorthMarker を返す class）＋`[ebx+0x28]` float の語。
2. 【低】**旧残点 V-406-4 semantic 完遂** — **`DAT_00B37D00` 正体**（`FUN_005E3320()` 返り object の `+0xc` field が何か＝v406「`[base+0x104]=class`」との対応）と donor（`TESObjectCELL+0x104`）の入手元 → `+0x64` 中身の語。
3. 【低】**V-479-3 引継ぎ** — `FUN_004CBA80` の `param_3`（char flag）の語、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v479_1..v479_2 の2本全 EXIT=0**（V-479-1 list owning class＝`TESObjectCELL` を vtable 逆引きで確定／V-479-2 `DAT_00B37D00` trigger 構造を確定相当・正体は残点／V-479-3 signature 確定・機能は推定高）。
- **決定打の作り**: (i) **walk fn の E8 callers＝0件 → vtable 登録を疑い `.rdata` dword search（v459 法）→ raw hit 1件で `TESObjectCELL slot+0x6C` 即決**（**E8 0件は「virtual method」の合図**）→ (ii) **`[vt+0x170]=0`（self 側 NULL）で walk の `+0x170` が elem 側と切り分け** → (iii) `DAT_00B37D00` は docs 4件 context を並べると「3件が `FUN_005E3320()+0xc` 比較」の同型が見え、`FUN_005222d0(1)` trigger と連結。
- 妥当性チェック: ①`slot+0x6C`＝`FUN_004CD960`⇔walk の self＝esi＝`TESObjectCELL` — ②`[vt+0x170]=0`⇔walk の edx=`[ebx]`（elem）と整合 — ③`FUN_004CBA80` の `param_1+0x48`⇔walk の `lea edi,[esi+0x48]`（同 list）— ④`FUN_005222d0(1)`⇔v462 `param_2==0` の逆分支（char 引数）— ⑤`DAT_00B37D00` docs 5件⇔v478 census と件数一致 — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_004CD960` に annotation 実在・vtable 登録と docs 一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **E8 callers＝0件は「vtable 登録 fn」の合図** — direct call が無ければ `.rdata` の fn ptr dword search（v459 法）へ即切り替える（本裁定で raw hit 1件）。
  2. **`[vt+slot]` が NULL（0）であることも情報** — self 側に slot が無い＝**その slot の call は相手（elem）側 vtable**と切り分けられる（walk の `+0x170` 判定）。
  3. **同型比較の docs context を4件並べると trigger 構造が見える**（3件が `FUN_005E3320()+0xc` 比較＋1件が `FUN_005222d0(1)` 呼び出し＝**条件→起動のペア**）。
  4. **`+0x48` list offset が walk（`lea edi,[esi+0x48]`）と insert（`param_1+0x48`）で一致**＝field map の横断確認（同一 list と同時確定）。
  5. **class 適用 trigger の char 引数（`FUN_005222d0(1)`）は v462 の `param_2==0` 分岐の対向分支** — 同一 fn の mode 選択と分かる。
