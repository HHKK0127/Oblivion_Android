# v298_ini_registration_bundle.md — serial 281（INI登録/パサ低位ビット束・5件）

- 作成日: 2026-11-05 / 担当: 本セッション内実施（連続投入なし・inline 継続）
- 出典: tmp/re/v297_changeflags_ledger_bundle.md 残点1..3 ＋ tmp/re/v291_getter_thunk_scope.md 残点1..2 ＋ tmp/re/v268_opcode_gt_names15.md 件6（TOFT）= 台帳 row281 起票文
- 中間物: `%TEMP%\opencode\v281_i1..i8.py/_out`（wide walk・stub 実start再走査・jump table 展開・setter 形状確認）
- 判定サマリ: **(3)=確定 / (4)=確定 / (1)=部分確定（候補否定3・残2に絞込）/ (2)=部分確定（bit2=v286既存に接続・bit1は静的到達止）/ (5)=ドラフト1件追加（#20/#47 ゲート）**

## 件(3) 0x4055E0 の機構 — 【確定】（lazy-init getter 説は棄却）

命令実測（0x4055E0..0x40564B・SEH handler 0x9AB578=v288 確定ファミリ）:

```
arg=[esp+8]→esi; if(esi && [esi+4]) { ecx=0xB35574; 4524C0(0xB35574, [esi+4]) }   ; 単例マップ 0xB35574 からキー除去（4524C0=v289 の map 除去関数）
if(esi && byte[esi]==0x53 'S') 401F20(esi)                                       ; 'S'-タグノードの文字列 delete
ret
```

- **裁定: 0x4055E0 = ノード破棄関数**（{+0=文字列/タグ, +4=マップキー} ノードを 0xB35574 単例マップから unlink + delete）。getter（返り値のない thiscall 破棄形・`jmp` 経由で ecx だけ引き継ぐ 0xA1B3A0/3B0 thunk=**atexit クリーンアップスタブ**）と確定し、v291 残点②「bss lazy-init getter 説」は**確定負**。
- 関連帯の帰属も確定: 0xA1B3A0→[0xB35AB0]・0xA1B3B0→[0xB35AB8] の各1参照=0x9E21F0/0x9E2210 スタブのみ。0x9E21F0 は `push 0; push 0xA42D88='iLevCreaLevelDifferenceMax'; ecx=0xB35AB0; call 0x419A70（SEH 0x9AB578 の INI スカラ登録・0x4040E0 の int 版）; push 0xA1B3A0; call 0x981FB4(atexit)` ＝**v291 と同一機構のスカラー登録变種**（0x419A70/0x4040E0=登録・0x65C620/0x4524C0=解除対応）。

## 件(4) 12+ スタブの CRT 初期化テーブル対応スロット — 【確定】

