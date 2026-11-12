# v299_pending_mask_setter.md — serial 282（pending マスク最終決着束・4件）

- 作成日: 2026-11-05 / 担当: 本セッション内実施
- 出典: tmp/re/v298_ini_registration_bundle.md 残点1..3 ＋ 台帳 row282 起票文
- 中間物: `%TEMP%\opencode\v282_i1.py/_out`（ptr-refs・bit1 窓走査・0xB35574 全参照）・`v282_i2.py/_out`（vft RTTI・fn 内文字列・setter/ctor 文脈）・`v282_i3.py/_out`（dispatch fn 0x4662C9 の caller/文字列）
- 判定サマリ: **件1=確定（非ゼロ書手否定＋再解釈）/ 件2=部分確定（bit2 確定・bit1 狭域化）/ 件3=部分確定 / 件4=ドラフトは v298 に記載済（本束は参照のみ）**

## 件1 pending マスク [ctx+0x50] の非ゼロ書手 — 【確定（否定＋再解訳）】

### 捜索範囲と結果（命令証拠）

1. **save/load band 整列 walk 0x440000..0x470000（関数 904 本）**: `[reg+0x50]` への**オブジェクト書込**は 3 点のみ（v298 §件1 で否定済）: 0x446DE8=memset 側 / 0x44976A=list head pop / 0x4670FC=型別 binder。他は `[esp+0x50]`=スタック局所。
2. **0x595190（code=0xc→[ecx+0x50] の仮想プロパティセッタ）の帰属 = `.?AVAudioMenu@@`**（vft 0xA6B29C・COL→TD 実測）。ctor 0x595150 は `[esi]=0xA6B29C` + [esi+0x28..0x50] を -1 fill＝**AudioMenu の UI プロパティ**で、SLG の pending とは無関係[確定負]。E8 caller 0x58812F（AudioMenu 生成帯）。
3. **0x5D335A の owner = `.?AVSaveMenu@@`**（vft 0xA6D624・ctor 内 `mov [esi],0xA6D624` 実測）。0x5D335A は `[esi+0x50]` を eax=0 で**クリア**する dtor 末尾＝UI メニューのフィールド初期化[確定負]。0x5C071E の fn は new(0x3F8=1016B)＋factory call で **GUI メニュー帯**[確定負]。
4. **dispatch fn 0x4662C9（LoadGame 参照ローダ）**: 内部文字列 `'Save Game Header'`/`'...changed cells...loading will be skipped'`/`'actor %08X: Actor has a middle high process but no parent cell'` で Identity 確定。**E8 caller ゼロ＝jump table 到達**。内部で [ebp+0x50] は読取（0x46675A→452D60 動的マスク）とゼロ化（0x46676B）のみ、**非ゼロ書込なし**。

### 再解訳（本件の結論）

**「pending マスク」は独立した書き込み元を持たない**。[ctx+0x50] の非ゼロ値は **Save レコード自身の flags dword が parse 中に mirror される**（0x46683D→452C90 が entry flags へ store＝v289 §件1 済み、対応する ctx フィールドに同値が写る）。0x463A6E/0x46675A の「動的マスク」は parse 後の mirror を読む側であり、書手は「レコード解析ループ内の memcpy 的 mirror」であって **OR/ADD でビットを足す関数は .text に存在しない**（v298 §件1 で or/and ゼロを確認済、本束で mov-nonzero も否定）。

- **裁定: [確定] 書手否定＝再解釈「レコード flags mirror」**。v297 §件1 の [推定]「SLG+0x50 pending」は**呼称を訂正**し、「LoadGame parse-time record-flags mirror field（書手はレコード解析経路のみ・独立 setter なし）」へ。
- docs 反映候補+1（下記）。

## 件2 pass2 `test bl,6`（bit1|bit2）— 【部分確定】

