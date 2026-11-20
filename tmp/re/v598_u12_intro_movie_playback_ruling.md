# v598 — U12 裁定: 導入ムービー `OblivionIntro.bik` の再生仕様（chargen との表示関係・入力・スキップ・音声・起動順）（ai-todo #164 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u12_1〜u12_23**・全 EXIT=0・冒頭 Test-Path）＋実物 XML/INI（`Oblivion - Misc.bsa`・`Oblivion_default.ini`）。
先行: `tmp/re/v557_title_video_ruling.md` §2.2/§2.3/§2.6/§2.7/§2.9・`tmp/re/v560_u1_residuals_ruling.md`・`docs/RE_EXE_REPORT.md` §21.3〜§21.7
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| ⒜ chargen パネルとの表示関係 | **[確定(命令)]** | **動画は全画面（foreground）で、chargen パネルを覆う**。`0x410BA0(path,1,1,0,0,1,0)` は `a7=0`＝**前景モード**で新規ムービーオブジェクトを作り、`0x410A70` が `FUN_004106C0` を**同期ブロックでループ**する。`FUN_004106C0` は毎フレーム**自前でレンダラのクリア＋全画面クアッド描画**（`0x40FA20`・`renderer_vtable[+0x104]`）を行う。→ **ムービー中はメニュー描画ループ（`0x57F170`）が回らない**ため、`RaceSexMenu` は生成済み（`0x57B7E0` が先）だが**描画されない**。ムービー終了後に通常ループへ戻り chargen が前面に出る。 |
| ⒝ 動画中の chargen UI 操作可否 | **[確定(命令)（negative）]** | **操作できない**。v560 の「入力は塞がない」は「ムービー自身が `PeekMessageA`/`TranslateMessage`/`DispatchMessageA` で OS メッセージを空にする」意味であり、**メニューへのクリック配送ではない**。ムービー中は `0x57F170`（メニュー render/update）が回らないのでタブ/Done ボタンは**非描画・非操作**。 |
| ⒞ スキップのキー 5 / 0x1D の実キー | **[確定(命令)]** | **`5` と `0x1D` は「キーコード」ではなく action 系の識別子**。`0x410390` は `FUN_00403520(input, 5, 1)` と `FUN_00403520(input, 0x1D, 1)` を呼ぶ。`0x403520` の arg1 は**アクション id**（`<0x1D`）で、`[input+0x1B7E]` の 29×3 バインド表（stride `0x1D`）を引く。**action 5 = `Activate`**（`0x9F6040` の action 表）で、既定 `Oblivion_default.ini` の `Activate=0039FFFF`＝**DIK 0x39 = Space（スペースキー）**。`0x1D`（=29）は**アクション表外の特殊擬似アクション＝マウスボタン**（`0x40356B`→`0x4035A8` が `[input+0x19F5]`/`[input+0x18F5]` をテスト）→ **マウスクリックでもスキップ**。**タッチ端末の等価操作＝タップ（マウスボタン相当）でスキップ**。 |
| ⒟ 音声の扱い | **[確定(命令)]** | **ムービーの既定音声トラックをそのまま再生**する。`_BinkSetSoundSystem(_BinkOpenDirectSound,0)`（`0x410310`・起動時 `0x40EADB`）で DirectSound を Bink に接続。**`BinkSetVolume` / `BinkSetAudioTrack` / `BinkSetSoundTrack` の import は `.rdata` に 0 件 [確定負]**＝**音量・トラックの明示制御は無い**。ただし**前景ムービーの前後でゲーム音楽を停止/再開**する: `0x66F2B0`-`0x66F2B9` で `0x6A8D00`＋`0x6A9B40`（音楽停止・`[+0xA4]=1`）→ ムービー → `0x66F2DB`-`0x66F2E2` で `0x6A8D50`＋`0x6A9C00`（音楽再開・`[+0xA4]=0`）。`0x410A70` 自身も前景ムービー開始時に `0x6A9B40`、終了時に `0x6A9C00` を呼ぶ。→ **BGM とは競合しない**。 |
| ⒠ 厳密な起動順 | **[確定(命令)]** | 起動: `0x40EADB`（Bink 音声系初期化）→ `0x40EAE0` `0x410E40`（**`CreateThread` "MoviePlayer"＝非同期**で `sIntroSequence` プレイリスト＝**法務/ロゴ4本**）→ `0x40F0AF` `0x410BA0(sMainMenuMovieIntro="Oblivion iv logo.bik",1,1,0,0,0,0)`（**同期**・`a6=0`＝**高さ基準 480**）→ `0x40F0C3` `0x5B5C90`（メインメニュー表示）。**`OblivionIntro.bik` はここでは流れない**。NEW 確定後: `0x66EAF0`（世界ロード）末尾で `0x57B7E0`（RaceSexMenu 生成）→ `0x410BA0(sOblivionIntro="OblivionIntro.bik",1,1,0,0,1,0)`（**同期・前景**・`a6=1`＝**幅基準 640**）。 |

