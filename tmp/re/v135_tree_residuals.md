# V135: TESObjectTREE 残点（+0x0C の FormID 判定・slot#71 レシーバの動的証明）(2026-10-27)

対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
出典: `tmp/re/v95_tree_map_owner.md` 残点 1–5。本ファイルは V135 の正本（`docs/`・`TODO_re49.md`・`session-memory.json`・`plan.md` は未変更、ESM 再スキャンなし）。
ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時）。
参照資産: `tmp/re/GT_CHEATSHEET.md` / `tmp/re/cache/*` / `tmp/re/SUBAGENT_CONTRACT.md` / `tmp/re/v72_global_b333c4_map.md`。

**結論（3 行）**

1. **+0x0C = FormID（u32）[確定]**。採番器 `FUN_00448d40` の初期カウンタ = **0x800**、`DAT_00b33a98` = bss ポインタ→**FormID採番/ファイル管理オブジェクト（0xCE0 B・非多型・唯一の生成は FUN_00406360→FUN_00441970）**、書手 = TESForm ctor(`FUN_0046C460`) / **`FUN_0046C300`=SetFormID（102 呼出し）** / **レコードヘッダ適用 `FUN_00451210`（TESFile+0x248 → SetFormID、58 個の formType Load 関数が呼ぶ）**。採番器出力は常に ≥0x800（0x800 = 下限＝最初の ID＝フォールバック センチネル）、上位バイト 0xFF がランタイム領域（`FUN_00446B80` 判定 39 呼出し）。
2. **slot#71 レシーバの型同定を前進**: マップは `NiTPointerMap<TESBoundObject*, DISTANT_3D_DATA*>`（RTTI で確定）で、キーは ESM ロード中の REFR NAME 解決先のベースオブジェクト。さらに **`80 AA 4B 00` はファイル中 1 回のみ（VA 0xA450D0）⇒ `*(vptr+0x11C)==0x4BAA80` が成り立つ vptr は 0xA44FB4 に一意** → **FUN_004baa80 へ到達した時点でレシーバの vptr = 0xA44FB4 であることを構造的に証明**（残るのは実行時キー集合の列挙）。
3. **残点 1/4/5 は解明**: arg1 = RTTI なしの非多型 0x2C 構造体（構築経路 `FUN_00483D60→FUN_004BD430→FUN_004BCE00` 確定）、`FUN_004b3030` の 8 引数は「float 配列×2＋配列 C＋件数＋座標ハッシュ系4」（双方 `ret 0x20` で機械検証）、vtable 読手規則は「disp≤0x7C → `FF /2 disp8`、disp≥0x80 → `mov R,[R2+disp32]; call R`（到達 12,597 件で disp<0x80 は 0 件）」＋無制限追跡で slot#71 は **15 サイト/11 関数・gap 最大 14（>14 は 0 件）**、v95 の 16 サイト目の `FUN_005fc090` は**偽陽性と訂正**。

---

## 手法（probe 一覧・すべて `%TEMP%\opencode\v135_*.py`）

| group | script | 目的 |
|---|---|---|
| 点1 | `v135_p1a`〜`p1n` | `FUN_00448d40` 全 DisAsm、`DAT_00b33a98` imm32 参照分類、カウンタ全書手（disp32=0x8C0/0x8C4）網羅、`FUN_0046C300`(SetFormID) 102 呼出し・登録マップ 0xB0613C の insert/erase 絞込、レコード適用 `FUN_00451210` 58 呼出し、formEnumString テーブル 0xB05E04 の実データ読み |
| 点2/3 | `v135_p2a`〜`p2q` | `FUN_004bcef0` 単一呼出し元、embedded map の RTTI（`mov [reg+0xC], imm` の disp8/disp32 全走査＋TypeDescriptor 解決）、value ctor `FUN_004cc7c0`、挿入経路 `FUN_004d43f0/004d16d0`、キー抽出 `FUN_004d8440`、`0x4BAA80` 一意性 |
| 点5 | `v135_p5a/p5b` | .text の `mov r32,[r32+disp32]` 32,994 件のバイト走査＋無制限前方データフロー追跡、`FF /2` 形の disp 分布、disp 境界の実測 |
| 補助 | `v135_p6a`〜`p6d` | `ret N` 機械検証、`FUN_005fc090` アラインデコード、+0x8C4 書手、定数（0.5/0.97/100.0 double 等）読み |

（PowerShell 7 の既知制約は遵守: heredoc / `python -c` 正規表現 / foreach+-match 不使用。ログは UTF-8 reconfigure ヘッダ 2 行付き。）

---

## §1 残点3: `+0x0C` の u32 の意味論 → **[解明: FormID]**

### 判定
`TESForm+0x0C = FormID (u32)`。構造は `[modIndex:8][objectIndex:24]`、採番器の初期値は **0x800**、`0x800` は「採番下限＝最初に採番される ID＝ ctor フォールバック」の三重意味。`+0x0C == 0` = 未採番。

