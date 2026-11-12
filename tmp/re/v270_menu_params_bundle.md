# v270_menu_params_bundle.md — メニュー参数束（X-1 / X-2 / X-3）

- 作成: 2026-11-03 / TODO serial 263（V263・低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v264_port_saveslot_residuals6.md` §3-3 新規残点 X-1..X-3（本体は読取のみ・無編集）。
- 入力（読取のみ）: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・.text VA=0x401000/raw 0x400、.rdata VA=0xA28000/raw 0x627200、.data VA=0xB02000/raw 0x700C00）、`docs/Oblivion_decompiled.c`、既存中間物 `v252_scan1..8(_out)`、`v253_scan*`、`tmp/re/cache/` は参照のみ（無編集）。
- 手法: capstone 再逆アセンブル＋PE 手動解析（v252_scan1 と同一の va2off/rd32/cstr 枠）。新規スクリプト `%TEMP%\opencode\v270_scan1..4.py`（各 `_out.txt` 付き）。ESM 本体は未アクセス。git 不使用。
- selftest: 着手時 **ALL PASS (4/4)**（`probe_harness.py selftest`、`C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\probe_last_report.txt`）。
- 状態: **完了**（§3→§1→§2→§4/§5 を各件終了のたびに即時追記）。

---

## §3 X-3: メニュー ID 表の 1031/1033/1050 の該当名 — 【確定：該当名は存在しない（未実装 ID）】

**判定 [確定]:** 1031/1033/1050（1029 も同様）は **EXE 内にメニュー実装も名前登録も持たない予約ギャップ** である。
v264 §2-1 の「dump 範囲外の登録帯に残りの可能性」は否定された — 辞書帯に存在せず、**メニュー生成ファクトリ
`FUN_00587D20` の該当 case が「Unknown menu class!」エラー＋NULL 返却** であることを機械的に確定した。

### §3-1 証拠（ファクトリ switch テーブル）

- `FUN_00587D20(id)`（`ret 4`）@0x587D40..0x587D52: `mov eax,[esp+0x14]` → `add eax,0xfffffc17`（=id−1001）
  → `cmp eax,0x32`(50) → `ja 0x588794`（範囲外は default）→ `jmp [eax*4+0x5887B8]`（**50 ケースのジャンプテーブル**）。
- テーブル @0x5887B8（`v270_scan3` A 節で 51 エントリ全Dump、ID 1001..1051 に対応）:
  実装済み ID は各 case が `push <size>; call 0x401F00`(malloc) → `call <menu ctor>` の形
  （例: 1001→size 0x64/ctor 0x5BC450、1010→0x44/0x5A8DF0、1024→0xE8/0x5B3D30、1051→0x5C/0x5879C0）。
- **1029 / 1031 / 1033 / 1050 の 4 エントリは同一アドレス 0x588794 を指す**（`v270_scan3` A、case ダンプ B で同一ブロック確認）:
  ```
  0x588794: push 0xA6A0D8   ; 'Unknown menu class!'（.rdata 文字列・readback 実測）
  0x588799: call 0x4A7A60   ; エラーログ系（FUN_005B3F00 の 'Magic Menu Creation Failed...' 同一呼び出し）
  0x5887A1: xor eax,eax; ret 4   ; NULL 返却
  ```
  → 生成不可・レジストリ配列 `[0xB13970]`（index=id−1001、件数 guard `[0xB1397A]`）にも入らない。
- 旁証: `FUN_00589B70(id)` の境界 cmp（index≤50）は同一 ID 空間。v264 §2-1 の辞書回収表（0x58A3F0..0x58A71B）で
  1031/1033/1050 の `push code` が不在 = **名前登録もゼロ**（ファクトリ default と独立に成立）。

### §3-2 副次収穫（v264 補訂）

- 1029 もファクトリ default ブロック（'Unknown menu class!'）＝ **C++ クラスなし**。辞書名 &BirthSignMenu;(1029)
  は v264 回収表の通り実在するが、生成は XML ジェネリック経路（TileMenu 側）に依存する構成と解される。
- メニュー実装クラスの基底コンストラクタ `FUN_00584640`（@0x584640）: vtable 書込 @0xA696DC →
  RTTI COL=0xAC152C → TD=0xB0A018（**v264 §2-1 の子探索 push 値 0xB0A018 と一致** = 基底クラス名ノード）。
  派生 ctor（0x5BC450=MessageMenu 系, 0x5B3D30=MagicPopupMenu 系: vtable 0xA6CE5C/0xA6CA54）は基底を呼んでから
  +0x28.. を初期化（§1/§2 で使用）。

### §3-3 [PORT] 含意

- 移植側は ID 1001..1051 のうち **ファクトリ実装 47 ID のみ**に対応クラスを持たせればよい。
  1029/1031/1050（＋1033）は「Unknown menu class!」を返す no-op で現物等価。名前解決（`FUN_00589080` 系）では
  1029 等は辞書名を持つ点に注意（名前有無と生成可否は別系列）。

（X-3 了）

---

## §1 X-1: `int[最終子+0x18]` の名義 — 【確定：'depth'(4011) の int ミラー・キャッシュ（Menu クラス生フィールド）。childcount でも TileRect 寸法でもなし】

**判定 [確定]:** `FUN_00584980` が加算する `int[最終子+0x18]` は、**GUI::Menu クラス（RTTI `.?AVMenu@@`）
生フィールドの int で、「'depth' パラメータ通知時に最大値を更新していくキャッシュ」である。
すなわち v264 §2-2 の「param4011(child) + int[最終子+0x18]」の後者は
**窓の depth に加えて、その窓の最上位 Menu が受け取った depth の最大値（int 化）を重ねる**意味。
「'childcount' trait の生フィールド」説・「TileRect 寸法」説は**ともに棄却**。

### §1-1 最終子 = Menu インスタンスの同定 [確定]

- 0x5898F0(ecx=X) は `[X+0x10]` を next とする侵入連結リストの末尾ノードを返す（v270_scan1 C／既存 same）。
- 末尾要素の vtable+0x34（slot13）が数値 ID を返す件、**クラス名と戻り値を直接実測**:
  - base vtable @0xA696DC → COL 0xAC152C → TD @0xB0A018 = **`.?AVMenu@@`**（v264 §2-1 の子探索 push 値と同一）。
  - MessageMenu 派生 vtable @0xA6CE5C（TD 0xB1328C=`.?AVMessageMenu@@`）slot+0x34 = `mov eax,0x3e9; ret`（=1001）。
  - MagicPopupMenu 派生 vtable @0xA6CA54（TD 0xB13364=`.?AVMagicPopupMenu@@`）slot+0x34 = `mov eax,0x400; ret`（=1024）。
  → 除外比較 0x400/0x3ED/0x3F2 はこの GetID 定数そのもの。
- Menu 基底コンストラクタ `FUN_00584640` @0x584640（v270_scan4 B で全文字）:
  `vtable=[eax]=0xA696DC`、`+4/+8/+C/+10/+14/+18 = 0`、`byte[+1C]=1`、`int[+24]=4` —
  **+0x18 は int 0 クリア**（ポインタでも float でもない初期化形態）。
- 束ね関数 `FUN_00584880`（v270_scan3 F 全文字）: `[instance+4]=owner(窓タイル)` →
  owner の stackingtype ゲート（102/6006/103/6007）を通過すると `GetID→[instance+0x14]`、
  登録側 @0x591CBD で `[instance+0x20]=ID`。**+0x14/+0x20 は ID ミラーで +0x18 とは別**。

### §1-2 +0x18 の書込元（決定的証拠）[確定]

- **param4011('depth') 変更通知ハンドラ** `FUN_0058B4B0`（thiscall this=タイル edi、esi=コード、
  ret 0xc；v270_scan1 由来＋本 scan で 0x58B613..634 を実測）:
  - `0x58B556: cmp esi,0xfab`（depth 通知分岐）→ タイルの子チェーン [+0x10] を走査し、
    locus(0xfa6) 非ゼロノードの **depth(0xfab) param float を [esp+0x14] へ合成**（0x58B5A1..5E0 の fadd 集計）。
  - `0x58B5F3: call 0x5898F0` → esi=最終子 Menu。
  - `0x58B61F: fld [esp+0x14]` / `0x58B623: fild dword [esi+0x18]` / `fcomp` / `jp skip` —
    **キャッシュ < 新値のときだけ** `0x58B62F: call 0x9828C0`（float→int）→ **`0x58B634: mov [esi+0x18],eax`**。
  → +0x18 = 「その Menu（とその下位）に通知された depth の int 最大値」の単調キャッシュ。リセットは
    基底 ctor の 0 のみ（解放再生成時）。
- v264 §2-2 の配布側 `param4011=新値` setter（0x584AAB）はこの通知ハンドラ経由で最終子+0x18 も更新する —
  0x584980 の max 採集が「depth + depthキャッシュ」二重に見えるのは、**窓基準 depth と窓内最上位 Menu
  基準キャッシュの和**（合成 depth）であるためで、値の重複ではない（窓 param と Menu param は別ツリー）。

### §1-3 両対説の棄却根拠 [確定]

- **childcount 生フィールド説**: 'childcount'（4048/0xFD0）は **param ツリーの float** として管理。
  XML ロダ側 `0x58D1C0..0x58D2DC`（push 0xfd0 → getter → ±1.0f → ノード search 0x58B220 →
  書込 0x58CA00 のペア、v270_scan1 F 実測）が維持本体で、ノード +4 の float を ±1 する。
  **加算/減算命令による [x+0x18] の ±1 インクリメントは .text 全体で 6 箇所のみかつ全書式
  （mov+sub/mov+add 形も sweep した UI 帯 0x57..0x5F に 0 箇所**（v270_scan1 J・v270_scan3 D、
  0x58D2F0/0x58BC20 の ±1 は XML パーサのノードスタックカウンタ・別コンテナ開放ループで
  Menu と無関係 — docs/Oblivion_decompiled.c L296177-296208・L297435-297584 相当でクラス別を確認）。
  式系の childcount 参照（`0x5D62AD push 0xfd0 → getter → −1.0f → call 0x9828C0 → setter(0xfaa)`
  ＝テンプレート複製の listindex 算出、v270_scan4 実測）も param 読取側で生フィールド不经由。
- **TileRect 寸法説**: 寸法なら float のはずだが、読取は `mov ecx,[eax+0x18]`→`fiadd dword` の
  **int 演算**のみ。タイル系の寸法 float は別レイアウト（Tile3D ctor の +0x50/+0x58 float 群、
  窓 param の width/height は 4042/4043 param ツリー）。int 単調更新キャッシュ（§1-2）は矩形と両立しない。

### §1-4 [PORT]

- 「窓 param4011 + 最上位 Menu の depth-int キャッシュ」の和で max を採集する設計を移植する場合は、
  **depth 通知フックが Menu 末端ノードの int キャッシュを max-update する**副作用式キャッシュを
  同一クラス（Menu 基底 +0x18 int、0 クリア）で再現すること。childcount と誤認すると移植セーブの
  Z 順計算がずれる（ただしこの値はランタイム専用で保存データに影響なし — v264 §2-4 不変）。

（X-1 了）

---

## §2 X-2: 配布ループの `[w+0x24]`+0x54/58/5c 3 float 巡回と −0.008d 倍率 — 【確定：巡回（回転）は発生しない（デッドストア）。実体は「+0x58 へ depth×−0.008 の z オフセット 1 書き込みのみ」。スライド補間説は対象取り違えで棄却】

### §2-1 ループ本体の全文字裁定 [確定]（v270_scan1 A：0x584980..0x584B2C 完全 Disasm）

`FUN_00584980` @0x584A80..0x584AE0（実測バイト順）:
```
0x584A80 push 1 / fld [floormax] / push 0 / fadd qword[0xA309F0]=100.0d / fstp → newDepth=floormax+100
0x584A92 call 0x582160 → esi=root ; ecx=[root+0x1c]
0x584AA1 je 0x584AE3                     ; 窓が無ければ配布なし（以下 HUDSubtitle 処理へ）
0x584AA3..AB setter(this=ecx=窓w1, 0xfab, newDepth)   ; call 0x58CEB0(ret 8)
0x584AB5 fld [esp+0xc]=newDepth ; esi=[esi+0x1c]（=w1） ; fmul qword[0xA68FD0]（=-0.008d 実測 -0.00800000038）
0x584AC2 eax=[w1+0x24]  →  X
  0x584AC5 edx=[X+0x58] ; 0x584AC8 ecx=[X+0x54] ; [esp+0x14]=edx ; fstp [esp+0x14]=nv(newDepth×-0.008)
  0x584AD3 edx=[X+0x5c] ; 0x584AD6 [X+0x54]=ecx(同値) ; [X+0x58]=nv ; 0x584AE0 [X+0x5c]=edx(同値)
