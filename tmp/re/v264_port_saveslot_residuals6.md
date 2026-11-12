# v264_port_saveslot_residuals6.md — 移植セーブ経路残点第6束（R-4・R-5 裁定）

- 作成: 2026-11-03 / TODO serial 252（V252・低）/ **分析専任（docs/GT/TODO/ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v253_port_saveslot_residuals5.md` §5-2 新規残点 R-4/R-5（本体は読取のみ・無編集）。
- 入力（読取のみ）: `v253_scan1..9(_out)`（FUN_00591C50/FUN_00584980/パラメータ機構の実測）、
  `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\menus\`（メニュー XML・ESM でないため読取可）、
  `tmp/re/cache/named_funcs.txt`, `docs/Oblivion_decompiled.c`（部分検索）。
- 手法: capstone 5.0.7 再逆アセンブル＋PE 手動解析（v253_scan1 と同一の va2off/rd32/cstr 枠）。
  新規スクリプト `%TEMP%\opencode\v252_scan*.py`。ESM 本体は未アクセス。git 不使用。
- selftest: 着手時／完了時の 2 回実行、いずれも **ALL PASS (4/4)**（`probe_harness.py selftest`）。
- 状態: **完了**（§1→§2→§3 を各件終了のたびに即時追記）。

---

## §1 項目1（R-4）: menu.xml 系パラメータ 4005/4011/4061/4062 の業務名称・値域

**判定: 【解明】4 件とも EXE 内の trait 名⇔コード登録表（`FUN_0058A0A0` による名前→u16 コード辞書、
@0x58A9A6..0x58AE8B の大量 `push code / push name / call 0x58a0a0` ブロックで機械的に抽出）で名称が確定した。
- 4005 (0xFA5) = **`stackingtype`**（メニューのクリック積み重なり制御。XML の `<stackingtype>` タグそのもの）
- 4011 (0xFAB) = **`depth`**（Z 方向重ね順。`<depth>` タグ。v253 §2-2 の「パネル高さ様寸法」は誤りで **depth（重ね層数）の再計算**）
- 4061 (0xFDD) = **`mouseover`**（ホバー状態フラグ）
- 4062 (0xFDE) = **`string`**（テキスト内容。 setter `FUN_0058CA00` が通知/再適用を special-case してた理由 = 文字列は float 通知対象外）
「名称は EXE 外 XML 側のみ」という v253 R-4 の前提は**誤り**（XML タグ名は EXE 側登録表に原語で存在）。
4005 のしきい 102/103/6006/6007 は「コード参照説」ではなく **stackingtype の列挙値帯**（実体値は
`&click_past;=101 / &no_click_past;=102 / &does_not_stack;=6008` と登録表 @0x58A215..0x58A24A で確定、
§1-3）。**

### §1-1 名前⇔コード辞書の機構 [確定]

- `FUN_0058A0A0(code, name)`（`ret 8`）: 名先頭が `'&'`(0x26) ならアンパサンド名（`&MagicPopupMenu;` 等のメニュー名）
  専用のリスト 0xB13BC4 へ、通常名は頭文字で 26 バケット（0xB3B0B0 + 頭文字*0x16... 実際は `shl ecx,4` で
  16B/バケット）のハッシュチェインへ **(code, name) を登録**（@0x58A0D8 `'&'` 分岐、@0x58A10E `'_'`/A-Z 正規化、
  @0x58A15C/0x58A193 でテーブル先頭書込）。
- 逆引き `FUN_00589080(code)`: code ≥ 10000 は動的 name table（0xB13BC8 配列）、未満は 0xB3B0B4..0xB3B274 の
  辞書バケットを walk してエントリ+8 の名前文字列を返す（@0x5890C5..0x5890FF 実測）。
- `FUN_0058BEE0`（パラメータ再適用側）@0x58C069: `cmp ax, 10000` で **コード ≥10000 を「動的ユーザー名」枠**として
  名前一部（先頭 `_` を除去）→ `0x983479`（数値化）へ流す — 4001..4063 の静的コードとは別系列。
- 登録ブロック（`0x58A9A6..0x58AE8B`、1 エントリ 15B `push code; push name; mov ecx,esi; call 0x58a0a0`）から
  復元した全表（`v252_scan3` A 節・文字列解決済み）:

| code(10進) | 16進 | 名前（XML タグ） | code(10進) | 16進 | 名前 |
|---|---|---|---|---|---|
| 4001 | FA1 | visible | 4042 | FCA | height |
| 4002 | FA2 | class | 4043 | FCB | width |
| 4003 | FA3 | listclip | 4044 | FCC | red |
| 4004 | FA4 | clipwindow | 4045 | FCD | green |
| **4005** | **FA5** | **stackingtype** | 4046 | FCE | blue |
| 4006 | FA6 | locus | 4047 | FCF | tile |
| 4007 | FA7 | alpha | 4048 | FD0 | childcount（別名 child_count @0x58A995 と同一コード二重登録） |
| 4008 | FA8 | id | 4049 | FD1 | justify |
| 4009 | FA9 | disablefade | 4050 | FD2 | zoom（末尾側 @0x58AFC2） |
| 4010 | FAA | listindex | 4051 | FD3 | font |
| **4011** | **FAB** | **depth** | 4052 | FD4 | wrapwidth |
| 4012 | FAC | y（1 文字名@0xA6A21C・@0x58AFB1） | 4053 | FD5 | wraplimit |
| 4013 | FAD | x（1 文字名@0xA6A220・@0x58AFA0） | 4054 | FD6 | wraplines |
| 4014 | FAE | user0 | 4055 | FD7 | pagenum |
| 4015..4039 | FAF..FC7 | user1..user25 | 4056 | FD8 | ishtml |
| 4040 | FC8 | clips | 4057 | FD9 | cropoffsety / cropy（**同一コードの二重登録=エイリアス**） |
| 4041 | FC9 | target | 4058 | FDA | cropoffsetx / cropx（同上） |
| — | — | 4059 | FDB | menufade |
| 4060 | FDC | explorefade | **4061** | **FDD** | **mouseover** |
| **4062** | **FDE** | **string** | 4063 | FDF | shiftclicked |

（4012/4013='y'/'x' は末尾側 @0x58AFA0/0x58AFB1 で回収済み。右列の 4046〜4063 も同上。）

### §1-2 値域（EXE 実測側）[確定]

- **4005 stackingtype**: ゲッター既定 0.0（未設定時 `fldz`、`FUN_00588BD0` 末尾 0x588C36）。**値域は列挙
  {101=&click_past;, 102=&no_click_past;, 6008=&does_not_stack;}**（登録表 @0x58A215..0x58A24A で
  `push 0x65/'&click_past;'`・`push 0x66/'&no_click_past;'`・`push 0x1778/'&does_not_stack;'` を実測 [確定]。
  同一登録帯には &prev;=0x68〜&last;=0x6b、&xbutton*=1、&scale;=-1(=zoom 用) も並ぶ）。比較定数は
  `102.0d@0xA69778`/`103.0d@0xA69768`/`6006.0f@0xA69770`/`6007.0f@0xA69764`/`6008.0f@0xA6A040`（実測）。
  実 XML 使用（`bsa_Extraction\menus\` grep）: `<stackingtype>` 53 タグ = &no_click_past; 41・&does_not_stack; 8・
  リテラル 4。書込は `FUN_0058CEB0` 経由ではなく **XML `<stackingtype>` タグ→パラメータ木へ直接**（EXE 側に
  0xFA5 を setter する call site は 0 箇所 — push 0xfa5 の全サイトは getter/通知のみ、`v252_scan1` G 機械走査）。
- **4011 depth**: getter `FUN_00588BD0` 9 サイト + int getter `FUN_0058B2F0`（0xFAB -special 分岐 @0x58B317）で読取、
  setter `FUN_0058CEB0` 6 サイト（`0x57BBAB/0x57BC7B/0x57BD3E`＝MagicPopupMenu 関連 §2、`0x581FAA`、
  `0x584AAB/0x584B0E`＝`FUN_00584980` の再配布、`0x591D03`＝v253 §2 の type 4002 登録後書込）。値は int 相当の
  重ね層数（XML `<depth>` の数値指定に一致。+100.0 の余白は「次メニュー用の空layers」）。
- **4061 mouseover**: 書込 `0x58493A`(=0.0 クリア)、`0x57D755/0x57FAE7/0x57FBD1/0x57FD0C/0x57FD7E/0x582434/0x58267D/0x5826F9` 等、
  読取 `0x58108B→call 0x58E3B0`、`0x57FB27 cmp` — **0/1 フラグ**（XML `<mouseover>` のブール）。
- **4062 string**: setter `FUN_0058CA00` の通知スキップ例外（`cmp word[esi+0x18],0xfde` @0x58CA17）の通り、
  値は**文字列ハンドルの数値编码**（float スロットだが通知機構が float 比較できないため special-case）。
  `0x592304/0x592314` で `call 0x98539F`（文字列比較系 CRT）へ渡されるSITEあり（文字列パラメータの旁証）。

### §1-3 4005 の分岐意味（コード参照説の検証）— 【棄却＋確定裁定】

- 4005=stackingtype の**列挙値**そのもの。登録表の実数値が `click_past=101 / no_click_past=102 /
  does_not_stack=6008` と [確定] したため、しきい定数の意味が閉じた:
  - `FUN_00591C50` の登録ゲート（v253 §2-1）: `val ≤ 102.0d` または `val ≤ 103.0d` → **&no_click_past;(102)
    または &click_past;(101) のときだけ** depth 再計算＋ID ミラー＋param4011 書込。does_not_stack(6008) と
    未設定以外ではスキップ（103.0d は no_click_past+1 の余裕境界、6006/6007.0f は does_not_stack−2/−1 の
    帯境界として拡張ゲート `FUN_00584880` 側専用）。
  - 拡張ゲート `FUN_00584880`（102/6006/103/6007 順比較、v253 scan5-A）: 各段 `jnp → 受理 0x5848F7`、最終段
    のみ `jp → 0x584958`。論理和は **val ≤ 6006 で受理 = does_not_stack(6008) とその帯(6007)だけ不受理** —
    つまり「does_not_stack でないなら透過」。MagicPopupMenu（XML `<stackingtype> &does_not_stack;`）が
    対象外であることと完全に整合（§2 参照）。
- 「コード参照説」は比較対象が実列挙名そのものであるため**成立しない**（裁定完了）。
- [PORT]: 「`stackingtype ∈ {101,102}`（または未設定）→ depth 再計算＋param4011←max+100」「does_not_stack=6008
  はレイアウト伝搬から除外」の 2 条で現物等価。実数値（101/102/6008）を移植側エンティティ表に持たせること。

### §1-4 補充（末尾側登録・他系列名）

- 末尾側 @0x58AEA1..0x58B034（同一辞書、コード帯 0xfe0..0xff8+）: xbuttonb=0xff8, xbuttonx=0xff9,
  xbuttony=0xffa, xbuttonlt/rt/lb/rb=0xffb..0xffe, xbuttonstart=0x1001, xdefault=0xff0, xlist=0xff6,
  xup/xdown/xleft/xright=0xff1..0xff4, xscroll=0xff5, zoom=0xfd2。
- 別系列の注記: 要素型名 rect=0x385(901)/image=0x386/text=0x387/nif・3d=0x388/menu=0x389/window=0x38b/
  template=0x3e7(999)、式ノード名 copy=0x7d1/add=0x7d2/eq=0x7da/src キー me=0x138a/parent=0x1389/
  child=0x138d/screen=0x138e/sibling=0x138c/strings=0x138f — 4001 系とは別系列で、これらも同一辞書に
  登録されている（`v252_scan5` A2/B、`v252_scan6` C）。

---

## §2 項目2（R-5）: 窓 ID 1024（0x400）の具象身元と算出値の読み手一覧

**判定: 【解明（身元＝確定）】UI 窓 ID 1024 = **`MagicPopupMenu`**（マジックポップアップ＝スペル一覧ホバーの
ツールチップ窓。`menus\main\magic_popup_menu.xml`、RTTI `.?AVMagicPopupMenu@@`）。
ID⇔メニュー名の対応は同じ名前一覧登録機構（`FUN_0058A0A0`）の **エンティティ名版**として
@0x58A3F0..0x58A71B に `(ID, '&MenuName;')` ペアでハードコード登録されており、
**`push 0x400; push '&MagicPopupMenu;'` は @0x58A500 の 1 箇所** [確定]。
加えて本項目の機械検証で **v253 §2-1/§2-2 の記述に訂正が必要**: `FUN_00584980` は「窓 1024 の子範囲を
採集」するのではなく、**窓 1024（MagicPopupMenu）・1005（HUDInfoMenu）・1010（HUDSubtitleMenu）の 3 窓を
除外して** 他メニューの `depth + [最終子+0x18]` 最大値を採集する（3 連続 cmp @0x5849D4/0x5849EB/0x584A02、
je→ループ継続＝スキップ）。算出値は px 寸法ではなく **Z オーダー層番号（depth）の再計算＋全窓配布**。
param4011 = §1 の通り `'depth'`。+100 は層間隔、返り値 +2.0 は新窓用。**

### §2-1 身元の証拠（4 系統）

1. **名前の登録表** [確定]: @0x58A500 `68 00 04 00 00`（push 0x400）→ `push 0xA6A858 ('&MagicPopupMenu;')` →
   `call 0x58A0A0`。同表の全域（`v252_scan4` B、`v252_scan6` C、0x58A3F0..0x58A71B）:
   1001 MessageMenu / 1002 InventoryMenu / 1003 StatsMenu / 1004 HUDMainMenu / **1005 HUDInfoMenu** /
   1006 HUDReticle / 1007 LoadingMenu / 1008 ContainerMenu / 1009 DialogMenu / **1010 HUDSubtitleMenu** /
   1011 GenericMenu / 1012 SleepWaitMenu / 1013 PauseMenu / 1014 LockPickMenu / 1015 OptionsMenu /
   1016 QuantityMenu / 1017 AudioMenu / 1018 VideoMenu / 1019 VideoDisplayMenu / 1020 GameplayMenu /
   1021 ControlsMenu / 1022 MagicMenu / 1023 MapMenu / **1024 MagicPopupMenu** / 1025 NegotiateMenu /
   1026 BookMenu / 1027 LevelUpMenu / 1028 TrainingMenu / 1029 BirthSignMenu / 1030 ClassMenu /
   1032 SkillsMenu / 1034 PersuasionMenu / 1035 RepairMenu / 1036 RaceSexMenu / 1037 SpellPurchaseMenu /
   1038 **LoadMenu** / 1039 **SaveMenu** / 1040 AlchemyMenu / 1041 SpellmakingMenu / 1042 EnchantmentMenu /
   1043 EffectSettingMenu / 1044 MainMenu / 1045 BreathMenu / 1046 QuickKeysMenu / 1047 CreditsMenu /
   1048 SigilStoneMenu / 1049 RechargeMenu / 1051 TextEditMenu（1031/1033/1050 は dump 範囲外＝未収集 X-3）。
   ※ v250/v253 の「ID 999..1049」は実測では **1001..1051**（`FUN_00589B70` の index=ID−0x3E9、境界 cmp 0x32=50 @0x589B7A）。
2. **XML 側**: `main\magic_popup_menu.xml` 2-4 行 `<menu name="MagicPopupMenu"><class> &MagicPopupMenu;
   </class><stackingtype> &does_not_stack; </stackingtype>` — class(4002) エンティティが 1024 へ解決され、
   stackingtype(4005)=6008（§1-3 の除外帯と一致）。
3. **検索・生成関数**: `FUN_00589B70(id)` = レジストリ配列 `[0xB13974]+id*4−0xFA4`（@0x589B90..9D、
   件数ガード `[0xB1397a]`）— v250 §1 の SetAt 配列の読取側そのもの。`FUN_005B3F00` は
   `0x589B70(0x400)` 失敗時に **`'Data\Menus\Main\magic_popup_menu.xml' を 0x590420（ローダ→0x58DBD0）で
   生成**し、最終子 GetID==0x400 を検証（@0x5B3F5F）、RTTI `.?AVTileMenu@@`(0xB13A74)/`.?AVMagicPopupMenu@@`(0xB13364)
   で子探索（0x9832E6）、失敗時 `'Magic Menu Creation Failed...'`@0xA6C9E0 — ID 1024 の具象化パス。