**移植への指針**: `TitleScreen::setupIntroVideo`（タイトル初期化時に再生）は**位置が誤り**。正しくは「**NEW 確定 → RaceSexMenu 生成 → OblivionIntro を同期・全画面で再生 → chargen を前面表示**」。再生中は chargen を**描画・操作しない**（メニュー render ループを止める）。スキップは**タップ**（または Space 相当）。音声は**既定トラックを再生**し、タイトル BGM は**ムービー前に停止・後に再開**。`IntroVideoActivity` の起動スプラッシュ4本は `sIntroSequence`（法務/ロゴ）に対応し、iv ロゴは別（`sMainMenuMovieIntro`）。

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 再生位置と順序（`0x66EAF0` 末尾・`0x66F295`→`0x66F2D1`）

```
66F260 cmp byte [ebp+0x7f8],0        ; 新規ゲームフラグ
66F295 call 0x57B7E0                 ; ★ RaceSexMenu 生成（chargen 開始）
66F29C mov byte [ebp+0x7f8],0
66F2AB call 0x572EC0
66F2B2 call 0x6A8D00                 ; 音楽停止系
66F2B9 call 0x6A9B40                 ; ★ 音楽停止（[+0xA4]=1）
66F2BE mov ecx,[0xb030a4]            ; = sOblivionIntro（"OblivionIntro.bik"）
66F2C4 push 0 / 66F2C6 push 1 / 66F2C8 push 0 / 66F2CA push 0 / 66F2CC push 1 / 66F2CE push 1
66F2D1 call 0x410BA0                 ; ★ 導入ムービー（同期・前景・a6=1 幅基準）
66F2DB call 0x6A8D50
66F2E2 call 0x6A9C00                 ; ★ 音楽再開（[+0xA4]=0）
```
`0x57B7E0`（chargen 開始）: `0x678D90`（player init）→ `0x67ACA0` → `0x589B70(0x40C)` で **RaceSexMenu 生成** → `0x9832E6` で RTTI cast → `0x5CA010(1)`（`0x5CA010` が `Data\Menus\CharGen\race_sex_menu.xml` をロード）。→ **RaceSexMenu の生成は完了、表示はムービー後**。

### 2.2 前景ムービーの同期ブロックと全画面描画

`0x410BA0(path,a2,a3,a4,a5,a6,a7)`: `a7==0`（Intro/iv ロゴ）→ `0x410BCD` で `0x28` B の新規ムービーオブジェクトを作り `[0xB33428]` に格納 → `0x410C29 call 0x410A70(movie,a2..a6)`。

`0x410A70`: `[movie+0x20]=1`（**前景**）→ 音楽停止 `0x6A9B40` → `0x4104C0`（open）→ ループ `0x410AC9 call 0x4106C0(movie,a2,a4)`（`al!=0` の間）→ 終了時 `0x6A9C00`×2。

`0x4106C0`（毎フレーム）:
```
4106C8 call 0x410390          ; ★ メッセージポンプ＋skip キー判定
4106E6 call [0xA28330]        ; _BinkWait
4106FC call 0x40FCA0          ; フレーム待ち
410726 call [0xA28334]        ; _BinkNextFrame
41073D..410755 renderer_vtable[+0xAC](...)   ; ★ 画面クリア/開始
410765..41076C renderer_vtable[+0xA4]
410790 call 0x40FA20          ; ★ 全画面クアッド描画
4107AD renderer_vtable[+0xA8]
4107C6 renderer_vtable[+0x44] ; present
```
`0x40FA20`（クアッド描画）: `0x40F970`（バッファ準備）→ `renderer[+0xE4](7/0xE/0xA8)`（描画ステート）→ テクスチャ矩形を全画面へ写す（`0x40F760`）。→ **ムービーが画面全体を占有**。

### 2.3 入力ポンプとスキップ判定（`0x410390`）

