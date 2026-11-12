# v192_havok_magic_residuals.md — havok/magic/pathing 残点束（v186 R1-R10 / v188 R-1..R-7）

- 作成日: 2026-10-29 / 担当: TODO serial 180 (V180) — **親セッションによる直接解析**（サブエージェントのサーバ再起動による中断を受け、inline 継続）
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / 既存 tmp/re / TODO_re49.md / GT / 台帳 / git / Supermemory / sync は一切変更していない（読取のみ）。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam, ImageBase 0x400000, 7,898,624 B)
- 出典（読取専用）: `tmp/re/v186_havok_ray_pathing.md` 残点 R1-R10 / `tmp/re/v188_magic_wave3_residuals.md` 残点 R-1..R-7 / `GT_CHEATSHEET.md` / `cache/*` / `docs/Oblivion_decompiled.c`
- 中間スクリプト: `%TEMP%\opencode\v192b_1..7_*.py`（7 本）+ 出力 `v192b_1..7_out.txt`
- 注意: **v188 R-6 の訂正バッチ4件は V168 向け**。本タスクでは**ドラフト記載のみ・適用しない**（docs/TODO 無編集）。
- 凡例: [確定] = 逆アセンブル/データ直接確認 / [部分] = 構造は判明・一部未 / [推定] = 論理推定 / [残] = 未解明。

---

## 1. 手法 / 入力

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl。着手時・成果物完成直前の 2 回）。
- 解析: `tmp/re/re_util.py`（capstone 5.0.7 + pefile、`disasm/callers/find_dword/cstr`）＋ detail 付き Cs によるオペランド disp 走査（modrm disp 0x0C/0x24/0x2C/0x64 等の全ストア列挙）。
- 参照数は `tmp/re/cache/call_targets.json`、ラベル境界は `cache/named_funcs.txt`、クラス/型情報は `cache/vtable_index.tsv`（`n name mangled vtable slots off col typedesc` 形式、1,715 行）を引く（再走査しない）。
- decompiled は関数ヘッダ（`// FUN_xxx @ 0x...`）部分読取のみ（GT §2 規則）。
- 前回子セッションは §0/§1 とスケルトン（44 行）まで記入しサーバ再起動で中断 → 本セッションが中間スクリプト7本を追加して §2 以降を完成。

---

## 2. v186 残点 R1-R10

### R1: LOS(0x1A)/ViewCaster(0x1D) ピックの request 生成元と発射サイト — [残（否定証拠を拡大）]

- **`.text` 全体（detail 走査）で「即値 0x1A/0x1D を disp 0x24 へ store」する命令 = 0 件**（esp/レジスタ全ベース対象。v186 の C7/C6 スキャンとは独立に、種別値 0x1A/0x1D に絞って再確認）→ **種別は即値では書かれない**（計算値またはレジスタ経由）ことが確定。
- HUD/Activate 帯（0x59E000..0x602000）: imm 0x14..0x1D → +0x24 は **0 件**。カウンタ 0xBA792C(LOS)/0xBA7930(ViewCaster) の .text 参照は PickObject 系（0x88B850..0x88C600）外では **frame リセット 0x889A49/0x889A4E の 2 件のみ** → **この帯に request 構築は存在しない**（R3 の結論と共通）。
- 0x5F2AA5 の含属は **FUN_005f2820**（SEH 大関数、sub esp,0x198、caller 12 件 = 0x4F935D/0x4F93D1・0x5F6647・0x615965・0x619E09・0x621C0A・0x6240A7・0x62CCA9・0x62D11A・0x62D225・0x62D554・0x630F6B = 戦闘/AI 帯）。+0x24 書きは全てスタックローカル（0x5F286E/0x5F31A7/0x5F35D5）で種別フィールドではない。
- **次の手**: レジスタ経由ストア（`mov [reg+0x24], reg` 型）の builder 追跡、および FUN_005f2820 の request 構築ブロック（stack オブジェクトの +0x24 初期化）を decompiled で直接読むこと。

### R2: [hkpWorld+0x64] のクラス同定（castRay 仮想の受け手） — [確定(受け手経路) / 残(クラス名)] + **重要注意**

