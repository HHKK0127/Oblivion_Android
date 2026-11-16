# v414 — `[TESActorBaseData+0xE]`＝**level [確定(命令)]** と `Player[+0x24]` クラス同定（V-413-1／V-412-1／V-412-3 一挙解決・派生残点第6弾）

- 採番: **v414**（派生残点群・probe prefix **v414_1..v414_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v413 の残点 **V-413-1（`FUN_00467240/00467320` identity → "level" 語）【中】** と v412 の **V-412-1（"level" 語）・V-412-3（`Player[+0x24]` RTTI）** を一挙に消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-412-1: `FUN_004677F0` が返す `[+0xE]`(ushort) の語** | **[確定(命令)]** | **＝ "level"** — 決定打は **`FUN_0066C200`（保留適用/levelup 適用 fn）内の increment**: `ecx = base+0x24 → call 0x4677F0（v取得）→ **`add ax,1`** → `push eax → ecx=base+0x24 → call 0x467270`（set）**（machine 0x66C45E-0x66C47B）＝「level を +1 して保存」＝level-up の機械的証明**。補証: ①`FUN_0050CD30`（script opcode）が **引数をそのまま `set [+0xE]`**（0x50CE06-0x50CE09＝script が level を設定）②`FUN_0050AAA0` fn が **GET→新へ SET（actor 転送コピー）→ `FUN_005222D0` 再計算**（0x50AB5E-0x50AB73）③既定 **1**（ctor/reset）＋`FUN_004677F0` 内 `<1→1` clamp |
| **V-412-3: `Player[+0x24]` のクラス** | **[確定(RTTI)]** | **`.?AVTESActorBaseData@@`** — ctor `FUN_00467320` が `[eax]=&PTR_LAB_00A3B654` を設定 → vtable `0xA3B654` の RTTI で即同定。**PlayerCharacter に埋め込み（+0x24）** |
| **setter/reset family（TESActorBaseData の u16 フィールド群）** | **[確定(命令)]** | **`FUN_00467240`＝reset**（`[+4]=0, [+8]=50, [+0xA]=50, [+0xC]=0, [+0xE]=1, [+0x10]=0, [+0x12]=0, [+0x14]=0`・`mov edx,0x32` 実測）／**`FUN_00467320`＝ctor**（vtable 0xA3B654＋同一 defaults・L80988-81003）。**u16 setter 4本（同型: arg u16 → store → `virtual+0x50(0x10)` 通知）**: `[+8]`＝`FUN_00467290`（AV index9・初50）／`[+0xA]`＝`FUN_004672B0`（index10・初50）／**`[+0xC]`＝`FUN_004672D0`（初0）**／**`[+0xE]`＝`FUN_00467270`（level・初1）** |
| **`[+0xE]` setter の全 callers** | **[確定]** | **E8=3 件・data refs 0（vtable 登録なし）**: `0x50AB6A`（actor 転送 fn・head 0x50AAA0）／`0x50CE09`（`FUN_0050CD30` opcode）／`0x66C47B`（`FUN_0066C200` levelup 適用） |
| （副次）`[+8]`/`[+0xA]` 既定50/50 の語 | **[推定高]** | actor value index9/10（v413 dispatcher）に格納・初値50 — **Health/Magicka 系 [推定]**（EXE 根拠は「初値50＋index 9/10」のみ。語の裏取り＝残点 V-414-1） |

## §2 証拠（v414_1..v414_3）

### §2.1 level increment（v414_3(a) machine）
```
66C45E: mov ecx, eax ; base actor66C460: add ecx, 0x24 ; → TESActorBaseData66C463: call 0x4677F0 ; v = [+0xE]66C468: add ax, 1 ; ★ +166C46C: push eax66C46F: push 0 / call 0x5E02E0 / mov ecx,eax / add ecx,0x24
66C47B: call 0x467270 ; set [+0xE] = v+1
```

### §2.2 script 設定と actor 転送（v414_3(b)(c)）
- `FUN_0050CD30`: `[esp+0x20]+0x24`（actor の TESActorBaseData）→ `push arg → call 0x467270` ＝**opcode が level を直接 SET**。
- `0x50AAA0` fn: `mov ecx,[eax+0x104]; [esi+0x104]=ecx`（class コピー）→ `lea ecx,[eax+0x24]; call 0x4677F0` → `lea ecx,[esi+0x24]; push eax; call 0x467270`（**level コピー**）→ `call 0x5222D0`（再計算）＝**actor 転送/複製 fn**。

### §2.3 reset/ctor と RTTI（v414_1/v414_2）
- `FUN_00467240`（reset・E8=0）: `edx=0x32` → `[+8]/[+0xA]=50`、`[+0xE]=1`、他0。
- `FUN_00467320`（ctor）: `[eax]=0xA3B654` → **RTTI `.?AVTESActorBaseData@@`（v414_2(a)）**・defaults 同一。
- `[+0xE]` setter `FUN_00467270`（v414_1 の 0x467270）: `mov ax,[esp+4]; mov [ecx+0xe],ax; virtual+0x50(0x10)`。
- `[+0xC]` setter `FUN_004672D0`（v414_2(c)）: 同型 family 完全化。

## §3 残点 **V-414-1..3**（＋V-413-2 引継ぎ）
1. 【低】`[+8]`/`[+0xA]`（AV index9/10・初50/50）の語（Health/Magicka 系 [推定高]）と `[+0xC]`（初0・setter FUN_004672D0）の identity — index8〜10 の actor value 名の裏取り（opcode テーブル or menu id 突合）。
2. 【低】`FUN_004677F0` の `[obj+4]>>7` flag と `[0xB333C4] virtual+0x170 → [+0x32]` 加算（PC base 修正）の意味。
3. 【低】V-413-2 引継ぎ: 未注記2 fn（`FUN_00511570`/`0x523F40`）の identity と `FUN_00519F50` の `case 0x0B+`。

## §4 検証・自己批評
- probe: **v414_1..v414_3 の3本全 EXIT=0**。
- **決定打の作り**: (i) reset/ctor の **既定値実測（50/50/1）** → (ii) **increment 操作（`add ax,1`→set）の発見で "level" を機械確定** → (iii) setter の全 callers（E8＋data refs）で使用系3種（levelup/script/transfer）を分類 → (iv) **ctor が設定する vtable を RTTI で即同定**（Player[+0x24] のクラス）→ (v) setter family を線形 decode で全数化。
- 妥当性チェック: ①`v+1` increment⇔`<1→1` clamp⇔既定1 — ②setter family 4本が同型（u16＋`virtual+0x50`）⇔dispatcher v413 の case9/10 — ③RTTI⇔ctor vptr⇔`lea ecx,[...+0x24]` 使用側 — **三方向矛盾なし**。
- docs との不整合: **なし**（V-412-1/3・V-413-1 を一括解決）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **reset/ctor の「既定値実測」と「increment 操作（add ax,1 → set）」をセットで見るとフィールドの語が機械的に確定する** — 既定1＋min1 clamp＋+1 increment ＝ level の三証拠。
  2. **ctor が `[eax]=vtable` を設定するなら、その vtable の RTTI が「埋め込みオブジェクトのクラス」を即同定する**（instance ctor を decompiled 注記から辿る）。
  3. **u16 setter family は連番 fn 帯（0x4672xx）の線形 decode で全数化** — 同型構造（arg→store→notify）が family の存在証明。
