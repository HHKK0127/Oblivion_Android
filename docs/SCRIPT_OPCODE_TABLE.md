# Oblivion スクリプト オペコード表 (532 件)

`OblivionStandalone.exe`（SHA256 `A8F313845C1545E9A60E1E995961EEF4C033115DA9443F6D756341DF3C2B7DC6`）の
オペコードテーブル **`0xB0AF48`**（532 エントリ × 0x28、範囲 `[0xB0AF48,0xB10268)`）を直接読み出して生成した正典表。
生成器は `tmp/re/v81_gen_opcode_table.py`（Git 管理外）。レイアウトの根拠は `docs/RE_EXE_REPORT.md` §70.4 / §70.8-1。

## レイアウト

| オフセット | 型 | 意味 |
|---|---|---|
| `+0x00` | dword ptr | 長名（コンソール/ドキュメント名） |
| `+0x04` | dword ptr | 短名（別名。実別名 176 件、空は共有 ptr `0xA2F7EC` が 356 件） |
| `+0x08` | dword | **オペコード id** |
| `+0x0C` | dword ptr | 説明文（ヘルプ。非空 132 件・空 399 件） |
| `+0x10` | byte | フラグ A（begin ブロック適法性。`1` が 272 件） |
| `+0x11` | byte | 未使用（全 532 件 0） |
| `+0x12` | word | **引数の個数** |
| `+0x14` | dword ptr | **引数記述子配列 ptr**（`.data` プール `0xB0A11C`〜`0xB0AF3C`、12 B レコード。`0xB0A7EC` 等の別名ポインタではない） |
| `+0x18` | dword ptr | **実装（ハンドラ）関数** |
| `+0x1C` | dword ptr | **共有 dispatcher / 実行器**（既定 `0x4FDE30`、例外 3 種） |
| `+0x20` | dword ptr | **条件関数（CTDA 対応オペコードのみ）** |
| `+0x24` | dword | 小フラグ（`0x100` / `0x1`） |

id は 3 帯域に分かれる: `0x00-0x1E`（block/イベント 31 件）/ `0x100-0x183`（コマンド 132 件）/ `0x1000-0x1170`（スクリプト関数 369 件）。

引数の個数の分布: 0 個 = 251 件, 1 個 = 195 件, 2 個 = 65 件, 3 個 = 12 件, 4 個 = 5 件, 5 個 = 3 件, 6 個 = 1 件

### 0x0000〜0x001E 帯の注意（ブロック型 ID）

この帯の 31 件（`0x0000`〜`0x001E`）は、**`0x0010` Begin 命令の第 1 引数（ブロック型 ID）**に対応する登録エントリである。
同じ数値が SCDA バイトコード上に「オペコード」として現れることもあり、**両者は別用途**である（`docs/RE_EXE_REPORT.md` §57.4・§70.14）。

実測（バニラ `Oblivion3.esm` の SCDA 2,393 スクリプト / 65,628 命令。証跡 `tmp/re/s59_svm_r7_bytecode.txt`）:

- **ブロック型 ID としての出現数**: `0x00` GameMode = 1332 / `0x02` OnActivate = 891 / `0x0A` OnDeath = 441 / `0x10` OnPackageDone = 307 / `0x1E` OnReset = 215 / `0x15` OnLoad = 208 / `0x11` ScriptEffectStart = 84 / `0x1C` OnActorEquip = 1 / **`0x1D` OnActorUnequip = 0（バニラでは未使用）**。
- **SCDA の制御オペコードとして独立に出現するもの**: `0x0010` Begin = 4,344 / `0x0011` ブロック終端 = 4,344 / `0x0015` Set = 12,098 / `0x0016` If = 10,480 / `0x0017` Else = 953 / `0x0018` ElseIf = 1,986 / `0x0019` EndIf = 10,530 / `0x001C` 参照スロット接頭辞 = 5,647 / `0x001D` プロローグ = 2,393 / `0x001E` Return = 191。
- したがって、本表の `0x0011 ScriptEffectStart` と SCDA のブロック終端 `0x0011`、`0x001C OnActorEquip` と参照スロット接頭辞 `0x001C`、`0x001D OnActorUnequip` とプロローグ `0x001D` は**同名値の別用途**である。

### `+0x14` 引数記述子テーブル

`+0x14` は `.data` の記述子プール `[0xB0A11C, 0xB0AF48)` を指し、12 バイトのレコード
`{ const char* name /* .rdata */; u32 type /* 0x00-0x25 = 38 種 */; u32 optional /* 0 or 1 */ }`
が `+0x12`（引数個数）ぶん連続する。全 532 件のうち **281 件**が非ゼロで、これは `+0x12 > 0` と完全一致（例外 0）。
実体レコードは **179 ユニーク**（`0xB0A6CC` = `{Integer, 1, 0}` を 42 コマンドが共用する等、共有が多い）。

型コード 38 種の目録:

| code | 型 | code | 型 | code | 型 |
|---|---|---|---|---|---|
| `0x00` | String / Filename | `0x0D` | Topic | `0x1A` | Container |
| `0x01` | Integer | `0x0E` | Quest | `0x1B` | WorldSpace |
| `0x02` | Float | `0x0F` | Race | `0x1C` | Crime Type |
| `0x03` / `0x15` | ObjectID | `0x10` | Class | `0x1D` | Package |
| `0x04` | ObjectReferenceID | `0x11` | Faction | `0x1E` | Combat Style |
| `0x05` | Actor Value | `0x12` | Sex | `0x1F` | Magic Effect |
| `0x06` | Actor / Criminal / Victim | `0x13` | Global | `0x20` | Birthsign |
| `0x07` | Spell Item | `0x14` | Furniture | `0x21` | Form Type |
| `0x08` | Axis | `0x16` | Variable Name | `0x22` | Weather ID |
| `0x09` | Cell | `0x17` | Stage | `0x23` | NPC |
| `0x0A` | Animation Group | `0x18` | Map Marker | `0x24` | Owner |
| `0x0B` | Magic Item | `0x19` | Actor Base | `0x25` | Effect Shader ID |
| `0x0C` | Sound | | | | |

消費側は共有 dispatcher **`0x4FDE30`**（528/532）で、コンパイラ `FUN_004ff710`（`0x4FF710`）が
`argn = entry+0x12` / `argdesc = entry+0x14` / `dispatcher = entry+0x1C` を取り出して
`(*dispatcher)(argn, argdesc, &argbuf, ctx)` を呼び、記述子列に従って引数をデコード・検証する。
詳細な全数計測は `docs/RE_EXE_REPORT.md` §70.4 の (V83) 節、証跡は `tmp/re/v83_col14_meaning.md`（Git 管理外）。

### 型コード 38 種のコンパイル挙動（V84・実測）

`+0x14` の型コードは **`ScriptCompiler::StandardCompile` の引数型**であり、コンパイル時の分岐は `+0x1C` の共有 dispatcher **`0x4FDE30`**（引数コンパイラ）にある。
証跡: `tmp/re/v84b_switch.txt` / `tmp/re/v84a_argtype_parity.py` / `tmp/re/v84a_run2.txt`。

- スイッチ前文（`0x4FDF7C`〜`0x4FDFA0`）は型情報テーブル **`0xB0A548`**（8 B × 38、`{u32 type_code; u8 fieldA; u8 fieldB; u8 pad2}`）を引き、`fieldB` で 2 ファミリに分岐する（`1` = main 27 型 / `0` = alt 11 型）。`fieldA==1` は `0x01` Integer / `0x02` Float / `0x17` Stage の 3 型のみ。
- **main 27 型**（フォーム参照系。`0x03`〜`0x25` の範囲で `fieldB==1`）は 27 ハンドラすべてが `call 0x9832E6`（フォーム検索）と `call 0x4FCE30`（`Unknown variable '%s' for parameter %s.`）へ委譲し、**型に依らず同一形式**にコンパイルされる。`0x04` / `0x06` / `0x18` / `0x1A` は同一ハンドラ `0x4FDFFF` を共有する。
- main スイッチテーブル `0x4FEEAC` は 35 エントリ（index = type − 3）。うち 8 エントリは `0x4FED7E`（"ここに来てはいけない型" のトラップ）で、`fieldB==0` の範囲内 8 型と一致する。
- **alt 11 型は 9 経路**に分かれる。alt テーブルは `0x4FEF38`（`0x4FEEAC + 35*4` と連続）、分配は `0x4FEF5C` の 38 B バイトテーブル。

| # | 型コード | 引数型名 | コンパイル結果 |
|---|---|---|---|
| 0 | `0x00` `0x25` | String / Filename, Effect Shader ID | 文字列 → u16 インデックス（`0x9812C0`） |
| 1 | `0x01` `0x02` `0x17` | Integer, Float, Stage | リテラル型タグ付きのインライン数値（先頭 `0x47`='G'） |
| 2 | `0x05` | Actor Value | 列挙テーブル `0xB0A1A8` |
| 3 | `0x08` | Axis | 軸文字 X/Y/Z |
| 4 | `0x0A` | Animation Group | 列挙テーブル `0xB102E0` |
| 5 | `0x12` | Sex | 列挙テーブル `0xB10BC4` |
| 6 | `0x1C` | Crime Type | `0x47D550` → `0x983468` |
| 7 | `0x21` | Form Type | 列挙テーブル `0xB081D0` |
| 8 | `0x16` | Variable Name | **未実装**（`Param type '%d' unimplemented in ScriptCompiler::StandardCompile.`） |

型別の引数スロット数（全 532 コマンドの実測。38 種すべてが使用される）:

`0x01`=135 / `0x02`=47 / `0x06`=34 / `0x00`=27 / `0x11`=22 / `0x04`=19 / `0x0E`=10 / `0x09`=9 / `0x05`=`0x08`=`0x15`=`0x1D`=`0x1A`=7 / `0x03`=`0x1B`=`0x22`=`0x1C`=`0x13`=`0x10`=`0x0D`=`0x1F` 等は 1〜5 / 総計 402

- optional=1 の型: `0x02`=13 / `0x01`=39 / `0x06`=14 / `0x04`=5 / `0x00`=4 など（全 52 スロット）。うち 11 件は名前に `(optional)` を持たない（`ObjectID(0x15)` / `ObjectReferenceID(0x04)`×2 / `Emotion(0x01)` / `EmotionValue(0x01)` / `String(0x00)` / `Count(0x01)` / `Distance(0x01)` / `Direction(0x01)` / `Topic(0x0D)` / `Flag(0x01)`）。
- 未消費の記述子枠は 13 件（`int` / `scn` / `ref` の 3 件は型情報テーブル等の拾い上げによる偽陽性寄り、残り 10 件は予備枠）。

