# v305 — REF change state RE 静的解析束（件A: CHANGE_* 棚卸し / 件B: .ess ヘッダ経路 TES4SAVEGAME）

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000）
- 日付: 2026-11-06 / 担当: 静的解析サブエージェント（ai-todo #65 の独立部分・v305 のみ）
- ツール: python 3.11 + capstone 5.0.7（`from capstone import Cs, CS_ARCH_X86, CS_MODE_32`）
- セクション（PE ヘッダ実測で契約値と一致確認済み）: `.text` VA 0x401000→file 0x400 (size 0x626E00) / `.rdata` VA 0xA28000→file 0x627200 (size 0xD9A00) / `.data` VA 0xB02000→file 0x700C00 (size 0x30C00)。変換 `file = va − VA + fileOff`。
- **方法注（親の教訓「+0x400 漏れ」が実際に発生）**: 初期スクリプト（v305_6〜14）は blob スライス後に絶対 file offset でインデックスし、disasm 表示が真の VA+0x400 のバイトになっていた。v305_15 以降は絶対 file offset 基準（`F(va)=0x400+(va-0x401000)`）で統一し、**関数先頭は「直前が ≥3 バイト（実務上 4 バイト）の 0xCC ラン、かつ VA が 16 アライン、かつその位置のバイトが CC でない」**規則＋関数先頭からの線形 disasm（capstone）で命令境界を検証した結果のみを以下に採用した。raw byte-pattern スキャン（v305_23）のヒットは境界検証で棄却されたものを明示して除外する。
- imm32 検索は整列に依存せずファイル全体の全バイト走査（`.text` 内は命令境界検証を併用）。

---

## 1. 件A: 'CHANGE_*' 文字列の全数棚卸しと xref

### 1.1 総論

- 検出は「.rdata/.data でバイト列 `CHANGE_` を含み NUL で終わる文字列を NUL まで延伸」して全 **67 件**。すべて `.rdata` の **0xA3914C〜0xA39E6C** の連続帯にあり、終端子は **`0D 0A 00`（"\r\n\0"）**。契約の正規表現 `rb"CHANGE_[A-Za-z0-9_]{2,40}\x00"` の想定（素の `\x00` 終端）と実体が異なる（CR LF が混入）。3 件は `%08X` 付き（パッケージ系ログ整形）。
- 各文字列への `.text` 内 imm32 参照（`struct.pack("<I", addr)` 全バイト検索）は**全 67 件が各 1 箇所のみ**、計 67 サイト。参照範囲 0x453E1F〜0x458D40。
- **全 67 サイトの包含関数は単一: `FUN_00453A90`**（先頭 0x453A90 = 16 アライン + 直前 4×CC 検証済、次関数先頭 0x458E50。関数長 0x53C0）。頭部は `add eax,1 / cmp eax,4 / jmp dword ptr [eax*4+0x458E38]`（0x453B6A〜0x453B6F）のジャンプテーブル分岐で、67 の変更タイプ名のいずれかを出力バッファへコピーまたは printf 系ログに出す巨大な if/switch の直列チェーン。ジャンプテーブル本体は 0x458E38〜0x458E48。
- 呼び出し元（E8 直接）: `0x45B029`（FUN_0045ADC0 のうち 0x45B029）、`0x45CD9F`（FUN_0045C9C0）。→ 変更レダump/ログ出力経路から呼ばれる。[確定]
- 参照フォームの内訳（命令アライン検証済）:
  - `mov esi, imm32` + `rep movsd/movsw/movsb`（可変長 memcpy 型、名前文字列の完全コピー）: **48 件**
  - `mov ecx, dword ptr [imm32]` + `mov dword ptr [eax], ecx`（先頭 4 dword `'CHAN'`(0x4E414843) の即込み dword コピー、残りは直後の dword 読込命令）: **16 件**
  - `push imm32` + `call 0x98208B`（printf 系ログ、`%08X` 付き 3 件）: **3 件**

### 1.2 全 67 件一覧（文字列 VA 昇順）

