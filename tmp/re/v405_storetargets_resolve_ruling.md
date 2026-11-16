# v405 — StoreTargets 解決系4件の裁定（#154・V-371-1/4/5 貫通）

- 採番: **v405**（ai-todo #154 専用予約・probe prefix **v405_1..v405_18 の18本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）＋ `tmp/nif_oracle/orig_src/nif_parser.cpp`（読み取りのみ・実データ KF/NIF パーサ＝oracle）
- 依頼経緯: ai-todo #154 の4件（①FindChild 系 `virtual+0x4C` の探索範囲【最優先】／②`0x6C9B12` の identity と +8 の port 対応／③predicate `virtual+0x8C`／④root と `[seq+0x5C]`/`[+0x60]` 正体）。出発点は v371（§2.2 A..E・§3 残点 V-371-1/4/5）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c 行番号・NIF oracle のみ（推測禁止）。

---

## §1 裁定サマリ

| # | 問い | 判定 | 核心 |
|---|---|---|---|
| **①** | `virtual+0x4C(root, name)` の探索範囲（子のみか孫再帰か・空名・自分自身） | **[確定]（実体）＋ v371 推定を訂正** | **root ＝ `NiDefaultAVObjectPalette`、slot19 impl `0x6C5430` ＝ `NiTMapBase<..., const char*, NiAVObject*>::Lookup(name,&out)` ラップ（見つからなければ0）** = **ハッシュマップ lookup であって構造探索ではない** → **子/孫再帰の概念なし [確定(否定側)]**・空名の特別扱いなし [確定(否定側)]・lookup 内の自己比較なし [確定(否定側)]（ただし palette には **Target 自身の名前も登録**される＝下記）。**実効的な「探索範囲」＝palette の内容**で決まり、遅延生成パスは **Target 自身＋子孫全ノード（孫以下まで）を再帰登録** [確定(命令)]、KF 読込パスはファイル記載の (name, AVObject) 対 [確定] |
| **②** | `0x6C9B12: mov [esi+eax+8], edi` の identity ＋ +8 が port `target_node_index`/`resolved_bone_index`/`keyframe_data_index` のどれか | **[部分確定]**（実体=[確定]・port 対応=[推定高]） | **esi＝i×16・eax＝`[seq+0x14]`（配列A base）・edi＝解決済み controller/keyframe data オブジェクト**（既存 registry lookup `FUN_00560920(&DAT_00b3cc5c, uVar7)` or `chosen->slot38(...)` 新規作成＋attach）。**+8 ＝ data オブジェクトの生ポインタ [確定]**。port 3候補のうち **keyframe_data_index に相当 [推定高]**（`target_node_index` でない＝解決 node は piStack_20 経由で transient、`resolved_bone_index` でない＝StoreTargets に骨テーブル照合なし＝v371 既報 [確定(否定側)]）。別パス `FUN_006c6910` は **同 +8 へ `[global(0xB3CD7C)+0x3C]+idx*48`（stride-48 配列要素の生ポインタ）** を格納 [確定] |
| **③** | predicate `virtual+0x8C` — 何が false になるか | **[部分確定]** | receiver ＝ **配列A[+0] ＝ NIF `Interpolator:Ref` オブジェクト** [確定]。impl ＝ **`0x6FDFF0: mov al,1; ret 4`（恒真・引数 palette は無視）**、**KF 実使用 interpolator 22/22 クラスが base 共通で override 0件** [確定(命令＋RTTI全数)] → **false を返す既知クラスなし → 'failed to resolve dependencies' ログ（0xA79CD8）経路は実質到達不能 [部分確定]**（未知クラスの存在だけが残る） |
| **④** | root 正体と `[seq+0x5C]`/`[seq+0x60]` | **[確定]** | **`[seq+0x40]` ＝ `NiControllerManager`**（NIF `Manager:Ptr`・version ≥ 0x0A010068 で存在・LoadBinary idx8=`FUN_006c7ad0` が stream object を格納）／**`manager[+0x7C]` ＝ `NiDefaultAVObjectPalette`**（NIF `Object Palette:Ref` 最終フィールド・遅延生成＝manager slot9 `FUN_006c5ab0`）／**`[seq+0x5c]` ＝ NIF `Accum Root Name`（string・port 呼称 target_name）**／**`[seq+0x60]` ＝ `palette->GetAVObject(AccumRootName)` の結果＝解決済み accumulator root の `NiAVObject*`** |
| 補1 | slot19 呼び出し箇所（同一 fn） | **[確定]** | 全4箇所（L498563 前処理A／L498620 ループ／L498821 "Bip02"／L498848-49 "Bip02"+"Bip01"）の receiver は **`local_24 = root`（palette）** — `FUN_006c9590`（single fn・L498520..498857）内 |
| 補2 | 配列A/配列B の entry レイアウト | **[確定]** | **配列A `[seq+0x14]` stride16**: `{+0=Interpolator*(NIF Interpolator:Ref) / +4=NiPointer(Controller・NIF Controller:Ref) / +8=data*(本件②) / +0xc=flags byte(既定 [0xA79EFC]=0xFF) / +0xd=priority byte(0xFF=なし)}` — ctor `0x6C62E0`（v401）＋ LoadBinary `FUN_006c8820`（2本の ref 読込＋priority byte 読込）＋ StoreTargets 書込の**3系統一致**。**配列B `[seq+0x18]` stride16**: `{+0=NiStringPalette* / +4,+6,+8,+0xa,+0xc=5×u16 palette offset(0xFFFF=なし)}` — `FUN_006c8a60` が5 string→u16 変換、StoreTargets の5識別子（m_pcAVObjectName/PropertyType/CtlrType/CtlrID/InterpolatorID）がこれに対応 |

