# v247_process_havok_residuals9.md — process/Havok 残点第9束（V-245-9）

- 日付: 2026-11-02 / 作成: opencode サブエージェント（TODO serial 240 / V240・低）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B）
- 出典: `tmp/re/v245_process_havok_residuals8.md` §8 新規残点5件（v240/v245 本体は無編集）
- 手法: capstone 5.0.7（`v172_common.py` 再利用）＋ `tmp/re/cache/{named_funcs,vtable_index,call_targets}`（参照のみ）＋ Web 検索
- 中断対策: **項ごとに即書込み**。本バッチは `%TEMP%\opencode\v247_*.py`（各 `_out.txt` 付き）
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**。新規は本 1 ファイル＋`%TEMP%\opencode/v247_*` のみ。ESM 読取 0 回・ソース編集 0 回
- selftest: `probe_harness.py selftest` **4/4 PASS**（本セッション着手時）
- **VA 換算**: `imm32_refs3()`（セクション個別 raw）・E8/E9 は raw→VA 変換付き全ファイル走査で v234/v240 罫注記遵守

---

## §1 item1（V-245-9-1）OEP `0xBAF2ED` → CRT startup（`0x98787C` トランポリン）の間接経路 — **解明: v126 の `.bind` 複号ブートストラップ経路と完全接続を独立再検証で確認**

### 証拠A: ピースの全部を本バッチで再実測（`v247_1_out.txt` / `v247_1b_out.txt`）

1. **PE エントリ**: `AddressOfEntryPoint RVA = 0x7AF2ED → VA 0xBAF2ED`（PE ヘッダ直接読み）。
2. **スタブ末尾の tail-jump**: `0xBB096B: mov eax,[ebp-0x430] / add esp,0x1000 / pop ebp,edi,esi,edx,ecx,ebx（入口6 push の逆） / 0xBB097D: jmp eax`（`v247_1_out.txt` (e)）。
3. **間接 jmp 全数**: 実行域 `[0xBAF2ED,0xBB1204)` の線形逆アス 1,791 命令・**jmp 合計 125 本 = 直接 124 / 間接 1**。間接は **`0xBB097D: jmp eax` のみ**（`v247_1b_out.txt`）。間接 call 85 本は全て IAT/API 解決用（`call [ebp-0x58]`=GetProcAddress 等）で転移先ではない。
4. **静的走査（E8/E9 rel32＋imm32。実行可能 raw 範囲 = .text 0x400..0x627200 と .bind 0x733200..0x788600 を全走査。v126 は全ファイル raw でも同一結果）**:
   - → `0x98787C`: **E8/E9 = NONE、imm32 = NONE**
   - → `0x98769C`: E8 = **NONE**、E9 = `0x987881` の1件のみ（トランポリン内部）、imm32 = NONE
   - → `0x40DF80`（WinMain）: E8 = `0x98780E` の1件のみ、imm32 = NONE
   → **CRT startup への接続は静的参照の一切を経由しない**（v126 §2-2 の全探索0件を本バッチで再現）。
5. **暗号ヘッダの復号再現**: VA `0xC03168`（file off `0x787368`）を chain-XOR（`dec[i]=c[i]^c[i-1]`、初鍵 = c[0]）で復号 → **`hdr[0x318] = 0x0098787C`** が一致。副次フィールドも v126 と一致: `hdr[4]=0x51C00`（payload size）／`hdr[0x398]=0xBB1568`（payload VA）／`hdr[0x3FC]=0xA239276D`（XTEA key 先頭）／`hdr[0x40C]=0x573A`（=config+0x2C）（`v247_1_out.txt` (h)）。暗号文 `c[0x318/4]=0x9A7A2C99`（file 0x787680 = v126 記載の `99 2c 7a 9a` と一致）。
6. **トランポリン本体**: `0x98787C: call 0x997BE3（__security_init_cookie） / 0x987881: jmp 0x98769C`（`v247_1_out.txt` (b)）。
7. **CRT 本体頭**: `0x98769C: push 0x60 / push 0xAFFAD8 / call 0x98C5A0（__SEH_prolog）`（(c)）、**WinMain 呼出し** `0x98780E: call 0x40DF80`（nShowCmd=10 / hInstance=0x400000 / (d)）＝v245 §1 の確定と接続。
8. **payload**: `%TEMP%\opencode\v126_payload.bin`（334,848 B）が実在し `MZ` ヘッダ（e_lfanew=0xE8）＝v126 の XTEA 復号結果を確認。`.bind` 上の payload 領域先頭は暗号文（`b'2\x99'`）＝復号必須。

### 接続図（本バッチで全ピース再測定）

