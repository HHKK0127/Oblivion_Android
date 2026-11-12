# V55: GMST 上書き/解決経路の特定 — Oblivion.exe v1.2.0416 (Steam)

作成日: 2026-10-24 / 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
証跡: `tmp\re\v55_b37b88_refs.txt` / `v55_disasm_key.txt` / `v55_dispatch_internals.txt` / `v55_gmst_manager.txt` / `v55_gmst_vtable.txt` / `v55_esm_apply.txt` / `v55_apply_path.txt` / `v55_final_verify.txt` / `v55_last_checks.txt` / `v55_decompiled_scan2.txt`

---

## 0. セルフテスト 4件 PASS

| # | 検証 | 結果 |
|---|------|------|
| 1 | `bytes@0x504050` = `518b4c24188b5424` | PASS |
| 2 | `bytes@0x4FB580` = `578b7908` | PASS |
| 3 | `string@0xA7035C` = `.\AI\CombatController.cpp` | PASS |
| 4 | オペコードテーブル 基底`0xB0AF48`/stride`0x28`/532件、opid `0x0015` impl = `0x504560` | PASS (count=532, impl一致) |

**マッピング補足（重要・ground truth の修正）**: `VA = 0x400C00 + file_off` は **.text 専用**。PE セクションヘッダ実測ではセクション毎に delta が異なる（.text=0xC00 / .rdata=0xE00 / .data=0x1400、VA=file_off+0x400000+RVA 差）。セルフテスト③④は最初 0x400C00 一律適用で FAIL し、`re_util.py` の pefile RVA 変換（`get_offset_from_rva`）使用で PASS。以降の全走査は pefile 方式。

---

## 1. 中央マップ `0xB35574` の正体（新規確定）

`0xB35574` は独立マップではなく **GMST/GameSetting マネージャシングルトン `0xB35468` の埋め込みハッシュマップ**（`this+0x10C`）。

- 初期化: 静的初期化 `0x9E06B0`: `push 0x191(=401); mov ecx,0xb35468; call 0x4A8330`(ctor) → vtable 書き換え `mov [0xb35468],0xa4049c` (`0x9E06C4`)。dtor thunk `0xA1AA00` (`mov ecx,0xb35468; jmp 0x4A8180`)。
- ctor `0x4A8330`: `[this]=0xa40474`(base vtable)、`lea edi,[this+0x10c]; call 0x4A7E60(401)` = 埋め込みマップ構築 → **`0xB35468+0x10C = 0xB35574`**。
- マップ構造 (ctor `0x4A7E60`): `[this+0]`=vtable(0xA403A8/0xA403F0)、`[this+4]`=bucket数401、`[this+8]`=bucket配列(`0xB3557C`)、`[this+0xC]`=count、`[this+0x10]`=重複許可フラグ。 （→ 訂正追記 2026-10-24 V63: 実際の vtable は `0xA40454`（hash=`FUN_004A80A0`=djb2、compare=`0x584D10`=`__stricmp`）。0xA403A8/0xA403F0 は取り違え（基底クラスの一時値））
- マネージャ vtable `0xA4049C` 主要スロット: `[0]=0x412E20`(Add: lookup→dup警告→insert 0x412D30)、`[1]=0x404FF0`(Remove: `add ecx,0x10c; jmp 0x4524C0`)、`[4]=0x4A7D60`(**ESM値適用**)、`[6]=0x4A7CB0`(record ptr クリア)、`[9]=0x4A8040`(**slot9 = ESM レコード適用入口**)。

---

## 2. GMST レコード読取ルーチン（特定結果）

ESM 読取ディスパッチ: type table `0xB05E04` idx3 = "GMST" → `0x44DE2E` の `jmp [ecx*4+0x44E6A0]` → **ハンドラ `0x44E326`**（jump table エントリ `0x44E6A4`、s69 の idx3 handler と一致）。

**処理系 4 関数（EDID キー＋DATA 値の読取を担当）**:

| VA | 役割 | decompiled/逆展開引用 |
|----|------|----------------------|
| `0x44E326` | GMST レコードハンドラ: `call 0x4510B0`(現在サブレコード tag) → `cmp eax,"EDID"` → EDID 文字列を `0x450C20` でスタックへ読取 → `mov edx,[0xb35468]; mov eax,[edx+0x24]`(slot9) → `push esi(edid); push edi(record); mov ecx,0xb35468; call eax` | v55_dispatch_internals.txt |
| `0x4A8040` | slot9: `[this+0x108]=record` → EDID を埋め込みマップ(`this+0x10c`=`0xB35574`)へ `call 0x55E000`(lookup) → 命中時のみ `call [vtable+0x10]`=`0x4A7D60` → 末尾 `call [vtable+0x18]`=`0x4A7CB0`(record ptr クリア)。**未命中なら insert しない**（mod 追加 GMST は無視） | v55_esm_apply.txt |
| `0x4A7D60` | 値適用: `esi=[this+0x108]`(record) → `call 0x44FEA0`(次サブレコードへ前進=EDID→DATA) → `call 0x4510B0; cmp eax,"DATA"` → 引数=マップ node value(=設定変数オブジェクト) → 型判別 `0x4A78D0`(key 先頭文字: 'f'→5 / 'i'→3 / 's'→6) → int/float: `call 0x4510E0`(4バイト DATA を**設定変数オブジェクト本体へ転送**) / string: `0x450C20`+`0x4A7980`(String set) | v55_apply_path.txt |
| `0x450C20` / `0x4510E0` / `0x44FEA0` | サブレコード読み取り原語: 現在サブレコード n バイトを dest へコピー / 4バイト版 / 次サブレコードへ前進(+6 ヘッダ、圧縮 0x40000 フラグ時 `[record+0x418]` 使用) | v55_final_verify.txt |

