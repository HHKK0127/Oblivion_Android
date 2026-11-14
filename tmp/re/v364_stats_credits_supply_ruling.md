# V364: StatsMenu/CreditsMenu のコンテンツ供給元と構造 — #133

- 日付: 2026-11-14 / タスク: ai-todo **#133**【RE レーン依頼】原本オーバーレイ画面のコンテンツ供給元と構造の特定（①StatsMenu ②Credits）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）＋ bsa 抽出物 `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\menus\`（**ユーザー提供済みの既存抽出物を読み取りのみ**・新規 bsa 展開なし）
- 出典: #133 本体 ／ 先行: `v363_racesex_cancel_exit_ruling.md`（#130・menu id/vtable 基盤）、`b07_vtable_index.md`（vtable 索引）、v352（TESFullName 契約）
- 手法: capstone 5.0.7（v307_common）／RTTI（COL→pTD=+0xC）／u32 xref 走査／**正しいセクション VA 変換での文字列検索**（PowerShell の file-offset 換算は section padding でズレる罠を自己発見→v364_2 で是正）／XML 直読
- 中間証跡: `%TEMP%\opencode\v364_1..v364_5.py/.out`（**全 EXIT=0**）
- 制約: git/docs/GT/台帳・session-memory・既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、EXE/抽出 XML 読み取り専用、L1

---

## §0 判定サマリ

| 項目 | 判定 |
|---|---|
| **② Credits 本文の供給元と表示方式** | **[確定]** 本文＝**外部 `Data\Credits.txt`**（EXE 内に本文文字列は不在＝[確定負]）を EXE が読み込み、**1行パーサ `0x59CB70`**＋アイテム生成 `0x59CC00`＋**per-frame スクロール `0x59CA70`（frame delta `[0xB33E9C]`）で自動クローズ**。加えて **`CreditsMenu.bik`（動画）** と `Textures\Menus\Credits\%s`（テクスチャ）も供給。枠＝XML の空 `credits_rect`（id1・clipwindow） |
| **① StatsMenu の構造・menu ID・表示項目の供給元** | **[部分確定]**: 構造（5タブ・menu id **0x3EB**・タブ通知 id1..5）は**[確定]**、値の供給チェーン（AV getter→trait→XML copy）は**[確定(命令)]**、ただし XML trait と EXE trait の完全対応表・一部リスト正体は**[推定/未確定]**（V-364-3/4） |

### 副次成果（#130 の残点を bsa 抽出物でクローズ）
`options\main_menu.xml` のボタン定義から **#130 V-363-3 を解消**: **id2=`main_continue`(_continue)→cmd2=Continue [確定(XML)]**（EXE の `0x465860` ロード系実装と完全整合）／id3=main_new／id4=main_load／id5=main_options／id6=main_credits／**id7=`main_exit`(_exit)→cmd7=Exit [確定(XML)]＝#130 の「cmd7=Quit[確定(振る舞い・排除法)]」を XML 文字列で追加裏付け**。

---

## §1 XML 構造（bsa 抽出物・[確定(XML)]）

### 1.1 stats_menu.xml（102,783B / 3,478行 / `<menu name="StatsMenu">` `<class> &StatsMenu;`）

- ルート traits: `<user0>` = **current visible tab page**、色定義 `_red/green/blue_under/even/over`（**値 vs base の色分岐用**）、`menufade 0.25`。
- **5タブ構成**: `stat_p1`（race/birthsign/class/level/health/magica/fatigue_layout＋`stat_faction_item`/`stat_misc_item`）／`stat_p2`（**8属性**: strength/intelligence/willpower/agility/speed/endurance/personality/luck_layout・各 `<id>`＝24=intelligence 等）／`stat_p3`（minor_label＋window）／`stat_p4`/`stat_p5`（up/down/window/pane＝スクロール付きスキルリスト）／`stat_tabs`(prev/next＋**stat_tabs_p1..p5 の `<id>` が 1,2,3,4,5**)。
- **数値の供給形**: 例 `stat_p2_strength_layout`(id 24): 行位置 `y = me().user0 × 67 + 87`、値＝`<string><copy src="stat_p2" trait="user1"/>`、色＝`onlyif user1 lt user9 → _red_under / eq → _red_even / over → _red_over`。→ **属性値は `stat_p2` rect の trait user1..user9 に供給**（user9＝base 比較値 [推定]）、ラベルは `strings()` trait `_strength` 等（＝**`menus\strings.xml` ローカライズ**）。

### 1.2 credits_menu.xml（942B）

