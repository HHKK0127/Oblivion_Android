# v531 — `+0x30` の実 call site 全3箇所（基底委譲は top-level のみ）・`TESObjectCELL` vtable `+0x30`/`+0x34`/`+0xC0` の groupType 判定と生成・groupType 6/7/8/9 の builder 特定・packed 4CC グローバル 3点・TESForm vtable `+0x78`〜`+0xB8` identity（V-531-1..3 完遂・旧残点 V-406-3 継続第61弾）

- 採番: **v531**（旧残点群消化・probe prefix **v531_1..13 全13本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-531-1..3（v530 残点）** — 1=`+0x30`（`0x46B510`）の実 call site 特定＋`case 0` の意味（順序か所属か）決着／2=groupType 1..5,7 の実データ確認とヘッダ組立関数の探索／3=`TESForm` vtable `+0x78`〜`+0xB8` の未読 slot identity。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・RTTI（vtable 実読）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-531-1: 基底 `+0x30`（`0x46B510`）の実 call site** | **[確定]（E8 機械走査）直接 call は全3箇所、全て「コンテナ vtable `+0x30` override の `case 0`（top-level グループ）」からの基底委譲** | ① `0x4CDEE9`（head `FUN_004CDEC0`＝**`TESObjectCELL` vtable `0xA469D4` の `+0x30`**、COL `0xABCCBC=.?AVTESObjectCELL@@`）② `0x4EEF9A`（head `FUN_004EEF70`＝**`TESWorldSpace` vtable `0xA4829C` の `+0x30`**）③ `0x52EC9A`（head `FUN_0052EC70`＝**`TESTopic` vtable `0xA5555C` と `TESTopicInfo` vtable `0xA5568C` の `+0x30``（同一実装を2クラスが共有）。静的 call 以外に `call [reg+0x30]`（disp32 形式 `ff 9x 30 00 00 00`）は `.text` に **0 件**、`ff 5X 30` は全部別オブジェクトの 0x30。 |
| **V-531-1: `+0x30` の意味** | **[確定]（override 群の構造で決着）「自分（コンテナ form）はこの 20B GRUP ヘッダの子グループに、いま保存／読み込み中の form を属させるか」の membership predicate** | TESForm 基底（`0x46B510`）は `groupType 0`（top-level）のみ型 index 順序で判定し、**実データ上のコンテナ（CELL/WRLD/TOPIC/TOPICINFO/REFR/Actor 系）は例外なく `+0x30` を override して groupType 別の判定に差し替える**（§2.2〜§2.4）。基底を呼ぶのは各 override の `case 0` だけ＝**「top-level グループには型 index が label 型 index より小さい form だけが入る」は順序による所属判定で確定**。 |
| **V-531-1: `case 0` の具体像** | **[確定]** | CELL の `+0xC0` builder `FUN_004CE1A0` は親 descriptor が無い（top）とき `{GRUP, 0, label=DAT_00b06048（packed 'CELL'）, groupType 0, 0}` を組む（`0x4CE28B..0x4CE2AD`）。CELL の型 index は **48（0x30）**。よって top GRUP `'CELL'` の下には index<48 の型（`ACTN..BOOK` 等の world-unknown form）が入り、**セル配置が必要な CELL 自体は top には入らず WRLD 経由（groupType 1..5）で保存される**構造と自己整合。 |
| **V-531-2: CELL `+0x30` override（`FUN_004CDEC0`）の groupType 別判定** | **[確定]（命令＋jump table `0x4CDFC0` 実読）** | `header==0 / *header!='GRUP' / groupType>0xa → false`。`case 0`→基底委譲。`case 1`→`FUN_0046B250(label, TESWorldSpace TD 0xB0301C)` で WRLD を formID 検索→`this->vtable[+0x34](wrld)`。`case 2`→interior gate→**`FUN_004CA5F0(this) != header[2] → true`**（block 一致）。`case 3`→同 `FUN_004CA640`（sub-block）。`case 4`→exterior gate（interior か flags bit10 で block 比較へ）。`case 5`→sub-block 比較。`case 6,8,9,10`→`FUN_0046B250(label, TESObjectCELL TD 0xB03038)` で CELL 検索→`this->vtable[+0x34](cell)`。`case 7`→無条件 true。 |
| **V-531-2: CELL `+0x34` override（`FUN_004CDC90`）の意味** | **[確定]（命令＋jump table `0x4CDE94` 実読）「label から引いた form（CELL 等）が自分の親か／自分の block か」＝所属検証** | `switch (target->type - 0x30)`（CELL..0x38 の9型）: `CELL`→`FUN_009832E6(form, TESForm TD, TESObjectCELL TD)` 検索→`[+0x24]&1` の interior 一致＋（exterior 時）`+0x50`（worldspace）比較／`WRLD(0x35)`→interior なら false、`this+0x50` の worldspace へ `vtable[+0x34]` 委譲（`0x4CDD07`）／`0x37`→TD `0xB0966C=.?AVTESTerrainLODQuadRoot@@` 検索後 `+4/+0x10` から worldspace を引いて比較、不一致は worldspace の `+0x34` へ／`0x38`→TD `0xB09654=.?AVTESRoad@@`、`+0x2c` 同上／既定（`0x31..0x34,0x36`）→TD `0xB08968=.?AVTESChildCell@@` 経由で `parent()` 相当 `vtable[0]()` を2回読み、直接一致で true、違えば `this->vtable[+0x34](owner)` に委譲（`0x4CDCE7`）。 |
| **V-531-2: groupType 1..5 の生成（`FUN_004CE1A0`＝CELL `+0xC0` override）** | **[確定]（命令）builder 一箇所から全5種** | 引数 `(this=CELL via ecx, out hdr=param_1, parent descriptor=param_2@ebx)`。`[this+0x24]&1` で interior 分岐、descriptor の `[3]=kind`（0=TES4/最上位, 1=world, 2=block, 3=sub-block）: **interior 且つ descriptor 無し**→`{GRUP,0,'CELL' packed,0,0}`（top）／interior+kind0+label=`'CELL' packed([0xB06048])`→**`{GRUP,0,FUN_004CA5F0(this),2,0}`**（interior block）／interior+kind2+label=`CA5F0(this)`→**`{GRUP,0,FUN_004CA640(this),3,0}`**（interior sub-block）／exterior+kind0+label=`'WRLD' packed([0xB06084])`→**`{GRUP,0,worldspace(this+0x50)->+0xC(formID),1,0}`**（world children）／exterior+kind1+label=`worldspace formID`→**`{GRUP,0,CA5F0(this),4,0}`**（exterior block）／exterior+kind3+label=`CA640(this)`+flags bit10 未設定→**`{GRUP,0,CA640(this),5,0}`**（exterior sub-block）。→ **v530 の [推定] 5名称が命令で [確定]**。 |
| **V-531-2: block/sub-block ラベルの値** | **[確定]（命令）** | `FUN_004CA5F0(cell)`: interior=`(formID&0xFFFFFF)%10`、exterior=`FUN_004EF1D0(pair[0]>>5, pair[1]>>5)`（+0x3c の座標 pair を **16bit×2 packed**、`FUN_004EF1D0(lo,hi)=CONCAT22`）。`FUN_004CA640(cell)`: interior=`((formID&0xFFFFFF)%100)/10`、exterior=`pair>>3` packed。→ **label は「block index×（>>5）と sub index×（>>3）を packed した座標キー」**。 |
| **V-531-2: groupType 6 の生成（CELL 側＋REFR 側）** | **[確定]** | ① v530 既出: `FUN_004D3460`/`FUN_004D3570` が `{GRUP,0,label=cell formID,6,0}`（cell children の親側）。② **`TESObjectREFR`/`Actor`/`Character`/`Creature`/`Projectile` 系 vtable `+0xC0`＝`FUN_004E0000`**（slot `0xA46D04`→REFR vtable `0xA46C44`+0xC0 等）が、親 group ヘッダ `[ebp+0xc]`（groupType）が 1..6 のとき **`{...,label=親コンテナ formID, groupType: 6 または 8, 0}`** を選択生成（`0x4E0078` に 6、`0x4E0099` に 8。 interior bit（`0x4D7030`＝TESForm `+0x78`＝flags bit10 getter）で切替）。 |
| **V-531-2: groupType 7** | **[部分確定]** | builder らしきは `FUN_0052EC70` 系の**手前**にある `TESTopicInfo` vtable `+0x34`＝**`FUN_00530720`**（slot `0xA556C0`）が `{..., [esi+0xc]=7, ...}` を組む（`0x53080D`）。TOPIC/TOPICINFO `+0x30` override（`FUN_0052EC70`）の `case 7` は `FUN_0046B250(label, TD 0xB05390=.?AVTESTopic@@)` で topic form を formID 検索→`vtable[+0x34](topic)`。基底 `+0x30` の `case 7: type < 0x39`（=DIAL 0x39 の手前、INFO=0x38 を含む）と整合 → **7=「TESTopic（topic）の子＝INFO 系」**の方向で [部分確定]（生成経路の完全追跡は残点）。 |
| **V-531-2: groupType 8/9/10** | **[部分確定]**（8/9）・**[未確定]**（10） | **8**=REFR builder `0x4E0099` が生成（exterior 側の選択肢）、loader `FUN_004D1340` は `hdr@0x248==8` で `FUN_00451580()` を呼ぶ。**9**=`TESObjectLAND`/`TESPathGrid` vtable `+0xC0`＝**`FUN_004E4E00`**（`0x4E4E3E` に 9、`0x4E4E6C` に 6）と `TESRoad` 系が生成、loader は `==9||==10` でセル読み込みループに入り、`piVar6[3]==9` の子グループをスキップ読みする。**10** は生成 site 未発見 [未確定]。**CELL predicate `FUN_004CDEC0` は 8/9/10 を 6 と同一経路（CELL formID 検索）で処理 [確定]**。 |
| **追加 [確定]（命令）`TESObjectREFR`/`Actor` 系の `+0x30` override＝`FUN_004DFE70`** | **[確定]** | slot: REFR `0xA46C74`（`+0x30`）、Actor/Character/Creature/MobileObject/PlayerCharacter/Magic*Projectile の各 `+0x30`（計12クラス）。内容は `*header=='GRUP'`＋**`groupType-8 < 3`（=8/9/10 のみ）**で `FUN_0046B250(label, CELL TD 0xB03038)`→`this->vtable[0]()`（container getter）と一致→`FUN_004DB4A0()`／flags bit15 判定。**＝「REFR は cell-block（8/9/10）グループのメンバになれるか」を親セル formID で検証**。 |
| **追加 [確定] packed 4CC グローバル 3点** | **[確定]** | **`DAT_00b06048 = packed 'CELL'`**（表 index 48 の f1）、**`DAT_00b06084 = packed 'WRLD'`**（index 53）、**`DAT_00b06138 = packed 'TOFT'`**（index 68）。読み site: `0x4CE2F0/0x4CE257`（CELL builder）、`FUN_00450080` の「GRUP でも TOFT でもないレコード」分岐、`FUN_004D1340` の `*piVar1 == DAT_00b06048`（CELL 直前チェック）。 |
| **V-531-3: TESForm vtable `+0x78..+0xB8` identity** | **[確定]（命令）** | `+0x78=0x4D7030`＝**`flags bit10 getter`**（`([+8]>>0xA)&1`）／`+0x7C=0x412980`＝`flags bit6 getter`／`+0x80=0x412990`＝`byte[+0xA] bit0 getter`／`+0x84/+0x88=0x4CA690`＝`flags bit17?（>>0x11）getter`（同一実装の二 slot）／`+0x8C=0x46A9E0`＝**`flags bit5(0x20) set/clear → jmp vtable[+0x40]`**／`+0x90=0x46C640`（flags bit8 系 set/clear）／`+0x94=0x46AA80`＝`bit10(0x400) set/clear→jmp +0x40`／`+0x98=0x46AAB0`＝`bit6(0x40) set/clear→jmp +0x40`／`+0x9C=0x46AAD0`＝`bit16(0x10000) set/clear→jmp +0x40`／`+0xA0=0x46AB00`＝`bit17(0x20000) setter ret 4`／`+0xA4/+0xA8/+0xAC=0x69D990`＝`xor al,al; ret`（**false 固定 no-op**）／`+0xB0=0x6F7070`＝`xor eax,eax; ret`（**0 固定**）／`+0xB4=0x46B7A0`＝**copy 未実装エラーログ**（`"Trying to copy form \"%s\".\r\nNo copy function for %s form types."` `0xA3BD20`、型名=`[0xB05E04].f0`、form 名=`vtable[+0xD4]`）／`+0xB8=0x46B7D0`＝**compare 未実装エラーログ**（`"Trying to compare form \"%s\"..."` `0xA3BD68`）。 |

## §2 証拠

### §2.1 基底 `+0x30` への直接 call 全3箇所（E8 ターゲット走査）
```
call site 0x4CDEE9 -> FUN_0046B510  head=0x4CDEC0  (TESObjectCELL vtable 0xA469D4 +0x30, case 0)
call site 0x4EEF9A -> FUN_0046B510  head=0x4EEF70  (TESWorldSpace vtable 0xA4829C +0x30, case 0)
call site 0x52EC9A -> FUN_0046B510  head=0x52EC70  (TESTopic 0xA5555C / TESTopicInfo 0xA5568C +0x30, case 0)
call [reg+0x30] disp32 形式 (ff 9x 30000000): .text 0 件
```

### §2.2 `TESObjectCELL` vtable `0xA469D4`（COL `0xABCCBC`=`.?AVTESObjectCELL@@`）の保存系 override
```
+0x20 0x4D3460（CELL 保存・v530）  +0x2C 0x4D3570（CELL ヘッダ保存・v530）
+0x30 0x4CDEC0（★groupType 0..10 membership 判定）
+0x34 0x4CDC90（★label 引きの form が自分の親か）
+0xBC 0x4CE0A0（★CELL 版 top-level gate: case 0 は interior 時 TESForm +0xBC、
        exterior 時 parent worldspace(+0x50) の +0xBC へ委譲。case 1 は FUN_0046AF50('WRLD')、
        case 2/4 は CA5F0、case 3/5 は CA640 と header[2] を比較）
