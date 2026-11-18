# v548 — UI 属性名↔ID 表の全 223 対を確定（**`FUN_0058A1C0`＝`registerAttr(name,id)` を 223 回呼ぶ属性テーブル**・`0xFA2`=`class`／`0xFA5`=`stackingtype`／`0xFAB`=`depth`）・**`0xB1399C`/`0xB139A4` は int 定数 30/890（v547 の「元 float」を訂正）**・**`591C50` の arg3 は未使用（実効 2 引数）＋`42BB90`＝`NiTArray::SetAt`（push_back ではない）**（V-548-1..3 完遂・旧残点 V-406-3 継続第78弾）

- 採番: **v548**（旧残点群消化・probe prefix **v548_1..8（8本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-548-1..3（v547 残点）** — 1=`0xB3A700`/`0xB3A704` を読む 3 head の正体＋元 float `0xB1399C`/`0xB139A4`／2=`591C50` の arg3 と `588BD0`/`58CEB0` の意味／3=type 4002/4005/4011 の UI メッセージ enum 定義元。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・RTTI のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-548-3: type 4002/4005/4011 の定義元** | **[確定]（命令＋文字列実読）＝`FUN_0058A1C0` の UI 属性テーブル** | `0x58A1C0` は **`FUN_0058A0A0(this, name, id)` を 223 回呼ぶ**（`push <id>; push <nameptr>; mov ecx,esi; call 58A0A0`）＝**UI 属性名↔ID の一括登録**。実読で **`0xFA2`(4002)=`"class"`／`0xFA5`(4005)=`"stackingtype"`／`0xFAB`(4011)=`"depth"`**。ID 域は 7 群: **4001–4097＝XML 属性**（`visible`/`clipwindow`/`alpha`/`stackingtype`/`depth`/`x`/`y`/`width`/`user0..25`/`xbuttona..start` 等）／**2001–2027＝演算子**（`add`/`div`/`eq`/`gt`/`onlyif`…）／**1001–1051＝メニュー名**（`&MessageMenu;`…）／**901–999＝要素型**（`image`/`menu`/`text`/`window`/`template`）／**3001–3004＝値型**（`name`/`value`/`src`/`trait`）／**5001–5007＝参照**（`me`/`parent`/`child`/`sibling`/`screen`/`strings`）／**101–109・6006–6008＝フラグ**（`click_past`/`prev`/`next`/`does_not_stack`）。caller は `0x58DA00` の 1 箇所。→ **v547 の「type 4002/4005/4011 は別 enum」は [確定負]**（同一属性テーブルの連番 4001–4097 内）。 |
| **V-548-1: `0xB3A700`/`0xB3A704` と元値** | **[確定]（命令）＝int の時刻キャッシュ。元値は int 定数** | `0x9FBC50`: `call 57D330; fiadd [0xB1399C]; call 9828C0(cvttsd2si=切捨て); mov [0xB3A700],eax`／`0x9FBC70`: `fild [0xB139A4]; call 57D390; fsubr; call 9828C0; mov [0xB3A704],eax`。**`0xB1399C=0x1E=30`・`0xB139A4=0x37A=890`（いずれも int。v547 の「元 float」は誤り→訂正）**。`57D330`/`57D390` は `[0xB06C4C=0x280=640]`/`[0xB06C50=0x1E0=480]`（画面解像度）依存の時刻計算（callers 各 3）。readers 3 head（`585620`/`585720`/`585820`）は `fild [0xB3A700]`/`fild [0xB3A704]` で float 化し `571F90`（描画/text 生成）へ渡す＝**メニュー/コンソールの時刻表示 [部分確定]**。`0xB13970` は **`NiTArray<Tile*>`（vtable `0xA69680`）**。 |
| **V-548-2: `591C50` の arg3 と子窓ヘルパ** | **[確定]（命令）＝arg3 は未使用（実効 2 引数）。`588BD0`=子要素 float getter／`58CEB0`=setter。`42BB90`=`NiTArray::SetAt`** | `591C50` body が読むのは **`[esp+4]`＝arg1(type)** と **`[esp+0x10]`＝arg2(float)** のみ（`[esp+0xC]` の読みは **0 件**）→ **arg3 は不読**（`ret 0xC` は 3 引数破棄の慣習）。arg1==`0xFA2`(4002="class") のときだけ動作。末尾 `591D0F`〜: `lea eax,[esp+0x10]; push eax; add ebx,0xFFFFFC17(=−1001); push ebx; mov ecx,0xB13970; mov [esp+0x18],esi(=this); call 42BB90`＝**`SetAt(NiTArray<Tile*>, index=int(arg2)−1001, value=this)`**（`42BB90` は `mov [ecx+4][index*4]=value` の slot 代入で push_back ではない。callers 992）。`588BD0(this,key)`＝**子要素 key→float getter**（BST 下降・miss は `fldz`=0.0・hit は `[node+4]`。callers **1073**）。`588C10(this,key)`＝同型の **pointer getter**（miss は `0xA2F7EC`＝空文字列 `""`。callers 57）。`58CEB0(key,float)`＝**setter**（`58B220`=node find→`58CA00`=float 書込。callers 1767）。→ `591C50` は `588BD0(0xFA5="stackingtype")` を 2 回呼び境界 `[0xA69778]=102.0`/`[0xA69768]=103.0` と比較、`58CEB0(0xFAB="depth", 584980())` で Z を設定。 |

## §2 証拠

