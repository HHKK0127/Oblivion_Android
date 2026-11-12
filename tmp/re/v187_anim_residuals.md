# V175 / 束G: アニメーション残件束 — s75_anim 残点 C1–C4 解析 (v187)

作成: 2026-10-29 (v187, TODO serial 175)
対象: Oblivion.exe v1.2.0416 Steam (ImageBase 0x400000)
出典: tmp/re/s75_anim.md §(C) C1–C4、v180_letter_rows_audit.md 束G、TODO_re49.md L143 (R2 行)、docs/Oblivion_decompiled.c、tmp/re/cache/

## 残点リスト (s75_anim.md §C)

- [C1] NiControllerManager::Activate (0x716140) full decode
- [C2] NiControllerSequence update loop (0x6C87A0) internal
- [C3] NiBlend weight computation (priority/based crossfade)
- [C4] Sneak/Swim animation dispatch path

## 前提検証メモ (解析中)

- C1 対象 0x716140: デコンパイル (decompiled.c L558211) では "m_fFrequency/m_fPhase/m_fLoKeyTime/
  m_fHiKeyTime/m_fStartTime/m_fLastTime/m_pTarget/anim type/cycle type/Active/Play Backwards" を
  書き出す関数 = **NiTimeController 系 Save メソッドの可能性**。→ s75 の "Activate" 命名を検証中。
- C2 対象 0x6C87A0: デコンパイル (L498008) では alloc(0x68) + ctor 0x6C6550 + 0x6C70A0(stream) =
  **ファクトリ/ロード関数の可能性**。→ s75 の "Update" 命名を検証中。

---

## [C1] NiControllerManager::Activate (0x716140) full decode — **解明（前提訂正あり）**

### 判定
**s75 の前提が誤り。0x716140 は Activate ではなく NiTimeController::Save（テキスト形式セーブ）**。
NiControllerManager ヴァーチャル関数に "Activate" は存在しない（Gamebryo 2.0 では Activate は非仮想の
ActivateSequence 系）。C1 は「0x716140 の正体 + アクティベート実経路」の解決として完了。

### 証拠（docs/Oblivion_decompiled.c L558211-558297）
- 0x716140 は以下の NiTimeController フィールドを名前付きでストリーム出力する:
  m_fFrequency(+0xC) / m_fPhase(+0x10) / m_fLoKeyTime(+0x14) / m_fHiKeyTime(+0x18) /
  m_fStartTime(+0x1C) / m_fLastTime(+0x20) / m_pTarget(+0x30) /
  "anim type"(byte+8 bit0) / "cycle type"(bit1-2) / "Active"(bit3) / "Play Backwards"(bit4)。
  → Gamebryo の NiTimeController レイアウト・フラグ定義と完全一致。セーブ処理であり再生処理ではない。
- 直接呼び出し 9 サイト（callers() スキャン）: 0x6D05CA, 0x6D59F7, 0x6D9397, 0x6DDE77, 0x6E06F7,
  0x6ECA67, 0x753DC7, 0x754F87, 0x8AAE71 — いずれも他 NiTimeController 派生クラスの Save が
  ベース Save を呼ぶ経路。NiControllerManager は slot+0x30 でこれを継承（未オーバーライド）。
- バイナリ版セーブ = **0x716050 = NiTimeController::SaveBinary**（L483-504: +0x8 u16 flags、
  +0xC..+0x18 float 4個、+0x30 m_pTarget を stream 経由 SaveObject [stream vt+0x2C]、
  +0x34 連結リストを vt+0x6c ゲートで走査）。NiControllerManager::SaveBinary =
  slot+0x28 = **0x6C4DA0**（L494640: 0x716050 でベース分セーブ → +0x6C bool → +0x46 カウント →
  +0x40 のシーケンス配列を stream->SaveObject で逐次セーブ → +0x7C オブジェクトパレットも SaveObject）。
  → s75 は 0x6C4DA0 を "AddSequence" と誤称していた。

### 実際のシーケンス活性化経路（Activate 相当）
- **AddSequence = 0x6C5760（非仮想）**: seq[+0x40]=manager を設定、既にアタッチ済みなら 0 を返す、
  manager[+0x30]（オブジェクトパレット）を渡して **0x6C9590 = NiControllerSequence::StoreTargets**
  （内部エラー�文字列 "NiControllerSequence::StoreTargets '%s' failed to resolve dependencies..." を
  decompiled.c L398 で確認）でターゲット解決 → NiTStringPointerMap へ挿入（0x6C5240/0x412D30）。
