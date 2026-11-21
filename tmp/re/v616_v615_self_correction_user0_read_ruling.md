# v616 — v615 の自己訂正: `0x5CDEF0` は user0 を「読む」（書かない）・`0x5790E0` の引数順訂正・V-615-1 は float で決着

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 我々の実読 probe **u57_1 / u58_1**（EXIT=0）。v615 の検収。
判定: `[確定(命令)]` / `[確定(命令)（negative）]`

---

## 1. 訂正（3 件）

### 1.1 `0x5CDEF0` は user0 を**読む**（v615 の「書く」は誤り）[確定(命令)（negative）]
- 12 サイトの `push 0xFAE` の直後 callee は **全 12 箇所が同一パターン**:
  `['0x4028D0', '0x4028D0', '0x5C3440', '0x588BD0']`
- **`0x588BD0` = `Tile::GetFloat`**（xOBSE）→ **読み出し**。`0x5C3440` = ページ/要素 tile の取得（v610 の tile lookup）。`0x4028D0` = BSStringT 代入（比較用の名前構築）。
- → **`0x5CDEF0` は user0(0xFAE) を「読む」**。v615 §3.2 の「user0 を 12 箇所で書く」は **[確定(命令)（negative）] 訂正**。

### 1.2 `0x5790E0` の引数順 [確定(命令)（negative）]
```
0x5CE572  push 0
0x5CE574  push 0x3E9
0x5CE57B  call 0x5790E0        ; cdecl: arg1=0x3E9(menu id), arg2=0(mask)
```
- 本体: `0x579145 mov eax,[esp+8]`＝**arg1（menu id）** → `GetTileMenu(arg1)`。`0x579159 mov edi,[esp+0x10]`＝**arg2（mask）**。
- `0x57915D test edi,edi; je 0x579172` → **mask==0 のときは `test [eax+0x24], edi` を踏まない**（v615 の「`test [menu+0x24], arg0`」は arg を 1 つずらして読んだ誤り）。
- → 呼出は **`0x5790E0(0x3E9, 0)`**。v615 の「`(0, 0x3E9)`」を訂正。

### 1.3 V-615-1 決着 = **user0 は float**（v610 が正）[確定(命令)]
| 側 | callee | 意味 |
|---|---|---|
| **builder `0x5CA010`**（user0×4） | **`0x58CEB0` = UpdateFloat** | **書き（float）** |
| **`0x5CDEF0`**（`push 0xFAE`×12） | **`0x588BD0` = GetFloat** | **読み** |
- → **user0(0xFAE) は float**。**v610 の float 裁定が正**、v615 の「string（ページ名）」は誤り。
- **ページ名グローバル（`sMain`/`sRace`/…/`sColor`）は `GetPage(name)`（`0x5C3110` の `__mbscmp(name, sMain)` 系）の引数**であって、user0 の値ではない。

---

## 2. `0x5790E0` の完全な意味 [確定(命令)]

`0x5790E0(menuId, mask)`:
1. `InterfaceManager_GetSingleton(0,1)` を 3 回；`[mgr+0x1C]!=0`・`[mgr+0x68]`(**menuRoot**)!=0 を確認。
2. `GetFloat(menuRoot, 0xFAE)` を **`0xA379B4`(=2.0)** と比較（不一致なら偽）。
3. `GetTileMenu(menuId)`（無ければ偽）。
4. `mask != 0` なら `GetContainingMenu(tileMenu)[+0x24] & mask` を検査（0 なら偽）。
5. 最後に `GetFloat(tileMenu, 0xFA1=visible)` を `[0xA2F948]` と比較。
- → **「menuId の tile menu が表示中か（visible 比較）＋任意マスク」の述語**。`IsMenuMode(0x578F60)` の同族。
- `[mgr+0x68]` = **menuRoot**（OBVR `kInterfaceMenuRootOffset=0x68`・xOBSE `GameAPI.h` と一致）。

---

## 3. row518（V542）の被引用 row [確定(命令)]

- 台帳 row: **519(V543)** / **520(V544)** / **524(V548)** / **550(V574)** / **555(V579)** / **582(V606)**、＋ 我々の **588/589/591**。
- U 節内の言及: line 393/396/424（`[0xB13970]` を「生成済み集合」として参照）。
- → **row518 の「ブック系マネージャ」という名称を前提にしている可能性がある downstream**: V543（メニューシステム）・V544（窓 ID↔クラス表）・V548（属性表）・V606（`0x42BB90`）。**V-615-5 の名称訂正はこれら 4 row へ波及**（docs/GT 反映時に一括）。

---

## 4. v615 への波及

- §3.2（「`0x5CDEF0` が user0 を書く」）と §3.4（「v610 float vs v615 string の不整合」）を **本件で解消**。
- **V-615-2 の再定義**: `0x5CDEF0` は「user0 を読んでページを判定する」関数。**user0 の書き手は builder（×4）**。
- 13 ページ名 vs 12 サイトの差は、**サイトが「読み」**である以上、対応表の突合対象は **builder の user0 書込（4 件）と GetPage(name) 呼出**に移る。

---

## 5. 残点

- **V-616-1【高】**: user0(0xFAE) の書き手の完全列挙（builder ×4 以外に存在するか）。`153 件の user0 set サイト`（過去裁定・supermemory 由来）との母集団の突合。
- **V-616-2【中】**: `GetPage(name)`（`0x5C3110`）経由のページ探索と user0(float) の関係（user0 はページ index か）。
- **V-616-3【低】**: builder の **user13(0xFBB)×6** の用途（chargen に不自然な数）。
- （継続）V-614-2（builder caller `0x57B7E0`）/ V-615-2（`0x5CDEF0` 完全 decode）/ V-615-4（式木 dump 仕様）。

---

## 6. 検証

- probe **u57_1**（slot 11 の call 直前 2 命令＝`push 0; push 0x3E9`／`0x5790E0` 全 body＝`[esp+8]`=arg1・`[esp+0x10]`=arg2・`je 0x579172`）。
- probe **u58_1**（12 サイトの callee 列が全て `[0x4028D0,0x4028D0,0x5C3440,0x588BD0]`）。
- 台帳 grep（row518 の被引用: 519/520/524/550/555/582）。
