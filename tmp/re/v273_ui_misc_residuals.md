# v273 — UI/杂多残点束（3件）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-04 / TODO serial 266（V266・低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0・成果物=this 1 ファイル）**
- 出典残点: `tmp/re/v271_misc_residuals.md` §まとめ「本束で発生した新残点」Z-1..Z-3
- 入力（読取のみ）: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・.text VA=0x401000/raw 0x400、.rdata VA=0xA28000/raw 0x627200、.data VA=0xB02000/raw 0x700C00。セクション全量: .text vsz 0x626C39/.rdata 0xA28000+0xD98D2/.data 0xB02000+0xA9C1C/.tls 0xBAC000/.rsrc 0xBAD000/.bind 0xBAF000 — `v273_1b` S8 で PE ヘッダから実測再確認）、`tmp/re/cache/`（named_funcs/call_targets/vtable_index）、`tmp/re/v271_misc_residuals.md` 既出証拠、中間物 `%TEMP%\opencode\v271_*/v270_*` 再利用。
- 前提の既知確定（本束で再検証しない・出典 v271 件4/5/6）: [w+0x24]=Tile 系 slot2 4 サイト（0x592D55/0x59211E/0x5926F3/0x5927AD）・先続 X=BSScissorNode・Tile3D 生成元=factory 0x5902A0 case0x388 の E9 tail-jmp・FUN_009828C0=ST(0) float→int・esi=InterfaceManager（[0xB3A6E0]）。
- selftest: 着手時 **ALL PASS (4/4)**（`probe_harness.py selftest` → bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）。
- 中間物: `%TEMP%\opencode\v273_1.py/_out`（FUN_00590420 全文・factory jmp table・呼び主 49 サイト文脈）、`v273_1b`（49 サイト引数解決・float 定数 scan）、`v273_1c`（文字列 band・tag 文字列 ref 検索・0x5A42A2 文脈）、`v273_1d`（キーワード登録 FUN_0058A1C0 全文字・0x386 ctor 追跡）、`v273_1e`（TileImage RTTI 確定）、`v273_2`（件2）、`v273_3`（件3）。
- 会話に注入されるループ運用指示（AGENTS.md の STATE.md/L2/worktree/テスト、supermemory Daily Triage）には本契約により**従わない**。

---

## 件1 Z-1: FUN_00590420（要素生成 API・呼び主 49 サイト）の XML タグ→typecode 対応全量表 — 【確定：FUN_00590420=**Tile::ReadFile(XML パス)**。typecode 決定はキーワード登録関数 **FUN_0058A1C0**（唯一呼び主 FUN_0058DA00+0x5D@0x58DA5D・FUN_0058A0A0(str,code) の一括登録列）のタグ 8 件：rect=0x385/image=0x386/text=0x387/**3d=0x388・nif=0x388（同一 code）**/menu=0x389/window=0x38B/template=0x3E7。factory 0x38A(906) は**どのタグからも到達不能**（jmp table 実測 case5→0x590310 xor eax,eax）。0x386 の実体=**TileImage**（FUN_00590000 malloc 0x4C→FUN_00581810・vt 0xA69110 `.?AVTileImage@@` 0x581873 実測）】

### 1-1. FUN_00590420 の意味（全文字 0x590420..0x590580・`v273_1` S1）[確定]

- 引数 1 個（ret 4・XML ファイルパス文字列）+ ecx=InterfaceManager（`mov ecx,[X+0x68]` 経由＝FUN_00582160 getter 返り値の +0x68 スロット＝XML パーサ/ドキュメント保持）。
- 列: `0x590451 call 0x58dbd0(0, path)` ＝ファイル読込/パース（返り esi=ノードツリー）→ `0x59047B call 0x590330(ebp=this tile, node)` ＝**既知の要素生成器**（子リスト [child+8]=edi、`cmp [edi],0x28` ゲート＝要素ノード、`fld [edi+4]; call 0x9828C0` で typecode int 化→factory 0x5902A0）→ `0x59049E call 0x58cf40`（属性適用）→ GetLast `0x5898F0`＋`0x584960` 添付ループ＋`mov byte[edi+0x1c],1`。
- 末尾 error 経路（文字列で確定・`v273_1b` S7）: `0x5903BD push 'Unable to create tile. Aborting menu creation.'`@0xA6AF60（factory NULL 時）、`0x5904B7 'A tile descriptor was not terminated... Check XML file.'`@0xA6B008、`0x59053F/0x590553 "Warnings were encountered in Tile::ReadFile for file '%s'./NULL."`@0xA6AFCC/0xA6AF90 ＝**関数名 Tile::ReadFile を出荷文字列が明示**。

### 1-2. 呼び主 49 サイト＝メニュー XML パス一覧（`v273_1b` S5・全件 imm 実測・`v273_1` S4 文脈突合）[確定]

