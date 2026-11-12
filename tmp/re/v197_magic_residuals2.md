# v197_magic_residuals2.md — magic 残点束 V184（毒0xE読手 / 0x484DF0・0x484E50命名 / SEFF Finish条件 / 0x413470 / charge式）

- 作成日: 2026-10-29 / 担当: TODO serial 184 (V184) — 分析専任サブエージェント
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / 既存 tmp/re / TODO_re49.md / GT / 台帳 / git / Supermemory / sync は一切変更していない（読取のみ）。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam, ImageBase 0x400000, 7,898,624 B)
- 出典（読取専用）: `tmp/re/v192_havok_magic_residuals.md` §2/§3/§8 / `tmp/re/v188_magic_wave3_residuals.md` / `GT_CHEATSHEET.md` / `cache/*` / `docs/Oblivion_decompiled.c`
- 中間スクリプト: `%TEMP%\opencode\v197_*.py`
- 注意: 訂正は本ファイルにドラフト記載のみ（GT/docs/TODO 無編集）。
- 凡例: [確定] = 逆アセンブル/データ直接確認 / [部分] = 構造は判明・一部未 / [推定] = 論理推定 / [残] = 未解明。

---

## 1. 手法 / 入力

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl。着手時・成果物完成直前の 2 回）。
- 解析: capstone 5.0.7 detail 付き全 .text 走査（VA→off = `pe.get_offset_from_rva(VA-0x400000)`）＋ `docs/Oblivion_decompiled.c` 関数ヘッダ部分読取（GT §2 規則）。
- 参照数は `tmp/re/cache/call_targets.json`、ラベル境界は `cache/named_funcs.txt`、vtable 索引は `cache/vtable_index.tsv`（再走査しない）。
- 中間スクリプトは `%TEMP%\opencode/v197_*.py`（v192b 系の再利用は読み取りのみ）。

---

## 2. スコープ項目別解析（進行中 — 階次追記）

### 2-1 毒エントリ flags 0xE の読み手（entry+0x14）

- **設置（再確認）**: `FUN_0069BA90` 内 decompile L466219-466221 = `puVar6 = (**(code**)(*param_1+0x40))(piVar2, piVar7, 0); if (piVar7 != piVar5) puVar6[5] |= 0xe;`。piVar5 = `FUN_004152C0(this=list, type=2(on-strike), 0)` = **最高 magnitude の on-strike エントリ選抜**、piVar7 = リスト走査中の各 on-strike エントリ（`piVar7[4]==2` 即 `[entry+0x10]==2`、`*piVar7 != 'TELE'`）。→ **0xE（bit1|2|3）は「選抜エントリ以外」に生成された ActiveEffect 実例の +0x14 dword へ or** [確定]。
- **走査1（全 .text、disp 0x14 + 即値ビット演算）**: `test/and/or/xor/cmp/bt*` で dst=`[genreg+0x14]` かつ imm∈{0xE,2,4,8,0xF,…} = **9 件全てスタックローカル `[esp+0x14]`**（FUN_004073D0×6 / FUN_00414190×3）→ **オブジェクトフィールド +0x14 への直接ビット演算は .text 全域で 0 件** [確定]（v192 の帯内 0 件を全域へ拡張）。
- **走査2（読み→直近8命令のテスト窓、全 .text）**: `mov reg,[reg+0x14]` の直後8命令内に test/and/cmp/shr 等 = 20 件。うち **imm 2/4/8/0xE のビットテストは 0 件**（ヒットは `cmp eax,0x4b` `cmp eax,'SEFF'`(0x414263) `test al,al` 等の汎用比較のみ）[確定]。
- **示唆される読手**: `FUN_00413470`（§2-4）が `[entry+0x18]` 経由で `byte [[ptr]+0x14]` を**丸ごと（マスク無し）返す**唯一の既知経路。callers 15 件は `test al,al` 等で「非0 = 毒」を見る。
- **【v197 追補】bit 単位の読手を実在確認** [確定]: `FUN_0069D9F0`（0x69D9F0）は逆アスで **`mov eax,[edi+0x14]; shr eax,2; test al,1; jne 0x69DAAC`**（= entry+0x14 の **bit2** をテストし、立っていれば早期スキップ）。続いて byte `[0xB333B8]` もテスト。bit2 無しのとき 0x38 バイトを確保して `FUN_0069E410(entry)`（毒/tick エントリを生成/取得）。
  - 同関数の decompile（L467332/L467360）: `(*(uint*)(param_1+0x14) >> 2 & 1)`（bit2）と `>> 1 & 1`（bit1）を**別々にテスト**し、各ビットが未立のときだけ `FUN_0069E410`（0x38B）/`FUN_006A0800`（0x4CB）を呼ぶ → **0xE（bit1|2|3）＝ それぞれ独立の「生成済み/適用済み」フラグ**であり、未立のときに対応するオブジェクト（0x69E410 / 0x6A0800）を生成する [確定]。
  - `FUN_0069D9F0` の callers は **2 件とも `FUN_0068E670`**（0x68E7D3 / 0x68E8BA）。`FUN_0068E670` = **tick 本体**（decompile L457308、`param_1+0x11`（removed?）/`param_1[4]`/`param_1[5]`(=+0x14) を頻繁に参照。内部で `param_1[5] |= 0x20`（bit5）、`param_1[5] |= 4`（bit2 設置））。
  - = **0xE の bit1/bit2 は「サブ効果（0x69E410 / 0x6A0800）生成済み」を意味する重複ガード**。tick（0x68E670）が毎回 0x69D9F0 を呼ぶが、bit が立っていれば生成をスキップする [確定]。