### 試行経路
1. `FUN_00448d40` を 0x448D40–0x448E1B まで全 DisAsm → thiscall（`mov ebx,ecx`）、引数なし（`ret`）。`[this+0x8C0]` を **post-increment 方式**で採番: `(v&0xFFFFFF)==0xFFFFFF` のとき `v=(v&0xFF000000)+0x800`（上位バイト保持・objIndex を 0x800 へ折り返し）、通常は `v+1`。map `0xB0613C` で重複排除（vtable slot1 ハッシュ + slot2 比較 + バケット鎖）、`FUN_00453450(*0xB33B00, id)` ゲート2本を通過した **pre-increment 値を返す**。→ 返り値域は初期値から見て常に ≥0x800。
2. カウンタの全書手を disp32 `0x8C0` バイト走査で列挙（allocator 構造体は下表のため他型の同オフセットは 0x5C4E40 系のみ＝無関係）:

| 書手 | VA | 値 | 文脈 |
|---|---|---|---|
| `FUN_00446d80`（ctor） | 0x446ED8 | **0x800** | ★初期値。`FUN_00441970` が malloc(0xCE0) 直後に呼ぶ唯一の ctor |
| `FUN_004492e0`（reset） | 0x449C9B | 0x800 | `+0x8C4` の解放と同時（呼出し元 = `FUN_004466f0`, `FUN_00464440`） |
| `FUN_0044f3d0`（"Loading Files..."） | 0x44F65B / decomp L60506 | **0xFF000800** | ファイル選択確定後にランタイム領域先頭へリセット |
| `FUN_00462b20` | 0x462BD7 | `max(virtual読値, 0xFF000800)` | 0x462BBB `cmp eax,0xff000800; jae; mov eax,0xff000800` クランプ |
| `FUN_00448d40` | 0x448D85 | +1 / 0xFF リージョン折返 | 採番本体 |
| `FUN_00446b40` | 0x446B75 | = id（巻き戻し） | 条件: `id>>24==0xFF && (id&0xFFFFFF)>0x7FF && id==counter-1` のみ |

3. `DAT_00b33a98` の正体: imm32 参照を全分類 → **store は 1 箇所のみ `0x441B8A mov [0xb33a98],eax`（`FUN_00441970` 内）**、read は `mov ecx,[0xb33a98]` 319 件（thiscall this 読み）＋ `mov eax,...` 37 件＋ `cmp ...,0` 5 件。`FUN_00441970` の呼出し元は **`FUN_00406360`（"Initializing Sky..." 初期化）1 件のみ**。生成列 = malloc(0xCE0) → `FUN_00446D80`(ctor) → store → `FUN_0044A2B0` → malloc(0x1C4) → `FUN_0045B300`(ctor) → `mov [0xb33b00],eax`@0x441BB9。
4. FormID 書手の同定: `FUN_0046C300` を decompile（L85867–85890）→ **唯一の正規セッタ**（旧 ID の map 削除 → `FUN_00446B40` 巻戻し（bFreeOld）→ 新 ID 登録 → `*(this+0xC)=id`）。E8 全走査で **102 呼出し**。レコード側を特定するため `FUN_00452570`(insert) / `FUN_004524c0`(erase) の呼出しを `mov ecx,0xb0613c` 近傍で絞込 → 登録は ctor/SetFormID/reset の **4 サイトのみ**、抹消は 3 サイトのみ（他 80 件は generic NiTMap 別インスタンス）。
5. ファイル由来 FormID の入口: `FUN_00451210` を decompile（L62074–62104）→ `form+4 = formType`、`form+8 = [TESFile+0x244](=record flags)`、**`FUN_0046c300([TESFile+0x248], 1)` = レコードヘッダ +0x0C（formid）を SetFormID**、末尾で `FUN_0046B6C0(form, file)`（form+0x10 所有ファイルリストへ push）。呼出し元 E8 全走査 = **58 関数**（`FUN_00416100` … `FUN_00566530` = 各 formType の Load）。
6. formType の ground truth: `formEnumString` テーブル `0xB05E04`（69 エントリ ×12B、tag 文字列ポインタ実読）→ **formType = テーブル index**（MISC=27=0x1B / CONT=23=0x17 / STAT=28=0x1C / DOOR=24=0x18 / **TREE=30=0x1E** を v95 の ctor バイト `mov [esi+4],0x1e` と5件交差検証）。よって `FUN_004511c0()==0x30` = **CELL**、`==0x31` = **REFR**。

### 証拠（asm / decomp 行）