```
OS ローダ → PE OEP 0xBAF2ED（.bind Steam DRM スタブ）
  → chain-XOR で config(0x364B) 復号 → steamdrm.dll を XTEA 復号・手動マップ
  → call [ebp-0x3f4]（steamdrm!start(config,0x1000)）@0xBB0869
  → 戻り値 eax = 暗号ヘッダ 0xC03168+0x318 の復号値 = 0x98787C
  → 0xBB097D: jmp eax（スタブ唯一の間接 jmp）
  → 0x98787C: call __security_init_cookie(0x997BE3) ; jmp 0x98769C
  → 0x98769C WinMainCRTStartup → 0x98780E: call WinMain(0x40DF80)
```

### 裁定

> - **OEP `0xBAF2ED` → CRT startup の間接経路は v126 の `.bind` 複号ブートストラップ経路で確定**（確度: 高）。本バッチで PE エントリ・スタブ末尾・間接 jmp 全数（1/125）・静的参照 0 件・ヘッダ復号（0x318=0x98787C）・トランポリン・WinMain 呼出しの**7ピースを独立に再実測**し、全て v126/v245 と一致。
> - v245 §1 が未解明と記した「0x98787C トランポリンへの間接接続の詳細」は、**暗号ヘッダ格納値を `steamdrm!start()` が戻り値として返し、レジスタ tail-jump で渡す**ことで解明（E8/E9/imm32/TLS/ポッチのいずれも不使用）。
> - `jmp eax` はスタブ実行部に1本のみ（125 jmp 中）＝到達不能経路も含めて接続は単一。

**判定: 解明（接続確認完了）**（証拠 `v247_1_out.txt` / `v247_1b_out.txt`、v126_bind_crt_bootstrap.md 継続）。

---

## §2 item2（V-245-9-2）packfile B（`0xA9C990`）の SDK 正式名と `__classindex__`/`__dataindex__` の出所 — **判定: 負証拠を機械面まで強化。B は「binary packfile writer（hkBinaryPackfileWriter 同族）」で確定、`__classindex__`/`__dataindex__` は B 私有の非公開名（公開仕様に不在）。正式クラス名は引き続き推定**

### 証拠A: B は本物の binary packfile を書く（`v247_2e_out.txt` / `v247_2d_out.txt` (a)）

- slot4 の呼出し列のうち **`FUN_0090DA40`** が packfile ヘッダを組み立てて書く:
  - `0x90DA67: mov [esp+0x4c],0x57E0E057` / `0x90DA6F: mov [esp+0x50],0x10C0C010`（**hkxHeaderData の magic1/magic2**）
  - `0x90DA77: mov [esp+0x54],ecx`（= arg の userTag）／`0x90DA7B: mov [esp+0x58],2`（**version = 2**）
  - 直後に `0x8B1890` で 0x40 B のヘッダをストリームへ書出し、続けて 0x18 B のセクションヘッダ領域をゼロ化 → `call [eax+8]`（slot2 = セクション追加）→ `0x941400`（終了処理）。
- **magic 定数の imm32 出現は EXE 全体で各1件のみ**（`0x90DA67` / `0x90DA73`）＝**この EXE に packfile リーダは存在しない**（書出し専用）。Oblivion 本体は .hkx を読まず、物理ワールドのスナップショット書出しだけを持つ（§3 と整合）。

### 証拠B: B が登録するセクション名は B 私有リテラル（`v247_2_out.txt` (b)(c) / `v247_2b_out.txt` (a)(b)）

| セクション | 文字列 VA | 実体 | 参照（imm32 全数） | 読手 |
|---|---|---|---|---|
| `__classnames__` | `0xA9C980` | file 0x69BB80・**1件のみ** | `.data@0xB2FF04` のみ | **B ctor `0x90D7DF` のみ** |
| `__classindex__` | `0xA9C970` | file 0x69BB70・**1件のみ** | `.data@0xB2FF08` のみ | **B ctor `0x90D7ED` のみ** |
| `__dataindex__` | `0xA9C960` | file 0x69BB60・**1件のみ** | `.data@0xB2FF0C` のみ | **B ctor `0x90D7FB` のみ** |

- 3文字列は **`0xA9C960..0xA9C98E` に連続配置**され、直後 `0xA9C990` が B の vtable ＝ **B 専用の静的リテラル**。
- B ctor `FUN_0090D7D0` は `[0xB2FF04]/[0xB2FF08]/[0xB2FF0C]` を順に push して `0x941140`（=`thunk_FUN_00940EF0` = **addSection(名前)**、`0x940EF0`: map で重複検査 → `0x8B18F0`(strdup) → 名前配列 `+0x20` へ push）を3回呼ぶだけ（`v247_2_out.txt` (a)(d)、`v247_2g_out.txt` (d)）。
- **EXE 内でこの2名を読む/書くコードは B ctor 以外に 0 件**（＝本 EXE にこの形式のリーダも他ライタも無い）。

