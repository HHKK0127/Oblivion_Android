# V363: RaceSexMenu Cancel / main menu Exit の原本準拠裁定 — #130

- 日付: 2026-11-14 / タスク: ai-todo **#130**【RE レーン依頼】原本の RaceSexMenu Cancel/Back 挙動と main menu Exit の正しい遷移の特定
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: #130 本体（`ui_character_creation.cpp` L98-101 の Cancel＝close のみ／タイトル Exit＝Launcher 移行でプロセス存続）＋先行裁定 `v352_race_sex_name_flow.md`（#123）／`v353_chargen_page_ruling.md`（#124）／`b07_vtable_index.md`
- 手法: capstone 5.0.7（v307_common）／RTTI（COL→pTD=+0xC）／PE インポート実パース／**整列 cover-check**（byte-scan の汚い ctx は aligned linear decode で検証）
- 中間証跡: `%TEMP%\opencode\v363_1..v363_20.py/.out`（**全 EXIT=0**。v363_17 はループ変数バグを自己発見・修正後に再実行）
- 制約: git/docs/GT/台帳・session-memory・既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、**ESM 未アクセス・bsa 未展開**、EXE 読み取り専用、L1

---

## §0 判定サマリ

| 質問 | 判定 |
|---|---|
| ② **main menu Exit の正しい遷移** | **[確定] プロセス終了**。Exit ボタン通知（cmd id 7）→ quit flag `[[0xB33398]+0]=1` → WinMain メッセージポンプが検出 → **shutdown cascade → WinMain 戻り値 0 → CRT → ExitProcess(0)**。**ポートの「アプリ内 Launcher へ遷移してプロセス存続」は原本不一致 [確定]** |
| ① **RaceSexMenu Cancel/Back の遷移** | **[部分確定]** EXE の RaceSexMenu 閉鎖経路（queue cb C＝0x5C28F0）は「**フェード閉鎖＋プレイヤー gameplay 復元＋(race 変更時) magic/stats メニュー再作成**」のみで、**MainMenu(0x414) を参照しない [確定(命令)]**＝EXE の RaceSex 側コードはタイトルへ遷移しない。原本が Cancel でタイトルに戻る場合、そのトリガは **EXE 外（ESM 配信スクリプト／テーブル登録系コマンド [推定]）** にあり、**ESM 未読のため遷移の有無は [未確定]（残点 V-363-1）** |

---

## §1 メニュー基盤（#130 で新規確定分）

### 1.1 vtable と slot 意味 [確定(RTTI/命令)]

| vtable | 型 | slots | #130 での override 同定 |
|---|---|---|---|
| 0xA696DC | `.?AVMenu@@`（基底） | 15 | slot0=dtor 0x5853F0 / slot1-5,8-10=**0x60CF60（`ret 8` no-op 通知）** / slot6,7=0x890B90（`ret 0xC`）/ slot11=0x60D0A0（warning no-op・v354）/ slot12=0x6FE080 / slot13=**0x6F7070（`xor eax,eax; ret`＝no-op GetMenuID・v354）** / slot14=0x725DA0 |
| 0xA6D38C | `.?AVRaceSexMenu@@` | 15（9 override） | slot0=dtor 0x5C4E20（本体 0x5C4BF0+deleting flag）/ slot1=**0x5C26D0**（SetElement: id 1,2,0x14,0x15,0xA,0x5A→[this+0x28..0x3C] 格納・v352）/ slot2=0x5C2790・slot8=0x5C3BE0（float=スライダー）/ **slot3=0x5CC200＝通知ハンドラ（本裁定の主対象）** / slot10=0x5C2870（per-frame trait 0xFB7）/ slot11=0x5CE570（per-frame: IsMenuOpen(0x3E9)＋GMST 0xFA1→**0x585190 close**、`[0xB3B5D8]` ガード）/ slot12=0x5C3DD0 / **slot13=0x5C4E10＝`mov eax,0x40C; ret`（GetMenuID）** |
| 0xA6CB1C | `.?AVMainMenu@@` | 15（5 override） | slot0=dtor 0x5B6020（本体 0x5B5870）/ slot1=0x5B5800 / **slot3=0x5B64C0＝通知ディスパッチ（本裁定の主対象）** / slot4=0x5BD9E0 / **slot13=0x5B5950＝`mov eax,0x414; ret`** |

