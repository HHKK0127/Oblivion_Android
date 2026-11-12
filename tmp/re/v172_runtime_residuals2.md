# V172: opcode 実行時残点 第2束（12件）— Begin u32 / +0x24 / FAB20 タグ表 / R1 scratch / RunScript 8引数 / emit wrapper 所有者 / serial キャッシュ / type0x3D / Update 内部順序 / 台帳結合 / +0x58 帰属 / slot+0x190

- 日付: 2026-10-28（opencode サブエージェント・分析専任／docs・TODO 編集なし）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416, ImageBase 0x400000）
- 出典: `tmp/re/v157_runtime_residuals.md` §3/§10 新規残点 O-a..O-e（5件）＋ `tmp/re/v158_slot_opcode_map.md` §5 起票案 V-158a..V-158g（7件）＝計 12 件
- 前提確認: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時）
- 手法: capstone 5.0.7（v172_common.py: raw E8 走査・セクション imm32 走査・int3 境界逆アスク）。中間スクリプトは `%TEMP%\opencode\v172_*.py`、出力 `v172_p*_out.txt`。**repo 内新規ファイルは本レポート 1 件のみ**。
- 制約遵守: git / docs / TODO / session-memory / plan / func_list_new / cache / 既存 `tmp/re/*.md` は**無編集**、ESM 未アクセス。

## 結論サマリ（12 行）

| # | 残点 | 判定 | 要旨 |
|---|---|---|---|
| 1 | O-a Begin u32 の意味論と上位 consumer | **解明** | u32 = **Begin ブロック本文のバイトコード長（PC スキップ量）**。executor は `*arg4 += u32`、caller1 は 0x51755E `add eax,[esp+0x1c]` で **PC に加算してブロック全体を飛ばす**。上位 consumer は**存在しない**（ローカル [E1+0x1C] は毎イテレーション 0 にリセットされ内部消費のみ。wrapper の戻り値は al=実行成否のみ）。v157「行番号」説は否定。 |
| 2 | O-b +0x24 列（argflag=1）デッドカラムの理由 | **解明** | オペコード表領域（0xB0AF48..0xB0F2E8、.data の raw バック付き静的領域）への**実行時書手 0 件**＝表は PE ファイル静的データ（実行中に構築・更新されない）。+0x24/+0x25 は行構造の最終 2 バイト（+0x26/+0x27 パディング）で、+0x25 のみが実行時消費者（executor 0x517295）を持ち **+0x24 は静的バイナリに焼き込まれたまま誰も読まない予約/区分列**。argflag=1 の 9 件 condition getter は「参照必須 getter の区分标记」が静的にセットされたもので、リテールコードは未使用＝編集ツール/旧バージョン向けの残置列と裁定。 |
| 3 | O-c FAB20 ジャンプテーブル全タグ表 | **解明** | byte リマップ表 0x4FAE4C + case 表 0x4FAE3C を**全 52 エントリ実測**。有効タグ 9 種の consume パス全解剖（'s' 等のリテラル系は 0x4FA110 で type ごと解決→double 出力）。'X'/'n'/'z' の emit 元は別コンパイル系（V147-r1 継続）。 |
| 4 | O-d R1 [esp+0x1C] スクラッチの網羅性 | **解明（一般化確定）** | R1 条件ヘルパ **89 件全数**の内訳: `mov [esp+X],r32 → fild [esp+X]` ペア = **16 件**（すべて int 戻り値）。int 戻り値ヘルパ計 21 件のうち 5 件は `fild [obj+off]` 直読でスクラッチ不要。**float 戻り値 64 件・bool 4 件には 0 件** → 「arg4 スロット read-after-hoist 再利用＝int→f64 変換時のみ」で全数一致。 |
| 5 | O-e RunScript ラッパー 8 引数の意味論 | **解明** | 8 引数は caller1 へ**恒等写像**。W1=スクリプト/フォーム、W2=ref（vt+0x170 GetBaseForm 対象＋executor ref フォールバック）、W3=event list（0 なら 0x4FBDC0 自動生成）、W4=実行コンテキスト（[frame+0] 格納）、W5=byte フラグ（end 時 0x4FA0E0 event list 実行の要否）、W6/W7=byte パラメータ（event ノード byte0/1）、W8=**float パラメータ**（event ノード +4 へ fstp）。ラッパー本体は**再入フレーム管理**（[frame+0x14]=実行中スクリプトが非 0 なら第 2 フレームを new）。 |
| 6 | V-158a emit wrapper 所有者 [owner+8] と入れ子実証 | **解明** | owner = **CombatController (CC)**。[CC+8] = **cc+0x14 に続く 0x30 バイト内包オブジェクト**（+0x1C = パック serial）で、パック書手 9 箇所の `P=[X+8]; P+=0x14; [P+0x1C]=` と 0x60d950 読手 `[param_2+0x1C]` は**同一オブジェクト**。param_2 = [CC+8]+0x14 を**型情報（ラッパ構造 +0x10 相対 ptr / +0x18 mode）と書手 site の X 特定で実証**。 |
| 7 | V-158b [0xB3B7D4] キャッシュの無クリアと dedup 実効性 | **解明** | [0xB3B7D4] への**書手は 0x607B60/0x6094C0/0x6095B0/0x6098C0 の 4 write のみ**（全て「非 0 のみ書く」型）。ゼロクリア・間接クリア（base+off / reg-indirect）を含めても**書手総数 0**。serial は**プロセス生存期間固定**（初回採番 0x0B）。dedup は「stamped serial>>16 == 現行 serial」比較＝**最初の採番以降、全 stamp が同値になる単発 dedup**として機能（非メモ化系カウンタ増分はキャッシュを追い越さない）。 |
| 8 | V-158c type0x3D float extra の多用途性 | **解明（v158 の帰属を訂正）** | 共有 0x4F8B90 は **0x3D と無関係**（IsCellOwner 専用の owner-ID 比較述語、唯一の caller = 0x505E4C）。type0x3D float 読手 0x41FC90 の caller は 5 件のみで、条件系では **IsHorseStolen（0x4F8C20 → [horse+0x44] extra list → 0x41FC90 → しきい値比較）だけが使用**。IsSneaking は 0x5E0550（sneak 状態）、IsRunning は process vcall +0x2C0 の bit9(0x200) を読み **extra 非使用**。type0x3D の本命用途 = crime gold（writer 0x4269E0=SetCrimeGold 系）。v158 の 4 opcode 帰属は誤り。 |
| 9 | V-158d 参照 Update 実装の内部順序 | **解明（「script→AI 直列」の否定）** | 0x4E3F30（Actor/REFR +0x160、vtable 実 bytes で確定）の本体は **フラグゲート（bit5/bit11）→ base form 参照配列走査 → エフェクト減衰チェーン（0x49F950/0x4E0D90/0x6C9CB0/0x4D90D0、0x49F950 は Render 帯 0x6AA980/0x6AC420/0x6AE0A0 を呼ぶ）** で、**スクリプト実行も AI も直接含まない**。Player 0x65FF40（0x65FF40..0x6600A3）もアニメキュー/extra/描画連動で script 直呼びなし。⇒ 参照 Update に「スクリプト実行 → AI tick」の直列順位は存在せず、両者は 0x4CCC50 ループ内で参照毎に逐次処理。script/AI は process 帯（HighProcess +0x160=0x629810 等）と event/opcode ディスパッチ層に分離。 |
| 10 | V-158e expression getter 帯と evaluator の台帳結合粒度 | **裁定: 台帳のみ粗い（最大 15:1）、機械データは精密** | 帯 0x4F5200-0x4F6100 は **int3 分離された 34 個の独立関数**だが、func_list 台帳エントリは 5 件のみ（FUN_004F54A0/5650/5730/5EA0/6060）。1 台帳エントリに最大 15 実関数。relay2 の CSV は call target の実アドレスで記録済み＝**帰属精度の問題は「ラベル」側のみ**（"共有 evaluator 経由" 表記が 15 関数を束ねる）。V-158c の誤帰属（IsSneaking/IsRunning→0x3D）はこの帯の relay2 ラベル由来であり、本裁定の実例。 |
| 11 | V-158f 直接ストア +0x58 5 件のクラス帰属 | **解明（REFR/CC ではなく位置オブジェクト）** | 5 件 = TestLocalMap(0x50EF90×2)/ShowPivot(0x511C10×2)/SetPos(0x508FC0×1)。全 site が **vcall +0x154/+0x174 の戻りオブジェクトへの連続 float フィールド書出し**（+0x4C..+0x58 = 位置ベクトル帯、ShowPivot は +0x50/+0x54/+0x58 に位置 3 成分を転記）。**TESObjectREFR の process 書手でも CC の latch でもなく、第 3 系統 = Ni/Transform 系位置オブジェクト**。`class unverified` を解消。 |
| 12 | V-158g 0x977C50 共有と slot+0x190 の真意 | **解明** | slot+0x190 = **Update スキップゲートの一部**: 0x4CCC50 実測で「**gate==1 かつ [esi+0x78]==0 かつ !0x45A500([0xB33B00]) → +0x160 を skip**、それ以外は call」。Actor 家族 gate=0x977C50（`mov al,1;ret` 常真）＝残り 2 条件で判定、素の REFR gate=0x69D990（`xor al,al;ret` 常偽）＝無条件 update。vtable 実 bytes で 2/2 確認。7 opcode（TestCode/BeginTrace/ShowNameMenu/TrapUpdate/AddAchievement/StartMasterFileSeekData/DumpMasterFileSeekData）は全て**リテール no-op の debug 系**で、常真スタブの意図的再利用。留保: Process 系 vtableの同 offset は別メソッド（Low=0x60D880 `ret 4` 等）＝slot はクラス系統別 API。 |

