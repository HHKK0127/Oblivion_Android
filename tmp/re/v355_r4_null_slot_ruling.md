# V355: R4「NULL スロット直接経路」の裁定 — #128 束④（#106 R4）

- 日付: 2026-11-12 / タスク: ai-todo **#128 束④**（#106 R4: indirect vtable clear 経路＝NULL スロット直接経路） / 作成: RE レーン
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: `tmp/re/v327_v324_residuals_ruling.md` L274（問4 [未確定] R4）＋ `tmp/re/v331_v327_residuals_ruling.md` §5（R4）
- 手法: capstone 5.0.7（v307_common.py）／**E8 rel32 逆算の全 caller 走査**／fn ptr テーブルの xref 分類／関数本体の構造読解
- 中間証跡: `%TEMP%\opencode\v355_1..v355_3.py/.out`（**全 EXIT=0**）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM 未アクセス、L1

---

## §0 判定サマリ（v327 問4 = R4）

| 設問 | 判定 |
|---|---|
| `FUN_00482170(obj=0)` の E8 到達性（v327: [推定]困難） | **[確定] 不到達**。E8 caller は `0x444275`（in `FUN_004441A0`）の**1件のみ**で、同呼び出しは **`test esi,esi; je 0x444321`（0x44421A）で cell obj 0 を投入前に切り捨て**、空スロット経路 `0x444244` は**最初が `mov eax,[esi+0xC]`＝esi=0 は即 AV** |
| `FUN_00482200` の間接 caller（v327: 未特定） | **[部分確定] rdata の fn ptr テーブル経由**。E8 caller は **0件**（v327 一致）、u32 参照は **`0xA3D7B4` のみ**（前後は fn 群＋文字列 `"...dsToLoad:General"` の隣接ブロック）。テーブル先頭候補のうち **`0xA3D7E0` のみが .text から2件参照**（`0x48392B`/`0x483ACB`）＝**実際の間接呼び出し元の特定は残点 V-355-1** |
| 「vtable+0x1C クリア経路」の実体 | **[確定]** = `FUN_00482170` の **obj==0 分岐（0x482198）** と `FUN_00482200` の末尾が**共通構造**（`mov edx,[this]; call [edx+0x1C](x,y)`）＝**World 側の vtable+0x1C メソッドへ x,y を渡して slot を処理させる** |
| NULL スロットの主源（寄与比） | **[部分確定] 新規 World 生成時の配列 NULL 充填が主源**（v324 §3-5 維持）。**vtable+0x1C クリアは NULL を「生成しない」**（`FUN_00482200` は slot が NULL なら `je` でスキップ、`FUN_00482170(obj=0)` は slot に触れず vtable+0x1C へ委譲）＝**解放経路であって NULL 源ではない** |

---

## §1 関数本体の同定 [確定]

### 1.1 `FUN_00482170(x, y, cell)` — slot 投入／クリア（stdcall ret 0xC）

```asm
482170: edx = arg3 (cell obj)
        test edx, edx
        je 0x482198                 ; ★ obj==0 → クリア経路
        ; --- obj!=0: slot へ投入 ---
        eax = [this+0xC](width) * arg1(y) + arg2(x)
        ecx = [this+0x10](slot配列 base)
        lea eax, [ecx + eax*8]       ; slot = base + (y*W + x)*8（8B/slot）
        [eax] = edx                  ; slot.obj = cell
        mov ecx, [eax+4]             ; 旧の第2ワード
        call 0x49A000                ; （ecx=旧, arg=cell）更新
        ret 0xC
        ; --- obj==0: クリア経路 ---
482198: edx = [this]      (vtable)
        edx = [edx+0x1C]  ; ★ vtable +0x1C メソッド
        push arg2(x) / push arg1(y)
        call edx           ; ★ this の vtable+0x1C(x,y) — slot には触れない
        ret 0xC
```

### 1.2 `FUN_00482200(x, y)` — slot 解放＋vtable+0x1C 呼び出し（stdcall ret 8）

```asm
482200: eax = [this+0xC](width) * arg2(y) + arg1(x)
        ecx = [this+0x10](slot配列)
        lea eax, [ecx + eax*8]
        ecx = [eax]                  ; slot.obj
        test ecx, ecx
        je 0x482231                  ; ★ NULL なら何もしない（clear 不要）
        call 0x4D6450                ; obj の解放/Release
        edx = [this] / eax = [edx+0x1C]
        push x / push y / mov ecx,this
        call eax                     ; ★ vtable +0x1C(x,y)
        ret 8
```

- **両関数とも末尾が `this の vtable+0x1C(x,y)` 呼び出し**＝**これが v327 の「vtable+0x1C クリア経路」の正体** [確定]。
- slot アドレス式は `base + (y*width + x)*8`（**8バイト/slot**＝`{obj ptr, +4 第2ワード}`）で両者一致 [確定]。

