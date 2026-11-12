# v238_port_mgef_residuals4.md — 移植 MGEF/設定 残点第4束（3件）

- 作成: 2026-11-02 / 担当: TODO serial 231（V231・低）/ **分析専任（docs/GT/TODO/ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v237_port_mgef_settings_residuals3.md` §7 新規残点 N-15..N-17（本体は読取のみ）
- 手法: EXE 再逆アセンブル（capstone 5.0.7・PE 手動解析・ImageBase 補正・セクション個別変換）＋ **`docs/Oblivion_decompiled.c` 部分読取**（Ghidra デコンパイル済み・読取のみ）＋ cache（named_funcs/vtable_index）突合 ＋ 自作データフロー走査（ ADDR ロード→[reg+d] / 引数伝播→被呼出先 / byte-span 読み の3系統）。ESM 本体は未アクセス（再スキャン禁止遵守）。git 不使用。
- 結論早見: 項目1 **【解明】slot9 = EffectSetting のレコード Save（プラグイン直列化）＝レコードヘッダ(0x46B990)＋サブレコード追記(0x46AE40: DESC/ICON/MODL/MODB/MODT/DATA(64B)/ESCE)＋サイズ修正(0x46B9F0) の一連。ポインタ7本（+0x70/78/7C/80/84/88/8C）は「ランタイムポインタ＝指し先オブジェクトの +0xC dword がシリアライズ値」という二重表示で、Save 時に一時的に [p+0xC]?:0 へ書換えて DATA 64B を書き、即復元。ctor=0・ローダ=memset+ESM 生コピー・代入=浅複製のみで、.text 全域に「指し先を確保して非0化する経路」は存在しない＝retail では常に NULL。slot46 の対象型は QI 型ペア実データから **EffectSetting 確定**、v237 の slot46 実行者候補 FUN_00512540 は **Process クラスへのオフセット偶発で棄却** ／ 項目2 **【解明】0x9FEC70 = `mov ecx,0xB3C0BB; jmp 0x749280` のうち 0x749280 は「初回呼び出し時のみグローバル init/fini ペア {0x748D20, 0x749060} を登録＋使用カウンタ 0xB40818 を加算し this を返す」共有スラック。登録ペアの中身は **NiPSys（Gamebryo パーティクル）クラスファクトリ登録**（NiMeshParticleSystem/NiParticleSystem/NiPSysAirField* 等を名前→関数で登録/解除）＝0x9FEC70 は「0xB3C0BB のバイトを初期化する」のではなく NiPSys 登録系への起動時フック。0xB3C0BB のストレージは参照1件のみ（当該エントリ自身）で読手・書手なし ／ 項目3 **【最終裁定: bookkeeping 成立】旗23本は retail .text に読手なし。v237 の3系統に加え（再検証済み）、データフロー検証（ADDR ロード→span 感知 [reg+d]:0 件）、引数伝播→被呼出先シミュレーション（1,713 箇所・758 シード:0 件）、4バイト span 覆蓋読み:0 件、データセクション参照テーブル:実質0件、.text 埋め込みテーブル:書手命令エンコード23件のみを総ざくいし、いずれも検出ゼロ。旗バイトは周辺オブジェクト（ペア3個・CS）と境界非重複** ／ （項目1の補足）ポインタ7本の「vanilla ESM でゼロ」は Save 逆参照整合からの推論（ESM 未検証・再スキャン禁止）＝新規残点 N-18 に記録
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**

---

## §1 項目1: EffectSetting vtable slot9（FUN_00415800）の意味論とポインタ7本の指す先（N-15）

**判定: 【解明】①slot9 = EffectSetting のレコード Save（プラグイン/セーブデータ直列化）＝ヘルパ鎖がすべてサブレコード書き出し系で、単一の解釈しか成立しない。②ポインタ7本は「指し先オブジェクトの +0xC dword を DATA エントリとして書く」予約スロットで、Save 時のみ [p+0xC]?:0 に一時書換（→DATA 64B 追記→即復元）。③.text 全域でポインタ7本に**非0を書き込む経路は存在しない**（ctor=0 ワード、ローダ=memset+ESM 生コピー、代入=浅複製、Save=一時書換のみ）＝retail では常に NULL で、Save の逆参照は決して実行されない。④ポインタ7本の gameplay 読手はゼロ（v236/v237 維持＋本タスクでクラスタ内候補をすべて外部クラスと同定）。⑤slot46 の引数型は **EffectSetting（QI 型ペア実データで確定）**、v237 の slot46 実行者候補 FUN_00512540 は **TESObjectREFR/Actor 型ペア＋エラーメッセージで「移動先アクターのプロセス」へのディスパッチと判明＝EffectSetting では無く偶発**。**

### §1-1 slot9 = レコード Save の決定的証拠（ヘルパ鎖の実体）

`docs/Oblivion_decompiled.c:14204`（slot9 本体）とヘルパ群（同ファイル）の実測:

| ヘルパ | 実体（decompiled.c 行） | 内容 |
|---|---|---|
| `FUN_0046B990`（:85180） | レコード書き出し開始 | グローバルバッファ `DAT_00B33C14`(ptr)/`DAT_00B33C18`(size) に 0x14B レコードヘッダを新設: シグネチャ = `&DAT_00B05E08[formType*3]`（formType = obj+0x04、EffectSetting は 0x0C）、flags = obj+0x08（formType!=1 なら 0x30EE0 マスク）、formID = obj+0x0C。**guard: (obj+0x08)>>0xE & 1 なら何もしない**（＝汎用フォーム Save と同じ bit14 ガード） |
| `FUN_0046AE40`（:84483） | サブレコード追記 | {fourcc(4B) + size(2B) + payload} をバッファ末尾へ追記（>0xFFFF は 'XXXX' 拡張サブレコード経由）。memcpy で payload を**現在のオブジェクトメモリから生書き** |
| `FUN_0046B9F0`（:85207） | レコード書き出し終了 | ヘッダ+4 の dataSize を `バッファサイズ-0x14` でパッチ |
| `FUN_0046A700`（:84070） | `FUN_0046A440(this,'DESC')` | DESC サブレコード書き出し |
| `FUN_0046C730`（:86101） | 文字列サブレコード | 長さ 0xFFFF は strlen に切替（BSString 系メンバー用） |
| `FUN_0046D8B0`（:87163） | モデルパス3点組 | vtable[0x14](確保) → `FUN_0046AE40(code1,…)` ＋ `FUN_0046BA80(code3,…)`＝MODL/MODB/MODT 群 |
| `FUN_00470070` | 'ICON' サブレコード | slot9 は `push 'ICON'` して呼出 |

slot9 本体の実行順（decompiled.c:14206-14292 と v237 §2-2 の一致確認済み）:
`FUN_0046B990`（ヘッダ）→ DESC → MODL/MODB/MODT 文字列群 → **ポインタ7本 save→`field = field ? *(field+0xC) : 0` 一時書換** → `FUN_0046AE40('DATA', this+0x58, 0x40)`（DATA 64B を現在メモリから追記）→ 7本復元 → （count=+0x6C が非0 かつ配列=+0x9C が非0 なら）`FUN_0046AE40('ESCE', array, count*4)` → `FUN_0046B9F0`（サイズ確定）。

→ **slot9 = EffectSetting::Save（フォームをプラグイン直列化バッファへ書き出す仮想関数）**。v237 の「整形/再初期化系（意図未解明）」はこれで解消。

### §1-2 slot9 を呼ぶ全サイトと期待値

| 系統 | 実測 | 期待値 |
|---|---|---|
| 直接 E8/E9 → 0x415800 | **ゼロ**（scanA direct-call 全数） | slot9 は仮想経由のみ |
| **汎用フォーム Save `FUN_0046B340`**（:84816） | `(obj+0x08)>>0xE&1` ガード → **slot9 ディスパッチ** → `FUN_00450A10(obj)`（バッファ→ファイル出力系） | 引数なし thiscall の Save。dirty フォーム（bit14 クリア）のみ実行 |
| `FUN_004CD3B0`（:161143・スロット+0x34 比較で重複排除ループ） | `0x4CD5CC: call 0x46B340` | 変化フォーム一括 flush 経路 |
| 無名スラック `0x519320` | `jmp 0x46B340`（int3 境界で独立関数） | 汎用 Save への尾分岐 |
| **SaveEditedForm 群** `FUN_004D3460`/`FUN_004D3479`/`FUN_004D3570`（:165478-165605） | slot9 直ディスパッチ → `FUN_00450A10` → 6B ヘッダ（`DAT_00B05E20` 系・formID=+0x0C）→ `FUN_004501F0` → `FUN_004CD3B0` flush | 単一フォームを編集中ファイルへ保存 |
| レジスタ間接ディスパッチ（`mov reg,[obj+0x24]→call`・game band 122 箇所＝v237 scanF／`call [reg+0x24]` 直形 123 箇所＝v237 scanC） | スポットチェック: `0x404708`(FUN_004046A0) は **3引数** slot9（メンバクラス・rep movsd パラメータコピー帯）、`0x8B61F7`(FUN_008B5E20) は **1引数** slot9（反復走査）＝いずれも EffectSetting Save ではない別クラス偶発。`FUN_0046B340` 自身が122箇所の1つ | — |

→ **EffectSetting の slot9 が実際に効く経路は「汎用フォーム Save 系（dirty フォームの書き出し）」のみ**。gameplay 判定は一切呼ばない。

### §1-3 ポインタ7本の指す先【解明】

1. **指す先の型**: Save の書換 `field = field ? *(field+0xC) : 0` は「各フィールドは**≥0x10B オブジェクトへのポインタ**であり、DATA ファイル形式に載せる値はその**+0xC の dword**」であることを意味する（NULL のときは 0 が載る）。DATA 64B（this+0x58..+0x97）のうち +0x18/+0x20/+0x24/+0x28/+0x2C/+0x30/+0x34（obj+0x70/78/7C/80/84/88/8C）＝7 スロットがこの二重表示、+0x1C(obj+0x74) は素の float のまま直書き。
2. **誰も非0化しない（本タスクの全数走査）**:
   - ctor `FUN_00416430`: `mov word[esi+0x6c],di` 直後に 7 本へ `edi`(=0) を連続代入（0x4164F1-0x41650C・decompiled.c:14786-14797 と一致）。**0 初始化**。
   - ローダ `FUN_00416100`（:14606 'DATA' 分岐）: `_memset(obj+0x58, 0, 0x40)` → `obj+0x74 = 1.0f` → `TESFile::Read(obj+0x58, 0x40)` の**生コピー**。ポインタ7本は「ファイルの生 dword」をそのまま受け取る（→指し先オブジェクトへの構築処理は**存在しない**）。
   - 代入 slot45 `FUN_00416000`: 浅コピーのみ。比較 slot46 `FUN_00415BC0`: dword 等値比較のみ。
   - Save slot9: 一時書換→即復元のみ。
   - setter `FUN_00415D40`（:14345）: +0x9C/+0x6C のみ（pointer7 に触れない）。runtime 生成 `FUN_00417220`: `push 0xA8; call 0x401F00`（new 0xA8B）→ctor→fourcc 分岐（tail への書込みなし・v237 維持）。
   - **.text 全域の pointer7 書手列挙（非スタックベース・scanD）**: EffectSetting メソッド圏の書手は ctor(0)/Save(一時)/assign(コピー) のみ。他の書手（`FUN_00406360`/`FUN_00412BE0`/`FUN_00419570`/`FUN_004198F0`/`FUN_00446D80`/`FUN_004492E0`/0x80xxxx-0x8Axxxx 帯多数等）はすべて**自クラスの同オフセットMembers**（vtable 定数書き込み `0xA320C0`/`0xA33E9C`・カウンタ・0 初期化等）＝EffectSetting ではない。
   - **クラスタ内 pointer7 読手候補の同定（scanD）**: `FUN_00416A80/0x416BA0/0x416DE0` = グローバル `0xB33B00/0xB33508/0xB3350C/0xB33510` を扱う **'BLOK'（セルブロック）系**（[0xB33B00]+0x80 等が対象）／`FUN_004190C0` = QI 型ペア（TESForm, **TESObject**）汎用フォーム述語（`test byte[eax+0x88],1` の eax は TESObject）／`FUN_00419570/0x419640/0x4198F0` = **ENCH 系クラス**（fourcc 'ENIT'/'SCRI'/'MODL'…、vtable 0xA33C84/0xA33F5C 等）。→ **EffectSetting の pointer7 を読むのは slot46/45/9 と ctor/ローダのみ**。
3. **常に NULL か**: runtime 経路（ctor 0・runtime 生成 0）は確定。ESM 経路は「生コピー＝ファイル値」なので、**vanilla ESM の MGEF DATA+0x18..+0x34 が全ゼロであることが整合条件**（非ゼロが載っていた record が dirty になって Save されると [値+0xC] の逆参照で即クラッシュするため、retail が動作していることから vanilla ではゼロと推論。**ESM 実値は再スキャン禁止のため未検証**＝§4 N-18）。
4. **[PORT] 判定**: port が pointer7 を 0 のまま扱う（読まない・埋めない）ことは engine と整合的に無害。Save/eq/assign 相当を再現する場合も「dword 0 同士の等値・複製」で足りる。

### §1-4 slot46 実行者の型確証（v237「確度中」の裁定）

1. **QI ヘルパ `FUN_009832E6` の型ペア実データを RTTI で復号**（cache 名: `FindSITargetTypeInstance`/`FindMITargetTypeInstance`/`FindVITargetTypeInstance` ＝ MSVC RTTI クロスキャスト内部）。各「PTR_PTR」アドレスは **TypeDescriptor そのもの**（vptr=0xAA3E80・spare=0・name at +8）:

| アドレス | 型名（実データ復号） | 使用者 |
|---|---|---|
| `0xB02F9C` | `.?AVTESForm@@` | eq(slot46)/FUN_0048DA00/FUN_00518000 等の list1 |
| `0xB03534` | `.?AVEffectSetting@@` | **eq(slot46) の list2** |
| `0xB03054` | `.?AVTESObjectREFR@@` | FUN_00512540 list1 |
| `0xB03070` | `.?AVActor@@` | FUN_00512540 list2 |
| `0xB03268` | `.?AVTESObject@@` | FUN_004190C0 list2 |
| `0xB0331C` | `.?AVTESScriptableForm@@` / `0xB05DA4` `.?AVTESLevItem@@` / `0xB036FC` `.?AVTESObjectWEAP@@` | FUN_0048DA00 / FUN_00518000 帯 |

   → **slot46(FUN_00415BC0) の引数は EffectSetting に確実にキャストされる**＝レコード等価述語の型保証は確立（v237 の理解を確定）。
2. **FUN_00512540 の slot+0xB8 ディスパッチは EffectSetting ではない**（v237 棄却）: `0x5127F2` 帯の実測（scanA window）: `push ebp(移動対象アクター) → QI(TESObjectREFR/Actor) → esi` → `mov ecx,[esi+0x58]; test; je "Actor being moved is Disabled he has no process"`（:205429 のエラーメッセージ）→ `[vtable+0x20]` → `[vtable+0xB8](esi)`。**[esi+0x58] はアクターのプロセス**で、+0xB8 はプロセスクラスの仮想関数＝**オフセット偶発**。slot46 の「実行者候補」は不存在（direct E8 もゼロ）。
3. slot45 の実行者（v237 確度高）を補強: `FUN_0046C360`（:85894）= `FUN_00447DF0(formType)` で新規フォーム生成 → `SetEditorID`（slot+0xD8）→ **`new->slot45(old)`**＝フォーム複製。`FUN_0048DA00` の QI ペア（TESScriptableForm/TESLevItem）もエンチャント/錬金複製経路と整合。

### §1-5 項目1の [PORT] 材料まとめ

- slot9/45/46 は「レコード管理 API（Save/複製/等値）」という v237 の全体像は正しい。追加された新事実は「slot9 は Save 本体」「pointer7 は Save 連動の予約スロット」「vanilla では常に NULL」。
- port が MGEF を再エンコード（plugin 生成）しない限り、slot9 相当は不要。等価比較・複製を再現する場合も dword 0 のまま扱えば一致する。

---

## §2 項目2: CRT 初期化エントリ 0x9FEC70（`mov ecx,0xB3C0BB; jmp 0x749280`）の用途（N-16）

**判定: 【解明】0x9FEC70 は「0xB3C0BB のバイトを初期化する」のではなく、**遷移先 0x749280（独立関数・int3 境界で確認）が**NiPSys（Gamebryo パーティクルシステム）クラスファクトリ登録系への『初回起動時フック』**。0x749280 は（a）カウンタ `0xB40818` を加算、（b）初回のみ `{初期化=0x748D20, 終了=0x749060}` のペアを**グローバル init/fini レジストリ**（`FUN_00747C40` → 配列 `0xB40378`/`0xB40338`・件数 `0xB403B8`）へ登録、（c）this（=0xB3C0BB）を返す。this は識別トークンで、**0xB3C0BB のストレージ自体には一切触れられない**（ファイル内で参照は当該エントリ1件のみ）。**

### §2-1 0x749280 の全文意味論（scanA window 実測）

```
0x749280  mov eax,[0xB40818]        ; 登録済みカウンタ
          push esi / mov esi,ecx    ; this 保存
          mov ecx,eax / add eax,1   ; 旧値を ecx へ、カウンタ+1
          test ecx,ecx / mov [0xB40818],eax
          jne 0x7492A8              ; 旧値!=0（初回以外）→ フック済みなのでスキップ
          push 0x749060             ; fini
          push 0x748D20             ; init
          call 0x747C40             ; グローバル init/fini レジストリへ追記
          add esp,8
