# v275 — UI 残点第2束（3件）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-04 / TODO serial 268（V268・低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0・成果物=this 1 ファイル）**
- 出典残点: `tmp/re/v273_ui_misc_residuals.md` §まとめ「本束で発生した新残点」W-1..W-3
- 入力（読取のみ）: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・.text VA=0x401000/raw 0x400、.rdata VA=0xA28000/raw 0x627200、.data VA=0xB02000/raw 0x700C00）、`tmp/re/cache/named_funcs.txt`、`tmp/re/v273_ui_misc_residuals.md` 既出証拠、中間物 `%TEMP%\opencode\v273_*` 再利用。
- 前提の既知確定（本束で再検証しない）: FUN_00590420=Tile::ReadFile・タグ→typecode 全量表（rect 901/image 902/text 903/3d=nif 904/menu 905/window 907/template 999）・TileText slot2=malloc 0xDC＋NiNode 素 ctor 0x70B780・FUN_00579320=IM∧先頭窓ガードの位置設定透過（実 site 0x4053CA/0x5AF8DE）・esi=InterfaceManager（[0xB3A6E0]・+0xb9 ダート機構）。
- selftest: 着手時 **ALL PASS (4/4)**（`probe_harness.py selftest` → bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）。
- 中間物: `%TEMP%\opencode\v275_w1.py/_out`（ゲート近傍全 abs 参照＋rep 併走走査・要修正判定あり）、`v275_w1b`（3 受点全文字＋ヘルパ）、`v275_w1c`（表の名前文字列）、`v275_w1d/_out`（**再開型全 .text 線形走査**＝v273 の単発線形走査が最初のundecodableで中間停止していた欠陥を修正し、表全域の書込/登録 282 参照を全量取得）、`v275_w1e`（設定 DB 0xB07BF0 の vtable/profile IAT）、`v275_w1f`（GetPrivateProfile* 全使用点＋マネージャ vtable）、`v275_w1g/w1h`（FUN_004a8800 冒頭・CRT テーブル実測）。
- 会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、supermemory Daily Triage）には本契約により**従わない**。

---

## 件1 W-1: ゲート byte [0xB06C74] の実行時間接書込の有無＋アドレス受取 3 サイトの先追跡 — 【確定：**[0xB06C74] = INI 設定マネージャ（obj @0xB07BF0・vt 0xA404F8）に登録された設定エントリ `{value@0xB06C74; const char* name@0xB06C78='bFull Screen:Display'}` の value 部**（出荷 .data 値 1=全画面既定）。実行時書込は**存在する**：汎用 INI ローダ **FUN_004a8800**（slot+0x10・E8 呼び主 0＝仮想経由）がエントリポインタ経由で `mov byte [esi],al` @**0x4A88DF**（b 型）／`mov dword [esi],eax` @**0x4A8AB3**（i 型）へ書込（GetPrivateProfileIntA IAT 0xA280E4 の直後）。アドレス受取 3 サイトは全て静的 init/exit 機構（実行時書込元でない）：0x9DCC60=**_initterm 表**（.rdata 0xA288DC）経由の起動時登録（0x4040E0 upsert＋atexit 0x981FB4→FUN_00a18bd0）、0xA18BD0=終了時に +0x20c リスト（0xB07CFC）から 0x65C620 でノード除去＋キーストリング 'S' マジック条件 free、0x9B0520=終了クリーンアップ表（.rdata 0xADC83C・0x19930522 ノード）の dtor thunk＝[entry+4] free 条件分岐。→ **ホスト1 FUN_00405370 の達可判定にのみ関係する事実は「4 番目のゲート `cmp byte [0xB06C74],0 / jne 0x4053D8`（0x405393/9A 実測）は bFull Screen==0、すなわちウィンドウモードでのみ成立」に解消**（全画面では v273 の [未確定] は「ini 由来的に変化あり」で確定化）】

### 1-1. ブロックの正体：INI 設定エントリ配列（`v275_w1b` D＋`v275_w1c` A）[確定]

- 0xB06C54..0xB06D1C は **8 バイトstride の {value dword @X; const char* "名前:セクション" @X+4}** エントリ列（受取サイトが push するのは常に value 部＝エントリ base）。名刺実測（.rdata）:

| base | name([base+4]) VA | 文字列 | .data 初期値 |
|---|---|---|---|
| 0xB06C54 | 0xA3E2F0 | `iAdapter:Display` | 0 |
| 0xB06C5C | 0xA3E304 | `iSize W:Display` | 640 |
| 0xB06C64 | 0xA3E314 | `iSize H:Display` | 480 |
| 0xB06C6C | 0xA3E324 | `bIgnoreResolutionCheck:Display` | 0 |
| **0xB06C74** | **0xA3E344** | **`bFull Screen:Display`** | **1** ← 本残点のゲート |
| 0xB06C7C | 0xA3E35C | `iLocation X:Display` | 0 |
| 0xB06C84 | 0xA3E370 | `iLocation Y:Display` | 0 |
| 0xB06C8C.. | … | `iNPatches/iNPatchPOrder/iNPatchNOrder/bUse Shaders/bUseRefractionShader/bDrawShadows/bDoCanopyShadowPass/bDoImageSpaceEffects/bDoAmbientPass/bDoDiffusePass…`（0xB06D1C まで継続） | 混在 |

- v273 の「設定ブロック先頭バイト/BSString 様」推定は**訂正**：0xB06C74 は単独 flag ではなく **bFull Screen 設定の実storage の低バイト**。読み 7 点（0x405393・0x494E0B・0x497BF0・0x497D63・0x498133・0x4986FA・0x57E7DA）は全て `cmp byte [g],0` 族（+0x4981AB/0x4983CD の al/bl 変種）＝「ウィンドウモード時」条件判定として意味完結。

### 1-2. 直接書込 0 の再確認と、v273 線形走査の欠陥修正（`v275_w1d` A）[確定]

- v273 は「.text 直接書込 0」としたが、その走査器は capstone の一発 `disasm()` が**最初の undecodable バイトで停止**する実装だった（w1 の 22 参照しか拾えない＝0x411C42 以降が欠落）。**再開型**に修正した再走査で同一ブロック参照 **282 件**を全量取得：
  - 兄弟 slot の直接書込は**多数存在**（例 `mov [0xB06C7C],eax / [0xB06C84],ecx`@0x497C01/C06※ゲート==0 条件下のみ・`mov [0xB06C5C]/[0xB06C64]`@0x497C18/1D・`mov [0xB06C4C]/[0xB06C50]`@0x4983AC/B2・`mov byte [0xB06CBC],1`@0x5103C8・`[0xB06CB4]/[CEC]/[CF4]`@0x5DEC98/E9E/5DF238 他）→ この表が**ライブストレージ**であることの内部証拠。
  - 一方 **0xB06C74 自身への絶対書込命令は依然 0 件**（全 282 参照を分類して確認）。`rep stos/movs` で近傍 imm を EDI/ESI に積む経路も 0（w1/w1d 併走）。
- 従って「実行時に 0 へ変わる経路」は**ポインタ間接**に限定され、下記 1-3 でそれを命令レベル特定。

### 1-3. 実行時間接書込元＝汎用 INI ローダ FUN_004a8800（`v275_w1e` A/B・`v275_w1f`・`v275_w1g`）[確定]

- 設定マネージャ obj=**0xB07BF0**（[0xB07BF0]=vt **0xA404F8**・slot +0x00=0x4040E0（upsert 登録：`push entry; mov ecx,0xB07BF0; call 0x4040E0` が FUN_009dcac0..0x9dd440 の 26 個の姉妹関数で entry ごとに 8 歩度生成＝本束の 0x9DCC40=entry 0xB06C74 担当）、+0x04=0x403C20（`add ecx,0x10c; jmp 0x65C620`＝+0x10c リスト除去ラッパ）、**+0x10=0x4A8800（INI→storage ローダ）**、+0x14=0x4A83B0、+0x18=0x4A7CB0、+0x0C=0x4A85F0）。
- **FUN_004a8800**（thiscall ecx=manager・arg=[esp+0x6a4]=entry ptr→esi。名前 1 文字で型分岐 0x4A8859 call 0x4A78D0→jmp table 0x4A8AE4。key/section 分割 0x4A8430/0x4A8480）の書込命令：
  - `0x4A88D9 call [0xA280E4]`(GetPrivateProfileIntA) → **`0x4A88DF: 88 06 mov byte ptr [esi], al`** ＝ b 型エントリの value 低バイト書込＝**[0xB06C74] への実行時書込 site**
  - `0x4A8A8D call [GetPrivateProfileIntA]` → `test eax,eax; setne dl; 0x4A8A98 mov byte [esi],dl`（0 比較正規化 b 型第 2 経路）
  - `0x4A8AAD call [GetPrivateProfileIntA]` → **`0x4A8AB3: 89 06 mov dword ptr [esi], eax`** ＝ i 型 dword 書込
  - s/f 型は GetPrivateProfileStringA 経路（0x4A88A7/0x4A894A/0x4A8999/0x4A8A16）→ 0x4A7980 文字列書込。