---

## §1 【残点1】Begin u32 の意味論と上位 consumer — **解明**

### 1.1 caller1（0x517330）の実行ループ（本調査で全文復元）

```
0x5174BF  xor ebp, ebp                       ; カウントダウン=0（通常実行モード）
0x5174C1  cmp dword [edi+0x20], ebx          ; script code size == 0 → 終了
0x5174C8  mov [esp+0x20], ebp                ; [E1+0x20] = 0
0x5174D4  jbe 0x51757b
0x5174DA: add dword [esp+0x28], 1            ; ★ 文カウンタ（arg7 = "Offset %d"）
0x5174E4  lea eax,[esp+0x24] / lea ecx,[esp+0x1c]
0x5174EF  mov dword [esp+0x1c], ebx          ; ★ [E1+0x1C] を毎イテレーション 0 にリセット
0x517503  call 0x516830                      ; fetch: (script, &opcode[E1+0x38], &X[E1+0x20], &len[E1+0x1C], &ref[E1+0x24], skip?)
0x51751D  cmp ecx, 0x1e / je 0x51757b        ; opcode 0x1E = センチネル（終端）
0x51752A  mov eax,[esp+0x2c]                 ; ref フォールバック = caller1 arg2
0x51754D  call 0x5169a0                      ; executor(stdcall 9引数): arg4=&[E1+0x1C], arg5=[E1+0x18](PC), arg7=[E1+0x28]
0x51755A  mov eax, dword ptr [esp + 0x18]    ; ★ eax = PC
0x51755E  add eax, dword ptr [esp + 0x1c]    ; ★★ PC += [E1+0x1C]（= fetch の文長 + Begin の u32 累積）
0x517562  cmp eax, dword ptr [edi + 0x20]    ; PC < code size → ループ
0x517565  mov dword ptr [esp + 0x18], eax
0x517569  jb  0x5174da
```

### 1.2 u32 の消費経路（確定）

1. fetch（0x516830）は**文長**を `[E1+0x1C]` に出力（Begin の header = i16+u32 = 6B / OnActivate は +u16 で 8B）。
2. executor（Begin 経路 0x516B17-0x516B1F）: `mov ecx,[esp+0x2c]`（u32）→ `mov eax,[esp+0x20]`（arg4 = &[E1+0x1C]）→ `add [eax],ecx` = **文長 += u32**。
3. caller1 0x51755E: **PC += 文長** ⇒ **u32 = Begin ブロック本文のバイトコード長**（ブロック全体を PC 加算でスキップするための語）。

