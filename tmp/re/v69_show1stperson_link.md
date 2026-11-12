# V69: Show1stPerson 文字列表 → フラグ → setter の接続確定（矛盾⑩決着）

- 対象: `Oblivion.exe` (v1.2.0416, Steam版) `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
- マッピング: `VA = 0x400C00 + file_off`（.text）。.rdata VA=0xA28000 Raw=0x627200、.data VA=0xB02000 Raw=0x700C00 RSz=0x30C00
- 日付: 2026-10-24 / 作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`

## 1. セルフテスト（4件 PASS）

| # | 検証 | 結果 |
|---|------|------|
| ① | `bytes@0x504050` | `518b4c24188b5424` (8B) PASS |
| ② | `bytes@0x4FB580` | `578b7908` PASS |
| ③ | `string@0xA7035C` | `.\AI\CombatController.cpp` PASS |
| ④ | テーブル opid `0x0015` (OnLoad) impl | `0x504560`（VA 0xb0b2a8） PASS |

## 2. 文字列表の所在と構造

### 2.1 Show1stPerson/S1ST ペアの発見

`.rdata`/`.data` file-backed 範囲全域（0xA28000–0xB01A00, 0xB02000–0xB32C00）を 4 バイトポインタで走査:

- `0xA513A8`（"Show1stPerson"）への参照: **1 件のみ** → `0xb0b718`（.data）
- `0xA513A0`（"S1ST"）への参照: **1 件のみ** → `0xb0b71c`（.data）
- `.text` からの imm32 直接参照: `0xA513A8`/`0xA513A0`/`0xA51340`(help文) ともに **0 件** ＝テーブル駆動と確定

### 2.2 テーブル構造（self-test ④ と同一配列）

`{Show1stPerson ptr, S1ST ptr}` は **コンソールコマンド/スクリプトイベント統合テーブル** の1エントリ。テーブル全体は .data の連続配列:

| セグメント | 先頭 VA | opcode 範囲 | 件数 | 備考 |
|---|---|---|---|---|
| スクリプトイベント | `0xb0af48` | `0x0000–0x001E` | 31 | GameMode/OnActivate/OnHit…（ブロック種別） |
| コンソールコマンド | `0xb0b420` | `0x0100–0x183` | 132 | Show1stPerson は op `0x113`。末尾 `0x183` は sentinel |
| （別テーブル・同構造） | `0xb0c8c0` | `0x1000–0x1170` | 369 | スクリプト VM オペコード表。本件ラベル対列挙の対象外（→ §6） |

**エントリ構造（stride `0x28` = 40B、10×u32）**:

| offset | 内容 |
|---|---|
| `+0x00` | long name ptr（例 "Show1stPerson"） |
| `+0x04` | short name ptr（4文字コード等。無い場合は共通空文字 `0xa2f7ec`=""） |
| `+0x08` | opcode (u32) |
| `+0x0C` | help 文 ptr（無い場合は `0xa2f7ec`） |
| `+0x10` | flags（`0x00010000`/`0x00020000`/`0x00040000`=引数要求数の様相、low byte はコンパイラで quest script 内可否チェックに使用: `0x4ff918 cmp byte[eax+0x10],0` → nonzero かつ quest script 場合はエラー） |
| `+0x14` | パラメータテーブル ptr（`+0x10` の引数要求がある場合のみ） |
| `+0x18` | **ハンドラ実装関数 ptr（setter 判定の核心）** |
| `+0x1C` | コンパイラ/コードジェネレータ ptr（ほぼ全件 `0x4FDE30` = スクリプト化時の opcode 埋め込み関数） |
| `+0x20..+0x24` | 0 |

opcode→entry 変換は `FUN_004FCA30`（`0x4fca30`）: op `0x100–0x182` → `entry = 0xb0b420 + (op-0x100)*0x28`、op `0x1000–0x1170` → `entry = 0xb0c8c0 + (op-0x1000)*0x28`。

