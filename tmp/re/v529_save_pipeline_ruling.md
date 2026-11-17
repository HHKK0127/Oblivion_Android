# v529 — 保存系の縦の並び（`+0x20`/`+0x2C`＝`TESFile` レコード書き込み・`TESForm` 変更フラグ map）・`DAT_00B33B00`＝`TESSaveLoadGame` と `TESFile` の分離・`DAT_00b05e20`＝packed `'GRUP'` の確定（V-529-1..3 完遂・旧残点 V-406-3 継続第59弾）

- 採番: **v529**（旧残点群消化・probe prefix **v529_1..11 全11本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-529-1..3（v528 残点）** — 1=`+0x20`（`FUN_0046B340`→`FUN_00450A10`）semantic＋`FUN_00450190`/`FUN_004502C0`/`FUN_00452C20`/`FUN_00452D60` identity／2=`DAT_00B33B00` 正式クラス名＋`+0x30` list node 構造／3=predicate slot35 英名・`DAT_00b05e20`。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・RTTI（COL/TD 実読）・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-529-1: `FUN_00450190` の正体** | **[確定]（命令＋実文字列）→ 保存ファイル書き込みラッパ** | `__thiscall(this=writer, src=[esp+8], n=[esp+0x10])`、`ret 8`。`src == 0` なら `*this = 0; return 0`。そうでなければ `file = *(this+0x10)` の `vtable[+8](file, src, n, &written, 1)` を呼び、戻り `< n`（短書き）なら **`*this = 10`（ファイルエラーコード）＋`FUN_004A7A60("Write Error in TES file")`** を出して 10 を返す。成功なら `*this = 0` を返す。→ **`this` は「書き込み状態＋TESFile ポインタ」を持つ writer、`+0x10` が実ファイル** [確定]。 |
| **V-529-1: `FUN_004502C0` の正体** | **[確定]（命令＋実文字列）→ 親グループの確保と GRUP ヘッダ書き込み** | `this+0x284` の**グループノード単方向リスト**を歩き `form->vtable[+0xBC](node,1,1)` が偽のノードを探す → `while (form->virtual[+0xBC](node,0,1) == 0)` の間 `form->virtual[+0xC0](&hdr20, node)` で 20 バイトのグループヘッダを取り、**`hdr20[0] != DAT_00b05e20`（＝`'GRUP'`）なら `FUN_004A7A60("Failed to CreateGroupData for %s form '%s' (%08X)", (&PTR_DAT_00b05e04)[type*3], form->virtual[+0xD4](), form->+0xC)`** で失敗、そうでなければ **`FUN_00401F00(0x18)` でノード確保 → `FUN_00446CB0(node)`（ctor）→ 20 バイトコピー → `node+0x14` = ファイル位置（`[file+0x30]`／`-1` なら `[file+0x148]`）→ `FUN_00450190(this, node, 0x14)`（GRUP ヘッダ 20B を書く）→ `this+0x3D4++`**。→ **「form を格納する GRUP の連鎖を（無ければ作って）書き出す」** [確定]。 |
| **V-529-1: `FUN_00450980`（`+0x2C` の前半）** | **[確定]（命令）→ レコードヘッダの開始** | `FUN_004502C0(this, form)` → `*(this+0x270) = form->flags & 0x30EE0` → `*(this+0x268) = (&DAT_00b05e08)[type*3]`（＝**その type の packed FourCC**）→ `*(this+0x274) = form->+0xC`（formID）→ `*(this+0x26C) = 0`（dataSize プレースホルダ）→ `file->vtable[+0xC](0, DAT_00a853d8)`（seek）→ `*(this+0x27C) = 現在位置（[file+0x30]、-1 なら [file+0x148]）` → `*(this+0x280) = 0` → **`FUN_00450190(this, this+0x268, 0x14)` で 20 バイトヘッダを書く**。 |
| **V-529-1: `FUN_00450A80`（`+0x2C` の後半）** | **[確定]（命令）→ dataSize の確定とヘッダ再書き込み** | `*(this+0x26C) = *(this+0x280)`（dataSize 確定）→ `this+0x3D4++` → `file->vtable[+0xC](*(this+0x27C), DAT_00a853d0)`（記録開始位置へ戻す）→ `FUN_00450190(this, this+0x268, 0x14)`（ヘッダ再書き込み＝サイズのパッチ）[確定]。 |
| **V-529-1: `FUN_00450A10`（`+0x20` の本体）** | **[確定]（命令）→ レコード本体の吐き出しと後始末** | 引数は `ECX = writer`、`[esp+4] = form`（`0x46B340` が `mov ecx,[esp+8]`＝呼び出し元の第1引数、`push esi`＝this を渡す）。本体: `FUN_004502C0(writer, form)`（グループ確保）→ `file->vtable[+0xC](0, DAT_00a853d8)`（seek）→ **`FUN_00450190(writer, DAT_00b33c14, DAT_00b33c18)`（グローバル保留バッファの本体を書く）** → `DAT_00b33c14 != 0 && DAT_00b33c18 != 0` なら `writer+0x3D4++` → **`FUN_0046AF30()`（バッファ解放）** → ステータスを返す。`0x46B340` は `sete al` で **`== 0` → true** を返す。 |
| **V-529-1: `TESForm` vtable `+0x20`** | **[確定]（命令＋上記）→ `TESForm` の 1 form 保存（本体フラッシュ）** | `if (((flags>>0xE)&1) != 0) return false;`（bit14 は保存対象外）→ `virtual[+0x24]()`（`0x60D0A0` no-op stub）→ `return FUN_00450A10(writer, this) == 0;`。`ret 4`（引数 1）＝ **`bool SaveRecord(TESFile* out)` 相当 [確定]**。 |
| **V-529-1: `TESForm` vtable `+0x2C`** | **[確定]（命令）→ レコードヘッダ経路（bit5 で分岐）** | `if (bit14) return false;` → **`if (flags bit5) { FUN_00450980(writer); FUN_00450A80(writer); return true; }`** → そうでなければ **`jmp vtable[+0x20]`（同じ引数で本体フラッシュへテールジャンプ）**。`ret 4`。→ **「bit5 の form は 20 バイトヘッダのみ書いて終わる／それ以外は通常保存へ委譲」** [確定]。 |
| **V-529-1: `FUN_0046AF30`／保留バッファ** | **[確定]（命令）** | `FUN_0046AF30()` = `FUN_00401E40(DAT_00b33c14); DAT_00b33c14 = 0;`＝**グローバル保留バッファの解放**。`0x46AE40(tag, src, n)` は **`{4CC, u16 size}` の 6 バイトヘッダ＋データをバッファ末尾に追加**（`DAT_00b33c18` を成長、ヒープ `0xB02020` で `FUN_00401E60` 再確保）、`size > 0xFFFF` のとき **`0x58585858`（`"XXXX"`）分割サブレコード**を挿す [確定相当]。`0x46AED0` は既存バッファへの追記（同じ再確保経路）。 |
| **V-529-1: `FUN_00452C20`（`+0x40`/`+0x48` の終端）** | **[確定]（命令）→ 変更 form の登録** | `ebx = form->+0xC`（formID）→ `FUN_0055E000(manager, &entry)`（ハッシュ map 検索。`FUN_0055E000` は `[+4]`=hash／`[+8]`=compare／`[+0x10]`/`[+0x18]`=解放の vtable を回す汎用 map）→ 見つからなければ **`FUN_00401F00(8)` で `{u32 changedFlags, u32 blocked}` を確保し `FUN_00452570(manager, formID, entry)` で挿入** → **`if (entry[1] == 0) entry[0] |= flags;`**。→ **「form を変更済みとして manager の map に記録（flags を OR）」[確定]**。 |
| **V-529-1: `FUN_00452D60`（`+0x44` の終端）** | **[確定]（命令）→ 変更フラグの解除** | **`if ((*(manager+0x18) >> 0xC & 1) != 0) return 0;`**（保存処理中の gate）→ `FUN_0055E000(manager, &entry)` → `entry` があれば `if (entry[1]==0) entry[0] &= ~flags;` → **`if (entry[0] == 0) { FUN_004524C0(formID)（map から削除）; free(entry[1]); free(entry); }`** → 1 を返す。→ **「書き終えた form の変更フラグを落とし、空になったら map から消す」[確定]**。 |
| **V-529-1: `FUN_0045B670`（`+0x40` の gate）** | **[確定]（本文）** | `TLS[+0x185] != 0 && TLS[+0x184] == 0 && DAT_00b33a98+0xCD4 == 0 && form bit14 == 0` の全条件成立時のみ `FUN_0045A500()`（`+0x18` の thread 依存状態読み）→ 0 なら登録、さもなくば `(*(manager+0x18) >> 3 & 1) != 0 && *(manager+4) != 0` なら登録。→ **ロード中のみ「変更」を記録する** [確定]。 |
| **V-529-1: `FUN_0045B9A0`／`FUN_004534D0`** | **[確定]（本文＋実文字列）→ manager ストリームの write/read** | `FUN_0045B9A0(manager, src, n)`: `+0x18` の thread 依存バイト（同 thread＝bit0／他 thread＝`>>0x12`）で **ロード中なら `"Error: TESSaveLoadGame::SaveGameData() was called while loading.\n"`** を出し、**`_memcpy(*(manager+0x14), src, n); *(manager+0x14) += n;`**。`FUN_004534D0(manager, dst, n)`: **`_memcpy(dst, *(manager+0x14), n); *(manager+0x14) += n;`**。→ **`manager+0x14` は「現在の保存／読込ストリームのカーソル」で、`+0x50`/`+0x54` は 4 バイト flags をこのカーソル経由で読み書きする** [確定]。 |
| **V-529-2: `DAT_00B33B00` の正式クラス** | **[確定相当]→ `TESSaveLoadGame`** | 全メソッド（`FUN_0045B300` ctor 0x1C4B／`FUN_0045B670`／`FUN_0045B700`／`FUN_0045B780`／`FUN_0045B9A0`／`FUN_00453940`／`FUN_00452C20`／`FUN_00452D60`）の `this` が同 singleton。さらに **`FUN_0045B9A0` 自身が `"Error: TESSaveLoadGame::SaveGameData() was called while loading.\n"`（`0xA3A4E7`）を出す**＝自分のクラス名を名乗る唯一の一次証拠。同族文字列 `TESSaveLoadGame::SaveQueuedHavokData`（`0xA3A82C`→ref `0x45D30D`）／`SaveQueuedAttachedAnimation`（`0xA3A854`→`0x45D3F9`）／`LoadForm`（`0xA3B06C`→`0x46385F`）／`UnloadForm`（`0xA3B0B8`→`0x463B5D`）も同範囲の関数群を指す。RTTI TD は存在しない（**非多態**・機械走査 1529 TD 中 `SaveLoad`/`TESSave` 該当 0 件）→ **`TESSaveLoadGame` [確定相当]**。 |
| **V-529-2: 書き込み側オブジェクト B の正体** | **[確定相当]→ `TESFile`** | `+0x268`（20B レコードヘッダ）／`+0x284`（グループノード list）／`+0x3D4`（レコード数）を触る `this` は **ctor `FUN_00451DA0`（alloc `0x41C`、vtable ストア無し＝非多態）**。ctor は **`"File '%s' is not a valid TES file."`**（`FUN_004A7A60`）と **`"Could not close file '%s'."`**、`FUN_004515B0`（`lstrcpyA`／`_WIN32_FIND_DATAA` を使う**ファイル open**）・`FUN_004507D0`（close）を呼ぶ → **`TESFile` [確定相当]**。`+0x10` = 実ファイル、`+0x268` = `{type, dataSize, flags, formID, vc}` の 20 バイト（TES4 レコードヘッダと同型）。 |
| **V-529-2: manager `+0x30` list node 構造** | **[確定]（命令）** | `FUN_00453940(manager, form)`: `p = manager+0x30; while (p != 0) { if (*p == form) break; p = p[1]; } FUN_0065C620(head, form)`。`FUN_0065C620` は `{item@0, next@4}` の**センチネル付き単方向リストからの削除**（ノードは `FUN_00401F20` で free）。→ **ノード＝2 dword `{void* item; node* next;}`、リスト先頭は manager `+0x30` に埋め込み** [確定]。 |
| **V-529-3: `DAT_00b05e20` の正体** | **[確定]→ packed `'GRUP'`** | ① アドレス算術: `0xB05E20 = 0xB05E04 + 12*2 + 4`＝**`0xB05E04` 表の index 2（`GRUP`）の第2 dword（f1）**。② **`0x46B961`（20 バイト GRUP ヘッダ組み立て）: `esi = [0xB05E20]; mov [eax], esi`**（ヘッダ先頭＝GRUP の 4CC）＋`[eax+8] = (&DAT_00b05e08)[type*3]`（ラベル＝form type の 4CC）。③ `0x4519D4`: 読み込んだレコード先頭 dword を `[0xB05E20]` と比較＝「次は GRUP か」。同型の `[0xB05E14]` 比較（`0x451982`/`0x45199C`）は「TES4 ヘッダか」。→ **`DAT_00b05e20` = packed `'GRUP'`（`0xB05E14` = packed `'TES4'`）** [確定]。 |
| **V-529-3: `0xB05E04` 表のフィールド意味（v528 の記述を訂正）** | **[確定]（全69件＋機械走査）** | レイアウトは **`{char* name; u32 packedFourCC; u32 indexPlusOne;}`（stride 12）**。ディスク上 `f0`＝`"NONE"`…`"TOFT"` の文字列ポインタ、**`f1`＝0（実行時に packed 4CC が入る）**、`f2`＝index+1。**`f1` は読み出し専用サイトのみ**（`[ecx*4+0xb05e08]` の形で `FUN_00450980`／`FUN_0046AF70`／`0x46B976`／`0x46B9D6`／`0x46C4F9`／`0x4519B7` が読む）で、値は使用側の比較から `0xB05E14`=`'TES4'`／`0xB05E20`=`'GRUP'` と確定。**v528 の「f1＝実行時充填の別ポインタ」は誤りで、正しくは「packed FourCC」**（`f0` が名前文字列、`+0x74` の名前生成が使うのは `f0`）。 |
| **V-529-3: `TESForm` vtable `+0x30`（旧 slot35）** | **[確定: 命令]／[推定: 名称]→ グループヘッダ membership predicate** | 引数は **20 バイトのグループヘッダ**。`if (arg == 0) return 0;` → **`if (*arg != 'GRUP') return 0;`** → `switch (arg[3])`（＝**groupType**）: `case 0:` `this->type > FUN_0046AF70(arg[2])`／`case 1,4,5:` `type < 0x35`／`case 2,3,6:` `type < 0x30`／`case 7:` `type < 0x39`。`arg[2]`＝**グループラベル（packed 4CC）** を `FUN_0046AF70` で type index に変換して比較。→ **「この form はこの GRUP ヘッダに入るか」を返す述語 [確定: 命令]／名称（`IsInGroup` 相当）は [推定]**。 |

