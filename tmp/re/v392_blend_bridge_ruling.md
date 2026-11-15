# v392 — `0x6C5C70`（lookup→blend wrapper ブリッジ）の identity の裁定（#153 ③・V-383-2）

- 採番: **v392**（ai-todo #153 束の第3弾、probe prefix v392_1）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v383 の残点 **V-383-2（`0x6C5C70`（ret 0x14・`0x6C5860` lookup 経由・callers `0x474A2F`/`0x5109E7`）の identity）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| `0x6C5C70` の identity | **[確定(命令)]** | **「lookup → blend wrapper」接続の静的ブリッジ fn** — `ret 0x14`＝**5 stack 引数＋ecx レジスタ入力**（decompiled L495514 `// FUN_006c5c70 @ 0x6c5c70`／L495516 `void FUN_006c5c70(undefined4 param_1..param_5)`＝**thiscall なしの静的 fn**）。本体2段: ①**`obj = 0x6C5860(a1, a5; ecx)`（lookup）** ②**`0x6C9D30(obj, a1, a3(float), a2(float), a4, 1.0, 1.0, 0)`（blend wrapper → Deactivate state4 → Activate state5・v379 確定）** |
| `0x6C5860` の identity | **[部分確定]** | **`0x6C5C70` 専用 lookup（E8 callers＝`0x6C5C7B` の1本）** — SEH prologue・本体に **`cmp [eax+0x44],0`（state チェック）**＋**`cmp [eax+0xC], ecx`（controller count vs レジスタ入力）**＋`call 0x6C4A10` ＝ **state/count 条件付き sequence 検索/解決** [部分確定]（完全なアルゴリズムは残点） |
| E8 callers（2本）の所属 | **[確定(命令)]** | ①**`0x474A2F`＝Morph fn `0x474530` 内の経路**（直後に `lea ecx,[eax+eax*8]; mov eax,[ecx*4+0xB102EC]`＝prologue の `0xB102E8` テーブルと**同一ファミリの ×36 index テーブル読み**・v391 の Morph ディスパッチに接続）②**`0x5109E7`（`0x51xxxx` 帯・`jmp 0x510A06` で継続）**＝別システムからの Morph/blend 要求 |
| decompiled 呼び出し例 | **[確定]** | L93028（**Morph fn `0x474530` 内**）`FUN_006c5c70(param_2, 0, param_3, 0, 0)`／L204516 `FUN_006c5c70(fVar1, 0, local_8[0], 0, 0)` |

**V-383-2 への回答**: `0x6C5C70` は **lookup（state/count チェック付き sequence 検索）の結果を blend wrapper `0x6C9D30`（state4/5 ブレンド対）へ渡す静的ブリッジ** [確定(命令)]。v383 の「NiControllerManager のブレンド系 [推定]」は**接続先（`0x6C9D30`）まで含めて確定・昇格**。

---

## §2 証拠

### §2.1 本体（v392_1(a) unfiltered 0x6C5C70..0x6C5CAE）

```
6C5C70: eax = [esp+0x14]（a5）
6C5C74: push esi
6C5C75: esi = [esp+8]（a1）
6C5C79: push eax（a5）; push esi（a1）
6C5C7B: call 0x6C5860            ; ★lookup(a1, a5; ecx=caller の register 入力) → eax = obj
6C5C80: fld1
6C5C82: ecx = [esp+0x14]（a4）
6C5C86: push 0; sub esp,8; fst 1.0 / fstp 1.0（★定数1.0×2 スロット）
6C5C92: push ecx（a4）
6C5C93: fld [esp+0x1C]（a2・float）→ sub esp,8 → fstp [esp+4]
6C5C9E: mov ecx, eax（★this = lookup 結果）
6C5CA0: fld [esp+0x28]（a3・float）→ fstp [esp]
6C5CA7: push esi（a1）
6C5CA8: call 0x6C9D30            ; ★★blend wrapper（ret 0x1c＝7引数）
6C5CAD: pop esi; ret 0x14
```
- **引数配線**: `0x6C9D30(this=obj, arg1=a1, arg2=a3, arg3=a2, arg4=a4, arg5=1.0, arg6=1.0, arg7=0)`。

### §2.2 `0x6C5860`（v392_1(b)）

- head `0x6C5860`（SEH prologue）・**E8 callers＝`0x6C5C7B` の1本**＝`0x6C5C70` 専用。
- body（filter）: **`cmp dword [eax+0x44],0`**（state チェック）／**`cmp dword [eax+0xC], ecx`**（controller count vs register 入力）／`call 0x6C4A10`／`call [0xA2807C]`（Interlocked 系）→ **state/count 条件付き sequence 検索 [部分確定]**。

### §2.3 callers（v392_1(c)）

- **`0x474A2F`**（Morph fn `0x474530` 内）: `fstp [esp]; push ebp; call 0x6C5C70` → **`mov eax,[esp+0x1C]; lea ecx,[eax+eax*8]; mov eax,[ecx*4+0xB102EC]`** — **prologue の `0xB102E8` と同一テーブルファミリの ×36 index 読み**（Morph ディスパッチの表駆動経路）。
- **`0x5109E7`**: `fstp [esp]; push ebp; call 0x6C5C70; jmp 0x510A06`（0x51xxxx 帯＝別システム）。

### §2.4 decompiled（v392_1(d)）

- `L495514: // FUN_006c5c70 @ 0x6c5c70`／`L495516: void FUN_006c5c70(undefined4 param_1,...)`（**静的 fn**）。
- `L93028: FUN_006c5c70(param_2,0,param_3,0,0)`（**fn `0x474530`＝Morph 内**）／`L204516: FUN_006c5c70(fVar1,0,local_8[0],0,0)`。

## §3 残点 **V-392-1..3**
1. **`0x6C5860` の完全な decode**（lookup の key とアルゴリズム: 何を手がかりに検索するか・`0x6C4A10` の役割）【低】
2. **ecx レジスタ入力の値と設定元**（caller `0x474A2F`/`0x5109E7` で ecx に何を入れるか・`cmp [eax+0xC],ecx` の右辺＝期待 controller count [推定]）【低】
3. **`0x474A2F` が属する path の条件**（Morph fn 内のどの分岐から来るか＝v391 の flag 経路に続く経路・`0xB333C4` グローバルチェック（`0x4749AA` 帯）との関係）【低】

## §4 検証・自己批評
- probe: **v392_1 全 EXIT=0**。
- **決定打の作り**: (i) 本体 unfiltered で **2段構え（lookup→0x6C9D30）と7引数配線を直接確認** → (ii) `0x6C5860` の E8=1 で専用lookup を証明 → (iii) callers のテーブル読み（`0xB102EC`＝prologue と同ファミリ）で Morph fn 内経路を接続 → (iv) decompiled の静的シグネチャと5引数呼び出し例を確認。
- 妥当性チェック: ①`0x6C9D30`＝v379 の blend wrapper（state4/5）⇔7引数 — ②`ret 0x14`＝5 引数⇔decompiled 5 params — ③`cmp [eax+0xC],ecx`⇔v391 の controller count — **三方向で矛盾なし**。
- docs との不整合: **なし**（v383 の V-383-2 を解消。v383 の「NiControllerManager のブレンド系 [推定]」は接続先まで確定して昇格）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
