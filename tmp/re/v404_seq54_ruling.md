# v404 — `[seq+0x54]` の正名と役割の裁定（#153 ⑥-10・V-388-1）

- 採番: **v404**（ai-todo #153 束の第15弾＝低10件の第10弾、probe prefix v404_1..v404_3）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v388 の残点 **V-388-1（`[+0x54]` 正名）** を消化（v403 の残点「`[+0x54]` の名前と供給元」と同一件）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **正式名** | **[部分確定]（名前=[確定(否定側)]／役割=[確定(命令＋decompiled)]）** | **名前: バイナリに存在しない** — 3系統の調査で否定: ①**sequence 自身の text dump（`FUN_006ca440`）は13スカラのみ**で +0x54 は不在（State/ArraySize/Weight/CycleType/Freq/Begin/End/Last/Offset/Start/End/WeightLast/LastScaled）②**`.rdata` の `m_f*` 文字列 全31件を在庫調査**（m_fScale/m_fEaseSpinner/m_fWeightThreshold/m_fLastTime/m_fStartTime...）— **該当候補なし**（m_fLastTime 等は他クラスの dump 用＝`FUN_00716140` のキー系 dump 等）③**sequence band（L499000-500000）の `m_` 名 dump＝0件**＋コンソール/ログでの言及なし。MSVC バイナリはフィールド名を持たないため名前は取得不能 |
| **役割（代替として確定）** | **[確定(命令＋decompiled)]** | **blend 開始時に両 sequence へ書き込まれる「内部時刻スナップショット（one-shot time override）」**:<br>①**書込**: `FUN_006c9d30`（blend wrapper・v393）成功時 **`this[+0x54] = LastScaled[+0x3c] / Freq[+0x28]`**・**`other[+0x54] = param_4 / other.Freq`**（L498988-89・machine `0x6C9DE6`/`0x6C9DF0`）<br>②**未設定 = −FLT_MAX**（Deactivate fade<=0 L498950・state6 消費 L499588・ctor `0x6C65EA`/`0x6C8081`）<br>③**消費A（評価時の time 上書き）**: `FUN_006ca950` default 経路で **`if (+0x54 ≠ −FLT_MAX) time := [+0x54]`**（L499610/499625）→ 直後 **time remap `FUN_006c5fc0(time, 1)`（v390・flag=1＝clamp 経路）** → **`Weight × fade`** を `FUN_006c6dc0` へ（L499627-628）＝**保存した内部時刻で pose 評価を固定する**<br>④**消費B（Offset への畳み込み）**: state2 出口で **`Offset = [+0x54] − time` して −FLT_MAX へクリア**（L499586-588・machine `0x6CAA4A`）→ v403 の Offset 数式に接続<br>⑤通常時（−FLT_MAX）: `time + Offset` / partner 経路（v403 と整合） |
| （補足）`FUN_006c6b30` は非参照 | **[確定(否定側)]** | pose query `FUN_006c6b30`（body L496499-496635・137行）に **0x54 参照0件** ＝上書きは update 側（`FUN_006ca950`）だけで完結 |
| **（副次・v401 残点1の解消）block 配列の container** | **[確定(命令＋RTTI 由来)]** | **container ＝ NiControllerSequence** — 根拠: ①sequence vtable **idx15 = `FUN_006cace0`**（v402 の vtable dump）が **`[this+0xc]`（count）＋`[this+0x14]`（stride16 配列）** を使っている（L496661-49675・+4 ref release/+8 zero＝block 構造）②dump の **`ArraySize` ＝ `[+0xc]`** ③`FUN_006c70a0`（v401 の構築 fn）が **NiFixedString の name コピー**（strlen/new/copy 経路 L496830 帯 machine `0x6C70E9-0x6C710A`）を伴う＝sequence の name 保有と整合 → **v401 残点1「container＝manager/sequence?」は sequence に確定** |

**V-388-1 への回答**: **正名は [未確定]（バイナリに名前不在＝確定(否定側)）**。役割は **「blend 時に LastScaled/Freq 等として保存され、①評価 time の上書き（→v390 remap→Weight×fade）②Offset への畳み込み、の2系統で消費される one-shot 内部時刻スナップショット」[確定]**。

---

## §2 証拠

### §2.1 書込・未設定・消費（v404_1 machine census 10件 ＋ v404_2 decompiled）

