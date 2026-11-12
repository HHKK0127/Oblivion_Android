# V161: quest/Script/extra 残点束 16 点の裁定（SysWindow 機能名・bit14 名称・装備ブロック・SCHR 初回書手）

- タスク: TODO_re49 serial 151（V151）/ 成果物本ファイル
- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- 出典: `v141_b19_quest_residuals.md` 残点 V143a–h(+i) 9点 ＋ `v139_b17_table2_residuals.md` 残点 V143-a/b 2点 ＋ `v142_s7010_residuals.md` 残点 1–5 5点 = **計16点**
- 作成: 2026-10-27（opencode サブエージェント、分析専任・移植実装なし）
- **二重実行確認**: 着手時に本ファイルは**不在** → 新規作成（上書き対象なし）

---

## §0 判定表（16 点）

| # | 出典 | 残点 | 判定 | 一句话 |
|---|---|---|---|---|
| 1 | v141 V143a | SysWindow のクラス同定（ctor に vtable ストア無し） | **確定** | `0xB3A6E0` 単一トン = `SysWindow`（size 0x134・非多相）。vtable 不存在は構造上の必然で RTTI は不要 |
| 2 | v141 V143b | コンソールコマンド `tag` の用途 | **確定** | 選択 REF の `"%s" (%08x)` 整形 → `[REF+0x3C]` ノード列に `0xB3FAB0` が含まれるか判定 → `FUN_00572850` へ |
| 3 | v141 V143c | `FUN_004dc270`（選択→フォーム解決）のアルゴリズム | **解明** | 鎖(+0x1c)を後ろから走査 → スロット1列に `0xB35ACC` を含む要素1つ → +0xC を GetBaseForm 検証、formType `0x22`(AMMO) のときだけ再帰。`x==[player+0x5D0]` なら player を返す |
| 4 | v141 V143d | コンパイル ctx フィールドの意味論 | **解明（残1）** | ctx は**レコード相当レイアウト**、quest ゲート3サイトの極性 = `flagA!=0 && ctx+0x38!=0 → エラー3種`（**`ctx+0x38≠0 ⟺ quest script`**、= `Script+0x28` 下位バイト）。`+0x2C/+0x30` の更新経路は V-161c |
| 5 | v141 V143e | Script の残フィールド | **解明** | `+0x2C`=ソース文字列（owned）/ `+0x30`=バイト列（owned、長さ `+0x20`）/ `+0x34..3C`=quest delay 浮動小数（slot5 リセット・slot27 使用・`fQuestDelayTime`）/ `+0x40/+0x44`=参照オブジェクトリスト。**`+0x44` は BaseExtraList でない** |
| 6 | v141 V143f | extra 0x12 の `+0xC` 初回書手 | **確定** | 書手は **`FUN_0041f090` の1本（19 呼出し）**＋ctor `FUN_00429f20`。値は **`TESScriptableForm+0x4`（Script*）**（`TESForm→TESScriptableForm` の `__RTDynamicCast`）。`+0x10` は別系統 `FUN_0041f130`(20)/`FUN_004fbdc0`(23) |
| 7 | v141 V143g | RTTI TD 名解決不能57件 → 台帳欠落1件 | **確定: 欠落なし** | `0xB2CF30` は `vtable_index.tsv` **行473 実在**（NiTStringTemplateMap…、vtable 0xA90370）。欠落は TD 文字列走査の 100B キャップ/正規表現終端由来の誤認 |
| 8 | v141 V143h | ラベル欠落3件の Ghidra 反映可否 | **確定: 追加不要** | `named_funcs.txt`（Ghidra FunctionManager 台帳）に **3件とも実在**（行4524/4364/4501）。欠落は `docs/Oblivion_decompiled.c` 出力側のみ |
| 9 | v141 V143i | 5:3 の差を生む Ghidra 判定要因 | **確定（相関）** | 本質は **decompiled.c 生成時点と台帳の世代差**（data参照のみ impl 520 中 **362 が台帳在・decompiled 無し**）。可観測因子は典型 prologue（31/32 有ヘッダ vs 非典型 127/488）と E8 呼出し（7/7）。パディング・参照条件は 8/8 同一 |
| 10 | v139 V143-a | **bit14 名称最終確定（save 除外判定）** | **確定** | **save 除外判定ではない**（`[0xB06150]` 参照6件に save 系0・読手62に save/writ 系0を独立再計測）。名称 = **「管理外/一時（unmanaged / not-in-world）」**で最終化、`Deleted` 説・`save gate` 説はともに棄却 |
| 11 | v139 V143-b | `0x9C3C10` スコープテーブル走査 | **確定** | `.rdata 0xAEC758` の1 dword（magic `0x19930522` の per-function FuncInfo クラスタ、参照者 `0x9C3C29` の trampoline 1系統） |
| 12 | v142 残点1 | player 手スロットグローバル 3 種の所有関係 | **確定** | **単一の静的 0x154B ブロック**（推定根 `0xB33C34`、直接参照0＝算出一致）: `0xB33C80`=+0x4C の 16×0x10 スロット配列（`FUN_00478730` が `memset(0x100)`）、`0xB33D10`=+0xDC、`0xB33D60`=+0x12C。多重所有なし |
| 13 | v142 残点2 | `FUN_00667be0` の関数同定 | **確定** | **`PlayerCharacter` vtable slot83（`vt+0x14C`、行1170）**。`[this+0x5C8]` に 0x154 attach block、`[this+0x5CC]` に 0xDC を構築し `Show1stPerson` を一時 1 に強制して再構築。消費は `+0x14C` の vcall 18件 |
| 14 | v142 残点3 | 装備アタッチブロック消費者（Process slot263） | **確定** | slot263 = `+0x41C`。High/MiddleHigh 実体 **`0x42BB10`（`[this+0x150]` ゲッタ）**、Base/Low/MiddleLow = 恒0スタブ `0x6F7070`。**消費者 = `FUN_005fcab0`(2) / `FUN_006575b0`(4) の計6サイトのみ** |
| 15 | v142 残点4 | `0x4A7980`（String set）の所有権 | **確定（残1）** | **必ず新規 malloc にコピー**（record ポインタ保持の経路なし）。所有権は先頭バイト **`'S'`(0x53)** マーカーで管理し、'S' のときだけ旧値を free。`obj[0]/obj[1]` の読み手役割は V-161f |
| 16 | v142 残点5 | 「読者ゼロ」の動的検証 | **確定（限定付き）** | 静的なハードコード読手は0（v142 維持）だが、**動的読手 `GetGameSetting`(opid `0x1100`, alias `GetGS`, impl `0x513FC0` → `FUN_0055E000` の実行時キー lookup) が実在**。コンソール帯には無い。到達性は実機入力依存で静的判定不能 |

---

## §1 手法・入力

- 着手時 `python "$env:TEMP\opencode\probe_harness.py" selftest` → **4/4 PASS**
  （`bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl=0x504560`）。
- 中間スクリプト: `%TEMP%\opencode\v151_*.py`（契約 §2 の接頭辞タスクID）＋出力 `v151_*_out.txt`。
  - 前セッション: `v151_1`〜`v151_13`（SysWindow / tag / RTTI TD / 0x9C3C10 / ctx / Script ctor / extra0x12 / FUN_004dc270 初走査）
  - 本追記セッション: `v151_14`（FUN_004dc270 全体アセンブル・questゲート3・typeinfo・8impl境界）、`v151_15`（bit14 62読手・配列参照・手スロット globals・FUN_00667be0・slot263・0x4A7980・GetGS 文字列）、`v151_16`（ゲート極性・quest 文字列・Script vtable×offset・FUN_004f9e50/9df0・decompile 5:3 初回集計）、`v151_17`（5:3 クロス集計・globals 訂正・slot83・slot263 vcall・0x4A7980 tail・opcode idx419・slot 実装・入力チェーン）、`v151_18`（FUN_004a7980 decompile・GetGameSetting 呼出し・`+0x44` 文脈・slot27 thunk・globals 文脈・vtable 0xA73B58・vcall 文脈・台帳×decompiled 2x2）、`v151_19`（FUN_0055E000・静的ブロック根・slot83 消費18・`lea +0x44` 全走査）、`v151_20`（slot30 実体・loader の複写文脈）
- 手法:
  1. `probe_harness.Ctx`（`rd`/`cstr`/`enclosing_fun`/`dec_index`/`dec_body`/`dec_search`/`text_imm32`）＋ capstone 5.0.7（skipdata=True）の実アセンブル。
  2. 画像全体の abs32 / E8 / 文字列 xref センサス（`data.find` バイト列走査、`e8 rel32` 再計算）。
  3. 共有キャッシュ `tmp/re/cache/{vtable_index.tsv, named_funcs.txt, opcode_table.json, call_targets.json}` を先に引く（再走査しない）。
  4. **ESM 本体は再スキャンしない**（277,504,985 B ×3 禁止）。ESM 値は既存成果物（`plan.md` L921 / `s50_*`）の引用のみ。
- 制約遵守: `docs/`・既存 `tmp/re/*.md`・`TODO_re49.md`・台帳・git・Supermemory・sync は**非編集**（読取のみ）。本ファイルのみ新規作成。

---

## §2 各点の詳細

### §2-1 (点1 / v141 V143a) SysWindow のクラス同定 — **「SysWindow」= コンソール実行モード名で確定（vtable 欠如は非多相のため）**

**単一トンの構造（`v151_5` / `v151_6`）**

- ゲッタ `FUN_00582160(char create, param_2)`: `DAT_00b3a6e0 == 0 && create` のとき **`malloc(0x134)` → `call 0x5802b0`（ctor）→ `mov [0xB3A6E0], eax` → `FUN_00581cc0(param_2)`**（decompiled L288674-288708）。`call 0x582160` = **763**。
- ctor `FUN_005802b0` は **`mov dword [esi], ebx`（=0）以外の vtable ストアを 1 件も持たない**（範囲 0x5802B0..0x5806D0 の .rdata/.data 系 imm32 は `mov [edi],0xa3d604`（NiAlphaProperty・別オブジェクト）1 件のみ、`v151_1` P2）。→ **このオブジェクトは非多相（vtable 不存在）**であり、RTTI 名が取れないのは「ストア漏れ」ではなく構造上の必然。
- コンパイラモード名テーブル（`.data 0xB0A110`、`v151_3` P2）:
  `[0xB0A110]='DefaultCompiler'` / **`[0xB0A114]='SysWindowCompileAndRun'`** / `[0xB0A118]='DialogueCompileAndRun'`（各 ptr は `.rdata` 文字列。`0xB0A110` の .text 参照は `FUN_00500280@0x50038F` の **1 件のみ**）。
- `FUN_00500280(param_1, param_2=seed, param_3=sourceId)`（L198554）: `puVar7 = (&PTR_s_DefaultCompiler_00b0a110)[local_5c=param_3]`（L198621）＝ **sourceId → モード名**。
- **呼出し経路は 1 系統のみ**: `call 0x500280` = **1**（`FUN_004fbf00` L195511 のみ）→ `FUN_004fbf00` の callers = **2 件すべて `FUN_00586000` 内**（L292178 / L292243）、どちらも **`param_3 = 1`**:
  - L292178（インライン実行系）: `iVar4 = FUN_00582160(0,1)` → `FUN_004fbf00(*puVar9, 1, *(iVar4+0xbc))` = **SysWindow 単一トンの `+0xbc`（選択中 REF）を seed として渡す**。
  - L292243（`bat` ファイル実行系）: `FUN_004fbf00(*puVar9, 1, 0)`。
- ⇒ **sourceId=1 が "SysWindowCompileAndRun" を指す**（表[1]）。`FUN_004fbf00` 内 `if (param_3 == 1) DAT_00b361ac = 1`（L195513-195514）＝ **コンソール由来のコンパイル時のみトレースフラグ 0xB361AC を立てる**（b10 §5.3 のトレースフラグと同一）。

**機能面（`FUN_00586000` = ディスパッチ本体、`call 0x586000` = 1）**

