# v316 — v313 新規残件11項の静的解析 / ai-todo #80（次束F）

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000、7,898,624 B）
- 日付: 2026-11-07 / 担当: 静的解析サブエージェント（v316 lane）
- 正式契約: `tmp/re/SAVE_LOAD_RE_FOLLOWUP_v313.md`（新指示書＝判定5段階・様式）＋ 本件固有の帯指定
- 調査項目の出典: `tmp/re/v313_save_ess_boundary_followup.md` **§7 新規残件11件**（正式な調査項目）
- 判定タグ: **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**
- クリーンルーム: OpenTES4 / xOBSE / COEF 等の逆コンパイル由来コードは参照・流用なし。既存 `tmp/re` 成果物（v309/v313/s58/v282/v217 等）は**自ラボの過去 RE 記録**として突合にのみ使用（新規の外部出典なし）。
- **実 `.ess` は不在（v313 §1.1 の8系統で確定）** → 動的照合は全項目未実施（§7）。

---

## 1. 解析対象と対象 VA 範囲

### 1.1 自帯（深解析してよい範囲）

| 帯 | 本件で実際に深解析した主要 VA |
|---|---|
| `0x459000-0x460000` | 0x459450 / **0x459570..0x45977F** / **0x45B300..0x45B668** / **0x45BA00..0x45BAAD** / **0x45D5F0..0x45D919** / **0x45D920..0x45DBB0** / **0x45E6A0..0x45E933** / 0x45F180 頭（0x45F7F0 窓）/ 0x45ADC0..0x45B2FD |
| `0x464000-0x467920` | 0x464320 / 0x464440..0x46458A（Player 生成）/ 0x464AB0 呼出し 0x465228 / **0x464E00-0x464E97** / **0x465130..0x465288** / 0x465860 頭（0x4659E0-0x465B12 窓）/ 0x466AD0・0x466B70・0x466D21（callers 文脈） |
| `0x430000-0x431000` | **0x4301A0..0x430233**（F7 旧版フォールバック） |
| `0x4D8000-0x4F2000` | **0x4E0F80..0x4E156F** / 0x4DA2A0 / 0x4DE1C0 / 0x4DE8D0 / 0x4D9A50 |
| `0x5E1000-0x5E2800` | 0x5E1FD0（call 文脈） |

### 1.2 帯外参照（浅層＝VA+命令+直近文脈のみ）

0x4067A6 / 0x416650 / 0x417040 / 0x444340-0x4445CC / 0x452150 / **0x452330..0x45239C** / **0x4610F0..0x461240** / **0x463EC0..0x464140** / 0x461680 / 0x461030 / 0x46C300 / 0x596C20 / 0x5A63F0 / 0x5D56C0 / 0x5D8F00 / 0x612740 / 0x614190 / 0x6A25E0 / 0x66A740 / 0x6FF9C0 / 0x6FFAC0 / 0x64FFF0 / 0x65A2C0 / 0x678D90 / 0x6AC420 / 0x70E560 / 0x70F010 / 0x7B4280 / 0x7C5E70 / 0x7C76D0 / 0x7C7DC0 / 0x9F0E80-0x9F10F0 / 0xA21560
（`0x452000-0x459000`・`0x460000-0x464000` も自帯外として浅層扱い）

### 1.3 v314 禁止帯（非侵入）

`0x404000-0x406000` / `0x44F000-0x452000` / `0x51B000-0x51C000` / `0x570000-0x574000`
- **深解析ゼロ**。命令境界検証のための .text 全体デコードに由来するヒット（例: 0x404CD7 / 0x405F9F / 0x571B29 / 0x572C6A / 0x57350D）は **VA+命令のみ**記録し、文脈・callers を追っていない。

---

## 2. 使用 probe/script 一覧

実行形: `chcp 65001` → `python $env:TEMP\opencode\v316_<n>.py *> $env:TEMP\opencode\v316_<n>.out`。
依存: `%TEMP%\opencode\v307_common.py`（読取専用・変更なし、`tmp/re/re_util.py` は不使用）。中間物は `%TEMP%\opencode\v316_*` のみ。

| script | 内容 | 主な結果 |
|---|---|---|
| v316_01 | FUN_464AB0 呼出し / dry bit スキャン / +0x88 スキャン / 465130・45D920・464320 callers | 464AB0 呼出し=0x465228 の1件、465130 callers 6件、+0x88 サイト（正規化前） |
| v316_02 | FUN_004E0F80 全 dump（507命令・ラベル/call/文字列注釈）＋ string-key callee 浅層 | 分岐カタログ一式、'BSX'/'REF' 文脈、0x6FF9C0/0x6FFAC0 頭 |
| v316_03 | [0xB333C4] 参照走査 / Player 生成窓 / ctor 0x66A740 浅層 / RTTI 解析 / '.?AV' 文字列 | **COL 0xAC3D80 → TD 0xB080A4 = `.?AVPlayerCharacter@@`** |
| v316_04 | FUN_45D5F0 bounds + callers push 列 + F8..F10 窓 + 0x70E560 callers | **callers = 4件**、F8..F10 命令列、ctor 0x70E560 callers 40件 |
| v316_05 | FUN_459570 全 dump（0x459570..0x45977F）＋ callers ＋ 文字列 | arg2/arg3/arg4 の契約一式 |
| v316_06 | FUN_465130 prologue+guard / 0x452330 callers・浅層全文 | 継続条件、0x452330 述語全文、**off-by-pad 第6例** |
| v316_07 | FUN_45B300 全 dump ＋ フィールド初期化抽出（初回・正規化前） | ctor 全文、対象フィールドの初期化有無 |
| v316_08 | FUN_45D920 全 dump / sprintf push 列 / FUN_465860 読取窓 / getter callers | 保存名書式と引数対応、サニタイズ、重複回避 |
| v316_09 | FUN_45D5F0 のゲート窓（0x45D650-0x45D870）＋ FUN_004301A0 全 dump | F1/F3/F6/F7 の else 分岐、FileTimeToSystemTime 経路 |
| v316_10 | FUN_00416650 callers 16件の push 列 ＋ compare 候補の head | compare カタログ16件 |
| v316_11 | FUN_0045E6A0 コピーループ全文 ＋ グローバル定数実値 | seek(0xD000,SET)/step(0x4000/0x2000,CUR)、{0,1,2} 定数表 |
| v316_12 | 補正: dry/+0x88 正規化走査、45D5F0 push 全列、B333C4 store、B38720、45B300 field、+0x7d | +0x88 実サイト7件、store 3件、field 表、+0x7d 読取2件 |
| v316_13 | 補正2: [+0x18] ソース=レジスタ走査 / or-0x200 文脈 / 0x4610F0・0x463EC0 引数窓 / +0x7d 文脈 | arg10 出所、'Level' 用 sprintf '%s %i'、+0x7d 使用 |
| v316_14 | 補正3: 文字列解決 / [0xB33B00]→[+0x18] 窓 / setter 浅層 / 45BA00 全 dump | '%s %i'、SaveGame+0x18 setter 系、+0x7d の実挙動 |
| v316_15 | 補正4: ローリング窓（[0xB33B00]→[+0x18]、or/and 0x200 の由来） | SaveGame+0x18 書込18件（bit9 なし）、0x45A530/0x45FDA0/0x461030 |
| v316_16 | **[reg+0x18] への or/and/xor/bts/add/sub 完全カタログ（530件）＋ mov 即値カタログ** | **bit9(0x200) を扱う命令0件** |

全16本、初回・再実行とも **EXIT=0**。

---

## 3. 11項ごとの判定表

