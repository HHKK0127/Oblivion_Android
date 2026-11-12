# V66 / V68 照合結果（2026-10-24）

対象: Supermemory 矛盾候補3点の EXE/decompiled/リポジトリ実測による裁定。
環境: `Oblivion.exe`（Steam版、capstone 5.0.7、ImageBase 0x400000、VA = 0x400C00 + file_off）、`probe_harness.py`（selftest 4件 **ALL PASS**: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
関連文書: `memory_contradictions_20261024.md` §⑧、`memory_structure_map_verify_20261024.md` §60-61、`TODO_re49.md` V66/V68 行。

---

## V66-1: gate/decoder の実住所（bfb0dcba の 0x4FAA80 主張）

**Verdict: `bfb0dcba…`「gate analysis 0x4FAA80 real decoder」は誤り → supersede。正 = 0x4FAE80（GT/§67 表どおり）。0x4FAA80 は +0x400-low（非境界アドレス）。**

### Evidence

**EXE バイト実測（境界判定）**

- `0x4FAA80`: 先頭16バイト `1c 9e b0 00 5d 83 c4 08 c2 08 00 cc cc cc…`
  - 0x4FAA80 起点デコードは `sbb al,0x9e`（ゴースト命令）→ **命令境界上にない**。
  - 実体は前関数の tail: `b0 00 5d 83 c4 08 c2 08 00` = `mov al,0; pop ebp; add esp,8; ret 8`（0x4FAA84–0x4FAA8A で終端）→ int3 ×5 → **次の関数は 0x4FAA90 開始**（`push ebx; push esi; lea esi,[ecx+0x48]`、`[esi]`/`[esi+4]` リンク走査 + `call 0x98262d` のリスト走査関数、ret+CC まで 0x99 バイト）。
  - pre16（0x4FAA70–0x4FAA7F）= `89 1d b4 61 b3 00 5b 89 3d b0 61 b3 00 5f 89 2d`（コード本体。int3 前置なし）→ 0x4FAA80 は関数先頭ではない。
- `0x4FAE80`: 先頭16バイト `83 ec 18 83 7c 24 1c 00 75 06 32 c0 83 c4 18 c3`
  - `sub esp,0x18; cmp dword [esp+0x1c],0; jne; xor al,al; add esp,0x18; ret` = **正規プロローグ**。Ghidra も `FUN_004fae80` として関数定義を作成済み。

**機能特性（capstone スキャン、ret+CC 終端まで）**

| 項目 | 0x4FAA80（起点とした場合） | 0x4FAE80 |
|---|---|---|
| 関数長 | 0x99（境界は 0x4FAA90 起点の別関数） | **0x631** |
| 命令数 | 74（頭は非境界デコード） | 514 |
| byte ptr オペレーション | 1 | **58** |
| movzx / movsx / shr / xor | 0/0/0/1 | **12/5/7/4** |
| 戻りエッジ（ループ） | 2 | **6** |
| 呼出先 | 0x98262d のみ | 0x4FA9C0, 0x469520, 0x9832E6（参照解決）, 0x9812C0, 0x4FAB20, 0x9828C0, 0x573880 |

- 0x4FAE80 本体はオペランドストリーム走査（`mov [ebp],eax` でカーソル前進、`movzx`/`cmp cl,0x72` でタグ判定、`cmp byte [eax*8+0xB0A54D],0` で型テーブル参照）＝ **スクリプトコマンド共通の引数デコーダ/評価器（trampoline/gate）**として GT どおり。

**decompiled.c / annotate_db**

- `FUN_004fae80`: 定義あり（L194577 `uint FUN_004fae80(int param_1,int param_2,int *param_3,…)`）、呼出 **100件超**（grep 上限到達で切れた）。
- `FUN_004faa80`: **0件**（ラベル自体が存在しない）。
- `annotate_db.json`（functions 368件）: 両アドレスとも無し。

**算術・文書整合**

- 0x4FAE80 − 0x4FAA80 = **0x400**（historical "+0x400-low" バグと一致）。
- tmp/re の多数文書（`functionid_design_spec.md`, `svm_495_miss_addrs.md`, `s102_camera.md`, `s50_*.md`, `s51_cmd*.md`, `s56_gmst_loader.md`, `s57_fatigue_death.md`, `s58_*.md`, `append_s67_69.py`）は一貫して `call 0x4FAE80 ; trampoline/gate`・「引数検査/評価器」。
- 逆方向の主張は `v58_ledger_generator_recovery.md`:142/162（「0x4FAE80 = mid-function continuation、実体は 0x4FAA80」）のみ — 本検証により **mid ののは 0x4FAA80 側**であり、この記述も実測と逆。

