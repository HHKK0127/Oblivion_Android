# v240_process_havok_residuals6.md — process/Havok 残点第6束（V-234-1/2）

- 日付: 2026-11-02 / 作成: opencode サブエージェント（TODO serial 233 / V233・低）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B）
- 出典: `tmp/re/v234_process_havok_residuals5.md` — §1 裁定残「`FUN_0088C600` の this クラス特定（低）」＋ §3 裁定残 (a)(b)(c)
- 手法: capstone 5.0.7（`v172_common.py` / `v217_util.py` 再利用）＋ `cache/{named_funcs,vtable_index,call_targets}`（参照のみ）
- 中間物: `%TEMP%\opencode\v240_1.py`〜`v240_1j.py` / `v240_2.py`〜`v240_2n.py`（各 `_out.txt` 付き）
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**。新規は本 1 ファイル＋`%TEMP%\opencode/v240_*` のみ。ESM 読取 0 回
- **VA 換算**: `imm32_refs3()` とセクション個別 raw バイト走査（`vaddr+オフセット`）のみ（v234 §ヘッダ罠注記遵守）

---

## §1 item1（V-234-1）`FUN_0088C600` の this クラス — **解明: this = bhkWorld（基底）/ bhkWorldM（派生）。RTTI `.?AVbhkWorld@@`（vtable 0xA95CF4）→ `.?AVbhkWorldM@@`（vtable 0xA97614）、ahkWorld は bhkWorld+0x8 保持**

### 証拠A: 派生 ctor `FUN_008A7B20` が vtable `0xA97614` を書く（`v240_1c_out.txt`）

`FUN_004D5000` は 0xC0 確保 → `FUN_008A7B20` で構築（`v240_1b_out.txt`）:

```
FUN_008A7B20（thiscall, ret 4、引数 = 生成元が用意したパラメータ構造体 ptr）:
  0x8A7B31  call FUN_0088AEB0          ; 基底 ctor（＝後述の bhkWorld ctor）
  0x8A7B3C  mov dword [esi], 0xA97614  ; ← vtable = 0xA97614（派生クラス）
  0x8A7B42  fst [esi+0x80] …（[0xA46B2C] float ×3 → +0x80..+0x88）
  0x8A7B5A  movaps [esi+0xA0], [edi+0x40]      ; +0xA0 = param+0x40（16B）
  0x8A7B6E  [esi+0x90] = param[0x40].x − [0xA97608]（y,z も同様、3軸）
```

### 証拠B: 基底 ctor `FUN_0088AEB0` が vtable `0xA95CF4` を書く（`v240_1e_out.txt` / `v240_1f_out.txt`）

- `FUN_004D4A20` は **0x90 確保 → `FUN_0088AEB0` のみ**（派生上書きなし）でインスタンスを生成 → `mov [esi], 0xA95CF4`。
- ctor 冒頭 `call FUN_0089D3D0`（最上位基底 ctor）は `[esi]=0xA96EB4`（= **bhkRefObject**、cache row1640）、`[esi+8]=0`（`v240_1f_out.txt`）。
- 階層（RTTI 型記述子は生バイト確認 `v240_1d/1g/1j_out.txt`）:
  - `bhkRefObject` vtable `0xA96EB4`（typedesc `0xB0694C` → 名前 `.?AVbhkRefObject@@`）
  - `bhkWorld` vtable **`0xA95CF4`**（cache `vtable_index.tsv` row1663、col `0xAD1988`、typedesc `0xB2E4BC` → 生ダンプで `.?AVbhkWorld@@`）
  - `bhkWorldM` vtable **`0xA97614`**（row1664、col `0xAD1F0C`、typedesc `0xB2EB1C` → 生ダンプで `.?AVbhkWorldM@@`）
- 注: `vtable-4` 位置の COL 相当構造は4Bシフト形（`[vt-4]=0/0/0`、pTD = `[vt-4+0xC]`、pCD = `[vt-4+0x10]`）で通常 MSVC 形とずれるが、typedesc 文字列は生バイトで直接確認済み。

### 証拠C: slot22（+0x58）= ahkWorld getter、+8 = ahkWorld\*（`v240_1e_out.txt` / `v240_1i_out.txt`）

