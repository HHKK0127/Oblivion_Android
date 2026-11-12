# V132: オペコード表 +0x1C 例外 dispatcher 4 件の切り分け

- 日付: 2026-10-27（opencode サブエージェント・分析専任／docs 編集なし）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416, ImageBase 0x400000）
- 出典: `tmp/re/v97_forwarder_family_map.md` §6-4 残点4（V77 以来保留）+ V77/V78 GT + `tmp/re/b20_impl_5071d0.md`（V116）+ `tmp/re/v131_impl18_arrival.md`（V131）
- 前提確認: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時とも実行）
- スコープ: 「`+0x1C` 列が 0x4FDE30 でない例外 4 件」= **0x500130×2（opid 0x1000 MessageBox / 0x114D EssentialDeathReload）・0x500150×1（0x1059 Message）・0×1（0x183 センチネル）** の非共有理由と実行時挙動差。b20 が調査済みの thunk 生バイト（push 1 @0x50013C / push 0 @0x50015C）は引用のみ（重複排除）。

## 結論（5 行）

| 例外 | 判定 | 非共有理由（なぜ 0x4FDE30 でないのか） | 実行時挙動差 |
|---|---|---|---|
| **0x1000 MessageBox**（+0x1C=0x500130, flag=1） | **解明** | 「本文文字列 + %変数≤10 + 末尾ボタン文字列≤10」というメッセージ専用構文を emit する必要。汎用 argdesc 型駆動ループ（0x4FDE30）では表現不能 → 専用コンパイラ `FUN_004FEF80` を flag=1 で束縛する thunk | 一般経路（arg8=0）で impl `0x513280` が **flag=1 形式の全体を消費**（本文→%変数式評価→ボタン文字列→`DAT_00b09e24=0xFF`→`FUN_00579c10` で表示） |
| **0x114D EssentialDeathReload**（+0x1C=0x500130, flag=1） | **解明** | **MessageBox と同じ構文**（flag=1 形式）だったため同一 thunk を共有。argdesc 0xB0A11C（String×1）単体では差別化不能なことの裏返し | impl `0x5071D0` は FAE80 で String 引数だけ消費し **tail（%変数+文字列）は一切読まないデッドペイロード**（外側の `PC += argLength` で安全にスキップ）。dispatch 側の挙動差は 0 件 |
| **0x1059 Message**（+0x1C=0x500150, flag=0） | **解明** | 同じメッセージコンパイラだが tail 形式が **u32 time**（整数チェック付き）→ flag=0 の別 thunk が必要 | impl `0x513600` が本文→%変数→**u32 time（0 なら既定 10 秒）**を消費し `FUN_0057acc0(msg,0,1,(float)time)` でメッセージログ表示 |
| **0x183 センチネル**（+0x1C=0） | **解明** | コマンドではなく **テーブル終端マーカー**（"ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!!"、impl=0/argdesc=0/argn=0）。dispatcher 呼出しが構造的に起きない二重の門 | 到達不能: コンパイラ FF710 のバンド2ゲート `< 0x83`（0x100..0x182）で弾かれて "Unknown function code %d."。手製バイト列でも FCA30 が範囲外→0→"Unable to find function definition…" |

**総括**: `+0x1C` は **コンパイル時専用の引数コンパイラ列**（V131 確定: 実行時は `[entry+0x18]` のみ参照）。例外 4 件の「非共有」はすべて **コンパイル時に emit するバイト列の形が違う**ことに归着し、実行時への影響は「その形を読む/読まない impl」だけに現れる。flag 値はバイト列に保存されず、opcode ごとの thunk 定数として暗黙に区別される（b20 と一致）。

---

## §0 入力・手法・生成物

### 入力（読むだけ・編集なし）

| パス | 用途 |
|---|---|
| `tmp/re/v97_forwarder_family_map.md` | §6-4 残点4 の定義、§4.1 +0x1C 消費者表（FF710 の 4 引数契約） |
| `tmp/re/v131_impl18_arrival.md` | 実行時 executor の経路（FCA30→`[eax+0x18]`→`call ecx`）、+0x1C 未読取の確定、偽陽性クラス（N-i） |
| `tmp/re/b20_impl_5071d0.md`（V116） | thunk 生バイト（push 1/0）、0x4FEF80 の flag 分岐 decompiled 引用、Essential tail=デッド、executor の 0x1000/0x1059 特別扱い=arg8≠0 経路、caller の arg8=0 |
| `tmp/re/cache/opcode_table.json` | 533 行のキャッシュ（raw 再読と 533/533 一致を確認） |
| `%TEMP%\opencode\probe_harness.py` | `Ctx.rd()/cstr()/text_imm32()`（selftest 4/4） |
| `%TEMP%\opencode\func_list_new.txt` | 関数境界（偽陽性アンカーの所有関数特定） |
| `docs/Oblivion_decompiled.c` | 部分検索のみ: `FUN_004fef80` 定義 L197796-197987、`FUN_004ff710` の +0x1C 呼出し L198415-198447・L198435-19838、impl 定義 L205739（0x513280）/L205871（0x513600） |
| （参照のみ）旧 v132 試行の残骸 `%TEMP%\opencode\v132_1_rows.py` 等 | 別セッションの未完試行。`v132_7_out.txt` の 0x52DE21 再デコードが本調査と同結論（矛盾なし） |

