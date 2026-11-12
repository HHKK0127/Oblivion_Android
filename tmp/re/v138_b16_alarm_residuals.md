# V138: b16 §4 OnAlarm 残点束 6 件の裁定（local 語彙・emit key・種別 0x12・mask 突合・+0x44 裁定・22/77 対応表）

- 日付: 2026-10-26
- 出典: `tmp/re/b16_alarm_masks.md` §4 残留不明点 1〜6（V112 完了時の残留）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416, 7,898,624 B, ImageBase 0x400000）
- 成果物は本新規 1 ファイルのみ。`docs/`・既存 `tmp/re/*.md`・`TODO_re49.md`・`plan.md`・台帳・git・memory は無変更（読取のみ）。
- 中間スクリプト/出力は `%TEMP%\opencode\v138_1..v138_10.py/.txt`（新規 10 本）。

## §0 判定サマリ（6 点）

| # | 残点 | 判定 |
|---|---|---|
| (1) | local_8/local_10 の語彙と 0x4FAE80 の出力型 | **解明（機構）＋部分未解明（CS 側 enum の基底）** — arg1 = **"Crime Type"**（short 即値 type 0x1C → int32 へ符号拡張保存）、arg2 = **"Actor (optional)"**（参照 type 6 → フォームポインタ = node key）。switch 2..6 は **1-based（v=1..6 ↔ 実行時 crime type 0..5）で 6/6 完全整合**、既定 0x10000。OnAlarmVictim は v=-1→mask 1 / v=0→0x400000 / v≥1→**mask 0（死）**、式 `2^(22*(v+1))` は設計意図 `0x400000<<v` の誤り |
| (2) | emit param_1（vcall +0x170 戻り値）と監視クラスの +0x170 | **構造は解明**: vcall は **0 引数 thiscall**（push 3 個はすべて後続 `0x4FBF90` の引数、`add esp,0xc` で証明）。監視クラスは vtable slot **+0x170 を持つ**。REFR/Actor 家族 primary vtable の +0x170 = **0x4D9B40 = GetBaseForm（`mov eax,[ecx+0x1C]; ret`、0 引数）** で 7 クラス共通 → key = u32 フォーム値。**this のクラス名と `[this+0x1C]` が formID か baseForm ポインタかは未解明**（試行経路 §2-2） |
| (3) | 0x41e920 / 0x41e210(0x12) の「0x12」同名衝突 | **解明**: 0x12 = **ExtraData type id（byte、範囲 0..0x5B）**。実体は `BaseExtraList::GetExtraData`（文字列 0xA34E5C）。b16 の「グローバル種別レジストリ」表記は **誤り**（正 = per-TLS キャッシュ + list 照合）。opid 0x012 / gate 型 id 0x12(=18) / formType 0x12 とは **別名前空間**で本経路は一義 |
| (4) | 0x60F250 の 2 emit mask（0x2000000 系）と §59.3 の突合 | **解明**: 対応は **case t → (0x10000<<t, 0x400000<<t)**（t=0..5）。§59.3 の 14 ビット表には crime 系 13 値は **不在**（§59.3 は 0x1..0x1000+0x80000000 のみ）→ b11/b16 追加分と整合。**b11 Appendix の行単位 mask2 归属は 1 ケースずれ**（隣接ケースの残 push 由来）→ 集合一致・行対応は訂正要。**mask2 の 5 値（0x800000..0x8000000）は check 側に読み手が不在 = 孤児ビット** |
| (5) | 0x4d8260/0x4d82e0 の `obj+0x44` と emit container の同一性 | **裁定: 同一フィールド（TESForm 系 +0x44 = BaseExtraList）を共用、ただし格納先は別ノード**。OnActivate = **extra type 0x13 の payload バイト（node+0xC）** を test/set/clear（0x4D8260=thunk→0x41F830=test、**0x4D8270=set(0x423DF0)、0x4D82E0=clear(0x423E50)**）、emit = 同一リスト上の **extra type 0x12 の node+0x10（event list ptr）**。report §59.3 の「obj+0x44 のフラグビットを直接 test/set」は不正確 → 訂正（＋OnActivate 呼出しは set ではなく **clear**） |
| (6) | 22 check / 77 emit の 1:1 対応表 | **解明**: §3 に全表。**emit 22 ハンドラへの属性合計 = 77/77 精密一致**、§59.3 の 14 件 + b11 の 8 件 = 22 と 1:1、mask 値は即値 20 種 + 導出 2 系（OnAlarm 6 値 / OnAlarmVictim 主 0x400000） |

## §1 手法と入力

| 入力 | 用途 |
|---|---|
| `%TEMP%\opencode\probe_harness.py` selftest | **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）— 着手時・成果物直前の 2 回 |
| `v138_1_disasm.py` → `v138_1.txt` | 14 領域の逆アセンブル（OnAlarm/OnAlarmVictim/gate/emit caller/0x60F250/0x41E920/0x41E210/0x4D8260/0x4D82E0/OnActivate/0x507010）※初回は VA 変換誤り（-0xC00）で再実行 |
| `v138_2_decomp.py` → `v138_2.txt` | decompiled 本文 14 関数（gate 0x4FAE80 / 0x41E920 / 0x41E210 / 0x4D8260 / 0x4D82E0 / 0x60A640 / 0x60B120 / 0x60F250 / OnAlarm / OnAlarmVictim / 0x507010 / 0x4FBF90 / 0x4FB580） |
| `v138_3_scan.py` → `v138_3.txt` | gate/0x41E920/0x41E210/0x41F830/0x423DF0/0x423E50 の全呼出し数、`push imm .. call 0x41e210`、型表 0xB0A54D（37×8B）、jumptable 0x60FB9C/0x5043CC、opcode_table.json idx0..30 |
| `v138_4.py` → `v138_4.txt` | **argdesc 文字列群（決定的証拠）**、check impl 3 種（OnHitWith/OnAdd/OnTrigger）、extra ヘルパ 6 種の本体、emit 0x4FBF90 本体 |
| `v138_5.py` → `v138_5.txt` | 0x41E990 バイト検証、call 検出器デバッグ、0x60A640/0x60B120 の caller、gate 参照型保存経路、extra ノード生成系 |
| `v138_6.py` → `v138_6.txt` | FUN_0060d950 冒頭（this=ebx=param_2 確定）、caller 前後、0x41E0A0 リンク 100 箇所、**b11 Appendix 77 サイト→mask グループ機械化** |
| `v138_7.py` → `v138_7.txt` | emit mask 追加 3 点、extra0x12 wrapper/creator、RTTI typeinfo 名、vtable scan v1（COL 解釈誤りを発見）、0x60C8DF 所属 |
| `v138_8.py` → `v138_8.txt` | **GetBaseForm 0x4D9B40 本体**、FUN_00607B60、0x60C7E0.. 呼出し前文脈、typeinfo 名（+8 位置補正）、家族 vtable slots |
| `v138_9.py` → `v138_9.txt` | **OnActivate impl の call 先 3 点（正確ターゲット）**、FUN_005febf0 emit 群（mask 0x100×3, 0x80×1）、0x4d86d0 region、**家族 vtable 全 23 行（base/disp/slots）**、slot impl 5 本 |
| `v138_10.py` → `v138_10.txt` | FUN_0060c5b0 の 0x60d950 実引数（`uStack_270` = entry+0x20 要素）、[esp+0x7c] 代入検索、家族 primary +0x154 impl |
| capstone 5.0.7（skipdata, detail）/ `docs/Oblivion_decompiled.c` / `tmp/re/cache/opcode_table.json` / `tmp/re/b11_event_bit_setters.md` Appendix | PE 読取・逆アセンブル／本文／argn・argdesc／emit 77 サイト表 |