- s59 実測値（219 / 275 / 280 / 95）は「本文バイト長」として整合（ハンドラ本文サイズはブロック毎に異なる）。
- **v157 §3.1 の「行番号またはソースオフセット」説は否定**（行番号が PC 加算量として消費されることはない）。
- 註: executor の arg7 は**文カウンタ [E1+0x28]**（0x5174DA で毎文 +1、エラー文 "Error Executing line (Offset %d)" の引数）であり、u32 とは別系統。

### 1.3 「上位 consumer」の不在（O-a の核心）

- 累積先 [E1+0x1C] は caller1 の**ローカル**。ループ毎に 0 へリセット（0x5174EF）され、0x51755E で消費された時点で役目終了。**caller1 の外へは一切渡らない**。
- caller1 の戻り値は `al = [esi+0xa1]`（VM フレームの「アクション実行フラグ」＝v157 §5.2 の argflag=256 制御ビット）のみ。
- 0x517810（RunScript ラッパー）は `mov bl,al → free frame → mov al,bl` で bool をそのまま返すのみ。**4 つの呼び出し元（0x4F9F20/0x4F9F50/0x4F9F82/0x4FBE6F）も al を伝播するだけで u32 系の値を受け取らない**。
- ⇒ **Begin u32 に上位 consumer は存在しない。v157 O-a は「consumer 不在」で解消。**

### 1.4 残る暧昧点（新規残点 V-172-r1 へ）

executor の Begin 経路は band1 ハンドラの double 結果で「加算（=本文スキップ）」するが、**arg9=1（FireEvent）では成立・不成立とも無条件に加算**される（v157 §12.3）。FireEvent モードでイベントハンドラ本文がどの経路で実行されるか（または FireEvent はゲート副作用のみで本文を実行しない設計か）は band1 ハンドラ群の全数確認が必要（§13 V-172-r1）。

---

## §5 【残点5】0x517810 RunScript ラッパーの 8 引数 — **解明**

### 5.1 ラッパー本体の構造（再入フレーム管理）

```
0x517810: ecx = this = 0x516970 の戻り = 静的シングルトン &[0xB36208]（atexit 0xA1C100 登録）
  [singleton] == 0          → malloc(0xA4) フレーム zero 化 → [singleton] = frame
  [frame+0x14] != 0（実行中） → malloc(0xA4) の第 2 フレームを作り esi に差替え（再入対応）
  [frame+0x14] == 0         → 既存フレームを流用（alloc なし、0x517911 パス）
  caller1(0x517330) を呼ぶ → フレーム free（第 2 フレーム経路のみ）→ al を返す
```

- `[frame+0x14]` は caller1 自身が `mov [esi+0x14],edi`（開始時）/ `mov [esi+0x14],ebx`（終了時 0x5175AE）で管理する「実行中スクリプト」＝**再入検出ゲート**。
- 0x516970 = **グローバル静的スクリプト実行コンテキストの getter**（`mov eax,0xb36208; ret`、初回に `or [0xb3620c],1` + atexit 登録）。

### 5.2 8 引数の意味論（caller1 への恒等写像で確定）

| 引数 | caller1 内の消費者 | 意味 |
|---|---|---|
| W1 | edi（arg1）: `[edi+8] bit3` チェック / `[edi+0x20]`=code size / `[edi+0x30]`= bytecode ptr（executor 0x5169F7） | **スクリプトオブジェクト**（bytecode を保持するフォーム系） |
| W2 | ebp（arg2）: `vt+0x170` GetBaseForm 呼出し（0x517446）/ ref フォールバック（0x51752A→executor arg3） | **ref（アクション参照）**。GetBaseForm 結果は [frame+4] に保存 |
| W3 | [esp+0x778]（arg3）: 0 なら `call 0x4fbdc0`（event list 工場）で生成、[frame+8] へ（0x51745B-0x517471） | **event list**（0 = 自動生成） |
| W4 | [esp+0x77C]（arg4）: `mov [esi],eax` = **[frame+0]** へ格納（0x517442） | **実行コンテキスト**（VM frame +0 フィールド） |
| W5 | [esp+0x780]（arg5）: `cmp byte [esp+0x780],0 / jne` で **end 時の 0x4FA0E0（event list 実行）呼び出しをゲート**（0x51757B） | **byte フラグ「event list を実行するか」** |
| W6 | [esp+0x784]（arg6）: `mov [ecx],dl` = event ノード byte0（0x51749D） | **byte パラメータ A** |
| W7 | [esp+0x788]（arg7）: `mov [ecx+1],dl` = event ノード byte1（0x5174AC） | **byte パラメータ B** |
| W8 | [esp+0x78C]（arg8, float）: `fld → fstp dword [ecx+4]` = event ノード +4（0x517493/0x5174BC） | **float パラメータ** |

- W6/W7/W8 は `[script+0x29] != 0` のときのみ event list +0x10 の 8 バイトノード `{byte, byte, float}` に書き込まれる（0x517474-0x5174BC）。
- 呼出し元の実引数（4 site 実測）: 0x4F9F00 系ヘルパ = `(this, a1, a2, 0, 0, 1, 0, 0.0f)` 系、0x4FBE00（イベント実行）= `(self, ref, ?, ?, ?, 0, 0, fldz)`。**float 引数（W8）は event ノードへ格納される値そのもの**であり、v157 O-e の「[esp+0x28] の float 引数」はこれにあたる。
- v157 O-e の「ref フォールバック [esp+0x2c]」は **W2（ref）が fetch の ref 出力と被ったときの既定 ref**（0x517522-0x51752E）＝ caller1 arg2 = W2 そのもの。

### 5.3 呼出し元 4 件の性格

| site | 関数 | 役割 |
|---|---|---|
| 0x4F9F20 | FUN_004F9F00（ret 8） | `(a1, a2)` → RunScript(this,a1,a2,0,0,**1**,0,0.0f) — W5=1 = event list 実行あり |
| 0x4F9F50 | FUN_004F9F30（ret 8） | 同形で W5=0/W6=1 — event ノード {1,0,0.0} 付き |
| 0x4F9F82 | FUN_004F9F60（ret 0xC） | `(a1,a2,a3f)` → float 引数を W8 に直接伝搬 |
| 0x4FBE6F | FUN_004FBE00（イベント実行系） | `mov bl,al` で結果を受け、後続の [esi+0x38]/[esi+0x3c] レート制御へ |

