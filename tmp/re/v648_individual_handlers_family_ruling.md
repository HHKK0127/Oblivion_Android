# v648 — 個別ハンドラ family（28 サイト）: `GetFloat(named_tile, 0xFA8) == double(arg1)`

- **日付**: 2026-11-23
- **帯域**: HandleClick `FUN_005cc200`（menu id 0x40C RaceSexMenu）
- **判定語彙**: `[確定(命令)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: 探索 = rizin 0.9.1（`D:\re\r2q.sh`）。デコード最終確認 = capstone。対象 md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`。範囲は半開区間。

## §0 結論（要旨）

- **個別ハンドラ family が存在する**: `fucompp` は HandleClick 内に **28 サイト**。各サイトは **`GetFloat(0x5c3440(paneName, tileName), 0xFA8) == double(arg1)`**。
- **arg1 の float 化は 1 回だけ**（`0x5CC915 fild dword [ebp+8]; fstp dword [esp+0x18]`）→ **`[esp+0x18]` を 28 サイトが共有**。だから `fild` の単純走査では 1 件しか出なかった。
- **遷移表は 2 層**: (a) chain（`id 90`、現在ページ→hub/leaf、v646/v647）＋ (b) 個別ハンドラ（28 サイト、`id == arg1` → ページ移動）。

## §1 — #1: `push 0xFA8` / `0xFAE` と arg1 比較 `[確定(命令)]`

範囲 `[0x5CC200, 0x5CDEF0)`。バイト検索（`68a80f0000` / `68ae0f0000`）:

| パターン | 件数 | 内訳 |
|---|---|---|
| `push 0xfa8` | **51** | 19（chain サイト名の hoist）＋1（`0x5CC8BF`）＋3（commit）＋28（fld 帯の GetFloat 引数） |
| `push 0xfae` | **24** | 19 = `GetFloat(this->tile,0xFAE)`（chain ガード）＋4 = `SetFloat(this->tile,0xFAE,..)`（`0x5CC857`＋3 commit）＋1（`0x5CDA69`） |

- **`GetFloat(クリックtile, 0xFAE)` の読者は 0 件**（`push 0xfae` の 24 件はすべて `this->tile`）。
- **arg1 と比較するガード = `fucompp` 28 サイト**（`u149_6.py`）。`fild dword [ebp+8]` は `0x5CC915` の 1 件のみ（`float(arg1)` を `[esp+0x18]` に保存し全サイトが再利用）。
- `cmp ebx, imm` は **3 件**のみ（`0x5a`/`0xa`/`0x62`）。他形式の arg1 比較なし。

### 28 サイトの (paneName, tileName) 対（idx 名は thunk から回収）

| fucompp | pane(idx) | tile(idx) | 定数 |
|---|---|---|---|
| `0x5CC92F` | Random(23) | Face(9) | — |
| `0x5CCC87` | （Face(9)） | Face(9) | — |
| `0x5CCD26` | **Main**(7) | **Age**(12) | — |
| `0x5CCD96` | **Hair**(10) | Red(17) | — |
| `0x5CCDF7` | **Hair**(10) | Green(18) | — |
| `0x5CCE58` | **Hair**(10) | Blue(19) | — |
| `0x5CCEB9` | **Hair**(10) | Length(20) | — |
| `0x5CCF22` | **Race**(8) | Gender(14) | — |
| `0x5CCF8B` | **Main**(7) | **Face**(9) | `[esp+0x3c]=0xa` |
| `0x5CD02A` | **Face**(9) | **Shape**(21) | — |
| `0x5CD0B0` | **Face**(9) | **Tone**(22) | — |
| `0x5CD136` | **Main**(7) | **Hair**(10) | `[esp+0x3c]=0x10` |
| `0x5CD1B6` | **Main**(7) | **Race**(8) | `[esp+0x3c]=0x12` |
| `0x5CD236` | **Shape**(21) | General(Face)(25) | — |
| `0x5CD2BC` | **Shape**(21) | Brow(26) | — |
| `0x5CD342` | **Shape**(21) | Cheeks(27) | — |
| `0x5CD3C8` | **Shape**(21) | Chin(28) | — |
| `0x5CD44E` | **Shape**(21) | Eyes(11) | — |
| `0x5CD4D4` | **Shape**(21) | Forehead(29) | — |
| `0x5CD55A` | **Shape**(21) | Jaw(30) | — |
| `0x5CD5E0` | **Shape**(21) | Mouth(31) | — |
| `0x5CD666` | **Shape**(21) | Nose(32) | — |
| `0x5CD6EC` | **Tone**(22) | General(Skin)(33) | — |
| `0x5CD772` | **Tone**(22) | Beard(35) | — |
| `0x5CD7F8` | **Tone**(22) | Eye shades(125) | — |
| `0x5CD87E` | **Tone**(22) | Eyebrows(36) | — |
| `0x5CD904` | **Tone**(22) | Mouth(Lips)(37) | — |
| `0x5CD98A` | **Tone**(22) | Nose shades(126) | — |

- **Main のボタン（Face/Hair/Race/Age）は `0x5CCF8B`/`0x5CD136`/`0x5CD1B6`/`0x5CCD26`** で、**chain ではなくこの family が処理**する。
- 追加 idx 名: 13=Complexion / 14=Gender / 15=Race Description / 16=Style / 17=Red / 18=Green / 19=Blue / 20=Length / 39-45=Face ratio 系 / 126=Nose shades / 127=Hair Color。
- 各サイトの分岐: 一致 → `[esp+0x3c/0x48]` に定数（`0xa`…`0x12`）を設定 → `jmp 0x5ccfb7`（共通 finalize）。不一致 → 次サイトへ `jp`。
- 定数の意味（`0xa`=10, `0x10`=16, `0x12`=18 等）は `[未確定]`（V-648-1）。

