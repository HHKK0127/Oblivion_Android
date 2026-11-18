# v532 — REFR/Actor builder `FUN_004E0000` の jump table 実読（groupType 6/8/9/10 選択規則確定）・groupType 7 の label＝TESTopic formID（`FUN_005307D0` builder）・Road/LAND/PathGrid の builder・`FUN_0046AF50`＝formID 下24bit 一致判定・CELL `+0xBC` の分岐意味（V-532-1..3 完遂・旧残点 V-406-3 継続第62弾）

- 採番: **v532**（旧残点群消化・probe prefix **v532_1..5 全5本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-532-1..3（v531 残点）** — 1=`FUN_004E0000` jump table（`0x4E00CC`）の親 groupType 1..6 各 case と 6/8 選択規則＋`FUN_00451580`（groupType 8 loader）本体／2=groupType 7 の label 意味（`FUN_00530720`）と QUEST/DIAL 系 override 列挙／3=`FUN_004CE0A0`（CELL `+0xBC`）の保存タイミング実行順。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・RTTI（vtable 実読）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-532-1: `FUN_004E0000`（REFR/Actor 系 `+0xC0` builder）の全規則** | **[確定]（命令＋jump table `0x4E00CC` 実読）→ v531 §2.8 の「interior bit で 6/8 選択」は誤り → 訂正** | `edi = virtual[0](this+0x18)` で**所属コンテナ form** を解決し `label = edi->+0xC`。親 descriptor の groupType（`parentDesc[3]`）で分岐: **kind 1**（world children）: `bit10 = edi->vtable[+0x78]()` が立つか `CA640(edi) == parentDesc[2]` なら `{GRUP,0,label,6,0}`（bit10 解決の時は label 不一致でも可）／**kind 3,5**（interior/exterior sub-block）: `CA640(edi)==parentDesc[2]` なら `{GRUP,0,label,6,0}`／**kind 2,4**: ヘッダを作らない／**kind 6**（cell children）: `parentDesc[2]==edi->+0xC` のとき **`FUN_004DB4A0(this)!=0 → groupType 8`**、else **`this->flags bit15(0x8000) → 10`**、else **`9`**。label はいずれも `edi->+0xC`。→ **groupType 10 の生成 site 確定**（v531 の [未確定] 解消）。 |
| **V-532-1: `FUN_004DB4A0` の意味** | **[確定: 命令]／[推定: 名称]「保存セッション中判定」** | `FUN_0046B680(-1)!=0 && TLS[+0x184]==0 && ((FUN_0045A500()==0 \|\| manager+0x18 bit4) && DAT_00b33a98+0xCD4==0 && FUN_0044FAA0()==0)` なら **1**、else `this->flags >> 10 & 0xFFFFFF01`（実質 flags bit10 のマスク値）。REFR の配置追記（8）か恒久（9/10）かのゲート。 |
| **V-532-1: `FUN_00451580`（loader の groupType 8 処理）の本体** | **[確定]（命令＋docs）** | `if (file+0x23c == 'GRUP' packed) { file+0x23c = 0; file+0x240 -= 0x14; FUN_00451310(); }` ＝**読み込み中に開きかけの GRUP ヘッダをクローズ**（ヘッダ 20B 分を消費戻し＋後続処理）。8 専用の特殊解析ではなく**グループ終了処理**。caller: `0x4478B0`／`0x44F0C0`（loader）。 |
| **V-532-2: groupType 7 の label** | **[確定]（builder `FUN_005307D0` 実読）→ v531 の「`FUN_00530720` が 7 を組む」は誤り → 訂正** | **TESTopicInfo vtable `0xA5568C` の `+0xC0` = `FUN_005307D0`** が生成: 親 descriptor が `groupType==0 && label==DAT_00b060b4`（**packed `'DIAL'`**＝表 index 57）かつ **`FUN_0052F690(self) != 0`**（ロード中の TESTopic リストに所有者が存在）のとき `{GRUP, 0, label = 所有者 TESTopic の formID, 7, 0}`。→ **groupType 7 = topic children（label＝親 TESTopic の formID、`'DIAL'` top グループの下）**。 |
| **V-532-2: `FUN_0052F690`** | **[確定]（命令）「TESTopicInfo を保持する TESTopic を返す」** | `DAT_00b33a98+0x7c`（実行時リスト）を歩き、各 topic の `FUN_0052F5D0(topic->+0xC, 0)` が self に一致すればその topic を返す（一致無 0）。 |
| **V-532-2: TESTopic/TESTopicInfo/TESQuest の override 一覧** | **[確定]（RTTI 逆引き・v532_4）** | **TESQuest vtable `0xA541EC`＝`+0x20/+0x2C/+0x30/+0x34/+0xBC/+0xC0` すべて TESForm 基底**（QUEST は top-level 専用、groupType 0 の `'QUST'` グループに type<61 の型が入る構造）。**TESTopic `0xA5555C`**: `+0x20=FUN_0052EDB0`（SaveRecord override・`FUN_00450A10` 呼び）、`+0x2C=FUN_0052EC60`、`+0x30=FUN_0052EC70`、`+0x34=FUN_0052FBA0`、`+0xBC/+0xC0`＝基底。 **TESTopicInfo `0xA5568C`**: `+0x20/+0x2C`＝基底、`+0x30=FUN_0052EC70`（TOPIC と共有）、`+0x34=FUN_00530720`、`+0xBC=FUN_00530360`、`+0xC0=FUN_005307D0`。 |
| **V-532-2: `FUN_00530720`（TESTopicInfo `+0x34`）の意味** | **[確定]（命令）所属検証の委譲スイッチ** | `switch(subject->type - 0x39)`: **0x39=DIAL** → TESTopic TD `0xB05390` で label form を検索→`FUN_0052F690(this)`（自分の所有者 topic）がその form と一致したら**所有者の `vtable[+0x34](topic)` へ委譲**／**0x3A=INFO** → TD `0xB0544C` 型照合後、両者の所有者 topic を比較し所有者の `+0x34` へ委譲／else → **基底 `FUN_0046B500`**（`this->type < subject->type`）。 |
| **V-532-2: other builders の groupType 生成** | **[確定]（命令）** | **TESRoad `+0xC0`（`FUN_004E8BA0`）**: 親 descriptor `groupType==0 && label==packed 'WRLD'` → `{GRUP,0,label=[this+0x2c]（worldspace）formID, **1**, 0}`（Road も world children を開く）。**TESRoad `+0xBC`（`FUN_004E8B30`）**: arg2!=0 なら worldspace の `+0xBC` へ委譲（true であれば返す）、else groupType 1 かつ `FUN_0046AF50(label)`（worldspace formID==label）→ true。**LAND/PathGrid `+0xC0`（`FUN_004E4E00`）**: 親 kind 3,5 かつ `label==CA640(cell(this+0x20))` → `{GRUP,0,cell formID, **6**,0}`；kind 4 かつ `label==cell formID` → `{GRUP,0,cell formID, **9**,0}`。**LAND/PathGrid `+0x30`（`FUN_004E4D00`）**: `'GRUP'`；groupType 8..10 は label form（CELL 検索）==`this+0x20` で**伪**、他は cell の `+0x30` へ末尾委譲。**LAND/PathGrid `+0xBC`（`FUN_004E4D70`）**: 6/9 は `FUN_0046AF50(label)`==cell formID（6 は arg2==0 時）、8/10 は伪、0..5,7 は cell の `+0xBC` へ委譲。 |
| **追加 [確定] `FUN_0046AF50`** | **[確定]（命令）「label == 自分の formID（下24bit）」** | `((this->+0xC) ^ label) & 0xFFFFFF == 0` を返す 1 引数 thiscall。CELL `+0xBC` case1・Road `+0xBC`・LAND `+0xBC` が使用。 |
| **追加 [確定] WRLD `+0x34`（`FUN_004EF3E0`）** | **[確定]（命令）** | `switch(subject->type - 0x30)`: **0x30=CELL** → `FUN_004C9CF0(cell) == this` で block 帰属判定／**0x35=WRLD** → **`this->+0xC < subject->+0xC` のとき true**（formID 順序）／**0x37** → TESTerrainLODQuadRoot（TD `0xB0966C`）検索 `found->+4->+0x10 == this`／**0x38** → TESRoad（TD `0xB09654`）検索（同型）／**0x31..0x34,0x36** → **TESChildCell（TD `0xB08968`）検索→owner へ `vtable[+0x34]` 委譲**（CELL の `+0x34` 既定分岐と同型）。 |
| **V-532-3: `FUN_004CE0A0`（CELL `+0xBC`）の分岐** | **[確定]（命令）／保存ループとの引数対応は [部分確定]** | `switch(header[3])`: **case0**: `arg2==0→伪`；interior → 基底 `FUN_0046B910(header,arg2,arg3)`（top `'CELL'`）、exterior → worldspace(`+0x50`) の `+0xBC` へ委譲。**case1**: interior→伪；`FUN_0046AF50(label)`（label==自分の formID）必要；`arg2==0` なら **flags bit10 を返す**、arg2!=0 なら true。**case2,4**: flags bit10→伪、`arg2==0`→伪、`CA5F0(this)==label`→true。**case3,5**: 同様に `CA640(this)==label`。**case6..**: default 伪。→ `FUN_004502C0` の `(node,1,1)`（位置特定）/`(node,0,1)`（閉じる条件）との厳密な実行順追跡は残点。 |

