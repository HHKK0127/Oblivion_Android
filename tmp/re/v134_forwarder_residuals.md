# V134: フォワーダ族の残件（R6 アクション本体16件・a2/a3 語義・+0x20 帯A/B 消費者）

- 日付: 2026-10-27
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416 / ImageBase 0x400000 / 7,898,624 B）
- 前文書: `tmp/re/v97_forwarder_family_map.md` §6-1 / §6-2 / §6-3 / §6-5（残課題）。
  補助: `tmp/re/b21_speculative_backing.md`（D類裏取り＝StartQuest/ShowRaceMenu/ShowSpellMaking 等の **impl 側** は V117 で済。本稿は **R6 helper 本体側** の精査に限定）、`tmp/re/b22_getter_types.md`（+0x20 語義）、`tmp/re/cache/opcode_table.json`（533行）
- 自己検証: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時の2回）
- 成果物: 本ファイル 1 件のみ（新規）。`docs/`・既存 tmp/re ファイルは無変更。中間スクリプトは `%TEMP%\opencode\v134_*.py`

## §0 結論サマリ（先出し）

| # | 残件（v97 §6） | 判定 |
|---|---|---|
| §6-1 | R6 アクション本体 16 件が叩くゲーム機構 | **解明**（§1）。16/16 すべて本文逆アセンブルで機構特定。うち 6 件は b21 で判明の impl 側証拠を本体側から再確認・拡張、7 件は本調査で初めて機構判明、StartQuest/CompleteQuest は「フラグ thunk」であることが判明し設計構造が更新された |
| §6-2 | R1/R2 helper の a2/a3 読手 | **解明**（§2）。R1 89 件は **a2/a3 を読まない**（不読が正）。読手は **R2 実体 3 件（a2 のみ、a3 は全 111 件で読手 0）**。CTDA 経路と opcode 経路の値差は「R1 では無害な不読」「R2 ではスタブによる正規化」で解消 |
| §6-3 | 帯 A/B の +0x20 消費コード | **解明（存在しない）**（§3）。表側: 帯 A 31 行・帯 B 132 行は **全行 cond(+0x20)==0**。コード側: 帯 A cond 基底 `0xB0AF68` / 帯 B cond 基底 `0xB0B440` の .text 参照は **0 件**、全セクション dword スキャンでも间接経路 **0 件**。+0x20 消費者は `FUN_0056AC50` / `FUN_0056AE20` の 2 関数のみ（帯 C 専用） |
| §6-5（関連） | R1 でトレース文字列を持たない 3 件の命名 | 本稿の対象外（b22 で型側は確定済）。命名の裏付けは未着手のまま（§6 残点参照） |

## §1 R6 アクション本体 16 件の精査

### 1.0 手法

- 16 VA を capstone 5.0.7（skipdata）で完全逆アシンブル（`ret` + 直後 `CC CC`/`00 00` パディングで打ち切り）。E8/E9 先・push 文字列（.rdata 0xA28000–0xB018D2）・グローバル参照・vt 間接 call を全採取。
- `docs/Oblivion_decompiled.c` を 1 回走査して索引構築（**7,251 関数**が本文定義として存在）。**R6 16 件はすべて ABSENT**（＝Ghidra 未登録＝v97 §5.1 の未登録 94 件に含まれる）。よって証拠は capstone 本文のみ。直接 callee についても decompiled 収録はごく一部（`FUN_009832E6` 等）で、他は asm の call 先列挙で代替。
- b21 と重複する点（XML パス・メニュー id・annotate_db 名）は「再確認」欄に留め、新情報（ thunk 構造・グローバル・深層機構）を主に記載。

### 1.1 16 件一覧（本体精査結果）

