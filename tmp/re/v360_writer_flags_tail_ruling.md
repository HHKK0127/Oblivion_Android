# V360: save ライタの flags ゲート不在と entry tail ワードの同定 — #139（V-356-1 / V-356-2 / V-356-4）

- 日付: 2026-11-13 / タスク: ai-todo **#139**【v356 残点束】①探索失敗 entry の最終ファイル残存（ライタ flags ゲート照合）②45BB00 の7件目 caller 0x46667E の可変長読取のデータ意味 ③`[entry+4]` ワードの正式名
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: `v356_pass2_tail0_loadside_ruling.md` 残点 V-356-1/V-356-2/V-356-4（V-356-3 は #82 依存で本束対象外）
- 手法: capstone 5.0.7（v307_common.py）／関数頭起点整列デコード＋`disasm_range`（無効バイト再同期付き・後半スキャンの欠落を自己発見して是正）／E8 rel32 逆算／スタックアドレス（esp disp）の生産者追跡
- 中間証跡: `%TEMP%\opencode\v360_1..v360_9.py/.out`（**全 EXIT=0**。v360_6/7 のスキャンは無効バイトで early-stop＝0件誤検出を v360_8/9 で是正）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM/.ess 未アクセス、EXE 読み取り専用、L1

---

## §0 判定サマリ

| 設問 | 判定 |
|---|---|
| **① V-356-1** ライタの flags 系ゲート（書出/スキップ分岐） | **[確定(静的)] ライタは entry[+0] flags で書出/スキップを分岐しない**。entry ループ（fn 0x465130・0x465303..0x46570C）に entry flags テストは不在。flags は **`0x4535A0`(CheckFlags) と form の vfunc+0x4C/+0x50 へ「内容生成パラメータ」として渡される**（＝flags は書出バイト列に影響するが、entry の生死・書出可否には影響しない）。付帯ゲート: entry null でループ退出／**bit9（[ebp+0x18]>>9）=dry カウントモード**（実 write をスキップし `[ebp+0x90]` 加算＝v316 dry と整合）／`cmp edi,[0xB333C4]` Player 特例／entry[+4] の有無で route A/B。→ **「fail entry は flags で捨てられず、存在すれば書出（dry ならカウント）」[確定]**。動的残存の最終確認のみ #82（V-356-1 維持・静的側はクローズ） |
| **② V-356-2** 0x46667E 可変長読取のデータ意味 | **[部分確定] load（LoadGame）経路 [確定]**：head 0x465860＝LoadGame 本体（v323 item6 の caller 0x465B12 と同一関数）。処理系＝`0x452C90` formID リマップ → **size=u16 local [esp+0x26] で `0x453500` バッファ確保** → **`0x45BB00` で size 分を読取** → `0x460BC0`（連携/直後 processing）→ vfunc+0x54 呼出 → `new(0x10)` の **node{form 参照, [esp+0x21], edi, cl} を `0x5A6AB0` へ insert**（**record==PlayerCharacter[0xB333C4] の場合は insert せずスタック退避**）。**u16 の出所＝12B レコードヘッダ**（`0x465C94` の n=0xC read が dst=frame+0x20 へ書出し、`mov edi,[esp+0x34]`＝header+0＝formID で実証）の **header+6 u16 ＝ペイロード長 [部分確定]**。残:ヘッダ +4/+5 field の意味（V-360-1） |
| **③ V-356-4** `[entry+4]` ワードの正式名 | **[確定(命令)] `entry[+4] = 台帳 entry の tail フレーム先頭ポインタ（0 == フレームなし）`**。三方向裏付け: pass2 の tail==0 分岐（0==）＋再発行 src＝`entry[+4]+4+Δ`＝frame+4+Δ（v328 P11 の lea 実測と整合）／writer route A は `[ebp+0x14]:=entry[+4]` 後に **`mov edx,[ecx]` で実 deref**し4Bヘッダ消費→cursor+4（frame 内歩行）／両 route 終端で field=0 クリア |

---

## §1 ① ライタの flags ゲート不在 [確定(静的)]

### 1.1 ライタ関数とその this

