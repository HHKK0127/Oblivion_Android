# v241_port_mgef_residuals6.md — 移植 MGEF 残点第6束（N-21..N-23 裁定）

- 作成: 2026-11-02 / 担当: TODO serial 234（V234・低）/ **分析専任（docs/GT/TODO/ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v239_port_mgef_residuals5.md` §4 新規残点 N-21..N-23（本体は読取のみ・無編集）
- 手法: EXE 再逆アセンブル（capstone 5.0.7・PE 手動解析・VA=0x400C00+file_off）＋ `docs/Oblivion_decompiled.c` 部分読取（読取のみ）＋ cache（named_funcs/vtable_index/call_targets）突合。ESM 本体は未アクセス（再スキャン禁止遵守）。git 不使用。
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**
- 結論早見: 項1 **【解明・前提訂正】セーブゲーム最上流の単一関数は `FUN_00465130`（TESSaveLoadGame::Save 相当）**。quicksave(FUN_00466AD0/0x466B00)/autosave(FUN_00466B70)/名前付き(FUN_00466BE0)/コンソール(FUN_005D3230) の全経路がここに収束。ただし本体が per-form ディスパッチするのは **slot19(+0x4C)/slot20(+0x50)** であって slot8/11 ではない（slot8/11＝TESFile プラグイン書き出し系で retail 到達不能）**／項2 **【解明】トークン 0xB33A35 は NiRTTI クラス記述子識別子ではなく「Collision 表示トグル byte 0xB33A34」（FUN_004447F0 が sete トグル・"Collision -> On/Off" 出力）の隣接領域 bookkeeping トークン。クラス名文字列は存在しない（call 0x70E220 記述子構築なし）＝63本中で唯一の NiRTTI クラス外トークン**／項3 **【解明・前提訂正】slot8=SaveEditedForm_a（bit5＋this+0x48 所属検証つき）、slot11=SaveEditedForm_c（無条件）の機能差は decompiled で確定。使い分け文脈は「TESFile プラグイン書き出し（GECK系）」で、retail の .ess 本体は slot19/20 を使うため slot8/11 は新規/編集済みを問わず retail では呼ばれない（エンジン帯に slot8(+0x20) ディスパッチ0件・slot11 相当0件）＝到達不能 dead code**

（分析結果は項ごとに即時書込み）

---

## §1 項目1（N-21）: セーブゲーム最上流の単一関数（dirty 列挙→slot8/11 ディスパッチ実行者）

**判定: 【解明（ただし前提を訂正）】セーブゲーム最上流の単一関数は `FUN_00465130`（TESSaveLoadGame::Save 相当）。フレームワーク上流（Menu/quicksave/autosave/コンソール）すべてがここに収束する。ただし本体が per-form ディスパッチするのは slot8/11 ではなく slot19(+0x4C)/slot20(+0x50) である。slot8/11（SaveEditedForm）は retail 実行経路で到達不能（エンジン帯に slot8(+0x20) ディスパッチ0件・slot11(+0x2C) も0件）。**

### §1-1 セーブゲーム最上流: FUN_00465130（フォームループ本体）

- `FUN_00465130(0x465130)` は .ess 書き出し本体。フォーム ID リスト（`*param_1+8`）を走査し、各フォームを `FUN_0046B250`/`FUN_009832E6` で解決後、**`(**(code **)(*piVar7 + 0x4c))`（slot19＝GetSaveSize 相当）と `(**(code **)(*piVar7 + 0x50))`（slot20＝SaveGame 相当）** を per-form ディスパッチしてバッファへ直列化（decompiled.c:79660/79679）。"SaveGame() call did not properly fill buffer." エラー（79681）はこのループ内。
- ヘッダ書込・ZLib 圧縮（`FUN_0045FB50`/`FUN_0045E220`/`FUN_00747D30`）・TESFile flush（`FUN_0045A190`）まで含む単一関数。

### §1-2 フレームワーク上流（すべて FUN_00465130 に収束）

| 経路 | 呼出し元 | 実測 |
|---|---|---|
| Quicksave | `FUN_00466AD0` | `FUN_0057acc0(効果音); FUN_00465130(0,"quicksave",0)`（:80568）。別系統 `FUN_00466B00` も `FUN_00465860(0,"quicksave",0)`（:80583） |
| Autosave | `FUN_00466B70` | ロード中ガード（+0x18 bit0）＋`FUN_0065D140` ガードの後 `FUN_00465130(0,"autosave",0)`（:80618） |
| 名前付き/テストセーブ | `FUN_00466BE0` | param_1==4/5 ディスパッチ後 `FUN_00465130(0,local_108,1)`（:80683・"Test All Cells %i.ess" 帯） |
| コンソール/メニュー | `FUN_005D3230` | `FUN_0057de50(0xb); FUN_00465130(0,0,0); FUN_00589b70(0x40f)`（:337922-337924）＝コンソール "save" 系メッセージ 0x40f 連動 |

- ⇒ Menu 決定・コンソール・オートセーブの**全経路が単一関数 FUN_00465130 に収束**（N-21 の問い「どの関数が入口か」に回答）。

### §1-3 前提訂正: セーブ本体は slot8/11 を使わない（slot19/20 を使用）

- v239 §2-1 は「slot8(+0x20)/slot11(+0x2C)＝セーブ実行枠」としたが、**retail の .ess セーブ本体 FUN_00465130 が呼ぶのは slot19(+0x4C)/slot20(+0x50)**（decompiled.c:79660/79679）。slot8/11 ではない。
- slot19/20 の実装（vtable 実測 scanB B1/B2）:
  - TESObjectCELL: slot19(+0x4C)=`0x4D1EB0`(FUN_004D1E40)、slot20(+0x50)=`0x4D2020`(FUN_004D2020)
  - TESForm 基底: slot19(+0x4C)=`0x46AC50`、slot20(+0x50)=`0x46B5E0`
- ⇒ slot8/11（SaveEditedForm）は **TESFile（プラグイン＝.esm/.esp）書き出し系**であり、`.ess` セーブゲームとは別系統。

### §1-4 slot8/11 到達経路の全数走査（retail 到達不能の確定）

| パターン | 実測（scanJ/scanK） |
|---|---|
| `call [reg+0x20]`（slot8 相当）.text 全域 63 件 | engine(TES/RT) band(0x400000-0x5FFFFF) **0 件**。gamebryo/Ni 帯 61＋非Ni帯2（0x6BE668/0x6BE73D＝メモリコピー系・slot8 無関係） |
| `call [reg+0x2C]`（slot11 相当）.text 全域 28 件 | engine band 1 件（0x4CBCED＝FUN_004CBC20・decompiled では **+0x174** ディスパッチ＝NiTList 系で slot11 ではない＝capstone skipdata 誤読） |
| `jmp [reg+0x20/0x2C]` | 9 件すべて gamebryo/Ni 帯（0x8A9905/0x8D958D/0x901B17/…＝NiTList 等の他クラス） |
| `add reg,0x20/0x2C; call [reg]` パターン | **0 件** |
| SaveEditedForm 群(0x4D3460/0x4D3570)への direct E8/E9 | **0 件**（v239 §2-2 と一致） |
| TESForm slot8(0x46B340) direct 呼出し | `FUN_004CD3B0`(0x4CD5CC)＋尾分岐 E9 `FUN_00518000`(0x519320) の2件のみ（＝いずれも TESFile 書き出し系） |
| TESForm slot11 本体(0x46B370) direct 呼出し | `FUN_004C8680`(0x4C8CE1)＋`FUN_004E61C0`(0x4E659B) の2件（＝TESFile 圧縮 flush 系） |

- ⇒ **slot8(+0x20)/slot11(+0x2C) へのディスパッチは retail の TES エンジン帯に一件も存在しない**。SaveEditedForm 群は TESFile プラグイン書き出し（GECK/Construction Set 系＝v239 §2-3 で「retail に文字列0件」と裁定済み）用であり、retail 実行経路では到達不能 dead code（EffectSetting::Save と同類）。

---

## §2 項目2（N-22）: トークン 0xB33A35（0x9DAE30 帯）の個別ラベル

**判定: 【解明】0xB33A35 は NiRTTI クラス記述子識別子ではなく、「Collision 表示トグル byte `0xB33A34`」の隣接領域に対する bookkeeping トークン。クラス名文字列は存在しない（NiRTTI 記述子構築なし）。**

### §2-1 スチューブと CRT ブロック（scanE/scanF）

- スチューブ: `0x9DAE90: mov ecx,0xB33A35; jmp 0x749280`（FUN_009DAE30 の内部）。
- CRT 初期化テーブル参照: .rdata `0xA28760` エントリの隣接列 `{0x9DACB0,0x9DAD10,0x9DAD70,0x9DADD0,0x9DAE30,0x9DAE90,0x9DAEA0,0x9DAF00,0x9DAF60}`。
- 0x9DACB0..0x9DAF60 ブロックは各々 `push -1; …; push 0xB051xx; mov ecx,0xB07BF0; call 0x4040E0; push 0xA17Cxx; call 0x981FB4` 形式（atexit/CRT ラッパ）で、トークンスチューブ 0x9DAE90 が**唯一** `mov ecx,imm; jmp 0x749280` 形式。

### §2-2 0x749280 の機構（scanG G1）

- `0x749280`: カウンタ `0xB40818` を increment。初回（ecx==0→1 遷移）のみ `{0x748D20(init),0x749060(fini)}` を `FUN_00747C40` へ登録。トークン(ecx)は eax に返るだけ＝**ただの識別子／遅延初期化ガード**（v238 §2-1 の裁定を機構レベルで裏付け）。

### §2-3 隣接記述子なし＝NiRTTI クラスではない（scanF F2）

- `call 0x70E220`（{name,parent} 記述子構築）のうち ecx が 0xB33A00..0xB33A80 のもの: **0 件**（scanF F2）。0xB33A35 に対応する {name,parent} スチューブは存在しない。
- ⇒ 63 本中、NiPSys 系61＋MagicHitEffect のように「クラス名文字列を持つ記述子」と対を成すのは 62 本。**0xB33A35 だけは対応クラス名文字列を持たない**。

### §2-4 0xB33A35 の正体: Collision トグル byte の隣接領域（scanF F1 / scanH / scanI）

- `0xB33A35 = 0xB33A34 + 1`。参照されるのは **0xB33A34**（byte）のみ:
  - `FUN_004447F0`(0x4447F0): `cmp byte[0xB33A34],0; sete bl; mov byte[0xB33A34],bl`（**トグル**）。`FUN_00444340` の中間ラベル。
  - `FUN_00507D30`(0x507D8D) から呼出し。`cmp byte[0xB33A34],0` → 0 なら `0xA4B910="On"`、非0 なら `0xA4B90C="Off"` を選んで `"Collision -> %s"`(0xA4C148) を出力（scanI I1）。
  - 読手: CELL 系 `FUN_004D4A20`/`FUN_004D5000`/`FUN_004D63A0`（`sete` で obj+0x19 へ反映）、`FUN_00671620`。
- ⇒ 0xB33A34 は **Collision 表示 On/Off トグル**。0xB33A35 はその直後 byte で、NiRTTI 対応クラスではなく「Collision トグル領域」に対する 0x749280 bookkeeping トークン。**個別クラスラベルは「該当なし（NiRTTI クラス外）」**。

---

## §3 項目3（N-23）: slot8（ガード付き）/slot11（無条件）の上流使い分け文脈

**判定: 【解明（ただし前提を訂正）】slot8=SaveEditedForm_a（bit5＋this+0x48 依存フォーム所属検証つき）、slot11=SaveEditedForm_c（無条件）という機能差は decompiled で確定。しかし上流使い分け文脈は「TESFile プラグイン書き出し（GECK系）」であり、retail の .ess セーブは slot19/20 を使うため、slot8/11 は新規/編集済みを問わず retail ではどの文脈でも呼ばれない（§1-4 の通りエンジン帯ディスパッチ0件）。**

### §3-1 機能差（decompiled.c:165476-165605・v239 と一致）

| | slot8 = FUN_004D3460 (SaveEditedForm_a) | slot11 = FUN_004D3570 (SaveEditedForm_c) |
|---|---|---|
| ガード | `bit5(param_1+8>>5)` で既存なら return 0（165493） | **なし** |
| 依存フォーム検証 | this+0x48 リストを走査し `FUN_0046B680` で所属ファイル照合。不一致なら bVar2=false で skip（165498-165509） | **なし** |
| 本体 | slot9(+0x24) ディスパッチ→`FUN_00450A10` バッファ→20B ヘッダ→`FUN_004CD3B0` flush | 同シーケンスをガードなしで無条件実行 |

### §3-2 使い分けの上流文脈: retail では不活性

- slot8/11 の直接呼出し（direct E8/E9）: **0 件**（SaveEditedForm 群への .text 全域 direct 呼出しゼロ）。
- エンジン帯（TES/RT band 0x400000-0x5FFFFF）の `call [reg+0x20]`: **0 件**、`call [reg+0x2C]`: slot11 相当 **0 件**（0x4CBCED は +0x174 の NiTList 系で slot11 ではない）。
- `add reg,0x20/0x2C; call [reg]` パターン: **0 件**。
- TESForm slot8(0x46B340) の direct 呼出しは `FUN_004CD3B0`(0x4CD5CC) と尾分岐 E9 `FUN_00518000`(0x519320) の2件、slot11 本体(0x46B370) は `FUN_004C8680`/`FUN_004E61C0` の2件で、いずれも TESFile 書き出しチェーン。
- ⇒ **slot8/11 は「TESFile への編集済みフォーム書き出し（プラグイン＝GECK系）」用**。retail は .ess のみを書き出し（v239 §2-3）、その本体は slot19/20 を使う。したがって retail では slot8/11 が「新規 vs 編集済み」「Form vs 個別データ」で使い分けられる文脈は存在せず、**両者とも到達不能 dead code**（GECK コード不在＝v239 §2-3-1 と整合）。

### §3-3 [PORT] 含意

- port がセーブゲーム（.ess）を再現する場合、再現すべきは **slot19(GetSaveSize)/slot20(SaveGame)** の2段（＋ヘッダ・ZLib 圧縮・TESFile flush）であり、slot8/11（SaveEditedForm）は不要（retail でも発火しない）。
- v239 §2-6 の「slot8 ガード（bit14）→slot9 payload→バッファ flush の3段構造」は **TESFile プラグイン書き出し系**の記述であり、.ess セーブゲーム本体（slot19/20 系）とは別系統。両者を取り違えないこと。

---

## §4 新規残点（本タスクで新たに発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-24 | slot19(+0x4C)=GetSize/slot20(+0x50)=SaveGame という .ess セーブ本体（FUN_00465130 の per-form ディスパッチ）の各クラス実装群（TESObjectCELL 0x4D1EB0/0x4D2020、TESForm 基底 0x46AC50/0x46B5E0、その他全フォーム型の override）の列挙・分類が未走査（本タスクは上流特定に注力、v239 §2-6 の [PORT] 材料＝slot8/11 中心の記述を slot19/20 中心へ更新する際の前提） | 中 |
| N-25 | FUN_00518000（0x519320 の尾分岐 E9 が FUN_0046B340/TESForm slot8 へジャンプ）の呼び出し文脈（slot8 への唯一の非 CELL 系到達）が未特定（v239 §2-2 の「尾分岐スラック」実体） | 低 |
| N-26 | FUN_004CBC20 の 0x4CBCED（capstone は call [reg+0x2C] と判定）と decompiled の +0x174 ディスパッチの不一致（capstone skipdata 誤読 or decompiled 再構成差）＝slot11 相当件数の確定に影響する範囲の照合 | 低 |
| N-27 | FUN_005D3230（コンソール save 経路）の param_1=0,0,0 引数とメッセージ 0x40f の対応、および Menu 決定系（セーブメニュー UI→FUN_00465130）の具体的な menu-ID 到達が未走査（経路の性質は裁定済み＝.ess、特定のみ残る粒度） | 低 |

（N-21: **§1 で解明＋前提訂正**（slot19/20 へ）。N-22: **§2 で解明**。N-23: **§3 で解明＋前提訂正**（retail 不活性）。本タスクの残点は上記 N-24..N-27。）

---

## §5 検証情報

- **selftest**: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → ALL PASS (4/4)。
- **EXE**: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（PE 手動解析: ImageBase 0x400000。.text VA 0x401000 raw 0x400 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00 rawsize 0x30C00）。capstone 5.0.7（skipdata＋detail）。
- **本タスクの走査規模**: decompiled.c 全関数マーカー索引化＋.text 総当り（indirect call/jmp [reg+disp] 全数、add→call パターン、direct E8/E9 呼出し元、imm32 参照、CRT 初期化テーブル、記述子構築 call 0x70E220）。scanA(ディスパッチ/セーブチェーン呼出し)・scanB(vtable スロット実測)・scanC(disp 全数)・scanD(エンジン帯フィルタ)・scanE(トークン/CRT)・scanF(0xB33A34 参照/記述子/文字列)・scanG(0x749280機構)・scanH(フラグ書手)・scanI(文字列/呼出し)・scanJ(band 分類/vtable 参照)・scanK(add→call/jmp/direct callers)。
- **スクリプト（新規のみ・`%TEMP%\opencode\`）**: `v241_scanA.py`..`v241_scanK.py`（各 `_out.txt` 付き）。v239 の `v239_*.py` は参照のみ（非改変）。
- **入力（読取のみ）**: `tmp/re/v239_port_mgef_residuals5.md`（本体・§2 必読）/ `cache/named_funcs.txt` / `cache/vtable_index.tsv` / `cache/call_targets.json` / `docs/Oblivion_decompiled.c`（FUN_00465130 :79501・FUN_00466AD0/B00/B70/BE0 :80562-80683・FUN_005D3230 :337915・FUN_004D3460/479/570 :165476-165605・FUN_004D1E40 :164556・FUN_004D2020 :164592・FUN_004CBC20 :159854・FUN_00444340/4447F0 :52789・FUN_004D4A20/5000/63A0 等を部分読取）。
- **移植ソース**: 本タスクでは編集・参照ともなし。
- **git / ESM**: git 不使用。**ESM 本体は未アクセス**（再スキャン禁止を遵守）。web 資料は未取得。
