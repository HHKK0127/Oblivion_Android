# v259_process_havok_residuals13.md — process/Havok 残点第13束（V-251-13）

- 対象: `Oblivion.exe`（Steam v1.2.0416）／出典: `tmp/re/v254_process_havok_residuals12.md` §7-2 の新規残点 5 件
- 分析担当サブエージェント（TODO serial 251 / V251・低）。docs/GT/TODO/plan/session-memory/cache/ソース無編集・訂正はドラフトのみ
- 環境: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7、.text VA=0x401000/raw 0x400、.rdata VA=0xA28000/raw 0x627200、.data VA=0xB02000/raw 0x700C00）、Havok 帯 `0x84xxxx–0xA2xxxx`。VA 換算は `v172_common.va2off()/imm32_refs3()/e8_callers()`・`v249_util/v251_util` に統一（罠非踏襲）
- 前提（v254 確定事項・再検証せず引用）: hkDisplay 派生 8 クラスの vt↔名は m_type 即値走査で確定（SEMICIRCLE=`0xAA2BEC`/CONE=`0xAA2C04`/CONVEX=`0xAA2C14`/AABB=`0xAA2C2C`/BOX=`0xA9D2F4`/CAPSULE=`0xA9D378`/PLANE=`0xA9B304`/CYL9=`0xA9D044`）／vt `0xAA2BA4` の [4] はゼロ引数・init 型（名前一念）／teardown 到達は tail-jmp 2 経路（`0x889802`・`FUN_00533C40` 末尾）／descriptor +8 は未確定のまま（本束対象外）／ST/CR/Ph の part vt=`+0x20/+0x28/+0x2C` 書込み列（v254 §3 表）／別系 node2 vt=`0xA98234`・node4 vt=`0xA9DD5C`・17 番目 tag=`[0xBA840C]`／14 表=`0xAA2B10..0xAA2B44`
- 方針: 各件「[確定]/[推定]/[未確定]＋VA・証拠」。**「確認できず」と「不存在証明」を区別**。
- 中断対策: **1項目終えるたびに即追記**（§1→§6 の順）
- selftest: 着手時 `probe_harness.py selftest` = **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl → ALL PASS）

## 対象 5 件（v254 §7-2 残点 1〜5）

1. SPHERE(=1)/RECTANGLE(=0) の生成箇所（低）— 共有生成経由の推定を実証（残る 2 種の ctor / m_type 書込点）
2. ST/CR/Ph の +0x20/+0x28/+0x2C part のクラス名（低）— 入口=ゲーム帯 `0x4CA2A0`、多重継承 part の実体を RTTI で確定
3. 14 表 `0xAA2B10` の帰属クラス（低）— 5.1 `hkDebugDisplayHandler.h` 全文と位置対応の確定化（web/一次資料可）
4. node2/node4 と 17 番目の tag 世界 `[0xBA840C]` クラスの同定（低）— 形状確定済みのクラス名の当否
5. registerProcess 17 件と tag 世界 16+1 の突合（低）

---
## §1 item1（V-251-13-1）SPHERE/RECTANGLE の生成箇所 — **解明（v254 の「共有生成推定」を訂正）: SPHERE は専用 ctor `0x958590`（m_type=1 を**レジスタ書込み** `mov ecx,1→mov [eax+0x54],ecx` @`0x9585A8` で行うため v254 の C7 即値走査から漏れていた）。生成箇所=node4 dispatcher `0x94A630` のコマンド切替（跳越表 `0x94B780`）の case cmd4 `0x94A6A4`／cmd11 `0x94A763` の 2 箇所と確定。RECTANGLE は**生成箇所が存在しない（不存在証明）**: 全 .rdata+.data を family vt 署名列挙した結果、geometry 系 vt は基底+9 型（SPHERE 含む）で尽き、m_type=0 の ctor/vt はゼロ**

### 証拠A: SPHERE クラス（vt `0xAA3718`・ctor `0x958590`）
- ctor 全文（`v259_i1b_out.txt`）: `push ebp` 後、`mov word[esi+6],1` と同一の参照count を **`mov ecx,1; mov word[eax+6],cx`**（レジスタ形）→ **`mov [eax+0x54],ecx`(=1=SPHERE)** @`0x9585A8` → `movaps [eax+0x10/0x20/0x30]=0`＋`[+0x10]/[+0x24]/[+0x38]=1.0f`（対角 3 語・v254 証拠A テンプレと一致）→ `[eax+0x40]=0`（行列ゼロ、v254 の「単位化」とみこみちがいなし）→ **末尾 `mov [eax],0xAA3718`** @`0x9585D1` → `[eax+0x60]←xmm(arg0 指す 16B)`／`[+0x70]←arg1`／`[+0x74]←arg2`／`ret 0xC`。**v254 テンプレの完全準拠で m_type=1 のため SPHERE ctor と確定**（5.1 `hkDisplaySphere.h` の center(+0x60)+radius 相当追加語と整合）。
- vt `0xAA3718` = {`0x8F4050`(共有 ~dtor), `0x68F970`(空 ret4), `0x9266A0`(draw2d=空 ret4), `0x918000`(draw3d=裸 ret)} — 4.0 の SPHERE は**描画空実装**（旁証: 本ビルドの Visualize は表示機能剥離、v254 §1 証拠C）。
- 生成箇所（`v259_i1h_out.txt`(b)(c)・e8 直接）: `e8(0x958590)={0x94A6A4, 0x94A763}` のみ。**両者とも node4 dispatcher `0x94A630`（v251 §5/§6 のコマンド ID 2..25 切替、跳越表 `0x94B780`）の内部**:
  - case cmd4（表 index2 → handler `0x94A66E`）@`0x94A6A4`: `alloc(align8, size0x80)`→`mov word[eax+4],0x80`（5.1 hkReferencedObject size フィール形）→ `push center*（スタック hkVector4: xmm ゼロ+[esp+0x7c]=[ebx+0xC] の z）; push [edi+8]; push [edi+0xC]; call ctor` → 続けて `0x539980`（hkArray append?）＝**ストリーム読取り値でSphere生成→登録**。
  - case cmd11（表 index9 → handler `0x94A6FC`）@`0x94A763`: 同型（`movaps xmm0,[0xBA7A40]` の静的 hkVector4 を center 雛形に使用）。
- → 4.0 の SPHERE 生成は「実行時引数で型を選ぶ共有生成関数」ではなく、**型別専用 ctor の dispatcher 直刺し**（v254 §7-2 の「共有生成経由」推定は誤り。共有生成機構そのものが存在しない）。

### 証拠B: geometry 系 vtable 全数列挙=RECTANGLE の不在証明
- 走査: `.rdata+.data 全域 0xA28000..0xB32C00` で `{slot0=0x8F4050, slot1=0x68F970}` 一致セル（=hkDisplayGeometry family 署名。基底自 vt `0xA9B2F4` が {共有dtor, 空, purecall×2}＝draw2d/draw3d 純粋で、全derived が slot0/1 を継承するため署名は厳密。`v259_i1g2_out.txt`(a)）。
- 結果 **10 セル = 基底 + 9 型で完全一致**:

