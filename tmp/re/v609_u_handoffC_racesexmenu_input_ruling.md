# v609 — 依頼 C 裁定: RaceSexMenu のページ送り入力経路と Main ページ（V-558-1/2/3）＝**既裁定の確認＋`0x5C1B80` は QuickKeysMenu(0x416) であり RaceSexMenu と無関係という訂正**

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u21_1〜u27_1**・全 EXIT=0・冒頭 Test-Path）＋実物 XML（`Oblivion - Misc.bsa` → `menus\chargen\race_sex_menu.xml` 22156B・705行）＋ `docs/Oblivion_decompiled.c`（`FUN_005c4340` @L330405 / `FUN_005ca010` @L333024）。
先行: `tmp/re/v558_racesexmenu_pages_ruling.md`（U2）・`v563_u2_page_switch_ruling.md`（V-558-1 クローズ）・`v567_u2_page_order_ruling.md`・`v602_mid_residuals_bundle_ruling.md`（V-558-2/3 クローズ）。
依頼書: `tmp/re/handoff_20261120_movie_fit_loading_slideshow_chargen_pages.md` 依頼 C（ai-todo #162）。
判定: `[確定(命令)]` / `[確定負]` / `[部分確定]`

---

## 0. 前提: 依頼 C の3残点は **v563/v602 で既にクローズ済み**

依頼書が求める V-558-1/2/3 は以下で既に裁定済み。本 v609 は**再確認＋新規の訂正1点**を行う。

| 残点 | 既裁定 | 出典 |
|---|---|---|
| V-558-1 ページ送り呼出元 | **Done ボタン（id=90）のみ**。タブ・左右キー・スクロールは [確定負] | v563 |
| V-558-2 AddPane index の実値 | **a4 が `-1` のときのみ `[0xB3B5D4]`++ 採番（初期 `0xC8`=200）／明示 id は非消費（Main のみ明示 0）** | v602 §2.2 |
| V-558-3 Main ページの内容 | **`TextEditMenu`(0x41B) ではなく `race_name` tile 内の内部 trait `0xFD3`/`0xFD4` 入力欄**。Done(90) は `0x5CCA77` で名前検証→確認ダイアログ | v602 §2.3 |

---

## 1. 結論（新規確定・訂正）

| 問い | 判定 | 結果 |
|---|---|---|
| **C-新規①** v558 §2.2 の「ページ送り実装型5つ」のうち `0x5C1B80` は RaceSexMenu のものか | **[確定負]（v558 訂正）** | **`0x5C1B80` は `QuickKeysMenu`（menu id `0x416`=1046）のハンドラ**であり、**RaceSexMenu のページ送りではない**。`0x5C1B80` は `0x589B70(0x416)` で QuickKeysMenu を取得し、その `[esi+0x24]`（状態）を 2/4 判定して処理する（probe u27_1）。→ v558 §2.2 の「EXE 全域の SetTrait(0xFAE) 5型」は**別メニューの実装型の寄せ集め**であり、**RaceSexMenu 自身のページ送りは `0x5CC200`（Done=id90 経路）の1箇所のみ**（v563 を補強）。 |
| **C-新規②** Done/Back の区別 | **[確定(XML+命令)]** | `race_button_done`（id=90）は **XML の `<user5>` で Done/Back を切替**。実物 XML: `<user5> &false; </user5>`＋コメント `<!-- 1 (false) for Done and 2 (true) for Back -->`、`<_text_1>`=`strings()._done`／`<_text_2>`=`strings()._return`、`<user0>` に `me().user5` を copy。**Done の通知経路 `0x5CC81F: push 0xFB3`** で `[RaceSexMenu+0x3C]` の値（user5）を書き戻す＝Done/Back のラベル更新。 |
| **C-新規③** 21 ページの `user0` 実値 | **[確定(命令)]** | `AddPane`（`FUN_005c4340`）は **`SetTrait(pane, 0xFA8 /*id*/, index)`・`SetTrait(pane, 0xFAA /*listindex*/, childcount−1)`・`SetTrait(pane, 0xFAE /*user0*/, index)`** を同一の `index` で書く（`0x5C43EB`-`0x5C443A` 逐語）。`index` は **`param_5`**（`ret 0x14`＝5引数）。→ **page の `user0` ＝ AddPane の index**（Main のみ明示 0、他は `[0xB3B5D4]` 採番 200 系）。可視判定は `me().user0 == RaceSexMenu.user0` の**一致比較**なので採番値の絶対値は不問。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 C-新規① `0x5C1B80` は QuickKeysMenu（v558 訂正）