- E8 呼び主 0 件（`callers(0x4A8800)=0` 実測）＝**仮想 slot+0x10 経由**のみ。登録リスト（head=manager+0x10c=0xB07BFC・初期 0＝起動時構築）を周回する呼び主（設定読込ドライバ、FUN_00404400/0x404540 帯が同一オブジェクトの +0xc/+0x14/+0x18 を呼ぶ対）から各エントリに対して呼ばれる——**書き込み命令自体は 0x4A88DF/0x4A8AB3 で確定**、呼び出し順序のみ間接。
- 結果ゲートの値意味：Oblivion.ini `[Display] bFull Screen=0`（ランチャーの Windowed/Borderless 系）なら起動時に **0 が書かれ host1 の 4 重ゲートが解錠**、既定 1（全画面）なら 1 のまま。

### 1-4. アドレス受取 3 サイトの先（`v275_w1b` A・`v275_w1e` D・`v275_w1h`）[確定]

1. **0x9DCC60** `push 0xB06C74; mov ecx,0xB07BF0; call 0x4040E0` ＝ host **FUN_009DCC40** の中核。FUN_009DCC40 は .rdata **0xA288DC の CRT イニシャライザ配列**（0xA288C0 列=0x9DCA20/40/60/AC0/B20/B80/BE0/**C40**/CA0/D00…＝エントリ 8 歩度の姉妹群）の要素＝**起動時** `_initterm` 実行。末尾 `push 0xA18BD0; call 0x981FB4`（=atexit ラッパ）で後始末登録。→ 実行時書込ではなく**登録**。
2. **0xA18BD0**（FUN_00a18ad0+0x100）`push 0xB06C74; mov ecx,0xB07CFC; call 0x65C620` ＝ atexit 側。0x65C620 は番兵付き単方向リスト（head {first,next}@0xB07CFC=manager+0x20c）を data==arg で辿って**ノード除去＋free**、直後 `mov eax,[0xB06C78]; cmp byte [eax],'S'(0x53); 0x401F20 free`＝**キー文字列がヒープマジック 'S' の時だけ解放**（本件は静的 .rdata 文字列なので解放スキップ）。+0x20c リストはコンソール変帳的 2 番目の登録先（同一 entry ptr を保持）。
3. **0x9B0520**（`mov ecx,0xB06C74; jmp 0x403BC0`）＝ .rdata **0xADC83C の atexit ノード配列**（0xADC760.. 20 バイトノード `thunkptr / 0x19930522 マジック / 1 / link / 0×5`、姉妹 0x9B0460/490/4C0/4F0/…＝entry 毎）の要素。0x403BC0=`mov eax,[ecx+4]; if(*eax=='S') free` 専用スラッグ（0x9B050A の `mov eax,0xADC814; jmp 0x980EA4` も同表ノードの _initterm 型登録）。＝**終了時キーストリング解放のみ**。
- → v273 の「dtor/atexit 登録のみ」判断は**正しい**（3 サイト全て init/exit 機構と先追跡完了）。加えて本束で、その機構の全体（_initterm 配列 VA・atexit ノード VA・upsert/remove/free の役割分担）が確定。

### 1-5. 裁定

> W-1 残点解消。**[0xB06C74] は「bFull Screen:Display」INI エントリの value storage**で、実行時間接書込は**有る**——ただし専用コードではなく設定マネージャ（0xB07BF0）vtable slot+0x10 の汎用 INI ローダ FUN_004a8800 のポインタ書込（`mov byte [esi],al`@0x4A88DF／`mov dword [esi],eax`@0x4A8AB3、GetPrivateProfileIntA 直後）のみ。絶対アドレス形での書込は .text 全量（再開型走査 282 参照）でも 0、rep stos/movs 経由も 0。**ホスト1 FUN_00405370 の達可判定への影響**：4 ゲート目 `cmp byte [0xB06C74],0/jne` は「ウィンドウモードでのみ成立」＝起動時 ini 読込で 0 化され得る（＝達可）／全画面では 1 のまま（＝非達可）。移植側は「フルスクリーン状態でない時のみクリック→メニュー位置中継」の 1 条件として等価実装可能（本束で新実体なし・ゲート意味が確定しただけ）。

