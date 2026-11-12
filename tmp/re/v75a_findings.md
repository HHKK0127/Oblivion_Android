# v75a Findings — Oblivion.exe (v1.2.0416 Steam) RE 台帳

Harness self-test: **PASS** — `ctx.rd(0x504050,8) = 51 8b 4c 24 18 8b 54 24`（期待値 `518b4c24188b5424` と一致）。
手法: すべての EXE 読み取りは `probe_harness.Ctx`。全 .text ディスセンは capstone 5.0.7（x86-32、`skipdata=True`、`detail=True`、2,257,987 命令、`text_end = 0xA27C39` まで到達を検証済み）。decompiled.c は行単位（`enumerate(f,1)`）で検索。名前は annotate_db.json（368 件）。
範囲: 調査のみ。他ファイルは一切作成・編集せず、git もなし。

---

## N12 — displacement 0x284 / 0x288 の使用数（測測法の論争）

### 判定
争われている 132/39 も 124/37 も、いかなる .text 全体測測法でも再現できない（どちらも過小計: 0x284 は約 2.6 倍、0x288 は約 3.2 倍の過少）。妥当な object/vtable displacement 使用数は **A = 344（0x284）/ 126（0x288）**。stack ローカル汚染（B）は実在するが 38/25 で、争われている 2 対の差（8/2）より遥かに大きく、2 つの争われた数値は stack 含む/抜きの差ではない。

### 根拠
.text 全体スキャン（capstone `skipdata=True`、2,257,987 命令、最終 VA == text_end）:

| disp | A_object（基底が esp/ebp 以外） | B_stack（esp/ebp 基底） | C_abs/imm | D_call/jmp [mem] | 合計 |
|------|--------------------------------|------------------------|-----------|------------------|------|
| 0x284 | **344**（load 332, store 8, lea 4） | 38（load 12, store 11, lea 15） | 0 | **0** | 382 |
| 0x288 | **126**（load 115, store 11, lea 0） | 25（load 8, store 9, lea 8） | 0 | **0** | 151 |

- `call [mem+0x284]` / `call [mem+0x288]` = **0 件**（D カテゴリ空）— 既往の「call [mem] = 0」を確認。object load はすべて `mov reg,[reg+disp]`（vtable スロット読み → `call reg`）で、メモリ間接呼び出しではない。
- SIB/インデックス形式なし: 全 object site は素の `[reg+disp]`。基底分布 0x284: eax=174, edx=149, esi=11, ebx=5, ecx=3, edi=2／0x288: edx=54, eax=46, esi=17, ebx=5, ecx=3, edi=1。
- vtable dispatch に接続する object load（`mov reg,[obj+disp]` → 8 命令以内に `call reg`）: **323（0x284）/ 103（0x288）**。
- object site を含む一意関数: **152（0x284）/ 63（0x288）**。
- decompiled でも vtable 形式を確認: `(**(code**)(…+0x284))(` 306 行、0x288 は 98 行。field アクセス行 19/23。受信式の一意数 32/19（上位: `*param_1`、`*DAT_00b333c4`）。

### 手法
- カテゴリ分類は CS_OP_MEM 判定 + disp32 尾判定 + 基底レジスタ（esp/ebp 有無）で A/B を分離、即時・絶対アドレスを C、`call/jmp` の演算子種別を D として集計。
- 争われた数値の再現試行として (1) object 専用（stack 除外）、(2) stack 含む、(3) 単一 VA での打ち切り、(4) no-`skipdata` シンプルディスセン、(5) 基底レジスタ別、(6) 命令種別別、(7) per-func / per-dispatch の各手法を実測。

### 残課題
- 132/39 と 124/37 の正確な出所は未特定（いずれも部分的・狭いスキャンの産物と推定）。打ち切り仮説は棄却: 累積 object 数は 124 @ 0x5E22ED、132 @ 0x5E39B7、37 @ 0x5EE22F、39 @ 0x5EE27D と交差し、単一の打ち切り VA で (132,39) も (124,37) も自己整合しない。no-`skipdata` は 0x428844 で停止（2.5% カバレッジ、10/1 件）。最も近いのは 0x288 の per-func=38（対 39/37）だが、同じ手法で 0x284 は 152（対 132/124）となり内部不整合。— 妥当な object/vtable 図は A=344/126。

