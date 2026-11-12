# v254_process_havok_residuals12.md — process/Havok 残点第12束（V-251-12）

- 対象: `Oblivion.exe`（Steam v1.2.0416）／出典: `tmp/re/v251_process_havok_residuals11.md` §7-2 の新規残点（BSA 項＝残点1 は serial 248 へ移管・LOD 束で解読済のため本束では扱わない）
- 分析担当サブエージェント（TODO serial 247 / V247・低）。docs/GT/TODO/plan/session-memory/cache/ソース無編集・訂正はドラフトのみ
- 環境: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7、.text VA=0x401000/raw 0x400、.rdata VA=0xA28000/raw 0x627200、.data VA=0xB02000/raw 0x700C00）、Havok 帯 `0x84xxxx–0xA2xxxx`。VA 換算はすべて `v172_common.va2off()`（セクション個別）／`imm32_refs3()`／`e8_callers()` に統一（罠非踏襲）
- selftest: 着手時 `probe_harness.py selftest` = **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl → ALL PASS）
- 前提（v251 確定事項・再検証せず引用）: `_initterm` 配列 `0xA283D0..0xA2F77C`（5,579 slot・昇順）・登録 5 ノード連続配置 `0xA2F63C..0xA2F650`・三つ組={FLT_EPSILON,FLT_MAX,HK_PI}（float）・node4/5 への書込み手 0 件（常時空）・hkProcess スロット順 `[0]=dtor(1 slot)+5.1 宣言順`・descriptor `+8` の読手は `0x90E88C`（`0x90E860` 内）のみ
- 方針: 各件「[確定]/[推定]/[未確定]＋VA・証拠」。**「確認できず」と「不存在証明」を区別**。一次資料= Havok 5.1.0r1 公開ソース（`sigmaco/havok-v5.1.0r1-b2007.09.19`。ヘッダ実体あり・.cpp は 168 B スタブ）
- 中断対策: **1項目終えるたびに即追記**（§1→§6 の順）

## 対象 6 件（v251 §7-2 残点 2〜6）

1. hkDisplay 派生 6 クラスの vt↔クラス名対応（低）— 定数 {FLT_EPSILON,FLT_MAX,HK_PI} の帰属先確定
2. vt `0xAA2BA4` の [4]/[5]=isOk/step の向き（低）— 4.0 宣言順の微差裁定
3. ゲーム側派生 process（0x91xxxx 帯）の追加仮想内訳（低）
4. teardown 鎖上位（低）— `0x8BB990`→`0x8BBB9D` の呼び主
5. 起動関数 `0x8BBA80` と別系 4 ノードのクラス同定（低）
6. descriptor `+8` の正式フィールド名（低）— 5.1 Options と非対応のまま。web 検索可

---

## §1 item1（V-251-12-1）hkDisplay 派生6クラスの vt↔クラス名対応 — **解明（新規・機械確定）: 各派生 ctor が `mov [obj+0x54], <N>`（5.1 `hkDisplayGeometry::m_type` の 4.0 実体）を即値で持つため .text 全走査で捕捉し、vtable↔型名 8 件が 1:1 で確定した。定数三つ組の帰属先も「node4/5 registry + pair-hold クラスの TU（非几何）」と分離**

### 証拠A: 基底コンストラクトの共通テンプレート（8 ctor 全部に同型）
`mov word[eax+6],1`（refcount）→ **`mov [eax+0x54], N`（=m_type）** → `movaps [eax+0x10/0x20/0x30]=0` ＋ `[eax+0x10]/[0x24]/[0x38]=1.0f` ＋ `[eax+0x40]=0`（**0x40 B の hkMatrix4 単位化**＝5.1 の `m_transform` 相当。5.1 は hkTransform 0x30 だが 4.0 は行列直保存と差分）→ 末尾 `mov [eax], <vtable>`（`and esp,0xfffffff0` 直後=命令列は 0x54 store と同一関数内。v251 §3 の int3 罠は非踏襲）。

### 証拠B: vt↔型名 対応表（`v254_i1g.py`=mov[reg+0x54]全.text走査、Havok帯のみ抽出。名は 0xB305C4 表の添字で解決）

| vtable | m_type | 型名（表添字） | ctor VA（vptr store） | 派生フィールド（ctor で確認） |
|---|---|---|---|---|
| `0xAA2BEC` | 6 | **SEMICIRCLE** | `0x94CCB0` | `[+0x50]=arg`（ret4） |
| `0xAA2C04` | 5 | **CONE** | `0x94D270` | `[+0x90..0x9C]=0`（半径/高さ/step 等 4 語） |
| `0xAA2C14` | 7 | **CONVEX** | `0x94D710` / `0x94D790`（2変種） | `[+0x60..0x90] ← 引数 4×ポインタ`（ret 0x10 5引数版＝**平面 4×hkVector4**）／ゼロ版 |
| `0xAA2C2C` | 4 | **AABB** | `0x94DAE0`（& 0x94DAD0 = 代入 `movaps [eax],[ecx+0x70]` ret4） | `[+0x60/+0x70]`=xmm0 ゼロ、`[+0x80..0x88]=0` |
| `0xA9D2F4` | 3 | **BOX** | `0x949300`帯 | `[+0x60/+0x70]` ゼロ（halfExtent） |
| `0xA9D378` | 2 | **CAPSULE** | `0x949CA0`帯（2変種） | `[+0x60] ← [ebp+8]` の点コピー／ゼロ版 |
| `0xA9B304` | 8 | **PLANE** | `0x8F4080`（ret 0x14＝6引数） | `[+0x60/+0x70]`＝xmm 2本、`[+0x80/0x84/0x88]=args` |
| `0xA9D044` | 9 | **（=5.1 の CYLINDER 相当・4.0 名文字列なし）** | `0x916380`（PLANE と同一テンプレ） | 同上 |
- SPHERE(=1)／RECTANGLE(=0) の即値 store は Havok 帯に無し → 共有生成経由（type 実行時引数）と推定。**未確定（確認できず。探索は 0x54 store・vptr store の両方式で実施、不存在証明ではない）**。
- v251 §7-2 の残点候補 5 vt のうち **`0xAA2BE4`（ctor 0x94A530/0x94B850）と `0xAA2BD0`（node4/5）は hkDisplay 家系でないことが確定**：BE4 は 0x14 B `{vtable, u16 size/refcnt, +8 ptrA, +C ptrB/copied pair, +0x10=0}` のペア保持、BD0 は registry（v251 §5）。0x54 store ゼロ＝几何基底を持たない。