**Show1stPerson エントリ（`0xb0b718`）**: op=`0x113`, long="Show1stPerson"@0xA513A8, short="S1ST"@0xA513A0, help="Show the 1st person Model from the 3rd person camera. If in 3rd person mode it will show both."@0xA51340, flags=0, param=0, **handler=`0x50A050`**, dispatcher=`0x4FDE30`。

### 2.3 ラベル対全列挙（163 件 = イベント31 + コンソール132）

**スクリプトイベント（31 件, op 0x000–0x01E）** — short は `OnPackageDone` のみ `OnPackageEnd`、他は空:

| op | long | short | handler |
|---|---|---|---|
| 0x000 | GameMode | - | 0x5022d0 |
| 0x001 | MenuMode | - | 0x502270 |
| 0x002 | OnActivate | - | 0x503f90 |
| 0x003 | OnAdd | - | 0x503d10 |
| 0x004 | OnEquip | - | 0x503e10 |
| 0x005 | OnUnequip | - | 0x503e90 |
| 0x006 | OnDrop | - | 0x503f10 |
| 0x007 | OnYield_unused | - | 0x504010 |
| 0x008 | OnHit | - | 0x504050 |
| 0x009 | OnHitWith | - | 0x5040d0 |
| 0x00a | OnDeath | - | 0x504150 |
| 0x00b | OnMurder | - | 0x504250 |
| 0x00c | OnKnockout | - | 0x5042d0 |
| 0x00d | OnAlarm | - | 0x504310 |
| 0x00e | OnAlarmVictim | - | 0x50b3e0 |
| 0x00f | OnPackageStart | - | 0x5043e0 |
| 0x010 | OnPackageDone | OnPackageEnd | 0x504460 |
| 0x011 | ScriptEffectStart | - | 0x5022f0 |
| 0x012 | ScriptEffectFinish | - | 0x502320 |
| 0x013 | ScriptEffectUpdate | - | 0x50a120 |
| 0x014 | OnPackageChange | - | 0x5044e0 |
| 0x015 | OnLoad | - | 0x504560 |
| 0x016 | OnMagicEffectHit | - | 0x5045e0 |
| 0x017 | OnSell | - | 0x503d90 |
| 0x018 | OnTrigger | - | 0x504660 |
| 0x019 | OnStartCombat | - | 0x5041d0 |
| 0x01a | OnTriggerActor | - | 0x5046e0 |
| 0x01b | OnTriggerMob | - | 0x504760 |
| 0x01c | OnActorEquip | - | 0x503e10 |
| 0x01d | OnActorUnequip | - | 0x503e90 |
| 0x01e | OnReset | - | 0x5045a0 |

**コンソールコマンド（132 件, op 0x100–0x183）**:

| op | long | short | handler |
|---|---|---|---|
| 0x100 | Show | TST | 0x502210 |
| 0x101 | ShowVars | SV | 0x5097a0 |
| 0x102 | ShowQuestVars | SQV | 0x509b30 |
| 0x103 | ShowQuests | SQ | 0x509d10 |
| 0x104 | ToggleCombatStats | TCS | 0x500770 |
| 0x105 | ToggleSafeZone | TSZ | 0x500f60 |
| 0x106 | ToggleAI | TAI | 0x5011f0 |
| 0x107 | ToggleCollisionGeometry | TCG | 0x500e30 |
| 0x108 | ToggleMaterialGeometry | TMG | 0x500e40 |
| 0x109 | ToggleBorders | TB | 0x507c60 |
| 0x10a | ToggleSky | TS | 0x507cb0 |
| 0x10b | ToggleLeaves | TLV | 0x507dc0 |
| 0x10c | ToggleWireframe | TWF | 0x50fca0 |
| 0x10d | ToggleCollision | TCL | 0x507d30 |
| 0x10e | ToggleDebugText | TDT | 0x501000 |
| 0x10f | ToggleMenus | TM | 0x500f10 |
| 0x110 | ShowScenegraph | SSG | 0x50fd10 |
| 0x111 | ToggleMagicStats | TMS | 0x507bd0 |
| 0x112 | ShowAnim | SA | 0x509d70 |
| 0x113 | **Show1stPerson** | **S1ST** | **0x50a050** |
| 0x114 | Help | - | 0x501540 |
| 0x115 | TogglePathGrid | TPG | 0x501650 |
| 0x116 | TogglePathLine | TPL | 0x501660 |
| 0x117 | PickRefByID | PRID | 0x513e90 |
| 0x118 | SetLightingPasses | SLP | 0x5101e0 |
| 0x119 | ToggleLODLand | TLL | 0x508240 |
| 0x11a | ToggleGrassUpdate | TGU | 0x501a60 |
| 0x11b | CenterOnCell | COC | 0x5083c0 |
| 0x11c | SetGameSetting | SetGS | 0x514100 |
| 0x11d | SetINISetting | SetINI | 0x5143c0 |
| 0x11e | GetINISetting | GetINI | 0x514230 |
| 0x11f | CenterOnExterior | COE | 0x5084a0 |
| 0x120 | CalcPathToPoint | P2P | 0x510c70 |
| 0x121 | CalcLowPathToPoint | LP2P | 0x510e60 |
| 0x122 | SetFog | - | 0x5087c0 |
| 0x123 | SetClipDist | - | 0x510480 |
| 0x124 | ToggleShadowVolumes | TSV | 0x5087a0 |
| 0x125 | SetImageSpaceGlow | SISG | 0x508860 |
| 0x126 | ToggleDetection | TDETECT | 0x5010e0 |
| 0x127 | ToggleHighProcess | THIGHPROCESS | 0x5013f0 |
| 0x128 | ToggleLowProcess | TLOWPROCESS | 0x501440 |
| 0x129 | ToggleMiddleLowProcess | TMLOWPROCESS | 0x501480 |
| 0x12a | ToggleMiddleHighProcess | TMHIGHPROCESS | 0x5014c0 |
| 0x12b | ToggleAiSchedules | TAIS | 0x501500 |
| 0x12c | SpeakSound | - | 0x513c90 |
| 0x12d | BetaComment | BC | 0x50a400 |
| 0x12e | ToggleCellNode | TCN | 0x50a8a0 |
| 0x12f | ToggleTrees | TT | 0x50a9a0 |
| 0x130 | SetCameraFOV | FOV | 0x511720 |
| 0x131 | Verbose | VERBOSE | 0x507930 |
| 0x132 | ToggleGodMode | TGM | 0x500d10 |
| 0x133 | RefreshShaders | REFRESHSHADERS | 0x50aa20 |
| 0x134 | ToggleScripts | TSCR | 0x500d50 |
| 0x135 | SexChange | - | 0x515920 |
| 0x136 | ToggleConversations | TCONV | 0x507ba0 |
| 0x137 | ToggleFullHelp | TFH | 0x500e00 |
| 0x138 | ShowQuestLog | SQL | 0x5120a0 |
| 0x139 | ShowFullQuestLog | SFQL | 0x512220 |
| 0x13a | DumpTexturePalette | DTP | 0x515330 |
| 0x13b | CenterOnWorld | COW | 0x5085d0 |
| 0x13c | ToggleGrass | TG | 0x50ec90 |
| 0x13d | AddFaceAnimNote | AFAN | 0x505120 |
| 0x13e | RefreshINI | REFINI | 0x513bb0 |
| 0x13f | ToggleEmotions | temo | 0x507a00 |
| 0x140 | AddDecal | - | 0x510580 |
| 0x141 | TestCode | - | 0x977c50 |
| 0x142 | ToggleMapMarkers | tmm | 0x507800 |
| 0x143 | PlayerSpellBook | psb | 0x507740 |
| 0x144 | PrintAiList | pai | 0x5007a0 |
| 0x145 | ShowQuestTargets | SQT | 0x515af0 |
| 0x146 | HairTint | - | 0x508b80 |
| 0x147 | SaveGame | save | 0x5023d0 |
| 0x148 | LoadGame | load | 0x502500 |
| 0x149 | TestAllCells | TAC | 0x507960 |
| 0x14a | SaveWorld | - | 0x500e60 |
| 0x14b | ReloadCurrentClimate | rcc | 0x507a30 |
| 0x14c | ReloadCurrentWeather | rcw | 0x507a90 |
| 0x14d | TestSeenData | tsd | 0x50ed50 |
| 0x14e | TestLocalMap | tlm | 0x50ef90 |
| 0x14f | MoveToQuestTarget | movetoqt | 0x512910 |
| 0x150 | PrintNPCDialog | pdialog | 0x5007c0 |
| 0x151 | ShowSubtitle | - | 0x5007d0 |
| 0x152 | ShowRenderPasses | srp | 0x500820 |
| 0x153 | FreezeRenderAccumulation | fra | 0x500830 |
| 0x154 | ToggleOcclusion | tocc | 0x500860 |
| 0x155 | OutputMemContexts | omc | 0x500890 |
| 0x156 | OutputMemStats | oms | 0x5009e0 |
| 0x157 | ToggleDetectionStats | tds | 0x500f70 |
| 0x158 | SetTargetRefraction | str | 0x507e60 |
| 0x159 | SetTargetRefractionFire | strf | 0x507ff0 |
| 0x15a | ToggleRefractionDebug | trd | 0x508180 |
| 0x15b | ToggleCharControllerShape | TCCS | 0x507c00 |
| 0x15c | ShowHeadTrackTarget | SHeadT | 0x508340 |
| 0x15d | SetSkyParam | SSP | 0x503c60 |
| 0x15e | SetHDRParam | SHP | 0x50b0c0 |
| 0x15f | ShowPivot | SP | 0x511c10 |
| 0x160 | PrintHDRParam | PHP | 0x50b210 |
| 0x161 | ToggleHDRDebug | THD | 0x503ce0 |
| 0x162 | RevertWorld | rw | 0x500c90 |
| 0x163 | OutputArchiveProfile | oap | 0x500ae0 |
| 0x164 | QuitGame | qqq | 0x5077e0 |
| 0x165 | SaveIniFiles | saveini | 0x513b60 |
| 0x166 | SetDebugText | sdt | 0x5081f0 |
| 0x167 | ToggleLiteBrite | tlb | 0x5081b0 |
| 0x168 | RunMemoryPass | rmp | 0x501040 |
| 0x169 | ResetMemContexts | rmc | 0x500890 |
| 0x16a | ToggleWaterSystem | TWS | 0x50d860 |
| 0x16b | ToggleWaterRadius | TWR | 0x50d8c0 |
| 0x16c | ShowWhoDetectsPlayer | SWDP | 0x500780 |
| 0x16d | ToggleCombatAI | tcai | 0x508300 |
| 0x16e | ToggleFlyCam | tfc | 0x5013e0 |
| 0x16f | ModWaterShader | mws | 0x50d8e0 |
| 0x170 | WaterShallowColor | shallow | 0x50e250 |
| 0x171 | WaterDeepColor | deep | 0x5065b0 |
| 0x172 | WaterReflectionColor | refl | 0x506680 |
| 0x173 | BeginTrace | bt | 0x977c50 |
| 0x174 | RunCellTest | rct | 0x5068c0 |
| 0x175 | StartAllQuests | saq | 0x50e5a0 |
| 0x176 | CompleteAllQuestStages | caqs | 0x50e600 |
| 0x177 | FlushNonPersistActors | Flush | 0x506920 |
| 0x178 | ToggleFogOfWar | TFOW | 0x506990 |
| 0x179 | OutputLocalMapPictures | OLMP | 0x50e6a0 |
| 0x17a | SetGamma | sg | 0x50e950 |
| 0x17b | WasteMemory | wm | 0x5069d0 |
| 0x17c | ClearAdaptedLight | cal | 0x506ad0 |
| 0x17d | ToggleCastShadows | tsh | 0x506b80 |
| 0x17e | ShowSubSpaces | sss | 0x506c80 |
| 0x17f | SetSTBBColorConstants | sscc | 0x508980 |
| 0x180 | CloseFile | - | 0x508a20 |
| 0x181 | SetTreeMipmapBias | stmb | 0x514500 |
| 0x182 | ClearFaceGenModelData | CFG | 0x506e90 |
| 0x183 | ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!!（sentinel, handler=0） | - | 0x000000 |