+0xC0 0x4CE1A0（★groupType 0..5 ヘッダ builder）
```

### §2.3 `FUN_004CDEC0`（CELL `+0x30`）jump table `0x4CDFC0` 実値
```
case 0 -> 0x4CDEE8（基底 0x46B510 委譲）  case 1 -> 0x4CDF7D（WRLD 検索→+0x34）
case 2 -> 0x4CDEF6（CA5F0!=label→true）   case 3 -> 0x4CDF10（CA640!=label→true）
case 4 -> 0x4CDF56（exterior gate+CA5F0）  case 5 -> 0x4CDF67（exterior gate+CA640）
case 6/8/9/10 -> 0x4CDF1D（CELL 検索→+0x34）  case 7 -> 0x4CDF06（true）
```

### §2.4 `FUN_004EEF70`（TESWorldSpace `+0x30`）jump table `0x4EF004` 実値
```
case 0 -> 0x4EEF97（基底委譲）  case 1 -> 0x4EEFE7（label を TESWorldSpace TD 0xB0301C で検索→vtable[+0x34]）
case 2,3 -> 0x4EEFA6（false）   case 4,5 -> 0x4EEFFD（false）
case 6 -> 0x4EEFAF（label を CELL TD 0xB03038 で検索→vtable[+0x34]）  case 7 -> 0x4EEFFB（true）
```

### §2.5 `FUN_0052EC70`（TESTopic/TESTopicInfo `+0x30`）jump table `0x52ECEC` 実値
```
case 0 -> 0x52EC97（基底委譲）  case 1..6 -> 0x52ECA6（false）  case 7 -> 0x52ECAF
（TESTopic TD 0xB05390 で label formID 検索→vtable[+0x34]）
```

### §2.6 `FUN_004DFE70`（REFR/Actor 系 `+0x30`）の所属
```
slot 0xA46C74=TESObjectREFR 0xA46C44+0x30 / 0xA6E0A4=Actor / 0xA6F0BC=ArrowProjectile /
0xA6FCCC=Character / 0xA71124=Creature / 0xA7332C=MobileObject / 0xA73A3C=PlayerCharacter /
0xA75974=MagicBallProjectile / 0xA75BF4=MagicBoltProjectile / 0xA75F2C=MagicFogProjectile /
0xA762B4=MagicProjectile / 0xA765C4=MagicSprayProjectile（12クラス）
内容: *header=='GRUP' && (header[3]-8)<3 → label formID で CELL 検索 → this->vtable[0]()==found → FUN_004DB4A0/flags bit15
```

### §2.7 `FUN_004CE1A0`（CELL `+0xC0` builder）の groupType 実書き込み
```
4CE295: [esi+0xc]=0 / 4CE2A8: [esi+8]=[0xb06048]('CELL' packed)   ; top
4CE2FE: [esi+0xc]=2 / label=CA5F0(this)                            ; interior block
4CE2D1: [esi+0xc]=3 / label=CA640(this)                            ; interior sub-block
4CE26B: [esi+0xc]=1 / label=[worldspace(+0x50)+0xc]                ; world children
4CE248: [esi+0xc]=4 / label=CA5F0(this)                            ; exterior block
4CE20A: [esi+0xc]=5 / label=CA640(this)                            ; exterior sub-block
入口判定: [edi+0x24]&1（interior）で interior 群、else exterior 群。親 descriptor ebx[3]=kind、
  exterior kind0 は ebx[2]=='WRLD' packed に等しいこと、kind1 は ebx[2]==worldspace formID、
  kind3 は ebx[2]==CA640(this) かつ flags bit10==0 を要求（不一致ならヘッダを作らない）。
