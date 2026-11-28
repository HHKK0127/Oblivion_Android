# v635 — 214 Style / 215 Hair Color の裁定: bool 反転ではなく index 巡回（wrap）（ai-todo #201）

対象: `Oblivion.exe` v1.2.0416 (Steam) 7,898,624B / sha256 `A8F313845C1545E9A60E1E995961EEF4C033115DA9443F6D756341DF3C2B7DC6`。
根拠: 命令実読（Ghidra 12.1.4 + GhidraMCP 7.0.0、project `OblivionRe`、program `/OblivionStandalone.exe`、image base `0x00400000`、関数 21,824）＋実物 XML。
XML: `D:\re\race_sex_menu.xml` 22,156B / raw md5 `d367341334fa89101ba4fc077edb52fe` / CRLF / 705 行。
**ベース一致の確認（2026-11-28 実測）**: `0x5C4340` の 32B が Ghidra メモリと PE `.text` ファイルオフセット `0x1C3740` で完全一致（`6aff6830119c0064a1…508d4424`）。
先行: `tmp/re/v570_racenotify_dispatch_ruling.md`（`0x5CC200` の 28 ブロック dispatch 表）／`tmp/re/v634_pane_names_global_ruling.md`（pane id の裁定）
判定: `[確定(XML)]` / `[確定(命令)]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 問い | 判定 | 答え |
|---|---|---|
| 214 Style は bool 反転か index 巡回か | **[確定(XML+命令)]** | **index 巡回（wrap）**。bool ではない |
| 215 Hair Color は bool 反転か index 巡回か | **[確定(XML+命令)]** | **index 巡回（wrap）**。決定打は §5（`user0` を配列添字に使う） |
| 端の挙動は wrap か clamp か | **[確定(XML)]** | **wrap**（`user0` の式が `<mod>`）。`V-654-3` は wrap で確定 |
| クリックはメニュー側ハンドラに行くか | **[確定(命令)(negative)]** | **行かない**。`FUN_005CC200` の 28 ブロックに Style / Hair Color の行は 0 件（§3） |
| id の割当順 | **[確定(命令)]** | **Style=214 / Hair Color=215**（builder の生成順、§7） |
| port 側に受け皿はあるか | **[確定(データ)]** | **無い**（§9）。`[+0x870]` / `[+0x88C]` 相当が未実装 |

bool に見えるのは、`race_template_toggle` の既定が `user4=0` / `user5=1` のときだけ。範囲をコードが与えると巡回器になる（§6）。

---

## 2. 証拠 1: XML テンプレートが index 巡回そのもの `[確定(XML)]`

`race_sex_menu.xml` L417-435（template `race_template_toggle` = 102 の実体化元）:

```
<rect name="race_template_toggle">           <!-- L402 -->
<id> 102 </id>                               <!-- L403 -->
...
<user0>                                      <!-- L417 current index -->
<!-- retain old value -->                    <!-- L418 -->
  <add src="me()" trait="clicked"/>          <!-- L419 本体クリック -->
  <sub src="child(race_template_toggle_left)" trait="clicked"/>   <!-- L420 -->
  <add src="child(race_template_toggle_right)" trait="clicked"/>  <!-- L421 -->
  <add src="me()" trait="user3"/>            <!-- L422 manual set value -->
  <sub src="me()" trait="user4"/>            <!-- L423 min index -->
  <mod>                                      <!-- L424 -->
    <copy src="me()" trait="user5"/>         <!-- L425 max index -->
    <sub src="me()" trait="user4"/>
    <add> 1 </add>
  </mod>
  <add src="me()" trait="user4"/>
