# V142: §70.10 残 2 件の確定報告（s\*Description の ESM 置換 / FUN_00478780 の型）

- タスク: TODO_re49 #142 — `docs/RE_EXE_REPORT.md` §70.10 未解決点のうち 2 件を分析専任で確定する
- 対象: `Oblivion.exe` v1.2.0416 (Steam) / `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (7,898,624 B)
- 実行日: 2026-10-27 / 作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
- ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）
- マッピング: .text `VA=0x400C00+off`（VA 0x401000–0xA27C39）/ .rdata `VA 0xA28000 / off 0x627200` / .data `VA 0xB02000 / off 0x700C00`（file-backed は 0xB32C00 まで、以降 bss）。capstone 5.0.7。
- **ESM 本体 (277,504,985 B) は再スキャンしていない**。ESM 側の値は既存キャッシュ成果物（V50 時点の s50_gmst 系）を再利用。
- 既存 docs / TODO_re49.md / session-memory.json / plan.md / 既存 tmp-re ファイルは無編集。本ファイルが V142 の新規成果物。

---

## 結論サマリ

| # | §70.10 の疑問 | 判定 | 1 行回答 |
|---|---|---|---|
| (1) | attribute `s*Description` 8 件が ESM ロード時に置換されるか | **解明** | **置換される**。8 件は EXE で中央マップ登録済み、Oblivion.esm にも GMST 実在（値は junk 文字列）→ V55 の hit 経路で value が上書きされる。ただし置換後の値を読むゲームコードは EXE に存在しない。INI との対応はなし（637 INI キーに該当 0） |
| (2) | `FUN_00478780` のオブジェクト型 | **解明** | `this` は **Character 系 Actor 参照が `[ref+0x104]`（プレイヤー 1 人称時は `[ref+0x5C8]`）に保持する 0x154 バイトの装備アタッチ管理ブロック**（非 vtable の素構造体、+0x150 = 所有 ref の逆ポインタ）。HighProcess 系でも TESObjectREFR 本体でもない |

---

## §1. attribute `s*Description` 8 件の ESM 置換 — [解明: 置換される／読者なし]

### 1-1. 対象 8 件の実名（既存確定事実の再掲＋本タスクでの裏取り）

| # | key（原文綴り） | obj VA | 登録 thunk (call) | EXE 既定値 |
|---|---|---|---|---|
| 1 | `sStrengthDescription` | `0xB38590` | `0x9F0A90` (`0x9F0A9F`) | `Need a gamesetting description`（共有 `0xA5DE7C`） |
| 2 | `sIntellegenceDescription`（**原文の綴りミス**） | `0xB38598` | `0x9F0AB0` | 同上 |
| 3 | `sWillPowerDescription` | `0xB385A0` | `0x9F0AD0` | 同上 |
| 4 | `sAgilityDescription` | `0xB385A8` | `0x9F0AF0` | 同上 |
| 5 | `sSpeedDescription` | `0xB385B0` | `0x9F0B10` | 同上 |
| 6 | `sEnduranceDescription` | `0xB385B8` | `0x9F0B30` | 同上 |
| 7 | `sPersonalityDescription` | `0xB385C0` | `0x9F0B50` | 同上 |
| 8 | `sLuckDescription` | `0xB385C8` | `0x9F0B70` | 同上 |

出所: `v59_key_count_diff.md` §4.1 / `v62_v59_residuals.md` §3.1 / `tmp\re\s50_gmst_full.txt` L1356-1363（obj VA・thunk・refs(2) = 登録+dtor の全数一致を再確認）。

### 1-2. EXE 側: 中央マップ登録済み（lookup が必ず hit する）

- 登録形: `push <default>; push <key>; mov ecx,<obj>; call 0x419A70`（FUN_00419A70 = Setting 登録 registrar: 重複検査 `0x55E000` 後に **マップ `0xB35574` へ insert**）。`0xB35574` は GMST マネージャ `0xB35468` の埋め込みマップ（`this+0x10C`）— `v55_gmst_override_path.md` §1。
- 件数的整合: ESM GMST 382 件のうち **378 件が EXE 登録キーと名前一致**（不一致は `fRegionGenTex*` 4 件のみ）— `v93_gmst_gt_residuals.md` §a-3（abs32 参照基準でも 378/378・差 0）。**本 8 件は 378 の中に含まれる**（登録 thunk 実在が個別に確認できている）。

### 1-3. ESM 側: 8 件すべて GMST レコードが実在（値は junk）

既存 ESM スキャンキャッシュ `tmp\re\s50_gmst_esm2.txt`（V50 時点の exe↔esm 値比較）より:

| key | ESM DATA 値（実測） |
|---|---|
| sStrengthDescription | `YOU ARE USING WRONG GAMESETTING` |
| sIntellegenceDescription | `YOU ARE USING WRONG GAMESETTING` |
| sWillPowerDescription | `YOU ARE USING WRONG GAMESETTING` |
| sAgilityDescription | `YOU ARE USING WRONG GAMESETTING` |
| sSpeedDescription | `YOU ARE USING WRONG GAMESETTING` |
| sLuckDescription | `YOU ARE USING WRONG GAMESETTING` |
| **sEnduranceDescription** | **`MESSAGE`**（別 junk） |
| （参考）sCombat/sMagic/sStealthDescription | 実本文（"Allows you to progress more quickly in these skills as you use them: …"） |

- ESM GMST グループ全体像（382 レコード = f243 / s108 / i31、GRUP@0x2FC size 0x6B43）は `v60_esm_only_keys.md` §5 の実測。本 8 件は s 型 108 件の内側。
- **本タスクは ESM 本体を開いていない**（上記はすべて既存成果物の再利用）。

### 1-4. ESM ロード時の置換経路（generic・型ヒット確認済み）

`v55_gmst_override_path.md` §2/§5 で確定済みの経路が本 8 件にもそのまま適用される:

```
ESM ロード: GMST ハンドラ 0x44E326（"GMST" type table 0xB05E04 idx3）
  → 0xB35468 slot9 = 0x4A8040: EDID をマップ 0xB35574 へ lookup
      → 命中時のみ 0x4A7D60（ESM 値適用）: EDID→DATA 前進、
        型判別 0x4A78D0（key 先頭文字: 'f'→5 / 'i'→3 / **'s'→6**）
        → s 型: 0x450C20 + 0x4A7980（String set）で **設定変数オブジェクト本体の value を上書き**
      → 未命中: 無視（insert なし）