| # | 文字列 VA | 文字列（`\r\n` = 0D0A 終端） | 参照 site VA | フォーム | 関数先頭 VA | 推定役割 |
|---|---|---|---|---|---|---|
| 1 | 0xa3914c | `CHANGE_CREATURE_COMBATSTYLE\r\n` | 0x458d40 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 2 | 0xa3916c | `CHANGE_CREATURE_SKILLS\r\n` | 0x458c24 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 3 | 0xa39194 | `CHANGE_NPC_FAME\r\n` | 0x458afc | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 4 | 0xa391bc | `CHANGE_NPC_COMBATSTYLE\r\n` | 0x4589f0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 5 | 0xa391e4 | `CHANGE_NPC_SKILLS\r\n` | 0x4588bc | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 6 | 0xa391f8 | `CHANGE_ACTOR_BASE_FULLNAME\r\n` | 0x458798 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 7 | 0xa39224 | `CHANGE_ACTOR_BASE_AIDATA\r\n` | 0x458680 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 8 | 0xa39254 | `CHANGE_ACTOR_BASE_MODIFIERS\r\n` | 0x458570 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 9 | 0xa39284 | `CHANGE_ACTOR_BASE_FACTIONS\r\n` | 0x458460 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 10 | 0xa392b4 | `CHANGE_ACTOR_BASE_SPELLLIST\r\n` | 0x458350 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 11 | 0xa392e4 | `CHANGE_ACTOR_BASE_DATA\r\n` | 0x458243 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 12 | 0xa39314 | `CHANGE_ACTOR_BASE_ATTRIBUTES\r\n` | 0x458130 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 13 | 0xa39344 | `CHANGE_ACTOR_BASE_HEALTH\r\n` | 0x458020 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 14 | 0xa39370 | `CHANGE_CHARACTER_EXTRA_CRIMEGOLD\r\n` | 0x457f16 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 15 | 0xa393a4 | `CHANGE_ACTOR_EXTRA_PERSUASION\r\n` | 0x457e00 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 16 | 0xa393d4 | `CHANGE_ACTOR_LIFESTATE\r\n` | 0x457ce0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 17 | 0xa39404 | `CHANGE_ACTOR_EXTRA_INVESTMENTGOLD\r\n` | 0x457bd0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 18 | 0xa3943c | `CHANGE_ACTOR_EXTRA_OBLIVION_ENTRY\r\n` | 0x457ab6 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 19 | 0xa39474 | `CHANGE_ACTOR_DISPOSITION_MODIFIERS\r\n` | 0x457992 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 20 | 0xa394b4 | `CHANGE_ACTOR_NONSAVED_PACKAGE\r\n` | 0x457880 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 21 | 0xa394ec | `CHANGE_ACTOR_INTERRUPT_PACKAGE %08X\r\n` | 0x457706 | push imm(→call 0x98208B) | FUN_00453A90 | 書式文字列ログ [:確定:形式]:[推定:用途] |
| 22 | 0xa39528 | `CHANGE_ACTOR_EXTRA_TRESPASS_PACKAGE %08X\r\n` | 0x4575e1 | push imm(→call 0x98208B) | FUN_00453A90 | 書式文字列ログ [:確定:形式]:[推定:用途] |
| 23 | 0xa39568 | `CHANGE_ACTOR_RUNONCE_PACKAGE %08X\r\n` | 0x4574ac | push imm(→call 0x98208B) | FUN_00453A90 | 書式文字列ログ [:確定:形式]:[推定:用途] |
| 24 | 0xa395a0 | `CHANGE_ACTOR_MAGIC_MODIFIERS\r\n` | 0x457370 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 25 | 0xa395d8 | `CHANGE_ACTOR_SCRIPT_MODIFIERS\r\n` | 0x457256 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 26 | 0xa3960c | `CHANGE_ACTOR_GAME_MODIFIERS\r\n` | 0x457132 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 27 | 0xa3963c | `CHANGE_ACTOR_EQUIPMENT\r\n` | 0x457020 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 28 | 0xa39670 | `CHANGE_DOOR_OPEN_DEFAULT_STATE\r\n` | 0x456f16 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 29 | 0xa396a4 | `CHANGE_DOOR_OPEN_STATE\r\n` | 0x456df2 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 30 | 0xa396d0 | `CHANGE_DOOR_EXTRA_TELEPORT\r\n` | 0x456cd5 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 31 | 0xa39700 | `CHANGE_OBJECT_EMPTY\r\n` | 0x456b5c | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 32 | 0xa3976c | `CHANGE_OBJECT_EXTRA_RANK\r\n` | 0x456920 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 33 | 0xa3979c | `CHANGE_OBJECT_EXTRA_GLOBAL\r\n` | 0x456810 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 34 | 0xa397c8 | `CHANGE_OBJECT_EXTRA_OWNER\r\n` | 0x456700 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 35 | 0xa397f0 | `CHANGE_OBJECT_EXTRA_LOCK\r\n` | 0x4565f0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 36 | 0xa3981c | `CHANGE_OBJECT_EXTRA_MAGIC\r\n` | 0x4564e0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 37 | 0xa39850 | `CHANGE_OBJECT_EXTRA_FURNITURE_MARKER\r\n` | 0x4563c0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 38 | 0xa39890 | `CHANGE_MAPMARKER_EXTRA_FLAGS\r\n` | 0x4562a7 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 39 | 0xa398c4 | `CHANGE_REFR_EXTRA_SAVEDMOVEMENTDATA\r\n` | 0x456180 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 40 | 0xa398f8 | `CHANGE_REFR_EXTRA_SCRIPT\r\n` | 0x456060 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 41 | 0xa39924 | `CHANGE_REFR_INVENTORY\r\n` | 0x455f1c | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 42 | 0xa39988 | `CHANGE_REFR_EXTRA_LEVELED_CREATURE\r\n` | 0x455cb3 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 43 | 0xa399b0 | `CHANGE_REFR_DISABLE_STATE\r\n` | 0x455c78 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途]（0x455C72 帯・既知の 'Disabled(%i)' 隣接帯を踏襲） |
| 44 | 0xa399dc | `CHANGE_REFR_ALL_EXTRA\r\n` | 0x455b3c | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 45 | 0xa39a00 | `CHANGE_REFR_SCALE\r\n` | 0x455a0f | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 46 | 0xa39a4c | `CHANGE_REFR_ANIMATION\r\n` | 0x4557dc | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 47 | 0xa39a64 | `CHANGE_REFR_OBLIVION_FLAG\r\n` | 0x4557a0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 48 | 0xa39ad0 | `CHANGE_REFR_HAVOK_MOVE\r\n` | 0x455553 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 49 | 0xa39af8 | `CHANGE_REFR_MOVE\r\n` | 0x45540c | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 50 | 0xa39b4c | `CHANGE_CREATED_NEW_REFERENCE\r\n` | 0x4551e0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 51 | 0xa39b7c | `CHANGE_BOOK_TEACHES_SKILL\r\n` | 0x4550d5 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 52 | 0xa39bb0 | `CHANGE_FACTION_REACTIONS\r\n` | 0x454fd0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 53 | 0xa39be0 | `CHANGE_FACTION_FLAGS\r\n` | 0x454eb3 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 54 | 0xa39c10 | `CHANGE_CELL_PATHGRID_MODIFIED\r\n` | 0x454db0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 55 | 0xa39c40 | `CHANGE_CELL_FULLNAME\r\n` | 0x454ca0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 56 | 0xa39c68 | `CHANGE_CELL_OWNERSHIP\r\n` | 0x454b5f | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 57 | 0xa39c90 | `CHANGE_CELL_DETACHTIME\r\n` | 0x454a42 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 58 | 0xa39cbc | `CHANGE_CELL_SEENDATA\r\n` | 0x454920 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 59 | 0xa39ce8 | `CHANGE_CELL_CREATED\r\n` | 0x4547e4 | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 60 | 0xa39d10 | `CHANGE_CELL_FLAGS\r\n` | 0x4546af | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 61 | 0xa39d38 | `CHANGE_PACKAGE_WAITING\r\n` | 0x4545b0 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 62 | 0xa39d68 | `CHANGE_PACKAGE_NEVER_RUN\r\n` | 0x454498 | mov esi,imm(→rep movs) | FUN_00453A90 | 名前コピー [:確定:形式]:[推定:用途] |
| 63 | 0xa39d98 | `CHANGE_TOPIC_SAIDONCE\r\n` | 0x454362 | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 64 | 0xa39df0 | `CHANGE_QUEST_SCRIPT\r\n` | 0x4541f2 | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 65 | 0xa39e1c | `CHANGE_QUEST_STAGES\r\n` | 0x4540b2 | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 66 | 0xa39e44 | `CHANGE_QUEST_FLAGS\r\n` | 0x453f6e | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |
| 67 | 0xa39e6c | `CHANGE_FORM_FLAGS\r\n` | 0x453e1f | mov ecx,[imm](→mov [eax]) | FUN_00453A90 | 名前コピー(dword inline) [:確定:形式]:[推定:用途] |

