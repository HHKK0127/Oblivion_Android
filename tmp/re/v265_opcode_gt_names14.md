# v265_opcode_gt_names14.md — opcode/GT 殘点 第14束（7件）

- 作成日: 2026-11-03 / 担当: TODO serial 253 (V253・低) spawning / 分析: サブエージェント
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ: `tmp/re/v257_opcode_gt_names13.md` §9 新規残点7件（すべて低）:
  1. PlayerCharacter の vt[0x164] オーバーライド（`FUN_0065D670`・byte 引数形）と Actor 側 getter（`FUN_004D8370`）署名衝突の実運用経路
  2. formType レジストリ 0xB05E04 の index 0x12 以降を含む 4CC 名全列挙（69 項完全版）
  3. 変更リスト 0xB0614C の保存時 consumer 不在（別機構可能性・save 経路 slot19/20 接続含む）と `FUN_004492E0` mgr の RTTI
  4. HUDInfoMenu +0x2C/+0x4C 接続書手・キー名表 0xB03E1C 全長
  5. 0xDC struct の [process+0x17C] 初期 link 書込点と `FUN_004706E0` 再索引の意味
  6. vt[0xE8] 残り 6 窓の引数実体と WEAP/ARMO/STAT 実装
  7. MGEF flags 全 32 ビットの GECK 名（uesp 通路）
- 継承確定事項（v257）: formType レジストリ 0xB05E04＝69 項×stride 0xC（{name ptr, 4CC, 序号}）・`FUN_0046AF70`=4CC→formType・0xB35468=`GameSettingCollection`・vt[0x90]=`FUN_0046C640` 変更フォームリスト 0xB0614C 登録・[0xB3B33C]=HUDInfoMenu（vtable 0xA6BDF4）・vt[0x164] Actor 側=`FUN_004D8370`（返り値=無 vtable 0xDC struct・`FUN_00473EB0` ctor）・ALCH スタブ=`AlchemyItem/IngredientItem` vt[0xE8]（0x4129C0）・BOOK bit0=「Scroll」照合済／MGEF bit24 名=未確認
- 契約: 分析専任（docs/GT/TODO/plan/session-memory/cache・ソース編集禁止・既存 tmp/re 上書き禁止・訂正はドラフトのみ）
- 手法: capstone 5.0.7（v172_common.py 経由・VA→raw はセクション個別換算）＋ MSVC RTTI（v227_lib.py）＋ 既存 v257 中間物再利用（`%TEMP%\opencode\v257_*_out.txt`）
- selftest: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）
- 中間物: `%TEMP%\opencode\v265_*.py`（v257 系再利用優先）
- 注: 本ファイルは v257 §9「新規残点（次束候補）」1..7 の逐次解消記録（1 項目完了ごとに即追記）

## 1. 【低1】PC vt[0x164]=FUN_0065D670（byte 引数形）と Actor 側 getter 署名衝突の実運用経路 — **[確定]**（衝突は**誤帰属による見かけ上の変化**と裁定・v257 §5.1/§5.4 を訂正）

### 1.1 slot 実値の再実測（衝突の発生源を特定）

- vtable slot の**raw 格納値**（`v265_1_out.txt`）: **Actor 0xA6E074+0x164 = 0x4D8370**（no-arg getter・継承確定通り）・**Creature 0xA710F4+0x164 = 0x4D8370**（上書きなし＝基底実装そのまま・cache vtable_index の primary 3 者 239 slot 同一形式で確認）・**PlayerCharacter 0xA73A0C+0x164 = 0x65D720**（**FUN_0065D670 ではない**）。
- v257 §5.1「PC +0x164=FUN_0065D670（byte 引数形・署名衝突）」の帰属は **func_of() 最近ラベル誤帰属**（`0x65D720` は named_funcs 未収載の別関数で、直前ラベル FUN_0065D670 に丸められた）。**FUN_0065D670 の imm32 refs = 0 件**＝どの vtable にも格納されていない＝そもそも仮想スロット 0x164 の実体でない（`v265_1b_out.txt`）。

### 1.2 PC の真の vt[0x164] オーバーライド = `FUN_0065D720`（署名互換ラッパー）［確定］

- 全文逐語（`v265_1b_out.txt`）:
  ```
  0x65D720: eax=[this+0x5DC]; if (eax) return eax           ; PC 一次 anim struct
  0x65D72A: if (byte[this+0x588] != 0) jmp FUN_004D8370     ; 再入ガード（交換中）→ Actor 実装へ
  0x65D732: eax=[this+0x5CC]; if (eax) return eax           ; PC 二次 anim struct
  0x65D73C: jmp 0x4D8370                                    ; ★ E9 tail-call（Actor 側 getter・同署名）
  0x65D741: ret
  ```
- E9 tail jump の相対先頭 = **0x65D73C 実在確認済み**（`v265_1c_out.txt` 末尾「E9 jmp @ 0x65D73C」）。
- 返り値の 0xDC struct 2 本の割当窓（`v265_1c/1d_out.txt`）:
  - **[PC+0x5DC]**（一次）= `FUN_00668D00`（0xDC ctor `FUN_00473EB0` E8 呼び 4 件のうち 1・@0x669276 ctor→**@0x669281 `mov [ebx+0x5DC], eax`**・2 書込 @0x66961A）。
  - **[PC+0x5CC]**（二次）= `FUN_00667BE0`（＝**PC vtable +0x14C の上書き**＝Actor Init 系仮想の PC 版・@0x667C54 `push 0xDC; call new`→@0x667E51 ctor→**@0x667E5A `mov [ebp+0x5CC], eax`**）。@0x667E73/E83 で `[ebp+0x588]` を 1→呼び出し（`FUN_00659F30`＝Actor 側 +0x14C）→旧値復元＝**+0x588 は struct 交換中の再入ガード byte**（ラッパーの 2 分岐の意味と完全整合）。
- ⇒ **裁定: PlayerCharacter は Actor と同じ「no-arg・返り値 0xDC ptr」契約のオーバーライドで、PC 固有 2 代を優先して Actor 実装に委譲するラッパー。署名衝突は存在しない（v257 の誤読訂正）。**

### 1.3 byte 引数の `FUN_0065D670` の実運用（別サブシステム・非仮想）［確定］

- 本文（`v265_1b_out.txt` 逐語）: `bl=arg` → **`[PC+0x610]=bl` 常時書込**；arg==0 のときのみ ①`FUN_0045A500(this=[0xB33B00])` 述語（`GetTickCount()==[0xB33398+0x10]` なら `+0x18 bit0`、 иначе `+0x18 bit12`＝タイマ/ポーズ系述語）が**偽**なら → ②`FurnitureManager(0xB3BD00).FUN_006765F0(1, [0xB3BAD0], 0)` → ③`[PC+0x608]=0`・`[0xB3BAD4]=0`・`[0xB3BAD0]=0`。**ret 4**。
- 呼び手 = **E8 直接 3 件だけ**（全件 `ecx=[0xB333C4]`＝PlayerCharacter 全局を実引数なし this で）:
  - `FUN_004B8A40` @0x4B90D2（push 0＝着席解除系・直前 `[ecx+0x608]=0`／直後 `[PC+0x200]=0`）と @0x4B9194（push 1＝解除抑止Set・直後 `fild [0xB36768]`）
  - `FUN_0060F250` @0x60F8AD（push 1・ゲート `eax==[0xB333C4]` && `FUN_0065DA50()`＝`[PC+0x608]!=0`（着席中述語））
- ⇒ **家具着席フラグ setter（`[PC+0x610]`＝着席無効化 byte＋着席解除時の FurnitureManager 後始末）**。slot 0x164 とは無関係で、v257 の「衝突」は 1.1 のラベル誤帰属のみが発生源。

### 1.4 真の vt[0x164] ディスパッチ窓の引数実体（全窓 no-arg 確認）［確定］

- byte 直撃＋窓照合（`v265_1d_out.txt`・vtable load 必須形）: 検出窓は**全件「push 無し・call 後 eax（0xDC ptr）を使用」**の getter 形。代表例:
  - `FUN_00407AE0`（main frame loop・@0x409828/@0x40A22F）: `ecx=[0xB333B4]`（**着席/更新中の actor ptr・NULL 可**・書込@0x407BA4／ゼロ化@0x4E2B54・[推定]）の vt[0x164]() を test eax で使用。
  - `FUN_004729F0`（@0x472A51/@0x472A70）: `call` 後 **`[eax+0x98]→[+0x7C]`** 読＝0xDC ctor 初期化フィールド群と自己整合（v257 §5.2 の +0x98/+0x7C と一致）。
  - `FUN_004ABE50`（@0x4AC3B3/@0x4AC3C3）: 2 オブジェクトの返り値から浮動比較（`fstp/fcomp`）＝anim state 同期検査。
  - `FUN_00654F10` @0x654F1C（v257_9 既載）: `edi->vt[0x164]()` no-arg＋`test ebp,ebp`。
  - 唯一の push 窓 @0x40F84D（`push 0x104; push esi`）は esi の vtable が Actor/PC/Creature 以外の別クラス（v256 系 UI/TLS オブジェクト帯）で、**本 slot の契約違反ではない**（[推定]）。
