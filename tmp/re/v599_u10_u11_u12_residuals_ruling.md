# v599 — U10/U11/U12 残点クローズ（低優先バンドル）: iSize 書込み元・進捗単位・ロード順序・`0x1771` trait・入力状態セレクタ・マウスボタン・ムービースレッドフラグ（ai-todo #96/#164 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u12_24〜u12_33**・全 EXIT=0・冒頭 Test-Path）＋実物 INI。
先行: `tmp/re/v596_u10_screen_virtual_resolution_ruling.md`・`tmp/re/v597_u11_loadingmenu_trait_suppliers_ruling.md`・`tmp/re/v598_u12_intro_movie_playback_ruling.md`
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| 残点 | 判定 | 結果 |
|---|---|---|
| **V-596-1** `iSize W/H:Display` の書き手 | **[確定(命令)]** | 書き手は**2関数**: **`0x497C10(width,height)`**＝`[0xB06C5C]=arg1`／`[0xB06C64]=arg2`（直接セッタ）と、**`0x497E70`**＝`[0xB06C5C]=[0xB350CC]`／`[0xB06C64]=[0xB350D0]`（現在の描画バッファ寸法から同期）＋**widescreen フラグ `[0xB350D7]`＝(`H/W != 0.75`)** を設定。呼出側は video メニュー適用経路（`0x405090`→`0x405CE0`）。 |
| **V-596-2** init 時の cropx/cropy | **[確定(命令)（negative）]** | `0x581CC0`（MenuRoot init）は `iSafeZoneX/Y`（wide 側でない）で初期化するが、`0x57D840`（`usz`）が同値20を再設定するため**差は出ない**。`usz` で `iSafeZone*Wide` に別値を入れた場合のみ式どおり wide 値が使われる。**実装上の追加作業なし**。 |
| **V-596-3** 投影行列の成分 | **[確定(命令)]** | `0x57E7C0` が書くのは `[MenuRoot+0x20..0x34]`（X/Y/Z 原点＋`-0.008` スケール＋`±仮想/2` 範囲）＝**直交投影の等価表現**。厳密な 4x4 成分は D3DX 相当で、移植は等価な ortho を組めばよい（**追加 RE 不要**）。 |
| **V-597-1** 進捗の時間単位 | **[確定(命令)]** | **ミリ秒（ms）**。`0x5AD980` は `[esi+0x40]`（進捗）を `[edi+0x10]`（経過ms）から更新し、`fdiv qword [0xA2FC70]`（=**1000.0**）で秒へ正規化。セクション内ステップ `[esi+0x64]` は経過msが `[0xB14160]`（=**2500**）を超えるごとに `+1`＝**段階（セクション）ベース**（`0xB14168`=100／`0xB14170`=10 のヒステリシス付き）。 |
| **V-597-2** ロード対象リストの供給元 | **[確定(命令)]** | `0x4F96F0` は **`'Textures\Menus\Loading\'`（`0xA498F8`）を返す定数関数**（ロード画像のパス接頭辞）。`0x5AD780` は `_strstr(name,'Menus\Loading')` で判定し、含まなければ `'Menus\Loading'`（`0xA6C4B8`）＋record 名を `'%s%s'`（`0xA2F884`）で合成。リスト構築は `0x5AD440` が `0x65C620`（連結リスト要素追加）で行う。 |
| **V-597-3** セクション順序規則 | **[確定(命令)]** | **セクションは順番に完了する必要がある**。`0x5ADEC0` は `[esi+0x28]`（次に完了すべきセクション index）と完了セクション `ebx` を比較し、**飛び越し（`ebx > [esi+0x28]`）なら `'LoadingMenu sections loading out of order. Trying to use…'`（`0xA6C4D0`）を警告**し、**先行セクションを `100` で埋める**（`0x5ADFC8: [esi+eax*4+0x2c]=0x64`）。 |
| **V-597-4** `0x1771` trait の正体 | **[確定(命令)]** | **「最後に表示したメインメニュー id」を保持する MenuRoot の永続数値 trait**。`0x57D530` が `SetFloat([MenuRoot], 0x1771, menu_id)`（既定 `0x3EB`=StatsMenu）で更新し、id に応じ `0x5A5E00(1/2/3/4)` を呼ぶ。`0x581CC0`（init）が **`0x1771`＝`1003.0`（`0xA68C00`＝`0x3EB`＝StatsMenu）** に初期化。→ **未登録の数値スロット（名前なし）をグローバル状態として流用**。 |
| **V-598-1** `0x403520` arg2 の意味 | **[確定(命令)]** | **状態セレクタ（state selector）**。`0x403520(arg1=action, arg2=state)` → `0x403490(slot, keybyte, state)`。`0x4032D0`（slot==0）の switch: **0=現在押下（down）／1=押下エッジ（pressed）／2=離上エッジ（released）／3=変化（changed）**。`0x4031E0`（slot==1）は別テーブル（`[+0x1B2C]`/`[+0x1B40]`/`[+0x1B50]`）で**エッジ/同時押し**を判定。 |
| **V-598-2** マウスボタンの実体 | **[部分確定]** | マウスボタン名表: **`sMouseLeftButton`(`L-Button`)／`sMouseRightButton`(`R-Button`)／`sMouseMiddleButton`(`Wheel`)／`sMouseButton3..8`(`Button 3..8`)**。擬似アクション **`0x1D`→`[input+0x18f5]`/`[0x19f5]`／`0x1E`→`[0x191d]`/`[0x1a1d]`／`0x1F`→`[0x19ab]`/`[0x1aab]`**（各 0x100 stride の 2 バイト対）。**どの擬似 id が左/右/中か**の厳密対応は本バンドルでは未確定（`0x1D` が左クリック相当の主ボタンである可能性が高い）＝**V-599-2 に継続**。 |
| **V-598-3** `[0xB33425]` の書き手 | **[確定(命令)]** | **`0x40FDD0`**（callers: `0x4053E0`／`0x40D4D0`／`0x66EAF0`）。MoviePlayer スレッド `[0xB33434]` を終了待ち（`WaitForSingleObject`）する直前に **`[0xB33425]=1`**、join 後に **`=0`**。→ **「ムービースレッド終了待ち中」フラグ**。`0x410390` はこれを見て skip 判定を抑止する。 |
| **V-598-4** 音量の動的確認 | **[未確定]（静的 RE 範囲外）** | Bink 音量 API 不在は [確定負]（v598）。実音量が DirectSound 既定に従うことの**動的確認は未実施**（静的 RE では閉じられない）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 V-596-1 iSize 書き手

