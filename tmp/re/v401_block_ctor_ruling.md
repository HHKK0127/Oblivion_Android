# v401 — `0x6C62E0`/`0x6C6300` の所属と `0xA79EFc` 定数の裁定（#153 ⑥-7・V-386-2）

- 採番: **v401**（ai-todo #153 束の第12弾＝低10件の第7弾、probe prefix v401_1..v401_4）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v386 の残点 **V-386-2（`0x6C62E0`/`0x6C6300` の所属 — block 構造の ctor/reset 系 [推定]・`0xA79EFc` 定数との関係）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内定数・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0x6C62E0` の所属** | **[確定(命令＋CRT 名)]** | **block 構造（stride16）の要素 ctor（initializer）** — **MSVC CRT `` `eh_vector_constructor_iterator' ``（`0x984BCF`）に ctor callback として渡される**（5箇所・例 `0x6C7155: push 0x6c64c0; push 0x6c62e0; push count; push 0x10; push buf; call 0x984bcf`）。CRT 注記: `void __stdcall 'eh vector constructor iterator'(void*, uint, int, ctor(__thiscall*), dtor(__thiscall*))`（**Visual Studio 2005/2008/2010 Release**・L987478-987490）＝**thiscall(this=要素スロット)**。**E8=0 の正体＝callback 経由のみ**（Ghidra も `FUN_006c62e0` を未アノテーション＝未分析）。中身: `{+0/+4/+8 zero, +0xC←[0xA79EFc], +0xD←0xFF}` のゼロ＋センチネル初期化 |
| **`0x6C6300` の所属** | **[確定(命令＋decompiled)]** | **block slot の release/reset** — `[+0]`/`[+4]` の2つの refcounted ptr を release（`Interlocked` via `[0xA2807C]`→0 なら delete）→ `[+0]=0, [+4]=0, [+8]=0` → **`[+0xC]←[0xA79EFc]`・`[+0xD]←0xFF`**（L495959-495984 逐語）。**E8 caller＝`0x6C6675` の1件＝`FUN_006c6610`（block 抽出 fn・v400 確定）内**＝「取り出し後に slot をクリア」の一部 |
| **ペア dtor** | **[確定(命令)]** | **`0x6C64C0`＝block 要素 dtor** — CRT iterator の **param_5** として全構築 site で渡される。SEH prologue→`this=ecx`→**`[+4]` ref release→`[+0]` ref release**（ctor `0x6C62E0` のゼロ化と対） |
| **`0xA79EFc` 定数との関係** | **[確定(命令)]** | **値＝`0x000000FF`（byte 0xFF）の「未設定センチネル」バイト定数**。用途3系統: ①**両 resetter の `+0xC` 既定値**（`mov cl,[0xA79EFc]; mov [reg+0xc],cl`＝`0x6C62EE`/`0x6C635E`）②**「未設定」比較**（`cmp al,[0xA79EFc]`＝`0x53D89E`/`0x74FADE`/`0x750335`）③**配列側 init**（`0x49F831`＝`[edx+ebp+0xc]←[0xA79EFc]`／`0x6CAFB4`＝`[esi+0xc]←[0xA79EFc]`）。**`+0xC` と `+0xD` は両方 0xFF 既定**（v400 の `DAT_00a79efc`=0xFF と整合） |
| 構築を呼ぶ container fn | **[確定(命令)]** | `[+0x14]`＝block 配列（`0x6C7173: mov [esi+0x14], data`）を持つ container を **`FUN_006c70a0`（copy/assign 系・`__thiscall (param_1,param_2,param_3)`・L496825）** と **`FUN_006c8580`（grow 系・`__fastcall`・count=`[edi+0xc]+[edi+0x10]`・L497902）** が構築時に CRT iterator でブロック配列を初期化 |

**V-386-2 への回答**: **`0x6C62E0`＝block 要素 ctor（CRT iterator callback）／`0x6C6300`＝block slot release/reset、共に block 構造の ctor/reset 系 [確定]（v386 の [推定] を昇格）**。**`0xA79EFc`＝値 0xFF のセンチネル定数で `+0xC` 既定値・未設定比較・配列 init の3用途 [確定]**。

---

## §2 証拠

### §2.1 `0x6C62E0` 本体（v401_1(a) unfiltered）

```
6C62E0: eax = ecx; ecx = 0
6C62E4: [eax]=0; [eax+4]=0; [eax+8]=0          ; 3 dword zero（2 ref ptr ＋ ptr）
6C62EC: cl = [0xa79efc]; [eax+0xc] = cl         ; ★+0xC ← センチネル(=0xFF)
6C62F5: byte [eax+0xd] = 0xff                   ; ★+0xD ← 0xFF
6C62F9: ret
```
- E8 callers＝**0件**（v386 再掲）→ **callback 経由**で下記5箇所から使用。