### 証拠C: 4.0 enum 順の確定（v251 §4「5.1 enum と CYLINDER 差分まで一致」の順序面を訂正）
- m_type 値と名表（0xB305C4: RECT,SPHERE,CAPSULE,BOX,AABB,CONE,SEMI,CONVEX,PLANE）が全件整合 → **4.0（Bethesda ビルド）の enum 順は 表順そのもの**。5.1 公布 enum（RECT,SPHERE,BOX,AABB,CONE,SEMI,CONVEX,PLANE,CAPSULE）とは**中間の並びが別**（CAPSULE が 4.0=2→5.1=8 等）。「CYLINDER=9」の語値のみ 5.1 と一致し、名文字列は本 EXE に無い（9 語のみ .rdata に存在）。
- 名表/名文字列群は .text から参照 0 件（v251 §4 再現。`v254_i1_out.txt` (e)）＝名表示機能は本ビルドで剥がれ済み（旁証）。

### 証拠D: {FLT_EPSILON, FLT_MAX, HK_PI} の帰属
- 三つ組 2 ブロック（`0xAA2BC4..BCC`／`0xAA2BD8..BE0`）は **`0xAA2BD0`（node4/5 registry）と `0xAA2BE4`（ペア保持）を挟む配置**＝この 2 クラスの TU 群の静的定数。両クラスは几何基底（0x54）を持たないため、v251 §4 の「派生 builder の既定値」推定は**三つ組に限れば registry/pair TU の未参照静的**に置換。**追加（本束新規）: 三つ組の第 3 出现 `0xA975F8..600 {ε, FLT_MAX, π}` が別系 node1 の vt `0xA975F0` 直後に並置**（§5 参照）＝別系 4 ノード側 TU（hkSystem/ロックハブ家系）にも同型の未参照静的三つ組が存在。
- 二つ組 {ε,π}（`0xAA2BFC/C24/C3C/C44/C50/C58/C60/C64`）は派生几何 ctor/build 群（SEMICIRCLE〜AABB の TU 列）間に並置（TU レベルのコンスタント列。各 ctor が即値読込みしない＝読手 0 は v251 §4 証拠C 再現）。
- 帰属「クラス名」レベルは本表で確定（SEMICIRCLE/CONE/CONVEX/AABB/… の TU 群＋registry/pair TU の 2 系統）。**「どのメンバ/ローカル用か」は読手 0 のため依然特定不能（確認できず）**。

**判定: 解明（8 クラスの vt↔クラス名を機械確定。SPHERE/RECTANGLE の ctor は確認できず。定数のメンバ名は不能のまま）**（証拠: `v254_i1_out.txt`(a)(d)、`v254_i1b_out.txt`(c)、`v254_i1c_out.txt`/`v254_i1f.py`（ctor 全文）、`v254_i1g.py`（m_type 走査、61 hit 中 Havok 帯 10）、SDK 5.1 `Shape/hkDisplayGeometry.h`/`hkDisplayGeometryTypes.h`/`Shape/hkDisplaySphere.h`（本束で `hkDisplaySphere.h` 新規取得））

---

## §2 item2（V-251-12-2）vt `0xAA2BA4` の [4]/[5]=isOk/step の向き — **部分解明（向き確定・名前一念）: [4]=ゼロ引数 virtual（実体は「戻り値なしのストリーム初期化/ドレイン」型→ init 推定）、[5]=一引数 virtual=step で確定。新規の機械確定: 基底 hkProcess 自身 vt=`0xA9D1C0`（6 slot・[4]/[5] の向きが基底でも同一）と隣接の hkProcessHandler 基底 vt=`0xA9D1D8`（{D2, 純粋仮想6}）。v251 の「[4]=isOk（裸ret）」は戻り値意味論と矛盾し、init 寄りに訂正**

### 証拠A: 派生側 ctor の vt 書込み列（=MI の部分構造。`0x9491F0`=中間基底 ctor・派生 ctor 冒頭が呼ぶ）
```
0x9491F0（中間基底 ctor。派生 6 クラスの ctor 冒頭 e8={0x91AA16(ST),0x91CD78(Ph),0x91B4B8(CR)}…）:
  [esi+8]  = 0xA9D1C0   ←★ hkProcess 基底自身の vt（初回設定）
  byte[esi+0xC] = 1     ← 5.1 hkProcess::m_selectable と位置・意味一致
  [esi+0x20] = 0xAA2B9C ← +0x20 部分の基底 vt（{purecall,purecall} = 純仮想 2 スロットの別 interface）
  [esi]    = 0xAA2BBC   ← 中間基底の primary vt（2 slot: D2 0x9492A0 / ret4 空）
  [esi+8]  = 0xAA2BA4   ← 中間基底の hkProcess 部分 vt（6 slot。v251 の対象）
  [esi+0x20] = 0xA9D250 ← 中間基底の +0x20 部分 vt
  [esi+0x24] = contexts 配列から name 照合で見つけた hkProcessContext 相当（[ctx+4] 呼出し、
               比較対象文字列 @0xA99BD8 = **"Physics"**）を +8 補正して AddRef 保存
```
- D1 `0x949180` が逆向きに [esi]=0xAA2BBC/[+8]=0xAA2BA4/[+0x20]=0xA9D250 → 末尾で [+8]=0xA9D1C0/[+0]=0xA45E5C（基底側 vt へ復元）＝**破壊順序の MI 部分リスト**。D2 `0x9492A0` は free tag **0x32**。
- 派生 ctor 例: Phantoms 0x91CD70 → 0x9491F0 後、5 部すべて書き換え [+0]=0xA9D708(2 slot)/[+8]=0xA9D6F0(**6 slot**)/[+0x20]=0xA9D6E8/[+0x28]=0xA9D6D4/[+0x2C]=0xA9D6C8。ObjectInspection ctor 0x9463E0 は 0x9491F0 を経由せず直接 [esi+8]=0xA9D1C0→[+8]=0xAA2938（6 slot）+[esi]=0xAA2950+[0x2C]=ctx+callback 0x9463B0 登録（0x8CA4D0）。**いずれも hkProcess 部分の仮想数は 6 のまま＝派生はスロットを増やさない（§3 の追加仮想は別 part）**。

