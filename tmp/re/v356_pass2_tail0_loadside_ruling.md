# V356: pass2 tail==0 分岐後の entry 処理と Δ=0x1C ロード側対応の裁定 — #128 束②（#109 v334 item7/item10）

- 日付: 2026-11-13 / タスク: ai-todo **#128 束②**（#109 静的残束②: pass2 の tail==0 分岐後の entry 処理／Δ=0x1C ロード側直接対応） / 作成: RE レーン
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: `tmp/re/v334_ess_next_bundle_T_ruling.md` item7/item10（ともに [未確定]）＋ `tmp/re/v328_ess_phaseB_ruling.md` 残件7/残件10・P4/P11/§2-1
- 手法: capstone 5.0.7（v307_common.py）／**頭起点の整列リニアデコード**（v328 L142 ルール遵守）／E8 rel32 逆算全走査／即値バイト範囲走査
- 中間証跡: `%TEMP%\opencode\v356_1..v356_5.py/.out`（**全 EXIT=0**）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM/.ess 検体未アクセス、EXE 読み取り専用、L1

---

## §0 判定サマリ

| 設問（v334） | 判定 |
|---|---|
| item7: `tail==0` 分岐の探索失敗後に entry をどう扱うか | **[確定]** 失敗全点は **0x461E8C（次 entry へ）に合流＝削除・無効化・flags 変更・tail 書換のいずれも行われない**。成功時のみ bit3 処理/cleanup（452DF0 削除・tail 再発行）へ進む |
| item7（従属）: `[ebx+0x14]=0` 後の entry 成否 | **[確定]（v334 前提を訂正）** `[ebx+0x14]=0` は **tail==0 経路（0x461BCC+）には1件も存在しない**。存在は tail≠0 経路の3か所で、いずれも**削除トリガではない**（0x461BB3 は ebp=0 で即 0x461E8C=生存） |
| item7: ファイル帰結 | **[部分確定]** 保存ライタ帯 0x464800-0x465800 に `[reg+0x14]` 参照 **0件**＝+0x14 は書出しゲートではない。fail entry は無加工でライタへ到達するまで静的に確認。最終残存の動的裏付けは #82 系 |
| item10: Δ=0x1C（28B）の load 側直接対応 | **[確定(静的)] load 側に 28B 固定読取は存在しない**。45BB00 読取ファンクタの**全7 caller** のサイズ = {4, 6, 0xC, 0x24, 0x2C, 可変u16, 4}、範囲走査 0x464800-0x467000 でも `push 0x1C`=**0件**・`mov ecx,7`=**0件** |
| item10: 28B が 36B 読取帯（0x465EB1）に包含されるか | **[確定負]** 36B 読取は **bit1=1 ∧ bit2=0** のみ到達（0x465D60-0x465D6A）。created 系（bit1=0）は 0x4660EA 経路へ分岐し **36B に到達不能** |
| item10: created flags/type 組合せで別経路か | **[確定]** 別経路＝`¬bit1` 経路（0x4660EA）。bit31 立→0x2C=44B 読取、bit31 消→0x4662F0（GetForm→台帳 entry の bit31 立時のみ `rep movsd ecx,0xB`=44B 再構成）。**28B 専用経路は存在しない** |

---

## §1 pass2 のループ頭と tail==0 判定 [確定(命令)]

整列デコード（頭 0x461680 起点・635 insn・0x461680-0x461EDC）:

```asm
461A51: ebx = [esp+0x18]（台帳 cursor）
        call 0x452600 (stdcall) → 現 entry を [esp+0x14] へ
461A75: eax = entry
461A79: edx = [entry+0]        ; ★ flags（後段 0x461CAF `mov ebx,[esp+0x24]; test bl,8` で確認）
461A7B: eax = [entry+4]        ; ★ tail ワード
461A80: cmp eax, 0 / mov esi,0x7fffffff / mov [esp+0x24],edx / mov [esp+0x1c],0
461A96: je 0x461bcc            ; ★ [entry+4]==0 → tail==0 経路（v328 P2 の tail 判定と同物）
        ; --- tail≠0: 0x461A9C+ ---
```

- **外周ゲート bit27（0x4617C8）・bit3 処理（0x461CB3-）は v328 P4 の記載どおり**で本裁定の改訂なし。

---

## §2 item7: tail==0 経路（0x461BCC-）の全帰結 [確定]

### 2.1 失敗点の全列挙（すべて 0x461E8C へ）

| # | 位置 | 条件 | 結果 |
|---:|---|---|---|
| 1 | 0x461BDB | `call 0x46b250`（GetForm）→ `test eax,eax; je` | entry 不変で次へ |
| 2 | 0x461BF6 | formType ∉ {0x31,0x32,0x33}（`cmp ecx,0x31/32/33; jne`） | 同上 |
| 3 | 0x461C0B | `call 0x6ecc80`（parent/worldspace）→ NULL | 同上 |
| 4 | 0x461C43 | `test ebp,ebp; je`（グリッド解決 0） | 同上 |
| 5 | 0x461C4F / 0x461C5B | `cmp esi/edi, 0x7fffffff; je`（GetCellX/Y 未解決） | 同上 |
| 6 | 0x461C7A | `call 0x55e000`（map find）→ `test al,al; je` | 同上 |
| 7 | 0x461CA8/0x461CAA | (x,y) 一致走査の尽き（linked-list 到達） | 同上 |

