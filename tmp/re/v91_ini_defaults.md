# V91: 0xB07BF0 INI 設定ノード全走査＝既定値テーブル復元（Oblivion.exe v1.2.0416 Steam）

- 作成日: 2026-10-25 / タスク: V91（優先度: 中・移植実装への実用価値最大）
- 手法: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（作業前・作成後の 2 回実行、ground truth: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。rd() はセクション対応（.text const=0x400C00 / .rdata const=0x400E00 / .data const=0x401400、bss は None+警告）、dec_index=20,465、enclosing_fun() で VA→ラベル。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（pefile なし・struct 手動パース）。逆アセンブルは capstone 5.0.7（skipdata=True）。decompiled は部分検索のみ（ラベル小文字 hex）。
- 成果物は **新規 2 ファイルのみ**: 本ノート `tmp/re/v91_ini_defaults.md` と `tmp/re/v91_ini_defaults.csv`（635 データ行 / 92,421 B）。既存 docs・tmp/re・TODO・session-memory・plan.md の編集、git、memory 追記、sync ツール実行は一切なし。
- 中間スクリプト/出力はすべて `%TEMP%\opencode\v91_*.py` / `v91_*_out.txt`（Git 管理外）。

---

## §0 結論（要約）

1. **ノード構造は 8 バイト固定**: `{ u32 value; const char* name; }`。`name` は **`"Key:Section"`**（区切りは `:` 1 文字、`.rdata` に ASCII 文字列として格納）。`:` 無しの 4 件はセクション **`MAIN`**（既定文字列 0xA404C4）に落とされる。
2. **登録済みノードは 635 個**（3 コンテナ: 0xB07BF0=572 / 0xB11C40=62 / 0xB07D28=1）。CSV はこの 635 行。
3. **`value` は int 固定ではない**: name 先頭文字で型が決まる **union**（`0x4A78D0`）。実測分布 float 238 / bool 173 / int 103 / string 99 / unsigned 21 / color(`r`) 1。
4. **孤立ノードは 0 件**。ファイルイメージでは head/chain とも 0 で全ノード「到達不能」だが、これはリストが **CRT 静的初期化時に構築**されるためで、静的到達不能＝孤立ではない。
5. **ノード読手（lookup）= `FUN_0050EC30`**（`_stricmp` で名前照合）。呼出し元はコンソール `GetINISetting` / `SetINISetting`。INI 読込は `Begin → LoadAll → End` の 1 段階で `LoadOne` が `node->value` を直接書換える。

---

## §1 構造レイアウト（タスク (a)）

### 1.1 ノード（.data の静的レコード、8 B）

```
+0x00  u32 value   // 型は name 先頭文字で決定（int / unsigned / bool / float32 / char* 既定文字列）
+0x04  char* name  // .rdata の "Key:Section"（NUL 終端 ASCII）
```

- 呼び出し側の工学的証拠（どちらのフィールドが name か一意に決定）:
  - `FUN_004040E0`（登録本体）: `mov eax,[ebx+4]` → `_stricmp(..., eax)` = **+0x04 は name**。
  - `FUN_00404850`（float ノード工場）: `fld [esp+0x20]; fstp [esi]`（**+0x00 = float value**）、`mov eax,[esp+0x1C]; mov [esi+4],eax`（+0x04 = name）、`ret 8`。
  - `FUN_00444060`（int ノード工場）: `mov [esi+4],arg1; mov [esi],arg2`、`ret 8`。
  - ゲッタ/セッタ `0x403BE0`/`0x403C00` は `node` を受け `&node->value` を返す（GetINISetting の switch 内）。
- **name 形式**: `strchr(name, ':')` で分割（`0x982EC0`）。
  - `0x4A8430`（セクション抽出）: `:` 以降をコピー、無ければ `0xA404C4 = "MAIN"`。
  - `0x4A8480`（キー抽出）: `:` 以前をコピー（`strncpy` + NUL）。
  - 実データ例: `iDebugTextLeftRightOffset:Display` / `bUse Joystick:Controls`（キーに空白あり）/ `fLowPerfCombatantVoiceDistance`（`:` 無し）。
