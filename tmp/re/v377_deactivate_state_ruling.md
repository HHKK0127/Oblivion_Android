# v377 — head `0x6C9CB0` の正式名と `[this+0x44]` state enum の裁定（#150 残点・V-376-1）

- 採番: **v377**（#150 残点 V-376-1 消化、probe prefix v377_1）
- 対象: Oblivion.exe v1.2.0416 ＋先行裁定 v187_anim_residuals.md
- 依頼経緯: v376 の残点 **V-376-1（head `0x6C9CB0` の正式 fn 名と `[this+0x44]` state の意味）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令または先行裁定のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| head `0x6C9CB0` の正式 fn 名 | **[確定]（v187 先行裁定＋v377 補強）** | **`Deactivate/Suspend`** — v187 L142「0x6C9CB0 = Deactivate/Suspend（**+0x44→3/4** のフェードアウト設定 or 即時停止）」。v377 補強: ①**既知 vtbl 10種48スロット・non-.text の fn ptr テーブル両方で `0x6C9CB0` 不在**＝**非仮想の直接呼び出し fn** ②末尾 `0x6C9CD9: mov [ecx+0x44],eax`（**引数で state を渡す**）＋`fld [0xA7DEB4]; fchs; [ecx+0x4C]/[+0x50]=-const`（フェード時間 [推定]）＋`ret 8`（2引数）③**callers 15本のうち4本が v172 のエフェクト減衰チェーン（`0x49F950→0x4E0D90→0x6C9CB0→0x4D90D0`）と一致** |
| `[this+0x44]` state enum | **[確定(命令)]（0/1/6）＋[部分確定]（2/5）＋[確定]（3/4 は v187）** | **0=停止/Reset**（`0x6C9D14: [ecx+0x44]=0; [ecx+0x58]=0; [ecx+0x54]=-const; call 0x6C6AC0`）・**1=再生/Activate**（`0x6C9C68`/`0x6C9D80`/`0x6C9EAC`）・**3/4=フェードアウト/停止**（v187・Deactivate/Suspend の遷移先）・**6=サイクル**（v187「state6 のサイクルオフセット」・`0x6C9EDD: [edi+0x44]=6`）・**2**（`0x6C9C2E`）・**5**（`0x6C9C0E`/`0x6C9E72`・caller `0x471A70` の `cmp [eax+0xAC],5`）＝[部分確定]（意味未特定＝残点） |

**#45 N5b への回答（v376 §3 残点1）**: `0x6C9CB0` は **`NiControllerSequence` の Deactivate/Suspend [確定]**（state `+0x44` を 3/4 へ遷移）。v376 が「activate/cycle 系 [推定高]」としたのは**呼び出し元 fn（`0x6C9D70`/`0x6C9E55`）の側**で、head 自体は Deactivate/Suspend。**state 1/5/6 への遷移は activate/cycle 側**（v376 の記述は呼び出し元について正しい）。

---

## §2 証拠

### §2.1 `0x6C9CB0` = Deactivate/Suspend [確定]

- **先行裁定 v187_anim_residuals.md L142**: 「**0x6C9CB0 = Deactivate/Suspend**（+0x44→3/4 のフェードアウト設定 or 即時停止）」（同 L140-141 で `0x6C6B30`＝テキストキー同期・**state6 のサイクルオフセット算出で使用**と併記）。
- **v377_1(a) 補強**: 既知 vtbl（NiControllerSequence `0xA79A34`／NiControllerManager `0xA79804`／NiTimeController `0xA7EB24`／NiInterpController `0xA7A16C`／NiSingleInterpController `0xA7A08C`／NiTransformController `0xA79634`／NiKeyframeManager `0xA7B3FC`／NiBlendAccumTransformInterpolator `0xA79584`／NiBlendTransformInterpolator `0xA79E54`／NiMultiTargetTransformController `0xA79984`）の**各48スロット走査で `0x6C9CB0` 不在**（no hit）＋ **non-.text セクションの u32 テーブル走査でも不在**（none）→ **非仮想の直接呼び出し fn** [確定(否定側)]。
- **v377_1(b) 末尾**: `0x6C9CD9: mov dword [ecx+0x44],eax`（**eax＝引数 state**）→ `fld dword [0xA7DEB4]; fchs; mov al,1; fstp [ecx+0x4C]; fstp [ecx+0x50]; ret 8`（2引数・`-const` を2フィールドへ＝フェード時間 [推定]）。
- **v377_1(c) callers**: `0x471A70`（`mov ecx,eax; call` → **`cmp [eax+0xAC],5`**）／`0x4745ED`（`mov ecx,ebp; call` → **`cmp [ebp+0x44],0; je`**）／**`0x4B7F15`/`0x4DE5C2`/`0x4DE561`/`0x4E30B6`＝v172 のエフェクト減衰チェーン（`0x49F950→0x4E0D90→0x6C9CB0→0x4D90D0`）と一致**。

