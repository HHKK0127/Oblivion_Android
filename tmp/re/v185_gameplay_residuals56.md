# v185: 束E §56 ゲームプレイ残件束 — F3/F4/F5 全 19 項目の解析

対象: Oblivion.exe v1.2.0416 (Steam) / ImageBase 0x400000 / `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
出典: TODO_re49.md F3/F4/F5 (L34-36) / v180_letter_rows_audit.md 束E / RE_EXE_REPORT §56.3-§56.5, §56.8
手法: 静的 RE (capstone 5.0.7 skipdata 逆アセンブル + imm32 参照走査 + 文字列ダンプ)。ESM 再スキャンなし（禁止遵守）。
断定度: [確定] = 命令/バイトで直接確認 / [確定-強] = 複数根拠の整合 / [推定] = 既知事実からの推論 / [未解明] = 残点。
証拠: 中間スクリプト `%TEMP%\opencode\v185_{util,a1,a2,a3,a4,a5,a6}.py`、生出力 `%TEMP%\opencode\v185_{a2..a6}_out.txt`。既存資料: s56_sleep_tail.md / s56_container_residual.md / s56_xml_onlyif.md / s56_section.md (§56.3-56.5, §56.8) / b22_getter_types.md (V118) / v157_runtime_residuals.md / copilot_ini_registries.txt / tmp/re/cache/{named_funcs.txt}。

**結果サマリ: 実施 19 項目 / 解明 17 / 部分解明 1（F4-4） / 既知統合 1（F3-7）。残点は §4 に列挙。**

---

## 1. F3 Sleep/Jail 残件 8 項目

### F3-1. [0xB383A8] / [0xB383B0] の意味 — **解明 [確定]**

**[0xB383A8] = string GMST「sSkillIncreased」、[0xB383B0] = string GMST「sSkillDecreased」の Setting エントリ（実行時 value 格納）**。

- 登録（起動スタブ）: `0x9F02FA: push 0xA5D43C; push 0xA5D42C; mov ecx, 0xB383A8; call 0x419A70`（0x419A70 = int/str registrar、§56.6 確定済み）。名前文字列（EXE 内 ASCII）:
  - 0xA5D42C = `"sSkillIncreased"` / 既定値 0xA5D43C = `"skill increased"`
  - 0xA5D44C = `"sSkillDecreased"` / 既定値 0xA5D45C = `"skill decreased"`
  - 兄弟: [0xB383A0] = `sLevelDoneWarning`（"You need to finish distributing attribute points..."）、[0xB383B0] の次 [0xB383B8] = `sMeditate`（"You should rest and meditate on what you've learned."）
- **jail 内の使い方**（既知 0x670700 メッセージループを確定化）: 日数ループで idx∈{0x1E,0x1F}（+1 される特殊スキル）は **[0xB383A8]**、それ以外（-1 される）は **[0xB383B0]**（負値は neg）を選択 → `0x52EA90`（%変数展開）→ sprintf `0xA73E60 = "%s %s %s %s by %d.\n"` → 0x579C10 表示。
- **別の使用箇所**（新規発見）: 0x50D227-0x50D2B8 / 0x66826B-0x668304（スキル上昇系）: `[0xB383B8](sMeditate) + [0xB383A8](sSkillIncreased)` を 0x52EA90 へ → sprintf `0xA4CCFC = "%s %s %s.  %s"`（第4引数=0xA4CCEC `"UIStatsSkillUp"` サウンド名）→ **0x57ACC0 で HUD 表示 → `[player+0x1DC] = 1`（レベルアップ要求、F3-8 と直結）**。フォールバック書式 0xA4CCE0 = `"%s %s %s."`。
- 静的 u32 値がゴミに見える理由の解明: これらは **.bss（0xB383xx）** にありファイル非存在。値は registrar+ESM ロードで充填される（§56.6 の GMST 書込経路と同一）。

### F3-2. 0x4AE8C0 未読分岐（0x4AEAA8 等）と 0xA41DE8 表 — **解明 [確定]**

**0x4AE8C0（ベッド/監獄アクティベートハンドラ、arg5 / ret 0x14、範囲 0x4AE8C0-0x4AEB31、next func 0x4AEB40）の全分岐を逆アセンブルで確定**:

| 分岐 | 条件 | 動作 |
|---|---|---|
| 0x4AE8E6 | arg1 が dynamic_cast で Actor にならない | return 0 |
| **0x4AEB0F** | `vt+0x18C(actor) != 0` | `0x66C600(actor)` → `[actor+0x61C]=0.0` → `vt+0x320(actor)` → return 1（actor 固有の覚醒/起床系処理） |
| 0x4AE93A | `0x4DBAE0(...)` 位置解決失敗 | return 0 |
| **0x4AEAA8** | `byte[esp+0x16] >= 0xA` | owner チェック経路（下記） |
| 0x4AE990→**0x4AEA84** | player `vt+0x354` 真 | msgbox `[0xB38AA0]`（戦闘/危険中）→ return 1 |
| 0x4AE9AF→**0x4AEA5F** | `0x6605A0(player)` 真 | msgbox `[0xB38AB8]`（敵接近/検知）→ return 1 |
| 0x4AE9C8→**0x4AEA3A** | `0x6773B0(0xB3BD00, 0x4D8B90(player))` 真 = jail 拘束中 | msgbox `[0xB38AC0]`（"You must sleep in a jail bed..."系）→ return 1 |
| 0x4AE9DA owner あり | `0x4DE770(arg1, actor, 1)==0` かつ actor==player | msgbox `[0xB38AB0]`（所有者ベッド）→ return 1（actor!=player は 0x4AEB2A で素通り return 1） |
| **0x4AEAEE** | owner なし/許可あり | `0x66C600(actor)` → `[actor+0x61C]=0.0` → **`0x660760(actor, arg1)` = ベッド使用実行** → return 1 |
| 0x4AEA1C | 可 | `0x676EE0(0xB3BD00)` → **`0x57B420(1)` ShowSleepWaitMenu 起動** → return 1 |

**0xA41DE8 表の正体**: u32 連続配列（0xA41D00-0xA41E38、stride 4）で、**`.text` 内関数アドレスを格納したクラス vtable 帯** [確定]。0xA41DE8 の値 = **0x4AE8C0**、前後 0xA41DE0=0x69D990（no-op）/ 0xA41DE4=0x6FE080 / 0xA41DEC=0x4129A0（×2）。0xA41E3C 以降は文字列 `"Functions that aren't in a loaded file aren't allowed in master file..."`（別領域）。表全体へのコード参照は 0x8A5C5E のみ → 0x4AE8C0 は **TESForm::Activate 相当スロットのクラス別実装** [確定]。表基底（vtable としての参照元）は 0x8A5C5E 帯の他 vtable 連立帯に属す [推定-強、vtable 全体同定は新規残点]。

### F3-3. 0x6738B3 の関数名/用途 — **解明 [確定]**

**0x6738B3 を含む関数 = FUN_006735D0（0x6735D0-0x673970、ret 4、prev 0x671620 / next 0x673980）** = PlayerCharacter の「ゲームタイム遷移/フェード監視ハンドラ」（this=ebx=player、[0xB3BAE0]=経過タイマ、[0xB15018]=フェード値、[0xB33EA0]=ms 時刻、[ebx+0x71C/0x71D]=遷移要求 byte、[0xA31C80]=10.0f 比較）。

- 0x673892 分岐: `now/1000 >= [0xB15018]` 到達時 → `edx = [0xB33B00](save manager); cmp [edx+0x1C0], 0`:
  - **`[+0x1C0] != 0` → 0x6738B3: `call 0x5BDA90`（ShowPauseMenu、s56 §4 確定済み）+ `0x5AEA60(0)`（メインメニュー起動系）** → 0x673935 へ
  - `[+0x1C0] == 0` → 0x6738C4: `[0xB33398]+1 = 1`（メインメニュー遷移要求フラグ）
- 0x6738CF-0x673935（別分岐）: `0x5E0DC0()==2` かつ `[0xB3BB07]==0` かつ vt+0x1C8 → `[+0x1C0]!=0` なら msgbox `0x579C10([0xB38C08], cb=0x663270, 1, [0xB38C10], [0xB38C18])`（セーブ確認ダイアログ [推定-強]）、==0 なら `[0xB33398]+1=1`。ワンショット `[0xB3BB07]=1`。
- 末尾 0x67393E-0x67395C: `0x403520(0x1B, 1)` 判定 → `[0xB33B00]` の `0x466B00` → 成立時 `[0xB15018] = -1.0f`。
- つまり s56_sleep_tail §4 の「ゲーム完成度/遷移系 [推定]」は **「フェード経過後の PauseMenu 表示 or メインメニュー遷移要求（save manager 状態 [+0x1C0] で分岐）」** に確定。

### F3-4. [0xB13228] の意味 — **解明 [確定]**

**[0xB13228] = INI/Setting「bSaveOnTravel」（byte、既定 1）**。

- 名前: 0xA68C8C = `"bSaveOnTravel:GamePlay"`（copilot_ini_defaults.txt と一致、name refs=1）。Setting 登録 0x9FB120 帯（`push 0xB13228; mov ecx,0xB07BF0; call 0x4040E0`）、getter thunk `0x9BE910: mov ecx,0xB13228; jmp 0x403BC0`。兄弟: [0xB13218]=`bSaveOnRest`(0xA68C5C) / [0xB13220]=`bSaveOnWait`(0xA68C74) / [0xB13230]=`bSaveOnInteriorExteriorSwitch`(0xA68CA4) — copilot_ini_registries.txt の台帳と refs 順で完全整合。
- **使用経路 3 系統** [確定]:
  1. `0x66F45B`（Sleep/Wait executor 0x66F420 冒頭）: `cmp byte [0xB13228], al(=0)` → 非 0 なら `0x466B70([0xB33B00])` = **Sleep/Wait 時のオートセーブ**。
  2. `0x4B7D69-0x4B7D9F`（fast travel 経路 0x4B7B40 内、0x6ECC80(player) 直後）: `[0xB13230] && [0xB13228] && [esp+0x18] && 0x4C97F0(...) 判定` → `[[0xB33B00]+0x18] |= 0x8000` = **fast travel セーブ要求**。
  3. `0x5A3810`（Options メニュー）: 表示更新 0x5A37BA（[0xB38DA0]/[0xB38DA8] = ON/OFF 表示 form を 0x58CED0(trait 0xFDE) へ）、トグル 0x5A39EC（`sete` 反転 + `0xB07BF0` manager へ通知）、終了時保存 0x5A3B54。
- s56_sleep_tail §6-5 の「bSaveOnTravel 系 [未確定: 意味]」は **bSaveOnTravel 本体で確定**。ただし Sleep/Wait でも bSaveOnTravel が使われる点は s56 の想定（bSaveOnRest/Wait）と異なる新規事実。

### F3-5. 0x670CA0 の関数終端・0x67108F の所属 — **解明 [確定]**

- **0x670CA0 の範囲 = 0x670CA0-0x670F47（`ret 4` @0x670F47、0x670F4A-0x670F4F は int3）**。next func = 0x670F50。s56 の「終端 0x670F4x 付近 [推定]」を確定。
- **0x67108F = FUN_00670F50 の内部**（範囲 0x670F50-0x6710A4、`ret` @0x6710A4）。0x670F50 = **「player のマーカー/ベッド配置ヘルパ」**（this=edi=player）:
  1. `0x420190([edi+0x44])` でリスト取得 → null/空なら early return
  2. `0x675D50(0xB3BD00, player, 0)`（jailSys 登録）
  3. `[esi+0x18]`（マーカー REFR）位置 `0x4D76F0` + `vt+0x174` で差分計算 → `0x683CB0`（距離）
  4. `[esi+0x18]` の cell 解決（`0x4D6670`）が成功 → **`0x66F370(player, pos, pos2, 0)`**（Sleep executor と同じ配置ヘルパ）→ `0x4D8E60(player, 0)`
  5. cell 解決失敗 → `0x6ECC80` + `0x4C97F0` 判定 → **0x67108F: `0x66EAF0(player, pos)`**（teleport）→ `0x4D8E60(player, 0)`
- つまり 0x66EAF0 teleport 出現 3 箇所（0x670B2D / 0x670F26 / 0x67108F）の第 3 者は「**jail/bed マーカーへの player 配置ヘルパ 0x670F50 の cell 解決失敗経路**」。

### F3-6. 0xA73DF0（jail marker 文字列）の参照コード — **解明 [確定]**

- 実体は **0xA73DE8 = `"no teleport on the interior jail marker"`**（s56 の「0xA73DF0 付近」表記は 8 バイト誤差）。参照コード = **0x6706CF-0x6706D4**（`push 0xA73DE8; call 0x4A7A60` = MessageHandler エラーログ、直後に `[esi+0x608] = 0`（刑期日数クリア））。
- 所属 = **jail enter 本体（0x6704E0 以降の連続ブロック、SEH 終端 0x6706F9 `ret 4`）**。ロジック: `[0xB3BAD0]`（jail ドア REFR）→ `0x4D7630` で REFR 解決 → 失敗時 0x6706CF → ログ + `[esi+0x608]=0`。成功時 0x670602: `0x6765F0` → `0x42B430/0x6899C0/0x42B460` で位置取得 → **`0x66EAF0(player, pos)`**（0x670664）→ `0x5E4140`。player `[+0x116]==0` かつ `[esi+0x1E0]` あり かつ `0x4DE770` 許可 かつ cell 不一致なら **msgbox `[0xB38B50]`（0x57ACC0 経由）**。
- 兄弟文字列の参照も確定: **0xA73E3C `"No interior linked to Jail Marker"` ← 0x6705B4**、**0xA73E10 `"No Jail marker was found to put the Player"` ← 0x6705C6**（いずれも 0x4A7A60 ログ + `[esi+0x608]` へ影響）。

### F3-7. 0x4FAE80 の完全仕様 — **既知統合（V118/V157 で解明済み・本束での新規調査不要）**

- **V118（b22_getter_types.md L221）**: 0x4FAE80 = 条件/式命令の「引数デコードゲータ」。impl（0x503470 等）は gate `0x4FAE80` 経由後 `mov ecx,[g_thePlayer]; call 0x4F6Exx`（GetIs* 本体）へ転送する 90B ラッパ。
- **V157（v157_runtime_residuals.md）**: 本文デコード → opcode 分岐（0x1000=MessageBox / 0x1059=Message）→ %変数式を **0x4FAB20** で逐次評価。%変数レコードの emit(0x4FEF80)⇔consume(0x4FAB20) はバイトレベル一致済み（'G' 3B / その他 6B）。arg8≠0 tail 経路は v1.2.0416 で完全到達不能（デッドコード）と証明済み。
- **V119（b23）**: evaluator 169 件の size 列再決定（GetLocked 実体 75B 等）— 0x4FAE80 帯の境界も確定。
- 残る詳細（0x4FAB20 ジャンプテーブル 0x4FAE3C の残タグ）は v157 §O-c に記録済み。本項目は「**[未確定] から [確定-既知] へ**」の状态昇格のみ。

### F3-8. [0xB14E88]・[ebx+0x1DC] の正体 — **解明 [確定]**

- **[0xB14E88] = INI/Setting「bTrackLevelUps:GamePlay」（byte）**。Setting エントリは 0xB14E88 帯、値 node 0xB14EA8、name ポインタ 0xB14EAC = **0xA73F04 = `"bTrackLevelUps:GamePlay"`**（find_dword 実証）。getter thunk `0x9C42D0: mov ecx,0xB14E88; jmp 0x403BC0`。登録スタブ 0x9FDAE0（`push 0xB14E88; mov ecx,0xB07BF0; call 0x4040E0`）。兄弟 [0xB14E44]=0x9FDA80 帯、[0xB14E90]=0x9C4300 帯。
- **[ebx+0x1DC] = player+0x1DC = レベルアップ要求 byte フラグ** [確定: 書込元実証]。書込み: `0x50D2B1: mov byte [ecx+0x1DC], 1`（ecx=[0xB333C4]=player、スキル上昇メッセージ表示後）、`0x6682FD: mov byte [esi+0x1DC], 1`（同型）。
- **消費経路 2 箇所** [確定]:
  1. `0x671C1C-0x671C3B`（FUN_00671620 = PlayerCharacter 更新大関数内）: `cmp byte [ebx+0x1DC], 0; je 0x671E16` → `cmp byte [0xB14E88], 0; je 0x671E16` → **`call 0x5ACE20`（LevelUpMenu 表示、s56 §3 確定済み）→ ret 4**（更新ループを抜けてメニューを開く）。
  2. `0x5D71FA-0x5D7218`（SleepWaitMenu 確認後処理、FUN_005D7090 内）: `player[+0x1DC]!=0` かつ `[0xB3B72B]==0` かつ `[0xB14E88]==0` のとき `[0xB3A6D1] = 1`（wait 後のレベルアップ要求フラグ、続く 0x65F770「1時間進める」と併用）。
- まとめ: **「bTrackLevelUps が有効 かつ player にレベルアップ要求（+0x1DC）があれば LevelUpMenu を開く」**の2 ガード構造が確定。

---

## 2. F4 menu 式残件 4 項目

### F4-1. `&token` → float 搬送路の断定 — **解明 [確定]（断定）**

- **s56 §56.4 の残点「&xbox; が ID≥0x2710 パス（0x58C063→0x589080→memset→'_'走査→atoi）かの断定」は「不通」で確定**。根拠: 評価器 0x58C063 `movzx eax, word [esi+0x18]`（node+0x18=ID）→ **`cmp ax, 0x2710; jb 0x58C168`**。&xbox; は登録順で ID=1（&true;=2, &generic;=-1 と同時帯 0x58A230-0x58A2A0 登録）< 0x2710 → **0x58C168 側**。
- **ID≥0x2710 パス（0x58C063-0x58C0CE）= 「数値そのものの文字列トークン専用」** [確定]: `0x589080(id)` で名前取得 → `0x4028D0` copy → `memset(0x981630, 0xFF, 0x20)` → '_' 走査ループ → `0x983479`(atoi) で整数化。書式 0xA43804 `"%s%i"`（変換結果の再結合/警告）。
- **ID<0x2710 パス（0x58C168→0x58C129→0x58C171-）** [確定-強]: `mov esi,[esp+0x20]`（名前文字列）→ 0x58C129（`test eax,eax`/`fld st(1) vs [ebx]` 比較、`cmp di,0xFFFF`）→ 0x58C171: 先頭 '_' 判定（`cmp byte [ecx+esi],0x5F`）→ 0x58C1A0: 文字列長測定 + `_` 位置走査 → 0x58C219: `fld [ebx]; call 0x9828C0`(ftol) → sprintf(0xA43804) = 警告/置換パス。
- **&トークンの float 値供給点** [確定]: 0x58C159 `fld [0xA379B4]=2.0f; movzx eax,di; fldz; fxch; jmp 0x58C129`（2.0 値トークン = &true;/&xenon; 系）。値の出所は**名前レジストリ**（初字バケット配列 0xB3B0B0（28×16）+ '_' 始まり専用第 2 コンテナ 0xB13BC4）で解決された float をノード値 (+8) として持ち、copy で acc ([ebx]) へ載せる構造 [確定-強]。
- 0x7D7 `user` ハンドラ 0x58C813 = `push 0xA6AB7C; call 0x4A7A60`、0xA6AB7C = `"ERROR: Unknown action enumeration %i \n"` のみ（ハンドラなし）[確定、s56 補強]。

### F4-2. ln・log（token 0x7E7/0x7E8）の call 先 — **解明 [確定]**

- jump table 0x58C998（token−0x7D2 が index 0..0x19）の該当 2 エントリ（0x58C9EC / 0x58C9F0）は **ln → 0x58C7D5 / log → 0x58C7F4**。
- **ln (0x7E7) 0x58C7D5**: `fstp st(1); fstp st(0); fld [ebx]`（acc）→ `[esp+0x18]` 退避 → **`call 0x986B80` = CRT `log()`（自然対数）** → `[ebx]` へ fstp → 0x58C82B。
- **log (0x7E8) 0x58C7F4**: 同型で **`call 0x986CD0` = CRT `log10()`**（§56.2 で音響解析時に log10=0x986CD0 [確定] 済みと完全一致、内部 `fldlg2+fyl2x`）。
- つまり Oblivion の menu 式で **`<ln>`=自然対数、`<log>`=常用対数** [確定]。

### F4-3. 第 2 コンテナ 0xB13BC4 の用途詳細 — **解明 [確定]**

**0xB13BC4 = 「'_'(0x5F) 始まりトークン専用の第 2 レジストリ（vector/set 相当）」**。

- **挿入側** 0x58A12D-0x58A16E（名前レジスタリ挿入関数 0x58A0A0 内）: 先頭文字が '_' のとき `0x401F00(0x10)` で **16 バイトのヒープ要素**を確保 → `0x589EB0` で初期化（引数: ポインタ + 名前）→ `mov ecx, 0xB13BC4; call 0x42D800` でコンテナへ挿入。それ以外の初字は 0x58A189: `shl ecx,4; add ecx, 0xB3B0B0; call 0x6AA320`（初字バケット 28×16）。
- **構造** [確定]: 0xB13BC4 は
  - [+0] = buffer\*（0xB13BC8）、[+8] = u16 size（0xB13BCC）、[+0xA] = u16 capacity（0xB13BCE）、[+0xC] = u16 追加カウンタ（0xB13BD0）、[+0xE] = u16（0xB13BD2）
  - 再確保 = `0x4E4A10`（0x58B1C6-0x58B1DA）。挿入/解決 0x58B1A7-0x58B21D（`mov [eax+esi*4], edi` で要素登録）。
- **コード参照は 3 箇所のみ** [確定]: 0x58A15E（挿入）、0x58B1D1（再確保呼び出し直前）、0xA24A48（.rdata 内の例外テーブル帯、コードではない）。
- 用途の確定意義: 裸 stackingtype 問題（§56.4）の名前解決失敗が「'_' 始まりトークンの不在」と対応する構造を裏付け。バケット 27='&' とは別の** '_' 専用スイート**。

### F4-4. VideoDisplayMenu 実行時更新 — **部分解明 [着手・主要経路確定]**

s56 §56.4「未着手（任意タスク）」から以下を新たに確定:

- **vtable 0xA6DCCC = {0x5DDA20(dtor), 0x5DD990(slot1), 0x60CF60(no-op), 0x5DDB70(slot2=command), 0x5DDB20(slot3), 0x5DDB40(slot4), 0x890B90×2, ...}**（生ダンプ）。vtable 書込（ctor）= 0x5DD981 / 0x5DDA25。`GetMenuID` = 0x5DDA10（`mov eax,0x3FB; ret`）。
- **0x5DD990（slot1, ret 8）**: arg1=1..8 → `this+0x28..+0x44` へ dword セット（8 スロットの設定値格納）。
- **0x5DDB70（command ハンドラ, ret 8）= 実行時更新の本体** [確定]:
  1. `GetMenuByID(0x3FB)` → tile（0x5898F0）→ `0x9832E6` dynamic_cast（RTTI 0xB147C8）
  2. **command 3**: `0x5DDA50`（既存 tile へ trait 0x1772=2.0 + `jmp 0x584740` 表示/アニメ）+ **`0x5DDCA0` = `GetMenuByID(0x3FA)`（VideoMenu）→ tile → `jmp 0x584390`**（VideoMenu の閉じる/更新）[確定]
  3. **command 4..8**: jump table 0x5DDC44 → 解像度プリセットを `0x5DDAA0(w, h)` へ: command4 = **800×600**（push 0x258/0x320）、command5 = 1080×607 相当（0x438/0x25F）、command6 = **1024×768**（0x400/0x300）、command7/8 = 0x5DDC19 以降（出力端）
  4. `0x5DDAA0`: **`0x497C10(w, h)`（解像度変更コア [推定-強]、E8 直呼びのみで vtable 非所属）** → GetMenuByID(0x3FB) 再表示 → **msgbox `[0xB38CE0]`（解像度確認/失敗メッセージ、runtime form）** → `[0xB13E94]` 等を経て ret 8
- **0x5DDB20 / 0x5DDB40（slot3/4, ret 8）**: command==3 のとき tile+0x30 の trait `"alpha"`(0xFA7) を **0.0 / 255.0（0xA40098）** に設定（フェード表示/非表示）[確定]。
- **0x5DDC60**: command 1..0x30 → `this+0xE8` へ選択値保存（command 6/0x17/0x18 は 0 クリア）[確定]。
- **残点**: `0x497C10` の内部（D3D/デバイスへの解像度適用本体）、`[0xB38CE0]` メッセージ本文（runtime form = B13 ダンプ待ち）、command7/8 の値（0x5DDC19 以降の出力端）。

---

## 3. F5 Container 残件（実機確認 1 件を除外した 7 項目）

※「同一再オープン実機確認」（G8/H7 と同一）は本束の対象外（実機待ち）。

### F5-1. runtime メッセージ form 群の用途 — **解明 [確定: 経路・用途]（本文は B13 ダンプ待ち）**

すべての runtime メッセージ form は **`0x57ACC0`（F5-5 で確定した HUD メッセージラッパ）経由で画面左上に表示**される TESForm（文字列フォーム）[確定]。参照箇所の文脈で用途を確定:

| グローバル | 代表参照 | 文脈（確定） | 用途 |
|---|---|---|---|
| `[0xB38B10]` | 0x59A019 | 0x599EE0 同一対象再オープン分岐: `0x492E70`（Player 全転送）→ sound 0x1D → 0x57ACC0 | 「全取得完了」通知 |
| `[0xB38568]` | 0x59A558 / 0x5AB9DA / 0x5ABDAE | 行アクティベート: `0x4D88F0` で item 取得 → `vt+0x78` 判定真 → 0x57ACC0（0x5ABDAB 系は `[0xB140E4]=-1` 併行） | 「持ち主の許可なしに取れない/使えない」警告 |
| `[0xB38A40]` | 0x59A5E1 / 0x5F2EAA / 0x5F2FC8 / 0x5FB56D | `0x484E80` 真 かつ player `[+0xEC]`（装備中 slot）と item 一致 → 0x57ACC0。0x5F2E7E 系は `0x41DF40` + actor `vt+0x198(0)==0`（死体） | 「装備中/使用中のアイテム」警告 |
| `[0xB37300]` | 0x51D6AD | 0x51D600: `vt+0x330` 経由 `[+0x70]==0xB` のとき、**form 内文字列をスタックへコピー**して 0x57ACC0 | 商人サービス不可メッセージ |
| `[0xB38B50]` | 0x66FC97 / 0x6706B8 | sleep 起床時（0x66FC97）/ jail enter の cell 不一致時（0x6706B8、F3-6） | sleep/jail 周辺通知 |
| `[0xB38AA0]` `[0xB38AB0]` `[0xB38AB8]` `[0xB38AC0]` | 0x4AEA84 / 0x4AEAC9 / 0x4AEA5F / 0x4AEA3A | F3-2 の 0x4AE8C0 各分岐（戦闘中/所有者ベッド/敵接近/jail 中） | 睡眠不可メッセージ 4 種 |
| `[0xB38B30]` `[0xB38AA8]` | 0x4AE96E / 0x4AE9FE | serve-time 確認 msgbox（cb 0x671600）/ sNoWaitInJail | jail ダイアログ |
| `[0xB38C00]` `[0xB38C98]` `[0xB38CF0]` `[0xB38CF8]` `[0xB38D00]` | 0x5BDD1A / 0x5BDF72 / 65 refs / 31 / 32 | PauseMenu 再表示通知 / case8 msgbox / **汎用 message_menu ダイアログ引数群（0x579C10 の第1/第5/第6引数）** | msgbox 形式ダイアログ |

- 精密な表示本文は B13 実機 .data ダンプ待ち（s50_b13_plan.md）だが、**「どの経路で・何のときに出るか」は全件確定**（s56 §5-2 残課題 2/8 に相当）。

### F5-2. +0x63(a4) 特殊フロー 0x59ABEA-0x59AC6F — **解明 [確定]**

ContainerMenu slot[3] ハンドラ（0x59A400 系）内の「**barter モード時の死体取得警告**」[確定]:

1. 0x59ABD7: `[0xB13E94] == -1`（数量未選択）→ `[0xB13E94] = 1` へ補正
2. **0x59ABEA: `cmp byte [ebx+0x63], 0; je 0x59AF11`** — `ebx` = メニュー、`+0x63` = 引数 a4（呼出元 4 経路 `(eax, 0, 0, 1)` = barter [s56 §1-2]）。a4==0（loot）なら通常経路 0x59AF11 へ素通り
3. 続く条件: `[esp+0x2C] == [0xB333C4]`（操作側が Player）→ `0x470620([esi+8])` の **float 戻り**を [esp+0x24] 保存（価格/所持金取得 [推定-強]）→ 直前に `[0xB3B279] = 0`（窃盗フラグクリア）
4. 対象 `[ebx+0x44]` が `0x4D7F80`（type 0x23 = NPC 系判定）真 かつ `vt+0x198(0)==0`（死亡） かつ `[esp+0x20] <= 0.0`（float 比較、jp で不成立）
5. 成立時: **`0x579C10([0xB38CF0], [0xB38C40], 1, 0, ...)` で msgbox**（`[0xB38C40]` = runtime 注意メッセージ form）→ `jmp 0x59A574`（`0x484470` + `0x401F20` delete = ハンドラ終了）

つまり「**barter 中に死んだ NPC から取得しようとした（または価値 0 の対象）ときの注意表示**」。通常経路 0x59AF11 への分岐と合わせ、s56 §4 の `+0x63 (a4) 特殊フロー [未確定]` は解消。

### F5-3. ガード `user0==2.0` の意味 — **解明 [確定-強]**

**[[0xB3A6E0]+0x68]（ルート Tile）の trait `user0`(0xFAE) == 2.0 = 「ゲームプレイ中（GameMode）」状態** [確定-強]。2.0 でないと以下が全てブロックされる:

1. **ContainerMenu オープン**（0x57A8D0 ガード、`fcomp [0xA379B4]` + `test ah,0x44; jp fail` で厳密一致）
2. 兄弟ラッパ（Training 0x57A940 / Skill 0x57A9B0 / Class 0x57AA20 等）
3. **0x57ACC0 = HUD メッセージ表示ラッパ**（F5-5: user0!=2.0 なら即 return）
4. menu 起動ファミリ 0x57B420-0x57B7E0（SleepWait/Pause/LockPick/Book）

`push 0xFAE` は `.text` 内 80 箇所（0x579000-0x57Dxxx 帯 = UI システム全般 + 0x59B5AD/0x59B5FA = ContainerMenu タブ設定 + 0x58AC81 = 名前登録）。値の設定者（2.0 へ書くコード）はゲーム開始/メニュー遷移系 [推定]（本調査では未特定 = 残点）。いずれにせよ「**ルート Tile の user0 は画面モード状態（1.0=メニューモード / 2.0=ゲーム中）**」であることが、ブロックされる群の集合から確定。

### F5-4. 0x422DC0 / 0x41E210 が返す float の内容 — **解明 [確定]**

- **0x41E210 = `ExtraDataList::GetByType(this, u8 type)`** [確定]:
  - `[this+8]` の **bitset**（byte[type>>3] の bit(type&7)、type<0x60）を検査 → 未立ちは即 return 0
  - 立ちならリンクリスト走査（this[+4] から、要素 [+4]=type byte、[+8]=next、`cmp esi,0x5C` キャップ）
  - ヒット時 TLS 経由のキャッシュ（fs:[0x2C]+[0xBA9DE4] マップ）更新 + 要素ポインタを返す
- **0x422DC0 = `GetHaggleAmount(tile+0x44)`** [確定]: `push 0x5C` → `call 0x41E210`（**type 0x5C = ExtraHaggleAmount（'HAHP'）**）→ 要素ありなら **`[eax+0xC]`（float）を返す**、要素なし/失敗なら **0.0f**。
- 用途（s56 §4-34 Haggle）: `0x422DC0(tile+0x44)` → **`0x5BD080(&値, tile, 1)` = NegotiateMenu（交渉メニュー）へ渡す float = 交渉相手の希望額/交渉余地（ExtraHaggleAmount 値）** [確定]。初値 0.0 は「交渉余地なし」。

### F5-5. 0x57ACC0 の正体 — **解明 [確定]**

**0x57ACC0 = HUD メッセージ表示ラッパ**（引数: `msgForm, a2, a3, duration(f32)`）:

1. `[esp+0x14]`（duration）< 0.0 なら 0.0 にクランプ（0x57ACC6-0x57ACDA）
2. `0x582160(0,1)` で UI 取得 → `[+0x1C]` / `[+0x68]` non-null 検証
3. **`0x588BD0([+0x68], 0xFAE=user0)` → `fcomp [0xA379B4]=2.0` 厳密一致**（NaN は jp で不成立 → return）— **ゲームプレイ中のみ表示**
4. `GetMenuByID(0x3F2)` = HUD メニュー → `0x5898F0` tile → `0x9832E6` cast（失敗時 `0x5A8E30` fallback 再取得）
5. **`0x5A9980(tile, msgForm, (a2!=0)+1, a3, duration)`** へ投入（type = a2!=0 ? 2 : 1）

これにより「メッセージ表示系 0x57ACC0（HUD）」と「0x579C10（message_menu.xml ダイアログ）」の役割分離が確定（s56 系の両表記が併存していた箇所の統一に使用可）。

### F5-6. handler id 0xFDF・0xFEB・0xFEA・0xFE9・0xFCC と残りの名前 — **解明 [確定]**

EXE 内文字列（0xA6A3E8 帯、8 バイト詰め配置）で全件確定（s56 §5-1 の id→アドレス対応と結合）:

| id | VA | 名前 |
|---|---|---|
| 0xFDE | 0xA6A3E8 | `type`（既知: "string"=0xFDE との整合は s56 原文の重複表記を訂正: **0xFDE="string" は 0xA6A3D8、0xA6A3E8="type"** [訂正]) |
| **0xFDF** | 0xA6A3F0 | **`shiftclicked`** |
| **0xFEB** | 0xA6A400 | **`returnvalue`** |
| **0xFEA** | 0xA6A40C | **`repeathorizontal`** |
| **0xFE9** | 0xA6A420 | **`repeatvertical`** |
| **0xFCC** | 0xA6A430 | **`red`** |
| 0xFCD（推定連番） | 0xA6A434 | `pagenum` |
| 0xFCE（推定連番） | 0xA6A43C | `pagecount` |
| （以降） | 0xA6A448 | `mouseover` |

- 原文の「handler id 0xFDF・0xFEB・0xFEA・0xFE9・0xFCC **ほか**」= `pagenum` / `pagecount` / `mouseover` まで本表で確定。id↔文字列の厳密対応は登録コード 0x58AC20-0x58AD70 の push 順（s56 §5-1 のバイト証跡）に依拠。

### F5-7. s56 §5-2 残課題の残り（転送コア 0x6AE0A0 シグネチャ）— **補足**

- `0x6AE0A0(AM, formID, 0x121, 0)` は §56.2 で **FUN_6AE0A0 = TESSound 取得 → `"data\sound\"`+name 構築 → 0x6AC610 → 0x6B6DC0 生成** として [確定] 済み（音響束で解消済み・F5 原文の残課題 6 前半は重複解消）。