- **value の型判定 `0x4A78D0(name)`**（`.text` 0x4A78DD-0x4A7928、テーブル: ジャンプ表 0x4A792C + バイト表 0x4A7954、`c-'S'` をインデクス）:

  | 先頭文字 | 戻り type | 実測件数 | 既定値の実体 |
  |---|---|---|---|
  | `b` | 0 | 173 | u32 の下位バイト 0/1（例 0xB13208 系は byte 書込み） |
  | `c` | 1 | 0 | （登録ノードに出現なし） |
  | `h` | 2 | 0 | 同上 |
  | `i` | 3 | 103 | signed int |
  | `u` | 4 | 21 | unsigned int |
  | `f` | 5 | 238 | IEEE754 float32 |
  | `s`/`S` | 6 | 99 | char* 既定文字列（.rdata / 1 件のみ .data） |
  | `r` | 7 | 1 | 色 0xAARRGGBB（`rDebugTextColor:Menu`=0xFFFBE9FF） |
  | `a` | 8 | 0 | 同上 |
  | その他 | 9 | 0 | （"UNKNOWN TYPE"） |

  ※ 上記「実測件数」は本タスクの 635 ノード集計（合計 635）。

### 1.2 コンテナ（設定コレクション本体）

| VA | RTTI クラス名 | vtable | 登録ノード | バックエンド |
|---|---|---|---|---|
| 0xB07BF0 | `.?AVRegSettingCollection@@`（+0x114） | 0xA404F8 | **572** | INI（Oblivion.ini） |
| 0xB07D28 | `.?AVDefaultCombatStyle@@` | 0xA40590 | **1**（sBasePath） | レジストリ（RegCreateKeyExA 系、V51 記載） |
| 0xB11C40 | `.?AVAtmosphere@@` | 0xA566A4 | **62** | INI（BlendSettings.ini、V51 記載） |
| 0xB07AD8 | （RTTI 無し） | 0xA40548 = **INI backend と同一 9 スロット** | **0** | 未使用（死蔵候補） |

オブジェクトレイアウト（0xB07BF0 を基準、ファイルイメージは vtable 以外すべて 0）:

```
+0x000  vtable (0xA404F8)
+0x004  char path[0x104]   // INI フルパス。0xB07BF4 へ dir[0xB3F280] + "Oblivion.ini"[0xB02C84] をコピー
+0x108  u32 handle/flag     // INI backend: Begin(+0x14) で this を代入 / End(+0x18) で 0
+0x10C  Node* head          // リスト先頭ノード（ファイルイメージ = 0）
+0x110  Record* chain       // 残り要素の 8B レコード連鎖（ファイルイメージ = 0）
+0x114  （この直後から RTTI TypeDescriptor が続く。オブジェクト本体は 0x114 B）
```

### 1.3 リスト構造（+0x10C / +0x110 の正体）

`FUN_004040E0` と `0x446CB0` の逆アセンブルで一意に決定（v80 の「head/tail」記述の精緻化 = **先頭ノード + 残り連鎖**、tail ではない）:

```c
// this = 0xB07BF0, node = 登録ノード
bool AddSetting(node* n) {                       // vtable[0] = 0x4040E0
  if (this->head == 0 && this->chain == 0) goto insert;   // cmp [ecx+0x110],0 / cmp [edi],0
  for (Rec* r = (Rec*)&this->head; r; r = r->next)        // r の +0 が常に Node*
      if (_stricmp(r->node->name, n->name) == 0)          // 0x98262D = _stricmp（大文字小文字無視）
          { Warning("Setting key '%s' already used in list.\nSetting keys must be unique.\n"); return; }
insert:
  if (this->head) {                                // 0x446CB0: malloc(8) = 0x401F00
      Rec* nr = malloc(8); nr->node = this->head; nr->next = this->chain;   // {old_head, old_chain}
      this->head = n; this->chain = nr;                                 // ← 先頭 prepend
  } else { this->head = n; this->chain = 0; }
}
```