（表の「参照 site VA」は imm32 フィールドの出現位置（全バイト走査基準）。命令先頭は `mov esi,imm`/`push imm` が site−1、`mov ecx,[imm]`(8B 0D) が site−2。#21〜23 の命令先頭は 0x457705/0x4575e0/0x4574ab で、関数先頭からのアライン disasm で `push 0x…` 命令境界を個別確認済み。）

### 1.3 テーブル帯（4-byte stride ポインタ配列）の探索 — 負証拠

- `.rdata`/`.data` 全体で CHANGE_* 文字列 VA を 4-byte stride で 2 件以上連続に含むポインタ配列候補を走査（v305_5）→ **0 区間**[確定]。
- 文字列帯 0xA3914C〜0xA39E6C は「型名 + 人間可読説明（例: `Fame(%i)`、`Combat Style(%i)`、`Process Level: High` 等）の交互レイアウト**（ポインタ配列ではなく生文字列の連鎖、各終端 `0D 0A 00` の後に 1〜4 バイトのアライン 00 パディング）。帯の直前 0xA390D4 には `"Could not create save buffer, out of memory."` が続く共通 .rdata 文字列池の一部。
- 参照はすべて即値（imm32）のみで、ポインタテーブル経由の間接参照は存在しない[確定]。

---