`v151_4` P4 の push 文字列チェーン（0x5862B0..0x5871DF）で全コマンド面を列挙:
`|` `dof`/`dof %f` / `usz`/`usz %i` / `clr` / `cls` / **`tag`** / `reload strings` / `reload HUDReticle` / `reload HUDSafeZone` / `TEH` / `hudreticle_enemy_health` / `reload %s` / `delete %s` / `front`(`Frontmost: %s`) / `stack`(`%i: %i`) / `close all menus` / `visible %i`(`Is Visible: %i`) / `exit menus` / `emergency texture release` / `playerpos %f %f %f` / `repair %i` / `set trait %s %s %f` / `depth`(`Max Depth/Cursor Depth`) / `new fonts` / `old fonts` / `use workbook` / **`bat`**(`bat %s`、`> %s` で echo) — いずれも **UI/メニュー/デバッグ系コンソールコマンド**。

**結論**: `0xB3A6E0` 単一トン = **`SysWindow`（コンソール／システムウィンドウ。クラス名は RTTI ではなくコンパイラモード名 `SysWindowCompileAndRun` とコマンド面から確定）**。vtable は**元から持たない非多相オブジェクト**（size 0x134）。+0xbc = 現在選択中の `TESObjectREFR` 系（v141 残点(2)の確定を踏襲）、コンパイル seed と `tag` の両方に使う。

### §2-2 (点2 / v141 V143b) コンソールコマンド `tag` の用途 — **「選択 REF の名前+FormID を印刷しつつ、0xB3FAB0 が 3D ノード配下か否かで表示分岐する"タグ表示"」**

実測（`v151_6` P1、`FUN_00586000` 内 0x5863ED..0x5864A0）:

```
0x5863ED push 0xa69f3c('tag') / push edx(buf) / call 0x9836c9 (__mbsicmp) / test eax / jne 0x5864b7
0x586403 mov ebp,[ebp+0xbc]                      ; ebp = SysWindow の選択中 REF
0x586421 mov eax,[ebp+0xc] / push eax / mov ecx,ebp / call 0x4da2a0   ; 名前取得
0x586434 push eax / lea eax,[esp+0x1c] / push 0xa4d700 ('"%s" (%08x)') / push eax / call 0x402e30 (sprintf)
0x586444 mov edx,[ebp] / mov eax,[edx+0x154] / mov ecx,ebp / call eax  ; vtable slot85
0x586454 push eax / push 0xb3fab0 / call 0x560920 / add esp,8
0x586462 cmp eax,esi(0) / jne 0x586484           ; 1回目で見つかったら 2回目をスキップ
0x586466 … slot85 再取得 → mov eax,[eax+0x1c] → push eax / push 0xb3fab0 / call 0x560920
0x586484 fld [0xa30634] … push … call 0x572850     ; 表示/登録へ
0x58649a push esi / call 0x401f20 (free)
```

- **slot85 (`vtable+0x154`) = `FUN_00422de0` = `return *(this+0x3C)`**（decompiled L25087-25091、`vtable_index` TESObjectREFR#1440 の slot85 実体と一致）＝ **3D ノード getter**（+0x3C = NiNode、v142 §2-2 と同一規約）。
- **`FUN_00560920(param_1, param_2)` = `Contains(list, item)`**（L262004-262021）: `param_2` の vtable slot1 で head 取得 → `*(node+4)` で走査 → `item==param_1` なら `param_2` を返す。呼び出し形は全 site で `FUN_00560920(&DAT_00b3fab0, <list>)`（例 L6036 `FUN_00560920(&DAT_00b3fab0,uVar32)`）＝ **item = グローバル `0xB3FAB0`（bss 内の埋め込みオブジェクト実体）、list = 2 引数**。
- したがって `tag` は: (1) `FUN_004da2a0(REF)`（`__RTDynamicCast` 由来の名前 getter、244 callers）で名前を取得、(2) `"%s" (%08x)`（名前 + FormID）を sprintf、(3) **`[REF+0x3C]` のノード配列に `0xB3FAB0` が含まれるか**を問う、(4) 済みで `FUN_00572850(…, 0, DAT_00a30634)`（唯一 caller = 本ディスパッチ、`v151_7` P1/P8）へ渡す。
- 残る未解明: `0xB3FAB0` の正体（45 の .text abs32 参照・`call 0x572850`=1）と `FUN_00572850` の書式/出力先 → **§3 残点 V-161a として継続**。

### §2-7 (点7 / v141 V143g) RTTI TD 名解決不能 → **1 件を特定（台帳欠落候補）**

`v151_4` P6（COL → CHD → BaseClassDescriptor の pTD を全走査）:

- TD 文字列（`.?A…@@`、160B キャップ）= **1,546 件**。GT の TypeDescriptor **1,547 件**との差 1 = 下記の長大名 TD。
- CHD 解析: **成功 1,479 / 失敗 1**。失敗の base pTD は **1 種類のみ**:
  - **`0xB2CF30`** namebytes = `.?AV?$NiTStringTemplateMap@V?$NiTPointerMap@PBDV?$NiPointer@VCachedGeometry@TallGrassShaderProperty@…`（**100B 読取で切り詰め＝実名はさらに長いテンプレート**）
  - 影響を受ける CHD = `.?AV?$NiTStringPointerMap@V?$NiPointer@VCachedGeometry@TallGrassShaderProperty@@@@@@`（1 件）
- ⇒ v141 の「57 クラスの base に入り `.?AV...@@` 正規表現に該当せず → 台帳欠落 1 件」の**実体はこの 1 TD**（正規表現の `+@@` 終端・長さキャップで落ちる長大 NiT テンプレート名）。`cache/vtable_index.tsv` への記載有無は §2-7b で追記（走査中）。

### §2-11 (点11 / v139 V143-b) `0x9C3C10` スコープテーブル — **`.rdata 0xAEC758` の 1 dword に確定（MSVC C++ EH FuncInfo クラスタ）**

`v151_1` P4 / `v151_2` P2-P4:

- 画像全体の abs32: **`0x9C3C10` = `.rdata 0xAEC758` の 1 件のみ**（`.data` 0 件）。`0x9C3C18` = `.text 0x659F93` の 1 件。
- `0x659F90` 局所: `int3… / push -1 / push 0x9c3c18 / mov eax,fs:[0] …` ＝ **`FUN_00659f90`（MobileObject 系 dtor）の SEH 登録が handler `0x9C3C18` を指定**。
- ハンドラ本文（`v151_2` P2）:
  ```
  0x9C3C10: mov ecx,[ebp-0x10] ; jmp 0x4e2920      ; EH クリーンアップ funclet（TESObjectREFR dtor へ tail-jmp）
  0x9C3C18: mov edx,[esp+8] ; lea eax,[edx-8] ; mov ecx,[edx-0xc] ; xor ecx,eax
            call 0x9811e2 (__security_check_cookie)
            mov eax, 0xAEC75C ; jmp 0x980EA4        ; FuncInfo を EAX へ載せて共通 EH ハンドラへ
  ```
- `.rdata` 周辺（`v151_2` P3）: `0xAEC750: {1, 0xFFFFFFFF, 0x9C3C10, 0x19930522}` / `0xAEC75C: {0x19930522, 1, 0xAEC754, 0}` … ＝ **magic `0x19930522`（MSVC EH magic 系）を持つ per-function FuncInfo クラスタ**（同型は `0xAEC730`（→ handler `0x9C3BF9: mov eax,0xAEC730; jmp 0x980EA4`）・`0xAEC790`（`0x9C3C40/0x9C3C4B` を指す）にも存在）。`0xAEC730` の .text 参照 = `0x9C3BFA` の 1 件、`0xAEC75C` = `0x9C3C2A` の 1 件、`0xAEC790` = `0x9C3C7E` の 1 件。
- **結論**: `0x9C3C10` は「登録先の孤カラ thunk」ではなく、**`.rdata 0xAEC758` に格納された EH スコープ/クリーンアップ対象アドレス**であり、参照者は `0x9C3C29` の trampoline（=`FUN_00659f90` の登録 handler `0x9C3C18` 経由）1 系統だけ。funclet 本体は「`[ebp-0x10]`（局所 REFR）を this にして `TESObjectREFR::~TESObjectREFR()` へ tail-jmp」するクリーンアップ。v139 の「方向は入向」裁定を維持しつつ、**登録先（.rdata スコープテーブル）を `0xAEC758` に特定**した。

### §2-3 (点3 / v141 V143c) `FUN_004dc270`（選択 → フォーム解決）のアルゴリズム — **解明（4 ステップ + 特例1）**

実測（`v151_14` P1、`0x4DC270..0x4DC347` 全アセンブル）:

```
0x4DC270 mov ecx,[0xb333c4]            ; ecx = g_thePlayer（GT §4 の確定名）
0x4DC282 cmp ecx,esi(0) / je 0x4dc297  ; player 未構築ならスキップ
0x4DC28C push 1 / call 0x660110        ; thiscall this=g_thePlayer, arg=1
0x4DC293 mov [esp+0x14],eax            ; 正解候補 = FUN_00660110(1) の戻り
0x4DC2A0 cmp ebx,[esp+0x14] / je 0x4dc306   ; x == それ → 0x4DC306 へ
0x4DC2A6 movzx edi,word [ebx+0x14]     ; 個数 u16
0x4DC2B0 mov ecx,[ebx+0x10]            ; ポインタ配列
0x4DC2B3 sub edi,1  →  末尾から逆順走査
0x4DC2C0 mov edx,[esi] / mov eax,[edx+4] / mov ecx,esi / call eax   ; vtable slot1 = ノード列挙
0x4DC2D0 cmp eax,0xb35acc / je 0x4dc2f0 ; センチネル同定
0x4DC2DA mov eax,[eax+4] / jne 0x4dc2d0 ; +4 連結で残りを走査
0x4DC2E0 sbb eax,eax / and eax,esi → esi  ; 該当なら要素を保持
0x4DC2F8 mov ebx,[ebx+0x1c] / jne 0x4dc2a0 ; 次ノードへ（+0x1c = next）
0x4DC310 mov ebp,[esi+0xc]              ; 採用要素の +0xC = 解決対象
0x4DC317 mov edx,[ebp] / mov eax,[edx+0x170] / call eax   ; slot0x170 (GetBaseForm 相当)
0x4DC324 test eax,eax / je 0x4dc34d    ; 0 → 下 fallthrough（後述の戻り先）
0x4DC335 cmp byte [eax+4],0x22         ; formType == 0x22 = AMMO
0x4DC339 jne 0x4dc34d
0x4DC33B mov eax,[ebx+0x1c] / push eax / call 0x4dc270  ; AMMO なら次のノードで再帰
```

- **`FUN_00660110`** = `__thiscall(this, char)`（`v151_10` P7）: `if (arg != 0) return *(this+0x5d0); else return FUN_00612220();` → 本サイトは **`[g_thePlayer + 0x5D0]`** を取得。
- **`formType 0x22 = AMMO`** は formType 表 `0xB05E00` の実測（`v151_11` P3: `ft 0x22 nameptr=0xA3BB78 name='AMMO'`）。
- **センチネル `0xB35ACC`**（.text 参照9件、`v151_13` P1）: getter `FUN_004b2470 = mov eax,0xb35acc; ret`（`0x4B24E0`）、構築/登録系 `0x9E26C0: mov ecx,0xb35acc; push 0xa43c18; ...; call 0x70e220`、`Contains(FUN_00560920)` の item 引数として `0x4E42FD` / `0x69FE63` 近傍にも出現 → **"名前付きグローバル単体オブジェクトを、スロット1 の連結列が保持しているか"で要素を選ぶ判定子**。
- **呼出し 34**（`v151_12` P3）: `FUN_005821f0`(SysWindow)×2 / `FUN_00538ec0`×3 / `FUN_0066d930`×2 / `FUN_0068f450`×2 / `FUN_005806d0`×1 …（v141 §3 の SysWindow `+0xbc` 書手と一致）。
- 戻り値の3分岐: (a) GetBaseForm が 0 → 要素+0xC をそのまま返す / (b) formType ≠ AMMO → 同上 / (c) formType == AMMO → `[x+0x1c]` の次のノードで再帰（結果が 0 なら (a) の値にフォールバック）。

**結論**: 「入力 `x`（+0x10 配列 / +0x14 u16 個数 / +0x1c next の鎖）を後ろから走査 → スロット1 の列に `0xB35ACC` を含む要素を1つ採用 → その +0xC を GetBaseForm で検証しつつ AMMO のときだけ鎖を進める」という**単一パス探索 + AMMO 限定再帰**である。残るのは入力 `x` の型名と `[g_thePlayer+0x5D0]` の同定（§3 V-161b）。