- **経路確定**: PickObject（FUN_0088b850）内で `mov edx,[ecx]; mov eax,[edx+0x58]; call eax` → bhkWorld vtable 0xA95CF4 **slot[22] = 0x452A60 = `mov eax,[ecx+8]; ret`（実測）** → **`mov edi,eax`（0x88B976）＝edi = [bhkWorld+8] のオブジェクト W**。
- **受け手確定**: `0x8987E0/0x898ED0` は ecx=W で呼ばれ、W+0x64 / W+0x78 を引数化。本発射は **0x8D95A0 内 0x8D9667 `mov eax,[ebp+8]`（= W+0x64 の値）→ `mov edx,[eax]` → `call dword ptr [edx+0x38]`** = **castRay 仮想の receiver は「W+0x64 に格納されたオブジェクト O」** [確定]。
- **クラス名は未特定**: `vtable_index.tsv` に `hkpWorld*` 名の行は **0 件** → O および W は **non-RTTI 系候補**（b13 の 142 ブロックと突合すべき対象）。W+0x64 の代入書手（W の ctor でゼロ初期化された後の実代入）は未特定。候補帯: 0x890000-0x8A0000 の `[reg+0x64]` 書手（0x89A6E4 FUN_0089A230 / 0x89AF0F FUN_0089AD80 等、vptr 0xA96D6C クラス）。
- **重要注意（フィールド意味のクラス依存）**: 未ラベル thunk **0x88FE70（`add dword [ecx+0x64],1`、`[esp+4]` の type&0x3F==0x14 時のみ）と 0x88FEB0（同条件で dec、jbe ガード）**は別オブジェクトの **+0x64 = type 0x14 ピック付随カウンタ**である。bhkWorld ctor が [esi+0x64]=0 でゼロ初期化（0x88AF04）し、[esi+0x61] フラグ（ctor の dword ゼロで覆盖）も併用 → **この thunk 群の ecx は bhkWorld（W ではない）**[推定]。**+0x64 はクラス毎に意味が異なる**（bhkWorld=カウンタ / W=オブジェクトポインタ）ため、以後の解析では混同禁止。

### R3: クロスヘア/Activate ピック — [確定(否定)]

- HUDReticle/Activate 帯（0x59E000..0x602000）に **pick 種別の store 0 件**（R1 と同一走査）＋ v186 済みの「`call [reg+0x88]` 全形式 0 件」→ **これらの帯にピック request 構築・発射は存在しない**。
- アクティベート対象選択は (a) path/LOS ピックの再利用（FUN_005f2820 等のゲーム側大関数経由）か (b) Gamebryo/レンダラ側ピック に分裂 [推定]。**未解**。

### R4: 0x898ED0 パスピック変種の全論理 — [確定]

`0x898ED0`（39 バイト、`ret 8`）は**薄いラッパ**:

```
スタック request 構築: [esp+0x14]=0xA96AD4（型タグ/定数。+8 以降は文字列
  "TtWatchDog:FreeMem" が隣接するため vtable ではない）, [esp+0x38]=0, [esp+0x3C]=0
引数並び（逆順 push）: param2, 0, [this+0x78], arg1, [this+0x64]
call 0x8D95A0   ; ret 0x14（callee が 5 引数破棄）
```

**`0x8D95A0` 本体（実処理）** [確定]:

1. **TLS プロファイルリング開始記録**: fs:[0x2C]+[0xBA9DE4] 経由のリングへ `{name=0xA9A25C, rdtsc}` を 12B で追記（[eax+0x1A4..0x1A8] バウンダリ付き）。
2. request フィールド充填: `[req+4] = arg1`（0x8D95FF）、`[req+0xC] = param2 系`（0x8D9602）、`[req+8] = (arg3 ? arg3+0x14 : 0)`（フィルタ）、`[req+0x34] = (arg1 の [+0x20] フラグ && arg3) ? arg3+0x10 : 0`。
3. `movaps xmm0,[eax]`（arg1 先頭 16B = ray from/to）→ `movaps [esp+0x1C], xmm0`、`[esp+0x2C]=1, [esp+0x34]=0x10`。
4. **発射**: `mov eax,[ebp+8]`（= **W+0x64 = O**）→ `mov edx,[eax]` → **`call dword ptr [edx+0x38]`**（this=O、スタック引数 = &request 系）。
5. **TLS リング終了記録**（name 0xA9610C、rdtsc）→ `ret 0x14`。

つまり **0x898ED0 =「W+0x64 の castRay 仮想を、フィルタ付き request で呼ぶ 1 発ラッパ + プロファイリング」**（種別は request 構築側で設定済みのものを使う）。

### R5: 回避 FSM 遷移関数（status 0..7 の書手） — [大部分確定 / 残(status 1,2)]

status byte（回避オブジェクト +0x0C）の**即値書手は 3 関数に集中**:

| 関所 | 書手（値 → 名前 0xB15728 表） |
|---|---|
| **FUN_00681050** | 0x681126 **7**(FAILED) / 0x68117B **5**(Waiting) / 0x68129D **5** / 0x6813BD **3**(Angling Left) / 0x6813D8 **4**(Angling Right) / 0x681406 **3** |
| **FUN_00681DF0** | 0x682093/0x68212A/0x682161 **0**(NO AVOIDANCE) / 0x682120/0x682196 **7** |
| ctor FUN_00680DC0 | 0x680DEC `[eax+0xC]=cl`（初期値は引数） |