| helper | opid | 名称 | 判定 | 叩くゲーム機構（本文確定） |
|---|---|---|---|---|
| `0x00676D90` | 0x150 | PrintNPCDialog | **解明** | **デバッグ・コンソール出力専用**。`this+0x68`（会話中アクターリスト）を先頭から走査 → 各ノードのアクターに IsActor ガード（vt+0x190）→ 述語 `0x005E6B40`（3Dノード vt+0x184 の結果の型バイト +0x20==0x12 = 「会話相手を持つ」判定）→ 3Dノード vt+0xCC で会話相手を取得 → `0x004DA2A0` で名称化し、`0x00579B60` へ `"%s %s%"`+"is talking to " を印字（`<speaker> is talking to <target>`）。ゲーム状態への副作用なし |
| `0x00440510` | 0x107 | ToggleCollisionGeometry | **解明** | 実体は **6 命令の分岐ディスパッチャ**。`[this+0x34]` 有 → `jmp 0x004D6560`（単一オブジェクト系: デバッグ描画マネージャ **グローバル `0xB35C24`**（+0x1A フラグ）の vt+0x98 へ `[this+0x54]` ノード可視性設定 + `0x004D1E40`）／無 → `ecx=[this+8]` して `jmp 0x00482600`（グリッド走査: `[grid+0xC]`×`[grid+0x10]` の 8B エントリを列挙し type byte `+0x26==6` のノードのみ `0x004D5320` へ）。= **コリジョン形状のデバッグ表示トグル**。同型の近接関数 `0x00440530`（`[ecx+0x53]` に引数保存 → 同グリッド走査 `0x004824C0` → `0x004CE3C0`→`0x004C5BA0`）も併存 |
| `0x006669F0` | 0x16E | ToggleFlyCam | **解明** | call/jmp 不使用の完全インライン実装。`[this+0x748]`（フライキャン有効バイト）を反転。**ON 化時**: vt+0x174（GetPos）で座標を `[this+0x754/758/75C]` へ保存、vt+0xEC（float 返し getter）の値 × `[this+0x5D4]`（速度係数）を Z アンカー `+0x75C` に加算、回転 `[this+0x28]/[+0x20]` を `+0x74C/750` へ保存。= **フライカメラのアンカー捕捉＋切替**（大構造体を持つ this = プレイヤー系オブジェクト。実名は §6 残点） |
| `0x004E7D10` | 0x115 | TogglePathGrid | **解明** | グローバル **`0xB35F84`**（表示フラグ）を反転して `0x004E76D0(flag)` を呼ぶ。同関数（143 命令・SEH /FS:0 セットアップ）が本体: ON= `0x401F00`(operator new, 0xDC) → `0x0070B780` 構築 → グローバル **`0xB35F88`** へ `0x0075FA90` で登録 → 現ワールド `[0xB333A0]+0xC` の vt+0x84(1) でアタッチ。OFF= vt+0x88 でデタッチ → dtor 経由（`[0xA2807C]` vtable チェック）→ `0x00477EF0` でリスト掃除 → `0xB35F88=0`。その後、外景グリッド `[0xB06A2C]`² を `0x00482150`(cell,i,j) → ノード `0x004AF170` → 表示 `0x004E71A0`（259 ins）／隠蔽 `0x004E5400`（78 ins、`0x70xxxx` ノード帯）で更新。**パスグリッド描画オブジェクトの生成/破棄＋セル単位再描画** |
| `0x00683BE0` | 0x116 | TogglePathLine | **解明** | **4 命令**: グローバル **`0xB3C08A`** バイトを反転するだけの純フラグ（パスライン描画フラグ。実描画側の消費者は本調査対象外 → §6 残点） |
| `0x0059F970` | 0x1145 | RefreshTopicList | **解明**（b21 再確認＋手順確定） | メニューゲート（`0x00589B70`/`0x005898F0`）→ MenuTopic マネージャ `0x006B8660`（文字列 `".\Dialogue\MenuTopic.cpp"`）→ `0x0059EA10`（ダイアログメニュー再構築、`Data\Menus\Dialog\dialog_menu.xml`）→ `0x0059E2B0`（`topic_template` ノード更新。トピック走査 `0x005E8900`/`0x005E89B0`/`0x0052ED50`/`0x0052F790`）→ `0x00584640`。b21 §2.1 の文字列証拠を call 順で再確認 |
| `0x005ACE20` | 0x10D5 | AdvancePCLevel | **解明**（b21+拡張） | **レベルアップメニューのオープン本体**（annotate_db `LevelUpMenu_Open` を本体で再確認）。MenuManager 系（`0x00582160`/`0x00588890`/`0x005898F0`/`0x0058CEB0`/`0x0058CED0`）＋`0x009832E6` RTTI → `0x00590420`（Tile::ReadFile。文字列 `Data\Menus\levelup_menu.xml` / `"Level Up  Menu Creation Failed..."`）→ テンプレ `0x005AC990`/`0x005ACB60` が **`levelup_template`** に `'+%i'` 行を構築（`0x005F1910` GetAV_Int ×3、`0x005E1FD0` GetLevel）→ サウンド `0x006AB160`（文字列 `".\Data\Music\Special\success.mp3"` / `'success'`/`'death'`）。**新情報**: XML パス・テンプレート名・成功ジングル |
| `0x0057B7E0` | 0x10DA | ShowRaceMenu | **解明**（b21+拡張） | MenuManager ×多数 + GMST ゲート `0x00588BD0` → **世界/カメラ整備**: `0x006A2190`/`0x006A2280`（現在セル取得 `0x006ECC80`、`0x0065C620` RemoveAll 等）、`0x00678D90`/`0x0067ACA0`（グローバル **`0xB3C0BC`** 操作）→ `0x005CA010`（`Data\Menus\CharGen\race_sex_menu.xml`、メニュー id 0x40C＝b21）→ デバッグテキスト系 `0x00571F90`/`0x005723E0`（文字列 `'Too many unique debug text items. \n'`）→ グローバル **`0xB3BD00`** 書き込み。**新情報**: メニュー構築前の世界整備 call と 2 グローバル |
| `0x0057A9B0` | 0x10DB | ShowBirthSignMenu | **解明**（b21 準確定の補強） | 37 命令。MenuManager ×4 → GMST ゲート `0x00588BD0` → **末尾 `jmp 0x005D6390`（mode=3, 0）**＝キャラゲンメニュー構築関数へ尾ジャンプ（同関数の文字列は `Data\Menus\CharGen\skills_menu.xml` / `"Attribute Menu Creation Failed..."`）。birthsign 専用 XML は本文に不在＝b21 の保留を**構造的に再確認**（mode 分岐先が別構築）。実到達先の XML は未特定（§6 残点） |
| `0x005D86C0` | 0x10EA | ShowSpellMaking | **解明**（b21+拡張） | `Data\Menus\dialog\Spellmaking.xml` オープン（`0x00585190` 経由、b21 再確認）。加えて **効果リスト構築**: `0x005D8180`（文字列 `'known_effect_template'` / `'Icons'` / `'%s\\%s'`）、詠唱アイテム整備 `0x00663AA0`（`0x005E02E0` フォーム取得・`0x00414F00`・`0x0065C620` RemoveAll）、プレイヤー呪文リストアクセス `0x004165E0`/`0x004170B0`/`0x004526E0`、`0x005E4420` |
| `0x005A1EC0` | 0x10EB | ShowEnchantment | **解明**（annotate_db 再確認+拡張） | `Data\Menus\dialog\Enchantment.xml` + 失敗文字列（b21 再確認）。**付加**: `0x005A17B0` = パラメータオブジェクト検証（文字列 `'pEnchantmentName is NULL'` / `'pUsesIcon is NULL'` / `'pEnchantmentIcon is NULL'` / `'pEnchantmentGoldCost is NULL'` / `'pKnownEffectList is NULL'`）、メニュー音 `0x0057DDE0`/`0x0057D2F0`（`'MenuEnd'`/`'MenuStart'`/`'UIMenuBracket'`/`'ITMBookPageTurn'`/`'UIMenuFocus'`/`'UIMessage'`）、`0x00438DB0` |
| `0x00529820` | 0x1036 | StartQuest | **解明**（構造更新） | 実体は **22 命令の thiscall thunk**（call 不使用）: 引数真偽で **`TESQuest +0x3C` の byte bit0 を set/clear** → `[esp+4]=4` として **vt+0x40 通知**（マスク 4）へ尾ジャンプ。「クエスト開始」はこの thunk ではなく**近接関数 `0x00529860`** に実装: スクリプト解決 `0x0046F020`（文字列 `"Unable to find script (%08X) on owner object \"%s\"."` 等3種）→ スクリプト実体 `0x004FBDC0` を `+0x58` へ → vt+0x48(0x8000000) 通知 → グローバル `0xB10CA0` ゲートで bit0+vt+0x40(4) → **CTDA 実行 `0x0056A480`（`quest+0x50` の条件ブロック、`0x0056AC50`/`0x0056AE20` を内包）** → リスト `+0x40` を `0x0052B160`（参照解決）・`+0x48` を `0x0052B340`（文字列 `"Could not find target reference (%08X) on quest (%08X) '%s'."` / `"No reference on target for quest (%08X) '%s'."`）で走査 → `0x0046AB80`(1)。`0x00529860` の E8 呼出し元は **0 件**（＝vt/表経由の仮想関数。§6 残点） |
| `0x00529840` | 0x1071 | CompleteQuest | **解明**（構造更新） | 同型 thunk で **`+0x3C` bit1（mask 0x2）** を set/clear → vt+0x40(4)。以降は `0x00529851` 経由で StartQuest thunk と同一の通知尾 |
| `0x005E02B0` | (FAM-12共有) | SetUnconscious 側 helper | **解明** | 9 命令: `[this+0x58]`（3Dノード）が非 NULL なら **ノード vt+0x194 へ actor を引数に callback**。= アクター状態変更後の 3D ノード通知（FAM-12 両メンバーが gate 成功後に呼ぶ定型） |
| `0x005E8E30` | 0x10F1 | SetUnconscious | **解明** | 引数真 → `[esp+4]=3` して `jmp 0x005E6680`（状態 3 = 気絶）。引数偽 → **`[this+0xB0]==3` のときのみ** `[esp+4]=0` で同一先（解除）。それ以外は `ret 4` |
| `0x005E8E60` | 0x10F3 | SetRestrained | **解明** | 引数真 → `0x005E6680`(5)（状態 5 = 拘束）＋ 3Dノード存在時 `vt+0x2C8` へ `[esp+4]=0` で尾ジャンプ。引数偽 → `[this+0xB0]==5` のときのみ `0x005E6680`(0) |
| （共有先）`0x005E6680` | — | 両者/他21 caller の共有先 | **解明** | **汎用 SetActorState(state)**（E8 xref = **21**: `0x45C224`/`0x4DFA61`/`0x5EA042`/`0x5EB5D2`/`0x5F1E39` 等）。手順: グローバル `0xB14E98` ゲート → base form（vt+0x170）＋IsActor ガード → base+0x28 の bit1 が立つ対象は state 1/2 を **6 へ強制**（＝制限付き遷移）→ state 2 は 3D vt+0xE8 → vt+0x9C(1,1) と extra `+0x44`（`0x0041E6F0` GetExtraData → `0x004876C0`）で特別処理 → ノード vt+0x47C ∈ {3,4} のとき `0x00628630`(0, actor) → **状態フィールド = `[this+0xB0]`**、変化時は **vt+0x40 通知 mask 0x40**。状態値: 0=通常 / 3=気絶 / 5=拘束 / 6=強制系（意味は §6 残点） |