## §2 証拠

### §2.1 `FUN_004E0000` 命令列（核心）
```
4E0023: edi = (*(this+0x18)->vtable[0])(&this+0x18)   ; 所属コンテナ解決
4E0033: ecx = edi->+0xC（label 候補）→ [esp+0x18]
4E0030: eax = parentDesc[0xc]（親 groupType） -1 ; >5 ならヘッダ無し
jump table 0x4E00CC: kind1->4E004E kind2->4E00C5 kind3->4E005E kind4->4E00C5 kind5->4E005E kind6->4E0081
4E004E(kind1): al = edi->vtable[+0x78]()  ; bit10 getter（FUN_004D7030）
               al!=0 → [esp+0x14]=1（inBlock）
4E005E(kind3/5 と共通): eax = CA640(edi) ; parentDesc[8]==eax → 4E0071:
               {GRUP(4E0071), [esi+c]=6(4E0078), label=edi formID(4E00C2)}
               不一致でも inBlock!=0 → 4E0071 へ（kind1 のみ）
4E0081(kind6): parentDesc[8] == edi formID でないなら伪
               call FUN_004DB4A0(this) ; al!=0 → [esi+c]=8(4E0099)
               else eax = (flags & 0x8000)!=0 ? 1 : 0; [esi+c] = eax+9 → 9 or 10(4E00B0)
               共通: [esi]=GRUP [esi+8]=edi formID [esi+4]=[esi+0x10]=0
```