## 2. 件B: .ess ヘッダ経路（TES4SAVEGAME）

### 2.1 リテラルと参照

- 検索 `b"TES4SAVEGAME\x00"`（13 バイト込み）: ファイル全体で **1 箇所のみ**: `.rdata` **VA 0xA38FA0**（file 0x6381A0。直後に `00 00 00` パディング、直前は `"File '%s' is not a valid TES file.\0"` @0xA38F7C、`"Could not close file '%s'.\0"` @0xA38F60、`"SetFilePointer() in SetOffset failed with error:\n%s\0"` @0xA38F2C、`"GetFilePointer() in NextForm failed with error:\n%s\0"` @0xA38F10 — ファイル検証/セーブ I/O エラー文字列群と同席）[確定]。
- `.text` imm32 参照（全バイト走査＋命令境界検証）: **ちょうど 2 サイト**、両方 `push 0xA38FA0`（直前に `push 0xC`）:
  - **0x45DC6A**（imm フィールド 0x45DC6B）→ 包含関数 **FUN_0045DBC0**（0x45DBC0..0x45DE30、先頭 16 アライン＋CC ラン 13 検証済）= **読取（ロード）側**
  - **0x464B0D**（imm 0x464B0E）→ 包含関数 **FUN_00464AB0**（0x464AB0..0x465130、先頭 16 アライン＋CC ラン 3 検証済）= **書出（セーブ）側**
- `.rdata`/`.data`/`.bind` に追加の imm32/ポインタ参照: ゼロ[確定]。`"TES4"/"SAVE"/"GAME"` を u32 即値として .text 全体検索 → 全ゼロ（マジック比較は文字列ポインタ + memcmp 経由のみ）[確定]。

### 2.2 読取経路 FUN_0045DBC0（命令列サマリ）

```
0x45DBC0  sub esp, 0x318 / ガード・プロローグ / esi=[esp+0x324]（ストリーム引数）edi=this
0x45DBE3..  push 1 / push buf[esp+0x20] / push 0xC / push esi / call edx([esi+4])  ; ストリームvtable+4 = read(buf,12)
0x45DC0D  cmp byte [esp+0x14],0 / je 0x45DC83                      ; read 失敗分岐
0x45DC14..  push 4 / push 0xA3A954("CON ") / lea eax,[esp+0x18] / call 0x984166  ; memcmp(buf, "CON ", 4)
0x45DC28  test eax,eax / jne 0x45DC64                              ; 一致しなければTES4SAVEGAME検査へ
0x45DC2C..  ; "CON "一致時: [0xA853D0](=0x78?) push / vtable+0xC seek 0xD000 → 先頭13バイトを再読 (buf[esp+0x10]) @0x45DC40..0x45DC61（再読サイズ 0xC）
0x45DC64..  push 0xC / push 0xA38FA0("TES4SAVEGAME") / lea eax,[esp+0x18] / call 0x984166  ; memcmp(buf,12)
0x45DC78  test eax,eax / je 0x45DC94                               ; マジック一致で継続
0x45DC7C  xor eax,eax / jmp 0x45DE0C                               ; 不一致 → リターン0（無効ファイル）
0x45DC94..  call vtable+4 ×2: push 1 / lea ebx=[edi+0x70] / push ebx → **this+0x70 ← 次1バイト（形式/種別バイト）**
                                    push 1 / lea ebp=[edi+0x71] / push ebp → this+0x71 ← 次1バイト
0x45DCC1  mov cl,[ebp] / mov byte [edi+0x7c],cl                    ; **version バイト = this+0x7c（ストリーム offset 13〜14 の後続バイト）**[確定]
0x45DCD0  cmp byte [edx+0x7c], 0x52 / jb 0x45DD33                  ; gate#1: 現ゲームバージョン([0xB33B00]+0x7c) < 0x52 分岐
0x45DCD6  cmp byte [esp+0x330],0（引数 bool）
           真:  0x45DCFC read 0x10B→this+0x94 / 0x45DD1A read 4B→this+0xA4      ; ≥0x52 用追加ヘッダ 20B を field 化
           偽:  0x45DD31 vtable+0xC seek([0xA853D4], 0x14)                       ; 旧ファイル: 追加 20B をスキップ
0x45DD38  cmp byte [eax+0x7c], 0x52 / jae 0x45DD69
           偽: this+0x94..0xA4 ← -1（0xFFFFFFFF×5 埋値）
0x45DD83  read 4B → [esp+0x20]                                     ; 直後の 4 バイト（[0xB33C14]/[0xB33C18] 関連、詳細は残点）
0x45DD85..  byte[edi+0x70]==0 かつ byte[edi+0x71] ∈ (==0x7d or >=0x13 → 0x45DE08=fail) の範囲チェック[確定]
0x45DDB9  push 0xA3A8E8("Save game version is %i.%02i and current version is %i.%02i, so errors may occur.  Continue trying to load?")
          + lea/log call 0x98208B @0x45DDBF                        ; バージョン不一致警告文字列経路[:確定:site]:[推定:表示]
0x45DDC9  or dword [this+0x18], 0x10000 / and ... 0xFFFEFFFF       ; 続行処理で状態ビット 0x10000 toggling
0x45DDEA  call 0x579CF0（yes/no ダイアログ系と推定）→ cmp al,2 → -1 化
0x45DE18  call 0x9811E2（__security_cookie チェック）/ ret 8
```
- FUN_0045DBC0 の E8 呼び出し元: 0x46112C(FUN_4610F0) / 0x4640B4(FUN_463EC0) / 0x464395(FUN_464320) / 0x465999(FUN_465860) の 4 箇所[確定]。

