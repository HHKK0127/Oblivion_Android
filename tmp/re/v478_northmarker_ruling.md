# v478 — `DAT_00B35EB8`＝NorthMarker singleton の確定・`DAT_00B37D00` singleton 定型・donor ptr 72件の裁定（V-478-1..3 完遂・旧残点 V-406-3/V-406-4 継続第8弾）

- 採番: **v478**（旧残点群消化・probe prefix **v478_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-478-1..3（v477 残点）** — 1=旧残点 V-406-3 継続（elem class 別法転換＝条件側調査と `elem[+0x28]` float の語）／2=旧残点 V-406-4 semantic 完遂（`DAT_00B37D00` identity と donor form 種別）／3=V-477-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-478-1: `DAT_00B35EB8` singleton の identity（条件側転換の成果）** | **[確定相当]** | docs 全13件のうち **生成ブロック L57914-57930 を実読 [確定(decompiled)]**: **`DAT_00b35eb8 = (int*)FUN_009832e6(..., 0, &PTR_PTR_00b02f9c, ...)`（`__RTDynamicCast` 経由生成）→ null のとき `L57923: DAT_00b35eb8 = (int*)FUN_004b9aa0();`（v470 TESObjectSTAT ctor）[確定]** → **`L57927: virtual[*DAT_00b35eb8 + 0xd8]("NorthMarker")` ／`L57928: virtual[DAT_00b35eb8[9] + 0x18]("Marker_North.nif")` ／`L57929: FUN_004B2B80(DAT_00b35eb8)`（v470 list 接続）／`L57930: virtual[* + 0x90](0)` [確定(decompiled)]** → **`DAT_00B35EB8` ＝「`TESObjectSTAT` singleton ＝ NorthMarker（北マーカー・NIF `Marker_North.nif`）実体 [確定相当]`**（**v469 で機械見た `mov eax,[edx+0xD8]; push 0xA38154` の `0xA38154` 文言が `"NorthMarker"` と一致**）→ **walk 条件 `virtual[elem]+0x170() == DAT_00B35EB8` ＝「elem の GetForm 返り値が NorthMarker singleton と等しい elem のみ対象」[確定(構造)]** |
| **V-478-1: `elem[+0x28]` float（再特定の現状）** | **[部分確定]（条件側は固まった）＋[未確定]（elem 本体）** | 条件側（NorthMarker singleton）は確定相当だが、**elem 本体の class は「GetForm で NorthMarker を返す object」[推定] まで**（`+0x170` slot を持つ class＝v477 否定側の範囲）。**`[ebx+0x28]` float の語は elem class 確定待ち＝残点継続** |
| **V-478-2: `DAT_00B37D00`（donor の `+0xC` 比較相手）identity** | **[確定相当]（singleton 定型）＋[未確定]（正体）** | **3系統 census [確定(機械)]: (i) init `FUN_009E62B0`（L1031385）内 `0x9EDEAA: mov ecx,0xb37d00; call 0x419A70`（init）→ `push 0xA20120; call 0x981FB4`（registration）／(ii) thunk **`FUN_00A20120`（L1052376）: `mov ecx,0xb37d00; jmp 0x4055E0`**／(iii) 比較 fn `FUN_0051C090`（L211964）: `cmp edx,[0xb37d00]; jne; ret 4`（等値比較・`FUN_0051C090` は donor 比較と同型）[確定(機械)]** → **`DAT_00B37D00` ＝ init＋registration＋thunk＋比較4系統を持つ runtime singleton object [確定相当]**（v466/v471 の singleton 定型と同一）。**何の singleton か（正体）は [未確定]＝残点**、docs mentions 5件（L211974/L216783/L329513/L425636/L426544） |
| **V-478-2: donor form（`self+0x104`）の種別** | **[部分確定]** | **docs `param_1[0x41] = ...` 代入72件 [確定]**（`L122594: *(param_1+0x104) = param_2`＝ptr 引数代入／`L212716/L216648: param_1[0x41] = piVar1[0x41]`＝コピー／`L1524: param_1[0x41]='\0'`）→ **`+0x104` は type 依存の汎用 ptr field（v475 の `+0xC` と同型）[確定相当]**。**`FUN_005222D0` band 内に assignment が無く donor の入手元は docs body 未特定＝残点**（ただし donor は「`+0xC` が `DAT_00B37D00` と比較される form」（v477）で、`cmp ecx,[eax+0xc]`（machine `0x522325`）が L216783 と一致 [確定(機械)]） |
| **V-478-3: V-477-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_004CBA80` identity、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v478_1）