### 1.2 v97 §6-1 への影響

- v97 §2.1 が「R6＝アクション本体 [仮説]」とした 16 件は、**16/16 で中身を確定**。うち PrintNPCDialog / TogglePathLine は「副作用すらデバッグ描画・ログのみ」であり、**移植時の実装優先度が最低**であることが本文で裏付けられた（b21 §6 の設計示唆と整合）。
- **StartQuest/CompleteQuest の構造は v97 の記載を更新する**: v97 §1.1 は `FUN_00529820(1)` を「StartQuest 本体」と見なしていたが、実体は **`+0x3C` フラグ bit 操作 + vt+0x40 通知の thunk** である。クエスト開始の一連動作（スクリプト起動・CTDA 実行・参照/ダイアログ解決）は `0x00529860` にあり、こちらは **E8 呼出し 0 件**（vt 経由と推定）。
- SetUnconscious/SetRestrained は **状態機械 `[this+0xB0]`（0/3/5/6）の操作**であり、b21 が D 類で扱った impl 側ゲート（`0x004FAE80`＋RTTI）と「3D ノード callback `0x005E02B0`」が噛み合う順序が確定した。

## §2 R1/R2 helper の a2/a3 読手特定（v97 §6-2）

### 2.1 呼び出し契約（再整理）

| 経路 | a1 | a2 | a3 | a4 |
|---|---|---|---|---|
| opcode 実行（`FUN_004FAB20`/`FUN_005169A0` → impl → helper） | 対象 ref（param_3） | **0** | **0** | 結果 `double*`（in_stack_0000001c） |
| CTDA 条件評価（`FUN_0056AC50` → +0x20） | subject（ref） | **ref2**（条件の第2オブジェクト） | **param+0x10**（条件パラメータ列への ptr） | 結果 `double*` |

### 2.2 手法

- 対象: v97 helper 111 件のうち **R1 89 件**（v97_helper_rows.json の `cond==true`）＋ **R2 スタブ 3 件**（`0x4F8910/8930/8950`）＋ **R2 実体 3 件**（`0x4F61A0/62A0/6350`）。計 95 VA。
- esp 相対アクセスを **2 モデル**で追跡: (std) call 直前の連続 push-run を callee pop とみなす／(cd) push を残存させる。`[esp+d]` を E0 基準の rel へ換算し rel==8(a2)/0xC(a3) の READ/WRITE を全採取（`lea` も address-taken として記録）。
- 候補は人手で全文ディスアセンブルして確定（自動モデルは引数スタック再利用を誤検出しうるため）。

### 2.3 結果 [確定]

| 集合 | a2 読手 | a3 読手 | 備考 |
|---|---|---|---|
| R1 89 件 | **0** | **0** | 自動スキャンの候補 2 件はいずれも**誤検出** |
| R2 スタブ 3 件 | 書手（`mov [esp+8], [0xB333C4]`） | 書手（`mov [esp+0xC], 0`） | CTDA 経路の ref2/param を**上書きして正規化**してから実体へ尾ジャンプ |
| R2 実体 3 件 | **3/3 読む**（比較相手） | 0 | SameFactionAsPC 全文で確認（下記） |

**誤検出 2 件の切り分け（証拠）**

- `0x004F4A00`（GetGold）: 自動スキャンは `mov [esp+0x1c],eax` / `fild [esp+0x1c]` を a2/a3 としたが、全文ではこれは **int→f64 変換用の scratch**（`call 0x4869C0`(count 取得) 直後の `mov [esp+0x1c],eax; fild; fstp [ebx]`）。実際の arg 読みは prologue の `mov ebx,[esp+0x14]`（k=4 → **a4 = out**）と `mov esi,[esp+0x10]`（stdcall 引数 pop 後 k=0xC → **rel 4 = a1 = subject**）。a2/a3 への実アクセスなし。
- `0x004F8F30`（IsInCombat）: 同型の scratch（`mov [esp+0x1c],eax; fild`）のみ。arg 読みは `mov ebx,[esp+0x14]`（a4）と `mov esi,[esp+0xC]`（k=8 → **a1**）。a2/a3 なし。
- 共通パターン: R1 helper は **a1（subject）と a4（out double*）だけ**を使い、残りはスタックスロット再利用（compiler の既知挙動）。

**R2 実体の a2 読手（証拠・`0x004F61A0` SameFactionAsPC 全文）**

```
fldz / fstp [ebp]                ; ebp = a4 (out) 既定 0.0
mov edi,[esp+0x10]               ; k=8 → a1 = subject
  NULL + IsActor(vt+0x190) ガード
mov ebx,[esp+0x14]               ; k=8 → rel 8 = **a2 = 比較相手**
  NULL ガード → 両者 0x5E02E0(1)（所属リスト取得）+0x3C 連結リスト走査
  0x467510（rank 比較）で一致時 *out = 1.0
  cmp ebx,[0xB333C4] / sete cl   ; 相手が PC なら専用フラグで rank 取得
  トレースゲート 0xB361AC → 'SameFaction >> %0.2f'
mov al,1 / ret
```

- `SameRaceAsPC`(0x4F62A0) / `SameSexAsPC`(0x4F6350) も同型（`mov esi,[esp+0x14]` = a2 読み）。
- **opcode 経路**では impl が `FUN_004f61a0(param_3, DAT_00b333c4, 0, out)` と **a2=g_thePlayer を直接渡す**（v97 §1.1 FAM-10 例）。**CTDA 経路**ではスタブが ref2 を捨てるように a2=g_thePlayer を強制。→ **a2 の語義は「比較相手」、両経路とも正規化されて PC**。a3 は 111 件全員で読手 0（スタブが 0 を書くだけ）。

### 2.4 v97 §6-2 への裁定

