# v385 — interpolator 配列の第2/第3構築経路（`0x6D018C`/`0x6D03AA`）の所属の裁定（#152 ④・V-375-1）

- 採番: **v385**（ai-todo #152 束の第4弾、probe prefix v385_1..v385_2）
- 対象: Oblivion.exe v1.2.0416 ＋ b07 RTTI/vtbl 表（v373 済み vtable 0xA79984）
- 依頼経緯: v375 の残点 **V-375-1（`0x6D018C`/`0x6D03AA` 帯 fn の所属＝第2/第3 interpolator 配列構築経路の正体）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令および実測 vtable のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| 第2経路 `0x6D018C` の所属 | **[確定(命令)]** | **fn `0x6D0170`＝interpolator 配列の間コピー**（`ret 8`＝2引数）— **E8 callers＝`0x6D034B` の1本**、その fn は **head `0x6D02E0` ＝ NiMultiTargetTransformController（vtable 0xA79984）の slot6(+0x18)** [確定(vtbl)]。slot6 本体は **`new(0x48)` → 基底 ctor `0x6D04E0` → `vtable 0xA79984` → 3フィールド0初期化 → `0x6D0170(this, new, arg)` で配列コピー → 新オブジェクトを返す** ＝ **Clone/Copy 相当**（正式名は [推定]＝残点） |
| 第3経路 `0x6D03AA` の所属 | **[確定(vtbl)+確定(命令)]** | **fn `0x6D0370` ＝ vtable slot7(+0x1C)＝`Load`**（本プロジェクトの slot+0x1C=Load 規約・v348/v350 と一致）。E8 callers＝**0件（virtual）**。本体: **`[arg+0x21C]`（stream の reader）の virtual で count を読む（push 2）→ `0x6D0010` で配列構築 → 各要素を `0x712A90(arg)`（link 解決 [推定高]）で取得し `[this+0x40]`（ptr 配列）へ格納** |
| 2経路の性格 | **[確定(命令)]** | **第2＝Clone（他オブジェクトからの実配列コピー）／第3＝Load（NIF ファイルからの読込＋リンク解決）**。v375 の「第2/第3 構築経路」はこの2つであり、**runtime 初期化ではなく『Clone』と『Load』** |

**V-375-1 への回答**: `0x6D018C`＝**Clone 経路（slot6 → 0x6D0170 → 0x6D0010＋要素コピー）**、`0x6D03AA`＝**Load 経路（slot7=+0x1C → stream count 読み → 0x6D0010 → 0x712A90 で ptr 配列埋め）** [確定]。

---

## §2 証拠

### §2.1 第2経路: slot6 → `0x6D0170`（v385_1(a)/v385_2(a)(b)）

**vtable 実測（v385_2(a)）**: `0xA79984` slot6(+0x018)=**`0x6D02E0`**／slot7(+0x01C)=**`0x6D0370`**（v373_3(a) と一致）。

**slot6 本体 `0x6D02E0`（v385_2(b) unfiltered）**:
```
6D02E0: SEH prologue（push -1 / 0x9CA42B）
6D0304: ebx = ecx（this）
6D0306: push 0x48; call 0x401F00 → esi      ; ★new(0x48)＝72B オブジェクト
6D0320: mov ecx,esi; call 0x6D04E0           ; 基底 ctor
6D0327: [esi] = 0xA79984                     ; ★vtable = NiMultiTargetTransformController
6D032D..6D0333: [esi+0x3C]=0 / [+0x40]=0 / [+0x44]=0（3フィールド0初期化）
6D033B: eax = [esp+0x24]（arg1）
6D033F: push eax; push esi; mov ecx,ebx
6D034B: call 0x6D0170                        ; ★★this → new への配列コピー
6D0350: return esi（新オブジェクト）
```

