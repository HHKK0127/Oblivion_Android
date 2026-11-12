# v188_magic_wave3_residuals.md — 束H 魔法 Wave3 残束（W3-1 / W3-2 / W3-3 / W3-5）

- 作成日: 2026-10-29 / 担当: 束H サブエージェント（TODO serial 176 / V176・中、分析専任）
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / TODO / session-memory / plan / cache / 既存 tmp/re 成果物は一切変更していない（読取のみ）。git 不使用。ESM 再スキャンなし。
- 対象: Oblivion.exe v1.2.0416（`D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`、7,898,624 B）
- 方法: `tmp/re/re_util.py`（capstone + pefile）+ `docs/Oblivion_decompiled.c`（29,000 KB・Ghidra デコンパイル全文、addr は lower-case 表記）+ `cache/{vtable_index.tsv,named_funcs.txt,call_targets.json}`。中間物は `%TEMP%\opencode\v188_seff_rtti.py` / `v188_menu_scan.py` / `v188_menu_part2.py` のみ。

---

## 0. 結論サマリ

| 項目 | 判定 | 要約 |
|---|---|---|
| **W3-1 Alchemy crafting** | **解明 [確定]** | AlchemyMenu(0x410) のコマンドハンドラ = vt+0x0C `FUN_00595080`（command 0xE）→ 調合実行 **`FUN_00594CA0`**（AlchemyItem 生成 0x412BE0・材料消費・毒/ポーション分岐）。§3 |
| **W3-2 Enchanting creation** | **解明 [確定]** | EnchantmentMenu=0x412（**binary 検証済み** `mov eax,0x412`@0x5A1670）。ハンドラ = vt+0x0C `FUN_005A3160` → 確定経路 0x587550 → 0x5D5200 → 作成実行 **`FUN_005D4EE0`**（EnchantmentItem 生成 0x419570・対象武器/防具へアタッチ）。§4 |
| **W3-3 SEFF スクリプト VM 実行経路** | **解明 [確定]** | TODO の前提「0x6A83D0 以降が SEFF VM」は **誤り**。実体は **ScriptEffect ハンドラ（vtable 0xA76AAC）→ 0x4F9F00/F30/F60 → ScriptRunner::Run(0x517330)** である。§1 に全経路 |
| **W3-5 Poison stack/期限** | **解明 [確定]** | 重ね合わせ: 武器側は二重塗布拒否（0x484E20）/ 対象側は 0x6A1F70 既存除去で「常に1個」。期限: 0x68E670 の elapsed→duration 到達で remove→vt+0x3C。消費: **毒は命中発動時に武器から即除去（single-use、0x5E46C0 内 0x484E50）**。§2 |

---

## 1. W3-3 SEFF スクリプト VM 実行経路 — **解明 [確定]**

### 1.1 前提の訂正（重要）

| 旧記述 | 出典 | 判定 |
|---|---|---|
| 「0x6A83D0 = SEFF tick」「→ 0x6A83A0 (SEFF handler) へ」 | `s98_magic_m15_misc.md` §2 | **誤り**。0x6A83D0 は **効果付与時の HUD メッセージ投稿関数**（下記 §1.4）。0x6A83A0 は **関数先頭でない**（`cache/named_funcs.txt` に無し。0x6A8350 の内部アドレス。0x6A8350=別系統 ctor） |
| 「SEFF 実行系は MGEF SEFF 側: tick 0x6A86F0」 | `v17_scripteffect_noop_report.md` §5.4 | 部分的誤り。0x6A86F0 (baseTick) は magnitude 符号反転の共通前処理であって **VM 実行ではない** |

### 1.2 実際の SEFF 実行系（全新規・hex 証拠付き）

**ScriptEffect ハンドラクラス**（ActiveEffect サブクラス）:

- **ctor `FUN_006A4420`**（base ctor `FUN_0068D7A0` → vtable 設定 `*this = &PTR_FUN_00A76AAC`、[this+0xE]=FUN_00412CC0 結果、[this+0xF]=0）
- **vtable 0xA76AAC**（.rdata、first-slot=0xA76AAC、[-8]=0 / [-4]=0xAC5430）:

| slot | VA | 役割（実証済み） |
|---|---|---|
| vt+0x00 | 0x6A4810 | scalar deleting dtor（本体 dtor=0x6A4580、SEFF コンテキスト [this+0x3C] を FUN_004FB4E0+解放） |
| vt+0x04 | 0x6A44A0 | clone（alloc 0x40 → ctor 0x6A4420 で複製） |
| **vt+0x08** | **0x6A4680** | **毎フレーム tick（dt 引数付き）= ScriptEffect Update/Elapsed 実行** |
| vt+0x38 | 0x6A45F0 | **効果開始時フック = ScriptEffect Start 実行** |
| vt+0x3C | 0x6A4620 | **効果終了時フック = ScriptEffect Finish 実行 → コンテキスト解放** |

- **生成**: MGEF ハンドラファクトリ **`FUN_0068EA50`** @0x68EA50 が `cmp dword [MGEF+0x98],0x46464553 ('SEFF')` → alloc 0x40 → **call 0x6A4420**（decompile L457509-457518）。登録 MGEF は FUN_0055E000 テーブル経由、'PARA' 等は個別 ctor（0x692980 等）。

- **3つの VM 実行ドライバ**（各々唯一の呼出し元は対応する 0x4F9Fxx ラッパ）:

| ドライバ | 内容（decompile L471900-471960） | → ラッパ | 呼出位置 |
|---|---|---|---|
| `FUN_006A45F0` (Start) | `[this+0x38]!=0` 時: ctx=FUN_004FBDC0() → [this+0x3C]=ctx、script=[this+0x20]→vt+4() | **`FUN_004F9F00`**（Start フラグ=1 で Run） | 0x6A4611 |
| `FUN_006A4620` (Finish) | `[this+0x38]!=0` かつ (0xB333B00+0x18 の bit11/bit1 チェック) 時: Run → ctx を FUN_004FB4E0+delete、[this+0x3C]=0 | **`FUN_004F9F30`**（Finish フラグ=1） | 0x6A4654 |
| `FUN_006A4680` (Update/Elapsed) | `[this+0x38]!=0` 時: Run に **dt(引数)を elapsed として渡す** | **`FUN_004F9F60`**（elapsed 引数付き） | 0x6A46A3 |

- **ラッパ 3 処理**（decompile L193538-193602）: `FUN_00516970(...)`（準備）→ `FUN_00517810(...)`（ScriptRunner コンテキスト確保）。
- **`FUN_00517810`**（L208502）: ScriptRunner 状態（0xA4 B）が無ければ確保、既に実行中（[runner+0x14]!=0）なら新規確保 → **`FUN_00517330`** へ。
- **`FUN_00517330` = ScriptRunner::Run [確定]**（L208256、証拠: 内部文字列 `"Nested call to ScriptRunner::Run.  Script \'%s\' attempting to execute script \'%s\'."`）:
  - script[8]=バイトコード長、script[0x29]=「ScriptEffect 状態あり」フラグ → 有効時 `alloc(8)` の SEFF 状態オブジェクトを ctx[0x10] に確保し `{[0]=start byte, [1]=finish byte, [4]=elapsed f32}` を書出し（**v17 §3 の {u8,u8,f32} 状態と一致**）。
  - 実行ループ: **`FUN_00516830`**（statement/block 取得: blockId 初値 0xFFFF、offset、長さ）→ **`FUN_005169A0`**（statement ディスパッチ、v17 §5.2 の entry+0x18 間接 = 全オペコード impl 基盤）を `local_754 < script[8]` の間反復。
  - QueryPerformanceCounter で実行時間計測 → script[0xD] に加算（実行時間ガード）。