**「opcode 実行経路は 0・CTDA 経路は実値」という値差は、helper 側の意味論に影響しない**:
- R1 89 件: a2/a3 不読 → 差は不可視（design上、単一 subject 型ゲッタ）。
- R2 3 件: a2 は読むがスタブ/impl が常に PC を注入 → CTDA の ref2 は捨てられる（SameXxxAsPC の名の通り PC 固定比較）。
- a3（param+0x10）を読むのは **+0x20 を持つ式ゲッタのうち本族外のもの**（例: b22 の GetScriptVariable 0x4F6710 等が param 列を読む型）であり、本族 111 件には該当なし。

## §3 帯 A/B の +0x20 消費コード（v97 §6-3）

### 3.1 表側の事実 [確定]

- `opcode_table.json`（533 行、idx532 は sentinel `"ADD NEW FUNCTIONS BEFORE THIS ONE!!!"`）で帯別集計:
  - 行数: 帯 A(0x00–0x1E)=**31** / 帯 B(0x100–0x183)=**132** / 帯 C(0x1000–0x1170)=**369** / sentinel=1。
  - **cond(+0x20)≠0 = 169 行は全員帯 C**。帯 A/B の cond≠0 は **0 行**（v97 §4.4 の「+0x20==0 は 23 件」を表全体へ拡張した形）。

### 3.2 コード側の探索（3 系統）

1. **imm32 フルスキャン**（.text 6,450,233 B、全バイトオフセット）: 7 基底の命中数は
   `0xB0AF48` 表基底=1（`0x4FF906` FUN_004FF710 の `lea eax,[ebx+0xB0AF48]`）／
   `0xB0AF60` 帯A impl 基底=1（`0x516A5F` FUN_005169A0）／
   **`0xB0AF68` 帯A cond 基底=0** ／ `0xB0B420` 帯B entry 基底=5 ／
   **`0xB0B440` 帯B cond 基底=0** ／ `0xB0C8C0` 帯C entry 基底=6 ／
   `0xB0C8E0` 帯C cond 基底=2（`0x56ACA8` FUN_0056AC50、`0x56AE85` FUN_0056AE20）。
2. **全セクション dword スキャン**: `.data`/`.rdata` 等に上記 6 アドレス値を含むセルは **0 件** → ポインタ・グローバル経由の間接消費も**不存在**。
3. **実行時テーブル読者 7 関数の列読みインベントリ**（本調査で全数確定）:

| 関数 | 役割 | 読む列（offset） | +0x20 読み |
|---|---|---|---|
| `FUN_004FAB20` | bytecode `'X'` 実行（帯B/C） | +0x18 impl（indirect call） | なし |
| `FUN_005169A0` | 帯 A インライン実行 | +0x10 flagA / +0x14 argdesc / **+0x18 impl**（`[eax+0xB0AF60]`） | なし |
| `FUN_00516830` | 診断パス（`0x4FCA30` caller） | +0x00 name（"Unknown reference object for function '%s'"） | なし |
| `FUN_004FCA30` | entry 解決（帯B/C） | （解決のみ） | なし |
| `FUN_004FDAF0` | コンパイラ側検証 | +0x10 flagA / +0x12 argn / +0x14 argdesc / +0x1C dispatcher | **なし**（基底は帯B/C entry 基底のみ） |
| `FUN_004FF710` | 引数コンパイラ | +0x10 / +0x14 / +0x1C（dispatcher call）。本文中の `[edi+0x20]`/`[+0x24]` は **emit バッファ構造体**（`mov word [eax+ecx],...` のアドレス計算）であり **entry ではない**（`0x500032` 帯の全文で確認） | **なし** |
| `FUN_00501540` | **コンソール dump コマンド**（新同定） | 帯B 132行/帯C 369行 を走査し +0x00 name / +0x04 alias / +0x0C desc を印字（`'----CONSOLE COMMANDS----'` / `'----SCRIPT FUNCTIONS----'` / `'%s (%s) -> %s'` / `'%s -> %s'`、E8 caller 0 件＝表経由コマンド） | **なし** |
| `FUN_0056AC50` | CTDA 条件評価 | **帯 C の +0x20**（`[eax+eax+0xB0C8E0]` → direct call） | **あり（帯C のみ）** |
| `FUN_0056AE20` | CTDA 診断/名前参照 | 帯 C +0x20（`[ecx*8+0xB0C8E0]` 比較）＋帯 C +0x00 name（エラー文 `'...Function "%s" is not a...'`） | **あり（帯C のみ）** |

### 3.3 裁定 [確定]

**帯 A/B の +0x20 を読むコードは存在しない。** 理由は三层:
1. 表側で帯 A/B の全 163 行が cond==0（値として読む意味がない）。
2. cond 列の絶対基底（`0xB0AF68`/`0xB0B440`）への参照が .text に 0 件、間接経路（.data ポインタ）も 0 件。
3. entry 解決後の +0x20 読みは FUN_0056AC50/0056AE20 のみで、両者は帯 C 基底をハードコード。

→ v97 §4.4「値を返すオペコードは +0x18 と +0x20 に同一 helper を置く」は**帯 C 専用の構造**である。帯 A（イベント帯）・帯 B（コンソールコマンド帯）は値を返さない命令帯であり、+0x20 列が構造的に未使用であることが**正**で確定した（バグや未実装ではなく設計上の排他）。

## §4 新規残点（V143+ 粒度）

| # | 内容 | 手掛かり |
|---|---|---|
| V143-a | `0x00529860`（クエスト開始実行体）の**呼び出し元**。E8 0 件＝vt/表経由と推定。どの vtable スロットか（TESQuest +0x48 相当?）と、`+0x3C` byte の bit2 以降・`+0x40`/`+0x48` リストの要素型名 | §1.1 StartQuest 行 |
| V143-b | `0x005E6680` の状態値 **6** の意味論（base+0x28 bit1 保持者での state 1/2 強制遷移先）と `[this+0xB0]` の正式フィールド名 | §1.1 共有先行 |
| V143-c | ToggleFlyCam の this オブジェクト（`+0x748` フライキャンバイト／`+0x5D4` 速度）の型名と vt+0xEC getter の名前。`0xB35C24`（+0x1A フラグ・vt+0x98）= デバッグ描画マネージャのクラス名 | §1.1 該当行 |
| V143-d | PathGrid 表示の実装本体 `0x004E71A0`（259 ins, show）/`0x004E5400`（78 ins, hide）の内部（`0x70B780`/`0x707610`/`0x707370` ノード帯）と、`0xB3C08A`（TogglePathLine）を読む描画コード | §1.1 該当行 |
| V143-e | `FUN_00501540`（opcode dump コマンド）のコンソールコマンド登録名と登録表（E8 0 件・表/VT 経由） | §3.2 |
| V143-f | `0x004DA2A0`（PrintNPCDialog の名称化ヘルパ、RTTI 15 ins）の返す型（base form name? full name?） | §1.1 PrintNPCDialog 行 |
| V143-g | +0x24 列（argflag）の消費者。本調査では FUN_004FF710 の `[edi+0x24]` が emit バッファであることを確認したのみ（列としての読者は未探索） | §3.2 |
| V143-h | v97 §6-5 の残り: R1 でトレース文字列なし 3 件（`0x4F57D0`/`0x4F57F0`/`0x4F85E0`）の命名裏付け（本稿未着手） | v97 §6-5 |
| V143-i | R1 helper の arg スロット再利用（`[esp+0x1c]` scratch 化）が MSVC の普遍規則かの確認（本稿は候補 2 件の人手切り分けのみ） | §2.3 |