- ノードは **先頭挿入**なので、実行時の走査順 = **登録順の逆**。
- 重複登録時の出力は `0x4A7A60` → `0x4941A0`（Warning）で `0xA2FAB0 = "Setting key '%s' already used in list.\nSetting keys must be unique.\n"`。
- 削除は vtable[4] `0x403C20 = add ecx,0x10C; jmp 0x65C620`（汎用リスト除去ヘルパ。E8 呼出し 854 件 = 本設定以外のリストも共用）。
- 走査例（vtable[8] `0x4A8B10` のカウント部）: `lea esi,[ebx+0x10c]; mov ecx,[esi]; ...; mov esi,[esi+4]; test esi,esi; jne` — head→chain の同一パターン。

### 1.4 vtable 9 スロット（0xA404F8 / 0xA40548 / 0xA566A4 は同一、0xA40590 のみ +0x08..+0x18 が差異）

| slot | INI backend | 役割（本タスクで確認） |
|---|---|---|
| +0x00 | 0x4040E0 | AddSetting（重複検査 + prepend） |
| +0x04 | 0x403C20 → 0x65C620 | Remove(node) |
| +0x08 | 0x4A8B10 | Dump/件数出力（"INISettings" 0xA40580 / "Total INISettings" 0xA4056C、ノード毎に 0x4A7B10 で整形） |
| +0x0C | 0x4A85F0 | **WriteOne**: 型別 sprintf（`%d` 0xA2F860 / `%.4f` 0xA40538 等）→ `WritePrivateProfileString`（[0xA280B0]） |
| +0x10 | 0x4A8800 | **ReadOne**: 0x4A8430/0x4A8480 で Key/Section 分割 + 0x4A78D0 型判定 → `GetPrivateProfile*`（[0xA280B4] を 4 回呼出）→ `node->value` へ書込 |
| +0x14 | 0x4A83B0 | Begin: `[ecx+0x108]=ecx; ret 4`（引数は無視、開ハンドル立て） |
| +0x18 | 0x4A7CB0 | End: `[ecx+0x108]=0` |
| +0x1C | 0x4A8560 | SaveAll（リスト走査して +0x0C を各ノードへ、結果を AND） |
| +0x20 | 0x4A85A0 | LoadAll（リスト走査して +0x10 を各ノードへ） |

---

## §2 全ノードの機械列挙（タスク (b)）

### 2.1 手法

1. `.text` 全域の `E8 rel32` で宛先 `0x4040E0` のものを探し **637 件**。
2. 各サイトから逆方向 128 B を capstone でデコードし、**最後の `push imm32`（0xB02000〜0xB32C00 の .data 範囲）= ノード VA**、直前の `mov ecx, imm32` = コンテナを抽出。
3. 抽出結果を `rd(node, 8)` で読み、`nameptr` → `cstr()` で ASCII 文字列化。
4. 補助経路として `.data`（VA 0xB02000–0xB32C00、file_off = VA−0x401400、計 199,680 B）を 4 B 刻みで形状スキャン（§4）。

### 2.2 件数

| 区分 | 件数 |
|---|---|
| `E8 → 0x4040E0` 呼出しサイト | **637** |
| ├ container 0xB07BF0 (RegSettingCollection) | 574 |
| ├ container 0xB11C40 (Atmosphere) | 62 |
| └ container 0xB07D28 (DefaultCombatStyle) | 1 |
| 静的ノード `push imm32` サイト | **635** |
| 動的ノード（`push reg`、.data 外） | **2**（`0x404893`←FUN_00404850、`0x4440A3`←FUN_00444060） |
| **静的ノード VA のユニーク数（重複 0）** | **635** = 572 + 62 + 1 |
| 読出成功（value + ASCII name） | **635 / 635** |
| `:` 無し（セクション MAIN 扱い） | 4（`sBasePath` / `iLastHDRSetting` / `iDetectionHighNumPicks` / `fLowPerfCombatantVoiceDistance`） |
| セクション数 | 40（`MAIN` は 1 明示 + 4 隠含 = 5） |

