# v267_process_havok_residuals14.md — process/Havok 残点第14束（V-255-14）

- 対象: `Oblivion.exe`（Steam v1.2.0416）／出典: `tmp/re/v259_process_havok_residuals13.md` §6-2 の新規残点 6 件
- 分析担当サブエージェント（TODO serial 255 / V255・低）。docs/GT/TODO/plan/session-memory/cache/ソース無編集・訂正はドラフトのみ
- 環境: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7、.text VA=0x401000/raw 0x400、.rdata VA=0xA28000/raw 0x627200、.data VA=0xB02000/raw 0x700C00）、Havok 帯 `0x84xxxx–0xA2xxxx`。VA 換算は `v172_common.va2off()/imm32_refs3()/e8_callers()`・`v249_util/v251_util` に統一（罠非踏襲）
- 前提（v259 確定事項・再検証せず引用）: hkDisplay 派生 8（→9+基底、v259 §1）クラスの vt↔名／14 表=`0xAA2B10..0xAA2B44`=`hkServerDebugDisplayHandler` 相当の**死テーブル**（抽象 vt `0xAA2984`={D2, purecall×12}・生実装=`0xAA29B8` 13 slot）／`[0xBA840C]`=静的 m_tag 常時 0（保持者=ctor `0x9187A0` の hkServerProcessHandler 相当・生成主 `0x8CA9B4`）／registerProcess 17 点=16 グローバル+`0xBA9508` 二重／VS6 COL=`{0,off,0,TD@+C,CHD@+10}`・TD+8 インライン名／Visualize/debug TU は RTTI 非生成／node2 vt=`0xA98234`（隣 `0xA98240` 5 slot・`0xA982A0` 4 slot）・node4 vt=`0xA9DD5C`（2 slot に訂正済）／+0x28=hkEntityListener/hkPhantomListener 確定・+0x20/+0x2C=機能確定・名前未確定
- 方針: 各件「[確定]/[推定]/[未確定]＋VA・証拠」。「確認できず」と「不存在証明」を区別
- 中断対策: **1項目終えるたびに即追記**（§1→§6 の順）
- selftest: 着手時 `probe_harness.py selftest` = **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 → ALL PASS）

## 対象 6 件（v259 §6-2 残点 1〜6）

1. 14 表 [13] の dtor 重複セル意味（低）
2. +0x20/+0x2C の 4.0 固有名（低）— Physics listener ヘッダ 404×3 継続・web 再試行
3. node2 の 0xA98240 5 スロット表と hkSystem 4.0 ヘッダ所在（低）
4. 0xAA2B4C/0xAA2B54 stream 系小表の帰属（低）
5. 0x9187A0 クラスの 0x8B0E10/0x947EE0/0x947FA0（低）— hkCommandRouter.h 再取得試行
6. 0x8CA900 の生成条件（低）

---
## §1 item1（V-255-14-1）14 表 [13] の dtor 重複セル意味 — **解明＋表境界の訂正: 「重複」ではない。0xAA2B10 の実体は 13 セル（dtor 調整 thunk+具象 12 virtual＝4.0 hkDebugDisplayHandler part vt）で終わり、0xAA2B44 は**直後に隣接する別表**＝同一クラスのプライマリ vt `0xAA2B44`={`0x948510`(scalar deleting dtor=D3), `0x68F970`(空 ret4)} の slot0。[0] の thunk（`sub ecx,8→jmp 0x948510`）が指す先＝プライマリ vt の slot0 そのものなので「同じ値が 14 番目に現れる」のは境界誤読の必然。副次発見として v259 §3 の**死活判定が誤り**と判明: ctor `0x9484E0`（imm32 store @`0x9484F9`＝v259 が「dtor 自己復元」と誤読した 2 件のうち 1 件、`mov word[+6],1`=refcount 初期化で ctor 確定）が存在し、`e8(0x9484E0)={0x91891C}`＝**hkServerProcessHandler 相当 ctor `0x9187A0` の末尾で alloc 0x10 tag 0x32 → ctor(stream=自オブジェクト[+0x18]) → 自オブジェクト[+0x1C]=D+8 に handler part ポインタ保存**。つまり 14 表（＝0xAA2B10 具象 12 virtual）は生成される生きたテーブル**

### 証拠A: 表境界の機械確定（`v267_i1_out.txt`(c)）
- `.rdata` 窓 0xAA2AEC..0xAA2B70 の 4B ダンプ: 0xAA2B10 からの連なり = [0..12]=`{0x9484D0, 0x9485C0, 0x9480A0, 0x948110, 0x948180, 0x9481E0, 0x9482A0, 0x948310, 0x948390, 0x9486D0, 0x948650, 0x948430, 0x948490}`（13 セル）→ 0xAA2B44=`0x948510`, 0xAA2B48=`0x68F970`, 0xAA2B4C=`0x9487D0`, 0xAA2B50=`0x68F970`, 0xAA2B54=`0x948C50`, 0xAA2B58=`0x68F970`, 0xAA2B5C..=`0x982D85`(purecall)×6 — 全部 CODE なので 14 セル 1 表に見える（v254/259 の取り込み原因）。
- **imm32 参照による境界立証**（`v267_i1b_out.txt`(d)）: `refs(0xAA2B44)={0x9484F5 のみ}`（ctor の `[+0]` store）／`refs(0xAA2B48)=0 件`（どの vt の先頭でもない）／`refs(0xAA2B4C)={0x948765,0x9487AC}`（別クラス A の 2 ctor が `[+0]` に store）／`refs(0xAA2B54)={0x948C45}`（別クラス B の ctor）。→ 各 vt の先頭は「store される VA」で一意に決まる方式で **0xAA2B10=13 slot / 0xAA2B44=2 slot / 0xAA2B4C=2 slot / 0xAA2B54=2 slot** と確定。

### 証拠B: クラス D（=14 表の帰属、hkServerDebugDisplayHandler 相当）の完全な生成列（`v267_i1d_out.txt`(a)(c)）
- ctor `0x9484E0`（int3 パッド後独立エントリ、thiscall+引数 1・`ret 4`）: `word[eax+6]=1`（refcount 初期化＝**ctor の定義証拠**）→ `[+8]=0xAA2984`（抽象 handler 埋込）→ **`[+0]=0xAA2B44`** → **`[+8]=0xAA2B10`**（具象で上書き）→ `[+0xC]=arg`。サイズ 0x10（+0 vptr／+4 size word／+6 refcount／+8 handler vptr／+0xC stream ptr）。
- 呼び主 `0x91891C` は ctor `0x9187A0`（0x50 B・tag 0x32・hkServerProcessHandler 相当、v259 §4）の末尾列の一部:
  - `0x91890F`: `[0xBA7D98]` allocator→`call [eax+0x10]`(alloc, size **0x10**, tag **0x32**) → `word[eax+4]=0x10` → `push [esi+0x18]`（後述 A オブジェクト）→ `call 0x9484E0` → **`[esi+0x1C]=eax+8`**（D の handler part ポインタを保持。eax=0 なら `[+0x1C]=0`）。
  - 同じ末尾ブロックが引数別で **B オブジェクト**（alloc 0x10 tag 0x17→`call 0x948C30`→`[esi+0x14]`）と **A オブジェクト**（alloc 0x10 tag 0x17→`call 0x948770`→`[esi+0x18]`）を生成（null 引数なら 0 埋め）。＝サーバハンドラの 0x50 オブジェクトは D を**内蔵メンバーとして常に構築**、A/B（接続・出力ストリーム）は ctor 引数 gates（item6 の生成条件に接続）。
- D3 `0x948510` の全文（`v267_i1_out.txt`(f)）: `test byte[esp+4],1`（delete flags）→ 破棄鎖 `[+8]=0xAA2B10 →[+8]=0xAA2984 →[+0]=0xA45E5C` → flags&1 なら `[0xBA7D98]`→`[eax+0x14]`(deallocate) with **size=word[+4]**・**tag 0x32** → `ret 4` ＝**スカラー deleting dtor（D3）そのもの**＝プライマリ vt slot0 に置くのが MSVC 規則（+0 に RefObj、+8 に handler の MI＝v259 §3 証拠B の復元列と相互整合）。調整 thunk `[0]=0x9484D0`（`sub ecx,8; jmp 0x948510`）は「handler part ポインタ経由で delete したとき D3 を呼ぶ」VS6 定番 thunk で、**[13] 相当セルは part 表ではなく D プライマリ表の slot0** — 値が同一に見えるのは重複ではなく同関数共有。
- 旁証: v259 §3 実測の「[1] `0x9485C0` が `[esi+4]`=m_outStream を読む」は part 視点（esi=this+8）で `this+0xC`＝ctor が書いた stream ptr と完全一致。抽象 `0xAA2984`={D2 0x947650, purecall×12}=13 slot と、part 表 13 slot が本数一致（`v267_i1_out.txt`(e): 0x947650 は flag test+0x401F20 delete の D3 型で slot0=~dtor ✓）。

### 証拠C: v259 §3「死テーブル」裁定の誤り箇所
- v259 §3 証拠B は `imm32_refs3(0xAA2B10)=.text 2 件` を「dtor 領域自身の自己復元書込みのみ」と読んだが、`0x9484FC`（実 store @0x9484F9）は **ctor 0x9484E0 の中**（0x9484E0 直前は thunk 0x9484D0+int3 パッド、`v267_i1_out.txt`(e) bytes: `83e9 08 e9 38000000 cc… 8bc1 8b4c2404 66c740 0601 00 c740 088429aa00 c700 442baa00 c740 08102baa00`）。v259 は e8 呼び主を dtor 2 関数（0x9484D0/0x948510）だけ確認し ctor を見落とした（教訓: **thunk/dtor 隣接ブロックは関数境界を int3 パッドで切断してから ctor 有無を言え**）。
- 12 virtual 実装関数の e8 呼び主 0（v259 実測）は妥当 — 呼出はすべて `[[D+8]+slot]` 間接経由（D を保持するサーバハンドラ側 0x918xxx が part ptr `[+0x1C]` を介して虚呼出）。旁証（`v267_i1c_out.txt`(d)）: `refs(0x948CA0)`（0xAA2B74 表 slot0）が `.rdata` 0xA9A620…0xA9B9xx 帯に大量出現＝コマンド別ジャンプ表群が VDB 実装関数群を参照（プロトコル生きている側の証拠）。