主要な4文字ラベル対の例: `S1ST/Show1stPerson`, `SA/ShowAnim`, `SSG/ShowScenegraph`, `TMS/ToggleMagicStats`, `TDT/ToggleDebugText`。

## 3. テーブル読者（オプション実装）の特定

.text 内のテーブル先頭/境界アドレスへの参照を走査して得られた読者群:

| 関数 | VA | 役割 |
|---|---|---|
| `FUN_004FCA70` | `0x4fca70` | **名前→opcode 解決器**。イベント表(0xb0a2cc–0xb0a54c 相当)・VM表(0xb0c8c4–0xb1026c)・コンソール表(0xb0b424–0xb0c89c) の3配列を走査し、各エントリの long/short 両名を `strcmp`(`0x98262d`)。一致時は `[edi+0x204]=0x58`、`[edi+0x208]=opcode` を格納。呼び出し元: `0x4fda06`, `0x4fdade`（コンソール入力実行系） |
| `FUN_004FF710` | `0x4ff710` | スクリプトコンパイラ。イベント表 31 エントリ（0xb0af4c〜0xb0b424, long/short 両名 strcmp）を走査し、opcode をコンパイル済みバッファに埋め込み。quest script 内では `+0x10` フラグで可否検証 |
| `FUN_004FCA30` | `0x4fca30` | opcode→entry アドレス変換。呼び出し元: `0x4fad43`（script VM ディスポータ）, `0x5168ea`, `0x51703e` |
| `FUN_00501540` | `0x501540` | `Help` コマンド。コンソール表を走査し long/short/help を列挙表示（"%s (%s) -> %s"）。VM 表(0xb0c8c0+)も同様に走査 |

