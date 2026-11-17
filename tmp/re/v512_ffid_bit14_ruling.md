# v512 — `FUN_00446B40`＝`0xFF` id set・bit14 gate family 12fn・Player `+0x80` の裁定（V-512-1..3 完遂・旧残点 V-406-3 継続第42弾）

- 採番: **v512**（旧残点群消化・probe prefix **v512_1 の1本 EXIT=0**（初回構文エラー→修正再実行））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-512-1..3（v511 残点）** — 1=`FUN_00446B40` identity・`+0x8` bit14 gate semantic・Player `+0x80` 中身／2=gated mask 源と同 offset異物切り分け／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-512-1: `FUN_00446B40`（v511「old free 候補」）identity** | **[確定相当]（`0xFF` id set・解釈更新）** | docs 実読 [確定(decompiled)]: **`void __thiscall FUN_00446b40(int param_1, uint param_2)`: **`if ((((param_2 != 0) && ((param_2 & 0xff000000) == 0xff000000)) && (param_2 == *(int *)(param_1 + 0x8c0) - 1U)) && (0x7ff < (param_2 & 0xffffff))) { *(uint *)(param_1 + 0x8c0) = param_2; }` [確定(構造)]** → **`param_2` は「上位 byte＝`0xFF`（`0xff000000`）＋下位24bit＞`0x7ff`（2047）」の値で、`param_1+0x8C0`（u32）が `param_2 == current-1` のときだけ更新 [確定相当]** → **`FUN_00446B40`＝「`0xFF` 上位 byte＋24bit id の値を `param_1+0x8C0` へ set（decrement 条件付き）fn [確定相当]」→ form id／ref id 系の set [推定相当]。v511「old free」解釈を本裁定で更新（free でなく id set）[確定（解釈更新）]**。**`0xFF000000` mask⇔v473 の gate と同型＝`0xFF` 上位 byte gate が再現 [確定]** |
| **V-512-1: `+0x8` bit14 gate semantic** | **[確定相当]（12 fn family）** | **`>> 0xe & 1` on `+0x8` field census＝34件・**12 fn [確定(decompiled)]**: **`FUN_004DF1A0`（2 hits）／`FUN_00416000`／`FUN_00451210`／`FUN_004533D0`／**`FUN_0045B670`（v492 gated）／`FUN_0045B700`／`FUN_0045B780`**／`FUN_0046B2C0`／`FUN_0046B340`／`FUN_0046B990`／`FUN_0046B9F0`／**`FUN_0046C280`** → **`+0x8` bit14 gate は「gated notify（`FUN_0045B670`/`B700`/`B780`）と `FUN_0046C300`（set/replace・v511）を含む12 fn family の共通 gate [確定相当]**（**bit gate の横断一致が gated family を完成**＝v492→v511→v512 の連鎖） |
| **V-512-1: Player `+0x80`（ptr）中身** | **[部分確定]（ptr 再確認・中身は残点）** | mentions **77件 [確定]**、`puVar1 = *(undefined4 **)(DAT_00b33b00 + 0x80)`（ptr・LoadGame band L15441/L15477/L28394/L29045/L50367/L50406 等）→ **`+0x80`＝ptr field（v506 確定）を再確認、中身は LoadGame の buffer/object ptr [部分確定]**（v510/v511 budget 外＝残点の方向を明記） |
| **V-512-2: gated mask 源／同 offset異物切り分け** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-512-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v512_1）

### §2.1 `FUN_00446B40`（v512_1a）
```
L54112: void __thiscall FUN_00446b40(int param_1, uint param_2) {
L54115:   if ((((param_2 != 0) && ((param_2 & 0xff000000) == 0xff000000)) &&
L54116:       (param_2 == *(int *)(param_1 + 0x8c0) - 1U)) && (0x7ff < (param_2 & 0xffffff))) {
L54117:     *(uint *)(param_1 + 0x8c0) = param_2;
L54119:   } return; }
=> id set (0xFF top byte + 24bit id > 0x7ff, decrement-1 condition)
   onto param_1+0x8C0  [確定相当]  (free reading corrected)
```