- 採番器: `00448D50 mov ebp,[ebx+0x8c0]` … `00448D6E cmp ecx,0xffffff / 00448D76 and eax,0xff000000 / 00448D7B add eax,0x800 / 00448D82 add eax,1` … `00448E16 mov eax,ebp; ret`（decomp L55955–55994）。
- ctor 書き: `0046C58F call 0x448d40 / 0046C594 mov [esi+0xc],eax`（L86008–86009）。フォールバック L86013–86025（`uVar7<0x800` → erase/free → `FUN_00452570(0x800,form)` → `param_1[3]=0x800`）。
- SetFormID: L85874–85887。asm 呼出し例: `0x4160E0 mov ecx,[0xb33a98] / push 1 / call 0x448d40 / push eax / mov ecx,esi / call 0x46c300`（`push 1` は SetFormID の param_3 先行 push。`FUN_00448d40` 本体はスタック引数を読まない＝Ghidra 表示 `FUN_00448d40(1)` は先行 push の誤帰属）。
- レコード適用: `FUN_00451210` L62094–62102（`FUN_0046c300(*(param_1+0x248),1)`）。
- ファイルヘッダ mirror: `FUN_004512a0` L62119–62123 が `+0x23c/+0x240/+0x244/+0x248/+0x24c` を一括0化 = record header 5 フィールド（type/size/flags|label/formid|groupType/…）。
- 0xFF リージョン判定: `FUN_00446b80` `00446B80 cmp dword [esp+4],0xff000000; sbb eax,eax; add eax,1; ret 4`（= `id>=0xFF000000`）、**39 呼出し**。利用例 `FUN_00453530` L64279（temp FormID なら special フラグ付与）。
- 初期値 0x800: `00446ED8 mov dword [esi+0x8c0],0x800`（ctor `FUN_00446d80`）。生成は `00441B4F push 0xce0 / call 0x401f00 / call 0x446d80 / mov [0xb33a98],eax`（`FUN_00441970`、呼出し元 `FUN_00406360` のみ）。
- +0x8C4 = 現在ファイル: `FUN_0044f3d0` L60502 `*(param_1+0x8c4)=iVar8`（ファイル反復変数）/ asm `0044F5A7 mov [esi+0x8c4],edi`。ctor は L86010–86011 で `FUN_0046B6C0(form, +0x8C4)`（TLS ゲート内＝ロード中を除く実行時生成分のみ）、ファイルロード中は `FUN_00451210` L62102 が `FUN_0046B6C0(form, this=file)`。
- 固定 ID 書手: init L5002 `FUN_0046c300(0x14,1)` → **g_thePlayer（GT: `0xB333C4`）の FormID = 0x14**、L4993–4999 malloc(0x804)+`FUN_0066a740`。既定フォーム: `FUN_00518000` L209026–209042（`FUN_004471a0(0x212)` lookup 失敗時 `FUN_0046c300(0x212,1)`+"FootSoundDirt"）。clone: `0x683131 mov eax,[esi+0xc] / push 1 / push eax / mov ecx,ebx / call 0x46c300`。
- 補強（+0x0C が u32 ID であること）: `FUN_00447350` L54664 `FUN_0055e000(id,&out)` → dynamic_cast、`FUN_004473a0` 同型。ロード中は `FUN_0044f3d0` L60424 が TLS[+0x184]=1 を立て **ctor の採番を無効化**（＝ロード時はファイル ID のみ）。

### 判定
- 初期カウンタ値 = **0x800** [確定]、wrap で上位バイト保持・下位を 0x800 へ [確定]。
- `DAT_00b33a98` = **FormID 採番オブジェクト（0xCE0・非多型・+0x8C0 counter / +0x8C4 current TESFile / +0x8C8 ファイルリスト / +0x8D0 選択ファイル数(≤0xFE, "Too many selected files to compile!") / +0x8D4.. ファイル配列[0xFF] / +0xCD0.. flags）への bss ポインタ** [確定]。
- FormID 書手 = **ctor（採番 or 0/0x800）・`FUN_0046C300`(102)・`FUN_00451210`(ファイル formid、58 Load が呼ぶ)・固定値 init/既定フォーム・clone 転写** [確定]。採番器出力 ≥0x800、SetFormID 自体は <0x800 のファイル/固定 ID も受付（v95 の「通常 ≥0x800」を「採番器出力に限り常に ≥0x800」へ厳密化）。

---

## §2 残点2: slot#71 レシーバの vptr 0xA44FB4 の実行時証拠 → **[構造証明 完了・残ギャップ 1]**

### 判定
v95 の「シグネチャ整合＋`0x4baa80` 単一出現」を **一意性の数学的証明**へ強化し、キーの型・挿入経路も特定した。**到達 ⇔ vptr=0xA44FB4 が構造的に証明された**。残る未証明は「実行時のキー型集合（TREE 以外を含むか）」のみ。

### 試行経路
1. arg1 の embedded map の vptr 書手を特定するため、`.text` を `mov dword [reg+0xC], imm32`（disp8 形 `C7 /0 mod=01` + disp32 形、imm ∈ .rdata）で全走査し、imm の vtable-4 = COL → TypeDescriptor を解決。
   → **`FUN_004BCE00` が 2 回書く**: `004BCE41 mov [esi+0xc], 0xA45AD0`（base）→ `004BCE6A mov [esi+0xc], 0xA45AF8`（derived）。RTTI:
   - `0xA45AD0` = `.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PAVTESBoundObject@@PAUDISTANT_3D_DATA@@@@`
   - `0xA45AF8` = `.?AV?$NiTPointerMap@PAVTESBoundObject@@PAUDISTANT_3D_DATA@@@@`
   → **キー型 = `TESBoundObject*`（RTTI のテンプレート引数で確定）**、値型 = `DISTANT_3D_DATA*`。