- 両 vtable の +0x58（slot22）= **`FUN_00452A60` = `mov eax,[ecx+8]; ret`** — this から **`bhkWorld+0x8` = ahkWorld\*** を返す。
- `+8` の書手 = **`FUN_0089D400`**（bhkWorld vtable +0x4C = slot19 の本体、thunk `0x889A60: jmp FUN_0089D400`）:
  `mov dword [esi+8], edi`（＋参照計数系 slot20 の呼出し）。
- slot19 の供給元 = **`FUN_00889A70`**（slot28、bhkWorld ctor からも直接呼出）:
  ahkWorld を allocator 確保（0x2C0）→ **ctor `FUN_0089A230`**（v234 §1B と同一＝world+0x64 broadphase / +0x280/+0x290 aabb を書く側）→ `mov [esi],0xA95C54`（ahkWorld vtable）→ `this->slot19(ahkWorld)`。
- ⇒ v234 §1B の「`[edi+0x64]` 保持側」の正体は **bhkWorld(+0x8 → ahkWorld)**。

### 証拠D: `FUN_0088C600` の this 供給経路は3系統すべて bhkWorld 系（`v240_1b/1c/1f_out.txt`）

1. **グローバル `bhkWorldM` シングルトン経路**: `FUN_004CCCC0` = `mov ecx,[0xB35C24] ; push arg ; call FUN_004CA2F0`（this パススルー）→ `FUN_008A7D50` → `FUN_0088C600`。`FUN_008A7D50` は `movaps xmm0,[ecx+0x50]` と `lea esi,[ecx+0x90]`（3軸しきい値）を読む ⇒ **+0x90 存在 = 0xC0 の bhkWorldM 必須**。`[0xB35C24]` の参照は 81 箇所（0x441D63..0xA1BAA3、`v240_1f_out.txt`）。
2. **bhkWorld 一時生成経路**: `FUN_004D4A20`：0x90 確保 → `call 0x88aeb0`（bhkWorld ctor）→ `mov ecx,esi ; call FUN_0088D260` → `FUN_0088C600`。
3. **bhkWorldM 生成経路**: `FUN_004D5000`：0xC0 確保 → `call 0x8a7b20`（bhkWorldM ctor）→ `FUN_0088D260` → `FUN_0088C600`。`FUN_004D5000` は `FUN_004D5600`（位置 = グローバル `[0xB3F9A8..0xB3F9B0]`）から呼ばれ、結果を `[0xB35C24]` に格納。

`FUN_0088C600` が this から使うのは **`+0x50`（位置 vec4）と slot22 のみ** ⇒ 基底 bhkWorld で充足。派生 bhkWorldM でも同一コードが動く（+0x50 は基底フィールド）。

### 証拠E（補足・v234 訂正2件）

- **v234 §1D の「閾値内なら FUN_0088C600」は逆**: `FUN_008A7D50` の1段目ループは `fabs(|delta_i|)` と `[ecx+0x90+i*4]` を `fcompp`（st0=閾値 vs st1=|delta|）で比較し、`test ah,0x41; jnp` = CF xor ZF の条件で **|delta_i| ≥ 閾値のとき 0x8A7DBE（シフト側）へ分岐**（未満なら次の軸へ、全3軸が閾値未満なら return 0）。2段目は `|adjusted| > [0xA3D65C]` が any 軸で成立したときのみ `FUN_0088C600` を呼ぶ（`v240_1c_out.txt`）。つまり「移動が閾値を**超えたら**原点シフト」。
- `FUN_008A7B20` が +0x90..+0x98 に書くのは `param+0x40 − [0xA97608]`（3軸差分＝AABB 由来のしきい値）で、`FUN_004D5000` はその後 `[0xA46B28]` 定数で上書きする（`v240_1b/1c_out.txt`）。

### 裁定

> - **`FUN_0088C600` の this クラス = `bhkWorld`（基底、vtable 0xA95CF4、RTTI `.?AVbhkWorld@@`）／`bhkWorldM`（派生、vtable 0xA97614、`.?AVbhkWorldM@@`、最上位基底 `bhkRefObject`）で確定**。
> - slot13 呼出し元が保持するのは **bhkWorld+0x8 = ahkWorld\***（slot22 = `FUN_00452A60` が返す）。ahkWorld 本体は `FUN_00889A70` が ctor `FUN_0089A230` で生成し slot19（`FUN_0089D400`）で +8 へ格納。
> - 実運用の this はグローバル `[0xB35C24]` の **bhkWorldM シングルトン**（`FUN_004D5000` が生成）が主経路。一時 bhkWorld（0x90）経路も併存。
> - 訂正: 「閾値**超えたら**シフト」（v234 の「閾値内なら」は逆）。