### 2.3 自己検証（回帰テスト）

- **`0xB02E24` は登録集合に含まれる**: `value=10 (0x00000000A)` / `name="iDebugTextLeftRightOffset:Display"` / 登録 site `0x009D9532`（`FUN_009d9500`）→ **PASS**（v80 §2 の記述と完全一致）。
- 登録関数群は **E8 呼出し 0 件**（＝呼び出し元は .rdata の関数ポインタ表のみ）で、その表は `.rdata 0xA283FC..0xA2EFEC` に **635 ポインタ**（各 1 回、欠落 0 / 重複 0）。先頭 `0xA283FC` → `FUN_009dac50` → `sBasePath(0xB0472C)`、末尾 `0xA2EFEC` → `FUN_00a10e50` → `iDistantLODGroupWidth:DistantLOD(0xB2C35C)`。CRT 静的初期化の実行順 = 表の昇順、prepend なので **実行時 head = 0xB2C35C**、**末尾 = 0xB0472C**。
- `.data` 形状スキャン（loose: 先頭文字 ∈ {b,c,f,h,i,r,s,u} + 印刷可能 ASCII + 長さ 3..160）が **登録 635 件を 635/635 で再発見**（欠落 0）→ スキャン条件が登録集合を過不足なく包含。

---

## §3 既定値テーブル（タスク (c)）→ `tmp/re/v91_ini_defaults.csv`

- 行数 **635**（= 登録静的ノード数。孤立候補は §4 のとおり 0 のため行に含めない）。
- 列（指定どおり）: `value, key, section, va, note`
  - `value` = **既定値を型に合わせて人間可読に整形**: float は `%.9g`（例 `fVersion` → `1.79999995`）、int は符号付き、bool は 0/1、unsigned は符号なし、string は**既定文字列そのもの**（`sArchiveList` は CSV で自動引用）、color(r) は `0xFFFBE9FF`。
  - `key` / `section` = `name` を最初の `:` で分割（無ければ `section=MAIN`）。
  - `va` = ノードアドレス（`0x` + 8 桁）。
  - `note` = `raw=0x…`（生ビット、再現可能）+ `type=…(code)` + `container=0x…:クラス名` + `reg=登録関数@登録site` + 既知キー注記 `KNOWN:<出典ファイル名>`。
- 並び順: `section` → `key` → `va`（セクション昇順）。
- **特殊ケース（値の形として正しいが機械処理注意）**:
  - 既定が空文字列の string ノード **15 件**（`sTestFile1`〜`sTestFile10`、`sStartingCell`/`sStartingCellX`/`sStartingCellY`/`sStartingWorld`、`sBetaCommentFileName`）は `value` 列が空。既定が本当に `""` であることを示すため `note` に `default-str-ptr=0x…` を残した。
  - `sGeneralMasterMismatchWarning` の既定文字列は末尾に **CRLF** を含むため、RFC4180 の引用フィールドとして 1 レコードが 2 物理行に跨る。CSV の物理行数は **637 = ヘッダ 1 + 636**（`csv.DictReader` の実データ行数は 635）。
  - カンマを含む value は 49 件（`sArchiveList` 等）ですべて自動引用されている。
- **型別件数**: float 238 / bool 173 / int 103 / string 99 / unsigned 21 / color(r) 1 = **635**。
- **セクション別件数（上位）**: Display 89, General 81, LOD 48, Audio 41, DEFAULT 28, Interface 26, Controls 24, HAVOK 23, Water 23, GamePlay 22, HIT 20, BlurShaderHDR 17, QUADHIT 14, SpeedTree 13, BackgroundLoad 12, BlurShader 12, bLightAttenuation 12, Combat 12, Grass 11, Landscape 10, LoadingBar 9, Archive 8, TestAllCells 8, BlurShaderHDRInterior 8, Menu 8, Weather 7, Pathfinding 7, MAIN 5, MESSAGES 5, Fonts 5, CameraPath 4, CopyProtectionStrings 4, Voice 4, Debug 3, DistantLOD 3, GethitShader 3, OPENMP 2, GeneralWarnings 2, Havok 1, PathFinding 1。
  - **大文字小文字の揺れ 2 組**: `HAVOK`(23) vs `Havok`(1 = `iHavokSkipFrameCountTEST`)、`Pathfinding`(7) vs `PathFinding`(1 = `bUseBackgroundPathing`)。比較は `_stricmp` かつ Windows の INI 読出も大文字小文字を区別しないため実害はないが、移植テーブルでは統合可。