---

## 4. 新規残点（本束で残ったもの）

| # | 残点 | 所属 | 備考 |
|---|---|---|---|
| R1 | 0xA41D00 vtable 帯の**基底アドレス**（0x8A5C5E 参照帯の特定）と全スロット分類 | F3-2 | 0x4AE8C0=Activate 相当は確定、vtable 全体像は未 |
| R2 | 0x46B800 系 vtable のクラス同定（0xA41DE8 近傍の他クラス） | F3-2 | RTTI なし帯 |
| R3 | `[ebx+0x63]`（=a4=barter）を**設定する呼び出し側**の厳密な意味論（barter 以外の値の有無） | F5-2 | 呼出元 4 経路は s56 で特定済み |
| R4 | 0x470620 の完全シグネチャ（[esi+8] 引数の正体 = item entry か gold か） | F5-2 | float 返しは確定 |
| R5 | `user0==2.0` へ**書くコード**（ゲーム開始/遷移系の特定） | F5-3 | push 0xFAE 80 箇所のうち SetTrait 経路を絞り込み未完 |
| R6 | `[0xB38B10]` 等 runtime メッセージ form の**表示本文** | F5-1 | B13 実機ダンプ待ち（s50_b13_plan.md） |
| R7 | 0x497C10（VideoDisplayMenu 解像度変更コア）の内部 | F4-4 | D3D 適用部 |
| R8 | VideoDisplayMenu command7/8 の解像度値（0x5DDC19 以降） | F4-4 | jump table 0x5DDC44 の残 2 エントリ |
| R9 | `[0xB38CE0]` / `[0xB38C40]` の表示本文 | F4-4 / F5-2 | B13 ダンプ待ち |
| R10 | 0x58C129-0x58C22C のうち「解決成功時に float をどう acc へ載せるか」の最終 1 命令列（0x58C230-0x58C33E 未読） | F4-1 | &xbox; の不通断定は確定済み |

