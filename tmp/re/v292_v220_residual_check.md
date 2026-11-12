# v292 — V220 残点8件＋GTドラフト6件の後続ラウンド照合（クローズ可否判定）（2026-11-05）

- 対象: `tmp/re/v221_opcode_gt_names8.md` §7 新規残点 **V-221-1..8**（8件）＋ §8「本タスクでの訂正まとめ」（GTドラフト 6件）
- 照合先: 後続ラウンド `tmp/re/v224`〜`v288`（v227/v231/v242/v256 が主戦場、補助に v230/v235〜v239/v262/v279）
- 方法: needle grep＋該当節読解（バイナリ再測なし＝照合レポート）
- selftest: 着手時 **4/4 PASS**（`tmp/re/handover_20261025/scripts/probe_harness.py selftest`、bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl@0x504560）
- 制約遵守: L1 report-only。本ファイル新規 1 件のみ。GT/docs/台帳/plan は無編集（反映判断は #20 ゲートへ）。

---

## 1. 判定表 — 残点8件（結論: **解消7 / 大部分解消1 → クローズ可**）

| ID | 残点（v221 §7） | 判定 | 後続の出典（判定根拠） |
|---|---|---|---|
| V-221-1 | `FUN_004DE980` 全体（enum 0..11）の正式機能名・各値の意味論／`FUN_005A4980`・`FUN_005A8C90` 所属 | **解消（後続へ再起票→解明）** | v227 §1 [部分] → v231 §2 [大幅進展] → **v242 §4 [解明]** |
| V-221-2 | `FUN_006505D0` の `0xB3BA80` 切替主体・`0xB3BA9C` コレクション実体名 | **解消** | **v227 §2 [解明]**（§8-R に GT 行としても記載） |
| V-221-3 | `FUN_00678750` の全体機能名 | **解消** | v227 §3 [部分] → v231 §3 → **v242 §5 [解明]** |
| V-221-4 | formType 0x0C クラスの正式クラス名と record fourcc | **解消** | **v227 §4 [解明]**（RTTI `.?AVEffectSetting@@`） |
| V-221-5 | ENIT フィールド意味論（ENCH 16B / ALCH・INGR 8B） | **解消** | v227 §5 [部分] → v231 §5 [大部分] → v242 §3（3フィールド名確定）→ **v256 §5.1 [解明]**（ALCH 残分） |
| V-221-6 | EffectSetting 未命名フィールド +0x64/+0x74/+0x78/+0x7C/+0x80 | **大部分解消（細目残）** | v227 (O)・v231 (Z)/(V-231-8)・v237/v238（tail POD） |
| V-221-7 | `FUN_00417420` 登録列の完全枚挙と fourcc 意味 | **解消** | **v227 §7 [解明]**（161 件完全列・fourcc 全判明） |
| V-221-8 | 適用帯 bit23 リーダーの `[obj+8]→[+0x1C]` ホルダクラス同定 | **解消** | **v231 訂正まとめ (Y)**（AlarmPackage として確定） |

### 各項目の根拠（要約）