- 補強: 家具 band の PC 系関数（FUN_0065D750/65D790/65E900/65E960/65FDA0/660130/664F70 ほか）は **`FUN_004D8370` を E8 直叩き**（29 件・`v265_1c_out.txt`）＝運用は「仮想窓（generic）＋ Actor getter 直接呼び（PC/家具 band）」の二重経路。

### 1.5 裁定まとめ（移植注記）

- **v257 §5.4「PC オーバーライド衝突」残点は解消**。GT 反映は §8 (B-1) ドラフト（訂正 1 件: v257 §5.1 の slot 帰属）。
- 新規確定 VA: `FUN_0065D720`（PC vt[0x164] ラッパー）・**[PC+0x5DC]=PC 一次 0xDC anim struct**（`FUN_00668D00` 生成）・**[PC+0x5CC]=二次**（`FUN_00667BE0` 生成＝PC vt[0x14C]）・**[PC+0x588]=交換ガード byte**・**[PC+0x610]/[+0x608]=着席フラグ/着席先 ptr**・`FUN_0065D670`=着席解除 setter（非仮想・全局 `[0xB3BAD0]/[0xB3BAD4]`=FurnitureManager 着席スロット）・`FUN_0065DA50`=`[PC+0x608]!=0` 着席中述語。

証跡: `v265_1_out.txt`（slot raw 値・FUN_0065D670 全文・imm32 0 件・E8 3 窓・窓 262 件生走査）、`v265_1b_out.txt`（FUN_0065D720 全文・呼び窓逐語・FUN_0045A500）、`v265_1c_out.txt`（緩和窓・FUN_00667BE0/668D00・0xDC ctor 呼び 4 点・FUN_004D8370 E8 29 件・E9 tail jump 位置）、`v265_1d_out.txt`（Creature slot・真窓 list・+0x5DC/5CC/588 書込手・0xB333B4 書込点）。

### 1.6 本項目内残点

- `[0xB333B4]` の所属正確化（main loop 側「更新中 actor」ptr の割当元型）— [推定] のまま。低。
- `FUN_006765F0`（FurnitureManager 0xB3BD00 の窓）の slot 意味と `[0xB3BAD0/4]` ペアの正確な型。低。

## 2. formType レジストリ 69 項 4CC 名全列挙（完全版表） — **[確定]**

### 2.1 デコード手順と有効範囲

- エントリ `i` = `0xB05E04 + 12i`、`{+0: name ptr（.rdata 4文字列）, +4: 4CC dword（画像では全 0＝base ctor `FUN_0046C460` が実行時合成）, +8: 序号 dword}`。name ptr を全項 deref（`v265_2_out.txt`）。
- **有効範囲 = idx 0..0x44（69 項）で確定**：`FUN_0046AF70` の走査窓 `[0xB05E08, 0xB06144)`（4CC スロットのみ・stride 12）は末尾 0xB06138＝idx 0x44 を含み、idx 69（0xB06140 先）は後続グローバル（変更リスト 0xB0614C の static オブジェクト）に接続＝**レジストリ終端**。
  - ⇒ **v257 §2.5「index 0x44..0x68 の 4CC 名が enum 公式範囲外の意味（SVR 系/未使用）」は誤想定の棄却**（実在は idx 0x44=TOFT の 1 項だけ。0x45 以上のスロットなし）。
- name 文字列は `.rdata 0xA3BA68..0xA3BC88` の **8 バイト間隔 69 本**（降順配置：idx 0='NONE' が末尾 0xA3BC88、idx 0x44='TOFT' が先頭 0xA3BA68）・unique 69 ✓。
- +8 序号：idx 0..67 で **i+1（1..68）** の静的列。**例外 idx 0x44(TOFT) の +8=0x00A3BA94**（name 領域の 'LSCR' スロット直後パディング＝空文字位置を指す・意味未解）→ **[未確定・低]**。

### 2.2 全 69 項表（VA=0xB05E04+12i・name ptr・4CC・seq）

| idx | 10進 | 4CC 名（name ptr deref） | seq(+8) | 備考（照合済みの機構注） |
|---|---|---|---|---|
| 0x00 | 0 | NONE | 1 | 未割当既定（base ctor の +4 初期値 0 と一致） |
| 0x01 | 1 | TES4 | 2 | CreateForm 特別窓 0x44DE35（プラグインヘッダ・bit0 強制＋vt[0x90](1)） |
| 0x02 | 2 | GRUP | 3 | `[0xB05E20]`＝GRUP スロット 4CC（FUN_00450F10 特判） |
| 0x03 | 3 | GMST | 4 | 特別窓 0x44E326＝form 生成なし・GameSettingCollection 0xB35468 へ EDID マージ |
| 0x04 | 4 | GLOB | 5 | |
| 0x05 | 5 | CLAS | 6 | |
| 0x06 | 6 | FACT | 7 | |
| 0x07 | 7 | HAIR | 8 | |
| 0x08 | 8 | EYES | 9 | |
| 0x09 | 9 | RACE | 10 | |
| 0x0A | 10 | SOUN | 11 | |
| 0x0B | 11 | SKIL | 12 | 特別窓 0x44E512 |
| 0x0C | 12 | **MGEF** | 13 | 特別窓 0x44E367（code キーで map 0xB33508 マージ・v256/v257 確定と自己整合） |
| 0x0D | 13 | SCPT | 14 | |
| 0x0E | 14 | LTEX | 15 | |
| 0x0F | 15 | ENCH | 16 | |
| 0x10 | 16 | SPEL | 17 | |
| 0x11 | 17 | BSGN | 18 | |
| 0x12 | 18 | ACTI | 19 | |
| 0x13 | 19 | APPA | 20 | |
| 0x14 | 20 | **ARMO** | 21 | vt[0xE8] 実装 `0x69D990`（項目6） |
| 0x15 | 21 | BOOK | 22 | DATA bit0='Scroll' 照合済（v257 §7.1）・vt[0xE8]=no-op |
| 0x16 | 22 | CLOT | 23 | |
| 0x17 | 23 | CONT | 24 | |
| 0x18 | 24 | DOOR | 25 | HUD 末尾列 TESObjectDOOR（v257 §4.3） |
| 0x19 | 25 | INGR | 26 | vt[0xE8]=0x4129C0（ENIT flags bit0 反転セッタ） |
| 0x1A | 26 | LIGH | 27 | 型 ctor byte[+4]=0x1A ハードコード（v256 §5.6 と一致✓） |
| 0x1B | 27 | MISC | 28 | |
| 0x1C | 28 | **STAT** | 29 | vt[0xE8] 実装 `0x4A8F90`（項目6） |
| 0x1D | 29 | GRAS | 30 | |
| 0x1E | 30 | TREE | 31 | |
| 0x1F | 31 | FLOR | 32 | |
| 0x20 | 32 | FURN | 33 | |
| 0x21 | 33 | **WEAP** | 34 | vt[0xE8] 実装 `0x46C2F0`（項目6） |
| 0x22 | 34 | AMMO | 35 | |
| 0x23 | 35 | NPC_ | 36 | |
| 0x24 | 36 | CREA | 37 | |
| 0x25 | 37 | LVLC | 38 | |
| 0x26 | 38 | SLGM | 39 | |
| 0x27 | 39 | KEYM | 40 | |
| 0x28 | 40 | ALCH | 41 | vt[0xE8]=0x4129C0（本束 §6 の ALCH/INGR スタブ） |
| 0x29 | 41 | SBSP | 42 | |
| 0x2A | 42 | SGST | 43 | |
| 0x2B | 43 | LVLI | 44 | |
| 0x2C | 44 | SNDG | 45 | |
| 0x2D | 45 | WTHR | 46 | |
| 0x2E | 46 | CLMT | 47 | |
| 0x2F | 47 | REGN | 48 | |
| 0x30 | 48 | CELL | 49 | 特別窓 0x44DEF0 |
| 0x31 | 49 | REFR | 50 | 特別窓 0x44E001（REFR/ACHR/ACRE 共用） |
| 0x32 | 50 | ACHR | 51 | vt[0x164] getter の gate formType（FUN_004D8370: 0x32/0x33 のみ） |
| 0x33 | 51 | ACRE | 52 | 同上 |
| 0x34 | 52 | PGRD | 53 | 特別窓 0x44E2A5 |
| 0x35 | 53 | WRLD | 54 | 特別窓 0x44E401 |
| 0x36 | 54 | LAND | 55 | 特別窓 0x44E237 |
| 0x37 | 55 | TLOD | 56 | default 窓（別 loader） |
| 0x38 | 56 | ROAD | 57 | 特別窓 0x44DE83 |
| 0x39 | 57 | DIAL | 58 | 特別窓 0x44E474 |
| 0x3A | 58 | INFO | 59 | 特別窓 0x44E4C9 |
| 0x3B | 59 | QUST | 60 | |
| 0x3C | 60 | IDLE | 61 | 特別窓 0x44E556 |
| 0x3D | 61 | PACK | 62 | |
| 0x3E | 62 | CSTY | 63 | |
| 0x3F | 63 | LSCR | 64 | |
| 0x40 | 64 | LVSP | 65 | default 窓（別 loader） |
| 0x41 | 65 | ANIO | 66 | |
| 0x42 | 66 | WATR | 67 | |
| 0x43 | 67 | EFSH | 68 | CreateForm `cmp eax,0x42` で case 対象化 |
| 0x44 | 68 | **TOFT** | 0x00A3BA94（序号欄の異常値・[未確定]） | formType 0x44。CreateForm 上限 0x43 外＝form 生成なし（Topic グループ断片＝GRUP sub のみ＝整合） |

