# v348 — #121 N5b 裏取り解析束（main 委託・部分クローズ: ①③⑤ 確定／②④=残件）

- 作成: 2026-11-10 / L1 report-only / ReverseEngineering レーン（分担: main=作製／本レーン=オリジナルデータ解析）
- 出典: #121（main が起票・#45 の生産ゲート）— Ni keyframe/controller スタックの**オリジナル読み取り構造**
- 対象: `re_util.EXE`（Oblivion Steam v1.2.0416・読取のみ）。probe `v121_1..3.py/.out`（全 EXIT=0・`%TEMP%\opencode\`）。教訓適用: 命令境界検証（C-7）・int3 終端（C-10a）・RTTI TD 直読（TD=.data 0xB02000+）
- 判定語: [確定]/[部分確定]/[推定]/[未確定]/[確定負]

## 1. 裁定表

| # | 委託項目 | 裁定 | 内容 |
|---|---|---|---|
| ① | 該当クラス群の vtable/TD 正式名 | **[確定（ファミリー map 完成）]** | RTTI TD 実在 11 クラス（vtable=COL walk で 1:1 確定）: **NiTransformController** TD 0xB23E54 / vtbl **0xA79634**・**NiInterpController** 0xB23EA0/0xA7A16C・**NiSingleInterpController** 0xB23E78/0xA7A08C・**NiTimeController** 0xB14B0C/0xA7EB24・**NiControllerManager** 0xB24108/0xA79804・**NiControllerSequence** 0xB07244/0xA79A34・**NiKeyframeManager** 0xB249D4/0xA7B3FC・**NiBoolData** 0xB24DAC/0xA7C2AC・**NiFloatData** 0xB242EC/0xA7A2EC・**NiPosData** 0xB2444C/0xA7A624・**NiRotData** 0xB2460C/0xA7AB3C。全 vtbl の slot0..11 を一覧化（v121_3(iii)・Data 群で同型スロット列=clone/getter 群）。**[確定負]×2**: `'.?AVNiKeyframeController@@'`/`'.?AVNiKeyframeData@@'` は**画像に TD 不在**（直接走査・0 hit。よってこの 2 クラスは RTTI 名で引けない—vtable は他クラス経由/名文字列 0xA791F9/0xA79209 のみ） |
| ② | NiKeyframeData キーフレーム配列レイアウト（個数前置き/interpolation 種別/タイム+値 stride） | **[未確定→残件 R1]**（機序は特定） | NIF 型名文字列（0xA79025/0xA791F9/0xA79209/0xA792C9/0xA792E1・各 1 部のみ）は **.text imm32 参照 0・非 .text アラインド pointer 参照 0（1 hit=ゴミ）→ 静的な名前一覧テーブルは不成立**。すなわち loader の型解決は**実行時登録**（Gamebryo 標準の NiObjectFactory プロトタイプ登録=.init 経由）で、Load 実体は vtbl slot 経由。レイアウト確定には (a) 0xA7A624(NiPosData)/0xA7AB3C(NiRotData)/0xA7A2EC(NiFloatData)/0xA7C2AC(NiBoolData) の **Load slot 同定→stream read 列**（count→time 配列→value 配列の stride 実測）と (b) アニメ付き NIF サンプルでの突合が必要。**現 lodnif サンプル 101 ファイルに controller ブロック 0（⑤）ため静的+サンプルの二本立てが揃わない**→ main 側へサンプル要求（§4） |
| ③ | 0x5A3520 resolve 系の参照解決アルゴリズム | **[確定（同定・前提注記）]** | **0x5A3520 は命令途中**（先頭 B7 0F…・0x5A3524 の call は前 fn の末尾窓。教訓 C-7 系の再発）。実 head 2 本: **0x5A3460**（E8 callers 0・UI 経由: 0x582160(0,1)→0x57D7A0×fmul[0xA2FAA0]→+fadd [edi+0x20]→CRT 0x9828C0→Tile トライト 0x588BD0(0xFB6/0xFB1)→0x4843B0(f64 補間)=**メニュー用 float 補間/進捗経路**）と **0x5A3540**（callers=1・`push 0x3fc(=1020)→0x589B70`=名表 getter（v331 帯）で UI オブジェクト取得→slot0 仮想(+1 引数)→`[[ui]+0x68]→0x590420(0xA6991C 名)→0x5898F0→vtbl[+0x34]→cmp eax,0x3fc→0x9832E6(0xB13A74/0xB13240)`=**「Main(1020) メニュー resolve+検証」UI レゾルバ**）。→ 委託の想定（NIF controller resolver）とは**別物**＝メニュー側 resolve。NIF キーフレーム解決本体は ②R1 の vtbl Load 側にあり（0x59xxxx/0x5Axxxx の NIF consumer は NiRotData 等の getter 群＝v121_3(iii)） |
| ④ | .ui/animation 経路の walker 読み幅（ブロック先頭→body prefix 表） | **[部分確定→残件 R2]** | ③ の同定で「walker が踏むのは**型名→プロトタイプ→Load slot**」経路が確認できたが、ブロック先頭の size/prefix 表は ② の Load stream read 列からしか導出できない（**型別読み幅の実測=R2**）。ただし Data 4 クラス（Bool/Float/Pos/Rot）の vtbl 同型性（slot7 以降 getter 列一致）は**共有基底レイアウト**の傍証（[推定]注記: time 配列+value 配列の 2 層 getter パターン） |
| ⑤ | .mirror 検証用サンプル所見（lodnif controller ブロック有無 census） | **[確定]** | `tmp/re/lodnif` **101 ファイル走査 → NiKeyframe/Controller/Sequence 系文字列 0 ファイル**。= LOD NIF にアニメブロックなし。**結論: 本サンプル群では ②④ の実データ突合は不可能**（誤った既知 NIF 期待値で walk しないために重要＝main の oracle に「アニメ付き NIF」を追加提供すべき） |

**集計**: [確定]①③⑤／[部分確定]④／[未確定]②（機序特定済・残 R1/R2）。[確定負]2（NiKeyframe* の RTTI TD 不在・静的名前一覧テーブル不在）。

## 2. main 側への回答（#45 ゲートの扱い）

- **すぐ使える事実**: ① の 11 vtbl map（rust walker が名前でなく**型別オフセット**で読むなら NiPosData/NiRotData/NiFloatData getter=0x6D9F90../0x6D8F60.. 群に相当）。ただし**②未確定＝キーフレーム配列の count 前置き/stride はまだ断定不可** → 移植実装は保留（「推測で移植実装しない」指示どおり #45 N5b の該当節はブロック推奨）。
- **必要データ（#82/#63 系と併記）**: アニメ付き NIF（Actor .nif か .kf 1 本で可）。あれば (a) Load slot 同定（b) 読み幅実測（c) `tmp/nif_oracle` の dump_orig/dump_rust 突合まで一気に閉じる。
- rust 側 (`rust/crates/nif-core/src/{block,lib,walk}.rs`) に Keyframe 語あり＝**仮説実装の存在確認のみ**（本レーンは rust/ 無編集。oracle パターンは新 AGENTS §Rust 準守）。

## 3. 残件（次回）

- R1: Data 4 クラス Load slot 同定（stream read 列→count/time/value stride）— アニメ NIF 前でも**静的に可能**（next probe: vtbl slot→fn 内 `call [IAT read]`/BSStream read 列の命令窓）。
- R2: 0x59xxxx/0x5Axxxx の NIF consumer 側からブロック先頭→prefix 表（④）。
- R3: 0x5A3460/0x5A3540 の UI resolve の正式名（v331 getter 群衔接・低優先）。

## 4. 状態

#121 は **open 継続**（②④ が未確定のためクローズせず・本 v348 は中間成果物）。台帳 row 化は完遂時（v348 参照で v348+ 使用済扱い＝次 bundle は v349+）。STATE (bc)。ハンドオフ索引 §7 に追記。

## 5. 追記 §5（2026-11-10 午後・第 2 ハーフ＝R1 一部達成・v348 自己追記・probe v121_4）

- **② レイアウト実証（部分）→ ②は [部分確定] へ昇格**:
  - ctor/ミソックチェーン確定: **NiTransformController 0x6C3E50→NiSingleInterpController 0x6CE1D0→NiInterpController 0x6D04E0→NiTimeController 0x715990**（各段 `call` 後に自 vptr 書込 0xA79634/0xA7A08C/0xA7A16C）。フィールド証拠: SingleInterp **+0x3C=target 初期 0**・InterpController **`and word[+8],0xFFDF`**（flag bit5 クリア）・NiControllerSequence ctor 0x6C6550: **+0x1C=1.0f**（fld1/fst）・+8..0x18=0・+0=0xA79A34。
  - **Data クラス群の共通レイアウト [確定（命令）】**: deleting-ctor 直後ブロック 0x6D8E10 が **count=[obj+0xC]**・**interpolation kind=[obj+0x10]** を読み、`mov edx,[ecx*4+0xB3D2F8]`（**.data の関数ポインタ dispatch テーブル**・kind=インデックス）・妥当性 `byte[eax+0xB3D3F4]`（種別有効バイト配列）で呼ぶ＝**Gamebryo interp kind enum→処理関数マップ**を発見。Float/Pos/Rot の getter 群（slot7+）は 3 クラス完全同型（0x7008A0/0x700670/0x68F970 経由・引数 obj の +0x21C/+0x220 を読む=**コントローラの target オブジェクト側フィールド**）＝共通基底レイアウト [部分確定]。
  - **残（②未闭环分）**: time 配列と value 配列（curved/pre/post 3 連の存否）の読み順/count 前置きは **NiKeyframeData 本体（TD なし・vptr 未同定・Float/Bool ctor の E8 callers=0＝バーチャル経由のみ）** の Load 実体特定か、アニメ付き NIF サンプルの実データ突合が必要（＝§2 の main 要求と同一ゲート）。
- **(a) の教訓追記**: vtbl imm32 ref から出た head は **vtable slot 実装そのもの**（E8 callers 0 が正常）— 「ctor callers=0」は未発見ではなく virtual-entry 型の印。
- #121 状態: ①③⑤ [確定]／② [部分確定]（count+0xC/kind+0x10/dispatch 0xB3D2F8 発見まで）／④ [部分確定]（R2 残）。台帳 row 化は完遂時（次 bundle v349+ 予約不変・#120=v347 空き）。STATE (bd)。

## 6. 追記 §6（2026-11-10 夜・R1 決定的 dump＝probe v121_5・②をレイアウト表まで昇格）

- **0x6D3BD0 の正体 [確定(命令)]**: NiPosData ctor(0x6D39E0) 唯一 caller の実体は **キーフレーム blob→NiKeyframeData 変換＋NiTransformController attach パイプライン**:
  - ①`[ctx+0x30]` リストを 0x700010 で walk し型名 **[0xB3D91C]** 照合（`call [vtbl+4]`=GetTypeName）→ 該当 target 探索、無ければ **new(0x40)＋NiTransformController ctor 0x6C3E50** を構築し 0x6D3B40 で attach。
  - ②**new(0x44)→0x6DFB70(0,0,0)** でコンテナ構築、`[edi+0x10]=[ctx+0x40]`、**[edi+0xC] u16 に ([ctx+0x3C]&1)→bit0・(([ctx+0x3C]>>1)&3)<<1→bit1-2** packed（=interp kind 2bit＋フラグ）。
  - ③target の `[vtbl+0x80](0)` 返り **esi+0x2C = 元 keyframe data blob**、そこから **0x6D3AA0 が 3 出力**: `countA=word[blob+0xA]`・`ptrA=dword[blob+0x14]`・`mode=byte[blob+0x1D]`（第 2 配列: countB=word[blob+0xC]・0x6D2990/0x6D3AF0/0x6D3830/0x6E1AC0 一式）。
  - ④要素コンテナ: 路 A=**new(0x20)+0x6DA160(0)**、路 B elem=**new(0x18)**→base 0x7005D0→vptr 0xA7A624・+[8]/[0xC]/[0x10]=0・byte[0x14]=0 の **24B レコード**。
  - → **blob レイアウト [確定(命令)]: +0xA u16 keys countA / +0xC u16 keys countB / +0x14 ptrA / +0x1D byte kind**。0x6D39E0 は厳密には ctor でなく **(count,A,B) 3 引数の SetData 系（ret 0xC）**（v121_4 の「ctor」表記は本節で差し替え＝旧併記）。
- **Data 要素比較形（0x6D8D70・NiRotData）[確定]**: `{+8 dword, +0xC dword, +0x14 byte}` 一致比較＝**要素側レイアウト**（§5 の +0xC count/+0x10 kind はコンテナ側 0x6D8E10 測定—両者は対象違いとして併記）。
- **②の残 1 段**: NIF stream からの読み順確定は **callers(0x6D3BD0)=0x6D4379（1 件・唯一）** の親窓起点で可能の見込み（次ハーフ第 1 標的）。lodnif=0 制約は不変だが静的側は残り 1 段。
- #121 更新: ② **[部分確定→レイアウト表確定]**（上記＋0x6D3AA0 getter 命令証跡）。①③④⑤ 不変。STATE (bf)。

## 7. 結語（2026-11-10・v121_8/9 完）

- **②④ は Load 実体（vftable 0xA7B0D4・slot7=0x6DFDE0）の読み順確定まで到達**（`u16 kind→u32 count→times→sub→8×count+refcount スワップ、値は fixup 0x6D42D0`）。型名ポインタ（0xB3DCF0 他）は**全て実行時 std::string**（静的 deref 不能・v347 教訓再確認）。
- **完遂版= v349_n5b_final_ruling.md（台帳 row326/V326）へ**。#121 クローズ・実行時残件 2 件のみ #82 移し（std::string 型名ダンプ／fixup 実データ突合）。STATE (bg)。