- **menu id 0x416 の正体を RTTI で追加確定**: getter `0x5C0CE0`（`mov eax,0x416; ret`）を含む vtable＝**0xA6D1F4 slot13 → COL 0xAC29C4 → `.?AVQuickKeysMenu@@` [確定(RTTI)]**（v353 の「0x416 疑い」を解消）。検証: 同手法で 0x5C4E10→`.?AVRaceSexMenu@@`、0x5B5950→`.?AVMainMenu@@` を再現し 3/3 突合。

### 1.2 menu id レジストリとヘルパ [確定(命令)]

- **`0x589B70(id)` = FindMenu**: `(id-0x3E9)≤0x32(50)` ゲート → `[0xB13974] + id*4 - 0xFA4`（登録配列）、count＝word `[0xB1397A]`、範囲外/null→`xor eax,eax`。
- **`0x579060` = GetTopMenuId**: MenuManager(`0x582160` シングルトン) の `[+0x1c]`(スタック) `[+0x68]`(トップ) 検査 → トップの trait `0xFAE` を `0x588BD0` で取得 → `fcomp [0xa379b4]` ゲート → `0x57CFA0` へ。呼び出し側は `cmp eax,0x40C/0x414/0x416`（v363_6/8）＝**現在トップの menu id を返す** [確定(呼び出し側)]。
- **`0x5790E0(id, flag)` = IsMenuOpen 系**: 同シーケンスで `push 0x40C; push 0; call 0x5790E0; test al,al` 形（メインループ0x40DAxx で 0x414/0x40C 等を4連判定）。
- **`0x5898F0` = root tile 取得**（Menu→[+0x10] 連結を辿り vfunc+0x0C==0x389 判定→[+0x44] 返却）、**`0x579C10(ctx, cb, phase, a, b, c)` = 遅延 queue**（6引数・cb は E8=0＝queue 専用で実行）、**`0x578D70` = phase 読取**（`[mgr+0xb0]` を読取り後 `0xFF` にクリア。phase 1/2/3 が各 cb の入口条件）。
- **XML 文字列 [確定(文字列)]**: `Data\Menus\CharGen\race_sex_menu.xml`(0xA6973C＝**race_sex_menu は CharGen 直下**)／`Options\options_menu.xml`(0xA698B0)／`Options\credits_menu.xml`(0xA69944)／`Options\load_menu.xml`(0xA698F8)／`Main\magic_menu.xml`(0xA6CA2C)／`Main\stats_menu.xml`(0xA6DBD4)。
- **menu id 対応 [確定(文字列/命令)]**: 0x40C=RaceSex / 0x414=MainMenu / **0x416=QuickKeysMenu** / 0x417=Credits / 0x40E=Load / **0x3F7=OptionsMenu** / **0x3FE=MagicMenu** / **0x3EB=StatsMenu** / 0x3F5=所属 fn 0x5BDE70（FindMenu(0x3F5)）[推定: PauseMenu 系] / 0x3E9=slot11・0x578DA0 が参照 [未確定]。

---

## §2 MainMenu 通知ディスパッチ（slot3 0x5B64C0）[確定(命令)]

```
0x5B64C0: cmp byte [menu+0x4D],0   ← トランジション中 latch（全 case が自己 set）
          jne ignore
          mov eax,[esp+8]           ← 通知 id
          lea ecx,[eax-2]; cmp ecx,5; ja ignore
          jmp [ecx*4 + 0x5B65CC]    ← jump table: id 2..7 の6本
```