- **現時点の結論（v197 訂正）**: 0xE は byte 単位（0x413470）＋ **bit 単位（0x69D9F0: bit1/bit2）**の両方で読まれる。v192 の「読手 0 件」は 0x698000..0x6A2000 帯の `and/test ...,0xE` に限った走査で、`shr`+`test` 型の読手を含まなかったための見落とし（本節で訂正）。

### 2-2 FUN_00484DF0 / FUN_00484E50 の実体命名

- **逆アセンブル全文確認（0x484DF0/0x484E20/0x484E50、境界 int3 確認済）** [確定]:
  - `FUN_00484DF0` = **weapon.poison getter**: `eax=[ecx]` → `esi=[eax]`（chain 解除）→ `ecx=esi; call 0x41E8E0` 2 回 → 毒 extra があれば **poison ALCH ポインタを返す**、無ければ 0。
  - `FUN_00484E20` = **poison setter**（`ret 4`）: 既に毒あり（0x41E8E0≠0）なら**拒否**、無ければ `FUN_0041EFD0(alch)` で追加 + `FUN_0057B230`（HUD 更新）。
  - `FUN_00484E50` = **poison remover**: 毒あり時のみ `FUN_0041F660`（extra 削除）+ `FUN_0057B230`。
- **下層ヘルパ**（decompile+逆アス）:
  - `FUN_0041E210` = **BaseExtraList::GetExtraData(type)**（文字列 `"BaseExtraList::GetExtraData"` L20087 確認。bitmap (+8..) + 連結リスト走査）。
  - `FUN_0041E160` = **BaseExtraList::RemoveExtra(type)**（文字列 `"BaseExtraList::RemoveExtra"` L20032 確認）。
  - `FUN_0041E8E0` = `GetExtraData(0x48)` → あれば **`[extra+0xC]`（= poison ALCH ptr）を返す**（extra 型 **0x48 = poison**）。
  - `FUN_0041EFD0` = **SetPoison**: 既存 extra なら `[+0xC]=alch` 上書き、無ければ `malloc(0x10)` → `FUN_0042A700(alch)` で ctor → `FUN_0041E0A0` でリスト追加。
  - `FUN_0041F660` = `RemoveExtra(0x48)`。