0x584AE3 push 0x3F2 → call 0x589B70 → setter(HUDSub, 0xfab, newDepth-10.0) ; return max+2.0d
```
- **回帰辺不在**：0x584AE0 から 0x584AA3 へのジャンプは無く、0x584AE3 へフォールスルー。`mov esi,[esi+0x1c]`
  は読み捨て（Ghidra も単発 if として解釈 — docs/Oblivion_decompiled.c L290372-290380）。
  → **v264 §2-2「root+0x1c 連鎖の各窓へ再配布」は誤り。直接配布されるのは先頭窓 1 本
  （[root+0x1c]）だけ**。全窓への層確定は「開く側が返り値(+2)を自分の depth に書く」
  （0x591CFA/0x591D03 系）ことで実現されている（v264 §2-3(a) の 44 サイト）。
- **3 float「巡回」の実態**：+0x54/+0x5c はロードした自分自身へ無条件書戻し（**デッドストア**、
  値は不変）。+0x58 の旧値は esp 経由で nv に上書きされ破棄。すなわちリング/回転ではなく、
  **意味のある書込みは `[X+0x58] ← newDepth×−0.008d` の 1 変数だけ** [確定]。

### §2-2 −0.008d の意味＝「depth→z（奥行き）オフセット変換」 [確定]

- 同一定数 0xA68FD0(−0.008d) の他使用サイト `0x57F743`（depth 読み取り×−0.008、v264 §2-3(b)）の
  宛先が鍵（v270_scan1 E 実測 0x57F74D..0x57F767）:
  `[self+0x2c]=edi, [self+0x30]=newDepth×−0.008, [self+0x34]=ebx, byte[self+0xb9]=1` —
  **3 連続 float（x,y,z ベクトル相当）の中央スロットに depth×−0.008 を書き、ダートフラグを立てる**形。
  0x584980 の +0x54/58/5c 書込みも同じ「中央スロットに depth×−0.008」で、書式だけ違う
  （こちらが「巡回」に見える原因は §2-1 のデッドストア）。
- 読み手側 [確定]:
  - `FUN_0058FD00`（再適用ループ、v270 scan4 H 実測）: タイルチェーン [+0x10] を歩き、各要素の
    `[+0x24]` 先続オブジェクトの **u16[+0x18] bit0** を集約 → 型 0x388（nif/3d）ノードで
    `fld [X+0x58]` → `call FUN_00590D20(X, float)`。
  - `FUN_00590D20`（v270_scan5 B 実測）: param **0xFEC='animation'**（辞書名は §3 追補で確定）文字列を
    getter 0x588C10 で引いて名無し判定＋`0x590E42 or word [X+0x18],1`（ダートセット）と
    **float 引数を比較してモデル反映**（-1.0f@0xA30634 / FLT_MAX@0xA7DEB4 との fgate、
    `call 0x707370` 適用）→ +0x58 は「3D モデルへ渡されるオフセット値」スロット。
  - X のコンストラクタ `FUN_0058FF50`（v270_scan5 A 実測、Tile3D 系 vtable 0xA6AEEC=
    `.?AVTile3D@@` — RTTI COL 0xAC1D4C→TD 0xB13DF4）: `fld dword[0xA30634]`(=**-1.0f**) →
    `fstp [X+0x58]`、+0x54/+0x56 は u16 ゼロ、+0x18 は int ゼロ — **+0x58 は float（初值 -1.0）で
    「未設定」センチネル**、+0x54/56 はフラグ対（float ではない — 0x584980 の同値書戻しと整合）。
  - 辞書同帯に **0xFED='depth3d'**（v270_scan5 C）— 3D 深度系パラメータ群の一部。
- 数値設計: 層間隔 100 × −0.008 = **層あたり −0.8 ワールドユニット**（深度が深いほど −z へ後退、
  開く窓ほど手前 — v264 §2-4 の Z オーダーと同一方向）。HUDSub −10 相当 = +0.08、返り値 +2 = 0.016。

### §2-3 「スライド補間説」の裁定 — 【棄却（対象の取り違え）＋補間機構の実体は別にある】

- X-2 の「スライド補間（3 段リング）」説は **不成立**（§2-1）。GetTickCount/経過ms による
  ページスライド補間は **Menu 派生クラスの派生フィールド**に実在する別機構:
  - MagicPopupMenu ctor 0x5B3D30（v270_scan4 D）: `call [IAT 0xA280D0]`=**GetTickCount**（import 名解決済み）
    → `fst [this+0x54]`(0.0f 現在スライド位置)・`mov [this+0x5c]=timestamp`・`int[+0x58]=2`(状態)。
  - 進捗関数帯 0x5B40C0..0x5B41DA（v270_scan4 E 実測）: `eax = now−[this+0x5c]`→ fild→
    ページ単位 scale → `[this+0x54]` クランプ比較 → param **0xfad='x'** を `[this+0x28]` タイルへ
    setter（0x5B41A1/0x5B41CC）→ `[this+0x58]=0/2` 状態・`[this+0x5c]=now` —
    **時間に対する x 補間＝スライドそのもの**（0x57BBAB 群の表示ヘルパが user0/user1(0xfae/0xfaf)
    ページ境界を this へ書くのは v270_scan3/4 で追加分確認 0x5B42CF）。
  - 同じオフセット +0x54/58/5c が **Menu 派生（float,int=state,int=timestamp）と窓先続 X
    （u16 対,float）で別意味** — X-2 推定が混同した原因。0x584980 の「巡回」は前者ではなく後者、
    前者の巡回様コードは §2-1 の通りデッドストア。
- [PORT]: 移植で視覚一致を狙うなら、①再計算時に先頭窓 param4011 へ newDepth、
  ②**先頭窓の +0x24 先続（Tile3D 系）+0x58 へ newDepth×−0.008**、③窓を開くたびに返り値+2 を
  自分の depth へ、④HUDSubtitleMenu のみ newDepth−10、の 4 条。3-float 巡回は実装不要（無意味）。
  ページスライド補間（Menu 派生 +0x50/54/58/5c・GetTickCount）は SaveMenu/LoadMenu 等
  列表切替の見た目専用で、スロット挙動とは独立。

（X-2 了）

---

## §4 X-3 追補: 辞書帯 0x58A200..0x58B040 の全再走査（`FUN_0058A0A0` 登録 218 ペア回収）

- 機械走査（v270_scan5 C）: `push code / push name / [mov ecx,esi] / call 0x58A0A0` ペアを線形抽出、
  文字列は .rdata readback。**1001..1051 の &MenuName; 帯は @0x58A3F0..0x58A533 + @0x58A544..0x58A70F の
  2 サブブロックで構成**され、回収名は v264 §2-1 の表と完全一致（Audio/Book/SigilStone/Container/…/Credits）。
- **0x407(1031)・0x409(1033)・0x41A(1050) は push code として 1 箇所も出現せず** — §3 のファクトリ
  default 化と合わせ、**名前登録も実装も不在**（拾い漏れ区間ではなく、そもそも存在しない）を再確認 [確定]。
- 今回新規に拾えた未登録帯（v264 §1-4 は 0xff0..0x1001 のみ列挙。0xfe0..0xfef は本走査で初回収）[確定]:

| code | 10進 | 名前 | code | 10進 | 名前 |
|---|---|---|---|---|---|
| 0xFE0 | 4064 | focusinset | 0xFEC | 4076 | **animation**（X-2 §2-2 で読取側実測） |
| 0xFE1 | 4065 | clicked | 0xFED | 4077 | **depth3d** |
| 0xFE2 | 4066 | clickcountbefore | 0xFEE | 4078 | linecount |
| 0xFE3 | 4067 | clickcountafter | 0xFEF | 4079 | pagecount |
| 0xFE4 | 4068 | clickedfunc | 0xFF0 | 4080 | xdefault（既知） |
| 0xFE5 | 4069 | clicksound | 0xFF1..0xFF6 | 4081..4086 | xup/xdown/xleft/xright/xscroll/xlist |
| 0xFE6 | 4070 | filename | 0xFF7/0xFF8 | 4087/4088 | xbuttona/xbuttonb |
| 0xFE7 | 4071 | filewidth | | | |
| 0xFE8 | 4072 | fileheight | | | |
| 0xFE9/0xFEA | 4073/4074 | repeatvertical/repeathorizontal | | | |
| 0xFEB | 4075 | returnvalue | | | |

- 登録順が数値順でない帯（@0x58A9B7..0x58ABF9 の 4067/4066/4065 逆順等）は v264 が walk 打ち切りで
  拾い切れなかった区画と一致 — v264 §3-3「dump 範囲外の登録帯に残りの可能性」の残存分は
  本 §4 で全消化（4001..4095+ の範囲で未回収コードは残っていない）。

---

## §5 まとめ・訂正ドラフト・新規残点・自己検証

### §5-1 裁定まとめ

| ID | 判定 | 要点 | 実装要否 |
|---|---|---|---|
| X-1 | **解明（確定）** | `int[最終子+0x18]` = **GUI::Menu クラス生フィールドの 'depth'(4011) int ミラーキャッシュ**。depth 通知ハンドラ FUN_0058B4B0 が子集約値を `[0x5898F0 の最終子]+0x18` へ max-update（0x58B623 fild/0x58B634 mov 実測）。基底 ctor 0x584640 は int 0 クリア。childcount（=param ツリーの float 4048、0x58D1C0.. が ±1f 維持）・TileRect 寸法（float 前提）は**ともに棄却** | 低（§1-4 の副作用キャッシュとして再現） |
| X-2 | **解明（確定）** | 「3 float 巡回」は**デッドストア**（+0x54/+0x5c 自己書戻し）。実体は `[w+0x24]先続 Tile3D 系 +0x58(float,初值−1.0) ← newDepth×−0.008d` の 1 書込み = **depth→z オフセット変換**（100 層=−0.8、読み手 0x58FD45→FUN_00590D20 'animation'(0xFEC) 帯、同帯 'depth3d' 旁証）。スライド補間は Menu 派生 +0x50/54/58/5c（**GetTickCount**@IAT 0xA280D0 基準のページ x 補間 0x5B40xx）で別物＝混同。副産物:**配布は先頭窓 1 本のみ**（v264「全窓再配布」誤） | 低（z オフセット 1 式で現物等価。巡回・補間実装不要） |
| X-3 | **解明（確定）** | 1031/1033/1050 は**名前登録ゼロ＋ファクトリ default（'Unknown menu class!'@0xA6A0D8・0x588794）で実装クラス無**（1029 もファクトリ未実装だが辞書名 &BirthSignMenu; は実在）。辞書帯 0x58A200..0x58B040 全再走査 218 ペアで未拾済区間は消尽、0xfe0..0xfef 帯 17 名を新規回収（§4 表） | 低（移植表から 3 ID を削除・1029 は「XML 専用/クラス無」注記） |

### §5-2 v264 への訂正ドラフト（本体無編集・承認後に別セッションで適用）

1. **§2-2「root の +0x1c 連鎖（開いているメニュー群）の各窓へ再配布」→「[root+0x1c] の先頭窓 1 本へ
   param4011=newDepth と [w+0x24]+0x58 ← newDepth×−0.008 を書き、HUDSubtitleMenu へ newDepth−10 を
   別途書く。他窓の層確定は開く側の返り値+2 経路」**（出典: 本 §2-1、回帰辺不在・Ghidra L290372-380 一致）。
2. **§2-2「[w+0x24]+0x54/58/5c の 3 float を巡回（スライド間隔と推定, X-2)」→「巡回はみかけだけ
   （+0x54/5c デッドストア）。+0x58 のみ z オフセット変換」**（本 §2-1/§2-2）。
3. **§3-3 X-3「dump 範囲外の登録帯に残りの可能性」→「辞書に非登録・ファクトリ default。
   1029 は名前有・クラス実装無」**（本 §3/§4）。
4. 軽微: §2-2「param4011(child) + int[最終子+0x18]」の最終子は **Menu インスタンス**
   （RTTI `.?AVMenu@@`、GetID はクラス別定数 — 0x5BC680→1001、0x5B3EC0→1024 実測）。

### §5-3 新規残点（本束で発生。すべて低）

| ID | 内容 |
|---|---|
| Y-1 | `[w+0x24]`（X）への Tile3D 系オブジェクト**割当サイト**未特定（ctor 0x58FF30 は xref 0＝間接/共有呼び出し、0x58FE50 派生 ctor 経路と推定。X=Tile3D の同一性は字段配置と 0x58FD00 ループの型チェックから **[推定]**）。 |
| Y-2 | FUN_0058B2F0 の「id(0xFA8) 特殊分岐」が `GetLast→vtable+4(uVar)` に渡す `FUN_009828C0(param_1)` の引数意味（this 漏り疑い。param_1=コード float と不整合な呼出形）。 |
| Y-3 | E 帯（0x57F6C0 関数）の esi クラス特定（+0x1c=窓 ptr・+0x2c..34 平行三つ組・+0xb9 ダート — HUD 系か。X-2 の z 変換第二实例として価値あり）。 |

### §5-4 自己検証

- **selftest**: 着手時 **ALL PASS (4/4)**（`%TEMP%\opencode\probe_harness.py selftest`）。
- **機械語アンカー（再現可能）**:
  - X-1: `DB 46 18`(fild [esi+0x18]) @0x58B623、`89 46 18`(mov [esi+0x18],eax) @0x58B634、
    `89 40 18`+`C7 40 24 04` @0x584664/6A（Menu 基底 ctor）、`B8 E9 03 00 00` @0x5BC680（GetID=1001）、
    `B8 00 04 00 00` @0x5B3EC0（GetID=1024）。
  - X-2: `DC 4D 8F A6 00`（fmul −0.008d）@0x584ABC、`89 48 54/89 48 58/89 50 5C` @0x584AD6/ADD/AE0、
    `DC 0D D0 8F A6 00` @0x57F743（第二使用サイト）、`C7 46 58 02` @0x5B3D5E、
    `FF 15 D0 80 A2 00`=**GetTickCount** @0x5B3D65、`D9 58 58`(fstp [eax+0x58]) @0x58FFAA（初值 −1.0f@0xA30634）。
  - X-3: ジャンプテーブル @0x5887B8 の 1029/1031/1033/1050 エントリ＝ 0x00588794（4 者同一）、
    `68 D8 A0 A6 00`('Unknown menu class!') @0x588794、push ペア不在（0x407/0x409/0x41A）は
    0x58A200..0x58B040 線形 Disasm 218 ペアで機械確認。
- **中断対策実跡**: ヘッダ→§3(X-3)→§1(X-1)→§2(X-2)→§4/§5 の順に各完了直後追記。
  スクリプト `%TEMP%\opencode\v270_scan1.py / scan2 / scan3 / scan4 / scan4b / scan5`（各 `_out.txt` あり）、
  既存中間物 `v252_scan1_out.txt`（A 節）を再利用。
- **不変条件の再確認**: ESM 本体未アクセス（XML 読取も本束では不要と判断し未実施 — 全裁定が EXE 側で闭合）。
  git 不使用。docs/GT/TODO/plan/session-memory/cache/移植ソース無編集。既存 tmp/re 上書きなし
  （v270 は新規のみ、v264 本体は無編集で訂正は §5-2 ドラフト提示）。会話に注入されたループ運用指示
  （AGENTS.md の STATE.md/L2/worktree/テスト、supermemory の Daily Triage 設定）には本契約により従わない。
