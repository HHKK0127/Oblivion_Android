# v397 — `0x6C4480` の this と `0x6C4200` の同一性の裁定（#153 ⑥-3・V-384-2）

- 採番: **v397**（ai-todo #153 束の第8弾＝低10件の第3弾、probe prefix v397_1..v397_6）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v384 の残点 **V-384-2（`0x6C4480` の this と `0x6C4200` 同一性）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **同クラス性** | **[確定(命令＋decompiled＋RTTI)]** | **両 this はともに NiControllerManager**。証拠: ①`FUN_006c4480`（sweep）は **`[this+0x54]`（count）と `[this+0x4c]`（配列）** を Update（`FUN_006c4200`）と**同一 semantics**（active sequence の走査）で読む（decompiled L494072-494079 逐語）②**{+0x4c, +0x54} は v395 で RTTI 確定の NiControllerManager ctor が初期化するフィールド**（`param_1[0x13]`/`param_1[0x15]`=0）③`[host+0x98]` 経由の読手が **`word[+0x46]`（count）＋`[+0x40]`（配列）**も読み（`0x4726A1`/`0x472702`）＝**manager の初期化フィールド4/4 一致** ④Update は manager vtable slot21（v395 RTTI） |
| **同一オブジェクト性（実行時）** | **[部分確定]** | sweep の this は **E8 全5 call site で `[host + 0x98]`**（`mov ecx, [reg+0x98]; call 0x6C4480`・`0x471064`/`0x471A84`/`0x473286`/`0x473B19`/`0x4746D7`）から読み込み、host は **inline sequence 配列 `[+0xA0]`（index付き）＋`[+0xAC]` seq ptr** を持ち、call 前に **`[seq+0x44]==5` ガード＋`[host+0x98]` null check（`0x47101D`）** がある。**`[host+0x98]` = NiControllerManager（上記③）** だが、**Update（slot21）の vtable dispatch site が同じ `[host+0x98]` のインスタンスを指すことの直接照合は未実施＝残点** |
| sweep の機能（再掲） | **[確定(命令＋decompiled)]** | `__thiscall FUN_006c4480(param_1, fade)`（`ret 4`）: manager の active list を走査し **`state==4` の sequence にのみ `Deactivate(fade, flag=0)`**（v393: fade>0→state3 へ降格／fade=0→即停）。callers は `fldz`（fade=0）が大半、`0x47106D` のみ実値 fade（v384 と一致） |
| （副次）`FUN_004da2a0` | **[確定(decompiled)]** | **name getter**（`__fastcall (param_1)`）: `FUN_009832e6([param_1+0x1c], 0, tables...)` の lookup 結果の `[+4]` を返し、無ければ空文字 `&DAT_00a2f7ec`（L171619-171632）＝**v396 の PlayGroup エラーで %s に使う「名前」取得 fn**（v396 残点 V-396-1 の一部を解消） |

**V-384-2 への回答**: **同クラス＝NiControllerManager は [確定]／同一オブジェクト（同一 host の同一 manager インスタンス）は [部分確定]**（sweep は全 site で `[host+0x98]` 経由・Update 側 dispatch の照合が残る）。

---

## §2 証拠

### §2.1 sweep 本体（v397_1）

```
6C4480: push esi; push edi; mov edi, ecx     ; ★this → edi
6C4486: cmp [edi+0x54], 0; jbe end           ; count
6C4490: eax = [edi+0x4c]（配列）; ecx = [eax+esi*4]（sequence）
6C4496: cmp [ecx+0x44], 4; jne skip          ; ★state==4 のみ
6C44A6: call 0x6c9cb0（Deactivate(fade=[esp+0xC], flag=0)）
6C44AB: loop → ret 4
```
- decompiled L494063 `// FUN_006c4480 @ 0x6c4480`／L494065 `uint __thiscall FUN_006c4480(int param_1, undefined4 param_2)` — **`param_1+0x54` / `param_1+0x4c` / `entry+0x44==4` を逐語**（Update と同一フィールド群）。
- packed dword `0x006C4480`＝**0件**（非 virtual・直接 call のみ）。

### §2.2 E8 callers 全5件の this（v397_2(a)）

