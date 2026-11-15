# v389 — `0x73A5E0` の identity の裁定（#152 ⑧・V-376-2・束の最後の1件）

- 採番: **v389**（ai-todo #152 束の第8弾・低優先帯の最終、probe prefix v389_1..v389_2）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v376 の残点 **V-376-2（`0x73A5E0`（state 遷移時呼出）の identity）** を消化 — **#152 束8件の最終**
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| `0x73A5E0` の identity | **[確定(命令)]** | **重複排除付き ptr 配列セット追加（`NiTSet::Add` 相当）** — `ret 4`＝1引数（値へのポインタ）。this 布局: **`[this+0]`=array／`[+4]`=capacity／`[+8]`=count**。本体: **線形探索で既存一致検出（`cmp [ecx],edi; je`＝重複なら即 return）→ 満杯なら capacity×2（0 なら 1）で `0x6E8CA0` realloc → `[array+count*4]=value; count++`**。decompiled L583100 `void __thiscall FUN_0073a5e0(int *param_1,int *param_2)` |
| state 遷移時の呼び出しの意味 | **[確定(命令)]** | **`this = [seq+0x40]（manager）+ 0x4C`**（3 call site 全員が `mov ecx,[esi+0x40]; add ecx,0x4c`）＝**manager の有効 sequence セットへ sequence を登録**。layout 一致: `[mgr+0x4C]`=array／`[mgr+0x50]`=capacity／**`[mgr+0x54]`=count ⇔ `0x6C4200`/`0x6C4480` が読む array/count と完全一致** |
| 呼び出し条件 | **[確定(命令)]** | 全 site が **`mov [esp+NN],esi(=sequence); call` の直前に `sete al; test al; je` ガード**（＝**state が 0 から変わった＝新規 activate のときのみ**）。→ **新規 activate の sequence を manager の有効セットへ Add** |
| セット管理の全体図 | **[確定(命令)]** | **Add＝`0x73A5E0`（新規 activate 時・state 0→N）**／**Remove＝`0x6C4200`（state==0 を前方詰めで除去）**／**force-Deactivate＝`0x6C4480`（state4 を即時 Deactivate）** — 3者が**同一 container（mgr+0x4C の {array,cap,count}）**を管理 |

**V-376-2 への回答**: `0x73A5E0` は **generic な重複排除セット追加（17 callers＝全体で利用）**で、state 遷移時の用途は **manager の有効 sequence セットへの登録**（新規 activate のときのみ）[確定(命令)]。**#152 束8件（①〜⑧）すべて消化**。

---

## §2 証拠

### §2.1 本体（v389_1(a) unfiltered）

```
73A5E0: ebx = [esp+8]（arg1＝値へのポインタ）
73A5E6: esi = ecx（this＝{array,cap,count}）
73A5E8: edx = [esi+8]（count）
73A5F2: edi = [ebx]（挿入値）
73A5F4: ecx = [esi]（array）
loop: cmp [ecx], edi → je 既存あり（→ return・重複排除）
      eax++ / ecx += 4 / jb loop
73A60B: eax = [esi+4]（capacity）
73A60E: cmp edx, eax; jne skip-grow
73A616: grow = capacity*2（`add eax,eax`・0 なら 1）
73A620: call 0x6E8CA0（realloc）
73A627: [array + count*4] = [ebx]（append）
73A631: [esi+8]++（count++）
73A638: ret 4
```
- decompiled: `L583098: // FUN_0073a5e0 @ 0x73a5e0` ＋ `L583100: void __thiscall FUN_0073a5e0(int *param_1,int *param_2)`。

### §2.2 state 遷移3 site の this（v389_2 unfiltered）

```
(a) Activate:    6C9C35: sete al; test al; je skip
                 6C9C3C: lea edx,[esp+0x20]; push edx
                 6C9C41: mov ecx,[esi+0x40]; add ecx,0x4c   ; ★this = manager+0x4C
                 6C9C47: mov [esp+0x24],esi                 ; local = sequence
                 6C9C4B: call 0x73A5E0
(b) wrapper:     6C9D87: sete al; test al; je skip
                 6C9D8E: mov ecx,[esi+0x40]; lea eax,[esp+0x18]; push eax
                 6C9D96: add ecx,0x4c; mov [esp+0x1c],esi
                 6C9D9D: call 0x73A5E0
(c) state4 path: 6CAB02: sete al; test al; je skip
                 6CAB09: mov ecx,[esi+0x40]; lea eax,[esp+8]; push eax
                 6CAB13: add ecx,0x4c; mov [esp+0xc],esi
                 6CAB1A: call 0x73A5E0
```

### §2.3 container 一致（三方向）

| 出典 | array | count |
|---|---|---|
| `0x73A5E0` 布局（this=mgr+0x4C） | `[mgr+0x4C]` | `[mgr+0x54]`（+8） |
| `0x6C4200`（update・除去） | `mov ecx,[esi+0x4C]` | `cmp [esi+0x54]` |
| `0x6C4480`（state4 一括 Deactivate） | `mov eax,[edi+0x4C]` | `cmp [edi+0x54]` |

→ **3者が同一 container** [確定(命令)]。E8 callers of `0x73A5E0`＝**17本**（sequence band 7本＝`0x6C9C4B`/`0x6C9C85`/`0x6C9D9D`/`0x6C9E8F`/`0x6C9EC9`/`0x6C9EFA`/`0x6CAB1A`＋他システム10本＝`0x56BB1F`/`0x6EBFDF`/`0x73ABD0`/`0x75E39F`/`0x7605F2`/`0x772623`/`0x772E62`/`0x772E92`/`0x772EB2`/`0x7736DF`）＝**generic utility**。

## §3 残点 **V-389-1**
1. **`0x73A5E0` の他システム側 callers のうち `sub [global],1` が直後にある3本（`0x56BB1F`/`0x6EBFDF`/`0x75E39F`・各 0xB3A67C/0xB3ECFC/0xB41DFC をデクリメンタ）の意味**（登録と同時にグローバル refcount を減らす文脈）【低】

## §4 検証・自己批評
- probe: **v389_1・v389_2 全 EXIT=0**。
- **決定打の作り**: (i) 本体 unfiltered（dedup＋grow＋append）→ (ii) E8 callers census（17本）→ (iii) **state 遷移3 site の unfiltered で `this=mgr+0x4C` を全件確認** → (iv) **container の array/count が 0x6C4200/0x6C4480 と一致することを3方向で突合**。
- 妥当性チェック: ①`sete al` ガード⇔「新規 activate 時のみ」⇔v378 の新規遷移判定 — ②`[mgr+0x54]` count⇔0x6C4200/0x6C4480 — ③dedup⇔Add が二重登録しない — **三方向で矛盾なし**。
- docs との不整合: **なし**（v376 の V-376-2 を解消）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
