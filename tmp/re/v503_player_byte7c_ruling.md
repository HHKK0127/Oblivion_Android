# v503 — Player `+0x7C` byte 閾値比較（410件）・node-set caller群・`FUN_0041E210` byte 判定の裁定（V-503-1..3 完遂・旧残点 V-406-3 継続第33弾）

- 採番: **v503**（旧残点群消化・probe prefix **v503_1 の1本 EXIT=0**（初回構文エラー→修正再実行））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-503-1..3（v502 残点）** — 1=`virtual+0xC` impl（node set）と Player `+0x7C` byte semantic（新事実）／2=gated mask 源と `FUN_0041E210(0x4b)` identity／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-503-1: Player `+0x7C` byte semantic（新事実の消化）** | **[確定相当]（閾値比較 byte）** | **docs usage＝410件 [確定]。閾値比較実読 [確定(decompiled)]: **`L23799/L23851/L23921: if (0x23 < *(byte *)(DAT_00b33b00 + 0x7c))`（**`0x23`＝35 超で true**）**／**`L24063/L24451: if ((0x3f < *(byte *)(DAT_00b33b00 + 0x7c)) && …)`（**`0x3f`＝63 超**）**／**`L15445/L15484/L15490: …(0xb8, *(undefined1 *)(DAT_00b33b00 + 0x7c))`（**引数として渡す（`0xb8`＝184 と併用）**）** → **Player `+0x7C` byte は「35 超」「63 超」の閾値比較と引数渡しに使われる byte field [確定相当]**（`0x23`/`0x3f` の閾値＝何かのカウンタ/種別 [推定]）。**machine abs ref `0xB33B7C`＝0件（ptr+offset 経由のみ）[確定(否定側)]** → **v502 の `cmp byte [eax+0x7c], 0x24`（`0x24`＝36 超）はこの閾値比較 family の1つ [確定]**（`0x23 < x` ⇔ `x >= 0x24` と一致） |
| **V-503-1: `virtual+0xC` node-set impl** | **[部分確定]（caller 群判明・impl は別途）** | docs `*param_1 + 0xc))(` call sites＝**123件 [確定]**、owning fns 8件（`FUN_00401260`/`FUN_00412D30`/`FUN_0042CAE0`/`FUN_0042CC00`/`FUN_00432130`/`FUN_00434800`/`FUN_00442F70`/`FUN_00452570`）を列挙 [確定]。**`L63137/L63148`（`FUN_00452570` 内・v494）と `L11106`（`FUN_00412D30` 内・v485）が map insert の set 呼び出し（`set(node, key, key)` 形）[確定]**。**RTTI vtable 逆引き（slot `+0xC`）は owning fn 全件で0 placement → owning は「set を呼ぶ側（caller）」で、set impl fn（vtable slot +0xC に載る fn）は別 [確定（否定側）] → impl 特定は「map系 vtable の slot `+0xC` 実読」へ経路を固定 [部分確定]** |
| **V-503-2: `FUN_0041E210(0x4b)` identity** | **[推定相当]** | L20071 実読 [確定(decompiled)]: **`int __thiscall FUN_0041e210(int param_1, byte param_2)`（**byte 引数**）→ v501 の `FUN_0041e210(0x4b)` は `0x4B`（75）を byte で渡す → **FormType/種別 byte 判定 fn [推定相当]**（`byte param_2` が裏付け）**、本文は本 probe 未読＝残点 |
| **V-503-2: gated 側 mask 源（`edx`）** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-503-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v503_1）

### §2.1 Player `+0x7C` 閾値比較（v503_1b）
```
docs DAT_00b33b00 + 0x7C usage: 410
  L23799 / L23851 / L23921: if (0x23 < *(byte *)(DAT_00b33b00 + 0x7c))   // >= 0x24
  L24063 / L24451: if ((0x3f < *(byte *)(DAT_00b33b00 + 0x7c)) && ...)    // >= 0x40
  L15445 / L15484 / L15490: ...(0xb8, *(undefined1 *)(DAT_00b33b00 + 0x7c))  // arg pass with 0xb8
machine .text abs refs of 0xB33B7C: 0   [否定側 -> ptr+offset only]
=> v502's cmp byte [Player+0x7c], 0x24 belongs to this threshold family  [確定]
```