2. value の実体: `FUN_004d43f0` が `malloc(0x34)` → `FUN_004cc7c0`（L166186–166192）。`FUN_004cc7c0` の DisAsm → **`NiTArray<float>` ×3（vptr `0xA45AF0` = `.?AV?$NiTArray@M@@` を +0/+0x10/+0x20 へ、各 {vptr, data@+4, u16 size@+8, u16@+0xA, u16@+0xC, u16@+0xE=1}）+ u32 count@+0x30 = 0x34 B**。
3. 挿入経路を特定: insert `0x452570` の 83 呼出しを function 単位で前方追跡（lea R,[x+0xC] → ecx フロー）→ 0x4B 系では検出不可（スタックローカル経由）。`FUN_004cc7c0` の呼出し元を辿り **`FUN_004D43F0` / `FUN_004D16D0`（両者とも単一呼出し元 `FUN_004F1990`）** を特定、その insert サイトの this を確認:
   - `004D4555 mov ecx,[esp+0x9c]` … `push eax(value) / push edx(key)` → `004D4580 call 0x452570`
   - `004D1894 mov ecx,[esp+0x44]` … `push esi(value) / push edi(key)` → `004D189A call 0x452570`
4. キー供給元: `FUN_004d43f0` decomp（L166168–166199）→ ゲート `FUN_004511c0()==0x30`(CELL)、`[file+0x23c]==DAT_00b05e20`（**DAT_00b05e20 = formEnum テーブル entry2 "GRUP" の fourcc フィールド** = ランタイム計算値）、`[file+0x248]∈{6,8,10}`（GRUP groupType）、`formType==0x31`(REFR) ループ、deleted フラグ 0x20 除外 → **`FUN_004D8440`** が REFR サブレコードを抽出: `NAME`(0x454D414E) → `FUN_0046B250(formid)` で **ベースフォーム解決**（L170021–170024 `*param_2 = FUN_0046b250(...)`）、`DATA` 0x18B 位置、`XLOD` 0xCB LOD 距離、`XSCL` スキップ。`*param_2 != 0` で return 1（**formType フィルタなし**）→ find(cell map, base) → 無ければ create+insert。
5. 一意性証明: ファイル全 7.9MB を `80 AA 4B 00` で走査 → **count=1, file offset 0x6442D0 → VA = 0x6442D0+0x400E00 = 0xA450D0 = 0xA44FB4+0x11C**。よって実行時に `*(vptr + 0x11C) == 0x4BAA80` が成立する vptr は `0xA450D0 - 0x11C = 0xA44FB4` のみ（アドレス演算として唯一、アライン非依存）。**`FUN_004BAa80` に到達した時点でレシーバ vptr = 0xA44FB4**。
6. vptr 0xA44FB4 がそのオブジェクトの先頭に立つこと: TESObjectTREE の main vtable は ctor `FUN_004B9DA0` の `004B9E02 mov [esi],0xa44fb4` のみが +0 に書く（adjustor 0xA44F94/0xA44F78 は +0x24/+0x3C のサブオブジェクトのみ、v95 a-1 で検証済み）。キーは `FUN_0046B250` の解決結果＝オブジェクト先頭ポインタ → +0 の vptr。

### 証拠（asm / RTTI / decomp 行）
- map vptr 書手: `004BCE41 / 004BCE6A`（fun `FUN_004bce00`、RTTI 名は上記）。standalone 版: `FUN_004bcbe0` `004BCBE5 mov [esi],0xa45ad0`、`FUN_004bcd70` `004BCD9A mov [esi],0xa45af8`。
- dispatch 本体: `004BCF98 mov edx,[ecx]（ecx=key） / 004BCF9A mov edx,[edx+0x11c] / push×8 / 004BCFBC call edx`（decomp L147908–147910、引数 = `local_10[5],[1],[9],[0xc], local_4, local_8, piVar1[7],[8]` = value+0x14/+4/+0x24/+0x30, gridhash, CONCAT22 coords, cell+0x1C, cell+0x20）。
- 挿入: `004D4580 call 0x452570`（key=[esp+0x5c]=`local_2c`=NAME 解決先）/ `004D189A`。value 初期化: `004D455C mov word [eax+0x2e],0x1e / 004D4562 [eax+0x1e],bx / 004D4566 [eax+0xe],bx / 004D456A [eax+0x30],edi`（decomp L166194–166197 = 0x1E/0x5A/0x5A/0 — **意味未解明=新残点**）。
- find: decomp L166184 `FUN_0055e000(local_2c,&local_68)`（hidden this = cell map）。
- 一意性: `bytes 80 AA 4B 00 file-wide count=1 pos=0x6442d0`。
- override が FormID を読む点（レシーバが TESForm 系である補強）: `FUN_004baa80` L146244 `FUN_007b2a00(*(param_1 + 0xc))`。

### 判定
- キー型 = `TESBoundObject*`（RTTI）/ 値 = `DISTANT_3D_DATA`(0x34) [確定]。
- **`FUN_004baa80` 到達 ⇒ レシーバ vptr = 0xA44FB4**（bytes 一意性 + アドレス演算）[確定・証明]。TREE 由来キー ⇒ vptr=0xA44FB4 も ctor 経路で [確定]。
- **[残]** 実行時のキー型集合（TYPE フィルタが無いため STAT 等の非 TREE キーが理論上あり得る）は、ESM 実データ検証が禁止のため未実測。v95 b-3 の「動的型がちょうど TESObjectTREE のときだけ」は**条件として**正しい（0x4BAa80 到達時のみ）。