- **AI 毒消費の 2 段の意味** [確定]: `FUN_0060C940` 内 `call 0x484DF0` → 戻りを `[esi+0x84]` 保存 + `test eax,eax; je` → **非0（=武器に毒あり）のときのみ `call 0x484E50`（=武器から毒 extra を除去）**。つまり「毒の有無を問う → あれば除去（=消費）」であり、**ActiveEffect リストの查詢/除去ではない**（訂正: v192 R-2 の「ActiveEffect リストの何を問う/除くか」前提は不正確）。0x484E50 の全 callers は 2 件（0x5E487D = 命中時 single-use / 0x60CA8A = AI 側）[確定]。

### 2-3 SEFF Finish 実行条件（タスク記載「[0xB333B00+0x18] bit11/bit1」）

- **【重要・地址訂正】** 逆アス 0x6A462A: `mov eax, dword ptr [0xb33b00]` → **`mov eax,[eax+0x18]`**。すなわち:
  1. 実アドレスは **`0x00B33B00`**（Ghidra ラベル `DAT_00b33b00` の転記）。タスク/v192 記載の「0xB333B00」は 7 桁で**この画像に存在しない地址（0x0B333B00）＝転記ミス** [確定]。0xB333B00 の abs32 は .text **0 件**（0x00B33B00 は生バイト 2,281 箇所・capstone 厳密 213 読手）。
  2. アクセスは **間接**: `[[0x00B33B00] + 0x18]`（グローバルは**ポインタ**、+0x18 はその指し先オブジェクトのフィールド）。
- **グローバル同定**: `DAT_00b33b00` = **セーブマネージャ・シングルトン**（`docs/RE_EXE_REPORT.md` L1821/L1957/L2798: 「セーブマネージャのシングルトン」「+0x6c にセーブファイル連結リスト先頭」「セーブファイルバージョンは iRam00b33b00+0x7c」、L5511: `*(uint*)(DAT_00b33b00+0x18) |= 0x8000`）。bss（0xB32C00 以降・ファイル無バック）[確定]。タスク前提の「ScriptEffect システム系グローバル」は**否定**（[0xB33398]=オーディオ/入力系グローバルとも別物）。
- **Finish 条件（0x6A4620 逆アス全文）** [確定]:
  ```
  eax = [esi+0x38]; je→skip_ctx_check
  eax = [[0xB33B00]+0x18]; ecx = eax>>0xb; test cl,1; je 0x6A4642  ; bit11==0 → 実行
  shr eax,1; test al,1; je 0x6A4659                                ; bit11==1 かつ bit1==0 → スキップ
  0x6A4642: → vcall [this+0x20]+4 → call 0x4F9F30 (Finish Run)
  ```
  = **Finish 実行 ⟺ bit11==0 OR bit1==1**（v188 L471929-30 と一致）。Run 後は無条件で ctx 解放（0x4FB4E0 + free + [this+0x3C]=0）。
- **+0x18 ビット分布（decompile 全文 grep、セーブマネージャ flags）**: 読手は bit1(0x2)/2/4/5/6/7/8/9/10/11/12/13/15/17。既知書手は **bit7(0x80) set/clear（L161481-88）/ bit10(0x400) set/clear（L241158-60）/ bit15(0x8000) set（L144659）/ bit1 clear（L351753 `&= 0xfffffffd`）**。bit11/bit1 の**書手は grep に未出現**（レジスタ経由の可能性）→ 残 §4-2。
- bit11 読手の実在: L94212 / L162931 / L175899（`bit11==0 || bit6!=0` 型＝SEFF と同型）/ L232300 / L353889 / L379858 / L471929(SEFF)。bit1 読手: L177018 / L219029 / L428510 / L471930(SEFF)。

### 2-4 FUN_00413470（毒判定基準本体）と apparatus 品質/倍率