## §2 証拠

### §2.1 `+0x20` の実命令（`0x46B340`）
```
46B340: push esi / mov esi,ecx
46B343: mov eax,[esi+8] / shr eax,0xE / test al,1 / je 46B353
46B34D: xor al,al / pop esi / ret 4                  ; bit14 -> false
46B353: mov edx,[esi] / mov eax,[edx+0x24] / call eax ; virtual[+0x24] (no-op stub)
46B35A: mov ecx,[esp+8]      ; ECX = caller's 1st arg  = writer (TESFile)
46B35E: push esi             ; arg = this (TESForm)
46B35F: call 0x450A10
46B364: test eax,eax / sete al / pop esi / ret 4     ; true when write status == 0
```

### §2.2 保存ファイル書き込み（`0x450190`）
```
450190: push esi / mov esi,ecx                  ; esi = writer
450193: mov ecx,[esp+8]                         ; src
450197: test ecx,ecx / je 4501E0                 ; src==0 -> status 0, return 0
45019B: mov eax,[esi+0x10]                      ; file object
45019F: mov edi,[esp+0x10]                      ; n
4501A3..4501AB: push 1 / lea edx,[esp+0x10] / push edx / push edi / push ecx / push eax
4501B8: call [eax+8]                            ; file->Write(file, src, n, &written, 1)
4501BD: cmp eax,edi / jae 4501E0
4501C7: push 0xA38BC0 / mov [esi],0xA / call 0x4A7A60   ; "Write Error in TES file"
4501DD: ret 8
```

