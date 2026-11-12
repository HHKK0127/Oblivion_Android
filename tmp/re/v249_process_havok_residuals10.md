# v249_process_havok_residuals10.md — process/Havok 残点第10束（V-247-10）

- 対象: `tmp/re/v247_process_havok_residuals9.md` §6-2 の新規残点 5 件
- 入力（読取専用）: `v247_process_havok_residuals9.md` §6-2 / `v245_process_havok_residuals8.md` §4〜§8 / `v243_process_havok_residuals7.md` §4〜§5 / `tmp/re/cache/{named_funcs,vtable_index,call_targets}` / `docs/Oblivion_decompiled.c`
- 環境: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7、VA=off+0x400C00/.text、0x400E00/.rdata、0x401400/.data、bss=None）、Havok 帯 `0x84xxxx–0xA2xxxx`
- selftest: 着手時 `probe_harness.py selftest` = **4/4 PASS**
- 方針: 各件を独立に「解明 / 部分解明 / 未解明」で裁定し、**「確認できず（探索したが見つからない）」と「不存在証明（全数走査で 0 件＝構造上あり得ない）」を区別**して書く。外部資料は Havok 5.1.0r1 SDK 公開ソース（`sigmaco/havok-v5.1.0r1-b2007.09.19`）を一次資料として使う。

> 注記（版の扱い）: 本 EXE は `Havok-x.y` 版文字列を 0 件しか持たないため版を静的に確定できない（v247 §2）。以下の外部照合は **Havok 5.1.0r1 (2007.09.19)** の公開ヘッダ/実装を基準にし、「5.1 に存在する＝4.0 にも存在する」とはみなさない。4.0 固有の断定はしない。

---

## §1 item4（V-247-10-4）map owner `FUN_00943070` の動的生成元 — **解明: 生成元は「hk アロケータ経由の遅延シングルトン（lazy singleton）」。`0x943210` が factory thunk（alloc 0x14/tag 0x15 → `jmp 0x943070`）で、静的登録ノード `0xB30598` → slot `0xBA94BC` に格納される**

### 証拠A: `0x943070` は直接呼ばれず、`0x943210` が唯一の入口（`v249_r1_out.txt` / `v249_r6_out.txt` / `v249_r10_out.txt`）

- `0x943070`（本体 ctor、終端 `0x943165` `ret`）への **e8 = 0 件 / imm32 = 0 件**（v247 §5 の再測定と一致）。
- 直後 `0x943210` が **factory thunk**:
  ```
  0x943210  mov  ecx,[0xBA7D98]        ; hk メモリマネージャ singleton
  0x943216  mov  eax,[ecx]
  0x943218  push 0x15                  ; tag = HK_MEMORY_CLASS_?
  0x94321A  push 0x14                  ; size = 20
  0x94321C  call [eax+0x10]            ; allocator vtable slot4 = alloc(size, tag)
  0x94321F  mov  word [eax+4],0x14     ; hkReferencedObject: +4 = u16 size
  0x943225  mov  ecx,eax
  0x943227  jmp  0x943070              ; tail-call ctor（ecx=this）
  ```
  ＝「確保 → ctor」だけを行う 1 引数なしファクトリ。**placement new ではない**（`operator new` 相当の確保は hk アロケータ vtable 呼出し）。
- `0x943210` の imm32 参照は **`.data@0xB30598` の 1 件のみ**（`v249_r6_out.txt` (d)）。

### 証拠B: `0xB30598` は「遅延シングルトン登録ノード」で、walker が `call [node]` → `*[node+8]` に格納する（`v249_r10_out.txt` (a) / `v249_r8_out.txt`）

- 登録コード（`0xA15980`）は C++ 静的初期化の定型:
  ```
  0x A15980  mov eax,[0xBA8188]     ; list head
  0xA15985   mov [0xB3059C],eax     ; node->next = head
  0xA1598A   mov [0xBA8188],0xB30598; head = &node
  ```
- 消費側 walker `0x8BB420`（`v249_r9_out.txt`）:
  ```
  0x8BB425  mov esi,[0xBA8188]
  0x8BB448  mov eax,[esi+8]     ; slotptr
  0x8BB44B  cmp [eax],0         ; 未生成か
  0x8BB44F  call [esi]          ; create()
  0x8BB459  mov ecx,[esi+8] / mov [ecx],eax   ; *slotptr = instance
  0x8BB45E  lea edi,[esi+4] / mov esi,[edi]   ; next
  ```
  → **ノード = {create, next, slotptr} の 12 B**、生成は「初回参照時」。
- 登録ノード全 9 件（`v249_r10_out.txt` (a)）:

| node | create | slotptr | 生成物 |
|---|---|---|---|
| `0xB2EB0C` | `0x8A7AD0` | `0xBA7DA0` | （別系 singleton） |
| `0xB2F1B4` | `0x8BB530` | `0xBA803C` | （別系） |
| `0xB30064` | `0x91CF20` | `0xBA8450` | （別系） |
| `0xB30118` | `0x91F6B0` | `0xBA84F4` | （別系） |
| **`0xB30598`** | **`0x943210`** | **`0xBA94BC`** | **map owner（本件）** |
| `0xB305A4` | `0x945BD0` | `0xBA94E8` | node2（クロック、§3） |
| `0xB305B8` | `0x947C00` | `0xBA950C` | node3（プロセスファクトリ、§3） |
| `0xB305E8` | `0x94A5F0` | `0xBA9510` | node4（§5） |
| `0xB305F4` | `0x94A5F0` | `0xBA9514` | node5（§5） |