## §2 各点の詳細

### §2-1 (1) local_8 / local_10 の語彙と gate 0x4FAE80 の出力型 — **解明（機構）／CS enum 基底は未解明**

**(a) argdesc の決定的証拠**（`opcode_table.json` idx13/14 → argdesc `0xB0ABDC` を raw 展開、v138_4 §B）:

```
-- argdesc 0xB0ABDC used by ['OnAlarm', 'OnAlarmVictim']
   +00: 0x00A51A18 str='Crime Type'   +04: type=0x1C  +08: optional=0
   +0C: 0x00A51A48 str='Actor (optional)' +10: type=0x6  +14: optional=1
   （後半 2 エントリは 'Crime Type' / 'Criminal'(0xA51A0C) — argn=2 のため未使用）
```

- **arg1 = "Crime Type"（type id 0x1C = 28）／arg2 = "Actor (optional)"（type id 6）**。
- 型表 0xB0A54D（37×8B 実測、v138_3 §5）で **byte0=0 が即値系 / byte1=1 が参照系**。type 28 は byte0=0（即値系）、type 6 は byte0=1（参照系）と gate の 2 本の switch（即値 `case 0,1,2,5,8,0xa,0x12,0x17,0x1c,0x21` / 参照 `case 3,4,6,7,9,...`）と 20/20 一致。
- **type 0x1C の保存形**（decompile L194575-, case 0x5/0xa/0x12/0x1c 共通）: `sVar3 = (short)読取; *(int*)out = (int)sVar3` → **符号拡張 int32**（int16 レンジ）。→ **local_8 / local_10 = Crime Type の int 値**。
- **type 6 の保存形**（v138_1 "gate store path" 0x4FB160-0x4FB192）: `'r'` パスで `FUN_004fa9c0` 参照解決 → `test [edi+8]` 非零チェック → `mov ecx,[esi]; add esi,4; mov [ecx], eax`（eax = `*(解決結果+8)`）→ **u32 ドワード保存 = フォームポインタ = node key**。→ **local_4 / local_c = key**。

**(b) gate の出力機構（b16 残点の核心）**（v138_1 "gate 0x4FAE80 head"）:
- `lea esi,[esp+0x48]` = **引数スロット &arg8 のアドレス**（OnAlarm は 9 push = 7 実引数 + 出力 2 個、`add esp,0x24` で裏取り）。ループごとに `mov ebx,[esi]; add esi,4` → **宣言変数 i 番目 → caller の第 (8+i) 実引数（= &local_8 / &local_4）へ 1 変数 1 ストア**。
- **出力型は変数の宣言型ごとに素のストアが異なる**（dword(int/ポインタ) / f32 ビット / byte / char / short→int）。**OnAlarm 系は dword をそのまま int として読む**（asm `mov ecx,[esp+8]` / `mov ecx,[esp+0xc]` — FPU 変換なし）。即値系以外に expr 型（type 1/0x17）があり `FUN_009828c0 = _ftol2` で float→int する経路もあるが **Crime Type(0x1C) は short 即値のため無関係**。
- 変数が無い（count=0）なら出力は初期値のまま → local=0 → OnAlarm 既定 0x10000 / OnAlarmVictim 0x400000。

**(c) switch 2..6 の実意味（mask 対応の突合）**:

| 出力値 v (Crime Type) | OnAlarm mask | 実行時 crime type（0x60F250 の `[esi+4]` 0..5） |
|---|---|---|
| 0（変数なし/未設定） | 0x10000（既定） | type 0 相当 |
| 1 | 0x10000（既定） | type 0 |
| 2 | 0x20000 | type 1 |
| 3 | 0x40000 | type 2 |
| 4 | 0x80000 | type 3 |
| 5 | 0x100000 | type 4 |
| 6 | 0x200000 | type 5 |
| 7+ / 負 | 0x10000（既定） | type 0 |

- **1-based 読み（v = type+1）で 6/6 完全一致**。0-based 読みだと v=1..5 が全て 1 個ずつずれる（= engine 側 off-by-one）矛盾 → **本バイナリの整合的解釈は 1-based**。
- **未解明**: CS（コンパイラ）が書き込む enum 基底（1..6 or 0..5）。EXE 内に実行時の対応表が無く、ESM ブロックヘッダ値の参照は本タスクの禁止事項（再スキャン）。→ §4 起票。

**(d) OnAlarmVictim のスイッチ実意味**（v138_1 0x50B3E0 + v138_2 decompile）:
- 式: `mask = (u32)ROUND(pow(2^22, v+1))`（`add ecx,1; push ecx; fld [0xa4cae0]=4194304.0f; call 0x507010`、0x507010 = **int 指数のバイナリ冪乗**、asm で n は整数スタック引数）。

| v (=Crime Type) | n=v+1 | 結果 | 実効意味 |
|---|---|---|---|
| -1 | 0 | **1** | check(key, **0x1** = OnAdd ビット) → **OnAdd と誤突合**（到達には type=-1 の short 即値が必要） |
| 0（変数なし含む） | 1 | **0x400000** | crime type0 の victim ビット（emit2 case0）と 1:1 ✓ 唯一有効な値 |
| 1 | 2 | 2^44 → 下位32=**0** | **check が恒偽 = ブロック死** |
| 2..6 | 3..7 | 2^66.. → **0** | 同上（全滅） |

- **設計意図は `0x400000 << v`（= emit2 側 `0x400000<<t` と同型）**であり、`base^(v+1)` は `2^(22+v)` とすべきところ `2^(22*(v+1))` になった **engine bug**。→ **1-based（1..6）なら OnAlarmVictim は構造上全ブロック死、0-based（0..5）なら v=0 のみ生還**。b16 §0-2 の「1 以上は 0」「-1 → 1」の数値は asm 突合で正しい。

### §2-2 (2) emit param_1（vcall +0x170 戻り値）と監視クラスの +0x170 — **構造解明／クラス同定は未解明**

**(a) vcall は 0 引数 thiscall（スタック勘定で証明）** — 0x60A842..0x60A86C（v138_1）:

```
0x60A842  mov eax,[ebx+0x5c]      ; action obj
0x60A845  mov [eax+0x28], esi     ; obj+0x28 = target（=param_4/対象 REFR）
0x60A848  mov ecx,[ebx+0x5c]
0x60A84B  mov eax,[ecx+0x28]      ; eax = target
0x60A84E  mov edx,[ebx]           ; edx = this の vtable
0x60A850  add eax,0x44            ; container = target+0x44
0x60A853  push 0x100              ; ← fbf90 arg3 (mask)
0x60A858  push eax                ; ← fbf90 arg2 (container)
0x60A859  mov eax,[edx+0x170]     ; vtable slot +0x170
0x60A85F  mov ecx, ebx            ; this = ebx
0x60A861  call eax                ; thiscall 0 引数
0x60A863  push eax                ; fbf90 arg1 (key) = vcall 戻り値
0x60A864  call 0x4fbf90
0x60A86C  add esp, 0xc            ; = 3 引数分のみ（vcall の引数片付けは 0）
```

