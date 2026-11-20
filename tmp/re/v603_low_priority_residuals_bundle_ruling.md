# v603 — 低優先残点クローズ（バンドル）: formType 0x23 述語・AV索引変換・AddPane 引数・カウンタ書手・`[item]+0x34`・Main/`0x5CC8BA` 分岐（ai-todo #96/#157/#161/#162 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u16_1〜u16_3**・全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v558_racesexmenu_pages_ruling.md`・`v559_statsmenu_tabs_ruling.md`・`v567_u2_page_order_ruling.md`・`v569_p3_p5_rows_ruling.md`・`v570_racenotify_dispatch_ruling.md`・`v571_race_id_allocation_ruling.md`・`v572_skill_grouping_predicate_ruling.md`
判定: `[確定(命令)]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 残点 | 判定 | 結果 |
|---|---|---|
| **V-572-3** `0x5E3320`/`0x4D7F80` の型 | **[確定(命令)]** | `0x4D7F80(this)` は **`[vfunc+0x190]() != 0` かつ `[vfunc+0x170](this)` の戻りオブジェクトの `[+4] == 0x23`** を検査する述語。**`[obj+4]`＝formType byte** で、`0x23`（35）＝`TESClass`（**class レコードの formType**）。`0x5E3320(this)` は `0x4D7F80` が真なら `[vfunc+0x170](this)+0x104` を返す（＝**player の class オブジェクトの `+0x104`**）。→ v572 の「P」は「**player の class（TESClass, formType 0x23）の管輪スキル7個判定**」。 |
| **V-572-2** `this+0x44` 配列と `this+0xC` | **[確定(命令)]** | `0x51C090(this=class, skill)` は **`[this+0xC] == [0xB37D00]`（=charactergen 既定クラス formID 0x230E6）なら即 false**、そうでなければ **`[this+0x44..+0x5C]` の 7 要素 dword 配列を線形探索し `skill` と一致すれば true**。加えて `0x51C0F0(this, idx, av)`＝**`av ∈ [0xC,0x20]` のとき `[this+idx*4+0x44]=av`（idx<7）**＝**この配列は TESClass の「majorSkills（管輪スキル）7 個」の AV 値リスト**（定義順でなく AV 番号を格納）。`[this+0xC]`＝**class の formID**。 |
| **V-569-4** `0x565BE0(2)` とシグネチャ | **[確定(命令)]** | `0x565BE0(idx, base)` は **`idx`(0..6) で `base` に加算する値が変わる AV オフセット関数**（jump table `0x565C30`）: **0→`base`／1→`base+8`／2→`base+0xC`／3→`base+0x21`／4→`base+0x25`／5/6→続き**（`movsx`＋定数加算）。第 1 引数 `2`＝**`base+0xC`** を選ぶセレクタ。`0x565CC0(idx)`＝**`idx<0x28` は `[0xB12758+idx*4]` の先頭 dword、`0x28<=idx<0x48` は `[0xB12868+…]`** を返す別の表引き。 |
| **V-571-3** `[0xB3B5D4]` の書手 | **[確定(命令)]** | `.text` 全域で **`[0xB3B5D4]` への直接ストアは `0x5C4F3C`（`0x5C4E40`＝RaceSexMenu ctor）の imm 1 箇所のみ**＝**`200` リセットは RaceSexMenu ctor だけ**（他メニューは使わない）。`0x5C4340`（AddPane）の `+1` は read-modify-write（直接ストア形ではない）ので上の走査に出ない。 |
| **V-570-6** `[0xB3B4C9]` のクリア経路 | **[確定(命令)]** | 書き手は 3 箇所: **`0x5C9FB1`（`0x5C9CD0`＝Reset 実行）／`0x5CCCBC`・`0x5CD9CA`（`0x5CC200`＝RaceSexMenu 通知ハンドラ本体）**。→ **Reset 実行時と通知ハンドラの初期化/末尾でクリア**。 |
| **V-570-5** `gt` 経路の誤爆なし | **[確定(命令)]** | 通知 id は **XML 由来の `<id>`（1/2/10/20/90）＋動的採番 200 系（201..428）** で、**`[0xB3B5D4]` は ctor で 200 にリセット後に単調増加**するため、**通知 id は一意**。ブロック照合は `<tile_id` の `gt` 閾値で行うが、id が一意なので誤爆しない。 |
| **V-567-2** `0x5CC580`/`0x5CC829` の重複理由 | **[部分確定]** | `0x5CC580` は `0x5CC57A jnp 0x5CC9C4`（**比較不一致時の Brow 経路**）の先、`0x5CC829` は `0x5CC810 jnp 0x5CC97F`（別比較）の先で、**それぞれ異なる `[0xB39008]`/`[0xB38F70]`（ページ名 GMST）を参照**する＝**同一名（`sEyes` 等）が複数ページの要素名として使われる**ため重複して見える。Main の `0x5CC829`/`0x5CC863` は **`0xFAE`(user0) と `0xFD0`(listindex) の両方を書く**ため 2 命令に見える（同一 page への別 trait 書込）。 |
| **V-567-3** `0x5CC8BA` の更に深い分岐 | **[確定(命令)]**（v602 で本体確定済の補完） | `0x5CC8BA` は id を `0xFA8` と比較し、`jp 0x5CCBF4` で **id98 判定→Random act**。id98 以外で全ブロック不一致なら `0x5CCC1C` 以降（act なし epilogue）。→ **`0x5CC8BA`＝「どの要素にも一致しない id」の共通フォールスルー**（v602 §2.4 と同一）。 |
| **V-571-4** AddPane の a3/a5 | **[部分確定]** | `0x5C4340(this, strA, strB, prep)`＝4 引数（`ret 0x10`、v571 確定）。**a3=prep flag（ページ遷移準備 index）／a5 は存在しない**（4 引数のみ）。a5 相当の引数は無い。 |
| **V-558-4** `sGeneralFace`/`sGeneralSkin` の ESM 既定値 | **[未確定]** | GMST の ESM 既定値（英語既定と一致するか）は ESM レコード依存。**`Oblivion.esm` の GMST ブロックを読めば確定できるが、本バンドルでは未走査**。 |
| **V-559-1** health(AV8) の p1 供給 | **[部分確定]** | 統計 builder の trait **`0xFB6`(current)/`0xFB7`/`0xFB8`/`0xFB9`/`0xFBA`/`0xFBB`/`0xFBC`** が AV 8/9/10 系の供給スロット（`0x58ADxx` 登録・`0x5992C0`/`0x599EE0`/`0x5987F0` 系 builder）。**health(AV8) は `0xFB6`(current)＋`0xFB8`/`0xFBA` 系**（magicka/fatigue と同型）。厳密な AV8 固有スロットの pin 留めは残る。 |
| **V-559-4** `[item]+0x34` の bit フラグ | **[部分確定]** | StatsMenu 帯（`0x5D9xxx`〜`0x5DAxxx`）で `[item+0x34]` は**表示要素のポインタ/値フィールドとして読書き**される（`0x5D9120`/`0x5D95E8`/`0x5D9AE8`）。**bit0/bit3 を faction の hidden フラグとする直接証拠は本バンドルで未取得**（`[+0x34]` は本帯では 4B フィールドとして使われ、byte フラグ読取 `test […],0x80` 形は出ない）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 V-572-3 / V-572-2（class 述語と管輪スキル）

```
0x4D7F80(this):                 ; formType 0x23 (TESClass) 判定
  4D7F85 call [vfunc+0x190] ; test al,al ; je false
  4D7F93 call [vfunc+0x170] ; test eax,eax ; je false     ; player->class
  4D7FAD cmp byte [eax+4],0x23 ; jne false                 ; ★ formType==0x23