- **entry 書出ループの関数頭＝`0x465130`**（find_head・SEH prologue・冒頭 `mov ebp,ecx`／`[ebp+0x18]>>9` bit9 テスト／関数名比較用文字列 `0xA3AAD4` を strncmp(9)）。
- **E8 callers 6件**: **`0x5D3246`/`0x5D3438` は `mov ecx,[0xB33B00]` で直接 this にグローバルを渡す**（＝this=SaveGame/stream オブジェクトの直接証明）、`0x466AF6`/`0x466BD0` は名前文字列付きラッパ（`0xA3B144`/`0xA3AAD4`）、`0x466D21`/`0x5024BA` はレジスタ経由。
- → **`[0xB33B00]` = 現行 SaveGame/stream オブジェクトのグローバルポインタ** [確定]（route A の store→deref 連鎖はこの同一性で成立）。

### 1.2 entry ループ（0x465303..0x46570C）のゲート全列挙

```asm
46531F: call 0x452600                 ; entry iterator（pass2 と同一）
465324/465336: entry/count null → 0x46570C（ループ退出）
46533C: edx = [entry+4]  → [ebp+0x14] へ転送
46533F: test edx,edx → 0x465472 = route B（tail なし）／非0 = route A
465344: eax = [entry+0]（flags）→ [esp+0x3d] へ保存
        ※ entry flags によるスキップ分岐は存在しない
46570C: cmp [esp+0x2c],0; jne 0x465303（ループ継続）
```

- **flags の行き先（0x465485/0x4654E4/0x46557A）**: `0x4535A0`(form, flags)=**CheckFlags**（v317 正典: bit3 再合成）／form **vfunc+0x4C**(flags)（戻り ax=サイズ）／form **vfunc+0x50**(flags)。→ **flags は書出「内容」を生成する入力**であり、書出の有無を決める条件ではない [確定(命令)]。
- **付帯ゲート**: **bit9 dry**（`[ebp+0x18]>>9` 立なら `add [ebp+0x90],n`（サイズカウンタ）のみで実 write を飛ばす・route A/B 双方に出現＝v316 の dry bit9 消費文脈 [確定]）／route B の `cmp edi,[0xB333C4]` Player 特例（0x465616）／route B の整合検（`base+size != [ebp+0x14]` ならログ `0xA3B088`）。
- → **V-356-1 静的側クローズ**: 探索失敗 entry（pass2 で生存）は flags で捨てられず、**存在すれば書出（bit9 dry ならカウント加算のみ）** [確定]。動的実測は #82 維持。

---

## §2 ② 0x46667E＝load 系 record 後処理 [部分確定]（v360_2/v360_9）

### 2.1 パス（LoadGame 関数 head 0x465860 内）

```
0x466650: [共通接続] eax=[esp+0x21](id), ecx=[ebx+0xC](formID)
           push ×2 → 0x452C90(ecx=[ebp+4])        ; ★ formID byte3 リマップ→解決（v328 P1 の reader 系と同一）
0x466661: edx = word [esp+0x26]（★size u16）
           push edx; ecx=ebp → call 0x453500        ; buf = alloc(size)
0x466674: eax = word [esp+0x26]; push eax; push esi(buf); push [esp+0x38]
0x46667E: call 0x45BB00 (ecx=ebp=stream, dst=buf, size=u16)  ; ★ 7件目 caller＝u16 可変読取
0x46668F: [ebp+0x80] = &local → call 0x460BC0      ; 連携処理（v328 文脈 fn）
0x46669A: vfunc+0x54(ebx=record, ...) 呼出
0x4666A9: new(0x10) → node { [0]=ebx, [4]=[esp+0x21], [8]=edi, [0xC]=cl }
0x4666D8: ebx == [0xB333C4]（PlayerCharacter）? → 0x5A6AB0 insert / 逆ならスタック側退避
0x4666FB: size と cursor の整合検（不一致時 → ログ 0xA3AFA0 系）
0x466720: [ebp+0x44] = 0x60000000
```

