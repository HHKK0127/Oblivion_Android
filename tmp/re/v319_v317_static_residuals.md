# v319_v317_static_residuals.md — 次束J 段階1（静的部分）: v317 §6 残件の静的解析（L1・日本語報告）

- 作成: 2026-11-07（ai-todo **#85 段階1 = 静的部分**／動的部分は次束以降・本文では扱わない）
- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000）
  - セクション: .text VA 0x401000 → file 0x400 (0x626E00) / .rdata VA 0xA28000 → file 0x627200 (0xD9A00) / .data VA 0xB02000 → file 0x700C00 (0x30C00)。`F(va) = foff + (va - sva)`（`%TEMP%\opencode\v307_common.py` 契約・読取専用）
- 必読踏襲（重複調査せず）: `tmp/re/v317_v315_residuals.md` **§6 新規残件＋§4 各項の [部分確定]/副次 [未確定]**（N1-N9 の確定済み結論は踏襲）/ `tmp/re/SAVE_LOAD_RE_FOLLOWUP_v313.md`（解析制約・5 段階判定・ess 不在前提）
- 判定: 5 段階 **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**。旧結論は削除せず、矛盾は §5 に両論併記＋裁定理由を記録。
- **実 `.ess` は不在**（v313 §1.1 の8系統で確定・本件では再検索せず踏襲）→ B 分類（ess 依存）項目は**一切解析せず** §6 に明記するのみ。
- クリーンルーム: OpenTES4 / xOBSE / COEF 等の逆コンパイル由来コードは参照・流用なし（自ラボ過去 RE 記録との突合のみ）。
- 成果物: **本ファイルのみ新規作成**。既存 tmp/re 成果物・台帳・STATE・TODO・docs・GT・ソース（app/**）・.github/** は無編集。ESM 未接触・git 未実行・`tmp/re/re_util.py` 変更なし・上書き削除なし。
- 中間物: `%TEMP%\opencode\v319_01..13.py/_out`（26 ファイル・本タスクのみ新規）。

---

## 1. スコープ（静的 / ess 分離）

### 1.1 A 分類 = 静的部分（本タスクの実行対象 → 全件解析した）

| 項 | 調査対象 | 結果の行き先 |
|---|---|---|
| A1 | `FUN_0045BB30` の名称/副作用正体（`0x4DD4B0` / `0x4D8A10` / `0x4D8A30` 未同定を含む = v317 N4 残り） | §4-1 |
| A2 | `FUN_00463700` / `FUN_00460BC0` と **bit7 ラッチ（0x463966-0x4639A9）**の意味 | §4-2 |
| A3 | `0x459950` / `0x459990` テーブル境界（[SaveGame+0x74]/[+0x78] 決定ヘルパ）の解釈 | §4-3 |
| A4 | `FUN_005F0310` 述語の正式名推定根拠・**vtable+0x4C getter の正式名**（文字列なし＝呼び手/RTTI/挙動からの分類） | §4-4 |
| A5 | 保存レコード `+9` の typeID vs 0x7D 差の**静的面**（比較構造のみ） | §4-5 |
| A6 | v317 §6 のうち ess 非依存で静的解析可能な残り（§6-5..11 ほか） | §4-6 |

### 1.2 B 分類 = ess 依存（**解析禁止**・[未確定] ポインタのみ・§6 に明記）

| 項 | 内容 | 扱い |
|---|---|---|
| B1 | N8 の tail `Δ=0x1C/0x2C` **中身** | 「実 .ess 待ち（#82 依存）」— 深推測禁止 |
| B2 | 保存レコード `+9`（および tail header byte3）の**実値**とその動的意味 | 同上（静的構造のみ A5 で扱う） |
| B3 | v317 §7 の実在面（実ファイルの bit3/bit7 有無等） | 踏襲・再検索なし |

### 1.3 帯遵守（深解析した自帯／非侵入帯）

- **深解析した自帯（許可帯すべて）**: `0x430000-0x431000`（走査のみ該当なし）/ `0x452000-0x467920`（459950/459990/45B550 期化/459AF0/45BDE0/45ADC0/45DE30 系/45E3D0 系/460BC0/461680 周辺/462080 系/463700/4637C0(=LoadForm)/464440/465860 LoadGame/formatter 0x453A90 起点窓 0x456680-0x456760/46AC30-46AC50）/ `0x4D8000-0x4F2000`（4D8A10/4D8A30/4D7AF0/4DD4B0 全文/4DC070 系窓/4D1EB0）/ `0x5E1000-0x5E2800`（5E0260/5E1F40/5E1F60/5E7D00）/ `0x5F0000-0x5F1000`（5F0310/5F0410/5F0280-5F0500 heads）。
- **.rdata/.data 文字列・テーブル帯（読取）**: formType 名テーブル `0xB05E04`、RTTI TD 0xB02F9C 等、文字列 0xA397BC/'CHANGE_OBJECT_EXTRA_OWNER' 0xA397C8/'TESSaveLoadGame::LoadForm' 0xA3B06C/'ResetObject()' 0xA3A524・0xA3A560/'ExtraDataList::GetSaveSize' 0xA3507C/version 文字列 0xA3A8E8・0xA3B450 ほか、`tmp/re/vtable_map.txt`（読取）。
- **v318 帯は非侵入（深解析禁止5帯: 0x404000-0x406000 / 0x44F000-0x452000 / 0x51B000-0x51C000 / 0x570000-0x574000 / 0x500000-0x504000）**。本件で帯外に触れたのは以下 **浅層のみ**（VA・文脈10-20行）: 0x446B80（先頭16行）/ 0x44EF19 文脈・0x44EE00 head と callers VA / 0x4CBD30 callers / 0x4CBECC 文脈 / 0x453A90 formatter 全体（頭起点走査＝許可帯内）/ 0x5F0410 callers VA / 0x465860 LoadGame（許可帯内）/ 0x40F3C1・0x44F058・0x507896・0x5BA629・0x500CBB・0x4CFF74・0x4CD9A1・0x65B813（VA 記録のみ）。
  - ※ `0x44F058` と `0x507896` は **v318 禁止帯内**（callers 一覧の VA として機械的に現れただけで、文脈展開は一切していない）。

---

## 2. 使用 probe 一覧

実行形: `chcp 65001` → `python $env:TEMP\opencode\v319_<n>.py *> $env:TEMP\opencode\v319_<n>.out`。依存は `%TEMP%\opencode\v307_common.py`（読取専用・変更なし）。`tmp/re/re_util.py` は不使用（変更なし）。raw E8=`va+5+rel==target`（callers_of）。

| script | 対応項 | 内容 / 主結果 | 最終 out 行数 |
|---|---|---|---|
| v319_01 | A1 | FUN_0045BB30 全文（0x45BB30-0x45BDCF）＋ callee 0x4DD4B0/0x4D8A10/0x4D8A30/0x5E0260/0x5E1F40/0x5E1F60 本体＋ callers | 984 |
| v319_02 | A1 | RTTI TD 文字列（TESForm/REFR/Actor/CELL/WRLD/MobileObject ほか）＋4DD4B0 尾部＋callers＋4C97F0/4D7AF0/4212E0/41F7F0 本体 | 485 |
| v319_03 | A2 | FUN_00463700 全文（0x463700-0x4639D8・**0x4637C0 で SEH prologue 分岐を検出**）＋FUN_00460BC0 全文＋callers＋0x464440/0x4646D9 文脈 | 527 |
| v319_04 | A2 | 境界 raw bytes（0x4637B0-0x4637C4: `5e e9 …` で 0x4637C0 **ゼロパディング**実証）＋callers(4637C0/464440/…)＋fn0x4637C0 尾部＋文字列（LoadForm/Load Error）＋**formType 名テーブル 0xB05E04 全走査**＋LoadGame 0x466695 文脈 | 214 |
| v319_05 | A3 | 459950/459990 精密デコード（jbe/jb）＋45BA53 jbe＋テーブル期化 0x45B550-0x45B640＋vtable 0xA39044 スロット＋0x446B80 浅層＋**bit7 リーダ許可帯走査 = 0 件** | 557 |
| v319_06 | A4 | 0xA397BC/'Owner(%i)'・formatter Owner 窓（頭 0x453A90 起点）＋call [reg+0x4c] 走査＋0x5F0310/5F0410 vtable スロット走査（=0 件）＋5F0410 本体＋family heads＋vtable_map まとめ | 124 |
| v319_07 | A5 | 保存/ロード帯のバイト `+9` アクセス全走査（**0 件**）＋`cmp *,0x7d/0x7c/0x7b` 2 件＋tail==0 保存窓＋version 文字列 4 点 | 89 |
| v319_08 | A6 | fn bounds＋**FUN_00459AF0 全文（arg2 軌跡）**＋0x4CBECC 呼び手探索＋45BDE0 文字列（ResetObject!）＋45ADC0 の [esp+0x30] 追跡 | 284 |
| v319_09 | A4/A6 | Owner 窓（頭起点）＋TES クラス vtable 窓（※この回は +0x48=slot18 で測定 → v319_10 で +0x4C=slot19 に**訂正**）＋0x46AC30 本体＋`cmp 0x7d` 文脈2点＋5F0310 呼び手3文脈＋0x464440 連続性＋4CBECC＋45ADC0 基点＋テーブル書手 hunt＋4DD4B0 尾部 | 697 |
| v319_10 | A4 | **vtable +0x4C（index19）再測**・TESForm/REFR/CELL/Actor/Character/WorldSpace/DOOR の実 target と本体＋45B700＋LoadGame byte3 出所＋書手文脈＋浅層2件＋5F0310 caller3＋文字列5点（※初回は find_head(None) で EXIT=1 → [4] に None ガードを追加して再実行＝EXIT=0） | 515 |
| v319_11 | A4/A6 | +0x4C オーバーライド本体（5E7D00/4D1EB0/60E360＝**CCラン不足型 head**検出）＋LoadGame byte3 源（0x46679C）＋書手文脈6点＋452800＋0x44EF19 浅層＋4DC22C 文脈＋文字列（ExtraDataList::GetSaveSize!） | 521 |
| v319_12 | A5/A6 | LoadGame のレコードフレーム [esp+0x1c..0x27] 書込み/サイズ push/45bb00 呼び出し列挙＋0x453530 文脈＋item8 鎖上位（0x44EDE0 が誤 head と判明） | 111 |
| v319_13 | A5/A6 | 0x465C88 の 12B 仮想 read 文脈＋memcpy/lea/frame 書込み列挙＋**fn 0x44EE00（実ヘッド・CC=2）と callers=3** | 42 |

全13本・最終実行 **EXIT=0**（§9）。

---

## 3. 各項の判定表（5 段階）

### 3.1 主判定

| # | 残点 | 判定 | 一句话裁定 |
|---|---|---|---|
| A1 | FUN_0045BB30 の名称/副作用正体 | **[部分確定]** | 副作用の正体は **[確定]**: 「親（CELL/WRLD）取得 → **0x4DD4B0=親セル決定・接続ルーチン**（位置>>12 の grid 検索 0x4EFE80＋0x4D35D0 接続）＋ 0x4D8A30/0x4D8A10=位置/float フィールド書込み＋[form+0x3c]+0x54..5C への位置転記＋TLS+0x185 一時クリア/復元」を実行する再配置ヘルパ。呼び手②は **FUN_0045BDE0 = `ResetObject` [確定(文字列)]**、③ pass2（戻り値棄却）。**正式名称 [推定]**（文字列なし） |
| A2 | FUN_00463700 / FUN_00460BC0 / bit7 ラッチ | **[確定]**（463700 の名称のみ [未確定]） | **bit7 ラッチの含有関数は `FUN_004637C0 = TESSaveLoadGame::LoadForm [確定(文字列)]`（v317 の「FUN_00463700」は find_head 誤答による帰属誤り＝§5-C1 訂正）**。ラッチは 460BC0(payload 適用)+vfunc+0x54 の間だけ立て元値復元＝適用中ラッチ [推定]。0x460BC0 = **payload 適用/スキップ両用ディスパッチャ [確定(命令)]**（REFR=読取→ID解決→位置適用／CELL=版次 cursor だけ進める／非該当=無操作）。FUN_00463700 = グローバル諸システムのクリア連鎖＋this クリア（末尾 `jmp 0x4F9DD0`）で fn 0x464440 から2回呼出 [確定(命令)]・名称 [未確定] |
| A3 | 0x459950 / 0x459990 テーブル境界の解釈 | **[部分確定]** | 両者とも同一テーブルクラス（vtable 0xA39044、期化 0x45B587/0x45B606）。**境界=[t+0xc]=要素数(count)**（投入ルーチンが `count=index+1` で更新）[確定]。**比較が1件ずれ: 459950=`jbe`（idx≤count＝1件多く許容）/ 459990=`jb`（idx<count）**[確定]。created(≥0xFF000000)は 446B80=`id≥0xFF000000` で素通し [確定]。越界時の安全性（array[count] 読取が cap 内なら 0）は [推定] |
| A4a | FUN_005F0310 正式名 | **[未確定]** | 構造・呼び手分類は [確定]（§4-4(a)）、**名称を裏付ける文字列/スロットなし [確定負]**（vtable スロット走査=0 件）。分類 [推定]: 「スクリプト(type1..5)所持 ∨ flag bit6 時の魔法効果(ActiveEffect/Reanimate, accum≥0x1E)所持 ＝ 実行時変更対象」述語 |
| A4b | vtable+0x4C getter の正式名 | **[部分確定]** | slot19(+0x4C) はクラス別 override: **TESForm/TESWorldSpace/TESObjectDOOR=0x46AC50（(mask&1)?4:0）/ TESObjectREFR=0x4E2BF0 / TESObjectCELL=0x4D1EB0 / Actor=0x5E7D00 / Character=0x60E360（0x5E7D00 へ jmp 別名）**。REFR 実装は mask ビット＋条件で定数 1/4/6/7… を合算し **sub-call に `ExtraDataList::GetSaveSize`(0xA3507C→0x421340)** を持つ → 分類 [推定] = **「mask 指定フィールドのセーブ時バイトサイズ算出（u16 返）GetSaveSize 系仮想」**。formatter は `getter(0x80) − 基準 word[esp+0x14]` を `Owner(%i)` 表示（quiet='CHANGE_OBJECT_EXTRA_OWNER'）。**基準 word の出所と %i の厳密意味は [未確定]** |
| A5 | 保存レコード +9 の意味差（静的面） | **[部分確定]** | **+9 byte = 「セーブ version byte」説が命令整合的 [推定（根拠4点）]**: ①tail==0 で必ず `[SaveGame+0x7c]`(=0x7D) が入る（0x46534A-0x465351）②tail≠0 では旧 tail header byte3 を踏襲（0x46536D-0x46537E）③その byte3 は LoadForm で **version 閾値 `cmp …,0x5b` 比較**（0x459C03）④0x7D は version 文字列（0xA3A8E8/0xA3B450）と LoadGame の `[+0x71]=0x7D→[+0x7c]` 初期化（0x46699E-A2）で現在 version。旧「typeID」呼称（v312件4/v315 R3/v317）とは**両論併記**（§5-C4）。ロード側は record+9 を tail header byte3 へ再_emit（0x46679C-0x4667A3）[確定]。**実値照合は .ess 待ち（§6-B2）** |
| A6-1 | v317 §6-5: FUN_00463700 と 0x460BC0 の同定 | **[確定]**（名称のみ [未確定]） | A2 に統合（上記）。v317 §6-5 の「呼び手 head 0x464440 との関係（2呼出）も未説明」→ **関係は解明**: fn0x464440（LoadGame 系・callers 0x466889=LoadGame 帯）が 0x464499/0x4646D9 の2呼出 [確定] |
| A6-2 | v317 §6-6: SaveGame+0x18 bit7 の意味 | **[推定]**（副次 [確定負]） | 許可帯全域で **`test/cmp *,0x80` 系リーダ = 0 件・shift7 リーダ = 0 件 [確定負]** → 意味は書込み側文脈のみ: **LoadForm 内 payload 適用中のみ立つ一時ラッチ（再入/適用中検知）[推定]**。他 bit（bit0/bit18 別名・bit6・bit9・bit12）との対応整理は残る |
| A6-3 | v317 §6-7: 459950 テーブル境界解釈 | **[確定] へ格上げ** | A3 に統合（上記）。v316 §6-9「[+0x74] の投入元（誰が書き込むか）未特定」→ **投入 find-or-add ルーチン = FUN_0045DE30 内 0x45E0EF-0x45E141 [確定(命令)]**（v309 確定の created base objects 検証経路と整合）＋別系 0x45E3D0（u8 索引→ID 復元＋投入）＋クリア系 fn 0x462080（LoadGame 側 0x4644AA から呼出） |
| A6-4 | v317 §6-8: 45AF0 連鎖の発動文脈 | **[部分確定]** | 鎖 = **fn 0x44EE00（実ヘッド・CCラン=2 で find_head 誤答）→ 0x4CBD30 → 0x459F30（`push 1`）→ 0x459AF0** [確定]。fn0x44EE00 の callers = 0x44F058 / 0x507896 / 0x5BA629 = **うち2件は v318 禁止帯** → 上位工程の同定は v318 lane 照会事項（§7-R7） |
| A6-5 | v317 §6-9: 459F30 の arg2（push 1）契約 | **[確定]**（意図は [推定]） | **arg2≠0 → CELL(0x30) ブロックを除外し {0x31 REFR, 0x32 ACHR, 0x33 ACRE} のみ走査 / arg2==0 → CELL も走査** [確定(命令)]（0x459BE1 `cmp byte[esp+0xa8],0 / jne 0x459d07` と 0x459BFA `cmp cl,0x30`）。fn0x459F30 の `push 1` = 「参照系のみ照合」[推定] |
| A6-6 | v317 §6-10: caller1(45ADC0) の列挙基点 | **[確定]** | `[esp+0x30] = ebx = 0x452800 の戻り（ノードptr）`（0x45AF1C-0x45AF20 で代入、0x45AFD0 で読出し→`mov ebx,[eax]`）。0x452800 = ノード {next@0, byte@4, dword@8} を pop するイテレータ前進処理 [確定(命令)] |
| A6-7 | v317 §6-11: 0x464440 窓と find_head の関係 | **[確定]** | **境界エラーなし**。0x464440-0x4646D9 間に ret/尾 jmp/競合 head なし（線形デコード実測・head=False 候補はすべて偽）→ find_head(0x4646D9)=0x464440 は正しく、v316 の窓 0x464440..0x46458A は**部分窓**（off-by-pad 候補ではない） |

### 3.2 5 段階集計（主判定 13 項: A1, A2, A3, A4a, A4b, A5 ＋ A6-1..A6-7）

| 判定 | 件数 | 項番 |
|---|---|---|
| [確定] | **6** | A2, A6-1, A6-3, A6-5, A6-6, A6-7 |
| [部分確定] | **5** | A1, A3, A4b, A5, A6-4 |
| [推定] | **1** | A6-2 |
| [未確定] | **1** | A4a |
| [確定負] | 0（主判定） | 副次: bit7 リーダ0件 / 5F0310 vtable スロット0件 / 帯内バイト `+9` アクセス0件 / 4CBECC の E8 呼び手0件 |

（合計 13 項）

---

## 4. 根拠（VA/命令/caller-callee）

### 4-1. A1: FUN_0045BB30 — 副作用正体 [確定]・名称 [推定]

**(a) 全体構造（0x45BB30..0x45BDCF・`ret 8`・head 検証済み）**
```
45BB33: mov eax,[0xba9de4] / 45BB3D: mov ecx,fs:[0x2c] / 45BB44: mov eax,[ecx+eax*4]
         → TLS ブロック取得（fs:0x2c = TEB ThreadLocalStoragePointer）[確定]
45BB47: mov dl,[eax+0x185]（保存）→ 45BB57: mov [eax+0x185],0（一時クリア）
45BB4F: mov esi,[esp+0x58] = arg1（form/refr）/ bl = arg2（byte）
45BB64: vfunc+0x190(form) → al / je 0x45bc26（代替経路）
45BB7E: call 0x5e0260        ; [form+0xf8]≠0 述語
45BB8D: call 0x5e1f60 → edi  ; ★親 CELL getter（下記(b)）
45BB96: call 0x5e1f40 → ebp  ; ★親 WRLD getter（下記(b)）
         edi==0 ∧ ebp==0 → 0x45bdb6（即終了）
45BBB8: vfunc+0xf4(form,&pos) → 12B → 45BBD1: call 0x4d8a30(form,pos)
45BBE5: vfunc+0xf0(form,&pos2) → fld [eax+8] → 45BBF0: call 0x4d8a10(form,f)
45BBF5: bl=arg2 / test / je 0x45bc05
45BC01: or dword[eax+0x18],0x20   ; ★[this+0x18] bit5 ラッチ（eax=[esp+0x14]=保存した ecx=this）
45BC08: call 0x4dd4b0（cdecl 3引数: push ebp(親WRLD) / push edi(親CELL) / push esi(form)）
45BC18: and dword[eax+0x18],0xffffffdf（bit5 復元）
45BD04〜: [form+0x3c]≠0 なら vfunc+0x174 の12Bを [form+0x3c]+0x54/58/5C へ転記
          RTDynamicCast(form, 0, TESObjectREFR, MobileObject, 0)@0x45BD26 → 0x65a2c0 …
          45BD8B: call 0x4d7af0（36B トランスフォーム構築）→ 36B コピー → 0x897a20
          45BDB1: fldz … call 0x707370([form+0x3c], 0.0, 0)
45BDC5: mov [edx+0x185],cl（TLS 復元）→ ret 8（al=成否）
```
- **bit5 ラッチの対象は `this`（[esp+0x14] = 先頭で保存した ecx）** [確定]（v317 の "[obj+0x18]" はこの ecx）。arg2==0（pass2）ならラッチは**しないが 0x4DD4B0 は呼ぶ** [確定]（`je 0x45bc05` がラッチ省略のみ）。

**(b) callee 同定 [確定(命令)]**
- `0x5E0260`: `cmp dword[ecx+0xf8],0 / setne al / ret` = **親（parent）所持述語**。
- `0x5E1F60`: `[ecx+0xf8]` を取り、`byte[ptr+4]==0x30` かつ `0x4C97F0`（=[+0x24]&1）なら返す = **親 CELL getter**（**formType 表 0xB05E04: 0x30=CELL**）。
- `0x5E1F40`: `[ecx+0xf8]` を取り、`byte[ptr+4]==0x35` なら返す = **親 WRLD getter**（0x35=**WRLD**）。
- `0x4C97F0`: `mov al,[ecx+0x24]; and al,1; ret` = **[+0x24] bit0 述語**（同形 setter 0x4C9800・bit7 getter 0x4C9820・bit5 getter 0x4C9830）。
- `0x4D8A30`: `[ecx+0x2c..0x34]` へ 3 float 書き込み → `vfunc+0x40(this,4)` 通知 ＝ **位置セット＋通知**。
- `0x4D8A10`: `[ecx+0x28]` ← float → 同じく `vfunc+0x40(this,4)` ＝ **単 float フィールドセット＋通知**。
- **`0x4DD4B0`（正式名 [推定]: 親セル決定・接続）**: (form, 親CELL, 親WRLD) を受取り — `0x4C97F0` で親検証 / 現親≠新親で `0x4F03D0`+`0x4D35D0` による張替 / 親 WRLD 経路では `vfunc+0x174` の位置を `sar …,0xc`（4096 単位 grid 変換）して **`call 0x4efe80(world,x,y)`（該当 grid の CELL 探索）→ `0x4d35d0(cell, form)`（接続）** / `[0xb333a0]` へ `0x43f8c0(1, …)` 通知 / `[form+0x58]`（スクリプト）vfunc+0x20 通知。RTTI 側の TD は TESForm/TESObjectREFR/TESObjectCELL/TESWorldSpace/Actor（0xB02F9C/0xB03054/0xB03038/0xB0301C/0xB03070）。
- **`0x4D7AF0`**: `rep movsd` で 0xB26DC4 既定値（9 dword=36B）を出力へ、vfunc+0x190 に応じ `[this+0x20/0x24/0x28]` 回転から 0x70FD30/0x70FD80 系でトランスフォーム構築 ＝ **36B 位置/回転セット構築**。

**(c) 呼び手の正体**
- **`FUN_0045BDE0` = `ResetObject` [確定(文字列)]**: 文字列 `'Trying to reset object %08X %s, but the reference has no save parent cell.'`(0xA3A560, 0x45BE9F) / `'Form could not be found in file during ResetObject() call.\n'`(0xA3A524, 0x45BFFE)。45BB30 成功時 `and [esp+0x20],0x7ffffff3`（**changeFlags bit2|bit3|bit31 をクリア**）@0x45BE49。callers(0x45BDE0)=5: 0x45C48D / 0x45F20E / 0x45FB1D / **0x4665B9（LoadGame 帯）** / 0x4CBFCA（帯外・VA のみ）。
- pass2（0x461DE7, `push 0`）は戻り値棄却（v317 踏襲）。
- **名称 [推定]**: 「REFR 再配置（位置＋親の再同期）副作用ヘルパ」。文字列なし [確定負]。

### 4-2. A2: FUN_00463700 / FUN_00460BC0 / bit7 ラッチ — [確定]

**(a) ★関数境界の訂正（v317 N7(c) の帰属誤り）**
- raw bytes（0x4637B0-0x4637C4）: `e8 … 5e e9 10 66 09 00 | 6a ff 68 3e …` ＝ **`call 0x4F9FD0; pop esi; jmp 0x4F9DD0`（尾 jmp で終端）→ 直後に 0x4637C0 `push -1`（SEH prologue）・パディング0 バイト** [確定]。
- 0x4637C0 側は独立 prologue（`push -1 / push 0x9ae63e / fs:[0] / sub esp,0x12c / xor [0xb30aac]` cookie）と対応する epilogue（0x4638F6-0x46391F: `fs:[0] 復元 / xor ecx,esp / call 0x9811e2 / add esp,0x138 / ret 4`）。
- **`func_head(0x4637C0)=False`（CC ラン=0）→ `find_head(0x463966)=0x463700` が v317 の誤答源**。`callers(0x463700)=2 (0x464499,0x4646D9)` と `callers(0x4637C0)=4 (0x463E2F,0x463E8B,0x46494A,0x4CD9A1)` は別系統。
- → **bit7 ラッチ（0x463966-0x4639A9）の含有関数 = FUN_004637C0**。呼称は本件で **`TESSaveLoadGame::LoadForm` [確定(文字列)]**（0x46385E `push 'TESSaveLoadGame::LoadForm'` → 0x401020 enter / 0x4638F1・0x463A7B 0x401040 leave）。

**(b) FUN_004637C0 = LoadForm の命令列（本件新規の完全像）**
```
guard: 同一スレッドなら byte[esi+0x18]&1、異スレッドなら dword[esi+0x18]>>18&1（＝「load 中」系フラグ）
46382B: eax=[edi+0xc]（formID）→ map find(0x55e000) → ebx=tail ptr
46385E: trace enter 'TESSaveLoadGame::LoadForm'
46386D: [esi+0x14]=ebx → [0xb33b00]+0x14 経由で旧 tail header 4B 読取（b0,b1,b2,b3）
463881: al=[edi+4]（live formType）/ cl=[esp+0x1a]（★tail b2 = 保存時 formType）
46388C: cmp al,cl → 不一致: ★formType 名テーブル 0xB05E04 で sprintf
        ('Load Error: Form with ID %08X was saved with form type %s, but currently has form type %s. Its loading will be skipped.' 0xA3AFF0)
        → 0x452df0（entry 削除）→ trace leave → 戻り偽
463922: 一致: 0x453530(merge) → 0x45a140(this,…) → ebp=結果
        [esp+0x28]=formID / [esi+0x80]=&（暫定ポインタ）
46395C: eax=[esi+0x18] / bl=旧bit7（shr7&1）
463966: or eax,0x80 → [esi+0x18]   ★bit7 セット
463976: or [edi+8],0x200000        （record 側 bit21 永続セット）
46398B: call 0x460bc0(this=esi, edi, ebp)   ★payload 適用
463990: vfunc+0x54(edi, ebp, 0)
46399C: test bl,bl → 4639A0 or / 4639A9 and …ffffff7f  ★元値へ復元
4639B0: cl=[esi+0x71] → 4639BE mov [esi+0x7c],cl  ★version のコミット
4639F9-0x463A86: new(0x10) ノード {form, ?, ?, b3} → 0x5a6ab0 で [+0x1c] へ追加
        消費長不一致なら 'LoadGame() call did not properly empty buffer…'(0xA3AFA0)
        → 0x452230 → unlock → al=1
```

**(c) FUN_00460BC0 = payload 適用/スキップ両用ディスパッチャ [確定(命令)]（`ret 8`・callers=2: 0x46398B / 0x466695）**
- `RTDynamicCast(arg1, TESForm→TESObjectREFR)` 失敗 → `TESForm→TESObjectCELL` → **CELL なら version 依存で cursor を進めるだけ**（`cmp [0xb33b00]+0x7c,0x5b` 境界: ≥0x5B で flags&0x4000000→+4 / &0x2000000→+6、<0x5B で bit2∧bit4→+0xc / else→+2）→ いずれも**読取・適用なし** [確定]。
- 非 REFR 非 CELL → 0x46101e（無操作）。
- **REFR**: `[0xb33b00]+0x14` cursor から payload を memcpy（0x24/0x2c/0x1c/4/6 ブロック）→ ID を **459950/459990** で解決 → RTTI 分岐（CELL/WorldSpace/0xB03248）→ grid 変換 0x4EFE80 → **`0x4d89a0`(回転)/`0x4d8a30`(位置)** → `0x45e990(this, …)`。
- 注: 0x466695 側（LoadGame 帯）は **bit7 ラッチを伴わない**（`[ebp+0x80]` のみ set/clear）→ **ラッチは LoadForm 固有** [確定]。

**(d) FUN_00463700 [確定(命令)・名称未]**
- 0x463700-0x4637BE: `0x67cf00(0xb3bdb0)` / `0x683500([0xb3bf80])` / `0x4be420` / `0x4bd8c0` / `0x65e800([0xb333c4]=PlayerCharacter)` / `0x45c470(this, &[0xb33a98]+0x74)` / `0x461580(this)` / `0x443300([0xb333a0])` / `0x677280・0x67ae90(0xb3bd00)` / `[0xb33398]+0x24 → 0x6ab160・0x6ab420` / `0x5c16d0 / 0x5a8ba0 / 0x57c0a0 / 0x4f9fd0` → 尾 `jmp 0x4f9dd0` = **グローバル諸システムのクリア/停止連鎖＋this クリア系**。callers=2（0x464499, 0x4646D9＝いずれも fn 0x464440 内・`mov ecx,ebp`）。**bit7 ラッチとは無関係**。

**(e) bit7 の総合裁定**: 許可帯全域スキャンで **bit7 リーダ（test/cmp 0x80 / shift7 on [...+0x18]）= 0 件 [確定負]** → 意味は書込み側のみから **「LoadForm の payload 適用＋vfunc+0x54 中だけ立つ一時ラッチ（適用中/再入検知）」[推定]**。fn0x4637C0 内の `[esi+0x80]`（formID ポインタ暫定格納）、`[esi+0x71]→[esi+0x7c]`（version ステージング→コミット）も本件で新規に [確定]。

### 4-3. A3: 0x459950 / 0x459990 のテーブル境界 — [部分確定]

**(a) 両ヘルパ精密デコード（head 検証済み）**
```
459950 (ret 4): ecx=[0xb33a98] → call 0x446b80（created 判定）
                 真 → id をそのまま返す（created 素通し）
                 假 → eax=[edi+0x74] / cmp esi,[eax+0xc] / jbe resolve
                       （idx > count なら 0 返却）→ [eax+4][esi*4] → 返却
459990 (ret 4): eax=[ecx+0x78] / ecx=movzx u16 arg / cmp ecx,[eax+0xc] / jb resolve
                 （idx ≥ count なら 0）→ [eax+4][ecx*4] → 返却
```
- **`jbe`（≤）と `jb`（<）で1要素分ずれ** [確定]。`0x446B80 = cmp [esp+4],0xFF000000 / sbb eax,eax / add eax,1 / ret 4` = **`id ≥ 0xFF000000`**（v309 引用を本件で命令裏付け）[確定]。

**(b) テーブルクラス（vtable 0xA39044・v316 項目7 踏襲）と境界の語義**
- 期化: `[esi+0x74]=table`@0x45B587 / `[esi+0x78]=table`@0x45B606、テーブル内部 `{+0=vtable 0xA39044, +4=配列, +8=cap, +0xc=0, +0x10=0, +0x14=1}`（cap は v316 により +0x74=0x64 / +0x78=5）。
- resize `0x452910`: 新 cap で再確保→旧要素コピー→ゼロ埋め→`[t+8]=cap`・shrink 時 `[t+0xc]=newsize`。
- **投入 find-or-add ルーチン = FUN_0045DE30 内 0x45E0EF-0x45E141 [確定(命令)]**: 線形探索（既存一致なら index 返却）→ `cmp edi,[esi+8]` 不足なら resize → **`lea ecx,[edi+1]; mov [esi+0xc],ecx`（=要素数 count 更新）** → `[array+edi*4]=value` → `[t+0x10]++` → `ret 4`。複数 ret の pop 数（exit 系=2 / 本流=3 で edi を途中 push）は同一関数フレームとして整合。
- 別系 0x45E3D0（`shl eax,0x18 / and ecx,0xffffff / add`＝u8 索引→ID 復元 → `[ebp+0x74]` 投入）・クリア系 **fn 0x462080**（0x4621A4/0x4621C8: +0x74/+0x78 の要素ゼロ化・`[t+0xc]=[t+0x10]=0`。呼出は fn0x464440 内 **0x4644AA** で実測 [確定]、callers 全数は未走査＝残件 §7-R6）。
- → **境界の解釈: [t+0xc] = 要素数 count [確定]**。459950/45BA53/459AF0 内 inline（0x459DBF）は `jbe` で **count を上限に含める＝1件多く許容**、459990 のみ `jb` [確定]。`idx==count` は配列上の次スロット（未投入なら 0＝解決失敗で安全）だが **cap 境界では越界読みになり得る [推定]**。
- 459AF0 内の適用文脈（補強）: cursor 4B 読みの **b3 が `cmp …,0x5b / jae` で version ゲート**され、旧形式(<0x5B)は 12B/2B・新形式(≥0x5B)は 4B/6B 読みに分岐（0x459C03-0x459C82）＝**A5 の version 説の直接根拠**。

### 4-4. A4: FUN_005F0310 と vtable+0x4C getter

**(a) FUN_005F0310（本体は v317 N5 で全文 [確定]・踏襲）＋本件の呼び手分類**
- callers=4 のうち3件を本件で文脈実測:
  - **0x45376F**（4535A0 非 CELL・arg=computed flags）: 5F0310 真→`or ebx,8`（bit3 再合成）= **CheckFlags の bit3 根拠** [確定]（v317 踏襲）。
  - **0x45D296**（head 0x45D190 内）: `RTDynamicCast(obj, TESObjectREFR→Actor)` → map find([esi+0xc]) → **value[0]=flags を arg にして Actor に適用** → `test al,al` 分岐 [確定]。
  - **0x4DC22C**（head 0x4DC070 系・帯内）: `push -1; mov ecx,esi; call 0x5f0310` → 真で vfunc+0x284(0xa) チェック＋`0x4212e0`（子 0x4B 参照）を**スキップ**、偽で実行 [確定]。
  - 0x65B813（帯外・VA のみ・v317 踏襲 arg=-1）。
- **名称の裏付け探索（本件）**: vtable スロット走査（.rdata/.data に 0x5F0310/0x5F0410 の imm32 ヒット **0 件** [確定負]）＝非仮想の直接呼び出しのみ。0x5F0280-0x5F0500 の head は 0x5F0310/0x5F0410 の2つのみ（family 勢力図）。
- → **正式名 [未確定]**（文字列なし・スロットなし）／**機能分類 [推定]**: 「スクリプト所持（[form+0x58] vfunc+0x2E4∈{1..5}）∨（flags bit6 ∧ [form+0xb0]∈{1,2,6,4…} の ActiveEffect/Reanimate（[ae+0x3c]≥0x1E）所持）＝実行時変更対象述語」。提案呼称（次束用・非正典）: `IsRuntimeChangeTarget(form, flags)`。

**(b) vtable+0x4C（slot19）getter — 本件の主要新知見**
- formatter Owner 呼び出し（頭 0x453A90 起点の線形デコードで確定）:
  ```
  4566D3: test dl,dl / jns 0x4567e3        ; bit7 ゲート（dl=[esp+0x170]=flags byte）
  4566DB: cmp [esp+0x178],0 / je 45670b    ; quiet フラグ
  quiet≠0: 'CHANGE_OBJECT_EXTRA_OWNER\r\n'(0xA397C8, 28B rep movsd) をコピー
  quiet==0:
  45670B: test ebp,ebp / je 0x45672a       ; ebp = form
  45670F: mov eax,[ebp] / mov edx,[eax+0x4c] / push 0x80 / mov ecx,ebp / call edx → ax
  45671E: movzx ecx,word[esp+0x14] / 456723: movzx eax,ax / 456726: sub eax,ecx
  45672C: push eax / push 'Owner(%i)'(0xA397BC) / sprintf
  ```
- **slot19(+0x4C) の実装（vtable_map.txt 起点＋生読で確定・slot18(+0x48)=0x46AC30 とは別物）**:

| クラス | +0x4C 実装 | 本体要点 |
|---|---|---|
| TESForm（既定）/ TESWorldSpace / TESObjectDOOR | `0x46AC50` | `test byte[esp+4],1 → 4 : 0`（mask bit0→4B） |
| TESObjectREFR | `0x4E2BF0` | 既定呼び出し＋`0x45a170`(SaveGame) 真で **+6**＋mask bit27 で子ノード `0x41e6f0→0x488580` 加算＋`mask & 0x177577E0` で **`0x421340`（文字列 `'ExtraDataList::GetSaveSize'`=0xA3507C を push）** の戻り ax を合算 |
| TESObjectCELL | `0x4D1EB0`（CCラン=1・実ヘッド） | mask bit27→+4、既定、`0x45a170` 真→+6、mask&8→+1、bit28… |
| Actor | `0x5E7D00`（CCラン=2・実ヘッド） | `0x659be0` 基礎値＋`0x45a170`→+6、**+7 常時**、mask&0x40→+1、親 formType==0x24(CREA) かつ `[+0x104]==4` かつ `[+0xd4]≠0`→+1 … |
| Character | `0x60E360` | `jmp 0x5e7d00`（Actor 実装への別名） |

- → **分類 [推定]: 「mask 指定フィールド群のセーブ時バイトサイズ（u16）を条件付き定数合算で算出する GetSaveSize 系仮想」**（根拠: 定数 1/4/6/7 の加算・mask ビット分岐・`ExtraDataList::GetSaveSize` 文字列・呼び出し側の差分表示）。TESForm 既定は「owner 等を持たない型の既定サイズ（0/4）」と解される。
- **残る不確定点**: `word[esp+0x14]`（基準値）の出所（Owner 窓より前の構築点を未追跡）と `%i` が「サイズ」「オフセット差」のどれとして表示されるか [未確定]。呼び手が4クラスすべて override を持つ点は「クラスごとに payload サイズ式が違う」ことの直接証拠 [確定]。

### 4-5. A5: 保存レコード +9（静的面）— [部分確定]

**(a) 保存側のフレーム構造（頭 0x465130 起点・v317_02.out 再利用＋本件窓で再確認）**
```
ローカル 10B レコード = buf [esp+0x38]: <formID @+0(0x38)><type @+4(0x3C)><flags @+5(0x3D)><★byte @+9(0x41)><tailSize u16>
465344: eax=[entry] (flags) → [esp+0x3d]
46534A: al=byte[ebp+0x7c]（★version byte）
46534D: [esp+0x38]=ecx（formID）
465351: [esp+0x41]=al   ← ★tail==0 経路: +9 にはそのまま version(0x7D) が入る
465355: je 0x465472（tail==0 → このまま書出）
46535B: 旧 tail header 4B 読取（cursor+4）
46536D: al=[esp+0x6e]（旧 header b2=type）→ [esp+0x3c]（type 上書き）
465371: cl=[esp+0x6f]（★旧 header b3）→ 46537E [esp+0x41]  ← ★tail≠0 経路: +9 には旧 b3 が上書き
4653AE: push 0xa で 10B 書出（+9 を含む）
```
- ロード側の対応（本件新規）: **record+9 → tail header b3 へ再構成**（`mov cl,[esp+0x25] / mov [esp+0x53],cl` @0x46679C-0x4667A3、u16 tailSize=[esp+0x26]→[esp+0x50]、type char=[esp+0x20]→[esp+0x52] → `push 4 … call 0x45b9a0`@0x4667E4 で4Bヘッダ append）。ローカルフレーム [esp+0x1c..0x27] = **12B レコードそのもの**（+0 formID / +4 type / +5 flags / **+9 b3** / +0xa tailSize）。

**(b) byte3(+9) の役割 = version 分岐キー [確定(命令)]**
- 459AF0: **`cmp byte[esp+0x1f],0x5b / jae 0x459c82`**（b3 vs 0x5B）で payload 形式を切替（<0x5B: 12B/2B 系 / ≥0x5B: 4B/6B 系）。
- version 文字列2点: `'Save game version is %i.%02i and current version is %i.%02i…Continue trying to load?'`(0xA3A8E8, 0x45DD9B 近傍の sprintf で `push 0x7d / push 0 / …` と併用) / `'You are loading a savegame with version %i, but the latest version this exe supports is %i…'`(0xA3B450, `mov al,[ebp+0x7c] / cmp al,0x7e / cmp al,0x7d / jbe …`@0x4659B1-0x4659BA)。
- LoadGame 初期化: `mov byte[ebp+0x70],0 / mov byte[ebp+0x71],0x7d / mov byte[ebp+0x7c],0x7d`@0x46699E-0x4669A2、LoadForm が `[esi+0x71]→[esi+0x7c]` にコミット（0x4639B0-0x4639BE）。
- LoadForm は b3 を `new(0x10)` ノード `+0xc` へ保存（0x463A07-0x463A13）＝**バイトの受け流し先は特定 [確定]、その消費元は [未確定]**。

**(c) 補助 [確定負]**: 許可帯 0x452000-0x467920 / 0x4D8000-0x4F2000 で **`[reg+9]` のバイトアクセスは 0 件** ＝ 12B レコードの +9 はローカルフレーム経由でのみ扱われる。

**(d) 裁定**: 「+9 = version byte」[推定（上記4根拠で命令整合）] / 「+9 = typeID」旧説（v312件4・v315 R3・v317）と**両論併記**。typeID 説は `b3` が 0x5B・0x7D の version 定数と比較される事実を説明できない点で劣後（裁定理由）。**確定には実 .ess のバイト照合（#82）が必要 [未確定=ess 面]**。

### 4-6. A6: その他の静的残件

- **A6-4（45AF0 発動文脈）**: `callers(0x459F30)=1 (0x4CBECC)` / `callers(0x4CBD30)=1 (0x44EF19)` / `callers(0x44EE00)=3 (0x44F058, 0x507896, 0x5BA629)`。fn **0x44EE00** の prologue は `push esi; mov esi,ecx; call 0x44d8e0…`（実ヘッド・head 直前 CC=2 で find_head 誤答 → 0x44EDE0 は直前の scalar-dtor）。`test byte[edi+0x24],1 → 0x4C97F0 同型守り → 0x459f30` の並びは 0x4CBEA2-0x4CBED1 で実測。**fn0x44EE00 の上位2件は v318 禁止帯 → 深解析せず**。
- **A6-6（45ADC0 列挙基点）**: `0x45AF11 call 0x452800` → `ebx=[esp+0x48]` → **`mov [esp+0x30],ebx`@0x45AF20** → `mov eax,[esp+0x30]`@0x45AFD0 → `mov ebx,[eax]`。0x452800 = ノード pop（`[eax]`=next を書き戻して前進、枯渇時は `vfunc+4(this, byte)` で次のバケット取得）[確定(命令)]。
- **A6-7（0x464440）**: 0x464440-0x4646F0 を頭起点線形デコード → `ret` 0 件・尾 jmp 0 件・16 アライン新規 head 0 件（head=True 候補なし）→ 単一関数 [確定]。prologue は SEH（`push -1/0x9ae6e6/fs:[0]/sub esp,0x44/xor cookie`）で 0x464440 自体が正しい開始 [確定]。
- **formType 符号名テーブル `0xB05E04`（新リソース発見）**: 12B/エントリ `{name ptr, 0, idx+1}`、**index = レコードの formType byte**、末尾 0x44=TOFT。主要行: `0x18=DOOR, 0x23=NPC_, 0x24=CREA, 0x30=CELL, 0x31=REFR, 0x32=ACHR, 0x33=ACRE, 0x35=WRLD`。usage: LoadForm の型名エラー表示（0x46389A/0x4638AB）。**本表により 5E1F60=CELL・5E1F40=WRLD・459AF0 の `cmp cl,0x30/0x31/0x32/0x33`・v317 N9 の区画呼称がすべて記号的に裏付けられた**。
- **教訓（追加）**: **`func_head()` の CC ラン下限4による「実ヘッドの見逃し（find_head 誤答）」を本件で5例検出**: `0x4637C0`(CC=0, LoadForm) / `0x446B80`(CC=2) / `0x4D1EB0`(CC=1) / `0x5E7D00`(CC=2) / `0x44EE00`(CC=2)。検出手順: ①直前が `ret/ret N/jmp` で**即16アライン開始**（パディング0-2B）②独立 prologue（SEH/cookie）または先頭で `[esp+4]` を読む③末尾の pop 数とフレーム整合。v317 のラッチ帰属ミスはこの型に起因（off-by-pad の**逆**系）。

---

## 5. v317/v315/v312 既知との整合・訂正（旧結論は不削除）

| # | 既存記載 | 出典 | 本件の裁定 | 裁定理由（証拠） |
|---|---|---|---|---|
| C1 | 「bit7 ラッチ含有関数 = **FUN_00463700**（head 0x463700 検証済・ret 4@0x46391F）」 | v317 §4-7(c) / §6-5 | **訂正: FUN_004637C0 = TESSaveLoadGame::LoadForm** | raw bytes で 0x4637BB 尾 jmp → 0x4637C0 SEH prologue（**パディング0**）・epilogue 対応（fs:[0] 復元+cookie 0x138）・callers 系統相違（2 件 vs 4 件）。v317 の find_head(0x463966)=0x463700 は CC ラン不足型の誤答（§4-6 教訓） |
| C2 | 「FUN_00463700（SaveGame メソッド）と 0x460BC0 の同定…呼び手 head 0x464440 との関係（2呼出）も未説明」 | v317 §6-5 | **部分解消**: 463700 はグローバルクリア連鎖（fn0x464440 から2回呼出＝関係は解明）／**0x460BC0 = payload 適用/スキップ両用ディスパッチャ [確定]**／名称は両方 [未確定] 残 | 0x463700 全文（0x463700-0x4637BE）、460BC0 全文（RTTI 分岐・cursor 進めるだけ経路・REFR 適用経路）、callers(460BC0)=2 |
| C3 | 「0x5E1Fxx 帯＝**Level 系 getter の隣接**」（推測語） | v317 §4-4(b) | **訂正: 親 CELL/WRLD getter** | `byte[ptr+4]==0x30/0x35` ＋ formType 表 0xB05E04（0x30=CELL/0x35=WRLD）＋5E0260=`[+0xf8]≠0` |
| C4 | 「12B +9 = tail frame **typeID**」 | v312件4 / v315 R3 / v317 N8・§5-11 | **両論併記 → version byte 説を採用（typeID 説は [確定] から [推定] へ降格提案）** | 訳①tail==0 で version(0x7D) が恒常入力（0x46534A-51）②tail b3 が version 閾値 0x5B 比較（0x459C03）③0x7D は version 文字列/現在 version 定数（0xA3A8E8・0xA3B450・0x4659B4-BA）④LoadGame の [+0x71]=0x7D→[+0x7c] 初期化。typeID 説は 0x5B 比較を説明不能。**実 .ess で最終裁定（#82）** |
| C5 | 「テーブル境界の解釈は [推定]」（459950） | v317 §4-6(c)/§6-7 | **[確定] へ格上げ（要修正点つき）** | [t+0xc]=要素数 count（投入ルーチン 0x45E123-0x45E12D が `count=index+1`）＋比較が 459950 `jbe` / 459990 `jb` で**1件ずれ** |
| C6 | 「45BB30 = TLS+0x185 クリア＋位置/レベル系取得・正規化の副作用型…副作用の正体は 0x4DD4B0/0x4D8A10/0x4D8A30 の同定待ち」 | v317 N4 / §6-2 | **同定完了（名称のみ [推定] 残）**: 4DD4B0=親セル決定・接続、4D8A30=位置セット+通知、4D8A10=float フィールドセット+通知、5E1F60/F40=親取得 | §4-1(b) の命令列 |
| C7 | 「0x31 区画 = Door / 0x23/0x24 = ActorBase 系」等の区画呼称 | v317 N9・N7(b) | **補強（矛盾なし・用語を記号化）**: 0x31=**REFR**（CHANGE_DOOR_OPEN_DEFAULT_STATE はドア参照の変更）／0x23/0x24=**NPC_/CREA**／0x32/0x33=**ACHR/ACRE** | formType 表 0xB05E04。ラベル語（Door/ActorBase）と formType 記号の対応を明示 |
| C8 | 「45BDE0 は 45BB30 呼び手②（名称未）」 | v317 §4-4(b) | **`FUN_0045BDE0 = ResetObject` [確定(文字列)]** | 0xA3A560/0xA3A524（本関数内で使用）＋成功時 bit2/3/31 クリア |
| C9 | 「45AF0 発動文脈は上位関数の同定待ち」/「45ADC0 列挙基点は伝搬未完全追跡」/「0x464440 窓は境界再確認要」 | v317 §6-8/§6-10/§6-11 | **45AF0=[部分確定]（v318 帯で頭打ち）/ 列挙基点=[確定] / 0x464440=境界エラーなし [確定]** | §4-6 |
| C10 | 「リーダ不在の bit7 は保険上位ラッチ」（再入ガード [推定]） | v317 §6-6 | **[推定] を維持・強化**: 許可帯リーダ 0 件 [確定負] を追加、ラッチ範囲＝460BC0+vfunc+0x54 のみ・LoadForm 固有を確定 | v319_05 スキャン / 466695 文脈 |
| C11 | v316 §6-9「formID 解決テーブル [+0x74] の投入元（誰が書き込むか）未特定」 | v316 | **解消（構造は [確定]）**: find-or-add = FUN_0045DE30 内 0x45E0EF-0x45E141、クリア = fn0x462080（LoadGame 0x4644AA から呼出） | §4-3(b)。残るは「fn0x45DE30 の上位呼び手」のみ（§7-R6） |

---

## 6. ess 依存で残す項目（**#82 待ち**・本件では一切解析しない）

| # | 項目 | 現状 | 必要な動的証拠 |
|---|---|---|---|
| E1 | **N8 の tail `Δ=0x1C/0x2C` 中身**（v317 §6-1） | [未確定]（レイアウト式のみ [確定]） | 実 .ess の旧 tail 先頭 28/44B 照合 |
| E2 | **保存レコード `+9` / tail header byte3 の実値**（A5 の version 説の最終裁定） | 静的構造 [確定]・語義 [推定] | 実 .ess の +9 実値と version の一致照合、可能なら複数世代スロット比較 |
| E3 | v317 §7 の実在面（実ファイルの bit3/bit7 有無・+9 実値行） | 踏襲・未実施 | 実 .ess |
| E4 | 45BB30 副作用の**動的観測**（TLS+0x185/bit5 ラッチの実行時挙動） | 静的経路は [確定]、実行時意味は [未確定] | 実行時トレース（本ループでは未実施） |

※ 本件は「実 .ess は不在（v313 §1.1 の8系統・再検索せず踏襲）」を前提とし、上記は**深い推測を禁止するポインタとしてのみ**記載する。

---

## 7. 新規残件（次束用）

1. **[未確定] FUN_00463700 の正式名称** — 挙動はグローバルクリア連鎖＋this クリア [確定(命令)]、fn0x464440 から2回呼出。文字列なし（末尾 tail-call 0x4F9DD0 の先も未同定）。
2. **[未確定] FUN_0045BB30 と 0x4DD4B0 の正式名称** — 副作用経路は §4-1 で完全同定 [確定]、名称は文字列なし。呼び手 ResetObject とは別に pass2 が副作用目的で呼ぶ理由の正式名付け。
3. **[未確定] FUN_005F0310 の正式名** — 分類 [推定]（IsRuntimeChangeTarget 型）のみ。スロット0件・文字列0件。
4. **[未確定] vtable+0x4C getter の厳密語義** — `Owner(%i)` の基準 `word[esp+0x14]` の構築点（Owner 窓より前）と、%i がバイトサイズ/オフセット差のどれか。0x45a170（+6 条件）・0x659be0 の意味も未同定。
5. **[未確定] ロード側 12B レコードフレーム（[esp+0x1c..0x27]）の fill 源** — 0x465C88 の 12B 仮想 read が候補だが、フレーム書込み点の線形追跡がデータ島で断片化（§4-5(a) の代入点 0x46534A/0x46537E 以外）。
6. **[部分確定] +0x74/+0x78 投入ルーチナの上位工程** — 投入=find-or-add@FUN_0045DE30 内 [確定]、ただし fn0x45DE30 自身の呼び手（ロード/検証のどちらのフェーズか）は未特定。
7. **[部分確定] 45AF0 発動の上位文脈** — fn 0x44EE00 まで [確定]、上位 callers=0x44F058/**0x507896**/0x5BA629 は **v318 禁止帯・帯外 → v318 lane への照会事項**（禁止帯のため本件では深解析せず）。
8. **[未確定] SaveGame+0x18 bit7 の読み手と最終意味** — 許可帯リーダ 0 件 [確定負]。帯外リーダ探索（0x430000-0x431000 之外）と bit0/bit6/bit9/bit12/bit18 との bit カタログ対応（v316 の530件）は未整理。
9. **[未確定] fn 0x464440 の本名** — LoadGame 系（callers 0x466889=LoadGame 帯・0x40F3C1/0x500CBB 帯外、内部で LoadForm 0x46494A・解決テーブルクリア 0x4644AA・463700 2回呼出）。v316「Player 生成」窓との呼称整理。
10. **[新規・手法] find_head の CC ラン不足型誤答の体系化** — 本件5例（0x4637C0/0x446B80/0x4D1EB0/0x5E7D00/0x44EE00）。off-by-pad（偽ヘッド混入）の**逆系**。検出手順は §4-6 教訓。`tmp/re/re_util.py` 変更禁止のため**本件では手動判定のみ**（v319 プローブでの `cc-run-before` 表示を次束へ継承）。

