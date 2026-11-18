# v536 — formID プール機構の確定（`FUN_00448D40`＝DataHandler `+0x8c0` カウンタ＋ハッシュ衝突検査＋グローバル map 二重チェックの採番器・`FUN_00446B40`＝先頭 release のカウンタ巻き戻し・`FUN_00453450`＝「formID 使用中」判定＝`FUN_0055E000` 検索）・TESForm vtable `+0x68`＝false 固定 / `+0x70`＝GetFormType [確定]・topic リスト（`DAT_00b33a98+0x7c`）head への disp32 ベース書込みは 237 関数走査でも不在（登録は head ポインタ引数経由に限定）＋v535 の「append 先=topic リスト」候補 3 箇所を別 head に訂正（V-536-1..3 完遂・旧残点 V-406-3 継続第66弾）

- 採番: **v536**（旧残点群消化・probe prefix **v536_1..2 全2本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-536-1..3（v535 残点）** — 1=topic リスト head の別経由参照探索／2=`FUN_00448D40`/`FUN_00446B40` 本体（formID プール機構）／3=subject vtable `+0x68/+0x70` の TESForm slot 意味。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・実文字列のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-536-2: `FUN_00448D40`＝新 formID 採番器** | **[確定]（命令＋docs L55955）** | this＝DataHandler（`DAT_00b33a98`、v528 系譜）。①**カウンタ `+0x8c0`** を読み次候補 `iVar5` を作す: **下24bit が 0xFFFFFF なら「mod byte 据え置きで +0x800」**（=そのファイルの ID 空間を 0x800 単位で進める）、else **+1** ②カウンタ更新 ③**ハッシュテーブル検査**: `PTR_PTR_00b0613c`（ポリシー object ptr）の `vtable[+4](id)`＝バケット index、**`DAT_00b06144 + idx*4`** の連結チェーンを歩き `vtable[+8](id, node->form)` が真（＝衝突）なら ① へやり直し ④**`FUN_00453450(id)`（グローバル map 検索）が真でもやり直し** ⑤最終関門 **`FUN_0055E000(候補, &out)`** が偽（map 不在）かつ `453450` 偽 ならその id を返す。→ **formID プール＝「DataHandler+0x8c0 のファイル内カウンタ」×「ハッシュ衝突検査」×「グローバル form map 検査」**。 |
| **V-536-2: `FUN_00446B40`＝カウンタ巻き戻し（先頭解放）** | **[確定]（命令＋docs L54110）** | `if (id != 0 && (id & 0xFF000000) == 0xFF000000 && id == +0x8c0 - 1 && (id & 0xFFFFFF) > 0x7FF) +0x8c0 = id;` ＝**「最高位 0xFF（本体ファイル）で現在のカウンタの直前（＝最後に採った ID）であり下24bit が 0x7FF 超」のときだけカウンタを戻す**。v535 ctor の「0x800 プレースホルダ化時に旧 ID を返す」対応物。 |
| **V-536-2: `FUN_00453450`** | **[確定]（命令＋docs L64202）** | `FUN_0055E000(id, &out); return out != 0` ＝**「id がグローバル form map に在るか」判定**（v528 の `55E000`＝map 検索の再利用）。 |
| **V-536-2 派生: map 機構の全体像** | **[確定]（命令＋docs）** | `FUN_004524C0`＝**削除**: `vtable[+4](key)` バケット→チェーン探索→一致ノードを link 外し `vtable[+0x10]`（デタッチ？）→`vtable[+0x18]`（free）→**カウント `param_1[3]--`**。`FUN_00452570`＝**挿入**: バケット先無ければ `vtable[+0x14]`（ノード生成）→`vtable[+0xC](node,key,key)`→先頭連結→`count++`、一致在りのときは既存ノードへ `+0xC(node, key, retaddr?)` 上書き（**バケット付きハッシュ map の汎用 CRUD**、コールバック 5 種＝key hash/compare/detach/free/make）。 |
| **V-536-3: TESForm vtable `+0x68`** | **[確定]（生バイト）** | `0x6FE080: xor al,al; ret 4` ＝**false 固定 no-op**。`FUN_00451920` の先行フィルタ `subject->vtable[+0x68](file)` は基底では常に偽＝**「ウォーク本体（4CC+formID 検索）へ必ず進む」**設計（override 用フック枠）。 |
| **V-536-3: TESForm vtable `+0x70`** | **[確定]（生バイト）** | `0x412970: movzx eax, byte ptr [ecx+4]; ret` ＝**`GetFormType`（type byte 返却）確定**。`FUN_00451920` の期待 4CC 算出 `表[type*3+4]`（f1）と v528 の「+0x70 は GetType」と相互裏付け（v528 [確定相当]→ **[確定]**）。 |
| **V-536-1: topic リスト head への書込み** | **[確定負]（機械走査強化）＋ [部分確定]** | `0xB33A98` disp3x を含む **.text 関数 head 237 個を全命令 walk**（+0x7c を `add/lea reg,0x7c`・`[reg+0x7c]` 形式で走査）。**`+0x7c` を DataHandler ベースで使うのは TESTopic 領域の読み手 3 関数（`FUN_0052F690` 走査／`FUN_0052F6D0` 走査・ソート／`FUN_0052FAE0` 走査）のみ**。store 形式 `mov [reg+0x7c], X` を持つ head はあるが、**それらは b33a98 と無関係なレジスタ（false positive・個別確認）**。→ **EXE 中に「disp32 b33a98 → +0x7c への直書き」は存在しない [確定負]**。登録は **head ポインタ（`b33a98+0x7c` のアドレス）を引数で受け取る関数**経由に限定される。 |
| **V-536-1 併行: v535 の append 先 3 候補の訂正** | **[確定]（生バイト）→ v535 §2.4 の「topic 系 append=候補」は誤り → 訂正** | ① `0x52FD02`（`FUN_0052FC40`）: `mov ecx,ebp` ＝**関数ローカル変数**（TESTopic+0x28 サブ構造ノード操作域）→ global リストではない。② `0x530D33`: `call 0x530C40; mov ecx,eax; call 0x67B1E0` ＝**サブルーチンが返した list object への append**（head は返り値＝呼び出し側が保持する object、global 直参照なし）。③ `0x5313CF`（`FUN_00531290`）: **`mov ecx, 0xb36570`** ＝ **global `DAT_00b36570`**（`DAT_00b36510` と 0x60 差の別 scratch リスト。会話候補生成系の第 2 リスト）。→ **3 箇所とも `DAT_00b33a98+0x7c` への append では無い**。 |
| **V-536-1 補強: `FUN_0052F330` の真の姿** | **[確定]（docs L225369）→ v535 §4 の「topic 破棄／リセット系」は誤り → 訂正** | `param_1+0x28`（TESTopic の INFO {arr,count} 配列、v535 で確定）を **要素の `byte+0x3d`（priority バイト）昇順でバブルソート**（swap は配列要素交換）。`FUN_0052FAE0` の walk 中で topic ごとに呼ばれる＝**会話生成前に各 topic の greetings/infos を優先度順に並べ直す処理**。 |

