# v239_port_mgef_residuals5.md — 移植 MGEF 残点第5束（N-18 対象外記録・N-19/N-20 裁定）

- 作成: 2026-11-02 / 担当: TODO serial 232（V232・低）/ **分析専任（docs/GT/TODO/ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v238_port_mgef_residuals4.md` §4 新規残点 N-18..N-20（本体は読取のみ・無編集）／参照: `v237_port_mgef_settings_residuals3.md` §3-2（読取のみ・無編集）
- 手法: EXE 再逆アセンブル（capstone 5.0.7・PE 手動解析・VA=0x400C00+file_off）＋ `docs/Oblivion_decompiled.c` 部分読取（読取のみ）＋ cache（named_funcs）突合。ESM 本体は未アクセス（再スキャン禁止遵守）。git 不使用。
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）
- 結論早見: 項1 **【対象外・未実施】**（ESM 本体再スキャン禁止により許可待ち。許可時に MGEF DATA+0x18..+0x34 を列挙）／項2 **【大部分解明】slot9(+0x24)=クラス別 payload writer で、汎用セーブ枠は slot8(+0x20)。SaveEditedForm 群=TESObjectCELL の slot8/slot11 実装（vtable 0xA469D4）で、retail での到達先はセーブゲーム（.ess）書き出し系（GECK/エディタ系コードは実行バイナリに不在＝文字列0件）。dirty MGEF は retail に生じ得ない（form+8|=0x4000 書手は FUN_0046B590 系のみ、その8呼出し元の対象は REFR/Script/Actor で EffectSetting はゼロ。runtime 登録 FUN_00417220 系は dirty 設定なし）＝EffectSetting::Save は到達不能 dead code。約67個トークン=実数63本すべて「読手なしの識別子」で全数同定成立（61本=NiPSys 系クラス、0xB3C0BB=MagicHitEffect、0xB33A35=未特定）**／項3 **【解明】v237 の「NiTMap 構構築」観測は命令列としては正しいが意味論ラベル誤り。実体は NiRTTI 的クラス記述子 {クラス名文字列, 親記述子} の2-dword 構築**（注記ドラフト §3-3・両本体無編集・適用は要承認）

---

## §1 項目1（N-18）: vanilla ESM の MGEF DATA+0x18..+0x34 全ゼロ検証

**【対象外・未実施】** 項1 は ESM 禁止のため未実施（許可時に列挙）。許可時は MGEF レコードの DATA サブレコード +0x18..+0x34（+0x1C は float 1.0f 期待・他7 dword は全0期待）を全数列挙すれば N-18 の裏付けとなる（v238 §1-3 の推論条件）。

---

## §2 項目2（N-19）: slot9(Save) 上流経路・dirty MGEF 経路・0x749280 トークン全数同定

**判定: 【大部分解明】①slot9(+0x24) は「クラス別 payload writer」で、セーブ実行の枠は slot8(+0x20)/slot11(+0x2C)。②SaveEditedForm 群（0x4D3460/479/3570）は TESObjectCELL の slot8/slot11 実装（vtable 0xA469D4）で、direct 呼出しはゼロ＝仮想ディスパッチのみ。③retail に GECK/エディタ系コードは不在（文字列0件）＝このチェーンの到達先はセーブゲーム（.ess）書き出し系。④dirty MGEF は retail に生じ得ない（全 dirty 呼出し元を実型付きで列挙・EffectSetting 対象ゼロ、runtime 登録系は dirty 設定なし）＝EffectSetting::Save(FUN_00415800) は到達不能 dead code。⑤0x749280 トークンは実数63本を全数走査し「自身スチューブのみ参照・他読手なし」を全数確認、62本を個別ラベル同定（NiPSys 系61＋MagicHitEffect）。**

### §2-1 vtable スロット構造の確定（slot8枠/slot9 payload モデル）

| クラス | vtable start | slot7 (+0x1C) | slot8 (+0x20) | slot9 (+0x24) | slot10 (+0x28) | slot11 (+0x2C) |
|---|---|---|---|---|---|---|
| TESForm（基底） | **0xA3217C**（ctor `FUN_00412880` が `*param_1=&PTR_FUN_00a3217c`・decompiled.c 同） | 0x41B5B0 = 汎用 Load（EDID/DFID サブレコード読取） | **0x46B340 = 汎用セーブ枠**（bit14 ガード→slot9 ディスパッチ→FUN_00450A10 バッファ→ファイル） | 0x41B490 = 基底 payload writer（ヘッダ+文字列サブレコード） | 0x412960 | 0x46B4C0（FUN_0046B370 = 無条件セーブ枠） |
| TESObjectCELL | **0xA469D4**（COL 0xABCCBC→TD 0xB03038=`.?AVTESObjectCELL@@`・ctor FUN_004D3210/004D32C0 が vptr 設定） | 0x4CD600 = **CELL ローダ**（fourcc 分岐 FULL/XEDID/XCLC/XCLL/XWTH/XOWN/XESP/XTRG/EDID…＝decompiled.c:161281 実測） | **0x4D3460 = SaveEditedForm_a**（ガード付きセーブ枠: bit5 ガード→this+0x48 依存リストの所属ファイル検証→slot9 ディスパッチ→0x450A10→20B ヘッダ→FUN_004CD3B0 flush） | 0x4CA3D0 = **CELL payload writer**（'DATA'(1B)+('XCLC' 0x28B/'XCLL' 8B)+サイズ修正） | 0x412960（=基底 slot10・非オーバーライド） | **0x4D3570 = SaveEditedForm_c**（無条件セーブ枠: 同シーケンスをガードなしで実行） |
| EffectSetting | （v238 §1・vptr 0xA320C0 帯） | — | （汎用 0x46B340 と推定・v238 の90 vtable 帯） | **0x415800**（v238 の「slot9=Save」＝payload writer。DESC/ICON/MODL/MODB/MODT/DATA64B/ESCE 書き出し） | — | — |

- v238 の「slot9 = EffectSetting のレコード Save」表記は本モデルと整合（slot9 は payload writer、その実行枠は slot8/slot11）。
- `FUN_004D3479` は独立関数ではなく **FUN_004D3460 の内部継続ラベル**（head の `je 0x4d3479` で確認・v238 の「SaveEditedForm_b」呼称はこれに対応）。

### §2-2 上流経路の全数（direct 呼出し）

| 系統 | 実測 |
|---|---|
| SaveEditedForm 群への direct E8/E9 | **ゼロ**（.text 全域）。0x4D3460/0x4D3570 の非 .text dword 参照は vtable 0xA469F4/0xA46A00 の2件のみ＝**slot8/slot11 仮想ディスパッチのみ** |
| FUN_004CD3B0（依存フォーム flush: this+0x48 リスト走査→slot+0x34 重複排除→各 form の汎用 Save）の呼出し | SaveEditedForm 群3所のみ（0x4D355D/0x4D35BB＋_a 内）＝**CELL のセーブ時にのみ走る** |
| FUN_0046B340 の呼出し | FUN_004CD3B0(0x4CD5CC)＋尾分岐スラック 0x519320＋**slot8 としての vtable 格納約90件**（.rdata 0xA3219C 起点の各クラス vtable） |
| CELL::Save(0x4CA3D0)・CELL::Load(0x4CD600) の direct 呼出し | ゼロ（vtable 格納のみ） |

### §2-3 経路裁定: セーブゲーム（.ess）書き出し系

1. **エディタ/GECK コード不在**: バイナリ内に `GECK`/`Geck`/`geck`/`Construction Set`/`TESCS`/`Edit Dialog`/`TES Construction`/`GECK.exe` 文字列 **0件**（scanA P6）。retail exe にプラグインオーサリング UI/経路は存在しない。
2. **dirty 対象の型がセーブゲーム永続化型に一致**: dirty されるフォームは runtime 生成 REFR（formType 0x31）・Script（0x0D）・配置 Actor/REFR（§2-4）＝セーブゲームに保存される変更類型のみ。
3. **ファイル書き出しは TESFile クラス経由**: SaveEditedForm の file 引数オブジェクトは +0x10 ストリーム/+0x3d4 カウンタ/+0x3DC フラグを持つ TESFile 系ライターで、FUN_00450A10 がレコードバッファをストリームへ flush。retail がこの系で開く「書き込み用ファイル」は .ess のみ（ESM/ESP は読み取り専用オープン）。
4. **残余（新規残点 N-21）**: dirty フォーム列挙→slot8(s)/slot11 ディスパッチを行うセーブゲーム最上流の単一関数（TESSaveGame::Save 相当）は、レジスタ間接ディスパッチ（`call [reg+0x20]` 系は game band で多数・多数が他クラスとのオフセット偶発）のため single-function 特定は未了。経路の性質（セーブゲーム側）自体は 1.–3. で裁定済み。

### §2-4 dirty MGEF 経路の裁定: retail に存在しない（到達不能 dead code）

1. **form+8 |= 0x4000（bit14）の書手は2系統のみ**（decompiled.c 全文 `| 0x4000` 突合＋binary 確認）:
   - `FUN_0046B590`（dirty 設定子本体: FUN_0046B2C0 連動→FUN_0045B780→bit14 セット）
   - `FUN_00451210`（フォームヘッダ適用ローダ: **既存 dirty の bit14 を保持**するのみで新規 dirty 化しない）
2. **FUN_0046B590 の全8呼出し元と実際の dirty 対象**（scanA P1/P2＋scanD D4 の ECX 実測）:

| 呼出し元 | 対象 | 型（実証） |
|---|---|---|
| FUN_0046A020 / 0x485D50 / 0x48E060 / 0x491CE0（4件） | `new FUN_004D9A70`（0x58B）→ dirty→挿入 | **REFR**: ctor が vptr 0xA46C44＋formType byte **0x31**、sig 表 dword[0xB06050]="REFR"、COL pTD 0xB03054=`.?AVTESObjectREFR@@`（三重一致）＝**runtime 生成参照**（PlaceAtMe/スポーン系） |
| FUN_0052ACC0 / 0x531290（global 0xB36530）/ 0x586000 コンソール（スタックオブジェクト×2）（3件） | `FUN_004FBAA0` ctor（vptr 0xA49DA4）→ dirty | **Script**: ctor が formType byte **0x0D**、sig 表 dword[0xB05EA0]="SCPT"、COL pTD 0xB03358=`.?AVScript@@`（三重一致）＝コンソール/グローバルのスクリプト実行系レコード |
| FUN_006830B0（1件） | QI(TESObjectREFR/Actor・PTR_PTR 0xB03054/0xB03070) → ebx → dirty | **Actor/REFR**（配置/転送アクター） |

   → **EffectSetting を引数にする dirty 呼出しはゼロ**（8件すべて実型同定済み）。
3. **runtime 登録 FUN_00417220 系は dirty を生まない**: `new 0xA8→ctor FUN_00416430→field 設定→FUN_00452570(map 0xB15A0C へ fourcc→obj 挿入)` のみ。bit14 設定なし・formID(+0xC) 設定なし・グローバルフォームリスト（0xB06150 系）登録なし。起動時バッチ FUN_00417420（"Water Breathing" 等数十件）も同様＝**セーブ対象に載らない**（毎起動再構築される）。
4. ⇒ **vanilla/retail で dirty MGEF は生じ得ず、EffectSetting::Save(FUN_00415800) は実行されない**。v238 §1-5 の「port が MGEF を再エンコードしない限り slot9 相当は不要」を裏付ける最終裁定。（前提: 静的解析到達経路の網羅。動的計算地址・バイナリ外モジュールは対象外＝v237/v238 と同一）

### §2-5 0x749280 個別トークンの全数同定（63本）

1. **全数走査**: direct jmp 0x749280 = **63 箇所・全63本が `mov ecx,<token>; jmp 0x749280` 形式**（無 token 版0件）。各トークンの .text 参照は**自身スチューブの mov 命令1件のみ**・非 .text dword 参照0件。63スチューブすべて CRT 初期化テーブル（.rdata 0xA283D0..0xA2F77C）内エントリの参照先（fn-start ref 0xA28760/0xA2AFE8/0xA2D74C 帯）。
2. **個別ラベル（隣接する {name,parent} 記述子スチューブとの 1:1 対応）**: トークンスチューブの直後（+0x20 帯）に必ず `push parent; push name; mov ecx,<desc>; call 0x70E220` スチューブが来る対列構造。対応表:
   - `0xB3C0BB` ↔ **MagicHitEffect**（記述子 0xB3C0BC の直前・v238 の考察が継続）
   - `0xB40848..0xB42004` の61本 ↔ **NiPSys/Ni 系61クラス**（順に NiParticleSystem, NiPSysMeshEmitter, NiPSysCylinderEmitter, NiPSysSphereEmitter, NiPSysBoxEmitter, NiPSysMeshUpdateModifier, NiPSysRotationModifier, NiMeshParticleSystem, NiPSysEmitter, NiPSysEmitterCtlr, …NiPSysModifierFloatCtlr, 最終 0xB42004↔NiDX9Renderer。全表は scanF F1 出力 `%TEMP%\opencode\v239_scanF_out.txt`）
   - `0xB33A35`（0x9DAE90 帯）↔ **ラベル未特定**（隣接記述子なし・新規残点 N-22）
3. **用途**: v238 §2-1 の 0xB3C0BB 裁定（this は識別トークン、初回のみ {0x748D20,0x749060} をグローバル init/fini へ登録＋カウンタ 0xB40818 加算、ストレージ自体は不変）が**全63本に成立**。トークン＝各クラスの隣接 BSS アドレス（NiPSys シングルトン帯 0xB408xx..0xB42xxx に61・MGEF 帯に1・0xB33 帯に1）で、読手・書手なしの bookkeeping 識別子。

### §2-6 [PORT] 材料まとめ

- slot9(EffectSetting payload writer)・slot8/11（セーブ枠）構造が確定。port が Save 系を再現する場合、再現すべきは「slot8 ガード（bit14）→slot9 payload→バッファ flush」の3段構造で足り、retail ではそもそも発火しない。
- dirty 化 API は `FUN_0046B590` 単一＋型限定（REFR/Script/Actor）。MGEF に dirty を与える API は engine に存在しない。

---

## §3 項目3（N-20）: v237 §3-2「NiTMap メンバ3個」と v238 実測（2-dword ペア書換）の差異

**判定: 【解明】v237 の観測（命令列）は正しいが、意味論ラベルが誤り。実体は「NiRTTI 的クラス記述子 {クラス名文字列, 親記述子} の2-dword 起動時構築」。v238 §2-2（2-dword ペア書換）が指示レベルで正、意味論は本 §3-2 で補完。**

### §3-1 v237 が何を観測していたか（差異の説明）

- v237 §3-2 の観測「`push 引数; push 定数(0xA761AC/0xA7624C/0xA76570); mov ecx,<addr>; call 0x70E220`」は**命令列として実測と一致**（scanC で再確認: 0x9FEC80/CA0/CC0 のバイト列同一）。
- 差異は解釈のみ: v237 は (a) 0x70E220 を「NiTMap 構築」、(b) push 定数を「vtable」、(c) 第2引数 0xB3A680/0xB3C0BC を「アロケータ/親指定」とした。
- 実態: (a) **FUN_0070E220 = `*this = arg1; this[1] = arg2;` の2-dword ストアのみ**（ret 8・scanC window 実測、v238 正）。(b) push 定数は vtable ではなく **ASCII クラス名文字列**（0xA761AC=`"MagicHitEffect"`、0xA7624C=`"MagicModelHitEffect"`、0xA76570=`"MagicShaderHitEffect"`。vtable-4 に COL なし・本体が ASCII で即座に否定）。(c) 第2引数は親**記述子**（下表）。

### §3-2 実体: NiRTTI 的クラス記述子の起動時構築（本タスクの新事実）

| シングルトン | = {name, parent} | 親の正体 |
|---|---|---|
| `0xB3C0BC` | {"MagicHitEffect", 0xB3A680} | **0xB3A680 = "BSTempEffect" 記述子**（name 0xA68214・parent=0＝ルート。scanF F2 実測） |
| `0xB3C0C4` | {"MagicModelHitEffect", 0xB3C0BC} | MagicHitEffect（＝継承階層 BSTempEffect→MagicHitEffect→MagicModelHitEffect） |
| `0xB3C0D4` | {"MagicShaderHitEffect", 0xB3C0BC} | MagicHitEffect |

- 消費者（scanC/scanD 実測）: `FUN_0069D960` = `mov eax,0xB3C0BC; ret`（getter）、`FUN_00678D90/678E70/678F50/6789120/679240` = `cmp eax,0xB3C0BC/C4/D4`（**型同一性比較**＝NiRTTI 的 IsKindOf 系）、`FUN_0069E350/6A0700` 等の ctor 群が vptr 0xA761C4/0xA764B4 を設定（= MagicModelHitEffect 等のクラス vtable・COL pTD 0xB15D90=`.?AVMagicModelHitEffect@@`）。
- **`call 0x70E220` は .text 全体で数百件**（FUN_009F9530/00A00190/00A0A290/00A10E50/00A12380/00A12540/00A12A90/00A13810/00A139F0/00A13DC0/00A15870 の CRT スチューブ帯）＝**NiRTTI クラス記述子階層全体（NiPSys 系61クラス＋magic VFX 系＋BSTempEffect 系＋NiDX9Renderer 他）の起動時構築**。0x9FEC80/CA0/CC0 はその中間3件であり、v238 §2-1 が同定した「NiPSys（Gamebryo パーティクル）クラスファクトリ登録」と同一初期化ブロックの別システム（名前→関数マップ vs 名前→親記述子）。
- 境界非重複（v238 の旗23本 bookkeeping 裁定）は維持: ペア3個は BC..C3/C4..CB/D4..DB で終端し旗バイトと重複しない（v238 §2-2 どおり）。

### §3-3 注記ドラフト（反映は要承認・v237/v238 両本体は無編集）

> **[注記ドラフト — v237 §3-2 の是正]** 「0x9FEC80/0x9FECA0/0x9FECC0 = 0xB3C0BC/C4/D4 の NiTMap 系メンバ3個を起動時構築」は**誤り**。実体は `FUN_0070E220`（2-dword ストア）による**クラス記述子 {name*, parent*} の構築**: 0xB3C0BC={"MagicHitEffect", BSTempEffect(0xB3A680)} / 0xB3C0C4={"MagicModelHitEffect", 0xB3C0BC} / 0xB3C0D4={"MagicShaderHitEffect", 0xB3C0BC}。push 定数は vtable ではなくクラス名文字列（NiRTTI 的 IsKindOf 用）。call 0x70E220 群は NiRTTI 記述子階層の起動時構築（数百件）。以後 v238 §2-2（指示レベル）＋v239 §3-2（意味論）を正とする。

---

## §4 新規残点（本タスクで新たに発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-21 | セーブゲーム書き出しの最上流単一関数（dirty フォーム列挙→TESObjectCELL/汎用 form の slot8(s)/slot11 ディスパッチ実行者）の同定未了。経路の性質（.ess 書き出し系）は裁定済み（§2-3）だが、特定はレジスタ間接ディスパッチの多量偶発のため要追加走査（dirty 配列 0xB06150 系の追加元探索が近道候補） | 低 |
| N-22 | トークン `0xB33A35`（スチューブ 0x9DAE90 帯）の個別ラベル未特定（隣接 {name,parent} 記述子スチューブなし・0x9DAE30 帯は別系統） | 低 |
| N-23 | slot8（ガード付きセーブ枠）と slot11（無条件セーブ枠）の各上流ディスパッチ文脈の使い分け（どの保存パスが無条件版を使うか）未特定（N-21 と同系統） | 低 |

（N-18 の扱い: **対象外・未実施**（ESM 禁止）＝§1。N-19: **§2 で大部分解明**（残余は N-21..N-23）。N-20: **§3 で解明＋注記ドラフト提示**。）

---

## §5 検証情報

- **selftest**: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → ALL PASS (4/4)。
- **EXE**: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（PE 手動解析: ImageBase 0x400000。.text VA 0x401000 raw 0x400 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00 rawsize 0x30C00）。capstone 5.0.7（skipdata＋detail）。
- **本タスクの走査規模**: .text 線形 225.8 万命令を7系統（scanA: 保存系 direct 呼出し＋dirty 書手呼出し元＋エディタ文字列／scanB: 0x749280 全63サイト＋トークン参照全数／scanC: vtable/RTTI 復号＋ペア記述子消費者／scanD: vtable start 特定＋dirty サイト ECX 実測＋formType 書手／scanE: formType sig 表＋レジスタ間接ディスパッチ列挙＋call 0x70E220 全数／scanF: トークン個別ラベル＋親記述子＋基底 vtable／scanG: ディスパッチャ候補文字列）。
- **スクリプト（新規のみ・`%TEMP%\opencode\`）**: `v239_scanA.py`(+_out.txt) / `v239_scanB.py`(+_out.txt) / `v239_scanC.py`(+_out.txt) / `v239_scanD.py`(+_out.txt) / `v239_scanE.py`(+_out.txt) / `v239_scanF.py`(+_out.txt) / `v239_scanG.py`(+_out.txt)。v238 の `v238_*.py` は参照のみ（非改変）。
- **入力（読取のみ）**: `tmp/re/v238_port_mgef_residuals4.md`（本体・全458行相当）/ `v237_port_mgef_settings_residuals3.md`（§3-2・§7）/ `cache/named_funcs.txt` / `docs/Oblivion_decompiled.c`（FUN_0046b340 :84816・FUN_004cd3b0 :161143・FUN_004cd600 :161281・FUN_004d3460/479/570 :165476-165605・FUN_0046b590 :84960・FUN_00451210 :62074・FUN_00417220 :15640・FUN_00417420 :15714・FUN_004d9a70 :165000 帯・FUN_004fbaa0・FUN_00412880・FUN_0041b490/41b5b0・FUN_004ca3d0・FUN_00450a10/4501f0/45b780/46b2c0/46b680/44faa0/46ab80/4baca0 等を部分読取）。
- **移植ソース**: 本タスクでは編集・参照ともなし。
- **git / ESM**: git 不使用。**ESM 本体は未アクセス**（再スキャン禁止を遵守）。web 資料は未取得。