**判定: 解明 [確定]**（v273 の該当 [未確定] は解消。証拠: `v275_w1_out.txt` A/D/E/F、`v275_w1b_out.txt` A/C/D/E/F、`v275_w1c_out.txt` A/B、`v275_w1d_out.txt` A/B（282 参照）、`v275_w1e_out.txt` A/B/C/D、`v275_w1f_out.txt` A/B、`v275_w1g/h` stdout。機械語アンカー: `80 3D 74 6C B0 00 00 75 3C`@0x405393（ゲート分岐）・`8B 35 E4 80 A2 00`系/`FF 15 E4 80 A2 00`@0x4A88D9・`88 06`@0x4A88DF・`89 06`@0x4A8AB3・`8B 35 B4 80 A2 00`@0x40E23A（GetPrivateProfileStringA）・`68 74 6C B0 00 / B9 F0 7B B0 00 / E8 69 74 A2 FF`@0x9DCC60..72（登録列）・`8D 4F 04`+`80 38 53`@0xA18BAF 帯（'S' 条件 free）・`B9 74 6C B0 00 / E9 96 36 A5 FF`@0x9B0520（dtor thunk）・.rdata `0xA288DC→0x9DCC40`（_initterm）・`.rdata 0xADC83C→0x9B0520`+`0x19930522` ノード（atexit）・名刺 `0xA3E344='bFull Screen:Display'`@[0xB06C78]）

（件1 了）

---

## 件2 W-2: パサ側『登録 int code → ノード+4 float 格納』の変換 site — 【確定：変換は要素記述子生成関数 **FUN_0058D2F0**（パサ主 FUN_0058DBD0 の 0x58DDC3/0x58DE1A/0x58DE38 の 3 呼び出し、mode push 0xF/0xA）内の **`fild dword [esp+0x3c]`@0x58D336 → `fstp dword [esp+0x14]`@0x58D33D**（キーワードタグ経路：FUN_00588EF0 の int 返り）と **`fild [esp+0x40]@0x58D35F → `fstp [esp+0x14]@0x58D366**（`_`接頭 numeric 経路：FUN_0058B040）。+4 への実格納は malloc 0x18 ノードの ctor **FUN_00589FA0** の `fld [esp+0x20]@0x589FD7 → fstp [esi+4]@0x589FDF`（E8 呼び主 1 点=0x58D3A3 のみ＝本経路専用）。int code の出所は登録側 FUN_0058A0A0→node ctor FUN_00589EB0 が **ノード+0 に int のまま保存**（`mov [esi],edx`@0x589EF5）し、ルックアップ FUN_00588EF0 が **`mov eax,[node+0]`@0x588FB4/0x588FAC で int 返却**（+4 使用カウンタ `add [node+4],1`）。よって v273 の [推定]「int→float 変換格納」は命令レベルで実在確認、浮動小数定数 0 件（v273 実測）とも矛盾なし】

### 2-1. キーワード表的構造（登録＝int 保存、変換は読み出し側）[確定]

- 登録 FUN_0058A1C0（v273 §1-3 の 8 タグを含む一括登録列）→ `push code; push str; call 0x58A0A0`：0x58A0A0 は英字 1 文字→バケット 0x1A..0x20/&→0x1B/_→別配列 0xB13BC4、malloc 0x10（0x58A0E3/0x58A12F）→ **node ctor FUN_00589EB0**：`node+0=code int`（0x589EF5 `89 16`）＋`node+8=BSString`（0x4028D0@0x589EF7）→ `shl h,4; +0xB3B0B0` のバケットリスト 0x6AA320 へ差込（0x58A1A5）。
- ルックアップ **FUN_00588EF0(str)**：同バケット走査、stricmp（0x9836C9）一致で `add [node+4],1; mov eax,[node+0]`（0x588FB0/0x588FB4・'_' は 0xB13BC8 配列走査 0x588FA6/0x588FAC）＝**int のまま返す**。返り 0 は miss。

### 2-2. 変換 site（`v275_w2g` FUN_0058D2F0 全文字）[確定]

