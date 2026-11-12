# v291_getter_thunk_scope.md — serial 280（getter thunk 0xA1B480 帯 対象範囲束・V219残点3）

- 作成日: 2026-11-05 / 担当: 本セッション内実施（SA-A 投入 1 回 upstream error → v277/v286/v289 先例で inline、AGENTS.md ループガード準拠）
- 出典: tmp/re/v293_v219_residual_check.md 残点3 ＋ 台帳 row280 起票文（正本 #44 推奨①の新束）
- 中間物: `%TEMP%\opencode\v280_i1.py/_out`（帯 disasm・ノード地域 dump・全セクション参照走査）・`v280_i2.py/_out`（12 thunk の text 参照全数・登録スタブ文脈・消費者 0x4B1160）
- 判定サマリ: **件(1)レイアウト=確定 / 件(2)帯全数=確定 / 件(3)二次登録経路=確定 / 件(4)bLightAttenuation 以外=確定負。帯の呼称は誤りでした（getter ではなく atexit 登録解除スタブ）**

## 結論先行: 「getter thunk 帯」の正体

0xA1B3C0..0xA1B5D0（**stride 0x30・ちょうど 12 件**）の thunk は getter ではなく **INI キー登録解除（atexit クリーンアップ）スタブ**:

```
thunkN: push &nodeN; mov ecx, 0xB07CFC; call 0x65C620   ; = INI名リスト unlink（v264 件9 の機構と同一）
        mov eax,[nodeN+4]; if(eax && byte[eax]=='S'(0x53)) call 0x401F20  ; キー文字列 buffer を条件付き delete
        ret
```

v223 が「getter thunk」と呼称したのは誤帰属。**真の getter 形 thunk は直前の 0xA1B3A0/0xA1B3B0**（`mov ecx, 0xB35AB0/0xB35AB8; jmp 0x4055E0`——別オブジェクト参照のジャンク形・本束範囲外として区別記録）。

## 件1 ノード配列レイアウト — 【確定】

- ノード = **8B {+0: 値スロット（u32/bytes 共用）, +4: キー文字列 ptr}**。地域 0xB08138..0xB08194 に 12 ノード連続（stride 8）。
- dump 実測（v280_i1 (b)）: `0xB0813C='bUseConstant:bLightAttenuation'`、`0xB08144='bUseLinear:…'`、`0xB0814C='bUseQuadratic:…'`、`0xB08154='fFlickerMovement:…'`、`0xB0815C='uLinearMethod:…'`、`0xB08164='uQuadraticMethod:…'`、`0xB0816C='fLinearValue:…'`、`0xB08174='fQuadraticValue:…'`、`0xB0817C='fConstantValue:…'`、`0xB08184='bOutQuadInLin:…'`、`0xB0818C='fLinearRadiusMult:…'`、`0xB08194='fQuadraticRadiusMult:…'`。+0 側には初期値语（0xB08150=0x41000000、0xB08168=0x40400000、0xB08170=0x41800000=float 2.0/3.0 系、0xB08188/190=0x3F800000=1.0）が立つ。
- ノード直後 0xB08198.. は別系統（'Actors'/'Items'/'Magic'/'WorldObjects'/'Miscellaneous' とカテゴリ名配列＋formType 索引バイト列＋'Activator'..'Potion' 名前表）＝thunk 対応なし＝**別テーブル**（v219 系谱の「bLightAttenuation 群」境界を確認）。

## 件2 帯の先頭/末尾/個数/帰属全表 — 【確定】（12/12 全対応）

| thunk | push ノード | INI キー | text 参照（登録側） |
|---|---|---|---|
| 0xA1B3C0 | 0xB08138 | bUseConstant:bLightAttenuation | 0x9E2268 |
| 0xA1B3F0 | 0xB08140 | bUseLinear:bLightAttenuation | 0x9E22C8 |
| 0xA1B420 | 0xB08148 | bUseQuadratic:bLightAttenuation | 0x9E2328 |
| 0xA1B450 | 0xB08150 | fFlickerMovement:bLightAttenuation | 0x9E2388 |
| **0xA1B480** | **0xB08158** | **uLinearMethod:bLightAttenuation** | 0x9E23E8 |
| **0xA1B4B0** | **0xB08160** | **uQuadraticMethod:bLightAttenuation** | 0x9E2448 |
| 0xA1B4E0 | 0xB08168 | fLinearValue:bLightAttenuation | 0x9E24A8 |
| 0xA1B510 | 0xB08170 | fQuadraticValue:bLightAttenuation | 0x9E2508 |
| 0xA1B540 | 0xB08178 | fConstantValue:bLightAttenuation | 0x9E2568 |
| 0xA1B570 | 0xB08180 | bOutQuadInLin:bLightAttenuation | 0x9E25C8 |
| 0xA1B5A0 | 0xB08188 | fLinearRadiusMult:bLightAttenuation | 0x9E2628 |
| 0xA1B5D0 | 0xB08190 | fQuadraticRadiusMult:bLightAttenuation | 0x9E2688 |