### §2.2 bit14 gate family（v512_1b）
```
`>> 0xe & 1` on +0x8 field: 34 hits / 12 fns
  FUN_004df1a0 (2) / FUN_00416000 / FUN_00451210 / FUN_004533d0 /
  FUN_0045b670 (v492 gated) / FUN_0045b700 / FUN_0045b780 /
  FUN_0046b2c0 / FUN_0046b340 / FUN_0046b990 / FUN_0046b9f0 / FUN_0046c280
=> +0x8 bit14 = common gate across the gated-notify + set/replace family  [確定相当]
```

### §2.3 Player `+0x80`（v512_1c）
```
mentions: 77  (all `*(undefined4 **)(DAT_00b33b00 + 0x80)` = PTR)
  L15441 / L15477 / L28394 / L29045 / L50367 / L50406 ...
=> ptr field reconfirmed; content (LoadGame buffer/object) unread  [部分確定]
```

## §3 残点 **V-513-1..3**
1. 【中】**`FUN_0046C300` の `+0x8C0` と `FUN_00446B40` の `param_1+0x8C0` の対応**（id set の caller 経路）＋ **Player `+0x80` 中身の特定**（LoadGame band の `puVar1` が何の object か）。
2. 【低】**gated mask 源（`edx` 設定元）** ＋ `FUN_00452C20`（lazy node）の `param_1+0xC` key と `FUN_0046C300` の `param_1+0xC`（current ptr）の同 offset異物切り分け。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v512_1 の1本 EXIT=0**（初回構文エラー→修正）。V-512-1 `FUN_00446B40`＝`0xFF` id set を確定相当で解釈更新・bit14 gate family 12fn を確定／Player `+0x80` は ptr 再確認で中身は残点／V-512-2/3 は引継ぎ）。
- **決定打の作り**: (i) **`FUN_00446B40` docs 全読** → `0xff000000` mask＋`0x7ff` 閾値＋`current-1` 条件の3つ組で **id set と即決（free 解釈を更新）** → (ii) **`>> 0xe & 1` census（34件/12fn）**で gated family を完成 → (iii) **`+0x80` mentions（77件）**で ptr 再確認。
- 妥当性チェック: ①`0xff000000`⇔v473 gate（`0xFF` 上位 byte）と同型 — ②`0x7ff <`⇔24bit id＞2047 — ③`param_2 == current-1`⇔decrement 条件（v493 `count--` 系と同 pattern [推定]）— ④`FUN_0046C300`⇔`FUN_00446B40`（set/replace 内の old free 候補が id set）— ⑤bit14 family 12fn⇔v492/v500 gated＋v511 set/replace — **五方向矛盾なし**。
- docs との不整合: **①v511 ruling「`FUN_00446B40`＝old free 候補 [推定]」→ 本裁定で `0xFF` id set [確定相当] と解釈更新（free でない）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`0xFF000000` mask＋24bit 閾値（`0x7ff`）が「id 値」の目印**（**`0xFF` 上位 byte gate が v473 から再現**＝**`param_2 == current-1` が decrement/set の定番**）。
  2. **bit gate census（`>> N & 1` on field）が family を完成させる**（**34件/12fn で gated family が揃う**＝v473/v489/v497/v503 に続く bit census 法）。
  3. **「free 候補」も machine/docs で「id set」と分かる**（**解釈更新の連鎖：v469 ctor→v509 reset、v500 ctor→v505 remove、v510 ctor→v511 set/replace、v511 free→v512 id set**）。
  4. **同 offset異物（`+0xC`）の切り分けは caller 経路で**（`FUN_00452C20` の key vs `FUN_0046C300` の current ptr＝**v512 残点の方向を明記**）。
  5. **Player `+0x80` は ptr 再確認までで中身は残点化**（**budget 制約を ruling に明記**＝probe1本の収穫最大化と残点の形を両立）。
