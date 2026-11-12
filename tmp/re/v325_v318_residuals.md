# v325 — v318 由来新規残件 6 件の裁定（次束L / ai-todo #89）

- 作成: 2026-11-08（L1 ドラフト・人間承認前）
- 対象バイナリ: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam、ImageBase 0x400000)
- セクション: `.text` VA 0x401000→file 0x400 (0x626E00) / `.rdata` 0xA28000→0x627200 / `.data` 0xB02000→0x700C00（VirtualSize 0xA9C1C → 0xB32C00..0xBABC1C は BSS）、`file = va - VA + fileOff`（`re_util.py` 経由）
- **出典（裁定対象）**: `tmp/re/v318_v314_residuals.md` **§6 新規残件 1..6**（ai-todo #89 の指示文中 `tmp/re/v318_v318_residuals.md` は存在しないファイル名の誤記であり、本束は §6 の 6 項を唯一の出典とする）
- 手法: capstone 5.x 線形 disasm（`re_util` 既定）＋境界検証付き機械 census（xor/型照合/disp32/定数スキャン）＋`find_dword`／callers／IAT 解決（pefile import parse）＋ rule v3 walker 再現。ESM 実データ・実行時トレースは不使用
- 中間物: `%TEMP%\opencode\v325_*`（.py 18 本＝102,416B、.out 20 本＝378,263B、selftest 再実行 run2 2 本を含む・合計 480,679B）。リポジトリ側は本 md のみ新規作成
- 禁止事項遵守: 既存成果物の編集・ai-todo/TODO.md 操作・STATE/docs/GT/ソース/git 変更は**一切なし**。`re_util.py` 変更なし。クリーンルーム維持（OpenTES4 / xOBSE / COEF 不使用）
- 判定: 5 段階 **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**。旧結論（v318）は削除せず §4 に併記＋裁定理由を記録

---

## 1. 判定表（6 件・5 段階）

| # | 項目（v318 §6 原文要約） | v318 旧判定 | **本束判定** | 要旨 |
|---|---|---|---|---|
| 1 | **0x447050 の al=0 条件（F7/F8 の深層）: vtable[+0x1C] 戻り値のクラス別定義** | [未確定] | **[部分確定]** | slot7 読手 69 対象を全数 census：**型ゲート 46 クラスは期待型 46/46 一致（mismatches=0）で [確定] 級**、no-op 恒真 2 [確定]、無ゲート 21 は「型起因の al=0 なし」[確定]。残る reader 内部の深い失敗帰属（xor が返値か引数算出か）が未了 |
| 2 | **間接 call 595 中 49 件の解決不可分＋vtable slot 先の実体**（静的限定・動的要項目） | [未確定] | **[部分確定]** | 49 件を full-fn deep trace で **45=IAT 間接（vtable 非該当）／1=vtable[+0x18] member／3=killed-by-call**（うち 1 は既知 impl 列）に分解 → 実質残 2。slot 先は PlayerCharacter 13 site を main vtable で静的解決、class 不明グローバル＋非 global 起源は動的待ち |
| 3 | **rule v3 の帯内 sweep 全数検証**: 56 差分中 11 件検証済・残 45 件、特に v3>old 8 件の全数裏取り | [部分確定] | **[確定]** | **56/56 を証拠帰属**（ラベル 16/16 再現、v3<old 50＝次 fn callers 27＋prologue/H 判別 23、v3>old 6＝early-ret 分岐証拠 6/6、window 充足 0x552<0x800）。**旧 prose「48/8」は 50/6 に訂正**（v318_05c 出力と本束再現が一致）。「8 件」の該当は 6 件で全数裏取り済。補足: E9/EB 拡張（v323 提案）の初回適用分のみ [部分確定] |
| 4 | **FUN_005169A0 の正式役割名と ecx（エントリ index）の供給元** | [未確定] | **[推定]** | 挙動は [確定]: バイトコード word 由来の index（`movsx ecx,[edi+eax]`、`cmp ecx,0x1e` 境界）→ 行跨 0x28 テーブル 0xB0AF5C/0xB0AF60 → `call ecx`。呼び手は ScriptRunner::Run 文言 fn の 2 箇所。**正式名はシンボル外裏付けなし（imm32 参照 0 件）＝名称のみ [推定]** |
| 5 | **0x44DCF0 型スイッチの型名対応**: 型 1..0x43 → マップ 0x44E6DC → 15 ケースの各レコード型名 | [未確定] | **[確定]** | 型名テーブル 0xB05E00（stride 12、t=0..68＝NONE..TOFT、d0==t 全一致）＋マップ 67 値→case 15 件への全数分類（67/67、case 別型名つき）＋docs 交差 3/3 一致 |
| 6 | **単位=秒の動的確認**（2-6 の [推定] 解消用） | （v318 §3 2-6 が [推定]） | **[確定]** | 静的代数閉合: IAT 0xA280D0=`kernel32!GetTickCount`（v228 hint/name 0xB00BC6 実測）→ 書手 FUN_0047D170 全文（×0.001・166ms 上限・倍率 1.0）→ 3 呼び手すべて `call [0xA280D0]; push eax` → 消費者 `fsub/fld [0xB33E9C]`。**「動的」観測は未実施（静的証明に方法を差し替え）**。動的は R6 として残す |

**主判定集計**: [確定] 3 ／ [部分確定] 2 ／ [推定] 1 ／ [未確定] 0 ／ [確定負] 0（合計 6）。副次判定は §3 各項内に明記。

---

## 2. probe 一覧（%TEMP%\opencode\v325_*、18 本＋再実行 2）

