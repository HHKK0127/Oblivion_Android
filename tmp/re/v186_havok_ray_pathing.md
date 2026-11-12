# v186_havok_ray_pathing.md — 束F: Havok RayCast/Pathing 継続束 (N4 / N7)

- 作成日: 2026-10-29 / 担当: TODO serial 174 (V174・中) の分析専任サブエージェント
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / TODO_re49.md / GT / session-memory / plan / cache / 既存 tmp/re 成果物は一切変更していない（読取のみ）。git 不使用。ESM 再スキャンなし。Ghidra 呼出なし（capstone 静的解析）。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam, ImageBase 0x400000)
- 入力（読取専用）: `TODO_re49.md`（N4/L93・N7/L96・M7/L87・K2/L76） / `v180_letter_rows_audit.md`（束F 行） / `s63_h2_raychar.md` / `s65_havok_char.md` / `s61_havok.md` / `s72_navm.md` + a1/a2/a32 traces / `s65_ai_follow.md` / `b13_nonrtti_vtable_classes.md`（bhkWorld@0xA95CF4・bhkWorldM@0xA97614） / `b26_nontt_class_names.md` / `cache/{named_funcs,call_targets,vtable_index}` / `s68_n2_context.md`（N2 は再調査せず踏襲）
- 中間スクリプト: `%TEMP%\opencode\v186_1..30_*.py`（30 本）。自己検証: re_util selftest 稼働 OK（callers/Disasm 確認）。
- 凡例: [確定] = 逆アセンブル直接確認 / [推定] = 論理推定 / [未] = 未解明。

---

## 0. 結論サマリ

| 項目 | 判定 |
|---|---|
| N4 前提矛盾（0x8A2650 = castRay vs 等値比較子） | **裁定: K2 訂正が正。0x8A2650 = 等値比較子 [確定]**。s62/s63 の「Shape::CastRay」は誤同定 |
| N4 本体 | **ゲーム側 RayCast 経路のコアは完全解明 [確定]**: `bhkWorld::PickObject 0x88B850`（vtable slot[34]=+0x88）→ `0x8BA1B0`（ワールド raycast 準備）→ **hkpWorld 系オブジェクト vtable slot[14]（+0x38）の castRay 仮想** → `0x8BA470`（幅広phase/shape レベル SSE 変換キャスト） |
| N4 テキスト手掛かり | `HavokPick:HAVOK` 0xA126A6 は**誤指引**（.text 内ソング thunk の中間バイト）。実体は INI 設定名 0xA378F8、かつ **bHavokPick は retail で死に設定（読取ゼロ）** [確定] |
| N4 ゲーム側呼出 | **path-node 妥当性検証ピック 0x68CB40（実行点 0x68CBC4）を確定**（N4↔N7 交差点）。LOS (0x1A)/ViewCaster (0x1D)/クロスヘア系の request 生成元は未特定（残差 R1） |
| N7 既存記述 | **s72 の状態機械記述を訂正**: 0x683A60/0x684780 は「状態機械分岐」ではなく**述語**。[+0x30] は PathQuery ではなく **回避オブジェクト（0x34B）** [確定] |
| N7 本体 | **直線移動（ウェイポイント 18° ショートカット）0x684000 と回避 FSM 8 状態名（表 0xB15728）を確定**。steering 本体 0x67EB60 帯と遷移関数が残差 |

---

## 1. N4: ゲーム側 RayCast 経路

### 1.1 裁定: 0x8A2650 は等値比較子である [確定]

s62「slot[11]=0x8A2650=Shape::CastRay」/ s63_h2 §1.2「slot[47]=hkShape::castRay と同一」**は誤り**。K2（2026-10-21 デコンパイル訂正「0x8A2650 = castRay ではなく等値比較子」）を**本束で逆アセンブルにより裏書**。

0x8A2650 全体（30 バイト、`ret 4` = thiscall 1 引数）:

```
008A2650  push esi / push edi
          mov edi, [esp+0xC]      ; arg1
          push edi ; mov esi, ecx ; call 0x89D6F0   ; 引数型チェック
          test al, al ; je ret_false
          mov eax, [edi+0x10]     ; arg1 のフィールド +0x10
          cmp eax, [esi+0x10]     ; this のフィールド +0x10
          sete al                 ; 等値なら 1
          pop edi / pop esi / ret 4
```

