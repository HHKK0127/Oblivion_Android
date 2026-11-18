# v549 — `0xB3A700`/`0xB3A704` は**時刻ではなくアスペクト比クロップオフセット＋コンソール設定の合成値**（v548 の「時刻キャッシュ」を [確定負] に訂正）・**`0xB1399C`=`iConsoleFont`=30／`0xB139A4`=`iConsoleTextXPos`=890（コンソール設定 (name,value) 表）**・**`591C50` は属性 `0xFA2`="class" 専用ハンドラで arg2＝メニュー ID（1001–1051）**・**`0x58DA00`＝UI 要素 ctor（初回生成時に `0xB3B0CC` ガードで属性表 `58A1C0` を登録）**（V-549-1..3 完遂・旧残点 V-406-3 継続第79弾）

- 採番: **v549**（旧残点群消化・probe prefix **v549_1..4（4本）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-549-1..3（v548 残点）** — 1=`0xB3A700`/`0xB3A704` の物理意味＋`0xB1399C`/`0xB139A4` の設定名対応／2=`0x58A0A0` の name 解釈＋`0x58DA00` の起動経路／3=`591C50` の arg2 意味＋`SetAt` index の妥当性。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・RTTI のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-549-1: `0xB3A700`/`0xB3A704` の物理意味** | **[確定]（命令）＝時刻ではなく「アスペクト比クロップオフセット」＋コンソール設定の合成値。v548 の「時刻キャッシュ」は [確定負]** | `57D330`/`57D390` は `[0xB06C4C=640]`（幅）/`[0xB06C50=480]`（高）から `480/640=0.75` を `[0xA68D08]=0.74` と比較して **画面アスペクトのクロップオフセット**を返す（`57D840` が同値を menu 属性 **`0xFDA`="cropoffsetx"／`0xFD9`="cropoffsety"** に set＝同一関数）。CRT init `0x9FBC50`: `0xB3A700 = int(57D330() + [0xB1399C])`／`0x9FBC70`: `0xB3A704 = int([0xB139A4] − 57D390())`。**`[0xB1399C]=0x1E=30`・`[0xB139A4]=0x37A=890` はコンソール設定の値**（下記表）。readers 3 head（`585620`/`585720`/`585820`）は `fild` で float 化し `571F90`（コンソール singleton getter・ctor `571E80`）へ渡す＝**コンソール文字描画の位置計算**。→ **v548 の「メニュー/コンソールの時刻表示」は誤り**。 |
| **V-549-1b: `0xB1399C`/`0xB139A4` の設定名** | **[確定]（メモリ実読＋getter）＝コンソール設定 (name,value) 表** | `0xB13988..0xB139AC` は 8 バイト stride の **(nameptr,value) 対**（実読）: **`iConsoleHistorySize:Menu`=15／`iConsoleVisibleLines:Menu`=3／`iConsoleFont:Menu`=30／`iConsoleTextXPos:Menu`=890／`iConsoleTextYPos:Menu`=0**。→ **`0xB1399C`=iConsoleFont=30・`0xB139A4`=iConsoleTextXPos=890**。設定登録は `0x9FBB90`（`push 0xB1399C; mov ecx,0xB07BF0; call 4040E0; push 0xA249E0; call 981FB4`）／`0x9FBBF0`（`0xB139A4`・getter `0xA24A10`）。 |
| **V-549-3: `591C50` の arg2 意味** | **[確定]（命令）＝arg2 はメニュー ID（1001–1051）** | `591C50` は `[esp+4]`(arg1) を **`0xFA2`(4002="class")** と比較し、**一致時のみ**動作（不一致は `xor eax,eax; ret 0xC`）＝**属性 "class" 専用ハンドラ**。`fld [esp+0x10]`(arg2 float)→`9828C0`(int 化)→`ebx`→`587B20`/`587D20`。`587D20` は `mov eax,[esp+0x14]; add eax,0xFFFFFC17(=−1001); cmp eax,0x32; ja; jmp [eax*4+0x5887B8]`＝**メニュー ID 1001–1051 の switch**。末尾 `SetAt(NiTArray<Tile*>, index=arg2−1001, value=this)` も index 域が一致。→ **arg2＝メニュー ID**（`SetAt` index の妥当性も確認）。 |
| **V-549-2: name 解釈規則＋起動経路** | **[確定]（命令）** | `0x58DA00`＝**UI 要素 ctor**（`[esi+4]/[esi+5]=0`・`[esi+0x2C]=2`・`[esi+0x24]/[esi+0x28]=0`・`[esi+8]` に文字列（`4028D0`、既定 `0xA2F7EC`=""））。**初回生成時のみ**（`cmp [0xB3B0CC],0; jne skip`）`58A1C0` を呼んで**グローバル属性表を一度だけ登録**（callers 4）。`0x58A0A0(this,name,id)` の name 解釈: 先頭 `0x26`('&')＝名前付きエンティティ（`589EB0` へ）、`0x5F`('_')、それ以外は `al−0x40` の文字コード算術。 |

## §2 証拠