### §2.1 `0x58A1C0` 属性テーブル（v548_5/6）
```
58A1C0: push 0x1; push 0xA6AAD8("&false;"); mov ecx,esi; call 58A0A0   ; id 1
58A9A6: push 0xFA2; push 0xA628D8("class"); call 58A0A0              ; ★ 4002 = class
58A951: push 0xFA7; push 0xA6A5B4("alpha"); call 58A0A0
58AA72: push 0xFA4; push 0xA6A4FC("clipwindow"); call 58A0A0
58AA83: push 0xFAB; push 0xA69D74("depth"); call 58A0A0              ; ★ 4011 = depth
58A995: push 0xFD0; push 0xA6A588("child_count"); call 58A0A0
… 223 対（push id; push nameptr; mov ecx,esi; call 58A0A0）
```
`58A0A0(this=ecx, name=[esp+0x1c], id=[esp+0x20])`＝内部で name の `&…;` を解釈（`0x26`='&' 分岐＝名前付きエンティティ、`0x5F`='_'、`al-0x40` の文字コード）→ id を登録。呼出側 push 順は `push id` → `push name`（callee は `[esp+0x1c]`=name, `[esp+0x20]`=id）。

### §2.2 ID 域（223 対の全数）
```
4001-4097 : XML 属性（visible/clipwindow/alpha/stackingtype/depth/x/y/width/height/user0..25/xbuttona..start）
2001-2027 : 演算子（add/sub/mul/div/eq/gt/gte/lt/lte/neq/and/or/not/min/max/mod/rand/log/ln/ceil/floor/abs/onlyif/onlyifnot/ref/user）
1001-1051 : メニュー名（&MessageMenu; .. &TextEditMenu;）
 901- 999 : 要素型（rect/text/image/3d/nif/menu/window/template）
3001-3004 : 値型（value/name/src/trait）
5001-5007 : 参照（me/parent/child/sibling/screen/strings）
101- 109  : フラグ（click_past/no_click_past/mixed_menu/prev/next/first/last/xlist/xitem）
6006-6008 : does_not_stack ほか
```

### §2.3 時刻キャッシュ（v548_7）
```
9FBBF0: push 0xB139A4; mov ecx,0xB07BF0; call 4040E0; push 0xA24A10; call 981FB4  ; atexit 登録
9FBC50: call 57D330; fiadd [0xB1399C=30]; call 9828C0; mov [0xB3A700],eax
9FBC70: fild [0xB139A4=890]; call 57D390; fsubr; call 9828C0; mov [0xB3A704],eax
9828C0: cvttsd2si eax, qword[esp]  ; float -> int（切捨て）
0xB13970 = NiTArray<Tile*>（vtable 0xA69680）
0xB06C4C=640 / 0xB06C50=480（57D330/57D390 が参照する画面解像度）
```

### §2.4 `591C50` 末尾＋`42BB90`（v548_1/8）
```
591D0F: lea eax,[esp+0x10]; push eax
591D14: add ebx, 0xFFFFFC17        ; ebx = int(arg2) - 1001
591D1A: push ebx
591D1B: mov ecx, 0xB13970          ; NiTArray<Tile*>
591D20: mov dword ptr [esp+0x18], esi   ; *eax = this
591D24: call 42BB90                ; SetAt(index, &this)
42BB90: movzx edx,word[ecx+0xa]; mov eax,[esp+4]; cmp eax,edx; jb …
42BBB6: mov edx,[esp+8]; mov edx,[edx]; mov [ecx+4][eax*4],edx; ret 8
```

## §3 判定の内訳

- **V-548-3 完遂＋訂正**: UI 属性名↔ID は **単一テーブル `0x58A1C0` の 223 対**で確定（v547 の「4002 と 4005/4011 は別 enum」は [確定負]）。`0xFA2=class`／`0xFA5=stackingtype`／`0xFAB=depth`。
- **V-548-1 完遂＋訂正**: `0xB3A700`/`0xB3A704` は **int の時刻キャッシュ**、元値 `0xB1399C=30`/`0xB139A4=890` は **int**（v547 の「元 float」を訂正）。readers 3 head はメニュー/コンソールの時刻表示 [部分確定]。
- **V-548-2 完遂**: `591C50` の arg3 は未読（実効 `open(type,arg2)`）。`588BD0`=float getter／`588C10`=pointer getter／`58CEB0`=setter。末尾は `NiTArray<Tile*>` への `SetAt(index=int(arg2)−1001, value=this)`（push_back ではない）。

## §4 残点（V-549-1..3）

1. `0xB3A700`/`0xB3A704` の物理意味（`57D330`/`57D390` の計算式と `0xB1399C=30`/`0xB139A4=890` の設定名対応）【中】
2. `0x58A0A0` の name 解釈規則（`&entity;` の内部表現）と `0x58DA00`（テーブル初期化 caller）の起動経路【中】
3. `591C50` の arg2（float→int）の意味（メニュー ID か時間か）と `SetAt` index（arg2−1001）の妥当性【中】

## §5 後続への申し送り

- **移植側 UI XML パーサ**: 属性名→ID は `0x58A1C0` の 223 対をそのまま表にできる（`class`=4002/`stackingtype`=4005/`depth`=4011/`visible`=4001/`clipwindow`=4004/`alpha`=4007/`user0..25`=4014..4039）。ID 域は 4001+ が属性、2001+ が演算子、1001+ がメニュー名、901+ が要素型。
- **メニュー open は実効 2 引数**（type, arg2）。arg1==4002("class") のときのみ動作し、`NiTArray<Tile*>`（`0xB13970`）へ `SetAt(arg2−1001, this)` で登録。子窓属性は `588BD0`（get）/`58CEB0`（set）で name-ID 越しに読み書き。
- **時刻キャッシュは起動時 1 回**（`0xB3A700=57D330()+30`、`0xB3A704=890−57D390()`、いずれも int 切捨て）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