## §2 証拠

### §2.1 `FUN_00448D40`（docs L55957..55994 要点）
```c
do {
  uVar1 = this[0x8c0];                       // 現カウンタ
LAB: do {
    uVar2 = this[0x8c0];
    if ((uVar2 & 0xffffff) == 0xffffff) next = (uVar2 & 0xff000000) + 0x800;   // mod byte 保持・0x800 進め
    else                                 next = uVar2 + 1;
    this[0x8c0] = next;
    bucket = policy->vtable[+4](next);        // PTR_PTR_00b0613c
    for (node = buckets[bucket]; node; node = node[0]) {   // DAT_00b06144 + idx*4
        if (policy->vtable[+8](next, node[1])) goto LAB;   // 衝突 → やり直し
    }
  } while (FUN_00453450(this[0x8c0]));        // グローバル map 在住 → やり直し
  if (!FUN_0055E000(uVar1, &out) && !FUN_00453450(uVar1)) return uVar1;   // 二重チェックを通過したら採用
} while (1);
```

### §2.2 `FUN_00446B40`（docs L54112）
```c
if (id && (id & 0xff000000) == 0xff000000 && id == this[0x8c0] - 1U && (id & 0xffffff) > 0x7ff)
    this[0x8c0] = id;                        // 最後に採った ID を返却 → カウンタ巻き戻し
```

### §2.3 map CRUD（`FUN_004524C0` 削除 / `FUN_00452570` 挿入、docs L63085/L63124）
```
削除: bucket=vt[+4](key); .walk チェーン: vt[+8](key,node[1]) 一致 →
      bucket 直結なら buckets[i]=node->next、でなければ prev->next=node->next
      → vt[+0x10](node) → vt[+0x18](node) → count-- → 1
挿入: bucket=vt[+4](key); 一致なしでチェーン終端 → node=vt[+0x14]()（生成）
      vt[+0xC](node,key,key) → node->next=buckets[i]; buckets[i]=node; count++
```

