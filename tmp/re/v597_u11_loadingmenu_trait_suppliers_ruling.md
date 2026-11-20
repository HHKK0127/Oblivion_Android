# v597 — U11 裁定: LoadingMenu の trait 供給元（user0〜user5）と 3 メニューの使い分け（ai-todo #96 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u11_1〜u11_14**・全 EXIT=0・冒頭 Test-Path）＋実物 XML（`Oblivion - Misc.bsa` から抽出）。
先行: `docs/RE_EXE_REPORT.md` §32.5（5255-5300）／`tmp/re/v557_title_video_ruling.md`／`tmp/re/v596_u10_screen_virtual_resolution_ruling.md`
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| ⒜ user0（スライドショー段数） | **[確定(命令)]** | 書き手＝**`0x5AD780`（`0x5AD7C7` で `0xFAE` を読む → `0x5AD914`-`0x5AD939` で `0xFAE += 1.0`）** と **`0x5AD440`（`0x5AD44E`-`0x5AD45C` で `0` にリセット）**。`0x5AD780`＝**ロード対象ごとに user0 を +1**（`0x5AD7D4` で現在値を読み、末尾 `0x5AD939` で `+1.0` を `SetFloat(0xFAE)`）＝**0=タイトルページ、>0=ロード画面**。遷移条件＝**`0x5ADCF0`（`loading_menu.xml` 再構築＋初期化）で `0`、`0x5AD780`（次ロード対象へ進む）で `+1`**。 |
| ⒝ user1（背景画像） | **[確定(命令)]** | 書き手＝**`0x5AD780`（`0x5AD88E`: `push 0xFAF` → `0x58CED0`＝SetString）**。値＝**`'Menus\Loading'`（`0xA6C4B8`）＋ロード対象の名前**（`0x5AD846`-`0x5AD853` の `_strstr(name, 'Menus\Loading')` が偽のとき `0x5AD85F`-`0x5AD87E` で `'%s%s'`＝`'Menus\Loading' + ファイル名` を sprintf）。**固定ではなくロード対象ごとに変わる**（XML 既定値 `Menus\Loading\load_in_game_default.dds` は初期値）。 |
| ⒞ user2（表示文字列） | **[確定(命令)]** | 書き手＝**`0x5AD780`（`0x5AD906`: `push 0xFB0` → `0x58CED0`）**。値＝**ロード対象の `DESC`（説明）文字列**（`0x5AD8A1`/`0x5AD8BD` で `0x43534544`=`'DESC'` を record の名前として取得 → `0x5AD8D0` で文字列コピー → `0x5AD8EA`-`0x5AD8F7` で末尾の改行 `0x0a` を除去 → `SetString(0xFB0)`）。**`sLoading` 系 GMST ではなくロード対象レコードの説明文**。XML 既定値 `strings()._missing` は初期値。 |
| ⒟ user3（進捗バー現在値） | **[確定(命令)]** | 書き手＝**`0x5AD980`（`0x5ADA7E`-`0x5ADA8E`: `push 0xFB1` → `0x58CEB0`＝SetFloat）**。値＝**`[LoadingMenu+0x60]`（現在の完了セクション数、0..100 スケール）**。更新は**ロード段階数ベース**（`[+0x60]` はセクション完了数。`0x5ADB40` が `[+0x60]` を `100` まで回して `0x5AD980` を繰り返す＝**段階が完了するたびに進む**）。実時間ベースではない。 |
| ⒟ user4（進捗バー最大値） | **[確定(命令)]** | XML 既定値 **`100`** のまま（`0xFB2` を書く LoadingMenu 内サイトは**0件**。`loading_menu.xml` の `<user4> 100 </user4>` が唯一の供給元）。`0x5AD980` の進捗計算は `[+0x60]`/`[+0x3c]`（最大100）を使う。 |
| ⒠ user5（デバッグ文字列） | **[確定(命令)]** | 書き手＝**`0x5ADCF0`（`0x5ADE12`-`0x5ADE21`: `push ' '`(0xA36430) → `push 0xFB3` → `0x58CED0`）**＝**ロード開始時に空白1文字へ初期化**。実行時は `0x5ADEC0`（`0x5ADF6A`: `'Loading Bar Section %d %d0%%'`）が **`0x404EE0`（コンソール出力）で表示するセクション進捗**を伴う。 |
| ⒟ 3 メニューの状態機械 | **[確定(命令)]** | **`loading_menu.xml` のみが `0x590420` でロードされる**（`0x5ADCF0` の `0x5ADD2D`）。**`loading_ingame_menu.xml` / `loading_bar_ingame_menu.xml` は EXE にリテラルが存在せず、ロードサイトも 0 件 [確定負]**＝**原版は未使用**（BSA 内に同梱されるがコードから参照されない）。起動時（タイトル前）もゲーム内ロードも**同じ `loading_menu.xml`（menu id `0x3EF`）**を使い、**user0 の値で `load_title_page`（=0）と `load_main`（>0）を切り替える**。 |
| ⒠ `strings()._missing` の実文字列 | **[確定(XML)]** | **`" Missing Text "`**（`menus\strings.xml` L190 `<_missing> Missing Text </_missing>`・`Oblivion - Misc.bsa` 抽出）。参考: `_loading`＝**`" Loading... "`**（同 L28）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 LoadingMenu の同定