---

## §2 証拠

### §2.1 ① root ＝ NiDefaultAVObjectPalette・slot19 ＝ hashmap lookup（v405_1/2/3/4/7/11..）

**呼び出し側（4箇所すべて同一 receiver）** — machine:
```
6C9596: eax=[seq+0x40] → edi=[eax+0x7c]     ; root
6C95AC: mov edx,[edi]; push eax(name); mov eax,[edx+0x4c]; mov ecx,edi; call eax → [seq+0x60]
```
- decompiled L498559-564（前処理A）／L498620（ループ・node 解決）／L498821（`"Bip02"`）／L498848-49（`"Bip02"` と `&DAT_00a30968`＝**"Bip01"**（v405_3 実読））— **全件 `local_24` = root**。

**root の正体**（§2.4 で接続）:
- `root = *(*(seq+0x40) + 0x7C)`、`seq[+0x40]` ＝ NiControllerManager、`manager[+0x7C]` ＝ palette。
- **vtable `0xA798C4` の RTTI ＝ `.?AVNiDefaultAVObjectPalette@@`**（v405_7(a)）— `FUN_006c5360` が生成する object（vptr 0xA798C4・member +8 ＝ vtable `0xA7973C`＝**`.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PBDPAVNiAVObject@@@@`（`const char*`→`NiAVObject*` map）**・bucket 37(0x25)・palette[+0x1C]=arg）。

**slot19(+0x4C) impl ＝ `0x6C5430`（v405_7(a) decode）**:
```
6C5430: push ecx
6C5431: mov edx, [esp+8]          ; name (arg)
6C5435: lea eax, [esp]            ; &out
6C5438: push eax
6C5439: push edx
6C543A: add ecx, 8                ; this+8 = the map
6C5445: call 0x55e000             ; map Lookup(name, &out) -> bool
6C544A: neg al / sbb eax,eax
6C544E: and eax, [esp]            ; found ? out : 0
6C5451: pop ecx
6C5452: ret 4
```
→ **NiDefaultAVObjectPalette::GetAVObject(name)** [確定]。**lookup は構造（子/孫）を走査しない** → 探索範囲の概念が存在しない [確定(否定側)]。空名は通常キー [確定(否定側)]。自己比較なし [確定(否定側)]。

