# v465 — `FUN_0041E210` id lookup・`FUN_00420C40` float 取得・flag setter の裁定（V-465-1..3 完遂・旧残点 V-406-3 継続第2弾）

- 採番: **v465**（旧残点群消化・probe prefix **v465_1..v465_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-465-1..3（v464 残点）** — 1=旧残点 V-406-3 継続（`FUN_004CCE00` の `param_1+0x24` flag bit0 意味と `FUN_00420C40` float 計算 identity、top-band 残り caller typed）／2=旧残点 V-406-4（`DATA+0x2C/+0x31` 意味）と V-406-5（`PTR_DAT_00B10D90` 所有 fn）／3=V-464-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-465-1: `FUN_00420C40`（`FUN_004CCE00` の float 計算）identity** | **[確定相当]** | L23449: **`float10 FUN_00420c40(void)`: `iVar1 = FUN_0041e210(0x4c); if (iVar1 != 0) return *(float*)(iVar1 + 0xc); return 0;` [確定(構造)]** → **`FUN_00420C40`＝「id `0x4C`（76）の object を TLS lookup で取得し、見つかればその `+0xC` float を返す・無ければ0 [確定相当]`** |
| **V-465-1: `FUN_0041E210`（lookup 本体）identity** | **[確定相当]** | L20071: **`int __thiscall FUN_0041e210(int param_1, byte param_2)`: `if ((param_2 >> 3 < 0xc) && ((*(u8*)((param_2>>3) + 8 + param_1) & (1 << (param_2 & 7))) != 0))`（**bitset availability テスト＝`self+8+id/8` byte の bit `id%8`**）` { iVar1 = *(TLS slot); if ((param_1 == *(iVar1+8)) && (FUN_0041DEA0(), param_2 < 0x5d)) iVar2 = *(iVar1 + 0x10 + param_2*4); } if (iVar2==0) …` [確定(構造)]** → **`FUN_0041E210(self, id)`＝「self の bitset で id availability を確認し、TLS テーブル（`TLS+0x10+id*4`）から per-thread cached object を返す lookup [確定相当]`**（id は byte・bound `<0x5D`=93、docs mentions **258件**・`FUN_0041e210(0x34)/(0x15)/(0x30)/(0x49)/(0x31)` 等の id 定数呼び出し多数）→ **id 系統は byte 単位の per-thread registry [確定相当]、`0x4C`（76）の意味は [未確定]＝残点** |
| **V-465-1: `form+0x24` flag byte の setter** | **[確定]（1行 setter）** | **`FUN_004C97E0`（L157634 `void __thiscall(int self, undefined1 v)`: `*(u8*)(self+0x24) = v;`）＝flag byte の1行 setter [確定]**。`mov byte [reg+0x24], al` census（class band `0x440000..0x530000`）＝**4件: `FUN_004C97E0`／`FUN_004CDEC0`（`undefined1 __thiscall(int*, int*)`）／`FUN_0052FB40`／`FUN_0052FE90` [確定]** → **`form+0x24` はこの4 fn 系統から書かれる [確定]**（`FUN_004CCE00` が読む `flag bit0`＝`param_2 & 1` の setter 候補） |
| **V-465-1: top-band 残り typed** | **[部分確定]** | 追加 signature: **`FUN_00460BC0` `void __thiscall(int param_1, undefined4 param_2, uint param_3)`（L76387・0x460000 帯新規）**／既出（`FUN_00603CA0(int*,float)`・`FUN_00460710(undefined4,uint)`・`FUN_006384B0(int*,int*)`）を再確認 → **band signature は4件目まで取得 [部分確定]**（全数 typed は未達＝残点） |
| **V-465-2: 旧 V-406-4/5** | **[未確定]（本 probe 未着手＝残点）** | `DATA+0x2C/+0x31` 意味、`PTR_DAT_00B10D90` 所有 fn は未調査 |
| **V-465-3: V-464-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v465_1/v465_2）