- MSVC thiscall は**スタック引数を callee 側で `ret 4N` クリーンする**。もし vcall が 1..2 引数を持てば `add esp,0xc` と二重 cleaned になりスタックが壊れる → **vcall のスタック引数は 0 個**。
- したがって b16 §2-5 の「push 0x100 ← **vcall の第2引数**」は **帰属誤り**（push 2 個はすべて `0x4FBF90` の arg2/arg3 を vcall より先に積んだもの）。結論「mask = 即値 0x100」自体は正しい。→ **訂正要点**。
- 0x60B371..0x60B390（FUN_0060b120）も**逐語的に同一**。

**(b) 監視クラスは +0x170 を持つ [解明]**
- `[ebx]`（vtable）の **+0x170 = 92 番目（index 0x5C）** の仮想関数が呼ばれる。
- RTTI vtable 全走査（v138_7 §D: 有効 RTTI vtable 1,715 本 / COL 解釈修正後、v138_8 §D で typeinfo 名再確認）:
  - **slot +0x170 = 0x4D9B40 を持つクラス = 7 つ**: `TESObjectREFR, Actor, Character, PlayerCharacter, Creature, ArrowProjectile, MobileObject`。
  - 本体: **`0x4D9B40: mov eax,[ecx+0x1C]; ret`（0 引数）= GetBaseForm 系**（b14 の記載と一致）。
  - 家族 primary vtable（b27 が実測した `0xA6E074 / 0xA6FC9C / 0xA710F4 / 0xA73A0C` 系 = 同一族の 4 番目ブロック）は **+0x170 = 0x4D9B40** で揃う（v138_8 §E: 同一クラスに 4 本の vtable = 多重継承。先頭 3 本は別スロット並びで +0x170 に 0x5E10D0 等 = **スロット番号は vtable 単位**である点に注意）。
  - `0x5E10D0`（先頭 vtable の +0x170）= `fldz; [out]=0;[out+4]=0;[out+8]=[ecx+0xf4]; ret 4` → 別関数（GetVelocity 系）で GetBaseForm ではない → **「+0x170 = GetBaseForm」は primary vtable に限る**。

**(c) 本呼び出しの `this` の出所（追跡結果）**
- `call 0x60a640` は **1 箇所のみ（0x60DB38）**、`call 0x60b120` も 1 箇所（0x60DB6B）。両者とも `FUN_0060d950` 内。
- FUN_0060d950 は cdecl（caller 側 `add esp,0x10/0x18`）。asm `mov ebx,[esp+0x24]`（sub esp,0x18+4push 後）→ **ebx = param_2**、`mov ecx,ebx; call 0x60a640` → **emit this = param_2**。
- param_2 の由来（v138_10 decompile L374647/374658）:
  - 呼出し元 `FUN_0060c5b0`（fastcall、ecx=this）は **0x30 stride の配列を走査**し、`uStack_270 = *(entry + 0x20 + i*0x30)` → **param_2 = 配列要素 +0x20 フィールドのオブジェクト**。
  - 0x60d950 内: `FUN_008afce0(param_2)` → `FUN_004dc270` → `FUN_009832e6(..., &PTR_PTR_00b03054, &PTR_PTR_00b03070, 0)` = **`dynamic_cast<TESObjectREFR→Actor*>`**（typeinfo 名は v138_8 §D で `0xB03054='.?AVTESObjectREFR@@'` / `0xB03070='.?AVActor@@'` を実測）。
  - param_2 への操作: `cmp byte[+0x18],2` / `*(+0x10)+param_2`（relptr）/ `*(+0x1c)>>0x10` と `FUN_00607B60()` の比較。
- **未解明**: param_2 の具体クラス（RTTI 名）と vtable。経路: dword `0x60A640` は rdata/data に**不在**（= 0x60a640 自身は vtable エントリではない）、直接 caller は 0x60d950 のみ、0x60c5b0 の配列は実行時構築 → **静的に vtable 定数へ辿れない**。→ §4 起票（V138-a）。
- **矛盾点（残）**: 家族 primary の GetBaseForm は `[this+0x1C]`（ポインタ/ID）を返すが、0x60d950 は `[param_2+0x1c]>>0x10` を **カウンタ比較**に使っている（`FUN_00607B60` = `[0xB2EB3C]+1 & 0xFFFF`、下限 0x0A、`[0xB3B7D4]` にキャッシュ）。ポインタ>>16 とは整合しにくく、**param_2 が家族外のクラスである可能性を残す**（その場合 +0x170 の impl は別物）。

**(d) check 側 key との型一致（b16 残点の問い）**
- check 側: `local_4/local_c` = **`*(参照解決結果+8)` の u32 ドワード**（§2-1(a)）。
- emit 側: vcall +0x170 の戻り値 u32（家族なら GetBaseForm = フォーム値）。
- **型的等価（両方 u32 フォーム値）は解明**。**「同一値である」直接証拠（実行時一致）は未取得** — this クラスの +0x170 impl 特定待ち（§4 起票）。
- 補助: OnHitWith の check は argn=1 で argdesc `0xB0A7E0` = **`ObjectID`（type 0x15=21, 参照系）** → key = 指定アイテムのフォーム値、emit 側（0x5FF608/61A/704/0x60A864/0x60B387 の 5 箇所）と突き合わさる設計であることは整合。

### §2-3 (3) 「0x12」の識別と同名衝突 — **解明**

- **`FUN_0041e920` 本体**（v138_1）: `push 0x12; call 0x41e210; test eax; je +0; mov eax,[eax+0x10]; ret`。すなわち **`GetExtraData(0x12)->+0x10`** の薄い wrapper。
- **`FUN_0041e210` 本体**（v138_1 §D + decompile L20067）:
  - presence bitmap 判定: `(type>>3) < 0xc` かつ `*(u8*)((type>>3)+8+this) & (1<<(type&7))` → **96 bit = type 範囲 0..0x5B**（`cmp esi,0x5c; ja`）。
  - **per-TLS キャッシュ**: `mov ecx,[0xba9de4]`（TLS index）→ `fs:[0x2C]` → thread block → `cmp ebp,[ebx+8]`（last list 照合）→ `edi = [ebx + esi*4 + 0x10]`（type 分キャッシュ）。
  - ミス時: 文字列 **`0xA34E5C = "BaseExtraList::GetExtraData"`** を出力し、`*(this+4)` からノード走査（`node+4 = type byte`、`node+8 = next`）。