---

## §3 残点1: `FUN_004bcef0` arg1 のクラス名 → **[解明: 非多型 0x2C 構造体（RTTI なし）]**

### 判定
arg1 は **RTTI を持たない非多型構造体**（size **0x2C**）で、コード上の名前は存在しない。セマンティクス名は「**DistantLOD セル（グリッド座標）レコード**」。構築経路と全フィールドを確定した。

### 試行経路
1. `FUN_004bcef0` の単一 E8 呼出し元 = `FUN_004BD1F0`（job、`[job+0xC]!=6` ゲート、`FUN_004bcef0(*(job+0x2C))`）→ arg1 = `job+0x2C` が指す先。
2. arg1 の ctor を `mov [reg+0xC], vptr` 全走査で特定 → **`FUN_004BCE00`**。これを decompile（L147815–147866）→ **+0 に vptr を書かない**（`*param_1=0; param_1[1]=0; param_1[2]=0` = 座標フィールド）。embedded map の inline 構築（base 0xA45AD0 → `param_1[4]=0x25`(hashSize=37) → `malloc(0x94)=37×4` buckets → derived 0xA45AF8）、`param_1[7]/[8]` = NiPointer（`InterlockedDecrement` で解放）、`byte param_1[10](+0x28)=0`。
3. 呼出し元 = `FUN_004BD430` 単一（E8）。その前方 DisAsm → **`004BD4E2 push 0x2c / call 0x401f00 / 004BD4FD call 0x4bce00` = malloc(0x2C)+ctor**。decomp（L148205–148220）→ `puVar5[2]=param_2; *puVar5=param_3; puVar5[1]=param_4; puVar5[9]=param_8`、`*(param_1+10)=0`。
4. `FUN_004BD430` の呼出し元 = `FUN_00483D60`（1 本 + 他2本 E8: 0x444396/0x4EB15A/0x4EBB1D は別 fun）。`FUN_00483D60` は DistantLOD グリッド走査（`DAT_00b34418`/`DAT_00B06A2C>>1`、`param_1[3]×param_1[4]` の 0x10B ステップ）。
5. 用途の確定: `FUN_004BD430` L148247 `_sprintf(buf, "DistantLOD\\%s_%i_%i.lod", (**(*cell[2]+0xd4))(), ...)` = **`.lod` ファイルパス生成**（cell+8 = 名前を持つ vptr オブジェクト、slot53 が名前返却）。L148230/148258 `FUN_004bca90/004bcac0((&DAT_00a45a58)[param_7])` で **DistantLODLoaderTask** を構築し NiPointer として +0x1C/+0x20 へ（vptr `0xA45A68` = `.?AVDistantLODLoaderTask@@`、関連 `0xA45B1C` = `.?AVLockFreeMap@IV?$NiPointer@VDistantLODLoaderTask@@@@@@`）。
6. 関連 RTTI 集計: `.?AV?$NiTArray@M@@` = 0xA45AF0（value のサブオブジェクト）、`.?AVDistantLODLoaderTask@@` = 0xA45A68、map = 0xA45AD0/0xA45AF8。

### フィールド表（確定、asm/decomp 交差）

| offset | 型 | 内容 | 根拠 |
|---|---|---|---|
| +0x00 | u32 (u16@+0,+2) | グリッド座標 X（FUN_004ef1d0/FUN_007b2110 の引数） | `movzx ecx,word [edi]` v95 / decomp L147885 |
| +0x04 | u32 (u16@+4,+6) | グリッド座標 Z | `movzx eax,word [edi+4]` / L147885–86 |
| +0x08 | ptr | 名前/系譜オブジェクト（`[+0]` = vptr、slot0xD4 = 名前） | L148246 `(**(*puVar5[2]+0xd4))()` |
| +0x0C | map | `NiTPointerMap<TESBoundObject*, DISTANT_3D_DATA*>`（vptr 0xA45AF8, +0x10 hashSize=0x25, +0x14 buckets） | ctor L147834–147840 / begin `004BCF4C lea ebp,[edi+0xc]; call 0x6a9030` |
| +0x1C | NiPointer | slot#71 arg7（refcount 解放は ctor L147856–147863） | ctor / dispatch L147909 |
| +0x20 | NiPointer | slot#71 arg8（末尾 `[ptr+4]>2` 判定 L147898–147900） | 同上 |
| +0x24 | u32 | `FUN_004BD430` の param_8（モード） | L148220 `puVar5[9]=param_8` |
| +0x28 | byte | 実行ガードフラグ | L147883 `(char)param_1[10] != 0` |

### 判定
クラス名 = **RTTI なし（非多型）** [確定]。構築経路 = `FUN_00483D60 → FUN_004BD430（malloc 0x2C + sprintf .lod + DistantLODLoaderTask 生成）→ FUN_004BCE00` [確定]。用途 = DistantLOD グリッドセル（座標 → base-object→LOD-data マップ＋ローダータスク×2）[確定]。

