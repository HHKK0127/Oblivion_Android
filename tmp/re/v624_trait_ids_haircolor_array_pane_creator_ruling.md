# v624 — trait ID 同定（`0xFA5`=stackingtype / `0xFAB`=depth）・`+0x8F0` hair color 配列・`0x5C4340` = pane 生成器

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u79_1 / u80_1 / u81_1**（EXIT=0）／実物 XML／既存逆コンパイル。
判定: `[確定(命令)]` / `[確定(実物XML)]` / `[部分確定]`

---

## 0. 訂正（査読受理）

- **◪ の「`16×'%s'→menu+0x8F4..0x92C`」はアドレスが誤り**。実測では
  - **`+0x8F0..+0x92C`（16 ints, stride 4）** = **hair color 値**（`iHairColor00..15`）
  - **`+0x930..+0x9AC`（16 バッファ, stride 8）** = **hair color 名**（`sprintf(dst,"%s",sHairColorN)`）
  - `0x8F0 + 16*4 = 0x930` で整合。**査読の算術指摘は正しい**（0x8F4 起点では 0x930 に届かない）。
- **◪ の「`0x5C4340` = pane→page = pane id」は正しい方向**だが、実体は **pane 生成器**（下記 §3）。

---

## 1. trait ID の同定 [確定(命令)] / [確定(実物XML)]

| trait | 名前 | 根拠 |
|---|---|---|
| `0xFA5` | **stackingtype** | builder が `GetFloat(menuRoot,0xFA5)`（`0x5CA15D`）で読み、XML の `<stackingtype> &no_click_past; </stackingtype>` に対応。`&no_click_past;`=`0x66`（登録）。 |
| `0xFAB` | **depth** | builder が `SetFloat(menuRoot,0xFAB,<0xFA5 の値>)`（`0x5CA197`）。 |
| `0xFA8` | id | §3 |
| `0xFAA` | listindex | §3 |
| `0xFAE` | user0 | §3 |

- → **`GetFloat(0xFA5) → SetFloat(0xFAB)` は「重なり順（stackingtype）を depth に写す」z オーダーの後処理**。
  chargen 固有の意味は無い。`0x5CA010` は「XML を読んで tile を作り、最後に depth を振る」**汎用の解釈器**で、
  chargen 固有なのは `0x40C` ガードのみ。
- 定数: `0x5CA169 fcomp [0xA69770]` / `0x5CA182 fcomp qword [0xA69778]` と比較（stackingtype の種別判定）。

---

## 2. `+0x8F0` 配列 = hair color（16 件）[確定(命令)]

`0x5CA1DE..0x5CA421`（builder 内の展開ループ、`0x5CA204` 以降は 4 バイト刻み）:

```
0x5CA1DE mov ecx, [0xB393B8]      ; iHairColor00
0x5CA1E4 lea ebx, [esi+0x8F0]
0x5CA1EA mov [ebx], ecx           ; this->[0x8F0] = iHairColor00   ★ int 配列先頭
0x5CA1EC mov edx, [0xB39338]      ; sHairColor0
0x5CA1F3 lea eax, [esi+0x930]
0x5CA1F9 push 0xA2F864            ; "%s"
0x5CA1FE push eax
0x5CA1FF call 0x402E30            ; sprintf(this->[0x930], "%s", sHairColor0)
...
0x5CA20A mov [esi+0x8F4], ecx     ; this->[0x8F4] = iHairColor01
...
0x5CA408 mov [esi+0x92C], ecx     ; this->[0x92C] = iHairColor15  ★ 末尾
0x5CA421 call 0x402E30            ; sprintf(this->[0x9A8], "%s", sHairColor15)
```

| 配列 | 範囲 | stride | 件数 | 内容 |
|---|---|---|---|---|
| int | **`+0x8F0..+0x92C`** | 4 | **16** | hair color 値（`iHairColor00..15`） |
| char | **`+0x930..+0x9AC`** | 8 | **16** | hair color 名（`sHairColor0..15`、`"%s"`） |

- **上限はハードコード**: `0x5CC11E cmp eax,0x10; 0x5CC125 jl 0x5CC0AB`（`i<16` のループ）。超過スロットは無い。
- **名前はラベル**: `0x5CBD16 lea eax,[esi+eax*8+0x930]` → `mov eax,[eax]` → `push 0xFB4`（=user6）
  ＝ **hair color 名を tile の user6（ラベル）へ書く**。→ §4。
- → **査読 §5 の「16 はページ数」は否定**。16 は **hair color 数**。ページ（pane）数は別（§3、callers 21）。

---

## 3. `0x5C4340` = pane 生成器 [確定(命令)]

