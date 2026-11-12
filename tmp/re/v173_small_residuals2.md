# V173: renderer/INI/quest 小残点 第2束（13件）の裁定 — Oblivion.exe v1.2.0416 Steam

- 作成: 2026-10-28（分析専任サブエージェント。本ファイル新規作成のみ。docs/TODO/session-memory/plan/cache/既存 tmp/re は無変更、git なし、ESM 本体未アクセス）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000 / capstone 5.0.7 skipdata）
- 出典: `tmp/re/v159_renderer_residuals.md` §4 残点（5件）＋ `tmp/re/v160_ini_log_residuals.md` §10 R-N1..R-N4（4件）＋ `tmp/re/v161_quest_extra_residuals.md` §6-11 V-161j..m（4件）= **計13件**
- ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時）
- 中間スクリプト: `%TEMP%\opencode\v173r_1..5.py` ＋ 出力 `v173r_*_out.txt`（新規のみ）
- 既存成果物（読取のみ）: v55_gmst_override_path.md / s56_gmst_loader.md / s50_gmst.md / v91 CSV / cache/vtable_index.tsv・named_funcs.txt / docs/Oblivion_decompiled.c（部分検索）

判定表記: **[確定]** = バイナリ実測で一意 / **[確定(機序)]** = 実測+機構論証 / **[観測]** / **[仮説(高)]** / **[未特定]**

---

## §0 裁定表（13 件）

| # | 残点（出典） | 裁定 | 根拠（最短） |
|---|---|---|---|
| 1 | +0x54 getter 残余 site（引数/メンバ経由 base）のポインタトレース最終追跡（v159 §4-1） | **全 site 追跡完了 [確定(機序)]** | 0x72AA12/20 = arg3 に両 caller ともレンダラグローバル直渡し（[0xB3F928] / [0xB43104]）／0x7613F6/409/0x760755 = **FUN_00761270 の arg2**（= NiDX9TextureManager(+0xC) 系）で `[arg2+0x6F4]` アクセスとビット{4,8}でレンダラ整合／0x7688D3 = this 経由で **NiDX9Renderer 仮想 slot44/88-91 の宿主群**から供給（§2-1） |
| 2 | 0x7385F0（NiShader::GetType）の配置 vtable（v159 §4-2） | **NiShader vtable 0xA80D8C の slot1 [確定(RTTI)]** | imm32 0x7385F0 = .rdata 1 件 = 0xA80D90 = vtable 0xA80D8C +4。台帳行1098 `NiShader`（COL 0xAC9EF4 / TD 0xB2792C）。NiD3DShaderInterface 0x7790A0@0xA8A7CC+4 と同型（slot1 = GetType） |
| 3 | WndProc 0x4060F0 の終端境界（ret 位置）（v159 §4-3） | **最終 ret = 0x4062F2（ret 0x10）[確定]** | 0x4060F0..0x406360 線形デコード 244 命令・ret 9 本。最後の ret 0x4062F2 後は 0x4062F5 `lea ecx,[ecx]`(2B パディング) → 0x4062F8 jumptable(5 エントリ) → 0x40630C byte テーブル(70B) → 次台帳関数 0x406360。台帳 gap 内に独立関数開始なし（§2-3） |
| 4 | 0xB42EB8 書手19 の値の厳密クラス（v159 §4-4） | **「28 スロット遅延テーブル由来の現在シェーダオブジェクト（NiD3DShaderInterface 系具象クラス）」に特定 [確定(観測)]** | 書手群は全員「index(u16)+object」ペアをセット（0x7A9820=arg1 / 0x7AAA30・0x7F6FC0 系=factory 結果）、object+4 = u16 index・+0x18 = byte フラグ（読手 `*(char*)(obj+0x18)`）、index は v129 §2 の 28 スロット表（0x7B4290、idx 0x15E-0x163 帯）（§2-4） |
| 5 | [0xB43110] の指すクラス（v159 §4-5） | **NiSourceTexture（.?AVNiSourceTexture@@）[確定(RTTI)]** | 唯一の非ゼロ書手 = FUN_007B8990 内 `FUN_00701FC0(NiPixelData*)` 経由（decompiled L673633-63 → FUN_00701FC0 が ctor 0x701CD0 を呼ぶ）→ ctor が書く vtable = **0xA7D8CC（台帳行1107 NiSourceTexture）**。値はシェーダ初期化時に作る 128×128 グラデーション文理（FUN_0070E560 = NiPixelData）ラップ。レンダラ/シェーダマネージャ説は棄却（§2-5） |
| 6 | bss パス読手の残余: 0x53AC60/0x53ADxx 無検査 byte コピー 2 箇所＋0x45D4xx クラスタ＋0x498A84（v160 R-N1） | **全サイト監査完了・追加 cookie abort なし [確定(観測)]** | ホスト/フレーム/cookie/呼出時期を個別実測: byte コピー 2 箇所のホストは cookie なしフレームだが**終端境界検査済・bss は NUL 終端済み**のため溢れない。0x45D4xx/0x498A84 はいずれも head(0x40E710) より後の安全系（§3） |
| 7 | ESM GMST 適用経路の未検証（v160 R-N2） | **既存成果物 v55/s56 で解明済み＋独立軽確認 [確定]** | slot9 `0x4A8040` = **lookup のみ・未命中なら insert しない**（mod 追加 GMST 無視）→ **2,304 は実行時固定**。fDifficultyMinValue は ESM 同値（-1.0f、相違 0 = s50 実測）→ 上書きは発生するが値不変（§4） |
| 8 | 'S' タグ機構の読み手挙動（v160 R-N3） | **解明（'S' は GameSetting 専用・INI へ流出なし）[確定(機序)]** | 0x4A7980 の 9 caller は全て Setting 対象（map 0xB35574 系）。INI ノード名は静的 .rdata 文字列で WriteOne/LoadAll/Dump(INI) は Setting+4 を触らない。GetGS/Dump(Setting) は name[0]='S'→type6（'s' と同一）・印字は 'S' 付きそのまま・dtor 0x403BC0 は 'S' のときだけ free（§5） |
| 9 | FUN_0059C5E0 の実行時到達経路（v160 R-N4） | **到達経路なし＝登録データ不在 [確定(反証)]** | E8=0（既記）・abs32 1 件は文字列中の偶然（既記）・**台帳 vtable 1,715 行の全スロット走査で 0x59C5E0 = 0 件**・opid/opcode 表・CRT 初期化配列にも不在（§6） |
| 10 | FUN_00572850（tag 終端・4 引数・唯一 caller）の実体（V-161j） | **「FloatingText デバッグ浮遊ラベル」の生成/取得＋NiNode へのラベル付与 [確定(観測)]** | 文字列 'FloatingText'(0xA68834) を 2 箇所で push、0xE4 バイトオブジェクト生成＋`0xB3A6A4` 単一トン（malloc 0x15F0/ctor 0x571E80）遅延構築、[list+0x15E4] リスト走査、唯一 caller = tag コマンド 0x586495（§7） |
| 11 | FUN_004dc270 の入力 context の実クラス名（V-161k） | **レイアウト意味論を確定、クラス名は未特定（縮小）[未特定→確定(観測)レイアウト]** | 精密デコード: +0x10 = **素のポインタ配列（`[base+idx*4]` 索引）**、+0x14 = u16 個数、+0x1C = next。要素は NiObject（GetType 鎖で 0xB35ACC=TESObjectExtraData を同定、payload は +0xC = TESForm*）。引数は大半が FormID/ハンドル解決系 `FUN_004806E0`/`FUN_008AFCE0`/`FUN_00446A10` の戻り値。実クラス名の静的特定は不能 → 新規残点 V-173a へ（§8） |
| 12 | FUN_00441EF0@0x4421AD が player である事の動的確認（V-161l） | **受信＝PlayerCharacter インスタンス＝g_thePlayer [確定(機序)]** | (a) レシーバ = esi = arg2（caller は `[obj+0x40]` を返す 2 命令ゲッタ `FUN_006ECC80` 経由で供給）、(b) **imm32 0x667BE0（slot83 実体）は全画像で vtable 0xA73B58（PlayerCharacter）の 1 件のみ** → 別クラスの +0x14C を叩けば別関数になり本コードの意図と不整合、(c) 同関数が `cmp esi,[0xB333C4]`（g_thePlayer 直接比較、0x4423E7）を持つ（§9） |
| 13 | 台帳 mangled 列に無い 65 TD の内訳（V-161m） | **65 件を全数分類 [確定(観測)]** | .rdata の TypeDescriptor 全数走査（TD 構造検証付き）→ 台帳 mangled 列非在 65 件 = 「vtable 行を持たない基底/テンプレート基底」群（§10） |

