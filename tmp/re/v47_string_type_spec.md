# v47: Oblivion.exe String 型 移植仕様書（Oblivion.exe Steam v1.2.0416）

作成日: 2026-10-24 / 解析対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
変換式: `.text` VA = 0x400C00 + file_off（ImageBase=0x400000, .text Raw=0x400）、
`.rdata` VA=0xA28000 Raw=0x627200、`.data` VA=0xB02000 Raw=0x700C00（PE は struct 自前解析）
基盤: v44（`tmp\re\v44_4028d0_memory_api.md`）の確定事実を継承。本ドキュメントは新規調査分を加える。

---

## 1. セルフテスト4件（実測・すべて PASS）

| # | 検査 | 期待値 | 実測値 | 結果 |
|---|------|--------|--------|------|
| 1 | bytes@0x504050 | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| 2 | bytes@0x4FB580 | `578b7908` | `578b7908` | PASS |
| 3 | string@0xA7035C | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| 4 | opid 0x0015 impl（table base 0xB0AF48, stride 0x28, 532件） | `0x504560` | entry[21] @0xB0B290 name=`OnLoad` impl=`0x00504560` | PASS |

スクリプト: `%TEMP%\opencode\v47_*.py`（capstone 5.0.7）。

---

## 2. String メソッドファミリの同定（0x401F00〜0x404000）

### 2.1 方法論

- Ghidra デコンパイルの全関数マーカ（`// FUN_xxx @` 20,465 件）から範囲内 57 関数を抽出し、
  各本文の u16 メンバアクセス（`+ 4` / `+ 6` / `param_N + 1` × `ushort|undefined2|short`）を走査。
- **根拠は capstone 逆アセンブルの `word ptr [reg+4]` / `word ptr [reg+6]` アクセスで裏付け**
  （フィルタ: `word ptr` かつ `dword ptr` 除外 — "dword ptr" は "word ptr" を部分文字列に含むため誤検出注意）。
- 範囲内で True-positive は下表の6関数のみ。`fldcw word ptr [esp+4]`（0x402B40/0x402C00/0x402C40/0x402C90）
  は FPU 制御ワード読みで型無関係、`[param+4]` dword アクセス（0x402DC0 等）は別型（+4 がポインタ、+0x24 が float）。

### 2.2 メソッド一覧（型操作関数・完全）

| VA | Ghidra名 | シグネチャ（実測） | 役割 | 被呼出先 | メンバ根拠 |
|----|----------|--------------------|------|----------|-----------|
| 0x402D60 | （無し・Ghidra 未関数化） | `String* __thiscall ctor(String* this)` ret(immなし) | ctor: `{buf,len,cap}=0` にして this を返す | なし（**呼び出し元0件**） | `[eax]/[eax+4]/[eax+6]` へ u32/u16/u16 ゼロ書き込み |
| 0x4028D0 | FUN_004028d0 | `bool __thiscall Set(const char* str, uint size)` ret 8 | 代入（cap不足なら再確保）。v44 確定 | 0x401F00(malloc) / 0x401F20(free) / 0x4028B0(strcpy) | `movzx eax,word [edi+6]`(cap比較) 他6件 |
| 0x402D00 | FUN_00402d00 | `int __thiscall Format(const char* fmt, va_list ap)` ret 8 | 1KB スタックへ vsprintf → Set(buf,0)。戻り値=vsprintf の文字数（Ghidra は void 描画） | 0x982820(_vsprintf) / 0x4028D0 / 0x9811E2(security cookie) | this=esi、末尾で ctor 相当のブロックを同居 |
| 0x402D70 | FUN_00402d70 | `String* __thiscall InitFrom(String* this, String* src)` ret 4 | copy-init: 自オブジェクトをゼロ-init → `Set(src->buf, 0)` | 0x4028D0 | `[esi]/[esi+4]/[esi+6]` ゼロ書き込み＋`*src` 読み |
| 0x402DA0 | FUN_00402da0 | `void __fastcall Clear(String* this)` ret(immなし) | dtor/Clear: `free(buf)` → 全フィールド 0 | 0x401F20（NULL-safe free） | `[esi]/[esi+4]/[esi+6]` ゼロ書き込み |
| 0x402E30 | FUN_00402e30 | `void __cdecl FormatV(String* self, const char* fmt, ...)` ret(immなし) | 可変引数エントリ: `va_list=&arg3` を組み self->Format(fmt, ap) へ転送 | 0x402D00 | 直接メンバアクセスなし（転送のみ） |