</user0>                                     <!-- L430 -->
<user1> 0 </user1>                           <!-- L431 starting value -->
<user2> Label </user2>                       <!-- L432 text label -->
<user3> 0 </user3>                           <!-- L433 manual set value -->
<user4> 0 </user4>                           <!-- L434 min index -->
<user5> 1 </user5>                           <!-- L435 max index -->
<user6> Value </user6>                       <!-- L436 current string, set by code from index -->
```

- 式: `user0 = ((clicked − left.clicked + right.clicked + user3) − user4) mod (user5 − user4 + 1) + user4`
- 左右の子が ±1、端は **`<mod>` による wrap**（clamp する要素が無い）。
- `user1` は「starting value – compare this to current to see changes」＝変化検出用、`user6` は「コードが index から設定する現在文字列」。
- `<xleft>` / `<xright>`（L452-453）は左右子の `clicked` を参照するだけの配線。

---

## 3. 証拠 2: 通知 dispatch に 214/215 の行が無い `[確定(命令)(negative)]`

`FUN_005cc200`（vtable slot 3、`0x5CC200`-`0x5CDA25`、6,182B、caller 0）は要素名（string）で分岐する 28 ブロックの線形 dispatch。**Hair ページに対応する行は次の 4 つだけ**で、Style / Hair Color の行は存在しない:

| No | strA | strB | ACT |
|---|---|---|---|
| 4 | Red `sRed` | Hair | `0x5C5C30(1)` |
| 5 | Green `sGreen` | Hair | 同上 |
| 6 | Blue `sBlue` | Hair | 同上 |
| 7 | Length `sLength` | Hair | `0x5CCFB7` |

→ クリックは「ページ遷移」にも「メニュー側の値変更」にも行かず、**値変更は widget（XML `user0`）側で完結**する。spec §10.4 で Race(211) について確定済みの構図（dispatch に無い＝ページ切替ではない）と同一。

補助: `"Style"` 文字列実体 `0x00A629F0` の xref は init thunk `0x009F6910` の 1 本のみ、`"Hair Color"` は `0x00A638D8` の xref が init thunk `0x009F76F0` の 1 本のみ（`0x5CC200` からの参照は無い）。

---

## 4. 証拠 3: 毎 tick の user0 読み手 `FUN_005cdef0`（`0x5CDF29` を含む）`[確定(命令)]`

`FUN_005cdef0`（`0x5CDEF0`-`0x5CE561`、1,650B、479 命令）が `PUSH 0xFAE`（trait **user0**）を **12 回**行い、`0x5C3440`（名前引き）→ `0x588BD0`（GetFloat）で値を読む。最初の読みが `0x5CDF29`。

### 4.1 trait オフセット（本ノートで使う範囲）

| trait | オフセット | 出典 |
|---|---|---|
| id | `0xFA8` | v570 §3 |
| user0（current index） | `0xFAE` | 本ノート §4.2 の実測（12 回） |
| user1（slider/toggle の開始値・最小値） | `0xFAF` | spec §7（L428） |
| user3（manual set value） | `0xFB1` | spec §10.6 |
| user4（min index） | `0xFB2` | §6 の実測 `PUSH 0xFB2` |
| user5（max index） | `0xFB3` | spec §10.4（`0x5C7FBC`）/ §6 |
| user6（current string） | `0xFB4` | spec §10.4（`0x5C80EE`） |

### 4.2 要素名・親ページは GameSetting 実体（静的 init thunk から復元）

`0x9F66D0`-`0x9F6A4E` に 0x20B 刻みの init thunk が並び、各 thunk は
`PUSH <既定値文字列>; PUSH <EDID>; MOV ECX, <GameSetting 実体>; CALL 0x00419A70; PUSH <型>; CALL 0x00981FB4`
の形。実読結果:

| GameSetting | EDID | 既定値 | 用途 |
|---|---|---|---|
| `0xB38F70` | `sMain` | Main | 親ページ Main |
| `0xB38F78` | `sRace` | Race | 親ページ Race ／ 要素 Race |
| `0xB38F80` | `sFace` | Face | 親ページ Face |
| `0xB38F88` | `sHair` | Hair | 親ページ Hair |
| `0xB38F90` | `sEyes` | Eyes | 要素 Eyes（Main） |
| `0xB38F98` | `sAge` | Age | 要素 Age（Main） |
| `0xB38FA0` | `sComplexion` | Complexion | 要素 Complexion（Main） |
| `0xB38FA8` | `sGender` | Gender | 要素 Gender（Race） |
| `0xB38FB0` | `sRaceDescription` | Race Description | text（Race） |
| **`0xB38FB8`** | **`sStyle`** | **Style** | **要素 Style（Hair）= 214** |
| `0xB38FC0` | `sRed` | Red | slider（Hair） |
| `0xB38FC8` | `sGreen` | Green | slider（Hair） |
| `0xB38FD0` | `sBlue` | Blue | slider（Hair） |
| `0xB38FD8` | `sLength` | Length | slider（Hair） |
| `0xB38FE0` | `sShape` | Shape | button（Face） |
| `0xB38FE8` | `sTone` | Tone | button（Face） |
| `0xB38FF0` | `sRandom` | Random | button（Face） |
| `0xB38FF8` | `sReset` | Reset | button（Face） |
| `0xB39000` | `sGeneralFace` | General(Face) | button（Shape） |
| **`0xB39330`** | **`sHairColor`** | **Hair Color** | **要素 Hair Color（Hair）= 215** |

`0xB39330` の EDID 文字列は exe 内 `0xA638D8`（`"Hair Color"`）、init thunk は `0x009F76F0`。
※ `0xB38F00`-`0xB39000` 帯は**ファイル上ゼロ**（`0xB38F40` を u32 で走査して全 0 を実測）。GameSetting は起動時の init thunk が実行時に構築する。よって静的ポインタ走査では 0 件になる（誤って「テーブルが無い」と結論しないこと）。

### 4.3 12 回の読みと消費型

`MOV EAX,[GameSetting]` → `CALL 0x004028D0`（一時オブジェクト）→ `CALL 0x5C3440`（tile 解決）→ `CALL 0x588BD0`（GetFloat）の順で 12 回。各回の**消費型**は次の通り:

| # | 要素（親ページ） | 変換 | 比較・キャッシュ先 | 変化時の適用 |
|---|---|---|---|---|
| 1-3 | Red / Green / Blue（Hair） | `×255` → 3 バイト | `[actor+0x1E8]` | `FUN_005c5c30` |
| 4 | Length（Hair） | float | `[actor+0x1CC]` | `FUN_005c62d0` |
| 5-6 | **Hair Color（Hair）** | **`FILD` int 比較** | **`[+0x88C]`** | **`FUN_005c5f00`** |
| 7 | Race（Race） | int | `[+0x86C]` | `FUN_005cda30` |
| 8 | Gender（Race） | **bool**（`SETG`→byte） | `[+0x868]` | `FUN_005c9770` |
| 9 | Age（Main） | float | `[+0x880]` | `FUN_005c50a0` → `0x5C7070` → `0x5C2F20` |
| 10 | Complexion（Main） | float | `[+0x884]` | 同上 |
| 11 | Eyes（Main, id 204） | int | `[+0x87C]` | `FUN_005c9980` |
| 12 | **Style（Hair）** | **int** | **`[+0x870]`** | **`FUN_005c5b00`** |

**bool に正規化されるのは Gender（8）だけ**。命令は `0x5CE330` で `CALL 0x005c9770` → `0x5CE335 CMP EBP,ESI`（ESI=0）→ `0x5CE337 SETG CL` → `0x5CE33A MOV byte [EDI+0x868],CL`。Style（12）と Hair Color（5-6）は int のまま扱われる。

---

## 5. 証拠 4（決定打）: Hair Color の適用関数が index を配列添字に使う `[確定(命令)]`

`FUN_005c5f00`（Hair Color 適用）の先頭:

```c
  FUN_005c3440();                 /* tile 解決（要素名 Hair Color / 親ページ Hair） */
  FUN_00588bd0();                 /* GetFloat(tile, 0xFAE) = user0 */
  iVar2 = FUN_009828c0();         /* float -> int */
  *(undefined4 *)(param_1 + 0x878) = *(undefined4 *)(param_1 + 0x8f0 + iVar2 * 4);