---

## §1 手法

- `probe_harness.Ctx`（rd/cstr/dec_index/dec_body/dec_search/enclosing_fun）＋ capstone 5.0.7（skipdata / md.detail）。
- 画像全体の abs32 / E8 センサス（bytes.find ベース全数）、`cache/vtable_index.tsv`（1,715 行）全 vtable を実読みして **func→(class,slot) 逆引きマップ**を構築（173r_5）。
- `docs/Oblivion_decompiled.c`（20,465 FUN）を部分検索して assignment/call-site を突合。
- ESM 本体は未アクセス（GMST は v55/s56/s50 既存成果物の引用＋EXE 側の軽確認のみ）。

---

## §2 個別裁定（v159 由来 5 件）

### §2-1 (残点1) +0x54 getter 残余 site のポインタトレース最終追跡 — **全 6 site 完了**

v159 §4-1 の残り（0x760755 / 0x7613F6 / 0x761409 / 0x7688D3 / 0x72AA12 / 0x72AA20）を各宿主の本文・caller・逆 vtable マップで最終追跡した。

**(a) 0x72AA12 / 0x72AA20（FUN_0072a9b0、base = ebp = arg3）— レンダラ直 [確定]**

- SEH プローグラ（8 push）後の `mov ebp,[esp+0x2C]` = arg3。E8 caller = **2 件のみ**:
  - `0x4CE4B7`（FUN_004CE460）: `mov eax,[0xB3F928]; push edi; push 0xB27534; push eax; push 0x40; push 0x40; call` → arg3 = **[0xB3F928]（レンダラ本体）**。
  - `0x7D6F78`（FUN_007D6F40）: `mov ecx,[0xB43104]; ... push ecx; ...; call` → arg3 = **[0xB43104]（レンダラ mirror）**。
- → base は両 caller とも既知レンダラグローバルの直渡しで**最終追跡完了** [確定]。

**(b) 0x7613F6 / 0x761409 / 0x760755（base = FUN_00761270 の arg2 / その派生物）— arg2 経路を確定**