---

## §2 【残点3】0x4FAB20 ジャンプテーブルの全タグ対応表 — **解明（全 52 エントリ実測・ハンドラ全文解剖）**

### 2.1 2 段ディスパッチの完全な表（byte 表 0x4FAE4C → case 表 0x4FAE3C）

byte リマップ表（tag−0x47 インデックス 0x00..0x33 = tag 0x47..'z'）の**全 52 バイトを実測**:

| case | 該当タグ | ハンドラ（case 表 0x4FAE3C の実測） | 消費バイト | 意味（本調査で確定） |
|---|---|---|---|---|
| 0 | **'G'(0x47) / 'Z'(0x5A) / 'r'(0x72)** | 0x4FABC3 | 3（'r' は以降の type-char 分岐で合計 6） | 変数解決レコード |
| 1 | **'n'(0x6E)** | 0x4FACE9 `fild dword [eax+edi]; cursor+=4` | 5 | **i32 インライン定数**（fild で double 化） |
| 2 | **'z'(0x7A)** | 0x4FAD06 `mov [ecx],…; mov [ecx+4],…; cursor+=8` | 9 | **u64/double インライン定数** |
| 3 | その他すべて（'X'/'f'/'l'/'s' を含む全 41 タグ） | 0x4FAD2E | 個別（下表） | case3 内で bl により再分岐 |
| （範囲外） | tag > 'z'（0x34..） | 0x4FAD2E 直行（ja） | — | 不正タグ → fail |

case3 内の再分岐（0x4FAD2E 以降、**bl が最初のタグのまま**）:

| bl | 経路 | 消費 | 意味 |
|---|---|---|---|
| **'X'(0x58)** | 0x4FAD37: i16 読取（cursor+=4 に丸め）→ `call 0x4FCA30` で式オブジェクト取得 → `[esp+0x54]` モードで **FAE80 再帰（7 引数サブデコード、al=0 で即返）** または `call [expr+0x18]` 仮想評価 | 5 | **式（sub-expression）** |
| **'f'(0x66)/'l'(0x6C)/'s'(0x73)** | 0x4FADF3: i16 読取（+2）→ `call 0x4FA110(owner, id, scope)` → `fstp [out]`（double） | 3 | **リテラル/変数 pool 参照**（下記 2.3） |
| その他 | 0x4FADF3 → bl∉{f,l,s} → **0x4FADDE fail**（al=0） | — | 不正タグ |

### 2.2 case0（'G'/'Z'/'r'）の内部（0x4FABC3 全文）

1. i16 id 読取（+2）→ `call 0x4FA9C0(scope, id, ctx)` で**変数レコード解決**。
   - 0x4FA9C0: id ≤ `[scope+0x1c]`（変数数）検証 → **1-based の `[scope+0x40]` リンクリスト走査**（[node]=next/[node+4]=count 形）で id に一致するノード探索。**メモ化キャッシュ 3 変数**（[0xB361B0]=scope / [0xB09E1C]=id / [0xB361B4]=ctx / [0xB361B8]=record）で連続同 id を高速化。
   - `[rec+0xC]`（式？）と ctx があれば `0x4FA110` で事前解決し `[rec+8]` へ double 保存（0x46B250 経由）。
2. `[rec+8]==0` で `[rec+0xC]==0` → fail / `[rec+0xC]≠0` → **0.0 成功**（遅延解決の痕跡）。
3. タグ別読み出し: **'G'** → `fld [val+0x24]`（float → double 出力）／**'Z'** → `mov [out],[rec+0xC]`（**int dword のみ書出し**）／**'r'** → type-char を 1B 読取:
   - type=='X' → 式パス（上表）
   - type ∈ {'f','l','s'} → u16 読取 → 0x4FA110
   - type==';'(0x3B) → `[typeobj+0x58]` を値として直読
   - type ∈ {'1','2','3'}(0x31..0x33) → 追加リスト照合（0x41FB00）＋`vt+0x170` GetBaseForm → 0x4D8D70 → 0x484F50 で **ref 解決→値化**
   - その他 → 0x4D7250 経由または fail
   → 'r' レコード合計 6B（id 3 + type 1 + u16 2）で v157 §4 の emit 対応表と**全タグで一致**。

### 2.3 0x4FA110（'f'/'l'/'s' の解決先）— **変数 ID 解決器**（本調査で特定）

```
0x4FA110(owner(ecx), i16 id([esp+0xC]), scope([esp+0x10])):
  メモ: [0xB361BC]==owner && [0xB09E20]==id → [0xB361C0] を即返
  [owner+0xC] のリンクリスト走査（[node] が id と一致するノード探索）
  発見 → fld qword [node+8] を double 戻り値で返す（ret 8）
  未発見 → 0x4A7A60 で "Variable ID %08X not found. Try to recompile script '%s'."（0xA49A68）出力 → 0.0
```

- ⇒ **'f'/'l'/'s' は「リテラル」ではなく「オーナー辺りの変数 pool（id→double）参照」**。タグ文字は emit 側の型区別（float/int/string 変数参照）で、consume 側は同一解決器。**v157 O-c の「's' 文字列型の consume パス」は「id→double の変数解決」で完全解明**。
- 0x4FA9C0 側のエラー時の再コンパイル勧告文と合わせ、**この帯は「コンパイル済みスクリプトの変数参照を実行時に id 解決する」共通基盤**と裁定。

### 2.4 残る未解消点

'X'/'n'/'z' の emit 元（MessageBox 以外のどのコンパイル系か）と 0x4FCA30 の型語義は **V147-r1 のまま継続**（本タスクで emit 側には踏み込まず）。

---

## §3 【残点4】R1 evaluator の [esp+X] スクラッチの網羅確認 — **解明（v97 全 111 行で一般化確定）**

### 3.1 全数スキャン（本調査: v97_helper_rows.json 全 111 行 = R1 条件ヘルパ群の超集合）

`mov dword [esp+X], r32` → `fild dword [esp+X]` ペアの全数検出:

| 検査 | 結果 |
|---|---|
| **write→fild ペア** | **21 件**（GetLockLevel/GetGold/GetSleeping/GetTalkedToPC/GetAttacked/GetLevel/GetIsAlerted/CanPayCrimeGold/IsContinuingPackagePCNear/GetOpenState/GetDayOfWeek/GetPCFame/GetPCInfamy/GetBarterGold/GetDisabled/GetSitting/GetFurnitureMarkerID/GetInvestmentGold/IsInCombat/AdvancePCLevel/ShowSpellMaking） |
| **ペアなしの孤発 fild [esp+X]** | **0 件** ← 決定的 |
| int 系（fild 含有） | 27 / 111 |
| bool 系（fldz/fld1） | 64 / 111 |
| その他 | 12 / 111 |

### 3.2 本調査による規則の精緻化（v157 §7 への補正）

1. **スクラッチの disp は固定 0x1C ではない**。実測 disp = **{0x8, 0xC, 0x10, 0x14, 0x18, 0x1C, 0x20}**（GetGold/IsInCombat=0x1C は偶然の多発例）。不変条件は「**arg4 (&out) をレジスタへ引き上げた後、そのスタックスロットを int→f64 変換の一時領域として再利用**」（v147 §12.6 の read-after-hoist 補正を disp 一般化つきで支持）。
2. **孤発 fild [esp+X] = 0 件** ＝ このヘルパ群で「未初期化スタックの fild（＝引数スロットの暗黙読取）」は**存在しない**。「[esp+0x1C] 読み＝arg2/arg3 の再利用ではない」という v147 の裁定が**全数で強化**された。
3. float 戻り値ヘルパ・bool ヘルパにこのペアは**出現しない**（fild 含有は int 系のみ）→ v157 §7.2 の適用範囲限定が維持される。
4. v147 の「16/89」と本調査の「21/111」の差は**母集合の差**（v97 111 行は R1 89 件＋帯隣接 22 件の超集合）。規則自体の例外は 0。

**判定: O-d は「int 戻り値ヘルパのうちスタック temp を経由するものに限定された普遍的 MSVC パターン」で最終解明。**

---

## §4 【残点2】+0x24 列（argflag=1）デッドカラムの理由 — **解明**

### 4.1 オペコード表は PE 静的データ（本調査で確定）

| 検査 | 結果 |
|---|---|
| 表領域 0xB0AF48..0xB10290（533 行 ×0x28）の raw バック | **あり**（file_off 0x709B48.. — .data の初期化領域） |
| 全セクションの imm32 参照（非整列含む） | **192 件**（.text 177 / .data 12 / .rdata 3） |
| .text 177 件の命令分類 | **全件が読み**（`mov/cmp [imm32], …` のソース側、ループ境界 `cmp eax,0xB0B424`（0x4FF846 の name 検索ループ）等）。**書手 0 件**（初出の「STORE 3 件」は cmp 2 件＋1 件が非整列デコード偽陽性で、実体は `cmp eax,imm` = 読み） |
| .data 12 件 | 表を指す静的ポインタ（条件ディスパッチ配列等、いずれも読み） |
| +0x24 列アドレス（0xB0AF6C+i×0x28, 533 件）の直接参照 | v147 §12.5 済: **0 件**（disp8 相対形含め全走査） |

⇒ **表は実行中に構築・更新されない静的バイナリデータ**。コンパイラ（FUN_004FF710 帯）は表を**読むだけ**（名前検索）で、+0x24 列への書手は存在しない。

### 4.2 裁定: 「予約」ではなく「ツールチェーン用区分フラグの残置」

- 行構造は `{+0 name, +4, +8, +0xC, +0x10 flagA, +0x14 argdesc, +0x18 impl, +0x1C, +0x20 condition, +0x24/+0x25 = u16 argflag, +0x26/+0x27 パディング}`。**+0x24/+0x25 は 1 つの u16 フラグ語**（値 1 = bit0 / 256 = bit8）。
- 実行時は **bit8（+0x25 byte、executor 0x517295 のみ）だけが消費**され、bit0（+0x24）はどの実行時コードも読まない（v157/v147 の全数走査＋本調査の表領域全体走査で三重確認）。
- argflag=1 の 9 件は condition getter 群（GetIsClass 等）＝ b03 の「参照必須 getter」区分と一致。この bit0 は**エディタ/コンパイラ検証用の区分属性**としてビルド時に静的に焼き込まれたもので、リテール実行コードは一切参照しない。
- ⇒ **「将来の拡張予約」ではなく「ツールチェーン側の属性が構造体に残ったまま実行時未使用」＝デッドカラム**。移植では無視してよい（v157 §12.10 と同一結論のうえ、理由が確定）。

---

## §6 【残点6】emit wrapper の所有者 [owner+8] と入れ子実証（V-158a） — **解明（構造実証つき）**

### 6.1 パック書手 9 箇所の一般形（本調査で全文採取）

```
X    = owner のポインタフィールド（系統別: alarm 系は [alarm+8] / 0x5354 系は [this+0x1A0] /
       0x695D 系は [process+0x364] / 0x89F5 系は arg1）
P    = [X+8]（test → add P,0x14）
[P+0x1C] = (serial<<16) | kind     ; kind = 0xF / 4 / 7（系統別）
その後: vcall X+0x80（変化通知）
```

### 6.2 wrapper（param_2）の同定と入れ子の実証

1. **reader 2 系が同一構造を読む**: 0x60d950 は `ebx=param_2 → [ebx+0x10] 相対 ptr / [ebx+0x18] mode(1|2) / [ebx+0x1C] パック`、0x4806E0 も `[w+0x10]`（`add eax,ecx` 相対解決）＋`[w+0x18]` mode を読む → **wrapper = {+0x10 rel-ptr, +0x18 mode byte, +0x1C pack} の非仮想 POD**（v158 §3-1 の同定を維持）。
2. **param_2 は格納値**: 2 番目の caller FUN_0060DBB0 は `mov ecx,[ecx+0x28]; push ecx` — **param_2 = record+0x28 フィールド**（0x60C5B0 系は record+0x20）。計算結果ではなく、各レコードが wrapper ポインタを保持する。
3. **入れ子構造の直接証拠（新）**: 0x99949C `lea eax,[ecx+0x14]; mov [ecx+8],eax; mov [ecx+0x18],2; …` — **`[X+8] = X+0x14` の自己埋め込み構築**が実在する。書手の `P=[X+8]; P+=0x14; [P+0x1C]=pack` と整合する入れ子レイアウト（X 内に +0x14 オフセットから始まるサブオブジェクト、その +0x10/+0x18/+0x1C = rel-ptr/mode/pack）を**構築コードごと**発見。
4. alarm 系（0x6095xx）では [alarm+8]、process 系では [proc+0x364]（0x6098C0 が `__RTDynamicCast` 後に [eax+0x364] から serial を shr 0x10 で読む実測）と、**owner のフィールド位置は系統別だが、X→X+8→+0x14→+0x1C の 4 段は全 9 site で同一**。