### §2.3 20 バイトレコードヘッダ（`0x450980` / `0x450A80`）
```
FUN_00450980(writer, form):                     ; 0x450980
  FUN_004502C0(writer, form)
  [writer+0x270] = form->flags & 0x30EE0
  type = *(u8*)(form+4);  [writer+0x268] = (&DAT_00b05e08)[type*3]   ; packed FourCC
  [writer+0x274] = *(u32*)(form+0xC)                                  ; formID
  [writer+0x26C] = 0                                                  ; dataSize placeholder
  file->virtual[+0xC](0, DAT_00a853d8)                                ; seek
  [writer+0x27C] = pos ([file+0x30], -1 -> [file+0x148])
  [writer+0x280] = 0
  FUN_00450190(writer, writer+0x268, 0x14)                            ; write 20B header
FUN_00450A80(writer):                           ; 0x450A80
  [writer+0x26C] = [writer+0x280]; write+0x3D4 += 1
  file->virtual[+0xC]([writer+0x27C], DAT_00a853d0)
  FUN_00450190(writer, writer+0x268, 0x14)                            ; rewrite header
```

### §2.4 GRUP ヘッダ組み立て（`0x46B950`）と `DAT_00b05e20`
```
46B950(header*, flag):
  if (header == 0 || flag != 0) return
  esi = [0xB05E20]                 ; packed 'GRUP'
  [header+0x0] = esi               ; record type
  [header+0xC] = 0                 ; groupType
  [header+0x8] = (&DAT_00b05e08)[this->type*3]   ; group label = form type FourCC
  [header+0x4] = 0                 ; size
  [header+0x10] = 0
0x4519D4: cmp eax,[0xB05E20]   ; read record type == 'GRUP' ?
0x451982: cmp eax,[0xB05E14]   ; read record type == 'TES4' ?
```