補助（型本体ではなくインフラ）:

| VA | 名前 | シグネチャ | 役割 |
|----|------|-----------|------|
| 0x4028B0 | FUN_004028b0 | `void __cdecl strcpy2(char* dst, char* src)` | strcpy 相当。`dst-src` 差分ループで NUL 含め複写（cdecl・ret 無 imm・呼出側 `add esp,8`） |
| 0x401F00 | FUN_00401f00 | `void* malloc(uint size)` | engine heap アロケータ（`FUN_00401AA0(size,1)` 経由、heap manager 0xB02020） |
| 0x401F20 | FUN_00401f20 | `void free(void* p)` | engine heap 解放。**`if (p != 0)` チェック付き**（L1127）→ dtor は NULL buf でも安全 |

ctor (0x402D60) の実体（全15バイト）:

```
00402D60  mov eax, ecx            ; return this
00402D62  xor ecx,ecx
00402D64  mov dword [eax], ecx    ; buf = 0
00402D66  mov word  [eax+4], cx   ; len = 0
00402D6A  mov word  [eax+6], cx   ; cap = 0
00402D6E  ret
```

デコンパイル冒頭（定義がある関数）:
- 0x4028D0: `bool __thiscall FUN_004028d0(undefined4 *param_1,char *param_2,uint param_3)` (L1908, v44 全文引用済)
- 0x402D00: `void FUN_00402d00(char *param_1,va_list param_2)` (L2168)
- 0x402D70: `undefined4 * __thiscall FUN_00402d70(undefined4 *param_1,undefined4 *param_2)` (L2185)
- 0x402DA0: `void __fastcall FUN_00402da0(undefined4 *param_1)` (L2199)
- 0x402E30: `void FUN_00402e30(undefined4 param_1,undefined4 param_2)` (L2247)

### 2.3 除外した近隣関数（誤同定防止の明示）

| VA | なぜ String メソッドではないか |
|----|-------------------------------|
| 0x402DC0 | `[param_1+4]` は **dword ポインタ**（==0 判定 → `[ptr+0x24]` float を 0x9828C0 へ）。+4/+6 への u16 アクセスなし。Gamebryo 系の型分類ヘルパ |
| 0x402E50 | this は float 保持オブジェクト。String は**出力先**としてのみ登録（`"%s %d, 3E%d"` を FormatV で流し込む数値→文字列クライアント） |
| 0x402B40/0x402C00/0x402C40/0x402C90 | `fldcw word ptr [esp+4]` = FPU 制御ワード。型無関係 |
| 0x401F00/0x401F20/0x401F40 ほか 0x401xxx〜0x4027xx | heap manager / CRT ラップ群。+4/+6 アクセスは他型またはスタック引数 |

**結論**: この型のメソッド集合は上記6つに尽きる。**Append / Compare / operator[] は存在しない**
（利用側は `buf` 生ポインタを直接読み、CRT strcmp 等で代替）。範囲外（0x404000 超）への展開は未走査（§7）。

---

## 3. FUN_009FBF20（グローバル文字列テーブル初期化）完全デコード

### 3.1 全体像

- シグネチャ: `void __cdecl FUN_009FBF20(void)`。ret(imm なし)・引数なし。
- **SEH 保護関数**: エントリで `push -1 / push 0x9C04BE(handler) / push fs:[0]`、
  GS cookie `DAT_00b30aac`、終端で `push 0xA24B60; call 0x981FB4`（.rdata のクリーンアップ/スコープ構造 —
  0xA24B6A は 0xB3B370 を含む）→ `mov fs:[0],...; pop esi; add esp,0xC; ret` @0x9FC132。
- 逆アセンブル実測: 655 命令スキャン、終端 ret @0x9FC132（次 byte 0xCC）。上限 0x800 内に完全収容。
- **処理内容**: `String` オブジェクト 12 個を「{buf,len,cap} をゼロ初期化 → `FUN_004028D0(リテラル, 0)`」
  で順に初期化するだけ（検索/挿入構造ではない）。

### 3.2 登録される全エントリ（address → 実リテラル・実読）

グローバル配列 **base=0xB3B370、stride=8、総数 12 件**（0xB3B370〜0xB3B3CF、計 96 バイト）。