---

## N13 — 0x6744B0 / 0x67B300 の境界（OnHit「トリガー vs スタブ」）

### 判定
0x6744B0 と 0x67B300 はどちらも**関数エントリではない**（ともに E8 直接呼び出し 0 件、いずれも関数途中）。直近の実エントリは **FUN_00674200**（毎フレーム update/tick、約 752 B、3 呼び出し、内部に 0x6744B1 の `call 0x67B320`）と **FUN_0067b320**（連結リスト登録解除/削除、約 502 B、7 呼び出し）。**どちらも OnHit トリガーではない** — OnHit ラベルは誤帰属。

### 根拠
int3 パディング + `ret` による境界スキャン:

- **0x6744B0** = `mov ecx,ebx`（0x6744AF の `8B CB`）の 2 バイト目。0x6744B0 からデコードすると `CB` = `retf`（1 バイト非整列、既往記載どおり）。0x6744B1 の実命令は `E8 6A 6E 00 00` = `call 0x67B320`。
- 0x6744B1 を含む関数: **エントリ 0x674200**（int3 run 0x6741F1–0x6741FF の後、プロローグ `sub esp,0x10; push ebx/ebp/esi; mov ebx,ecx`）、**終端 0x6744EF**（`C2 08 00` = `ret 8`）、大きさ ≈ 0x2F0 = 752 B。dec_index `fun_00674200`（decompiled L435863）。**E8 呼び出し 3 件**: 0x65F809（fun_0065f770）、0x66F8EA（fun_0066f420）、0x678655（fun_00678510）。
- **0x67B300** = 別関数の末尾側（その `ret` = `C2 10 00` @ 0x67B30C、終端 0x67B30E）。その関数: **エントリ 0x67B260**（int3 run 0x67B253–0x67B25F の後）、E8 呼び出し 4 件（0x673add, 0x673dad, 0x6740f3, 0x674427）。dec_index `fun_0067b260`（L441598）。
- **0x67B320 = 実エントリ**（int3 run 0x67B316–0x67B31F の後）。プロローグ `push ebp/esi/edi; mov edi,ecx`（thiscall）。**FUN_0067b320**、dec_index `fun_0067b320`（L441645）、終端 ~0x67B515、大きさ ≈ 0x1F6 = 502 B。**E8 呼び出し 7 件**: 0x673d9f, 0x673e61, 0x6740e5, 0x6741cb, 0x674419, **0x6744b1**, 0x6753d6。（次エントリ 0x67B520 = `fun_0067b520`, L441793。）
- annotate_db に 0x6744B0/0x674200/0x67B300/0x67B320/0x67B260 の名前は**なし**。

### 手法
トリガー vs スタブの分類は decompiled の副作用で判定: FUN_0067b320（L441646）は連結リスト削除（ノード `[0]`=ID、`[1]`=next、`[2]`=tail、番兵 `DAT_00b3bd68`、`DAT_00b3bd74/0xb3bd78` クリア、`FUN_00401f20`=free、`FUN_0065c620`=notify）。FUN_00674200（L435864、`void(float)`=デルタ時間）は毎フレーム走査（`FUN_007616d0` でヘッド取得、フラグ `>>0x15`/`>>0xb`/`>>0x5` テスト、vtable 0x190/0x148/0x154/0x1c0/0x168/0x164 呼出、終了時 `FUN_0067b320(piVar5,0)`+`FUN_0067b260(...)` で登録解除）。どちらも effect 適用・ActiveEffect フィールド書込なし → オブジェクトリスト保守であり OnHit 経路ではない。

### 残課題
- FUN_00674200 が tick するオブジェクトリストのシステム名（どのサブシステムか）は未特定。ただしこの領域は OnHit 効果適用パスではないという点は確定。

---

## N15 — 役割の切り分け: FUN_00668440 vs FUN_00522760

