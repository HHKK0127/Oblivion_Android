# v198_opcode_gt_residuals2.md — opcode/GT 残点 第5束（V185）

- 作成日: 2026-10-29 / 担当: TODO serial 185 (V185)
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- スコープ: `TODO_re49.md` serial 185（V185）行
  1. %変数レコード +0x10（'f'/'s' 判定 byte）の初回書手（型宣言側）
  2. opcode 0x1C の seta フラグ（record[+0xC]>0）の後続消費箇所と record[+0xC] の実体
  3. `[0xB33B00]` クラスの正式命名（非仮想・RTTI 無し）
  4. `base+0x58` bit7/22/31 の正式フラグ名（+0x58 の formID/flags デュアリティ確定）
  5. MagicItem 派生クラス（SpellItem 等）の slot[6] override 本体と +0x5C 埋め込みコントローラの class 名
- 契約: `tmp/re/SUBAGENT_CONTRACT.md`（読取のみ・成果物は本ファイルのみ・docs/・TODO・台帳・git・Supermemory・sync は編集禁止）
- 出発点: `tmp/re/v193_opcode_gt_residuals.md` §4（残留9件のうち本スコープ5件）／元表 `v178_runtime_residuals3.md` §2.5/§3・`v177_gt_slot_residuals2.md` §5/§7

## 1. 手法 / 入力

- capstone 5.0.7（detail 付き Cs）で逆アセンブル。`%TEMP%\opencode\probe_harness.py`（selftest 4/4 PASS）＋ `cache/`（opcode_table.json 533行 / call_targets.json / vtable_index.tsv / named_funcs.txt）。
- decompiled（29 MB）は該当行の部分読取のみ（GT §2 規則: `python enumerate(f,1)` 部分検索）。
- 中間スクリプト: `%TEMP%\opencode\v198_1..N_*.py` + 出力 `v198_*_out.txt`。
- 既存中間物: `v193_1..3_*.py` / `v193_*_out.txt`（前タスク）を再利用。
- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時確認済: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。

## 2. 証拠・分析

### V-185-1: %変数レコード +0x10 の初回書手 — [残を縮小]

- **レコード実体の完全同定**: `FUN_004FA910`（clone、v193 で確認済）本文実測で、レコードは **0x18 セル**: `+0 = value(dword)` / `+8 = f64(8B)` / `+0x10 = byte フラグ`。name は clone 先に無い（v193 の注記どおりソースノード側）。
- **decompiled L194376-194385（FUN_004faa90）再確認**: this+0x48 リストのノード `puVar2` は **`puVar2[6]`（+0x18）= name char\*** を持ち、`*(char*)(puVar2+4)`（+0x10）で 'f'/'s' を返す。→ **リストのノード実体は 0x1C+（+0x18 に name ptr）**であり、clone 先の 0x18 セルは name 無しの値コピーである [確定]。
- **書手走査（本タスク）**: compiler band `0x4F0000..0x504000` で `C6 disp8=0x10` 形の byte store は **0 件**（sanity: band 内 C6 総数 241 / mod=1,2 は 140 件 → 走査自体は機能している）。malloc(0x18/0x1C/0x20/0x24)→直後 64B 以内に disp 0x10 byte store を持つ候補も 0x4E0000..0x520000 で 0 件。→ **compiler 帯に初回書手は存在しない** [確定(不在)]。
- **登録帯の断片**: `FUN_004fbb60`（decompiled 全読）内で param_1+0x10 リスト（+8=f32 ptr / +0xC=参照解決カウンタ）の走査と fQuestDelayTime 例外経路を確認。これは **スクリプトの "Referenced object list"**（エラー文 "Referenced object %d on Script (%08X) '%s' is invalid"）であり、%変数レコードとは別リストである（**+0xC の実体はこのリストでは参照 formID**）。
- `FUN_004fbdc0`（clone 呼び出し元2の1）= 0x14 セル {param_1, 0, 0, clone結果, 0} を生成する **スクリプトコンパイル単位の変数スナップショット**。初回書手（INI/エンジン変数登録帯）は **本タスクでも未検出 [残]**（%変数システムの登録関数は 0x4F0000 帯外の可能性が高い）。

### V-185-2: opcode 0x1C の seta フラグ後続消費 — [確定]