- **bit2 = 古典形式(0x5B 未満)の key フォーマット選択**（v286 §件1 確定済・再確認）。pass2 の `test bl,6` skip＝bit1|bit2 が立つレコードは再発行されない＝**古典系レコードの保護**。
- **bit1 の生成側**: 0x452000..0x465000 の `83/1(imm=2)` 窓は 3 点（0x452A77 `or [ecx+0x18],2` ret 4 / 0x45C2CF 同型 / 0x45FDD6 `or [edi+0x18],2`）で、いずれも **+0x18 フィールド（form 級の changed フラグ系）**であり record flags（+0x50 mirror）への bit1 書込ではない[否定的根拠]。bit1 は **セーブファイル側（ESM/セーブデータ）由来の値域**で、EXE 内に独立生成窓なし＝静的到達不能[確定（負方向）]。
- 残: bit1 のセーブ側語義（GECK/セーブデータ域＝禁止域）→ 恒久保留。

## 件3 0xB35574 単例マップの所有者 — 【部分確定】

- .text 参照 19 点（v282_i1 (4)）の帰属:
  - **0x40561D** = fn 0x4055DA 内＝v298 §件3 の**ノード破棄関数 0x4055E0 の直後**（unlink 対象マップ）
  - **0x419AB8/0x419AF1** = fn 0x419A70 内＝v298 §件3 の**スカラー INI 登録関数**（登録先マップ）
  - 0x40C54F..0x40C7DB = fn 0x40C3FB（teardown 帯・文字列なし）
  - 0x51404B/0x514183・0x575CF9..0x575D33 = 登録/参照の周辺 consumer
- **裁定: 0xB35574 = INI 値ストア（スカラー/文字列の実行時 value map）**＝登録（0x419A70）と破棄（0x4055E0）が同一マップを往復する INI ランタイム value store[部分確定]。正式クラス名は RTTI なし（非 polymorphic 候補・v274 §6 の SLG と同型の「vtbl 非設置」パターン）。
- 0xB07BF0（v291 §件3 の SettingsManager 単例）と 0xB07CFC（v291 §件2 の INI 名リスト）との関係: **0xB07BF0(登録 API owner) → 0x4040E0/0x419A70 → 0xB07CFC(名リスト)＋0xB35574(値 store)** の 3 層構造[推定→本束で登録/破棄の往復を確認し強化]。

## 件4 TOFT 訂正ドラフト — 【参照のみ】

ドラフト本文は v298 §件(5) に確定記載済（before: `+8=0xA3BA94（異常値）` / after: `0x00A3BC94＝隣接静的 NiTPointerMap<uint,TESForm*> のスロット境界共有・v265 1 桁誤り`）。本束は重複執筆せず参照。

## docs 反映候補（+1 点・#20/#47）

1. pending マスクの呼称訂正: 「SLG+0x50 pending setter」→「LoadGame parse-time record-flags mirror（独立 setter なし・レコード解析経路のみ）」— v297 §件1 の [推定] を差し替え

## 残点（次束候補・低）

1. 0xB07BF0→0xB07CFC→0xB35574 の 3 層構造の最終確認（0x4040E0/0x419A70 の key 引数が 0xB07CFC ノードを指すかの命令突合）
2. 0x40C3FB（teardown 帯）の所有クラス（0xB35574 の dtor 側・RTTI needle 帯 0xB05xxx TD の一覧化は v298 件(1) の隣接産物）
3. bit1 のセーブ側語義（恒久保留・禁止域）

## 検証（selftest 相当）

`v282_i1..i3.py` 再実行 EXIT=0。数値整合: 0xB35574 参照 19 点（1+7+2+2+1+6）＝v282_i1 出力と一致。0x595190 ptr-refs 1 件（0xA6B2A0=AudioMenu vtable slot1）・0x5D335A 归属は ctor 内 vft store 実測。dispatch fn 0x4662C9 は v297 §件1 の 452D60/DF0 caller 一覧（0x466755/0x46684C）と整合。既存 tmp/re・docs・GT・ソース無編集、ESM/git 非接触。
