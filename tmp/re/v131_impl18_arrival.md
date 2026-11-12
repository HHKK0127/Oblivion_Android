# V131: +0x18 ハンドラの band2/band3 実行時到達経路と共有 impl 意味論

- 日付: 2026-10-26（opencode サブエージェント・分析専任／実装・docs 編集なし）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416, ImageBase 0x400000, .text VA 0x401000 / raw 0x400）
- 出典: `tmp/re/v94_boundary_dups.md` §e **R-a / R-c / R-d**、併せて `tmp/re/b20_impl_5071d0.md`（V116）と突合
- 前提確認: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**
- 突合依頼の `tmp/re/v132_exception_dispatcher.md` は **リポジトリに存在しない**（`tmp/re` の glob 確認済み）→ 突合対象なし
- スコープ: V116（idx496 0x5071D0）は既出のため再調査せず、引用のみ

## 結論（3 行）

| 残点 | 判定 | 要旨 |
|---|---|---|
| **R-a** band2/band3 の `+0x18` 到達経路 | **解明** | 実行時 executor `FUN_005169a0` は opcode∉[0x10,0x1E] を **`0x517036 → call FUN_004FCA30(opcode)`（opcode→エントリ先頭解決）→ `0x517299 mov ecx,dword[eax+0x18]`（相対 disp）→ `0x5172C6 call ecx`（8 引数）** で実行する。アンカー 0 件だったのは「絶対 imm32 を使わない」ためで、v94 の残アンカー `0x5E8725` は **バイト重なりの偽陽性**（実アンカーは 0x516A5D の 1 件のみ）。 |
| **R-c** 0x500890（omc/rmc）共有 impl | **解明** | リテール本体は **MEM_DEBUG ガードのスタブ**: 引数デコード（FAE80）→".xls"拡張子補完（結果はデッド）→ 常にコンソールへ **"This function only works in MEM_DEBUG"** を出力して **常に戻り 1**。opid/argn 依存の分岐は 0 件 ⇒ **rmc と omc の観測動作は完全同一**（ヘルプ文字列の宣言のみ相違）。 |
| **R-d** 0x50D020（AdvSkill/ModPCSkill）共有 impl | **解明** | 全ディスパッチ列が一致（相違は name/alias/+0x0C ヘルプの識別子のみ）、かつ **opid は impl の 8 引数に一切渡らない** ⇒ 本体内に差分コードは**構造的に存在し得ない**。**AdvancePCSkill ≡ ModPCSkill は実行時完全同値**（本体は FAE80 で (ActorValue, Amount) をデコード → Amount 回の +1 増分ループ＋レベルアップ計上 → 通知 → 戻り 1 の単一コードパス）。 |

---

## §0 入力・手法・生成物

### 入力（読むだけ・編集なし）

| パス | 用途 |
|---|---|
| `tmp/re/v94_boundary_dups.md` | R-a/R-c/R-d の定義、アンカー走査結果（§a.5/§c/§e） |
| `tmp/re/b20_impl_5071d0.md` | executor 0x5169A0 の引数契約（arg8/arg9）、フェッチ 0x516830、opcode 0x10=Begin、call site 0x517295 |
| `%TEMP%\opencode\probe_harness.py` | `Ctx.rd()` セクション対応読み取り（selftest 4/4） |
| `%TEMP%\opencode\func_list_new.txt` | 関数境界（FCA30=0x4FCA30..0x4FCA70、executor callers=0x517330/0x517640 等） |
| `docs/Oblivion_decompiled.c` | 部分検索のみ: `FUN_00579b60` 定義 L281899、`FUN_004fae80` 定義 L194575、`FUN_004fca30` 呼出し L194523 |

### 手法

- capstone 5.0.7（skipdata=True / md.detail）。ファイルオフセット = VA − 0x400C00（.text）。**注意**: 本タスクで 2 本のスクリプトを VA=off+0x401000 誤りで一度だけ実行したが、**selftest と既知値（0x4FCA30 本体の lea 定数）で検出後に修正・全再実行**済み。以下はすべて訂正後の値。
- opcode テーブル 532 件を全項目（10 dword）再読して idx/opid/flagA/argn/argdesc/impl/dispatcher/cond/f24 を確定（`v131_rows.json`）。
- .text 全域の imm32 アンカー再走査（フィールド分類 + 特殊値）→ v94 の既知アンカーを全再現。
- E8 直接呼出しの全数スキャン（FCA30/0x9840E0/0x579B60/0x579C10）。

### 生成物（すべて %TEMP%\opencode、新規 23 件）