- 例: 0x681120 `test al,al; jne; mov byte [esi+0xC],7; mov al,1`（失敗時 7）、0x6813BD 直後 `mov byte [esi+0xC],3` + fld シーケンス（Angling 突入）。
- **status 1(Checking)/2(Avoiding) の即値書手は 0x67C000..0x68C000 全域で 0 件**（C6 disp=0x0C imm=1/2 をバイト/ディスペイト両形式で走査）＋ 0x680DC0..0x68A100 のレジスタ源ストアも上記以外は vec4 コピーの誤検出と判明（0x684626/0x6846D9/0x685CAF/0x685E20 は vec 書込、0x6837F5 は FUN_006836E0 の別構造体初期化）。
- **残**: 1/2 の書手（レジスタ経由 `mov [esi+0xC], reg` か、FUN_00684530/0x6894xx 帯の未走査分か）。

### R6: Turning bit5 設置元 — [確定]

**FUN_00686150**（NavMesh 更新帯、0x686450 の手前）に一対で存在:

- **設置**: `0x6862AA  or byte ptr [ebp+0x2C], 0x20`
- **解除**: `0x686265  and byte ptr [ebp+0x2C], 0xDF`

- 補助: bit3 セッタ = 0x683A47/0x683A4E（or 8 / and 0xF7）、bit7 セッタ = 0x683A87（getter/setter 0x683A70/0x683A80）、0x6843A7 と 0x6894DD も `or [edi+0x2C],0x80`。Init クリア = 0x684F08 `mov byte [esi+0x2C],0`（FUN_00684EC0 内、v186 §2.1 と一致）。
- 0x67D000..0x685200 帯の `mov [reg+0x2C], reg/dword` の大半は [esp+0x2C] ローカルまたは回避オブジェクト等の別フィールド（ctor 表の +0x2C dword）で、PathingSystem フラグではない（誤検出を除外済み）。

### R7: 速度出力の書込本体 — [訂正 + 部分]

- **【v186 訂正】`0x68C0F0` は速度書込ではない**: 全読の結果、ノードリスト操作（引数 node との照合・`0x780D10` で next 走査・`0x60D0A0`+`0x401F20` で解放・`0x6A2FD0(last,0)` 呼び出し・`[ebp+0]=0 / [ebp+4]=新頭`）。= Init 経路 0x68C6E0 の「位置/状態リセット」の正体は **node list のクリア/カレント設定**（v186 §2.6 の「本体は 0x68C0F0」は誤り）。
- **`0x67EB60` 本体（先頭 80 命令実測）**: グローバル表 0xB15450/0xB15454/0xB15458 から 2 組の vec3 を構造へコピー → `0x67D7B0` → 引数 pos1/pos2（[esp+0xC]/[esp+0x10] 先頭 3 ワード）をコピー → **fucompp 3 軸等値比較**（pos1==pos2 ならスキップ）→ 不一致時 `0x67E160` → 真なら `0x67E8D0` → `ret 0x10`。= **移動構築の「from/to 設定＋不一致なら経路再構築」本体**。
- **PathingSystem[+0x3C..] の書手候補（非スタック・enclosing 付き）**:

| site | 関数 | 形 |
|---|---|---|
| 0x684F1F | FUN_00684EC0（Init） | `[esi+0x3C]=eax`(ゼロ化) |
| 0x682FE5 | **FUN_00682EF0** | `[esi+0x3C]=edi` |
| 0x688569 | **FUN_00688120** | `[esi+0x3C]=ecx` |
| 0x68CDDD / 0x68CE34 | **FUN_0068CD90** | `[esi+0x3C]=ebx` |
| 0x68CF75 / 0x68D36C | FUN_0068CEA0 / 0x68D3xx | `=0`（ゼロ化） |

**残**: 上記のうち「esi が PathingSystem 本体」であることの実測裏付け（各関数のレシーバ同定）と、実際に速度 XYZ を書く最終 store の確定。

### R8: velocity → actor 配線（0x5FAAE0 前段） — [確定(1点) / 残(配線)]