## §5 証拠一覧（すべて `%TEMP%\opencode\`、既存ファイル非変更）

| ファイル | 内容 |
|---|---|
| `v134_a2a3.py` / `v134_a2a3_out.txt` | R1 89+R2 6 VA の esp 追跡 2 モデル・a2/a3 アクセス全採取 |
| `v134_r6.py` / `v134_r6_out.txt`（756 行） | R6 16 件の完全 disasm＋特徴量、decompiled.c 索引（7,251 関数）＋R6/callee 突合、callee level-1 列挙 |
| `v134_band20.py` / `v134_band20_out.txt` | 帯別 cond 集計、.text imm32 フルスキャン（7 基底）、`FUN_004FCA30` caller 3 件の文脈 |
| `v134_follow.py` / `v134_follow_out.txt`（1,025 行） | GetGold/IsInCombat 全文（誤検出切り分け）、raw hit 全アドレス、FUN_00501540/004FDAF0/004FF710/005169A0/0056AE20 の列読み、R6 小関数 10 件の全文（StartQuest thunk 等） |
| `v134_win.py` / `v134_win_out.txt`（658 行） | FUN_00501540 全文（dump コマンド確定）、帯基底ウィンドウ 5 点、深層 callee 6 関数、R2 実体 `0x4F61A0` 全文 |
| `v134_final.py` / `v134_final_out.txt` | FUN_004FF710 の entry/バッファ識別、全セクション dword スキャン（6 アドレス=0 件）、PrintNPCDialog 全文 |
| （補助 xref probe） | `0x529860` xref=0 / `0x5E6680` xref=21 / `0x501540` xref=0 / `0x440510` xref=2 の実測 |

### 5.1 自己検証

```
着手時: python "%TEMP%\opencode\probe_harness.py" selftest → 4/4 PASS
完了時: python "%TEMP%\opencode\probe_harness.py" selftest → 4/4 PASS
```

| 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|
| 行数・帯別 cond（cache/opcode_table.json） | cond≠0=169 全て帯 C | 169 全て帯 C（帯A 0/帯B 0） | OK（v97/b22 と一致） |
| 帯A cond 基底 `0xB0AF68` / 帯B `0xB0B440` の .text 参照 | 0 | 0 / 0 | OK |
| +0x20 消費者 | 0x56AC50 / 0x56AE20 のみ | 2 関数のみ（`0xB0C8E0` hit = 2） | OK（b22 と独立一致） |
| R1 の a2/a3 読手 | 0（仮説） | 0（候補 2 件を全文で誤検出と証明） | OK |
| R2 実体の a2 読手 | — | 3/3（`[esp+0x14]`=a2、a3=0） | 記録 |
| R6 本文の decompiled 収録 | — | 16/16 ABSENT（capstone 代替） | 記録 |
| `0x440510` の E8 xref | v97: 2 | 2（`0x500E36`/`0x500E4D`＝同一 impl） | OK |

### 5.2 制約・既知 limitation

- R1 スキャンは `[esp+idx*N+d]` 形式（index≠0）を arg slot 判定から除外（本集合では該当なしと判断したが機械的網羅ではない）。
- esp 追跡の std/cd 2 モデルは stdcall/cdecl 混在下で偽陽性を生みうる。本稿は**全ての候補を全文人手確認**して解消済み。
- ESM 走査は禁止事項のため未実施（b21 の頻度データを引用のみ）。

## §6 継続セッション（V134-2 / 2026-10-27・残件 (4) 無トレース文字列3件の命名裏付け）

- 状態: 著手中（§0〜§5 は前セッション完了分）。本節は TODO 行の (4) `0x004F57D0` / `0x004F57F0` / `0x004F85E0` の命名裏付けに专当する。
- 手法: `tmp/re/cache/` の共有キャッシュを先に確認（再走査禁止）→ `opcode_table.json` の impl(+0x18)/cond(+0x20)/name(+0x00) 列から逆引き → 補助ヘルパ本文を capstone で精査 → v97/b22 の族ラベルと突合。
- selftest: `python "%TEMP%\opencode\probe_harness.py" selftest` → 4/4 PASS（継続開始時）

### 6.1 無トレース3件の表側情報（cache 再利用・再走査なし）

`tmp/re/cache/opcode_table.json` から該当3行（`name`/`desc`/`impl`/`cond` はすべて GT どおり）:

| idx | opid | name | alias | desc | flagA | argn | argdesc | impl(+0x18) | cond(+0x20) | argflag |
|---:|---:|---|---|---|---:|---:|---:|---|---|---:|
| 388 | 0x10E1 | GetPersuasionNumber | '' | '' | 0 | 0 | 0x0 | `0x505930` | `0x4F57D0` | 0x0 |
| 478 | 0x113B | GetTotalPersuasionNumber | '' | '' | 0 | 0 | 0x0 | `0x505950` | `0x4F57F0` | 0x0 |
| 481 | 0x113E | GetIdleDoneOnce | '' | '' | 0 | 0 | 0x0 | `0x5063B0` | `0x4F85E0` | 0x0 |

- `desc(+0x0C)` は3件とも**空**（共有空文字）。よって **命名の一次証拠は表側に存在しない**（v97 §6-5 の認識どおり）。
- Ghidra 台帳（`cache/named_funcs.txt`）: helper 3件は**未登録（absent）**、impl 3件は登録済（`FUN_00505930` / `FUN_00505950` / `FUN_005063b0`）。
- `docs/Oblivion_decompiled.c`（7,251 関数索引）: **helper 3件・impl 3件とも ABSENT**（= R6 16件と同じく v97 §5.1 の未登録94件側。capstone 本文が唯一の証拠）。
- **impl 3件はすべて9命令の forwarder** で、`[esp+0x1C]`(a4=out)・`[esp+0x0C]`(a1=subject) を読まずに `push a4 / push 0 / push 0 / push a1 / call helper / add esp,0x10` する → §2.1 の「opcode 実行経路は a2=0, a3=0」を **3/3 で impl 側から独立確認**（追加整合）。

### 6.2 helper 3件の本文（capstone・ret+パディング境界）

