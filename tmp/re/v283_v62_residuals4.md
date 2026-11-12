# v283_v62_residuals4.md — v60v76-bundle 依頼1 [P1]: v62 残件4点（2026-11-04）

- 依頼元: `tmp/re/handoff_20261105_v60v76_residuals_bundle.md` 依頼1 [優先度 P1]（出典 `tmp/re/v62_v59_residuals.md` §6 残点）
- 対象: Oblivion.exe v1.2.0416 (Steam) / 7,898,624 B
- ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**
- **判定: 4点とも [確定]**

---

## 項目1: s*Description 系の ESM 置換有無 → **[確定] 置換あり（12件中 10件が ESM に実在・文字列型も転送）**

### 1-1. EXE ⇔ ESM 対応表（全数）

EXE は12件すべてを `FUN_00419A70` で中央マップ `0xB35574` に登録（key push を実測）。ESM 側は `gmst_group()`（382件・errors=0）で照合:

| # | EXE 登録鍵（VA / push site） | EXE 既定値 | ESM 実在 | ESM DATA サイズ | 置換 |
|---|---|---|---|---|---|
| 1 | `sStrengthDescription` 0xA5DE64 / 0x9F0A96 | `Need a gamesetting description.` | ○ | 32 B | **あり** |
| 2 | `sIntellegenceDescription`（**綴り違い**）0xA5DE9C / 0x9F0AB6 | 同上 placeholder | ○（**ESM も同じ綴り違い** `sIntellegenceDescription`） | 32 B | **あり** |
| 3 | `sWillPowerDescription`（**大文字 P**）0xA5DEB8 / 0x9F0AD6 | 同上 placeholder | ○（**ESM も `sWillPowerDescription`**） | 32 B | **あり** |
| 4 | `sAgilityDescription` 0xA5DED0 / 0x9F0AF6 | 同上 placeholder | ○ | 32 B | **あり** |
| 5 | `sSpeedDescription` 0xA5DEE4 / 0x9F0B16 | 同上 placeholder | **×** | — | **なし（EXE 既定のまま）** |
| 6 | `sEnduranceDescription` 0xA5DEF8 / 0x9F0B36 | 同上 placeholder | ○ | 8 B | **あり** |
| 7 | `sPersonalityDescription` 0xA5DF10 / 0x9F0B56 | 同上 placeholder | ○ | 32 B | **あり** |
| 8 | `sLuckDescription` 0xA5DF28 / 0x9F0B76 | 同上 placeholder | ○ | 32 B | **あり** |
| 9 | `sCombatDescription` 0xA5DF74 / 0x9F0C16 | 同上 placeholder | ○ | 137 B | **あり** |
| 10 | `sMagicDescription` 0xA5DF88 / 0x9F0C36 | 同上 placeholder | ○ | 150 B | **あり** |
| 11 | `sStealthDescription` 0xA5DF9C / 0x9F0C56 | 同上 placeholder | ○ | 146 B | **あり** |
| 12 | `sRaceDescription` 0xA629C0 / 0x9F68F6 | `Race Description` | **×** | — | **なし（EXE 既定のまま）** |

- ESM GMST に `escription` を含む鍵は**10件のみ**（上記 ○ の10件と完全一致）。**属性8件のうち `sSpeedDescription` と `sRaceDescription` は ESM に存在しない**（ESM 側は属性7件＋クラス3件）。
- **綴りの完全一致が重要**: EXE も ESM も `sIntellegence`（綴り違い）/ `sWillPower`（大文字P）で**両側が同じ綴り** → V55 の lookup はヒットする（片側だけの綴り違いなら不一致になるが、この2件は両側揃い）。

### 1-2. 転送機構（文字列型も実装されていることの実証）

V55 の経路（ESM GMST ハンドラ → `0xB35574` lookup → 命中時 DATA 転送）の先端 `FUN_004A7D60` を実測:

```
FUN_004a78d0(type_char): 's'|'S' -> 6 / 'f' -> 5 / 'i' -> 3 / 'a' -> 8 / 'b' -> 0 / 'c' -> 1 / 'h' -> 2 / 'r' -> 7 / 'u' -> 4
FUN_004a7d60: if (FUN_004510b0() == 'DATA') {
    switch (FUN_004a78d0(type_char)) {
      case 3 ('i'): case 5 ('f'): FUN_004510e0(...)            // 数値 4B 転送
      case 6 ('s'): FUN_00450c20(buf, len) + FUN_004a7980(...) // 文字列: スタックバッファ経由でコピー
    }
}
```

