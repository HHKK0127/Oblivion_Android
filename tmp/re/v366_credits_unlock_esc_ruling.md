# v366 — Credits 解除入力とタイトルでの ESC 挙動の裁定（#135）

- 採番: **v366**（#135 専用、probe prefix v366_1..v366_10 ＋1行 shell 読取2回）
- 対象: Oblivion.exe v1.2.0416（ImageBase 0x400000）
- 依頼: ai-todo #135（#132 の仕様裁定材料。①原本 CreditsMenu の解除トリガー ②タイトルメニューでの ESC の原本挙動）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令か原本 XML/実データのみ。

---

## §1 裁定サマリ

### ① CreditsMenu の解除トリガー — **[部分確定]**
| 要素 | 判定 | 根拠 |
|---|---|---|
| 閉鎖関数 | **`0x59C9F0` のみ [確定]** | E8 caller = **2本だけ**: slot11 `0x59CAAB`（per-frame 条件）と slot14 `0x59CB5E`（通知）。実体 = `FindMenu(0x417)`→`SetFloat(0x1772)`→**フェード閉鎖 `0x584740`**→`FindMenu(0x414)`＋ログ |
| 解除入力（slot11 条件） | **`0x4031E0(mgr,0,1)!=0` OR `0x403520(mgr,0x1D,1)!=0` [確定(命令)]** | `jne→0x59CAAB` の2本。**3条件目 `0x6DA150(9)==2` は死枝 [確定]** — `0x6DA150 = mov eax,1; ret 4`（恒1スタブ） |
| input id 0（条件1） | マウス/入力0系クエリ **[推定高]** | `0x4031E0` は id0/1 を `[mgr+0x1B2C/+0x1B40]` の0x80テスト、**id8/9 は `[mgr+0x1B28]` カウンタ（±＝ホイール）[推定]**、mode0..4=押下/エッジ分岐（mode4 は2配列 XOR で変化検出） |
| command 0x1D（条件2） | **グローバル入力コマンド。credits と pump の両方が照会 [確定]** | 詳細は §2.4。**ESC単独の credits 経路は存在しない [確定負]**（close caller が2本のみ） |
| slot14（通知） | 通知 id（-1/1/2/3/4）スイッチ→`0x59C9F0` **[部分確定]** | 分岐方向（どの id で close）は未完全 decode |
| 解除入力の main menu への通過（no_click_past） | **[未確定残点 V-366-4]** | `&no_click_past;` の DTD 定義は抽出物に不在 [確定負]。**stackingtype=trait 0xFA5 [確定]**（registry `push 0xfa5; push "stackingtype"; call 0x58A0A0`）→ **読取5箇所が値 {102,103,6006,6007}（＋0.0）で分岐し `SetFloat(0xFAB)`/`vfunc+0x34` 等へ到達 [確定(命令)]** — 値↔マクロ対応と通過/保持の意味が未特定 |

### ② タイトルメニューでの ESC — **[部分確定]（Exit 確認/即終了は該当経路なし [確定(否定側)]）**
- **EXE に「ESC→Exit 確認/終了」の経路は存在しない [確定(否定側)]**。Exit は MainMenu の cmd7（ボタン）のみ（#130 で確定）。
- **グローバル入力 pump**（`0x583690..0x58387C`）は command を照会して以下を実行 [確定(命令)]:
  - **command 0x1A → `0x57ACC0([0xB387A0],0,1,1.0)`**
  - **command 0x1B → ガード（menu count==0 OR `[*0xB13974 + 0xAC]==0`）通過時のみ `0x57ACC0([0xB387A8],0,1,1.0)`** — ガード不成立なら **ESC/当該キーは無視される [確定]**
  - **command 0x1D → メニュー群 0x414/0x3EF/0x40F/0x40E/0x3F5 への操作チェーン（`0x587B20(id,1)→0x5878B0`）＋`0x57B560`**（＝Back/メニュー系 [推定]）
  - **command 0x1E → `GetTopMenuId()==0x414` なら skip ＋ close 系（`0x57D640(3)`/`0x57CFE0(3,0)`）** — **0x414（＝MainMenu [推定高]）は Back 系で閉じられない [確定(命令)]**