| # | String オブジェクト VA | リテラル VA | リテラル実値 | 初期化形 |
|---|------------------------|-------------|--------------|----------|
| 0 | 0x00B3B370 | 0x00A2F7EC | `""`（空文字） | ゼロ-init 省略（BSS）→ Set("",0) → **need=0 パスで全て0のまま** |
| 1 | 0x00B3B378 | 0x00A6C0C8 | `item_pickup` | 明示ゼロ-init → Set |
| 2 | 0x00B3B380 | 0x00A6C0B8 | `open_container` | 同上 |
| 3 | 0x00B3B388 | 0x00A6C0AC | `chair_sit` | 同上 |
| 4 | 0x00B3B390 | 0x00A6C098 | `activate_pull_push` | 同上 |
| 5 | 0x00B3B398 | 0x00A6C08C | `bed_sleep` | 同上 |
| 6 | 0x00B3B3A0 | 0x00A6C078 | `book_scroll_read` | 同上 |
| 7 | 0x00B3B3A8 | 0x00A6C06C | `npc_talk` | 同上 |
| 8 | 0x00B3B3B0 | 0x00A6C060 | `open_door` | 同上 |
| 9 | 0x00B3B3B8 | 0x00A6C058 | `horses` | 同上 |
| 10 | 0x00B3B3C0 | 0x00A6C050 | `crown` | 同上 |
| 11 | 0x00B3B3C8 | 0x00A6C048 | `vampire` | 同上 |

- 各エントリの +0x00 = **char* buf**（Set 実行後にヒープコピーへのポインタ。**リテラル VA ではない**）、
  +0x04 = **u16 len**、+0x06 = **u16 cap**（別フィールドではない。v44 の 8 バイト構造を再確認）。
  初期化直後: #1〜#11 は buf=ヒープ ptr / len=cap=文字長、#0 は buf=NULL / len=cap=0。
- 呼び出し形の実例（entry1）: `push 0; push 0xA6C0C8; mov [0xb3b378],0 / mov word [0xb3b37c],0 / mov word [0xb3b37e],0; mov ecx,0xb3b378; call 0x4028D0` @0x9FBF53〜0x9FBF76。
- リテラル群は .rdata の連続ブロック（0xA6C048〜0xA6C0D3）に格納。

### 3.3 終了時クリーンアップ（新規発見）

0x9C0450〜0x9C04B9 に **11 個の tail-jump thunk**（`mov ecx,<VA>; jmp 0x402DA0`、1個あたり10バイト）:

```
009C0450  mov ecx, 0xb3b370 / jmp 0x402da0   …
009C04AA  mov ecx, 0xb3b3b8 / jmp 0x402da0
009C04B4  mov ecx, 0xb3b3c0 / jmp 0x402da0   ← 11個目（最後）
```

- これは MSVC の静的デストラクタ（atexit）登録用 thunk 群の典型形 [推定・パターン確度高]。
  同領域には他型の同種 thunk（例 0x9C0420 `mov ecx,0xb140b0; jmp 0x403bc0`）が混在。
- **エントリ #11 `vampire`(0xB3B3C8) だけ dtor thunk が存在しない**（imm32 参照は init 内2箇所のみ、
  他エントリは3箇所 = init×2 + thunk×1）。終了時バッファリーク（意図的除外か遗漏かは不明、§7）。

---

## 4. FUN_00402D00 の解読（Format 本体）

- **シグネチャ**: `thiscall`、`ret 8`（スタック引数2個）。`void __thiscall FUN_00402d00(String* self, const char* fmt, va_list ap)`。
  戻り値は実体では vsprintf の文字数（`mov eax,edi` @0x402D4C、Ghidra は void と描画）。
- **本体フロー**（逆アセンブル + L2168 デコンパイル一致）:
  1. `sub esp,0x404`（1028バイト = **char buf[1024]** + 余白）、GS cookie 付与/検証（0xB30AAC / 0x9811E2）。
  2. `call 0x982820` = **`_vsprintf(buf, fmt, ap)`**（cdecl 3引数、CRT リージョン 0x98xxxx）。
     → 被呼出先は vsprintf 系で確定。**バウンズチェックなし**（書式結果が1024超だとスタック破壊 → cookie クラッシュ）。
  3. `push 0; push &buf; mov ecx,self; call 0x4028D0` → **Set(buf, 0)**（strlen で長さ確定）。
  4. cookie 検証 → `ret 8`。
- **書式引数の扱い**: 呼び出し元は cdecl で `push 引数…; push fmt; push &self; call 0x402E30`。
  0x402E30 が `va_list = &arg3` を組んで 0x402D00 へ転送する（§2.2）。実利用は 0x402E30 経由が 244 箇所、
  0x402D00 直接は **2 箇所のみ**（0x402E30 自身と FUN_00585C90 @0x585CFC — 後者は引数を事前計算して直接渡す）。