```
GetPersuasionNumber      0x4F57D0  body=[0x4F57D0,0x4F57EE) 12 ins
  fldz / mov esi,[esp+0x14](a4) / fstp qword[esi]        ; *out = 0.0 (既定)
  call 0x57BDE0                                          ; -> PersuasionMenu* (下記)
  test eax,eax / je skip
  fild dword ptr [eax + 0xF4] / fstp qword[esi]          ; *out = (double)menu->field_F4
  mov al,1 / pop esi / ret                               ; 成功=true
GetTotalPersuasionNumber 0x4F57F0  body=[0x4F57F0,0x4F580E) 12 ins
  同型。読むのは dword ptr [eax + 0xF8]                   ; *out = (double)menu->field_F8
GetIdleDoneOnce          0x4F85E0  body=[0x4F85E0,0x4F8622) 32 ins
  fldz / mov esi,[esp+0x8](a1) / test esi / mov edi,[esp+0x18](a4) / fstp qword[edi]
  je out0
  vt+0x190 (IsActor) ガード -> je out0
  mov ecx,[esi + 0x58] / test ecx / je out0              ; +0x58 = BaseProcess 系（§6.4）
  vt call [eax + 0x64]  (無引数・al を読む)               ; = family slot25 bool getter（§6.4）
  test al,al / je out0 / fld1 / fstp qword[edi] / return 1
  out0: mov al,1 / return 1
```

- 3件とも **helper 本文内に `0xB361AC`（トレースゲート）参照は 0 件**（正確な body 範囲で imm32 検索 = 0/0/0）、trace 文字列・その他グローバル参照も 0 件。→ **「無トレース」は本文で確定**（v139 §6-5 のタグ無し3行の本文根拠。v139 の「3行とも cond に 0xB361AC 実在」は helper 本文ではなく **cond 実行経路（`FUN_0056AC50`/`FUN_0056AE20` 側）** と解釈するとき矛盾しない。b22 §9-1 の裁定（b10 の159タグはオミット・表記は正しい）と独立に一致）。

### 6.3 GetPersuasionNumber / GetTotalPersuasionNumber — 命名裏付け [確定]

**`0x57BDE0` = PersuasionMenu の RTTI dynamic_cast getter**（22 命令）:

```
0x57BDE0: push 0x40A          ; menu id 0x40A = PersuasionMenu (RE_EXE_REPORT L5627)
          call 0x589B70       ; GetMenuByID(id) gate
          mov ecx,eax / xor eax,eax / test ecx,ecx / je ret0
          push eax            ; 0
          push 0xB13384       ; = .?AVPersuasionMenu@@ TypeDescriptor（vtable_index n1169 typedesc 列で裏取り）
          push 0xB0A018       ; = .?AVMenu@@ TypeDescriptor（b07 L1033 / s56 L72 で同定）
          push eax            ; 0
          call 0x5898F0       ; menu object 取得
          push eax
          call 0x9832E6       ; __RTDynamicCast(src, 0, Menu TD, PersuasionMenu TD, 0)
          add esp,0x14
          ret                 ; eax = PersuasionMenu* (失敗時 0)
```

- **`push 0xB13384` の `.text` 全体出現 = 1 箇所のみ（`0x57BDF6`）** → この EXE で PersuasionMenu へ dynamic_cast するコードは getter 1本だけ。
- よって:
  - **GetPersuasionNumber = `PersuasionMenu+0xF4` を int→f64 で返す**（メニュー未開放/非Persuasion なら 0.0）。
  - **GetTotalPersuasionNumber = `PersuasionMenu+0xF8` を int→f64 で返す**。
- **書手の裏付け**: PersuasionMenu 実装帯 `0x5BD000..0x5C3000` で `+0xF4/+0xF8` に触れるのは3関数のみ:
  | 関数 | アクセス | 文脈 |
  |---|---|---|
  | `FUN_005BF470`（446 ins, SEH, `mov esi,ecx`） | `add dword [esi+0xF8], edi` / `mov dword [esi+0xF4], edi` | `call 0x589B70`+`call 0x5898F0`（メニュー取得）、`[0xB333C4]`=g_thePlayer と `[esi+0xD8]` の双方から `vt+0x284`/`vt+0x374` を呼ぶ**説得の点数計算**、直後に g_thePlayer+0x44 の ExtraData（`0x41E6F0`）経由 `0x491700` |
  | `FUN_005BFB90` | `add [esi+0xF8], edi` / `mov [esi+0xF8], ebx` | 同帯（メニュー内処理） |
  | `FUN_005C1F70` | `mov eax,[edx+0xF8]` ×3（読手） | 同帯 |
- **+0xF4 = 直近の獲得値、+0xF8 = 累積値**（add/mov のペア）であり、名前 `Number` / `TotalNumber` と**構造が一致**。

### 6.4 GetIdleDoneOnce — 命名裏付け [ほぼ確定・+0xC8 の語義は残点]

呼ぶ `vt+0x64`（slot 25）の実装を vtable_index 1,715 本全件で特定:

| 対象 | vtable | slots | slot25 impl | 本文 |
|---|---|---:|---|---|
| **BaseProcess / LowProcess / MiddleLowProcess** | 0xA6F464 / 0xA720A4 / 0xA72D64 | 322/347/348 | **`0x69D990`** | `xor al,al / ret`（常に false・無引数） |
| **MiddleHighProcess / HighProcess** | 0xA72684 / 0xA71814 | 358/358 | **`0x629520`** | `mov al, byte [ecx+0xC8] / ret`（**無引数 bool getter**） |
| NiAVObject / NiNode / BSFadeNode | 0xA7DF24 / 0xA7E38C / 0xA3F944 | 33/39/39 | `0x7075B0` / `0x70A190` / `0x4A0D50` | **float 引数1つ（`ret 4`）の Update 系** → GetIdleDoneOnce の**無引数 call とは不整合** |
| Actor / Character / TESObjectREFR / MobileObject / PlayerCharacter | — | — | `0x68F970` | `ret 4`（nop 相当） |

- **GetIdleDoneOnce の呼出し形式（push なし・`test al,al`）は Process family slot25 と完全整合**し、Ni 系 Update（float 引数）とは不整合 → 呼び出し先は **Process family** と判定できる。
- その根拠は b08 と一致: **`[actor+0x58]` = BaseProcess 系**（b08_container_owner.md:234/240 — `FUN_005E0AC0` が `[ecx+0x58]` の先で `vt+0x430`(slot 268) を呼ぶ＝269 スロット以上は Process 5クラスのみ、図示 `+0x58 ─► BaseProcess / Low / MiddleLow / MiddleHigh / High`）。
- **Process family の vtable impl 全走査（1,733 エントリ）でフィールド `+0xC8` に触れる impl は4本のみ**（`slot = {25 getter, 26 setter(0x629530: mov [ecx+0xC8],al), 18 init(mov [ebp+0xC8],0), 330 cmp}`）→ `+0xC8` は **bool フィールドで、slot25/26 が getter/setter ペア**。High/MiddleHigh のみが実装＝低段階プロセスは常に false。
- 判定: **「High/MiddleHigh プロセスの bool フィールド +0xC8 を返すゲッタ」**であり、`GetIdleDoneOnce` の名（アイドルアニメの1回終了）と**構造整合**（b10 の `bool` 型判定・`fld1/fldz` と一致）。**+0xC8 が idle 終了フラグである直接証拠（setter `vt+0x68` 呼出し元の文脈）は未取得 → §7 残点 V144-b**。

