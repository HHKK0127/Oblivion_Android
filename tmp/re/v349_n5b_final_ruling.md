# v349 — #121 N5b 裏取り 最終（クローズ）: NiKeyframeData::Load 読み順確定＋ファミリ vtbl map 完（5 件全部静的極限到達）

- 作成: 2026-11-10 / L1 report-only / ReverseEngineering レーン（main=作製/#121=解析受託）
- 出典: #121（main 起票・#45 生産ゲート）。中間=v348（§5/§6 追記済）→ 本文件が完遂版。probe v121_1..9 全 EXIT=0
- 帯: Data/Controller 群 head 窓（int3 終端）・vftable データ読・callers/参照 census（境界検証 v117_9 方式）・std::string deref（教訓 v347: .data 名ポインタは実行時構築）。ESM/git/docs 無接触

## 1. クローズ裁定表（5 件）

| # | 裁定 | 確定内容 |
|---|---|---|
| ① クラス群正式名 | **[確定]**（v348 §5/§6＋追補） | 11 クラス vtbl/TD map（NiTransformController 0xA79634/NiInterpController 0xA7A16C/NiSingleInterp 0xA7A08C/NiTimeController 0xA7EB24/NiControllerManager 0xA79804/NiControllerSequence 0xA79A34/NiKeyframeManager 0xA7B3FC/Bool/Float/Pos/Rot Data 0xA7C2AC/0xA7A2EC/0xA7A624/0xA7AB3C）。**追補: NiKeyframeData 実 vftable=0xA7B0D4（ctor 0x6DFB70 の vptr 書 2 サイト 0x6DF0FA/0x6DFBA6）＋キーデータ容器 A=0xA7AD5C（24 slot 内 Load 系）/B=0xA7A55C（refcount 共有・+4 で Interlocked*）**。**[確定負] 系譜**: NiKeyframeController/Data の RTTI TD 不存在・静的名前テーブル不存在・GetTypeName 返却先（0xB3D91C/DBDC/DC0/DF34/F584/DCF0/CFBC/DF08 全部）=**実行時構築 std::string**（静的 dword=ゴミ値 0x21818285 等）→ NIF 型名は静的復元不能（v347 の std::string 教訓の再確認） |
| ② キーフレーム配列レイアウト | **[確定（命令）]** | **NiKeyframeData::Load = vftable+0x1C スロット 0x6DFDE0**（stream arg=[esp+8]・base Load 0x6EBA80=jmp 0x7008A0 後・版本ゲート `[stream+0x21C]`）。読み順（命令列）: ①`call [ver+4](stream,2,&this+0xC,…)` → **u16 kind（quaternion/interp）→this+0xC**（0x6DFDFF-0F）②`0x712A90(stream)` → **u32 key count→this+0x10**（0x6DFE16-21）③`0x713620(stream,&this+0x14)` → **time float 配列→this+0x14**（0x6DFE24）④`0x6CB990(stream,&this+0x18)` → **サブ構造 Load→this+0x18**（0x6DFE2D）⑤**8 反復（this+0x38..+0x54）**: `0x712A90` で u32 count→旧配列 InterlockedDecrement→delete[0]/push1→新ポインタ格納→InterlockedIncrement（0x6DFE40-89・value 側は resize のみ＝**充填は 0x6D4300 dispatcher→0x6D42D0 fixup 路**）。要素 stride: 時間=4B（float）、値=型別（路 A elem new(0x18)+vptr 0xA7A624 の 24B レコード=Rot 系/路 B=0x20 コンテナ—v348 §6 継承）。[8]=3・[9]=… の後続 slot 群は getter/visitor 群（0x6DFA70/0x6DFEA0 等・+0x21C/+0x220 引用=stream 側フィールド） |
| ③ 0x5A3520 resolve | **[確定]**（v348 §1-③） | 委托座標は命令途中。実体 0x5A3460（UI float 補間・Tile 0xFB6/0xFB1）/0x5A3540（メニュー Main(1020) resolve・getter 0x589B70・**XML 名=gameplay_menu.xml@0xA6991C**—v347 収穫）。NIF resolver ではない |
| ④ walker 読み幅（.ui/animation） | **[確定（NiKeyframeData 本块）]** | animation 系ブロック body prefix=**② の読み順そのもの**（u16 kind→u32 count→float[count] times→sub→8×u32 counts）。型別分岐は **0x6D4300 dispatcher**（GetTypeName=vtbl+4 照合→keyframe 変換 0x6D3BD0/NiBool 系 0x6D3EB0/NiPos 系 0x6D4000 の 3 路＋attach 0x6D42D0@0xB3F584 型）。他ブロックの接頭辞は各クラス slot7 Load に帰属（一般表の必要なし＝機構クローズ）。残=std::string 型名レパ〖トリ（実行時・#82） |
| ⑤ サンプル所見 | **[確定]** | lodnif 101/101 に controller ブロックなし（文字列 census）。**main 要求=アニメ付き NIF 1 本**（②④の runtime pin・oracle 入力）。静的側は既に読み順を閉じているため、サンプルは検証用（ゲート解除必須ではない） |

**集計**: ①③⑤ [確定]／② [確定（命令）]／④ [確定（本块）＋機構]。**#121 クローズ**。新規残件: 実行時系のみ＝#82 note 移し（std::string 型名ダンプ・value fixup の実データ突合・A/B 容器の NIF 上型名）。

## 2. main（#45 N5b）への実装向け要約

- NiKeyframeData（.kf/NIF の "KeyframeData"）読み: `u16 kind; u32 n; float[n] times; <sub @0x18>; then 8× u32 counts`（値配列は**別パッチフェーズ**（0x6D42D0）で埋まる点に注意＝walker は count 部まで直読み、値は fixup 前提）
- キー レコード: 時間 4B／値 24B レコード（vptr 0xA7A624 路）or 0x20 コンテナ路。Interpolant kind enum→処理は dispatch 表 **0xB3D2F8×[kind]**（妥当性 byte 0xB3D3F4）
- コントローラ attach フラグ: data+0xC bit0=flag・bit1-2=kind（property flag [ctx+0x3C]&1,>>1&3 から packed・0x6D3B40 で双方向変換）
- 型名文字列は**静的に無い**（実行時 std::string）。名前言語表が必要な場合、#63 系ダンプに `.data 0xB3DCF0/…` の char* を追加すること（本レーンの #82 note 参照）

## 3. 状態

ai-todo: #121 note→complete・#82 note 追記。台帳 row326（V326）・bak176・round_verify。STATE (bg)。ハンドオフ索引 §7 に要約反映。supermemory 滞留（v345..v349 分）は復帰時一括。