```

`user0` を **`[actor+0x8F0 + idx*4]` の添字**として使っている。bool（0/1）でも式は成立してしまうが、
- 直後に `FUN_0058CEB0` を 3 連 × 4 回（RGB 4 系統）呼ぶ
- `FUN_005c5c30`（色適用）へ落ちる

という構造から、**色テーブル（`+0x8F0`）の選択**であることは明白。bool では「色テーブルの選択」を説明できない。

`FUN_005c5b00`（Style 適用）は `FUN_005c50a0()`（前提条件チェック）→ `GetFloat(tile,user0)` と `[+0x870]` を比較 → 差があれば `FUN_005c2b50` を呼ぶ。こちらも int のまま。

---

## 6. 証拠 5: min/max はコードが供給する `[確定(命令)]`

`FUN_005c7ba0`（Race トグルを埋める関数。spec §10.4 で `user5=件数-1` / `user6=名前` を書くと確定済み）の中に:

- `0x5C896E PUSH 0xFB2`（user4 = min）
- `0x5C89DB PUSH 0xFB3`（user5 = max）

があり、`0xB39330`（sHairColor）の read が `0x5C8911` / `0x5C897A` / `0x5C89E7` に、`0xB38FB8`（sStyle）の read が `0x5C855E` / `0x5C8645` / `0x5C8751` / `0x5C87BA` / `0x5C8830` / `0x5C88AE` にある。

→ **Style / Hair Color の `user5` は「リスト件数-1」がコードから入る**。既定 0/1 は初期値にすぎず、範囲が広がれば巡回器として振る舞う。

---

## 7. 証拠 6: id の割当順 `[確定(命令)]`

builder `FUN_005ca010`（Hair ページ構築部）:

```
0x5CA640  MOV EDX,[0x00B38FB8]    ; sStyle
0x5CA652  CALL 0x005C4630         ; toggle 生成 (1)
0x5CA657  MOV EDX,[0x00B39330]    ; sHairColor
0x5CA674  CALL 0x005C4630         ; toggle 生成 (2)
0x5CA679  MOV EDX,[0x00B38FC0]    ; sRed（以降 slider）
```

`0x5C4630` は toggle 生成器で **id を +1 消費**（spec §2）。生成順がそのまま id 順になるので **Style=214 / Hair Color=215**。`app/src/main/cpp/ui/race_sex_panel.cpp:148-149` の割当と一致。

---

## 8. `V-654-3`（Race トグルの端挙動）の改訂 `[確定(XML)]`

- 旧: 「端は clamp（wrap か未計測／`V-654-3`）」＝ `cycleRace()` のコメントと spec §10.4 の実装注記。
- 新: **wrap**。根拠は §2 の `user0` 式が `<mod>` を使うこと（clamp する要素が無い）。Race(211) も同じ template 102 の実体化なので同じ式で動く。
- つまり **port の `clamp` は原作とずれている**。実装は `((i - min) mod (max - min + 1)) + min` へ直す必要がある（`#195` 本体の作業に含める）。