### §2-4 (点4 / v141 V143d) コンパイル ctx フィールドの意味論 — **解明（quest ゲート3サイトの極性とエラー文を確定）**

**(a) quest ゲートの実体 = `ctx+0x38` の下位バイト比較（3 サイト = b03 の3ゲートと 1:1）**

| サイト | 関数 | 判定形 | エラー文（`.rdata` → xref） |
|---|---|---|---|
| `0x4FDC92` | `FUN_004fdaf0` | `cmp byte [eax+0x10],0 je skip` → `cmp byte [ecx+0x38],0 je skip` → `je 0x4fddf3` | `0xA4A4B0 = "Reference function '%s' requires explicit reference in quest ..."`（xref `0x4FDDF9`） |
| `0x4FF91E` | `FUN_004ff710` | 同型（`[eax+0x10]` = flagA / `[edi+0x38]` = quest バイト） | `0xA4B524 = "Invalid block type for quest script."`（xref `0x4FF925`） |
| `0x4FFF94` | `FUN_004ff710` | 同型 ＋ 追加条件 `cmp dword [esi+0x414],ebx; jne skip` | `0xA4B2F8 = "Reference function requires explicit reference in quest scri..."`（xref `0x4FFFA3`） |

- 3サイトとも **`flagA(行+0x10) != 0` かつ `ctx+0x38 != 0` のときだけエラー**（`je` で2回スキップ、v151_16 P1）。エラー文が "quest script" を指名するため、**`ctx+0x38 (byte) != 0 ⟺ quest script** が極性として確定。
- `ctx+0x38` の供給元 = **`Script+0x28`（SCHR scriptType の下位バイト）**、`ctx+0x39` = `Script+0x29`（上位）: `FUN_00500170` が `mov al,[ebp+0x28] → [esi+0x38]` / `mov cl,[ebp+0x29] → [esi+0x39]`（`0x500193` 起点、`v151_16` P1 の writer `0x500193`）＝ v141 §2.2 の写像と一致。
- ⇒ **quest バイト = scriptType の下位バイト**。ESM 分布 `{0:2031, 1:265, 256:97}`（`plan.md` L921、ESM 再スキャンせず引用）と突合すると quest 判定になるのは **下位バイト≠0 の scriptType=1（265件）** 相当で、`0x100`(97件) は下位バイト 0 のため非 quest 扱い。

**(b) ctx フィールド対応表（実測マージン）**

| ctx offset | 初期化 / 供給 | 書き戻し | 判定 |
|---|---|---|---|
| `+0x04`, `+0x1C`, `+0x24` | ctor `FUN_004fcd60` が 0 化（`0x4FCDBF/0x4FCDBC/0x4FCDC2`） | `param_2[7](+0x1C)` は文ごとに `*piVar1` 代入（L198535） | 文脈一時変数 |
| `+0x20` | ctor が **`0x4000` バイトのバッファを確保**（`push 1; push 0x4000; call 0x401e20` → `mov [esi+0x20],ebp`、`0x4FCDD8`） | — | コンパイル作業領域（レコード +0x20 = SCDA 相当の位置） |
| `+0x28..+0x38`（5 dword） | **シード = `Script+0x18..+0x28`（SCHR 5 フィールド）**（`0x500319` 系、v141 §2.2 で独立再現） | `FUN_004fc850` が `mov [esi+0x28], ecx`（ecx = ctx+0x38）で **host `Script+0x28` へ dword のまま**（v141 §2.3） | unused/refCount/compiledLength/lastVarIndex/scriptType |
| `+0x34` | `FUN_00500170` が `mov [esi+0x34],edx`（`edx ← Script+0x24` lastVarIndex、**`0x5001A0`** 実測 / `0x500193` は `mov [esi+0x38],al`） | 同（v141 §2.2） | lastVarIndex の作業コピー |
| `+0x38/+0x39` | `Script+0x28/+0x29`（scriptType 下位/上位） | `Script+0x28` へ dword 書き戻し | **quest 判定バイト（上記 (a)）** |
| `+0x50` | `FUN_00500170` が `param_2+0x14`（= `ctx+0x50`）を文リストとして走査（L198532） | — | 文リスト先頭 |
| コンパイル末尾 | `param_2[0xc] = param_2[9]`（`ctx+0x30 ← ctx+0x24`、L198541）→ `FUN_004fc850(param_2)` | | 末尾コミット（write-back） |

- `ctx` は **レコード相当レイアウト**（ヘッダが `+0x28..` に来る）なので、レコード読込用 `FUN_004fc850`（`v151_10` P5: `param_1[6..10] = *(param_2+0x28..0x38)`）をコンパイル完了時にも流用できる（**1 関数2用途**）。
- 残点: `ctx+0x2C`（シード = refCount）と `ctx+0x30`（シード = compiledLength → 末尾に `ctx+0x24` で上書き）の**コンパイル中の更新経路**は未特定（§3 V-161c）。

### §2-6 (点6 / v141 V143f) extra type 0x12 の `+0xC` 初回書手 — **解明（唯一書手 `FUN_0041f090` + 供給元 = `TESScriptableForm+0x4`）**

- **extra 0x12 = `ExtraScript`**（RTTI 確定、`v151_9` P2: `.?AVExtraScript@@` vtable `0x00A35884` / COLL `0x00AB5378`）。ctor = **`FUN_00429f20`**（`*(obj+1)=0x12`、`*obj=&PTR_FUN_00a35884`、`obj[3]=arg` → **+0xC = 引数**、`obj[4]=0` → +0x10 = 0）、size **0x14**（`FUN_00447df0` の `malloc(0x14)` 系ではなく `FUN_00401f00(0x14)` 直呼、`v151_10` P1）。
- **+0xC の唯一の恒常書手 = `FUN_0041f090(param)`**（`v151_10` P1）:
  ```
  iVar2 = FUN_0041e210(0x12)          ; GetExtraData(0x12)
  if (iVar2 != 0) { *(iVar2+0xc) = param; return; }   ; 既存なら上書き
  else { alloc 0x14 → FUN_00429f20(param) → FUN_0041e0a0(node) }  ; 無ければ新規生成
  ```
  **呼出し = 19**（`v151_11` P1 全列挙）: `FUN_00426a90`(オペコード分岐 case 0x12) / `FUN_004282a0`(コピー) / `FUN_0046a020` / `FUN_0048e060`×4 / `FUN_00491ce0`×6 / `FUN_004ae220` / `FUN_004d70e0` / `FUN_005ea380` / `FUN_006701b0`×3。
- **供給元 = `TESScriptableForm +0x4`**（RTTI キャストで確定、`v151_16` P2 相当の台帳照合）:
  - `PTR_PTR_00b02f9c` = **`TESForm`**（`vtable_index.tsv` 行1318、typedesc `0x00B02F9C`）
  - `PTR_PTR_00b0331c` = **`TESScriptableForm`**（行1485、vtable `0x00A3C490` **4 スロット**、typedesc `0x00B0331C`）
  - `PTR_PTR_00b03358` = `Script`（行1213）/ `00B03054` = `TESObjectREFR`（行1440）/ `00B03070` = `Actor`（行490）
  - 呼出し形は全て `FUN_009832e6(x, 0, &TESForm, &TESScriptableForm, 0)` → 戻り `+0x4` を `FUN_0041f090` へ渡す（`L83803/83832`＝AddItem 系、`L168507/168515`＝`FUN_004d70e0`、`L28467/28470`＝`FUN_00426a90`）。⇒ **`TESScriptableForm+0x4` = `Script*` メンバ**。
- **+0x10 は別系統**: 書手 `FUN_0041f130`（呼出し **20**、`*(extra+0x10) = FUN_004fbdc0()` の戻り）＝ **イベントリスト**（`FUN_004fbdc0` 呼出し 23、0x14B ノード工場 — `v158` §0-3 の確定を本稿でも引用）。ctor は `+0x10 = 0` で初期化するため、+0xC（Script）と +0x10（イベントリスト）は独立。
- 複製経路: `FUN_00428b10 = ExtraDataList::CopyListForReference` の `case 0x12`（`v151_10` P3）は **`+0xC` は旧ノードから継承、`+0x10` は `FUN_004fbdc0()` で再生成**（＝`v158` §0-3 と一致）。

**結論**: 「初回書手」は **`FUN_0041f090`（19 呼出し）** 1本で、値は必ず `TESScriptableForm+4` から来た `Script*`（または既存 extra の上書き）。`TESScriptableForm` は **4 スロットの独立 vtable を持つ"script 可能性"コンポーネント**で、`TESForm` から `__RTDynamicCast` で所在を解決して `+4` を読む方式。

### §2-7b (点7 の追補) 台帳欠落の最終判定 — **「欠落」は不成立（`0xB2CF30` は台帳に実在）**

- `cache/vtable_index.tsv` を直接照合: **行473** = `name=NiTStringTemplateMap<V?$NiTPointerMap::PBDV?$NiTPointer::VCachedGeometry::TallGrassShaderProperty...>` / mangled `.?AV?$NiTStringTemplateMap@V?$NiTPointerMap@PBDV?$NiTPointer@VCachedGeometry@TallGrassShaderProperty@@@@@@...` / vtable `0x00A90370` / 7 slots / COLL `0x00ACFB98` / **typedesc `0x00B2CF30`**。
- したがって v141 の「台帳 `cache/vtable_index.tsv` の欠落 1 件」は**誤り**。実際は (i) TD 文字列走査時の **100B キャップ／正規表現終端**で落ちた1件（v151_4 P6 の通り）、(ii) 台帳側は mangled 名の `@V...` 連続を `::` に畳んで記録している（行473 の name 列の `:::` 表記）——という**表示上の問題**。GT の TypeDescriptor **1,547 件** と TD 文字列走査 **1,546 件**の差1も、この長大名1件の読取キャップで説明可能。
- ⇒ 点7 = **[確定: 台帳欠落なし。v141 の「欠落1件」は走査キャップ由来の誤認]**。

### §2-8 (点8 / v141 V143h) ラベル欠落3件の Ghidra 反映可否 — **「Ghidra 側は既に3関数とも存在」（v141 の前提を訂正）**

- `cache/named_funcs.txt`（**Ghidra FunctionManager 台帳**、`func_list_new.txt` コピー）を実照合:
  - `4524: 00509d70,FUN_00509d70` / `4364: 00505120,FUN_00505120` / `4501: 00508340,FUN_00508340` — **3件とも台帳に実在**（比較: `4521 FUN_005097a0` / `4522 FUN_00509b30` / `4631 FUN_00511c10` も同様）。
- よって v141 §5.1 の前提「**埋め込先（直前の Ghidra 関数）= `FUN_00509b30` 等**」「Ghidra が関数化せず吞込」は **Ghidra プロジェクトの現状に対しては不正確**（`dec_index`＝`docs/Oblivion_decompiled.c` の `// FUN_` ヘッダのみを見て判定したため）。
- 欠落しているのは **`docs/Oblivion_decompiled.c` の出力**（ヘッダ・hex 文字列とも0件、v141 実測）だけで、Ghidra 上では独立関数として存在する。
- ⇒ **反映可否の回答は「新規作成不要」**。運用は (a) decompiled.c のみ再生成すれば解消、(b) 再生成しないなら従来どおり capstone 代替（b19 §3.2）—— のどちらか。Ghidra 側への手作業追加は**不要**。

### §2-9 (点9 / v141 V143i) 「5:3 の差を生む Ghidra 判定要因」 — **要因 = 典型 prologue の有無（8/8 一致）+ decompiled.c 世代差**

実測（`v151_14` P4/P4b/P4c、表オペコード impl 8 件）:

| impl | decompiled ヘッダ | 先頭バイト | 直前 0xCC パディング | data 参照 | E8 呼出し |
|---|---|---|---:|---:|---:|
| `0x5097A0` | **有** | `55 8b ec 83 e4 c0`（frame） | 1 | 1 | 0 |
| `0x510C70` | **有** | `6a ff 68 …`（SEH） | 0 | 1 | 0 |
| `0x510E60` | **有** | `55 8b ec` | 2 | 1 | 0 |
| `0x513C90` | **有** | `6a ff 68 …`（SEH） | 9 | 1 | 0 |
| `0x511C10` | **有** | `6a ff 68 …`（SEH） | 0 | 1 | 0 |
| `0x509D70` | **無** | `53 8b 5c 24 10`（push ebx） | 9 | 1 | 0 |
| `0x505120` | **無** | `81 ec 08 02 00 00`（sub esp） | 10 | 1 | 0 |
| `0x508340` | **無** | `8b 44 24 0c 56`（mov eax,[esp+0xc]） | 9 | 1 | 0 |

- **コード参照・パディングは 8 件とも同一条件**（E8=0 / data 参照1 / 直前に CC あり）→ v141 の通り「コード参照の有無では説明できない」を**再確認**。
- 分けるのは**先頭プロローグのみ**: ラベル有 5 件 = `55 8b ec`(2) + `6a ff 68`(3)、ラベル無 3 件 = いずれも非典型。**これは MSVC/Ghidra の typical function-start パターン（frame / SEH prologue）と 8/8 で一致**。
- 逆説の残る点: Ghidra 台帳（`named_funcs.txt`）には **8 件とも実在**（§2-8）。したがって 5:3 の差は「Ghidra が関数を作ったか」ではなく **`docs/Oblivion_decompiled.c` の生成時点の関数集合**にあり、典型的 prologue のない3件が**後から**（本プロジェクトの解析で）関数化されたか、出力時に吞まれたかのどちらか。**静的には「生成時点差」とまでは特定できない**（= v141 の「ヒューリスティクス／手動作成」仮説のうち、**手動作成側に寄る**が、日時証拠は残らない）。
- コード実行には無影響（opid は `.data` テーブル経由で解決）— v141 §5.3 を維持。

### §2-10 (点10 / v139 V143-a) bit14 名称の最終確定 — **「save 除外判定」は否定。名称は「管理外/一時（not-in-world）」で確定**

**(a) 独立再計測（本タスク `v151_15` P1/P1b）**

- `shr reg,0xe` 読取形 = **62 件**（v139/v158 の 62 と一致）。enclosing 59 種、うち **`save|writ|serial` を含む関数名 = 0 件**。
- `[0xB06150]` abs32 参照 = **6**（`FUN_004492e0` / `FUN_0046b2c0` / `FUN_0046c640`×2 / `FUN_009dbe90` / `FUN_00a18500`）、`[0xB0614C]` = **2**、`[0xB06158]` = **10** — **`v158` §5 の 6/2/10 と完全一致**（走査者は register / dereg / ClearData / leak 検査 / CRT init・atexit のみ、**save 系 writer 0 件**）。
- したがって v139 §6-3(e) が確定条件に挙げた「`[0xB06150]` 配列の用途 = save 書出除外判定か」は **[否定]**（v158 §5 の裁定を独立再計測で支持）。

**(b) 名称の最終化**

- set 対象（v139 §6-3(b) 全19件）は (i) 実行時生成 `TESObjectREFR`（ctor `0x4D9A70`、malloc 0x58、AddItem/イベントリスト系）、(ii) stack/global の `Script` 単体、(iii) 起動時走査で cast 成功した `Actor` — いずれも**セル登録・永続化対象外**。
- 読手62件の内訳（v139 §6-3(c)）: dtor 群（`TESForm 0x46C2B6` / `TESObjectREFR 0x4E2978` / `MobileObject 0x659FC8`）で**ワールドデタッチ＋保留集合除去をスキップ**、`Disable 0x50A25E`、`DeleteForm 0x45C7FC`、仮想 `FUN_0046B4C0`(data ptr 82件) が偽を返す、実行時 **clear は不在（set-only）**。本タスクの「save 系を呼ぶ読手0件」を加えて、**セーブ/ロードの serializer 経路には一切関与しない**。
- 「Deleted」説は v139 §6-3(d)（`DeleteForm` が setter を呼ばない・`'Delete'` 文字列はキーボードキー名テーブル）で棄却済み。本タスクの save 否定により「save 除外フラグ」説も棄却。
- **最終名称 = 「管理外/一時（unmanaged / not-in-world）」[確定]**。救済: フラグの主用途は「**正規のワールド管理下（`[0xB0614C]` 登録配列・セルの参照リスト）に無いインスタンス**」のマーカーであり、セーブ除外はフラグで行うのではなく、**そもそもセルの参照リストに載らないという構造で成立**する。

### §2-5 (点5 / v141 V143e) Script の残フィールド（`+0x2C` / `+0x34..+0x3C` / `+0x40` / `+0x44`） — **解明（各件の書手・読手を特定。+0x44 は BaseExtraList でない）**

- `Script` size = **0x50**（factory `FUN_00447df0` case `0xd`: `FUN_00401f00(0x50)` → `FUN_004fbaa0`、`v151_11` P2）。ctor `FUN_004fbaa0` は **`+0x18..+0x4C` を全部0化**（`param_1[6..10]`, `[0xb..0xf]`, `[0x10..0x13]`、`v151_9` P5）。

| offset | 内容 | 書手（実測） | 読手（実測） | 永続性 |
|---|---|---|---|---|
| `+0x18..+0x28` | SCHR 5 フィールド | コンパイル書き戻し `FUN_004fc850`（`v141` §2.3） | 各所 | **保存あり**（slot9 = `0x4FA810` 系、v141 §4.3 実測 `0x4FA818: lea eax,[esi+0x18]; push eax; push 0x52484353('SCHR'); push 0x14; call 0x46AE40`） |
| `+0x2C` | **所有 C 文字列（ソーステキスト）ポインタ** | **`FUN_004f9e50`**（`v151_16` P3b）: `[this+0x2c]` が非0なら allocator `0x401E40` で解放 → `strlen(arg)` → `0x401E20` で確保 → 複写。`FUN_004fc7a0` は src の `+0x2C` が非0なら `FUN_004f9e50(src+0x2c)` で複製、0 なら dest を free（`v151_10` P5） | コンソール/`bat` 系がソース行を格納（`FUN_004f9e50(local_1e39+1)` / `FUN_004f9e50(acStack_1238)`、`v151_8` P4） | **保存なし** |
| `+0x30` | **所有バイト列（コンパイル済みコード）** / `+0x20` = その長さ | **`FUN_004f9df0`**（`v151_16` P3b）: `[this+0x30]` を free → `[this+0x30]=0` → `[this+0x20]=arg(len)` → `0x401E20` 確保して arg2 を複写 | `FUN_004fc850`（レコード読込/書き戻し） | **保存は SCHR 外**（バイト列は SCDA 系が別） |
| `+0x34/+0x38/+0x3C` | **浮動小数3つ** | **slot5 = `0x4FA510`**（台帳 `FUN_004fa2d0` 内包）: `fldz; xor eax,eax` で `+0x18..+0x24` と `+0x2C/+0x30` を0化し、`fst/fstp` で `+0x34/+0x38/+0x3C` を 0.0 化（= **リセット系**、`v151_17` P7） | **slot27 = `0x4FBDB0`（`push ecx; call 0x4fbb60` スルー）→ `FUN_004fbb60`** が `fcom/fld/fstp [esi+0x38]` `fld/fstp [esi+0x3c]`（`0x4FBE19..0x4FBEE9`）＋ **GMST `fQuestDelayTime`（文字列 `0xA49E80`、xref `0x4FBD0D`）** と比較 | **保存なし** |
| `+0x40/+0x44` | **`{value, next}` リスト先頭**（先頭ノードは Script 自身のこの8バイト） | 複写 = `FUN_004fc040(src+0x44 → dest+0x40, owner)`（loader `0x4FC8F7-0x4FC8FF`: `lea ecx,[esi+0x40]; lea edx,[edi+0x44]; call 0x4fc040`、`v151_20` B） | slot27 `FUN_004fbb60` が `piVar11 = param_1+0x10` で走査。アイテムは **0x10B**（`FUN_00401f00(0x10)` → `FUN_004fbfd0` が複写、`FUN_0067b1e0` へ挿入）。項目エラー文 = `"Referenced object %d on Script (%08X) '%s' is invalid. Script will not be executed."` | **保存なし** |
| `+0x48/+0x4C` | int / flag byte | 複写 = `FUN_004fa780(src+0x3C → dest+0x48)`（`0x4FC904-0x4FC90D`、`v151_20` B） | slot10 = `0x412960` が `fild dword [ecx+0x48]`、共有 impl `0x412A80` が `test byte [ecx+0x4c],1`（slots 10/28/31/32/52/53） | 未確認 |

- **slot30 = `0x4D7030` = フラグ bit0x400 ゲッタ**（`mov eax,[ecx+8]; shr eax,0xa; and al,1; ret`、`v151_20` A）→ vtable 経由で `+0x44` を参照しない。なお `v151_16` P3 で slot30 由来と出た `0x4d70f2` の `lea ebx,[esi+0x44]` は**走査範囲が次関数 `FUN_004d70e0` に跨いだ誤検出**（本タスクで打ち切りを修正）。
- **`+0x44` は BaseExtraList でない [否定寄り]**: `.text` 全域で `lea …,[reg+0x44]` 直後に extra ベース関数（`0x41E210/0x41E900/0x41E920`）を呼ぶサイトは **18 件**（`v151_19` P4）で、**Script クラスタ（`0x4FA000..0x4FD100`）からは 0 件**（唯一の `lea …+0x44` は上記 `0x4FC8FB` = リスト複写の引数）。18件はすべて `push 0x14/0x12/0x17/0x2c/0x2b/0x31/0x39/0x3a/0x32/0x3a…`（extra type 指定）を伴う **TESObjectREFR/Actor 系**の処理で、`FUN_004d7240 = add ecx,0x44; jmp 0x41E900`（v141 のシード経路）も REFR 上の `ref+0x44` 用。⇒ **同じ数値オフセットでもクラスごとに別意味**（Script = リスト next / REFR = BaseExtraList）。汎用 extra 関数が Script を this として受ける経路は未検出（§3 V-161e に最小限残す）。

### §2-12 (点12 / v142 残点1) `DAT_00b33d10` / `DAT_00b33d60` / `DAT_00b33c80` の所有関係 — **3つは「単一の静的 0x154B ブロック」の部分アドレス（推定根 `0xB33C34`）**

実測（`v151_15` P2 で **0xB333D10 等は誤記** → `0xB33D10` 等に訂正、`v151_18` P5 / `v151_19` P2）:

| グローバル | abs32 参照 | 使われ方 |
|---|---:|---|
| `0xB33C80` | **5**（`FUN_00478730` / `FUN_0047ab90` / `FUN_0047bc40`×3） | `FUN_00478730` が `push 0x100; push 0; push 0xb33c80; call 0x981630`＝**`memset(0xB33C80, 0, 0x100)`**（同時にインスタンス側 `memset(this,0,0x154)`）。`FUN_0047bc40` は `shl eax,4 / lea eax,[eax+0xb33c80]`＝**16×0x10 のスロット配列**としてインデックス参照（`0xB33C84/88/8C` は各スロットの +4/+8/+0xC）、idx **6..8** に特殊分岐（`cmp ebx,6 / cmp ebx,8`） |
| `0xB33D10` | **1**（`FUN_00478780` @`0x478829`） | `cmp esi,0xb33d10` → 不一致なら `lea eax,[ebx+0xdc]; cmp esi,eax` と**同値比較** |
| `0xB33D60` | **1**（`FUN_00478780` @`0x478793`） | `cmp esi,0xb33d60` → 不一致なら `lea ecx,[ebx+0x12c]; cmp esi,ecx` と同値比較 |

- **`0xB33D10 − 0xDC = 0xB33D60 − 0x12C = 0xB33C34`**（算出一致）。さらに `0xB33C80 = 0xB33C34 + 0x4C` で、その `0x100` バイト（`+0x4C..+0x14C`）は 0x154B の末尾 `+0x150`（owner 逆ポインタ、v142 §2-3）の直前まで exactly 整列 → **3グローバルは根 `0xB33C34` の静的 0x154B ブロックの内部アドレス**（根そのものへの直接参照は abs32 **0件**＝算出値である点に注意）。
- **初期化**: `FUN_00478730`（attach block 構築ルーチン）が**どのインスタンス構築時にも** `memset(0xB33C80,0,0x100)` を実行 → 静的スロット配列は最初の attach block 構築（player 初期化）で0化される。
- **多重所有 = 無し**:1/3 人称の instance は `[Player+0x5C8]`（0x154）と `[Player+0x5CC]`（0xDC）に別体で存在（§2-13）、静的側は本3グローバルの単一ブロックのみ。`FUN_00478780` は「渡された sub-block が静的側（`0xB33D10`/`0xB33D60`）か、instance 側（`this+0xDC`/`this+0x12C`）か」を判別する比較器として働く。