### 証拠C: 標準名セットは別系統として実在（＝B は標準名を使っていない）

- `__types__` = `0xAA21D4`（refs 1: `.data@0xB30594`）、`__data__` = `0xAA21E0`（refs 1: `.data@0xB30590`）。
- これらは **base/XML 系のヘルパが実際に読む**: `0xB30594` の読手7件（`0x90D43F`/`0x90D4E6`/`0x90E361`/`0x940FE4`/`0x941011`/`0x94103D`/`0x9410F1`）、`0xB30590` の読手1件（`0x941191` = base slot2 `FUN_00941190`）。`FUN_00941190` は `ret 0x10`（4引数）で `[0xB30590]`(`__data__`) を追加引数として `FUN_00941070` へ渡す（`v247_2h_out.txt`）。
- つまり **同一 EXE 内に「標準3名（classnames/types/data）」を使う系統と「classnames/classindex/dataindex」を使う B が併存**する。B が標準実装そのものなら起きない差。

### 証拠D: 公開仕様の負証拠（Web 再検索）

- 標準 binary packfile のセクションは **`__classnames__` / `__types__` / `__data__`**（reshax「Most common sections are…」／lukascone 2024「Most common sections are…」／HedgeLib `HavokXML.cs` は `hksection name=` を任意名で往復）。新形式は `TAG0/SDKV/DATA/TYPE/TSTR/TNAM/FSTR/TBOD/THSH/INDX/ITEM/PTCH`。
- `__classindex__` / `__dataindex__` は **どの仕様・どのツール記述にも出現しない**（本バッチ: `websearch` の完全一致クエリ 0 件、`grep.app` API 429、`searchcode` API 404＝**取得失敗も明記**）。
- ヘッダ仕様（hkxHeaderData）は `numSections / contentsSectionIndex / contentsClassNameSectionIndex` という**インデックス**で内容を指し、**名前でセクションを引く仕様ではない**（reshax の struct 引用）→ リーダ側は名前非依存で成立するため、非標準名でも読める。

### 裁定

> - **B = binary packfile writer（`hkBinaryPackfileWriter` 同族）で確定**（magic1/magic2・version=2・0x40B ヘッダ・0x18B セクションヘッダ・セクション名登録の5点が機械一致）。確度: **高（機能）**。
> - **SDK 正式クラス名は依然として推定**（RTTI/COL 不在・vtable_index 行なし）。`hkBinaryPackfileWriter` を第一候補（確度: 中）に据えるが、**`__classindex__`/`__dataindex__` は公開 SDK 仕様に存在しない**ため、次の2択の判別は不能:
>   (i) Bethesda（ゲーム）側の派生/改造ライタが独自2セクションを追加（**本命・確度: 中**。根拠: 標準名系統が同一 EXE に別存し、B だけ命名が異なる／文字列が B 直前に私有配置）、
>   (ii) Havok 4.0 期の旧命名で公開資料（5.0+ 前提）に未収載（**従属説・確度: 低**。根拠: ヘッダ version=2 が公開資料の 4/6/8 より古い／反証: EXE に `Havok-x.y` 版文字列が0件で版を特定できない）。
> - **`__classindex__`/`__dataindex__` の「出所」= B ctor が `addSection` に渡す B 私有リテラル**（＝上流 SDK からの引用ではなく、この writer の設計者が命名）。EXE 内に消費者が皆無な点が「外部ツール向けの自己記述ラベル」であることを支持。

**判定: 部分解明（機能=確定／正式名=推定、出所=B 私有リテラルで確定）**（証拠 `v247_2_out.txt` / `v247_2b_out.txt` / `v247_2c_out.txt` / `v247_2d_out.txt` / `v247_2e_out.txt` / `v247_2g_out.txt` / `v247_2h_out.txt`、Web 再検索5回）。

---

## §3 item3（V-245-9-3）`'snap'` の上流出所＋slot4 B の書込/読込最終判別 — **解明: slot4 B = 書込（write）で確定。`'snap'` はゲーム側が合成する4CCタグ、上流概念は Havok `hkpHavokSnapshot`（WorldSnapshot）系**

### 証拠A: B slot4 は「書込」で確定（5点）

