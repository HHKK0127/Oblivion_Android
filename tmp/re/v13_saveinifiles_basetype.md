# V13 — SaveIniFiles (opid 0x0165) 間接呼出の基底型特定

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416, Steam)
- 作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
- マッピング (ground truth): ImageBase=0x400000 / .text RVA=0x1000 Raw=0x400 / .rdata VA=0xA28000 Raw=0x627200 / .data VA=0xB02000 Raw=0x700C00
- 解析: PE 構造体自前解析 + capstone 5.0.7。既存ドキュメントは編集していない（本ファイル新規のみ）。

---

## 1. セルフテスト 4 件（全 PASS）

| # | 検査 | 期待値 | 実測値 | 結果 |
|---|------|--------|--------|------|
| ① | `bytes@0x504050` | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| ② | `bytes@0x4FB580` | `578b7908` | `578b7908` | PASS |
| ③ | `string@0xA7035C` | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| ④ | opid `0x0015` impl | `0x504560` | idx=21, entry=0xB0B290, impl=**0x504560** | PASS |

---

## 2. opid 0x0165 の impl 確定（校正結果含む）

- オペコードテーブル: 基底 `0xB0AF48`、stride `0x28`、532 エントリ走査。
- 校正: opid `0x0015` → impl `0x504560`（GT 一致、上表④）。
- **opid `0x0165` → idx=132, entry=`0xB0C3E8`, impl=`0x513B60`**、name=`'SaveIniFiles'` (0xA502A0)、shortname=`'saveini'` (0xA50298)、help=`'Writes all the .ini files.'` (0xA5027C)、+0x1C compiler helper=`0x4FDE30`。
- 台帳照合（一致）:
  - `tmp\re\v10_impl_decompile_join.csv` L134: opid 0x0165 SaveIniFiles impl 0x00513B60、prologue bytes `b9f47bb0008a01880183c10184c075f5`（本解析の逆アセンブルと一致）、Ghidra FUN 欠落。
  - `tmp\re\v18_indirect_calls.csv` L132-134: `0x513B7F call edx (+0x14)` / `0x513B92 call edx (+0x1C)` / `0x513BA1 call edx (+0x18)`（本解析と完全一致）。
- 周辺の同系統コマンド（テーブルから特定）: opid 0x013E `RefreshINI` impl=0x513BB0、opid 0x011D `SetINISetting` impl=0x5143C0、opid 0x011E `GetINISetting` impl=0x514230、opid 0x011C `SetGameSetting` impl=0x514100。

---

## 3. impl `0x513B60` 逆アセンブル全文（int3 終端・23 命令）

```asm
0x00513B60: mov      ecx, 0xb07bf4          ; INI パス文字列バッファ先頭 (object+0x4)
0x00513B65: mov      al, byte ptr [ecx]     ; +-- NUL までのバイト走査
0x00513B67: mov      byte ptr [ecx], al     ; |   (読戻し付き = strlen 相当パス)
0x00513B69: add      ecx, 1                 ; |
0x00513B6C: test     al, al                 ; |
0x00513B6E: jne      0x513b65               ; +--
0x00513B70: mov      eax, dword ptr [0xb07bf0]   ; eax = グローバルオブジェクトの vtable ptr
0x00513B75: mov      edx, dword ptr [eax + 0x14] ; edx = vt+0x14 (Begin)
0x00513B78: push     1                      ; arg1 = 1 (write モード)
0x00513B7A: mov      ecx, 0xb07bf0          ; this = &g_INISettings (グローバル実体)
0x00513B7F: call     edx                    ; *** 間接呼出 1 *** Begin(1) -> 0x4A83B0
0x00513B81: test     al, al
0x00513B83: je       0x513ba3               ; Begin 失敗なら SaveAll/End を飛ばす
0x00513B85: mov      eax, dword ptr [0xb07bf0]
0x00513B8A: mov      edx, dword ptr [eax + 0x1c] ; edx = vt+0x1C (SaveAll)
0x00513B8D: mov      ecx, 0xb07bf0
0x00513B92: call     edx                    ; *** 間接呼出 2 *** SaveAll -> 0x4A8560
0x00513B94: mov      eax, dword ptr [0xb07bf0]
0x00513B99: mov      edx, dword ptr [eax + 0x18] ; edx = vt+0x18 (End)
0x00513B9C: mov      ecx, 0xb07bf0
0x00513BA1: call     edx                    ; *** 間接呼出 3 *** End -> 0x4A7CB0
0x00513BA3: mov      al, 1                  ; 常に true を返す
0x00513BA5: ret
0x00513BA6: cc ... (int3 パディング)
```