（新規残件 = **10 件**。ess 依存は §6 の4件と別掲）

---

## 8. 再現手順

1. 環境: Windows / python 3.11（capstone 5.0.7・pefile）/ EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`。
2. 依存ヘルパー（読取専用・変更禁止）: `%TEMP%\opencode\v307_common.py`（F / func_head / find_head / func_bounds / disasm / disasm_range / str_at / callers_of / resolve_imm32_refs）。`tmp/re/re_util.py` は不使用（変更なし）。補助読取: `tmp/re/vtable_map.txt`（v319_06/09/10 のみ）。
3. 実行（全13本）:
   ```powershell
   chcp 65001
   python $env:TEMP\opencode\v319_<n>.py *> $env:TEMP\opencode\v319_<n>.out
   ```
   `<n>` = 01..13（§2 の表）。
4. 解析上の要点（本件で実効した教訓）:
   - **境界検証は「関数頭（head 検証済）からの線形デコード＋サイズ表示」**（`disasm_range` は chunk 境界を `got` で継続するためデコードがずれない）。
   - **raw bytes で関数境界を確定**（0x4637B0-0x4637C4 の `5e e9 … 6a ff` = 尾 jmp→直後 SEH prologue・パディング0）。
   - **16 アライン head は CC ラン不足でも独立 prologue/先頭 `[esp+4]` アクセスで補完判定**（§4-6 教訓5例）。
   - vtable オフセットは **+0x4C = index19（19×4）**。index18(+0x48) と取り違えると別スロット（本件 v319_06/09→v319_10 で自ら訂正）。
   - raw E8 caller 判定 = `va+5+rel==target`（callers_of）。`callers(0x4CBECC)=0` は「呼び出しサイトであって呼び出し先ではない」ケース。
   - 文字列 → 同定の威力（ResetObject / LoadForm / ExtraDataList::GetSaveSize / version 文字列）を優先して使用。
5. selftest（§9）:
   ```powershell
   $dir="$env:TEMP\opencode"; $before=@{}; Get-ChildItem "$dir\v319_*.out" | %{ $before[$_.Name]=(Get-FileHash $_.FullName -Algorithm MD5).Hash }
   1..13 | %{ $f="{0:d2}" -f $_; python "$dir\v319_$f.py" *> "$dir\v319_$f.out" }
   # 前後の MD5 比較 → 差分0 を確認（2 回連続）
   ```

---

## 9. selftest 結果

- **最終3連続実行すべて EXIT=0（全13本）・MD5 差分 = 0 / 0**（R1→R2 差分0、R2→R3 差分0）→ **出力は2回連続で一致＝現行状態で決定的**。
- 初回時のみ: v319_10 が `find_head()=None` で EXIT=1（TypeError）→ [4] に `or 0x465860` ガードを追加して再実行後は EXIT=0・以後差分0（**本件で唯一のスクリプト修正。`re_util.py` とは無関係**）。
- Traceback/Error パターン: `.out` 内でヒットしたのは **v319_04.out の1件のみ＝証拠文字列 `'Load Error: Form with ID …'`（誤検出）**。実例外は 0 件。
- 内容別 selftest ポイント:
  - v319_01: FUN_0045BB30 dump が 0x45BDCF `ret 8` で終端、callers(4DD4B0)=…（head True）。
  - v319_04: formType 表 0x44=TOFT で終端（0x45 以降は他データ＝偽エントリ区間）。
  - v319_05: bit7 リーダ走査の出力がヘッダのみ（=0 件）であること。
  - v319_10: +0x4C グループ先頭 `0x715540:58`（Ni 系）・TES 系は `0x46AC50/0x4D1EB0/0x4E2BF0/0x5E7D00/0x60E360`。
  - v319_13: callers(0x44EE00)=3 が毎回一致。

---

## 10. ドキュメント/旧記載とコードの不整合（本件で検出・AGENTS 契約による報告）

1. **「typeID」呼称（tmp/re v312件4・v315 R3・v317 N8・§5-11）vs コード**: byte3(+9) は version 閾値（0x5B/0x7D/0x7E）と比較され version 文字列と対応する（§4-5）。呼称は要訂正（正典 docs/RE_EXE_REPORT.md に "typeID" の記載は**存在しない**＝用語は自ラボ内に限定された旧記載）。
2. **v317 §4-7(c) のラッチ帰属（FUN_00463700） vs コード**: FUN_004637C0（LoadForm）が正しい（C1）。
3. **v317 §4-4(b) の「Level 系 getter 隣接」 vs コード**: 親 CELL/WRLD getter（C3）。
4. **v317 §6-11 の「0x464440 窓境界は要再確認（off-by-pad 候補）」 vs コード**: 単一関数で境界エラーなし（C9）。
5. **v316 §6-9「[+0x74] 投入元未特定」 vs コード**: FUN_0045DE30 内 find-or-add を特定（C11）。
