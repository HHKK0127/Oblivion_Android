# v375 — singleton interpolator 配列と `0xB3CBA4` 系統の切り分け（#45 N5b・V-374-2）

- 採番: **v375**（#45 N5b の残点 V-374-2 消化、probe prefix v375_1）
- 対象: Oblivion.exe v1.2.0416
- 依頼経緯: v374 の残点 **V-374-2（singleton `0xB3CD7C` の interpolator 配列が `0x6D0010` で構築されるのか、`0xB3CBA4` が別途初期化されるのかの2系統切り分け）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| singleton `0xB3CD7C` の interpolator 配列 | **[部分確定]** | **ctor `0x6C5EE0`（`NiMultiTargetTransformController`）が `0x6D0010` を呼ぶ**（v374 確定）→ singleton の interpolator 配列は **ctor 経路（CRT static ctor）** で構築される [推定高]。singleton 自体は **.bss の静的オブジェクト**（`0xB3CD7C` への store 形式 0 件＝CRT 初期化で実行される static ctor）[確定負（静的）] |
| `0xB3CBA4` 系統 | **[確定(否定側)]＋[部分確定]** | **`0xB3CD7C` と `0xB3CBA4` の両方を参照する fn = 0 件**（refs 13/22/both 0）→ **2系統は無関係** [確定(否定側)]。`0xB3CBA4` 系統の構築経路＝**`0x6D018C`／`0x6D03AA` 帯の fn**（`0x6D0010` の E8 callers 3本のうち ctor 以外の2本）[部分確定] |
| `0x6D0010` の callers | **[確定(命令)]** | **3本**: `0x6C5F2A`（ctor `0x6C5EE0` 内）／**`0x6D018C`**（`mov ecx,ebx; call` → `cmp word [esi+0x44],di`）／**`0x6D03AA`**（`mov ecx,esi; call` → `call 0x712A90` → `mov edx,[esi+0x40]`）＝**interpolator 配列の構築は3経路** |

**#45 N5b への回答（残点2）**: **2系統の interpolator 配列が併存する** — ①**singleton（`0xB3CD7C`）系統**＝ctor（CRT static ctor）→ `0x6D0010` → `new(count*48+4)` → `[+0x3C/+0x40/+0x44]`、②**`0xB3CBA4` 系統**＝`0x6D018C`/`0x6D03AA` 帯の fn が構築 → various object へコピー（v373 の初期化群14件）。

---

## §2 証拠

### §2.1 `0x6D0010` の E8 callers 3本 [確定(命令)]

v375_1(a):
- `0x6C5F2A`（ctor `0x6C5EE0` 内・v374 確定）
- **`0x6D018C`**: `mov ecx,ebx; call 0x6D0010` → `xor edi,edi; cmp word [esi+0x44],di; jbe 0x6D01C3`（count 検査）
- **`0x6D03AA`**: `mov ecx,esi; call 0x6D0010` → `xor ebx,ebx; cmp word [edi],bx; jbe 0x6D03CE; mov ecx,ebp; call 0x712A90; mov edx,[esi+0x40]`（count 検査＋ptr 配列操作）

### §2.2 singleton と `0xB3CBA4` は無関係 [確定(否定側)]

v375_1(d): `0xB3CD7C` への refs **13件**／`0xB3CBA4` への refs **22件**／**両方を含む fn = 0 件**。→ **2系統の interpolator 配列は独立**。

### §2.3 singleton は .bss 静的オブジェクト [確定負（静的）]

v375_1(b): `0xB3CD7C` への **store 形式（A3/C705/8905/89 modrm）全走査で 0 件** → singleton は **CRT 初期化で実行される static ctor** によりセットされる（`0x6C5EE0` がその ctor [推定高]）。

## §3 残点 **V-375-1..2**
1. **`0x6D018C`／`0x6D03AA` 帯 fn の所属**（第2/第3 の interpolator 配列構築経路の正体。`0xB3CBA4` 系統の真の構築元）【中・#45 の interpolator 配列構築元の完全な図に必要】
2. **singleton の static ctor が CRT 初期化（`.CRT$XCU` セクション）で実行されるかの直接証拠**【低・動的確認（#82 系）】

## §4 検証・自己批評
- probe: **v375_1 全 EXIT=0**。
- **決定打の作り**: (i) `0x6D0010` の E8 callers census（3本を特定）→ (ii) singleton/`0xB3CBA4` の store 形式走査（0 件＝.bss）→ (iii) **両方を参照する fn の交集合（0 件）で2系統の独立を証明**。
- 妥当性チェック: ①`0x6D0010` の3 callers ⇔ v374 の構築 fn ⇔ v373 の初期化群14件 — ②singleton の .bss 性⇔ store 0 件 — **矛盾なし**。
- docs との不整合: **なし**（本裁定は新規。v374 の V-374-2 を解消）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
