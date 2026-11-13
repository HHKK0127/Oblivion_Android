# V359: GridCellArray 同定と間接経路の確定 — #138（V-355-1 / V-355-3 / V-357-1）

- 日付: 2026-11-13 / タスク: ai-todo **#138**【v355/v357 残点束】①FUN_00482200 の実行時呼び出し元 ②vtable+0x1C メソッド本体 ③0x44EE00 の正式名と自再入目的
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: `v355_r4_null_slot_ruling.md` 残点 V-355-1/V-355-3 ＋ `v357_named5_ruling.md` 残点 V-357-1
- 手法: capstone 5.0.7（v307_common.py）／vptr 代入（C7 06 / `mov [esi],imm32`）のバイト走査／**RTTI COL レイアウト校正（NiDX9 既知 COL 0xACC9D0 で pTD=+0xC を実測確定）→ COL→TD→型名解決**／E8 rel32 逆算／`call [reg+0x18]` 全列挙／annotate_db.json 照合
- 中間証跡: `%TEMP%\opencode\v359_1..v359_9.py/.out`（**全 EXIT=0**。v359_2/3 は pre-run で `blob` 定義・`X86_OP_IMM` import を補正、v359_9 も同 import 補正後に実行）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM/.ess 未アクセス、EXE 読み取り専用、L1

---

## §0 判定サマリ

| 設問 | 判定 |
|---|---|
| **① V-355-1** FUN_00482200 の実行時呼び出し元 | **[確定] 機構**＝**`.?AVGridCellArray@@` vtable（0xA3D79C）の +0x18 間接 call が唯一経路**（E8=0・u32 参照＝rdata スロット1件のみ・abs 参照0 の排他証明＋**+0x18 の値 0x482200 は GridCellArray 固有**＝他の3 RTTI class は別値）。**[部分確定] 個別 caller**＝`call [reg+0x18]` 全74件中、owner 経由（`[reg+8]`→GridCellArray*）型の候補 **0x906BBE / 0x9073FE**。残点＝74件の型帰属 |
| **② V-355-3** vtable+0x1C メソッド本体 | **[確定(命令+RTTI)] `this`＝GridCellArray（vtable 0xA3D79C）**、**vtable+0x1C ＝ `FUN_00482240` ＝ ClearCell(x,y)**: `[this+0x10][(y*width+x)*8] = 0`（slot を NULL 化）＋ `[this+0x20] = 0`。生成/保持経路まで全リンク（container ctor 0x441970 が `new`→ctor 0x483350→**`[container+8]=GridCellArray*`（ポインタ格納）**、container 自体は `new(0xAC)`→fn 0x406360 が `[0xB333A0]` へ格納） |
| **③ V-357-1** 0x44EE00 の自再入目的・正式名 | **自再入目的=[部分確定]**＝**子オブジェクト解決の再帰ループ**（リスト走査→子取得→座標 getter→predicate→`[0xB33AC4]` refcheck→**self に3引数で再帰**）。**機能＝チェーン解決/リゾルバ系 [部分確定]**（once-init・`List_Clear`×2・`Push 0x446CB0`・12B コピー・refcount 管理を統合）。**正式名=[未確定] 維持**（関数内文字列0 [確定負]・annotate_db 未登録・vtable 非会員＝RTTI 不適用） |

---

## §1 鍵となった発見: 4つの RTTI テーブル（v359_5/6）

**COL レイアウト校正**: 既知 `.?AVNiDX9Renderer@@` の COL `0xACC9D0` を実測 → **pTD = COL+0xC**（+8 ではない）を確定。この校正で4テーブルすべての型名を解決:

| vtable | vtable[-1] (COL) | RTTI 型名 |
|---|---|---|
| 0xA3D718 | 0xAB8C6C | **`.?AVGridArray@@`** |
| 0xA3D744 | 0xAB8CB4 | `.?AVNiStencilProperty@@` |
| **0xA3D79C** | 0xAB8D0C | **`.?AVGridCellArray@@`** ← FUN_00482200 保持 |
| 0xA3D7E0 | 0xAB8D58 | **`.?AVGridDistantArray@@`** |

- **vptr 代入元のバイト走査**（v359_4/5）: `mov [esi],0xA3D79C` @**0x483382**（ctor `0x483350` 内）／`mov [esi],0xA3D718` @0x481DE0/0x481DF0（GridArray setter）／`mov [esi],0xA3D744` @0x4825D0・0x49B8C8・0x718FC7（NiStencilProperty ctor 群）／`mov [esi],0xA3D7E0` @0x483929・0x483AC9（GridDistantArray ctor 群）。
- → v355 時点の「0xA3D7E0 の xref 2件（0x48392B/0x483ACB）」は**GridDistantArray の vptr 代入＝caller ではない**と**訂正**（v355 §4 の推測的記述を改訂）。