スクリプト: `v131_1_table.py`(3,235B) / `v131_2_anchors.py`(2,191) / `v131_3_disp.py`(767) / `v131_4_fca30.py`(888) / `v131_5_impls.py`(941) / `v131_6_names.py`(1,717) / `v131_6b_callers.py`(673) / `v131_7_fetch.py`(876) / `v131_8_strings.py`(1,784) / `v131_9_deep.py`(2,748)
出力: `v131_disp5169a0.txt`(42,741B / 730 insn) / `v131_fca30.txt`(831 / 14) / `v131_callers.txt`(22,342 / 375) / `v131_impl_500890.txt`(5,711 / 97) / `v131_impl_50d020.txt`(15,942 / 281) / `v131_fetch_fca30.txt`(7,401 / 128) / `v131_4fad43.txt`(3,450 / 60) / `v131_fae80_head.txt`(2,588 / 43) / `v131_9840e0.txt`(3,296 / 61) / `v131_579b60.txt`(3,729 / 73) / `v131_5e8725.txt`(1,612 / 29) / `v131_anchors_out.txt`(59,976 / 65 hit) / `v131_rows.json`(94,826)
**repo 内の新規ファイルは本レポート 1 件のみ**（docs/・既存 tmp/re・TODO・memory・plan.md は未編集、git 未実行、ESM 再スキャンなし）。

---

## §1 【R-a】band2/band3 の実行時到達経路 — **解明**

### 1.1 結論: opcode クラス別の実行経路（executor `FUN_005169a0` = 0x5169A0、呼出し 2 件のみ）

executor の入口（`v131_disp5169a0.txt`）:

```
0x5169FC  mov ecx, [esp+0x764]            ; ecx = arg2 = フェッチ済み opcode
0x516A03  cmp ecx, 0x10
0x516A1D  jne 0x516b55                    ; opcode != 0x10 → 下記 (B)(C)
   (A) opcode == 0x10（Begin ブロックヘッダ、b20 既出）:
0x516A23  movsx ecx, word [edi+eax]       ;   eax = arg5 = PC（ヘッダ消費後）= ペイロード先頭
0x516A27  cmp ecx, 0x1e                   ;   ← v94 が「opid ≤ 0x1E 限定」としたゲート。
0x516A31  ja  0x516acf                    ;     正しくは「フェッチ opcode ではなく Begin ペイロードの
                                           ;     i16（イベント idx）」のゲート。>0x1E → 即 return 0
0x516A3E..0x516A5D  lea/add … mov ecx,[eax+eax+0xb0af5c]（argdesc）
                                   mov ecx,[eax+0xb0af60]（★+0x18 絶対 = 表[idx]、idx = ペイロード）
0x516AF3  call ecx                        ;   band1（0x00–0x1E）イベントハンドラ実行
   (B) opcode ∈ 0x11–0x1E:
0x516B63  add ecx,-0x10 / cmp ecx,0xe / ja 0x517036   ; range 外 → (C)
0x516B6F  movzx ecx,[ecx+0x517320] ; jmp [ecx*4+0x517300]  ; 式/条件評価のスイッチ（'G'/'r' 型レコード読取）
   (C) opcode ∈ band2/band3 その他（0x00–0x0F・0x1F–0xFF・0x100–0x182・0x1000–0x1170）:
0x517036  push [esp+0x764]                ; opcode
0x51703E  call 0x4FCA30                   ; ★ opcode → エントリ先頭
0x517046  test eax,eax / je 0x51704a      ; 0 = 未知 opcode → エラー文 0xA4DB98
                                           ;   "Unable to find function definition for command %d in script '%s'."
0x5170C9  cmp byte [eax+0x10],0           ; entry.flagA（下位バイト = "requires reference"、§5 N-d）
0x5170CD  mov ecx,[eax+0x14] / mov [esp+0x18],ecx   ; entry.argdesc（→ impl の arg1 へそのまま渡る）
0x5170D4  je  0x517171 …                  ; flagA!=0 かつ ref 無しならエラー 0xA4DB58 "…requires a reference."
0x517171  cmp byte [esp+0x77c],0 / je 0x517295       ; arg8==0 = 本命パス（b20: arg8!=0 経路は v1.2.0416 到達不能）
0x517295  cmp byte [eax+0x25],0           ; entry+0x25 → なら this+0xA1=1（§5 N-c）
0x517299  mov ecx, dword ptr [eax+0x18]   ; ★★ +0x18 IMPL（**相対 disp32 = 0x18**）
0x5172A5  test ecx,ecx / je 0x516acf      ; impl==0（5 件）→ return 0
0x5172C6  call ecx                        ; ★★ ハンドラ実行（8 引数, add esp,0x20）
0x5172C8  test al,al / je 0x516acf        ; 失敗 → return 0 ／ 0x5172D3: mov al,1 → return 1
```