- **fetch `0x516830` 全文実測**（`ret 0x18`、0x516830..0x51696D）。seta 周辺:
  ```
  0x51687C: mov byte [esp+0x18], 0          ; local 初期化
  0x516895: cmp dword [eax+0xC], 0          ; eax = 解決レコード
  0x51689C: seta byte [esp+0x18]            ; record[+0xC] > 0 (unsigned)
  ...  rec[+8] を 0x9832E6 RTTI キャスト → [out] へ ...
  0x5168D3: pop ebx
  0x5168DF: cmp byte [esp+0x14], 0          ; pop ebx 後の同一 local
  0x5168E4: jne 0x516955                    ; ← seta 消費点
  ```
- **後続消費箇所 = 同一 fetch 内の 0x5168DF**（pop ebx により [esp+0x18]→[esp+0x14] に再番号）。**エラーパス抑制ゲート**として機能: `bl(引数 seta) != 0` か `[esp+0x24](out) != 0` か `seta local != 0` のいずれかで 0x516955（正常 return 1）へ。3 条件すべて 0 のときのみ 0x4FCA30(表行)→vcall +0xD4→0x5167A0(0xA4DB08/0xA4DAC8 メッセージ) のエラー/診断経路へ進む [確定]。
- **record[+0xC] の実体**: v193 の「double 上位ワード」推定は **誤り**（訂正）。レコードは **union**: 'f' 型なら +8=f64（+0xC=上位ワードだが opcode 0x1C は +8 を **ポインタとして RTTI キャスト**している＝+8 が f64 の経路は 0x1C では使えない）。opcode 0x1C が解決する +0x40 リスト（`FUN_004fa9c0`: this+0x40 リストを id 番号順に走査、キャッシュ DAT_00b361b0/b09e1c/b361b4）のレコードは **object-reference 型**: +8=TESForm\*、+0xC=formID（未解決時も残る）。seta は「formID > 0 ＝ form 解決済み/ID 付きレコード」を意味する。'G'/'Z' タグ系（0x4FAB20 L194470-194481: rec[+8] を object として [obj+0x24](f32) / [obj+0xC](formID) を読む）と整合 [確定(構造)]。

### V-185-3: `[0xB33B00]` クラスの正式命名 — [確定] `TESSaveLoadGame`

- **文字列直接証拠（.rdata 実測）**: `0xA3A4E0` = `"Error: TESSaveLoadGame::SaveGameData() was called while loading.\n"`。同族文字列を .rdata 全走査で追加検出:
  - `0xA3A82C` = `"TESSaveLoadGame::SaveQueuedHavokData"`
  - `0xA3A854` = `"TESSaveLoadGame::SaveQueuedAttachedAnimation"`
  - `0xA3B06C` = `"TESSaveLoadGame::LoadForm"`（"urrently has form type %s. Its loading will be skipped."）
  - `0xA3B0B8` = `"TESSaveLoadGame::UnloadForm"`
- **xrefs**: `0xA3A4E0` を参照する 3 site = `FUN_0045B9A0`（0x45B9D3）/ `FUN_0045F7A0`（0x45F7D4）/ `FUN_0045F820`（0x45F8F1）。ctor `0x45B300` と同帯（0x45A000..0x45C400）に所属。
- **ctor `0x45B300` 全文実測**: size 0x1C4 zero-fill（`[esi+4]..[esi+0x1C0]`）→ `mov [0xB33B00], esi`（内部公開 @0x45B3C8）→ malloc(0x10) を 8 回（各 `+0/+8/+0xC/+0x10/+0x54/+0x58/+0x5C` 等へ sub-ctor 0x45A860/0x45A940/0x45AA80/0x45ABE0 を配布）→ `[esi+0x7D]=1`（byte）→ `[esi+0x1C0]=0`。vtable 代入なし＝**非仮想 POD**。
- **台帳・他文書突合**: `.github/memory/session-memory.json` L5819「0x00465130=SaveGame本体(ecx=0x00B33B00)」、L9212「save manager global [0xB33B00]; save exec 0x465130 / load exec 0x465860」。`tmp/re/b01_counter.md:110`「g_pSaveLoad(0xB33B00)」、`b20_impl_5071d0.md`「[g_0xB33B00+0x1C0] で callback を選ぶ」、`b25_cc_residuals.md:72`「[g_pSaveLoad(0xB33B00)+0x7C] = セーブファイルのバージョン byte」。**Ghidra/FID ラベルは `FUN_0045b300`（named_funcs）で、命名は無い**＝本タスクで `TESSaveLoadGame` に確定。
- **命名根拠の整理**: クラスは (a) 非仮想・RTTI 無し POD、(b) formID マップ +0/+4（v178 §3.2）、(c) `+0x7C` = セーブバージョン byte（0x7D=125。b25 で `cmp +0x7C, 0x5F/0x66/0x7D` ゲート実測）、(d) `+0x80` リスト（警報報道 0x426A90 が読む）、(e) `+0x1C0` = メニュー callback ptr（b20）。**セーブ/ロード管理シングルトン**。
- ⚠️ **訂正候補（v178 §3.2 の +0x7C "formType"）**: v178 は `+0x7C` を「formType byte（[0x20,0x5A) なら真、0x45A170）」と記載したが、**b25 は +0x7C をセーブバージョン byte（0x7D）と実測**。`FUN_0045A170` の `[0x20,0x5A)` 判定は formType ではなく**サポート版数レンジ**の可能性が高い（0x7D=125 は [0x20,0x5A) 外）。→ **+0x7C = セーブバージョン [有力]、v178 の formType 命名は要訂正**。