### 2.3 自己整合検査（既知確定事項との突合）

- v257 §2.1 の CreateForm dispatch 全ケース注釈（TES4/GMST/SKIL/MGEF/CELL/REFR/ACHR/ACRE/PGRD/WRLD/LAND/ROAD/DIAL/INFO/IDLE の特別窓 VA）と本表 index が**全件一致**（`byte[formType−1+0x44E6DC]` 索引の formType 値＝本表 idx）。
- v256 §5.6 LIGH=0x1A・本束 §6 ALCH/INGR 型・vt[0x164] gate 0x32/0x33・MGEF=0x0C — いずれも本表で独立再現✓。
- ⇒ **formType 正表は 0x00..0x43 公式 68 種＋TOFT(0x44) の計 69 スロット**（Oblivion では TOFT はレコードだが TESForm ではない）。4CC→formType 変換 `FUN_0046AF70` の返却域 = 0..68、不発時 0（NONE）。

証跡: `v265_2_out.txt`（69+オーバーラン 3 項の name/4CCimg/seq 実測・scan 終端直後の 16B・name ptr 領域 min/max/unique・ft 0x00..0x44 名一覧）。

## 3. 変更リスト 0xB0614C の保存時 consumer 不在・FUN_004492E0 mgr RTTI — **[確定]**（consumer 不在は画像全域で再確認・mgr=**TES** シングルトンと RTTI 実デコード）

### 3.1 リスト object の所属クラス — **`NiTLargeArray<TESForm*>`**［確定・RTTI 実デコード］

- `decode_rtti(0xA37AAC)` = **`.?AV?$NiTLargeArray@PAVTESForm@@@@`**（`v265_3_out.txt`）。
- 実レイアウト（画像＋書込窓から）: `[0xB0614C]`=vtable 0xA37AAC・`[0xB06150]`=data ptr・**`[0xB06154]`=capacity（静的 0x20＝32 要素）**・`[0xB06158]`=size（実走査上限）・`[0xB0615C]`=used。
- **生成窓**（`v265_3_out.txt`・CRT init 帯）: @0x9DBF50 `xor ecx,ecx; 0x20*4 バイト new(0x401F00) → mov [0xB06150],eax; push 0xA18500; call 0x981FB4(atexit)` ＝**初期 capacity 32 の静的 NiTLargeArray**（`FUN_009DBE90` は別配列 0xB07BF0/table 0xB05DC0 の生成で本件と無関係・[確定]）。

### 3.2 consumer 不在の再検証（画像全域・4 パターン）

- imm32 refs（`v265_3_out.txt`）: **0xB0614C=2 件**（`FUN_0046C640` @0x46C682 の push_back ecx 設定・`FUN_00A18500` static 初期化のみ）。**0xB06150=6 件**（clear 449303／remove 46B2E9／登録走査 46C66B・46C6AB／生成 9DBF71／atexit dtor A18501）。**0xB06158=10 件**（すべて 4492E0/46B2C0/46C640 の保守命令内）。**0xB0615C=3 件**（同上）。**0xB06154=0 件**。
- vtable 0xA37AAC メソッド（+0x10 erase `FUN_00449160`／+0x14/0x18/0x1C 各 no-op・+0x20=`FUN_0068F970`）の **E8 直接呼びは erase 0 件**（+0x20 の 81 件は no-op 共有スタブで list と無関係・`v265_3b_out.txt`）。
- ⇒ **retail 実行可能画像内でリスト中身「列挙」は 0 件**。読取は①登録時線形去重（`FUN_0046C640`）・②解除時除去（`FUN_0046B2C0`）・③リセット時全消し（`FUN_004492E0`）＝**保守専用**［確定］。
- 保存本体 `FUN_00465130`（TESSaveLoadGame 系・v241 確定）の窓確認: 呼び手引数は **name 文字列（'autosave' 0xA3AAD4 等）＋数値**であり、0xB0614C/その data ptr を渡す経路は皆無（`v265_3c_out.txt`）。**save 経路 slot19/20（GetSaveSize/SaveGame・v244 確定）との接続点は存在しない＝[確定]**。
- 裁定: **「保存時 consumer 不在」は機構の欠落ではなく設計**。変更リスト＝登録側（vt[0x90]）のみ retail に組み込まれたエディタ系 bookkeeping（「変更フォーム追跡」の読手＝slot8/11 SaveEditedForm 経路は v241/v244 で retail dead code 確定済み＝読手側コード自体が TESCS/GECK 側［推定・外部知見との整合］）。retail での有効な機能は「再ロード時の dirty クリア（FUN_004492E0）＋ formID 解除時の除去（FUN_0046B2C0）」に限定。

### 3.3 `FUN_004492E0` の mgr = **`TES` ゲームシングルトン**［確定・RTTI 実デコード］

- `this` 実測: 両 callers（`FUN_004466F0` @0x44687A・`FUN_00464440` @0x4464E9）とも **`mov ecx, [0xB33A98]; call FUN_004492E0`**（`v265_3b_out.txt`）。
- `FUN_004466F0` = mgr の **dtor/reload**（@0x44671B `mov [esi], 0xA37470`＝破棄時暫定 vtable→末尾 @0x446923 `mov [0xB33A98], ebx(=0)`）— **vtable 0xA37470 の COL デコード = `.?AVTES@@`**（`v265_3e_out.txt`・vtable slot+0=0x43F280、直後バイトは 'TES:CreateTextu…' 書式文字列＝vtable 終端後 .rdata）。
- 整合: `FUN_004492E0` 内 `[mgr+0xCD4]=1`（reload フラグ・`FUN_00447D00` が同フラグ read-modify）／`[mgr+0x64]/[+0x68]`=loaded file link リスト頭尾（v257 §3.2「loaded file リスト [mgr+0x64]」と一致）／`[mgr+0x104]`+21×0x60B 配列＝TES 保持データ／`FUN_004491D0`（dtor 内）=`[mgr+0xCD C]` ptr 解放系。
- ⇒ **v257 §3.2 の「DataHandler 系」呼称は差し替え可**: mgr（esi+0xCD4 の owner）= **`TES`（RTTI `.?AVTES@@`・インスタンス ptr グローバル `[0xB33A98]`）**。0xB33A98 への非ゼロ store は .text に存在せず（全 store 形走査で 0x446923 のゼロ化のみ・`v265_3d/3e_out.txt`）＝生成は CRT/atexit 帯の外側（推定: main 起動器）— **[未確定・低]**。