### 手法

- capstone 5.0.7（md.detail なし / probe_harness の Ctx.rd でセクション対応読み取り）。opcode テーブル 533 行を 10 dword で raw 再読し cache と全項目照合（mismatch 0/533）。
- `.text` 全域の E8 直接呼出しマップを構築して各対象の呼出し数を全数集計。imm32 リテラル走査（.text）。
- decompiled は python enumerate による部分走査（全文読み込みなし）。

### 生成物（すべて %TEMP%\opencode、新規 14 件）

スクリプト: `v132_1_table.py`(4,029B) / `v132_2_thunks.py`(1,573) / `v132_3_ff710.py`(1,211) / `v132_4_runtime.py`(1,202) / `v132_5_lookup.py`(1,823) / `v132_6_xrefs.py`(1,717) / `v132_7_anchors.py`(1,158)
出力: `v132_table.txt`(5,930) / `v132_thunks.txt`(14,154) / `v132_ff710.txt`(6,497) / `v132_runtime.txt`(9,563) / `v132_lookup.txt`(6,087) / `v132_xrefs.txt`(1,413) / `v132_anchors.txt`(23,809)
**repo 内の新規ファイルは本レポート 1 件のみ**（docs/・既存 tmp/re・TODO・memory・plan は未編集、git 未実行、ESM 再スキャンなし）。

---

## §1 表監査 — 例外 4 件の確定（解明）

### 1.1 census（idx 0..531 = V77 の 532 件カウント）

| +0x1C 値 | 件数 |
|---|---|
| 0x4FDE30（共有 dispatcher） | 528 |
| 0x500130（flag=1 thunk） | 2（0x1000 / 0x114D） |
| 0x500150（flag=0 thunk） | 1（0x1059） |
| 0x0 | 1（0x183 センチネル） |

→ V77/V78 GT（528/529+例外）と一致。cache 533 行全項目 raw 再読一致（mismatch 0/533）。

**【訂正・拡張】** 表の実体は **533 行**であり、idx532 = **opid 0x1171 "ADD NEW FUNCTIONS BEFORE THIS ONE!!!"**（impl=0 / +0x1C=0）という**第 2 センチネル**が存在する。V77 の「532 件」は band3 上限 0x1170（idx531）で打ち切ったカウントのため 0x1171 は数えておらず、タスク定義の「0×1」も 0x183 のみを指す。到達不能メカニズムは 0x183 と完全同一（§4.3）。

### 1.2 例外行と近傍（v132_table.txt）

```
idx=0xA2  opid=0x183  'ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!!'  impl=0x0      +0x1C=0x0
idx=0xA3  opid=0x1000 'MessageBox'           argn=1 argdesc=0xB0A11C impl=0x513280 +0x1C=0x500130
idx=0xFC  opid=0x1059 'Message'              argn=1 argdesc=0xB0A11C impl=0x513600 +0x1C=0x500150
idx=0x1F0 opid=0x114D 'EssentialDeathReload' argn=1 argdesc=0xB0A11C impl=0x5071D0 +0x1C=0x500130
idx=0x214 opid=0x1171 'ADD NEW FUNCTIONS BEFORE THIS ONE!!!'         impl=0x0      +0x1C=0x0
```

近傍（0x1001 GetDistance / 0x1058 AddTopic / 0x105A SetAlert / 0x114C IsInDangerousWater / 0x114E SetShowQuestItems ほか）はすべて +0x1C=0x4FDE30。**例外は opid 群ではなく 3 コマンド+1 マーカーの点在**である。

### 1.3 argdesc だけでは例外化しない（切り分けの要点）

