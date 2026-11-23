# v647 — commit の `paneX` は group ごとに固定（遷移は hub/leaf 構造）

- **日付**: 2026-11-23
- **帯域**: HandleClick `FUN_005cc200`（menu id 0x40C RaceSexMenu）
- **判定語彙**: `[確定(命令)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: 探索 = rizin 0.9.1（`D:\re\r2q.sh`）。デコード最終確認 = capstone。対象 md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`。範囲は半開区間。

## §0 名前レジストリの所在（重要な訂正）

- rizin `iS`: `.text` vaddr `0x401000` / `.rdata` `0xA28000` / **`.data` vaddr `0xB02000`（RVA `0x702000`）vsize `0xAA000` / paddr `0x700C00` size `0x30C00`**。
- **名前レジストリ `0xB38F38+i*8` は `.data` 内だが、`.data` の file 実体は `0x30C00` バイトのみ**。`0xB38F38` は raw 範囲外＝**BSS（実行時充填）**。→ **静的ファイルから名前は読めない**（v646 の registry 読みが garbage だった原因）。
- 名前は **thunk `0x9F6710+i*0x20`** が供給する（`push <display>; push <editorid>; call 0x419A70`）。thunk の即値文字列から回収した（`u148_4.py`）:

| idx | display | editorid |
|---|---|---|
| 7 | Main | sMain |
| 8 | Race | sRace |
| 9 | Face | sFace |
| 10 | Hair | sHair |
| 11 | Eyes | sEyes |
| 12 | Age | sAge |
| 21 | Shape | sShape |
| 22 | Tone | sTone |
| 23 | Random | sRandom |
| 24 | Reset | sReset |
| 25 | General(Face) | sGeneralFace |
| 26 | Brow | sBrow |
| 27 | Cheeks | sCheeks |
| 28 | Chin | sChin |
| 29 | Forehead | sForehead |
| 30 | Jaw | sJaw |
| 31 | Mouth | sMouth |
| 32 | Nose | sNose |
| 33 | General(Skin) | sGeneralSkin |
| 34 | Skin | sSkin |
| 35 | Beard | sBeard |
| 36 | Eyebrows | sEyebrows |
| 37 | Mouth(Lips) | sMouthLips |
| 38 | Lips | sLips |
| 125 | Eye shades | sEyesTex |

## §1 — #1: commit 4 ブロック全文 `[確定(命令)]`

各ブロックは同型: `paneX = 0x5c3110(name)` → `SetFloat(this->tile, 0xFAE, GetFloat(paneX, 0xFA8))` → 共有 tail へ `jmp`。

| ブロック | name（idx） | paneX | 末尾 |
|---|---|---|---|
| `0x5CC97F` | `[0xb38fe8]` = **Tone**(22) | Tone | `jmp 0x5cc869` |
| `0x5CC9C4` | `[0xb38fe0]` = **Shape**(21) | Shape | `jmp 0x5cc869` |
| `0x5CCA09` | `[0xb38f80]` = **Face**(9) | Face | `jmp 0x5cc87d` |
| `0x5CCA77` | — | — | 名前検証（`GetString(race_name,0xFDE)` 空白チェック）→ Done |

- 代表（`0x5CC97F`）:
  ```
  0x5CC97F mov eax,[0xb38fe8]        ; Tone
  0x5CC984 push 0xfa8
  0x5CC989 sub esp,8; ... push eax; call 0x419b10   ; BSFixedString(Tone)
  0x5CC998 mov ecx,esi; call 0x5c3110               ; paneX = pane(Tone)
  0x5CC99F mov edi,[esi+4]                           ; edi = this->tile
  0x5CC9A2 mov ecx,eax; call GetFloat                ; GetFloat(paneX,0xFA8)
  0x5CC9A9 push ecx; fstp dword [esp]                ; 値
  0x5CC9AD push 0xfae
  0x5CC9B2 mov ecx,edi; call SetFloat                ; SetFloat(this->tile,0xFAE, paneX.id)
  0x5CC9B9 mov edx,[0xb38fe8]; jmp 0x5cc869          ; tail（user1 0xFAF 設定）
  ```
- **paneX の出所 = 各ブロック固有の `0x5c3110(name)` 呼び出し**（`0x5c3440` の戻りではない）。**ブロックごとに固定**。
- 共有 tail `0x5CC869`: `0x5c3110(name)` → `SetFloat(this->tile, 0xFAF=user1, GetFloat(pane,?))` → `jmp 0x5cd9c5`。

## §2 — #2: site1 全文 `[確定(命令)]`

```
0x5CC2A8 call 0x5c3110          ; paneN = pane(Main)
0x5CC2AD mov ecx,[esi+4]        ; this->tile
0x5CC2B0 push 0xfae
0x5CC2B7 call GetFloat          ; GetFloat(this->tile,0xFAE)
0x5CC2BC fstp qword [esp+0x20]
0x5CC2C0 mov ecx,edi; call GetFloat   ; GetFloat(paneN,0xFA8)
0x5CC2C7 fcomp qword [esp+0x1c]
0x5CC2CB fnstsw ax; test ah,0x44
0x5CC2D0 jnp 0x5cca77           ; 一致(=現在ページが Main) → 名前検証(Done)
0x5CC2D6 mov eax,[0xb38fe0]     ; ← 不一致なら site2 のセットアップへ落ちる
```

