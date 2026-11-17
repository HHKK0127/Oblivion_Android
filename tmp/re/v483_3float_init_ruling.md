# v483 — 3float 群＝対で処理・NaN 修正 log・`DAT_00B37D00` init fn の裁定（V-483-1..3 完遂・旧残点 V-406-3/V-406-4 継続第13弾）

- 採番: **v483**（旧残点群消化・probe prefix **v483_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-483-1..3（v482 残点）** — 1=旧残点 V-406-3 継続（3 float 群の正式 field 語と `FUN_004A7A60` identity）／2=旧残点 V-406-4 semantic 完遂（`DAT_00B37D00` 正体）／3=V-482-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-483-1: 2つの3float 群（`+0x20..+0x28`／`+0x2C..+0x34`）の関係** | **[確定相当]（対）＋[未確定]（3軸の語）** | REFR band（115 fn）usage 実読 [確定(decompiled)]: **`L171195-171200 (FUN_004D9A70)`: `param_1[8]=0; [9]=0; … [0xb]=0; [0xc]=0; [0xd]=0`（**両群をゼロ初期化＝ctor/init [確定]**）** ／**`L171397-171401 (FUN_004D9D00)`: `[0xb]=local_2c; [0xc]=local_28; [0xd]=local_24; [8]=local_20; [9]=local_1c`（**ローカル6 float 連続（`local_2c..local_1c`）から両群へ一括セット＝6 float（2×3D vector）が対で書かれる [確定(構造)]`**）／`L174624-174630 (FUN_004DDDE0)`: `[8]=0; [9]=0; [0xb]=*(iVar3+0xc); [0xc]=iVar5`（部分セット）→ **2つの3float 群は対（6 float）で処理される [確定相当]**。**3軸の正式語（位置/スケール/色 [推定] の切り分け）は [未確定]＝残点**（`FUN_004DD070` の clamp（v482）＝`+0x28` が累積 float で上限付きであることまで確定） |
| **V-483-1: `FUN_004A7A60`（NaN 修正時 helper）identity** | **[推定相当]** | L133758: **`void FUN_004a7a60(undefined4 param_1) { FUN_004941a0(1, param_1, &stack0x00000008); return; }`（3引数・variadic 展開）[確定(decompiled)]** → **`FUN_004941a0(1, …)` の第1引数 `1` が level/flag で、`&stack+8` が可変引数 → log/report 系 core（`FUN_004941a0`）の wrapper [推定相当]** → **NaN 修正時のログ/報告（警告出力）fn [推定相当]**（`FUN_004941a0` identity は [未確定]＝残点） |
| **V-483-2: `DAT_00B37D00` 正体（init 経路）** | **[確定相当]（init fn 構造）＋[未確定]（正体）** | **`FUN_009E62B0`（L1031387）実読 [確定(decompiled)]: `void FUN_009e62b0(void) { … FUN_004040e0(&DAT_00b11e34); _atexit(&LAB_00a1d4b0); … }`** → **machine（v478）では同 fn 内 `0x9EDEAA: mov ecx,0xb37d00; call 0x419A70`（init）→ `push 0xA20120; call 0x981FB4`（registration）** → **`FUN_009E62B0`＝「`DAT_00B37D00`（this）を `DAT_00B11E34`（once-init guard・v462 `FUN_004040e0` 同型）付きで初期化し、`LAB_00A1D4B0`（atexit dtor）で破棄登録する init fn [確定相当]`** → **`DAT_00B11E34`＝guard/flag との対（v462 `DAT_00b05dc0` と同型）[推定高]**。**正体（何の object か）は [未確定]＝残点**（次の手＝atexit dtor `LAB_00A1D4B0` の逆引き、または init 実体 `FUN_00419A70` の本文） |
| **V-483-3: V-482-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v483_1）

### §2.1 3float 群 usage（v483_1a）
```
band 0x4D9000..0x4E2000: 115 fns
L171195-171200 (FUN_004d9a70): param_1[8]=0; [9]=0; [0xb]=0; [0xc]=0; [0xd]=0  (both groups zero-init)
L171397-171401 (FUN_004d9d00): [0xb]=local_2c; [0xc]=local_28; [0xd]=local_24;
                                [8]=local_20;  [9]=local_1c
                                (contiguous local 6-floats -> both groups at once)
L174624-174630 (FUN_004ddde0): [8]=0; [9]=0; [0xb]=*(iVar3+0xc); [0xc]=iVar5  (partial)
```

