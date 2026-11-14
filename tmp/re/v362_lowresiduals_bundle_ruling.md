# V362: 低残点7件まとめの裁定 — #141（V-354-1..4 / V-357-5・6 / V-358-1・2・5・6 ほか）

- 日付: 2026-11-13 / タスク: ai-todo **#141**【RE 低優先・v354/v357/v358 低残点まとめ】
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: `v354_v350_residuals_ruling.md` §8 ／ `v357_named5_ruling.md` §7 ／ `v358_bundle5_bss_nidx9_ruling.md` §6（＋v360/v361 の未命名文字列）
- 手法: capstone 5.0.7（v307_common.py）／**整列 cover-check**（find_head 大窓＋ストリーム到達検証＝byte-scan 偶然一致の排除）／RTTI（pTD=+0xC）／PE インポート解決／生バイト走査
- 中間証跡: `%TEMP%\opencode\v362_1..v362_8.py/.out`（**全 EXIT=0**。初回出力の未整列/byte-scan 誤検出を v362_6/7/8 の cover-check で是正・追補したことを明記）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM/.ess 未アクセス、EXE 読み取り専用、L1

---

## §0 判定サマリ

| 残点 | 判定 |
|---|---|
| **V-354-1** 型別 reader 登録元の全列挙 | **[部分確定] 維持**: 整列確定したテーブル ref＝SIB **読**2件（`0x6BD1AA: mov edx,[ecx*4+0xB3D028]`／`0x6D8DA2: mov ebp,[eax*4+0xB3D4D0]`）＋ v354 済の lazy-init 書。**残 byte-scan hits（0x6BD5A0 等6件）は整列ストリーム非カバー＝生バイト偶然一致と判定 [確定負]** → 登録元の全列挙は未達（低・維持） |
| **V-354-2** u32 Order → u16 切り詰め | **[確定] 切り詰めあり**: 整列実測 `0x6E1ECB: mov word [esi+8],ax`・`0x6E1EE9: mov word [esi+8],bx`（**head=0x6E1E90 KeyGroup0 setter 内**）＋ `0x6E0F20`/`0x6E1B50`（head 0x6E0EF0/0x6E1A80）。EXE は [+8] を **u16 で書く**＝walk.rs の u32 skip との非対称が確定（v354 の推測を確定昇格） |
| **V-354-3** 0x6D4300 以降の全型分岐・2段目 TD | **[未確定] 維持**（周辺前進）。`0x6D37xx` 帯の vptr store を **RTTI 実名で同定**: `0xA7A2EC=.?AVNiFloatData@@`／`0xA7A624=.?AVNiPosData@@`（N5b 帯の型構築箇所）。**0xB3DBDC は store 未発見**（address load `mov eax,0xB3DBDC`@0x6D91A0／READ `mov eax,[0xB3DBDC]`@0x6D939C／`mov ecx,0xB3DBDC`@0xA04C2A の3系統のみ）→ 値は BSS＝マップ外・ローダ zero-init [推定]（実行時 store は callee 内 [未確定]） |
| **V-354-4** 0xA7DEB4 定数・文字列 | **[確定]**: **`0xA7DEB4 = 0x7F7FFFFF = FLT_MAX (3.40282e+38)`**＝bool 0 時の float 代替＝**FLT_MAX センチネル**。`0xA79E2C='m_kTranslate'`／`0xA79E3C='NiQuatTransform'`（Gamebryo 変換構造体のメンバー名＝文脈確定） |
| **V-357-5** 545 vs 530 の差15 | **自側 545＝全件実在 [確定]**（76件の dropped を find_head 窓 0x4000 で **76/76 再検証・残0**＝最初の0件誤判定は窓不足）。**差15は v316 側の集計条件差 [部分確定]**（再現不能・原因は特定せず） |
| **V-357-6** bit7 側背 mask 群の帯属 | **SaveGame+0x18 ではない [確定]**（head 8件で `[0xB33B00]` 非使用を実測・残2件も他 object context）。帯属: `0x53CF31`=NiRefObject 系 word flags（refcount+4 を InterlockedDecrement）／`0x703EC0/0x7043B0/0x704510/0x704800` 系＝thiscall 別クラス word flags（0xF00F）／`0x7AB491`=word flags／`0x760010` 系＝dword 減算（0x76006E）／`0x98DF50`＝**構造体一括ゼロ**（{+0,+8,+0x18} zero＋[+0xC] mask `0x19741000`）／`0x6D8200`＝N5b 帯 word 減算。**残2件（0x99AFC1/0x5A7891）は頭不能で帯属 [未確定]** |
| **V-358-1** 0xB33A35 の ctor 0x749280 対象クラス | **[未確定] 維持**（本束で未到達・RTTI 未取得。周辺＝fn0x441970 のグローバル構築帯を確認したが 0x749280 は未 decode＝残点 V-362-2） |
| **V-358-2** byte フラグ4本 | **[部分確定] 4本すべて機能文脈を特定**: 下表 §2 |
| **V-358-3** FUN_00445A10 静的状態9フィールドの用途 | **[部分確定] 構造的役割を確定**（§2.5）: `A8C/A7C`=byte フラグ（mode 4/5 等で set）／`A88`=現在 mode 値（esi 引数）の保存／`A78/A6C`=カウンタ（cmp0・add1）／`A70/A74/A80/A84`=dword 保存（ptr/値）。**fn0x445A10 内の mode パラメータ駆動シーケンス**（esi∈{-1,0,1,4,5} 分岐・`[0xB33A98]` 経由 `0x44E720/0x44CF80` 呼出・`[ebp+0x51]` 設定・`[0xB06B18]` グローバル flag）を整列デコードで確認 [確定(命令)]。**群としての用途正式名=[未確定]**（低・継続） |
| **V-357-2/3・V-358-5/6** 低命名群 | 0x659BE0 属クラス=**`.?AVMobileObject@@` [確定(RTTI)]**（**v357 の「Actor」推定を訂正**）／slot0 `0x76BED0`=**flag&1 付き deleting dtor [確定]（V-358-6 クローズ）**／`0x58CEB0`=**id-based float setter [確定(命令)]**／0x5BA4D0=**非 virtual [確定]**→名 [未確定] 維持／0x464440 の this クラス=[未確定] 維持／NiDX9 正式メソッド名=**静的不能 [確定負]**で [未確定] 維持 |