→ **GMST の型文字（EDID 先頭文字）で分岐し、文字列型 's' も専用ブランチで転送される**。V55 の「4バイト転送」は数値型の話であり、**文字列型は type6 ブランチでコピー転送**される（`FUN_00450C20` はレコード/オブジェクトのデータバッファへ書く汎用ライタ、+0x254/+0x264 バッファ管理）。

### 1-3. 判定

- **ESM 置換 = あり**。12件中10件で ESM 値が EXE placeholder を上書き可能（登録済み鍵 × ESM 実在 × 転送ブランチ実在の3条件が揃う）。
- `sSpeedDescription` / `sRaceDescription` は **ESM に無いため EXE 既定値が維持される**。
- v62 §6-1 の残点「ESM 側の該当 GMST 有無は未検証」→ **解消**。
- 注意（v62 の確定済み事実と接続）: 属性8件は **EXE 内に静的読者がいない**（v62 §3.4）ため、ESM に置換されても EXE の静的コードは読まない。読者が実在する4件（クラス3=sCombat/sMagic/sStealth → chargen 変数0xFAF、sRaceDescription → 変数0xFB4）のうち、**3件は ESM 置換の恩恵を受ける**（sRaceDescription は ESM に無し）。

---

## 項目2: `FUN_00478780` の this 型 → **[確定] neither HighProcess nor TESObjectREFR 本体 — 参照が保持する 0x154 B の「装備アタッチ/ボーン管理オブジェクト」**

### 2-1. this（ecx）の実体

| 証拠 | 内容 |
|---|---|
| コンストラクタ `FUN_00478730` | `_memset(this, 0, 0x154)` → **this サイズ = 0x154 B（340 B）**。`this+0x150 = param_2`（＝参照の逆ポインタ）、併せてグローバル `0xB33C80`（0x100 B、`0xB33D10`/`0xB33D60` を内包）をゼロ化 |
| 生成元 | `new(0x154)` → `FUN_00478730` は 3 箇所（`FUN_00521A40` / `FUN_00523170` / `FUN_00667BE0`）。生成後は**参照の vtable +0x168（getter）/+0x16C（setter）**で受け渡し（`push owner; call [vtable+0x16C]`） |
| 保管先 | `FUN_006600D0(ref, 0) = [ref+0x104]`。`FUN_00478780` 内 `mov ecx,[0xB333C4]; push 0; call 0x6600d0; cmp ebx,eax` → **this == [player+0x104]**、かつ **this+0x150 == player**（双方向リンク） |
| vtable | **なし**（ctor が vptr を書かない・owner band 0x478000-0x47C000 に `mov [reg], imm32(.rdata)` 0件）→ TESForm/TESObjectREFR 系ではない |
| 中身 | +4.. にスケルトンボーンポインタ（`FUN_00478070` が `PTR_s_Bip01_Head_00B06550` テーブルを `FUN_006F94A0(node, name)` でルックアップ: **`Bip01 Head` / `Bip01 R Finger1` / `Bip01 L Finger1` / `Weapon` / `BackWeapon` / `SideWeapon` / `Quiver` / `Bip01 L ForearmTwist` / `Torch`** の9点）。**アタッチスロット15個 = +0x4C..+0x12C（0x10 stride）**、+0x28/+0x30 = 解放対象オブジェクト、+0x150 = 参照 |

### 2-2. スロットの内容（v62 の slot clear 族と接続）

setter 群（`FUN_00478CA0`/`00478D70`/`00478DF0`/`00478E80` 等24 caller of `FUN_00478780`）は **TESForm を formType で振り分ける**:

- `cmp byte [form+4], 0x21` → `FUN_0042A3B0` が formType **0x21** を設定する ctor（`*this = &PTR_FUN_00A35968`）
- `cmp byte [form+4], 0x22` → `FUN_0042A180` が formType **0x22**（vtable 0xA35908）
- `cmp byte [form+4], 0x1A` → `FUN_00429D50` が formType **0x1A**（vtable 0xA35800）
- スロット格納: `slot[0] = form` / `slot[1] = form+0x30`（model データ）/ `slot[2] = アタッチされたノード（解放時に InterlockedDecrement + vcall 0x88/0x0）`

### 2-3. 判定