3 コマンドは **argdesc 0xB0A11C（"String", type=0, flags=0, argn=1）を共有**するが、この argdesc を持つオペコードは表全体で **13 件**あり、そのうち **10 件（SetLightingPasses/CenterOnCell/GetINISetting/BetaComment/AddFaceAnimNote/SaveWorld/CloseFile/StreamMusic ほか）は標準 0x4FDE30** を使う。
→ **差別化要因は argdesc ではなく「tail（%変数列+time/文字列）の存在」**。0x4FDE30 も 0x4FEF80 も先頭部 `[u16 argc][String(len 前提付き)]` は同一書式（FAE80 互換）なので、tail を持たない String コマンドには標準 dispatcher で十分である。

---

## §2 +0x1C の呼び出し契約と 3 本の本文（解明）

### 2.1 唯一の消費者 = `FUN_004FF710`（ScriptCompiler）、4 引数cdecl

`v132_ff710.txt`（0x4FF950-0x4FF9BB）:

```asm
0x4FF950  mov  edx,[eax+0x14]        ; entry.argdesc
0x4FF953  test edx,edx
0x4FF955  mov  [esp+0x18],edx
0x4FF959  je   0x4ff9ff              ; ★ argdesc==NULL → dispatcher 呼出しなし（未使用命令として処理）
0x4FF95F  movzx edx,word [eax+0x12]  ; entry.argn（+0x12 の u16）
0x4FF963  mov  ebx,[eax+0x1c]        ; ★ +0x1C dispatcher
...
0x4FF998  push edi                   ; arg4 = コンパイラ状態（レキサ/エラー報告先）
0x4FF9A0  push edx                   ; arg3 = ステージング ctx（esp+0x944、buf=+0x20C / len=+0x40C）
0x4FF9AC  push eax                   ; arg2 = argdesc
0x4FF9AD  push ecx                   ; arg1 = argn
0x4FF9B9  call  ebx                  ; ★ cdecl 4 引数、戻り al!=0 で成功
0x4FF9BB  add  esp,0x10
0x4FF9BE  test al,al / je 0x4ff8c1   ; 失敗 → コンパイル中断
```

decompiled L198435-198438 も同型: `if ((ppuVar7[5]==NULL) || (cVar3 = ((code*)ppuVar7[7])(*(u16*)((int)ppuVar7+0x12), ppuVar7[5], ctx, state), cVar3==0)) error`。
成功時はステージング ctx の内容がメイン出力（esi+0x20C）へ memcpy され `[esi+0x40c] += len`（0x4FF9E4-0x4FF9F9）。

**含意**: argdesc==NULL の行（表に 252 行。GameMode 等の引数なし命令 + 両センチネル + UnusedFunction1/2 等）は **+0x1C が何であれ一切呼び出されない**。よって 0x183 の +0x1C=0 は「存在しても無害」な設計（§4）。

### 2.2 共有 dispatcher 0x4FDE30 = argn/argdesc 駆動の汎用引数コンパイラ

`v132_thunks.txt`（抜粋、本体 ≥0x3FE B）:

```asm
0x4FDE60  mov  eax,[esi+0x40c]            ; ctx.len（esi = arg3 = ステージング ctx）
0x4FDE73  mov  ax,word [esp+0x240]        ; arg1 = argn
0x4FDE81  mov  word [ecx],ax              ; ★ 先頭に u16 argn を書出し（実行時 FAE80 の argc 読取と対）
0x4FDE84  add  dword [esi+0x40c],2
0x4FDE9D  jle  0x4fec56                   ; argn≤0 → これで終わり
0x4FDEA5..0x4FDEB0  argdesc[i]（12B/個）の type を取り
0x4FDEB0  mov  bl,byte [eax*8+0xb0a54c]   ; type→クラステーブル
0x4FDF10  call 0x4fd7c0                   ; レキサ: トークン読取（9 引数）
0x4FDF94  add  eax,-3 / cmp eax,0x22 / ja 0x4fed7e
0x4FDFA0  jmp  dword [eax*4+0x4feeac]     ; ★ type-3 で 0x23 分岐のジャンプテーブル
（各 type ハンドラ: 数値/文字列/フォーム ref 等を 0x4FD0A0, 0x9832E6, 0x4D6D40 …で
  ctx+0x20C へ型付きエンコード。失敗時は 0x4FCE30 へフォーマット文字列
  0xA4AF28/0xA4AF70/0xA4AED8/0xA4B000/0xA4B050 等を渡して al=0 ret）
```

- **E8 直接呼出し = 0 件**（v132_xrefs.txt）。表経由の間接呼び出し（call ebx）のみで到達する。
- .text 内 imm32 リテラル参照も 0 件（表本体は .rdata、.text からは FF710 の `lea eax,[ebx+0xb0af48]` 0x4FF906 経由のみ — v97 と一致）。