### §2-13 (点13 / v142 残点2) `FUN_00667be0` の関数同定 — **`PlayerCharacter` vtable slot83（`vt+0x14C`）の実装 [確定]**

- `.rdata` に `0x667BE0` を含む箇所 = **1 箇所 `0xA73B58`**（`v151_15` P3 → `v151_18` P6 照合）: `vtable_index.tsv` 行1170 **`PlayerCharacter` / vt `0x00A73A0C` / 239 slots** → `(0xA73B58 − 0xA73A0C)/4 = **83**`。E8 直接呼出し = **0**（間接 vcall 専用）。
- 本文（`v151_15` P3 + `v151_18` P6 tail）:
  1. `vtable+0x170`（GetBaseForm 相当）→ `[0xB33B00]` のフラグ確認 → `call 0x5227a0` / `call 0x6670c0`
  2. `malloc(0xDC)` → ctor `0x70B780` → `or word [ebx+0x18],0x40` → **`[this+0x5D0]` と比較・置換**（`0x667C85`、旧値は vtable[0] 経由で解放）
  3. `mov ecx,ebp; call 0x4d7c50` で9 dword をコピ → `push 0x154; malloc; call 0x478730`（**attach block 構築**）→ **`mov [ebp+0x5c8], eax`（`0x667E2E`）**
  4. `push 0xdc; malloc; call 0x473eb0` → **`mov [ebp+0x5cc], eax`（`0x667E5A`）**
  5. `[ebp+0x588]`（Show1stPerson）を保存 → **強制 `=1`** → `call 0x659f30` → 旧値へ復元（= **1st person を一時的に強制してモデル/装備を再構築**）
  6. `[0xB36BB8]` / `[0xB33A1C]` → `call 0x435830` → `call 0x475d80(this=0x5CC, …, player)` → `[this+0x58]` の vtable `+0xEC`/`+0x104` 呼出し
- **判定**: v142 の「player の 1st person body 用ブロック生成」仮説は**構造的に正しいが、関数名義は "spawn 経路" ではなく `PlayerCharacter` の仮想メソッド（slot83）**。slot83 (+0x14C) の vcall 形呼出しは `.text` 全域で **18 件**（`v151_19` P3、enclosing 全列挙: `FUN_0043af30/00441ef0/0044e720/004aa1f0/004abe50`×2/`004df1a0`/`00560ac0`/`005f6330`/`00622820`×2/`00646020`/`00649340`/`0064bbc0`/`00658df0`/`0067cdb0/0069f1e0/007d6b00`）。受信クラスの照合は未実施（§3 V-161g）。

### §2-14 (点14 / v142 残点3) 装備アタッチブロック消費者（Process vt slot263） — **消費者 = 2 関数 6 サイト [確定]**

- **slot263 = `vtable + 0x41C`**（263×4）。`vtable_index.tsv` で **slots ≥ 264 の 5 クラスのみ**（`v151_15` P4）:

| クラス | vt | slots | 実装 |
|---|---|---:|---|
| `BaseProcess` | `0x00A6F464` | 322 | `0x6F7070`（`enc=FUN_006f7070`、恒 0 相当の共有スタブ、直接 call 21） |
| `LowProcess` | `0x00A720A4` | 347 | `0x6F7070` |
| `MiddleLowProcess` | `0x00A72D64` | 348 | `0x6F7070` |
| `HighProcess` | `0x00A71814` | 358 | **`0x42BB10`**（`enc=FUN_0042ba90`、E8 直接 call 0） |
| `MiddleHighProcess` | `0x00A72684` | 358 | **`0x42BB10`** |

  → **実体ゲッタ = `0x42BB10`**（v142 §4-2 の `+0x150` getter thunk と一致）＝ `[this+0x150]`（owner ref）を返す系。Base/Low/MiddleLow にはフィールドが無い（恒 0）。
- **消費者（`.text` 全域の vcall 形 `mov reg,[reg+0x41C]` → 直近 `call reg` スキャン = 6 件、`v151_17` P4 + `v151_18` P7 文脈確認）**:

| 関数 | サイト | 受信オブジェクト | 使われ方 |
|---|---|---|---|
| `FUN_005fcab0` | `0x5FD655`, `0x5FD66A` | `ecx = [esi+0x58]` | 1回目: 戻り null チェック → 続行可否。2回目: 戻りを `push; push esi; mov ecx,0xB3BD00; call 0x679240`（コンテナ登録系）へ |
| `FUN_006575b0` | `0x6576C0`, `0x6576EF`, `0x657721`, `0x657799` | `ecx = esi` | 戻りを `malloc(0x4C)` 製品へ / `call 0x6a0980` へ引数渡し / `g_thePlayer+0x5E0` と比較 |

- それ以外に vcall 形は無い（`call dword [mem]` 形は本バイナリに不在 = `v158` §1 の罠3 と同条件）。直接 E8 呼出し = **0**（`0x42BB10`）。

### §2-15 (点15 / v142 残点4) `0x4A7980`（String set）の所有権 — **「毎回 heap コピー＋先頭バイト `'S'` 所有マーカー」[確定]**

`v151_18` P1（decompile L133691-133753）＋ `v151_15` P5（アセンブル）:

- 签名: `__thiscall FUN_004a7980(undefined4 *obj, char *new)`。`obj[0]` と `obj[1]` は**同一 allocation 内の2ポインタ**。
- 手順: (1) `len_old = strlen(obj[1])`（null なら 0、`ebx=0`）(2) `buf = malloc(len_new + ebx + 1)`（old あり = `len_new+len_old+2`、old なし = `len_new+1`、`lea eax,[eax+ebx+1]`）(3) **new を `buf + ebx` へ複写** (4) `obj[0] = buf + ebx`（＝new 先頭）(5) `ebx != 0` のとき **old を `buf+0` へ複写 → `buf[0] = 'S'(0x53)` → `old[0] == 'S'` なら `free(old)`（`0x401F20`） → `obj[1] = buf`** (6) `new == NULL` なら `obj[0] = 0` で終了。
- **v142 の問いへの回答**: ESM/GMST 適用時の文字列は **必ず新規 malloc にコピーされる**（record メモリをポインタで指し続ける経路は無い）。**所有権の判定は先頭バイト `'S'`** — ヒープ由来（本関数が作ったもの）だけを free し、record 由来（先頭が `'S'` でない）は free しない＝**二重解放しない**。新しい buf には必ず `'S'` を書いて所有権を主張。
- 呼出し = **9**（`v151_17` P5）: `FUN_0040d4d0`（主ループ）×4 / `FUN_004a7d60` / `FUN_004a8800` / `FUN_004a8cf0` / `FUN_00514100` / `FUN_005143c0`。v142 の適用経路 `0x4A7D60 → 0x4A7980` と整合。
- 残: `obj[0]`（new 先頭）と `obj[1]`（old ベース＋'S'）の**読み手側の役割分担**は未特定（§3 V-161f）。

### §2-16 (点16 / v142 残点5) 「読者ゼロ」の動的検証 — **静的読者は0を維持、ただし動的読手 `GetGameSetting` が実在 → 「ゼロ」は静的範囲に限定**

- **スクリプトオペコード表（`0xB0AF48` 系）idx419 実測**（`v151_17` P6）: `name='GetGameSetting'` / **`alias='GetGS'`** / `opid=0x1100`（`0x1000` 帯 = スクリプト関数369のうち1つ）/ `flagA=0` / **`argn=1`** / `impl=0x513FC0` / dispatcher `0x4FDE30` / cond 0。画像内の `'GetGS'` 文字列はこの alias の1箇所のみ（`0xA4E860`、`.data` 参照1件）で、**コンソール帯（idx31–162 / opid `0x100`–`0x183`）には存在しない**。
- `impl 0x513FC0` の呼出し（`v151_18` P2）: `0x4FAE80`（引数ゲート、GT）→ **`0x55E000`** → `0x4A78D0` → `0x579B60`（コンソール/トレース出力）。
- **`FUN_0055E000` = 実行時キーによるハッシュマップ lookup**（`v151_19` P1）: `vtable+4` で `key` をハッシュ → バケット `[this+8]` → 連結を `vtable+8` の比較で走査 → 一致時 `*out = [node+8]` して 1 を返す（`ret 8`）。
- ⇒ **判定**: (a) **8 件（`s*Description`）を固定文字列として読むコードは無い**（v142 §1-5 の静的走査結果を維持）。(b) しかし **`GetGameSetting`（`impl 0x513FC0` → `FUN_0055E000`）は「任意のキー名を実行時に受け取って中央マップを叩く」読手として構造上実在**するため、**スクリプトが該当キー名を渡せば到達し得る**。(c) コンソール直接入力（`GetGS` 系コマンド）はコンソール帯に無いため不可。
- ⇒ v142 残点5 の最終裁定: **「読者ゼロ」は"8キーをハードコードした読手が存在しない"という静的事実としてのみ成立**。実機での到達性は実行時入力（スクリプトの文字列）に依存するため、**静的解析では正証も反証もできない**（動的検証は要実機、本タスクでは未実施）。

---

## §3 残留不明点

| ID | 内容 | 起票先 |
|---|---|---|
| V-161a | `tag` コマンドの item **`0xB3FAB0` の正体**（`.text` abs32 45件・`call 0x572850`=1 は判明、構築経路とクラス名が未特定） | 次ラウンド |
| V-161b | `FUN_004dc270` の**入力ノード `x` の型名**（+0x10 配列 / +0x14 u16 / +0x1c next）と `[g_thePlayer+0x5D0]` の型（`FUN_00667be0` が 0xDC オブジェクトで比較・置換する同一物と推定） | 次ラウンド |
| V-161c | コンパイル ctx の `+0x2C`（シード=refCount）と `+0x30`（シード=compiledLength → 末尾 `ctx+0x24` から上書き）の**コンパイル中の更新経路** | 次ラウンド |
| V-161d | `Script+0x40` リスト項目（0x10B、`+8` float / `+0xC` int）の正確な意味論（エラー文 "Referenced object …" の中身）と `+0x48/+0x4C` の意味 | 次ラウンド |
| V-161e | **TESForm 系の汎用 extra 関数が Script を this として受ける経路の最終排除**（Script クラスタ内は0件を確認済、汎用側の受信型は静的限定） | 次ラウンド |
| V-161f | `FUN_004A7980` の `obj[0]`（new 先頭）/ `obj[1]`（old ベース＋`'S'`）の**読み手側の役割分担** | 次ラウンド |
| V-161g | `PlayerCharacter` slot83（`+0x14C`）の消費18件の**受信オブジェクトクラス**照合（Player only or Actor 系全体） | 次ラウンド |
| V-161h | `docs/Oblivion_decompiled.c` に無いが Ghidra 台帳にある **362 件（data参照のみ impl）と v133「本文なし365」の完全突合**（残3件の内訳） | 次ラウンド |
| V-161i | `0x9C3C10`/`0xAEC758` 系 EH スコープテーブルの**同型クラスタ全数**（0xAEC730/750/790 のみ確認・全体像は未走査） | 次ラウンド |

---

## §4 自己検証