**判定: 解明**（証拠 `v240_1_out.txt`〜`v240_1j_out.txt`、v234 §1 継続）。

---

## §2 item2（V-234-2）hkNull クラス名レジストリの消費系 — (a) **使用あり（v234 の未使用判定を訂正）**／(b) **factory 読手ゼロ＝write-only**／(c) **引数3/8/9 = size / member list / member count**

### (a) 走査者 `FUN_00943070` の最終確認 — **解明: 出荷バイナリで「使用される」（E9 tail-jump ＋ 遅延ファクトリリストの2段間接）**

#### 証拠A-1: `FUN_00943070` への直接経路は E9 のみ（`v240_2b_out.txt` / `v240_2c_out.txt`）

- E8 = **0 件**、imm32 参照 = **0 件**、call_targets.json = 0 — v234 の走査結果を再確認。
- **E9 jmp rel32 → 1 件 @ `0x943227`**（v234 は E9 を走査していなかった）:

```
0x943210  push 0x15 / push 0x14 / call [0xBA7D98 vtable+0x10]   ; allocator(0x14, tag 0x15)
0x94321F  mov word [eax+4], 0x14                                 ; size フィールド
0x943225  mov ecx, eax
0x943227  jmp 0x943070                                           ; ← ctor へ tail-jump
```

`FUN_00943070` は「走査ユーティリティ」ではなく **vtable `0xAA2768`・サイズ0x14 のオブジェクトの ctor**（`[edi]=0xAA2768`、`[edi+6]=1` refcount）。`FUN_00943210` がそのファクトリ。

#### 証拠A-2: ファクトリは CRT 初期化で「遅延シングルトンリスト」に登録され、Havok 初期化経路から呼ばれる（`v240_2d/2e/2h/2i/2j/2k_out.txt`）

- `FUN_00943210` の参照: E8 = 0、imm32 = **`.data 0xB30598` のみ**。
- `0xB30598` はノード `{\+0 factory=0x943210, \+4 next, \+8 out_slot_ptr=0xB305A0→0xBA94BC}`。
  登録初期化子 **`FUN_00A15980`**（`.rdata 0xA2F63C` = CRT `_initterm` 列に登録）が実行時に `node.next=[0xBA8188]; [0xBA8188]=node` で**リスト頭 `0xBA8188` へ push**（同型ノードが他にも 0xB305A4 / 0xB305B8 / 0xB305E8 / 0xB305F4…）。
- リストの歩き手 = **`FUN_008BB420`**:
  ```
  esi = [0xBA8188]（head）
  loop: eax=[esi+8]（out slot）; if [eax]==0 { call [esi]（＝factory 0x943210）; [ [esi+8] ] = eax }
        esi = [esi+4]（next）
  ```
  ⇒ **`call [esi]` が factory を遅延実行し、結果を `[0xBA94BC]`（レジストリ実体のグローバル）へ格納**。ノード+0/+4/+8 の意味はこの本体と初期化子の対応で確定。
- 到達チェーン（全て E8、末段だけ E9 tail-jump）:
  **`FUN_0040D4D0`**（e8 5件: 0x4455E0/0x44E720/0x50D4C0/0x5BDCD0 等＝広範な初期化関数）
  → `FUN_00406360`（0x40EB2C = FUN_0040D4D0+0x165C）
  → `FUN_00441970` → `FUN_005350F0` → **`FUN_0088B070`**（`cmp [0xBA8040],0` の once ガード）→ **`FUN_008BBA80`** → **`FUN_008BB420`** → node → **`call 0x943210` → `jmp FUN_00943070`**。
- 破棄側も live: `FUN_008BBB90`（head 走査＋登録解除系）は **`FUN_008897A0`（`0x889802: jmp FUN_008BBB90`）から到達**。`FUN_008897A0` は `[0xBA7A00]`/`[0xBA8038]`/`[0xBA7904]` を vtable 経由で解放する **Havok shutdown 相当**（`FUN_0088B070`（init）の対）（`v240_2o_out.txt`）。once フラグ `0xBA8040` は `FUN_0088B094`（設定元）/ `FUN_008BBA81`/`FUN_008BBB71`（設定）/ `FUN_008BBB92`/`FUN_008BBC57`（検査）のみ。