- **`0x5FAAE0` は E8 呼出し 0 件の理由が判明: vtable 経由**。abs32 参照 4 件 = **0xA6E3DC / 0xA70004 / 0xA7145C / 0xA73D74 = Actor / Character / Creature / PlayerCharacter の各 vtable slot[218]**（vt=0xA6E074/0xA6FC9C/0xA710F4/0xA73A0C、n=239、cache vtable_index 実測）→ **0x5FAAE0 = Actor 族 共通 vfunc slot[218]** [確定]。本体は `[esi+0xBC]` との float クランプ演算（此のフィールドが速度上限系）。
- **残**: PathingSystem[+0x3C..]（速度出力）→ slot[218] 本体への直接配線は未同定。0x5FA000..0x614000 帯の `[reg+0x3C]` 読みは 178 件（うち非スタック: FUN_005fa320 の 0x5FA665/0x5FA87A、FUN_005fcab0 の `lea ecx,[ebx+0x3C]` 0x5FE963/0x5FEAD1、FUN_006005f0 の 0x600CD2 等）で、いずれも PathingSystem と未突合。

### R9: NAVM ESM ローダ — [進展: 「NAVM」は存在せず PathGrid に収束]

- **`b'NAVM'` 文字列 0 件 / u32 0x4D56414E 参照 0 件**（全セクション）→ 本 EXE に NAVM レコード型は存在しない（Oblivion は **PathGrid/PGRD** 系）。
- **PathGrid 文字列と参照**: `'PathGrid not associated with cell.'`(0xA384A0) → ref **0x44E2B0 in FUN_0044DCF0**（`push 0xA384A0; call 0x4A7A60` ロガー）。`'PathGrid (%08X) in cell %s found different connection data...'`(0xA47720) → ref **0x4E6BAC in FUN_004E65B0** = SEH 大関数（sub esp,0x13C、`cmp word [esi+0x30],0`、`mov eax,[esi+0x24]`）＝ **PathGrid ロード/検証本体の筆頭候補** [推定]。
- **候補 2 件の裁定**: `0x4EF7E0` = 所有チェーン walker（`[ecx+0x7C]` を末尾まで辿って `lea eax,[ecx+0x38]; ret`、21 呼出し）で**ローダではない**。`0x4EB0E0` = `[0xB3608f]` ラッチ＋ NavMeshManager 0x43F270 呼出ヘルパで**ローダではない**。
- **残**: FUN_004E65B0 を「PathGrid ESM ローダ本体」としての確定（レコード読取ループ・入出力構造の解析）。

### R10: bDisableAvoidance の消費経路 — [半解: 登録/解放は確定・値読手は未]

登録・解放系のみ検出（0xB15748 の abs32 参照は全体で 3 件）:

1. **登録**: `0x9FE6F0: push 0xB15748; mov ecx,0xB07BF0; call 0x4040E0` — bHavokPick（0xB05204→0xB07BF0）と**同一 INI レジストリ** [確定]。
2. **クリーンアップ thunk**: **`0x9C4CB0: mov ecx,0xB15748; jmp 0x403BC0`**（0x403BC0 = `[node+4]` の文字列先頭が 'S' なら `0x401F00` で free、という値解放ヘルパ）。参照 **0xAED584**（.rdata テーブル = atexit 系）[確定]。
3. **第2登録 thunk**: **`0xA25DD0: push 0xB15748; mov ecx,0xB07CFC; call ...`**（別レジストリ 0xB07CFC）、参照 **0x9FE708**（登録関数内の push）[確定]。

- **値読取（[0xB15748] の value → 消費先）は .text に 0 件** → 読取はレジストリ（0xB07BF0/0xB07CFC）経由の間接か、未検出。bHavokPick の先例（兄弟設定は 0x441Axx 帯でグローバルへコピー）に比べ、本設定の対応コピー先が見つからない点が残。

---

## 3. v188 残点 R-1..R-7

### R-1: 毒エントリ flags 0xE（0x69BA90 内）の読み取り側意味 — [部分: 設置サイト確定]

- **設置サイト**: **FUN_0069BA90 内 0x69BB6C `or dword ptr [esi+0x14], 0xE`** [確定]。
  - esi = 直前の **vcall `call [ebx]+0x40`**（引数 edi, ebp, 0、ecx=ebx）の戻り値（= 毒エントリ取得/生成）。
  - ガード: `cmp edi, [esp+0x1C]; je` ならスキップ（**キー不一致時のみ 0xE を立てる**）。直後に `0x68E170(entry, float[esp+0x14])`。
- **読み手**: 0x698000..0x6A2000 に `and/test ..., 0xE`（disp 0x14 付き）は **0 件** → 読手は (a) 他帯、(b) bit 単位（test 2/4/8）での読み、のいずれか [残]。
- 意味推定: 0xE = bit1|2|3（magnitude/duration/speed 系の「未適用/要伝播」ビット群）だが、読手特定まで [推定]。

### R-2: FUN_0060C940（0x60CA8A）AI 側毒消費の条件詳細 — [部分]