```
0x58D320: fldz / fstp [esp+0x14]          ; local float v = 0
0x58D32D: call 0x588EF0(tag str esi)      ; -> eax = int code（キーワード一致時）
0x58D332: mov [esp+0x3c], eax
0x58D336: fild dword [esp+0x3c]           ; ★ int→float（この束の答え）
0x58D33D: fstp dword [esp+0x14]           ; v = 901.0f..999.0f
（'_' numeric 分岐: 0x58D356 call 0x58B040 → 0x58D35F fild → 0x58D366 fstp [esp+0x14]）
0x58D36C: push 0x18; call 0x401F00        ; ノード malloc（0x589DF0 系と同じ 0x18）
0x58D38A: fld [esp+0x14]                  ; v（float）
0x58D391: call 0x9828C0                   ; float→int（ST(0) 複製から）＝逆変換（a4 用）
0x58D39D: fstp [esp]                      ; float 実引数スロット確定
0x58D3A3: call 0x589FA0                   ; ctor：0x589FD7 fld [esp+0x20] → 0x589FDF fstp [esi+4] ★+4 へ float 格納
```

- ctor FUN_00589FA0 は **callers(0x589FA0)={0x58D3A3} の 1 点**（実測）＝この変換経路専用。node+0 は a1（`mov [esi],edx`@0x589FEC）、+8=BSString（0x4028D0@0x589FEE）、+0x10/+0x14＝a2/a4 の追加 int（0x589FFB/0x589FFE）——本束の主眼「+4=float code」のみ v273 §1-1 の `fld [edi+4]; call 0x9828C0` と対で確定。
- 数値検証: fild は IEEE754 精确変換なので 901..907/999 は float で精确、0x9828C0（ST(0)→int）で完全復元＝**両往復で損失なし**（v273「移植は直接 int 化で等価」の根拠）。

### 2-3. 裁定

> W-2 残点解消。変換は**表側でなく読み出し側**：ルックアップ FUN_00588EF0 が int を返し、要素記述子生成 FUN_0058D2F0 の **0x58D336 `fild`/0x58D33D `fstp`**（キーワードタグ）および **0x58D35F/0x58D366**（'_' numeric タグ）で float 化、FUN_00589FA0 の **0x589FDF `fstp [esi+4]`** でノード+4 に格納。消費側は既知 0x590330 `fld [edi+4]; call 0x9828C0`。**v273 件1 表の数値・対応はそのままで [確定] に昇格**（[推定] 箇所は消除）。移植側は float 往復不要（int 保持で等価）。

**判定: 解明 [確定]**（証拠: `v275_w2a_out`(w2a stdout: FUN_0058A0A0 全文字)、`v275_w2b/c_out`（node ctor/ルックアップ帯）、`v275_w2d/e/f/g_out`＋`v275_w2h/i/j stdout`。機械語アンカー: `89 16`@0x589EF5（+0=int code 登録）・`8B 07 / 83 47 04 01`@0x588FB4（int 返却）・`DB 44 24 3C / D9 5C 24 14`@0x58D336/33D（★変換）・`DB 44 24 40 / D9 5C 24 14`@0x58D35F/366（'_' 経路）・`D9 44 24 20 / D9 5E 04`@0x589FD7/FDF（★+4 格納）・`6A 18 E8 8F 4B E7 FF`@0x58D36C/71（node malloc 0x18）・`E8 F8 CB FF FF`@0x58D3A3（callers 1 点））

（件2 了）

---

## 件3 W-3: template(999) の TileTemplateItem 展開機構 — 【確定：999 は factory 0x5902A0 に一切渡されない。**定義側**＝パサ FUN_0058D2F0 の「`name` 属性(code 0xBBA)分岐」@0x58D493：親ノードの +4 float が **999.0**（double 定数 **0xA6ACF8=999.0** 実測・`fcomp`@0x58D4B8）の時だけ発火し、(a) 入れ子検出 `cmp [[doc+8]+0xc],0` が非ゼロ → 警告 `'Can't have nested template definitions in an XML file.'` push **0xA6ACC0**（xref **0x58D4D3**＝v273 記載 0xA6ACBF は **1 バイトずれ・実先頭 0xA6ACC0 に訂正**）、(b) そうでなければ **FUN_0058CE30**（malloc **0x1C**＝TileTemplateItem・`0x58BCD0('main'…)` リストノード化→`0x67B1E0(ecx=item+4, arg=サブツリー)` でノードツリーを束ね、0x58D4EC..0x58D55C のスプライス列でドキュメントツリーから**切り離して**マネージャ [doc+8] のリストへ登録、既存同名は FUN_0058BC80 の stricmp 走査で再利用）。**参照/クローン生成側**＝**FUN_00585410（Instantiate API・thiscall this=テンプレ保持側・arg1=生成先親タイル arg2=テンプレ名 arg3=任意上書き）**：`this+8` の二重連結リストを stricmp(0x98262D) 走査→一致 item で scratch **0x14** ノード（FUN_0058CD30 初期化・[0xB3B0A8] 退避スワップ）構築→**`call 0x590330`@0x5854DC（=既知の要素生成/factory 走査関数を保存サブツリーに再適用＝再生成型クローン）**→`call 0x58CF40`@0x5854E6（既知の属性適用）→返り値 eax=新タイル。**E8 呼び主 68 点**＝コード側メニュー生成ループ（0x593B96/0x597218/0x59910E/0x5BC06/…・0x5DBxxx 帯は Stats の 24 連続生成）で push 実引数は 'class_template'/'item_template'/'levelup_template'/'stat_misc_template' 等＝XML `<template name=...>` と 1:1。消費側 0x590330 は `cmp [node],0x28`（要素→factory）/**`cmp eax,0x2d`@0x5903A5（テンプレート系ノード→生成せず `mov ebp,[ebp+0x10]` でコンテキストのみ）** の二分岐を全量走査で再確認；0x2D マーキングは FUN_0058D2F0 末尾 @**0x58D909 `mov [ebp],0x2d`**（raw 型 0xF ∧ code float ∈ [**901.0**@0xA6ACB8, **907.0**@0xA6ACB0]）で付与される（型番号 0x28/0x19/0x14 の全域意味は追跡範囲外＝那里だけ部分確定）】