- **band2（0x100–0x182）・band3（0x1000–0x1170）の全 opcode は、この (C) 経路で `+0x18` ハンドラへ到達する。**
  よって v94 §e R-a で未同定だった **class3 3 件（0x0152 idx113 / 0x011A idx57 / 0x0124 idx67 = band2）と §b 9 件（0x015A idx121 = band2、0x1005 idx168 / 0x1023 idx198 / 0x1041 idx228 / 0x10BC idx351 / 0x112A idx461 / 0x112B idx462 = band3、0x0000 idx0 / 0x0007 idx7 は band1 で (A) 経由）の到達経路はすべて同定**した。
- 本命経路の生きた引数: 呼出し元 `FUN_00517330`（arg8=0, arg9=0）が `push` 9 個（末尾 push edi=arg1=スクリプトオブジェクト, arg2=opcode, …, arg8=0, arg9=0）。→ b20 §3.4 の裁定（EssentialDeathReload=0x114D がこの経路で `call ecx`）と**完全一致**。

### 1.2 `FUN_004FCA30`（opcode → エントリ先頭、実体 0x40 B / 14 insn）

```asm
0x4FCA30  mov ecx, [esp+4]                 ; opcode
0x4FCA34  lea edx, [ecx - 0x100]
0x4FCA3C  cmp edx, 0x82 / ja 0x4fca53      ; band2: 0x100 ≤ op ≤ 0x182
0x4FCA44  lea eax, [ecx + ecx*4 - 0x500]   ;   5*(op-0x100)
0x4FCA4B  lea eax, [eax*8 + 0xb0b420]      ;   + 40 → 0xB0B420 = &entry[31] = base+31*0x28
0x4FCA52  ret                              ;   ⇒ &entry[op-0xE1]  （v94 の想定 delta 0xE1 と一致）
0x4FCA53  lea edx, [ecx - 0x1000]
0x4FCA59  cmp edx, 0x170 / ja 0x4fca6f     ; band3: 0x1000 ≤ op ≤ 0x1170
0x4FCA61  lea eax, [ecx + ecx*4 - 0x5000]  ;   5*(op-0x1000)
0x4FCA68  lea eax, [eax*8 + 0xb0c8c0]      ;   + 40 → 0xB0C8C0 = &entry[163] = base+163*0x28
0x4FCA6F  ret                              ;   ⇒ &entry[op-0xF5D]  （v94 の想定 delta 0xF5D と一致）
  それ以外 → eax=0
```

- 範囲はテーブルの opid 分布と**厳密に一致**: 本セッション実測の delta run は `idx0..30: opid=idx / idx31..162: opid=idx+0xE1 / idx163..531: opid=idx+0xF5D`（band2 最大 0x183 = idx162 は `name="ADD NEW CONSOLE COMMANDS"`・`impl=0` の**区切りセンチネル行**で FCA30 の範囲外＝到達不可、band3 最大 0x1170 = idx531 で範囲上限 0x170 と一致）。
- **FCA30 の E8 直接呼出しは 3 件のみ**（訂正後スキャン）:
  1. `0x0051703E` — **executor の (C) 経路（唯一の実行時到達点）**
  2. `0x005168EA` — `FUN_00516830`（フェッチ）内。ref 解決失敗時のエラー報告用: エントリ名を取って `0x5167A0` へ `"Unknown reference object for function '%s'.  Execution of script '%s' stopped."`(0xA4DB08) / `"Unknown reference object. …"`(0xA4DAC8) を出力 → **実行到達ではない**。
  3. `0x004FAD43` — コンパイラ側（型文字 `'X'`(0x58) の直後に来る u16 を opcode として解決し `entry+0x14 argdesc` を読む引数検証）→ **実行到達ではない**。
- （参考）同じ `lea eax,[eax*8+0xb0b420]` / `+0xb0c8c0` 定数は 0x4FDC5F/0x4FDC7C・0x4FFF57/0x4FFF71 にも**インライン展開**で存在（いずれもコンパイラ領域 0x4FC000–0x500000）。実行時でこの定数を使うのは FCA30 だけ。

### 1.3 なぜ v94 のアンカー走査で band2/band3 が 0 件だったのか（3 理由＋1 訂正）