```
0x497C10 (直接セッタ):
  497C10 mov eax,[esp+4] ; 497C14 mov ecx,[esp+8]
  497C18 mov [0xb06c5c],eax   ; iSize W:Display
  497C1D mov [0xb06c64],ecx   ; iSize H:Display
  497C23 ret
0x497E70 (描画バッファから同期):
  497ED3 mov ecx,[0xb350d0]   ; 高さ
  497EDF mov [0xb06c5c],eax   ; =[0xb350cc] 幅
  497EE4 mov [0xb06c64],ecx   ; =[0xb350d0] 高さ
  497EF0 fcomp qword [0xa31c70](=0.75)
  497EFB jnp ... ; 497EFD mov byte [0xb350d7],1   ; widescreen = (H/W != 0.75)
```
video メニュー適用は `0x405090`（`0x43FD20` で解像度列挙）→ `0x405CE0`（デバイス再生成）。

### 2.2 V-597-1 進捗単位（ms）

```
0x5AD980:
  5AD9CA mov ebp,[edi+0x10]        ; 経過ms（renderer time）
  5AD9CF sub eax,[esi+0x68]        ; 前回基準との差
  5AD9D7 cmp eax,[0xb14160](=2500) ; 2500ms しきい値
  5AD9DF add [esi+0x64],1          ; ★ セクション内ステップ++
  5ADA12 fild [esp+0x14] ; 5ADA1E fdiv qword [0xa2fc70](=1000.0)  ; ms→秒
  5ADF... 0xB14168=100（上限）/0xB14170=10（ヒステリシス）
```

### 2.3 V-597-2 パス接頭辞

