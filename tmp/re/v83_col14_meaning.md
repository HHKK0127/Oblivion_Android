# V83: オペコード表 0xB0AF48 列 +0x14 の実体 — 引数記述子テーブル（arg descriptor table）

日付: 2026-10-24 / 手法: probe_harness（selftest 4/4 PASS）による raw PE 読み取り（セクション aware rd()、.data const = VA − 0x401400）。スクリプト: `%TEMP%\opencode\v83_col14.py` / `v83_col14b.py` / `v83_col14c.py`。対象: Oblivion.exe (Steam v1.2.0416)、テーブル基底 0xB0AF48、stride 0x28、532 エントリ。

**結論（verdict）**: +0x14 は **引数記述子（arg-spec）配列の先頭ポインタ**。.data 内 0xB0A11C–0xB0AF3C の記述子プール（表基底 0xB0AF48 の直下）を指し、12 バイトレコード {u32 名前文字列 ptr(.rdata), u32 型コード 0x00–0x25, u32 optional フラグ} が引数個数（+0x12）ぶん連続する。文字列ペアでも .text 関数ポインタでもない。

## 1. 全数計測（532 件 raw PE、u32 単位）

### 1.1 +0x14 の全数分布
- nonzero = **281 / 532**、zero = 251。ユニーク値 = **94**（記述子先頭の共有）。
- セクションヒスト: **.data 281 件 / .text 0 件 / .rdata 0 件 / UNMAPPED 0 件**。
- 値域: min = **0xB0A11C**、max = **0xB0AF30**（+12 = 0xB0AF3C ≤ 表基底 0xB0AF48）。≥ 0x10000000 の値は 0 件（符号付きフラグ等の疑いなし）。
- **+0x14 ≠ 0 ⟺ +0x12（u16 引数個数）> 0 が完全一致**（双方向とも例外 0 件）。引数なし 251 件はすべて +0x14 = 0。
- 総引数スロット（Σ argn）= **402**、実体レコードは **179 ユニーク**（記述子の再利用・共有）。

### 1.2 +0x10 の全数分布（旧 GT の「408 nonzero」の正体）
+0x10 周りは 4 バイトにパックされた複合フィールドで、単一の u32 ポインタではない:
- byte0 (+0x10) = フラグ A: {0: 260, 1: **272**}。
- byte1 (+0x11) = **全 532 件 0**（Project Knowledge の旧記述「flagB 0x00/0x01/0x02」は今回の全数計測で再現せず。以後 0 として扱う）。
- u16 (+0x12) = 引数個数 argn: {0: 251, 1: 195, 2: 65, 3: 12, 4: 5, 5: 3, 6: 1}。
- u32(+0x10) ≠ 0 = **408** は byte0 フラグ（272）と argn>0（281）の和集合（重複 45）。
- u32(+0x10) の distinct 値は 11 種のみ: 0x1×127, 0x10001×103, 0x10000×92, 0x20000×34, 0x20001×31, 0x30000×7, 0x30001×5, 0x40001×3, 0x50001×3, 0x40000×2, (0x60001 ほか少数)。≥ 0x400000（ポインタ相当）は **0 件**。

### 1.3 +0x20（対照計測）
- nonzero = **169 / 532**、distinct = 166（Project Knowledge と一致）。