### 2.3 書出経路 FUN_00464AB0（命令列サマリ）

```
0x464AB0  プロローグ（SEH push -1 / push 0x9AE718）/ esi=this / edi=[esp+0x74]（ストリーム引数）
0x464AEB  mov byte [esi+0x70], 0                                    ; 形式/種別バイト ← 0
0x464AEE  mov byte [ebp], 0x7D   (ebp=esi+0x71)                     ; **version バイト ← 0x7D(125)**
0x464AF2  mov byte [esi+0x7c], 0x7D                                  ; this+0x7c にも 0x7D（現行版定数）[確定]
0x464AF6..  if ([esi+0x18]>>9 & 1) → [esi+0x90] += 0xC（ドライカウント）else
0x464B04..  push 1 / lea ecx,[esp+0x30] / push 0xC / push 0xA38FA0("TES4SAVEGAME") / push edi / call edx([edi+8])  ; vtable+8 = write(magic,12)
0x464B37..  push 1 / push ebx(=&[esi+0x70]) / push 1 → write 1B（+0x70）
0x464B75..  push eax(=1) / push 1 → write 1B（version 0x7D）        ; 実質 offset13 に 1 バイト
0x464B8E..  cmp dword [esi+0xA4],0 の分岐で call [0xA28108]（strlen系）で byte 化 → [esi+0xA4] ←
0x464BB4..  push 0x10 / lea edx,[esi+0x94] → write 16B（+0x94..0xA3）
0x464BEB..  push 4 / push ebx(=&[esi+0xA4]) → write 4B（+0xA4）
0x464C0A..  call 0x4DA2A0 / 0x5E1FD0 / 0x4DE8D0 / 0x6ECC80（プラグイン数・名前長算出、[0xB333C4] オブジェクト経由）
0x464CD9..  ff15 [0xA28174]（sprintf系）/ 以降 0x100/0x400 定数群で continued（ファイル名・タイムスタンプ部）
```
- FUN_00464AB0 の E8 呼び出し元: 0x465228（FUN_00465130 のうち）1 箇所のみ[確定]。FUN_00465130 はセーブヘッダ群の上位合成関数で、E8 呼び出し元 = 0x466AF6/0x466BD0/0x466D21/0x5024BA/0x5D3246/0x5D3438 の 6 箇所[確定]。
- 書出レイアウト裁定（命令列から）: `magic(12) | this+0x70(1) | version 0x7D(1) | +0x94 の 16B | +0xA4 の 4B | …（プラグイン名配列は FUN_45D5F0 側で読む）`。**version dword 実体のオフセット = ファイル内 offset 13 単一バイト（.ess 上）、オブジェクト内は this+0x71→this+0x7c（1 バイト幅）**[部分]（読み側 0x45DCC1 の 1 バイト転送＋書き側 0x464AEE の 1 バイト定数が対）。

### 2.4 version ゲート比較 site 表（band 0x440000〜0x470000・dedup capstone 検証済）

| 閾値 | site VA | 命令（bytes） | 包含関数 |
|---|---|---|---|
| 0x1C (28) | — | 該当なし（帯内ゼロ）[確定] | — |
| 0x2E (46) | 0x45D84A | `cmp byte ptr [edx+0x7c], 0x2E` (80 7A 7C 2E) | FUN_0045D5F0 |
| 0x34 (52) | 0x45D6DF | `cmp byte ptr [eax+0x7c], 0x34` (80 78 7C 34) | FUN_0045D5F0 |
| 0x37 (55) | — | 該当なし（帯内ゼロ）[確定] | — |
| 0x38 (56) | 0x45D67E / 0x45D699 / 0x45D7B6 / 0x45D7DB | `cmp al,0x38`(3C 38) / `cmp byte[edx+0x7c],0x38` / `cmp byte[eax+0x7c],0x38`×2 | FUN_0045D5F0 |
| 0x51 (81) | — | 該当なし（帯内ゼロ）[確定] | — |
| 0x52 (82) | 0x45DCD0 / 0x45DD38 | `cmp byte ptr [edx/eax+0x7c], 0x52` (80 7x 7C 52)×2 | FUN_0045DBC0 |
| （参考）0x1D (29) | 0x45D791 | `cmp byte ptr [edx+0x7c], 0x1D` | FUN_0045D5F0（契約閾値 0x1C に対し実コードは 0x1D を使用） |
| （無関係 real cmp） | 0x459740 (`[ebx+edx]`,0x2E・FUN_459570) / 0x448EAD (`[eax+esi+2]`,0x52・FUN_448E20) | '.' / 'R' 文字検査と推定、version ゲート非該当として除外 |