| site | 直前の this 設定 | ガード | 呼び出し元 fn（v397_2(b)） |
|---|---|---|---|
| `0x47106D` | `mov ecx, [esi+0x98]` | `[eax+0x44]==5`（eax=`[esi+edi*4+0xa0]`） | `FUN_00470fc0` |
| `0x471A8D` | `mov ecx, [ebx+0x98]` | `[eax+0x44]==5`（eax=`[ebx+0xac]`） | `FUN_00471990` |
| `0x47328F` | `mov ecx, [esi+0x98]` | 同 | `FUN_00473200` |
| `0x473B22` | `mov ecx, [esi+0x98]` | 同 | `FUN_00473aa0` |
| `0x4746E3` | `mov ecx, [esi+0x98]` | `[edx+0x44]==5`（edx=`[esi+0xac]`） | **`FUN_00474530`（Morph・v391）** |

- decompiled 呼び出し行: L90116/90720/91975/92316/92943（表示は stack 引数のみ＝hidden this 省略・**v394 教訓どおり機械語が正**）。

### §2.3 `[host+0x98]` の正体（v397_6(c) reader census 0x470000-0x476000）

```
0x47101D: mov ecx,[esi+0x98]; test ecx,ecx; je ...        ; null check
0x4726A1: mov eax,[ebp+0x98]; cmp word [eax+0x46],0; ...  ; ★count word
          mov eax,[eax+0x40]; mov esi,[eax+edi*4]          ; ★配列
0x472702: 同パターン再掲
0x471AEF: mov ecx,[ebx+0x98]; call 0x6C4A10                ; ★manager の count−1 fn（v395）
0x471064/0x471A84: sweep 呼び出し（上表）
```
→ **`[host+0x98]` の対象は `{+0x40 配列, +0x46 count, +0x4c 配列, +0x54 count}` を持ち、NiControllerManager ctor（v395）の初期化フィールドと4/4一致**。

### §2.4 host 側の手がかり（v397_1/2/3/5/6）

- host（esi/ebx）: **inline 配列 `[+0xA0 + i*4]`（sequence ptr 群）＋`[+0xAC]` seq ptr**、`[seq+0x44]==5`（blend-active）の sequence を対象に sweep。
- ctor 系: `FUN_00473eb0`（decompiled L92558/92582 `param_1[0x26]=0`＝`[+0x98]` ゼロ初期化）・machine `0x473EDE`（`mov [esi+0x98], ebx(=0)`）。
- **非ゼロ代入候補: `0x475C6E: mov [esi+0x98], ebp`**（出所未特定＝残点）。
- 教訓対象の census 事故: `+0x98`（=152）は **disp8（符号付き ±127）で表現不可 → disp32（mod=10）形式**。初回 census は `[ebp−0x68]`（disp8=0x98 の負値解釈）を誤検出＝修正後が正。

## §3 残点 **V-397-1..3**
1. **Update（vtable slot21）の dispatch site 特定**と「そのインスタンス＝`[host+0x98]`」の照合（実行時同一性の残り一歩）【中】
2. **host クラスの同定**（`+0x98`=manager／`+0xA0` inline seq 配列／`+0xAC` seq・ctor `FUN_00473eb0`・`+0x9c`/`+0xb4` 等の全フィールド）【中】
3. **`[host+0x98]` の非ゼロ代入元の確定**（`0x475C6E` の ebp 出所＝どの lookup/getter か）【低】

## §4 検証・自己批評
- probe: **v397_1..v397_6 全 EXIT=0**（v397_6 は初回パターンバグ→修正後）。
- **決定打の作り**: (i) sweep 本体＋decompiled（フィールド同一性）→ (ii) **E8 全5 site の this 設定を find_head 起点の align decode で特定（`[host+0x98]`）** → (iii) reader census で `[host+0x98]` の対象 layout 4/4 一致 → (iv) RTTI-確定 manager（v395）との接続。
- 妥当性チェック: ①sweep の {+0x4c,+0x54}⇔Update の {+0x4c,+0x54}⇔ctor 初期化 — ②`[host+0x98]` の {+0x40,+0x46}⇔ctor — ③`0x6C4A10`（manager 系）が同じ ecx 経由 — ④`[seq+0x44]` state 判定⇔v393 の state 意味 — **四方向で矛盾なし**。
- docs との不整合: **なし**（v384 の V-384-2 を解消）。
- **教訓**: **disp8 は符号付き（−128..127）→ +0x80 以上の offset は disp32（mod=10）で census せよ**（disp8 で走査すると `[ebp−0x68]` 等の負オフセットを誤検出しつつ本命を漏らす）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
