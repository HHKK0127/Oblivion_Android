# V139: b17 §4 第2テーブル残点束 7 件の裁定

- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000、7,898,624 B）
- 出典: `tmp/re/b17_table2_identity.md` §4（V113 完了行の残留 7 点）
- 作成: 2026-10-27 / 手法: capstone 5.0.7 (skipdata=True) + `docs/Oblivion_decompiled.c` 部分検索 + 既存資産突合
- 位置づけ: リバースエンジニアリング専任成果物（docs 編集なし）。**本稿は「中間スクリプト実行 → 即書込み → 追加検証 → 追記」方式で作成、§6 に追記あり。**

## §0 着手時検証・スクリプト

| 項目 | 結果 |
|---|---|
| 成果物の事前存在 | **不在**（新規作成） |
| `probe_harness.py selftest` | **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560） |
| 中間スクリプト | `%TEMP%\opencode\v139_1.py` → `v139_1.txt`（約2,258,000命令の全 .text 1パス走査＋範囲逆アセンブリ） |
| 追加検証 | `%TEMP%\opencode\v139_2.py` → `v139_2.txt`（§6） |
| 主要入力資産 | `b17_table2_identity.md` / `b10_expression_getters.md` / `b17_table2_out.txt`(5,655B, mtime 10/26 0:05) / `b07_vtable_index.md` / `v138_b16_alarm_residuals.md` / `v121_gt_corrections.md` / `s72_navm.md` / `s65_ai_follow.md` / `s80_particle_startup_loop.md` / `vtable_map.txt` |

## §1 判定表（7点）

| # | 残点 | 判定 | 要旨 |
|---|---|---|---|
| (1) | `0xB333A0` の正体 | **解明（訂正あり）** | `0xB333A0` = **クラス `TES` のグローバルインスタンス**（vtable `0xA37470` = `.?AVTES@@`）。s65/s72 の「NavMesh 管理オブジェクト」[推定] は**誤り**。残2呼出しも同定済み（§2-1） |
| (2) | `ref+0x44` の型 / type 0x3F か 0x41 か | **解明** | `ref+0x44` = **BaseExtraList**（form 系共通）。**有効状態親 = type `0x3F`(63) = `ExtraEnableStateParent`**（三重裏付け）。type `0x41`(65) = **`ExtraItemDropper`**（別物）（§2-2） |
| (3) | bit14 (`0x4000`, setter `0x46B590`) の名称 | **機構解明・名称は暫定** | setter は「グローバル bit1/bit14 配列から登録解除 → `[form+8] |= 0x4000`」。**実行時 clear 書込みは .text に不在（set-only）**。名称候補 = **Deleted（削除済み）**。19 呼出しの所属確定待ち → §6 追記（§2-3） |
| (4) | `FUN_004e2920` の削除対象 `esi` | **解明** | `FUN_004e2920` = **`TESObjectREFR::~TESObjectREFR()`（完全 dtor）**、`esi` = **`this` = 破棄対象自身**。`push esi; call 0x4FA540` は **self を保留集合から除去**（子走査は EDI の別ループ）。`0x9C3C13` = **入向 thunk `jmp 0x4e2920`**（方向は逆）（§2-4） |
| (5) | b10「cache 0xB361xx」混称 | **裁定: 混称は b10 側になし（b17 §4-6 の推測が誤り）** | b10 表の 162 行中 **保留集合 `0xB361CC/D0` を直接参照する行 = 0 件**。`0xB361xx` は b10 §5.3 既出の {`0xB361AC` トレースフラグ, `0xB36180..C4` 自己メモ化} のみ。GetDisabled 行さえ `0xB361CC` 直接参照なし（間接: cond 内 `call 0x4FA560`）（§2-5） |
| (6) | Disable 直後〜flush の 1 フレーム挙動 | **解明（機構）＋残点1件** | 即時: GetDisabled（flag OR 集合）・AI 対象選別 `FUN_0065fda0`（flag と集合の両方で除外）。非即時: 集合読手は全体で上記2件のみ → **描画・経路探索は集合を参照せず、flush（フレーム末）まで実体は生きている**（§2-6） |
| (7) | `b17_table2_out.txt` 表示バグの記録整合 | **解明（raw 値は維持・該当表示は再現不能）** | raw 再読: `[0xB0D708]=0x00000001`（row254 col4）/ `[0xB0D6E0]=0x00010001`（row253 col4）✓。現行ファイルに row254 の誤表示は**不在**（0x00010001 表示は全て正しい行）。b17 の採用値は正しい（§2-7） |

## §2 各点の詳細

### 2-1. (1) `0xB333A0` = クラス `TES` のグローバル（s65「NavMesh 管理オブジェクト」は誤り）【解明・訂正】

**ホストオブジェクトの同定（新規証拠）**
- `FUN_00441970`（thiscall コンストラクタ）`0x44199E`: `mov dword [esi], 0xa37470` — **ホストの vtable = `0xA37470`**。
- `vtable_map.txt:199` / `b07_vtable_index.md` #1252: `0x00A37470 → .?AVTES@@ → クラス TES`（RTTI 実測一致）。
- 生成経路（`0x406600` 領域、実測）: 引数 `push 0xa2ffb8`（.rdata 文字列）/ `push ebp` / `push esi` / `push [0xB33B00]` → `mov ecx,eax; call 0x441970` → **`0x40663F: mov [0xB333A0], eax`** → `mov ecx,eax; call 0x43f560`（サブオブジェクト初期化 = s72 記載の init と同一）。**生成は1回のみ**（`call 0x441970` = 1件）。
- `FUN_00441970` は `+0x78/+0x7C/+0x80/+0x84/+0x88..+0xA4` をゼロ化し `0xAC` バイトの初期化を伴う（= リスト群の初期化）。`+0x78/7C` は head/tail リスト、`+0x80` は `TESObjectREFR` dtor が `add ecx,0x80; call 0x65c620`（リスト erase）で参照を外す登録リスト、`+0x88` は flush が解放する保留リスト。

**s65/s72「NavMesh 管理オブジェクト」説の検証 → 却下**
- `s72_navm.md` S1 の根拠は「`0x6860F0` の経路探索コードが `esi = [0xB333A0] + 0x78`（セル座標照合リスト）を走査」「init が `0x40663F` で書く」だったが、**読み手が NavMesh 管理でも World 管理でも構わない**。全 .text 参照の実態は:
  - 参照総数は数百件（`mov ecx/eax/edx,[0xb333a0]` の thiscall 受けが绝大多数）で、**全フレーム・全シーン操作がこのオブジェクトを this にする**（例: `0x40DB64/0x40DB9F/0x40DBD6` = フレーム末処理、`0x4E2A44`（REFR dtor 内 `call 0x4416d0`）、`0x67875B`（HavokContext subObj68））。
  - `+0x20/+0x24` = グリッド座標、`+0x34` = 「セル内/トランジション中」判定用ポインタ（`FUN_0043FD20` の「適用対象セル数」= `+0x34!=0 → 1`、v128 実測）、`+0x74` = 現在空間/セル ptr（`FUN_004431f0` 切替、v128）、`+0x80` = 参照登録リスト — **これは `TES`（＝ゲーム世界の最上位状態オブジェクト）のレイアウト**であり、NavMesh 専用オブジェクトではない。
  - `s80_particle_startup_loop.md:507`「`0xB333A0` = World object pointer」および `s69_wrld.md:204`「SceneGraph」は方向として正しい（クラス名は `TES`）。
- **訂正**: `s65_ai_follow.md:212`「NavMesh 管理オブジェクト [推定]」・`s72_navm.md` S1「NavMeshManager Global [Def]」・`TODO_re49.md:87`「NavMesh 0xB333A0」はいずれも **誤り（NavMesh/経路探索側は TES+0x78 のセルリストを参照しているだけ）**。`s68_n6_detect.md:84`「`[0xB333A0]=playerRef`」も誤り（player は `0xB333C4`、b10 §5.1 と整合）。

**残る2呼出しの同定（捷径達成）**

