# v535 — `FUN_0046C460`＝**`TESForm` ctor（vtable 0xA3BE3C ストア＋`0xB05E04` 表 f1 packed 4CC の実行時充填場所＋"TESForm.cpp" 自己申告文字列）**・form `+0x10` リスト＝**TESFile（出自プラグインファイル）チェーンと確定**（`FUN_0046B6C0` の 2 call site で arg=ファイルオブジェクト）・`FUN_004518B0`/`FUN_00451920`（bit12 スキップ／type+formID による form 探索ウォーカ）確定・`FUN_0052F010`＝会話候補生成（push 先は `DAT_00b36510` で topic リストではないと判明）（V-535-1..3 完遂・旧残点 V-406-3 継続第65弾）

- 採番: **v535**（旧残点群消化・probe prefix **v535_1..5 全5本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-535-1..3（v534 残点）** — 1=topic リスト（`DAT_00b33a98+0x7c`）登録 call site 決着／2=`FUN_004DB4A0` が読む form `+0x10` リスト item 型／3=`FUN_004518B0`/`FUN_00451920` 本体。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・実文字列のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-535-2: form `+0x10` リストの正体** | **[確定]（両 call site の生バイト）＝「この form を定義した TESFile（プラグイン／セーブファイル）のチェーン」** | ① `0x451289`（`FUN_00451210` NextFormEx 末尾）: `push esi`（**esi=TESFile**）`/ mov ecx, edi`（form）`/ call 0x46b6c0` ＝ **レコードを読んだ form に、読込元の TESFile を push**。② `0x46C5A7`（`FUN_0046C460` TESForm ctor）: `mov ecx,[0xb33a98]; mov eax,[ecx+0x8c4]`（**DataHandler の現アクティブファイル**）`/ push eax / mov ecx,esi / call 0x46b6c0` ＝ **ctor 時に現ファイルを出自リストへ登録**。→ `FUN_004DB4A0` の `FUN_0046B680(form,-1)`＝**出自の先頭ファイル**、`FUN_0044FAA0(file)`＝**file+0x3dc bit0**（v534 の「item 型未確定」を解消。+0x3dc は DataHandler ではなく **TESFile の状態ビット群**）。 |
| **V-535-2 派生: `FUN_0046C460`＝`TESForm::TESForm()` ctor** | **[確定]（自己申告文字列＋命令）** | **`*param_1 = &PTR_FUN_00a3be3c`**（v528 で全 slot 確定済みの TESForm vtable）をストア。`param_1[2]=8`（flags dword＝`+0x8` 初期値 8）／`param_1[3]=0`（formID）。**(1) 4CC 表の初期化**: `DAT_00b33c10`（init フラグ）が立つまで **`0xB05E04` 表 69 件**（`local_8=0x45`）を回り、**f0 名前文字列 4 文字を packed に変換して f1 へ書込む**（`ppuVar5[1] = ((s[3]<<8|s[2])<<8|s[1])<<8|s[0]`）→ **v529 の「f1=実行時充填の packed 4CC」の充填場所を特定**（[確定] に昇格）。整合チェックで **`"formEnumString[ %d ].cFormID in TESForm.cpp is out of order."`**／**`"... have the same iFormString %s in TESForm.cpp."`**／**`"You must fix the problems in TESForm.cpp to run this game."`**（`0x404EC0`＝致命的 abort）。**(2) formID 採番**: 非保存中（`TLS[+0x184]==0`）なら `FUN_00448D40()`（新 formID 割当）→ `+0xC`、`<0x800` は 0x800 プレースホルダ扱い（旧登録 `FUN_004524C0` 解除＋`FUN_00446B40`）→ **`FUN_00452570(formID, this)`** でグローバル map 登録（v528 の「+0x40 save-change 登録」の対応物）。 |
| **V-535-3: `FUN_004518B0`** | **[確定]（命令＋docs）＝「bit12 レコード読み飛ばし（NextRecord）」** | `+0x414/+0x418`（解凍バッファ）を free して 0 クリア → **`FUN_00451310`（NextForm）を呼び、`type!='GRUP' && type!='TOFT' && (flags & 0x1000)` の間ループ**。→ **record flags bit12（0x1000）＝「このレコードは読み飛ばし対象」マーカー**（'AGGR' 集約スキップの実体は、AGGR レコード側の flags bit12 ではなく **`FUN_00451A60` が型 index==1 の間このスキップを呼ぶ**構成。bit12 自体はロード系の一般フラグ）。 |
| **V-535-3: `FUN_00451920`** | **[確定]（命令＋docs）＝「(型, formID) 指定のレコード探索ウォーカ」** | 前提 `+0x28c!=0`（`FUN_00451A60` が設定）・stream!=0・**stream+0x24!=0**（ストリーム有効）。① `subject->vtable[+0x68](file)` 先行フィルタ（真で即 1）② `FUN_004512A0(1)`（Reset＋初回読）③ 先頭が 'TES4'（`DAT_00b05e14`）なら **連続 'TES4' を `FUN_004518B0(1)` で読み飛ばす** ④ 期待 4CC=`表[subject->vtable[+0x70]() * 3 + 4]`（f1 packed 4CC＝**`+0x70` は GetFormType 系**と相互確認）＋期待 formID=`subject+0xC` ⑤ ループ: `'GRUP'` なら `subject->vtable[+0xBC](&hdr,1,0)`（**v530/531 の membership predicate そのもの**、第2引数=1）が偽なら **`FUN_00451580()`（グループ閉じ）**、真なら ⑧ の読進へ／レコードなら `type==期待4CC && +0x248==期待formID` でヒット（`local_5=1`）で終了、外れたら ⑧。→ **ロード側が「目的 form の本体レコードまでグループ構造を辿る」中核関数**。 |
| **V-535-1: `FUN_0052F010` の正体と topic リスト登録** | **[部分確定]（命令）→ v534 の仮説を修正** | `FUN_0052F010` は「**会話（greeting）候補生成**」: flags bit5 なら即 0／候補リストヘッド **`DAT_00b36510`**（8B、初回 `FUN_00401F00(8)` 生成）へ `push` しながら歩く。TESTopic の `+0x28` サブリスト（{arr@8, cnt@0x10}）の各 info を `FUN_00530830` で絞り、`+0x25` bit5 で候補停止・bit1 で **`FUN_00446CB0(DAT_00b36510 の head, info)`（v530 確定の単方向 push）に追加**。候補から **`FUN_0047DF80(0) % n + 1` 番目**を返す（`0x47DF80` は `0x985994/0x9859D0` 疑似乱数系＋`DAT_00b069c3` 再初期化フラグ）。**この push 先は `DAT_00b36510` であり `DAT_00b33a98+0x7c`（topic リスト）ではない** → **topic リストへの登録 call site は依然未発見**（`.text` の `0xB33A98` 参照 548 箇所を機械走査、`+0x7c` を使う関数は `FUN_0052F690`（読）と `FUN_0052F6D0`（読・ソート）の 2 関数のみと確定 [確定負: この 2 関数以外の直接参照]。登録は **head 埋込先（DataHandler 内）を指す別経由**とみられ、残点 V-536-1 に降ろす）。 |
| **派生 [確定] `FUN_0052F520`/`FUN_0052F570`/`FUN_0052F480`** | **[確定]（命令）** | F520: `topic+0x28` サブ配列を歩き `child==param_2` の**子を持つ TESTopic を返す**。F570: 同走査で一致子の **topic 自身（*puVar2＝sub[0]）を返す**（F5D0 の list 版は既に v533）。F480: `+0x34` の名前を `__stricmp` する比較ヘルパ（ソートフック `0x52F4E0` 経由）。→ **`+0x28`＝TESTopic の INFO（topic info）配列 {arr,count}**（v533 の「topic+0x28 サブリスト」の正確化: {item,nodes} 連結でなく **{arr@+8, count@+0x10} の配列**）。 |