### §2.5 `TESFile` の正体（ctor `FUN_00451DA0`、alloc `0x41C`）
```
0x461391: push 0x41C / call 0x401F00   ; alloc
0x4613AF: mov ecx,eax / call 0x451DA0  ; ctor
ctor: no vtable store (non-polymorphic)
      +0x284/+0x288 = 0 (group list), +0x3E0..0x3EC/+0x404.. = 0
      +0x22C/+0x230/+0x234 = 0, +0x258/+0x25C/+0x260 = 0
      +0x8A = DAT_00b055CC, _memset(+0xA4, 0, 0x140), +0xF6 = 0x800
      FUN_004515B0(fileName, ext, 0)                  ; open (lstrcpyA / FindFirstFileA)
      on failure: "File '%s' is not a valid TES file."
                  "Could not close file '%s'."        (FUN_004507D0 close)
```

### §2.6 manager の変更 map（`FUN_00452C20` / `FUN_00452D60`）
```
FUN_00452C20(manager, form, flags):
  id = *(u32*)(form+0xC);  FUN_0055E000(manager, &entry)      ; hash map lookup
  if (!found) { e = alloc(8); e[0]=e[1]=0; FUN_00452570(manager, id, e); entry = e; }
  if (entry[1] == 0) entry[0] |= flags;                       ; {changedFlags, blocked}
FUN_00452D60(manager, form, flags):
  if ((*(manager+0x18) >> 0xC) & 1) return 0;                 ; saving gate
  FUN_0055E000 lookup; if (entry) { if (entry[1]==0) entry[0] &= ~flags;
     if (entry[0]==0) { FUN_004524C0(id); free(entry[1]); free(entry); } return 1 }
```

