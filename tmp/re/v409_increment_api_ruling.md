# v409 — V-406-1 部分解: skill 増分 API の登録構造と opcode 特定の裁定（派生残点）

- 採番: **v409**（派生残点群の第1弾・probe prefix **v409_1..v409_6 の6本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v406 の残点 **V-406-1【高】（「+10 初期適用 fn 未特定・間接登録テーブル `.rdata 0xA73DA8` の同定と opcode id・`FUN_0050D020` の opcode 特定）」** を消化（部分解: opcode 特定と登録構造を確定、「+10 適用 fn 本体」は残点として再定義＝V-409-1）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`FUN_0050D020` の opcode 特定** | **[確定(実文字列)]** | **script opcode 3本の共通 impl**: **`AdvancePCSkill`（完全名）/`AdvSkill`（alias）＝opid `0x10D4`**・**`AdvancePCLevel`/`AdvLevel`＝`0x10D5`**・**`ModPCSkill`/`ModPCS`＝`0x110F`（help `'Modify the Players Skill. [modpcs Blunt, 10]'`）** — impl は3本とも `FUN_0050D020`、第2 fn は3本とも `FUN_004FDE30`（flags `0x20000`・container `0xB0A900`・stride **0x28** レコード）。隣接 `ModPCAttribute`/`ModPCA`＝`0x1110`（help `'Modify an actor value. [modpca luck, 10]'`・impl `FUN_005056F0`＝別） |
| **間接登録テーブル `.rdata 0xA73DA8` の正体** | **[確定(RTTI)]** | **PlayerCharacter の vtable（head `0xA73A0C`・RTTI `.?AVPlayerCharacter@@`・232 slots）の index 231（+0x3BC）エントリ** — head は ctor `FUN_0066A740`/`FUN_0066B150` の `mov [esi],0xA73A0C` と一致。**`FUN_00668C30` = PlayerCharacter virtual slot231**（E8=0 の理由＝virtual dispatch） |
| **slot231（`FUN_00668C30`）の実行時呼出し** | **[確定(否定側)]** | **呼出し経路0件（6系統）**: ①E8 直接=0（v406_21）②生 dword data refs=vtable 格納部1件のみ（v406_22）③decompiled `+ 0x3bc))`=0件＋broader `+ 0x3bc)` は別クラスの field write 1件のみ（v409_4）④machine `call [reg+0x3BC]`（mod=10 disp32）=0件（v409_5・他7件は lea/mov の別クラス field アクセス）⑤SIB 形式は hit1件も decode 棄却（`lea ebx,[esi+0x3bc]`＝Havok 帯・FF は `xor edi,edi` の modrm 偽陽性）⑥**FF15/FF25 で vtable 範囲(0xA73A0C..0xA73DC8) を指す indirect=0 件（sealed）**（v409_6）→ **`FUN_00668C30`（skill record `[+0x38+idx*4]` テーブル×倍率→保留増分 API）は dead slot** |
| **帰結: 保留増分システムの実行有無** | **[部分確定]** | `FUN_00668B30`（`[Player+0x5B0][idx] +=` の唯一加算者）の E8=1 は **`FUN_00668C30` のみ**（v406_20）→ **slot231 が dead なら当該加算の直接経路も実質到達不能**。ただし `FUN_00668B30`/`FUN_0066C200` 自身の vtable 登録/間接呼出しは本 probe で未走査 → 「保留増分システム全体が dead」の断定は [未確定]（残点 V-409-2） |
| **「+10 初期適用 fn」本体** | **[未確定]（据え置き）** | 本6 probe では opcode 特定・登録構造・dead slot 証明まで到達。適用 fn 自体は未特定 → **残点 V-409-1 に再定義**（v406 の否定証拠群＝major getter 書手0件等と統合して次ラウンドへ） |

## §2 証拠（v409_1..v409_6）

### §2.1 opcode レコード（v409_1(b)/v409_2・.data 2 hits 実読）
```
record @0xB0E9E0 (stride 0x28):
  +0x00 'AdvancePCSkill'   +0x04 'AdvSkill'   +0x08 opid 0x10D4
  +0x0C ''(0xA2F7EC)       +0x10 0x20000      +0x14 0xB0A900
  +0x18 impl 0x50D020      +0x1C fn2 0x4FDE30 +0x20/24 0
  +0x28 'AdvancePCLevel'   +0x30 'AdvLevel'   +0x38 opid 0x10D5  …
record @0xB0F318: 'ModPCSkill'/'ModPCS'/0x110F/'Modify the Players Skill. [modpcs Blunt, 10]'/…/impl 0x50D020
next:            'ModPCAttribute'/'ModPCA'/0x1110/'Modify an actor value. [modpca luck, 10]'/…/impl 0x5056F0
```
- decompiled `FUN_004FDE30`（第2 fn）: 512B ローカルバッファ＋`param_3+0x40c` アクセス＝**opcode の別ハンドラ（コンテキスト/文字列処理系）** [部分確定・詳細は残点]。