証跡: `v265_3_out.txt`（list object 画像値・vtable 0xA37AAC RTTI/slots・0xB0615C/54 refs・FUN_009DBE90 全文・FUN_004492E0 全文・callers this 窓）、`v265_3b_out.txt`（FUN_0044FAF0 6 窓逐語・list methods callers 0）、`v265_3c_out.txt`（FUN_00465130 callers 引数窓・[0xB33A98] store 走査・冒頭窓）、`v265_3d_out.txt`（store 全形・dtor 先頭 `mov [esi],0xA37470` 実測・'autosave'/'Test All Cells %i.ess'/'.ess' 文字列）、`v265_3e_out.txt`（RTTI '.?AVTES@@'）。

### 3.4 本項目内残点

- `[0xB33A98]`（TES*）・`[0xB33B00]`（TESSaveLoadGame* 相当）の生成書込点（CRT init 外経路）の特定。低。
- `FUN_004492E0` の 21×0x60B 配列（`FUN_0052EAF0` 初期化）の要素型・内容。低。

## 4. HUDInfoMenu +0x2C/+0x4C 接続書手・キー名表 0xB03E1C 全長 — **[確定]**（接続=汎用 message setter・表は**ロック強度名表 6 項**と判明）

### 4.1 +0x2C/+0x4C（〜+0x50）の接続書手 = **`FUN_005A4700`**（HUDInfoMenu vtable **+0x04** の message-id 分岐 setter）［確定］

- ctor `FUN_005A4660` の全文実測（`v265_4b_out.txt`）: @0x5A4676-0x5A469A が `xor eax,eax` 後 **[esi+0x28..+0x58] を順に 0 クリア**（v257 §4.1 の「0 クリア群」の逐語確定・この関数内に実接続の書込は無し）。
- 実接続窓 = **`FUN_005A4700`(this=ecx) `ret 8` 形 switch**（`v265_4e_out.txt` 逐語、arg1=id / arg2=interface ptr）:

  | id | 書込先 | | id | 書込先 |
  |---|---|---|---|---|
  | 1 | `[this+0x28]` | | 7 | `[this+0x40]` |
  | **2** | **`[this+0x2C]`** | | 8 | `[this+0x44]` |
  | 3 | `[this+0x30]` | | 9 | `[this+0x48]` |
  | 4 | `[this+0x34]` | | **0xA** | **`[this+0x4C]`** |
  | 5 | `[this+0x38]` | | 0xB | `[this+0x50]` |
  | 6 | `[this+0x3C]` | | （default） | 書込なし（`ret 8`） |

- ⇒ **+0x2C は id=2、+0x4C は id=0xA の interface 接続**で埋まる（タイル 0xFDE 書込用インターフェース）。読み手は v257 §4.3 の HUD 末尾列（`[esi+0x2c]`@0x5A4FFB・`ebp=[esi+0x4C]` ゲート @0x5A4FA8 帯）。発火側は UIMenu 系 message 配線（vtable+0x04 汎用ディスパッチ経由・送信元特定は [推定・低]）。
- 補助検査: `FUN_005A46B0`（thiscall 述語）が `[+0x28..+0x4C]` の 10 連 non-null を一括検査＝**「interface 接続完了」判定述語**（接続が外部 message で行われる設計の裏付け・`v265_4c_out.txt`）。
- vtable 0xA6BDF4 は 15 slot（unique 8: 0x5A4810 dtor / **0x5A4700 setter** / 0x60CF60 / 0x890B90 / **0x5A5540**（+0x2C slot・tick カウンタ `+[0x58]++ %0x3C`→タイル 0xFB2 読→`FUN_005A4980` 再描画）/ 0x6FE080 / 0x5A4800（定数 0x3ED 返却）/ 0x725DA0）。

### 4.2 「キー名表 0xB03E1C」の実体 = **ロック強度名表・全 6 項**［確定・名称訂正含む］

- 表の型: 各 ent は `std::string object` への ptr（読手 7 窓すべて `mov eax,[idx*4+0xB03E1C]` → `test` → `mov eax,[eax]` ＝ string data ptr deref → `%s`/UI 文字列化・`v265_4b_out.txt`）。
- **全長 = 6**（`FUN_004299E0` の返り値域 0..5 と完全一致: 閾値 5 本 → tier 6・`v265_4e_out.txt`）。画像値: `[0..5]=0xB33888/90/98/A0/A8/B0`（6 個の静的 std::string・画像では全 0）、`[6]=0xAA3E80` は隣接別データ（読み手は 0..5 にクランプ済み＝表外）。
- **初期化書手 = CRT 帯 `FUN_009D9FA0`**（@0x9DA7F1..0x9DA896・`push 既定値; push INIキー名; mov ecx,静的string; call INIGetString; push dtor thunk(0xA1793x); call atexit` の 6 連＋閾値 5 連が raw 逐語で確定・`v265_4h_out.txt`）:

  | idx | 静的 string | INI キー | 閾値 global | INI キー（既定値） |
  |---|---|---|---|---|
  | 0 | 'Very Easy' | `sLockLevelNameVeryEasy` | 0xB338B8 | `iLockLevelMaxVeryEasy`（7） |
  | 1 | 'Easy' | `sLockLevelNameEasy` | 0xB338C0 | `iLockLevelMaxEasy`（20） |
  | 2 | 'Average' | `sLockLevelNameAverage` | 0xB338C8 | `iLockLevelMaxAverage`（40） |
  | 3 | 'Hard' | `sLockLevelNameHard` | 0xB338D0 | `iLockLevelMaxHard`（80） |
  | 4 | 'Very Hard' | `sLockLevelNameVeryHard` | 0xB338D8 | `iLockLevelMaxVeryHard`（99） |
  | 5 | 'Impossible' | `sLockLevelNameImpossible` | （>99 すべて） | — |

- 変換の意味確定: `FUN_004299E0(v)` = 難易度 int の段階化（`≤7→0 / ≤20→1 / ≤40→2 / ≤80→3 / ≤99→4 / else 5`）。引数 v は **ロック難易度値**（0..100）:
  - `FUN_00429990` 逐語（`v265_4g/4h_out.txt`）: `byte[form+0]` 基準難易度＋`[+8] bit2` 時に `player Security（FUN_005E1FD0）× [0xB33880]`（INI `fLeveledLockMult`）を加算 int 化・100 クランプ。`FUN_0042ACD0`＝同式別窓口（tier 算出関数群）。
  - HUD DOOR 列: `FUN_004D7780(ref)`＝ExtraData 0x31（`FUN_0041E690`）→ 無時 0x32（`FUN_0041E6B0`・[entry+0xC]）→ key/lock form 経由 → **末尾 `jmp FUN_00429990`（tail・@0x4D77C1）＝返り値は難易度 int**。⇒ **v257 §4.3/§8 の「キー formID→キー名表」表現は「ロック強度名表」に訂正**（タイル 0xFAF の float(keyID) 側とは別値・`[entry+0xC]` の formID/ptr 別は次残点）。
- 使用側全集（`FUN_004299E0` callers 14 件・`v265_4e_out.txt`）: HUD DOOR（0x5A4F1D）・`FUN_005A08E0`・`FUN_005AF440`×5・`FUN_005B11A0`・`FUN_00413BE0`・`FUN_00429A80`×2・`FUN_006A3550`×2 ＝**ロック難易度ラベルの UI 横断利用**。

証跡: `v265_4_out.txt`（0xB3B33C 参照集合・表 8 項 dump・FUN_004299E0 全文・閾値画像 0）、`v265_4b_out.txt`（7 参照窓逐語）、`v265_4c_out.txt`（FUN_005A46B0 述語全文）、`v265_4d/4e_out.txt`（FUN_005A4700 全 case 表・callers 一覧・0xB33888..D8 参照先）、`v265_4g/4h_out.txt`（INI キー名 6+5・既定値 7/20/40/80/99・fLeveledLockMult・FUN_00429990/42ACD0 逐語）。

### 4.3 本項目内残点

- vt[0x04] message setter の発火経路（UIMenu message ルータ側の id 定義）。低。
- `FUN_004D7780` 経路 `[entry+0xC]`（0x31/0x32 extra payload）の formID か ptr か最終確定。低。

## 5. 0xDC struct の [process+0x17C] 初期 link 書込点と FUN_004706E0 再索引の意味 — **[確定]**（link は setter 2 段経由と特定・再索引は 7 論理→5 物理 slot 写像）

### 5.1 初期 link の Exact 書込点 = **`FUN_0064FFF0` @0x650017**（HighProcess 側の唯一 setter）［確定］