### §2.7 manager ストリームカーソル（`+0x14`）
```
0x46B5E0 (TESForm vtable +0x50, raw):
  test byte [esp+4],1 / je ret
  add ecx,8           ; &this->flags
  push 4 / push ecx
  mov ecx,[0xB33B00]  ; ECX = TESSaveLoadGame
  call 0x45B9A0       ; Write(manager, &flags, 4)
0x46B600 (+0x54): ECX = [0xB33B00] -> call 0x4534D0 (Read(manager, &tmp, 4))
FUN_0045B9A0: thread-gated +0x18 -> "Error: TESSaveLoadGame::SaveGameData() was called while loading.\n"
              memcpy(*(manager+0x14), src, n); manager+0x14 += n
FUN_004534D0: memcpy(dst, *(manager+0x14), n); manager+0x14 += n
```

## §3 判定の内訳

- **V-529-1 完遂**: `+0x20`／`+0x2C` の保存経路を命令で最後まで追い、`FUN_00450190`（Write Error in TES file）・`FUN_004502C0`（Failed to CreateGroupData）・`FUN_00450980`/`FUN_00450A80`（20B ヘッダと dataSize パッチ）・`FUN_00450A10`（本体フラッシュ＋バッファ解放）・`FUN_00452C20`/`FUN_00452D60`（変更 map の OR／クリア）・`FUN_0045B670`（ロード中 gate）・`FUN_0045B9A0`/`FUN_004534D0`（manager ストリーム）を全て [確定]。
- **V-529-2 完遂**: `DAT_00B33B00`＝**`TESSaveLoadGame`**（自前のエラー文字列が一次証拠）、書き込み側 B＝**`TESFile`**（alloc 0x41C・"File '%s' is not a valid TES file."）、manager `+0x30`＝**センチネル付き単方向リスト `{item,next}`** [確定]。
- **V-529-3 完遂**: `DAT_00b05e20`＝**packed `'GRUP'`** [確定]（アドレス算術＋GRUP ヘッダ組み立て＋読み込み側の比較）、`+0x30`＝**グループヘッダ membership predicate** [確定: 命令]／名称 [推定]。あわせて **`0xB05E04` 表のフィールド意味を訂正**（f0=名前文字列、f1=packed 4CC、f2=index+1）。