## 4. 3 本の間接呼出の詳細

| # | 呼出 VA | 命令 | ベース/オフセット | 解決先 (vtable 0xA404F8) | this | 引数 |
|---|---------|------|------------------|--------------------------|------|------|
| 1 | 0x513B7F | `call edx` | eax=`[0xB07BF0]` + **+0x14** | `0x4A83B0` Begin(writeFlag) | `ecx=0xB07BF0` | push 1 |
| 2 | 0x513B92 | `call edx` | eax=`[0xB07BF0]` + **+0x1C** | `0x4A8560` SaveAll() | `ecx=0xB07BF0` | なし |
| 3 | 0x513BA1 | `call edx` | eax=`[0xB07BF0]` + **+0x18** | `0x4A7CB0` End() | `ecx=0xB07BF0` | なし |

- **ポインタの由来**: `mov eax,[0xB07BF0]` でロードされるのは「ヒープ経由の this」ではなく、**`.data` に静的に定義されたグローバル実体オブジェクトの先頭 dword = vtable ポインタ**。`mov ecx,0xB07BF0` が this として渡される点（ロード値ではなくアドレス）が決定的。
- **vtable アドレス = `0xA404F8`（.rdata）**。初期値は PE の .data イメージ（fileoff 0x7067F0 = `f8 04 a4 00`）に静的初期化されており、**実行時書き込みはゼロ**（0xB07BF0 を絶対アドレスで参照する 634 箇所を全分類 → 読み取り 618 / 書き込み 0 / その他 16 = mid-instruction 誤検出）。

## 5. オブジェクト初期化の逆追跡

- **vtable**: 静的初期化のみ（上記）。コンストラクタによる実行時代入は存在しない。
- **実体 2 インスタンス**（同一抽象インターフェースの 2 バックエンド）:
  - `0xB07BF0` — vtable `0xA404F8` = **INI ファイルバックエンド**（SaveIniFiles の対象）
  - `0xB07D28` — vtable `0xA40590` = **レジストリバックエンド**（+0x4 に 'Software\Bethesda Softworks\Oblivion' (0xA30FEC) をコピー: 0x40E6C0、+0x14/+0x18 が RegCreateKeyExA/RegOpenKeyExA/RegCloseKey、+0x0C が RegSetValueExA、HKEY_CURRENT_USER=0x80000002、ハンドルは +0x108 に保存 → 0x4A8C90/0x4A8CF0）