- **0x6C5C20（manager slot+0x20）**: 対象の既存コントローラ列挙（0x7124D0/0x7124A0）から
  0x6C5760 を一括呼ぶ（AttachTarget 時の再アタッチ）。
- 毎フレーム活性化状態の推移は C2/C3 の 0x6C4200 + 0x6CA950 状態機械が担う（下記）。

### s75 vtable ラベル訂正表（NiControllerManager 0xA79804, 30 スロット — s75 は16と誤記）
| slot | VA | s75 誤称 | 実体 |
|---|---|---|---|
| +0x18 | 0x6C5CC0 | Update | Clone 系ファクトリ（new 0x80 + ctor 0x6C5520 + 引き写し 0x6C5A10）|
| +0x1C | 0x6C4E50 | SetTarget | （未詳細化・SetTarget 候補維持）|
| +0x24 | 0x6C5AB0 | GetSequenceByName | IsEqual 系（各子 vt+0x24 を再帰照合、+0x7C パレット欠損時は生成して照合）|
| +0x28 | 0x6C4DA0 | AddSequence | **SaveBinary**（上記）|
| +0x2C | 0x6C42B0 | RemoveSequence | **IsEqual**（+0x46 カウント一致 → +0x40 配列を vt+0x2C で要素照合）|
| +0x30 | 0x716140 | Activate | **Save（テキスト）= NiTimeController::Save 継承** |
| +0x38 | 0x6C4C20 | DeleteSequence | CloneDeep 系（NiPointerMap で共有参照を保ちつつ複製）|
| +0x54 | 0x6C4200 | （未記載）| **NiControllerManager::Update（実体）**: Active フラグ(bit3) 免許付きで +0x4C 配列/+0x54 個数の
  シーケンスを 0x6CA950 へ毎フレーム呼ぶ、+0x44==0 になったシーケンスは swap-with-last で除去 |
（+0x08..+0x14, +0x40, +0x44, +0x50 等の 0x6F7070/0x60D0A0 は空/stub）

---

## [C2] NiControllerSequence update loop (0x6C87A0) internal — **解明（前提訂正あり）**

### 判定
**s75 の前提が誤り。0x6C87A0 は Update ではなく Clone 系ファクトリ**（new(0x68) + ctor 0x6C6550 +
引き写し 0x6C70A0）。シーケンスの実更新ループは **0x6C4200（manager 側）→ 0x6CA950（sequence 側本体）**。

### 証拠
- 0x6C87A0（L498008-498036）: `operator new(0x68)` → ctor 0x6C6550 → FUN_006c70a0(新規, 引数)。
  0x6C6550（L1229-1267）は NiControllerSequence 構築（*obj = vtable 0xA79A34、+0x1C=1.0f、
  ±FLT_MAX 初期化…テキストセーブ 0x6CACE0 のフィールドと一致）。0x6C70A0（L1272-）は
  名前(+8)/配列(+0x14,+0x18,各 0x10 要素)/+0x20/+0x40 等を vt+0x18(Clone) 経由で引き写す。
  → vtable slot+0x18 は両クラス共通で "Clone(NiStream&)" 系。NiControllerSequence vtable は
  s75 記載の12ではなく **19 スロット**（cache/vtable_index.tsv 行993: count=19）。
- slot+0x1C = 0x6C8E40 も "Activate" ではなく **LoadBinary**（L498199-: stream(+0xD8) の版数
  0x0A010068 で分岐、+8/+0x5C の NiFixedString 読込、+0xC ArraySize 読込、0x10 要素×2 配列 +
  0x140B/エントリのベクタ構築、文字列 ID 解決）。s75 が Activate と呼んだのはここ。