| cmd id | case 先 | 内容（証拠） | ボタン [判定] |
|---|---|---|---|
| 2 | 0x5B64E4 | latch=1＋queue cb **0x5B5D60** → `0x465860`（0x310 パスバッファ＋GetTickCount＋FindMenu(0x414)＋callers=セーブ/ロード帯 0x466B31/0x466D5F＝quicksave 帯近接）＋`0x459400`（[game+0x6c] リスト解放）＋`0x5B5960`（**MainMenu 閉鎖**） | **[推定] ロード/開始系**（呼び出し構造のみ・ボタン名は V-363-3） |
| 3 | 0x5B6511 | `0x57DE50(1)`（UI transition サウンド。0x57E090 帯＝`'MenuStart'`/`'MenuEnd'`/`'UIMenuBracket'`→`0x6ADE50`）＋queue cb **0x5B5DF0** → フォーム検査(0x46b250・type 0x3B・`__RTDynamicCast` 0xB05468/0xB0301C)＋**`0x4431F0`（キャラ生成クエスト起動）**＋`0x5B5960`（MainMenu 閉鎖）＋0xB3C0EC=0＋音楽 0x6AB160/0x6AB420＋ログ **'Unable to find character generation quest %s'**(0xA6CB64@0x5B5F0E) | **New Game [確定(文字列)]** |
| 4 | 0x5B6541 | vtable+0x14(base no-op)＋`push 0; call 0x5AEA60` → FindMenu(0x40E)＋`load_menu.xml` 読込（b20 で 0x65DC00→0x5AEA60=Load メニュー開と一致） | **Load [確定(文字列+b20)]** |
| 5 | 0x5B655E | `call 0x5BD680` → FindMenu(**0x3F7**)＋`options_menu.xml`＋**'Options Menu Creation Failed...'(0xA6CFE8)** | **Options [確定(文字列)]** |
| 6 | 0x5B6576 | `call 0x59D680` → FindMenu(**0x417**)＋`credits_menu.xml`＋**'Credits Menu Creation Failed...'(0xA6B820)**＋MainMenu 表示 trait 更新(0x5B5A80→0xFAE/FAF/FB0=1.0) | **Credits [確定(文字列)]** |
| 7 | 0x5B658E | vtable+0x14(no-op)＋latch=1＋queue cb **0x5B59B0** | **Quit/Exit [確定(排除法＋§3 の振る舞い)]** |

- `0x57DE50(n)`＝UI 遷移サウンド dispatcher（`(n-1)≤0x23` jump table 0x57E0C0 → `'MenuEnd'`/`'MenuStart'`/`'UIMenuBracket'`＋0x121 → `0x6ADE50` 再生系）[確定(命令)]。

---

## §3 Exit 経路【判定: プロセス終了・[確定]】

### 3.1 チェーン（全節点実測）

1. **cmd7 case** 0x5B658E：`[menu+0x4D]=1`（latch）＋`0x579C10(ctx=[0xB38C50], cb=0x5B59B0, phase=1, ...)` に queue。
2. **cb `0x5B59B0`（queue 実行・E8=0）**:
   - `FindMenu(0x414)` → root tile `[+0x4D]=1`
   - `phase=0x578D70()`。**phase==1**: メニュー2本（`[mgr+0x68]`/`[mgr+0x1C]` 経由）の `[tile+0x18]` word に `|=1`（closing フラグ）→ **`0x57F170(mgr, 0)`**（遷移処理。`[0xB42CE3]=1`・0x7B7650 等）→ **`mov eax,[0xB33398]; mov byte [eax],1`＝quit flag 設定**。phase≠1: `[tile+0x4D]=0`（latch 解除のみ）。
3. **WinMain メッセージポンプ**（0x40F000 帯・PeekMessageA=IAT 0xA2826C@0x40F0F2・ループ頭 0x40F130）:
   - **0x40F275**: `mov eax,[0xB33398]; cmp byte [eax],0; **jne 0x40F488**`（quit flag 検出）
   - `cmp byte [eax+1],0; je 0x40f130`（＋1＝タイトル復帰要求。§4.3）→ ループ継続