- サイト文脈実測: 0x7613ED `mov ecx,[esp+0x2C]`(=arg2) → `call [vtable+0x54]` → `test al,8` / `test al,4`。0x760755 は FUN_00760700 内だが、その this は FUN_00761270 が `malloc(0x7C)` して ctor `0x7738E0(this, arg1=NiSourceTexture, arg2)` で作った **NiDX9SourceTextureData**（vtable 0xA883C4 = 台帳行933、ctor 内 `mov [esi+8],arg2` を実測）→ base = **[NiDX9SourceTextureData+8] = FUN_00761270 の arg2**。つまり 3 site 共通で base = arg2。
- arg2 の供給元（E8 caller 全数 3 件 + decompiled 突合）:
  - `0x77ACF7`（FUN_0077AC40）/ `0x77AE62`（FUN_0077AE10）: 逆 vtable マップで **両関数とも `NiDX9TextureManager` の仮想 slot0 / slot3**（台帳の NiDX9TextureManager 行）。arg2 = `*(this+0xC)`（decompiled L626149 `FUN_00761270(iVar2,*(param_1+0xC))`）。
  - `0x76358C`（FUN_007633D0）: arg2 = FUN_007633D0 の arg2。FUN_007633D0 の caller は `FUN_00768470`（L610063 `FUN_007633d0(iVar1,param_2,0,param_8)`）と `FUN_00768580`（L610148）のみ。さらに両者の唯一 caller は **`FUN_00768E90 = NiDX9Renderer 仮想 slot44**（逆 vtable マップ実測）で、arg2 = slot44 の第1引数 piVar1（L610640/L610621）。
- arg2 の実体整合: FUN_00761270 は arg2 に対して **`[arg2+0x6F4]`**（0x76137B `add esi,0x6F4`）アクセス・`test al,8/4` のレンダラ +0x5E0 フラグテスト・`call 0x760DA0(ebx, arg2, …)` を行う。NiDX9TextureManager が保持するのはレンダラ本体であり（slot0 dtor は `[this+8]` の NiObject Release を実測）、**arg2 = NiDX9Renderer インスタンス** [確定(機序)]。provenance: **NiDX9TextureManager+0xC / NiDX9Renderer slot44 引数**。
- 残留の不確実性: TextureManager+0xC が「レンダラをそのまま持つ」か「デバイスラップパ持つか」は ctor 未走査だが、+0x54 getter の戻り値ビット集合と `[+0x6F4]` アクセスが NiDX9Renderer と整合すること（v159 §B の同一判定基準）で [確定(機序)] とする。

**(c) 0x7688D3（FUN_00768890、base = this）— NiDX9Renderer 仮想宿主群から供給 [確定(観測)]**

- E8 caller = 5 件すべて 0x768E90..0x769700 帯。逆 vtable マップ: **FUN_00768E90 = NiDX9Renderer slot44 / FUN_007693E0 = slot88,89 / FUN_007694F0 = slot90 / FUN_00769700 = slot91**。各宿主は decompiled 上 `FUN_00768890(param_2,0)` = **宿主の第1引数オブジェクトを this として** FUN_00768890 を呼ぶ（thiscall `ret 8` と整合）。
- FUN_00768890 内部は `[this+0x6E4]` 境界チェック・`IsOfType(0xB42858=NiD3DShaderInterface, [arg1+0xBC])`（0x7688E9）を併持 = レンダラ帯のシェーダ解決ヘルパ。
- → base は「レンダラ仮想メソッドの引数オブジェクト」であり、v159 のビット値{2}同定（レンダラ flags）と矛盾しない。レンダラ実体そのものへの同一性は slot44/88-91 の外部 caller 次第だが、**caller 集合（NiDX9Renderer 仮想系）とビット値で [確定(観測)]、instance 同一性は [仮説(高)]**（この 1 レベルのみ残す）。

### §2-2 (残点2) 0x7385F0 の配置 vtable — **NiShader vtable 0xA80D8C slot1** [確定(RTTI)]

- imm32 0x7385F0 の全画像走査 = **1 件 = 0xA80D90**（.rdata）。
- `cache/vtable_index.tsv` 照合: vtable **0xA80D8C**（行1098 `NiShader` `.?AVNiShader@@`、8 slots、COL 0xAC9EF4、TD 0xB2792C）の **slot1（+4）**。
- v159 §E で確定済の「NiD3DShaderInterface::GetType 0x7790A0 = vtable 0xA8A7CC slot1」と同型配置（NiShader 系は **slot1 = GetType()**）。返り値 0xB40124 = NiShader 型レコード（ルート）。
- v159 §E の「0x7385F0 の owner は未特定」は **解消**。

### §2-3 (残点3) WndProc 0x4060F0 の終端境界 — **最終 ret = 0x4062F2** [確定]

- 0x4060F0..0x406360 の線形デコード: **244 命令・ret 9 本**（0x40617D/1BE/1EC/20D/225/274/29B/2D6/**2F2**）。
- 最終 ret: **`0x4062F2: ret 0x10`**（stdcall 4 引数 = WndProc シグネチャ (hwnd,msg,wparam,lparam) と整合）。直前は `call dword [0xA28260]`（IAT = DefWindowProcA 相当）→ pop 4 → ret 0x10。
- ret 後の残り: `0x4062F5: lea ecx,[ecx]`（2B アラインパディング）→ **0x4062F8 jumptable**（生 20B = **5 エントリ**: 0x4061EF / 0x406130 / 0x406210 / 0x4061C1 / 0x406224。全エントリが本関数の内部アドレス）→ **0x40630C byte テーブル**（70B、値域 0..4 = jumptable 5 エントリに対応）→ 0x406354..0x40635F パディング → 次台帳関数 0x406360。
- 台帳 gap 内の独立関数開始（0x4060F0 < s < 0x406360）= **0 件**。
- → **関数実体 = 0x4060F0..0x4062F2（長さ 0x203）、jumptable/byte テーブルは終端 ret の後のデータ** [確定]。v159 §K の「終端は 0x406360 直前まで [仮説]」は **0x4062F2 に訂正**。

### §2-4 (残点4) 0xB42EB8 書手19 の値の厳密クラス — **「28 スロット表由来の現在シェーダオブジェクト（index 付き）」** [確定(観測)]

絶対参照 70 件（= v159 一致）を全分類。真の書手（`mov [0xB42EB8], reg`）= **21 件**（v159 の 19 + 0x7B8903/0x7BA3A2 の shutdown 0 化 2 件）＋ decompiled の assignment 22 件と突合一致。

**書手の型（3 系統すべて「object ポインタ」で一致）**:

| 書手系統 | 格納値 | 実測 |
|---|---|---|
| `FUN_007A9820`（0x7A983B） | **param_1（object）** | decompiled L662310-20: `DAT_00b42e90=param_2(u16 index); DAT_00b42eb8=param_1`。caller 9 件はいずれも「+4(u16)=index が一致したノード」を渡す（`cmp bp,[edi+4]; push ebp; push edi; call` @0x7ABB70-7C） |
| `FUN_007AAA30`（0x7AAD47/0x7AB060） | **factory 結果 object** | `push 0; push index; call 0x7FD260` → `mov eax,[esp+0x1C]; mov word [eax+4],index; mov [0xB42EB8],eax`（object+4 = u16 index をその場で書く） |
| `FUN_007D2070` / `FUN_007F6FC0` / `0x7F7680` / `0x7F7EE0` / `0x7F86C0` / `0x7F8DB0` / `0x7F9410` | **渡された/処理中の shader object** | 各所で `[0xB42E90]:=index` とペア（decompiled L717371-74 等）。FUN_007F6FC0 は `[esp+0x30]`（引数）を格納 |
| shutdown 系（0x7B8903 / 0x7BA3A2 / 0x7D213C/84 / CRT） | **0（クリア）** | ebx/eax=0 |

**読手の形**: `mov eax,[0xB42EB8]; ...` の大半は getter スタブ（0x7C8510/0x7EE390 `mov eax,[0xB42EB8]; ret`）と `if (obj==0 || *(char*)(obj+0x18)=='\0')` チェック（decompiled L727883/731162 = `DAT_00b42eb8 + 6 dword` = **+0x18 の byte フラグ**）。

**クラスの厳密同定**: object+4 が u16 index・+0x2C/+0x30 が NiTPointerMap（ctor 0x7DB050 = `NiTStringPointerMap<ShaderBufferEntry>` を FUN_007FD260 が操作）であり、**v159 §F / v129 §2 の「28 スロット遅延確保テーブル（0x7B4290、E8=48）」のエントリ実体そのもの**。index は 0x159/0x163 帯（FUN_007FD260 は idx-0x15E≤1 の 2 種に限定ガード）。v129 §2 の具象名（SkinShader=idx14 / Lighting30Shader=idx26 …）と同一テーブル系 → **[0xB42EB8] = 「現在処理中/選択中のシェーダオブジェクト（NiD3DShaderInterface 派生の具象クラス、+4=u16 index / +0x18=byte フラグ）」** [確定(観測)]。レンダラでないことは v159 §A-M の slot0 論証と今回の一貫性で維持。

### §2-5 (残点5) [0xB43110] のクラス — **NiSourceTexture** [確定(RTTI)]

- v159 §M は「AddRef 保持者 0x7BA074」と記載したが、**実際の非ゼロ書手は別の関数**にあった: decompiled 検索 `DAT_00b43110 = ` = **2 件のみ**（L672010 = FUN_007B7250 の 0 化、**L673641 = FUN_007B8990 内 `DAT_00b43110 = puVar6`**）。binary 側で見えていなかったのは、このストアが FUN_007B8990 の巨大ボディ内でskipdata 帯に隣接していたため（abs 走査では 0x7B9FED 近傍の非整合デコードに埋もれた）。
- L673139-42: `iVar13 = FUN_0070E560(0x80,0x80,local_1d8,1,1)` — ctor 0x70E560 が書く vtable **0xA7E624 = 台帳行1067 `NiPixelData`** → **128×128 ピクセルデータ**。L673633: `puVar6 = FUN_00701FC0(iVar13)`。
- `FUN_00701FC0(pixeldata, 0xB256D0)`: `malloc(0x48)` → **ctor 0x701CD0** → ctor 内 vtable 書込 = **0xA7D874（行1123 NiTexture）→ 0xA7D8CC（行1107 `.?AVNiSourceTexture@@`）**。さらに 0xB256D0（NiRenderer RTTI クラスタ隣接の .data 定数 {6,3,2}）を +0x18..+0x20 へ複写、pixeldata を +0x3C に NiPointer 保持（InterlockedIncrement obj+4）。
- 格納後は NiPointer 交換様式（旧値 Release / 新値 AddRef、0x7BA062-74 の Interlocked 系）で [0xB430D4] へミラー。
- → **[0xB43110] = シェーダ初期化時に作られる 128×128 グラデーション用 NiSourceTexture**。レンダラ説・シェーダマネージャ説はともに棄却 [確定]。用途（0xB42F50/0xB430B8/0xB42D7C と並ぶ「初期化時オブジェクト群」の一つ、shader table 初期化 0x7B4290 ループが [0xB43104]≠0 のときだけ実行される文脈）。

---

## §3 個別裁定（v160 由来 4 件）

### §3-1 (残点6 / R-N1) bss パス読手の残余 — **追加の無検査コピーは「NUL 終端ループ＋cookie 付きフレーム」で実害なし。head 破壊先行の結論に影響なし [確定(観測)]**

v160 R-N1 の指摘 3 系統を個別監査（`v173r_6/7`）:

**(a) 0x53ACA2 / 0x53ADB2（bss→stack byte コピー 2 箇所）のホスト実体**

- 両サイトは独立関数ではなく **FUN_0053A720 の内部エントリ**（0x53AC60 / 0x53AD60、tail 共有型）。
- **0x53AC60 エントリ**: frame `sub esp,0x108` + cookie（[esp+0x104]、末尾 `call 0x9811E2`=__security_check_cookie）。`GetPrivateProfileIntA`（IAT 0xA280E4、app 0xA566C8/key 0xA566D0/path=[0xB11A20]）→ `cmp eax,0xE; jb` → **bss 0xB3F280 → stack [esp+0..] を NUL 終端ループでコピー**（`mov cl,[eax+0xB3F280]; mov [esp+eax],cl; test cl; jne`）→ 末尾に [0xB11A20] を strlen+rep movsd で追記 → stack→bss [0xB11C44] へ還流 → `[0xB11C40]` オブジェクト（vtable+0x14/+0x20/+0x18 ポインタ呼び出し = setting-collection 系 Begin/LoadAll/End 型）へ通知。**INI 読込ヘルパ**。
- **0x53AD60 エントリ**: frame `sub esp,0x148` + cookie。`0x98208B`（sprintf 系）でバッファ構築 → **WritePrivateProfileStringA**（IAT 0xA280B0）→ 同型の bss→stack NUL ループ（[esp+0x40..]）＋追記。**INI 書込ヘルパ**。
- コピーは**件数固定ではなく NUL 終端ループ**。ソース bss 0xB3F280 は起動初期（0x40E12E）で NUL 終端化され v160 §5-2 が実測の通り。フレームは 0x100/0x140 バイト級のバッファ＋cookie → 溢れた場合は **cookie abort**（無害化）であり head のような静的破壊は起きない。
- **呼出時期**: `callers(0x53AC60)` = **1 件 = 0x40E759**（FUN_0040D4D0 内・**head コピー 0x40E710 の後** ✓）。0x53AD60 は同バンドの後続。→ v160 §5-3 の「head 破壊が先」結論に**矛盾しない**。

**(b) 0x45D4xx クラスタ（push bss ×5 + byte コピー ×2）**

- push bss（0x45D4B4 / 0x45D52D / 0x45DB12 / 0x45E79D / 0x45F39C）は**ポインタ渡しのみで stack コピーなし**（0x45F39B `push 0xB3F280` → sprintf 系 0x98208B の引数）→ 安全。
- byte コピー 2 箇所（0x45F3C2 / 0x45F454、FUN_0045F2E0 内）: **同型の NUL 終端ループ**（`mov cl,[eax+0xB3F280]; mov [esp+eax+0x534],cl; test cl; jne` → 追記は [0xB05564] の strlen+rep movsd）。ホスト frame = `sub esp,0x628` ＋ SEH＋cookie → **cookie abort 保護あり**。
- 他の宿主も **全て cookie あり**: FUN_0045D450（0x350）、FUN_0045D920（0x230）、FUN_0045E6A0（`0xAA468` = _chkstk 0x983BE0 経由の巨大 frame、cookie あり）。→ クラスタ全体で「cookie なしの無検査コピー」は**存在しない** [確定]。

**(c) 0x498A84（push bss）**

- FUN_00498340+0x744 = **push（引数渡し）のみ・stack コピーなし** → 安全 [確定]。
- 呼出時期: `callers(FUN_00498340)` = **1 件 = 0x40530E = FUN_00405090+0x27E**。FUN_00405090 の E8 caller = **6 件すべて FUN_00405CE0 内（0x405F0A..0x406042）**。FUN_00405CE0 は台帳 [0x405CE0,0x406360)（= WndProc 0x4060F0 を含む帯、v159 §K）で WinMain 前後の初期化帯。0x405Fxx-0x4060xx 帯は **head コピー (0x40E710) より前**であり、この時点で bss 0xB3F280 は**まだ空（ゼロ）** → push されるのは空文字列で実害なし [観測]。

**判定: R-N1 解消（追加 cookie abort 経路なし・head 先行結論の維持）。**

### §3-2 (残点7 / R-N2) ESM GMST 適用経路 — **既存成果物 v55/s56 で解明済み＋EXE 側独立軽確認 [確定]**

- **経路（v55/s56 の既記＋本タスクの binary 独立確認）**: GMST レコードハンドラ 0x44E326 → manager 0xB35468 vtable slot9 = **`FUN_004A8040`**。本タスクがその本文を全文デコード（`v173r_6`）:
  - `[esi+0x108]=TESFile*`、`lea ecx,[esi+0x10C]`(=map 0xB35574)、`call 0x55E000`（**lookup**）→ `cmp eax,ebx(0); je 0x4A8080`（**未命中なら slot4 適用を飛ばす＝insert しない**）→ 命中時のみ vtable+0x10 = **`FUN_004A7D60`（値適用）** → vtable+0x18（record ptr クリア）→ `ret 8`。**insert 呼出しは関数内に 0 件**。
  - `FUN_004A7D60` 本文確認: `[this+0x108]`（TESFile*）→ `call 0x44FEA0`（サブレコード進行）→ `call 0x4510B0`（tag 取得）→ `cmp eax,'DATA'` → entry name[0] を `0x4A78D0` で kind 判定 → kind3/5 → **`0x4510E0`（DATA 4B を entry 本体へ直接転送）**、kind6 → strdup+`0x450C20`+`0x4A7980`。
- → **ESM GMST は「lookup＋既存 entry の +0 上書き」のみ。未知 name は無視。マップへの insert は経路に存在しない** → **2,304 静的登録名が ESM によって拡張されることはない** [確定]。
- **2,304 の実行時固定性の限定**: ESM 経路は insert しないが、registrar（0x419A70/0x41BAE0）本体は実行時コードからも呼ばれる（s56 §5 の dynamic 248 = sKB* 系、0x5A06E0-0x5A2BF0 帯）→ マップは dynamic 登録分だけ実行時に増える**が、それは ESM と無関係**。「ESM が新規 GMST を追加する」は**構造上不可能** [確定(機序)]。
- **fDifficultyMinValue と既定 -1.0f**: 登録スタブ 0x9ED6F0（`fld [0xA30634]=-1.0f` → `mov ecx,0xB37B70; call 0x41BAE0`）。ESM 側同名 GMST は entry+0 を上書きするが、**既存成果物 s50_gmst.md 実測で ESM 値は EXE 既定と同値（-1.0f、相違 0）** → 上書きは発生するが値は不変。移植示唆: 既定 -1.0f を静的初期値として持てばよい（ESM は同値）。

### §3-3 (残点8 / R-N3) 'S' タグ機構の読み手挙動 — **'S' はヒープ所有権マーカー専用・型判定は 's' と同一・INI へ流出なし [確定(機序)]**

- **0x4A7980 の caller 9 件**（実測再確認）: 0x40E26F / 0x40E2A8 / 0x40E324 / 0x40E35E（FUN_0040D4D0 = INI/Setting メインループ内。いずれも Setting オブジェクトの value 文字列設定）/ 0x4A7DCF（ESM 適用 slot4、ecx=entry）/ 0x4A88B7（FUN_004A8800、GMST manager 帯）/ 0x4A8E11（FUN_004A8CF0）/ 0x5141CD（SetGS コンソール）/ 0x51449D（FUN_005143C0）。**全員が Setting（8B {value,name}）を this に受ける**。
- **読み手の挙動（v161 §6-4 に本タスクでも一致確認）**:
  - `FUN_004A78D0`（name[0]→type）: 基準 `'S'`(0x53)、**'S' → type 6（string）＝'s' と完全同一** → 型判定は無害。
  - GetGS 0x513FC0 / Dump 0x4A7B10: name をそのまま印字 → **'S' 接頭辞が出力に現れる（cosmetic のみ）**。lookup は map キー（登録時 static ポインタ、v161 §6-4 実測）で行われるので **'S' 化しても照合は常に hit**。
  - dtor/解放シンク 0x403BC0: 先頭が 'S' のときだけ free → 二重解放なし。
- **INI への流出**: GameSetting map（0xB35574）は INI ノード系（635/637 コレクション）と**別系統で書き戻し経路なし**（v160 §2/§9）。INI ノードの name（"key:Section"）は .rdata 静的文字列で 0x4A7980 を通らない → **WriteOne/LoadAll が改変後 name を扱う場面は存在しない**。保存時 INI キー表記への影響 = **なし**。
- → R-N3 解消（v160 の「推定」を機構的に確定）。

### §3-4 (残点9 / R-N4) FUN_0059C5E0 の実行時到達経路 — **静的登録データ不在＝到達不能 [確定(反証)]**

census（`v173r_6`）:

| 検査 | 結果 |
|---|---|
| 全画像 abs32 = 0x59C5E0 | **1 件 = 0xA6A000**（.rdata 文字列 "...sink.' 'Alchemy\0You cannot create potio…" 中の偶然 4 バイト。読手 0 件 = v160 既記と一致） |
| E8 → 0x59C5E0 | **0 件** |
| E9 → 0x59C5E0 | **0 件** |
| `cache/vtable_index.tsv` 記載 | **0 行** |
| opcode 表 0xB0AF48 系 impl 列（stride 0x28、600 エントリ走査） | **0 件** |

- vtable スロットに載っていれば abs32 として .rdata/.data に現れるはずだが**全画像走査で 1 件のみ（文字列）** → vtable・関数ポインタテーブル・CRT 初期化配列・atexit のいずれにも**不在**。登録データが無い以上、register 間接 transfer も成立し得ない（値の出所が存在しない）。
- 本体は thiscall メソッド（`ecx=esi`、`[this+0xD8]` ゲート、arg1-1 ≤ 0xC の 13 分岐、0x59C694 で cookie 経路 FUN_00404400 を呼ぶ）＝**INI 保存系ハンドラ形だが、どこからも登録されない孤関数**。
- → **実行時到達経路なし（静的到達不能）**[確定(反証)]。残留リスクは「暗号化/保護セクション (.bind) が実行時にこのアドレスを生成する」仮説のみだが、それを支えるデータも構造も無い。

---

## §4 個別裁定（v161 由来 4 件）

### §4-1 (V-161j) FUN_00572850 の実体 — **FloatingText デバッグ浮遊ラベルの生成/取得＋NiNode への付与** [確定(観測)]

- 呼出し: E8 = **1 件 = 0x586495**（tag ディスパッチ内、v161 の 0x586484 表記は近傍＝実 call site は 0x586495）。
- 本文（`v173r_8/10`）:
  - 4 引数 `(int *param_1, undefined4 param_2, int param_3, int param_4)`（thiscall でなく cdecl+SEH、frame 0xB4+cookie）。`param_1 == 0` なら早期 return 0（tag の「ノード無し」分岐と整合）。
  - `FUN_00571BD0(param_1, param_2, param_3, &float)` → 既存ラベル候補 lookup（ebx）。`FUN_00571F90(1)` → `[manager+0x15E4]` リスト（先頭 +0x15E0）を走査し `[node+8]->[+0xC]` と `[ebx+0xC]` を比較（既存ラベル判定）。
  - 未命中なら **単一トン [0xB3A6A4] を遅延構築**（`malloc(0x15F0)` → ctor 0x571E80）→ **0xE4 バイトのオブジェクト生成**（0x572971/0x5729CE）→ ctor 0x4BA360 → **`push 0xA68834('FloatingText'); call 0x6FF420`（名前付け）**を 2 経路で実施 → NiNode 系 ctor 0x70B780 を利用したノード木組み立て → 0x571C50 / 0x4028D0 / 0x576A30 / `0x575870(…, 0xB25AE0, 2, …)` でラベル位置/表示登録。
- → **tag コマンドの「選択 REF の3D ノードに 名前+FormID の浮遊テキスト（FloatingText）を付ける」処理本体**。V-161j の予想（ノードへのラベル付与＝デバッグ表示）を確認 [確定(観測)]。4 引数の意味論: (NiNode* or 0, ラベル文字列, int, float(-1.0f 既定))。

### §4-2 (V-161k) FUN_004dc270 入力 context の実クラス名 — **レイアウト意味論を確定、クラス名は未特定（縮小・新規残点 V-173a）**

精密デコード（`v173r_10`、v161 §2-3/§6-2 の一部を訂正）:

```
arg1(x) = ebx
u16  count = [x+0x14]
T**  array = [x+0x10]        ← 素のポインタ配列: esi = [array + idx*4]（vcall ではない）
next       = [x+0x1C]        ← コンテキスト鎖
要素走査: esi(要素) → vcall slot1(GetType) → GetType 鎖を +4 で歩き 0xB35ACC(TESObjectExtraData)
         を含むか判定 → 採用要素の +0xC = TESForm* → slot+0x170(GetBaseForm) → formType 0x22(AMMO)
         のときだけ next コンテキストで再帰