---

## 9. port への含意（受け皿が無い）`[確定(データ)]`

- `UIToggle`（`app/src/main/cpp/ui/ui_toggle.h` 31 行）は `bool checked_` と `label_` だけ。**index 状態も左右ヒット領域も持たない**。`render()` は `size.y*0.8` のチェックボックスを 1 個描くだけ。
- `RaceSexPanel::add_toggle` は `add_element(slot, ElemType::Toggle, label)`（`race_sex_panel.h:445`）。**左右の子（`race_template_toggle_left` / `_right` 相当）を生成しない**。
- Race(211) だけは `cycleRaceFromTap(pane_local_x, row_width)`（`race_sex_panel.cpp:718`）が「左半分 = -1 / 右半分 = +1」で代替している。ただし `cycleRace` → `setSelectedRaceIndex` は **clamp**（§8 のとおり原作は wrap）。
- 214/215 の**適用先**（`[+0x870]` = Style index、`[+0x88C]` = Hair Color index、`[+0x8F0]` = 色テーブル）に相当する port 側の構造体は無い。`Element` の `value/min/max` は slider 用で、toggle は `checked`（bool）しか持たない。

→ `#195` 本体（`UIToggle` の index 化）と、min/max 供給＋適用先（別チケット）に分割する。

---

## 10. 未確定（本ノートで閉じない）

- Style の index が指す実体（髪メッシュのリスト）と、その件数の供給元（ESM / BSA のどちらか）。`V-635-1`
- Hair Color の `[+0x8F0]` テーブル（色数と各要素の型）。`V-635-1`
- 実機での見え方（左右タップで何が変わるか）は未計測。実装後に実機で確認する。