### Method

`probe_harness.Ctx`（selftest ALL PASS）で VA 直読み、capstone 5.0.7 逆アセンブル、pre16/int3 前置チェック、ret+CC までの関数長・オペコード統計、decompiled.c grep（`FUN_004fae80` / `FUN_004faa80`）、annotate_db.json キー走査、tmp/re 全 md のアドレス xref。

### Residual uncertainty

- 0x4FAA80 がどの関数の途中か（前関数の開始 VA）は未特定。tail 終端 0x4FAA8A・次関数 0x4FAA90 のみ確定。
- `FUN_004fae80` の呼出総数は「100件超」（grep 上限超過のため実数未計測）。

---

## V66-2: 0x66C580 の身元（linked list processor 説）

**Verdict: 「0x66C580 = linked list processor (0x66C2A0)」（`bfb0dcba…`）は誤り。実体 = `this+0x588` byte フラグのエンジン共通 boolean setter（反転書込み、戻り値 old==new）。リンクリスト処理は皆無（"half-right" でもなく全面却下）。**

### Evidence

**EXE 実測（0x66C580–0x66C5F4、ret 8個の int3 前置で独立関数を確認）**

```
0x66C580  mov al, [esp+4]              ; 引数（thiscall、ecx = this）
0x66C586  mov esi, ecx
0x66C588  cmp byte [esi+0x588], al     ; 読取は BYTE
0x66C58E  sete bl                      ; bl = (old == requested) → 戻り値
0x66C591  test al,al / sete al         ; al = (param == 0)
0x66C598  mov byte [esi+0x588], al     ; ★書込みは BYTE（+0x588、逆値保存）
0x66C5A0  mov eax,[esi]; mov edx,[eax+0x198]; push 0; call edx   ; vtable+0x198
0x66C5B0  fld [0xB36B80]; fstp [0xB3BB24]                       ; 成功時 float 同期
0x66C5BC  cmp byte [0xB3BB04],0; jne end
0x66C5C5  cmp dword [esi+0x5D0],0; je end
0x66C5D2  mov byte [0xB14E4D],1
0x66C5D9  call 0x66B710 (0)
0x66C5EB  call 0x664F70 (!flag)
0x66C5F0  mov al, bl; ret 4
```

- **byte/integer 曖昧の解消**: decompiled の `param_1[0x162]`（int 添字＝+0x588）はデコンパイラ表記であり、実命令は `cmp byte`/`mov byte`（0x66C588/0x66C598）＝**+0x588 への BYTE 読取・BYTE 書込**。逆に `[esi+0x5D0]` は dword 読取（`param_1[0x174]`）。
- **リンク走査なし**: 呼出は vtable+0x198・`0x66B710`・`0x664F70` の3件のみ。`[+0x4]`/`[+0x8]` next/prev ループなし。
- **`FUN_0066c2a0` は存在しない**: decompiled.c 全体で `66c2a0` 出現 **0件**、annotate_db にも無し。参照先不明の住所が身元説に使われている。
- decompiled.c L431340: `bool __thiscall FUN_0066c580(int *param_1,char param_2)` — 小関数（本文19行）。

**呼出網（decompiled.c 実測、v66_callers.py）**

- 呼出行 **21** / 呼出元関数 **16**（既知の EXE E8 実測 22件と 1件差。下記 residual）:
  - `FUN_00671620` ×3（フレーム/トグル系、+0x588 xor-toggle の当事者）
  - `FUN_0066c600` / `c650` / `c6f0` / `cc40` / `ceb0` / `cf20` 各×1（+0x588/+0x589/+0x58a フラグ群近傍）
  - `FUN_00633080` / `633130` / `6331c0` 各×2
  - `FUN_00528d90` ×2（SetStage helper 0x529AC0 地帯）
  - `FUN_006005f0` ×1（Kill 管線）、`FUN_0059e580`（アルケミー地帯）、`FUN_005c28f0`、`FUN_005ca010`
- Show1stPerson 経路（opcode 0x113 → 0x50A050）からの呼出 0件 = 既存 GT と一致。

### Method

decompiled.c の関数定義マーカー走査で呼出網を関数単位に分類、capstone で 0x66C580 起点の書き込み命令オペランドサイズ（byte）と call 目標を直接確認、annotate_db/decompiled で `FUN_0066c2a0` の存否を検索。

### Residual uncertainty