### 裁定
> - **「[13] 重複セル」の正体 = 表境界アーティファクト**（13 セル part vt + 直後 2 セルプライマリ vt slot0 の同値）。VS6 の dtor 二重掲載 quirk では**ない**。意味論の残点（「4.0 ヘッダの ~dtor 宣言位置依存」）は解消: dtor は part 表では [0]（調整 thunk）・プライマリ表では [0]（D3 実体）で、どちらも宣言位置＝先頭と無関係に MSVC の固定規則。
> - **訂正ドラフト（v259 §3 対象）**: 「14 表 は本 EXE で未インスタンス化の死テーブル（ctor 不存在）」→ **「hkServerProcessHandler 相当（0x9187A0）に常時埋め込まれる生きた hkServerDebugDisplayHandler 相当 D（ctor 0x9484E0・alloc 0x10 tag 0x32・出力先=自 [0x18]=A(hkOstream 系)・保持 [0x1C]=D+8）」**。ただし A/B の生成は 0x9187A0 の引数ゲート（→ item6 で条件確定）で、出力ストリーム不在時は 12 実装が null チェックで early-return（VDB 無効時の挙動＝v259 の「表示機能剥離」旁証は維持）。
> - 「14 表」という呼称自体は v254 来の変更禁止対象なので本mdでは併記のみ。

**判定: 解明（重複でないと機械確定・帰属 class D の生成列まで確定・v259 死活判定に訂正 1 件）**（証拠: `v267_i1_out.txt`(a)(b)(c)(d)(e)(f)(g)、`v267_i1b_out.txt`(a)(b)(c)(d)(e)、`v267_i1c_out.txt`(b)(d)、`v267_i1d_out.txt`(a)(b)(c)(d)(e)）

---
## §2 item2（V-255-14-2）+0x20/+0x2C の 4.0 固有名（＋既知 +0x28 のメソッド名） — **大進展: Physics listener ヘッダ 404 系譜は今束で解消（正パスは `Source/Physics/Dynamics/Entity/hkpEntityListener.h`・`.../Phantom/hkpPhantomListener.h`、取得成）。+0x28 のメソッド名は 5.1 と 5 virtual 逐語一致（slot3 実装=ガード `[arg+8]`→slot2→slot1 呼出＝本 EXE `0x4CA2A0` と byte 級一致）→「entityAddedCallback/entityRemovedCallback/entityShapeSetCallback/entityDeletedCallback（phantom 側は phantom* 接頭辞）」[確定（相当名）]。+0x2C はバイナリ側 Tt リフレクション名表で `sendProps`/`getProps` が初出（ST viewer の TU 名表 @0xA9D38A/0xA9D39A）→ 機能手前まで特定。さらに 4.0 の表示 process クラス群の固有名を発見: 本 EXE は 5.1 の `hk*Process` 相当を `hk*Viewer`（`.\visualdebugger\viewer\hkSweptTransformDisplayViewer.cpp` 等・Tt 名 `hkSweptTransformDisplayViewer`/`hkConvexRadiusViewer`/`hkPhantomDisplayViewer`/`hkShapeDisplayViewer`/`hkSimulationIslandViewer`/`hkRigidBodyCentreOfMassViewer`/`hkConstraintViewer`/`hkBroadphaseViewer`/`hkRigidBodyInertiaViewer`）と呼ぶ。+0x20（B 系 2 空仮想）の固有名は素材不在のまま=未確定（確認できず）**

### 証拠A: +0x28 = hkEntityListener/hkPhantomListener のメソッド名（5.1 一次資料と位置逐語一致）— [確定（相当名）]
- 今束 web 取得（sigmaco/havok-v5.1.0r1-b2007.09.19・branch main）:
  - `Source/Physics/Dynamics/Entity/hkpEntityListener.h` — 宣言順 **{virtual ~hkpEntityListener, entityAddedCallback(hkpEntity*), entityRemovedCallback, entityShapeSetCallback, entityDeletedCallback}** = 5 virtual、うち shapeSet のみ実装付き（`if (entity->getWorld()) { entityRemovedCallback(e); entityAddedCallback(e); }`）。
  - `Source/Physics/Dynamics/Phantom/hkpPhantomListener.h` — 同型 5 virtual（phantomAdded/Removed/ShapeSet/DeletedCallback）。
- 本 EXE 実測（v259 §2 表）: hkEntityListener 基底 vt `0xA46684` = {D2 `0x4CA2D0`, 空`0x68F970`, 空`0x68F970`, **`0x4CA2A0`**, 空} — **slot 位置が 5.1 の 5 virtual と 1:1**（[1]=added 空実装・[2]=removed 空実装・[3]=shapeSet のみ実 body・[4]=deleted 空実装）。`0x4CA2A0` 本体（v259 実測「`if([edi+8]!=0){ call [[this]+8]; call [[this]+4] }`」）= 5.1 実装の byte 級写像（`[edi+8]`=entity/phantom の m_world@+8、slot2→slot1 順 = removed→added）✓✓。Ph 基底 `0xA56320` の slot3 も同一 `0x4CA2A0`（added/removed 形状同一のため ICF 共有＝phantom 側 world ゲートも +8）。
- 派生側（ST part `0xA9D404` [1]=`0x91ADB0`/[2]=`0x91AAB0`＝added/removed 実装、CR `0x91B6A0`/`0x91B550`、Ph `0x91D040`/`0x91CD10`）— v259 §2 証拠B の表と位置そのまま対応。
- 404×3 系譜の教訓: 5.1 ツリーは `Source/` ルート（Physics/Collision ではなく **Physics/Collide**、listener は Physics 直下でなく Dynamics/{Entity,Phantom}、Visualize は **Source/Common/Visualize**）。

### 証拠B: 4.0 の process 家系固有名 = hk*Viewer（バイナリ内 Tt 名表＋パス文字列）— [確定（実名）]
- `.\visualdebugger\viewer\hkSweptTransformDisplayViewer.cpp`@`0xA9D440`、`hkConvexRadiusViewer.cpp`@`0xA9D530`、`hkShapeDisplayViewer.cpp`@`0xA9D5EA`、`hkPhantomDisplayViewer.cpp`@`0xA9D710`（`v267_i2d_out.txt`(a)・全文字列実測）。
- Tt リフレクション名: `TthkSimulationIslandViewer`@`0xA9D304`／`TthkRigidBodyInertiaViewer`@`0xA9D3A4`／`TthkSweptTransformDisplayViewer`@`0xA9D3D8`／`TthkConvexRadiusViewer`@`0xA9D4D0`／`TthkShapeDisplayViewer`@`0xA9D56A`／`TthkRigidBodyCentreOfMassViewer`@`0xA9D658`／`TthkPhantomDisplayViewer`@`0xA9D744`／`TthkConstraintViewer`@`0xA9D874`／`TthkBroadphaseViewer`@`0xA9D8C0`（`v267_i3_out.txt`(a)、`v267_i2d_out.txt`(b)）。各名は registerProcess 登録名（WorldMemory@0xA9D244/Tensor@0xA9D326/SweptTransforms@0xA9D3C6/ConvexRadius@0xA9D4BE/CentreOfMass@0xA9D622/Phantoms/ForcedContactPoints@0xA9D76A/ContactPoints@0xA9D7D6/Constraint@0xA9D82A…v259 §5 表）と同プールに同居＝**v251/254/259 の「ST/CR/Ph/Q 帯」= 4.0 では `hk<Name>Viewer` クラス**（5.1 の `hkSweptTransformsProcess` 等に対応、改名は 4.5→5.0 間と推定）。
- 帰結: v259 §2 が「Bethesda 側 interface」と見た +0x2C 実装群は、クラス名付きで「hk*Viewer が実装する基底 Q の interface」と読み替え可能（Q 基底自身の Tt 名は未発見＝`hkDisplayViewer`/`hkDebugDisplayViewer` 系名文字列は本帯に存在せず）。

### 証拠C: +0x2C のメソッド名候補 = sendProps/getProps（Tt 名表に実在）— [推定（中）]・interface 固有名自体は [未確定]
- `TtsendProps`@`0xA9D388`／`TtgetProps`@`0xA9D397` — 全イメージ走査で**この 2 箇所のみ**（`v267_i2c_out.txt`(b)）。位置は `TthkSweptTransformDisplayViewer`@0xA9D3D8 と同一名表ブロック（0xA9D388..0xA9D440 の ST viewer TU プール）＝ hkSweptTransformDisplayViewer の**メソッド名反射**。
- 形状対応: ST の +0x2C 派生実装 = {m1 `0x91A720`（ret8・TEB スレッド別リング 2 出力）、m2 `0x91A910`（ret4・引数 object +0x50 経由）}（v259 証拠D）。sendProps/getProps の 2 つ組がこの {m1,m2} に対応する蓋然性は高いが、**名と slot の割当方向を含む直接紐付けは未証明**（反射名表にアドレス情報なし）。
- 交差材料（5.1 hkProcess.h を今束取得・全文）: `getConsumableCommands(hkUint8*&, int&)` 既定体 `commands=HK_NULL; numCommands=0` は本 EXE `0x918CD0`（`mov[eax],0; mov[ecx],0; ret 8`）・`consumeCommand(hkUint8)` 既定 `{}` は `0x9266A0`（空 ret4）と**本体逐語一致**。4.0 hkProcess 基底 vt `0xA9D1C0` = {`0x918500`(~dtor), purecall(getProcessTag), `0x918CD0`, `0x9266A0`, `0x918000`(init), `0x9266A0`(step)} = 6 slot で 5.1 7 slot から isOk を除いた形と一致（hkProcess 自体の位置付けは v259 §4/§5 と整合）。**しかし +0x2C part 基底 vt `0xA9D2B4` は 3 slot {D2 `0x919180`, pure, `0x9266A0`} で hkProcess の 6 slot と別物**（v259 の「同一形」は slot1/2 の既定-body 一致まで。クラス同一性は否定）。よって +0x2C = hkProcess 由来でなく **hk*Viewer 基底固有の props 送受 interface**（sendProps/getProps 候補）と整理。
- 判定: interface クラス名（Q の基底名）は **確認できず**（RTTI 非生成・Tt 名なし・5.1 に対応物なし＝v259 証拠C/D の「4.0 専用」結論維持）。メソッド名は sendProps/getProps [推定・中]。