1. **packfile ヘッダを書き出す**: `FUN_0090DA40` が `0x57E0E057`/`0x10C0C010`/version=2 の 0x40 B ヘッダをストリームへ書く（§2 証拠A・`v247_2e_out.txt`）。
2. **16 B 境界パディングを「書く」**: `FUN_0090DC60` は `0x90DD30`: 0x10 B を `0xFF` で埋め → 端数 `ecx = 0x10-(pos&0xF)` を `call [edx+0xc]`（=write スロット）で送出（`v247_2e_out.txt`）。読込では起こらない処理。
3. **slot4 末尾も write**: `0x90F00C/0x90F03D` で `call [edx+0x18]`（バッファ+サイズ）→ `0x90F051: xor eax,eax; ret 8`＝成功 0 を返す書出し契約。
4. **出力先は実ファイル `<name>.hkx`**: `fn@0x444740` が引数文字列をコピーし **`".hkx"`（`0xA375F8`、5 B 転写 @0x4447B1..0x4447BF）を連結** → `[0xB35C24]` のオブジェクト経由で **`fn@0x889D60`（`'Running Cell Test'` 帯）→ `FUN_008BC6C0` → `FUN_008BC660` → `FUN_008BC540`（slot4 呼出し）**（`v247_3_out.txt` (a)(b) / `v247_3c_out.txt` (a)）。`'.hkx'` の imm32 参照は EXE 全体で **この1件のみ**。
5. **リーダ不在**: magic 定数の出現は各1件（書込側のみ）＝この EXE に packfile 読込経路は無い。

→ **slot4 B の書込/読込の最終判別 = 書込（write）で確定（確度: 高）**。v245 §3 の「直列化ヘルパ群のみで判別不能」を解消。

### 証拠B: `'snap'` は文字列ではなく 4CC 即値合成（`v247_3_out.txt` (c)(e) / `v247_3b_out.txt`）

- **`b"snap"` の生バイト出現 = EXE 全体で 0 件**（`snapshot`/`SNAP` も 0）。`0x8BC5DB..0x8BC5EA` で `0x73/0x6E/0x61/0x70` を**バイト単位で積んで** dword `0x70616E73` を作り、`[esp+0xc]` に格納（＝`{+0='snap', +4=…, +8=1}` の 12 B ディスクリプタ先頭）。
- 同ディスクリプタは **slot4 の第2引数**として渡る（`0x8BC610: lea eax,[esp+0xc] / push eax / push edi / call [edx+0x10]`）。`FUN_0090EF80` 側は `esi=[esp+0x6c]`（=このディスクリプタ）を `0x90D960` に渡して一時オブジェクト化する。
- `+4` は **`"SnapshotSave"`（`0xA98368`）由来の dword**（`0x90BBA0` が `*(u32*)src` をコピー）。`+8=1` はフラグ/版と推定（確度: 低）。
- ラベル `"SnapshotSave"` は `0xA98368` に**実文字列として存在**し、直前 `0xA98360` に `fn@0x8BC520`/`0x8BC3E0`（dtor/補助）を伴う静的ブロック。隣接して **`'CollAgtPtr'`（0xA9837C）** ＝ contents 側の文字列（v245 §3 証拠D）と同一クラスタ。
- contents の期待型は `0xBA8320='hkRootLevelContainer'` / `0xBA82D8='hkPhysicsData'`（`0x8BC687/0x8BC69A`）＝**Havok ワールドスナップショットの典型構成**。

### 証拠C: 上流（Web）

- Havok SDK には **`hkpHavokSnapshot`** があり、`save`/`load` で物理ワールドのスナップショットを直列化する（GameDev.net #628051 が `hkpHavokSnapshot::load` と SDK 同梱デモ `Demo\Demos\Physics\Api\Dynamics\World\WorldSnapshot` を明示）。本経路の contents（hkRootLevelContainer＋hkPhysicsData）と一致。
- ただし **`'snap'` という 4CC 自体は公開 SDK/仕様/ツール記述に出現しない**（本バッチ Web 5回＋v245 の8回、いずれも 0 件）。→ タグは**ゲーム側が付けた種別識別子**（"snapshot" の 4 文字略）。

### 裁定

> - **slot4 B = 書込（write）で確定**（確度: 高）。出力は `"<引数文字列>.hkx"`。読込経路は本 EXE に存在しない。
> - **`'snap'` の上流出所 = 概念は Havok `hkpHavokSnapshot`（WorldSnapshot）系（確度: 高）、4CC 文字列そのものはゲーム側合成（確度: 中〜高）**。EXE 内に `"snap"` 文字列が 0 件で、即値合成＋`"SnapshotSave"` ラベルという組み合わせが「ゲーム側の命名」を支持。
> - 未解明: `'snap'` が Havok SDK 内の実在 4CC か（負証拠のみ）／ディスクリプタ `+4`/`+8` の厳密な意味（確度: 低）。

**判定: 解明（書込確定・上流概念同定）＋4CC の SDK 実在は負証拠のみ**（証拠 `v247_3_out.txt` / `v247_3b_out.txt` / `v247_3c_out.txt` / `v247_2e_out.txt`、v245 §3 継続、Web 5回）。

---