実測（0x60CA60..0x60CAE3）:

1. `call 0x484F80` → fadd → `[esi+0x70]` へ蓄積（float 残量？）。
2. **`call 0x484DF0` → 結果を `[esi+0x84]` に保存**。
3. **`test eax,eax; je 0x60CA8F; mov ecx,edi; call 0x484E50`** — **0x484DF0 の戻りが非0 のときのみ 0x484E50（消費操作）を実行**（0x60CA8A）[確定]。
4. 以降: `push 0xB036FC; push 0xB03248; call 0x9832E6`（RTTI キャスト）→ `[eax+0x9C]&1` を `[esi+0x96]` へ、`[eax+0x64]` をローカルへ等。
- **残**: 0x484DF0 / 0x484E50 の実体（ActiveEffect リストの何を問う/除くか）の命名。

### R-3: SEFF ハンドラ RTTI 名 + Finish 条件 — [前半: 確定]

- **TD 0xAC5430 = `.?AVScriptEffect@@`** [確定]（cache/vtable_index.tsv row 1214: `ScriptEffect / .?AVScriptEffect@@ / vtable 0x00A76AAC / 16 slots / typedesc 0x00AC5430`。refs to 0xAC5430 = 0xA76AA8 = vtable 直前の COL、実測一致）→ SEFF ハンドラクラス名 = **ScriptEffect**。
- **残**: Finish 実行条件の `[0xB333B00+0x18]` **bit11/bit1** の意味は未解析。

### R-4: 調合の効果選択/合成ロジックと FUN_00414EB0 の毒判定基準 — [構造確定 / 基準本体は残]

- 0x59489C = **`call 0x414EB0`（毒判定ヘルパ） inside FUN_00593CD0**（callers 0x594EDF/0x595055 = 調合/メニュー帯）[確定]。周辺: `[ebp+0x7C]`（第二素材?）→ `0x46E3F0` で属性取得 → fild → `[esp+0x20]` に 0/1 フラグ（0x414EB0 が真なら 1）。
- **FUN_00414EB0 全読** [確定（構造）]: リスト走査（next=[esi+8]、距離 -4 歩行）で各要素ごとに —
  1. `[esi+4]` 非0 のとき `ecx=[ecx+0x1C]; eax=[ecx+0x58]; shr edx,0x16; test dl,1` → **bit22(0x400000) が立っていれば素通し**（チェック回避）。
  2. 非立なら **`call 0x413470` が偽なら即 return 0**。
  3. 全通過で **return 1**。
- **残**: 実判定は **0x413470** の中身（毒 base かどうか等）。apparatus 品質による成功/失敗・倍率は未解析。

### R-5: charge 設定値 [menu+0x7C] と FUN_00438DB0 — [部分]

- **`FUN_00438DB0 = mov eax,[ecx+0x18]; ret`（4バイト getter）** [確定] — 「ソウルジェム chg 判定」は **+0x18 フィールドの読手**で、呼出しは **30 件**: 0x593751/0x593760（アルケミア帯）、0x5A213C/0x5A2D11/0x5A2E7D/0x5A31B7（エンチャント作成帯）、0x5C30F5、**0x5D4F24/0x5D4F34/0x5D4FA9・0x5D7CFC/DD2・0x5D8160/8695・0x5DD1DD（チャージ帯）**、0x9603xx/0x9612xx/0x962Exx/0x9647xx/0x964Axx/0x96D5xx（UI 帯）。
- **FUN_005D4EE0 = チャージ計算本体の筆頭候補** [推定]: `[esi+0x2c]==0` ゲート → `[esi+0x74]` へ `0x438DB0` → **`cmp byte ptr [eax],0`** → 0x5D51BB へ分岐（値の先頭バイト判定）。[menu+0x7C] の算出式は本関数内の未読部分にあり **未解析 [残]**。

### R-6: 訂正バッチ候補（**V168 向けドラフト・本タスクでは適用しない**） — [ドラフト]

v188 §5 の記載をそのまま転記（反映可否はユーザー承認＝V168 で裁定）:

1. s98 §2「0x6A83A0/0x6A83D0=SEFF tick」→ **誤り**（正: v188 §1.1）
2. s96「0x69AC50=ActiveEffect 生成」→ **実体=面範囲ターゲット収集**（v180 既指出を本調査でデコンパイル確認）
3. s92「vt+0x38=毎フレーム」→ **実体=開始フック**、毎フレームは vt+0x08
4. W3-3 の TODO 文言「0x6A83D0 以降」自体の更新

### R-7: FUN_00447DF0 の呼び出し元同定 — [確定 + v188 訂正]