---

## §2 ①の裁定: FUN_00482200 の唯一経路 [確定]

### 2.1 排他証明

| 経路 | 探査 | 結果 |
|---|---|---|
| E8 直接 call | rel32 逆算全走査（v355_2） | **0件** |
| u32 アドレス参照 | 全セクション（v355_1/3） | **rdata `0xA3D7B4` の1件のみ**（＝GridCellArray vtable の +0x18 スロット） |
| abs `call [0xA3D7B4]` / mov imm | v355/v359 バイト走査 | **0件** |
| **間接 vtable dispatch** | `call [reg+0x18]` disp8/disp32 全列挙（v359_7） | **74件**（うち古典的 vptr ロード形 `mov r,[r]; call [r+0x18]` を多数含む） |

### 2.2 スロット固有性 [確定]（v359_8）

| クラス | vtable+0x18 の値 |
|---|---|
| GridArray | 0x982D85（thunk） |
| NiStencilProperty | 0x718ED0 |
| **GridCellArray** | **0x482200 = FUN_00482200** ← 固有 |
| GridDistantArray | 0x483630（別 impl） |

- → **`call [reg+0x18]` で GridCellArray を呼ぶ site だけが実行時 caller**。GridCellArray の実体は container 構築時に1個のみ生成（§3）＝**本命 caller は container 経由**。
- **個別候補 [部分確定]**: `0x906BBE` / `0x9073FE`（`mov ecx,[edi+8]; mov edx,[ecx]; call [edx+0x18]` ＝ owner 経由型）。残り72件は他クラスの +0x18 dispatch を含み多義（残点 V-359-1）。

---

## §3 ②の裁定: vtable+0x1C = GridCellArray::ClearCell [確定(命令+RTTI)]

### 3.1 `this` の型と生成経路（全リンク）

```
fn 0x406360（起動期初期化）:
  push 0xAC; call operator new(172)          ; container 本体
  mov ecx,eax; call fn 0x441970              ; container ctor（rdata 文字列 0xA2FFB8 等を引数）
  mov [0xB333A0], eax                        ; ★ グローバル = container ポインタ
  mov ecx,eax; call 0x43F560                 ; 続行初期化

fn 0x441970（container ctor・this=esi）:
  ... new → mov ecx,eax; call 0x483350       ; ★ GridCellArray ctor
  mov dword [esi+8], eax                     ; ★ container+8 = GridCellArray*（ヒープ別体・ポインタ格納）
  mov edx,[eax]; mov eax,[edx+4]; call eax   ; 新 instance の vtable+0x04 初期化呼出し
  push 0x14; call new → call 0x4838F0        ; GridDistantArray ctor も同梱生成（vptr 0xA3D7E0）

GridCellArray ctor = 0x483350:
  call 0x481DE0（基底 GridArray ctor＝vptr 0xA3D718 スタンプ）
  mov [esi], 0xA3D79C                        ; ★ derived vptr 再スタンプ
  eax = [0xB06A2C]（≥5・奇数化）→ mov [esi+0x0C], eax   ; ★ width
  w*w*8 バイトを確保（imul/mul edx=8）        ; ★ slot 配列（8B/slot と一致）
```

- **使用側（v359_2/4）**: `mov edx,[0xB333A0]; mov ecx,[edx+8]`（v327 tail 同型）→ FUN_00482170/004821B0/00482390 の `this`。`FUN_004441A0`/`FUN_004430F0`/`FUN_00444C70` も全て `mov ecx,[esi+8]`（owner）経由。
- → **`this` = GridCellArray [確定]**、型名は RTTI `.?AVGridCellArray@@` [確定]。

### 3.2 vtable+0x1C の本体 = `FUN_00482240` [確定(命令)]

```asm
482240: eax = [ecx+0x0C]            ; width
        eax = width*y + x
        edx = [ecx+0x10]            ; slots
        dword [edx + eax*8] = 0     ; ★ slot[y*W+x] = NULL
        byte  [ecx+0x20] = 0        ; ★ 状態フラグクリア
        ret 8
```