---

## 5. 使用分布（データ関係性）

### 5.1 カウント総括（decompile 1パス走査が典拠）

| メソッド | 箇所数 | 呼び出し元関数 |
|----------|--------|----------------|
| Set (0x4028D0) | **530** | 221 |
| FormatV (0x402E30) | **244** | 116 |
| Clear/dtor (0x402DA0) | **81** | 45 |
| Format (0x402D00) 直接 | 2 | 2 |
| InitFrom/copy (0x402D70) | 1 | 1 |
| ctor (0x402D60) | **0**（死コード or 完全インライン化） | 0 |
| **合計** | **858** | **318**（重複除く） |

capstone の素通し E8 走査では Set=557 / FormatV=250 / dtor=83 だが、差（+27/+6/+2）は関数体内の
インラインジャンプテーブル（例 0x402E10 境界の data-in-code）由来の幻影デコードであり、**flow-accurate な
decompile 数（530/244/81）を採用**。v44 の 530/221 と完全一致（上位10呼出元も 75/31/24/13/13/12/10/9/9/8 で一致）。

### 5.2 サブシステム別分類（上位15系統・858 箇所ベース）

分類は「VA 範囲 + 呼出関数が参照する .rdata 文字列の実読」による手動補正付き。

| # | サブシステム | 箇所数 | 関数数 | 代表 |
|---|--------------|--------|--------|------|
| 1 | UIメニュー/テンプレート（0x59〜0x5D メニューXML/テンプレ生成/各画面） | 467 | 116 | FUN_005C7BA0(75) FUN_005C34D0(31) FUN_005CDEF0(24) |
| 2 | スクリプト/VM/フォーム/コンソール（構文エラー文・GS dump・console コマンド応答） | 77 | 42 | FUN_004F2270(7) FUN_0052D2C0(6) FUN_00514950(4) |
| 3 | ワールド/NIF/スケルトン/LOD（`Tyring to add skinned object…`、LOD パス、水音） | 74 | 51 | FUN_0048F450(6) FUN_0048F6A0(5) FUN_0047BC40(3) |
| 4 | UIメニュータイル/XML系（tile/trait/action パーサ、`InterfaceManager`） | 66 | 38 | FUN_00577C10(10: `IMG/SRC/WIDTH/HEIGHT/DIV/ALIGN/LEFT`) FUN_0058BEE0(5) |
| 5 | レベルアップ/成長（`%sLevelUpData_1.txt` ダンプ） | 25 | 1 | FUN_006684B0(25) |
| 6 | FaceGen/チェアリジェン（`%s.egm/.nif/.tri/.egt`、FaceGenHair/Eye） | 24 | 13 | FUN_005547F0(4) |
| 7 | debug/コンソール出力（FPS/Actor Loc、ref 情報 overlay、dof/usz コマンド） | 23 | 3 | FUN_005821F0(15) |
| 8 | ESM/レコード+MGEF名（`%s %s` 系書式・読込エラー文） | 19 | 16 | FUN_00413BE0(4) |
| 9 | 空模様/スカイ（`Atmosphere/Clouds/Stars.nif`、`\Textures\Sky\%s_%s.dds`） | 12 | 6 | FUN_0053D260(7) |
| 10 | init/定数テーブル（アクション名12件） | 12 | 1 | FUN_009FBF20(12) |
| 11 | クエストUI通知（`UIQuestNew/UIQuestUpdate`） | 11 | 1 | FUN_006697A0(11) |
| 12 | ゲームプレイ/プレイヤー/魔法デバッグ | 10 | 9 | FUN_006B8280 等 |
| 13 | 経済/バーター確認ダイアログ（`%s %i %s?`、`Merchant does not have enough gold.`） | 6 | 1 | FUN_0059A400(6) |
| 14 | コア/デバッグ（本体の Format/copy 周辺） | 6 | 5 | FUN_00412FA0(2) |
| 15 | アクター装備/リップシンク/ダイアログ（`Arrow%d`、`BigAah 0.9 0.2…`、Topic Info） | 11 | 6 | FUN_005FCAB0(2) FUN_005F7310(2) FUN_006267A0(2) |

（残り: フォーム初期化/セーブ 2、AI/コンディション 4 — TESConditionItem エラー文のみ）
→ **String は UI/メニューグループで爆発的に使われ、ゲームプレイ系ではエラー文/デバッグ文に限定**される。

