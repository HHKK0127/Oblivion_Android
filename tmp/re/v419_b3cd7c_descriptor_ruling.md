# v419 — `0xB3CD7C` singleton の中身・初期化経路・設計意図の裁定と `0x6D0450`（dump virtual）の同定（V-419-1／V-419-2・派生残点第11弾）

- 採番: **v419**（派生残点群・probe prefix **v419_1..v419_7 の7本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v418 の残点 **V-419-1【中】（`0xB3CD7C` の中身＝実行時文字列と設計意図・初期化経路）** と **V-419-2【低】（`0x6D0450` identity）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-419-1(a) `0xB3CD7C` の中身** | **[確定]** | **2フィールド pair オブジェクト**: **`[0xB3CD7C]`（dword）= `0xA7913C` ＝ char* `"NiMultiTargetTransformController"`**（.rdata 文字列実読）／**`[0xB3CD80]` = `0xB3CDF8` ＝ 次の同型 pair へのリンク** — init は `FUN_0070E220(this, a, b){ *[this]=a; [this+4]=b; }`（L552053-552058・2行 pair ctor） |
| **V-419-1(b) 初期化経路** | **[確定]** | **file 初期値なし（BSS）**: `0xB3CD7C` は .data の rel `0x3AD7C ≥ rawsz 0x30C00` で **非ファイルバック**・`.reloc` サイズ **0（stripped）** → **実行時のみ賦流**。**static initializer 表 `.rdata 0xA2BC5C`**（連続 fn ptr 配列・`0xA02DF0,0xA02E00,…,0xA02E80,0xA02EA0,…` stride 0x10 帯）の **エントリ `0xA02E80`** が実行する thunk: **`push 0xB3CDF8; push 0xA7913C("NiMultiTargetTransformController"); mov ecx,0xB3CD7C; call 0x70E220; ret`** — E8 callers 0 件＝**当該テーブル経由のみ** |
| **V-419-1(c) 設計意図（リンク先とチェーン）** | **[確定(実読)]（3段）＋[推定高]（意味）** | **リンク先 `0xB3CDF8` も同型 pair（別 initializer thunk `0xA0300A`）**: `{[0]=0xA7A224="NiInterpController", [4]=0xB3FC98}` → **3段目 `0xB3FC98`（thunk `0xA09D9A`）**: `{[0]=0xA7EC1C="NiTimeController", [4]=0xB3F684}`。→ **name 連鎖 = `NiMultiTargetTransformController → NiInterpController → NiTimeController → …` ＝クラス階層（extends）チェーン [推定高]**（各段 name 実読は [確定]）。**`&0xB3CD7C` が map key（type identity）＝「NiMultiTargetTransformController 型である識別子」**（v416/v417 の等値比較・lookup 経路と整合）・**`[0xB3CD7C]`（char* name）は dump ヘッダ `--- %s ---` の入力**（v418 確定の `FUN_0070F910`）＝**Oblivion 独自の実行時 RTTI ディスクリプタ [推定高]** |
| **V-419-2 `0x6D0450` の identity** | **[確定(RTTI)＋命令]** | **vtable `0xA79984`（`.?AVNiMultiTargetTransformController@@`）の slot12（+0x30）＝dump 系 virtual**（data ref `.rdata 0xA799B4` のみ・E8=0）。本体（thiscall: ebx=this, stack1=テキスト buffer オブジェクト）: ①`FUN_006D05C0(this, buffer)`（役割は残点）②**`FUN_0070F910([0xB3CD7C])` → `"--- NiMultiTargetTransformController ---"` ヘッダ生成 → buffer へ insert**（`[buf+8]`/`[buf+0xA]` size/cap 比較 → 超過時 `FUN_004E4A10` grow → `FUN_0042BB90` insert・L553023 帯 machine 0x6D0450-0x6D04C8）③**`FUN_0070FAA0("m_usNumInterps", [this+0x44])` → `"%s = %hu"` 行を buffer へ** |
| **副次（MTTC layout）** | **[確定]** | **`[NiMultiTargetTransformController +0x44]` = `m_usNumInterps`（u16）** — dump が `[ebx+0x44]` を `%hu` で出力（`0xA7A150="m_usNumInterps"`・`FUN_0070FAA0`＝`"%s = %hu"` 生成・L553133-553147） |
| **直書き書手の否定** | **[確定(否定側)]** | `0xB3CD7C` の **A3/C7 05/89 xx メモリ書手（modrm=05・SIB=25 含む）は .text に0件** — occurrence 全13件の prev バイトは `3D(cmp)/68(push)/B8(mov eax,imm＝thunk)/A1(mov eax,[...])／B9(mov ecx,imm＝initializer thunk)` のみ → **賦流は initializer thunk の他なし** |