- 全 slot の初期値は 0（.data 末尾の bss 相当域）。walker の呼出し元は `0x8BBB5F` の 1 件（`v249_r10_out.txt` (b)）。

### 証拠C: owner の実体と利用面（`v249_r10_out.txt` (c) / `v249_r1_out.txt` (e)）

- slot `0xBA94BC` の参照は **2 件のみ**: `.data@0xB305A0`（ノードの slotptr）と `.text@0x940D69`（利用側）。
- owner vtable = `0xAA2768`: slot0 `0x943170`（dtor）/ slot1 `0x68F970`（purecall 枠）/ slot2 `0x943410`（値アクセサ）/ slot3 `0x452A60` / slot4 `0x780D10` / slot5 `0x942ED0`。
- ctor `0x943070` は `+8`/`+0xC`/`+0x10` に 3 つの map（vt `0xAA2738`/`0xAA2748`/`0xAA2758`）を生成し、`0x942F20`/`0x942F50`/`0x942F70`/`0x942F90`/`0x942FC0`/`0x942FD0` は **すべて vtable slot にしか現れない**（e8 = 0、`v249_r6_out.txt` (d)）。

### 裁定

> - **生成元 = 遅延シングルトン機構**（確度: **高**）。`0xB30598`（静的登録ノード）→ walker `0x8BB420` が初回に `call [node]` = `0x943210` → hk アロケータ `alloc(0x14, tag 0x15)` → ctor `0x943070` → 戻り値を `[0xBA94BC]` に格納。
> - 「e8 0 件・関数ポインタ表にも無い」の理由は**間接呼出しだから**（`call [esi]`）。直接参照が無いことは「デッド」でも「動的 new」でもなく、**未生成時のみ生成する遅延初期化**の正常形。
> - v247 §5 の「動的生成の可能性」は **確定**（ただし placement new ではなく hk allocator 経由）。owner の利用は 12 caller と `0x940D69` に閉じる点も v247 と整合。
> - 残（低）: ノード登録関数 `0xA15980` 自体の起動順（CRT 初期化列のどこか）は未特定。ただし登録は静的初期化チェーン上で、walker 呼出し元 1 件（`0x8BBB5F`）が実体。

**判定: 解明**（証拠 `v249_r1_out.txt` / `v249_r6_out.txt` / `v249_r7_out.txt` / `v249_r8_out.txt` / `v249_r9_out.txt` / `v249_r10_out.txt`）

---

## §2 item5（V-247-10-5）node4/node5 の vtable slot2..4 が定数（`0x34`/`FLT_MAX`/π）である理由と実 slot 数 — **解明: それらは vtable slot ではない。実 slot 数は 2（dtor + purecall 枠）。3 語は vtable 間に並置された未参照の静的定数で、呼出し可能な slot ではあり得ない**

### 証拠A: node4/node5 の実体は「hkArray 2 語 + vptr」の 0x14 B オブジェクト（`v249_r5_out.txt` (a) / `v249_r11_out.txt` (e)）

- ctor `0x94A5F0`（node4/node5 共通）:
  ```
  0x94A5F0  mov ecx,[0xBA7D98] / mov eax,[ecx]
  0x94A5F8  push 0x15 / push 0x14 / call [eax+0x10]   ; alloc(0x14, tag 0x15)
  0x94A5FF  mov word [eax+4],0x14                      ; u16 size = 20
  0x94A605  mov word [eax+6],1                         ; u16 refcount = 1
  0x94A60B  mov dword [eax],0xAA2BD0                   ; vptr
  0x94A613  mov [eax+8],0 / mov [eax+0xC],0            ; hkArray data / size
  0x94A619  mov [eax+0x10],0x80000000                  ; hkArray flags（static storage）
  ```
- 実使用は dispatcher の default 分岐のみ（`v249_r2_out.txt` (a)(b)）:
  `mov eax,[0xBA9510]; mov ecx,[eax+0xC]; mov eax,[eax+8]; lea esi,[eax+ecx*8]` → **8 B エントリ {handler, id} の線形走査**。`[obj]`（vptr）を介した仮想呼出しは **EXE 全体で 0 件**。

### 証拠B: vtable `0xAA2BD0` は 2 エントリで閉じる（`v249_r3_out.txt` (a)(b) / `v249_r5_out.txt` (b)）

| addr | 値 | 判定 |
|---|---|---|
| `0xAA2BD0` | `0x94BBC0` | slot0 = scalar deleting dtor（`0x94BBF0` で hkArray 解放 → 条件付き free） |
| `0xAA2BD4` | `0x68F970` | slot1 = purecall 枠（未実装仮想のプレースホルダ） |
| `0xAA2BD8` | `0x34000000` | **コードでない**（0x34 = 52） |
| `0xAA2BDC` | `0x7F7FFFFF` | **FLT_MAX**（コードでない） |
| `0xAA2BE0` | `0x40490FDB` | **π**（コードでない） |
| `0xAA2BE4` | `0x94B880` | 別クラスの vtable 先頭（slot0） |