---

## §1 V-354 系（N5b・v354 残点）

- **V-354-2 [確定]**: KeyGroup0 setter `0x6E1E90` 内で `[esi+8]` へ **u16 ストアを整列実測**（0x6E1ECB ax／0x6E1EE9 bx）。band 内の u16→+8 ストアは他に 0x6E0F20/0x6E1B50/0x6C6845（u16 copy 形）等21件を生バイト走査で列挙（うち4件を整列確定）。
- **V-354-4 [確定]**: FLT_MAX センチネル＋NiQuatTransform/m_kTranslate 文字列（§0）。
- **V-354-3 [未確定] 維持**: NiFloatData/NiPosData ctor 同定・0xB3DBDC の3系統（load/read/address）発見まで。store 未出＝値取得不能。
- **V-354-1 [部分確定] 維持**: 整列確定は読2件のみ、残 hits は偶然一致 [確定負]。

---

## §2 V-358-2: byte フラグ4本の文脈 [部分確定]（v362_2 + v362_7 IAT/文字列）

| flag | 文脈（証拠） | 機能記述 |
|---|---|---|
| `0xB333F5` | `mov ecx,[0xB33398]→0x405370` 判定→ebx==0 で set=1 → **`call [0xA28260]=USER32!DefWindowProcA`** 直前（0x4062E4 帯） | ウィンドウメッセージ処理系の**1発 byte フラグ [部分確定]** |
| `0xB3341C` | fn `0x40D800`（**find_head が再び CC不足型で 0x40D4D0 誤答**）: `0x578F60` 判定で set 1／clear 0、**flag!=0 時のみ** `0xB3BD00→0x673A50→0x7616D0→0x659A00→vfunc+0x154` の処理を実行 | **状態 byte フラグ（本体は flag≠0 ガード）[部分確定]**。副次: **同一帯の fn0x40D4D0 本体が SaveGame `+0x18` の bit13(0x2000)/bit15(0x8000) を操作**（0x40D554/0x40D56B）＝SaveGame flags map の新規事実 |
| `0xB33427` | fn `0x410D10`＝**拡張子比較ハンドラ**: `strcmp` 系 `0x9836C9` で入力 vs **`'bik'`(0xA319F8)/`'dds'`(0xA319F4)/','(0xA319FC)** → `0x410BA0` へ7引数ディスパッチ（flag は read 引数）・別 site で al から set | **拡張子系 mode byte [部分確定]** |
| `0xB33430` | メッセージループ域: 文字列 **`'Begin Idle loop...'`(0xA30E70)**・IAT `PeekMessageA/TranslateMessage/DispatchMessageA`・`0x40F0DB→0x40FDD0`(Wait 系) と同一 fn 帯で一発 set=0 | **Idle ポーリング one-shot クリア byte [部分確定]** |