### 証拠D: +0x20（B 系） — [未確定（確認できず）]（増補事項のみ）
- v259 §2 証拠C の形状（抽象 `0xAA2B9C`{pure,pure}→実装既定 `0xA9D250`{空 ret4×2}、WM プライマリ共有、ST 派生 slot1 がエンティティ配列線形探索）から前進せず: Tt 名表（0xA9D2C0..0xA9DA40）・`.\visualdebugger` 文字列・RTTI 索引（v259）のいずれにも B 基底候補名なし。WM プライマリが B を +0 に取る＝**WM（hkWorldMemoryViewer 相当）は B 単独実装**（v259 §2 証拠C の再確認）。旁証候補: 「Visual Debugger」名帯（0xA99ACC）＝ VDB server TU と B の関係は不明（0x20 のメソッド 2 つに割り当て可能な名前文字列を本帯で発見できず）。
- 「確認できず」（不存在証明ではない・素材不在）。機能名推奨は v259 維持:「エンティティ照会（B 系 2 槽）インターフェース」。

### 裁定
> - **+0x28: メソッド名確定（相当）** — hkEntityListener{entityAddedCallback/entityRemovedCallback/entityShapeSetCallback/entityDeletedCallback}・hkPhantomListener{phantom*}（5.1 一次資料＋slot 位置・ガード・呼出順の byte 級一致。4.0 でクラス名 RTTI 実在は v259 済）。
> - **process 家系 = hk*Viewer 4.0 実名**（パス文字列 4・Tt 名 9・登録名と同プール）— v259 §2/§4 の「無名 process クラス」の大半に実名が入る（訂正ドラフト §6-5 相当）。
> - +0x2C = {~dtor, m1 純粋, m2 既定空} の props 送受 interface（メソッド名 sendProps/getProps 候補 [推定]）— 固有名は確認できず。
> - +0x20 = 確認できず（変更なし）。

**判定: 部分解明（+0x28 のメソッド名と家系実名 hk*Viewer が新規確定。+0x20/+0x2C の interface 固有名は依然「確認できず」だが +0x2C は sendProps/getProps 候補まで進展）**（証拠: `v267_i2_out.txt`(a)(b)、`v267_i2b_out.txt`(a)(b)(c)、`v267_i2c_out.txt`(a)(b)(c)(d)、`v267_i2d_out.txt`(a)(b)(c)(d)、`v267_i3_out.txt`(a)(b)(c)(d)、SDK 5.1 `Source/Physics/Dynamics/Entity/hkpEntityListener.h`・`Source/Physics/Dynamics/Phantom/hkpPhantomListener.h`・`Source/Common/Visualize/hkProcess.h`（本束全文取得））

---
## §3 item3（V-255-14-3）node2 の隣接 `0xA98240` 5 スロット表の帰属・意味 と hkSystem 4.0 ヘッダ所在 — **解明（機能・生成経路）＋所在は構造的確定: `0xA98240` = hkSystem boot 関数 `0x8BBA80` が作る「ノード/ストリーム工場」オブジェクトの vtbl。5 slot = {`0x8BB3F0`(~dtor=node2 と同一)、空 ret4、`0x8BB060` createNode（8B・vt `0xA98254`）、`0x8BB090` createIStream（`0x8F5BC0`→hkIstream 系 vt `0xA9B38C`＋0x1000 バッファ streambuf）、`0x8BB240` createOStream（`0x8F5F10`→hkOstream 系 vt `0xA9B3BC`＋0x1000）}。この hub は alloc 8 tag 0x15→vt store→`call 0x534070`（静的 hkRef スロット `[0xBA7FB4]` への割当解放 helper）でグローバル保持。生成主 `0x8BBA80` の呼び主は `0x88B0F7` の 1 件＝ゲーム側 NiHavok グルー（`[0xBA8040]` 二重初期化ガード@`0x88B094` を持つ起動関数）＝ v259 §5「boot④ node2 slot2 no-op 呼出」の本体そのもの。node2（8B・vt `0xA98234`）は **.text の store 0 件**で、`.data 0xB2F1B4 の CRT 静的初期化子テーブル→0x8BB530(factory)` により起動時自動生成（読手は boot 末尾 `0x8BBB64` のみ＝v259 踏襲）。hkSystem 4.0 ヘッダ所在: 5.1 リポジトリに hkSystem.h は**存在せず**（`Source/Common/Base/System/hkBaseSystem.h` に分離＝namespace init/initThread/clearThreadResources/quit — 本 EXE の 0x8BA9C0/0x8BBA80 と意味 1:1 対応候補）。4.0 版の実パスは素材なし＝確認できず**

### 証拠A: 0xA98240 の所在・store・スロット意味（`v267_i3b_out.txt`(a)(b)(c)、`v267_i3c_out.txt`(a)(e)(f)）
- 表本体: `0xA98240` = {`0x8BB3F0`, `0x68F970`, `0x8BB060`, `0x8BB090`, `0x8BB240`}（5 slot、直後 `0xA98254` は createNode が 8B ノードに書く別 vt）。store 元 = **`0x8BBB24` の 1 件のみ**（imm32 refs 実測）。
- `0x8BBB24` は boot 関数 `0x8BBA80` の中核列（`v267_i3c_out.txt`(a)）:
  - `[0xBA8040]`（byte flag）立哨 → `call 0x8BA9C0`（TLS `+[TEB]+[0xBA9DE4]*4` の `+0x1A0/+0x1A4/+0x1A8/+0x1AC` ゼロ化＝**clearThreadResources 相当**）→ `0x8A70F0(arg)` → `malloc 0x330`（CRT `0x401F00`）→ `0x8A72A0(0, arg)` 構築→ `0x8A7260(eax)` 登録、第 2 malloc 0x330 → 同構築 → **`[0xBA7D9C]=eax`**（＝ hkThreadMemory フォールバック、0x8BB03E で TLS 接続失敗時の代替として読まれる ✓）
  - `[[0xBA7D98]]+[0x10]` で **alloc 8 / tag 0x15**（＝ hkMemory シングルトン経由・物理/システム用プール tag）→ size/refcount store → `[eax]=0xA98240` → `push eax; call 0x534070`（**`[0xBA7FB4]` 静的 hkRef への割当 helper**：現値 release→新値 addref→store の列、先頭 10 命令実測）＝ hub をグローバル Singleton 化。
  - 続けて alloc 0x28/tag 0x15 → ctor `0x8BB560`（`[esi]=0xA982C0`＋`call 0x8B0E10`（＝+8 部の hkPointerMap/hkArray 初期化、**item5 で再登場のあの関数**）＋ hkArray `{+0x14,+0x18}=0`/`+0x1C=0x80000000`）→ `call 0x534020`（`[0xBA7FB0]` 静的 hkRef 割当）→ **レジストリ 0x28 オブジェクト**を第 2 Singleton 化 → `call 0x8BB420`（`[0xBA8188]` 配列走査の別 init）→ `mov ecx,[0xBA803C]; call [[ecx]+8]` = **node2 slot2（`0x918000` no-op）呼出＝v259 §5 boot④ の本体確定**。
- e8(0x8BBA80) = **`{0x88B0F7}` の 1 件**のみ（`v267_i3d_out.txt`(c)）— 呼び主側 0x88B094 で `[0xBA8040]` ガード＝**ゲーム起動グルー（NiHavok/bhk 系 0x88B0xx 帯）から 1 回だけ呼ばれる hkSystem::init 相当**。teardown 帯（v259 の 0x8BBC03）は同ファイルの `[0xBA8040]` 参照 `0x8BBC57` で裏取り ✓。

### 証拠B: 各 create の実体（`v267_i3b_out.txt`(c)）
- slot2 `0x8BB060`: alloc 8 tag 0x15 → `word[+4]=8`/`word[+6]=1`/`[+0]=0xA98254` → **接続ノード（hkRefObj 系 8B）**を量産。`0xA98254` 表（14 slot dump）は `{0x9180D0(D2), 空, 0x8BC090, 0x8BB8F0, 0x60D0A0, 0x8B1670/80/90 群, …}` ＝ **ソケット操作群（0x8B1xxx = send/recv/close 系）**。
- slot3 `0x8BB090`: alloc 0x14 → ctor `0x8BB120`（`[+0]=0xA98274`、`[+8]=-1`（SOCKET 初期値）、`0x982440(arg,"rb")`＝**CRT fopen mode 引数**（`0x982440` は push 0x40→`0x98237C` の CRT ラッパ））→ 虚 `call [vt+0x14]` → 成否見て alloc 0x24 → **`0x8F5BC0(0x1000, …)`**（[esi]=`0xA9B38C`＝ streambuf／istream 実装）→ 複合戻り。
- slot4 `0x8BB240`: alloc 0x10 → ctor `0x8BB2B0` → alloc 0x1C → **`0x8F5F10(0x1000, esi)`**（[esi+8]=arg、[esi]=`0xA9B3BC`、+0x18=1）＝ hkOstream 系 streambuf。**`0x8F5F10` は D サーバ側 A オブジェクト（0x948770）が 0x2000 で使うのと同関数**（item4 と接続）。
- 帰結: hub の 5 slot = **{~dtor, 空, ノード作成, 入力ストリーム作成, 出力ストリーム作成} の抽象工場** — VDB サーバ（接続受付）がノード＋双方向 stream を確保する口。v259 §5 boot④・teardown `0x8BBC03`・node2 slot2 の三つ巴が同一 TU（0x8BA9xx–0x8BC0xx＝hkSystem/hkMemoryWin32 帯、`[0xBA7D98]`=root hkMemory・`[0xBA7D9C]`=thread memory・`[0xBA9DE4]`=TLS index・TEB+0x2C／+0x19C〜0x1AC＝**スレッドコンテキスト**）に属することも再確認（`v267_i3b_out.txt`(e)(f)）。

### 証拠C: node2 の生成経路（CRT 静的初期化子）（`v267_i3d_out.txt`(a)）
- `.text` で `[0xBA803C]` を**書く**命令はゼロ（read は `0x8BBB66` の 1 件＝boot 末尾 slot2 呼出のみ、imm32 refs=2 件のうち .data 側は 0xB2F1BC）。
- `refs(0x8BB530)`（node2 factory）= **`.data@0xB2F1B4`** — 0xB2F1B4/0xB2F1BC = {0x8BB530, 0x00BA803C} の**ペア**＝ VS6 の静的オブジェクト初期化子テーブル（CRT `_initterm` 走査リスト、entry=生成 fn＋保管先 ptr）。→ **node2 はモジュール起動時に CRT 経由で自動生成された静的インスタンス**（実行時生成箇所なし＝v259 §5 の「静的 [0xBA803C]」の生成メカニズムを初特定）。vt `0xA98234`（3 slot {D2, 空, no-op}）の保持者＝この node2 ただ 1 つ。