`0x5C4340(this=ecx, parentTile, id)`:
```
0x5C4367 mov ebx, ecx                 ; this
0x5C4369 mov eax, [esp+0x34]          ; arg1 = parentTile
0x5C4370 push 0xA6D2DC                ; ★ "race_template_pane"
0x5C4375 push eax
0x5C437A call 0x585410                ; テンプレート名引き → 生成 → esi
0x5C4381 mov eax, [esp+0x40]          ; arg2 = id
0x5C4385 cmp eax, -1
0x5C4388 jne ...
0x5C438A mov eax,[0xB3B5D4]; add eax,1; mov [0xB3B5D4],eax   ; ★ id 採番（counter）
0x5C43F5 push 0xFA8 ; call 0x58CEB0   ; UpdateFloat(tile, id,   id)
0x5C4421 push 0xFAA ; call 0x58CEB0   ; UpdateFloat(tile, listindex, ...)
0x5C4435 push 0xFAE ; call 0x58CEB0   ; ★ UpdateFloat(tile, user0, id)
0x5C4444 mov [ebx + edx*4 + 0x94], esi ; ★ this->[0x94 + id*4] = tile
0x5C4459 mov eax, esi                 ; 戻り = tile
```
- **入出力型**: 入力 `(menu* this, Tile* parent, int id)`、戻り `Tile*`。id==-1 なら counter `[0xB3B5D4]` から採番。
- **`0x585410` = テンプレート名引き**: `ecx+8` のリストを `[edi+4]` で走査し、`__stricmp`(`0x98262D`) で
  名前一致 → `0x401F00(0x14)` 確保 → `0x58CD30`（clone）。＝**表引き（名前→テンプレート）→ 生成**。
- **`tile.id`(0xFA8) と `tile.user0`(0xFAE) に同じ id** を書き、**`menu+0x94` の id→tile 表**に入れる。
- `callers_of(0x5C4340)=21`。すべて builder 内で `arg1=[esi+0x28]`（同一の親 tile＝window）→ **pane 21 個**。
- `+0x94` 表は 0x5C4340/0x5C45EE/0x5C47C9/0x5C48E1/0x5C9584/0x5C958B で書き、0x5C64E0/0x5C655C/0x5C6C59/0x5C6D57 等で読む。

---

## 4. buttonID の正体（§3 からの結論）[確定(命令)]

- 生成 pane/button は **`id`(0xFA8) と `user0`(0xFAE) の両方に同じ値**を持つ。
- よって v623 の `if (buttonID == GetFloat(GetTile(要素名,ページ名), 0xFAE))` が成立する
  （エンジンは clicked tile の `id` を buttonID として渡し、コードは同名 tile の user0 と比較）。
- **`menu+0x94` = pane id→tile 表**。`menu+0x8F0`/`+0x930` = hair color 配列（§2）。
- **名前（sMain/sHair/…/sHairColor0..15）はラベル**。ページ/項目の識別子は **数値 id**。

---

## 5. V-619-1 の前提更新 [部分確定]

- 31 名（`0xB38F70..0xB39068`）は **ラベル文字列とページ識別子の混在**。
  - ラベル用途: `RaceDescription`（Text 行）、`HairColor0..15`（user6）、各種スライダ名。
  - 識別子用途: `Main/Race/Face/Hair/Eyes`（`GetPage(name)` のキー）、要素名（`GetTile(element,page)` のキー）。
- **名前を user0 の比較相手と決め打たない**（user0 は数値 id）。`ClassMenu` の prior（user1..10=名前 / user11..20=値）と整合。

---

## 6. 残点

- **V-624-1【中】**: 21 pane の **id と親子関係**（どの id がどのページ/タブか）。`0x5C4340` 呼出の arg2（id）を全列挙。
- **V-624-2【中】**: `menu+0x94` 表の読み手（`0x5C64E0` 等）と pane 選択の実装。
- **V-624-3【低】**: `0x585410` の arg 完全同定（`0x14` 確保と `0x58CD30` clone の中身）。
- **V-624-4【低】**: `0xB3B5D4` counter の初期値と上限。
- （継続）V-623-3（ページボタン tile の user0 書き手・155 母数）／V-620-2（155 の層別）。

---

## 7. 検証

- probe **u79_1**（builder の `0x8E0..0x950` 参照＝store 15＋`0x930` 配列、`cmp eax,0x10`、`0xFA5/0xFAB`、`0x5C4340` 本体）。
- probe **u80_1**（`+0x8F0` 配列の展開ループ本体＝`iHairColor`/`sHairColor`＋`"%s"`、`0x5CBD16` の user6 書き、`+0x94` 表の全参照、`callers_of(0x5C4340)=21`）。
- probe **u81_1**（21 呼出の引数＝親 `[esi+0x28]`、`0x585410`＝`__stricmp` 表引き、`0x9828C0`＝`cvttsd2si`）。
- 実物 XML `race_sex_menu.xml`（`<stackingtype> &no_click_past;`）。