1. **相対アドレッシング**: 実行時の読み出しは `mov ecx,[eax+0x18]`（eax = FCA30 が返すエントリ先頭）で **imm32 を含まない** → 絶対アンカー走査の原理的に検出不能。バンド1 経路だけが `[idx*0x28 + base+0x18]` の絶対形（imm32 0xB0AF60）を使う。
2. **FCA30 の定数は field-0 として分類される**: 検出される imm32 は `0xB0B420`（base+31*0x28）と `0xB0C8C0`（base+163*0x28）で、v94 の分類では **`+0x00 name` 列（idx31/idx163）に混入**していた。「band2/band3 用アンカー（base+0x18−0xE1*0x28 = 0xB08C38 系 / base+0x18−0xF5D*0x28 = 0xAE48D8 系）は 0 件」は正しいが、それは **opid を scaled index にして field を bake する命令形が存在しない**ことを意味するだけで、到達不能を意味しない。delta は `lea … -0x500 / -0x5000` の即値側に折り込まれている。
3. **band1 経路のゲートは「opcode」ではなく Begin ペイロード**: `cmp ecx,0x1e` は (A) 経路に入った後の `movsx ecx,word[edi+eax]`（イベント idx）に対するもの。v94 の結論（band1 限定）は正しいが、比較対象の解釈のみ訂正。
4. **【訂正】残アンカー 0x005E8725 は偽陽性**: その位置の実命令は `0x5E8724 ff d0  call eax` + `0x5E8726 e9 b0000000 jmp 0x5e87db` で、imm32 読み取り（0x5E8725 起点）は `d0 e9 b0 00` = **命令境界をまたいだバイトの偶然一致**（`call eax` の最終バイト + `jmp rel32` 先頭 3 バイト）。`0x5E8710 cmp byte[eax+0x26],6 / 0x5E871A-0x5E8724 vtable(+0x1a4) 呼出し` の文中にあり、テーブル参照は存在しない。
   ⇒ **`.text` 全域で `+0x18` 絶対アンカーは 0x516A5D の 1 件のみ**（v94 の「2 件」のうち 1 件が誤検出）。バンド1 経路＝絶対形、バンド2/3 経路＝相対形という非対称が確定した。

### 1.4 executor の 8 引数契約（impl 側から見た観測）

`0x5172AD..0x5172C6` の push 順（末尾が arg1）:

| impl 引数 | 実体 | 備考 |
|---|---|---|
| arg1 | **`entry+0x14 argdesc`**（`[esp+0x18]` 経由。rmc の場合は **NULL**） | FAE80 の第 1 引数（§2） |
| arg2 | executor の ebp = 呼出し元 arg1 = スクリプトオブジェクト | impl 内で `mov edi,[ebp+0x30]` = コードバッファ |
| arg3–arg7 | `[esi]` / `[esi+8]` / PC ポインタ / argLength ポインタ等の実行コンテキスト | |
| arg8 | バイト列カーソル容器（`lea [esp+0x14]`） | b20 §3.3 と一致 |
| **opcode は 8 引数に含まれない** | `[esp+0x764]`（executor の arg2）は call 直前の push に**参照されない** | §3 の決定的根拠 |

バンド1 経路（0x516ADA..0x516AF3）も arg1 = argdesc（`[esp+0x18]` = 0x516A59 格納分）で同一契約。

---

## §2 【R-c】0x500890 OutputMemContexts / ResetMemContexts 共有 impl — **解明**

### 2.1 本体の実行フロー（323 B / 84 insn、`v131_impl_500890.txt`）

```
1. ローカル buf(S0+4, 22 B) に定数 "OutputMemContexts.xls"(0xA4B818..0xA4B82D) をコピー（無条件・初期値）
2. call 0x4FAE80(arg1=argdesc, …, buf)     ; 型付き引数デコーダ（§2.3）
   ├ 成功 → buf はデコード結果（omc の String 引数 = ユーザー指定ファイル名）で上書きされ
   │        0x500977 へジャンプ（初期値リセットをスキップ）
   └ 失敗 → buf を定数 "OutputMemContexts.xls" にリセット（0x50093B..0x500972）
3. 0x500977: strstr(buf, ".xls")（0x9840E0、65 呼出しの CRT 相当。戻り 0=未発見）
   └ 未発見なら strlen 後に ".xls\0" を追記（0x50098D..0x5009AC）
4. 0x5009AF: push 0xA4B7E8 ; call 0x579B60     ; 常に実行
        → "This function only works in MEM_DEBUG" をコンソール出力
5. mov al,1 / cookie check / ret              ; 常に成功 (戻り 1)
```

- **分岐は 2 つとも「引数内容」起因のみ**（FAE80 成否 / strstr 成否）。**opid・argn・entry を読むコードは本体に 0 件**（opcode は §1.4 の通り impl に渡らない）。
- v94 の記載「本文は OutputMemContexts.xls 固定・ResetMemContexts 用の分岐が無い」を確認した上で、**R-c の問い「rmc が omc と同じ動作になるのか、ゲート FAE80 の引数差で分岐するのか」への回答**: ゲートは argn 差（実体は argdesc の NULL 有無）で**内部経路だけ分岐するが、合流後の観測動作は同一**。

### 2.2 omc と rmc の実経路の違い（内部のみ・観測同一）