- **実更新ループ**:
  1. 0x6C4200（manager）: byte+8 bit3（Active）==1 のときのみ、+0x4C 配列（+0x54 個）の各シーケンスに
     0x6CA950(シーケンス, 時刻) を呼ぶ。+0x44==0（終了）のシーケンスは末尾スワップで除去。
  2. **0x6CA950 = NiControllerSequence::Update 本体**（L188-303）: +0x44 状態機械
     （1=アクティブ確定, 2=フェードアウト線形, 3=残時間で減衰, 4=同4, 5=フェードイン線形 +
     終了時刻 +0x54 予約処理, 6=サイクルオフセット補正 [FUN_006c6b30 / 対象 +0x58]）、
     +0x48/-0x48, +0x4C(fade start), +0x50(fade end), +0x54(reserved end) を時刻比較して
     ブレンド係数 local_8（横軸 ramp）/local_4（縦軸 ramp）を算出し、
     **FUN_006c6dc0(seq->m_fWeight(+0x1C) * local_4, local_8, FUN_006c5fc0(時刻,1))** で
     バインド先インポータに重みを伝播。終了条件で +0x44 を 0 化（= manager 側が除去）。
  3. StoreTargets 0x6C9590（依存解決）はシーケンス+0x14 配列（0x10 要素: interpolator, name ID ×5）を
     パレット照合して +0x18 配列へ解決済みを格納（失敗時は上記文字列で警告）。
- GetDuration も s75 誤り: 0x6C77A0 は +0x14/+0x18/+0x20/+0x64 の各子に vt+0x24 を投げる
  **IsEqual 系**（真の GetDuration は未特定、VT +0x24 ではない）。

---

## [C3] NiBlend weight computation (priority/based crossfade) — **大部分解明**

### 判定
ブレンド重みの**データ経路・フィールド意味・レジストラ／解除ロジックまで判明**。最終的な
加算数式（single/multi ブレンド本体 0x6CBDF0 / 0x6CD0F0・0x6CBEE0、Transform 適用 0x6CCBC0）は
入口特定に留まり本文未精査 = **部分残**（新残点 N2 へ）。

### 経路（sequence → blend interpolator）
1. **NiControllerSequence::Update 0x6CA950**（C2 参照）が +0x44 状態機械で ramp 係数
   local_8/local_4 を作り、**0x6C6DC0** を呼ぶ。
2. **0x6C6DC0 = 重み伝播**: 各バインド（+0x14 配列, 0x10 stride: interpolator, 名前ID×5）に対し
   - `interp->blendUnit[idx*0x18 + 4]` ← m_fWeight（引数2: seq重み×ramp）
   - `interp->blendUnit[idx*0x18 + 0x10]` ← m_fEaseSpinner（引数3: FUN_006c5fc0 のシーケンス時刻）
   - `interp->blendUnit[idx*0x18 + 0x14]` ← 引数4（逆再生時の位相）
   - interp+0x24/0x28/0x2C を -FLT_MAX にリセット、interp+0x0C |= 4（再計算要求フラグ）。
   優先度==1 の特殊ケースは interp+0x20 へ直書き。
3. **NiBlendInterpolator オブジェクト布局**（0x6CDDF0 相当 FUN_006cddb0 のテキストセーブが
   Gamebryo フィールド名を明示: ManagerControlled, m_fWeightThreshold, m_bOnlyUseHighestWeight,
   m_ucInterpCount, m_ucSingleIdx, per-unit: NiControllerSequence, m_cPriority, m_fWeight,
   m_fNormalizedWeight, m_fEaseSpinner）:
   - +0x0C flags: bit0=ManagerControlled / bit1=m_bOnlyUseHighestWeight / bit2=再計算要求
   - +0x0D m_ucArraySize, +0x0E m_ucInterpCount, +0x0F m_ucSingleIdx (0xFF=なし)
   - +0x10/+0x11 最低/最高 priority バイト（0x80 でリセット）
   - +0x14 ブレンドユニット配列（0x18/ユニット: [0]interp, +4 m_fWeight,
     +8 m_fNormalizedWeight, +0xC m_cPriority, +0x10 m_fEaseSpinner, +0x14 相位）
   - +0x18 m_pkSingle（InterpCount==1 の単体）、+0x1C m_fWeightThreshold,
     +0x20 m_fTotalW、+0x24/28/2C キャッシュ（-FLT_MAX=無効）
4. **0x6CC5C0 = RegisterInterpolator**（NiBlendTransformInterpolator vt+0x98）:
   空きユニット探索 → interp/priority/weight 格納 → +0x0E++ → priority 範囲更新。
   数が満杯なら vt+0xA4（拡張）を試して失敗時 0xFF。