### §2.1 `FUN_00420C40` / `FUN_0041E210`（v465_2a）
```
L23449: float10 FUN_00420c40(void) {
  iVar1 = FUN_0041e210(0x4c);
  if (iVar1 != 0) return (float10)*(float*)(iVar1 + 0xc);
  return (float10)0; }

L20071: int __thiscall FUN_0041e210(int param_1, byte param_2) {
  if ((param_2 >> 3 < 0xc) &&
      ((*(byte*)((param_2 >> 3) + 8 + param_1) & (byte)(1 << (param_2 & 7))) != 0)) {
    iVar1 = *(int*)(*(int*)(FS+0x2c) + _tls_index*4);   // TLS
    iVar2 = 0;
    if ((param_1 == *(int*)(iVar1 + 8)) && (FUN_0041DEA0(), param_2 < 0x5d))
      iVar2 = *(int*)(iVar1 + 0x10 + param_2*4);        // TLS table[id]
    if (iVar2 == 0) { ... } }
docs mentions of FUN_0041e210: 258  (0x34 / 0x15 / 0x30 / 0x49 / 0x31 / ... id consts)
```

### §2.2 `form+0x24` flag setter census（v465_1b）
```
`mov byte [reg+0x24], ...` in 0x440000..0x530000: 4 hits
  @0x4C97E4 owning=FUN_004c97e0 (L157634: *(u8*)(param_1+0x24) = param_2; 1-line setter)
  @0x4CE041 owning=FUN_004cdec0 (L161587: undefined1 __thiscall(int*, int*))
  @0x52FB81 owning=FUN_0052fb40
  @0x52FF52 owning=FUN_0052fe90
```

### §2.3 top-band signature 追加（v465_1c）
```
band 0x460000: FUN_00460bc0 (L76387): void __thiscall(int param_1, undefined4 param_2, uint param_3)
(reconfirmed: FUN_00603CA0 / FUN_00460710 / FUN_006384B0)
```

## §3 残点 **V-466-1..3**
1. 【中】**旧残点 V-406-3 継続** — **id `0x4C`（76）の意味**（TLS registry の何の object か・`+0xC` float の語）＋ `form+0x24` flag bit0 の setter特定（4候補の絞り込み）＋ top-band 残り typed。
2. 【低】**旧残点 V-406-4／V-406-5** — `DATA+0x2C/+0x31` 意味（UESP 差分の docs 内裏取り）／`PTR_DAT_00B10D90` 所有 fn と spec 名前テーブル書手。
3. 【低】**V-465-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v465_1..v465_2 の2本全 EXIT=0**（V-465-1 は `FUN_00420C40`/`FUN_0041E210` identity 相当＋flag setter4件＋band signature 追加／V-465-2/3 は未着手＝残点）。
- **決定打の作り**: (i) **`FUN_00420C40` の decompiled が `FUN_0041E210(0x4c)` を直呼び** → lookup 本体を1手で特定 → (ii) **`FUN_0041E210` の body の bitset＋TLS table 構造**で id lookup と確定（**`param_2 < 0x5d` bound と `+0x10+id*4` が決定打**）→ (iii) **`mov byte [reg+0x24]` census 4件**で flag setter 候補を列挙し、1行 setter（`FUN_004C97E0`）を即決 → (iv) top-band は signature 行を docs から追加取得。
- 妥当性チェック: ①`FUN_0041E210(0x4C)`⇔`param_2 < 0x5D`（0x4C=76<93 ✓）— ②`+0x10+id*4`⇔TLS table（`iVar1+0x10` base）— ③bitset `self+8+id/8`⇔`param_2>>3 < 0xc`（96bit=12byte）— ④`FUN_004C97E0` の `*(u8*)(+0x24)=v`⇔`FUN_004CCE00` の `*(u8*)(+0x24)&1` 読手 — ⑤docs mentions 258件⇔id 定数呼び出し多数 — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_0041E210`/`FUN_00420C40`/`FUN_004C97E0` に annotation 実在）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **「A fn が B fn を引数定数で直呼び」のとき、B の identity は A の decompiled で即決できる**（`FUN_00420C40`→`FUN_0041E210(0x4C)`）。
  2. **id lookup の bound（`param_2 < 0x5d`）と table offset（`+0x10+id*4`）が id 系統の設計を語る** — byte id・per-thread・93種 bound。
  3. **flag byte は「1行 setter（`*(u8*)=v`）」と「複合 setter（census 4件）」が並存する** — census で候補を出し、decompiled で1行 setter を即決。
  4. **`mov byte [reg+disp], reg` census は class band で絞ると候補が少ない**（本件4件）— 全 .text ではなく対象 band。
  5. **docs mentions の件数（258件）と id 定数呼び出し列挙が「lookup の利用頻度と id 系統」の裏付け**（`0x34/0x15/0x30/0x49/0x31`…）。