**結論**: GMST 読取は「EDID をキーに中央マップ lookup → 命中した設定変数へ DATA を直接書込む」。**ESM ロード時にマップへ insert は一切しない**。

---

## 3. `0xB37B88` (fDifficultyDamageMultiplier) への書込み箇所（全走査）

バイナリ全体を imm32 スキャン（証跡 v55_b37b88_refs.txt）。参照は3件のみ:

| VA | 種別 | 内容 |
|----|------|------|
| `0x9ED790` | **初期化(既定値 10.0f)** | `fld dword [0xa31c80]`(=10.0f) → `push 0xa5b128`("fDifficultyDamageMultiplier") → `mov ecx,0xb37b88` → `call 0x41BAE0`(float setter)。setter 本体が `fstp [esi]` で**変数へ 10.0f を書き**、キーを `0xB35574` へ insert（value=変数ポインタ）。 |
| `0x5E25B6` | **runtime 読取** | `FUN_005E2560`(戦闘ダメージ倍率): `fmul dword ptr [0xb37b88]` — 変数を直接読み（マップ lookup なし）、ESM 適用後の値がそのまま使われる。decompiled L347461 `fVar2 = *(float *)(DAT_00b333c4 + 0x734) * _DAT_00b37b88;` |
| `0xA1FE31` | dtor thunk | `mov ecx,0xb37b88; jmp 0x4055E0`(shutdown 時にマップ `0xB35574` から削除 0x4524C0 + 文字列 free)。 |

**絶対アドレスによる runtime 書込みは0件**。ESM 値(5.0f)は **node value ポインタ経由の間接書込**（`0x4A7D60` → `0x4510E0` → `0x450C20` が `*[node.value]` = `0xB37B88` へ 4 バイト転送）で入る。よちは「0xB35574 系へ runtime insert が無い」という既存観測と両立する。

**第2の runtime 書込経路（新規確認）**: コンソール SetGS `0x514100` 系: lookup 後、float は `call 0x985fb2`(atof) → `fstp dword [esi]`(`0x5141DE`)、int は `0x983479`(atoi) → `mov [esi],eax`、string は `0x4A7980` — 同じく**変数本体へ直接書込**（マップは不変）。GetGS `0x513FC0` 系は lookup+読取のみ（"GameSetting %s >> %.2f" 等、書式 `0xA4D740/0xA4D758/0xA4D770`）。

---

## 4. 分類表: `FUN_0044ac20` と 上位 `0x55E000` 呼出元 / insert `0x412D30` 呼出元

### 4.1 `0x55E000`（lookup: hash→bucket→連結走査、node{next@0,key@4,value@8}、命中時 `*out = node[2]`。**書込なし** — 実装 0x55E000-0x55E04E 確認）

| 関数 | 0x55E000 呼出 | 対象マップ | 分類 |
|------|--------------|-----------|------|
| `FUN_0044AC20` | 29 | **`0xB0613C`**（`mov ecx,0xb0613c` 固定、整数キー 0x1a/0x19/0x194…） | **(c) 別処理**: グローバル作成オブジクト(eyeReanimate, VampireRace 等)の FormID→object レジストリ。lookup 失敗時は factory `0x9832E6` で生成（マップ `0xB35574` とは無関係） |
| `FUN_006C7470` | 7 | `[ebx]`（per-object マップ） | (a) lookup のみ（node value 受け取り後、`[edi+0x14]` 配列走査） |
| `FUN_00459AF0` | 6 | `esi`（per-object マップ） | (a/c) lookup + `node[4]==0` 時のみ `0x4524C0` 削除（クリーンアップ） |
| `FUN_004DEEE0` | 6 | `ebx`（per-object マップ） | (a+create) lookup + factory `0x9832E6` |

→ **`0xB35574` を lookup する runtime ユーザは**: ESM slot9 `0x4A8040`(書込を伴うが insert なし)、コンソール GetGS/SetGS `0x513FC0/0x514100`、INI/その他既知の取得系のみ。

### 4.2 insert `0x412D30` 呼出元 47件（ecx マップ別）

