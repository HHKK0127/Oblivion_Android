# v412 — V-411-1【中】`iVar10` の出所と V-411-2【中】後続計算段の裁定（派生残点第4弾）

- 採番: **v412**（派生残点群・probe prefix **v412_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v411 の残点 **V-411-1（`iVar10 = FUN_004677F0()-1` の identity と「-1」）【中】／V-411-2（後続計算段 `FUN_00547F80`/`FUN_00548020`/`FUN_005479D0→FUN_004672B0` の identity）【中】** を消化（V-411-1 完遂・V-411-2 は部分確定で残点縮小）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-411-1: `iVar10 = FUN_004677F0()-1` の出所** | **[確定(命令)]（系統自体）＋[推定高]（"level" の語）** | `FUN_005222D0` **直頭の machine**: `mov edi,ecx（＝player）→ lea ecx,[edi+0x24] → call 0x4677F0` — **this ＝ `Player[+0x24]` の埋め込みオブジェクト**（&member ごと thiscall）。`FUN_004677F0`（L81348-81371）: `v = [obj+0xe]`（ushort）→ **flag `[obj+4]>>7&1` 時は `[0xB333C4] virtual+0x170 → [+0x32]`（short）を加算** → `[obj+0x10]`/`[obj+0x12]` で clamp → **`<1 → 1`** → ushort return。**`iVar10 = v − 1`（v>=1 より 0 以上）＝「level/進捗系の値 −1」**（base として skill 式に使用）[推定高：語の確定は V-412-1] |
| **V-411-2: `FUN_005479D0` と SET 経路** | **[確定]** | **`FUN_005479D0(a,b,c,d) = a+b+c+d`（L243767-243771）＝4属性の合計** → `FUN_004672B0(&Player[+0x24], u16)` が **`[obj+0xA] = u16` ＋ `virtual+0x50()` 通知**（L80955-80963）／`FUN_00467290(&Player[+0x24], u16)` が **`[obj+0x8] = u16` ＋ 同通知**（L80941-80949）。machine: `push 4引数 → call 0x5479D0 → lea ecx,[edi+0x24] → call 0x4672B0`（0x5226F5-0x522701）・`FUN_005482B0(FUN_00468AD0(1),0) → FUN_00467290`（0x5226AA-0x5226AE） |
| **V-411-2: `FUN_00547F80`（5引数）/`FUN_00548020`（2引数）** | **[部分確定]** | **引数付き計算ヘルパ** — decompiled 表示は `void(){ FUN_009828C0(); ... }`（**fastcall/stack 引数が Ghidra で落ちている**）、machine は `push 5引数 → call 0x547F80`（attr0, attr5, iVar10, local_4, class_spec）／`push 2引数 → call 0x548020`（attr0, attr5）＝**attr/level/spec 入力の数式計算**。内部の `FUN_009828C0`（2回/1回呼び出し）は **floor/round 系 math [推定高]** — 厳密な数式は残点 V-412-2 |
| **`Player[+0x24]` の identity** | **[確定(命令)]＋[部分確定]** | **vtable 付きの埋め込みオブジェクト**（`FUN_00467290/004672B0` の this・いずれも `virtual+0x50()` で dirty 通知）。**フィールド**: `[+8]`(u16・`FUN_00467290` setter)／`[+0xA]`(u16・`FUN_004672B0` setter)／`[+0xe]`(ushort 値)／`[+0x10]`/`[+0x12]`(ushort min/max clamp)／`[+4]>>7`(flag＝PC 修正有効) — **derived stat/level 系の構造 [推定高]**（RTTI は残点 V-412-3） |
| **skill 式 base の最終解釈** | **[部分確定]** | v411 の skill 段 `base = iVar10` は **「`Player[+0x24]` の ushort 値（PC 加算・clamp・min1）−1」** → **major+25／非 major×0.1+5／spec+0.5×base+5 の "base" はこの level/進捗系値** [確定(命令)・語は推定高] |

## §2 証拠（v412_1）