| # | 調査項目 | 判定 | 一句话裁定 |
|---|---|---|---|
| 1 | +0x88 counter の dry→real 連続実行時の増加挙動 | **[部分確定]** | 増加は FUN_464AB0 1呼出しにつき **+1 のみ（dry/real 排他）**。ただし **bit9 を立てる命令が .text に存在せず dry 発火経路が未検出**、dry→real 2パスの有無は [未確定] |
| 2 | FUN_004E0F80 の各分岐の厳密な責務 | **[部分確定]** | = `TESObjectREFR::SetModel`（[+0x3C] 差し替え）。分岐を15系統にカタログ化、うち末尾の補助呼び出し実体が未同定 |
| 3 | [0xB333C4] の正確なクラス名・RTTI | **[確定]** | **`.?AVPlayerCharacter@@`**（vtable 0xA73A0C → COL 0xAC3D80 → TD 0xB080A4） |
| 4 | FUN_0045D5F0 の thumb tail 読取先とロード側受け先 | **[部分確定]** | **ロード実行(FUN_465860)は arg10=0 で thumb を読まない [確定]**。読むのは callers 4件中の **FUN_004610F0 の1件のみ**（帯外・受け先の実型は [部分確定]） |
| 5 | FUN_00459570 の第3・第4引数の契約 | **[確定]** | arg3=中央セグメントの `sprintf("%s%s",[0xB38720],seg)` 出力先、arg4="Playing Time" 以降の時刻文字列（'.ess'切断+'.'→':'）出力先 |
| 6 | FUN_00465130 内の FUN_00452330 ガードの厳密な意味 | **[確定]** | 継続条件 = **dry ‖ 452330() ‖ name=="autosave"**（dry が先行し 452330 を短絡）。452330 = プレイヤー状態＋メニューID集合の述語。**off-by-pad 第6例 0x452330** |
| 7 | FUN_0045B300 の未表示フィールド | **[部分確定]** | 初期化有無は全項目 [確定]（+0x44/+0x48=**未初期化**、+0x54..60/+0x74/+0x78=サブオブジェクト、+0x7C=0x7D、**+0x7D=formID 解決テーブル適用フラグ**）。+0x44/+0x48 の意味は [未確定] |
| 8 | name1/name2 の表示上の意味 | **[部分確定]** | `'Save N - <name1> - <name2>, Level L, Playing Time HH.MM.SS'` の第1/第2 %s。name1=TESFullName [確定]、name2=4DE8D0 出力 [確定]、**語彙（キャラ名/位置名）は [推定]** |
| 9 | 旧版ゲート（F1/F3/F6/F7 未満）の実挙動 | **[部分確定]** | 静的 else は全ゲート [確定]（明示0／FUN_004301A0 の FILETIME 代替）。**実 .ess との照合は未実施** |
| 10 | FUN_00416650 呼出元16件の比較関数カタログ | **[部分確定]** | 16件すべて compare 関数を特定、うち **11件 [確定] / 4件 [部分確定] / 1件 [未確定]** |
| 11 | 0x4000/0x2000 seek の実ファイルレイアウト上の意味 | **[部分確定]** | 命令・算術は [確定]（初回 SEEK_SET 0xD000、以降 SEEK_CUR 0x4000/0x2000、書出は got のみ）。**レイアウト解釈は [推定]・実データ照合 [未確定]** |

**判定内訳: [確定] 3件（項目3/5/6） / [部分確定] 8件（項目1/2/4/7/8/9/10/11） / [未確定] 0件（主判定） / [推定] 0件（主判定） / [確定負] 0件（主判定・副次に1件あり §4-1）**

---

## 4. VA/命令/caller-callee 根拠

### 4-1. 項目1: +0x88 counter の dry→real 挙動 — [部分確定]

**counter の全参照（0x450000-0x46FFFF の `[非esp +0x88]` 完全走査 = 7件）**

| VA | 命令 | 役割 |
|---|---|---|
| 0x45B38A | `mov dword [esi+0x88], ebx(0)` | ctor での初期化0 |
| 0x45DA7D / 0x45DA8D | `cmp [ebx+0x88],0` / `mov ebx,[ebx+0x88]` | 名前生成での読取（0なら FUN_464320 再計算） |
| 0x46442F | `mov dword [edi+0x88], eax` | FUN_464320 の max+1 代入 |
| 0x464E40 / 0x464E53 | `cmp` / `mov ecx,[esi+0x88]` | writer のガードと読取 |
| 0x464E6C | `add dword [esi+0x88], ebp(1)` | **dry 経路の増加** |
| 0x464E91 | `add dword [esi+0x88], ebp(1)` | **real 経路の増加** |

**dry/real の排他（命令）**
```
464E50: mov edx,[esi+0x18] / mov ecx,[esi+0x88] / shr edx,9
464E5C: test dl,1 / mov [esp+0x34],ecx
464E63: je 0x464e74            ; dry でなければ real 経路へ
464E65: add [esi+0x90],4 / add [esi+0x88],ebp / jmp 0x464ed1   ; dry: 計上のみ
464E74: ... 4B 書出 ... 464E91: add [esi+0x88],ebp             ; real: 書出後に +1
```
→ **1回の FUN_464AB0 呼出しでは必ず片方だけが実行され、+1 は1回** [確定(命令)]。

**呼出し回数**
- `callers_of(0x464AB0) = {0x465228}` の1件のみ（FUN_00465130 内、`mov al,0x7d` の直後のヘッダ書出）[確定]
- FUN_00465130 の callers = 0x466AF6(quicksave) / 0x466BD0(autosave) / 0x466D21(テスト保存) / 0x5024BA(コンソール) / 0x5D3246(SaveMenu) / 0x5D3438(スロット再保存) の6件、**いずれも1回のみ呼び出し** [確定]
- FUN_464320（counter 再計算）のガードは `cmp [x+0x88],0 / jne` → 0 のときのみ [確定]。**帯内に counter を0へ戻す命令は存在しない**（ctor の0x45B38A 以外）[確定]

**dry ビット（[SaveGame+0x18] bit9）の探索 — 証拠なし**
1. **`[reg+0x18]` への or/and/xor/bts/btc/add/sub 全530件（.text 全体・命令境界検証済）に `0x200` / `0xfffffdff` は0件** [確定負]。dword 形が触るビットは {0,1,2,3,4,5,6,7,8,10,11,13,14,15,16,17,18,19,26,27} のみ。
2. `[0xB33B00]` 読み込みから14命令以内の SaveGame+0x18 書き込み = **18件**、内訳は bit0/1/5/7/8/10/11/13/15/16/18 のみ（0x40D554, 0x40D56B, 0x4443A0, 0x44454A, 0x444588, 0x4445B0, 0x4445B5, 0x45CC0B, 0x45CC34, 0x45DDC9, 0x45DDF5, 0x4B7D98, 0x4CDA46, 0x4CDA74, 0x4CDA79, 0x5439CE, 0x5439E7, 0x5E87B6）[確定]
3. `or/and reg,0x200|0xfffffdff` の出現は31件だが、**直前8命令内に `[x+0x18]` 読出があるもの=0件**（read-modify-write 経路なし）[確定]
4. `mov [reg+0x18], 0x200` は 0x9847D6 / 0x9997F0（.text 尾部 CRT 内・SaveGame 無関係）[確定]

→ **dry(bit9) を立てる経路は静的に未検出** [部分確定]（`this` 引数経由の setter で8命令超間隔の RMW が存在する可能性は残る＝完全否定は未証明）。
→ **dry→real の2パスが実行序列上生じるかは [未確定]**（動態・実 .ess 無し）。生じた場合も増加は各パス +1 で合計 +2 となりうるが、静的には確定できない。

**副次 [確定負]**: `.text` 全体で `[reg+0x18]` の bit9 を set/clear する命令は存在しない（上記1）。

---

### 4-2. 項目2: FUN_004E0F80 の各分岐 — [部分確定]

**範囲**: 0x4E0F80..0x4E156F（`ret 4` @0x4E156F、507命令）。this=ebp、arg1=[esp+0x68]。
（既存成果物 v282 が「REFR 主 vtbl 0xA46C44 +0x150 = SetModel」と同定済み。本件は命令で裏付け：入口 `mov eax,[ebp+0x3c] / cmp eax,edi`＝現行 [+0x3C] と新引数の同一判定）