- → **0x12 = ExtraData type id 名前空間（byte）**。b16 §2-4 の「グローバル種別 0x12 のエントリを引く解決ルーチン」は **部分誤り**: グローバルレジストリではなく **BaseExtraList オブジェクト（this = form+0x44）を叩く TLS キャッシュ付き探索**。名前は未登録だが **`0x41E900`(+0xC 読取) / `0x41E920`(+0x10 読取) / `0x41F0B0`(+0xC 書込・生成付) / `0x41F130`(+0x10 書込) / `0x41F670`(+0x10 ゼロ化)** と **type 0x12 専用 wrapper 群**が存在（`push 0x12 .. call 0x41e210` = 11 箇所、0x41E920 の外部 caller = 25 箇所 — うち 0x4FBF9E/0x4FBFA9 = emit、0x4F9ECB/0x4F9ED6 = VM 起動系）。
- **登録元候補**: `0x428BB8` 周辺（v138_7 §B）— `malloc(0x14)` + ctor `0x429F20` + `call 0x41e0a0`（リンク）で type 0x12 ノード生成 → `call 0x4fbdc0` → **`mov [eax+0x10], ebp`** = **event list ポインタを extra0x12 の +0x10 へ格納**。`0x4FBDC0` は ScriptEventList クラスタ内 23 呼出し（b11 §2）。
- **同名衝突の列挙（この「0x12」が紛れ込む 5 名前空間）**:

| # | 名前空間 | 値 0x12 の意味 | 根拠 |
|---|---|---|---|
| A | **ExtraData type（byte, 0..0x5B）** | **本経路の 0x12**（event list 収容ノード） | 0x41E210 の bitmap/ノード type byte 判定 + 文字列 "BaseExtraList::GetExtraData" |
| B | gate の script 変数型 id（0..36） | 0x12=18 = short 即値型 | 型表 0xB0A54D byte0=0（即値系）、gate 即値 switch `case 0x12` |
| C | opcode/opid（block-type 表 idx 基底 0xB0AF48） | 0x012 = **ScriptEffectFinish**（idx18） | opcode_table.json 実測 |
| D | ESM formType（TESForm byte、report 記載 +4） | 0x11=BirthSign の次 = 0x12 | report §59.4 等（本調査では未使用） |
| E | 0x41E210 内 TLS キャッシュの index | = type そのもの（A と同じ値、格納先だけ TLS） | `fs:[0x2C]` 経由 `[ebx+esi*4+0x10]` |

- **裁定**: 本経路（emit → 0x41E920 → 0x41E210(0x12)）の 0x12 は **A に一義**。C（opid 0x012）や B（型 id 0x12）との衝突は **文書上の混同リスクのみ**で、コードパス上は判別可能（渡し方が即値 type byte か、opcode 表 index か、型表 index か）。

### §2-4 (4) 0x60F250 の 2 emit mask と §59.3 の突合 — **解明**

**(a) jumptable の対応（asm + decompile + jumptable raw の 3 点照合、v138_1/v138_3 §6）**

jumptable 0x60FB9C: `case0=0x60F2DF, case1=0x60F32A, case2=0x60F2F8, case3=0x60F2AD, case4=0x60F2C6, case5=0x60F31E`。各 case の asm（emit1 = call、emit2 = 末尾共有タール 0x60F346 へ jmp）と decompile の 6 case が **6/6 一致**:

| `[esi+4]` = crime type t | emit1 呼出し（key = `[esi+0xc]`） | emit2 = **0x60F346**（key = `[esi+8]`） |
|---|---|---|
| 0 | **0x60F2EC** mask `0x10000` | mask `0x400000` |
| 1 | **0x60F337** mask `0x20000` | mask `0x800000` |
| 2 | **0x60F305** mask `0x40000` | mask `0x1000000` |
| 3 | **0x60F2BA** mask `0x80000` | mask `0x2000000` |
| 4 | **0x60F2D3** mask `0x100000` | mask `0x4000000` |
| 5 | **0x60F31E** mask `0x200000` | mask `0x8000000` |

- **一般式: emit1 = `0x10000<<t`、emit2 = `0x400000<<t`**（t=0..5）。7 呼出し（0x60F2BA/2D3/2EC/305/31E/337 + 共有 0x60F346）= b27 §3.2 の「7 箇所」と一致。
- スタック構造: 各 case は `push emit1_mask; push ebp(container=target+0x44); push eax(key); call; push emit2_mask; jmp 0x60F341` → 共有タール `push ebp; push eax; call`、末尾 `add esp,0x18`（6 push = 2 emit 分）。emit2 の push は **次 case の直前に残る**ため、**b11 Appendix の行単位表示は 1 ケースずれる**（例: 0x60F2D3 行に `0x2000000` が混入 = 前 case の残 push）。**集合としては一致、行対応は訂正要**（b16 §2-7 の対 6 組は正しい）。
- 0x6100b0（`0x61037D` mask 0x10000 / `0x61038C` mask 0x400000）は **状態リセット（`mov [0xb361c4],0`）とセットの購読更新** = crime type0 と victim type0 の再宣言。

**(b) §59.3 との突合**
- §59.3 の 14 ビット表 = `0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0x80000000`。
- 0x60F250 が emit する **13 値（crime 6 + victim 系 6 + 0x400000 の内 1）は §59.3 に全て不在** → §59.3 は 22 ハンドラのうち 14 のみ記載、残 8（b11 §4 追加）+ crime 系 13 値は b11/b16/b27 の追加分。**矛盾なし（包含関係で整合）**。
- **孤児ビット検出**: emit されるが **22 check のいずれも読まない mask 値 = 5**: `0x800000, 0x1000000, 0x2000000, 0x4000000, 0x8000000`（全て 0x60F346 の runtime 選択 mask2）。読み手になり得る OnAlarmVictim は式bugで v≥1 なら mask 0 のため、**実効的な読み手は 0（完全孤児）**、0x400000 のみ v=0 で到達可能。OnAlarm は emit1 の 6 値のみを読む。
- 逆方向（check 可能だが emit なし）は 0 件（§3 表で 77/77 突合）。

### §2-5 (5) 0x4d8260 / 0x4d82e0 の `obj+0x44` 裁定 — **解明（report 訂正を含む）**

**(a) 3 ヘルパの実体**（v138_1 §D + v138_4 §D）:

| 函数 | 本体 | 意味 |
|---|---|---|
| `0x4D8260` | `add ecx,0x44; jmp 0x41F830`（5B thunk） | this を **form+0x44（BaseExtraList）** へズラして 0x41F830 へ |
| `0x41F830` | `push 0x13; call 0x41E210` → 無ければ `eax=1`、有れば `eax=node[+0xC]` → `test arg, al; setne` | **extra type 0x13 の payload バイト（node+0xC）のビット演算テスト**（既定 flags=1） |
| `0x4D8270` | arg==4 特殊系 + `lea ecx,[esi+0x44]; call 0x423DF0` | **SET**（0x423DF0 = `flags |= arg`、node 無ければ `0x41EB90` = GetOrCreate extra 0x13（malloc 0x14 + ctor 0x429D30 + link）） |
| `0x4D82E0` | arg==4 特殊系（`0x4533F0` query の `0x40000` test + extra0x13 bit3(0x8) test + vslot+0x40/0x44 へ `0x80000`）+ `lea ecx,[esi+0x44]; call 0x423E50` | **CLEAR**（0x423E50 = `flags &= ~arg`）+ form フラグ更新 |

- **vtable slot +0x44（`(**(*form+0x44))(0x80000)` = form フラグ系メソッド）と field form+0x44（BaseExtraList）は別物** — 同オフセットの多重性に注意。