- **site1 は fall-through で site2 へ進む**（jmp ではない）。19 サイトは直列チェーン。
- site1（Main）だけ commit を持たないのは、**Main の「確定」が名前検証（`0x5cca77`）＝Done 処理**であり、ページ切替 commit ではないため。

## §3 — #3: `0x5C3440` の 4 引数と戻り `[確定(命令)]`

- prologue: SEH 8 push → `esi=ecx`(this) / `edi=[esp+0x2c]`(arg3) / `ebx=[esp+0x24]`(arg1)。**`ret 0x10`（4 引数）**（`0x5C34C6`）。
- 本体:
  ```
  0x5C346E push edi; sub esp,8; ... push ebx; call 0x4028d0   ; BSFixedString(arg1)
  0x5C3493 mov ecx,esi; call 0x5c3110                          ; pane = pane(arg1)
  0x5C349A mov ecx,eax; call 0x589930                          ; tile = 0x589930(pane, arg3)
  0x5C34A2 mov esi,eax; ... free; mov eax,esi; ret 0x10        ; return tile
  ```
- **`0x5C3440(this, str1, str2)` = `str1` で pane を引き（`0x5c3110`）、`0x589930` で pane 内の tile を `str2` で検索して返す**（`0x589930` = v602 V-571-2「page の子ツリーから name 完全一致 tile を返す探索関数」）。
- → 戻り値は「**クリックされた tile そのもの**」ではなく「**名前で引いた pane 内の tile**」。
- 呼び出し site（`0x5CC910`）は 2 つの BSFixedString（`[0xb38ff0]`=idx23=Random、`[0xb38f80]`=idx9=Face）を積む（4 dword = 4 引数）。
- → `0x5CC92B` ガード = **`GetFloat(0x5C3440(this,"Random","Face") の戻り tile, 0xFA8) == double(クリックid)`**。

## §4 — #4: commit の `paneX` が `paneN` と異なる説明 → **C** `[確定(命令)]`

site の `paneN`（`0x5c3110` 名）と jnp 先 commit の `paneX`:

| site | paneN（idx） | jnp → commit | paneX（idx） |
|---|---|---|---|
| 1 | Main(7) | `0x5cca77` | （名前検証/Done） |
| 2 | Shape(21) | `0x5cca09` | **Face(9)** |
| 3 | Tone(22) | `0x5cca09` | **Face(9)** |
| 4-13 | General(Face)(25), Brow, Cheeks(27), Chin, Eyes(11), Forehead, Jaw(30), Brow, Mouth(31), Nose | `0x5cc9c4` | **Shape(21)** |
| 14-19 | General(Skin)(33), Beard, Eye shades(125), Eyebrows, Mouth(Lips)(37), Nose shades | `0x5cc97f` | **Tone(22)** |

- **paneX は group 内で固定**（Face 系→Shape、Skin 系→Tone、{Shape,Tone}→Face）。**クリックされた tile の user0 ではない**（B 否定）。
- paneX は paneN の**親**（General(Face)→Shape、General(Skin)→Tone）＝ hub/leaf の**親方向**（A の「子ページ」は否定）。
- → **C が正しい**: 「グループ内で paneX が固定（グループ＝行き先、site は現在ページ＝クリック元の判定）」。
- 構造の読み: ガードは `this->tile.user0（現在ページ） == paneN.id`。一致時に `this->tile.user0 := paneX.id`。＝ **「現在ページが paneN のとき、クリックは paneX へ遷移する」**。Shape/Tone が hub、その配下（Face 系／Skin 系）が leaf。

## §5 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-647-1 | `0x5C3440` の str1/str2 が pane 名/tile 名のどちらに割り当たるか（"Random"/"Face" の役割） | `[部分確定]` |
| V-646-2 | commit の paneX が paneN と異なる理由 | **解決 `[確定(命令)]` = C** |
| V-646-3 | fld 反復の増分定数（0xb..）の用途 | `[未確定]` |
| V-645-1 | fld ループの走査対象 | `[部分確定]` |

## §6 — 検証手段（コピペ可）

```sh
sh /mnt/d/re/r2q.sh -c "iS" -c "q" /mnt/d/re/Oblivion.exe
python D:\re\u148_1.py   # commit 4 ブロック / site1 / 0x5C3440
python D:\re\u148_2.py   # sections / tail / 0x419b10 / registry raw
python D:\re\u148_3.py   # thunk imms / site 名
python D:\re\u148_4.py   # thunk name 文字列
python D:\re\u148_5.py   # 0x5C3440 全命令 + 4 引数
```