**palette の内容（＝実効探索範囲）— 2経路（v405_16/17）**:
1. **KF stream 読込 `0x7166B0`**: `palette[+0x1C] = ReadRef(Target)` → count → loop `{ name=ReadString(0x713620), obj=ReadRef(0x712a90), palette->virtual+0x50(name,obj) }`（insert）＝**ファイル記載の対** [確定] — nif_parser oracle L1972-1978 の `NiDefaultAVObjectPalette` → `skipAVObjectArray(numObjs)`（"AVObject: Name | AV Object" L1138）と一致。
2. **遅延生成（manager slot9 `FUN_006c5ab0` → `FUN_006c5360(manager[+0x30])` → `FUN_00716690` → `FUN_007165b0(target, palette)`）の歩行 `FUN_007165b0`**（v405_17）:
```
if (node && palette) {
    if (node->m_kName[+8]) palette->virtual+0x50(name, node);   // node 自身を insert
    childRoot = node->virtual+8();                               // slot2
    for (i = 0; i < childRoot->count[+0xb6]; i++)
        FUN_007165b0(childRoot->children[+0xb0][i], palette);    // ★自己再帰＝子孫全ノード
}
```
→ **Target 自身＋子・孫以下の全ノードを登録**（自己呼び出し `call 0x7165b0`・NiNode children [+0xb0]/count [+0xb6]）[確定(命令)]。**「子のみ」は否定**。したがって **"自分の名前"（Target の名前）も palette に存在**する。

**v371 §2.2-② の「`virtual+0x4C(root,name)` ＝FindChild 系 [推定高]」は本裁定で palette lookup に訂正**。

### §2.2 ② `0x6C9B12` identity と +8（v405_9/11/12/13/18）

**machine（0x6C9AF5..0x6C9B12・v405_9(b)）**:
```
6C9AF5: or word [esi+8], 0x20            ; piVar19 の word[+8] |= 0x20（flag）
6C9AFA: mov ebx, [esp+0x14]              ; seq
6C9AFE: mov edx, [ebx+0x14]              ; 配列A base
6C9B01: push esi                          ; piVar19（controller 候補）
6C9B02: mov esi, [esp+0x24]              ; i*16（ループ index×stride16）
6C9B06: lea ecx, [esi+edx+4]             ; &entry+4
6C9B0A: call 0x75fa90                    ; entry+4 NiPointer = piVar19（ref 付き代入）
6C9B0F: mov eax, [ebx+0x14]
6C9B12: mov [esi+eax+8], edi             ; ★entry+8 = edi
```
- decompiled 相当 **L498806-808**: `*(ushort*)(piVar19+2) |= 0x20; FUN_0075fa90(piVar19); *(seq+0x14 + i*16 + 8) = iVar13`。

**edi = iVar13 の生成（L498791-498802）**:
- `uVar7 = chosen->slot32(sVar6)` → ①`iVar13 = FUN_00560920(&DAT_00b3cc5c, uVar7)`（既存 data の registry lookup・0 でなければ使用）②0 なら `iVar13 = chosen->slot38(sVar6, 1, uStack_14, 0, 2)`（新規 data 作成）→ `chosen->slot33(iVar13, sVar6)`（attach・0 なら fail ログ）→ **L498808 で entry+8 へ格納**。

**格納物の補証（v405_6(c)・FUN_006c6910 ＝ 同 seq の別解決パス）**:
```
6C6A31: movzx ecx, cx ; idx
6C6A34: lea ecx, [ecx+ecx*2] ; ×3
6C6A39: shl ecx, 4 ; ×16 → idx*48
6C6A3C: add ecx, [esi+0x3c] ; global(FUN_00700010(&DAT_00b3cd7c))[+0x3c] 基点
6C6A3F: mov [eax+8], ecx ; entry+8 = stride-48 配列要素の生ポインタ
```
- **entry+8 ＝ data オブジェクトの生ポインタ [確定]**（既存配列要素 or 新規作成物）。
- 既解決チェック `L498613: else if (entry+8 == 0)` ＝「data 未解決なら node 解決へ」[確定]。
- **port 対応**: `keyframe_data_index`（配列 index）に相当 [推定高] — index は `(ptr − (global+0x3C))/48` で復元可能。**`target_node_index` [確定(否定側)]**（解決 node ＝ piStack_20 は transient・+8 には node が入らない）・**`resolved_bone_index` [確定(否定側)]**（StoreTargets decode 窓に骨テーブル照合なし＝v371 既報と整合）。