0x7492A8  mov eax,esi / pop esi / ret   ; this を返す
```

- `FUN_00747C40`（decompiled.c:592837）= `配列0xB40378[件数]=init; 配列0xB40338[件数]=fini; 件数0xB403B8++`（並列 fn-ptr 配列へのペア登録）。
- **実行者（ランナー）**: `0x747BA0` = `call 0x7489F0; if([0xB403BC]) call; for(i<0xB403B8) call [0xB40378+4i]`（**起動時イニシャラ走査**）、`0x747BE0` = `for(i<count) call [0xB40338+4i]; if([0xB403C0]) call`（**終了時ファイナライザ走査**）。`FUN_00747C20` が前後フック `0xB403BC/0xB403C0` を設定。
- **登録ペアの中身**（scanB window＋文字列復号）:
  - `0x748D20`（init・ガード byte `0xB407C0`）: `FUN_00712590(名前, 関数)` 連続呼出 → 実体は `mov ecx,[0xB3FB80]; call 0x412D30`＝**名前→関数のハッシュマップ挿入**（`FUN_00412D30` = :11092 のキー付き挿入ヘルパ）。登録キー（.rdata 文字列）: **'NiMeshParticleSystem'(0xA85A18→0x74ECB0) / 'NiMeshPSysData'(0xA85A08→0x75D7F0) / 'NiParticleSystem'(0xA859F4→0x749FE0) / 'NiPSysAirFieldAirFrictionCtlr'(0xA859D4→0x75D450) / 'NiPSysAirFieldInheritVelocityCtlr'(0xA859B0) / 'NiPSysAirFieldModifier'(0xA85998)** …＝**NiPSys クラスファクトリ登録**。
  - `0x749060`（fini・同じガード）: 同キー群を `FUN_007125B0`（→`FUN_004524C0`＝マップ除去）で**解除**。
- **同型スラックの分布**: `jmp 0x749280`（direct E8/E9）は .text 内 **63 箇所**（scanA direct-call 全数 72 のうち大半・`0x9DAE95`/`0x9FEC75`＋FUN_00A0A290 帯 61）。サンプル6件すべて `mov ecx,<個別グローバル>; jmp 0x749280` 形式（`0x9DAE90`: 0xB33A35 / `0x9FEC70`: **0xB3C0BB** / `0xA0AB90`: 0xB40848 / `0xA0AD20`: 0xB408B8 / `0xA0AEB0`: 0xB40918 / `0xA0B040`: 0xB40980）。カウンタ `0xB40818` の参照は 0x749280 と **0xA0AA00**（this 無し版の同一ロジック: カウンタ加算＋初回ペア登録）のみ。
- **0xB3C0BB の位置づけ**: .text/.rdata/.data の全参照は **0x9FEC70 の1件のみ**（scanA「refs to 0xB3C0BB: total 1」）＝ストレージへの読手・書手なし。隣接ペア（§2-2）の1バイト前という並びだが、初期化対象バイトではない。

### §2-2 付随是正: 0x9FEC80/0x9FECA0/0x9FECC0 は「NiTMap 構築」ではなく 2-dword ペア書換

- v237 §3-2「0xB3C0BC/0xC4/0xD4 の NiTMap 系メンバ3個を起動時構築」は**実態と異なる**。実測（scanA window）: `push 引数; push 定数(0xA761AC/0xA7624C/0xA76570); mov ecx,<addr>; call 0x70E220`、`FUN_0070E220`（decompiled.c:552051）= **`*this = arg1; this[1] = arg2;` の2-dword ストアのみ**。
- したがって実体は 8B ペア3個: `0xB3C0BC={0xA761AC, 0xB3A680}` / `0xB3C0C4={0xA7624C, 0xB3C0BC}` / `0xB3C0D4={0xA76570, 0xB3C0BC}`。サイズは BC..C3 / C4..CB / D4..DB で、**旗バイト（A5-A9,AA,AC-B1,B8,B9,DC-E0,E8-EB）と境界重複しない**（pair3 は DC の直前で終端）。

### §2-3 裁定と残余

- **0x9FEC70 の用途 = NiPSys ファクトリ登録系の起動時フック（初回登録＋利用数カウンタ）**。this=0xB3C0BB はトークンであり、このエントリ単体の副作用は「{0x748D20,0x749060} のグローバル init/fini 登録を保証 ＋ 0xB40818++」のみ。
- 残余（低優先・§4 N-19 に記録）: 約67 個の個別トークン（0xB3C0BB/0xB33A35/0xB408xx…）がそれぞれどの NiPSys シングルトン/インスタンスに対応するかは未個別同定（パターン同一・個別性は低い）。

---

## §3 項目3: 登録マーカー旗23本の「読手なし bookkeeping」最終裁定（N-17）

**判定: 【最終裁定: bookkeeping 成立】retail .text 内に旗23本（0xB3C0A5,A6,A7,A8,A9,AA,AC,AD,AE,AF,B0,B1,B8,B9,DC,DD,DE,DF,E0,E8,E9,EA,EB）の読手は存在しない。v237 の3系統（絶対参照・imm・base+disp 共起）を独立に再検証したうえで、N-17 が示唆した盲経路（構造体経由のバイト読み・引数伝播経由・4バイト span 覆蓋・埋め込みテーブル・データセクション間接ポインタ）をすべて追加走査し、いずれも検出ゼロ。旗バイトは周辺オブジェクトと境界非重複。**

### §3-1 v237 の3系統ゼロの独立再検証

| 系統 | 本タスク実測 |
|---|---|
| 絶対参照＋imm 参照（.text 全域・mem-abs/imm 全オペランド） | **23 ヒット＝すべて FUN_009FEA50 帯（23 スチューブの書手 `mov byte[旗],al`）のみ**・distinct funcs=1（scanA sweep1） |
| base+disp 共起（0xB3C0xx 窗口 ref 関数の小 disp クロスプロダクト） | 278 キャンディデートが**すべて別用途のクロスプロダクト**であることを §3-2 のデータフロー検証で裏付け（scanA sweep2 → scanB/scanC で棄却） |
| 一括/範囲参照（push imm・bulk） | push は imm オペランド系に包含され sweep1 に集約＝旗アドレス直接はゼロ（v237 維持） |

### §3-2 盲経路の総ざくい（本タスクの新規走査4系統）

| # | 盲経路 | 手法 | 結果 |
|---|---|---|---|
| 1 | **構造体経由（ADDR ロード→[reg+d]）**: `mov reg, B`(B∈0xB3BE00..0xB40000) → `add reg,imm`/`lea`/`mov reg2,reg` 伝播 → `[reg+d]` が 4B span で旗に触れるケース | 関数別データフロー（span 感知・1,264 関数・scanB part1/scanC part1、lea 修正・スタックスロット追跡込み） | **0 件** |
| 2 | **引数/this 伝播経路**: `mov ecx,B` / `push B`（B∈0xB3BE00..0xB3C100）→ 被呼出先が `[this+d]`/`[argN+d]` で旗に触れるケース | 呼出しサイト 1,713 件（758 ユニークシード）から被呼出先をシード付きシミュレーション（scanC part2）。例: 0xB3C000→CS 0x49F470/49F480/0x55E000 系、**0xB3C094/0xB3C08C→0x446CB0（登録ヘルパ）**、0xB3CAC0/0xB3CD7C→0x560920/0x700010 系 | **0 件**（CS は +0x00..+0x17 域、0x446CB0 は旗オフセットへ到達しない） |
| 3 | **4B span 覆蓋読み**: 非フラグ地址 V への dword/word 読みが [V, V+3] で旗バイトを含むケース（例: dword@[0xB3C0AB] は AC/AD/AE を含む） | imm/mem-abs オペランド全数の span 判定（非スタブ帯・scanC part7）＋.text 生 dword 23 件はすべてスチューブ命令エンコード（scanB） | **0 件** |
| 4 | **データセクション間接ポインタ**: .rdata/.data/.bind/.tls/.rsrc 内に旗域地址を指す dword が在り、経由で間接アクセスされるケース | 全セクション byte-aligned dword 走査: 0xB3BE00..0xB3C100（scanA sweep4/4b → scanC part3 拡張） | **実質 0 件**（`.bind+0xE3F1=0xB3C0CB` の1件は **非4バイト整列**の偶然値＝ポインタではない。旗アドレス完全一致 dword は 0 件） |
| 5 | （付随）B+add-imm クロスプロダクト 46 件 | すべて「別用途の imm と add の偶然合成」（例: 0xB3C0EC+(-1)）で、ADDR 伝播実体なし（系統1で棄却） | 棄却 |

### §3-3 周辺オブジェクト境界チェック（「独立 BSS スカラー」v237 記述の精密化）

- `0xB3C000` = CRITICAL_SECTION（+0x00..+0x17）／`0xB3C094`/`0xB3C08C` は登録ヘルパ 0x446CB0 への this（runtime カウンタ/ptr 帯）／pair3 は `0xB3C0D4..0xDB` で終端し **0xB3C0DC（旗）の直前で境界終了**／pair1/pair2 は BC..C3/C4..CB。→ 旗23バイトは**どの既存オブジェクトのフィールドとも重複しない**（系統1・2 のデータフロー検証と整合）。
- v237 §3-5-2 の「base+disp 共起 0 件」は本タスクのデータフロー版で**強化されて維持**。

### §3-4 最終裁定

- **retail バイナリ内では、旗23本は23 スチューブが起動時に 1 を書くだけの bookkeeping マーカーであり、読手は存在しない**（静的解析で到達可能な全経路を走査済み。動的計算地址・バイナリ外モジュールは対象外という前提は v237 と同一）。
- 用途推定は v237 のまま「登録成否の記録（誰も消費しない）」。0xB3C088/89/8A/AB/BA/EC 等の**稼働中 runtime フラグ**（読手あり）とは別個のバイトであり、混同しないこと。

---

## §4 新規残点（本タスクで新たに発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-18 | **vanilla ESM の MGEF DATA+0x18..+0x34 が全ゼロであることの直接未検証**: 「pointer7 常時 NULL」は (a) retail Save が [値+0xC] を逆参照する設計上 vanilla で非ゼロが在れば dirty 化時にクラッシュする、(b) ctor/生成系はゼロ、の整合推論。ESM 本体スキャン禁止のため実値未確認。ESM スキャン許可時に DATA+0x18..+0x34（+0x1C は float 1.0f 期待）を列挙すれば裏付け可能 | 低 |
| N-19 | **slot9(Save) が実際に走る上流経路の未特定**: 汎用 Save 系（FUN_0046B340/FUN_004CD3B0 群）の上流（セーブゲーム書き出し／アクティブパッチへの変更フォーム flush の起動元、および dirty MGEF が生じ得る gameplay 経路）は今回未追跡。約67 個の 0x749280 トークン（0xB3C0BB 等）の個別同定も未実施 | 低 |
| N-20 | v237 §3-2 の「0x9FEC80/CA0/CC0 = NiTMap 系メンバ3個」記述と本タスク実測（2-dword ペア書換）の差異: v237 本文は無編集のままとし、以後の参照では本タスク §2-2 を正とする（訂正ドラフト提示済み・適用は [PORT] 要承認） | 低 |

（N-15/N-16/N-17 の扱い: **N-15 は §1 で解明（slot9=Save・pointer7 二重表示と常時 NULL・slot46 型確証・FUN_00512540 棄却。残余は N-18/N-19）／N-16 は §2 で解明（NiPSys 登録フック・0xB3C0BB は無害トークン）／N-17 は §3 で最終裁定成立（bookkeeping 確定・盲経路4系統ゼロ）。**）

---

## §5 検証情報

- **selftest**: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）。
- **EXE**: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（PE 手動解析: ImageBase 0x400000。.text VA 0x401000 raw 0x400 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00 rawsize 0x30C00（超えは BSS）／.tls 0xBAC000・.rsrc 0xBAD000・.bind 0xBAF000）。capstone 5.0.7（skipdata＋detail・operand access 利用）。
- **本タスクの走査規模**: .text 線形 225.8 万命令を複数系統（scanA: PTR7 全数＋direct call＋flag ref＋base+disp クロスプロダクト／scanB: 関数別データフロー＋引数伝播＋文字列復号＋.text 生 dword／scanC: span 感知データフロー v2＋被呼出先シードシミュレーション＋拡張データ走査／scanD: クラスタ候補同定＋PTR7 書手全列挙／scanE: TypeDescriptor 復号・ctor 確認）。
- **スクリプト（新規のみ・`%TEMP%\opencode\`）**: `v238_scanA.py`(+`_out.txt` 13,677行) / `v238_scanB.py`(+`_out.txt` 443行) / `v238_scanC.py`(+`_out.txt` 2,976行) / `v238_scanD.py`(+`_out.txt` 2,239行) / `v238_scanE.py`。v237 の `v237_*.py` は参照のみ（非改変）。
- **入力（読取のみ）**: `tmp/re/v237_port_mgef_settings_residuals3.md`（本体・§3/§7）/ `tmp/re/v236_port_mgef_settings_residuals2.md`（§5 DATA layout）/ `cache/named_funcs.txt`（`FindSITargetTypeInstance` 等の QI ヘルパ名を含む）/ `cache/vtable_index.tsv` / `docs/Oblivion_decompiled.c`（slot9 :14204・slot46 :14296・assign :14520・loader :14560・ctor :14747・setter :14345・FUN_00512540 :205321・FUN_00518000 :209000・FUN_0046B340 :84816・FUN_0046B990 :85180・FUN_0046AE40 :84483・FUN_0046C360 :85894・FUN_00747C40 :592837・FUN_0070E220 :552051・FUN_00412D30 :11092・FUN_00712590 :555231 等を部分読取）。
- **移植ソース**: 本タスクでは編集・参照ともなし（v237 までの記述を踏襲）。
- **git / ESM**: git 不使用。**ESM 本体は未アクセス**（再スキャン禁止を遵守）。web 資料は未取得（本タスクの3件は EXE 内証のみで裁定）。