- decompiled 21 vs EXE 22 の呼出件数 1件差（デコンパイラの統合・重複展開いずれかの可能性。未特定）。
- 「linked list」誤同定の流入元は未特定（参考: V66-1 で判明した 0x4FAA90 = リンクリスト走査関数だが、因果は未証）。
- +0x588 フラグの完全セマンティクス（first person 相当と推定）は本件スコープ外。

---

## V68: EVMP「516」の出所

**Verdict: 出所 = Supermemory エントリ自体（2026-10-22 のアーキテクチャ系2件 `mTDswMVcnW2qSLLNzRD27K` / `WMYfcQmEVGnZfcKmAx8ArC`）。「516」は同エントリが主張する "official EVMP VM table" のエントリ数で、エントリ内部分解 516 = 158(present) + 358(missing) が正本。リポジトリ artifact は 0 件、EXE 実測テーブル（532エントリ）とも不一致 → 出所はメモリのみ・未検証数値と確定（undeterminable ではなく origin特定済み／ただし EVMP の正体は undeterminable）。**

### Evidence

**原典エントリ（ローカル dump `sm_full.txt` #49/#47、supermemory search でも corroborated）**

- `mTDswMVcnW2qSLLNzRD27K`（2026-10-22T04:09:40Z, type=architecture）:
  "This report cross-references the **official EVMP VM table (516 entries)** with the decompiled source (21,232 functions) and identifies **358** functions that are missing (MISS) … SetStage -> **3,479** calls…"
- `WMYfcQmEVGnZfcKmAx8ArC`（2026-10-22T04:11:01Z, type=architecture）:
  "splits the **516 handlers** into five priority phases … revealing that **158 of the 516** functions were actually present but invisible to Ghidra…（出典として `s69_scda_b_2_handlers.txt` を引用）"
- → **516 = 158 + 358 が両エントリに明記**（検算一致）。「欠358」表記（TODO_re49 V68 行）もこれと整合。

**リポジトリ走査（rg: `\b516\b` / `EVMP` in tmp\re, docs）**

- `EVMP` の出現は矛盾追跡文書のみ（`memory_contradictions_20261024.md`:68、`memory_structure_map_verify_20261024.md`:12/44/72、`TODO_re49.md`:254 — すべて自ら「出所未確認(mem-only)」と自認）。
- `\b516\b` の非追跡ヒット: `v5_ghidra_gaps_report.md`:415（表の行番号＝IsActor 0x506D00 の行）と `docs/RE_EXE_REPORT.md`:911（クエストステージ総数 2,516）のみ → EVMP 无关。

**引用元ファイルの検証（WMYfcQm が典拠とする `s69_scda_b_2_handlers.txt`）**

- 実在（1,901行、handler ブロック **28件** — エントリの「28 capstone-dumped handlers」とは一致）。
- ただし `516` / `158` / `358` / `532` の計数記述は **0件**（出現する `00509516` 等はアドレス/バイト列の偶発一致）→ **エントリの 516/158/358 は引用元ファイルに支持されない**。

**算術仮説の裁定**

- 532−16 / 531−15 / 495+21 / 480+36 はいずれも成立するが典拠ゼロ。テキストで典拠があるのは **358+158** のみ。
- 分母系列 532/531/527/495/480/516/368 はエントリ `98v4sWUmZjfQcwJbACFXLG`（"MISS denominators verified…"）に集約されており、516 は同系列経由で伝播（supermemory search "516 EVMP" で確認、検索1回使用）。
- 旁証（信頼性低下）: 同エントリの "21,232 functions" は実測3値（20,465 / 21,240 / 19,593）のいずれとも不一致、「SetStage→3,479 calls」は V65 で出典不明と判定済み → 本エントリ群は未検証の AI 由来要約と整合。

### Method

rg（`\b516\b` / `EVMP`）を tmp\re・docs に走査、ローカルメモリダンプ（sm_dump/sm_full/num_ctx/addr_xref/name_xref/xa_pairs）から原典エントリ全文を復元、supermemory mode=search（scope=project, "516 EVMP"、1回使用/上限2回）、引用元 `s69_scda_b_2_handlers.txt` の handler 数カウントと計数語検索。

### Residual uncertainty

- "EVMP" の正体（ツール名/セッション名/誤記）は未特定 — リポジトリ内に手がかりなし。
- 516 が実在の別スキャン結果を指すか完全な hallucination かは、メモリ外に判別材料が無いため判別不能。
- `jfqJjZKUZSsXUiA3ikp8tj` 本体はローカル dump（76件時点）より新しく未収録だが、参照数値（516/358）は上記2件に追跡済み。

---

検証日: 2026-10-24。本ファイルが V66/V68 の正本（`TODO_re49.md` / `docs/` は未変更）。