- 呼出元 3 件は全て Havok 内部（0x8B795D / 0x8C5759 / 0x8C6639）で bool 文脈（`test al` → jcc）。レイキャスト特有の ray 入出力（begin/end/hitFraction）は**存在しない**。
- vtable 参照 13 件（s65 記載と一致: 0xA55948/0xA55A88/0xA55B28/0xA55D78/0xA657C0/0xA65968/0xA96600/0xA991A8/0xA99258/0xA992F8/0xA99650/0xA99708/0xA99F90）。
- **bhkAabbPhantom v#1 (0xA559CC) slot[47] [+0xBC] = 0x8A2650 はスロット内容自体は正しい**（slot[44]=0x8A2600 / slot[46]=0x8A2610 と同族の小型比較子群）。誤りは「slot[47] = castRay」という**命名**のみ。slot[48] [+0xC0] = 0x8AE9A0（衝突委譲）は s63 の記載どおり。

### 1.2 `HavokPick:HAVOK` 0xA126A6 の正体 — 指引は誤り [確定]

- **0xA126A6 は文字列参照ではなく .text 末尾のソング thunk の中間バイト**である。0xA126A0 の thunk 全文:

```
00A126A0  push 0x00BA7C00
          push 0x00A981C8        ; "bhkCharacterProxy"
          mov  ecx, 0x00BA8024
          call 0x70E220           ; 名前付きシングルトン登録
          ret
00A126C0  push 0x00BA7F60
          push 0x00A95E5C        ; "bhkAabbPhantom"
          mov  ecx, 0x00BA8030
          call 0x70E220
          ret
```

  すなわち **bhkCharacterProxy / bhkAabbPhantom のグローバルオブジェクト（0xBA7C00 / 0xBA7F60）を名前登録するイニシャライザ thunk**。TODO N4 行の「0xA126A6 が手掛かり」は廃棄。

- 実在する `HavokPick:HAVOK` は **INI 設定名**（0xA378F8 = "bHavokPick:HAVOK"。キー "bHavokPick" / セクション HAVOK）。copilot_ini 系キャッシュ（`copilot_ini_defaults.txt` L451 等）と一致。

- **bHavokPick は retail 1.2.0416 で「登録のみ・読取なし」の死に設定** [確定]。
  - ノード 0xB05204: {value=0（既定）, name=0xA378F8}。直後の兄弟ノード: 0xB0520C iUpdateType / 0xB05214 fMoveLimitMass(95.0f) / 0xB0521C bAddBipedWhenKeyframed。
  - 登録: 0x9DB170 `push 0xB05204; mov ecx,0xB07BF0; call 0x4040E0`。後続 0x9DB187 `push 0xA17E10; call 0x981FB4`（atexit クリーンアップ登録）。0xA17E10 thunk は shutdown 時に 0x65C620 でノード解放。0x9AD3C0 = 別の解放ジャンプ先。
  - **値 0xB05204 の参照は 3 件のみ**（0x9AD3C1 / 0x9DB171 / 0xA17E11 = 全て登録・解放系）。対照的に兄弟設定には 0x441Axx 帯の実消費がある: iUpdateType→0x441A10（書込先 0xBA7918）、bAddBiped→0x441A22（→0xBA7909 byte）、fMoveLimitMass→0x441A4A（→0xB2E764 f32）。
  - → **ピック経路の選択は INI ではなくハードコード**。bHavokPick=1 への切替は retail では発生しない。

### 1.3 実のレイキャスト本体 [確定]

| 関数 | 役割 | 証拠 |
|---|---|---|
| **0x8BA470** (332B) | **hkWorldRayCaster のキャスト実装**（vtable 0xA74C80 slot[1]） | 要素配列（stride 0x30、this+0xC/+0x10）を index 参照 → vfunc[0] で broadphase テスト → **3x4 行列の SSE 逆変換**（movaps/subps/shufps/mulps/addps で [this+0x20]/[+0x30] に ray をローカル空間へ）→ vfunc[0x18] に shape レベルキャスト → **戻り float = [結果+4] = hitFraction** |
| 0xA74C74 / 0xA74C80 | hkWorldRayCaster vtable | RTTI index 行1708（`.?AVhkWorldRayCaster@@`）。slot[0]=0x683BF0 scalar deleting dtor（[esi]=0xA74C74 復元）。vptr 書込: 0x683BFA/0x683C12/0x88DEE9/0x89885A（ctor 帯）、0xA74C80 参照 0x6876F6/0x88B9B0/0x88BA48/0x88DDDA/0x89880C |
| **0x8BA1B0** (thiscall, `ret 0x14`) | **ワールドレイキャスト準備＋発射**。プロファイル名 0xA981E4 "TtRayCstCached" | リクエストの from/to を `movaps xmm0,[eax]` で読み、this（=スタック上の hkWorldRayCaster、vtable 0xA74C80 を [esp+0x24] へ設定）へ格納。**最終発射: 引数1 オブジェクトの vtable slot[14]（+0x38）を `call [edx+0x38]`**（ecx=world 系オブジェクト、引数=スタック collector＋ray 入力＋0）= **本物の castRay 仮想** |
| 0x8BA2C0 | AABB/グループキャスト（プロファイル名 0xA981F4 "TtRayCastGroup"） | minps/maxps で N 個の 0x30B ボックスの AABB を合成 → vfunc[0x3C] 呼び出し。0x8BA3E0 から 0x8BA1B0 を再利用 |
| 0x8987E0 (`ret 8`) | パス系ピック用薄いラッパ | スタック hkWorldRayCaster（vtable 0xA74C80）を構築し 0x8BA1B0 へ委譲。[ecx+0x78]/[ecx+0x64] を world 引数として使用 |