#### 証拠A-3: ctor 本体の意味と実体の消費者（`v240_2_out.txt` / `v240_2m/2n_out.txt`）

- `FUN_00943070` は2本のリストから3コンテナを作る:
  1. vtable `0xAA2738` のセットへ **list2（0xA9AA48、bss デスクリプタ94件）**を全挿入（slot2 は 2 引数形）。
  2. vtable `0xAA2748` のセットへ **list1（0xA9A910、{name,factory,vtable} レコード76件）**を全挿入。
  3. vtable `0xAA2758` のコンテナへ **`FUN_009546D0(list1, list2)`**: `key = record[+8]（vtable 欄）`、`value = descriptor` の**マップ構築**（`mov ecx,[eax+8]` = +8 のみ読み、**+4 は読まない**）。
- 実体 `[0xBA94BC]` の読手 = **`FUN_00940D60`**（`mov ecx,[0xBA94BC]; call [edx+0x10]`＝レジストリ vtable スロット呼出し）。この関数は `.rdata` の vtable 列（`0xA9C8C0` / `0xA9C99C` / `0xAA21F8`）に格納＝**vcalls で間接到達（live）**。`FUN_00940D60` は `FUN_00942170+0x37C` から呼出。
- ⇒ **「レコード/リストの最終消費者」は存在する**: vtable→hkClass（デスクリプタ）マップ = Havok **packfile（tagfile）型レジストリ**。周辺証拠: デスクリプタ名に `'hkPackfileSectionHeader'`（0xAA2720）、ファクトリノード近傍のセクション名 `'__data__'`（0xAA21E0）/ `'__types__'`（0xAA21D4）、member に `'name'`/`'parent'`（hkClass 自己記述）。

#### 裁定(a)

> - **v234 §3F「到達経路0件＝未使用の可能性」は訂正: 出荷バイナリで使用される**。経路は「CRT 初期化のリスト push（`FUN_00A15980`）→ Havok 初期化（`FUN_0040D4D0`→…→`FUN_008BB420`）→ **`call [node]`（間接ディスパッチ）→ E9 tail-jump**」の2段間接で、**E8/imm32 の全走査では検出不能**だった。
> - `FUN_00943070` の正体 = **'__types__' セクション用レジストリ ctor**（vtable 0xAA2768、サイズ0x14）、`FUN_009546D0` = vtable→デスクリプタ・マップ構築。実体グローバル = `[0xBA94BC]`。
> - 間接ディスパッチ経路の網羅確認: E9=1（0x943227）、imm32 参照=0、±0x60 のアドレス算術ヒューリスティック=0、call_targets=0 — これらを合わせて「**唯一の入口 = ファクトリ経由**」を確定。

**判定(a): 解明（未使用判定を訂正）**（証拠 `v240_2_out.txt` / `v240_2b_out.txt` / `v240_2c_out.txt` / `v240_2d_out.txt` / `v240_2h_out.txt` / `v240_2i_out.txt` / `v240_2j_out.txt` / `v240_2k_out.txt` / `v240_2m_out.txt` / `v240_2n_out.txt`）。

### (b) factory `0x8DA210` の読み手特定 — **解明: 読手ゼロ（出荷バイナリで write-only）**

#### 証拠B-1: 静的参照はレコード+4 格納のみ、直接呼出し0（`v240_2b/2l_out.txt`）

- `imm32_refs3(0x8DA210)` = **`.data 0xB2FC40`（レコード `0xB2FC3C` の factory 欄）1件のみ**、`e8(0x8DA210)` = **0**。
- レコード `0xB2FC3C` 自体の参照 = **list1 スロット `.rdata 0xA9A954` のみ**（`.text` からの直接参照 0 件）。
- **list1 全76レコードを機械走査**: `.text` 参照は**0件**（残りは全て list1 スロット自身の DATA 参照、`v240_2b_out.txt` (b2)）。同様に list2 全94デスクリプタの `.text` 直接参照は**特定クラスの thunk（書手）と live 読手のみ**（hkNull の `0xBA9118` は thunk `0xA14E6A` ＋ list2 スロットのみ）。
- `.text` の範囲走査 `[0xB2F000,0xB30600)`（レコード帯）= 211件、内訳は**浮動小数点定数プール**（0xB2FFE4=0.05f、0xB2FD20..0xB2FD2C=0.01/0.05/0.1/0.2f、0xB2FDBC..=2.748/0.369/-1.0f 等）と**無関係なグローバル**（0xB2F080/0xB2F090/0xB2FC70/0xB2FC84＝broadphase factory 表等）で、**レコード+4 読手は0件**（`v240_2c_out.txt` の生ダンプで各値の実体を確認）。
- list 帯 `[0xA9A900,0xA9AB00)` の `.text` 参照 = **6件すべて `FUN_00943070` 内**（= (a) の ctor のみ）。