| vt | m_type（ctor 実測） | 型 | ctor 入口 | 生成 e8 元 |
|---|---|---|---|---|
| `0xA9B2F4` | — | **基底 hkDisplayGeometry**（純粋2） | — | clone/assign の中間書換のみ（`0x9193BA/0x919F3A/0x91A4FE/0x933DDB`） |
| `0xA9B304` | 8 | PLANE | `0x8F4080` | `0x8B6D63`（server帯直生成）+ dispatcher cmd8 `0x94A965` |
| `0xA9D044` | 9 | CYL(相当) | `0x916380` | `0x8C8603` + cmd5 `0x94AA09` + `0x94C16E` |
| `0xA9D2F4` | 3 | BOX | `0x949300` | **process 帯のみ**（`0x919656/0x919A1D/0x91D346/0x91EBA6`） |
| `0xA9D378` | 2 | CAPSULE | `0x949CA0`/`0x949D00` | cmd7 `0x94AC2F` + `0x94C1BD`（a）／`0x91A1D6/0x91A5D6`（b=ST 帯） |
| `0xAA2BEC` | 6 | SEMICIRCLE | `0x94CCB0` | `0x91C25A`(CR帯) + `0x94A5B0/0x94B07A/0x94B286/0x94BD7B`（server 帯複数） |
| `0xAA2C04` | 5 | CONE | `0x94D270` | `0x91E1E3` のみ |
| `0xAA2C14` | 7 | CONVEX | `0x94D710`/`0x94D790` | cmd23 `0x94A8DD`（a）／`0x91E0DF`（b） |
| `0xAA2C2C` | 4 | AABB | `0x94DAE0` | `0x91E0BE/0x91E0C9/0x91E0D4`（St 帯のみ・3 連成） |
| `0xAA3718` | **1** | **SPHERE** | `0x958590` | **cmd4 `0x94A6A4`／cmd11 `0x94A763`** |
- **m_type=0 RECTANGLE の ctor・vt は上記 10 に含まれず全イメージに 0 件**（署名列挙は構造的被覆率：全 family vt は基底 slot0/1 を継承する必然）。名表 `0xB305C4` の "RECTANGLE" 語は参照 0（v254 §1 証拠C 再現）＝**4.0 でも未リンクの死に型**。→ **不存在証明（生成箇所は存在しない）**。
- dispatcher 跳越表 24 case の ctor 対応（`v259_i1h_out.txt`(a)、線形走査のため重複 handler あり: cmd2/12/13/16/18/20 は同一 handler `0x94AD9F`、cmd14/15/25 に `0x8B1F70` 系、cmd18/21 に `0x94B719` default 系）。ctor を直接呼ぶ case は cmd4/5/7/8/11/23 の 6 種のみ確認＋server 帯直呼出（PLANE/CYL/SEMI/CAPSULEa）。

### 証拠C: 誤候補の棄却（+0x54 が別クラスの別フィールドだった箇所）
- `0x8AA85A`（imm=1）: hkStreambufFactory/hkError 系 TU（vt `0xA97AC4`・[0xB30AAC] シングルトン）の ctor で、同関数末尾に **`mov [esi+0x54],4` の上書き**あり＝状態/種別 int（geometry テンプレ（行列対角/参照count）なし）→ 棄却。
- `0x8DDC56`（imm=0）: `0x8DDC20` は配列 pop/attach 解除系（`[+0x34]/[+0x38]` 配列・`[+0x8c]` word 周回）で ctor ですらない → 棄却。
- `0x955517`（0x3e4ccccd=0.2f）・`0x95A0C6`: vt `0xAA352C`/`0x9602c0` 帯の float 設定列（arrow/style config）で geometry テンプレなし → 棄却。
- `0x9193EB`/`0x919F6B`（`mov [eax+0x54],edx`）: clone/assign 演算子（基底 vt `0xA45E64→0xA9B2F4→終vt` の段階書換＋全フィールドコピー）＝m_type 実行時レジスタ書込みだが**生成ではなく複写**（`v259_i1e/i1h`）。clone 関数は 4 系統確認（BOX/CAPSULE×2/0x933DD0=hkDisplayGeometry::~dtor 系＝[+0x50] ポインタ解放・`0xA45E5C` 基底 vt 復元）。

### 裁定
> - **SPHERE: 確定** — 専用 ctor `0x958590`/vt `0xAA3718`、生成 2 箇所（node4 dispatcher cmd4 `0x94A6A4`・cmd11 `0x94A763`、alloc 0x80 B tag8）。
> - **RECTANGLE: 不存在証明** — 型としてリンクされていない（ctor/vt 全イメージ 0 件）。
> - **v254 訂正ドラフト（§7-5 相当）**: ①§1 末尾「SPHERE/RECT の即値 store が無い＝共有生成経由推定」→ **SPHERE はレジスタ形 store で実在**（走査が C7/81 即値のみ対象だったことの教訓: family 確定は「即値＋レジスタ両形＋vt 署名列挙」の三重必要）。②§1 表に `SPHERE=0xAA3718`（ctor 0x958590）行を追加し「8 クラス」→「9 クラス＋基底」。③RECTANGLE のみ真の死に型。

**判定: 解明（SPHERE 生成箇所 2・RECT 不在証明。v254 の残点記述を訂正）**（証拠: `v259_i1_out.txt`(a)(b)、`v259_i1b_out.txt`、`v259_i1g2_out.txt`(a)(b)(c)、`v259_i1h_out.txt`(a)(b)(c)、`v259_i1e_out.txt`）

---
## §2 item2（V-251-13-2）ST/CR/Ph の +0x20/+0x28/+0x2C part のクラス名 — **部分解明: +0x28 = hkEntityListener（ST/CR）/ hkPhantomListener（Ph）と RTTI で確定（各 ctor が基底 part vt として RTTI 名付きプライマリ vt `0xA46684`/`0xA56320` を一旦書き、自 part vt で上書きする連鎖を発見）。入口 `0x4CA2A0` は両インターフェースの基底実装ディスパッチャ（slot1→slot2 順呼出・[O+8]ガード）で、ゲーム側 12 クラス（bhk/TES/ahk リスナー群）の vt slot[3] と完全一致。+0x20/+0x2C は Visualize/Havok TU のため RTTI 自体が存在せず（COL なしのバイト実証）クラス名=未確定（確認できず）、機能名まで確定**

### 証拠A: RTTI 基盤（本束新規・環境事実）
- 本 EXE の RTTI は `.data` の 0xB02E9C..0xB2FBBC 帯に TD/CHD/COL、COL プール 0xABxxxx–0xADxxxx。**VS6 32bit COL = `{sig=0, offset, 0, TD@+C, CHD@+10}`、TD+8 = 名文字列インライン**（ポインタでなく char[] — 初回解決バグ修正済）。`v259_rtti.py`/`v259_i2b.py`。
- **Visualize/Havok TU の vtable は COL を持たない**（vt-4 に直前スロット値や float 定数 HK_PI 0x40490FDB が並ぶ実測: `0xAA2B10-4=0`/`0xA98234-4=HK_PI`/`0xA9D418-4=0x68F970` など、`v259_rtti.py rtti` 出力）＝物理事務系（bhk*/hk* collision）のみ /GR。→ 名前の機械取得可能範囲が「ゲーム帯クラス＋物理系基底」に限られる（**不存在証明**：Visualize 家系の RTTI 生成自体が無い）。
- `.?AV` 名 1547 件中 hk/bhk 系 176 件。**hkDisplay*/hkProcess*/hkDebugDisplay* 名は 0 件**（`v259_rtti_index_out.txt`）＝VDB 系は RTTI 無しでリンク済（旁証）。

### 証拠B: +0x28 part = hkEntityListener（ST/CR）／hkPhantomListener（Ph）— [確定]
- ctor の基底→派生 書換列（本束の発見・`v259_i2d`/直接 dump）:
  - ST ctor `0x91AA10`: `call 0x9491F0`（Q 中間基底）直後 **`mov [esi+0x28], 0xA46684`** → … → `mov [esi+0x28], 0xA9D404`（自 part vt で上書き）
  - CR ctor `0x91B4B0`: 同一列（`0xA46684`→`0xA9D4F4`）
  - Ph ctor `0x91CD70`: **`mov [esi+0x28], 0xA56320`** → `0xA9D6D4`