- **`python "%TEMP%\opencode\probe_harness.py" selftest` → 4/4 PASS**（着手時＋完了時。`bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl=0x504560`）。
- **内部整合（既存成果物との一致）**:
  1. `shr reg,0xe` 読取形 = **62**（本タスク独立計測）= v139 §6-3(c) の 62、v158 §0-5 の 62 と一致。
  2. `[0xB06150]` abs32 = **6** / `[0xB0614C]` = **2** / `[0xB06158]` = **10** = v158 の 6/2/10 と一致。
  3. `call 0x41F090` = **19** / `call 0x41F130` = **20** / `call 0x4FBDC0` = **23**（v158 の 23 と一致）/ `call 0x41E210` = **257** / `call 0x4A7980` = **9** / `call 0x667BE0` = **0**（間接のみ）。
  4. `named_funcs.txt` = 21,597 行、`dec_index` = **20,465** 件、data参照のみ opcode impl = **520 = 158（ヘッダ有）+ 362（ヘッダ無）**、うち台帳非在 = **0**（＝欠落362はすべて台帳在）→ v133「本文なし365」と 362 がほぼ一致。
  5. RTTI 照合: `vtable_index.tsv` 1,715行から `00B02F9C`=TESForm(1318) / `00B0331C`=**TESScriptableForm**(1485, 4 slots) / `00B03358`=Script(1213) / `00B03054`=TESObjectREFR(1440) / `00B03070`=Actor(490) / **`00B2CF30` = 行473 実在**（§2-7b）/ `00A73B58` → PlayerCharacter(1170) slot83。
  6. 8 impl の prologue/参照条件は 8/8 再計測（E8=0・data参照1・CC パディング有）で v141 の主張と矛盾しない（§2-9 表）。
  7. quest ゲート3サイト（`0x4FDC92`/`0x4FF91E`/`0x4FFF94`）は b03 の 3 サイト（L197229/L198214/L198420）と数・極性とも 1:1。
  8. Process slots≥264 = **5 クラス**（GT の "206スロット超の9クラス" とは別集計軸 = slot263 対象）。

---

## §5 Android 移植への示唆

`SysWindow`（`0xB3A6E0` 単一トン、size 0x134、非多相）は「コンソール実行モード名 `SysWindowCompileAndRun` + UI系コマンド面」から命名でき、RTTI は最初から不要。`tag` は「選択 REF の名前+FormID を整形し、3D ノード配列に特定グローバル (`0xB3FAB0`) が含まれるかで分岐する表示系」なので移植時は文字列整形＋ノード包含判定の2手順に分けられる。コンパイル ctx は**レコード相当レイアウト**（ヘッダ `+0x28..`）なので、SCHR 読込・コンパイル・書き戻しの3経路を1つの `header_view` にまとめられる（quest 判定は `scriptType & 0xFF != 0` の1バイト比較で、エラー文3種をそのまま対応付け可能）。`Script` は `+0x18..+0x28` のみ永続、`+0x2C/+0x30` は所有ヒープ（ソース/バイト列）、`+0x34..+0x3C` は quest delay タイマ、`+0x40/+0x44` は参照オブジェクトリスト——と**所有権ラベル付き**で設計でき、`0x4A7980` の `'S'` 所有マーカー方式（コピーのみ・record 非所有）もそのまま踏襲してよい。extra `0x12`(ExtraScript) は `+0xC`=Script* / `+0x10`=イベントリスト（別生成）という2系統を保持し、Script* の供給元は `TESScriptableForm+4` の dynamic_cast 解決1本に集約できる。bit14（`0x4000`, set-only）は「セル未登録＝管理外/一時」マーカーとして移植し、セーブ除外はフラグでなく「セル参照リストに載らない」構造で表現する。装備アタッチは「静的手スロット（16×0x10）＋ instance（`+0x5C8/+0x5CC`）」の対応を保ち、1/3 人称切替を `Show1stPerson` 一時強制で隠す既存パターンを再現すれば十分。`GetGameSetting` は実行時キー lookup として実装する（静的なキー専用読手は作らない）。

---

## §6 追記（継続サブエージェント 2026-10-28 / 独立再測による訂正・残点解消・補強）

> 本ファイルは **§2-1〜§2-16・§3・§4・§5 が記載済み**の状態で受け取り（継続セッション分）。既記本文は**無改変**で、本追記は**別スクリプト群 `v161_20_a..p`（16 本）による独立再測**の結果のみを追加する。
> 着手時・完了時とも `probe_harness.py selftest` → **4/4 PASS**。

### §6-0 追記の要点（既記への影響マップ）

| 既記の残点 / 記述 | 本追記の判定 |
|---|---|
| §2-2「`0xB3FAB0` = bss 埋め込みオブジェクト」「`FUN_00560920` = `Contains(list,item)`」 | **訂正**: `0xB3FAB0` = **`NiNode` の型レコード**、`FUN_00560920` = **`IsOfType(type,obj)`**（§6-1）→ **V-161a 解消** |
| §3 V-161b（`FUN_004dc270` の入力ノード型 / `[g_thePlayer+0x5D0]` の型） | **大部分解消**: sentinel = **`.?AVNiNode@@`**（vtable `0xA7E38C` / 名前 `Player1stPerson`）、フィルタ `0xB35ACC` = **`TESObjectExtraData` 型レコード**（`NiExtraData`→`NiObject`）、`+0x1C` = **next**（§6-2） |
| §3 V-161c（ctx `+0x2C` / `+0x30` の更新経路） | **解消**: `+0x2C` = **`FUN_004FD0A0` の宣言通し番号カウンタ**（this = ctx を実証）、`+0x30` = **コンパイル中は更新されず `0x50024C` で `ctx+0x24` から一括転送**（§6-3） |
| §3 V-161f（`0x4A7980` の `obj[0]`/`obj[1]` 読み手の役割分担） | **解消**: `obj[0]`=value / `obj[1]`=name。読み手 = `FUN_004A78D0`（**基準 `'S'`・`'S'`→type 6**）／GetGS `FUN_00513FC0`／Dump `FUN_004A7B10`／dtor `0x403BC0`。**map キーは登録時ポインタのまま＝lookup 無傷**（§6-4） |
| §3 V-161g（slot83 の受信オブジェクト照合） | **縮小**: `+0x14C` disp32 ロード **64** のうち**引数なしか vcall = 10**、うち player グローバルを含むのは **`0x4421AD`(`FUN_00441EF0`) 1 件**＝最有力候補（§6-5。一意決定は動的型待ち＝維持） |
| §2-8「Ghidra 台帳に 3 件実在 → 新規作成不要」 | **補強**: Ghidra MCP は稼働中だが現インスタンスは**別バイナリ**（`d1tamers_linear.img`）。Oblivion プロジェクトは `Oblivion.gpr` / `OblivionRe.gpr` の 2 系統がディスク実在（§6-6） |
| §2-10「save 系を呼ぶ読手 0 件」（関数名ベース判定） | **構造的に再確認**: save/load serializer 呼出し関数 **169** との集合交差 **0**、forward 深さ2 **0**、backward **5**（§6-7） |
| §2-6「初回書手 = `FUN_0041f090`」 | **補強**: ExtraScript ノード生成は **`SetScript` と `CopyListForReference` の 2 経路のみ**（ctor `0x429F20` の E8 = 2・間接参照 0）。`TESScriptableForm` = **`BaseFormComponent` 派生・派生クラス 0 件**（§6-8） |
| §2-12（手スロット 3 グローバル） | **補強＋v142 ラベル訂正**: `+0xDC`=`WEAP` / `+0x10C`=`AMMO` / `+0x12C`=`LIGH`。`FUN_0047AB90` は block→グローバルの**移動**（§6-9） |
| §2-5（`+0x44` は BaseExtraList でない） | **独立裏付け**: Script ctor は BaseExtraList 初期化 `0x422EE0` を**呼ばない**／sizeof `0x50` vs 裸 `TESObjectREFR` `0x58`（§6-10） |
| §2-3 / §2-4 / §2-7b / §2-13 の各結論 | **独立再測で一致**（AMMO `0x22`・quest ゲート 3 サイト・台帳行 473・slot83 = PlayerCharacter）。**訂正なし** |

### §6-1 (点2 の訂正 / V-161a 解消) `0xB3FAB0` = `NiNode` 型レコード、`FUN_00560920` = `IsOfType`

**`v161_20_o.py` / `v161_20_p.py` 実測:**

1. **`0xB3FAB0` は型レコード（type record）**。CRT 初期化スタブ `0xA09C90`:
   ```
   00A09C90  push 0xb3fa80 / push 0xa7e440 / mov ecx, 0xb3fab0 / call 0x70e220 / ret
   ```
   `FUN_0070E220(this,a1,a2)` = `[this]=a1(名前), [this+4]=a2(親)` → **`[0xB3FAB0] = 0xA7E440 = 'NiNode'`、`[0xB3FAB0+4] = 0xB3FA80`**。
   親 `0xB3FA80` のスタブ `0xA09BEF` → **名前 `0xA7E0B4 = 'NiAVObject'`**、さらに `0xB3F584`。型連鎖 = **`NiNode` → `NiAVObject` → …**（`call 0x70E220` 登録スタブ **398 件**を全走査して復元。bss のためファイル画像非バックでスタブからのみ解決）。
2. **getter が `NiNode` vtable の slot1 に載っている（決定的）**:
   `mov eax, 0xB3FAB0 ; ret` = **`FUN_0070B800`**、その imm32 参照は **`0xA7E390` の 1 件のみ** = **`NiNode` vtable `0xA7E38C` の slot1**（slot0 `0x741AE0` / slot2 `0x7616D0` / slot3 `0x6F7070`）。⇒ **slot1 = `GetType()`、返り値 `0xB3FAB0` = NiNode の型レコード**。
   同型の例: `mov eax,0xB35ACC ; ret` = **`FUN_004B2470`（`0x4B24E0`）** = **`TESObjectExtraData` の GetType**（§2-3 のセンチネルを裏付け）。
3. **`FUN_00560920(type, obj)` = `IsOfType`**（本文実測）:
   ```
   esi = arg2(obj) ; if (!obj) return 0
   eax = obj->vtable[1]                      ; GetType()
   loop: if (eax == arg1(type)) return obj    ; [esp+8] = type
         eax = [eax+4]                        ; 親型レコード
   return 0
   ```
   **caller = 168 件、全て `push obj ; push <型レコード>` の形**（例: `push eax; push 0xb3fab0` @`0x44ECAE`、`push 0xb4335c`、`push 0xb3cac0`、`push 0xba7a20`、`push 0xb3f96c`、`push 0xb35288` …）。
   ⇒ **v129 §9-a の「汎用 IsOfType(type)-based cast」が本文・caller 形状・vtable 載せの三方向で確定**。§2-2 の `Contains(list,item)`／「`0xB3FAB0` = item、2 引数 = list」は**引数の入れ替え＋実体の誤認**として**訂正**。
4. **`tag` の確定文脈**: `IsOfType(NiNode, [REF+0x3C] のノード)` → 不一致なら `[node+0x1C]` のオブジェクトで再試行 → `"%s" (%08x)`（`0xA4D700`）ラベルと `-1.0f`(`0xA30634`) を **`FUN_00572850`（唯一 caller・4 引数）** へ渡す。＝**「その REF の 3D ノード（NiNode）に名前+FormID のラベルを付ける」**。残るは `FUN_00572850` 本体のみ（既存 V-161a は本節で**解消**、代替残点 = §6-11 の V-161j）。

### §6-2 (点3 の補強 / V-161b の大部分解消) `FUN_004dc270` の sentinel・フィルタ・リンク

- **sentinel `[g_thePlayer+0x5D0]` の型 = `.?AVNiNode@@`**: vtable `0xA7E38C` → COL → **`.?AVNiNode@@`（RTTI 実測）**、ctor `FUN_0070B780`（`push 0xDC; malloc`）、名前 **`Player1stPerson`**（`0xA738A8`）。生成元は §2-13（既記）の `FUN_00667be0`（slot83）＝**「0xDC オブジェクトと比較・置換する同一物」を RTTI で確定**。
  供給: `FUN_00660110(this,flag)` = `flag ? [this+0x5D0] : FUN_00612220(this)`、`this = [0xB333C4]`。
