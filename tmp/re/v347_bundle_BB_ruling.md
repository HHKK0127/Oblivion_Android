# v347 — 次束BB（#120）: v346 §4 静的残件 2 件＋付随 1 件 — 静的到達限界の確定と待記宣言クローズ

- 作成: 2026-11-10 / L1 report-only / 静的のみ（ReverseEngineering レーン）
- 出典: #120（v346 §4 残件 N2'/N4'＋付随・#119/#118 クローズ時起票）
- 対象 EXE: Oblivion Steam v1.2.0416（ImageBase 0x400000・読取のみ）
- probe: `v120_1..3.py/.out`（全 EXIT=0・`%TEMP%\opencode\`）。教訓適用: 命令境界検証（C-7）・head 規則（E9/EB 拡張）・int3 終端窓（C-10a）・RTTI TD 最終審
- 判定語: [確定]/[部分確定]/[推定]/[未確定]/[確定負]

## 1. 裁定表

| # | 残件 | 裁定 | 内容 |
|---|---|---|---|
| N2' | 0x6B6B90 の owner クラス（0x6AE720→caller 0x66BFA0 の 1 段遡上＋callers 0 heads の取得元） | **[部分確定]＋静的経路クローズ（待記宣言・#82）** | 遡上結果: 0x66BFA0 ∈ head **0x66BB80**（window: `…call 0x5E06C0; test al; jne`→`push 0; mov ecx,esi; call 0x6AE720`）。**0x66BB80 自身は raw-E8 callers=0・非.text データ参照 0**。heads 0x6AB570/0x6AB720/0x6AED70（0x6B6B90 の直接 callers）も **E9 jmp=0・push/mov-imm=0・データ参照=0**（0x6AED70 はデコード空=命令境界不整合＝head ではない可能性、窓注記）。→ **この 0x66xxxx/0x6Axxxx 群は静的_pointer（E8/E9/imm32/表）で一切到達不能**＝実行時コールバック登録（Ni/BS 側 fn ポインタ）と宣告、owner クラス=#82 runtime dump 待記。0xB16178 ゲート byte: 静的 0・C605/8805 書込形 **0**（disp32 出現 29=読取/他形混在、書き手も計算経由）＝同一 runtime 家族。 |
| N4' | 0x65C620 本体帯 217 サイト/160 owner heads のマネージャ型帰属・命名 | **[部分確定]＋待記宣言クローズ（意味形は昇格・名前は静的不能）** | 高密度 cluster 6 heads（0x449D40/449E20/449FC0/44A470/44A630/44AC20）を全面 census: **E8 callers=0・E9=0・push/mov-imm=0・アラインド データ参照=0（全セクション）×6 全て**＝v119_2 の owner-head 集計は**実行時登録された teardown 関数群**で静的親なし（窓視測で各ブロック末尾 `pop;pop;ret 4` の field 毎個別 fn 形式を確認・0x449E20+0xC0 窓はデコードずれ混入のため断片は命令単位で再注記）。**セマンティクス昇格 [確定(命令)]**: teardown 形=`call 0x9832E6(0, name_g, cat_g, 0, eax)`→`push eax; lea ecx,[esi+0x34/0x3c…]; call 0x65C620` ＝ **名前付きレジストリの lookup→value 除去**（0x9832E6=5 引数 lookup/register 候補・CRT 帯だが engine utility の可能性・追跡外）。name/cat スロット（0xB05434/0xB0541C/0xB0532C/0xB02F9C）は**全て実行時構築 std::string グローバル**（静的 dword=0xAA3E80 共通=未構築プレースホルダ、cstr ゴミ）→ **名前もクラス名も静的復元不能＝待記（#82/B13 系）**。|
| 付随 | 0x4FBB60 の残 2 callers（0x52B0C0・0x531290）の vtbl+0xD4 concrete 先 | **[部分確定]（Script 経路は再確認 [確定]）** | 再検証: head 0x4FBDB0 → .rdata 参照 **0xA49E10→vftable 0xA49DA4 '.?AVScript@@' slot idx 27**・**[0xA49DA4+0xD4]=0x4129A0**（`mov eax,0xa2f7ec; ret`＝静的 zero 槽のアドレス返却＝**空 slot/カウンタの getter**）＝v346 N2a 追認 ✓。残 2: head 0x52B0C0（callers=1・ecx 窓内 origin なし・データ参照 0）／head 0x531290（callers=3・ecx=ebx/edi 連鎖のみ・データ参照 0・**[0xB36530] 静的=0xFFFFF2E0＝vptr 不是**（オブジェクト先頭でなくカウンタ/センチネル類））→ この 2 経路の this 型=実行時依存、**待記宣言で最終クローズ**（#82）。 |

**集計**: 3 件とも **[部分確定]＋静的経路尽きた分の待記宣言クローズ**（主観点を誤爆させる [未確定] は残さない—残りは全て「実行時 pointer/型」類で #82/B13 ゲート同一）。[確定負]1（この群に静的呼出・静的名前解決経路は存在しない）。新規残件なし（#82 note に統合、次束を要さない）。

## 2. probe 一覧（再現）

- v120_1: (1) 0x66BFA0 host=head 0x66BB80（callers 0・データ参照 0）・(2) 3 heads の E9/push/mov/data 参照全 0・(3) 0xB16178（静的 0・C605/8805=0・disp32 29）
- v120_2: head callers の ecx 起源→[abs] 静的解決＋data-refs→vtable base walk（**0x4FBDB0→0xA49DA4 Script・[+0xD4]=0x4129A0 再確認**。0x449E20/0x449FC0/0x52B0C0/0x531290=refs 全 0）。[0xB36530]=0xFFFFF2E0 非 vptr
- v120_3: (1) 0x531290 callers 3 の ecx=ebx/edi/None（起源連鎖内・グローバル直積なし）(2) cluster 6 heads 全参照形 census 全 0＋teardown 窓 0x449EE0-0x449F33（`call 0x9832E6` 5push→`push eax; lea ecx,[esi+0x34/0x3c]; call 0x65C620; ret 4` 連続ブロック実測）
- 付随確認: 文字列 deref（0xB05434 等→0xAA3E80 共通=std::string 未構築・0xA6991C='Data\Menus\Options\gameplay_menu.xml'＝**v348③ の 0x5A3540=ゲームプレイメニュー resolve の semantic 裏付け増加**・[v348 §5 反映済扱い・本文件に注記）

## 3. 帯遵守の自己宣言

深: 0x66BB80/0x6ABxxx/0x6AED70 命令窓（head 判定＋12 命令）・0x449xxx cluster 窓・0x4129A0 head・0x9832E6 は呼び形のみ（本体未読）。機械: 全 .text E8/E9/imm32 参照 census・全セクション アラインド dword 参照走査・RTTI COL/TD。未踏宣言: 0x5E06C0 本体・Ni 帯 0x66xxxx 内部・CRT 0x9832E6 本体（低価値）。#121（stop 済・サンプルゲート）/BG 帯（0x57xxxx/0x586-0x5DD/0x676xxx）と非交差。ESM/git/docs 無接触。

## 4. クロージング

ai-todo: #120 note→complete（新規残件なし→**起票要らず**・#82 へ run-time 待記 1 note 統合）。台帳 row325（V325）・bak175・round_verify。STATE (be)。memory 登録。supermemory 滞留分（v345/v346/v348/本件）は復帰時一括追登録。