- **毎フレーム駆動元**: **`FUN_0068E670` = ActiveEffect::Update(dt)**（L457304）— vt+0x08(dt) を毎フレーム呼出（L457418 `(**(code **)(*param_1 + 8))(param_2)`）。※**vt+0x38 は初回1回のみ**（started フラグ [0x14]==0 ブロック内 L457328）、vt+0x3C は expire 時1回（L457447）。**s92 の「vt+0x38=毎フレーム tick」は本調査で裏取りし、実際は「初回開始フック」**（vt+0x08 が毎フレーム）。
- 0x68E670 の呼出元: 0x68EA21（=0x68EA10 remove-now ラッパ）/ **0x6A235D・0x6A245F（MagicTarget の効果リスト毎フレーム更新ループ）**/ 0x6A2C03。

### 1.3 完全経路（移植用サマリ）

```
配送（touch/self: 0x69BEC0 系 / on-strike: 0x69BA90 / projectile: 0x699900 系）
  → dispatcher FUN_006A2CF0（SEFF 判定: [magicItem+0xc]+0x1c→[+0x98]=='SEFF'
      → source vt+0x1c() skip チェック、vt+0x20 already-applied チェック
      → vt+0x28 Reflect → vt+0x24 Absorb → apply）
  → apply FUN_006A27F0:
      ① FUN_004FBF90(magicItem, caster+0x44, mask 0x2000) = 適用イベント発火
      ② SEFF 時: [MGEF+0x98]=='SEFF' → source vt+0x1c() で skip 判定
      ③ 抵抗/mag. 計算 (source vt+0x20)、0 なら "FULLY RESISTED" で中断
      ④ スタック除去 FUN_006A1F70（後述 §2）
      ⑤ handler = magicItem の vt+4() ファクトリ → 0x68EA50 が 'SEFF' を検出 →
         ctor 0x6A4420（vtable 0xA76AAC）
      ⑥ list 挿入 vt+0x08 / アクティブ化 vt+0x10
  → 毎フレーム: MagicTarget 更新ループ → ActiveEffect::Update 0x68E670(dt)
      ├ 初回:      vt+0x38 = 0x6A45F0 → 0x4F9F00 → 0x517810 → ScriptRunner::Run
      │             （SEFF 状態 [0]=start=1 で script の ScriptEffectStart 条件が成立）
      ├ 毎フレーム: vt+0x08 = 0x6A4680(dt) → 0x4F9F60 → Run
      │             （SEFF 状態 [4]=経過秒 = 累積 elapsed。Update/Elapsed 条件成立）
      └ 期限到達:   vt+0x3C = 0x6A4620 → 0x4F9F30 → Run（Finish）
                    → ctx解放（[this+0x3C]=0）
```

### 1.4 0x6A83D0 の正体（訂正用）

`FUN_0068D740`（vtable ターゲット系・callers 0 件）→ baseTick 0x6A86F0 → 条件で 0x6A84D0 → **`FUN_006A83D0`**: player（0xB333C4）+0x68 MagicTarget を対象に、`[magicItem+0x14]` と MGEF flags bit1/bit2 で 3 文字列を `_sprintf("%s %s %s", ...)` → **`FUN_0057ACC0`（HUD/メッセージ投稿）**。つまり「効果付与時のメッセージ表示」であって VM ではない。

### 1.5 移植への示唆

- 移植対象は「opcode 側 impl」（v17 既出）+「SEFF 状態オブジェクト」（8B: start/finish/elapsed）+「ScriptRunner::Run ループ」（0x516830/0x5169A0 ペア）+「0x6A45F0/620/680 の3ドライバ」+「0x68E670 の tick 呼出順序（start→毎フレーム(dt)→finish）」。
- SEFF 状態は **script[0x29] フラグが立つスクリプトのみ**確保される。Run 時の ctx は FUN_004FBDC0（ctx[0x10] が SEFF 状態）。

---

## 2. W3-5 Poison stack/期限 — **解明 [確定]**

### 2.1 毒のデータモデル（新規確定）