- `0xA46684` の COL=`0xABC9D8`→TD=`0xB09534`=`.?AVhkEntityListener@@`、`0xA56320` の COL=`0xABF708`→`.?AVhkPhantomListener@@`（`v259_i2b_out.txt`）。**基底 part vt として派生 ctor が書く＝その MI part の基底クラス名の直接証拠**。
- スロット形状の完全一致（5 slot）:

| | slot0 | slot1 | slot2 | slot3 | slot4 |
|---|---|---|---|---|---|
| hkEntityListener 基底 vt `0xA46684` | `0x4CA2D0`(D2) | 空`0x68F970` | 空`0x68F970` | **`0x4CA2A0`** | 空`0x68F970` |
| hkPhantomListener 基底 vt `0xA56320` | `0x536960`(D2) | 空 | 空 | **`0x4CA2A0`** | 空 |
| ST 派生 part `0xA9D404` | `0x91AA90`(D2 調整 -0x28) | `0x91ADB0` | `0x91AAB0` | `0x4CA2A0`（継承） | `0x68F970` |
| CR 派生 part `0xA9D4F4` | `0x91B530` | `0x91B6A0` | `0x91B550` | `0x4CA2A0` | `0x68F970` |
| Ph 派生 part `0xA9D6D4` | `0x91CDF0` | `0x91D040` | `0x91CD10` | `0x4CA2A0` | `0x68F970` |
- **入口 `0x4CA2A0` の実体**（int3 パッド直後の独立関数、v254 の「FUN_004ca210+0x90」はグループ化アーティファクトと訂正）: `edi=arg → if([edi+8]!=0){ edx=[[this]+8]; call edx(this,edi); edx=[[this]+4]; call edx(this,edi) } ret 4` ＝**基底の「変更通知ディスパッチャ」（slot1,slot2 を順に呼ぶ既定実装）**。
- 同一 `0x4CA2A0` を slot[3] に持つ RTTI 名付きゲーム側 vt（全 13、`v259_i2b_out.txt`）: hkEntityListener(0xA46684)・TESWindListener・bhkEntityListener・TESArrowTargetListener・hkPhantomListener・TESTrapListener×2・TESWaterListener・bhkTelekinesisListener・bhkWindListener・bhkWaterListener・ahkCharacterProxy×2 → **このディスパッチャがゲーム側リスナー実装群と共有される基底実装**であることを交差確認。
- 派生 slot1/slot2 の動作（`v259_i2d_out.txt`）: 引数 O={+8: 配列, +0x14: count} を読み、自身のキャッシュ配列（[self+8]/[self+0xc]）と突き合わせる走査＝**added/removed の 2 通知**（5.x `entityChanged(const hkArray& added, const hkArray& removed)` 相当・Physics ヘッダは 404 で未取得のためメソッド名は [推定]、クラス名は [確定]）。

### 証拠C: +0x20 part — [未確定（確認できず）]・形状/帰属連鎖は確定
- Q 中間基底 ctor `0x9491F0`: `[+0x20]=0xAA2B9C{purecall,purecall}`（参照元は Q ctor のみ 1 件＝抽象 2 槽）→ 同 ctor 末尾で `[+0x20]=0xA9D250{0x9266A0,0x9266A0}`（既定空 2 実装）に上書き。`0xA9D250` は **WM（WorldMemory）プライマリ(+0) と共有**（WM ctor `0x918D4F` が [+0] に書く、参照計 4 件: `0x918D4F/0x918E26/0x94919A(Q D1 復元)/0x949226(Q ctor)`）→ Q と WM が**同一基底 B（2 空仮想クラス）**を共有する構造が確定。
- 派生側（ST `0xA9D418`={D2(-0x20) `0x91B150`, `0x91B2A0`}）: `0x91B2A0` は引数 O の配列（[O+0x14] count/[O+8] base）を self の [self+0x10] hkArray から線形探索（`cmp dword[ebx],edi` 一致検出）＝**「エンティティ所有/有効性判定」1 個のメソッド**。
- RTTI anchor ゼロ（Visualize TU 実証済、§2 証拠A）。5.1 hkDebugDisplayProcess の MI=(hkReferencedObject, hkProcess, hkDebugDisplayHandler) の 3 個のみで +0x20 相当なし（hkInspectProcess 5.1 も MI=(RefObj, Process)）＝**4.0 専用 Bethesda 側 interface**。クラス名は素材不在＝確認できず（不存在証明ではない）。機能名推奨: 「エンティティ照会（B 系 2 槽）インターフェース」。

### 証拠D: +0x2C part — [未確定（確認できず）]・「コマンド源 interface」として機能確定
- 基底 part vt `0xA9D2B4` = {D2 `0x919180`（実装・scalar delete 0x401F20 併用）、**purecall（m1）**, `0x9266A0`（m2 既定空）}。**参照元 19 件すべてが 0x918xxx–0x91Exxx の物理表示 process ctor**（ST/CR/Ph に加え Shapes ほか同帯 16 クラス）＝**Q 経由 process 全家系が実装する interface**（v254 §3 の「19 ctor」内訳を class 単位で初確定）。
- 派生側（ST `0xA9D3F8`={D2(-0x2C), `0x91A720`(ret8), `0x91A910`(ret4)}）: m1 実体 `0x91A720` は `[0xBA9DE4]`×TEB→[+0x1A4/+0x1A8] の**スレッド別リング読出し 2 出力**、m2 `0x91A910` は引数 object +0x50 経由の参照＋処理。シグネチャ（ret8 out×2 / ret4 in×1）は hkProcess の getConsumableCommands（`0x918CD0` = `mov[eax],0;mov[ecx],0;ret 8`）／consumeCommand（`0x9266A0` ret4 空）と同一形＝**「消費可能コマンド列挙／コマンド消費」の display-protocol 版**（5.1 の同名仮想が process 本体に同居していたものの 4.0 分離版と推定）。
- 名前の RTTI anchor なし（同上）。**未確定（確認できず）**。機能名推奨: 「表示コマンド源（consumable/consume）interface」。

### 裁定
> - +0x28 = **hkEntityListener（ST/CR）/ hkPhantomListener（Ph）確定**（RTTI 名付き基底プライマリ vt の part store が決定的証拠。5.1 公布 4.0 非対称＝Physics ヘッダ 404 によりメソッド名は推定）。
> - +0x20 = B{2 空仮想}系（WM プライマリと共有）＋派生 1 実メソッド（エンティティ配列探索）＝形状確定・**名前未確定**。
> - +0x2C = Q 経由 19 クラス共通の 3 槽コマンド源 interface（基底 m1 純粋）＝形状・使用側確定・**名前未確定**。
> - v254 §7-2 残点の前提だった「RTTI で part 実体確定」は +0x28 のみ達成可能（其余は Visualize TU の RTTI 非生成により構造的に不能＝不存在証明側）。

**判定: 部分解明（+0x28 の 2 クラス名は確定、+0x20/+0x2C は機能確定・名前未確定）**（証拠: `v259_i2_out.txt`（impl 参照 census・0x4CA2A0 全文）、`v259_i2b_out.txt`（RTTI 解決一覧）、`v259_i2c_out.txt`（a: 形状対照／b: 1412 プライマリ vt census）、`v259_i2d_out.txt`（各 impl body・CR/Ph ctor 列）、SDK 5.1 `Visualize/Process/hkDebugDisplayProcess.h`・`Visualize/hkDebugDisplayHandler.h`・`Visualize/hkProcessContext.h`・`Visualize/Process/hkInspectProcess.h`（本束取得））