### V-185-5: MagicItem 派生 slot[6] と +0x5C 埋め込みコントローラ — [大部分確定]

- **MagicItem vtable の slot[6] は 2 系統**（vtable_index 突合）:
  1. **TESForm 系 55-slot primary**（SpellItem `0xA34C3C` / EnchantmentItem `0xA33C84`）: slot[6]（+0x18）= **`0x418F10`**（Spell・Enchant 共通）。
  2. **MagicItem 系 16-slot**（埋め込み MagicItem サブオブジェクト。SpellItem `0xA34BF4` off=+0x18 / EnchantmentItem `0xA33C3C` off=+0x18）: base MagicItem `0xA322C4` の slot[6] = **`0x982D85`**（pure-call 共有スタブ）。派生 override: **SpellItem → `0x4D9B40`** / EnchantmentItem → `0x7AFE90` / AlchemyItem → `0x4059C0` / IngredientItem → `0x419750`。
- **`FUN_00699190` が読む slot[6] は 16-slot MagicItem 系**: `iVar3 = (**(code**)(*param_2 + 0x18))()` → **`==1` で SpellItem dynamic_cast（型表 0xB03280→0xB03800）→ this(+0x5C) の vtable[1] 呼び出し** / **`==4` で vtable[0] 呼び出し**。param_2 = `item[+0x64]→+0x18`（enchantment サブオブジェクトの埋め込み MagicItem）。→ **slot[6] は「SpellItem か否か」を返す type 述語**（SpellItem=1 / その他=4）。[確定(構造)]
- **`FUN_00418F10`（55-slot slot[6]）本文**: `lea ecx,[esi+0x24]; call 0x414C70; mov ecx,esi; jmp 0x46C2F0(→0x46B170)`。+0x24 リスト解放＋base dtor へ tail-jmp ＝ **destructor thunk**（1/4 を返さない）。→ v177 の「vt+0x18=slot[6]」は **55-slot primary と 16-slot sub の取り違え** [訂正]。実際の type 述語 slot[6] は **16-slot MagicItem 系**。
- **`+0x5C` 埋め込みコントローラ = Actor の MagicCaster サブオブジェクト** [確定(構造)]:
  - Actor ctor `FUN_005F13D0`（0x5F13FD）: `lea ebx,[esi+0x5C]` → `[esi]=0xA6E074`(Actor primary) / `[esi+0x18]=0xA6E06C` / **`[ebx]=0xA6E024`** / `[esi+0x68]=0xA6DFF4` を代入。＝ **Actor+0x5C は vtable `0xA6E024`（17 slots）を持つサブオブジェクト**。
  - `vtable_index` row 492: `Actor 0xA6E024 / 17 slots / off=0x5C`（RTTI `.?AVActor@@`）。
  - **slot 突合で MagicCaster と同型**: `0xA6E024` の slot[2]=`0x69BE90` / slot[3]=`0x699500` / slot[4]=`0x5EA2E0` / slot[10]=`0x6992A0` / slot[11]=`0x5E5630` / slot[16]=`0x699090` が **MagicCaster `0xA75DEC`（row 798）と一致**。slot[3]=`FUN_00699500` は difficulty 計算（`[actor+0x26c] × [actor+0xec]`、GT §4.1.1 C の GetScale 系と整合）。
  - `FUN_00699190` の this = `lea ecx,[esi+0x5C]`（StartItemAction `0x5E4440` から）＝ **+0x5C コントローラは MagicCaster フェイス**。+0x5C は Actor が多重継承する **MagicCaster サブオブジェクト**（非 Actor 本体）。class 名: RTTI は `.?AVActor@@` を指すが、機能的正体は **MagicCaster**。

