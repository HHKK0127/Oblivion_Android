# v602 — 中優先残点クローズ: `0x589930` シグネチャ・RaceSexMenu act 本体（`0x5C7070`/`0x5C5C30`）・AddPane id 採番・id98 特別 act・得点応答バイト・StatsMenu 派生残点（ai-todo #96/#161/#162/#157 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u15_1〜u15_3**・全 EXIT=0・冒頭 Test-Path）＋実物 XML（`Oblivion - Misc.bsa` 抽出）。
先行: `tmp/re/v558_racesexmenu_pages_ruling.md`・`v559_statsmenu_tabs_ruling.md`・`v561_u5_done_button_ruling.md`・`v567_u2_page_order_ruling.md`・`v569_p3_p5_rows_ruling.md`・`v570_racenotify_dispatch_ruling.md`・`v571_race_id_allocation_ruling.md`・`v590_id98_loop_survey_ruling.md`
判定: `[確定(命令)]` / `[確定(XML)]` / `[部分確定]`

---

## 1. 結論

| 残点 | 判定 | 結果 |
|---|---|---|
| **V-571-2** `0x589930` のシグネチャ | **[確定(命令)]** | **`0x589930(this=page, const char* name)`＝「この page の子ツリー（`[this+0x34]` BST）から、name が完全一致（`__mbscmp`）する tile を返す」探索関数**（`ret 4`＝1引数）。木の各ノードは `[node+8]` に tile、`[tile+8]` に名前文字列。`0x5C3440` は `GetPage(strB)` の結果 page に対し `0x589930(page, strA)` で要素 tile を取得する。→ **28 ブロックの要素名突合は「ページ名→要素名」の 2 段名検索**で、v571 のブロック対応表は正しい。 |
| **V-570-2** `0x5C7070`／`0x5C5C30` の本体 | **[確定(命令)]** | `0x5C7070`＝**Age/Main の確定実行系**（`vfunc+0x130`/`+0x134` で player 取得→`vfunc+0x170`→`0x521A10`＝年齢再計算）。`0x5C5C30`＝**Style ページ切替**（`[0xB38FC0]`=要素名／`[0xB38F88]`=ページ名で `0x5C3440`→`0x588BD0`＝trait 読み→`SetFloat(user0)`）。 |
| **V-558-2** AddPane の index 起点 | **[確定(命令)]** | `0x5C4340`（AddPane）は **a4（id）引数が `-1` のときのみ** `[0xB3B5D4]` カウンタを `+1` して採番（`0x5C4385 cmp eax,-1`／`0x5C438A..0x5C4392`）。**初期値 0xC8(200)**（v571 で確定）。**明示 id（0 等）は非消費**。→ **index は 1 始まり相当だが Main のみ明示 0**。 |
| **V-558-3** Main ページの内容 | **[確定(命令)]** | Main ページは**名前入力（`TextEditMenu` 直接起動ではない）と性別トグルを含む**。Done（id=90）押下時は `0x5CCA77` が名前の検証（プレースホルダ一致／空）→確認ダイアログ（`'%s %s?'` `0xA6B4D8`）。**`TextEditMenu`(0x41B) の生成呼出は RaceSexMenu の Done 経路に無い**（`0x5CCA77` は race_name の string trait `0xFDE` を読むだけ）＝**名前は `race_name` tile 内の `0xFD3`/`0xFD4` 入力欄で行う**（v570 id10 経路）。 |
| **V-567-1** `0x5CC8BA` の完全 decode | **[確定(命令)]** | `0x5CC8BA` は**「現在 id が全 28 ブロックの id より小さい（＝どの要素にも一致しない）」ときのフォールスルー**。id を `0xFA8`（tile id）と比較し、**不一致なら `0x5CCBF4`（id==0x62=98 判定）→ id98 特別 act**。`0x5CC936` は **id==98 のとき Random 確認**（`[0xB3B4C8]` 未確認時 `sRandomizeFace`＋cb `0x5C9FC0`；確認済みなら `0x5C9CD0`＝Random 実行）。→ **id98＝Random ボタンの act**。 |
| **V-593-2** `al==1` を立てるボタン | **[確定(命令)]** | `0x5BCDD0` の `id>=4` 分岐で **`[singleton+0xB0] = [MessageMenu+0x60](base=1) + id − 4`**（`0x5BCE16`-`0x5BCE21`）。**base は常に 1**（v594）。→ **id4→al=1（否定/No）、id5→al=2（肯定/Yes）**。v592 の「al==2 が Yes」と整合。 |
| **V-559-3** タブアイコンの UV crop | **[確定(XML)]** | `stat_tabs_p1_icon`〜`p5_icon`（`all_tabs_icons.dds`）の `<cropx>`/`<cropy>`。**p1: cropx=0, cropy=128, 64×64**（以降 p2..p5 は順次 cropy が 64 刻みで増加）。 |
| **V-569-2** p5 空白スロットの y 式 | **[確定(XML)]** | `stat_misc_template` の `stat_misc_item` は **`<y> = me().height(32) × me().listindex`**（`<copy src="me()" trait="height"/> <mul src="me()" trait="listindex"/>`）。**空白スロットは listindex×32 の空行**で正しい（動的追加行の予約枠）。 |
| **V-570-4** `0xFD3`/`0xFD4` の XML 名 | **[部分確定]** | `0xFD3`/`0xFD4` は race_name の**テキスト入力欄の内部 trait（カーソル位置/表示窓）**。XML に同名 trait は無く（`race_name` は `<id>10</id>` のみ）、**コード側の内部スロット**。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 V-571-2 `0x589930`