## §2 — #2: head の arg1 分岐 `[確定(命令)]`

```
0x5CC25A mov ebx,[ebp+8]     ; arg1
0x5CC258 test al,al
0x5CC25D je 0x5cc8a3         ; flag==0 → 0x5cc8a3
0x5CC263 mov ecx,[esi+0x8ec]; push 0; call 0x57dd90
0x5CC270 mov ecx,esi; call 0x5c30c0
0x5CC277 cmp ebx,0x5a; jne 0x5cc8ba     ; arg1==90 → chain / else → 0x5cc8ba
0x5CC280 mov eax,[0xb38f70]  ; Main → chain（0x5CC2A8）
...
0x5cc8a3 cmp ebx,0xa; jne 0x5cc277      ; arg1==10 → 0x5CC8AC / else → 0x5cc277
0x5cc8ac mov ecx,esi; call 0x5c2730; mov ecx,esi; call 0x5c30c0 → 0x5cc8ba
0x5ccbf4 cmp ebx,0x62        ; arg1==98 経路
```

- **chain（`0x5CC2A8`）へ入るのは `arg1==90` のみ**。
- `arg1==10`（0xa）→ `0x5CC8AC`（2 call）→ `0x5cc8ba`（個別 family）。
- それ以外 → `0x5cc8ba`（個別 family）。`arg1==98` → `0x5ccbf4`。
- **ジャンプテーブルは無し**（if-else 連鎖）。

## §3 — #3: Face / Race / Hair の扱い `[確定(命令)]`

- chain の 19 サイトの paneN 集合 = {Main, Shape, Tone, General(Face), Brow, Cheeks, Chin, Eyes, Forehead, Jaw, Brow, Mouth, Nose, General(Skin), Beard, Eye shades, Eyebrows, Mouth(Lips), Nose shades}。
- **Face(9) / Race(8) / Hair(10) はこの集合に含まれない** → 現在ページが Face/Race/Hair のとき、chain は **19 サイトを全部素通り**して **`0x5CC816`（`SetFloat(race_button_done,0xFB3,1.0)` → `SetFloat(this->tile,0xFAE,Main)` = Main 復帰）** に落ちる。
- Face/Race/Hair への移動は **§1 の個別 family**（`0x5CD02A`/`0x5CD0B0` 等、および Main のボタン `0x5CCF8B`/`0x5CD136`/`0x5CD1B6`）が担う。

## §4 — #4: Main のボタン生成 `[確定(命令)]（negative）`

- `0x5CA448 call 0x5c4340`（`ecx=esi`, `arg=[esi+0x28]=race_window`）→ 戻りを **`0x5CA45F mov [esi+0x40], eax`**（= `panes[0]` = Main pane 生成）。
- `0x5CA491`/`0x5CA4D5`/`0x5CA518 call 0x5c4480`（button 生成）。名前は **Race(8)/Face(9)/Hair(10)**、index は `mov [esp+0x4c], 0/1/2`。
- **生成 site の直後に `0xFAE` を書く命令は無い**（次の名前ロードへ続く）。
- **`GetFloat(クリックtile, 0xFAE)` の読者は 0 件**（全 .text の `push 0xfae` 322 件を分類、HandleClick 内 24 件はすべて `this->tile`）。
- → **「user0 に遷移先 id を入れる」汎用経路は存在しない**（否定）。ボタンは **§1 の名前ベース（`0x5c3440(pane,tile)` ＋ `id==arg1`）** で処理される。

## §5 — 遷移表の書き方（2 層）

| 層 | 入力 | 判定 | 動作 |
|---|---|---|---|
| **chain** | `arg1==90` | `this->tile.user0 == paneN.id`（現在ページ） | `this->tile.user0 := paneX.id`（hub/leaf: Face↔Shape/Tone、他は Main） |
| **個別ハンドラ** | 任意 `arg1` | `GetFloat(0x5c3440(pane,tile), 0xFA8) == arg1` | 定数（`0xa`…`0x12`）設定 → finalize `0x5ccfb7` |

## §6 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-648-1 | 個別ハンドラが設定する定数（`0xa`…`0x12`）の用途（遷移先 id？ sub-index？） | `[未確定]` |
| V-648-2 | `0x5C3440` の戻り tile を `GetFloat` する意味（クリック tile との同一性は id 比較で担保） | `[部分確定]` |
| V-647-1 | `0x5C3440` の str1/str2 が pane 名/tile 名（§1 で pane/tile と確定） | **解決** |
| V-646-3 | fld 反復の増分定数の用途 | `[未確定]` |

## §7 — 検証手段（コピペ可）

```sh
python D:\re\u149_1.py   # push 0xfa8/0xfae 全件 + head
python D:\re\u149_2.py   # fild/cmp ebx / ボタン生成
python D:\re\u149_3.py   # cmp ebx 全形式 / 0xfae 分類
python D:\re\u149_4.py   # fild 走査 / ボタン生成前方
python D:\re\u149_5.py   # push 0xfae 全 .text 分類
python D:\re\u149_6.py   # fucompp 28 サイト + name 対
python D:\re\u149_7.py   # 追加 idx 名
```