（G8/H7 の「同一再オープン実機確認」は本束対象外のまま）

## 5. 使用証拠

- **本セッション新規**: `%TEMP%\opencode\v185_util.py`（PE セクション変換 / VA↔off / find_dword / capstone skipdata）、`v185_a1.py`（文字列 + imm32 参照一括）、`v185_a2.py`（F3 逆アセンブル群）、`v185_a3.py`（F4/F5 + registrar/INI 検索）、`v185_a4.py`（ln/log + jump table + vtable + push 0xFAE 一覧）、`v185_a5.py`（GMST 名 + UTF-16 + VideoDisplayMenu）、`v185_a6.py`（final: INI registrar + 文字列最終確認）。生出力 `v185_a2_out.txt`(70,965B) / `v185_a3_out.txt`(48,399B) / `v185_a4_out.txt`(12,703B) / `v185_a5_out.txt`(10,818B) / `v185_a6_out.txt`(9,459B)。
- **既存資料（読取）**: `tmp/re/TODO_re49.md`（F3/F4/F5 行）、`v180_letter_rows_audit.md`（束E 節）、`s56_sleep_tail.md`（0x4AE8C0/0x670700/0x670CA0/0xB13228 群の土台）、`s56_container_residual.md`（ガード/handler/userN 表）、`s56_xml_onlyif.md`（&token/ln/log/0xB13BC4 の土台）、`s56_section.md`（§56.3-56.5, §56.8）、`b22_getter_types.md` L221（0x4FAE80）、`v157_runtime_residuals.md`（0x4FAE80/0x4FAB20）、`cache/named_funcs.txt`（関数境界）、`copilot_ini_registries.txt` / `copilot_ini_defaults.txt`（bSaveOn* / bTrackLevelUps の name VA）。
- **主な決定打**: 0xA41DE8 の u32 値 = 0x4AE8C0（vtable 実証）/ 0xA5D42C="sSkillIncreased"・0xA5D44C="sSkillDecreased"（文字列）/ 0x986B80=log・0x986CD0=log10（CRT 呼び出し）/ 0x670F47=ret 4（終端）/ 0x670F50-0x6710A4=0x67108F 所属 / 0xA73DE8="no teleport..."←0x6706CF / 0xA6A3F0="shiftclicked" 等 5 件 / `[player+0x1DC]=1` 書込元 0x50D2B1・0x6682FD / 0x41E210 の bitset+リンクリスト実装。