---

## §3 V-357-5 / V-357-6（計数・帯属）

- **545 全件実在 [確定]**: dropped76 を窓0x4000 で全数 cover（残0）。→ v316 530 との差15は**相手側の条件差**（v316 の exact フィルタは再現不能）→ **[部分確定] で原因特定を打ち切り**（自側の正当性は [確定]）。
- **mask 群は SaveGame 非使用 [確定]**: 帯属表は §0 のとおり。`0x98DF50` の `and [esi+0x18],0` は**構造体一括ゼロの一部**（+0/+8/+0x18 と [+0xC] mask 0x19741000）であることを文脈で確認。

---

## §2.5 V-358-3: FUN_00445A10 静的状態9フィールド [部分確定]（v362_5a + v362_7 slice）

- **field アクセス全数（fn 0x445A10 内・整列 range スキャン）**: `A6C`=add1 のみ（単発カウンタ）／`A70`=store ebx・load×2（ptr）／`A74`=store edi・load×2（ptr）／`A78`=store edi・`cmp0`・`add1`（**カウンタ/非0ゲート**）／`A7C`=**byte フラグ**（set 1/clear 0/cmp）／`A80`=ptr（load/store/cmp）／`A84`=**頻繁 R/W dword×9**（現在値バッファ）／`A88`=mode 値 store/load／`A8C`=**byte フラグ**（set 1/cmp 0）。
- **フェーズ文脈（0x445E00 スライス・整列）**: 引数 `esi=[esp+0x13c]`（**mode 値**: 4/5 → `A8C=1`、-1 → 0x445F47、0 → 0x445F52、1 → 別枝）→ `A88=esi` 保存・`[ebp+0x51]=1`・`[0xB06B18]=1` → `A8C==0` 時のみ log 文字列（0xA37810/0xA377D4/0xA37790 経由 `0x4A7A60`）＋**`[0xB33A98]` 経由で `0x44E720`/`0x44CF80` を呼出** → `A8C=1`。
- → **判定**: 群は「fn0x445A10 の mode 分岐シーケンス用静的状態」[確定(命令)]、個別フィールドの機能名（どこの状態か）とシーケンス全体の用途名＝**[未確定]（低・継続＝V-362-3 へ）**。

---

## §4 追加成果（本束の探査で新発見）

1. **`[0xB33B00]` と `[0xB33A98]` の生成経路 [確定(命令)]** — 全て fn **`0x441970`（＝束③の「container ctor」を拡張したグローバル群構築関数）** 内で整列確認:
   - `[0xB33A1C]` ← 0x43E950 結果
   - **`[0xB33A98]` ← `new(0xCE0=3296B)` → ctor `0x446D80` → `call 0x44A2B0`**
   - **`[0xB33B00]` ← `new(0x1C4=452B)` → ctor `0x45B300`**
   - 両 class 名は ctor 内 vptr store 未取得で**残点（V-362-1・低）**
2. **v360/v361 の未命名文字列を解決（IAT/strings）**:
   - **`0xA89618` = `'NiDX9Renderer::LostDeviceRestore> Lost device notification function failed'` → v361 の残点 **V-361-4 クローズ**＋**fn 0x76B3F0 の機能を自文字列で裏付け**（LostDeviceRestore 系）**
   - `0xA390D4` = `'Could not create save buffer, out of memory.'`（v360 route B new[] 失敗ログ）
   - `0xA3B088` = `'SaveGame() call did not properly fill buffer.'`（v360 route B 整合検ログ）
   - `0xA3AFA0` = `'LoadGame() call did not properly empty buffer. See Warnings.txt…'`（v360 0x46667E 整合検ログ）
   - `0xA3B144` = `'quicksave'`（v360 の writer caller 0x466AF6 の section 名）
   - `0xA3B230` = `'Created form %08X with type %s is going to be reset due to flags: %08X'`
