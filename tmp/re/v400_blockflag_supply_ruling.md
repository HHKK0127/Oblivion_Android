# v400 — `[block+0xD]` の非0xFF供給元の裁定（#153 ⑥-6・V-386-1）

- 採番: **v400**（ai-todo #153 束の第11弾＝低10件の第6弾、probe prefix v400_1..v400_6）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v386 の残点 **V-386-1（`[block+0xD]` の非0xFF供給元 — 帯内はコピーのみ＝既定0xFF、初回に非0xFF を書く場所）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| 帯内（0x6C6000..0x6CA000）の non-0xFF 書手 | **[確定(否定側)]** | **0件**（v386 の imm census＋本 v400 の **reg ストア（`88 /r disp=0x0D`）全 .text census**で帯内を再確認）。帯内在在の `+0xD` 書手はすべて: `0x6C68FB`（copy）／`0x6CA0FA`（copy）／`0x6CD411`（**別構造**: v394 container の count）／`0x6CCE9A`（別構造 ctor）＝**非0xFF の直接代入は帯内に不在** |
| 候補の否定（3件） | **[確定負]** | ①**`FUN_006cce40`（`+0xd ← 第4引数`）＝別クラス族の base ctor** — vtable +0 型・6派生クラス（`0xA79584`/`0xA79E54`/`0xA7A49C`/`0xA7C454`/`0xA7C504`/`0xA7C5B4` が caller 後に自 vtable 設定）・layout `{+0xc=0, +0xd=param_4, +0xe=0, +0xf=0xFF, +0x10/11=0x80}` は block と不一致 ②**`0x7343A4`/`0x734142`（stride16 へのストリーム格納）＝ピクセル/テクスチャデコーダ**（`FUN_00734170`/`FUN_00733f90`＝u16 読み→**RGB565 bit unpack**（`(u>>8)&0xf8` 等）・`local_14[3]=0xFF`＝alpha）③**`FUN_006c6610`＝block 抽出 fn**（`*param_2 = block[i].ptr`＋AddRef 後に `FUN_006c6300(slot)` で slot クリア＋`+0x18` 配列を0xFFFF化＝take-out で供給元ではない） |
| 伝播経路（非0xFFがどこを通るか） | **[確定(命令＋decompiled)]** | **source container の entry（`edi`/`piVar13`）→ dest container の `block[+0xd]` へのコピーが2所**: ①`FUN_006c9f10`（L499164 `*(iVar8+0xd) = *(piVar13+0xd)`・`[vtable+0x74]` count ループ内・ptr/`+4` ref とセットで転送）②**`0x6CA0FA` ループ**（container `[+0x14]` 配列 stride16 へ `cl = [edi+0xd]` を格納・`[esi]→[+0x74]/[+0x80]/[+0x90]` virtual getter で照合→一致時に `[edi+8]` ptr／`[+4]` name ref／**`[+0xd]`** を転送・FUN_006c9f10 内と推定高）＝**コピーの連鎖** |
| setter/getter 系の発見（+0xd の操作系） | **[確定(命令＋decompiled)]** | **{Getter `FUN_00468260`＝`return byte[+0xd]`／Setter `FUN_00468270`＝`byte[+0xd] ← arg`（E8=1＝`FUN_005222d0` 内のみ・non-virtual）／Clear `FUN_0046CAD0`＝`+0xc=+0xd=0`（E8=0・vtable slot×4: file 0x63B278/0x641D84/0x641FA8/0x642104）}**。**setter の値源＝`FUN_004a9700 = return byte[+0x69]`**（`FUN_005222d0` 内: `[param_1+0x104]` オブジェクトの `+0x69` フィールドを getter → `FUN_00468270` へ渡す（L216913-216919）） |
| 原点 | **[未確定]** | **最終原点＝`[+0x69]` フィールド（+0x104 オブジェクト）の意味と設定元**。0x468xxx descriptor ファミリ→block への接続（`edi` source の正体）は[推定高]＝残点 |

**V-386-1 への回答**: 非0xFFは**帯内に直接書手を持たず [確定(否定側)]、`FUN_006c9f10`/`0x6CA0FA` の source entry からのコピー連鎖で伝播 [確定]**。setter 系（`FUN_00468270`・値源 `byte[+0x69]`）を特定したが、**block への最終接続と `[+0x69]` の identity は未確定＝残点**。判定は **[部分確定]**。

---

## §2 証拠