## §2 証拠

### §2.1 form `+0x10`＝TESFile チェーン（call site 生バイト）
```
0x451279: mov ecx,[esi+0x248] / push 1 / push ecx / mov ecx,edi / call 0x46c300   ; SetFormID(+0x248)
0x451289: push esi（=TESFile）/ mov ecx,edi（=form）/ call 0x46b6c0               ; ★ 出自ファイル push
---
0x46C597: mov ecx,[0xb33a98] / mov eax,[ecx+0x8c4]（現アクティブ TESFile）
0x46C5A7: push eax / mov ecx,esi（=構築中 form）/ call 0x46b6c0                    ; ★ ctor でも push
0x46C58F: call 0x448d40 / mov [esi+0xc],eax                                        ; 新 formID 採番
```

### §2.2 `FUN_0046C460` の 4CC 表充填（docs L85968..86001）
```c
if (DAT_00b33c10 == 0) {                     // 初回のみ
  DAT_00b06b18 = 1;                          // リエントリガード
  p = &PTR_DAT_00b05e04; n = 69;
  do { s = p[0];                              // f0: 名前文字列
       p[1] = ((s[3]<<8|s[2])<<8|s[1])<<8|s[0];   // ★ f1 = packed 4CC 充填
       if (p[-1byte] != idx) log("formEnumString[ %d ].cFormID in TESForm.cpp is out of order.");
       for (m=0; m*0xc < 0x33c; m++) if (idx!=m && p[1]==tablef1[m]) log("...same iFormString...");
       p += 3; n--; } while (n);
  if (bad) FUN_00404EC0("You must fix the problems in TESForm.cpp to run this game.");
  DAT_00b33c10 = 1;
}
```

