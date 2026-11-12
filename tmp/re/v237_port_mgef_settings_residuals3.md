# v237_port_mgef_settings_residuals3.md — 移植 MGEF/設定 残点第3束（6件）

- 作成: 2026-11-02 / 担当: TODO serial 230（V230・低）
- 出典本体: `tmp/re/v236_port_mgef_settings_residuals2.md` §6 新規残点 N-9..N-14（本体は読取のみ）
- 手法: 移植ソース**読取のみ**（`app/src/main/cpp/` 編集なし）＋ EXE 再逆アセンブル（capstone 5.0.7・PE 手動解析・**ImageBase 補正版**）＋ 既存 RE 成果物（v233/v235/v236・cache）の突合 ＋ Copilot `events.jsonl` 走査（読取のみ）＋ 日付誤り grep 対象（TODO/plan/STATE/記憶台帳 — **いずれも読取のみ**）。git は不使用（本タスクで必要な git 検査は v236 §4-2 で完備と判断）。ESM 本体は未アクセス（再スキャン禁止を遵守）。
- 結論早見: 項目1 **【解明】誤記は TODO.md（根/.ai-todo ミラー）2箇所×2本＋plan.md 2箇所＋supermemory 1件の計 7 ポイント＝実 3 ファイル 6 行＋記憶 1 件。STATE.md と session-memory.json は該当なし（別案件の正 date／#26・#29 台帳無し）＝訂正ドラフト 6 行＋記憶再保存 1 件** ／ 項目2 **tail POD の正体を実測裁定: +0x70/78/7C/80/84/88/8C = ポインタ7本＋+0x74/90/94 = float3。3関数は EffectSetting の仮想スロット 9/45/46（レコード管理 API）で、slot45 はレコード複製経路（FUN_0046C360/0048DA00）から実行される＝gameplay 読手はゼロのまま「レコード値として常に有効に保つ」ための ctor 既定と裁定／GMST 2係数はレコード初期値供給のみ（直接 E8 呼出しゼロ）** ／ 項目3 **【解明】0xB15A0C = `NiTPointerMap<MagicSystem::EffectID, ActiveEffect*(fnptr)>`（ActiveEffect 工場レジストリ）・登録 23 スチューブは CRT 初期化テーブル（.rdata 0xA283D0..0xA2F77C・走査機 0x981CB4 帯）経由で起動時実行・0xB3C000 = グローバル CRITICAL_SECTION（+0xA5.. のバイト旗は独立 BSS スカラーで **.text 読手ゼロ＝書手のみ**）** ／ 項目4 **【整理】engine ESCE 読出（0x416286 帯）⇔ port 読取ゼロのギャップ表＋Cure Disease/Poison 実装に要する読取欄ドラフト骨子を提示（現状 consumers ゼロで実害なし）** ／ 項目5 **【整理】baseMagnitude/baseDuration/range は代入0・読手0（enchanting_system の同名はローカル引数名で別物）＝削除案 vs 保全案を整理し削除（or 派生値 spec コメント）を推奨、[PORT] 判断** ／ 項目6 **【解明・前提反転】v235 本文（281行）に「oblivion_ini.cpp が編集された」記述は存在しない（grep 'ini.cpp' は L269 読取のみリストの1件のみ）。N-7(L280) は正しく oblivion_ini_defaults.h を記載。誤表記は v236 §4-4(1)/N-14 が「タスク指示の誤り」を v235 に帰属させた点に存在＝注記ドラフトを §6 に提示**
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**

---

## §1 項目1: 「2026-10-30 実装」日付誤りの grep 特定と訂正ドラフト（N-9）

**判定: 誤記は 3 ファイル・6 行＋supermemory 1 件の計 7 ポイント（訂正ドラフト提示・適用は要承認）。STATE.md / .github/memory/session-memory.json / tmp/re/TODO_re49.md には本件の日付誤記なし（別案件の正 date・または台帳無し）。実態はいずれも 2026-11-01 JST（Copilot 編集帯 20:51:18–20:52:23／host tests 21:08:59／build 検証 21:13:38＝いずれも v236 §4-1 の events.jsonl 実測）。**

### §1-1 grep 全数（読取のみ・本タスク実測）

