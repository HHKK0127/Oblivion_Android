# v289_extra_space_bundle.md — serial 278（extra 型空間後始末束・3件）

- 作成日: 2026-11-05 / 担当: 本セッション内実施（引継ぎ: サブエージェント投入 1 回 upstream error のため v277/v286 先例に従い inline 化。AGENTS.md ループガード準拠）
- 出典: tmp/re/v286_tail_payload_bundle.md 残点1..3（= 台帳 row278 起票文）
- 番号注記: 台帳 row278 予定名 `v287` は他セッション（v60v76-bundle・v287_v76_data_map_residuals.md）が先占済みのため正本 #40 注記（v289 以降へ振り分け推奨）に従い **v289** へ移す（v289=当時空き・v294/v295 は Copilot バンドル v2 予約）。
- 中間物: `%TEMP%\opencode\v278_i1..i10b.py/_out`（i1 は ident オフセットバグで ID 列 unusable、i2 以降が実測本体）
- 判定サマリ: **件1=確定（命名は[確定負]＋機構閉塞）/ 件2=確定（ID 71 件の完全再構築）/ 件3=確定（3 dword 固定・全サンプル網羅）**

## 件1 extra flags bit3|bit31 の命名裏取り — 【確定負（命名証拠ゼロ）＋機構[確定]】

### 1-1 実体訂正: 「extra エントリ flags」ではなく **TESForm ChangeFlags 台帳エントリ**

v286 の「entry+0 flags（extra エントリ自身の flags）」は機能記述として不正確だった。本束の命令実測で以下が確定:

- `FUN_00452C90(this, key, flags)` @0x452C90: `call 0x55E000`(find) → 不在時 `push 8; call 0x401F00`（**operator new(8)** → entry=**8B{+0 flags dword, +4 refcount dword}**・0x452CB8/CBA でゼロ初期化）→ `call 0x452570`(挿入) → **`mov [entry+0], flags引数`（無条件上書き）**。
- `FUN_00452CF0(key, flags, refcount)` @0x452CF0: 452C90 を呼び `mov [entry+4], 第3引数`。**キーは [node+0xC] = TESForm の FormID**（TESForm ヘッダレイアウト +0 vtbl/+4 formType/+8 flags/+C FormID）。 getter `FUN_00452D30` の全 caller で `push <form*>` → 内部 `mov eax,[edx+0xc]`（0x463AF6 で `cmp edi,[0xB333C4]`（PlayerCharacter 単例）ガード＝「プレイヤー自身は除外」の意味論も確定）。
- つまり entry flags dword = **セーブレコードから復元された form の changeFlags 値**（v273 の属性 bit 表 12..30/7/5 と同一 bit 空間）。bit3(0x8)/bit31(0x80000000) は v273 表の extra 型所有ビット（7,12..30）の**いずれにも対応しない**——これが高低位の「エンジン側のロード寿命制御ビット」対として clear される理由の構造根拠。

### 1-2 消去・除去経路（命令実測）

- `FUN_00452D60(nodeKey, mask)` 消去側（v286 と同一結論、再確認）: SLG+0x18 bit12 ロード中ゲート → find → **refcount[+4]==0 のときのみ** `not ecx; and [entry+0],~mask` → flags ゼロ化なら `4524C0`(map 除去)→pool 0xB02020 free→`call 0x401F20`(operator delete)。
- callers 3 点（E8 全数）: **0x461DD3=pass2 末尾（mask 静的=push 0x80000008）**／0x463A6E（**mask 動的=[esi+0x50] pending マスク**、call 後 `mov [esi+0x50],0`）／0x466766（dispatch 末尾・[ebp+0x50] 同型、clear 0x46676B）。
- `FUN_00452DF0(key, force)` 除去側: find 失敗→return 0／refcount==0→除去実行／refcount!=0 かつ force==0→保持 return 1／**force==1→refcount>0 でも除去**（`push eax` を pool 0xB02020 へ free 後 entry を operator delete）。callers 14 点: 0x459C4B/CB4/CFD（PurgeSavedDataForCell）・0x45C7E0・0x4606F7・0x46144A・0x461CD7（`cmp ebx,8` 後=**レコード種別 8 のみ force 除去**）・0x461E42・0x461E78（`cmp ebp,0x31`∧`test ebx,0x20000`）・0x4638E0・0x463B28・0x463F04・0x466755・0x46684C。**全 caller `push 1`（force=1）固定**。
- LoadGame dispatch（0x465860 系）@0x466576..: RTDynamicCast(0x9832E6・TESForm→TESForm=実体検証) 通過時、`movzx [ebx+4]`(formType)×3 索引テーブル **0xB05E04**（'CELL','REFR','ACHR',…型名3つ組）経由でログ `'Created form %08X with type %s is going to be reset due to flags: %08X. Header flags: %08X Current Flags: %08X'`@0xA3B230 を出力後、`452C90(key=[ebx+0xc], flags)`(+0x50 由来)→`452CF0(key,flags,refcount)`→`452DF0(key,1)` の順適用（0x466650/0x466830/0x466842）。末尾 @0x46675A: [ebp+0x50]!=0 なら 452D60 適用→0 クリア。関連エラー文字列 0xA3AFA0='LoadGame() call did not properly empty buffer…'／0xA3B088='SaveGame() call did not properly fill buffer.'（同末尾検証帯）。
- v273 との対照（依頼の「ChangeFlags 生成側 0x4535A0 帯との対照」）: 生成側 producer 群は `and 0x1FFFF080`（A=bit12..28+7）／`and 0x60000000`（B=bit29..30）で属性群を整列化 — **0x8/0x80000000 は producer 側値域に一切現れない**（v273 §1-2 表と完全相補）。消費側 0x425650 のビット別 RemoveExtra 窓にも 0x8/0x80000000 の検査なし（v273 §4 負証拠と整合）。