**`0x6D0170`（v385_1(a) unfiltered・`ret 8`）**:
```
6D0170: ebx = arg1（new）、ebp = arg2（param）
6D017C: push ebp; push ebx; mov esi,ecx（orig）
6D0180: call 0x6D0530（setup/version helper・E8 callers=0x6CE2CF/0x6D0180/0x6D148F）
6D0185: eax = [esi+0x44]（orig の count）
6D0189: push eax; mov ecx,ebx; call 0x6D0010 ; ★new 側に配列構築（count=orig）
loop（i < [esi+0x44]・stride48）:
6D01A0: ecx = [ebx+0x3C]+i*48（new 要素）
6D01B0: this = [esi+0x3C]+i*48（orig 要素）
6D01B5: call 0x6CC480（param=ebp）           ; ★要素コピー（0x6CC480 = jmp 0x6CD3D0）
6D01C7: ret 8
```
- **E8 callers of `0x6D0170` ＝ `0x6D034B` のみ**（v385_1(a)）＝ **slot6 専用**。

### §2.2 第3経路: slot7 = Load（v385_1(b) unfiltered）

**`0x6D0370`（E8 callers＝0件＝virtual）**:
```
6D0370: ebp = arg1（stream 相当）
6D037B: call 0x6D0550（version/read setup）
6D0380: eax = [ebp+0x21C]（★stream の reader・v350 の [stream+0x21C]reader と同一）
6D0386: edx = [eax+4]（virtual）
6D0389..6D039F: push 1 / lea ecx,[esp+0x18] / push 2 / lea edi,[esi+0x44] / push edi / push eax → call edx
                ; ★reader へ（type=2・&count・1）＝count 読み [推定高]
6D03A1: eax = [edi] = [esi+0x44]（count）
6D03A7: push eax; mov ecx,esi; call 0x6D0010  ; ★★this に配列構築
loop（i < [esi+0x44]）:
6D03B6: mov ecx,ebp; call 0x712A90            ; ★arg1 から対象取得（link 解決 [推定高]）
6D03BD: edx = [esi+0x40]（ptr 配列）
6D03C6: [edx+i*4] = eax                       ; ★ptr 配列へ格納
6D03D2: ret 4
```
- **slot7(+0x1C)＝Load** は本プロジェクトの規約（v348/v350 で `Load=slot+0x1C`）と一致し、**stream reader（`[+0x21C]`）＋count 読み＋リンク解決**という挙動も Load と整合 [確定(vtbl)+確定(命令)]。
- 近接 slot10(+0x28)=`0x6D03E0` は **`[edi+0x220]`**（別 reader）から読み＝Save 系 [部分確定]（残点対象外）。

### §2.3 setup helpers（v385_1(c)/v385_2(c)）

- `0x6D0530`（E8: `0x6CE2CF`/`0x6D0180`/`0x6D148F`）と `0x6D0550`（E8: `0x6CE329`/`0x6D037B`/`0x6D0D3C`）: いずれも **`call 0x715F40` → `cmp eax,0xA010068` / `cmp eax,0xA01006D`（Ni バージョン定数比較）＋読取構築** ＝ **version-gated 読取 setup** [部分確定]。

## §3 残点 **V-385-1..3**
1. **`0x6CC480`（= `jmp 0x6CD3D0`）の要素コピーの方向と内容**（this=orig 要素／stack=new 要素／param=arg2 の意味。orig→new か双方向か）【中】
2. **slot6 の正式名**（Clone か Copy か・1引数付きで新オブジェクトを返す挙動＝Gamebryo シグネチャとの突合）【低】
3. **`0x6D0530`/`0x6D0550`（version-gated setup）の正確な役割**（`0x715F40` の戻り値と 0xA010068/0xA01006D 比較の意味）【低】

## §4 検証・自己批評
- probe: **v385_1・v385_2 全 EXIT=0**。
- **決定打の作り**: (i) 2 call site の find_head → (ii) **slot6 本体の unfiltered（new(0x48)→ctor→vtable→0x6D0170）で Clone 構造を直接確認** → (iii) **vtable 実測で slot6/7 を確定** → (iv) Load 本体の stream reader（`[+0x21C]`）＋`0x712A90` ループを直接確認。
- 妥当性チェック: ①slot6=0x6D02E0⇔0x6D034B がその body 内 — ②slot7=0x6D0370⇔E8=0（virtual）⇔`[+0x21C]` reader — ③`0x6D0010` の3 callers（v375）⇔ctor/Clone/Load の3系統 — **三方向で矛盾なし**。
- docs との不整合: **なし**（v375 の V-375-1 を解消。v375 の「第2/第3 構築経路 [部分確定]」を本 v385 で確定に昇格）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