- **v355 の問い（vtable+0x1C クリア経路の対象メソッド）はこれで閉鎖**: `FUN_00482170(obj=0)` と `FUN_00482200` が委譲先として呼ぶ `this->vtable+0x1C(x,y)` ＝ **GridCellArray::ClearCell（slot NULL 化＋フラグクリア）** [確定]。
- **GridCellArray vtable 全10スロット**（0xA3D79C、+0x24 で終端前）: +0x00 `0x483570`（deleting dtor 形: `call 0x483460`(complete dtor)＋flags&1 で op delete）／+0x04 `0x482080`／+0x08 `0x481E20`／+0x0C `0x4821E0`（`call 0x481E10`→`[0xB33A98]+0xCD4` ゲート→`jmp 0x4CA390`）／+0x10 `0x482090`（座標 → `shl 0xC` fixed-point 化＋`[this+0x14/0x18/0x1C]` float store＋`0x4CCCC0` 呼出）／+0x14 `0x481E60`／**+0x18 `0x482200` release（slot 解放→vtable+0x1C 委譲）**／**+0x1C `0x482240` ClearCell**／+0x20 `0x482260` copy（src/dst スロット移動・src NULL 時に**自 vtable+0x1C を fallback 呼び出し**）／+0x24 `0x4822A0` move 系（同 fallback）。
- complete dtor `0x483460`: vptr 再スタンプ→`call 0x481E10`（基底 dtor）→`[0xB33A98]+0xCD4` ゲート→`call 0x482670`（width×width ループで slot 破棄）。
- ctor に **E8 caller 0件**（slot0=0x483570 経由のみ＝vtable ディスパッチ）→ 生成は container ctor の1系統のみ [確定]。

---

## §4 ③の裁定: 0x44EE00（V-357-1）[部分確定]

### 4.1 全 call リスト（v359_3・248 insn 完全デコード）

`0x44D720`(init) / `0x981FB4`(_onexit) / **`0x4526E0` List_Clear ×2**（annotate_db 正典一致） / `0x9832E6` ×2 / `0x4C97F0`(predicate) / `0x4EF130` ×2 (predicate) / `0x452570`(list push 系) / `0x4CBD30` / `0x4EF270` / `0x4D7630`(子取得) / `0x42B470`・`0x42B460`(座標 getter ペア) / `0x6899C0`(**12B 構造体コピー** 3dword) / **`0x4D6760`（ecx=0xB33AC4・out byte フラグ付き refcheck）** / `0x446CB0`(**Push**・正典同定済) / `0x7C2A40`([0xB33AC4] refcount リリース) / **`0x44EE00`（self・3引数）** / `0x451A40`。

### 4.2 自再入の目的 [部分確定]（v359_3 完全デコード 0x44EFB0-0x44F058）

```
eax = entry ptr → ebx=[eax]（先頭）／next=[eax+4]
  ebx==0 → skip; bit5/bit11 テストで skip
  child = 0x4D7630(entry) → 候補1 = 0x42B460/0x42B470 の (座標A,座標B)
  predicate 0x4C97F0(child) 合格 → 候補=child、不然は 0x4EF130(other) 判定で選択
  0x4D6760(0xB33AC4, 選択体, &flag) → flag 検査
  → self へ再帰: call 0x44EE00(this 同一, arg1=選択体, arg2=旧arg, arg3=flag側)
```
- → **子オブジェクトを解決して自分自身を再帰呼び出しする探索ループ**（解決が尽きるまで再帰）。`[0xB33AC4]`（refcount 付き静的 obj・束③(v357)/v358 で確定済）の refcheck を経る点で「オブジェクト解決時の資源管理」と一体化。
- 外部 callers 2件（`ecx=[0xB33A98]`）＋ self の3引数形は、**arg 数が呼び出し経路で異なる**点は未解明（cdecl/stdcall 語義含め残点 V-359-2）。

### 4.3 正式名 [未確定] 維持

- 関数内文字列 **0件 [確定負]**（v357 確認）・annotate_db.json に **0x44EE00/callee 全滅**（372件中該当なし）・vtable 非会員（E8 のみ）→ RTTI 本来不適用。**機能記述「チェーン解決/リゾルバ系（リスト管理＋再帰探索＋refcount 整合）」を暫定表現とし、正式名は [未確定]**（v338 規約遵守）。

---

## §5 既知の補強（再利用事実）

1. **RTTI COL の pTD は +0xC**（本バイナリ・NiDX9 既知 COL で校正）→ 以後の RTTI 解析手順が確定。
2. **GridCellArray の全体像**: vtable 0xA3D79C（10スロット）・ctor 0x483350（width=dim≥5・w²×8B）・dtor 0x483460→0x483570・container は `new(0xAC)`（0x441970 ctor）・**保持＝[[0xB333A0]+8] ポインタ**・GridDistantArray（0xA3D7E0/ctor 0x4838F0）と同梱。
3. **`[0xB333A0]` の正体 [確定]**: 起動初期化 fn 0x406360 が `new(0xAC)` 経由で生成して格納する**container オブジェクトのポインタ**（v338 3.4「全体身元 [部分確定]」を拡張）。container 布局: +4=…／**+8=GridCellArray\***／+0x20/+0x24=座標（FUN_00444340 使用）／+0x34/+0x38/+0x3C/+0x54/+0x5C/+0x74 フィールド群（0x40xxxx 帯の多数サイトで使用）。
4. **annotate_db（372件）は 0x44EE00 帯・Grid 系を全否定して未登録**＝同定済み集合の外側が確認された。