- **経路の性質 [確定]**: LoadGame 関数内＝**読取（load）経路**、`0x452C90` は formID リマップ（v328 P1 の reader 系）、node 生成は record への解決結果登録。
- **size u16 の出所 [部分確定]**: 関数内に `[esp+0x26]` への**直接書込・lea out-param は0件**（disasm_range 再同期付き全域走査で確認）。生産者は **`0x465C94` の12B レコードヘッダ read（n=0xC）**: 戻り直後 `mov edi,[esp+0x34]`＝**header+0＝formID**（`cmp edi,0xfeffffff` の特別 formID 判定と整合）→ 仮定 dst=frame+0x20 なら **`[esp+0x26]` = header+6 の u16**、その用途（alloc/read/整合検の3点すべて＝**ペイロード長**）と整合。→ **「header+6 = レコードのペイロード長」[部分確定]**。残: header +4/+5 field の意味と lea の厳密な対応（V-360-1）。

---

## §3 ③ entry[+4]＝tail フレーム先頭ポインタ [確定(命令)]

### 3.1 三方向の裏付け

1. **pass2（0x461A7B-0x461A96, v360_3）**: `eax=[entry+4]` → `cmp 0; je tail==0(0x461BCC)`、非0なら `mov [ebx+0x14],eax` で **SaveGame/stream オブジェクトの +0x14 へ転送**（ebx＝SaveGame：`[ebx+0x74]` 配列は v328 P11 の [SaveGame+0x74] と一致）。
2. **再発行（v328 P11 実測と整合）**: 再発行 src＝`lea eax,[edi+esi+4]`（esi=`[entry+4]`）＝**frame 先頭+4+Δ**＝tail フレームのペイロード（4B ヘッダ後）から Δ を除去した位置。**ポインタ演算が成立 → entry[+4] はポインタ [確定]**（長さなら不成立）。
3. **writer route A（0x46533C-0x465369, v360_4/5）**: `[ebp+0x14] := entry[+4]` → `mov eax,[0xB33B00]`（＝同一 object）→ `mov ecx,[eax+0x14]` → **`mov edx,[ecx]` で実 deref** → 4B ヘッダ（u16 tailSize/char/typeID）消費 → `add [eax+0x14],4`（frame 内歩行）→ byte2/byte3 をヘッダから展開（v328 P2 の分散書出と完全一致）。

### 3.2 writer の route A/B 全像（v360_5 補強）

- **route A（entry[+4]!=0）**: 旧 frame を deref してヘッダ分解 → u16 tailSize（`test dx,dx` 0 なら本体 skip）→ 本体 payload を vtable 系呼び出し（`[ebp+0x14]` を引数で渡却）で書出 → 終端 **`[ebp+0x14]=0`**（0x465466）。
- **route B（entry[+4]==0, 0x465472）**: `0x46B250` GetForm → **`0x4535A0`(CheckFlags)** → formType byte → form vfunc+0x4C → `0x452250` でペイロード長算出 → **`new[](size)` → `[ebp+0x14]=buf`** → **`0x460710` シリアライズ** → `base+size==[ebp+0x14]` 整合検（不一致時ログ `0xA3B088`）→ **`0x45C4A0` で write** → `delete[]` → 終端 **`[ebp+0x14]=0`**（0x4655BE）。
- → **`SaveGame+0x14` = 「現 entry の tail フレーム先頭（scratch/cursor）」フィールド** [確定]。pass2 の失効時クリア（0x461BB3/0x461BC3/0x461DB1）と writer の終端クリアが同義（**tail 失効/処理完了のクリア**）→ **v356 §2.2 の該当箇所の「解決済みインデックス」説を本裁定で更新**（クリア機構・生存判断の結論自体は変わらない）。

### 3.3 正式名

- **`entry[+4]` = "tail frame pointer"（tail フレーム先頭へのポインタ・null==フレームなし）** を機能名として確定 [確定(命令)]。C++ フィールドの公式名はシンボル不在で [未確定]（v328 P2/P11 の "tail" 語彙と整合）。

---

## §4 既知の補強（再利用事実）

1. **`[0xB33B00]` = 現行 SaveGame/stream オブジェクト**（writer this の直接証明2件）・**`+0x14` = 現 entry の tail frame ptr scratch**・`+0x18` bit9 = dry・`+0x90` = dry サイズカウンタ・`+0x44/+0x80` = 処理中状態。
2. **writer（fn 0x465130）の section-name 進行**: 引数文字列 `0xA3AAD4`（9B strncmp）比較＋caller 側名前 `0xA3B144` 等＝セクション単位の書き出しラッパ群。
3. **dry bit9 の実装形**: `shr 9; test` → カウンタ加算 vs vtable write の二岐（v316 の dry 定義を命令で確認）。
4. **12B レコードヘッダ（n=0xC @0x465C94）**: dst=frame+0x20・formID=header+0（[esp+0x34]）・+6=u16 size [部分確定]。