- **既知キー注記（タスク指定の 3 資料からの照合）**: `docs/RE_EXE_REPORT.md` / `tmp/re/v80_partB_globals.md` / `tmp/re/v81_shader_ini_residual.md` を正規表現 `\b[fbisur]…:Section\b` で走査 → 33 件中 **30 件が本テーブルに一致し `KNOWN:` を付与**（例: `fGlobalTimeMultiplier:General`、`bIgnoreResolutionCheck:Display`、`bDoHighDynamicRange:BlurShaderHDR`、`fQuestScriptDelayTime:MAIN`、`bSaveOnRest:GamePlay`、`fDSoundRolloffFactor:Audio` 等）。残り 3 件は登録ノードに存在しない（§7）。
- 代表値の抜粋（検証用）:

  | key:section | value | 由来 |
  |---|---|---|
  | iDebugTextLeftRightOffset:Display | 10 | 0xB02E24（v80 中核例） |
  | iDebugText:Display | 2 | 0xB02E0C |
  | bShowMenuTextureUse:Display | 1 | 0xB02E14 |
  | fVersion:Controls | 1.79999995 | 0xB02C4C（既存成果物の engine 値と一致） |
  | uGridsToLoad:General | 5 | 0xB06A2C |
  | fGlobalTimeMultiplier:General | 1 | 0xB06704 |
  | sIntroSequence:General | `bethesda softworks HD720p.bik,…Oblivion Legal.bik` | 0xB03084 |
  | sArchiveList:Archive | `Oblivion - Meshes.bsa, …Oblivion - Misc.bsa` | 0xB04458 |
  | sBasePath | `.\`（.data 内文字列 0xB04628） | 0xB0472C、`:` 無し→MAIN |
  | rDebugTextColor:Menu | 0xFFFBE9FF | 0xB12DAC（type 7 = 色） |
  | iDistantLODGroupWidth:DistantLOD | 8 | 0xB2C35C（= 初期化表の最終登録 = 実行時 head 候補） |

---

## §4 無効・孤立ノード（タスク (d)）

### 4.1 「head から到達不能」の正しい解釈

ファイルイメージでは 4 コンテノすべて **head=0 / chain=0**（vtable 以外全 0）。したがって**静的にはどのノードも head から辿れない**が、これは「リストが未構築」であって孤立ではない。構築は CRT 静的初期化（§2.3 の .rdata ポインタ表 635 件）で行われ、実行時には 635 件すべてが 1 本の連結リストに属する。

### 4.2 形状スキャンによる孤立候補の探索と棄却

**(i) 厳格条件**（先頭文字 ∈ {b,c,f,h,i,r,s,u,**S**} + 印刷可能 ASCII + `:` を含む + キー/セクションが INI 名として妥当な文字集合・長さ）→ `.data` に **659 件**

- 登録ノードのうち `:` を持つ **631 件が一致**（`:` 無しの登録 4 件は条件上対象外 → 631 + 4 = 635 で整合）。
- **残り 28 件が孤立候補** → 全件棄却:

  | 判定 | 件数 | 根拠 |
  |---|---|---|
  | 隣接候補との `value == nameptr` 相互一致（フラットなポインタ配列） | 27 | `Script Error: …`（0xB0A0C4 起点）と `Spells: Any` 等。実体は `char*` の平配列で、8 B 読みすると「前の要素の文字列ポインタ」が value に見える |
  | 配列先頭（value==0 かつ隣接と相互一致） | 1 | `0xB0A0C4 'Script Error: NOERROR'`（value=0 = 配列先頭の 0） |

  → **真の孤立 INI ノード: 0 件**。

**(ii) 緩和条件**（`:` 不要、先頭文字 ∈ {b,c,f,h,i,r,s,u}）→ **1,442 件**

- 登録 **635/635 を再発見**（欠落 0）。
- 余剰 807 件のうち **534 件を構造で棄却**（value==0 の配列先頭 128 / 隣接との value↔nameptr 相互一致 406）。
- 残る 273 件は **いずれも `:` を含まない**（= 万一ノードでも MAIN 直下になる）うえ、**INI キー慣例（型文字の直後が大文字/数字）に合致するのは 1 件のみ** = `0xB30200 'cArraySize'`。これはスクリプトコンパイラの記述子テーブル（前後は `'type'` `'offset'`、value は型コード `0x000D0008` 等）で **INI ノードではない**。
  - 残りの例: `saveini` `resurrect` `svs`（コンソールコマンド名）、`returns the player's fame`（説明文）、`int` `ref` `china` など = 他テーブルの `{u32, char*}`。