| site | 命令/式 | 意味 |
|---|---|---|
| `0x6C9DE6`/`0x6C9DF0` | `fstp [esi+0x54]`／`fld [esp+0x14]; fdiv [edi+0x28]; fstp [edi+0x54]` | **書込**: this=LastScaled/Freq・other=param_4/Freq（L498988-89） |
| `0x6C65EA` / `0x6C8081` | `fstp [esi+0x54]`（ctor 系） | 初期化（−FLT 系） |
| `0x6C9D22` | `fstp [ecx+0x54]`（Deactivate fade<=0 内・`call 0x6c6ac0` 前） | −FLT_MAX リセット（L498950） |
| `0x6CAA4A` | `fld [esi+0x54]; fsub st(1); fstp [esi+0x48]` → `[+0x54]=−FLT` | **消費B**: Offset へ畳み込み＋クリア（L499586-588） |
| `0x6CAB84` | `fld [esi+0x54]; fld −FLT; fucompp; jnp …` | state1 帯の gate（SET/UNSET 分岐） |
| L499610/499625 | `if (−FLT == [+0x54]) {…} else { time = [+0x54]; }` | **消費A**: time 上書き（default 評価経路） |
| L499627-628 | `FUN_006c5fc0(time, 1)` → `FUN_006c6dc0(Weight × local_4, …)` | 上書き後の pose 評価（v390 remap 接続） |

### §2.2 名前の不在証明（v404_2(b)/v404_3）

- 他クラスの `m_` dump（比較）: `FUN_006cbad0`＝m_kTranslate/m_kRotate/m_fScale（NiQuatTransform）／`FUN_006cddb0`＝m_fWeightThreshold/m_fWeight/m_fNormalizedWeight/m_fEaseSpinner／`FUN_006e4f80`＝m_k*CPHandle／`FUN_006ed580`＝m_fStartTime/m_fEndTime／`FUN_00716140`＝m_fFrequency/m_fPhase/m_fLoKeyTime/m_fHiKeyTime/m_fStartTime — **sequence の m_ dump は存在しない**。
- `.rdata` `m_f*` 全31件（0xA79E14..0xA8146C）に sequence 用候補なし。
- sequence dump 本体の13名（L499341-499433）に +0x54 なし。

### §2.3 副次: container = NiControllerSequence（v401 残点1 解消）

- `FUN_006cace0`（sequence **vtable idx15**・v402 dump）: `[param_1+0xc]` を count・`[param_1+0x14]` を stride16 配列として走査し、各要素の `+4` ref release＋`+8` zero（L496661-49675）＝**block 構造のクリーンアップ**。
- `FUN_006c70a0`（v401 の構築 fn）は **NiFixedString name コピー**を伴う（machine `0x6C70E9-0x6C710A`）→ **sequence が name＋block 配列（count=ArraySize）を持つ**と整合。

## §3 残点 **V-404-1..3**
1. **dump ヘッダ文字列の正体** — `FUN_006ca440` 冒頭の `FUN_0070f910(DAT_00b3cb24)` が先頭に append するグローバル（0xB3CB24）の中身と意味【低】
2. **`FUN_006cddb0`（m_fWeightThreshold/m_fEaseSpinner dump）の対象クラス同定**（`[+0x1c]`=WeightThreshold の構造＝何の dump か）【低】
3. **`FUN_006c9d30` の `param_4`（other seq へ渡す時間元）の供給元**（blend 呼び出し元の実引数意味）【中】

## §4 検証・自己批評
- probe: **v404_1・v404_3 全 EXIT=0**、**v404_2 は出力ファイル生成済み（実行の最終段がサーバ再起動で interrupt・.out 4572B 完全）**。
- **決定打の作り**: (i) x87 disp=0x54 census（R/W 全数）→ (ii) decompiled の書込/消費式を翻訳 → (iii) **名前の3系統調査（当該 dump のフィールド列挙→m_f 在庫 census→band 内 dump 呼び出し全数）で不在を証明** → (iv) FUN_006c6b30 非参照の否定確認 → (v) 副次で idx15 から container=sequence を接続。
- 妥当性チェック: ①`LastScaled/Freq` ⇔ L499621/499596 の同型 `/Freq` — ②`time := [+0x54]` ⇔ 消費B の `Offset = [+0x54] − time`（両方とも +0x54 を"基準時刻"として使う）— ③`+0x54 ≠ −FLT` gate ⇔ 全リセット site — ④idx15⇔dump の ArraySize⇔v401 の block container — **四方向で矛盾なし**。
- docs との不整合: **なし**（v388 の V-388-1／v403 残点1 を消化。名前は [確定(否定側)]・役割は [確定]）。
- **教訓**: **「正名」系は ①当該クラスの text dump フィールド列挙 ②名前文字列在庫（m_f* 等）census ③band 内 dump 呼び出しの全数 の3点で否定してから [未確定]（名前）/[確定]（役割）に分離裁定する**。推測で名前を当てない。＋**disp8/disp32 の教訓（v397）と同様、census は形式を問い全数で**。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