| ecx マップ | 呼出元 | runtime? |
|-----------|--------|----------|
| **`0xB35574`** | `0x419AF5`(obj setter 0x419A70 内) / `0x41BB65`(float setter 0x41BAE0 内) / `0x412E65`(slot0 Add 0x412E20 内 — vtable 経由、E8 直呼び0件) | **全件 静的初期化のみ**（setter 呼出元は初期化群。slot0 への runtime E8 なし） |
| `0xB06164` | `0x44B623/0x44C67F/0x44C75D/0x44C836/0x44C916/0x44C9F2/0x44CACB/0x44F2D9` + `0x5180E1`〜`0x519305`(28件) | レコード型名レジストリ（別マップ） |
| `0xB13948` | `0x585291`("GameYear" 等 GLOB) | 別マップ（GLOB 管理） |
| レジスタ経由 | `0x521679/0x6C5836/0x7125A0/0x749673/0x749A32/0x77C423/0x77CB3D/0x77F79F` | per-object マップ |

→ **`0xB35574` への runtime insert は0件**（ground truth と一致）。`0x412D30` は汎用ハッシュマップメソッドで、多数のマップインスタンスが共有。

---

## 5. 判定: 上書き経路 = **(b) 別オブジェクト（個別変数オブジェクト）に上書き**

**証拠**:

1. `0xB35574` マップは runtime 不変（insert 呼出元 47 件のうち `0xB35574` 実体は静的初期化の setter 3 経路のみ。slot9 `0x4A8040` は lookup→適用のみで insert 呼び出し無し）。
2. ESM GMST 適用は `0x44E326`→`0x4A8040`(lookup EDID)→`0x4A7D60`(DATA 前進・型判別)→`0x4510E0`/`0x450C20` で **node value = 設定変数オブジェクト本体へ 4 バイト（float/int） or String set** を実行。`fDifficultyDamageMultiplier` では `0xB37B88+0` の float フィールドが 5.0f に直接置換される。
3. runtime 読取 `0x5E2560` は `fmul dword [0xb37b88]` と**変数を直接参照**（読み取り時に ESM 値を優先する `(c)` 型の分岐は存在しない）。
4. 既定値 10.0f の初期化元は静的初期化 `0x9ED780`（setter `0x41BAE0` が `fstp [esi]` で変数へ書く）。ESM ロード後の 5.0f は上記間接書込。第3の経路としてコンソール SetGS `0x5141D4`(atof→`fstp [esi]`) も変数本体へ書く。

**(a) ではない**: runtime insert が無い。(c) ではない: 読取側に ESM 優先ロジックが無く、変数そのものが書き換えられる。

---

## 6. GMST キーと `0xB35574` の関係 = **(aa) 同一マップ共有**

**根拠**: float setter `0x41BAE0` はキー文字列（例 `0xA5B128`="fDifficultyDamageMultiplier"）で `0xB35574` に insert（value=変数ポインタ）。ESM ハンドラ `0x44E326` が渡す EDID を slot9 `0x4A8040` が **同じ `0xB35574`**（`this+0x10c`）へ lookup して同じ変数を更新する。GMST の f\*/s\*/i\* 2,175 キーは EXE 静的初期化でこのマップに登録済みで、ESM の GMST editorID と同一命名空間・同一マップ。他の 129 エントリ（AV 名/Desc/Icon 等）も同マップに別 setter で登録されるが、ESM GMST は f/s/i 型キーに限定して命中する。

---

## 7. 移植示唆（1〜3 行）

1. 既定値を「変数オブジェクト（value + key）」として登録し、key→変数ポインタの map を構築する（EXE の setter 相当）。
2. ESM GMST ロード時: EDID で map lookup → 命中した変数へ DATA を直接書く（float/int は 4 バイト、string は代入）。**map への insert は行わない**（未知キーは無視すれば原本互換）。以降の読み取りは変数を直接参照するだけで ESM 値が効く。
3. 順序: 静的既定値の登録 → ESM 上書き。コンソール SetGS 相当も同じ変数フィールドに書けばよい。

---

## 8. 未解決点

- slot0 `0x412E20`（Add ラッパ）の vtable 経由 runtime 呼出元は未特定（E8 直呼び0件。ESM で新規 GMST キーを追加する経路は確認できず = 未知キーは無視される実装と判断。mod 追加 GMST が本当に無視されるかは実機検証が望ましい）。
- `0x4A78D0` 型判別テーブルは 'f'→5/'i'→3/'s'→6 を実字节で確認したが、他の先頭文字（'S' 系 AV 名等）の全分類は未列挙。
- slot9 呼出後 `[this+0x108]` クリア (`0x4A7CB0`) までの間に record イテレータが DATA 以外（圧縮レコード内）の場合の挙動は `0x44FEA0` の圧縮分岐のみで部分確認。
- ESM GMST のうち EXE 未登録キー（シリアライズ済みセーブ経由等）が lookup 失敗時に無視される点のゲームプレイ影響は未評価。