- **callers(0x447DF0) = 9 件**: 0x44E55F / 0x44E581 / 0x44E5C0（**FUN_0044DCF0**）、0x46C38E（FUN_0046C360）、0x48DB16 / 0x48DD71（FUN_0048DA00）、**0x5115B0（FUN_00511570）**、**0x5A2E4B（FUN_005A2D30 = エンチャント帯）**、**0x5D4F4D（FUN_005D4EE0 = チャージ帯）** [確定]。
- **【v188 訂正】「呼び出し元 0x5FB58C」は誤り**: 0x5FB58C の実呼叫は **`call 0x5D5200`**（0x5FB58B `push edi` 直後）。0x447DF0 ではない。
- script/console 経路の候補は **FUN_00511570**（0x511570 帯）[推定]。

---

## 4. 結論（3行）

1. **pick 経路の残2点が接続**: R4 = 0x898ED0/0x8D95A0 が「W+0x64 の castRay 仮想（`[edx+0x38]`）をフィルタ付き request で呼ぶラッパ＋TLS プロファイル」として全解、R2 = castRay 受け手が **[W+0x64]（W=[bhkWorld+8]）** まで確定。ただし **W/O のクラス名は non-RTTI 系として vtable_index に存在せず（hkpWorld 行 0 件）**、+0x64 はクラス毎に意味が違う（bhkWorld=type0x14 カウンタ vs W=オブジェクト ptr）点が最大の罠。
2. **N7 の残点が3件確定**: Turning bit5 は **FUN_00686150 の 0x6862AA(or 0x20)/0x686265(and 0xDF)** に一対、回避 status 書手は **FUN_00681050（3/4/5/7）と FUN_00681DF0（0/7）＋ctor 引数** に集中（1/2 の即値書手のみ未）、**0x68C0F0 は速度書でなく node リスト操作**（v186 訂正）＝速度書手は FUN_00682EF0/00688120/0068CD90 等の候補群に収束。
3. **magic 束は「NAVM は存在しない（=PathGrid）」「SEFF ハンドラ RTTI=ScriptEffect」「FUN_00438DB0=+0x18 getter・FUN_00447DF0 の実 callers 9 件（0x5FB58C 訂正）」まで確定**し、毒 flags 0xE の設置サイト（FUN_0069BA90）と毒判定チェッカ（FUN_00414EB0→0x413470 残）の構造を解明。R-6 の訂正4件は V168 向けドラフトに保全。

## 5. 残留不明点

1. R1: LOS/ViewCaster request の種別書手（レジスタ経由）と FUN_005f2820 の request 構築ブロック。
2. R2: W と O（[W+0x64] の pointee）のクラス名（b13 non-RTTI ブロック突合へ）と W+0x64 の代入書手。
3. R5: status 1(Checking)/2(Avoiding) の書手。
4. R7: 各 `[reg+0x3C]` 書手候補のレシーバ同定と速度 XYZ 最終 store。
5. R8: PathingSystem 速度 → Actor slot[218] の直接配線。
6. R9: FUN_004E65B0 を PathGrid ESM ローダ本体としての確定。
7. R10: bDisableAvoidance の値読手（レジストリ経由か、未検出か）。
8. R-1: 毒 flags 0xE の読手（他帯 or bit 単位読み）。
9. R-2: 0x484DF0/0x484E50 の実体命名。
10. R-3: [0xB333B00+0x18] bit11/bit1 の意味。
11. R-4: 0x413470（毒判定基準本体）・apparatus 品質/倍率。
12. R-5: FUN_005D4EE0 内の [menu+0x7C] 算出式。

## 6. 自己検証

- **selftest 4/4 PASS**（着手時・完成直前の 2 回、同一結果: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 内部整合（本タスクの独立実測と既存成果物の一致）:
  1. bhkWorld vtable 0xA95CF4 **slot[22] = 0x452A60**（生バイト実測）= v186 §1.4-3 の記載と一致。
  2. 0x452A60 本文 `mov eax,[ecx+8]; ret` = v186 の「hkpWorld は bhkWorld+8」記載と一致。
  3. status 書手の値集合 {0,3,4,5,7} ⊆ 0xB15728 表の 8 状態、书き込み関数 2+c'tor の 3 重構造。
  4. Turning bit5 の or/and 即値 0x20/0xDF が同一関数 FUN_00686150 内に一対（解除ペアの整合）。
  5. vtable_index 実測: Actor/Character/Creature/PlayerCharacter はいずれも n=239 で slot[218] が 0x5FAAE0（4/4 一致）、TD 0xAC5430 ↔ `.?AVScriptEffect@@` row 1214 と refs 0xA76AA8 が一致。
  6. 走査の網羅性: disp 0x24（全 .text、即値 0x1A/0x1D）= 0 件 / disp 0x0C imm 1,2（0x67C000..0x68C000）= 0 件 / 0xB15748 refs = 3 件 / b'NAVM' = 0 件・u32 0x4D56414E = 0 件。
  7. callers 一致: 0x8D95A0=1（0x898F00 のみ）・0x447DF0=9・0x438DB0=30・0x5FAAE0=0（E8）+ abs32 vtable 4 件 — いずれもキャッシュ/独立走査と矛盾なし。