---
## §3 item3（V-251-13-3）14 表 `0xAA2B10..0xAA2B44` の帰属クラス — **解明（位置対応を一次資料で確定化）: 14 表 = hkDebugDisplayHandler（4.0 の純粋仮想 12＋dtor）の具象実装 part vt。**帰属=5.1 の `hkServerDebugDisplayHandler`（MI: hkReferencedObject+0 / handler+8）相当クラスで、**本 EXE では ctor が存在せず生成されない死テーブル（実体側 e8 参照 0・vt 書込み 0 の不存在証明）**。生きた同インターフェース実装は別 TU の `0xAA29B8`（13 slot）で、**4.0 server オブジェクト（primary `0xAA2A04`・hkProcess part `0xAA29EC`・handler part `+0x20`）が ctor `0x9477B0` で実store している**（registerProcess 自己登録「DebugDisplay」→ tag `[0xBA9508]`、item5 と接続）。

### 証拠A: 5.1 一次資料（本束全文取得）との位置対応 — [確定]
- `Visualize/hkDebugDisplayHandler.h`: 純粋仮想は宣言順 **addGeometry, addGeometryInstance(4.5追加), setGeometryColor, updateGeometry, removeGeometry, updateCamera, displayPoint, displayLine, displayText, display3dText(4.6追加), displayGeometry(4引数), displayGeometry(3引数), sendMemStatsDump, holdImmediate**（+ 非純粋 lockForUpdate/unlockForUpdate + 実装 ~dtor）。4.0 は 4.5/4.6 追加 2 本が無く**純粋仮想 12**。
- 抽象側の実測: `0xAA2984` = {`0x947650`(D2 実装), **purecall `0x982D85`×12**} ＝ dtor+12 純粋 = **4.0 hkDebugDisplayHandler 抽象 vt（本数一致）** ✓
- 14 表 slot 実測（`v259_i3_out.txt`(b)・ret 即値=引数語数、v254 の列を再確認）:

| cell | fn | ret | 5.1 宣言対応 | 本束の追加証拠 |
|---|---|---|---|---|
| [0] | `0x9484D0` | — | ~dtor（`sub ecx,8→jmp 0x948510` の part+8 調整 thunk） | scalar deleting dtor 枠 |
| [1] | `0x9485C0` | 0x10 | addGeometry(geoms,transform,id,tag)＝**4.0 は shapeIdHint 無し**（5.1 と 1 語差） | `[esi+4]`=m_outStream の null check＋`0x918440/0x9181B0`(writeByte/writeU32) 連成 |
| [2] | `0x9480A0` | 0xC | setGeometryColor(color,id,tag) ✓ | |
| [3] | `0x948110` | 0xC | updateGeometry(transform,id,tag) ✓ | |
| [4] | `0x948180` | 8 | removeGeometry(id,tag)（hint 無し）✓ | |
| [5] | `0x9481E0` | **0x1C** | **updateCamera(4vec+3real+name) 7 語** ✓（v254 の一致を維持） | |
| [6] | `0x9482A0` | 0xC | displayPoint ✓ | |
| [7] | `0x948310` | 0x10 | displayLine ✓ | |
| [8] | `0x948390` | 0xC | displayText ✓ | |
| [9] | `0x9486D0` | 0xC | displayGeometry(geoms,color,tag) 3引数版 | |
| [10] | `0x948650` | 0x10 | displayGeometry(geoms,transform,color,tag) 4引数版（`0x948550/0x948580`=sendGeometryData 系 helper 呼出） | |
| [11] | `0x948430` | 8 | sendMemStatsDump(data,len) | **cmd 0xF 送出**＝5.1 enum `HK_SEND_MEMSTATS_DUMP=0xF` ✓✓ |
| [12] | `0x948490` | 0 | holdImmediate() | **cmd 0xB 送出**＝enum `HK_HOLD_IMMEDIATE=0xB` ✓✓ |
| [13] | `0x948510` | 4 | （D2 本体そのもの・[0] の被調整先） | 破壊鎖: `[+8]=0xAA2B10→[+8]=0xAA2984→[+0]=0xA45E5C`（自分の handler part vt→抽象→RefObj 基底）＋free tag **0x32** |
- 即値 cmd 一致（0xB/0xF）と引数語数一致 12/12 で**位置対応は確度確定**。残る微疑問は [13] が dtor 本体の重複セルとなる理由のみ（4.0 ヘッダの ~dtor 宣言位置依存、機能判断に影響なし）。

### 証拠B: 帰属と死活 — [確定]（死側は不存在証明）
- **MI 形状**: D2 `0x948510` の復元列 `[+8]→[+0]=0xA45E5C`＝**「+0 に hkReferencedObject、+8 に handler」**（5.1 `hkServerDebugDisplayHandler : public hkReferencedObject, public hkDebugDisplayHandler` と同一順序）。free tag 0x32。
- **死んでいる証明**: `imm32_refs3(0xAA2B10)`=.text 内 2 件（`0x9484FC/0x94851B`＝**dtor 領域自身の自己復元書込みのみ**）。e8(0x948510)=e8(0x9484D0)=0、14 slot の各 fn も e8 呼び主 0（`v259_i3_out.txt`(c)）。**ctor が存在しない＝この vt を書き込む生成経路ゼロ**＝本 EXE で未インスタンス化（VDB サーバ出力の死に実装。旁証: 名前表参照 0・draw2d/3d 空実装＝v254 の「表示機能剥離」）。
- **生きていた別実装（新規確定・item5 への接続）**: ctor `0x9477B0` は `word[+6]=1`→`[+8]=0xA9D1C0`（hkProcess 基底）→**`[+0x20]=0xAA2984`（抽象 handler）**→`[+0]=0xAA2A04 / [+8]=0xAA29EC / [+0x20]=0xAA29B8`（自前 3 part vt で上書き）→`push 0x947860; push "DebugDisplay"@0xA99A94; … call 0x947C50(registerProcess) @0x94780C` → tag を `[0xBA9508]` へ保存。D2 `0x947890` は復元列 `[+0x20]=0xAA2984 / [+8]=0xA9D1C0 / [+0]=0xA45E5C`＝**MI(hkReferencedObject?, hkProcess@+8, hkDebugDisplayHandler@+0x20) の 4.0 サーバオブジェクト**（5.1 では hkServerProcessHandler(RefObj, ProcessHandler, Process) と hkServerDebugDisplayHandler(RefObj, Handler) の 2 クラス；4.0 は 1 クラス統合と読むのが整合的）。
  - 生 handler 実装 vt `0xAA29B8` = {`0x947850`(thunk), `0x947670/0x9476D0/0x9476F0/0x947710/0x947730/0x947740/0x947760/0x947780/0x9476B0/0x947690/0x9477A0/0x9477B0 帯`} 13 slot（dtor+12 実装）。
  - server primary vt `0xAA2A04` = 4 slot {D2 `0x947890`, 空 ret4, `0x947C20`, 空 ret4}、直後 `0xAA2A14` に書式文字列 **"VDBF: … when unable to find the Process"**（VDB Formatted 系ログ＝TU 同定旁証）。
- hkProcessHandler 抽象 vt `0xAA2B58` = {`0x68F970`(空 ~dtor), purecall×6} 7 slot ＝ 5.1 `hkProcessHandler.h`（~dtor{}+6 純粋 registerProcess/getProcessId/getProcessName/selectProcess/createProcess/deleteProcess）と**本数・形状一致**（v251/254 の推定を一次資料で確定化）。隣 `0xAA2B4C/0xAA2B54` の {空 ret4, D2(tag0x17 free)}×2 は in/out stream オブジェクト系 part の小 vt（旁証、帰属名は未確定）。