- `<class> &CreditsMenu;`・`stackingtype=&no_click_past;`・`alpha 0`・`locus &true;`。
- `background`（`shared_empty.dds` 全画面 RGB 8/8/20 濃紺）＋ **`credits_rect`（id=1・width0/height0・clipwindow &true;・x=640）＝本文領域は空＝EXE が動的生成**。

### 1.3 main_menu.xml（#130 副次）
`main_continue`(id2)／`main_new`(3)／`main_load`(4)／`main_options`(5)／`main_credits`(6)／`main_exit`(7)＝`strings()` `_exit`。**id8**＝_pressstart 帯、**id9**。

---

## §2 ② Credits 供給元チェーン【判定: [確定]】

```
[外部] Data\Credits.txt ──(xref 0x59D07C 読込帯 0x59CExx..0x59D1xx)──┐
[外部] CreditsMenu.bik（+ini 'CreditsMenuMovie:General' @0xA6B7DC 帯）┤
[XML ] credits_rect(id1, clipwindow) ← レイアウト枠                    │
        ↓                                                                │
0x59CB70 = 1行パーサ [確定(命令)]: [esi+0x38]=pos / +0x30=cur / +0x34=end、
        0x0D(CR) で行分割、'<'(0x3C)/'>'(0x3E)/'*'(0x2A) はタグ扱いで除外、
        行を [esp+0x10] バッファへコピー → 0x4028d0 で string 化 → ret 4
0x59CC00 = アイテム生成 [推定高]: new(0x30)/new(0x20)/new(0xC)、
        pos(float)→vec3 レイアウト（fild/neg/st パターン）
0x59CA70 = CreditsMenu slot11 per-frame [確定(命令)]:
        条件系 0x4031E0/0x403520(0x1d)/0x6DA150(9)==2 → 不成立なら 0x59C9F0(閉鎖)
        [esi+0x2C] scroll obj の位置 += vel × **[0xB33E9C]（frame delta・v222[確定]）**
        → 0x707370(obj, newpos, 0, 1) 位置更新 → 終端超過 → **jmp 0x59C9F0 自動クローズ**
        （0x59C9F0 = FindMenu(0x417) 破棄＋MainMenu 操作ヘルパ・v363 で同定）
0x59D12E = 'Credits Scroll Root'（xref）＝スクロールルート要素名 [確定]
0x59CEE6 = 'Textures\Menus\Credits\%s'（xref）＝行アイコン/テクスチャ書式 [確定]
（関連）'Data\Credits.txt' @0xA6B7F0、'CreditsMenu.bik' @0xA6B7CF（[data]0xB0309C 参照＝設定/ID）
```

- **EXE 内にクレジット本文文字列は不在 [確定(負)]**: `'Bethesda'` の正確 VA 実在は `Software\Bethesda Softworks\Oblivion`（レジストリキー @0xA30FEC 帯）と `CLIENT.Ph.Bethesda_Oblivion`（Havok プロパティ @0xA95BD0 帯）の2件のみ、`'Bethesda Game'`=0 件（v364_3/4）。→ 本文はすべて**外部ファイル供給**（テキスト＝`Data\Credits.txt`、映像＝`CreditsMenu.bik`）。
- **#98 F3（クレジット内容の差分裁定）への回答**: ポート側はこの2外部リソースを再現すべき。EXE 内文字列を移植しても原本と一致しない [確定]。

---

## §3 ① StatsMenu の構造と表示項目の供給元【判定: [部分確定]】

### 3.1 menu id と vtable【確定】

- **menu id = 0x3EB**: slot13 getter `0x5D9D60 = mov eax,0x3EB; ret`。XML root と整合（v363 で `stats_menu.xml`＝0x3EB を確定済み）。
- **StatsMenu vtable = 0xA6D9DC（15スロット・override 10個）** [確定(RTTI)]:
  - slot0=ctor/dtor `0x5D9D70`（vptr 0xA6D9DC 書込＋`0x585330` 基底）
  - **slot1=SetElement `0x5D9BE0`（id==0x0A 分岐＝name editor 要素、RaceSex slot1 と同型）**
  - **slot3=通知ハンドラ `0x5DCD70`**
  - slot4=`0x5DBBD0`（SEH 大関数）、slot5=`0x5D9FE0`（id 到達判定 `cmp0x22`）、slot8=`0x5D9DA0`（**notification id 0x21/0x2B/0x35 分岐**→trait 0xFB0/0xFAF→0xFCA 比較）、slot10=`0x5DA050`、slot11=per-frame `0x5DA180`、slot14=`0x5DC520`
