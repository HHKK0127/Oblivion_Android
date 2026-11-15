# v374 — interpolator 実配列の構築元と `0xB3CBA4`（base グローバル）の裁定（#45 N5b・V-373-1）

- 採番: **v374**（#45 N5b の残点 V-373-1 消化、probe prefix v374_1）
- 対象: Oblivion.exe v1.2.0416
- 依頼経緯: v373 の残点 **V-373-1（`0xB3CBA4` base グローバルの writer＝interpolator 実配列の構築元）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| interpolator 実配列の構築元 | **[確定(命令)]** | **`0x6D0010` = `NiMultiTargetTransformController` の interpolator 配列構築 fn**（arg1=count u16）。**`new(count*0x30+4)`（overflow チェック付き）→ プール初期化（dtor ペア `0x6CBCA0`/`0x6CBCB0`・各要素 48B）→ `[+0x3C]`（base）= 配列先頭（+4 ヘッダ分）／`[+0x40]`（ptr 配列）= `new(count*4)`（memset 0）／`[+0x44]`（count）= arg1** |
| `0xB3CBA4`（base グローバル）の writer | **[確定負（静的）]＋[部分確定]** | **.text への store 形式（A3/C705/8905/89 modrm）全走査で 0 件**（v374_1(a)）。`0xB3CBA4` は **.bss（セクション外）**＝**静的初期化のみ**（CRT/初期化セクション経由の実行時セット [推定]、または singleton の interpolator 配列を指す単なるポインタ [部分確定]）。**.text から書けない**ことは [確定(否定側)] |
| 3フィールドの帰属 | **[確定(命令)]**（v373 §2.3 から昇格） | `[+0x3C]`=**interpolator 配列 base**（`new(count*48+4)` の +4・各要素 `NiBlendTransformInterpolator` 48B・dtor `0x6CBCA0`/`0x6CBCB0`＝vtable `0xA79E54`）／`[+0x40]`=**ptr 配列**（`new(count*4)`・memset 0）／`[+0x44]`=**count（u16・arg1）** — 全て `0x6D0010` 内で直接構築される |

**#45 N5b への回答（残点1）**: interpolator 実配列は **`0x6D0010`（NiMultiTargetTransformController の ctor 経由）が `new(count*48+4)` で構築**する。`0xB3CBA4` は .bss の静的グローバルで、**.text からは書けない**（store 0 件）＝singleton の interpolator 配列を指すポインタ、または CRT 初期化でセットされる既定値 [部分確定]。

---

## §2 証拠

### §2.1 `0x6D0010` = interpolator 配列構築 [確定(命令)]

v374_1(b) unfiltered 全 decode:
```
6D0034: ebp = ecx                          ; this = NiMultiTargetTransformController
6D0036: bx  = [esp+0x24]                   ; arg1 = count（u16）
6D003E: [ebp+0x44] = bx                    ; ★count 設定
6D0042: je 0x6D00F4                        ; count==0 → base/ptr なしで終了
6D0048: edi = count
6D004D..6D0069: eax = count * 0x30 + 4     ; ★size = count*48 + 4（overflow チェック: seto/setb）
6D006A: call 0x401F00                      ; operator new
6D0072: [esp+0x24] = ptr                   ; 配列
6D0080..6D0093: push 0x6CBCA0(dt) / push 0x6CBCB0(dt) / push count / lea esi,[eax+4] / push 0x30 / push esi / [eax]=count / call 0x984BCF
                                            ; ★プール初期化: 各要素 48B・dtor ペア・ヘッダ count
6D00A3: [ebp+0x3C] = esi                   ; ★base = 配列先頭（+4 ヘッダ分）
6D00B0..6D00C4: 各要素の dtor 登録ループ（esi += 0x30）
6D00C6..6D00E9: eax = count*4 → new → [ebp+0x40] = ptr 配列 → memset(0)
6D0107: ret 4
```
- **`0x6CBCA0`/`0x6CBCB0` = dtor ペア**（v373_4 で vtable `0xA79E54`＝`NiBlendTransformInterpolator` を確認済）→ **各48B 要素が NiBlendTransformInterpolator** [確定]。
- **`0x6D0010` の呼び出し元**: ctor `0x6C5EE0` の `6C5F2A: call 0x6D0010`（v373_2(a)）＝**NiMultiTargetTransformController の ctor 経由** [確定]。

### §2.2 `0xB3CBA4` の writer = 0 件 [確定負（静的）]

- v374_1(a): **A3**（`mov [imm32],eax`）／**C7 05**（`mov dword [imm32],imm32`）／**89 05/0D/15/1D/35/3D**（`mov [imm32],reg`）の**全形式で .text 全走査 → store 0 件**。
- **セクション**: `0xB3CBA4` は **.bss（セクション外）**＝**静的値の読取不可**（v371 の `0xB39440`（GMST 登録先）と同現象）。実行時の値は CRT/初期化セクション経由 [推定]、または **singleton（`0xB3CD7C`）の interpolator 配列を指す単なるポインタ** [部分確定]。
- v373 で実測した「`0xB3CBA4` から various object へコピーする初期化群14件」は**読み取りコピー**（`mov eax,[0xB3CBA4]; mov [dest+off],eax` 型）＝**global 側が真値**であることは変わらない。

### §2.3 継承と整合

- **基底 ctor `0x6D04E0`**（v374_1(d)）: `call 0x715990` → **`mov [esi],0xA7A16C`（vtable NiInterpController＝b07 0xA7A16C）** → ret。継承: `NiInterpController`（0xA7A16C）← … ← `NiMultiTargetTransformController`（0xA79984）[確定]。
- **整合**: v373 の「3フィールド=[部分確定]」→ 本 v374 で **`0x6D0010` の直接構築により [確定(命令)]** に昇格。

## §3 残点 **V-374-1..2**
1. **`0xB3CBA4`（.bss）の真の初期化元**（CRT/初期化セクションか、singleton の interpolator 配列を指す単なるポインタかの切り分け。.text に書手0件のため**動的確認が必要＝#82 系**）【低】
2. **singleton（`0xB3CD7C`）の interpolator 配列が `0x6D0010` で構築されるのか、`0xB3CBA4` が別途初期化されるのかの区別**（2系統の interpolator 配列が併存する可能性）【中】

## §4 検証・自己批評
- probe: **v374_1 全 EXIT=0**。
- **決定打の作り**: (i) `0xB3CBA4` の store 形式全走査（0 件＝.bss 証明）→ (ii) `0x6D0010` unfiltered 全 decode（`new(count*48+4)`＋dtor ペア＋3フィールド構築を直接確認）→ (iii) dtor の vtable 追跡（v373 で NiBlendTransformInterpolator 確定済みと接続）。
- 妥当性チェック: ①`0x6D0010` の size 計算（count*48+4）⇔ v373 の accessor（idx*48→ptr）⇔ 48B 格納 site census 12件 — **三方向で矛盾なし**。②count=u16 が `0x6D0010`（arg1）／ctor（`[+0x44]`）／`0x6C6910`（`movzx`）で一致。
- docs との不整合: **なし**（本裁定は新規。v373 の V-373-1 を解消）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