### §2.1 帯内 census（v400_1(a)/v400_2(a)）

- `C6 /0 disp=0x0D imm≠0xFF` 全 .text＝**帯内0件**（5件は全て別帯: `0x5BBF91`/`0x682097`/`0x6820F1`/`0x9806B7`/`0x980CDC`）。
- `88 /r disp=0x0D` 全 .text＝帯内在は **`0x6C68FB`・`0x6CA0FA` の2件のみ**（＋`0x6CD411`/`0x6CCE9A` は別構造）。

### §2.2 否定3件

| 候補 | 証拠 | 判定 |
|---|---|---|
| `FUN_006cce40` | L501467-501496: `param_1+0xd ← param_4`・caller6本が各 vtable を後置設定 | **別クラス base ctor** [確定負] |
| `0x7343A4`/`0x734142` | `FUN_00734170`/`FUN_00733f90`（L578986/578918）: stream u16 読み→**RGB565 unpack** | **ピクセルデコーダ** [確定負] |
| `FUN_006c6610` | L496165-496195: `*param_2 = [arr+i*0x10]`＋AddRef→`FUN_006c6300(slot)`＋0xFFFF 化 | **抽出 fn** [確定] |

### §2.3 伝播経路（v400_2(c)/v400_3(a)/v400_4(a)）

```
FUN_006c9f10（L499152-499169）: [vtable+0x74] count ループ内
  ref 転送（+4）＋ ★L499164: *(iVar8+0xd) = *(piVar13+0xd)
0x6CA0FA（同 fn 内と推定高）:
  6CA0A8: call 0x6c94e0（index 解決）→ 6CA0B3: ebx = [ebp+0x14] + idx*0x10
  6CA0B8: [ebx+8] = [edi+8]（ptr）→ name ref 転送 → ★6CA0F7: cl = [edi+0xd]; [ebx+0xd] = cl
  6CA105: stride +0x10・count = [container+0xc]
```

### §2.4 setter/getter 系（v400_4(b)/v400_5(c)(d)/v400_6）

```
FUN_00468260: return byte[param_1+0xd]                 （getter・fastcall）
FUN_00468270: byte[ecx+0xd] = [esp+4]; ret 4           （setter・E8=1: 0x522741）
FUN_0046CAD0: +0xc=0; +0xd=0; ret                      （clear・vtable slot×4）
FUN_005222d0 内（L216913-216919）:
  FUN_00468230([+0x104]+0x64); FUN_004684c0(FUN_0051beb0([+0x104]));
  uVar14 = FUN_004a9700([+0x104]); FUN_00468270(uVar14)   ★
FUN_004a9700: return byte[param_1 + 0x69]              （★値源＝+0x69 getter）
```

## §3 残点 **V-400-1..3**
1. **`[+0x69]` フィールドの identity**（`+0x104` オブジェクトの何＝game状態か・設定元）【中】
2. **0x468xxx descriptor ファミリ→block の接続証明**（`FUN_006c9f10`/`0x6CA0FA` の source `edi`/`piVar13` が descriptor かの直接照合）【中】
3. **`FUN_0046CAD0` Clear の vtable slot 位置と4つの vtable（`0x63B278`/`0x641D84`/`0x641FA8`/`0x642104`）のクラス同定**【低】

## §4 検証・自己批評
- probe: **v400_1..v400_6 全 EXIT=0**。
- **決定打の作り**: (i) imm/reg 両形式の全 .text censusで帯内0件を再確定 → (ii) **候補3件を layout/decompiled で否定** → (iii) 伝播2か所（`FUN_006c9f10` L499164＋`0x6CA0FA` ループ）を machine/decompiled で特定 → (iv) **setter/getter pair と値源 `byte[+0x69]` を特定**。
- 妥当性チェック: ①帯内0件⇔v386 の imm census — ②`FUN_006c6300` が0xFF 既定⇔setter 系は別ファミリ（Clear=0）— ③`0x6CA0FA` の stride16⇔container `[+0x14]` — ④setter E8=1⇔decompiled 呼び出し1件 — **四方向で矛盾なし**。
- docs との不整合: **なし**（v386 の V-386-1 を**[部分確定]**まで前進。原点は残点）。
- **教訓**: **「stride16 へのストリーム格納」はピクセルデコーダ等の別構造と紛れやすい → bit-unpack パターン（`&0xf8`/`&0xfe`）と `local[3]=0xFF`（alpha）で識別して棄却**。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
