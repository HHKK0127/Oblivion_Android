# v286_tail_payload_bundle.md — serial 277（tail ペイロード末尾束・2件）

- 作成日: 2026-11-05 / 担当: 本セッション内実施（subagent 起動が 4 連続 upstream error のため手段変更。AGENTS.md ループガード「3回同じ結果なら手段を変える」準拠）
- 出典: tmp/re/v283_slg_model_tail_bundle.md 残点①②。EXE=Oblivion.exe v1.2.0416 Steam（capstone 5.0.7・セクション個別 VA 換算）
- 中間物: `%TEMP%\opencode\v277_i1..i6.py` / `v277_i1_out.txt`（本セッション規約。成果物・既存 tmp/re は v286 のみ新規）
- 注記: 台帳 row277 の予定名 v284 は並行セッションの v61 relabel 束が先占のため v286 へ変更（row 側も round98 で更新済）
- 判定サマリ: **件1=確定（境界の意味）、件2=確定（メカニズム）＋bit 命名のみ推定**

## 件1 tail byte+3（0x5B ゲート）の語義 — **[確定]**

byte+3 は **extra データの型 ID byte**（BSExtraData+4。v281 の「型 ID=BSExtraData+4 byte」と同一空間）。境界値 0x5B の正体は **ExtraSound** の型 ID:

| 型 ID | vft | RTTI TD | 生成 ctor |
|---|---|---|---|
| 0x5A | 0xA35A4C | `.?AVExtraHasNoRumors@@` | 0x42AB50（C6 40 04 5A）|
| **0x5B** | **0xA35A58** | **`.?AVExtraSound@@`** | **0x42AB70**（C6 40 04 5B・+8=0・+C=参照引数）|

（v282 の「ACTR/FLOR/WEAP の ExtraSound 0x5B add/remove」と相互整合。0x5A/0x5B は v281 付録 56 空間表に未載のため追加確定＝付録差分 +2）

tail 歩行2箇所が同一の境界を比較している（命令実測）:

- LoadGame pass2 `FUN_00461680`: `cmp byte[esp+0x26],0x30`（type=='0'）→ `0x461811 cmp byte[esp+0x27],0x5b; jb 0x4618da`。通過時（=**sub≥0x5B**）のみ change flags dword の bit26（0x4000000）/bit25（0x2000000）で分岐し、**12B ペイロード（3 dword）**を [SLG+0x14] カーソルから消費: 1st dword→`FUN_00459990`（キー読取）、2/3rd（movsx byte×2）を伴う除去レコード（0x461824..0x461863）
- PurgeSavedDataForCell `FUN_00459AF0`: `0x459BFA cmp cl,0x30` → `0x459C03 cmp byte[esp+0x1f],0x5b; jae 0x459c82`。**sub<0x5B** は flags bit2/bit4 分岐で `FUN_00459950`（formID 比較）または u16 読取→459990、**sub≥0x5B** は 12B フレーム→459950 比較後 `push 1; call FUN_00452DF0`（エントリ除去）

**帰結**: byte+3 は「除去対象 extra の型 ID」。`≥0x5B`（ExtraSound 以降の新增フォーム系）だけが 12B 拡張フレーム形式（key+2 param）を運び、`<0x5B` は古典形式（flags/u16）。型 ID と frame 形式が 1 進数の境界で融合している点に注意（0x5A までの全 classic extra は 4/6B、0x5B 以上は 12B）。

## 件2 FUN_00452D60 の 0x80000008 変換先 — **[確定]（メカニズム）/ bit 名のみ[推定]**

命令実測（0x452D60..0x452DE9）:

1. `test (([0xB33B00]TES SaveLoadGame+0x18)>>0xC),1` → セット済みなら即 return 0（ロード中ゲート。v281 の +0x18 状態語と同一 dword）
2. `FUN_0055E000(this, key=arg1) → entry`（this=caller の form/extras マップ、key=[ebx+0xC]）
3. entry 存在時: `cmp [entry+4],0` **refcount=0 のときのみ** `not ecx; and [entry],ecx` ＝ **entry+0 の flags dword から bit3|bit31 をクリア**（ecx=arg2=0x80000008）
4. flags ゼロ化時: `4524C0(this,key)` でマップ除去→`[entry+4]` をプール 0xB02020 へ free→`call 0x401F20`(delete)
5. 呼び主（0x461DCD）: LoadGame pass2 で**除去レコードを処理し終えた extra エントリ**に対して実行（直前に 45B9A0 追加/検索・4524C0 経路の対）

**裁定**: 0x80000008 の変換先は **[form+8] ではない**（452D60 本体は form を一切参照せず＝v283 の[推定]仮説を**棄却**）。実体=**extra エントリ自身の flags dword（entry+0）から bit3|bit31 を落とす後始末**（refcount 0 なら完全除去、ゼロでなければフラグのみ維持）。兄弟 `FUN_00452DF0`（0x459C4B から `push 1` 付きで呼ばれる）=同ゲート・同キー構造の**設定側**（or/挿入系）。bit3/bit31 の正式名は EXE 内文字列証拠ゼロ＝**[推定]（bit31=変更/一時フラグ群、bit3=delete-pending 候補）**、命名は次束（v283 残点① TESNPC switch 帯と併せて）へ。

## 残点（次束候補・低）
1. bit3|bit31 の命名裏取り（ChangeFlags 生成側 0x4535A0 帯との対照で語源候補を絞る）
2. extra 56 空間付録へ 0x5A/0x5B を追記した統合表の更新（v281 付録の差分反映）
3. 0x5B 以上 extra の 12B フレームの param2 個数上限（3 dword 固定の再確認は 1 サンプルのみ）

（件1/件2 ともに**既存成果物 v283 の該当[未確定]を解消**。docs 反映候选には本束 3 点—0x5B=ExtraSound 境界・452D60=extra flags クリア除去・5A/5B 追記—を追加、適用は承認待ち）
