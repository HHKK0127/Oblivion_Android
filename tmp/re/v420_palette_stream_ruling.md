# v420 — palette の stream 読込経路の manager 側接続と実行優先関係の裁定（V-405-1 完遂・派生残点第12弾）

- 採番: **v420**（派生残点群・probe prefix **v420_1..v420_5 の5本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c` ＋ `tmp/nif_oracle`（実データ parser・読み取りのみ）
- 依頼経緯: v405 の残点 **V-405-1【中】（`0x7166B0` の vtable slot 未確認・manager の stream LoadBinary が `Object Palette:Ref` を `[+0x7C]` へ格納する site・KF 読込と遅延生成の実行順序/優先関係）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・NIF oracle（実データ parser）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0x7166B0`（palette LoadBinary）の vtable slot** | **[確定(RTTI)]** | **NiDefaultAVObjectPalette の vtable `0xA798C4` slot7（+0x1C）** — data ref は `.rdata 0xA798E0` の1件（RTTI backward scan）・**E8=0＝virtual dispatch のみ** |
| **manager の stream LoadBinary（idx）** | **[確定(RTTI＋構造)]** | **NiControllerManager の vtable `0xA79804` slot7（+0x1C）= `FUN_006C4E50`** — 本体が `NiTimeController 系 base 読込（FUN_00715f40）` → Cumulative bool → sequences 読込 → **version gate 付き Object Palette:Ref 読込** の oracle 順と完全一致（下記） |
| **`Object Palette:Ref` → `[mgr+0x7C]` 格納 site** | **[確定(命令)]** | `FUN_006C4E50` 末尾: **`cmp dword [ctx+0xd8], 0x0A010068; jb 0x6C4EB2`（version ≥ 10.1.0.104 ゲート）** → `mov ecx,esi; call 0x712A90`（**ReadRef**: u32 index 読込、`-1→0`、`[ctx+0x1F0][idx]` の block table 解決・L555600-555611）→ `push eax; **lea ecx,[edi+0x7C]**; call 0x75FA90` → **`FUN_0075FA90`＝NiPointer assign（旧 ptr は `InterlockedDecrement`→0 で dtor、新 ptr を代入→`InterlockedIncrement`・L604450-604467）**＝**`[mgr+0x7C]` へ refcounted 格納**（machine 0x6C4E91-0x6C4EB5） |
| **EXE⇔oracle の読込順一致** | **[確定]** | machine 順: **`[edi+0x6C] = Cumulative bool`**（stream virtual read 1byte → `setne cl` → `mov [edi+0x6c],cl`）→ **`call 0x712AE0`（u32 読込＋`[ctx+0x238/0x23c]` vector grow＝Num Controller Sequences 読込 [推定高]）** → version gate → **Object Palette:Ref** ⇔ oracle `parseNiControllerManager`（L3184-3193: `Cumulative:bool(1) → Num Controller Sequences:uint(4)+Ref[N] → Object Palette:Ref(4) ← last field`）— **順序完全一致** |
| **KF 読込 vs 遅延生成の優先関係** | **[確定(命令)]** | **ファイル記載（stream）優先・遅延生成はフォールバック**: ①stream 経路＝slot7 `FUN_006C4E50` が load 時に**無条件 assign**（version gate 内・ref=-1 なら 0 を assign）②遅延経路＝slot9 `FUN_006C5AB0` は **`if ([mgr+0x7C]==0)` のときのみ** `[mgr+0x7C] = FUN_006c5360(this[+0x30])`（Target 部分木走査・v405 確定）→ **非 null の stream 値は slot9 を通過させない**（null 時のみ slot9 が生成・**version < gate でも slot9 が生成に走る**） |
| **slot9 の呼出し形** | **[確定(否定側)]** | **E8=0・data ref は manager vtable slot9（`0xA79828`）の1件のみ** → **virtual dispatch（`call [reg+0x24]`）のみ**・直接呼出し不在 |
| （副次） | **[確定]** | **`[NiControllerManager +0x6C]` ＝ Cumulative bool**（1byte 読込＋`setne`） |

## §2 証拠（v420_1..v420_5）

### §2.1 slot 同定（v420_1(a)/v420_2(a)）
- `0x7166B0` data ref → `.rdata 0xA798E0` → RTTI backward scan → **vtable `0xA798C4` slot7 RTTI `.?AVNiDefaultAVObjectPalette@@`**。
- manager vtable `0xA79804`: slot5=`0x6F7070`／slot6=`0x6C5CC0`／**slot7=`0x6C4E50`**／slot8=`0x6C5C20`／**slot9=`0x6C5AB0`**（v405 確定の遅延生成）／slot10=`0x6C4DA0`／slot11=`0x6C42B0`。