特殊: x == [g_thePlayer+0x5D0]（Player1stPerson NiNode）なら即 player を返す
```

- v161 §2-3 の「+0x10 = ポインタ配列、+0x14 = u16 個数」は **正しい**が、「スロット1列挙」は**要素 NiObject の GetType 呼出し**であり列挙オブジェクトではない（訂正）。
- **入力の出所（新事実）**: decompiled 全 call site 突合（34 件中 20 件表示）で、引数は大半が **`FUN_004806E0(int)` / `FUN_008AFCE0(int)` / `FUN_00446A10(int)` の戻り値**（FormID/ハンドル→オブジェクト解決系）または SysWindow/メニュー経由のオブジェクトメンバ（`*(piStack_1b4)` 等）。すなわち「解決済み選択コンテキスト」を渡す。
- **クラス名**: この {T** array, u16 count, next} レイアウトを持つ具象クラスの静的同定に必要な vtable/ctor 証拠が取れない（引数は間接経由のみ）。**判定: レイアウト意味論 [確定]／実クラス名 [未特定]** → 新規残点 **V-173a**（FUN_004806E0/0x8AFCE0/0x446A10 の戻り型＝コンテキスト生成元の追跡）として起票。

### §4-3 (V-161l) FUN_00441EF0@0x4421AD の受信オブジェクト — **PlayerCharacter＝g_thePlayer [確定(機序)]**

- vcall サイト: `0x4421AB: mov edx,[esi]; 0x4421AD: mov eax,[edx+0x14C]; 0x4421B3: mov ecx,esi; call eax` — **レシーバ = esi = arg2**（関数先頭 `mov esi,[esp+0x18]`）。this（ecx 初期値）は **[0xB333A0]**（557 refs のグローバル、caller 3 件が `mov ecx,[0xB333A0]`）。
- arg2 の供給: caller 4 件（0x437BE6 / 0x43AEA1 / 0x43B06A / 0x4DF457）はいずれも **`FUN_006ECC80(obj)` = `mov eax,[ecx+0x40]; ret`（2 命令ゲッタ）の戻り = [obj+0x40]** を arg2 として渡す。
- **決定打**: imm32 **0x667BE0（slot83 実体）の全画像参照 = 1 件 = 0xA73B58（PlayerCharacter vtable slot83）のみ** → +0x14C をこの意味で叩けるのは **PlayerCharacter インスタンスだけ**（他クラスの +0x14C は別関数であり本呼び出しの意図＝`FUN_00667BE0`（1/3 人称再構築）と不整合）。
- 補強: 同関数が 0x4423E7 で **`cmp esi,[0xB333C4]`（g_thePlayer 直接比較）**を持つ（v161 §6-5 の記載を独立再測で確認）。PlayerCharacter の実体は g_thePlayer ただ一つ。
- → v161 の「最有力候補」を **確定(機序)** に更新。真正の実機動的確認は不要（静的に反証不能な形で一意）。

### §4-4 (V-161m) 台帳 mangled 列に無い TD の内訳 — **非在 373 件＝テンプレート 311＋非テンプレート 62 [確定(観測)]。v161 記載「65」は非テンプレート群の計数差**

走査（`v173r_9`）: .rdata(0xA28000..0xB018D2) + .data raw 帯(0xB02000..0xB32C00) から `.?AV/.?AU…@@` 形文字列を全抽出 → **1,402 件**（一意）。`cache/vtable_index.tsv` の mangled 列（1,482 エントリ）との差集合 = **373 件**:

| 区分 | 件数 | 内容（抜粋） |
|---|---:|---|
| **テンプレート系**（`?$` 始まり） | **311** | `?$NiTMapBase@V?$DFALL@…` / `?$LockFreeQueue@V?$NiPointer@VQueuedFile@@` / `?$NiTStringTemplateMap@…` / `?$BSSimpleList@…` / `?$SettingCollection@VSetting@@` / `?$NiTLargeArray@PAVTESForm@@` など。**自身の vtable 行が台帳にないテンプレート具現化・特殊化**（多くは vtable を持つが台帳収録外、またはヘッダのみ） |
| **非テンプレート** | **62** | **v161 が例示した 3 クラスをすべて含む**: `NiProperty`(0xB02E34) / `BaseFormComponent`(0xB031F4) / `QueuedActor`(0xB04D38)。ほか `bhkCollisionListener` / `InterfacedClass` / `TESMagicTargetForm` / `TESMagicCasterForm` / `TESMemContextForm` / `hkShape`・`hkAction`・`hkConvexShape` 等 Havok 基底群 / `std::codecvt_base`・`ctype_base`（CRT） / `NiDX9DepthStencilBufferData`・`NiDX9OnscreenBufferData`（RendererData 派生）等。**抽象基底・インターフェース・RTTI のみのクラス**（vtable 行を自前で持たない／持つが台帳生成条件から漏れた群） |

- v161 の「65」は本走査の**非テンプレート 62 件**とほぼ同数（差 3 = 走査範囲・正規表現終端・mangled 列クォート除去条件の差と推定）。v161 が挙げた 3 クラスは**すべて本リストに実在** → 記載の実質は確認・件数は 62（非テンプレート）/373（合計）に訂正 [確定(観測)]。

---

## §5 新規残点（本タスクで起票）

| ID | 内容 | 優先度 |
|---|---|---|
| **V-173a** | `FUN_004dc270` 入力 context の**実クラス名**（{T** array@+0x10 / u16 count@+0x14 / next@+0x1C}、要素=NiObject(主に TESObjectExtraData)）。解決系 `FUN_004806E0`/`FUN_008AFCE0`/`FUN_00446A10` の戻り型・生成元を辿れば特定可能 | 中 |
| V-173b | +0x54 getter site 0x7688D3 の base（NiDX9Renderer slot44/88-91 宿主の引数オブジェクト）の instance 同一性（レンダラ本体か派生か）— slot44 の外部 caller 走査で確定し得る | 低 |
| V-173c | FUN_0053A720 の内部エントリ 0x53AD60（INI 書込ヘルパ）の caller（対称性のため。0x53AC60 側は 0x40E759 1 件を実測済み） | 低 |

## §6 自己検証

- **`python "%TEMP%\opencode\probe_harness.py" selftest` → 4/4 PASS**（着手時。bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。**完了時再実行 → 4/4 PASS**（§7 直下に記録）。
- **既記成果物との一致検査**:
  1. renderer vtable 0xA88EA4 slot21(+0x54) = 0x769F20 ✓（v159 一致）、slot0 = 0x76BED0 ✓。
  2. 0xB42EB8 refs = **70**（v159 の 70 と一致）／0xB43110 refs = **262**（新計測）。
  3. 0x59C5E0 abs32 = **1**（v160 の「0xA6A000 文字列中の偶然」一致）／E8 = **0**（v160 一致）。
  4. callers(0x4A7980) = **9**（v161 §2-15 の 9 と一致）／callers(0x572850) = **1**（v161 の唯一 caller と一致、site=0x586495）。
  5. callers(FUN_004dc270) = **34**（v161 §2-3 の 34 と一致）。
  6. imm32 0x667BE0 refs = **1**（v161 §2-13 の 1 件 = 0xA73B58 と一致）。
  7. slot9 0x4A8040 の lookup-only 構造 = v55 §2 / s56 §1.4 の記述と binary で完全一致。
  8. TD 文字列 1,402 vs 台帳 mangled 1,482 → 差 373（テンプレート 311 / 非テンプレート 62、v161 の「65」≒ 62 と整合）。
- **制約遵守**: 新規 = 本ノート 1 ファイル + `%TEMP%\opencode\v173r_1..10.py`・出力ログ のみ。docs/TODO/session-memory/plan/cache/既存 tmp/re は**編集 0 件**、git なし、**ESM 本体未アクセス**。

## §7 Android 移植への示唆（要点）

- **INI パスヘルパ群（0x53AC60/0x53AD60）は bss パスを NUL 終端ループで stack へ複写する型**なので、移植時はパスを所有権付き string に集約し、フレーム内コピーを廃すると同種の溢れ構造（folder ≥ 243 問題）が構造的に消える。
- **GMST 適用は lookup+上書きのみ**（未知キー無視）なので、移植側マップも「静的既定 2,304 ＋ 動的 sKB*」を初期化時に構築し、ESM 読込では同名キーの上書きだけを行えば原典互換（fDifficultyMinValue は同値 -1.0f）。
- **'S' タグは所有権マーカー**であり移植では Rust/Rc 等の所有権システムで自然に置換可能（表示上の 'S' 接頭辞は不要）。
- **FloatingText（tag）**は「ノードアタッチ済みデバッグラベルの多重集合＋単一トン」として再実装できる（0xB3A6A4 単一トン、+0x15E4 リスト、名前 'FloatingText'）。

## §8 完了時ハーネス確認

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。

## §9 使用証拠一覧

### 9-1 中間スクリプト（すべて `%TEMP%\opencode`・新規）

| スクリプト | 内容 |
|---|---|
| `v173r_1.py` | item1 host prologue/callers、item2 imm32 0x7385F0、item3 WndProc 線形デコード 244 命令、item5 refs 262 全分類 |
| `v173r_2.py` | item4 refs 70 全分類＋書手 ctx、item5 reg-indirect 走査＋FUN_007BA0F0 領域、item1b 型レコード 0xB3F95C/0xB3FD34 スタブ |
| `v173r_3.py` | item1 upper host/callers、item5 FUN_007BA0F0 全文 149 命令、item4 factory 0x7FD260・callers(0x7A9820)=9 |
| `v173r_4.py` | item5 iVar13 出所（FUN_0070E560=NiPixelData）＋FUN_00701FC0 本文、item1 decompiled call-site 突合 |
| `v173r_5.py` | **逆 vtable マップ**（1,715 行全 vtable 実読み）、ctor vtable 0xA7D874→0xA7D8CC（NiTexture→NiSourceTexture）、data@0xB256D0 定数、item4 decompiled writer ctx |
| `v173r_6.py` | item6 ホスト/frame/cookie、item7 0x4A8040 全文、item8 caller 9 件 ctx、item9 census、item12 初走査、IAT 名手動パース |
| `v173r_7.py` | item6 コピーループ全文（NUL 終端証明）＋0x45F3xx、item8 decompiled call site、item12 esi/global |
| `v173r_8.py` | item10 FUN_00572850 本文＋'FloatingText'、item11 callers 34、item12 全関数 442 命令走査、item13 初走査（.rdata のみ＝0 件） |
| `v173r_9.py` | item6 呼出時期（callers FUN_00405090=6）、item11 decompiled 全 call site、item12 callers 4 件、item13 **.data 範囲修正**で TD 1,402/欠落 373 |
| `v173r_10.py` | item11 精密デコード（array 索引方式の確定）、item12 FUN_006ECC80=`[ecx+0x40]` getter、item10 ラベル付与帯、[0xB333A0] refs 557 |

### 9-2 主要実測値（抜粋）

- item2: imm32 0x7385F0 = 1 件（0xA80D90 = NiShader vtable 0xA80D8C slot1）
- item3: ret 9 本・最終 ret **0x4062F2**、jumptable 5 エントリ（0x4061EF/130/210/1C1/224）、byte table 0x40630C=70B
- item4: refs 70・真書手 16＋clear 5、object+4=u16 index・+0x18=byte フラグ、0x7B4290 系 28 スロット表
- item5: 書手 = FUN_007B8990 の `FUN_00701FC0(NiPixelData@0x70E560→0xA7E624)`、ctor 0x701CD0 → **0xA7D8CC = NiSourceTexture**、Interlocked obj+4
- item6: 0x53AC60 frame 0x108/cookie、0x53AD60 frame 0x148/cookie、callers(0x53AC60)=0x40E759（head 後）、callers(FUN_00498340)=0x40530E、FUN_00405090 callers=6（全て FUN_00405CE0 帯）
- item7: 0x4A8040 = lookup 0x55E000 → miss で skip・insert 0 件
- item8: callers(0x4A7980)=9、IAT 0xA280E4=GetPrivateProfileIntA / 0xA280B0=WritePrivateProfileStringA / 0xA28078=InterlockedIncrement / 0xA2807C=InterlockedDecrement
- item9: abs32=1（0xA6A000）・E8=0・E9=0・vtable/opcode 表 0
- item10: 'FloatingText' 0xA68834 ×2、0xE4 alloc ×2、singleton 0xB3A6A4（0x15F0/ctor 0x571E80）
- item11: 34 callers、+0x10=素配列 `[base+idx*4]`、+0x14=u16、+0x1C=next、解決系 0x4806E0/0x8AFCE0/0x446A10
- item12: this=[0xB333A0]（557 refs）、arg2=[obj+0x40]（getter 0x6ECC80）、0x667BE0 refs=1（0xA73B58）
- item13: TD 1,402 / 欠落 373（テンプレート 311 / 非テンプレート 62）

（了）