- **毒 = ALCH（AlchemyItem）**。武器への保存: セーブロード switch の **case 0x48** が `FUN_0046B250(..., &PTR_PTR_00B0320C)`（0xB0320C = **AlchemyItem TypeDescriptor**、`cache/vtable_index.tsv` L496-502 と一致）で毒参照を解決し、失敗時 **"Unable to find poison %08X. Poison data will be removed."**（decompile L28069）。
- **武器への塗布**: `FUN_00616CA0`（L382415）: 追加された毒アイテム ref([param+0xA4]) かつ `[weaponEntry+0x64]==0`（= 現在毒なし）→ **debug: `"%.20s poisons current weapon with %s!"`**（L382446）→ **`FUN_00484E20`**（毒セット）→ HUD 更新 0x57B230。呼出元 0x618FC4 / 0x620A27（インベントリ使用系）。
- **塗布ヘルパ群**: `FUN_00484DF0`=毒取得 / `FUN_00484E20`=毒セット（**「既に毒あり（0x41E8E0()!=0）ならセットしない」= 武器への二重塗布・重ね合わせ拒否 [確定]**）/ **`FUN_00484E50`=毒除去**（毒あり時のみ FUN_0041F660 + HUD 更新）。

### 2.2 命中時の適用と消費（0x69BA90 + 0x5E46C0）

**攻撃ヒット解決 `FUN_005E46C0`**（L348906、0x5E46C0 = on-strike 適用の総括関数）:
- エンチルメント側: `[weapon+0x64]`（enchantment）→ **getCost 0x4849C0** と charge 比較。charge 不足なら flag=1（発動不可）、十分なら **FUN_00488AA0 で charge 減算**（M11 の 0x488AA0=減算 と一致）。
- 毒側: `FUN_00484DF0()`（ポイソン getter）で取得。
- 各エントリを `FUN_0069BEC0`（M12 の配送トリガ）で発動 → **毒エントリ発動後 `if (param_2 != 0) FUN_00484E50()` = 武器から毒を即除去 [確定] — 毒は single-use（1 回の命中で消費）**。第2の呼出点 0x60CA8A（`FUN_0060C940` 内、AI/戦闘側）も同一パターン。

**on-strike 適用本体 `FUN_0069BA90`**（L466170、近接 0x6970A0 内 0x69730A / 飛び道具 0x695400 内 0x695617 等で呼出）:
1. `FUN_004152C0(type=2, best=1)` で最高 magnitude の on-strike エントリ（＝エンチルメント）選抜。
2. list 走査: `[entry+0x10]==2`（on-strike）かつ `*entry != 'TELE'` の各エントリで `src vt+0x40` エントリ生成 → **選抜エントリ以外（＝毒エントリ）には flags |= 0xE** → 0x68E170 で magnitude スケール → MagicTarget vt+0 へ適用 → `FUN_006992F0`（範囲内各ターゲットへ反映）。

### 2.3 スタック規則（対象側・一般効果）

- 適用本体 `FUN_006A27F0`（L470603）: 恒久系でなければ **`FUN_006A1F70(item, caster, effect)`** で **同一 magicItem+MGEF+source の既存 ActiveEffect を 0x68EA10 で除去してから新規挿入** → **同効果は常に 1 個（重複乗算なし）[確定]**（呼出位置 0x6A2B36/0x6A2B6E）。
- 恒久系（FUN_0068D9F0() true）かつ MGEF flags & 0x30000 の場合のみ、対象の全 ActiveEffect を走査し magicItem 一致（FUN_0041CAC0）を 0x6A1F70 で除去（L470757-470766）。
- **毒は on-strike 経由でも同じ apply 経路（0x6A2CF0→0x6A27F0）を通る**ため、**同一毒の再命中は「古い ActiveEffect を消して張り直す」＝効果は重ならないが duration はリセットされる** [確定]。

### 2.4 期限（消滅条件）— `FUN_0068E670`（ActiveEffect::Update、L457304）