### 1.4 サンプル 8 エントリの raw ダンプ（+0x00〜+0x28）
| idx | VA | raw(先頭32B) | name1/name2 | opid | +0x0C | +0x10 u32 | +0x14 | +0x18 | +0x1C | +0x20 | +0x24 |
|----|----|----|----|----|----|----|----|----|----|----|----|
| 0 | 0xB0AF48 | 9418a500 ecf7a200 00000000 ecf7a200 00000000 00000000 00000000 d0225000 | GameMode / "" | 0x0000 | 0xA2F7EC(空) | 0 | 0 | 0x5022D0 | 0x4FDE30 | 0 | 0 |
| 21 | 0xB0B290 | 7817a500 ecf7a200 15000000 ecf7a200 01000000 00000000 00000000 60455000 | OnLoad / "" | 0x0015 | 0xA2F7EC(空) | 0x00000001 | 0 | 0x504560 | 0x4FDE30 | 0 | 0 |
| 50 | 0xB0B718 | a813a500 a013a500 13010000 4013a500 00000000 00000000 00000000 50a05000 | Show1stPerson / S1ST | 0x0113 | 0xA51340 | 0 | 0 | 0x50A050 | 0x4FDE30 | 0 | 0 |
| 54 | 0xB0B7B8 | c412a500 bc12a500 17010000 9012a500 00000100 eca7b000 903e5100 30de4f00 | PickRefByID / PRID | 0x0117 | 0xA51290 | 0x00010000 | **0xB0A7EC** | 0x513E90 | 0x4FDE30 | 0 | 0 |
| 162 | 0xB0C898 | 60fca400 ecf7a200 83010000 00000000 ... | ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!! / "" | 0x0183 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 163 | 0xB0C8C0 | 54fca400 ecf7a200 00100000 ecf7a200 00000100 1ca1b000 80325100 50015000 | MessageBox / "" | 0x1000 | 0xA2F7EC(空) | 0x00010000 | **0xB0A11C** | 0x513280 | 0x500130 | 0 | 0 |
| 252 | 0xB0D6A8 | d8f6a400 ecf7a200 59100000 ecf7a200 00000100 1ca1b000 00365100 50015000 | Message / "" | 0x1059 | 0xA2F7EC(空) | 0x00010000 | **0xB0A11C** | 0x513600 | 0x500150 | 0 | 0 |
| 496 | 0xB0FCC8 | e8e0a400 ecf7a200 4d110000 ecf7a200 00000100 1ca1b000 d0715000 30015000 | EssentialDeathReload / "" | 0x114D | 0xA2F7EC(空) | 0x00010000 | **0xB0A11C** | 0x5071D0 | 0x500130 | 0 | 0 |

（entry54 の +0x14 = 0xB0A7EC は v76 が「名前ポインタ」と誤読した値。先頭16B = `2c1ba500 04000000 00000000 2c1ba500` → 記述子 {'ObjectReferenceID', code=4, optional=0}。PRID は ObjectReferenceID を 1 個取るコマンドで、記述子内容と整合。）

## 2. 假説検証

### 2.1 仮説 a「name1/name2 ペア」— 棄却
- +0x10 は pointer ではない（distinct 11 種・最大 0x50001・≥0x400000 は 0 件、§1.2）。実体は flagA byte + 未使用 byte + argn word のパック。
- +0x14 の指す先は .rdata 文字列ではなく .data の 12 バイト構造体。cstr すると制御文字（例: `',\x1b¥'`）となり文字列として読めない。
- 短名・別名は +0x04 に既に存在（S1ST / PRID 等 176 件）。

### 2.2 仮説 b「alternate handler（OnHit/OnActivate 型の二次ハンドラ）」— 棄却
- 281 件すべて .data（0xB0A11C–0xB0AF30）。.text（0x401000–0xA27E00）に落ちる値は **0 件**。関数ポインタなら .text に落ちるはず。
- 実装ハンドラは +0x18（527/532、V78 確定）で既に埋まっており、二重構造と整合しない。

### 2.3 仮説 c「その他」— 採用: **引数記述子テーブル（arg descriptor）**
- レコード形式（12 バイト、+0x14 が引数スロット 0 の先頭）:
  `struct ArgDesc { const char* name; /* .rdata 文字列 */ u32 type; /* 0x00–0x25 = 38 種 */ u32 optional; /* 0 or 1 */ };`