| 範囲 | 責務（命令根拠） | 判定 |
|---|---|---|
| 0x4E0FB2-0x4E0FC8 | 同一 ptr なら early-out。arg1==0 → `mov ecx,[0xB33A1C]; push ebp; call 0x439DC0`（ModelLoader 経由） | [確定] |
| 0x4E0FCD-0x4E0FE4 | [+0x2C/+0x30/+0x34] をローカル退避、`call 0x4D7260` → float を [esp+0x1C] へ | [確定]（内容 [部分確定]） |
| 0x4E0FED-0x4E100E | `vfunc+0x190(this)` 真なら esi=this | [確定] |
| 0x4E1012-0x4E1028 | `[+0x3C]!=0 && arg1!=0 && [+0x1C]!=0 && byte[[+0x1C]+4]==0x1E` → early-out（form type 0x1E の名称は [未確定]） | [部分確定] |
| 0x4E102E-0x4E1066 | `[0xB33A98]+0xCD4==0` かつ esi≠0 → `vfunc+0x380`（2回）→ `vfunc+0x164` | [部分確定] |
| 0x4E1068-0x4E1077 | 旧モデル `[ebp+0x3C]` の refcount++（`call [0xA28078]`=InterlockedIncrement） | [確定] |
| 0x4E108D-0x4E10E8 | 旧モデル側の解放: `vfunc+0x88` 経由の結果に対し `InterlockedDecrement(0xA2807C)` → 0 なら `vfunc+0(...,1)` deleting dtor | [確定] |
| 0x4E10EA-0x4E1118 | transform 退避: `[+0x54]/[+0x58]/[+0x5C]/[+0x60]` ＋ `rep movsd` 9 dwords（`[+0x30]` 起点＝3x4 f32 行列＋位置＋スケール） | [確定]（意味 [推定]） |
| 0x4E111C-0x4E114C | `0x7B4280(0,obj)` → `0x7C5E70` / `vfunc+0x188` 真なら `0x7B4280` → `0x7C76D0`。**0x7B4280 = `[index*4+0xB42F54]` のグローバル配列 getter（`ret`）** | [部分確定] |
| **0x4E1160-0x4E11A2** | **保存キュー投入のゲート**: `[ebp+8]>>5&1` 立ち → 0x4E11A7（全部スキップ）。esi==0 → **D220(0x4E1196)→D390(0x4E11A2)** の順。esi≠0 なら `vfunc+0x198(edi)` を判定し、真 → **D220 のみ**（jmp で D390 を回避）、偽 → 両方なし | [確定] |
| 0x4E11A7-0x4E11FD | `[ebp+0x44]` active-ref リスト: `0x41E650` 取得 → 先頭要素の deleting dtor → `[edi]=0` → `0x41F5B0` | [確定]（要素型 [未確定]） |
| 0x4E1201-0x4E1254 | `vfunc+0x188` 真 → `vfunc+0x44(this,0x2000000)`、`[ebp+0x58]` の `vfunc+8` ≤1 なら `0x64FFF0(obj,0)`（＝`[+0x17C]` setter・`ret 4`）、偽側は `0x41E630`/`0x41F590` | [部分確定] |
| 0x4E1259-0x4E1276 | esi≠0 → `0x4DC5B0 / 0x4DC8F0(0) / 0x4DCAB0 / 0x4DCCF0` の4連鎖 | [部分確定] |
| **0x4E128A-0x4E12AB** | **`push 'BSX'(0xA7D0EC) / mov ecx,ebx / call 0x6FF9C0` → 戻り `[+0xC]>>4&1` 真なら `push ebx / call 0x4DE1C0`**。0x6FF9C0 は `[esp+4]==0 → 0`、`push 0xB3F600; call [0xA2806C]`（ロック）＋`[ebx+0x14]` の探索＝**名前キーによる extra 検索** | [部分確定]（'BSX' の実体名 [未確定]） |
| 0x4E12AE-0x4E12E9 | esi≠0 → `0x65A2C0`（`[+0x58]` の `vfunc+0x18c` 経由・結果 refcount--）→ `0x4D9A50(0)`、`[ebp+0x58]` → `vfunc+0x470(...,0)`。`[0xB33398]+0x24` → `0x6AC420(0.0f,this)` | [部分確定] |
| 0x4E12F2-0x4E1316 | `[ebp+0x40]` が NULL または `byte[[+0x40]+0x26]!=1` → `mov ecx,0xB3BD00; call 0x678D90`、`[0xB3B7D0]>0` → `0x607B90(1,this)` | [部分確定] |
| **0x4E131E-0x4E1340** | **`push 'REF'(0xA3CEB0) / mov ecx,ebx / call 0x6FFAC0` → `InterlockedDecrement(&[ebx+4])` → 0 なら deleting dtor** | [確定]（'REF' の実体名 [未確定]） |
| 0x4E1342-0x4E1392 | `vfunc+0x144` → `push 3/2; call 0x977C50`（timeGetTime 相当 [推定]）で計測 → `[ebp+0x3C]` refcount-- → 0 で dtor → **`mov dword [ebp+0x3c],0`** | [確定] |
| 0x4E1395-0x4E13CF | arg1==0 のとき `[ebp+0x1c]` を `vfunc+0xF0` か `0x439DC0` へ渡し、最後に **`push 0; mov ecx,ebp; call 0x4D9310`（＝音の無効化）** | [確定]（v282 の FUN_004D9310=音 init/無効化と整合） |
| **0x4E13D4-0x4E13E0** | **`push arg1; mov ecx,ebp; call 0x4D7D10`＝[REFR+0x3C] への新モデル代入（v272 の「唯一の書込窓」と整合）** | [確定] |
| 0x4E13E8-0x4E1459 | arg1≠0 かつ transform オブジェクトあり → 新モデルへ transform 適用（`vfunc+0x84(1,obj)` / `0x897A20` / `0x707610` / `0x707980` / `0x707370(0,...)`）＋ `rep movsd` 9 dwords | [部分確定] |
| 0x4E145E-0x4E150C | arg1 の `word[+0x14]` 分の子ノード走査 → `vfunc+4` 戻りを 0xB35ACC までリンク照合 → 一致で `mov [ebx+0xc],ebp`（`[arg1+0xC]=this`）、不一致なら `new(0x10)+0x4D67C0(this)+0x6FF8A0(arg1,...)` | [部分確定] |
| 0x4E150F-0x4E1557 | `mov ecx,[0xB33B00]; call 0x45A500`（bit0 getter）→ 真で early-out。`[SaveGame+0x44]` リストを `0x41E6B0` で取得 → `0x42B460` → `0x4CCA60` | [部分確定] |

**callers 7件**（v313 踏襲）: 0x5CDC07 / 0x5F153E / 0x608C66 / 0x60E45F / 0x6121EF / 0x62505B / 0x6251EC（すべて帯外・浅層）。

**残る不確定点**: 'BSX'/'REF' の実体名、`vfunc+0x188/+0x198/+0x380/+0x44/+0xF0/+0x164/+0x16c/+0x174` の対象クラス、0x7C5E70/0x7C76D0/0x7C7DC0 の辞書操作内容。

---

### 4-3. 項目3: [0xB333C4] のクラス名・RTTI — [確定]

**生成・破棄（.text 全体の store 完全走査 = 3件）**

| VA | 文脈 | 意味 |
|---|---|---|
| 0x4067A6 | `new` → `call 0x66a740` → `mov [0xb333c4],eax` → `push 1; push 0x14; mov ecx,eax; call 0x46C300` | **初回生成**（帯外浅層） |
| 0x464553 | `push 0x804; call 0x401F00` → `call 0x66a740` @0x46453C → `mov [0xb333c4],eax` → `call 0x46C300(eax,0x14,1)` @0x464558（head 0x464440） | **再生成**（自帯） |
| 0x4497A9 | `mov ecx,[0xb333c4]` → `vfunc+0x10(obj,1)` deleting dtor → **`mov [0xb333c4], ebx(0)`** | **破棄時の NULL 化**（帯外浅層） |