- 制約遵守: 新規ファイルは本 1 件のみ。docs/・TODO_re49.md・GT・台帳・git・Supermemory・sync は**編集 0 件**（他セッション保留依頼を遵守）。既存 tmp/re 成果物の上書き 0 件。ESM 読取 0 回（再スキャン禁止）。中間物は `%TEMP%\opencode/v192b_1..7_*` のみ。

## 7. Android 移植への示唆（実装しない）

- **回避 FSM**: status 書手は FUN_00681050/FUN_00681DF0 の遷移表をそのまま移植し、Turning は PathingSystem[+0x2C] bit5 を FUN_00686150 相当（or 0x20/and 0xDF の一対）として set/clear を1箇所に集約する。status 1/2 の入口だけは実機観測で補完すること。
- **速度出力**: 0x68C0F0 は「node リスト操作」として切り離し、速度書き出しは 0x67EB60 の from/to 等値短絡（pos1==pos2 なら再構築スキップ）と FUN_00682EF0 帯の書き出しを一セットとして再現する。
- **毒オブジェクト**: flags 0xE（bit1..3）は「未適用フラグ群」として entry+0x14 に or で立てる設計を再現し、適用時にクリアする 1 対を実装する（読手は移植時に bitrate 単位で確認）。
- **NAVM は存在しない**: 移植側のレコード処理は **PathGrid（PGRD）** を正とし、NAVM 想定コードを書かないこと。
- **RTTI**: SEFF ハンドラは `ScriptEffect`、pick 系 W/O は non-RTTI で型情報が取れないため、dynamic_cast 相当のフォールバックは型名ではなく vtable アドレスで分岐する。

---

## 8. 追補: サブエージェント独立再検証（2026-10-29・本節のみ追記・上記本文は無編集）

> 親セッションが inline 継続で本文（§2〜§7）を完成させた後、分析専任サブエージェントが **別途 capstone バイト走査（%TEMP%\opencode/v192_b1..b38_*.py）で独立再測定**した。既存記載との**数値一致**と、残留不明点 2 件を前進させる**追加証拠**を記録する（本文の値は変更していない）。

### 8-1. 本文主要値との独立一致（クロスバリデーション）

| 項目 | 本文記載 | サブ独立実測 | 一致 |
|---|---|---|---|
| R2 受け手経路 | `call [edx+0x38]` receiver = [W+0x64] | 0x8BA1B0/0x8D95A0 両方で `mov eax,[ebp+8]→mov edx,[eax]→call [edx+0x38]`、0x8987E0 decompile が `FUN_008ba1b0(*(param_1+100),...,*(param_1+0x78),...)`（+100=+0x64）で receiver=[W+0x64] を**別ルートで確認** | 一致 |
| R4 実体 | 0x898ED0=薄ラッパ→0x8D95A0 | 0x8D95A0 本文・プロファイル名 **"TtRayCstCchSim"**（0x8BA1B0 は "TtRayCstCached"）を decompile で確認。callers(0x8D95A0)=1（0x898F00 のみ） | 一致 |
| R5 status 書手 | 0x681050={3,4,5,7} / 0x681DF0={0,7} / ctor 引数 | disp 0x0C 全形式走査（C6 即値・88 レジスタ源・rm=0..7 全含む）で **同一批**。status 1/2/6 は 0x67C000..0x6A2000 で書手 0 件 | 一致 |
| R6 Turning bit5 | 0x6862AA(or 0x20)/0x686265(and 0xDF) | 全 .text で `or byte [r+0x2C],imm&0x20` = **0x6862AA の 1 件のみ**、`and [r+0x2C],0xDF` = **0x686265 の 1 件のみ**（fn 帯 0x686150）。`or al,0x20`/`08 /r`/`66 81 or word` 等の別エンコードも 0 件で**排他的であることを証明** | 一致（強化） |
| R1 即値種別 | disp 0x24 へ 0x1A/0x1D 即値ストア 0 件 | C6/C7 disp8・SIB 除く rm 全対象で 0 件、**レジスタ源+近傍即値（B0/B8/6A）複合でも 0x1A/0x1D は 0 件**（0x15 の 3 件は 0x7B/0x7C/0x7E 帯の formType で偽陽性） | 一致（強化） |
| R10 値読手 | 値読取 0 件 | 0xB15748 refs=3 件（登録 0x9FE6F0 / cleanup thunk 0x9C4CB0 / 別登録 thunk 0xA25DD0）＋ **名称文字列 0xA74B2C refs = 0xB1574C（ノード自身の name フィールド）のみ** → 名前経由の間接読手も不在を追証 | 一致（強化） |

