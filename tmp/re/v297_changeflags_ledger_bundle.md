# v297_changeflags_ledger_bundle.md — serial 279（ChangeFlags 台帳後片付け束・4件）

- 作成日: 2026-11-05 / 担当: 本セッション内実施
- 出典: tmp/re/v289_extra_space_bundle.md 残点1..4（= 台帳 row279 起票文）
- 中間物: `%TEMP%\opencode\v279_i1.py/_out`（[reg+0x50] 5種パターン全 .text）・`v279_i2.py/_out`（0x99D 実バイト検証+SLG参照フィルタ+pass2 ebx流）・`v279_i3.py/_out`（(a) 0x461CF4-0x461E00全文 / (b) host fn=0x463913 / (c) 0x465860 無上限walk / (d) 台帳API呼び出し元16関数一覧）
- 判定サマリ: **件1=部分確定（機構+帰属[推定]・書手は次束）/ 件2=確定（確定負）・件3=確定・件4=確定（前提が幽霊と判明）**

## 件3 pass2「レコード種別 8」の語義 — 【確定】（先に掲載: 件1の命名証拠を内包するため）

v289 残点③「0x461CC5 `cmp ebx,8` の種別8語義」は**種別 byte ではなく changeFlags dword の下位バイト（bl）の値**だった（命令列 `%TEMP%\opencode\v279_i2.out (c)`）:

```
0x461CAF  mov ebx,[esp+0x24]   ; レコードの flags（dword）
0x461CB3  test bl, 8           ; bit3=0 ならブロック全体をスキップ (je 0x461dec)
0x461CBC  test bl, 6           ; bit1|bit2 のいずれかが立つとスキップ (jne 0x461dec)
0x461CC5  cmp ebx, 8           ; flags 全体 == 0x00000008（bit3 のみ）なら
0x461CC8   → 0x461CCA: key=[esp+0x20] / map=[[esp+0x18]] → push 1; call 452DF0  ; **台帳 entry を force 除去**して終端 (jmp 0x461dd8)
0x461CE1  else 経路: 変換再発行ブロック ↓
```

**else 経路（bit3+他ビット）= pass2 による「フラグ整形再発行」**（0x461CF4..0x461DBA 全文実測）:

1. `and ebx, 0xfffffff7`（**bit3 除去**）→ `or ebx, 0x800`（**bit11 追加**）— bit11 は v275 の「disable ミラー」機構側のビット。v275 の「両消費者=`bit11‖bit5` 同一視」および引擎 Disable 経路 FUN_004E4690（bit11 set）と整合: **再発行時に discard 標を disable 標へ翻訳している**。
2. `test esi,esi; jns` — **bit31（符号ビット）で再発行フレームサイズが分岐**: 負（bit31 立）→ `edi=0x2c` 拡張フレーム、非負→ `edi=0x1c` 標準フレーム。`and esi, 0x7fffffff`（符号清）→ フレーム長 `sub ax(word[esp+0x30]), di`。
3. `and esi, 0x800000`（**bit23 検査**）— 立っていると `add ax,4`（+4 バイト長）。
4. 確保 `call 0x453500`（セーブバッダプール・v283 §2 の 453500 と同一）→ `45B9A0` 積込み 3 連（key/val ペア）→ `mov [eax+4],ebx`（変換後 flags）・`mov [eax],ecx`（bit31 清後 flags）・`mov [edi+0x14],0`（SLG+0x14 カーソル）。

**帰結**: bit3 = **セーブ済みレコードの「破棄 tombstone」標**（単独ビットなら entry 除去のみ・他ビットと併存なら除去標を bit11 disable 標に翻訳して再発行、台帳 entry からは消し続ける）。bit31 = **拡張フレーム（0x2c）選択の符号標**——ロード後に残る値ではないため 452D60 の清去先（0x80000008）と完全一致。v289 件1 の [推定] 命名は本件で**役割[確定]** に昇格（語彙名は依然ゼロ＝命名は確负のままと整合）。

## 件1 pending マスク [ctx+0x50] — 【部分確定】（機構確定・書手未特定・帰属[推定]）