### 3.1 `S1ST` 等の4文字コードの使われ方

- **純粋な strcmp 用 lookup キー**（long/short のどちらでも可）。構築・送信は一切行われない。
- 一致時 opcode `0x113` が引出され、opcode→entry 変換/VM ディスパッチを経て `entry+0x18` のハンドラが呼ばれる。

### 3.2 読者とフラグの関係

- 読者4関数（`0x4fca70`/`0x4ff710`/`0x4fca30`/`0x501540`）はいずれも **byte `0x588` / `DAT_00b333c4` に一切アクセスしない**（本文確認済み）。
- フラグアクセスはテーブル経由で呼ばれる **ハンドラ `0x50A050` のみ**。

### 3.3 ハンドラ `0x50A050`（Show1stPerson 実装）の解析

capstone 逆アセンブル（0x50a050–0x50a11a、int3 で前後と区切られた独立関数）:

```
0x50a050: mov ecx, [0xb333c4]              ; ecx = 基点オブジェクト
0x50a056: cmp byte [ecx+0x588], 0          ; フラグ読取
0x50a05d: je  0x50a0fb                     ; 0 なら「ON化」パス
0x50a063: push 0 ; call 0x660110            ; FUN_00660110(0) → 3rd person ノード取得 (0x612220 経由)
0x50a06a: test byte [eax+0x18], 1           ; ノード +0x18 bit0（表示フラグ）
0x50a06e: je  0x50a099
0x50a070: mov eax, [0xb333c4]
0x50a075: mov byte [eax+0x588], 0           ; ★フラグ書込み site#1 = 0
          ... verbose時 "Normal 1st person mode."
0x50a099: push 1 ; call 0x660110            ; FUN_00660110(1) → 1st person ノード ([ecx+0x5d0])
          or/and word [eax+0x18], 1/0xfffe  ; ノード表示 bit トグル
          ... verbose時 "Normal 3rd person mode." / "Showing 1st and 3rd person models."
0x50a0fb: mov byte [ecx+0x588], 1           ; ★フラグ書込み site#2 = 1
          ... verbose時 "Showing 1st person model in 3rd person camera."
0x50a118: mov al, 1 ; ret
```