| probe | 内容 | out (B) |
|---|---|---|
| v325_01 | 項1: callers(0x447050) 全数／0x44DCF0 各 case 先頭のオブジェクト構築／factory 0x447DF0 head／ctor→vtable imm／vtable_map（2172 entries）／slot7(+0x1C) census／distinct slot7 target 浅層 head／非 0x44DCF0 callers 文脈 | 105239 |
| v325_01b | 項1: slot7 type-check census（初版・SEH prologue 時の窓不足あり）／INFO・ACTI・KEYM 深窓 90 insn／factory head 80 insn | 14115 |
| v325_01c | 項1: type-check census 修正（窓16）／reader 失敗経路窓（unknown-tag / loop-end）／factory ジャンプテーブル 0x448B60 全 64 stub 解読 | 14910 |
| v325_01d | 項1: INFO unknown-tag/loop-end decode／ctor **MAIN-vtable 修正**（無変位 `mov [reg],imm`）／factory default stub 0x448B26／69 reader の直接ゼロ書 census（境界検証） | 14364 |
| v325_01e | 項1: **0x4511C0 型 getter 全文**／type-check census 最終版（SEH 耐性・窓40、mismatches 検証）／WRLD・ROAD・REFR 当該窓 | 7284 |
| v325_02 | 項2: 595 再現＋初回 deep trace（窓依存・02b で置換）／グローバル起源 census 第1版 | 8147 |
| v325_02b | 項2: v318_04b 精再現（win=15、bucket 一致検証）／49 unresolved の **full-fn callee-saved deep trace**／slot-site 全 fn グローバル census／PlayerCharacter vtable 静的 slot 解決 | 6898 |
| v325_02c | 項2: slot-site 起源＋disp 取得（窓24・callee-saved）／known-class グローバルの disp→code 解決 | 3115 |
| v325_03 | 項3: rule v3 ラベル検証再現（16/16）／帯内 303 head・56 差分 per-diff 自動裁定／v3>old 6 件の分岐ターゲット詳細／v323 E9/EB 拡張走査（第1版） | 32943 |
| v325_03b | 項3: flagged H（callers=0）23 件の prologue/vptr/pad 判別／拡張候補フィル（interior 棄却）／tier A/B 14 head の old vs v3 裁定 | 6780 |
| v325_03c | 項3: window 充足検証（0x800 vs 実 span）／no-evidence H への branch entry 探索／0x502630 thunk 主張検証／data-region 確認／56 行再検 E | 6124 |
| v325_04 | 項4: 0x5169A0 head 60 insn／内部文字列 imm32 全数／callers＋ecx 供給文脈／index→impl 列転送窓／role-name 系 imm32 参照探索 | 13807 |
| v325_04b | 項4: 0x5169A0 境界（rule v3）と呼び手 fn 候補／'ScriptRunner' 文字列走査／文字列 ref の所属判定 | 6323 |
| v325_04c | 項4: caller fn 0x517330 head/END・SEH guard 文脈／PC+=argLength ループ tail／0x5169A0 終端と次 head の data table | 5274 |
| v325_05 | 項5: ジャンプテーブル 0x44E6A0（15 entries）＋マップ 0x44E6DC（68B）／型名テーブル t=0..68 全 dump／type→case 分類／docs 交差／d0==t 検証 | 8023 |
| v325_06 | 項6: timer フィールド disp32 参照 census（dt164／+0x10 76／+0x14 2／+4 5／+8 1）／0.001・1000.0 定数候補／境界検証 decode／0xB33E90 imm callers と 0x47D170 呼び手 | 56704 |
| v325_06b | 項6: dt 計算文脈 3 窓（tickbase 読手 0x407C30／setter 呼び手 0x40D860／+0x10 読手 0x405290）／timer methods 0x47D0F0/0x47D120 文脈 | 15734 |
| v325_06c | 項6: **FUN_0047D170 全文**＋定数実測／0x47D0F0/0x47D120/0x47D150 全文／GetTickCount 文脈／dt 消費者例（レート制御） | 11513 |
| v325_03.run2 / v325_05.run2 | selftest 再実行（MD5 一致検証用） | 32943 / 8023 |

---

## 3. 各項の根拠（VA・命令・caller-callee）

### 項1 — 0x447050 の al=0 条件 / vtable[+0x1C] クラス別定義（主判定 **[部分確定]**）

**(a) 構造（v318 既記を再確認・変更なし）**

- `0x447050(source=edi, obj=esi)`: v318_06 既記 head の `mov edx,[eax+0x1c]; call edx`（0x44705B/0x447069）＝**slot7 呼び出し型、al はその戻り値直返**。呼び手側は `push edi; push esi; call 0x447050; add esp,8`（例 0x44E051..0x44E05D → `test al,al; jne 0x44E0F1`、case[6] REFR）で2引数 cdecl。
- **callers(0x447050) = 18（v318「18 件」と一致）**: 0x44DCF0 switch 内 14 件（0x44DEB5/44DF2C/44E053/44E293/44E314/44E39F/44E436/44E45C/44E4AB/44E502/44E544/44E566/44E58A/44E643）＋帯外 4 件（0x45C5C8・0x45C6BE＝`call 0x4CC850 → 0x4511C0 → 0x4DB260 → 0x447050` 系、0x4F176F・0x4F31F5）。

**(b) slot7 読手の全数 census（v325 新）**

- vtable_map 2172 entries から TES*／記録系クラスの slot7(+0x1C) を集計 → **distinct 69 対象**（例: 0x6FDFF0 ← TESForm/TESObject/TESBoundObject/TESBoundAnimObject/TESBoundTreeObject/TESActorBase n=6、0x69D990 ← AlchemyItem/MagicItem/MagicItemObject/TESActorBaseData/TESActorBase/TESNPC n=6、0x4CD600 ← TESObjectCELL、0x4F1E70 ← TESWorldSpace）。
- **最終 type-check census（v325_01e、SEH 耐性窓40）: type-check 46 / no-check 21 / no-op 2 = 69、mismatches=0**。
  - 型ゲートの典型形: `call 0x4511C0; cmp al,imm; je ok; xor al,al; ret`（例 0x4AF74A `cmp al,0x25`＝TESLevCreature←LVLC 37、0x4CD61A `cmp al,0x30`＝CELL 48、0x4A31F2 `cmp al,0x2f`＝REGN 47、0x517CCA `cmp eax,0xa`＝SOUN 10 は 32bit 形）。**46 件すべてクラスの記録型と一致（0 件の不一致）**。