### 2.3 thunk 0x500130 / 0x500150 — 0x4FEF80 への flag 束縛ラッパ（b20 の再確認）

```asm
0x500130  mov eax,[esp+0x10] / mov ecx,[esp+0xc] / mov edx,[esp+8]   ; 引数退避
0x50013C  push 1                       ; ★ flag = 1（param_5）
0x50013E  push eax                     ; ctx（元 arg4）
0x500143  push ecx / push edx
0x500145  push eax                     ; argn（元 arg1）
0x500146  call 0x4fef80                ; FUN_004FEF80(argn, argdesc, ctx, state, flag)
0x50014B  add  esp,0x14 / ret          ; 戻り値 al をそのまま返す
0x500150  …同型… 0x50015C push 0 … call 0x4fef80
```

- 2 本は **push 定数（6a 01 / 6a 00）以外バイト同一**の 0x20 刻みラッパ。flag は opcode ごとのコンパイル時定数。
- **`FUN_004FEF80` の E8 直接呼出しはこの 2 本のみ**（0x500146 / 0x500166、全数集計）= 0x4FEF80 はメッセージ系 3 コマンド専用コンパイラであることが機械的に裏付けられる。

### 2.4 `FUN_004FEF80`（メッセージコンパイラ）の emit 書式 — decompiled L197832-197987

```
u16 argc := 1                       （固定。param_1=argn / param_2=argdesc は本文中で不参照）
[u16 len][len B] 本文文字列          （0x4FD7C0 失敗 → "No message text."）
% 走査: 単独 %（%% は除外）の数を counting、>10 → "Max variables of %d exceeded."
u16 varcount
varcount 回: 0x4FD7C0(...,1,0) で変数名を解決し、
   local_16f4!=0 なら [tag 1B ('G'=0x47 / それ以外=0x72)][u16 id]
   非 'G' なら [type-char 1B][u16 val]（解決不能 → "Unknown variable '%s' in MessageBox parameters."）
--- flag 分岐（param_5, L197919）---
flag=0（Message）: トークン 1 個を読み、非整数なら "Message time must be an integer."→失敗。
                   atol で u32 化して書出し（トークン無し → 0 を書出し）
flag=1（MessageBox/EssentialDeathReload）: トークンを最大 10 個読み、
                   u16 count（無条件書出し、0 でも書かれる）→
                   各文字列 [u16 =1][u16 len][len B]（= argc=1 の String 式表現）
```

`param_3` = ctx（buf +0x20C / len +0x40C）、`param_4` = レキサ/エラー状態。**flag 自体はバイト列に一切書かれない**（b20 の裁定と一致）。

---

## §3 例外 4 件の実行時挙動差（解明）

### 3.1 実行時の共通前提（V131 から引き継ぎ・再確認）

executor `FUN_005169A0` の一般経路（arg8=0 = **リテール到達可能の唯一経路**）:

```
0x517036  push opcode → call 0x4FCA30         ; opcode → entry 先頭（band2 ≤0x182 / band3 ≤0x1170）
0x5170C9  cmp byte [eax+0x10],0               ; flagA ゲート
0x517299  mov ecx,[eax+0x18]                  ; ★ +0x18 impl（相対 disp。+0x1C は不読）
0x5172C6  call ecx                            ; 8 引数、opcode は渡らない
```

よって **+0x1C の例外であることは実行時に直接観測されない**。影響は「コンパイラが emit したバイト列の形」と「その形を解釈する impl」のみ。

### 3.2 実行時 tail 特別処理（executor 内）は「0x1000/0x1059 のみ・かつ到達不能」

`v132_runtime.txt`（arg8≠0 側の分岐、0x51717F-0x517293）:

```asm
0x517171  cmp byte [esp+0x77c],0 / je 0x517295 ; arg8==0 → 一般経路（call ecx）へ
（arg8!=0 のみ以下が実行される）
0x5171A8  call 0x4fae80                        ; 本文 String を FAE80 でデコード
0x5171AD  mov eax,[esp+0x780]                  ; arg9
0x5171B7  sub eax,0x1000 / je 0x517210         ; ★ 0x1000 MessageBox
0x5171BE  sub eax,0x59   / jne 0x5172d3        ; ★ 0x1059 Message（他は即 return）
0x517210  u16 count 読取 → loop: call 0x4fab20  ; %変数式を逐次評価（0x1000）
0x517255  u16 count 読取 → loop: call 0x4fae80  ; ボタン文字列（[u16=1]=argc1 互換）
0x5171C7  u16 count 読取 → loop: call 0x4fab20  ; %変数式評価（0x1059）
```