### 6.5 訂正（本稿 §1.1 および b21 の記述）

- 本稿 §1.1 `0x005E02B0` 行および b21_speculative_backing.md:152 の **「`[this+0x58]`（3Dノード）」は誤り**。b08 の根拠（vt+0x430/0x434 は Process 系のみ実装）に従い **正: `[this+0x58]` = BaseProcess 系（プロセスオブジェクト）**。b21 の `vt+0x434`（SetAlpha）も Process family のスロット呼び出しとして読むべき（既存ファイルは編集禁止のため記録のみ）。
- 帯に伴う影響: GetIdleDoneOnce の `vt+0x64` が Ni ノードではなく **Process slot25** であることが §6.4 で確定したことで、v97 §6-5「命名の裏付けがない」のうち2件（Persuasion 2件）は**裏付け済**、1件（GetIdleDoneOnce）は**構造整合まで確定・フィールド語義のみ残存**。

### 6.6 vt slot 調査の副次統計（追加 GT）

- `vt+0x64` の「`mov r,[r+0x64]` → `call r`」idiom は `.text` 全体で **46 サイト / 36 関数**（skipdata 線形走査）。所有者は Ni 帯（`0x7073A0`/`0x709E30`/**`0x70A190`**/`0x77xxxx`/`0x78xxxx`）＋Process 帯（`0x4F8590` 近接=GetIdleDoneOnce 自身を含む）＋`0x4046A0`(3) 等。
- vtable 1,715 本の slot25 impl 分布 = **66 種**。最多は `0x68F970`（87 本, `ret 4`）→ `0x7155A0`(52) → **`0x69D990`(25, Process 定番 `xor al,al; ret`)** → `0x8B6950`(23) → `0x7075B0`(22, NiAVObject)。

### 6.7 vt+0x68（Process slot26 setter）呼び出し元調査（+0xC8 語義の裏付け試行）

- `.text` 全体（skipdata 線形走査）で **`mov r,[r+0x68]` → `call r` idiom = 58 サイト / 51 関数**、`call [reg+0x68]` 直接形 = **38 サイト**。
- owner は Ni/Havok 帯（`0x70A280`/`0x722A10`/`0x76xxxx`/`0x88-0x8C`）・メニュー/GL 帯・ゲームロジック帯（`0x5E0270`/`0x5E0B70`/`0x5E0C20`/`0x505FE0`/`0x512CA0`）に分散 → **vt+0x68 はクラス依存の汎用スロット**であり、Process setter `0x629530` を指すものだけを owner から判別するには「`[x+0x58]`（BaseProcess 経由）を先に読む」経路に限定する必要がある（本稿では未実施 → V144-b）。
- 参考: owner のうち idle に近い語を持つもの `0x00512CA0`（文字列 `'SpecialIdle_HitEffect'` ＋ `'Visual effect has been applied to %s'`）、`0x0055D8C0`（`'bAnimationUpdate = %s'`）。
- **`+0xC8` 語義は現時点で「Process family の bool フィールド（getter=vt+0x64 / setter=vt+0x68 / ctor で0初期化 / slot330 で比較）」まで**。これが idle アニメの1回終了フラグである直接証拠は未取得。

## §7 継続セッションの結論（3行）

1. **無トレース3件の命名裏付け: 2件は確定、1件は構造整合まで**。`GetPersuasionNumber`/`GetTotalPersuasionNumber` は `0x57BDE0`（menu id `0x40A` → `__RTDynamicCast` で `.?AVPersuasionMenu@@` へ cast。この cast は `.text` 全体で1箇所のみ）が返す **PersuasionMenu の `+0xF4`/`+0xF8` を int→f64 で返す**ゲッタで、同帯 `0x5BF470`/`0x5BFB90`/`0x5C1F70` が add/mov/読する **獲得値/累積値** と名前が一致。`GetIdleDoneOnce` は `subject+0x58`（BaseProcess 系）の **vt slot25 = Process family の bool getter（`[process+0xC8]`）を無引数で呼ぶ**ゲッタ（Ni 系 Update は float 引数で不整合）。
2. **「無トレース」の理由は本文確定**: 3件の helper 本文（ret+パディング境界、12/12/32 命令）に `0xB361AC` 参照・文字列・グローバルはいずれも **0 件**。impl 3件は9命令 forwarder（a2=0/a3=0）で trace も持たない。→ §2.1 の呼び出し契約を impl 側からも 3/3 独立確認。
3. **訂正 1 件**: 本稿 §1.1 および b21 の「`[this+0x58]` = 3Dノード」は誤りで **正は BaseProcess 系**（b08 の vt+0x430/0x434 根拠に整合）。GetIdleDoneOnce の呼出し形式（無引数 bool）がこの訂正を裏側から支持する。

## §8 残留不明点（V144+ 粒度・§4 の追補）

| # | 内容 | 手掛かり |
|---|---|---|
| V144-a | `0x57BDE0` 内の `0x5898F0(menu obj 取得)` の正確な仕様（引数 `(0, MenuTD, PersuasionMenuTD, 0)` は dynamic_cast の形と重複し、`call` 直前の push-run は2つの call で共有される）と、`0x5898F0` が返す実体（TileMenu? Menu?） | §6.3 |
| V144-b | `Process +0xC8` が「idle アニメの1回終了フラグ」であることの直接証拠。setter = slot26 `0x629530`（`mov [ecx+0xC8],al`）を呼ぶ `[x+0x58]` 経路の特定（vt+0x68 idiom 58/38 サイトのうち Process 経由に限定） | §6.4 / §6.7 |
| V144-c | `[actor+0x58]` の**正式フィールド名**（b08 は「BaseProcess 系であること」を vt slot 268 で証明済だが、フィールド名は b08 残点と重複して未確定） | §6.4 |
| V144-d | `FUN_005BF470` 等3関数が PersuasionMenu の**メンバー（this=esi）である**ことの確定（vtable 15スロット impl には非出現、dynamic_cast 経由で this を得る経路が未特定）。`+0xF4/+0xF8` の符号性（`fild` は s32 扱い） | §6.3 |
| V144-e | `vt+0x64` idiom 46 サイトの「Ni 系 / Process 系」内訳の確定（both=4 件 `0x498340`/`0x4F8590`(GetIdleDoneOnce 含む)/`0x779210`/`0x7AD270` の this 経路別判定） | §6.6 |
| V144-f | v139 §6-5「3行とも cond に `0xB361AC` 実在」の文言。**helper 本文では 0 件**が実測済み → cond 実行経路側の意味と確定したい（v139/b10 は既存ファイル・記録のみ） | §6.2 |
| V144-g | ShowBirthSignMenu（§1.1）の実到達 XML、`FUN_00501540` のコマンド登録名 など §4 の既存残点は変更なし | §4 |

## §9 証拠ファイル（継続セッション分・すべて `%TEMP%\opencode\`）