- **V-221-1**: v242 §4 が `[解明]` を宣言（enum = HUD tile `0xFB3` へ float で渡される操作種別コード・`FUN_005A8BC0` 経由、live 消費は {3,5}=家具マーカー探索ゲートのみ）。同 §4.5 で **v231 §2.1「E8 callers 5件」表は `FUN_004DE980` と `FUN_005A4980` の callers 混同と訂正＝実 E8 は 2 件のみ**（`FUN_005A4980`@0x5A4ADB／`FUN_005A8C90`@0x5A8CAE、後者は E8/imm32 とも外部からの参照 0＝dead）。※細目（formType 0x23 ケースのプレイヤ状態連鎖等）は v242 §4 の enum 表に包含される形で処理され、v242 以降に同内容の再起票なし。
- **V-221-2**: v227 §2 — `0xB3BA80` = **`g_effectTargetMode`**（byte・唯一の書手 `FUN_006553E0` arg1 @0x6553F2・初期値 0＝BSS・vanilla 常時 bit31 経路が実効）、`0xB3BA9C` = **`g_feetLevelCandidateRefs`**（連結リスト・追加=`FUN_006505D0`＝`FUN_006553E0` が `0x566B30` へ**関数ポインタとして渡す**ため E8 0 件・消費=`FUN_006553E0` の `this+0xB0` ドレイン）。whole-file imm32 走査 3/7 参照の過不足一致。
- **V-221-3**: v242 §5 — this = **静的非多態シングルトン `0xB3BD00`**（+0x68 = REFR リンクリスト・機能＝**家具/ベッド使用の進行役**・vtable ストア 0 件）。型コード = 着席/睡眠のステートマシン `0..0xA`（1/6=要求 → 4/9=成立、2/7=失敗、3/8=位置確定、0=解除、**0x7F は marker sentinel**）、`0x88CE30` = BSX ゲート付きシーングラフ走査器。
- **V-221-4**: v227 §4 — formType **0x0C = EffectSetting**（ctor `FUN_00416430`・size 0xA8・RTTI 実デコード）、'ESCE' はファイル chunk ハンドラ帯 `FUN_00415800`/`FUN_00416100`。**併せて v221「formType 0x20 = EffectSetting」は誤りと判明**（0x20 = TESFurniture・v227 (M)）。
- **V-221-5**: v242 §3 で 4 フィールド名確定（+0x34 type=cast type 0..3／+0x38 SpellCost／+0x3C SpellLevel／+0x40 Flags）、v256 §5.1 で ALCH/INGR 8B = **{cast type（-1=既定/自動）, flags（byte）}** まで解明。残点なし。
- **V-221-6**: +0x64 = **school（kMagicSchool 0..5・既定 6=なし・resolver `0x41B820`→`0xB33560..`）**（v227 (O)。v221 の「formID 系」は訂正）、school グローバルの正体 = **TESSound\* の MagicFailureSound 5〜6 学校分**（v231 (Z)）、+0x7C = **formID 変換される ptr**（v231 V-231-8 [解明]）、+0x74 = float 既定 1.0、**+0x70/78/7C/80/84/88/8C = ポインタ 7 本＋+0x74/90/94 = float 3** で、消費は EffectSetting の仮想スロット 9/45/46（レコード管理 API・Save 経路）のみ＝**gameplay 読手ゼロのレコード保持用**（v237 項目2・v238 §1 で解明）。v221 の「+0x80 = 2 callers が非ゼロ必須ゲート」はこの slot API 系と対応し、追加調査の必要なし。**細目として残るのは「+0x64 を経由する `FUN_00413890` actor スケール経路と field の1対1注記」程度**（→ 本文書 §2）。
- **V-221-7**: v227 §7 — **組み込み EffectSetting 登録 161 件の完全列（`0x417420..0x418DAA`）**。引数契約 (code, name, school, costF, AV, flags, resistAV, 関連件数, 関連 code[])。fourcc 全判明（DSPL=Dispel・CUDI/CUPO/CUPA=Cure Disease/Poison/Paralysis・STMA=Stunted Magicka 等、DUMY→REHE/FOHE/ABHE/DSPL 等の**関連 fourcc=対抗効果コード**）。"multiple ASSOCIATED flags" エラー経路は**死コード**（訂正）。v235/v236 で 89 点/157 点の機械デコードと相互整合。
- **V-221-8**: v231 訂正まとめ (4) — ホルダクラス = **`AlarmPackage`**（v227「bit23 ホルダクラス名未取得」を確定）。

---

## 2. GTドラフト6件（v221 §8）の整合確認