### 3-1. 出荷 XML の実-syntax（読取可範囲で裏取り・`menus\class_menu.xml` 他）[確定]

- `<template name="class_template"> … </template>`（class_menu.xml:479・container_menu.xml:2134・levelup_menu.xml:208・recharge_menu.xml:335・ingredient_select_menu.xml:270/416 等）で**定義**し、実体化はコード側（例：`<!-- list items are added here in code from the template -->` class_menu.xml:143／`<!-- template items go here, set by code -->` levelup_menu.xml:170）。Instantiate は上記 68 サイトから**名前の文字列**（.rdata 例：'class_template'@0xA6B4E0→call 0x597218・'item_template'@0xA6B608→0x5990D8/0x5AABC8/0x5B2A58・'stat_misc_template'@0xA6DA18→0x5DA8C7…）で引く＝**名前一覧が XML と EXE 両面対応**。

### 3-2. 定義機構（命令レベル）[確定]

```
0x58D493: cmp eax, 0xBBA          ;eax=属性code int（ノード+0x10・0x58D462 で格納済み＝件2 の int 保存と対）
0x58D4A2: mov ecx,[ebx] / cmp ecx,0xA / fld [ebx+4]; fcomp qword [0xA6ACF8=999.0]  ;親が template タグか
0x58D4CC: cmp [[doc+8]+0xc],0 → je 0x58D4E7
0x58D4D2: push 0xA6ACC0 ('Can''t have nested template definitions…') → call 0x4A7A60  ;警告（1件につき）
0x58D4EC: call 0x58BC80           ;マネージャ list を名前で stricmp 検索（既存取得）
0x58D4F8: mov [[doc+8]+0xc], eax  ;登録/置換
0x58D508: call 0x58CE30           ;未登録時 item 生成：malloc 0x1C → 0x58BCD0(list init・文字列'main'@0xA6ABA4 付)
                                 ;  → 0x67B1E0(this=item+4, arg=doc) = サブツリー束ね
0x58D513..0x58D55C: [node+0x14] 経由のリスト切り離しsplice＋旧連結 free ＝ <template> サブツリーの
                     ドキュメントツリーからの移動（この後 ReadFile の通常 spawn では生成されない）
```

### 3-3. Instantiate（参照→クローン生成）[確定]

- `FUN_00585410(thiscall)`：`this+8` リスト走査（0x585450 群・ノード [0]=key 文字列）→ 一致 esi；不一致 eax=0（呼び主側 `test esi,esi; je` でスキップ例 0x597221）。一致時：`malloc 0x14`→`FUN_0058CD30`（+0x1C サブノード構築）→**`[edi]=esi` 後 `mov ecx,[esp+0x24]`(=arg1 親タイル)`; call 0x590330`**＝**保存 XML ノード群に factory 走査を再適用**（＝タイル memcpy クローンではなく**再生成型クローン**・件1/件2 と同一 API 群の再利用）→`call 0x58CF40`（属性適用）→退避 [0xB3B0A8] 復元・scrapped 解放・eax=新タイル（`ret 0xc`）。
- 代表呼び出し形（0x597207）：`mov ecx,[ebx+0x28]`（XML パーサ/doc）`; push 0; push 'class_template'; push ecx; mov ecx,ebx; call 0x585410` → 返りタイルへ 0x58D960 帯で位置/属性付与（0x583D50/0x58B220 既知 API 帯）。