### §2.2 `[this+0x44]` state enum（v377_1(b) 全 writers）

| state | site | 文脈 |
|---|---|---|
| **0**（停止/Reset） | `0x6C9D14` | `[ecx+0x44]=0; [ecx+0x58]=0; [ecx+0x54]=-const; call 0x6C6AC0; al=1`（time/phase ゼロ化） |
| **1**（再生/Activate） | `0x6C9C68`／`0x6C9D80`／`0x6C9EAC` | `[esi+0x40]+0x4C`（sequence ptr）へ push → `0x73A5E0` |
| **2** | `0x6C9C2E` | `[esi+0x44]=2`（意味 [部分確定]） |
| **3/4**（フェードアウト/停止） | Deactivate/Suspend 内（`0x6C9CD9` の eax） | v187 先行裁定 |
| **5** | `0x6C9C0E`／`0x6C9E72` | `[esi+0x44]=5`・caller `0x471A70` の `cmp [eax+0xAC],5`（意味 [部分確定]） |
| **6**（サイクル） | `0x6C9EDD` | v187「state6 のサイクルオフセット」（`0x6C6B30` テキストキー同期で算出） |

- **整合**: v376 が実測した `0x6C9D70`/`0x6C9E55`（`0x6C6A50` callers）は **state 1/5/6 へ遷移する activate/cycle 側 fn**、head `0x6C9CB0` は **Deactivate/Suspend（3/4）** — 両者は別 fn で、`0x6C9D70`/`0x6C9E55` が head を `push 1; push ecx; call 0x6C9CB0` で呼ぶのは**自オブジェクトの Deactivate 経由**（v376_1(a) の `0x6C9D3C`/`0x6C9DD9`/`0x6C9E0D`）。

## §3 残点 **V-377-1..2**
1. **state 2 と 5 の意味**（`0x6C9C2E`/`0x6C9C0E`/`0x6C9E72`・caller `cmp [eax+0xAC],5`。b07 `NiControllerSequence` の cycle type enum と照合で切り分け可）【中】
2. **`[this+0x4C]`/`[+0x50]`/`[+0x54]`/`[+0x58]` のフィールド名**（fade time/phase/time/cycle offset [推定]・`0xA7DEB4` 定数の値）【低】

## §4 検証・自己批評
- probe: **v377_1 全 EXIT=0**。
- **決定打の作り**: (i) b07/tmp/re の先行裁定 grep で **v187 L142 の既存裁定を発見**（重複解析を回避）→ (ii) vtbl48スロット×10種＋non-.text fn ptr テーブルの**両方の不在走査で非仮想 fn を証明**→ (iii) `[this+0x44]` writers census で state enum 全値を列挙。
- 妥当性チェック: ①v187 の「+0x44→3/4」⇔ `0x6C9CD9` の `[ecx+0x44]=eax`（引数）⇔ callers の `cmp [ebp+0x44],0` — ②state 6⇔v187 の「サイクルオフセット」⇔ `0x6C9EDD` — **矛盾なし**。
- docs との不整合: **なし**（v187 先行裁定を本 v377 が補強・v376 の「activate/cycle 系 [推定高]」は呼び出し元 fn については正、head 自体は Deactivate/Suspend と整理）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