5. **0x6CC6E0 = UnregisterInterpolator**（vt+0x9C）: ユニットクリア、残ユニットから
   最低/最高 priority を再計算、InterpCount==1 で m_ucSingleIdx/m_pkSingle 決定、0 なら無効化。
6. **0x6CC3A0（vt+0x4C）= アプリケーション分岐**: InterpCount==1 なら **0x6CBDF0**（単体高速経路）、
   2以上なら **0x6CD0F0 + 0x6Cbee0**（multi: 正規化合算）。priority 抑制（高 priority が
   m_bOnlyUseHighestWeight で低を上書き）と weight 归一化はここで実装、とフィールド意味から一義的に導ける。
7. 関連: **0x6C6B30 = テキストキー同期**（prefix 比較 strnicmp(PTR_DAT_00B241C4, len 2) で
   対象シーケンスのキーを走査し線形補間 — state6 のサイクルオフセット算出で使用）、
   **0x6C9CB0 = Deactivate/Suspend**（+0x44→3/4 のフェードアウト設定 or 即時停止）。

### s75 訂正
- s75 B7 の "handler 0x6CBCE0 (size 0x30) / 0x6E9030 (size 0x70)" は NIF クラス登録ファクトリ
  （0x6E9030 は named_funcs 上 CreateObject 命名）であり、重み算出関数ではない。
- 本物の vtable: NiBlendTransformInterpolator **0xA79E54（42スロット）** /
  NiBlendFloatInterpolator **0xA7A49C（11）** / NiBlendInterpolator 基底 **0xA79F0C（13）** /
  NiBlendAccumTransformInterpolator **0xA79584（40）**（cache/vtable_index.tsv 行863-870 で台帳化済）。
- s75 B8 の fAnimationDefaultBlend/fAnimationMult は B0D37C 単一アドレスとあるが、
  本ラウンドでは未再検証（キープ）。

---

## [C4] Sneak/Swim animation dispatch path — **部分解明（表構造・合成系は確定、実行時 decision 経路は残）**

### 解明
1. **AnimGroup 名テーブル 0xB10290 = 平坦21エントリ（4B stride）** — s75 B4 を確認:
   0-3=Left/Right/Left Back/Right Back Foot, 4=Idle, 5=Aware, 6=Attack, 7=Hit, 8=Death,
   9=Weapon, 10=空, **11=Sneak, 12=Swim**, 13=Fly, 14=空, 15=HandToHand, 16=OneHand,
   17=TwoHand, 18=Staff, 19=Bow, 20=Idle（トーチュ用重複）。
2. **隣接する36バイトストライド属性表を新発見**（0xB102E0 起点・index=byte[+8]）:
   0=Idle, 1=DynamicIdle, 2=SpecialIdle, 3=Forward, 4=Backward, 5=Left, 6=Right,
   7=FastForward, 8=FastBackward, 9=FastLeft, 10=FastRight, 11=DodgeForward, 12=DodgeBack,
   13=DodgeLeft, 14=DodgeRight, 15=TurnLeft, 16=TurnRight, 17=Equip, 18=Unequip。
   レコード内: +0=name, +4=byte フラグ, +0xC=ループタイプ enum(0-6) 等。
   ※平坦21リストと36Bテーブルは**別の配列**（メモリ上隣接・index も別: 11 は平坦=Sneak/
   records=DodgeForward）。
3. **TESAnimGroup ビットフィールドゲッタ3本を実体確定**（+8 に packed）:
   0x51AC70 = byte[+8]（低8bit）、0x51AC60 = (word[+8]>>8)&0xF、0x51AC50 = word[+8]>>12、
   0x51AA00 = movzx byte 引数（identity: flat group id 取得ラッパ、48 箇所から使用）。
4. **KF 名合成＋インポータ検証系を判明**: FUN_00472030（0x472030, caller 0x472677）は
   "Could not find AccumRoot in animation '%s'" と
   "AnimGroup '%s%s%s' for '%s' was exported with 'Animate in Place' from MAX."（0xA3C780）を
   出力する **KF インポート検証**。名前3部は 36B レコード名 + 0x51AC60/0x51AC50 経由の
   サブ名（0xB102C8/0xB102B8 基点の平坦配列）を連結。ループタイプ case 0/1/4/5/6 のみ
   in-place 判定対象。武器スキル KF 経路（0x4766xx 帯の7サイト）と0x470720 の
   slot remap（slot5→idx0, slot6→idx3, word[this+0x3C+idx*2]）も再確認。