- CreditsMenu vtable = **0xA6B784（override 5個）**: slot0=dtor `0x59C9D0`（内部 `0x584740` 系クローズ＋MainMenu 相関＝v363）／slot1=SetElement `0x59C930`（id==1 → `[ecx+0x28]`＝**credits_rect(id1) 要素格納**）／slot11=per-frame `0x59CA70`（§2）／slot13=`0x59C950 = mov eax,0x417; ret`／slot14=`0x59CB30`（id∈{-1,0,1,2,3,4} は即 reject、他は値>=1 判定で `0x59C9F0` 呼出＝**解除系**）[確定(命令)]。

### 3.2 slot3 `0x5DCD70`＝タブ切替通知ハンドラ【確定(XML+命令)]

- `lea eax,[ebx-1]; cmp eax,4; ja` → **通知 id 1..5 ＝ `stat_tabs_p1..p5` の `<id>1..5`（XML で確定）＝タブ切替**。
- 各 id の更新 fn（callers 4/4/3 で裏付け）:
  | 通知 id | fn | 内容 [判定] |
  |---|---|---|
  | 1 (tab p1) | `0x5DBB00` | **`0x4DA2A0(player)` → SetString(elem,0xFAF,名前)**（v352 で TESFullName getter 同定）＋**`0x5EA720` → SetString(0xFB0)**（v364_5: vfunc+0x170→[+0xE8]→[+0x18]→`len==0xFFFF`→[+4]=name ptr の **名前 getter**（TESFullName 契約と一致）＝ class/race 名 [確定(命令)]）＋ vfunc+0x268→[+0x18+4] の文字列（''=0xA2F7EC fallback） |
  | 2,3 (tab p2/p3) | **`0x5DA1A0(-1)`** | **値の再計算本体**（§3.3） |
  | 4 (tab p4) | `0x5DC950` | **`__RTDynamicCast(from .?AVTESBoundObject@@ 0xB03248 → .?AVTESNPC@@ 0xB02FB4)`** → `[ebx+0x44]` element `0x5893F0` → `[esi+0x3C]` リスト walk → 行生成 [部分確定: NPC の何のリストかは offset 解釈のみ＝V-364-3] |
  | 5 (tab p5) | `0x5DA930` | `[esi+0x50]` element trait0xFB5→int → `[esi+0x4C]` clear(0x5893F0) → **`[0xB383E8]`（BSS 名前テーブル[未確定]）＋`[0xB332E0]`＋`0x5DA8C0` で行構築** [部分確定] |
  | 8,9 | （共通 tail） | `0xFAE` 取得→int→`esi∈[1,5]` 検証→同 fn 群＋`jmp 0x5D9CB0`（共通更新: `0x65FD00` 判定→trait 0xFAF/0xFB0 SetFloat） |

- 付随: slot3 内 `FindMenu(0x41B)`→`Data\Menus\Dialog\TextEditMenu.xml` 生成（RTTI cast `.?AVTextEditMenu@@`）＝**StatsMenu は TextEditMenu(0x41B) を内部生成** [確定(命令)]（v352 の「TextEditMenu は ClassMenu 専用」記述は**訂正が必要**: StatsMenu も使用 [確定] — v352 §4.1 の注記へ波及）。

### 3.3 `0x5DA1A0`＝AV 値→trait の供給本体【確定(命令)】

- 引数 `edi`（-1=全再計算 / 指定 index）。**AV index スイッチ（`edi==0..10`）**:
  - **`0x65D270(..., idx, 0)` ＋ vfunc `edx(idx)`（＝`+0x284` 系 GetActorValue）で現在値取得**、**`0x5F1910(idx)` で base/関連値取得** → `[esi+0x28]` element へ **trait 2系統を SetFloat(`0x58CEB0`)**:
    | AV idx | 現在値 trait | base/関連 trait |
    |---|---|---|
    | 0..7 | 0xFAF,0xFB0,0xFB1,0xFB2,0xFB3,0xFB4,0xFB5,0xFB6 | 0xFB7,0xFB8,0xFB9,0xFBA,0xFBB,0xFBC,0xFBD,0xFBE |
    | 9 | 0xFB6 | 0xFB7＋0xFBB |
    | 10 | 0xFB8 | 0xFB9＋0xFBC |
  - **AV 0..7 ＝ 8属性（STR..Luck）→ これが XML の `stat_p2 user1..9` 比較（`userN lt user9` 色分岐）の供給元** [確定(命令)]（対応表の user 表記は V-364-4）。