---

## §5 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v360_1..v360_9 **全 EXIT=0**。**v360_6/7 のスキャンは無効バイトで early-stop し0件を誤出力** → v360_8 で `disasm_range`（再同期付き）に作り直して是正（欠落自己検出の事例として記録） |
| 2 | flags 不在の網羅性 | entry ループ全域（0x465303..0x46570C）を整列デコードし、flags 保存先 [esp+0x3d] の全参照を追跡 → CheckFlags/vfunc への**渡却のみ・分岐なし** |
| 3 | entry[+4] 三方向 | pass2（分岐＋転送）／再発行 lea（実測整合）／writer（store→deref→cursor+4）の**3系統独立確認** |
| 4 | size 源の否定的検証 | 関数全域（再同期付き）で `[esp+0x26]` アクセス＝**読取2件のみ**（書込/lea 0件）→ ヘッダ read 経由と推定、推定根拠（formID=header+0 の実証）を明示 |
| 5 | v356 との整合 | v356 §2.2 のクリア3箇所は本裁定で「tail ptr クリア」と同定 → **結論（クリア＝削除トリガでない・生存判断不変）は維持、フィールド同一性のみ更新** |

---

## §6 新規残点

| ID | 内容 | 深刻度 |
|---|---|---|
| V-360-1 | [部分確定] 12B レコードヘッダの field 対応（+4/+5 の意味・dst lea の厳密対応） | 低 |
| V-360-2 | [未確定] writer caller 4件（0x466AF6/0x466BD0/0x466D21/0x5024BA）の this 同一性（グローバル [0xB33B00] 経由2件は確定済） | 低 |
| （維持）V-356-1 動的 | fail entry の**実ファイル残存**の実測（静的側は本裁定でクローズ）＝#82 | 中 |

---

## §7 main への報告

1. **V-356-1 静的側クローズ**: **ライタは entry flags で書出/スキップしない [確定]**（flags は CheckFlags/vfunc+0x4C/0x50 へ渡され内容生成のみに影響）。付帯: bit9 dry カウントモード・Player 特例を発見。**動的確認のみ #82 残**。
2. **V-356-2 [部分確定]**: 0x46667E＝**LoadGame の record 後処理**（formID リマップ→alloc(u16)→読取→0x460BC0→vfunc+0x54→node insert・Player 特例）。size＝**12B ヘッダ+6 の u16（ペイロード長）[部分確定]**。
3. **V-356-4 [確定] 完全解消**: **entry[+4]＝tail フレーム先頭ポインタ**（0==なし）。**`SaveGame+0x14`＝現 entry の tail scratch/cursor** と同定 → **v356 §2.2 のフィールド解釈を更新**（クリア＝tail 失効/処理完了。生存判断の結論は不変）。
4. 副次: `[0xB33B00]`=SaveGame グローバル・dry bit9 実装形・12B レコードヘッダ layout（部分）。docs/GT 反映候補は #47/#20 一括（L1）。

---

### 参照（本タスクの証跡）

- v360_1（ライタ帯整列デコード: flag test 一覧・iterator/45E220/45FB50 呼出）
- v360_2（0x466650-0x466720 全文脈・外部分岐源5件）
- v360_3（pass2 tail==0 決定部の再確認・writer [reg+4] 使用一覧）
- v360_4（entry ループ+route A ヘッダ読・size 源初回探索（バグあり）・0x4652E0 コンテキスト）
- v360_5（route A tail＋**route B 全流**＋flags テスト列挙＋word 書込探索（バグあり））
- v360_6（writer head 0x465130 確認・スキャン是正前チェック）
- v360_7（writer callers 6件＋[esp+0x26] 初回追跡（バグあり））
- v360_8（**disasm_range 是正版**＝[esp+0x26] 読取2件のみを確定）
- v360_9（12B ヘッダ read の dst/stack 算術＝formID=header+0 の実証）