3. **IAT 解決（PE 実パース）**: `0xA28260=DefWindowProcA / 0xA28264=DispatchMessageA / 0xA28268=TranslateMessage / 0xA2826C=PeekMessageA` → 0x40xxxx 帯の位置づけ（ウィンドウ/メッセージループ）を確定。
4. **fn0x40D4D0 の SaveGame `+0x18` bit13/bit15 操作**（§2・flags map 追加事実）。
5. **整列 cover-check の教訓**: 生バイト走査（`66 89`・table addr 等）は**偶然一致を含む**（u16 初出21件中 context が mid-decode のもの、table ref 6件が非実在）→ **cover-check 必須**を手順として文書化。

---

## §5 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v362_1..v362_8 **全 EXIT=0**。初回出力の限界（未整列・byte-scan 誤検出）を後続 probe で**自己発見・是正**（545 全実在確認／table hits 非実在判定／u16 store 整列確定） |
| 2 | 切り詰めの実在性 | 4件を **cover-check 整列確定**（head+ストリーム到達）で裏付け |
| 3 | 計数差の評価 | 「自側誤検出」仮説を**76/76 再検証で否定**し、残因を相手側条件差に切り分け |
| 4 | 帯属判定 | head 起点整列デコード＋`[0xB33B00]` 使用有無の機械判定（8件実測） |
| 5 | 文字列/IAT | すべて `str_at`／PE インポートディレクトリ実パースの一次証拠 |

---

## §6 残点（本束で更新）

| ID | 内容 | 深刻度 |
|---|---|---|
| V-362-1 | [未確定] `[0xB33A98]`/`[0xB33B00]` の class 正式名（ctor 0x446D80/0x45B300 の vptr/RTTI 未取得） | 低 |
| V-362-2 | [未確定] `0x749280`（0xB33A35 の ctor）の対象クラス（本束未到達＝V-358-1 維持分） | 低 |
| V-362-3 | [未確定] FUN_00445A10 mode シーケンス群（9フィールド）の用途正式名（構造は §2.5 で確定） | 低 |
| V-354-1 継続 | 型別 reader 登録元の全列挙（読2件確定・書は未達） | 低 |
| V-354-3 継続 | 0x6D3770 以降の全型分岐・0xB3DBDC の store 経路 | 低 |
| V-357-6 継続 | 0x99AFC1/0x5A7891 の帯属（頭不能） | 低 |
| （旧）V-361-4 | **→本束でクローズ**（0xA89618 文字列解決） | - |

---

## §7 main への報告

1. **V-354-2/V-354-4/V-357-5(自側)/V-357-6(SaveGame 否定)/V-358-2/V-358-6 がクローズ**、V-354-1/V-354-3/V-358-1/**V-358-3** は周辺前進のまま低で維持、**V-361-4 は本束でクローズ**。
2. **確定昇格**: u32→u16 Order 切り詰め（walk.rs 非対称の実証）／`0x659BE0`＝**MobileObject**（v357 の Actor 推定を訂正）／slot0＝deleting dtor／`0x58CEB0`＝id float setter／`0xA7DEB4`＝FLT_MAX。
3. **新規確定**: `[0xB33B00]`=new(0x1C4)+ctor 0x45B300、`[0xB33A98]`=new(0xCE0)+ctor 0x446D80（生成=fn 0x441970）／USER32 ループ IAT 4件／SaveGame+0x18 bit13/bit15 操作（fn0x40D4D0）／v360・v361 の警告/ログ文字列6件を命名。
4. docs/GT 反映候補（MobileObject・u16 切り詰め・LostDeviceRestore 文字列・bit13/15）は #47/#20 一括（L1）。

---

### 参照（本タスクの証跡）

- v362_1（定数/KeyGroup setter/table byte-census/BSS store 初搜/type rdata imm）
- v362_2（byte flag 4本の window・fn head・strings）
- v362_3（mask 11 site の head・`[0xB33B00]` 使用判定）
- v362_4（545 再収集＋cover 検証＝dropped76 検出）
- v362_5（FUN_00445A10 field hits・0x659BE0 vtable RTTI=.?AVMobileObject@@・0x58CEB0/0x76BED0 本体・0x464440 callers・[0xB33A98] assigner 初出）
- v362_6（**dropped76→76/76 再検証**・0xB3DBDC 再搜・NiFloatData/NiPosData RTTI・table ref cover 初回・u16 store 初搜）
- v362_7（**IAT 4件解決・ラベル文字列11件**（v360/v361 文字列含む）・0x40D800 帯 decode・[0xB33A98]/[0xB33B00] 生成序列・field slice・0x58B220 head）
- v362_8（**生成序列の整列確定（head 0x441970）**・u16 store cover 確定4件・table hits 非実在判定・0xB3DBDC 3系統）