### 裁定
> **帰属クラス = hkDebugDisplayHandler の具象サーバ実装（5.1 名 `hkServerDebugDisplayHandler` 相当・MI=RefObj+0/handler+8）と確定。ただし本ビルドでは未生成の死テーブル（不存在証明）。実運用の handler 実装は 4.0 サーバオブジェクト（MI: RefObj?+hkProcess@+8+hkDebugDisplayHandler@+0x20・primary vt `0xAA2A04`）の `0xAA29B8`**。4.0 の正確なクラス名（Bethesda 側命名）は RTTI なし・4.0 ヘッダなしのため**確認できず**（5.1 相当名は確度中〜高）。

**判定: 解明（14 表のインターフェース帰属・位置対応・死活・実運用側の代替まで）／4.0 固有名のみ推定**（証拠: `v259_i3_out.txt`(a)〜(f)、`v259_i3b_out.txt`、`v259_i3` 追試 dump（0xAA2984 purecall×12 計数・0xAA29B8/0xAA2A04/0x9477B0 ctor 列）、SDK 5.1 `Visualize/hkDebugDisplayHandler.h`・`Visualize/hkServerDebugDisplayHandler.h`・`Visualize/hkServerProcessHandler.h`（本束全文取得））

---
## §4 item4（V-251-13-4）node2/node4 と 17 番目の tag 世界 `[0xBA840C]` クラスの同定 — **部分解明: ① `[0xBA840C]` の保持者はついに確定 — ctor `0x9187A0` の 4-part MI クラス（primary `0xA9D230`/hkProcessHandler 部 `+8`=`0xA9D214`/hkProcess 部 `+0xc`=`0xA9D1FC`/メンバ配列 `+0x24..`、alloc **0x50 B tag 0x32**、生成主=ゲーム帯 `0x8CA9B4`（bhkWorldM 系イニシャ））。`0xA9D1FC` 部 slot1=`0x918950`=`mov eax,[0xBA840C]; ret` が getProcessTag。**[0xBA840C] への書込み手は全イメージ 0 件（読みは当該 getter のみ）＝常時 0** — 5.1 `hkServerProcessHandler.h` の `virtual int getProcessTag() { return m_tag; } // always just 0` と完全一致 → クラス = **4.0 の hkServerProcessHandler 相当 [確定（相当名）]**。v254 の「17 番目の process タグ」は **registerProcess 未登録のサーバプロセス自身の静的 m_tag** で、tag 世界 17 個目ではない。② node2/node4 は RTTI 非生成 TU に属しクラス名は**未確定（確認できず）**。node4 の「単一 vt」を訂正（ε/π 定数が表を分けており実体 2 slot の registry 形）; node2 の隣接 TU に stack-trace 書式列を発見（hkError/hkDebug 系名残旁証）**