- **`[0xB387A8] = 'sCantQuickLoad'（"You can't Quickload while the game is paused."）／`[0xB387A0]` = 直前登録の隣接メッセージ（sCantQuickSave 系 [推定]）／`[0xB387B0]='sFileNotFound'` [確定(登録)]** → **command 0x1A/0x1B = QuickSave/QuickLoad [推定高]** → **`0x403520` の id 空間は VK ではなく control-command [推定高]（V-366-1）**。
- **`0x57ACC0` = menu 0x3F2（`Data\Menus\Main\hud_subtitle_menu.xml` [確定]）のインスタンス確保 → `0x5A9980`（mode==2 → `0x5A95C0(menu, msgObj, 1.0, 0, 0)`、mode≠2 → `SetString(0xFDE)`＋`SetFloat(0xFA1/0xFAF)`）へ委譲 [確定(命令)]** — HUD subtitle/toast 表示系 [推定]（成否ゲート `[x+0x1C]`/`[x+0x68]` の意味は V-366-3）。
- **結論**: ESC キーが Back 系（command 0x1E/0x1D）に束縛されていれば **タイトル（top==0x414）では何もしない [推定高]**。**現ポートの Activity「Exit Oblivion?」確認ダイアログは原本 EXE に該当経路なし＝原本不一致の可能性高い [部分確定]**（束縛の INI 初期値未特定が残留）。

---

## §2 証拠

### 2.1 閉鎖と条件（probe v366_1）
```
0x59C9F0: push 0x417; call 0x589B70(FindMenu) → SetFloat(0x1772) → call 0x584740(フェード閉鎖)
          → push 0x414; call 0x589B70 + ログ(0x9832E6, 5引数)     [E8 callers = 2]
slot11 0x59CA70:
  59CA85: call 0x4031E0(mgr, 0, 1) ; test; jne 0x59caab  (条件1)
  59CA94: call 0x403520(mgr, 0x1d, 1); test; jne 0x59caab (条件2)
  59CAA1: call 0x6da150(mgr, 9)    ; cmp eax,2; jne skip  (条件3 → 恒1で死枝)
  59CAAB: call 0x59c9f0            ; ★CLOSE
slot14 0x59CB30: cmp eax {-1,2,4,3,1} → 0x59CB5E call 0x59c9f0  [通知 close]
0x6DA150: mov eax, 1 ; ret 4        ★恒1スタブ＝条件3は到達不能 [確定]
```
- `0x59CB70` 以降の `cmp dl,0xd/0x3c/0x3e/0x2a`（CR/'<'/'>'/'*'）は v364 確定の**行パーサ本体**（slot14 と別関数で矛盾なし）。

### 2.2 入力クエリ3空間（v366_1/2）
- **`0x4031E0(mgr, id, mode)`**（input 空間）: id0/1→`[mgr+0x1B2C/+0x1B40]+{0,1}` の `test …,0x80`、**id8/9→`[mgr+0x1B28]` の符号付きカウンタ（ホイール [推定]）**、mode0..4→押下/エッジ（mode4 は2配列 XOR の符号判定＝変化検出）。
- **`0x403520(mgr, id, mode)`**（command 空間）: **id<0x1D → binding テーブル `[mgr+0x1B7E + id + row*0x1D]`（3 row×29 列、0xFF=未割当）を `0x403490`（→`0x4031E0`/`0x4032D0`）で解決**、**id 0x1D/0x1E/0x1F → 直結0x80フラグ `[mgr+0x18F5/0x191D/0x19AB]`（各 +0x100 に2重コピー）**、その他→0。
- **0x403520 の E8 caller = 30**（pump 0x583696/0x5836DA/0x583735/0x583782/0x583844…・credits 0x59CA94・RaceSex 帯 0x5C1FAE..0x5C2079 連番11本等）。

### 2.3 pump のコマンド処理（v366_2/5）
（§1② の通り。加えて `0x40D9FC..0x40DAC9` のメインループは `0x5790E0(x, id)`（IsMenuOpen）で 0x40C/0x414/0x3F3/0x3Exx を連鎖確認 [確定]。`0x474326` は 0x40C 開時に VK 0x20/0x21（Space/PageUp）判定＝**別系統の VK 使用例**。）

### 2.4 command 0x1D/0x1E と静的メッセージ（v366_9/10）
- 静的初期化スロット連番: **`[0xB387A0]`=（sCantQuickSave 系 [推定]）／`[0xB387A8]`='sCantQuickLoad'／`[0xB387B0]`='sFileNotFound'**（`0x419A70(&obj, name, text)` 登録＋`mov ecx,&obj; jmp 0x4055E0` thunk で確認）。
- **→ pump の command 0x1A/0x1B は QuickSave/QuickLoad コマンド [推定高]**（表示＝`0x57ACC0`）。

### 2.5 stackingtype（v366_1/3 ＋1行読取）
- **`stackingtype ↔ trait 0xFA5 [確定]**（`0x58AC3D: push 0xfa5; push 0xa6a3e0("stackingtype"); call 0x58A0A0`＝XML 名→trait レジスタ）。
- 読取5箇所（`0x57D288`/`0x584893`/`0x58FBCB`/`0x591CB6`/`0x5938FB`/`0x59544A` 等）: `GetFloat(0xFA5)` を **{102.0, 103.0, 6006.0, 6007.0}（＋0.0）** と逐次比較して分岐 → `SetFloat(0xFAB,…)`／`vfunc+0x34`／`0x58B220→0x58CA00` 等 [確定(命令)]。`0x57D288` は int 化して `cmp eax,0x66(102)`/`0x1776(6006)`。
- **`&no_click_past;` の DTD 定義は抽出物に不在 [確定負] → 値↔マクロ対応は未特定（V-366-4）**。