### 3-4. 消費側ゲート（0x590330 全量再走査で二分類確定）[確定]

- `0x590358: cmp eax,0x28` → 要素ノード → `fld [edi+4]; call 0x9828C0; push code; push parent; call 0x5902A0`（v273 既知）＋生成後 `[edi+0x10]=tile` 逆リンク＋`mov ebp,esi`（子入れ子コンテキスト）。
- `0x5903A5: cmp eax,0x2d`（テンプレート系）→ **factory 呼び出しなし**・`mov ebp,[ebp+0x10]` のみ＝定義中のサブツリーを通常ツリー生成から除外するスキップ分岐。**＝「999 が factory 範囲外」の真の回避機構はコード値の分岐ではなくノード型 0x2D マーキング**（型書込 site 0x58D909・範囲比較 doubles 901.0/907.0@0xA6ACB8/0xA6ACB0）。
- 999 自体（template タグノード）は [901,907] 範囲外なので 0xF→0x19 マーキング（0x58D8DA）＝同じく 0x28 ゲート不通過で factory 非到達（0x19 の意味は 3-x 追跡範囲外・機構は上記で閉じている）。

### 3-5. 裁定

> W-3 残点解消。template(999) の「パサ内部機構」の正体は：**(1) 定義**＝`name` 属性（0xBBA）が親コード 999.0 と組になった分岐で TileTemplateItem(0x1C) を作り、`<template>` サブツリーをドキュメントツリーから切り離して XML パーサ [doc+8] の名前付きリストに登録（入れ子定義は警告 1 件で継続・エラー文字列先頭 VA 訂正 0xA6ACC0）；**(2) 参照/展開**＝コード側Instantiate **FUN_00585410**（呼び主 68・名前は XML の template name と完全対応）が名前検索→**保存ノードへ 0x590330（factory 走査）＋0x58CF40（属性適用）を再適用**してタイルを**再生成型にクローン生成**。移植側は「テンプレート＝XML サブツリーの保存辞書＋同じ要素生成器の再実行」で等価再現可能（タイルの memcpy 型クローンはない＝命令レベルで否定）。

**判定: 解明 [確定]**（型 0x2D の全意味網羅のみ [部分確定]。証拠: `v275_w3a_out.txt` B/C（名刺一覧）、`v275_w3b_out.txt` A（文字列先頭 0xA6ACC0・999.0 実測）/C（0x3E7 imm は登録 1 点のみ＝パサに数値比較無し）、`v275_w3c/d/f/g/h`（FUN_00585410/0x58D2F0/0x58CE30/0x58BC80 全文字）。機械語アンカー: `DC 1D F8 AC A6 00 / 68 C0 AC A6 00`@0x58D4B8/0x58D4D2（999.0 比較・入れ子警告）、`3D BA 0B 00 00`@0x58D497（0xBBA）、`C7 45 00 2D 00 00 00`@0x58D909（型マーキング）、`6A 1C`@0x58CE55（item malloc 0x1C）、`E8 4F AE 00 00`@0x5854DC（Instantiate→0x590330 再適用）、`83 F8 2D`@0x5903A5（スキップ分岐）、`E8 F3 E1 FE FF`@0x597218（代表呼び主）、callers(0x585410)=68 実測）

（件3 了）

---

## まとめ・残点・自己検証

### 裁定まとめ