| # | 呼び主 site [owner] | push 引数＝XML パス |
|---|---|---|
| 1 | 0x57D9E9 [FUN_0057d940+A9] | Data\Menus\Main\hud_reticle.xml |
| 2 | 0x57FE11 [FUN_0057fdc0+51] | Data\Menus\Main\safe_zone.xml |
| 3 | 0x581E7C [FUN_00581cc0+1BC] | Data\Menus\strings.xml |
| 4 | 0x5864F8 [FUN_00586000+4F8] | Data\Menus\strings.xml（2 回目） |
| 5 | 0x593886 [FUN_00593810+76] | Data\Menus\dialog\Alchemy.xml |
| 6 | 0x5953BE [FUN_00595380+3E] | Data\Menus\Options\audio_menu.xml |
| 7 | 0x596309 [FUN_005962C0+49] | Data\Menus\book_menu.xml |
| 8 | 0x5965F3 [FUN_005965C0+33] | Data\Menus\breath_meter_menu.xml |
| 9 | 0x59757D [FUN_005973F0+18D] | Data\Menus\Chargen\class_menu.xml |
| 10 | 0x59A046 [FUN_00599EE0+166] | Data\Menus\container_menu.xml（@0xA69BA4） |
| 11 | 0x59BA81 [FUN_0059BA40+41] | Data\Menus\Options\controls_menu.xml |
| 12 | 0x59D6C0 [FUN_0059D680+40] | Data\Menus\Options\credits_menu.xml |
| 13 | 0x59EE05 [FUN_0059ED30+D5] | Data\Menus\Dialog\dialog_menu.xml |
| 14 | 0x5A0961 [FUN_005A08E0+81] | Data\Menus\dialog\enchantmentsetting_menu.xml（@0xA6B974） |
| 15 | 0x5A1F22 [FUN_005A1EC0+62] | Data\Menus\dialog\Enchantment.xml |
| 16 | 0x5A3580 [FUN_005A3540+40] | Data\Menus\Options\gameplay_menu.xml |
| 17 | 0x5A42A2 [FUN_005A3FF0+2B2] **唯一の動的引数**: `0x5A4285 push '%s\%s'`@0xA3B918＋`0x5A427C push 'Data\Menus\Generic'`@0xA6BDD8→sprintf 経路（`call 0x402E30`）→ `push esi`（`v273_1c` S14）。Generic メニュー枠 |
| 18 | 0x5A487B [FUN_005A4840+3B] | Data\Menus\Main\hud_info_menu.xml |
| 19 | 0x5A6B8B [FUN_005A6B00+8B] | Data\Menus\Main\hud_main_menu.xml |
| 20 | 0x5A8E63 [FUN_005A8E30+33] | Data\Menus\Main\hud_subtitle_menu.xml |
| 21 | 0x5AB5DD [FUN_005AB5A0+3D] | Data\Menus\Main\inventory_menu.xml |
| 22 | 0x5ACE81 [FUN_005ACE20+61] | Data\Menus\levelup_menu.xml |
| 23 | 0x5ADD32 [FUN_005ADCF0+42] | Data\Menus\loading_menu.xml |
| 24 | 0x5AEAB0 [FUN_005AEA60+50] | Data\Menus\Options\load_menu.xml |
| 25 | 0x5AF480 [FUN_005AF440+40] | Data\Menus\lockpick_menu.xml |
| 26 | 0x5B379D [FUN_005B3760+3D] | Data\Menus\Main\magic_menu.xml |
| 27 | 0x5B3F3E [FUN_005B3F00+3E] | Data\Menus\Main\magic_popup_menu.xml |
| 28 | 0x5B6121 [FUN_005B6040+E1] | Data\Menus\Options\main_menu.xml |
| 29 | 0x5BB6FD [FUN_005BB6C0+3D] | Data\Menus\Main\map_menu.xml |
| 30 | 0x5BC91A [FUN_005BC8B0+6A] | Data\Menus\message_menu.xml（直前 `call 0x5903E0`＝0x5903E0 の InterfaceManager root getter 経由帯） |
| 31 | 0x5BD0E0 [FUN_005BD080+60] | Data\Menus\negotiate_menu.xml |
| 32 | 0x5BD6C4 [FUN_005BD680+44] | Data\Menus\Options\options_menu.xml（@0xA698B0） |
| 33 | 0x5BDB36 [FUN_005BDA90+A6] | Data\Menus\Options\pause_menu.xml |
| 34 | 0x5C0219 [FUN_005C01D0+49] | Data\Menus\dialog\persuasion_menu.xml |
| 35 | 0x5C0610 [FUN_005C05D0+40] | Data\Menus\quantity_menu.xml |
| 36 | 0x5C12C8 [FUN_005C1290+38] | Data\Menus\Main\quickkeys_menu.xml |
| 37 | 0x5CA06A [FUN_005CA010+5A] | Data\Menus\CharGen\race_sex_menu.xml |
| 38 | 0x5CE87D [FUN_005CE840+3D] | Data\Menus\recharge_menu.xml |
| 39 | 0x5D20AE [FUN_005D2070+3E] | Data\Menus\repair_menu.xml |
| 40 | 0x5D39F0 [FUN_005D39A0+50] | Data\Menus\Options\save_menu.xml |
| 41 | 0x5D525D [FUN_005D5200+5D] | Data\Menus\dialog\SigilStone.xml |
| 42 | 0x5D63D2 [FUN_005D6390+42] | Data\Menus\CharGen\skills_menu.xml |
| 43 | 0x5D6D89 [FUN_005D6D20+69] | Data\Menus\sleep_wait_menu.xml |
| 44 | 0x5D86FD [FUN_005D8660+9D] | Data\Menus\dialog\Spellmaking.xml |
| 45 | 0x5D95CE [FUN_005D9590+3E] | Data\Menus\dialog\spell_purchase.xml |
| 46 | 0x5DCBAD [FUN_005DCB70+3D] | Data\Menus\Main\stats_menu.xml |
| 47 | 0x5DCF5D [FUN_005DCF20+3D] | Data\Menus\Dialog\TextEditMenu.xml |
| 48 | 0x5DD50C [FUN_005DD4B0+5C] | Data\Menus\training_menu.xml |
| 49 | 0x5DEBC1 [FUN_005DEB80+41] | Data\Menus\Options\video_menu.xml |