4. **shutdown cascade 0x40F488**：`[obj+3]=0` → `0x43E0F0/0x410B80/0x67F180/0x682430/0x6844D0/0x684710/0x405B00/0x405B40/0x67CF00/0x578EF0/0x5C0FC0` 系ティアダウン → player/game 破棄（`0x4466F0`＋delete、`[0xB33398]` オブジェクト破棄・`[0xB33398]=NULL`）→ `0x747BE0` → **`0x40F637: xor eax,eax … ret 0x10`（WinMain 戻り値 0）**。
5. **CRT → `ExitProcess(0)`**（CRT ラッパ 0x981BA9＝`push [esp+4]; call [0xA280D4]`）。

### 3.2 ExitProcess xref 監査（通常経路の不在証明）[確定]

- ExitProcess（IAT 0xA280D4）の参照は **3件のみ**: ①0x405347＝fatal 経路（MessageBoxA 経由 head 0x4052F0）②**0x494390＝孤立 stub `push 0; call ExitProcess`（E8=0・E9=0・唯一の参照は rdata 0xA3DA08＝'StackTrace.txt'/'ProgrammerOutput.txt' 帯のエラーハンドラ表で 0x49465A/0x4946C8 から参照＝fatal/例外系）** ③0x981BA9＝CRT。
- **PostQuitMessage は未インポート**（PE 実パース230件に不在）→ 通常の quit は「フラグ→ループ離脱→CRT 経由」以外に経路が存在しない。
- 第2経路: queue cb **0x5BDDE0（所属=0x3F5 系メニュー）**: `phase==2` → `[0xB33398]+1=1`（**タイトル復帰要求**）＋`0x440AF0(game,1,0,0)`／`phase==3` → closing フラグ＋`0x57F170`＋**quit flag=1**（＝段階的な Quit 系 UI）[確定(命令)]。

### 3.3 ポート判定

- 原本 Exit ＝ **プロセス終了（ExitProcess(0)）[確定]**。
- ポートの「Title screen Quit - returning to launcher（アプリ内 Launcher へ遷移・プロセス存続）」＝ **原本不一致 [確定]**。原本準拠なら quit flag 相当 → ループ離脱 → **プロセス（Activity）終了**が正。

---

## §4 RaceSexMenu Cancel 経路【判定: [部分確定]】

### 4.1 slot3 0x5CC200 の通知ディスパッチ [確定(命令)]

`ebx = arg1`（通知 id）。構造（v363_7/8 全文）:

- **id == 0x5A (90)**: 名前検証ブロック（GMST 0xFA8/0xFAE/0xFAF/0xFB3/0xFD0・`0xFDE` 名前文字列取得 `0x588C10`・スペース切除ループ・先頭文字 vowel(aeiouAEIOU) 判定→`[0xB38660]`/`[0xB38668]` 選択→`'%s %s?'`(0xA6B4D8) 整形）→ **name≠default 分岐で queue cb C**、default 分岐は 0x5CCBB7（cb=ebx 設定部が未解読＝V-363-4）。
- **id == 0xA (10)**: `0x5C2730`（editor 設置）＋`0x5C30C0`（name 書出・v352）。
- **id == 0x62 (98)**: `0x526F00`（呼び出し1件のみ）＋`0x5C91E0`。
- **残り**: trait 比較チェーン（`0x5C3440`=要素取得・`0x588BD0`=GetFloat・`fild arg` と fucom）→ latch `0xB3B4C8`/`0xB3B4C9` で **queue cb A / cb B** を再入防止付きで登録、以降は要素別アクション直呼び（0x5C7070/0x5C2F20/0x5C62D0/0x5C9770/0x5C9650/0x5C5C30）。

3つの queue cb（**いずれも E8=0＝queue 経由のみ**）:

| cb | アクション本体 | 内容 [判定] |
|---|---|---|
| **A** 0x5C9FC0→`0x5C9CD0` | `0x5273A0`（先頭に 0x43ACE0/0x43EB30 のコールバック登録＝コミット系大関数）＋`0x5C91E0`＋trait `0xFB1` 更新 | **[推定] Done/適用系**（UI 更新のみ・menu 閉鎖なし・gameplay 復元なし） |
| **B** 0x5C9F70→`0x5C9C70` | `0x521E40`（vtable+0x128 arg0x45・[esi+0x168]/[0x108] リスト操作）＋`0x5C91E0`＋**`0x5C7BA0`**（player+0xE8+0xA8 と **[0xB33A98]+0x3C** の2リストを `strcmp(0xA549D8)` で走査・`[menu+0x87C]=0`・要素ごと SetString） | **[推定] 元に戻す/revert**（menu 閉鎖なし） |
| **C** 0x5C2BA0→**`0x5C28F0`** | 下記 4.2 | ****閉鎖＋復元 [確定(命令)]**** |

