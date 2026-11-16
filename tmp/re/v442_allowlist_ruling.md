# v442 — 許可リスト・flag-B chain root・`[esp+0x5c]` 経路確定の裁定（V-442-1..3 完遂・派生残点第34弾）

- 採番: **v442**（派生残点群・probe prefix **v442_1..v442_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v441 の残点 **V-442-1【低】（`0x19`/`0x28` FormType の最終同定と `FUN_004fab20` 許可リスト `{6,0x23}` の意味）／V-442-2【低】（flag B chain root `FUN_0056a3b0` の identity と `FUN_0056b220/2e0` の callers）／V-442-3【低】（`[esp+0x5c]` path CFG と docs census unconfirmed 3159 の切り分け）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-442-1: `0x23` FormType の同定** | **[確定]** | expanded window（-0x90）＋denylist 拡張で **`0x23 → .?AVTESNPC@@` を解決 [確定]**（v440 の masked だった switch 外値。`NPC_` は inventory 非対象で switch 不在と整合） |
| **V-442-1: `0x19`/`0x28`（switch set 内2値）** | **[未確定]（残点）** | expanded window でも masked（noise `TESIcon` 等と window 共存で解消せず）。**別法（fn head からの ctor 直接 decode、window 内 vtable 全列挙）が要** |
| **V-442-1: `FUN_004fab20` 許可リスト `{6, 0x23}` の意味** | **[確定]（値）＋[部分確定]（用途）** | v441 表より **`0x06=TESFaction`、`0x23=TESNPC`（本 probe で確定）** → **`byte[obj+4] ∈ {TESFaction, TESNPC} のみ許可、否则 fail [確定(命令)]**。**`FUN_004fab20` は compiled script の char-opcode decode/実行 fn [確定(decompiled)]**（L194461 `switch(cVar4) { case 'G': case 'Z': case 'r': … }`＝文字タグ opcode、`FUN_004fa9c0(short, …)` 呼出し多数）→ **許可リストはその中の「Faction/NPC のみ受容する parameter 検証」[推定高]**（どの command case かは [未確定]） |
| **V-442-2: flag-B chain root `FUN_0056a3b0` の identity** | **[確定]（signature）＋[部分確定]（用途）** | L269931: **`undefined2 FUN_0056a3b0(undefined4 param_1, float param_2, float param_3)` ＝ 条件比較 evaluator [確定(decompiled)]**（`switch(param_1)`: case0 `if (param_3 != param_2) return 0;` / case1 反転 / case2 `if (param_2 <= param_3) return 0;` / case3 `<` …＝ **比較演算子の実装**）。**その body 内 `0x56A464` が `FUN_0056ad60`（→flag B getter `FUN_0056b190`）の唯一 caller [確定]** → **比較評価時に type-B flag を読む [確定（chain 実在）]、文脈的用途は条件式の type 検証 [推定高]** |
| **V-442-2: `FUN_0056b220/2e0` の callers** | **[未確定]（残点）** | 本 v442 群では未調査（probe budget） |
| **V-442-3: docs census unconfirmed 3159 の切り分け** | **[部分確定]** | **末尾 byte histogram**: `0xFF=1859 / 0x00=1214`（99%+）・**jcc rel32（end-2＝`0x0F`）は0件**・その他は≤10件ずつ → **non-ret 境界の大半は `FF …`（`jmp/call [reg*4+table]` 等 indirect）か disp32 末尾 `0x00` で終わる＝indirect-jmp tail/switch 表リレー [推定高]**。**ret-confirmed 3038 は下限として堅い [確定]**、non-ret 3159 の内訳は「tail-jmp epilogue」と「switch case チャンクの intra-function padding」が混在 [推定]（両者の機械的分離は残点）→ **欠落下限3038／上振れ上限6197 のレンジ維持** |
| **V-442-3: `[esp+0x5c]` path CFG** | **[部分確定]（経路の完全列挙）＋[確定]（v439 訂正1件）** | **(i) `FUN_00404fd0` 全 body 完読（L4119-4136）＝`uint __fastcall(ulong* p)`: `if (*(ushort*)(p+1)==0xffff) return strlen((char*)*p); else return *(ushort*)(p+1);` ＝ string length getter・読みのみ [確定] → v439 の「`408FCF→FUN_00404fd0` は lea 経由 fill」記述を訂正：**404fd0 は書かない**。**(ii) 書くのは direct 3件（`407CBA` edx／`407E5B` ecx／`408F72` eax=0）＋ dtor 1件（`409027→FUN_00402da0`: `*p=0/+6=0/+4=0` のゼロ化）＝**fill 点は5件（非ゼロは direct の3件のみ）[確定]**。**(iii) CFG**: `0x409E80..0x40A29D` への branch targets 42件、**nearest target `0x40A262` から `0x40A29D` まで `[esp+0x5c]` 触れず → block 継承 [確定]**（値はそれより前の block から。非ゼロ経路は `407CBA/407E5B` のどちらか [確定]、**到達 path 決定は [未確定]＝残点**） |

## §2 証拠（v442_1..v442_3）

### §2.1 FormType 解決と許可リスト（v442_1）
```
expanded window (-0x90) + denylist: 0x23 -> .?AVTESNPC@@  [確定]
  0x19 / 0x28 / 0x09 / 0x0C / 0x0D / 0x10 / 0x11 / 0x1F / 0x2C / 0x32..0x35 -> still masked
FUN_004fab20 (L194420): void FUN_004fab20(double*, int, int*, int, undefined4, int, int, char)
L194461: switch(cVar4) { case 'G': case 'Z': case 'r': sVar1=*(short*)(...);
         local_18 = FUN_004fa9c0((int)sVar1, param_7); ... }   <- char-opcode decode