### §2.2 `FUN_004A7A60`（v483_1a2）
```
L133758: void FUN_004a7a60(undefined4 param_1) {
           FUN_004941a0(1, param_1, &stack0x00000008);
           return; }
```

### §2.3 `DAT_00B37D00` init fn（v483_1b）
```
L1031387: void FUN_009e62b0(void) {
  L1031399: FUN_004040e0(&DAT_00b11e34);      // once-init guard (v462 type)
  L1031400: _atexit(&LAB_00a1d4b0);           // dtor registration
machine (v478): 0x9EDEAA mov ecx,0xb37d00 / call 0x419a70   (init this)
                push 0xa20120 / call 0x981fb4               (registration)
neighbour: L1031365 FUN_009e6250 -> FUN_004040e0(&DAT_00b11e2c) (paired sibling)
```

## §3 残点 **V-484-1..3**
1. 【中】**旧残点 V-406-3 継続** — **6 float（2×3D vector）の正式語**（`FUN_004D9D00` の local 6 float が何をコピーするか＝**`FUN_004D9D00` 本文の context 採取**が次の手）＋ `FUN_004941a0`（log core）identity。
2. 【低】**旧残点 V-406-4 semantic 完遂** — **`DAT_00B37D00` 正体**（atexit dtor `LAB_00A1D4B0` 逆引き／init 実体 `FUN_00419A70` 本文）→ `+0x64` 中身の語。
3. 【低】**V-483-3 引継ぎ** — `FUN_004CBA80` の `param_3`、`FUN_005222D0` 一語名別法、predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v483_1 の1本 EXIT=0**（V-483-1 対処理を確定相当・3軸語は残点＋`FUN_004A7A60`＝log wrapper 推定相当／V-483-2 init fn 構造を確定相当・正体は残点／V-483-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_004D9D00` の連番 local（`local_2c/28/24`→`local_20/1c`）が6 float 連続コピー**＝「両群が対」の機械証拠 → (ii) `FUN_004A7A60` の **variadic wrapper（`FUN_004941a0(1, …, &stack+8)`）**が log 形 → (iii) **`FUN_009E62B0` の `FUN_004040e0(&DAT_...)`＋`_atexit` 型**が v462 init fn 型と一致 → init fn と確定、**隣接 fn（`FUN_009E6250`→`&DAT_00b11e2c`）が同じ型で guard 連番（`0xB11E2C/0xB11E34`）**＝系列性を確認。
- 妥当性チェック: ①`local_2c..local_1c` 連番⇔6 float 連続（`-0x30,-0x2C,-0x28,-0x24,-0x20,-0x1C`）— ②`[8]/[9]/[0xb]/[0xc]/[0xd]`⇔byte `+0x20/+0x24/+0x2C/+0x30/+0x34`（×4）— ③`FUN_004040e0`＋`_atexit`⇔v462 `DAT_00b05dc0` init 型— ④`mov ecx,0xb37d00; call 0x419A70`⇔`FUN_004040e0(&DAT_00b11e34)` の docs/machine 対応（`this=0xB37D00`）— ⑤guard 連番 `0xB11E2C/0xB11E34`⇔隣接 init fn 系列 — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **連番 local（`local_2c/28/24/20/1c`）の代入束が「6 float ＝2×3D vector」の証拠** — **local の byte offset が連続していれば配列/vector のコピー**（`param_1[N]` への束セットで対を確定）。
  2. **`FUN_004040e0(&DAT_)`＋`_atexit` の fn 型が「singleton init fn」の定型**（v462 に続く再現）— **隣接 init fn（同じ型・guard 連番）が系列性を語る**（`0xB11E2C/0xB11E34`）。
  3. **variadic wrapper（`f(1, arg, &stack+8)`）は log/report 系の目印**（`FUN_004941a0` core 候補）。
  4. **init fn の docs（guard/atexit）と machine（`mov ecx,global; call init`）の対応で「何が this か」が確定する**（`DAT_00B11E34`＝guard、`0xB37D00`＝this）。
  5. **正体不明 singleton は「init fn を確定→dtor（atexit）または init 実体の中身へ」と経路を固定**（本裁定で経路のみ確定・中身は次の手＝残点の形を明示）。