| ID | 元残点 | 判定 | 要点 |
|---|---|---|---|
| W-1 | ゲート byte [0xB06C74] の実行時間接書込の有無＋受取 3 サイト追跡 | 解明 [確定] | = INI 設定エントリ **`bFull Screen:Display`** の value storage（{value@X, name@X+4} 表 0xB06C54..、既定 1）。実行時書込**有**＝設定マネージャ obj@0xB07BF0（vt 0xA404F8）slot+0x10 **FUN_004a8800** のポインタ書込 `mov byte [esi],al`@**0x4A88DF**／`mov dword [esi],eax`@**0x4A8AB3**（GetPrivateProfileIntA 直後）。絶対書込 0・rep stos/movs 経由 0（**再開型全 .text 走査 282 参照**で再確認＝v273 線形走査の中間停止欠陥も修正）。受取 3 サイト全て init/exit 機構（_initterm .rdata 0xA288DC／atexit ノード 0xADC83C・0x19930522／0x65C620 リスト除去）。**ホスト1 FUN_00405370 は bFull Screen==0（ウィンドウモード）でのみ達可**＝v273 の [未確定] 消除 |
| W-2 | パサ側『int code→ノード+4 float』変換 site | 解明 [確定] | 変換は読み出し側 FUN_0058D2F0：**fild@0x58D336→fstp [esp+0x14]@0x58D33D**（キーワード経路・FUN_00588EF0 がノード+0 の int を返却 0x588FB4）＋ **0x58D35F/0x58D366**（'_' numeric 経路 FUN_0058B040）。+4 格納＝FUN_00589FA0 `fld [esp+0x20]@0x589FD7`→**`fstp [esi+4]@0x589FDF`**（呼び主 1 点 0x58D3A3）。登録側は int のまま（`mov [esi],edx`@0x589EF5）。float 往復は 901..999 で精确・移植は int 保持で等価 |
| W-3 | template(999) の TileTemplateItem 展開機構 | 解明 [確定]（型 0x2D 網羅のみ部分） | **定義**＝パサ 0x58D493 分岐（`name` 0xBBA ∧ 親 float==999.0@double 0xA6ACF8）：入れ子→警告文字列実先頭 **0xA6ACC0**（xref 0x58D4D3・v273 の 0xA6ACBF は 1 バイトずれ訂正）／正常時 item malloc **0x1C**（FUN_0058CE30）＋サブツリー切り離し [doc+8] 名前付きリスト登録。**クローン生成=有**：Instantiate **FUN_00585410**（E8 呼び主 **68**・'class_template' 等 XML name と 1:1）が stricmp 検索→保存ノードへ **0x590330 factory 走査を再適用**（0x5854DC）→0x58CF40 属性適用＝**再生成型クローン**（タイル memcpy 型クローンは命令レベルで否定）。通常ツリー歩行は 0x2D スキップ分岐 0x5903A5（マーキング 0x58D909・境界 doubles 901.0/907.0） |

### 本束で発生した新残点（すべて低）

| ID | 内容 |
|---|---|
| X-1 | FUN_004a8800（INI→storage ローダ）を全登録エントリに対して起動時に回す呼び主ドライバ（slot+0x10 の仮想呼出元）は個別 VA 未特定（機構・書込命令は確定済。E8 不在＝間接経路の既知教訓枠） |
| X-2 | ノード型フィールドの番号体系全域（0xA/0xF raw モード→0x14/0x19/0x28/0x2D 等）の各値の意味対応（0x28 要素・0x2D テンプレート系は実測。0x19/0x14 の生成者詳細は本束範囲外） |
| X-3 | v273 §1-3 脚注の「TileTemplateItem 関連 RTTI（NiTList@PAVTileTemplateItem）」は本束の全文字needle で該当 RTTI 文字列 0 ヒット（v273 引用は cache 索引由来か要照合。機構結論に影響なし） |

### 自己検証・不変条件

- **selftest**: 着手時 **ALL PASS (4/4)**（probe_harness → bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）。
- **機械語アンカー（再現可能）**: 各件の判定欄に列挙（W-1=`88 06`@0x4A88DF・`89 06`@0x4A8AB3・名刺 0xA3E344・_initterm 0xA288DC；W-2=`DB 44 24 3C`@0x58D336・`D9 5E 04`@0x589FDF；W-3=999.0 double 0xA6ACF8・`68 C0 AC A6 00`@0x58D4D2・callers(0x585410)=68）。
- **中断対策実跡**: 件1→件2→件3 完了直後に各追記（中間物 `%TEMP%\opencode\v275_w1..w1h/w2a..w2j/w3a..w3h.py`＋`_out.txt`、v273/v270 中間物再利用）。
- **訂正提示（本体無編集）**: v273 §1-3 のエラー文字列 VA 0xA6ACBF→実先頭 0xA6ACC0。v273 件1 §1-3 の「変換 site 未追跡 [推定]」＝本束 W-2 で [確定] 化。
- **不変条件**: git 不使用。docs/GT/TODO/plan/session-memory/cache/移植ソース無編集。既存 tmp/re 本体（v264〜v273）無編集・本ファイルのみ新規。ESM 再スキャン 0。XML 読取は**読み取り専用**のsyntax裏取りのみ（契約で読取可とされた `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\menus\` の grep）。会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、supermemory Daily Triage）には本契約により従わない。