**RTTI（本件の中心証拠）**
```
66A740 (ctor, head=0x66A740, 帯外浅層):
  66A767: mov esi,ecx / call 0x60e4f0          ; base ctor
  66A774: mov dword [esi], 0xA73A0C            ; ← vtable
  66A77A: mov dword [esi+0x18], 0xA73A04
  66A781: mov dword [esi+0x5c], 0xA739BC
  66A788: mov dword [esi+0x68], 0xA73988
  66A815/66A84A: mov dword [esi+0x788], 0xA735A8 / 0xA73744

vtable 0xA73A0C:
  [-1] = COL 0xAC3D80 → sig=0, thisoff=0, cdOff=0, pTD=0xB080A4, pCD=0xAC3D94
  TypeDescriptor @0xB080A4 → name = ".?AVPlayerCharacter@@"
  vt[+00]=0x60D0A0  [+04]=0x60D0A0  [+08]=0x46B800  [+0C]=0x46B870
  [+10]=0x66EAD0    [+14]=0x4D6BF0  [+18]=0x4D9B50  [+1C]=0x4D9D00
```
→ **[0xB333C4] = `PlayerCharacter` の単一インスタンス** [確定]。
`.rdata/.data` の `.?AV` 文字列走査でも `0xB080AC: .?AVPlayerCharacter@@`（＝TD+8）を確認 [確定]。
既存成果物（v217「PlayerCharacter ctor FUN_0066A740・vptr 0xA73A0C」/ v285 / s50_b13「g_thePlayer」）と**独立に一致**。

---

### 4-4. 項目4: FUN_0045D5F0 の thumb tail とロード側受け先 — [部分確定]

**callers = 4件（v313 の2件から訂正）** `callers_of(0x45D5F0) = {0x4611E8, 0x464115, 0x4643D6, 0x465A3F}` [確定]

| caller | head（帯） | arg10（先頭 push） | arg9 | arg4/name2 | arg6 |
|---|---|---|---|---|---|
| 0x4643D6 | 0x464320（IN） | `push 0` @0x4643B3 = **0** | `lea ecx,[esp+0x34]` | `push 0` @0x4643CB | `push 0` @0x4643C4 |
| 0x465A3F | 0x465860（IN） | `push 0` @0x465A17 = **0** | `lea ecx,[ebp+0x1bc]` | `push 0` @0x465A29 | `push 0` @0x465A20 |
| 0x464115 | 0x463EC0（帯外） | `push 0` @0x4640EA = **0** | `lea edx,[esp+0x30]` | `push 0` @0x464106 | `push 0` @0x4640FF |
| **0x4611E8** | 0x4610F0（帯外） | **`push ecx` @0x4611B4（ecx=[esp+0x50]＝FUN_004610F0 の arg9・dword 出力先）** | `lea edx,[esp+0x44]` | 非NULL | 非NULL |

**F8..F10 の実挙動（0x45D80E-0x45D8EE、命令）**
```
45D80E: mov eax,[esi+0x14] / mov ecx,[eax] / add eax,4 / mov [esi+0x14],eax   ; F8 = u32 size
45D818: test ecx,ecx / je 0x45d8ee                                            ; size==0 → 終了
45D823: mov edx,[esp+0xac]（=arg10）/ test edx,edx / je 0x45d8e9              ; arg10==0 → スキップ
45D832: mov ebx,[eax]（w）… 45D83A: mov edi,[eax]（h）… 45D842: mov [edx],edi  ; *arg10 = h
45D844: cmp byte [0xB33B00+0x7c],0x2e / jae 0x45d854 / add [esi+0x14],0x24    ; ver<0x2E で +0x24 skip
45D854: push 0xB25E48 / lea ecx,[esp+0x38] / call 0x70F010
45D862: push 0x70 / call 0x401F00（new）… 45D88D: call 0x70E560（画像 ctor）
        … ×3B memcpy / 0x701FC0 / InterlockedIncrement(0xA28078)
45D8E9: add eax,ecx / mov [esi+0x14],eax                                      ; arg10==0 のときは size 分だけ進める
45D904: mov eax,edi                                                            ; 戻り = 画像（arg10==0 なら 0）
```
- **ロード実行 FUN_465860（0x465A3F）は arg10=0 → thumb を読まない** [確定]
- **FUN_464320 / FUN_463EC0 も arg10=0 → 読まない** [確定]
- **読むのは FUN_004610F0（0x4611E8）のみ**。呼び出し直後 `4611ED: mov ebx,eax` で画像を保持 [確定]。同関数は 0x4611F7-0x46120A で `sprintf(dst, '%s %i'(0xA3AEB8), [0xB38720], word[esp+0x30])`＝**Level 表示文字列**を生成 [確定]。
- FUN_004610F0 は `ret 0x28`（10引数）・`mov bl,[esp+0x48]`（フラグ）で mode2 open → `call 0x45DBC0`（ヘッダ読）→ FUN_45D5F0 という**ヘッダ読取ラッパ** [確定]（帯外のため深解析は浅層に留める）。
- **受け先（FUN_004610F0 の arg9）の実型・用途は帯外のため [部分確定]**。`0x70E560` の callers は40件で保存帯内は 0x45D88D のみ [確定]。

---

### 4-5. 項目5: FUN_00459570 の arg3/arg4 — [確定]

**範囲**: 0x459570..0x45977F（`ret 0x10` @0x45977F）[確定]。
**引数の取り出し**: `sub esp,0x214` 直後に `[esp+0x218]`=arg1, `[esp+0x220]`=arg3 を読み、push 後 `[esp+0x228]`=arg4（ebx）、`[esp+0x224]`=arg2（ebp）。`mov [esp+0x1c],ecx` で arg3 をローカル保存 [確定]。

**処理列（命令）**
1. `push 0x5c; push eax(arg1+0x3c); call 0x983FD0`（strrchr 相当 [推定]）→ esi=末尾 '\\' の次＝ファイル名 [確定]
2. `strstr(filename,'Playing Time'(0xA3A0E8))` → ローカル [B+0] へ、`strstr(filename,'-'(0xA3A0E4))` → edi [確定]
3. 両者非NULL かつ `strncmp(filename,'Save '(0xA3A0DC),5)==0` でなければ fail（al=0） [確定]
4. **arg2**: ファイル名+5 から '-' までを memcpy → `0x983479`（atoi 相当 [推定]）→ **`mov [ebp],eax`（*arg2 = 連番）** [確定]
5. **arg3 が NULL なら** 2つの ',' へ進まず arg4 のみ処理 [確定]
6. **arg3 処理**: 最後の ',' とその手前の ',' の間（前頭の ' ' をスキップ）をローカルへ memcpy → **`sprintf(arg3, '%s%s'(0xA2F884), [0xB38720], <セグメント>)`** @0x4596EE [確定]
7. **arg4 処理**: `strstr('Playing Time')+0xD` 以降を arg4 へコピー → `strstr(arg4,'.ess')` があればそこで NUL 切断 → **ループで `'.'(0x2E)` を `':'(0x3A)` に全置換** @0x459740-0x45975F [確定]
8. 戻り: 成功=1（0x459761）、失敗=0（0x459765） [確定]

**callers（2件）**: 0x46408A（arg2..arg4 すべて実ポインタ、`test al,al / jne 0x46430E` else mode2 再open→ヘッダ読）、0x46436F（`push 0; push 0` = arg4/arg3 とも NULL、arg2=&local のみ） [確定]
**[0xB38720]**: .text 参照5件（0x4596CD / 0x4611FC / 0x464127 / 0x9F10EA / 0xA21560）。0xA21560 = `mov ecx,0xb38720; jmp 0x4055E0`（静的初期化トランポリン、隣接 0xB38728/0xB38730/0xB38738 と同型）、0x9F10EA は `push 0xa5e364; push 0xa5e37c; mov ecx,0xb38688; call 0x419A70` の静的初期化列 [確定]。**実行時の文字列内容は [未確定]**（BSS・動的初期化）。

---

### 4-6. 項目6: FUN_00465130 の FUN_00452330 ガード — [確定]