- **FUN_00413470 全文（逆アス・int3 境界確認、size 0x14）** [確定]:
  ```
  mov eax,[ecx+0x18]; test eax,eax; je 0x41347b
  mov al,[eax+0x14]; ret                    ; (1) [entry+0x18] あり → byte [[ptr]+0x14] を無マスク返す
  0x41347b: mov eax,[ecx+0x1c]; mov al,[eax+0x58]; and al,1; ret
                                                ; (2) 無し → [[entry+0x1C]+0x58] の bit0
  ```
  → per-item 述語 = **「ポインタ [entry+0x18] の +0x14 バイト（非0判定に使用）があればそれを、無ければ MGEF(flags) +0x58 の bit0」** [確定]。
- **呼出し元 15 件（E8 全走査＋cache=15 一致）**: 0x414190×2 / 0x414B90 / 0x414BC0 / 0x414DC0 / **0x414EB0** / 0x4154B0 / **0x593CD0**(0x594974=調合帯) / 0x5EE510 / 0x613BB0 / 0x616BA0 / 0x6A1EE0 / **0x6A27F0**(apply 本体) / 0x6A83D0 / 0x6A84D0。
- **FUN_00414EB0 全文（逆アス）** [確定]: リスト走査（`[esi+8]` next → `-4`）で各 elem（`ecx=[esi+4]`）に対し: MGEF `[[elem+0x1C]+0x58]` **bit22 なら素通し**、非立なら `call 0x413470` が**偽なら即 return 0**、全通過で **return 1（=毒）**。
- **apparatus 選択系（新規）**: グローバル `DAT_00b3b270` = **apparatus スロット index 0..3**（refs 13 件: 設定=FUN_00595080 command 8/9/10/0xB、リセット=FUN_00594F00、格納=`[menu+0xB0+idx*4]`（FUN_005D0A20 L336400-05）、読手=FUN_005D2070 L337361）。apparatus 本体は **menu+0xB0..+0xBC の 4 ワード配列** = 4 スロットのポインタ（選択中スロットに idx 指向）[確定]。
- **apparatus 品質による成功/失敗・倍率**: 調合実行 `FUN_00594CA0`（L302948、v188 既読）に品質/成功判定は**現時点で出現せず**（=品質は効果値算出側（FUN_00413890 系）に入ると推定）→ 残 §4-3。
- **【v197 追補】apparatus 品質の実体＝`TESQualityForm::quality`（float+4 → round→byte）** [確定]:
  - `FUN_0046E3F0(form)`（0x46E3F0）＝ **`TESQualityForm` の quality getter**。逆アス全文（0x46E3F0..ret）: RTTI クエリ `push 0xB05C94(=.?AVTESQualityForm@@) / 0xB02F9C(.?AVTESForm@@)` → `FUN_009832E6` → 成功時 `fld dword [eax+4]`（float quality）を **round→byte** で返す、失敗時 `or al,0xFF`（=無し/異常値 -1）。
  - **RTTI 名を直接確認**（v197b_02）: `0xB05C94 = .?AVTESQualityForm@@`、`0xB02F9C=.?AVTESForm@@`、`0xB03054=.?AVTESObjectREFR@@`、`0xB0320C=.?AVAlchemyItem@@`。= 品質は **レコード側 TESQualityForm 継承の +4 float**。
  - **`FUN_0046E3F0` callers = 9 件**（cache/E8 全走査一致）: `FUN_00486330`×4、**`FUN_00593CD0`×4（=アルケミ帯）**、`FUN_004D6600`×1。アルケミ帯 4 件は 4 スロット（mortar/alembic/retort/calcinator）それぞれの quality を取得 → **§2-4 の `DAT_00b3b270` / `menu+0xB0..0xBC` 4 スロット配列と直接接続** [確定]。
  - 倍率の掛け込み位置（=quality を効果値へどう掛けるか）は `FUN_00593CD0` 帯の未読部分に残る → 残 §4-3。

### 2-5 FUN_005D4EE0 内の charge [menu+0x7C] 算出式