### 証拠A: [0xBA840C] クラス（本束の主要新事実・`v259_i45_out.txt`(a)(c)）
- getter `0x918950`（`mov eax,[0xBA840C]; ret`、int3 パッド後独立エントリ）を slot に持つ vt=`0xA9D1FC`（hkProcess 系 6 slot: {`0x918970`(D2 調整 `sub ecx,0xc→0x918B90`), **`0x918950`(=getProcessTag slot1)**, `0x918540`, `0x918560`, `0x918000`(V4 init 型), `0x918760`（ctx 配列走査＋`call [edx+0x14]`＋`0x947FA0`＝5.1 getConsumableCommands 相当?）。
- ctor `0x9187A0`（`v259_i45`）: `[+6]u16=1`（refcount）→ `[+8]=0xA9D1D8`（hkProcessHandler 基底 vt）→ `[+0xc]=0xA9D1C0`（hkProcess 基底 vt）→ `[+0]=0xA9D230`/`[+8]=0xA9D214`（自前 handler 部 7 slot: D2 0x918690 帯）/`[+0xc]=0xA9D1FC`（自前 process 部）→ `+0x24` 部に別 refcount 子オブジェクト（`[+0x24]=0xA9D1F4`={D2 `0x918690`, …}）→ `0x8B0E10`/`0x947EE0` 呼出＋hkArray `{+0x2C ptr,+0x30 cap,+0x4C flags 0x80000000}`。
- 生成: `e8 callers(0x9187A0)={0x8CA9B4}`＝**0x8CA900 帯（alloc 0x50 tag 0x32 → ctor → `call 0x918B40`）= bhkWorldM のイニシャライザ**（bhkWorldM TD/COL→プライマリ vt `0xA97614` が RTTI 解決済み、`v259_i4_out.txt`(a)）。＝サーバハンドオブジェクトはゲーム側の物理ワールド初期化から作られる。
- **書込み手ゼロの構造証明**: `imm32_refs3(0xBA840C)` は .text で getter 1 件のみ（BSS 領域ゆみ disk 値なし）。17 registerProcess 書込み先（§5）にも非該当。→ m_tag は代入経路を持たず**常時 0**＝5.1 の「always just 0」と意味一致。**判定: 相当クラス確定、4.0 固有名は確認できず**。

### 証拠B: node2（8B・vt `0xA98234`・[0xBA803C]）
- 形状確定（v254 踏襲）: create `0x8BB530` alloc 8 B tag 0x15、vt={D2 `0x8BB3F0`, 空 ret4, `0x918000`}、boot 末尾 slot2 呼出=no-op（既確定）。
- 新規: 本 TU（0x8BB3xx–0x8BB5xx）の D2 群は MI 復元列 `[0]=0xA982A0（4 slot 表 {0x8BB3B0,空,0x8BB360,0x8BB2E0}）→[0]=0xA45E5C（hkReferencedObject 基底 vt・RTTI `0xA45E64` の直前=復元形）+free tag 0x17` を持ち、隣接メソッド `0x8BB020`（0xA98240 表 slot）は **`call 0x8BAA10`→TEB→[0x19C]→`call [hkSystem系 vt+4]`→`0x8A7260(0)`** ＝**スレッド単位の hkSystem 接続/切断処理**（v254 §5 boot④・teardown `0x8BBC03` と対）。
- 書込み先 `0xA982E0..0xA98320` の直後データに **"Stack trace is:" / "(null)" / "%I64i" / "%I64u"**（`v259_i45` 追試 zone dump）＝**スタックトレース出力 TU**。→ node2 = hkSystem スレッド接続フック保持 8B オブジェクト（機能確定）、**クラス名=未確定（確認できず）**（Visualize/debug TU は RTTI 非生成＝§2 証拠A の不存在証明が及ぶ。旁証候補は hkError/hkDebug（5.1 Common/Base/Debug）だが名文字列の直接リンクなし）。

### 証拠C: node4（0x1C B・vt `0xA9DD5C`・[0xBA84F4]）
- 新規確定: ctor `0x91F6B0` の実書込み列 = `mov [eax],0xA9DD5C`（**1 vptr のみ**）＋ `[+6]u16=1`（**refcount 設定＝hkReferencedObject 家系確定**・tag 0x15 free）＋ `{+8,+C}=0` ＋ `{+0x10,+0x14}=0, +0x18=0x80000000}`（**hkArray 2 本**。v254 §5 の「+8,+C 0 /+0x10..+0x18 hkArray」を「配列 2 本(+0x8/+0xC と +0x10/+0x14 の対、flags 0x80000000)」に精密化）。
- **訂正**: vt `0xA9DD5C` の直後語 `ε@0xA9DD64, π@0xA9DD68` に続いて見える {`0x953580`,空,`0x60D0A0`,`0x91FED0`}（0xA9DD6C 帯）は **別クラスの表**（node4 ctor は 0xA9DD5C しか書かない、imm32 参照追跡で node4 由来の書込み無し）。node4 の実 vt は **2 slot（D2+空）＝node3/registry と同型**。v254 §5 表の「node4=0x1C B・{D2,空,…}`0x953580` 等入り」読みを分離（訂正ドラフト §6-3）。
- 死性（踏襲・再現）: `0xA9DD5C` は ctor 1 書込み、slotptr `[0xBA84F4]` の .text 読手 0 → 遅延生成されても参照されない hkReferencedObject シングルトン。クラス名=**未確定（確認できず）**。候補材料: `0x91F720`（D2 呼出す破棄 helper）のみ。

**判定: 部分解明（[0xBA840C]=hkServerProcessHandler 相当・生成経路・常時 0 を確定／node2・node4 は形状+家系確定・固有名未確定。node4 の vt 境界を訂正）**（証拠: `v259_i4_out.txt`(a)(b)(c)(d)、`v259_i45_out.txt`(a)(b)、`v259_i45b_out.txt`、ctor/dtor dump（`0x9187A0`/`0x91F6B0`/`0x91F6F0`/`0x8BB3B0`/`0x8BB020`）、SDK 5.1 `Visualize/hkServerProcessHandler.h`（本束全文・getProcessTag 注释一致））

---

## §5 item5（V-251-13-5）registerProcess 17 件と tag 世界 16+1 の突合 — **解明（全 17 呼び出し点の保存先と登録名を機械確定）: 17 件 → 16 グローバル（`0xBA9508` だけ 2 経路: サーバ ctor `0x94780C` と静的 registerProcess `0x947900`、登録名は両方 "DebugDisplay"）。tag 世界 16 = 登録名 16 と 1:1。v254 が疑った `[0xBA840C]` は 16 の中にも 17 件目の保存先にも**含まれない**（§4 で hkServerProcessHandler 相当の静的 m_tag と確定）。v251 §6「17 本→16 グローバル」が正確であったことを再確認し、名前対応まで完成**

### 証拠A: 17 点対応表（`v259_i45b.py`/関数先頭解析。書式: 各 registerProcess 静的関数 = `mov ecx,[0xBA950C]`（factory 単体）→ push createfn → push name → `call 0x947C50` → `mov [tag],eax`）
| # | 呼び出し VA | 登録名 | createfn | tag 保存先 | v251/254 の対応 |
|---|---|---|---|---|---|
| 1 | `0x918E00` | WorldMemory | `0x918D10` | `0xBA8410` | WM ✓（v251 getter `0x918CF0`） |
| 2 | `0x919040` | MousePicking | `0x918FE0` | `0xBA8414` | 新規名前対応 |
| 3 | `0x919450` | Islands | `0x919410` | `0xBA8418` | 〃 |
| 4 | `0x919FC0` | Tensor | `0x919F80` | `0xBA841C` | 〃（帯名文字列 "Tensor"@0xA9D328 と v254 (d) dump で吻合） |
| 5 | `0x91AC50` | SweptTransforms | `0x91AC10` | `0xBA8420` | ST ✓ |
| 6 | `0x91B690` | ConvexRadius | `0x91B650` | `0xBA842C` | CR ✓ |
| 7 | `0x91BFA0` | Shapes | `0x91BF60` | `0xBA8438` | Shapes ✓（getter `0x91BE10` v251） |
| 8 | `0x91CBF0` | CentreOfMass | `0x91CBB0` | `0xBA8444` | 新規名前対応 |
| 9 | `0x91D260` | Phantoms | `0x91CFE0` | `0xBA8448` | Ph ✓ |
| 10 | `0x91DB40` | **ForcedContactPoints (See Docs For Usage)** | `0x91DAB0` | `0xBA8454` | 新規（名@`0xA9D76C`） |
| 11 | `0x91E0A0` | ContactPoints | `0x91E010` | `0xBA8458` | 〃 |
| 12 | `0x91E770` | Constraint | `0x91E700` | `0xBA845C` | 〃 |
| 13 | `0x91E990` | Broadphase | `0x91E920` | `0xBA8460` | 〃 |
| 14 | `0x946560` | ObjectInspection | `0x946520` | `0xBA9500` | OI ✓ |
| 15 | `0x947640` | Statistics | `0x947600` | `0xBA9504` | St ✓ |
| 16 | `0x94780C` | **DebugDisplay**（サーバ ctor `0x9477B0` 内。名@`0xA99A94`） | （self） | `0xBA9508` | item3 サーバオブジェクト ✓ |
| 17 | `0x947900` | DebugDisplay（静的 registerProcess・`push 0x947860` create 関数） | `0x947860` | `0xBA9508` | **同一グローバル 2 書込み** |
- tag グローバルは 16 個で v251 §6 の列挙（`0xBA8410..0x60`/`0xBA9500..508`）と完全一致。**`0xBA840C` は保存先 17 件に存在せず**（`v259_i45b_out.txt` 全行）。
- createfn 群は 5.1 の `static hkProcess* create(contexts)` と同位置・同形（`0x91AC10`→ST ctor `0x91AA10` を call して `[+8]` 返す等、v254 §2 の「派生 ctor が e8={0x91AA16(ST),…}」と相互整合）。
- 名前 16 件は v251 §3「factory 16 登録名」と 1:1（ForcedContactPoints は v251 factory 名表にあり tag 側の対応が欠けていた 1 件。本束で座標確定）。

### 証拠B: 突合の結論
> 「registerProcess 17 = tag 世界 16 + 1」ではなく、**「17 呼び出し = 16 グローバル（DebugDisplay の 2 重登録）＋ 別枠静的 m_tag（0xBA840C・未登録）」**が正しい内訳。v254 §7-2 の再確認要求（「0x840C が 16 グローバルに含まれていたか」）→ **含まれない（機械全列挙で否定確定）**。

**判定: 解明（17/17 全点の・名前・createfn・保存先確定。0x840C の非所属を証明）**（証拠: `v259_i45_out.txt`、`v259_i45b_out.txt`、窓解析 dump（0x918DF0/0x919030/…/0x9478F0 の各 registerProcess 静的関数先頭））

---
## §6 まとめ・新規残点・自己検証・使用証拠・訂正ドラフト

### 6-1. 本バッチ（§1〜§5）の判定一覧

| # | 残点（v254 §7-2） | 判定 | 確度 | 主証拠 |
|---|---|---|---|---|
| §1 | SPHERE/RECTANGLE の生成箇所 | **解明**（SPHERE=専用 ctor `0x958590`・vt `0xAA3718`・m_type=1 を**レジスタ書込み** `0x9585A8` で持つため v254 の即値走査から漏れていた。生成箇所=node4 dispatcher `0x94A630` の cmd4 `0x94A6A4`/cmd11 `0x94A763`（alloc 0x80 B）。RECTANGLE=geometry 系 vt 全数署名列挙（{slot0=`0x8F4050`,slot1=`0x68F970`} 全 .rdata+.data）で基底+9 型が尽き**不存在証明**。"共有生成経由" 推定は否定） | 高 | `v259_i1/i1b/i1d/i1g2/i1h(_out)` |
| §2 | ST/CR/Ph +0x20/+0x28/+0x2C part のクラス名 | **部分解明**（+0x28=**hkEntityListener**（ST/CR・基底 part vt `0xA46684` を ctor が store）／**hkPhantomListener**（Ph・`0xA56320`）と RTTI で確定。入口 `0x4CA2A0`=両インターフェース共有の基底ディスパッチャ（slot1→slot2 順呼出・[O+8] ガード）、ゲーム側 13 リスナークラス vt と slot[3] 一致。+0x20=B{2 空仮想}系（WM プライマリと共有）・+0x2C=Q 経由 19 ctor 共通のコマンド源 3 槽（getConsumable 風 ret8/consume 風 ret4）＝機能確定・**名前未確定（Visualize TU の RTTI 非生成はバイト実証）**） | +0x28=高／其余=形状高・名低 | `v259_i2/i2b/i2c/i2d(_out)`、5.1 6 ヘッダ |
| §3 | 14 表 `0xAA2B10` の帰属 | **解明**（=4.0 `hkDebugDisplayHandler` 具象実装 part vt（MI: hkReferencedObject+0/handler+8・D2 復元列 `+8:0xAA2B10→0xAA2984→+0:0xA45E5C`・free tag 0x32）。位置対応確定: [1..12]=addGeometry/setGeometryColor/updateGeometry/removeGeometry/**updateCamera([5] ret0x1C)**/displayPoint/displayLine/displayText/displayGeometry×2([9]3 引数,[10]4 引数)/**sendMemStatsDump([11] cmd 0xF)/holdImmediate([12] cmd 0xB**]（5.1 enum と cmd 一致）。抽象 vt `0xAA2984`={dtor,purecall×12} で 4.0 純粋 12 確定。但し** ctor 書込み 0 件＝本ビルド未生成の死テーブル（不存在証明）**。生実装はサーバオブジェクト（MI: primary `0xAA2A04`/hkProcess@+8 `0xAA29EC`/handler@+0x20 `0xAA29B8`・「DebugDisplay」登録）＝4.0 hkServerProcessHandler+Handler 統合形） | 高（帰属・位置）／4.0 固有名=推定 | `v259_i3/i3b(_out)`、hkDebugDisplayHandler.h・hkServer*Handler.h 全文 |
| §4 | node2/node4・[0xBA840C] クラス同定 | **部分解明**（**[0xBA840C]=ctor `0x9187A0` の 4-part MI クラス（handler@+8 `0xA9D214`／process@+0xc `0xA9D1FC`・alloc 0x50 tag 0x32・生成主 `0x8CA9B4`=bhkWorldM 系）の getProcessTag 静的 m_tag、書込み手全イメージ 0＝常時 0**＝5.1 `hkServerProcessHandler.h`「always just 0」と一致→相当クラス確定。node2=hkSystem スレッド接続フック保持（0x8BB020 が TEB→[0x19C]→`call [vt+4]`、隣接 TU に "Stack trace is:" 群）＝形状・機能確定/名前未確定。node4=refcount 1・hkArray 2 本の hkReferencedObject 系 registry（**vt は 2 slot のみ**、0xA9DD6C 帯は別クラスに訂正）/名前未確定） | [0x840C]=高／node2・4=形状高・名低 | `v259_i4_out.txt`、`0x9187A0/0x91F6B0/0x8BB3B0/0x8BB020` dump |
| §5 | registerProcess 17 件と tag 世界 16+1 の突合 | **解明**（17 呼び出し点=静的 registerProcess 群、**保存先 16 グローバルと完全一致＋`0xBA9508` 2 経路（サーバ ctor `0x94780C`・静的関数 `0x947900`、ともに "DebugDisplay"）と確定**（WorldMemory/MousePicking/Islands/Tensor/SweptTransforms/ConvexRadius/Shapes/CentreOfMass/Phantoms/ForcedContactPoints/ContactPoints/Constraint/Broadphase/ObjectInspection/Statistics/DebugDisplay の 16 名）。**`[0xBA840C]` は 17 点の保存先に非包含（機械全列挙で確定）**＝tag 世界 16 の外側・未登録 m_tag） | 高 | `v259_i45/i45b(_out)` |

**本バッチの新規確定事項（要点）**

1. **SPHERE クラス実在**（ctor `0x958590`/vt `0xAA3718`/m_type=1 レジスタ形、生成=node4 dispatcher cmd4/cmd11）＋**RECTANGLE 不在証明**（geometry vt 全数=基底+9）。v254 の「共有生成」仮説は不要化。
2. **+0x28 part = hkEntityListener/hkPhantomListener**（RTTI 基底 part vt store の発見）。`0x4CA2A0` はリスナー基底の通知ディスパッチャでゲーム側 13 クラスと共有。
3. **14 表 = hkDebugDisplayHandler 具象（hkServerDebugDisplayHandler 相当）の死テーブル**。位置対応は cmd 0xB/0xF 即値で確定。抽象 4.0 handler=**純粋 12**（5.1 14 から addGeometryInstance/display3dText 欠）。
4. **サーバ系 2 オブジェクト体系**: 生きている「DebugDisplay」登録オブジェクト（MI: primary/hkProcess@+8/handler@+0x20、`0x9477B0` ctor・tag `[0xBA9508]`）と、**hkServerProcessHandler 相当**（`0x9187A0` ctor・4 part・m_tag `[0xBA840C]` 常時 0・bhkWorldM init から alloc 0x50 tag 0x32）。
5. **registerProcess 17 点=16 名**（ForcedContactPoints の座標初確定: 名@`0xA9D76C`/createfn `0x91DAB0`/tag `0xBA8454`）。
6. 環境新知見: **VS6 COL=`{0, offset, 0, TD@+C, CHD@+10}`・TD 名は TD+8 インライン char[]**（ポインタでない）/ **Visualize/debug TU は RTTI 非生成**（物理事務系 bhk*/hk* collision のみ /GR）/ geometry family vt 署名 = {slot0 `0x8F4050`, slot1 `0x68F970`}。

### 6-2. 新規残点（次バッチ候補）

1. **[残] 14 表 [13] の dtor 本体重複セルの意味**（低）: [0] 調整 thunk と同一関数の二重掲載＝4.0 ヘッダの ~dtor 宣言位置/VS6 part-vt quirks の微裁定。
2. **[残] +0x20 インターフェース（B 系）と +0x2C コマンド源 interface の 4.0 固有名**（低）: RTTI 素材なし。5.1 の Physics 側 hkEntityListener.h/hkPhantomListener.h（Physics/Collision 404 要パス再探索）と hkpDisplay* 系互換ヘッダが出素材。
3. **[残] node2 の隣接メソッド `0x8BB060/0x8BB090/0x8BB240`（vt `0xA98240`）と 5 slot 表の帰属・意味**（低）: hkSystem 4.0 のヘッダ所在不明（Base/System 404・Base/hkStackTracer.h 404）— 5.1 でのパス再試行 or「未参照静的定数」列からの TU 推断。
4. **[残] `0xAA2B4C/0xAA2B54` の {空, D2(tag 0x17)} 2 小表 = stream 系 part の帰属名**（低）。
5. **[残] hkServerProcessHandler 相当（0x9187A0）の +0x24 サブオブジェクト（vt `0xA9D1F4`）と `0x8B0E10`/`0x947EE0`/`0x947FA0` の機能**（中）: router/command-queue 系（5.1 hkCommandRouter.h 未取得—Path 判明済 Visualize/）。
6. **[残] bhkWorldM init 帯 `0x8CA900` の全文脈**（低）: サーバハンドラ生成条件（config ファイル/デバッグビルド限定?).

