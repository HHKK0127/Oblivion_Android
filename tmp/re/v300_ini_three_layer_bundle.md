# v300_ini_three_layer_bundle.md — serial 283（INI 3層構造確定束・4件）

- 作成日: 2026-11-05 / 担当: 本セッション内実施
- 出典: tmp/re/v299_pending_mask_setter.md 残点1..3 ＋ 台帳 row283 起票文
- 中間物: `%TEMP%\opencode\v283_i1.py/_out`（0x4040E0/0x419A70 全文・0x40C3FB fn 帯・0xB07CFC/0xB07BFF0 参照国勢）・`v283_i2.py/_out`（0x40C54F 文脈・0x412D30 insert 本体・0x4055E0 参照ゼロ確認）
- 判定サマリ: **件1=確定（3層構造・算術一致）/ 件2=確定（teardown drain＋新 companion 発見）/ 件3=恒久保留（v299 判定踏襲）/ 件4=参照のみ**

## 件1 3層構造 0xB07BF0 → 0xB07CFC → 0xB35574 — 【確定】（v299 [推定強化] → [確定]）

### 算術・レイアウトの直接証明

- **`0xB07BF0 + 0x10C = 0xB07CFC`**（命令 `lea edi,[ecx+0x10c]`@0x4040EE・ecx=0xB07BF0 SettingsManager 単例）＝ v291 §件2 の「INI 名リスト 0xB07CFC」は **SettingsManager の +0x10C リストヘッド**であり、別オブジェクトではない。+0x110 = 空/件数ゲート（`cmp dword[ecx+0x110],0`@0x4040E0 先頭）。
- **登録 API `0x4040E0(this=SettingsManager, node)`（全文実測）**:
  1. `+0x110==0 && +0x10C==0` → 空リスト時 0x40411E へ直行
  2. リスト走査: `mov eax,[ebx+4]`（node のキー文字列）と各ノードのキーを **`call 0x98262D`（stricmp）**で比較（0x98262D=v288 確定帯の文字列 compare ラッパ）
  3. **重複検出**（eax==0 → 0x40412C）: `push [ebx+4]; push 0xA2FAB0; call 0x4A7A60`＝**重複登録警告ログ**（0x4A7A60=ログ出力・v289 件2-2 の 0xA3B230 系と同一ファミリ）
  4. 未登録: **`push ebx; mov ecx,edi(=+0x10C); call 0x446CB0`（Push）**＝v289 §2-2 で「既存正典で同定済・重複登録しない」とされた 0x446CB0 の**実呼び出しを本束で初確認**
- **スカラー値 store 登録 `0x419A70`（全文実測・SEH 0x9AB578 ファミリ）**: ノード `{[esi]=arg2, [esi+4]=arg1}` 構築 → arg1 が非 null なら **`mov ecx,0xB35574; call 0x55E000`（find）** → 重複時（al≠0）は **`push [esi+4]; push 0xA32490; call 0x4A7A60`（重複警告）** → 未登録時は **`mov ecx,0xB35574; call 0x412D30`（insert）**。
- **`0x412D30`（insert 本体・全文頭部実測）**: NiTMap 型（`[esi]=vtbl` → `call [vtbl+4]`=hash → `mov eax,[esi+8]; mov edi,[eax+ebp*4]`=バケット → チェーン走査 `[edi+4]` キー比較 `call [vtbl+8]` → 0x412D68 で `call [vtbl+0x14]`=挿入）。**0xB35574 は vtable 付き NiTMap**（非 polymorphic グローバル＝v274 §6 SLG と同型の「vtbl 非設置だが中身は vtbl 付き」パターン）。

### 3 層の確定した役割分担

| 層 | アドレス | 実体 | API |
|---|---|---|---|
| ① owner | 0xB07BF0 | SettingsManager 単例（vt 0xA404F8・v275） | 0x4040E0 登録（stricmp 重複検査+警告 0xA2FAB0）、0x40448C.. 等 32 mov ecx |
| ② name list | 0xB07CFC（=①+0x10C） | ノード連結リスト（{+0 value, +4 key str}） | 0x446CB0 Push（登録）/ 0x65C620 unlink（解除・v264 件9） |
| ③ value store | 0xB35574 | **NiTMap（vtbl 付き）** | 0x55E000 find / 0x412D30 insert / 0x4524C0 remove / 0x4055E0 node 破棄（v298） |

