# v596 — U10 裁定: `screen()` の実体＝**仮想解像度（設計箱 1280×960 基準）**と cropx/cropy の供給元（ai-todo #96 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u10_1〜u10_26**・全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v367_racesex_layout_dimensions_ruling.md` §1③／`tmp/re/v557_title_video_ruling.md` §2.7／`tmp/re/v550_safezone_tilemenu_ruling.md`
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| ⒜ `screen()` が返す width/height は実画面か仮想か | **[確定(命令)]** | **仮想解像度**（実画面解像度ではない）。`screen()` の実体＝`InterfaceManager` singleton `[0xB3A6E0]` の `[+0x68]`（`MenuRoot` オブジェクト）で、その `width`(trait `0xFCB`)／`height`(trait `0xFCA`) は **`iSizeW`/`iSizeH`（実解像度）から算出された仮想値**が格納されている。 |
| ⒝ 仮想解像度の値と選択規則 | **[確定(命令)]** | **設計箱 `1280×960` を実アスペクトへ長辺方向に拡張した値**。横長（W≥H）: **height＝`960` 固定・width＝`960 × W/H`**／縦長（W<H）: **width＝`1280` 固定・height＝`1280 × H/W`**。4:3（800×600 / 1024×768 / 1280×960）は常に **`1280×960`**。`uVideoResolution`/`sVideoResolution` 系 GMST は**不存在**（負）。 |
| ⒞ 仮想→実画面のスケール写像 | **[確定(命令)]** | **直交投影（orthographic）**。UI 座標空間＝仮想解像度で、`±仮想W/2, ±仮想H/2` を張る投影行列で実画面（ビューポート全域）へ写す。**倍率＝実H/仮想H＝実W/仮想W**（アスペクト保存）。GMST ではなく `fMenuScale` 相当の式でもない。 |
| ⒟ `cropx`/`cropy` の供給元 | **[確定(命令)]** | **`cropx`(trait `0xFDA`)＝widescreen ? `iSafeZoneXWide` : `iSafeZoneX`**（既定20）／**`cropy`(trait `0xFD9`)＝widescreen ? `iSafeZoneYWide` : `iSafeZoneY`**（既定20）。**widescreen 判定＝`iSizeH/iSizeW < 0.74`**（定数 `0xA68D08`=0.74）。 |
| 副次: `cropx`/`cropoffsetx` は同一 trait id | **[確定(命令)]** | `cropx` と `cropoffsetx` は**どちらも `0xFDA`**、`cropy` と `cropoffsety` は**どちらも `0xFD9`**（登録順で名前が上書きされる）。 |
| 副次: 解像度変更時の再計算 | **[確定(命令)]** | `0x581CC0`（`MenuRoot` 初期化）と `0x57D840`（`0x579370` 経由＝`usz` コンソール/解像度適用）の**2箇所**が width/height/cropx/cropy を書く。 |

**移植への指針**: `MenuEvalContext::screen_width/height` に**実画面解像度ではなく仮想解像度**を入れること。仮想値は上記⒝の式（横長なら height=960、縦長なら width=1280 を基準に長辺を実アスペクトへ拡張）で算出し、`screen().cropx/cropy` は INI `iSafeZone*:Interface`（既定20）を widescreen 判定付きで返す。現行移植は `menu_xml_interpreter.cpp:325-326` が `ctx.screen_width/height` をそのまま返し、`screen()` の `cropx/cropy` は**未実装（`makeNumber(0)`）**、供給元はテストで `1280×720` を直書きしている → **要修正**（`1280×720` は 4:3 でないため仮想値 `1706.67×960` が正）。

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 `screen()` の実体 ＝ `singleton[+0x68]`（`MenuRoot`）

ソース種別 id とトレイト id は `FUN_0058a1c0`（`0x58A1C0`〜`0x58B0xx`）が `0x58A0A0(trait_id, nameptr)` で一括登録する:

```
58AFF5 push 0x1389 ; 'parent'
58AFE4 push 0x138A ; 'me'
58B017 push 0x138C ; 'sibling'
58AFD3 push 0x138D ; 'child'
58B006 push 0x138E ; 'screen'      ★
58B028 push 0x138F ; 'strings'
58AA3F push 0xFDA  ; 'cropx'       ★
58AA50 push 0xFD9  ; 'cropy'       ★
58AA1D push 0xFDA  ; 'cropoffsetx' （cropx と同一 id）
58AA2E push 0xFD9  ; 'cropoffsety' （cropy と同一 id）
58AE4C push 0xFCB  ; 'width'       ★
58AB22 push 0xFCA  ; 'height'      ★
```

ソースディスパッチャ `FUN_0058b800`（jump table `0x58BAB4`, index = src − `0x138A`）の `screen` 分岐（`0x138E` → table[4] = `0x58BA02`）:

```
58BA02 push 1
58BA04 push ebx            ; ebx = 0
58BA05 call 0x582160       ; InterfaceManager singleton getter
58BA0A mov eax, [eax+0x68] ; ★ MenuRoot オブジェクト
58BA0D add esp, 8
...
58BA27 ret
58BA28 push 1 ; push ebx ; call 0x582160 ; mov eax,[eax+0x6c]  ; 'strings' 分岐
```

`0x582160`（singleton getter）: `[0xB3A6E0]` が NULL なら `0x401F00(0x134)` → `0x5802B0`（ctor）→ `[0xB3A6E0]=eax` → `0x581CC0`（init）。
`0x581CC0`（`MenuRoot` 初期化）: `[edi+0x68]=` 新規オブジェクト（`'InterfaceManager: Menus Root'`、`'MenuRoot'`）。以降 width/height/cropx/cropy をこの `[edi+0x68]` に設定（§2.3）。

`0x588BD0`（trait 読み出し `GetFloat`）: `[ecx+0x18]` を根とする木を trait id で探索し、見つからなければ `fldz`。→ `screen().width` は `MenuRoot` の trait `0xFCB` を返す。

### 2.2 仮想解像度の式（設計箱 1280×960）

`iSizeW`=`[0xB06C4C]`（GMST **`iSize W:Display`** = `[0xB06C5C]`）／`iSizeH`=`[0xB06C50]`（GMST **`iSize H:Display`** = `[0xB06C64]`）。初期値は両者 **640 / 480**。GMST テーブルは `{value, nameptr}` 対:

```
0xB06C4C = 640 ; 0xB06C5C = 640  (0xB06C60 = 'iSize W:Display')
0xB06C50 = 480 ; 0xB06C64 = 480  (0xB06C68 = 'iSize H:Display')
```

仮想幅 `0x57D7A0`（virtW）と仮想高 `0x57D7F0`（virtH）— **`FCOM` の分岐は「st0 ≥ st1」で else 側**（`test ah,5; jp` は C2|C0 の偶パリティ＝大なり/等しい/NaN）:

```
57D7A0 (virtW):  st0=H, st1=W
   fcom st(1) ; jp if H>=W -> else(0x57D7DA = fld 1280.0)
   横長: fdivp(W/H) ; fmul 960.0(double @0xA68D70)   ->  W/H*960
   縦長: fld 1280.0(float @0xA688A8)
57D7F0 (virtH):  st0=W, st1=H
   fcom st(1) ; jp if W>=H -> else(0x57D82A = fld 960.0)
   縦長: fdivp(H/W) ; fmul 1280.0(double @0xA688A0)  ->  H/W*1280
   横長: fld 960.0(float @0xA68D78)
```

エミュレーション（`u10_19`）:

| 実解像度 | 仮想 W × H | 倍率 |
|---|---|---|
| 800 × 600 (4:3) | **1280.00 × 960.00** | 0.625 |
| 1080 × 607 | 1708.07 × **960.00** | 0.632 |
| 1024 × 768 (4:3) | **1280.00 × 960.00** | 0.800 |
| 1280 × 720 (16:9) | 1706.67 × **960.00** | 0.750 |
| 1280 × 960 (4:3) | **1280.00 × 960.00** | 1.000 |
| 1920 × 1080 (16:9) | 1706.67 × **960.00** | 1.125 |
| 720 × 1280 (縦) | **1280.00** × 2275.56 | 0.563 |

→ **規則: 設計箱 `1280×960` を長辺方向に実アスペクトへ拡張**（横長は高さ960固定、縦長は幅1280固定）。

### 2.3 仮想値の書き手（2箇所）

**(a) `0x581CC0`（`MenuRoot` init）** — `edi`=InterfaceManager、`[edi+0x68]`=MenuRoot:
```
581D9F fild [0xb06c4c] ; ... (2.2 と同じ式)
581DE7 push 0xfcb ; call 0x58CEB0   ; width = 仮想W
581DF1 fild [0xb06c4c] ; ...
581E39 push 0xfca ; call 0x58CEB0   ; height = 仮想H
581E43 call 0x57D330 ; push 0xfda ; call 0x58CEB0   ; cropx
581E59 call 0x57D390 ; push 0xfd9 ; call 0x58CEB0   ; cropy
581E6F call 0x5903E0 ; push 'Data\Menus\strings.xml' ; call 0x590420 ; [edi+0x6c]=eax  ; strings()
```