---

## §4 残点4: `FUN_004b3030` の 8 引数契約（[ebp+8..0x24]）→ **[解明]**

### 判定
this + **8 スタック引数**（機械検証: 最終 `ret 0x20` @0x4B31C7）。override `FUN_004baa80` も **`ret 0x20` @0x4BAC90** → 8 引数契約は両実装で一致（Ghidra の override 表示で欠落する未使用 arg8 を機械レベルで補正、v95「arg8 未使用」の裏付け）。

### 試行経路
1. `FUN_004b3030` decompile（L141329–141397）+ `FUN_004baa80` decompile（L146207–146266）+ 両関数の最終 ret を DisAsm。
2. 呼出し元 `FUN_004bcef0` の push 値（decomp L147909–147911 と asm `push ×8`）と突合。`FUN_004b3030` の直接 E8 呼出しは **0 件**（vtable 経由のみ）。

### 引数対応表 [確定]

| stack | Ghidra | 呼出し元 `FUN_004bcef0` の供給値 | 型/意味 | base 実装の使用 | override `FUN_004baa80` |
|---|---|---|---|---|---|
| [ebp+0x08] arg1 | param_2 | `local_10[5]` = **value+0x14 = NiTArray1.m_pData** | float 配列 A（12B stride × N） | ✓ `pfVar3=[arg1+8]`、3f を log 系変換 → rec[0..2] | ✓（`FUN_004ba2d0` へ） |
| [ebp+0x0C] arg2 | param_3 | `local_10[1]` = **value+0x04 = NiTArray0.m_pData** | float 配列 B（12B stride × N） | ✓ rec[3..5] = B[3i..3i+2] | ✓ |
| [ebp+0x10] arg3 | param_4 | `local_10[9]` = **value+0x24 = NiTArray2.m_pData** | 配列 C（1 エントリ/反復） | ✓ rec[6] = f(C[i]) / 100.0（`_DAT_00a309f0` = double 100.0） | ✓ |
| [ebp+0x14] arg4 | param_5 | `local_10[0xc]` = **value+0x30 = count** | 件数 N（≠0 ガード＋ループ回数） | ✓ `while (i < N)` | ✓ |
| [ebp+0x18] arg5 | param_6 | `local_4` = `FUN_007b2110(x,z)` | グリッドハッシュ | **未使用** | ✓ |
| [ebp+0x1C] arg6 | param_7 | `local_8` = `FUN_004ef1d0(x,z)`（CONCAT22） | 座標合成値 | ✓ `FUN_004b2730(this, N, arg6)` = **集計キー**（マップ `0xB08300` の `{form→count}` リストへ加算、insert @0x4B279A） | ✓ |
| [ebp+0x20] arg7 | param_8 | `piVar1[7]` = **cell+0x1C** NiPointer | DistantLODLoaderTask 等 | **未使用** | ✓ |
| [ebp+0x24] arg8 | param_9 | `piVar1[8]` = **cell+0x20** NiPointer | DistantLODLoaderTask 等 | ✓ ガード `param_9 != 0` のみ | ✓（Ghidra 表示では arg7 に見えるが実体は8番目） |

- ガード（base）: arg8≠0, arg1≠0, arg2≠0, arg3≠0, arg4≠0, **`FUN_0046d540(this, buf[268])` の結果が非空**（this のモデルパス/名前） [L141355–141356]。
- 本体: N 回、`malloc(0x20)` レコードへ `{log系(A[3i..]), B[3i..], f(C[i])}` を格納し `FUN_004385b0(path, rec, ctx=FUN_004b2bd0())` へ追加、末尾 `FUN_004b2730(this, N, arg6)` [L141360–141393]。
- override の追加ガード: `DAT_00b125e8 != 0` と **`FUN_007b2a00(*(this+0x0C))`（this の FormID を DistantLOD マップで解決）** [L146239–146245] → レシーバが TESForm 系である傍証。

### 判定
8 引数契約 = **(float配列A, float配列B, 配列C, 件数, gridhash, 座標合成値, NiPointer, NiPointer)** [確定]。基底実装は A/B/C から DistantLOD レコードをログ距離系で整形登録し、座標合成値をキーに form 単位の件数集計を行う [確定]。arg5/arg7 は base 未使用・override 使用（`ret 0x20` で整合）[確定]。

---

## §5 残点5: `mov R,[...]` 形 vtable 読手の判定規則の一般化 → **[解明＋v95 訂正 1 件]**