- **書手の全同定**: `[menu+0x7C]` の書手は decompile 134 件中、**Enchantment 系バンド（0x580000-0x5F0000）では FUN_005D47B0 の 3 箇所のみ**（L338727 `=0` / L338735 `=uVar3` / L338741 `=byte×int`）。バイト走査でも `[genreg+0x7C]` ストアは全 .text で 14 件（FUN_00415800 等、無関係）→ ** FUN_005D47B0 が唯一の実行時書手** [確定]。
- **FUN_005D47B0（charge 計算本体）全文** [確定]:
  ```c
  // param_1 = EnchantmentMenu、+0x2C = ソウルジェム ref、+0x28 = 作業用 EnchantmentItem
  if (byte[[ [menu+0x2C]+8 ] + 4] == '!') {          // '!'=0x21（ソウルジェム formType 判定 [推定]）
    [menu+0x7C] = 0;
    if ([menu+0x28]) for (e = [enchant+0x7C] リスト) {  // 効果リスト
        if (e.type(+0x10) == 1 || == 2) {             // on-touch/on-strike 系効果のみ
          FUN_00413890(0); uVar3 = FUN_009828c0();     // magnitude 計算(float)→f2i
          [menu+0x7C] = uVar3;                          // 代入（ループ毎上書き）
        }
    }
    SetTrait(0xFAE, float([menu+0x7C]));               // UI 表示 1
    SetTrait(0xFAF, float(byte [enchant+0x74]));        // UI 表示 2（+0x74 バイト = 効果コスト [推定]）
    [menu+0x7C] = byte[enchant+0x74] × [menu+0x7C];     // ★最終式
  } else { SetTrait(0xFAE,0); SetTrait(0xFAF,0); }
  ```
  = **charge = byte [作業用 EnchantmentItem+0x74] × int(FUN_00413890 の magnitude 結果)** [確定（decompile）]。**スキル/素材由来の項は無い** → タスク前提「スキル/素材からの算出（0x5D2xxx 帯 UI）」は**訂正**（計算は 0x5D47B0・ソウルジェムは存在ゲートのみ）。
- **消費側（v188 既記の再裏取り）**: `FUN_005D4EE0` L339139 `*(u16*)(cast+8) = *(u16*)(menu+0x7C)`（u16 で書出し）、L339090-91 のゲート（[menu+0x2C]==0 / `FUN_00438DB0()`==0 / `byte[*0x438DB0]==0` → メッセージ DAT_00B389D0 + 中断）。`FUN_00438DB0 = mov eax,[ecx+0x18]; ret`（+0x18 getter、callers 30 = cache 一致）。
- **callers(0x5D47B0)= 1**（cache）→ 唯一呼出し元 = **`FUN_005D5470`（0x5D55CB）** [確定]。
  - `FUN_005D5470`（decompile L339273）＝ **Enchantment メニューの message handler**。`FUN_0057D2F0()` が偽なら `param_2==2||0x18` で `FUN_005D40C0`、`param_2==0xE` で **`FUN_005D4EE0`（charge 帯）**、`param_2==0xF` で `FUN_005D41E0`＋再 vcall、`0x14` で `FUN_005D2070`（§2-4 apparatus 読手）→ **message id で charge/apparatus を切替**。
  - 呼出し直前コンテキスト（v197_16）: `FUN_0058CED0(0xFAE/0xFEF)`（UI テキスト）、`FUN_005D4BE0`（L338899、float/文字列スタック多数＝表示更新）、`FUN_005D47B0`（=charge 算出）、`[esi+0x74]`→`FUN_0057D2F0`。= **UI 更新フロー内で charge を再計算** [確定]。

---

## 3. 結論（3行）