| ファイル | 内容 |
|---|---|
| `v134_append0.py` / `v134_append1.py` / `v134_append2.py` | 本稿 §6〜 の追記（append-only） |
| `v134_name3.py` / `v134_name3_out.txt` | opcode_table 3行全列＋argdesc、named_funcs、decompiled 索引突合、helper/impl 全文 disasm |
| `v134_name3b.py` / `v134_name3b_out.txt` | `0x57BDE0` 全文（menu 0x40A gate → RTTI cast）、E8 caller=helper 2件のみ、imm32 参照 0 件 |
| `v134_name3c.py` / `v134_name3c_out.txt` | PersuasionMenu vtable 15スロット列挙、`push 0xB13384` = 1件（`0x57BDF6`）、(途中 `skipdata` kwarg エラー → d で再実施) |
| `v134_name3d.py` / `v134_name3d_out.txt` | 3件の **正確な body 範囲**（AC=0/0/0）、Persuasion 帯 `0x5BD000..0x5C3000` の `+0xF4/F8` アクセス 7件/3関数、vt64 初版走査（skipdata 途中エラー → e/f で再実施） |
| `v134_name3e.py` / `v134_name3e_out.txt` | `FUN_005BF470` 446命令の文脈（メニュー取得・g_thePlayer・`vt+0x284/0x374`・`+0xF4/F8` 書換）、vt idiom 初版（enclosing 台帳欠落の影響を確認） |
| `v134_name3f.py` / `v134_name3f_out.txt` | `vt+0x64` idiom **46サイト/36関数**の owner 列挙、兄弟3関数＋GetIdleDoneOnce の全文 |
| `v134_name3g.py` / `v134_name3g_out.txt` | vtable **1,715本の slot25 impl 分布 = 66種**、NiAVObject/NiNode/BSFadeNode/Actor 等の slot25 本文（Ni=Update(float) / Actor系=`ret 4`） |
| `v134_name3h.py` / `v134_name3h_out.txt` | **Process family 5クラスの slot25**（`0x69D990`=`xor al,al;ret` / `0x629520`=`mov al,[+0xC8];ret`）、vtable impl 1,733 中 `+0xC8` 触点 = **4本（slot 25/26/18/330）** |
| `v134_name3i.py` / `v134_name3i_out.txt` | `vt+0x68` idiom 58サイト/51関数＋`call [x+0x68]` 38サイトと owner 文脈（文字列/呼出し先） |

## §10 自己検証（継続セッション）

```
着手時:   python "%TEMP%\opencode\probe_harness.py" selftest → 4/4 PASS
継続開始: python "%TEMP%\opencode\probe_harness.py" selftest → 4/4 PASS
完了時:   python "%TEMP%\opencode\probe_harness.py" selftest → 4/4 PASS
```

| 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|
| opcode_table 3行（idx 388/478/481）の impl/cond | `0x505930`/`0x4F57D0`、`0x505950`/`0x4F57F0`、`0x5063B0`/`0x4F85E0` | 3/3 一致（alias='' desc='' argn=0 argflag=0 も GT どおり） | OK |
| helper 3件の本文長（ret+パディング境界） | — | 12 / 12 / 32 命令（`0x4F57EE` / `0x4F580E` / `0x4F8622` 終端） | 記録 |
| helper 本文内の `0xB361AC` | 0（仮説） | **0 / 0 / 0**（正確範囲での imm32 検索） | OK（V139 文言の本文側裏付け） |
| `push 0xB13384`（PersuasionMenu TD）の `.text` 出現 | 1 | **1（`0x57BDF6`）** | 記録 |
| PersuasionMenu TD `0xB13384` / Menu TD `0xB0A018` の同定 | vtable_index n1169 typedesc / b07 L1033 | 両者とも一致（s56 の `__RTDynamicCast` 形式と整合） | OK |
| `0x57BDE0` の E8 caller | helper 2件 | **2（`0x4F57D9`/`0x4F57F9`）**、他経路 0 | OK |
| Persuasion 帯の `+0xF4/+0xF8` 触点 | — | 7 hits / 3 関数（`0x5BF470` 加算+書換、`0x5BFB90`、`0x5C1F70` 読手） | 記録 |
| impl 3件の forwarder 引数 | a2=0, a3=0（§2.1） | 3/3 `push 0 / push 0 / push a4 / push a1` | OK（追加整合） |
| decompiled 収録（helper+impl 6件） | — | **6/6 ABSENT**（capstone 代替） | 記録 |
| Process family slot25 | 無引数 bool | Base/Low/MiddleLow=`0x69D990`(`xor al,al;ret`)、MiddleHigh/High=`0x629520`(`mov al,[+0xC8];ret`) | OK（呼び出し形式と整合） |
| `+0xC8` 触点（Process vtable impl 1,733 エントリ） | getter/setter pair | **4本 = slot25(get)/26(set `0x629530`)/18(init 0)/330(cmp)** | OK |
| vtable 1,715 本の slot25 impl 種類 | — | 66 種（最多 `0x68F970`=87本 `ret 4`、`0x69D990`=25本 Process 定番） | 記録 |
| `vt+0x64` idiom / `vt+0x68` idiom（.text 全走査） | — | 46 サイト/36関数 ／ 58 サイト/51関数＋直接形 38 | 記録 |
| 本稿 §1.1・b21 の「`[this+0x58]`=3Dノード」 | 訂正 | **BaseProcess 系**（b08 L234/240 と GetIdleDoneOnce 呼出し形式で独立整合） | 訂正記録 |

### 10.1 制約・既知 limitation

- 3件は decompiled 収録が無いため **capstone 本文のみ**（境界は ret+`CC`/`00` パディングで確定、非整列読みは行っていない）。
- `push`-run を2つの `call` が共有する MSVC パターン（`0x57BDF5..0x57BE0C`）の引数帰属は、`__RTDynamicCast(src,0,srcTD,dstTD,isRef)` の5引数形と s56 の既知例で確定させたが、`0x5898F0` 側の4引数解釈は V144-a に残す。
- ESM 走査は禁止事項のため未実施。

## §11 Android 移植への示唆

`GetPersuasionNumber`/`GetTotalPersuasionNumber` は説得メニュー（`PersuasionMenu`、id `0x40A`）の実装依存値であり、移植時はメニュー未実装でも「PersuasionMenu 相当のオブジェクトが存在するときのみ `+0xF4`（直近値）/`+0xF8`（累積値）を返し、それ以外は 0」の**契約だけ踏襲すればスクリプト互換性が保たれる**（実 UI が無ければ常に 0.0 が正解挙動）。`GetIdleDoneOnce` はプロセス階層（BaseProcess→MiddleHigh/HighProcess）に置く **bool フィールド1つ＋vt slot25/26（getter/setter ペア）** として再現すればよく、低段階プロセスでは常に false（`xor al,al; ret`）という既定値も移植側のプロセス段階設計に合わせればよい。いずれも移植コード側にメニュー/アニメの実装を先行させる必要はなく、**フィールドと既定値の対応表**として抽出しておけば十分である。なお本稿の訂正（`[actor+0x58]` は BaseProcess 系であって3Dノードではない）は、移植側で「actor の +0x58 をシーングラフノードと誤解して Node 参照として扱う」事故を防ぐ情報として重要である。