```
0x4F96F0: mov eax,0xa498f8 ; ret      ; 0xA498F8 = 'Textures\Menus\Loading\'
0x5AD780: push 'Menus\Loading'(0xA6C4B8) ; _strstr(name, …)
          push '%s%s'(0xA2F884) ; 0x402E30  ; 'Menus\Loading'+名前
```

### 2.4 V-597-3 セクション順序

```
0x5ADEC0:
  5ADF7C mov eax,[esi+0x28]        ; 次に完了すべきセクション
  5ADF7F cmp ebx,eax ; 5ADF81 jge 0x5ADFB6   ; 飛び越し？
  5ADF8F push 'LoadingMenu sections loading out of order…'(0xA6C4D0)
  5ADFC0.. mov [esi+eax*4+0x2c],0x64         ; ★ 先行セクションを 100 で埋める
```

### 2.5 V-597-4 `0x1771` trait

```
0x57D530: mov esi,[esp+8] ; if 0 -> esi=0x3EB ; SetFloat([ecx+0x68],0x1771,esi) ; 0x5A5E00(1..4)
0x581CC0: fld [0xa68c00](=1003.0) ; SetFloat([edi+0x68],0x1771,1003.0)   ; 0x3EB=StatsMenu
```
（`push 0x1771` の登録名検索＝0件 → **名前なしの数値スロット**。）

### 2.6 V-598-1/2 入力状態セレクタとマウスボタン

```
0x403520(arg1,arg2): arg1<0x1D → 29×3 表 [input+0x1B7E]; arg1>=0x1D → 擬似マウス
  0x403490(slot,keybyte,state):
    slot==0 → 0x4032D0(keybyte,state): switch(state 0..3)
        0: test [input+keybyte+0x18f4],0x80        ; down
        1: [0x19f4] up & [0x18f4] down              ; pressed edge
        2: [0x18f4] up & [0x19f4] down              ; released edge
        3: xor [0x19f4],[0x18f4]                    ; changed
    slot==1 → 0x4031E0(keybyte,state): switch(state 0..4) [0x1B2C]/[0x1B40]/[0x1B50]
    slot==0xFE → return 0
    else → 0x402FC0(slot-2,keybyte,state)
マウス名表: sMouseLeftButton(L-Button)/sMouseRightButton(R-Button)/sMouseMiddleButton(Wheel)/sMouseButton3..8(Button 3..8)
```

### 2.7 V-598-3 ムービースレッド終了フラグ

```
0x40FDD0:
  40FDD5 test eax,[0xb33434] ; je 0x40FDFA
  40FDDC mov byte [0xb33425],1   ; ★ join 前
  40FDE3 call [0xa280cc]         ; WaitForSingleObject
  40FDE9 mov [0xb33434],0
  40FDF3 mov byte [0xb33425],0   ; ★ join 後
```

---

## 3. 残点（更新後）

- **V-599-1【低】**: `0x402FC0`（slot≥2）の状態ビット（`[input+slot*0xA0+keybyte+0x60]`）の意味は未確定。
- **V-599-2【低】**: 擬似アクション `0x1D`/`0x1E`/`0x1F` とマウス左/右/中の厳密対応（V-598-2 継続）。
- **V-599-3【低】**: `0x497E70` の `[0xB350CC]`/`[0xB350D0]`（描画バッファ寸法）の供給元。
- **V-598-4【低】**: ムービー音声の実音量（動的確認・静的 RE 範囲外）。

---

## 4. 検証

- probe: **u12_24〜u12_33（10本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x497C10`/`0x497E70`（iSize 書き手）／`0x5AD980`（`fdiv 1000.0`＝ms）／`0x4F96F0`（`'Textures\Menus\Loading\'`）／`0x5ADEC0`（セクション順序＋`0xA6C4D0`）／`0x57D530`＋`0x581CC0`（`0x1771`=`1003.0`）／`0x403520`/`0x403490`/`0x4032D0`/`0x4031E0`（状態セレクタ・jump table `0x403368`/`0x4032B8`）／マウス名表（`0xA62064`〜`0xA620D0`）／`0x40FDD0`（`[0xB33425]`）。
- 実物: `Oblivion_default.ini [Controls]`。