- imm32 形式（`cmp reg32, 0x1C` 等）の閾値比較: 帯内**ゼロ（負証拠）**[確定]。全 gate は byte-width（version は 1 バイト）。
- 実ゲート site 数（version バイト +0x7c 検査・検証済）= **9**（FUN_45D5F0×7 + FUN_45DBC0×2）。
- raw byte-pattern スキャン（v305_23/305_17 の非dedup版）が報告した 0x45D5D9(0x52)/0x45D887(0x51)/0x464DFA(0x51)/0x46568B(0x51)/0x4657DF(0x52)/0x465B4D(0x51) は、すべて命令境界検証で**偽陽性（他命令の imm/disp バイト）と判定し除外**[確定]。
- FUN_0045D5F0（0x45D5F0..0x45DBB6、先頭アライン/CC13 検証済）はヘッダ直後の**プラグイン名テーブル（"File '%s' is not a valid TES file." 報告経路を含む）読み取り**関数と推定[:部分]。E8 呼び出し元は FUN_45DBC0 と同一の 4 callers（0x4611E8/0x464115/0x4643D6/0x465A3F）[確定]。

### 2.5 圧縮ペイロード開始位置の裁定

- **zlib 静的リンクの存在証拠**[確定]:
  - `" inflate 1.2.1 Copyright 1995-2003 Mark Adler"` @.rdata 0xA82858（直前 0xA82820〜0xA82854 に zlib エラー文字列群 `need dictionary / stream end / file error / stream error / data error / insufficient memory / buffer error / incompatible version`）
  - `"deflate 1.2.1 Copyright 1995-2003 Jean-loup Gailly"` @0xA826F9
  - inflate 本体エラー文字列（`incorrect header check` @0xA826B0 / `invalid block type` @0xA8266C / `header crc mismatch` @0xA82680 / `unknown compression method` @0xA826DC 等）は FUN_007425A0〜 の状態機械内部から imm32 参照（0x7426F6/0x74276F/0x7427B7/0x742AD9/0x742C74 等）[確定]。`zlib.dll`/`zlib1` 名は全ファイルに無く、CRT 静的リンク版 zlib 1.2.1。
- **zlib API の同定**[部分]: inflate=FUN_007425A0（内部 `cmp ecx,0x1C`+ジャンプテーブル 0x7438E4 の状態機械）、inflateInit 系=FUN_00742490、zfree/管理解放=FUN_00743970、deflate=FUN_00743A40、deflateInit 系=FUN_00744FB0。
- **.text 全体 E8 走査で判明した game 側ラッパと呼び手**[確定（エッジ）]:
  - inflate 経路: FUN_00450550（inflateInit+inflate+zfree を使用、`[esi+0x23c/0x240/0x244(0x40000)]` ゲート）の呼び手 = 0x450D61/0x450EC3/0x450F88（FUN_004501F0）。FUN_0042C6F0（file window `[esi+0x15c/0x16c/0x170]` 経由で inflate @0x42C798 直呼び）。FUN_0042D6D0（inflateInit+zfree）。FUN_0042C6F0 は imm32 が `.data` ポインタ 0xA35E9C に積まれ vtable 経由[:部分]。
  - deflate 経路: FUN_0046B2C0（deflate 0x46B410 / deflateInit 0x46B3C9 / 0x743E50）の呼び手 = 0x45C817(FUN_0045C7A0) / 0x46B593(FUN_0046B590) / 0x46C289(FUN_0046C220)。FUN_0046B370（deflateInit 群）の呼び手 = 0x4C8CE1 / 0x4E659B。