### 証拠D: hkSystem 4.0 ヘッダ所在（web 側）
- 今束で 5.1 ツリーの該当箇所を確認取得: `Source/Common/Base/System/hkBaseSystem.{h,cpp}` のみ（**`hkSystem.h` は 5.1 に存在しない** — 4.x の class hkSystem は 5.0 代で `hkBaseSystem`（namespace: init/initThread/clearThreadResources/quit/isInitialized/initSingletons/quitSingletons）と `Common/Base/Thread/*`・`System/Io/*` へ分割。本 EXE の `0x8BBA80`(init)/`0x8BA9C0`(clearThreadResources)/`0x8BB020`(quit-thread) と意味対応が 1:1 で並ぶ＝**5.1 の後継ヘッダ所在は確定**）。
- v259 の 404（Base/System/hkSystem.h・Base/hkStackTracer.h）は「Source/ 接頭辞＋5.1 改名」の二重の壁だった（StackTracer も `Common/Base/Debug/` 側に移動しているが hkSystem と違い別物）。**4.0 版ヘッダそのものの所在（配布 SDK 内のパス）は取得素材なし＝確認できず**（GitHub に 4.x public source 非公開は本束の再試行でも同結果）。

### 裁定
> - `0xA98240` 5 slot 表 = **hkSystem boot が作るノード/ストリーム工場 vtbl**（slot2/3/4=createNode/createIStream/createOStream、hub の単一 store 元=boot `0x8BBB24`、Singleton 化先 `[0xBA7FB4]`）。意味＝VDB サーバ接続用リソースアロケーションの抽象工場（4.0 hkSystem サブシステムの一部）と確定。クラス固有名は名文字列なし＝**確認できず**（機能名「(hkSystem debug server) node/stream factory」推奨）。
> - node2 = **CRT 静的初期化子（.data 0xB2F1B4 ペア）経由の唯一生成**・読み手は boot 末尾 slot2 のみ — v259 の形状確定に生成メカニズムを追加。
> - hkSystem 4.0 ヘッダ所在: **5.1 に同名ファイル無し（構造的確定）＋後継 `Source/Common/Base/System/hkBaseSystem.h`**（本束全文取得）。4.0 SDK 内のパスは未確定。

**判定: 解明（表の帰属・全 slot 意味・生成経路・Singleton 座標／固有名のみ確認できず）**（証拠: `v267_i3b_out.txt`(a)(b)(c)(d)(e)(f)、`v267_i3c_out.txt`(a)(b)(c)(d)(e)(f)、`v267_i3d_out.txt`(a)(b)(c)(d)(e)(f)、SDK 5.1 `Source/Common/Base/System/hkBaseSystem.h` 全文）

---
## §4 item4（V-255-14-4）`0xAA2B4C/0xAA2B54` の {D3, 空} 2 小表 = stream 系 part の帰属 — **解明: 両小表は hkServerProcessHandler 相当（ctor `0x9187A0`）の**接続ストリームメンバー 2 オブジェクトの vtbl**。`0xAA2B4C`＝**出力側 A**（自オブジェクト [+0x18]、D=14表クラスの m_outStream `[+0xC]` に渡される 0x10B・tag 0x17・**base ctor `0x9183A0`＝`0xA9D1B8` 表の ostream クラス**派生）、`0xAA2B54`＝**入力側 B**（自 [+0x14]、0x10B・tag 0x17・**base ctor `0x9479F0`＝同 `0xA98328` 表の istream 系**派生、D3 が `0x8BC000`→`0xA98328`→`0xA45E5C` 鎖＝hkSocket 帯基底）。生成主は `0x9187A0` の末尾 block のみ（B=alloc→`0x948C30`@`0x9188CF`、A=alloc→`0x948770`@`0x9188F8`、**引数 null gate**）＋ A は `0x918C56`/`0x948750` in-place 版が `0x918B40` 帯で再生成（接続ごとの再バインド疑い）。A/B の実体は hkSystem 工場（§3 の `0xA98240` 5 slot 表 createIStream/createOStream）と**同一の streambuf ctor ペア `0x8F5BC0`(0xA9B38C)/`0x8F5F10`(0xA9B3BC)** を使う＝VDB サーバの送受信ストリーム対と確定。4.0 固有名は名文字列なし＝確認できず（相当名 hkOstream/hkIstream ＝推定）**

### 証拠A: A（0xAA2B4C）の完全な生成列（`v267_i1c_out.txt`(b) 再読＋`v267_i4_out.txt`(b)）
- A の ctor は 2 変種: in-place `0x948750`（`call 0x9184A0`→`[esi]=0xAA2B4C`）と create 版 `0x948770`（alloc 0x1C tag 0x17 の streambuf → **`0x8F5F10(this, raw, 0x2000)`**＝buffered streambuf ctor・`[sb]=0xA9B3BC` → `0x9183A0(A, sb, flag=0)` → `[esi]=0xAA2B4C` → 旧 streambuf release → return A）。`0x9183A0`（base ctor・呼び主 7 件@0x90e579/0x90e885/0x90eef6/0x94161a/0x9487a2/0x954638/0x958140）は `[+6]u16=1`→`[+0]=0xA9D1B8`→`[+8]=sb`→`[+C]=byte flag`→sb refcount++ ＝ **ostream ラッパ基底（0xA9D1B8 表 {D2 `0x9183D0`（sb release・`0xA45E5C` 復元＝RefObj 基底実証）, 空, `0x918500`, purecall, `0x918CD0`, `0x9266A0`, `0x918000`, `0x9266A0`}）**。
- 生成主: `e8(0x948770)={0x9188F8, 0x918C56}`／`e8(0x948750)={0x918BE5}` — 0x9188F8 は `0x9187A0` 末尾の引数 gate 列（`if([esp+0x1c]!=0){alloc 0x10 tag 0x17; word[+4]=0x10; call 0x948770}`→`[self+0x18]=A`）と確定（v267_i1d(a) 全文）。0x918BE5/0x918C56 は別メソッド内（0x918BC0 帯・§6 で「再接続」候補として追加確認 — A を作り直す列＝**接続単位の stream 再バインド**）。
- **用途確定（item1 と接続）**: D ctor `0x9484E0` の引数＝`[self+0x18]` の A がそのまま D+0xC=m_outStream に格納（`0x918916: mov ecx,[esi+0x18]; push ecx; call 0x9484E0`）。14 表 impl `0x9485C0` 等の `[esi+4]`（=part 視点の D+0xC）null check はこの A の有無＝**VDB 出力ゲート**。

### 証拠B: B（0xAA2B54）の生成列（`v267_i4_out.txt`(a)(c)、`v267_i4b_out.txt`(b)(c)）
- B ctor `0x948C30`（int3 パッド後独立・`v267_i4_out.txt`(a) の 0x948C30..0x948C4C。直前 0x948BD9..0x948C0F の `mov eax,N; ret` 群は `0x948910` switch の**コマンド ID getter ジャンプ表**で dtor ではない）: `call 0x9479F0(arg)` → `[esi]=0xAA2B54`。
- 基底 ctor `0x9479F0` = `0x9183A0` と**命令列完全同型**（`word[+6]=1; [+0]=0xA98328; [+8]=arg; [+C]=flag; arg refcount++`）＝ B は **0xA98328 表の istream 系基底**派生。B D3 `0x948C50` の `call 0x8BC000`（→ `[+0]=0xA98328` 復元＋[+8] リリース＋`0xA45E5C`）＝ 基底も RefObj 家系。0x8BC000/0x8BC030 は §3 の hkSocket/hkSystem 帯（TEB TLS スレッド接続関数 `0x8BB020` と同 TU）。
- 生成主: `e8(0x948C30)={0x9188CF}` のみ — `0x9187A0` 末尾引数 gate 列（`if([esp+0x18]!=0){alloc 0x10 tag 0x17; word[+4]=0x10; push arg; call 0x948C30}`→`[self+0x14]=B`）。B は `0x918760`（process part slot5）で `call [X+0x8→0x947FA0]` の **入力ストリーム引数**として使われる（consumeCommands の読取元＝§5）→ A=出力/B=入力ペア確定。

### 証拠C: 同一 streambuf ファクトリの交差（item3 と接続）
- `0x8F5F10`＝出力 buffered streambuf ctor（`[esi]=0xA9B3BC`・`[+8]=arg`・`[+0x18]=1`）: A（`0x948770`@push 0x2000）と hkSystem 工場 slot4 `0x8BB240`（@push 0x1000）が共用。
- `0x8F5BC0`＝入力側（`[esi]=0xA9B38C`・tag 0x17・refcount）: 呼び主は `e8={0x8BB0E3}`（工場 slot3 `0x8BB090`）のみ — A/B は「工場が作るストリームと同族」だがサーバ側は buffer 0x2000 で直接構築。
- 帰属: v259 §3 の「in/out stream オブジェクト系 part の小 vt（旁証）」が **A/B として実体化**。5.1 相当名は `hkOstream/hkIstream`（4.x の stream 系・5.1 では `Common/Base/System/Io/*` に再編）＋ VDB 用 `hkDisplaySerializeOStream/IStream` 相当（5.1 版は非仮想化された別物＝4.0 の 0xA9D1B8/0xA98328 表は仮想 serialize 版）と**推定（中〜高）**。固有名の実名証拠なし（RTTI 非生成・名文字列なし）＝**確認できず**。

### 裁定
> `0xAA2B4C/0xAA2B54` = hkServerProcessHandler 相当オブジェクトの**出力/入力ストリームメンバー（A/B）の vtbl**（生成主・用途・基底・streambuf 共用まで確定）。4.0 クラス名は「hkOstream/hkIstream（VDB serialize 版）相当」[推定]。v259 の slot 順記述 `{空 ret4, D2}` は実際 `{D3(実体), 空 ret4}` の順で、**「2 小表」は 2-cell 各 vt（D3＋1 空仮想）** と精密化（訂正ドラフト相当）。

**判定: 解明（帰属・生成主・A/B 役割・基底鎖・streambuf 共用。固有名のみ確認できず）**（証拠: `v267_i4_out.txt`(a)〜(f)、`v267_i4b_out.txt`(a)(b)(c)、`v267_i1c_out.txt`(b)、`v267_i1d_out.txt`(a)(c)(e)、`v267_i3b_out.txt`(c)(e)(f)）