### 2.6 menu id（v366_4/5/7）
- **0x3F2 = `Data\Menus\Main\hud_subtitle_menu.xml` [確定]**（`0x5A8E30` 内 `push 0xa69a00→0x590420(ローダ)`＋エラー文字列 `'HUD-Subtitle Menu Creation Failed... Are your menu and art r…'`）。
- **0x414 = MainMenu [推定高]**（Back 系 skip ガード・credits 閉鎖時の参照・`0x580CB6` の open チェーン先頭）。
- 0x408/0x40C は FindMenu 連続（0x40C=RaceSex 系 [推定]、0x408=v365 の Stats 系）＝正式名は残点。

---

## §3 残点 **V-366-1..5**
1. **id 空間の確定と ESC の束縛**: `0x403520` の id＝control-command [推定高]だが、binding テーブル初期化元（INI [Controls] 読取）が未特定 → **ESC キーのデフォルト束縛先 command＝②の最終確定に必須【中】**。
2. **command 0x1D/0x1E の正式名**（Back? Menu?）と pump 内 `0x587B20(id,1)→0x5878B0` チェーンの分岐方向・`0x57B560`/`0x5BDcd0` の正体【中】。
3. **`0x57ACC0→0x5A95C0` の最終効果**（ゲート `[x+0x1C]`/`[x+0x68]` の意味＝表示系か実行系か）【中】。
4. **stackingtype 値↔マクロ対応（102/103/6006/6007 の各意味・click-through の実装箇所）**【中・①の後半】。
5. **menu id 0x408/0x40C 正式名と slot14 の close 分岐 id**【低】。

## §4 検証・自己批評
- probe: **v366_1..v366_10 全 EXIT=0**（＋1行 shell 読取2回: stackingtype の id push、stackingtype 定数）。
- **教訓**: (i) **引数スタックの再解釈** — `push 0x417; call 0x589B70`（1引数）と `push×5; call 0x589B70` の矛盾は、直前の4pushが**後続 `0x9832E6`（5引数）の引数を先取り**していたため＝呼び出し境界の再読が必須。(ii) **id 空間3種（input/command/VK）の混同罠** — 'sCantQuickLoad' 対応で command 空間と判明し VK 読みが反転した。(iii) **文字列読取の決定力** — 'HUD-Subtitle Menu Creation Failed...'→menu 0x3F2 同定、'sCantQuickLoad'→command 0x1B 同定。
- docs との不整合: なし（新規裁定。#132 の移植修正「back→解除」は **command 0x1D 経由の解除と整合 [推定]**）。
- L1 制約: docs/GT/ESM 変更なし。編集は tmp/re/・STATE・台帳・memory のみ。