machine 0x4FB160..0x4FB1A1 (v441_3):
  test edi,edi / je fail
  movzx ecx, byte [edi + 4]     ; FormType
  cmp ecx, 6     / je ok        ; TESFaction
  cmp ecx, 0x23  / jne fail     ; TESNPC
  jmp ok
  cmp byte [eax + 4], 0x43      ; TES4 EffectShader 判定（別 gate）
  4FB1A1: cmp eax, 0x21         ; arg type == Form Type (v437 table)
```

### §2.2 flag-B chain root（v442_2a）
```
L269931: undefined2 FUN_0056a3b0(undefined4 param_1, float param_2, float param_3) {
  switch(param_1) {
    case 0: if (param_3 != param_2) return 0; break;
    case 1: if (param_3 == param_2) return 0; break;
    case 2: if (param_2 <= param_3) return 0; break;
    case 3: if (param_2 <  param_3) return 0; ...
chain: FUN_0056a3b0 --call@0x56A464--> FUN_0056ad60 --call@0x56ADAC--> FUN_0056b190 (flag B getter)
```

### §2.3 census split + CFG（v442_2b/c, v442_3）
```
minseg=0x10 boundaries (gaps>0x200): ret-family 3038 / non-ret 3159
  non-ret last-byte: 0xFF=1859, 0x00=1214, else <=10 each
  end-2 == 0x0F (jcc rel32): 0
  -> non-ret は indirect jmp/call (FF ..) か disp32 末尾 0x00 [推定高]
branch targets into [0x409E80, 0x40A29D) inside FUN_00407AE0: 42
  nearest <= 0x40A29D: 0x40A262 -> decode to 0x40A29D: [esp+0x5c] touch 0
  -> block 継承 [確定]
FUN_00404fd0 (L4119-4136): uint __fastcall(ulong* p) {
  if (*(ushort*)(p+1) == 0xffff) { strlen-loop over (char*)*p; return len; }
  return *(ushort*)(p+1); }   ; READ ONLY -> v439 の fill 記述を1件訂正
```

## §3 残点 **V-443-1..3**
1. 【低】**`0x19`/`0x28` FormType**（fn head からの ctor 直接 decode＋window 内 vtable 全列挙）＋ **許可リスト `{TESFaction, TESNPC}` がどの opcode case の validation か**（`0x4FB160` の所属 case を jump table から特定）。
2. 【低】**`FUN_0056b220`/`FUN_0056b2e0` の callers**（所属 pass）＋ flag-B chain の文脈（`FUN_0056ad60` 呼出が比較 evaluator のどの case か）。
3. 【低】**`[esp+0x5c]` 非ゼロ経路（`407CBA`/`407E5B` の到達 block）の CFG 確定**＋ **docs census non-ret 3159 の機械的分離**（indirect tail＝真 function end と switch case チャンク境界の切り分け）。

## §4 検証・自己批評
- probe: **v442_1..v442_3 の3本全 EXIT=0**（V-442-1 は `0x23` 解決と許可リスト値を確定・2値 masked は残点／V-442-2 root signature 確定・callers は残点／V-442-3 経路完全列挙＋v439 訂正・path 決定は残点）。
- **決定打の作り**: (i) **window 拡大＋denylist 拡張で1値だけ解決（`0x23=NPC`）→ 2値は解消せず＝別法必要と自覚**（フィルタ改善の限界を数値で確認）→ (ii) **許可リストの値は v441 の49件表との突合で即決（`6=Faction, 0x23=NPC`）**→ (iii) **`FUN_004fab20` の identity は docs body の char-opcode switch（`'G'/'Z'/'r'`）実読** → (iv) **chain root は decompiled の `switch` 本文で比較 evaluator と即決** → (v) **census 分離は末尾 byte histogram＋`0x0F` end-2 検査の2系統**（jcc rel32 ゼロという否定側結果も記録）→ (vi) **`[esp+0x5c]` は「write 全列挙（v438）＋各 leacall の書否を decompiled 全 body で判定」で fill 点を5件に確定**。
- 妥当性チェック: ①`0x23=NPC`⇔switch 外（inventory 非対象）— ②許可 `{Faction, NPC}`⇔それらは script の `->` receiver になり得る型 — ③`FUN_00404fd0` が length getter⇔`return` が2系統とも値返しで store 無し — ④block 継承⇔v438 census の direct write が4092xx 以降0件 — ⑤histogram 99%≤0x02種⇔jcc rel32 ゼロ — **五方向矛盾なし**。
- docs との不整合: **なし**（v439 の ruling 記述に**自ら1件訂正**（`404fd0` を fill と記した点）を追加）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **denoise フィルタは改善しても残る masked は必ず出る** — window 拡大（0x50→0x90）で1件解消した時点で「残りは別法（fn-head decode）」を次 probe の第一歩に書く（v440/441/442 の3連で2値だけ残った理由を明記）。
  2. **許可リスト/即値の意味は「既に確定した対応表（49件 FormType）との突合」で1手で済む** — 新表が出来た直後は旧値の意味付けをその場で片付ける。
  3. **fn identity は docs body の switch 文（`case 'G'` など char リテラル）で一撃** — address 周辺の string push を探すより本文の switch キーが早い。
  4. **census の unconfirmed は末尾 byte histogram で機械分離できる**（`0x0F` end-2＝jcc rel32 を系統的に検査して0件＝否定側も記録）。
  5. **「関数 A が slot に書くか」は decompiled 全 body の `return` の向きで判定** — `lea` 経由の call を fill と決めつけると（v439）訂正が発生する。**lea→call は必ず callee 全 body を読む**。