```
589930 mov esi,[ecx+0x34]          ; 子ツリー根
  589940 mov edi,[esi+8]           ; node->tile
  589946 mov eax,[edi+8]           ; tile->name
  58994C call __mbscmp(eax, arg)   ; ★ 完全一致
  589957 je → return edi (tile)    ; 一致なら tile 返却
  589959 call 0x589930(edi, arg)   ; 子へ再帰
  ret 4                            ; 1引数
```
`0x5C3440(this, strA, strB, prep)`＝`GetPage(strB)`→`0x589930(page, strA)`（v571 の 2 段名検索）。

### 2.2 V-558-2 AddPane `0x5C4340`

```
5C436F push 0 ; push 'race_template_pane' ; call 0x585410   ; テンプレ解決
5C4385 cmp eax,-1 ; 5C4388 jne 0x5C4397                     ; a4==-1?
5C438A mov eax,[0xb3b5d4] ; 5C438F add eax,1 ; 5C4392 mov [0xb3b5d4],eax   ; ★ カウンタ+1
```

### 2.3 V-558-3 Main ページ（名前検証）

```
5CCA77 mov ecx,[esi+0x30] ; push 0xfde ; call 0x588C10   ; race_name string trait
5CCA9B je 0x5CCADF                                        ; 長さ0（空）
5CCAA0.. 空白判定ループ（非空白で flag 落とす）
5CCAF4 call __mbscmp(name, [0xB39440]=sEnterName)         ; プレースホルダ一致?
5CCAFE je 0x5CCBB7                                        ; → エラー経路
5CCB0E.. 母音判定→ '%s %s?'（0xA6B4D8）→ 0x579C10 確認     ; ★ 名前確定は確認ダイアログ
```
→ **TextEditMenu は使わない**（race_name の内部入力欄＋Done 確認）。

### 2.4 V-567-1 `0x5CC8BA`（id98 特別 act）

```
5CC8BA mov eax,[0xb38ff0] ; push 0xfa8 ; ... ; call 0x5C3440   ; 要素 tile 取得
5CC915.. call 0x588BD0   ; tile id 読取
5CC936 jp 0x5CCBF4       ; 全ブロック less → id98 判定へ
5CCBF4 cmp ebx,0x62(98) ; 5CCBF7 jne 0x5CCC1C
5CC936（id98）: cmp [0xB3B4C8],0 → 未確認なら 0x579C10(sRandomizeFace 確認, cb 0x5C9FC0)
5CCBE5 call 0x5C9CD0     ; 確認済み → Random 実行
```
→ **`0x5CC8BA` は id98（Random）専用の特別 act**。

### 2.5 V-593-2 応答バイト

```
5BCDE7 cmp ebx,4 ; 5BCDEA jl 0x5BCE2D
5BCE16 mov cl,[edi+0x60]   ; base（常に1）
5BCE19 add cl,bl           ; + id
5BCE1B sub cl,4            ; - 4
5BCE21 mov [eax+0xB0],cl   ; ★ 応答バイト = 1 + (id-4)
```
→ **id4→1（No）／id5→2（Yes）**。

### 2.6 V-559-3 / V-569-2（XML 実測）

`stats_menu.xml`（`Oblivion - Misc.bsa`・102,783B）:
```
stat_tabs_p1_icon: <filename> all_tabs_icons.dds </filename>
                   <cropx> 0 </cropx> <cropy> 128 </cropy> <width> 64 </width> <height> 64 </height>
stat_misc_template > stat_misc_item:
  <id> 44 </id> <width> 600 </width> <height> 32 </height>
  <y> <copy src="me()" trait="height"/> <mul src="me()" trait="listindex"/> </y>
  <listindex> 0 </listindex>
```

---

## 3. 残点（更新後）

- **V-558-4【低】**: `sGeneralFace` 等 GMST の ESM 既定値。
- **V-559-1/4【低】**: health(AV8) の p1 供給箇所／`[item]+0x34` bit0/bit3 のフラグ名。
- **V-567-2/3【低】**: Brow・Main の2箇所目重複理由／`0x5CC8BA` の更に深い分岐。
- **V-569-3/4/5【低】**、**V-570-5/6【低】**、**V-571-3/4【低】**、**V-572-2/3【低】**。
- **V-573-2・V-599-2・V-598-4・V-601-1【低】**: 動的領域/実用価値低（打切り継続）。

---

## 4. 検証

- probe: **u15_1〜u15_3（3本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x589930`（`__mbscmp` 名完全一致木探索・`ret 4`）／`0x5C4340`（`cmp eax,-1`→カウンタ）／`0x5CCA77`（名前検証＋`'%s %s?'`）／`0x5CC8BA`〜`0x5CCBF4`（id98 Random act）／`0x5BCDD0`（`[+0xB0]=base+id-4`）／`0x5C7070`／`0x5C5C30`。
- 実物 XML: `stats_menu.xml`（`stat_tabs_p1_icon` cropx=0/cropy=128・`stat_misc_template` y=height×listindex）。
