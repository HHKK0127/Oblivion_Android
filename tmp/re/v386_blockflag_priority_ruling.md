# v386 — `[block+0xD]`（block byte flag）と s1（priority）の優先関係 census の裁定（#152 ⑤・V-381-1）

- 採番: **v386**（ai-todo #152 束の第5弾・低優先帯、probe prefix v386_1..v386_2）
- 対象: Oblivion.exe v1.2.0416
- 依頼経緯: v381 の残点 **V-381-1（`[esi+0xD]`（block byte flag）と s1 の優先関係の実運用 census）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| 優先規則の実運用 | **[確定(命令)]** | `0x6C6A50` 内: **`[block+0xD] == 0xFF` → s1（`movsx` して RegisterInterpolator の priority へ）／非0xFF → `[block+0xD]`（`movzx`）が優先**（v381 再掲）。**既定値 0xFF は resetter 2本が保証** — `0x6C62E0`（zero-init 後 `mov [eax+0xD],0xFF; ret`・E8=0＝ctor/reset）と `0x6C6300`（E8 caller `0x6C6675`・`mov [edi+0xD],0xFF`） |
| 非0xFF 書手 | **[確定(否定側)]＋[部分確定]** | 帯内（0x6C6000..0x6CA000）の書手 census＝**3件のみ**: resetter 2本（0xFF）＋**`0x6C68FB` の1本**。`0x6C68FB` は **fn `0x6C6870`（E8 callers＝`0x6C86C3` の1本）＝block 構造のフィールドコピー**（`[edi+4]=[ebx+4]`／`[edi+8]=[ebx+8]`／`[edi+0xC]=[ebx+0xC]`／`[edi+0xD]=[ebx+0xD]` を順にコピーして返す・`ret 4`）＝**独立した priority 代入ではなく「ソースからの伝播」**。→ **非0xFF の原本供給元は帯内に不在**（コピー経由のみ）＝帯外 census は残点 |
| s1 の渡し元 census | **[確定(命令)]** | `0x6C6A50` の E8 callers＝**3本**: ①`0x6C9BEA`（Activate `0x6C9BA0` 内）＝**`push [esp+0xC]`＝Activate 自身の s1（priority）** ②`0x6C9D70`＝**`push edi`** ③`0x6C9E55`＝**`push ebx`**（`mov ecx,esi; mov [esi+0x58],0` の直後）— ②③は wrapper 引数由来の動的値 |

**運用まとめ**: **block 側 flag（`[+0xD]`）が既定 0xFF の間は s1（Activate の priority 引数）が RegisterInterpolator の priority になり、明示的に非0xFF が設定された block は flag が優先する** [確定(命令)]。非0xFF の初期ソースは本帯に書手がなく（コピーのみ）、供給元は残点 V-386-1。

---

## §2 証拠

### §2.1 優先規則と既定値（v386_1(b) census）

| site | 命令 | 意味 |
|---|---|---|
| `0x6C62E0`（head・E8=0） | `mov [eax],0; …; mov cl,[0xA79EFc]; mov [eax+0xD],cl` → `0x6C62F5: mov byte [eax+0xD],0xFF; ret` | **ctor/reset で 0xFF 既定化** [確定(命令)] |
| `0x6C6300`（head・E8 caller `0x6C6675`） | `… mov [edi+0xD],0xFF; pop; ret` | **2本目の 0xFF リセット** [確定(命令)] |
| `0x6C68FB`（fn `0x6C6870`・E8 caller `0x6C86C3`） | `mov dl,[ebx+0xC]; mov [edi+0xC],dl; mov al,[ebx+0xD]; mov [edi+0xD],al` | **block フィールドコピー（+4/+8/+0xC/+0xD）＝伝播のみ** [確定(命令)] |

- 読み手（v381 再掲）: `0x6C6A75: mov dl,[esi+0xD]` → `cmp dl,0xFF` → 0xFF なら `movsx edx, byte [esp+0x18]`（s1）／否则 `movzx edx,dl`（flag 優先）。

### §2.2 s1 渡し元 3 callers（v386_1(a)/v386_2(a)）

```
① 0x6C9BEA: mov eax,[esp+0xC]; push eax; mov ecx,esi; call 0x6C6A50
   = Activate の s1（priority・v378/v381 確定）
② 0x6C9D70: … mov [esi+0x58],0; call 0x6C6A50（push は v376_1 の push edi）
③ 0x6C9E55: push ebx; mov ecx,esi; mov [esi+0x58],0; call 0x6C6A50   （v386_2(a)）
   = wrapper 引数由来（0x6C9E00 帯）
```
- `0x6C6A50` の **E8 callers＝3本**（v386_1(a)）＝Activate／wrapper2系統のみ。

### §2.3 コピー fn `0x6C6870`（v386_2(b) unfiltered）

```
6C6870: ebx = arg1（src block）
6C687E: edi = ecx（dst block）
…（refcount/Interlocked 処理 [0xA28078]/[0xA2807c] 経由）
6C68D8: [edi+4]  = [ebx+4]    （node 名 offset）
6C68EC: [edi+8]  = [ebx+8]    （解決済み ptr）
6C68F2: [edi+0xC]= [ebx+0xC]  （結果 byte）
6C68F8: [edi+0xD]= [ebx+0xD]  （★byte flag）
6C68FE: return edi; ret 4
```
- **E8 callers＝`0x6C86C3` の1本**（0x6C8220 帯＝StoreTargets 前提 fn の近傍）。

## §3 残点 **V-386-1..2**
1. **`[block+0xD]` の非0xFF 供給元**（帯外。帯内はコピーのみ＝既定0xFF。初回に非0xFF を書く場所＝NIF 読込か init fn `0x6C6300` のソース（`0x6C6675` caller 側）の decode で追えるか）【低】
2. **`0x6C62E0`/`0x6C6300` の所属**（block 構造の ctor/reset 系 [推定]・`0xA79EFc` 定数との関係）【低】

## §4 検証・自己批評
- probe: **v386_1・v386_2 全 EXIT=0**。
- **決定打の作り**: (i) `[reg+0xD]` 書手の全形式 census（C6/88）で**3件のみを確定** → (ii) 各 site の fn 同定（reset×2＋copy×1）→ (iii) s1 渡し元 3 callers の push を unfiltered で確認。
- 妥当性チェック: ①0xFF 既定⇔2 resetter⇔ v381 の `cmp dl,0xFF` 分岐 — ②s1 3 callers⇔ `0x6C6A50` ret 4（1引数）— ③copy fn の +4/+8/+0xC/+0xD⇔ block 構造レイアウト（stride16）— **三方向で矛盾なし**。
- docs との不整合: **なし**（v381 の V-381-1 を解消）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