### 証拠B: 基底系 vtable 群（`0xA9D1C0` 帯の再読・本束新規確定）
```
0xA9D1C0（6 slot）= hkProcess 基底の自前 vt:
  [0]0x918500(D2 本体: test byte[esp+4],1; ret4) [1]purecall(getProcessTag 純粋)
  [2]0x918CD0 consumable [3]0x9266A0 consume [4]0x918000 裸ret [5]0x9266A0 ret4
0xA9D1D8（7 slot）= hkProcessHandler 基底の自前 vt: {D2 0x918520, purecall×6}
```
- **基底 hkProcess の vt でも [4]/[5] の向き（0 引数→1 引数）は同一**（派生部分 vt と完全同型）。隣接する `0xA9D1D8` の 6 純粋仮想は 5.1 `hkProcessHandler.h`（本束取得）の registerProcess/getProcessId/getProcessName/selectProcess/createProcess/deleteProcess 6 本と**別 vt として本数一致**（1 本の巨大 vt ではなく密接 vtable 列）。
- 帯の続き `0xA9D1F8` 以降も別クラス vt（adjustor `sub ecx,0xc`/`sub ecx,8` 付き）。うち 1 本は tag getter `[0xBA840C]`＝**v251 §6 の tag 世界 16 個の外側の 17 番目の process タグ**（表示 process 系の追加クラス 1 が存在。残点化）。
- 5.1 の hkProcess 7 仮想（~dtor/getProcessTag/getConsumableCommands/consumeCommand/isOk/init/step）に対し本 EXE 基底=6 slot＝**ゼロ引数仮想 1 個（isOk か init）が減**。
- V5 は一引数（ret 4）。5.1 の一引数仮想は step(hkReal)（consumeCommand は [3]）→ **[5]=step 確定**。V4 はゼロ引数（裸 ret）。ゼロ引数候補は 5.1 で isOk と init の 2 つ→**V4 の名前一念は 4.0 ソース不在**。

### 証拠C: 派生 override 実体の戻り値/引数検査（`v254_i1e_out.txt`(e)）
- [4] の実装群: Phantoms `0x91CE10`（[+0x1C]=m_inStream の配列周回、`pop edi;ret`＝**eax 戻り値設定なし**）／ConvexRadius `0x91C100`（同型）／ObjectInspection `0x9461B0`（型索引ストリーム書出 0x9181B0/0x918440/0x918460＋[0xB2FDE4..] キー 4 バイト送出。v251 §2 証拠C の 0x9461DE 帯と同一機能）／Statistics は基底の 0x918000 継承。**全件 0 引数・戻り値なし** → 5.1 `isOk`（hkBool 返却）とは**不整合**、`init`（void）と整合。呼び手の `call [reg+0x10]` 検査は server 帯で該当 0（全 hit が `[0xBA7D98]` allocator 経由の alloc/free と判明、`v254_i2_out.txt`）→ 戻り値を使う箇所は無く、返り値無視でも壊れない＝**init 推定（確度 中）**。
- [5] の実装群: Statistics `0x946FB0`（`ret 4`、+0x1C の ctx 周回＋描画＝5.1 hkStatisticsProcess::step に相当する動作）／ObjectInspection `0x946880`（`ret 4` 実体）→ 一引数＝step として矛盾なし。
- v251 §6 が [5]/[4] に引用した ST `0x91ADB0`/`0x91AAB0` は**本束で [+0x28] 別 part のスロットと確定**（ST hkProcess 部分=0xA9D420 の [4]=0x91B110/[5]=0x9266A0）。「update(O)」は hkProcess でなく +0x28 パートの仮想（§3 で整理）。

### 裁定
> - **向き: 確定** — [4]=ゼロ引数（前）、[5]=一引数 step（後）。5.1 宣言順 (isOk, init, step) のうち**ゼロ引数 2 個のうち 1 個が 4.0 に存在しない**（6 スロット実測）。
> - **名前: 未確定（確認できず）** — [4] の正体 isOk/init のどちらかは 4.0 ヘッダ不在のため決定不能。実装の戻り値欠如から **init 推定（確度中）**。v251 の [4]=isOk 裸ret 読みは「戻り値なし isOk」を追加仮定が必要だった点を訂正ドラフトへ（§7 相当）。

**判定: 部分解明（[5]=step 確定。[4] はゼロ引数スロットとして確定・名は init 推定）**（証拠: `v254_i1b_out.txt`(b)(c)、`v254_i2_out.txt`(a)、ctor/dtor dump（`0x9491F0`/`0x949180`）、raw `0xA9D1C0..0xA9D250`、SDK 5.1 `Visualize/hkProcess.h`/`Visualize/hkProcessHandler.h`（本束再取得・6 純仮想確認）/`Process/hkDebugDisplayProcess.h`（本束取得・MI 構造と m_tag/getProcessTag パターン一致））

---

## §3 item3（V-251-12-3）ゲーム側派生 process（0x91xxxx 帯）の追加仮想内訳 — **解明（内訳を slot 単位で確定）: hkProcess 部分 vt は全派生で 6 スロットのまま（追加仮想なし）。追加は MI の別 part（+0x20/+0x28/+0x2C）と primary part の自前スロットに分散。Q 経由派生（ST/CR/Ph）は 5 part、OI/St/WM は 2〜3 part**

### 証拠A: 派生 ctor の part vt 書込み一覧（`v254_pvt_out.txt` + ctor dump `0x91AA10/0x91CD70/0x91B4B0/0x9463E0/0x946CB0`）

