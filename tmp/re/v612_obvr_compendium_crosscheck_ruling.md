# v612 — 追加参考先クロスチェック裁定: OBVR / compendium からの照合、RTTI 機械同定（RaceSexMenu vtable）、`g_TileMenuArray` の特定と tile 木走査 probe

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。**md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`・PE timestamp `0x462392c7`(2007-04-16)・no base relocations・ImageBase 0x400000** — OBNG/OBVR/compendium の記載と完全一致（同一ビルド）。
根拠: (a) 参考先 clone（`%TEMP%\opencode\{OBVR,compendium,openoblivion}`・読み取り専用）。(b) 我々の実読 probe **u45_1 / u46_1 / u47_1**。(c) compendium の MIT スクリプト **`rtti_scan.py` / `pe_info.py`** を我々の EXE に実行。
判定: `[確定(命令)]` / `[確定(外部+実読)]`（外部と我々実読の二源一致）/ `[参照(OBVR)]` / `[参照(compendium)]`

---

## 1. 結論

| 問い | 判定 | 答え |
|---|---|---|
| 参考先は同一バイナリか | **[確定(命令)]** | **同一**。md5・PE timestamp `0x462392c7`・no relocations・7,898,624 B が一致。`pe_info.py` で我々の EXE を直接確認。 |
| RaceSexMenu の vtable | **[確定(外部+実読)]** | **`0xA6D38C`**（15 slots・base `Menu`）。compendium `rtti_scan.py` で機械同定。 |
| `g_TileMenuArray` の実体 | **[確定(外部+実読)]** | **`NiTArray<TileMenu*>` @ `0x00B13970`**（vtable `0xA69680`）。data ptr `0x00B13974`・count u16 `0x00B1397A`・**index = menuId − 0x3E9**。我々の `0x589B70` 実読と OBVR `GameAddresses.h` が一致。 |
| tile 木の走査 | **[確定(外部+実読)]** | Tile `+0x08` name・`+0x10` parent・`+0x18` 値リスト・`+0x34` 子リスト。ノード `{next+0x00, data+0x08}`。子判定 `data.parent == tile`。**走査 probe `u47_1.py` を具体形で作成**。 |
| メニュー id | **[確定(外部+実読)]** | 0x3E9 起点。**RaceSex=`0x40C`・Loading=`0x3EF`・Main=`0x414`**。OBVR `MenuType.h`（xOBSE 由来・static_assert 付き）。 |
| `IsMenuMode` | **[確定(外部+実読)]** | `0x578F60`。`mgr = InterfaceManager_GetSingleton(0,1)`；`mgr && [mgr+0x1C]!=0 && [mgr+0x08]!=1`。 |

---

## 2. 参考先の全体像（ライセンス別の扱い）

| プロジェクト | ライセンス | 価値 | 我々の扱い |
|---|---|---|---|
| **Naragorn/OBVR** | GPL-3.0 | `HANDOFF.md`(141KB)・`GameAddresses.h`(全アドレス)・`MenuType.h`・`HudTiles.cpp`（tile 走査実装）・`docs/vr-modding/*` | **読んで書き直す**（コピー禁止） |
| **Naragorn/OBNG** | GPL-3.0 | v611 で処理済 | 同上 |
| Naragorn/OblivionWalkers | GPL-3.0 | プラグイン構成例（エンジン知見は薄い） | 参照 |
| Naragorn/OBDLSS5 | GPL-3.0 | 描画のみ | 範囲外 |
| reaffirmed/openoblivion | 要確認 | Rust+Bevy の engine 再実装。XML 解釈の一次情報になり得る | 参照（メニュー実装は未確認） |
| madpai/OpenOblivion | 要確認 | Android ネイティブ研究・`docs/research/EXECUTABLE_RECON.md` | 参照 |
| **madpai/game-decomp-compendium** | **MIT** | `skills/re-binary-recon/scripts/{rtti_scan,pe_info,string_xrefs,aob_scan,pe}.py`・`skills/bethesda-gamebryo-re/scripts/gmst_defaults.py`・`knowledge/games/oblivion/*`(2本) | **スクリプトはそのまま使用可**（MIT） |

- **compendium の方針**: 「exe 由来のアドレス表・GMST ダンプは公開リポジトリに置かない（方法・件数のみ公開）」。→ **我々の `tmp/re/` は gitignore 済み**で、この方針と整合。
- **OBVR の `GameAddresses.h` はアドレス表を公開している**（GPL）。事実の参照は可、コードは書直し。

---

## 3. RTTI 機械同定（compendium `rtti_scan.py`・MIT・そのまま使用）[確定(外部+実読)]

`python rtti_scan.py <Oblivion.exe> --grep <re> --format tsv` で MSVC RTTI から vtable を機械抽出。

| クラス | vtable | slots | bases |
|---|---|---|---|
| **RaceSexMenu** | **`0xA6D38C`** | 15 | Menu |
| **LoadingMenu** | **`0xA6C40C`** | 15 | Menu |
| Menu（base） | `0xA696DC` | 15 | — |
| ClassMenu | `0xA6B49C` | 15 | Menu |
| MainMenu | `0xA6CB1C` | 15 | Menu |
| Tile | `0xA690E8` | 7 | — |
| TileRect | `0xA69138` | 7 | Tile |
| TileMenu | `0xA6AF14` | 7 | TileRect,Tile |
| TileText / Tile3D / TileImage / TileWindow | `0xA6AEC4` / `0xA6AEEC` / `0xA69110` / `0xA6AF3C` | 7 | Tile |
| `NiTArray<PAVTile@@>` | `0xA69680` | 1 | — |

- **`LoadingMenu` vtable `0xA6C40C` は我々の v611 ctor 実読（`mov [esi],0xa6c40c`）と一致** ＝ RTTI 抽出の正しさを我々の側で独立確認。
- **`NiTArray<PAVTile@@>` vtable `0xA69680` = `g_TileMenuArray` の vtable**（後述）。
- 総数: Menu 系 53 vtables。全体は 1,715 vtables / 1,482 classes（compendium 記載）。
- 出力は exe 由来 → **リポジトリへは置かない**（`%TEMP%` のみ）。

---

## 4. メニュー id 表（OBVR `MenuType.h`・xOBSE 由来）[確定(外部+実読)]

- `kMenuIdFirst = 0x3E9` / `kMenuIdLast = 0x41B`。
- 主要: Message `0x3E9`・Inventory `0x3EA`・Stats `0x3EB`・HudMain `0x3EC`・HudInfo `0x3ED`・**Loading `0x3EF`**・Container `0x3F0`・Dialog `0x3F1`・Generic `0x3F3`・SleepWait `0x3F4`・LevelUp `0x403`・Persuasion `0x40A`・**RaceSex `0x40C`**・Load `0x40E`・**Main `0x414`**・QuickKeys `0x416`・Credits `0x417`。
- OBVR の static_assert（`kMenuIdSleepWait == first+11`・`kMenuIdMain == first+43` 等）が、この build の実測 `0x3F4`(SleepWait) と一致 ＝ enum と本ビルドが一致。

---

## 5. `g_TileMenuArray` の特定 [確定(外部+実読)]

我々の実読（probe u45_1/u46_1）:
```
0x589B70 GetTileMenu(menuType):                 ; eax = menuType
  ecx = eax - 0x3E9 ; cmp ecx, 0x32 ; ja -> return 0
  if (word[0xB1397A] <= menuType-0x3E9) -> return 0
  edx = dword [0xB13974]                        ; 配列データポインタ
  return dword [edx + eax*4 - 0xFA4]            ; 0xFA4 = 0x3E9*4 → [edx + (menuType-0x3E9)*4]
```
- **配列オブジェクト `NiTArray<TileMenu*>` @ `0x00B13970`**（+0x00 vtable=`0xA69680`）。
- **data ptr @ `0x00B13974`**（静的値=0 → runtime で埋まる）／**count u16 @ `0x00B1397A`**（静的値=0）。
- OBVR `GameAddresses.h` の `kTileMenuArray=0x00B13970` / `kTileMenuArrayData=0x00B13974` / `kTileMenuArrayCount=0x00B1397A` と**完全一致**（二源）。
- 呼出元 321 箇所（`0x589B70`）。`0x5AD980`(Tick) は `0x3EF` で引く。

---

## 6. Tile レイアウトと tile 木走査アルゴリズム [確定(外部+実読)]

我々の実読（`0x589930` tile 名前検索: `esi=[ecx+0x34]`→`edi=[esi+8]`→`eax=[edi+8]`→`esi=[esi]`）と OBVR `HudTiles.cpp`/xOBSE `GameTiles.h` が一致:

| Tile offset | 内容 |
|---|---|
| +0x00 | vtable |
| +0x08 | name（BSStringT：**文字列ポインタが先頭**） |
| +0x10 | parent（Tile*） |
| +0x18 | 値リスト node start（NiTListBase の先頭ノード） |
| +0x24 | render node（NiNode*、translation +0x54） |
| +0x34 | 子リスト node start |

- **リストノード**: `+0x00` next・`+0x08` data。
- **値 (Value)**: `+0x04` number(float)・`+0x08` string(BSStringT)・`+0x18` id(u16)・`+0x1A` isNumber(u8)。
- **子判定**: `data.parent(+0x10) == tile`（リストに親以外の項目が混じるため必須）。
- **trait id（xOBSE GameTiles.h）**: visible `0xFA1`・locus `0xFA6`・alpha `0xFA7`・**id `0xFA8`**・depth `0xFAB`・y `0xFAC`・x `0xFAD`・height `0xFCA`・width `0xFCB`・string `0xFDE`・filename `0xFE6`。**我々の v610 の `0xFA8`(id)/`0xFAE`(user0)/`0xFAF`(user1)/`0xFB0`(user2)/`0xFB1`(user3)/`0xFB3`(user5) と整合**。

---

## 7. `IsMenuMode` の実装 [確定(外部+実読)]

OBVR `HANDOFF.md`（実測）:
```
0x578F60: push 1; push 0; call 0x582160   ; InterfaceManager_GetSingleton(0,1)
          if (!mgr) return 0
          push 1; push 0; call 0x582160; if ([mgr+0x1C]==0) return 0
          push 1; push 0; call 0x582160; return ([mgr+0x08] != 1)
```
- `InterfaceManager` ポインタは `[0x00B3A6E0]`（OBVR）。`menuRoot` は `+0x68`、`activeMenu` は `+0x9C`。

---

## 8. tile 木走査 probe（具体形）`u47_1.py`

`%TEMP%\opencode\u47_1.py`。**静的モード**（引数なし）で配列の所在・添字算術を確認し、**`--pid <PID>`** で稼働中の Oblivion.exe を `ReadProcessMemory` で読み、`g_TileMenuArray[menuId]` から tile 木を深さ優先で走査して `name/id/x/y/w/h/visible/alpha/locus` を表にする。

- 静的実行の実測:
  ```
  array object @ 0x00B13970 : vtable=0x00A69680
  data ptr  [0x00B13974] = 0x00000000  (runtime で埋まる)
  count u16 [0x00B1397A] = 0
  index = menuId - 0x3E9 ; RaceSex(0x40C)=idx 35 ; Loading(0x3EF)=idx 6
  ```
- 使用例: `python u47_1.py --pid <Oblivion.exe の PID> --menu-id 0x40C` → RaceSexMenu の tile 木。
- **制約**: 配列は runtime 構築のため、静的ファイルからは木を辿れない（data ptr=0）。**稼働プロセスかメモリダンプが必要**。

---

## 9. compendium のノート（MIT・方法と件数のみ）[参照(compendium)]

- **`oblivion-menus-xml-and-script-command-coverage.md`**: `Oblivion - Misc.bsa` の `menus\` 配下は **104 XML（1.22 MB・570 種タグ）**。XML は宣言的レイアウト言語（`copy/ref/include/add/sub/onlyif/user0..N/clicked`）。**executable 側は `Tile`/`TileMenu`/`TileRect`＋約38 `*Menu` クラス（RTTI）で、C++ クラスは user trait を埋めるだけ**。→ **我々の chargen 前提（UI の正体はデータ）を裏づけ**。
- **`oblivion-exe-recon-rtti-voices-gmst.md`**: RTTI 完全（1,715 vtables / 1,482 classes）・GMST 既定値は**静的初期化子に約2,050個**（master が上書きしない限り有効）。**GMST スキャン手順**: `push NAME; mov ecx,OBJ; call` を軸に、float は10バイト手前（`fld [addr]; push ecx; fstp [esp]`）、`push imm32` は5バイト手前、`fldz`/`fld1` は **`mov ecx,OBJ` が push NAME より前**、小整数は `push imm8`。→ **我々の v610 の gmap 構築（登録バンド）にそのまま効く**。
- **GMST 既定値ダンプは exe 由来 → 公開しない**（`gmst_defaults.py --tsv` の出力は private）。

---

## 10. 移植への指針

1. **UI はデータ**（104 XML）。port 側は「XML を解釈して tile 木を組む」＋「C++ クラスが user trait を埋める」の分離で設計する（compendium の裏づけ・我々の v610 と一致）。
2. **tile 木の正しさは `u47_1.py` で実機検証できる**（RaceSex=`--menu-id 0x40C`）。
3. **trait id** は §6 の表を使う（`0xFA8`=id・`0xFAE`=user0 … `0xFB3`=user5）。
4. **GMST 逆引き**は §9 の手順（二つの命令順・二つの push 形）で gmap を拡張できる。

---

## 11. 残点

- **V-612-1【中】**: `u47_1.py` の実機走査（ゲーム起動時）。静的には配列が空のため未検証。
- **V-612-2【中】**: compendium `gmst_defaults.py` の出力と我々の gmap（v610 §2）の突合（出力は exe 由来・非公開管理）。
- **V-612-3【低】**: reaffirmed/openoblivion のメニュー/XML 実装状況（trait 解釈順序の一次情報になり得る）。
- **V-612-4【低】**: `0x589B70` の 321 callers の分類（どの menuType をどこから引くか）。

---

## 12. 検証

- clone: OBVR(54.8 MB/2154 files)・compendium(2.0 MB/127 files)・openoblivion(1.4 MB/115 files)。読み取り専用。
- md5: `cdd2f0c5eff198d4f26b7b5b54ce4930`（OBNG/compendium 記載と一致）。
- `pe_info.py`: PE32(x86)・7,898,624 B・image base 0x400000・entry 0x7af2ed・**timestamp 0x462392c7 (2007-04-16)**・**no base relocations**・PDB `Oblivion.pdb`・imports に **d3d9 無し／d3dx9_27.dll 有り**（OBVR 記載と一致）・`.bind` 高エントロピー。
- `rtti_scan.py`: RaceSexMenu `0xA6D38C`・LoadingMenu `0xA6C40C`（我々 v611 ctor と一致）・Menu 系 53 vtables。
- probe **u45_1 / u46_1 / u47_1**（EXIT=0・冒頭 Test-Path・recovery 一致）。
- 実読生バイト: `0x589B70`（配列アクセス）／`0x589930`（子走査 `[ecx+0x34]`）／`0xB13970`（vtable `0xA69680`）。
- 二源一致: `g_TileMenuArray`＝我々実読 ＋ OBVR `GameAddresses.h`。
- 外部成果物は `%TEMP%` のみ（リポジトリへは置かない）。