- argn 個のレコードが +0x14 から連続（全 281 エントリで argn*12 バイト分すべて有効な名前 ptr、検証 0 例外）。
- **記述子プール**: .data の [0xB0A11C, 0xB0AF48) = 表基底の直下 0xE2C バイト。4 バイトグリッドで「{rdata 文字列 ptr, code≤0x25, flag≤1}」を満たす位置 192 件（うち arg list が消費 179、未消費 11–13 件は未使用レコードまたは偶発一致）。プール直前 0xB0A100 付近は別構造（属性名等のポインタ配列）。
- **型コード目録（38 種、使用頻度上位）**: 0x00 String/Filename, 0x01 Integer, 0x02 Float, 0x03/0x15 ObjectID, 0x04 ObjectReferenceID, 0x05 Actor Value, 0x06 Actor/Criminal/Victim, 0x07 Spell Item, 0x08 Axis, 0x09 Cell, 0x0A Animation Group, 0x0B Magic Item, 0x0C Sound, 0x0D Topic, 0x0E Quest, 0x0F Race, 0x10 Class, 0x11 Faction, 0x12 Sex, 0x13 Global, 0x14 Furniture, 0x16 Variable Name, 0x17 Stage, 0x18 Map Marker, 0x19 Actor Base, 0x1A Container, 0x1B WorldSpace, 0x1C Crime Type, 0x1D Package, 0x1E Combat Style, 0x1F Magic Effect, 0x20 Birthsign, 0x21 Form Type, 0x22 Weather ID, 0x23 NPC, 0x24 Owner, 0x25 Effect Shader ID。
- **optional フラグ**: 179 レコード中 52 件が 1。うち 41 件は名前自体に "(optional)"/"(Optional)" を含む（'Integer (Optional)'×15, 'Float (Optional)'×10 等）。残り 11 件は名前に optional 表記がなくても flag=1（'ObjectID'@0xB0A7E0, 'ObjectReferenceID'@0xB0A7F8, 'Emotion', 'EmotionValue', 'String', 'Count', 'Distance' 等）。
- **共有の例**: 0xB0A6CC = {'Integer', 1, 0} を **42 コマンド**が共用（ToggleCellNode / SetCameraFOV / SetAlert / SetCrimeGold 等の 1 引数コマンド）、0xB0ABAC = {'Actor (optional)', 6, 1} を OnHit / OnActivate / OnDeath / OnStartCombat 等 **12 件**が共用、0xB0A11C = {'String', 0, 0} を MessageBox / Message / EssentialDeathReload 等 **13 件**が共用。
- **具体例（引数列の直感的整合）**:
  - idx1 MenuMode argn=1: ['Integer (Optional)']
  - idx13 OnAlarm argn=2: ['Crime Type'(code 28), 'Actor (optional)'(code 6)]
  - idx54 PickRefByID argn=1: ['ObjectReferenceID'(code 4)]
  - idx31 Show argn=1: ['Float']
- **消費者（決定的証拠）**: スクリプトコンパイラ **FUN_004ff710 @ 0x4FF710**（docs/Oblivion_decompiled.c L198134–198254、begin ブロック処理）。begin 名照合後、L198214 で entry+0x10 byte0 をテスト、L198215–198229 で:
  `local_1158 = (&DAT_00b0af5c)[i*10];`（= entry+0x14 = 記述子 ptr）
  `local_115c = (ushort)(&DAT_00b0af5a)[i*0x14];`（= entry+0x12 = argn）
  `pcVar1 = (&PTR_FUN_00b0af64)[i*10];`（= entry+0x1C = dispatcher）
  `(*pcVar1)(argn, argdesc, &argbuf, ctx);` → **dispatcher 0x4FDE30（528 件）が記述子列に従って引数をデコード/検証**する。0x4FF880–0x4FF9BE の逆アセンブル（memory の lea/mov edx/call ebx）と同一箇所。

## 3. 最終列レイアウト表（+0x00〜+0x24 全 11 列）

| オフセット | 型 | 意味 | 全数証拠（/532） |
|-----------|----|------|------------------|
| +0x00 | u32 | 長名 ptr（.rdata 文字列） | 532/532 有効（GameMode, OnLoad, PickRefByID …） |
| +0x04 | u32 | 短名・別名 ptr（.rdata） | 実別名 176 件（S1ST, PRID …）、空文字列共有 ptr 0xA2F7EC が 356 件、0 は 0 件 |
| +0x08 | u16 | オペコード id | 532 unique・狭義単調増加・3 帯域（0x00–0x1E が 31 / 0x100–0x183 が 132 / 0x1000–0x1170 が 369） |
| +0x0C | u32 | 説明文 ptr（help テキスト） | 非空 132 件（"Show global scripts and variables." 等）、空文字列 399 件（うち 389 は共有 0xA2F7EC）、0 は idx162 のみ |
| +0x10 | u8 | flagA（begin ブロック適法性フラグ） | 1 が 272 件。FUN_004ff710 L198214 で quest-script 判定に使用。イベント帯 31 件中 29 が 1（flagA=0 は GameMode / MenuMode のみ）。コンソール帯の意味（0x100 帯 8/132、0x1000 帯 235/369 が 1）は未完全確定 |
| +0x11 | u8 | 未使用（全 0） | 532/532 = 0。旧「flagB 0x01/0x02」記述は失効 |
| +0x12 | u16 | 引数個数 argn | {0:251, 1:195, 2:65, 3:12, 4:5, 5:3, 6:1}、総スロット 402 |
| +0x14 | u32 | **引数記述子配列 ptr（.data プール 0xB0A11C–0xB0AF3C、12B レコード {name, type 0x00–0x25, optional}、argn 個連続）** | 281（⟺ argn>0、完全一致）、94 ユニーク先頭、.text/.rdata 落ち 0 件 |
| +0x18 | u32 | 実装ハンドラ（.text） | 527/532（zero 5 件: idx162/188/189/216/242） |
| +0x1C | u32 | 共有 dispatcher | 0x4FDE30×528 / 0x500130×2（MessageBox, EssentialDeathReload）/ 0x500150×1（Message）/ 0×1（idx162） |
| +0x20 | u32 | 条件/評価系関数 ptr（CTDA 対応） | 169/532・166 ユニーク |
| +0x24 | u32 | 小フラグ | 0x1×9（+0x20 あり）/ 0x100×10（+0x20 なし）、計 19 |