```
410394..4103DF PeekMessageA(PM_REMOVE)/TranslateMessage/DispatchMessageA  ; OS メッセージを空にする
4103E3..4103F5 入力デバイス更新（[0xB3F928]+0x280 vtable+0xC）
4103FF cmp byte [0xB33426],0 ; skip 済みなら return
41041C..41043A 前景ムービー以外（背景）なら MessageMenu(0x3E9) 存在チェック
41044C mov esi,[0xB33398+0x20]  ; 入力オブジェクト
41045C push 1 ; 41045E push 5 ; 410462 call 0x403520   ; ★ action 5 = Activate
41046B push 1 ; 41046D push 0x1d ; 410471 call 0x403520 ; ★ 擬似 0x1D = マウス
（押下中かつ MoviePlayer スレッドが STILL_ACTIVE(0x103) なら [0xB33426]=1）
```
`0x403520(arg1, arg2)`: `arg1 < 0x1D` なら `[input+0x1B7E]` の **29×3 バインド表**（`arg1 + k*0x1D`, k=0..2）を引き、各バイトを `0x403490(slot=k, byte, arg2)` でテスト（`0xFF`=未割当）。`arg1 >= 0x1D`（29/30/31）は**特殊擬似アクション**（`0x40356B`→`0x4035A8`/`0x403594`/`0x403580` が `[input+0x19F5]`/`[0x18F5]` 等の**マウスボタン状態**をテスト）。

action 表（`0x9F6040` が `0x419A70(id,name)` で一括登録・`sUActn*`）:
```
0 Forward / 1 Back / 2 Slide Left / 3 Slide Right / 4 Use(Attack) / 5 Activate
6 Block / 7 Cast / 8 Ready Weapon / 9 Sneak / 10 Run / 11 Always Run / 12 Auto Move
13 Jump / 14 Change View / 15 Journal(Menu Mode) / 16 Wait / 17 Quick Menu
18-25 Quick1..Quick8 / 26 QuickSave / 27 QuickLoad / 28 Grab
```
既定 `Oblivion_default.ini` `[Controls]`（第2バイト＝DIK スキャンコード）:
```
Forward=0011FFFF(W) Back=001FFFFF(S) Slide Left=001EFFFF(A) Slide Right=0020FFFF(D)
Activate=0039FFFF(Space) Menu Mode=000FFFFF(Tab) Jump=0012FFFF(E) Crouch/Sneak=001DFFFF(LCtrl)
```
→ **action 5 = Activate = Space（DIK 0x39）**。**`0x1D` はマウスボタン**。→ **スキップ＝スペースキー or マウスクリック**（タッチ端末はタップ）。

### 2.4 音声

```
0x410310: cmp [0xB33398+0x24],0 / cmp [0xB16178],0 / call [0xA2833C](=BinkSetSoundSystem, BinkOpenDirectSound(0xA28338), 0)
0x40EADB call 0x410310   ; 起動時に Bink へ DirectSound を接続
0x410257 call [0xA28344] ; _BinkOpen（0x410160 内）
```
`.rdata` の import 名: `BinkOpen`(0xB00927)／`BinkSetSoundSystem`(0xB00945)／`BinkOpenDirectSound`(0xB0095D)／`BinkDoFrame`(0xB00905)／`BinkCopyToBuffer`(0xB008E9)／`BinkNextFrame`(0xB00977)／`BinkWait`(0xB0098B)／`BinkPause`(0xB00917) は存在。**`BinkSetVolume`／`BinkSetAudioTrack`／`BinkSetSoundTrack`／`BinkGetTrackType`／`BinkGetTrackData` は 0 件 [確定負]** → **既定トラックを既定音量で再生**。
`0x6A9B40`（音楽停止）: `[this+0xA4]=1`。`0x6A9C00`（再開）: `[this+0xA4]=0`。

### 2.5 起動順（`0x40D4D0`）

```
40EADB call 0x410310                        ; Bink 音声系初期化
40EAE0 call 0x410E40(sIntroSequence, -1, [0xB030C4])  ; ★ CreateThread "MoviePlayer"（非同期）
40F0AF mov ecx,[0xB0308C]                    ; = sMainMenuMovieIntro（"Oblivion iv logo.bik"）
40F0B5..40F0BD push 0,0,0,0,1,1,ecx           ; a7..a1（a6=0＝高さ基準）
40F0BE call 0x410BA0                         ; ★ iv ロゴ（同期）
40F0C3 call 0x5B5C90                         ; ★ メインメニュー表示
```
GMST 実値（`0xB03084` 起点・`{value,name}` 対）:
```
sIntroSequence     = 'bethesda softworks HD720p.bik,2k games.bik,game studios.bik,Oblivion Legal.bik'
sMainMenuMovieIntro= 'Oblivion iv logo.bik'
sMainMenuMovie     = 'Map loop.bik'
sCreditsMenuMovie  = 'CreditsMenu.bik'
sOblivionIntro     = 'OblivionIntro.bik'
```
`0x410E40`: `CreateThread(0x410E20, ...)` → `0x747830("MoviePlayer")`。プレイリスト `0x410C40`→`0x410D10` が `strtok(",")` で分割し各要素を `0x410BA0(token,1,1,0,[0xB030B4],[0xB33427],0)` で再生。