### §2.2 CRT iterator 呼び出し（v401_2(a)/v401_3 整列 decode）

```
（site1: fn FUN_006c70a0 内 0x6C7155..0x6C7168）
6C711E: edi = [ebp+0xc]（count）
6C7122..6C713A: size = count*0x10 + 4（overflow check）
6C713D: call 0x401F00（new）→ [eax] = count（header）
6C7155: push 0x6c64c0        ; ★param_5 = dtor
6C715A: push 0x6c62e0        ; ★param_4 = ctor
6C715F: push edi             ; count
6C7160: lea ebx, [eax+4]; push 0x10; push ebx  ; size=16, buf
6C7168: call 0x984bcf        ; ★`eh_vector_constructor_iterator`
6C7173: mov [esi+0x14], eax  ; ★container[+0x14] ← block 配列
```
- **site3（fn `FUN_006c8580` 内 0x6C8604..0x6C8617）も同一5引数パターン**（＋残2 site: `0x6C8F08`/`0x6C9120`）。
- CRT 本体（v401_4(c)）: **`984BCF: …` decompiled 注記 L987478 `// \`eh_vector_constructor_iterator' @ 0x984bcf`**・シグネチャ `(void*, uint, int, ctor(__thiscall*), dtor(__thiscall*))`・ループ `call [ebp+0x14]`（ctor・ecx=要素）→ `add esi, stride`。`push 0x10; push 0xAFFA78; call 0x98C5A0` は **`__SEH_prolog4`（0x98C5A0 注記確認）**＝prologue で意味なし。

### §2.3 `0x6C6300` 本体（v401_1(a) ＋ v400 decompiled L495959-495984）

```
6C630B: esi = [edi]; release([esi+4]) → 0 なら delete → [edi]=0
6C632F: 同様に [edi+4] を release → [edi+4]=0
6C6355: [edi+8] = 0
6C635C: cl = [0xa79efc]; [edi+0xc] = cl        ; ★+0xC ← センチネル
6C6365: byte [edi+0xd] = 0xff                  ; ★+0xD ← 0xFF
6C636C: ret
```

### §2.4 dtor ペア `0x6C64C0`（v401_4(a)）

```
6C64E4: edi = ecx（this=要素）
6C64EA: esi = [edi+4]; release([esi+4]) → …    ; ★+4 ref release
6C6517: esi = [edi]  → 同様に release           ; ★+0 ref release（以降 cut）
```

### §2.5 `0xA79EFc` の値と全用途（v401_1(c)/v401_2）

- **値: `dword=0x000000FF`（byte 0xFF）**。
- 参照: `0x6C62EE`/`0x6C635E`（両 resetter の +0xC 既定）／`0x49F831`・`0x6CAFB4`（配列 init で `+0xC←[0xA79EFc]`）／`0x53D89E`・`0x74FADE`・`0x750335`（`cmp al,[0xA79EFc]`＝未設定判定）。

## §3 残点 **V-401-1..3**
1. **`FUN_006c70a0`/`FUN_006c8580` の所属クラスの確定**（`[+0x14]` block 配列を持つ container の RTTI 照合 — manager/sequence [推定]）【中】
2. **残り2構築 site（`0x6C8F08`/`0x6C9120`）の owner fn 同定**【低】
3. **dtor 側 CRT 名（`eh vector destructor iterator`）の対応確認と `0xAFFA78`（SEH static）の役割**【低】

## §4 検証・自己批評
- probe: **v401_1..v401_4 全 EXIT=0**。
- **決定打の作り**: (i) 両 fn の unfiltered 全文 → (ii) **`0xA79EFc` の実値（0xFF）取得** → (iii) **packed dword census（E8=0 の正体＝callback）→ 整列 decode で5引数パターン確定** → (iv) **CRT 注記 `` `eh_vector_constructor_iterator' `` で ctor/dtor callback を確定** → (v) dtor 本体でペア確認。
- 妥当性チェック: ①ctor zero⇔dtor release の対 — ②`+0xC←[0xA79EFc]`⇔実値0xFF⇔v400 の `DAT_00a79efc` — ③stride 0x10⇔v386 の block stride16 — ④`thiscall(ctor)`⇔`mov eax,ecx` — ⑤`[+0x14]`⇔FUN_006c6610/66b0 の block 配列 — **五方向で矛盾なし**。
- docs との不整合: **なし**（v386 の V-386-2 を解消。[推定]→[確定] 昇格）。
- **教訓**: **E8=0 の fn は「packed dword census（imm32 参照）」で callback 呼び出しを特定せよ**（Ghidra 未アノテーションでも機械語の `push imm32` が唯一の手がかり）＋**CRT（`eh_vector_*`）の iterator 呼び出しは (buf, size, count, ctor, dtor) の5引数定石**。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