- **21(0x15) ループ**: `ebx∈[0x14]` 制御下で **`0x65FCA0(idx)`＋`0x51C090(player, idx)`（skill 所持/種別判定）** → trait 0xFAF/0xFB0/0xFB1/0xFB5 へ → **スキル（21個）の p3/p4/p5 表示供給** [確定(命令)]。`0x5E3320`（player 関連オブジェクト取得）も併用。
- 値 getter 個数: `0x65D270`（AV current）、`0x5F1910`（AV base 系）、vfunc+0x284 [確定(命令)]。
- **per-frame ガード slot11 `0x5DA180`**: `0x57C140(0x3EB)`（menu stack 検査系）＋`[0xB3B43D]` flag → 成立時 `jmp 0x5C1000`（メニュー per-frame 帯へ tail）[確定(命令)]。
- **slot10 `0x5DA050`**: `[edi+0x30/0x34/0x38]` の trait0xFA1 比較→`[edi+0x40/0x48/0x50]` 選択 element → 各 trait0xFB5→**0x77777777 定数除算（÷100）**→ trait0xFB3 へ＝**タブ窓切替時の百分率/値再整形** [推定高]。

### 3.4 供給元の総括（#133① の回答）

| 表示項目 | 供給元 | 判定 |
|---|---|---|
| 属性/スキル **ラベル** | `strings()` trait `_strength` 等＝**`menus\strings.xml`（ローカライズ）** | [確定(XML)] |
| **属性値＋base** | **EXE `0x5DA1A0`: AV getter `0x65D270`/vfunc+0x284 と base `0x5F1910` → stat_p2 element の trait 0xFAF..0xFBE へ SetFloat** → XML `<copy src="stat_p2" trait="userN">` が読む | [確定(命令)] |
| **色分岐（red/green/blue）** | XML の `onlyif userN lt/eq user9` ＋ menu rect の `_red.._over` 定数 | [確定(XML)] |
| **スキル行** | EXE `0x5DA1A0` 21ループ（`0x51C090` 判定）＋ `0x5DA930`（p5 行構築・名前テーブル `[0xB383E8]`） | [部分確定] |
| p1 の名前/class/race 文字列 | `0x5DBB00`: `0x4DA2A0`（TESFullName）＋`0x5EA720`（obj 名前 getter）→ trait 0xFAF/0xFB0 | [確定(命令)] |
| p4 のリスト | `0x5DC950`: TESNPC cast＋`[+0x3C]` walk | [部分確定]（V-364-3） |
| 行位置 | `y = me().user0×67+87` 等（XML） | [確定(XML)] |

---

## §4 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe | v364_1..5 **全 EXIT=0**。v364_1 の未定義関数参照（`linear_from`）を**実行前に自己発見・修正** |
| 2 | 文字列 VA の罠 | PowerShell の file-offset→VA 換算が section padding で**ズレ（'Data\Credits' が keyword 帯と誤照合）**→ v364_2 で SECTIONS 変換付き再検索で是正（正確 VA 0xA6B7F0）。**今後の教訓: 文字列検索は必ずセクション VA 変換経由** |
| 3 | 不存在証拠 | `'Bethesda Game'`=0 件・既存 hits 2件はレジストリキー/Havok 名と文脈実測で裏付け（不在証拠を境界明示付きで採用） |
| 4 | head 特定不能 | credits 読込帯 0x59CEE6/0x59D07C/0x59D12E は CC ラン不足＋ドリフトで head=0／cover=False → **xref 站地帯（0x59CExx..0x59D1xx）までを [確定]、fn 境界は [未確定] として残点化**（推測で head を断定しない） |
| 5 | RTTI 突合 | TESBoundObject/TESNPC/TextEditMenu の TD 名を str_at(+8) で実測（b07 索引と整合） |
| 6 | XML↔EXE 突合 | タブ id1..5（XML）⇔ slot3 switch（EXE）／`stat_p2 user1..9`（XML）⇔ trait 0xFAF..（EXE）を対応付けてから判定 |

---

## §5 残点（本裁定で新規）