## §4 item4（V-245-9-4）dispatcher `FUN_0094A630`/`FUN_0094BE00` の静的到達経路と node2/node3 の SDK 名 — **解明: 静的鎖は「初期化関数 → node3 レジストリ（名前→factory）→ factory → オブジェクト vtable slot → update → wrapper → dispatcher」で閉じる（デッドコードではない）。node2/node3 の SDK 名は推定**

### 証拠A: dispatcher 自体への静的参照は 0、だが vtable slot 経由で到達する（`v247_4_out.txt` / `v247_4b_out.txt` / `v247_4d_out.txt`）

- `0x94A630`/`0x94BE00`/`0x94B7E0`/`0x94CC50` の **imm32 参照は全セクションで 0 件**、e8 は「自己 span 内 self-call」＋「wrapper 1本」のみ（v245 §5(3) を再現）。
- しかし **wrapper を呼ぶ update メソッドは vtable スロットに実在**する:
  - `0x91ADB0` = `.rdata@0xA9D408`（vt 先頭 `0xA9D3F8` の slot4）
  - `0x91C150` = `.rdata@0xA9D5B0`（vt 先頭 `0xA9D5A0` の slot4）
  - `0x91D010` = `.rdata@0xA9D708`（vt 先頭 `0xA9D6F0` の slot6）
  - `0x94CC50` の呼出し元 `0x91B73B` は `ConvexRadius` 系メソッド内（named_funcs 上は `FUN_0091b680+0xBB`。登録ラッパは `+0x1A` で ret 済みのため別関数本体）
- vtable 先頭 4 B は **COL でない**（`0xA9D3F4 = 0x00726577`='wer', `0xA9D59C = 0x73`='s'）＝RTTI 不在は node 系と同じ。

### 証拠B: 静的登録鎖（`v247_4b_out.txt` (a) / `v247_4c_out.txt` (a) / `v247_4h_out.txt` / `v247_4i_out.txt`）

```
FUN_0088B680+0x51
  → FUN_008CABF0  （13連続 call ＋末尾 jmp で計14＋別経路。node3 レジストリの静的初期化）
      → 登録ラッパ 0x91E980/0x91E760/0x91E090/0x91DB30/0x91D250/0x91CBE0/0x91BF90/0x91B680/
                    0x91AC40/0x919FB0/0x919440/0x919030/0x918DF0（＋0x946550/0x947630/0x9478F0 は別 init）
          → FUN_00947C50(registry=node3, name, factory)   ※ 重複 strcmp 検査つき
```

- 登録内容（**16 名 → factory の全数**、`v247_4c_out.txt` (a)）:
  `WorldMemory`→0x918D10 / `MousePicking`→0x918FE0 / `Islands`→0x919410 / `Tensor`→0x919F80 / **`SweptTransforms`→0x91AC10** / **`ConvexRadius`→0x91B650** / **`Shapes`→0x91BF60** / `CentreOfMass`→0x91CBB0 / **`Phantoms`→0x91CFE0** / `ForcedContactPoints (See Docs For Usage)`→0x91DAB0 / `ContactPoints`→0x91E010 / `Constraint`→0x91E700 / `Broadphase`→0x91E920 / `ObjectInspection`→0x946520 / `Statistics`→0x947600 / `DebugDisplay`→0x947860。
- factory → ctor → vtable（`v247_4f_out.txt` / `v247_4g_out.txt`）:
  - `SweptTransforms`: factory `0x91AC10`（alloc 0x3C）→ ctor `0x91AA10`（**vt `0xA9D3F8` を格納** @0x91AA49）→ update `0x91ADB0` → wrapper `0x94B7E0` → **dispatcher `0x94A630`（node4 `0xBA9510`）**
  - `Shapes`: factory `0x91BF60`（0x40）→ ctor `0x91BD70`（**vt `0xA9D5A0`** @0x91BDAC）→ update `0x91C150` → `0x94B7E0` → `0x94A630`
  - `Phantoms`: factory `0x91CFE0`（0x3C）→ ctor `0x91CD70`（**vt `0xA9D6F0`** @0x91CD94）→ update `0x91D010` → `0x94B7E0` → `0x94A630`
  - `ConvexRadius`: factory `0x91B650`（0x3C）→ ctor `0x91B4B0` → update `0x91B6A0` 帯（`0x94CC50` 呼出し @0x91B73B）→ wrapper `0x94CC50` → **dispatcher `0x94BE00`（node5 `0xBA9514`）**

### 裁定（§4 前段）