### 1-3 裁定

- **正式名[確定負]**: EXE 内文字列・enum・assert に bit3/bit31 の語彙証拠なし（文字列検査実施: 0xA3AFA0/0xA3B088/0xA3B230 の 3 本は機構説明のみ）。
- 語源候補（[推定]）: bit31 = 「ロード中生成/拡張フレーム系の上位セマンティクスビット（符号ビット領域＝エンジン予約）」、bit3 = 「同一レコードの生成中フラグ（種別 8=0x461CC5 経路で force 除去対象）」——両者とも**対（0x80000008）でしか観測されず、個別セット/テスト窓ゼロ**。
- 残点①（次束）: pending マスク [ctx+0x50] の**非ゼロ書手**が 0x45C000..0x467000 整列 walk に存在しない（0x463A73/0x46676B のゼロ化のみ）。書手は dispatch 外（0x45xxxx 以前か別帯）＝要広域探索。

## 件2 extra 型 ID 空間の統合表（v281 付録の完全再構築＋0x5A/0x5B 反映）— 【確定】

### 2-1 方法（v281 走査の欠陥修正）

クラス側から権威再構築: TD `.?AV(Extra|BSExtraData|NonActorMagic|BaseExtraList|ExtraDataList)*`（**83 本**）→ COL(`u32(col+0x0C)==TD`・この実体の COL は +0xC に TD)→ vft(`u32(vt-4)==col`)→ **.text 全域の `mov dword ptr [R], imm32(vft)` 整列実照合 141 件**→ 同一命令ウィンドウ内（st-24..st+48）で**同一レジスタ R** の `mov byte/dword ptr [R+4], ID` をバイトパターン一致（C6 4R 04 / C7 4R 04・capstone 空白フォーマットに依存しない）。
- 前走 i1 プローブは ident 読みが blob 実オフセット基準（+0x400 漏れ）で**全 ID 列が偽**、i2 は producer 側 window 方式で vft 誤紐付けあり——**統合表は 3c（クラス側・レジスタ整合）を正本**とし i2 と突合。

### 2-2 統合表（ID 0x03..0x5C・71 値／vft 住い＝.rdata 0xA34Dxx-0xA35Axx 帯・NonActorMagic のみ 0xA769xx）