- `0xAA2BD0` の imm32 参照は **ctor `0x94A60B` の 1 件のみ**。`0xAA2BD8`/`0xAA2BDC`/`0xAA2BE0` の imm32 参照は **全セクションで 0 件**（`v249_r3_out.txt` (b)）。
- 同一の 3 語組は帯内に 2 回反復する（`0xAA2BC4..0xAA2BCC` と `0xAA2BD8..0xAA2BE0`。`0xAA2BFC`/`0xAA2C00` にも部分反復）＝ vtable の直後に**同じ静的定数ブロックがリンカ順で並置**された形。
- 参考: `.rdata` の `FLT_MAX` は 350 箇所（`.text` 155 + `.rdata` 195）、π は 230 箇所に出現する一般的な静的定数で、本 3 語組は特異な構成ではない（`v249_const_out.txt`）。

### 証拠C: 「slot2..4 が定数」は読み違いであることの機械证明

- 仮に slot2..4 なら実行時に `call [vptr+8]` で `0x34`（NULL ページ近傍）へ飛ぶ＝**即クラッシュ**。本 EXE の node4/node5 は初期化後ただちに dispatcher から参照される（`0x94B71A`/`0x94CB86`）ため、そのような vtable は成立しない。
- 実際に node4/node5 に対して発行される操作は `+8`/`+0xC`（hkArray）の読みのみで、**vptr 経由の呼出しは 0 件**。vtable は dtor のためだけに存在する。
- → **実 slot 数 = 2**（slot0 dtor、slot1 purecall 枠）。3 語は **slot ではない**。

### 裁定

> - **実 slot 数 = 2**（確度: **高**）。`0x34`/`FLT_MAX`/π は vtable slot ではなく、vtable 間に並置された**未参照の静的定数ブロック**（`0x34`=52、`FLT_MAX`、π の 3 語組が 2 回反復）。
> - 値の由来（確度: **低〜中**・推定）: 近接する `.rdata` 帯 `0xAA2A60..0xAA2AE4` は `HK_DISPLAY_PLANE` / `HK_DISPLAY_CONVEX` / `HK_DISPLAY_SEMICIRCLE` / `HK_DISPLAY_CONE` / `HK_DISPLAY_AABB` / `HK_DISPLAY_BOX` / `HK_DISPLAY_CAPSULE` / `HK_DISPLAY_SPHERE` の文字列群（Havok の表示ジオメトリ型名）で、`.data@0xB305C4..0xB305E4` に 9 個のポインタとして連続する（`v249_r3_out.txt` (c) / `v249_r5_out.txt` (a)。配列自体への imm32 参照は 0 件＝`v249_r12_out.txt` (f)）。このことから 3 語は**表示ジオメトリ系クラスの既定値**（頂点/プリミティブ上限 52・無限大の広がり FLT_MAX・半回転 π）と推定する。ただし**直接参照が 0 件**のため「この EXE のコードがその意味で使っている」ことの証明にはならない。
> - v245 §5(3) の「slot2..4 は vtable ではなく定数と推定」を **機械面で確定**（推定→確定）。レイアウトの完全解明（どのクラスの静的メンバか）は未了。

**判定: 解明（実 slot 数 2・定数は slot でない）＋定数の意味は推定**（証拠 `v249_r3_out.txt` / `v249_r5_out.txt` / `v249_r11_out.txt` / `v249_const_out.txt`）

---

## §3 item1（V-247-10-1）`__classindex__`/`__dataindex__` が Havok 4.0 期の正式名か＋B 私有リテラル `0xA9C960/70/80` の妥当性 — **部分解明: `__classnames__` は SDK 正式名（確定）。`__classindex__`/`__dataindex__` は公開 SDK（5.1）に不在・Web 完全一致 0 件＝「公開名ではない」は確定。4.0 期の正式名だった可能性は棄却できない（不存在証明にはならない）。リテラルは SectionTag として妥当**

### 証拠A: SDK 5.1 の公式セクションタグは 3 個で、その中に classindex/dataindex は無い（一次資料）

- `Source/Common/Serialize/Packfile/hkPackfileWriter.h`（Havok 5.1.0r1, BUILD #20070919）:
  ```cpp
  typedef const char* SectionTag;
  static SectionTag SECTION_TAG_DATA;   // "The default section for normal object data."
  static SectionTag SECTION_TAG_TYPES;  // "The default section for hkClass instances."
  virtual void addSection(SectionTag sectionTag);
  ```
- `Source/Common/Serialize/Packfile/Binary/hkBinaryPackfileWriter.h`:
  ```cpp
  class hkBinaryPackfileWriter : public hkPackfileWriter {
      static SectionTag SECTION_TAG_CLASSNAMES;  // "Section to contain names of types requiring finishing."
  ```
- → **公開 SDK の section tag は `__classnames__` / `__types__` / `__data__` の 3 個**で、`__classindex__` / `__dataindex__` という定義・言及は**ヘッダ群に存在しない**（`hkPackfileWriter.h` / `hkBinaryPackfileWriter.h` を全文確認）。これは「探索して見つからない」ではなく、**公開 API のタグ定義が 3 個で閉じている**という構造的負証拠。

### 証拠B: 本 EXE の writer は 5.1 より前の形式（version=2）＝「5.1 に無いから 4.0 にも無い」とは言えない

- `Source/Common/Serialize/Packfile/Binary/hkPackfileHeader.h`（5.1）:
  ```cpp
  hkInt32 m_magic[2];      // 0x57e0e057 / 0x10c0c010
  hkInt32 m_userTag;
  hkInt32 m_fileVersion;   // "Binary file version. Currently 4."
  ```