結論: **構造的にノードと判別できる孤立 INI ノードは 0 件**。ただし `:` 無しノードは name 形状だけでは原理的に区別できないため（登録ノードにも 4 件存在する）、登録経路（`E8 → 0x4040E0`）を正とする本タスクの判定を優先した。

---

## §5 呼出し側（タスク (e)）

### 5.1 ノード読手（lookup 関数）= `FUN_0050EC30`

```c
bool FUN_0050EC30(RegSettingCollection* this /*ecx*/, const char* name /*[esp+8]*/, Node** out /*[esp+0xC]*/)
{
    if (!name) return false;
    for (Rec* r = (Rec*)&this[0x10C]; r; r = r->next)          // lea esi,[ecx+0x10c]
        if (r->node && (_stricmp(r->node->name, name) == 0))   // 0x98262D = _stricmp
            { *out = r->node; return true; }
    return false;                                               // xor al,al / ret 8
}
```

- **登録時（AddSetting 0x4040E0）と同じ走査パターン・同じ `_stricmp`** を使い、名前（`"Key"` または `"Key:Section"`）からノードを返す。大文字小文字無視。
- E8 呼出しは **2 件のみ**:
  - `0x5142B4` = **`GetINISetting`**（0x514230、コンソール opcode **0x011E** / short `GetINI`、+0x18 実装 ptr は .data `0xB0B8E8`）
  - `0x51444F` = **`SetINISetting`**（0x5143C0、opcode **0x011D** / `SetINI`、.data `0xB0B8C0`）
  - 両者とも E8 呼出し 0 件＝**オペコード表の間接ディスパッチのみ**で呼ばれる。
- 命中後の型別処理（`0x4A78D0` の type で switch、表 0x5143A0 / 0x5144E4）:
  - GetINISetting: int/bool → `"INISetting %s >> %i"`（0xA4D7FC）、float → `"% .2f"`（0xA4D7E4）、string → `"'%s'"`（0xA4D7CC）、未検出 → `"INISetting %s >> NOT FOUND"`（0xA4D790）、型不明 → `"... >> UNKNOWN TYPE"`（0xA4D7AC）。
  - SetINISetting: `0x983479`(数値→u32) → `mov [esi],eax` / `0x985fb2`(atof) → `fstp [esi]` / `0x4A7980`(文字列格納) と、**ノードの value を直接書換える**。

### 5.2 INI 読み込み時にどう更新されるか（1 段階）