**FUN_00465130 内の継続条件（0x465159-0x4651A5、命令）**
```
465159: mov eax,[ebp+0x18] / shr eax,9 / test al,1 / jne 0x4651a5   ; (1) dry → 続行
465167: call 0x452330 / test al,al / jne 0x4651a5                   ; (2) 述語真 → 続行
465170: test ebx,ebx / je 0x465186                                  ; name==NULL → reject
465174: mov edi,0xA3AAD4 / mov esi,ebx / mov ecx,9 / xor edx,edx
465182: repe cmpsb / je 0x4651a5                                    ; (3) name=="autosave" → 続行
465186: fld1 / push ... / call 0x57ACC0([0xB38788]) / xor al,al / jmp 0x465843  ; reject
4651A5: mov ecx,[0xB3BF80] / xor edi,edi … call 0x683490 / 0x432860 / 0x461680 …
```
→ **続行 = dry ‖ FUN_00452330() ‖ (name=="autosave")** [確定]。
**dry 判定が 452330 より先行**し、dry 立ち時は 452330 を呼ばない（短絡）→ v313 の「dry 判定との関係が未追跡」を解消 [確定]。

**FUN_00452330 = 0x452330..0x45239C（`ret`、帯外浅層）**
```
452330: mov ecx,[0xB333C4] / call 0x5E0DC0 / cmp eax,2 / je 0x45239a(=0)
452340: mov ecx,[0xB333C4] / call 0x5E0DC0 / cmp eax,1 / je 0x45239a(=0)
452350: mov ecx,[0xB333C4] / call 0x65D140 / test al,al / jne 0x45239a(=0)
45235F: call 0x578FE0 → eax∈{0x40F, 0, 0x3F5, 3} でなければ 0
45237B: call 0x579060 → 同集合でなければ 0
452397: mov al,1 / ret        ; 45239A: xor al,al / ret
```
→ **「プレイヤー状態（0x5E0DC0）が 1/2 でなく、0x65D140(player) 偽で、かつ 0x578FE0() と 0x579060() の両方が {0,3,0x3F5,0x40F} のいずれか」を返す述語。ecx(this) は本文で即上書きされ引数無視** [確定(命令)]。
**callers = 0x465167 / 0x5BDC25 の2件**（いずれも `mov ecx,[0xB33B00]` 直後）[確定]。メニュー ID の名称は [未確定]。

**off-by-pad 第6例**: `find_head(0x452330) = 0x4522F0`（判定 False）。直前は `45232D: ret 4`（0x45232D..0x45232F の3バイト）で **CC パッド0** → 教訓の「直前≥4B CC ラン」を満たさず、0x452330 が新関数頭 [確定]（教訓4例＋0x45B300 に続く第6例）。

---

### 4-7. 項目7: FUN_0045B300 の未表示フィールド — [部分確定]

**範囲**: 0x45B300..0x45B668（`ret` @0x45B668）[確定]（`find_head(0x45B300)` は直前 CC ラン0のため 0x45ADC0 を返す＝v313 の off-by-pad 第5例を再確認）。

**対象フィールドの初期化（ctor 全文からの抽出）**

| フィールド | 初期化 | 根拠 VA | 判定 |
|---|---|---|---|
| **+0x44** | **なし** | — | **[確定]（未初期化）** |
| **+0x48** | **なし** | — | **[確定]（未初期化）** |
| +0x54 | `mov [esi+0x54],eax`（`new(0x10)`+ctor 0x45ABE0） | 0x45B4AB | [確定] |
| +0x58 | 同上 | 0x45B4D7 | [確定] |
| +0x5C | 同上 | 0x45B503 | [確定] |
| +0x60 | 同上 | 0x45B52F | [確定] |
| +0x74 | `mov [esi+0x74],edi`（`new(0x18)`+vtable 0xA39044、cap=0x64、[+0x14]=0x32、[+0x4]=配列 new） | 0x45B587 | [確定] |
| +0x78 | 同上（cap=5、[+0x14]=1） | 0x45B606 | [確定] |
| +0x7C | `mov byte [esi+0x7c], al(0x7D)` | 0x45B64D | [確定] |
| +0x7D | `mov byte [esi+0x7d], 1` | 0x45B37A | [確定] |

（+0x55..+0x57 等が「none」と出るのは1byte単位の抽出上のもので、実際は上記 dword 書き込みでカバーされる。）

**+0x7D の実挙動（本件の新規確定）**
```
FUN_0045BA00 = 0x45BA00..0x45BAAD（ret 8）
  45BA19: call 0x9812C0（memcpy: [ebp+0x14] ← ebx, 長さ edi）
  45BA21: cmp byte [ebp+0x7d],0 / je 0x45baa2        ; 0 なら素通し（長さ分だけカーソル前進）
  45BA40: mov edi,[ebx+esi*4] / mov ecx,[0xB33A98] / push edi / call 0x446B80
          ; 0x446B80 = formID ≥ 0xFF000000 判定（v309 確定）
  45BA53: mov eax,[ebp+0x74] / cmp edi,[eax+0xc] / jbe → [eax+4] の配列でインデックス解決
          （超過なら 0）
  45BA74: mov [ebx+esi*4], edi（解決後の値で上書き）→ ループ
FUN_0045F7F0（head 0x45F180）: cmp byte [esi+0x7d],0 / je 素通し / else call 0x45E0D0（要素変換）
```
→ **+0x7D ≠ 0 のときは、u32 配列の各要素を [+0x74] の解決テーブルで変換してから書き出す／0 なら素通し** [確定(命令)]。ctor で1に設定され、帯内にクリア命令なし [確定]。**正式名称（「created base objects リマップ」等）は [未確定]**。

**残る不確定点**: +0x44/+0x48 の意味 [未確定]（SaveGame 上での使用箇所を特定していない）、+0x74 テーブルの投入元（誰が書き込むか）[未確定]。

---

### 4-8. 項目8: name1/name2 の表示上の意味 — [部分確定]

**FUN_0045D920 = 0x45D920..0x45DBB0（`ret 4`）＝自動保存名生成** [確定]

**書式と引数（push 順・`add esp,0x30`＝12 push と整合）**
```
0xA3A8A4 = 'Save %i - %s - %s, Level %i, Playing Time %02i.%02i.%02i'
  arg3 (%i)  = ebx  = [SaveGame+0x88] counter            （0x45DA8D `mov ebx,[ebx+0x88]`）
  arg4 (%s)  = ecx  = [P+0x28] = name1（0x4DA2A0 の戻り）  （0x45DAA6 `mov ecx,[esp+0x38]`）
  arg5 (%s)  = eax  = [P+0x18] = name2（0x4DE8D0 出力）    （0x45DAAD `mov eax,[esp+0x28]`）
  arg6 (%i)  = edx  = Level（0x5E1FD0 の戻り, movzx edx,ax）
  arg7/8/9   = edi/ebp/esi = 時/分/秒
              0x45DA23-0x45DA7A: [Player+0x714] の累積 ms を
              ÷3600000(magic 0x95217CB1,shr 0x15) → edi(時)
              ÷60000 (magic 0x45E7B273,shr 0xE)  → ebp(分)
              ÷1000  (magic 0x10624DD3,shr 0x6)  → esi(秒)
  arg10      = &suffix（sprintf 未使用の余剰）
```
→ **表示形式 = `Save <counter> - <name1> - <name2>, Level <level>, Playing Time <HH>.<MM>.<SS>`** [確定]。

**name1 / name2 の取得**
- **name1**: `mov ecx,[0xB333C4] / call 0x4DA2A0` = `[PlayerCharacter+0x1C]` を `__RTDynamicCast(TESForm→TESFullName@0xB03298)` し、成功なら `[eax+4]`、失敗なら `""(0xA2F7EC)` [確定]。**[+0x1C] が何を指すかは [未確定]**。
- **name2**: `mov ecx,[0xB333C4] / lea eax,[esp+0x1c] / push eax / call 0x4DE8D0` → NULL なら `call 0x6ECC80` → `vfunc+0xD4`、それも無ければ `""` [確定]。0x4DE8D0 の callers は 0x45D9AB / 0x464C58 / 0x5B9065 の3件 [確定]。
- **語彙（表示上のラベル）**: name1 = キャラクタ名相当、name2 = 位置/セル名相当 と解するのが書式上もっとも自然だが、**命令だけでは確定できない → [推定]**（name2 側は 0x4DE8D0 の system(2) が `vfunc+0x174` の位置情報系を含むことの根拠のみ）。