| | idx116 OutputMemContexts (omc) | idx136 ResetMemContexts (rmc) |
|---|---|---|
| flagA/argn dword(+0x10) | 0x00010000（flagA=0, argn=1） | 0x00000000（flagA=0, argn=0） |
| argdesc (+0x14) | 0xB0A690（"String" 1 個） | **NULL** |
| FAE80 | arg1=非 NULL → デコード実行（u16 argc 回ループ、type 表 0xB0A54D）→ 成敗はバイトコード次第 | **arg1=NULL → 即 ret 0**（FAE80 冒頭 `cmp [esp+0x1c],0 / je xor al,al; ret` を実測） |
| buf 経路 | デコード結果（ファイル名）→ .xls 補完（**その後 buf は未使用 = デッド**） | 初期値 "OutputMemContexts.xls"（ strstr で .xls 発見 → 補完スキップ） |
| **観測動作** | **"This function only works in MEM_DEBUG" 出力 + 戻り 1** | **同左（完全同一）** |

- 実際の「メモリコンテキスト出力／リセット」の本処理は **MEM_DEBUG ビルド専用でリテール v1.2.0416 には未実装**。ファイル名ロジック（.xls 補完）は本処理不在のため**到達後デッド**（末尾の `0x579B60` は書式に `%` を含まない定数のみを渡すため vararg 読み出しなし = decompiled L281899 の `FUN_00579b60(undefined4 param_1)` 定義で確認）。
- **ヘルプ（+0x0C 列）のみ宣言上相違**: omc=`"Output Mem Context info to a file"` / rmc=`"Reset Max Mem Contexts"`（+0x0C = コンソール help 文字列と確定。0xA2F7EC = 空文字が既定）。
- ⇒ **判定: 共有は正当な「リテール未実装スタブの二重登録」。rmc が omc と同一動作になる（表宣言と実装の落差）ことを証明。ゲート分岐はデッドデータ（buf）にのみ影響。**

### 2.3 補助同定

- `FUN_004FAE80`（定義 decompiled L194575）= 型付き引数デコーダ: 冒頭 arg1(NULL) チェック → コンテナ先頭の **u16 argc** を読んで argc 回ループ、各引数を argdesc レコード（12 B: {char* name, u32 type, u32 flags}、実測 idx375 = "Actor Value"(type5) / "Amount"(type1)）でデコード、type は表 `byte[eax*8+0xb0a54d]` で分岐。tail は消費しない（b20 と一致）。
- `0x9840E0` = `strstr` 相当（needle の先頭 2 バイト展開ループ、未発見で xor eax,eax; ret）。名前は CRT のため decompiled に `FUN_009840e0` 定義なし。
- `FUN_00579B60(param_1)`（decompiled L281899）= コンソール/メッセージ出力（`FUN_00582160` ゲート → `FUN_00585C10/00585C90` へ書式付き転送、E8 呼出し 516 件の汎用 print）。

---

## §3 【R-d】0x50D020 AdvancePCSkill / ModPCSkill 共有 impl — **解明**

### 3.1 差分コードは「存在しない」— 3 つの独立証拠

1. **全ディスパッチ列が一致（本セッション全項目実測）**

   | 列 | idx375 AdvancePCSkill | idx434 ModPCSkill |
   |---|---|---|
   | +0x00 name | 0xA4EE08 "AdvancePCSkill" | 0xA4E694 "ModPCSkill"（**識別子のみ相違**） |
   | +0x04 alias | "AdvSkill" | "ModPCS"（識別子） |
   | +0x08 opid | 0x10D4 | 0x110F（**impl に渡らない = §1.4**） |
   | +0x0C help | 0xA2F7EC = ""（空） | 0xA4E65C = "Modify the Players Skill. [modpcs Blunt, 10]"（**help 文字列のみ相違**） |
   | +0x10 flagA/argn | 0x00020000（flagA=0, argn=2） | **同値** |
   | +0x14 argdesc | 0xB0A900 | **同一ポインタ**（"Actor Value","Amount"…） |
   | +0x18 impl | 0x50D020 | **同一** |
   | +0x1C dispatcher | 0x4FDE30（標準） | **同一** |
   | +0x20 cond / +0x24 | 0 / 0 | **同値** |

   ⇒ コンパイラ側（+0x1C thunk）も実行側も両 opcode を**同一形式のバイトコード・同一 impl** に落とす。相違は opid の 2 バイトのみで、これは FCA30 で入口解決した時点で消費され impl へは届かない。

2. **impl の 8 引数に opcode/entry が無い**（§1.4 の push 実測）。本体全文（281 insn）を通読したが、テーブル・opid・グローバル経由の「どちらのコマンドか」を取得するコードは存在しない。分岐はすべて FAE80 がデコードした値（カウンタ `[S0+0]` / 値スロット `[S0+0xC]`）と、対象オブジェクト・スキル値・レベルアップ閾値のみに依存。