**`FUN_00478780` の this は HighProcess ではなく TESObjectREFR 本体でもない。参照（TESObjectREFR 系オブジェクト）が vtable +0x168/+0x16C で遅延生成し `ref+0x104` に保持する、非多形 0x154 B の「装備アタッチ／スケルトンボーン管理オブジェクト」。** 依頼の2択で答えるなら「TESObjectREFR 系（参照のメンバー）」に近いが、本体とは別オブジェクト。

---

## 項目3: AV 名テーブル `0xB0A1A8` の消費者一覧 → **[確定] 直接消費者3件＋境界走査1件（文脈つき）**

`.text` 内 abs32 は**全セクションで3件のみ**（再現・v62 と一致）。既知 `0x4FEA21`（L12180）は3件目の一部。

| # | 参照先 | 関数 | 文脈（実測） |
|---|---|---|---|
| 1 | `0x4F4610` | `FUN_004F45D0` = **GetBaseActorValue** | `(&PTR_s_Strength_00B0A1A8)[param_2]` を **`FUN_00579B60("GetBaseActorValue: %s >> %0.2f", name, val)`** の引数へ。`cmp byte [0xB361AC],0` でゲートされた**デバッグログのみ**（本体は `FUN_005F1910` の値取得）。caller = `FUN_00501A00` @0x501A4E（E8 1件）。登録 ptr = `.data` |
| 2 | `0x4F60BC` | `FUN_004F6060` = **GetActorValue** | 同型: `FUN_00579B60("GetActorValue: %s >> %0.2f", name, *out)`、`[0xB361AC]` ゲート。vcall `+0x288` / `+0x12C`（`FUN_005E02E0(0)` 経由）で値取得。caller = `FUN_00501670` @0x5016BE（E8 1件） |
| 3 | `0x4FEA26`（起点 `0x4FEA21` = `xor edi,edi`） | `FUN_004FDE30` | **`while (__stricmp((&PTR_s_Strength_00B0A1A8)[iVar4], local_218), …)`** — 512 B のトークンバッファ `local_218` を AV 名72件と照合する**名前解決ループ** |

### 3-1. 【v62 訂正】`FUN_004FDE30` は「全AVダンプ」ではなく**スクリプトコンパイラの引数パーサ**

- `FUN_004FDE30(ushort opcode, int, int, int)` の本文: `param_3` = コンパイル文脈（`+0x40c` = バイトコード長、`+0x20c` = バイトコードバッファ、`+0x204/+0x208` = エラー管理）、`FUN_004FCE30(…, "Expected end of line.\r\nCompiled script not saved!", …)` = **コンパイルエラー出力**。
- **opcode テーブル（base `0xB0AF48`、stride `0x28`、162 エントリ = `GameMode`…`ClearFaceGenModelData`）の `+0x1C` フィールドが全162件で `0x4FDE30`**。全体では stride-0x28 の複数テーブルにまたがり **528 エントリ**（連続 run 162+88+243+35）が `+0x1C = 0x4FDE30`。
- エントリ実測（idx21 `OnLoad`）: `+0x00 name / +0x08 opid=0x15 / +0x18 impl=0x504560 / +0x1C=0x4FDE30`。
- → **`+0x18` = 実行時 native impl、`+0x1C` = コンパイラ側の引数解析関数**。`FUN_004FDE30` は汎用パーサで、AV 名を取るコマンドでは AV 名表を走査してトークンを解決する（E8 caller 0件は**テーブル経由の間接呼び出し**のため当然）。
- v62 の「edi を0..0x48で走査する全AVダンプ」は、この **stricmp ループの走査（0..71）をダンプと誤認**したもの → **訂正**。

### 3-2. 境界走査（abs32 ではないが同一表に触れる）

- `FUN_004FCA70`: `mov esi,0xB0A2CC`（= AV 名表の終端直後）→ `[esi-4]`/`[esi]` を stricmp、`add esi,0x28 / cmp esi,0xB0C89C` で **stride-0x28 テーブル `0xB0A2C8..0xB0C898`（242 エントリ）**を走査、ヒット時 `mov byte [edi+0x204],0x58` + `mov eax,[ebx*0x28+0xB0A2D0]; mov [edi+0x208],eax`（コンパイル文脈へ）→ **コンパイラのキーワード/コマンド名解決系**。
- 注: AV 名表は **72 エントリ（`0xB0A1A8..0xB0A2C8`）**で、その直後に stride-0x28 テーブルが隣接（依頼6 の opcode テーブル `0xB0C8C0` はこの表の後続）。