**ファイル名化と重複回避（追加の新規確定）**
- `sprintf('%s%s%s.ess'(0xA3A88C), [0xB3F280]dir, [0xB05564]sep, name)` [確定]
- `0x985480`（strpbrk 相当）で `'\/:*<>?|"'(0xA3A898)` を検出し、**`'"'` は `'\''(0x27)`、それ以外は `space(0x20)` に置換**（`setne cl / sub cl,1 / and cl,7 / add cl,0x20`）[確定]
- 存在確認（`mov ecx,[0xB33A98]` の vfunc+4）→ 存在するなら `sprintf(' #%d', n+1)` を付けて再試行（カウンタ [esp+0x14]）[確定]

**ヘッダとの関係**: F2=name1、F4=name2（len0 は既定文字列 `[0xB38728]` をコピー）[確定]。ロード時は F2 のみ `SaveGame+0xB0` へ復元され、name2/thumb は arg=0 [確定]。

---

### 4-9. 項目9: 旧版ゲート（F1/F3/F6/F7 未満）の実挙動 — [部分確定]

version byte = `[0xB33B00]+0x7c` をゲート毎に再読込 [確定]。

| gate | VA | ver 未満時の挙動 | 判定 |
|---|---|---|---|
| F1 | 0x45D67E `cmp al,0x38 / jb 0x45d693` | 読取スキップ＋ **`45D6A3: mov dword [ecx],0`**（明示的に0を書く） | [確定] |
| F3 | 0x45D6DF `cmp byte [eax+0x7c],0x34 / jb 0x45d6fe` | ローカルを事前0（0x45D6E3 `mov [esp+0x8c],0`）→ `mov cx,[esp+0x8c]`＝**0** → *arg5=0 | [確定] |
| F6 | 0x45D789 `xor ecx,ecx / cmp byte [edx+0x7c],0x1d / jb 0x45d7a2` | **ecx=0 のまま** → *arg9=0 | [確定] |
| F7 | 0x45D7AF `xor ecx,ecx / mov edx,ecx / mov edi,ecx / mov ebp,ecx / cmp …,0x38 / jb 0x45d7d6` | 4レジスタ0 → `0x45D7DB: cmp …,0x38 / jae 0x45d7f8` で **`lea ecx,[esp+0x24] / push ecx / mov ecx,ebx / call 0x4301A0` にフォールバック**（16B を上書き） | [確定] |
| F4（参考） | 0x45D72C `test al,al / je 0x45d741` | len0 → **`mov ecx,[0xB38728]` の既定文字列をコピー** | [確定] |
| F8 以降 | 0x45D84A `cmp …,0x2e / jae / add [esi+0x14],0x24` | **ver<0x2E のときのみ +0x24 バイト skip**（F8 本体に version gate なし） | [確定] |

**FUN_004301A0 = 0x4301A0..0x430233（`ret 4`、自帯）** [確定]
```
4301B6: cmp dword [eax+0x1c],0 / je → 16B ゼロ（4301C4-4301CE）
4301D3: lea ecx,[esp+0x1c] / push ecx / add eax,0x3c / push eax / call [0xA2812C]=FindFirstFileA
4301E5: cmp eax,-1 / je … ; ftLastWriteTime を [esp+4]/[esp+8] へ退避
4301F7: push eax / call [0xA28120]=FindClose
4301FE: lea edx,[esp+0xc] / push edx / lea eax,[esp+8] / push eax / call [0xA28138]=FileTimeToSystemTime
43020E: push esi / lea ecx,[esp+0x10] / push ecx / push 0 / call [0xA28134]=SystemTimeToTzSpecificLocalTime
```
→ **旧版 F7 = ファイル最終更新時刻 → FileTimeToSystemTime → SystemTimeToTzSpecificLocalTime(lpTZ=NULL) のローカル16B** [確定]。`[slot+0x1C]==0`（未オープン）なら16B ゼロ [確定]。

**残る不確定点**: 実 `.ess` との照合（契約書 §1 ②⑤）が未実施 → 総合 [部分確定]。

---

### 4-10. 項目10: FUN_00416650 呼出元16件の比較関数カタログ — [部分確定]

`callers_of(0x416650) = 16件`（b15 成果物の計測と一致）[確定]。各呼び手の **call 直前の「最後から2番目の push」が compare 関数**（`push cmpfn / push item / mov ecx,list / call`）。

| # | caller（帯） | compare | 比較内容（本体より） | 判定 |
|---|---|---|---|---|
| 1 | 0x41717D（OUT） | **0x417040** | `[x+0x3C]` を NULL→"" に補正してバイト列比較 → `sbb eax,eax / sbb eax,-1`（3-way） | [確定] |
| 2 | 0x45AD70（IN） | **0x452150** | `movzx eax,[eax+0xa] / movzx ecx,[ecx+0xa] / cmp ax,cx`（u16 3-way） | [確定] |
| 3 | 0x45AD91（IN） | **0x452150** | 同上 | [確定] |
| 4 | 0x45ADA9（IN） | **0x452150** | 同上 | [確定] |
| 5 | 0x45D59B（IN） | **0x459450** | FILETIME 3-way（v313 確定を再確認） | [確定] |
| 6 | 0x5974B2（OUT） | **0x596C20** | `[x+0x1C]`（NULL→""）→ `jmp 0x9836C9`（strcmp 相当 [推定]） | [確定] |
| 7 | 0x5A7EF9（OUT） | **0x5A63F0** | 浮動小数を算出する本体（浅層 0x60B まで）。3-way 返却は未確認 | [部分確定] |
| 8 | 0x5D65A5（OUT） | **0x5D56C0** | 両引数を `0x565CC0` で解決し `call 0x98539F`（strcmp 相当 [推定]）へ tail | [部分確定] |
| 9 | 0x5D91D7（OUT） | **0x5D8FD0** | **push 先が関数頭でない（head=0x5D8F00 からのミドルエントリ）**、本体未確認 | [未確定] |
| 10 | 0x616278（OUT） | **0x614190** | `[x+4]` の int 比較 → 3-way | [確定] |
| 11 | 0x617054（OUT） | **0x612740** | `vfunc 経由 0x415240` で f32 を取り出し `fcom` → 3-way | [確定] |
| 12 | 0x61709E（OUT） | **0x612740** | 同上 | [確定] |
| 13 | 0x617182（OUT） | **0x612740** | 同上 | [確定] |
| 14 | 0x6171C0（OUT） | **0x612740** | 同上 | [確定] |
| 15 | 0x68F08A（OUT） | **0x6A25E0** | 大きな本体（0x6A25E0..0x6A27EB、値算出系）。3-way 返却は浅層で未確認 | [部分確定] |
| 16 | 0x6A2BDA（OUT） | **0x6A25E0** | 同上 | [部分確定] |

**内訳: [確定] 11 / [部分確定] 4 / [未確定] 1**。保存帯（0x459000-0x460000）内は #2-#5 の4件のみで、すべて [確定] [確定]。
（補足: 0x416650 本体 0x416650..0x416703 `ret 8` は帯外・v313 の記載を踏襲。`push offset` 由来の偽候補 0x9AE38E/0x9C2076 等は SEH ハンドラで除外済み。）

---

### 4-11. 項目11: 0x4000/0x2000 seek の実ファイルレイアウト上の意味 — [部分確定]