### 判定（規則）
1. **形の分岐は disp で決まる（実測）**: slot disp ≤ 0x7C → **`FF /2 disp8`（`call [reg+disp8]`、非SIB 6,562 件）**、slot disp ≥ 0x80 → **`mov R,[R2+disp32]; call R`**（到達 **12,597 件、disp 最小 0x80・最大 0x998、<0x80 は 0 件**）。`call [reg+disp32]`（FF /2 mod=10, 非SIB 67 件）は**負 disp のフレーム関数ポインタと非整合デコードのみで、vtable slot には使われない**（v95 の `FF 90 1C010000 = 0 件` を一般化）。
2. **検出手順（無制限・再現可能）**: (i) `.text` をバイト走査し opcode `8B` + modrm(mod=10) の disp32 を候補化 → slot 範囲 [0,0x1000] ∩ 4 倍数 = **30,717 件**（生候補 32,994）。(ii) capstone で `mov r32,[r32+disp32]` であることを検証。(iii) **前方データフローを無制限に追跡**（`call R`/`jmp R` まで。target レジスタの再定義・`ret`/`int3`・他レジスタへの call で打ち切り）→ 命令数 gap を記録。ウィンドウ長に依存しないため「14 命令超の間引」を原理的に検出できる。
3. **vptr 読手の同定**: base レジスタの到達定義が `mov R2,[obj+0]`（vptr ロード）であり、disp が 4 倍数の slot 範囲であること。偽陽性（+0x11C をフィールドとして読むコード）は前方追跡で `redefined` / `other-call` へ分類され機械的に除外される。

### slot#71（disp=0x11C）への適用 [残点5 の回答]

- 生候補 99 → 無制限追跡 → **dispatch = 15 サイト / 11 関数**:
  `FUN_004aa1f0`(0x4AA620, 0x4AA630) / `FUN_004abe50`(0x4AC116, 0x4AC124) / `FUN_004bcef0`(0x4BCFBC) / `FUN_00546930`(0x54694F) / `FUN_005f5d10`(0x5F5E61, 0x5F5F6F) / `FUN_00603500`(0x603B1B, 0x603B33) / `FUN_00613440`(0x613458) / `FUN_006134c0`(0x6134D9) / `FUN_0061e8a0`(0x61E8BA) / `FUN_00656270`(0x6562C8) / `FUN_00731f80`(0x731FA0)。
- **gap（mov→call の命令数）最大 = 14、>14 は 0 件** → v95 の 14 命令ウィンドウは slot#71 に対して**充足していた**（無制限化による増分なし）→ 残点5「間引ケースの残存可能性」= **[解明: 該当なし]**。
- **v95 訂正**: v95 の16サイト目 `FUN_005fc090 (decomp L364269)` は**偽陽性**。アライン DisAsm では当関数に `mov r,[r+0x11c]` は存在せず `005FC196 add ebx,0x11c` のみ。decomp L364265–364268 は `iVar3 = *(param_2[0x16]); … (**(code**)(iVar3+0x11c))(uVar7)` = **構造体の関数ポインタフィールド（+0x118/+0x11C ペア）の indirection**（vtable slot ではない）。→ 正確なカウントは **16/12 → 15/11**。
- 既知偽陽性の再分類（本規則による自動除外）: `FUN_0051d0d0`（ネスト vptr 読み → redefined）、`FUN_00479f80`/`FUN_008dcdf0` 等（フィールド参照 → other-call/redefined）— v95 の手動除外と一致。

### 証拠（数値）
`mov r32,[r32+disp32]` 総数 32,994 / slot 範囲 30,717 / disp=0x11C 生候補 99 / dispatch 15・gap max14・>14 は 0 / FF·2 disp8(非SIB) 6,562 / FF·2 disp32(非SIB) 67（負 disp・誤検出のみ）/ mov+call 到達 12,597（min disp 0x80, max 0x998, <0x80 = 0）。

### 判定
一般化規則 = 上記 1–3 [確定]。slot#71 は **15 サイト/11 関数・gap≤14** [確定]。v95 の `FUN_005fc090` サイトは訂正 [確定]。

---

## 証拠一覧（本レポートが引用した一次証拠）

| 区分 | 内容 |
|---|---|
| asm VA | 0x448D40–0x448E1B（採番）, 0x446ED8/0x449C9B/0x44F65B/0x462BD7（counter 書手）, 0x441B8A（DAT_00b33a98 store）, 0x46C594（ctor 書き）, 0x46C324/0x46C34E/0x46C5D5/0x46C5F8/0x46C612/0x449BBF（map 0xB0613C erase/insert）, 0x44F5A7（+0x8C4 = current file）, 0x46C300 サイト 102, 0x451210 サイト 58, 0x4BCE41/0x4BCE6A（map vptr）, 0x4BCF9A–0x4BCFBC（dispatch）, 0x4D4580/0x4D189A（insert）, 0x4BD4E2/0x4BD4FD（arg1 malloc+ctor）, 0x4B31C7/0x4BAC90（ret 0x20）, 0x5FC196（add ebx,0x11c） |
| decompiled 行 | L55955–55994, L85867–85890, L85946–86031, L62074–62104, L60404–60512, L14520–14556, L166115–166263, L169971–170080, L147815–147924, L148024–148039, L148157–148281, L141329–141397, L146207–146266, L140758–140801, L160407–160438, L4993–5007, L209000–209043 |
| RTTI / テーブル | 0xA45AD0 / 0xA45AF8（map）、0xA45AF0=`NiTArray<float>`、0xA45A68=`DistantLODLoaderTask`、0xA45B1C=`LockFreeMap<unsigned,NiPointer<DistantLODLoaderTask>>`、formEnumString `0xB05E04`（69 件実読、formType=index、CELL=0x30/REFR=0x31/TREE=0x1E）、`DAT_00b05e20`=GRUP fourcc フィールド |
| ユニーク性 | `80 AA 4B 00` file-wide count=1 @file 0x6442D0 = VA 0xA450D0 |
| スキャン統計 | 0x8C0/0x8C4 disp32 書手網羅、insert/erase 83/76 件（0xB0613C 絞込 4/3）、`mov r32,[r32+disp32]` 32,994、FF/2 6,562・67、到達 12,597 |
| GT 突合 | `probe_harness.py selftest` 4/4 PASS、`0xB333C4`=g_thePlayer（GT）、`0x446CB0`=list push（GT）、.rdata const 0x400E00（GT） |

