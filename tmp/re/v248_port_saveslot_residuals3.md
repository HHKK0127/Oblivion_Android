# v248_port_saveslot_residuals3.md — 移植セーブ経路残点第3束（N-32..N-36 裁定）

- 作成: 2026-11-02 / TODO serial 241（V241・低）/ **分析専任（docs/GT/TODO/ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v246_port_saveslot_residuals2.md` §5-2 新規残点 N-32..N-36（本体は読取のみ・無編集）。
- 入力（読取のみ）: `tmp/re/v246_port_saveslot_residuals2.md`, `tmp/re/v244_port_saveslot_residuals.md`,
  `tmp/re/cache/vtable_index.tsv`, `tmp/re/cache/named_funcs.txt`, `tmp/re/cache/call_targets`,
  `docs/Oblivion_decompiled.c`（部分検索）。
- 手法: capstone 5.0.7 再逆アセンブル＋PE 手動解析（セクション・インポートディレクトリ・RVA/VA 変換）。
  ESM 本体は未アクセス。git 不使用。既存 `%TEMP%\opencode\v246_scan*.py` の発想を再利用し、新規は `v248_scan*.py`。
- selftest: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**。
- 状態: 分析中（**1 項ごとに即時追記**）。
- PE 実測（`v248_scanA`）: `.text 0x401000..0xA27C39`（raw 0x400）/ `.rdata 0xA28000..0xB018D2`（raw 0x627200）/
  `.data 0xB02000..0xBABC1C`（raw 0x700C00）/ `.tls` / `.rsrc` / `.bind 0xBAF000..0xC05000`。
  `IMAGE_BASE = 0x400000`。→ **`0x400000 + RVA`** と **`VA`** の混同が本束の主題の一つ。

---

## §1 項目1（N-32）: IAT `[0xA28160]`/`[0xA28164]` の静的初期値 0x700E56 / 0x700E62

**判定: 【解明】異常ではない。0x700E56 / 0x700E62 は「.text を指す VA」ではなく、PE 仕様どおりの **RVA**（= `VA - IMAGE_BASE`）であり、指す先は `.rdata` の `IMAGE_IMPORT_BY_NAME`（hint+名前）構造体 `0xB00E56` / `0xB00E62`。0xA28160 は KERNEL32 の IAT（FirstThunk）スロット index 71、0xB00590 は同じ index 71 の **INT（OriginalFirstThunk）側ミラー**であって、独立した「名表」ではない。ローダが FT を解決済みアドレスで上書きするため、ファイル上の初期値が RVA のまま残るのは正常。**

### §1-1 算術の確定（`v248_scanC` C1..C4）

- インポートディレクトリ: RVA `0x700304` / size `0x12C`（`dir[1]`）→ VA `0xB00304`、**14 ディスクリプタ**。
  - `KERNEL32.dll`: `OriginalFirstThunk(RVA)=0x00700474` → INT VA **0xB00474**、
    `FirstThunk(RVA)=0x00628044` → IAT VA **0xA28044**。
  - `USER32.dll`: OFT `0x00700680` → 0xB00680 / FT `0x00628250` → 0xA28250。
  - （他: COMCTL32 / DINPUT8 / WINMM / DSOUND / binkw32 / GDI32 / ADVAPI32 / SHELL32 / ole32 / VERSION / d3dx9_27 / WSOCK32）
- KERNEL32 の INT/IAT を 128 件走査（C2）。**全件で INT 値 == IAT 値**（未結合時の PE の正常状態）。
  - index **71**: 値 `0x00700E56`、名前 `lstrcatA`、hint `0x03BD`
  - index **72**: 値 `0x00700E62`、名前 `lstrcpyA`、hint `0x03C6`
- スロット算術（C3）:
  - `0xA28160 - 0xA28044 = 0x11C` → `0x11C / 4 = 71` ✓（IAT 側）
  - `0xB00590 - 0xB00474 = 0x11C` → 71 ✓（INT 側）
  - したがって **`0xA28160` = IAT[71]**, **`0xB00590` = INT[71]**。両者が同値なのは仕様（INT は名前解決用の控え、FT は解決後アドレス置換用）。
- 名前構造体の実バイト（C4）:

  | VA | バイト | 意味 |
  |---|---|---|
  | 0xB00E54 | `79 00` | `"…Frequency"` の末尾 `y`+NUL（直前エントリの終端） |
  | **0xB00E56** | `bd 03` | **hint = 0x03BD** |
  | **0xB00E58** | `6c 73 74 72 63 61 74 41 00` | `lstrcatA` |
  | 0xB00E60 | `00` | パディング |
  | **0xB00E62** | `c6 03` | **hint = 0x03C6** |
  | **0xB00E64** | `6c 73 74 72 63 70 79 41 00` | `lstrcpyA` |

- 決定的等式（C4 出力）: `0x700E56 == 0xB00E56 - 0x400000` → **True**、`0x700E62 == 0xB00E62 - 0x400000` → **True**。

### §1-2 なぜ「.text の関数先頭でない場所」に見えたか

- `IMAGE_IMPORT_BY_NAME` への参照は PE では **RVA** で持つ。本モジュールの `.text` は VA `0x401000..0xA27C39` に広がるため、
  RVA `0x700E56` を **VA と誤読すると .text の内側**（`0x700E56`）に落ちる。実際にその地点を逆アセすると
  `0x700E56: 0d 00 0c 00 00` = `or eax, 0xC00`（直前の `movzx eax, word ptr [esp+0x3c]` の途中）となり、
  「関数先頭でない .text を指す」という錯覚が生じる（`v248_scanB` B6）。
- 正しい読み方では、`0x700E56` は **`.rdata` の hint/name 構造体**を指す。すなわち
  「IAT 自己解決」「独自名表」といった特殊機構は**存在しない**。通常の by-name インポートである。
- 補強: 全イメージ走査で「値 + 0x400000 が妥当な hint/name 構造体になる dword」は **544 件**（C5）。
  INT と IAT のミラー（2 箇所×272）に相当する規模で、特殊表ではなくインポート表全体の性質であることを示す。

### §1-3 出典 v246 の記述との差分（docs ではなく解析メモの誤り）

- v246 §2-2 / §5-2 N-32 は「`.rdata` 名表 0xB00E59 `lstrcatA`（hint 0x03BD）/ 0xB00E65 `lstrcpyA`（hint 0x03C6）」と書くが、
  実測では **hint は 0xB00E56 / 0xB00E62、名前文字列は 0xB00E58 / 0xB00E64**。v246 の +3 ずれは
  「hint 2B + 名前先頭」の読み違い。N-32 の前提（「名表」という別概念）も本節で置換する。
- 数値自体（hint 0x03BD / 0x03C6、名前 lstrcatA / lstrcpyA、スロット 0xA28160 / 0xA28164）は v246 と一致。

### §1-4 [PORT] 含意

- 移植側で移植対象になるのは **API の意味論**（`lstrcatA` = 連結、`lstrcpyA` = 複写）だけで、IAT の初期値・RVA は再現不要。
- ただし「`__stricmp`/`lstrcatA` 等の Win32 依存を自前実装へ置換する」際、**`lstrcpyA` の上書き（既存バッファ内容を破棄）**と
  **`lstrcatA` の追記**の違いが §2-2 のファイル名正規化の挙動（空文字初期化 → 連結）を支えている点は保持する。
- v246 §2-2 の「初期値が .text を指す」という記述は**移植仕様に持ち込まない**（誤読の伝播防止）。

---

## §2 項目2（N-33）: メニュー・コンテナ 0xB13970 の base 配列を確保する grow/ctor

**判定: 【解明】確保実装は `FUN_004E4A10` = 汎用 `NiTArray<T>::SetSize`（要素 4B 固定、`+4 base` / `+8 maxSize` / `+0xA size` / `+0xC 非NULL件数` / `+0xE growBy` を書く）。0xB13970 に専用 ctor は存在しない。同オブジェクトは .data 内で vptr=0xA69680 と全カウンタ 0 が**イメージに焼き込まれた静的初期化オブジェクト**であり、最初の確保は `FUN_00587B20` が `SetSize(0x33=51)` を呼ぶ時点（0x587B74 / 0x587B92）で起きる。**

### §2-1 コンテナ実体（`v248_scanA` A5 / `v248_scanE` E6）

- .data 実測 `0xB13970`:

  | off | 値 | 解釈 |
  |---|---|---|
  | +0x00 | `0x00A69680` | vptr（＝イメージ内の即値。ctor 不要の証拠） |
  | +0x04 | `0x00000000` | base 配列ポインタ（未確保） |
  | +0x08 | `0x0000` | maxSize (u16) |
  | +0x0A | `0x0000` | size (u16) ← v246 の count |
  | +0x0C | `0x0000` | 非 NULL 要素数 (u16) |
  | +0x0E | `0x0001` | growBy (u16) |

  dword で見ると `0xB1397C = 0x00010000` は「+0xC=0 / +0xE=1」の合成であり、単一 dword フラグではない
  （v246 §5-2 の count=+0xA は正しい。v246 §3 が +0xC を growBy とみなしていた場合は誤り）。
- **.data 全体で dword[0]==0xA69680 のオブジェクトは 0xB13970 ただ 1 個**（E6 走査）。他に同型の確保済み配列はこの時点で存在しない。
- 型情報: vtable[-1] `0xA6967C = 0x00AC140C`（MSVC の COL ポインタ）。COL の type_info `0xAC1418 → 0xB13620`、
  名前文字列は `0xB13628 = ".?AV?$NiTArray@PAVTile@@@@"`（E5）。
  - したがって **0xA69680 は `NiTArray<PAVTile>` の vtable**（RTTI 由来）。`tmp/re/cache/vtable_index.tsv` の該当行
    （id 76, vtable `0x00A69680`, COL `0x00AC140C`）と一致。
  - 「menu コンテナ」という呼称は **v246 の用途推定**であり、RTTI 上の型名は `NiTArray<PAVTile>`。
    要素型名と格納実体（後述 0x585330 が vptr `0xA696DC` = `Menu` を設定する）の対応は本項の範囲外 → **新規残点 N-37**。

### §2-2 grow 本体 FUN_004E4A10（`v248_scanE` E1 全命令）

- シグネチャ: `__thiscall void FUN_004E4A10(this, u16 newSize)`（`ret 4`）。
- 実機械語の流れ:
  1. `0x4E4A17`: `movzx eax, word[esi+8]` / `cmp edx, eax` / `je 0x4E4AF7` — **newSize == maxSize なら即 return**（再確保しない）。
  2. `0x4E4A2A`..`0x4E4A62`: `newSize < size` の縮小経路。`[newSize, size)` の非 NULL 要素を 0 で埋め、
     `add word[esi+0xC], 0xFFFF`（非 NULL 件数を減算）、最後に `mov word[esi+0xA], dx`（size 更新）。
  3. `0x4E4A66`..`0x4E4A8A`: **確保**。`mov edi,[esi+4]`（旧 base 退避）→ `mov word[esi+8], dx`（maxSize 更新）→
     `mov edx,4; mul edx` で `newSize*4` を計算（**要素サイズ 4B 固定 = ポインタ配列**）→ `call 0x401F00`
     （引数 1 個・戻り値ポインタの汎用アロケータ）。
  4. `0x4E4A93`: `mov dword[esi+4], eax` — **base 配列ポインタの唯一の書手**。
  5. `0x4E4A98`..`0x4E4AAF`: 旧 base から `size` 個をコピー。
  6. `0x4E4AB1`..`0x4E4AD4`: `[size, maxSize)` を 0 埋め。
  7. `0x4E4AD6`: `push edi; call 0x401F20` — 旧 base 解放。
- 汎用性の証拠（`v248_scanE` E3 / `v248_scanF` F3）: `call 0x4E4A10` の直接呼出しは **1043 箇所**。スポット確認した 3 箇所
  （0x40C1D6 / 0x4A7C4E / 0x430BD0）はいずれも
  `movzx ecx, word[obj+0xE]`（growBy）→ `add ecx, needed` → `push ecx; mov ecx, obj; call 0x4E4A10` → 続けて
  `push &value; call 0x42BB90`（SetAt）という **SetAtGrow 定型**であり、テンプレートの共有実体であることを示す。

### §2-3 0xB13970 の実際の確保点（`v248_scanE` E2 / `v248_scanF` F4）

- `FUN_00587B20`（呼出し元 **25 箇所**）の実体:
  - `0x587B51: push 1; call 0x401F00` → 1 バイトのマーカ割当。`0x587B58: mov esi,eax`。
  - 成功時 `0x587B6D: push 0x33` / `0x587B6F: mov ecx,0xB13970` / `0x587B74: call 0x4E4A10` → **SetSize(51)**。
  - 失敗時も `0x587B7B: xor esi,esi` で合流し、`0x587B7D: push 0x33` / `0x587B7F: mov ecx,0xB13970` /
    `0x587B92: call 0x4E4A10` → 同じ SetSize(51)。
  - `0x587B8C: mov [0xB3A708], esi`（マーカをグローバルに保存）。
  - ⇒ 実効的には「初期化時に 1 回 SetSize(51)」。base は `0x401F00(51*4 = 0xCC)` で確保され `0xB13974` に格納される。
- 破棄側: `FUN_00A24910`（`0xA24910`）が `mov eax,[0xB13974]; push eax; mov dword[0xB13970],0xA69680; call 0x401F20`
  = **base 解放のみ**（配列の dtor thunk）。同帯の `0xA24930` / `0xA24940` は別オブジェクト（0xB13948 / 0xB1395C）の
  dtor thunk（`mov ecx, imm; jmp ...`）。
- 初期化順序: vptr は **ファイル上の即値**であり ctor が書く必要がない。よって「base を確保する ctor」は存在せず、
  **grow は初回の SetSize 呼出しのみ**。SetAt `FUN_0042BB90` は maxSize を検査せず
  `if (index < size) { 既存要素の差替え } else { size=index+1; base[index]=*value; 非NULL なら +0xC++ }` を実行するだけ
  （`v248_scanF` F1: `0x42BBBC` 側がその既存差替え経路）で、**確保は行わない**。v246 §5-2 N-33 の前提
  （SetAt は確保済み前提）は正しい。

### §2-4 [PORT] 含意

- メニュー ID 表を移植するなら必要なのは (1) `{vptr, base, maxSize, size, numElements, growBy}` の 0x10 バイト構造、
  (2) `SetAt(index, value)` の「size 拡張＋非 NULL 件数維持」、(3) `SetSize(n)` の「newSize==maxSize 早期 return /
  縮小はゼロ埋め＋件数減 / 拡張は再確保＋コピー＋ゼロ埋め＋旧解放」、(4) 初期 `SetSize(51)` と growBy=1、の 4 点。
  アロケータ/解放関数のアドレスは再現不要。
- **注意**: 0xB13970 はポインタ配列（要素 4B）であり、SetAt は値の**コピー**ではなく**ポインタ格納**。NULL を格納すると
  size は伸びるが非 NULL 件数は増えない（`0x42BBC0` の分岐）。移植で `nullptr` を「要素なし」として扱えば同じ意味になる。

---

## §3 項目3（N-34）: FUN_00465130 param_4（bool）の 8B メタ分岐

**判定: 【機構は解明／設計動機は未解明】`param_4 != 0` のとき作られる 8B は「**LoadFormHeader 登録表**」で、中身は
`{ NiTPointerMap<unsigned char formType, BSSimpleList<LoadFormHeader*>*>* ／ BSSimpleList*（(size,name) 記録リスト） }` の 2 ポインタ。
セーブ中に書き出した各フォームの 12B `LoadFormHeader` を `formType` 別リストへ登録し、レコード名＋サイズの一覧も併記する。
console `save` だけがこの表を生成し、quicksave / autosave / メニュー保存は生成しない（＝登録は全て no-op）。
「なぜ console 経路だけ詳細記録モードになるのか」という設計動機は retail バイナリからは確定できない。**

### §3-1 8B の正体（`v248_scanG` G2/G3, `v248_scanH` H1/H4）

- `FUN_0045F0F0(this=8B領域)`（`__thiscall`、`ret`、`0x45F0F0`）:
  - `0x45F114: push 0x10; call 0x401F00` → 16B 確保 → `mov ecx, eax; push 0x25; call 0x45EF00`（ctor、引数 0x25=37）
    → `mov [esi], eax`（8B の +0）。
  - `0x45F13B: push 8; call 0x401F00` → 8B 確保し `[eax]=0 / [eax+4]=0` → `mov [esi+4], eax`（8B の +4）。
  - 戻り値は `esi`（＝渡された 8B 領域）。
- `FUN_0045EF00(this, n)`（`0x45EF00`）: `[esi+4]=n`、`[esi+8]=new(4*n)`、`[esi+0xC]=0`、vptr を
  `0xA3904C` → **`0xA3AA98`** に設定（2 段階で派生 vptr を書く MSVC ctor 定型）。
- **RTTI による型の確定**（`tmp/re/cache/vtable_index.tsv` id 208 / 371）:
  - `0x00A3904C` = `NiTMapBase<V?$NiTPointerAllocator@I@@, E, PAV?$BSSimpleList@PAULoadFormHeader@@@@>`
    （mangled `.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@EPAV?$BSSimpleList@PAULoadFormHeader@@@@@@`, COL `0x00AB7A7C`）
  - `0x00A3AA98` = `NiTPointerMap<unsigned char, BSSimpleList<LoadFormHeader*>*>`
    （mangled `.?AV?$NiTPointerMap@EPAV?$BSSimpleList@PAULoadFormHeader@@@@@@`, COL `0x00AB7DD0`）
  - MSVC の型コード `E` = **unsigned char**。したがってマップのキーは **フォーム型 (0..255)**、値は **LoadFormHeader のリスト**。
- 実体は「`+0` = フォーム型→ヘッダ列マップ（37 バケット）、`+4` = (size, name) を積む単連結リストのダミーヘッド」。

### §3-2 何が登録されるか（`v248_scanH` H1）

- `FUN_0045AD00(this=8B表, ptr=12B構造)`（`0x45AD00`, `ret 4`）:
  1. `push 0xC; call 0x401F00` で 12B ノードを確保し、引数の 12B をコピー（`0x45AD13..0x45AD20` の 3 回の mov）。
  2. `mov cl, byte[esi+4]` → キー = **コピー先 +4 のバイト（フォーム型）**。
  3. `mov ecx, [edi]`（＝表の +0 マップ）; `push &out; push key; call 0x4527A0`（マップの Find）。
  4. 未登録なら `new(8){0,0}` のリストを作り `0x452710`（Insert）→ `push 0x452150; push node; call 0x416650`（リスト追加）。
  - ⇒ 1 フォーム = 12B の `LoadFormHeader` を `map[formType]` のリストへ追加する処理。
- セーブ側の呼出し（`FUN_00465130`）: `0x465461` / `0x46560A` の 2 箇所。直前の分岐はどちらも
  `if (param_1[0x10] != 0)`（decompiled :79689 / :79768）で、12B を作って `FUN_0045AD00(&rec)` を呼ぶ。
  12B の中身は `{ 10B のレコードヘッダ雛形, formID(4B), flags(1B), formType(1B), u16 payload サイズ }` 相当（:79690-79695）。
- `FUN_0045AD00` の呼出しは全 6 箇所: `0x45E02A`, `0x45E0A3`, `0x4614EE`, `0x465461`, `0x46560A`, `0x46685D`
  （セーブ系とロード系の双方）。

### §3-3 分岐の実配線（`v248_scanH` H2/H3）

- セーブ側の生成箇所: `0x4651EC: push 8; call 0x401F00` → `0x465204: call 0x45F0F0`。
- `FUN_0045F0F0` の呼出しは**全 2 箇所のみ**: `0x465204`（セーブ = FUN_00465130）と `0x465A6A`（ロード = FUN_00465860）。
  どちらも直前に `if ((char)param_4 != '\0')` がある（decompiled :79582 / :79985）。⇒ **param_4 は「LoadFormHeader 表を作るか」の唯一のゲート**。
- 破棄: `FUN_004530A0`（`0x4530A0`）= 表の dtor。マップ全バケットを走査して中身を解放し、`+4` のリストも解放。
  呼出し 3 箇所（`0x4657EF` セーブ正常終了 / `0x465AAA` ロード / `0x4669BA` quicksave 系の後始末。後 2 つは表が無ければ no-op）。
- `FUN_004531B0`（`0x4531B0`, 呼出し 28 箇所）= `{u32 size, char* strdup(name)}` を `表+4` のリスト先頭へ挿入する
  「レコード配置ログ」。(size, name) の実例が `FUN_00464AB0` の `FUN_004531B0(iStack_40, "Save Game Header")`（decompiled :79492、
  `iStack_40` は :79372 で `local_59 + 0x24 + (param_2 & 0xff) + fStack_58` と計算された**バイト数**）。

### §3-4 param_4 が真になる条件（v246 §2-2 の再確認）

- console `FUN_005023D0`: `cmp dword[esp0-0x310],0; setne cl` → `push bool`（`0x5024AD`）。`esp0-0x310` は
  引数解析 `FUN_004FAE80` の out 領域。⇒ 正常に引数が 1 個解釈されれば **1**。
- quicksave `FUN_00466AD0` = `push 0`、autosave `FUN_00466B70` = `push 0`、SaveMenu `FUN_005D3230` = `push 0,0,0`。
- ⇒ **console 経路のみ 1**、他は常に 0。したがって console 保存だけが LoadFormHeader 表とレコード配置ログを持つ。

### §3-5 残る謎と [PORT] 含意

- 未解明（**新規残点 N-38**）: なぜ console 経路だけがこの詳細記録を必要とするのか。候補は
  (a) console 保存が「進行中のゲーム状態をそのまま」書くため formType 別索引を必要とする、
  (b) 単なる診断／レイアウト記録（"Save Game Header" 等の名前とサイズのログ）でゲーム進行には不要、
  の 2 系統。retail バイナリには表を**読み出して出力に使う**消費者が見当たらない（登録と破棄のみ）ため、
  (b) の比重が高いと考えるが断定しない。
- [PORT]: 8B 表の再現は必須ではない。ただし移植 API に `bool bDetailedSave`（console 相当）を残し、
  「表が存在する時だけ LoadFormHeader 登録とレコード配置ログを行う」構造にしておけば、後から挙動差を再現できる。
  `LoadFormHeader` の 12B レイアウト（`+0 u32` / `+4 u8 formType` / `+8 u32`、他に flags と u16 サイズ）は
  セーブ形式を読む側で必要になるので、フィールド順だけは保持する。

---

## §4 項目4（N-35）: flags 0x10000 / 0x20000 / 0x30000 / 0x10001 のビット意味

**判定: 【解明】flags は「ビット集合」ではなく **3 フィールドの合成**である。
`+0x10` = u8（bit0 のみ意味を持つ「reference function」フラグ）、`+0x11` = u8（未使用）、
`+0x12` = **u16 の引数スロット数（argc）**。
したがって `0x10000` は bit16 ではなく「argc=1」、`0x20000` は「argc=2」、`0x30000` は「argc=3」、`0x10001` は「argc=1 ＋ reference function」と読むのが正しい。**

### §4-1 実データの分布（`v248_scanI` I1/I2）

131 件の flags は 9 種類しかなく、全て `0x1` または `0x10000` の倍数で説明できる。

| flags | n | 分解 | 代表コマンド |
|---|---|---|---|
| `0x00000000` | 72 | ref=0, argc=0 | ToggleSky / Help / RefreshINI / QuitGame ほか |
| `0x00000001` | 5 | ref=1, argc=0 | ShowVars, ShowAnim, CalcPathToPoint, ShowHeadTrackTarget, ShowPivot |
| `0x00010000` | 33 | ref=0, **argc=1** | Show, ToggleAI, PickRefByID, CenterOnCell, GetINISetting, SetClipDist, ToggleMapMarkers, TestAllCells, SaveWorld, CloseFile |
| `0x00010001` | 1 | ref=1, **argc=1** | AddFaceAnimNote |
| `0x00020000` | 10 | ref=0, **argc=2** | SetGameSetting, SetINISetting, CenterOnExterior, SetFog, **SaveGame**, **LoadGame**, SetTargetRefractionFire, SetSkyParam, ModWaterShader, SetTreeMipmapBias |
| `0x00030000` | 5 | ref=0, **argc=3** | CenterOnWorld, HairTint, WaterShallowColor, WaterDeepColor, WaterReflectionColor |
| `0x00030001` | 2 | ref=1, **argc=3** | CalcLowPathToPoint, SpeakSound |
| `0x00040000` | 2 | ref=0, **argc=4** | SetImageSpaceGlow, SetSTBBColorConstants |
| `0x00060000` | 1 | ref=0, **argc=6** | SetHDRParam |

- `0x10001` が「bit0 + bit16」に見えるのは偶然で、実体は `ref=1, argc=1`。
- v246 §2-4 の「`entry+0x10 != 0` をクエスト外参照の明示要求判定に使う」は **u8 側（ref フラグ）の読み**として正しい。
- 逆に「bit17 = SaveGame 固有のマスク」という読みは誤り。SaveGame は **argc=2** のコマンドの 1 つにすぎない。

### §4-2 ref フラグ（+0x10 の u8）の実消費（`v248_scanJ` J1/J3, `v248_scanL` L1）

- **スクリプト VM 実行部** `FUN_004FAB20`（`0x4FAB20`）: `0x4FADB0: cmp byte ptr [eax+0x10], 0`、
  decompiled :194543 `if (((*(char *)(iVar7 + 0x10) != '\0') && (iVar5 == 0)) || ...) goto LAB_004fadde;`
  ⇒ **ref フラグが立ち、かつターゲット参照 `iVar5` が無い場合はコマンドを実行せずに戻る**。
- **スクリプト・コンパイラ** `FUN_004FDAF0`（`0x4FDAF0`）: `0x4FDC88: cmp byte ptr [eax+0x10], 0`、
  decompiled :197229-197262。ref フラグが立ち、スクリプト側に明示参照が無く、呼出し側にも参照が無い場合
  **"Reference function '%s' requires explicit reference in quest script."** を出して中断する。
- **別系統の実行部** `FUN_005169A0`（`0x5169A0`）: `0x5170C9: cmp byte ptr [eax+0x10], 0` — 同じ判定。
- `[reg+0x10]` に対する byte アクセスは .text 全体で 26 件のみ（`v248_scanL` L2）、そのうちコマンド表エントリに対する
  判定は上記 3 箇所だけ。⇒ **ref フラグは u8 の bit0 としてのみ消費される**。

### §4-3 argc（+0x12 の u16）の実消費（`v248_scanJ` J 出力 0x4FDCB2）

- `0x4FDCB2: movzx ebx, word ptr [eax+0x12]`（decompiled :197237 `uVar1 = *(undefined2 *)((int)ppuVar8 + 0x12);`）
  → その値が **エントリ +0x1C の共通関数 `FUN_004FDE30`** の第 1 引数として渡される。
- `FUN_004FDE30`（`0x4FDE30`）の実機械語:
  1. `0x4FDE73..0x4FDE84`: `mov ax, word[esp+0x240]`（= argc）→
     `mov word ptr [ecx], ax` / `add dword ptr [esi+0x40c], 2`
     — **argc をコンパイル済みスクリプトの命令ストリームへ u16 として書き出す**（esi = コンパイラ文脈、+0x40C は出力位置）。
  2. `0x4FDE8B..0x4FDE9D`: `movzx eax, ax; cmp eax, 0; jle 0x4FEC56` — **argc が 0 なら引数検査を省略**。
  3. `0x4FDEA9..0x4FDEB0`: `lea ecx,[eax+eax*2]`（×3 = 12B ストライド）→
     `mov eax, [edx + ecx*4 + 4]` → `mov bl, byte[eax*8 + 0xB0A54C]`
     — **ParamInfo[index] の +4（kind）** を読み、kind 別属性表 `0xB0A54C` を引いて 1 引数ずつ型検査する。
  4. 以降 `argc` 回ループして引数トークンを解析（`FUN_004FD7C0` を再帰呼出し）。
  ⇒ **u16 は「このコマンドが消費する ParamInfo スロット数」＝ argc**。v246 §2-3 の ParamInfo 解釈と整合。
- 値の妥当性: HairTint=3（Integer×3 = RGB, help "3 ints, RGB"）✓、CenterOnWorld=3（world + x + y）✓、
  SetGameSetting/SetINISetting=2（設定名 + 値）✓、CenterOnExterior=2（x,y）✓、SetFog=2 ✓。
- **SaveGame = argc 2** の含意: ParamInfo は `0xB0A678 = {'String',0,0}` の直後が
  `0xB0A684 = {'Integer (Optional)',1,1}`（v246 §2-3 で実測済み）。したがって **`save <name> [integer]` の
  2 引数形が構文上は許容される**。2 番目の整数が何を意味するかは未確定 → **新規残点 N-39**。
  （console 実行時の `FUN_004FAE80` は入力トークン数で打ち切るため、`save foo` だけでも成立する。）

### §4-4 [PORT] 含意

- コマンド表は `{..., u32 flags, ParamInfo*, handler, void (*compile)(u16 argc, ParamInfo*, ...)}` として実装し、
  flags は **`ref`(bool) と `argc`(u16) に分解して保持**するのが正しい。`0x20000` 等の生値比較を移植コードに残さない。
- `save` の移植仕様は「argc=2 / 1 個目は必須 String / 2 個目は任意 Integer」。2 個目を無視しても
  ゲーム内 console の通常運用（`save foo`）は一致する。
- ref フラグの 2 段ガード（VM 側の実行時チェックとコンパイラ側の静的エラー）は、
  移植でコマンドをスクリプトから呼べるようにする場合に両方必要。

---

## §5 項目5（N-36）: vtable 0xA69680 の slot+4（0xAC1454）/ slot+0x1C（0x982D85）

**判定: 【解明】どちらも `NiTArray<PAVTile>` の vtable のスロットではない。`0xA69680` の vtable は
**スロット 1 個だけ**（scalar deleting destructor `0x5844D0`）で、その直後から隣接 vtable 群が始まる。
`0xA69684` の `0xAC1454` は**隣の vtable（0xA69688）の COL（Complete Object Locator）**、
`0xA6969C`/`0xA696A0` の `0x982D85` は**その隣の vtable の関数スロット**である。
つまり v246 N-36 の「slot+4 / slot+0x1C」は vtable 境界を越えた読み違い。**

### §5-1 レイアウトの実測（`v248_scanM` M1/M2）

| .rdata | 値 | 実体 |
|---|---|---|
| 0xA6967C | `0x00AC140C` | **COL_A**（vtable 0xA69680 の直前） |
| 0xA69680 | `0x005844D0` | vtable_A slot0 = scalar deleting dtor |
| 0xA69684 | `0x00AC1454` | **COL_B**（vtable 0xA69688 の直前） |
| 0xA69688 | `0x00584470` | vtable_B slot0 = dtor |
| 0xA6968C | `0x006A9060` | vtable_B slot1 = hash（`key % [ecx+4]`） |
| 0xA69690 | `0x00763E80` | vtable_B slot2 = key 等値 |
| 0xA69694 | `0x0067F130` | vtable_B slot3（3 引数の setter 形、`ret 0xC`） |
| 0xA69698 | `0x0068F970` | vtable_B slot4（`ret 4` の no-op 形） |
| 0xA6969C | `0x00982D85` | 隣接領域の関数ポインタ（**vtable_A の slot ではない**） |
| 0xA696A0 | `0x00982D85` | 同上（2 連） |
| 0xA696A4 | `0x00AC149C` | **COL_C**（vtable 0xA696A8 の直前） |
| 0xA696A8 | `0x005844A0` | vtable_C slot0 = dtor |

- COL のデコード（M2、MSVC x86 形式 `{signature, offset, cdOffset, pTypeDescriptor, pClassDescriptor}`）:

  | COL | pTypeDescriptor | 型名（type_info +8 の文字列） |
  |---|---|---|
  | `0xAC140C` | `0xB13620` | `.?AV?$NiTArray@PAVTile@@@@` = **NiTArray\<PAVTile\>** |
  | `0xAC1454` | `0xB13648` | `.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PBDPAVXMLStorage@@@@` = **NiTMapBase\<NiTPointerAllocator\<unsigned int\>, char const *, XMLStorage *\>** |
  | `0xAC149C` | `0xB13690` | `.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PBDPAVBuildStorage@Tile@@@@` = **NiTMapBase\<…, char const *, BuildStorage::Tile *\>** |

- ⇒ **`0xA69680` は 1 スロット vtable**。`0xA69684` の `0xAC1454` は「vtable_B の RTTI 前置詞」であり、
  関数ポインタですらない（vtable[-1] の COL を slot1 と誤読したもの）。
- `0xA69680` の唯一のスロットの実体（M4a, `0x5844D0`）:
  `mov eax,[esi+4]; push eax; mov dword[esi],0xA69680; call 0x401F20`（base 解放）→
  `test byte[esp+8],1` で `delete` 判定 → `ret 4`。**virtual dtor のみ**を持つ `NiTArray<T>` の形。
- 隣の `0x584470`（M4b）: `mov dword[esi],0xA69688; call 0x7C2A40; push [esi+8]; call 0x401F20` — マップの dtor
  （要素破棄＋バケット配列解放）。vptr が `0xA69688` である点で vtable_A の dtor と明確に別物。
- §2 の結論（0xB13970 の vptr = `NiTArray<PAVTile>`）は維持される。ただし **その vtable に
  hash/keyeq 等は無い**（v246 §3 が 0xA69680 を多スロット表として扱っていた場合は要訂正）。

### §5-2 0x982D85 の実体（`v248_scanM` M3）

- `0x982D85` は `.text` 後半（CRT/ライブラリ帯 `0x98xxxx`）の**関数先頭**。直前 `0x982D84 = 0xC3`（前関数の ret）。
- 逆アセンブル:
  `push dword ptr [0xBAA5EC]` → `call 0x98BEEB` → `test eax,eax` → `je +2` → `call eax` →
  `push 0x19` → `call 0x98D5B8` → `push 1 / push 0` → `call 0x98BD5E` → `jmp 0x98BC6B`。
  「登録ハンドラ（`[0xBAA5EC]`）があれば呼び、引数 0x19 で異常終了経路へ落ちる」という CRT の定型。
- Ghidra の FID は `__access` と同定（`FID_conflict:__access`）しているが、命令形は `__access` ではなく
  **CRT のエラー／アボート系スタブ**と見るのが妥当（同定は推定。ただし「`NiTArray<PAVTile>` の
  slot ではない」ことは構造から確定）。
- これが 2 連で並ぶ（`0xA6969C` / `0xA696A0`）のは、隣接 vtable の末尾 2 スロットが同じスタブを共有しているため、
  または直後に独立 2 スロット vtable が置かれているため。いずれにせよ vtable_A の範囲外。

### §5-3 [PORT] 含意

- `0xB13970` の型は `NiTArray<PAVTile>`（1 仮想関数 = 仮想 dtor）として実装すればよい。
  `0xA69680+4` や `+0x1C` を「コンテナの仮想関数」として呼び出す実装は**誤り**（実バイナリでも呼ばれない）。
- v246 N-36 の「slot+4 が .rdata を指すのは謎」は、**vtable 境界の読み違い**として閉じる。
- 同型の `NiTMapBase<…, char const*, T*>` vtable（`0xA69688` / `0xA696A8`）は 0xB13948 / 0xB1395C の
  オブジェクトが使う「文字列→ポインタ」マップの実体。セーブ経路では未使用（v246 §3 の結論どおり）。

---

## §6 まとめ・新規残点・自己検証

### §6-1 N-32..N-36 の裁定まとめ

| ID | 判定 | 要点 | 実装要否 |
|---|---|---|---|
| N-32 | **解明** | 0x700E56/0x700E62 は **RVA**（`VA - 0x400000`）。指す先は `.rdata` の `IMAGE_IMPORT_BY_NAME` `0xB00E56`/`0xB00E62`（hint 0x03BD/0x03C6）。`0xA28160` = KERNEL32 IAT[71]、`0xB00590` = INT[71]（同一 index のミラー） | 不要 |
| N-33 | **解明** | base 確保は `FUN_004E4A10`（`NiTArray<T>::SetSize`、要素 4B）。専用 ctor は無く、`FUN_00587B20` の `SetSize(0x33=51)`（0x587B74 / 0x587B92）が初回確保。SetAt `FUN_0042BB90` は確保しない | 要（配列セマンティクス） |
| N-34 | 機構は解明／動機は未解明 | `param_4!=0` で 8B = `{NiTPointerMap<u8 formType, BSSimpleList<LoadFormHeader*>*>*, BSSimpleList*}` を生成。`FUN_0045AD00` が 12B `LoadFormHeader` を formType 別に登録。console 経路のみ有効 | 任意（`bDetailedSave` として温存） |
| N-35 | **解明** | flags は `{u8 ref フラグ(+0x10), u8 pad(+0x11), u16 argc(+0x12)}`。`0x10000`=argc1 / `0x20000`=argc2 / `0x30000`=argc3 / `0x10001`=ref+argc1 | 要（SaveGame は argc=2） |
| N-36 | **解明** | `0xA69680` は 1 スロット vtable（`NiTArray<PAVTile>` の仮想 dtor のみ）。`+4` の `0xAC1454` は隣接 vtable `0xA69688` の **COL**、`+0x1C` の `0x982D85` はその隣接 vtable の関数スロット（CRT スタブ） | 不要（誤読の解消） |

### §6-2 新規残点（本タスクで発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-37 | `0xB13970` の vptr は RTTI 上 `NiTArray<PAVTile>` だが、登録側 `FUN_00585330` は vptr `0xA696DC`（RTTI `Menu`）のオブジェクトを渡す。要素型名と実際に格納されるオブジェクト型の対応が未確定（`PAVTile` が Menu の基底/別名か、RTTI がテンプレート共有によるものか） | 低 |
| N-38 | console `save` だけが `LoadFormHeader` 登録表（8B）を生成する設計動機。表を**読み出して出力に使う**消費者が retail バイナリに見当たらず、登録と破棄のみ | 低 |
| N-39 | `save <name> [integer]` の第 2 引数（`0xB0A684 = {'Integer (Optional)',1,1}`）の意味。コンパイラは argc=2 として受理するが、console 実行時はトークン数で打ち切られる | 低 |
| N-40 | `0x982D85` の正確な関数同定（CRT スタブ。Ghidra FID は `__access` と衝突同定）。`0xA6969C`/`0xA696A0` の 2 連の意味（vtable 末尾共有か独立 2 スロット vtable か） | 低 |

### §6-3 自己検証

- **selftest**: 着手時と完了時の 2 回実行し、いずれも **ALL PASS (4/4)**（`probe_harness.py selftest`）。
- **手法**: PE 手動解析（セクション表・インポートディレクトリ・RVA/VA 変換）＋ capstone 5.0.7 逆アセンブル＋
  `docs/Oblivion_decompiled.c` の部分読取＋ `tmp/re/cache/` 突合。ESM 本体は未アクセス。git 不使用。
- **v246 記述の訂正点（3 件）**:
  1. N-32: 「静的初期値が .text を指す」→ **RVA の読み違い**。名表の位置も 0xB00E59/0xB00E65 ではなく
     hint が 0xB00E56/0xB00E62、名前が 0xB00E58/0xB00E64。
  2. N-36: 「vtable の slot+4 / slot+0x1C」→ **隣接 vtable の COL / 関数スロット**。`0xA69680` は 1 スロット。
  3. N-35: 「flags のビット分解」→ **ビットではなく u8 + u16 の 2 フィールド合成**（argc）。
- **機械語アンカー（再現可能）**:
  - `A1 60 81 A2 00`（lstrcatA IAT 読み）系は既存 v246 のまま。新規は
    `0xB00590 = 0xB00474 + 0x11C`、`0xA28160 = 0xA28044 + 0x11C`、
    `BD 03 6C 73 74 72 63 61 74 41` @0xB00E56（hint+名前）。
  - `8B 41 04 / 89 41 04` 系は §2 の SetSize。`C7 06 80 96 A6 00` @0x5844D7（vptr A）、
    `C7 06 88 96 A6 00` @0x584473（vptr B）、`C7 06 DC 96 A6 00` @0x58533D（Menu vptr）。
  - `80 78 10 00`（`cmp byte [eax+0x10],0`）@0x4FADB0 / 0x4FDC88 / 0x5170C9。
  - `0F B7 58 12`（`movzx ebx, word [eax+0x12]`）@0x4FDCB2。
  - `66 89 01 / 81 86 0C 04 00 00 02`（argc の u16 書き出し）@0x4FDE81 / 0x4FDE84。

### §6-4 使用証拠一覧（すべて `%TEMP%\opencode\` 配下、`_out.txt` 付き）

- `v248_scanA.py`（PE セクション表 / IAT 周辺 / 0xB13970 構造 / vtable 周辺 / 0xAC14xx / 0x982Dxx のダンプ）
- `v248_scanB.py`（インポートディレクトリ 14 件 / 2 つのポインタ配列と hint-name 対応 / 0xB00DB0 ダンプ）
- `v248_scanC.py`（KERNEL32 INT/IAT 128 件走査・index 71/72 の確定・`0x700E56 = 0xB00E56 - 0x400000` の等式検証）
- `v248_scanD.py`（SetAt `FUN_0042BB90` 全命令 / `0xA24910` dtor thunk / `FUN_004E4A10` head / vtable 0xA696DC）
- `v248_scanE.py`（`FUN_004E4A10` 全命令・`0x4E4A10` 呼出し 1043 件 / `FUN_00587B20` 実体 / RTTI デコード / .data 走査）
- `v248_scanF.py`（SetAt の既存差替え経路 0x42BBBC / SetSize 呼出しスポット 3 件 / `FUN_00587B20` 呼出し 25 件）
- `v248_scanG.py`（`FUN_00465130` param_4 分岐 / `FUN_0045F0F0` / `FUN_0045EF00` / `FUN_004531B0` / `FUN_004530A0`）
- `v248_scanH.py`（`FUN_0045AD00` の LoadFormHeader 登録 / `push 8;call 0x401F00` 位置 / 呼出し数 / vtable 0xA3AA98）
- `v248_scanI.py`（131 件の flags 全件ダンプ・ヒストグラム / `FUN_004FCA30` 呼出し 3 件 / `[reg+0x10]` 直値走査）
- `v248_scanJ.py`（ディスパッチャ・VM の ref 判定 / `FUN_004FDAF0` head / 0x5168EA・0x51703E 文脈）
- `v248_scanK.py`（0x4E0000..0x540000 の直値・dword 読み走査＝該当 0 件の負証拠）
- `v248_scanL.py`（コマンド表消費関数の `[reg+0x10..0x13]` 全走査 / byte アクセス 26 件の全列挙）
- `v248_scanM.py`（0xA69670..0xA696C0 ダンプ / COL 3 件のデコードと型名 / 0x982D85 / 0x5844D0 / 0x584470）
- 入力（読取のみ）: `tmp/re/v246_port_saveslot_residuals2.md`, `tmp/re/v244_port_saveslot_residuals.md`,
  `tmp/re/cache/vtable_index.tsv` / `named_funcs.txt`, `docs/Oblivion_decompiled.c`
  （:79217-79497 FUN_00464AB0 / :79501-79640 FUN_00465130 / :79689-79770 登録呼出し /
  :79960-80060 ロード側 / :194470-194575 スクリプト VM / :197185-197265 コンパイラ）
- decompiled 突合: `FUN_0045F0F0` :75100-75140 / `FUN_004531B0` :64020-64064。

### §6-5 不変条件の再確認

- ESM 本体は未アクセス。git 不使用。**docs/GT/TODO/session-memory/plan/cache/ソースの編集なし**。
- 既存 `tmp/re` の上書きなし。**新規に書いたのは本ファイル（v248）のみ**。
- スクリプトは `%TEMP%\opencode\v248_*.py` の新規 13 本のみ（v246 既存スクリプトは読取参照）。