**FUN_0045E6A0 コピーループ（0x45E7F6..0x45E8BE、自帯）の命令列**
```
45E7F6: mov eax,[0xA853D0]      ; 定数 = 0 (SEEK_SET)
45E7FB: mov edx,[esi] / mov edx,[edx+0xc]        ; vfunc+0xC = seek
45E807: push eax / 45E80C: add ebp,0xffff3000    ; 残 = [esp+0x4c] - 0xD000
45E812: push 0xd000 / mov ecx,esi / call edx     ; seek(0xD000, SEEK_SET) on ESI（ctor#1 読ストリーム）
45E81F: test ebp,ebp / mov bl,1 / jle end
--- loop ---
45E830: test bl,bl / mov eax,0xaa000 / je / mov eax,0xa9000   ; chunk = 初回 0xA9000 / 以降 0xAA000
45E83E: push 1 / lea ecx,[esp+0x20] / push ecx / push eax / mov eax,[esi+4] / lea edx,[esp+0x484] / push edx / push esi
45E85A: call eax   ; read(this=ESI, &bigbuf, chunk, &got, 1)
45E85C: mov ebp,eax / add esp,0x14 / test ebp,ebp / je skip
45E865: mov eax,[edi+8] / push 1 / lea ecx,[esp+0x2c] / push ecx / push ebp(got) / lea edx,[esp+0x484] / push edx / push edi
45E881: call eax   ; write(this=EDI, &bigbuf, got, &written, 1)   ← 書くのは got のみ
45E886: test bl,bl / mov edi,0x2000 / je / mov edi,0x4000       ; step = 初回 0x4000 / 以降 0x2000
45E894: mov eax,[0xA853D4]      ; 定数 = 1 (SEEK_CUR)
45E89E: push eax / push edi / mov ecx,esi / call edx             ; seek(step, SEEK_CUR) on ESI
45E8A4: mov eax,[esp+0x20] / add edi,ebp / sub eax,edi           ; 残 -= (got + step)
45E8B0: xor bl,bl / test eax,eax / mov [esp+0x20],eax / jg 0x45e830
```
- **グローバル定数の実値**: `[0xA853D0]=0`, `[0xA853D4]=1`, `[0xA853D8]=2` → **SEEK_SET/CUR/END の定数表** [確定]
- **入力は `got+step` だけ進み、出力には `got` だけ書かれる** [確定(命令)]
- → **ソース側に 0x4000（初回）/ 0x2000（2回目以降）のギャップが存在する前提で書かれていないとバイトが失われる** [確定(命令)]
- 残サイズ初期値 = `[esp+0x4c] − 0xD000`。`[esp+0x4c]` が WIN32_FIND_DATA の nFileSize という解釈は [推定]
- **出力 .ess の総長 = nFileSize − 0xD000 − (0x4000 + (n−1)×0x2000)**（n = 読取回数）[推定]
- **実ファイルレイアウト上の理由**: 命令は [確定] だが、**XBox CON チャンク間のパディングという解釈は [推定]**、実サンプルが無いため [未確定]（契約書 §1 ③の「圧縮データ物理位置特定」相当が未実施）
- 後始末: 両ストリーム deleting dtor → `DeleteFileA([0xA280AC])` → `FindNextFileA([0xA28124])` ループ → `FindClose([0xA28120])` [確定]

---

## 5. v313/v309 既知事項との整合（矛盾と訂正・旧結論は不削除）

| # | 既存記載 | 出典 | 本件の裁定 | 裁定理由（証拠） |
|---|---|---|---|---|
| 1 | FUN_45D5F0 の callers = 2件（0x4643D6 / 0x465A3F） | v313 項目5 | **訂正: 4件**（+0x4611E8 / 0x464115） | `callers_of(0x45D5F0)` の完全列（v316_04） |
| 2 | 「off-by-pad 4例」＋第5例 0x45B300 | 教訓 / v313 §6-12 | **第6例: 0x452330** | `find_head(0x452330)=0x4522F0`、直前 `45232D: ret 4` の3バイトで CC ラン0 |
| 3 | 「[0xB333C4] を書く唯一の関数 FUN_00464440」 | v128_winmain_tail | **訂正: store は3件**（0x4067A6 / 0x4497A9 / 0x464553） | `[0xb333c4]` を dest にする `mov` の .text 完全走査（v316_12 [D]） |
| 4 | 「0x4651A5 へジャンプ（実書込なし）」 | s58 §2.1-1 | **訂正: 0x4651A5 は続行パス**。reject は 0x465186（通知→al=0→epilogue） | 0x4651A5 以降に `call 0x461680` → `call 0x45f2e0`（open）→ `call 0x464ab0` が並ぶ（v316_06） |
| 5 | FUN_459570 の arg3/arg4 未追跡 | v313 §7-5 | **解消: arg3=中央セグメントの sprintf 出力先、arg4=Playing Time 以降の時刻文字列** | 0x4596EE の `push 0xA2F884('%s%s')` と 0x459740-0x45975F の '.'→':' ループ |
| 6 | FUN_0452330 の意味 [部分確定] | v313 §7-6 | **解消: プレイヤー状態＋メニューID集合の述語、dry が短絡先行** | 0x452330..0x45239C 全文＋0x465159-0x465184 |
| 7 | FUN_45B300 の残フィールド [未確定] | v313 §7-7 | **初期化有無は全 [確定]**。+0x7D の挙動も [確定]（formID 解決テーブル適用） | ctor 全文抽出＋0x45BA00 / 0x45F7F0 |
| 8 | 0x416650 呼び手16件の compare カタログ [未確定] | v313 §7-10 | **16件すべて特定（11[確定]/4[部分確定]/1[未確定]）** | 各呼び手の push 列＋候補本体 dump |
| 9 | +0x88 の dry/real 挙動 [未確定] | v313 §7-1 | **[部分確定] へ前進**（1呼出し=+1・排他を確定、dry 発火経路は .text 未検出） | §4-1 の4種の負走査 |
| 10 | seek(0xD000) の origin / step の origin | v313 項目7 | **補強: 初回 seek は SEEK_SET(=0)、step は SEEK_CUR(=1)**（`[0xA853D0..0xA853D8]={0,1,2}`） | 0x45E7F6-0x45E81D / 0x45E894-0x45E8A2 |
| 11 | FUN_004E0F80 = 0x4E0F80..0x4E1571 | v313 項目8 | **微修正: `ret 4` は 0x4E156F（+3B で 0x4E1572）** | linear dump の最終命令（v316_02） |
| 12 | 「name1/name2 = TESFullName / 氏名風」 | v313 項目6 | **踏襲＋補強: 書式上の第1/第第2 %s と対応付けを確定**。語彙は [推定] | 0x45D920 の sprintf 引数列 |

**矛盾なし（踏襲確認）**: +0x88=offset38 対応、FUN_464320 の max+1、quicksave/autosave の offset38=0、FUN_465130 mode=0 恒、F1..F10 の読取順、0x4301A0=FILETIME フォールバック、0x5E1FD0→0x4677F0 の Level クランプ、0x45E6A0 の 2 jmp=new 成敗スキップ、D220/D390 の対関数、0x401000/0x981FB4 の正体。

---

## 6. 新規残件（次束用）