0x5E3320(this):
  5E3323 call 0x4D7F80 ; test al,al ; je 0x5E3344
  5E3336 call [vfunc+0x170] ; mov eax,[eax+0x104] ; ret    ; ★ class+0x104
0x51C090(class, skill):
  51C090 mov edx,[ecx+0xC] ; cmp edx,[0xB37D00] ; je return0  ; ★ formID==charactergen
  51C0A7 add ecx,0x44 ; loop 7: cmp [ecx],esi ; je found      ; ★ 7要素線形探索
0x51C0F0(class, idx, av):
  51C0F4 lea edx,[eax-0xC] ; cmp edx,0x14 ; ja ; 51C100 mov [ecx+edx*4+0x44],eax  ; ★ av 0xC..0x20
```

### 2.2 V-569-4 AV オフセット関数

```
0x565BE0(idx,base): jump [idx*4+0x565C30]
  [0] 0x565BF0 movsx eax,[esp+8] ; ret            ; base
  [1] 0x565BF6 movsx eax,[esp+8] ; add eax,8 ; ret ; base+8
  [2] 0x565BFF add eax,0xC ; ret                   ; base+0xC   ★ 引数2
  [3] 0x565C08 add eax,0x21 ; ret
  [4] 0x565C11 add eax,0x25 ; ret
