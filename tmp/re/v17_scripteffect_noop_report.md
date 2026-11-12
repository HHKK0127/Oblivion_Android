# V17 報告書: ScriptEffect 系オペコードは「出荷版で no-op」か

- 対象バイナリ: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (Oblivion.exe v1.2.0416, Steam版)
- 作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
- 作成日: 2026-10-23 / 作成のみ・既存 .md の編集なし
- マッピング: ImageBase=0x400000, .text VA=0x401000.. / PointerToRawData=0x400 → `file_off = 0x400 + (VA - 0x401000)`
- 使用スクリプト (すべて本番実測・新規): `v17_00_selftest.py`, `v17_01_table.py`, `v17_02_disasm.py`, `v17_03*dispatch*.py`, `v17_04_mgef.py`, `v17_05_compare.py` ( `%LOCALAPPDATA%\Temp\opencode\` )

---

## 1. セルフテスト4件（Phase 0 実測）

| # | 検査 | 期待 (GT) | 実測 | 結果 |
|---|---|---|---|---|
| ST1 | `bytes@0x504050` | `518b4c24188b5424` | `518b4c24188b5424` (file_off=0x103450) | **PASS** |
| ST2 | `bytes@0x4FB580` | `578b7908` | `578b7908` (file_off=0xFA980) | **PASS** |
| ST3 | `string@0xA7035C` | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` (file_off=0x66F55C) | **PASS** |
| ST4 | テーブル532件 + opid `0x0015` impl | 532件 / impl==`0x504560` | count=532, idx=21, opid=`0x15`, impl=`0x504560` | **PASS** |

`RESULT: ALL_PASS` → 分析継続。

セクションテーブル実測（PE ヘッダ自前解析）:

| sect | VirtualAddress | VirtualSize | PointerToRawData | RawSize |
|---|---|---|---|---|
| .text | 0x1000 | 0x626C39 | 0x400 | 0x626E00 |
| .rdata | 0x628000 | 0xD98D2 | 0x627200 | 0xD9A00 |
| .data | 0x702000 | 0xA9C1C | 0x700C00 | 0x30C00 |
| .tls / .rsrc / .bind | 0x7AC000 / 0x7AD000 / 0x7AF000 | ... | ... | ... |

テーブル基底 `0xB0AF48` は **.data** 内（file_off=0x709B48）。名称文字列は .rdata。

---

## 2. 4 opid のテーブルエントリ全フィールド（Phase 1 実測、raw hex 付き）

エントリ長 0x28。レイアウトは raw から再導出:
`+0x00 u32 nameptr / +0x04 u32 (0x00A2F7EC=空文字列) / +0x08 u16 opid / +0x0A u16 0 / +0x0C u32 (0x00A2F7EC) / +0x10 u32 flags / +0x14 u32 (argc ではなくポインタ) / +0x18 u32 impl / +0x1C u32 compiler helper / +0x20 u32 式評価関数 / +0x24 u32 引数型フラグ`

| idx | opid | name | +0x10 flags | +0x14 | **+0x18 impl** | **+0x1C helper** | **+0x20 eval** | +0x24 |
|---|---|---|---|---|---|---|---|---|
| 17 | 0x0011 | ScriptEffectStart | 0x00000001 | 0 | 0x005022F0 | **0x004FDE30（既定）** | **0（未設定）** | 0 |
| 18 | 0x0012 | ScriptEffectFinish | 0x00000001 | 0 | 0x00502320 | **0x004FDE30（既定）** | **0（未設定）** | 0 |
| 19 | 0x0013 | ScriptEffectUpdate | 0x00000001 | 0 | 0x0050A120 | **0x004FDE30（既定）** | **0（未設定）** | 0 |
| 349 | 0x10BA | ScriptEffectElapsedSeconds | 0x00000000 | 0 | 0x00502350 | **0x004FDE30（既定）** | **0（未設定）** | 0 |
| 2 | 0x0002 | OnActivate (対照) | 0x00010001 | 0x00B0ABAC | 0x00503F90 | 0x004FDE30 | 0 | 0 |
| 8 | 0x0008 | OnHit (対照) | 0x00010001 | 0x00B0ABAC | 0x00504050 | 0x004FDE30 | 0 | 0 |

raw 40B 例（生バイト証拠）:

```
idx17 (0x0011): b817a500 ecf7a200 11000000 ecf7a200 01000000 00000000 f0225000 30de4f00 00000000 00000000
idx18 (0x0012): a417a500 ecf7a200 12000000 ecf7a200 01000000 00000000 20235000 30de4f00 00000000 00000000
idx19 (0x0013): 9017a500 ecf7a200 13000000 ecf7a200 01000000 00000000 20a15000 30de4f00 00000000 00000000
idx349(0x10BA): 2cf0a400 ecf7a200 ba100000 ecf7a200 00000000 00000000 50235000 30de4f00 00000000 00000000
idx8  (OnHit)  : 4018a500 ecf7a200 08000000 ecf7a200 01000100 acabb000 50405000 30de4f00 00000000 00000000
```

全532件の統計（+0x1C/+0x20 の代替経路有無の裏づけ）:

- `+0x1C` ヒストグラム: `0x4FDE30`=528件, `0x500130`=2件, `0x500150`=1件, `0`=1件 → **ScriptEffect 系4件はすべて既定値 0x4FDE30。実行系が +0x1C に移っている兆候なし。**
- `+0x20` 非ゼロ = **169件**（GT と一致）。ScriptEffect 系4件は 0（未設定）→ **式評価関数経由の別経路なし。**

**訂正（GT「+0x14 = u8 引数数相当（要実測）」）**: +0x14 は u8 ではなく u32 であり、OnHit/OnActivate では `0x00B0ABAC`（.data 内ポインタ、先頭16B = `481aa500 06000000 01000000 3c1aa500` = 引数記述子リストと見られる）が入る。ScriptEffect 系4件は 0。**argc ではない。**

---

## 3. 4 impl の完全逆アセンブリ（Phase 2、int3 / ret+0xCC 終端、上限 0x400）

終端理由はいずれも **`ret` 直後が `0xCC`（ret+0xCC）**。call は **4件とも 0 個**（完全インライン）。

### 3.1 opid 0x0011 ScriptEffectStart — impl 0x5022F0（38B, GT 38B 一致）

raw: `d9ee8b44241885c08b4c241cdd1974138b401085c0740c803800b0017407d9e8dd19c3b001c3`

```
005022f0: d9ee                     fldz
005022f2: 8b442418                 mov      eax, dword ptr [esp + 0x18]   ; arg6 = [context+8]
005022f6: 85c0                     test     eax, eax
005022f8: 8b4c241c                 mov      ecx, dword ptr [esp + 0x1c]   ; arg7 = result(double*)
005022fc: dd19                     fstp     qword ptr [ecx]               ; *result = 0.0 (既定)
005022fe: 7413                     je       0x502313                      ; obj==NULL → al=1, ret
00502300: 8b4010                   mov      eax, dword ptr [eax + 0x10]
00502303: 85c0                     test     eax, eax
00502305: 740c                     je       0x502313                      ; +0x10==NULL → 0.0
00502307: 803800                   cmp      byte ptr [eax], 0             ; state.byte0 (start flag)
0050230a: b001                     mov      al, 1
0050230c: 7407                     je       0x502315                      ; flag==0 → 0.0 のまま
0050230e: d9e8                     fld1
00502310: dd19                     fstp     qword ptr [ecx]               ; *result = 1.0
00502312: c3                       ret
00502313: b001                     mov      al, 1
00502315: c3                       ret                                     ; 次byte=CC → 終端
```

- **戻り値**: 常に `al=1`（成功）。数値はスタック経由（[esp+0x1C] の double* へ `fstp`）。ST0 は `fldz` で積んだ直後に `fstp qword [ecx]` で**必ず退避（捨てるのではなく結果へ書出し）**。`fld1` 経路では 1.0 を書出し。
- **引数**: `[esp+0x18]`（arg6）と `[esp+0x1C]`（arg7=result）を**読んだ上で**2段参照 `obj→+0x10→byte0` を検査し、0.0/1.0 を使い分け → **「引数を読んで何もしない」ではない。**

### 3.2 opid 0x0012 ScriptEffectFinish — impl 0x502320（39B, GT 39B 一致）

raw: `d9ee8b44241885c08b4c241cdd1974148b401085c0740d80780100b0017407d9e8dd19c3b001c3`

```
00502320: d9ee                     fldz
00502322: 8b442418                 mov      eax, dword ptr [esp + 0x18]
00502326: 85c0                     test     eax, eax
00502328: 8b4c241c                 mov      ecx, dword ptr [esp + 0x1c]
0050232c: dd19                     fstp     qword ptr [ecx]               ; *result = 0.0 (既定)
0050232e: 7414                     je       0x502344
00502330: 8b4010                   mov      eax, dword ptr [eax + 0x10]
00502333: 85c0                     test     eax, eax
00502335: 740d                     je       0x502344
00502337: 80780100                 cmp      byte ptr [eax + 1], 0         ; state.byte1 (finish flag)
0050233b: b001                     mov      al, 1
0050233d: 7407                     je       0x502346
0050233f: d9e8                     fld1
00502341: dd19                     fstp     qword ptr [ecx]               ; *result = 1.0
00502343: c3                       ret
00502344: b001                     mov      al, 1
00502346: c3                       ret                                     ; 次byte=CC → 終端
```

- Start と同型。**差分は検査バイトが `[eax]` → `[eax+1]`** にすぎない（+0 の start フラグ / +1 の finish フラグ）。

### 3.3 opid 0x0013 ScriptEffectUpdate — impl 0x50A120（39B, GT 39B 一致）

raw: `d9ee8b44241485c08b4c241cdd1974148d501885d2740d80782900b0017407d9e8dd19c3b001c3`

```
0050a120: d9ee                     fldz
0050a122: 8b442414                 mov      eax, dword ptr [esp + 0x14]   ; arg5 = ebp (=script obj)
0050a126: 85c0                     test     eax, eax
0050a128: 8b4c241c                 mov      ecx, dword ptr [esp + 0x1c]   ; arg7 = result
0050a12c: dd19                     fstp     qword ptr [ecx]               ; *result = 0.0 (既定)
0050a12e: 7414                     je       0x50a144
0050a130: 8d5018                   lea      edx, [eax + 0x18]
0050a133: 85d2                     test     edx, edx                      ; (eax+0x18 の非ゼロ確認 = コンパイラ産)
0050a135: 740d                     je       0x50a144
0050a137: 80782900                 cmp      byte ptr [eax + 0x29], 0      ; obj+0x29 byte (update flag)
0050a13b: b001                     mov      al, 1
0050a13d: 7407                     je       0x50a146
0050a13f: d9e8                     fld1
0050a141: dd19                     fstp     qword ptr [ecx]               ; *result = 1.0
0050a143: c3                       ret
0050a144: b001                     mov      al, 1
0050a146: c3                       ret                                     ; 次byte=CC → 終端
```

- **Start/Finish との相違点**: 読む引数が `[esp+0x18]` → **`[esp+0x14]`（別のスロット）**、検査場所が `[[obj]+0x10]→byte` → **`[obj+0x29]` 直接バイト**。3件で挙動は同一型だが**入力オブジェクトとオフセットが異なる**（個別移植必要）。

### 3.4 opid 0x10BA ScriptEffectElapsedSeconds — impl 0x502350（31B, GT 31B 一致）

raw: `d9ee8b44241885c08b4c241cdd19740c8b401085c07405d94004dd19b001c3`

```
00502350: d9ee                     fldz
00502352: 8b442418                 mov      eax, dword ptr [esp + 0x18]   ; arg6
00502356: 85c0                     test     eax, eax
00502358: 8b4c241c                 mov      ecx, dword ptr [esp + 0x1c]   ; result
0050235c: dd19                     fstp     qword ptr [ecx]               ; *result = 0.0 (既定値)
0050235e: 740c                     je       0x50236c
00502360: 8b4010                   mov      eax, dword ptr [eax + 0x10]
00502363: 85c0                     test     eax, eax
00502365: 7405                     je       0x50236c                      ; NULL → 0.0 のまま
00502367: d94004                   fld      dword ptr [eax + 4]           ; state.f32 +4 = 経過秒
0050236a: dd19                     fstp     qword ptr [ecx]               ; *result = elapsed
0050236c: b001                     mov      al, 1
0050236e: c3                       ret                                     ; 次byte=CC → 終端
```

- **`fldz` は「既定値 0.0 の用意」であり no-op ではない**: 状態が有効なら `fld dword [eax+4]`（float 経過秒）で**実値を上書き**して返す。`al=1` 常時。

### 3.5 4件の共通構造（判定用まとめ）

| opid | 終端 | call 数 | 読む引数 | 既定出力 | 条件付き出力 | al |
|---|---|---|---|---|---|---|
| 0x0011 | ret+0xCC | 0 | [esp+0x18], [esp+0x1C] | 0.0 (`fldz`→`fstp`) | 1.0 if `[[arg6]+0x10]→byte0 != 0` | 常に1 |
| 0x0012 | ret+0xCC | 0 | [esp+0x18], [esp+0x1C] | 0.0 | 1.0 if `[[arg6]+0x10]→byte1 != 0` | 常に1 |
| 0x0013 | ret+0xCC | 0 | [esp+0x14], [esp+0x1C] | 0.0 | 1.0 if `[arg5+0x29] != 0` | 常に1 |
| 0x10BA | ret+0xCC | 0 | [esp+0x18], [esp+0x1C] | 0.0 | `[[arg6]+0x10]→f32+4`（経過秒） | 常に1 |

→ **「`fldz` して即 return の no-op」は誤り。引数デコード（[esp+0x14]/[esp+0x18]/[esp+0x1C]）を行い、条件分岐・2段ポインタ参照・値書出し（fstp）まで行う実装。**

---

## 4. 追加7件（計11件）の本体一覧と分類（Phase 2 実測）

終端は全件 `ret+0xCC`、**call は全件 0**（＝「直接E8ゼロ11件」の前提と一致）。分類はすべて **実処理（完全インライン）/ no-op は 0 件**。

| opid | 名称 | impl | size | raw (先頭…) | 実処理の中身 | 分類 |
|---|---|---|---|---|---|---|
| 0x011A | ToggleGrassUpdate | 0x501A60 | 18B | `803de49ab000000f94c0a2e49ab000b001c3` | `cmp [0xB09AE4],0; sete al; mov [0xB09AE4],al; mov al,1; ret` → **グローバルフラグ反転** | **実処理** |
| 0x0124 | ToggleShadowVolumes | 0x5087A0 | 18B | `b00138053c2fb4000f95c1880d3c2fb400c3` | `mov al,1; cmp [0xB42F3C],al; setne cl; mov [0xB42F3C],cl; ret` → **フラグ反転** | **実処理** |
| 0x015A | ToggleRefractionDebug | 0x508180 | 45B | `803d3e2fb400007421803d842eb400007418833d482fb400027c0f803ddc2cb40000f94c0a2dc2cb400b001c3` | 3条件（`[0xB42F3E]`,`[0xB42E84]`,`dword[0xB42F48]>=2`）ガード付きで `[0xB42CDC]` 反転 | **実処理** |
| 0x0161 | ToggleHDRDebug | 0x503CE0 | 46B | `803d7030b400007412803de431b400000f94c0a2e431b400b001c3803d0c61b40000b0010f94c1880d0c61b400c3` | 分岐2系統（`[0xB43070]` 有無）で `[0xB431E4]` / `[0xB4610C]` を反転 | **実処理** |
| 0x016B | ToggleWaterRadius | 0x50D8C0 | 17B | `803d3c70b00000b0017405a22a52b300c3` | `cmp [0xB0703C],0; mov al,1; je ret; mov [0xB3522A],al(=1); ret` → 条件付き書込み（単純トグルではない） | **実処理** |
| 0x101F | GetButtonPressed | 0x5072B0 | 104B | `8b44240c85c0740b8b4808c1e90ef6c10174088b44241485c0744a...32c0c3` | 引数 [esp+0xC]/[esp+0x14] チェック + `[obj+8]>>0xE &1` ビットテスト + `[0xB09E24]`(0xFF判定) + `fild`→結果書出し + `[0xB09E24]=0xFF`,`[0xB361C8]=0` クリア。失敗時 `fld [0xA3D360]`(0.0定数) 返却、arg 無効時 `xor al,al; ret` | **実処理** |
| 0x10BC | GetPCSleepHours | 0x50BAA0 | 20B | `a1c433b300db80900500008b4c241cb001dd19c3` | `mov eax,[0xB333C4]; fild dword [eax+0x590]; mov ecx,[esp+0x1C]; mov al,1; fstp [ecx]` → **実値（整数→double）を返す** | **実処理** |

**分類内訳（表11件）: no-op = 0 件 / 実処理（call 0 個・完全インライン）= 11 件。**

---

## 5. ディスパッチャ経路の特定（Phase 3）

### 5.1 依頼例 `0x4FF904` の関数先頭（int3 バウンダリ遡り）

- `0x4FF904: 8d8348afb000 lea eax,[ebx+0xb0af48]` の **関数先頭 = `0x4FF710`**（直前 `0x4FF70F` に int3 1バイトのパディング、`0x4FF70A e915ffffff jmp` で前関数終了）。先頭は SEH プロローグ `6a ff 68 296d9b00 64 a1 00000000 50 b8 50110000 e8...`（`sub esp,0x1150` 相当）。
- ただし本関数は **実行時ディスパッチャではない**。内容は (a) イベント名→opid の線形探索（`mov [esp+0x14],0xB0AF4C` @0x4FF803 → +0x28 刻みで `cmp eax,0xB0B424`、2回 strcmp=0x98262D）、(b) `mov ax,word [ebx+ebx+0xB0AF50]` @0x4FF88E（= entry[opid]+8 の **opid 読出し**）、(c) `lea eax,[ebx+0xb0af48]` @0x4FF904 → `cmp byte [eax+0x10],0` @0x4FF918 → エラー報告 call 0x4FCE30、という **コンパイラ側**（イベント名解決・opid 書出し・フラグ検査）である。
- .text 全域の **テーブル領域 [0xB0AF48,0xB10268) に入った imm32 参照 = 実際に指令オペランドとして使われたもの** に絞ると、実行時参照は以下3本のみ（他は非アライズ読みの誤検出）:
  - `0x516a45: 8b8c005cafb000  mov ecx,[eax+eax+0xb0af5c]`（entry+0x14）
  - `0x516a5d: 8b8860afb000    mov ecx,[eax+0xb0af60]`（entry+0x18 = **impl**）
  - `0x516a9f: 80b858afb00000  cmp byte [eax+0xb0af58],0`（entry+0x10 flags）

### 5.2 実行時ディスパッチャ: **関数先頭 `0x5169A0`**（int3 バウンダリ + SEH プロローグで確定）

- 先頭検証: 直前 `0x51699F = c3`（前関数の ret）、`0x5169A0: 6aff 681c739b00 64a100000000 50 81ec3c070000` = `push -1 / push handler / mov eax,fs:[0] / push eax / sub esp,0x73c`（MSVC SEH プロローグ）。エピローグ `0x5172FC: c22400 ret 0x24`、直後 0x517300/0x517320 のジャンプテーブル（埋め込みデータ）、`0x51732F = int3` 終端。
- **関数サイズ 2447B（0x5169A0..0x51732F）、723 命令。**

**(A) イベント系 (opid 0x00–0x1E) の間接呼出し — 生 hex 付き**

```
00516a23: 0fbf0c07                 movsx   ecx, word ptr [edi + eax]        ; コード列から opid 読出
00516a27: 83f91e                   cmp     ecx, 0x1e                       ; opid <= 0x1E ガード
00516a31: 0f8798000000             ja      0x516acf
00516a3e: 8d0489                   lea     eax, [ecx + ecx*4]               ; *5
00516a41: 03c0                     add     eax, eax                         ; *10
00516a43: 03c0                     add     eax, eax                         ; *20
00516a45: 8b8c005cafb000           mov     ecx, dword ptr [eax + eax + 0xb0af5c]  ; entry+0x14
00516a4c: 03c0                     add     eax, eax                         ; eax = opid*0x28
00516a5d: 8b8860afb000             mov     ecx, dword ptr [eax + 0xb0af60]  ; ★ entry+0x18 = impl
00516a9f: 80b858afb00000           cmp     byte ptr [eax + 0xb0af58], 0     ; entry+0x10 flags
00516ad6: 85c9                     test    ecx, ecx                         ; impl NULL?
00516ad8: 7450                     je      0x516b2a
00516ada: 8d442414                 lea     eax, [esp + 0x14]
00516ade: 50                       push    eax                              ; arg8 = &local
00516adf: 8b4608                   mov     eax, dword ptr [esi + 8]
00516ae2: 8d54241c                 lea     edx, [esp + 0x1c]
00516ae6: 52                       push    edx                              ; arg7 = &result (double)
00516ae7: 8b16                     mov     edx, dword ptr [esi]
00516ae9: 50                       push    eax                              ; arg6 = [context+8]
00516aea: 8b442424                 mov     eax, dword ptr [esp + 0x24]
00516aee: 55                       push    ebp                              ; arg5 = script obj
00516aef: 52                       push    edx                              ; arg4 = [context]
00516af0: 53                       push    ebx                              ; arg3
00516af1: 57                       push    edi                              ; arg2 = code base
00516af2: 50                       push    eax                              ; arg1
00516af3: ffd1                     call    ecx                              ; ★★ 間接呼出し (impl)
00516af5: 83c420                   add     esp, 0x20                        ; 8引数
00516af8: 84c0                     test    al, al
00516afc: d9ee                     fldz
00516afe: dc5c2418                 fcomp   qword ptr [esp + 0x18]           ; 結果と0.0比較
```

- **呼出位置 VA = `0x516AF3`（生 hex `ff d1`）**、その前に **8 push（`0x516ADA..0x516AF2`）**、後ろに `add esp,0x20`。ロードは `0x516A5D` の `8b 88 60 af b0 00`。
- ScriptEffectStart/Finish/Update (0x11/0x12/0x13) はこの経路（entry+0x14=0 → flags チェック → `call ecx`）で実行される。

**(B) 汎用系 (opid 0x100–0x182 / 0x1000–0x1170) の間接呼出し — 同一関数内**

```
00517036: 8b842464070000         mov     eax, dword ptr [esp + 0x764]   ; arg2 = opid
0051703d: 50                     push    eax
0051703e: e8ed59feff             call    0x4fca30                        ; GetOpcodeEntry(opid)
00517046: 85c0                   test    eax, eax
00517048: 757f                   jne     0x5170c9                        ; entry != NULL
005170c9: 80781000               cmp     byte ptr [eax + 0x10], 0        ; flags
005170cd: 8b4814                 mov     ecx, dword ptr [eax + 0x14]     ; entry+0x14
...
00517171: 80bc247c07000000       cmp     byte ptr [esp + 0x77c], 0
00517179: 0f8416010000           je      0x517295                        ; → impl 呼出し経路
...
00517299: 8b4818                 mov     ecx, dword ptr [eax + 0x18]     ; ★ entry+0x18 = impl
005172a5: 85c9                   test    ecx, ecx
005172a7: 0f8422f8ffff           je      0x516acf
005172ad: 8d442414               lea     eax, [esp + 0x14]
005172b1: 50                     push    eax                              ; arg8
005172b2: 8b4608                 mov     eax, dword ptr [esi + 8]
005172b5: 8d442430               lea     edx, [esp + 0x30]
005172b9: 52                     push    edx                              ; arg7 = &result
005172ba: 8b16                   mov     edx, dword ptr [esi]
005172bc: 50                     push    eax                              ; arg6
005172bd: 8b442424               mov     eax, dword ptr [esp + 0x24]
005172c1: 55                     push    ebp                              ; arg5
005172c2: 52                     push    edx                              ; arg4
005172c3: 53                     push    ebx                              ; arg3
005172c4: 57                     push    edi                              ; arg2
005172c5: 50                     push    eax                              ; arg1
005172c6: ffd1                   call    ecx                              ; ★★ 間接呼出し (impl)
005172c8: 83c420                 add     esp, 0x20
```

- **呼出位置 VA = `0x5172C6`（生 hex `ff d1`）**、ロード `0x517299: 8b 48 18`。
- `GetOpcodeEntry` = **0x4FCA30**（実測: opid∈[0x100,0x182] → `lea eax,[eax*8+0xb0b420]` で entry31 起点、opid∈[0x1000,0x1170] → `lea eax,[eax*8+0xb0c8c0]` で entry163 起点、範囲外は 0 を返す）。
- **ScriptEffectElapsedSeconds (0x10BA)** と **Toggle 系コンソール (0x011A–0x016B)** はこの経路で実行される（0x10BA は flags=0 → `0x517171` → `[esp+0x77c]==0` のとき `0x517295` へ → impl 呼出し）。

**(C) カーソル実行系: 関数先頭 `0x4FAB20`**（直前 `0x4FAB1A..0x4FAB1F` = int3 6バイト、プロローグ `83 ec 24` + セキュリティ cookie、target 0x4FAD43 を終端なしで覆うことを実測確認）

```
004fad37: 8b06                   mov     eax, dword ptr [esi]        ; コード列カーソル
004fad39: 0fbf0c38               movsx   ecx, word ptr [eax + edi]   ; opid 読出
004fad43: e8e81c0000             call    0x4fca30                    ; GetOpcodeEntry(opid)
004fad7b: 8b5014                 mov     edx, dword ptr [eax + 0x14] ; entry+0x14
004fadb0: 80781000               cmp     byte ptr [eax + 0x10], 0    ; flags
004fadba: 8b4018                 mov     eax, dword ptr [eax + 0x18] ; ★ entry+0x18 = impl
004fadbd: 85c0                   test    eax, eax
004fadc1..004fadd4: 8 push       (esi/ebp/edi/ecx/… : 引数並べ替え)
004fadd5: ffd0                   call    eax                         ; ★★ 間接呼出し (impl)
004fadd7: 83c420                 add     esp, 0x20
```

- **呼出位置 VA = `0x4FADD5`（生 hex `ff d0`）**、ロード `0x4FADBA: 8b 40 18`。
- この関数の呼び出し元は `0x4FB22B / 0x4FB284`（gate 0x4FAE80 内）, `0x5171FC / 0x517246`（汎用ディスパッチのループ）, `0x4F3F3B/0x4F3F65`, `0x51336A/0x5136DD` の計8件。

**直接 E8 / imm32 の不在（裏づけ）**: 4 impl + OnHit/OnActivate/OnLoad への直接 E8 = **0 件**、.text 内の impl 値そのものの imm32 = **0 件**、テーブル基底 `0xB0AF48` を保持する data セクション内の dword スロット = **0 件**。よって **impl への到達は上記3経路の間接呼出しのみ**。

### 5.3 既知候補 0x517300 / 0x517320 / 0x4FAE80 の位置づけ

- `0x517320`/`0x517300` は **関数 0x5169A0 の末尾に埋め込まれた switch テーブル**（`0x516b6f: 0fb68920735100 movzx ecx,byte [ecx+0x517320]` → `0x516b76: ff248d00735100 jmp dword [ecx*4+0x517300]`）。判定値は引数2（opid/statement code）−0x10（範囲 0..0xE）。index map 15B = `00 00 07 07 07 01 02 03 04 05 07 07 00 00 06`、ターゲットは 0x5172D3 / 0x516B7D / 0x516E9F / 0x516FCE / 0x516EBE / 0x517007 / 0x517028 / 0x517036 の8種。**impl を直接持たない**。
- `0x4FAE80`（gate、呼び出し 278 箇所）は独立関数（先頭 `83ec18 837c241c00...`）。OnHit/OnActivate の impl からも内部 call される。**impl のロードは行わない**（impl 呼出しは上記3経路のみ）。

### 5.4 MGEF `SEFF` との関係（Phase 4 — 同一経路か別系統か）

**結論: 別系統（code-level で非結合）だが、データ/イベント面で接続点あり。**

静的証拠:

| 走査 | 結果 |
|---|---|
| 4 impl → MGEF 系 (0x413094 / 0x6A27F0 / 0x6A2CF0 / 0x6A86F0 / 0x6A8220 / 0x6A84D0) の直接 E8 | **0 件**（4 impl は call を一切持たない） |
| MGEF 系 → 4 impl への直接 E8 | **0 件** |
| MGEF 系 → 4 impl の imm32 参照 | **0 件** |
| MGEF 系の活動（逆方向の実在証明） | `0x6A86F0 (SEFF baseTick)` 呼出し **10 箇所**、`0x6A2CF0 (dispatcher)` 2 箇所、`0x6A27F0 (apply)` 2 箇所、`0x6A84D0/0x6A8220` 各 6/3 箇所 |
| `SEFF` コード判定 | `0x6A2879: 81ba9800000053454646  cmp dword [edx+0x98],0x46464553`（'SEFF'）ほか 0x41309A, 0x41427C-0x414293, 0x6A234D, 0x6A2D14 に即値出現 |

**接続点（生 hex）** — 魔法適用側からスクリプトイベント系への跨ぎ:

```
006a285b: 8b450c                 mov     eax, dword ptr [ebp + 0xc]
006a285e: 8b401c                 mov     eax, dword ptr [eax + 0x1c]     ; MGEF 関連ポインタ
006a2861: 6800200000             push    0x2000                          ; イベントマスク bit13
006a2866: 8d4b44                 lea     ecx, [ebx + 0x44]               ; 対象+0x44
006a2869: 51                     push    ecx
006a286a: 50                     push    eax
006a286b: e82097e5ff             call    0x4fbf90                         ; ★イベント emitter（77 呼出し）
006a2870: 83c40c                 add     esp, 0xc
006a2873: 8b450c                 mov     eax, dword ptr [ebp + 0xc]
006a2876: 8b501c                 mov     edx, dword ptr [eax + 0x1c]
006a2879: 81ba9800000053454646   cmp     dword ptr [edx + 0x98], 0x46464553  ; 'SEFF'
006a2883: 750f                   jne     0x6a2894
006a2885: 8b06                   mov     eax, dword ptr [esi]
006a2887: 8b501c                 mov     edx, dword ptr [eax + 0x1c]
006a288c: ffd2                   call    edx                               ; 仮想関数 (slot7)
```

- emitter `0x4FBF90` は全 .text で **77 箇所**から呼ばれる（魔法領域は `0x6A286B` のみ、他はアクター/クエスト/会話系）。
- つまり: **opcode 側 impl は「状態の読取・ゲート」、ScriptEffect の「開始/終了/更新/経過秒」とイベント発火の生成側は MGEF「SEFF」系 + emitter 経路**にある。opcode 側に SEFF 処理は書かれていない（call 0 件がその証明）。

---

## 6. 比較対照: OnHit / OnActivate vs ScriptEffect 系（Phase 5）

| 項目 | OnActivate (0x0002, 0x503F90) | OnHit (0x0008, 0x504050) | ScriptEffectStart (0x0011, 0x5022F0) |
|---|---|---|---|
| size / 終端 | 117B / ret+0xCC | 116B / ret+0xCC | 38B / ret+0xCC |
| 直接 call | **4**: `0x503FCA call 0x4fae80`(gate), `0x503FE1 call 0x4d8260`, `0x503FF2/0x503FFB call 0x4d82e0` | **2**: `0x504083 call 0x4fae80`(gate), `0x5040B0 call 0x4fb5f0`(TestMask) | **0** |
| gate 0x4FAE80 | あり | あり | なし |
| 定型 prologue | `8b4c2418 d9ee ...`（引数読込 + fldz） | `51 8b4c2418 8b542414 ...` | `d9ee 8b442418 ...` |
| 内容 | ゲート呼出し + 通知系ヘルパ 3 回 | ゲート呼出し + マスクテスト + fldz 返却 | 2段参照による 0.0/1.0 返却のみ |

raw 証拠:
- OnHit: `518b4c24188b542414568b742414578b7c24248d442408508b44243057518b4c2420528b54242056505152c744242800000000e8f86dffff83c42084c075045f5e59c385f6d9ee538b5c242cdd1b741d85ff74198b44240c6880000000508bcfe83b75ffff84c07404d9e8dd1b5b5fb0015e59c3`
- OnActivate: `8b4c2418d9ee8b542414568b742410578b7c24248d442424dd1f508b44241c518b4c2430528b54241c508b44241c56515250c744244400000000e8b16effff83c42084c075035f5ec3...`

**差分の意味**: OnHit/OnActivate は「gate を呼んでイベントブロックを駆動する本体」であり、ScriptEffect 系4件は「自身では一切呼ばず、渡された状態を読んで値を返すゲッタ」である。**同じ定型 prologue でも役割が違う。ScriptEffect 系が gate を呼ばない＝no-op という意味ではなく、呼び出し側（0x5169A0 / 0x4FAB20）から間接呼出しされている（§5 実証）。**

---

## 7. 結論と移植への示唆

### 結論: **(B) — 別経路（MGEF SEFF 系）に実装あり。ただし (A) no-op 確定は明確に否定。**

根拠（hex 付き）:

1. **(A) の否定**: 4 impl はいずれも「引数読み → 2段ポインタ参照 → 条件分岐 → double への `fstp` 書出し → `mov al,1`」まで行う。`fldz` は no-op ではなく**既定値 0.0 のセット**であり、状態が有効なら 1.0（Start/Finish/Update）または経過秒 float（Elapsed）で上書きされる（§3.1–3.4）。call 数0は「完全インライン処理」であって「何もしない」ではない。
2. **テーブル経由の別経路なし**: +0x1C は4件とも既定 `0x4FDE30`、+0x20 は4件とも 0、+0x24 は 0（§2, 全532件統計と整合）。
3. **MGEF SEFF 系に実装あり（ScriptEffect の実行系本体）**: SEFF 判定 `cmp dword [edx+0x98],0x46464553` @`0x6A2879`、apply `0x6A27F0`・dispatcher `0x6A2CF0`・tick `0x6A86F0`（各 E8 2/2/10 箇所）が実在し、opcode 側 impl とは **相互に E8/imm32 が一切ない**（§5.4）。開始/終了/更新/経過秒の**状態生成とイベント発火はこの側**にある。
4. **opcode 側は「読取ゲッタ」として実装・実行されている**: 到達経路は3経路の間接呼出しのみ（`0x516AF3 ff d1` / `0x5172C6 ff d1` / `0x4FADD5 ff d0`、いずれもテーブル +0x18 ロード付き、§5.2）。

### 移植への示唆（工数判断）

- **`no-op 実装で可` は誤り（要移植）**: ScriptEffect 系4件は各 ~40B の独立関数であり、①引数スロット（arg5=[esp+0x14] / arg6=[esp+0x18] / arg7=result ptr）②`obj+0x10 → {u8@0, u8@1, f32@+4}` ③Start=byte0, Finish=byte1, Update=`obj+0x29`, Elapsed=`f32+4` の対応を**忠実に移植**すること。省略すると `if ScriptEffectStart/Finish/Update` と `ScriptEffectElapsedSeconds` が常に 0.0 を返す（＝イベント条件が全滅）になる。
- **ディスパッチャも移植必須**: 「opid → entry(+0x28) → +0x10 flags チェック → +0x18 間接呼出し（8引数、arg7=結果 double*）」の3経路（§5.2）。これは ScriptEffect 以外の全オペコードの共通基盤。
- **ScriptEffect の実行系（状態生成・発火・経過秒更新）は MGEF SEFF 側で移植**すること（emitter `0x4FBF90` 経由、mask `0x2000`）。opcode 側だけ移植しても状態が永遠に 0 のままになる。
- **追加7件（Toggle/Get）も no-op ではない**: グローバル変数反転・実値取得を伴う（§4）。Toggle 系はリテラルに移植可能（アドレス値のみ Android 側へ写像必要）。

---

## 8. 判定できなかった点（残課題）

1. **引数オブジェクトの型未確定**: Start/Finish/Elapsed が読む arg6 = `[esi+8]`（ディスパッチャ context のフィールド）、Update が読む arg5 = `ebp`（= スクリプト/プログラムオブジェクト、`[ebp+0x30]`=コード列）が**具体的にどのクラスのインスタンスか**は型情報なしで確定できず。`obj+0x10` が指す `{u8,u8,f32}` 構造体の**書込み元（setter）も未特定**（ActiveEffect/ScriptEffect の状態、あるいはイベント系の書込みの可能性）。
2. **イベントマスクと opid の対応表未特定**: emitter の第3引数がマスクビット（実測 `0x2000`, `0x1000`, `0x08`, `0x04`, `0x80000000`）であることまでは判明したが、**bit ↔ opid (0x11/0x12/0x13) の対応は未解析**。「0x6A286B の `push 0x2000` が ScriptEffectStart を発火する」までは証明できていない。
3. **汎用経路の分岐条件の意味**: `0x517171` の `[esp+0x77c]`（引数8）が「式評価モード/文脈フラグ」のどれに相当するかは推定止まり（0x10BA の impl 呼出し可否を司る）。
4. **`0x4FAB20` の呼び出し元 `0x51336A/0x5136DD`（コンソール系？）の役割**、およびコンソールコマンド実行が 0x4FAB20 経由か別経路かは未確認。
5. **同一関数内に複数の ret（int3 パディングなし）が存在する箇所**（例: 0x4FAD2A 以降のブロック）は、関数境界を機械的に100%確定できていない（§5.2 の3経路のうち (A)(B) は関数先頭 0x5169A0 で確定、(C) は 0x4FAB20 で確定だが、これらの細かい基本ブロック帰属は Ghidra 側の再解析が別途必要）。

---

## 付録: 実行したスクリプトと主出力

| Phase | スクリプト | 主結果 |
|---|---|---|
| 0 | `v17_00_selftest.py` | ST1–ST4 **ALL_PASS** |
| 1 | `v17_01_table.py` | 4+2 エントリ raw/全フィールド、+0x1C ヒストグラム、+0x20=169 |
| 2 | `v17_02_disasm.py` | 11 impl 完全逆アセンブラ（raw hex、終端理由、VERDICT） |
| 3 | `v17_03*_dispatch*.py` 等 | テーブル参照走査、関数先頭特定、間接呼出し3経路の hex |
| 4 | `v17_04_mgef.py`, `v17_04b_fire.py` | E8/imm32 カウント、'SEFF' 判定、emitter 接続点 |
| 5 | `v17_05_compare.py` | OnHit/OnActivate の call 一覧と raw |