## 6. 項目別サマリ

| 束 | 項目 | 判定 |
|---|---|---|
| F3 | [0xB383A8]/[0xB383B0] | 解明 [確定]: sSkillIncreased / sSkillDecreased（string GMST） |
| F3 | 0x4AE8C0 未読分岐 + 0xA41DE8 表 | 解明 [確定]: 全分岐読了 + vtable 帯であることを実証 |
| F3 | 0x6738B3 | 解明 [確定]: FUN_006735D0 = フェード経過後の PauseMenu/メインメニュー遷移 |
| F3 | [0xB13228] | 解明 [確定]: bSaveOnTravel（sleep/wait + fast travel の autosave ゲート + Options チェックボックス） |
| F3 | 0x670CA0 終端・0x67108F | 解明 [確定]: 終端 0x670F47 / 0x67108F=FUN_00670F50（マーカー配置ヘルパ）内部 |
| F3 | 0xA73DF0 | 解明 [確定]: 実体 0xA73DE8、参照 0x6706CF（jail enter のエラーログ） |
| F3 | 0x4FAE80 | 既知統合（V118/V157/V119 で確定済み・状態昇格のみ） |
| F3 | [0xB14E88]・[ebx+0x1DC] | 解明 [確定]: bTrackLevelUps / player レベルアップ要求 byte（0x671C36・0x5D7200 で消費） |
| F4 | &token→float 搬送路 | 解明 [確定]: &xbox; 等 ID<0x2710 は 0x58C168 パス、ID≥0x2710 パスは数値文字列専用 |
| F4 | ln・log (0x7E7/0x7E8) | 解明 [確定]: 0x58C7D5→CRT log() / 0x58C7F4→CRT log10() |
| F4 | 第 2 コンテナ 0xB13BC4 | 解明 [確定]: '_' 始まりトーク専用第 2 レジストリ（16B 要素・0x42D800 挿入） |
| F4 | VideoDisplayMenu 実行時更新 | 部分解明: ハンドラ 0x5DDB70/0x5DDCA0/0x5DDAA0 と解像度プリセット確定、0x497C10 内部は残 |
| F5 | runtime メッセージ form | 解明 [確定: 経路・用途]: 全 form が 0x57ACC0 経由の HUD 表示（本文は B13 待ち） |
| F5 | +0x63(a4) 0x59ABEA-0x59AC6F | 解明 [確定]: barter 中の死体/価値 0 取得警告 → msgbox [0xB38C40] |
| F5 | user0==2.0 | 解明 [確定-強]: ゲームプレイ中状態（HUD メッセージ/メニュー起動の総合ゲート） |
| F5 | 0x422DC0・0x41E210 | 解明 [確定]: ExtraDataList::GetByType / ExtraHaggleAmount(0x5C) float 取得 |
| F5 | 0x57ACC0 | 解明 [確定]: HUD メッセージ表示ラッパ（menu 0x3F2 → 0x5A9980 投稿） |
| F5 | handler id 0xFDF 系 | 解明 [確定]: shiftclicked/returnvalue/repeathorizontal/repeatvertical/red（+ pagenum/pagecount/mouseover） |
| F5 | （同一再オープン実機確認） | 対象外（実機待ち = G8/H7 と同一） |

*分析専任のため docs/TODO/cache/session-memory は無変更。中間スクリプトは %TEMP%\opencode 配下のみ。*