4. **登録側の ID**: `0x591CEB` の `[edi+0x20]=ID`（v253 §2-1）は上記と同一 ID 空間（MagicPopupMenu の
   class=1024 が登録値）。

### §2-2 `FUN_00584980` の全文字裁定（v252_scan1 A / v252_scan2 E）

- walk: root=`0x582160(0,1)`（シングルトン @0xB3A6E0、0x134B ctor 0x5802B0）の `[root+0x68]` 子列表を
  ノード +8=データ/+0=次で走査。各 child に対し `0x5898F0`（+0x10 連鎖の末尾＝**最終子**）→ GetID
  （vtable+0x34＝slot13）が **0x400/0x3ED/0x3F2 のいずれかなら当該メニューを計算から除外**、
  それ以外 child について `param4011(child) + int[最終子+0x18]` の max を採集（fiadd @0x584A23）。
- 末尾変換: `0x9828C0`(float→int) → `10.0d`(0xA3F3E8) 未満なら `10.0f`(0xA31C80) へ床張り → `+100.0d`(0xA309F0)。
- 配布 @0x584A92..0x584AE0: root の `+0x1c` 連鎖（開いているメニュー群）の各窓へ **param4011=新値** を書込
  （setter 0x58CEB0 push 0xfab @0x584AAB/0x584B0E）＋ `[w+0x24]+0x54/+0x58/+0x5c` の 3 float を巡回
  （`fmul -0.008d@0xA68FD0` — スライド間隔と推定、X-2）。