- 各 thunk の imm32 参照は **.text 恰も 1 件**（上表右列）＋自帯内。rdata/data 参照ゼロ（v280_i1 (c) で 4 重点検索、0xA1B480/0xA1B4B0 の data 参照 0 を直接確認＝v223「needle 0xA1B480 ヒット 0」の理由=間接でなく**登録は 0x9E22xx 帯・解除は thunk 自体**）。
- 帯 = 12 のみ（0xA1B3C0 先頭・0xA1B5D0 末尾、stride 0x30、以降は別形 thunk 群）。

## 件3 二次登録経路 — 【確定】

0x9E22xx..0x9E26xx 帯（1 スタブ stride 0x60・12 件）の各関数形（命令実測）:

```
push -1; push <SEH handler 0x9B31xx>; mov eax,fs:[0]; push eax; mov eax,[0xB30AAC]; xor eax,esp; push eax
lea eax,[esp+4]; mov fs:[0],eax
push &nodeN; mov ecx, 0xB07BF0; mov [esp+0x10],0; call 0x4040E0   ; SettingsManager 単例@0xB07BF0（v275: vt 0xA404F8・slot+0x10 汎用 INI ローダ FUN_004A8800 の保持元）へのキー登録
push thunkN; call 0x981FB4                                        ; atexit ラッパ（v288(v60a) 確定 の 0x981FB4 と同一）→ 解除 thunk を exit 時に登録
add esp,4; mov ecx,[esp+4]; mov fs:[0],ecx; pop ecx; add esp,0xc; ret
```

- **帰結（起動時期の裁定）**: 登録は **0x9E22xx 静的初期化帯**＝MSVC do_global_ctors/CRT 初期化経路（v282 が確定した「CRT 初期化テーブル 0xA283D0..0xA2F77C→インラインループ 0x981CC5」の外周域と同型）。**ゲーム実行時・ESM リロード時ではなくプロセス起動 1 回**。解除は atexit 登録によりプロセス終了時。
- 0x4040E0 は ecx=SettingsManager(0xB07BF0) の thiscall 登録 API、0x65C620（v264 件9）は解除側のリスト unlink——**登録/解除対が 0x981FB4/atexit で束ねられている構造**が本束で閉じた。

## 件4 「bLightAttenuation 群以外」の到達 — 【確定負】

- 帯 12/12 の push 先は全件 `*:bLightAttenuation`（上表）。群外キーはこの帯に**存在しない**（0xA1B600 以降は別形 thunk、0xB08198.. のカテゴリ名群は別テーブルで thunk 対応なし）。
- v223 記載「imm32 参照 0xB08158/0xB08160 各 6 件」と完全一致（実測 6/6: ノード 158={0x4B11A5, 0x4B11B8, 0x4B129E, 0x9B3191, 0x9E23D1, 0xA1B481}／160={0x4B11BD, 0x4B11D0, 0x4B1340, 0x9B31C1, 0x9E2431, 0xA1B4B1}）。6 件の内訳=**消費者 3+SEH 1+登録 1+thunk 1** が本束で確定。

## 消費者側（おまけ・命令実測）

- **0x4B1160 帯**: bUseConstant/Linear/Quadratic（bytes at 138/140/148）から **0xB35AC0 の減衰モード合成**（定数→1/線形→2/二次→4、いずれも off なら 4 既定）。uLinearMethod(158)/uQuadraticMethod(160) を 0/1/2 へ**clamp 書換**（0x4B11B6 `mov [0xB08158],ecx`・0x4B11CE `mov [0xB08160],2`）→ fLinearRadiusMult(188=0x3F800000)/fQuadraticRadiusMult(190) は fld/fcom で消費。移植対応: Oblivion の `bUseX:bLightAttenuation` 3 フラグ→減衰モード 1/2/4 とメソッド clamp はこの関数が正典。
- 0x9B319x = 登録スタブの SEH ハンドラ帯（ノード参照は try ブロック内の一時）。

## docs 反映候補（+2 点・#20/#47 ゲートへ）

1. 「getter thunk 0xA1B480 帯」の呼称訂正: 実体=**bLightAttenuation INI 12 キーの atexit 登録解除スタブ帯**（0x4040E0(ecx=0xB07BF0 SettingsManager) 登録 + 0x981FB4(atexit) 対）
2. bLightAttenuation 減衰合成規則（0x4B1160: 3 フラグ→[0xB35AC0]=1/2/4・off時既定4、uLinear/uQuadratic は {0,1,2} clamp）

## 残点（次束候補・低）

1. 0x9E22xx スタブ帯 12 件の**CRT 初期化テーブル側の対応スロット列挙**（登録の実行順序保証の有無。0x981FB4 の atexit LIFO 依存なら解除順序=逆順）。
2. 0x4055E0（真の getter 形 thunk が jmp する先）の機構——bss オブジェクト 0xB35AB0/AB8 の lazy-init getter 説の検証。

## 検証（selftest 相当）

`v280_i1/i2.py` 再実行 EXIT=0。数値整合: 帯 12=登録 12=キー 12=参照 12（各 1 件）で閉包・v223「各 6 件」と突合一致・v293「needle 0xA1B480 直接ヒット 0（＝参照は自帯+0x9E23E8 のみ）」の説明成立。既存 tmp/re・docs・GT・ソース無編集、ESM/git 非接触。
