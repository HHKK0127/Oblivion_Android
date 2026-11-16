# v417 — global `0xB3CD7C` の使用形体系分類と getter thunk の同定（V-416-1 部分解・派生残点第9弾）

- 採番: **v417**（派生残点群・probe prefix **v417_1..v417_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v416 の残点 **V-416-1【中】（`0xB3CD7C` global の正体）** を追跡（使用形11 refs の体系分類と getter thunk の特定まで到達・singleton 型は残点縮小）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0xB3CD7C` の使用形（.text refs 11件の体系分類）** | **[確定]** | **5形式**: ①**map lookup 経由 `push 0xB3CD7C; call 0x700010`＝4件**（`0x4732EA`(FUN_00473200)／`0x47F762`(FUN_0047F750)／`0x4E3EC6`／`0x6C825B`(FUN_006C8220＝NiTransformController 登録確認)）②**インライン等値比較 `cmp eax,0xB3CD7C`＝4件**（`FUN_004730B0`@0x473180／**`FUN_0047C930`×2**@0x47C9B3/0x47CA50（float set 系）／`FUN_0047CC80`@0x47CC95）＝**リスト走査の key 一致チェック（`FUN_00700010` 内ループのインライン版）** ③**getter thunk `FUN_006C5F50` ＝ `mov eax,0xB3CD7C; ret`**（E8=0）④**value read `mov eax,[0xB3CD7C]` → `FUN_0070F910`（出力系）＝1件**（`0x6D0450`・未注記）⑤`0x4969E1`（FUN_004964F0 内・境界ズレ decode で context 不明） |
| **getter thunk `FUN_006C5F50` の所属** | **[確定(RTTI)]** | **vtable `0xA79984` の slot1（+0x4）・RTTI `.?AVNiMultiTargetTransformController@@`** — data ref は **`.rdata 0xA79988` の1件（＝vtable 格納部）のみ**・E8=0・refs to `0xA79988`＝0件（vtable 自体は他に参照されない slot） |
| **`[0xB3CD7C]`（値）の性質** | **[推定高]** | value read の唯一消費者は `FUN_0070F910`（**v401 で確定の dump/文字列出力ヘルパ**）へ渡す → **`[0xB3CD7C]` は出力対象の値（文字列/数値 singleton）** |
| **singleton の型（最終確定）** | **[未確定]**（残点 V-418-1） | 使用形から **「NiAVObject のコレクション（map）に singleton key として登録されたオブジェクト」[推定高]**（float 設定系・flag set 系・NiTransformController 登録確認・StoreTargets epilogue が node を探す対象）だが、`0x6D0450`（未注記）の identity と FUN_0070F910 の arg 用途で型を確定させられず |
| **usage 帯の分布** | **[確定]** | float 設定系 `0x47C930`/`FUN_004730B0`/`FUN_00473200`・flag set `0x4E3EC6`（`or word [eax+8],0x40`）・`FUN_0047F750`（取得 node→`FUN_0047C710`→`cmp [0xA7A160]`）・**`FUN_006C8220`（NiTransformController 登録確認・v371）**・**StoreTargets epilogue（`FUN_006C6910`）**・出力 `0x6D0450` |

## §2 証拠（v417_1..v417_3）

### §2.1 使用形5形式（v417_1）
- lookup 経由: `mov ecx,[esi+4]; push edi; push 0xB3CD7C; call 0x700010`（0x4732E6-0x4732EF 等）。
- インライン等値: `cmp eax,0xB3CD7C; je`（0x473180/0x47C9B3/0x47CA50/0x47CC95）＝`FUN_00700010` の `list==key` 比較を各 fn がインライン実装。
- getter thunk: `6C5F50: mov eax,0xB3CD7C; ret`。
- value read: `6D045F: mov eax,[0xB3CD7C]; push eax; call 0x70F910`。

### §2.2 vtable 判定（v417_3）
- RTTI backward scan: **vtable `0xA79984` slot1（+0x4）RTTI `.?AVNiMultiTargetTransformController@@`**。neighbour: slot0=`0x6C5F60`（dtor 系）・slot2-4=`0x6F7070`（恒0系）・slot5+`0x6D02E0/0x6D0370` — vtable 構造と整合。

### §2.3 出力ヘルパの受け手（v417_2(c)）
- `0x6D0450`（未注記 fn）が `[0xB3CD7C]` の値を `FUN_0070F910` へ（v401 の「dump ヘッダ文字列出力」と同 fn）。

## §3 残点 **V-418-1..2**
1. **【中】`0xB3CD7C` singleton の型** — `0x6D0450`（未注記）の identity と `FUN_0070F910` の arg 用途（文字列 or 数値）＋`FUN_0047C930` 帯の float set で何を set しているか（該当 node のフィールド）から型を確定。
2. 【低】V-416-2/V-415-2 引継ぎ: `L498845` slot23 呼び出しと `manager[+0x30]` 正体。

## §4 検証・自己批評
- probe: **v417_1..v417_3 の3本全 EXIT=0**（V-416-1 は [部分確定]＝体系分類と thunk 特定まで）。
- **決定打の作り**: (i) **refs 全数の context decode（lookup/等値/thunk/value read の5形式へ分類）** → (ii) **data ref が1件のみ＝vtable 格納の兆候 → RTTI backward scan で slot1+class 即同定** → (iii) value read の消費者 fn（FUN_0070F910＝v401 既知）から値の性質を推定。
- 妥当性チェック: ①等値比較4件⇔`FUN_00700010` の内側ループと同型 — ②thunk `mov eax,imm; ret`⇔vtable slot1⇔data ref1件 — ③usage 帯（float/flag/登録確認/epilogue）⇔v371/v405 の既知経路 — **三方向矛盾なし**。
- docs との不整合: **なし**（v416 の V-416-1 を部分解）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **data ref が「1件のみ」は vtable 格納の強兆候** → **ref 位置そのものを RTTI backward scan して slot と class を即同定**（v409 の「テーブル head 後方 walk」と表裏の技）。
  2. **key の使用形は「push imm→call lookup」と「cmp imm→je」の2形式で横断 census**（インライン化と関数化が併存する）。
  3. **thunk getter（`mov eax,imm; ret`）は E8=0 でも vtable 登録（data ref）で十分**（v409 slot231 の dead 判定と対照: こちらは slot 登録＋vtable 保持＝機能性のあるスロット）。
