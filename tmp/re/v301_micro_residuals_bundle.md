# v301_micro_residuals_bundle.md — serial 284（残留微細束・3件）

- 作成日: 2026-11-05 / 担当: 本セッション内実施
- 出典: tmp/re/v300_ini_three_layer_bundle.md 残点1..2 ＋ tmp/re/v297_changeflags_ledger_bundle.md 残点3（Review で検出した残解析 3 件を1束化・新ルール4の並行対応）
- 中間物: `%TEMP%\opencode\v284_i1..i8.py/_out`（8 probe・全 EXIT=0）
- 判定サマリ: **件1=確定（書手実証＋slot0 負確定）/ 件2=確定（文言＋sink）/ 件3=確定（fstart 誤検出の訂正）**

## 件1 [0xB39578] 配列の登録書手 — 【確定】

### 構造の実証（probe #1/#5/#7）

- **v300 §件2 の「登録順 companion リスト」呼称は配列へ訂正**（V284 行記載の通り）: teardown は `xor edi,edi → mov esi,[edi+0xB39578] → ... → add edi,4 → cmp edi,0x3b8 → jb`（0x40C534..0x40C587）＝**238 スロット（0x3B8/4）のポインタ配列を index 反復**。読手 `0x59BE5B cmp bl,0xee; jae`（238 上限）・`0x57C2A6 [ecx*4+0xB39578]` と完全整合。
- 姉妹配列: base **0xB39554**（teardown 2nd loop @0x40C590・slots 0xB39554..0xB39574 書手 0x9F55xx 帯・読手 0x57C275/0x59BE8A）＝2 系統並立（v284_i2 (d)）。末尾 slots 0xB3992C/0xB39930（書手 0x9F549A/0x9F575B・読手 0x40C5F2/0x57C2D6）まで配列境界閉じる。

### 書手（本件の核心）— probe #7 (a) 命令実測

**CRT 静的初期化帯 0x9F34xx..0x9F77xx が書手**。各登録関数は同型反復:

```
push 8; call 0x401F00(new) → if(ptr) { push <str>; push <str>; mov ecx,ptr; call 0x419A70(登録) }
→ mov dword [0xB3957C/80/84..0xB3992C], eax   ; 返却ノードをスロットへ逐次 store
→ (次スロットへ)
```

- スクロット書込 imm32 の実在: 0xB3957C→0x9F345A / 0xB39580→0x9F3492（stride≈0x38/件）／0xB39554→0x9F552A / 0xB39558..74→0x9F5563..0x9F56E5 ／0xB3992C→0x9F549A・0xB39930→0x9F575B。**全スロットの書手が 0x9F34xx-0x9F77xx 静的帯に閉じる**。
- 0xB39000 系の 11 参照（probe #6）は `mov eax,[0xB39000]`＝**ポインタ読み出し側**で静的帯の登録データ。`[esi+0x578]` disp32 書手10件（probe #5）は 0x66xxxx 保存帯の **SLG 隣接オブジェクトの別フィールド**（`mov ecx,[0xB33B00]; cmp [ecx+0x7c],0x42` 文脈・配列とは無関係）と否定。

### slot0（0xB39578）の裁定 — 【確定（負）】

- 全 .text の imm32 `78 95 B3 00` は**3 件のみ＝読手3件**（0x40C536 teardown／0x57C2A6／0x59BE63）。**静的 writer も disp32 0x578 writer も SIB 書手も存在しない**（probe #3/#5/#8 で三重否定）。
- **裁定: slot0 は一切書かれず .bss ゼロ維持＝予約/未使用スロット** [確定(負)]。設計整合: 読手は全て `test eax,eax; je no-entry` ガード付きで index0 → 恒に null 経路（ID 空間の 0=none センチネル[推定]）。

## 件2 0x4A7A60 ログの文言と Warnings.txt sink — 【確定】

- **文言（probe #1b 実測）**:
  - 0xA2FAB0 = `"Setting key '%s' already used in list.\nSetting keys must be unique.\n"`（v300 件1 の名リスト重複警告の実文言語）
  - 0xA32490 = `"Setting key '%s' already used in map.\nSetting keys must be unique.\n"`（0x419A70 値 store 重複警告）
  - 0xA2F864 = `'%s'`（配列読手側フォーマット断片）
- **sink 経路**: 0x4A7A60 = 0x4941A0 への薄ラッパ（probe #2: push 1 + fmt + va → `call 0x4941A0`）。**0x4941A0** は `mov eax,0x32CC`（13KB スタック）→ `call 0x983BE0`(_chkstk) → `cmp [0xB34D90],0`＝**v297 確定の警告ロガー単例** → `call 0x982820`(整形)。関数体内 **0x4944BC に 'Warnings.txt'@0xA3D9BC の ptr-ref**（probe #8b）＝**file sink 接続を確定**（エントリ 0x4941A0 と同一関数スパン 0x4941A0..0x4944F0）。

