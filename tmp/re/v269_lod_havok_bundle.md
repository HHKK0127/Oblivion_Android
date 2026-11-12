# v269 — LOD/Havok 統合残点束（7件）（Oblivion.exe v1.2.0416 Steam）

- 出典: `tmp/re/v266_object_lod_residuals2.md` 新規残点 V-266-1a / V-266-2a ＋ `tmp/re/v267_process_havok_residuals14.md` §7-2 残点 1〜5（serial 262/V262・低）
- 分析担当サブエージェント（契約=分析のみ・成果物この1ファイル）。docs/GT/TODO/plan/session-memory/cache/ソース無編集・訂正はドラフト表記のみ・リポジトリ側ループ運用指示（STATE.md/L2/worktree/テスト）には不従
- 環境: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7、.text VA=0x401000/raw 0x400、.rdata VA=0xA28000/raw 0x627200、.data VA=0xB02000/raw 0x700C00）、Havok 帯 `0x84xxxx–0xA2xxxx`。VA 換算・走査は `v172_common.va2off/rd/dump/e8_callers/imm32_refs3`・`v249_util(refs/vt_dump/e8)`・`v217_util(fname/cstr)` に統一
- 前提（既確定・再検証せず引用）: 窓式 `G ≤ idx < G+D`・外周 band `18 ≤ d ≤ 27`・bit30 XOR flag bit2 圧縮式（v266 件2）・14 表=生（v259 死テーブル訂正済・v267 §1）・hkSweptTransformDisplayViewer 等 9 実名（v267 §2）・hkVisualDebugger::createClient/port 25001（v267 §6）
- 方針: 各件「[確定]/[推定]/[未確定]＋VA・証拠」。「確認できず」と「不存在証明」を区別
- 中断対策: **1件終えるたびに即ファイルへ追記**（§1→§7 の順）
- selftest: 着手時 `probe_harness.py selftest` = **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 → ALL PASS）
- 中間物: `%TEMP%\opencode\v269_*.py`（出力同所 `_out.txt`）・既存 `v266_*/v267_*` 再利用

## 対象 7 件

1. kind 分布の运行时ログ実測（V-266-1a・低）— PC 版実行環境非所有。静的導出済みのため**未確定確認のみ**（試行範囲を記録）
2. packed-索引モード（`[obj+0x194]&8`・設定側 `0x42EF6B`）の f2 bit31 無効レコード経路（V-266-2a・低）— 出荷 17 BSA で bit31=0・distantlod 発火なし[推定]の最終化
3. OI/Statistics 静的 registerProcess（`0x946552`/`0x947632` 帯）の呼出経路（V-267 §7-2-1・低）
4. WorldMemory/MousePicking/Tensor の viewer 実名探索（V-267 §7-2-2・低）— 5.1 公開ソース listing 参照可・Tt 名表の残り
5. enable ラッパ呼出 6 点（0x445xxx/0x4D4-6xxxx）のゲーム側イベント同定（V-267 §7-2-3・低）
6. +0x20（B 系 2 空仮想）のメソッド名・型名（V-267 §7-2-4・低）
7. hkProcess 4.0 isOk/init/step slot 最終確定＋hkProcessHandler 抽象 vt 先頭（0xAA2B58 か 0xAA2B5C か）の ctor store 確定（V-267 §7-2-5＋訂正ドラフト 8・低）

---

## 件1 kind 分布の运行时ログ実測（V-266-1a）— **[未確定]（試行範囲記録・実測不能と確認）**

### 1-1. 結論の枠
- v266 件1 で band 幾何・kind 出生/降格・prio は命令レベルで **[確定] 済み**（窓式 `G≤idx<G+D`・T box `|Δ|≤17`・外周 band `18≤d≤27`・出生 0x484019・降格 0x484129・prio 表 `[0xA45A58]={2,3,4}`）。本件は「运行时ログによる分布裏取り」のみ残点で、**移植実装上は不要**（v266 §1-7 明記）。

### 1-2. 試行範囲（実測不能の確認・`v269_1.py`/`v269_1_out.txt`）
1. **実行環境**: 本環境は PC 版 Oblivion の実行環境を非所有（解析専用 VM・ゲーム実行は契約外）。インストール実態の確認:
   - `D:\Cargo\Ghidra\temp\Oblivion_Steam版\` は未実行のスチーム展開そのまま — `Data\Logs\` 無し、`Oblivion.sav` 等の保存無し、ini は `Oblivion_default.ini` のみ（2018-08-11 タイムスタンプ・書換痕跡なし）。
   - `%USERPROFILE%\Documents\My Games\Oblivion` **存在せず**（`My Games` 直下にも Oblivion 無し）＝このマシンで一度も起動されていない。既存ログ収集は不可能。
2. **ログ出力能力の静的確認（新規）**: 要求スキャン帯（FUN_00483D60 の band 実体 0x483D60..0x484300 付近）に文字列 push・log/print 系呼出が**ゼロ**。関数後方 0x48596D..0x488795 で検出される 12 本の push 文字列はすべて `LoadGame/SaveGame buffer ...`・`GetSaveSize()`・`..\TES Shared\InventoryChanges.cpp` ＝**隣接するセーブ/ロード関数の assert 文**で、LOD 要求ループの出力ではない（`v269_1_out.txt`・E8 宛先 151 件中、帯内で優位な 0x401F00(alloc)/0x401F20(free)/0x41Exxx(pack 索引系) に出力機能なし）。
   - **含意**: 仮に PC 版を起動しても、エンジン自身のログに kind 分布は現れない（instrument 系フックなしでは原理的に測れない）。残点 V-266-1a は「ログ実測」の手段自体が存在せず、**静的導出が唯一にして十分な証拠**と確認。

### 1-3. 判定
> **[未確定]（运行时実測そのものは不能・未実施）** — 実行環境なし（試行 1）＋エンジンに kind ログ出力機構なし（試行 2・静的確認）。distribution 値の追記証拠なし。裁定は v266 §1-7 の静的確定（proxy 実測済み）をそのまま採用し、**残点解消（実測不要と結論）か「実機 instrument 必須の別残点」への読み替え**が正しい後続扱い。新規残点化なし（式は命令から完全導出済みのまま）。

**証拠**: `v269_1.py`/`v269_1_out.txt`、v266 件1 §1-7、ディレクトリ実査（Logs/My Games 不在）。

---

## 件2 packed-索引モード（`[arch+0x194]&8`）の f2 bit31 無効レコード経路（V-266-2a）— **[確定]**（機構・到達性・出荷データ三角照合）＋移植影響なし **[確定]**

### 2-1. bit31 ゲートの完全形 — 「無効レコード = bit31 XOR `[arch+0x194]` bit3」（命令レベル [確定]）

v266 で片隅（0x42E0CE..DE）だけ見えていた XOR 式が、**出荷エンジン内で計 5 箇所に同一イディオムで展開**されていると確定（`v269_2c_out.txt` G1a/G1b・帯 0x42B000..0x432000 全走査）:

```
有効判定: 有効 ⇔ f2.bit31 == [arch+0x194].bit3   （一致で続行／不一致でレコード無視）
          さらに and f2,0x7FFFFFFF ≠ 0（オフセット 0 も無効）