| ID | クラス（RTTI） | ctor vft store | ID store |
|---|---|---|---|
| 0x03 | ExtraCell3D | 0x41D823 | 0x41D81C |
| 0x04 | ExtraWaterHeight | 0x41D934 | 0x41D929 |
| 0x05 | ExtraCellWaterType | 0x41D9AB | 0x41D9A4 |
| 0x08 | ExtraRegionList | 0x41D951 | 0x41D946 |
| 0x09 | ExtraSeenData | 0x41D9EB | 0x41D9E4 |
| 0x0A | ExtraEditorID | 0x424544 | 0x42453A |
| 0x0B | ExtraCellMusicType | 0x41D971 | 0x41D966 |
| 0x0C | ExtraCellClimate | 0x41D991 | 0x41D986 |
| 0x0D | ExtraProcessMiddleLow | 0x41D9CB | 0x41D9C4 |
| 0x0F | ExtraCellCanopyShadowMask | 0x41DCB4 | 0x41DCAD |
| 0x10 | ExtraDetachTime | 0x41DA8B | 0x41DA84 |
| 0x11 | ExtraPersistentCell | 0x42A2F1 | 0x42A2E6 |
| 0x12 | ExtraScript | 0x429F2F | 0x429F28 |
| 0x13 | ExtraAction | 0x429D3B | 0x429D34 |
| 0x14 | ExtraStartingPosition | 0x42AC7A | 0x42AC6B |
| 0x15 | ExtraAnim | 0x4298D1 | 0x4298C6 |
| 0x17 | ExtraUsedMarkers | 0x42A39B | 0x42A394 |
| 0x18 | ExtraDistantData | 0x42A4C7相当(fn 0x42A4C0) | 0x42A4CA |
| 0x19 | ExtraRagDollData | 0x42A30B | 0x42A304 |
| 0x1A | ExtraContainerChanges | 0x429D61 | 0x429D56 |
| 0x1B | ExtraWorn | 0x429FFD | 0x429FF2 |
| 0x1C | ExtraWornLeft | 0x42A03D | 0x42A032 |
| 0x1E | ExtraPackageStartLocation | 0x42AE83 | 0x42AE78 |
| 0x1F | ExtraPackage | 0x42A1CA | 0x42A1BF |
| 0x20 | ExtraTresPassPackage | 0x42A1F1 | 0x42A1E6 |
| 0x21 | ExtraRunOncePacks | 0x42A3BB | 0x42A3B4 |
| 0x22 | ExtraReferencePointer | 0x42A191 | 0x42A186 |
| 0x23 | ExtraFollower | 0x429B5D | 0x429B48 |
| 0x24 | ExtraLevCreaModifier | 0x42A6CB | 0x42A6C4 |
| 0x25 | ExtraGhost | 0x429FDD | 0x429FD2 |
| 0x26 | ExtraOriginalReference | 0x429DF1 | 0x429DE6 |
| 0x27 | ExtraOwnership | 0x429E11 | 0x429E06 |
| 0x28 | ExtraGlobal | 0x429E31 | 0x429E26 |
| 0x29 | ExtraRank | 0x429E51 | 0x429E46 |
| 0x2A | ExtraCount | 0x429E72 | 0x429E67 |
| 0x2B | ExtraHealth | 0x429E94 | 0x429E89 |
| 0x2C | ExtraUses | 0x429EB1 | 0x429EA6 |
| 0x2D | ExtraTimeLeft | 0x429ED4 | 0x429EC9 |
| 0x2E | ExtraCharge | 0x429EF4 | 0x429EE9 |
| 0x2F | ExtraSoul | 0x429F11 | 0x429F06 |
| 0x30 | ExtraLight | 0x429981 | 0x429976 |
| 0x31 | ExtraLock | 0x429A91 | 0x429A86 |
| 0x32 | ExtraTeleport | 0x429C21 | 0x429C16 |
| 0x33 | ExtraMapMarker | 0x429CB1 | 0x429CA6 |
| 0x35 | ExtraLeveledCreature | 0x429BFD | 0x429BF2 |
| 0x36 | ExtraLeveledItem | 0x42A2C1 | 0x42A2B6 |
| 0x37 | ExtraScale | 0x429FC4 | 0x429FB9 |
| 0x38 | ExtraSeed | 0x42A171 | 0x42A166 |
| 0x39 | NonActorMagicCaster | 0x6A300C/0x6A302E | 0x6A3012（base ctor 経由・BSExtraData vft store 直後） |
| 0x3A | NonActorMagicTarget | 0x6A333C/0x6A335B | 0x6A3342（同上） |
| 0x3D | ExtraCrimeGold | 0x42A2A4 | 0x42A299 |
| 0x3E | ExtraOblivionEntry | 0x42A51B | 0x42A514 |
| 0x3F | ExtraEnableStateParent | 0x42A5EB | 0x42A5E4 |
| 0x40 | ExtraEnableStateChildren | 0x42A60B | 0x42A604 |
| 0x41 | ExtraItemDropper | 0x42A84B | 0x42A844 |
| 0x42 | ExtraDroppedItemList | 0x42A86B | 0x42A864 |
| 0x43 | ExtraRandomTeleportMarker | 0x42A66B | 0x42A664 |
| 0x44 | ExtraMerchantContainer | 0x42A68B | 0x42A684 |
| 0x46 | ExtraPersuasionPercent | 0x42A6F5 | 0x42A6E7 |
| 0x47 | ExtraCannotWear | 0x42A05D | 0x42A052 |
| 0x48 | ExtraPoison | 0x42A711 | 0x42A706 |
| 0x4A | ExtraLastFinishedSequence | 0x42A762系 | 0x42A749 |
| 0x4B | ExtraSavedMovementData | 0x42A8CB | 0x42A8C4 |
| 0x4C | ExtraNorthRotation | 0x41DA72 | 0x41DA67 |
| 0x4D | ExtraXTarget | 0x42A82B | 0x42A824 |
| 0x4E | ExtraFriendHitList | 0x42A9BD | 0x42A9A8 |
| 0x4F | ExtraHeadingTarget | 0x42AB21 | 0x42AB16 |
| 0x50 | ExtraBoundArmor | 0x42A01D | 0x42A012 |
| 0x51 | ExtraRefractionProperty | 0x42AB44 | 0x42AB39 |
| 0x52 | ExtraInvestmentGold | 0x42A081 | 0x42A076 |
| 0x53 | ExtraStartingWorldOrCell | 0x42995B | 0x429954 |
| 0x55 | ExtraQuickKey | 0x42A0A1 | 0x42A096 |
| 0x57 | ExtraEditorRefMoveData | 0x42B0CB | 0x42B0BA |
| 0x58 | ExtraTravelHorse | 0x42A6AB | 0x42A6A4 |
| 0x59 | ExtraInfoGeneralTopic | 0x42A151 | 0x42A146 |
| **0x5A** | **ExtraHasNoRumors** | **0x42AB61** | **0x42AB56（C6 40 04 5A・vft 0xA35A4C）** |
| **0x5B** | **ExtraSound** | **0x42AB81** | **0x42AB76（C6 40 04 5B・vft 0xA35A58）** |
| 0x5C | ExtraHaggleAmount | 0x42AC14 | 0x42AC09 |