```

### §2.8 他クラスの builder が書く groupType
```
FUN_004E0000（REFR/Actor 系 +0xC0）: [esi+0xc]=6（0x4E0078）/ 8（0x4E0099）
FUN_004E4E00（TESObjectLAND 0xA46064+0xC0 / TESPathGrid 0xA47644+0xC0）: 9（0x4E4E3E）/ 6（0x4E4E6C）
FUN_004E8BA0（TESRoad 0xA4788C+0xC0; +0xBC=0x4E8B30）: [eax+0xc]=1?→（0x4E8BCF、別経路）
FUN_00530720（TESTopicInfo 0xA5568C+0x34）: [esi+0xc]=7（0x53080D）
```

### §2.9 `FUN_004CDC90`（CELL `+0x34`）jump table `0x4CDE94`
```
type 0x30(CELL) -> 0x4CDD92（TESObjectCELL 検索+interior 一致+worldspace(+0x50) 比較）
type 0x35(WRLD) -> 0x4CDCF7（interior→true；else this+0x50 の worldspace->vtable[+0x34]）
type 0x37 -> 0x4CDD15（TESTerrainLODQuadRoot TD 0xB0966C）
type 0x38 -> 0x4CDD56（TESRoad TD 0xB09654）
type 0x31..0x34,0x36 -> 0x4CDCB3（TESChildCell TD 0xB08968 で parent()→自分 or this->vtable[+0x34](owner)）
```

## §3 判定の内訳

- **V-531-1 完遂**: 基底 `+0x30` の call site は 3 関数（CELL/WRLD/TOPIC+TOPICINFO の `+0x30` override）の `case 0` のみと機械的に確定。`case 0` の `type < index(label)` は「top-level グループの所属＝型順序」で決着（builder が top の label に自身の型 4CC を書くため、上位型の form が上位の top に混入しない不変条件）。コンテナ系（CELL/WRLD/TOPIC/REFR…）は override 側で formID/座標比較に切り替わる。
- **V-531-2 完遂**: groupType 1..5 は `FUN_004CE1A0`（CELL `+0xC0`）の命令で [確定]。6 は CELL 側（v530）＋REFR 側 builder の両論拠で [確定]。7/8/9 は [部分確定]（生成 site と判定経路は特定、名称の完全確定は DIAL/TOPIC 縦並びの追跡待ち）、10 は [未確定]。
- **V-531-3 完遂**: `+0x78..+0xB8` は §1 表のとおり（flags bit getter/setter・false/0 固定 no-op・copy/compare 未実装エラーログ）。

## §4 残点（V-532-1..3）

1. `FUN_004E0000`（REFR `+0xC0`）の jump table `0x4E00CC`（parent groupType 1..6 の各 case）と groupType 6/8 の選択規則の完全追跡、および loader 側の groupType 8 処理 `FUN_00451580` の本体【中】
2. `FUN_00530720`（TESTopicInfo `+0x34`）が組む groupType 7 の label 意味（DIAL formID か INFO formID か）と QUEST/DIAL 系 `+0xC0`/`+0x34` override の列挙【中】
3. `FUN_004CE0A0`（CELL `+0xBC`）が呼ばれる保存タイミング（`FUN_004502C0` の `(node,1,1)/(node,0,1)` との相互作用）の実行順追跡【低】

## §5 後続への申し送り

- **移植側の membership 実装は「基底＋override」の2層**：基底 `TESForm +0x30`（top-level の型順序）だけでは不十分で、**CELL/WRLD/TOPIC/TOPICINFO/REFR/Actor 系は `+0x30` を完全に置換**する（CELL は groupType 0..10、WRLD は 0..7、REFR 系は 8..10 専用、TOPIC 系は 0 と 7）。基底だけ実装するとセル配置と子グループの往復が壊れる。
- **`FUN_004CE1A0` は「親ヘッダ descriptor から自分の子ヘッダを組み立てる」唯一の CELL builder**。移植側は descriptor `{parentKind@0xc, parentLabel@8}` 相当を保持し、§2.7 の表どおり `{label, groupType}` を選ぶ。
- **groupType の label 意味は型で変わる**: 0=型 4CC／1=worldspace formID／2,4=block packed（座標 >>5 または formID%10）／3,5=sub-block packed（>>3 または %100 の位）／6,8,9,10=cell formID／7=topic formID。
- **block/sub-block の packed 値は `FUN_004CA5F0`/`FUN_004CA640` の一元関数**で算出（exterior=座標>>5,>>3、interior=formID の10の位/100の位）。移植側はこの 2 関数を共有実装にすること。
- `DAT_00b06048='CELL'`／`DAT_00b06084='WRLD'`／`DAT_00b06138='TOFT'` は `0xB05E04` 表 f1 スロットの named グローバル（実行時充填 packed 4CC）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