- **+0x4（パス/ターゲット文字列）の書込み箇所**（NUL 終端コピー、いずれも `0xB07BF4` へ）:
  - `0x404470`（Controls 保存系）、`0x4045B0`（Controls 読込系）、`0x40E710`（起動時ロード、fn 0x40DF80 内）、`0x513C20`（RefreshINI impl 0x513BB0 内）
  - 既定値の供給元: `.data` グローバル **`[0xB02C84] = ptr 'Oblivion.ini' (0xA2FBA4)**（fileoff 0x701884 で検証。同ブロックに 0xB02C80='Oblivion'、0xB02C88='.\\Plugins.txt'）。RefreshINI は `mov eax,[0xb02c84]` (0x513BC4) でこれを読む。
- **共通の Open ヘルパ**: `0x4040A0(this, path, flag)` = パスを this+4 へコピー → `vt+0x14(flag)` を呼び出し（path=NULL なら this+4 を 0 化）。ラッパ `0x40C180` = Open + `vt+0x1C`(SaveAll) + `vt+0x18`(End)（0x40F58E から `push 0xb07bf4; mov ecx,0xb07bf0; call 0x40C180` で使用）。
- **設定エントリ表 +0x10C**: 先頭ポインタ。挿入は `0x446CB0`（ノード 8B を `0x401F00`(operator new) で確保、+0x00=entry、+0x04=next）。登録系関数 `0x444080` がノードを組み立てて `vt+0x00`(0x4040E0) に渡す。
- **非仮想メンバ**: `0x50EC30(this=0xB07BF0, name, out**)` = +0x10C リストを名前 strcmp（`0x98262D`）で走査し entry を返す FindSetting。GetINISetting/SetINISetting (0x5142A7 / 0x514442) から this=0xB07BF0 で呼ばれる。

## 6. +0x00〜+0x24 スロット表（vtable 0xA404F8、thiscall）

| スロット | 実装 VA | 役割（実証済み） |
|----------|---------|------------------|
| +0x00 | `0x4040E0` | AddSetting（+0x10C リスト走査→同名なら置換 0x4A7A60、なければ 0x446CB0 で挿入） |
| +0x04 | `0x403C20` | 破棄系シンコール（`add ecx,0x10C; jmp 0x65C620` = エントリリスト破壊） |
| +0x08 | `0x4A8B10` | Dump/計数（コンソール出力: 'INISettings' 0xA40580, 'Total INISettings' 0xA4056C） |
| +0x0C | `0x4A85F0` | **WriteOne**: 型別 sprintf（`%d`/`%.4f`/`%f`/`%u,%u,%u`/`%u,%u,%u,%u`/`%u`）→ `call [0xA280B0]` = **WritePrivateProfileStringA**(section, key, value, **file=this+4**) |
| +0x10 | `0x4A8800` | **ReadOne**: `call [0xA280B4]`=**GetPrivateProfileStringA**(..., buf, 0x100, **file=this+4**) / `call [0xA280E4]`=**GetPrivateProfileIntA**(..., **file=this+4**) |
| +0x14 | `0x4A83B0` | **Begin(flag)**: `mov [ecx+0x108], ecx; mov al,1; ret 4`（セッション開始マーク = this 自身を格納） |
| +0x18 | `0x4A7CB0` | **End()**: `mov [ecx+0x108], 0; mov al,1; ret`（セッション終了） |
| +0x1C | `0x4A8560` | **SaveAll**: `[this+0x108]!=0` のとき +0x10C リストを走査し各 entry へ **vt+0x0C** を呼び、結果を AND |
| +0x20 | `0x4A85A0` | **LoadAll**: 同リスト走査し各 entry へ **vt+0x10** を呼び、結果を AND |
| +0x24 | (0x00007525) | **スロットではない** — 隣接 .rdata 文字列 "`%u\0`" (0xA4051C) の先頭。vtable は 9 エントリ (+0x00..+0x20) |

オブジェクト実体（0xB07BF0）のレイアウト: `+0x00` vtable / `+0x04` INI ファイル名バッファ / `+0x108` セッション状態（INI では this か 0、レジストリでは HKEY）/ `+0x10C` 設定エントリ双方向リスト先頭。

## 7. `.rdata` 文字列実読（本調査で実読みした主要値）

| VA | 値 | 参照元 |
|----|----|--------|
| 0xA502A0 | `SaveIniFiles` | テーブル entry 0xB0C3E8 +0x00 |
| 0xA50298 | `saveini` | entry +0x04（コンソール短縮名） |
| 0xA5027C | `Writes all the .ini files.` | entry +0x0C（help） |
| 0xA2FBA4 | `Oblivion.ini` | .data グローバル [0xB02C84]（RefreshINI 等が読み、+4 へコピー） |
| 0xA31254 | `Oblivion_default.ini` | 0x40E1C3（fn 0x40DF80 内、4x2B ブロックコピー） |
| 0xA4D690 | `The in-game settings have been refreshed from the Oblivion.ini file.` | RefreshINI 0x513C64 |
| 0xA40580 / 0xA4056C | `INISettings` / `Total INISettings` | vt+0x08 Dump 0x4A8B10 |
| 0xA4D7FC / 0xA4D7E4 / 0xA4D7CC | `INISetting %s >> %i` / `INISetting %s >> %.2f` / `INISetting %s >> '%s'` | GetINISetting 0x514230 |
| 0xA4051C/520/52C/538/540 | `%u` / `%u,%u,%u,%u` / `%u,%u,%u` / `%.4f` / `%f` | vt+0x0C 書式 |
| 0xA30FEC | `Software\Bethesda Softworks\Oblivion` | レジストリ側インスタンス 0xB07D28 の +4 初期値 |

Win32 IAT 実解決（IMAGE_IMPORT_BY_NAME 照合）: `[0xA280B0]=WritePrivateProfileStringA`、`[0xA280B4]=GetPrivateProfileStringA`、`[0xA280E4]=GetPrivateProfileIntA`、`[0xA28000]=RegSetValueExA`、`[0xA28004]=RegOpenKeyExA`、`[0xA28008]=RegCreateKeyExA`、`[0xA2800C]=RegCloseKey`、`[0xA28010]=RegQueryValueExA`。

## 8. 判定: (a) INI/設定管理オブジェクト

**(a) INI/設定管理オブジェクト**（厳密には「設定ストレージ抽象クラス」。INI ファイル・レジストリの 2 バックエンド実装を持つ）。根拠:

1. **+0x14/+0x18/+0x1C は構造体インライン関数ポインタではなく vtable スロット**（+0x00 が vtable ポインタ、this は &g=0xB07BF0）。9 スロットすべてが thiscall メソッドで、+0x0C/+0x10 が実際 Win32 プロファイル API（WritePrivateProfileStringA / GetPrivateProfileStringA / GetPrivateProfileIntA）を this+4 のファイル名に対して呼ぶ。
2. SaveIniFiles の呼び出し順 `Begin(1) → SaveAll(全設定を 1 つの .ini へ書出し) → End` は「設定管理オブジェクトの一括保存」そのもの。RefreshINI (0x513BB0) は `Begin(0) → LoadAll → End` + '…refreshed from the Oblivion.ini file.' で対を成す。
3. (b) vtable 先頭クラステーブルではない: スロットはすべて関数 VA であり、文字列 ptr は vtable から隣接領域にのみ存在（+0x24 は "%u" 文字列）。(c) コールバック群でもない: セッション制御 (Begin/End) とストレージ read/write の一貫インターフェースであり、イベント/フック構造ではない。

## 9. 呼出元（1 次元）

- **直接 E8 はゼロ**: .text 全体（6.4MB）を E8 リラティブ走査 → `0x513B60` 実装への直接呼出は 0 件（同様に RefreshINI / SetINISetting / GetINISetting / SetDebugText / opid 0x0015 impl も 0 件 = 全コマンドがテーブル経由であることを確認）。
- **impl ポインタの所在はファイル内に 1 箇所のみ**: テーブル entry `0xB0C3E8` の **+0x18 (0xB0C400) = 0x513B60**。オペコードテーブル 532 エントリの +0x18 列が唯一の供給経路。
- **テーブル基底 0xB0AF48 の参照も 1 箇所のみ**: `0x4FF904 lea eax,[ebx+0xb0af48]`（fn ~0x4FF4C0 = **スクリプトコンパイラ**: 'Mismatched begin/end block.' 0xA4B54C / 'Invalid block type for quest script.' 0xA4B524 を参照し、entry の +0x10 flags / +0x12 u16 / +0x14 / +0x1C compiler helper を読んでコンパイル）。
- 結論: SaveIniFiles は **オペコード/コンソールコマンド実行系がテーブル entry +0x18 を間接呼出しする経由のみ**で実行される（= K2 分類の「間接呼出が残った唯一の実装」opid 0x0165 の位置づけと整合）。実行時の entry への到達はインデックス解決後であり、ファイル画像上は vtable 式の間接呼出としてのみ存在する。

## 10. K2 / 既往調査との整合

- V9/V18 の K2 = opid 0x0165 のみに残る `call edx` × 3 を、本調査で個別実装まで同定（Begin / SaveAll / End）。V18 CSV の 3 行と VA・disp が完全一致。
- 別解 (b)(c)(d) を棄却する直接証拠: +0x14=0x4A83B0 / +0x18=0x4A7CB0 / +0x1C=0x4A8560 の 3 実装は、いずれも `[this+0x108]`（セッション状態/HKEY）と `+0x10C`（設定エントリリスト）のみを操作するストレージ制御メソッドであり、vtable 先頭クラステーブルでもコールバック群でもない。