## 全 532 件

handler が空（`-`）の 5 件は「パースはされるが実装なし」。条件関数（cond）は 169 件。

| # | opcode | 長名 | 短名 | 引数 | ハンドラ | dispatcher | 条件 | 説明 |
|---|---|---|---|---|---|---|---|---|
| 0 | 0x0000 | GameMode | - | 0 | `0x5022D0` | `0x4FDE30` | - | - |
| 1 | 0x0001 | MenuMode | - | 1 | `0x502270` | `0x4FDE30` | - | - alias:0x1024 |
| 2 | 0x0002 | OnActivate | - | 1 | `0x503F90` | `0x4FDE30` | - | - |
| 3 | 0x0003 | OnAdd | - | 1 | `0x503D10` | `0x4FDE30` | - | - |
| 4 | 0x0004 | OnEquip | - | 1 | `0x503E10` | `0x4FDE30` | - | - alias:0x001C |
| 5 | 0x0005 | OnUnequip | - | 1 | `0x503E90` | `0x4FDE30` | - | - alias:0x001D |
| 6 | 0x0006 | OnDrop | - | 1 | `0x503F10` | `0x4FDE30` | - | - |
| 7 | 0x0007 | OnYield_unused | - | 1 | `0x504010` | `0x4FDE30` | - | - |
| 8 | 0x0008 | OnHit | - | 1 | `0x504050` | `0x4FDE30` | - | - |
| 9 | 0x0009 | OnHitWith | - | 1 | `0x5040D0` | `0x4FDE30` | - | - |
| 10 | 0x000A | OnDeath | - | 1 | `0x504150` | `0x4FDE30` | - | - |
| 11 | 0x000B | OnMurder | - | 1 | `0x504250` | `0x4FDE30` | - | - |
| 12 | 0x000C | OnKnockout | - | 0 | `0x5042D0` | `0x4FDE30` | - | - |
| 13 | 0x000D | OnAlarm | - | 2 | `0x504310` | `0x4FDE30` | - | - |
| 14 | 0x000E | OnAlarmVictim | - | 2 | `0x50B3E0` | `0x4FDE30` | - | - |
| 15 | 0x000F | OnPackageStart | - | 1 | `0x5043E0` | `0x4FDE30` | - | - |
| 16 | 0x0010 | OnPackageDone | OnPackageEnd | 1 | `0x504460` | `0x4FDE30` | - | - |
| 17 | 0x0011 | ScriptEffectStart | - | 0 | `0x5022F0` | `0x4FDE30` | - | - |
| 18 | 0x0012 | ScriptEffectFinish | - | 0 | `0x502320` | `0x4FDE30` | - | - |
| 19 | 0x0013 | ScriptEffectUpdate | - | 0 | `0x50A120` | `0x4FDE30` | - | - |
| 20 | 0x0014 | OnPackageChange | - | 1 | `0x5044E0` | `0x4FDE30` | - | - |
| 21 | 0x0015 | OnLoad | - | 0 | `0x504560` | `0x4FDE30` | - | - |
| 22 | 0x0016 | OnMagicEffectHit | - | 1 | `0x5045E0` | `0x4FDE30` | - | - |
| 23 | 0x0017 | OnSell | - | 1 | `0x503D90` | `0x4FDE30` | - | - |
| 24 | 0x0018 | OnTrigger | - | 1 | `0x504660` | `0x4FDE30` | - | - |
| 25 | 0x0019 | OnStartCombat | - | 1 | `0x5041D0` | `0x4FDE30` | - | - |
| 26 | 0x001A | OnTriggerActor | - | 1 | `0x5046E0` | `0x4FDE30` | - | - |
| 27 | 0x001B | OnTriggerMob | - | 1 | `0x504760` | `0x4FDE30` | - | - |
| 28 | 0x001C | OnActorEquip | - | 1 | `0x503E10` | `0x4FDE30` | - | - alias:0x0004 |
| 29 | 0x001D | OnActorUnequip | - | 1 | `0x503E90` | `0x4FDE30` | - | - alias:0x0005 |
| 30 | 0x001E | OnReset | - | 0 | `0x5045A0` | `0x4FDE30` | - | - |
| 31 | 0x0100 | Show | TST | 1 | `0x502210` | `0x4FDE30` | - | yes |
| 32 | 0x0101 | ShowVars | SV | 0 | `0x5097A0` | `0x4FDE30` | - | yes |
| 33 | 0x0102 | ShowQuestVars | SQV | 1 | `0x509B30` | `0x4FDE30` | - | yes |
| 34 | 0x0103 | ShowQuests | SQ | 0 | `0x509D10` | `0x4FDE30` | - | yes |
| 35 | 0x0104 | ToggleCombatStats | TCS | 0 | `0x500770` | `0x4FDE30` | - | - |
| 36 | 0x0105 | ToggleSafeZone | TSZ | 0 | `0x500F60` | `0x4FDE30` | - | yes |
| 37 | 0x0106 | ToggleAI | TAI | 1 | `0x5011F0` | `0x4FDE30` | - | - |
| 38 | 0x0107 | ToggleCollisionGeometry | TCG | 0 | `0x500E30` | `0x4FDE30` | - | yes |
| 39 | 0x0108 | ToggleMaterialGeometry | TMG | 0 | `0x500E40` | `0x4FDE30` | - | yes |
| 40 | 0x0109 | ToggleBorders | TB | 0 | `0x507C60` | `0x4FDE30` | - | yes |
| 41 | 0x010A | ToggleSky | TS | 0 | `0x507CB0` | `0x4FDE30` | - | - |
| 42 | 0x010B | ToggleLeaves | TLV | 0 | `0x507DC0` | `0x4FDE30` | - | - |
| 43 | 0x010C | ToggleWireframe | TWF | 0 | `0x50FCA0` | `0x4FDE30` | - | yes |
| 44 | 0x010D | ToggleCollision | TCL | 0 | `0x507D30` | `0x4FDE30` | - | - |
| 45 | 0x010E | ToggleDebugText | TDT | 0 | `0x501000` | `0x4FDE30` | - | yes |
| 46 | 0x010F | ToggleMenus | TM | 0 | `0x500F10` | `0x4FDE30` | - | yes |
| 47 | 0x0110 | ShowScenegraph | SSG | 0 | `0x50FD10` | `0x4FDE30` | - | yes |
| 48 | 0x0111 | ToggleMagicStats | TMS | 0 | `0x507BD0` | `0x4FDE30` | - | - |
| 49 | 0x0112 | ShowAnim | SA | 0 | `0x509D70` | `0x4FDE30` | - | yes |
| 50 | 0x0113 | Show1stPerson | S1ST | 0 | `0x50A050` | `0x4FDE30` | - | yes |
| 51 | 0x0114 | Help | - | 0 | `0x501540` | `0x4FDE30` | - | yes |
| 52 | 0x0115 | TogglePathGrid | TPG | 0 | `0x501650` | `0x4FDE30` | - | yes |
| 53 | 0x0116 | TogglePathLine | TPL | 0 | `0x501660` | `0x4FDE30` | - | yes |
| 54 | 0x0117 | PickRefByID | PRID | 1 | `0x513E90` | `0x4FDE30` | - | yes |
| 55 | 0x0118 | SetLightingPasses | SLP | 1 | `0x5101E0` | `0x4FDE30` | - | yes |
| 56 | 0x0119 | ToggleLODLand | TLL | 0 | `0x508240` | `0x4FDE30` | - | - |
| 57 | 0x011A | ToggleGrassUpdate | TGU | 0 | `0x501A60` | `0x4FDE30` | - | - |
| 58 | 0x011B | CenterOnCell | COC | 1 | `0x5083C0` | `0x4FDE30` | - | - |
| 59 | 0x011C | SetGameSetting | SetGS | 2 | `0x514100` | `0x4FDE30` | - | - |
| 60 | 0x011D | SetINISetting | SetINI | 2 | `0x5143C0` | `0x4FDE30` | - | yes |
| 61 | 0x011E | GetINISetting | GetINI | 1 | `0x514230` | `0x4FDE30` | - | yes |
| 62 | 0x011F | CenterOnExterior | COE | 2 | `0x5084A0` | `0x4FDE30` | - | - |
| 63 | 0x0120 | CalcPathToPoint | P2P | 0 | `0x510C70` | `0x4FDE30` | - | - |
| 64 | 0x0121 | CalcLowPathToPoint | LP2P | 3 | `0x510E60` | `0x4FDE30` | - | yes |
| 65 | 0x0122 | SetFog | - | 2 | `0x5087C0` | `0x4FDE30` | - | yes |
| 66 | 0x0123 | SetClipDist | - | 1 | `0x510480` | `0x4FDE30` | - | yes |
| 67 | 0x0124 | ToggleShadowVolumes | TSV | 0 | `0x5087A0` | `0x4FDE30` | - | - |
| 68 | 0x0125 | SetImageSpaceGlow | SISG | 4 | `0x508860` | `0x4FDE30` | - | - |
| 69 | 0x0126 | ToggleDetection | TDETECT | 0 | `0x5010E0` | `0x4FDE30` | - | - |
| 70 | 0x0127 | ToggleHighProcess | THIGHPROCESS | 0 | `0x5013F0` | `0x4FDE30` | - | - |
| 71 | 0x0128 | ToggleLowProcess | TLOWPROCESS | 0 | `0x501440` | `0x4FDE30` | - | - |
| 72 | 0x0129 | ToggleMiddleLowProcess | TMLOWPROCESS | 0 | `0x501480` | `0x4FDE30` | - | - |
| 73 | 0x012A | ToggleMiddleHighProcess | TMHIGHPROCESS | 0 | `0x5014C0` | `0x4FDE30` | - | - |
| 74 | 0x012B | ToggleAiSchedules | TAIS | 0 | `0x501500` | `0x4FDE30` | - | - |
| 75 | 0x012C | SpeakSound | - | 3 | `0x513C90` | `0x4FDE30` | - | - |
| 76 | 0x012D | BetaComment | BC | 1 | `0x50A400` | `0x4FDE30` | - | yes |
| 77 | 0x012E | ToggleCellNode | TCN | 1 | `0x50A8A0` | `0x4FDE30` | - | yes |
| 78 | 0x012F | ToggleTrees | TT | 0 | `0x50A9A0` | `0x4FDE30` | - | yes |
| 79 | 0x0130 | SetCameraFOV | FOV | 1 | `0x511720` | `0x4FDE30` | - | yes |
| 80 | 0x0131 | Verbose | VERBOSE | 0 | `0x507930` | `0x4FDE30` | - | yes |
| 81 | 0x0132 | ToggleGodMode | TGM | 0 | `0x500D10` | `0x4FDE30` | - | yes |
| 82 | 0x0133 | RefreshShaders | REFRESHSHADERS | 0 | `0x50AA20` | `0x4FDE30` | - | yes |
| 83 | 0x0134 | ToggleScripts | TSCR | 0 | `0x500D50` | `0x4FDE30` | - | yes |
| 84 | 0x0135 | SexChange | - | 0 | `0x515920` | `0x4FDE30` | - | yes |
| 85 | 0x0136 | ToggleConversations | TCONV | 0 | `0x507BA0` | `0x4FDE30` | - | yes |
| 86 | 0x0137 | ToggleFullHelp | TFH | 0 | `0x500E00` | `0x4FDE30` | - | yes |
| 87 | 0x0138 | ShowQuestLog | SQL | 1 | `0x5120A0` | `0x4FDE30` | - | yes |
| 88 | 0x0139 | ShowFullQuestLog | SFQL | 1 | `0x512220` | `0x4FDE30` | - | yes |
| 89 | 0x013A | DumpTexturePalette | DTP | 1 | `0x515330` | `0x4FDE30` | - | yes |
| 90 | 0x013B | CenterOnWorld | COW | 3 | `0x5085D0` | `0x4FDE30` | - | yes |
| 91 | 0x013C | ToggleGrass | TG | 0 | `0x50EC90` | `0x4FDE30` | - | yes |
| 92 | 0x013D | AddFaceAnimNote | AFAN | 1 | `0x505120` | `0x4FDE30` | - | - |
| 93 | 0x013E | RefreshINI | REFINI | 0 | `0x513BB0` | `0x4FDE30` | - | yes |
| 94 | 0x013F | ToggleEmotions | temo | 0 | `0x507A00` | `0x4FDE30` | - | yes |
| 95 | 0x0140 | AddDecal | - | 0 | `0x510580` | `0x4FDE30` | - | - |
| 96 | 0x0141 | TestCode | - | 0 | `0x977C50` | `0x4FDE30` | - | - alias:0x0173,0x10DC,0x110C,0x114A,0x115F,0x1160 |
| 97 | 0x0142 | ToggleMapMarkers | tmm | 1 | `0x507800` | `0x4FDE30` | - | yes |
| 98 | 0x0143 | PlayerSpellBook | psb | 0 | `0x507740` | `0x4FDE30` | - | yes |
| 99 | 0x0144 | PrintAiList | pai | 0 | `0x5007A0` | `0x4FDE30` | - | yes |
| 100 | 0x0145 | ShowQuestTargets | SQT | 0 | `0x515AF0` | `0x4FDE30` | - | yes |
| 101 | 0x0146 | HairTint | - | 3 | `0x508B80` | `0x4FDE30` | - | yes |
| 102 | 0x0147 | SaveGame | save | 2 | `0x5023D0` | `0x4FDE30` | - | yes |
| 103 | 0x0148 | LoadGame | load | 2 | `0x502500` | `0x4FDE30` | - | yes |
| 104 | 0x0149 | TestAllCells | TAC | 1 | `0x507960` | `0x4FDE30` | - | yes |
| 105 | 0x014A | SaveWorld | - | 1 | `0x500E60` | `0x4FDE30` | - | yes |
| 106 | 0x014B | ReloadCurrentClimate | rcc | 0 | `0x507A30` | `0x4FDE30` | - | yes |
| 107 | 0x014C | ReloadCurrentWeather | rcw | 0 | `0x507A90` | `0x4FDE30` | - | yes |
| 108 | 0x014D | TestSeenData | tsd | 0 | `0x50ED50` | `0x4FDE30` | - | yes |
| 109 | 0x014E | TestLocalMap | tlm | 1 | `0x50EF90` | `0x4FDE30` | - | yes |
| 110 | 0x014F | MoveToQuestTarget | movetoqt | 1 | `0x512910` | `0x4FDE30` | - | yes |
| 111 | 0x0150 | PrintNPCDialog | pdialog | 0 | `0x5007C0` | `0x4FDE30` | - | yes |
| 112 | 0x0151 | ShowSubtitle | - | 1 | `0x5007D0` | `0x4FDE30` | - | yes alias:0x1164 |
| 113 | 0x0152 | ShowRenderPasses | srp | 0 | `0x500820` | `0x4FDE30` | - | yes |
| 114 | 0x0153 | FreezeRenderAccumulation | fra | 0 | `0x500830` | `0x4FDE30` | - | yes |
| 115 | 0x0154 | ToggleOcclusion | tocc | 0 | `0x500860` | `0x4FDE30` | - | yes |
| 116 | 0x0155 | OutputMemContexts | omc | 1 | `0x500890` | `0x4FDE30` | - | yes alias:0x0169 |
| 117 | 0x0156 | OutputMemStats | oms | 1 | `0x5009E0` | `0x4FDE30` | - | yes |
| 118 | 0x0157 | ToggleDetectionStats | tds | 1 | `0x500F70` | `0x4FDE30` | - | yes |
| 119 | 0x0158 | SetTargetRefraction | str | 1 | `0x507E60` | `0x4FDE30` | - | yes |
| 120 | 0x0159 | SetTargetRefractionFire | strf | 2 | `0x507FF0` | `0x4FDE30` | - | yes |
| 121 | 0x015A | ToggleRefractionDebug | trd | 0 | `0x508180` | `0x4FDE30` | - | yes |
| 122 | 0x015B | ToggleCharControllerShape | TCCS | 0 | `0x507C00` | `0x4FDE30` | - | yes |
| 123 | 0x015C | ShowHeadTrackTarget | SHeadT | 0 | `0x508340` | `0x4FDE30` | - | yes |
| 124 | 0x015D | SetSkyParam | SSP | 2 | `0x503C60` | `0x4FDE30` | - | yes |
| 125 | 0x015E | SetHDRParam | SHP | 6 | `0x50B0C0` | `0x4FDE30` | - | yes |
| 126 | 0x015F | ShowPivot | SP | 0 | `0x511C10` | `0x4FDE30` | - | yes |
| 127 | 0x0160 | PrintHDRParam | PHP | 0 | `0x50B210` | `0x4FDE30` | - | yes |
| 128 | 0x0161 | ToggleHDRDebug | THD | 0 | `0x503CE0` | `0x4FDE30` | - | yes |
| 129 | 0x0162 | RevertWorld | rw | 0 | `0x500C90` | `0x4FDE30` | - | yes |
| 130 | 0x0163 | OutputArchiveProfile | oap | 1 | `0x500AE0` | `0x4FDE30` | - | yes |
| 131 | 0x0164 | QuitGame | qqq | 0 | `0x5077E0` | `0x4FDE30` | - | yes |
| 132 | 0x0165 | SaveIniFiles | saveini | 0 | `0x513B60` | `0x4FDE30` | - | yes |
| 133 | 0x0166 | SetDebugText | sdt | 1 | `0x5081F0` | `0x4FDE30` | - | yes |
| 134 | 0x0167 | ToggleLiteBrite | tlb | 0 | `0x5081B0` | `0x4FDE30` | - | yes |
| 135 | 0x0168 | RunMemoryPass | rmp | 1 | `0x501040` | `0x4FDE30` | - | yes |
| 136 | 0x0169 | ResetMemContexts | rmc | 0 | `0x500890` | `0x4FDE30` | - | yes alias:0x0155 |
| 137 | 0x016A | ToggleWaterSystem | TWS | 0 | `0x50D860` | `0x4FDE30` | - | yes |
| 138 | 0x016B | ToggleWaterRadius | TWR | 0 | `0x50D8C0` | `0x4FDE30` | - | - |
| 139 | 0x016C | ShowWhoDetectsPlayer | SWDP | 0 | `0x500780` | `0x4FDE30` | - | yes |
| 140 | 0x016D | ToggleCombatAI | tcai | 0 | `0x508300` | `0x4FDE30` | - | yes |
| 141 | 0x016E | ToggleFlyCam | tfc | 0 | `0x5013E0` | `0x4FDE30` | - | yes |
| 142 | 0x016F | ModWaterShader | mws | 2 | `0x50D8E0` | `0x4FDE30` | - | yes |
| 143 | 0x0170 | WaterShallowColor | shallow | 3 | `0x50E250` | `0x4FDE30` | - | yes |
| 144 | 0x0171 | WaterDeepColor | deep | 3 | `0x5065B0` | `0x4FDE30` | - | yes |
| 145 | 0x0172 | WaterReflectionColor | refl | 3 | `0x506680` | `0x4FDE30` | - | yes |
| 146 | 0x0173 | BeginTrace | bt | 0 | `0x977C50` | `0x4FDE30` | - | yes alias:0x0141,0x10DC,0x110C,0x114A,0x115F,0x1160 |
| 147 | 0x0174 | RunCellTest | rct | 1 | `0x5068C0` | `0x4FDE30` | - | yes |
| 148 | 0x0175 | StartAllQuests | saq | 0 | `0x50E5A0` | `0x4FDE30` | - | yes |
| 149 | 0x0176 | CompleteAllQuestStages | caqs | 0 | `0x50E600` | `0x4FDE30` | - | yes |
| 150 | 0x0177 | FlushNonPersistActors | Flush | 1 | `0x506920` | `0x4FDE30` | - | yes |
| 151 | 0x0178 | ToggleFogOfWar | TFOW | 0 | `0x506990` | `0x4FDE30` | - | yes |
| 152 | 0x0179 | OutputLocalMapPictures | OLMP | 0 | `0x50E6A0` | `0x4FDE30` | - | yes |
| 153 | 0x017A | SetGamma | sg | 1 | `0x50E950` | `0x4FDE30` | - | yes |
| 154 | 0x017B | WasteMemory | wm | 1 | `0x5069D0` | `0x4FDE30` | - | yes |
| 155 | 0x017C | ClearAdaptedLight | cal | 0 | `0x506AD0` | `0x4FDE30` | - | yes |
| 156 | 0x017D | ToggleCastShadows | tsh | 1 | `0x506B80` | `0x4FDE30` | - | - |
| 157 | 0x017E | ShowSubSpaces | sss | 0 | `0x506C80` | `0x4FDE30` | - | yes |
| 158 | 0x017F | SetSTBBColorConstants | sscc | 4 | `0x508980` | `0x4FDE30` | - | yes |
| 159 | 0x0180 | CloseFile | - | 1 | `0x508A20` | `0x4FDE30` | - | - |
| 160 | 0x0181 | SetTreeMipmapBias | stmb | 2 | `0x514500` | `0x4FDE30` | - | yes |
| 161 | 0x0182 | ClearFaceGenModelData | CFG | 0 | `0x506E90` | `0x4FDE30` | - | yes |
| 162 | 0x0183 | ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!! | - | 0 | - | - | - | - |
| 163 | 0x1000 | MessageBox | - | 1 | `0x513280` | `0x500130` | - | - |
| 164 | 0x1001 | GetDistance | - | 1 | `0x5003F0` | `0x4FDE30` | `0x4F43C0` | - |
| 165 | 0x1002 | AddItem | - | 2 | `0x507320` | `0x4FDE30` | - | - |
| 166 | 0x1003 | SetEssential | - | 2 | `0x50B780` | `0x4FDE30` | - | - |
| 167 | 0x1004 | Rotate | - | 2 | `0x510620` | `0x4FDE30` | - | - |
| 168 | 0x1005 | GetLocked | - | 0 | `0x502390` | `0x4FDE30` | `0x4F4760` | - |
| 169 | 0x1006 | GetPos | - | 1 | `0x501A80` | `0x4FDE30` | `0x4F4410` | - |
| 170 | 0x1007 | SetPos | - | 2 | `0x508FC0` | `0x4FDE30` | - | - |
| 171 | 0x1008 | GetAngle | - | 1 | `0x501B40` | `0x4FDE30` | `0x4F8890` | - |
| 172 | 0x1009 | SetAngle | - | 2 | `0x509110` | `0x4FDE30` | - | - |
| 173 | 0x100A | GetStartingPos | - | 1 | `0x501AE0` | `0x4FDE30` | `0x4F4490` | - |
| 174 | 0x100B | GetStartingAngle | - | 1 | `0x501BA0` | `0x4FDE30` | `0x4F4500` | - |
| 175 | 0x100C | GetSecondsPassed | - | 0 | `0x501C20` | `0x4FDE30` | `0x4F6010` | - |
| 176 | 0x100D | Activate | - | 2 | `0x507650` | `0x4FDE30` | - | - |
| 177 | 0x100E | GetActorValue | GetAV | 1 | `0x501670` | `0x4FDE30` | `0x4F6060` | yes |
| 178 | 0x100F | SetActorValue | SetAV | 2 | `0x5016D0` | `0x4FDE30` | - | yes |
| 179 | 0x1010 | ModActorValue | ModAV | 2 | `0x5017E0` | `0x4FDE30` | - | yes |
| 180 | 0x1011 | SetAtStart | - | 0 | `0x509220` | `0x4FDE30` | - | - |
| 181 | 0x1012 | GetCurrentTime | - | 0 | `0x501C00` | `0x4FDE30` | `0x4F46E0` | - |
| 182 | 0x1013 | PlayGroup | - | 2 | `0x5107A0` | `0x4FDE30` | - | yes |
| 183 | 0x1014 | LoopGroup | - | 3 | `0x501C40` | `0x4FDE30` | - | - |
| 184 | 0x1015 | SkipAnim | - | 0 | `0x501CF0` | `0x4FDE30` | - | - |
| 185 | 0x1016 | StartCombat | - | 1 | `0x514660` | `0x4FDE30` | - | - |
| 186 | 0x1017 | StopCombat | - | 0 | `0x501D30` | `0x4FDE30` | - | - |
| 187 | 0x1018 | GetScale | - | 0 | `0x501E20` | `0x4FDE30` | `0x4F4720` | - |
| 188 | 0x1019 | UnusedFunction1 | - | 0 | - | `0x4FDE30` | - | - |
| 189 | 0x101A | UnusedFunction2 | - | 0 | - | `0x4FDE30` | - | - |
| 190 | 0x101B | GetLineOfSight | GetLOS | 1 | `0x501EF0` | `0x4FDE30` | `0x4F9120` | - |
| 191 | 0x101C | AddSpell | - | 1 | `0x514950` | `0x4FDE30` | - | - |
| 192 | 0x101D | RemoveSpell | - | 1 | `0x510B90` | `0x4FDE30` | - | - |
| 193 | 0x101E | Cast | - | 2 | `0x501F50` | `0x4FDE30` | - | - |
| 194 | 0x101F | GetButtonPressed | - | 0 | `0x5072B0` | `0x4FDE30` | - | - |
| 195 | 0x1020 | GetInSameCell | - | 1 | `0x503210` | `0x4FDE30` | `0x4F6CF0` | - |
| 196 | 0x1021 | Enable | - | 0 | `0x50A150` | `0x4FDE30` | - | - |
| 197 | 0x1022 | Disable | - | 0 | `0x50A240` | `0x4FDE30` | - | - |
| 198 | 0x1023 | GetDisabled | - | 0 | `0x502370` | `0x4FDE30` | `0x4F60E0` | - |
| 199 | 0x1024 | MenuMode | - | 1 | `0x502270` | `0x4FDE30` | `0x4F4570` | - alias:0x0001 |
| 200 | 0x1025 | PlaceAtMe | - | 4 | `0x514B50` | `0x4FDE30` | - | - |
| 201 | 0x1026 | PlaySound | - | 1 | `0x509490` | `0x4FDE30` | - | - |
| 202 | 0x1027 | GetDisease | - | 0 | `0x502630` | `0x4FDE30` | `0x4F4800` | - |
| 203 | 0x1028 | GetVampire | - | 0 | `0x502650` | `0x4FDE30` | `0x4F4830` | - |
| 204 | 0x1029 | GetClothingValue | - | 0 | `0x502670` | `0x4FDE30` | `0x4F6150` | - |
| 205 | 0x102A | SameFaction | - | 1 | `0x502690` | `0x4FDE30` | `0x4F61A0` | - |
| 206 | 0x102B | SameRace | - | 1 | `0x5026F0` | `0x4FDE30` | `0x4F62A0` | - |
| 207 | 0x102C | SameSex | - | 1 | `0x502750` | `0x4FDE30` | `0x4F6350` | - |
| 208 | 0x102D | GetDetected | - | 1 | `0x502810` | `0x4FDE30` | `0x4F6400` | - |
| 209 | 0x102E | GetDead | - | 0 | `0x502870` | `0x4FDE30` | `0x4F4890` | - |
| 210 | 0x102F | GetItemCount | - | 1 | `0x502890` | `0x4FDE30` | `0x4F48F0` | - |
| 211 | 0x1030 | GetGold | - | 0 | `0x5029C0` | `0x4FDE30` | `0x4F4A00` | - |
| 212 | 0x1031 | GetSleeping | - | 0 | `0x5029E0` | `0x4FDE30` | `0x4F4A80` | - |
| 213 | 0x1032 | GetTalkedToPC | - | 0 | `0x502B00` | `0x4FDE30` | `0x4F4B10` | - |
| 214 | 0x1033 | Say | - | 5 | `0x5118D0` | `0x4FDE30` | - | - |
| 215 | 0x1034 | SayTo | - | 3 | `0x511AB0` | `0x4FDE30` | - | - |
| 216 | 0x1035 | GetScriptVariable | - | 2 | - | `0x4FDE30` | `0x4F6710` | - |
| 217 | 0x1036 | StartQuest | - | 1 | `0x502D30` | `0x4FDE30` | - | - |
| 218 | 0x1037 | StopQuest | - | 1 | `0x502D80` | `0x4FDE30` | - | - |
| 219 | 0x1038 | GetQuestRunning | GetQR | 1 | `0x502B80` | `0x4FDE30` | `0x4F67A0` | - |
| 220 | 0x1039 | SetStage | - | 2 | `0x502CB0` | `0x4FDE30` | - | - |
| 221 | 0x103A | GetStage | - | 1 | `0x502BE0` | `0x4FDE30` | `0x4F4B70` | - |
| 222 | 0x103B | GetStageDone | - | 2 | `0x502C40` | `0x4FDE30` | `0x4F4BC0` | - |
| 223 | 0x103C | GetFactionRankDifference | - | 2 | `0x502E20` | `0x4FDE30` | `0x4F67E0` | - |
| 224 | 0x103D | GetAlarmed | - | 0 | `0x502E90` | `0x4FDE30` | `0x4F4C10` | - |
| 225 | 0x103E | IsRaining | - | 0 | `0x502EF0` | `0x4FDE30` | `0x4F69D0` | - |
| 226 | 0x103F | GetAttacked | - | 0 | `0x502FD0` | `0x4FDE30` | `0x4F4C80` | - |
| 227 | 0x1040 | GetIsCreature | - | 0 | `0x502FF0` | `0x4FDE30` | `0x4F6B00` | - |
| 228 | 0x1041 | GetLockLevel | - | 0 | `0x5023B0` | `0x4FDE30` | `0x4F47B0` | - |
| 229 | 0x1042 | GetShouldAttack | - | 1 | `0x503010` | `0x4FDE30` | `0x4F6B50` | - |
| 230 | 0x1043 | GetInCell | - | 1 | `0x503070` | `0x4FDE30` | `0x4F6D70` | - |
| 231 | 0x1044 | GetIsClass | - | 1 | `0x503270` | `0x4FDE30` | `0x4F6EC0` | - |
| 232 | 0x1045 | GetIsRace | - | 1 | `0x503330` | `0x4FDE30` | `0x4F6F40` | - |
| 233 | 0x1046 | GetIsSex | - | 1 | `0x503390` | `0x4FDE30` | `0x4F6FC0` | - |
| 234 | 0x1047 | GetInFaction | - | 1 | `0x503410` | `0x4FDE30` | `0x4F70C0` | - |
| 235 | 0x1048 | GetIsID | - | 1 | `0x5035F0` | `0x4FDE30` | `0x4F4CE0` | - |
| 236 | 0x1049 | GetFactionRank | - | 1 | `0x5037B0` | `0x4FDE30` | `0x4F7280` | - |
| 237 | 0x104A | GetGlobalValue | - | 1 | `0x503930` | `0x4FDE30` | `0x4F7330` | - |
| 238 | 0x104B | IsSnowing | - | 0 | `0x502F10` | `0x4FDE30` | `0x4F6A20` | - |
| 239 | 0x104C | GetDisposition | - | 1 | `0x503990` | `0x4FDE30` | `0x4F4E20` | - |
| 240 | 0x104D | GetRandomPercent | - | 0 | `0x5039F0` | `0x4FDE30` | `0x4F4EF0` | - |
| 241 | 0x104E | StreamMusic | - | 1 | `0x509610` | `0x4FDE30` | - | - |
| 242 | 0x104F | GetQuestVariable | - | 2 | - | `0x4FDE30` | `0x4F6710` | - |
| 243 | 0x1050 | GetLevel | - | 0 | `0x503A10` | `0x4FDE30` | `0x4F4F50` | - |
| 244 | 0x1051 | GetArmorRating | - | 0 | `0x503A30` | `0x4FDE30` | `0x4F4FB0` | - |
| 245 | 0x1052 | RemoveItem | - | 2 | `0x513810` | `0x4FDE30` | - | - |
| 246 | 0x1053 | ModDisposition | - | 2 | `0x503A50` | `0x4FDE30` | - | - |
| 247 | 0x1054 | GetDeadCount | - | 1 | `0x503B00` | `0x4FDE30` | `0x4F5010` | - |
| 248 | 0x1055 | ShowMap | - | 2 | `0x50AD00` | `0x4FDE30` | - | - |
| 249 | 0x1056 | StartConversation | - | 2 | `0x50AE10` | `0x4FDE30` | - | yes |
| 250 | 0x1057 | Drop | - | 2 | `0x500650` | `0x4FDE30` | - | - |
| 251 | 0x1058 | AddTopic | - | 1 | `0x503B60` | `0x4FDE30` | - | - |
| 252 | 0x1059 | Message | - | 1 | `0x513600` | `0x500150` | - | - |
| 253 | 0x105A | SetAlert | - | 1 | `0x503BC0` | `0x4FDE30` | - | - |
| 254 | 0x105B | GetIsAlerted | - | 0 | `0x503C40` | `0x4FDE30` | `0x4F5060` | - |
| 255 | 0x105C | Look | - | 1 | `0x50AFD0` | `0x4FDE30` | - | - |
| 256 | 0x105D | StopLook | - | 0 | `0x50B070` | `0x4FDE30` | - | - |
| 257 | 0x105E | EvaluatePackage | evp | 0 | `0x50AF90` | `0x4FDE30` | - | - |
| 258 | 0x105F | Yield | - | 0 | `0x501DD0` | `0x4FDE30` | - | - |
| 259 | 0x1060 | EnablePlayerControls | - | 0 | `0x5047E0` | `0x4FDE30` | - | - |
| 260 | 0x1061 | DisablePlayerControls | - | 0 | `0x5047F0` | `0x4FDE30` | - | - |
| 261 | 0x1062 | GetPlayerControlsDisabled | - | 0 | `0x504800` | `0x4FDE30` | `0x4F7470` | - |
| 262 | 0x1063 | GetHeadingAngle | - | 1 | `0x504820` | `0x4FDE30` | `0x4F74B0` | - |
| 263 | 0x1064 | PickIdle | - | 0 | `0x511FA0` | `0x4FDE30` | - | - |
| 264 | 0x1065 | IsWeaponOut | - | 0 | `0x504890` | `0x4FDE30` | `0x4F50C0` | - |
| 265 | 0x1066 | IsTorchOut | - | 0 | `0x5048B0` | `0x4FDE30` | `0x4F75F0` | - |
| 266 | 0x1067 | IsShieldOut | - | 0 | `0x5048D0` | `0x4FDE30` | `0x4F7650` | - |
| 267 | 0x1068 | IsYielding | - | 0 | `0x5048F0` | `0x4FDE30` | `0x4F5120` | - |
| 268 | 0x1069 | IsActionRef | - | 1 | `0x5006E0` | `0x4FDE30` | - | - |
| 269 | 0x106A | IsFacingUp | - | 0 | `0x504910` | `0x4FDE30` | `0x4F8970` | - |
| 270 | 0x106B | GetKnockedState | - | 0 | `0x504950` | `0x4FDE30` | `0x4F76B0` | - |
| 271 | 0x106C | GetWeaponAnimType | - | 0 | `0x504970` | `0x4FDE30` | `0x4F7750` | - |
| 272 | 0x106D | GetWeaponSkillType | - | 0 | `0x504990` | `0x4FDE30` | `0x4F7800` | - |
| 273 | 0x106E | GetCurrentAIPackage | - | 0 | `0x5049B0` | `0x4FDE30` | `0x4F78B0` | - |
| 274 | 0x106F | IsWaiting | - | 0 | `0x5049D0` | `0x4FDE30` | `0x4F5180` | - |
| 275 | 0x1070 | IsIdlePlaying | - | 0 | `0x5049F0` | `0x4FDE30` | `0x4F51E0` | - |
| 276 | 0x1071 | CompleteQuest | - | 1 | `0x502DD0` | `0x4FDE30` | - | - |
| 277 | 0x1072 | Lock | - | 2 | `0x504A10` | `0x4FDE30` | - | - |
| 278 | 0x1073 | UnLock | - | 1 | `0x504B00` | `0x4FDE30` | - | - |
| 279 | 0x1074 | GetCrimeGold | - | 0 | `0x504CE0` | `0x4FDE30` | `0x4F5230` | - |
| 280 | 0x1075 | SetCrimeGold | - | 1 | `0x50B4A0` | `0x4FDE30` | - | - |
| 281 | 0x1076 | ModCrimeGold | - | 1 | `0x504D00` | `0x4FDE30` | - | - |
| 282 | 0x1077 | GetCrimeKnown | - | 3 | `0x504BB0` | `0x4FDE30` | - | - |
| 283 | 0x1078 | PositionWorld | PosWorld | 5 | `0x508C30` | `0x4FDE30` | - | - |
| 284 | 0x1079 | PositionCell | PosCell | 5 | `0x508E20` | `0x4FDE30` | - | - |
| 285 | 0x107A | GetCrime | - | 2 | `0x504C60` | `0x4FDE30` | `0x4F7AF0` | - |
| 286 | 0x107B | PayFine | - | 0 | `0x50B590` | `0x4FDE30` | - | - |
| 287 | 0x107C | PayFineThief | - | 0 | `0x504DB0` | `0x4FDE30` | - | - |
| 288 | 0x107D | IsGuard | - | 0 | `0x504E10` | `0x4FDE30` | `0x4F7B90` | - |
| 289 | 0x107E | GoToJail | - | 0 | `0x50B620` | `0x4FDE30` | - | - |
| 290 | 0x107F | CanPayCrimeGold | - | 0 | `0x504E30` | `0x4FDE30` | `0x4F5290` | - |
| 291 | 0x1080 | GetFatiguePercentage | GetFatigue | 0 | `0x5019E0` | `0x4FDE30` | `0x4F4640` | - |
| 292 | 0x1081 | GetPCIsClass | - | 1 | `0x503470` | `0x4FDE30` | `0x4F7190` | - |
| 293 | 0x1082 | GetPCIsRace | - | 1 | `0x5034D0` | `0x4FDE30` | `0x4F71A0` | - |
| 294 | 0x1083 | GetPCIsSex | - | 1 | `0x503530` | `0x4FDE30` | `0x4F71B0` | - |
| 295 | 0x1084 | GetPCInFaction | - | 1 | `0x503590` | `0x4FDE30` | `0x4F71C0` | - |
| 296 | 0x1085 | SameFactionAsPC | - | 0 | `0x5027B0` | `0x4FDE30` | `0x4F8910` | - |
| 297 | 0x1086 | SameRaceAsPC | - | 0 | `0x5027D0` | `0x4FDE30` | `0x4F8930` | - |
| 298 | 0x1087 | SameSexAsPC | - | 0 | `0x5027F0` | `0x4FDE30` | `0x4F8950` | - |
| 299 | 0x1088 | GetIsReference | - | 1 | `0x503750` | `0x4FDE30` | `0x4F7220` | - |
| 300 | 0x1089 | SetFactionRank | - | 2 | `0x50AB80` | `0x4FDE30` | - | - |
| 301 | 0x108A | ModFactionRank | - | 2 | `0x50AC40` | `0x4FDE30` | - | - |
| 302 | 0x108B | KillActor | kill | 1 | `0x501960` | `0x4FDE30` | - | - |
| 303 | 0x108C | ResurrectActor | resurrect | 1 | `0x510150` | `0x4FDE30` | - | - |
| 304 | 0x108D | IsTalking | - | 0 | `0x504E50` | `0x4FDE30` | `0x4F5310` | - |
| 305 | 0x108E | GetWalkSpeed | GetWalk | 0 | `0x504E70` | `0x4FDE30` | `0x4F4690` | - |
| 306 | 0x108F | GetCurrentAIProcedure | - | 0 | `0x504E90` | `0x4FDE30` | `0x4F7C00` | - |
| 307 | 0x1090 | GetTrespassWarningLevel | - | 0 | `0x504EB0` | `0x4FDE30` | `0x4F7EF0` | - |
| 308 | 0x1091 | IsTrespassing | - | 0 | `0x504ED0` | `0x4FDE30` | `0x4F5370` | - |
| 309 | 0x1092 | IsInMyOwnedCell | - | 0 | `0x504EF0` | `0x4FDE30` | `0x4F53D0` | - |
| 310 | 0x1093 | GetWindSpeed | - | 0 | `0x502F30` | `0x4FDE30` | `0x4F7F50` | - |
| 311 | 0x1094 | GetCurrentWeatherPercent | getweatherpct | 0 | `0x502F50` | `0x4FDE30` | `0x4F6A70` | - |
| 312 | 0x1095 | GetIsCurrentWeather | getweather | 1 | `0x502F70` | `0x4FDE30` | `0x4F6AB0` | - |
| 313 | 0x1096 | IsContinuingPackagePCNear | - | 0 | `0x504F10` | `0x4FDE30` | `0x4F5440` | - |
| 314 | 0x1097 | AddScriptPackage | - | 1 | `0x5123A0` | `0x4FDE30` | - | - |
| 315 | 0x1098 | RemoveScriptPackage | - | 0 | `0x50B6A0` | `0x4FDE30` | - | - |
| 316 | 0x1099 | CanHaveFlames | - | 0 | `0x504F30` | `0x4FDE30` | `0x4F8A50` | - |
| 317 | 0x109A | HasFlames | - | 0 | `0x504F50` | `0x4FDE30` | `0x4F54A0` | - |
| 318 | 0x109B | AddFlames | - | 0 | `0x504F70` | `0x4FDE30` | - | - |
| 319 | 0x109C | RemoveFlames | - | 0 | `0x505020` | `0x4FDE30` | - | - |
| 320 | 0x109D | GetOpenState | - | 0 | `0x505090` | `0x4FDE30` | `0x4F5520` | - |
| 321 | 0x109E | MoveToMarker | MoveTo | 4 | `0x512870` | `0x4FDE30` | - | - |
| 322 | 0x109F | GetSitting | - | 0 | `0x502A00` | `0x4FDE30` | `0x4F64E0` | - |
| 323 | 0x10A0 | GetFurnitureMarkerID | - | 0 | `0x502A20` | `0x4FDE30` | `0x4F65B0` | - |
| 324 | 0x10A1 | GetIsCurrentPackage | - | 1 | `0x5050B0` | `0x4FDE30` | `0x4F7FA0` | - |
| 325 | 0x10A2 | IsCurrentFurnitureRef | - | 1 | `0x502A40` | `0x4FDE30` | `0x4F6640` | - |
| 326 | 0x10A3 | IsCurrentFurnitureObj | - | 1 | `0x502AA0` | `0x4FDE30` | `0x4F66A0` | - |
| 327 | 0x10A4 | SetSize | CSize | 1 | `0x501760` | `0x4FDE30` | - | yes |
| 328 | 0x10A5 | RemoveMe | - | 1 | `0x500450` | `0x4FDE30` | - | - |
| 329 | 0x10A6 | DropMe | - | 0 | `0x5005F0` | `0x4FDE30` | - | - |
| 330 | 0x10A7 | GetFactionReaction | - | 2 | `0x511800` | `0x4FDE30` | - | - |
| 331 | 0x10A8 | SetFactionReaction | - | 3 | `0x503810` | `0x4FDE30` | - | - |
| 332 | 0x10A9 | ModFactionReaction | - | 3 | `0x5038A0` | `0x4FDE30` | - | - |
| 333 | 0x10AA | GetDayOfWeek | - | 0 | `0x5051D0` | `0x4FDE30` | `0x4F5570` | - |
| 334 | 0x10AB | IsPlayerInJail | - | 0 | `0x5051F0` | `0x4FDE30` | `0x4F55C0` | - |
| 335 | 0x10AC | GetTalkedToPCParam | - | 1 | `0x502B20` | `0x4FDE30` | `0x4F4B10` | - |
| 336 | 0x10AD | RemoveAllItems | - | 2 | `0x5074F0` | `0x4FDE30` | - | - |
| 337 | 0x10AE | WakeUpPC | - | 1 | `0x50B700` | `0x4FDE30` | - | - |
| 338 | 0x10AF | IsPCSleeping | - | 0 | `0x505210` | `0x4FDE30` | `0x4F8050` | - |
| 339 | 0x10B0 | IsPCAMurderer | - | 0 | `0x505230` | `0x4FDE30` | `0x4F5600` | - |
| 340 | 0x10B1 | SetCombatStyle | setcs | 1 | `0x505250` | `0x4FDE30` | - | - |
| 341 | 0x10B2 | PlaySound3D | - | 1 | `0x509520` | `0x4FDE30` | - | - |
| 342 | 0x10B3 | SelectPlayerSpell | spspell | 1 | `0x5129E0` | `0x4FDE30` | - | - |
| 343 | 0x10B4 | GetDetectionLevel | - | 1 | `0x505350` | `0x4FDE30` | `0x4F5650` | - |
| 344 | 0x10B5 | IsActorDetected | - | 0 | `0x5053D0` | `0x4FDE30` | - | - |
| 345 | 0x10B6 | GetEquipped | - | 1 | `0x502960` | `0x4FDE30` | `0x4F4980` | - |
| 346 | 0x10B7 | Wait | - | 1 | `0x50B810` | `0x4FDE30` | - | - |
| 347 | 0x10B8 | StopWaiting | - | 1 | `0x50B920` | `0x4FDE30` | - | - |
| 348 | 0x10B9 | IsSwimming | - | 0 | `0x5053B0` | `0x4FDE30` | `0x4F56D0` | - |
| 349 | 0x10BA | ScriptEffectElapsedSeconds | - | 0 | `0x502350` | `0x4FDE30` | - | - |
| 350 | 0x10BB | SetCellPublicFlag | setpublic | 2 | `0x5031A0` | `0x4FDE30` | - | yes |
| 351 | 0x10BC | GetPCSleepHours | - | 0 | `0x50BAA0` | `0x4FDE30` | - | - |
| 352 | 0x10BD | SetPCSleepHours | - | 1 | `0x50BAC0` | `0x4FDE30` | - | - |
| 353 | 0x10BE | GetAmountSoldStolen | - | 0 | `0x505460` | `0x4FDE30` | `0x4F80A0` | - |
| 354 | 0x10BF | ModAmountSoldStolen | - | 1 | `0x50BB20` | `0x4FDE30` | - | - |
| 355 | 0x10C0 | CloseCurrentOblivionGate | - | 1 | `0x515D20` | `0x4FDE30` | - | - |
| 356 | 0x10C1 | GetPCExpelled | - | 1 | `0x505480` | `0x4FDE30` | `0x4F80E0` | - |
| 357 | 0x10C2 | SetPCExpelled | - | 2 | `0x50BB70` | `0x4FDE30` | - | - |
| 358 | 0x10C3 | GetPCFactionMurder | - | 1 | `0x5054E0` | `0x4FDE30` | `0x4F8140` | - |
| 359 | 0x10C4 | SetPCFactionMurder | - | 2 | `0x50BD80` | `0x4FDE30` | - | - |
| 360 | 0x10C5 | GetPCFactionSteal | - | 1 | `0x505540` | `0x4FDE30` | `0x4F81A0` | - |
| 361 | 0x10C6 | SetPCFactionSteal | - | 2 | `0x50BE00` | `0x4FDE30` | - | - |
| 362 | 0x10C7 | GetPCFactionAttack | - | 1 | `0x5055A0` | `0x4FDE30` | `0x4F8200` | - |
| 363 | 0x10C8 | SetPCFactionAttack | - | 2 | `0x50BE80` | `0x4FDE30` | - | - |
| 364 | 0x10C9 | GetPCFactionSubmitAuthority | - | 1 | `0x505600` | `0x4FDE30` | `0x4F8260` | - |
| 365 | 0x10CA | SetPCFactionSubmitAuthority | - | 2 | `0x50BF00` | `0x4FDE30` | - | - |
| 366 | 0x10CB | GetDestroyed | - | 0 | `0x505660` | `0x4FDE30` | `0x4F82C0` | - |
| 367 | 0x10CC | SetDestroyed | - | 1 | `0x505680` | `0x4FDE30` | - | - |
| 368 | 0x10CD | GetActionRef | getAR | 0 | `0x50BF80` | `0x4FDE30` | - | - |
| 369 | 0x10CE | GetSelf | this | 0 | `0x50BFF0` | `0x4FDE30` | - | - |
| 370 | 0x10CF | GetContainer | - | 0 | `0x50C160` | `0x4FDE30` | - | - |
| 371 | 0x10D0 | GetForceRun | - | 0 | `0x50C230` | `0x4FDE30` | - | - |
| 372 | 0x10D1 | SetForceRun | - | 1 | `0x50C290` | `0x4FDE30` | - | - |
| 373 | 0x10D2 | GetForceSneak | - | 0 | `0x50C330` | `0x4FDE30` | - | - |
| 374 | 0x10D3 | SetForceSneak | - | 1 | `0x50C390` | `0x4FDE30` | - | - |
| 375 | 0x10D4 | AdvancePCSkill | AdvSkill | 2 | `0x50D020` | `0x4FDE30` | - | - alias:0x110F |
| 376 | 0x10D5 | AdvancePCLevel | AdvLevel | 0 | `0x5056F0` | `0x4FDE30` | - | - |
| 377 | 0x10D6 | HasMagicEffect | - | 1 | `0x505710` | `0x4FDE30` | `0x4F8300` | - |
| 378 | 0x10D7 | GetDoorDefaultOpen | - | 0 | `0x5057D0` | `0x4FDE30` | `0x4F57A0` | yes |
| 379 | 0x10D8 | SetDoorDefaultOpen | - | 1 | `0x50C430` | `0x4FDE30` | - | yes |
| 380 | 0x10D9 | ShowClassMenu | - | 0 | `0x505810` | `0x4FDE30` | - | - |
| 381 | 0x10DA | ShowRaceMenu | - | 0 | `0x505850` | `0x4FDE30` | - | - |
| 382 | 0x10DB | ShowBirthSignMenu | - | 0 | `0x505870` | `0x4FDE30` | - | - |
| 383 | 0x10DC | ShowNameMenu | - | 0 | `0x977C50` | `0x4FDE30` | - | - alias:0x0141,0x0173,0x110C,0x114A,0x115F,0x1160 |
| 384 | 0x10DD | SetOpenState | - | 1 | `0x50C560` | `0x4FDE30` | - | yes |
| 385 | 0x10DE | CloseOblivionGate | - | 1 | `0x515EF0` | `0x4FDE30` | - | - |
| 386 | 0x10DF | IsSpellTarget | - | 1 | `0x505770` | `0x4FDE30` | `0x4F5730` | - |
| 387 | 0x10E0 | GetIsPlayerBirthsign | GetPBS | 1 | `0x5058D0` | `0x4FDE30` | `0x4F8370` | - |
| 388 | 0x10E1 | GetPersuasionNumber | - | 0 | `0x505930` | `0x4FDE30` | `0x4F57D0` | - |
| 389 | 0x10E2 | VampireFeed | - | 1 | `0x50C620` | `0x4FDE30` | - | - |
| 390 | 0x10E3 | HasVampireFed | - | 0 | `0x505970` | `0x4FDE30` | `0x4F5810` | - |
| 391 | 0x10E4 | GetIsClassDefault | - | 1 | `0x505990` | `0x4FDE30` | `0x4F83D0` | - |
| 392 | 0x10E5 | GetClassDefaultMatch | - | 0 | `0x5059F0` | `0x4FDE30` | `0x4F5880` | - |
| 393 | 0x10E6 | GetInCellParam | - | 2 | `0x5030D0` | `0x4FDE30` | `0x4F6D70` | - |
| 394 | 0x10E7 | SetInChargen | - | 1 | `0x5032D0` | `0x4FDE30` | - | yes |
| 395 | 0x10E8 | GetCombatTarget | - | 0 | `0x50C070` | `0x4FDE30` | - | - |
| 396 | 0x10E9 | GetPackageTarget | - | 0 | `0x50C0F0` | `0x4FDE30` | - | - |
| 397 | 0x10EA | ShowSpellMaking | - | 0 | `0x505890` | `0x4FDE30` | - | - |
| 398 | 0x10EB | ShowEnchantment | - | 0 | `0x5058B0` | `0x4FDE30` | - | - |
| 399 | 0x10EC | SetGhost | - | 1 | `0x505A30` | `0x4FDE30` | - | yes |
| 400 | 0x10ED | GetIsGhost | - | 0 | `0x505A10` | `0x4FDE30` | `0x4F5900` | yes |
| 401 | 0x10EE | EquipItem | EquipObject | 2 | `0x5162B0` | `0x4FDE30` | - | - |
| 402 | 0x10EF | UnequipItem | UnEquipObject | 2 | `0x5164C0` | `0x4FDE30` | - | - |
| 403 | 0x10F0 | SetClass | - | 1 | `0x50C6E0` | `0x4FDE30` | - | - |
| 404 | 0x10F1 | SetUnconscious | - | 1 | `0x505AC0` | `0x4FDE30` | - | yes |
| 405 | 0x10F2 | GetUnconscious | - | 0 | `0x505B50` | `0x4FDE30` | `0x4F7370` | yes |
| 406 | 0x10F3 | SetRestrained | - | 1 | `0x505B70` | `0x4FDE30` | - | yes |
| 407 | 0x10F4 | GetRestrained | - | 0 | `0x505C00` | `0x4FDE30` | `0x4F73F0` | yes |
| 408 | 0x10F5 | ForceFlee | Flee | 2 | `0x50C790` | `0x4FDE30` | - | yes |
| 409 | 0x10F6 | GetIsUsedItem | - | 1 | `0x503650` | `0x4FDE30` | `0x4F4D50` | - |
| 410 | 0x10F7 | GetIsUsedItemType | - | 1 | `0x5036B0` | `0x4FDE30` | `0x4F71D0` | - |
| 411 | 0x10F8 | ModPCFame | - | 1 | `0x50C980` | `0x4FDE30` | - | yes |
| 412 | 0x10F9 | GetPCFame | - | 0 | `0x505C20` | `0x4FDE30` | `0x4F5960` | yes |
| 413 | 0x10FA | ModPCInfamy | - | 1 | `0x50CA90` | `0x4FDE30` | - | yes |
| 414 | 0x10FB | GetPCInfamy | - | 0 | `0x505C40` | `0x4FDE30` | `0x4F59B0` | yes |
| 415 | 0x10FC | SetPCFame | - | 1 | `0x50CA10` | `0x4FDE30` | - | yes |
| 416 | 0x10FD | SetPCInfamy | - | 1 | `0x50CB20` | `0x4FDE30` | - | yes |
| 417 | 0x10FE | GetIsPlayableRace | - | 0 | `0x5033F0` | `0x4FDE30` | `0x4F7030` | - |
| 418 | 0x10FF | GetOffersServicesNow | - | 0 | `0x505C60` | `0x4FDE30` | `0x4F5A00` | - |
| 419 | 0x1100 | GetGameSetting | GetGS | 1 | `0x513FC0` | `0x4FDE30` | - | - |
| 420 | 0x1101 | StopCombatAlarmOnActor | SCAOnActor | 0 | `0x501D80` | `0x4FDE30` | - | - |
| 421 | 0x1102 | GetUsedItemLevel | - | 0 | `0x503710` | `0x4FDE30` | `0x4F4DA0` | - |
| 422 | 0x1103 | GetUsedItemActivate | - | 0 | `0x503730` | `0x4FDE30` | `0x4F4DE0` | - |
| 423 | 0x1104 | SetWeather | sw | 2 | `0x507AE0` | `0x4FDE30` | - | yes |
| 424 | 0x1105 | ForceTakeCover | TakeCover | 2 | `0x50C8D0` | `0x4FDE30` | - | yes |
| 425 | 0x1106 | ModBarterGold | - | 1 | `0x505C80` | `0x4FDE30` | - | - |
| 426 | 0x1107 | SetBarterGold | - | 1 | `0x50CBC0` | `0x4FDE30` | - | - |
| 427 | 0x1108 | GetBarterGold | - | 0 | `0x505D10` | `0x4FDE30` | `0x4F5A80` | - |
| 428 | 0x1109 | IsTimePassing | - | 0 | `0x505D90` | `0x4FDE30` | `0x4F5AE0` | - |
| 429 | 0x110A | IsPleasant | - | 0 | `0x502EB0` | `0x4FDE30` | `0x4F68F0` | - |
| 430 | 0x110B | IsCloudy | - | 0 | `0x502ED0` | `0x4FDE30` | `0x4F6960` | - |
| 431 | 0x110C | TrapUpdate | - | 0 | `0x977C50` | `0x4FDE30` | - | - alias:0x0141,0x0173,0x10DC,0x114A,0x115F,0x1160 |
| 432 | 0x110D | SetQuestObject | - | 2 | `0x5052D0` | `0x4FDE30` | - | - |
| 433 | 0x110E | ForceActorValue | ForceAV | 2 | `0x501890` | `0x4FDE30` | - | yes |
| 434 | 0x110F | ModPCSkill | ModPCS | 2 | `0x50D020` | `0x4FDE30` | - | yes alias:0x10D4 |
| 435 | 0x1110 | ModPCAttribute | ModPCA | 2 | `0x50D3E0` | `0x4FDE30` | - | yes |
| 436 | 0x1111 | EnableFastTravel | EnableFast | 1 | `0x50D460` | `0x4FDE30` | - | - |
| 437 | 0x1112 | GetArmorRatingUpperBody | - | 0 | `0x505DB0` | `0x4FDE30` | `0x4F84F0` | - |
| 438 | 0x1113 | GetParentRef | - | 0 | `0x50C1C0` | `0x4FDE30` | - | - |
| 439 | 0x1114 | PlayBink | - | 2 | `0x50D4C0` | `0x4FDE30` | - | - |
| 440 | 0x1115 | GetBaseActorValue | GetBaseAV | 1 | `0x501A00` | `0x4FDE30` | `0x4F45D0` | yes |
| 441 | 0x1116 | IsOwner | - | 1 | `0x500570` | `0x4FDE30` | `0x4F8AD0` | yes |
| 442 | 0x1117 | SetOwnership | - | 1 | `0x5075A0` | `0x4FDE30` | - | - |
| 443 | 0x1118 | IsCellOwner | - | 2 | `0x505DD0` | `0x4FDE30` | `0x4F8B90` | yes |
| 444 | 0x1119 | SetCellOwnership | - | 2 | `0x505E60` | `0x4FDE30` | - | - |
| 445 | 0x111A | IsHorseStolen | - | 0 | `0x505EE0` | `0x4FDE30` | `0x4F8C20` | - |
| 446 | 0x111B | SetCellFullName | - | 2 | `0x512AE0` | `0x4FDE30` | - | - |
| 447 | 0x111C | SetActorFullName | - | 1 | `0x512BB0` | `0x4FDE30` | - | - |
| 448 | 0x111D | IsLeftUp | - | 0 | `0x504930` | `0x4FDE30` | `0x4F89E0` | - |
| 449 | 0x111E | IsSneaking | - | 0 | `0x505F20` | `0x4FDE30` | `0x4F8CF0` | - |
| 450 | 0x111F | IsRunning | - | 0 | `0x505F40` | `0x4FDE30` | `0x4F8DB0` | - |
| 451 | 0x1120 | GetFriendHit | - | 1 | `0x505F60` | `0x4FDE30` | `0x4F8E70` | - |
| 452 | 0x1121 | IsInCombat | - | 0 | `0x505FC0` | `0x4FDE30` | `0x4F8F30` | - |
| 453 | 0x1122 | SetPackDuration | SPDur | 1 | `0x50D5E0` | `0x4FDE30` | - | - |
| 454 | 0x1123 | PlayMagicShaderVisuals | PMS | 2 | `0x505FE0` | `0x4FDE30` | - | - |
| 455 | 0x1124 | PlayMagicEffectVisuals | PME | 2 | `0x512CA0` | `0x4FDE30` | - | - |
| 456 | 0x1125 | StopMagicShaderVisuals | SMS | 1 | `0x5061B0` | `0x4FDE30` | - | - |
| 457 | 0x1126 | StopMagicEffectVisuals | SME | 1 | `0x512FA0` | `0x4FDE30` | - | - |
| 458 | 0x1127 | ResetInterior | - | 1 | `0x506260` | `0x4FDE30` | - | - |
| 459 | 0x1128 | IsAnimPlaying | - | 0 | `0x510A90` | `0x4FDE30` | - | - |
| 460 | 0x1129 | SetActorAlpha | SAA | 1 | `0x50D680` | `0x4FDE30` | - | - |
| 461 | 0x112A | EnableLinkedPathPoints | - | 0 | `0x5062B0` | `0x4FDE30` | - | - |
| 462 | 0x112B | DisableLinkedPathPoints | - | 0 | `0x5062D0` | `0x4FDE30` | - | - |
| 463 | 0x112C | IsInInterior | - | 0 | `0x5062F0` | `0x4FDE30` | `0x4F9010` | - |
| 464 | 0x112D | ForceWeather | fw | 2 | `0x500D90` | `0x4FDE30` | - | yes |
| 465 | 0x112E | ToggleActorsAI | - | 0 | `0x501120` | `0x4FDE30` | - | - |
| 466 | 0x112F | IsActorsAIOff | - | 0 | `0x501180` | `0x4FDE30` | - | - |
| 467 | 0x1130 | SetInvestmentGold | - | 1 | `0x50CC80` | `0x4FDE30` | - | - |
| 468 | 0x1131 | GetInvestmentGold | - | 0 | `0x505D50` | `0x4FDE30` | `0x4F8420` | - |
| 469 | 0x1132 | IsActorUsingATorch | - | 0 | `0x505D70` | `0x4FDE30` | `0x4F8480` | - |
| 470 | 0x1133 | SetLevel | - | 4 | `0x50CD30` | `0x4FDE30` | - | - |
| 471 | 0x1134 | ResetFallDamageTimer | - | 0 | `0x50CFB0` | `0x4FDE30` | - | - |
| 472 | 0x1135 | IsXBox | - | 0 | `0x505D30` | `0x4FDE30` | `0x4F5D80` | - |
| 473 | 0x1136 | GetInWorldspace | - | 1 | `0x503140` | `0x4FDE30` | `0x4F6E50` | - |
| 474 | 0x1137 | ModPCMiscStat | ModPCMS | 2 | `0x50D740` | `0x4FDE30` | - | - |
| 475 | 0x1138 | GetPCMiscStat | GetPCMS | 1 | `0x506310` | `0x4FDE30` | `0x4F8590` | - |
| 476 | 0x1139 | IsActorEvil | - | 0 | `0x506370` | `0x4FDE30` | `0x4F5B30` | - |
| 477 | 0x113A | IsActorAVictim | - | 0 | `0x506390` | `0x4FDE30` | `0x4F5BB0` | - |
| 478 | 0x113B | GetTotalPersuasionNumber | - | 0 | `0x505950` | `0x4FDE30` | `0x4F57F0` | - |
| 479 | 0x113C | SetScale | - | 1 | `0x5093B0` | `0x4FDE30` | - | yes |
| 480 | 0x113D | ModScale | - | 1 | `0x501E40` | `0x4FDE30` | - | yes |
| 481 | 0x113E | GetIdleDoneOnce | - | 0 | `0x5063B0` | `0x4FDE30` | `0x4F85E0` | - |
| 482 | 0x113F | KillAllActors | killall | 1 | `0x508700` | `0x4FDE30` | - | - |
| 483 | 0x1140 | GetNoRumors | - | 0 | `0x5063D0` | `0x4FDE30` | `0x4F5C50` | - |
| 484 | 0x1141 | SetNoRumors | - | 1 | `0x50D7B0` | `0x4FDE30` | - | - |
| 485 | 0x1142 | Dispel | - | 1 | `0x502140` | `0x4FDE30` | - | - |
| 486 | 0x1143 | WhichServiceMenu | - | 0 | `0x506410` | `0x4FDE30` | `0x4F5C90` | - |
| 487 | 0x1144 | TriggerHitShader | ths | 1 | `0x506450` | `0x4FDE30` | - | - |
| 488 | 0x1145 | RefreshTopicList | - | 0 | `0x5064A0` | `0x4FDE30` | - | - |
| 489 | 0x1146 | Reset3DState | - | 0 | `0x50E320` | `0x4FDE30` | - | - |
| 490 | 0x1147 | IsRidingHorse | - | 0 | `0x5064B0` | `0x4FDE30` | `0x4F5CB0` | - |
| 491 | 0x1148 | DispelAllSpells | - | 0 | `0x5021E0` | `0x4FDE30` | - | - |
| 492 | 0x1149 | IsTurnArrest | - | 0 | `0x506570` | `0x4FDE30` | `0x4F5D60` | - |
| 493 | 0x114A | AddAchievement | - | 1 | `0x977C50` | `0x4FDE30` | - | - alias:0x0141,0x0173,0x10DC,0x110C,0x115F,0x1160 |
| 494 | 0x114B | DuplicateAllItems | - | 2 | `0x507470` | `0x4FDE30` | - | - |
| 495 | 0x114C | IsInDangerousWater | - | 0 | `0x506530` | `0x4FDE30` | `0x4F86A0` | - |
| 496 | 0x114D | EssentialDeathReload | - | 1 | `0x5071D0` | `0x500130` | - | - |
| 497 | 0x114E | SetShowQuestItems | - | 1 | `0x50AA40` | `0x4FDE30` | - | - |
| 498 | 0x114F | DuplicateNPCStats | - | 1 | `0x50AAA0` | `0x4FDE30` | - | - |
| 499 | 0x1150 | ResetHealth | - | 0 | `0x5028F0` | `0x4FDE30` | - | - |
| 500 | 0x1151 | SetIgnoreFriendlyHits | sifh | 1 | `0x506750` | `0x4FDE30` | - | yes |
| 501 | 0x1152 | GetIgnoreFriendlyHits | gifh | 0 | `0x5067C0` | `0x4FDE30` | `0x4F8720` | yes |
| 502 | 0x1153 | IsPlayersLastRiddenHorse | - | 0 | `0x5064F0` | `0x4FDE30` | `0x4F5D20` | - |
| 503 | 0x1154 | SetActorRefraction | sar | 1 | `0x50E340` | `0x4FDE30` | - | - |
| 504 | 0x1155 | SetItemValue | - | 1 | `0x506810` | `0x4FDE30` | - | - |
| 505 | 0x1156 | SetRigidBodyMass | - | 1 | `0x5131F0` | `0x4FDE30` | - | - |
| 506 | 0x1157 | ShowViewerStrings | svs | 0 | `0x50FF70` | `0x4FDE30` | - | - |
| 507 | 0x1158 | ReleaseWeatherOverride | rwo | 0 | `0x507B90` | `0x4FDE30` | - | - |
| 508 | 0x1159 | SetAllReachable | - | 1 | `0x5012F0` | `0x4FDE30` | - | - |
| 509 | 0x115A | SetAllVisible | - | 1 | `0x501340` | `0x4FDE30` | - | - |
| 510 | 0x115B | SetNoAvoidance | - | 1 | `0x501390` | `0x4FDE30` | - | - |
| 511 | 0x115C | SendTrespassAlarm | - | 1 | `0x506AF0` | `0x4FDE30` | - | - |
| 512 | 0x115D | SetSceneIsComplex | - | 1 | `0x50E9D0` | `0x4FDE30` | - | - |
| 513 | 0x115E | Autosave | - | 0 | `0x50A3E0` | `0x4FDE30` | - | - |
| 514 | 0x115F | StartMasterFileSeekData | - | 0 | `0x977C50` | `0x4FDE30` | - | - alias:0x0141,0x0173,0x10DC,0x110C,0x114A,0x1160 |
| 515 | 0x1160 | DumpMasterFileSeekData | - | 0 | `0x977C50` | `0x4FDE30` | - | - alias:0x0141,0x0173,0x10DC,0x110C,0x114A,0x115F |
| 516 | 0x1161 | IsActor | - | 0 | `0x506D00` | `0x4FDE30` | `0x4F5E30` | - |
| 517 | 0x1162 | IsEssential | - | 0 | `0x506CC0` | `0x4FDE30` | `0x4F5DF0` | - |
| 518 | 0x1163 | PreloadMagicEffect | - | 1 | `0x513090` | `0x4FDE30` | - | - |
| 519 | 0x1164 | ShowDialogSubtitles | - | 1 | `0x5007D0` | `0x4FDE30` | - | - alias:0x0151 |
| 520 | 0x1165 | ForceCloseOblivionGate | - | 1 | `0x5160D0` | `0x4FDE30` | - | - |
| 521 | 0x1166 | IsPlayerMovingIntoNewSpace | - | 0 | `0x506D40` | `0x4FDE30` | `0x4F5E60` | - |
| 522 | 0x1167 | CreateFullActorCopy | - | 0 | `0x511570` | `0x4FDE30` | - | - |
| 523 | 0x1168 | DeleteFullActorCopy | - | 0 | `0x50A340` | `0x4FDE30` | - | - |
| 524 | 0x1169 | GetTimeDead | - | 0 | `0x506D80` | `0x4FDE30` | `0x4F8740` | - |
| 525 | 0x116A | GetPlayerHasLastRiddenHorse | - | 0 | `0x505F00` | `0x4FDE30` | `0x4F8630` | - |
| 526 | 0x116B | PurgeCellBuffers | pcb | 0 | `0x500C60` | `0x4FDE30` | - | yes |
| 527 | 0x116C | SetPlayerInSEWorld | SPInSE | 1 | `0x50EA30` | `0x4FDE30` | - | - |
| 528 | 0x116D | GetPlayerInSEWorld | gpInSE | 0 | `0x506DC0` | `0x4FDE30` | `0x4F87F0` | - |
| 529 | 0x116E | PushActorAway | - | 2 | `0x50EAB0` | `0x4FDE30` | - | - |
| 530 | 0x116F | SetActorsAI | - | 1 | `0x506DE0` | `0x4FDE30` | - | - |
| 531 | 0x1170 | ClearOwnership | - | 0 | `0x507620` | `0x4FDE30` | - | - |

## 共有ハンドラ（別名オペコード）

同一ハンドラを複数のオペコードが共有する組（別名コマンド）。

| ハンドラ | オペコード |
|---|---|
| `0x5007D0` | 0x0151, 0x1164 |
| `0x500890` | 0x0155, 0x0169 |
| `0x502270` | 0x0001, 0x1024 |
| `0x503E10` | 0x0004, 0x001C |
| `0x503E90` | 0x0005, 0x001D |
| `0x50D020` | 0x10D4, 0x110F |
| `0x977C50` | 0x0141, 0x0173, 0x10DC, 0x110C, 0x114A, 0x115F, 0x1160 |

## 実装なしの 5 件

| # | opcode | 長名 |
|---|---|---|
| 162 | 0x0183 | ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!! |
| 188 | 0x1019 | UnusedFunction1 |
| 189 | 0x101A | UnusedFunction2 |
| 216 | 0x1035 | GetScriptVariable |
| 242 | 0x104F | GetQuestVariable |

## 例外 dispatcher

| dispatcher | 件数 | 対象 opcode |
|---|---|---|
| 0 | 1 | 0x0183 |
| `0x500130` | 2 | 0x1000, 0x114D |
| `0x500150` | 1 | 0x1059 |