### 1.4 ピック統括: bhkWorld::PickObject 0x88B850 [確定]

**bhkWorld vtable = 0xA95CF4**（b13 L244 と一致）。**slot[34] [+0x88] = 0x88B850**。bhkWorldM（MemoryWatchDog）vtable 0xA97614 も同位置に 0x88B850 を保持。bhkWorld ctor = 0x88AEB0（vptr 書込 0x88AEF1。呼出 2 件: 0x4D4B18 = ゲーム側ワールド生成（0x4D4A20 SEH 初期化関数、0x4D5A65 から呼出）、0x8A7B31 = ラッパ）。

0x88B850 本体（プロファイル名 0xA96110 **"TtPickObject"**、TLS リングバッファ fs:[0x2C]+[0xBA9DE4]、rdtsc 計測を [+0x1A4..+0x1A8] へ 12B レコード）:

1. **ゲート**: `[0xBA790A]==0` → 全処理スキップ（ピック統計/描画のマスタースイッチ）。
2. **ピック種別スイッチ**: `([リクエスト+0x24] & 0x3F)` ∈ 0x14..0x1D。バイト表 0x88BB48 → ジャンプテーブル 0x88BB34:

| 種別値 | カウンタ増加 | 意味（文字列 "%d Havok picks (%d path, %d LOS, %d ViewCaster)" 対応） |
|---|---|---|
| 0x15, 0x1B | total 0xBA7924++ + **path 0xBA7928++（上限 0x1E=30/フレーム）** | **path ピック**。超過時はピック中止（[req+0x50]=0, [req+0x44]=1.0f へ戻して早期 return）＝**フレーム予算制** [確定] |
| 0x1A | total++ + LOS 0xBA792C++ | **LOS ピック** |
| 0x1D | total++ + ViewCaster 0xBA7930++ | **ViewCaster ピック** |
| 0x14 | total++ のみ | （種別記録なし） |
| 0x16–0x19, 0x1C | total++ + 最終種別を 0xBA7934 へ記録 | "Other type = %d" 用 |

3. **ワールド取得**: slot[22]（+0x58）= **0x452A60 = `mov eax,[ecx+8]; ret`** → **hkpWorld は bhkWorld+8 に埋め込み**。ピックは [hkpWorld+0x64] と [hkpWorld+0x78] を world 引数として使う。
4. **ディスパッチ**: リクエストの [+0x70]/[+0x78] が両方非 null → スタック hkWorldRayCaster + 0x8BA1B0（フルレイキャスト） / [+0x70]==0 → 0x8987E0 ラッパ（パス系） / [+0x78]==0 → [req+0x74] 分岐 → 0x898ED0。
5. **結果の格納**: 命中時 `movaps [req+0x30..0x3C] ← ヒット位置`、`[req+0x40] ← 追加キー`、`[req+0x44] ← hitFraction`（初期値 1.0f）、`[req+0x50] ← collidable キー`。0x8B22F0 で補正。
6. **デバッグ描画**: グローバル [0xBA7DA0]（デバッグレンダラ）の 0x8A78E0 へ ray セグメントを色付きで登録（命中=0x000000FF / 未命中=0xFF888888 の定数演算で選択）。
7. **統計の生命周期**: フレーム毎リセットは 0x889A20–0x889A5F 帯（参照 0x889A36/0x889A59）。出力は bhkWorld slot[32]（+0x80）= **0x88C440 統計プリンタ**（0x88C4C0 で "%d Havok picks..." を整形、0x88C4E9-0x88C507 で全カウンタ読出）。

**リクエスト構造（ピック関連フィールド）** [確定/推定]:
+0x00..0x0F ray from/to（16B movaps 読取）、+0x20 byte フラグ（フィルタ有無）、**+0x24 種別**、+0x30..0x3F ヒット位置出力、+0x44 hitFraction 出力、+0x50 extra キー出力、+0x70/+0x74/+0x78 world/フィルタ系ポインタ。

### 1.5 ゲーム側呼び出しチェーン [確定 1 件 + 残差]

