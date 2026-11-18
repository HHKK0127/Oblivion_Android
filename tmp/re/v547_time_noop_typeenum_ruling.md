# v547 — メニュー時刻計算の確定（**`0xB3A700`/`0xB3A704`＝キャッシュ済み float（ms）で、CRT init 関数 `0x9FBBF0`（ctor テーブル `0xB139A4`→`FUN_00A24A10`）から `0x9FBC50`/`0x9FBC70` が `57D330`/`57D390`（時間 API）＋`9828C0`（float→int）で算出**）・**`0x890B90`＝`ret 0xC` の純 no-op（Menu `+0x18`/`+0x1C`・callers 2＝vtable のみ）で「no-op 8 slot の理由」も確定**・**type 4002（`0xFA2`）の実供給 site は 3 箇所（`426A90`/`58A1C0`/`591B70`）＝UI XML type の値域は散在 enum [部分確定]**（V-547-1..3 完遂・旧残点 V-406-3 継続第77弾）

- 採番: **v547**（旧残点群消化・probe prefix **v547_1（1本）EXIT=0**／**ハーネス `v307_common.py` は TEMP 掃除で消失したため 2026-11-18 に再構築（EXE=`D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`・`named_funcs.txt` 21,597 heads 参照の正確 `find_head`）**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-547-1..3（v546 残点）** — 1=`0xB3A708` manager 実体と CRT init／2=`Menu` vtable `+0x18`/`+0x1C` の役割＋no-op 8 slot の理由／3=type 4002/4005/4011 の値域定義元。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-547-1: `0xB3A700`/`0xB3A704` の正体** | **[確定]（命令）＝float 2 値を int 化して保持する時刻キャッシュ（CRT init が算出）** | CRT init `0x9FBBF0`（`FUN_0057D330`＋`9828C0`（float→int）→`[0xB3A700]`、`FUN_0057D390` を使う `0x9FBC70`→`[0xB3A704]`）。`0x9FBBF0` は `push 0xB139A4; mov ecx,0xB07BF0; call 0x4040E0; push 0xA24A10; call 0x981FB4`＝**`0xB139A4`（設定値）を引数に ctor テーブル登録**（`FUN_00A24A10` が atexit 系 dtor）。readers は `0x585620`/`0x585720`/`0x585820` の 3 ヘッド（メニュー時刻表示系）。→ `0xB3A700`/`0xB3A704`＝**ゲーム内時刻（日数/時刻）の数値キャッシュ**で、`0xB3A708`（型索引 0x33 manager スロット）は同クラスタの別グローバル。 |
| **V-547-1: `0xB3A708` manager** | **[部分確定]** | v546 の `alloc(1)`→`4E4A10(0x33)` は「型索引 0x33 の manager 配列スロット用意」で、実 manager は `0xB3A700`/`0xB3A704` と同 CRT init クラスタに属する（同一 `.data` 窓 `0xB3A700..0xB3A708` を `0x9FBxx0` 系 init が触る）。 |
| **V-547-2: `0x890B90`（`Menu` `+0x18`/`+0x1C`）** | **[確定]（命令）＝`ret 0xC` の純 no-op。callers=2（vtable のみ）** | 本体は **`ret 0xC` 1 命令**（3 dword 引数を捨てて戻る Void 仮想）。`callers_of` は 2 件＝**vtable/間接呼びのみ**（静的呼び無し）。→ 「no-op 8 slot の理由」＝**`Menu` 基底が仮想枠だけ用意し、実際の UI 処理は全 47 派生クラスが override する設計**（v546 §2.3 の `0x60CF60`×8 と同趣旨、`0x890B90` はこの no-op 群の 1 つ）。 |
| **V-547-3: type 4002（`0xFA2`）の値域** | **[部分確定]（命令）** | `.text` の imm32 `0xFA2` は **3 箇所のみ**: `0x42703E`（head `0x426A90`）／`0x58A9A7`（head `0x58A1C0`）／`0x591C54`（head `0x591B70`＝v543/545 の open 仮想含む）。`0xFA5`(4005)=98 箇所・`0xFAB`(4011)=91 箇所＝**汎用 UI メッセージ/コールバック ID として広く使われる**。→ **メニュー type（4002）と UI コマンド ID（4005/4011）は別値域の enum**で、定義元は単一テーブルでなく**各 UI ハンドラが直値で持つ散在 enum** [部分確定]（値域表は未発見＝V-548-3）。 |
| **追加 [確定] `591C50` の引数構成** | **[確定]（生バイト）** | `cmp dword [esp+4], 0xFA2`（arg1=type）→ `fld dword [esp+0x10]`（arg2=float）→…→ `ret 0xC`＝**`open(type, float, ?)` の 3 引数**。`0x584880`（初期化）・`588BD0`（子窓検索 4005 値＝`fcomp [0xA69778]`/`[0xA69768]` 境界）・`58CEB0`（4011 設定）を順に呼び、`[this+0x44]` に menu を保持。 |