```
起動部（0x40DF80 系）
  ├ パス構築: bss [0xB3F280](dir, 実行時生成) + [0xB02C84]="Oblivion.ini" → this+4 (0xB07BF4) へコピー
  │   （FUN_0040D4D0 の 0x40E710 コピーループ / FUN_00513BB0 の 0x513C20 でも同一）
  ├ ファイル無し時: CopyFileA("Oblivion_default.ini", path) @0x40E1C3（v51 記載）
  └ FUN_0040D4D0（E8 呼出し 5 件: 0x40DF54 系 / FUN_004455e0 / FUN_0044e720 / FUN_0050d3e0 / FUN_005bdcd0）
        ├ mov eax,[0xb07bf0]; mov edx,[eax+0x14]; push ebp; mov ecx,0xb07bf0; call eax   // Begin
        ├ vtable +0x20  = 0x4A85A0  LoadAll
        │     for (Rec* r = &head; r; r = r->next)  this->ReadOne(r->node);
        │        └ vtable +0x10 = 0x4A8800  ReadOne(node)
        │              0x4A8430(node,&section) / 0x4A8480(node,&key) / 0x4A78D0(node->name)=type
        │              → GetPrivateProfile*（[0xA280B4] を 4 回呼出、ファイル名 = this+4）
        │              → 型に応じて node->value へ書込み（int→0x98629e系, float→, string→0x4A7980）
        └ vtable +0x18  = 0x4A7CB0  End  ([this+0x108]=0)
```

- **要するに「node->value は初期化時に .data の既定値で置かれ、LoadAll の 1 ステップで INI ファイル値に上書きされる」**。値の読手は以降すべてこの `node` の絶対アドレスを直接参照する（例: `fun_00407ae0` が `0x500 - DAT_00b02e4` / `fild dword[0xb02e24]` を 79+113 サイト、v80 §2）。
- 書出し側（対になる経路）:
  - `SaveIniFiles` = **0x513B60**（opcode **0x0165**、.data `0xB0C400`）: パスコピー → Begin(**1**) → `SaveAll`(0x4A8560) → End。
  - `RefreshINI` = **0x513BB0**（opcode **0x013E**、.data `0xB0BDE8`）: パスコピー → Begin(**0**) → `LoadAll` → End → `0x579B60` で `"The in-game settings have been refreshed from the Oblivion.ini file."`（0xA4D690）。
  - 起動時自動書出しラッパ `0x40C180` の E8 呼出しは **1 件のみ `0x40F59E`**（v51/v53 の記述と一致）。
  - 個別設定の書き戻し: `FUN_005A3810` 系（トグル系コンソール）が `node->value` を更新した直後に `vtable+0x0C`（WriteOne）へ `push 0xb13208` 等でノードを渡して 1 件だけ INI へ保存。
- **名前による insert/lookup 以外の入口**: `FUN_00404850`（float ノード工場、caller `FUN_0049d2a0` ← `FUN_0049dd00`/`FUN_0049e280`/`FUN_0049e750`）と `FUN_00444060`（int ノード工場、caller `FUN_0049dd00` ← `FUN_0049e610`）は **ヒープにノードを作って登録する**ため、実行時のノード数は 635 より多い可能性がある（本タスクの .data 列挙対象外）。

---

## §6 件数検証（整合の明示）

| # | 検証式 | 結果 |
|---|---|---|
| V1 | `E8 → 0x4040E0` = 574 + 62 + 1 | **637 = 637** OK |
| V2 | 静的 push 635 + 動的 2 = 637、ユニーク VA 635、重複 0 | **OK** |
| V3 | ノード読出成功 = 635 / 635（value + ASCII name、欠損 0） | **OK** |
| V4 | コンテナ別静的ノード 572 + 62 + 1 = 635 = 登録ノード総数 | **OK** |
| V5 | CSV 行数 635 = セクション別合計 635 = 型別合計 635 | **OK** |
| V6 | 自己検証 `0xB02E24` → value=10, `iDebugTextLeftRightOffset:Display`, reg=`FUN_009d9500@0x009D9532` | **PASS** |
| V7 | 厳格スキャン 659 = 登録(有 `:`) 631 + 登録(`:` 無) 4 + 候補 28、候補 28 → 棄却 28 / 真の孤立 0 | **631+4+28 = 659** OK |
| V8 | 緩和スキャン 1,442 = 登録 635 + 余剰 807（うち構造棄却 534 → 残 273、`:` 有 0、INI キー形状 1 件は別テーブル） | **635+807 = 1,442** OK |
| V9 | .rdata 登録関数ポインタ 635 = 登録関数 635（欠落 0 / 重複 0、E8 呼出しは各 0 件） | **OK** |
| V10 | probe_harness selftest | **4/4 PASS**（作業前・作成後） |