### 6.3 alarm 帯の実体（owner の文脈）

- FUN_006094C0/0x6095B0/0x6098C0 は **0x60BEF2 のジャンプテーブル（type 0..4）で選ばれる 5 つのアラーム処理系**（0x608FC0/0x609150/0x6094C0/0x6095B0/0x6098C0）。`[alarm+0x5c]` = ペイロード。
- 0x6094C0（kind=0xF）: 位置ベクトル 3 dword をコピー → 0x4D8A30（移動系）→ 旧 pack へ `and 0xffffbfcf / or 0xf`（kind ビット更新、serial 保持）→ vcall +0x80 → キャッシュ付き serial 採番 → `(serial<<16)|0xF` 新規書込み → vcall +0x80。
- ⇒ owner は「**アラーム/イベント系オブジェクト（alarm ノード・Actor 系 process の +0x364 ノード等）**」で、X+8 が指す **+0x14 埋め込み wrapper が param_2 本体**。**V-158a は実証をもって解消**（[owner+8] の「クラス名」までとはいかないが、構造・生成経路・文脈すべて確定）。

---

## §7 【残点7】[0xB3B7D4] キャッシュの無クリアと dedup 実効性（V-158b） — **解明**

### 7.1 クリア書手の不存在（再走査で確定）

| 検査 | 結果 |
|---|---|
| [0xB3B7D4] の abs32 参照（全 3 セクション） | **8 件すべて .text**: 読 4（0x607B61 / 0x609557 / 0x609627 / 0x609945）＋ 書 4（0x607B87 / 0x60957D / 0x609650 / 0x60996B）。**書 4 件はすべて「キャッシュ 0 のときだけ」採番値を書く非 0 書き**（0 クリアなし） |
| **register 経由書手**（`mov reg, 0xB3B7D4` → `[reg]` 書き） | **0 件**（アドレスをレジスタへ積む instruction 全在無し。±8 近傍 0xB3B7C8/CC/D8 も同様に 0 件、唯一の近傍参照 0xA254A0 は別変数 0xB3B7C8 の atexit 登録） |
| base+off 間接（`[0xB3B7D0]+4` 等） | 走査済み・該当なし |

⇒ **[0xB3B7D4] はプロセス生存期間にわたって一度もクリアされない**。bss 初期値 0 → 初回採番後は恒 nonzero。

### 7.2 dedup の実効性（serial 固定の帰結）

- serial 供給系は 2 系統とも**キャッシュ優先**: 0x607B60（関数版）と 0x609556/0x609626/0x609944（**インライン展開クローン 3 系統**、いずれも `test [0xB3B7D4]; jne → 採番` の同一パターン）。非メモ化インクリメント系統（0x531D80 等）が [0xB2EB3C] を直接 +1 しても、**キャッシュは古い値を保持し続け**、全 stamp 系統が同一 serial を見ることに変わる。
- 0x60d950 の dedup: `[wrapper+0x1C]>>16 == serial` なら**早期 return（再 stamp・再 emit を skip）**。serial が固定である以上、**「各 wrapper は生涯 1 回だけ stamp される」**という degenerate な単一 epoch dedup として機能（fresh wrapper は +0x1C=0 → 0≠serial → 通過 → stamp）。
- wrap（0xFFFF→0x0A）経路はキャッシュ固定により**実行不能**（下限 0x0A の設計保証は理論上のもの）。
- ⇒ **V-158b: 無クリアを確定。dedup は「2 重 stamp 防止」として実効するが、epoch はプロセス寿命に固定**（v158 の「同一処理ブロック内固定」は「プロセス全体固定」へ強化・訂正）。

---

## §8 【残点8】type0x3D float extra の多用途性（V-158c） — **解明（v158 帰属を訂正）**

### 8.1 type0x3D 読手の真の消費者（0x41FC90 = `push 0x3d; call 0x41E210; fld [node+0xC]`）

`call 0x41FC90` は **5 件のみ**: 0x4F8C55（IsHorseStolen 帯）/ 0x5DAB55 / 0x5EADF6 / 0x60FBC7 / 0x60FD79。writer は 0x4269E0（`push 0x3d; 0x41E210; 0x42A290` で float 付きノード生成 = SetCrimeGold 系）。

### 8.2 4 opcode の実際の経路（全文実測）

| opcode | impl | 経路 | extra 使用 |
|---|---|---|---|
| IsCellOwner 0x1118 | 0x505DD0 | FAE80 → ref フォールバック（[0xB333C4] vt+0x170）→ **0x4F8B90** | **なし**。0x4F8B90 = `0x4CA970`（owner-ID 取得）→ `cmp eax,期待値` → 一致で 1.0 = **owner 比較述語**（唯一 caller = IsCellOwner 0x505E4C）。0x3D 読手を含まない |
| IsHorseStolen 0x111A | 0x505EE0 | **0x4F8C20**: gate vcall+0x190 → 0x5E3290（馬判定）→ `[horse+0x44]` extra list → **0x41FC90（type0x3D float）** → fcomp [0xA2FAA8] しきい値 → 1.0 | **あり（type0x3D を条件比較で使用）** |
| IsSneaking 0x111E | 0x505F20 | **0x4F8CF0**: gate → **0x5E0550**（sneak 状態判定）→ 1.0 | **なし** |
| IsRunning 0x111F | 0x505F40 | **0x4F8DB0**: gate → `[actor+0x58]` process → vcall +0x2C0 → **`test ax,0x200`**（実行中ビット）→ 1.0 | **なし** |

### 8.3 裁定