### §2.2 `FUN_005307D0`（TESTopicInfo `+0xC0`）
```
5307E7: parentDesc[0xc] != 0 → 何もしない
5307F0: parentDesc[8] != DAT_00b060b4（packed 'DIAL'） → 何もしない
5307F9: eax = FUN_0052F690(this) ; ==0 → 何もしない
53080B: [esi]='GRUP' ; 53080D: [esi+0xc]=7 ; 530814: [esi+8]=eax->+0xC（topic formID）
```
`DAT_00b060b4`: 表 index 57 の f1（`0xB05E04+12*57+4`）＝ packed `'DIAL'`。

### §2.3 v532_4（TD→COL→vtable 機械逆引き）で確定した vtable 群
```
TESQuest      0xA541EC: +0x20..+0xC0 すべて基底（0x46B340/0x46B4C0/0x46B510/0x46B500/0x46B910/0x46B950）
TESTopic      0xA5555C: +0x20=52EDB0* +0x2C=52EC60* +0x30=52EC70* +0x34=52FBA0* +0xBC/+0xC0=基底
TESTopicInfo  0xA5568C: +0x30=52EC70* +0x34=530720* +0xBC=530360* +0xC0=5307D0*
TESObjectREFR 0xA46C44: +0x30=4DFE70* +0x34=4DFD10* +0xBC=4DFF40* +0xC0=4E0000*
TESWorldSpace 0xA4829C: +0x30=4EEF70* +0x34=4EF3E0* +0xBC/+0xC0=基底
TESObjectCELL 0xA469D4: +0x20=4D3460* +0x2C=4D3570* +0x30=4CDEC0* +0x34=4CDC90* +0xBC=4CE0A0* +0xC0=4CE1A0*
TESObjectLAND 0xA46064: +0x30=4E4D00* +0x34=4C00B0* +0xBC=4E4D70* +0xC0=4E4E00*
TESPathGrid   0xA47644: +0x30=4E4D00* +0x34=4E4C20* +0xBC=4E4D70* +0xC0=4E4E00*
TESRoad       0xA4788C: +0x30=4E8AF0* +0x34=4E89F0* +0xBC=4E8B30* +0xC0=4E8BA0*
Actor         0xA6E074: +0x30=4DFE70* +0x34=4DFD10* +0xBC=4DFF40* +0xC0=4E0000*（REFR と同一実装）
```

