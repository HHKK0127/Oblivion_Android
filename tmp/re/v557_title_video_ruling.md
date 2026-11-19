# v557 — U7 裁定書: タイトル画面の映像演出（ai-todo #166）

対象: `Oblivion.exe` v1.2.0416 (Steam) `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
根拠: 命令実読（probe v557_1〜v557_17）＋ BSA 実物 XML ＋ `.bik` ヘッダ実測。原本は読み取り専用。
判定: `[確定]` / `[確定(命令)]` / `[確定(XML+命令)]` / `[部分確定]` / `[推定]` / `[確定負]`

---

## 1. 結論（依頼 ①〜⑥）

| # | 質問 | 判定 | 結論 |
|---|------|------|------|
| ① | メインメニューは「ロゴのアニメーション→背景」の2段か | **[確定(XML+命令)]** | **2段ではない。背景は動画、ロゴは静止画2枚の alpha クロスフェード。** 背景 = `Map loop.bik`（ループ）、ロゴ = `tes_oblivion_logo_bink.dds`（760x169, alpha=trait user3）→ `tes_oblivion_logo_final.dds`（748x159, visible=trait user0）の**重ね合わせ**。ロゴ用の `.bik` はタイトルでは使われない。 |
| ② | ロゴ表示の時間・フェード時間に規定値はあるか | **[確定(XML+命令)]** | `main_menu.xml` の `<user4> 3 </user4>`（コメント: *how many seconds to fade from bink logo to real logo*）が唯一の規定値。コードは `user3`(trait 0xFB1) を 255 → 0 へ **user4 秒**で補間する。**`IV_LOGO_HOLD_MS=1200` に対応する定数・GMT は存在しない [確定負]**。 |
| ③ | 背景ムービーはループするか／継ぎ目は先頭復帰か逆再生か | **[部分確定]** | **ループする。** 明示的な巻き戻し命令・逆再生フラグは存在しない（`[確定負]`）。毎フレーム `BinkNextFrame` を呼び、フレーム数到達後の継続判定は「スタック第1引数が非 0 なら継続」。描画ループ `0x57F170` は毎回 `1` を渡すため停止しない。継ぎ目が「先頭復帰」かは binkw32 内部（`BinkNextFrame`）の挙動で、EXE 側には判断材料が無い。 |
| ④ | 背景ムービーに音声はあるか | **[確定]** | **無い。** `Map loop.bik` のヘッダは `audioTracks=0`。binkw32 import は 9 個のみで `BinkSetVolume` は存在せず（音量 API 自体が無い）、起動時に `BinkSetSoundSystem(BinkOpenDirectSound, 0)` を 1 回呼ぶだけ。背景開始時に BGM を止める処理も無い。 |
| ⑤ | credits を 1% の確率で出す演出は原版にあるか | **[確定負]** | **存在しない。** `0.01f`(0x3C23D70A) は `.text` に 10 箇所あるが全て `0x89A230`〜`0x94A630` 帯（Havok/描画系）で、映画コード（`0x40xxxx`/`0x57xxxx`/`0x5Bxxxx`）には 0 箇所。クレジット動画名は GMST `sCreditsMenuMovie` の固定デフォルト値。 |
| ⑥ | 移植 `map_loop.mp4` / `oblivion_iv_logo.mp4` / `credits_menu.mp4` に対応する原版 `.bik` 実名と画面別使い分け | **[確定]** | 下表 §1.1。 |

### 1.1 `.bik` 実名と画面別の使い分け（GMST デフォルト値）

| GMST | デフォルト値（実バイト） | 使われる場面 |
|------|--------------------------|--------------|
| `sIntroSequence` | `bethesda softworks HD720p.bik,2k games.bik,game studios.bik,Oblivion Legal.bik`（カンマ区切りプレイリスト） | **起動時スプラッシュ**（非同期スレッド） |
| `sMainMenuMovieIntro` | `Oblivion iv logo.bik` | 起動スプラッシュの直後（同期） |
| `sMainMenuMovie` | `Map loop.bik` | **タイトル背景ループ** |
| `sCreditsMenuMovie` | `CreditsMenu.bik` | クレジット画面 |
| `sOblivionIntro` | `OblivionIntro.bik` | NEW 確定後（キャラ作成開始と同ブロック） |
| （文字列なし） | — | `OblivionOutro.bik` は **EXE 内に名前が無い [確定負]**（ファイルは `Data\Video\` に存在） |

対比: `map_loop.mp4` ← `Map loop.bik`／`oblivion_iv_logo.mp4` ← `Oblivion iv logo.bik`／`credits_menu.mp4` ← `CreditsMenu.bik`。

---

## 2. 根拠

### 2.1 最重要の発見 — `.data 0xB03084` は「ポインタスロット」ではなく **GMST デフォルト値テーブル**

`0xB03084` から **8 バイト間隔で {デフォルト値, GMST 名ポインタ}** が並ぶ（probe v557_13/v557_15 で実読）。

| 値アドレス | デフォルト値 | GMST 名 | 名前 VA |
|---|---|---|---|
| `0xB03084` | `0xA31A10` | `sIntroSequence:General` | `0xA31A60` |
| `0xB0308C` | `0xA31A78` | `sMainMenuMovieIntro:General` | `0xA31A90` |
| `0xB03094` | `0xA31AAC` | `sMainMenuMovie:General` | `0xA31ABC` |
| `0xB0309C` | `0xA31AD4` | `sCreditsMenuMovie:General` | `0xA31AE4` |
| `0xB030A4` | `0xA31B00` | `sOblivionIntro:General` | `0xA31B14` |
| `0xB030AC` | `3.0f` (`0x40400000`) | `fStaticScreenWaitTime:General` | `0xA31B2C` |
| `0xB030B4` | `1` | `bPreloadIntroSequence:General` | `0xA31B4C` |
| `0xB030BC` | `26214400` (`0x01900000` = 25 MiB) | `iPreloadSizeLimit:General` | `0xA31B6C` |

* 文字列の実バイト: `0xA31A10` = `"bethesda softworks HD720p.bik,2k games.bik,game studios.bik,Oblivion Legal.bik"`（**1 本の NUL 終端文字列**。個別の `2k games.bik` 等は独立したリテラルではないため参照 0 件）
* **帰結**: これらの値は ESM の GameSetting で上書きできる（mod でタイトル動画を差し替え可能）。v555/v556 で「ポインタスロット」と呼んだのは誤りで、正しくは **GMST の現在値フィールド**。コードは `mov ecx,[0xB030A4]` のように現在値を読む。
* `0xB030BC`(=`iPreloadSizeLimit`) が `FUN_00410160` の `0x2000` フラグ条件の閾値、`0xB030AC`(=`fStaticScreenWaitTime`) が `.dds` 静止画の待ち時間、`0xB030B4`(=`bPreloadIntroSequence`) がプレイリスト再生時の引数、という対応も同時に確定。

### 2.2 起動チェーン `0x40D4D0`

| 箇所 | 内容 |
|---|---|
| `0x40EADB` | `FUN_00410310` = `_BinkSetSoundSystem(_BinkOpenDirectSound, 0)` |
| `0x40EAE0` | `FUN_00410E40` → `CreateThread`（"MoviePlayer"）。**非同期**で `sIntroSequence` プレイリストを再生 |
| `0x40F0AF` | `mov ecx,[0xB0308C]` → `FUN_00410BA0(iv logo, 1, 1, 0, 0, 0, 0)`（**同期**・約 13.9 秒 = 418f/29.97fps） |
| `0x40F0C3` | `0x5B5C90` = メインメニュー表示（§2.4） |

### 2.3 プレイリスト実行 `0x410D10`（スレッド本体は `0x410E20`）

* 引数の文字列をヒープへ複製し **`strtok(buf, ",")`**（`0x98349C`）で分割
* 各要素について `[0xB33426]`（skip）が 0 かつ `FUN_00410390(1)` が真なら:
  * 末尾 3 文字が `"bik"`（`0x9836C9`）→ `FUN_00410BA0(token, 1, 1, 0, [0xB030B4], [0xB33427], 0)`（`0x410DC9`）
  * 末尾 3 文字が `"dds"` → `FUN_00410840(token)`（`0x410DE6`）＝**静止画を `fStaticScreenWaitTime` 秒表示**
* したがって起動時は **bethesda softworks HD720p → 2k games → game studios → Oblivion Legal** の 4 本を順に再生し、その後 iv ロゴ（同期）→ メインメニュー。

`FUN_00410840`（`.dds` 静止画）の待ち時間は実測どおり:
```
0x4109EB  GetTickCount()                      ; 開始時刻
0x410A05  fld [0xB030AC] (=3.0f)
0x410A0B  fmul qword [0xA2FC70] (=1000.0)     ; 3000 ms
0x410A35  GetTickCount() ; 0x410A37 cmp edi,eax ; jbe → 終了
0x410A3B  FUN_00410390(1)                     ; キー中断可
```

### 2.4 メインメニュー

**ctor `0x5B5F20`**（vtable `0xA6CB1C`・グローバル `0xB3B40C`）
* `0x5B5F78`: 既存ムービー（`[0xB33428]`）が無ければ `0x5B5F98` で `FUN_00410C40([0xB03094]="Map loop.bik", 1)`
* `0x5B5FB3`: `[0xB3C0EC]=1`、`0x6AC330(3)`（音楽）
* `0x5B5FF0`: `[0xB3B409]=1`

**表示 `0x5B5C90`**
```
0x5B5CA8  SetTrait(0xFB1 /*user3*/, 255.0)
0x5B5CBD  GetTrait(0xFB2 /*user4*/)                 ; フェード秒（XML では 3）
0x5B5CDF  FUN_00589980(menu, 0xFB1, 255.0, 0.0, 秒)  ; user3 を 255→0 へ 秒 で補間
0x5B5CE4  背景ムービー有無で visible(0xFA1) を 1.0 / 2.0 に設定（0x5B5D2B / 0x5B5D52）
```

**`FUN_00589980`**（`0x589980`）は「現在値が目標値と違えば `FUN_00588A70(menu, trait, from, to, 秒)` で補間を登録する」だけの関数（decompile 294839–294898）。

**XML 実物**（`menus\options\main_menu.xml`, 6554 B。`Oblivion - Misc.bsa` から `bethesda_structs` で直接抽出）
```xml
<user0> &true; </user0>   <!-- is the logo visible set by code -->
<user1> &true; </user1>   <!-- are the buttons visible set by code -->
<user2> &false; </user2>  <!-- is Press Start visible set by code -->
<user3> 255 </user3>      <!-- alpha value set by animated code -->
<user4> 3 </user4>        <!-- how many seconds to fade from bink logo to real logo -->
...
<image name="oblivion_logo">                       <!-- 748x159, depth 2 -->
  <visible> <copy src="MainMenu" trait="user0"/> </visible>
  <filename> Menus\Loading\tes_oblivion_logo_final.dds </filename>
  <image name="oblivion_logo">                     <!-- 内側 760x169, x=-5 y=-5 -->
    <filename> Menus\Loading\tes_oblivion_logo_bink.dds </filename>
    <alpha> <copy src="MainMenu" trait="user3"/> </alpha>
  </image>