### V-185-4: `base+0x58` bit7/22/31 の正式フラグ名 — [大部分確定 / フラグ名は部局推定]

- **+0x58 の formID/flags デュアリティは [確定]**: SCRI ロード時 `FUN_004510E0`（4-byte formID reader）→ `[form+0x58]` へ格納（BOOK `FUN_004B5440` の SCRI ケース、v177 §5.2 で実測済）。解決器 `FUN_0046F020`（本タスク全文実測）:
  ```
  0x46F020: cmp byte [edi+8],0        ; +8 = resolved/error フラグ
            mov eax,[edi+4]           ; +4 = formID
            ... 0x46B680/0x46BB20（ファイルコンテキスタグ）...
            call 0x46B250             ; formID → lookup
            push 0xB03358 / 0xB02F9C
            call 0x9832E6             ; RTTI キャスト（script 型）
            mov [edi+4],eax           ; ← formID を解決済 TESForm* へ置換
            失敗時: "Unable to find script (%08X) on owner object" (0xA3C508) + [edi+8]=1
  ```
  → **link dword は「解決前=formID / 解決後=TESForm\*」のデュアリティ**を再確認 [確定]。
- **bit 読みがどのモードで意味を持つかの裁定**: 走査で **`[reg+0x58]` への `or/and` 即時 byte/dword store は 2 件のみ（bit7/22/31 マスクを含むもの 0 件）** → **bit7/22/31 は明示的に set されない**＝formID 値（またはポインタ値）の**一部として**現れる。ここからの帰結:
  - **bit31 (0x80000000)**: 有効な formID（<2^24）でも有効な user-space ポインタ（<2^31）でも bit31 は構造的に **0**。よって bit31 立ちは **負のセンチネル（-1 = 0xFFFFFFFF 等）＝ 無効/削除リンク**のみ。`FUN_004AE5A0`（`setne`、callers 5: 0x4DBB5A/0x4DEA02/0x650698/0x6788A2/0x678985）は「このリンクは無効/未解決センチネルか」述語 [有力]。
  - **bit7 (0x80)**: formID getter/setter ガード `FUN_004133C0`/`0x4133E0`（callers 9/6）。bit7 立ちなら formID を返さず 0（setter も拒否）。formID モードでは object-index の一部、pointer モードではアドレス bit7 ＝ **どちらでも厳密な意味名は確定不能**。候補: 「未解決/一時フォーム」ガード [推定・要深掘]。
  - **bit22 (0x400000)**: `FUN_00415180` のリスト走査チェッカ（本タスクで **callers=1** を実測＝`FUN_00699190` のみ。v177 の「毒判定 0x414EB0 同型」は走査様式の類似であって同一関数ではない）。候補: 「エフェクトリスト内 特殊状態フォーム」[推定・要深掘]。
- **新規 bit グループ発見（v177 未記載）**: decompiled 全走査で **`[base+0x58] & 0x70000`（bit16/17/18）** テストが**多用**されている（L13354 / L15989 / L16601 / L16662 / L16720 / L16783 / L16882）。成立時に **`[base+0x60]` を読む**（`FUN_00418F30`: `test [eax+0x58],0x70000; je; mov eax,[eax+0x60]; ret`）。L16601 はさらに **`>>0x12 & 1`（bit18）** と組み合わせ。→ **bits16-18 (0x70000) = 「script/エフェクト ペイロード有」ゲートで [base+0x60]（ペイロード ptr）を解放** [確定(構造)]。v177 の bit 調査は 7/22/31 のみだったため本タスクの追加。
- **+0x60 の語義**: `FUN_00418F30` が bits16-18 立ち時のみ `[base+0x60]` を返す → +0x60 = **script/effect ペイロード本体 ptr**（+0x58 はその link/flags デュアリティ）[有力]。

### V-185-5 補強: slot[6] は「マジックアイテム型列挙」getter — [確定]（v177「1/4」を訂正）