| 対象 | ヒット | 判定 |
|---|---|---|
| `Oblivion_Android/TODO.md`（根） | **L6**（#29 注記「host tests ALL GREEN …, **2026-10-30**)／**L13**（#26 注記 同上）／L39（v201 RE セッション「2026-10-30: 子エージェント中断続き…」） | **L6・L13 = 誤記（要訂正）**／L39 = 別事象の正 date（v201 作業は 10-30 実在＝訂正対象外） |
| `.ai-todo/state/TODO.md` | L6・L13・L39（根 TODO.md と **MD5 一致 1ABC8272DA81F14945D00814811D3811・同 mtime 2026-11-01 21:10:24＝ミラー本**） | 同上（L6/L13 誤記・L39 対象外）。**訂正は両ファイルへ（またはミラー生成元へ）** |
| `plan.md` | **L1781**「**Implemented 2026-10-30 (#26, #29).**」／**L1790**「externalNativeBuildDebug (BUILD SUCCESSFUL, **2026-10-30**)」 | **両方 = 誤記（要訂正）**（実測: 編集 11-01 20:51–20:52、build 検証 11-01 21:13:38） |
| `.github/memory/session-memory.json` | `portres`／`#26, #29`／`Implemented.*2026-10-30` = **grep ゼロ**（該当する #26/#29 台帳エントリ自体が存在しない。"2026-10-30" ヒットは rel2 連託・RE_EXE_REPORT 訂正・V201 起票等の**別案件の正 date**） | **訂正不要（対象外）** |
| `STATE.md` | "2026-10-30" ヒットはすべて NIF/Rust 移行（N1–N5・oracle 等）の走査記録＝**別案件で実 date と一致**。`#26/#29/frame-dt/find_by_key` = grep ゼロ | **訂正不要（対象外）** |
| `tmp/re/TODO_re49.md` | `#26/#29` の日付記載なし。V229 完了行は「不一致検出: …「2026-10-30 実装」の日付は誤り（実測 11-01）」と**正しい指摘を既に記載** | **訂正不要（正）** |
| supermemory（Copilot 側） | save-memory イベント（events.jsonl・11-01 21:12 JST = timestamp 1793535138128）: id **`copilot-v230-portres-done-20261030`**、本文冒頭「ai-todo #26/#29 … **implemented and completed on 2026-10-30**, verified ALL GREEN …」 | **誤記（要再保存）**。ローカル台帳には無く Copilot supermemory クラウド側のみ |

- supermemory 保存本文（完全版）は `%TEMP%\opencode\v237_supermem_payload.txt` に抽出済み（v236 §4-1 の 21:12:13 save と同一 Call: `call_761033a9a364440b9d7629a1`）。誤 date は冒頭1文の「completed on 2026-10-30」のみ（他は date 表記なし）。
- 実態との対応（v236 §4-1 実測・再掲）: 編集 = **2026-11-01 20:51:18–20:52:23 JST**、host tests ALL GREEN = **11-01 21:08:59**、ai-todo 注記書込 = 11-01 21:10:24、plan 追記 = 11-01 21:11:14、supermemory 保存 = 11-01 21:12、gradle build 検証 = 11-01 21:13:38。

### §1-2 before→after 訂正ドラフト一覧（[PORT]/要承認・**コード/文書は本タスクで無編集**）

| # | ファイル:行 | Before（現状） | After（案） |
|---|---|---|---|
| 1 | `TODO.md`:L6（#29 注記） | `Implemented and verified (host tests ALL GREEN via run_host_tests.sh, 2026-10-30).` | `… 2026-11-01).` |
| 2 | `TODO.md`:L13（#26 注記） | `Implemented and verified (host tests ALL GREEN via run_host_tests.sh, 2026-10-30). ① find_by_key()…` | `… 2026-11-01). ① …` |
| 3 | `.ai-todo/state/TODO.md`:L6 | （#2）と同じ文・ミラー本） | 同上に合わせる |
| 4 | `.ai-todo/state/TODO.md`:L13 | （#2）と同じ文・ミラー本） | 同上に合わせる |
| 5 | `plan.md`:L1781 | `- **Implemented 2026-10-30 (#26, #29).** Both low-priority v230 residuals are resolved.` | `- **Implemented 2026-11-01 (#26, #29).** …` |
| 6 | `plan.md`:L1790 | `… externalNativeBuildDebug (BUILD SUCCESSFUL, 2026-10-30); host tests do not cover jni_bridge.cpp.` | `… (BUILD SUCCESSFUL, 2026-11-01); …` |
| 7 | supermemory（Copilot 側・再保存） | id `copilot-v230-portres-done-20261030` / 本文 `… completed on 2026-10-30, verified ALL GREEN …` | id `…-20261101`（任意）/ 本文 `… completed on 2026-11-01 …` を再 save。**実行主体は Copilot セッション（supermemory MCP）＝本セッションからは [PORT] 承認後に連託** |

- **訂正ドラフト件数: 6 行（3 ファイル）＋ supermemory 再保存 1 件 ＝ 計 7 ポイント**。
- リスク: 低（日付文字列のみ。TODO.md は ai-todo ミラーのため**両方同時**に直すか生成元のみを確認のこと）。
- 対象外と判断した根拠を明記: L39（v201）は「子エージェント中断続き」の 10-30 作業記録＝正。STATE.md の 10-30 は Rust 移行セッションの実績。session-memory.json には本件エントリ自体が無い（重複台帳リスクなし＝AGENTS/memory 規則どおり）。

---

## §2 項目2: MGEF DATA tail POD（obj+0x70..+0x94）の意味論 — 裁定【解明・一部残】

**判定: ①tail は「float 3（+0x74/+0x90/+0x94）＋ポインタ 7（+0x70/+0x78/+0x7C/+0x80/+0x84/+0x88/+0x8C）」という C++ メンバ型構成（v236 の「7 dword」は正しく、ただし slot9 実測で「dword 7個のうち +0x74 以外はすべてポインタ」と型判別できた＝新事実）。②copy/assign/== の正体は EffectSetting の**仮想スロット 9 / 45 / 46**（vtable 0xA32B14・RTTI `.?AVEffectSetting@@`）＝レコード管理 API。③slot45(=assign) はレコード複製/オーバーライド経路（FUN_0046C360 / FUN_0048DA00: formType lookup 0x447DF0 → vtable+0xD8 SetEditorID → slot45）から実際にディスパッチされる。④gameplay 読手は再測でもゼロ（クラスタ走査＋オフセット共起走査で object レジスタ上の tail 読手は ctor/ローダ/eq/assign/slot9/setter のみ、他は esp/ebp スタックローカル）。⑤したがって「copy/assign/== でのみ扱われる」理由の裁定 = **tail は ESM(64B DATA) から生コピーされるレコード値であり、engine は (a) レコード複製・(b) 等価比較・(c) 整形/再初期化・(d) 64B 一括往復 の4経路でのみ扱う。gameplay 判定は一切読まない。ctor 既定（1.0f・GMST 2係数）は「DATA が 36B しかないレコード・runtime 生成レコードでもレコード値として常に有効」を保つための供給源である**。＋0x90/+0x94 の GMST（CEEnchant/CEBarter）も同じ: レコード初期値供給のみで、getter 直接呼出しはゼロ（間接＝GMST ノード経由は残余地）。各ポインタ7本が「何を指すか」は未解明（§7 N-15）。**

### §2-1 3関数の実体 = EffectSetting 仮想スロット（dword 参照スキャン・本タスク）

| 関数 | vtable 参照位置 | スロット（0基点） | vtable 内 disp | RTTI |
|---|---|---|---|---|
| `FUN_00415800`（v236「copy」） | .rdata `0xA32B38` | **slot 9** | +0x24 | `.?AVEffectSetting@@`（vtable 0xA32B14・cache vtable_index と一致・55 slots） |
| `FUN_00416000`（assign） | .rdata `0xA32BC8`（+ .data 0xB2B789/0xB2B791 = 非整列の偶発一致） | **slot 45** | +0xB4 | 同上 |
| `FUN_00415BC0`（==） | .rdata `0xA32BCC` | **slot 46** | +0xB8 | 同上 |

- 直接 `call dword [reg+0xB4/0xB8]` 形式は **.text 全域ゼロ**（123 ヒットの `call [reg+disp]` はすべて disp=+0x24（Steam 帯 0x8B–0x95 が多数＋esp 帯）で、0xB4/0xB8 の直接形は無し）。
- **レジスタ経由ディスパッチ（`mov reg,[base+0xB4/0xB8] → call reg`）は 58 箇所**。うち EffectSetting に接続する候補:
  - **slot45 確度高**: `FUN_0046C360` — `movzx eax,[esi+4]`（formType）→ `call 0x447DF0`（型別 lookup）→ `mov eax,[esi]; call [eax+0xD8]`（**SetEditorID**・v235 §1-2 確定のローダ同物）→ `mov eax,[edi]; mov edx,[eax+0xB4]; push esi; mov ecx,edi; call edx` ＝ **新規 form へ old を代入するレコード複製/オーバーライド処理**。`FUN_0048DA00` も同型（0x447DF0 → slot45）。
  - **slot46 確度中**: `FUN_00512540` — `mov ecx,[esi+0x58]; mov edx,[ecx]; mov eax,[edx+0xB8]; push esi; call eax`（対象オブジェクトの型は未確証。前段で vtable+0x20 呼出を伴うフォーム系）。
  - 残り 54 箇所は 0x46xxxx–0x81xxxx の他クラス vtable の slot45/46（オフセット一致の偶然）。

### §2-2 3関数の本体実測（本タスク・v236 §5-4 の補足）

| スロット/関数 | 本体の要点（実測） | 読み |
|---|---|---|
| **slot46 `FUN_00415BC0`（==）** | 引数（[esp+8]）を `call 0x9832E6(p,0,0xB02F9C,0xB03534,0)` で解決 → `call 0x46AD70`（同一性チェック・bool）→ **フィールド順比較**: `[edi+0x98]` vs `[esi+0x98]`（mgefCode）、`[+0x58]`（flags）、`fld [+0x5C]`（baseCost）…に続き v236 測定の通り dword7＋float3（+0x74/+0x90/+0x94）＋word(+0x6C) | **レコード等価述語**（mgefCode 一致を含む＝別 mgef との誤照合を防ぐ） |
| **slot45 `FUN_00416000`（assign）** | v236 測定どおり [edi+0x70..+0x8C] → [esi+…] の 7 ワード＋float(+0x90/+0x94/+0x74) を代入 | **レコード代入**（複製経路 §2-1 から実行） |
| **slot9 `FUN_00415800`（v236「copy」→ 本タスクで実体解明・細部は未解明）** | ①サブオブジェクト整形（+0x30/+0x38/+0x44 call 0x46A700/0x46C730/0x470070、`push 'ICON'`）、②文字列群 `push 'MODT'/'MODB'/'MODL'; lea ecx,[esi+0x18]; call 0x46D8B0`、③**ポインタ7本を save → `field = field ? [field+0xC] : 0` に一時書換**（+0x80/84/88/8C/78/7C/70 = 対象がまさに「ポインタ型7本」の決定的証拠・+0x74 は除外）、④`push 0x40; lea eax,[esi+0x58]; push; push 'DATA'; call 0x46AE40`（**DATA 64B を扱うヘルパ**）、⑤保存値を復元、⑥`movzx eax,word[esi+0x6C]`（関連効果件数）非0なら `[esi+0x9C]` 配列処理（`movsx edx,ax; add edx,edx` でバイト数算出） | **整形/再初期化系**。引数（[esp+0x24]）を +0x88 へ書く経路も存在＝細部の意図は**未解明**（§7 N-15） |

### §2-3 gameplay 読手ゼロの再確認（本タスク・base 感知共起走査）

- `.text` 全域 225.8 万命令を線形走査し、`disp ∈ {+0x74,+0x90,+0x94}`（tail）と `{+0x58,+0x5C,+0x60,+0x64,+0x68,+0x6C,+0x98,+0x9C}`（head）を **同一 base レジスタ単位**で共起記録（v236 の走査は esp/ebp スタックを含むため本タスクで是正）。
- EffectSetting クラスタ（0x412F00–0x41C400）で object レジスタ共起したのは: `FUN_00415BC0`(esi/edi) / `FUN_00416000`(esi/edi) / `FUN_00416430`(esi=ctor) / `FUN_00416100`(ebx・+0x6C のみ) / `FUN_00415D40` ほか setter 系 のみ。
- 一見ヒットした `FUN_00413BE0` / `FUN_00415390` / `FUN_0041BB80` / `FUN_004190C0` / `FUN_00411B70` は **base=esp/ebp のスタックローカル**（個別ウィンドウで確認）＝EffectSetting 読手ではない。`FUN_0041BB80`（tail 23ヒット）は `mov ecx,[0xB33A98]; …; mov [0xB33560],eax` の別オブジェクト生成系。
- ⇒ v236 §5-4「gameplay 判定でこのブロックを読む命令は存在しない」を **base 感知で維持**。

### §2-4 +0x90/+0x94 GMST の供給網（実測・v236 §5-3 の補強）

| 経路 | 実測 |
|---|---|
| 登録（CRT） | `FUN_009D9FA0` 帯: `0x9DA740`（fMagicDefaultCEEnchantFactor → slot 0xB33704）/ `0x9DA770`（CEBarter → 0xB3370C）。**getter スタブ 0xA178F0/0xA17900 のアドレスが `0x9DA756`/`0x9DA786` で push されている**＝GMST ノードのアクセサとして登録（今回の新確認） |
| 読手 | ctor `0x416512`/`0x416520` のみ（`.text` 全域の 0xB33704/0xB3370C 参照は v236 の3箇所＝登録/ctor/getter スタブ）。**getter スタブへの直接 E8 呼出し = ゼロ**（間接＝GMST ノード経由・脚本 `GetGameSetting` 経由の可能性は残る） |
| レコードへの流れ | ctor が +0x90/+0x94 へ書いた後、**64B DATA の生コピー（ReadData(obj+0x58,0x40)）が DATA+0x38/+0x3C で上書き**（＝ESM 値優先・v236 §5-3 維持） |

### §2-5 裁定（説明仮説 → 実測ベースの結論）と [PORT] 材料

1. **なぜ copy/assign/== でのみ扱われるか**: tail は「レコードの所有権に属する値」（ESM 生コピー＋ctor 既定）であり、engine の扱いは **複製(slot45)/比較(slot46)/整形(slot9)/64B 一括往復** の4つのみ。gameplay（コスト・magnitude・分岐）は head（flags/baseCost/av/school/ResistAV）のみを使う＝実測で確認。
2. **なぜ ctor 既定が必要か**: 36B DATA のレコード・runtime 生成 EffectSetting（`FUN_00417220` は tail を書かない）でも、slot45/46 が触れる値が未定義でないようにするため（float=1.0f・ポインタ=NULL・GMST=既定 1.0f）。
3. **+0x90/+0x94 の語義**: GMST 名から「作成 enchant コスト係数／売却価格係数（CE）」と**推定**（確度: 中・UESP 404 のため EXE 外裏付けなし）。retail では**レコード初期値供給のみ・gameplay 消費なし**が実測。
4. **[PORT] 材料**: port が tail を読む必要があるのは (a) engine 同等の EffectSetting 等価比較を再現する場合、(b) CUDI/CUPO の関連効果走査を再現する場合（§4）に限られる＝現状読取なしは無害。

---

## §3 項目3: `FUN_009FEA50` スチューブ列の登録先 0xB15A0C とグローバル 0xB3C0xx の用途（N-11）

**判定: 【解明】①0xB15A0C = **`NiTPointerMap<MagicSystem::EffectID, ActiveEffect*(__cdecl*)(MagicCaster*, MagicItem*, EffectItem*)>`**＝**mgefCode(fourcc) → ActiveEffect 生成関数のレジストリ**（RTTI 実測）。②登録者は **23 個のミニ関数（スチューブ）**で、これらは孤立コードではなく **CRT 動的初期化テーブル（.rdata 0xA283D0..0xA2F77C・約3,133エントリ）の要素**、走査機は `0x981CB4` 帯（`mov esi,0xA283D0; mov edi,0xA2F77C; loop: if([esi]) call [esi]`）＝**プロセス起動時に必ず1回実行**。③消費者はディスパッチ本体 `FUN_0068EA50`（v236 #22）＝lookup 成功なら工場関数を呼ぶ、失敗なら fourcc 直書き木へフォールバック。④グローバル **0xB3C000 = CRITICAL_SECTION**（Init/Lock/Unlock/Delete が 0x49F450/0x49F470/0x49F480/0x49F460 = 0x401000系 IAT サンク経由で判明）で、スチューブが書くバイト旗 0xB3C0A5..0xB3C0EB は**独立 BSS スカラー（CS 領域は +0x00..+0x17 のみ）**。⑤バイト旗は **.text 読手ゼロ（絶対参照・imm 参照・base+disp 共起の3系統すべてで0）＝retail では書手のみ**。**

### §3-1 登録先 0xB15A0C の正体（RTTI 実測）

| 検査 | 結果 |
|---|---|
| .data ファイル値（0xB15A0C, 64B ダンプ） | vptr = `0xA750D8`（前後は TypeDescriptor 名文字列帯: `"VampirismEffect@@"` 等＝0xB15800–0xB15A68 = RTTI/初期化データ領域） |
| 起動時セット | `0x9FEA3B`（初期化関数 FUN_009FE990 内）`mov dword [0xB15A0C], 0xA75114` ＋ atexit(0xA25F90) 登録 |
| vptr 0xA750D8 の RTTI | `.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@W4EffectID@MagicSystem@@P6APAVActiveEffect@@PAVMagicCaster@@PAVMagicItem@@PAVEffectItem@@@Z@@` |
| **vptr 0xA75114（実行時）の RTTI** | **`.?AV?$NiTPointerMap@W4EffectID@MagicSystem@@P6APAVActiveEffect@@PAVMagicCaster@@PAVMagicItem@@PAVEffectItem@@@Z@@`** |
| → 解釈 | `NiTPointerMap<Key=MagicSystem::EffectID, Value=ActiveEffect*(*)(MagicCaster*,MagicItem*,EffectItem*)>`。Key はスチューブが fourcc imm32 を push している（§3-3）＝**mgefCode キー** |

### §3-2 スチューブ全量（本タスク・FUN_009FEA50 帯 0x9FEAB0–0x9FEDF7 を機械解析）

書式（各 0x20 バイト刻み・int3 パディング）: `push <工場関数> / push <fourcc> / call 0x68DA10 / add esp,8 / mov byte [旗], al / ret`（0x68DA10 = `push arg2; push arg1; mov ecx,0xB15A0C; call 0x452570(NiTMap::Insert); mov al,1; ret`）。

| # | スチューブ VA | fourcc | 工場関数 | 旗バイト | # | VA | fourcc | 工場関数 | 旗バイト |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 0x9FEAB0 | CALM | 0x691CA0 | 0xB3C0A5 | 13 | 0x9FEC30 | LGHT | 0x694A30 | 0xB3C0B8 |
| 2 | 0x9FEAD0 | CHML | 0x692040 | 0xB3C0A6 | 14 | 0x9FEC50 | LOCK | 0x694BD0 | 0xB3C0B9 |
| 3 | 0x9FEAF0 | COCR | 0x692300 | 0xB3C0A7 | 15 | 0x9FECE0 | NEYE | 0x6A2E30 | 0xB3C0DC |
| 4 | 0x9FEB10 | COHU | 0x6928D0 | 0xB3C0A8 | 16 | 0x9FED00 | OPEN | 0x6A3550 | 0xB3C0DD |
| 5 | 0x9FEB30 | DARK | 0x692C90 | 0xB3C0A9 | 17 | 0x9FED20 | PARA | 0x6A37B0 | 0xB3C0DE |
| 6 | 0x9FEB50 | DEMO | 0x692E40 | 0xB3C0AA | 18 | 0x9FED40 | REAN | 0x6A4030 | 0xB3C0DF |
| 7 | 0x9FEB70 | DTCT | 0x693450 | 0xB3C0AC | 19 | 0x9FED60 | STRP | 0x6A4D10 | 0xB3C0E0 |
| 8 | 0x9FEB90 | DIAR | 0x693550 | 0xB3C0AD | 20 | 0x9FED80 | SUDG | 0x6A6890 | 0xB3C0E8 |
| 9 | 0x9FEBB0 | DIWE | 0x693880 | 0xB3C0AE | 21 | 0x9FEDA0 | TELE | 0x6A79B0 | 0xB3C0E9 |
| 10 | 0x9FEBD0 | DSPL | 0x693A30 | 0xB3C0AF | 22 | 0x9FEDC0 | TURN | 0x6A7F30 | 0xB3C0EA |
| 11 | 0x9FEBF0 | FRNZ | 0x693DD0 | 0xB3C0B0 | 23 | 0x9FEDE0 | VAMP | 0x6A8AD0 | 0xB3C0EB |
| 12 | 0x9FEC10 | INVI | 0x693FE0 | 0xB3C0B1 | | | | | |

- fourcc は **EDID 由来 mgefCode**（v235 §1-2）と同系。うち vanilla 到達可否が未確認の候補: CALM/DEMO/FRNZ/STRP 等（**ESM 未調査・再スキャン禁止のため保留**＝fourcc は仕様のまま記載）。
- 工場関数 0x691CA0–0x6A8AD0 は 0x69/0x6A 帯の **ActiveEffect サブクラス群**（v236 #22 の固有ハンドラ帯と同一領域）。
- 同帯には **member 初期化3点も初期化テーブルエントリとして混在**: `0x9FEC80/0x9FECA0/0x9FECC0` = `push 引数; push vtable(0xA761AC/0xA7624C/0xA76570); mov ecx, 0xB3C0BC/0xB3C0C4/0xB3C0D4; call 0x70E220; ret` ＝**0xB3C0BC/C4/D4 の NiTMap 系メンバ3個を起動時構築**（第2引数は 0xB3A680/0xB3C0BC ＝アロケータ/親指定）。
- `0x9FEC70`: `mov ecx, 0xB3C0BB; jmp 0x749280`（初期化テーブル経由で起動時実行・**用途未解明**＝§7 N-16）。

### §3-3 スチューブの実行経路 = CRT 動的初期化テーブル【新規解明】

| 検査 | 結果 |
|---|---|
| スチューブへの E8/E9 呼出し | **ゼロ**（間接実行の兆候） |
| .rdata の DWORD 参照 | 各スチューブ VA は `.rdata 0xA2AFEC..0xA2B054` に**連番で格納**（前後にも FExxx 初期化関数が並ぶ） |
| テーブル全域の実測 | `.rdata 0xA283D0..0xA2F77C` に **fn ptr＋NULL パディング**（0xA283F0 から本体・fn-ptr 系 dword 合計 3,133・うち連続帯 0xA283F0–0xA2885C / 0xA28860–0xA2B4E8。スチューブ23個はすべてこの範囲内） |
| **走査機の特定** | `0x981CB4`（CRT 初期化関数・Ghidra ラベルは `__get_winmajor` で不正確な併合名）: `atexit(0x98D7E1)` → `mov esi, 0xA283D0` / `mov edi, 0xA2F77C` → ループ `mov eax,[esi]; test; je next; call eax; add esi,4; cmp esi,edi; jb` ＝**起動時に全初期化関数を順次実行** |
| ファイル内へのテーブル境界参照 | 始点 0xA283D0 が上記1件のみ（終点 0xA2F77C は他で参照・naive スキャンの rel32 偶発一致4件は命令境界検証で除外済み） |

→ **N-11 の核心（登録先の消費者を遡る）完了**: 起動 → CRT 走査機 → 23 スチューブ → `0x68DA10` → `NiTMap::Insert(0xB15A0C, fourcc, 工場)` ＋ 旗バイト=1。以降は §3-4 の消費者が参照。

### §3-4 0xB15A0C の消費者（実測全数）

| 消費者 | 呼び出し内容 |
|---|---|
| **`FUN_0068EA50`（効果別ディスパッチ本体・v236 #22）** | `0x68EAD7`: `mov eax,[esi]`（EffectItem+0x00 = mgefCode）→ `push &out; push key; mov ecx,0xB15A0C; call 0x55E000`（Lookup）→ 命中時 `call eax`（工場: (caster,item,effect) 3引数 cdecl）。未命中時: fourcc 直書き木（ABHE/ABFA/CUPA/SHLD/FRSH…）へフォールバック＝**両系統の実存を確認** |
| `FUN_0068DA10` | Insert ヘルパ（呼出しは23スチューブのみ） |
| `0x68DA30`（サンク: `mov ecx,0xB15A0C; jmp 0x7C2A40`＝Get 系） | **E8/E9・データ参照ともゼロ＝未使用候補** |
| `0xA25F90`（サンク: `mov ecx,0xB15A0C; jmp 0x68E0E0`） | atexit に登録された**デストラクタ**（初期化関数 FUN_009FE990 が `push 0xA25F90; call 0x981FB4`＝`_atexit`） |

### §3-5 グローバル 0xB3C0xx の用途【v236 記述の是正を含む解明】

1. **0xB3C000 = グローバル CRITICAL_SECTION（v236 の「byte グローバル帯」は独立スカラー群）**:
   - ctor `FUN_0049F450` = `mov esi,ecx; call 0x401000`、dtor `0x49F460` = `jmp 0x401010`、lock `0x49F470` = `push 適当; call 0x401020`、unlock `0x49F480` = `jmp 0x401040`。
   - 0x401000系の正体（PE インポート解決）: **IAT 0xA28064=InitializeCriticalSection / 0xA28068=DeleteCriticalSection / 0xA2806C=EnterCriticalSection / 0xA28070=TryEnterCriticalSection / 0xA28074=LeaveCriticalSection**（0x401020 は thiscall→stdcall 変換サンク `mov [esp+4],ecx; jmp [IAT]`・直前の `push imm` はスロット確保用ダミー）。
   - `mov ecx,0xB3C000` 呼出し 21 箇所（0x6825C6..0x683956 の magic 帯 19＝**ロック取得**、0x9FE790＝起動時 ctor、0xA25E30＝atexit デストラクタ）。初期化は CRT 初期化テーブルの `0x9FE790` エントリ。
   - ⇒ **CRITICAL_SECTION は +0x00..+0x17 のみ**。したがって +0x80..+0xFC 帯のバイト/カウンタ/ポインタ/マップ群（0x80/0x84 dword、0x88/0x89/0x8A byte、0x8C..0x98 ペア、0x9C/0xA0 counter/ptr、0xA4.. 旗、0xB4 counter、0xBC/0xC4/0xD4 マップ3個、0xD0 float、0xE4 ptr、0xF0..0xFC dword…）は**個別グローバルの集合**（魔法システム状態帯）。ただし v236 の記述「スチューブが 0xB3C0AA..0xB3C0EB に1を格納」は実測と一致（+0xA5 起点23本・うち AB/B2–B7/BA/BB/DB/E1–E7 はスキップ＝隣接する他フラグと交錯）。
2. **バイト旗の読手探索（3系統すべてゼロ＝書手のみ）**:
   - 絶対参照走査（.text 全域・mem/imm 全オペランド）: 0xB3C0A5..A9/AA/AC..B1/B8/B9/DC..E0/E8..EB は**全てスチューブの書手のみ**（読手なし）。対照的に 0x88/0x89/0x8A/A4/AB/BA/EC は**稼働中の runtime フラグ**（例: `0x68CC80 cmp byte[0xB3C0A4],0`、`0x63A365 cmp byte[0xB3C0AB],al`＋書手 0x6930F0 系、`0x6AA1E9 cmp byte[0xB3C0EC],0`＋切替書手 0x5B58xx 系）。
   - base+disp 共起（`[reg+0xA4..0xBF/0xDC..0xEC]` を使い **かつ** 0xB3C0xx imm を参照する関数）: **0 件**。
   - 一括コピー/範囲参照（`push 0xB3C0xx` 系 imm、bulk move）: ゼロ。
   - ⇒ **登録マーカー23本は retail .text に読手が存在しない（＝bookkeeping の書手のみ）**。CRC/デバッグ等の外部読手も想定されるが、本バイナリ内では未検出。
3. 付随する未検出系: `0x4040E0`（初期化関数が呼ぶ `push グローバル; mov ecx,0xB07BF0; call`＝登録ヘルパ・`0x446CB0`（cache: 347呼出しの add）を内部呼出）は「初期化ノード登録＋重複排除（0x98262d 比較）＋atexit(0xA25xxx デストラクタ)」の定型で、0xB15750/0xB15800/0xB15814/0xB15A68/0xB16178 の各グローバルを登録する系（RTTI/文字列系グローバルの寿命管理）＝**本件の主軸ではないため詳細は未追**。

---

## §4 項目4: port が 'ESCE'（関連効果 formID 列）を読まない件の整理（N-12・[PORT] 材料）

**判定: 【整理】engine 側読出（ローダ 0x416286 帯）と port 側のギャップを表で確定。port は `ESCE` を一切持たず（`app/src/main/cpp` 全域 grep **0 件**）、MGEF DATA も 20 バイト分しか読まない＝件数 word（DATA+0x14）と formID 配列が未読取。現状 consumers ゼロで実害なし。Cure Disease/Cure Poison（および Dispel 等の関連効果走査系）を実装する場合の読取欄ドラフト骨子を以下に示す（実装は [PORT]・本タスクではソース無編集）。**

### §4-1 ギャップ表（engine 実測 vs port 現状）

| 構造/経路 | engine（EXE 実測・v236 §5-2/§2-2 維持） | port 現状（本タスク grep/読取） | ギャップ |
|---|---|---|---|
| 件数 | DATA+0x14（word）→ obj+0x6C。ローダは `movzx eax,word[ebx+0x6c]` を見て配列サイズ決定 | `decodeMagicEffect` は DATA 先頭 20 バイトのみ（esm_reader.cpp:1257 `size()>=20`）＝**+0x14 未読取** | 件数列が無い |
| formID 配列本体 | **'ESCE' サブレコード**（分岐 `0x416142 cmp eax,0x45435345 / je 0x416286`）。0x416286 帯: 既存配列解放 → `count*4` 確保（ヒープ 0xB02020）→ `[ebx+0x9c]` 格納 → `TESFile::Read`（0x450C20）で ESCE payload 読出 | `ESCE` = **`app/src/main/cpp` グレップ 0 件**（rec.findSubRecord 呼出しも無し）。MagicEffectData に該当フィールドなし | 配列が完全に未取得 |
| runtime 生成側 | `FUN_00417220` → `call 0x415D40`（setter: `mov word[esi+0x6c],bx` ＋ 0x9C 配列確保）。既定効果レジストリ `FUN_00417420` は関連効果リテラルを供給 | （port に runtime 生成系なし） | — |
| **消費者** | **`FUN_00616BA0`（CUDI/CUPO＝v236 #22-18）**: `mov ebp,[ebx+0x9c]` ＋ `cmp word[ebx+0x6c],si` で**関連効果配列を走査**し各疾患/毒を除去（bit22 チェッカ 0x414EB0 ガード付き）。同型走査が Dispel(DSPL) 系にも存在 | Cure Disease/Cure Poison 挙動は port 未実装（alchemy/spell 側に CUDI/CUPO 処理なし＝v233/v236 と整合） | 実装時に前提が揃わない |

### §4-2 Cure Disease/Poison 実装時に必要な対応（読取欄追加ドラフト骨子・**[PORT]**）

```cpp
// assets/esm_reader.h — MagicEffectData へ追加（草骨架・実装は承認後）
    // ESCE: related-effect formIDs (engine: DATA+0x14 = count(word) @ obj+0x6C,
    // payload array @ obj+0x9C, read from the 'ESCE' subrecord — v236 §5-2,
    // consumed by CUDI/CUPO/DSPL walkers e. FUN_00616BA0).
    std::vector<uint32_t> relatedEffects;

// assets/esm_reader.cpp — decodeMagicEffect の DATA 読出直後に追加
    // DATA+0x14 = related-effects count (word), the array itself lives in ESCE.
    // (optional) effect.relatedEffectCount = readU16(medt->data.data() + 0x14);
    auto* esce = rec.findSubRecord("ESCE");
    if (esce) {
        const size_t n = esce->size() / 4;
        effect.relatedEffects.reserve(n);
        for (size_t i = 0; i < n; ++i)
            effect.relatedEffects.push_back(readU32(esce->data.data() + i * 4));
    }
```

- **接続点**: (1) Cure/Poison 挙動は「CUDI/CUPO の relatedEffects（＝治せる疾患/毒の MGEF 群）に一致するアクティブ効果を除去」（engine の走査と同型）。(2) 件数 word と ESCE 要素数の不一致（mod 混入）は min 扱いか要検証（engine は件数 word を唯一のサイズ元にする＝0x41629B 実測）。(3) DATA 読出を `size() >= 0x16`（+0x14 含む 22B）へ広げるかは [PORT]（現行 20B 判定のまま 22B 以降のみ別読取でも可）。
- **現状リスク**: consumers ゼロ（v236/本タスク grep 一致）＝読取なしは無害。**Cure/Dispel 系を実装する最初の時点で本ドラフトが前提**。

---

## §5 項目5: baseMagnitude/baseDuration/range 死フィールドの [PORT] 扱い（N-13）

**判定: 【整理・参照ゼロを実測】3 フィールドは「宣言のみ・代入 0・MagicEffectData 経由の読手 0」の三件すべて確認。MGEF DATA に相当列は存在しない（v236 §5-5 維持）。削除案 vs 保全案を整理し、**削除（または effectType と同様「派生値である」という spec コメント付きで実装方針を [PORT] が決定）**を推奨。**

### §5-1 参照ゼロの確認（grep 全数・本タスク）

| 検査 | 結果 |
|---|---|
| 宣言 | `esm_reader.h:429-431`（`float baseMagnitude/baseDuration/range = 0.0f;`）のみ |
| 代入 | `decodeMagicEffect`（esm_reader.cpp:1243-1272）に**無し**＝常に 0.0f。MagicEffectData の生成点はこの1箇所のみ（v235 §1-1 と同型の確認） |
| 読手 | `baseMagnitude\|baseDuration` グレップ = h:429-430 の宣言＋**`game/enchanting_system.cpp` のローカル引数名**（`getEffectMagnitude(..., float baseMagnitude, ...)` 等＝**別スコープの同名変数で MagicEffectData とは無関係**）。`MagicEffectData` 経由の `.baseMagnitude` / `->baseMagnitude` 等 = **0 件** |
| `range` | `effect.range` / `mgef.range` / `mgef->range` = **0 件**（combat_manager/phase45/stb 等のヒットは別構造体） |
| DATA に列が無いこと | MGEF DATA+0x14 以降 = 件数 word＋ポインタ7＋float3（§2）＝magnitude/duration/range の値は DATA に**無い**（magnitude/duration は EffectItem/EFIT 側の値＝v233/v236 と整合） |

### §5-2 削除案 vs 保全案（[PORT] 判断材料）

| 案 | 内容 | メリット | リデット/リスク |
|---|---|---|---|
| **削除案（推奨）** | esm_reader.h:429-431 の3行を削除 | 「MGEF DATA に magnitude/duration/range は無い」という EXE 実測仕様を型で表現。v233 N-1（effectType＝常に0で消費点が死にコード化）と同じ事故パターンを未然に防ぐ | 将来 EFIT 側の基準値として必要になったら再宣言（読手ゼロのため差分は無害） |
| 保全案 | フィールドを残しコメントで spec 化 | 「効果の基準値」という将来 port 仕様（spellmaking 等）の接続点を残す | engine に equivalent が無い＝**ESM 由来値では決して埋まらない**。effectType 同様「常に0」の罠が残る。残すなら「derived/unused — NOT from MGEF DATA; no engine counterpart (v237 §5-1)」の明記が必須 |
| （折衷） | 削除し、必要時は engine 同等の fourcc 由来派生値として再設置 | 二重管理を回避 | [PORT] の将来仕様次第 |

- **テスト**: tests/ に3フィールドの参照なし（grep と同範囲）＝削除でビルド/テスト影響なし。**[PORT] 判断事項: 削除 vs コメント付き保全の選択のみ**（v236 §1-2 (iii) の残課題）。

---

## §6 項目6: v235 N-7 のファイル名誤り訂正（N-14）— 注記ドラフト【前提反転を検出】

**判定: 【解明・ただしタスク前提と相違】v235 本体（全 281 行）を全文読取し、`ini.cpp` の grep ヒットは **L269 の 1 件のみ＝「移植ソース: 読取のみ（… oblivion_ini.cpp …、いずれも編集なし）」リスト**である。**「oblivion_ini.cpp が編集された」という記述は v235 本文に存在しない**。N-7（L280）は正しく `oblivion_ini_defaults.h（find_by_key コメント訂正）` を記載している。誤表記「oblivion_ini.cpp（find_by_key コメント訂正）」は **v236 §4-4(1)（L283）と N-14（L387）にのみ存在**し、いずれも「本タスク指示/出典（v235 N-7）の…」と v235 へ帰属させている＝**誤りの実体はタスク指示側にあり、v236 がそれを v235 に転記帰属させた**。ファイル実体の裏取り（mtime）は v236 記載どおりと確認。以下、注記ドラフトのみ（v235/v236 とも本タスク無編集）。**

### §6-1 裏取り（本タスク・読取/Get-Item）

| 検査 | 結果 |
|---|---|
| v235 全文 grep `ini.cpp` | **L269 のみ**（読取のみリスト・「いずれも編集なし」）＝編集主張なし |
| v235 N-7（L280） | `… settings_manager.h（NOTE -18 行相当削除）/ **oblivion_ini_defaults.h（find_by_key コメント訂正**＝v233 §5-3 とは別文言での訂正）/ esm_reader.h …` → **正しいファイル名** |
| 誤表記の所在 | v236 L283: 「本タスク指示/出典（v235 N-7）の「oblivion_ini.cpp（find_by_key コメント訂正）」はファイル名誤り」／v236 L387（N-14）: 「**v235 N-7 の対象ファイル名誤り**: …」→ **v235 に存在しない文を N-14 の対象として設定している** |
| ファイル mtime 実測 | `oblivion_ini.cpp` = **2026-10-25 09:58:09**（編集帯外・未編集）／`oblivion_ini_defaults.h` = **2026-11-01 20:51:24**（編集帯一致）／esm_reader.h・gmst_defaults.h・jni_bridge.cpp・combat_formulas_tests.cpp = 2026-11-01 20:51:24、settings_manager.h = 20:52:22（v236 §4-3 と完全一致） |
| コメント実在先 | v233 §5-2 がヘッダコメントの実在先を `system/oblivion_ini_defaults.h:54-57` と特定済み＝**v235/v233/v236(実体記述) は三者一致で oblivion_ini_defaults.h** |

### §6-2 注記ドラフト（**v237 内の本記載がそれ自体の注記**。v235/v236 本体は無編集・以後の参照時は以下を正とする）

```
【N-14 訂正注記（v237・2026-11-02）】
1. v235 N-7（L280）のファイル名は原文どおり正しい（oblivion_ini_defaults.h）。v235 本文に
   「oblivion_ini.cpp が編集された」記述は存在しない（grep 'ini.cpp' = L269 読取のみリスト1件）。
2. 「oblivion_ini.cpp（find_by_key コメント訂正）」という誤表記の出所は V229 へのタスク指示側
   （＝v236 §4-4 が "本タスク指示/出典（v235 N-7）" と帰属させた相手）であり、v236 N-14 の
   「v235 N-7 の対象ファイル名誤り」という**問題設定自体が誤帰属**。実体は v236 L283/L387 の
   帰属記述の訂正を要する。
3. ファイル実体は v236 記載どおり: 編集されたのは system/oblivion_ini_defaults.h
   （mtime 2026-11-01 20:51:24）のみ。system/oblivion_ini.cpp は mtime 2026-10-25 09:58:09
   で編集帯外（git status にも無し＝v236 §4-4 実体記述は正しい）。
4. 適用: v236 L283/L387 の文言訂正は [PORT] 要承認（既存成果物は本タスク無編集のまま）。
```

---

## §7 新規残点（本タスクで新たに発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-15 | **EffectSetting vtable slot9（`FUN_00415800`）の正確な意味論と、ポインタ7本（obj+0x70/78/7C/80/84/88/8C）の指す先が未解明**: slot9 は「サブオブジェクト整形＋'ICON'/'MODL/MODB/MODT'/`('DATA',this+0x58,0x40)` ヘルパ＋ポインタ7本の save→`[p+0xC]` 書換→復元＋関連効果 word ループ」を行単位で実測したが、意図（複製？整形？リセット？）と引数（[esp+0x24]→+0x88 書込み）の解釈が未確定。ポインタ7本は ESM 由来でも runtime 生成でも**常に0（NULL）**が現状＝誰も非0化しない reserved member とみられるが未証。slot45 の実行者（FUN_0046C360/0048DA00＝レコード複製系）は確度高・**slot46 の実行者候補 FUN_00512540 等は対象型未確証（確度中）** | 低 |
| N-16 | **CRT 初期化テーブルエントリ `0x9FEC70`（`mov ecx, 0xB3C0BB; jmp 0x749280`）の用途未解明**: 起動時に実行される（テーブル 0xA2B024 格納）が、this=0xB3C0BB（非整列・旗帯の隙間）と遷移先 0x749280 の関係が未調査。0x749280 の識別が次手 | 低 |
| N-17 | **23 本の登録マーカー旗（0xB3C0A5..0xB3C0EB）の読手が retail .text に存在しないことの最終裁定**: 3 系統の走査（絶対参照・imm・base+disp 共起）すべてゼロを確認したが、「誰も読まない bookkeeping」か「読み方が走査外（構造体一括シリアライズ等）」かは証明しきれない（一括参照の imm ゼロまでは確認済み）。バイナリ外の読手（外部モジュール等）は対象外 | 低 |

（N-9..N-14 の扱い: **N-9 は §1 で解消（訂正ドラフト 7 ポイント・適用は要承認）／N-10 は §2 で裁定解明（残余は N-15 に分離）／N-11 は §3 で解明（残余 N-16/N-17）／N-12 は §4 で整理（読取欄ドラフト骨子・実装は [PORT]）／N-13 は §5 で整理（削除推奨・[PORT]）／N-14 は §6 で前提反転を検出しつつ解消（注記ドラフト提示・v236 帰属文言の訂正は要承認）。**）

---

## §8 検証情報

- **selftest**: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）。
- **EXE**: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（PE 手動解析: **ImageBase 0x400000・section VA は RVA のため ImageBase 補正が必要**——本タスク初回実行は VA=RVA 誤りでラベルが -0x400000 ずれたが椢出内容は同一、補正後に全スクリプト再実行。.text VA 0x401000 raw 0x400 size 0x626E00 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00 rawsize 0x30C00（超えは BSS・ファイル値なし＝0xB33704/0xB3C0xx は該当））。
- capstone 5.0.7（skipdata＋detail）。本タスクの再逆アセンブル/走査: .text 線形走査 225.8 万命令×2 回（tail/head 共起・indirect dispatch・imm/mem 絶対参照）／0x415800・0x415BC0・0x416000 仮想関数本体／0x9FE700–0x9FEE80 初期化帯（スチューブ全23＋member 初期化3＋サンク）／0x68EAB0–0x68EB60 ディスパッチ／0x68DA10・0x68DA30・0x4040E0・0x49F450/460/470/480・0x401000–0x401060 サンク／0x981C40–0x981D80 CRT 走査機／0x682590・0x6833F0・0xA25E10・0xA25F50 ウィンドウ／.rdata 0xA28000–0xA2B800 dword ダンプ（初期化テーブル特定）。
- **スクリプト（新規のみ・`%TEMP%\opencode\`）**: `v237_events_scan.py`(+`_out.txt`・Copilot events.jsonl 1,168 ヒット) / `v237_extract_payload.py`(+`v237_supermem_payload.txt`・supermemory 保存完全文／`v237_date_contexts.txt`) / `v237_exe_scan.py`(+`_out.txt` 607KB・abs216/call116/tail5085) / `v237_exe_scanB.py`(+`_out.txt`・スタブ解析/データ参照/Windows) / `v237_exe_scanC.py`(+`_out.txt`・virtual call 123/dispatch 58/RTTI 修正版) / `v237_exe_scanD.py`(+`_out.txt`・ウィンドウ/IAT 前哨) / `v237_exe_scanE.py`(+`_out.txt`・**PE インポート名解決**) / `v237_exe_scanF.py`(+`_out.txt`・slot9 間接 dispatch 122) / `v237_scanG/H/I/J.py`(+`_out.txt`・初期化テーブル境界・走査機特定・命令境界検証付き refs）。v236 の `v236_slots2.py`/`v236_fourcc28*.py` 等は参照のみ（非改変）。
- **セッション記録（読取のみ）**: `C:\Users\hiroki.kogarumai\.copilot\session-state\9427ac74-...\events.jsonl`（save-memory payload 抽出。timestamp 1793535138128 = 2026-11-01 21:12:18 JST）。
- **移植ソース（読取のみ・編集なし）**: `assets/esm_reader.h`（L400-454）/ `assets/esm_reader.cpp`（L1238-1282）/ `game/enchanting_system.cpp`（grep 突合）/ `game/alchemy_system.cpp`・`spell_manager.cpp`（v236 突合）。
- **日付誤り grep 対象（読取のみ）**: `TODO.md` / `.ai-todo/state/TODO.md`（MD5 一致ミラー確認）/ `plan.md` L1770-1790 / `STATE.md` / `.github/memory/session-memory.json`（＋.bak 2本・grep のみ）/ `tmp/re/TODO_re49.md` / `%TEMP%` 外の supermemory は events.jsonl 経由。
- **RE 成果物（読取のみ）**: `tmp/re/v236_port_mgef_settings_residuals2.md`（本体・全 404 行）/ `v235_port_alchemy_settings_residuals.md`（本体・全 281 行）/ `v233_relationship_residuals2.md`（§5）/ `cache/named_funcs.txt`（21,597行）/ `cache/vtable_index.tsv`（EffectSetting vtable 行・NiTMapBase 行 274）/ `cache/call_targets.json`・`cache/README.md`。
- **git**: 本タスクでは不使用（v236 §4-2 の検査結果を引用）。**commit/push/stash 等は一切実行していない。**
- **ESM 本体: 未アクセス**（再スキャン禁止を遵守）。web 資料: 未取得（+0x90/+0x94 の語義は推定のまま・確度中と明記）。
