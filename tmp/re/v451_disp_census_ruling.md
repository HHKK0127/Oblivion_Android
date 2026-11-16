# v451 — `[reg+0x1C]` 形式別 census 完了・AV special path・sub-call identity の裁定（V-451-1..3 完遂・旧残点群移行第5弾）

- 採番: **v451**（旧残点群移行・probe prefix **v451_1..v451_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-451-1..3（v450 の残点）** — 1=`Player[+0x1C]` reg-src/pointer 経由書手と `+0x32 s16` の語／2=`FUN_005E2210`（Player AV special path 実体）と `DAT_00b33b00`（LoadGame context）identity／3=`0x523F40` の各 sub-call identity 一覧を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-451-1: `[reg+0x1C]` 形式別 census** | **[確定]（形式完了）＋[未確定]（書手特定）** | **(i) disp32（imm/reg/lea）＝0 hits（v450/v451a）**／**(ii) disp8 dword store（`89 4X/4C.. 1C`＋SIB 8形式）＝239件（no-SIB）＋SIB 追加で合計1602件 [確定]**／**(iii) disp8 byte store（`88/C6`）＝32件**／**(iv) disp8 lea（`8D 4X 1C`）＝42件** → **`+0x1C` は全 .text で頻出 field（313〜1602件規模）＝多数 class が持つ ptr field [確定]**。**class 選別（owning fn の vtable RTTI が record/actor class 系）＝2件のみ: `FUN_00437F00`（`.?AVQueuedCharacter@@`）／`FUN_00437FE0`（`.?AVQueuedCreature@@`）[確定（この手法）]** → **`Player[+0x1C]` の書手 fn は特定未達 [未確定]＝残点**（vtable store が window 外の可能性／SIB base+index 書手／sub-object 経由の3候補） |
| **V-451-1: `+0x32 s16` の書手 census** | **[確定]（否定側）＋[未確定]（語）** | **`mov word [reg+0x32], imm16`（`66 C7 4X 32`）＝全 .text 1件のみ（`FUN_006D5B20`）[確定]** → **imm 書手はほぼ無し＝reg-src が主 [確定]**。`+0x32` の語義（level bonus 供給元のフィールド）は別アプローチが要 [未確定] |
| **V-451-2: `FUN_005E2210`（Player AV special path 実体）** | **[確定(decompiled)]** | L347307: **`void __thiscall FUN_005e2210(int *self, int index, float *out0, float *out1)`: `*out0=0.0; *out1=1.0` → `switch(index)`: `case 8: virtual[self]+0x284(5) → virtual+0x284(0,…) → FUN_00548020(…) → *out0=(float)int`（Health 系 float 取得）／`case 9: virtual+0x288(0x28)` 続行 [確定]** → **AV index 8..11 を Player の virtual call 連鎖（`+0x284/+0x288`）で float 値化する fn [確定]**（v450 の `FUN_005EAD00` 内 Player 分岐の実体） |
| **V-451-2: `DAT_00b33b00` identity** | **[推定高]** | **docs mentions 1018件 [確定]**: `[+0x14]`（ptr・short 群も読む）／`[+0x18]`（flags u32・**bit15/bit31 mask 操作** `&0xffffdfff`/`&0xffff7fff`）／`[+0x80]/[+0x84]`（ptr 群）→ **大型 file/load コンテキスト（`FUN_00467CA0`＝LoadGame 内でも `[+0x14]` を読む＝同物）[推定高]** |
| **V-451-3: `0x523F40` sub-call identity 一覧** | **[確定]（signature）** | **6件とも annotation 実在**: **`FUN_0046F020`（L88345）`void __thiscall(int, int*)`／`FUN_00469E90`（L83673）`void __thiscall(int, undefined4)`／`FUN_00468590`（L82050）`void(void)`／`FUN_0046E4F0`（L87849）`void __thiscall(int, int*)`／`FUN_004676B0`（L81252）`void __thiscall(int, int*)`／`FUN_0046FC10`（L88964）`void __thiscall(int, int*)` [確定(decompiled signature)]** → **sub-obj 単位の reset/load fn 群（`int*`＝stream/param 引数）**。各 subobject の正式名（`+0xC4/+0x44/+0x68/+0xE4/+0x24/+0x54` の対応）は**本裁定で未同定＝残点**（`+0x24`＝TESActorBaseData のみ v448/450 確定） |

## §2 証拠（v451_1..v451_3）

### §2.1 形式別 census（v451_1a/v451_2）
```
disp32: mov dword [reg+0x1c], imm = 0 / ,reg = 0 / lea = 0
disp8 dword store: 89 4X 1C (no-SIB) = 239 ; + SIB 8 forms -> total 1602
disp8 byte store: 88/C6 4X 1C = 32
disp8 lea: 8D 4X 1C = 42
sample owning: FUN_00413130/413FC0/414790/41D1D0/42FA50/... (widely spread)
```