- Actor Init `FUN_004E3490` の逐語連鎖（`v265_5_out.txt`）:
  ```
  0x4E3813: push 0xDC; call FUN_00401F00(new) → eax
  0x4E3835: call FUN_00473EB0(0xDC ctor)       ; ecx=eax
  0x4E383E: push eax; mov ecx,ebp; call FUN_004D83B0   ; ★ link 依頼
  ```
  （第 2 new 窓 @0x4E3AE1/ctor @0x4E3AFE→**@0x4E3B15 同一 `FUN_004D83B0`**＝再生成分岐。`FUN_004D83B0` の E8 callers は `FUN_004E3490`×3 のみ・@0x4E3506/@0x4E384C/@0x4E3B15）
- **`FUN_004D83B0`（this=Actor・arg=struct・ret 4）**（`v265_5_out.txt`）: `this->vt[0x188]()` 述語で `vt[0x44]/vt[0x48]`(flag 0x2000000) クリア/設定後、**`ecx=[this+0x58]`(process) が非 NULL かつ `process->vt[0x08]() <= 1`**（process タイプ 0/1 ゲート）なら **@0x4D83F7-8 `push struct; call FUN_0064FFF0`**。
- **`FUN_0064FFF0`（ecx=process・arg=新 ptr・ret 4）** 全文（`v265_5c_out.txt`）: `esi=[process+0x17C]` 旧値読み；`!old`→そのまま @0x650017 `mov [edi+0x17C], ebx` 書込；`old==new`→何もしない；Otherwise `old->FUN_00475B60()`（旧 struct reset）→ `FUN_00401F20(old)`（free）→ @0x650017 store。**E8 callers = `FUN_004D83B0`@0x4D83F8 と `FUN_004E0F80`@0x4E123D の 2 件のみ**＝初期 link の実体はこの setter 1 個。
- disp32=0x17C の store 全件走査（`v265_5b_out.txt`・前回 typo 修正後 11 件）: process 帯 4 件＝**0x650017（setter・上記）**／0x64B51B（`FUN_0064B400` の HighProcess クリーンアップ・ebx=0 ゼロ＋[+0x34]/[+0xC0]/[+0xCC]/[+0x114] クリア群）／0x64B6B6+0x64B6BC（`FUN_0064B650` の process 間 swap：`[edi+0x17C]=[esi+0x17C]`→`[esi+0x17C]=0`→非 NULL 側へ push 5/reset）／0x6520FB（`FUN_00651F70`＝64FFF0 と同型の free+store・process 移動系）。残り 7 件（0x69-0x7F 帯）は別クラスの +0x17C フィールドで本件無関係。
- ⇒ **v257 §5.4「4D83B0 後部窓未展開」解消**: 「Actor Init→FUN_004D83B0（process ゲート付き中継）→FUN_0064FFF0@0x650017」の 2 段が唯一の初期 link 経路。

### 5.2 `FUN_004706E0` 再索引の構造確定（論理 7ch → 物理 5 slot・alias 5→0/6→3）

- 本文逐語（`v265_5_out.txt`）:
  ```
  0x4706E0: i=[esp+4]; j=i−5; if(j==0) slot=0; elif(−−j==0 i.e. i==6) slot=3; else slot=i;
            return [this+0xA0+slot*4]; ret 4
  ```
- 物理配列境界: 0xDC ctor は `+0x98` と `+0xB4` をゼロ、array = **+0xA0/+0xA4/+0xA8/+0xAC/+0xB0 の 5 slot**（v257 §5.2 初期化リストと自己整合）＝**論理 index 0..6 を物理 0..4 に畳む getter**。alias 先は slot 0 と slot 3。
- callers 実測 52 件（`v265_5_out.txt`/`v265_5b_out.txt` 窓）: 定数引数は **0（最多数）/1/2/3** のみ確認（家具 band・`FUN_00650030`/`6545E0`/`654F10`/`655590`/`655BF0` が push 0、sequence band `FUN_005F4F00`(3)/`5F4FD0`(0)/`5F48D0`(3)・`FUN_005FCAB0`×6・`FUN_004E4080`(0)）。論理 5/6 の alias 経路は定数 call せず（loop/レジスタ引数側、`FUN_005FCAB0` は this->vt[0x198] 利用の sequence 更新大函）。
- ⇒ **裁定**: 「slot 意味表」は EXE 内に文字列証拠なし＝**[未確定] のまま**（v257 §5.4 の残点継続）。確定した機構事実: **5 物理 slot・7 論理 index・alias 5→0・6→3**（移植時は index→slot 変換関数として写像表のみ必要）。

証跡: `v265_5_out.txt`（Actor Init link 窓逐語・FUN_004D83B0 前半・FUN_004706E0 全文・callers 一覧・swap 窓）、`v265_5b_out.txt`（store 全件再走査 11 件・第 2 new 窓・引数 push 実体）、`v265_5c_out.txt`（FUN_0064FFF0 全文・clear/移動窓・FUN_005FCAB0 呼び窓・FUN_00472EA0/4711F0 逐語）。

### 5.3 本項目内残点

- 論理 index 0..6 の意味名（anim priority チャネル表）— 文字列証拠なし。低。
- `FUN_00475B60`（setter が旧 struct に対し呼ぶ reset/free 前処理）の意味細部。低。

## 6. vt[0xE8] 残り 6 窓の引数実体と WEAP/ARMO/STAT 実装 — **[確定]（帰属は v257 §6.3 に 2 件の訂正必要）**

### 6.1 primary vtable +0xE8 の直接実測（数え上げ含む）

- cache vtable_index primary 行の slot 数（`v265_6f_out.txt`）: **TESForm=55（+0xE8 は表外）／TESObject=69・TESBoundObject/BOOK/LIGH/MISC/WEAP/ARMO/STAT/AMMO/KEY/Furniture=72（+0xE8 は表内）／AlchemyItem/IngredientItem/MagicItemObject=72・EffectSetting/MagicItemForm=55（+0xE8 表外）**。
- +0xE8 実測値（`v265_6c_out.txt`・表内クラスのみ有効）:
  - **AlchemyItem/IngredientItem = `0x4129C0`**（v257 §6.1 の ENIT flags bit0 反転 bool セッタ・`ret 4`）✓ 再確認。
  - **MagicItemObject = 0x982D85**（`push [0xBAA5EC]→call 0x98BEEB…` の **__purecall 形 thunk**）＝基底で**純仮想**（VS6 は純仮想 slot を purecall で埋める）⇒ ALCH/INGR はこれのオーバーライド＝系譜整合 ✓［確定］。
  - **BOOK/LIGH/MISC/WEAP/ARMO/STAT/AMMO/KEY/Furniture/TESObject/TESBoundObject = すべて `0x68F970`**（raw `C2 04 00`＝純 `ret 4` no-op・`v265_6e_out.txt`）＝**上書き無し（基底 MagicItemObject 純仮想に対する空実装群）**。
  - TESObjectCONT = 0x6FE080（`xor al,al; ret 4`＝引数無視 false 返却の実装）。
- ⇒ **訂正①（v257 §6.3）**: 「WEAP=0x46C2F0・ARMO=0x69D990・STAT=0x4A8F90」は**読み位置誤り**（base ずれ）。実測は上記の通り 3 型とも `0x68F970` no-op。3 地址の実際の帰属は `v265_6b_out.txt` の imm32 全格納先で確定:
  - `0x46C2F0`（=`jmp 0x46B170`）= TESForm 族**全 primary の slot+0x18**（TESForm/WEAP/ARMO/…40 vtable に同一 store・実体 0x46B170 は byte[form+4]−0xD の formType switch jump table = 型別共通ディスパッチ既定）。
  - `0x69D990`（`xor al,al; ret`）= 多数 vtable に散る**no-arg false スタブ**（格納 slot 多种）。
  - `0x4A8F90` = **TESAmmo primary +0x50（slot20=SaveGame）のオーバーライド**（中身: `call FUN_0046B5E0`(TESForm::SaveGame)+`FUN_00470420(&[this+0x64])`＝v241/v244 の slot19/20 系譜と直結）。
- ⇒ **訂正②（v257 §6.3「slot+0xE8 は TESForm 族の共通仮想スロット」）**: +0xE8 は**MagicItem 族（MagicItemObject→AlchemyItem/IngredientItem/…）のスロット**で TESForm 基底 vtable（55 slot）には存在しない。「フォーム種別ごとの +0x7C 系 flag accessor」ではなく「**MagicItem（ポーション/食材ランタイム obj）の ENIT flags bit0 accessor**（ALCH/INGR のみ実装・既定は純仮想 or no-op）」。

### 6.2 残り 6 ディスパッチ窓の引数実体と帰属（線形同期ズレを関数先頭逐語で再確定）