### 4.2 cb C `0x5C28F0`＝RaceSexMenu 閉鎖＋gameplay 復元 [確定(命令)]

1. `FindMenu(0x40C)` → null なら終了。root tile 取得。
2. trait `0x1772 = [0xa379b4]`（0.0f）→ **`0x584740`＝menu フェード閉鎖**（[tile+0x24]=2 へ遷移、47 callers の汎用 close）。
3. `[tile+0x864]` 非0 → `0x5FAEA0(player, ..., 1, 0, [tile+0x864])` → **`[0xB3B5D8]=1`**（読手＝slot11 0x5CE570 ほか 0x5C2C18/0x5C5065・v363_15/20）。
4. **プレイヤー gameplay 復元**: `g_thePlayer` vfunc+0x170 → `0x5E3320` と `[0xB37D00]` の比較で分岐:
   - **race 変更時（非等値）**: リスト revert（vfunc+0x2E0/+0x2DC 呼出）→ `0x6A2280(player+0x68, 0.0)`（カメラ）→ `[player+0x184]=0`・`[player+0x1DC]=0` → `0x65FBB0`/`0x65D610` → `0x66C200` → `0x57A7D0`（×2・0x3EB 検査）→ `0x5F2530/0x5F1910(9)/0x5F25F0`（アニメ）→ `0x6645C0` → **`0x5B3760`＝MagicMenu(0x3FE) 再作成**（FindMenu→deleting dtor(1)→`magic_menu.xml` 読込→root id==0x3FE 検証）→ **`0x5DCB70`＝StatsMenu(0x3EB) 再作成**（同型・`stats_menu.xml`）→ `0x522760/0x6626E0/0x664700(0)/0x6670C0/0x66C580`。
   - **等値時**: `0x5222D0(player, 1)` のみ → 共通尾（`0x57A7D0` 以降）。
5. **この経路のどこにも MainMenu(0x414) の open は存在しない。**

### 4.3 タイトル復帰は「別系統」である [確定(命令)]／EXE RaceSex から到達不能

- **RaceSex コード域 0x5C0000..0x5D2000 の menu id `0x414` 即時数＝0件**（v363_10(6a)）。E8 target 監査（slot3/cbA/cbB/cbC/0x5C91E0/0x5C7BA0 の全範囲・menu 帯 0x570000..0x59F000）でも ShowMainMenu 系なし（0x57A7D0=0x3EB/0x5B3760=0x3FE/0x5DCB70=0x3EB のみ）。
- **タイトル復帰機構は WinMain 側に別途存在**（0x40F3D8..0x40F471）: player 復元（0x405CE0/0x5E9690/0x663340/`[player+0x58]` vfunc+0x3A8）→ **[0xB33B00] game セッション破棄**（0x462080/0x459870/0x45A530(0)/0x45C320）→ **`0xB3BD00` リセット（0x675310）** → `[0xB33A98]` 0x447300 → `0x57CCC0(0)` → **`0x5B5AC0` 音楽切替（'Data\Music\' 帯・`[0xB3C0EC]=1`・0x6AB160(8)）** → **`0x5B6040`＝MainMenu 再構築**（FindMenu(0x414)→存在時は deleting dtor→`0x57CC00`/`0x579770`）→ `+1/+4` クリア → **ループ継続（プロセス存続のままタイトルへ）**。入口＝§3.1-3 の **`+1` flag**。
- **`+1=1` 書手**: `0x662ED0`/`0x66328F`/`0x6738C4`（3本とも **E8=0・E9=0＝ポインタ/テーブル登録 [推定: スクリプトコマンド系]**）＋`0x5BDDE0` phase2（§3.2）。
- **RaceSex コードから +1 書手への E8 到達＝0件 [確定(命令)]**（v363_20）。
- → **EXE 単体で見る限り、RaceSexMenu の閉鎖（Cancel 相当）は「フェード閉鎖＋gameplay 復元」で終わり、タイトルへ遷移しない [確定]**。原本が Cancel でタイトルへ戻るなら、そのトリガは（ア）ESM 配信のキャラ生成クエストスクリプト（menu close 検知→+1 書手系コマンド or メニュー再生成）か（イ）ポインタ登録系コマンド [推定]。**ESM 未読・bsa 未展開のため断定は [未確定]（V-363-1/2）。**