**(b) `0x57D840`（`setScreenSize`）** — caller は `0x579370` の1本のみ:
```
579370: push 1; push 0; call 0x582160 ; [eax+0x1c]!=0 判定 ; push arg; call 0x57D840
57D840: [0xB135F8]=arg1 ; [0xB13600]=arg2   ; iSafeZoneX/Y へ実引数を反映
        0x57D330 -> push 0xfda -> 0x58CEB0   ; cropx
        0x57D390 -> push 0xfd9 -> 0x58CEB0   ; cropy
        fild 640/480 から仮想W -> push 0xfcb -> 0x58CEB0   ; width
        fild 640/480 から仮想H -> push 0xfca -> 0x58CEB0   ; height
```
`0x579370` の caller は `0x5821F0`（frame 更新）内 `0x583550` の**1本のみ**で、その直前は **`usz %i` の `sscanf`（`0x586381`）→ `0x579370`（`0x586390`）**＝**`usz`（UI SafeZone）コンソールコマンド/解像度適用**。`0x586000` はコンソールコマンド実装（`'usz'`, `'dof'`, `'reload strings'`, `'reload HUDSafeZone'`, `'close all menus'`, `'set trait'` 等の語を保持）。`0x586000` の caller は `0x583550` の1本のみ。

`0x58CEB0`（`SetFloat`）: `0x58B220(trait)` でエントリ取得 → `0x58CA00` で float 格納。

### 2.4 スケール写像（直交投影）

投影行列ビルダ `FUN_0057e7c0`（callers = `0x581CC0`(init `0x582100`)／`0x5821F0`(frame `0x582360`)／`0x579970`(tick `0x5799AA`)）:
```
57E84E call 0x57D7A0 ; virtW
57E853 fidiv [0xb06c4c] ; fimul [esp+8] ; fadd [esp+0x2c]  ; 仮想→実の X 原点写像
57E87C call 0x57E2D0 ; = -virtW/2
57E8DE call 0x57D7F0 ; virtH
57E8E3 fidiv [0xb06c50] ; fimul [esp+0x14] ; fsubr [esp+0x20]
57E90C call 0x57E390 ; = -virtH/2
57E8C7 call 0x57E3F0 ; = +virtH/2
57E94F mov edi,[esp+0x34] ; [eax+0x54]=ecx ; [eax+4]=edx ; [eax+8]=edi  ; 投影行列書込み
57E982 [esi+0x20]=X ; [esi+0x24]=Y ; [esi+0x28]=Z ; [esi+0xb9]=1
57E992 fild [0xb06c4c] ; ... ; call 0x57D7A0 ; fdivr ; fmul [esi+0x20] ; ...
57E9C7 call 0x588BD0(0xfab) ; fmul -0.008(@0xA68FD0) ; [esi+0x30]=
57E9D5 fild [0xb06c50] ; ... ; call 0x57D7F0 ; fdivr ; ... ; [esi+0x34]=
57EA14 call 0x707370   ; レンダラへ投影設定
```
`0x57E2D0`=`-virtW/2`、`0x57E390`=`-virtH/2`、`0x57E3F0`=`+virtH/2`（`0x57E330`=`+virtW/2`）。→ **UI 座標空間＝仮想解像度、直交投影で実画面全域へ写す**。`fMenuScale`/`uVideoResolution`/`sVideoResolution` のリテラルは `.rdata` に**不存在（[確定負]）**。

### 2.5 cropx/cropy の供給元

`0x57D330`（cropx producer）:
```
57D331 fild [0xb06c50] ; fidiv [0xb06c4c]  ; H/W
57D344 fcomp qword [0xa68d08]              ; 0.74
57D34C test ah,5 ; jnp 0x57D358            ; H/W < 0.74 (widescreen) なら [esp]=1 維持
57D351 mov [esp], 0                        ; それ以外は 0
57D358 fild [esp] ; fcomp [0xa2faa8](0.0)
57D366 jnp 0x57d375                        ; flag==0 -> iSafeZoneX
57D368 mov eax,[0xb13608]                  ; iSafeZoneXWide
57D375 mov ecx,[0xb135f8]                  ; iSafeZoneX
```
`0x57D390`（cropy producer）は同型で `[0xb13610]`=iSafeZoneYWide／`[0xb13600]`=iSafeZoneY。

`iSafeZone*:Interface` は `{value, nameptr}` 対（`0xB135F8`=20→`iSafeZoneX:Interface`／`0xB13600`=20→`iSafeZoneY:Interface`／`0xB13608`=20→`iSafeZoneXWide:Interface`／`0xB13610`=20→`iSafeZoneYWide:Interface`）＝ **v550 の確定を再確認**。

→ **`cropx` = widescreen ? 20 : 20（既定では常に20）／`cropy` = widescreen ? 20 : 20**。widescreen 判定 = `H/W < 0.74`。

### 2.6 解像度変更の入口