ActiveEffect フィールド（本関数で確定）: [1]=elapsed(f32 as int) / [7]=duration / [0x0A]=magnitude 種別（4=無限・1=特殊） / [0x14]b=started / [0x15]b=remove 要求 / [0x16]b=finished / [8]=source ref / [3]=magicItem / [0x0D]=icon 参照。

- **タイマ累積**: `elapsed += dt`、ただし `duration < elapsed+dt` になる場合は **elapsed を duration にクランプ**（type 4/1・恒久(FUN_0068D9F0) は無制限累積）。
- **消滅条件（remove 要求 [0x15]=1）**:
  1. **duration と elapsed が一致しなくなる（＝クランプ到達＝ duration 経過）**（L457425-457429 の XOR 比較）
  2. **MGEF flags bit7（0x80）セット** → 即時消滅（1 フレームで消える「一発効果」）
  3. **caster 死亡/消滅**（source vt+0x0c() が true かつ MGEF flags bit28(0x10000000) 未セット）（L457430-457436）
- remove 要求後: player 対象なら HUD/メニュー更新 → **vt+0x3C()** → finished=1。
- 外部からの即時消滅: **`FUN_0068EA10`**（[0x15]=1 + update(0)）— スタック除去 0x6A1F70 もこれを使用。

### 2.5 移植への示唆（毒）

```cpp
// 塗布: weapon.poison = alch;   // 既存 poison ありなら拒否（0x484E20 相当）
// 命中: 0x5E46C0 相当 — enchantment は charge>=cost のみ発動+減算、
//       poison は発動後に必ず weapon.poison = nullptr（0x484E50 相当・single-use）
// 対象側: 同一 (item,mgf,source) は 0x6A1F70 で置換（重複しない）
// 期限: elapsed を毎フレーム加算、duration 到達で remove → finish フック
```

### 2.6 残る小件

- 毒エントリの flags |= 0xE（0x69BA90 内）の読み取り側意味（消費タイミング制御か）は未特定。
- 0x60CA8A（FUN_0060C940、AI 側）での毒消費条件の詳細（近接 AI のみか全 AI hit か）は未確認。

---

## 3. W3-1 Alchemy crafting (INGR→ALCH) — **解明 [確定]**

### 3.1 AlchemyMenu クラス（本調査で新規同定）

- **vtable 0xA6B1B4**（`cache/vtable_index.tsv` L503 `.?AVAlchemyMenu@@`。first-slot=0xA6B1B4、[-8]=0、[-4]=0xAC1E34）:

| slot | VA | 備考 |
|---|---|---|
| vt+0x00 | 0x5936F0 | dtor |
| vt+0x04 | 0x592E80 | （FUN_00592A70 域） |
| vt+0x08 | 0x60CF60 | 共通（他メニュー共有） |
| **vt+0x0C** | **0x595080** | **コマンドハンドラ（§3.2）** |
| vt+0x10 / vt+0x14 | 0x593370 / 0x5934D0 | |
| vt+0x18 / vt+0x1C | 0x890B90 | 共通スタブ |
| vt+0x20 | 0x5934F0 | |
| vt+0x24 | 0x60CF60 | 共通 |
| vt+0x28 | 0x5935C0 | |
| vt+0x2C | 0x593AF0 | |
| vt+0x30 | 0x593C90 | |
| **vt+0x34** | **0x5931B0** | **GetMenuID: `mov eax,0x410` — menu ID 0x410 を binary で検証 [確定]** |
| vt+0x38 | 0x725DA0 | 共通 |

- **ctor `FUN_00593050`**: vtable 書込 @0x593086、**作業用 AlchemyItem を 0x412BE0（AlchemyItem ctor）で生成** @0x59312F（素材を入れるたびに更新される「作ろうとしているポーション」オブジェクト）。
- AlchemyItem ctor **`FUN_00412BE0`**（L11003）: vtable 0xA3217C（+ 0xA32134/0xA32124/0xA32104/0xA320E8/0xA320D4/0xA320C0 = vtable_index の AlchemyItem 7 行と完全一致）、**byte[this+4]=0x28（formType 0x28=ALCH）**、[this+0x78]=-1、[this+0x7C]=0。

