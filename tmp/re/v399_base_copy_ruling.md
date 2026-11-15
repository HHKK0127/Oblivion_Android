# v399 — `0x6D0530`/`0x6D0550` の役割の裁定（#153 ⑥-5・V-385-3）

- 採番: **v399**（ai-todo #153 束の第10弾＝低10件の第5弾、probe prefix v399_1..v399_2）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v385 の残点 **V-385-3（`0x6D0530`/`0x6D0550` 役割）** を消化（`FUN_006d0170`（interpolator 間コピー・v394）の先頭で `call 0x6d0530`）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0x6D0530` の役割** | **[確定(命令＋decompiled)]** | **`jmp 0x715D80` の5バイト tail-jump thunk**（先頭が `jmp`・int3 パディングで独立 entry）。先の **`FUN_00715d80`＝基底クラスの直接フィールドコピー**: `FUN_00700770(dest, ctx)` 後、**`dest.word[+8] ← src.word[+8]`（flags ワード＝bit3/bit5 を含む）**＋**`dest[+0x0c..+0x24] ← src`（dword 7フィールド）**＋**`[src+0x30]` ptr の型解決付きコピー**（`FUN_0055e000` 判定＋vtable[1] 型照合→`dest[+0x30]=resolved`）（L558033-558061）。**callers 3件**: `0x6CE2CF`／**`0x6D0180`（`FUN_006d0170`＝interpolator 間コピー先頭・v394）**／`0x6D148F` |
| **`0x6D0550` の役割** | **[確定(命令＋decompiled)]** | **「vtable[1] 介在のフィールドコピー＋条件付き flag 導出」の copy 系ヘルパ fn（本体・`ret 4`）**: ①`FUN_00715f40(src)`＝**`[src+0x21C]` オブジェクトの `vtable[1]` を介した size/type-tag 付きフィールドコピー**（`(+8, size2)`/`(+0xc, 4)`/`(+0x10, 4)`/`(+0x14, 4)`/`(+0x18, 4)`... を各 `vtable[1](field_ptr, size, &tag, 1)` で転送）②**`[src+0xD8]` type-id が `0xA010068 ≤ id ≤ 0xA01006C` の範囲内のみ続行** ③`[src+0x21C]`→`vtable[1]` 問合せの byte 結果に応じて **`word[dest+8] |= 0x20` / `&= ~0x20`**（L504074-504096）。**callers 3件**: `0x6CE329`／`0x6D037B`／`0x6D0D3C`（`0x6D0530` の caller 群と**同一3つの fn ファミリが対で使用**＝コピー経路の2系統） |
| **（副次）`word[+8]` bit5(0x20) の意味** | **[確定(文字列)]** | **`IsManagerControlled`** — 隣接 fn `FUN_006d05c0`（`0x6D05C0`）が **`byte[+8] >> 5` を bool 化して `FUN_0070f950("IsManagerControlled", value)` で出力**（L504116-504117・EXE 文字列 VA `0xA7A210`）。**v395/v397 の bit3（Update gate）に続く2つ目の既知 bit** |
| （参考）`0x6D0540` | **[確定(命令)]** | `jmp 0x715820` の3番目 thunk（同帯・役割は残点） |

**V-385-3 への回答**: **`0x6D0530`＝基底直接フィールドコピーへの thunk（0x715D80）／`0x6D0550`＝vtable[1] 介在コピー＋type-id チェック＋IsManagerControlled（bit5）導出のヘルパ**。両者は**同一3 caller ファミリが対で呼ぶ2系統のコピー経路** [確定]。ついでに **`word[+8]` bit5＝`IsManagerControlled` を確定**。

---

## §2 証拠

### §2.1 `0x6D0530`（v399_1(a) machine ＋ v399_2(a) decompiled）

```
6D0530: jmp 0x715d80          ; ★5バイト thunk（int3 パディング）
```
- `FUN_00715d80(int param_1, int *param_2, undefined4 param_3)`（L558033・**thiscall: this=param_1=src／param_2=dest／param_3=ctx**）:
```
L558043: FUN_00700770(dest, ctx)                        ; 基底 init
L558044: dest.word[+8]  ← src.word[+8]                  ; ★flags ワード（bit3/bit5 含む）
L558045-51: dest.dword[+0x0c..+0x24] ← src（7フィールド）
L558052+: if (src[+0x30]) FUN_0055e000(...) → vtable[1] 型照合 → dest[+0x30] = resolved
```

