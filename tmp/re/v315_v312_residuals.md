# v315_v312_residuals.md — 次束E: v312 §3 新規残点 10 項（R1-R9＋R6'）の EXE 静的検証（L1・日本語報告）

- 作成: 2026-11-07（ai-todo #79 / 次束E）
- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（.text 0x401000 → file 0x400 / .rdata 0xA28000 → 0x627200 / .data 0xB02000 → 0x700C00）
- 本タスク帯（disasm 実施）: **0x452000-0x459000**（pass2 helpers 0x452600/0x452C90/0x452CF0/0x453500/0x453530/0x4535A0・formatter 0x453A90-0x458E38 本体、ジャンプテーブル 0x458E38-0x458E4C はデータ扱いで disasm 除外）＋ **0x460000-0x464000**（pass2 FUN_00461680 0x461680-0x461E90・FUN_00463A90 窓）＋ CHANGEFLAG_* 文字列帯（.rdata 読取）。
- 帯外読取（raw/参照のみ、disasm しなし）: LoadGame dispatch 0x466000-0x466AC6・RTTI TypeDescriptor（.data 0xB02F9C 等）・FUN_005F0310 先頭・全セクション raw byte 走査（E8/E9/FF15/FF25/imm32/文字列在庫）・%TEMP% 保存中間物（v281_*/v282_*）・既存 tmp/re 成果物（v311/v312/v304/v305/v306/v289/v297/v282 他）。
- **v313 帯（0x459000-0x460000 / 0x464000-0x466000 / 0x430000 / 0x400000-0x402000 / 0x4D8000-0x4F2000 / 0x5E1000-0x5E2800）・v314 帯（0x404000-0x406000 / 0x44F000-0x452000 / 0x51B000-0x51C000 / 0x570000-0x574000）は disasm 侵入なし**（該当域は raw caller アドレス列挙のみ）。
- 判定: 5 段階 [確定] / [部分確定] / [推定] / [未確定] / [確定負]。旧結論は削除せず、矛盾は両論併記＋裁定理由を「裁定更新案」として併記。
- 成果物: 本ファイルのみ新規。既存 tmp/re 成果物・台帳・STATE・TODO・docs・GT・ソース（app/**）・.github/** は無編集。ESM 未接触・git 未実行・`tmp/re/re_util.py` 変更なし。
- 中間物: `%TEMP%\opencode\v315_01..07.py/_out`（14 ファイル・本タスクのみ新規）。

---

## 1. R1-R9＋R6' 各項の結果

### R1 — pass2 の bit3 翻訳と formatter bit3 ゲートの発火緊張 — **[部分確定]**

**(a) pass2 bit3 翻訳（帯内再実測・v297 件3 と完全一致）**
- `mov ebx,[esp+0x24]`@0x461CAF（flags＝452600 経由の entry+0。452600@0x461A70 → `mov [esp+0x24],edx`@0x461A87、entry+4 は `mov eax,[eax+4]`@0x461A7B → `mov [ebx+0x14],eax`@0x461A9C で cursor に設定）→ `test bl,8`@0x461CB3 `je 0x461dec`（bit3 なし＝翻訳スキップ）→ `test bl,6`@0x461CBC `jne 0x461dec`（bit1|2 あり＝スキップ）→ `cmp ebx,8`@0x461CC5 `jne 0x461ce1`。
  - 単独 bit3: `push 1`@0x461CD4 → `call 0x452df0`@0x461CD7（force 除去）→ `jmp 0x461dd8`。
  - bit3+他: `and ebx,0xfffffff7`@0x461CF4 → `or ebx,0x800`@0x461CF7 → 再発行（`call 0x453500`@0x461D4D で tail 再確保 → `call 0x45b9a0`@0x461D61/0x461D7B/0x461D94 でヘッダ4B/key4B/旧 tail 追加 → `mov [eax+4],ebx`@0x461DAC（entry+4＝新 tail）・`mov [eax],ecx`@0x461DAF（entry+0＝翻訳後 flags）・`mov [edi+0x14],0`@0x461DB1）。
  - tail 無し経路: 0x461DBC `push 0x80000008`@0x461DCD → `call 0x452d60`@0x461DD3（bit3|bit31 清去マスク＝v312 件4「0x80000008」と一致）。
- → **台帳 entry からの bit3 恒久除去/翻訳 [確定]**（v297 件3 再確認）。

**(b) formatter bit3 ゲート＝3 件 [確定]**（[0x453A90,0x458E38) 線形全走査、`test ...,8` 形のみ）

| gate | VA | label / quiet | 型区画 |
|---|---|---|---|
| `test dl,8` | 0x45468B | Cell Flags(%i) / 'CHANGE_CELL_FLAGS\r\n'@0xA39D10 | type 0x30 |
| `test dl,8` | 0x454FA3 | Faction Reactions(%i)（v312 件2 既載） | type 6 |
| `test dl,8` | 0x458103 | Base Attributes / 'CHANGE_ACTOR_BASE_ATTRIBUTES\r\n'@0xA39314 | 0x23/0x24 区画 |

全 site が `[esp+0x170]`（＝formatter 入口 `call 0x4535a0`@0x453AD0 の返り値）を読む。v312 §3 R1 の列挙（0x45468B/0x458103）に 0x454FA3 を補足。

**(c) formatter は台帳を一切読まない [確定]**
- formatter 帯 [0x453A90,0x458E38) 内の台帳 API（452600/452C90/452CF0/452D10/452D30/452D60/452DF0/453500/453530）E8 caller = **0 件**（4535A0 のみ 1 件 @0x453AD0）。4535A0/453530 本体にも台帳呼出なし。
- → pass2 の台帳 bit3 除去が formatter の bit3 を直接殺す経路は**帯内に存在しない**。

**(d) 4535A0＝表示時 flags ポストプロセッサ（RTTI 名同定・本タスク新知見）**
- 4535A0(form, flags): `RTDynamicCast` で `TESObjectCELL`（TypeDescriptor 0xB03038 = '.?AVTESObjectCELL@@'）判定。
  - **CELL 経路（早帰り 0x45366D/0x45367E）**: 入力 flags を通し（bit3/bit31 保存）、bit24/25/26/28 をライブ状態で合成（`or ebx,0x4000000`@0x453660 / `or ebx,0x2000000`@0x453671）。
  - **非 CELL 経路**: bit3 ← 述語 `call 0x5f0310`@0x45376F → 真なら `or ebx,8`@0x453778 / 假なら `vtable+0x198` getter 経由で偽なら `and ebx,0xfffffff7`@0x45378F。恒常 `and ebx,0x7fffffff`@0x453792（bit31 消去）、位置差分で `or ebx,0x80000000`@0x4538D5 再設置。
- → **formatter の bit3 は台帳 bit3 に依存せず、非 CELL ラベル（Faction Reactions / Base Attributes）は入力 bit3 が無くてもライブ述語で発火し得る [確定]**。CELL ラベル（Cell Flags）は入力パススルー → ファイル側レコードの bit3 存否に依存。

**(e) 台帳 bit3 の出所・生死遷移**
- 挿入は LoadGame のみ: callers(0x452C90)=3（0x452CFA＝452CF0 内部 / 0x46665C / 0x46683D、いずれも LoadGame dispatch）→ **ledger flags（bit3 含む）はファイル・レコード由来 [確定]**。452CF0 の唯一 caller = 0x466829。
- pass2 で除去/翻訳（(a)）[確定]。

**(f) 両論併記＋裁定（pass2 の性格）**: (a) v312 件6/11「FUN_00465130 = save 側」、(b) v289「LoadGame pass2 @0x461680」。新旁証: FUN_00465130 内に 12B レコード列挙子 `call 0x45ad00`@0x465461/0x46560A（LoadGame の [ebp+0x40] リストと同一）＝(b) 方向の旁証。ただしロード後もリストが残存し得るため決定不能 → **裁定保留（v313 帯の上位同定に委ねる）**。

**総合**: 機構（翻訳・ゲート・合成・出所）は帯内で全解明。決定的残件＝①保存ライタの bit3 処理（ファイル内 bit3 の存否＝FUN_00464AB0/FUN_00465130 帯・v313）、②caller1 の 12B レコード出所（FUN_0045ADC0 帯・v313）。→ **[部分確定]**。「ロード済みファイルで formatter bit3 が発火し得ない」は非 CELL 合成経路により **[確定負] 方向**、CELL の Cell Flags 表示は [未確定]（保存ライタ次第）。

---

### R2 — formatter への間接呼出（FF15/FF10 等）未走査 — **[確定負]**

- `find_dword(0x453A90)`（全セクション: .text/.rdata/.data/.tls/.rsrc/.bind 含む）= **0 件** → formatter アドレスを格納したポインタ・vtable スロット・IAT エントリが**一切存在しない**。
- コード内 raw imm32(0x453A90) = **0 件**（`mov reg,imm32` / `push imm32` とも不在）。
- raw `FF 15 <imm32==slot>` / `FF 25 <imm32==slot>` = 0 件（slot 自体が無いため成立せず）。全セクション `E9`（long jmp）→ 0x453A90 = **0 件**。
- → FF15/FF10/FF11-FF17 系の間接呼出は**構造的に成立し得ない**。callers(0x453A90)=2（0x45B029/0x45CD9F・E8）が唯一の呼出形。件6 の負証拠（保存帯 E8=39 中 formatter=0）と合わせ、**間接呼出 [確定負]**。
- 限定: 即値を経由しない理論的な計算経路のみ除外不能（格納器不在のため実質消滅）。

---

### R3 — v289 `<key><flags><refcount>` vs arg3=ebx（453500 alloc）/ 12B +9 u8・+10 u16 — **[確定]**

**(a) FUN_00453500（head: pre8 @0x4534F8 = CC×8、16 アライン、非 CC）**
```
00453500  mov eax,[esp+4]        ; arg1 = size
00453505  push 1 / push eax / mov ecx,0xb02020 / call 0x401e20  ; プール確保（thiscall・pool 0xB02020）
00453514  test eax,eax / mov [esi+0x14],eax   ; esi=ecx=this → [this+0x14] にも保存
00453519  jne ret / push 0xa390d4 / call 0x404ec0 (失敗時エラー)
0045352B  pop esi / ret 4        ; 返り値 = 確保ブロック ptr
```

**(b) entry+4 = tail バッファ ptr（refcount ではない）[確定]**
- FUN_00452CF0: `call 0x452c90` → `mov ecx,[esp+0xc]`（arg3） → `mov [eax+4],ecx`@0x452D03。唯一 caller = 0x466829（`push ebx`@0x466823、ebx = `mov ebx,eax`@0x4667BA ← 453500 結果）。
- no-tail 経路 0x466830: `mov ax,[esp+0x26]`→`test ax,ax; je` → 452C90 のみ（entry+4 は new 時 `mov [eax+4],esi`(0)@0x452CBA で 0）。
- 452DF0 は arg3 を pool 0xB02020 へ free（v289 既載）＝**arg3 は解放可能なブロック＝ポインタ**。452D60/452DF0 の `==0` 検査は「tail 無し」検査として整合。
- → v289 の「refcount」命名 = **[確定負]**（tail buffer ptr）。v312 件4 の留保「453500 の返り値の名前未同定」は本件で解除。

**(c) 12B +9 u8・+10 u16 の対応 [確定]**（LoadGame dispatch 0x466787-0x466829）
- **+10 u16 = tail payload size**: `mov ax,[esp+0x26]`@0x466787 → `test ax,ax; je 0x466830`（size 0＝no-tail 分岐）→ `movzx edx,ax; add edx,4; push edx; call 0x453500`@0x466795-0x4667AF（alloc = size+4）→ `movzx eax,[esp+0x26]`@0x466805 → `push eax` → `call 0x45bb00`@0x466816（payload 長さとして使用。45BB00 は全 7 caller が LoadGame 帯）。
- **+9 u8 = tail frame の typeID byte**: `mov cl,[esp+0x25]`@0x46679C → `mov [esp+0x53],cl`@0x4667A3。ローカル4B フレーム = `<u16 tailSize @+0x50><type char @+0x52><typeID @+0x53>` で、`push 4 / lea / push / call 0x45b9a0`@0x4667D7-0x4667E4 により **v303 仕様の tail frame `<u16 size><char type><typeID>`** と一致（char type は列挙子 45AD00 由来の cl @0x466798）。
- → v289 `<key><flags><refcount>` の対応: **key = FormID（map key [node+0xC]）／flags = entry+0（12B +5 由来）／"refcount" ≡ entry+4 = tail ptr**。12B の +9/+10 は refcount ではなく typeID/size に相当し、3 項目記述と実レイアウトの対応は以上で閉じる。

---

### R4 — `CHANGEFLAG_*` 3 件の正体 — **[確定]**

- .rdata 在庫（全セクション走査）: **`CHANGEFLAG_` = 3 件のみ**（0xA39730 / 0xA39A98 / 0xA39B20）、**`CHANGE_` = 67 件**（v305 の 67 件と一致）。他セクション 0 件。
- 文字列全文（cstr 実測）:
  - 0xA39730 = `'CHANGEFLAG_OBJECT_DROPPED_NON_QUEST_ITEM\r\n'`
  - 0xA39A98 = `'CHANGEFLAG_REFR_HAD_HAVOK_MOVE_FLAG\r\n'`
  - 0xA39B20 = `'CHANGEFLAG_REFR_CELL_CHANGED\r\n'`
- xref（find_dword 全セクション）: 各 **1 件のみ・全て formatter 帯内** — 0x456A40（Dropped Item quiet）/ 0x4556B0（Had Havok Move quiet）/ 0x4552F0（Cell Changed quiet）。imm32 ポインタテーブル不在（v305 の「ポインタテーブル帯なし（負）」と同型）。
- **裁定**: 正体 = **quiet 専用名 [確定]**（使用先＝formatter の quiet パスのみ）。保存系参照（保存レコードを識別する文字列）説は **[確定負]**（保存帯 xref 0・テーブル 0）。67 `CHANGE_*` も v305 既確定で 67/67×1 箇所・単一包含関数 FUN_00453A90 → **両プレフィックスとも formatter quiet 名で双方向は成立**。差は命名系統の揺れのみ（3 ラベル＝Dropped Item Flag / Had Havok Move / Cell Changed が CHANGEFLAG_ を使用）。

---

### R5 — 0x10000 組の %s@0x457718 ↔ Non-saved@0x457853 の sub-dispatch — **[確定]**

**区画 {0x32,0x33} 内の直列ゲート連鎖（帯内全実測）**:

| 順 | gate | VA | skip 先 |
|---|---|---|---|
| 1 | Trespass: `test edx,0x40000`（bit18） | 0x4575A3 | `je 0x4576d3` |
| 2 | **Interrupt/'%s': `test edx,0x20000`（bit17）** | **0x4576D3** | **`je 0x457853`** |
| 3 | Non-saved: `test edx,0x10000`（bit16） | 0x457853 | `je 0x457963` |

- **%s ブロックの実ゲート = bit17@0x4576D3 [確定]**。quiet≠0 なら `sprintf('CHANGE_ACTOR_INTERRUPT_PACKAGE %08X\r\n'@0xA394EC, edi)`@0x457700-0x45770B → `jmp 0x457814`。verbose は 0x457718 へ。
- **0x457718 の `test edx,0x10000` はゲートではなく getter マスク選択 [確定]**: clear → `mov esi,0x20000`@0x45771E → `je 0x45772a` / set → `mov esi,0x30000`@0x457725 → 双方 0x45772A へ合流（スキップ無し）。以降: 既定 '%s' = `'Interrupt'`@0xA394E0 を10B コピー → ebx≠0 なら `call 0x5e4080`@0x45774F/0x45775A でパッケージ名解決 → `vtable+0x4C` getter（mask=esi）の差分 → `sprintf('%s %08X(%i)'@0xA394D4)`@0x457799 → 25B 長さガード → append → fall-through で 0x457853 へ。
- Non-saved: bit16 で skip → quiet = `'CHANGE_ACTOR_NONSAVED_PACKAGE\r\n'`@0xA394B4（0x45787A）/ verbose 0x45788B。
- **裁定**: sub-dispatch = **bit17（Interrupt Package）vs bit16（Non-saved Package）の別 flag で分離 [確定]**。両ブロックは直列（bit17∧bit16 同時立なら両行出力）＝排他構造ではない。v311 row63 の gate 列「flag 0x10000@0x457718」は**誤帰属＝訂正提案**（実ゲート 0x20000@0x4576D3）。row63 の quiet 名（v311「対応外」）は `'CHANGE_ACTOR_INTERRUPT_PACKAGE %08X'`。
- → v312 §3 R5 の「同区画 {32,33}・同 flag 0x10000・formType では分離不能・sub-dispatch 未確証」は **flag 分離で解消 [確定]**（Empty Flag@0x456B33 は type 0x31 区画で既に分離＝件3 どおり）。

---

### R6 — bit7(0x80) の新多義（Owner / Crime Gold）の保存側突合 — **[部分確定]**

**formatter 側（帯内・全実測）[確定]**:

| label | type gate | bit7 gate | getter | quiet |
|---|---|---|---|---|
| Owner(%i) @0x456731 | 区画 0x31（v312 件3） | `test dl,dl`@0x4566D3 → `jns 0x4567e3`@0x4566D5 | `push 0x80`@0x456715（vtable+0x4C） | 'CHANGE_OBJECT_EXTRA_OWNER\r\n'@0xA397C8 |
| Crime Gold(%i) @0x457F4A | `cmp byte[esp+0x174],0x32`@0x457EE1 `jne 0x458e1a`（インライン） | `test dl,dl`@0x457EEF → `jns 0x458e1a`@0x457EF1 | `push 0x80`@0x457F2E（vtable+0x4C） | 'CHANGE_CHARACTER_EXTRA_CRIMEGOLD\r\n'@0xA39370 |

- direct imm 形 `test *,0x80` = formatter 全域 **0 件** → v304「bit7 formatter 全域で未検出」は direct 形限定の負証拠であり、**本体は sign 形（test dl,dl / jns）**（formatter の jns 3 件 = 0x4552C5(bit31 Cell Changed) / 0x4566D5 / 0x457EF1、v312 probe15 と一致）。
- → **bit7 新多義:0x31 Owner ↔ 0x32 Crime Gold は formType 区画（＋Crime Gold はインライン型ゲート）で分離 [確定]**（件3 の 12/13 組分離と同型）。**v311 §5-4 の多義リストに 0x80 の追記要**。v311 row46 Owner「ゲート未取得」→ **ゲート取得**（判定は保存側未突合で [部分確定] 維持を提案）。
- getter mask 0x80 = changeFlags bit 要求 [推定]（%s の 0x20000/0x30000・Quest Script の 0x8000000 と同一パターン。vtable+0x4C の関数名はクラス依存 vtable のため未同定）。

**保存側 [部分確定]**: raw `test *,0x80`（F6/F7 /0 形）走査 = **0 件** — pass2 [0x460000,0x464000) / LoadGame dispatch [0x466000,0x466AC6) / [0x463000,0x464000) / helpers [0x452000,0x453A90) の全帯。残る保存側候補（459AF0 読取ループ・保存ライタ）は **v313 帯で未突合**（新規残点 N7）。

---

### R6' — v311 row30 の S8「0x2000000 適用」引用と formatter 帰属の artifact 側再突合 — **[確定]**

- **artifact 再検証（%TEMP%\opencode\v282_9_out.txt＝FUN_00459AF0 保存中間物・読取のみ）**: 要求 bit 分岐を実バイトで確認 —
  - `test al,4`@0x459C0E → 12B 読み → `call 0x459950`@0x459C34
  - `test eax,0x4000000`@0x459C82 → 4B 読み → `call 0x459990`@0x459C9D
  - **`test eax,0x2000000`@0x459CBE → `je 0x459ea5` / 6B 読み（u32＋u16@+4）→ `call 0x459990`@0x459CE6**
  - → v282 件2-1 L68 の「要求 bit{2,4,0x2000000,0x4000000} で 459950/459990 適用」は **artifact と一致 [確定]**（S8 記載自体は正しい）。
- **性質裁定 [確定]**: その 0x2000000 分岐は 459AF0 一般ループの**ペイロード整形分岐（ビットごとに読取長が異なる）**であり、Oblivion Flag 固有の帰属を示す証拠ではない。
- **formatter 側の帰属 [確定]**: `test *,0x2000000` = formatter 全域 **1 サイト 0x4557AA = ANIMATION ゲート**（v312 件8c）→ row30 の「0x2000000 は他ラベル無し（本ラベルに一意）」= **FALSE**。`test *,0x800000` = formatter 全域 **1 サイト 0x455773 = Oblivion Flag** → 0x800000 の一意は維持。
- **裁定**: v312 の「row30 から 0x2000000@0x4557AA 除去」を維持し、**S8 引用は「保存側に 0x2000000 分岐が存在する」一般事実として読み替える**。row30 の [確定] は S8 腿が欠落するため **[部分確定] への格上げを提案**（残根拠: 0x800000@0x455773 一意＋type 0xE 複合 `test dl,0xE`@0x455814＋S4 bit23）。459950/459990 の適用対象の最終ラベル帰属＝v313 帯（新規残点 N6）。

---

### R7 — formatter 入口の arg3 / call 0x4535a0 の名前・意味、arg4=formType・arg5=quiet — **[部分確定]**

**callee 側 [確定]（プロローグ実測 0x453A90: pre8 = `…c2 04 00`＋CC×4、16 アライン、非 CC）**
- `sub esp,0x154`@0x453A90 → cookie → `push ebx; push ebp` → `mov ebp,[esp+0x164]`@0x453AA6 = **arg2 = form** → `push esi; push edi` → `mov edi,[esp+0x168]` = **arg1 = 出力 buf**（`mov byte [edi],0`@0x453AC2 初期化）。
- `mov eax,[esp+0x170]`@0x453AC7 = **arg3 = changeFlags** → `push eax; push ebp; call 0x4535a0`@0x453AD0 → `mov [esp+0x170],eax`@0x453AD5（**arg3 スロットへ書戻し＝以降の全 flag ゲートが読む値**）。
- `[esp+0x174]` = **arg4 = formType byte**、`[esp+0x178]` = **arg5 = quiet flag**（`cmp byte[esp+0x178],0`@0x453ADC、`mov al,[esp+0x174]`@0x453F2B 等 4 参照 — push 群後 4 本のスタック計算で確定）。ecx = this = SLG（`mov ebx,ecx`@0x453AB8、v304 既確定の両 caller の `mov ecx,[SLG]` と整合）。
- **4535A0 の名前・意味 = (form, flags) → 表示用 changeFlags ポストプロセッサ**（CELL/非 CELL 分岐・bit 合成/消去、`ret 8`＝2 引数）[確定]（R1-(d) の命令列）。5 つの caller（0x453AD0 / 0x45CD2C / 0x45F1E3 / 0x463B72 / 0x465494）は v306 既確定の「flags 検査 → 0 なら警告/スキップ」型と矛盾しない（0x463B72 の文脈を帯内で再確認: `'Created form…'` 系文字列 0xA3B0D4/0xA3B0B8・`mov cl,[esi+0x7c]`（SLG version byte）・`mov al,[edi+4]`（formType））。
- **caller 側（0x45B029/0x45CD9F の push 群）は v313 帯のため本帯では再測不可** → v304（ecx=[SLG] 先行・node+4 の型 ID byte push）/ v312 件4（`byte[ebx+4]`=formType・`[ebx+5]`=flags 渡し）の既確定を引用（矛盾なし）。
- → 総合 **[部分確定]**（callee 全確定・caller 側は帯外のため既確定引用のまま）。

---

### R8 — pass2 の ebp（formType）の初出ロード地点 — **[確定]**

- **ebp（formType 用途）の初出 = [esp+0x34] への 2 経路 [確定]**:
  - **経路 A（entry+4＝tail ptr ≠ 0）**: `mov eax,[eax+4]`@0x461A7B → `mov [ebx+0x14],eax`@0x461A9C（cursor := tail）→ 4B 読み `mov [esp+0x30],edx`@0x461AAD → **`movzx eax,byte[esp+0x32]`@0x461AB4（tail frame の char type）** → `cmp eax,0x31/0x32/0x33` → **`mov [esp+0x34],eax`@0x461ABC**（不一致は `jne 0x461bc3` で記録スキップ）。
  - **経路 B（entry+4 == 0）**: `je 0x461bcc`@0x461A96 → `call 0x46b250`@0x461BD1（汎用 form 解決・callers 483）→ **`movzx ecx,byte[eax+4]`@0x461BE1（TESForm::formType）** → `cmp ecx,0x31/0x32/0x33` → **`mov [esp+0x34],ecx`@0x461BE8**。
- `mov ebp,[esp+0x34]`@0x461DEC → `cmp ebp,0x32`@0x461DF0 / `cmp ebp,0x33`@0x461DF5 → 0x461E5A → **`cmp ebp,0x31`@0x461E62 → `test ebx,0x20000`@0x461E67**（Dropped Item force 除去）。
- **v312 R8 evidence の「[esp+0x34] ← 0x461D77」は push オフセット誤り＝訂正**: 0x461D77 実行時は 2 push 済み（`push 4`@0x461D6A / `push ecx`@0x461D70）→ `mov [esp+0x34],ebp` は実際には **[esp+0x2C]** へ書込。0x461D6C の `lea ecx,[esp+0x30]`（1 push 後）も同じ [esp+0x2C] を指し、**append バッファへ ebp（＝find キー）を格納する同一処理**。[esp+0x34] の書手は 0x461ABC / 0x461BE8 のみ。
- 他の ebp ロード（0x461AF5/0x461B47/0x461B76＝位置ブロック +0x10、0x461BC0＝テーブル、0x461C1C/0x461C2C＝[x+0xC] FormID、0x461A63/0x461BB3＝xor 0）は find キー用途で [esp+0x34] を汚染しない（レジスタのみ・ストア無し）。
- 比較定数 0x31/0x32/0x33 = tail char／formType と一致（formatter 区画 {31,32,33} と同一ビット空間）[確定]。v312 R8 の「比較定数が formatter 区画と一致」は維持、出所のみ確定。

---

### R9 — Warnings.txt oracle（件12）の実機検証 — **[未確定]（実機待ち）**

- v312 件12 と同様、実機 Warnings.txt 生成待ち。本タスク帯内に B 型ゲートの実機裏付けとなり得る**新規の静的証拠はなし**（formatter 側の bit31/bit16/bit17/bit7 ゲートはすべて R1/R5/R6 で機械側＝静的側として処理済み）。
- [未確定] 維持。静的解析では結論を出せない唯一の項目（v312 件12 の記述を維持）。

---

## 2. 判定サマリ

### 2.1 10 項の判定一覧

| # | 残点 | 判定 | 要点（VA） |
|---|---|---|---|
| R1 | pass2 bit3 翻訳 vs formatter bit3 発火緊張 | **[部分確定]** | 翻訳 0x461CB3-D7 再確認、formatter は台帳非依存（帯内 caller 0）＋非 CELL は 0x453778 で bit3 合成、残件は保存ライタ/caller1（v313 帯） |
| R2 | 間接呼出 FF15/FF10 | **[確定負]** | 0x453A90 のデータ格納 0・imm32 0・FF15/FF25 0・E9 0 → 器が不在 |
| R3 | refcount vs tailptr / 12B +9・+10 | **[確定]** | 453500=pool alloc、entry+4=tail ptr（free 対象）、+9=typeID・+10=tailSize |
| R4 | CHANGEFLAG_* 3 件の正体 | **[確定]** | .rdata 3 件・xref 各 1 で全て formatter quiet、保存系参照 0 |
| R5 | 0x10000 組 %s↔Non-saved sub-dispatch | **[確定]** | 実ゲートは bit17@0x4576D3 vs bit16@0x457853、0x457718 はマスク選択 |
| R6 | bit7(0x80) 新多義の保存側突合 | **[部分確定]** | Owner 0x4566D3/Crime Gold 0x457EEF の sign ゲート＋getter 0x80＝formType 分離、調査可能帯の保存側 test 0x80=0、残り v313 帯 |
| R6' | S8「0x2000000 適用」artifact 再突合 | **[確定]** | v282_9 artifact と一致＝一般整形分岐、0x2000000 は ANIMATION 専用 → row30 の一意主張 FALSE |
| R7 | arg3/4535a0/arg4/arg5 | **[部分確定]** | callee 5 引数マッピング確定（arg1 buf/arg2 form/arg3 flags in-out/arg4 formType/arg5 quiet）、caller 側は v313 帯 |
| R8 | pass2 ebp 初出ロード | **[確定]** | [esp+0x34] ← tail char 0x461ABC / formType 0x461BE8 の 2 経路、0x461D77 は [esp+0x2C]（push 数え訂正） |
| R9 | Warnings.txt oracle | **[未確定]** | 実機待ち（新証拠なし） |

### 2.2 5 段階集計（10 項）

| 判定 | 件数 | 項番 |
|---|---|---|
| [確定] | **5** | R3, R4, R5, R6', R8 |
| [部分確定] | 3 | R1, R6, R7 |
| [推定] | 0 | —（R6 内の getter mask 解釈のみ個別 [推定]） |
| [未確定] | 1 | R9 |
| [確定負] | 1 | R2 |

### 2.3 旧結論への裁定更新案（本タスクで提案する差分）

1. **v289「refcount」** → **entry+4 = tail buffer ptr [確定]（命名訂正・refcount 説 [確定負]）**。v312 件4 の留保（「453500 返り値の名前未同定」）を解除。
2. **v311 row63 ゲート列** `flag 0x10000@0x457718` → **`0x20000@0x4576D3`（bit17）**に訂正。0x457718 は getter マスク選択（0x20000/0x30000）。quiet 名を `'CHANGE_ACTOR_INTERRUPT_PACKAGE %08X'@0xA394EC` で追記（v311「対応外」の補完）。
3. **v311 row30**: S8 引用を「一般ペイロード整形分岐」へ読み替え＋「0x2000000 一意」主張を訂正（ANIMATION 専用・formatter 1 サイト）。**row30 の [確定] → [部分確定] 格上げ案**（0x800000 一意＋type 0xE＋S4 bit23 は維持）。
4. **v311 row46 Owner**: 「ゲート未取得」→ **`test dl,dl; jns`@0x4566D3 取得**（getter `push 0x80`@0x456715 併記。判定は [部分確定] 維持）。
5. **v304「bit7(0x80) は formatter 全域で未検出」** → direct imm 形限定の負証拠と訂正。**sign 形 2 件（0x4566D3/0x457EEF）で formatter 側検出済み**（保存側は未突合）。
6. **v312 R8 evidence「[esp+0x34] ← 0x461D77」** → push オフセット誤りと訂正（0x461D77 は [esp+0x2C]。[esp+0x34] の書手は 0x461ABC/0x461BE8）。
7. **v312 R1 の formatter bit3 ゲート列挙（2 件）** → **3 件**（0x454FA3 Faction Reactions を補足）。
8. **pass2 の性格（v312「SaveGame」 vs v289「LoadGame pass2」）** → 両論併記・**裁定保留**（新旁証: FUN_00465130 内の 45AD00 リスト共有@0x465461/0x46560A。上位同定は v313 帯）。
9. **v311 §5-4 多義リスト** → **0x80(Owner/Crime Gold) の追記要**（formType 0x31/0x32 で分離、件3 と同型）。
10. **v311 row70 Crime Gold**: 型ゲート `cmp byte[esp+0x174],0x32`@0x457EE1 を gate 列に追記可能（bit7 gate 0x457EEF と合わせ二段ゲートで確定）。

---

## 3. 新規残点（次束候補）

| # | 残点 | 帯 / 根拠 |
|---|---|---|
| N1 | 保存ライタ（FUN_00464AB0 / FUN_00465130 帯）の bit3 書込み・除去 — **ファイル内 bit3 の存否**（R1 の決定的残件、CELL の Cell Flags 表示の可否に直結） | v313 帯 |
| N2 | caller1/caller2（FUN_0045ADC0 / FUN_0045CC60）の 12B レコード出所（SLG+0x40 リストとの同否） | v313 帯 |
| N3 | FUN_00465130 の save/load 同定（v312「SaveGame」 vs v289「LoadGame pass2」の仲裁、45AD00/45BB00 caller 分布を含む） | v313 帯＋帯内 caller 一覧は取得済 |
| N4 | 45BB30（pass2 再発行後の `call 0x45bb30`@0x461DE7・(form,0)）と 45BB00 本体の同定 | 0x45B000-0x45C000 = v313 帯 |
| N5 | 5F0310 述語の名前（`[form+0x3c]`/`[form+0xb0]∈{1,2,6,4}`/RTTI 0xB14914・0xB14930）と vtable+0x4C getter の名前 | 0x5F0310 先頭のみ帯外読取・深部未走査 |
| N6 | 459950/459990 の適用対象の最終ラベル帰属（R6' の残余） | 459AF0 深部 = v313 帯 |
| N7 | 保存側 bit7 分岐の有無（459AF0 読取ループ・保存ライタ）＝R6 の残余 | v313 帯 |
| N8 | pass2 再発行レコードの完全バイトレイアウト（帯内＝ヘッダ4B＋key4B＋旧 tail 追加のみ判明、45B9A0 本体は v313 帯） | 0x45B9A0 = v313 帯 |
| N9 | Trespass 以前の型ゲートチェーン（区画 {32,33} 入口の詳細 — formatter 帯内の minor） | 0x4574xx-0x457550（次束で走査可） |

---

## 4. selftest 再現メモ

### 4.1 実行形式・中間物

```powershell
chcp 65001; python $env:TEMP\opencode\v315_<n>.py *> $env:TEMP\opencode\v315_<n>.out
```

- 中間物: `%TEMP%\opencode\v315_01..07.py` ＋ `.out`（7+7 = 14 ファイル・本タスクのみ新規）。
- 全 probe は `tmp/re/re_util.py`（変更なし）を sys.path 経由で再利用し capstone で read-only disasm。raw E8 判定 = `va+5+rel==target`、VA→file = `pe.get_offset_from_rva(VA-0x400000)`。
- 関数先頭は教訓訓②どおり検証（pre8 CC run ＋ 16 アライン ＋ 非 CC）: 0x453500 / 0x453530 / 0x4535A0 / 0x453A90 / 0x452600 / 0x452C90 / 0x461680 / 0x463A90 / 0x5F0310 すべて充足。教訓③（raw 検索は命令境界検証）を raw test/F6-F7 走査・FF15-E9 走査で適用（境界検証失敗は報告せず除外）。教訓④（formatter 内ジャンプテーブル 0x458E38-0x458E4C はデータ）で走査上限を 0x458E38 に設定。

### 4.2 probe 一覧と主結果

| probe | 対応項 | 主結果 |
|---|---|---|
| 01 | R1 | 4535A0 head＋前半、formatter プロローグ、pass2 0x461C90-D70・0x461A50-AC0・0x461E00 段、formatter `test *,8`=**3 件**、formatter 帯台帳 API caller=**0**、callers(0x4535A0)=**5** |
| 02 | R1/R7/R8 | 453530 本体（446b80 ゲート＋RTTI 別 flags 調整）、formatter プロローグ引数対応、pass2 0x461ABC-C90（tail char 経路）・0x461DD0-E90（Dropped Item 段）、pass2 ebp 触点 20 行、0x463B72 文脈、quiet 文字列 |
| 03 | R1/R2/R4/R6 | 4535A0 後半（`or ebx,8`@0x453778 等）、slot(0x453A90)=**0**、imm32=**0**、CHANGEFLAG_=**3**/CHANGE_=**67**、xref=0x456A40/0x4556B0/0x4552F0、452C90 callers=**3**、pass2 帯 test 0x80=**0** |
| 04 | R3 | 453500/452C90/452CF0 本体、LoadGame 12B パース 0x466700-870、452C90@0x46665C 文脈、+9/+10 参照 7 件 |
| 05 | R1/R5/R6 | %s/Non-saved 両ブロック、Owner/Crime Gold ブロック、4 帯 test 0x80=**0**、RTTI 名 8 点、5F0310 先頭 |
| 06 | R5 | %s 入口域 0x4575E0-6F6（bit17 ゲート発見）、jcc ターゲット走査 21 件、ラベル/quiet 文字列 12 点、0x457853 入口 = 0x4576D9 のみ |
| 07 | R5/R6'/R2 | `test *,0x800000`=**1（0x455773）**／`test *,0x2000000`=**1（0x4557AA）**、Trespass ゲート bit18@0x4575A3＋quiet、45AD00=6/45BB00=7（全 LoadGame 帯）/45BB30=3、E9→0x453A90=**0**、452600 head＋本体 |