- **0x4511C0（型 getter、0x4511C0..0x451205 全文）**: `[esi+0x23c]==0` → `call 0x450080`（type 未ロード）→ **失敗時 `xor eax,eax; ret`（0x4511D5）**＝型不可→以降の cmp 必ず不一致→al=0 経路の起点。成功時 `[esi+0x23c]` → `call 0x46AF70` → 型コード返却。
- **no-op 恒真 2**: `0x6FDFF0`（TESForm 系共通基底）、`0x977C50`（TESRegionDataGrass/TESRegionDataMap）＝いずれも `NO-OP(al=1 always)` [確定]。
- **無ゲート 21**: AlchemyItem(0x41B5B0)、TESAIForm(0x468440)、TESRegionDataLandscape/Manager/Sound/Weather(0x4A41A0/0x4A4860/0x4A5020/0x4A5930)、TESFlora(0x4AE0A0)、TESFurniture(0x4AE700)、TESObjectACTI(0x4B3DD0)、TESKey(0x4B9600)、TESObjectREFR(0x4D9D00)、TESLoadScreen(0x4F9700)、TESCreature thunk(0x51CD10/0x51EAF0)、TESActorBase(0x51E750)、TESFaction(0x51F8C0)、TESNPC(0x526CC0)、TESWindListener(0x5388D0)、AlchemyItem 系(0x69D990)、TESObjectExtraData(0x721610)、TESRegionData(0x982D85) → **型不一致では al=0 にならない** [確定]（＝これらのクラスの失敗定義は reader 内部経路のみ）。
- **タグ loop の成否（ACTI/KEYM/CELL/INFO 窓）**: unknown tag は `call 0x44FEA0`（取得）＋`call 0x4510B0`（tag 再読）で**素通り**、終端は `mov al,1`（0x4B3EF4 / 0x4B9756 / 0x4CD945）＝**読めないタグは失敗にしない・loop 終了＝成功** [確定(命令)]。
- **直接ゼロ書 census（境界検証、分布）**: `{0:9, 1:18, 2:17, 3:8, 4:9, 5:2, 6:5, 8:1}`（合計 69。n=1 の 18 件は probe 注記で型ゲートのみパターンとしてリスト省略）。**n≥2 の xor には引数算出型（`cmp esi,edi; je; lea eax,[esi+0x24]; jmp; xor eax,eax` → NULL 名前引数、0x4B3ECE 等）と返値失敗型が混在し、帰属の全数づけは未了** → 項1を [部分確定] に留める主因。
- **factory 0x447DF0（type→class 構築、明示項目依存・浅層 80 insn）**: `lea ecx,[eax-4]; cmp ecx,0x3f; ja 0x448B26; jmp [ecx*4+0x448B60]`（64 entry）。**default stub 0x448B26**（type 44 SNDG/50 ACHR/51 ACRE/55 TLOD/57 DIAL/58 INFO＝6 型）は型名 ptr（0xB05E00 系、`cmp dl,0x45; jae → 0xA2F7EC` フォールバック）→ `call 0x4A7A60`（エラー書手）→ esi=0 → **恒 NULL（これらの型は ConstructObject 対象外）** [確定(命令)]。

**判定内訳**: 型ゲート 46 [確定] ／ no-op 2 [確定] ／ 無ゲート 21 の「型起因成否」[確定] ／ 21 件の内部失敗全数帰属 [未確定] ／ 0x447050 本体の al 直返全文確認（現状 v318 head 36 insn 依拠）[未実施] → **主判定 [部分確定]**。

### 項2 — 間接 call 595 中 49 件＋vtable slot 先（主判定 **[部分確定]**）

**(a) 再現と分解（v325_02b、v318_04b と bucket 完全一致）**

- `buckets = {slot:348, slot0:179, global:19, no load found:49} → total 595`（v318_04b 期待値と一致）。
- **49 件の full-fn callee-saved deep trace → `{global:45, killed-by-call:3, member:1}`**:
  - **global 45 = IAT 経由の間接 call であって vtable dispatch ではない**（窓15 の初回探索では load が遠くて拾えなかった）。実例: `[0xA2807C] InterlockedDecrement`（0x5723E0/0x707610/0x559CE0/0x7205A0 等）、`[0xA28078] InterlockedIncrement`、`[0xA280C8] ReleaseSemaphore`（0x431FA0）、`[0xA28150] TlsGetValue`（0x43AB80）、`[0xA28160] lstrcatA`（0x450550×4）、`[0xA2813C] InterlockedCompareExchange`（0x559BA0/0x43A260）。
  - **member 1**: `0x5172C6 call ecx` ← `0x517299 mov ecx,[eax+0x18]`（FUN_005169A0 本体内＝vtable[+0x18] dispatch）。
  - **killed-by-call 3**: `0x6B6D2D call edx`（fn 0x6B6B90）、`0x4FBC4C call eax`（fn 0x4FBB60）、`0x516AF3 call ecx`（fn 0x5169A0）。うち **0x516AF3 は既知 impl 列** `mov ecx,[eax+0xB0AF60]`@0x516A5D → `call ecx`（v13 stride 0x28・v159「impl 列 0xB0AF60 = 1 site」・v318 項目1 と完全一致）＝解決済。**静的に残るのは 2 件**（0x6B6D2D/0x4FBC4C）。
- v318 の「imm 解決 0 件」は維持（即値解決は依然 0）: 未解決だった 49 件は deep trace で IAT / member / impl 列へ分解され、**静的未解決は 2 件に縮小**。

**(b) vtable slot 先の実体**

- **disp 取得: 491/595（窓24方式・v325_02c）**: 内訳 非 global/不明 450 ＋ **グローバル起源 41 site / 21 globals**（0xB333C4×13、0xB34D90×7、0xB33A04×3、0xB33A1C、0xB3B364、0xB430xx 系×16）。
- **既知クラスの静的解決（PlayerCharacter）**: `[0xB333C4]` ×13 → main vtable `0xA73A0C`（n=239）で **13/13 解決**: disp 0x284×4→`0x65E030`、0x288×4→`0x65E110`、0x39C→`0x668C30`、0x23C→`0x610540`、0x238→`0x6100B0`、0x174→`0x5F10E0`、0x354→`0x60E320`（vtable slot 番号＝disp/4 = 161/162/231/143/142/93/212）。他クラスで利用可能な vtable が無く class 不明の 28 site（0xB34D90×7・0xB33A04×3・残 18）は**残 [未確定]**。
- **手法間の差（両論併記）**: full-fn callee-saved 方式（v325_02b）では slot 系 527 site 中グローバル起源 **4 site のみ**（0xA853D0×2、0xB3B27C、0xB333C4×1）。窓24方式（02c）の 41 site は高再現・低精度で、**両者の未収束は残件 R2**。非 global 起源 450 site（stack/param/field 経由）は実 .ess なしの動的要項目のまま。
- 「実 .ess なし・動的要項目」は v318 記載のまま維持。