### 4.4 ポート判定

- ポートの「`hitTestCancelButton → close()` のみでタイトルに戻らず游戏中に残留」:
  - **「EXE の RaceSex 閉鎖（cb C）と対応」なら整合 [部分確定]**（cb C もタイトルを開かない）。
  - ただし cb C は **fade 閉鎖＋camera/HUD/player フラグ復元＋(race 変更時) magic/stats 再作成** まで行う。ポートの close() がこの復元群を伴うかは移植側調査課題（#130 は材料供給が目的）。
  - **「原本はタイトルに戻る」という想定の正否は [未確定]**（ESM/bsa 依存・V-363-1/2）。→ main レーンは #101/#136 等と同じ人間ゲート（ESM 抽出物の確認）で裁定可能。

---

## §4.5 追補（V-363-2 縮小: Cancel ボタンの通知 id）— probe v363_21/22

bsa 抽出済み `menus\chargen\race_sex_menu.xml`（705行）と EXE で以下を確定:

1. **Cancel/Back は独立ボタンではなく `race_button_done`(id=90) の表示モード切替 [確定(XML)]**:
   - `<image name="race_button_done"> <include src="button_long.xml"/> <id> 90 </id>` … `<user5> &false; </user5> <!-- 1 (false) for Done and 2 (true) for Back -->`
   - ラベル: `_text_1 = strings() _done = 'Done'`、`_text_2 = strings() _return = 'Return'`（strings.xml 実測: `<_done> Done </_done>`／`<_return> Return </_return>`）→ **Back ラベルは 'Return' [確定(XML)]**
   - 他 id の実測: 1=race_window／2=race_player_grab／10=race_name（editor・v352 と一致）／20=race_scroll_bar／90=race_button_done／100=race_pane／101=race_template_button_layout／102=toggle／103=slider／104=？
2. **trait 対応の裏付け**: EXE は **Done ボタン `[this+0x3C]`（v352 確定）の trait `0xFB3`（＝user5）へ 1.0 を書く**（`0x5CC81F: fld1; push 0xfb3; call 0x58CEB0`・id90 処理内）[確定(命令)]。→ **user0=0xFAE・userN=0xFAE+N の対応を再確認**（user5=0xFB3・user9=0xFB7＝v364 §3.3 の対応表と整合＝**V-364-4 の根拠を補強**）。
3. **通知経路**: Back 表示時も通知は **id=90 の1本**で slot3 `0x5CC200`（`cmp ebx,0x5A`）に入る → **Cancel は「id90 共通経路のうち user5=2 モード分岐 → cb C（0x5C28F0 閉鎖+復元）到達」という構造が最も整合 [部分確定]**。
4. **未確定の残**: user5=**2.0（Back）を書く EXE site** と **user5==2 の判定箇所**が未特定（`push 0xFB3` 全数=199件/RaceSex band 98件を走査し、value=1.0/0.0 書出は確認・2.0 書出 site 未発見）。open 時初期設定 fn `0x5CA010`（race_sex_menu.xml 読込・21 pane 生成・16回の `0x402E30` パス構築・`0x5C4340` pane id 設定）にも `push 0xFB3` は不在 → **初期値は XML の `&false;`、表示確定は実行時 bookkeeping [部分確定]**。→ **V-363-2 は「通知 id=90 と Cancel の構造」で[確定]、mode 切替書手のみ [未確定]＝深刻度を中→低へ降格**。
5. **ポートへの示唆**: 原本では Cancel も Done と同一要素（id90）の別ラベル表示。移植側が独立ボタン（hitTestCancelButton）を使う場合でも、**通知系を統合して扱うのが原本構造と整合** [材料]。