| クラス | primary(+0) | hkProcess(+8) | +0x20 | +0x28 | +0x2C |
|---|---|---|---|---|---|
| SweptTransforms（tag 世界 `0xBA8420`） | `0xA9D438` {D2 `0x91B2E0`, 空 ret4} = 2 | `0xA9D420` {D2(-8) `0x91AA80`, tag getter `0x91AA70`, consumable `0x918CD0`, consume 空, **V4 `0x91B110`**, V5 空} = 6 | `0xA9D418` {D2(-0x20), `0x91B2A0`} = 2 | `0xA9D404` {D2(-0x28), `0x91ADB0`(ret4), `0x91AAB0`(ret4), **`0x4CA2A0`（ゲーム帯 0x4CA210+0x90!）**, `0x68F970`} = 5 | `0xA9D3F8` {D2(-0x2C), `0x91A720`(ret8), `0x91A910`(ret4)} = 3 |
| ConvexRadius（`0xBA842C`） | `0xA9D528` {D2 `0x91BA40`, 空} = 2 | `0xA9D510` {D2(-8), tag `0x91B510`, `0x918CD0`, 空, **V4 `0x91C100`**, 空} = 6 | `0xA9D508` = 2 | `0xA9D4F4` {D2(-0x28), `0x91B6A0`(ret4, sub esp 0x234), `0x91B550`(ret4), `0x4CA2A0`, 空} = 5 | `0xA9D4E8` {D2(-0x2C), `0x91B340`(ret8), `0x91B310`(ret4)} = 3 |
| Phantoms（`0xBA8448`） | `0xA9D708` {D2 `0x91D010`, 空} = 2 | `0xA9D6F0` {D2(-8), tag `0x91CDD0`, `0x918CD0`, 空, **V4 `0x91CE10`**, 空} = 6 | `0xA9D6E8` {D2(-0x20), `0x91CF00`} = 2 | `0xA9D6D4` {D2(-0x28), `0x91D040`(sub esp 0x23C), `0x91CD10`(ret4), `0x4CA2A0`, 空} = 5 | `0xA9D6C8` {D2(-0x2C), `0x91D270`(ret8), 空 ret4} = 3 |
| ObjectInspection（`0xBA9500`） | `0xAA2950` {D2 `0x946570`, 空} = 2 | `0xAA2938` {D2(-8), tag `0x946450`, **consumable 実体 `0x946110`(ret8)**, **consume 実体 `0x946940`（`movzx [esp+4]`＋switch）**, **V4 実体 `0x9461B0`（型索引書出）**, **V5 実体 `0x946880`(ret4)**} = 6 | — | — | — |
| Statistics（`0xBA9504`） | `0xAA297C` {D2 `0x946F80`, 空, `0x947650`(test byte[esp+4],1), purecall×3…} | `0xAA2964` {D2(-8), tag `0x946D70`, `0x918CD0`, 空, **V4=`0x918000`(基底空継承)**, **V5 実体 `0x946FB0`（描画）**} = 6 | — | — | — |
| WorldMemory（`0xBA8410`） | `0xA9D250` {`0x9266A0`,`0x9266A0`} = 2 | `0xA9D258` {D2(-8) `0x918D00`, tag `0x918CF0`, `0x918CD0`, 空, **V4 `0x91DF60`**, **V5 `0x918D60`(ret4)**} = 6 | — | — | — |

- **hkProcess 部分の仮想数は 6 のまま（派生で増加なし）**— [確定]。V4=[4] override のみ全派生で実体化（ST `0x91B110`／CR `0x91C100`／Ph `0x91CE10`／OI `0x9461B0`／WM `0x91DF60`、St は基底空継承）、V5=[5] override は部分実体化（OI/St/WM 実体、ST/CR/Ph 空）— §2 の [4]=init 型・[5]=step 型の帰結と整合。
- **Q 経由 3 クラス（ST/CR/Ph）の +0x28 part** に共有スロット `0x4CA2A0`（**ゲーム側 0x4CA210+0x90、ret4**）が刺さる = engine バンド実体の追加仮想 1 本。+0x2C part は ret8/ret4 の 2 実体（ST `0x91A720`/`0x91A910`）。Q の +0x20 base vt は `{purecall, purecall}`（`0xAA2B9C`）で、ST/CR/Ph は各自 D2(-0x20) thunk + 実体 1〜2 の small vt で上書き — **クラス名なしの Bethesda 側 interface part**（5.1 対応物なし＝[未確定]、機能は display サーバ通信系と推定）。

### 証拠B: handler 系 14 スロット表（v251 §6 の hkProcessHandler 6 純粋仮想の隣）
- `0xAA2B10..0xAA2B44` = code 14 本の列。ret 即値: [0]D2(-8) 系, [1]`0x9485C0` ret 0x10, [2]`0x9480A0` ret 0xC, [3]`0x948110` ret 0xC, [4]`0x948180` ret 8, **[5]`0x9481E0` ret 0x1C（=7 引数）**, [6]ret 0xC, [7]ret 0x10, [8]ret 0xC, [9]ret 0xC, [10]ret 0x10, [11]`0x948430` ret 8, [12]`0x948490` ret 0（push ecx 型）, [13]`0x948510` ret 4（test byte[esp+4],1=flag 形 D2）— 5.1 `hkDebugDisplayHandler.h` のは未取得だが、5.1 `hkDebugDisplayProcess` 宣言の 14 virtual（addGeometry…holdImmediate・本束取得）と**本数 14 で一致**、うち updateCamera の 7 引数（4 vec/real 組 + name）が [5] ret 0x1C と一致（**6 番目宣言=updateCamera とは限らないため位置対応は推定**）。→ [推定・確度中]: この 14 表 = hkDebugDisplayHandler 系 14 仮想の実装テーブル（Visualize TU 帯・imm32 参照 0 の単独配置）。正式帰属（どのクラスの primary vt か）は [未確定]。

**判定: 解明（追加仮想の内訳=6 固定 hkProcess 部分＋MI 別 part スロット＋engine 帯実体 `0x4CA2A0`＋14 表の位置づけまで）／別 part 3 系列のクラス名は 5.1 対応なし＝未確定**（証拠 `v254_pvt_out.txt` 全文、ctor 5 本 dump、SDK 5.1 `Process/hkDebugDisplayProcess.h`（14 virtual 列挙））

---

## §4 item4（V-251-12-4）teardown 鎖上位 — **解明: 0x8BB990（逆順破棄 walker）の唯一 e8 = 0x8BBB9D は teardown 関数 `0x8BBB90`（FUN_008bbb90）の内部。0x8BBB90 本体の呼び主は e8 ゼロで、到達は 2 経路の末尾 `jmp`（tail-call）: (1) `FUN_008897A0+0x62 @0x889802 → jmp 0x8BBB90`（上流: thunk `0x533CD0` の唯一 e8=`0x446961`＝ゲーム破棄関数 `FUN_004466F0`（その唯一 caller=`0x40F511` in `FUN_0040D4D0`）— docs でも `thunk_FUN_008897a0();` として一致）、(2) `FUN_00533C40` の末尾 jmp（docs 229784 `FUN_008bbb90();`、その caller は `0x4D3C91` in `FUN_004D3B10`）。両関数とも直前に `DAT_00ba7904=0` を書く同一尾部**