```asm
461E8C: cmp dword [esp+0x38], 0
        jne 0x461a51        ; ★ 次 entry へ（ループ継続）
```

- **削除（452DF0）・無効化・flags 変更・tail 書換は一切ない** → **探索失敗 entry は台帳に生存** [確定]。
- **成功時のみ** 0x461CAF へ: bit3 単独→`call 0x452df0`@0x461CD7（削除）／bit3 複合→tail 再発行（0x461D22 `sub ax,di`（di=0x1C or 0x2C）・bit23 で +4・`lea eax,[edi+esi+4]`＝old+4+Δ を再 append @0x461D94・`mov [edi+0x14],0`@0x461DB1）／通過→cleanup（{32,33} created 述語 446B80・4DB4A0・vfunc+0x78・4D9040 全滅で 452DF0@0x461E42＋dtor@0x461E54、0x31∧bit17(0x20000) で 452DF0@0x461E78＋dtor@0x461E8A = v317 L158 と一致）。

### 2.2 `[ebx+0x14]=0` の実在位置 — v334 の前提を訂正 [確定]

| 位置 | 経路 | 意味 |
|---|---|---|
| 0x461A9C `mov [ebx+0x14],eax` | tail≠0 初期 | 初期 store（非0） |
| 0x461BB3 `xor ebp,ebp; mov [ebx+0x14],ebp` | tail≠0 | `[ebx+0x74]` 配列の count 超過 → ebp=0 |
| 0x461BC3 `mov dword [ebx+0x14],0` | tail≠0 | `0x446b80` created 判定 true |
| 0x461DB1 `mov [edi+0x14],0` | tail≠0 再発行完了 | 再発行後のクリア |

- **tail==0 経路（0x461BCC-0x461CAF）に `[reg+0x14]` 書き込みは0件** → **v334 item7 の「0x461BCC 付近に探索失敗時の [ebx+0x14]=0 経路」は記述誤り（実際は tail≠0 側）** [確定]。
- **+0x14=0 後の成否**: 0x461BB3 は `jmp 0x461c41` → `test ebp,ebp; je 0x461e8c` で**即生存**。0x461BC3 は ebp 有効のまま探索継続（0x461CAF 到達可）。→ **+0x14=0 は削除トリガではない** [確定]。

### 2.3 保存ライタと +0x14 の独立性 [確定]

- **0x464800-0x465800（保存/ライタ帯）の `[reg+0x14]` 参照 = 0件**（v356_4 c2）→ **+0x14 はファイル書出しのゲートにならない**。
- 452DF0（entry 削除）の本体: `55E000` map で formID 探索 → `4524c0` unlink → `401e40/401f20` で解放。**ガード**: `[0xb33b00]+0x18 >>0xC & 1` 立なら**削除せず return 0**（v317 の14 caller と本件14件一致）。
- → **ファイル帰結 = [部分確定]**: fail entry は削除されず無加工でライタへ到達。最終ファイル残存の可否はライタの flags 系ゲート＋動的検証（#82 系＝残点 V-356-1）。

---

## §3 item10: load 側の読取サイズ全列挙 [確定(静的)]

### 3.1 45BB00（可変ペイロード読取ファンクタ）の全 E8 caller = 7件

| caller | 直前の push | サイズ | ゲート（v328 §2-1＋本件で補強） |
|---|---|---|---|
| 0x465D97 | `push 4`@0x465D8F | **4B** | bit1∧bit2 ∧ ver≥0x5B ∧ bit26(0x4000000) |
| 0x465DC0 | `push 6`@0x465DB8 | **6B** | bit1∧bit2 ∧ ver≥0x5B ∧ ¬bit26 ∧ bit25(0x2000000) |
| 0x465DFF | `push 0xc`@0x465DF4 | **12B** | bit1∧bit2 ∧ ver<0x5B（`cmp [ecx+0x7c],0x5b; jb`） |
| 0x465ED6 | `push 0x24`@0x465EB1 | **36B** | **bit1=1 ∧ bit2=0**（0x465D60 `test al,2` set → 0x465D68 `test al,4` je） |
| 0x466113 | `push 0x2c`@0x466108 | **44B** | **¬bit1 ∧ bit31**（0x4660EA `test eax,eax; jns 0x4662f0` の ns 解除＝負＝bit31 立のみ fall-through） |
| 0x46667E | `push eax`（u16 可変）@0x466679 | **可変** | 汎用ブロック（`movzx eax,word [esp+0x26]`）＝固定 28B ではない |
| 0x466816 | `push 4`@0x4667D7 | **4B** | （帯下部の別経路） |

- **範囲走査 0x464800-0x467000**: `push 0x1C`=**0件** / `mov ecx,7`（28B memcpy）=**0件** / `push 0x24`=1件(0x465EB1) / `push 0x2C`=1件(0x466108) / `mov ecx,0xB`=1件(0x466327)。