---
## §5 item5（V-255-14-5）hkServerProcessHandler 相当（ctor `0x9187A0`）の +0x24 サブ（vt `0xA9D1F4`）と `0x8B0E10`/`0x947EE0`/`0x947FA0` の機能 — **解明（3 関数とも機能確定・うち 0x947FA0/0x947EE0 は 5.1 `hkCommandRouter.h` と意味一致）: `0x8B0E10`＝**hkPointerMap 既定 ctor（バケット 16×8B を tag 0x14 で 0x80 B 確保・count=0・mask=0xF・0x8B18C0 ゼロ埋め 2 部）**（共通ユーティリティ、e8 呼び主 15 件＝0x534483/0x8bb572(§3 hkSystem レジストリ!)/0x9187e8(本 ctor)/…。`0x947EE0`＝**hkCommandRouter::registerProcess(hkProcess\*)**（引数 process の虚 `[vt+8]`=getConsumableCommands(cmds&,num&) を呼ばれ、cmd 1 バイト毎に `0x8B0E80`（hkPointerMap::insert・ゴールデン比ハッシュ 0x9E3779B1・2 倍倍率）で self+8 の map に登録）。`0x947FA0`＝**hkCommandRouter::consumeCommands(istream, ostream)**（`0x947910` で 1 バイト読込→`0xF0`（5.1 enum `COMMAND_ACK=0xF0` 一致）で抜ける・`0x8B0F40`（hkPointerMap::get）で引きた process の虚 `[vt+0xc]`=consumeCommand(cmd) 実行・未登録時 `0xAA2A18`=**"VDB: Found a command (%x) with no handler. Could corrupt the stream."** を 0x8B1990(hkOStream <<) で出力・cmd 0xB0/0xB1 は `0x948C80`(u32×3 読) が特別処理）。サブオブジェクト `0xA9D1F4`＝**4.0 hkCommandRouter インスタンス（hkReferencedObject+内部 map、D2 `0x918690`=`lea ecx,[esi+8]; call 0x8B0E60`(map free)→`0xA45E5C` 復元）**。保持者クラス（`0x9187A0`・0x50 B・tag 0x32）＝ MI(RefObj@0/hkProcessHandler@+8/hkProcess@+0xc)＋B@+0x14＋A@+0x18＋D@+0x1c(+0x20=D part)＋**router@+0x24**＋hkArray 2 本(+0x38/+0x44) — 生成末尾 `0x918B40` は静的 hkProcessFactory `[0xBA950C]` の登録 16 process を走査し自 handler part 虚 `[vt+4]`(=`0x9185F0`=registerProcess)で**全て router に自己登録**＝5.1 の「create で作った process を router.registerProcess する」の 4.0 統合形**

### 証拠A: 0x8B0E10 = hkPointerMap 既定 ctor（`v267_i5_out.txt`(b)）
- `[[0xBA7D98] vt+0x10]`（root hkMemory alloc）で **size 0x80 / tag 0x14** を 1 ブロック確保→ `[esi]=block`・`[esi+4]=0`（count）・`[esi+8]=0xF`（mask）→ `0x8B18C0`（memset 系）×2（バケット配列＋対偶配列。0x80=16×8B={key,value}16 組＝**4.0 hkPointerMap は単一複合ブロック**、5.1 の keys/values 2 本と形が異なる）。呼び主 15 件は共通コンテナ利用証（`0x8BB572`＝§3 hkSystem レジストリ 0x28 オブジェクトの map、`0x940DBx/0x941EE9/0x942DF2/0x943146`＝VDB 系広範）。

### 証拠B: 0x947EE0/0x947FA0 = router の register/consume（`v267_i5_out.txt`(c)(d)(e)、`v267_i5b_out.txt`(a)(c)(d)）
- `0x947EE0(ecx=&router, arg process)`：`call [process_vt+8]`（**getConsumableCommands(hkUint8*&, int&) の 4.0 位置 [vt+8] = hkProcess slot2** ✓ hkProcess 基底 vt `0xA9D1B8`/`0xA9D1C0` 表と整合）→ count 分ループ `0x8B0E80(self+8=map, cmd)` insert → `ret 4`。**5.1 hkCommandRouter::registerProcess と意味一致**（「Register handlers so that the stream can be understood」）。
- `0x947FA0(ecx=&router? =X+0x18→base+0x24 ✓, args: [esp]=outStream, [ebp+0xc]=inStream, ret 8)`：`0x947910`（istream 読込 — `[[sb+8] vt+0xc]` 経由・flag `[+0xC]`=big endian 時の endian スワップ列あり）で 1 バイト→`cmp bl,0xF0`＝**COMMAND_ACK**→`0x8B0F40`（**hkPointerMap::get**・`shr 4`→`imul 0x9E3779B1`→and mask のオープンアドレス走査＝**Havok hkHashPtr ゴールデン比実装の指紋**）→ found なら `[proc_vt+0xc]`＝consumeCommand(cmd)→ 未 found なら 0x8B1990(hkOStream<<)＋**"VDB: Found a command (%x) with no handler. Could corrupt the stream."**@0xAA2A18・カウンタ `[0xBA7FC0]++`→ cmd 0xB0/0xB1（server 側保留コマンド）は `0x948C80`→ 末尾でローカル 1 バイト read → 0 なら抜ける／`bl==0xF0` でループ継続（5.1 の "Consume the commands until an ACK is reached or the stream is not OK anymore" と**処理順序まで一致**）。
- 呼び主：`e8(0x947EE0)={0x918809, 0x918B24}`（自 process 登録＋registerProcess メソッド帯）／`e8(0x947FA0)={0x918791}` の 1 件＝`0x918760`（process part slot5）**step(inStream)** の末尾。0x918760 全文（`v267_i5_out.txt`(e)）：hkArray `[this+0x20]/[+0x24]`（=base+0x38/3c の選択 process 配列）を走査し各 process の**虚 [vt+0x14]=step(stream)** を呼んだ後 router.consumeCommands(self の B=[base+0x14]) — 5.1 の「サーバが 1 本の inStream を router で配線する」設計と一致。
- 静的 registerProcess ヘルパ `0x947C50`（v259 §5 の 17 点の呼出先）は別物＝hkProcessFactory `[0xBA950C]` の名前列挙（0x8A7720 確保＋0x8B1770 文字列照合ループ）。混同注意。

### 証拠C: サブオブジェクト 0xA9D1F4 = hkCommandRouter インスタンス（`v267_i5_out.txt`(g)(h)）
- 2 cell 表 {`0x918690`, `0x68F970`}。D2 `0x918690`＝`lea ecx,[esi+8]; call 0x8B0E60`（map free・`[eax+8]` block を `[[0xBA7D98]]+[0x14]` で tag 0x14 解放）→`[esi]=0xA45E5C`（**RefObj 基底のみ＝単一継承実証**・5.1 `hkCommandRouter : public hkReferencedObject` ✓）→flag→free tag 0x32（親メモリ内埋込なので delete 経路は親 ctor 列と整合）。
- 実 body（0x50 B オブジェクト内）：vt@+0x24・+0x28=size/refcount（ctor `word[ebp+6]=1`）・map@+0x2c{buckets,count,mask}（`0x8B0E10` ecx=+0x2c）→ **hkCommandRouter(4.0) ＝ {hkReferencedObject, hkPointerMap m_commandMap}**＝5.1 と同形（map の entries が複合 1 ブロックな点のみ相違）。

### 裁定
> - `0x8B0E10`/`0x947EE0`/`0x947FA0` = **[確定]**（それぞれ hkPointerMap::init / hkCommandRouter::registerProcess / hkCommandRouter::consumeCommands・5.1 `Visualize/hkCommandRouter.h`（今束全文取得。Path=`Source/Common/Visualize/hkCommandRouter.h`）と enum 値 0xF0・処理語順まで一致）。
> - +0x24 サブ `0xA9D1F4` = **[確定]** hkCommandRouter 相当インスタンス。
> - 保持者クラス（0x9187A0）の 4.0 実名は依然「確認できず」だが、構成部材（A/B/D/router/選択配列）が全て機能確定した。v259 §6-2 item5 の「中」残点は今束で**消し込み**。
> - 副産: hkProcess 4.0 基底 vt=`0xA9D1C0` 6 slot = {~dtor `0x918500`, getProcessTag pure, getConsumableCommands `0x918CD0`, consumeCommand `0x9266A0`, init `0x918000`, step `0x9266A0`} — 5.1 の 7 仮想から isOk を除いた確定形（`v267_i2b_out.txt`(a)・slot 位置は hkProcess 実装群 0x918950(getProcessTag)/0x918760(step) と相互整合）。

**判定: 解明（3 関数＋サブの正体を 5.1 一次資料で確定）**（証拠: `v267_i5_out.txt`(a)〜(h)、`v267_i5b_out.txt`(a)〜(f)、SDK 5.1 `Source/Common/Visualize/hkCommandRouter.h`・`hkProcess.h` 全文（本束取得））
## §6 item6（V-255-14-6）bhkWorldM init 帯「0x8CA900」の全文脈 = hkServerProcessHandler 相当（0x9187A0）の生成条件 — **解明（v259 の所在解釈を訂正）: 0x8CA9B4 は bhkWorldM 系ではなく **hkVisualDebugger::createClient**（関数 `0x8CA940`、vdb クラスの pollForNewClients `0x8CAA60` の accept 分岐から唯一呼出）の中。0x9187A0 インスタンスは**Visual Debugger クライアントの TCP 接続受付 1 件ごとに** alloc 0x50・tag 0x32 で作られる（per-client。5.1 `hkVisualDebugger.h` の `struct hkVisualDebuggerClient { hkSocket* m_socket; hkServerProcessHandler* m_processHandler; }` と配列要素 8B ペア `{+0=stream/sock, +4=0x9187A0 系}` が構造一致）。サーバ自体の生成条件は **ini 設定 `bHavokDebug:HAVOK`**（名文字列@`0xA46B80`、値 byte`[0xB097B8]`、登録点 `0x9E3030`=`push 0xB097B8; ecx=0xB07BF0; call 0x4040E0`=GameSetting 追加列）を引数にゲーム側 enable ラッパ `0x88B680`（呼出 6 点=0x445628/0x445B87/0x4D4C27/0x4D4DDD/0x4D52D0/0x4D6401、うち 0x445B87/0x4D6401 は `[0xB097B8]` を push、0x445628/0x4D4C27/0x4D4DDD は 0=シャットダウン側）が vdb オブジェクト（0x90 B・tag 0x32・ ctor `0x8CA540`＝"VDB Server instance has been created"）を生成し、**`0x8CABF0` が 13 個の静的 registerProcess（WorldMemory..Broadphase）を一括呼出**、`0x8C9E20`(serve) が **ポート 0x61A9=25001**（5.1 `HK_VISUAL_DEBUGGER_DEFAULT_PORT` 実数一致）で listen（"Server created and will poll for new client(s) on port ... every frame"）**