### 6-3. 自己検証

- **selftest**: 着手時 **4/4 PASS**、完了時再実行 **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 → ALL PASS）。
- **VA 換算**: 全走査は `v172_common.va2off/imm32_refs3/e8_callers`（セクション個別・raw0x400 起点）＋`v249_util/v251_util` に統一。罠非踏襲。**本束の教訓**: ① **TD 名は TD+8 インライン文字列**（初回にポインタ deref 解決バグ→全 name=None、`v259_i2b` で修正）。② **VS6 COL の語順は `{0,off,0,TD,CHD}`**（+8=0 の語義は別ビルドの CHD 位置と紛れるため、**名解決は `td+8` の ".?A" プレフィックス実測で裏取り**）。③ 即値/レジスタ両形 store（§1 の SPHERE 見逃し再現教訓）。
- **数値の相互整合**: §1 vt 列挙 10（基底+9）= v254 表 8＋SPHERE＋基底で矛盾なし／§2 listener 基底 vt 5 slot=ST/CR/Ph part 5 slot と逐語一致（slot3/4 の共有語まで）／§5 17 点の tag 16 個=v251 §6 列挙と完全一致／§4 getter `0x918950` の保持セル `0xA9D200`=`0xA9D1FC+4`（=hkProcess slot1 位置）で v251 の getProcessTag=slot1 と整合。
- **負証拠の区分**: **不存在証明**（構造 0 件）= RECTANGLE ctor/vt（署名列挙全 .rdata+.data）／`[0xBA840C]` 書込み手／`0xAA2B10` の ctor 書込み（dtor 内自己復元 2 件のみ）／Visualize 家系 vtable の COL（vt-4 実測 bytes）／node4 slotptr 読手 0（v254 再現）。**確認できず** = +0x20/+0x2C/14 表 4.0 固有名、node2/node4 名、Physics listener ヘッダ（GitHub 404×3: Source/Physics/Collision・Base/System/hkSystem.h・Base/hkStackTracer.h）、4.0 サーバ 2 クラスの Bethesda 側命名。
- **外部資料（本束取得）**: SDK 5.1（sigmaco/havok-v5.1.0r1）— `Visualize/hkDebugDisplayHandler.h`（全文・enum Command 含む）、`Visualize/hkServerDebugDisplayHandler.h`、`Visualize/hkServerProcessHandler.h`、`Visualize/hkProcessContext.h`、`Visualize/Process/hkDebugDisplayProcess.h`、`Visualize/Process/hkInspectProcess.h`、`Visualize`+`Visualize/Process` ディレクトリ listing（目録から取得パス確定）。websearch 0 回、404 は「不存在証明」でなく**「確認できず」**として扱う。
- **制約遵守**: git 書込み 0／docs・GT・TODO・plan・session-memory・cache・ソース編集 0／既存 tmp/re 上書き 0（新規は本 v259 のみ・指定名厳守）／ESM 再スキャン 0。cache は参照のみ（vtable_index/named_funcs は本束では未参照でも到達可だったため不使用）。
- **中断対策**: §1→§2→§3→§4+§5→§6 の各節完了直後に python 追記（断片 `v259_frag1/2/3/45.md`→本体）を遂行。PowerShell ヒアドキュメント不使用・`python -c` に正規表現不使用（内蔵 str メソッドのみ）。