```

- 8 件はいずれもキー先頭が `'s'` → 文字列適用パスに **必ず入る**（型不一致によるスキップは不存在）。
- lookup は EDID 完全一致（compare = `__stricmp` 系 `0x584D10`、hash = djb2 `FUN_004A80A0`）。8 件は登録済みなので **ESM ロード時に placeholder → ESM junk 文字列へ確実に置換される**。
- 逆方向の注意: ESM 側にしかないキーは無視される（insert 不要）— 本 8 件は「登録済み・ESM 実在」型なので置換が成立。

### 1-5. 置換後の値を読むコード — [なし]（v62 の再確認結果を維持）

- key 経由 lookup 読者: **0 件**（key VA は登録 thunk 以外どこからも参照されない — `v62_v59_residuals.md` §3.2）。
- obj `0xB38590..0xB385C8` 直接参照: **登録 thunk + dtor thunk のみ**（各 2 件）。クラス 3 件（sCombat/sMagic/sStealthDescription = `0x5D5CBA/57/8D`）と sRaceDescription（6 箇所）にある「生 obj 直読みの読者」は属性 8 件には存在しない。
- 動的キー構築（`%sDescription` 等の書式片）: EXE 全ファイル 0 件。
- 唯一の観測者: コンソール **GetGS/SetGS**（`0x513FC0` / `0x514100` 系）— 同一マップの汎用 lookup 経由なので、`GetGS sStrengthDescription` を打てば置換後の junk 文字列が返る（`v55_gmst_override_path.md` §3）。ゲームプレイ上の消費者はゼロ。

### 1-6. INI との対応 — [なし]

- `v91_ini_defaults.csv`（INI 既定キー **637 行全数**）を全文走査: `Description` / `Desc` ヒット **0 件**（本タスク実測）。`v130_ini_residuals.md`（INI キー 637 ノード確定台帳）にも該当なし。
- ⇒ INI 経由の上書き・観測経路は存在しない。本 8 件は「EXE 既定値 → ESM 上書き」の 2 段のみ。

### 1-7. §1 判定（移植示唆を含む）

**[解明] ESM ロード時に置換される。** 登録済み（map hit）× ESM 実在（junk 値）× generic 適用経路 = placeholder `Need a gamesetting description` は ESM 値（6 件 `YOU ARE USING WRONG GAMESETTING` / sEnduranceDescription は `MESSAGE`）に上書きされる。ただしこの値を読む EXE コードは存在せず、UI が実際に使う属性説明文は表C の `sAttributeDesc*`（`0xB127F8` idx0..7 / thunks `0x9F9B60+0x20i`）側である（`v59` §4.1）。**移植側がこの 8 件を無視しても挙動差分は発生しない**（値の保存・参照実装は不要。監査目的で上書きする場合は上記 junk 値が正）。

---

## §2. `FUN_00478780` のオブジェクト型 — [解明: Character の 0x154B 装備アタッチブロック]

### 2-1. 問題の所在（V62 までの状態）

`FUN_00478780`（0x478780）はアタッチスロット 1 本を解放する関数で、内部に ` [this+0x150] == DAT_00B333C4`（player）と `this == FUN_006600d0(player,0)（=[player+0x104]）` の条件を持つ。「`this` の型は HighProcess 系か TESObjectREFR 系か未特定」（`v62_v59_residuals.md` §6-2）— が本タスクの标的。

### 2-2. 決定的証拠链（すべてバイナリ/decompiled 実測）

1. **`this` の到達経路**: `FUN_006600d0`（decompiled L424197）= `__thiscall FUN_006600d0(this, flag)` → `flag ? *(this+0x5C8) : *(this+0x104)`。呼び出し元（例 `0x4DC920`）では `ecx = [0xB333C4]`（player）、flag = **byte `[player+0x588]`（Show1stPerson 系）**。＝ player は **2 本のアタッチブロック**を持ち、1/3 人称フラグで選択される。
2. **`[ref+0x104]` に何が入るか（生成側）**: `FUN_00478730`（L96286）:
   ```c
   void* __thiscall FUN_00478730(void* this, undefined4 ref, int extra) {
     _memset(this, 0, 0x154);            // 本体は 0x154 バイト
     _memset(&DAT_00b33c80, 0, 0x100);   // player 用グローバルスロット配列も同時クリア
     *(int*)(this + 0x150) = ref;        // +0x150 = 所有 ref の逆ポインタ
     if (extra) FUN_00478070(extra);
     return this;
   }
   ```
   呼出し元 3 箇所（`0x521AD2` / `0x5231E9` / `0x667E16`）はいずれも **`push 0x154; call 0x401F00`（ヒープ確保）→ `mov ecx,eax; call 0x478730`**。＝ **埋め込み構造体ではなく独立ヒープオブジェクト**。
3. **ref への登録**: 呼出し後 `mov eax,[edi]; call [eax+0x16C]` = **Character vtable slot `+0x16C`** へ newly-created ブロックを渡す。`0x667E16` 系（FUN_00667be0）は `[ebp+0x5C8] = eax` で直接第 2 ブロックへ格納。
4. **Character vtable `+0x16C` = `FUN_00611B00`**（L378280）:
   ```c
   void __thiscall FUN_00611b00(int ref, int block) {
     old = *(ref + 0x104);
     if (old != 0) { if (old == block) return; thunk_FUN_004796f0(); FUN_00401f20(old); } // 旧ブロックの全解放+delete
     *(ref + 0x104) = block;   // ← [ref+0x104] = 0x154 ブロック が確定
   }
   ```
5. **Character vtable `+0x168` = `0x4349A0`** = `mov eax,[ecx+0x104]; ret`（.rdata 参照は vtable slot `0xA6FE04` の 1 件のみ）。Character 一次 vtable 基底 `0xA6FC9C`（239 slots、vtable-4 = COL `0xAC36D4` = b07 索引と一致）の **slot 90 (+0x168)** に一致。TESObjectREFR/Actor 基底は `0x6F7070`（`return 0`）＝ **Character がオーバーライドして初めて値が返る**。
6. **Actor 使い分けコード**（FUN_004dc5b0 / FUN_004dc8f0 / FUN_004dcf10 / FUN_004dcf90 / FUN_004dd000、いずれも `param_1 == DAT_00b333c4` 判定＋`param_1[0xf]`（=+0x3C NiNode）＋`param_1[0x16]`（=+0x58 TESForm）＋"Creatures are not allowed to wear amulets."）は **ref 側のメソッド**であり、`(**(code**)(*ref + 0x168))()` の戻り値（= [ref+0x104] ブロック）に slot clear ラッパ群を適用する。逆アセンブル実測（`0x4DC907 call edx` → `mov ecx,edi; call 0x478dd0`）で this 経路を確定。
7. **アクセス範囲の整合**: FUN_00478780 とラッパ群の最大ディスプレースメントは `+0x150`（back-ptr）で、0x154 サイズに完全収まる。HighProcess（vtable 358 slots・サイズ 0x1000 級）や TESObjectREFR（0x804）とは矛盾しない組み合わせが存在しない。

### 2-3. `this` のレイアウト（本タスクで確定した範囲）

| offset | 内容 | 根拠 |
|---|---|---|
| `+0x28` / `+0x30` | NiNode 系ポインタ（手首/武器ノード）。非 0 のとき `FUN_00897a90(node,0)` + `FUN_00477ef0()` で解放 | FUN_00478780 内 L96334-96341 |
| `+0x4C .. +0x13C` | **0x10 ストライドのアタッチスロット配列**（(i+5)\*0x10 で +0x50..+0x140 を走査 = FUN_004788e0、+0x4C+(6\|7)\*0x10 = +0xAC/+0xBC を手スロットとして使用 = FUN_00478e80） | decompiled L96396 / L96723 |
| `+0xCC` | アロー/クイバー スロット解放（FUN_00478ea0） | L96734 |
| `+0xDC` | **右手武器スロット**（part 型コード `'!'`=0x21 を item+4 で検証。FUN_00478ca0 が {item, item+0x30} を書込み） | L96601-96609 |
| `+0x10C` | 左手スロット（`'"'`=0x22、FUN_00478d70） | L96649-96657 |
| `+0x11C` | 追加スロット（FUN_00478dd0、unequip 系から解放） | L96675-96679 |
| `+0x12C` | **アミュレットスロット**（`0x1A`、FUN_00478df0/e30。「Creatures are not allowed to wear amulets.」と対応） | L96686-96713 |
| `+0x150` | **所有する Actor ref の逆ポインタ**（player は `DAT_00B333C4` と一致） | FUN_00478730 L96292 |
| （スロット要素） | `{+0: part, +4: part+0x30（ノード）, +8: item（+4=refcount, vtable+0 dtor）}` — 解放時に `FUN_007B4280`/`FUN_007C5E70`/`FUN_00481350`（3D デタッチ）→ `[item+0x1C]` の vtable+0x88 呼び出し → スロットクリア | FUN_00478780 本体 L96313-96352 |

補助: **player 専用グローバルスロット** `DAT_00b33d10`（右手）/ `DAT_00b33d60`（左手）/ `DAT_00b33c80`（0x100B = 16×0x10 配列）も同一関数が「スロット」として扱い、ブロック初期化時にゼロクリアされる。右手解放時[player] のみ HUD 武器アイコンを H2H へフォールバック（`FUN_0057B190([0xB33D84])`）— V62 §2.3 の 3 箇所目 `0x478890` と一致。

### 2-4. 混同しやすい隣接事実（新規旁証）

- **Process 系 vtable にも同型フィールド規約**: HighProcess vtable `0xA71814` の slot 263（+0x41C、実体 `0xA71C30`）と MiddleHighProcess `0xA72684` の同 slot（実体 `0xA72AA0`）は **thunk `0x42BB10` = `mov eax,[ecx+0x150]; ret`** を指す。＝ Process 本体も `[+0x150] = owner ref` を返すゲッタを持つ。**アタッチブロック（0x154B）と Process は別オブジェクト**であり、`+0x150 = ref` というフィールド規約を共有するだけの混同注意点。
- `0x401F00` / `0x401F20` は本ブロックの new / delete として使用されている（§70.10 の別問「アロケータ 0x401F00」への旁証だが、当該問の担当節を本タスクでは更新しない）。
- `TESNPC::InitWorn`（`FUN_00524280`、"Bad part '%s' in TESNPC::InitWorn." を保持）の case 9（アミュレット部品）は **呼び出し元から渡されたこのブロック**（ebp = param_2）に `FUN_00478ca0` で右手 part を直接装着する（`0x52432B mov ecx,ebp`）。TESNPC 本体がブロックを持つわけではない。
- base の `vtable+0x164` = `FUN_004D8370`（別物: formType 0x32/0x33 の ref から `[ref+0x58]+0x17C`（TESNPC テンプレ系）を返す。アタッチブロックではない）。

### 2-5. §2 判定

**[解明] `FUN_00478780` の `this` = Character（Actor 系 ref）が所持する 0x154 バイトの装備アタッチ管理ブロック**（エンジン内部に名前なし。本報告では「アタッチブロック」と呼称）。参照は `[ref+0x104]`（Character vtable +0x168 ゲッタ / +0x16C セッタで管理、プレイヤーは Show1stPerson=+0x588 で `[ref+0x5C8]` に切替）、+0x150 に所有 ref の逆ポインタ。**V62 の二択（HighProcess 系 / TESObjectREFR 系）はいずれも否定**され、第三の答え（ref が所有する専用サブオブジェクト）で確定。

---

## §3. 新規残点（V143+ 粒度）

1. **`DAT_00b33d10` / `DAT_00b33d60` / `DAT_00b33c80` グローバル 3 種の正式な所有関係** — player 用グローバル手スロットであること（FUN_00478780/8730 の実測）まで。初期化順序と 1/3 人称 2 ブロックとの多重所有の有無は未検証。**[低]**
2. **`FUN_00667be0`（`[ebp+0x5C8]` への第 2 ブロック格納文脈）の関数同定** — 「player の 1st person body 用ブロック生成」仮説が +0x588 連動と整合するが、関数名義（どの spawn 経路か）は未特定。**[低]**
3. **Process vtable slot 263（+0x41C）ゲッタの消費者列挙** — `[process+0x150] = owner ref` の新規確定事実（§2-4）。誰が呼ぶかは未走査。**[低]**
4. **`0x4A7980`（String set）の所有権** — ESM 適用時に文字列をコピーするか record メモリを指すか（置換の恒久性・セーブ相互作用に影響しうるが、読者が存在しないため本 8 件では無影響）。**[低]**
5. §1 の「読者ゼロ」は静的解析基準（key lookup / 動的構築 / obj 直読みの全走査）。実機での GetGS 観測以外に到達しないことの動的検証は未実施。**[残]**

（§70.10 内で本タスク対象外だった項目 — `0xA28860..0xA2D184` 初期化配列・`0x981FB4`/`0x981F78`（V126 合流済）/ AV コード名テーブル（節 46 判定済）— には触れていない。）

---

## §4. 使用証拠一覧

### 4-1. 既存成果物（読み取りのみ・ESM 再スキャンなし）

| 資産 | 使用箇所 |
|---|---|
| `docs/RE_EXE_REPORT.md` §70.10 / §70.2（V62 節） | 問題文と背景（11875 / 11987 行） |
| `tmp\re\v59_key_count_diff.md` §4.1 | 12 件キー族の実名・obj VA・thunk |
| `tmp\re\v62_v59_residuals.md` §2-3, §3, §6 | 属性 8 件の「登録のみ・読取なし」、FUN_00478780 の条件式、未特定の問 |
| `tmp\re\v55_gmst_override_path.md` §1, §2, §3, §5 | 中央マップ構造、ESM 適用経路（0x44E326→0x4A8040→0x4A7D60→'s'→0x4A7980）、GetGS/SetGS |
| `tmp\re\v60_esm_only_keys.md` §4, §5 | 登録経路全体像、ESM GMST 382 件（f243/s108/i31） |
| `tmp\re\v93_gmst_gt_residuals.md` §a-3 | ESM 382 × EXE 名前一致 378（未一致は fRegionGenTex\* 4 のみ） |
| `tmp\re\s50_gmst_full.txt` L1356-1363 / `s50_gmst_rows.json` | 8 件の登録行（obj/thunk/dtor refs(2)）の再確認 |
| `tmp\re\s50_gmst_esm2.txt` L228-233, L200-202 | **ESM 側 DATA 値の一次証拠**（junk 8 件＋クラス 3 件実本文） |
| `tmp\re\v91_ini_defaults.csv`（637 行） | INI キー全数に Description 系 0 件（本タスクで全文走査） |
| `tmp\re\v130_ini_residuals.md` | INI 637 ノード台帳との整合 |
| `tmp\re\v72_global_b333c4_map.md` | 0xB333C4 = 0x804B ヒープオブジェクト（player）、+0x588/+0x738 フィールド地図 |
| `tmp\re\b07_vtable_index.md` L699, L809-812, L172-178 | Actor/Character vtable 基底・slot 数・Process 系 vtable |
| `tmp\re\b13_nonrtti_vtable_classes.md` | 非 RTTI 断片（Character 0xA6FC1C/C4C/C94 等）の照合 |
| `docs/Oblivion_decompiled.c`（部分検索のみ） | FUN_00478780 本体 L96301、FUN_00478730 L96286、ラッパ群 L96599-96736、FUN_00611B00 L378280、FUN_006600d0 L424197、FUN_004d8370 L169912、Actor 系 caller L173533-174081、TESNPC::InitWorn L217985 |

### 4-2. 新規スクリプト（すべて `%TEMP%\opencode\`）

| ファイル | 内容 |
|---|---|
| `v142_p1_refs.py` | [player+0x104] 参照走査（dec）＋ slot clear ラッパ群の E8 caller 全走査 |
| `v142_p2_enclose.py` | caller 站点の enclosing 関数同定＋本文ダンプ（Actor 使い分け 5 関数を発見） |
| `v142_p3_wrappers.py` | ラッパ群本文（L96590-96745）＋ InitWorn/FUN_004dc8f0 呼出し点の逆アセンブル（ecx 経路実証） |
| `v142_p4_writers.py` | `+0x104`/`+0x150` 代入行の全走査（dec）＋ ctor FUN_0066a740 本文確認 |
| `v142_p5_final.py` | **+0x150 書き込み元 = FUN_00478730 を特定**、TESObjectREFR vtable slot 実体読出 |
| `v142_p6_final2.py` | vtable+0x168/0x164/0x190 基底実装の dec（0x6F7070=return 0 等）、FUN_00478730 の E8 caller 3 件、InitWorn head 逆アセンブル |
| `v142_p7_actor.py` | **Actor vtable slot 実体＋「new(0x154)→FUN_00478730」3 呼出し点の ecx 経路実証**（0x667E2E で [ebp+0x5C8] 格納を発見） |
| `v142_p8_setter.py` | vtable+0x16C 実体探索＋関連文脈（"This npc … already been used." 等） |
| `v142_p9_thunks.py` | **`mov eax,[ecx+0x104];ret` thunk 0x4349A0 の .rdata 参照 = 0xA6FE04 を発見**、+0x150 getter thunk 0x42BB10 の Process 系 vtable 参照を発見 |

検証日: 2026-10-27。ハーネス selftest 4/4 PASS。本ファイルが V142 の正本（`docs/`・`TODO_re49.md`・`session-memory.json`・`plan.md`・既存 `tmp/re` ファイルは無変更）。