### 証拠A: teardown 関数 `0x8BBB90` の実体（`v254_i4c_out.txt`/`v254_i45b_out.txt`）
```
0x8BBB90  cmp byte[0xBA8040],1 ; jne 0x8BBC5C         ; boot ガード==1 のときだけ実行
0x8BBB9D  call 0x8BB990                                ; ★逆順破棄 walker（唯一呼出点）
0x8BBBA2  [0xBA7FB0] の refcnt デクリ→0 なら [vt] 破棄 ; 8B オブジェクト解放
0x8BBBC6  [0xBA7FB4] を同型解放、両者=0 クリア
0x8BBBFE  call 0x8BAA10                                ; 内部で walker 帯の解放ループ
0x8BBC03  [0xBA9DE4]スレッドTEB→[0x19C] の hkSystem 系参照 call [vt+4]
0x8BBC29  call 0x8A7260(0) ; call 0x8A70F0(0)          ; hkMemory ハブ/0xC スロットの refcnt 外し（boot の逆）
0x8BBC55  mov byte[0xBA8040],0                          ; ガード解除（v251 §3 の第2書込み手）
0x8BBC5C  xor eax,eax / ret
```
- `e8(0x8BB990)={0x8BBB9D}`（1 件・再現）、`e8(0x8BBB90)=0`・`imm32 refs=0`。E9 走査（`v254_i4d.py`）で `0x889802: jmp 0x8BBB90`（= `FUN_008897A0+0x62`、docs `792918` の `FUN_008bbb90();` と一致。0x8897A0 は `DAT_00ba7904` 解放→末尾 tail-jmp）。**tail-jmp のため e8 走査では出ない**（本束の教訓: 鎖上位は e8+imm32+E9/EB 3 種必須）。
- `thunk_FUN_008897a0 = 0x533CD0`（`jmp 0x8897A0`）の唯一 caller `0x446961`（`FUN_004466F0+0x271`、SEH/GS フレームのゲーム破棄シーケンス: `FUN_0049B6C0 → thunk_FUN_008897a0 → FUN_004B2C80 → FUN_004A08E0`）。`e8(0x4466F0)={0x40F511}`（`FUN_0040D4D0`＝ゲーム側アプリ終了/シャットダウン経路、`[0xB333A0]` オブジェクト delete 直前）。

### 証拠B: 起動条件
- teardown の実行条件 = `[0xBA8040]==1`（boot 成功済み）。boot 側 `0x8BBA80`（v251 §3）と対で、**「boot 成功 → ゲーム破棄 FUN_004466F0（唯一経路）→ tail-jmp teardown → ガード 0 復帰」**。`FUN_00533C40`（0x533C40）側は `DAT_00b35c08 = FUN_00533c40(...)` の生成パスに同居する同一 tail（docs 165864）＝**別文脈（スナップショット系生成?)からの 2 番目の到達経路** [推定]。
- walker `0x8BB990` 自体（`mov esi,[0xBA8188]` 起動＝v251 の CRT リスト head）は逆順解放＋`0x80000080` フラグ群の初期化（head ダンプ）＝**登録リストの全ノード slotptr を 0 に戻す uninit**。

**判定: 解明（呼出主=teardown `0x8BBB90`、その上位 2 経路・起動条件まで確定。e8 だけの走査では見えない tail-jmp 構造を明記）**（証拠 `v254_i4c_out.txt`/`v254_i4d.py`/`v254_i4e_out.txt`、`docs/Oblivion_decompiled.c` 229784/792918/53995/165864 行（読取のみ））

---

## §5 item5（V-251-12-5）起動関数 `0x8BBA80` と別系 4 ノードのクラス同定 — **部分解明: 4 ノードの create 関数・オブジェクト形状・vtable を全数確定。うち 1 件（node2）は boot 末尾の slot2 呼出先が `0x918000` 空関数と確定（＝boot の [0xBA803C] 呼出は実質 no-op）。クラス名の確定は node1=推定 hkError、node3/4=registry 兄弟型として推定どまり**

### 証拠A: ノードレコード 4 件（`.data`、node={createFn, +4=link, +8=slotptr}）
| node | slotptr | create | 割付 | ctor / vt（スロット） | 読手（.text imm32 refs） |
|---|---|---|---|---|---|
| `0xB2EB0C` | `0xBA7DA0` | `0x8A7AD0` | alloc **0x18 B tag 0x32** | ctor `0x8A79B0`: vt **`0xA975F0`** = {D2 `0x8A7AF0`, 空 ret4} 2 slot + hkArray{+8,+C,+0x10=0x80000000} ＋ **vt 直後に三つ組 {ε,FLT_MAX,π}@`0xA975F8..600`** ＋ **+0x14 に 0x18 B tag0x12 ヘルパ＝CRITICAL SECTION**（`push 0x3E8; push ptr; call [IAT 0xA2818C]`＝`InitializeCriticalSectionAndSpinCount`（スピン 1000）と IAT 解決で確定。0x18 B=CS 構造サイズ一致） | 8 件（`0x88A7B6/0x88BAD2/0x88DEA7/0x891C8C/0x8922E5/0x89250F/0x4D4FD9/0x5385AC`＝ゲーム 0x88xxxx/0x89xxxx 帯から多用） |
| `0xB2F1B4` | `0xBA803C` | `0x8BB530` | alloc **8 B tag 0x15** | vt **`0xA98234`** = {D2 `0x8BB3F0`, 空 ret4, **`0x918000`（裸 ret）**, …} （+8 slot2=空） | **1 件＝`0x8BBB66`（boot 末尾）のみ** |
| `0xB30064` | `0xBA8450` | `0x91CF20` | alloc **0x14 B tag 0x15** | vt **`0xA9D6C0`** = {D2 `0x91CF60`(free tag0x15), 空 ret4} ＋hkArray{+8,+C=0,+0x10=0x80000000}＝**node4/5(`0xAA2BD0`) と同一形状の registry** | 1 件＝`0x91D094`（own D2 帯のみ） |
| `0xB30118` | `0xBA84F4` | `0x91F6B0` | alloc **0x1C B tag 0x15** | vt **`0xA9DD5C`** = {D2 `0x91F6F0`(free tag0x15), 空 ret4, **直後 {ε,π} 定数**, `0x953580`, 空, `0x60D0A0`(ゲーム帯 0x4C?→0x60D0A0), `0x91FED0`} ＋ {+8,+C 0}/{+0x10..+0x18 hkArray} | **0 件（slotptr `0xBA84F4` への .text 参照ゼロ）＝書込み専用に近い dead singleton** |

- **boot の `[0xBA803C] call [vt+8]`（v251 §3）= `0x918000` の空関数**と確定（`v254_pvt/v254_i45b` の vt `0xA98234` slot2）。boot 成功手順の最終ステップが no-op virtual ＝「取り消し可能フック」の名残と読む（機能ゼロは確定、意図は推定）。
- node1 は memset 1000 B のメッセージ様バッガを持つ**エラー/メッセージハブ候補（5.1 `hkError` 相当と推定・確度低）**。[0xBA7DA0] がゲーム帯 8 箇所から読まれる＝実使用あり。
- node3 は node4/5 と逐語一致の形状（{0x14 B、2-slot vt、hkArray}）で**DisplayCommandRegistry 兄弟の 3 番目インスタンス**（消費 dispatcher 未確認＝使用箇所ゼロ、v251 §5 の「常時空」系譜）。

