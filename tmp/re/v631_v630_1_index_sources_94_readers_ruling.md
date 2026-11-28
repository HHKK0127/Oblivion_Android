# v631 — V-630-1 確定版: 6 store の index 源・counter read/inc 順・`+0x94` 読み手・pane 別クラス内訳

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u96_1 / u97_1**（EXIT=0）。
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. 予測の検証（査読 §3）— バイト予測は外れ、仮説は当たり

- 予測: `0x5C9588..0x5C958A` = `83 C2 01`（`add edx,1`）。
- **実バイト @`0x5C9588` = `00 00 00`** → **外れ**（`0x5C9584` の `mov` は 7 バイト `89 B4 87 94 00 00 00` で、
  差 7 は displacement の末尾）。
- ただし**仮説（+2 = 2 スロット）は当たり**:
  ```
  0x5C957B cmp [esp+0x44], -1
  0x5C9580 mov eax, [esp+0x14]            ; = counter+1
  0x5C9584 mov [edi + eax*4 + 0x94], esi  ; ★ id = counter+1
  0x5C958B mov [edi + ebp*4 + 0x94], esi  ; ★ id = counter+2（ebp）
  ```
  → **slider の +2 = 2 スロット（id と id+1）への 2 store**で確定。`add edx,1` は `0x5C954C` に実在（別位置）。

## 2. 6 store の index 源（査読 §4-1）[確定(命令)]

| store | 関数 | index レジスタ | 直前の定義 |
|---|---|---|---|
| `0x5C4444` | `0x5C4340`(pane) | `edx` | **`[esp+0x40]` = arg4** |
| `0x5C45EE` | `0x5C4480`(button) | `edx` | `[esp+0x18]`（ローカル） |
| `0x5C47C9` | `0x5C4630`(toggle) | `ecx`/`eax` | `[esp+0x1c]` / `[esp+0x14]` |
| `0x5C48E1` | `0x5C4800`(text) | — | （counter 由来ローカル） |
| `0x5C9584` | `0x5C93F0`(slider) | `eax` | `[esp+0x14]` = **counter+1** |
| `0x5C958B` | `0x5C93F0`(slider) | `ebp` | **counter+2** |

- **pane だけ arg4 由来**、**item builder 4 つは counter 由来**（自 id）。

## 3. counter の read/inc/write 順（査読 §4-2）[確定(命令)]

5 関数すべて **read → inc → write**:
```
0x5C4340: read 0x5C438A -> add 1 0x5C438F -> write 0x5C4392   (+1)
0x5C4480: read 0x5C44A9 -> add 1 0x5C44B5 -> write 0x5C44C6   (+1)
0x5C4630: read 0x5C4659 -> add 1 0x5C465E -> write 0x5C4664   (+1)
0x5C4800: read 0x5C4829 -> add 1 0x5C482E -> write 0x5C4836   (+1)
0x5C93F0: read 0x5C9419 -> add 1 0x5C941E -> add 1 0x5C9425 -> write 0x5C942D   (+2)
```
- **inc 後 read（= 消費値が id）**なので **pane 先頭は 201**（v629/v630 の表は ±0 で正しい）。

## 4. `[reg+0xE8]` writer と `-1`（査読 §4-4）[部分確定]

- `[reg+0xE8]` への store は 13 箇所、**即値は `0` のみ**（`0x5DDC7E mov [ecx+0xe8],0`）。他は全てレジスタ store。
- **`or reg,0xffffffff` / `not reg` は多数実在**（`0x40123B` 他 25+ 件）→ **`-1` は即値以外でも生成可能**。
  → **「`-1` は無い」ではなく「即値 store としては無い＝未確定」**が正しい（査読 §2）。
- **arg4 = 0 の可能性は残る**: その場合 `+0x94[0]` が 21 回上書きされる（「配列は永遠に書かれない」は成り立たない）。

## 5. `+0x94` の読み手（査読 §4-5）[確定(命令)]

- 全 .text の `[reg+idx*4+0x94]` load は 455 件、うち `0x5C0000..0x5D0000` は **15 件**
  （`0x5C044E`/`0x5C3BF7`/`0x5C4EA7`/`0x5C64E0`/`0x5C64F8`/`0x5C655C`/`0x5C6638`/`0x5C67B9`/`0x5C698B`/`0x5C69A3`/`0x5C6C59`/`0x5C6C8A`/`0x5C6CA2`/`0x5C6D57`/`0x5C6D6F`）。