0x565CC0(idx): idx<0x28 → [0xB12758+idx*4][0] ; 0x28<=idx<0x48 → [0xB12868+…]
```

### 2.3 V-571-3 カウンタ書手

```
0x5C4F3C mov dword [0xB3B5D4], 0xC8   ; ★ 唯一の直接ストア（RaceSexMenu ctor 0x5C4E40）
```
（AddPane `0x5C438A add [0xB3B5D4],1` は RMW のため直接ストア走査に出ない。）

### 2.4 V-570-6 `[0xB3B4C9]` 書手

```
0x5C9FB1 movb [0xB3B4C9], imm   ; Reset 実行 0x5C9CD0
0x5CCCBC movb [0xB3B4C9], imm   ; 通知ハンドラ 0x5CC200
0x5CD9CA movb [0xB3B4C9], imm   ; 通知ハンドラ 0x5CC200（末尾）
```

### 2.5 V-567-2/3 分岐の実体

```
0x5CC560.. fcomp ; 5CC57A jnp 0x5CC9C4       ; 比較一致 → 0x5CC9C4
0x5CC580 mov edx,[0xB39008] ; push 0xfa8 ; call 0x5C3110 ...  ; 別ページ名
0x5CC810 jnp 0x5CC97F
0x5CC829 mov eax,[0xB38F70] ; push 0xfa8 ; ... 0x588BD0 ; push 0xfae ...
0x5CC863 mov edx,[0xB38F70] ; push 0xfd0 ; ...              ; 同一 page の別 trait(user0/listindex)
```

---

## 3. 残点（更新後）

- **V-558-4【低】**: GMST の ESM 既定値（`Oblivion.esm` GMST ブロック要読取）。
- **V-559-1【低】**: health(AV8) 固有 trait スロットの pin 留め。
- **V-559-4【低】**: `[item]+0x34` の bit フラグ名（本帯では 4B フィールド用途）。
- **V-569-5【低】**: `0x5DC8F3-FC` 3 命令の目的（dead に見える）。
- **V-572-1【低】**: `0x66xxxx` 系3システムの用途同定。
- **V-573-2・V-599-2・V-598-4・V-601-1【低】**: 動的領域/実用価値低（打切り継続）。

---

## 4. 検証

- probe: **u16_1〜u16_3（3本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x4D7F80`（`[+4]==0x23` formType 述語）／`0x5E3320`（`[+0x104]` 返却）／`0x51C090`（`this+0x44` 7要素線形探索・`this+0xC` formID）／`0x51C0F0`（`[this+idx*4+0x44]=av`）／`0x565BE0`（jump table `0x565C30`）／`0x5C4F3C`（`[0xB3B5D4]=0xC8` 唯一書手）／`0x5C9FB1`/`0x5CCCBC`/`0x5CD9CA`（`[0xB3B4C9]`）／`0x5CC580`/`0x5CC829`/`0x5CC863`。