u32 として +0x10 を読むと「408 nonzero」になるが、これは flagA byte と argn word の和集合であり、単一フィールドではない点に注意（旧 V78 列GT の "+0x10 nonzero=408" は正しい計測値・誤りではない）。

## 4. 参考: 古 GT「+0x14 = OnHit/OnActivate の 2 件のみ」誤りの由来

- 旧 GT 表記:「+0x14 = pointer only for OnHit/OnActivate (2 entries, VA 0xB0ABAC-ish)」。
- **誤りの構造**: OnHit（idx8, opid 0x0008）と OnActivate（idx2, opid 0x0002）は、いずれも同一の記述子 **0xB0ABAC** = {'Actor (optional)', code 6, optional 1} を指す。この 2 件だけを著名なイベントとしてサンプルした場合、「+0x14 にポインタがあるのはこの 2 件」「VA も 0xB0ABAC 付近」という観察になる。つまり「2 件」はサンプリングの偶然であり、実際は「OnHit/OnActivate が同じ記述子を共有している」の 1 つの事実にすぎない。
- **副次的な誤読**: v76 の entry54 ダンプは PickRefByID の +0x14 = 0xB0A7EC を「名前ポインタ」と解釈したが、0xB0A7EC の先は文字列ではなく {'ObjectReferenceID', 4, 0} の記述子レコードである（cstr は壊れたバイト列に見える）。
- **訂正の系譜**: 旧 2 件 → V78 全数計測で nonzero 281 を検出（Project Knowledge に「+0x14 = 引数記述子テーブル（281/532・94 ユニーク）」として暫定記録）→ **本 V83 で実体（12B レコード、38 型コード、プール位置、消費者 FUN_004ff710/dispatcher 0x4FDE30）を確定**。

## 5. RE_EXE_REPORT §67 への反映案（未適用・ユーザー判断待ち）

§67 のオペコード表列定義に以下を追記することを推奨（本ノートでは適用しない）:

1. 「+0x14 = OnHit/OnActivate のみポインタ」記述を撤回し、**+0x14 = 引数記述子テーブル ptr（281/532、94 ユニーク、プール .data 0xB0A11C–0xB0AF3C、レコード 12B {name ptr, type 0x00–0x25, optional flag}、argn 個連続、dispatcher 0x4FDE30 が消費）** に置換。
2. +0x10 を「単一フラグ」ではなく **byte0 = flagA（begin 適法性、272 件が 1）/ byte1 = 未使用（全 0）/ u16 +0x12 = 引数個数** のパックとして記述（「nonzero=408」は和集合である旨を注記）。
3. +0x04 に「空文字列共有 ptr 0xA2F7EC が 356 件」、+0x0C に「非空説明 132 件・0 は idx162 のみ」を注記。
4. 型コード 38 種の目録（§2.3）を付録として追加（Android 移植の script_opcodes.h 引数型定義と突合可能）。
5. 参考として §4（旧 2 件 GT の誤りの由来）を残し、今後のサンプリング注意喚起とする。

検証: `python probe_harness.py selftest` → 4/4 PASS。本ノートは raw PE 読み取りのみで生成（Ghidra 無依存、.data −0x401400 のピットフォール回避済み）。