### 5.3 呼出チェーン（代表3本）

**Chain A: UI ダイアログ文言の構築（Format 系の典型）**
```
FUN_005CC200 (確認ダイアログ)
  └ push 引数 / push "%s %s?"@0xA6B4D8 / lea eax,[esp+0x24](stack String) / push eax
    call 0x402E30 (FormatV, cdecl)
      └ call 0x402D00 (Format, thiscall) → _vsprintf@0x982820 (1KB stack buf, 無制限)
          └ call 0x4028D0 (Set) → [grow時] 0x401F00(malloc) + 0x4028B0(strcpy) → 0x401F20(free旧)
  …使用後… call 0x402DA0 (Clear) → 0x401F20(free) → heap manager@0xB02020
```

**Chain B: グローバル定数テーブルの生成と破棄**
```
CRT 初期化 → FUN_009FBF20 (SEH付, cdecl)
  └ 12エントリ: {buf,len,cap}=0 → call 0x4028D0(リテラル, 0) → malloc コピー
終了時: CRT 静的デストラクタ thunk (0x9C0450..0x9C04B9, 11本)
  └ jmp 0x402DA0 → 0x401F20(free)   ※"vampire"@0xB3B3C8 は登録なし
```

**Chain C: 構造体埋め込み String（this = オブジェクト+8）**
```
FUN_00585C90 (メニュータイル系)
  └ tile オブジェクト取得 (vtable call) → lea ecx,[edi+8] (String member)
    call 0x4028D0 (Set) … 他フィールドと混在してゼロ-init→Set を反復
```
→ String は**値メンバとして他構造体に埋め込まれる**（+8 は一例）。ローカル変数として
スタック上にゼロ-init→Set で構築する形も到处にある（ctor 0x402D60 は呼ばれない＝インライン化）。

---

## 6. 移植仕様

### 6.1 構造体レイアウト（C++ 推奨表現）

```cpp
// 8 bytes, no vtable, trivially copyable (raw 8-byte copies are used across the engine)
struct OblivionString {          // EXE 内の正式名は不明（RTTI なし）
    char*    buf;                // +0x00  NUL 終端・heap 所有。NULL 可
    uint16_t len;                // +0x04  min(need, 0xFFFF)
    uint16_t cap;                // +0x06  min(need, 0xFFFF)（Set 経路では常に len と同値）
};
static_assert(sizeof(OblivionString) == 8);
```

### 6.2 実装すべきメソッド（挙動仕様）

| VA | 擬似コード | 備考 |
|----|-----------|------|
| 0x402D60 | `String& ctor(){ buf=0; len=cap=0; return *this; }` | 呼出0件だがグローバル/ローカル宣言の意味論として必要 |
| 0x4028D0 | `bool Set(const char* s, uint32 size)`<br>`need = s ? (strlen(s)>size ? strlen(s) : size) : size;`<br>`if (cap < need) { nb = malloc(need+1); if (nb) { s ? strcpy2(nb,s) : nb[0]=0; } else need=0; free(buf); buf=nb; cap=min(need,0xFFFF); }`<br>`else if (need==0) { free(buf); buf=0; len=cap=0; return false; }`<br>`else { s ? strcpy2(buf,s) : buf[0]=0; }`<br>`len = min(need,0xFFFF); return need!=0;` | grow=「malloc新→コピー→free旧」。malloc失敗は need=0 扱いで続行。**戻り値は全858サイトで未使用** |
| 0x402D00 | `int Format(const char* fmt, va_list ap){ char b[1024]; int n=vsprintf(b,fmt,ap); Set(b,0); return n; }` | **vsprintf 無制限**（1024超でスタック破壊。移植推奨: `vsnprintf(b,1024,…)` — 但しこれは挙動変更） |
| 0x402E30 | `void FormatV(String* self, const char* fmt, ...){ va_list ap; va_start(ap,fmt); self->Format(fmt,ap); }` | 実利用の99% がこの経路 |
| 0x402D70 | `String& InitFrom(String* dst, const String* src){ dst->ctor(); dst->Set(src->buf,0); return *dst; }` | src の len/cap は**再計算**されない（strlen 依存） |
| 0x402DA0 | `void Clear(){ free(buf); buf=0; len=cap=0; }` | free は NULL-safe。dtor 後に再 Set すれば復活可能 |

### 6.3 グローバル文字列テーブル

