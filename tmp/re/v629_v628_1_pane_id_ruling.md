# v629 — V-628-1 決着: `0x5C4340` 本体読み（a）と counter simulation による pane id 確定（b）

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u91_1 / u92_1 / u93_1**（EXIT=0）。
判定: `[確定(命令)]` / `[部分確定]`

> **訂正（v630）**: §2 の「item +2」は一般化の誤り。正しくは **増分は generator 関数の性質**
> （`0x5C93F0`=+2、他 4 関数=+1）。数値（21 pane id）は不変。→ v630 §0。

---

## 1. V-628-1a: `0x5C4340` 本体の 7 項目 [確定(命令)]

**(1) extent**: `0x5C4340..0x5C4471`、`ret 0x14` @`0x5C446E`、以降 int3 パディング。

**(2) `[esp+0x30..0x50]` の全読み（本体全域）**:

| 命令 | 対象 | 種別 |
|---|---|---|
| `0x5C4369 mov eax,[esp+0x34]` | arg1 | read（parent → `0x585410`） |
| `0x5C4376 mov [esp+0x38],edi` | arg2 | **write**（ローカル再利用） |
| `0x5C4381 mov eax,[esp+0x40]` | arg4 | read（id） |
| `0x5C4397 mov [esp+0x40],eax` | arg4 | write（正規化 id） |
| `0x5C43AD mov ebp,[esp+0x38]` | arg2 | read |
| `0x5C43BC mov byte [esp+0x38],1` | arg2 | write |
| `0x5C43EB fild [esp+0x40]` | arg4 | read |
| `0x5C43FF mov ecx,[esp+0x34]` | arg1 | read（`GetFloat(parent,0xFD0)`） |
| `0x5C4416 fstp [esp+0x38]` / `0x5C441A fld [esp+0x38]` | arg2 | write/read |
| `0x5C443F mov edx,[esp+0x40]` | arg4 | read（→ `+0x94` 索引） |

- **`arg3`（`[esp+0x3C]`）と `arg5`（`[esp+0x44]`）は本体全域で一度も読まれない** → **未使用**（確定）。
- `arg2` は読み書きされるローカル（name ではなくスクラッチ）。

**(3) `this+0x94` store**:
```
0x5C442B fild [esp+0x14]          ; id ローカル
0x5C443A call 0x58CEB0            ; UpdateFloat(tile, 0xFAE=user0, id)
0x5C443F mov edx, [esp+0x40]      ; ★ arg4
0x5C4444 mov [ebx + edx*4 + 0x94], esi   ; this->[0x94 + id*4] = tile
```
- **`arg4` が `+0x94` の索引に届く**（確定）。値の出所は**引数**（counter ではない）。

**(4) `0xB3B5D4` の全 xref**（5 関数が触る）:
| 関数 | 増分 |
|---|---|
| `0x5C4340`（pane） | **+1** |
| `0x5C4480` | +1 |
| `0x5C4630` | +1 |
| `0x5C4800` | +1 |
| `0x5C93F0`（item） | **+2** |

- **`0x5C4340` と `0x5C93F0` は同一グローバルを共有**（確定）。→ **pane 用と item 用で counter は別ではない**。

**(5) `0x5C4F3C`**: 関数 head は **`0x5C4E40`（`FUN_005c4e40`）**（builder `0x5CA010` ではない）。
`0x5C4F3C mov [0xB3B5D4],0xC8`（=200）で、`[esi+0x8c0/0x8c4/0x8c8/0x8cc/0x8d0]` を設定 → **menu の Init/Open 相当**。

**(6) `[ebp+0xe8]`**: `0x5CA1B9 mov ebp,eax`（`eax` = `(*[0xB333C4]+0x170)()` = player）、
`0x5CA1C1 mov edi,[ebp+0xe8]` → **dword ロード**（ポインタ deref なし、値として使用）。

**(7) `edi` の 3 代入**（`0x5CA0B3` / `0x5CA0C5` / **`0x5CA1C1`**）と 21 呼出（`0x5CA448..0x5CBB21`）:
**21 呼出はすべて `0x5CA1C1` の後** → **全 21 サイトが同一 `edi`（=`player[0xE8]`）**。3 群には割れない。

---

## 2. V-628-1b: counter simulation による pane id 確定 [部分確定]

`arg4 = player[0xE8]`（全サイト同値）。**`+0x94` 表は id を索引にするため、全 pane が同値だと衝突する**。
→ **`player[0xE8] == -1`（＝自動採番）でなければならない**（この -1 は実行時値のため推定）。
`0x5C4340` は arg4==-1 のとき **counter+1** を id にする。

builder を線形 simulation（counter 初期 200、増分 §1(4)）:

> 訂正(v634): 以下の pane id 表は全 pane が +1 ずれ（Main は counter 非消費で 0）。
> 正値は docs/chargen_ui_spec.md §2 と tmp/re/v634_pane_names_global_ruling.md。

| pane | id | pane | id |
|---|---|---|---|
| Main | **201** | Forehead | 300 |
| Race | **210** | Jaw | 307 |
| Hair | **214** | Mouth | 316 |
| Face | **225** | Nose | 337 |
| Shape | **230** | GeneralSkin | 366 |
| Tone | **240** | Beard | 377 |
| GeneralFace | **247** | EyesTex | 390 |
| Brow | **258** | Eyebrows | 403 |
| Cheeks | **265** | MouthLips | 418 |
| Chin | **276** | NoseTex | **425** |
| Eyes | **291** | | |

- **counter 使用者 call 総数 = 139** = 既知の **139 generator**（Pane21+Slider90+Button22+Toggle5+Text1）と一致。
- final counter = 429。

---

## 3. 残点

- **V-629-1【低】**: `player[0xE8] == -1` の実行時確認（推定の裏取り。runtime 依存）。
- （継続）V-626-2（`menu+0x94` の読み手）/ V-626-3（item builder と template）。
- （継続）row 604: 旧書式行（pipes!=7）の正規化。

---

## 4. 検証

- probe **u91_1**（extent／本体全域の `[esp+0x3X/0x4X]` 読み／`+0x94` store／`0xB3B5D4` 全 xref／`0x5C4F3C`／`0x5CA1C1`／edi と 21 呼出）。
- probe **u92_1**（counter 使用者の増分）。
- probe **u93_1**（counter simulation → 21 pane id・139 呼出）。