> - **dispatcher は「デッドコード」ではなく、node3 レジストリに名前登録された表示システムを実行時に生成したときだけ到達する vtable 経由の実コード**（確度: 高）。
> - **静的到達経路 = `FUN_0088B680` → `FUN_008CABF0`（一括登録）→ レジストリ → factory → ctor(vt) → update slot → wrapper → dispatcher**。dispatcher エントリ自身への静的参照が 0 なのは、呼出し点が「動的生成オブジェクトの vtable」だから（v245 の「実行時構築かデッドか」を **実行時構築** と裁定）。
> - どの表示システムがどちらの表を使うか: **node4 (`0xBA9510`) ← SweptTransforms / Shapes / Phantoms、node5 (`0xBA9514`) ← ConvexRadius**（他12システムは update 内で dispatcher を呼ばない＝別経路）。

### 証拠C: node2/node3 の SDK 名（推定・Web 裏付け）

- **node2（`0xBA94E8`）= 64bit tick タイマのシングルトン**。`hkStopwatch` が Havok の「64bit タイマ」で `start()/getElapsedSeconds()` を持つ（Web: marcoarena 2011 / engine-showcase のデモが `#include <Common/Base/Types/hkStopwatch.h>` を使用）。rdtsc＋QPC 校正＋`1000000.0` での µs 換算という本実装はこの系譜に一致。
  → **候補: `hkStopwatch`（第一）／`hkSystemClock` 系（第二）。確度: 中**（RTTI 不在・vtable-4=`0x7070` 文字列残骸のため確定不可）。
- **node3（`0xBA950C`）= Havok デバッグ表示システムの「名称→factory」レジストリ**。登録16名が **hkpWorld のデバッグ表示名（Shapes/Broadphase/ContactPoints/Islands/WorldMemory/CentreOfMass/Phantoms/Constraint/…）と完全一致**すること、各 factory が `hkReferencedObject` レイアウト（`+4`=u16 size, `+6`=u16 refcount）でオブジェクトを生成することが機械的に確認できた。
  → **候補: `hkProcessFactory`／`hkDebugDisplay` 系レジストリ。確度: 中〜高（機能）、正式クラス名は推定**。

**判定: 解明（静的到達鎖を全数確定）＋SDK 名は推定**（証拠 `v247_4_out.txt` / `v247_4b_out.txt` / `v247_4c_out.txt` / `v247_4d_out.txt` / `v247_4f_out.txt` / `v247_4g_out.txt` / `v247_4h_out.txt` / `v247_4i_out.txt`、Web 2回）。

---

## §5 item5（V-245-9-5）descriptor `+0xC` のマップ値経路（`FUN_009546D0` 経由・理論残） — **判定: 理論経路も陰性（値アクセサの戻り値を `+0xC` で読む箇所 0 件）。v245 §4 の write-only 裁定を補強**

### 証拠A: `FUN_009546D0` の正体は「挿入ループ」で、読手ではない（`v247_5_out.txt`）

- `FUN_009546D0`（全 0x37 B・`ret 8`）は **2つの NULL 終端配列を突き合わせて map へ `call [edx+8]`（slot2 = insert）するだけ**。フィールドを読む命令は `[eax+8]`（キー側）のみで **`+0xC` の read は 1 件も無い**。
- e8 呼出し元は **`0x94315A`（`FUN_00943070+0xEA`）の1件のみ**。`FUN_00943070` は「map owner」の ctor で、
  - `+8` = map A（vt `0xAA2738`、配列 `0xA9AA48`=list2 94件を挿入）
  - `+0xC` = map B（vt `0xAA2748`、配列 `0xA9A910` を挿入）
  - `+0x10` = map C（vt `0xAA2758`、`FUN_009546D0` で A×B の対応を構築）
  を生成する（`v247_5b_out.txt`）。

### 証拠B: map のメソッドは直接呼出し 0 件＝すべて vtable 経由（`v247_5d_out.txt` / `v247_5e_out.txt`）

| メソッド | imm32 参照（vtable slot のみ） | e8 直接呼出し |
|---|---|---|
| `0x942F20`/`0x942F50`（map A insert/lookup） | `.rdata 0xAA2740` / `0xAA2744` | **0** |
| `0x942F70`/`0x942F90`/`0x942FC0`/`0x942FD0`（map B/C） | `0xAA2750`/`0xAA2754`/`0xAA2760`/`0xAA2764` | **0** |
| `0x943410`（owner slot2 = 値アクセサ） | `.rdata 0xAA2770` | **12** |
| `0x943070`（owner ctor） | （e8 0） | **0**（動的生成） |

- つまり **値を返し得る唯一のアクセサは owner slot2 `0x943410`**（`FUN_00943070` が作る owner オブジェクトの getter）。他の map メソッドは直接呼出しが無く、vtable 経由のみ。

### 証拠C: `0x943410` の全12呼出し元で「戻り値 +0xC」の read は 0 件（`v247_5f_out.txt`）