（注: v363_21 の (b) 走査は pat のバイト列ミス（0xFB3≠0x000000B3）で0件＝無効、v363_22 で正しい `68 B3 0F 00 00` に是正して再実行＝**byte-scan の pat 検算教訓を追加**）

---

## §5 検証・自己批評

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | **v363_1..v363_20 全 EXIT=0**。v363_17 のループ変数バグ（`j` 未定義）を実行前に自己発見・修正。整列 cover-check を全 ctx に適用（v362 の教訓継承） |
| 2 | RTTI 突合 | getter→vtable→COL→TD の手法を RaceSex/MainMenu/QuickKeys の3件で実施、**3/3 が b07 索引と一致**（0x416=QuickKeysMenu は新規） |
| 3 | quit 経路の連続性 | flag 設定（0x5B5A13）→読手（0x40F27A）→cascade（0x40F488）→`ret 0x10`（0x40F637）→CRT（0x981BA9）を**全節点実デコード**で連結。中継なし・推測節なし |
| 4 | 不在証在証拠の扱い | 0x414 参照0件・+1 書手到達0件はいずれも**対象範囲を明示**（0x5C0000..0x5D2000／全 E8）してから主張。0件 census を「不在証拠」とするのは境界検証済の範囲に限定（v311/v319 教訓） |
| 5 | find_head 誤答 | 0x57DE50（head 0x57DDE0）・0x40D4D0・0x5C5065（cover 不可）で再発。**直アドレス aligned decode に切り替えて解消**。cover 不可は [未確定] として保持 |
| 6 | 残りの限界 | bsa（ボタン id 定義）・ESM（クエストスクリプト）に依存する点を §4.4 と残点に明記、推測で [確定] にしない |

---

## §6 残点（本裁定で新規）

| ID | 内容 | 深刻度 |
|---|---|---|
| V-363-1 | [未確定] 原本 RaceSexMenu Cancel がタイトルへ遷移するか（＋1 書手 0x662ED0/0x66328F/0x6738C4 の呼出し元＝ポインタ/テーブル/ESM クエスト） | **中（#130① の主眼）** |
| V-363-2 | [未確定] `race_sex_menu.xml`（bsa）の Cancel/Back ボタンの通知 id と slot3 チェーン対応 | 中 |
| V-363-3 | [推定] MainMenu cmd2 のボタン名（0x465860＝ロード/開始系の切り分け。cmd3=New/4=Load/5=Options/6=Credits/7=Quit は確定済） | 低 |
| V-363-4 | [未解読] slot3 の name==default 分岐 0x5CCBB7 queue（cb=ebx の設定部 0x5CCA77..0x5CCADF が cover 不能） | 低 |
| V-363-5 | [推定] menu 0x3F5 と cb 0x5BDDE0 の所属（PauseMenu 疑い）・0x3E9 の正体 | 低 |

---

## §7 main への報告

1. **【② Exit＝[確定]】原本 main menu Exit はプロセス終了**。経路: cmd7 → cb 0x5B59B0 → `[[0xB33398]+0]=1` → WinMain 0x40F27A → shutdown cascade → WinMain 戻り値 0 → CRT → ExitProcess(0)。**ポートの Launcher 移行・プロセス存続は原本不一致 [確定]** → 修正時は「quit flag 相当→ループ離脱→Activity 終了」が正。
2. **【① Cancel＝[部分確定]】EXE の RaceSexMenu 閉鎖経路（cb C 0x5C28F0）は fade 閉鎖＋gameplay 復元（camera/HUD/player フラグ・必要なら magic/stats 再作成）までで、MainMenu を開かない [確定(命令)]**（0x414 即時数0件・+1 書手到達0件）。タイトル復帰は WinMain の `+1`→teardown→`0x5B6040` 再構築という**別系統**であり、その `+1` は RaceSex コードから到達不能。→ **「原本はタイトルに戻る」想定の正否は ESM/bsa 依存で [未確定]（V-363-1/2）**。人間ゲート（ESM 抽出物の chargen クエスト確認）に送る。
3. **付随確定**: menu id 0x416=QuickKeysMenu（RTTI）／0x3FE=Magic・0x3EB=Stats（XML）／0x3F7=Options・0x417=Credits・0x40E=Load（XML＋エラー文字列）／MainMenu cmd 2..7 の jump table ディスパッチ構造と latch[+0x4D]／quit flag とタイトル復帰 `+1` flag の2系統／WinMain shutdown/teardown の両 cascade 全文。
4. docs/GT 反映候補（ExitProcess 経路・menu id 表・quit/+1 二系統）は #47/#20 一括（L1）。