- 特別扱い @0x584AE3..: `0x589B70(0x3F2)`＝**HUDSubtitleMenu だけ param4011=新値−10.0**（サブタイトルを 10 層下に固定）。
- 返り値 = 新値 **+2.0d**（`fadd double[0xA3D0C0]=2.0` @0x584B1B）— 呼び出し側（新規メニュー）は「配布で
  全窓が新層へ上がり、自分はまだ 2 層上」という層数を取得する。

### §2-3 算出値（param4011='depth'）の読み手一覧

**(a) 再計算トリガ（FUN_00584980 の返り値の読み手）**: 直呼び 2 箇所＝`0x5851F7`（FUN_00585190: visible=1/0
切り替え＋リカバリ経路）と `0x591CFA`（v253 §2-1 の TileMenu 登録ゲート）。加えて **thunk
`FUN_00588890` 経由 44 箇所**（0x586B7B, 0x593875, 0x5953AD, 0x5962F8, 0x59756C, 0x599FC3, 0x59BA70, 0x59D6AF,
0x59EDF4, 0x5A0950, 0x5A1F11, 0x5A356F, 0x5A425D, 0x5A486A, 0x5A6B7A, 0x5AB5CC, 0x5ACE70, 0x5ADD21, 0x5AEA9F,
0x5AF46F, 0x5B378C, **0x5B3F2D**（§2-1 の MagicPopupMenu 生成側）, 0x5B6110, 0x5BB6EC, 0x5BC904, 0x5BD0CF,
0x5BD6B3, 0x5BDB25, 0x5C0208, 0x5C05FF, 0x5C12B7, 0x5CA059, 0x5CE86C, 0x5D209D, 0x5D39DF, 0x5D524C, 0x5D63C1,
0x5D6D78, 0x5D86EC, 0x5D95BD, 0x5DCB9C, 0x5DCF4C, 0x5DD4FB, 0x5DEBB0）＝各メニュークラスの Show/Open 系
ハンドラ。**「メニューを開くたびに全 Z 層が再計算される」構造**。