- 呼び出し先: **`FUN_00660110`（1st/3rd person ノードgetter）と `FUN_00579b60`（コンソール出力）のみ**。`FUN_0066C580` / `FUN_00671620` は **呼ばない**。
- decompiled 側: 0x50A050–0x50A11A は `docs\Oblivion_decompiled.c` に **FUN ラベルなし**（FUN_005097a0 は ShowVars 実装 0x5097a0–0x509b2f で終端、次は FUN_00509b30。0x509d10/0x509d70/0x50a050 は未ラベル領域）。ShowVars(FUN_005097a0) 本文に `b333c4`/`0x162`/`0x588` 参照は **0 件**。
- 出力メッセージは help 文の意味（3rd person カメラに 1st person モデル表示）と完全一致。

## 4. setter 勝敗判定

**接続チェーン（確定）**:

```
コンソール入力 "S1ST"/"Show1stPerson"
  → FUN_004FCA70 名前解決（strcmp）→ opcode 0x113
  → entry 0xb0b718（+0x18）
  → ハンドラ 0x50A050
  → 直接: [DAT_00b333c4 + 0x588] = 0 (0x50A075) / 1 (0x50A0FB)
  → 副次: FUN_00660110 で 1st/3rd person ノードの +0x18 bit0 トグル
```

**判定: (B) — ただし表現を訂正して採用。**

- **旧説B のアドレス `0x50A050` が Show1stPerson 専用 setter の実体そのもの**。テーブル `0xb0b718+0x18` が指す実装であり、byte `0x588` ライタ 7 件リスト中の `0x50A075`/`0x50A0FB` はこの関数に属する。
- **訂正1**: 「`FUN_005097A0` の内部ブロック」は誤り。0x50A050–0x50A11A は int3 境界で区切られた**独立関数**であり、Ghidra decompiled にも FUN ラベルがない。`FUN_005097A0`(ShowVars) は 0x5097a0–0x509b2f で終端し、フラグ非参照（keyword scan 0 件）。旧「ライタ=FUN_005097A0(0x50A075/0x50A0FB)」表記は、naive スキャンによる近接ラベル誤帰属だった。
- **却下 (A)**: `FUN_0066C580` は `bool FUN_0066C580(this, char v)` 形式の**エンジン共通フラグ setter**（`[this+0x588] = !v`、変化時ハンドラ 0x66b710/0x664f70 呼び出し、decompiled 呼び出し約21–22箇所）。S1ST ハンドラはこれを呼ばない。S1ST 経路の setter ではない。
- **(C) との関係**: 7 ライタ一覧の「FUN_005097A0」エントリの正体が本ハンドラ（0x50A050）である、という意味で旧リスト自体は総数7件のまま有効。新たな別関数が加わったわけではない。
- **(D) 不要**: 接続は完全に確定。

## 5. 未解決点

1. `DAT_00b333c4` の正体（オブジェクトクラス）。byte `0x588` の宿主だが、Main/TESMain かは未特定。
2. `FUN_0066C580` の約22呼び出し元（エンジン側トランジション）の各文脈分類。S1ST 以外のフラグ変化経路の全体像。
3. `entry+0x10` フラグの厳密な意味（引数要求数 vs console-only ビット）は `0x4ff918` での使用から部分推定のみ。
4. コンソール実行系（`0x4fd9dd` 周辺）が opcode 後に `entry+0x18` ハンドラへ到達する最終 call 命令の正確な所在（script VM 側 `0x4fad43` は確認済み、コンソール側は resolver 格納後の VM 実行に合流する様相だが未トレース）。

## 6. 副次メモ: VM オペコード表（0xb0c8c0, 369 件, op 0x1000–0x1170）

本表も同一構造（stride 0x28, long/short ペア）で `FUN_004FCA70`/`FUN_00501540` が走査するが、これはスクリプト VM オペコード（If/Set/関数呼び出し等）であり「オプションメニュー」ではないため §2.3 の全列挙対象外。件数 369、先頭 `0xb0c8c0`、範囲 `0xb0c8c0–0xb10268`。全ラベル列挙は別タスク（V70 候補）。