- 12 caller（`0x4EA855/0x4EA869/0x50881A/0x67E042/0x67E4BE/0x67E897/0x68C5D2/0x72316F/0x723300/0x77AAB5/0x77AAD0/0x77C828`）を call 前 0x28 B＋call 後 0xC0 B で線形逆アスし、`+0xC` 形のメモリ読みを全数抽出。
- 検出された `+0xC` は **すべて `[esp+0xC]`（スタック）**で、`[eax+0xC]`/`[reg+0xC]`（戻り値＝descriptor 由来）は **0 件**。
- ecx 起源も全て owner 系（`[0xB333A0]+0x5C +0x20` / `[esi+0x24]` / `[esi+0xBC]` / 局所 owner）で、**D-set デスクリプタ直接ではない**（v245 §4 証拠D の 43 caller 全数結果と整合）。

### 裁定

> - **descriptor `+0xC` のマップ値経路も陰性**。値アクセサ `0x943410` の戻り値を `+0xC` で読む箇所は 12 caller 全数で 0 件、map メソッドは直接呼出し 0 件（vtable 経由のみ）で、`FUN_009546D0` は挿入専用。
> - したがって v245 §4 の **「`+0xC` は write-only（live 読手 0 件）」を、動的マップ経路まで含めて補強**（確度: 高／理論残は「未知の間接 vtable 呼出しが map を列挙して値を返す」場合のみで、その経路も owner の既知 vtable（5 メソッド）からは観測されない。確度: 中）。
> - 残: map owner の生成元（`0x943070` の動的呼出し元）は未特定（e8 0 件・関数ポインタ表に出現せず）。ただし owner の利用は 12 caller に閉じており、`+0xC` read は無い。

**判定: 解明（理論経路も陰性・write-only 補強）**（証拠 `v247_5_out.txt` / `v247_5b_out.txt` / `v247_5c_out.txt` / `v247_5d_out.txt` / `v247_5e_out.txt` / `v247_5f_out.txt`、v245 §4 継続）。

---

## §6 まとめ・新規残点・自己検証

### 6-1. 本バッチ（§1〜§5）の判定一覧

| # | 残点（v245 §8） | 判定 | 確度 | 主証拠 |
|---|---|---|---|---|
| §1 | OEP `0xBAF2ED` → CRT startup の間接経路 | **解明**（`.bind` 複号ブートストラップ、7ピース再実測） | 高 | `v247_1(_b)_out.txt`, v126 |
| §2 | packfile B の SDK 正式名＋`__classindex__`/`__dataindex__` の出所 | **部分解明**（B=binary packfile writer 確定／正式名=推定／出所=B 私有リテラルで確定） | 機能=高・名前=中 | `v247_2*,v247_2b..2h` |
| §3 | `'snap'` 上流出所＋slot4 B の書込/読込 | **解明**（slot4=**書込**確定／概念=`hkpHavokSnapshot` 系／4CC はゲーム側合成） | 書込=高・上流=中〜高 | `v247_3,3b,3c` |
| §4 | dispatcher 静的到達＋node2/node3 SDK 名 | **解明**（静的鎖を全数確定＝デッドコードでない／SDK 名は推定） | 到達=高・名前=中 | `v247_4,4b..4i` |
| §5 | descriptor `+0xC` のマップ値経路 | **解明**（理論経路も陰性＝write-only 補強） | 高 | `v247_5,5b..5f` |

**本バッチの新規確定事項（要点）**
1. OEP→CRT は `steamdrm!start()` の戻り値（暗号ヘッダ `0x318` = `0x98787C`）を `jmp eax` で渡す経路で、静的参照は 0（§1）。
2. packfile B は **magic `0x57E0E057`/`0x10C0C010`・version=2 の binary packfile を実際に書く**（`FUN_0090DA40`）。EXE に**リーダは不在**（magic 定数は各1件）（§2）。
3. B の 3 セクション名は `__classnames__`/`__classindex__`/`__dataindex__` で、**B ctor 以外に参照が無い B 私有リテラル**。標準名 `__types__`/`__data__` は別系統（base/XML 側）が使用（§2）。
4. スナップショット書出しは **`"<引数>.hkx"` を新規作成**する経路（`fn@0x444740`）で、contents は `hkRootLevelContainer`＋`hkPhysicsData`、タグは即値合成の `'snap'`＋ラベル `"SnapshotSave"`（§3）。
5. dispatcher 到達鎖は **`FUN_0088B680` → `FUN_008CABF0`（一括登録14連）→ node3 レジストリ（16名）→ factory → ctor(vt) → update slot → wrapper → dispatcher** で閉じる（§4）。
6. descriptor `+0xC` は **マップ値経路まで含めて live 読手 0 件**（§5）。

### 6-2. 新規残点（次バッチ候補）