- **E8 直接呼出し 2 件のみ**（全数）: `0x51754D`（arg9=0, arg8=0）と `0x5177C6`（arg9=1, arg8=0 — push 順実測）。**どちらも arg8=0 → この tail 処理はリテール v1.2.0416 で到達不能**（b20 §3.4 の裁定を独立に再確認）。
- **0x114D はopcode 分岐に存在しない** → Essential は常に一般経路。b20 の「flag=1 分類はコンパイラ側仕様であり実行時の特別扱いと無関係」に実行時側からの裏付け。

### 3.3 impl 3 本の消費差（decompiled 実測）

| | 0x513280 MessageBox | 0x513600 Message | 0x5071D0 EssentialDeathReload |
|---|---|---|---|
| FAE80 で本文デコード | ✓ L205799 | ✓ L205917 | ✓（b20: decompiled 欠落、生バイナリで確認済） |
| u16 varcount + `0x4FAB20` で %変数式評価 ×N | ✓ L205801-205814 | ✓ L205919-205932 | **なし** |
| `FUN_00402e30` で %vars 展開 | ✓ L205820 | ✓ L205938 | なし |
| tail 処理 | **u16 ボタン数 + FAE80 で各文字列**（[u16=1] 形式と一致）L205822-205838 → `DAT_00b09e24=0xFF` → `FUN_00579c10`（ボックス表示）L205856-205858 | **u32 time 読取**（+4、0 なら既定 10）L205940-205944 → `FUN_0057acc0(msg,0,1,(float)time)` L205957 | **なし（tail はデッド**、`PC += argLength` で外側がスキップ: 0x51755E / 0x5177D3） |
| 戻り値 | void（成功時のみ処理） | void | 戻り 1 固定（b20） |

- 0x1000 と 0x1059 の差 = **コンパイル時 flag（tail 形式）+ impl の tail 解釈**。0x1000 と 0x114D の差 = **コンパイル時は同一（同 thunk・同書式）、実行時は impl が tail を読むか否か**。
- ボタン結果チャネル `DAT_00b09e24`（= b20 の 0xB09E24）を **MessageBox impl が使う**（0xFF にリセット→`FUN_00579c10` へ渡す）こと、Essential 侧では不使用 — b20 と整合。

### 3.4 v94 の「+0x1C アンカー」2 件はバイト重なり偽陽性（V131 N-i の +0x1C 列解消）

真の関数エントリから再デコード（`v132_anchors.txt`。旧 v132 試行の `v132_7_out.txt` でも独立に同結論）:

| v94 アンカー | 所有関数 | 真の命令 | 判定 |
|---|---|---|---|
| 0x52DE21 | FUN_0052d800 | `0x52DE20 lea ecx,[ebx+eax*8+0xb0]` の**命令本体内バイト** | 偽陽性（8-bit disp 0xB0 周辺の偶然一致） |
| 0x552B7A | FUN_00552990 | `0x552B77 fld dword [esp+0x1c]` の**スタック disp バイト 0x1C** | 偽陽性（テーブル無関係の x87 命令） |

→ **`+0x1C` 列の読取者は FUN_004FF710 の一箇所（0x4FF963/0x4FF9B9）のみ**。これにより「+0x1C はコンパイラ専用列」という V131 の位置づけがアンカー面からも補強された。

---

## §4 センチネル 0x183（および 0x1171）の構造的到達不能性（解明）

| # | ゲート | 場所 | 効果 |
|---|---|---|---|
| ① | コンパイラ band2: `iVar6 - 0x100U < 0x83` → 0x100..0x182 | decompiled L198415（FF710 default 分岐） | 0x183 は範囲外 → `FUN_004fce30("Unknown function code %d.")` L198446 |
| ② | コンパイラ band3: `iVar6 - 0x1000U < 0x171` → 0x1000..0x1170 | L198442 | 0x1171 は範囲外（同上） |
| ③ | 実行時 FCA30: `cmp edx,0x82/ja`・`cmp edx,0x170/ja` | 0x4FCA3C / 0x4FCA59（V131 確定） | 0x183/0x1171 → eax=0 → "Unable to find function definition for command %d in script '%s'."（0xA4DB98） |
| ④ | argdesc==NULL ゲート | FF710 L198435（0x4FF953-0x4FF959） | センチネルは argdesc=0 のため、仮に①を抜けても **+0x1C は呼び出されない**（0 呼出しの安全性の保証） |