### §2.2 `0x6D0550`（v399_1(a)(b) machine ＋ decompiled L504074-504096）

```
6D0550: push ecx; push esi; esi=[esp+0xc]（src）; push edi; push esi; mov edi,ecx（dest）
6D055A: call 0x715f40        ; ★base copy（vtable[1] 介在）
6D055F: eax=[esi+0xd8]; cmp eax,0xa010068; jb skip; cmp eax,0xa01006d; jae skip  ; ★type-id 範囲
6D0573: eax=[esi+0x21c]; ... call [eax+4]（vtable[1]・5引数）→ byte 結果
6D059F: result!=0 → or word[edi+8], 0x20   /  6D05AA → and word[edi+8], 0xffdf
6D05A7: ret 4
```
- `FUN_00715f40`（L558150-558169）: **`(field, size, &tag, 1)` 形で `[src+0x21C]` の `vtable[1]` を各フィールドに呼ぶ**（+8: size2／+0xc,+0x10,+0x14,+0x18...: size4）＝**介在型のフィールド単位コピー**。

### §2.3 `word[+8]` bit5 ＝ IsManagerControlled（v399_2(c)(d)）

```
L504116: param_2 = CONCAT31(..., *(byte *)(param_1 + 8) >> 5) & 1   ; ★bit5 抽出
L504117: param_2 = FUN_0070f950("IsManagerControlled", param_2)      ; ★同名で出力
```
- EXE 文字列: **VA `0xA7A210` = `"IsManagerControlled"`**。

### §2.4 対関係（callers）

| thunk/fn | E8 callers |
|---|---|
| `0x6D0530`（→0x715D80 直接コピー） | `0x6CE2CF`, **`0x6D0180`（FUN_006d0170 先頭・v394）**, `0x6D148F` |
| `0x6D0550`（介在コピー＋flag 導出） | `0x6CE329`, `0x6D037B`, `0x6D0D3C` |

- **同一3つの fn ファミリが両方を呼ぶ**（`0x6CE2xx` 内で `0x6CE2CF`→0x6D0530 と `0x6CE329`→0x6D0550 が隣接）＝**コピーの2系統**（直接転送 vs vtable[1] 介在）[確定(命令)]。

## §3 残点 **V-399-1..3**
1. **`[+0xD8]` type-id 定数 `0xA010068..0xA01006C` の意味**（どのクラス群の ID か・NiRTTI 系 [推定]）＋**`[+0x21C]` オブジェクトの identity**（`vtable[1]` query の主体＝何を返すか）【中】
2. **`FUN_00715F40` 介在コピーの全体**（全フィールド列と type-tag の意味・0x715xxx 帯のどのクラスか）【低】
3. **`0x6D0540`（`jmp 0x715820`・3番目 thunk）と `0x715820` の役割**【低】

## §4 検証・自己批評
- probe: **v399_1・v399_2 全 EXIT=0**。
- **決定打の作り**: (i) machine で thunk 構造（`jmp` 先と本体の別）を確認 → (ii) **先関数2つの decompiled でコピー内容を逐語取得** → (iii) **隣接 fn `FUN_006d05c0` の `byte[+8]>>5` ⇔ `"IsManagerControlled"` 文字列で bit5 を確定** → (iv) callers の対関係で2系統を確定。
- 妥当性チェック: ①`0x6D0530` caller `0x6D0180`⇔v394 の `FUN_006d0170` 先頭 call — ②`FUN_00715d80` の `dest.word[+8]←src`⇔bit3/bit5 両 flag の copy — ③`0x6D0550` の `or/and 0x20`⇔`FUN_006d05c0` の `>>5` 読手 — **三方向で矛盾なし**。
- docs との不整合: **なし**（v385 の V-385-3 を解消）。
- **教訓**: **「thunk と本体」を分離して裁定せよ**（head が `jmp` なら本体は先のアドレス・decompiled アノテーションも先に付く）＋**隣接 fn の文字列読手から bit フィールド名を拾う**（bit3=Update gate→bit5=IsManagerControlled の順で既知 bit が増える）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