### §2.4 `FUN_0046AF50`（生バイト）
```
46AF50: mov eax,[ecx+0xc] ; xor eax,[esp+4] ; test eax,0xffffff ; sete al ; ret 4
```

### §2.5 `FUN_00451580`（docs L62253）
```c
if (*(int *)(file + 0x23c) == DAT_00b05e20) {      // 開いているヘッダが GRUP
  *(undefined4 *)(file + 0x23c) = 0;               // ヘッダクリア
  *(int *)(file + 0x240) += -0x14;                 // グループヘッダ 20B を巻き戻し消費
  FUN_00451310();                                   // 次ヘッダ読込処理
}
```

## §3 判定の内訳

- **V-532-1 完遂**: `FUN_004E0000` の 6 分岐を jump table 実読で確定。**6=cell children は親が sub-block（kind3/5）または world children＋bit10 のとき**、**8/9/10=親が cell children（kind6）のときの REFR 追記種別**（8: `FUN_004DB4A0` 真＝保存セッション系、9: 恒久配置、10: flags bit15 立上の配置）。v531 の [未確定] だった 10 の生成が確定。`FUN_00451580` は「8 の特殊解析」でなく**ロード側 GRUP クローズ**。
- **V-532-2 完遂**: groupType 7 = topic children（label＝TESTopic formID、`'DIAL'` top 直下）と確定（builder `FUN_005307D0`）。QUEST は基底のみの top-level 型と確定。Road が 1 を、LAND/PathGrid が 6/9 を生成することも命令で確定。
- **V-532-3 部分完遂**: `FUN_004CE0A0` の各 case の条件（`FUN_0046AF50`／CA5F0/CA640／arg2 の扱い）は確定したが、保存ループ `FUN_004502C0` が `(node,1,1)`/`(node,0,1)` を呼ぶ厳密な実行順は追跡継続（残点 V-533-1）。

## §4 残点（V-533-1..3）

1. `FUN_00451310`（`FUN_00451580` が呼ぶ次ヘッダ処理）本体と、ロード側 GRUP スタック（`file+0x23c/+0x240/+0x244`）の完全な開閉状態機械【中】
2. `FUN_004DB4A0` ゲート trio（`FUN_0046B680`／`FUN_0044FAA0`／`FUN_0045A500`）の identity 確定【中】
3. TESTopic `+0x34`＝`FUN_0052FBA0` 本体と、実行時 topic リスト `DAT_00b33a98+0x7c` の登録／解除 site（`FUN_0052F5D0` 系）【低】

## §5 後続への申し送り

- **移植側は groupType を「6/8/9/10 の選び分け」として実装する必要がある**: REFR 保存時に既に開いているセル子グループ（6）へ入るのか、追記グループ（8）／恒久グループ（9）／bit15 系（10）なのかを `FUN_004DB4A0` 相当ゲートと flags bit15 で決める。v531 の「interior bit で 6/8」は誤り（正: 6 は親 kind 1/3/5、8/9/10 は親 kind 6）。
- **groupType 7 は `'DIAL'` top の下のみ**（TESTopicInfo の `+0xC0` が、親が DIAL top でかつロード中の topic リストに自分が居る時のみ topic 子グループを開く）。DIAL/QUST/INFO の型 index は 57/61/58 で、基底 predicate の `case 7: type<0x39`（DIAL 未満）と組み合わせて検証できる。
- **ロード側のグループ close（`FUN_00451580`）は「GRUP ヘッダ 20B を巻き戻して閉じる」だけ**。保存側（v530 `FUN_00450250`）と対。
- **`FUN_0046AF50` は 4CC 判定ではなく formID 下24bit 一致判定**（v531 残点での「label が 'WRLD' 4CC か」仮説は却下）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