- 別関数 `0x4FDC40` 系（0x4FDC49 `lea eax,[ecx-0x100]; cmp eax,0x82` / 0x4FDC65 `-0x1000; cmp eax,0x170`、`v132_lookup.txt`）も**同一のバンド上限**を持つ = 表を opid 索引で歩く消費者はすべて 0x182/0x1170 上限で揃う。
- FF710 冒頭の名前走査（strcmp 環、0x4FF80B-0x4FF84F）は **band1 限定**（`add eax,0x28 / cmp eax,0xb0b424 / jl` = idx0..30）でセンチネルに触れない。
- ⇒ 「+0x1C=0・impl=0 の全 0 行」は **壊れたコマンドではなく開発者向けのテーブル延長マーカー**。4 重のゲートによりコンパイル時・実行時のいずれでも無害。

---

## §5 新規残点（V143+ 起票粒度）

| ID | 内容 | 試行経路/証拠 |
|---|---|---|
| **P-a** | %変数レコードのバイト列仕様の完全トレース: コンパイル側 0x4FD7C0 出力（[tag 'G'/0x72][u16 id] + [type-char][u16] の生成条件 local_16f4/local_16f0）と、実行側 0x4FAB20 の消費形式の突合。現状は「両者が対であること」まで（0x4FAB20 側の引数契約 push 1 が CONCAT44(1,…) と一致）のみ。 | decompiled L197875-197915、L205807-205811、`v132_runtime.txt` 0x5171E6-0x517209 |
| **P-b** | executor arg9（push 順で caller1 0x51754D=0 / caller2 0x5177C6=1）の意味論。到達不能 tail 経路が `[esp+0x780]`=arg9 を 0x1000/0x1059 セレクタとして読むこととの関係（旧来の別呼び出し契約の名残か）。 | `v132_lookup.txt` 両 caller の 9 push 実測 |
| **P-c** | band2/3 の名前→opid 解決関数の帰属（FF710 冒頭走査は band1 のみ）。0x501555/0x501571/0x50158F/0x5015A7・0x4FCABA..0x4FCCE2 等の表歩行（旧試行 imm32 census）の役割分担。センチネル名が名前解決に届くか（届いても①④で無害）。 | 旧 `v132_7_out.txt` census、`v132_lookup.txt` |
| **P-d** | v94 列アンカーの残存偽陽性: バイト重なりクラスは +0x18（0x5E8725, V131）・+0x1C（0x52DE21/0x552B7A, 本タスク）で計 3 件確定。+0x20/+0x10/+0x14 列の再検は未実施。 | `v132_anchors.txt`、V131 §1.3 |
| **P-e** | （軽）0x4FEF80 の param_1/param_2 が本文中不参照であることを L197800-197987 全文で確認済。argn/argdesc は thunk 経由で受け取るが使用しない設計の明文化（移植時の assert 材料）。 | decompiled L197800-197987 |

## §6 自己検証

```
$ python "%TEMP%\opencode\probe_harness.py" selftest
   bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl 0x504560
   => ALL PASS（4/4、着手時・完了時）
```

| 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|
| オペコード表 census（idx0..531） | 0x4FDE30=528 / 0x500130=2 / 0x500150=1 / 0=1 | 同左 | OK（V77 GT 一致） |
| cache vs raw 再読 | 全項目一致 | 533/533 mismatch 0 | OK |
| 0x4FEF80 の E8 直接呼出し | 2（両 thunk のみ） | 0x500146 / 0x500166 | OK |
| 0x4FDE30 の E8 直接呼出し | 0（間接のみ） | 0 | OK |
| executor 0x5169A0 の E8 呼出し | 2 件とも arg8=0 | 0x51754D(arg8=0) / 0x5177C6(arg8=0) | OK（tail 特別扱い=到達不能） |
| thunk 本体 | push 1 @0x50013C / push 0 @0x50015C、とも call 0x4FEF80+add esp,0x14 | 同左 | OK（b20 と一致） |
| FF710 の argdesc==NULL ゲート | dispatcher 非呼出し | 0x4FF953-0x4FF959 je 0x4ff9ff | OK |
| v94 +0x1C アンカー 2 件 | テーブル参照 | 両者とも真境界再デコードで無関係 | 偽陽性と確定 |

## §7 使用証拠ファイル一覧

- 読み取り: `tmp/re/v97_forwarder_family_map.md`(51,811B) / `tmp/re/v131_impl18_arrival.md`(26,466) / `tmp/re/b20_impl_5071d0.md`(41,353) / `tmp/re/cache/opcode_table.json`(129,379) / `%TEMP%\opencode\probe_harness.py`（selftest 4/4）/ `%TEMP%\opencode\func_list_new.txt` / `docs/Oblivion_decompiled.c`（L197796-197987 `FUN_004fef80`、L198350-198470 `FUN_004ff710` band/ dispatcher 呼出し、L205739-205965 `FUN_00513280`/`FUN_00513600` の部分走査のみ）/ 旧試行残骸 `v132_7_out.txt`・`v132_4_out.txt`（矛盾確認のみ）
- 本タスク生成: §0「生成物」の 14 件（%TEMP%\opencode）+ 本レポート 1 件