- 本 EXE の書出しは `0x90DA7B: mov dword [esp+0x58], 2`（= header+0xC = `m_fileVersion`）で **version = 2**、magic は 5.1 と同一（`0x57E0E057`/`0x10C0C010`、`v249_r13_out.txt` (d)）。
- → **同一 magic・異なる version**。5.1 が 4 を書くのに対し本 EXE は 2 を書くため、**本 EXE の writer は 5.1 より古い世代**（Havok 4.0 期と整合）。したがって 5.1 のタグ名 3 個を 4.0 にそのまま外挿できない。v247 §2 裁定 (ii)（旧命名説）を**棄却できない**理由が、version 差という機械証拠で補強された。

### 証拠C: リテラルの配置と参照（v247 §2 の再確認・`v249_r1_out.txt`）

- `0xA9C960`（`__dataindex__`）/ `0xA9C970`（`__classindex__`）/ `0xA9C980`（`__classnames__`）は **`0xA9C960..0xA9C98E` に連続**し、直後 `0xA9C990` が B の vtable。
- 各文字列の imm32 参照は **`.data@0xB2FF04/08/0C` の 1 件ずつ**で、読手は **B ctor `0x90D7D0` のみ**（`0x90D7DF/0x90D7ED/0x90D7FB` が `0x941140` = `addSection` を 3 回呼ぶ）。EXE 内に消費者 0。
- 妥当性: `hkPackfileSectionHeader::m_sectionName` は 20 B 固定（5.1 ヘッダ。`Source/Common/Serialize/Packfile/Binary/hkPackfileSectionHeader.h`）。3 名は 14/14/13 文字で**収まる**。重複なし・`addSection` の契約（短い文字列）を満たすため、**SectionTag としての妥当性は機能面で成立**。

### 証拠D: Web（完全一致）

- `"__classindex__" havok`（DuckDuckGo）: **0 件**。
- `"__dataindex__"`（DuckDuckGo / Bing 両方、havok 語なしの完全一致）: **0 件**。
- `"__classnames__"` は hkx 解説（reshax / lukascone / HedgeLib）で標準名として流通。
- → `__classnames__` のみが公開名。classindex/dataindex は**公開資料に存在しない**（ただし「Web に無い」＝「SDK に無い」ではない。SDK については証拠A が独立に負証拠）。

### 裁定

> - **`__classnames__` = SDK 正式名（`SECTION_TAG_CLASSNAMES`）で確定**（確度: 高）。
> - **`__classindex__` / `__dataindex__` = 公開 SDK・公開資料のいずれにも存在しない非公開名**（確度: 高）。B ctor だけが参照する B 私有リテラルである点は v247 の裁定を維持。
> - **「Havok 4.0 期の正式名か」= 未証明**（確度: 低のまま）。理由: (a) 本 EXE の version=2 は 5.1(version=4) より古く、旧世代での改名は**あり得る**、(b) しかし 4.0 の SDK/ドキュメントは公開されておらず、名前の裏付けが 0。**「存在しない証明」ではなく「確認できず」**。
> - 傍証の偏り: `__classnames__` だけ正式名を保持し、他 2 名が入れ替わるという**系統的な差分**は「ゲーム側が場当たりに改名」より「旧版の命名」を示唆する（確度: 低〜中・推測）。逆に、本 EXE に消費者が 0 で出力 `.hkx` を外部ツールに渡す設計なら、非標準名は外部互換を損なうため不可解でもある。
> - **B 私有リテラルの妥当性 = 妥当**（SectionTag 契約・20 B 制限・重複検査を満たす。機能面で問題なし）。

**判定: 部分解明（classnames=正式名確定／classindex・dataindex=非公開名確定／4.0 正式名かは未証明）**（証拠 `v249_r1_out.txt` / `v249_r13_out.txt`、SDK 5.1 ヘッダ 3 本、Web 3 回）

---

## §4 item2（V-247-10-2）`'snap'` 4CC の SDK 実在 — **解明: `'snap'` は SDK の 4CC ではなく、`hkPackfileHeader::m_userTag` に書かれる「ゲームが選んだ任意の u32 ユーザータグ」。カテゴリ自体が SDK に無い（4CC 名前空間は存在しない）**

### 証拠A: `'snap'` は header+8 = `m_userTag` に直接書かれる（`v249_r12_out.txt` (a) / `v249_r13_out.txt` (d)）

- 生成（`0x8BC5DB..0x8BC5F8`）:
  ```
  0x8BC5DB  mov byte [esp+0x38],0x73   ; 's'
  0x8BC5E0  mov byte [esp+0x39],0x6e   ; 'n'
  0x8BC5E5  mov byte [esp+0x3a],0x61   ; 'a'
  0x8BC5EA  mov byte [esp+0x3b],0x70   ; 'p'
  0x8BC5EF  mov edx,[esp+0x38]         ; = 0x70616E73
  0x8BC5F8  mov [esp+0x0c],edx         ; descriptor+0 = 'snap'
  ```
- 伝搬: `0x8BC610 lea eax,[esp+0xc] / push eax / push edi / call [edx+0x10]`（= B の slot4 = `FUN_0090EF80`）→ `0x90EFCE mov ecx,[esi]`（esi = descriptor）→ `0x90EFDA push ecx` → `0x90DA40`。
- 書出し（`0x90DA40`）:
  ```
  0x90DA67  mov dword [esp+0x4c],0x57E0E057   ; m_magic[0]
  0x90DA6F  mov dword [esp+0x50],0x10C0C010   ; m_magic[1]
  0x90DA77  mov dword [esp+0x54],ecx          ; m_userTag  ← 'snap'
  0x90DA7B  mov dword [esp+0x58],2            ; m_fileVersion
  ```
  → **`'snap'` = `hkPackfileHeader::m_userTag`（header+8）**。offset は 5.1 の `hkPackfileHeader` 定義（magic[2] / m_userTag / m_fileVersion）と一致。

