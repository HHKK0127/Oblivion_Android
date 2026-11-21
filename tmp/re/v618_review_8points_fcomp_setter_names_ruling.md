# v618 — 8 点の実測回答: `fcomp`→`jp` の実バイト・setter 引数順（trait は最後）・`0xA379B4` xref・名前グローバル 31 件・「153」の v615 訂正・precheck 改良と STATE 修正

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 我々の実読 probe **u60_1 / u61_1**（EXIT=0）。v617 の検収。
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| # | 指摘 | 判定 | 答え |
|---|---|---|---|
| 1 | `fcomp`→`jp` の機構 | **[確定(命令)] v617 の読みは正** | 実バイトは `fcomp; fnstsw ax; test ah,0x44; jp`。`sahf` ではなく `test` 経由なので `jp`＝「等しくない（>・<・NaN）」。→ **builder は `user0 == 2.0` のときだけ呼ばれる**。 |
| 2 | 116 は過小の疑い（trait が途中） | **[確定(命令)（negative）] 過小ではない** | `0x58CEB0`/`0x58CED0` とも **trait は最後の push**（`[esp+4]`=trait, `[esp+8]`=値）。→ `push 0xFAE; call setter` の **dist=0＝116 が正確**。 |
| 3 | 116 は単一母集団でない | **[部分確定] 同意（層別は未了）** | tile 取得元（menuRoot／GetTileMenu／ローカル）による層別は **V-618-4**。 |
| 4 | `0xA379B4` の xref で近道 | **[確定(命令)] 実行したが未特定** | `fld [0xA379B4]` **386** / `fcomp [0xA379B4]` **121**（計 507）。**user0=2.0 の書き手は逆走査が整合せず 0 件** → **V-618-1**。 |
| 5 | 名前 11／サイト 12／前回 13 の不一致 | **[確定(命令)] 母集団の差** | 登録バンドの名前は **173 件**、うちページ/セクション名は `0xB38F70..0xB39068` の **31 件**。12 サイトが緩い窓で参照するのは 11 種 → **厳密窓で作り直す**（V-618-3）。 |
| 6 | 「153」の訂正は v615 にも | **[記録] 実施** | v615 本文に訂正注記を追加（我々の文書なので追記可）。 |
| 7 | precheck の改良＋STATE 修正 | **[実施]** | regex を簡体字固有字の明示リストに変更。STATE の過去由来の中国語接続詞 5 箇所を本コミットで日本語に修正。 |

---

## 2. (1) `fcomp`→`jp` の実バイト [確定(命令)]

```
0x57B83B  d8 1d b4 79 a3 00      fcomp dword ptr [0xA379B4]
0x57B841  df e0                  fnstsw ax
0x57B843  f6 c4 44               test ah, 0x44
0x57B846  0f 8a 93 00 00 00      jp 0x57B8DF
```
- **`sahf` は無く、`test ah,0x44`**。ah bit6=`C3`(0x40)、ah bit2=`C2`(0x04)。
- `test ah,0x44` の結果と PF:
  | 関係 | C3 | C2 | result | PF | `jp` |
  |---|---|---|---|---|---|
  | 等しい | 1 | 0 | 0x40 | 0（奇数） | 不成立 → **fall through（builder 呼出）** |
  | >/< | 0 | 0 | 0x00 | 1（偶数） | 成立 → skip |
  | NaN | 1 | 1 | 0x44 | 1（偶数） | 成立 → skip |
- → **builder `0x5CA010` は `menuRoot.user0 == 2.0` のときだけ呼ばれる**（v617 §2 の読みは正）。
- 査読の「`jp` 単独なら NaN 専用」は **`sahf` 前提**。実バイトは `test` 経由なので成立しない。

---

## 3. (2)(3) setter の引数順 [確定(命令)]