1. **[残] `__classindex__`/`__dataindex__` が Havok 4.0 期の正式名か**（低）: 公開資料は 5.0+ 前提で負証拠のみ。EXE に `Havok-x.y` 版文字列が 0 件のため版特定不可（§2 裁定 (ii) の従属説を棄却できない）。
2. **[残] `'snap'` 4CC の SDK 実在**（低）: `hkpHavokSnapshot` 系の概念は確認できたが、4CC 文字列自体は Web・EXE 双方で 0 件（§3）。
3. **[残] node2/node3 の SDK 正式クラス名**（低）: RTTI 不在・vtable-4 非 COL のため機械確定不可。候補 `hkStopwatch`／`hkProcessFactory`（§4）。
4. **[残] map owner（`FUN_00943070`）の生成元**（低）: e8 0 件・関数ポインタ表にも出現せず、動的生成元が未特定（§5 裁定の注記）。
5. **[残] node4/node5 の vtable slot2..4 が定数（`0x34`/`FLT_MAX`/`π`）である理由**（低）: v245 §5(3) の未了点。`hkReferencedObject` レイアウト（`+4`=u16 size / `+6`=u16 refcount）は本バッチで複数クラスに確認済みだが、当該 vt の実 slot 数は未確定。

### 6-3. 自己検証

- **selftest**: 着手時 `probe_harness.py selftest` = **4/4 PASS**、完了時にも再実行して **4/4 PASS**（`ALL PASS`）。
- **中断対策**: §1 は無改変で保持し、**§2→§3→§4→§5 を1項ごとに即追記**（書き溜め無し）。本 md は §1 の 5,215 B から §6 まで追記。
- **VA 換算**: `imm32_refs3()`（セクション個別 raw）を使用。E8 走査は `e8_callers`（.text raw 起点 0x400C00 換算）。v245 §8 の罠（raw/VA 混同）は踏襲回避。
- **数値の相互整合**: `magic 0x57E0E057`（§2）と `slot4=write`（§3）は同一関数 `FUN_0090DA40` を根拠に一致。`SweptTransforms/Shapes/Phantoms→0x94A630`、`ConvexRadius→0x94BE00`（§4）は v245 §5(3) の「node4=0xBA9510 / node5=0xBA9514」と一致。
- **負証拠の明示**: Web は `websearch` 5回＋`webfetch` 3回（grep.app 429 / searchcode 404＝**取得失敗**として明記）。`'snap'`・`__classindex__`・`__dataindex__` の不在は「確認できなかった」であり「存在しない証明」ではない。
- **制約遵守**: git 書込み 0・docs/GT/TODO/session-memory/plan/cache/ソース編集 0・ESM 再スキャン 0・既存 `tmp/re/*.md` 上書き 0（**新規/追記は本 v247 のみ**）。新規中間物は `%TEMP%\opencode\v247_*`（`v247_2..2h`, `v247_3..3c`, `v247_4..4i`, `v247_5..5f`＋各 `_out.txt`）。

### 6-4. 使用証拠一覧

- **本体**: `tmp/re/v247_process_havok_residuals9.md`（本ファイル・§1 は前回分を無改変保持）
- **入力（読取のみ）**: `v245_process_havok_residuals8.md` §3〜§8 / `v243_process_havok_residuals7.md` / `v126_bind_crt_bootstrap.md` / `tmp/re/cache/{named_funcs,vtable_index,call_targets}`
- **本バッチ出力**（`%TEMP%\opencode\`）:
  - §1: `v247_1_out.txt`, `v247_1b_out.txt`
  - §2: `v247_2_out.txt`, `v247_2b_out.txt`, `v247_2c_out.txt`, `v247_2d_out.txt`, `v247_2e_out.txt`, `v247_2f_out.txt`, `v247_2g_out.txt`, `v247_2h_out.txt`
  - §3: `v247_3_out.txt`, `v247_3b_out.txt`, `v247_3c_out.txt`
  - §4: `v247_4_out.txt`, `v247_4b_out.txt`, `v247_4c_out.txt`, `v247_4d_out.txt`, `v247_4e_out.txt`, `v247_4f_out.txt`, `v247_4g_out.txt`, `v247_4h_out.txt`, `v247_4i_out.txt`
  - §5: `v247_5_out.txt`, `v247_5b_out.txt`, `v247_5c_out.txt`, `v247_5d_out.txt`, `v247_5e_out.txt`, `v247_5f_out.txt`
- **Web**: reshax「Havok middleware」（セクション3種・hkxHeaderData struct）/ lukascone 2024（同）/ HedgeLib `HavokXML.cs` / hkxcmd README / GameDev.net #628051（`hkpHavokSnapshot::load`・WorldSnapshot デモ）/ marcoarena 2011（`hkStopwatch` = 64bit timer）/ engine-showcase `hkStopwatch.h`