### 証拠B: SDK 側の意味は「ユーザー設定可能なタグ」で、固定 4CC ではない（一次資料）

- `Source/Common/Serialize/Packfile/hkPackfileWriter.h`（5.1）`struct Options`:
  ```cpp
  /// An optional tag for this file.
  /// You can use this tag to identify different types of file,
  /// examining only the file header.
  hkUint32 m_userTag;
  ```
- → SDK は **u32 の任意値をユーザーが入れる枠**を提供するだけで、`'snap'` という定数も 4CC 名前空間も持たない。**「SDK に `'snap'` 4CC が実在するか」は問いのカテゴリがずれている**（定義が存在し得ない）。

### 証拠C: descriptor の残り 2 語もゲーム側の内部キー（`v249_r12_out.txt` (b)(d)）

- descriptor = `{+0: u32 'snap', +4: u32 = *(u32*)"SnapshotSave" = 0x70616E53 ('Snap'), +8: u8 = 1}`。
  - `+4` は `0x8BC5FE..0x8BC60A` が `0x90BBA0`（`*(u32*)src` コピー）で文字列 `"SnapshotSave"`（`0xA98368`）の先頭 4 B を取得して格納。
  - `+8 = 1` はフラグ/版（確度: 低）。
- `0x90D960` は descriptor を受けて 0x3C B の一時オブジェクトを構築し、`lea eax,[ebx+4]`（= descriptor+4）を `0x942D70` に渡す。`0x942D70` は `+4` の dword を**数値として** push（`0x942DA6 mov ecx,[esp+0x10] / push ecx`）して `0x953530` に渡す＝**文字列ではなく 4 B キー**として扱う。
- → `'snap'`/`'Snap'` は「パックファイルのユーザータグ + 内部キー」の組で、いずれもゲーム側が決めた値。`+4` が `"SnapshotSave"` の先頭 4 B と一致するのは、名前から 4CC を機械的に作った痕跡（＝SDK 定数の引用ではない）。

### 裁定

> - **`'snap'` の SDK 実在 = 「無い」ではなく「SDK に 4CC 定数の概念が無い」**（確度: 高）。SDK が提供するのは `Options::m_userTag`（任意 u32）で、本 EXE はそこに `0x70616E73` を書く。
> - したがって v247 §3 の「`'snap'` はゲーム側が合成する 4CC タグ」は **正しい**。ただし「4CC タグ」ではなく **「ユーザータグ値」** と呼ぶのが厳密（header+8 の意味が確定したため）。
> - 上流概念（`hkpHavokSnapshot::save(world, stream, binaryFormat=true, ...)`、5.1 ヘッダで確認）と、`m_userTag` に `'snap'` を入れる経路は**同一の writer 呼出し**として整合。**未解明は残らない**（`+8` フラグの意味のみ低確度）。

**判定: 解明（`'snap'` = `m_userTag` のゲーム設定値。SDK に 4CC 定数は存在しない）**（証拠 `v249_r12_out.txt` / `v249_r13_out.txt`、SDK 5.1 `hkPackfileWriter.h` / `hkPackfileHeader.h`）

---

## §5 item3（V-247-10-3）node2/node3 の SDK 正式クラス名 — **解明（クラス同定）: node2 = `hkWindowsSystemClock`（`hkSystemClock` の Win32 実装）、node3 = `hkProcessFactory`。v247 の `hkStopwatch` 第一候補は誤り（hkStopwatch は非仮想・QPC 校正を持たない）**

### 証拠A: node2 の実装が `hkWindowsSystemClock` のソースと 1 対 1 で一致（`v249_r13_out.txt` (a)(b)(c)）

- node2 の vtable `0xAA28A4`:
  | slot | 値 | 実体 |
  |---|---|---|
  | 0 | `0x8BB3F0` | dtor |
  | 1 | `0x68F970` | **空スタブ `ret 4`**（purecall ではない。`v249_r12_out.txt` (e)） |
  | 2 | `0x945A30` | `rdtsc` → `edx:eax` を返す（`mov [esp+4],eax / mov [esp+8],edx`） |
  | 3 | `0x945A50` | 校正付き ticks/sec |
- slot3 `0x945A50` の本体（`v249_r13_out.txt` (c)）:
  ```
  0x945A59  mov eax,[0xBA94F0] / mov edx,[0xBA94F4]   ; 64bit キャッシュ
  0x945A74  fld qword [0xA47CA8]                      ; 1.0e6
  0x945A7A  mov dword [esp+0x14],0xA                  ; iter < 10
  0x945A86  call [ebx vtable+8]                       ; 自分の getTickCounter
  0x945A98  call [0xA28158]                           ; QueryPerformanceCounter
  0x945AA6  mov eax,0x1388                            ; 5000 回
  0x945AB0  imul ecx,edx / add ecx,[esp+0x10] / dec eax / jne
  0x945ACA  call [ebx vtable+8]                       ; 2 回目 tick
  0x945AD6  call [0xA28158]                           ; QPC
  0x945AE1  call [0xA2815C]                           ; QueryPerformanceFrequency
  ```