## §2 証拠（v419_1..v419_7）

### §2.1 initializer bank（v419_3(v)/v419_4(v)/v419_5）
```
.rdata 0xA2BC5C（表エントリ）:
  -36:0xA02DF0 -32:0xA02E00 -28:0xA02E10 -24:0xA02E20 -20:0xA02E30
  -16:0xA02E40 -12:0xA02E50  -8:0xA02E60  -4:0xA02E70  +0:0xA02E80 ★
  +4:0xA02EA0  +8:0xA02EB0 +12:0xA02EC0 +16:0xA02ED0 +20:0xA02EE0 …
bank 形態は2種: 「call factory → mov [global],eax → ret」（例 0xA02E00: call 0x6c13b0 → [0xB3CD88]）
と 「push b; push a; mov ecx,global; call 0x70E220; ret」（pair init）／「mov ecx,global; jmp ctor」（0xA02E70 等）。
★0xA02E80: push 0xB3CDF8 / push 0xA7913C / mov ecx,0xB3CD7C / call 0x70E220 / ret
```

### §2.2 pair ctor と3段チェーン（v419_4/v419_6/v419_7）
- `FUN_0070e220(this,a,b){ *this=a; this[1]=b; }`（L552053-552058）。
- 段1: `0xA7913C = "NiMultiTargetTransformController"`（bytes 実読 `4e 69 4d 75 6c 74 69…`）。
- 段2 thunk `0xA0300A`: `push 0xB3FC98; push 0xA7A224("NiInterpController"); mov ecx,0xB3CDF8; call 0x70E220; ret`。
- 段3 thunk `0xA09D9A`: `push 0xB3F684; push 0xA7EC1C("NiTimeController"); mov ecx,0xB3FC98; call 0x70E220; ret`。
- 同型 pair init thunk の銀行: `mov ecx, <0xB3CD00..0xB3CE40>`＝**8件**（0xB3CD40/0xB3CD1C/0xB3CDA0/0xB3CD7C/0xB3CE00/0xB3CDF8/0xB3CE1A/0xB3CE30）＋他帯（0xA09D9A 等）→ **群としての実行時 type descriptor bank [確定(命令)]**。

### §2.3 dump virtual（v419_1(b)/v419_5(v)/v419_7）
```
6D0450: push ebx/esi; mov esi,[esp+0xc]; mov ebx,ecx(=this); call 0x6D05C0(this,esi)
6D045F: mov eax,[0xB3CD7C]; push eax; call 0x70F910      ; "--- NiMultiTargetTransformController ---"
        (buf 文字列 grow: [esi+0xA] vs [esi+8] → call 0x4E4A10 → insert 0x42BB90)
6D0498: movzx ecx,[ebx+0x44]; push ecx; push 0xA7A150("m_usNumInterps"); call 0x70FAA0
        → FUN_0070faa0(s,u16){ "%s = %hu" 生成 }（L553133-553147）→ 同様に buffer へ insert
```
- vtable 判定: `.rdata 0xA799B4` RTTI backward scan → **vtable 0xA79984 slot12（+0x30）RTTI `.?AVNiMultiTargetTransformController@@`**（v419_2(c)）。