- **`0x5CC200`(HandleClick)〜`0x5CE000`(Update) 帯の load は 0 件**。
- → **HandleClick / Update は `+0x94` を読まない**。読み手は `0x5C64E0..0x5C6D6F`（RaceSexMenu の pane/scroll ロジック）。
- → 査読 §4-6 適用: **id→pane 解決は HandleClick/Update では `0xB13970`（`g_TileMenuArray`）/GetTileMenu/by-name 経由に一本化**。
  **V-625-3 はその形に書き直す**。

> **撤回（v632）**: 上記の「一本化」は **[部分確定] に格下げ**。読み手は `0x5C0440`/`0x5C3BE0`（**vtable slot 8**）/
> `0x5C4E40`/`0x5C6390`/`0x5C6860`/`0x5C6AF0` に実在し、`+0x94` は **vtable 経由で到達可能**。
> HandleClick/Update が直接読まないのは事実だが、到達不能ではない。→ v632 §3。

## 6. pane 別クラス内訳（査読 §5）[部分確定]

クラス写像（関数→種別）: slider=`0x5C93F0` / button=`0x5C4480` / toggle=`0x5C4630` / text=`0x5C4800`（推定）。

> 訂正(v634): 以下の pane id 表は全 pane が +1 ずれ（Main は counter 非消費で 0）。
> 正値は docs/chargen_ui_spec.md §2 と tmp/re/v634_pane_names_global_ruling.md。

| pane | id | slider | button | toggle | text |
|---|---|---|---|---|---|
| Main | 201 | 0 | 0 | 0 | 0 |
| Race | 210 | 2 | 3 | 1 | 0 |
| Hair | 214 | 0 | 0 | 2 | 1 |
| Face | 225 | 4 | 0 | 2 | 0 |
| Shape | 230 | 0 | 4 | 0 | 0 |
| Tone | 240 | 0 | 9 | 0 | 0 |
| GeneralFace | 247 | 0 | 6 | 0 | 0 |
| Brow | 258 | 5 | 0 | 0 | 0 |
| Cheeks | 265 | 3 | 0 | 0 | 0 |
| Chin | 276 | 5 | 0 | 0 | 0 |
| Eyes | 291 | 7 | 0 | 0 | 0 |
| Forehead | 300 | 4 | 0 | 0 | 0 |
| Jaw | 307 | 3 | 0 | 0 | 0 |
| Mouth | 316 | 4 | 0 | 0 | 0 |
| Nose | 337 | 10 | 0 | 0 | 0 |
| GeneralSkin | 366 | 14 | 0 | 0 | 0 |
| Beard | 377 | 5 | 0 | 0 | 0 |
| EyesTex | 390 | 6 | 0 | 0 | 0 |
| Eyebrows | 403 | 6 | 0 | 0 | 0 |
| MouthLips | 418 | 7 | 0 | 0 | 0 |
| NoseTex | 425 | 3 | 0 | 0 | 0 |

- 最終 pane 以降: slider 2（final counter 429）。
- 差分検算（例 Race: 210-201 = 9 = 1 + 2×2 + 3 + 1）✓。
- **実装単位列への反映は保留**: リポジトリの UI 実装は `app/src/main/cpp/ui/*.cpp`（`race_sex_panel` は**未作成**）。
  → 実装単位が存在しないため `!` のまま（**V-631-1**）。

## 7. 残点

- **V-630-1【中】**: arg4 の値（`-1` / `0` / 他）と pane の `+0x94` store の意味（更新）。
- **V-631-1【低】**: 実装単位列の充填（`race_sex_panel` 作成後）。
- **V-629-1【低】**: `player[0xE8] == -1` の実行時裏取り。
- （継続）V-626-2 / V-626-3 / row604。

---

## 8. 検証

- probe **u96_1**（6 store の index 源／5 関数の counter read-inc 順／`[reg+0xE8]` writer／`or,0xffffffff`／`+0x94` load 全掃引）。
- probe **u97_1**（pane 別クラス内訳）。
- 生バイト `0x5C9570..0x5C9595`（予測検証）。