- **type0x3D = crime gold 系 float extra**（SetCrimeGold が書き、GetCrimeGold/CanPayCrimeGold 等の evaluator 帯がメーターとして読む）。条件系での type0x3D 使用は **IsHorseStolen のみ**（馬の「盗難値」）。
- v158 §3-8 の「IsCellOwner/IsHorseStolen/IsSneaking/IsRunning が共有 0x4F8B90 経由で 0x3D に到達」は **誤り**（0x4F8B90 の caller は IsCellOwner の 1 件のみ、0x3D 読手でもない）。誤帰属の原因は §10 の台帳/relay ラベル精度問題。

---

## §9 【残点9】参照 Update 実装の内部順序（V-158d） — **解明（「script→AI 直列」を否定）**

### 9.1 0x4E3F30（Actor/REFR +0x160、vtable 実 bytes 確認: REFR 0xA46C44 / Actor 0xA6E074 とも [+0x160]=0x4E3F30）

本体は 0x4E3F30..0x4E407F。構造:

```
[ebx+8] bit5 / bit11(disabled) ゲート → base formType {0x1A,0x23,0x24} 判定
→ vcall +0xFC → vcall +0x154 → base form の参照配列（+0xB0/+0xB6）
→ 0x560920(0xB3CAC0, key) contains チェック
→ 各要素へ: 0x49F950 → 0x4E0D90 → 0x6C9CB0 → 0x4D90D0（ループ）
```

- **スクリプト実行系（0x5169A0/0x517810/0x4FBE00/0x441670）の直接呼出し = 0**。
- 0x49F950 の E8 先 12 種は **Render/NI 帯（0x65DC50/0x6AA980/0x6AC420/0x6AE0A0/0x6B71xx/0x6B73xx）＋ 0x447490** で、エフェクト/描画ノード連動の減衰処理。
- Player 0x65FF40（0x65FF40..0x6600A3、vtable 0xA73A0C 実 bytes 確認）: 0x476D10（アニメキュー）×2 → vcall +0xFC → process(+0x58) vcall +0xF0 → extra 系（0x41E880/0x41EDF0/0x41DF10/0x41F6A0/0x41F200）→ 0x65AC50（[0xB33398] 経由 0x6AE0A0 = 描画連動）→ 0x6B73E0（[0xB33A98] TES global → 0x57B050/0x57AF10/0x57B0F0）。**script 直呼びなし**。

### 9.2 裁定

**参照 Update（vtable+0x160）の内部に「スクリプト実行 → AI tick」の直列順序は存在しない。** 0x4CCC50 はセルの参照リストを走査し、各参照で gate(+0x190) → Update(+0x160) を**逐次**呼び出すだけ。スクリプト実行は event/opcode ディスパッチ層（0x5169A0 系）、AI は process 帯（HighProcess +0x160=0x629810 等）に分離されている。v158 の「同一 vcall 内に還元」は正しく、「+0x160 実装側の深い階層に script あり」は**誤り**だった点を訂正。

---

## §10 【残点10】expression getter 帯の台帳結合粒度（V-158e） — **裁定**

- 0x4F5200..0x4F6100 を int3 境界で走査 → **34 個の独立関数**（0x4F5200/5290/5310/…/6060）。
- 同レンジの func_list 台帳エントリは **5 件のみ**（FUN_004F54A0 / FUN_004F5650 / **FUN_004F5730** / FUN_004F5EA0 / FUN_004F6060）。最大で **1 台帳エントリ = 15 実関数**（FUN_004F5730 が 0x4F5730..0x4F5E9F を包括、b10 の expression getter 帯全体）。
- ⇒ **台帳結合は粗い（最大 15:1）が、v158_map.csv の relay2 行は call target の実アドレス（target_va）で記録されており機械データは精密**。粗いのは「共有 evaluator 0x4F5230 経由」等の**人間可読ラベル**のみ。
- **実例**: V-158c の IsSneaking/IsRunning → type0x3D 帰属は、この帯の relay2 が evaluator 帯の共有経路を 1 つのラベルに束ねたことによる誤帰属（§8 で訂正）。**将来の relay2 行は target_va 単位で帰属せよ**。

---

## §11 【残点11】直接ストア +0x58 5 件のクラス帰属（V-158f） — **解明**

CSV の `direct_store` 5 行（TestLocalMap 0x50EF90×2 / ShowPivot 0x511C10×2 / SetPos 0x508FC0×1）をベースレジスタ追跡:

| site | ベースの出所 | フィールド文脈 |
|---|---|---|
| 0x5090E5（SetPos） | vcall +0x154 の戻りを esi に再代入 → `mov [esi+0x58],ebx` | 位置更新 |
| 0x50F860 / 0x50FC4E（TestLocalMap） | `[esi+0x54]` と対の float 算術（fsub/fstp）連続書出し | vec4 帯 +0x54/+0x58 |
| 0x511E18 / 0x511F65（ShowPivot） | `+0x4C..+0x58` へ連続 float 書出し（0x511F65 は vcall +0x174（位置 getter）の結果 [eax..+8] を +0x54/+0x58 へ転記） | 位置ベクトル転記 |

⇒ **+0x58 = 位置ベクトル帯（+0x4C..+0x58 の連続 float フィールド）を持つ Ni/Transform 系オブジェクトの第 3 成分**。TESObjectREFR（sizeof 0x58 で +0x58 は範囲外）でも CC latch でもない。**3 系統混在の第 3 系統として `class unverified` を解消**（v158 の「Actor+0x58=process との混在」仮説は本 5 件には非該当）。

---

## §12 【残点12】0x977C50 共有と slot+0x190 の真意（V-158g） — **解明**

### 12.1 0x4CCC50 のゲート使用形（全文実測）

```
0x4CCC79  gate = vcall [esi]+0x190
0x4CCC85  test al,al / je 0x4CCC9E            ; gate==0 → 無条件に +0x160 を呼ぶ
0x4CCC89  cmp byte [esi+0x78],0 / jne 0x4CCC9E ; [esi+0x78]!=0 → 同上
0x4CCC8F  0x45A500([0xB33B00]) / test / je 0x4CCCAA ; 述語偽 → +0x160 を SKIP
0x4CCC9E  vcall [esi]+0x160（Update 本体）
```

⇒ **スキップ条件 = gate==1 && [esi+0x78]==0 && !0x45A500()**。slot+0x190 は「Update を保留/スキップしてよいか」の判定材料の一つ（bool 戻り）。