## 件3 0x463913 到達元 — 【確定（v297 訂正）】

- **fstart() ヒューリスティックの誤検出**を命令レベルで証明（probe #4）: `0x463912 = 33 CC` は **`xor ecx,esp`（__security_check_cookie 0x9811E2 の直前スタック整合チェック）**であり int3 padding ではない。0xCC バイトを境界と誤認した。
- 実構造: 前関数の GS epilogue `xor ecx,esp / call 0x9811E2 / add esp,0x138 / ret 4`（0x463912..0x46391F）で終了 → **新関数は 0x463922**（`mov eax,[ebp]` 開始）で、そこへは **`je 0x46388e` → 0x463922 の関数内ジャンプ1本のみ**（E8/E9/imm32 参照は全セクションでゼロ＝probe #3c 合わせ）。
- **実エントリ = 0x463700**（CC 境界・probe #4b: `push esi; mov esi,ecx; mov ecx,0xB3BDB0; call 0x67CF00` …シングルトン破棄帯）。**E8 caller 2 件**: 0x464499 / 0x4646D9（両方 `mov ecx,ebp; call 0x463700`＝SaveLoadGame 帯からの呼出・probe #7c）。
- **v297 §件1 の「host fn 0x463913」表記を「0x463700（0x463913 は前関数 epilogue 内）」へ訂正**。[esi+0x50] 読手 0x463A63 の帰属は新関数 0x463922..（0x46388e の je 先）＝**v297 §件1 の結論（parse-time mirror 読取）は不変**。

## docs 反映候補（+3 点・#20/#47）

1. Setting 二重登録警告の実文言語 2 系列（list=0xA2FAB0／map=0xA32490）と sink 経路（0x4A7A60→0x4941A0→[0xB34D90]→'Warnings.txt'@0xA3D9BC）
2. INI 値配列の 2 系統 238 スロット構造（base 0xB39554/0xB39578・静的帯 0x9F34xx-0x9F77xx 書手・slot0 予約ゼロ）— v300 件2 の統合表記
3. v297 §件1 の host fn 表記訂正（0x463913→0x463700・`33 cc` 誤検出の教訓）

## 残点（次束候補・低）

1. slot0 センチネル説の ID 空間突合（index0 が実際に使われるか＝読手 0x57C240/0x59BE50 の index 供給元域の桁分析）
2. 0xB39000 ポインタの所有オブジェクト名（静的帯登録データの帰属・RTTI needle 不要の生グローバル）

## 検証（selftest 相当）

`v284_i1..i8.py` 再実行 EXIT=0。整合: 238=0x3B8/4=0xEE 三数一致（teardown/reader 上限/書手範囲）、0x419A70=V283 件1 確定 API、0x401F00=operator new（v289 件2 と同一）、0x9811E2=__security_check_cookie（cmp [0xB30AAC]＝GS cookie シングルトン、v289 走査で毎回出現の 0xB30AAC と一致）、0x983BE0=_chkstk（v288(v60a) CRT 帯）。既存 tmp/re・docs・GT・ソース無編集、ESM/git 非接触。

## 訂正（v308 統合時・2026-11-06）

件2 の「sink 経路」記述のうち 1 点を**訂正**する（v304 追補3・v305 §6 と同型の訂正節）:

- **誤（従来記述）**: 「関数体内 0x4944BC に 'Warnings.txt'@0xA3D9BC の ptr-ref（probe #8b）＝**file sink 接続を確定**（エントリ 0x4941A0 と同一関数スパン 0x4941A0..0x4944F0）」
- **正**: 0x4944BC は **FUN_00494480（Warnings path getter・範囲 0x494480..0x4944E3・直前 CC padding 0x494474..0x49447F）** の体内であり、FUN_004941A0 のスパン外。v301 のスパン想定 0x4941A0..0x4944F0 は **CC/関数境界を跨いだ帯判定の誤り**。[確定]
- **帰結**: 「file sink 接続を確定」の結論は**破棄**。v306/v308 の証拠: vtable ch=1 → 0x4943A0 no-op、FUN_00494730 の Warnings 直接書出 call 0 件、path getter は 0xB34C80 に `<base>\Warnings.txt` を構築するだけで書出者なし ＝ **Warnings.txt 出力は未確認（負）**。caller1=FUN_0045ADC0 は object+0x3C（セーブパス欄）+".txt" の別経路で Warnings.txt を書かない。
- **不変（件2 の有効部分）**: 0x4A7A60 → `call 0x4941A0` の薄ラッパ関係、0x4941A0 の警告ロガー単例構造（`cmp [0xB34D90],0` ゲート・13KB スタック・_chkstk）、件2 の文言 3 点（0xA2FAB0/0xA32490/0xA2F864）は**そのまま有効**。
- 出典: `tmp/re/v306_warnings_path_evidence.md` §2 / `tmp/re/v308_warnings_residuals.md` §2、台帳 row287 / row289。