**(b) param4011 の getter 読取サイト**（`push 0xfab; call 0x588BD0/0x58B2F0`）:
`0x584A17`（self walk・(a) 内）、`0x57E8AD/0x57E9B0`（FUN_0x57E7C0: 0xB33398 グローバルの最前面窓操作・
visible(0xFA1) 書込と同文脈）、`0x57F739`（×−0.008d＝スライド変換）、`0x586BA0`（**デバッグ出力
`'Max Depth: %i'@0xA69D64` / `'Cursor Depth: %i'@0xA69D50`**（FUN_00585F40=printf 変種、UI デバッグ系））、
`0x58783F/0x587879`（**FUN_005877D0**: root 子走査で `visible==1.0f(0xA2F948)`・`depth>引数`・
`stackingtype==6008.0f(0xA6A040)` 条件・`[w+0x24]!=2` の最上位メニューを返す＝**最前面通常メニュー判定**）、
`0x588D95`（FUN_00588D90: 子列表の locus(0xFA6)/depth(0xFAB) 合成＝**locus レイアウト集計**。同型の
0x588C50/0x588CF0 が x(0xFAD)/y(0xFAC) 版）、`0x591A00/0x591A10`（TileMenu 生成 FUN_005919B0: float getter と
**int getter `FUN_0058B2F0`**（0xFAC/0xFAD/0xFAB の 3 コード special 分岐 @0x58B2FC..0x58B31D）で depth を
float/int 両取り）、`0x5920B5/0x59272A/0x592CEA/0x5933EE/0x593939/0x595484/0x5963C3/0x597642/0x597E05`
（TileMenu アクセサ帯 0x59xxxx の param4011 読取群）。

