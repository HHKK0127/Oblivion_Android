# v422 — `FUN_006c63a0`（+6 property_type reader）と first-walk payload の裁定（V-422-1 完遂・派生残点第14弾）

- 採番: **v422**（派生残点群・probe prefix **v422_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v421 の残点 **V-422-1【低】（`FUN_006c63a0` の body 未取得（+6 property_type 読みの [推定高]）と first walk の payload＝property オブジェクト説の最終裏取り）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`FUN_006c63a0` の identity** | **[確定]** | **`u16(entry+6)`（property_type offset）の palette 文言 reader** — decompiled L495991-495998: `if (*(u16*)((int)param_1+6) != 0xffff) return *(u16*)((int)param_1+6) + *(int*)(*param_1+8); return 0;` ＝**offset==0xFFFF なら0、否则 palette base+offset**。→ v421 の family 並び [推定高]（+6）を **[確定]** に引き上げ。offset-reader family 確定: **+6=`FUN_006c63a0`／+8=`FUN_006c63c0`（CtlrType）／+0xA=`FUN_006c63e0`（CtlrID）**（+4 node_name は machine 使用形で確定済・`FUN_006c6380` は annotation 不在） |
| **first walk の payload＝property オブジェクト説** | **[確定(命令)]** | machine 0x6C96EE-0x6C972D で完全確認: **`movzx eax,[ebx+6]; cmp ax,0xffff; je 0x6C9767`（property_type==0xFFFF なら first walk 全体 skip）** → `property_type 文言 = palette[ebx]+offset` → `node[+0x9C]` list を walk（`payload=[edi+8]`・`next=[edi+0]`＝v418 の FUN_0047C930 list 構造と同一）→ **`payload->slot1()`（thiscall ecx=payload）→ descriptor pair → `pair[0]`（クラス名）を property_type 文言と strcmp** → 一致で `[esp+0x1c]=payload`。→ **payload は「クラス名が property_type 文言と一致する typed object ＝ property 本体」[確定]**（v421 の [推定高] を確定） |
| **second walk の container（残り）** | **[部分確定]** | 一致時＝**property `payload[+0xC]`**／不一致・property なし時＝**resolved node `[ebp+0xC]`**（`[esp+0x1c]` の初期値＝node のまま）→ どちらも `{bucket [+0xC]・next [+0x34]・slot1 descriptor}` walk（v421 確定）→ **node `[+0xC]` が NiAVObject の layout 上どこに当たるかは V-422-2 へ** |

## §2 証拠（v422_1）

### §2.1 `FUN_006c63a0` body（L495991-495998）
```c
int __fastcall FUN_006c63a0(int *param_1) {
  if (*(ushort *)((int)param_1 + 6) != 0xffff)
    return (uint)*(ushort *)((int)param_1 + 6) + *(int *)(*param_1 + 8);
  return 0;
}
```
- `param_1`＝配列B entry（`[0]`=NiStringPalette* → `[+8]`=string base）。**byte+6 ＝ property_type offset** [確定]。0xFFFF＝なし [確定]。

### §2.2 first walk machine（0x6C96EE..0x6C972D）
```
6C96EE: movzx eax, word [ebx+6]        ; property_type offset
6C96F2: cmp ax, 0xffff
6C96F6: je 0x6c9767                    ; ★ なし → first walk skip（ebp=node のまま second walk へ）
6C96F8: mov ecx, [ebx] / movzx eax,ax / add eax,[ecx+8]   ; property_type 文言
6C9702: mov edi, [ebp+0x9c]             ; resolved node の +0x9C list（v418 構造）
6C9708: test edi,edi / je 0x6c9767
loop:
6C9710: mov ebp, [edi+8]                ; payload（list node: [+0]=next, [+8]=payload）
6C9713: mov edx, [ebp] / 6C9719: mov eax,[edx+4]  ; slot1 = descriptor getter
6C971C: mov edi, [edi]                  ; next
6C971E: mov ecx, ebp                    ; thiscall this = payload
6C9720: call eax                        ; pair = payload->slot1()
6C9726: call 0x6c63a0                   ; property_type 文言
6C972B: mov ecx, [esi]                  ; pair[0] = クラス名
        strcmp(pair[0], property_type 文言) → 一致で [esp+0x1c]=payload（6C975F）
```

## §3 残点 **V-423-1..2**（V-422 引き継ぎ）
1. **【低】resolved node `[+0xC]` の layout 謎（旧 V-422-2）** — fallback walk が NiAVObject `[+0xC]` を bucket として読むが、Gamebryo 布局では flags 帯のはず → このエンジンの NiAVObject layout（`[+0xC]`＝list 先頭？）の特定と、**`Bip01[+0x1C]`（epilogue）との関係**（同一 container か）。
2. 【低】`FUN_006c6380`（+4 node_name reader）の annotation 不在分の machine body（使用形は既確定）。

## §4 検証・自己批評
- probe: **v422_1 全 EXIT=0**（1本で V-422-1 完遂）。
- **決定打の作り**: (i) **decompiled body 実読**で offset（+6）と 0xFFFF ガードを確定 → (ii) **machine の guard（`cmp ax,0xffff; je`）と slot1 呼出し（`mov ecx,ebp; call`＝thiscall）・strcmp 構造**を linear decode で復元 → (iii) v418 の list 構造（`[+0]=next/[+8]=payload`）と突合。
- 妥当性チェック: ①+6 reader ⇔ machine の `[ebx+6]` read ⇔ 0xFFFF guard — ②payload slot1 pair[0] ⇔ v419 descriptor pair 構造 ⇔ property_type 文言 strcmp — ③skip ターゲット `0x6C9767`（test ebp → 6C9852）⇔v421 確定の second walk 入口 — **三方向矛盾なし**。
- docs との不整合: **なし**（v421 の [推定高]2件を [確定] へ昇格）。docs/GT 反映は #47/#20 一括。
- **教訓**: **offset-reader ファミリ（連番 addr・同型 body）は1本の body を確定させれば並びの [推定高] が全部 [確定] になる**（63A0/+6・63C0/+8✓・63E0/+0xA✓）／**skip ターゲット先（`je → 0x6C9767`）まで読むと「あり／なし」分岐の全体像が同時に取れる**。