```
0x58CEB0 UpdateFloat:  mov eax,[esp+4]  ; trait（arg1）
                       call 0x58B220     ; GetValue(trait)
                       fld [esp+8]       ; value（arg2）
                       call 0x58CA00     ; SetNumber
0x58CED0 UpdateString: mov eax,[esp+4]  ; trait（arg1）
                       call 0x58B220
                       mov ecx,[esp+8]   ; string（arg2）
                       call 0x58CA50     ; SetString
```
- **両者とも trait は `[esp+4]`＝最後の push**。呼出は `push <値>; push 0xFAE; mov ecx,<tile>; call`。
- → **`push 0xFAE` の直後が call の dist=0＝116 が、user0 の正確な書込数**。
- 査読の「setter は trait が途中（`push 0xFAE; push 2.0; call`）」は**本エンジンの引数順では成立しない**（trait が途中だと 0xFAE は arg2＝値になる）。
- dist 分布: **0=116**(UF88/US28) / **1=132**(GF99/UF28/US5) / 2=10 / 4=5 / 6=1。
  - dist≥1 は「途中命令が引数か否か（`mov ecx,...` は引数でない）」の切り分けが要る → **V-618-2**。

---

## 4. (4) `0xA379B4` の xref [確定(命令)]

- `fld dword [0xA379B4]` **386** / `fcomp dword [0xA379B4]` **121**（計 507）。2.0 は広く使われる定数。
- **user0=2.0 を書くサイトは特定できず**（dist=0 setter の直前行逆走査で `0xA379B4` を含むもの 0 件）。逆走査の整合（命令境界）が取れていないため → **V-618-1**（正しい逆走査 or Unicorn）。
- 参考: `0x57902F/0x5790AF/0x579138` 等に `fcomp [0xA379B4]` が集中（`0x5790E0` 系＝menuRoot.user0==2.0 の判定群）。

---

## 5. (5) 名前グローバル [確定(命令)]

- 登録バンドの `0xB38F70..0xB39500` = **173 件**。
- うちページ/セクション名は `0xB38F70..0xB39068` の **31 件**:
  `Main/Race/Face/Hair/Eyes/Age/Complexion/Gender/Race Description/Style/Red/Green/Blue/Length/Shape/Tone/Random/Reset/General(Face)/Brow/Cheeks/Chin/Forehead/Jaw/Mouth/Nose/General(Skin)/Skin/Beard/Eyebrows/Mouth(Lips)/Lips`
- 12 サイトが緩い窓で参照するのは 11 種（`sEyes 0xB38F90`・`sLength 0xB38FD8` が窓から落ちた）→ **「11/12/13 の不一致」は母集団の取り方の差**。
- → **サイト毎の厳密窓で対応表を作り直す**（V-618-3）。

---

## 6. (6) 「153」の v615 への訂正 [記録]

- 初出は **v615**（v616 で引用）。**v615 本文に訂正注記を追加**した（我々の裁定文書であり、原本 exe ではないので追記可）。
- 正: 実測 **`0xFAE` 即値 322／trait 直後 setter 116／getter 22**。

---

## 7. (7) precheck 改良＋STATE 修正 [実施]

- precheck の中国語判定を、**中国語専用語のみの明示リスト**（precheck v3）に変更。日本語語彙（使用・可能・点 等）と日本語複合語に含まれる2文字列の誤検出を排除。
- **STATE の過去由来の中国語接続詞 5 箇所**（L1299/1435/2103/2207/2212）を本コミットで `さもなくば` に修正（挙動に影響しない文書修正）。

---

## 8. 残点

- **V-618-1【高】**: user0=2.0 の書き手（正しい逆走査 or Unicorn）。
- **V-618-2【高】**: dist≥1 の引数切り分け＝正確な user0 書き手数。
- **V-618-3【中】**: サイト毎の厳密な名前対応表（31 名の母集団から）。
- **V-618-4【中】**: tile 取得元による層別（menuRoot／GetTileMenu／ローカル）。
- （継続）V-617-1（user0 の型/意味の最終確定）/ V-617-3（12 サイトの消費先）。

---

## 9. 検証

- probe **u60_1**（fcomp→jp 実バイト／0xA379B4 参照／`[+0x68]` 近傍／dist 分布／名前グローバル 173 件）。
- probe **u61_1**（`0x58CEB0`/`0x58CED0` prologue／`0xA379B4` x87 参照 507／dist=0 setter 116 の逆走査）。
- 既存資産のみ（`v307_common.py`＋capstone）。