### 項3 — rule v3 帯内 sweep 全数検証（主判定 **[確定]**、拡張分のみ [部分確定]）

**(a) 再現（v325_03）**

- **ラベル検証: OK=16 / NG=0**（v318_05c と同一セット）。
- **303 head / old≠v3 = 56 / v3<old 50・v3>old 6**。
- 56 行の自動裁定（証拠クラス）:
  - **v3<old 50**: `v3正(旧融合:次fn有callers)` 27 件 ＋ `v3正(旧融合:次fn callers=0 要prologue確認)` 23 件。
  - **v3>old 6**: `v3正(旧early-ret停止是正)` 6 件 — 6/6 に `[old, old+0x800)` 内への分岐ターゲット存在（0x405440/0x450190/0x451460/0x5005F0/0x5031A0/0x51BF10、詳細は probe 03 §3）。
- **v3>old の全数裏取り（残件の核心）**: 6/6 の old 停止点が早期 ret であり、v3 が window 内の branch target で継続する根拠を全数提示 → **「8 件の全数裏取り」要求は 6 件で完了**（48/8 は 50/6 に訂正、§4-B C1）。

**(b) flagged H（callers=0 23 件）の prologue 判別（v325_03b/03c）**

- prologue/vptr/pad 検査: **H が実 fn head 9 件**（H=0x450AC0/0x500150/0x5007D0/0x500F70/0x501340/0x501D80/0x502140/0x503D10/0x573910、いずれも cc pad 終端直後の prologue）／**H 非 head 14 件**。
- 非 head 14 件の裏取り:
  - **branch entry 探索（14 件中 10 件＋拡張側 H=0x51B440 の計 11 対象）**: rel32 entry は 2 件のみ（0x404F60←`jmp 0x9C5623`、0x5706B0←`jmp 0x9BDD03`）、残 9 件は rel32/rel8 entry 皆無 → 呼び出し起点ではない。
  - **data-region 確認（残る 4 件）**: H=0x44F3B0（u32 列 0x44F2E0/0x44F2B0…＝fn ptr テーブル）、0x51BE30、0x5737C0、0x573BF0 → **v3 の停止点＝データ境界**（old はデータ内通過）＝v3 正 [確定]。0x51B440 も head16 が非 prologue（`adc [eax],al; mov edi,edi; pushal…`＝データ）。
- **window 充足**: v3 継続証明の最大実 span = 0x552（head 0x405B80）< window 0x800 → **窓打ち切りによる誤停止なし**。
- **0x502630 thunk 検証**: 実バイトは `mov eax,[esp+0x1c]; mov ecx,[esp+0xc]; push eax; push 0; push 0; push ecx; call 0x4F4800; add esp,0x10; ret` → v318 §3-5-5／§4 の「call 0x4F4800; ret thunk」は厳密には **`call; add esp,0x10; ret`（cdecl 4 引数ラッパ）**（§4-B C4）。
- **56 行再検（03c §E）**: `{'v3<old H=present': 50, 'v3>old early-ret proof=True': 6}`。

**(c) 補足: v323 E9/EB head 拡張の初回適用（本項の新スコープ → [部分確定]）**