- **ABI**: `PickObject(request)` = thiscall・`ret 4`（callee がスタック引数 1 つを破棄）。`call [reg+0x88]` 形式の直接呼出しは **.text 全体で 0 件**（FF 90..97 88 00 00 00 および SIB/jmp 変種すべてスキャン済み・0 件）。呼び出しは全て `mov edx,[world]; mov ecx,world; mov eax,[edx+0x88]; call eax` 形式。
- **ワールド解決ヘルパ 0x4CAFC0**（13 呼出）: [obj+0x28] を 0x4204C0 で探索 → 不可なら [obj+0x50] を 0x4EF7C0 → 最終フォールバックは**グローバル 0xB360AC**（メイン物理ワールド）。キャッシュ先 0xB35220 は 0x49EDB0 で書込み（0x49Exx 帯のみ使用）。
- **確定したピック発射地点 = 0x68CBC4（関数 0x68CB40・`ret 4`・PathQuery 帯）**:

```
0x68CB40: ecx = PathQuery/ステータス構造体（引数 = リクエスト相当のオブジェクト ebx）
  1. [0xB333A0]=NavMeshManager → [+0x34] セル取得（null なら 0x43F270）
  2. 0x44A270(x=[esi+8], y=[esi+4], cell) → 位置上のオブジェクト edi
  3. 0x4CACE0(edi) の Z と [esi+0xC] を fcomp → 範囲外ならフラグクリアで終了
  4. [esi] |= 4（オブジェクト発見）
  5. push ebx（=リクエスト）; world = 0x4CAFC0(edi);
     mov edx,[world]; mov ecx,world; call [edx+0x88]   ; ← PickObject!
     （ret 4 が push を消費するため、呼出後 ebx を [esp+0x18] から再ロード）
  6. 命中 bool なら リクエストの vtable[+0x284] を arg 0x47 で呼ぶ（>=0 なら [esi]|=0x10）
  7. 0x5E06C0(リクエスト, [esi+4] 位置, edi, 定数 0xA6E688) → bool で [esi]|=8、常に |=0x20（完了）
```

  → これは**経路ノード位置の障害物検証ピック**（N4↔N7 交差点）。path ピック（種別 0x15/0x1B）の実使用者に整合。呼出元 2 件: **0x67E43C**（移動/ステアリング帯 0x67Exx）と **0x6867ED**（NavMesh システム更新 0x686450 内、0x68CA00 パスクエリ初期化の直後、引数 edi=リクエスト）。

- **+0x88 ディスパッチの伪陽性分類**（16 件の直接隣接 `mov+call` を全件調査）:
  - 0x6869D4 = **TESForm の slot[34]**（formType 0x12 ドア分岐、GetBaseForm vtable[+0x170] 経由）→ bool メソッド。ピックではない。
  - 0x5FA28A / 0x604AD7 / 0x6865EE / 0x536CB7 / 0x537B01 / 0x4F86FA = 引数なし bool/ポインタ返しの別クラス +0x88。
  - 0x40CF57 / 0x40D03D / 0x7650BF / 0x76521C = [obj+0x280] 由来・**6 引数**のレンダラ系 +0x88。
  - 0x8A05E0 = 小型ラッパ（ecx の +0x88 を引数ゼロで呼ぶ別クラス用）。
- **LOS 検知側**: s68_n6 の「0x43F8C0 = LOS 判定」は**37 バイトの障害分類述語**（[arg+0x26] ∈ 2..6 のジャンプテーブル、53 呼出）でありレイキャストではない。検知パイプラインの実レイ発射地点は未特定（残差 R1）。ただしカウンタ 0xBA792C/0xBA7930 が種別 0x1A/0x1D ピックの存在を実証。

### 1.6 N4 未解明 + 試行経路

| ID | 内容 | 試行（失敗理由） | 次の手掛かり |
|---|---|---|---|
| R1 | LOS(0x1A)/ViewCaster(0x1D) ピックの request 生成元と発射サイト | 種別即値スキャン（C7/C6 xx 24 / push imm8 6A xx）→ 0x5353xx/0x5389xx 帯は 0x8A7F20(category,id,flag) 登録テーブルと判明（伪陽性）／`call [reg+0x88]` 全形式 0 件／0x4CAFC0 呼出 13 件は上記分類で全て非ピック | 0x5F2AA5（検知）周辺の request 構築追跡、0x61762D/0x64BE85（0x67EB60 呼出＝戦闘/AI 帯）の上流 |
| R2 | [hkpWorld+0x64] のクラス同定（slot[14]=+0x38 = castRay 仮想の受け手） | hkWorldRayCaster vtable（2 スロット）では説明不能 → 別クラス | bhkWorld ctor 0x88AEB0 内の [+0x64] 書込追跡、b13/b26 の非 RTTI ブロック照合 |
| R3 | クロスヘア/Activate ピック | TODO の仮説「activate → castRay」は**該当サイト不存在**（Activate 帯に +0x88 ピック発射なし）。アクティベート対象選ベート対象選択は経路/LOS ピックか Gamebryo 側ピックか未分化 | HUDReticle/Activate ハンドラ帯（0x59Exx/0x5FExx）の request 構築探索 |
| R4 | 0x898ED0 パスピック変種の全論理 | 本束ではヘッドのみ | 0x898ED0 全逆アセンブル（規模小） |