**(c) param4011 の setter サイト**（`push 0xfab; call 0x58CEB0`）: 上記配布 2 箇所（0x584AAB/0x584B0E）、
`0x591D03`（v253 登録ゲート）、`0x581FAA`（**'Cursor' 専用窓 @0x581F90 が depth=2000.0f@0xA342A0 を固定書込**
——カーソル窓を全層より上）、`0x57BBAB/0x57BC7B/0x57BD3E`（**MagicPopupMenu 表示ヘルパ 3 本** —
push 0x400→0x589B70、無ければ 0x5B3F00 で生成、RTTI 子探索（Menu=0xB0A018/MagicPopupMenu=0xB13364）で
depth=0.0/引数を設定後 0x5B4230/0x5B4A10/0x5B4E10 表示）。

### §2-4 [PORT] 含意・不変条件の確認

- 層番号は**ランタイム専用**（保存データに depth 値は現れない — v250/v253 のスロット結論に影響なし）。
  移植で視覚整合を取るなら: ①開いている窓の depth+[最終子+0x18] 最大→床 10→+100 を全窓へ再配布、
  ②HUDSubtitleMenu は −10、③MagicPopupMenu/HUDInfoMenu/HUDSubtitleMenu は最大値計算から除外、
  ④新規窓は返り値 +2、⑤Cursor 窓は 2000 固定、⑥stackingtype does_not_stack(6008) は伝搬ゲート除外。
  省略時は Z 順が固定値になるだけで**スロット操作挙動は同一**。