**entry レイアウトの3系統一致（v405_11/12/13）**:
- ctor `LAB_006c62e0`（v401 で CRT iterator と確定済）: `{+0=0, +4=0, +8=0, +0xc←[0xA79EFC], +0xd=0xFF}`。
- LoadBinary `FUN_006c8820`（ecx=entry・arg=stream）: `entry+0 = ReadRef(Interpolator:Ref)`、`entry+4 = ReadRef(Controller:Ref)`、version<0x0A01006F で u16 読み、`entry+0xd = priority byte 読み` — **nif_parser oracle の ControlledBlock 順（Interpolator | Controller | Priority | ...）と完全一致** [確定]。
- 配列B `FUN_006c8a60`（ecx=entry+0x18 系）: 5×(string→`FUN_006c6270`→u16) を `+4/+6/+8/+0xA/+0xC` へ（0xFFFF=なし）＝**5 palette offset** [確定]。

### §2.3 ③ predicate `virtual+0x8C`（v405_13/14）

- receiver: `L498573-577: piVar2 = *(i*16 + seq+0x14); cVar5 = piVar2->virtual+0x8C(local_24)` — piVar2 ＝ **配列A[+0] ＝ Interpolator** [確定]。arg ＝ `local_24` ＝ palette（§2.4）。
- **impl ＝ `0x6FDFF0: mov al, 1 / ret 4`（恒真）** — `NiInterpolator`（vt 0xA7C6BC）の idx35。
- **RTTI→vtable→idx35 の全数調査（v405_14）: KF 実使用 22 クラスすべて idx35 = `0x6FDFF0`（override 0件）**: NiTransform / NiFloat / NiBool / NiPoint3 / NiColor / NiQuaternion / NiBlendFloat / NiBlendBool / NiBlendTransform / NiBlendPoint3 / NiBlendColor / NiBlendQuaternion / NiBoolTimeline / NiPath / NiBSpline / NiBSplineTransform / NiBSplineCompTransform / NiBSplinePoint3 / NiBSplineCompPoint3 / NiBSplineFloat / NiBSplineCompFloat（＋base）。nif_parser oracle L1029-1048（人馬＝NiBSplineCompTransformInterpolator／クリーチャー＝NiTransformInterpolator）と照合済。
- → **false を返す既知クラスなし [確定(命令＋RTTI 22/22)]** → `L498578 if (cVar5=='\0') { 'failed to resolve dependencies' }` 経路は **実質到達不能 [部分確定]**（未知 interpolator クラスのみ残る）。
- 参考: controller 系（NiSingleInterpController `0xA7A08C` / NiTransformController `0xA79634` / NiInterpController `0xA7A16C`）の idx35 ＝ `0x6F7070: xor eax,eax; ret`（恒0）だが **predicate の receiver ではない**。
- **v371 の「controller 側 vtable slot」表記は receiver 間違え（interpolator 側）**と訂正。

### §2.4 ④ root / `[seq+0x5c]` / `[seq+0x60]`（v405_3/5/6/7/8 + NIF oracle）

- **`seq[+0x40]` = NiControllerManager [確定(命令＋oracle)]**:
  - NIF oracle `nif_parser.cpp:3271-3285`: `NiControllerSequence layout: Name | Num Controlled Blocks | Array Grow By | Controlled Blocks[N] | Weight | Text Keys:Ref | Cycle Type | Frequency | Start Time | Stop Time | **Manager:Ptr(4)** | **Accum Root Name:string** | **String Palette:Ref(4)**`。
  - LoadBinary（**sequence vtable idx8 = `FUN_006c7ad0`**（RTTI vtable 0xA79A34 / BSAnimGroupSequence 0xA3F7D4 両方に格納＝override））: `cmp [stream+0xD8], 0x0A010068; jae` → 新形式で **`seq[+0x20] = ReadRef`（TextKeys）→ `seq[+0x40] = ReadRef`（manager）**（machine `0x6C7B8F..0x6C7BA4: mov [ebp+0x40], eax`）、旧形式は per-block ref を entry+4 へ（L497431-497462）。
  - IsEqual（**idx11 = `FUN_006c7bb0`**）が `seq[+0x40]` を **virtual+0x2C（slot11＝IsEqual）** で対照比較（machine 0x6C7D03-6C7D13）＝+0x40 は NiObject 系 ptr。