3. **v94 の「差分未特定」は全列調査の省略に起因**していたことが上表で説明される（重複 5 件のうち idx4/28・idx5/29 は argdesc も相違するが、この対は argdesc も同一）。

### 3.2 本体の動作（単一コードパス）

```
FAE80(9 引数: …, &outA(S0+0)=Amount カウンタ, &outB(S0+0xC)=ActorValue)
 ├ 失敗 → 戻り 0（0x50D3C3 経由）
 └ 成功:
    0x565C50(2, decoded) → 0x446AF0(g, x) → esi = 対象オブジェクト（無ければ戻り 1）
    0x5F23B0(g, av) → 現在値を取得（比較用に保存）
    ループ [S0+0] 回（= Amount、0 ならスキップ）:
       0x65FA90/0x65FAD0 で上限/基準取得 → 0x65FA40 でスキル値を増分更新
       0x5F1910 + 0x65D1E0 / [g+av*4+0x158] += 1（使用・成長カウンタ）
       0x5E3320→0x51C090 判定 true なら [g+0x184] += 1（レベルアップ蓄積）
    0x589B70(0x3EC) メニュー判定 → [g+0x184] が old から変化し ≥10 かつ 0x5A56C0 否なら
       0x98208B で文字列組み立て(0xA4CCFC) → 0x57ACC0(メッセージログ) → 0x6ADE50(0x121) 通知
    スキル値が変化したら 0x52EAB0 → 0x579C10（ダイアログ/HUD 更新系）
    av ∈ {0x0E, 0x10, 0x11} のみ追加処理（0x65D750/0x471990/0x476410）
    0x57A7D0 → 戻り 1
```

- これは **AdvSkill としての「Amount 回、1 ずつ増分し、その都度成長カウンタとレベルアップ閾値を評価する」実装**であり、増分ループの各回がレベル計上を挟む点が「Advance」的な挙動。ただし **ModPCSkill はこの同じ本体を走る**（差分コード無し）。
- ⇒ **判定: 共有 impl の意味論 = 「両コマンドは実行時完全同値のエイリアス」。Advance 固有／Mod 固有の差分コードは本体・全ディスパッチ列のいずれにも存在しない（構造的不能）。v94 §e R-d の「差分所在」は「所在しない」で決着。**

---

## §4 突合

- **b20 (V116) との突合**: ①executor 引数（arg8=[esp+0x77c] / arg9=[esp+0x780]）、②arg8==0 本命パスの call site「0x517295 ブロック」（本調査で `call ecx`@0x5172C6 を含むことを確認）、③opcode 0x10=Begin・0x1000/0x1059 の tail 特別扱い（0x5171B7 帯を逆汇で確認）、④FAE80 が tail を消費しない — **4/4 一致**。b20 §6-9（arg3/arg6 の意味論）は本タスクでも未解のまま（§5 N-b に接続）。
- **v132_exception_dispatcher.md**: ファイル不在につき突合なし。
- **v94 の数値再現**: +0x1C アンカー 0x52DE21/0x552B7A、+0x20 アンカー 0x56ACA8/0x56AE85、+0x10 アンカー 0x516AA1、+0x14 アンカー 0x516A48、+0x18 アンカー 0x516A5F を訂正後スキャンで全再現（0x5E8725 のみ偽陽性と判定）。

## §5 新規残点（V143+ 起票粒度）