```

| # | ゲート VA | 関数意味 | 不一致時の動作 | バイパス引数 |
|---|---|---|---|---|
| 1 | 0x42C2DE..EF | FUN_0042C2D0=レコード妥当照会（hasFile 系） | `xor al,al; ret 8`（false） | `[esp+8]!=0` で無条件続行 |
| 2 | 0x42D08A..98 | FUN_0042D000=フォルダ索引ハッシュ構築ループ | `jne 0x42d0e0`（そのレコードを索引に載せない） | `[esp+0x14c]!=0` |
| 3 | 0x42DC7A..8D | FUN_0042DB50=全アーカイブ横断のファイル検索 | `jne 0x42ddae`（走査スキップ） | — |
| 4 | 0x42E0C6..D8 | FUN_0042E070=索引オープン（folder/file slot 指定） | `jne 0x42e180`（cleanup→NULL 返却） | — |
| 5 | 0x42E1E3..F6 | FUN_0042E1A0=レコードポインタ直接オープン | `jne 0x42e2ae`（cleanup→NULL） | — |

- オープン系（4/5）は通過後 `and ebx,0x7FFFFFFF` で bit31 を落としてから seeksize 実引数化（0x42E0DE/0x42E1FC）。f1 側サイズは従来どおり `and 0x3FFFFFFF`（0x42E125/0x42E161/0x42E240）。
- **bit31 を XOR ずにそのまま「別テーブル行」のセレクタにする例外経路 1 本**（新規発見・[確定]）: FUN_0042EBC0（0x42EBD1..0x42EC08）はレコードの bit31 で `lea edx,[ebp+eax*8]; add edx,eax; mov esi,[edx*4+0xB338E8]` ＝ **タイプ別アーカイブテーブル（BSS 0xB338E8、18 slot=9×2 行）の行 2（slot 9..17）を指す**。行 2 の書込元は出荷コードに**存在せず**（オープン時書込は 0x42F983/8D の `ecx<9` ループ＝行 0 のみ。行 1 は FUN_0042BE10 の close 時ゼロ化で 0xB3390C を読む箇所があるだけ＝読/クリア専用・`v269_2f/2g_out.txt`）。つまり bit31=1 レコードは「どのアーカイブにも紐づかない＝開けない」構造で、XOR ゲート側の `[+0x194] bit3` も常に 0 → 二重に到達不能。

### 2-2. packed モード設定側（`mov byte[esi+0x194],8` @0x42EF6B）の条件と到達性 — **出荷エンジンでは到達不能 [確定]**

- FUN_0042EE80（アーカイブ初期化・ecx=自 object）: 0x42EF41 で `byte[+0x194]=0` 初期化 → **`[esp+0x17c]`（init 引数 3）が非 0 のときのみ 0x42EF6B で 8 を立る**（`cmp byte[esp+0x17c],0; je skip`@0x42EF61..69 実測）。
- 同関数の呼出経路: `e8(0x42EE80) = {0x42F567} の 1 件・imm32 refs 全セクション 0 件`（vtable 経由なし）。0x42F567 は FUN_0042F4C0（BSA オープン処理）内の列で、直前の push 群は `push ecx; push 0; push 0x40000; push edx`（0x42F55A..64 実測）— **引数 3（packed フラグスロット）はリテラル 0**。
- 帯内 +0x194 の書込は全列挙で: `mov 0`@0x42EF41／`mov 8`@0x42EF6B（本件）／`or 0x10`@0x42F04F／`or 0x20`@0x42F0A6／`or 1`@0x42F481／`and 0xef/0xdf`（0x42C0C6/0x42C14B/0x42C9ED）— **bit3 を立てる他の書込元は .text 全体で 0 件**（帯外ヒットはいずれも Ni 系仮想スロット +0x194 の dword アクセスで別クラスの雑音、byte ゲートとの交わりなし）。
- よって **packed 索引モードは出荷エンジンで動的に起動する余地のない死経路**（引数リテラル 0＋単一呼出元＋ref なし＝構造的不存在証明の部類）。読み方: EE80 の packed 分岐 0x42EFB5→0x42F359 は標準索引パースをスキップし、8 バイト（{+0x180=索引 blob offset, +0x184=size}）を [arch+0x180/+0x184] に読んで終える（0x42F38C/92 実測）。実 blob 展開は FUN_0042C1D0（`mov edi,[ebx+0x180]; mov esi,[ebx+0x184]; call 0x983754` ＝ offset/size でストリーム read）が担い、呼出元は索引遅延構築側 FUN_0042D000/0x42D2A0 の 0x42D0BC/0x42D174 のみ — この遅延読取は `[0xB04460]`（ログ/索引デバッグ byte・FUN_0042BD70 が同一 byte を読むゲート、0x42F3A0/0x42F040/0x42F097 と同じ立哨）非 0 時に短絡する分岐を持つ。bit3=0 ならこれらの経路は XOR ゲートにのみ関与する到達済みコード。

### 2-3. 出荷 17 BSA × distantlod 供給での最終化

- **f2 bit31 Set は全アーカイブ 0 件**（v266_6 §1/§2・TOTALS ヒストグラム `'f1b30': 5221, 'f2b29': 54464, 'f2b30': 1744` に f2b31 カウンタ無し=0、147,629 レコード）。→ 全ゲートで `bit31(0)==bit3(0)` 一致＝**無効レコード判定は出荷データでは一度も発火しない**（命令上は発火可能、実データで 0 件）。
- distantlod（BASE Meshes .lod 921+.cmp 41・SI 相当）供給でも同上（BASE f2 bit31=0・`[+0x194]` は BSA ヘッダ読取モードでのみ設定、v266 §2-3 の loose 供給は索引バイパス）。
- v266-2a の **[推定] を格上げ確定**: 「0x194=8 経路は設定側リテラル 0 で起動不能＋実データ bit31=0 の二重根拠で、移植実装は bit31 を完全無視可」= **[確定]**。破綻パターンは存在しない（bit31=1 レコードを積んだ BSA を読ませても、本エンジンなら索引構築時のスキップと NULL オープンになるだけ＝移植側は 0x7FFFFFFF マスクのみ実装で同挙動）。

### 裁定
> - XOR ゲート 5 点＋別テーブル行セレクタ 1 点（0x42EBC0/0xB338E8 行 2）＝bit31 経路の全貌 **[確定]**
> - packed モード（bit3）は初期化引数がリテラル 0・ref 0・書込元 1 箇所のみで**出荷エンジン到達不能** **[確定]**（不存在証明の部類）
> - 出荷 17 BSA で bit31=0・distantlod 発火なし → **移植影響なし [確定]**（v266-2a の [推定] を解消）

**判定: 解明（V-266-2a 残点解消）**（証拠: `v269_2_out.txt` A1/A2/B、`v269_2b_out.txt` D/E1/E2/F、`v269_2c_out.txt` G1a(25)/G1b(8)/G2/G4、`v269_2d_out.txt` H1/H3、`v269_2f_out.txt` J1/J2、`v269_2g_out.txt` K1/K2、v266_6 §1/§2）

---

## 件3 OI/Statistics 静的 registerProcess（0x946552/0x947632 帯）の呼出経路（V-267 §7-2-1）— **[確定]**（3 命令 thunk を経由、v267 の「e8 0 件」は thunk 見落とし）

### 3-1. 呼出連鎖（本束新規確定・命令レベル）

```
enable ラッパ 0x88B680（ini bHavokDebug 反映・v267 §6 済み）
 ├─ 0x88B6D1 call 0x8CABF0            ; ★ viewer 13 本一括 registerProcess（v267 §6 証拠D の本体・enable 直後）
 ├─ alloc 0x74 → 0x8CB220（socket/registry サブ）→ alloc 0x90 → vdb ctor 0x8CA540
 │    └─ 0x8CA707 call 0x9184F0       ; ★★ 本残点の答え＝3 命令 thunk:
 │         0x9184F0  call 0x9478F0    ; DebugDisplay 登録 → hkRef [0xBA9508]
 │         0x9184F5  call 0x947630    ; Statistics 登録     → hkRef [0xBA9504]
 │         0x9184FA  jmp   0x946550   ; ObjectInspection 登録→ hkRef [0xBA9500]（テール jmp）
 │    └─ 0x8CA713/71F/72B call 0x8CA3C0("DebugDisplay"@0xA99A94 / "Shapes"@0xA96140 / "MousePicking"@0xA99A84)
 │         ; 0x8CA3C0 = 名前列挙エントリ生成（tag 0x13 alloc・0x8B1860/0x8B1950 文字列 copy・失敗時 [0xBA7FC0]++）
 └─ serve 0x8C9E20（port 25001）