### 証拠A: クラス S=vdb の同定（5.1 hkVisualDebugger.h と字段・仮想一致）
- ctor `0x8CA540`（呼び主 `e8={0x88B781}` のみ）: 0x90 B を埋める列 — `[+0]=0xA99AE8`（vt）、`[+0x8]=0`（m_server）、`+0xc/+0x10`=m_clients{ptr,count}、`+0x14` 埋込 hkRef 雛形、`+0x18..+0x44`=hkArray 5 本（+0x20/+0x24...flags 0x80000000 列挙実測）、`[+0x48]=[arg+4]`（**contexts 配列の count 引数**＝5.1 ctor `hkVisualDebugger(const hkArray<hkProcessContext*>&)` ✓）、`[+0x58]=0`（m_amTimingFrame ✓ poll 側 `test byte[esi+0x58]` で使用）、`+0x60..+0x84`=hkStopwatch 8 語、**`[+0x88]="Frame Timer"`**（0xA99ADC ✓ hkStopwatch 名前列）、`[+0x224]`=arg contexts。
- vt `0xA99AE8` = {`0x8CAB80`(D3→call D1 `0x8CA770`+free tag0x32), `0x68F970`, `0x8C9FA0`, `0x8CAA60`} — slot2=**step(frameTime)**（`sub esp,0x21c` の大フレーム、0x917FC0 tick 読取）、slot3=**pollForNewClients**（`0x8CAA60`: 2 重ガード `[+0x58]&&[+0x80]`→stopwatch 差分蓄算→`[[+8] vt+0x24]`=**hkSocket::accept**→受付時 hkIstream 組立（`0x8BBFB0` buffer 0x200）→"A new network client has been received..." ログ→**`call 0x8CA940`(createClient)**→タイマー再装填）＝5.1 の `pollForNewClients()/step()` と仮想 2 本対応。
- `0x8CA2A0` = **shutdown()**: hkError `[[0xBA7FB0]]+0x18`（0x1293adef=311646191 hash?）→"Shuting down Visual Debugger.."@0xA99A64→clients 逆順 deleteClient（0x8C9F30）→"Client deleted."。
- `0x8C9E20` = **serve(port)**: `[+8]==0` ガード→`call [0xB3004C]`（IAT ワンガ＝**WSAStartup**）→ソケット生成（`[0xBA7FB4]` 系 hkSystem ファクトリ経由 0x8BBFB0）→"Server created and will poll for new client(s) on port %d every frame"（0xA999B8/0xA999F0）。呼び主 `0x88B7B0` の 1 件＝enable ラッパ。旁証: `.\hkVisualDebugger.cpp` 実パス文字列 @0xA998DB（アサート用・5.1 と同ファイル名）と**ゲーム側ラッパ TU `.\...\VisualDebugger.cpp`@0xA561F6**（callers 帯の assert 列）。

### 証拠B: createClient `0x8CA940` の全文脈（0x9187A0 生成条件そのもの）
- 列（`v267_i6_out.txt`(a)）: arg2 非null なら `call 0x918BC0`（**m_clients 用の初期 props/WorldMemory パケット送出**: in-place A 構築 `0x948750`→writeU32(0x90)→writeU32([0xB3005C]/[0xB30058])→文字列 "WorldMemory"(0xA9D244) 送出＝5.1 sendMemStatsDump/hkStopwatch プロトコルの 4.0 版）→ m_clients 配列（要素 8B `{+0=arg(stream/sock),+4=0}`、容量 2 倍拡張 `0x8A6E40`）に追加 → **alloc 0x50 tag 0x32→`call 0x9187A0(this, arg1=sock+0x14?, arg2=sock+8?, contexts=self+0x18)`** → `{pair+4}=新 handler` → **`call 0x918B40`（=5.1 のデフォルトプロセス選択: 静的 factory `[0xBA950C]` の名前一覧 [edi+0xc]×[edi+0x10] を周回し、自 handler part(+8) の虚 `[vt+4]`（=`0x9185F0` registerProcess/selectProcess）で登録/選択**）→ clients の各 stream へ `[[c]+8]/[+0x10]` の応答→末尾で新クライアントへ writeByte(5)/writeU32(0) + 0x953130(+8 getter) 経由の stream 操作。
- 引数ゲート（§1/§4 と接続）: `0x9187A0` は arg1(B=hkIstream 系)/arg2(A=hkOstream 系) を**非 null 時のみ**自 object `[+0x14]/[+0x18]` に構築し、D=0x9484E0 は常時構築（[+0x1C]）。→ VDB 出力が生きる条件=「クライアント接続が存在する間だけ」。enable でも接続ゼロなら 0x9187A0 は 1 つも生成されない（v259「表示機能剥離」旁証は「接続時にのみ有効」に読み替え）。

### 証拠C: enable ゲート = bHavokDebug（ini 設定）
- 名文字列 **`bHavokDebug:HAVOK`@0xA46B80**（`v267_i6j_out.txt`(b)）＋値格納 byte `[0xB097B8]` の静的登録列 `0x9E3030`（`push 0xB097B8; mov ecx,0xB07BF0(GameSettings グローバル); call 0x4040E0`=設定項目追加、近隣 float 既定 0）→ **oblivion.ini [HAVOK] bHavokDebug**。
- 呼出 6 点の使われ（`v267_i6e_out.txt`(b)）: 0x445B87/0x4D6401=`push [0xB097B8]`（enable 状態の反映）、0x445628/0x4D4C27/0x4D4DDD/0x4D52D0 側は `push 0`＋前後に「LoadHavokData/SaveQueuedHavokData（0xA3A0A4/0xA3A847 文字列帯）」処理＝**セーブ/ロード・コンソール経路での shutdown→再 enable 列**。`0x88B680` 本体: arg==0 → `[[+0x14] D3]` で vdb を delete（"Shuting down..."）→arg!=0 かつ未生成時のみ alloc 0x74（0x8CB220=hkSocket/registry サブ）→ alloc 0x90 vdb→serve(25001)。**単一インスタンス制限**旁証: "Server has already been created, only one server allowed per visual debugger instance"@0xA998F8（ctor/serve のガード）。

### 証拠D: `0x8CABF0` = viewer 群の bulk registerProcess
- 列 = `0x91E980`（Broadphase）/`0x91E760`/`0x91E090`/`0x91DB30`/`0x91D250`/`0x91CBE0`/`0x91BF90`/`0x91B680`/`0x91AC40`（SweptTransforms）/`0x919FB0`/`0x919440`/`0x919030`/`jmp 0x918DF0`（WorldMemory）の **13 連呼**（`v267_i6e_out.txt`(a)）— v259 §5 表の静的 registerProcess 群（0x91E990 系 head に対して -10 の関数先頭＝同一群）から ObjectInspection/Statistics/DebugDisplay を除いた **物理 viewer 13 本のみ**。＝VDB enable 時にだけ 13 viewer が factory 登録される（OI/St の 0x946xxx/0x947xxx 静的群は呼び主 .text 0 件＝**別経路**、enable 側でなく game 起動側 or 未到達＝残点）。旁証: 静的 17 点のうち `refs(0x918DF0..)` の factory 単体 `[0xBA950C]` 参照 22 点（`v267_i6f_out.txt`(b)）— 0x8CABF0 の 13 点はすべてこの set に内包。

### 裁定
> - **生成条件 = 「ini `bHavokDebug` 有効下で VDB クライアント（ポート 25001）が接続した瞬間」の per-client 生成**（createClient `0x8CA940`@`0x8CA9B4`、m_clients 8B ペアの +4）。デバッグビルド限定ではない（release 同梱）。接続が無ければ 0x9187A0 は 0 個、切断時に shutdown/deleteClient 経路で解放。
> - v259 §4 の「生成主 `0x8CA9B4` = **bhkWorldM のイニシャライザ**」は**所在誤り**（0x8CA900 帯のクラスは vdb=hkVisualDebugger 相当、RTTI 0xA97614 系 bhkWorldM とは無関係な 0xA99AE8）。→ 訂正ドラフト（§7）。
> - S=vdb・0x9187A0=hkServerProcessHandler 相当（**per-client 版**）・createClient=0x8CA940・serve=0x8C9E20・pollForNewClients=0x8CAA60・step=0x8C9FA0・shutdown=0x8CA2A0・enable ラッパ=0x88B680（ゲーム側 `.\\VisualDebugger.cpp` TU）=[確定（相当名・5.1 hkVisualDebugger.h 字段/仮想/ポート/ログ文一致）]。

**判定: 解明（生成条件・全文脈・v259 所在訂正 1 件。OI/St 静的群の呼出経路のみ残点）**（証拠: `v267_i6_out.txt`(a)(b)、`v267_i6b_out.txt`(a)(b)(c)(d)(e)(f)、`v267_i6c_out.txt`(a)(b)(c)(d)(e)、`v267_i6d_out.txt`(a)(b)(c)(d)(e)(f)(g)、`v267_i6e_out.txt`(a)(b)(c)、`v267_i6f_out.txt`(a)(b)(c)(d)(e)(f)、`v267_i6j_out.txt`(a)(b)(c)(d)、SDK 5.1 `Source/Common/Visualize/hkVisualDebugger.h` 全文（本束取得））

---
## §7 まとめ・自己検証・新規残点・訂正ドラフト・使用証拠

### 7-1. 本バッチ（§1〜§6）の判定一覧