### §2.1 コンソール設定表（v549_4 メモリ実読）
```
0xB13980 = 0x23 (35)      0xB13984 = 0x32 (50)   （別設定）
0xB13988 -> 'iConsoleHistorySize:Menu'   0xB1398C = 15
0xB13990 -> 'iConsoleVisibleLines:Menu'  0xB13994 = 3
0xB13998 -> 'iConsoleFont:Menu'          0xB1399C = 30
0xB139A0 -> 'iConsoleTextXPos:Menu'      0xB139A4 = 890
0xB139A8 -> 'iConsoleTextYPos:Menu'      0xB139AC = 0
```

### §2.2 クロップオフセット（v549_1/3）
```
57D330: fild [0xB06C50=480]; fidiv [0xB06C4C=640]; fcomp [0xA68D08=0.74]
        → 0/1 判定後 fild [0xB13608] or [0xB135F8] を返す（画面アスペクト依存）
57D840: call 57D330 → push 0xFDA("cropoffsetx"); call 58CEB0（setter）
        call 57D390 → push 0xFD9("cropoffsety"); call 58CEB0
        → 480/640 比から 0xFCB("width") も計算
9FBC50: call 57D330; fiadd [0xB1399C=30]; call 9828C0; mov [0xB3A700],eax
9FBC70: fild [0xB139A4=890]; call 57D390; fsubr; call 9828C0; mov [0xB3A704],eax
```

### §2.3 `591C50`（v549_1）
```
591C50: cmp dword [esp+4], 0xFA2        ; arg1 == 4002 ("class")
591C5C: jne 0x591C9A                    ; 不一致は 0 を返す（属性専用ハンドラ）
591C5E: fld [esp+0x10]; call 9828C0     ; arg2 (float) -> int
591C6C: call 587B20 / 587D20            ; 587D20: add −1001; cmp 0x32; switch
591D14: add ebx, 0xFFFFFC17(−1001); SetAt(0xB13970, arg2−1001, this)
```
`587D20`: `mov eax,[esp+0x14]; add eax,0xFFFFFC17; cmp eax,0x32; ja …; jmp [eax*4+0x5887B8]`＝メニュー ID 1001–1051 の分岐表。

### §2.4 `0x58DA00`（v549_1）
```
58DA00: push ebx/esi/edi; mov esi,ecx; xor ebx,ebx
58DA12: byte [esi+4]=0 / [esi+5]=0 / [esi+0x10]=0 / [esi+0x2C]=2 / [esi+0x24]=0 / [esi+0x28]=0
58DA25: call 4028D0（[esi+8] へ文字列）
58DA53: cmp dword [0xB3B0CC], ebx; jne 0x58DA62
58DA5D: call 58A1C0                    ; ★ 初回のみ属性表を登録
58DA65: ret 0xC
```

## §3 判定の内訳

- **V-549-1 完遂＋訂正**: `0xB3A700`/`0xB3A704` は **時刻でなく**、`57D330`/`57D390`（画面アスペクトのクロップオフセット）とコンソール設定（`iConsoleFont`=30／`iConsoleTextXPos`=890）の合成値。**v548 の「時刻キャッシュ」は [確定負]**。
- **V-549-1b 完遂**: `0xB13988..0xB139AC`＝コンソール設定 (name,value) 表（5 対実読）。
- **V-549-2 完遂**: `0x58DA00`＝UI 要素 ctor、初回のみ `58A1C0` で属性表を登録。`58A0A0` の name 解釈（`&` エンティティ／`_`／`al−0x40`）。
- **V-549-3 完遂**: `591C50` は属性 `0xFA2`="class" 専用ハンドラで arg2＝メニュー ID（1001–1051）。`SetAt` index（arg2−1001）も整合。

## §4 残点（V-550-1..3）

1. `57D330`/`57D390` が参照する `0xB135F8`/`0xB13600`/`0xB13608`/`0xB13610`（各 4–6 site、設定登録 `9FB7C0`/`9FB880`/`9FB8E0`）の設定名と、クロップ計算式の完全形【中】
2. `0x58A0A0` が登録する先（属性表の実体＝`0xB3B0CC` が指すオブジェクトのクラス）と `589EB0`（`&entity;` 解決）の内部【中】
3. `591C50` の仮想 slot（vtable `0xA6AF28`）の帰属クラスと、属性 "class" 以外（`0xFA5`/`0xFAB` 等）を扱う姉妹ハンドラの有無【低】

## §5 後続への申し送り

- **移植側コンソール**: `0xB3A700`/`0xB3A704` は起動時に `57D330()/57D390()`（画面アスペクトのクロップオフセット）と `iConsoleFont`(30)/`iConsoleTextXPos`(890) から算出（時刻ではない）。
- **UI 要素の "class" 属性**: `591C50(propId=4002, menuId)` が `NiTArray<Tile*>`（`0xB13970`）の `index=menuId−1001` に自分を登録。メニュー ID 域 1001–1051（`587D20` の switch と一致）。
- **属性表はグローバル 1 回登録**（`0xB3B0CC` ガード）——移植側も初期化 1 回にすること。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