#### 証拠B-2: ctor 経由でも factory 欄は読まれない（`v240_2_out.txt`）

- `FUN_009546D0` が読むのは `record[+8]`（vtable 欄＝マップのキー）のみ。レコードの `+4`（factory）は **ctor を含め全コードで未読**。
- 兄弟10レコードの factory を抽様（`v240_2l_out.txt`）: `0x914400`（hkMoppBvTreeShape）/ `0x92B250`（hkBvShape）/ `0x8E8B30`（hkTransformShape）/ `0x9148C0` / `0x914FB0` / `0x8E92C0` / `0x8EA0E0` / `0x8F3470` / `0x917990` / `0x92A540` — **e8=0 かつ参照はレコード+4 格納のみで10/10 一致**。`0x8DA210` も同型。
- ⇒ record は `{name, factory, vtable}` の3点ペアだが、**消費されるのは name（thunk 経由の文字列）と vtable（マップキー）だけ**。factory 欄は「登録されたが未使用」の静的情報。

#### 裁定(b)

> - **`0x8DA210`（hkNullCollisionFilter のインスタンス生成関数）の読み手は出荷バイナリに存在しない**。唯一の参照 = レコード+4 への格納、直接呼出し0、レコードの実消費者（`FUN_00943070`→`FUN_009546D0`）は +8（vtable）のみ読み +4 を読まない。
> - 一般化: 兄弟10件も同一（10/10）＝**record の factory 欄全体が write-only の死んだ登録情報**。実インスタンス生成は Havok 側の個別経路（各クラスの通常 ctor 呼出し）で行われ、このレジストリは型名・vtable の照合用途に限られる。

**判定(b): 解明（読手0件）**（証拠 `v240_2b_out.txt` / `v240_2c_out.txt` / `v240_2l_out.txt`、v234 §3D 継続）。

### (c) 登録 thunk 引数3/8/9 の意味 — **解明: 引数3 = インスタンスサイズ、引数8 = member（フィールド）配列、引数9 = member 数（＋引数2 = 親クラス・引数6/7 = enum リスト/数の追加同定）**

#### 証拠C-1: `FUN_0090D190` のフィールド対応（`v234_3c_out.txt` 再確認＋`v240_2f/2g_out.txt`）

```
ret 0x28（10引数）の写像（読み順）:
 [esp+4]=arg1 → this+0x00   [esp+8]=arg2 → this+0x04   [esp+0xC]=arg3 → this+0x08
 [esp+0x10]=arg4 → **読み飛ばし（保存されない）**
 [esp+0x14]=arg5 → this+0x0C  [esp+0x18]=arg6 → this+0x10  [esp+0x1C]=arg7 → this+0x14
 [esp+0x20]=arg8 → this+0x18  [esp+0x24]=arg9 → this+0x1C  [esp+0x28]=arg10 → this+0x20
```
（hkNull thunk `FUN_00A14E50`: push 0×7 → arg10..arg4=0、`push 0x18` → arg3、`push 0xBA965C` → arg2、`push 0xA9A2B8` → arg1。v234 のリテラル記述と整合）

#### 証拠C-2: デスクリプタ = **Havok `hkClass` 反射オブジェクト**（`v240_2e/2f/2g/2h_out.txt`）