- 全 .text パターン走査（v279_i1 (a)）: `[reg+0x50]` への **or/and（ビット演算）はゼロ**、書込形態は `mov`（即値/レジスタ）のみ → pending マスクは**ビットごとに立たず丸値で代入**される（= 直近レコードの changeFlags 写し）。
- 0x465860 dispatch 無上限 walk（v279_i3 (c)、end=0x466AC6）: [reg+0x50] 接触は 3 点のみ（読 0x46675A / ゼロ化 0x46676B / `[esp+0x50]` はスタック局所で無関係）。0x463A63 host fn の関数先頭=**0x463913**（E8 直接 caller ゼロ＝間接/例外帯経由の到達、0x463913 自体は epilogue 断片から始まる巨大 SaveLoadGame 側帯）。
- 台帳 API（452230/452D10/452D30/452D60/452DF0/452C90/453500/453530）呼び出し元関数の全数（v279_i3 (d)）: **16 関数**（0x452CE5・0x45359B・0x459AE8・0x45C79D・0x4606C1・0x46130C・0x461674・0x4636F4・0x463913・0x463A87・0x463C55・0x463EBA・0x465BF2・**0x4662C9（8 API 全部のハブ＝452D10 唯一 caller 0x466300 を内包）**・0x663299・0x66B40B）。うち [reg+0x50] に触れるのは 0x463913/0x465860（=0x4662C9 側の塊）のみ。
- 帰属[推定]: ctx=**TESSaveLoadGame 単例**（+0=Changes マップ〔v274「ChangesMap」・this=ecx=[ctx] 55E000 find と整合]、+0x50=pending マスク）。v276 の SLG 窓目録に +0x50 は未載（+0x44/48 と +0xB0..0x1BC の間＝v276 が「書込窓なし」としか検証していない区間）。
- **残点（次束①）**: 非ゼロ mov 書手の特定——残 89+48 サイトの SLG 帯所属絞り込み（呼び出し元 16 関数の無上限 walk＋`mov [reg+0x50],reg` の整列実照合）。判定強度は現状 [部分確定] で留める。

## 件2 ExtraHavok のランタイム ID 帰属（0x0E 説） — 【確定（確定負）】

- v289 残点②の前提「0x99D9xx 帯に dword-ID ストア群」は**実バイト検証で消滅**（v279_i2 (a): 0x99D9A0/D9D1/DA20/DA6B/DB31/DC55/DD5B のいずれも `C7 4X 04` 系でない——例 0x99DA6B=`00 00 00 f6 c1 80…`）。当該主張は親セッション前段の probe#1 ident バグ（+0x400 ズレ）汚染由来と裁定。
- 0x99D700 の 1 件のみ実在したが、これは **CRT 側の文字列テーブル初期化ループ**（`call 0x989E22` 確保・stride 0x28・[+4]=0/[+5]=0x0A 等——`[eax+4]` はレコードヘッダ側で BSExtraData+4 とは無関係）。CRT 初期化表（0xA283D0..0xA2F77C）からの参照もゼロ。
- 3c 表（v289 §2-2）に **0x0E ストアサイト存在ゼロ**・ExtraHavok ctor（0x41DAF0/0x41DBB0 両変種）に ID ストア無し[確定]（v289 §2-3 再確認済）。
- 裁定: **ExtraHavok は extra 型 ID 空間不参加[推定→機構旁証確定]** — Havok データは **LoadHavokData/SaveQueuedHavokData キュー帯**（v282 件2: +0x58/+0x60 pop・0x45D030/0x45D220・v271 'HavokData'@0xA3A847=SaveQueuedHavokData 断片）で専用運搬され、ChangeFlags/ExtraDataList の type-ID 台帳を通過しない。0x0E 説は **確定負**。

## 件4 0x99D9xx サイト群の個別帰属 — 【確定（件2と共通: 前提消滅でクローズ）】

上記の実バイト検証で対象そのものが非実在と確定。i1 産物の当該行は廃棄扱い（v289 の残点④記載は本束で差し替えクローズ）。

## docs 反映候補（+3 点・#20/#47 ゲートへ）

1. bit3=破棄 tombstone（pass2 で bit11 disable 標へ翻訳再発行／台帳 entry からは恒久清去）
2. bit31=拡張フレーム選択符号（再発行 0x2c/0x1c 分岐・bit23 で +4）— v289 件1 の [推定] を格上げ
3. ExtraHavok=型 ID 空間不参加・HavokData キュー専用運搬（0x0E 説確定負）

## 残点（次束候補・低）

1. pending マスク [ctx+0x50]（=SLG+0x50[推定]）の非ゼロ mov 書手（16 呼び出し元関数の無上限 walk）。
2. pass2 再発行ブロックの **bit11 追加**が 0x461CBF の「bit1|bit2 でスキップ」条件と矛盾しないケース網羅（bit1/2 の意味：v273 の A/B 群に含まれない低位ビット=形式ビット候補）。
3. 0x463913 の呼び出し経路（E8 直接ゼロの到達元＝例外処理帯/関数ポインタ表）。

## 検証（selftest 相当）

`v279_i1..i3.py` 再実行 EXIT=0。主要数値の内部整合: 呼び出し元 16 関数（(d)）↔ v289 i4/i5 の E8 カウント（452DF0=14・452D60=3・452C90=3・452CF0=1・452D10=1・452D30=3）と二重計上含め矛盾なし。0x99D 検証は対象 8 アドレスの実バイト列を全文出力（再現可能）。既存 tmp/re 成果物・docs・GT・ソースは無編集。ESM/git 非接触。
