# V358: 未同定 BSS 53本と NiDX9Renderer vtable 98スロットの同定 — #128 束⑤（最終束）

- 日付: 2026-11-13 / タスク: ai-todo **#128 束⑤**（v283-v288 成果物精査で保留指定の「未同定 BSS」「NiDX9Renderer 個別スロット名」・最低優先）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: `v285_v72_b333c4_residuals.md` §#5（未同定53本・完全リスト・残点4）＋ `v287_v76_data_map_residuals.md` §5-2（98スロット表・残点4）＋ STATE.md「v283-v288 成果物精査（保留指定）」
- 手法: capstone 5.0.7（v307_common.py）／**.text 整列完全走査（operand 級の read/write/幅分類）**／CRT 静的初期化ブロックの tight ウィンドウ抽出（ctor 名前文字列）／**PE インポートディレクトリ解析による IAT 名解決**／vtable 全 impl の body inventory＋rdata クロス参照
- 中間証跡: `%TEMP%\opencode\v358_1..v358_8.py/.out`（**最終全 EXIT=0**。v358_1 は後方デコード順序バグを自己検出→v358_3 で整列走査に全面作り直し、v358_7 はセクション頭の find_head 例外→修正再実行）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM/.ess 未アクセス、EXE 読み取り専用、L1

---

## §0 判定サマリ

| 項 | 対象 | 判定 |
|---|---|---|
| a-1 | **未同定 BSS 53本（v285 §#5）** | **全53本を同定 [確定]28本／[部分確定]25本**。特に帯26本=**魔法学校/タイプ/射程/飛翔体 GMST 表示オブジェクト群**（'Alteration'…'Fog'＋各 `sMagic*` ID）、帯7本=**ロックレベル名 GMST**、`0xB33800/0xB33B80/0xB33980`=**静的 CRITICAL_SECTION**（IAT 名で裏付け）、`0xB33454`='SceneGraph'、`0xB33438`=SetThreadPriority 引数、`0xB33A6C..A8C`=FUN_00445A10 静的状態9フィールド |
| a-2 | v285 の記載誤り | 「`0xB335BC`〜`0xB33684` stride8 **22本**」は実際 **26本**（完全リスト自体は正しい＝計数の誤記を訂正） |
| b-1 | **NiDX9Renderer vtable 98スロット（v287 §5-2）** | **全スロットに機能分類を付与 [確定]33スロット／[部分確定]65スロット**。うち getter/setter 群は**フィールドオフセットまで [確定(命令)]**（+0x5D8/+0x5DC/+0x5E0/+0x878/+0x87C/+0x894/+0x898/+0xA90/+0x61C/+0x620・global 0xB42168/0xB42050）、no-op/stub 14スロット、member vfunc フォワーダ11スロット、+0x6F0 ゲート大群、+0x180 埋め込み CS の Lock、+0x8C0/8D0/8E0 の unlink 3点 |
| b-2 | 正式 C++ メソッド名 | **[未確定]**（メソッド単位の RTTI/シンボル/名称文字列なし [確定負]。v338 の規約どおり機能記述を正式名としない）。クラス名は v287 済み `.?AVNiDX9Renderer@@` |
| b-3 | base vtable 層分け | **[確定負]** `.?AVNiRenderer@@` TD(0xB256B0) は存在するが**単独の NiRenderer vtable が rdata に見つからない**（NiDX9 のみが派生＝フラット配置）→ クラス跨ぎスロット照合は不能。NiDX9 固有 impl は他 vtable 参照 0 件（base スタブ群のみ共有） |

---

## §1 (a) 方法: owner 解明が同定の鍵だった

1. **v285 の観測**: 未同定53本は decompiled 未出力（エリジョン）でラベルが付かない独立 bss。owner は `FUN_00445A10`(38)/`FUN_009D9FA0`(34) 等。
2. **`FUN_009D9FA0` 帯（頭 0x9D9D60〜）は CRT 静的初期化の定型ブロック連鎖**と判明（v358_2）:

```asm
push <dtor fn>            ; (直前ブロックの onexit 登録分)
push "<display name>"     ; ★ 名前文字列1
push "<GMST id>"          ; ★ 名前文字列2
mov  ecx, <BSS addr>      ; this = 静的オブジェクト
call 0x419A70             ; ctor(this, id, display)
push <dtor fn>            ; このブロックの dtor
call 0x981FB4             ; _onexit(dtor)
```

3. **dtor は全て共通ヘルパへ tail-jump**（v358_7）: `mov ecx, <addr>; jmp 0x4055E0`（例 0xA17660 → 0xB335BC）。
4. → **サイト近傍 tight デコード（v358_4）で53本全数の ctor 引数文字列/初期化経路を抽出**。
5. **グローバル同期族は IAT 解決（v358_8・PE インポートディレクトリ実パース）で正式名を獲得**。

---

## §2 (a) 全53本の同定表

### 2.1 帯 26本 `0xB335BC`..`0xB33684`（stride8）= 魔法系 GMST 静的オブジェクト [確定(文字列)]

ctor=`0x419A70`・dtor=`mov ecx,addr; jmp 0x4055E0`（dtor fn は `0xA17660`+）・登録=`0x981FB4(_onexit)`。

| addr | display | id |
|---|---|---|
| 0xB335BC | Alteration | sMagicSchoolAlteration |
| 0xB335C4 | Conjuration | sMagicSchoolConjuration |
| 0xB335CC | Destruction | sMagicSchoolDestruction |
| 0xB335D4 | Illusion | sMagicSchoolIllusion |
| 0xB335DC | Mysticism | sMagicSchoolMysticism |
| 0xB335E4 | Restoration | sMagicSchoolRestoration |
| 0xB335EC | Spell | sMagicTypeSpell |
| 0xB335F4 | Ability | sMagicTypeAbility |
| 0xB335FC | Disease | sMagicTypeDisease |
| 0xB33604 | Lesser Power | sMagicTypeLesserPower |
| 0xB3360C | Power | sMagicTypePower |
| 0xB33614 | Poison | sMagicTypePoison |
| 0xB3361C | Enchantment | sMagicTypeEnchantment |
| 0xB33624 | Potion | sMagicTypePotion |
| 0xB3362C | **Wortcraft**（vanilla 拼字そのまま） | sMagicTypeWortcraft |
| 0xB33634 | Scroll | sMagicCastOnce |
| 0xB3363C | Weapon | sMagicCastWhenStrikes |
| 0xB33644 | Staff | sMagicCastWhenUsed |
| 0xB3364C | Apparel | sMagicCastConstant |
| 0xB33654 | Self | sMagicRangeSelf |
| 0xB3365C | Touch | sMagicRangeTouch |
| 0xB33664 | Target | sMagicRangeTarget |
| 0xB3366C | Ball | sMagicProjectileTypeBall |
| 0xB33674 | Bolt | sMagicProjectileTypeBolt |
| 0xB3367C | Spray | sMagicProjectileTypeSpray |
| 0xB33684 | Fog | sMagicProjectileTypeFog |

- 分類: **school 6＋type 10（Spell..Potion/Wortcraft）＋cast 状態4（CastOnce..Constant）＋range 3＋projectile 4** の**表示名 GMST オブジェクト列**（配列は用途別グループが連続、末尾 0xB33684 の次は `fMagicBallOptimalDistance` フロート GMST が続く＝float 用 ctor `0x41BAE0` 系へ遷移）。

### 2.2 帯 7本 `0xB33880`..`0xB338B0` = ロックレベル系 GMST [確定(文字列)]

| addr | 内容 | ctor |
|---|---|---|
| 0xB33880 | **fLeveledLockMult**（float GMST） | `0x41BAE0`（float 用） |
| 0xB33888 | 'Very Easy' / sLockLevelNameVeryEasy | 0x419A70 |
| 0xB33890 | 'Easy' / sLockLevelNameEasy | 0x419A70 |
| 0xB33898 | 'Average' / sLockLevelNameAverage | 0x419A70 |
| 0xB338A0 | 'Hard' / sLockLevelNameHard | 0x419A70 |
| 0xB338A8 | 'Very Hard' / sLockLevelNameVeryHard | 0x419A70 |
| 0xB338B0 | 'Impossible' / sLockLevelNameImpossible | 0x419A70 |