- **`manager[+0x7C]` = NiDefaultAVObjectPalette [確定]**:
  - NIF oracle `nif_parser.cpp:3186-3221`: `NiControllerManager layout: NiTimeController(...) | Cumulative | Num Controller Sequences | Controller Sequences:Ref[N] | **Object Palette:Ref(4)** ← last field`。
  - EXE: **manager vtable 0xA79804 idx9(+0x24) = `FUN_006c5ab0`**（RTTI 突合 v405_8(a)）が `if (this[+0x7C]==0) this[+0x7C] = FUN_006c5360(this[+0x30])`（L495423-495444）＝palette の遅延生成・保持。
- **`[seq+0x5c]` = Accum Root Name（string）[確定(命令＋oracle)]**:
  - LoadBinary `FUN_00713620(&seq[+0x5c])` による stream string 読込（L498234 / L498415・FUN_00713620 は `stream::ReadString(out)`（v405_11(d) の実 call 形式））。
  - IsEqual で strcmp（machine 0x6C7D19-0x6C7D67）＝文字列フィールド。
  - NIF の `Manager:Ptr` 直後の唯一の string（Accum Root Name）とフィールド順・IsEqual 比較順が整合。**port は同位置を `sequence.targetName`（nif_parser.cpp:3326）と呼称** ＝ port の `target_name` は EXE の `[+0x5c]`（AccumRootName）に対応。
- **`[seq+0x60]` = `palette->GetAVObject(AccumRootName)` の結果 [確定]**: L498562-564 `if (seq[+0x5c]) seq[+0x60] = root->slot19(seq[+0x5c])`（machine 0x6C95AC-0x6C95B6）＝**解決済み accumulator root の `NiAVObject*`**。
- **`manager[+0x30]` [部分確定]**: palette 遅延生成の引数（`FUN_006c5360(*(param_1+0x30)`（L495430）が palette[+0x1C] に保管（L494985）→ `FUN_007165b0` の歩行起点）＝**node 系オブジェクト**、L498741 でも解決対象と比較される → NIF `Target:Ptr` 対応 [推定]。

**StoreTargets の全体像（副次整理・一部 [部分確定]）**: 前処理A（AccumRootName→root/palette lookup→[seq+0x60]）→ 前提 `FUN_006c8220`（"NiTransformController" 登録確認・v371）→ ループ（配列A/B 併走）: predicate(恒真) → `[entry+8]==0` なら palette lookup(m_pcAVObjectName) → 解決 node の `+0x9C` リストを名前一致で走査（L498625-651）→ `elem[3]` 連鎖の controller 群を名前一致で選択（L498657-707）→ 選択 controller が無ければ配列A[+4]（NIF Controller）を fallback（無ければ 'failed to find target' ログ L498734）→ data 作成/lookup して **entry+8 へ**（L498799-498808）→ epilogue: `stricmp(name,"Bip01")==0 && palette->Get("Bip02")` で bVar3（L498820-823）→ `(bVar3||Get("Bip02")) && Get("Bip01")[+0x1C]!=0` なら `FUN_006c6910(Get("Bip01")[+0x1C])`（L498848-51・v372 骨解決 epilogue と接続）。

## §3 残点 **V-405-1..5**