---

## 判定（3 行）

1. 点3（+0x0C）= **FormID [解明]**: 採番初期値 0x800・`DAT_00b33a98`=採番オブジェクトの bss ポインタ・書手は ctor/SetFormID(102)/レコード適用 `FUN_00451210`(58)・≥0x800 は採番器出力に限り恒真、0x800 は下限＝初回 ID＝フォールバック、0xFF 上位バイトがランタイム領域。
2. 点2（slot#71 レシーバ）= **構造証明 完了 [解明＋残 1]**: キーは `TESBoundObject*`（RTTI）で ESM ロード中の REFR NAME 由来、`0x4BAA80` のファイル中一意（VA0xA450D0）より到達⇔vptr 0xA44FB4。残は実行時キー集合の列挙のみ。
3. 点1/4/5 = **arg1=非多型 0x2C 構造体（経路 FUN_00483D60→004BD430→004BCE00）[解明] / 8 引数契約 = float配列×2+配列C+件数+座標系4（双方 ret 0x20）[解明] / vtable 読手規則 = disp8≤0x7C or mov+call≥0x80、slot#71=15 サイト gap≤14、v95 の FUN_005fc090 は偽陽性と訂正 [解明]**。

## 残存リスク

- **動的実行なし**: 全証明は静的（バイナリ + decompiled）。実行時プロファイル（キー集合・dispatch 回数）は未取得。
- **ESM 実データ未検証**: スキャン禁止のため「REFR NAME キーの型分布（TREE/STAT/…）」は未測定。値の u16(+0x0E/+0x1E/+0x2E)=0x5A/0x5A/0x1E も意味未解明。
- Ghidra decompile の thiscall this 省略・先行 push 誤帰属（`FUN_00448d40(1)`、`FUN_0046c300` 呼出しの2表示など）が再出現しうる → **asm を正とする**。
- `FUN_00462B20` の virtual 読値（`[edi+4]` 呼出し）の正体（HEDR nextObjectID 説）は未検証。

## 新規残点（V143+ 粒度）

1. **V143**: `DISTANT_3D_DATA` の u16 フィールド 3 本（+0x0E/+0x1E/+0x2E、ctor=1 → `FUN_004d43f0` が 0x5A/0x5A/0x1E を書く）の意味論と、3×`NiTArray<float>` への float 値の供給元（`FUN_004d16d0` の配列 resize `FUN_004ca040` 以降の填充）。
2. **V144**: cell マップ（`TESBoundObject*`→`DISTANT_3D_DATA*`）の**実行時キー型集合**（非 TREE を含むか）。ESM の REFR NAME 分布（スキャン禁止のため別手段: ランタイム dump または許可時の一覧）。
3. **V145**: `FUN_005FC090` の `param_2[0x16]` 先オブジェクト（+0x118/+0x11C が関数ポインタ ペア）の同定（v95 残点5 の訂正に伴う新対象）。
4. **V146**: `FUN_00462B20` の virtual 読値の正体と、allocator counter の modIndex 切替が「ファイル単位」でどこで行われるか（`0xFF000800` リセット以外の切替経路）。
5. **V147**: `FUN_004B2730` の集計マップ `0xB08300`（coordhash→`{form,count}` リスト）の読手（消費側）と用途名。
6. **V148**: グループ `DAT_00b05e20`(GRUP fourcc) ゲートの `{6,8,10}` groupType の正式名称対応（persistent/temporary 等の GECK 名）。

## 検証日・ハーネス

検証日: **2026-10-27**。`python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**。証跡スクリプト: `%TEMP%\opencode\v135_p1a.py` 〜 `v135_p6d.py`（新規のみ）。既存 `tmp/re/*.md`・`docs/`・`TODO_re49.md`・`session-memory.json` は未変更、ESM 再スキャンなし。

## Android 移行への示唆（1 行）

移植時は **`TESForm::SetFormID` の map 0xB0613C 整合（ctor/SetFormID/レコード適用の3系統のみ）** と、**slot#71 の機械契約（this + 8 stack args / callee-clean `ret 0x20`、TREE のみ 0x4BAA80 へ到達）**、および **DistantLOD セル構造体 0x2C（embedded `NiTPointerMap<TESBoundObject*, DISTANT_3D_DATA*>`）** の ABI を固定すること。