### 証拠B: 0x534020/0x534070 レジストリ帯（task の列举要求）
- `0x534020` = [0xBA7FB0] の refcount デクリ+0 時破棄後、引数を [0xBA7FB0] へ格納（swap セッタ）。`0x534070` = [0xBA7FB4] の同型。**teardown `0x8BBB90` が両スロットを 0 で swap 解放**（証拠A §4 と接続）。boot `0x8BBA80` 内の `0x534070`（8 B vptr `0xA98240` オブジェクト）/`0x534020`（alloc 0x28 B tag 0x15 → `0x8BB560` ctor）が格納先。
- hkSystem 系初期化一覧（boot 関数 `0x8BBA80` の順次）: ①状態取得 `0x8BA9C0` → ②`0x8A70F0(esi)`（hub refcnt） → ③CRT new 0x330 B → ④`0x8A7260`（[0x19C] スワップ） → ⑤`[0xBA7D9C]` 格納 → ⑥`[0xBA7D98]` アロケータ alloc(8,tag0x15)→`0xA98240` vptr 8B→**`0x534070`** → ⑦alloc(0x28,tag0x15)→`0x8BB560` ctor→**`0x534020`** → ⑧walker `0x8BB420` 初回（9 生成） → ⑨[0xBA803C] slot2（no-op） → ⑩[0xBA8040]=1。

### 裁定
> - 4 ノードの**生成・形状・到達 = 確定**。クラス正式名: node1 hkError 様 [推定]／node2 8B フックホルダ（boot no-op 先）[形状のみ確定・名前未確定]／node3 registry 兄弟 [推定]／node4 参照ゼロ dead singleton [未確定]。4.0 ヘッダ素材待ちの残存課題として明示（確認できず、不存在証明は形状側に限り成立）。

**判定: 部分解明（4/4 の形状・boot 呼出 no-op=確定。名前は node1 のみ推定）**（証拠 `v254_i45b_out.txt` (b)(c)、`v254_pvt_out.txt`（vt 4 本）、`v254_i45.py`(b) refs、boot 本文 v251 §3 証拠C）

---

## §6 item6（V-251-12-6）descriptor `+8` の正式フィールド名 — **未確定（確認できず）。5.1 一次資料 3 本を再取得して候補を絞ったが 1:1 の正式名は成立せず。機能名は v251 のまま「ルートコンテナ系エクスポートセット収集ゲート（常時 ON）」**

### 証拠A: 5.1 公布 Options との照合（本束で `Serialize/Packfile/hkPackfileWriter.h` 全文取得・一次資料）
- 5.1 `hkPackfileWriter::Options` の実体 = `{ hkUint32 m_userTag(+0); hkStructureLayout m_layout(+4); hkBool m_writeMetaInfo(+8); const char* m_contentsVersion(+0xC); }`。**+8 に hkBool が現れる（m_layout が 4 B 単体と仮定した場合の）位置は本 EXE descriptor{+0 name 4 字符 inline, +4 object ptr, +8 u8=1} と数値一致し得る**が、Options 本体は 16 B で本 temp(0x4C B)/descriptor(12 B) とサイズ不整合 — v251「5.1 Options と 1:1 にならない」を再確認（保持）。
- `m_writeMetaInfo` の意味（コメント直訳）=「各オブジェクトの hkClass を保存するか否か」＝descriptor+8 がゲートする**型索引（hkClass `0xBA8788` 照合→{ptr,size} ペア収集）の ON/OFF と機能が一致** → **意味的対応候補: writeMetaInfo/writeTypeinfo** [推定・確度低。4.0 期の実際のメンバ名は公布ソースに存在せず]。
- `hkBinaryPackfileWriter.h`（5.1・本束取得）: 追加 API は save/classnames 節のみで bool 候補なし。`hkSerialize.h`（5.1）は include スタブで実体なし（無関係確認済み）。

### 証拠B: Web 探索（本束 2 クエリ、全部 DuckDuckGo webfetch＝websearch クォータ枯）
| # | クエリ | 結果 |
|---|---|---|
| 1 | `"hkSerialize" "writeTypeinfo" havok`（DuckDuckGo） | **0 件**（4.0 期 `writeTypeinfo` 様の命名は公開コードに存在せず） |
| 2 | `havok hkSerialize createPackfileFromContainer`（DuckDuckGo） | 3〜10 位は botw_havok/HKX2（2012+ 世代）・deepwiki 等の**無関係ヒット**。4.0 期署名を示す資料ゼロ |
- grep.app は v251 で 429 再現以降、本束では未リトライ（手段枯渇）。**「確認できず」であって「4.0 に別名が存在する証明」でも「存在しない証明」でもない**（4.0 SDK 非公開＝素材不存在が構造的制約）。
- descriptor 自体の機能的命名（v251 §2 の読手 1 件・ゲート先・死分岐まで確定済み事項）に変更なし。本束で追加したのは 5.1 Options の **`+8 hkBool m_writeMetaInfo` 位置一致**という旁証 1 点のみ。

### 裁定
> **正式フィールド名 = 未確定（確認できず）**。最有力の意味対応候補 = 5.1 `hkPackfileWriter::Options::m_writeMetaInfo`（位置+8・機能一致・確度低）。4.0 期固有名は公開資料ゼロ（web 2 クエリ 0 件＋grep.app 429＋4.0 SDK 非公開）。将来手段: Oblivion 世代（HK4.0）の他ゲーム SDK ヘッダ断片流出や、BSA/LOD 系で既知の 2012 世代 `hkSerialize` ドキュメントとの差分照合。

**判定: 未確定（確認できず。候補 1・旁証 1 を記録）**（証拠: 本節表、SDK 5.1 `Serialize/Packfile/hkPackfileWriter.h`（本束全文）・`Packfile/Binary/hkBinaryPackfileWriter.h`（本束）・`hkSerialize.h`（本束、中身なし）、v251 §2）

---

## §7 まとめ・新規残点・自己検証・訂正ドラフト

### 7-1. 本バッチ（§1〜§6）の判定一覧

