# v388 — `[seq+0x54]`/`[seq+0x58]` の裁定（#152 ⑦・V-377-2 の残り＝v387 で +0x4C/+0x50 は解決済み）

- 採番: **v388**（ai-todo #152 束の第7弾・低優先帯、probe prefix v388_1）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v377 の残点 **V-377-2（`[+0x4C]`/`[+0x50]`/`[+0x54]`/`[+0x58]` のフィールド名）**。**前半2件（+0x4C='Start'/+0x50='End'）は v387 で解決済み** → 本 v388 は残る **+0x54/+0x58** を裁定
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| `[seq+0x54]` の名前 | **[確定(否定側)]（label 不在）＋[確定(命令)]（挙動）** | **exe 自身の label は存在しない**（`FUN_006ca440` の print 11件に +0x54 は不在・decompiled.c のラベル付き書手も0件）。挙動: **−FLT_MAX＝未設定**（ctor/Deactivate/state1 各リセット）／**wrapper `0x6C9D30` 帯が `[+0x54] = arg / Freq(+0x28)`（頻度で割る）で設定**／state1 切替時に **`Offset(+0x48) = [+0x54] − time` へ変換**されて自身は −FLT_MAX リセット（`0x6CAA4A`）／fade path で未設定判定（`0x6CAB84`）。→ **frequency 正規化された保留時刻 [推定高]**・**正名 [未確定]**（label 不在のため） |
| `[seq+0x58]` の名前 | **[確定(否定側)]（label 不在）＋[確定(命令)]（挙動）** | **exe label 不在**（同上）。挙動: **crossfade 相手 sequence** — Activate が **s5 を `0x6C9BE0: mov [esi+0x58],edi` で格納**（先頭 `0x6C9BB6` で0リセット・自己参照 `cmp eax,esi; je 拒否`・`0x6C6110`/`0x6C6ED0` 検証）／Deactivate・各 wrapper が0リセット／**相手の `Freq(+0x28)` で割る（`0x6CAAD8`）・相手の `Last(+0x34)`/`Offset(+0x48)` を読む（`0x6CAB9F`/`0x6CABC6`）・テキストキー同期 `0x6C6B30` へ相手を渡す（`0x6CAAC0`）**。v379/v380 の「相手 sequence」記述を命令で裏付け |
| （整理）V-377-2 全体 | **4/4 裁定済み** | `+0x4C='Start'`／`+0x50='End'`（v387・label 確定）／`+0x54`=label 不在＋frequency 正規化保留時刻 [推定高]／`+0x58`=label 不在＋crossfade 相手 sequence [確定(命令)] |

**V-377-2 への回答**: 前半2件は v387（label 確定）、残り2件は **exe label 不在 [確定(否定側)]** とし、**挙動ベースで裁定**（+0x54=frequency 正規化保留時刻 [推定高]／+0x58=crossfade 相手 [確定(命令)]）。**正名が要る場合の次手段＝Gamebryo フィールド名との usage 突合（m_fPhase/m_fStartTime 等）＝残点 V-388-1**。

---

## §2 証拠

### §2.1 label 不在の根拠（v388_1(a)）

- `FUN_006ca440` の print 11件（v387 §2.2）に **+0x54/+0x58 は不在**。
- decompiled.c を `'+ 0x54)'/' + 0x58)'` × ラベル付き出力で全走査 → **該当0件**（ヒットは全て vtable スロット呼び出し `(*obj + 0x58)`＝別概念）[確定(否定側)]。

### §2.2 `[seq+0x54]` の挙動（v388_1(b) census 8件）

| site | 命令 | 意味 |
|---|---|---|
| ctor `0x6C65EA` / Deactivate `0x6C9D22` / state1 `0x6CAA5A` | `= −FLT_MAX` | 未設定リセット |
| wrapper `0x6C9DE6`/`0x6C9DF0`（ret 0x1c・`0x6C9D30` 帯） | `fld [esp+0x14]; fdiv [edi+0x28]; fstp [edi+0x54]` | **★arg ÷ Freq で設定** |
| state1 切替 `0x6CAA36`→`0x6CAA4A` | `fld [+0x54]; ==−FLT_MAX? 否なら [+0x48] = [+0x54] − st1` | **★Offset へ変換** |
| fade path `0x6CAB84`/`0x6CAB9A` | `fld [+0x54]; ==−FLT_MAX 判定 → 値を継続` | 未設定分岐 |

### §2.3 `[seq+0x58]` の挙動（v388_1(c) census 11件）

- **書手**: `0x6C9BE0: mov [esi+0x58],edi`（**Activate s5=相手格納**）／`0x6C9BB6`（Activate 先頭=0）／`0x6C9D17`（Deactivate=0）／`0x6C9D69`・`0x6C9E4E`（wrapper=0）。
- **読手**: `0x6C9BBF: mov eax,[edi+0x58]; test; cmp eax,esi(自己)→拒否`（ Activate 検証）／**`0x6CAAD8: fdiv [eax+0x28]`（相手の Freq）**／**`0x6CAB9F: fld [ecx+0x34](相手 Last); fld [ecx+0x48](相手 Offset); fadd`**／**`0x6CABC6: fadd [eax+0x48]`（相手 Offset 加算）**／**`0x6CAAC0: mov ecx,[esi+0x58]; … call 0x6C6B30`（テキストキー同期へ相手渡し）**。
- （`0x6C990C: mov edx,[eax+0x58]; call edx` は別オブジェクトの fn ptr 経路＝本フィールドと無関係 [確定]。）

## §3 残点 **V-388-1..2**
1. **`[+0x54]` の正名**（label 不在のため、Gamebryo フィールド（m_fPhase/m_fStartTime 等）と usage（arg÷Freq・Offset 変換元）の突合で切り分け）【低】
2. **`0x6CAA4A` の `Offset = [+0x54] − st1` の st1 の出所**（time か frequency 換算値か）【低】

## §4 検証・自己批評
- probe: **v388_1 全 EXIT=0**。
- **決定打の作り**: (i) decompiled.c の label 付き書手全走査で **label 不在を証明** → (ii) `[+0x54]`/`[+0x58]` の書手/読手 census（8件/11件）→ (iii) `arg÷Freq` と `Offset=[+0x54]−time`、相手の Freq/Offset/Last 読取という**使い方の束**で意味を確定。
- 妥当性チェック: ①+0x58 の格納＝Activate s5⇔v379/v380 の「相手 sequence」— ②+0x54 の −FLT_MAX 初期化⇔全 site の未設定判定— ③`[+0x48]='Offset'`（v387）⇔ `Offset=[+0x54]−time`（本裁定）— **三方向で矛盾なし**。
- docs との不整合: **なし**（v377 の V-377-2 を4/4 でクローズ）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