### §2.3 `FUN_004518B0` / `FUN_00451920`（docs L62420/L62443 要約）
```
18B0(file, stopWhenBit12Gone=param_2):
  if (+0x414) { free(+0x414); +0x414=+0x418=0; }
  c = NextForm();
  while (c && param_2 && type!='GRUP' && type!='TOFT' && (flags & 0x1000)) c = NextForm();
1920(file, subject) -> bool:
  if (+0x28c && stream && stream+0x24):
    if subject->vtable[+0x68](file) return 1;
    Reset(1); while (type=='TES4') skip(1);
    want4cc = 表[subject->vtable[+0x70]() * 3 + 4]; wantID = subject+0xC;
    while (type):
      if type=='GRUP': if !subject->vtable[+0xBC](&type,1,0) CloseGroup(); else skip(1);
      else if (type==want4cc && +0x248==wantID) { hit; break; }
      else skip(1);
```

### §2.4 topic リスト（`DAT_00b33a98+0x7c`）の直接参照は読み手 2 関数のみ [確定負]
```
.text の disp32==0xB33A98 全 548 箇所を関数単位で走査:
  「mov reg,[0xb33a98] → add/lea +0x7c」を持つ head = FUN_0052F690（所有者検索）、FUN_0052F6D0（名前ソート）のみ。
  FUN_0052F010 の FUN_00446CB0 push 先 = DAT_00b36510（別リスト）。
  → head への書込み（登録）はこの 2 関数経由ではなく、+0x7c を指す別計算（未追跡）で行われる [残点]。
```

## §3 判定の内訳

- **V-535-2 完遂**: form `+0x10`＝**出自 TESFile チェーン**と確定。`FUN_004DB4A0` のゲートは「**出自の先頭ファイルが生きている（+0x3dc bit0 未設定）か**」を読むもの＝**v533 の「DataHandler の読込済フラグ」解釈を TESFile 側へ訂正**（関数 `FUN_0044FAA0` 自体は汎用 getter）。`FUN_0046C460`＝TESForm ctor の自己同定も兼ねる。
- **V-535-3 完遂**: `FUN_004518B0`/`FUN_00451920` を確定。特に **`FUN_00451920` がロード側の「グループ構造を辿って (型,formID) レコードに到達する」中核**で、v530〜v533 の保存／close／predicate が全部この中で再利用されている（`+0xBC(hdr,1,0)`／`FUN_00451580`／表 f1）。
- **V-535-1 部分完遂**: 会話候補生成（`DAT_00b36510`）と topic+0x28 配列構造を確定。topic リスト（+0x7c）への登録命令は**読み手以外の直接参照が無いことを機械的に示せました**（[確定負: 既知 2 関数外の直接 +0x7c 参照]）ため、登録は head アドレスを指す別計算（`lea` の即値でなくベース＋オフセット演算等）経由と限定できた → 残点 V-536-1。

## §4 残点（V-536-1..3）

1. topic リスト（`DAT_00b33a98+0x7c`）head の**別経由参照**の探索（`mov reg,[0xb33a98]` の後、`0x7c` 以外の経由（サブルーチンに +0x7c 付き this を渡す等）で登録する場所の特定。候補: `FUN_00446CB0`/`FUN_0067B1E0` の callers 548 箇所から ecx 計算が b33a98 ベースのものを洗う）【中】
2. `FUN_00448D40`（新 formID 割当）と `FUN_00446B40`（formID 解放?）の本体確定＝**formID プール機構**【中】
3. `subject->vtable[+0x68]`/`[+0x70]`（`FUN_00451920` の先行フィルタ／型取得）の TESForm vtable slot 意味（`+0x70` は GetFormType と推定済み→命令で確定へ）【低】

## §5 後続への申し送り

- **移植側 TESForm ctor 必携の 3 仕事**: ①vtable ストア ②flags=8 初期化 ③formID 採番（`448D40` 相当）＋グローバル map 登録（`452570` 相当）＋**出自 TESFile チェーンへ自分自身のファイル push**。0x800 未満の formID は「未配置」プレースホルダ。
- **4CC 表 f1 は ctor の初回実行で名前文字列から packed 化される**（ディスク上の 0 は正しい）。ゲームは表の順序・一意性を起動時に検証し、壊れていれば abort（"You must fix..."）。移植側は表生成時に同じ検査を入れると元の挙動と一致。
- **`+0x244` bit12（0x1000）＝ロードスキップマーカー**、`FUN_004518B0` は「解凍バッファを捨てて次の非スキップレコードへ」。`FUN_00451920` が「(型,formID) まで GROUP を開いたり閉じたりしながら歩く」唯一の経路で、`+0xBC` predicate の第2引数に **1** を渡す点（保存側 `FUN_004502C0` の `(node,0,1)` とは組み合わせが違う）に注意。
- **topic（TESTopic）は `+0x28` に INFO の {arr,count} 配列を持つ**（連結リストではない）。`DAT_00b36510` は会話候補の一時的な連結リスト（8B ノード）で、毎回の生成で使い回される。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