---

## §7 既存成果物との突合（参考）

| 相手 | 相手の値 | 本タスク | 判定 |
|---|---|---|---|
| `tmp/re/oblivion_ini_defaults_manifest.txt` | engine nodes **572** | 0xB07BF0 の静的登録 572 | **一致** |
| 同 manifest | `[Menu] rDebugTextColor engine-raw=-267777` | 0xFFFBE9FF の符号付き = -267777 | **一致** |
| 同 manifest | section-less 3 件（iLastHDRSetting / iDetectionHighNumPicks / fLowPerfCombatantVoiceDistance） | `:` 無し 4 件（+ DefaultCombatStyle の `sBasePath`） | **一致**（+1 は別コンテナ） |
| 同 manifest | `[Controls] fVersion engine=1.79999995 vs file=1.7000` | 0xB02C4C = 1.79999995 | **一致**（冲突はファイル側） |
| `tmp/re/copilot_ini_crosscheck.md` | unique **635** / 637 サイト | 635 / 637 | **一致** |
| `tmp/re/s60_io.md` | INI キー一覧（615 キー / 6 セクション） | 635 ノード / 40 セクション | 抽出方法が異なる（s60 は文字列走査、本タスクは登録サイト走査）→ 未照合 |
| 3 資料の既知キー 33 件 | 30 件が本表に一致 | `KNOWN:` 付与 30 | 未一致 3 件 = `fAlpha:Water`, `uSurfaceTextureSize:Water`, `fDifficultyMinValue:GamePlay` |

未一致 3 件の実体確認: `fAlpha:Water`(0xA3F43C) / `uSurfaceTextureSize:Water`(0xA3F4C4) は **.rdata に文字列が存在するが .data からポインタ参照が 0 件**（＝ノード化されていない）、`fDifficultyMinValue:GamePlay` は連続文字列として不在（`fDifficultyMinValue` 接頭のみ存在）。いずれも**孤立ノードではない**。

---

## §8 残点

1. **path バッファの厳密な長さ**: `+0x004..+0x107`（0x104 B）と V51 の推定で整合するが、コピーは NUL まで境界検査なし（`mov [0xB07BF4+eax], cl`）で、`+0x108` との衝突可否は未検証。
2. **`type 1(c)/2(h)/8(a)` の意味**: 登録ノードに出現せず（0 件）、switch の実体（`0x4A78F9`=1, `0x4A7905`=2, `0x4A7923`=8）も未解読。`r`(7) は `rDebugTextColor` の実測から「色」で確定。
3. **`0xB07AD8`（vtable 0xA40548 = INI backend と同一、登録 0 件）の用途**: 未使用か、実行時だけ使われるか未判定。RTTI も不在。
4. **動的ノード（`FUN_00404850` / `FUN_00444060`）の全数**: 各 1 呼出し元経由でヒープ生成されるが、実行回数とノード内容（.data 外のため既定値取得不可）は未調査。
5. **`LoadOne` の `lpDefault` 引数**: GetPrivateProfile へ渡す既定値が「現行 `node->value`」か「専用文字列」かは未特定（INI にキーが無い場合の挙動に影響）。
6. **`.data` スキャンの原理的限界**: `:` を持たない孤立ノードは登録経路なしでは識別できない（§4.2(ii) のとおり該当候補は 0 件だが、証明は「INI キー形状に合致しない」まで）。
7. **s60 の 615 キーとの全数照合**: 抽出定義が異なるため未実施（件数差 20 の内訳未特定）。

---

検証: `python "%TEMP%\opencode\probe_harness.py" selftest` → 4/4 PASS（本ノート作成前後とも実行）。バイナリ・docs/・既存 tmp/re ファイル・TODO・session-memory.json・plan.md は無変更。新規作成は本ノートと `v91_ini_defaults.csv` の 2 ファイルのみ。