### 判定
「レベルアップ」ラベルは **0x668440** に属する（FUN_00668440 = レベルアップ時の単一属性再計算）。0x522760（FUN_00522760）はレベルアップと**無関係**な小さな get/set トグル補助。

### 根拠
- **FUN_00668440(uint param_1)**（decompiled L428841、dec_index `fun_00668440`、annotate_db 名なし）:
  ```c
  void FUN_00668440(uint param_1) {
    if (param_1 < 8) {                        // param_1 = 属性 index (0..7)
      iVar1 = FUN_005f1910(param_1);           // base
      iVar3 = FUN_005480a0(FUN_00664970(param_1));  // bonus
      iVar1 = iVar1 + iVar3;
      if (100 < iVar1) iVar1 = 100;            // 100 に clamp
      piVar4 = FUN_005e02e0(0);                // actor 取得
      (**(code **)(*piVar4 + 0x134))(param_1, iVar1);  // vtable slot 0x134 設定
      FUN_0057a6f0(param_1);
      FUN_005e2670(param_1, 1);
    }
  }
  ```
  `param_1 < 8` = Oblivion の 8 属性、100 clamp → vtable 0x134 へ設定 → **属性再計算**。
- **呼び出し元 = レベルアップ処理**: FUN_00668440 の E8 呼び出しは **3 件すべて FUN_0066c3e0 内**（0x66C3E9, 0x66C3F5, 0x66C401、L431258）。FUN_0066c3e0（`__thiscall(param_1, param_2, param_3, param_4)`）は `FUN_00668440(param_2/3/4)` を 3 回 = **レベルアップで選択された 3 属性**を再計算し、続けて `uVar4 = level+1`（`FUN_004677f0`+1）を算出して `FUN_00467270(uVar4)` で適用。0x66C3E0–0x66C406 のディスアセンで 3 push/call を確認。→ FUN_0066c3e0 = レベルアップ処理、FUN_00668440 = 属性別の再計算補助。
- **FUN_00522760(void)**（L216930、`fun_00522760`、名なし）:
  ```c
  void FUN_00522760(void) {
    FUN_00519d20();
    uVar1 = FUN_00468ad0(6);
    FUN_00468af0(6, uVar1);   // カテゴリ 6 の get→set トグル
  }
  ```
  E8 呼び出し **1 件**（0x5C2ACC、fun_005c28f0 内）。内部の FUN_00519d20 は **47 件**の呼び出し（汎用ユーティリティ）。属性・レベル・GMST との関係なし。

### 手法
`dec_body`/`dec_search` で decompiled の関数本体を取得し、E8 rel32 を .text 全体走査して呼び出し元を列挙（graph ツールは Android C++ ソース指向のため不使用）。レベルアップ相関は param_1<8・clamp 100・vtable 0x134・呼び出し元の 3 属性+level+1 適用で確認。

### 残課題
なし（本体と呼び出し構造が明確、旧「低信頼」の疑問は 0x668440 側に解消）。

---

## 小a — 「0x9A–0x9F に 7 ハンドラ」（座標系パズル）

### 判定
最も有力な座標系は **Android ポートの合成 FunctionID enum**（`app/src/main/cpp/script/script_opcodes.h`）— これは EXE オプコードテーブル opid とは別の軸。ただし「0x9A–0x9F に 7 ハンドラ」は正確でなく、**当該帯域は 6 件**（Evp, PlaySound, ModCrimeGold, SetCrimeGold, KillActor, ResurrectActor）、**0x99 は空き**。総数 7 は Phase-68 の新規 SCDA 関数 7 個（7 つ目 StopQuest=0x7A は帯域外）に対応し、数え方のずれを誘発したとみられる。