- 続きの `0xB338B8`+（`iLockLevelMaxVeryEasy`(7)/`iLockLevelMaxEasy`(20)…）は decompiled ラベル済み（本リスト外）と確認。

### 2.3 グローバル同期オブジェクト群 [確定(IAT)]

| addr | 同定 | 根拠 |
|---|---|---|
| **0xB33800** | **静的 CRITICAL_SECTION**（BaseExtraList 系が使用） | `mov ecx,0xb33800; call 0x401020(=EnterCriticalSection サンク)` ＋ `0x401040(=Leave)`（v358_3/4: サイト 0x41DFF9.. の guard 名 'BaseExtraList::RemoveAll'/'ItemsInList'/'AddExtra'。注: guard 名の push はサンク `[esp+4]=ecx` で上書きされ IAT へは渡らない＝vestigial [確定(命令)]） |
| **0xB33B80** | **静的 CRITICAL_SECTION**（ExtraDataList/fn0x4637C0 帯が使用） | 同サンク群（'ExtraDataList::RestoreSavedAnimationData' guard・v357_1 の 0x463863 等） |
| **0xB33980** | **静的 CRITICAL_SECTION**（once 初期化 `[0xB33A00]`bit0） | `mov ecx,0xb33980; call 0x401000(=InitializeCriticalSection サンク); push 0xA17B60; call 0x981FB4(_onexit)`。dtor `0xA17B60` は `0x65C620`（ID レジストリ登録系）＋`0x401F20`(delete) [部分確定: dtor は登録解除＋解放] |

**ロック族サンク（IAT 正式名）** [確定(IAT)]:

| fn | IAT | 実体 |
|---|---|---|
| 0x401000 | 0xA28064 | KERNEL32!**InitializeCriticalSection**（this 転送・戻り this） |
| 0x401010 | 0xA28068 | **DeleteCriticalSection** |
| 0x401020 | 0xA2806C | **EnterCriticalSection**（`mov [esp+4],ecx; jmp`＝thiscall→引数書換） |
| 0x401030 | 0xA28070 | **TryEnterCriticalSection**（bool 返 `setne`） |
| 0x401040 | 0xA28074 | **LeaveCriticalSection** |
| 0x401050 | 0xA2807C | **InterlockedDecrement**（`lea eax,[esi+4]`＝refcount(+4) 減算→0 なら vtable slot0 へ dtor 委譲＝**Release 相当** [確定(命令)]） |
| （補助）0x401060 系 | 0xA2808C | GetCurrentThreadId（同帯） |

### 2.4 その他 [判定別]

| addr | 同定 | 判定 |
|---|---|---|
| 0xB33454 | **'SceneGraph' 命名静的オブジェクト**（ctor `0x70E220(this, 'SceneGraph', 0xB3FAB0)`） | [確定(文字列)] |
| 0xB33438 | **SetThreadPriority 引数ストア**（switch(0..5) のコード −2/−1/1/2/0xF を書出・6書込） | [確定(IAT)] |
| 0xB333F5 | byte フラグ（唯一書手 `mov byte [0xb333f5],1` @0x4062DD） | [部分確定] |
| 0xB3341C | byte フラグ（0x40D822 cmp／0x40D884=0／0x40D88C=1） | [部分確定] |
| 0xB33427 | byte フラグ（0x410DB9 movzx 読／0x410E72 書） | [部分確定] |
| 0xB33430 | byte（唯一書手 `mov byte [0xb33430],0` @0x40F0CA＝zeroing） | [部分確定] |
| 0xB33A35 | 静的オブジェクト（init = `mov ecx,0xB33A35; jmp 0x749280`＝ctor tail-jump） | [部分確定]（クラス未特定） |
| 0xB33AC4 | **リファレンスカウント付き静的オブジェクト**（count=`0xB33AC0`・release=`0x7C2A40`・ctor=fn0x44EE00 once ブロック `call 0x44D720`・dtor=`0xA18360`） | [確定(命令)]（束③(v357)と統合） |
| 0xB33A6C..0xB33A8C（9本・stride4） | **FUN_00445A10 の静的状態構造体**（bounds 0x445A10..0x4477F0＝size 0x1DE0 が全サイトを包含）: A6C=dword カウンタ(add1)／A70=dword ptr／A74=dword ptr／A78=dword( cmp0・add1＝**長さ/カウンタ**)／A7C=**byte フラグ**／A80=dword ptr／A84=dword(頻繁 R/W)／A88=dword／A8C=**byte フラグ** | [確定(命令)]（構造）／用途名 [未確定] |
| 0xB33800（再掲） | BaseExtraList 系 guard 群の CS | [確定(IAT)] |