```
0x5C1B80:
  5C1B94 call 0x5C1900
  5C1B99 push 0x416          ; ★ QuickKeysMenu (1046)
  5C1B9E call 0x589B70       ; FindMenu
  5C1BA6 test eax,eax ; jne 0x5C1BC4
  5C1BAA call 0x5C1290 ; 5C1BAF push 0x416 ; 5C1BB4 call 0x589B70
  5C1BC4.. 5C1BCA call 0x5898F0  ; 基底取得
  5C1BD5 cmp [esi+0x24],2 ; je 0x5C1BE1
  5C1BDB cmp [esi+0x24],4 ; jne 0x5C1BE8
  5C1BE1 call 0x584390
```
→ `0x416`＝QuickKeysMenu（v586 で `0x587D20` switch の `0x416` case → `0x5886F7` ctor `0x5C0B50`・`quickkeys_menu.xml` [確定]）。**RaceSexMenu(0x40C) ではない**。

### 2.2 C-新規② Done/Back（実物 XML・`race_sex_menu.xml` L210-232）

```xml
<image name="race_button_done">
  <include src="button_long.xml"/>
  <id> 90 </id>
  <user0>
    <copy src="me()" trait="user5"/>
    <copy src="me()" trait="_text_"/>
  </user0>
  <user5> &false; </user5>   <!-- 1 (false) for Done and 2 (true) for Back -->
  <_text_1> <copy src="strings()" trait="_done"/> </_text_1>
  <_text_2> <copy src="strings()" trait="_return"/> </_text_2>
</image>
```
```
0x5CC200 内: 5CC816 mov ecx,[esi+0x3C] ; 5CC819 fld1 ; 5CC81B push ecx ; 5CC81C fstp [esp]
              5CC81F push 0xFB3        ; ★ user5 ← [RaceSexMenu+0x3C]（Done/Back ラベル）
              5CC824 call 0x58CEB0      ; SetFloat
```
→ **Done/Back は同一 tile（id=90）で `<user5>` により切替**（1=Done, 2=Back）。タブやキーではない。

### 2.3 C-新規③ AddPane の index 三連書込（`0x5C4340`・逐語）

```
5C43EB fild [esp+0x40]        ; index
5C43F5 push 0xfa8 ; call SetFloat   ; id     = index
5C4403 push 0xfd0 ; call 0x588BD0   ; childcount
5C440D fsub qword [0xA2F928]=1.0
5C4421 push 0xfaa ; call SetFloat   ; listindex = childcount-1
5C442B fild [esp+0x14]        ; index（[esp+0x14] は 5C439B で index を保存）
5C4435 push 0xfae ; call SetFloat   ; user0  = index
```
```
5C4385 cmp eax,-1 ; jne 0x5C4397     ; param_5==-1 のときだけ
5C438A mov eax,[0xB3B5D4] ; add eax,1 ; mov [0xB3B5D4],eax   ; 採番（初期 0xC8=200: ctor 0x5C4F3C）
```
→ **`user0`＝`id`＝AddPane の index**（Main のみ 0、他は 200 系採番）。可視は一致比較。

---

## 3. 移植への指針（依頼 C）

1. **原版のページ送りは Done(id=90) の連打のみ**（タブ・キー・スクロール無し）。Done/Back は同一ボタンの `<user5>`（1=Done/2=Back）で切替。
2. port の 5 タブ（`Name/Race/Attributes/Skills/Appearance`）は原版に無い [確定負]。正典 21 ページへ寄せるなら「Done で次ページ」のウィザード挙動を再現する必要がある（タブ化は仕様変更）。
3. Main ページの名前入力は `TextEditMenu` ではなく **`race_name` tile の内部入力欄**（v602）。Done で名前検証→確認ダイアログ。
4. **`0x5C1B80`（QuickKeysMenu）を RaceSexMenu のページ送りと混同しないこと**（v558 §2.2 の誤誘導の訂正）。

---

## 4. 残点

- **V-609-1【低】**: `race_button_done` の `<user5>` を 1/2 に書き換える実命令（Done/Back の切替元＝現在ページによる set）。
- **V-609-2【低】**: `AddButton`（`FUN_005c4480`）の index 引数（uStack_4 の 0..0x36 系）と各要素の対応（要素一覧は v571 の 28 ブロック表）。
- **V-567-2/3【低】**（未消化のまま継承）。

---

## 5. 検証

- probe: **u21_1〜u27_1（7本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x5C4340`（`0xFA8`/`0xFAA`/`0xFAE` 三連書込・`cmp eax,-1`→採番）／`0x5C4F3C`（`[0xB3B5D4]=0xC8`）／`0x5CC81F`（`push 0xFB3`＝Done/Back ラベル）／`0x5C1B80`（`push 0x416`＝QuickKeysMenu）／`0x5CA010`（21 AddPane 呼出・head 確定）。
- 実物 XML: `race_sex_menu.xml`（L210-232 `race_button_done` `<user5> &false;`・`_text_1`=_done/`_text_2`=_return）。
- 反証: `0x5C1B80` の `push 0x416` により QuickKeysMenu と同定（RaceSexMenu=0x40C ではない）。