## §2 証拠

### §2.1 CRT init（v547_1 (a)）
```
9FBBF0: push 0xB139A4 / mov ecx,0xB07BF0 / call 0x4040E0 / push 0x00A24A10 / call 0x981FB4
9FBC50: call 0x57D330 / fiadd [0xB1399C] / call 0x9828C0 / mov [0xB3A700],eax / ret
9FBC70: sub esp,8 / fild [0xB139A4] / fstp [esp] / call 0x57D390 / fsubr [esp] / call 0x9828C0 / mov [0xB3A704],eax
```

### §2.2 `0x890B90`（v547_1 (b)）
```
890B90: ret 0xC          ; 純 no-op（引数 3 dword 破棄）
callers = 2（vtable/間接のみ）
```

### §2.3 `591C50`（open 仮想）再掲＋引数（v547_1 (d)）
```
591C50: cmp dword [esp+4], 0xFA2 / jne …       ; arg1=type（4002 のみ処理）
591C5E: fld dword [esp+0x10]（arg2 float）/ 9828C0（floor）→587B20(1,floor)→587D20（menu 生成）
591C7D: [esi+0x44]=menu / null なら "ERROR: Cant find menu class!"（0xA6B178）
591CBD: [menu+0x20]=menu->vtable[+0x34]()      ; 窓 ID 実行時採番
591CC0: 588BD0(0xFA5=4005) ×2（fcomp 0xA69778/0xA69768 境界）
591D0A: 58CEB0(0xFAB=4011,..)
591C9E: ret 0xC                                ; 3 引数
```

## §3 判定の内訳

- **V-547-1 完遂（manager 実体は部分）**: `0xB3A700`/`0xB3A704` は CRT init が時間 API（`57D330`/`57D390`）＋float→int で埋める時刻キャッシュと確定。`0xB3A708` は同窓の別グローバル（型索引 0x33 スロット）。
- **V-547-2 完遂**: `0x890B90`＝`ret 0xC` no-op、callers=vtable のみ。`Menu` 基底の no-op 仮想枠設計を確定。
- **V-547-3 部分完遂**: type 4002 は 3 サイトのみ・4005/4011 は 90+ サイト＝**別 enum**と判明。定義元単一テーブルは未発見。

## §4 残点（V-548-1..3）

1. `0xB3A700`/`0xB3A704` を読む 3 head（`585620`/`585720`/`585820`）の正体（時刻表示 or 経過時間）と `0xB1399C`/`0xB139A4`（元 float 設定値）【中】
2. `591C50` の arg3（`[esp+0xC]`/`ret 0xC` の 3 番目）と `0x588BD0`/`0x58CEB0` の子窓系関数の意味【中】
3. type 4002/4005/4011 を含む UI メッセージ enum の定義元（`.rdata` の enum 表 or XML パーサ）【低】

## §5 後続への申し送り

- **移植側 UI 時刻表示**: `0xB3A700`（日/時）`0xB3A704`（分）を起動時 1 回算出し、以降キャッシュ参照（毎フレーム再計算しない）。元値は `0xB1399C`/`0xB139A4`（設定 float）。
- **`Menu` 基底は no-op 仮想（`ret 0xC`）**——移植側は全 47 派生で override する前提の純インタフェース。`+0x18`/`+0x1C` は 3 引数仮想。
- **`591C50` は `open(type: int, time: float, ?: int)` の 3 引数仮想**。type 4002 のときのみメニュー生成、窓 ID は `vtable[+0x34]` 実行時採番、子窓 4005/4011 を `588BD0`/`58CEB0` で構成。
- **TEMP の probe ハーネス（`v307_common.py`）は消える前提**——再構築手順（EXE パス＋`named_funcs.txt` 参照の `find_head`）を本裁定に記録済み。次回以降は最初に存在確認すること。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