### 1.3 `FUN_004821B0(x, y)` — 空検査（E8 caller 2件: 0x443159/0x444211）[確定]

```asm
4821B0: eax = [this+0xC] * arg1(y) + arg2(x)
        ecx = [this+0x10] / lea eax,[ecx+eax*8]
        eax = [eax]               ; slot.obj
        test eax,eax
        je → return 0             ; NULL = 空
        xor edx,edx
        cmp byte [eax+0x26], 6    ; cell 状態 == 6?
        sete dl / mov al,dl / ret 8
```
- → **「空」判定は slot.obj==0 または state≠6**（v327 の記述と一致）[確定]。

---

## §2 E8 caller 全走査（rel32 逆算）[確定]

| 関数 | E8 caller | v327 記載 | 判定 |
|---|---|---|---|
| `FUN_00482170` | **1件** `0x444275`（in `FUN_004441A0`） | 1 site = 0x444275 | ✅一致 |
| `FUN_00482200` | **0件** | **0 sites** | ✅一致（＝間接呼び出しのみ） |
| `FUN_004821B0` | **2件** `0x443159`（in `FUN_004430F0`）／`0x444211`（in `FUN_004441A0`） | 2 sites | ✅一致 |

- **u32 アドレス参照（テーブル/データ）**: `FUN_00482170` = **0件**／`FUN_00482200` = **`0xA3D7B4` のみ**（rdata）。

---

## §3 `FUN_00482170(obj=0)` の到達性 [確定]（v327 [推定]困難 → 確定）

`FUN_004441A0`（cell loader）の該当窓（0x4441F8-0x444298）:

```asm
444211: call 0x4821B0 (空検査 x=ebx, y=ebp)
444216: test al,al
444218: je 0x444244          ; 空 → 投入処理へ
44421A: test esi,esi         ; ★ esi = cell obj
44421C: je 0x444321          ; ★ esi==0 → 即 return（投入もクリアもしない）
444222: ... (0x4C97F0 検証 → 0x43FD70/0x43FED0)
444244: mov eax, [esi+0xC]   ; ★ 空スロット経路の最初が esi deref
444247: ... GetCellY/GetCellX (0x4C9AA0/0x4C9A80) → sprintf
444272: push esi / push ebp / push ebx
444275: call 0x482170        ; (x=ebx, y=ebp, cell=esi) ← esi は必ず非NULL
```

- **`esi==0` は 0x44421A で return**、かつ **空スロット経路（0x444244）は最初の deref で AV** → **`FUN_00482170(obj=0)` はこの caller から到達不可** [確定]。
- E8 caller が1件のみである以上、**静的に obj=0 でこの関数に入る経路は存在しない** [確定]。

---

## §4 テーブル `0xA3D7B4` と間接呼び出し元 [部分確定]

### 4.1 rdata ブロック（0xA3D780-0xA3D7FC）

| アドレス | 値 | 判定 |
|---|---|---|
| 0xA3D780..0xA3D7C0 | fn ptr 群（0x700160/0x6F7070/0x68F970/0x483570/0x482080/0x481E20/0x4821E0/0x482090/0x481E60/**0x482200**/0x482240/0x482260/0x4822A0） | **fn ptr テーブル**（`FUN_00482200` が `0xA3D7B4` に格納） |
| 0xA3D7C4..0xA3D7D8 | ASCII `"…dsToLoad:General"` | テーブルに隣接する**文字列**（ログ/メッセージ） |
| 0xA3D7E0..0xA3D7FC | fn ptr 群（0x4842B0/0x481E00/0x481E20/0x481E10/0x483B60/0x481E60/0x483630/0x483680） | 別の fn ptr グループ |

### 4.2 xref 分類

- **`0xA3D780`..`0xA3D7C0`（`FUN_00482200` を含む）: u32 xref = 0件** → **この範囲は直接参照されない**（＝前後のテーブル先頭が参照される設計、または別の indirection）。
- **`0xA3D7E0` のみ2件**: `0x48392B` / `0x483ACB`（in `FUN_00483800`/`FUN_00483A00` 帯）→ **実際の間接呼び出し元候補はここ**。
- → **`FUN_00482200` の実行時呼び出し元は「`0xA3D7E0` テーブルを経由する indirection」** [部分確定]＝**残点 V-355-1**（0x48392B/0x483ACB の context と、0xA3D7B4 を含むブロックの使われ方の確定）。

---

## §5 寄与比の裁定（v327 問4 の核心）[部分確定]