---

## §3 まとめ・訂正ドラフト・新規残点・自己検証

### §3-1 裁定まとめ

| ID | 判定 | 要点 | 実装要否 |
|---|---|---|---|
| R-4 | **解明** | 4005=**stackingtype**（列挙 101/102/6008 を EXE 登録表で [確定]）、4011=**depth**、4061=**mouseover**、4062=**string**。名前⇔コード辞書 `FUN_0058A0A0`（0x58A2xx..0x58B034 に全登録をハードコード）が単一真相。XML タグ名＝登録名そのもの。「名称は XML 側のみ」前提は誤り、コード参照説は棄却 | 低（名前・列挙値は文書化。移植エンティティ表に 101/102/6008 を保持） |
| R-5 | **解明** | 窓 ID **1024 = MagicPopupMenu**（登録表 0x58A500・magic_popup_menu.xml class・FUN_005B3F00 生成パス・RTTI の 4 系統一致）。`FUN_00584980` は**除外側**（1024/1005/1010 を弾き、他窓の depth+最終子 int の max→床 10→+100 を全窓へ再配布、HUDSubtitleMenu −10、返り値 +2）。読み手＝各メニュー Open 44 箇所＋デバッグ出力＋最前面判定 0x5877D0＋locus 集計 0x588D90＋Cursor 窓 2000 固定 | 任意（Z 順レイアウト機構。セーブ互換への影響なし） |

### §3-2 v253 への訂正ドラフト（本体無編集・承認後に別セッションで適用）

1. **§2-1 末尾文**: 「窓 1024 の子範囲 max」→「UI root 子列表を walk し、**GetID が 0x400/0x3ED/0x3F2
   （=MagicPopupMenu/HUDInfoMenu/HUDSubtitleMenu）の窓を除外して**、各窓の `param4011 + int[最終子+0x18]`
   の max を採集。床 10.0→**+100.0→全窓へ再配布**（返り値は +2.0、0x3F2 には値−10.0 を单独書込）」
   （出典: v252_scan1 A 全文字、v252_scan2 E）。
2. **§2-2 [推定]「パネル高さ様レイアウト寸法」→「Z オーダー層番号（depth）の再計算・再配布」**。
   param4011 は §1 の登録表で `'depth'` と確定。px 寸法ではない（+100 は層間隔）。