### 2-3 v281 付録との差分・訂正（本束の正本化により確定）

- **v286 の主張「0x5A/0x5B は v281 付録 56 空間表に未載」は 0x5B に対して誤り**: v281 §2-2 は `0x5B=ExtraSound` を**すでに掲載**。未載だったのは **0x5A のみ**（しかも 0x5A は穴リストにも無く台帳の穴あき不整合だった）。→ **付録差分は +1（0x5A）に訂正**。0x5B の再確認と ctor 実測値（v286 の C6 40 04 5B は fn 先頭ではなく +6 位置=0x42AB76・v286 の「0x42AB70=C6 40 04 5B」表記は関数先頭と格納位置の圧縮記述と裁定、実体は整合）。
- v281 §2-2 の**低帯欠落を新規反映**: 0x03=ExtraCell3D・0x04=ExtraWaterHeight・0x05=ExtraCellWaterType・0x08=ExtraRegionList・0x09=ExtraSeenData・0x17=ExtraUsedMarkers・0x1E=ExtraPackageStartLocation（0x17/0x1E は §2-1 の bit 表側にのみ掲載、§2-2 全域表は初載）。
- 穴（全 extra ctor 走査で ID ストア無しと再確認）: **0x00..0x02, 0x06, 0x07, 0x0E, 0x16, 0x1D, 0x34, 0x3B, 0x3C, 0x45, 0x49, 0x54, 0x56, 0x5D**（v281 の穴リスト + 低帯追加）。
- ID 非ストアの系譜クラス[確定]: BSExtraData(0xA34D50)・BaseExtraList(0xA34DEC)・ExtraDataList(0xA350F8)・**ExtraHavok(0xA34DD4)**・Extra@Tile(0xA6A164=UI 別 namespace)。うち **ExtraHavok は [this+4] ID ストアを持たない**（0x41DAF0/0x41DBB0 の両 ctor 変種で確認）→ 残点②（ランタイム ID の帰属: 0x99D9xx 帯の `mov dword [R+4], ID`＋BSExtraData vft ストア群が raw-keep 経路ならその可能性）。
- 0x39/0x3A の帰属: ID store は base ctor（BSExtraData vft store 後・derived vft 差し替え前）に在るため 3c 表上は BSExtraData サイトに紐づくが、関数帰属（0x6A2FE0/0x6A3310）は v281 の **NonActorMagicCaster/Target** と一致。帰属は v281 を維持。

## 件3 0x5B 以上 extra の 12B フレーム param 網羅（3 dword 固定の全サンプル確認）— 【確定】

v286 は 1 サンプル（pass2）のみ。本束で**書手 1+読手 2 の全経路**を網羅:

1. **書手（唯一）**: `452CF0` の E8 caller は **0x466829 のみ**（LoadGame dispatch・0x465860 系）。push 3 組 = `push ecx([esp+0x1c]) / push eax / push ebx`（0x466823-26）→ **key・flags・refcount の 3 値**。生成物=8B entry+レコード 12B（`<key><flags><refcount>`）。
2. **読手 1（PurgeSavedDataForCell @0x459AF0・sub≥0x5B 枝）**: 0x459C1B `mov esi,[eax+4]` → 0x459C24 `mov esi,[eax+8]` → **0x459C27 `add eax,0xc`** —— 遊標 advance が全経路 0xC 固定（+4/+8 Beyond の参照ゼロ・16/20B advance ゼロ）。
3. **読手 2（LoadGame pass2 @0x461680・0x461811 `cmp byte[esp+0x27],0x5b`）**: 通過時 3 dword（1st→459990 キー、2nd/3rd→movsx byte×2 の除去レコード）を消費（v286 実測と同一・再確認）。sub<0x5B は古典 4/6B（flags bit2/4・u16 formID）。
4. **.ess ランタイム突合は不能と確認**（`Documents\My Games\Oblivion\Saves` 存在せず・i1-D で MISSING）。静的全網羅で代替（上 3 経路が 12B フレームの生成・消費の全 E8 到達点）。
5. **境界の意味の再確認**: 12B 化するのは型 ID≥0x5B の extra **レコード形式**であり、上表の ≥0x5B クラスは 0x5B=ExtraSound と 0x5C=ExtraHaggleAmount の 2 クラスのみ（0x5D..=穴/不存在）。→「≥0x5B 全 extra が 12B」は**実在クラス 2 件の話**として正確化。

## docs 反映候補（+4 点・承認ゲート #20/#47 へ）

1. changeFlags bit3|bit31=ロード寿命対ビット（正式名なし・対運用のみ観測、extra 所有ビットと相補）
2. 8B entry{flags,refcount}・キー FormID([form+0xC])・new(8)@0x452CAA の ChangeFlags 台帳構造（v286「extra エントリ」呼称の訂正を伴う）
3. 452DF0=force 除去（全 caller push 1・refcount>0 でも除去・pass2 の種別 8 限定 0x461CC5）
4. ID 空間統合表 71 件（本 §2-2）＋穴リスト——v281 §2-2 の置換版として v289 を正典参照に
5. （据置）0x5B/0x5A=ExtraSound/ExtraHasNoRumors（v286 分と統合・付録差分 +1 訂正込み）

## 残点（serial 279 起票用・すべて低）

1. **pending マスク [ctx+0x50] の非ゼロ書手**: 0x45C000..0x467000 整列 walk はゼロ化（0x463A73/0x46676B）のみ。広域（.text 全域の `mov [reg+0x50], reg/imm` で ctx 型を絞る）探索。
2. **ExtraHavok のランタイム ID 不在**: 0x99D9xx 帯（dword ID store＋BSExtraData vft ストアの集合）の帰属——raw-keep/変換経路なら ExtraHavok=0x0E 説の検証（**静的証拠のみで裁定**）。
3. pass2 0x461CC5 の「レコード種別 8」語義（452DF0 force 除去が種別 8 に限定される理由＝changeFlags レコードの種別表との突合）。
4. 0x99D9xx dword-ID サイト群の個別関数帰属（i2 で vft=BSExtraData 名称なし site 一覧は %TEMP%\opencode\v278_i1.out に残存）。

## 検証（selftest 相当・再現手順）

`%TEMP%\opencode\v278_i2.py`（producer 全 site・ID 列正）→ `v278_i3c.py`（クラス側 141 site・表 2-2 正本）→ `v278_i4.py`（452D60/DF0/CF0/C90 構造・E8 callers・[reg+disp] 即値スキャン）→ `v278_i5.py`（callers 文脈全文）→ `v278_i6.py`（getters callers・dispatch 末尾）→ `v278_i8/i9.py`（文字列/RTDynamicCast 解決）→ `v278_i10b.py`（整列関数 walk・[reg+0x50] 書手探索）——**i1 の ID 列は ident オフセットバグ（blob index +0x400 漏れ）で参照不可**（教訓: バイトパターン照合は blob 基準オフセット統一）。全再実行 EXIT=0・表 2-2 の 71 値と v281 §2-1/§2-2 の重複分 100% 一致・矛盾 0（0x5A のみ新規、0x3E/0x18 の混線は 2-3 のウィンドウ bleed 説明で解消）。

（件1/件2/件3 とも v286 残点 3 件を解消。**既存成果物本体は無編集**・ESM 非接触・git 非接触）