- **slot[6] 実装の戻り値を全実測**（16-slot MagicItem 系）:
  | クラス | slot[6] impl | 戻り値 |
  |---|---|---|
  | MagicItem（base） | `0x982D85` | **pure-call 共有スタブ**（`push 0x19; jmp 0x98BC6B` ＝ 未定義/pure-virtual ハンドラ） |
  | SpellItem | `0x4D9B40` | **`[this+0x1C]`**（dynamic。SpellItem サブオブジェクトの型フィールド） |
  | EnchantmentItem | `0x7AFE90` | **6**（`mov eax,6; ret`） |
  | AlchemyItem | `0x4059C0` | **7** |
  | IngredientItem | `0x419750` | **8** |
- → **slot[6] = `GetMagicItemType()`（型列挙 getter）** [確定]。base は pure-stub（未定義）、派生は型コードを返す。**v177 §3 の「vt+0x18 戻り値 1/4」はこれの取り違え**（16-slot 系のうち SpellItem は `[+0x1C]` から型を返す動的経路、Enchantment/Alchemy/Ingredient は即時 6/7/8）[訂正]。
- **`FUN_00699190` のディスパッチ**（decompiled 再確認）: `iVar3 = slot[6]()` → `==1`（Spell）で SpellItem dynamic_cast（型表 `0xB03280`→`0xB03800`）→ this(+0x5C) の `vtable[1]` 呼び出し / `==4`（別型）で `vtable[0]` 呼び出し / それ以外は slot[12]（`0x6992A0`＝recast ガード）を経由して `vtable[+0x34]/[+0x3c]`。**1 と 4 は型列挙値の一部**（1=Spell 確定、4=別マジックアイテム型 [推定]）。
- **55-slot primary の slot[6]（+0x18）は別物**: SpellItem `0x418F10` = `lea ecx,[esi+0x24]; call 0x414C70; jmp 0x46C2F0(→0x46B170)` ＝ **destructor thunk**（+0x24 リスト解放→base dtor）。v177 はこの 55-slot slot[6] を「vt+0x18」として参照しており、type 述語（16-slot 系）と**混同していた** [訂正]。
- **+0x64 周辺**: `FUN_0046ACB0`/ENAM→`[+0x64]` は v177 §5.2 で確定済（enchantment リンク）。`FUN_005E4440`（StartItemAction）は `item[+0x64]→+0x18`（EnchantmentItem 埋め込み MagicItem）を `FUN_00699190` へ供給 [再確認]。

（随時追記）

## 3. 結論（3行）

1. **runtime 残点2件が確定**: opcode 0x1C の seta フラグ後続消費は **同一 fetch `0x516830` 内の 0x5168DF**（エラーパス抑制ゲート、bl/out/seta local の 3 条件 OR）で、record[+0xC] は v193 の「double 上位」推定を訂正して **object-reference レコードの formID**（+8=TESForm\* と対）。`[0xB33B00]` は .rdata 文字列 5 種（`TESSaveLoadGame::SaveGameData/SaveQueuedHavokData/SaveQueuedAttachedAnimation/LoadForm/UnloadForm`）と ctor 0x45B300・台帳（session-memory L5819/L9212, b20/b25）で **`TESSaveLoadGame`（セーブ/ロード管理シングルトン）に正式命名確定**。
2. **GT/slot 残点は構造確定**: `base+0x58` のデュアリティ（SCRI ロード=formID → `FUN_0046F020` で解決後 TESForm\*）を再確認し、**新規 bit グループ bits16-18(0x70000)=「ペイロード有」ゲート（[base+0x60] を読む）** を検出。bit31=無効センチネル [有力]、bit7/22 名は部局推定。MagicItem **slot[6]=`GetMagicItemType()` 型列挙**（Enchant=6/Alchemy=7/Ingredient=8/Spell=[+0x1C]）を全実測し v177 の「1/4」を訂正、**+0x5C 埋め込み=Actor の MagicCaster サブオブジェクト**（vtable 0xA6E024、Actor ctor 0x5F13D0 で配置）を同定。
3. **%変数 +0x10 初回書手は不在を再確認**: compiler band 0x4F0000..0x504000 に byte-store [reg+0x10] は 0 件（sanity: C6 総数 241/mod1,2=140 で走査は機能）。登録帯候補（INI 読取 `GetPrivateProfileString` L134586 帯）まで縮小したが特定は [残]。