---

## 2. N7: PathingSystem 詳解（NavMesh 直線移動・障害物回避・steering）

### 2.0 既存記述の訂正（3 件）[確定]

| # | 既存記述 | 訂正 |
|---|---|---|
| C1 | s72 S3.1「PathingSystem::Update が状態機械分岐 0x683A60=Turning / 0x684780=Avoiding / default=Pathing」 | **誤り**。0x684F40 はこれらを呼ばない。0x683A60 は **5 バイトの述語**（`mov al,[ecx+0x2C]; shr al,5; and al,1; ret` = Turning フラグ getter）、0x684780 は **34 バイトの述語**（回避アクティブ判定）。3 状態表示はデバッグプリンタ **0x408715–0x4087B2** が这两个述語を順に詢く派生表示 |
| C2 | s72 S3.2「[+0x30] = PathQuery ハンドル (0x14B)」 | **誤り**。**[+0x30] = 0x34 バイトの回避オブジェクト**（ctor 0x680DC0。確保は 0x6847B0 内 `push 0x34` → 0x401F00 → 0x680DC0 → 0x680E20([this+0x28])）。0x14B の PathQuery は 0x68BED0/0x68CB30 で作られる**別オブジェクト** |
| C3 | s65 §3.4「0x683A60: Checking → Turning status」 | C1 と同じ誤帰属 |

### 2.1 Init 0x684EC0 [確定]

1. 0x684830（ノードリスト解放・初期化）→ 2. [+0x30] の回避オブジェクトを解放（dtor 0x538B60 → free 0x401F20、[+0x30]=0）→ 3. 0x68C6E0（位置/状態リセット、本体は 0x68C0F0 へ委譲）→ 4. **[+0x1C] = [0xA32048] = FLT_MAX**（待機タイマ）、**[+0x48]=0**（経路状態フラグ）、**[+0x24]=0.0f**、**[+0x2C]=0**（フラグバイト: bit5=Turning・bit7=flag2）、**[+0x20]=0.0f**。

### 2.2 Update 0x684F40 フロー [確定]

引数: edi=[esp+0x178]=actor、ebp=this。SEH 大関数（sub esp,0x154）。

1. **経路ステータス照合**: `0x68BFB0(this+0x14, out..., )` → false なら即終了（0x685114）。
2. **ノード進行/座標更新**（分岐 [esp+0x18]=現在ノード有無）:
   - 有: 現在ノード → 0x780D10（next アクセサ）→ **0x6A2FD0(クエリ, next)**（パスクエリの目標設定 = s72 a4 の +0x10 フィールド設定元）。旧ノード解放（0x60D0A0 + 0x401F20）。0x6899C0（ノード位置 getter、`lea eax,[ecx+4]` 系）で座標を 3 ワード複製。
   - 無: 0x68C170 → **actor vtable[+0x174]（GetPos、b14 で確定済）**で座標取得 → 0x68C280(pos, 0) でクエリ構築。
3. **プレイヤー特別扱い**: `[0xB333B4]==actor` なら 0x585C10(1)（メニュー状態 [+0x31]>0 または [0xB3B908] デバッグフラグ）→ actor vtable[+0xD4] 呼出し → typeid 0xA74CD8 を 0x98208B/0x579B60 で複製（デバッグオーバーレイ）。
4. **ステアリング/移動構築**: 0x67D760（構造初期化）→ **0x67EB60(this 構造, pos1, pos2, actor, 0)** ※[this+0x10] のモードバイトを引数構造へコピー（`mov dl,[ebp+0x10]; mov [esp+0x5C], dl`）→ 0x67E000 → **0x684000(this, actor)（ウェイポイント進行、§2.3）**。
5. 0x60D0A0 で後始末。

0x67EB60 の呼出元 7 件 = 0x510CE8（メニュー帯）/ **0x61762D（戦闘帯）** / **0x64BE85（AI 帯）** / 0x6850E9（Update）/ 0x6894AA・0x689753（NavMesh セル帯）/ 0x68B4AD（pathnode 帯）。→ 移動構築は PathingSystem 外の複数システムからも使われる共通物。

### 2.3 直線移動コア: 0x684000 ウェイポイント・ショートカット [確定]

thiscall（this=PathingSystem、arg=actor、`sub esp,0x50`）。**ノードリスト [this+0x14] を先読みし、曲がり角が十分鈍いならノードを削除して直線歩行へ寄せるループ**:

1. actor フラグ 2 種取得: `0x5E3400(actor)` → al [esp+0x12]、`0x5E1E90(actor)` bool を反転 → [esp+0x13]。
2. ノード列: ebx=現在（0x42B410 で [this+0x14] から取得）→ ebp=next（0x780D10）→ esi=nextnext。欠けなら早期終了。
3. **フラグ棄却**: [esp+0x12]==0 なら `0x68CA80(nextnext)`、[esp+0x13]==0 なら `0x68CAB0(現在)`（ノードフラグ bit アクセサ、s72 a26 系）が真なら終了。
4. **NavMesh 妥当性**: [0xB333A0+0x34]==0（現在セル無）のときのみ `0x43F840(NavMeshManager, ノード座標)` を要求。
5. **幾何判定**（ループ本体、ノード位置は 0x6899C0）:
   - u1 = pos(next)−pos(cur)、v1 = pos(nextnext)−pos(next)、w1 = pos(nextnext)−pos(cur)
   - `0x683CB0(a,b)` = **内積**（フロート返し）、`0x683AD0(a,b,out)` + `fabs` = **ベクトル間角**（s68_n6 の sub_683AD0/sub_683CB0 と同一の共有数学ヘルパ）
   - a1 = angle(u1, v1)、a2 = angle(u1, w1)、a3 = angle(u1, 次差分)（ループ継続分）
   - **ショートカット条件: a1 ≥ [0xA6E734] かつ a2 ≥ [0xA6E734] かつ [esp+0x64]==0 かつ 0x68CA50(next)==0**
   - 成立時 **`0x68BE80(this+0x14, next, cur)` で現在ノードをリストから削除**（s72 §3.5 の replace/free）→ cur=next へ前進して継続。
6. **定数**: **[0xA6E734] = 0.174533 = π/10 = 18°**（隣接 [-4]=8192.0f、[+4]=-0.174533）。=「進行方向と次セグメントのずれが 18° 未満ならノードをカット」。

呼出元 3 件: 0x6850FB（Update）/ 0x6894EC / 0x68976C（NavMesh セル帯）。

### 2.4 障害物回避 FSM [確定（名前・構造）/ 部分（遷移）]

**回避オブジェクト**（0x34 バイト、PathingSystem[+0x30]。ctor 0x680DC0 のレイアウト）:

| offset | 内容 | 証拠 |
|---|---|---|
| +0x00 | スマートポインタ（参照カウント: IAT 0xA2807C/0xA28078 = Interlocked 系。0x680E20 = 参照カウント付き差し替え setter） | 0x680E20 全読 |
| +0x04 | 参照カウント/内部ポインタ | ctor |
| +0x08, +0x10, +0x14, +0x18, +0x1C, +0x24 | f32（+0x1C は getter 0x680CC0 / setter 0x680CD0） | 0x680CC0/0x680CD0 |
| **+0x0C** | **ステータス byte**（getter 0x680CB0 = `movsx eax,[ecx+0xC]; ret`） | 0x680CB0 |
| +0x0D | byte フラグ | ctor |
| +0x28, +0x2C, +0x30 | ptr/dword | ctor |

- **アクティブ述語 0x684780**: [this+0x30] が存在し、ステータス ∉ {0, 7} のとき真。
- **ステータス名前表 0xB15728（8 状態）** [確定]:

| 値 | 名前（0xB15728[n] → 文字列） |
|---|---|
| 0 | NO AVOIDANCE (0xA30B28) |
| 1 | Checking Obstacle (0xA74A58) |
| 2 | Avoiding Obstacle (0xA74A44) |
| 3 | Avoiding: Angling Left (0xA74A2C) |
| 4 | Avoiding: Angling Right (0xA74A14) |
| 5 | Waiting (0xA74A0C) |
| 6 | Forcing New Path (0xA749F8) |
| 7 | FAILED (0xA749F0) |

  プリンタ経路: 0x408859 `call 0x680CB0` → `mov eax,[eax*4+0xB15728]` → "Avoidance Status: %s" (0xA30B90)。

- **設定**: 表の直後 0xB15748 に {value=0, name=0xA74B2C **"bDisableAvoidance:Pathfinding"**} の設定ノードが隣接 [確定]。既定 0 = 回避有効。
- **遅延生成**: 0x6847B0（回避オブジェクト生成、呼出 0x68959D / 0x6899A1 = NavMesh セル帯 0x6894xx–0x6899xx）。
- **path ピックとの接続**: §1.5 の 0x68CB40 が経路ノード上で検出した障害/ドアオブジェクト（formType 依存の vtable[+0x284](0x47) 判定と 0x5E06C0 述語）は回避 FSM 入力源の筆頭候補 [推定]。
- **遷移関数**（ステータス byte 0..7 の書込）: 未特定（残差 R5）。最有力帯 = 0x680Fxx–0x682xxx（回避オブジェクトメソッド）+ 0x6894xx/0x6899xx（セル帯）+ 0x684530（0x6869B5 での formType 0x1C 専用呼出 `push 1; push ebx; push edi; call 0x684530`）。