| # | 残点（v251 §7-2） | 判定 | 確度 | 主証拠 |
|---|---|---|---|---|
| §1 | hkDisplay 派生6クラスの vt↔クラス名 | **解明**（`mov [obj+0x54],m_type` 即値で SEMICIRCLE=`0xAA2BEC`／CONE=`0xAA2C04`／CONVEX=`0xAA2C14`／AABB=`0xAA2C2C`／BOX=`0xA9D2F4`／CAPSULE=`0xA9D378`／PLANE=`0xA9B304`／CYL(9)=`0xA9D044` を機械確定。4.0 enum 順=名表順と確定。三つ組帰属=registry/pair TU＋node1 TU へ分離） | 高（8 クラス）／SPHERE・RECT の ctor 所在=推定 | `v254_i1g.py` 走査、`v254_i1f/i1b` |
| §2 | vt `0xAA2BA4` の [4]/[5] の向き | **部分解明**（[5]=step(hkReal) 確定。[4]=0 引数スロット確定・名は init 推定（override 実体群が全て戻り値なし＝isOk 説と不整合）。新規: 基底 hkProcess 自身 vt=`0xA9D1C0`（6 slot・同型）と隣接 hkProcessHandler 基底 vt=`0xA9D1D8`（D2+純粋6）。Q=hkDebugDisplayProcess 系中間基底・"Physics" ctx 探索・boot の slot2 呼出は no-op） | 向き=高／[4] 固有名=低 | `0x9491F0` ctor、raw `0xA9D1C0..`、5.1 `hkDebugDisplayProcess.h`/`hkProcessHandler.h` |
| §3 | ゲーム側派生 process の追加仮想内訳 | **解明**（派生でも hkProcess 部分は 6 スロット固定。追加仮想は MI の別 part（+0x20=2／+0x28=5／+0x2C=3）と primary part 自前スロット（St `0x947650` 等）に分属。+0x28 part の slot[3]=`0x4CA2A0`（ゲーム帯 0x4CA210 実体 override）。OI は 6 中 5 を実体化。14 表 `0xAA2B10`＝hkDebugDisplayProcess の handler 系 14 virtual 実装列と本数一致（updateCamera 7 引数=ret 0x1C が [5] に一致・位置対応は推定）） | 高（形状）／帰属クラス=中 | `v254_pvt_out.txt` 25 テーブル |
| §4 | teardown 鎖上位 | **解明**（`0x8BB990` ← 唯一 `0x8BBB9D` ∈ teardown `0x8BBB90`（ガード `[0xBA8040]==1`）。`0x8BBB90` への e8/imm32=0、到達は 2 経路の tail-jmp: ①`0x889802`（`FUN_008897A0+0x62`←thunk `0x533CD0`←`0x446961` `FUN_004466F0`（ゲーム破棄・単一 caller `0x40F511`））②`FUN_00533C40` 末尾（docs 229784）。teardown 内容=walker→[0xBA7FB0/FB4] 解放→CS/hub 解除→ガード 0） | 高 | `v254_i4c/i4d/i4e`、docs 4 箇所 |
| §5 | 起動関数と別系 4 ノードのクラス | **部分解明**（4 ノードの create/形状/vt/読手 全確定: node1=0x18B・CS(spin1000) 内蔵スレッドセーフハブ・読手 8、node2=8B no-op フック保持（boot slot2 呼出先=`0x918000` 空と確定）、node3=`0xAA2BD0` 同款 registry（0x14B）、node4=0x1C B・slotptr 読手 0 の dead。0x534020/70=[0xBA7FB0]/[0xBA7FB4] swap セッタ） | 形状=高／クラス名=低 | `v254_i45b/i45.py`、`v254_pvt`、`v254_iat.py`（IAT 解決） |
| §6 | descriptor `+8` の正式フィールド名 | **未確定（確認できず）**（5.1 Options 全文で `+8 hkBool m_writeMetaInfo` の位置・機能一致旁証を得たが 1:1 不成立（サイズ不整合継続）。web 2 クエリ 0/無関係。4.0 固有名は素材不存在が構造的制約） | 低 | `Serialize/Packfile/hkPackfileWriter.h`（本束全文）、§6 表 |

**本バッチの新規確定事項（要点）**

1. **hkDisplay 几何 8 クラスの vt↔型名が 1:1 で確定**（m_type 即値 store 方式）。CONVEX=平面 4×hkVector4、PLANE/CYL=6 引数 ctor 等、派生フィールドも同時確認。三つ組 {ε,FLT_MAX,π} の第 3 出現が node1 vt 直後 `0xA975F8..600`。
2. **基底 hkProcess の自前 vt=`0xA9D1C0`（6 slot）＝派生部分 vt と完全同型**（[4] 裸 ret／[5] ret4）。隣接 `0xA9D1D8` は hkProcessHandler 基底 vt（D2+純粋仮想6・5.1 と本数一致）。帯内の追加 vt に **17 番目の process タグ `[0xBA840C]`** の保持クラスが在る。
3. **派生ゲーム process で hkProcess 部分仮想は 6 のまま**（追加仮想は Bethesda 側 MI part と primary 側。+0x28 part にゲーム帯実体 `0x4CA2A0`）。
4. **teardown 到達 = tail-jmp 2 経路**（e8 走査のみでは見えない。教訓: 鎖上位は e8+imm32+E9/EB/0F8x の 3 種併査）。
5. **boot 最終ステップ [0xBA803C] slot2 = 空関数 no-op**（`0x918000`）。node1/factory の 0x18 B ヘルパは **CRITICAL_SECTION**（`[0xA2818C]=InitializeCriticalSectionAndSpinCount`、spin 1000/2000）。

### 7-2. 新規残点（次バッチ候補）

1. **[残] SPHERE(=1)/RECTANGLE(=0) の生成箇所**（低）: 0x54 即値 store ゼロ＝共有生成（type 実行時引数）経由と推定。呼び出し元からの type 引数経路の特定。
2. **[残] ST/CR/Ph の +0x20/+0x28/+0x2C part のクラス名**（低）: Bethesda 側 interface 3 系列（5.1 対応なし）。入口＝ゲーム帯 `0x4CA2A0`（FUN_004ca210+0x90, ret4）の機能解析。
3. **[残] 14 表 `0xAA2B10..0xAA2B44` の帰属クラス**（低）: hkDebugDisplayHandler 系 14 仮想と本数一致だが refs ゼロ（関数ポインタ経由の証拠なし）。5.1 `hkDebugDisplayHandler.h` 全文取得で位置対応の確定化。
4. **[残] 別系 node2（8B hook）/node4（0x1C B dead）の正式クラス名と、17 番目のタグ `[0xBA840C]` クラスの同定**（低）: 形状確定・名前素材（4.0 ヘッダ）不在。
5. **[残] v251 §6 の「登録 17 件」系譜との突合**: tag 世界は 16（v251）＋`0xBA840C`（本束）の計 17 候補。17 件の registerProcess 先との対応完成（`0x947C50` 直後の `mov [tag],eax` 16 グローバルに 0xBA840C が含まれていたかの再確認）。

