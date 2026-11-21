# v621 — V-620-1 決着（`0x589980`＝削除系）・`0x5C3440` 同定・12 サイトのページ名・155 を母数として固定

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 我々の実読 probe **u66_1 / u67_1**（EXIT=0）。v620 の検収。
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. 155 を母数として固定（定義の再掲）

| 数 | 定義（以後この母数で層別する） |
|---|---|
| **155** | `0xFAE` を **arg1** として消費する call が **setter**（`0x58CEB0`/`0x58CED0`）であるサイト数（**esp 追跡**で判定）。 |
| **160** | 同条件で消費 call が **`0x588BD0 GetFloat`**（読み手）。 |
| **+5** | 消費 call が **`0x589980`**（下記＝**削除**）。→ **155 setters ＋ 5 deleters = 160 mutations**。 |
| 2 | 未同定。 |

- 合計: 155 + 160 + 5 + 2 = **322** ✓（v620 と同じ）。
- **証拠として残す形**: `0x57D518` の `fld [0xA379B4]; push ecx; mov ecx,[esi+0x68]; fstp [esp]; push 0xFAE`（**`push` で枠を確保し `fstp [esp]` で埋める**）＝ esp 追跡が正しく動いている根拠。**別の trait を扱うときも同じ形が出る**ので手順として有効。

---

## 2. V-620-1 決着: `0x589980` は**削除系（mutator）** [確定(命令)]

```
0x5899A4  mov ebp, ecx              ; this
0x5899A6  mov ecx, [ebp+0x28]       ; ★ リスト先頭（this+0x28）
0x5899AB  mov ebx, [esp+0x24]       ; arg1（= trait）
0x5899B1  cmp [ecx+4], ebx          ; ★ エントリ +0x04 = key と比較
0x5899B4  jne ...
0x5899B6  fld [ecx+0xC]             ; ★ エントリ +0x0C = float（現在値）
0x5899C5  fucompp / jnp ...         ; arg2（新値）と比較
0x5899D2  mov esi,[ecx]             ; エントリ +0x00 = owner
0x5899D4  mov edi,[esi+0x28]        ; owner+0x28 = リスト先頭
0x5899E0  loop: cmp eax,ecx / je / mov eax,[eax+0x14]   ; ★ +0x14 = next
0x5899FD  mov [edx+0x14], ecx       ; ★ unlink
0x589A02  call 0x401F20             ; free
```
- **エントリ形**: `{ +0x00 owner, +0x04 key, +0x0C float, +0x14 next }`。リストは **`[this+0x28]`**。
- **動作**: `key(+0x04)==arg1` かつ `float(+0x0C)==arg2` のエントリを **unlink + free** ＝ **削除**。
- → **`0x589980` は読み手ではなく削除系（mutator）**。5 サイトは `push 0xFAE` で **user0 のエントリを削除**している。
- → **155（setter）＋ 5（deleter）= 160 の「user0 を変更するサイト」**。
- 査読の「`GetValueByType`/`GetChildByIDTrait` のどちらかなら読み手」は **[確定(命令)（negative）] 否定**（`+4` は key、`+0xC` は float、`+0x14` は next＝**リスト削除**）。
- `callers_of(0x589980)` = **28**（うち 5 が 0xFAE）。

---

## 3. `0x5C3440` の同定 [確定(命令)]

```
0x5C3466  edi = [esp+0x2C]          ; arg2 = strB（親ページ名）
0x5C346A  ebx = [esp+0x24]          ; arg1 = strA（要素名）
0x5C346E  push edi / sub esp,8 / <BSStringT 構築> / call 0x4028D0
0x5C3495  mov ecx,esi(this) ; call 0x5C3110    ; GetPage(strB)
0x5C349C  mov ecx,eax       ; call 0x589930    ; 0x589930(page, strA) = 名前で子 tile 検索
```
- → **`0x5C3440(Menu* this, strA=要素名, strB=親ページ名)` ＝ `GetPage(strB)` → `0x589930(page, strA)`**＝**名前で引いた子 tile**。v610 の読みと一致。
- `callers_of(0x5C3440)` = **142**。

---

## 4. 12 サイトのページ名 [部分確定]

`push 0xFAE` → `0x5C3440` の間の global ロード（登録バンド名）:

| site | ページ名 |
|---|---|
| `0x5CDF29` | Red / **Hair** |
| `0x5CDF92` | **Hair** |
| `0x5CDFF3` | **Hair** |
| `0x5CE0F5` | **Hair** |
| `0x5CE170` | **Hair** |
| `0x5CE1E3` | **Hair** |
| `0x5CE24B` | **Race** |
| `0x5CE2C4` | **Race** |
| `0x5CE345` | **Main** |
| `0x5CE3A6` | **Main** |
| `0x5CE45C` | **Main** |
| `0x5CE4D5` | **Hair** |

- 分布: **Hair 7 / Race 2 / Main 3**。
- **要素名（strA）は窓が狭く未取得**（0x5CDF29 の Red は次サイト由来の混入の可能性）→ **V-621-1**。

---

## 5. 残点（査読案の順序）

- **V-621-1【中】**: 12 サイトの **strA（要素名）** の確定（0x5C3440 の直前 push を厳密に）。
- **V-621-2【中】**: 12 名（Hair/Race/Main 配下の要素）への user0 **書き手**探索（`0x5C3440`＋同名、または同名 tile を保持するローカル）。
- **V-621-3【中】**: 読み手 160 の層別（menuRoot / ページ tile / その他）。
- **V-621-4【低】**: `[this+0x28]` リストの owner が何か（Tile か Menu か）。
- **V-621-5【低】**: 未同定 2 件。
- （継続）V-619-1（31 名の用途分類）。

---

## 6. 検証

- probe **u66_1**（`0x589980` 本体＝削除系／`0x5C3440` 本体／callers 28・142）。
- probe **u67_1**（12 サイトのページ名＝Hair7/Race2/Main3）。
- xOBSE `GameTiles.h`（`Tile`/`Value`/`Expression` の宣言・trait 表）と照合。`+0x28` は xOBSE の Tile 宣言に無いフィールド（別リスト）。