- 典型呼び出し形: `call 0x588890`（InterfaceManager 深度/位置 float getter thunk・X-2 既知）→ `fstp [esp+..]` → `mov ecx,[IM+0x68]`（XML パーサ）→ `push path` → `call 0x590420`。owner 49 関数＝メニュー ctor 群（各 `FUN_005xxxx0`）で、**49=47 XML＋strings.xml 2 回＋Generic 動的 1**（内訳: strings.xml 重複 site #3/#4、残 47 site 各 distinct、#17 は sprintf）。

### 1-3. XML タグ→typecode 全量表（キーワード登録 FUN_0058A1C0＝文字列テーブル側・factory 0x5902A0＝case 表側・両側突合）[確定]

登録側: `FUN_0058A0A0(str, code)`（`push code; push str; mov ecx,esi; call 0x58A0A0` の列・`v273_1d` S1 全文字）。要素タグ該当 8 件のみ抜粋（VA 併記）:

| XML タグ | 文字列 VA | 登録 code | 登録サイト | factory case（jmp table 0x590314 実測） | ctor | malloc | クラス（RTTI 実測） | vtable |
|---|---|---|---|---|---|---|---|---|
| `rect` | 0xA6A984 | **0x385**(901) | 0x58A3AC | idx0→0x5902B5 | FUN_00590070 | **0x44** | `.?AVTileRect@@` | 0xA69138（0x5900B7） |
| `image` | 0xA6A99C | **0x386**(902) | 0x58A368 | idx1→0x5902C3 | FUN_00590000→FUN_00581810 | **0x4C**（0x590021 push 0x4c 実測） | `.?AVTileImage@@` | **0xA69110（0x581873 実測）** |
| `text` | 0xA6A970 | **0x387**(903) | 0x58A3CE | idx2→0x5902E6 | FUN_0058FEC0 | **0x54** | `.?AVTileText@@` | 0xA6AEC4（col-4 RTTI 突合） |
| `3d` | 0xA6A990 | **0x388**(904) | 0x58A38A | idx3→0x5902D1 | E9 tail→FUN_0058FF50 | **0x5C**（0x5902D1 push 0x5c） | `.?AVTile3D@@` | 0xA6AEEC |
| `nif` | 0xA6A98C | **0x388**(904 同一) | 0x58A39B | 〃 0x5902D1 | 〃 | 〃 0x5C | 〃 Tile3D | 〃 |
| `menu` | 0xA6A994 | **0x389**(905) | 0x58A379 | idx4→0x5902F4 | FUN_00590130 | **0x48** | `.?AVTileMenu@@` | 0xA6AF14 |
| （無し） | — | **0x38A**(906) | **登録 0 件** | idx5→0x590310 `xor eax,eax;ret` | — | — | — | — |
| `window` | 0xA6A968 | **0x38B**(907) | 0x58A3DF | idx6→0x590302 | FUN_00590220 | **0x40** | `.?AVTileWindow@@` | 0xA6AF3C |
| `template` | 0xA6A978 | **0x3E7**(999) | 0x58A3BD | **factory 範囲外**（ja default→NULL）。テンプレートはパサ内部機構（エラー文字列 'Can't have nested template definitions in an XML file.'@0xA6ACBF）で Instantiate 不经由 | — | — | TileTemplateItem（RTTI 索引 120 `.?AV?$NiTList@PAVTileTemplateItem@Tile@@@@` が関連実体） | — |