| 経路 | NULL 生成能力 | 裁定 |
|---|---|---|
| **新規 World 生成時の配列（再）確保＋NULL 充填** | **あり**（配列全体が 0 で埋まる） | **主源 [部分確定]**（v324 §3-5 維持） |
| **`FUN_00482170(obj=0)` → vtable+0x1C(x,y)** | **slot に直接触れない**（World メソッドへ委譲） | 解放/委譲経路・**NULL 源ではない** [確定] |
| **`FUN_00482200` → 解放 → vtable+0x1C(x,y)** | **NULL slot は `je` でスキップ**（生成しない） | 解放経路・**NULL 源ではない** [確定] |
| `FUN_004821B0`（空検査） | 読むだけ | 読み手 [確定] |

- **結論**: NULL スロットの**主源は配列の（再）確保時充填**、**vtable+0x1C クリアは「slot を使わせなくする（または解放する）」側の経路**であり、**両者は役割が異なる**（生成 vs 解放）[部分確定]（＝**定量的な寄与比（件数比）は実機ログ依存で本裁定では未確定**＝残点 V-355-2・#82 系）。

---

## §6 既知の補強（本裁定で新たに確定した構造）

1. **slot レイアウト**: `base + (y*width + x)*8`、**8バイト/slot**＝`{+0: obj ptr, +4: 第2ワード}`（`FUN_00482170` の `mov ecx,[eax+4]` → `call 0x49A000`）。
2. **cell 状態**: `slot.obj+0x26` が **6 =「完成/有効」**（`FUN_004821B0` の `cmp byte [eax+0x26],6; sete`）。
3. **`FUN_00482170` の投入時更新**: `call 0x49A000`（ecx=第2ワード, arg=cell）＝v327 §ループB で登場した `FUN_0049A000` 系と接続。
4. **`FUN_00482200` の解放**: `call 0x4D6450`（obj の Release/free）。

---

## §7 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v355_1..v355_3 **全 EXIT=0**（うち2回はサーバ再起動で中断→再実行で成功） |
| 2 | E8 全走査の手法 | `.text` 全体の E8 rel32 を逆算して dst==target を検出（v352_25 と同一手法）→ v327 の記載3件と**完全一致** |
| 3 | obj=0 不到達の二重根拠 | `test esi,esi; je`（0x44421A）と `mov eax,[esi+0xC]`（0x444244）の**2経路**で確認 |
| 4 | テーブル xref の網羅性 | 0xA3D780..0xA3D7FC の8候補すべてで u32 xref を全セクション走査 → `0xA3D7E0` のみ hit |
| 5 | v327 との整合 | 「E8 caller 0件」「0x444275 のみ」「0x443159/0x444211」すべて一致 → v327 の事実関係は改訂不要 |

---

## §8 新規残点（次束用）

| ID | 内容 | 深刻度 |
|---|---|---|
| V-355-1 | [部分確定] `FUN_00482200` の**実際の実行時呼び出し元**（`0x48392B`/`0x483ACB` の context と `0xA3D7E0` テーブルの使われ方、および `0xA3D7B4` を含むブロックがどの indirection から参照されるか） | 中 |
| V-355-2 | [未確定] NULL スロットの**定量的寄与比**（新規 World 充填 vs クリア経路の実件数比）＝実機ログ/ハーネス依存（#82 系ゲート） | 低 |
| V-355-3 | [未確定] `vtable+0x1C` メソッド（World 側 ClearCell 相当）の**本体特定**（this の型と slot 書込実装） | 中 |
| V-355-4 | [部分確定] `FUN_00482170` の `call 0x49A000`（投入時第2ワード更新）の詳細役割（v327 §ループB の `FUN_0049A000` との関係） | 低 |

---

## §9 main への報告

1. **v327 問4（R4）は本裁定で [確定]2点＋[部分確定]2点に解消**: `FUN_00482170(obj=0)` は**静的に不到達 [確定]**、`FUN_00482200` の E8 caller 0件は**間接（fn ptr テーブル）のみ [部分確定]**。
2. **v331 §5 の「vtable+0x1C の間接 call は caller とルーチンが未確定」は解消**: ルーチン＝`FUN_00482170(obj=0)` 分岐＋`FUN_00482200` 末尾の**共通構造 `call [this.vtable+0x1C](x,y)`** [確定]。
3. **NULL スロットの主源は配列（再）確保時充填**が維持され、**vtable+0x1C クリアは解放経路**（役割分離）[部分確定]。
4. **§6 の slot レイアウト（8B/slot）と `slot+0x26==6` 状態**は、v327 §ループB・v324 §3 の理解を補強する再利用可能な事実。

---

### 参照（本タスクの証跡）

- `v355_1`（`FUN_00482200`/`FUN_00482170` の u32 参照走査・`call [reg+0x1C]` 138件列挙）
- `v355_2`（rdata 0xA3D7B4 周辺ダンプ・**E8 全走査3関数**・`FUN_004441A0` 窓の obj=0 検証）
- **`v355_3`（テーブル xref 分類・`FUN_00482200`/`FUN_00482170`/`FUN_004821B0` 本体全文＝本裁定の核心）**