- **圧縮ストリーム先頭（0x78 0x9C/0x01/0xDA 系）の直接検査コード**: `.text` 内 `'78 9C'` バイト対ヒットは 0x454B82 等すべて `mov ecx,[0xA39C78]` の modrm/disp バイトなど**偽陽性**（境界検証済）。ヘッダ読み FUN_0045DBC0 / 書き FUN_00464AB0 のどちらにもヘッダ後オフセットの算出式（`base = hdr_end + n`）や zlib ストリーム先頭バイト検査は**存在しない**[確定]。チャンクマジック候補 u32（`COMP`/`NOCS`/`ZNPS`/`LZ4 `/`DEFL`/`ZLIB`）の .text imm32 出現は**ゼロ**（`COMP` のファイル内ヒット 3 件は "PROCEDURE_**ACCOMP**ANY" / "**COMPLETE**" / "IN**COMPLETE**" の部分文字列のみ）[確定]。`HUNK`/`CHUNK`/`SIZE` NUL 終端文字列も無し。
- **裁定**: 「ヘッダ直後の圧縮ペイロード開始位置の決定式」は**このリテール exe の静的証拠では特定不能（負証拠）**。ヘッダ読取関数は読/ seek のみをストリーム vtable（+4 read / +8 write / +0xC seek と推定）に委ね、圧縮境界の判定は deflate/inflate ラッパ側（FUN_0046B2C0 / FUN_00450550 / FUN_0042C6F0）のチャンク管理で行われている。チャンクマジックが imm32 に一切_materialize_ しない以上、チャンク層（'DECOMP'/'COMP'）の判定は (a) 別命令列でのバイト組立、(b) `.data` 上の 4 バイト定数、(c) ラッパ内部比較のいずれかだが、本帯 0x451000〜0x467000 では観測できず → 次束の残点。

---

## 3. 判定サマリ

| 区分 | 件数 | 内容 |
|---|---|---|
| [確定] | 17 | CHANGE_* 67 件全列挙・67/67×1 箇所 xref・単一包含関数 FUN_00453A90(0x453A90..0x458E50)・参照 3 フォーム(48/16/3)・`\r\n\0` 終端・ポインタテーブル帯なし(負)・TES4SAVEGAME 全ファイル 1 箇所 @0xA38FA0・imm32 参照 2 サイト・読 FUN_45DBC0 / 書 FUN_464AB0 の命令列・version バイト this+0x7c 転送(0x45DCC1/0x45DCC4)・書き側現行版 0x7D・帯内ゲート 9 サイト（0x1C/0x37/0x51 帯内不在は負）・imm32 形ゲート不在(負)・zlib 1.2.1 静的(負: zlib.dll 無し)・COMP/NOCS 等 u32 imm 無し(負)・0x78 系ヘッダ検査無し(負)・ペイロード開始式 特定不能(負) |
| [部分] | 4 | memcmp=FUN_00984166 の同一性（バイト+dword 混合比較は一致、ライブラリ名は未確認）/ inflate 等の zlib API 割当（署名一致）/ ストリーム vtable +4/+8/+0xC の read/write/seek 意味 / FUN_45D5F0 の「プラグイン名テーブル読み」役割 |
| [推定] | 3 | FUN_453A90 の ChangeData タイプ名ダンプ用途の細部 / 「CON 」4 バイト分岐の意味（非 Steam/修正版ファイル対応と推定）/ +0x94..0xA4 の 16+4 バイト追加ヘッダの意味（既知 .ess ヘッダのファイル作成日時/ロード画面情報等との対応） |

---

## 4. 新規残点（次束用・低優先）

1. FUN_0045DA30（0x45DA30..0x45DBB6、0xB02F9C 系ログ文字列群＋0x9832E6 呼出）: ヘッダ読取 FUN_45DBC0 との役割分担（XFER フェーズ名ロギング帯と推定されるが未検証）。
2. 「CON 」(0xA3A954) 4 バイト一致分岐の意図 — 0xA853D0 の 0x78 dword と seek 0xD000 の関係（GOG/Steam 両対応判定ロジックの可能性）。動的証拠または FUN_45D450 呼び出し文脈で確定。
3. チャンク層 'DECOMP'/'COMP' マジックの組立 site 探索（imm32 不在が確定したため、バイト逐書 or `.data` 4B 定数 or ラッパ内部比较のいずれか。FUN_46B2C0 本体の 0x9828C0/0x450980 周辺が候補）。
4. 0x52 ゲートで読む +0x94 の 16B と +0xA4 の 4B の内容（セーブヘッダの既知フィールド群—作成日時/ロード画面数—との対応取り）。
5. 全 .text の `cmp byte [reg+0x7c], imm8` raw ヒット 579 件の整列デコード検証（+0x7c version 型が他 Object 系にも蔓延しており、band 外 gate の全体棚卸しは未実施）。
6. ストリーム vtable オブジェクト（[0xB333C4]/[0xB33B00] 系統）の +4/+8/+0xC エントリ確定（read/write/seek の割当を FUN_4520C0/FUN_4522E0 帯から）。
7. FUN_004501F0（inflate ラッパ FUN_450550 の 3 呼出元）の圧縮チャンクループ境界（0x10000=64KB? [esi+0x244]&0x40000 ゲート）。
8. FUN_00984166 引数順の確定（memcmp(dst,src,n) か）と CRT 名。
9. 0x45DD83 の 4B フィールド（[0xB33C14]/[0xB33C18] 関連）のヘッダ内位置。
10. FUN_464910（マジック書き FUN_464AB0 の直前・同一呼び手 chain）と FUN_465130 のセーブファイル名生成（[0xA2F920] の除算定数=秒→日時換算と推定）関連の確定。