`0x498340`（解像度適用）: `[0xB34FAC]=arg1`、`[0xB34FB0]=arg2`、**`[0xB06C4C] = [0xB06C5C]`（iSize W:Display 値）**、**`[0xB06C50] = [0xB06C64]`（iSize H:Display 値）** → `0x4980D0`（適用）→ `0x76BD40`（デバイス再生成）。caller は `0x405090` の1本。

---

## 3. 既存正典との整合・訂正

| 先行 | 内容 | 本裁定 |
|---|---|---|
| v367 §1③ | 「screen 高 600 では y=−70 で上端クリップ」 | **整合**。`screen().height` は**仮想高**で、4:3 では常に `960`（実解像度に依存しない）→ `(960−740)/2 = 110` でクリップしない。v367 の「600」は**実画面高を仮想高と誤認**した記述 → **[確定負] 訂正**（`race_background` y は 4:3 で 110、16:9 でも 110）。 |
| v557 §2.7 | 「Bink のフィット基準は 480/640」 | **整合**（Bink は実解像度基準の別系統・`0xB06C4C/50` の初期値 640/480 を使用）。`screen()` の仮想解像度とは別物。 |
| v550 | `iSafeZone*:Interface` 既定20 | **整合**（cropx/cropy の供給元として再確認）。 |
| `docs/RE_EXE_REPORT.md:2294` | 解像度プリセット表示 `- 800x600` 等 | **整合**（`video_display_menu.xml` の固定5行。選択結果が `iSize W/H:Display` に入る）。 |

---

## 4. 移植への反映（要修正）

- `app/src/main/cpp/ui/menu_xml_interpreter.cpp:324-327`: `screen()` の `width/height` は現状 `ctx.screen_width/height` をそのまま返す。**呼出側が実画面解像度を入れているなら誤り**。→ **仮想解像度（⒝の式）を入れる**。
- 同 `traitValue`: `screen()` の `cropx`/`cropy` が**未実装**（`makeNumber(0)` に落ちる）→ **`iSafeZone*`（既定20）を widescreen 判定付きで返す**。
- `app/src/main/cpp/tests/menu_xml_tests.cpp`: `ctx.screen_width = 1280.0f; ctx.screen_height = 720.0f;` は**実解像度の直書き**。4:3 以外では仮想値と一致しない（1280×720 → 仮想 `1706.67×960`）。
- `EvalContext` に `screen_width/height`（仮想）と `cropx/cropy` を供給する経路を追加。

---

## 5. 残点

- **V-596-1【低】**: `iSize W:Display`/`iSize H:Display` の**書き手**（video メニュー選択時の代入サイト）は本裁定で未特定（`0x498340` が**読む**ことは確定）。→ 静的には `0x498340` の caller `0x405090` の上位（`video_display_menu` 系）を decode すれば確定可能。
- **V-596-2【低】**: `0x581CC0` init 時の cropx/cropy は `iSafeZoneX/Y`（wide 側でない）で初期化されるが、`0x57D840`（`usz`）は同値20を再設定するため差は出ない。`usz` で `iSafeZone*Wide` に異なる値（例: 40）を入れた場合の cropx の挙動は式どおり（widescreen なら Wide 値）。
- **V-596-3【低】**: 投影行列の正確な要素（`0x57E7C0` が書く `[eax+0x54..0x5c]` の4x4 の意味）は本裁定で「直交投影（±仮想/2）」まで確定。行列の厳密な成分は D3DX 相当のため移植では等価な ortho を組めばよい。

---

## 6. 検証

- probe: **u10_1〜u10_26（26本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `screen`=`0x138E`／`cropx`=`0xFDA`／`cropy`=`0xFD9`／`width`=`0xFCB`／`height`=`0xFCA`（`0x58A1C0` 登録）／`screen()` 本体 `0x58BA02`（`mov eax,[eax+0x68]`）／仮想式 `0x57D7A0`/`0x57D7F0`（定数 `0xA68D70`=960.0d, `0xA688A8`=1280.0f, `0xA688A0`=1280.0d, `0xA68D78`=960.0f）／書き手 `0x581CC0`・`0x57D840`／投影 `0x57E7C0`（`0x57E2D0`/`0x57E330`/`0x57E390`/`0x57E3F0`）／crop 供給 `0x57D330`・`0x57D390`（`0xB135F8`=iSafeZoneX, `0xB13608`=iSafeZoneXWide, 閾値 `0xA68D08`=0.74）／解像度適用 `0x498340`。
- エミュレーション `u10_19` で5プリセット＋3解像度の仮想値を再現（1280×960 基準を確認）。
- 反証: `fMenuScale`/`uVideoResolution`/`sVideoResolution`/`MenuScale` の `.rdata` リテラルは**0件**（[確定負]）。