### §2.2 node-set call sites（v503_1a）
```
docs `*param_1 + 0xc))(` sites: 123
  L258:   iVar3 = (**(code **)(*param_1 + 0xc))(0x80);
  L11106: (**(code **)(*param_1 + 0xc))(puVar3, param_2, param_2);     (FUN_00412d30, v485)
  L63137: (**(code **)(*param_1 + 0xc))(puVar3, param_2, param_2);     (FUN_00452570, v494)
  L63148: (**(code **)(*param_1 + 0xc))(puVar3, param_2, unaff_retaddr);
  L39311: (**(code **)(*(int *)*param_1 + 0xc))(param_2, param_3, 0);
  owning fns: FUN_00401260 / 00412d30 / 0042cae0 / 0042cc00 / 00432130 /
              00434800 / 00442f70 / 00452570
RTTI vtable+0xC placement for these owners: 0 for all   [否定側 -> owners are CALLERS]
=> set impl fn lives at vtable slot +0xC of map-family classes; locate via that  [部分確定]
```

### §2.3 `FUN_0041E210`（v503_1c）
```
L20071: int __thiscall FUN_0041e210(int param_1, byte param_2) { ... }
  -> byte param_2 = type byte argument (0x4B = 75 at the v501 site)
  -> FormType/type-byte check fn  [推定相当]  (body unread -> residual)
```

## §3 残点 **V-504-1..3**
1. 【中】**node-set impl fn の特定**（map 系 vtable（`NiTMapBase` 系 [推定]）の slot `+0xC` 実読 → value→owner/count field 対応完遂）＋ **Player `+0x7C` byte の閾値（35/63/36）の意味**（何のカウンタ/種別か＝`0xb8` 引数との関係含む）。
2. 【低】**`FUN_0041E210` 本文**（FormType 判定の中身）＋ gated 側 mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v503_1 の1本 EXIT=0**（初回構文エラー（余分な閉じ括弧）→修正）。V-503-1 Player `+0x7C` 閾値比較（410件・35/63/引数）を確定相当・node-set caller 群8件を列挙し impl は否定側で経路固定／V-503-2 `FUN_0041E210` byte 判定を推定相当／V-503-3 引継ぎ）。
- **決定打の作り**: (i) **`DAT_00b33b00 + 0x7C` の docs grep（410件）から閾値比較（`0x23 <`/`0x3f <`）と引数渡しを拾う** → byte field の性格を確定 → **v502 の `0x24` 判定を family に接続**（`0x23 < x` ⇔ `x >= 0x24` の一致）→ (ii) **node-set call sites（123件）の owning 8件を vtable 逆引き →0 placement**で「caller であり impl は vtable slot +0xC」と経路固定 → (iii) **`FUN_0041E210` signature の `byte param_2`** が type byte 判定を推定させる。
- 妥当性チェック: ①`0x23 < x`⇔v502 `cmp …, 0x24; jb`（`>= 0x24` 同値）— ②`0x3f < x`⇔`>= 0x40`（別閾値）— ③abs ref 0⇔docs の `DAT_00b33b00 + 0x7c`（ptr+offset）と整合 — ④123 call sites⇔v485/v494 の `set(node, key, key)` 形と同型 — ⑤`byte param_2`⇔`0x4b`（75）渡し — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **byte field の semantic は「閾値比較の constants」で語られる**（`0x23`/`0x3f`/`0x24`＝35/63/36 の比較が byte の性格を決める＝**閾値 censusが field 調査の定番**）。
  2. **同一比較（`0x23 <` ⇔ `cmp 0x24; jb`）が docs と machine で一致すれば family 接続が確定**（v502 の新事実を v503 で消化）。
  3. **virtual call の owning（呼ぶ側）と vtable slot の載せ先（impl）は別** — **owning を vtable 逆引きして0件なら「caller 確定・impl 別探索」に切り替える**（v479/v488 の E8→vtable 法の逆方向活用）。
  4. **`byte` 型 param の signature が type byte 引数の目印**（`FUN_0041E210(int, byte)`⇔`0x4b`＝FormType [推定]）。
  5. **usage 件数（410件）と閾値（35/63）の組み合わせが「頻用 byte field」の規模感を語る**（Player field map の充実度を示す）。