**補足（関連する同定済みグローバルの意味も併記）**: 0xB33434=スレッドハンドル（WaitForSingleObject(handle,-1) @FUN_0040FDD0）／0xB33425=busy byte／0xB33418=カウンタ（FUN_0040D4D0）→ **53本のうち 0xB3341C/27/30/38 はこのスレッド制御群の周辺** [確定(命令+IAT)]。

---

## §3 (a) 残点

- **V-358-1** [低] `0xB33A35` の ctor `0x749280` の対象クラスと用途。
- **V-358-2** [低] byte フラグ4本（F5/41C/27/30）の意味命名（構造と読み書き点は確定済み）。
- **V-358-3** [低] FUN_00445A10 静的状態9フィールドの用途名（本体 0x1DE0 の全文脈解析は未実施）。

---

## §4 (b) NiDX9Renderer vtable 0xA88EA4 — impl inventory（83 unique / 98 slots）

v287 は「impl 全数確定・名称のみ未取得」。本件は **impl の body 実読で機能分類を全スロットに付与**した。

### 4.1 no-op / 定数スタブ [確定]

| impl | 本体 | slots |
|---|---|---|
| 0x6F7070 | `xor eax,eax; ret` | 2,3,4,5,16,17（6） |
| 0x68F970 | `ret 4`（1引数破棄） | 8,10,13,15,18,70（6） |
| 0x60D0A0 | `ret` | 74,75（2） |
| 0x60CF60 | `ret 8`（2引数破棄） | 61,62（2） |
| 0x6FDFF0 | `mov al,1; ret 4`（恒 true） | 19,64（2） |
| 0x700640 | `mov eax,ecx; ret 4`（this 返し） | 6 |

### 4.2 getter / setter — フィールドオフセットまで [確定(命令)]

| slot | +off | impl | 機能 |
|---|---|---|---|
| 21 | +0x054 | 0x769F20 | `return [this+0x5E0]`＝**flags getter**（v287 の「フラグクエリ」本体） |
| 22 | +0x058 | 0x769F70 | `[this+0x5D8] = arg(float)` SetFloat |
| 23 | +0x05C | 0x769F80 | `return [this+0x5D8]` GetFloat |
| 27 | +0x06C | 0x76A170 | `[this+0x5DC] = arg` SetU32 |
| 28 | +0x070 | 0x76A180 | `return [this+0x5DC]` GetU32 |
| 31 | +0x07C | 0x762590 | `return [this+0x878]` |
| 32 | +0x080 | 0x769F60 | `return [this+0x87C]` |
| 33 | +0x084 | 0x769F10 | `return byte [this+0xA90]` |
| 39 | +0x09C | 0x762570 | `return [this+0x894]` |
| 40 | +0x0A0 | 0x762580 | `return byte [this+0x898]` |
| 42 | +0x0A8 | 0x7625E0 | `[0xB42050] = arg; return 1`（global set） |
| 43 | +0x0AC | 0x7625F0 | `return [0xB42050]`（global get） |
| 1 | +0x004 | 0x769F00 | `return 0xB42168`（**global ポインタ返し**） |
| 84 | +0x150 | 0x762FB0 | `[this+0x61C]/[+0x620] = (arg1,arg2)` 2語 setter（ret 8） |