### 8-2. 残留不明点 #4（R7 レシーバ同定）を解決 [確定]

本文 R7 の残「esi が PathingSystem 本体であることの実測裏付けと最終 store」に対し:

- **FUN_00688120（size 0xCA0、E8 callers 0 = vtable/コールバック経由）が PathingSystem メソッドであることを実測確定**: 内部 0x688579 `lea ebx,[esi+0x14]`（= PathingSystem[+0x14] ノードリスト容器）と 0x68857E `call 0x68C170` / 0x688585 `call 0x42B410`（v186 §2.2 の Update と同一アクセサ）を使用 → **esi = PathingSystem** [確定]。
- **[+0x3C/40/44] の書手 0x688569..75 は「速度」ではなく「現在ノード位置」の複製**: 0x688552 `call 0x68CA20`（bool ゲート）が真のとき 0x688562 `call 0x6899C0`（= v186 §2.3 のノード位置 getter、戻り=pos ポインタ）→ `mov ecx,[eax];[esi+0x3C]=ecx; mov edx,[eax+4];[esi+0x40]=edx; mov eax,[eax+8];[esi+0x44]=eax` = **3 ワード位置コピー** [確定]。
- **帰結**: s72 M7 の「PathingSystem[+0x3C/40/44]=速度出力」は **誤帰属**であり、正しくは **「追従中ノードの位置 XYZ」**。Init 0x684F1F-2D・0x68CD90・0x682EF0・0x68FC60 はこの位置フィールドのゼロ化。**速度本体は 0x67EB60 帯が構築する move オブジェクト（ctor 0x67D300/0x67D3A0、vtable 0xA747BC、その own [+0x3C..+0x48] をゼロ/設定）**にあり [推定]、R8 の actor 配線はこの move オブジェクト経由と推定される。
- 補助: 本文 R8 の 0x6849F0 は **0x688120 内 0x688C66 から唯一呼出**され、`[ecx+0x3C]` を **位置ベクトルとして &this[+0x3C] を 0x4F1A80（NiNode 位置照合）へ渡す**＝速度消費ではなく場面ノード関連付け [確定]。

### 8-3. 残留不明点 #2（R2 クラス名）への追証 [推定]

- W+0x64 の代入書手候補として本文が挙げた帯 0x890000-0x8A0000 の `[reg+0x64]` 書手を全走査し（10 件）、**FUN_0089A230（size 0xB50、vtable 0xA96D6C）**に着目。本関数は 0x889A70（bhkWorld 帯の attach）から呼ばれ、**0x89A6E4 で IAT 0xB2FC80（Havok アロケータ）呼び出しの戻りを `[esi+0x64]` へ格納**（サイズ引数は [edi+0x64] 由来）＝ **+0x64 へサブオブジェクトを割当てる実注入点** [確定]。
- vtable 0xA96D6C の直後には **"broadphase" / "collectionFilter" / "StepUpStateOverlaps" / "StateAddAggs" / "LtUpStateFilterOnPhantoms"** のプロファイル名群が埋め込みで隣接 [確定] → **FUN_0089A230 は hkpWorld の broadphase/フィルタ複合オブジェクトの ctor** であり、**O = hkpBroadPhase 系（W は hkpWorld）**が最有力 [推定]。
- 本文の注意「+0x64 はクラス毎に意味が異なる（bhkWorld=type0x14 カウンタ vs W=オブジェクト ptr）」は追補でも維持: 0x89A230 の +0x64 は broadphase サブオブジェクトであり、bhkWorld の +0x64（0x88AF04 でゼロ）とは別物。混同しないこと。

### 8-4. 追補の自己検証

- `probe_harness.py selftest` = **4/4 PASS**（本追補作成前）。走査は capstone 5.0.7 + pefile、VA→off = `pe.get_offset_from_rva(VA-0x400000)`。
- 追補スクリプト: `%TEMP%\opencode/v192_b1..b38_*.py`（既存 `v192b_1..7` 系とは別系統）。docs/TODO/GT/台帳/git/supermemory/sync 編集 0 件、既存 tmp/re 上書き 0 件、ESM 読取 0 回。
- 本節は**追記のみ**。§2〜§7 の親セッション記載には手を加えていない。
