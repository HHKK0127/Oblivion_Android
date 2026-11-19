# v568 — V-557-1..3 クローズ: ループ継ぎ目 / `BinkOpen` フラグ名 / `CreditsMenu.bik` の再生箇所（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v568_1〜v568_4**・全 EXIT=0）＋ `docs/Oblivion_decompiled.c`。
先行: `tmp/re/v557_title_video_ruling.md`（U7）／`docs/RE_EXE_REPORT.md` §14.3
判定: `[確定(命令)]` / `[確定負]` / `[未確定]`

---

## 1. 結論

| 残点 | 判定 | 結論 |
|---|---|---|
| **V-557-3** `sCreditsMenuMovie` の実再生箇所 | **[確定(命令)] 再生箇所は存在しない** | `CreditsMenu.bik` を再生するコードは **EXE に無い**（§2.1 の3系統すべて否定側で実測）。クレジット画面は**動画を出さない**（本文は外部 `Data\Credits.txt`、`v364` と整合）。 |
| **V-557-1** ループの継ぎ目（先頭復帰か最終保持か） | **[確定負]**（EXE 側に判断材料が無い） | **巻き戻し／リセットを要求する命令は EXE に1件も無い**。`FUN_004106C0` は `BinkNextFrame` を繰り返すだけで、停止判定は「フレーム数到達 + スタック引数（背景は 1）」。**継ぎ目の実現は binkw32 内部**（SDK 依存）＝EXE から決定不能。 |
| **V-557-2** `BinkOpen` フラグ `0x2000` / `0x08000000` の SDK 名 | **[未確定]**（EXE に名称の手掛かりが無い） | 名称は bink.h が必要。ただし**設定条件は [確定(命令)]** で §2.3 のとおり（サイズ閾値／スレッド不一致）。 |

---

## 2. 根拠

### 2.1 V-557-3: `CreditsMenu.bik` の再生経路は無い [確定(命令)]

`CreditsMenu.bik`（リテラル `0xA31AD4`）とその GMST 名 `sCreditsMenuMovie:General`（`0xA31AE4`）について、**`.text` の dword 参照を全走査**:

| 対象 | `.text` dword 参照数 | 内容 |
|---|---|---|
| `CreditsMenu.bik`（値 `0xA31AD4`） | **0** | `.data` の GMST テーブルのみ |
| `sCreditsMenuMovie:General`（名 `0xA31AE4`） | **0** | ランタイムの名前検索も無い |
| スロット `[0xB0309C]` | **1** | `0x9AAB71`（head `0x9A9D44`）＝**静的登録 thunk**（他の4スロット `0xB03084/8C/94/A4` も同型の thunk のみ） |

比較（再生される側）:

| スロット | `.text` 参照 | 実際の再生箇所 |
|---|---|---|
| `[0xB03084]` sIntroSequence | 2 | `0x40EAE2`（スレッド起動）＋ thunk |
| `[0xB0308C]` sMainMenuMovieIntro | 2 | `0x40F0B1`（iv ロゴ同期再生）＋ thunk |
| `[0xB03094]` sMainMenuMovie | 4 | `0x405577` / `0x5B5D00` / `0x5B5F99` ＋ thunk |
| `[0xB030A4]` sOblivionIntro | 2 | `0x66F2C0`（New Game 導入）＋ thunk |
| **`[0xB0309C]` sCreditsMenuMovie** | **1（thunk のみ）** | **再生呼び出しなし** |

→ **`CreditsMenu.bik` は EXE から再生されない [確定(命令)]**。
整合: `menus\options\credits_menu.xml`（942 B・v557 抽出）に `.bik` 参照は無く、`credits_rect`（id 1）は空枠。本文は外部 `Data\Credits.txt`（`v364` §2 [確定]）。

> **残留の可能性（[確定負] で潰せないもの）**: Tile システムの movie trait 経由の再生。ただし該当 trait を XML が持たないこと（上記）と、残り4スロットがすべて直接アドレス参照で実装されていることを合わせて、**実質なしと判定**。

### 2.2 V-557-1: ループ継ぎ目 [確定負]（EXE 判定不能）

`FUN_004106C0`（`docs/Oblivion_decompiled.c` L9416-9462）の該当:

```
_BinkNextFrame_4(*param_1);            ← 進め方のみ
param_1[3] = param_1[3] + 1;           ← ローカルのフレーム数を加算
...
if ((*(uint *)(*param_1 + 8) <= (uint)param_1[3]) && (cVar1 == '\0')) return 0;
return 1;
```

* **先頭へ戻す命令（先頭フレームへのセット／リセット／Rewind）は EXE に1件も無い** [確定負]
* 背景ムービーは描画ループ `0x57F170` が毎フレーム `FUN_00410810(0,1)` を呼び、引数1（＝継続フラグ）で到達後も回し続ける（v557 §2.5）
* → **「継ぎ目が先頭復帰か」を決めるのは binkw32 の内部挙動**であり、**EXE の命令からは決定できない** [確定負]
* 補助: `BinkOpen` のフラグ（§2.3）がループ指定を担う可能性が高いが、フラグの意味名が未確定（V-557-2）なので断定しない。

### 2.3 V-557-2: `BinkOpen` フラグの設定条件 [確定(命令)]（名称は [未確定]）

`FUN_00410160`（L9118-9209）が `_BinkOpen@8(buf, flags)` を合成:

| フラグ | 成立条件（命令実測） |
|---|---|
| **`0x2000`** | ファイルサイズが **`[0xB030BC]` = 26,214,400 B（＝GMST `iPreloadSizeLimit` = 25 MiB）を超える**とき（＋別の条件の AND） |
| **`0x08000000`** | **スレッド不一致時**（`0x983754` で判定）に追加。`SuspendThread`/`ResumeThread`（`ResumeThread` IAT `0xA280F0`）でガード |
| 失敗時 | `"Could not open %s for playback."`（`0xA279xx`） |

→ **名称（SDK 定数名）は EXE に一切の手掛かりが無く、bink.h が必要** [未確定]。値と条件だけが確定。
（`BinkOpen` import は `_BinkOpen@8` のみ、他は `BinkWait/NextFrame/OpenDirectSound/SetSoundSystem/Close/Pause/DoFrame/CopyToBufferRect` の9個＝v557 §2.4 と一致）

---

## 3. 移植への示唆

1. **クレジット動画は原版に無い**（`CreditsMenu.bik` は同梱されるが**再生されない**）。移植の `credits_menu.mp4` は独自演出——**存在するなら削除候補**（U7⑤ の1% 抽選と同根）。
2. ループは移植側の仕様（`setVolume(0f,0f)` と同じく結果として一致）。**「原版はこうしている」ではなく「原版は binkw32 に任せている」**と書くのが正確。
3. BinkOpen フラグを移植（Android側の動画プレイヤ）に移す際、**フラグの SDK 名を根拠にしない**こと（本裁定で [未確定]）。**値と成立条件のみが根拠**。

---

## 4. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-568-1 | `BinkOpen` フラグ `0x2000`/`0x08000000` の SDK 名（bink.h があれば即決） | 低 |
| V-568-2 | `FUN_00410160` の `0x2000` 成立時のもう1つの AND 条件（`0x983754` の返り値の意味） | 低 |
| V-568-3 | クレジット画面が動画枠を用意しているなら、その描画元（XML の `credits_rect` 以外） | 低 |