### 根拠
- EXE オプコードテーブル: opid 0x0099–0x009F は**存在しない**（events 0x000–0x01E、console 0x100–0x183、空き 0x1F–0xFF）→ 「0x9A–0x9F」は EXE opid である可能性を棄却。
- `script_opcodes.h` FunctionID enum（L324–347）:
  - `SetSceneIsComplex = 0x0098`（L335、エンジンブリッジ）
  - **0x0099 = エントリなし（空き）**
  - `Evp = 0x009A`（L341、native 0x105E）、`PlaySound = 0x009B`（L342、0x1026）、`ModCrimeGold = 0x009C`（L343、0x1076）、`SetCrimeGold = 0x009D`（L344、0x1075）、`KillActor = 0x009E`（L345、0x108B）、`ResurrectActor = 0x009F`（L346、0x108C）→ **6 件**。
  - ヘッダコメント（L337–340）: 「RE_EXE_REPORT 68: opcodes the original exposes only through the native SCDA dispatcher… the synthetic FunctionID space matches the native opcode names the bridge validates against」。
- 相互参照: `native_scda_bridge.cpp` L42–53 が native {0x105E,0x1026,0x1076,0x1075,0x108B,0x108C} → 当該 FunctionID へマッピング。`script_functions.cpp` L568–579 で 6 ハンドラ登録、`script_functions.h` L232–237 で 6 つの `fn*` 宣言。StopQuest は Phase-68 の 7 つ目だが FunctionID 0x7A（native 0x1037）で 0x9A–0x9F の帯域外。

### 手法
(1) `xiaa_search.py` で decompiled の `+ 0x99..0x9f` フィールドオフセット出現数と .text の mem-disp 0x99–0x9f を実測、(2) リポジトリ grep で FunctionID enum と相互参照を特定、(3) 代替座標系（下記）を棄却。

`xiaa_search.py` 結果（デcompiled 出現数）: `+0x99`=1, `+0x9a`=1, `+0x9b`=0, `+0x9c`=420, `+0x9d`=7, `+0x9e`=8, `+0x9f`=1。.text mem-disp: 0x99=1, 0x9a=2, 0x9b=0, 0x9c=441, 0x9d=5, 0x9e=3, 0x9f=0。→ 単一オフセット（0x9c 主導）に散在し、「0x9A–0x9F に 7」の構造は該当なし。唯一 7 件の 0x9d は単一オフセットで帯域ではなく、範囲としての記載とは不整合。

**ルールアウトした座標系候補:**
- EXE オプコードテーブル opid — 空き 0x1F–0xFF に 0x9A–0x9F が存在しない。
- stride 0x28 の乗算（opcode table インデックス×0x28）— 0x9A/0x28 が非整数で帯域に到達不可。
- vtable スロット — 0x9A–0x9F（＝154–159）に 7 スロットの整列したクラスタは特定不可。
- decompiled フィールドオフセット・.text mem-disp — 上記実測のとおり散在、範囲としての「7」に一致せず。

### 残課題
「7」という数は帯域内の 6 件と厳密に一致しない（7 つ目 StopQuest=0x7A が帯域外）。座標系自体は FunctionID enum と特定できたが、記載の数え方（帯域内 6 vs Phase-68 総数 7）のどちらが原文意図かは原典照合が必要。

---

## 小c — FUN_0058a0a0 の役割と `'_'` 分岐の意味

### 判定
FUN_0058a0a0 は**登録/生成**ルーチン（ルックアップではない）: malloc(0x10) → ノード生成（`FUN_00589eb0`）→ **名前先頭文字で分岐** — `'_'` 接頭辞 → 配列/ベクタ挿入（`FUN_0042d800`）、それ以外（`'&'` 含む）→ 連結リスト先頭挿入（`FUN_006aa320`）。正しいラベル: **「名前接頭辞で振り分けるトークン登録」**。`'_'` 分岐はアンダースコア接頭辞（内部名）を公開/`'&'` トークンと別のコンテナ型へ振り分ける。