### §2.2 格納 site（v420_3/v420_4/v420_5(a)）
```
6C4E8F: mov ecx, esi                  ; ctx(stream wrapper)
6C4E91: call 0x712AE0                 ; u32 読込 + [ctx+0x238/0x23c] grow（Num Controller Sequences [推定高]）
6C4E96: cmp dword [esi+0xd8], 0x0A010068
6C4EA0: jb 0x6C4EB2                   ; ★ version < 10.1.0.104 なら palette 読込 skip
6C4EA2: mov ecx, esi
6C4EA4: call 0x712A90                 ; ReadRef（-1→0、[ctx+0x1F0][idx]）
6C4EA9: push eax
6C4EAA: lea ecx, [edi + 0x7c]          ; ★ &mgr[+0x7C]
6C4EAD: call 0x75FA90                 ; ★ NiPointer::assign（refcount 管理付き代入）
6C4EB2: pop edi / pop esi / pop ecx / ret 4
```
- `FUN_0075fa90`（L604450-604467）: `if (old!=new){ old? (InterlockedDecrement→0 なら dtor) ; *this=new; new? InterlockedIncrement }`。
- `FUN_00712a90`（L555600-555611）: `Read u32 index; -1→0; return [ctx+0x1f0][index]`。
- `[edi+0x6C]` Cumulative: `6C4E7F: call eax`（stream virtual read size=1）→ `setne cl` → `mov [edi+0x6c],cl`（v420_4(a)）。

### §2.3 slot9 遅延生成と oracle（v420_4(b)/v420_2(c)/v420_5(c)）
- `FUN_006C5AB0` 内: `mov ecx,[esi+0x7c]`／`mov edi,[esi+0x7c]`／**`mov [esi+0x7c], ebx`**（生成結果の store）＝v405 の `if (this[+0x7C]==0) this[+0x7C]=FUN_006c5360(this[+0x30])` と整合。
- oracle `parseNiControllerManager`（nif_parser.cpp L3184-3193）: **`Object Palette:Ref(4) ← last field`** ＝NiControllerManager の最終フィールド [確定(実データ parser)]。
- slot9: E8=0・data ref = `0xA79828`（=vtable 0xA79804+0x24）のみ。

### §2.4 palette 側 LoadBinary（v405 継承＋本 probe 補強）
- `0x7166B0`（slot7）は E8=0・virtual のみ → **stream フレームワークが palette block を読むときに実行**（`palette[+0x1C]=ReadRef(Target)` → count → `virtual+0x50(name,obj)` insert＝v405 確定のファイル記載対）。

## §3 残点 **V-421-1..3**
1. 【低】`FUN_00712AE0` の格納先の正確な意味（`[ctx+0x238/0x23c]` vector grow が sequences 配列か、および sequences の manager 側最終格納先（本 probe の decode 窓では未出現））。
2. 【低】slot9（`+0x24`）の virtual 呼出し元 fn 群（`call [reg+0x24]` を打つ caller の特定＝遅延生成の起動タイミング特定）。
3. 【低】V-405-2..5 引継ぎ（stride-48 型照合【中】／palette insert 本体【低】／未知 interpolator idx35【低】）。

## §4 検証・自己批評
- probe: **v420_1..v420_5 の5本全 EXIT=0**（V-405-1 完遂）。
- **決定打の作り**: (i) **data ref → RTTI backward scan で slot 同定**（palette slot7・manager slot7/9）→ (ii) **`[reg+0x7C]` の store census を「mov 形」で打つが0件 → `lea ecx,[reg+0x7c]`（out-param/this 形）にフォーカスし直し**で site を発見 → (iii) site 周辺の線形 decode で **version gate＋ReadRef＋NiPointer assign** を復元 → (iv) **oracle のレイアウトコメント（実データ parser）と machine 読込順を突合** → (v) slot9 の null 判定と store で優先関係を確定。
- 妥当性チェック: ①machine 順（Cumulative→u32→palette ref）⇔oracle 順（bool→u32+ref array→palette ref last）— ②`lea ecx,[edi+0x7c]`＋`FUN_0075FA90`（refcount assign）⇔`[mgr+0x7C]`＝NiPointer（v405 の遅延生成 store と同型）— ③version gate `0x0A010068`⇔v405 の `seq[+0x40]` 読込 gate と同一定数 — ④slot9 null 判定⇔「stream 優先」— **四方向矛盾なし**。
- docs との不整合: **なし**（v405 の「[+0x7C] の writer＝manager idx9」は**本裁定で補正**: idx9 は遅延生成のみで、**stream 読込 writer は slot7 `FUN_006C4E50`**）。docs/GT 反映時は v405 ruling の該当注記を併せて更新・#47/#20 一括。
- **教訓**:
  1. **`[reg+disp]` store census が0件なら「`lea ecx,[reg+disp]` でアドレス渡し（out-param／this 代替）」を疑え** — NiPointer/コンテナへの代入は fn 呼び出しで行われるため mov 形で現れない。
  2. **version gate 定数（`0x0A010068`）は同一 EXE 内で複数の LoadBinary が共有** — gate 定数の一致が「同一 stream形式の読込帯」の接合点になる。
  3. **実データ parser（NIF oracle）のレイアウトコメントと machine 読込順の突合で、読込フィールドの identity を一括確定できる**（Cumulative/Sequences/Object Palette の3連）。