| # | ドラフト（v221 §8） | 後続での状態 | 判定 |
|---|---|---|---|
| 1 | v177「`0x450C20`＝4-byte formID reader」→ `TESFile::Read(dest, size)` へ呼称訂正 | v230 L35-36／v235 L35／v236 L339／v237 L182／v238 L52 が**全て TESFile::Read として正使用**（ESCE・DATA 64B・EDID 0x200 まで一貫） | **整合（訂正確定が浸透）** |
| 2 | v193b H.2「bit31=負センチネル」棄却 | v227 L69 で再言及（callers 5 件すべて kMgefFlag_MagnitudeIsFeet 正規用途）。反証ゼロ | **維持** |
| 3 | v216 §4.1「同値は rand()（`0x9828C0`）」誤り → ftol | **v279 L208 で独立再検証**: `0x9828C0` = SSE 経路 `cvttsd2si`（trunc）／x87 経路 `_ftol2`。v262 L116 の trunc 記述・v270/v275 の float→int 用途とも整合 | **整合（再検証済み）** |
| 4 | v216 §4.2「[+0x20]（school word）」→ `TESFullName::nameLength` 訂正候補 | 後続で矛盾記述 **0 件**（needle `nameLength` は v221 自身のみ）。訂正候補は未破棄 | **反証なし・GT 反映待ち** |
| 5 | v216 §6 V-216-1「{0,3,5}=表示単位 enum」確定（種別コードとして使用） | v242 §4 で enum 体系が解明され整合（live 消費 {3,5}） | **整合** |
| 6 | v216 §3.1 mgefFlags +0x58 追記（当時「formType 0x20=EffectSetting」と併記） | **v227 (M) で上書き訂正**: formType **0x20 = TESFurniture**（ctor `FUN_004AE660`・RTTI 確定）、**EffectSetting = formType 0x0C**。v256 L59 でも「0x0C=MGEF は本タスク実測の新規確定」 | **訂正の上書きが必要（GT 反映時の注意点）** |

→ GT ドラフト 6 件は 5 件が整合・1 件（#6）は **v227 の新確定により書き換え必須**。GT/docs 本体への反映は **#20（V168 台帳/GT 反映第2・`#needs-approval`）で一元管理**されており、本照合では編集しない。

---

## 3. クローズ可否の結論

- **#43（V220 残点確認）はクローズ可**:
  - 残点8件 = **解消7・大部分解消1**（V-221-6。細目は v237/v238 の tail POD 解明に包含され、新たな再調査要件なし）。全件に後続レポートの `[解明]` 宣言または確定記述が存在する。
  - GTドラフト6件 = 整合5・上書き訂正1（#6）。**GT 本体反映は #20 ゲートの担当範囲**であり、V220 の残点としては「調査・照合」が完了している。
- **引継事項**:
  1. GT 反映時に **#6（formType 0x20=TESFurniture／0x0C=EffectSetting）** を必ず反映（v221 の旧記載は誤り）。
  2. **#4（nameLength）** は訂正候補のまま反証なし＝GT 反映時に確定扱いで可。
  3. V-221-1 由来の細目（enum 各値の個別セマンティクス）が将来必要になった場合の再起票先は v242 §4 相当（現時点で再起票なし＝消化済み扱い）。

---

## 4. 検証情報

- selftest: **4/4 PASS**（`tmp/re/handover_20261025/scripts/probe_harness.py selftest`、`exit=0`）。慣習の `%TEMP%\opencode\probe_harness.py` は本体不在のためリポジトリ内同一物で代替（v293 §4 と同一状況）。
- needle 一覧（`tmp/re/v2[2-8]*.md`）:
  - `FUN_004DE980|FUN_006505D0|FUN_00678750|FUN_00417420` → 63 件（v227/v231/v242/v235/v237/v239/v256 に解明記述）
  - `0xB3BA80|0xB3BA9C|0x446CB0|0x446C30` → 22 件（v227 §2 が主）
  - `ESCE|0x44E3B1|formType 0x0C` → 44 件（v227 §4・v235/v236 ESCE 系）
  - `DSPL|REHE|CUPA|CUDI|Stunted Magicka` → 55 件（v227 §7・v230/v233/v235 fourcc 表）
  - `0x450C20|TESFile::Read|0x9828C0` → 48 件
  - `FUN_00413890|0x41B820|ENIT|FUN_00417220` → 95 件
  - `nameLength` → 3 件（v221 自身のみ＝後続に矛盾なし）
- 読取のみ（無編集）: `v221_opcode_gt_names8.md` 本体／後続 v227・v231・v242・v256・v230・v235〜v239・v262・v279 の該当節
- ESM 本体・git・docs・GT・plan・台帳: **未接触**
- 採番: 本ファイルは v292（v290=Copilot bundle v2 起案・v291=データ関係性・v293=V219 残点確認と並行割当）
