# v312_phase2_gate_verification.md — B-1 Phase2: v311 §6 リスト 12 件の EXE 静的検証（L1・日本語報告）

- 作成: 2026-11-07（ai-todo #77 / B-1 Phase2）
- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（.text 0x401000 → file 0x400）
- 使用帯: 本タスク帯 0x45D000-0x466000、および 0x401000-0x410000 / 0x452000-0x455000（formatter 窓 0x453A90..0x458E60 は probe で read-only 読取）
- 検証対象: `tmp/re/v311_label_dispatch_crosscheck.md` §6 Phase2 リスト 12 件
- 判定: 5 段階 [確定] / [部分確定] / [推定] / [未確定] / [確定負]。旧結論（v311）は削除せず、矛盾時は両論併記＋裁定理由を「裁定更新案」として併記。
- 成果物: 本ファイルのみ新規。既存 tmp/re・台帳・STATE・TODO・docs・GT・ソース・.github/** は読み取りのみ。ESM 未接触・git 未実行・`tmp/re/re_util.py` 変更なし。

---

## 1. §6 の 12 件ごとの結果

### 件1（§6-1）保存帯 0x465860 の 5 caller site 引数の再検証 — **[確定]**

**callers(0x465860) = 5 件（raw E8 走査）**: `00466B31 00466D5F 005025F7 005AE1E6 005B5DA8`（probe01）。貸与の 5 site と完全一致。

**fn head（背中合わせ確認）**
- 0x466D50: 直前 `...c414010000 c20400` = `ret 4` @0x466D4D → head 0x466D50。先頭命令 `mov eax, dword ptr [ecx + 0x1c0]`（probe01b）。
- 0x5AE190: 直前 `50 5e 83 c4 08 c2 08 00` = `ret 8` @0x5AE18D → head 0x5AE190。

**引数 push 順列（stdcal 右→左、最後の push = arg1）**
| site | push 列（先→後） | arg1 / arg2 / arg3 | 貸与との照合 |
|---|---|---|---|
| 0x466B31 | `push 0`@0x466B26 → `push 0xa3b144`('quicksave')@0x466B28 → `push 0` | 0 / 'quicksave' / 0 | 一致 |
| 0x466D5F | `push 0`@0x466D5A → `push eax`@0x466D5C（eax=`[ecx+0x1c0]`） → `push 0`@0x466D5D | **0 / [ecx+0x1C0] / 0** | **相違**（下記裁定） |
| 0x5025F7 | `push ecx`@0x5025E9 → `push edx`@0x5025F4 → `push 0`@0x5025F5 | 0 / &buf / - | 一致（arg1=0） |
| 0x5AE1E6 | `push 0`@0x5AE1E1 → `push 0`@0x5AE1E3 → `push eax`@0x5AE1E5 | eax（`[esi+0x4C]`） / 0 / 0 | 一致 |
| 0x5B5DA8 | `push 0`@0x5B5DA3 → `push 0`@0x5B5DA5 → `push eax`@0x5B5DA7（`mov ecx,[0xb33b00]; mov eax,[ecx+0x6c]`@0x5B5D90-96 経由） | [[0xB33B00]+0x6C] / 0 / 0 | 一致 |

**両論併記＋裁定（0x466D5F）**: (a) 貸与 v308 追補「arg1=[ecx+0x1C0]」、(b) 本実測「arg1=0・arg2=[ecx+0x1C0]」。→ **裁定: (b) を採用**。根拠は命令順列（`push eax` が中央＝arg2）で一次証拠、(a) の値自体は head 0x466D50 の `mov eax,[ecx+0x1c0]` 由来と考えられ index の書き違いと判断。v311 §6-1 の parenthetical のみ訂正対象。

**FUN_0045F2E0 のパス整形**: `strrchr(path, '\\')` = `call 0x983fd0`@0x45F5F7、.ess 判定 = len>4 → `_strnicmp(tail, 0xA3A0D4='.ess', 4)` = `call 0x9864d9`@0x45F646（一致時 truncate）。**0x45F640 は call target ではなく関数内 push 命令**（callers(0x45F640)=0）＝貸与どおり。callers(0x45F2E0)=8 件。

**裁定更新案**: v311 §6-1 の「0x466D5F（arg1=[ecx+0x1C0]）」→「**arg2=[ecx+0x1C0]・arg1=0**」に更新。他 4 site・fn head・.ess 判定は貸与どおりで追加訂正なし。

---

### 件2（§6-2）ゲート形式争い 7 ラベルの命令単位裁定 — **[確定]**

**7 ラベルすべてが「二段ゲート」（第 1 段 = formType の `cmp al`、第 2 段 = changeFlags の `test dl/edx`）**（probe02a/02b/07/18）:

| ラベル | 第 1 段（型ゲート） | 第 2 段（flag ゲート） | label |
|---|---|---|---|
| Quest Flags | `mov al,[esp+0x174]`@0x453F2B → `cmp al,0x23`@0x453F32 `je 0x457ff1` / `cmp al,0x24`@0x453F3A `je 0x457ff1` / `cmp al,0x3B`@0x453F42 `jne 0x454333` | `test dl,4`@0x453F4A | 0x453FC7 |
| Cell Flags | `cmp al,0x30`@0x454683 `jne 0x454e81` | `test dl,8`@0x45468B `je 0x4547b7` | 0x4546FF |
| Cell Created | 上流 0x454683（type 0x30 かつ bit3 未設置で fall） | `test dl,2`@0x4547B7 + `test dl,4`@0x4547C0 | 0x454838 |
| Faction Flags | `cmp al,6`@0x454E81 `jne 0x4550a3` | `test dl,4`@0x454E89 `je 0x454fa3` | 0x454EE4 |
| Faction Reactions | 上流 0x454E81（type 6） | `test dl,8`@0x454FA3 | 0x455001 |
| Book Skill | `cmp al,0x15`@0x4550A3 `jne 0x4551a3` | `test dl,4`@0x4550AB `je 0x458e1a` | 0x455101 |
| Created | `cmp al,0x31`@0x4551A3 `je 0x4551b3` / `cmp al,0x32`@0x4551A7 / `cmp al,0x33`@0x4551AB `jne 0x458e1a` | `test dl,2`@0x4551B3 `je 0x4552c3` | 0x455208 |

**追加**: Lock の `test dl,0x40`@0x4565C3 は **changeFlags ゲート**（probe15 全 67 ゲート一覧に登録）。v311 row45 の probe 行「typeID 0x40@0x4565C3」は flag 誤り。

**両論併記＋裁定（§5-3）**: (a) D 表（v304 追補2-D）の bit 読み、(b) probe 行の type 読み。→ **裁定: 両者とも隣接する別段を指しており「形式不一致」は存在しない**。D 表の flag 読みが正（第 2 段）、probe 行の typeID 呼称は第 1 段（型ゲート）への読み替え。**二段ゲートで統一 [確定]**。

**裁定更新案**: v311 §5-3 の「形式が未統一のラベルは [確定] を認めず [部分確定]」という**理由のうち「形式争い」分は消滅**。ただし該当 7 行の総合判定は §5-4 の同値多義（保存側 S セットの値多義）が残るため **[部分確定] 維持**（row8/15/16/22/23/24/25。更新理由欄から「ゲート形式争い」を除去し「保存側値の多義」のみに絞る）。row45 Lock は probe 行を flag 読みへ訂正。

---

### 件3（§6-3）同値多義 bit の裁定（13 組・§5-4） — **[部分確定]**

**型チェーン 13 件とセクション区画（probe07）**: `0x453F32`(0x23/24→0x457ff1) / `0x453F42`(0x3B) / `0x454333`(0x3A) / `0x454463`(0x3D) / `0x454683`(0x30) / `0x454E81`(6) / `0x4550A3`(0x15) / `0x4551A3`(0x31/32/33) / `0x45626A`(0x32→0x456ff1) / `0x456272`(0x33→0x456ff1)。
区画: `0x457ff1`+=0x23/24、`0x453F4A`+=0x3B、`0x45433B`+=0x3A、`0x45446B`+=0x3D、`0x45468B`+=0x30、`0x454E89`+=6、`0x4550AB`+=0x15、`0x4551B3..0x45626A`={31,32,33}、`0x45627A..0x456ff1`=0x31、`0x456ff1..0x457ff1`={32,33}。

**結果: 13 組中 12 組が formType 区画で分離 [確定]**
- 例: 0x4 組 = Quest Flags(0x3B) / Faction Flags(6) / Book Skill(0x15) / Cell Created(0x30) / Base Health(0x23/24) → 全別区画。
- 0x800 組 = Had Havok Move(区画 {31,32,33}) ↔ Fame(区画 0x23) → 分離。
- **0x8000000 組 4 ラベル全分離（第 4 ラベル同定）**: Quest Script `test edx,0x8000000`@0x4541CB（区画 0x3B）/ Waiting Flag @0x454583（区画 0x3D）/ Detach Time @0x454A13（区画 0x30）/ Inventory @0x455EF3（区画 {31,32,33}）。→ **v311 の「第 4 ラベル未同定」残点は解消**（第 4 = Quest Script）。
- 0x400000 組 = Furniture Markers @0x456393(0x31) ↔ Game Modifiers @0x457103({32,33}) → 分離。0x400 組 = Map Marker @0x45627A(0x31) ↔ Combat Style @0x4589C1(0x23)/@0x458D11(0x24) → 分離。

**例外（未分離）: 0x10000 組の %s ↔ Non-saved**
- Empty Flag `test edx,0x10000`@0x456B33（区画 0x31）は分離。
- 一方 **%s %08X @0x457718 と Non-saved Package @0x457853 は同じ区画 {32,33}・同じ flag 0x10000**（probe15 ゲート一覧 L51-52）→ formType では分離不能。%s 側の別条件は v311 が v304 追補A から引く「package 名解決 0x5E4080」のみで、本タスクの命令検証では未確証 → **sub-dispatch [部分確定]**。

**両論併記＋裁定（§5-4）**: (a) formType ごとに同一ビットが別意味（v304 D 表の意図的同値割当）、(b) 700 命令ウィンドウの帰属 bleed。→ **裁定: (a) を採用する方向で強い命令証拠**（各ゲートが型ゲートの下流に位置し、formType 区画がビット意味を決定）。ただし (b) 完全否定には保存側（S セット）の formType 条件が artifact 側で未突合であり、**[確定] には至らず [部分確定]**。

**裁定更新案**: v311 §5-4 を「(a) 採用を命令で裏付け（12/13 分離）、0x10000 組の %s↔Non-saved のみ保留」と更新。row71/72/80 等の多義を理由に落としている行の判定は **[部分確定] 維持**（保存側値の逆方向が未着地のため）。

---

### 件4（§6-4）452CF0 の flags 実体確認 → bit3 二義解消 — **[確定]**

**452CF0 = upsert**: 452C90 が find-or-insert（via `0x55E000` / `0x452570`）、value[0]=arg2、452CF0 が value[+4]=arg3（probe03 系）。

**call site 0x46681B-0x466829（probe09 A）**:
```
0046681B  mov eax, dword ptr [esp + 0x21]   ; 12B レコード +5 = changeFlags
0046681F  mov ecx, dword ptr [esp + 0x1c]   ; 12B レコード +0 = formID
00466823  push ebx                          ; arg3
00466824  push eax                          ; arg2 = flags
00466825  push ecx                          ; arg1 = formID
00466826  mov ecx, dword ptr [ebp + 4]
00466829  call 0x452cf0
```

**12B レコード layout（命令実測）**: `<formID u32 @+0><formType u8 @+4><changeFlags u32 @+5><u8 @+9><tailSize u16 @+10>`（src = [esp+0x1C..0x27]）。根拠: flags=[esp+0x21](+5)、+9=`mov cl,[esp+0x25]`@0x46679C、+10=`mov ax,[esp+0x26]`@0x466787（tailSize）→ `movzx edx,ax / add edx,4 / push edx / call 0x453500`@0x466795-0x4667AF（tail payload alloc）。

**同物性の三段連鎖 [確定]**
1. 保存側: 12B +5 flags → 452CF0 arg2 → ledger value[0]。
2. pass2: ebx = [esp+0x24] ← `0x452600` lookup value[0]（@0x461A87 書込）→ **同一 changeFlags dword**。
3. formatter: caller1（0x45B029）が `byte[ebx+4]`=formType・`[ebx+5]`=flags を渡し、formatter 入口 `call 0x4535a0`@0x453AD0 の返り値が [esp+0x170]、以降の全 `test dl/edx` ゲートがそれを読む（例 `mov edx,[esp+0x170]`@0x45575E/0x4569FE）。

**bit3 二義の裁定（§5-5）**: (a) 同一 changeFlags dword 上の文脈依存多義、(b) pass2 の ebx はレコード制御フィールドで別物。→ **裁定: (a) を採用 [確定]、(b) は [確定負]**。根拠: 両者とも ledger value[0]（452600/452C90 経由）を参照し、formatter 側は 12B +5 と同物である以上、pass2 の `test ebx,0x8` 系も同一 dword を見ている。v297「pass2 bit3=tombstone」と v304 D 表「bit3=Cell Flags/Base Attributes」は**同一ビットの文脈依存多義**（pass2 = 保存処理文脈、formatter = 表示文脈）。

**v289 refcount 記述との関係（両論併記）**: arg3=ebx は `call 0x453500`@0x4667AF（`push edx`=tailSize+4）の **alloc 結果**（`mov ebx,eax`@0x4667BA、size==0 経路 0x466830 では 452C90 のみで value[4]=0）。(a) v289「refcount」= refcount、(b) = tail buffer ptr。→ **裁定: 命令証拠優先で (b) 方向**、ただし 453500 の返り値の名前未同定のため refcount 説を完全否定せず **残点 R3** へ。

**裁定更新案**: v311 §5-5「未決着・Phase1 では両論併記」→「**[確定]（(a) 採用・(b) [確定負]）**」に更新。§6-4 の「S10 通過格納の確定」は12B +5 連鎖で達成。row15 Cell Flags / row72 Base Attributes の bit3 は同一ビット・別 formType として共存可能（件3 の formType 分離と整合）。

---

### 件5（§6-5）[未対応] 2 件の裏取り（Cell Changed / Dropped Item Flag） — **[部分確定] へ格上げ**

**Cell Changed(%i) @0x455324**（probe07/09/11）
- **ゲート取得**: `test edx, edx`@0x4552C3 → `jns 0x4553e3`@0x4552C5。edx は `mov edx,[esp+0x170]`（changeFlags）由来 → **bit31(0x80000000) ゲート**（jns = bit31 未設置でスキップ）。v311 row26「ゲート未取得」→ **取得**。
- quiet 名: `'CHANGEFLAG_REFR_CELL_CHANGED\r\n'`@0xA39B20（xref は formatter 内 0x4552F0 のみ、S12 band 0xA3914C..0xA39E6C 内 ✓）。
- ロード側分岐: **帯内 bit31 テスト 2 件**（probe09 B: `004647A6 test dword[eax],0x80000000`、`0046630D test dword[eax],0x80000000` = LoadGame 内 `call 0x452d10` 直後＝ledger value[0] 対象）。
- formatter 内の `test/cmp ...,0x80000000`（imm32 形）= **0 件**（sign 形のみ・jns 3 件）。
- 保存側: v311 の S5（bit31 frame）記載と整合。

**Dropped Item Flag(%i) @0x456A74**（probe10/14/15）
- ゲート: `test edx,0x20000`@0x456A13 `je 0x456b33` ✓（v311 既載）。
- quiet 名: `'CHANGEFLAG_OBJECT_DROPPED_NON_QUEST_ITEM\r\n'`@0xA39730（xref は 0x456A40 のみ、band 内 ✓）。
- **保存側値分岐の実在（v311 訂正点）**: pass2 @0x461E62 `cmp ebp, 0x31` → `jne 0x461e8c` → @0x461E67 `test ebx,0x20000` → `push 1 / push edi / call 0x452df0`@0x461E3F-42（force remove）＋ `mov eax,[edx+0x10] / push 1 / call eax`@0x461E4B-54。→ **v311「S セットに 0x20000 不在」は pass2 の実分岐を欠いていた＝訂正**。ebp=formType で formatter の区画（0x456A13 ∈ 区画 0x31）と一致（件11 参照）。

**限定理由（[確定] にしない理由）**: band 内の `CHANGEFLAG_*` 文字列は **3 件のみ**（s60 照合: 0xA39730 / 0xA39A98 HAD_HAVOK / 0xA39B20）で、67 `CHANGE_*`（save レコード名）との双方向は未成立。quiet 専用名か save レコード名かの正体は **残点 R4**。

**裁定更新案**: v311 row26 `**[未対応]**` → `**[部分確定]**`（ゲート bit31 取得・quiet 名・ロード側分岐）、row49 `**[未対応]**` → `**[部分確定]**`（quiet 名・保存側 pass2 分岐）。§4.2 の [未対応] リストは 2 件とも格上げされるため**残 [未対応] 0 件**（リスト空振りの更新を提案）。

---

### 件6（§6-6）保存実行 FUN_00465130 鎖から formatter 呼出ゼロの再確認 — **[確定]**

- 対象を **0x465130（save）側に特定**して再計測（probe03 item6b）: `[0x465130, 0x465860)` の raw E8 = **39 件**、target に `0x453A90` は **0 件**（一覧に 0x452330/0x452600/0x4535A0/0x45F2E0/0x461680 等は現れるが formatter は不在。一部は即値由来の誤検出だが 0x453A90 の不在に影響なし）。
- 全体 callers(0x453A90) = **2 件**: `0045B029`（caller1）/ `0045CD9F`（caller2）— 保存帯外（probe03）。
- v304 追補B の負証拠は 0x465860（=LoadGame、§5-2 裁定どおり）帯で測定されていたが、**0x465130 側で再計測しても同一結論** → §5-2 の残存疑い（「caller2 窓の計測範囲外」）は解消。

**残点**: 間接呼出（`FF15`/`FF10` 等）は未走査 → **残点 R2**。

**裁定更新案**: v311 §5-2 後半の「0x465130 側未計測の可能性を残す」→「**0x465130 側でも E8=0 を確認済み（間接呼出を除く）**」に更新。

---

### 件7（§6-7）プロセスレベル 6 ラベルの保存有無 — **[確定負] 維持（格上げ根拠なし）**

**新証拠（probe09/12/16）: jumptable dispatch は changeFlags 非依存**
- `mov ecx,[esi+0x58]`@0x453B56 → `mov eax,[edx+8]`（vtable+8 仮想呼出）@0x453B60-65 → `add eax,1` / `cmp eax,4` / `ja 0x453b96` → **`jmp dword ptr [eax*4 + 0x458e38]`**@0x453B6F。
- feed = `RTDynamicCast`（`call 0x9832e6`@0x453B14/0x453B2C/0x453B42、type 0xB03054 / 0xB05538 / 0xB03070、ebp=form オブジェクト）→ **ライブオブジェクト getter 由来**。
- 表: [0]=`0x453B76` / [4]=`0x453CE3` / [8]=`0x453CB2` / [12]=`0x453C81` / [16]=`0x453C50`、第 6 エントリ = `0xCCCCCCCC`（範囲外ガード）。
- **index0 target 0x453B76** = `cmp byte ptr [esp+0x178], 0` → `je 0x453c33`（quiet 分岐のみ・**flag ゲート無し**）＝プロセスレベル系が changeFlags を読まない直接証拠。

**判定**: v311 row1-6 の [確定負]（67 件に process level 名なし・S12 対象外）を **維持 [確定負]**。changeFlags 非依存という新証拠は格上げ（[未対応] 化）に**反する**方向の裏付けであり、保存側 change record の存在根拠は得られず。

---

### 件8（§6-8）CHANGE_QUEST_SCRIPT の 1:2 同定 ＋ VA 順序逆転 2 組の site 帰属 — **[確定]**

**(a) QUEST_SCRIPT 1 ゲート 2 ラベル（probe06 item8a/10）**
- ゲート: `test edx,0x8000000`@0x4541CB → `je 0x458e1a`。quiet: `'CHANGE_QUEST_SCRIPT\r\n'`@0xA39DF0（dword 列書込 @0x4541F0-0x454223 → `jmp 0x458e1a`）。
- normal (`je 0x45422c`@0x4541DF): `RTDynamicCast`（`push 0 / push 0xb05468 / push 0xb02f9c / push 0 / push ebp / call 0x9832e6`@0x45422C-0x45423B）→ `test eax,eax / je 0x454265`、**`cmp dword ptr [eax + 0x58], 0` / `jne 0x454265`**@0x454247 → `[eax+0x58]==0` のときのみ `'Quest Script(No longer has script locals)'`@0xA39DC4 `push`@0x454255 → sprintf → `jmp 0x454297`。
- 否则（0x454265）: getter `push 0x8000000`@0x45426F → vtable+0x4C 呼出 → `'Quest Script(%i)'`@0xA39DB0（push@0x454289）。
- → **1 ゲート・script-locals ポインタ判定で 2 ラベル排他 = [確定]**。v311 row10 の [推定]（「site が 2 ラベルの中間で一意でない」）は**排他条件の発見により解消**、row11 もゲート共通化で [確定] 候補。

**(b) VA 順序逆転 1: LEVELED / Disabled（probe08b/09/16）**
- site `0x455CB3` = `mov esi, 0xa39988`@0x455CB2 の即値 = **LEVELED quiet** `'CHANGE_REFR_EXTRA_LEVELED_CREATURE\r\n'`@0xA39988（quiet 帯 0x455C9C..0x455CBA 内）。
- **LEVELED ゲート = `test edx,0x10000000`@0x455C82**（v311 row38「ゲート未取得」→ **取得**。row35 の「＋0x10000000@0x455C82」は row38 への誤帰属）。normal: `je 0x455e18` → getter `push 0x10000000`@0x455E22 → `'Leveled Creature(%i)'`@0xA3993C `push`@0x455E3E。
- Disabled: ゲート `test edx,0x40000000`@0x455C51 → `je 0x455c7e`、quiet `'CHANGE_REFR_DISABLE_STATE\r\n'`@0xA399B0（`mov esi,0xa399b0`@0x455C77、帯 0x455C63..7C → fall 0x455C7E）、normal `je 0x455cbf`@0x455C61 → 0x455CBF ブロック → `'Disabled(%i)'`@0xA39978 push@0x455CF3。
- **LEVELED 再入**: `jmp 0x455c82`@0x455E13（Disabled 処理末尾から LEVELED ゲートへ戻る）＝連鎖構造を確認。
- → **site 0x455CB3 = LEVELED quiet [確定]、label 0x455CF3 = Disabled normal [確定]**。順序逆転は quiet 文字列 push の compiler layout であり、**dword-inline hoist 嫌疑は解消 [確定]**。

**(c) VA 順序逆転 2: ANIMATION / OBLIVION（probe08b/10/13）**
- site `0x4557DC` = `mov ecx, [0xa39a4c]`@0x4557DA の即値 = **ANIMATION quiet** `'CHANGE_REFR_ANIMATION\r\n'`@0xA39A4C（quiet 帯 0x4557C4..0x45580F 内）。
- **ANIMATION ゲート = `test edx,0x2000000`@0x4557AA**（v311 row32「ゲート未取得」→ **取得**）、quiet `je 0x455908` → label `'Animation(%i)'` 0x45592E。
- OBLIVION: ゲート `test edx,0x800000`@0x455773 → `je 0x4557a6`、quiet `'CHANGE_REFR_OBLIVION_FLAG\r\n'`@0xA39A64（`mov esi,0xa39a64`@0x45579F）、normal `je 0x455814`@0x455783 → `test dl,0xE`@0x455814 → `'Oblivion Flag(0)'`@0xA39A38 push@0x45581D。
- flow: OBLIVION normal `jmp 0x455860` → … `jmp 0x4557aa`@0x455903（ANIMATION ゲートへ戻る）＝連鎖確認。
- → **v311 row30 の「flag 0x2000000@0x4557AA」は ANIMATION ゲートへの誤帰属＝訂正提案**（row30 の実ゲートは 0x800000@0x455773）。row30 の [確定] は S8「0x2000000 適用」引用を含むため、その artifact 側再突合を **残点 R6'** として明記（formatter 側の帰属自体は命令で確定）。

**裁定更新案**: row10 [推定]→**[確定]**、row11 [部分確定]→**[確定]**、row38 ゲート `0x455C82` 追記（[部分確定] 維持・ゲート列のみ更新）、row32 ゲート `0x4557AA` 追記、row30 から `0x2000000@0x4557AA` を除去（S8 引用は要再突合）、row35 から `＋0x10000000@0x455C82` を除去、§5-8「VA 順序異常」を「quiet push の compiler layout 由来・帰属確定」へ更新。

---

### 件9（§6-9）変更レコードの flags 載せ位置 — **[確定]**

**flags = 12B レコードの +5**（formID u32 + formType u8 の直後）。命令根拠: `mov eax,dword ptr [esp + 0x21]`@0x46681B（=[esp+0x1C]+5）→ 452CF0 arg2。tail frame `<u16 size><char type><typeID>+payload` には **flags 無し**（tail payload は 453500 が tailSize+4 で別アロケート、0x4667AF）。

**pass2 の tail frame header 読取**: `[0xb333b00]+0x14` カーソラから 4B → [esp+0x24]（`cmp byte ptr [esp + 0x26], 0x30`@0x4617F1 / `cmp byte ptr [esp + 0x27], 0x5b`@0x4617FF = **v283 の 0x5B ゲートと一致**）、別パスで [esp+0x30]（`movzx eax,byte ptr [esp + 0x32]` / `cmp eax,0x31`@0x461AAD）。

**formatter 引数マッピング（probe12/16 E・新知見）**: 入口で `call 0x4535a0`@0x453AD0（`push [esp+0x170]` → `push ebp`）→ 返値を arg3 スロット [esp+0x170] へ書戻し、以降の全 flag ゲートがそれを読む。`[esp+0x174]` = **formType byte**（`mov al,[esp+0x174]`@0x453F2B/0x456263/0x458881 + `cmp byte[esp+0x174],0x32`@0x457EE1 の計 4 参照）、`[esp+0x178]` = quiet flag（全ラベルの `cmp byte[esp+0x178],0`）。ebp = arg2 = form オブジェクト（`mov edx,[ebp] / mov eax,[edx+0x4c] / push 0 / mov ecx,ebp / call eax`@0x453AF2-0x453AFC）。

**裁定更新案**: v311 §2.2 に「**12B レコード = `<formID><formType><changeFlags><u8><tailSize>`、flags=+5、tail frame に flags は載らない**」を追記。v283 件2 / v297 件3 の接続点（12B entry ↔ pass2 frame）を確定。v289 の `<key><flags><refcount>` 3 項目記述と実レイアウト（+9 u8 / +10 u16）の対応は **残点 R3** に含む。

---

### 件10（§6-10）0x5B 境界と formatter 型 ID の対関係 — **[確定]**

- **`cmp al,imm`（型ゲート）= 13 件・最大 0x3D・最小 0x6・values ≥0x40 は 0 件・`0x5B present: False`**（probe06 item10）。
- `cmp byte[esp+0x174],imm` = 1 件（0x457EE1, imm 0x32 = Crime Gold、probe09 C）。
- formatter 全 `test dl/edx,imm` = **67 件**（probe15 A）、うち **imm 0x5B の test/cmp = 0 件**。
- **Lock の `test dl,0x40`@0x4565C3 は changeFlags ゲート**（probe15 L36）→ v311 §6-10 の前提「formatter 側の型 ID ゲート最大は 0x40（Lock）」は**誤り**（flag を型 ID と読み違え）。
- `cmp eax,0x19`（127 件）は**文字列長チェック**（`sub eax,edx / cmp eax,0x19 / jae`、例 0x453BB9 / 0x455D83）で型 ID と無関係。
- → **型 ID ゲート最大 0x3D < 0x5B [確定]、ExtraSound(0x5B) にラベルなし [確定]**（§6-10 の結論は維持かつ強化）。

**裁定更新案**: v311 §6-10 の「最大 0x40（Lock）」→「**最大 0x3D。Lock の 0x40 は flag（`test dl,0x40`）**」に訂正。row45 Lock の probe 行も flag 読みへ（件2 と同一裁定）。

---

### 件11（§6-11）pass2（FUN_00461680）到達の単一性 — **[確定]**

- **callers(0x461680)（global 全走査）= 1 件: `004651C3`**（FUN_00465130 = SaveGame 内）（probe03 item11）。
- ロード側に pass2 相当の呼出は**存在しない**（global 走査で 1 件のみ）→ 「ロード側にも pass2 相当があれば判定が変わる」シナリオは消滅。
- 参考: callers(0x465130) = 6 件（`00466AF6 00466BD0 00466D21 005024BA 005D3246 005D3438`）＝既存 6 件と一致。

**裁定更新案**: v311 §2.1 の pass2 到達記述を「**単一 caller = 0x4651C3（SaveGame）[確定]**」で補強（行の判定変更なし）。

---

### 件12（§6-12）Warnings.txt oracle 突合 — **[未確定]**

- 本件は**実機 Warnings.txt 生成待ち**であり、EXE 静的解析では B 型ゲートの実機裏付けが不可能（v306/v308 で出力経路は負証拠化済み、本タスクでの新証拠なし）。
- 帯内に関連する静的証拠は得られていない（ formatter 側の bit31/0x2000000 等は本ファイル件5/件8 で処理済み）。
- **[未確定]（実機待ち）**。Phase2 の 12 件中、静的解析で結論を出せない唯一の項目。

---

## 2. 判定サマリ

### 2.1 12 件の判定一覧

| 件 | §6 項目 | 判定 | 要点（VA） |
|---|---|---|---|
| 1 | 5 caller 引数再検証 | **[確定]** | 5/5 一致、0x466D5F の arg index のみ訂正（arg2=[ecx+0x1C0]） |
| 2 | ゲート形式 7 ラベル | **[確定]** | 7/7 が二段ゲート（型 `cmp al` → flag `test dl`）、形式争い解消 |
| 3 | 同値多義 13 組 | **[部分確定]** | 12/13 組を formType 区画で分離、0x10000 組 %s↔Non-saved のみ保留 |
| 4 | 452CF0 flags 実体 | **[確定]** | 12B +5 = changeFlags 同物（3 段連鎖）、bit3 = 同一 dword 文脈依存 (a) 採用 / (b) [確定負] |
| 5 | [未対応] 2 件裏取り | **[部分確定]** | Cell Changed=bit31 ゲート+ロード側分岐、Dropped Item=保存側 0x461E67 分岐 → 格上げ |
| 6 | 保存鎖 formatter 呼出ゼロ | **[確定]** | 0x465130..0x465860 の E8=39 件中 0x453A90=0 件 |
| 7 | プロセスレベル 6 ラベル | **[確定負]** | jumptable = live getter（0x453B6F）・index0 に flag ゲート無し → 格上げ根拠なし |
| 8 | QUEST_SCRIPT 1:2 ＋ VA 逆転 2 組 | **[確定]** | 排他条件 [eax+0x58]==0、LEVELED=0x455C82 / ANIMATION=0x4557AA を取得 |
| 9 | flags 載せ位置 | **[確定]** | 12B +5、tail frame に無し、formatter 入口 0x453AD0 経路 |
| 10 | 0x5B 境界と型 ID | **[確定]** | 型ゲート最大 0x3D・0x5B ゲート 0 件・Lock 0x40 は flag |
| 11 | pass2 単一性 | **[確定]** | callers(0x461680)=1 = 0x4651C3（SaveGame） |
| 12 | Warnings.txt oracle | **[未確定]** | 実機待ち・静的解析不能 |

### 2.2 5 段階集計（12 件）

| 判定 | 件数 | 件番号 |
|---|---|---|
| [確定] | **8** | 1, 2, 4, 6, 8, 9, 10, 11 |
| [部分確定] | 2 | 3, 5 |
| [推定] | 0 | — |
| [未確定] | 1 | 12 |
| [確定負] | 1 | 7 |

### 2.3 v311 への裁定更新案サマリ（本タスクで提案する差分）

1. **§5-3（ゲート形式 7 ラベル）**: 「形式未統一」は**二段ゲートで解消**。D 表 flag 読みを正、probe 行 typeID 呼称は型ゲートへ読み替え。行判定は [部分確定] 維持（残理由を「保存側値の多義」に限定）。
2. **§5-4（同値多義 13 組）**: **(a) formType 別の意味共有を命令で裏付け（12/13 分離）**。0x10000 組 %s↔Non-saved のみ保留 → [部分確定]。
3. **§5-5（bit3 二義）**: **(a) 同一 dword 文脈依存多義 [確定]、(b) 別物説 [確定負]**。452CF0→pass2→formatter の 3 段連鎖で裏付け。
4. **§4.2 [未対応] リスト**: row26 Cell Changed / row49 Dropped Item とも **[部分確定] へ格上げ → 残 [未対応] 0 件**。
5. **§6-10「型 ID ゲート最大 0x40（Lock）」**: **最大 0x3D に訂正**（Lock の 0x40 は flag）。
6. **row30 Oblivion Flag**: `0x2000000@0x4557AA` を**除去**（ANIMATION row32 のゲート）。S8 引用は artifact 再突合へ（残点 R6'）。
7. **row35 Disabled**: `＋0x10000000@0x455C82` を**除去**（LEVELED row38 のゲート）。
8. **row10/11 Quest Script**: [推定]/[部分確定] → **[確定]**（排他条件の発見）。
9. **row32/38**: ゲート列を `0x4557AA` / `0x455C82` で更新（判定は [部分確定] 維持）。
10. **§5-8 VA 順序逆転**: 「dword-inline hoist 嫌疑」を**解消**（quiet push の compiler layout・site 帰属確定）。
11. **§6-1 parenthetical**: 0x466D5F を「arg2=[ecx+0x1C0]・arg1=0」に訂正。
12. **§5-2 後半**: 0x465130 側でも E8=0 確認済み（間接呼出を除く）と補強。
13. **§2.2 フレーム仕様**: 12B レコード layout と flags=+5、tail frame に flags 無しを追記。

---

## 3. 新規残点（10 項：R1-R9 ＋ R6'）

| # | 残点 | 判定 | 根拠 / VA |
|---|---|---|---|
| R1 | pass2 の bit3 除去/翻訳（`==8` 単独→0x452DF0 force remove、bit3+他→`and ebx,0xfffffff7` / `or ebx,0x800`@0x461CF4-0x461CF7）と formatter の bit3 ゲート（`test dl,8`@0x45468B / 0x458103）の発火緊張（ロード済みファイルで formatter bit3 が発火し得ない） | [未確定] | pass2 は保存時処理、formatter は表示時。台帳の bit3 生死の遷移が未追跡 |
| R2 | formatter への**間接呼出**（FF15/FF10 等）未走査（件6 の負証拠は E8 のみ） | [未確定] | callers(0x453A90)=2 は E8 基準 |
| R3 | v289 `<key><flags><refcount>` の refcount と arg3=ebx（0x453500 alloc 結果）/ 12B +9 u8・+10 u16 の対応 | [未確定] | 0x4667AF `call 0x453500` → `mov ebx,eax`@0x4667BA |
| R4 | `CHANGEFLAG_*` 3 件（0xA39730 / 0xA39A98 / 0xA39B20）の正体（quiet 専用名 か save レコード名 か）＝67 `CHANGE_*` 双方向が未成立 | [未確定] | s60 照合で band 内 3 件のみ |
| R5 | 0x10000 組の **%s@0x457718 ↔ Non-saved@0x457853** の sub-dispatch（同区画 {32,33}・同 flag） | [未確定] | probe15 L51-52、%s の別条件は v304 記載の 0x5E4080 のみ |
| R6 | **bit7(0x80) の新規多義**（Owner gate `test dl,dl`/`jns`@0x4566D3-5・区画 0x31、Crime Gold `cmp byte[esp+0x174],0x32`@0x457EE1 + `test dl,dl`/`jns`@0x457EEF・区画 0x32、いずれも getter `push 0x80`）の保存側突合 | [未確定] | v311 §5-4 の多義リストに 0x80 が不在＝追記要 |
| R6' | v311 row30 の S8「0x2000000 適用」引用と、0x2000000=ANIMATION ゲート（0x4557AA）という formatter 帰属の artifact 側再突合 | [未確定] | 件8 (c) |
| R7 | formatter 入口の arg3 / `call 0x4535a0`（返値=changeFlags）の名前・意味、arg4=formType・arg5=quiet の caller 側渡し | [部分確定] | 0x453AC7-0x453AD5、[esp+0x174] 参照 4 件（probe17 C） |
| R8 | pass2 の ebp（formType）の**初出ロード地点**（0x461D77 保存より前）未追跡。ただし比較定数 0x31/0x32/0x33 が formatter 区画と一致 | [部分確定] | 0x461DEC `mov ebp,[esp+0x34]` ← 0x461D77、`cmp ebp,0x31`@0x461E62 が Dropped Item ゲートの前置 |
| R9 | Warnings.txt oracle（件12）の実機検証 | [未確定] | 実機待ち |

**本タスクで解消した v311 の残点・疑い（参考）**: 0x8000000 第 4 ラベル = Quest Script@0x4541CB 同定 / sign ゲート 2 件の帰属（Owner・Crime Gold）/ LEVELED・ANIMATION のゲート未取得→取得 / VA 逆転の hoist 嫌疑解消 / ebp 出所 / formatter 入口シグネチャ / 0x45F640 非 call target / 0x465130 側 E8 計測。

---

## 4. selftest 再現メモ

### 4.1 実行形式

```powershell
chcp 65001; python $env:TEMP\opencode\v312_<n>.py *> $env:TEMP\opencode\v312_<n>.out
```

- 中間物: `%TEMP%\opencode\v312_*`（.py 26 本 + .out 26 本 = 52 ファイル、本タスクのみ新規）。
- 全 probe は `tmp/re/re_util.py`（変更なし）を `sys.path` 経由で再利用し、capstone で .text を read-only disasm。raw E8 判定は `va+5+rel==target`。

### 4.2 probe 一覧と主結果

| probe | 対応件 / 質問 | 主結果（自己照合の値） |
|---|---|---|
| 01 | 件1: callers(0x465860) | **5 件** `00466B31 00466D5F 005025F7 005AE1E6 005B5DA8`、callers(0x45F2E0)=8、callers(0x45F640)=0 |
| 01b | 件1: fn head / push 順列 / 45F2E0 | head 0x466D50（先頭 `mov eax,[ecx+0x1c0]`）・0x5AE190、.ess=`call 0x9864d9`@0x45F646 |
| 02a/02b | 件2: 7 ラベルのゲート | 全て型 `cmp al` → flag `test dl` の二段 |
| 03 | 件4/6/11: 452CF0・E8・pass2 | **E8 in [0x465130,0x465860)=39**（0x453A90=0）、callers(0x453A90)=**2**、callers(0x461680)=**1**（0x4651C3）、callers(0x465130)=**6** |
| 03b-03g | 件4: 452CF0 caller・pass2 ebx/bit | 452CF0 caller=1（0x466829）、pass2 bit テスト列 |
| 04 | 件4: pass2 0x461C40..0x461D00 | `and ebx,0xfffffff7` / `or ebx,0x800`@0x461CF4-0x461CF7 |
| 06 | 件8/10: QUEST_SCRIPT・cmp al | QUEST gate `test edx,0x8000000`@0x4541CB、**cmp al count=13 max=0x3d min=0x6 / ≥0x40: 0 / 0x5B: False** |
| 07 | 件3: 型チェーン・区画 | チェーン 13 件と 10 区画 |
| 08a/08b | 件4 caller / 件8 LEVELED・ANIMATION | caller1 0x45B029・caller2 0x45CD9F、quiet 帯の帰属 |
| 09 | 件4/5: call site・bit31・文字列 | 12B +5 命令列、帯内 bit31 テスト=**2 件**（0x4647A6/0x46630D）、`cmp byte[esp+0x174],imm`=**1 件**、quiet 文字列所在 |
| 10 | 件5/8: OBLIVION・ANIMATION・Dropped quiet | gate 0x455773/0x4557AA、Dropped `test edx,0x20000`@0x456A13、jumps 一覧 |
| 11 | 件5: band 内 CHANGEFLAG_* | **3 件**（0xA39730/0xA39A98/0xA39B20） |
| 12 | 件4/9: formatter head・pass2 0x461D60.. | 引数マッピング、`cmp ebp,0x31/0x32/0x33` |
| 13 | 件8: site jump 羽 | `jmp 0x4557aa`@0x455903 等 |
| 14 | 件5: 保存側 0x20000 | pass2 0x461E67 分岐 |
| 15 | 件10: 全ゲート | **test dl/edx,imm = 67 件**、jns=**3 件**（0x4552C5/0x4566D5/0x457EF1） |
| 16 | 残問 5 点 | index0=0x453B76（quiet のみ）、`jmp 0x455c82`@0x455E13、ebp←0x461D77、入口 0x453AD0 |
| 17 | sign ゲート帰属 | Owner `test dl,dl`@0x4566D3（edx=[esp+0x170]）、Crime Gold 0x457EE1/0x457EEF、**[esp+0x174] 参照=4 件** |
| 18 | 型ゲート演算子 | `mov al,[esp+0x174]` 選択肢の chain head（0x456263/0x458881）と `cmp al` 連鎖を確認 |

### 4.3 主要件数の自己照合（再現時の一致確認ポイント）

- callers(0x465860)=5 / callers(0x453A90)=2 / callers(0x461680)=1 / callers(0x465130)=6
- E8 in [0x465130,0x465860)=39（target 0x453A90=0）
- formatter: `test dl/edx,imm`=67、`cmp al,imm`=13（max 0x3D）、`cmp byte[esp+0x174],imm`=1、imm 0x5B=0、jns=3、[esp+0x174] 参照=4
- 帯内 bit31 テスト=2、band 内 CHANGEFLAG_*=3、型チェーン=13

---

## 5. 遵守宣言

- EXE: read-only 解析のみ（帯 0x45D000-0x466000 ＋ 0x401000-0x410000 / 0x452000-0x455000、formatter 窓 0x453A90..0x458E60）。ESM: 未接触。git: **変更系コマンド（add/commit/checkout 等）は一切未実行**。成果物確認のため `git status --porcelain` を1回試行したが出力は得られず（read-only・リポジトリ状態への影響なし）。
- 編集: **本ファイルのみ新規作成**。既存 tmp/re 成果物・台帳・STATE・TODO・docs・GT・ソース・.github/** は読み取りのみ（v311 は無編集、訂正は本ファイルの「裁定更新案」で提示）。
- 中間物: `%TEMP%\opencode\v312_*` のみ新規（52 ファイル）。`tmp/re/re_util.py` 変更なし。
- 判定整合: 12 件 = [確定] 8 + [部分確定] 2 + [未確定] 1 + [確定負] 1 = 12（一致）。裁定更新案 13 項・新規残点 10 項（R1-R9 ＋ R6'）。

## 7. 次束E（v315）による R1-R9 裁定（2026-11-07）

`v315_v312_residuals.md`（31,900B/probe 7 本）による本ファイル §3 残点の裁定:

| 項 | 裁定 | 概要 |
|---|---|---|
| R1 bit3 発火緊張 | **[部分確定]** | pass2 の bit3 除去は実在するが formatter は帯内台帳 API caller=0（台帳を読まず）。非 CELL は `or ebx,8` @0x453778（5F0310 述語）で**表示時に合成** → 「ロード済みで発火し得ない」は [確定負] 方向。CELL の Cell Flags は入力パススルーで、ファイル内 bit3 の存否（保存ライタ＝v313 帯）が唯一の残件 → N1 へ |
| R2 FF15 間接呼出 | **[確定負]** | 0x453A90 の全セクション格納 0・imm32 0・FF15/FF25 0・E9 0 → **器が不在で構造的に不可能**、呼出形は E8 2 件のみ |
| R3 refcount vs tailptr | **[確定]** | entry+4（v289「refcount」）＝453500 pool alloc の **tail buffer ptr**（free 対象で証明・refcount 説は [確定負]）。12B +9=tail typeID / +10=tailSize |
| R4 CHANGEFLAG_* 3件 | **[確定]** | .rdata 3 件のみ・xref 各 1・全て formatter quiet（保存系参照 0）＝**quiet 専用名**、67 CHANGE_* と双方向成立 |
| R5 0x10000 組 | **[確定]** | %s ブロック実ゲート=**bit17 @0x4576D3**（Non-saved=bit16 @0x457853・0x457718 の 0x10000 は getter マスク選択）→ v311 row63 訂正へ |
| R6 bit7 多義 | **[部分確定]** | Owner `test dl,dl/jns` @0x4566D3（区画 0x31）↔ Crime Gold 同 @0x457EEF（type 0x32）＝**formType 分離**・getter `push 0x80` 共通・調査可能帯の保存側 test 0x80=0 件 → 残件は v313 帯（N7） |
| R6' 0x2000000 一意 | **[確定]** | v282_9 再検証一致 → v311 row30「一意」FALSE・[確定]→[部分確定] 格上げ |
| R7 callee 引数 | **[部分確定]** | callee 5 引数確定（arg1 buf / arg2 form / arg3 flags in-out / arg4 formType / arg5 quiet）、caller 側は v313 帯（N2） |
| R8 ebp formType 出所 | **[確定]** | `[esp+0x34]` ← tail char @0x461ABC / formType @0x461BE8 の 2 経路。**本ファイルの「←0x461D77」は push 数え誤りを訂正**（0x461D77 は [esp+0x2C] へ key 格納） |
| R9 Warnings oracle | **[未確定]** | 実機待ち（新証拠なし・従来どおり） |

新規残点 N1-N9（大半が v313 帯との跨り）は #81 へ。

## 8. 追補（次束G＝v317 統合時・2026-11-07）

`v317_v315_residuals.md` の訂正提案 13 項のうち本ファイル関連を反映:

1. **件5 の VA 誤引用を訂正**: 該当箇所は v317 §5 の対応表に従い正 VA へ差替え（本節のみで旧記述は上書き不削除）。
2. **formatter の `jns` は 3→4 件に訂正**: 第 3 ゲート `test dl,dl / jns` @0x458771 ＝ **`CHANGE_ACTOR_BASE_FULLNAME`（type 0x23/0x24）を新発見**（本ファイル R6 節の jns 件数を訂正）。
3. **pass2 呼称の訂正**: pass2 の唯一 caller ＝ `0x4651C3`（`FUN_00465130` 保存オーケストレータ内）→ **pass2 は保存直前実行**。「LoadGame pass2」と呼称した v289 系記述は**誤り**（詳細仲裁＝v317 §4-3）。
4. **R1/R6 の帰結更新**: bit3 単独 entry は pass2 削除で非書出（N1）・保存側 bit7 分岐 0 件（N7）＝本ファイル §7 の R1/R6 部分確定を本裁定で補強。
5. **教訓追加: off-by-pad 第 7 例 ＝ `0x45CC60`**（既知 6 例に追加: 0x466D4D/0x5AE18D/0x44F3C1/0x51B484/0x45B300/0x452330）。

## 9. 動的照合による訂正（v320 統合時・2026-11-07）

実 .ess 検体（836,742B・MD5 FC32EFAEB90D26141D3DC4A325AC10E7）による `v320_ess_dynamic_phaseA.md` で、本ファイルの size 記述を**訂正**する:

1. **「size = offset 38 起点・total = 38+size」は誤り** — **実 size field ＝ u32@34（実測 147,521）**。`38 + size = 147,559` は **ヘッダ+thumb ブロック終端と完全一致**（＝v307 の `0x24+len+len+thumb` 式を実測確定）。**ファイル総長（836,742）と一致する u32 はファイル内 0 件 [確定負]**（payload 689,183B は size に含まれない）。
2. **u32@38 は count 欄ではなく counter = 1**（＝ファイル名の 'Save 1'）[確定]。
3. 「dow=318 異常・mtime 不整合」も**観測スクリプト側の field 順誤り**として解除（正の布局と双帯の裁定は v320 §4-1・§4-3: UTC 05:20:22.318 / local 14:20:22.318・ms 同値・mtime 秒一致）。