### 3.2 コマンドハンドラ `FUN_00595080`（vt+0x0C、decompile L303102）

param = **command ID**（タイル通知値）。入口ガード FUN_0057D2F0()。

| command | 動作 |
|---|---|
| 2 / 3 | 閉じる（FUN_00592FB0 → FUN_00593710） |
| 8 / 9 / 10 / 0xB | **実験器具選択** → グローバル `DAT_00B3B270 = 0..3`（apparatus 種別）→ FUN_005D2070 で UI 更新 |
| **0xE** | **`FUN_00594CA0` = 調合実行** |
| 0xF | `FUN_005932B0`（push 0x410 = メニュー再オープン/初期化） |

### 3.3 調合実行 `FUN_00594CA0`（decompile L302948）— **INGR→ALCH 本体 [確定]**

1. `player vt+0x39C(0x13, ...)` 通知 + **`player[+0x1A2]++` = Alchemy スキル増加**。
2. 作業用 item `[menu+0x94]`: FUN_0044A750 で既存確認、無ければ FUN_0044D950/FUN_00459800（フォーム登録）、**`[working+0x74] = [menu+0x98]`（価格 = FUN_00594F00 の平均コスト: Σ FUN_00470620(素材)/個数）**。
3. **`FUN_00414EB0()` で毒判定** → 毒なら **`"Clutter\\Potions\\PotionPoison.NIF"` + `"IconPotionPoison01.dds"`**、通常は `Potion01.NIF`/`IconPotion01.dds`（モデル/アイコン切替）。
4. **`alloc(0x80) → FUN_00412BE0()` = AlchemyItem 生成**（結果を [menu+0x94] へ）。
5. HUD メッセージ `FUN_0057ACC0(DAT_00B388E8, ...)`、効果音 `FUN_0057DE50(0x12)`、`[menu+0xA4]=3`（状態=完了）。
6. **材料消費ループ（4 スロット [menu+0xB0]）**: 各素材について `player vt+0x100(素材, 0, 1, ...)` で **インベントリから除去**、残数 FUN_004B2460()==1 なら最後の1個消費（ref 解放 + FUN_00484470 + メッセージ 0xFAE/0xFA1）、否则は残数-1 表示。**→ 調合1回につきスロット毎に素材1個を消費**。
7. FUN_00593CD0() でメニュー更新。

### 3.4 周辺経路

- メニュー open 経路: `FUN_0056AE20`（push 0x410 ×2）、`FUN_0058A1C0`（push 0x410 + 0x412 — 作成ステーション系）、`FUN_005D0A20`/`FUN_005D1080`/`FUN_005D2070`（0x5D0000-0x5D5600 = **MagicItemCreation 共通 UI コントローラ**。FUN_005D0A20 は作業用 item のコスト平均 0x594F00 を呼ぶ）。
- `FUN_00447DF0`（0x447DF0）: AlchemyItem ctor と EnchantmentItem ctor の両方を呼ぶ共有ヘルパ（0x44842E/0x4483F3）。AI/スクリプト系のアイテム生成経路と推定（詳細は未解析）。

---

## 4. W3-2 Enchanting creation — **解明 [確定]**

### 4.1 EnchantmentMenu クラス（本調査で新規同定）

- **vtable 0xA6BA0C**（vtable_index L646 `.?AVEnchantmentMenu@@`。[-8]=0、[-4]=0xAC21C4）:

| slot | VA | 備考 |
|---|---|---|
| vt+0x00 | 0x5A1EA0 | dtor |
| vt+0x04 | 0x5A14A0 | （FUN_005A1390 域） |
| vt+0x08 | 0x60CF60 | 共通 |
| **vt+0x0C** | **0x5A3160** | **コマンドハンドラ（§4.2）** |
| vt+0x10 / vt+0x14 | 0x5A1990 / 0x5A1B50 | |
| vt+0x18 / vt+0x1C | 0x890B90 | 共通スタブ |
| vt+0x20 | 0x5A1B70 | |
| vt+0x24 | 0x60CF60 | 共通 |
| vt+0x28 | 0x5A1D10 | |
| vt+0x2C | 0x5A20C0 | |
| vt+0x30 | 0x5A2CD0 | |
| **vt+0x34** | **0x5A1670** | **GetMenuID: `mov eax,0x412` — menu ID 0x412 を binary で検証 [確定]**（H5 の既定を独立裏付け） |
| vt+0x38 | 0x725DA0 | 共通 |

- **ctor `FUN_005A1390`**: vtable 書込 @0x5A13CA、**作業用 EnchantmentItem を 0x419570（EnchantmentItem ctor）で生成** @0x5A143D。EnchantmentItem ctor 領域 `FUN_00419570`（L16296、vtable 0xA33C84 = 55 slots、呼出 0x4483F3/0x5D4FF1）。

### 4.2 コマンドハンドラ `FUN_005A3160`（vt+0x0C、decompile L310842）

| command | 動作 |
|---|---|
| 2 / 0x18 | 閉じる |
| **0xE** | **作成準備**: `FUN_005A2D30`（検証。呼出 0x5A324B は本ハンドラ内部）+ vt+0x14() |
| 0xF | スロット解除（[param[10]] vt+0x10）+ `FUN_005A1740`（push 0x412 再オープン） |
| 0x14 / 0x16 | 武器/防具選択 → `FUN_005D2070(4/5, 0)` UI 更新 |
| 1000–2999 | 効果リスト選択（`FUN_005A3020` / エラー `FUN_00419B10(DAT_00B389B0)`） |
| ≥3000 | `FUN_005A1E10`（選択解除系） |

### 4.3 作成実行 — **確定**

```
確定ダイアログ経路: 0x5866DC（~0x5866xx）→ FUN_00587550 → FUN_005D5200
                    → FUN_005D4EE0 = Enchanting 作成実行
予備経路: FUN_005FAEA0 内 0x5FB58C（script/console 系）→ 0x5D5200
```

**`FUN_005D4EE0`（decompile L339066）[確定]**:
1. `[menu+0x2C]`（選択中ソウルジェム/対象）と `FUN_00438DB0()`（チャージ確認）チェック、駄目なら `DAT_00B389B0` メッセージ + `FUN_005D48A0` で中断。
2. `FUN_00447DF0()` で対象アイテム解決 → vt+0xB4 / FUN_0044D950（フォーム登録）。
3. **formType 0x14(ARMO)/0x16(WEAP) 判定**で武器/防具フラグ bVar3。
4. **`alloc → FUN_00419570()` = EnchantmentItem 生成**（piVar8）。
5. 作業用 enchantment `[menu+0x28]+0x7C` の効果リストを走査し、タイプ適合分岐（武器: 0 可 / 防具: 1,2）で **`FUN_004149A0`/`FUN_00414B90` により新 EnchantmentItem へ効果を複製**。
6. **`piVar8[0xD] = bVar3 + 2`**（2 = 武器 on-strike / 3 = 防具）。
7. **`[enchantable+8] = [menu+0x7C]`**（charge word = UI 設定値を直接書出し）。
8. 効果 0 件（`piVar8[0xB]==0 && [10]==0`）なら中断（メッセージ + 両 item 释放）。
9. **`[target+4] = piVar8` — EnchantmentItem を対象武器/防具へアタッチ**。
10. **ソウルジェム消費**: ref 解放 + FUN_00484470、`[menu+0x2C]=0`、player vt+0x100 でインベントリ反映、効果音 `FUN_0057DE50(0x22)`、`FUN_005D41E0`（UI 更新。呼出 0x5D51A2/0x5D54EC）。

---

## 5. 新規残点（次束への引き継ぎ）