## §4 残点（V-530-1..3）

1. **`TESFile+0x280` が常に 0**（`.text` に加算サイトが無く、`mov 0` と read のみ）→ この経路の `dataSize` は 0 で書かれ、本体は `TESSaveLoadGame+0x14` カーソル側が担う。**保存完了時のサイズ確定／`0x26C` の後処理を manager 側で特定**する【中】
2. グループノード（`0x18` バイト、ctor `FUN_00446CB0`）の `+0x14` の意味（ファイル位置か）＋ノード ctor の identity【低】
3. `+0x30` の `groupType` 0..7 の各意味（0=top／1=world children／2,3=interior block/subblock／4,5=exterior block/subblock／6=cell children／7=temporary children の対応）と `case 0` の不等号方向【低】

## §5 後続への申し送り

- **保存レコードは二層**: ①`TESFile` が 20 バイトヘッダ（`type=packed FourCC`／`dataSize`／`flags & 0x30EE0`／`formID`／vc）を書く（`+0x2C`）、②本体は `TESSaveLoadGame` のストリームカーソル `+0x14` 経由で `+0x50`/`+0x54` が 4 バイト単位で読み書きする、③`+0x20` がグローバル保留バッファを吐いて解放する。**移植側はこの 3 点を分けて実装しないと整合しない。**
- **`0xB05E04` 表は `{name, packed4CC, index+1}`**（f1 は実行時充填）→ 移植側で「type byte → 4 文字レコード署名」を引くときは **f1 相当（packed）を自前で持つ**のが正しい（v528 の記述は f1 を別ポインタと誤記 → 本裁定で訂正）。
- **`+0x30` がグループヘッダ述語**と分かったので、保存時の「どの GRUP に入るか」判定を移植側で再現できる（`0x46B950` の GRUP ヘッダ組み立てと対）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