### 3-3. 判定

**消費者一覧 = 直接3件（上表）＋ 境界走査1件。3件の正体（GetBaseActorValue / GetActorValue のデバッグログ、コンパイラの AV 名前解決）まで確定。** 依頼の「正式な呼び出し元」（v62 §6-3）も解消（`FUN_00501A00` / `FUN_00501670` / opcode テーブル +0x1C 経由）。

---

## 項目4: `0x66C580` の callers 分類 → **[確定] 22件全数・3系統**

### 4-1. 本体（`FUN_0066C580` = this+0x588 bool setter、再確認）

```c
bool __thiscall FUN_0066c580(this, char x) {
  old = this[0x162];                       // +0x588
  this[0x162] = (x == 0);                  // 反転セット（x=1 → 0, x=0 → 1）
  if (x == 0) { if (vcall this+0x198(0)) DAT_00B3BB24 = DAT_00B36B80; }   // 時刻系グローバルのコピー
  if (DAT_00B3BB04 == 0 && this[0x174] != 0) {   // +0x5D0
    DAT_00B14E4D = 1; FUN_0066b710(0); FUN_00664f70(this[0x162] == 0);
  }
  return old == x;                         // 変化したか
}
```

`this` のクラスは `+0x588`（正典 L12040-41: コンソール実行フラグ）/ `+0x589` / `+0x58A` / `+0x5C8` / `+0x5D0` を持ち、**グローバル `[0xB333C4]` の指すオブジェクトと同型**（= v72 の対象クラス）。

### 4-2. 呼出し22件の分類（全数・アドレス→文脈）

**A群: ecx を `[0xB333C4]`（グローバル player オブジェクト）に明示ロード — 12件**

| site | 関数 | 文脈 |
|---|---|---|
| 0x5293A5 | `FUN_00528D90` | `mov dl,[ecx+0x588]`（現在値保存）→ `push ebp(=0)` → call。直後に復元系 |
| 0x5294F0 | `FUN_00528D90` | `sete cl` → `push ecx`（条件反転値を渡す） |
| 0x59E5F3 | `FUN_0059E580` | `ecx=[0xB333C4]` をヌルチェック後、`push sete([esi+0x7C]==0)` |
| 0x5C2B36 | `FUN_005C28F0` | `push sete(...)` |
| 0x5CC17A | `FUN_005CA010`（RaceSexMenu 初化、v62 と同一関数） | `mov byte [esi+0x894],al` 直後 `push ebp` |
| 0x600BCC | `FUN_006005F0` | **`cmp byte [ecx+0x588],0 / jne` でゲート** → `push 0`（フラグが立っている時だけ再設定） |
| 0x6330F4 | `FUN_00633080` | `cmp esi,ecx(=global); jne skip; mov bl,[ecx+0x588]; test bl; jne skip; push 0` — **this==global かつ flag==0 のときのみ** |
| 0x633123 | `FUN_00633080` | `push 1`（条件でスキップ） |
| 0x633179 | `FUN_00633130` | 0x6330F4 と同一パターン |
| 0x633209 | `FUN_006331C0` | 同上（`ret 0xc` の3引数ハンドラ群） |
| 0x656F80 | `FUN_006564C0` | **`movzx edx,byte [ecx+0x588]; push edx` = 現在値をそのまま再適用** |
| 0x656FAF | `FUN_006564C0` | 同上（2回目・別パス） |

**B群: `this` = 同クラスのメソッド引数（`esi`/`ebx`/`ecx` = クラスインスタンス、+0x588/589/58A を直接操作）— 10件**

| site | 関数 | 文脈 |
|---|---|---|
| 0x66C63D | `FUN_0066C600` | `mov byte [ecx+0x58A], dl` 直後 `push 0`。`FUN_0066C600` は `+0x589=1` → `+0x588==0` なら `+0x58A=1` → `FUN_0066C580(0)` |
| 0x66C691 | `FUN_0066C650` | `+0x589` 走査（vcall `+0x2D0`/`+0x2E4`）後、`+0x58A` なら `push 1` → call、`+0x58A=0` |
| 0x66CB26 | `FUN_0066C6F0` | `fstp [0xB3BAC4]`（float 保存）→ `push 1` |
| 0x66CCBD | `FUN_0066CC40` | `mov byte [esi+0x58A],1` → `push ebp` |
| 0x66CEEE | `FUN_0066CEB0` | `+0x58A=1` → `push 0` |
| 0x66CF5D | `FUN_0066CF20` | 同上 |
| 0x6710DA | `FUN_00670F50` | `sete dl; push edx` → call → `cmp byte [esi+0x589],0` |
| 0x671A93 | `FUN_00671620` | `push 1` → `fld [0xB36B60]` |
| 0x671AED | `FUN_00671620` | `push 0` → `fld [0xB3BB24]` |
| 0x673557 | `FUN_00671620` | `push 0` → `call 0x4D5370` |