---

## §6 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v359_1..v359_9 **全 EXIT=0**（v359_2/3/9 は pre-run の import/変数補正後・補正内容も証跡に記録） |
| 2 | RTTI 校正 | 既知 NiDX9 COL を実測して pTD=+0xC を確定してから4型名を解決（+8 前提の初回失敗を校正で是正） |
| 3 | 唯一経路の排他 | E8=0／u32=rdata スロット1件／abs 参照0／vtable +0x18 固有値確認の**4方向**で裏付け |
| 4 | 生成経路 | vptr 代入元のバイト走査（C7 05/B8-BF/生バイト）→ ctor 特定 → E8 caller → container ctor → global 格納の**全リンクを命令で接続** |
| 5 | v355/v357 との整合 | v355 の「0xA3D7E0 xref=callers」記述は**本裁定で訂正**（vptr 代入）。v357 §2.2 の境界・once ガード等は維持 |
| 6 | 判定段階 | [確定]（機構・本体・経路）/[部分確定]（caller 候補・0x44EE00 機能）/[未確定]（正式名2点）を混在させず分離 |

---

## §7 新規残点

| ID | 内容 | 深刻度 |
|---|---|---|
| V-359-1 | [部分確定] `call [reg+0x18]` 全74件の型帰属（GridCellArray 実 caller 群の完全列挙・候補 0x906BBE/0x9073FE） | 中 |
| V-359-2 | [未確定] 0x44EE00 の引数語義（外部1引数 vs self3引数の呼び出し規約） | 低 |
| V-359-3 | [未確定] 0x44EE00 の正式名（名称証拠なし [確定負]・新証拠手法が必要） | 低 |
| V-359-4 | [部分確定] container（[0xB333A0]）+0x4/+0x34/+0x54/+0x5C 等の残フィールド命名 | 低 |

---

## §8 main への報告

1. **V-355-1 [確定(機構)]**: FUN_00482200 の唯一経路は **GridCellArray vtable +0x18 の間接 call**（他経路は排他証明済）・個別 caller 候補2件（V-359-1 に残）。
2. **V-355-3 [確定] 完全解消**: `this`=**GridCellArray**（RTTI）・**vtable+0x1C＝FUN_00482240＝ClearCell（slot=NULL＋[this+0x20]=0）**・生成経路 `new(0xAC)→0x441970→[0xB333A0]→[+8]` まで全リンク。**v355 §4 の 0xA3D7E0 記述を訂正**。
3. **V-357-1 [部分確定]**: 自再入＝**子オブジェクト解決の再帰**・機能＝チェーン解決/リゾルバ系・正式名は証拠なしで [未確定]。
4. **副次**: RTTI pTD=+0xC の校正手順・GridCellArray vtable 全10スロット・`[0xB333A0]`＝container ポインタ（v338 の部分確定を拡張）。
5. docs/GT 反映候補（GridCellArray/ClearCell/RTTI 手順）は #47/#20 一括（L1 では docs 無編集）。

---

### 参照（本タスクの証跡）

- v359_1（テーブル域走査・0xA3D7E0 user コンテキスト・rdata ダンプ）
- v359_2（FUN_004441A0/004430F0 の this 源＝`[obj+8]`・C7 05 サンプル）
- v359_3（0x44EE00 全 call＋annotate_db 照合・self-call 領域完全デコード）
- v359_4（vptr 代入バイト走査＝table A の ctor 0x48348D 発見・FUN_00482240 本体＝**②の核心**・World=[0xB333A0] 確認）
- v359_5（COL レイアウト校正のための hexdump・+0xC 検出）
- v359_6（**4 RTTI 型名解決**・table A ctor head・0xB333A0 assigner 群）
- v359_7（GridCellArray slot0/dtor 形・container ctor FUN_00444C70 の owner 呼出・**`call [reg+0x18]` 全74件列挙**）
- v359_8（global 代入元 fn 0x406360＝`new(0xAC)` 経路・ctor 0x483350 完全・**4 vtable の +0x18 値比較＝固有性証明**）
- v359_9（container ctor 0x441970 の **`[esi+8]=GridCellArray*` ポインタ格納**の最終確認）