### 4.3 member vfunc フォワーダ [確定(命令)]

| slot(s) | 宛先 |
|---|---|
| 34,35 | `[this+0x880]` の vtable **+0x74** / **+0x70**(0) へ |
| 41 | `[this+0x8B8]` vtable **+0x0C** |
| 46 | `[this+0x8AC]` vtable **+0x50** |
| 56,57,58,59 | `[this+0x8AC]` vtable **+0x48/+0x4C/+0x3C/+0x40(float)** |
| 9,38 | `this` 自身 vtable **+0x20 / +0x94**（引数 8,0x20,arg 付き） |
| 95,96,97 | `[arg+0x24]=0` → `add ecx,0x8D0/0x8C0/0x8E0` → `jmp 0x4524C0`＝**埋め込みリスト3本の unlink** |
| 67 | `EnterCriticalSection(&[this+0x180])` → **+0x180 = 埋め込み CS の Lock** [確定(IAT)] |

### 4.4 +0x6F0 ゲート大群 [部分確定]

`cmp byte [this+0x6F0], 0` で早出/分岐する大関数群: slots **53,54,55,77,78,80,81,82,88,89,90,91,92,93,94**（impl 0x764E50/0x764FE0/0x765120/0x762600/0x762640/0x762BE0/0x7647E0/0x7693E0/0x7694F0/0x769700/0x765990/0x766880/0x76B160 等）＋ **52(0x7625D0) / 79 / 85(+0x614) / 86,87**。
- `+0x6F0` = **デバイス/レンダ実行可否系 byte フラグ**（NiDX9 は DX9 デバイス状態を持つため）[部分確定]。
- 大関数（size 0x700〜0x2000 超）は実装差分が大きく残点（§6）。

### 4.5 その他（trivial 以外の個別 impl）[部分確定]

0: 0x76BED0（`call 0x76B5D0`→flag&1 分岐＝**flag 付き dtor 形**）／7: 0x7008A0（`[arg+0xD8]==0x5000006` 検証）／11: 0x700670（null→bool）／12: 0x701A30（`call 0x7009A0`）／14: 0x700750（`0x452570(arg,1,this)`＝リスト挿入系）／20: 0x762480（`sub esp,0x450` スタックバッファ大）／24,25,26: float/byte 変換（0x76A040/0x769F90/0x76A110、`[+0x5D7]` ほか）／29,30,36,37,44,45,47,48,49,50,51,52,60,65,66,68,69,71,72,73,76,83,85: 個別大中関数（各ヘッダは v358_5 記録）／63: 0x4348A0 `delete(arg)`（**operator delete 相当** [確定]）／60: 0x725FA0 `if(arg) new(arg)`（new 相当）。

### 4.6 クロス vtable（層構造）[部分確定／一部 確定負]

- **base スタブ共有群**: 0x6F7070=rdata **2,738 参照**／0x68F970=**2,329**／0x60D0A0=**863**／0x60CF60=**474**／0x700750=176／0x6FDFF0=168／0x700640=118／0x4348A0=9／0x7008A0・0x700650・0x700670=各16 → **数百の vtable に共有される基底層スタブ**であること [確定]。
- **NiDX9 固有 impl（0x76xxxx 等）は他 vtable 参照 0 件**（自己スロットのみ）→ 単継承フラット vtable [確定]。
- `.?AVNiRenderer@@` TD=0xB256B0 は存在するが **COL→vtable への解決0件**＝**単独 NiRenderer vtable なし [確定負]** → スロットの「基底クラス正式名」対応表は作成不能。

---

## §5 (b) 残点

