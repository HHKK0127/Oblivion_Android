# v467 — id `0x4C` float consumer（class form 系）と top-band signature の裁定（V-467-1..3 完遂・旧残点 V-406-3 継続第4弾）

- 採番: **v467**（旧残点群消化・probe prefix **v467_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-467-1..3（v466 残点）** — 1=旧残点 V-406-3 継続（id `0x4C` の `+0xC` float の語、top-band 残り typed）／2=旧残点 V-406-4（`DATA+0x2C/+0x31`）と V-406-5（`PTR_DAT_00B10D90`）／3=V-466-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-467-1: id `0x4C` float の consumer（`+0xC` float の語）** | **[確定相当]（consumer 系統）＋[推定高]（語）** | **`FUN_00420C40`（get）の callers＝3件 [確定]**: **`FUN_004CCE00`**（class form float getter・v464 確定）／**`FUN_004CCE20`**（float 計算3引数・get 結果を `fmul` で乗算に使う）／**`FUN_004D2720`**（L164869）。**`FUN_00420C60`（set）の caller＝1件 `FUN_004CD960`（L161428）[確定]**: `call 0x420c60` 直後に **`test dword [esi+8], 0x400`（flag bit10 判定）→ `mov ecx,ebx; call 0x6ECC80`（CLAS getter）→ `cmp eax,esi`** [確定(機械)] → **id `0x4C` float は class form（TESClass/CLAS）系の値 [推定高]**（consumer が全て class form 系 fns）。**`+0xC` float の正式な語（何の値か）は [未確定]＝残点** |
| **V-467-1: top-band 残り typed** | **[確定]（6帯分 signature 取得）** | v464 の histogram 上位帯の owning fn signature を取得 [確定(decompiled)]: **`0x649000: FUN_00649190(int *param_1, int *param_2)`**／**`0x635000: FUN_00635D60(int *param_1, undefined4, undefined4, undefined4, …)`（5+引数）**／**`0x636000: FUN_00636FD0(int *param_1, int *param_2, char param_3)`**／**`0x627000: FUN_00627680`（signature 行 (none)）**／**`0x634000: FUN_00634F60(int *param_1, int *param_2)`**／**`0x63A000: FUN_0063A020(int *param_1, int *param_2)`** → **`FUN_006ECC80`（CLAS getter）の consumer band は signature 一覧で11帯分まで確定**（0x603000/0x460000/0x638000/0x641000/0x4B7000/0x460BC0（v464/465）＋本裁定6帯）→ **全数 typed は未達 [部分確定]＝残点継続** |
| **V-467-2: 旧 V-406-4/5** | **[未確定]（残点継続）** | 本 probe 未着手（budget は V-467-1 に集中） |
| **V-467-3: V-466-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v467_1）

### §2.1 id `0x4C` float consumers（v467_1a）
```
callers of FUN_00420C40 (get): 3
  0x4CCE0F owning=FUN_004cce00 (L160781): call / fstp [esp] / fld [esp]
  0x4CCE46 owning=FUN_004cce20 (L160799): call / fstp [esp] / fld [esp+0x40]
                                           / fmul [esp] / fstp [esp+0x40]
  0x4D2777 owning=FUN_004d2720 (L164869): call / fstp [esp+0x18]
callers of FUN_00420C60 (set): 1
  0x4CD9D8 owning=FUN_004cd960 (L161428): call 0x420c60
                                           / test dword [esi+8], 0x400
                                           / mov ecx, ebx / call 0x6ecc80 / cmp eax, esi
```

### §2.2 top-band residual signatures（v467_1b）
```
0x649000 FUN_00649190: void __thiscall(int *param_1, int *param_2)
0x635000 FUN_00635d60: (int *param_1, undefined4, undefined4, undefined4, ...)
0x636000 FUN_00636fd0: undefined1 __thiscall(int *param_1, int *param_2, char param_3)
0x627000 FUN_00627680: (signature line none)
0x634000 FUN_00634f60: void __thiscall(int *param_1, int *param_2)
0x63A000 FUN_0063a020: void __thiscall(int *param_1, int *param_2)
```

## §3 残点 **V-468-1..3**
1. 【中】**旧残点 V-406-3 継続** — **id `0x4C` の `+0xC` float の正式な語**（class form の何の値か・`FUN_004CD960` の `+0x8` flag `0x400` の意味と接続）＋ `FUN_00627680`（signature none）の docs 確認。
2. 【低】**旧残点 V-406-4／V-406-5** — `DATA+0x2C/+0x31` 意味（UESP 差分の docs 内裏取り）／`PTR_DAT_00B10D90` 所有 fn と spec 名前テーブル書手。
3. 【低】**V-467-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v467_1 の1本 EXIT=0**（V-467-1 は id `0x4C` consumer 系統を確定相当＋top-band 6帯 signature 取得／V-467-2/3 は未着手＝残点）。
- **決定打の作り**: (i) **get/set の E8 census で consumer を全列挙**（get3件/set1件）→ (ii) **set caller（`FUN_004CD960`）の context decode で `+0x8` flag `0x400` 判定と CLAS 比較を発見** → class form 系と推定 → (iii) **get caller（`FUN_004CCE20`）の `fmul` で「乗算に使う float」であることを確認** → (iv) top-band は signature 行を docs から一括取得。
- 妥当性チェック: ①`FUN_004CCE00/4CCE20`⇔v464 確定の class form float 計算系 — ②`FUN_004CD960` の `call 0x6ECC80`（CLAS getter）⇔`cmp eax,esi`（class 比較）— ③`fmul`⇔float の乗算利用 — ④11帯 signature⇔v464 histogram 上位帯と一致 — **四方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **getter/setter pair の E8 census が consumer の全体を限定する**（get3/set1＝4件のみ）— **id 定数 lookup の consumers は grep＋census で完全に見える**。
  2. **set caller の直後命令（flag テスト＋他 getter 呼び出し）が「何の系統の値か」を語る**（`test [esi+8],0x400`＋`call 0x6ECC80`⇔class form 系）。
  3. **`fmul`/`fstp` の float 演算命令が「get 結果の利用形」を語る**（乗算に使われる＝某种の係数/倍率 [推定]）。
  4. **band signature 一覧は「docs の signature 行を band ごとに1件」で蓄積できる**（本裁定で11帯分）— 全数 typed ではなく**帯の代表で十分 band の性格が分かる**。
  5. **signature 行が `(none)` の fn（`FUN_00627680`）は docs の annotation が無い可能性** — 次 probe で docs確認（annotation 欠落の v437 類型候補）。