### 7-3. 自己検証

- **selftest**: 着手時 4/4 PASS（ヘッダ記載）。**完了時再実行 → 4/4 PASS**。
- **VA 換算**: 全走査は `v251_util`/`v172_common` のセクション個別 raw（`imm32_refs3`/`e8_callers(.text raw 0x400)`/`va2off`）に統一。**本束の教訓 2 件**: ① **tail-jmp 到達**（§4）＝e8/imm32 ゼロでも E9/EB/0F8x に有意の参照が在る。② jmp 教示スキャンでは `.text` の `va=0x400C00+i` 誤り（正: `0x401000+i`＝raw0x400 起点）を途中検出・修正済（断片内スクリプトは修正版使用、結論不変）。
- **数値の相互整合**: §1 の m_type 値（2〜9）と名表添字の解決が全件矛盾なし／§3 の part 間隔（+0x20/+0x28/+0x2C）と adjustor 即値（-0x20/-0x28/-0x2C）一致／§4 ガード書込み 2 手（0x8BBB6F/0x8BBC55）は v251 §3 と再現。
- **負証拠の区分**: 不存在証明（構造的 0 件）= node4 slotptr 読手 0／`0x8BBB90` の e8/imm32 0（tail-jmp 2 件のみ）／14 表 imm32 refs 0／hkDisplay 定数群の imm32 読手 0（v251 再現）。確認できず = SPHERE/RECT 生成、4.0 固有名（Options/表示几何メンバ/node2/4）、grep.app 全文検索（429 前例、本束未リトライ）。
- **外部資料（本束取得）**: SDK 5.1（sigmaco/havok-v5.1.0r1）— `Visualize/Shape/` listing、`Shape/hkDisplayGeometry.h`、`Shape/hkDisplaySphere.h`、`Visualize/hkProcessHandler.h`、`Process/hkDebugDisplayProcess.h`、`Serialize/Packfile/hkPackfileWriter.h`、`Serialize/Packfile/Binary/hkBinaryPackfileWriter.h`、`Serialize/hkSerialize.h`（中身なし確認）。websearch クォータ到達（0 使用）、DuckDuckGo webfetch 2 クエリ。
- **制約遵守**: git 書込み 0／docs・GT・TODO・plan・session-memory・cache・ソース編集 0／既存 tmp/re 上書き 0（新規は本 v254 のみ・指定名厳守。v252 欠番/v253/v256 非 touch）／ESM 再スキャン 0。
- **中断対策**: §1→§2→§3〜§6→§7 の順に**節完成の都度即追記**（計 3 回の応答中断はいずれも追記済み内容を保持して再開）。ヒアドキュメント禁止遵守（追記は python 追記方式で統一）。

### 7-4. 使用証拠一覧

- **本体**: `tmp/re/v254_process_havok_residuals12.md`（本ファイル・新規）
- **入力（読取のみ）**: `tmp/re/v251_process_havok_residuals11.md`（§7-2 出典本体）/ `v249_process_havok_residuals10.md` / `v247_process_havok_residuals9.md` / `tmp/re/cache/{named_funcs,vtable_index,call_targets}`（fname/call_count 参照）/ `docs/Oblivion_decompiled.c`（§4 で 4 箇所読取）
- **本バッチ中間物**（`%TEMP%\opencode\`）: `v254_i1/i1b/i1c/i1d/i1e/i1f/i1g(_out)`（item1）、`v254_i2(_out)`・`v254_pvt(_out)`・`v254_i2c`（item2/3）、`v254_i45/i45b/i4c/i4d/i4e(_out)`（item4/5）、`v254_iat.py`（IAT 解決: CS/関数名）、`v254_frag12/frag3456/frag7.md`（追記断片）、既存 `v251_util.py`/`v249_util.py`/`v172_common.py` 再利用
- **一次資料（外部）**: 上記 7-3 の 5.1 ヘッダ 8 点
- **Web**: DuckDuckGo webfetch 2 クエリ（§6 表）。websearch クォータ到達（0 回）。

### 7-5. 既存記述の訂正ドラフト（本体は未編集・反映は承認待ち）

1. **v251 §4 証拠A 表「0xAA2BC4/BD8 の三つ組→node4/5・0xAA2BE4 の直前」**: 三つ組の**第 3 出現 `0xA975F8..600`（別系 node1 vt `0xA975F0` 直後）を追加**（本束 §1 証拠D）。帰属は「派生 builder の既定値」でなく「各 TU の未参照静的」に統一。
2. **v251 §7-2 残点2 の候補列挙「0xAA2BE4/BEC/C04/C14/C2C が派生6クラス」**: 正しくは **BE4/BD0 は非几何**（ペア保持/registry。0x54 store ゼロ）、名前の付く几何 8 クラスは §1 証拠C の表（BEC=SEMICIRCLE ほか）で確定。
3. **v251 §6 証拠B「[4]=0x918000=isOk 4.0 版 `{}` と推定/[5]=step」**: 向き（[4]0 引数→[5]1 引数）は再現・維持。**基底自身 vt `0xA9D1C0` でも同型と確認**、かつ override 実体が全件「戻り値なし」＝**[4]=init 型に訂正（isOk 説は要追加仮定）**。
4. **v251 §6 証拠C「process の update（例 `0x91ADB0`）…入力を渡す」**: `0x91ADB0` は **ST の +0x28 part（Bethesda 側 MI part）スロット**で hkProcess の仮想ではない（§3 証拠A）。ST の hkProcess 部分 [4]=`0x91B110`／[5]=空。
5. **v251 §6 裁定「hkProcess 家系=5.1 宣言順 1:1（dtor 1 slot）」**: 5.1 比で**ゼロ引数仮想 1 個減の 6 スロット**に更新（基底 vt 実測 §2 証拠B）。
6. **v251 §3 末尾「teardown walker の e8=1 件 `0x8BBB9D` まで確認」を拡張**: 上位鎖確定は §4（tail-jmp 2 経路・呼び主 2 関数）。