| # | 残点（v259 §6-2） | 判定 | 確度 | 主証拠 |
|---|---|---|---|---|
| §1 | 14 表 [13] dtor 重複セル意味 | **解明**（重複ではなく**表境界アーティファクト**: 0xAA2B10=13 セル part vt、0xAA2B44=直後の**同クラス D プライマリ vt slot0**={D3 `0x948510`, 空 ret4}（store 元=`0x9484F5` の ctor のみ）。帰属 D=hkServerDebugDisplayHandler 相当 MI(RefObj@0, handler@+8)。**v259「死テーブル」は誤り**: ctor `0x9484E0`（refcount init 実在）が hkServerProcessHandler 相当 ctor `0x9187A0` の末尾 `0x91891C` で alloc 0x10 tag 0x32→`[+0x1C]=D+8` 保存＝**常時生成の生テーブル**（出力は A 引数ゲート） | 高 | `v267_i1/i1b/i1c/i1d(_out)` |
| §2 | +0x20/+0x2C の 4.0 固有名 | **部分解明**（① Physics listener 404 系譜解消: `Source/Physics/Dynamics/Entity/hkpEntityListener.h`・`.../Phantom/hkpPhantomListener.h` 取得、5 virtual の slot 逐位一致＋`0x4CA2A0`=entityShapeSetCallback 実 body（guard `[arg+8]`→slot2→slot1）→ +0x28 のメソッド名は相当名で確定。② 5.1 `hkProcess.h` 取得: `getConsumableCommands`/`consumeCommand` 既定 body が `0x918CD0`/`0x9266A0` と逐語一致＋4.0 hkProcess 基底 vt=`0xA9D1C0` 6 slot 確定（5.1−isOk）。③ **バイナリ実名発見**: 4.0 の process/viewer 家系は `hkSweptTransformDisplayViewer`/`hkConvexRadiusViewer`/`hkPhantomDisplayViewer`/`hkShapeDisplayViewer`/`hkSimulationIslandViewer`/`hkRigidBodyCentreOfMassViewer`/`hkRigidBodyInertiaViewer`/`hkConstraintViewer`/`hkBroadphaseViewer`（Tt 名表＋`.\visualdebugger\viewer\*.cpp` 4 本）＋メソッド反射名 `sendProps`/`getProps`/`display3lines`/`getMassAndLines`/`draw`。④ +0x2C=props 送受 interface（メソッド名 sendProps/getProps 候補 [推定]）、型名自体は確認できず。+0x20=確認できず（素材不在維持） | +0x28 名=高／家系名=高／+0x2C 名=中／+0x20=低 | `v267_i2/i2b/i2c/i2d(_out)`、`v267_i3_out`、SDK 5.1 4 ヘッダ |
| §3 | node2 隣接 0xA98240 5 slot 表・hkSystem 4.0 ヘッダ所在 | **解明**（0xA98240=hkSystem boot `0x8BBA80` 内 `0x8BBB24` のみが書く「ノード/ストリーム工場」vtbl: {D2 0x8BB3F0, 空, createNode 8B(vt `0xA98254`=hkSocket 系 14 slot), createIStream(0x8BB090→`0x8F5BC0`/`0xA9B38C`・0x1000 buf), createOStream(0x8BB240→`0x8F5F10`/`0xA9B3BC`・0x1000 buf)}、hub は静的 hkRef `[0xBA7FB4]` へ（`0x534070` 割当 helper）。boot の呼び主=`0x88B0F7`（v259 boot④ の本体）。node2 自体は .data `0xB2F1B4` の**CRT 静的初期化子ペア {0x8BB530, 0xBA803C}** で起動時生成、読手=boot 末尾のみ。5.1 に `hkSystem.h` は**存在せず**（`Source/Common/Base/System/hkBaseSystem.h` 関数群＝init/initThread/clearThreadResources/quit に分割・全文取得、本 EXE `0x8BA9C0`/`0x8BBA80`/`0x8BB020` と意味対応）＝4.0 ヘッダ所在は「同名ヘッダは 5.x 側に消失」が構造的回答） | 高（表・生成鎖）／5.1 所在=高・4.0 固有名=低 | `v267_i3b/i3c/i3d(_out)`、hkBaseSystem.h 全文 |
| §4 | 0xAA2B4C/0xAA2B54 stream 系小表の帰属 | **解明**（2-cell {D3, 空}×2＝**hkServerProcessHandler 相当の I/O ストリーム pair の vtbl**。A=`0xAA2B4C`（出力・D の m_outStream `[+0xC]` へ渡される・base=`0xA9D1B8` ostream 系 8 slot・`0x9183A0/0x9184A0` ctor・buffered streambuf `0x8F5F10(this,src,0x2000)`）生成@`0x9188F8`（引数 gate）＋`0x918C56`/`0x918BE5`（`0x918BC0` のメモリ統計送出で in-place）。B=`0xAA2B54`（入力・`[+0x14]`、base ctor `0x9479F0`＝`0xA98328` istream 系・`[arg+8]` addref、D3→`0x8BC000` 復元鎖）生成@`0x9188CF` の 1 箇所、`0x918760`→`0x947FA0` の読み取りストリームとして使用＝役割対確定。tag 0x17 共通・`.\\stream\\impl\\hkBsdSocket.cpp`@0xAA28C2 旁証。4.0 型名=確認できず（hkIstream/hkOstream／hkDisplaySerialize{I,O}Stream 相当 [推定]、5.1Serialize は非仮想リファクタ） | 高（帰属・生成主・役割）／名=低 | `v267_i4/i4b(_out)`、`v267_i1d/i2b(_out)` |
| §5 | 0x9187A0 の 0x8B0E10/0x947EE0/0x947FA0（+0x24 サブ含む） | **解明**（`0x8B0E10`=**hkPointerMap 既定 ctor**（0x80 B tag 0x14 確保・count@+4=0・mask@+8=0xF=16 桶、呼び主 15 件・`0x8B0E60`=map free）。`0x947EE0`=**hkCommandRouter::registerProcess**（process の虚 `[vt+8]` getConsumableCommands→cmd 毎 `0x8B0E80`（golden ratio 0x9E3779B1 insert）で self+8 map へ）。`0x947FA0`=**hkCommandRouter::consumeCommands**（`0x947910` 読込 1 バイト→`0xF0`=5.1 `COMMAND_ACK` 一致で抜、`0x8B0F40`（get）→process 虚 `[vt+0xc]` consumeCommand、未登録は**"VDB: Found a command (%x) with no handler. Could corrupt the stream."**@0xAA2A18→0x8B1990(hkOStream<<)、cmd 0xB0/0xB1 特別。0x918760=process slot5 **step(inStream)**（登録 process の虚 [vt+0x14]=step を回して consumeCommands）。+0x24=`0xA9D1F4`＝**4.0 hkCommandRouter インスタンス**（D2 `0x918690`→map free・`0xA45E5C` 復元＝hkReferencedObject 単一継承＝5.1 と同形）。末尾 `0x918B40`=factory `[0xBA950C]` 走査で 16 process を自 handler part 虚 `[vt+4]`(0x9185F0) 経由 router 登録） | 高 | `v267_i5/i5b(_out)`、hkCommandRouter.h・hkProcess.h 全文 |
| §6 | 0x8CA900 の生成条件 | **解明＋v259 所在訂正**（0x8CA900 帯＝**hkVisualDebugger 相当 TU**（`.\\hkVisualDebugger.cpp`@0xA998DB、5.1 hkVisualDebugger.h と字段一致: +0x88 "Frame Timer" hkStopwatch・+0x58 m_amTimingFrame・vt `0xA99AE8`={D3, 空, step `0x8C9FA0`, pollForNewClients `0x8CAA60`}）。0x9187A0 の生成主 `0x8CA9B4` は `pollForNewClients→createClient 0x8CA940` の中＝**VDB クライアント接続ごとの per-client 生成**（m_clients 8B ペア={socket/stream, server handler}＝5.1 構造一致）。サーバ側 enable はゲームラッパ `0x88B680`（呼出 6 点 0x445xxx/0x4D4-6xxxx、引数=ini 設定 **`bHavokDebug:HAVOK`**@0xA46B80 の値 byte`[0xB097B8]`）→ alloc 0x90 tag0x32→ctor `0x8CA540`（"VDB Server instance has been created"）→ `0x8CABF0`=viewer 13 本の静的 registerProcess 一括→ serve(`0x8C9E20`) **port 0x61A9=25001**（5.1 `HK_VISUAL_DEBUGGER_DEFAULT_PORT` 一致）＋"Server created and will poll for new client(s) on port %d every frame"。デバッグビルド限定でなく、接続皆無なら 0x9187A0 は 0 個） | 高 | `v267_i6/i6b/i6c/i6d/i6e/i6f/i6j/i6k(_out)`、hkVisualDebugger.h 全文 |

**本バッチの新規確定事項（要点）**

1. **14 表は死ではなく生**（ctor 0x9484E0 が 0x9187A0 末尾で常時構築、保持 [+0x1C]）。[13] 重複セル問題は「表境界の誤読」で消滅（13 セル＋2 セルプライマリ）。
2. **4.0 実名ラッシュ**: hkSweptTransformDisplayViewer/hkConvexRadiusViewer/hkPhantomDisplayViewer/hkShapeDisplayViewer/hkConstraintViewer/hkBroadphaseViewer/hkSimulationIslandViewer/hkRigidBodyInertiaViewer/hkRigidBodyCentreOfMassViewer（Tt 名表＋`\visualdebugger\viewer\*.cpp`）。メソッド反射名 sendProps/getProps/display3lines/getMassAndLines。
3. **hkServerProcessHandler 相当（0x9187A0）の解剖完了**: MI(RefObj/handler@+8/process@+0xc)+hkIstream B@+0x14+hkOstream A@+0x18+hkServerDebugDisplayHandler D@+0x1c+**hkCommandRouter@+0x24**+hkArray 2 本＋self 参照[+0x20]。registerProcess/consumeCommands/step の全連鎖＝5.1 hkCommandRouter.h/hkProcess.h と body 級一致。
4. **VDB 起動全経路**: ini `bHavokDebug`→enable ラッパ 0x88B680→vdb 0x90 B→serve(port **25001**)→pollForNewClients→createClient→per-client 0x50 handler。`.data 0xB2F1B4` CRT 初期化子ペアで node2 生成も確定。
5. hkSystem 系: 0xA98240 工場表（createNode/createIStream/createOStream）＋hub 静的 `[0xBA7FB4]`、0xA9D1C0=hkProcess(4.0) 基底 6 slot 確定。
6. 教訓: ①**vt 境界は store 元 imm32 で決める**（連なり CODE 走査は隣接表を喰う—§1/§3 の誤読の根）。②関数境界は int3 パッド切断＋refcount-init 有無で ctor 判別（§1 v259 誤読の根）。③ 5.1 検索は `Source/` プレフィックス必須＋ Physics=Collide/entity は Dynamics/{Entity,Phantom}（404×3 の真因）。