- 5 帯で CC k<4 ＋終端 C3/C2/**E9/EB** 候補 = **118** → `A:head(証拠あり) 6 / interior(既知fn内) 98 / B:head(証拠なし) 8 / C:棄却(prologue NG) 6`。
- tier A/B 14 head の old vs v3: **diff 4**（0x4057B0 v3<old H=0x405870、0x450080 v3>old targets=450090/4500CA/45017E、0x450430 v3<old H=0x4504B0、0x51B260 v3<old H=0x51B440＝データ境界）、他 10 same。全 118 候補 walk では old≠v3 = 36。
- **拡張則自体はラベル検証未実施（現行 16/16 は非拡張則）＋re_util 変更禁止＝文書化のみ** → 残件 R3。

### 項4 — FUN_005169A0 役割名と ecx(index) 供給元（主判定 **[推定]**）

**(a) 境界と接続**

- `0x5169A0..0x5172FF`（`ret 0x24` @0x5172FC、nop @0x5172FF、**直後 0x517300..0x51732F は data table**＝u32 列 0x5172D3/0x516B7D/0x516E9F/0x516FCE…、次 head 0x517330＝SEH `push -1`）。
- `callers(0x5169A0)=2`（imm32 refs は .text/.rdata/.data 全て空＝**vtable/関数ポインタ登録なし**）: `0x51754D`（fn 0x517330..0x51763A 内）、`0x5177C6`（**fn2**: fn1 末尾の `add esp,0x758`@0x517631 → `ret 0x20`@0x517637 → 0x51763A..0x51763F cc → **0x517640 `push -1`＝SEH prologue head**。fn2 は PC+=argLength ループ `0x5177D3 add ebp,[esp+0x14]; cmp ebp,[edi+0x20]; jb 0x517754` を保持）。両者とも `mov ecx,esi; call 0x5169A0`（thiscall、ecx＝this）。

**(b) index の供給元 [確定(命令)]**

- 本体（0x5169A0 head＋0x516A20..0x516B10 窓）: `[esp+0x764]==0x10` 経路で **`0x516A23 movsx ecx, word ptr [edi+eax]`** — edi＝スクリプト・バイトコード列（`mov edi,[ebp+0x30]`）、eax＝現在 PC（[esp+0x770]）。これが**エントリ index の供給元**。
- 境界: `cmp ecx,0x1e; ja 0x516ACF` → **失敗経路 `0x516ACF xor al,al; jmp 0x5172D5`（al=0 返却）** [確定]。
- index→テーブル（stride 0x28）: `lea eax,[ecx+ecx*4]`（×5）→ `add eax,eax`×2（×20＝0x14）→ `mov ecx,[eax+eax+0xB0AF5C]`（＝0xB0AF5C+idx×0x28＝argLen 列）→ `add eax,eax`（×40＝0x28）→ `0x516A5D mov ecx,[eax+0xB0AF60]`（impl 列）→ **`0x516AF3 call ecx`**（v13 行跨 0x28・v159 impl 列 1 site・v318 項目1 と整合）。
- ecx の二義性の解消: **外側 ecx＝this**（呼び手の `mov ecx,esi`、本体冒頭 `mov esi,ecx`）／**内側 ecx＝index（bytecode word）と impl ptr（テーブル読出し直後の call ecx）**。

**(c) 役割の証拠（名称は [推定]）**

- 本体内の文字列 imm32（一式）: `'Script '%s', line %d: Null for a required ref pointer.'`(0xA4DCC0)、`'…Error Executing line (Offset %d). '`(0xA4DC88)、`'set global >> %0.2f'`(0xA4DC34)、`'set %s >> %i'`(0xA4DC24)、`'set %s >> %.2f'`(0xA4DC14)、`'…failed to evaluate expression.'`(0xA4DBDC)、`'Unable to find function definition for command %d in script '%s''`(0xA4DB98)、`'…Function '%s' requires a reference.'`(0xA4DB58)。
- 呼び手側 fn1（0x517330..0x51763A）内の `0x5173F8 push 0xA4DCF8` = `'Nested call to ScriptRunner::Run.  Script '%s' attempting to execute script '%s'.'` → **fn1 = ScriptRunner::Run [確定(文字列)]**。fn1 は 0x5169A0 を1回、fn2（PC ループ保持）も 1 回呼ぶ。
- 関連: `callers(0x4FCA30)（command entry lookup）=3`: 0x4FAD43 / 0x5168EA / 0x51703E（0x51703E は 0x5169A0 内部＝'Unable to find function definition' 経路と整合）。
- **推定名: `ScriptRunner` の「コマンド 1 件（bytecode 1 エントリ）実行ディスパッチ」**。正式シンボル名を裏付ける vtable/関数ポインタ/imm 参照は **0 件（名称の静的裏付けなし）**、残る確証は実行時シンボルのみ → 主判定は挙動 [確定] を含めて **[推定]**。
- 未走査: `[esp+0x764]!=0x10` 経路（0x516B55 以降）→ 残件 R4。

### 項5 — 0x44DCF0 型スイッチの型名対応（主判定 **[確定]**）

- **スイッチ構成**: `ebx=type, idx=type-1, map@0x44E6DC, jt@0x44E6A0` → `jmp [ecx*4+0x44E6A0]`（15 entries）。マップは 68B（67 値＋末尾 CC 境界）で **type 1..0x43 の 67 型を 15 case に全数分類**。
- **型名テーブル（帯外データ照会）**: entry[t] = {u32 d0, char* name, u32 X} stride 12 @0xB05E00、nameptr = `u32(0xB05E04+12*t)`。**t=0..68 を全 dump: 'NONE','TES4','GRUP','GMST','GLOB','CLAS','FACT','HAIR','EYES','RACE','SOUN','SKIL','MGEF','SCPT','LTEX','ENCH','SPEL','BSGN','ACTI','APPA','ARMO','BOOK','CLOT','CONT','DOOR','INGR','LIGH','MISC','STAT','GRAS','TREE','FLOR','FURN','WEAP','AMMO','NPC_','CREA','LVLC','SLGM','KEYM','ALCH','SBSP','SGST','LVLI','SNDG','WTHR','CLMT','REGN','CELL','REFR','ACHR','ACRE','PGRD','WRLD','LAND','TLOD','ROAD','DIAL','INFO','QUST','IDLE','PACK','CSTY','LSCR','LVSP','ANIO','WATR','EFSH','TOFT'**、`d0==t` 不一致 **0 件**（t=69..71 は表外ゴミ＝表の終端確認）。
- **case 別の型名対応（全数）**:
  - case0 `0x44DE35` = 1 型: TES4
  - case1 `0x44E326` = 1 型: GMST
  - case2 `0x44E578` = **22 型**: GLOB, CLAS, FACT, HAIR, EYES, RACE, SOUN, SCPT, LTEX, ENCH, SPEL, BSGN, WTHR, CLMT, REGN, QUST, PACK, CSTY, LSCR, ANIO, WATR, EFSH
  - case3 `0x44E512` = SKIL ／ case4 `0x44E367` = MGEF ／ case5 `0x44DEF0` = CELL
  - case6 `0x44E001` = 3 型: REFR, ACHR, ACRE
  - case7 `0x44E2A5` = PGRD ／ case8 `0x44E401` = WRLD ／ case9 `0x44E237` = LAND ／ case10 `0x44DE83` = ROAD ／ case11 `0x44E474` = DIAL ／ case12 `0x44E4C9` = INFO ／ case13 `0x44E556` = IDLE
  - case14 `0x44E5A5` = **30 型**: GRUP, ACTI, APPA, ARMO, BOOK, CLOT, CONT, DOOR, INGR, LIGH, MISC, STAT, GRAS, TREE, FLOR, FURN, WEAP, AMMO, NPC_, CREA, LVLC, SLGM, KEYM, ALCH, SBSP, SGST, LVLI, SNDG, TLOD, LVSP
  - 合計 1+1+22+1+1+1+3+1+1+1+1+1+1+1+30 = **67 型（type 1..0x43）**。type 0x44 'TOFT' はマップ表外（対象外）。
- **docs 交差 3/3 一致**: RE_EXE_REPORT「type3 GMST → 0x44E326」✓／s69「WRLD idx53 → map[52]=8 → 0x44E401」✓／「CELL slot5 → jt[5]=0x44DEF0」✓。
- v318 項目3 で [確定] 済みの「15 entries・マップ 68B@0x44E6DC」と矛盾なし。**型名対応の欠落なし → [確定]**。

### 項6 — 単位=秒（主判定 **[確定]**、方法差替を明記）

**(a) 書手 FUN_0047D170 全文（0x47D170..0x47D255、`ret 4`）**

- `mov edx,[esp+4]`（引数 now）／`fldz`／`sub edx,[ecx+0x14]`（tick 基底との差分取得）／`[0xB39DB0..B4]` へ前回差分を退避。
- **ポーズ系: `cmp byte [ecx],0; jne 0x47D24D` → `fstp [ecx+0xC]` に st=fldz** → **[ecx]≠0（ポーズ中）は dt=0 強制** [確定(命令)]。
- 通常系: `fcomp [ecx+4]` 判定 → 整数丸め経路（`fistp`＋`[0xA2FC78]=4294967296.0` の負 delta 符号補正）または tick 差分経路 `esi = edx-[ecx+0x10]`（**0→1ms に丸め `mov esi,1`**、しきい値 `fcomp [0xA3D14C]`（=166.66667ms、v228 実測・本束は使用箇所のみ確認）超過で `mov esi,0xA6`＝166ms 上限）。
- 收束点 `0x47D238: fmul qword [0xA30E40]`（**0.0010000000474974513＝f32 0.001**、ms→s）→ `0x47D23E mov [ecx+0x10],edx`（tick 基底更新）→ `fstp [ecx+0xC]` → `fld [ecx+0xC]; fmul [0xB06704]（=1.0f、ゲーム速度倍率）; fstp [ecx+0xC]`。
- **timer object 0xB33E90 のフィールド対応**: +0x00=pause refcount、+0x04/+0x08=accum、**+0x0C=dt（＝[0xB33E9C]）**、+0x10=tick prev、+0x14=tick base。参照 census（v325_06 §A）: dt **164**、+0x10 **76**、+0x14 **2**、+0x04 5、+0x08 1。

**(b) 呼び手（3 件すべて GetTickCount 直後 push）**

- `callers(0x47D170) = 3`: **0x40D8AB**（`0x40D89F call dword [0xA280D0]` → `push eax` → `mov ecx,0xB33E90` → call、メインループ FUN_0040D4D0 の時計進行直後）、**0x5AD327**（`call ebx(=[0xA280D0]); push eax; mov ecx,edi`）、**0x5AD9C2**（同形）。
- **IAT `0xA280D0` = `kernel32!GetTickCount`（hint/name 0xB00BC6 実測＝v228 既記、本束は文脈再検証のみ）**。
- サブ操作: `FUN_0047D0F0`=pause（refcount 減算・0 到達時 GetTickCount で基底補正、callers 4: 0x40616E/0x40EAD2/0x494E5E/0x4E18C6）、`FUN_0047D120`=resume（加算・0→1 で基底補正、callers 3: 0x4061AF/0x494A18/0x4E15D6）、`FUN_0047D150`=reset（`fldz` で +0x04/+0x08/+0x0C を 0 化、+0x10 も 0、+0x14=引数 tick、`ret 4`）。

**(c) 消費者（dt の利用形）**

- getter `0x4052A0: fld dword [0xB33E9C]; ret`（ポーズ分岐時は fldz）。
- `0x40D8B0 fld [0xB33E9C]` → push → `call 0x889810`（フレーム直後の dt 引き渡し）。
- レート制御 `0x4FBE26 fsub [0xB33E9C]`（[esi+0x38] -= dt）／`0x4FBE32 fadd [0xB33E9C]`（[esi+0x3C] += dt）＝v318 既記の `[+0x38] -= dt` と一致。
- 他系統の同型換算: `0x407C80..0x407CCA`（[0xB33EA4] tickbase 読手、`fmul [0xA30E40]` で同 0.001 適用、`fadd [0xA30E48]=3.0` は別目的の加算）。

**(d) 代数の閉合**

`dt(+0xC) = clamp_ms( GetTickCount差分 ) × 0.001 × 倍率(既定1.0) ＝ 秒`。入力 API の単位（ms、hint/name 実測）、換算定数（0.001 実測）、上限（166.66667ms、v228 実測＋本束で使用箇所確認）、倍率（1.0f 実測）、消費者（秒率の減算）の五点が揃い、**反証になり得る静的経路なし → 単位=秒 [確定]**。
**方法注記（透明性のため明記）**: 残件として要求された「**動的**（実行時）確認」は本束でも**未実施**（実 .ess なし・実行トレース不使用）。本判定は静的代数閉合による**方法の差し替え**であり、動的観測は R6 として残す。

---

## 4. v318 既知との整合・訂正（旧結論は削除せず併記）

### 4-A 整合（矛盾なし）

| 論点 | v318/v319/v323 既記 | 本束結果 |
|---|---|---|
| 0x447050 callers | 3-5「callers 18 件」 | 18 件全数一致（内訳 14＋4 を特定） [一致] |
| F7/F8 の al 判定 | 3-4g/3-4h [確定]（al==0 → 0x44E672/674 経由） | case 内 `test al,al` 経路を再確認、変更なし [一致] |
| impl 列 | v159「0xB0AF60 = 1 site」／v318 項目1「0x516A5D→0x516AF3」 | 0x5169A0 本体内で同 site を再発見、stride 0x28（v13）とも整合 [一致] |
| rule v3 ラベル | 5-3「ラベル 16/16」 | 再現 OK=16 NG=0 [一致] |
| 595 分類 | 4-1「slot348/slot0 179/global19/未解決49」 | 完全一致 [一致] |
| 帯内 sweep 数 | 5-6「303 head・56 差分・個別検証 11 件（残 45 同型推定）」 | 303/56 再現一致・本束で 56/56 証拠帰属（内訳は下記 C1 で訂正） [总数一致] |
| dt 消費者 | 項目2-5「[+0x38] -= dt」 | 0x4FBE26 の `fsub [0xB33E9C]` で一致 [一致] |
| E9/EB 拡張提案 | v323 項9・§6-3 | 帯内に初回適用（118 候補）。v323 と矛盾なし。0x4637C0 実例は v318 禁止帯のため未走査 [整合] |
| docs 照会 | RE_EXE_REPORT / s69 の case 対応 | 3/3 一致・不整合なし [一致] |

### 4-B 訂正・格上げ（v318 本文はそのまま併記）

- **C1（項3 の内訳訂正）**: v318 §3-5-6 の prose「**v3<old 48 / v3>old 8**」および §6-3「特に **v3>old 8 件**の全数裏取り」は、**v318_05c.out の個別リスト計数でも本束 v325_03 再現でも `v3<old 50 / v3>old 6`（合計 56 は一致）**。→ **正しい内訳は 50/6、v3>old の全数裏取り対象は 6 件**（6/6 完了）。旧 48/8 表記は誤記として併記維持（削除しない）。
- **C2（項1 格上げ）**: §6-1 [未確定] → **[部分確定]**。型ゲート 46 クラスの期待型一致（mismatches=0）・no-op 2 恒真・無ゲート 21 の型起因不在まで [確定]。残は reader 内部失敗の帰属のみ。
- **C3（項2 格上げ）**: §6-2 [未確定] → **[部分確定]**。「49 件解決不可」は 45 IAT＋1 member＋3（うち1既知）に分解し実質残 2。slot 先は PlayerCharacter 13 site 解決済。
- **C4（項3 に付随する v318 記載の軽訂正）**: v318 §3-5-5／§4 の「0x502630..0x502647 は独立 thunk（`call 0x4f4800; ret`、callers 0）」→ 実バイトは **`call 0x4F4800; add esp,0x10; ret`**（cdecl 4 引数ラッパ）。**独立 thunk・callers 0・位置・0x502500 end=0x50262F の結論は維持**し、「call 直後に ret（tail 型）」の表現のみ補正。
- **C5（項6 格上げ・方法差替）**: v318 §3 2-6「単位=秒 [推定]（動的実測なし）」→ **[確定]**（v228 の hint/name 実測＋本束の書手全文・3 呼び手・消費者の三重検証で代数閉合）。**要求された動的観測自体は未実施**である点を旧 [推定] と併記する。

### 4-C v321 ほか先行成果物との関係

- `v321_v318_next_bundle_H.md` との不一致3 点（195.06・xor eax,eax 列挙・walker [部分確定]）は **v318 §5 で裁定済みのまま維持**。本束は新証拠なし。項6 の「runtime 単位=秒 [確定]」は `.rsrc` ファイルゴミ（fileOff 0x732A9C, bytes 0a104343）の 195.06 論点とは**別件**（v321(a)「raw f32・単位不明」の対象はファイル素値であり矛盾しない）。
- `GT_CHEATSHEET.md` §3、v13（0xB0AF50 基底・stride 0x28）、v159（impl 列）、v228（GetTickCount・定数 0.001/166.66667/1.0）、v319（静的 lane 制約）、v323（E9/EB 提案）と**不整合 0 件**。

---

## 5. 新規残件（次束用）

1. **[未確定] slot7 無ゲート 21 reader の内部失敗の全数帰属**: 直接ゼロ書 census（n=2..8）の各 site が「返値失敗」か「引数算出（NULL 名前等）」かの区別（例: ACTI 0x4B3EA4/0x4B3ECE は引数算出と判明済み、他の 20 reader は未了）＋ n=0 群（TESAIForm 0x468440 / TESCreature 系 thunk / TESNPC 0x526CC0 / TESWindListener 0x5388D0 等 9 件）の「間接的 0 返却なし＝恒 success」確認。併せて **0x447050 本体全文**（帯外・明示項目可）で al＝slot7 直返の最終確認。
2. **[部分確定→残] 間接 call の残**: killed-by-call 2 件（`0x6B6D2D call edx` / `0x4FBC4C call eax`）の実先。class 不明グローバル 28 site（0xB34D90×7・0xB33A04×3・0xB33A1C・0xB3B364・0xB430xx×16）の slot 先。非 global 起源 450 site。**手法間未収束（full-fn 方式 4/527 vs 窓24 方式 41/595）の突合**。実 .ess/動的 lane。
3. **[新規] E9/EB head 拡張則のラベル検証セットと rule v3 への本採用可否**: 現行 16/16 ラベルは非拡張則用。帯内 tier A/B 14 head・4 diff は本束で初検済み（0x4637C0 実例は禁止帯で未走査）。`re_util.py` 変更禁止のため文書化のみ。
4. **[新規] FUN_005169A0 の残経路と caller fn2**: `[esp+0x764]!=0x10` 経路（0x516B55 以降）未走査。fn2 の head 同定（0x517640 `push -1` が SEH prologue とみられるが rule v3 での正式 head 判定未実施・0x5177C6 収容）と名寄せ。役割名 [推定] の格上げにはシンボル外裏付け（実行時シンボルなし）が必要。
5. **[新規] factory/ctor→MAIN vtable 表の疑義行**: v325_01d 表で TESIcon(0xA320A4) に集約される 10 型（MGEF/APPA/BOOK/INGR/LIGH/MISC/WEAP/AMMO/ALCH/QUST）と TESFlora 行の slot7=0x63696E61（ASCII 混入＝vtable 特定誤り）→ ctor の MAIN vtable 書き込み位置の全数確定。**判定に使用した 01e census は vtable_map 直結であり無傷**（mismatches=0）。
6. **[残] 単位=秒の実行時（動的）観測**: 静的閉合済のため低優先。実行 lane で `dt(+0xC)` の実値が 1/60 〜 0.2 秒系に収まることの確認。

---

## 6. 再現手順

```powershell
chcp 65001
python $env:TEMP\opencode\v325_01.py   *> $env:TEMP\opencode\v325_01.out    # callers/case 構築/factory/vtable_map/slot7 census
python $env:TEMP\opencode\v325_01b.py  *> $env:TEMP\opencode\v325_01b.out   # type-check census 初版・深窓・factory head
python $env:TEMP\opencode\v325_01c.py  *> $env:TEMP\opencode\v325_01c.out   # census 修正・失敗経路窓・0x448B60 全解読
python $env:TEMP\opencode\v325_01d.py  *> $env:TEMP\opencode\v325_01d.out   # MAIN-vtable 修正・default stub・zerowrite census
python $env:TEMP\opencode\v325_01e.py  *> $env:TEMP\opencode\v325_01e.out   # 0x4511C0 全文・census 最終(40 insn)
python $env:TEMP\opencode\v325_02.py   *> $env:TEMP\opencode\v325_02.out    # 595 再現・初回分類
python $env:TEMP\opencode\v325_02b.py  *> $env:TEMP\opencode\v325_02b.out   # bucket 精再現・49 deep trace・PC vtable 解決
python $env:TEMP\opencode\v325_02c.py  *> $env:TEMP\opencode\v325_02c.out   # slot-site 起源+disp・class 解決
python $env:TEMP\opencode\v325_03.py   *> $env:TEMP\opencode\v325_03.out    # ラベル16/16・56 差分裁定・E9/EB 走査
python $env:TEMP\opencode\v325_03b.py  *> $env:TEMP\opencode\v325_03b.out   # flagged H 23 prologue 判別・tier A/B 裁定
python $env:TEMP\opencode\v325_03c.py  *> $env:TEMP\opencode\v325_03c.out   # window 充足・branch entry・thunk・data 境界
python $env:TEMP\opencode\v325_04.py   *> $env:TEMP\opencode\v325_04.out    # 0x5169A0 head/文字列/callers/impl 転送
python $env:TEMP\opencode\v325_04b.py  *> $env:TEMP\opencode\v325_04b.out   # 境界・ScriptRunner 文字列・ref 所属
python $env:TEMP\opencode\v325_04c.py  *> $env:TEMP\opencode\v325_04c.out   # caller fn head/end・PC ループ tail
python $env:TEMP\opencode\v325_05.py   *> $env:TEMP\opencode\v325_05.out    # jt/map/型名表/type→case/docs 交差
python $env:TEMP\opencode\v325_06.py   *> $env:TEMP\opencode\v325_06.out    # timer 参照 census・定数・callers
python $env:TEMP\opencode\v325_06b.py  *> $env:TEMP\opencode\v325_06b.out   # dt 計算文脈 3 窓・timer methods
python $env:TEMP\opencode\v325_06c.py  *> $env:TEMP\opencode\v325_06c.out   # 0x47D170 全文・定数・消費者
# selftest 再実行（03/05 は run2 と MD5 一致）
python $env:TEMP\opencode\v325_03.py   *> $env:TEMP\opencode\v325_03.run2.out
python $env:TEMP\opencode\v325_05.py   *> $env:TEMP\opencode\v325_05.run2.out
```

---

## 7. 帯遵守宣言

- **深解析帯は v318 自帯のみ**: `0x404000-0x406000` / `0x44F000-0x452000` / `0x500000-0x504000` / `0x51B000-0x51C000` / `0x570000-0x574000`。帯内走査は walker/sweep 検証の機械走査＋境界検証を伴うもので、深読はこの 5 帯の帯内 head/差分箇所に限る。
- **明示調査項目の帯外・単関数 read-only**（v318 0x44DCF0 条項と同型、各 probe の docstring で明示）:
  - `0x447050`（項1、callers 文脈＋v318_06 既記 head のみ）、`0x4511C0`（項1 依存・69 insn 全文）、factory `0x447DF0` head 80 insn＋`0x448B60`/`0x448B26`（項1 依存・単関数）
  - `0x44DCF0..0x44E69E`＋`0x44E6A0` jt/`0x44E6DC` map（項5、本体は v318_03 再利用）
  - `0x5169A0`（項4・単関数）＋ callers 直近文脈、`0x517330` head/end（項4 依存）
  - `FUN_0047D170` 全文＋`0x47D0F0`/`0x47D120`/`0x47D150`（項6・単関数群）
- **0x4C0000-0x4E0000 は浅層**: slot7 reader（CELL 0x4CD600 / LAND 0x4C7090 / LTEX 0x4C90A0 / REFR 0x4D9D00 等）は **head 窓 44-90 insn＋境界検証付き機械 census（型照合・xor カウント）のみで、手詰め深読は 0 件**。0x4E0000-0x4F2000 の PGRD/ROAD/WRLD 系も同様に head 窓＋機械 census のみ。
- **v319 禁止帯（0x430000 / 0x452000-0x467920 / 0x4D8000-0x4F2000 / 0x5E1000-0x5E2800 / 0x5F0000-0x5F1000）は深解析 0 件**: 該当は 0x45C5C8/0x45C6BE・0x4D9D00/0x4F176F/0x4F1E70 の直近文脈窓と機械 census のみ（浅層）。0x4F31F5 は禁止帯外（帯外・直近文脈のみ）。v323 の 0x4637C0 実例は本束未走査。
- **データ照会条項**: `.data`/`.rdata` 全域の読取（0xB05E00 型名テーブル、0xB0AF50 系テーブル、0xB33E90 系 timer、定数 0xA30E40/0xA3D14C/0xA2FC78/0xB06704/0xA30E48、IAT 0xA280D0 等）は参照元にかかわらず読み可（v318 条項踏襲）。
- **機械走査（disp32/定数/raw u32/E8/E9 スキャン）は命令境界検証を伴うデータ走査であり深解析に含めない**（v318 第13行と同じ扱い）。
- クリーンルーム維持: OpenTES4 / xOBSE / COEF 不使用、ESM 実データ不使用、実行時トレースなし。`re_util.py` 変更なし・既存成果物の上書きなし・git 操作なし。リポジトリへの新規追加は本 md のみ。

---

## 8. selftest 結果

- 項1: `type-check=46 no-check=21 no-op=2 total=69 mismatches=0`（01e）／zerowrite 分布 `{0:9,1:18,2:17,3:8,4:9,5:2,6:5,8:1}`（01d）／factory stubs decoded 64・default stub=0x448B26（01c/01d）
- 項2: `buckets {'slot':348,'slot0':179,'global':19,'no load found':49} total 595`（02b、v318_04b 期待値一致）／`deep {'global':45,'killed-by-call':3,'member':1}`／`disp capture 491/595`（02c）／PlayerCharacter vtable 0xA73A0C n=239 → 13/13 解決
- 項3: `labeled pairs OK=16 NG=0`／`sampled=303 diffs=56 v3<old=50 v3>old=6 extended_cands=118 extended_diffs=36`（03）／`rows=56 {'v3<old H=present':50,'v3>old early-ret proof=True':6}`・`max span 0x552 < window 0x800`（03c）／flagged H 23＝prologue OK 9＋NG 14、branch entry 有 2/11、data region 4 件（03b/03c）
- 項4: `callers(0x5169A0)=2 (0x51754D,0x5177C6)`・`imm32 refs=[]`・`end(0x5169A0)=0x5172FF (ret 0x24)`・`end(0x517330)=0x51763A`（04/04b/04c）
- 項5: `d0 != t for: []`（表 t=0..68 全一致）・type→case 67/67 分類・docs 交差 3/3（05）
- 項6: 参照 census `dt=164 / +0x10=76 / +0x14=2 / +4=5 / +8=1`・`callers(0x47D170)=3, (0x47D0F0)=4, (0x47D120)=3`・定数実測 `0xA30E40=0.001(f64), 0xB06704=1.0f, 0xA2FC78=2^32, 0xA30E48=3.0, 0xA2FCC8=60.0`（06/06b/06c。しきい値 0xA3D14C=166.66667 は v228 実測値を引用し本束は使用箇所を確認）
- **再実行一致**: `v325_03.out` と `v325_03.run2.out` MD5 = `0BC0D03303B3FA4E32889201FF26C9BE` 一致、`v325_05.out` と `v325_05.run2.out` MD5 = `3FA6358EA5A05BF0C037A8E52ACCDDDB` 一致

**判定件数（主判定6件）**: [確定] 3（項3・項5・項6）／ [部分確定] 2（項1・項2）／ [推定] 1（項4）／ [未確定] 0 ／ [確定負] 0。