---

## 5. selftest 再現メモ

実行環境: `python $env:TEMP\opencode\v305_<n>.py *> $env:TEMP\opencode\v305_<n>.out`（chcp 65001、%TEMP%=C:\Users\hiroki.kogarumai\AppData\Local\Temp）

| スクリプト | 内容 | 再現結果（今回） |
|---|---|---|
| v305_1/2 | PE ヘッダ/セクション表 + 全ファイル "CHANGE_" 生検索 | ImageBase 0x400000、6 セクション、契約 VA→file マッチ確認。CHANGE_ 67 / TES4SAVEGAME 1 |
| v305_3 | 0xA38FA0 / 0xA3914C 帯 hexdump | `TES4SAVEGAME\0`+padding、型名+説明の交互レイアウト確認 |
| v305_5 | CHANGE_* 列挙 + .text imm32 xref + データ参照 + stride-4 テーブル探索 | 67 件・各 1 xref・テーブル帯 0 区間（データ参照ゼロ） |
| v305_15/16/25 | アライン修正後の関数先頭（4CC+16 アライン規則）| CHANGE_* 全 67 → FUN_00453A90 / TES4SAVEGAME 2 サイト → FUN_0045DBC0・FUN_00464AB0（v305_16 の暫定帰属 FUN_45DA30/FUN_464910 は非アライン境界誤判定として弃却） |
| v305_23 | zlib 文字列検索・u32 マジック検索・E8 走査 | inflate 1.2.1 @0xA82858、deflate 1.2.1 @0xA826F9、zlib.dll 無し、COMP/NOCS u32 .text ゼロ |
| v305_26/31 | ラッパ→E8 呼び手・包含関数・vtable 文字列 push | inflate 呼び手 FUN_450550/FUN_42C6F0、deflate 呼び手 FUN_46B2C0→FUN_45C7A0/46B590/46C220、FUN_45DBC0 呼び手 4・FUN_464AB0 呼び手 1 |
| v305_29/32 | 帯 0x440000-0x470000 の dedup capstone cmp 走査＋境界検証 | ゲート 9 サイト確定・raw 偽陽性 6 サイト弃却・0x1C/0x37/0x51 帯内ゼロ・imm32 形ゼロ |
| v305_30/33/34 | writer 包含関数 FUN_464AB0 のアライン確認・'CON ' / 0xA3A8E8 文字列・FUN_453A90 頭部 | 0x464AB0=16 アライン run3・「Save game version is %i.%02i…」@0xA3A8E8・ジャンプテーブル 0x458E38 |
| v305_35 | FUN_45DBC0 尾部 + FUN_464AB0 中央の完全アライン dump | +0x7c 転送(0x45DCC1)、0x52 ゲート 2、write 列 12B/1B/1B/16B/4B 確認 |
| v305_28 | 67 行 markdown 表生成 | `v305_28_table.md` 67 行（本ファイル 1.2 節と一致） |

中間物: `%TEMP%\opencode\v305_*.py` / `*.out` / `v305_4_xref.json` / `v305_26_forms.json` / `v305_28_table.md`（再実行で同一出力を確認済み。唯一の非決定要素なし）。

## 6. 訂正（v307 統合時・2026-11-06）

後続束 `v307_saveheader_residuals.md`（本 artifact の残点 10 件の調査）により、本 artifact の記述を以下で**訂正**する:

1. **[0xA853D0] の dword は「0x78」でなく実値 0**（'CON ' 分岐における origin 定数。0x78 は raw スキャンの見間違い）。
2. **FUN_45D5F0 の範囲は 0x45D5F0..0x45DBB6 でなく ..0x45D919**（境界の CC 判定差）。
3. **帯内ゲートの意味は「現ゲームバージョン」ではなくファイル version byte（this+0x7c）の比較**。
4. **FUN_004501F0 は FUN_450550（inflate ラッパ）の呼び手ではない**（該当なし。seek END helper と同定）。
5. **0x9832E6/0xB02F9C バンドは FUN_45DA30 でなく FUN_45DE30 にある**（FUN_45DA30 は非関数帯）。
6. **FUN_464910 はファイル名生成でなくプラグイン/スレッド検査系**。
7. **raw パターン「579 件」は再現不能**（同仕様で 299 件・全境界検証 0 棄却。327/451/23 の変異を併記）。

また caller 帰属 1 点（0x45CD9F∈FUN_0045C9C0）は **v306 により FUN_0045CC60 へ訂正**済み（詳細は v304 追補3）。