### §2.4 BSS/reloc 判定（v419_2(a)/v419_3(a)(b)）
- `.data sva=0x702000 vsize=0xA9C1C raw=0x700C00 rawsz=0x30C00` → `0xB3CD7C` rel `0x3AD7C ≥ rawsz` → **file 非バック（BSS 相当）**＝file 残骸値 `0xC3FD4D24` は**非意味値（読まないこと）**。
- `.reloc rva=0 size=0` → loader fixup もなし → **初期化は initializer bank のみ [確定]**。

## §3 残点 **V-420-1..4**
1. 【低】チェーン4段目 `0xB3F684` の name と終端、＋**「extends（親クラス）チェーン」説の最終裏取り**（全段 name を実データのクラス階層と突合。MTTC→NiInterpController は Gamebryo 常識では NiTransformController が中間に入る想定と齟齬 → **EXE 根拠のみで語義確定させる**）。
2. 【低】initializer 表 `.rdata 0xA2BC5C` の全体構造（エントリ総数・表頭・これを走査する起動 fn＝CRT/独自 startup のどちらか）。
3. 【低】`0x6D0450` 内 `FUN_006D05C0(this, buffer)` の役割（dump の前処理か）。
4. 【低】V-419-3 引継ぎ: V-418-2/V-416-2/V-415-2（`L498845` slot23 と `manager[+0x30]` 正体）。

## §4 検証・自己批評
- probe: **v419_1..v419_7 の7本全 EXIT=0**（V-419-1 完遂＋V-419-2 完遂）。
- **決定打の作り**: (i) **[disp32] 直書き census を「occurrence の prev バイト」で機械分類**（modrm=05／SIB=25 が0件＝直書き不在証明）→ (ii) 書手不在なら **BSS/backed 判定（rel vs rawsz）と .reloc 解析**で「実行時賦流」を確定 → (iii) 残る `mov ecx,imm` ヒントを **線形 decode で initializer thunk として復元** → (iv) **push された imm の文字列実読**で pair の意味を確定 → (v) 同型 thunk の銀行 census で一般化 → (vi) vtable data ref の RTTI backward scan で dump virtual 同定。
- 妥当性チェック: ①`FUN_0070E220`（2行 init）⇔thunk の push2/mov ecx⇔`.rdata` 表エントリ — ②3段 name（実読）⇔各段の initializer thunk — ③`[0xB3CD7C]` 値＝name char* ⇔ `FUN_0070F910`（char* arg・v418）⇔ dump ヘッダ — ④vtable slot12⇔`m_usNumInterps` フォーマット ⇔ MTTC — **四方向矛盾なし**。
- docs との不整合: **なし**（v418 の「singleton 文字列のアドレスが key」仮説を**訂正強化**: 実体は **pair ディスクリプタ（name + リンク）で、`&pair` が key・`[pair]` は name**（「char* 変数のアドレス」ではなく「ペア構造体のアドレス」）。docs/GT 反映時は v417/v418 の該当表記を本裁定に合わせる・#47/#20 一括）。
- **教訓**:
  1. **.data の「rel ≥ rawsz（非ファイルバック）」確認をせずに file 値を初期値と読むと偽の手がかりを得る**（本件 `0xC3FD4D24`）— BSS 判定＋`.reloc` 解析をセットで行う。
  2. **`[disp32]` 書手不在の否定は「occurrence の prev バイト census」で機械的**（modrm=05／SIB=25 が0件＝メモリ書手ゼロの証明・opcode 列挙より網羅的）。
  3. **書手ゼロなら「initializer bank（`mov ecx,global; call/push 群` thunk ＋ .rdata fn ptr 表）」を先に疑え** — 静的オブジェクトの賦流経路はここに集中する。
  4. **`push imm32` の imm を実読（str_at）すれば対象の意味が一撃で決まる** — pair init thunk は push 2本が全て情報源。
