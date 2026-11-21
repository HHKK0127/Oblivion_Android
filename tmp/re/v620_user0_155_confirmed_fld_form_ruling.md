# v620 — 155 の確定（esp 追跡）・2.0 の積み方（`fld` 形）・29 の条件明記・tile 層別の残点

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 我々の実読 probe **u64_1 / u65_1**（EXIT=0）。v619 の検収。
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 定義（以後この数は定義付きで扱う）

| 数 | 定義 |
|---|---|
| **155** | `0xFAE` を **arg1(`[esp+4]`) として消費する call** が **setter**（`0x58CEB0 UpdateFloat` / `0x58CED0 UpdateString`）であるサイト数。判定は **esp 追跡**（`push`/`pop`/`sub esp`/`add esp` と callee の `ret N` を反映し、各 call で 0xFAE が arg1 に来るかを判定）。 |
| **29** | **`fld dword [0xA379B4]`（2.0）を含む直近 8 命令**を持ち、消費 call が `0x58CEB0 UpdateFloat` のサイト数。※`push dword [0xA379B4]` 形は **0 件**（下記）。 |
| 160 | 同条件で消費 call が `0x588BD0 GetFloat`（読み手）。 |

---

## 2. (1) 2.0 の積み方は **`fld` 形**（`push` 形は存在しない）[確定(命令)]

- **`push dword ptr [0xA379B4]`（`ff 35 b4 79 a3 00`）の出現: 0 件**。
- **`fld dword ptr [0xA379B4]`（`d9 05 b4 79 a3 00`）: 386 件**。
- 実例（`push 0xFAE` の直前）:
  ```
  0x57D50B  fld  dword ptr [0xA379B4]      ; 2.0
  0x57D511  push ecx
  0x57D512  mov  ecx, dword ptr [esi+0x68] ; ★ menuRoot
  0x57D515  fstp dword ptr [esp]           ; 値 = 2.0
  0x57D518  push 0xFAE                     ; trait
            ... call 0x58CEB0 UpdateFloat
  ```
  ```
  0x59605A  fld1                           ; 1.0
  0x59605C  jmp  0x596064
  0x59605E  fld  dword ptr [0xA379B4]      ; 2.0
  0x596064  fstp dword ptr [esp]           ; bool（1.0 or 2.0）
  0x596067  push 0xFAE
  ```
- → **`fld [0xA379B4]; push ecx; fstp [esp]; push 0xFAE`** の形。**`push` 形は 0 件**なので、`fld` 起点の 29 は**全数**（食い違いなし）。

---

## 3. (2)(4) esp 追跡による真の消費者 → **155 を確定** [確定(命令)]

`push 0xFAE` 全 322 の消費者（arg1 に 0xFAE が来る call）:

| 区分 | 件数 | 内訳 |
|---|---|---|
| **書き手**（setter） | **155** | UF 120 / US 35 |
| 読み手（getter） | 160 | GF 160 |
| その他 | 5 | `0x589980` ×5 |
| 未同定 | 2 | — |
| **計** | **322 ✓** | |

- → **v619 の 155 は過小ではない**（`other` 41 は全て**文字列構築 `0x4028D0` 等**で、真の消費者は後続の `GetFloat`＝**読み手**だった）。
- 査読の「`other` に書き込み経路が混ざっていれば 155 は過小」は **否定**（esp 追跡で全 322 が 155/160/5/2 に解決）。
- **失格 37 の理由**（callee 別）: `0x4028D0` 系 31（`sub esp,8`＋文字列構築 → 真の消費者は後続の GetFloat）／`0x419B10` 4（`sub esp,8`＋`push edx`）／`0x58A0A0` 1（属性登録）／UF 1（`0x5A2C73`＝`push ecx; push 0xFA1`）。
- **残る「その他」= `0x589980` 5 件**: `mov ecx,[ebp+0x28]; cmp [ecx+4], ebx` で始まる検索系（0x5DA036/0x5DBE39/0x5DC2BA/0x5DC357/0x5DC4EE）。**getter か writer か要同定** → V-620-1。

---

## 4. (3) 29 の tile 取得元 → **層別は未完** [部分確定]

- 直近 8 命令の後方探索で層別できたのは 14 件のみ（境界探索の限界）。うち **menuRoot(`[esi+0x68]`) は 2 件**（`0x57D518`・`0x5BBF76`）、他 12 は local/other。
- → **menuRoot 由来の除外**は完了していない。**ecx の前方追跡**（tile をどこから得たか）が必要 → **V-620-2**。
- 確度のある事実: `0x57D518`（0x57 帯）は **menuRoot** 由来＝**汎用フラグの書き手**（v619 §4 の予想どおり）。

---

## 5. 残点（順序は査読案を採用）

- **V-620-1【中】**: `0x589980`（5 件）の同定（getter / writer / その他）。
- **V-620-2【中】**: 155 書き手・29 の tile 取得元を **ecx 前方追跡**で層別（menuRoot / GetTileMenu / GetContainingMenu / ローカル）。
- **V-620-3【中】**: menuRoot 由来を除外した残りが chargen 経路（V-619-3 を継承）。
- **V-620-4【低】**: 未同定 2 件。
- （継続）V-619-1（31 名の用途分類）。

---

## 6. 検証

- probe **u64_1**（2.0 の積み方＝fld 形・push 形 0／other 41 の全同定＝`0x4028D0`31・`0x419B10`4・`0x589980`5・`0x58A0A0`1／失格 37 の理由）／**u65_1**（esp 追跡で消費者を同定＝155/160/5/2）。
- 合計一致: 155 + 160 + 5 + 2 = **322**。