- **`OblivionString g_ActionNames[12]` @ 0x00B3B370**（stride 8、0xB3B370〜0xB3B3CF）。
- 初期値（§3.2 表）: `"" / "item_pickup" / "open_container" / "chair_sit" / "activate_pull_push" /
  "bed_sleep" / "book_scroll_read" / "npc_talk" / "open_door" / "horses" / "crown" / "vampire"`。
- #0 は buf=NULL/len=0/cap=0 のまま。#1〜#11 はヒープコピー保持。
- 終了時に #0〜#10 が Clear され #11 は Clear されない（EXE 互換では再現不要の軽微差）。
- imm32 参照は init/thunk に限定されており、**他コードからの直接参照は確認されず**
  （間接経由の有無は未調査）。ポインタ解決テーブル等を移植する際はアドレス固定の要否を再確認のこと。

### 6.4 std::string 等への代替可否（事実ベースの注意点）

1. **「Set で空化したら buf が free され NULL になる」**: 0x4028D0 は `need==0` 時に
   `free(buf); buf=0` を実行する（v44 L1950-1955）。std::string は内部バッファを保持し続けるため、
   **「以前に読み出した buf 生ポインタ」の寿命が異なる**。EXE コードは Set 後に buf を再取得する設計だが、
   「Set 呼び出し前に buf を確保して使う」パターンが1つでもあれば EXE 側は UAF、移植側は安全 =
   **テスト差分の源泉**。UAF 再現が必要な箇所では明示的な参照計数/世代管理を検討。
2. **SSO の有無**: EXE は cap==0 即ヒープ確保で SSO なし。std::string（SSO あり）は短文字列で
   buf が内部ストレージを指す → **buf アドレスの同一性・安定性が異なる**。ポインタ比較をしている
   コード（自前 strcmp でなくアドレス比較）があれば差異に出る。
3. **len が u16 クランプ & 「size>strlen 時に余剰長を len に記録」**: `Set(ptr, size)` の size は
   最低保証長であり、`len = min(max(strlen, size), 0xFFFF)`。std::string::size() は再現不能 →
   非互換レイヤ（`assign(ptr, size)` + size 上書き）が必要。64KB 超は len が飽和する点も再現要。
4. **free 連鎖**: grow 時の「malloc新→コピー→free旧」と Clear の free は engine heap
   （0xB02020 manager 経由）だが、**レイアウト依存なし＝通常 malloc/free で代替可能**（buf は
   オブジェクト外独立ブロック）。互換テストでアロケータ統計を取る場合のみ差異。
5. **Format の溢れ**: 無制限 vsprintf は 1024 超でスタック破壊（GS cookie 即クラッシュ）。
   std::format/snprintf への置換は安全だが公式挙動（クラッシュ）の再現ではない。既知の溢れ書式は
   未調査（§7）。

---

## 7. 未解決点

1. クラスの正式名（NiString / BSString 等）。RTTI 不在で特定不能（v44 と同じ留保）。
2. **0x404000 超に Compare/Append 等の追加メソッドが存在するか未走査**（本次の範囲外）。
3. ctor 0x402D60 の呼び出し元0件 = 完全インライン化か死コードかの断定は未了（raw E8 スキャンでも0件）。
4. len(+0x04)/cap(+0x06) を**直接読む**外部コードの網羅的同定は未実施
   （+4/+6 への u16 アクセスは他型と混在し、790関数・千件超で分離不能。buf 経由の間接利用が主流と推定）。
5. "vampire"(0xB3B3C8) の dtor thunk 欠落の意図（終了時リークは無害レベル）。
6. FUN_006684B0（LevelUpData ダンプ25箇所）等、大户の書式文字列溢れリスク評価は未実施。
7. 0x564730/0x564860（dtor のみ各2件）の正体。0x981FB4/0xA24B60（SEH スコープ機構の実体）の詳細。

---

## 付録: 検証メタ情報

- スクリプト: `%TEMP%\opencode\` の `v47_lib.py`（PE パーサ）、`v47_selftest.py`、`v47_family.py`、
  `v47_9fbf20.py`、`v47_402d00.py`、`v47_detail.py`、`v47_member_scan.py`、`v47_usage.py`、
  `v47_recount.py`、`v47_final.py`、`v47_xref.py`、`v47_dtor_table.py`
- 中間成果: `v47_9fbf20_full.txt`（全120行逆アセンブル）、`v47_usage_full.txt`（339行・呼出元一覧）、
  `v47_decompile_counts.txt`（318行・decompile 基準カウント）
- decompile 窓読みは enumerate+islice 方式、ラベル照合は lowercased を徹底。