| 窓 | host | レシーバ | 引数実体 | 裁定 |
|---|---|---|---|---|
| `FUN_005E12B0`@0x5E12BE | actor 側 thunk（[actor+0x58] gate・ecx=actor を push して process に委譲） | **process 族**（Base/Low/MiddleLowProcess +0xE8=`0x4943A0` `xor eax,eax;ret 4`／High/MiddleHighProcess +0xE8=`0x64B350`） | **push ecx = actor form ptr**（1 個） | **別系統確定**。`0x64B350` は `actor->vt[0x154]()` を呼び、返り値 0 なら `[process+0x15C]=0`、非 0 は `[process+0x15C]` キャッシュ＋`push 0` の継続（actor 状態述語キャッシュ系）。callers 32 件（0x5BE-0x5C 帯） |
| `FUN_0060D780`@0x60D7DB | 冒頭実測（`v265_6c_out.txt`）: `[esi+8]` obj・`[esp+8]` flags gate（bit0x20000/0x10000 分岐・'log' 形式窓 0xB03E00/0xB02F9C） | esi=process 系（唯一 caller @0x643824・FUN_00643810帯）の [+8] obj | **無 push（no-arg 形）** | offset 数値一致の**別クラス窓**［確定（ALCH 経路ではない）］／レシーバ型 [推定] |
| `FUN_0061CE40`@0x61D0EB | 冒頭 `[[esi+0x3C]]->vt[0x25C]()` ゲート（`v265_6c_out.txt`） | レシーバ = [esp+0x20] の obj | **`push ecx; fstp [esp]` = float 値**（+ 退避 ecx） | **別系統（float 引数）確定**。E8 callers 0（仮想ディスパッチのみ） |
| `FUN_006516D0`@0x651718 | process 帯（`FUN_009832E6` ロガー呼びあり） | **[edi+0xC0]**（HighProcess サブオブジェクト・`FUN_0064B400` の [+0x34]/[+0xC0]/[+0xCC] クリア群と対応） | 無 push（no-arg）／ゲート `esi & 0x80000` | offset 一致の別系統［確定（ALCH 経路でない）］・レシーバ型 [推定] |
| `FUN_00773920`@0x77393C | host の破棄経路（`mov [esi],0xA89F54` 暫定 vtable store 実測） | **[[host+8]+0x8AC]**（device/レンダラ保持 obj） | **push eax = [host+0x50] のテクスチャデータ ptr**、呼び出し後 `obj->vt[0x08](1)`＝自析棄 + フィールドゼロ | **NiDX9TextureData の release 通知窓**［確定: host RTTI = `.?AVNiDX9TextureData@@`（`v265_6c_out.txt`）］ |
| `FUN_00774370`@0x77438C | 上と同一構造（逐語一致） | 同上 | 同上 | 同上（双胞胎窓） |

- ⇒ **vt[0xE8]「8 窓」の実態**: **MagicItem 族 slot+0xE8（ALCH/INGR ENIT flags bit0 accessor）の実消費者は v257 確定済みの `FUN_00425040`×2 のみ**で、残り 6 窓は **process 族/NiDX9/float 引数系など位相同的な別仮想**（v257 §6.3「実呼び出し経路あり（8 窓）」→「2 窓＋別系統 6 窓」に訂正・GT ドラフト §8-6）。

証跡: `v265_6_out.txt`（6 窓逐語・実装 3 件の本体: WEAP th uk/ARMO false/STAT SaveGame 転送・callers 数）、`v265_6b_out.txt`（imm32 全格納先突合: 0x46C2F0=TESForm族+0x18×40・0x69D990=多 vtable false スタブ・0x4A8F90=TESAmmo+0x50・0x4129C0=ALCH/INGR+0xE8 再確認）、`v265_6c_out.txt`（primary +0xE8 実測 18 クラス・TESForm slot 数超過の発見・0xA89F54 RTTI・process 族 vtable 行一覧）、`v265_6d_out.txt`（CONT/MIF/MGEF+0xE8・0x982D85=purecall 形）、`v265_6e_out.txt`（cache slot 数: AlchemyItem=72・TESForm=55・0x68F970 raw `c20400`）、`v265_6f_out.txt`（form 側 +0xE8 in-range 確認・0x4943A0/0x64B350 頭・FUN_00425040 窓の no-arg 実測）。

### 6.3 本項目内残点

- `actor->vt[0x154]()` の返り値型と `[process+0x15C]` キャッシュの意味（家具/戦闘状態判定候補）。低。
- MagicItem 族 slot+0xE8 の純仮想名（GECK 対応: AlchemyItem「Enable/Disable」系か）。低（外部知見待ち）。

## 7. MGEF flags 全 32 ビットの GECK 名（uesp 通路） — **[一次資料照合済み・2026-11-03 通路回復]**（bit24 は一次資料に**定義無し**と確認）

### 7.1 取得経路（transport error 継続の中での成功例）

- `webfetch` en.uesp.net 直: **503**（1 件）・`index.php?action=raw`: **503**・`web.archive.org`: **503** ⇒ 直接通路は不通継続。
- **`websearch` の snippet 抽出に成功**（v257 の BOOK 時と同法）: クエリ「Oblivion MGEF flags 0x… 列挙」系 4 発で **UESP「Oblivion Mod:Mod File Format/MGEF」DATA サブレコードの Flags 表全文**（2024-07-30 版スナップショット表示）を回収（本文この会話内・第一結果に完全な表が含まれた）。

### 7.2 UESP/GECK Flags 表（32 bit 全ビュー・named 19＋projecti field＋unnamed）

| bit | mask | GECK/UESP 名 | | bit | mask | GECK/UESP 名 |
|---|---|---|---|---|---|---|
| 0 | 0x00000001 | Hostile | | 10 | 0x00000400 | FX persist |
| 1 | 0x00000002 | Recover | | 11 | 0x00000800 | Spellmaking |
| 2 | 0x00000004 | Detrimental | | 12 | 0x00001000 | Enchanting |
| 3 | 0x00000008 | Magnitude % | | 13 | 0x00002000 | No Ingredient |
| 4 | 0x00000010 | Self | | **14/15** | 0x00004000/0x00008000 | **（記載無し・unnamed）** |
| 5 | 0x00000020 | Touch | | 16 | 0x00010000 | Use weapon |
| 6 | 0x00000040 | Target | | 17 | 0x00020000 | Use armor |
| 7 | 0x00000080 | No duration | | 18 | 0x00040000 | Use creature |
| 8 | 0x00000100 | No magnitude | | 19 | 0x00080000 | Use skill |
| 9 | 0x00000200 | No area | | 20 | 0x00100000 | Use actor value |
| **21** | 0x00200000 | （unnamed） | | **22/23** | 0x00400000/0x00800000 | （unnamed） |
| **24** | **0x01000000** | **（unnamed・一次資料に定義なし）** | | 25-26 | 0x02000000/0x04000000 | projectile field: 0x02000000=Spray（「Ball if Spray, Bolt or Fog not specified」注記付き）/ 0x04000000=Bolt / 0x06000000=Fog（2bit フィールド） |
| 27 | 0x08000000 | No hit effect | | 28-31 | 0x10000000.. | （unnamed） |

- Reference Value（+0x60）対応（UESP 同ページ）: 「Use weapon/armor/creature → formid」「**Use actor value → ulong（Actor Value Indices）**」。

### 7.3 EXE 側突合（bit24 裁定の再検証・v256 の維持と限定条件の発見）

- `FUN_00413130` @0x4131B4-0x4131CC 逐語（`v265` 実測・この会話窓）: `edi=[form+0x58]`（flags）→ **`test edi, 0x1180000`**（**bit20|bit23|bit24 の 3 種ゲート**）成立時のみ → **`shr ecx,0x18; test cl,1`（bit24 単独検査）** 真なら `[form+0x60]` を **`[EffectItem+0x14]` へ copy**；直後 `cmp [form+0x98], 'SEFF'` 分岐。
- ⇒ **v256 の「bit24 = +0x60 を ActorValue 宣言ビット」裁定は EXE 実動作として有効（維持）**。ただし GECK 公称名の体系では **「Use actor value」= bit20（0x00100000）** で、**EXE が AV 転送に要求する bit24（0x01000000）は UESP/GECK 表に定義が存在しない**（unnamed）。bit20/23 が立っていても bit24 が無ければ AV copy は発火しない＝**エンジン内部用の未文書化ビット**（推定: GECK が「Use actor value」選択時に bit20 と bit24 を連動設定するか、mod 側手動）。この連動自体は一次資料なしでは確認できず → **[推定]**。
- ⇒ v257 §7.2/§9 の「MGEF bit24 名 = 未確認」は **「uesp 一次資料に bit24 の定義が存在しないことまで確認済み」** に格上げ（＝公式名は存在しない、の方向で確定扱い可。断定は「GECK 公開表に名前なし」まで、内部名が無いとは言い切らない）。