- **フィルタ `0xB35ACC` の実体 = `TESObjectExtraData` の型レコード**（名前 `0xA43C18`、親 `0xB3FD44 = 'NiExtraData'` → `0xB3F684 = 'NiObject'`、getter `FUN_004B2470`）。⇒ **採用要素は `TESObjectExtraData` 系オブジェクトで `+0xC` が payload（form）**（§2-3 の「名前付きグローバル単体オブジェクト」を具体化）。
- **`+0x1C` = next（確定）**: ループ `mov ebx,[ebx+0x1c] ; jne 0x4dc2a0` と再帰 `FUN_004dc270([arg1+0x1c])` の**両方**が同じ「次の context」を指す。`+0x10` = ポインタ配列、`+0x14` = u16 個数（既記と一致）。
- **残る最小限**: 入力 context の実クラス名（`+0x10/+0x14` が配列/個数である点は確定。NiNode の children が `+0xAC` であることと異なるため **NiNode とは限らない**）。
- 独立一致: 呼出し **E8 34 + E9 1**（既記の 34 と一致）、`formType 0x22 = AMMO`（名前表 `0xB05E00` idx34 実測、`0x21='WEAP'` / `0x1A='LIGH'` / `0x31='REFR'` も一致）。

### §6-3 (点4 / V-161c 解消) ctx `+0x2C` と `+0x30` の更新経路

**`FUN_00500170`（write-back ドライバ、本文 `0x500170..0x50026E`）実測:**

```
ebp = arg1 = host Script / esi = arg2 = ctx
  [ctx+0x38] ← [host+0x28]  [ctx+0x39] ← [host+0x29]  [ctx+0x34] ← [host+0x24]   ; b03 既記と一致
（コンパイル本体: FUN_004FF710 / FUN_004FD2A0）
 0x500247  mov edx,[esi+0x24]
 0x50024C  mov [esi+0x30],edx            ; ★ ctx+0x30 ← ctx+0x24（一括転送）
 0x50024F  call FUN_004FD060(esi)        ; ★ ctx+0x44 リスト検証（不正で 0xA4A174 エラー → ret 0）
 0x500258  push esi / mov ecx,ebp / call FUN_004FC850   ; ★ write-back (this=host, arg=ctx)
```

- **`ctx+0x2C` の唯一の増分書手 = `FUN_004FD0A0`**: `0x4FD27B  add dword [ebp+0x2c],1` → `0x4FD288 mov eax,[ebp+0x2c]` → **`mov [edi+0x200],eax`（コンパイラ状態へインデックス格納）**。
  **`this` = ctx** を実証: 呼出し `0x4FFB5A`（`FUN_004FF710` 内）= **`mov ecx,edi ; call 0x4FD0A0`**、その `edi` は b03 サイト2 `cmp byte [edi+0x38],0` と同じレジスタ＝**ctx**。
- **`FUN_004FD0A0` の機能**: `this+0x44` のリストを **`_stricmp`（`0x98262D` → tail `0x982525` = 大文字小文字無視バイト比較、null 引数で `errno=0x16`(EINVAL) → `0x7FFFFFFF`）** で照合 → 一致なら既存インデックスを `arg1+0x200` へ、不一致なら `malloc(0x10)` の宣言ノード（`+0` name 所有 / `+8`,`+0xC` = コンパイラ状態 `+0x20C`,`+0x210` 由来）を作って `FUN_0067B1E0` で追加し `+1`。**caller = 5**: `FUN_004FD5F0` / `FUN_004FD7C0` / `FUN_004FDAF0`(b03 サイト3) / `FUN_004FDE30`(dispatcher) / `FUN_004FF710`(本体)。
- **`ctx+0x30` はコンパイル中に個別更新されない**: コンパイラ帯 `0x4FB000..0x500500` の `[reg+0x30]` 書きは Script ctor / `FUN_004F9DF0` / `FUN_004FC7A0` / `FUN_004FC850` / ctx init のみで、**ctx への書手は `0x50024C` の 1 件**。⇒ seed（旧 compiledLength）は write-back 直前に **`ctx+0x24`（作業バッファ `ctx+0x20` の実長）で置換**され、`FUN_004F9DF0(host, len=ctx+0x30, src=ctx+0x20)` として host `+0x20`/`+0x30` に入る。
- 既記 §2-4 の「`ctx+0x20` = ctor が確保する `0x4000` B バッファ」と整合し、**`+0x24` = その実長 / `+0x30` = コミット済み長**という読みを補強。
- ⇒ **V-161c 解消**。

### §6-4 (点13 / V-161f 解消) `0x4A7980` の読み手側役割分担と map キー分離

| 項目 | 実測 |
|---|---|
| **`FUN_004A78D0`（name[0] → type）** | `add ecx,-0x53`（**基準 = `'S'`(0x53)**）→ byte 表 `0x4A7954` → dword 表 `0x4A792C`。**`'S'` → `0x4A7917` = `mov eax,6`**、**`'s'` も 6**。他: `'f'`→5 / `'i'`→3 / `'b'`→0 / `'c'`→1 / `'u'`→4 / `'h'`→2 / `'r'`→7 / `'a'`→8 / 範囲外→9 |
| **GetGS `FUN_00513FC0`** | lookup → `FUN_004A78D0([esi+4])`（**改変後 name**）→ fmt `0xA4D758` `"GameSetting %s >> '%s'"`（`[esi]`=value / `[esi+4]`=name）。他: `0xA4D728` `'%i'` / `0xA4D740` `'%.2f'` / `0xA4D770` `UNKNOWN TYPE` / `0xA4D70C` `NOT FOUND` |
| **Dump `FUN_004A7B10`** | `[ebp+4]`（name）を prefix `'  '`(`0xA403A0`) の後ろへ複写 → `FUN_004A78D0` → type switch（`jmp [eax*4+0x4A7C7C]`） |
| **dtor / name 解放シンク `0x403BC0`** | **先頭が `'S'`(0x53) のときだけ `free`**（＝タグの対応物・二重解放なし） |
| **map キー（新事実）** | 登録系 `FUN_00419A70` / `FUN_0041BAE0`: `[esi+4]=name` → dup-check `0x55E000` → **insert key = そのポインタ値**。⇒ **`0x4A7980` が `Setting+4` を差し替えても map キーは登録時 static のまま** → ESM 適用・`GetGS`/`SetGS` の lookup は**元の名前で常に hit** |
| **INI への流出** | GameSetting map `0xB35574` は INI ノード系（635/637）と**別系統**・書き戻し経路なし（v160 §2/§9）→ `'S'` はファイルへ漏れない |

- **`obj[0]` = value の読み手** = GetGS/Dump の値欄（int/float/string 分岐）。**`obj[1]` = name の読み手** = `FUN_004A78D0`（type 判定）・GetGS/Dump の名前欄（**'S' プレフィックスが印字に現れる**）・dtor（所有判定）。→ **V-161f 解消**。
- 呼出し 9 件（`FUN_0040D4D0`×4 / `FUN_004A7D60` ESM 適用 / `FUN_004A8800` / `FUN_004A8CF0` / `FUN_00514100` SetGS / `FUN_005143C0`）も既記と一致して確認。

### §6-5 (点12 の補強 / V-161g 縮小) `PlayerCharacter::slot83(+0x14C)` の呼出し候補

- `+0x14C` の **disp32 ロード = 64 件**（`.text` 全域）。既記の「vcall 形 18 件」は「load→call 直近」基準、本測は「load の有無」基準のため数が異なる（**矛盾しない**）。
- うち **push を伴わない（＝`FUN_00667be0` のシグネチャ `ret` に一致）vcall = 10 件**:
  `0x4421AD`(`FUN_00441EF0`) / `0x44EC06`(`FUN_0044E720`) / `0x4AAF83`(`FUN_004AA1F0`) / `0x4AC2DC`・`0x4AC2EC`(`FUN_004ABE50`) / `0x4DF474`(`FUN_004DF1A0`) / `0x560B5C`(`FUN_00560AC0`) / `0x62289C`・`0x622967`(`FUN_00622820`) / `0x7D6B24`(`FUN_007D6B00`)。
- **player グローバル（`0xB333C4`/`0xB33398`）を同一関数内に含むのは `0x4421AD`（`FUN_00441EF0`）1 件のみ**（`0x442245 mov ecx,[0xB333C4]`、`0x4423E7 cmp esi,[0xB333C4]`）→ **最有力候補**。
- 他の候補は `fcomp` / `test al,al`（float/bool 戻り）や 1 引数 push あり＝**別クラスの同オフセット**（`0x64A11E`/`0x659758`/`0x67CDF9` 等）。
- ⇒ V-161g は「18 件の受信クラス照合」から「**ゼロ引数 10 件へ絞込・player 想定は 1 件**」まで縮小。**一意決定は動的型情報が必要**（維持）。

### §6-6 (点8 の補強) Ghidra 反映可否の環境側実測

- `ghidra-mcp list_instances`: **接続可能**（Ghidra **7.0.0 headless**、pid 29132、`http://127.0.0.1:8089`、endpoint 190、**program_count 1**）。ただし **project = `d1tamers_analysis` / open_programs = `/d1tamers_linear.img`（別バイナリ）** → Oblivion はロードされていない。
- Oblivion プロジェクトはディスクに **2 系統**: `D:\Cargo\Ghidra\Oblivion.gpr` + `Oblivion.rep`、`D:\Cargo\Ghidra\temp\ghidra_projects\OblivionRe.gpr` + `OblivionRe.rep`。
- ⇒ 既記 §2-8 の結論（**Ghidra 台帳 `named_funcs.txt` に 3 件実在 → 手動関数作成は不要**）に**矛盾しない**。追加事実として「反映するなら別インスタンスで Oblivion プロジェクトを開き直す必要がある」「再デコンパイルは `docs/Oblivion_decompiled.c` 再生成＝本タスクの編集禁止範囲」を記録（＝運用は **capstone 代替のまま**が現実的、v141 §5.3 と整合）。

### §6-7 (点9 の補強) bit14 × save の**構造的**決定打

`v161_20_b.py` / `v161_20_n2.py`（raw E8 全走査・1 パス）:

| 検査 | 結果 |
|---|---|
| `shr r/m32,0xE`（bit14 読手） | **62 サイト / 59 関数**（既記・v139・v158 の 62 と一致） |
| save `0x45B9A0`（`"Error: TESSaveLoadGame::SaveGameData() was called while loading."`）／load `0x4534D0` を**直接呼ぶ関数** | **169** |
| **この 169 関数に bit14 読手を持つもの** | **0** |
| **深さ2 forward（読手関数 → callee → serializer 呼出し関数）** | **0** |
| 深さ2 backward（serializer 呼出し関数 → 読手関数） | **5**: `FUN_0045C7A0`(`DeleteForm`)←`FUN_00465860` / `FUN_0046C300`←`FUN_0067BA90` / `FUN_004CECD0`・`FUN_004D35D0`×2←`FUN_00426A90`(AddItem 系) |

- 既記 §2-10 の「`save|writ|serial` を含む読手 0 件」（**関数名ベース**）を、**呼び出しグラフの集合交差**で再現し、さらに **forward 2 ホップでも 0 件**という強い形に更新。backward 5 件は全て「フォーム寿命管理が serializer を呼ぶ」側＝**bit14 を save の除外条件として読む構造ではない**。
- ⇒ 既記の結論（**save 非関与・名称「管理外/一時（unmanaged / not-in-world）」で確定**）を**独立手法で支持**（訂正なし）。

### §6-8 (点6 の補強) ExtraScript ノード生成の経路限定と `TESScriptableForm` の継承

- **ノード生成は 2 経路のみ**: ExtraScript vtable `0xA35884` の imm32 参照 = **2 件**（ctor `FUN_00429F20@0x429F31` / reset `FUN_00429F40@0x429F6B`）。ctor `0x429F20` の E8 = **2 件**（`0x41F0FA` = `FUN_0041F090` SetScript / `0x428BF6` = `FUN_00428B10` `ExtraDataList::CopyListForReference`、文字列 `0xA3576C` 実測）。`0x429F20`/`0x429F40` の imm32 参照 = **0 件**（間接生成なし）。`FUN_00429F40`（vtable 再設定＋旧 event list 解放）の caller = 1（`FUN_0042AE50`、`.rdata 0xA41BE8` に登録＝仮想）。
- **`FUN_004282A0` = 型別クローン・ディスパッチ**（`movzx eax,[esi+4] ; add eax,-3 ; cmp eax,0x55 ; ja ; jmp [eax*4+0x4288C4]`）で **type `0x12` case `0x4284A1` が SetScript を呼ぶ**。`FUN_00428B10` の caller = **`FUN_0048FBC0` の 5 箇所**（`0x48FFF0/0x490135/0x4902F0/0x490B8B/0x490C3C`）＝**参照生成時の extra 複写が「初回付与」の実体**（既記の `TESScriptableForm+4` 供給元と併走）。
- **`TESScriptableForm` の継承（新事実）**: COL `0xAB8528` の CHD = **base[0] `.?AVTESScriptableForm@@` (disp 1) / base[1] `.?AVBaseFormComponent@@` (disp 0)** → **`TESScriptableForm : BaseFormComponent`（単一継承）**。**`0xB0331C` を基底に持つクラス = 0 件**（`.rdata` 内 pTD 出現 10 件を BCD→CHD→COL まで逆走査、自身の COL 以外 0）→ **派生クラスなし＝フォームが所有する埋め込みコンポーネント**。vtable `0xA3C490` = slot0 `0x46EF50` / slot1 `0x60D0A0`(`ret`) / slot2 `0x46EFB0` / slot3 `0x46EFE0`（4 スロットで完結）。
- 併せて `FUN_0041F130` = **`[node+0x10] = arg`（event list 差し替え、20 caller）**、`FUN_0041F670` = **`[node+0x10] = 0`（4 caller）** を実測（既記の event list 別系統と一致）。