**(b) OnActivate impl 0x503F90 の正確な呼出し**（v138_9 §A、E8 目標を明示計算）:

```
0x503FDD push 2 / mov ecx,esi / call 0x4D8260   ; extra0x13 flags bit1(0x2) テスト
0x503FEA (成立時) fld1; fstp [edi]               ; 結果 double = 1.0
0x503FEE push 1 / mov ecx,esi / call 0x4D82E0   ; flags bit0 を CLEAR
0x503FF7 push 2 / mov ecx,esi / call 0x4D82E0   ; flags bit1 を CLEAR
0x504000 al=1; ret
```

- → report §59.3「0x4D8260 / 0x4D82E0 で test/**set**」は **不正確**: 実体は **test（0x4D8260）と clear（0x4D82E0）**。セット系は **0x4D8270**（OnActivate からは呼ばれない）。
- 0x4D8260 の thunk 化により Ghidra decompile は `void FUN_004d8260(void){FUN_0041f830();}`（this/引数も消える）→ **asm 必須**。

**(c) 同一フィールド裁定**
- **共通**: 両系統とも **`form+0x44` の BaseExtraList オブジェクトを this にする**（OnActivate: `add ecx,0x44`/`lea ecx,[esi+0x44]`、emit: `push target+0x44` → `0x4FBF90` が `mov ecx,esi; call 0x41E920`）。→ **フィールドは同一**。
- **相違**: 値の格納先は **別 extra type の別ヒープノード** — OnActivate = **extra 0x13 の node+0xC（1 byte flags）**、emit = **extra 0x12 の node+0x10（u32 event list ptr）**。`+0x44` の直下（presence bitmap `+8..+0x13` / ノードヘッダ `+4`）には**両系統のフラグは直接置かれない**。
- → 「**offset 衝突**」は発生しない（同一リスト上の別エントリ）。report の「obj+0x44 のフラグビットを直接 test/set」は **"extra 0x13 の flags バイト（node+0xC）を BaseExtraList 経由で test/clear"** に読み替えること。

### §2-6 (6) 22 check / 77 emit の 1:1 対応表 — **解明（77/77 精密一致）**

**全表**（check 側: b16 §2-3 の site/mask に v138_4 の impl 逆引きを追加。emit 側: b11 Appendix 77 行を v138_6 が機械グループ化 + v138_1/v138_7/v138_9 で mask を実アセンブル検証）:

| # | impl (check site) | ハンドラ (idx/opid) | mask | emit サイト（mask 未設定は実アセンブルで補完） | emit 数 |
|---|---|---|---|---|---|
| 1 | 0x503D10 (0x503D6D) | OnAdd (3/0x003) | 0x1 | 0x4D86F1, 0x4D8738, 0x4DDC5C, 0x4DDD9C | 4 |
| 2 | 0x503E10 (0x503E6D) | OnEquip (4/0x004) + OnActorEquip (28/0x01C) 共有 | 0x2 | 0x5E4515, 0x5E4521, 0x5FAEF1, 0x5FAEFD, 0x5FB2C5, 0x5FB35E, 0x5FB8D5, 0x5FB97A, 0x5FB986 | 9 |
| 3 | 0x503F10 (0x503F6D) | OnDrop (6/0x006) | 0x4 | 0x48FEE1, 0x490040, 0x4D878B | 3 |
| 4 | 0x503E90 (0x503EED) | OnUnequip (5/0x005) + OnActorUnequip (29/0x01D) 共有 | 0x8 | 0x4D8848, 0x5E42A8, 0x5E42B1, 0x690ECC, 0x690ED8 | 5 |
| 5 | 0x504150 (0x5041AD) | OnDeath (10/0x00A) | 0x10 | 0x5EA007 | 1 |
| 6 | 0x504250 (0x5042AD) | OnMurder (11/0x00B) | 0x20 | 0x5E9F4E | 1 |
| 7 | 0x5042D0 (0x5042EC) | OnKnockout (12/0x00C) | 0x40 | 0x6547C7, 0x6547E1 | 2 |
| 8 | 0x504050 (0x5040B0) | OnHit (8/0x008) | 0x80 | 0x5EE46F, 0x5FF630 | 2 |
| 9 | 0x5040D0 (0x504130) | OnHitWith (9/0x009) | 0x100 | 0x5FF608, 0x5FF61A, 0x5FF704, 0x60A864, 0x60B387（末尾 2 は push eax 型 → 実アセンブルで 0x100 確定） | 5 |
| 10 | 0x5043E0 (0x504440) | OnPackageStart (15/0x00F) | 0x200 | 0x512452, 0x5692CD, 0x64A49C | 3 |
| 11 | 0x504460 (0x5044C0) | OnPackageDone (16/0x010) | 0x400 | 0x50B6D7, 0x63A681, 0x63B693, 0x63BB5D, 0x63C430, 0x648194, 0x6494D6, 0x64A16E, 0x64A4AF, 0x64F744, 0x653EAF, 0x658325, 0x6597A8 | 13 |
| 12 | 0x5044E0 (0x504540) | OnPackageChange (20/0x014) | 0x800 | 0x51243C, 0x5692B7, 0x601D84 | 3 |
| 13 | 0x504560 (0x50457F) | OnLoad (21/0x015) | 0x1000 | 0x4422E5, 0x4D5C86, 0x4DC1E6 | 3 |
| 14 | 0x5045E0 (0x504640) | OnMagicEffectHit (22/0x016) | 0x2000 | 0x6A286B | 1 |
| 15 | 0x503D90 (0x503DF0) | OnSell (23/0x017) | 0x4000 | 0x59AB1E, 0x59AE3D, 0x62EEC3, 0x62EEF0, 0x64FEE5 | 5 |
| 16 | 0x5041D0 (0x504230) | OnStartCombat (25/0x019) | 0x8000 | 0x60124E | 1 |
| 17 | **0x504310 (0x5043B6)** | **OnAlarm (13/0x00D)** | **switch 0x10000..0x200000** | 0x60F2EC(0x10000), 0x60F337(0x20000), 0x60F305(0x40000), 0x60F2BA(0x80000), 0x60F2D3(0x100000), 0x60F31E(0x200000), 0x61037D(0x10000) | 7 |
| 18 | **0x50B3E0 (0x50B489)** | **OnAlarmVictim (14/0x00E)** | **2^n → 通常 0x400000** | 0x60F346（共有タール・runtime mask2、case0 のみ 0x400000 = 到達可）、0x61038C(0x400000) | 2 |
| 19 | 0x504660 (0x5046C0) | OnTrigger (24/0x018) | 0x10000000 | 0x5371D4, 0x5373E2, 0x5F9E62, 0x60B8D1 | 4 |
| 20 | 0x5046E0 (0x504740) | OnTriggerActor (26/0x01A) | 0x20000000 | 0x537230 | 1 |
| 21 | 0x504760 (0x5047C0) | OnTriggerMob (27/0x01B) | 0x40000000 | 0x5371F9 | 1 |
| 22 | 0x5045A0 (0x5045BF) | OnReset (30/0x01E) | 0x80000000 | 0x4CFE10 | 1 |
| — | （NO-CHECK 7 件: GameMode/MenuMode/OnActivate/OnYield_unused/ScriptEffectStart/Finish/Update — b16 §2-6 通り） | | | | 0 |
| | | | | **合計** | **77** ✓ |

- **検算 1**: emit 数合計 = 4+9+3+5+1+1+2+2+5+3+13+3+3+1+5+1+7+2+4+1+1+1 = **77** = cache `call_targets.json` の `0x4FBF90`=77 ✓（全 emit サイトが 1 ハンドラに 1 回ずつ属性付与、重複・取りこぼしなし）。
- **検算 2**: check 22 行 = cache `0x4FB5F0`=22 ✓。
- **§59.3 14 件 + b11 8 件の 1:1**:
  - §59.3 の 14 = 上表 #1,2,3,4,5,6,7,8,9,10,11,12,13,22（OnAdd/OnEquip/OnDrop/OnUnequip/OnDeath/OnMurder/OnKnockout/OnHit/OnHitWith/OnPackageStart/Done/Change/OnLoad/OnReset）→ **14/14 完全一致**。
  - b11 追加 8 = OnActorEquip（#2 共有）、OnActorUnequip（#4 共有）、OnSell #15、OnMagicEffectHit #14、OnStartCombat #16、OnTrigger #19、OnTriggerActor #20、OnTriggerMob #21 → **8/8 一致**。14+8 = 22 ✓（うち impl 共有 2 組 = distinct immediate mask **20 種** + 導出 2 系）。
- **mask 値の分布（emit 側、32 種）**: check で読める 27 種（即値 20 + OnAlarm 6 + 0x400000）+ **孤児 5 種**（§2-4(b)）。逆方向の孤児 check は 0。
- **key は emit ごとに独立**: OnAlarm 系は crime record の 2 フィールド（emit1=`[+0xc]`, emit2=`[+8]`）、OnHitWith 5 サイトは 3 通り（直接レジスタ key ×3 + vcall key ×2）→ check の arg2（Actor optional / 0 のとき key=0 ワイルドカード）で吸収、という b16 §0-3 のフォールバック設計と整合。

## §3 修正・訂正ポイント（既存文書へ親セッションが反映すること）

| 対象記載 | 本調査の訂正 |
|---|---|
| b16 §2-4「0x41e920 は**グローバル種別 0x12** のエントリを引く」 | 正: **BaseExtraList::GetExtraData(0x12)**（this = form+0x44、per-TLS キャッシュ + presence bitmap + リンクリスト）。グローバルレジストリではない |
| b16 §2-5「`push 0x100` ← **vcall の第2引数**」 | 正: push 2 個はすべて `0x4FBF90` の arg2/arg3。**vcall は 0 引数**（`add esp,0xc` 勘定）。mask=0x100 の結論は維持 |
| b16 §4-1「local_8 が 2..6 という範囲が警報の関係種別」 | 正: arg1 = **"Crime Type"**（argdesc 実測）。1..6 = 実行時 crime type 0..5（1-based）。local_10 の 1/0x400000/0/(u32 0) は式 `2^(22*(v+1))` の帰結 = v≥1 で死 |
| report §59.3「OnActivate は 0x4D8260/0x4D82E0 で obj+0x44 のフラグビットを直接 **test/set**」 | 正: test=0x4D8260（→0x41F830, **extra 0x13 の node+0xC バイト**を演算）、**set = 0x4D8270、clear = 0x4D82E0**（OnActivate は test→**clear×2**）。格納先は extra 0x13 ノードで `+0x44` 直下のビットではない |
| b11 Appendix の 0x60F2xx 行の mask2 归属 | 表示は「前回 call 以降の push」→ **行単位で 1 ケースずれ**。正は §2-4(a) の対応表（case t → 0x10000<<t, 0x400000<<t）。集合は一致 |
| b11 §3.1 histogram「mask 1: 2 件」等 | 行解析では mask 1 = **4 件**（0x4D86F1/0x4D8738/0x4DDC5C/0x4DDD9C、いずれも `push 1` を実アセンブル確認） |

## §4 新規残点（V143+ 起票粒度）

| 起票案 | 内容 | 優先度 |
|---|---|---|
| V-138a | **emit `this`（FUN_0060d950 param_2）のクラス同定**: FUN_0060c5b0 の 0x30-stride 配列 entry+0x20 由来、dynamic_cast は TESObjectREFR/Actor。rdata に vtable 定数が無く静的特定不能 → 配列構築元（0x60c5b0 内の entry 書き込み or その配列の所有者）から逆引き。**`[this+0x1C]` が formID か baseForm ポインタかの矛盾（0x60d950 の `>>0x10` カウンタ比較）も同時に裁定** | 中 |
| V-138b | **CS 側 "Crime Type" enum の基底（1..6 or 0..5）**: OnAlarm の switch 桁合わせと OnAlarmVictim の生死を決定。OblivionCS バイナリ参照、または ESM ブロックヘッダの short 即値実測（ESM 再スキャン禁止のため別タスク） | 中 |
| V-138c | **OnActivate の extra 0x13 フラグバイト仕様**: bit0(0x1)/bit1(0x2)/bit3(0x8)/0x4 の名称と「test → clear×2」ラッチ設計の意味、0x4D8270(set) の呼び出し元一覧（0x4D6E84/0x4D9C96/0x4DA494 等 17 caller を区分） | 中 |
| V-138d | **extra type 0x12 の登録元 0x428BB8 周辺と `0x4FBDC0` の関係**（Form コピー/ロード時の event list 移行か、0x4FBDC0=cluster 内 23 呼出しの正体） | 低-中 |
| V-138e | **`FUN_00607B60` のカウンタ**（`[0xB2EB3C]+1 & 0xFFFF`、下限 0x0A、`[0xB3B7D4]` キャッシュ）の正体（runtime mod index? process id?） | 低 |
| V-138f | **OnAlarmVictim の式 bug（`2^(22*(v+1))`）が実際に行使されるか**: vanilla スクリプトの OnAlarmVictim ブロック有無（ESM ブロックヘッダ調査）→ 「構造上死」か「型 0 のみ生還」かの確定 | 低 |
| （転記） | b16 §2-4/§2-5/§4-1、report §59.3 の訂正（§3 表）を親セッションが反映 | **高**（本タスクは編集禁止） |

## §5 自己検証

- **`python "%TEMP%\opencode\probe_harness.py" selftest` → 4/4 PASS**（着手時・成果物直前の 2 回、同一結果）。
- 内部整合:
  1. **emit 属性合計 77/77**（§2-6 検算 1）= b11 Appendix 行数 77 = cache `0x4FBF90`=77 ✓。
  2. check 22 = cache `0x4FB5F0`=22 ✓、impl↔mask は v138_4 の asm で 22/22 再確認。
  3. §59.3 の 14 ビット ⊂ 22 ハンドラ、b11 追加 8 ⊂ 22 → 14+8=22 ✓。
  4. 0x60F250: jumptable raw 6 エントリ × asm × decompile の **6/6 三声一致**（§2-4(a)）、emit 呼出し 7 = b27 §3.2 の 7 ✓。
  5. OnAlarm jt 0x5043CC の 5 エントリ（0x50437C..0x504398）= decompile case 2..6 の 5 値と一致（b16 の 5/5 検証を本タスクでも再現）。
  6. argdesc 2 種（OnAlarm/OnAlarmVictim = 0xB0ABDC 共有、OnHitWith = 0xB0A7E0）を raw hex から復元、name ポインタは `.rdata` 文字列実測（"Crime Type" / "Actor (optional)" / "ObjectID"）。
  7. 型表 0xB0A54D の byte0 フラグ 20/20（即値系 11 / 参照系 26）が gate の 2 本の switch case 集合と一致。
  8. OnActivate の call 目標 3 件をバイト列（E8 rel32）で再計算し `0x4D8260 / 0x4D82E0 / 0x4D82E0` を確定。
  9. vtable scan v1 の 0 件結果 → COL 構造体 unpack 誤り（td へ self を割当）を検出、v138_8 で修正 → 有効 RTTI vtable 1,715 本、slot+0x170 の 0x4D9B40 集合 7 クラスを取得。
  10. 数式: `pow(2^22, n)` の v=-1/0/1/6 → 1 / 0x400000 / 2^44→u32 0 / 2^154→u32 0 を python で再計算し asm（`add ecx,1` + fistp 低32）と一致。

## §6 証跡（すべて `%TEMP%\opencode\`、既存 docs 非変更）

| ファイル | 役割 | サイズ(B) |
|---|---|---|
| `v138_1_disasm.py` / `v138_1.txt` | 14 領域の逆アセンブル（本稿の asm 引用の主証跡） | 3,445 / 35,240 |
| `v138_2_decomp.py` / `v138_2.txt` | decompiled 14 関数の本文抜粋（argdesc 以外の機構裏付け） | 1,458 / 36,435 |
| `v138_3_scan.py` / `v138_3.txt` | 呼出し数・型表・jumptable・opcode_table | 7,419 / 10,853 |
| `v138_4.py` / `v138_4.txt` | **argdesc 復元（決定的）**・check impl・extra ヘルパ・RTTI 初動 | 6,145 / 24,632 |
| `v138_5.py` / `v138_5.txt` | caller/byte 検証・gate 保存経路・emit マスク機械グルーピング v1 | 5,865 / 9,570 |
| `v138_6.py` / `v138_6.txt` | 0x60d950 冒頭（this=param_2）・b11 Appendix 77→mask 表 | 6,748 / 9,802 |
| `v138_7.py` / `v138_7.txt` | emit mask 補完・extra0x12 wrapper・vtable scan v1・typeinfo | 6,497 / 16,395 |
| `v138_8.py` / `v138_8.txt` | **GetBaseForm 本体・typeinfo 名（+8 補正）・家族 vtable slots** | 5,169 / 7,586 |
| `v138_9.py` / `v138_9.txt` | **OnActivate call 目標・0x5FEBF0 emit 群・家族 vtable 全 23 行・slot impl** | 5,643 / 14,479 |
| `v138_10.py` / `v138_10.txt` | 0x60d950 実引数（entry+0x20）・+0x154 impl | 3,471 / 3,194 |
| （参照のみ） | `tmp/re/b16_alarm_masks.md`、`tmp/re/b11_event_bit_setters.md`、`tmp/re/b27_slot_residuals.md`、`docs/RE_EXE_REPORT.md` §59.3、`tmp/re/cache/opcode_table.json`、`docs/Oblivion_decompiled.c` | 無変更 |

## §7 Android 移植への示唆（実装しない）

- **事件ブロックの arg 処理**: `Crime Type` は short 即値 → `mask = 0x10000 << clamp(v-1,0,5)`、`victim = (v==0) ? 0x400000 : 0` を**式のまま**移植（`2^(22*(v+1))` の engine bug 再現要否は V-138b 次第）。key（Actor/参照引数）は「フォーム値 u32、未指定=0=ワイルドカード」。
- **vcall +0x170 は「0 引数 key 生成」**として実装し、`(container, 0x100)` を vcall 引数と誤読しないこと（fbf90 の arg2/arg3）。
- **BaseExtraList（form+0x44）** を 1 つの共通コンテナとして扱い、event list は **extra 0x12 の +0x10**、OnActivate ラッチは **extra 0x13 の +0xC バイト**として別フィールドに分離。extra type の presence bitmap（96bit）を先に実装すれば 0x41E210 相当の探索はそのまま流用可。
- **孤児ビット 5 種（0x800000..0x8000000）は check 不在** → 移植版でビットを立てても誰も読まない（互換性のためビットだけ立てておくか、 emit 自体を省くかは方針決定事項）。
## §8 追記（2026-10-27 独立再検証ラウンド）

前セッション（2026-10-26 22:13 完筆）の成果物に対し、本エージェントが**独立に probe で再裏取り**を行う。
対象は §0 サマリの 6 裁定のうち、地址・定数・表値に依存する主張（selftest / 呼出し数 / argdesc /
jumptable / thunk 本体 / GetBaseForm 本体 / emit 合計 77）。

### §8-1 証跡（中間スクリプト v138_11..v138_19 / `%TEMP%\opencode\`）

| スクリプト / 出力 | 内容 | B |
|---|---|---|
| `v138_11_verify.py` / `v138_11.txt` | 独立 .text E8 スキャン（cache 非依存）call 数・argdesc 逆引き・OnAlarm/0x60F250 jumptable・小関数本体・77 表機械再加算・OnAlarmVictim 数式 | 6,910 / 6,221 |
| `v138_12.py` / `v138_12.txt` | argdesc raw レイアウト確定（opt フラグ列）・0x60F2A0..0x60F360 完全逆アセンブル・jt raw 6 エントリ | 2,674 / 5,980 |
| `v138_13.py` / `v138_13.txt` | RTTI vtable slot+0x170=0x4D9B40 探索（7 クラス）・家族 primary・thunk 本体 | 3,742 / 1,528 |
| `v138_14.py` / `v138_14.txt` | extra ヘルパ caller 集計（test 9 / set 7 / clear 9、0x41F830 direct 17、union 25） | 1,702 / 1,244 |
| `v138_15.py` / `v138_15.txt` | check/emit mask ヒストグラム抽出・§59.3/b11 集合包含・孤児 mask・push 0x12 数・jt/call 突合 | 8,692 / 3,097 |
| `v138_16.py` / `v138_16.txt` | 異常 5 サイトのウィンドウ逆アセンブル（抽出 cut 起因の切り分け） | 1,793 / 4,914 |
| `v138_17.py` / `v138_17.txt` | 改良抽出（push 戻り数則 ps[-2]/ps[-3]）でヒストグラム再計算 | 5,628 / 1,445 |
| `v138_18.py` / `v138_18.txt` | **22 check / 77 emit の site→mask 完全一致検証** + 0x5FF704 関数整列 | 7,011 / 992 |
| `v138_19.py` / `v138_19.txt` | OnHitWith 0x5FF608 / 0x5FF61A の key 構造（direct vs vcall 判定） | 1,537 / 1,448 |

（v138_11..14 は API レート制限で停止した前回試行の遺産で、本ラウンドが結果を継承。v138_15..19 が本ラウンドの追加分。）

### §8-2 再裏取り結果

1. **呼出し数（cache 非依存の独立 E8 スキャン、v138_11 §1）**: `call 0x4FB5F0`=22 / `0x4FBF90`=77 / `0x4FB510`=1 → cache と **3/3 一致**。付測: `0x4FAE80`=278 / `0x41E920`=25 / `0x41E210`=257 / `0x4D8260`=9 / `0x4D8270`=7 / `0x4D82E0`=9 / `0x60A640`=1 / `0x60B120`=1 / `0x446CB0`=347（GT と一致）。
2. **argdesc（v138_12 §A raw）**: `0xB0ABDC` = `{name:'Crime Type', type=0x1C, opt=0}` + `{name:'Actor (optional)', type=0x06, opt=1}`（+0x18/+0x24 は argn=2 の未使用枠 = 'Crime Type'/'Criminal'）。§2-1(a) と**完全一致**。OnHitWith `0xB0A7E0` = 'ObjectID'(0x15, opt=1)。※ v138_11 の optional 表示は列オフセット誤りで、raw の v138_12 が正。
3. **OnAlarm jumptable 0x5043CC**: 5 エントリ **5/5 MATCH**、case 本体バイト `b8 00 00 02..20 00` = 0x20000..0x200000、既定 `b8 00 00 01 00` = 0x10000（v138_11 §3）。
4. **thunk / ヘルパ本体（v138_11 §5, v138_13 misc）**: `0x4D8260` = `add ecx,0x44; jmp 0x41f830`、`0x41F830` = `push 0x13; call 0x41e210`、`0x423DF0` / `0x423E50` = `push 0x13` 起始 → §2-5 の extra 0x13 裁定と一致。`0x41E920` = `push 0x12; call 0x41e210; mov eax,[eax+0x10]`、文字列 `0xA34E5C` = 'BaseExtraList::GetExtraData'。
5. **GetBaseForm `0x4D9B40`** = `mov eax,[ecx+0x1c]; ret`。RTTI 再走査で slot+0x170=0x4D9B40 を持つクラスは **7 つ**（Actor / ArrowProjectile / Character / Creature / MobileObject / PlayerCharacter / TESObjectREFR）、b27 家族 primary 4 本（0xA6E074/0xA6FC9C/0xA710F4/0xA73A0C）も一致（v138_13）。
6. **mask 完全一致（本ラウンドの中心結果）**: site→mask の 1:1 突合で **check 22/22 / emit 77/77 = §2-6 表と一致**（v138_18）。ヒストグラムも check = 20 即値種 + 2 導出、emit = 27 即値種（76 サイト）+ 1 動的（0x60F346）で一致（v138_15/v138_17）。
7. **§59.3/b11 の包含関係**: §59.3 の 14 ビット ⊂ check 即値 **14/14**、b11 追加 8 ⊂ check 即値 **8/8**、union = 22（distinct 20 = impl 共有 2 組）。孤児 mask = `0x800000 / 0x1000000 / 0x2000000 / 0x4000000 / 0x8000000` の **5 種一致**、逆方向孤児 = 0（v138_15 §3/§4）。emitted set 32 / readable set 27 も §2-6 の記載と一致。
8. **`push 0x12` → `call 0x41E210` = 11 箇所**（§2-3 の 11 と一致）。0x60F250 の mask2 6 push（0x400000/0x800000/0x1000000/0x2000000/0x4000000/0x8000000）も raw バイトで **6/6 OK**（v138_15 §4/§5）。
9. **OnAlarmVictim 数式** `u32(2^(22*(v+1)))` を v=-2..6 で再計算 → 0 / 1 / 0x400000 / 0（v≥1）が §2-1(d) 表と一致（v138_11 §9）。
10. **0x5FF704 の関数整列逆アセンブル**（台帳関数 0x5FEBF0 起点、enclosing_fun の Ghidra ラベル解決は `fun_00baf2db` と誤ルックアップのため named_funcs.txt の二分探索で代替）で `push 0x100; lea eax,[esi+0x44]; mov eax,[edx+0x170]; call eax; push eax; call 0x4fbf90` を確認 → 0x60A864 / 0x60B387 と**同一構造の vcall キー第 3 サイト**。

### §8-3 本ラウンドの訂正（3 件 — §3 表へ追記するもの）

| 対象 | 本文記載 | 訂正 |
|---|---|---|
| §2-4(a) jumptable エントリ列 | `case5=0x60F31E` | **エントリは 0x60F311**（raw u32[5]=0x60F311、v138_12 §C / v138_18 §6）。0x60F31E はその case 内の emit1 **call** アドレス。mask 対応表（case5 → emit1 0x200000）は変化なし → 訂正はエントリ列表記のみ |
| §2-6 行9 および行末注記 | OnHitWith「直接レジスタ key ×3 + vcall key ×2」「末尾 2 は push eax 型」 | 正: **直接 key ×2（0x5FF608 = `[esp+0x2c]` / 0x5FF61A = `[ebx+8]`）+ vcall +0x170 key ×3（0x5FF704 / 0x60A864 / 0x60B387）**。「3 通り」自体は正しい（direct 2 種 + vcall）。5 サイト全て `push 0x100` 即値で「push eax 型（レジスタ mask）」は**実測該当なし**（b11 訂正の追補、v138_19 / v138_16） |
| §4 V-138c | 「0x4D8270(set) の呼び出し元一覧（0x4D6E84/0x4D9C96/0x4DA494 等 17 caller）」 | 17 は **0x41F830（test 本体）の直接 caller**。0x4D8270 は **7 caller**（0x4B92A6/0x4B94DD/0x4DA4B4/0x4DE499/0x5076EA/0x5076F3/0x50C495）。0x4D8260 = 9 / 0x4D82E0 = 9 / union = 25（v138_13 / v138_14） |

補足（**非訂正**）: v138_15 の機械抽出は共有タール `0x60F346` を直前 fall-through（case1）の `push 0x800000` に引きずられて 0x800000 と誤分類した。これは抽出ルールの限界であり、**§2-4/§2-6 の「DYN（runtime 選択）」帰属が正しい**ことを v138_12 の完全逆アセンブル（6 case 全て `push mask2; jmp 0x60f341` → 共有タール）で確認した。

### §8-4 内部整合（再検証後の確定数）

1. 独立 call スキャン 22 / 77 / 1 = cache 22 / 77 / 1（**3/3**）。
2. site→mask 完全一致 **check 22/22、emit 77/77** = §2-6 表（v138_18）。
3. emit ヒストグラム: 即値 27 種の件数が §2-6 表の行合計と **27/27 一致**、DYN 1（0x60F346）。check ヒストグラム: 即値 20 種 + DYN 2 = **22/22 一致**。
4. §59.3 14/14 + b11 8/8 の集合包含、14+8 = 22（distinct 20）。
5. 孤児 mask 5 種一致・逆方向 0 / push 0x12 = 11 / mask2 push 6/6 / OnAlarm jt 5/5 / RTTI slot+0x170 = 7 クラス。
6. 本文の合計検算 77 = Σ行（rows=22, sum=77）を v138_11 §8 が機械再加算で再現（**MATCH 77**）。

### §8-5 selftest

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（§8 着手時・本節完了直前の 2 回、bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