### 2.5 Turning [確定（機構）/ 残（設置元）]

- Turning = **PathingSystem[+0x2C] の bit5**（getter 0x683A60）。bit7 は別フラグ（getter 0x683A70 / setter 0x683A80 = `or/and [ecx+0x2C], 0x80`）。
- bit5 の設置元は未特定（残差 R6。[+0x2C] 書込を 0x67D–0x684 帯で探索）。
- 補助述語: 0x683AA0 = 「次ノード有在」（0x42B410 → 0x780D10、null なら 0）。0x683AD0 = 角度/クランプ付き演算（[0,1] clamp → アークコサイン系。検知式 s68_n6 と共有）。

### 2.6 Velocity 出力 [部分]

- 出力フィールド [+0x3C]/[+0x40]/[+0x44] = XYZ（s72 M7 記載を踏襲）。ゼロは Init 経路 0x68C6E0 → 本体 0x68C0F0（ラッパ確認済。ストア本体は残差 R7）。
- **生成側**: Update §2.2-4 のステアリング構築帯（0x67D760 / **0x67EB60** / 0x67E000）が最有力 [推定]。モード byte [this+0x10] が挙動を切替。
- **消費側**: actor 移動（0x5FAAE0 = ActorSpeedCalc、s65 §3.6）への配線は未直接確認（残差 R8）。

### 2.7 PathingSystem 構造レイアウト（確定分の更新版）

| offset | 内容 | 確定度 |
|---|---|---|
| +0x10 | モード byte（Update が移動構築へ転送） | 確定 |
| +0x14 | ノードリスト容器（0x68BFB0 / 0x68BE80 / 0x42B410 / 0x6899C0 のレシーバ） | 確定 |
| +0x1C | 待機経過時間（Init = FLT_MAX 0xA32048） | 確定 |
| +0x20 / +0x24 | f32（Init 0） | 確定 |
| +0x28 | ノード ptr（回避オブジェクト初期化へ渡す） | 確定 |
| +0x2C | フラグ byte（bit5=Turning / bit7=flag2） | 確定 |
| **+0x30** | **回避オブジェクト ptr（0x34B）** | 確定（s72 の PathQuery 記載を訂正） |
| +0x3C/40/44 | 出力速度 XYZ | 確定（書込元は推定） |
| +0x48 | 経路状態フラグ（Init 0） | 確定 |

### 2.8 N7 未解明 + 試行経路

| ID | 内容 | 試行 | 次の手掛かり |
|---|---|---|---|
| R5 | 回避 FSM 遷移関数（status 0..7 の書手） | 0x683A60/0x684780 が述語と判明（s72 訂正）し、書込は別関数と確定 | 0x680Fxx–0x682xxx の回避オブジェクトメソッド群全読、0x6894xx/0x6899xx セル帯 |
| R6 | Turning bit5 設置元 | プリンタ依存でなく [+0x2C] 書手が必要 | 0x67D/0x67E ステアリング帯の [+0x2C] 書込走査 |
| R7 | 速度出力の書込本体（0x68C0F0 内ストア + 0x67EB60 帯） | s72 a32 はラッパ止まり | 0x68C0F0 全読 + 0x67EB60 本体解析 |
| R8 | velocity → actor 配線（0x5FAAE0 前段） | s65 §3.8 の推定のまま | AIProcess tick 内の PathingSystem→移動呼び出し追跡 |
| R9 | NAVM ESM ローダ（s72 S6-1 の残） | 本束では未着手（Phase 3-3 本体） | 0x5F9000 帯 / 0x4EF7E0 / 0x4EB0E0 候補 |
| R10 | bDisableAvoidance の消費経路 | ノード 0xB15748 のみ特定 | 回避 FSM 遷移関数（R5）内の参照 |

---

## 3. ロードマップ/既存束への接続

- **Phase 3-3（LAND/NAVM）**: ランタイム側（経路探索・回避・steering 入口・ノードショートカット）が本束で文書化され前進。Phase 3-3 の本体残件は **NAVM レコードの ESM ローダ**（R9）に収束。
- **P4（pathing_system.h/.cpp 移植）**: 移植形状が確定 — ノードリンクリスト + 18° ショートカット（0x684000）+ 回避 FSM 8 状態（表 0xB15728）+ velocity 出力 + 経路ノード検証ピック（0x68CB40）。
- **K2/N4 裁定**: TODO K2 行の訂正（0x8A2650=等値比較子）が本束で確定 → s62/s63 の「Shape::CastRay」命名は次版 relmap で訂正素材。
- **M7 行**: 「NAVM 詳細は Phase 3 継続」の回収を本束で部分実施（steering/回避/直線移動を解明、遷移関数とローダが残）。
- **N2**: `s68_n2_context.md` の結論（HavokContext 非仮想）を踏襲。本束で vtable/仮想呼出の再調査はしていない。bhkWorld（0xA95CF4・slot[34]=ピック）は **HavokContext とは別物**である点に注意。

