# v384 — `0x6C4480`（state5 のときの Manager 通知）の identity の裁定（#152 ③・V-378-2）

- 採番: **v384**（ai-todo #152 束の第3弾、probe prefix v384_1）
- 対象: Oblivion.exe v1.2.0416
- 依頼経緯: v378 の残点 **V-378-2（`0x6C4480`（state5 のときの Manager 通知）の identity＝ActivateSequence の逆操作/ブレンド登録か）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| `0x6C4480` の identity | **[確定(命令)]** | **Manager の「state4 の sequence を即時 Deactivate する」一括メソッド**（`ret 4`＝1引数=time）。this（ecx）＝**`[+0x4C]`=sequence ptr 配列／`[+0x54]`=count**（**`0x6C4200` と同一レイアウト**＝同一クラス [推定高]）。本体: 全 sequence を走査し **`cmp [seq+0x44],4`（state4 のみ）** → `Deactivate(seq, time, flag=0)`（→state3）→ 常時 `xor al,al; ret 4`（false 返し） |
| 「ActivateSequence の逆操作/ブレンド登録」仮説 | **[確定(否定側)]** | 本体に**登録/追加操作は一切ない**（走査＋Deactivate のみ）＝**ブレンド登録ではない**。実体は**中断（クリーンアップ）** |
| 通知の発火パターン | **[確定(命令)]** | **E8 callers＝5本（`0x47106D`/`0x471A8D`/`0x47328F`/`0x473B22`/`0x4746E3`）全員が `cmp [seq+0x44],5; jne skip` ガード付き**＝**追跡 sequence が state5（フェード遷移中）に入った瞬間にのみ**、`mov ecx,[obj+0x98]`（Manager ptr）→ `fld time`（`0x471A8D`/`0x47328F` 等は `fldz`＝time=0）→ `call 0x6C4480` |

**実運用の解釈 [推定高]**: 新しいブレンド（state5）が開始されたとき、**以前のブレンドwrapper（`0x6C9D30`/`0x6C9E00`）が `Deactivate(flag=1)` で置いた state4 の sequence 群を、現在時刻（または0）で即時 Deactivate して掃討する**＝**ブレンド遷移時の「前のフェードアウト残骸」クリーンアップ**。v378 の「Manager 通知 [推定高]」は**方向として正しいが、内容は『登録』ではなく『state4 の中断』**。

---

## §2 証拠

### §2.1 本体（v384_1(a) unfiltered）

```
6C4480: push esi; push edi; mov edi, ecx; xor esi,esi
6C4486: cmp [edi+0x54],0; jbe exit        ; count（★0x6C4200 と同一レイアウト）
6C4490: eax = [edi+0x4C]                  ; ★sequence ptr 配列
6C4493: ecx = [eax+esi*4]                 ; sequence
6C4496: cmp dword [ecx+0x44], 4           ; ★state == 4 のみ対象
6C449A: jne next
6C449C: fld [esp+0xC]                     ; arg1 = time
6C44A0: push 0; push ecx; fstp [esp]
6C44A6: call 0x6C9CB0                     ; ★Deactivate(seq, time, flag=0)（→state3）
6C44AB: i++ / loop
6C44B3: exit: xor al, al; pop esi; ret 4  ; 常時 false・1引数
```

### §2.2 通知の発火パターン（v384_1(c)・5 callers 全員）

```
0x47106D :: cmp [eax+0x44],5; jne skip; fld [esp+0x10]; mov ecx,[esi+0x98]; call 0x6C4480
0x471A8D :: cmp [eax+0x44],5; jne skip; fldz; mov ecx,[ebx+0x98]; call 0x6C4480   （v378 再掲）
0x47328F :: cmp [eax+0x44],5; jne skip; fldz; mov ecx,[esi+0x98]; call 0x6C4480   （0x473200 帯＝v371）
0x473B22 :: cmp [eax+0x44],5; jne skip; fldz; mov ecx,[esi+0x98]; call 0x6C4480
0x4746E3 :: cmp [edx+0x44],5; jne skip; fldz; mov ecx,[esi+0x98]; call 0x6C4480
```
- **全員が `state==5` ガード**（v378 の記述を5件で再確認）＋ **Manager ptr は `[obj+0x98]`**（v371 の game fn `0x473200` の「Manager 呼び出し」と一致）。
- 引数は `fldz`（time=0・即時）が4件、`fld [esp+0x10]`（現在時刻）が1件。

### §2.3 仮説の否定と state4 の出所

- **登録操作なし [確定(否定側)]**: 本体は走査＋`0x6C9CB0`（Deactivate）のみ。`0x6C9BA0`（Activate）への呼出しは**0件**。
- **state4 の出所（v379 §2.2 再接続）**: blend wrapper `0x6C9D30`/`0x6C9E00` が先頭で呼ぶ **`Deactivate(fade=0.0, flag=1)` → state 4**（v377: `setne al; add eax,3`＝flag=1→4）。→ **state4 ＝ブレンド wrapper が前 sequence に付けた「フェードアウト中」マーク** [確定(命令)]＋[推定高]。

## §3 残点 **V-384-1..2**
1. **state 3 と 4 の意味の差**（Deactivate の byte flag: 0→3 / 1→4。`0x6C4480` は state4 を flag=0 で再 Deactivate（→state3）＝3 への遷移は何を完了させるか。`0x6C9CEF` 帯の分岐（`[this+0x34]`==−const で state0 即停）と合わせて切り分け）【中】
2. **`0x6C4480` の this と `0x6C4200` の同一性**（同一クラス＝NiControllerManager [推定高]・vtable/ctor 経由の確定）【低】

## §4 検証・自己批評
- probe: **v384_1 全 EXIT=0**。
- **決定打の作り**: (i) head `0x6C4480` の unfiltered 全 decode（走査＋state4 ガード＋Deactivate）→ (ii) E8 callers census（5本）→ (iii) **全 caller の ctx で `cmp state,5` ガードを5件すべて直接確認**。
- 妥当性チェック: ①this レイアウト（`[+0x4C]`/`[+0x54]`）⇔ `0x6C4200` — ②state4 ⇔ blend wrapper の `Deactivate(flag=1)`（v377/v379）⇔ `cmp ,4` — ③state5 ガード ⇔ v378 の「state5 のときだけ Manager 通知」— **三方向で矛盾なし**。
- docs との不整合: **なし**（v378 の「通知 [推定高]」を本 v384 で内容確定・仮説「ブレンド登録」は否定側で訂正）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