### §6-9 (点11 の補強 / v142 ラベル訂正) 手スロットの formType と `FUN_0047AB90` の移動

- **スロット setter の formType 要求（`cmp byte [ebx+4], imm` 実測）**:

| setter | 要求 formType | スロット | 鏡グローバル | v142 のラベル |
|---|---|---|---|---|
| `FUN_00478CA0` | **`0x21` = `WEAP`** | `block+0xDC` | **`0xB33D10`**（既記の idx9） | 「右手」**正しい** |
| `FUN_00478D70` | **`0x22` = `AMMO`** | `block+0x10C` | `0xB33D40` | 「左手」**誤り → 弾薬スロット** |
| `FUN_00478DF0` | **`0x1A` = `LIGH`** | `block+0x12C` | **`0xB33D60`**（既記の idx14） | 「アミュレット」**誤り → ライト（松明）スロット** |
| `FUN_00478E80` | （型チェックなし、idx=`(flag?1:0)+6`） | `block+0xAC`/`+0xBC` | `0xB33CE0`/`0xB33CF0` | 「手スロット」**正しい** |
| `FUN_00478EA0` | （型チェックなし） | `block+0xCC` | `0xB33D00` | 「アロー/クイバー」**維持** |

  （名前表 `0xB05E00` 実測: `0x14='ARMO'` / `0x1A='LIGH'` / `0x21='WEAP'` / `0x22='AMMO'`。⇒ `"Creatures are not allowed to wear amulets."` は `+0x12C`（LIGH）とは**別スロット**。）
- **`FUN_0047AB90` = block → グローバルの「移動」**: `lea eax,[edi+0x4c]` / `mov ecx,0xb33c80` から **4 dword を 16 回（`cmp ecx,0xB33D80`）**コピーし**ソースを0化**。caller = `FUN_005268D0`（←`FUN_0043DC00`,`FUN_00526DB0`）、`FUN_00526DB0`（←`FUN_004E1580`）。`FUN_0047BC40`（caller `FUN_00526DB0`）は `[0xB33C80 + idx*0x10]` へのインデックス書換、`FUN_00478730`（caller 3 = `FUN_00521A40`/`FUN_00523170`/`FUN_00667BE0`）が `memset(0xB33C80,0,0x100)`。
- **`FUN_00478780` の WEAP 鏡分岐の本文**（既記の「比較器」を具体化）:
  ```
  if (slot == 0xB33D10 || slot == block+0xDC) {
      [block+0x28] / [block+0x30] の NiNode を FUN_00897a90(node,0) + FUN_00477ef0(&node+0xAC) で解放
      if ([block+0x150] == [0xB333C4] && block == FUN_006600d0(player,0))
          FUN_0057B190([0xB33D84])            ; HUD 武器アイコンを H2H へフォールバック
  }
  ```
  `0xB33D84` = 根 `0xB33C34` の `+0x150`（owner 逆ポインタ）＝**既記 §2-12 の静的ブロック解釈と完全整合**（配列 `0xB33C80..0xB33D7F` = `+0x4C..+0x14B`、`+0x150` は配列外）。
- ⇒ 既記 §2-12 の「静的 0x154B ブロックの部分アドレス」に**矛盾せず**、**スロットの型制約（WEAP/AMMO/LIGH）と move 同期の 2 点を追加**。

### §6-10 (点5 の独立裏付け) `+0x44` は BaseExtraList でない

- **Script ctor `FUN_004FBAA0` の call は `FUN_0046C460`（TESForm ctor）と `FUN_0046C2E0` のみ** → **`lea ecx,[esi+0x44]; call 0x422EE0`（BaseExtraList 初期化）を呼ばない**。対して **`TESObjectREFR` ctor `FUN_004D9A70` は `lea ecx,[esi+0x44]; call 0x422EE0` を呼ぶ**（v158 §3-5）。
- **サイズ整合**: SCPT ファクトリ `0x44887A: push 0x50` → **`sizeof(Script) = 0x50`** ＝ `0x18`(TESForm) + `0x38`。裸 `TESObjectREFR` = `push 0x58` → `0x18` + `0x2C` + **`0x14`(BaseExtraList @ `+0x44..+0x57`)**。
- **TESForm 接頭辞 = 0x18**: `FUN_0046C460` が書くのは `[esi]=0xA3BE3C` / `[esi+0x10]=[esi+0x14]=0` のみ（`+4`=formType / `+8`=flags / `+0xC`=FormID は後続）→ Script の SCHR が `+0x18` から始まる配置と**矛盾しない**。
- write-back のリスト転記も既記と一致（`lea ecx,[esi+0x40]` ← `lea edx,[edi+0x44]` / `lea eax,[esi+0x48]` ← `add edi,0x3c`）→ **ctx と Script はレイアウト非対称**（`+0x3C` は ctx=リスト先頭 / Script=float `fldz`）＝**ctx は専用型**という既記 §2-4 の読みを補強。
- ⇒ 既記 §2-5 の結論を **ctor 経路・サイズ・TESForm 接頭辞の 3 系統で独立に支持**（訂正なし）。

### §6-11 新規残点（本追記で起票）

| ID | 内容 | 優先度 |
|---|---|---|
| **V-161j** | `FUN_00572850`（`tag` の終端・4 引数 `(NiNode or 0, "%s" (%08x) buf, 0, -1.0f)`・唯一 caller・本体 0x200B に文字列なし）の実体＝「ノードへのラベル付与（デバッグ表示）」の確認（既存 V-161a の代替） | 中 |
| **V-161k** | `FUN_004dc270` の**入力 context の実クラス名**（`+0x10` 配列 / `+0x14` 個数 / `+0x1C` next は確定。sentinel が NiNode だが children 位置 `+0xAC` とは不一致） | 中 |
| **V-161l** | `FUN_00441EF0@0x4421AD`（slot83 の最有力呼出し候補）の受信オブジェクトが player である事の動的確認 | 低 |
| **V-161m** | 台帳 mangled 列に無い **65 の TD**（`BaseFormComponent`/`NiProperty`/`QueuedActor` 等、自身の vtable 行を持たない群）の内訳分類 | 低 |
| （既存 V-161a / f） | 本追記 §6-1 / §6-4 で**解消**。V-161b / c / g は**大部分解消・縮小**（残は上表 k / l） | — |

### §6-12 使用証拠（本追記分）

- **新規スクリプト（すべて `%TEMP%\opencode`）**: `v161_20_a.py`〜`v161_20_p.py` の **16 本** ＋ 出力 `v161_20_*_out.txt` 16 点。
  内訳: `a` formType 表・`FUN_004dc270` 呼出し・RTTI・slot setter・SetScript 呼出し / `b` 手スロット本文・type 判定表・slot83 vtable・bit14×serializer / `c` 型レコード復元・台帳差・SetScript hosts / `d` slot setter formType・GetGS fmt・+0x14C / `e` write-back 全文・TESScriptableForm・`FUN_00667be0` tail / `f` ctx 調査・+0x14C ゼロ引数 / `g` `FUN_00500170` 全文・`FUN_00441EF0`・型レコード連鎖 / `h` `FUN_004FD0A0` 呼出し・ラベル可否 / `i` `_stricmp` 判定・ctx 初期化 / `j` `0x982525` 本文・`+0x24` 書手 / `k` `FUN_004FF710` 先頭 / `l` 台帳行照合 / `m` CopyListForReference 到達性 / `n`,`n2` bit14 save 到達（169/0/0/5）/ `o` `0xB3FAB0` 同定 / `p` GetType getter・vtable slot1・IsOfType caller 168。
- **主な実測アドレス**: `FUN_0070B800@0x70B800`（NiNode slot1 getter）/ `0xA7E390` / `FUN_00560920@0x560920`（caller 168）/ `FUN_004B2470@0x4B24E0` / 型レコード `0xB3FAB0`→`0xB3FA80`→`0xB3F584`・`0xB35ACC`→`0xB3FD44`→`0xB3F684` / 登録スタブ `0xA09C90`・`0xA09BEF`・`0x9E26CF`（`call 0x70E220` 398 件）/ `FUN_00500170@0x50024C-0x50025B` / `FUN_004FD0A0@0x4FD27B-0x4FD288`・呼出し `0x4FFB5A` / `FUN_004FD060@0x4FD060`・エラー `0xA4A174` / `FUN_004A78D0@0x4A78D0`（表 `0x4A7954`/`0x4A792C`）/ `FUN_00513FC0@0x513FC0`・fmt `0xA4D70C/728/740/758/770` / `FUN_004A7B10@0x4A7B10`・prefix `0xA403A0` / `+0x14C` ロード 64・ゼロ引数 10・`0x4421AD` / serializer `0x45B9A0`・`0x4534D0`（169 関数）/ `FUN_00429F20`・`FUN_00429F40`・`FUN_004282A0`・`FUN_00428B10`・`FUN_0048FBC0` / COL `0xAB8528` / `FUN_00478CA0`・`0x478D70`・`0x478DF0`・`0x478E80`・`0x478EA0` / `FUN_0047AB90@0x47AB90`・`FUN_00478780@0x478793,0x478829` / `FUN_004FBAA0`・`push 0x50@0x44887A`・`push 0x58` / formType 表 `0xB05E00` / Ghidra MCP `list_instances`・`Oblivion.gpr`・`OblivionRe.gpr`。
- **読取のみ（無変更）**: `v141_b19_quest_residuals.md` / `v139_b17_table2_residuals.md` / `v142_s7010_residuals.md` / `v158_slot_opcode_map.md` / `v160_ini_log_residuals.md` / `v129_shader_slot_residuals.md` / `b19_quest_byte_writer.md` / `b03_flaga_meaning.md` / `cache/vtable_index.tsv`・`named_funcs.txt` / `docs/Oblivion_decompiled.c`（部分検索）/ 本ファイル既記分。

### §6-13 自己検証（本追記分）

- `probe_harness.py selftest` → **4/4 PASS**（着手時・完了時）。
- **既記との一致検査（訂正 0 件の項）**: bit14 読手 **62** / SetScript caller **19** / `FUN_0041F130` **20** / `FUN_004FBDC0` **23** / `FUN_004A7980` **9** / `FUN_00667BE0` E8 **0** / `FUN_004dc270` 呼出し **34** / quest ゲート **3 サイト** / 台帳 **`0x00B2CF30` 行 473 実在** / slot83 = **PlayerCharacter** / Script size **0x50** / `shr *,0xe` **62** — すべて既記と一致。
- **既記との相違（本追記の訂正・追加分）**: §2-2 の `0xB3FAB0`/`FUN_00560920` 解釈（§6-1）、V-161c/f の解消（§6-3/§6-4）、V-161b/g の縮小（§6-2/§6-5）、v142 の手スロット formType ラベル 2 件（§6-9）、serializer 交差 169/0/0/5（§6-7）。
- **制約遵守**: git / `docs/` / `TODO_re49.md` / `session-memory.json` / `plan.md` / `cache/` / **既存 `tmp/re/*.md` は編集 0 件**（本ファイルへの追記のみ）、**ESM 本体は未アクセス**、新規スクリプトは `%TEMP%\opencode` のみ。

（本追記 了）