| ID | 内容 | 深刻度 |
|---|---|---|
| V-364-1 | [未確定] credits 読込帯（xref 0x59D07C/0x59CEE6/0x59D12E）の fn 境界と、行アイテムの生成/保持構造（0x59CC00 の归属・[esi+0x2C] scroll obj の構築元） | 低（外部ファイル供給は [確定] 済） |
| V-364-2 | [未確定] `[0xB383E8]`/`[0xB332E0]`（真 BSS・zero-init）の実体 — `0x5DA930` の名前テーブル供給源（data セクション外で初期値取得不能） | 低 |
| V-364-3 | [推定] `0x5DC950`（tab p4）の `[TESNPC+0x3C]` リストの正体（faction/relation/misc のどれか）と p4 表示内容の対応 | 低 |
| V-364-4 | [推定] XML `stat_p2 user1..9` と EXE trait `0xFAF..0xFBE` の完全対応表（user9=base 比較値 [推定]、user1=0xFAF? の対応は順序推定） | 中（#127 配線で要） |
| V-364-5 | [未確定] CreditsMenu slot1 `0x59C930` の `[ecx+0x28]`（credits_rect id1 要素）の消費者（本文生成側がどこからこの rect を得るか） | 低 |
| （旧）V-352-1 系 | **v352 §4.1 の「TextEditMenu は ClassMenu 専用」は本裁定で訂正**: StatsMenu も `Data\Menus\Dialog\TextEditMenu.xml` を生成 [確定] | 波及注記 |

### #130 への残点クローズ（副次）
- **V-363-3 [クローズ]**: cmd2=**Continue**（`main_continue`・XML `_continue`）・cmd7=**Exit**（`main_exit`・XML `_exit`）＝[確定]。#130 の「cmd2=ロード/開始系[推定]」は **Continue（直近セーブロード＝`0x465860`）に確定昇格**。

---

## §6 main への報告

1. **【②Credits＝[確定]】クレジット本文の正体は `Data\Credits.txt`（テキスト）＋ `CreditsMenu.bik`（動画）の2外部リソース**。EXE は読込（0x59D07C 帯）→行パーサ（0x59CB70）→アイテム生成（0x59CC00）→**per-frame スクロール（0x59CA70・frame delta 0xB33E9C・終端で自動クローズ 0x59C9F0）**。枠は XML の空 `credits_rect`（id1/clipwindow）。**EXE 内に本文文字列なし [確定負]** → **#98 F3 の回答: ポートはこの2ファイルを再現すべき（EXE 文字列移植では不一致）**。textures は `Textures\Menus\Credits\%s` 書式。
2. **【①Stats＝[部分確定]】menu id=0x3EB・5タブ（タブ通知 id1..5＝XML で確定）**。値供給: **AV getter（0x65D270/vfunc+0x284）と base（0x5F1910）→ stat_p2 element の trait 0xFAF..0xFBE へ SetFloat（0x5DA1A0）**、スキルは 21 ループ（0x51C090 判定）、p1 文字列は TESFullName getter（0x4DA2A0/0x5EA720）。ラベルは `menus\strings.xml`。**#127（UICharacterSheet 配線）の正典材料は §3.4 の表**。残り（user 対応表・p4 リスト正体・名前テーブル）は V-364-2/3/4。
3. **訂正波及**: v352 の「TextEditMenu は ClassMenu 専用」→ **StatsMenu も TextEditMenu(0x41B) を生成 [確定]**（docs 反映時に #47/#20 へ）。
4. **#130 副次クローズ**: main_menu.xml で **cmd2=Continue・cmd7=Exit [確定]**（V-363-3 解消）。#130 の Exit 裁定は XML 裹付けで補強。
5. docs/GT 反映候補: menu id 表（0x3EB/0x417 ほか §1/§3.1）・Credits 供給系・Stats 特性対応 — #47/#20 一括（L1）。

---

### 参照（本タスクの証跡）

- v364_1（bsa 抽出物一覧・stats/credits XML 構造・ Bethesda/credits 文字列初走査（VA ズレあり）・両 vtable 15スロット差分＝override 列挙）
- v364_2（**正しい VA での文字列再検索: Data\Credits.txt→0x59D07C ほか xref3点**・Stats slot3 全 decode（id1..5 switch）・slot10/slot11・Credits slot11 全文（frame delta スクロール＋自動クローズ）・行パーサ 0x59CB70・アイテム生成 0x59CC00）
- v364_3（credits 読込 fn head 探索（cover 不能を記録）・'Textures\Menus\Credits\%s'/'Credits Scroll Root'/'Data\Credits.txt' xref 確定・Stats page fn 3本（0x5DBB00/0x5DC950/0x5DA930）・0x5D9CB0 共通更新・'Bethesda' 正確 VA 初走査・slot11 ガード 0x57C140）
- v364_4（**0x5DA1A0 の AV index スイッチ全文（trait 2系統の対応表）**・credits 帯 head 再挑戦・'Bethesda' 文脈確定＝レジストリキー/Havok 誤爆・0xA6DC48=TextEditMenu.xml）
- v364_5（RTTI: TESBoundObject/TESNPC/TextEditMenu TD 実名・BSS 初期値取得不能を記録・find_head cover=False 記録・0x5EA720=名前 getter 同定）