### §2.1 machine（FUN_005222D0 の call 文脈）
```
5222D4: mov edi, ecx                     ; this = player5222D6: lea ecx, [edi + 0x24]          ; ★ this = &Player[+0x24]
5222D9: call 0x4677F0                    ; v = FUN_004677F0(&Player+0x24)（iVar10=v-1 は decompiled L216779）
...
522497: lea ecx, [edi + 0x88] / call 0x468AF0   ; attr byte setter（v411 確定）の this=Player[+0x88]
5225E2: mov ecx, edi / push 0x200 / call 0x46ABF0 ; 通知52263C: push2 / call 0x548020  ; (attr0, attr5) 計算52267C: push5 / call 0x547F80        ; (attr0, attr5, iVar10, local_4, spec) 計算
52268E: push / mov ecx, edi / call 0x519C50      ; param_2==0 分岐5226AE: lea ecx, [edi + 0x24] / call 0x467290 ; [obj+8] = u165226F5: call 0x5479D0 → 5226FE: lea ecx,[edi+0x24] → call 0x4672B0 ; [obj+0xA] = attr0+attr5+attr3+attr2522706: mov eax, [edi+0x104] ... 3連鎖（v410/v411 確定の class→actor copy）
```

### §2.2 `FUN_004677F0` body（L81348-81371）
```c
ushort FUN_004677F0(obj) {
  v = (ushort)obj[+0xe];
  if (obj[+4] >> 7 & 1) {                       // PC 加算 flag    v += (short)([0xB333C4]->virtual+0x170() ) [+0x32];      // base 修正
  if ((min=obj[+0x10]) && v < min) return min;
  if ((max=obj[+0x12]) && max < v) return max;
  if (v < 1) v = 1;
  return v;
}
```

### §2.3 後続計算段（L216884-216911 ↔ machine）
- `FUN_005479D0(param_1..4){ return param_2+param_1+param_3+param_4; }`（L243767-71）＝**4属性合計 [確定]**。
- `FUN_004672B0(this, u16){ [this+0xA] = u16; virtual+0x50(); }` / `FUN_00467290(this, u16){ [this+0x8] = u16; virtual+0x50(); }`（L80941-80963）。
- `FUN_00547F80`/`FUN_00548020` の decompiled は void 表示（引数落ち）→ machine の push 数（5/2）で arity を確定。

## §3 残点 **V-412-1..4**
1. **【中→低】`FUN_004677F0` の "level" 語の確定** — 他 callers（E8 census）と `Player[+0x24]` の ctor/RTTI から語（level? rank? progress?）の裏取り。
2. **【低】`FUN_00547F80`/`FUN_00548020` の数式** — `FUN_009828C0`（floor/round 系 [推定高]）を含む実体と、5引数（attr0, attr5, iVar10, local_4, spec）の意味づけ。
3. 【低】`Player[+0x24]` オブジェクトの RTTI（vtable）とフィールド名（`[+8]`/`[+0xA]` の u16 が何か）。
4. 【低】`FUN_00519C50`（param_2==0 分岐の初動・0x52268E）の identity（V-411-2 引き継ぎ）。

## §4 検証・自己批評
- probe: **v412_1 全 EXIT=0**（1本で V-411-1 完遂＋V-411-2 大半）。
- **決定打の作り**: (i) `FUN_005222D0` 全 call site の **4命令文脈 dump**（this=ecx の source 追跡＝`lea ecx,[edi+0x24]` を即座に発見）→ (ii) decompiled body 実読で clamp/flag/戻り値を復元 → (iii) `FUN_005479D0` の one-liner 実読（合計）＋SET 経路（`[this+8]/[this+0xA]`＋`virtual+0x50`）→ (iv) arity は machine push 数で確定（Ghidra void 表示の陷阱回避）。
- 妥当性チェック: ①`lea ecx,[edi+0x24]` ⇔ decompiled `FUN_00467290/004672B0` の this ⇔ `[this+8]/[this+0xA]` — ②4属性合計 ⇔ `FUN_005479D0` ボディ ⇔ push4 — ③`iVar10=v-1` ⇔ L216778-79 ⇔ `v>=1` — **三方向矛盾なし**。
- docs との不整合: **なし**（V-411-1 消化・V-411-2 を残点2件に縮小）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **"thiscall の this 追跡は call 直前の `lea ecx,[reg+disp]` / `mov ecx,...` を4命令文脈で見る"** — decompiled の引数落ち表示を machine で即補完。
  2. **Ghidra の `void fn(void){...}` 表示は fastcall/stack 引数落ちの兆候 → arity は machine の push 数で確定**（v406 の thiscall 教訓の stack 引数版）。