**C群: ecx の直前ロードが `[0xB333C4]`（A群と同型・null チェックあり）— 0件**（A群に集約済み。残りは全て A/B に分類可能）

### 4-3. 呼出しパターンの型（最終整理）

1. **現在値の保存/復元**（`mov dl,[global+0x588]` … `push dl` 形）— 操作の前後でフラグを維持
2. **現在値の再適用**（`movzx eax,[global+0x588]; push eax`）— 幂等セット
3. **条件反転**（`sete` → push）— 別フィールドの状態の否定を渡す
4. **明示 0/1**（`push 0` / `push 1`、多くは `+0x58A`/`+0x589` の状態遷移とペア）
5. **ガードつき**（`[global+0x588]==0` のときのみ / `this==global` のときのみ）

「linked-list processor 誤認の残滓」= **全22件が `0xB333C4` クラスの bool setter 呼出しであり、linked-list 処理は一切含まれない**ことをもって解消。

---

## 残点（本依頼の範囲外・記録のみ）

1. `FUN_004FDE30` の `+0x1C` フィールドを参照する残り366エントリ（run2-4: 88+243+35）のテーブル境界の厳密な同定。
2. `FUN_0066C580` の vcall `+0x198` と float グローバル `0xB3BB24/0xB36B80/0xB36B78` の意味（時間系かポーズ系か）。
3. owner（0x154 B）の**クラス正式名**（RTTI が取得できない非多形のため、Oblivion 一次資料のクラス名との突合は未実施）。
4. formType 0x21/0x22/0x1A の**クラス名**（ctor vtable 0xA35968/0xA35908/0xA35800 は RTTI COL 不在 → 名前は未特定。装備スロット文脈から武器/防具/その他と推定）。

## 証跡（すべて `%TEMP%\opencode\`）

| スクリプト | 内容 |
|---|---|
| `v283_p1.py` | ESM×Description 突合・FUN_00478780 全文+caller24件・0xB0A1A8 参照・0x66C580 caller22件 |
| `v283_p1b.py` | ESM DATA サイズ・gap 周辺 aligned disasm・formType ctor 候補・RTTI 走査 |
| `v283_p1c.py` | formType ctor decompile・幅広 byte store 走査（0x10..0x40）・`[reg+0x150]` family・FUN_006600D0 callers38件 |
| `v283_p1d.py` | RTTI(via vtable-4)・`+0x104/+0x150` 書手60/19関数・FUN_00478780 呼出し窓 |
| `v283_p1e.py` | owner ctor/helpers decompile・owner band vtable store・new サイズ |
| `v283_p1f.py` | owner ctor callers3件（`new(0x154)` 実測）・`push 0x154` 16件・FUN_00478070・setter callers・`0xB33C80` refs |
| `v283_p1g.py` | ボーン名テーブル `0xB06550` 全9件・`0xB333C4` 書手3件・AV 境界参照・consumer decompile |
| `v283_p1h.py` | `FUN_0066C580` 本体・残 ecx 源5件・間接 ptr 走査・typo 鍵登録 refs |
| `v283_p1i.py` | opcode テーブル entry 型（+0x18/+0x1C）・`0x4FDE30` .data 参照528件の run 分析 |
| `v283_p1j.py` | decompiled の AV 参照帰属3件・テーブル162エントリ +0x1C 分布（162/162 = 0x4FDE30） |
| `v283_p1k.py` | EXE 既定値（12件の value push 実測）・`gmst_group` が返すのは DATA サイズ |
| `v283_p1l.py` | `FUN_004A7D60` の DATA 型分岐・`FUN_004A78D0`（'s'→6/'f'→5/'i'→3）・`FUN_00450C20` |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。内部整合: ESM 382件 errors=0 / EXE 登録12/12 / abs32 消費者3件（v62 と一致）/ 0x66C580 caller 22件（v69 の "~22件" と一致）/ opcode +0x1C 162/162。