- menu id **`0x3EF`（1007）**。`0x589B70(id)`＝FindMenu、`0x5898F0(menu)`＝**基底メニュー取得**（`0x3EF` の具象オブジェクト）。
- ローダ **`0x5ADCF0`**: `0x589B70(0x3EF)` で既存を取得 → `[vtable][0]`（`0x5AE060`＝dtor）で破棄 → `0x5A8FD0`（他メニュー後始末）→ `0x582160(1,0)`＝InterfaceManager → **`0x590420('Data\Menus\loading_menu.xml')`** で再ロード → `0x5898F0` で基底取得 → `[vtable+0x34]()` が `0x3EF` か検証 → `0x583DF0`・`0x584880`・`0x9832E6`（RTTI cast で `TileMenu`/`LoadingMenu`）→ `0xFA5`（stackingtype）判定 → `0x5AD440`（user0 リセット）→ `0x5AD780`（次ロード対象へ）→ `0x578D50` → `SetString(0xFB3, ' ')`。
- vtable **`0xA6C40C`**（`0x5AD0D0`＝ctor で設定）: `[0]=0x5AE060`（dtor）／`[13]=0x5AD0D0`／他は基底 `TileMenu` の共通 slot。

### 2.2 user0（スライドショー段数）— 書き手 `0x5AD780` / `0x5AD440`

```
0x5AD780 (進める):
  5AD7C7 push 0xfae ; mov ecx,[edi+4] ; call 0x588BD0  ; 現在の user0 を読む
  5AD7D9 call 0x9828C0                                  ; float→int (round)
  ... (ロード対象リストを走査、DESC/ファイル名を取得)
  5AD914 push 0xfae ; mov ecx,esi ; call 0x588BD0       ; 再度 user0 を読む
  5AD920 fadd qword [0xa2f928] (=1.0)                   ; ★ +1.0
  5AD934 push 0xfae ; call 0x58CEB0                     ; ★ SetFloat(0xFAE) = user0+1
0x5AD440 (リセット):
  5AD44E fldz ; 5AD457 push 0xfae ; call 0x58CEB0       ; ★ SetFloat(0xFAE) = 0.0
```
→ **XML の `load_title_page`（`visible = user0==0`）と `load_main`（`visible = user0>0`）はこの 1 値で切り替わる**。`0x5AD440` はローダ/開始時、`0x5AD780` は各ロード対象で呼ばれる。

### 2.3 user1（背景画像）— 書き手 `0x5AD780`

```
5AD81D mov eax,[esi+0x1c]      ; record のファイル名（null なら 0xA2F7EC）
5AD842 mov ebp,[esp+0x18]      ; ロード対象名
5AD846 push 'Menus\Loading' (0xA6C4B8)
5AD84B push ebp ; 5AD853 call 0x9840E0 (_strstr)   ; 名前が 'Menus\Loading' を含むか
5AD85D jne 0x5AD88A            ; 含む → そのまま
5AD86B push eax ; 5AD86E call 0x4F96F0            ; 含まない → ファイル名取得
5AD878 push '%s%s' (0xA2F884) ; 5AD87E call 0x402E30  ; 'Menus\Loading' + 名前
5AD88A mov ecx,[edi+4]
5AD88E push 0xfaf ; 5AD893 call 0x58CED0           ; ★ SetString(0xFAF) = user1
```
→ **user1＝`'Menus\Loading'`＋ロード対象名**（対象ごとに変わる）。XML 既定値 `Menus\Loading\load_in_game_default.dds` は初期値。

### 2.4 user2（表示文字列）— 書き手 `0x5AD780`