| 呼出し | 関数 | 同定 |
|---|---|---|
| `0x444E80` | `FUN_00444c70`（thiscall, `param_1`=TES, `param_3`=切替種別） | **セル/空間トランジション（移動）ハンドラ**。2本のサブオブジェクト（`this+8`/`this+4`）へ vtable+0x10(x,y) 通知、`FUN_004430f0`/`FUN_00444340`/`FUN_004ef7e0`（ESM 系）、`DAT_00b333c4` vtable+0x174 → `FUN_004ea6e0`、切替種別==0 では位置起点の再設定（`FUN_0043fff0`/`FUN_0043fc20`/`FUN_00434020(4)`/`FUN_004ebda0`/`FUN_00441610`/`FUN_00678750` ほか）→ **両分岐の末尾で必ず flush**。唯一の呼出し = `0x444F70`（`FUN_00444ec0` 内、座標 `float*` をグリッド座標 `>>0xC` へ変換する「セル切替入口」関数）。flush 直前は `mov ecx, esi`（= this = TES）で、**`[0xB333A0]` の直接ロードではない**（b17 §2-7 の「3件とも `mov ecx,[0xB333A0]`」は 2件 direct + 1件 this 経由に**細部訂正**。実体は同じ TES オブジェクト）。 |
| `0x5D7225` | `FUN_005d7090`（グローバル関数、SEH 付） | **メインループ内（`0x40DEEA` 呼出し = `call 0x5d7090` は1件のみ）の定期メンテ処理**。ガード: `FUN_00589b70(0x3f4)` チェック、`DAT_00b3b724` のタイマ減算、`FUN_005898f0()` 結果の dynamic cast（typeinfo `0xB0A018`→`0xB132E0`）成功 → 内部で `FUN_004402f0()`（flush）+ `FUN_0065f770` + `FUN_0058ceb0(0xfb3/0xfa1,…)`（メッセージ出力系）。flush 直前は `mov ecx,[0xb333a0]`（**direct 経路**）。 |

→ flush 3経路の性格: **毎フレーム末（`0x40DBDC`）／セル切替時（`FUN_00444c70` 末尾）／定期メンテ（`FUN_005d7090` 内・条件付き）**。いずれも this/`ecx` は `TES`。

**`FUN_004402f0` 本体の構造（decompiled L49502-49564 再確認）**
1. reent ガード `DAT_00b33a58` + `FUN_004f9fa0()` 成功時のみ実行。
2. `DAT_00b33a98 + 0x84` のリスト巡回（要素 `+0x3C` bit0 が立つもの → `FUN_00529aa0`）。
3. `this+0x34 == 0` なら `[0xB06A2C]`(uGrids) の二重ループで `FUN_00482150(x,y)`（グリッドセル取得）→ `*slot != 0 && *(char*)(*slot+0x26) == 6` → `FUN_004cb8c0(0,0)`。非0なら即 `FUN_004cb8c0(0,0)`。
4. `this+0x88` の保留リストを一旦差し替えて各要素へ `FUN_004d7190` → ノード解放。
5. **`FUN_004fa580()`（保留集合を各要素へコア無効化 `0x4E4690` 適用後に全解放）** → `DAT_00b33a58 = 0`。

（残留: `FUN_004f9fa0`/`FUN_00529aa0`/`FUN_004cb8c0`/`FUN_004d7190` の命名は §3 V143-c）

### 2-2. (2) `ref+0x44` = BaseExtraList、**有効状態親 = type `0x3F`**【解明】

- `ref+0x44` = **BaseExtraList**（`v138_b16_alarm_residuals.md` §(5) と一致）。構造（本作業 実測）:
  - list head `+4`、**presence bitmap `+8..+0x13`（96 bit = type 0..0x5B、`cmp esi,0x5c; ja`）**、探索は `0x41E210`（`BaseExtraList::GetExtraData(type)`、per-TLS キャッシュ＋文字列 `0xA34E5C`）。
  - ノード: `+0` = vtable、**`+4` = type byte**、`+8` = next、`+0xC` = payload、`+0x10` = 付帯バイト/ワード。
- **type `0x3F` = 有効状態親**（三重裏付け）:
  1. `FUN_00420260` = `push 0x3f; call 0x41e210; → [node+0xC]`、および **`FUN_004d7990` = `add ecx,0x44; jmp 0x420260`** — Disable/Enable が親有無を問う経路そのもの（b17 §2-7）。
  2. ノード ctor `FUN_0042a5e0`: `mov byte [eax+4], 0x3f` + `mov dword [eax], 0xa3598c` → `0xA3598C` = **`ExtraEnableStateParent`**（b07 #670）。
  3. decompiled L171548: `FUN_0041e210(0x3f)` → `FUN_009832e6(..., &PTR_PTR_00b03850, &PTR_PTR_00b04024, 0)` = dynamic_cast、**typeinfo `0xB04024` = `.?AVExtraEnableStateParent@@`**（b07 #670 typeinfo 列と一致）。
  - 付帯 API: `FUN_00420280(v)` = 親の設定/解除（0 で `0x41e160(0x3f)` 削除）、`FUN_00420340/0x420360/0x420380` = `node+0x10` バイトの get/set/clear（親フラグ）。
- **type `0x41`(65) は有効状態親ではない**:
  - ctor `FUN_0042a840`: `mov byte [eax+4], 0x41` + `mov dword [eax], 0xa35a04` → `0xA35A04` = **`ExtraItemDropper`**（b07 #682）。`FUN_004203c0` = `node+0xC` 読取（dropper 参照）、`FUN_004203e0` = 設定/削除（ctor `0x42A840`・node 0x10B）。
  - `TESObjectREFR` dtor 内の利用（`0x4E2A00`-`0x4E2A0D`）: `FUN_004203c0()` → 非0なら `push self; lea ecx,[eax+0x44]; call 0x424c00` = **破棄時に dropper 側の BaseExtraList から自分を外す**。