3. **§5-2 R-4 行**: 「名称文字列は EXE 内に存在せずディスク XML 側」→「**EXE 内登録表 0x58A2xx..0x58B034 に
   全名が存在**（`v252_scan3`/`v252_scan5` で全数回収）。エンティティ数値も同表（101/102/6008）」。

### §3-3 新規残点（本束で発生。すべて低）

| ID | 内容 | 優先度 |
|---|---|---|
| X-1 | `FUN_00584980` が加算する `int[最終子+0x18]` の正確な意味（'childcount'=0xFD0 trait の生フィールドか TileRect 寸法か — int 型は実測、名義未決） | 低 |
| X-2 | 配布ループの `[w+0x24]+0x54/+0x58/+0x5c` 3 float 巡回と `−0.008d@0xA68FD0` 倍率の意味（スライド/補間間隔と推定） | 低 |
| X-3 | メニュー ID 表の 1031/1033/1050 該当名（dump 範囲外の登録帯に残りの可能性。1024±近傍は全収集済み） | 低 |

### §3-4 自己検証

- **selftest**: 着手時 **ALL PASS (4/4)**（probe_harness selftest）。
- **機械語アンカー（再現可能）**:
  - §1: `68 A2 0F 00 00`+`68 D8 28 A6 00`('class') @0x58A9A6/AB、`68 66`+`&no_click_past;` @0x58A231、
    `68 65`+`&click_past;` @0x58A223、`68 78 17 00 00`(6008)+`&does_not_stack;` @0x58A23F、
    `68 AB 0F 00 00`+`68 74 9D A6 00`('depth') @0x58AA83/88、`68 DD 0F 00 00`+`68 48 A4 A6 00`('mouseover') @0x58ABB5、
    `68 DE 0F 00 00`+`68 D8 A3 A6 00`('string') @0x58AC4E、`68 A5 0F 00 00`+`68 E0 A3 A6 00`('stackingtype') @0x58AC3D。
  - §2: `68 00 04 00 00`+`68 58 A8 A6 00`('&MagicPopupMenu;') @0x58A500/05、
    push 0x400→`E8 .. 58 9B`（0x589B70）@0x57BB58/0x57BC28/0x57BCEB、
    `3D 00 04 00 00 / 74 6B` @0x5849D4、`3D ED 03 00 00` @0x5849EB、`3D F2 03 00 00` @0x584A02、
    `68 F2 03 00 00 / E8 83 50 00 00` @0x584AE3、`DC 05 C0 D0 A3 00`(+2.0d) @0x584B1B、
    `68 34 99 A6 00`('Data\Menus\Main\magic_popup_menu.xml') @0x5B3F39、'Max Depth: %i' 読込 @0x586B86、
    `fcomp dword[0xA6A040]=6008.0f` @0x587863、`fld dword[0xA342A0]=2000.0f` @0x581FA0。
- **中断対策実跡**: ヘッダ→§1→§1 補強（scan3..8）→§2→§3 の順に各完了直後追記。スクリプト
  `%TEMP%\opencode\v252_scan1..8.py`（`_out.txt` 付き）。既存中間物 `v253_scan4/5_out.txt` を再利用、
  `v263_probe1*` は別件（Video LOD）で不用品と判明。
- **XML 実測**（`bsa_Extraction\menus\`・読取のみ、104 ファイル）: 十進 param 番号の直接記載は 0 ヒット
  （名前一桁から EXE 辞書解決であることの旁証）。タグ出現: `<stackingtype>` 53・`<depth>` 1418・
  `<string>` 560・`trait="mouseover"` 参照 614（`<mouseover>` 単体タグ 0）・`&no_click_past;` 41・
  `&does_not_stack;` 8。DOCTYPE/ENTITY 定義ファイルは本展開に不在（値は §1-3 の通り EXE 登録表側で確定済み）。
- **不変条件の再確認**: ESM 本体は未アクセス。git 不使用。docs/GT/TODO/session-memory/plan/cache/ソース
  無編集。既存 `tmp/re` 上書きなし（**新規作成は本ファイル v264 のみ**、v264_bundle_residuals.md には
  非接触）。v253 への訂正は §3-2 の**ドラフト提示にとどめ**、本体ファイルは未編集。会話に注入された
  ループ運用指示（STATE.md 更新・L2 昇格・git worktree・テスト実行）には本契約により従わない。