→ **法務/ロゴ4本（非同期）→ iv ロゴ（同期・高さ基準）→ メインメニュー**。**`OblivionIntro.bik` は NEW 確定後の世界ロード（`0x66EAF0`）でのみ再生**され、起動時には流れない。

---

## 3. 既存裁定との整合・訂正

| 先行 | 内容 | 本裁定 |
|---|---|---|
| v557 §2.2 | 起動チェーン（sIntroSequence→iv ロゴ→メニュー） | **整合**（§2.5 で再確認・`a6` 値を追記）。 |
| v557 §2.6 | 「入力は塞がない・キー 5/0x1D は [部分確定]」 | **⒞ を [確定(命令)] に昇格**: `5`＝action Activate＝Space、`0x1D`＝マウスボタン。 |
| v560 | RaceSexMenu 生成が先・ムービーが後（同一ブロック） | **整合**（⒜ で「ムービーは全画面で chargen を覆い、ムービー中は chargen 非描画・非操作」まで確定）。 |
| `docs/RE_EXE_REPORT.md` §21.7 | 「OblivionIntro は New Game 確定後に再生」 | **整合**。移植の `setupIntroVideo`（タイトル初期化時）は位置が誤り。 |

---

## 4. 移植への反映（要修正）

- `TitleScreen::setupIntroVideo` をタイトル初期化から**外し**、「NEW 確定 → chargen 生成 → OblivionIntro 同期再生 → chargen 表示」に移す。
- ムービー中は **chargen を描画・操作しない**（メニュー render/update を止める）。
- スキップは**タップ**（マウス相当）。Space も等価。
- 音声は**既定トラックを再生**（音量制御なし）。再生前にタイトル BGM を停止し、終了後に再開する。
- `IntroVideoActivity`（起動スプラッシュ4本）は `sIntroSequence` に対応。iv ロゴは `sMainMenuMovieIntro`（別扱い）。

---

## 5. 残点

- **V-598-1【低】**: `0x403520` の `arg2`（0x403490 の arg3）の厳密な意味（ボタン番号か状態種別か）は本裁定で「マウスボタン選択子」まで確定。`0x4031E0`/`0x4032D0` の switch 分岐の全対応表は未列挙。
- **V-598-2【低】**: `0x1D`/`0x1E`/`0x1F` が具体的に左/右/中どのマウスボタンかは未確定（`0x40D706` は `0x1F` を使用）。
- **V-598-3【低】**: `[0xB33425]`（前景ムービー判定の別フラグ）の書き手は未特定。
- **V-598-4【低】**: ムービーの音声トラック音量が DirectSound 既定に従うことの動的確認は未実施（静的には Bink 音量 API 不在）。

---

## 6. 検証

- probe: **u12_1〜u12_23（23本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x66F295`(chargen)→`0x66F2D1`(movie)／`0x57B7E0`（`0x589B70(0x40C)`→`0x5CA010`）／`0x410BA0`→`0x410A70`→`0x4106C0`（`0x410390` ポンプ＋`0x40FA20` 全画面クアッド）／`0x410390`（`push 5`/`push 0x1D`）／`0x403520`（29×3 表 `[input+0x1B7E]` stride `0x1D`）／`0x9F6040`（action 表・action5=Activate）／`0x410310`（`_BinkSetSoundSystem`）／`0x40EADB`・`0x40EAE0`・`0x40F0BE`・`0x40F0C3`（起動順）／GMST `0xB03084`〜。
- 実物: `Oblivion_default.ini [Controls]`（`Activate=0039FFFF`＝Space）・`menus\options\controls_menu.xml`（`Oblivion - Misc.bsa` 抽出）。
- 反証: `BinkSetVolume`/`BinkSetAudioTrack`/`BinkSetSoundTrack` の `.rdata` リテラル＝**0 件**（[確定負]）。