### 4.3 主要数値の自己照合（再現時の一致確認ポイント）

- slot(0x453A90)=0 / code imm32(0x453A90)=0 / FF15,FF25=0 / E9=0 / callers(0x453A90)=2（既確定）
- formatter: `test *,8`=3（0x45468B/0x454FA3/0x458103）、`test *,0x800000`=1、`test *,0x2000000`=1、`test *,0x80`=0、jns=3（0x4552C5/0x4566D5/0x457EF1）
- CHANGEFLAG_=3（xref 各 1・全 formatter 内）/ CHANGE_=67
- callers(0x452C90)=3（0x452CFA/0x46665C/0x46683D）/ 452600=105 / 452D60=3（0x461DD3/0x463A6E/0x466766）/ 452D10=1 / 452D30=3 / 452230=3 / 4535A0=5 / 45AD00=6 / 45BB00=7 / 45BB30=3
- `test *,0x80` raw = 0 件 × 4 帯（0x460000-0x464000 / 0x466000-0x466AC6 / 0x463000-0x464000 / 0x452000-0x453A90）
- bit3 合成/消去: `or ebx,8`@0x453778 / `and ebx,0xfffffff7`@0x45378F / `and ebx,0x7fffffff`@0x453792 / `or ebx,0x80000000`@0x4538D5
- R5 ゲート: bit18@0x4575A3 / bit17@0x4576D3 / bit16@0x457853 / mask 選択@0x457718
- R8: [esp+0x34] 書手 = 0x461ABC（tail char）/ 0x461BE8（formType）のみ、0x461D77 は [esp+0x2C]
- R6': artifact 内 `test al,4`@0x459C0E / `test eax,0x4000000`@0x459C82 / `test eax,0x2000000`@0x459CBE → call 459950@0x459C34 / 459990@0x459C7B,0x459C9D,0x459CE6
- RTTI: 0xB02F9C='.?AVTESForm@@' / 0xB03038='.?AVTESObjectCELL@@' / 0xB03054='.?AVTESObjectREFR@@' / 0xB03070='.?AVActor@@' / 0xB05538='.?AVMobileObject@@' / 0xB05468='.?AVTESQuest@@' / 0xB058E8='.?AVDialoguePackage@@' / 0xB03E00='.?AVTESPackage@@'