</image>
```
* `backdrop` = `Menus\Loading\loading_background.dds` 1280x960・`<visible> false </visible>`・`id 8` ＝ **未使用**（移植が背景静止画を出す根拠にはならない）
*　ボタンは `id 2`=Continue / `3`=New / `4`=Load / `5`=Options / `6`=Credits / `7`=Exit（`button_layout`、`y=725`、`visible=user1`）

**trait ID の実測**（名前→ID テーブル `0x58A1C0` を機械走査・194 組）
`visible=0xFA1` / `alpha=0xFA7` / `user0=0xFAE` / `user1=0xFAF` / `user2=0xFB0` / **`user3=0xFB1`** / **`user4=0xFB2`** / `xdefault=0xFF0` / `zoom=0xFD2` / `depth=0xFAB` / `filename=0xFE6`。
→ **0xFB1 = user3、0xFB2 = user4** であることが命令側（`0x58ACB4` `push 0xFB1; push "user3"`／`0x58ACC5` `push 0xFB2; push "user4"`）で裏付けられる。

### 2.5 背景ムービー（ループ）の機序

* `FUN_00410C40(path, a2)` = **背景モード**で開く: 再生中ムービーがあれば `_BinkPause(handle,1)` で退避し、`FUN_004105F0` で新規ムービーを `movie+0x20 = 2`（=背景）で初期化。**音楽停止 `FUN_006A9B40` は呼ばない**（前景モードの `FUN_00410A70` だけが呼ぶ）。
* 描画ループ `0x57F170` の毎フレーム処理:
```
0x57F3A5  eax = [0xB33428]            ; 現在のムービー
0x57F3AE  if (eax[0x20] != 2) skip
0x57F3B9  push 1 ; push 0
0x57F3BC  FUN_00410810(0, 1)
```
* `FUN_00410810(a,b)` → `FUN_004106C0(movie, a, b)`。フレーム前進部:
```
0x410726  _BinkNextFrame(handle)
0x41072C  [movie+0xC] += 1            ; 再生フレーム数
0x4107D4  eax = [movie]               ; Bink ハンドル
0x4107D7  ecx = [movie+0xC]
0x4107DA  cmp ecx, [eax+8]            ; Bink 側のフレーム数と比較
0x4107DD  jb  → 継続
0x4107DF  cmp byte [esp+0x10], 0      ; = FUN_004106C0 の第1スタック引数
0x4107E4  jne → 継続
0x4107E6  return 0                    ; 停止
```
→ 描画ループは常に `1` を渡すので**フレーム数到達後も止まらない**。巻き戻し命令・逆再生フラグは無い。
継ぎ目（先頭復帰か）は binkw32 内部の `BinkNextFrame` の挙動で、EXE 側に判断材料は無い **[部分確定]**。ピンポン（逆再生）を示す命令は無い **[確定負]**。

### 2.6 入力の塞がり方（U1 ⒝ 兼用）

`FUN_00410390(a)`（`0x410390`）はムービーの毎フレーム先頭で呼ばれる:
1. `PeekMessageA(…, PM_REMOVE)` → `TranslateMessage` → `DispatchMessageA` で OS メッセージを空にする（`0x410394`〜`0x4103DF`）
2. `[0xB3F928]+0x280` の入力デバイス vtable `+0x0C` を呼ぶ（入力更新）
3. `[0xB33426]`（skip）が立っていれば中断
4. 前景ムービー中に `FUN_00582160(0,1)` と `MessageMenu`(0x3E9) が存在すれば中断
5. `a≠0` なら `[0xB33398]+0x20` の入力オブジェクトへ `FUN_00403520(1, 5)` / `FUN_00403520(1, 0x1D)` を問い合わせ、押下中かつ `GetExitCodeThread([0xB33434]) == 0x103`（STILL_ACTIVE）なら `[0xB33426]=1`（skip）

→ **入力は「塞がれる」のではなく、ムービー自身がメッセージポンプを回して特定キーを skip として消費する**。キーコード `5` / `0x1D` の実キー同一性は未確定 **[部分確定]**。

### 2.7 `FUN_00410BA0` の引数（U1 ⒜ 兼用）

`FUN_00410BA0(path, a2, a3, a4, a5, a6, a7)` の転送（`0x410C05`〜`0x410C29` の push 列を実読）:
```
FUN_00410A70(movie, a1=path, a2, a3, a4, a5, a6)
  ├─ FUN_004104C0(movie, a2, a3, a5, a6)   ; 0x410A90–0x410AA9
  └─ FUN_004106C0(movie, a2, a4)           ; 0x410ABC–0x410AC9