5. **即値による group11/12 書込みは .text 全域で0件**（`mov [reg+8], 0x0B/0x0C` を
   byte/word/dword 3形式でスキャン、0x51AC70 呼び出し ±0x30 に cmp 0xB/0xC も
   インポータ検証 0x47229E のみ）→ **sneak/swim の group id は実行時に計算されて選択される**。

### 未解明（残点）
- 実行時に Actor 状態（sneaking フラグ/水中移動）から AnimGroup 11/12 が選択され
  シーケンスが切り替わる **decision 経路本体**（0x5E-0x60 帯: 0x51AC70 呼び出し 29 箇所の
  うち 0x5EC1AD/0x5F0020/0x5F4409/0x5FCC15/0x5FD7C9 等が候補）。→ 新残点 N3。

---

## 総括

| 残点 | 判定 | 主証拠 |
|---|---|---|
| C1 | 解明（前提訂正: 0x716140=NiTimeController::Save。Activate 相当は非仮想 AddSequence 0x6C5760 + Update 0x6C4200 系）| decompiled.c L558211/L483/L494640、callers 9 サイト |
| C2 | 解明（前提訂正: 0x6C87A0=Clone。実更新=0x6C4200→0x6CA950 状態機械、依存解決=0x6C9590 StoreTargets）| decompiled.c L498008/L188-303/L398 |
| C3 | 大部分解明（重み伝播 0x6C6DC0・登録/解除 0x6CC5C0/0x6CC6E0・フィールド完全対応。数式本体 0x6CBDF0/0x6CD0F0/0x6Cbee0/0x6CCBC0 は入口のみ）| FUN_006cddb0 テキストセーブのフィールド名、vtable 0xA79E54 等 |
| C4 | 部分解明（21名表+36B属性表+ゲッタ3本+インポータ検証を確定。実行時 sneak/swim decision 経路は残）| 0xB10290 ダンプ、0x472030 デコンパイル、即値スキャン0件 |

## 新規残点（親セッション起票用）

- **N1（低）**: s75_anim.md 本体の訂正反映 — B1/B2 の vtable ラベル（16/12スロット→30/19、
  slot+0x30 Activate→Save、slot+0x18 Update→Clone、slot+0x1C Activate→LoadBinary、
  slot+0x28 AddSequence→SaveBinary、0x6C77A0 GetDuration→IsEqual）と B7（ハンドラ≠重み算出）、
  §C の C1-C4 消化マーキング。vtable_index.tsv が正（NiBlend 系 vtable 4件も台帳化済）。
- **N2（中）**: ブレンド最終数式の本文精査 — 0x6CBDF0（single）、0x6CD0F0/0x6Cbee0
  （multi 正規化）、0x6CCBC0（Transform 適用）、0xA79584 の Accum 版42スロット。priority
  抑制（m_bOnlyUseHighestWeight/m_cPriority）の分岐条件と m_fNormalizedWeight 計算式の確定。
- **N3（中）**: sneak/swim 実行時 dispatch — 0x5E-0x60 帯の group-id decision 経路
  （0x51AC70 呼び出し 29 箇所のシャント解析、TESAnimGroup 設定関数の特定）。
- **N4（低）**: NiControllerManager slot+0x1C=0x6C4E50（SetTarget 候補）と真の
  GetDuration/真の ActivateSequence（非仮想候補 0x6C4890-0x6C4BD0 帯）の残り整理。

## 使用ツール・経路メモ

- capstone/pefile: `tmp/re/re_util.py` + `%TEMP%/opencode/v187_*.py`（round2-7, extract 系）
- docs/Oblivion_decompiled.c（29MB, Ghidra 出力）: 関数ヘッダ `// FUN_xxxxxx @ 0xxxxxx` を
  スライス抽出
- cache/vtable_index.tsv: vtable アドレス+スロット数の一次台帳（RTTI COL 検索は
  .data の "?AV" 文字列が MSVC COL 形式でなく不通 → 台帳利用が正解だった）
- cache/named_funcs.txt: 関数名（小文字16進キー）

（了）