- 参照国勢: 0xB07CFC=574 件（mov ecx 60・thunk 帯 0xA164xx.. 0xA1B5xx 多数）、0xB07BF0=634 件（mov ecx 32＋`mov eax,imm` 16）＝ **INI システムの 2 大ハブ**と確定。

## 件2 teardown drain と [0xB39578] companion — 【確定】（新事実）

- **fn 0x40C3FB 帯（shutdown 経路・v281 確定の WinMain 帯）の 0xB35574 参照文脈**（0x40C530..0x40C57B 全文実測）:
  ```
  mov esi,[0xB39578]            ; companion シングルトンのリスト先頭
  loop: if([esi+4]!=0) 4524C0(0xB35574, [esi+4])   ; value store からキー除去
        if(byte[[esi+4]]=='S') 401F20([esi+4])      ; 'S'-タグ文字列 delete
        401F20(esi)                                 ; ノード自体 delete
        esi=[esi] へ進む
  ```
- **新規発見: `[0xB39578]`＝登録順 companion リスト**（teardown の drain 元）。0xB35574（map）と 0xB39578（list）は**二重構造**で、破棄は list 側を辿って map から remove——登録順保証のための従属リストと確定。
- 0x40C3FB fn 自体は E8 caller ゼロ・内部文字列ゼロ＝**CRT/WinMain shutdown walk から到達**（v281 件4 と同型の「到達はテーブル経由」パターン）。所有クラスは**生グローバル（RTTI needle 不要）**＝v299 件3 の「正式クラス名 RTTI なし」を本束で裏付け（vtbl 付き NiTMap だが外皮は生アドレス）。
- **0x4055E0 の refs=0（E8 全域・jmp 専用）**: thunk 形 `mov ecx,obj; jmp 0x4055E0`（0xA1B3A0 系）のみから到達と**命令レベルで確定**（v298 §件3 の「atexit クリーンアップスタブ」を補強）。

## 件3 bit1 — 【恒久保留】（v299 判定踏襲・再調査不要）

v299 §件2 の結論を維持: bit1 は 0x452000..0x465000 に生成窓なし（3 窓は全て +0x18 form 級）＝セーブ/GECK 側値域。禁止域のため恒久保留。

## 件4 — 【参照のみ】

v298 §件(5)（TOFT 訂正ドラフト）・v299 §件4（呼称訂正ドラフト）に記載済。重複執筆せず。

## docs 反映候補（+2 点・#20/#47）

1. INI 3 層構造の確定: SettingsManager(0xB07BF0) の +0x10C=名リスト(0xB07CFC)、+0x110=空ゲート、登録=0x4040E0(stricmp 重複検査+警告 0xA2FAB0)+0x446CB0 Push、値=0xB35574 NiTMap(0x55E000/0x412D30/0x4524C0)、teardown=[0xB39578] drain list
2. 重複登録警告の 2 系列（名=0xA2FAB0・値=0xA32490）と 0x4A7A60 ログ出力の正体

## 残点（次束候補・低）

1. [0xB39578] の登録書手（0x419A70/0x4040E0 側のどちらが list push するか——0x419A70 内に list 操作は未見＝別の登録経路候補）
2. 0x4A7A60 ログ出力のフォーマット定数群（0xA2FAB0/0xA32490 の実文言語・Warnings.txt 経路の接続）

## 検証（selftest 相当）

`v283_i1/i2.py` 再実行 EXIT=0。整合: 0xB07BF0+0x10C=0xB07CFC 算術一致、0x446CB0/0x65C620/0x55E000/0x4524C0 の 4 API は v289/v291/v298 の既確定と完全整合、0x98262D=v288 確定の stricmp ラッパ、0x9AB578=v288 確定の EH ファミリ。既存 tmp/re・docs・GT・ソース無編集、ESM/git 非接触。