## 4. 使用証拠（主要アドレス一覧）

- 裁定: 0x8A2650（30B・sete）/ 0x89D6F0 / vtable 0xA559CC+0xBC
- 指引訂正: 0xA126A0 thunk / 0xA981C8 / 0xA95E5C / 0x70E220 / INI 0xA378F8・node 0xB05204・registry 0xB07BF0・0x4040E0・atexit 0x981FB4・cleanup 0xA17E10 / 兄弟設定消費 0x441A10・0x441A22・0x441A4A
- レイキャスト本体: hkWorldRayCaster vtable 0xA74C74/0xA74C80（slot1=0x8BA470・332B）/ 0x8BA1B0（slot[14]=+0x38 発射・ret 0x14）/ 0x8BA2C0・0x8BA3E0 / 0x8987E0 / プロファイル名 0xA981E4・0xA981F4
- PickObject: bhkWorld vtable 0xA95CF4 slot[34]=0x88B850（bhkWorldM 0xA97614 も同）/ ctor 0x88AEB0（0x4D4B18・0x8A7B31）/ gate 0xBA790A / 種別表 0x88BB34+0x88BB48 / カウンタ 0xBA7924/28/2C/30/34（上限 0x1E）/ slot[22]=0x452A60（[this+8]=hkpWorld）/ 結果レイアウト +0x30/40/44/50 / デバッグ描画 0xBA7DA0・0x8A78E0・色 0x000000FF/0xFF888888 / 統計 0x88C440・0x88C4C0・リセット 0x889A20 帯 / プロファイル名 0xA96110・0xA96198
- ゲーム側: ワールド解決 0x4CAFC0（13 呼出）・グローバル 0xB360AC・キャッシュ 0xB35220 / **ピック発射 0x68CBC4（関数 0x68CB40・ret 4）**・オブジェクト取得 0x44A270・Z 0x4CACE0・vtable[+0x284](0x47)・0x5E06C0・定数 0xA6E688 / 呼出 0x67E43C・0x6867ED / 伪陽性分類 0x6869D4（TESForm）等 16 件 / 0x43F8C0（37B 述語・53 呼出）
- PathingSystem: Init 0x684EC0（0x684830・0x538B60・0x68C6E0→0x68C0F0・0xA32048）/ Update 0x684F40（0x68BFB0・0x6A2FD0・0x780D10・0x6899C0・0x68C170・0x68C280・vtable +0x174・0xB333B4・0x585C10・0xA74CD8・0x67D760・**0x67EB60**・0x67E000・**0x684000**）/ ショートカット 0x684000（0x5E3400・0x5E1E90・0x42B410・0x68CA80/AB0/A50・0x43F840・0x683CB0 内積・0x683AD0 角度・**定数 0xA6E734=0.174533=18°**・0x68BE80 ノード削除）/ 回避 0x6847B0・ctor 0x680DC0・0x680E20・getter 0x680CB0・述語 0x684780・**名前表 0xB15728（8 状態）**・設定 0xB15748/"bDisableAvoidance:Pathfinding" 0xA74B2C / Turning 0x683A60（+0x2C bit5）・0x683A70/A80（bit7）・0x683AA0 / デバッグプリンタ 0x408715-0x4087B2・"Avoidance Status" 0xA30B90 / NavMeshManager 0xB333A0（+0x34 セル）・0x43F270・0x44A270

## 5. 遵守・検証

- git 不使用 / docs・TODO_re49・GT・session-memory・plan・cache の**編集 0 件** / 既存 tmp/re 上書き 0 件（本ファイル v186 のみ新規）/ ESM・ESP 読取 0 回（再スキャン禁止遵守）/ Ghidra 呼出 0 回 / N2（HavokContext vtable）再調査 0 回（指示遵守）。
- 中間物は `%TEMP%\opencode\v186_1..30_*.py` のみ。re_util selftest 稼働確認済。
- 反証可能性: 全 [確定] 値は上記 VA の逆アセンブル/バイト列で再現可能（capstone + pefile、VA→off = pe.get_offset_from_rva(VA−0x400000)）。

（了 — v186_havok_ray_pathing.md / 束F: N4 裁定と経路解明、N7 直線移動・回避 FSM・steering 入口の解明と残差 R1-R10）