証跡: websearch snippet 4 件（Flags 表全文＋bit 範囲別抽出。直接 fetch 503×3 の記録を含む）。EXE 窓: `0x4131A0..0x4131D9` 逐語（本会話内・mask 0x1180000 と bit24 検査）。

### 7.4 本項目内残点

- GECK 側が bit24 をどう設定するか（Construction Set Executable 側の flags チェックボックス→bit マップ。TESCS.exe 分析か GECK UI 実機確認が必要）。低。

## 8. GT/plan ドラフト（反映は要承認・本体無編集）

> v257 §8 の (AG)-(AI)/(B) 系に対する本束の増分。反映前に GT 本体（docs/RE_EXE_REPORT.md 等）と行単位突合のこと。**v257 自体への訂正 2 件（D1/D2）を含む**。

### 8.1 v257 系ドラフト行への訂正・更新

- **(D1 訂正・項目1)**: v257 §5.1/§5.4 の「PC 0xA73A0C+0x164=`FUN_0065D670`（byte 引数形・Actor 側 getter と署名衝突・未解決）」→ **func_of 最近ラベル誤帰属として撤回**。slot+0x164 raw 値=**0x65D720**（no-arg ラッパー: `[PC+0x5DC]`→（`[PC+0x588]==0` 時）`[PC+0x5CC]`→E9 tail `FUN_004D8370`）。衝突なし。`FUN_0065D670` は**非仮想**の着席解除 setter（imm32 0 件・E8 3 件 all `ecx=[0xB333C4]`・`[PC+0x610]`=arg・解除時 `[0xB3BD00]` FurnitureManager `FUN_006765F0(1,[0xB3BAD0],0)`＋`[PC+0x608]`/`[0xB3BAD0/4]` クリア・`FUN_0065DA50`=`[PC+0x608]!=0`）。Creature 0xA710F4+0x164=0x4D8370（上書きなし）。PC 0xDC struct 2 代: **[PC+0x5DC]**=`FUN_00668D00`（ctor @0x669276→store @0x669281）、**[PC+0x5CC]**=`FUN_00667BE0`（=PC vt+0x14C override・@0x667E51→store @0x667E5A・`[+0x588]`=交換中ガード）。家具 band は `FUN_004D8370` を E8 直叩き 29 件。
- **(D2 訂正・項目6)**: v257 §6.3 の「slot+0xE8 は TESForm 族の共通仮想スロット（WEAP=0x46C2F0・ARMO=0x69D990・STAT=0x4A8F90）」→ **WEAP/ARMO/STAT primary +0xE8 実測=0x68F970 no-op**（BOOK/LIGH/MISC と同一）。3 地址の真の帰属: 0x46C2F0=TESForm 族 **+0x18**（thunk→0x46B170 formType switch 既定）、0x69D990=no-arg false スタブ（多 vtable 散落）、0x4A8F90=**TESAmmo +0x50（SaveGame override）**。+0xE8 系譜は **MagicItem 族**（基底 MagicItemObject+0xE8=0x982D85=purecall 形＝純仮想・ALCH/INGR のみ実装、TESForm 基底 vtable 55 slot で +0xE8 表外）。8 ディスパッチ窓は「2 窓（MagicItem 経路）＋**別系統 6 窓**」に修正（下表）。
- **(AG) 追記（項目2）**: **formType レジストリ完全 69 項表**（idx 0x00..0x44＝NONE,TES4,GRUP,GMST,GLOB,CLAS,FACT,HAIR,EYES,RACE,SOUN,SKIL,MGEF,SCPT,LTEX,ENCH,SPEL,BSGN,ACTI,APPA,ARMO,BOOK,CLOT,CONT,DOOR,INGR,LIGH,MISC,STAT,GRAS,TREE,FLOR,FURN,WEAP,AMMO,NPC_,CREA,LVLC,SLGM,KEYM,ALCH,SBSP,SGST,LVLI,SNDG,WTHR,CLMT,REGN,CELL,REFR,ACHR,ACRE,PGRD,WRLD,LAND,TLOD,ROAD,DIAL,INFO,QUST,PACK,CSTY,LSCR,LVSP,ANIO,WATR,EFSH,TOFT）。seq=1..68＋**TOFT 項のみ +8=0xA3BA94 異常値**。v257 §2.5「index 0x44..0x68 存在説」は **実在 0x44(TOFT) 1 項のみ**として棄却（TOFT は CreateForm case 上限 0x43 外＝form 生成なし・Topic 断片 4CC）。
- **(AG) 追記（項目3）**: 変更リスト object 0xB0614C の RTTI 実デコード = **`NiTLargeArray<TESForm*>`**（vtable 0xA37AAC・layout {+0 vtbl,+4 data,+8 cap=0x20,+C size,+10 used}・生成窓 @0x9DBF50 new 0x80B→[0xB06150]→atexit 0xA18500）。**保存時 consumer 不在は画像全域再検証で確定**（0xB0614C imm32=2・data ptr=6・size=10・used=3・cap=0・vtable methods E8 callers 0）。save 本体 `FUN_00465130`（slot19/20 系・v241/v244）との接続点なし［確定］＝retail では**登録専用 bookkeeping**（読手側 slot8/11=dead code と整合。GECK/TESCS 消費は [推定]）。`FUN_004492E0` の mgr の RTTI 実デコード = **`.?AVTES@@`**（this=[0xB33A98]・dtor 暫定 vtable 0xA37470・v257 §3.2「DataHandler 系」呼称は TES シングルトンに差し替え）。
- **(AJ) 追記（項目4）**: HUDInfoMenu **vtable+0x04=`FUN_005A4700` が interface 接続 setter**（message-id 1..0xB → +0x28..+0x50・**+0x2C=id 2／+0x4C=id 0xA**・ret 8）。`FUN_005A46B0`＝+0x28..+0x4C 10 連 non-null 接続完了述語。**「キー名表 0xB03E1C」はロック強度名表 6 項に訂正**（'Very Easy'/'Easy'/'Average'/'Hard'/'Very Hard'/'Impossible'・静的 std::string 0xB33888..0xB338B0・INI `sLockLevelName*`＋閾値 `iLockLevelMax*`=既定 7/20/40/80/99（0xB338B8..D8）＋`fLeveledLockMult`(0xB33880)・初期化=FUN_009D9FA0 帯）。`FUN_004299E0`=難易度 int→0..5 段階化（引数は**難易度値**：`FUN_00429990` が byte[form]＋levized 時 Security×mult・100 クランプ、`FUN_004D7780` は末尾 tail jmp 0x429990）。
- **(AK) 追記（項目5）**: 0xDC struct の **[process+0x17C] 初期 link 唯一経路 = Actor Init `FUN_004E3490`（new@0x4E3813→ctor@0x4E3835）→ `FUN_004D83B0`（@0x4E3841/4C・process 有無+`vt[0x08]()<=1` ゲート）→ `FUN_0064FFF0`@0x650017**（唯一 store setter・旧 struct を `FUN_00475B60`+free して置換・E8 callers 2）。swap/clear 窓は v257 既載の 3 件（0x64B51B/0x64B6B6-6BC/0x6520FB）。`FUN_004706E0`=**論理 index 0..6 → 物理 slot 配列 [+0xA0..+0xB0（5 個）変換（5→0・6→3・其它=i）**［機構確定］・チャネル意味名は EXE 内文字列証拠なし（未確定継続）。
- **(AI) 更新（項目7）**: **MGEF flags 全 32 ビット表取得**（UESP 2024-07-30 版・websearch snippet 通路）: named 19（bit0 Hostile/bit1 Recover/bit2 Detrimental/bit3 Magnitude %/bit4 Self/bit5 Touch/bit6 Target/bit7 No duration/bit8 No magnitude/bit9 No area/bit10 FX persist/bit11 Spellmaking/bit12 Enchanting/bit13 No Ingredient/bit16 Use weapon/bit17 Use armor/bit18 Use creature/bit19 Use skill/bit20 Use actor value）＋projectile 2bit field（bit25-26: 0x02000000 Spray/0x04000000 Bolt/0x06000000 Fog）＋bit27 No hit effect；**unnamed**: bit14-15・bit21-23・**bit24**・bit28-31。EXE 再突合: @0x4131B7 `test edi,0x1180000`（bit20|23|24 ゲート）→ **bit24 単独検査で +0x60→EffectItem+0x14 copy**＝v256 bit24 裁定は維持、ただし **bit24 の GECK 公開名は一次資料に存在しない**（pending 解除は「名なし」として確定扱い可）。

### 8.2 項目6 別系統 6 窓の帰属表（GT 反映用）

| 窓 | レシーバ系 | 引数 | 用途推定 |
|---|---|---|---|
| FUN_005E12B0@0x5E12BE（callers 32・0x5BE-0x5C 帯） | process 族（Base/Low/MiddleLow=0x4943A0 `xor eax,eax;ret 4`・High/MiddleHigh=0x64B350） | **actor ptr**（push ecx） | actor->vt[0x154]() 述語の [process+0x15C] キャッシュ |
| FUN_0060D780@0x60D7DB | esi=[process 系?]（唯一 caller @0x643824） | no-arg | flags 0x20000/0x10000 ゲート・ログ窓併設 |
| FUN_0061CE40@0x61D0EB | [[esp+0x20]] obj（head ゲート vt[0x25C]） | **float**（fstp） | 別系統 setter/問い合わせ |
| FUN_006516D0@0x651718 | [process+0xC0]（HighProcess サブ obj） | no-arg | ゲート `esi&0x80000` |
| FUN_00773920/774370@0x77393C/0x77438C | [[NiDX9TextureData+8]+0x8AC] device 側 | texture data ptr | **VRAM リリース通知**（直後 obj->vt[0x08](1) 自析棄） |

### 8.3 plan（Oblivion_Android 移植）への影響候補

- **formType 完全 69 表**（項目2）: ESM リーダの 4CC→formType 写像はこの 69 項で全量（TOFT=68 は form 化しない注意）。v257 §8.2「移植価値」の表が完成。
- **ロック強度 6 段**（項目4）: 移植側が HUD 文言を持つなら INI 既定 7/20/40/80/99・名 6 種・`fLeveledLockMult` 式（Security×mult+base、100 上限）をそのまま再現可。
- **変更リスト**（項目3）: retail save は list 非依存（slot19/20＋param formID list）と確定 → 移植側で「dirty list→save」相当を実装済みなら設計差の見直し（ドラフトのみ）。
- **PC の anim state 2 代**（項目1）: PlayerCharacter は HighProcess+0x17C 経由でなく **[PC+0x5DC]/[PC+0x5CC] 優先**（vt[0x164] ラッパー）。v236-v241 port 資料の PC 家具経路記述に注記追加候補。
- **MagicItem slot 系譜**（項目6・D2）: ALCH/INGR の ENIT bit0 accessor は **MagicItem ランタイム族専用**。フォーム側（TESObject*）の同オフセット読み出しは表外値（GT の既載 (AL) 更新時、TESForm 族共通スロット説を差し替え）。

## 9. 自己検証・判定まとめ・残点（次束候補）

- **selftest**: 着手時 **ALL PASS (4/4)**（§0 記載）→ 完成直前再実行 → 本節末尾。
- **判定まとめ**: 1 **[確定]**（衝突見かけ・誤帰属訂正・FUN_0065D720 ラッパー/0xDC 2 代/家具 setter 全経路）/ 2 **[確定]**（69 項完全表・TOFT+8 異常のみ [未確定]）/ 3 **[確定]**（NiTLargeArray<TESForm*>・consumer 不在再確認・mgr `.?AVTES@@`・save slot19/20 無接続）/ 4 **[確定]**（setter 全表・ロック強度名表 6 項 INI 全名/既定値）/ 5 **[確定]**（link=FUN_0064FFF0 単一 setter 経由・再索引=7 論理→5 物理・意味名 [未確定]）/ 6 **[確定]**（v257 訂正 2・別系統 6 窓帰属・実装 3 地址の正体）/ 7 **[確定]**（uesp 表 19 名取得・**bit24 は一次資料に定義なし**・EXE mask 0x1180000/bit24 再突合）。
- **本束新規確定 VA/データ**: `FUN_0065D720`（PC vt[0x164]）／[PC+0x5DC/5CC/588/608/610]・`FUN_0065DA50`・FurnitureManager 窓 `FUN_006765F0`+`[0xB3BAD0/D4]`／formType 完全 69 名（TOFT=0x44）／`NiTLargeArray<TESForm*>`（RTTI・cap 0x20・生成@0x9DBF50）／**`.?AVTES@@`**（mgr 0xB33A98・dtor `FUN_004466F0` 0x44671B）／`FUN_005A4700`（message setter 全表）・`FUN_005A46B0`／ロック強度名/閾値 INI 群（sLockLevelName*6・iLockLevelMax*5=7/20/40/80/99・fLeveledLockMult）・`FUN_00429990/42ACD0/429A30`／`FUN_0064FFF0`（+0x17C 唯一 setter・callers 2）・`FUN_004D83B0`（E8 3・ゲート vt[0x08]<=1）／`FUN_004706E0` 写像（5→0/6→3・物理 5 slot）／primary+0xE8 実測表・0x982D85 purecall 形・`0x4A8F90`=TESAmmo+0x50・`0x46C2F0`→0x46B170（formType switch）／`.?AVNiDX9TextureData@@`（0xA89F54）／`0x64B350`/`0x4943A0`（process+0xE8）／UESP MGEF Flags 表 19 名＋unnamed 一覧・EXE mask `0x1180000`。**RTTI 新規デコード 3**（NiTLargeArray<TESForm*>/TES/NiDX9TextureData）。
- **証跡（すべて `%TEMP%\opencode\`）**: `v265_1/1b/1c/1d_out.txt`（項目1）、`v265_2_out.txt`（項目2）、`v265_3/3b/3c/3d/3e_out.txt`（項目3）、`v265_4/4b/4c/4d/4e/4f/4g/4h_out.txt`（項目4）、`v265_5/5b/5c_out.txt`（項目5）、`v265_6/6b/6c/6d/6e/6f_out.txt`（項目6）、Web: websearch 4＋fetch 503×3（§7.1）。再利用した v257 中間物: 各 § の「継承」表記（v257_9/3b/6c/6d/5r/5s 等 out・無再スキャン）。
- **遵守事項**: git 書出なし / docs・GT・TODO・plan・session-memory・cache・ソース無編集（cache 読取のみ・vtable_index.tsv/named_funcs.txt 参照）/ 既存 tmp/re 無上書き（本ファイル v265 のみ新規）/ ESM 本体再スキャンなし / VA→raw はセクション個別換算（v172_common 準守）・capstone 線形ズレ窓（0x5FDEAD 帯・0x9DA7E0 帯）は関数先頭再復唱＋raw byte 直接デコードで補正し本文に反映 / PowerShell ヒアドキュメント不使用（スクリプトファイル化）・`python -c` は正規表現なしの軽量確認のみ / Web 失敗は経過として明記（§7.1）・推測名は [推定] タグ付け / 中断対策（1 項目完了ごとに即追記）**7 回実施** / v257 への訂正は本ファイル §8 ドラフトのみ（v257 本体無編集）。
- **新規残点（次束候補）**:
  1. （低）`FUN_00425040` 2 窓の**引数なし call 形と MagicItem slot+0xE8 `ret 4` 契約の不整合**再検討（レシーバ実クラス＝[esi+0xC] の型特定・v257 裁定「ALCH 経路」の再検証含む）。
  2. （低）process 族 slot58（`FUN_0064FFF0` 隣接帯の actor->vt[0x154]・[process+0x15C]）の語義と callers 32 窓（0x5BE-0x5C 帯）分類。
  3. （低）UIMenu message ルータ側 `vt[0x04]` id 定義表（HUDInfoMenu id 2/0xA の発火名目）。
  4. （低）`FUN_004D7780` 経路 `[entry+0xC]`（0x31/0x32 extra）payload の formID/ptr 別と `FUN_00429990` レシーバ form 級（TESLock 系）。
  5. （低）`[0xB33A98]`（TES*）/[0xB33B00] の生成書込点（CRT 外）。
  6. （低）formType レジストリ TOFT 項 +8=0xA3BA94 異常値の意味。
  7. （低）GECK 側 bit24 設定挙動（TESCS.exe または UI 実機・`Use actor value` との連動仮説の一次確認）。
- **最終 selftest（§8/§9 追記後・2026-11-03 再実行）**: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid 0x0015→impl 0x504560）。