| フィールド | 引数 | 意味 | 根拠 |
|---|---|---|---|
| +0x00 | arg1 | **クラス名（const char\*）** | アクセサ **`FUN_0090D1E0` = `mov eax,[ecx]; ret`（e8 12件）** が name を返す。hkNull=`0xA9A2B8` 等 |
| +0x04 | arg2 | **親 hkClass\*（0=root）** | hkNull→`0xBA965C`（thunk 確認＝**'hkCollisionFilter'** `0xAA3010`、size 0x18・親 `0xBA94C0`='hkReferencedObject'（`0xAA27C8`、親 'hkBaseObject' `0xAA351C`、size 8））。さらに 0xBA81D0='hkRigidBody'→0xBA8464='hkEntity'（size 0xC4、親 `0xBA9560`=名前未確認）の連鎖も thunk で逐次確認 |
| +0x08 | **arg3** | **インスタンスサイズ（バイト）** | hkNull **0x18=24** = factory `0x8DA210` の書込範囲 [+0x00..+0x17] と完全一致。hkClass 0x24 / hkClassEnum 0xC / hkConstraintInstance 0x28 / hkRigidBody 0xC4 / hkEntity 0xC4 / hkBaseObject 4 / hkPackfileSectionHeader 0x30 |
| +0x0C | arg5 | （未特定: 0/1/4 を観測） | 例: hkCollisionFilter=4、hkBaseObject=1、多クラス=0 |
| +0x10 | arg6 | **enum リスト ptr** | hkConstraintInstance = `0xA9DBFC`（＝'ConstraintPriority' enum 記述子、生ダンプで `'ConstraintPriority'` 確認）。member `'priority'` の enum 欄も `0xA9DBFC`（`v240_2g_out.txt`）で一致 |
| +0x14 | arg7 | **enum 数** | hkConstraintInstance=2、hkClassMember=3、hkClass/hkClassEnum=0 |
| +0x18 | **arg8** | **member（フィールド）配列 ptr** | 'hkClass' の arg8=`0xA9EFC8`＝`{'name','parent',…}` 7件の配列を生ダンプ確認。'hkConstraintInstance' の arg8=`0xB30078`＝`{'owner','data','entities','priority','wantRuntime',…}` 8件。**.rdata/.data 両方あり**（v234「.rdata の補助構造体」は一部だけ正確） |
| +0x1C | **arg9** | **member 数** | hkClass **7**（配列と一致）、hkConstraintInstance **8**、hkClassMember 8、hkClassEnum 2、hkRootLevelContainer 1 |
| +0x20 | arg10 | （全サンプル0・未特定） | — |

- member 構造体は stride `0x14` = `{name_ptr, 型 hkClass\*, enum_ptr, …}`（hkConstraintInstance の member `'data'` の型 = `0xBA8620`、`'entities'` の型 = `0xBA8464`＝'hkEntity' hkClass → **member の型がデスクリプタポインタで自己記述**）。
- **階層走査アクセサ `FUN_0090D240`** = `eax=[ecx+0x1C]; while ([ecx+4]) { eax += [[ecx+4]+0x1C]; ecx=[ecx+4] }` → **親チェーン沿いの member 数合計**（直列化時の総フィールド数計算）。`FUN_0090D1E0`（name 取得, e8 12）と併せて arg8/arg9 の意味を機能面から確定。

#### 裁定(c)

> - **引数3 = インスタンスサイズ、引数8 = member 配列、引数9 = member 数** で確定（確度: 高）。加えて **引数2 = 親クラスの hkClass\***、引数6/7 = enum リスト/数、引数1 = 名前、**引数4 は `FUN_0090D190` が読み飛ばす（保存されない）**。
> - デスクリプタ実体（.bss 94件、stride 0x24 = 9 dword）は **Havok `hkClass` 反射情報**であり、v234 §3E の「{name, 補助構造体ptr, スカラー…}」記述を精緻化する。デスクリプタ単独の live 読手（例: `0xBA8788`='hkClass' を type タグ比較 `cmp eax,0xBA8788`（`FUN_0090E860`）や `FUN_00940D90`/`FUN_00940F60`/`FUN_00942170` 等17件）も確認済み＝**descriptor 側は live、record の factory 側は dead**（(b) と対）。

**判定(c): 解明**（証拠 `v240_2e_out.txt` / `v240_2f_out.txt` / `v240_2g_out.txt` / `v240_2h_out.txt`、v234 §3E 継続）。

---

## §3 自己検証（内部整合・制約遵守）