- SDK 5.1 `Source/Common/Base/System/Stopwatch/Impl/hkWindowsSystemClock.h` の `getTicksPerSecond()` と**行単位で一致**:
  - `for (int iter = 0; iter < 10; iter++)` ↔ `0xA`
  - `volatile int x=1; for (int j=0; j<5000; j++) x += x*x;` ↔ `0x1388` の `imul/add` ループ
  - `QueryPerformanceCounter` / `QueryPerformanceFrequency` を 2 回ずつ ↔ IAT `0xA28158`/`0xA2815C`
  - `double minFactor = 1e6f;` ↔ `0xA47CA8`
  - `getTickCounter()` の `rdtsc`（`__asm { push ebx; rdtsc }`）↔ `0x945A30`
- **`hkStopwatch` ではない**根拠: 5.1 `hkStopwatch.h` は `HK_DECLARE_NON_VIRTUAL_CLASS_ALLOCATOR` で**仮想関数なし**（vtable を持たない）、メンバは 64bit ticks 4 本＋flag＋name で、QPC 校正も rdtsc 直呼びも持たない。本 node2 は vtable 持ち・rdtsc・QPC 校正なので**クラスが違う**。
- `hkSystemClock`（5.1 `hkSystemClock.h`）は `hkSingleton<hkSystemClock>` で `virtual hkUint64 getTickCounter()=0; virtual hkUint64 getTicksPerSecond()=0;`＝**仮想 2 本の singleton**。node2 の vt 実コード 2 本と一致。
- → **node2 = `hkWindowsSystemClock`**（`hkSystemClock` 派生の Win32 実装）。確度: **高**。

### 証拠B: node3 の構造が `hkProcessFactory` の定義と一致（一次資料）

- SDK 5.1 `Source/Common/Visualize/hkProcessFactory.h`:
  ```cpp
  /// This class stores a list of process which are available.
  /// It is a singleton that store the registry of name to create
  /// function. It will only allow a scheduler to create any though.
  /// This is a thread safe singleton
  class hkProcessFactory : public hkSingleton<hkProcessFactory> {
      int registerProcess(const char* viewerName, hkProcessCreationFunction creationFunction);
      const char* getProcessName(int id);
      int getProcessId(const char* name);
      hkProcess* createProcess(const char *processName, hkArray<hkProcessContext*>& contexts);
      hkProcess* createProcess(int tag, hkArray<hkProcessContext*>& contexts);
    public:
      struct ProcessIdPair { hkString m_name; hkProcessCreationFunction m_processCreationFunction; int m_tag; };
    protected:
      int m_freeTag;
      hkObjectArray<ProcessIdPair> m_name2creationFunction;
      mutable hkCriticalSection* m_criticalSection;
  };
  ```
- 本 EXE の node3（`0xBA950C` / ctor `0x947B00` / 登録 `FUN_00947C50(registry, name, factory)`・`strcmp` 重複検査）:
  - **名前 → factory の singleton レジストリ**（v247 §4 証拠B の 16 名全数）。
  - 名前は `WorldMemory` / `MousePicking` / `Islands` / `Tensor` / `SweptTransforms` / `ConvexRadius` / `Shapes` / `CentreOfMass` / `Phantoms` / `ForcedContactPoints (See Docs For Usage)` / `ContactPoints` / `Constraint` / `Broadphase` / `ObjectInspection` / `Statistics` / `DebugDisplay` ＝ **Havok の可視化プロセス名そのもの**（`ObjectInspection` / `Statistics` / `DebugDisplay` は hkVisualize の代表的プロセス）。
  - factory は `hkReferencedObject` レイアウト（`+4`=u16 size / `+6`=u16 refcount）でオブジェクトを生成＝`hkProcess` 派生の生成関数。
- さらに `Source/Common/Visualize/hkProcess.h`（5.1）で `class hkProcess { virtual ~hkProcess(); virtual int getProcessTag()=0; ... virtual void step(hkReal)=0; }` を確認。`hkProcessFactory::registerProcess` の戻り値が **タグ**で、これが node4/node5 の `{handler, tag}` 配列（§2）と接続する。
- → **node3 = `hkProcessFactory`**。確度: **高**（機能一致＋一次資料の文言一致）。

### 裁定

> - **node2 = `hkWindowsSystemClock`**（確度: 高）。v247 の「`hkStopwatch`（第一）／`hkSystemClock` 系（第二）」を **`hkSystemClock` 系（Win32 実装）へ確定**。`hkStopwatch` は非仮想で本実装と構造が合わない。
> - **node3 = `hkProcessFactory`**（確度: 高）。v247 の「`hkProcessFactory` / `hkDebugDisplay` 系（中〜高）」を **`hkProcessFactory` に確定**。
> - 残（低）: 本 EXE の Havok が 4.0 のとき、5.1 と同一クラス名・同一 vt 順序だったかの版間差は未検証（5.1 を基準にした同定）。RTTI/COL 不在のため機械的な名前証明は依然不可（**名前は外部一次資料との一致による同定**）。

**判定: 解明（クラス同定。node2=hkWindowsSystemClock / node3=hkProcessFactory）**（証拠 `v249_r13_out.txt` / `v249_r12_out.txt` / v247 §4 の 16 名全数、SDK 5.1 ヘッダ 4 本）

---

## §6 まとめ・新規残点・自己検証

### 6-1. 本バッチ（§1〜§5）の判定一覧