### 7-2. 新規残点（次バッチ候補）

1. **[残] ObjectInspection/Statistics の静的 registerProcess（`0x946560`/`0x947640`、head 帯 `0x946552`/`0x947632`）の呼出経路**（低）: e8 呼び主 .text 0・0x8CABF0 の 13 本に含まれず。factory refs 22 点のうち未帰属数点を追跡（ゲーム側起動時の別 bulk 列 or hkServerProcessHandler ctor 経由のみ？）
2. **[残] WorldMemory/MousePicking の viewer 固有名（hkWorldMemoryViewer? 実在確認）**（低）: 名表プール 0xA9D2C0..0xA9DA40 で未捕捉の分（0xA99xxx server 帯・0xA96xxx/0xA97xxx bhk 名帯に分散の可能性）。
3. **[残] enable ラッパ `0x88B680` 呼び 6 点（0x445628/0x445B87/0x4D4C27/0x4D4DDD/0x4D52D0/0x4D6401）のゲーム側イベント同定**（低）: Load/SaveHavokData・コンソール/デバッグメニュー（VisualDebugger.cpp@0xA561F6 と assert 帯）の内、どれが通常起動でヒットするか（実機ログ要证）。
4. **[残] +0x20（B 系 2 空仮想）のメソッド名・型名**（低）: Tt 名表に候補無し（sendProps/getProps は +0x2C 側と解读）。5.1 hkProcessContext.h との突合で「process context observer」候補の当否。
5. **[残] hkProcess 4.0 の isOk/init/step 位置最終確定**（低）: §5 で slot{2..5} 対応を [確定] に上げる実証（consumeCommand/step の ret 形と 0x918xxx 群の呼出側）。

### 7-3. 自己検証

- **selftest**: 着手時 **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 → ALL PASS）。完了時再実行 4/4 PASS。
- **VA 換算**: 全走査を `v172_common.va2off/rd/disasm/dump・v249_util(refs/e8/vt_dump/head 系)・v217_util(fname/cstr)` に統一（罠非踏襲）。int3 パッド後エントリの独立関数判定（0x9484E0 ctor/0x948C30 B ctor）を本束の全 item で適用。
- **数値の相互整合**: §1 alloc 0x10 tag0x32 ⇔ D3 free tag0x32・`word[+4]=0x10`・[esi+4]=m_outStream（part 視点）三角確認済／§4 A base 8 slot `0xA9D1B8` と §5 hkProcess `0xA9D1C0`=+0xc 内同一（別 TU・隣接）／§5 router map 16 桶⇔0x8B0E10 size0x80 tag0x14（`0x8BAA60` の TLS+0x1AC も tag0x14 圏＝hkMemory class 群と矛盾なし）／§6 m_clients 8B ペア⇔0x8CA98A `lea esi,[eax+ebx*8]`・createClient 末尾 [esi+0x14]/[esi+0x18]⇔D ctor 引数 A=[+0x18] 保存列／§2 hkEntityListener 5 slot⇔hkpEntityListener.h 5 virtual⇔ゲーム側 13 vt slot[3] 共有（v259）三点閉合。
- **負証拠の区分**: **不存在証明（構造 0 件）**= hkSystem.h in 5.1（ツリー構造）・`hkDebugServer/hkServerProcessHandler/hkVisualDebugger` 等の .?AV RTTI（v259 引）＋4.0 ヘッダ public 配布自体の不在（本束 web 再確認）。**確認できず** = +0x20/+0x2C/Q 基底の 4.0 型名、A/B の 4.0 型名、Tensor の viewer 名、D プライマリ slot1 の仮想意味。
- **制約遵守**: git 書込み 0／docs・GT・TODO・plan・session-memory・cache・ソース編集 0／既存 tmp/re 上書き 0（v267 のみ新規・指定名厳守）／ESM 再スキャン 0／cache 参照 0（cache は本束不使用で到達可確認済）。PowerShell ヒアドキュメント不使用・`python -c` 正規表現不使用。
- **中断対策**: §1→§2→§3→§4→§5→§6→§7 の各完了直後に断片→本体追記（断片 `v267_frag1..6.md`＋本節。追記ヘルパ `v267_append.py`）。追記順序の 1 度の重複挿入を `v267_fix.py` で除去・§6 を再追記して復旧（現最終構成 §1〜§7 各 1 部）。

### 7-4. 使用証拠一覧

- **本体**: `tmp/re/v267_process_havok_residuals14.md`（本ファイル・新規）
- **入力（読取のみ）**: `tmp/re/v259_process_havok_residuals13.md`（§6-2 出典本体）/ `v254_*`（part vt 表・14 表）/ `v251_*`（tag/registerProcess 前提）/ `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7）
- **本バッチ中間物**（`%TEMP%\opencode\`）: 共通 `v267_append.py`/`v267_fix.py`。item1: `v267_i1/i1b/i1c/i1d(_out)`。item2: `v267_i2/i2b/i2c/i2d(_out)`。item3: `v267_i3/i3b/i3c/i3d(_out)`。item4: `v267_i4/i4b/i4c(_out)`。item5: `v267_i5/i5b(_out)`。item6: `v267_i6/i6b/i6c/i6d/i6e/i6f/i6g/i6h/i6j/i6k(_out)`。断片 `v267_frag1..6.md`。既存再利用: `v172_common.py`/`v249_util.py`/`v251_util.py`/`v217_util.py`/`v259_rtti.py`（索引は v259 版で代替可）。
- **一次資料（外部・本束取得、リポジトリ `sigmaco/havok-v5.1.0r1-b2007.09.19` branch main・**Source プレフィックス必須**）**: `Source/Common/Visualize/hkCommandRouter.h`・`hkProcess.h`・`hkVisualDebugger.h`・`Serialize/hkDisplaySerializeOStream.h`・`Serialize/hkDisplaySerializeIStream.h`（全文）、`Source/Physics/Dynamics/Entity/hkpEntityListener.h`・`.../Phantom/hkpPhantomListener.h`（全文）、`Source/Common/Base/System/hkBaseSystem.h`（全文）、directory listing 5 本（Source/・Source/Physics/・Source/Physics/Dynamics/・Source/Common/Visualize/・同/Process/・同/Serialize/・Source/Common/Base/System/Io/）
- **Web**: GitHub api/raw 約 15 リクエスト（404×3 含む。教訓: branch=main 旧表記 master 404・sigmaco リポジトリ名に `-b2007.09.19` が必要＝旧記録の `havok-v5.1.0r1` は 404）。websearch 2 回。

### 7-5. 既存記述の訂正ドラフト（本体は未編集・反映は承認待ち）

1. **v259 §3「14 表=本 EXE で未インスタンス化の死テーブル（ctor 書込み 0・不存在証明）」**→「**生テーブル**。ctor `0x9484E0`（imm32@`0x9484F9` の store を v259 は dtor 自己復元と誤読）が hkServerProcessHandler 相当 ctor `0x9187A0` 末尾 `0x91891C` で alloc 0x10 tag 0x32→`[+0x1C]=D+8` に保持。VDB 実運用の handler 実装は 0xAA29B8 側（v259 記述維持）だが、14 表側（=D の part 実装 12  virtual）もクライアント接続時にのみ出力 active」。
2. **v259 §3 表の [13] 行「D2 本体そのもの・[0] の被調整先（重複セル）」**→「[13] に相当する重複セルは存在せず。`0xAA2B10` は 13 セル（[0..12]）で完結、`0xAA2B44` はクラス D のプライマリ vt（2 セル {D3 `0x948510`, 空 ret4 `0x68F970`}）の slot0」。
3. **v259 §4「生成主=ゲーム帯 `0x8CA9B4`（bhkWorldM 系イニシャライザ）」**→「**hkVisualDebugger::createClient（`0x8CA940`）内**、pollForNewClients `0x8CAA60`（vdb vt `0xA99AE8` slot3）の accept 分岐。bhkWorldM との直接関係なし（帯の RTTI 誤帰属）」。
4. **v259 §3 証拠A「残る微疑問は [13] の重複理由のみ（4.0 ヘッダの ~dtor 宣言位置依存）」**→ 消滅（§7-1 §1 参照）。
5. **v259 §2 の無名 home 記述（ST/CR/Ph/Q 家系）**→ 実名反映: `hkSweptTransformDisplayViewer`/`hkConvexRadiusViewer`/`hkPhantomDisplayViewer`/`hkShapeDisplayViewer`/`hkSimulationIslandViewer`/`hkRigidBodyCentreOfMassViewer`/`hkRigidBodyInertiaViewer`/`hkConstraintViewer`/`hkBroadphaseViewer`（+0x28 のメソッド名 = entityAdded/Removed/ShapeSet/DeletedCallback 相当も追記）。
6. **v259 §6-2 残点 2「Physics listener ヘッダは 404×3 継続」**→ 今束で取得済（`Source/Physics/Dynamics/{Entity,Phantom}/hkp*Listener.h`。5.1 の hkp 接頭辞版で、4.0 の `hk*Listener.h` 原典は public 配布に存在せず）。
7. **v254 §3「+0x2C=Q 経由 Bethesda 側 interface（5.1 対応なし）」**（v259 §2 維持）→「5.1 hkProcess.h の getConsumableCommands/consumeCommand と既定 body 逐語一致の props 送受 interface（型名は依然無名、メソッド名候補 sendProps/getProps）」。
8. **v259 §3 証拠B「hkProcessHandler 抽象 vt `0xAA2B58`={空 dtor, purecall×6}」**→境界精密化: `0xAA2B54` は istream 系 B クラスの 2 セル vt の slot1 まで＝抽象 vt は `0xAA2B5C..0xAA2B74`（purecall×6）＋ slot0=`0xAA2B58` の 7 セル構成は妥当だが「空 ~dtor=`0x68F970`@0xAA2B58」は B vt slot1 と同語共有の境界紛帯（store 元 `0x948C45` は B ctor、`0xAA2B58` の store 元は未発見＝`refs=0`）。hkProcessHandler 抽象の先頭は `0xAA2B58` または `0xAA2B5C` のいずれか（次バッチで ctor 群の store 列から確定）。

**判定（束全体）: 6 件中 5 件「解明」＋1 件「部分解明」（+0x20 の固有名のみ素材不在）。v259 に対し死活 1・所在 1・境界 2 の訂正ドラフト提出**