### 根拠
- **FUN_0058a0a0(char *param_1, undefined4 param_2)**（decompiled L295321、`fun_0058a0a0`、名なし）:
  ```c
  if ((param_1 != NULL) && (cVar1 = *param_1, cVar1 != '\0')) {
    if ((cVar1 != '&') && (cVar1 == '_')) {           // '_' 分岐（かつ '&' でない）
      iVar3 = FUN_00401f00(0x10, …);                   // malloc(0x10)
      param_2 = (iVar3==0) ? 0 : FUN_00589eb0(param_2, param_1);   // ノード生成
      FUN_0042d800(&param_2);                           // → 配列/ベクタ挿入
      return;
    }
    iVar3 = FUN_00401f00(0x10, …);                     // malloc(0x10)
    param_2 = (iVar3==0) ? 0 : FUN_00589eb0(param_2, param_1);     // ノード生成
    FUN_006aa320(&param_2);                             // → 連結リスト先頭挿入
  }
  ```
  呼び出し元 **FUN_0058a1c0**（L295371）は `FUN_0058a0a0("&generic;",0xffffffff); FUN_0058a0a0("&false;",1); …` とトークンを登録 → 生成・挿入ルーチン。
- **FUN_0042d800**（L33771、`__thiscall`、`fun_0042d800`）= **可変配列/ベクタ挿入**: capacity `*(ushort*)(this+8)`、index `*(ushort*)(this+0xa)`、elem-size `*(ushort*)(this+0xe)` を読み、`capacity <= index` なら `FUN_004e4a10` で拡張、`FUN_0042bb90(index, value)` で挿入。
- **FUN_006aa320**（L476147、`__thiscall(param_1, param_2)`、`fun_006aa320`）= **連結リスト先頭挿入**: コンテナ vtable スロット 1 経由でノード確保（`(**(code**)(*param_1+4))()`）、`node[2]=value, node[1]=next=0, node[0]=head`、`param_1[1]`=head / `[2]`=tail / `[3]`=count を保守。
- annotate_db に 0x58a0a0/0x589eb0/0x42d800/0x6aa320 の名前は**なし**。

### 手法
`dec_body` で本体取得、呼び出し元 FUN_0058a1c0 と分岐分派を decompiled で追跡。`'_'`/`'&'` の分岐条件は C 短絡評価の形（`cVar1 != '&'` かつ `cVar1 == '_'`）から `'&'` が `'_'` 分岐から明示的に除外され、リスト側に落ちることを確認。2 つのコンテナ型はそれぞれ 0x42d800（配列）と 0x6aa320（連結リスト）の本体から特定。

### 残課題
2 つのコンテナが厳密にどのシンボルテーブルか（名前付きの意味カテゴリ）は未特定。ただし登録/生成 + 接頭辞分派の構造は明確で、記載（「登録系 … 2 リスト振り分け」）は役割面で正しい。

---

## 判定サマリ

| 論点 | 判定（1 行） |
|------|--------------|
| **N12** | 132/39・124/37 は .text 全体では再現不可（部分スキャンの過小計）。妥当な object/vtable disp = **A 344（0x284）/ 126（0x288）**、stack B=38/25、`call [mem]`=0。 |
| **N13** | 0x6744B0・0x67B300 は**途中アドレスで非エントリ**（E8 各 0 件）。実エントリ = FUN_00674200（tick、~752 B）と FUN_0067b320（list-removal、~502 B）、**いずれも OnHit トリガーではない**（誤帰属）。 |
| **N15** | **0x668440 = レベルアップ時属性再計算**（param 0–7、clamp 100、vtable 0x134、レベルアップ処理 FUN_0066c3e0 から 3 回呼出）。**0x522760 = カテゴリ 6 の get/set トグル**（呼び出し 1 件、無関係）。 |
| **小a** | 座標系 = **Android ポート合成 FunctionID enum**（`script_opcodes.h`）。**0x9A–0x9F = 6 件**（Evp〜ResurrectActor）、**0x99 空き**。「7」は Phase-68 の新規 SCDA 7 関数（StopQuest=0x7A が 7 つ目で帯域外）。ルールアウト: opid 空き 0x1F–0xFF、stride 0x28、vtable スロット、decompiled フィールドオフセット。 |
| **小c** | **FUN_0058a0a0 = 名前接頭辞で振り分ける登録/生成**: `'_'`→配列挿入 FUN_0042d800、それ以外（`'&'` 含む）→連結リスト FUN_006aa320。ルックアップではない。 |