| # | 残点（v247 §6-2） | 判定 | 確度 | 主証拠 |
|---|---|---|---|---|
| §1 | map owner `FUN_00943070` の動的生成元 | **解明**（遅延シングルトン: node `0xB30598` → `0x943210`(hk alloc 0x14/tag 0x15) → ctor `0x943070` → slot `0xBA94BC`） | 高 | `v249_r1/r6/r7/r8/r9/r10` |
| §2 | node4/node5 の slot2..4 が定数である理由＋実 slot 数 | **解明**（実 slot 数 = 2。3 語は vtable 間に並置された未参照静的定数で slot ではない） | 高（構造）／低（意味） | `v249_r3/r5/r11/r12`, `v249_const_out.txt` |
| §3 | `__classindex__`/`__dataindex__` は 4.0 期の正式名か | **部分解明**（classnames=正式名確定／他2名=非公開名確定／4.0 正式名かは未証明） | 高（非公開名）／低（4.0） | `v249_r1/r13`, SDK 5.1 ヘッダ3本 |
| §4 | `'snap'` 4CC の SDK 実在 | **解明**（`'snap'` = `hkPackfileHeader::m_userTag` のゲーム設定値。SDK に 4CC 定数は無い） | 高 | `v249_r12/r13`, SDK 5.1 ヘッダ2本 |
| §5 | node2/node3 の SDK 正式クラス名 | **解明**（node2=`hkWindowsSystemClock` / node3=`hkProcessFactory`） | 高 | `v249_r13/r12`, SDK 5.1 ヘッダ4本 |

**本バッチの新規確定事項（要点）**

1. map owner は **遅延シングルトン**。`0x943070` が e8 0 件なのは「デッド」ではなく、walker `0x8BB420` の `call [esi]`（間接）でしか呼ばれないため。生成は `0x943210` の **hk allocator `alloc(0x14, tag 0x15)` → ctor** で、placement new ではない（§1）。
2. 登録ノードは 9 件で、`{create, next, slotptr}` の 12 B。node2/node3/node4/node5/owner がすべてこの 1 機構で生成される（§1 表）。
3. node4/node5 の vtable は **2 slot**（dtor + `ret 4` の空スタブ）。`0x34`/`FLT_MAX`/π は**非コード値**で slot になり得ず、imm32 参照も 0 件。v245 の「slot2..4 は定数と推定」を確定（§2）。
4. `'snap'` は header+8 の **`m_userTag`** に書かれる（`0x90DA77`）。SDK の `Options::m_userTag` は「任意の u32」なので、4CC 定数の実在を問うこと自体が不成立（§4）。
5. **node2 = `hkWindowsSystemClock`**（rdtsc / QPC 10 回 / 5000 回ループ / 1e6 が 5.1 ソースと 1:1）。v247 の `hkStopwatch` 第一候補は**誤り**（hkStopwatch は非仮想）（§5）。
6. **node3 = `hkProcessFactory`**（名前→生成関数の thread-safe singleton、`registerProcess` がタグを返す）。16 名は Havok 可視化プロセスの実名（§5）。
7. SDK 5.1 の公式 section tag は `__classnames__`/`__types__`/`__data__` の 3 個のみ。本 EXE は version=2（5.1 は 4）を書く旧世代で、`__classindex__`/`__dataindex__` は公開名ではない（§3）。

### 6-2. 新規残点（次バッチ候補）

1. **[残] B の 3 セクション名が Havok 4.0 正式名かの最終確認**（低）: 4.0 の SDK/実 `.hkx` が必要。現状は「公開名でない」までが確定で、旧版改名説は棄却できない（§3）。
2. **[残] descriptor `+8` フラグの意味**（低）: `{+0 'snap', +4 'Snap', +8 = 1}` の 3 語目。`0x90D960` が `[+0x48]` に書き込む差分フラグとの関係は未追跡（§4）。
3. **[残] 静的登録関数群（`0xA15980` 等）の起動順**（低）: 9 ノードを積む CRT 静的初期化チェーンの順序と呼出し元（`0x8BBB5F` の上位）は未特定（§1）。
4. **[残] `0x34`/`FLT_MAX`/π が属するクラス（静的メンバ）の同定**（低）: 表示ジオメトリ系と推定するが、定義クラスを特定できていない（§2）。
5. **[残] node4/node5 のクラス名**（低）: `hkArray<{handler, tag}>` を保持する 0x14 B クラス。`hkProcessFactory` のタグと接続するが、5.1 ヘッダ群に対応クラスを見出せていない（§2/§5）。
6. **[残] `hkProcess` vtable のスロット順**（低）: dispatcher の `call [eax+8]` を `getProcessTag` と同定したが、5.1 の宣言順（dtor, getProcessTag, …）とは 1 slot ずれる可能性がある。派生クラスでの vt 追加を前提にした整理が要る（§5）。

### 6-3. 自己検証