### §2.2 テーブル特定（v409_1(a)/v409_3）
- `0xA73DA8` 前方 walk: **232 連続 code ptr → head `0xA73A0C`**（境界は 0xAC3D80）。`FUN_00668C30` の index = **231（0xE7）**。
- head refs = **`FUN_0066A740`（PlayerCharacter ctor）と `FUN_0066B150` の `mov [esi],0xA73A0C`** ✓（v406_20(b) の RTTI `.?AVPlayerCharacter@@` と整合）。
- `0xA73DB0..` の float 定数（0.89/−1.97/2.85/96/0.16）は vtable 直後の別データ（vtable 末尾境界）。

### §2.3 dead slot の6系統否定（v406_21/v406_22/v409_4/5/6）
| 経路 | 結果 |
|---|---|
| E8 rel32 直接 call | **0 件** |
| 生 dword data refs | vtable 格納部 `0xA73DA8` のみ（実行時参照なし） |
| decompiled virtual 表現 `+ 0x3bc))` | **0 件** |
| machine `FF /2 + disp32 0x3BC`（mod=10） | **0 件**（他の7件は lea/mov/cmp の field アクセス） |
| SIB 形式 | hit 1件も decode で `lea ebx,[esi+0x3bc]`（0x896D25・Havok 帯）＝偽陽性棄却 |
| FF15/FF25 imm ∈ vtable 範囲 | **0 件（sealed）** |

## §3 残点 **V-409-1..3**（＋V-406-1 の再定義）
1. **【高】「+10 初期適用 fn」本体（＝V-406-1 再定義）** — v406 の否定群（major getter 書手0件・setter E8=2）と本 probe の「保留増分 dead」を統合し、**残る書手系（`FUN_0065FA40` の2 callers 以外の間接・`PlayerCharacter` 他 vtable スロットの skill 値書込経路）**を再走査。特に `[Player+0x130+idx*4]` の **間接（virtual）書手**と chargen finish 系 fn（`FUN_00597540`/`FUN_0059FF60` machine 帯の未解析部分）。
2. 【中】**保留増分システム全体の実行有無** — `FUN_00668B30`/`FUN_0066C200` の vtable 登録有無と間接呼出し（slot231 dead の連鎖確認）。
3. 【低】`FUN_004FDE30`（opcode 第2 fn）の identity と `0x20000` flags/`0xB0A900` container の意味。

## §4 検証・自己批評
- probe: **v409_1..v409_6 の6本全 EXIT=0**。
- **決定打の作り**: (i) data ref の隣接 dword window で opcode レコード構造（stride/flags/impl 列）を解読 → (ii) name ptr 実読で opcode 名と opid を確定 → (iii) fn ptr テーブルの後方 walk で head を求め、**head refs（ctor の vptr 設定）＋RTTI で vtable 同定** → index 逆算 → (iv) **dead 判定は6系統の否定 census**（E8/data/decompiled/disp32/SIB/FF15・毎回 decode で偽陽性棄却）。
- 妥当性チェック: ①opcode レコード2種の stride 一致（0x28）⇔隣接 name 実読 — ②table head⇔ctor 設定⇔RTTI — ③index 231⇔`+0x3BC`⇔disp32 census — **三方向矛盾なし**。
- docs との不整合: **なし**（v406 の V-406-1 を部分消化＋再定義）。docs/GT への反映は #47/#20 一括。
- **教訓**:
  1. **「E8=0 の間接 fn」の登録先は data ref の隣接 window から構造を解読**（opcode レコード・vtable・dispatch 表を dword 並び＋実読で識別）。
  2. **fn ptr テーブルの head は後方 walk で、その head の refs＝（多くは）ctor の vptr 設定 → RTTI で即同定**（vtable と dispatch 表の区別）。
  3. **dead slot 判定は単一 census でなく多系統**（E8/生 data/decompiled/disp32/SIB/FF15-FF25）— SIB・偽陽性は毎回 decode で棄却。
  4. FF バイトは `xor r,r` の modrm でも出現する → **bytes pattern hit は必ず capstone decode で検証**。