### §2.2 class 絞り込み（v451_3）
```
for each of 1602 hits: owning fn -> vtable store in first 0x160 -> RTTI
matched record/actor classes: 2 only
  writer @0x437F16 owning FUN_00437f00 vt 0xA36DDC .?AVQueuedCharacter@@
  writer @0x437FF6 owning FUN_00437fe0 vt 0xA36E1C .?AVQueuedCreature@@
Player/Character class writer: none in this method [未確定]
mov word [reg+0x32], imm16 census: 1 hit (FUN_006d5b20)
```

### §2.3 FUN_005E2210 / DAT_00b33b00（v451_1b）
```
L347307: void __thiscall FUN_005e2210(int *param_1, int param_2,
             float *param_3, float *param_4) {
  *param_3 = 0.0; *param_4 = 1.0;
  switch(param_2) { case 8: v+0x284(5); v+0x284(0,..); FUN_00548020; *p3=(float);
                    case 9: v+0x288(0x28); ... }
DAT_00b33b00 docs 1018 hits: +0x14 ptr / +0x18 flags(bit15,bit31 masks) / +0x80,+0x84 ptrs
```

### §2.4 sub-call signatures（v451_1c）
```
0x46F020 L88345 void __thiscall(int, int*)   0x469E90 L83673 void __thiscall(int, undefined4)
0x468590 L82050 void(void)                   0x46E4F0 L87849 void __thiscall(int, int*)
0x4676B0 L81252 void __thiscall(int, int*)    0x46FC10 L88964 void __thiscall(int, int*)
```

## §3 残点 **V-452-1..3**
1. 【低】**`Player[+0x1C]` 書手の別ルート** — (i) vtable store が fn window 外の **大きな ctor の linear decode 走査**、(ii) **SIB base+index 形式（`[reg+reg+disp8]`）census**、(iii) **`Player[+0x1C]` の読手側（`mov reg,[Player+0x1c]`）から逆引き**。＋ `+0x32 s16` の語（reg-src 書手の context decode）。
2. 【低】**docs 欠落 fn census の機械化** — 「annotation 無し＋int3×8 境界」2条件を全 .text に適用し docs 欠落 fn 総数を確定（v448/449/450 の3件は個別発見のまま）。
3. 【低】**`0x523F40` の各 subobject（`+0xC4/+0x44/+0x68/+0xE4/+0x54`）の正式名**（RTTI/field 検査）＋ `flag bit3`/`[+0x104]` の意味。

## §4 検証・自己批評
- probe: **v451_1..v451_3 の3本全 EXIT=0**（V-451-1 は形式 census 完了・書手特定は未達＝部分／V-451-2 完遂（special path＋context 推定）／V-451-3 完遂（signature 6件））。
- **決定打の作り**: (i) **形式別 census を「disp32→disp8→SIB」の順に段階拡張**（v450 の disp32-only が穴だった件を v451 で是正・結果 0→1602 件）→ (ii) **1602 件から「owning fn の vtable RTTI」で class 絞り込み**（2件に収束）→ (iii) **`FUN_005E2210` は decompiled switch 本文で identity** → (iv) **sub-call は backward annotation＋signature 行の1手**。
- 妥当性チェック: ①disp32=0⇔disp8=1602（`+0x1C < 0x80` は disp8 常時可用＝encoding 論理）— ②`case 8..11`⇔`FUN_005EAD00` の `index-8<4`（v450）— ③`+0x284/+0x288` virtual⇔`DAT_00b333C4` 側の `+0x284` docs 呼出（v447/v448）と整合 — ④sub-call 6件 signature⇔`int*` stream 引数（LoadGame 文脈）— **四方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`[reg+disp]` census は disp8/disp32/SIB の3形式を必ず揃える** — disp32 のみでは0 hits の誤排他を起こす（本件 0→1602）。**「0 hits」が出たら encoding 形式を疑え**。
  2. **大量 hits の絞り込みは「owning fn の vtable RTTI」が最速の class フィルタ**（本件1602→2件）。ただし **vtable store が fn window 外だと取りこぼす**（次の probe は window 拡大か head decode を追加）。
  3. **docs の backward annotation は signature 行まで取れる** — sub-call identity は annotation＋signature の2行で即記録可能（body 読まない）。
  4. **`imm16 store census` が1件なら imm 書手は非主流と即判断** — reg-src/indirect へ移る（形式消去法の順序を固定する）。
  5. **既知の virtual slot（`+0x284/+0x288`）は docs mentions と machine 呼出の2系統で整合確認**（v447 の Player virtual 群と接続）。