---

### 参照（本タスクの証跡）

- v363_1（menu 文字列探索・ExitProcess/PostQuitMessage IAT 監査・'Exit' 文字列＝クエスト topic 誤爆の排除）
- v363_2（RTTI *Menu* クラス列挙・'Exit' blob＝BarterExit 等の誤爆確定・quit stub 0x494390 E8=0）
- v363_3（vtable 3本解決＝Menu/RaceSexMenu/MainMenu・slot 差分・0x494390 rdata 参照）
- v363_4（RaceSex override 9個の初 decode・slot13=0x40C getter）
- v363_5（MainMenu override 5個・slot3 0x5B64C0 初見・slot4 0x5BD9E0）
- v363_6（MainMenu jump table 0x5B65CC 全6本・case4/5/6/7 本文・0x57DE50 初decode・menu id 即時数走査）
- v363_7（slot3 0x5CC200 全文 1425 命令・helper 3本実体・menu id 0x414/0x40C 参照ペア走査・0x589B70=FindMenu 実体）
- v363_8（XML ラベル6本・slot3 の cmp/call 抽出・queue cb A/B/C 実体・0xB3C0EC 読手・0x494390=エラーハンドラ表）
- v363_9（rdata 0xA3DA08 監査・0x585C10/0x587B20/0x578D70/0x584740/0x5C7BA0 実体・vtable→RTTI 3件突合・[menu+0x4D] 走査・0x6AA170 読手）
- v363_10（cmd7 cb 全文・0x57F170/0x465860/0x4431F0 初decode・main loop 0x40D4D0 帯・RaceSex 域 0x414=0件）
- v363_11（PeekMessageA 4 site・[0xB33398] 296 refs・cb C 0x5C28F0 全文・queue 站点の cb 同定・0x5B64C0 入口＋cmd2/3 本文）
- v363_12（BSS 文字列ポインタ走査・quit flag 読手12本・ラベル文字列 magic/stats XML・0x57F170 呼出し3本・0x43ACE0/0x43EB30 実体）
- v363_13（**quit flag=byte0／WinMain 0x40F275 読手**・race_sex_menu.xml=CharGen 直下・0x465860/0x4431F0/0x5C91E0 初decode）
- v363_14（**WinMain shutdown cascade→ret 0x10 を完全取得**・chain3 strcmp 前提・cb A tail・0x579060 全文）
- v363_15（0xB3B5D8 読手4件・BSS 初期化形探索・0x57D2F0=flag getter・chain 入口）
- v363_16（MainMenu 表示/非表示ヘルパの呼出し元・0x579060=GetTopMenuId 全文・queue cb E8=0 確認）
- v363_17（**RaceSex ハンドラ→menu 帯 E8 target 全監査**・0x59C9D0=CreditsMenu dtor（rdata 0xA6B784）・0x5B6040 callers=WinMain×2）
- v363_18（**teardown 前半全文（game 破棄＋MainMenu 再構築 0x5B6040）**・0x580120 mega-fn 同定）
- v363_19（WinMain gap 0x40F290..0x40F3D8＝in-game frame＋branch 監査・0x5C5065 head cover 不可）
- v363_20（**+1 書手3本の E8=0 確認・0x5C91E0 callers=4 監査**）