- **§1 のクラス特定は3系統の this 供給経路が収束**: `FUN_004CCCC0`（`ecx=[0xB35C24]`）/ `FUN_004D4A20`（0x90 確保＝bhkWorld）/ `FUN_004D5000`（0xC0 確保＝bhkWorldM）すべて bhkWorld 系。`FUN_0088C600` の使用フィールドは `+0x50` と slot22 のみで、基底 bhkWorld のレイアウト（+0x50 vec4、+0x60/+0x6C に hkArray×2、+0x74 まで）と矛盾なし。alloc サイズは命令列の `push 0x90` / `push 0xC0` から直接確認。
- **§1 の RTTI**: typedesc 生ダンプ = `0xB2E4BC+8 → ".?AVbhkWorld@@"`、`0xB2EB1C+8 → ".?AVbhkWorldM@@"`、`0xB0694C+8 → ".?AVbhkRefObject@@"`。vtable_index.tsv row1663/1664/1640 と一致。
- **§2(a) の経路**: 末段のみ E9（0x943227）、他は全て E8 直呼出しで、`e8`/`e9`/`imm32_refs3`/`call_targets` の4種を突合。偽陽性対策として **`imm32_refs3(0x406360)` の `.text 0x5718EC` ヒットは `call 0x977C50` の rel32 バイト列（0x5718EB+1 から計算値 0x406360）＝偽陽性**であることを disasm で確認し除外した（本物の参照は0件、到達は E8 のみ）。
- **§2(a) のノード整合**: ノード `{\+0 factory, \+4 next, \+8 out}` は `FUN_008BB420`（+0 を call、+4 を next、+8 を out-deref）と初期化子 `FUN_00A15980`（+4 に旧 head、head にノード先頭）の2系統が一致。list1/list2 の hkNull 位置 = 両リスト **index17** で平行対応。
- **§2(b) の網羅性**: list1 全76・list2 全94 を1件ずつ `imm32_refs3` で走査（レコード/デスクリプタ単位）＋帯レンジ走査2本（レコード帯・list 帯）＋個別4アドレス（0xB2FC3C/0xB2FC40/0xBA9118/0xB2FA8C）で三重確認。レコード帯211ヒットの内訳は生ダンプで浮動小数点定数・無関係グローバルと判定。
- **§2(c) の写像**: `ret 0x28` = 40B = 10引数と、push 数（hkNull = 7 zeros + 3 = 10）、`FUN_0090D190` の読み順（[esp+4]→[esp+0x28]）が一致。arg4 未保存は命令列（[esp+0x10] への読みが0回）で確認。member 配列の要素数 arg9 と生ダンプ実数（hkClass 7件・hkConstraintInstance 8件）が一致。
- **§2 の外部参照**: なし（すべて EXE 内訳拠。web・他バイナリ参照0回）。
- **制約遵守**: ESM 読取0回・ソース編集0回・既存 `tmp/re/*.md` 無編集（新規は本1ファイルのみ）・git/docs/GT/TODO/session-memory/plan 無編集・`cache/` は参照のみ。スクリプトは全て `%TEMP%\opencode\v240_*.py`（`python "<path>"` 実行、ヒアドック・`python -c` 正規表現なし）。新規中間物: `v240_1..1j` / `v240_2..2n`（各 `_out.txt` 付き）。

## §4 新規残点

1. **[残] descriptor `+0xC`（thunk 引数5）の意味**（低）: 値 0 / 1 / 4 を観測（hkCollisionFilter=4、hkBaseObject=1、多クラス=0）。enum 数でも size でもない。version/flag の可能性。
2. **[残] descriptor `+0x20`（thunk 引数10）の意味**（低）: 全サンプル0。保存されるが用途不明。
3. **[残] 登録 thunk 150件 と list2（94件）の差分56の正体**（低）: `FUN_0090D190` の E8 呼出し150に対し list2 は94。残り56は enum 系（`hkClassEnum` 等）か別リスト未登録か未突合。
4. **[残] `'__data__'` セクションのファクトリノード完全同定**（低）: `0xB30590` の `'__data__'` 文字列と対応するノード/初期化子が特定できていない（'__types__' ノード `0xB30598` のみ確定。`0xB305A0=0xBA94BC`（'__types__' の out 槽）等の並列構造は一部推定）。
5. **[残] `FUN_00940D60` の受信クラス名**（低）: `[0xBA94BC]`（レジストリ実体）の消費者で `.rdata` vtable 列（0xA9C8C0/0xA9C99C/0xAA21F8）から間接呼出し＝live。受信側 packfile reader 系クラスの名称未特定。
6. **[残] item1 周辺の上流文脈名**（低）: `FUN_004820CF`→`FUN_004CCCC0`（bhkWorldM シングルトン更新）と `FUN_0040D4D0`（Havok 初期化の起点、5呼び出し）の役割名が未命名（named_funcs は FUN_ のみ）。