- **selftest**: 着手時 `python probe_harness.py selftest` = **4/4 PASS**、完了時にも再実行して **4/4 PASS**（`bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl`、`ALL PASS`）。
- **VA 換算**: すべて `vc.va2off()`（PE セクションテーブル）と `imm32_refs3()`（セクション個別 raw）を使用。E8 走査は `e8_callers`（.text raw 起点 0x400C00）。v245 §8 の raw/VA 混同は踏襲していない。
- **数値の相互整合**: `0x90DA77` の書込み先 `[esp+0x54]` は header 先頭 `[esp+0x4c]` から +8 = `m_userTag`（`hkPackfileHeader` の宣言順と一致）。`0x90DA7B` の +0xC = `m_fileVersion`=2。`'snap'`＝`0x70616E73` は `0x8BC5DB..EA` の 4 バイト積み上げと一致。
- **負証拠の区別**:
  - **不存在証明（構造的 0 件）**: `0x943070` への e8/imm32 = 0（ただし間接呼出しが存在することを walker で確認）／`0xAA2BD8..0xAA2BE0` の imm32 = 0／node4/node5 の vptr 経由呼出し = 0／SDK 5.1 の SectionTag 定義が 3 個で閉じる。
  - **確認できず（探索したが見つからない）**: `__classindex__`/`__dataindex__` の Havok 4.0 での正式性／Web 完全一致 0 件（DuckDuckGo・Bing。`grep.app` は 429 で取得失敗）／`hkProcess` の vt スロット順の版間差。
- **外部資料の扱い**: 一次資料は Havok 5.1.0r1 公開ソース（`sigmaco/havok-v5.1.0r1-b2007.09.19`、BUILD #20070919）のヘッダ。**5.1 の記述を 4.0 へ外挿していない**（version 差を明記）。
- **制約遵守**: git 書込み 0／docs・GT・TODO・session-memory・plan・cache・ソース編集 0／既存 `tmp/re/*.md` 上書き 0（新規は本 v249 のみ）／ESM 再スキャン 0。中間物は `%TEMP%\opencode\v249_*`（新規）。
- **中断対策**: 本 md は §1（item4）→ §2（item5）→ §3〜§5（item1/2/3）→ §6 の順に**1 項ごとに追記**（書き溜め無し）。

### 6-4. 使用証拠一覧

- **本体**: `tmp/re/v249_process_havok_residuals10.md`（本ファイル・新規）
- **入力（読取のみ）**: `v247_process_havok_residuals9.md` §6-2 / §2〜§5、`v245_process_havok_residuals8.md` §4〜§8（node2 vt・node4/5 記述）、`v243_process_havok_residuals7.md` §4〜§5、`tmp/re/cache/{named_funcs,vtable_index,call_targets}`
- **本バッチ出力**（`%TEMP%\opencode\`）: `v249_r1_out.txt`（globals/dispatchers/owner）、`v249_r2_out.txt`（wrapper/node4-5 反復）、`v249_r3_out.txt`（vt 帯・slot 表・静的スロット表）、`v249_r4_out.txt`（ctor/init 探索）、`v249_r5_out.txt`（vt 帯構造・小関数）、`v249_r6_out.txt`（owner band 全断・refs）、`v249_r7_out.txt`（静的登録列）、`v249_r8_out.txt`（list head・create 関数）、`v249_r9_out.txt`（walker 本体）、`v249_r10_out.txt`（9 ノード全数）、`v249_r11_out.txt`（ノード/定数/walker）、`v249_r12_out.txt`（`'snap'` descriptor・0x68F970）、`v249_r13_out.txt`（node2 vt・header writer・0x942D70）、`v249_const_out.txt`（定数センサス）
- **一次資料（外部）**: Havok 5.1.0r1 公開ソース — `Source/Common/Serialize/Packfile/hkPackfileWriter.h`（SectionTag / Options::m_userTag）／`.../Binary/hkBinaryPackfileWriter.h`（SECTION_TAG_CLASSNAMES, class 名）／`.../Binary/hkPackfileHeader.h`（magic, "Currently 4"）／`.../Binary/hkPackfileSectionHeader.h`（m_sectionName 20 B）／`Source/Common/Base/System/Stopwatch/hkStopwatch.h`（非仮想）／`.../hkSystemClock.h`（仮想 2 本 singleton）／`.../Impl/hkWindowsSystemClock.h`（rdtsc + QPC 校正の実装）／`Source/Common/Visualize/hkProcessFactory.h`（name→create registry / tag）／`.../hkProcess.h`（getProcessTag）
- **Web 検索**: DuckDuckGo 完全一致 4 回（`"__classindex__" havok` 0 件 / `"__dataindex__"` 0 件 / `"hkProcessFactory" havok` で SDK リポジトリ発見 / `"ForcedContactPoints (See Docs For Usage)"` 0 件）、Bing 1 回（`"__dataindex__"` 0 件）。`grep.app` API は 429（**取得失敗**として明記）。

### 6-5. 既存記述の訂正（ドラフト・本体は未編集）

> 本節は**提案のみ**。`v245` / `v247` 等の既存 md は編集していない。

1. **v247 §4 証拠C**: node2 の第一候補 `hkStopwatch` は**誤り**。正: `hkWindowsSystemClock`（`hkSystemClock` の Win32 実装）。根拠: `hkStopwatch` は非仮想・QPC 校正なし、本実装は仮想 2 本＋QPC 校正（§5）。
2. **v245 §5(3) / v247 §4 の「`0x68F970` = purecall 系」**: 実体は **`ret 4` の空スタブ**（`v249_r12_out.txt` (e)）。「purecall」ではなく「空仮想関数」と表記するのが正確（§2）。
3. **v247 §3 の「`'snap'` = 4CC タグ」**: 実体は **`hkPackfileHeader::m_userTag` の値**。4CC というより「ユーザータグ u32」（§4）。
4. **v245 §5(3) の「slot2..4 は定数と推定」**: **推定を解除**。3 語は slot ではなく、実 slot 数は 2（§2）。