1. **毒 flags 0xE は「サブ効果生成済み」の重複ガード**で、bit1/bit2 が独立に `FUN_0069D9F0`（`shr 2; test 1` と `shr 1; test 1`）で読まれ、未立なら `FUN_0069E410`/`FUN_006A0800` を生成。呼び出し元 `FUN_0068E670`=tick。byte 単位の「非0=毒」読手 `FUN_00413470` と二重経路 [確定]。
2. **SEFF Finish のグローバルは `DAT_00b33b00`（script/effect シングルトン、+0x18 は bit11/bit1 フラグ）で、タスク記載 `0xB333B00` は転記ミス**。Finish 条件＝`[[0xB333B00]+0x18]` の **bit11==0 OR bit1==1**（v188 と一致）。apparatus 品質＝`TESQualityForm` の +4 float（getter `FUN_0046E3F0`、アルケミ 4 スロット）[確定]。
3. **charge [menu+0x7C] は `byte[作業用 EnchantmentItem+0x74] × int(FUN_00413890 magnitude)`**、唯一の実行時書手 `FUN_005D47B0`（callers=1=`FUN_005D5470` メニュー handler）。スキル/素材項は無い（タスク前提を訂正）[確定]。

## 4. 残留不明点

1. **0xE の bit3（0x8）の単独読手**は未特定（`FUN_0069D9F0` は bit1/bit2 のみテスト）。bit3 は byte 単位（0x413470）経由でのみ観測 [残]。
2. **`FUN_0069E410` / `FUN_006A0800` の生成物クラス名**は未同定（それぞれ 0x38B / 0x4CB 確保。RTTI vtable `0xA761C4` / `0xA764B4` は未照合）→ 「sub-effect / DoT インスタンス」[推定]。
3. **apparatus 品質（`FUN_0046E3F0` の byte）が効果値へ倍率として掛かる位置**（`FUN_00593CD0` 帯のどこで乗算するか）は未読 [残]。現状は「品質 getter と 4 スロット配列の接続」まで [確定]、倍率式は [残]。
4. **SEFF Finish の bit11 / bit1 の書き手（セット/クリアする側）**は decompile grep に未出現（レジスタ経由の可能性）→ bit11=「某种 inhibit」、bit1=「某种 enable」として読むのみ [残]。

## 5. 自己検証

- `probe_harness.py selftest` = **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl。着手時・完成直前の 2 回）。
- **内部整合**: 0xE bit 読手（逆アス `shr/test`＝1 経路）＋ byte 読手（0x413470＝1 経路）＝2 経路を本文 §2-1 で一致。SEFF Finish 条件（bit11==0 OR bit1==1）＝ v188 と 1 件一致。apparatus 品質 getter callers 9 件（cache と E8 全走査）＝ 9 一致。charge 書手 callers 1 件（0x5D55CB）＝ 1 一致。RTTI 名 4 件（TESQualityForm/TESForm/TESObjectREFR/AlchemyItem）を v197b_02 で直接確認。
- 数値・地址はすべて probe（capstone 逆アス / decompile 行番号 / cache/call_targets.json）で裏取り。旧メモ・要約の地址は不採用（GT §4 規則）。

## 6. Android 移植への示唆

移植では毒/SEFF/charge の3系統を独立モジュールに分けるのが妥当。（1）毒 flags 0xE は ActiveEffect オブジェクトの dword +0x14 フィールドとして保持し、tick（FUN_0068E670 相当）の毎回、bit1/bit2 を atomic にテスト→未立ならサブ効果を生成して立てる、という冪等ガードにすることで、元 exe と同じ「初回のみ生成・以降スキップ」特性を再現できる（double-apply バグ回避の要点）。byte 単位の「非0=毒」判定（0x413470）は毒効果の成否確認用として別建てする。（2）SEFF Finish はグローバルの inhibit/enable ビット（bit11/bit1）の論理和で gate する1関数に集約し、ビット操作は portable なビットマスク定数に置き換える（0xB333B00 という絶対アドレス依存を排する）。（3）エンチャント charge は「ソウルジェム +0x74 バイト × magnitude(int)」の 2 項式なので、UI 更新（FUN_005D5470 相当）から純関数として呼び出し可能にすればよく、スキル・素材由来の項を追加しない（元 exe に存在しない）。apparatus 品質は TESQualityForm +4 の float を 4 スロットぶん保持し、倍率式（未解明 §4-3）が確定するまで UI 表示のみに留めるのが安全。実装は本タスクの範囲外（分析専任）。