```
* `a7 == 0` → ムービーオブジェクトを新規 `0x28` B 確保（前景）。`a7 != 0` → 既存 `[0xB33440]` を再利用（`0x410BA3`）。
* `a2` = 入力チェック有効化（`FUN_004106C0` の第1引数＝フレーム到達後の継続可否も兼ねる）
* `a3` = `FUN_004104C0` の第2引数（ファイル存在／`0x2000` フラグ条件）
* `a4` = `FUN_004106C0` の第2引数
* `a5` = `FUN_004104C0` の第4引数（`FUN_00410160` のサイズ閾値判定に渡る）
* **`a6` = `FUN_004104C0` の第5引数 = フィット基準**（`0` → 高さ基準 `480`=`0xB06C50`／非 0 → 幅基準 `640`=`0xB06C4C`）
  → 依頼の「第6引数 `1`」は **幅基準（640）でスケール**の意味。Intro=`1`、iv ロゴ=`0`（高さ基準）、プレイリスト=`[0xB33427]`。

### 2.8 クレジット

* GMST `sCreditsMenuMovie` のデフォルトは `CreditsMenu.bik`（`0xB0309C` → `0xA31AD4`）。
* `menus\options\credits_menu.xml`（942 B 実物）は **`CreditsMenu` というテキストスクロール用メニュー**で、`<image name="credits_rect">`（`id 1`・`clipwindow`・`width 0`・`x 640`）だけ。**`.bik` 参照は無い**。
* したがってクレジット動画はメニュー XML 起点ではなく、メニュークラス（`&CreditsMenu; = 0x417`）側の処理。GMST テーブルは 8 バイト間隔の配列なので `[0xB0309C]` への直接 imm32 参照が無いのは正常（`mov eax,[base+index*8]` 経由）。再生呼出箇所の特定は未完 **[部分確定]**。
* 1% 抽選が無いことは §1 ⑤ のとおり **[確定負]**。

### 2.9 `.bik` ヘッダ実測（`Data\Video\*.bik`・読み取りのみ）

| ファイル | 署名 | フレーム | 解像度 | fps | audioTracks |
|---|---|---|---|---|---|
| `bethesda softworks HD720p.bik` | BIKi | 300 | 1280x720 | 2997/100 | 0 |
| `2k games.bik` | BIKi | 180 | 1280x720 | 2997/100 | 0 |
| `game studios.bik` | BIKi | — | 1280x720 | 2997/100 | 0 |
| `Oblivion Legal.bik` | BIKi | 122 | 1280x720 | 2997/100 | 0 |
| `Oblivion iv logo.bik` | BIKi | 418 | 1280x720 | 2997/100 | 0 |
| `Map loop.bik` | BIKi | 750 | 1280x720 | 2997/100 | **0** |
| `CreditsMenu.bik` | BIKi | 1712 | 1280x720 | 2997/100 | 0 |
| `OblivionIntro.bik` | BIKi | 3461 | 1280x720 | 2997/100 | **1** |
| `OblivionOutro.bik` | BIKi | 1739 | 1280x720 | 2997/100 | **1** |

* 音声トラックがあるのは Intro / Outro のみ。
* `Oblivion iv logo.bik` = 418f ÷ 29.97 = **13.95 秒**（移植の「IV ロゴ動画」区間に相当）。
* `Map loop.bik` = 750f ÷ 29.97 = **25.03 秒**（ループ 1 周期）。

---

## 3. 移植への示唆（正典との差分）

1. **タイトル構成**: 「IV ロゴ動画 → 1200ms 静止 → 背景ループ」は原版と異なる。原版は
   `起動スプラッシュ 4 本（非同期） → iv ロゴ動画（同期・13.95s） → メインメニュー（背景ループ + ロゴ静止画 2 枚の 3 秒クロスフェード）`。
   移植が「ロゴ動画 → 静止保持」としている部分は、原版では**ロゴ静止画の alpha フェード（user4 秒）**が対応する。
2. `IV_LOGO_HOLD_MS = 1200` は原版に根拠が無い（[確定負]）。置き換えるなら `main_menu.xml` の `user4`（=3 秒）を使う。
3. 背景の `setVolume(0f, 0f)` は結果としては正しいが、原版は**音声トラックが無い**（`audioTracks=0`）ため消音 API 自体が存在しない。
4. `credits_menu.mp4` の 1% 抽選は独自実装（削除候補）。クレジットは `CreditsMenu.bik` を明示的に再生する画面。
5. 背景静止画 `loading_background.dds` は `visible=false` で未使用。移植が背景静止画を出す根拠にはならない。

---

## 4. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-557-1 | ループの継ぎ目が「先頭復帰」か「最終フレーム保持」か（binkw32 `BinkNextFrame` 内部。EXE 側に材料なし） | 中 |
| V-557-2 | `BinkOpen` フラグ `0x2000` / `0x08000000` の SDK 名（`iPreloadSizeLimit` 閾値と `SuspendThread` ガード条件から合成されることは確定） | 中 |
| V-557-3 | `sCreditsMenuMovie` の実再生呼出箇所（GMST 配列の添字アクセス経由で imm32 参照が無い） | 中 |
| V-557-4 | `FUN_00403520(1, 5)` / `(1, 0x1D)` の実キー同一性（skip キー） | 低 |
| V-557-5 | `visible`(0xFA1) を 1.0/2.0 の 2 値で書き分ける理由（どちらも非 0＝可視。0 との区別のみ意味がある） | 低 |

---

## 5. probe 一覧（`%TEMP%\opencode\`）

`v557_1`〜`v557_17`（EXIT=0）。主なもの:
* `v557_5` binkw32 import 9 個・`FUN_004102C0`・`0x57F170` の背景ムービー呼出
* `v557_6` `FUN_004106C0` フレームループ／`FUN_00410390` 入力ポンプ
* `v557_7` 起動チェーン・`FUN_00410BA0` の a7 分岐・IAT 名
* `v557_11`/`v557_13` 文字列 VA と参照、GMST テーブル
* `v557_14` プレイリスト `0x410D10`・`.dds` 静止画 `0x410840`
* `v557_15` GMST 名の参照・メニュー型 ID 走査
* `v557_16`/`v557_17` trait 名→ID テーブル（194 組）の機械抽出
* BSA 抽出: `v557_bsa3.py` → `%TEMP%\opencode\bsa_menus\title_main_menu.xml`（6554 B）/ `menus\options\credits_menu.xml`（942 B）/ `race_sex_menu.xml`（22156 B）/ `stats_menu.xml`（102783 B）

**注記（ツール）**: `tools/BSAFileExtractor/extracted/menus/**` の 4 ファイルは全て 2620 B で壊れている（`BSAFileExtractor.py:105-110` が探索ループ変数 `archive_file` を使い回すバグ）。本裁定は `bethesda_structs` で BSA から直接抽出した実物を根拠にした。