- **V-358-4** [中] +0x6F0 ゲート大群と個別大関数（4.4/4.5 の [部分確定]）の実用途（デバイス lost/初期化/描画状態の切り分け）。
- **V-358-5** [低] 正式 C++ メソッド名全般（名称証拠なし [確定負]・Ghidra/シンボル導入時のみ取得可能）。
- **V-358-6** [低] slot0 `0x76BED0` が deleting dtor か single dtor か（flag&1 分岐の語義）。

---

## §6 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v358_1..8 **最終全 EXIT=0**。v358_1 の後方デコード順序バグ（back=1 が `8B 0D <addr>` を `or eax,imm` 誤読）を**出力の破綻から自己検出し**、v358_3 で operand 級の整列完全走査へ全面作り直し（v358_1 の結果は棄却） |
| 2 | 53本の網羅 | 整列走査（2.258M insn と同方式）で全53本の参照種別を分類し、**53/53 に同定または構造分類を付与** |
| 3 | 帯ラベルの頑健性 | tight ウィンドウ抽出＋**隣接ブロックとの交叉検証**（各 site の前後 push が隣接ブロックの dtor/文字列と矛盾しないこと、dtor thunk `jmp 0x4055E0` の対象 addr と一致）を全帯で実施 |
| 4 | IAT 同定 | PE ヘッダ実パース（e_lfanew 0x108・import dir RVA 0x700304・6セクション）→ 対象13スロット全解決・hint 付き。**ロック族6本＋WaitForSingleObject/SetThreadPriority/InterlockedDecrement** |
| 5 | 98スロット | v287 の表と impl 一致を全数照合（99 読出＝+0x188 終端0含む＝v287 記載どおり） |
| 6 | 旧成果物との整合 | v285 の「22本」**計数誤記のみ訂正**（完全リスト・判定は維持）。v287 残点4のうち「4. スロット名称」を本裁定で機能分類まで解決、正式名のみ [未確定] |

---

## §7 main への報告

1. **v285 残点4（未同定53本の要素型・用途）= 本裁定でクローズ**: 26＋7 の GMST 群（名前文字列全数）・CRITICAL_SECTION 3本（IAT）・SceneGraph・SetThreadPriority ストア・FUN_00445A10 静的状態9フィールド・byte フラグ4本（挙動付き）・refcount 付きオブジェクト。**v285 の「22本」は 26本の計数誤記を訂正**。
2. **v287 残点4（スロット名称）= 機能分類まで解決**: getter/setter はフィールドオフセット確定、スタブ14、フォワーダ12、+0x6F0 ゲート群、埋め込み CS Lock。**正式 C++ 名は証拠なしで [未確定]**（v338 規約遵守）。**base vtable 層分けは NiRenderer vtable 不在で [確定負]**。
3. **移植側への示唆**: 53本は decompiled ラベル不要で**バイナリアドレス直打ち**可（v285 の指摘どおり）。ロック3本はCriticalSection 相当（Android 移植では mutex 化可能）。NiDX9 は Dx9 固有＝#51 判定どおり移植対象外（分類表は参照のみ）。
4. docs/GT 反映（RTTI/CriticalSection 族の正式名）は #47/#20 一括（L1 では docs 無編集）。

---

## §8 証跡一覧（%TEMP%\opencode\）

| probe | 内容 |
|---|---|
| v358_1 | 53本 census（**後方デコードバグ検出により棄却・記録のみ**） |
| v358_2 | owner 5関数の本体（FUN_009D9FA0 の CRT 定型ブロック発見＝転換点） |
| v358_3 | **整列完全走査 census＋ラベル抽出第1報**（帯26本・SceneGraph・lock 系の初確定） |
| v358_4 | tight ウィンドウ第2報（帯の各ブロック名前文字列・ロックレベル7本・GMST id 全数） |
| v358_5 | NiDX9 83 unique impl inventory（head/size/calls/strings） |
| v358_6 | クロス vtable 共有数・NiRenderer TD 解決試行 |
| v358_7 | ロック族本体・各ブロック完全デコード・dtor thunk・FUN_00445A10 bounds |
| v358_8 | **PE インポート解析で IAT 13スロット正式名解決** |