```
5AD8A1 push 0x43534544 ('DESC') ; 5AD8A9 call [record_vtable+0x10]  ; DESC サブレコード取得
5AD8BD push 0x43534544          ; 2 回目（長さ計算）
5AD8CC.. 文字列をローカルへコピー
5AD8E2 mov cl,0xa ; 末尾の改行 0x0a を 3 段まで除去 (0x5AD8E4-0x5AD8F7)
5AD906 push 0xfb0 ; 5AD90B call 0x58CED0            ; ★ SetString(0xFB0) = user2
```
→ **user2＝ロード対象レコードの `DESC`（説明文）**。XML 既定値 `strings()._missing`＝`" Missing Text "` は初期値。

### 2.5 user3/user4（進捗バー）— 書き手 `0x5AD980`（段階ベース）

```
0x5AD980 (進捗ポンプ):
  5AD9CA mov ebp,[edi+0x10]        ; 経過時間
  5AD9CD sub eax,[esi+0x68] ; 5AD9D4 sub ecx,[esi+0x6c]
  5AD9D7 cmp eax,[0xb14160](=2500) ; セクション滞留時間しきい値
  5AD9DF add [esi+0x64],1          ; セクション内ステップ++
  5ADA4D mov eax,[esi+0x64]        ; 現在セクション
  5ADA50 mov ecx,[esi+0x60]        ; 完了セクション数
  5ADA57.. 完了判定
  5ADA6F mov ecx,[edi+0x10] ; 5ADA72 [esi+0x68]=ecx  ; 基準時刻更新
  5ADA75 [esi+0x60]=eax            ; ★ 完了セクション数を進める
  5ADA78 fild [esi+0x60]           ; 
  5ADA89 push 0xfb1 ; 5ADA8E call 0x58CEB0  ; ★ SetFloat(0xFB1) = user3 = 完了セクション数(0..100)
```
`0x5ADB40`（全セクション強制完了）: `[esi+0x60]<100` の間 `0x5AD980` を繰り返す → `[+0x60]=100`。`0x5ADEC0`（セクション開始）: `fPercentageOfBar0..3`（INI `[LoadingBar]`）を重みに `[+0x3c]`（最大100）を算出し `0x5AD440` → `0x57E150`。
→ **user3 は「ロード段階（セクション）の完了数」**（実時間ではない）。**user4 は XML 既定 `100`**（`0xFB2` を書く LoadingMenu サイトは 0 件）。

INI `[LoadingBar]` GMST（`0xB14138`〜・`{value, nameptr}` 対）:
```
0xB14138 = 0.06f  → 'fPercentageOfBar0:LoadingBar'
0xB14140 = 0.35f  → 'fPercentageOfBar1:LoadingBar'
0xB14148 = 0.44f  → 'fPercentageOfBar2:LoadingBar'
0xB14150 = 0.15f  → 'fPercentageOfBar3:LoadingBar'
0xB14158 = 15.0f  → 'fLoadingSlideDelay:LoadingBar'
0xB14160 = 2500   （セクション滞留しきい値・int）
```

### 2.6 user5（デバッグ文字列）

```
0x5ADCF0: 5ADE12 push ' '(0xA36430) ; 5ADE1A push 0xfb3 ; 5ADE21 call 0x58CED0  ; ★ user5 = ' '
0x5ADEC0: 5ADF6A push 'Loading Bar Section %d %d0%%' ; 5ADF74 call 0x404EE0       ; コンソール表示
```

### 2.7 3 メニューの使い分け（状態機械）

```
0x590420 の全 49 caller のうち LoadingMenu 系は 0x5ADCF0 のみ。
  0x5ADCF0: push 'Data\Menus\loading_menu.xml' (0xA69B6C) ; call 0x590420   ; ★ 唯一
'loading_ingame_menu.xml' / 'loading_bar_ingame_menu.xml' の .rdata リテラル = 0 件 [確定負]
```
→ **原版は `loading_menu.xml` のみ使用**。`loading_ingame_menu.xml`（user0=1 固定・独自レイアウト）と `loading_bar_ingame_menu.xml` は BSA に同梱されるが**コードから参照されない**（＝未使用アセット）。起動前・ゲーム内の区別は **user0 の 0/1 切替**で行う。

### 2.8 `_missing` / `_loading` の実文字列（`menus\strings.xml`）