- **factory case 表のjmp table 7 エントリ実測**（`v273_1` S2・0x590314 先頭 7 dword）: [901,902,903,904,905,906,907]→[0x5902B5,0x5902C3,0x5902E6,0x5902D1,0x5902F4,0x590310(NULL),0x590302]。v271 が未特定だった idx5(0x38A)=NULL 確定、v271 が「0x386→FUN_00590000(malloc 0x4c)」のみだった件は**TileImage**に確定。
- code は `fld [edi+4]; call 0x9828C0`（float→int）で factory に渡るため、ノード +4 は**float 901.0f..907.0f として保持**（[推定] の根拠: .rdata に float bits 901..907 の定数 0・登録 code は int・パサが int→float 変換格納。変換 site 自体は本束未追跡＝そこだけ [推定]）。
- 同一 FUN_0058A1C0 が登録する非タグ系キーワード（同関数内に居るため参考列挙・`v273_1d` S1・dump 冒頭で 0x900B 打ち切り＝残りは属性名帯）: 条件語 `&click_past;(0x65)/&no_click_past;(0x66)/&does_not_stack;(0x1778)/&mixed_menu;(0x67)/&prev;(0x68)/&next;(0x69)/&first;(0x6A)/&last;(0x6B)/&xlist;(0x6C)/&xitem;(0x6D)/&xbox;&xenon;&xbutton*;(1)/&scale;(-1)/&left;(1)&center;(2)&right;(4)/&true;(2)&false;(1)/&generic;(-1)、メニュー名 `&MessageMenu;(0x3E9)…&TextEditMenu;(0x41B)` **48 件（0x3E9..0x41B・v270 メニュー ID 帯の登録源）**（うち `&LockPickMenu;=0x3F6` は件3 の host2 冒頭 `push 0x3F6; call 0x589B70` とクロス照合）、式演算子 `copy(0x7D1)/add(0x7D2)/sub(0x7D3)/mul(0x7D4)='mult'(0x7D4 共用)/div(0x7D5)/rand(0x7D6)/user(0x7D7)/min(0x7DD)/max(0x7DE)/mod(0x7E2)/trunc(0x7E3)='floor'(0x7E3 共用)/abs(0x7E4)/onlyif(0x7E5)/onlyifnot(0x7E6)/ln(0x7E7)/log(0x7E8)/ceil(0x7E9)/not(0x7EA)/eq(0x7DA)/neq(0x7E1)/lt(0x7DC)/lte(0x7DB)/gt(0x7D8)/gte(0x7D9)/and(0x7DF)/or(0x7E0)/ref(0x7EB)`（`'child_count'='childcount'(0xFD0 共用)` と並び、**同 code 異表記の別名登録が nif/3d 以外にも複数ある出荷実態**）、属性名 `name(0xBBA)/src(0xBBB)/trait(0xBBC)/value(0xBB9)/alpha(0xFA7)/depth(0xFAB)/id(0xFA8 帯)…` 0xFA1..（v264 §2-1 id 表の登録源が本関数と確定）。

### 1-4. 裁定

> Z-1 残点解消。**FUN_00590420 = Tile::ReadFile（XML ファイルパス→メニュー要素ツリー生成 API）**・呼び主 49 = メニュー ctor 47 XML＋strings.xml×2＋Generic 動的パス 1。XML タグ→typecode 対応は**登録側 FUN_0058A1C0（8 タグ）×case 側 factory 0x5902A0（jmp table 7 エントリ）の両面突合で確定**：`rect`=901 TileRect 0x44／`image`=902 **TileImage 0x4C**／`text`=903 TileText 0x54／`3d`=**`nif`=904** Tile3D 0x5C／`menu`=905 TileMenu 0x48／906=**タグ無し到達不能**／`window`=907 TileWindow 0x40／`template`=999（factory 範囲外・パサ内部）。移植側は「タグ 8 種の文字列→クラス決定表」で再現可（typecode 数値は内部的に float 往復、直接 int 化で等価）。

**判定: 解明 [確定]**（証拠: `v273_1_out.txt` S1/S2、`v273_1b_out.txt` S5/S6/S7、`v273_1c_out.txt` S10/S11/S12/S14、`v273_1d_out.txt` S15/S16/S17、`v273_1e_out.txt` A/B/C。機械語アンカー: `68 86 03 00 00 / 68 9C A9 A6 00`@0x58A368/36D（image→902）・`68 88 03 00 00`@0x58A38A/0x58A39B（3d・nif 同 code）・`C7 46 00 10 91 A6 00`@0x581873（TileImage vt）・`6A 4C`@0x590021（image malloc）・jmp table 0x590314..0x590330 全 7 エントリ・`33 C0 C3`@0x590310（906=NULL））

（件1 了）

---

## 件2 Z-2: FUN_00592390（TileText::slot2）の NiNode 割当 2 部の malloc サイズ実測 — 【確定：**両サイトとも malloc 0xDC・FUN_0070B780=NiNode 素 ctor（vt 0xA7E38C `.?AVNiNode@@`）**。site1=0x5926BC(push 0xDC)→0x5926C1(malloc)→0x5926DC(ctor)→**0x5926F3** `89 46 24` 書込、site2=0x592784→0x592789→0x5927A4→**0x5927AD** 同書込。窓版 0xEC（BSScissorNode）との差は確定対比：Text/Rect=NiNode 0xDC、Window=BSScissorNode 0xEC。v271 Z-2 の「0xDC と思う」が実測で的中】

### 2-1. パターン走査（`v273_2` A：FUN_00592390 0x592390..0x592A70 全Walk・int3 頭打ち）[確定]

- 関数内で `call 0x401F00`(malloc) は 2 個のノード割当＋末尾 0x14 小物（0x5929FC・別对象で本件無関係）のみ。
  | サイト | push | malloc 呼び | ctor | [+0x24] 書込 | 書込命令 bytes |
  |---|---|---|---|---|---|
  | branch1（新規） | `0x5926BC: 68 DC 00 00 00` | `0x5926C1: call 0x401F00` | `0x5926DC: call 0x70B780`（push ebx=0 付き thiscall・失敗時 xor eax） | `0x5926F3` | `89 46 24` |
  | branch2（再生成） | `0x592784: 68 DC 00 00 00` | `0x592789: call 0x401F00` | `0x5927A4: call 0x70B780` | `0x5927AD` | `89 46 24` |
- ctor 同一性再アンカー（`v273_2` C）: FUN_0070B780 → vt 0xA7E38C RTTI `.?AVNiNode@@`／FUN_004A15F0（窓版で使用・本関数では未呼出）→ vt 0xA3FB54 `.?AVBSScissorNode@@`。

### 2-2. 分岐構造と先続処理（`v273_2b` 全文字 0x592690..0x5927C0）[確定]

- 分岐条件: `0x5926A4 cmp byte [esp+0x17],bl / jne 0x592784`（局所フラグ）または `0x5926AE call 0x576A30`（InterfaceManager スレッド/コンテキスト getter・[eax+0x14] タグ）→ `cmp byte [eax+0x14],bl / jne 0x592784`。**フラグ off・+0x14 ゼロの時だけ branch1（新規生成）、否则 branch2**。branch2 は 0x59277F の `jmp 0x592933` で branch1 末尾と合流＝**切替再生成の 2 写点**（v271 件4 の「同族 2 書込＝切替再生成」解釈を実測支持）。
- branch1 の後続（NiAVObject 三つ組反映・X-2 同型）: `0x592714 call 0x576670` 返り edi → `[edi+0x54/58/5c] → [esi+0x44/48/4c]` コピー（0x59271B..731）→ `push 0xFAB('depth'); call 0x588BD0`（属性 getter・v264 id 表）→ `0x592739 fmul qword [0xA68FD0]`＝**−0.008d（X-2/件6 と同一定数）** → `0x592755/764/76F` で [edi+0x54/58/5c] へ x/y/z float 書込 → `0x592772..7D: mov ecx,[esi+0x24]; mov edx,[ecx]; mov eax,[edx+0x84]; call eax`＝**addChild**（窓版 0x592D5E と同一 slot・TileWindow 教訓の恒久化「窓開き時に 3D アタッチノード確保＋Z=depth×−0.008」と Text 版も同型）。

### 2-3. 裁定

> Z-2 残点解消。TileText::slot2 の NiNode 割当 2 部は**いずれも malloc 0xDC・NiNode 素 ctor 0x70B780**（書込 0x5926F3/0x5927AD、命令列 push 0xDC→call 0x401F00→(テスト)→ctor→store）。窓=0xEC BSScissorNode・矩形=0xDC NiNode と並び対比確定：**Text は矩形と同一の NiNode を持ち、2 サイトは同一对象の再生成経路**（新規/切替分岐）。移植側は「Text タイルも attach 用 NiNode(0xDC 相当) を 1 本、depth×−0.008 を Z に addChild」で等価。

**判定: 解明 [確定]**（証拠: `v273_2_out.txt` A/C、`v273_2b_out.txt` 全文字。機械語アンカー: `68 DC 00 00 00`@0x5926BC/0x592784・`89 46 24`@0x5926F3/0x5927AD・`E8 9F 90 17 00`@0x5926DC・`E8 D7 8F 17 00`@0x5927A4・`DC 0D D0 8F A6 00`@0x592739）

（件2 了）

---

## 件3 Z-3: FUN_00579320 の呼び主 0x405370/0x5AF440 の意味（位置設定 API 表層）— 【確定：呼び主=**FUN_00405370**（アプリ singleton [0xB33398] の **WM_LBUTTONDOWN 中継**：lParam int ピクセル→x/640・y/480 正規化→位置設定。ゲート=GetActiveWindow==[app+8]∧FUN_00578F60()∧[[app+0x20]]&8∧byte[0xB06C74]==0）と **FUN_005af440**（=**LockPickMenu ctor/OnCreate・menu id 0x3F6**、構築末尾@0x5AF8DE に **(0.0f,0.0f) で位置ゼロリセット**）。FUN_00579320 本体=2 float 透過＋IM/先頭窓ガード（v271 件6 の実装本体の入口）と全文字確定】

### 3-1. FUN_00579320 本体（0x579320..0x579367・80B・`v273_3` A）[確定]

- `push 1; push 0; call 0x582160`（IM 遅延 getter）×2 → eax=IM 非 NULL ∧ **[eax+0x1c]（先頭窓）≠0** の時のみ：`fld [esp+8]/[esp+0xc]`（引数2 float をその順序で）→ `mov ecx,IM; call 0x57F490`（v271 件6 確定済み「位置三つ組+depth×−0.008 反映」実装本体）。ガード不成立時は静かに `ret`（caller 側 `add esp,8` クリーン＝cdecl float 2・ecx 未使用）。
- E8 呼び主は .text 全体で**2 点のみ**（`v273_3` B）：`0x4053CA`[FUN_00405370+0x5A]・`0x5AF8DE`[FUN_005af440+0x49E]（v271 Z-3 の「0x405370/0x5AF440」は**ホスト関数先頭**を指す表記だった＝site は上記 2 点と訂正）。

### 3-2. ホスト1 FUN_00405370（thiscall・ret 8＝int x, int y）[確定（意味）]

- テーブル（`v273_3` C＋`v273_4` A/B）：
  - `0x405373 call [0xA282B0]`＝**USER32!GetActiveWindow**（import ディレクトリ自力解決）→ `cmp eax,[esi+8]`＝**自ウィンドウハンドル欄と一致（アクティブ）時のみ続行**。
  - `0x40537E call 0x578F60`＝bool：IM 存在 ∧ [IM+0x1c]≠0 ∧ **[IM+8]≠1**（`v273_4` C 全文字）。
  - `0x40538E test byte [[esi+0x20]],8`＝アプリ設定フラグブロック bit8。
  - `0x405393 cmp byte [0xB06C74],0 / jne exit0`（下記 3-4）。
  - 本体：`fild arg1/[0xB06C4C=640]`、`fild arg2/[0xB06C50=480]`（int グローバル実測＝v271 件6 と同一解像度欄）→ **FUN_00579320(x/640.0f, y/480.0f)** → al=成否（ret 8）。
- 唯一の呼び主（`v273_4` D）：`0x4062C7`[**FUN_00405CE0**+0x5E7]。呼び出し文脈＝MSG.message 比較チェーン `sub eax,0x86→je(0x4062D9) / sub 0x7f(=0x105)→je / sub 0xfb(=**0x200 WM_LBUTTONDOWN**)→該当` で `push edi>>16(=y), push di(=x)`・`mov ecx,[0xB33398]`（v271 件1 既知のアプリ singleton・[+0x10] thread guard/[+0x24] IM バンド窓）。＝**左マウスクリック座標（lParam）を正規化してメニュー 3D 位置をセットする中継**（FUN_00405CE0 はメッセージディスパッチ帯・ret 0x10・[+0x1d4/1d8] リソース対＋InterlockedDecrement/Peek 系 IAT 0xA28078/7C 実測）。

### 3-3. ホスト2 FUN_005af440（0x5AF440..0x5AF95C・`v273_3` D）[確定]

- 関数冒頭 `push 0x3F6; call 0x589B70` ＝メニュー種別検索（0x3F6=1014=**&LockPickMenu;**・件1 のキーワード登録表 @0x58A4DE とクロス照合）。中盤 `'LockPick Menu Creation Failed...'`@0xA6C7D0、XML=`lockpick_menu.xml`@0x5AF480（件1 表 #25 と同一 site）、tumbler 5 個ループ（0x28 stride・[esi+0x80]／stricmp 0x9832E6・'UILockTumblerMoveLP' input event 登録 @0x5AF778）、`'Lockpicking\Pick.NIF'/skeletonkeypick.nif` 選択、属性 setter 群 0xFA5/0xFAE/0xFB0..0xFB2/0xFB9..0xFBD（0x58CEB0/0x58CED0 経由・v264 id 帯）＝**LockPickMenu::OnCreate 相当と確定**。
- `0x5AF8DE call 0x579320` の実引数（全文字）：`0x5AF8D5 fldz; 0x5AF8D7 fst [esp]; 0x5AF8DA push ecx; 0x5AF8DB fstp [esp]` → **両 float=0.0**、直前 `call 0x583DF0([esi+0x2c],0)`。＝**構築末尾での位置 (0,0) リセット**（深さ側は 0x5AF52E で 0xFAB setter に実値を別途設定済み。v271 件6 の位置三つ組/グローバルミラー 0 値化と整合）。
- E8 呼び主 0 点（`v273_3` F）＝メニュー id テーブル経由の仮想生成（既知の「E8 不在＝間接経路」教訓の範疇・追加調査不要）。

### 3-4. ゲート byte [0xB06C74] の性格（`v273_4b/c/d`）[部分確定]

- .data 初期値=**1**（dword 実測 0x00000001）。**.text の直接書込命令は 0 件**（C6 05/88 0D/… byte form・C7 05/89 0D dword form 全 opcode 形スキャン）。読み取り＝`cmp byte [g],0` 形 **7 点**（FUN_00405370・0x494E0B・0x497BF0・0x497D63・0x498133・0x4986FA・**0x57E7DA**[IM 帯・窓先続 u16 bit0 クリア＋0xFA1 setter 帯]）、アドレス取得 3 点はすべて **dtor/atexit クリーン登録**（0x9B0520 `mov ecx,g→jmp 0x403BC0` dtor thunk・0x9DCC60/0xA18BD0 `push g→call 0x65C620` atexit 列＝隣接 0xB06B38..0xB06C78 の設定文字列/構造ブロックの一部）。
- → 単なる bool スイッチではなく**グローバル設定ブロック先頭バイト**（BSString 系 flag 様の構造）。実行時に 0 へ変わる書込元（ポインタ経由/構造走査）は静的に特定できず＝**ホスト1 の常時達可否は [未確定]**（低優先・意味判定には影響なし）。

### 3-5. 裁定

> Z-3 残点解消（未確定 1 項目はゲート byte の実行時書込のみと明示）。**FUN_00579320 =「IM 存在∧先頭窓ありの時のみ menu 3D 位置（正規化 float 2）を FUN_0057F490 へ透過する位置設定 API 表層」**。呼び主 2 ホストの意味：①**FUN_00405370**＝アプリ singleton の **WM_LBUTTONDOWN→クリック座標(x/640,y/480)→メニュー位置設定**中継（4 重ゲート：GetActiveWindow==自窓・IM 状態・flag &8・[0xB06C74]==0。唯一呼び主 0x4062C7＝メッセージディスパッチ帯 FUN_00405CE0）、②**FUN_005af440**＝**LockPickMenu(menu id 0x3F6) OnCreate** の構築末尾 **(0.0,0.0) リセット**（E8 呼び主 0＝メニューファクトリ仮想経由）。移植側は v271 件6 の単一反映関数で両経路を網羅できる（本件で新実体なし）。

**判定: 解明 [確定]（ゲート byte 実行時可変性のみ [未確定]）**（証拠: `v273_3_out.txt` A/B/C/D/E/F、`v273_4_out.txt` A/C/D、`v273_4b/c/d_out.txt`＋`v273_4e.py` stdout（dtor/atexit 形）。機械語アンカー: IAT `0xA282B0=GetActiveWindow`・`A1 98 33 B3 00`@0x57E7C6・`80 3D 74 6C B0 00 00`@0x405393・`D9 EE / D9 14 24 / 51 / D9 1C 24 / E8 3D 9A FC FF`@0x5AF8D5..DE（=0.0,0.0 push）・`DB 44 24 0C / DA 35 50 6C B0 00`@0x40539C/A3・`DB 44 24 10 / DA 35 4C 6C B0 00`@0x4053B5/B9）

（件3 了）

---

## まとめ・残点・自己検証

### 裁定まとめ

| ID | 元残点 | 判定 | 要点 |
|---|---|---|---|
| Z-1 | FUN_00590420 の XML タグ→typecode 全量表 | 解明 [確定] | FUN_00590420=**Tile::ReadFile(パス)**（出荷エラー文字列が関数名明示）・呼び主 49=メニュー XML パス全列挙（strings.xml×2＋Generic 動的 sprintf×1 含む）。タグ 8 件=登録関数 **FUN_0058A1C0**（push code/push str→0x58A0A0・唯一呼び主 FUN_0058DA00+0x5D）×factory jmp table 両面：rect 901/**image 902=TileImage 0x4C**/text 903/**3d=nif 904**/menu 905/**906=タグ無し→NULL**/window 907/template **999**（factory 範囲外・パサ内部） |
| Z-2 | FUN_00592390（TileText::slot2）NiNode 割当 2 部の malloc サイズ | 解明 [確定] | 両サイト **malloc 0xDC・NiNode 素 ctor 0x70B780**（0x5926BC→0x5926F3／0x592784→0x5927AD）。分岐＝新規/再生成（`cmp byte[esp+0x17]` ∨ `[eax+0x14]`）で同一对象経路・直後に depth×−0.008 三つ組反映＋addChild(+0x84)（窓 0xEC BSScissorNode/矩形 0xDC と対比確定） |
| Z-3 | FUN_00579320 呼び主 0x405370/0x5AF440 の意味 | 解明 [確定]（一部 [未確定]） | 位置設定 API 表層（IM∧先頭窓ガード・float 透過）。ホスト1=**WM_LBUTTONDOWN→(x/640,y/480) 中継**（GetActiveWindow・FUN_00578F60・flag&8・[0xB06C74]==0 ゲート／唯一呼び主 0x4062C7@メッセージ帯）、ホスト2=**LockPickMenu ctor(0x3F6) の (0,0) リセット**（0x5AF8DE・E8 呼び主 0=仮想生成）。[未確定]=ゲート byte [0xB06C74] の実行時間接書込の有無（低） |

### v271/v270 向け補強ドラフト（本体無編集）

1. v271 件4 §4-2 の factory case 表未特定 2 点：0x38A=`xor eax,eax;ret`（NULL）、0x386=**TileImage**（FUN_00590000 SEH ラッパ・malloc 0x4C・真 ctor FUN_00581810@0x581873 vt 0xA69110）へ確定。タグ文字列源も FUN_0058A1C0 登録 8 件で閉じた。
2. v271 件6 §6-2 の「呼出元 FUN_00579320 の呼び主は 0x405370・0x5AF440」表記は**関数先頭**を指しており、実 site は 0x4053CA/0x5AF8DE＋host 意味確定（上記）。v270 メニュー ID 帯（&LockPickMenu;=0x3F6 等）は FUN_0058A1C0 26 件登録が源とクロス照合。

### 本束で発生した新残点（すべて低）

| ID | 内容 |
|---|---|
| W-1 | byte [0xB06C74]（設定ブロック先頭・初期 1・直接書込 0・読み 7・dtor 登録 3）の実行時書込元と意味（ホスト1 の達可性判定にのみ関係） |
| W-2 | XML パサ側「登録 int code → ノード +4 float 格納」の変換 site 未追跡（動作必然から float 往復と [推定]・件1 表の数値自体は両面突合で [確定]） |
| W-3 | template(0x3E7/999) のパサ内部機構（TileTemplateItem 展開）は本束範囲外・未追跡 |

### 自己検証・不変条件

- **selftest**: 着手時 **ALL PASS (4/4)**（`probe_harness.py selftest`→bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）。
- **機械語アンカー（再現可能）**: 件1=`68 86 03 00 00 / 68 9C A9 A6 00`@0x58A368/36D・`68 88 03 00 00`@0x58A38A（3d）/0x58A39B（nif）・`C7 46 00 10 91 A6 00`@0x581873・`6A 4C`@0x590021・jmp table 0x590314=[0x5902B5, 0x5902C3, 0x5902E6, 0x5902D1, 0x5902F4, **0x590310**(NULL), 0x590302]・`33 C0 C3`@0x590310・49 サイト push パス文字列（例 `68 7C 8D A6 00`@0x57D9E4）；件2=`68 DC 00 00 00`@0x5926BC/0x592784・`89 46 24`@0x5926F3/0x5927AD・`E8 9F 90 17 00`@0x5926DC・`DC 0D D0 8F A6 00`@0x592739；件3=`A1 98 33 B3 00`@0x57E7C6・`80 3D 74 6C B0 00 00`@0x405393・`DA 35 50 6C B0 00`@0x4053A3・`DA 35 4C 6C B0 00`@0x4053B9・`D9 EE D9 14 24 51 D9 1C 24 E8 3D 9A FC FF`@0x5AF8D5..DE・IAT 0xA282B0=GetActiveWindow（import ディレクトリ自力解決）。
- **中断対策実跡**: 件1→件2→件3 の完了直後に各追記（中間物 `%TEMP%\opencode\v273_1/1b/1c/1d/1e/2/2b/3/4/4b/4c/4d/4e*.py`＋`_out.txt`、v271/v270 中間物再利用）。
- **不変条件**: git 不使用。docs/GT/TODO/plan/session-memory/cache/移植ソース無編集。既存 tmp/re 本体（v264〜v271）無編集・訂正はドラフト提示のみ。ESM 再スキャン 0・XML 実体ファイル読取 0（パス文字列は EXE 内 .rdata のみ）。会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、supermemory Daily Triage）には本契約により従わない。
