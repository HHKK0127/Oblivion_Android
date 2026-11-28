# v632 — pane 帰属の修正（1 行ずれ）・item の arg1 源・`+0x94` 読み手の同定・counter 生バイト

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u98_1**（EXIT=0）。
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. item の arg1 源 = pane 配列（査読 §2）[確定(命令)]

4 つの item builder の最初の呼出:
```
slider 0x5CA55E: mov eax,[esi+0x40]; push eax; mov ecx,esi; call 0x5C93F0
button 0x5CA491: mov eax,[esi+0x40]; push eax; mov ecx,esi; call 0x5C4480
toggle 0x5CA53A: mov eax,[esi+0x40]; push eax; mov ecx,esi; call 0x5C4630
text   0x5CA60B: mov eax,[esi+0x44]; push eax; mov ecx,esi; call 0x5C4800
```
- **item の arg1 = `[esi+0x40+i*4]`（＝ pane 配列 `menu+0x40`、v627）** = **直前に生成した pane tile**。
- → **item は pane の子。生成順は pane 先 → item**。よって **v631 §6 の表は 1 行ずれ**（`counts[X]` は「X に入る item」）。
- **`+0x90` は builder 内で write/read される**（`0x5CBB2F` write、`0x5CBB47`/`0x5CBB70` read）。→ `+0x90` は実フィールド。

## 2. pane 帰属の修正（査読 §1）[確定(命令)]

**pane X に、その直後に生成された item を帰属**（サイト実アドレスで計数）:

> 訂正(v634): 以下の pane id 表は全 pane が +1 ずれ（Main は counter 非消費で 0）。
> 正値は docs/chargen_ui_spec.md §2 と tmp/re/v634_pane_names_global_ruling.md。

| pane | id | slider | button | toggle | text |
|---|---|---|---|---|---|
| **Main** | 201 | **2** | **3** | **1** | 0 |
| Race | 210 | 0 | 0 | 2 | 1 |
| Hair | 214 | 4 | 0 | 2 | 0 |
| Face | 225 | 0 | 4 | 0 | 0 |
| Shape | 230 | 0 | 9 | 0 | 0 |
| Tone | 240 | 0 | 6 | 0 | 0 |
| GeneralFace | 247 | 5 | 0 | 0 | 0 |
| Brow | 258 | 3 | 0 | 0 | 0 |
| Cheeks | 265 | 5 | 0 | 0 | 0 |
| Chin | 276 | 7 | 0 | 0 | 0 |
| Eyes | 291 | 4 | 0 | 0 | 0 |
| Forehead | 300 | 3 | 0 | 0 | 0 |
| Jaw | 307 | 4 | 0 | 0 | 0 |
| Mouth | 316 | 10 | 0 | 0 | 0 |
| Nose | 337 | 14 | 0 | 0 | 0 |
| GeneralSkin | 366 | 5 | 0 | 0 | 0 |
| Beard | 377 | 6 | 0 | 0 | 0 |
| EyesTex | 390 | 6 | 0 | 0 | 0 |
| Eyebrows | 403 | 7 | 0 | 0 | 0 |
| MouthLips | 418 | 3 | 0 | 0 | 0 |
| **NoseTex** | 425 | **2** | 0 | 0 | 0 |

- 差分検算（**入る側**）: Race `210-201 = 9 = 1+2×2+3+1` ✓ / Hair `4 = 1+0+0+2+1` ✓ / Face `11 = 1+2×4+0+2` ✓ /
  Shape `5 = 1+4` ✓ / Tone `10 = 1+9` ✓ / GeneralFace `7 = 1+6` ✓ / Brow `11 = 1+2×5` ✓。
- **末尾**: NoseTex の 2 slider（`429-425 = 4 = 1+2×2`）が表に入った。**表から漏れていた分を回収**。
- Main = **3 button（タブ Race/Face/Hair）+ 2 slider（Age/Complexion）+ 1 toggle（Gender）** と解釈できる。

## 3. `+0x94` の読み手（査読 §3）[部分確定] — 一本化は撤回

読み手 15 件の head:

| site | head | 備考 |
|---|---|---|
| `0x5C044E` | `0x5C0440` | |
| `0x5C3BF7` | **`0x5C3BE0`** | **vtable slot 8**（`0xA6D3AC`） |
| `0x5C4EA7` | `0x5C4E40` | counter リセット関数 |
| `0x5C64E0`/`0x5C64F8`/`0x5C655C`/`0x5C6638`/`0x5C67B9` | `0x5C6390` | |
| `0x5C698B`/`0x5C69A3` | `0x5C6860` | |
| `0x5C6C59`/`0x5C6C8A`/`0x5C6CA2`/`0x5C6D57`/`0x5C6D6F` | `0x5C6AF0` | |

- 基底レジスタは **`esi`/`ebp`/`edi` = RaceSexMenu（this）**。
- **`0x5C3BE0` は RaceSexMenu の vtable slot 8** → **`+0x94` は vtable 経由で到達可能**。
- → **v631 §5 の「`0xB13970`/by-name に一本化」は撤回**し、**[部分確定] に落とす**:
  `+0x94` は pane/項目の解決に**使われている**（少なくとも vtable slot 8 のメソッドで）。HandleClick/Update が直接読まないのは事実だが、**到達不能ではない**。

## 4. counter の生バイト（査読 §4）[確定(命令)]

5 関数すべて同一形:
```
a1 d4 b5 b3 00   mov  eax, [0xB3B5D4]      ; read
83 c0 01         add  eax, 1               ; inc
a3 d4 b5 b3 00   mov  [0xB3B5D4], eax      ; write-back
```
- 直後に **eax を再ロードせず**ローカルへ store（`0x5C4340`: `89 44 24 40` / `0x5C439B` `89 44 24 14`、
  `0x5C4630`: `89 44 24 18`、`0x5C4800`: `8b e8`＝`mov ebp,eax`、`0x5C93F0`: `89 44 24 14`）。
- → **消費される値は加算後** → **id = counter+1**（pane 先頭 201）。語順は「**read → inc → write-back → 加算後値を消費**」。
- `0x5C93F0` のみ inc を 2 回（`83 c0 01` ×2）→ **counter+1 と counter+2 の 2 スロット**（v631 §1）。

## 5. 残点

- **V-630-1【中】**: arg4 の値（`-1`/`0`/他）。
- **V-632-1【中】**: `+0x94` を読む 5 関数（`0x5C0440`/`0x5C3BE0`/`0x5C4E40`/`0x5C6390`/`0x5C6860`/`0x5C6AF0`）の役割と、`0x5CC200`/`0x5CDEF0` からの到達性。
- **V-631-1【低】**: 実装単位列の充填（`race_sex_panel` 未作成）。
- （継続）V-629-1 / V-626-2 / V-626-3 / row604。

---

## 6. 検証

- probe **u98_1**（item の arg1 源／`+0x90` read／pane 帰属の再計算／`+0x94` 読み手 15 件の head／counter 生バイト）。