### 12.2 クラス別実装（vtable 実 bytes で検証）

| クラス | +0x190 | 本体 | 意味 |
|---|---|---|---|
| Actor/Character/Creature/Player | **0x977C50** | `mov al,1; ret` | 常真 → skip は残り 2 条件に依存 |
| TESObjectREFR（素） | **0x69D990** | `xor al,al; ret` | 常偽 → **無条件 Update** |
| HighProcess | 0x634910 | 大きな本体（+0x118 ハンドル操作） | 同 offset は別メソッド（slot 非互換） |
| LowProcess | 0x60D880 | `ret 4`（引数あり） | 同上 |

### 12.3 0x977C50 を共有する 7 opcode（全数）

TestCode(0x141)/BeginTrace(0x173)/ShowNameMenu(0x10DC)/TrapUpdate(0x110C)/AddAchievement(0x114A)/StartMasterFileSeekData(0x115F)/DumpMasterFileSeekData(0x1160) — **全てリテール no-op の debug/ユーティリティ系**（impl が `return true` のみ）。

**裁定**: 共有は「常真 no-op スタブ」の同一コード再利用で**意図的**。slot+0x190 の真意は「Update スキップゲート（3 条件の 1 つ）」であり、opcode impl としての共有は無害な節約。**留保**: slot+0x190 はクラス系統ごとに異なるメソッドが同一 offset に並ぶ（Process 系）ため、「slot+0x190 = ゲート」は REFR/Actor 家族に限定された解釈。

---

## §13 新規残点（V172 起票粒度）

| ID | 内容 | 手掛かり |
|---|---|---|
| **V-172-r1** | FireEvent（arg9=1）モードで Begin ブロック本文が実行される経路。executor は成立・不成立とも本文をスキップ（u32 加算）するのに、band1 ハンドラ（OnActivate 0x503F90 等）にも本文実行が無い — handler 本文の実行主体が未特定 | §1.4、v157 §12.3 |
| **V-172-r2** | 'X'/'n'/'z' タグの emit 元（MessageBox 以外のコンパイル系）と 0x4FCA30 の型語義（V147-r1 継承） | §2.4 |
| **V-172-r3** | 0x45A500([0xB33B00]) 述語の語義（Update skip 条件の第 3 項、alarm 帯 0x60967E でも使用） | §12.1 |
| **V-172-r4** | 0x4F8C20 が読む type0x3D 値の実データ意味（SetCrimeGold 系 writer が書く float と IsHorseStolen が比較する [0xA2FAA8] しきい値の関係 — crime gold と馬盗難が同一 extra を共有する理由） | §8.2 |
| **V-172-r5** | Process 系 vtable の slot+0x190（0x634910/0x60D880）がゲートと異なるシグネチャを持つ理由（クラスレイアウト差か slot 割当ずれか） | §12.2 |

## §14 自己検証

```
$ python "%TEMP%\opencode\probe_harness.py" selftest
   bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl
   => ALL PASS（4/4、着手時）
```

| # | 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|---|
| 1 | Begin u32 の消費 | PC 加算 | caller1 0x51755E `add eax,[esp+0x1c]`、arg4=&[E1+0x1C]、毎イテレーション 0 リセット | OK |
| 2 | RunScript 8 引数 | 恒等写像 | 0x5178CD-0x5178FA の push 並びを k 追跡で W1..W8 = caller1 arg1..8 を確認 | OK |
| 3 | 表領域の書手 | 0 | 192 imm32 参照を全分類（読み 192 / 書 0、初出の STORE 3 件は cmp 2 + 非整列偽陽性 1） | OK |
| 4 | +0x24 列直接参照 | 0 | v147 全数走査（本タスクでは表領域全体で再確認） | OK |
| 5 | FAB20 byte/case 表 | 全 52 エントリ | 0x4FAE4C 全 52 バイト＋case 表 0..3 実測、有効タグ 9 種 | OK |
| 6 | R1 fild スクラッチ | 孤発 0 | v97 111 行で fild [esp+X] 孤発 **0 件**／ペア 21 件 | OK |
| 7 | [0xB3B7D4] 参照 | 8 件（読4/書4） | 8/8 一致、reg 経由ロード 0 件 | OK |
| 8 | 0x41FC90 callers | 5 件 | 0x4F8C55/0x5DAB55/0x5EADF6/0x60FBC7/0x60FD79 | OK |
| 9 | 0x4F8B90 callers | 1 件（IsCellOwner） | 0x505E4C の 1 件のみ | OK |
| 10 | vtable +0x160/+0x190 実 bytes | 0x4E3F30/0x69D990・0x977C50 | REFR 0xA46C44・Actor 0xA6E074 で一致 | OK |
| 11 | 0x977C50 共有 opcode | 7 件 | 7/7（v158 の「7 個」を全数確認） | OK |
| 12 | +0x58 direct_store 行 | 5 件 | 5/5（TestLocalMap×2/ShowPivot×2/SetPos×1） | OK |
| 13 | 帯関数数 vs 台帳 | 34 vs 5 | int3 分離 34 / 台帳 5（最大 15:1） | OK |
| 14 | 0x4E3F30 の script 帯直接呼出し | 0 | 0（0x49F950 の script 帯は 0x447490 のみ、dispatcher なし） | OK |

## §15 使用証拠ファイル

- 読み取り: `tmp/re/v157_runtime_residuals.md` / `tmp/re/v158_slot_opcode_map.md` / `tmp/re/cache/{opcode_table.json, vtable_index.tsv, named_funcs.txt, call_targets.json}` / `%TEMP%\opencode\{probe_harness.py, v97_helper_rows.json, v158_map.csv, func_list_new.txt}` / capstone 5.0.7 逆アスク（docs の decompiled は今回は不使用）
- 本タスク生成（%TEMP%\opencode のみ）: `v172_common.py` / `v172_p1..p10.py` / `v172_p1..p10_out.txt`。**repo 内新規ファイルは本レポート 1 件のみ**。
- 未使用（禁止事項遵守）: ESM（再スキャン禁止）、`docs/`・`TODO_re49.md`・`session-memory.json`・`plan.md`・`func_list_new.txt`・`cache/`（いずれも編集なし）、git 操作なし。