### 3.2 判定

1. **[確定(静的)] load 側に 28B 固定読取は存在しない**（固定サイズ集合 = {4,6,0xC,0x24,0x2C}＋可変1経路）。
2. **[確定負] 「28B が 36B 帯に包含」は不成立**: 36B 読取は bit1 ゲート（`test al,2`）の set 側のみ到達で、created 系（bit1=0）は 0x4660EA へ分岐し物理的に到達不能。
3. **[確定] created 系の load 側は `¬bit1` 別経路**: bit31 立→44B 読取、bit31 消→0x4662F0 → `call 0x452d30`(GetForm) → 台帳 entry の `test dword [eax],0x80000000`@0x46630D 立時のみ `mov ecx,0xb; rep movsd`@0x466327 で **44B を tail+4 から再構成**（v328 P1 の記載を命令で確認）。
4. **[部分確定] 28B が load 側にないのは構造的に整合**: pass2 が保存時に Δ=28B を tail から除去する（v328 P11: 再発行 tail=old payload[Δ:]）ため、本エンジン書出ファイルには 28B ブロックが残らず、対応読取が不要。**ただし本検体に created 系 entry が0件**（v328 §6-10）ため、動的裏付けは #82 系で残す（残点 V-356-3）。

---

## §4 既知の補強（再利用可能な事実）

1. **entry レイアウト（pass2 視点）**: `[entry+0]`=flags（bit27=0x8000000 外周ゲート・bit3/bit1/bit2/bit17/bit23/bit31 使用）、`[entry+4]`=tail ワード（0 なら tail==0）[確定(命令)]。
2. **452DF0 = 台帳 entry 削除**（55E000 探索→4524C0 unlink→解放）＋**`[0xb33b00]+0x18>>0xC & 1` 立なら抑止** [確定]。
3. **load 側 gate 表の完全形**（4/6/0xC/0x24/0x2C/可変、bit1/bit2/bit25/bit26/bit31/ver0x5B）[確定]。
4. **pass2 tail==0 成功ループの解決チェーン**: 46B250 GetForm → formType{31,32,33} → 6ECC80 → 4C97F0/4C9CF0/4C9A80/4C9AA0 → 55E000 map → (x,y) 一致 [確定]。

---

## §5 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v356_1..v356_5 **全 EXIT=0** |
| 2 | 整列デコード | 頭 0x461680／0x465860 起点のリニアデコードで分岐先へ線形到達（v328 L142 CCラン回避ルール遵守） |
| 3 | 28B 不存在の二重根拠 | 45BB00 全7 caller の push 列挙＋0x464800-0x467000 の `6A 1C` バイト走査の**両方で0件** |
| 4 | 36B 到達不能 | `test al,2`→je 0x4660ea の分岐構造を命令で直接確認（ゲート表 v328 §2-1 と整合） |
| 5 | v334/v328 との整合 | item7 の +0x14 位置は v334 記述を**訂正**（本文 §2.2）。item10 の 44B 再構成は v328 P1 と一致。v328 残件7/10 の [未確定] は本裁定で更新 |

---

## §6 新規残点

| ID | 内容 | 深刻度 |
|---|---|---|
| V-356-1 | [部分確定] 探索失敗 entry の**最終ファイル残存**（ライタの flags 系ゲート照合と動的検証＝#82 系） | 中 |
| V-356-2 | [未確定] 7件目 caller 0x46667E の可変長読取（u16 サイズ）のデータ意味 | 低 |
| V-356-3 | [未確定] created 系 entry の **load 側動的裏付け**（本検体0件・#82 系） | 低 |
| V-356-4 | [部分確定] `[entry+4]` ワードの正式名（tail 長 vs tail ptr）＝ファイルライタ側の参照照合が未了 | 低 |

---

## §7 main への報告

1. **v334 item7 は [確定] で解消（前提訂正付き）**: tail==0 探索失敗は全点 **0x461E8C＝生存（削除/無効化なし）**、`[ebx+0x14]=0` は **tail==0 経路に存在せず tail≠0 経路の3か所**（いずれも削除トリガではない）。
2. **v334 item10 は静的解析で大幅前進**: **load 側に 28B 読取は存在しない [確定(静的)]**、**36B 帯包含は [確定負]**（bit1 ゲートで到達不能）、created 系は ¬bit1 別経路で **44B のみ**。残る動的裏付けは #82 系（V-356-3）。
3. **v328 残件7/10 の [未確定] 表記は本裁定で更新可**。docs/GT 反映は #47/#20 一括（L1）。

---

### 参照（本タスクの証跡）

- v356_1（pass2 頭整列デコード・tail==0 経路全文＝§2 の核心）
- v356_2（452DF0 本体・全14 caller・+0x14 書き込み列挙）
- v356_3（load dispatch 地域・即値走査＝28B 不存在の根拠1）
- v356_4（ループ頭・**45BB00 全7 caller＋push サイズ**・load ゲート全文・ライタ帯 +0x14=0件）
- v356_5（7件目 caller の可変 push・bit31 ゲート `jns 0x4662f0` の確認）