### §2.1 `DAT_00B35EB8` 生成ブロック（v478_1a）
```
L56800: DAT_00b35eb8 = 0;                     (reset)
L57560: DAT_00b35eb8 = (int *)0x0;
L57914: DAT_00b35eb8 = (int *)FUN_009832e6(-(uint)(cVar2 != 0) & local_b4, 0,
                                           &PTR_PTR_00b02f9c, ...);   // RTDynamicCast path
L57916: if (DAT_00b35eb8 == 0) {
L57923:   DAT_00b35eb8 = (int *)FUN_004b9aa0(); }                      // ctor (v470 TESObjectSTAT)
L57927: virtual[*DAT_00b35eb8 + 0xd8]("NorthMarker");
L57928: virtual[DAT_00b35eb8[9] + 0x18]("Marker_North.nif");
L57929: FUN_004b2b80(DAT_00b35eb8);                                   // list link (v470)
L57930: virtual[*DAT_00b35eb8 + 0x90](0);
L161460 (FUN_004CD960 walk): iVar7 = virtual[*piVar8 + 0x170](), iVar7 == DAT_00b35eb8
L163835 (FUN_004D0C20): iVar6 = FUN_004cba80(DAT_00b35eb8, 1);
```

### §2.2 `DAT_00B37D00` singleton census（v478_1b）
```
0x9EDEAA (FUN_009e62b0, L1031385): mov ecx, 0xb37d00 / call 0x419a70
                                    / push 0xa20120 / call 0x981fb4  (init+registration)
FUN_00A20120 (L1052376):            mov ecx, 0xb37d00 / jmp 0x4055e0  (thunk)
0x51C095 (FUN_0051c090, L211964):  cmp edx, [0xb37d00] / jne / ret 4  (eq-compare fn)
0x522320 (FUN_005222d0): or eax, 0xb37d00 / cmp ecx, [eax+0xc]        (mid-decode; donor+0xC cmp)
docs mentions: 5 [L211974, L216783, L329513, L425636, L426544]
```

### §2.3 donor `+0x104` assignments（v478_1c）
```
param_1[0x41] = ... : 72 assignments (type-dependent generic field)
  L1524 = '\0' / L122594 *(param_1+0x104) = param_2 (ptr arg)
  L212716 / L216648 = piVar1[0x41] (copy)
  -> no assignment in FUN_005222D0 body = donor origin not visible in docs body
```

## §3 残点 **V-479-1..3**
1. 【中】**旧残点 V-406-3 継続** — **elem class（「GetForm で NorthMarker singleton を返す object」の本体）特定**＋`[ebx+0x28]` float の語（`+0x170` slot 持有 class を NorthMarker 消費側の逆引きへ）。
2. 【低】**旧残点 V-406-4 semantic 完遂** — **`DAT_00B37D00` の正体**（5件 docs usage の context から）＋donor form（`self+0x104`）の入手元 → `+0x64` 中身の語。
3. 【低】**V-478-3 引継ぎ** — `FUN_004CBA80` identity、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v478_1 の1本 EXIT=0**（初回に構文エラー（余分な閉じ括弧）→ 修正再実行）。
- **決定打の作り**: (i) **条件側転換方針（v477 残点）どおり `DAT_00b35eb8` docs 全13件を列挙** → 生成ブロック（L57914-57930）が singleton の用途を全部語る（**`"NorthMarker"`／`"Marker_North.nif"` の実文字列が identity を一撃**）→ (ii) `DAT_00B37D00` は **v466/v471 の singleton 定型（init `call 0x419A70`＋registration `call 0x981FB4`＋thunk `mov ecx,addr; jmp 0x4055E0`）を3系統 census**で即特定 → (iii) donor は assignment 72件で type 依存性を再確認し、band 内 assignment 無しの否定側を記録。
- 妥当性チェック: ①`FUN_004b9aa0()`⇔v470 TESObjectSTAT ctor— ②`[+0xD8]("NorthMarker")`⇔v469 machine `push 0xA38154`（同文言）— ③`FUN_004B2B80`⇔v470 list 接続 fn— ④`cmp ecx,[eax+0xc]`⇔L216783 donor `+0xC` 比較— ⑤3系統 census⇔v466 `0xB362E4`/v471 `0xB364E8` 定型と同一 — **五方向矛盾なし**。
- docs との不整合: **なし**（L57927/L57928 の実文字列が machine 文言と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **条件側（比較対象の singleton）へ転換すると identity が実文字列で当たる** — `DAT_00b35eb8` は生成ブロックの `"NorthMarker"`/`"Marker_North.nif"` で identity 完結（**walk の比較相手 singleton の identity は「その singleton を初期化する fn」を読む**）。
  2. **singleton 定型（init `call 0x419A70`＋registration `call 0x981FB4`＋thunk `mov ecx,addr;jmp 0x4055E0`＋比較 fn `cmp [addr]; ret 4`）の4系統 census が新しい singleton を即特定する**（v466/v471/v478 で3度目の再現）。
  3. **docs mentions 全件列挙（13件）は「生成/使用/比較」の3種に分かれる** — 生成ブロック（`DAT_ = FUN_...`）が identity の宝庫。
  4. **汎用 ptr field（72件 assignment）は band 内 assignment が無くても type 依存性が確定する**（否定側として記録）。
  5. **probe の構文エラー（余分な括弧）は即修正で再実行**（`v477_1` の ValueError に続く2件目の probe 自己修正）。