| ID | 内容 | 試行経路/証拠 |
|---|---|---|
| **N-a** | 呼出し元 `FUN_00517330` の `cmp fetched_opcode,0x1e / je 0x51757b`（0x517519-0x517520）の 0x1E センチネルの確定。band1 idx30=OnReset の opid と同値だが、Begin は [0x10][i16 idx] 形なので文ストリームに 0x1E は現れず「終端マーカー」として使える（推定）。呼出し元 2（0x517640）には同チェックが無い。 | `v131_callers.txt` L142-144 / L342-368 |
| **N-b** | Begin ペイロードの後続 u32（`0x516A37 mov eax,[edi+edx]` → `[esp+0x2c]` → `0x516B1F add [eax],ecx` で加算先へ加算）の意味論。b20 §6-9 の arg3/arg6 未特定と同一箇所。ペイロード書式 [i16 event][u32 x] の x が何か（line? 初期値?）未確定。 | `v131_disp5169a0.txt` L37-48, L111-114 |
| **N-c** | entry+0x24 のバイト意味論: `+0x25==1` の 10 件（Activate / PositionWorld / PositionCell / MoveToMarker / ForceFlee / ForceTakeCover / CloseCurrentOblivionGate / ForceCloseOblivionGate / CreateFullActorCopy / DeleteFullActorCopy）が executor で `this+0xA1=1` を設定する目的と、`+0x24==1` の 9 件（GetIsClass/GetIsRace/GetIsSex/GetInFaction/GetIsID/GetFactionRank/IsGuard/GetIsReference/GetIsPlayableRace = condition 系 getter）の消費者（v94 の +0x24 アンカー 0x60D90F/0x7BF16A）の未突合。 | `v131_6_names.py` 出力（f24≠0 全 19 件 = 10+9）、`0x517295` ブロック |
| **N-d** | flagA（+0x10 下位バイト）= "requires reference" の正式確定。executor のエラー文 0xA4DB58 "Function '%s' requires a reference." / 0xA4DCC0 "Null for a required ref pointer." と、judge 材料の ebx=impl arg3（=呼出し元 `[E+0x24]` = フェッチ arg5 出力）の中身（ref 解決結果？）の同定。 | `0x5170C9-0x5170EF`, `0x516A9F-0x516ACF` |
| **N-e** | 0x50D020 のループカウンタ `[S0+0]` が本当に decoded Amount か（bytecode 実測なし・ESM 再スキャン禁止のため列論拠のみ）と、負値時 `sub 1 / jne` の非终止懸念、FAE80 の 8 引数形(0x500890)と 9 引数形(0x50D020)の出力スロット差（追加スロット=第 2 出力）。 | `v131_fae80_head.txt`, `v131_impl_50d020.txt` L16-35, L51-115 |
| **N-f** | band1 経路の impl 戻り値 false → `0x516B26` 経由で "Error Executing line (Offset %d)"(0xA4DC88) を出力し呼び出し元は byte[esp+0x16]=1 のみ設定（ループ継続）。イベント非活性時にエラー文が出る挙動の実運用解釈（出力抑制のある別経路か、band1 impl が常に 1 を返す設計か）。 | `v131_disp5169a0.txt` L100-128 |
| **N-g** | `0x4FAD43`（コンパイラ側、型文字 'X' 後の u16→FCA30→argdesc 参照）の型システム上の位置づけと、型名テーブル（0xB0A54D の type*8 表）の全 type 値対応。 | `v131_4fad43.txt` |
| **N-h** | MEM_DEBUG ビルドにおける 0x500890 の実本体（本リテールには不在。"OutputMemContexts.xls" 出力処理は別バイナリのみ）。同一バイナリからは復元不能 = スコープ外。 | §2 |
| **N-i** | v94 のアンカー走査の他列にもバイト重なり偽陽性が混入している可能性（本調査で +0x18 列 1/2 件が偽陽性を確認。+0x00 列の 18 件等、要再検）。 | `v131_anchors_out.txt`（`e8..`/`ff..` 直後の imm32 ヒット多数） |

## §6 使用証拠ファイル一覧

- 読み取り: `tmp/re/v94_boundary_dups.md`(31,509 B) / `tmp/re/b20_impl_5071d0.md`(41,353 B) / `%TEMP%\opencode\probe_harness.py`（selftest 4/4）/ `%TEMP%\opencode\func_list_new.txt` / `docs/Oblivion_decompiled.c`（L281899 `FUN_00579b60` 定義、L194575 `FUN_004fae80` 定義、L194523 FCA30 呼出しのみ部分走査）
- 本タスク生成: §0「生成物」の 23 件（%TEMP%\opencode のみ）

---

## §7 追記（2026-10-26 再試行セッション・独立再検証）

- 経緯: 遭遇したセッションが成果物完成後に終了（API レート制限の再試行タスク）。**本セッションは上書きせず追記**し、既存記載の全主要主張を独立再検証した。
- 追加生成物（%TEMP%\opencode のみ、repo 内は本レポート 1 件のまま）: `v131_10_verify.py`（独立検証スクリプト）。

### 7.1 独立再検証 37 項目（`v131_10_verify.py` → **37/37 PASS**）