```

- 3 登録本体（v259 §5 表の 17 点のうち未帰属だった分）:
  - `0x946550`: `mov ecx,[0xBA950C]; push 0x946520(creator: alloc 0x30 tag0x32→ctor 0x9463E0→+8); push 0xAA2924("ObjectInspection"); call 0x947C50; mov [0xBA9500],eax`（`v269_3_out.txt` L1a 実測）
  - `0x947630`: 同形、creator 0x947600（alloc 0x44→ctor 0x946D90）、名 0xAA2958 **"Statistics"** → [0xBA9504]（`v269_3g_out.txt` Q1）
  - `0x9478F0`: 同形、creator 0x947860（alloc 0x24→ctor 0x9477C0）、名 0xA99A94 **"DebugDisplay"** → [0xBA9508]＝v259 の「二重スロット」の本体（`v269_3d_out.txt` O2）
- 旁系: **DebugDisplay 自己登録のガード列**が ctor 0x9477C0 内 0x9477F3..0x947816 に存在（`[0xBA9508]==0` のときのみ 0x947C50 再登録＋vt store `0xAA2A04/0xAA29EC/0xAA29B8`＝v259 実名の生 handler 実装群）＝bulk 前にオブジェクトが作られた場合の自己修復経路 [確定]。
- 0x8CA707 の直前列（`v269_3f/3g_out.txt` Q4）: hkError `[0xBA7FB0]+0x18(0x1293ADEF,"Visual Debugger"@0xA99ACC)→` バッファ 0x200 stream 生成（0x8BBFB0）→ **"VDB Server instance has been created"**@0xA99AA4 → assert 報告（`.\hkVisualDebugger.cpp` line 0x26）→ **0x9184F0 呼出** → 名前 requests ×3。＝ 0x9184F0 は vdb **ctor 0x8CA540 の末尾ブロック**（int3 区切 0x8CA6E0..0x8CA6E1 跨ぎ・フレーム `add esp,0x210; ret 8`@0x8CA75E）にあり、enable→インスタンス生成時に 1 回だけ走る。

### 3-2. v267 §7-2-1「e8 呼び主 .text 0 件」の訂正理由
- v267 は登録**本体**（0x946560/0x947640=call サイト帯、本体 head 0x946550/0x947630）へ E8 を走査。実際は ①Statistics 本体の呼び主は **0x9184F5（call）**＝ thunk の中（`e8(0x947630)={0x9184F5}` 実測・`v269_3b_out.txt` M1）、②ObjectInspection 本体は **0x9184FA の jmp（テールジャンプ）** link のため E8 走査で 0 件に見えた（jmp-hits 機械走査で 0x9184FA を検出・`v269_3c_out.txt` N2）。thunk 0x9184F0 自体の e8 呼び主は `{0x8CA707}` 1 件。

### 裁定
> OI/Statistics/DebugDisplay の静的 registerProcess = **vdb ctor（enable 直後のインスタンス生成）末尾の 3 命令 thunk 0x9184F0 経由**（13 viewer bulk は enable ラッパ 0x88B6D1→0x8CABF0 で別枠・既存確定と整合）。game 起動時 CRT 列でも hkServerProcessHandler ctor でもない。factory refs 22 点のうち未帰属 4 点（0x946552/0x947632/0x9477FE/0x9478F2）は上記 3 本体＋ガード列に完全帰属。**残点解消 [確定]**。
> 付随発見（件4 接続）: vdb ctor が名前列挙に **"DebugDisplay"/"Shapes"/"MousePicking"** を要求リスト化 — MousePicking の名前文字列は 0xA99xxx server 帯に存在（v267 §7-2-2 の「分散」予測どおり）。

**判定: 解明**（証拠: `v269_3_out.txt` L1a/L1b/L2/L3、`v269_3b_out.txt` M1/M2、`v269_3c_out.txt` N1/N2/N3、`v269_3d_out.txt` O1/O2/O3、`v269_3f_out.txt`・`v269_3g_out.txt` Q1〜Q4）

---

## 件4 WorldMemory/MousePicking/Tensor の viewer 実名探索（V-267 §7-2-2）— **[未確定]（確認できず＝不存在限定証明）**＋名表残りの全量確定

### 4-1. Tt 名表プールの完全列挙（`v269_4_out.txt` R1・0xA9D200..0xA9DB00 の 43 文字列を全走）
- hk*Viewer 実名 9 ＋メソッド反射名 `sendProps/getProps/display3lines/getMassAndLines/draw` は v267 §2 で確定済（本走査で矛盾なし・再現）。
- **新規捕捉（残り分）**: `ShapeDisplayViewerOptions`@0xA9D584（hkShapeDisplayViewer の options 型名・TU 名表内に実在）。0xA9DA2C 以降は無 Tt のフィールド反射名 12 本（actions/entityListeners/activationListeners/collisionListeners/internalCollideFlag/isFixedOrKeyframed/fixed/autoRemoveLevel/processContactCallbackDelay/storageIndex/constraintRuntime/constraintsSlave）＝**hkEntity/hkRigidBody 属性列挙プール（ObjectInspection/Statistics 用）**で viewer 名ではない。
- 登録名だけあって viewer 名がない 3 対象: `'I@WorldMemory'`@0xA9D242（登録名 "WorldMemory"@0xA9D244・refs .text@0x918DFC＝WorldMemory 登録関数の push・`v269_4c_out.txt` T3）、`'I@Tensor'`@0xA9D326、`MousePicking`@0xA99A84（server 帯＝vdb ctor の requests 名・件3）。

### 4-2. 3名の不存在機械証明（字符串地域・負証拠は「本 EXE 文字列に無し」）
- 全イメージneedle走査（`.rdata+.data` 0xA28000..0xB02000・`v269_4_out.txt` R2）: `TthkWorldMemory` 0・`TthkMousePicking` 0・`TthkTensor` 0・`WorldMemoryViewer` 0・`hkTensor` 0・`MousePicking` 1（=0xA99A84 登録/requests 名のみ）。
- `WorldMemory` の 2 度目の出現 0xAA3046 は **`hkWorldMemoryWatchDog`**@0xAA3044 の部分文字列（physics 側のメモリ watchdog クラス名・`memoryLimit`@0xAA3038 と同居する別反射プール）＝viewer 名の証左ではない（`v269_4c_out.txt` T1b）。
- `.\visualdebugger\viewer\*.cpp` パス文字列は既存 4 本（ST/CR/SD/PD）のみで WM/MousePicking/Tensor の TU パスは存在せず（`v269_4b_out.txt` S1）。
- 旁証（新規）: `TtPickObject`@0xA96110 はゲーム側プロファイルリングの**イベント名**（0x88B94E が ring エントリ 12B {name,rdtsc,...} へ imm store・TLS `[ebx+0x1a4]/[+0x1a8]` 周回・`v269_4c_out.txt` T2/T3＋0x88B900..0x88B976 ダンプ）— Bethesda 側 PickObject 系計測名であり、Havok MousePicking viewer の実名ではない（混同注意）。

### 4-3. 5.1 公開ソース側の対照（一次資料・本束取得）
- `Source/Common/Visualize/Process/` = **hkDebugDisplayProcess / hkInspectProcess / hkStatisticsProcess の 3 組のみ**。`Source/Common/Visualize/` 直下＝CommandRouter/DebugDisplay(Handler)/DrawUtil/Process(Context|Factory|Handler|RegisterUtil)/ServerDebugDisplayHandler/ServerProcessHandler/VersionReporter/VisualDebugger(DebugOutput)/Visualize ＋ Serialize/Shape/Type サブ目录。**WorldMemory・MousePicking・Tensor に対応するファイル名は 5.1 ツリーに存在しない**（4.0 専用/5.x で除去・改名）。よって外部資料からの実名確定も不可（v267 §7-2-2 の「5.1 listing 参照可」の到達点＝不在確認）。
- 4.0 命名規則（hk<登録名>Viewer 系・v267 §2 で 9 実名が該当）からの類推は **hkWorldMemoryViewer / hkTensorViewer / hkMousePickingViewer [推定・低]**（本 EXE に文字列実在せず）。ObjectInspection/Statistics の 5.1 相当名が `hkInspectProcess`/`hkStatisticsProcess`（今 listing で実証）＝4.0→5.1 の Viewer→Process 改名系列にも整合。

### 裁定
> **3 対象の viewer 実名 = [未確定]（確認できず）** — 不存在証明は「本 EXE 文字列地域＋5.1 ツリー listing」の範囲で完了（RTTI 非生成領域のためイメージ全体証明ではない）。機能名は v259/267 運用維持（WorldMemory=メモリ統計・Tensor=物理テンソル表示・MousePicking=VDB ピック）。Tt 名表の「残り」は本束で全量列挙し尽した（新規実名は ShapeDisplayViewerOptions とフィールド反射名 12 のみ）。残点は「4.x SDK 配布そのもの」が無い限り解けず——**次バッチ候補から外して可**（素材不在の恒久未確定に格下げ）。

**判定: 探索完了（3名とも「確認できず」を機械確定・残り名表を全量消化）**（証拠: `v269_4_out.txt` R1/R2、`v269_4b_out.txt` S1〜S4、`v269_4c_out.txt` T1〜T3、GitHub listing 2 本）

---

## 件5 enable ラッパ `0x88B680` 呼出 6 点のゲーム側イベント同定（V-267 §7-2-3）— 各点 owner 関数 **[確定]**／イベント意味 **[推定]**（起動時にヒットする点は静的導出で確定）

### 5-1. 各呼出点の帰属（`v269_5/5b/5d_out.txt` 実測・arg=push 値）

| site | owner（head） | 押す引数 | owner の意味（命令証拠） | 呼出主（e8 実測）とイベント |
|---|---|---|---|---|
| 0x445628 | FUN_004455E0（+0x48 head 確認） | **0**（disable） | 冒頭 `[0xB43384]=1`・`[0xB43074]=1`・`[esi+0x51]/[+0x52]` ガード→`0x40D160`、`0x7B7650→0x7AB6F0`（サスペンド連鎖）、`[[0xB35C24]+0x19]=0` | ①0x40ED72＝FUN_0040D4D0（ロード・セーブの巨大状態機械: bUseMyGamesDirectory/CopyProtection/プール初期化帯と同梱・`0x4C97F0` 判定分岐）②0x4461ED（jmp table 帯からの分岐・`[ebp+0x34]` で enable 側 0x446201 と排他選択）③0x66EC26＝FUN_0066EAF0＝**プレイヤーテレポート関数**（s56 確定済「0x66EAF0 で jail マーカーへ teleport」・b15 で caller 15 の座標転送ハブ）＝**転送中の物理/VDB サスペンド** [確定] |
| 0x445B87 | FUN_00445A10 | **[0xB097B8]**（ini 値で enable） | 0x4455E0 と対（`[0xB43074]=0` のみ差分）、末尾 `0x45A500` | ①0x40EC6B（状態機械 resume）②0x446201 ③**0x463000＝FUN_00462B20（セーブ書込本体・'TES Class'/'Process Lists Class'/'Sky/Weather'/'Could not create save buffer' chunk 列挙関数**）— 0x4CECD0 直後の resume 列 ④0x66ED00（テレポート完了側）＝**セーブ/転送後の VDB 再開（ini 値反映）** [確定] |
| 0x4D4C27 | FUN_004D4A20 | **[0xB097B8]** | 0x4CD2D0/0x4CB7F0/**0x88D260（NiHavok 帯）** 呼び出し後の enable・関数内に 'Player'@0x4D4EFD | 単一呼び主 0x4D5A65＝**FUN_004D58B0**（world セットアップ/_tick_ 帯関数。その呼び主 {0x441DF9,0x4421F7,0x44463F,0x444660,0x445829,0x4458E8}＝メインループ帯、+0x4BDFD3,0x4D63A5,0x4D64AB）＝**ワールドシミュレーション設定完了時 enable** [確定・帯]／イベント語は [推定中] |
| 0x4D4DDD | FUN_004D4DC0（25B ヘルパ・`push edi;mov edi,ecx;test[edi+0x24],1`） | **0** | `[+0x24]&1` 時 `0x424180([+0x28])` の返り物限定で disable | 単一呼び主 0x4D64A4（FUN_004D63A0 帯＝world rebuild 末尾列）／兄弟 0x4D4D00 の呼び主＝0x482358（world 反復ループ）/0x4D5D55（FUN_004D5BD0 tick）/0x4D63E5 ＝**ワールド再構築中のサスペンド分岐** [推定中] |
| 0x4D52D0 | FUN_004D5000（SEH・0x88A4F0 呼出・fld [0xA46B20]） | `sete` = **[0xB33A34]==0** | 「ローディング/シャットダウンフラグ」[0xB33A34] の非立時にのみ enable（=loading flag 同期） | 単一呼び主 0x4D562E＝FUN_004D5600（**その head の e8/refs 0 件**＝vtable/コールバック登録経由の可能性・追跡 [未確定]）＝**ロード要求同期** [推定] |
| 0x4D6401 | FUN_004D63A0（0x4D58B0 呼出＋`[+0x26]=5`＋虚 `[+0x84](1,edi)`＋`[+0x19]=![0xB33A34]`） | **[0xB097B8]** | ワールド再構築完了列の末尾 enable | 呼び主＝FUN_004D5D70 の 4 点 **{0x40EED4（グリッド状態機械: `[0xB06A2C]>>1`＝uGridsToLoad/2 を push して 0x482150→ループ内 call ＝セルグリッド構築）、0x460015（FUN_0045FDA0=セーブ読込・'Savegame loading error...'）、0x4E2391（FUN_004E2160・'Disabling collision on ref %s (%08X)'＝読込後物理整理）、0x66FE74（テレポート帯 FUN_0066FD90）}** ＝**通常起動・ロード・転送のたびにヒット [確定]**（FUN_004D5D70 は cell 座標×0x1000+0x800 換算＝bhkWorld 生成帯） |

### 5-2. 「通常起動でヒットする点はどれか」（v267 §7-2-3 の問い・実機ログ不使用で静的回答）
- **必ず通る**: ①0x4D6401（初回ワールド/グリッド構築で FUN_004D5D70＝enable×ini 値）②0x445B87（状態機械 resume・セーブ書込後も）。テレポートするたびに ①②の両系統＋0x66EC26/66ED00 ペアが追加発火。
- **条件付き**: 0x4D4C27/0x4D4DDD/0x4D52D0 は world tick/再構築・loading flag の状態分岐内のみ（立哨 `[+0x24]&1`・[0xB33A34] 実測）。
- 0x88B680 自体は idempotent（`[esi+0x14]!=0`＝生成済分岐 0x88B6B7、v267 §6）→ 毎フレーム再 enable は安全。よって**実機ログ無しでも「初回ロード時に 0x4D6401 経由で VDB server が立つ」順序は確定**（bHavokDebug=1 時）。
- v267 記述の訂正 2 点（ドラフト表記）: ①「0x4D4C27 は push 0」→ **実測 push [0xB097B8]（enable 側）**。②「0x4D52D0 側は push 0」→ **実測 `sete`（=[0xB33A34]==0 のとき 1）**。Load/SaveQueuedHavokData 語は 0xA3A0A4='HavokData() call did not properly empty buffer.'・0xA3A847='HavokData' の実体名であり、両者の imm32 refs は .text 0 件（直接参照なし＝この帯との紐付けは v267 の語感由来、本束で否定）。

### 裁定
> 6 点すべて owner 関数确定。うち **0x4D6401（ワールド構築完了）と 0x445B87（resume）が通常起動/ロードで毎回ヒット [確定]**、0x445628（サスペンド）はセーブ書込・テレポート・状態機械 pause 分岐 [確定級]、0x4D4C27/0x4D4DDD は world tick/再構築帯 [推定中]、0x4D52D0 の親 FUN_004D5600 は登録呼出経路 [未確定]。**実機ログ要证の部は解消**（静的で起動順序確定）。

**判定: 解明（6/6 帰属・うち 1 点は呼出登録経路が残微残点）**（証拠: `v269_5_out.txt` U1〜U3、`v269_5b_out.txt` V1〜V3、`v269_5c_out.txt`、`v269_5d_out.txt` W1〜W3、s56/b15 cache ラベル）

---

## 件6 +0x20（B 系 2 空仮想）のメソッド名・型名（V-267 §7-2-4）— **[確定]（機構・使用方法）＋型名自体は [未確定]（素材不在）。「process context observer」候補 = observer/data writer として正しい（hkProcessContext ではないと判明）**

### 6-1. コンテキスト選択機構の発見（本束最重要・命令レベル [確定]）
- process 家系の基底 ctor `0x9491F0`（`v269_6b_out.txt` Y1）: `[+6]=refcount 1`→`[+8]=0xA9D1C0`（hkProcess part 抽象）→`[+0xC]=1`→`[+0x20]=0xAA2B9C`（抽象）→`[+0]=0xAA2BBC`/`[+8]=0xAA2BA4`（具象上書）→`[+0x20]=0xA9D250`（既定上書）→`[+0x24]=0`、**その後 arg1 `{ptr,count}` 配列（vdb の contexts 列＝v267 §6 `[+0x18]` 自 object）を周回し `call [[ctx_i]+4]` → 返り文字列と `"Physics"`@**0xA99BD8** を `0x8B1770` 文字列照合、一致した ctx を `[self+0x24]` に保持**（word[+6] refcount 増減列 0x9491A9..0x9491C1 実測）。
- ⇒ **5.1 `hkProcessContext` との意味一致 [確定（相当）]**: 5.1 ヘッダ（本束全文取得）は「`virtual const char* getType() = 0` … Common ones are **'Physics'` or 'Animation'`」— 本 EXE の選択ループが slot1（+4）返り値を "Physics" と照合するのと逐語対応。コンテキスト実体はゲーム側（外部）クラスで viewer の `[+0x24]`、**+0x20 ではない**。
- 5.1 hkProcessContext の仮想は {~dtor, getType pure, setOwner} の 3 つ。**+0x20 の B part（2 仮想）とは別物**（v267 §7-2-4 の「process context observer 候補」は **observer/data-writer 部として的中、型帰属としてはハズレ** と裁定）。

### 6-2. +0x20（B part）の実体 = VDB display data writer（feed＋entity 検索の 2 仮想）
- 既定 inst `0xA9D250 = {0x9266A0, 0x9266A0}`（空 ret4×2）、抽象 `0xAA2B9C = {purecall×2}`（v267 §2 維持）。
- **使用方法の実測（新規）**: WM process slot4 `0x91DF60`（`v269_7_out.txt` S1）= `[part+0x1C]`＝**viewer+0x24 の ctx** の entity 配列 `[ctx+0x5C]/[+0x60]` を周回し **`[[viewer+0x20]+0]`（B slot0）へ entity を 1 個ずつ feed**（thiscall・引数=entity ptr）。ST は slot4 `0x91B110` で同周回→`0x91AC60`（=B slot0 実装先・+0x28/+0x2C を束ねて `0x899CA0`（物理 query/world）へ）。
- **ST 派生表 `0xA9D418`（新規確定・ST ctor 0x91B225 store 実測）= {`0x91B150`(=`sub ecx,0x20; jmp 0x91AC60` adjust thunk＝feed/display 実装), `0x91B2A0`(＝entity 配列線形探索: `[ctx+0x5C]` の各要素 `*elem` と引数 id/ハンドルを比較→index 返却, `ret 4`)}**。v259「ST 派生 slot1 がエンティティ配列線形探索」の本体命令列がこれ。
- B part の基底 inst ctor `0x948D00`: {word+6 refcount=1、[+0]=0xAA2B8C（primary 2 cell {0x949150, 0x68F970}）、[+8]=0xAA2B5C→0xAA2B74、+0x10 カウンタ、+0x14 count、**+0x18 hkPointerMap**（0x8B0E10・16 桶 tag0x14）、+0x1C バッファ {ptr,ptr,size &0x3FFFFFFF}、+0x24 owner（0x948CB0=setOwner 相当）} — ハンドル→データ写像＋内部 grow バッファを持つ **serialize/writer クラス**（隣接実装 0xAA2B74 slot1 `0x948E80` が size vs `[buf]+0x46` の書空間判定＝stream append 型ロジック）。
### 6-3. 判定
> - **+0x20 の型 = 「VDB display data writer（エンティティ feed＋ハンドル検索の 2 仮想）」相当 [確定]**（上記 ctor/使用命令列）。4.0 固有名・メソッド実名は **文字列素材ゼロ [未確定]（不存在は文字列地域のみ証明済）**。機能名運用推奨: writer slot0=`addEntity/display`、slot1=`entityToIndex/handleToEntity 系` [推定（命名・中）]。
> - v267 §2 の `sendProps/getProps`（+0x2C 側）割当は不変。+0x20 と hkProcessContext の関係は **ctx→[+0x24] 保持**（+0x20 は writer）と訂正（ドラフト: v267 §7-2-4「process context observer 候補」→「observer(data writer) として正しい。context 本体は +0x24・"Physics"照合で選択 [確定]」）。
> **残点解消の扱い**: 機構・使い方・slot 意味＝確定。型名/メソッド名の実名確定は素材不在（件4 と同じ恒久未確定に格下げ・次バッチ不要）。

**証拠**: `v269_6_out.txt` X1〜X5、`v269_6b_out.txt` Y1（0x9491F0 全体）/Y2/Y3（0x948D00 群）、`v269_6c_out.txt` Z1（"Physics" 実測）/Z2b（0xA9D418）/Z3、`v269_7_out.txt` S1（0x91DF60/0x91B110）。

---

## 件7 hkProcess 4.0 slot 最終確定＋hkProcessHandler 抽象 vt 先頭（V-267 §7-2-5・訂正ドラフト 8）— **[確定]**

### 7-1. hkProcess 4.0 = 6 slot・**isOk/init(context) は存在せず**（実装本体の引数形が決定的）
基底 `0xA9D1C0` = {`0x918500`, purecall, `0x918CD0`, `0x9266A0`, `0x918000`, `0x9266A0`} — slot 意味は具象 2 クラスの本体命令形で最終確定:

| slot | byte off | 基底既定 | 実形 | WM 具象 | ST 具象 | 最終意味 |
|---|---|---|---|---|---|---|
| [0] | +0x00 | `0x918500` | `[+4]`test→復元→delete | `0x918D00`（thunk→`0x918E10` scalar del） | `0x91AA80` | **~hkProcess（D2/D3）** |
| [1] | +0x04 | purecall | 抽象純粋 | `0x918CF0` = `mov eax,[0xBA8410]; ret`（**自 factory entry を返す**） | `0x91AA70` | **getProcessTag**（4.0 の tag＝登録 entry。WM の返す [0xBA8410] は 0x918DF0 登録の保存先と同一・Y2 実測） |
| [2] | +0x08 | `0x918CD0` | `mov[eax],0;mov[ecx],0;ret 8` | 同一（共有） | 同一 | **getConsumableCommands（5.1 既定体と逐語一致・v267 §2）** |
| [3] | +0x0C | `0x9266A0` | 空 `ret 4` | 同一 | 同一 | **consumeCommand(cmd)（5.1 既定 `{}`）** |
| [4] | +0x10 | `0x918000` | **`ret`（引数ゼロ！）** | `0x91DF60`（ctx entity 配列周回→B writer slot0 へ feed・末尾 `ret`） | `0x91B110`（同型→`0x91AC60`・末尾 `ret`） | **step()/frame hook（無引数）** |
| [5] | +0x14 | `0x9266A0` | 空 **`ret 4`（1 引数）** | `0x918D60`（ctx の entity 数+「World」@0xA30058 等を `[+0x28]` stream part へ書き出す・`ret 4`） | `0x9266A0`（既定＝stream 書きなし） | **step/append(stream) 系・1 引数。呼出側 `0x918760`: `call dword[edx+0x14]`＋引数 push（`v269_7_out.txt` S1 実測）** |

- **裁定理由 [確定]**: slot4 の既定 `0x918000` と WM/ST 具象はいずれも **plain `ret`（thiscall 無引数）** — 1 引数の thiscall は MSVC で必ず `ret 4` のため **init(hkProcessContext*) の余地なし**。コンテキストは ctor `0x9491F0` で到着済（件6）。slot5 既定/WM/呼出側は `ret 4`＋push 1 ＝ストリーム系 1 引数。**5.1 hkProcess.h の 7 仮想 {~dtor, getProcessName, getConsumableCommands, consumeCommand, isOk, init, step} に対し 4.0 は {…, isOk, init} を持たない 6 仮想**（v267 §2 の「5.1−isOk」を **5.1−isOk−init** に訂正。slot4=step・slot5=stream-append で 5.1 の isOk/init/step は「4.0 の step/append に対応、5.1 で init/step に再編」[推定（高）]）。
- 0x918000 の再利用（node2 vt `0xA98234` slot2・hkProcess slot4・B/handler 実装表 0xAA2B74 slot5）＝**同一の無引数 no-op**＝4.0 では frame hook 群が ICF 共有。boot④「node2 slot2 no-op 呼出」（v267 §3）もこれで整合。

### 7-2. hkProcessHandler 抽象 vt 先頭 = **`0xAA2B5C`** [確定]（0xAA2B58 でない）
- **store 元は 1 箇所のみ**: `0x948D18`（B/writer base ctor 0x948D00 内 `mov [esi+8], 0xaa2b5c`、直後 0x948D25 で具象 `0xAA2B74` に上書）→ imm32 位置 `0x948D1B`＝refs(0xAA2B5C) の唯一ヒット。
- `refs(0xAA2B58) = 0`（再実測）。0xAA2B58 の値 `0x68F970`（空 ret4）は **vdb 入力ストリーム B クラス vt `0xAA2B54 = {0x948C50(D3), 0x68F970(slot1)}` の slot1**（§4 の A/B 帰属と整合、別クラスのセル）。
- 抽象表の形状 = **purecall×6（0xAA2B5C..0xAA2B70）の 6 slot**、具象 `0xAA2B74 = {0x948CA0(ret 0xc), 0x948E80, 0x948F60, 0x949070, 0x949110, 0x918000}` の 6 cell（境界は primary 表 `0xAA2B8C = {0x949150, 0x68F970}` の store 0x948D1F と float データ 0xAA2B94 で閉じる）。**「空 ~dtor+pure×6」の 7 cell 形（v267 訂正ドラフト 8 の A 案）は棄却** — dtor cell 持たず、purecall の直列が先頭 0xAA2B5C。
- 帰結（訂正ドラフト v267 §7-5-8）: 「hkProcessHandler 抽象 vt は 0xAA2B58 または 0xAA2B5C」→ **0xAA2B5C（6 slot・store 元 0x948D18 一点）**。この抽象は B/writer base の +8 part（serialize 系基底）に埋込まれるもので、handler 系 13 cell 表（0xAA2984/0xAA29B8・§1）とは別族。

**判定: 7 件完遂（件1 未確定確認／件2・3・5・6・7 解明／件4 素材不在の恒久未確定確定）**（証拠: `v269_7_out.txt` S1/S2＋`v269_6b/6c_out.txt`）

---

## §8 まとめ・自己検証・新規残点・訂正ドラフト・使用証拠

### 8-1. 本束 7 件の判定一覧

| # | 残点（出典） | 判定 | 核心 |
|---|---|---|---|
| 1 | kind 分布の运行时ログ実測（V-266-1a） | **[未確定]（実測不能と確認・残点解消扱い）** | PC 実行環境なし（Data\Logs 無し・My Games 無し）＋**エンジン要求ループに kind ログ出力機構なし**（帯 0x483D60..0x484300 に push 文字列 0・検出文字列は全て後方 0x48596D.. のセーブ/ロード assert）。式は命令から確定済（v266）で分布ログは原理的に存在しない |
| 2 | packed-索引モード f2 bit31 経路（V-266-2a） | **[確定]**（[推定]→格上げ） | 有効判定 `bit31(f2) XOR bit3([arch+0x194])` の 5 ゲート（0x42C2DE/0x42D08A/0x42DC7A/0x42E0C6/0x42E1E3）＋ bit31→タイプ別表 0xB338E8 の**行 2（slot9..17）セレクタ**（FUN_0x42EBC0）。packed フラグ書込元は `0x42EF6B`（EE80 引数 gate）のみ、EE80 の唯一呼出 0x42F567 は**リテラル 0 push**＋ref 0＝**出荷エンジンで到達不能**。出荷 17 BSA bit31=0 全件（v266_6）→発火 0 件。**移植は bit31 完全無視可 [確定]** |
| 3 | OI/Statistics 静的 registerProcess 呼出経路（V-267 §7-2-1） | **解明 [確定]** | **3 命令 thunk `0x9184F0`**（call 0x9478F0 DebugDisplay→[0xBA9508]／call 0x947630 Statistics→[0xBA9504]／**jmp** 0x946550 ObjectInspection→[0xBA9500]）＝**vdb ctor 0x8CA540 の末尾 0x8CA707** から呼出。v267 の「e8 0 件」は thunk と tail-jmp 見落とし。続けて 0x8CA3C0 で requests 名 DebugDisplay/Shapes/MousePicking を列挙 |
| 4 | WM/MousePicking/Tensor viewer 実名（V-267 §7-2-2） | **[未確定]（確認できず＝文字列地域で不存在証明）** | Tt 名表プール全量 43 文字列列挙済み（新規: `ShapeDisplayViewerOptions`@0xA9D584・フィールド反射名 12）。`TthkWorldMemory/TthkMousePicking/TthkTensor` 0 ヒット・viewer TU パス 4 本のみ。'WorldMemory'@0xAA3046=**hkWorldMemoryWatchDog**（別クラス）と判明。5.1 Visualize/Process = DebugDisplay/Inspect/Statistics 3 組のみ＝WM/MP/Tensor 系は 5.1 に不在（実名確定素材は恒久欠如）。`TtPickObject`@0xA96110 はゲーム側プロファイル ring イベント名（0x88B94E）と併記 |
| 5 | enable ラッパ呼出 6 点のイベント同定（V-267 §7-2-3） | **解明（6/6 帰属）** | 0x445628=サスペンド系 helper（テレポート FUN_0066EAF0・状態機械 0x40D4D0・pause 分岐）、0x445B87=resume（セーブ書込 FUN_00462B20@0x463000 含む）、0x4D4C27/0x4D4DDD=world tick/再構築帯（FUN_004D58B0/FUN_004D63A0）、0x4D52D0=loading flag 同期（親 FUN_004D5600 の呼出経路は微残点）、**0x4D6401=ワールド構築完了（cell×0x1000+0x800 換算帯・呼び主 {グリッド状態機械 0x40EED4, セーブ読込 0x460015, cell 後処理 0x4E2391, テレポート 0x66FE74}）＝通常起動/ロードで毎回ヒット [確定]**。v267 の push 値 2 点誤記を訂正（§8-4） |
| 6 | +0x20（B 系 2 空仮想）の名・型（V-267 §7-2-4） | **機構 [確定]／実名 [未確定]（素材不在）** | +0x20 = **VDB display data writer**（feed slot0／entity ハンドル線形検索 slot1・ST 表 0xA9D418={0x91B150, 0x91B2A0}、基底 ctor 0x948D00・hkPointerMap@+0x18・owner@+0x24）。**hkProcessContext 本体は +0x20 ではなく ctor 0x9491F0 が contexts 配列から slot1()==`"Physics"`@0xA99BD8 照合で `[self+0x24]` へ保持**（5.1 getType と意味一致）。observer 候補は「data writer として的中／context 帰属は棄却」 |
| 7 | hkProcess isOk/init/step＋handler 抽象先頭（V-267 §7-2-5・draft8） | **[確定]** | hkProcess 4.0 6 slot の最終形 = {~dtor, getProcessTag（pure→WM 具象 0x918CF0 は factory entry [0xBA8410] を返す）, getConsumableCommands ret8, consumeCommand ret4, **slot4=無引数 step/frame hook（既定 0x918000 `ret`・WM 0x91DF60・ST 0x91B110 が ctx entity 周回 feed）, slot5=1 引数 stream append（既定 0x9266A0 ret4・WM 0x918D60・呼出 0x918760 `call[+0x14]`）**}。**isOk も init(context) も存在せず**（thiscall ret 形が決定証）。context は ctor で到着（件6）。handler/writer 抽象表先頭 = **0xAA2B5C（purecall×6・6 slot）**・store 元 0x948D18（ctor 0x948D00）1 点、refs(0xAA2B58)=0 再確認＝**0xAA2B58 は vdb 入力 stream B の slot1**（7 cell 形は棄却） |

### 8-2. 新規残点（V269・次バッチ候補）
- V-269-1 [低]: `'HavokData'`@0xA3A847・`'HavokData() call did not properly empty buffer.'`@0xA3A0A4 の参照関数（imm32 refs .text 0 件＝参照経路不明・件5 で紐付けできず）。
- V-269-2 [低]: FUN_004D5600（enable site5 0x4D52D0 の親）の e8/refs 0 ＝呼出登録（vtable/関数表）経路の帰属。
- V-269-3 [低]: タイプ別表 0xB338E8 の行 2（slot9..17・bit31=1 レコード解決用）を書こうとする外部経路の有無（出荷コードに writer ゼロ＝移植には不要）。
- 恒久未確定（素材不在・次回以降不要）: WM/MousePicking/Tensor viewer 実名（件4）、+0x20 writer クラスと vdb B/writer 抽象基底の 4.0 実名（件6/7）、A/B stream・D・router 等の 4.0 型名（v267 既知）。

### 8-3. 自己検証
- **selftest**: 着手時 **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 → ALL PASS）。
- **VA 換算・走査**: `v172_common（va2off/rd/dump/e8_callers/imm32_refs3）・v249_util・v217_util（fname/cstr）` に統一。capstone 線形スキャンの invalid-byte 早期停止は byte 検索＋window 再同期で回避（教訓: 全 .text 線形 1 発走査は不可、 disp32 `94 01 00 00` 検索が正）。
- **数値の相互整合**: 件2 の XOR ゲート 5 点と v266 §2-2 bit30 XOR の同形性／件3 の保存先 [0xBA9500/04/08] と v259「17 点=16 グローバル+0xBA9508 二重」／件5 の FUN_004D5D70 cell 換算（`shl 0xc`+0x800）と LOD グリッド 0x1000 セル／件6-7 の ctx@+0x24→件7 slot4 周回 `[ctx+0x5C/0x60]` の連続使用／件7 呼出側 0x918760 と v267 §5 の slot5 記述（実測 `call [edx+0x14]`）。
- **負証拠の区分**: 構造的不存在＝packed writer 単独・arg リテラル 0・refs(0xAA2B58)=0・行 2 writer ゼロ。文字列地域不存在＝WM/MP/Tensor Tt 名・+0x20 名前文字列。単なる未発見＝FUN_004D5600 経路・HavokData 参照。
- **制約遵守**: git 書込み 0／docs・GT・TODO・plan・session-memory・cache・ソース編集 0（cache は読取のみ）／既存 tmp/re 上書き 0（v269 新規・指定名厳守）／ESM 再スキャン 0／PowerShell ヒアドキュメント不使用・`python -c` で正規表現不使用（スクリプトは `%TEMP%\opencode\v269_*.py`）。リポジトリ側ループ運用指示（STATE.md/L2/worktree/テスト）には**不従**（契約=分析のみ）。
- **中断対策**: 件1→件2→件3→件4→件5→件6+7 の各完了直後に本体へ追記（全 7 件追記済・本節で最後）。

### 8-4. 既存記述の訂正ドラフト（本体は未編集・反映は承認待ち）
1. **v266 §2-3（V-266-2a [推定]）**: 「0x194=8 経路は distantlod で発火せず [推定]」→「**設定側リテラル 0・呼出 1 点・実データ bit31=0 の三重で [確定]**。bit31 は XOR ゲート＋タイプ別表行選択専用、移植は不要」（件2）。
2. **v267 §6 証拠D/§7-2-1**: 「OI/St 静的群の呼び主 .text 0 件＝別経路」→「**thunk 0x9184F0（call×2＋jmp×1）経由で vdb ctor 0x8CA540@0x8CA707 が呼び主 [確定]**。E8 走査は tail-jmp を捕らえない」（件3）。
3. **v267 §6 証拠C**: 「0x445628/0x4D4C27/0x4D4DDD/0x4D52D0 側は push 0」→「**0x4D4C27 は push [0xB097B8]、0x4D52D0 は push sete([0xB33A34]==0)**（実測）。push 0 は 0x445628/0x4D4DDD のみ」（件5）。
4. **v267 §5 副産/§7-2-5**: 「hkProcess 4.0 = 5.1 の 7 仮想−isOk の 6 slot」→「**−isOk−init の 6 slot。slot4=無引数 step（既定 0x918000 `ret`）、slot5=1 引数 stream append（既定 0x9266A0 `ret 4`）。context は ctor 0x9491F0 で到着**」（件6/7）。
5. **v267 §2 証拠D/§7-2-4**: 「+0x20 = process context observer 候補」→「**+0x20 = display data writer（feed/検索）。hkProcessContext 実体は ctor の "Physics"@0xA99BD8 照合で [self+0x24] に保持**」（件6）。
6. **v267 §7-5-8（ドラフト 8）**: 「抽象 vt 先頭 0xAA2B58 か 0xAA2B5C」→「**0xAA2B5C（purecall×6・store 元 0x948D18 の ctor 0x948D00）で確定。0xAA2B58 は vdb 入力 stream B（0xAA2B54）の slot1 で表頭ではない**」（件7）。

### 8-5. 使用証拠一覧
- **本体**: `tmp/re/v269_lod_havok_bundle.md`（本ファイル・新規）。
- **入力（読取のみ）**: `tmp/re/v266_object_lod_residuals2.md`・`v267_process_havok_residuals14.md`・`v259_*`・`v258_*`（前提引用）・`v255_15_out.txt`（帯ログ能力確認）・`v266_6_out.txt`（bit31=0 実測）・`tmp/re/cache/named_funcs.txt`（読取）・既存 `%TEMP%\opencode\v266_*/v267_*`。
- **本束中間物**（`%TEMP%\opencode\`）: `v269_1/2/2b/2c/2d/2e/2f/2g/3/3b/3c/3d/3e/3f/3g/4/4b/4c/5/5b/5c/5d/6/6b/6c/7.py(_out)`。
- **EXE**: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7）。
- **一次資料（外部）**: GitHub `sigmaco/havok-v5.1.0r1-b2007.09.19`（branch main）`Source/Common/Visualize/hkProcessContext.h` 全文・`Source/Common/Visualize/` と `Source/Common/Visualize/Process/` directory listing（2 本）。

**判定（束全体）: 7/7 処理完了 — 実質解明 4（件2・3・5・7）＋機構解明 1（件6）＋恒久未確定化 1（件4）＋手段不存在の確認 1（件1）。v266/v267 に対し残点解消 3・訂正ドラフト 6・新規残点 3（低）**