- 実 start 再走査（v281_i2 (b')・CC 境界）: 0x9E21F0/0x9E2210/0x9E2230/0x9E2290/…/0x9E26F0 の**16+ スタブが全て CRT .initterm テーブル（走査域 0xA283D0..0xA2F77C・v282 確定）に連続スロット 0xA28E38..0xA28E7C（stride 4）で実在**。
- **順序保証の裁定**: 登録はテーブル順＝**起動時にアドレス昇順で 1 回実行**（walk ループ 0x981CC5..0x981CD4・v282）。解除は 0x981FB4=atexit ラッパ（v288）経由で **MSVC atexit の LIFO＝登録逆順**。順序は二重に保証されており、ゲーム実行時の再登録経路は存在しない（v291 件3 の結論をスロットレベルで補強）。

## 件(1) pending マスク [ctx+0x50] の非ゼロ書手 — 【部分確定】（候補絞り込み＋3件否定）

- .text 全域の整列 fn walk 0x440000..0x470000（関数 904・v281_i1 (c)）で `[reg+0x50]` 接触の**オブジェクト書込**は次以外は全て `[esp+0x50]`=スタック局所と判別:
  1. **0x446DE8（fn 0x446D7B）=否定**: 関数先頭で `memset(obj,0,0x68)`（0x981630 実体=memset 命令確認済み）→ リセット側。pending の初値/消去。
  2. **0x44976A（fn 0x4492E0）=否定**: intrusive list head ポップ（[esi+0x4c]/[esi+0x50] ペア・next 辿り・delete）。フィールド実体=リスト head（`Form '%s' (%08X) of type %s in file '%s' was not freed.`@fn内文字列）。**SLG の +0x50 とは無関係の別オブジェクト**。
  3. **0x466E80/0x4670FC（fn 0x466E80・呼び主 0x46AD06..0x46B1DF）=否定**: 関数先頭 `memset(edi,0,0x68)` → jump table（0x4671D0・case 0..0x19=26 種）で `RTDynamicCast(esi→TD 0xB05xxx)` を**型別スロット [edi+4k] へ格納する binder**（case 0x14→[edi+0x50]、隣接 case が +0x4c/+0x54/+0x58）。0x68=104B オブジェクトで SLG(0x1C4B) とサイズ不整合＝**別クラス**。pending との一致は disp 一致のみ。
  4. 残存候補=**0x5C071E/0x5D335A 帯**（0x5C08A0/0x5C08BB のゼロ化と同クラス＝SLG ±0x800 参照あり）と**0x59519D（code=0xc→[ecx+0x50] の仮想プロパティセッタ）**。0x595190 は 5 case ディスパッチ（0xc→+0x50/1→+0x28/2→+0x2c/3→+0x30/4→+0x34/5→…）で、読み側 0x59516C（同一 object の +0x40/+0x50 fill=0xFFFFFFFF シリーズ）と対。呼び主が仮想ディスパッチ専用（E8 ゼロ・slot 検索要）のため**実体帰属未了**。
- 機構側既知制約（v297）: 値は丸代入のみ・pass2/purge/dispatch の読取側 3 点とゼロ化 2 点で閉じる。書手は「0x5951xx プロパティ経由[推定]」まで。**残点①: slot 0x595190 の vtable 帰属＋0x5C07xx/0x5D33xx 帯の object 同定**（絞り込み済み・1 ラウンドで決着可能な範囲に縮小）。

## 件(2) pass2 0x461CBF `test bl,6`（bit1|bit2）— 【部分確定】

- **bit2=古典フォーマット選択と接続確定**: v286 件1 で purge 側 sub<0x5B の分岐=`flags bit2/bit4`（bit2 立=FUN_00459950 formID 比較／bit4=u16 読取）。pass2 の再発行ブロックが bit2 を弾くのは**古典形式レコードを新フレームへ変換しない**ためと整合（変換対象=12B フレーム系のみ）。
- bit1: 全画像で `bl` 低位検査に bit1 単独窓は pass2 の 0x461CBC のみ（and/or 窓ゼロ・v271/v273 の A/B マスク=bit7/12..30 に非包含）。**静的証拠のみでは語彙名・生成側到達不能**＝[未確定]（GECK/セーブ実データ域＝禁止域横断）。skip 条件の機能は「bit1|bit2=変換不適格レコード」まで確定。

## 件(5) TOFT 異常値訂正ドラフト — 【1件追加・#20/#47 ゲート】

- before（GT/台帳側 v265 §件6 記載の**引用元**tmp/re/v268_opcode_gt_names15.md 件6・docs 側該当: RE_EXE の TOFT +8 言及箇所）: `+8=0xA3BA94（異常値）`
- after（案）: `TOFT 項 +8 = 0x00A3BC94 ＝隣接静的 NiTPointerMap<uint,TESForm*>（this=0xB0613C・vtable 0xA3BC94）のスロット境界共有、異常値でない（v265 の 0xA3BA94 は 1 桁誤り）`
- 適用先・手順: v296 §手順雛形（bak129〜）に統合、docs 本文は編集せず本 § が正本ドラフト。判定=[確定]（v265 実測）。

## docs 反映候補（+3 点・#20/#47）

1. 0xA1B3A0 帯=INI スカラ登録（0x419A70=0x4040E0 の int 版）+ atexit クリーンアップ、0x4055E0=ノード破棄（0xB35574 unlink）
2. 12+ スタブの .initterm スロット 0xA28E38..0xA28E7C 連続＝登録順序昇順/解除 LIFO の二重保証
3. pass2 `test bl,6`=古典形式(bit2=v286)変換スキップゲート（bit1 は語義未確定と明記）

## 残点（次束候補・低）

1. 0x595190 プロパティセッタの vtable 帰属＋0x5C071E/0x5D335A 書手の object 同定（**pending マスク最終決着・絞り込み済**）
2. pass2 bit1 の生成側（セーブ書側 0x453xxx 帯で 0x2 マスクの store 有無の網羅再確認）
3. 0xB35574 単例マップの所有者名（0x4055E0 母体のクラス同定・RTTI needle 帯 0xB05xxx TD 26 種の一覧化）

## 検証（selftest 相当）

`v281_i1..i8.py` 再実行 EXIT=0（i8 のみ pipe 実行・EXIT=0）。内部整合: (3) の 0x419A70=SEH 0x9AB578 ファミリ＝v288(v64-4) の EH 5件分類と一致・0x981630=memset＝CRT ライブラリ帯・(4) スロット連続は v282 walk ループの開始 0xA283D0 後方 0x5A8 バイト地点＝テーブル有効域内。件1 の否定 3 件はいずれも**関数先頭/memset サイズ/jump table 隣接 case**という構造的否定（アドレス一致の誤帰属を命令証拠で潰した）。既存 tmp/re・docs・GT・ソース無編集、ESM/git 非接触。