| 群 | 検証内容と結果 |
|---|---|
| テーブル（9件） | `cache/opcode_table.json` 533行 OK / band delta run（idx0..30=idx、idx31..162=idx+0xE1、idx163..531=idx+0xF5D）**不一致0件** / フェンス行 idx162=0x183・idx532=0x1171 / **class3 3件＋§b 7件の opid→idx 対応10件すべて一致**（0x0152→113, 0x011A→57, 0x0124→67, 0x015A→121, 0x1005→168, 0x1023→198, 0x1041→228, 0x10BC→351, 0x112A→461, 0x112B→462）/ impl=0x500890 は idx{116,136} のみ・impl=0x50D020 は idx{375,434} のみ |
| idx375/434（2件） | argn/argdesc/impl/dispatcher/cond/argflag **6列全同一**、相違は name/alias/opid のみ（AdvancePCSkill/AdvSkill/0x10D4 vs ModPCSkill/ModPCS/0x110F）→ §3.1 の表を全再現 |
| FCA30（4件） | 本体 0x4FCA30..0x4FCA70（0x40 B）、band2 定数 0xB0B420・band3 定数 0xB0C8C0・範囲 `cmp 0x82/0x170` を逆汇で確認 |
| 呼出し数（4件） | `call 0x4FCA30` = **3** / `call 0x5169A0`(executor) = **2** / `call 0x579B60` = 516 / GT `call 0x446CB0` = 347 |
| executor 経路（5件） | 0x516A27 `cmp ecx,0x1e` / 0x51703E `call 0x4fca30` / 0x516B63 `add ecx,-0x10` / **0x517299 `mov ecx,dword[eax+0x18]`** / **0x5172C6 `call ecx` → 0x5172C8 `add esp,0x20`**（9 push=8引数+align） を直接逆汇で確認 |
| アンカー（3件） | imm32 `base+0x18` は .text 全域で **[0x516A5F] の1件のみ**（命令先頭 0x516A5D `mov ecx,[eax+0xb0af60]`、imm32 は+2=0x516A5F — スコープ表記 0x00516A5D と §4 表記 0x516A5F は同一命令の先頭/即値位置で矛盾なし）/ band2・band3 の opid-scaled アンカーは **0件** / 0x5E8724=`ff d0`(call eax)、0x5E8725 起点の imm32 読取 = `d0 e9 b0 00` → **§1.3-4 の偽陽性判定をバイト列で再確認** |
| 0x500890（7件） | 定数 0xA4B818="OutputMemContexts.xls" / 0xA4B7E8="This function only works in MEM_DEBUG" / 本文にテーブル base(0xB0AF48系) 参照 **0件** / call = {0x4FAE80, 0x579B60, 0x9840E0, 0x9811E2} / `ret`@0x5009D2 |
| 0x50D020（4件） | 本文 0x50D020..0x50D3E0（≥0x3A0 B）/ opcode テーブル lea **0件** / `call 0x4FAE80`(31 call 中) / 0x5F23B0・0x589B70 呼出し確認 → §3.2 の単一コードパス |
| 成果物（1件） | 本レポート UTF-8 デコード可・17,819 chars（追記前 26,466 B / 249 行） |

- 初回実行時 1 件の FAIL は検証スクリプト側の線形逆汇境界（0x5169A0 起点の線形走査が 0x517299 以前に命令境界を逸脱）が原因で、0x517290 起点の直接逆汇で **命令実在を確認済み（検証スクリプト側を修正）**。バイナリ・レポート側の誤りではない。

### 7.2 残留不明点（契約 §6 の節、本項が該当）

§5「新規残点」N-a〜N-i の **9 件が残留不明点の全容**（本再試行でも未解決・変更なし）。特に N-i（v94 アンカー走査の他列にバイト重なり偽陽性が混入している可能性）は本セッションで +0x18 列 1/2 件が偽陽性と確定した直接後続であり、+0x00/+0x1C/+0x20 列の再検が優先度高。N-e（0x50D020 のループカウンタ実測）は ESM 再スキャン禁止のため列論拠に留まる。

### 7.3 Android 移植への示唆

移植では opcode テーブルの `+0x18` を「単一の絶対ハンドラ配列」として実装しないこと。band1（イベント idx、opid≤0x1E）のみテーブル idx 直接参照、band2/band3 は「opid→エントリ先頭解決（FCA30 相当: 0x100..0x182→base+31*0x28、0x1000..0x1170→base+163*0x28）→ 相対 impl 参照」という **2 経路の非対称**を再現する必要がある。AdvancePCSkill/ModPCSkill は同一関数へのエイリアス2登録、OutputMemContexts/ResetMemContexts は MEM_DEBUG 未実装スタブの二重登録として移植すれば挙動差は生じない（rmc の argdesc=NULL は FAE80 早期リターンを誘発するが合流後の観測は同一）。本体は実装しない（分析専任）。

### 7.4 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）— 再試行セッション冒頭と本項実行時の 2 回で一致。
- 独立再検証: `v131_10_verify.py` → **37/37 PASS**（7.1）。内部整合: opid→idx 10/10、ディスパッチ列 6/6、アンカー 3/3、呼出し数 4/4（うち GT 一致 2 件 = 0x446CB0=347 / FCA30=3）。
- 出典整合: v94 §e R-a/R-c/R-d の 3 残点すべてに「解明」判定で対応（§1/§2/§3）。b20 (V116) との突合 4/4 一致（§4）。
- 編集範囲: repo 内の書き込みは本レポートの追記のみ（docs/・TODO・台帳・git・Supermemory・sync 未実行）。中間物は `%TEMP%\opencode\v131_*`（24 件）。