- 併せて同定した隣接 type（新規）:
  | type | ctor | vtable | クラス（b07） | payload |
  |---|---|---|---|---|
  | `0x3F`(63) | `0x42A5E0` | `0xA3598C` | ExtraEnableStateParent (#670) | `+0xC` = 親 REF、`+0x10` = フラグ |
  | `0x40`(64) | `0x42A600` | `0xA35998` | **ExtraEnableStateChildren** (#669) | `+0xC` = 子リスト head（`FUN_004203a0` は `node+0xC` を返す） |
  | `0x41`(65) | `0x42A840` | `0xA35A04` | **ExtraItemDropper** (#682) | `+0xC` = dropper REF |
  | `0x42`(66) | `0x42A860` | `0xA35A10` | **ExtraDroppedItemList** (#666) | `+0xC` = ドロップアイテムリスト head（`FUN_00420480`） |

### 2-3. (3) bit14 (`0x4000`) — 機構は確定、名称は暫定【機構解明・名称保留 → §6 追記】

**setter `FUN_0046b590` 本体（実測）**
```
0x46B590: push esi / mov esi,ecx
          call 0x46b2c0                 ; (A) 登録解除プロローグ
          mov ecx,[0xB33B00] / test ecx,ecx / je skip
          push 0 / push esi / call 0x45b780   ; (B) [0xB33B00] 有り時のみ (form,0) 通知
0x46B5AA: or dword [esi+8], 0x4000       ; (C) form flags へ設定
          pop esi / ret
```

**(A) `FUN_0046b2c0`（実測 0x46B2C0..）**
- `eax=[esi+8]; shr eax,0xe; test al,1; jne → 即 return`（**bit14 既立なら何もしない＝二重登録/二重解除ガード**）。
- `ecx=[esi+0xC]`（formType byte）→ `push ecx; mov ecx,0xB0613C; call 0x4524C0`（型別クエリ）→ `[0xB06158]`（個数）/[0xB06150]（配列）を線形走査して **`esi`（=form 自身）を検索** → 一致時 `0x46B2FE..` で除去処理。
- この配列 `[0xB06150]×[0xB06158]` は **`v121_gt_corrections.md:57-59` で「bit1/bit14 グローバル配列」と既命名済み**: `FUN_0046c640`（vt+0x90）が「未登録なら `0x449070` で `[0xB0614C]` へ挿入 → `or [esi+8], 2`」＝登録して bit1 を立てる側。**setter はその逆（登録解除してから bit14 を立てる）**。
- 呼出し3件: `0x46B593`（本 setter）、`0x46C289`（`FUN_0046c280` = `TESObjectREFR` dtor 末尾で呼ばれる base dtor 系）、`0x45C817`。

**bit14 が絡む既知経路（総括）**
| 経路 | 挙動 | 出典 |
|---|---|---|
| Disable `FUN_0050A240` / Enable `FUN_0050a150` | bit14 立ちは**素通し（無視）** | b17 §2-7 |
| `TESObjectREFR` dtor `FUN_004e2920` | 入口で `shr 0xe; test; jne 0x4e2ada` → **bit14 立ちは world デタッチ一式＋保留集合除去を全部スキップ** | 本作業 実測 0x4E2975-0x4E298D |
| vt+8 `0x46B800`（フォーム比較/同期ヘルパ） | 引数フォームの **bit14 を this へ同期**（formType と同時にコピー） | v121 §1.1 |
| vt+0x90 `FUN_0046c640` | bit1/bit14 グローバル配列へ登録＋bit1 設定 | v121 §1.1 |
| setter 19 件 | `call 0x46b590` = **19 件**（E8 直走査、b19 §… call counts と一致） | 本作業 + b19:185 |

- **実行時 clear は不在**: `and dword [x+8], 0xFFFFBFFF` 形式の書込みは全 .text に **0 件**（clear 系インスタンスは `[esi+0x18]`/`[ecx+0x1C]`/`[edi+0x1F4]` 等の**別フラグ空間**のみ）→ bit14 は実行時に外せない **set-only**（＝「削除済みは復帰しない」の性格）。
- **名称判定は保留**: 最有力は **Deleted（削除済みフォーム）**（Disable/Enable 不可・dtor スキップ・型別レジストリからの除去・set-only の4点が整合）。ただし `FUN_00683030`（WinMain 由来の1件、v128 実測）等、初期走査系を含む19呼出しの所属未同定のため**暫定**とし、§6 追記で確定を図る。

### 2-4. (4) `FUN_004e2920` の削除対象 `esi` = **self（破棄対象自身）**【解明】

- decompiled は 45 行目で途切れていたが、**本作業で 0x4E2920..0x4E2BE0 を全逆アセンブル**（`v139_1.txt` [P4]）。
- `0x4E2946: mov esi, ecx`（esi = this）→ `0x4E2A3E: push esi / 0x4E2A3F: call 0x4fa540`（保留集合 erase、arg = v）。**つまり削除対象 = 破棄中の TESObjectREFR 自身**。
- 「子走査中の別要素」説の検証 → 却下: 子走査は **EDI レジスタ**の別ループ2本で、どちらも `push 0`（＝親/リスト項目の解除）を渡す:
  - `0x4E29D3` `FUN_004203a0`（type 0x40 = 子リスト）→ 各子 `lea ecx,[eax+0x44]; call 0x420280(0)`（子の親ポインタ解除）。
  - `0x4E2A14` `FUN_00420480`（type 0x42 = dropped item list）→ 各子 `lea ecx,[eax+0x44]; call 0x4203e0(0)`（子の dropper 解除）。
  - ESI を使うのは自己参照（`push esi`）のみ。
- **文脈（この関数が何か）**: `0x4E294C: mov [esi], 0xa46c44`（= **TESObjectREFR** primary vtable, b07 #1440）＋ `0x4E2952: mov [esi+0x18], 0xa46c38`（TESObjectREFR secondary vptr, b07 #1441）。入口の bit14 ゲートの後、world デタッチ一式（`0x4e1f80` / `0xB3BD00` イベント dispatcher / `0x60df00` / `[0xB33B00]→0x45a300` / 親・子・dropper の BaseExtraList 整理 / **保留集合から self 除去** / **`mov ecx,[0xb333a0]; push esi; call 0x4416d0`（TES から自分を外す）** / `[0xB33A98+0xCD4]` ガード下の処理 / flag bit22 時に player+0x780 リストから erase / base formType==0x20 時に **`TES+0x80` リストから erase**）→ 共通末尾（`0x46a9e0(1)` / `0x4d9b50` / メニュー通知 / `0x422f00` = BaseExtraList dtor / `0x46c280` = TESForm 系 dtor）。
- **`FUN_004e2920` は「構築/リセット系」ではなく destructor**（b17 §2-7 の記述を**訂正**）。呼出し構造:
  | 呼出し | 所属 | 意味 |
  |---|---|---|
  | `0x4E4923 call` | `FUN_004e4920` = **deleting dtor**（`FUN_004e2920` → `if (arg&1) FUN_00401f20(free)`） | TESObjectREFR の vtable dtor エントリ本体 |
  | `0x65A02B call` | `FUN_00659f90`（`0x659F90..0x65A050`、vtable `0xA732FC` = **MobileObject**, b07 #830） | MobileObject 系 dtor がメンバー破棄後に基底 TESObjectREFR dtor へ連鎖 |
  | **`0x9C3C13 jmp`** | **独立 thunk**（`jmp 0x4e2920`） | **「疑似 jmp 0x9C3C13」の正体 = 0x9C3C13 側から FUN_004e2920 へ飛ぶ入向 jmp**。`FUN_004e2920` から出て 0x9C3C13 へ飛ぶコードは**存在しない**（全 .text 走査で `op_str` に `9c3c13` を含む命令 = 0 件）。登録先（vtable/call xref）は §3 V143-b |

### 2-5. (5) b10「cache 0xB361xx」混称 — **裁定: b10 は正しい、混称は b17 §4-6 の推測側**【解明】

**検証（`v139_1.py` [P5]）**: b10 の162行表から `(impl, cond)` を全抽出し、両関数を境界付きで走査して `0xB361xx` 系参照を全数集計。

| 検査 | 結果 |
|---|---|
| b10 行数 / `cache 0xB361xx` タグ行 | 162 行 / **159 行** |
| **`0xB361CC` または `0xB361D0`（保留集合）を参照する行** | **0 件 / 162** |
| `0xB361AC` を参照する行（impl+cond 合算の本作業実測） | **162 / 162** |
| 自己メモ化スロット `0xB36180..0xC4` を併用する行 | 26 行（GetInCell / GetDisposition / GetInFaction / GetIsSex / GetIsPlayableRace 等） |
| GetDisabled 行（idx13, opid 0x1023）の参照 | **`0xB361AC` のみ2箇所**（`0x4F6128`, `0x4F6179` = `cmp byte [0xb361ac],0` トレース）。集合は **cond 内 `call 0x4FA560`（間接）** で参照 |

**GetDisabled cond `0x4F60E0` 実測（＝flag‖set の決定的証拠）**
```
0x4F60F1: mov ecx,[eax+8] / shr ecx,0xb / test cl,1 / jne → 1.0        ; flag bit11
0x4F60FC: push eax / call 0x4fa560 / test al,al / je → 0.0 else 1.0     ; 保留集合所属
0x4F6128: cmp byte [0xb361ac],0 / je / push 0xa48e18 / call 0x579b60    ; トレース出力
```

**裁定**
1. `b10_expression_getters.md` の行表「cache 0xB361xx」は **b10 §5.3 で既に正体を明記済み**（`0xb361ac` = トレース有効フラグ 159 行 / `0xb36180..0xC4` = 自己メモ化トリプレット）。**b10 表が保留集合 `0xB361CC` を指すことは162行中0件で、混称は生じていない**。
2. b17 §4-6 の推測「GetDisabled 行以外はほぼ `0xB361AC` 指」は**方向として正しいが不正確**: **GetDisabled 行も `0xB361CC` 直接参照ではない**（`0xB361AC` のみ）。集合を直接触るのは GetDisabled の **cond が呼ぶ `0x4FA560`（本体が `0xB361CC` を読む）** と `FUN_0065fda0` 内 `0x65FE00` の2系統だけ。
3. 「`0xB361AC` = 249件」は b17 記載と**完全一致**（本作業の全 .text 走査: 総 **249** = `cmp byte [0xb361ac],0` 243 + `cmp [0xb361ac],bl` 2 + write 4）。
4. **b10 の159タグ vs 本作業162の差3行**は、b10 のタグ付与が impl/cond のどちらか片方にしか及ばなかった可能性 → §3 V143-f。

### 2-6. (6) Disable 直後〜flush 到達までの 1 フレーム内挙動【機構解明＋残点1】

**保留集合の読手は全体で2件のみ**（`call 0x4FA560` 全走査 = 2件、`0xB361CC` 直接参照8命令はいずれも集合 API 自体）:

| 読手 | 保留中の扱い | 即時性 |
|---|---|---|
| GetDisabled cond `0x4F60E0`（script 条件, opid 0x1023） | `flag bit11 OR 集合所属` → **true** | **即時** |
| `FUN_0065fda0`（`param_1+0x704` リスト走査、最寄り候補選別 → `DAT_00b3bad0`） | `(((elem+8)>>0xb)&1)==0 && !集合所属` を満たす要素のみ処理 → **即除外** | **即時** |
| それ以外（描画・経路探索・その他） | **集合を参照しない**（読手ゼロ）→ flag bit11 は未設定 = **有効扱いのまま** | flush まで非即時 |

**フレーム内タイミング（`s80_particle_startup_loop.md` §C 実測、`0x40D800` PerFrameUpdate）**
```
24. 0x40DBA5 call 0x440400  ; World::Update   ← スクリプト Disable 実行はここ（集合へ dedup 挿入のみ）
25. 0x40DBB4 call 0x6AE860  ; Render::Frame   ← flag 未設定・集合非参照 → まだ描画される
26. 0x40DBD1 call 0x4F5DB0
27. 0x40DBDC call 0x4402F0  ; flush = 集合全要素へ 0x4E4690（flag bit11 設定＋vtable+0x90/+0x40 通知＋+0x188/+0x190 脱離）→ 集合クリア
28. 0x40DBEC call 0x674A20  ; EndFrame / present
```
- **結論**: Disable された参照は**同一フレームの Render::Frame まで描画され続ける**（描画系はフラグだけ見るため）。スクリプト条件（GetDisabled 等）は插入と同時に true、AI の対象選別（`FUN_0065fda0` 系）も即時除外。**経路探索は集合を参照しない**（NavMesh/pathing 系に `0x4FA560` 呼出し・`0xB361CC` 参照なし）→ 保留中は従来どおり扱われる。
- flush は他に **セル切替時**（`FUN_00444c70` 末尾）と **定期メンテ**（`FUN_005d7090` 内・条件付き）でも実行されるため、保留は「最長1フレーム＋α」で、**セル移動やメンテタイミングでは即時強制適用**される。
- 適用本体 `0x4E4690` は「bit11 既立ならスキップ／未立なら `vtable+0x90(1)` → `vtable+0x40(0x40000000)` → `0x46ABA0(this,1)`（bit11 設定）→ `vtable+0x188/+0x190`（脱离）→ 有効状態子へ伝播」（b17 §2-7 と一致）→ **シーングラフ反映は flush 時に初めて走る**のが描画1フレーム遅れの機構的原因。
- **残点**: `World::Update` 内部での「スクリプト実行 vs AI tick」の前後関係、および flag bit11 のみを見る描画側 IsDisabled 相当ルートの列挙（§3 V143-d）。

### 2-7. (7) `b17_table2_out.txt` の表示バグ記録 — **raw 値は維持、該当表示は現行ファイルで再現不能**【解明】

**raw 再読（本作業、`v139_1.py` [P7]）**

| VA | bytes | 値 | 意味 |
|---|---|---|---|
| `0xB0D708` | `01 00 00 00` | **`0x00000001`** | main row254 col4（=flagA\|argn） |
| `0xB0D6E0` | `01 00 01 00` | **`0x00010001`** | main row253 col4 |

→ b17 §2-4 の「raw 再読2本はいずれも `0x00000001`」「`[0xB0D6E0]` = row253 = `0x00010001`」と**完全一致**。

**現行 `b17_table2_out.txt`（5,655 B、mtime 2026/10/26 0:05:14）の全表示を精査**
- `== main rows idx 250..258 raw ==` セクション: **`idx254 ... 0x00000001`（正しく表示）**。`0x00010001` を含む raw 行は `idx253` / `idx255` / `idx257` の3行のみで、いずれも生値と一致（＝正しい表示）。
- T2 dump（`0xB0D6E8` 起点）内の `0x00010001` は L12（`0xB0D710` rec1 の9番目 = **main row255** の flagA）と L19（`0xB0D828` rec8 = **main row262** の flagA）— どちらも正しい値。
- → **b17 §2-4 が記した「main row254 raw が col4 を `0x00010001` と表示」という箇所は現行ファイルに存在しない**。当時の観測は「row253 行（`idx253`、`0x00010001` を正しく表示）を row254 と読んだ」または「T2 dump L12 を main row254 の再構成と誤読した」のいずれかと判断（行ラベル off-by-one の旧観測）。
- **裁定**: (a) 採用値 `0x00000001` は raw 2系統＋byte一致式 243/243（b17 §5 検証4）で**正しい**。(b) 「表示バグ」という記録は**現行ファイルでは再現不能**であり、b17 §2-4 の該当1文は「row253 の `0x00010001` が混入表示された」という趣旨の観測記録として**実体と不一致**→ 文書側は raw 値（`0x00000001`）を採用済みのため**訂正要求なし**、表現のみ要精査（§3 V143-g）。
- b11 への波及: `b11_event_bit_setters.md` に `0x00010001`/`0x00000001` の当該行記載なし（§6 で確認）。

## §3 新規残点（V143+ 粒度）

| ID | 残点 | 優先度 |
|---|---|---|
| V143-a | **bit14 の名称確定**: setter `0x46B590` の19呼出しの所属（enclosing）確定＋`.rdata` 文字列 xref（"Deleted"/"Temporary" 等）＋`[0xB0613C]`/`[0xB0614C]`/`[0xB06150]`＝「bit1/bit14 グローバル配列」の用途（save ChangeForm 系か ESM load 系か） | 中 |
| V143-b | `0x9C3C13`（`jmp 0x4e2920` thunk）の登録先（vtable エントリ or 呼出し xref）と役割 | 低 |
| V143-c | `FUN_004402f0` 内部の残り4関数の命名（`FUN_004f9fa0` ガード / `FUN_00529aa0` / `FUN_004cb8c0` / `FUN_004d7190`）、`FUN_00444ec0`（セル切替入口）の上流呼出し元、`FUN_005d7090` の文脈命名 | 中 |
| V143-d | 1フレーム挙動の残り: `World::Update` 内のスクリプト実行と AI tick の前後関係、flag bit11 のみを見る描画側「IsDisabled 相当」ルート（vtable slot の実装列挙） | 中 |
| V143-e | `0xB361AC` トレースフラグの書き手4件（`0x4FA529`/`0x4FBAF8`/`0x4FBF28`/`0x4FBF48`）の意味と `0x579B60`（trace print、書式 `0xA48E18`/`0xA48740`）の出力先 | 低 |
| V143-f | b10 の `cache` タグ159 vs 実測162の差3行（タグ付与規則の補記）——b10 は既存ファイルのため**記録のみ**（編集しない） | 低 |
| V143-g | b17 §2-4 の「表示バグ」1文の表現精査（現行ファイルで再現不能な観測記録の扱い）——b17 は既存ファイルのため**記録のみ** | 低 |

## §4 使用証拠一覧

- **バイナリ実測（`v139_1.py` / `v139_1.txt`）**: 全 .text 2,257,988 命令の1パス走査（mem ref watch: `0xB333A0`/`0xB361AC`/`0xB361CC`/`0xB361D0`/`0xA3598C`/`0x9C3C13`、call/jmp watch 20 目標、bit14 mask `0x4000`/`0xFFFFBFFF`）、範囲逆アシンブル（`FUN_004402f0` 周辺・`FUN_00441970`・`0x40663F`・`FUN_0046b590`・`FUN_0046b2c0`・`FUN_004e2920` 全体・extra ctor 群・`GetDisabled` cond/impl・`GetDistance` cond・vtable+0x198 実装）
- **raw 読取**: `[0xB0D6E0]`/`[0xB0D708]`/`[0xB0D6E4]`/`[0xB0D70C]`、`b17_table2_out.txt` 全文（84行）
- **decompiled 部分検索**: `FUN_004402f0`@49502, `FUN_00444c70`@53122, `FUN_005d7090`@340302, `FUN_0046b590`@84960, `FUN_00420260/0280/0340/0360/0380/03a0/03c0/03e0/0480`@22705-, `FUN_004e2920`@177959, `FUN_004e4920`@179125, MobileObject dtor 系@419745, `FUN_0065fda0`@424124, `FUN_004fa560`@193971, GetDisabled 動的キャスト@171548
- **既存資産**: `b07_vtable_index.md`（#666/#669/#670/#682/#830/#1252/#1440/#1441 の vtable↔class）、`vtable_map.txt:199`（`.?AVTES@@`）、`v138_b16_alarm_residuals.md` §(5)（BaseExtraList 構造・type 名前空間）、`v121_gt_corrections.md` §1.1（vt+8 同期・`FUN_0046c640`「bit1/bit14 グローバル配列」）、`v128_winmain_tail.md`（`FUN_00683030` 内 `FUN_0046B590` 呼出し・frame 1 フレーム処理）、`b19_quest_byte_writer.md:185`（`0x46B590`=19 件の独立計測）、`s80_particle_startup_loop.md` §C（frame 24/25/27/28 順序）、`s65_ai_follow.md`/`s72_navm.md`/`s69_wrld.md`/`s68_n6_detect.md`（誤帰属の出所）、`b10_expression_getters.md` §5.1/§5.3（cache 列の定義）

## §5 自己検証

- `probe_harness.py selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 内部整合（本作業の再計測）:
  1. 全 .text 1パス走査2回（`v139_1.py` / `v139_2.py`）で `0x4402f0` 呼出し = **3件**（`0x40DBDC`/`0x444E80`/`0x5D7225`）、`0x4FA560` = **2件**、`0x4FA540` = **2件**、`0x4FBB30` = **1件**、`0x4FA580` = **1件**、`0x46B590` = **19件** — b17 §5 および b19:185 の独立計測と全一致。
  2. `0xB361CC` 直接参照 = **8命令**（読4/書4）、`0xB361D0` = **2命令** — b17 記載と一致。`0xB361AC` = **249件**（cmp 243 + cmp bl 2 + write 4）— b17 §4-6 の 249 件と一致。
  3. `0xB333A0` 参照 = **557件**、うち書込みは `0x40663F` の **1件のみ**（比較1件 `0x44EC93` を除く）→ 「起動時に1回だけ構築」を裏付け。
  4. raw 再読: `[0xB0D708]=0x00000001` / `[0xB0D6E0]=0x00010001`、`b17_table2.json` rec0 `fields[8]='0x00000001'` — 3系統一致。
  5. decompiled 関数ヘッダ 20,465 件を正規表現 `@ (0x)?hex` で再パース（初回は `0x` 前缀を落とすバグで全件 addr=0 → enclosing が壊れた。`v139_2.py` で修正・再実行済み。`v139_1.txt` 内の `; FUN_00baf2db` 表記はこのバグの痕跡なので本稿には採用しない）。

## §6 追加検証の追記（`v139_2.py` / `v139_3.py` → `v139_2.txt` / `v139_3.txt`）

### 6-1. (1) `0xB333A0` = `TES` — 追加確定と呼出しチェーンの厳密化

- **RTTI 実測（vtable[-4]=COL → TD+8 内包名、`v139_2.py` [A]）**: `0xA37470=.?AVTES@@` / `0xA46C44=0xA46C38=.?AVTESObjectREFR@@` / `0xA732FC=.?AVMobileObject@@` / `0xA49DA4=.?AVScript@@` / `0xA3598C=.?AVExtraEnableStateParent@@` / `0xA35998=.?AVExtraEnableStateChildren@@` / `0xA35A04=.?AVExtraItemDropper@@` / `0xA35A10=.?AVExtraDroppedItemList@@` / `0xA3BE3C=.?AVTESForm@@` — 全て b07 と完全一致（§2-2 の type→class 対応は RTTI 直読で三重確認済み）。
- **生成経路の関数名**: `0x40663F` の書込みは **`FUN_00406360`（0x406360..0x406950、起動処理）** 内。`call 0x441970` の引数文字列 = **`0xA2FFB8 = "Data\\"`**（前後は `'Initializing TreeManager...'`(0xA2FF9C) / `'Initializing Menus...'`(0xA2FF84) の起動ログ）→ **`TES::TES("Data\\", …)` の1回構築**。
- **`FUN_00444ec0`（セル切替入口）の this == TES を直接確認**: 呼出し2件のうち主ループ側 `0x40EE66` の直前に **`0x40EE59: mov ecx,[0xb333a0]`**（`FUN_0040d4d0` = 0x40D4D0..0x40F640 の主ループ関数。`s80` の PerFrameUpdate `0x40D800` を内包）。もう1件 `0x4455BF`（`FUN_00444fb0`）は `this=esi` で座標 `fistp → sar 0xC` を `this+0x20/+0x24/+0x28/+0x2C` へ書いた後 `FUN_00440af0`（`s50_sleep_events.md:38` 実測で `0x440AF0([0xB333A0],…)` = TES 経路）→ さらに `FUN_00444ec0`。
- → **b17 §2-7「3件とも `mov ecx,[0xB333A0]` 経由」は実質正しい**（2件 direct ＋ 1件は thiscall 経由でその this を `0x40EE59` が `[0xB333A0]` からロード）。§2-1 の「細部訂正」は「ロード形が direct か this 経由か」の違いに限定される。
- **残2呼出しの同定を確定**（§2-1 表を更新）: `FUN_00444c70` の唯一 caller = `FUN_00444ec0`（`0x444F70`）、`FUN_00444ec0` の caller = 主ループ `0x40EE66` ＋ `FUN_00444fb0`(`0x4455BF`)、`FUN_005d7090` の唯一 caller = **主ループ `FUN_0040d4d0` の `0x40DEEA`**。
- **`FUN_004402f0` の `+0x88` リスト要素処理 `FUN_004d7190` を同定（V143-c 一部解消）**: thiscall。`[this+8]` の **bit5(0x20)** が立つ要素は即 return 0（スキップ）→ `vtable+0x170`(GetBaseForm) → base の formType byte(+4) が **`0x17`(23) または `0x23`(35)/`0x24`(36)**（`cmp eax,0x17` / `sub eax,0x23; cmp eax,1`）に該当するときのみ `0x4d6d40` 経由の処理へ。呼出し13件（`FUN_004402f0` / `FUN_004cb8c0` / `FUN_004dd260` / 0x5Exxx・0x60xxx・0x64xxx・0x673xxx 系）。`FUN_004cb8c0`（グリッド走査内の処理）にも本体から直接呼出し #2 が生える（`0x4CB933`）。

### 6-2. (2) `ref+0x44` — 追加: unlink ヘルパ2件の実体

- `FUN_00424b10(this=parent+0x44, arg=self)`: `push 0x40; call 0x41e210` → **type 0x40 = ExtraEnableStateChildren のリストから `0x65C620` で self を除去**、空になれば `0x422e20` で extra 自体を削除。dtor の「親あり → `push self; lea ecx,[parent+0x44]; call 0x424b10`」= **親の子リストから外す**操作で確定。
- `FUN_00424c00(this=dropper+0x44, arg=self)`: `push 0x42; call 0x41e210` → **type 0x42 = ExtraDroppedItemList から self を除去**。dtor の「type0x41(ExtraItemDropper) の `+0xC` = dropper 参照 → dropper の dropped-item リストから外す」で確定。
- → §2-2 の親/子/dropper/dropped-item の4系統が、**ctor・unlink・dtor 使用の三方向**で一致。

### 6-3. (3) bit14 — 19呼出しの全特定と名称判断の精緻化（**名称は暫定のまま**）

**(a) setter 本体周辺の同定追補**
- `FUN_0046b2c0` 尾部（実測）: 配列 `[0xB06150]`（個数 `[0xB06158]`、別カウンタ `[0xB0615C]`）内を線形走査して自身のスロットを **NULL 化＋末尾なら個数減算** → 最後に `[0xB33A98]` が非0なら `0x446b40([esi+0xC])` へ通知（`0xB33A98` = 現在空間/セル系コンテキスト・`s72` の `0xB33A9C`=current WRLD と同一系）。キーは **`[esi+0xC]` の語**（初版の「formType byte」表記は誤り → **`[+0xC]` のキー語**に訂正。`v121` は `+4` を formType と同定しており、`+0xC` は別フィールド）。
- `FUN_004524C0` = **マップの key 削除**（vtable+4 ハッシュ → バケット配列 `[esi+8]` → ノード連結走査 → 一致で unlink ＋ vtable+0x10/+0x18 の解放コールバック2種 ＋ `[esi+0xC]` カウンタ減）。`ecx=0xB0613C` での使用はその1例（全80呼出しは汎用）。
- `FUN_0046c280` = **`TESForm::~TESForm()`**（入口 `mov [esi], 0xA3BE3C` = `.?AVTESForm@@`、**86呼出し = b14 の「TESForm 系 86 vtable」と一致**）。`0x46b2c0`（登録解除）→ `+0x14` のノード連鎖解放 → `+0x10` クリア → **bit14 立ちは `jne 0x46c2db` で以降をスキップ**（未立なら追加の解除処理）。
- `FUN_0046c640`（vt+0x90、63 data ptr）: **引数!=0 → bit1 未立かつ bit14 未立なら配列走査→未登録なら `0x449070` で `[0xB0614C]` へ挿入 → `or [esi+8],2`**。**引数==0 → bit1 が立っていれば配列走査→所見なら削除／不所見なら `and [esi+8],0xFFFFFFFD`（bit1 解除）**。（※ `v121_gt_corrections.md:57-59` は「arg==0 で挿入」と記載だが、実アセンブリは **arg!=0 が挿入/登録側、arg==0 が解除側** → §3 V143-h に要確認として記録。）

**(b) 19呼出しの所属（全特定）と文脈**

| 関数（所属） | 件数 | 直前の文脈（実測） |
|---|---:|---|
| `FUN_0046a020` ← **`FUN_00507320` = script opcode `AddItem`(0x1002) impl（`0x50743A`）** | 1 | extra0x12 event list 生成/設定 → **`malloc(0x58)` + ctor `0x4d9a70`（= TESObjectREFR ctor: vtable `0xA46C44`/`0xA46C38`、formType(+4)=`0x31`、`+0x3C/+0x40` ゼロ、`+0x44` extra リスト）→ bit14 → `0x41E920`/`0x4FBE00` でイベントリストへ登録** |
| `FUN_0048e060`（10 callers: `FUN_0044a7d0`×2 / `FUN_00491ce0`×3 / `FUN_004da3e0` / `FUN_004ddb00` / `FUN_004df820` / `FUN_005ea380` / jmp `FUN_004d70e0`） | 6 | 同一パターン（0x58 REFR 生成 → bit14 → イベントリスト登録）。本体は **TESScriptableForm（typeinfo `0xB0331C`）へ dynamic_cast した要素のリスト走査** |
| `FUN_00491ce0`（唯一 caller `FUN_004937e0`、内部で `FUN_0048e060` を3呼出し） | 6 | 同一パターン |
| `FUN_00485d50`（caller: `FUN_00448f40` / `FUN_00485e00`） | 1 | **グローバル `[0xB34428]` の初回生成時のみ**（0x58 REFR → bit14） |
| `FUN_0052acc0`（caller `FUN_0052a570`） | 1 | **this+0xC に stack/埋め込み Script（ctor `0x4FBAA0`）を構築 → `0x46ab80` → bit14** |
| `FUN_00531290`（caller `FUN_00531470`/`FUN_00531b40`） | 1 | **グローバル `[0xB36530]`**（dedup ガード `[0xB3652C]`、stack Script 構築 → `0x4FC7A0` → `0x46ab80` → **bit14** → `0x46b680(-1)`） |
| `FUN_00586000`（唯一 caller `FUN_005821f0`） | 2 | **`'delete %s'`(0xA69EC0) / `'delete '`(0xA69ECC) の文字列出力（`0x579B60` 呼出系、別途 fmt `0xA69CBC`）→ stack に Script 構築 → bit14 → `0x4F9E50`**（b19 の quest バイト書手系） |
| `FUN_006830b0`（唯一 caller `FUN_006836e0`、WinMain 一次系統 — v128 の `FUN_00683030` 記述と同一領域） | 1 | `formType(+4)` → `0x4db260` → **dynamic_cast TESObjectREFR→Actor に成功した対象**へ `0x6c5420` → **bit14** → `0x46c300(1, key)` |

**(c) 読手の全数**: `shr reg,0xe`（bit14 読取形）= **62件**。中核は Disable `0x50A25E` / `TESObjectREFR` dtor `0x4E2978` / `TESForm` dtor `0x46C2B6` / `DeleteForm` `0x45C7FC` / `0x46C640` / **vt スロット `FUN_0046B4C0`（data ptr 82件 = TESForm 系仮想、bit14 立ちなら偽を返す）** / `MobileObject` dtor `0x659FC8` / `FUN_004cecd0` / `FUN_004ddcb0` ほか。
- setter `0x46B590` と `0x46b2c0`/`0x45C7A0` は **.rdata/.data から関数 ptr 参照0件 = direct call 専用**（`0x46B4C0`=82件・`0x46C640`=63件は仮想）。実行時 clear は再確認でも **不在（set-only）**。

**(d) 「Deleted」説の検証 → 弱める（新証拠）**
- **`FUN_0045c7a0` = `DeleteForm()`**（実測: 入口で load 中ガード `call [0xa2808c]` vs `[0xB33398+0x10]` / `[this+0x18]` bit0・bit18 → 失敗時 **`push 0xA3A644 'DeleteForm() was called, but the game is not being loaded.'` → 警告 `0x4A7A60`**、`0x46c300`＋`0x46b2c0` で登録解除、bit14 gate（立ちは skip）、vtable+0xD8 へ空文字列、20呼出し）。**ただし `FUN_0045c7a0` は `0x46B590` を呼ばない（19件に不在）→ DeleteForm 経路は bit14 を設定しない**。
- 並行系として **bit5(0x20)** が別ゲートで存在（`FUN_0046B4C0`: bit14→偽、bit5→`0x450980`/`0x450a80` 実行後に真 / `FUN_004d7190`: bit5 立ちは要素スキップ）— 「削除済み」系の役割は bit5 側と重なる可能性が高く、**bit14 = Deleted は根拠が弱い**。
- `'Delete'`(0xA61894) の push 先 `FUN_009f3400` は **キーボードキー名テーブル（`'sKBDelete'`/`'Delete'` は `'sKBLWin'`/`'sKBInsert'` 等と並ぶ入力名）** → 「コンソール Delete コマンド」説は**棄却**。

**(e) (3) の判定更新**: 觳測された bit14 設定対象は全て **(i) 実行時生成の 0x58B `TESObjectREFR`（AddItem/イベントリスト系・グローバル単体）、(ii) stack/global の `Script` 単体、(iii) 起動時走査で cast 成功した `Actor`** であり、いずれも **World セルに登録されない・永続化対象外の「管理外」オブジェクト**。これは「Disable/Enable 対象外」「dtor で world デタッチ＋保留集合除去をスキップ」「`[0xB06150]` 登録配列から除外（`0x46c640` が bit14 立ちは登録 skip）」と完全整合。
- **暫定名称 = 「管理外/一時（not-in-world / Temporary・unmanaged）」**、`Deleted` 説は (d) により棄却寄り。**確定には [0xB06150] 配列の用途（save 書出除外判定か）と bit14 読手の save 経路該当の確認が必要 → V143-a に維持。**

### 6-4. (4) `FUN_004e2920` / `0x9C3C13` — 追記

- dtor 全体実測（§2-4）に加え: **`0x9C3C13` を指すオペランド（`.text` 全走査）= 0件、`.rdata`/`.data` の dword `0x9C3C13` = 0件、`call/jmp 0x9c3c13` = 0件** → 「登録先なしの孤カラ thunk」ではなく、**先頭 `0x9C3C10: mov ecx,[ebp-0x10]` からなる MSVC EH クリーンアップ funclet（末尾が `jmp 0x4e2920`）**であり、直後 `0x9C3C18` は SEH ハンドラ本体（`__security_check_cookie` `0x9811E2` → `0xAEC75C` データ → `jmp`）。decompiled の `puStack_8 = &LAB_009c3c18`（`FUN_00659f90` 相当の SEH フレーム）と連なる。**funclet 先頭 `0x9C3C10` を指す .rdata スコープテーブルの走査が未実施 → V143-b に更新**（現状: 「FUN_004e2920 から出る jmp ではない」ことは確定＝**方向は入向**）。
- 呼出し3件の最終確定: `0x4E4923 call`（`FUN_004e4920` deleting dtor）/ `0x65A02B call`（`FUN_00659f90` = MobileObject 系 dtor チェーン）/ `0x9C3C13 jmp`（上記 EH funclet）。

### 6-5. (5) b10 混称裁定 — 追加（タグ差3行の正体）

- **`cache 0xB361xx` タグ無し3行** = `GetPersuasionNumber(0x10E1)` / `GetTotalPersuasionNumber(0x113B)` / `GetIdleDoneOnce(0x113E)`。3行とも **cond に `0xB361AC` 実在**（前2行はさらに自己メモ化スロット12個を併用）→ **b10 の159タグはオミットであって表記内容は正しい**。
- impl/cond 分解（162行）: **cond に AC = 160行 / impl に AC = 11行 / どちらにも無い行 = 0** → 「GetDisabled 行以外はほぼ 0xB361AC」は**方向として正しいが、正確には162/162行が AC を持つ**（GetDisabled 行も cond の AC 2箇所のみで `0xB361CC` 直接参照なし）。
- `GetDisabled` impl `0x502370` は `0xB361xx` 参照ゼロ（cond `0x4F60E0` のみ）。

### 6-6. (6) 1フレーム挙動 — 追記（消費者の同定）

- `FUN_0065fda0` の呼出し2件を特定: **`FUN_006701b0`（0x6701B0..0x670CA0、`0x67027D`）** と **`FUN_00670ca0`（`0x670D31`）**。`DAT_00b3bad0`（最寄り候補）の消費者は `FUN_006765f0(…, DAT_00b3bad0, …)`（確保/解放）と `vtable+0x174`（位置取得）で、全て 0x670000 帯＋`FUN_0050b3e0` 区分（v10 CSV の欠落 opcode 群 `0x50B3E0..0x50BB70` 内 `0x50B690`）・`FUN_005821f0` から → **対象/アクション状態管理系（AI・対象選別）**。
- **`FUN_006701b0` 本体（decompiled L433742-）**: `param_2==0` で `FUN_006765f0(0, DAT_00b3bad0, 1)`＝**選別結果の解放**、`DAT_00b3bad4`/`DAT_00b3bad0` クリア、`vtable+0x250/+0x354` 呼出し等 → 「保留中の参照を含む最寄り候補を毎フレーム作り直す」型の処理で、**保留集合除外（`0x65FE00`）が即時性の要**という §2-6 の結論を補強。
- → 描画（Render::Frame は集合非参照＋flag 未設定）・経路探索（NavMesh/pathing に集合読手なし）は**フリーズではなく「当フレームは有効扱い」**、スクリプト条件と対象選別だけが即時 — この対比構造を最終結論とする。

### 6-7. (7) 表示バグ記録 — 追記（json と b11 の最終確認）

- `b17_table2.json` rec0 `fields[8] = '0x00000001'`（= main row254 col4）→ **json も raw と一致**。
- `b11_event_bit_setters.md` に `0x00010001` / `0x00000001` / `b17_table2` の出現 **0件** → 「b11 に影響しない」を確認。
- `b17_table2_out.txt` 全84行精査（§2-7）→ **現行ファイルに row254 の誤表示は不在**。裁定維持。

### 6-8. §3 残点の更新（本追記による縮小・改訂）

| ID | 状態 |
|---|---|
| V143-a | **維持・縮小**: 残るのは「`[0xB06150]`/`[0xB0614C]` 配列の用途（save 書出除外か）」「`FUN_0046a020`/`FUN_00491ce0`/`FUN_0048e060` が作る 0x58 REFR の意味（AddItem/イベントリスト＝在庫 REF か）」「bit14 読手62件の save 経路該当」「`FUN_00586000` の正体」 |
| V143-b | **更新**: thunk は `0x9C3C10` 起点の EH funclet と判明。残るは `.rdata` スコープテーブル（`0x9C3C10` 参照）の走査 |
| V143-c | **縮小**: `FUN_004d7190` を同定（bit5/formType ゲートのリスト要素処理）。残り = `FUN_004f9fa0` / `FUN_00529aa0` / `FUN_004cb8c0` 本体 / `FUN_0045c7a0`(DeleteForm) 20呼出しの内訳 |
| V143-d | 維持（`World::Update` 内 前後関係＋描画側 flag ルート列挙） |
| V143-e | 縮小: 書き手4件の所属を特定（`FUN_004fa2d0` / `FUN_004fbaa0`(Script ctor) / `FUN_004fbf00`×2＝Script/VM クラスタ）。残り = `0x579B60` の出力先 |
| V143-f | **解消**: タグ差3行の正体を特定（§6-5）。b10 は既存ファイルのため**記録のみ** |
| V143-h（新規） | `v121_gt_corrections.md:57-59` の `FUN_0046c640`「arg==0 で挿入」は実アセンブリと**方向が逆**（arg!=0=登録/挿入、arg==0=解除）→ v121 側の要訂正確認（既存ファイルのため本稿では記録のみ） |

### 6-9. 追加分の使用証拠

- `%TEMP%\opencode\v139_2.py`/`v139_2.txt`（enclosing 再マップ20,465件、RTTI 直読、call/jmp watch 16目標、`shr 0xe` 62件、文字列 xref 7件、data dword 8目標スキャン、Disable/Enable/dtor周辺・registry 群の逆アセンブル）
- `%TEMP%\opencode\v139_3.py`/`v139_3.txt`（候補関数の caller 再走査、data ptr 9目標、19 setter サイト文脈 dump、`FUN_0045c7a0` head、キー名文字列隣接、b10 未タグ3行分解）
- `v10_impl_decompile_join.csv`（`FUN_00507320` = `AddItem`、`0x50B3E0..0x50BB70` の opcode 欠落群対応表）、`b07_vtable_index.md:1694`（typeinfo `0xB0331C` = TESScriptableForm）、decompiled `FUN_004d9a70`@171167 / `FUN_006701b0`@433742 / `FUN_0048e060`@114093 / `FUN_00491ce0`@116908

## §7 最終裁定・結論・移植示唆（追記セッション 2026-10-27）

### 7-0. 二重実行確認と追記方針

- 着手時に `tmp/re/v139_b17_table2_residuals.md` は**既存**（前回セッションが §0〜§6 を作成済み）。
  → **上書きせず §7 を追記**（契約 §3・タスク指示どおり）。既存 §0 の「成果物の事前存在 = 不在」は
  **前回セッション（新規作成時）の記録**としてそのまま残置。本追記セッションの検証は §7-4 に分離。
- 既存ファイルは読取のみ（他 `tmp/re/*.md`・`docs/`・`TODO_re49.md`・台帳・git・Supermemory は**非編集**）。
- 中間スクリプト: `%TEMP%\opencode\v139_4_final.py` → `v139_4_final.txt`（本追記セッションの再計測）。
  前回分 `v139_1..v139_3` は読取のみ再利用。

### 7-1. 結論（3行）

1. **b17 §4 の残留7点は全て裁定済み**: (1)(2)(4)(6)(7) は解明、(3) は機構解明＋名称暫定、
   (5) は「b10 側が混称」という b17 の推測を**棄却**（b10 は §5.3 で正体明記済み・混称なし）。
2. Disable の遅延設計は「dedup 保留集合（`0xB361CC`）→ 毎フレーム末 `FUN_004402f0` で全要素へ
   コア無効化（flag bit11）を適用して空にする」の3段で、集合の読手は **GetDisabled 条件と
   AI 対象選別 `FUN_0065fda0` の2件のみ**。描画・経路探索は集合を参照しないため、
   保留中（最長1フレーム）は有効扱いのまま実行される。
3. `0xB333A0` は s65/s72 の「NavMesh 管理オブジェクト [推定]」ではなく **クラス `TES` の単一
   グローバルインスタンス**（RTTI `.?AVTES@@` で確定、書込みは起動時 `0x40663F` の1件のみ）。

### 7-2. 7点裁定の最終表（§1 の確定版・検証出所付き）

| # | 残点 | 最終裁定 | 主な裏付け（§参照 / v139_4_final 再測） |
|---|---|---|---|
| (1) | `0xB333A0` 正体 | **クラス `TES` のグローバル**。s65/s72「NavMesh 管理 [推定]」は**誤り（訂正）**。残2呼出し: `FUN_00444c70` = セル/空間トランジション時 flush、`FUN_005d7090` = メインループ定期メンテ時 flush | §2-1/§6-1。RTTI `0xA37470 → .?AVTES@@`（再測一致）。`call 0x4402f0`=3 |
| (2) | `ref+0x44` の型 | **BaseExtraList**。有効状態親 = **type `0x3F` = `ExtraEnableStateParent`**（三重裏付け）。type `0x41` = `ExtraItemDropper`（別物） | §2-2/§6-2。RTTI 5vtable 再測一致（6/6）、ctor type byte `0x3F`/`0x41` 再測一致（2/2） |
| (3) | bit14 名称 | **機構解明・名称は暫定「管理外/一時（not-in-world/Temporary）」**。setter は登録解除→`[form+8] |= 0x4000`、実行時 clear **不在（set-only）**、`DeleteForm()` は経路に不在 → `Deleted` 説は棄却寄り。確定は V143-a | §2-3/§6-3。`call 0x46B590`=19（再測一致） |
| (4) | `FUN_004e2920` の esi | **self（破棄対象 TESObjectREFR 自身）**。本関数は destructor（`vtable 0xA46C44` 設定）。`push esi; call 0x4FA540` = 保留集合から自己除去。子走査は EDI の別ループ。`0x9C3C13` = **入向 EH funclet の `jmp 0x4e2920`**（方向は逆） | §2-4/§6-4 |
| (5) | b10「cache 0xB361xx」混称 | **混称は b10 側になし（b17 §4-6 の推測が誤り）**。b10 の162行は全て `0xB361AC`（トレースフラグ）を持ち、`0xB361CC/D0`（保留集合）直接参照は **0/162**。GetDisabled 行さえ直接参照なし（間接: cond の `call 0x4FA560`） | §2-5/§6-5。imm32 再測: `0xB361AC`=**249** / `CC`=**6** / `D0`=**2** |
| (6) | Disable〜flush の1フレーム | 集合読手は GetDisabled（即時 true）と `FUN_0065fda0`（AI 対象選別・即除外）の**2件のみ**。描画（Render::Frame）・経路探索は集合非参照＋flag 未設定 → **当フレームは有効扱いで実行**。flush はフレーム末／セル切替時／定期メンテの3経路 → 保留は**最長1フレーム＋α** | §2-6/§6-6。cond `0x4F60E0` 逆アセンブル再測: `shr 0xb`(bit11) と `call 0x4FA560` を確認（51命令） |
| (7) | `b17_table2_out.txt` 表示バグ | **raw 値 `0x00000001`（row254 col4）は正しい**。`0x00010001` は row253 の生値。現行ファイルに row254 の誤表示は**不在**（当時の観測は行ラベル off-by-one）→ 訂正要求なし、表現のみ V143-g で記録 | §2-7/§6-7。raw 再測: `[0xB0D708]=0x00000001` / `[0xB0D6E0]=0x00010001` |

### 7-3. Android 移植への示唆（実装しない）

移植側が実装すべき中核は (1)〜(7) が支える「**オペコード表1本＋Disable 遅延キュー1本＋TES グローバル1本**」の三点セットである。
オペコード表は `cache/opcode_table.json` と同型の 533 行×40B 静的データでよく、第2テーブルは +0x18 の非整列窓なので転記不要。
Disable は `HashSet<Ref*>`（dedup 挿入 / Enable 除去 / GetDisabled は `flag||set` / フレーム末 apply & clear）で代替でき、
即時適用に書き換えると mid-iteration のリスト破壊と毎フレーム一括適用タイミング（`FUN_004402f0`）が失われるため**遅延キューの形を保つ**こと。
有効状態親（`ExtraEnableStateParent`, type 0x3F）が在る参照への Enable/Disable は文字列どおり警告を出して無視する再現が要る。
保留中の1フレーム（描画・経路探索は有効扱い、スクリプト条件と AI 対象選別だけ即時）は**バグではなく仕様**として移植し、
`TES` グローバル（`0xB333A0`）は「起動時に1回構築される最上位状態オブジェクト」として扱い、
NavMesh 専用オブジェクトと誤認しないこと（s65/s72 の旧記述は訂正済み・本稿が正）。

### 7-4. 最終自己検証（追記セッション）

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**
  （bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C=`.\AI\CombatController.cpp` / opid0x0015 impl=0x504560）。
- 再計測（`%TEMP%\opencode\v139_4_final.py` → `v139_4_final.txt`、全 .text 再走査なしの独立再読取）:
  1. call 件数 **11/11 一致**: `0x4402f0`=3 / `0x4FA560`=2 / `0x4FA540`=2 / `0x4FBB30`=1 / `0x4FA580`=1 /
     `0x46B590`=19 / `0x446CB0`=347 / `0x4468B0`=0 / `0x4FBF90`=77 / `0x4E4690`=5 / `0x4DD850`=3
     （b17 §5・GT・b19:185 と一致）。
  2. raw dword **4/4**: `[0xB0D708]`=0x00000001 / `[0xB0D6E0]`=0x00010001 / `[0xB0D6E4]`=0x00B0A6CC(row253 argdesc) /
     `[0xB0D70C]`=0x00000000。
  3. RTTI 直読 **6/6**: `0xA37470`→`.?AVTES@@`、`0xA3598C`→ExtraEnableStateParent、`0xA35998`→ExtraEnableStateChildren、
     `0xA35A04`→ExtraItemDropper、`0xA35A10`→ExtraDroppedItemList、`0xA46C44`→TESObjectREFR（b07 と一致）。
  4. extra ctor type byte **2/2**: `FUN_0042A5E0`→`0x3f`、`FUN_0042A840`→`0x41`。
  5. GetDisabled cond `0x4F60E0` 逆アセンブル 51命令: `shr …,0xb`（flag bit11）と `call 0x4fa560`（集合判定）**両方確認**。
  6. .text imm32 走査 **3/3 一致**: `0xB361AC`=249 / `0xB361CC`=6 / `0xB361D0`=2（b17 §2-3・§4-6 と一致）。
  7. 合計: **内部整合 27/27**（11 call + 4 raw + 6 RTTI + 2 ctor + 3 imm32 + 1 cond）。
- 残点は §3 の V143-a〜g（既存ファイルのため**記録のみ・編集なし**）。