## 4. 残留不明点

1. **scope1**: %変数レコード +0x10 の**初回書手**（INI/エンジン変数登録帯。compiler band で不在＝登録関数の特定が次手。`GetPrivateProfileString` 周辺 0x46xxxx 帯の深掘り）。
2. **scope4**: `base+0x58` **bit7 / bit22 の正式名**（bit31=無効センチネル [有力] はほぼ確定、bits16-18 は構造確定。bit7/22 は formID/pointer のどのモードで意味を持つかの実測 = B13 実機 .data ダンプ待ち）。
3. **scope5**: slot[6] の型列挙値 **4 の帰属クラス**（1=Spell 確定、6/7/8=Enchant/Alchemy/Ingredient 確定、4=？）。SpellItem の `[+0x1C]` が返す具体的型コード値（=1 か）も B13 待ち。
4. v177 由来の他残点（ExtraStartingPosition node+8 読手 / +0x11D 値5 / +0xE4..F0 元素型 / 馬警報トリガ）は本タスク範囲外（v193 §4 の 3/4/7/8 が継続）。

## 5. 自己検証

- **selftest 4/4 PASS**（着手時・完成直前の 2 回: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 内部整合:
  1. **TESSaveLoadGame 文字列 5 種**を .rdata 実測（0xA3A4E0/0xA3A82C/0xA3A854/0xA3B06C/0xA3B0B8）、xrefs 3 件（0x45B9D3/0x45F7D4/0x45F8F1）が ctor 0x45B300 帯に一致。
  2. **seta 消費 0x5168DF**：push ebx（0x516872）→ [esp+0x18] local → pop ebx（0x5168D3）→ [esp+0x14] の再番号を disasm で追跡、decompiled なしで asm 単独で確定。
  3. **slot[6] 戻り値 4/4 実測**: base=0x982D85(pure-stub) / Spell=0x4D9B40([+0x1C]) / Enchant=0x7AFE90(6) / Alchemy=0x4059C0(7) / Ingredient=0x419750(8) を disasm で確認。
  4. **Actor+0x5C = 0xA6E024**（17 slots）を ctor 0x5F13D0 の `lea ebx,[esi+0x5C]; mov [ebx],0xA6E024` で実測、MagicCaster 0xA75DEC と slot[2,3,4,10,11,16] が一致。
  5. **+0x58 デュアリティ**: SCRI→formID（BOOK loader v177）→ FUN_0046F020 で `[edi+4]` を RTTI キャスト置換（本タスク disasm+decompiled）。
  6. **compiler band byte-store [reg+0x10]=0 件**を独立再走査（C6 disp8/disp32 両形式）。callers(0x4FA910)=2 / (0x4FAA90)=11 も raw E8 で再計測。
- 制約遵守: 新規ファイルは本 1 件のみ。docs/・TODO_re49.md・GT・台帳・git・Supermemory・sync は**編集 0 件**。ESM 読取 0 回。中間物は `%TEMP%\opencode/v198_1..7_*`。

## 6. Android 移植への示唆（実装しない）

- **セーブ/ロード**: `TESSaveLoadGame` は非仮想 POD シングルトン。移植版でも formID マップ（+0/+4）＋バージョン byte（+0x7C、既定 0x7D）＋メニュー callback ptr（+0x1C0）の 3 要素に集約し、スレッド文脈フラグ（+0x18 bit0/bit18、v193）と併せ RAII で entering/leaving を表現する。
- **opcode 0x1C**: %変数参照は「レコード解決 → RTTI キャスト → 失敗時 diagnostic」の 3 段。seta は formID>0 のエラー抑制ゲートとして再現し、record[+0xC] を formID として扱う（double 解釈は誤り）。
- **マジックアイテム型分岐**: slot[6] を `GetMagicItemType()` 列挙（Spell/Enchant=6/Alchemy=7/Ingredient=8/…）として移植し、MagicCaster（Actor+0x5C）が型を見て SpellItem dynamic_cast または共通パスへ分岐する構造を再現。base MagicItem は「未定義＝pure-stub」扱い。
- **form リンクのデュアリティ**: リンク dword は「未解決 formID / 解決済 TESForm\*」の 2 意味。bit31 は無効センチネル、bits16-18 は「ペイロード（+0x60）有」ゲートとして移植し、解決前後でビット解釈が変わることを型で明示する。