| # | 内容 | 優先 |
|---|---|---|
| R-1 | W3-5: 毒エントリ flags 0xE（0x69BA90 内）の読み取り側意味 | 低 |
| R-2 | W3-5: FUN_0060C940（0x60CA8A）AI 側毒消費の条件詳細 | 低 |
| R-3 | W3-3: SEFF ハンドラ RTTI 名（TD 0xAC5430 の名前文字列）未解決。Finish 実行条件 0xB333B00+0x18 bit11/bit1 の意味未解決 | 低 |
| R-4 | W3-1: 調合の効果選択/合成ロジック本体（作業用 item への効果マージ = 0x59489C 周辺）と FUN_00414EB0 の毒判定基準は未深掘り。apparatus 品質による成功/失敗・倍率は未解析 | 中 |
| R-5 | W3-2: charge 設定値 [menu+0x7C] の計算式（スキル/素材からの算出 = 0x5D2xxx 側 UI）と FUN_00438DB0（ソウルジェム chg 判定）の詳細は未解析 | 中 |
| R-6 | 訂正バッチ候補（**docs/TODO 反映は親セッション承認制**）: ①s98 §2「0x6A83A0/0x6A83D0=SEFF tick」→ 誤り（本ファイル §1.1）②s96「0x69AC50=ActiveEffect 生成」→ 実体=面範囲ターゲット収集（v180 既指出を本調査でデコンパイル確認）③s92「vt+0x38=毎フレーム」→ 実体=開始フック、毎フレームは vt+0x08 ④W3-3 の TODO 文言「0x6A83D0 以降」自体の更新 | 中 |
| R-7 | `FUN_00447DF0`（Alchemy/Enchantment 共有 item 生成ヘルパ）の呼び出し元 0x5FB58C 等（script/console 経路）の同定 | 低 |

## 6. 使用証拠一覧

- `docs/Oblivion_decompiled.c`（本体 29 MB・読取のみ）: FUN_0068E670(L457304) / FUN_006A27F0(L470603) / FUN_006A2CF0(L470821) / FUN_006A1F70(L470186) / FUN_006A4420(L471807) / FUN_006A45F0-F60(L471900-471960) / FUN_0068EA50(L457486) / FUN_00517330(L208256) / FUN_00517810(L208502) / FUN_004F9F00-F60(L193538-193602) / FUN_004FBE00(L195444) / FUN_0069BA90(L466170) / FUN_0069AC50(L465641) / FUN_006970A0(L463460) / FUN_006992F0(L464536) / FUN_006A83D0(L474511) / FUN_00616CA0(L382415) / FUN_00484DF0-E50(L106727-106776) / case0x48 poison save(L28062) / FUN_004152C0(L13845) / FUN_00416410(L14737) / FUN_005E46C0(L348906) / FUN_00593050 域(vtable 0xA6B1B4) / FUN_00595080(L303102) / FUN_00594CA0(L302948) / FUN_00412BE0(L11003) / FUN_005A3160(L310842) / FUN_005D4EE0(L339066) / FUN_00419570(L16296)
- `cache/vtable_index.tsv`（AlchemyMenu 0xA6B1B4 / EnchantmentMenu 0xA6BA0C / AlchemyItem 7 vtable+TD 0xB0320C / EnchantmentItem 0xA33C84）
- `cache/named_funcs.txt`（0x6A83A0 が関数先頭でないこと・各 site の包含関数同定）
- `tmp/re/re_util.py`（callers/find_dword/imm32 走査: 0x4F9F00←0x6A4611 等）
- 既存成果物（読取）: `TODO_re49.md` W3 行(L156-162) / `v180_letter_rows_audit.md` 束H / `TODO_magic.md` Wave3 / `s92/s96/s98` / `v17_scripteffect_noop_report.md` §5.4 / `GT_CHEATSHEET.md` §1
- 中間物: `%TEMP%\opencode\v188_seff_rtti.py` / `v188_menu_scan.py` / `v188_menu_part2.py`

（了 — 4 項目すべて判定完了）