---

## §8 【追記・裁定】フェンス行 idx162/idx532 と b05（V101）idx162/163 訂正の突合

- 追記日: 2026-10-27（同一タスク V132 の続き。既存本文は上書きせず本節を追加）
- 追加入力: `tmp/re/b05_compiler_handlers.md`（V101）、`tmp/re/cache/opcode_table.json`（533行 raw 一致）、`tmp/re/b06_field20_24.md` L41/L389、`tmp/re/GT_CHEATSHEET.md` §3
- 追加生成物: `%TEMP%\opencode\v132_8_fence.py`（キャッシュ独立検証）、`v132_9_startcheck.py`（`FUN_004fcce0` probe）

### 8.1 「idx162/idx532 の opid 重複」議論の裁定 = 重複ではない

独立検証（`v132_8_fence.py`、cache 533行）:

| 検査 | 実測 |
|---|---|
| opid 重複 | **0 件**（533 行すべて一意） |
| idx162 | va `0xB0C898` / opid `0x183` / name `ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!!` / argn=0 / argdesc=0 / impl=0 / +0x1C=0 / cond=0 / flagA=0 |
| idx532 | va `0xB10268` / opid `0x1171` / name `ADD NEW FUNCTIONS BEFORE THIS ONE!!!` / 同じく全 0 |
| `+0x1C==0` の行（533 全体） | **2 行**: idx162 (0x183) と idx532 (0x1171) |

- 「重複」の印象の正体は **532 行 vs 533 行のカウント法の差**: b05 L36「+0x1C==0 は idx162 の 1 行のみ」（= idx0..531 範囲）、`memory_contradictions_20261024.md` L55「EXE 実測 = 532」、`b06_field20_24.md` L389「従来の 532 行カウントはフェンス 1 行を落としていた」。両フェンス行は argn/argdesc/impl/dispatcher/cond/argflag/flagA がすべて 0 の**同型行**のため、片方を数えるカウント法では「全 0 行は idx162 だけ」「終端が 1 行」に見える。
- 裁定: **opid 重複は存在しない。idx162=0x183 と idx532=0x1171 は独立した 2 本のフェンス**（コンソール帯終端 / 関数帯終端）。タスク定義の「0×1（0x0183）」は b05/V77 と同じ **idx0..531 カウント**を指す語であり、idx532 は band3 上限（0x1170=idx531）外のため数えない（§1.1 の「訂正・拡張」と矛盾しない）。
- 機械的裏付け（`v132_9_startcheck.py` probe、GT §3 と一致）— 起動整合性チェック `FUN_004fcce0`:

```asm
0x4FCCE0  cmp dword [0xB10270],0x1171   ; idx532 opid（entry 0xB10268+8）
0x4FCCEA  push esi
0x4FCCEB  mov  esi,ecx
0x4FCCED  je   0x4FCCFC                 ; 一致 → 続行
0x4FCCEF  push 0xA4A0E0 / 0x4FCCF4 call 0x404EC0 / 0x4FCCF9 add esp,4（不一致メッセージ）
0x4FCCFC  cmp dword [0xB0C8A0],0x183    ; idx162 opid（entry 0xB0C898+8）
0x4FCD06  je   0x4FCD15
0x4FCD08  push 0xA4A098 / 0x4FCD0D call 0x404EC0 / 0x4FCD12 add esp,4
0x4FCD15  mov  eax,esi / pop esi / ret
```

  → 両フェンスの opid は**起動時に個別検証**される。加えて `FUN_004fca30`/`FUN_004ff710`/`0x4FDC40` 系のバンド判定（§4）により、どちらのフェンスも実行不能であることが二重に保証される。

### 8.2 b05（V101）idx162/163 訂正の踏襲（整合・差異なし）