### 6-4. 使用証拠一覧

- **本体**: `tmp/re/v259_process_havok_residuals13.md`（本ファイル・新規）
- **入力（読取のみ）**: `tmp/re/v254_process_havok_residuals12.md`（§7-2 出典本体）/ `tmp/re/v251_process_havok_residuals11.md`（§3/§6 tag・registerProcess 前提）/ `tmp/re/v249_process_havok_residuals10.md`（系譜・未参照のまま結論に不要と確認）/ `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7）
- **本バッチ中間物**（`%TEMP%\opencode\`）: `v259_rtti.py`＋`v259_rtti_index_out.txt`（RTTI 基盤・1547 .?AV 索索引）。item1: `v259_i1/i1b/i1c/i1d/i1e/i1f/i1g2/i1h(_out)`・`v259_i1g_repro_out.txt`（v254 脚本再現=低バイト imm バグ確認）。item2: `v259_i2/i2b/i2c/i2d(_out)`。item3: `v259_i3/i3b(_out)`。item4/5: `v259_i4/i45/i45b(_out)`。追記断片 `v259_frag1/2/3/45.md`。既存再利用: `v172_common.py`/`v249_util.py`/`v251_util.py`/`v217_util.py`/`v254_i1g.py`（再現）。
- **一次資料（外部）**: 上記 6-3 の 5.1 ヘッダ 6 点＋ディレクトリ listing 2 件
- **Web**: GitHub raw/api 8 リクエスト（6 取得・404×3 のうち 2 は本束新規教訓（Physics/Collision・Base/System）、hkStackTracer 404）。websearch 0。

### 6-5. 既存記述の訂正ドラフト（本体は未編集・反映は承認待ち）

1. **v254 §1 末尾「SPHERE(=1)/RECTANGLE(=0) の即値 store は Havok 帯に無し → 共有生成経由（type 実行時引数）と推定」**: **SPHERE は実在**（ctor `0x958590` の `mov ecx,1→mov [eax+0x54],ecx` レジスタ形＝v254 の C7/81 即値専用走査のこぼし。vt `0xAA3718`・生成箇所=node4 dispatcher cmd4 `0x94A6A4`/cmd11 `0x94A763`）。**RECTANGLE は不存在証明**（geometry 署名列挙で基底+9 型が全て、名表のみ残存）。§1 証拠B 表に「SPHERE=`0xAA3718`/ctor `0x958590`」行追加、「8 クラス」→「9 クラス＋基底」。
2. **v254 §7-1 表§1 行「SPHERE・RECT の ctor 所在=推定」**: 本束 §1 で「SPHERE 確定（高）/RECT 不存在証明」に置換。
3. **v254 §5 証拠A node4 行の vt 記述「`0xA9DD5C` = {D2, 空, 直後{ε,π}, `0x953580`, 空, `0x60D0A0`, `0x91FED0`}」**: node4 の実体 vt は **{D2 `0x91F6F0`, 空} 2 slot のみ**（ε/π が表を分離、0xA9DD6C 帯は別クラス。ctor は `0xA9DD5C` のみ store）。
4. **v254 §2 証拠B「帯の続き 0xA9D1F8 以降に別クラス vt…tag getter `[0xBA840C]` の保持クラス 1（表示 process 系の追加クラス）」**: 保持者特定＝**ctor `0x9187A0` の hkServerProcessHandler 相当 4-part MI（process 部 `0xA9D1FC`@+0xc の slot1）で、tag は registerProcess 経路の外・常時 0**（本束 §4/§5）。
5. **v254 §3 証拠B「14 表=hkDebugDisplayHandler 系 14 virtual と本数一致・位置対応は推定」**: **hkDebugDisplayHandler 具象実装 vt と確定**（MI=RefObj+0/handler+8、5.1 hkServerDebugDisplayHandler 相当・死テーブル）。位置対応は [5]=updateCamera・[11]=sendMemStatsDump(cmd 0xF)・[12]=holdImmediate(cmd 0xB) で**確定側に移動**（抽象 `0xAA2984`={dtor, pure×12} で 4.0 純粋 12 も確定）。
6. **v254 §3 証拠A 表の「+0x28=Q 経由 Bethesda 側 interface（5.1 対応なし）」**: **+0x28 は hkEntityListener（ST/CR）/hkPhantomListener（Ph）**（基底 part vt store `0xA46684`/`0xA56320`＝RTTI 名付き）。+0x20/+0x2C の無名扱いは維持。
7. **v251 §6 証拠A「17 本の registerProcess 呼出し直後すべて mov[tag 世界], eax（保存先 16 グローバル）」**: 再現・維持。詳細追加: 16 名のうち ForcedContactPoints=`0xBA8454`（名@`0xA9D76C`）、`0xBA9508` は "DebugDisplay" の 2 重登録（静的関数 `0x947900` と ctor `0x94780C`）。