---

## 5. 遵守宣言

- EXE: read-only 解析のみ（disasm 実施帯 = 0x452000-0x459000 / 0x460000-0x464000 / formatter 0x453A90-0x458E38。帯外は raw 走査・参照読取のみで v313/v314 帯の disasm は**一切なし**）。ESM: 未接触。git: 変更系コマンド未実行。
- 編集: **本ファイルのみ新規作成**。既存 tmp/re 成果物・台帳・STATE・TODO・docs・GT・ソース（app/**）・.github/** は読み取りのみ（v311/v312/v304/v289 は無編集、訂正は本ファイルの「裁定更新案」で提示）。
- 中間物: `%TEMP%\opencode\v315_*` のみ新規（14 ファイル）。`tmp/re/re_util.py` 変更なし。
- 判定整合: 10 項 = [確定] 5（R3/R4/R5/R6'/R8）＋ [部分確定] 3（R1/R6/R7）＋ [未確定] 1（R9）＋ [確定負] 1（R2）＝ 10（一致）。裁定更新案 10 項・新規残点 9 項（N1-N9）。

## 追補（次束G＝v317 統合時・2026-11-07）

`v317_v315_residuals.md`（52,471B/421行・probe 12 本）により本ファイルの新規残点 **N1-N9 は全件裁定**された（[確定]7＝N1/N2/N3/N6/N7/N8/N9、[部分確定]2＝N4/N5、主[未確定]0）:

1. **R1(f) の保留は解消**: `FUN_00465130` ＝ **保存オーケストレータ**（`this=[0xB33B00]` を quicksave 呼び手 `0x672F36` で実証・open(mode0)→ヘッダ→12B 書出）。**pass2 の唯一 caller ＝ `0x4651C3` → pass2 は保存直前実行**であり、v315 R1(f) が傍証不成立とした 45AD00 はロード/保存双方向使用のため裁定に必要なかった（仲裁＝N3）。
2. **bit3 の実在面（N1）**: 保存ライタに bit3 直接操作は 0 件。ファイル bit3 は (A) tail≠0 なら pass2 後 entry flags 直書き（pass2 が bit3∧bit1|2 例をスキップした場合のみ残存）／(B) tail==0 なら `4535A0` 経由（CELL は入力パススルー・非 CELL は `5F0310` 述語で再合成）の **2 経路のみ**＝**bit3 単独 entry は pass2 で削除され非書出** [確定]。
3. **保存側 bit7（N7）**: 残帯全走査で保存側 changeFlags の bit7 分岐は **0 件 [確定]**。実在は formatter 第 3 ゲート `test dl,dl / jns` @0x458771＝**`CHANGE_ACTOR_BASE_FULLNAME`（type 0x23/0x24）を新発見**（formatter jns は 3→4 件へ訂正）と SaveGame+0x18 bit7 の一時ラッチ @0x463966-0x4639A9。
4. **4535A0 の bit16 ライブ合成を補足**（v315 R5 関連・詳細は v317 §5 訂正提案）。
5. N2 caller1=`[SaveGame+0x40]` の 12B リスト（quiet=0）／caller2=SaveGame+0 マップ value[0]（quiet=1）＝R7 caller 側 [確定] 格上げ、N6＝`[+0x74]/[+0x78]` テーブルのペイロード参照 ID 解決ヘルパ（R6' 追認）、N8＝新 tail `<u16 size'(旧−Δ)><char><typeID>`+【bit23 のみ 4B key】+旧 payload[Δ:]（Δ=bit31 有 0x2C/無 0x1C）で alloc と数式一致、N9＝区画入口 型 gate 2 源 → bit29→22→21→20→19→Trespass に加え **`0x31` 区画 bit18 @0x456EEB = `CHANGE_DOOR_OPEN_DEFAULT_STATE`（bit18 二義）を新知見**。
6. **動的不足（実 .ess 待ち）**: N8 の Δ(0x1C/0x2C) 中身＝実 .ess 待ちのため動的照合未実施 [未確定]（v317 §7）。新規残件 11 → #85。