### §2.4 `+0x68`/`+0x70` 生バイト
```
6FE080: xor al,al ; ret 4                                  ; false 固定
412970: movzx eax, byte ptr [ecx+4] ; ret                  ; GetFormType
```

### §2.5 237 関数走査の内訳（v536_2）
```
disp32 0xB33A98 を含む head = 237。うち `+0x7c` 使用:
  0x52F690 add esi,0x7c（走査・読）
  0x52F6D0 add ebp,0x7c（走査・名前ソート）
  0x52FAE0 add edi,0x7c（走査・F330 呼び）※head 表記は 0x52F830 walk 越境
  （0x416000/0x4430F0/... の [reg+0x7c] 系は b33a98 と無関係レジスタ＝誤検出と個別確認）
→ disp32 ベースの store は 0 件 [確定負]。
```

### §2.6 append call site 3 箇所の ecx（v535 §2.4 訂正）
```
52FD00-52FD02: mov ecx,ebp / mov [eax],edi / call 67b1e0    ; ebp=ローカル（52FC40 内）
530D2C-530D33: call 530c40 / mov ecx,eax / call 67b1e0      ; ecx=返り list object
5313C9-5313CF: push esi / mov ecx,0xb36570 / call 67b1e0    ; global DAT_00b36570
```

## §3 判定の内訳

- **V-536-2 完遂**: formID プールは「**ファイル内カウンタ（DataHandler+0x8c0）＋ハッシュバケット衝突＋グローバル map（55E000/453450）二重チェック**」の三層で確定。0x800 境界（未配置プレースホルダ）のスキップ/巻き戻し規則も命令で決着。CRUD（4524C0/452570）はバケットハッシュ map の汎用実装として確定。
- **V-536-3 完遂**: `+0x68`＝false 固定 override フック、`+0x70`＝GetFormType（v528 の [確定相当] を [確定] へ昇格）。
- **V-536-1 部分完遂**: head への disp32 ベース書込みの不在を 237 関数走査で示し [確定負] に強化。v535 の候補 3 箇所はいずれも別 head（ローカル／返却 object／`DAT_00b36570`）と判明し**訂正**。`FUN_0052F330` は topic+0x28 の priority（byte+0x3d）ソートと確定（v535「破棄系」を訂正）。**head アドレスを引数に渡す呼び出し元（登録経路そのもの）は未特定のまま残点**。

## §4 残点（V-537-1..3）

1. `DAT_00b33a98+0x7c` の**アドレスを引数で渡す** call site 探索（`add reg,0x7c` ではなく `lea reg,[rbx+0x7c]` の rbx=[b33a98] 分離型を含む再走査。`FUN_00446CB0`/`FUN_0067B1E0`/`FUN_0065C620`（v529 リスト削除）callers 107+347 の ecx 計算を全列挙）【中】
2. `PTR_PTR_00b0613c`（ハッシュポリシー object）の**実クラス**と `DAT_00b06144` バケット配列の寿命（誰が作るか）【低】
3. `TESTopic+0x28` の {arr,count} への**要素追加/削除命令**（6B80D0 系 array insert/remove の本体確認。v535 ctor 経由の経路）【中】

## §5 後続への申し送り

- **移植側の formID 割当は 3 層で再現**: ①ファイル内カウンタ（下24bit roll、0xFFFFFF 超えは +0x800 単位・mod byte 保持）②グローバル form map（key=id）③ハッシュ衝突チェイン。**0x000〜0x7FF は予約**（未配置プレースホルダ 0x800 扱い）。
- **`FUN_00446B40` の条件（0xFF mod・カウンタ直前・>0x7FF）だけ**カウンタを戻せる——乱雑な free はカウンタを動かさない。移植側で「ID 回収」を単純デクリメント実装にすると元の挙動とズレる。
- **`+0x68` は常時 false の override フック枠**（TESForm 基底）。`FUN_00451920` はこのフックを先に問うてから検索へ進む——移植側は「form 側で読み飛ばしを認める鉤」としてスロットを残すこと。
- **`DAT_00b36510`（会話候補）と `DAT_00b36570`（別 scratch）は 8B ヘッダの連結リスト**。TESTopic 系 global はこの 2 つ＋`b33a98+0x7c`（topic 一覧）の 3 系統。
- **priority ソート（`52F330`）は info の `+0x3d` バイト**を使う。移植側で greeting 選択の順序がずれる原因の第一候補。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