1. **【中】palette の stream 読込経路の manager 側接続** — `0x7166B0` は palette 自身の LoadBinary（vtable slot 未確認）。manager の stream LoadBinary（idx 未確認）が `Object Palette:Ref` を `[+0x7C]` へ格納する site の特定と、**KF 読込（ファイル記載対）と遅延生成（Target 部分木走査）の実行順序・優先関係**。
2. **【中】edi（新規 data）と stride-48 配列要素の型照合** — `slot38` 作成物・`FUN_00560920(&DAT_00b3cc5c)` registry・`global(0xB3CD7C)[+0x3c]+idx*48` の3者の同一型性、および **port `keyframe_data_index` への厳密な写像**（index↔ptr 変換の実データ検証）。＋ port `target_node_index` の供給元（V-371-2/V-371-3 と接続）。
3. **【中】`Get("Bip01")[+0x1C]` の identity** — `FUN_006c6910` は sequence layout（+0xc/+0x14/+0x18/+0x40→+0x7c）を this として読むため、**NiAVObject+0x1C に sequence 系 ptr が入る構造の説明**が必要。＋ L498845 `[manager+0x30]` deref の slot23(+0x5C) 呼び出しの意味。
4. **【低】palette insert 本体（virtual+0x50）と `FUN_00716620`** — 0x716620 は同一 walker 形で `insert(name, 0)`（NULL 登録）＝unregister 派生か。`FUN_0055E000`（Lookup）以外の insert/release 経路整理。
5. **【低】未知 interpolator クラスの idx35** — 22 クラスは KF 実使用ベース。＋ predicate slot35 の正式名（slot は class family で多義）。

## §4 検証・自己批評

- probe: **v405_1..v405_18 の18本全 EXIT=0**（本リポジトリの v 採番 v405 を消費）。
- **決定打の作り**: (i) 呼び出し側 machine で receiver を辿る → (ii) **書手（writer）から逆算**: `seq[+0x40]` の writer＝LoadBinary idx8、`[+0x7C]` の writer＝manager idx9 → **vtable slot の vtable 所属（vtable 中の位置＝slot index）から fn のクラスを確定** → (iii) **RTTI（COL→TypeDescriptor）でクラス名を実読** → (iv) impl decode → (v) **NIF oracle（実データ parser のフィールド順）と stream 読込順・IsEqual 比較順の突き合わせ**で offset↔NIF フィールド対応を確定 → (vi) predicate は **RTTI 全数調査（22 クラス）** で否定確認。
- 妥当性チェック: ①lookup impl の戻り値式（found?out:0）⇔ StoreTargets が `[seq+0x60]` に null 検査付きで使う — ②NIF 順（Manager→AccumRootName→StringPalette）⇔ EXE IsEqual 順（+0x40→+0x5c→+0x64）⇔ EXE stream 読込順 — ③entry 3 段（ctor/LoadBinary/StoreTargets）のフィールド位置一致 — ④oracle の ControlledBlock 順⇔`FUN_006c8820` 読込順 — **四方向で矛盾なし**。
- **当初の矛盾と解消**: 「NiNode 系 slot19＝float 伝播なので root は別クラス」という v405_2 時点の推論は正しく、**root は scene node ではなく palette だった**（slot の多義性）。
- docs との不整合: **v371 §2.2-②「FindChild 系 [推定高]」→ palette lookup に訂正**／v371 §3 **V-371-1（predicate）・V-371-4（結果格納）・V-371-5（root/target 名）は本裁定で消化**／v371 §1 ③「resolved_bone_index [未確定]」は本裁定で「StoreTargets は書かない」方向の裏付け（ただし AnimationPlayer 側の供給元は未特定＝V-405-2）。docs/GT への反映は #47/#20 の一括承認待ち。
- **教訓**:
  1. **vtable slot の意味は class family で多義** — 同じ +0x4C が「NiNode 系＝float 伝播」「NiDefaultAVObjectPalette＝名前 lookup」「TESForm 系＝FormType 比較」「NiTMap 系＝ハッシュ剰余」。**slot index から fn を推測せず、receiver のクラスを RTTI で確定してから impl を decode せよ**（「+0x4C＝FindChild」仮説の否定例）。
  2. **「root の正体」系は caller を遡るより writer から逆算が速い**（seq[+0x40] の writer → [+0x7C] の writer → vtable slot 所属 → RTTI 実読）。
  3. **NIF oracle（実 parser のフィールド順コメント）を EXE の stream 読込順・IsEqual 比較順と突き合わせる**と、offset↔NIF フィールド対応が推測なしで決まる。
  4. **「何が false になるか」系は base と全 subclass の RTTI 全数調査**で否定してから裁定（22/22 override 0件）。