1. **[未確定] dry(bit9) の発火経路** — `[reg+0x18]` 演算530件・SaveGame+0x18 直接書込18件・RMW31件のいずれにも bit9 なし。`this` 引数経由 setter の8命令超間隔 RMW を否定できていない。→ dry→real 2パスの有無、ひいては counter の増加回数は動的確認待ち。
2. **[部分確定] FUN_004E0F80 の残実体** — 'BSX'(0x6FF9C0→0x4DE1C0) / 'REF'(0x6FFAC0) の正式名、0x7C5E70/0x7C76D0/0x7C7DC0 の辞書操作、`vfunc+0x188/0x198/0x380/0x44/0xF0` の対象クラス。
3. **[未確定] [0xB38720] の実内容** — BSS グローバル文字列（0x9F10Exx の静的初期化、0xA21560 トランポリン）。FUN_459570 arg3 の接頭辞として使用されるが実値不明。
4. **[未確定] SaveGame +0x44 / +0x48 の意味** — ctor で未初期化。使用箇所の特定を未実施。
5. **[部分確定] FUN_004610F0（帯外）の引数契約** — arg9 が thumb 出力先と判定したが、10引数の全契約と呼び手、返値（画像）の消費者が未特定。
6. **[部分確定] 0x416650 compare の残3種** — 0x5A63F0 / 0x6A25E0 の 3-way 返却確認、0x5D8FD0 のミドルエントリ本体（head 0x5D8F00 のどこから始まるか）。
7. **[推定/未確定] seek 0x4000/0x2000 の実レイアウト** — CON チャンク間ギャップという解釈。実 `.ess`/CON サンプル入手後に総長式 `nFileSize−0xD000−(0x4000+(n−1)×0x2000)` を検証。
8. **[未確定] FUN_00452330 のメニュー ID 名称** — {0, 3, 0x3F5, 0x40F} と 0x578FE0/0x579060 の戻り値の意味（0x40F は s58 の `push 0x40f; call 0x589b70` と同値）。
9. **[未確定] formID 解決テーブル [+0x74] の投入元** — FUN_45BA00/0x45F7F0 が読むのみで、書手が未特定。
10. **[部分確定] name2 の表示語彙** — 0x4DE8D0 の3系統（[Player+0x40]+0x1C / vfunc 経由の位置情報 / 空）のどれが使われるかと、UI 上のラベル。
11. **[未確定] FUN_0045BA00 / FUN_0045F7F0（head 0x45F180）の所属** — 帯内だが関数名・呼び手の全体像を未調査（+0x7d の文脈調査で到達）。

（新規残件 = **11 件**）

---

## 7. 実 `.ess` 不在の影響（動的未実施箇所の明記）

v313 §1.1 の8系統検索（ワークスペース全深さ / %TEMP% / ユーザープロファイル / D: depth8 / Documents\My Games / Steam 先 / BSA 展開先 / `rg -a TES4SAVEGAME`）で **実 `.ess` = 0 件 [確定]**（本件では再検索せず v313 の結論を踏襲）。

**動的照合が未実施のまま残る箇所**:

| 項目 | 未実施の照合 | 現状の判定への影響 |
|---|---|---|
| 9 | F1/F3/F6/F7 の旧版値と実ファイルの突合（契約 §1 ②⑤） | 静的 else は [確定]、総合は [部分確定] |
| 11 | 実 CON/`.ess` の総長・ギャップ位置の検証（契約 §1 ③） | レイアウト解釈は [推定]、総長式は [推定] |
| 4 | thumb ブロック（size/w/h/pixels）と実データの照合（契約 §1 ⑤） | arg10 判定は [確定]、データ形式は v313 の [推定] を維持 |
| 1 | dry モードの実発火と counter の実増加回数 | [未確定]（動態が必要） |
| 7 | +0x74 テーブルの実データ内容 | [未確定] |

**推測を確定扱いしない方針を維持した**（契約書 §解析制約）。本成果物は静的解析のみ。

---

## 8. 再現手順

1. 環境: Windows / python 3.11（capstone 5.0.7・pefile）/ EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`。
2. 依存ヘルパー（読取専用・変更禁止）: `%TEMP%\opencode\v307_common.py`（F / func_head / find_head / disasm / str_at / callers_of / resolve_imm32_refs）。`tmp/re/re_util.py` は不使用。
3. 実行（全16本）:
   ```powershell
   chcp 65001
   python $env:TEMP\opencode\v316_<n>.py *> $env:TEMP\opencode\v316_<n>.out
   ```
   `<n>` = 01..16（§2 の表）。
4. 判定はすべて出力の VA/命令列に基づく。.text 全体スキャン（v316_01/03/12/13/15/16）は **命令境界検証**（ウィンドウ内デコードの継続）を通過した命令のみ採用。`op_str` の空白ゆれ対策として `replace(" ","")` 正規化を v316_12 以降で使用（v316_01 の dry/+0x88 走査が0件だった不具合の是正）。
5. selftest（§9）:
   ```powershell
   $before=@{}; Get-ChildItem "$dir\v316_*.out" | %{ $before[$_.Name]=(Get-FileHash $_.FullName -Algorithm MD5).Hash }
   1..16 | %{ $n="{0:d2}" -f $_; python "$dir\v316_$n.py" *> "$dir\v316_$n.out" }
   # 前後の MD5 比較
   ```

---

## 9. selftest 結果

- **初回実行**: v316_01..v316_16 全16本 **EXIT=0**（v316_04/v316_05 のみ初回に `TypeError: %X format` → `find_bounds` の ret 検出がウィンドウ末尾のみを確認していた不具合を修正し再実行 EXIT=0）。
- **再実行（決定的検証）**: 全16本を再実行 → **全 EXIT=0**、実行前後の全 `.out` の **MD5 比較 = 差分0（hash-differences=0）** → 出力は決定的で再現可能。
- 内容別 selftest ポイント:
  - v316_03: RTTI 解析が `COL 0xAC3D80 → TD 0xB080A4 → .?AVPlayerCharacter@@` を返す（`.?AV` 文字列走査の `0xB080AC` と TD+8 が一致）。
  - v316_04: `callers_of(0x45D5F0)=4`、各 push 数=10 が `ret 0x28` と一致。
  - v316_05: `ret 0x10`（4引数）と arg1..arg4 の取り出し位置が整合。
  - v316_06: `find_head(0x452330)=0x4522F0`（off-by-pad 検出）、0x452330..0x45239C で `ret` が存在。
  - v316_07: `ret` @0x45B668 が v313 の範囲と一致。
  - v316_11: `[0xA853D0..0xA853D8]={0,1,2}`（SEEK 定数表）を実値で確認。
  - v316_16: `[reg+0x18]` 演算530件に `0x200`/`0xfffffdff` が0件（項目1の負証拠）。
- バンド遵守: 自帯外への到達はすべて浅層（§1.2）。v314 禁止帯ヒットは VA+命令のみ（§1.3）。

---

## 10. docs/RE_EXE_REPORT.md 等への反映要否

**本指示のみでは docs/ソース/CI/TODO/STATE は変更しない**。人間確認後の反映候補（正典 `docs/RE_EXE_REPORT.md`、矛盾時は §55 補遺→§50 優先、旧結論は削除せず §5 のように裁定理由を記録）:

1. **[0xB333C4] = PlayerCharacter 単一インスタンス（RTTI `.?AVPlayerCharacter@@`・vtable 0xA73A0C・生成2経路/破棄1経路）**
2. **FUN_00459570 の arg3/arg4 契約（中央セグメント sprintf／Playing Time 以降の '.'→':' 変換）と FUN_45D5F0 の callers 4件・thumb 読取は FUN_004610F0 のみ**
3. **FUN_00465130 の継続条件 = dry ‖ FUN_00452330() ‖ "autosave"、FUN_00452330 = プレイヤー状態＋メニューID述語（off-by-pad 第6例 0x452330）**
4. **SaveGame ctor のフィールド初期化表（+0x44/+0x48 未初期化、+0x54..60/+0x74/+0x78 サブオブジェクト、+0x7C=0x7D、+0x7D=formID 解決テーブル適用フラグ）**
5. **自動保存名書式 `'Save %i - %s - %s, Level %i, Playing Time %02i.%02i.%02i'` の引数対応とファイル名サニタイズ／重複時 ' #%d'**
6. **旧版ゲートの else 分岐表（F1=明示0 / F3=0 / F6=0 / F7=0x4301A0 の FILETIME、F4 len0=[0xB38728]、F8 ver<0x2E で +0x24 skip）**
7. **FUN_00416650 の compare カタログ16件（保存帯4件含む）**
8. **FUN_0045E6A0 の seek 実測（初回 SEEK_SET 0xD000、step SEEK_CUR 0x4000/0x2000、書出は got のみ、定数表 {0,1,2}）**
9. **FUN_004E0F80 の分岐カタログ（SetModel の15系統）と保存キュー投入ゲートの分岐条件**
10. **解析手順ノート: off-by-pad 第6例 0x452330、`op_str` 空白ゆれによる走査0件の罠（正規化必須）、func head 判定は ret 直後を含めて確認すること**