| b05 の記述 | 本タスク実測 | 判定 |
|---|---|---|
| L44: V83 の idx163 raw 誤記（`50015000` と記載）→ 実バイトは **0x500130** | cache idx163 dispatcher = **0x500130**（§1.2 と一致） | 訂正を踏襲。V83 raw 文字列セルのみ誤記 |
| idx163 → impl `FUN_00513280`（0x513280） | cache idx163 impl = 0x513280 | 一致 |
| idx252(+0x1C=0x500150) / idx496(+0x1C=0x500130) | 実測 252=0x500150 / 496=0x500130 | 一致 |
| 「全 532 行で +0x1C≠0x4FDE30 はちょうど 4 行」 | idx0..531 で 4 行（163/252/496/162）、533 行なら 5 行（idx532 加算） | **範囲違いであって矛盾ではない** |
| L206: idx162 到達不能は ①名前ループ停止（0xB0C89B）②バンド外 ③argdesc ガード の 3 重 | §4 の 4 ゲート（バンド2/3・FCA30・argdesc==NULL）+ 8.1 の起動チェック | 補完関係。b05 の「L198213 null チェックは死んだコード（Ghidra 合成）」指摘も §2.1 の `test edx,edx @0x4FF953`（対象=argdesc）と矛盾しない |

### 8.3 GT §3 の impl ゼロ行リストの裁定（idx532 欠落）

- GT §3「+0x18 impl = 527、0 は idx162/188/189/216/242」の **527 は 533 行ベースで正しい**（実測 impl≠0 = 527）。ただし 0 行リストは **6 行中 5 行**（idx162 UnusedFunction1=0x1019 / UnusedFunction2=0x101a / GetScriptVariable=0x1035 / GetQuestVariable=0x104f / **idx532=0x1171 欠落**）。
- 527 + リスト5 = 532 となることから、リストは idx0..531 抜粋であり、**数値とリストのどちらも誤りではなくカウント範囲の差**（8.1 と同じ 532/533 問題）。impl=0 の 4 コマンド（188/189/216/242）は +0x1C=0x4FDE30 であるため、本タスクの例外 4 件には**入らない**（実行時未実装コマンドという別種の例外である点に注意）。

### 8.4 argdesc 0xB0A11C の独立再現（§1.3 再確認）

cache 全 533 行で argdesc==0xB0A11C は **13 件**、うち +0x1C=0x4FDE30 は **10 件**
（SetLightingPasses / CenterOnCell / GetINISetting / BetaComment / AddFaceAnimNote / SaveWorld / CloseFile / StreamMusic / GetGameSetting / SetActorFullName）、例外 3 件 = MessageBox(0x500130) / Message(0x500150) / EssentialDeathReload(0x500130)。
→ 差別化要因が argdesc でなく「tail の有無」である §1.3 の結論をキャッシュから独立に再現。

## §9 Android 移植への示唆

移植ではオペコード表を **+0x18（実行時 impl）と +0x1C（コンパイル時 arg コンパイラ）の別列**として保持し、実行時 executor は +0x1C を一切参照しない構造（V131 確定）を再現する。メッセージ系 3 コマンド（0x1000/0x1059/0x114D）だけは argn/argdesc を無視して「本文 + %変数(≤10) + tail（flag=1: ボタン文字列≤10 / flag=0: u32 time）」を読む専用コンパイラに分岐し、flag はバイト列に保存されない暗黙定数であることをコメントとアサートで明示する。フェンス行 2 本（0x183/0x1171）は行データとして複製せず、バンド判定（0x100–0x182 / 0x1000–0x1170）と argdesc null ガードの 2 段構えだけで同等の到達不能性を得られる（起動時 assert は任意の診断機能）。実装は本タスクの範囲外で行わない。

## §10 追記部の自己検証（2026-10-27）

```
$ python "%TEMP%\opencode\probe_harness.py" selftest
   => ALL PASS（4/4、追記完了時）
```

| 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|
| opid 重複（533行） | 0 | 0（unique 533） | OK（8.1 裁定の根拠） |
| `+0x1C==0` 行（533全体） | idx162 + idx532 の 2 行 | 同左 | OK（b05 の「1 行」は idx0..531 範囲） |
| b05 idx163/252/496 の +0x1C | 0x500130 / 0x500150 / 0x500130 | 同左 | OK（V83 raw 誤記訂正を踏襲） |
| impl≠0 / impl==0 行 | 527 / 6（GT リスト5+idx532） | 527 / 6 | OK（8.3 裁定） |
| argdesc 0xB0A11C | 13 件中 10 件が 0x4FDE30 | 13 / 10 | OK（§1.3 再現） |
| `FUN_004fcce0` 起動チェック | `[0xB10270],0x1171` と `[0xB0C8A0],0x183` | 0x4FCCE0 / 0x4FCCFC で同値、不一致時 `call 0x404EC0` | OK（GT §3 probe 一致） |
| idx162/532 entry VA | 0xB0C898 / 0xB10268（+8=GT の cmp 先） | 同左 | OK |
| §6 全体 | 8 項目 | 8 項目 OK + 追記 7 項目 OK | 合計 15 項目一致 |