```xml
L28:  <_loading> Loading... </_loading>
L190: <_missing> Missing Text </_missing>
```
（`Oblivion - Misc.bsa` 内 `menus\strings.xml`。`0x581CC0` の `0x590420('Data\Menus\strings.xml')` で読込まれ `[InterfaceManager+0x6c]` に保持＝v596 §2.1 の `strings()`。）

### 2.9 呼出側（いつ使われるか）

- `0x578E10`（`0x578E21: jmp 0x5ADCF0`）＝**ローダの入口**。caller `0x587550`（窓 id switch の case 0x3EF）。
- `0x578E30`（callers: `0x440AF0`／`0x4455E0`／`0x445A10`）＝**既存 LoadingMenu を破棄→`0x57CFE0`（スタック操作）**。`0x578E90`（callers: `0x440AF0`／`0x5BDDE0`／`0x66F420`）＝**`0x5AD380`（LoadingBar 初期化）**。
- `0x57E150`（callers: `0x57CAC0`＝`0xFAE`(user0) チェック付きメニュー切替／`0x578F20`／`0x5ADEC0`）＝**「ロードを進める」ポンプ**（`0x5AD980`×8 → 各メニューの `0xFA1`(visible)=1）。
- `0x5AD410`（`0x540600` から）＝**LoadingMenu が有効か（`0x3EF` 存在＋基底あり）**の判定。

---

## 3. 移植への指針

- **`loading_menu.xml` のみ実装すればよい**（`loading_ingame_menu.xml`/`loading_bar_ingame_menu.xml` は原版未使用）。
- trait 供給: `user0`＝スライドショー段数（0=タイトルページ、>0=ロード。ロード対象ごとに +1）／`user1`＝`'Menus\Loading'`+対象名／`user2`＝対象レコードの `DESC`／`user3`＝完了セクション数(0..100)／`user4`＝100 固定／`user5`＝デバッグ文字列（既定 `" "`）。
- `_missing`＝`" Missing Text "`、`_loading`＝`" Loading... "`。
- 進捗は**段階（セクション）ベース**。INI `[LoadingBar]` の `fPercentageOfBar0..3`（0.06/0.35/0.44/0.15）で各セクションの重み、`fLoadingSlideDelay`(15) で滞留を決める。

---

## 4. 残点

- **V-597-1【低】**: `0x5AD980` の `[esi+0x64]`（セクション内ステップ）と `0xB14160`(2500) の厳密な時間単位（ms か tick か）は未確定（`0x47DF80`＝乱数、`0x9828C0`＝float→int round）。
- **V-597-2【低】**: `0x5AD780` のロード対象リスト（`[edi+0x4c]` の連結リスト）の構築元（`0x5AD440` が `0x65C620` で要素を追加）。ロード対象名の実データ供給元（`0x4F96F0`＝record 名 getter）は静的 RE 範囲外。
- **V-597-3【低】**: `0x5ADEC0` の `[esi+0x28]`（セクション順序）と「LoadingMenu sections loading out of order」警告の順序規則。
- **V-597-4【低】**: `0x57CAC0` の `0x1771` trait（`0x588BD0` で読み `0x9828C0`）の正体（v596 で `0x581CC0` が `0x1771` に `0xA68C00` を設定＝別用途）。

---

## 5. 検証

- probe: **u11_1〜u11_14（14本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: menu id `0x3EF`／ローダ `0x5ADCF0`（`0x590420('Data\Menus\loading_menu.xml')`）／vtable `0xA6C40C`／user0 書込 `0x5AD780`(`0x5AD939` +1.0)・`0x5AD440`(`0x5AD45C` =0)／user1 `0x5AD88E`（`'Menus\Loading'`+名前）／user2 `0x5AD90B`（`'DESC'`）／user3 `0x5ADA8E`（`0xFB1`）／user5 `0x5ADE21`（`' '`）／INI `[LoadingBar]` GMST `0xB14138`〜（`fPercentageOfBar0..3`＝0.06/0.35/0.44/0.15・`fLoadingSlideDelay`＝15）。
- 実物 XML 抽出: `Oblivion - Misc.bsa` → `menus\strings.xml`（`_missing`＝`" Missing Text "`・`_loading`＝`" Loading... "`）・`menus\loading_menu.xml`・`menus\loading_ingame_menu.xml`・`menus\loading_bar_ingame_menu.xml`。
- 反証: `loading_ingame_menu.xml`/`loading_bar_ingame_menu.xml` の `.rdata` リテラル＝**0 件**（[確定負]）。
