# V279: barter 数式台帳訂正ドラフト＋独立追試（serial 272・中）

- 日付: 2026-11-04
- 目的: `tmp/re/v277_re_port_bridge_map.md` §1/§3 が主張した barter 数式の「誤記3件」(①sqrt 引数 min→max ②sk 合成 skill+0.4Luck→Luck+Merc×0.4−20 ③丸め floor→ceil) を docs・v262/v264・port・EXE 実バイトに対して照合し、**訂正ドラフト（未適用）と独立追試結果**を1ファイルに集約する。
- 契約: 本ファイルのみが成果物（新規）。docs/既存 tmp/re/port ソース/git/ESM/TODO/plan/session-memory は**読取のみ・無編集**。ESM 再スキャン無し。修正適用はすべて人間承認ゲート待ち（§5）。
- 環境: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam）、capstone 5.0.7、selftest **4/4 PASS 着手時**（`%TEMP%\opencode\v279_selftest.py`）、VA 規約は `probe_harness.Ctx` の PE セクションテーブル実読で再確認（.text 0x401000/file−0x400、.rdata 0xA28000/−0x627200、.data 0xB02000/−0x700C00、0xB37xxx は .data bss=ファイル未バック＝実行時 ESM 供給域）。
- 手法: 前走中間物再利用（`v279_disasm_out.txt`/`v279_disasm2_out.txt`）＋本セッション新規再実測（`%TEMP%\opencode\v279_probe5.py`→`v279_probe5_out.txt`、`v279_probe6.py`→`v279_probe6_out.txt`）。第二訳者として `docs/Oblivion_decompiled.c`（Ghidra 台帳・読取のみ）を突合。前走の x87 実行ハーネス（v279_cpu*/probe3）はフラグ建模の欠陥で nan/誤値のため**不採**（§4 に解析的裁定＋Ghidra 突合で代替）。

## 0. 追試サマリ（3件すべての裁定）

| # | 依頼の主張（v277 §1/§3 由来） | 追試裁定 | 帰結 |
|---|---|---|---|
| ① | docs の sqrt 引数 `min(x,100)` は誤りで原版は **max** | **一致（主張どおり）** — docs L10004/L10005 の min が誤記。命令列 max idiom 確定（§4-1） | §1 で docs 訂正ドラフト2行（承認待ち）。port の `std::min`（barter.cpp L36/L44）は実差＝§3 高 |
| ② | docs L10245「skill + 0.4·Luck」が誤りで原版は **Luck + Merc×0.4 − 20** | **不一致（主張が誤り）** — 原版は `sk = trunc(clamp(Mercantile + 0.4×Luck − 20, 0..100))`。docs と port は**既に原版と一致**。誤った記述は v262 §5/疑似コード と v277 §1/§3 側（§4-2） | docs 訂正不要（§1-2 に記録）。v262/v264/v277 側注記（§2-3）。v277 の sk[PORT]中項目は**撤回**（§3/§5-7） |
| ③ | 「floor FUN_00484370」表記は誤りで **ceilf** が正しい | **一致（主張どおり）** — 0x484370=ceilf（byte 列＋Ghidra＋0x9828C0=trunc 確認、§4-3）。docs L7953/L7960/L9142/L10347 は既に ceilf で正。tmp/re v262/v264 の floor 表記が誤称 | docs 変更不要。誤称行は §2-1/§2-2 に注記（本体編集禁止→本節が代替） |

**要旨**: ①③は依頼どおり。②は依頼（v262/v277 由来）が誤りで、docs・port の既存記述が正しかった。**この成果物で「②の docs 訂正」は行わない**（行わないこと自体が判定）。

---

## 1. docs/RE_EXE_REPORT.md 訂正ドラフト（読取のみで特定・**適用は人間承認待ち**）

該当箇所は grep `min(`/`0.4`/`484370`/`sqrt` で確定。**§57.5・§59.5 の推定は実物で正しい**（§57.5= L10003 見出し、§59.5= L10244 見出し）。`min(x,100)` は文書全体で**L10004/L10005 の2行のみ**（他セクションへの混入なし、grep 全数確認）。

### 1-1. ①（実在の誤記・要訂正）

対象: `docs/RE_EXE_REPORT.md` **L10004-L10005**（§57.5 経済・所持品）

before（現物・逐語）:
```
- 買値 0x5481C0 = 0.01·(300 − 9.9·sqrt(200 − min(x,100)) − 1.5·trunc((y−50)/10)) [確定 (式形)]。
- 売値 0x548240 = 0.01·(5·sqrt(200 − min(x,100)) − 10 + 1.5·trunc((y−50)/10)) [確定 (式形)]。
```

after（草案）:
```
- 買値 0x5481C0 = 0.01·(300 − 9.9·sqrt(200 − max(x,100)) − 1.5·trunc((y−50)/10)) [確定 (式形)]（x=sk は 0x547B90 で 0..100 clamp 済のため sqrt 項は常に √100=10。分岐は `fld 100.0 / fcom st(1) / test ah,5 / jp` の MSVC max idiom・命令 0x5481C7-0x5481DA — V279 §4-1 で独立追試。tmp/re/v262 §3-4 は当初から max で一致）。
- 売値 0x548240 = 0.01·(5·sqrt(200 − max(x,100)) − 10 + 1.5·trunc((y−50)/10)) [確定 (式形)]（同上・命令 0x54824A-0x548257。誤記 min は §57 当初記述で、L10347/L7953 系訂正時の漏れと推定）。
```
- 影響: 既定域（sk≤100）で乗率は買 2.01／売 0.40（min のままだと sk<100 で 1.60/0.607 相当に化ける）。port 側 `std::min`（barter.cpp L36/L44）は**この誤記に整合した実装**になっているため §3 高項目と同時修正が必要。
- 適用手段: 未適用。docs 編集権限セッション／人間の承認後に行う。

### 1-2. ②（**訂正不要**の記録 — docs は正しい）

`docs/RE_EXE_REPORT.md` L10245（§59.5）:
```
`0x547B90` = 有効スキル (`clamp(skill + 0.4·Luck − 20, 0, 100)`)。
```
- 追試（§4-2）でこの記述は**原版と一致**と確認。GMST 名との和音も意味論的に整合（`fActorLuckSkillMult`=「Luck→スキル換算の乗数」、`iActorLuckSkillBase`=加数定数）。同文は docs 内 L1444（§20 `Actor_CalcSkillWithLuck`）にも一貫して存在し、AV 名表（0xB0A1A8 実読・idx7='Luck'/0x1D='Mercantile'）と命令スロット写像（`fild [esp+8]`×0.4）で裁定完了。
- v277 §3-③/§1 表が「docs L10245 は引数対応の誤記」と主張したのは**v262 §5 の誤読の伝播**（§2-3 参照）。docs 側は**変更なし**。
- 派生の付記（人間承認時の任意事項）: L10245 の「ラッパ先頭 … 0x5E1500 = haggle」→ canHaggle ラッパの実測先頭は **0x5E10F0**（0x5E10F0-0x5E1194、v279_disasm F窓/0x548130 呼び）。0x5E1500 の実体は本シリアル未検証（検証注記のみ・本ドラフトでは書換わない）。

### 1-3. ③（**訂正不要**の記録 — docs は既に訂正済）

- L7953: 「`0x00484370` = `ceilf(x)`（trunc 後、x − trunc(x) > 0 のときのみ +1.0）」= 正しい。
- L7960: 「`0x00484370` は `ceilf`（当初「trunc + floor 補正」推測は誤り…）」= 正しい。
- L9142・L10347: `0x484370`(ceil) / `0x404F70=floorf` / `0x4842F0=truncf` の住み分け表記 = 正しい。
- §59.5 L10245 も「`ceilf(0x484370)`」表記済み。docs 側で 0x484370 を floor と書く行は**残存ゼロ**（grep `floor`×`484370` 全数確認、L7960 の「誤りであった」過去形言及のみ）。誤称は tmp/re 側（§2-1/§2-2）。

---

## 2. tmp/re v262/v264 の誤称行 行番号付き注記（**両本体は無編集**・本節が代替台帳）

tmp/re 既存ファイル編集は契約上禁止のため、以下は「将来の台帳編集権限セッション用」の注記ドラフト。各行の「正」は §4 の再現実測に由来。

### 2-1. v262_barter_pricing.md — ③ floor 誤称（5行＋疑似コード1行）

| 行 | 現文（抜粋） | 注記（正） |
|---|---|---|
| L68 | 「**売**: `price = floor( … )`、**買**: `floor( … )`（floor=`FUN_00484370`、命令列 0x484370-0x4843AC: trunc→frac 補正…）」 | `FUN_00484370 = ceilf`（trunc→`x−t>0` のときのみ +1.0。§4-3）。命令列自体と trunc→+1 の記述は正しいので**関数名の floor だけ置換**で足りる |
| L74 | 「最終価格は **floor**（FUN_00484370）」 | 「最終価格は **ceilf**（FUN_00484370）」 |
| L144 | `float price = std::floor(mult * base); // floor = FUN_00484370` | `std::ceil(...)` |
| L190 | 差分表「端数 \| floor（FUN_00484370） \| std::lround + min1 …」 | 「ceilf（FUN_00484370）」。port 現況 lround+min1 との乖離記述自体は有効 |
| L230 | 「3. tradeGold の lround → **std::floor**（0x484370）」 | **[PORT] 提案自体が誤り**。正: lround → **std::ceil**（barter.cpp の barterRoundPrice は既に ceil で正しい） |
| L43 | 疑似コード先「`SellMult = 0.01 × ( floor? なし・実数: …`」 | 乗率式に丸めなし＝結論正しい。「floor?」の曖昧記述は削除可（低） |

### 2-2. v264_bundle_residuals.md — ③ floor 誤称（2行＋低1）

| 行 | 現文（抜粋） | 注記（正） |
|---|---|---|
| L79 | 「`0x489136: floor(FUN_00484370)`」 | `ceilf(FUN_00484370)` |
| L80 | 「正: `price = floor( price × floor(v/count×100) × 0.01 )`」 | 外側の floor→**ceilf**（0x489136）。内側 floor(v/count×100) は 0x484850 の F パス（0x4842F0 経由）で、0x4842F0 の実体は docs L10347 確定の **truncf**（非負域なので結果同一・命名注記のみ・低） |
| L7 | 既知確定事項リスト「barter 式 `FUN_00548240/0x005481C0/0x00547B90`」 | 引用元 v262 の②誤りを含む「足場」だった。**再検証禁止方針は本追試で解除**（§4-2 が正） |

### 2-3. v262 の ② sk 引数誤読（**誤りの発生源。v277 が伝播**）

| 行 | 現文（抜粋） | 注記（正） |
|---|---|---|
| L60 | 「`FUN_00547B90(Luck, Mercantile)` = **clamp(Luck + Mercantile×fActorLuckSkillMult(0.4) + iActorLuckSkillBase(−20), 0, 100)**」 | 引数対応と式の役割が反転。正: **`FUN_00547B90(Mercantile, Luck) = clamp(Mercantile + Luck×0.4 − 20, 0..100)`**（callee `[esp+4]`=素加算側=後push=GetAV(0x1D)、`[esp+8]`=×0.4 側=先push=GetAV(7)。§4-2）。末尾「slot 0xB3738C/0xB37394」は**name field**（値は 0xB37388/0xB37390、§4-4） |
| L112-116 | 疑似コード `merchantSkill`: `GetAV(7) + GetAV(0x1D)*kLuckSkillMult + kLuckSkillBase` | `GetAV(0x1D) + GetAV(7)*0.4 − 20` へ差替（コメントの AV 名対応 L59・L111 は正しい） |
| L214 | [PORT] 草案コメント「merchantSkill = trunc(clamp(**Luck + Merc*0.4** - 20 …」 | 「Mercantile + Luck*0.4 − 20」。草案の式本体（sqrt max 等）は正しいためコメントのみ |

### 2-4. v262 の GMST「スロット(値 field)」表記規約（L13/L15-28/L97-106）

- L13「値は slot+4 を直接 fld/fild で読む」**＝記述反向**。実測: 消費者は**スロット base そのもの**（登録 ecx）を読む。値= slot+0・name= slot+4（登録ヘルパ 0x41BAE0=`fstp [esi]` / 0x419A70=`mov [esi],ecx` + name を `[esi+4]`、§4-4）。
- よって L15-28 表の「スロット(値 field)」列 0xB37594/0xB375A4/0xB375AC/0xB375B4/0xB375BC… は全て **+4 ズレの name field**。真の値 field = 消費者読取 VA: **fBarterDispBase 0xB37590=1.5 / fBarterBuyBase 0xB375A0=300 / fBarterSellBase 0xB375A8=−10 / fBarterBuyMult 0xB375B0=9.9 / fBarterSellMult 0xB375B8=5**（既定 float の .rdata 実読は §4-4 で全件一致）。
- L97-106（疑似コード定数コメント）の slot 表記も同 +4。L106「fActorLuckSkillMult 0xB37394」→ **0xB37390**（0xB37388=−20 は L107 どおり一致）。
- v277 §1「GMST 定数」行が予告した「表記ゆえ統一」の解決版が §4-4。

### 2-5. v277_re_port_bridge_map.md（伝播側・本契約では編集禁止）

| 行 | 現文（抜粋） | 注記（正） |
|---|---|---|
| L6（手法 ③ 末尾文） | 「合成式 = **Luck + Merc×0.4 − 20**（tmp/re/v262 §5 と一致、RE_EXE_REPORT L10245『skill + 0.4·Luck』は引数対応の誤記）」 | 反転。原版= **Merc + 0.4·Luck − 20**、docs L10245 は正しい（§4-2） |
| L31（§1 表「合成スキル sk」行） | 「0.4 が Luck 側に掛かる（役割反転）…**相違** … after clamp(luck + 0.4×skill − 20) … テスト L100-116 差替」 | 判定は **実装済** へ降格＋[PORT]中項目**撤回**（port 式は原版と一致、テスト L101-116 の固定も正しい）。残るは命名/コメント低項目（§3-3） |
| L45（§1 引き渡しメモ） | 「修正順序 …③sk 引数役割（Luck/Merc 0.4）…」 | ③工程を削除（実差なし）。順序は ①max 化→②テスト差替→③purse クランプ→④tier/stolen ゲート→⑤命名 |
| L42（§1 表 商人入力 ESM 配線 末注） | 「既定値は原版『庶民 Luck50・Merc0→**sk=30**』とも一致せず」 | 正式では庶民 sk = 0+0.4×50−20 = **0**。port 既定（skill0/luck50→0）は**原版と一致**。この注は撤回 |
| L176（§7 逆方向の陳腐化） | 「docs/RE_EXE_REPORT の min(x,100)・『skill+0.4·Luck』…が誤」 | 誤り列挙は **min(x,100) のみ**。「skill+0.4·Luck」は正（docs 訂正不要） |

---

## 3. port 照合表（読取のみ: `app/src/main/cpp/game/barter.{h,cpp}`・`merchant.{h,cpp}`・`tests/barter_price_tests.cpp`）

原版式（§4 再追試で確定した形）:
- 売乗率 = `0.01 × (5·sqrt(200 − max(sk,100)) − 10 + 1.5·((disp−50)/10))`（@0x548240）
- 買乗率 = `0.01 × (300 − 9.9·sqrt(200 − max(sk,100)) − 1.5·((disp−50)/10))`（@0x5481C0）
- `sk = trunc(clamp(Mercantile + 0.4·Luck − 20, 0..100))`（@0x547B90 + 0x9828C0、商人側の AV）
- 単価 = **ceilf**(乗率±0.01·haggleAdj × base)（@0x484370、売は stolen%・0x48911C 前置）、買は base 床/売は base 天井（@0x488F75-/0x489153-）、売は `min(price, GetBarterGold)` サイレントクランプ（@0x48916F、負 purse は実質無制限）
- player Mercantile tier>3 → price=base、tier<1 のときのみ stolen 補正（@0x4890FC）

### 3-1. 項目別差分表（port 現況 vs 原版）

| # | 項目 | port 現況（VA=行） | 原版 | 判定 | 要[PORT] |
|---|---|---|---|---|---|
| 1 | 買乗率 sqrt 引数 | `std::min(merchantEffectiveSkill, 100.0f)` → `sqrt(200−min)`（barter.cpp L35-41、コメント L118-119 は test 側） | `max(sk,100)`（§4-1） | **相違** | **高**: sk<100 全域で誤（既定 sk=0 で 1.5999↔2.01）。修正概要 before L36 `std::min(...,100.0f)` → after `std::max(...,100.0f)`。定数・他項は一致 |
| 2 | 売乗率 sqrt 引数 | 同 min（barter.cpp L43-49） | 同 max | **相違** | **高**: 既定 0.607↔**0.40**（売乗率の既定表示が根本的に wrong）。before L44 `std::min` → after `std::max` |
| 3 | sk 合成 | `clamp(skill + 0.4f*luck − 20, 0..100)`（barter.cpp L31-33、コメント barter.h L33-35・merchant.h L96） | `clamp(Merc + 0.4·Luck − 20, 0..100)`（§4-2） | **実装済**（v277 の「相違/役割反転」は追試で覆る） | 低: 数式変更不要。①引数名 `skill` は「Mercantile」が正確（merchant.h L86「Barter-skill actor value」は Skyrim 由来の誤称）、②原版は int trunc 済み値を消費（port は float 直渡し—haggle 差分で ±1 の挙動差あり、テストは整数値のみで無風）。③merchant.h L87「fBarterDispositionMod」→ 1.5 の正体は **fBarterDispBase**（barter.h L28 定数名も同。0.01 の別物がある）。コメント/命名のみ |
| 4 | 丸め | `barterRoundPrice = std::ceil`（L51-53） | ceilf（§4-3） | **実装済** | — |
| 5 | 数量×丸め位置 | `tradeGold = std::lround(unit×count)` + min1 + `unit<=0||count<=0 → 0`（L23-29） | 単価 base 乗算前に ceilf（×qty はメニュー/サービス側、qty 乗算の丸め位置は [部分確定]/v262）。min1・ゼロ拒否は原版無し | 相違(小) | 低: after「単価は ceilf のみ（barterRoundPrice で済ませる）→ qty 乗算は lround 維持か切り捨てを UI 裁定」と「worth nothing=0 円成立 UI 分岐」の整合（docs L10249 の「サイレントクランプ」と矛盾しないこと）。`<=0 拒否`（L98/L165）は 0 円取引との差分として据置可 |
| 6 | haggle ±0.01×adj | 買 `coef − kBarterHaggleScale*offset`（merchant.cpp L67-68）/売 `+`（L83-84）、40% ハードクランプ無し | 一致（[player+0x11C] int、エンジン += 無し/v264 件1、HaggleMax=40 は UI 配付のみ） | **実装済** | — |
| 7 | haggle 可否式 | `barterHaggleAccepts`（L55-69、int 化・pow 省略=curve1.0・≥0） | 0x548130 と一致（ε=5.24e-11 は実務 ≥0） | **実装済** | 低: ε 注記のみ |
| 8 | stolen 補正 | `price*percent*0.01`（L71-76、percent<0 skip） — **tier<1 ゲート無し**（merchant.cpp L86 で無条件） | 売かつ **player Mercantile tier<1 のみ**（0x4890FC） | 暫定 | 中: tier 入力（player Mercantile→0x56A300 相当ティア）導入まで tier<1 前提の明記、または tier<1 相当ゲート追加（#9 と一括） |
| 9 | player tier>3 → price=base | 無し | 0x488F75-/0x489153-（call 0x5F23B0 idx 0x1D） | 未実装 | 中: 追加項目（テストは既定域で不変） |
| 10 | base 床/天井 | 買 `std::max(price, baseValue)`（merchant.cpp L70）/売 `std::min(price, baseValue)`（L88） | 買床/売天井 一致（docs L10247） | **実装済** | — |
| 11 | 商人財布 | `merchant.getGold() < earned → 取引拒否`（barter.cpp L169-172） | `min(price, GetBarterGold)` で**成立**（0x48916F、docs L10249。負 purse 無制限扱い） | **相違** | **高**: before L169-172 拒否 → after `earned = min(tradeGold(unit, count), purse)`。purse 実値（ACBS barterGold 化・既定 5000/CNTTO 撤去）は v277 §2 と同一改动、そちらの承認ゲートと連動 |
| 12 | GMST 定数 | barter.h L24-31 の値は全一致（300/9.9/−10/5/1.5/0.01/0.55） | §4-4 実読と照合済 | **実装済(値)** | 低: `kBarterDispositionMod=1.5f` の**命名誤り**（1.5=`fBarterDispBase`、`fBarterDispositionMod` は 0.01・別 slot 0xB375D8 の Persuasion 用）→ リネーム or コメント訂正（値不変） |
| 13 | 商人入力 ESM 配線 | merchant.h L105-107 既定 skill_=0/luck_=50/disp_=50、merchant_factory は setBarterSkill 等を読まない | sk/disp は商人 Actor の GetAV/GetDisposition（vt+0x284/0x224） | 未実装(配線) | 中（v277 同一件）。**注記訂正**: 既定値は正式「庶民 Merc0・Luck50 → sk=0」と一致（v277 L42 の sk=30 は②誤記由来。§2-5） |
| 14 | disp 入力非対称 | disp は merchant 側の単一入力 | 売ラッパ 0x5E11A0 は **self(商人)** を vt+0x224 へ、買ラッパ 0x5E1200 は **player([0xB333C4])** を渡す（B5/B6 実読・v262 L61 [部分確定]） | 要検証 | 低: vt+0x224 の意味（相互 disposition/他者 disposition）確定後に追随。現ポートは「商人 disposition」で統一しており買側が逆可能性 [部分確定] — 単独では修正指示しない |

### 3-2. テスト固定化表（`tests/barter_price_tests.cpp` 12 テストがどちらの式を固定しているか）

**min 誤式を固定**（#1/#2 の max 化と同時に期待値差替が必要 — さもないと CI が誤式を保護する）:

| テスト | 行 | 現期待値 | max 化後（草案） | 備考 |
|---|---|---|---|---|
| buy_coefficient_known_values | L120-133（コメント L118-119） | 1.5999286 / 2.01 / 1.6749286 | **2.01 / 2.01 / 2.085** | sk=100 のみ min=max で無風 |
| sell_coefficient_known_values | L137-150（コメント L135-136） | 0.60710678 / 0.40 / 0.53210678 | **0.40 / 0.40 / 0.325** | 同上 |
| disposition_term_truncates_toward_zero | L154-166 | 1.5399286 / 1.6599286 | **1.95 / 2.07** | trunc 挙動自体は維持（95→+4・5→−4 据替） |
| buy_price_defaults | L214-226 | 400 / 8 / 2 | **503 / 11 / 3** | ceil(2.01×250)=503・ceil(10.05)=11・ceil(2.01)=3（base 床 250/5/1 に胜らせず） |
| buy_price_haggle_floor | L228-240 | 250 / 250 | **253** / 250 | 250×(2.01−1.0)=252.5→ceil=253（床 250 は下）、extreme は 250 不変 |
| sell_price_defaults | L242-256（コメント L243） | 152 / 4 / 1 | **100 / 2 / 1** | 1 は ceil(0.40)=1 で不変（コメント「0.6071×1」→「0.40×1」） |
| buy_sell_spread | L275-288 | 400 / 152 | **503 / 100** | 関係 buy>sell は維持 |

**固定内容が正しい/中立**（差替不要）:
| テスト | 行 | 状況 |
|---|---|---|
| effective_skill_clamps | L101-116 | 現式=原版式（§4-2）で正しい値を固定（0/50→0・50/50→50 は `Merc+0.4Luck−20` でのみ成立。v277 の「L100-116 差替」は撤回）。sk 引数 role は**変更不要** |
| round_price_ceils | L168-181 | ceil 固定で正しい。ただし入力定数 151.7767 は 0.607×250 由来の歴史的数値（将来 clean 時に 100.4 系へ置換可・無風） |
| stolen_discount_and_sentinel | L183-195 | ×0.01 固定で正しい（tier ゲートは #8 側） |
| haggle_acceptance | L197-210 | 原版一致式で正しい |
| sell_price_haggle_cap_and_skill_clamp | L258-273 | sk=100/900(clamp→100) 域で min=max 同一のため**全値不変** |

### 3-3. 修正案 before→after 概要（**[PORT] 承認ゲート・本シリアルでは実装しない**）

1. **高（#1/#2）**: barter.cpp L36・L44 `std::min(` → `std::max(`。同一セッションで §3-2 上表 7 テストの期待値・コメント（L118-119/L135-136/L243）差替。v277 §1 メモの「既定 disp50・value250 → 売=100・買=ceil(2.01×250)=503」と一致することを確認。
2. **高（#11）**: barter.cpp L169-172 の拒否分岐 → `earned = std::min(earned, purse)` クランプで成立（purse≤0 は無制限扱い=原版ガード相当、v262 §6）。財布供給源（shopGold_ 既定 5000/CNTO 合計）の ACBS barterGold 化は v277 §2 高項目と同一 API — 併走承認。
3. **中（#8/#9/#13）**: stolen tier<1 ゲート・player tier>3 base override・NPC AV（Luck/Mercantile/disposition）の ESM 配線。#3 の低（命名）と一括可。
4. **低（#3/#5/#12/#14/#7）**: コメント/命名（Barter→Mercantile、fBarterDispositionMod→fBarterDispBase、ε 注記、vt+0x224 非対称の検証注記）と tradeGold の ceil/0 円整合。

---

## 4. 独立追試（capstone 5.0.7 実バイト＋Ghidra 第二訳者＋.rdata/.data 実読）

- スクリプト: `%TEMP%\opencode\v279_probe5.py`（→`v279_probe5_out.txt`）、`v279_probe6.py`（→`v279_probe6_out.txt`）。前走分 `v279_disasm.py`/`v279_disasm2.py`（→同名 out）は窓同一・全命令一致で**再確認済み**（B1-B6 窓のバイト列は probe5 側 dump と同一）。
- 前走の実行系 x87 ハーネス（`v279_cpu_test.py`/`v279_cpu2.py`/`v279_probe3.py`/`v279_probe4.py`）は不採: FCOM の C3 を ah ビット 2 と誤る建模・`jc` で `test ah,5;jp`（PF は偶数パリティ）を代替する等の欠陥で出力 nan/誤値（out 生ファイルで確認）。裁定は命令セマンティクス表＋Ghidra 独立翻訳で行う。

### 4-1. sqrt 引数の境界方向 = **max**（①一致）

 FUN_005481C0（買、0x5481C0-0x5481DA）/ FUN_00548240（売、0x548240-0x548257）同一 idiom（cmov ではなく x87 分岐）:
```
fild dword [esp+4]        ; st0 = sk(int)
fld  qword [0xA309F0]     ; st0 = 100.0, st1 = sk
fcom st(1)                ; 100 vs sk: sk<100 → C0=1 / sk>100 → C3=1 / sk==100 → 無
fnstsw ax / test ah,5     ; ah bit0=C0, bit2=C2。sk>100 では C3 のみ=ah&5=0
jp  +4                    ; PF=偶数パリティ: ah&5 ∈ {0,5} で jp 実行
  jp 実行側: fstp st(0)   ; 100 を捨て sk を残す
  不実行側: fstp st(1)    ; sk を捨て 100 を残す
fsubr qword [0xA37478]    ; 200.0 − (選択値) → 200 − max(sk,100)
call 0x982C30             ; = sqrt（fsqrt 実体 @0x982C6C、H2 で確認）
```
sk<100 → 100 採用、sk>100 → sk 採用 = **max**。以降 買 `− 9.9×√…`（fmul [0xB375B0]）/ 売 `5×√…−10+1.5×adj`（fmul [0xB375B8]、fadd [0xB375A8]、fild×fmul [0xB37590]、int 除算 imul 0x66666667/sar2）は v262 §3-4・docs L10004-10005 の係数割当と一致（√引数の min 表記のみ docs 側が誤り＝§1-1）。sk は 0x547B90 で 0..100 クランプ済のため **√項は常に 10**（既定乗率 買 2.01 / 売 0.40）。

### 4-2. sk 合成の引数役割（②**不一致** — 依頼/v262/v277 が誤り）

 FUN_00547B90（0x547B90-0x547BB0）:
```
0x547B90  fild dword [esp+8]   ; ×0.4 される側 = 第1引数 push（callee param_2）
0x547B94  fmul dword [0xB37390]; × fActorLuckSkillMult(0.4)
0x547BA2  fiadd dword [0xB37388]; + iActorLuckSkillBase(int −20)
0x547BAC  fild dword [esp+4]   ; 素加算側 = 第2引数 push（param_1）
0x547BB0  fadd dword [esp+8]   ; param_1 + (param_2×0.4 − 20)
（0x547BB4-0x547BF4 で clamp 0..100、Ghidra L243882-243886 同型）
```
- cdecl スタック対応: 先 push の値が callee の [esp+8]、後 push が [esp+4]。**[esp+8]=×0.4 側**。
- ラッパ push 順序（B5/B6・probe6 Z2）: `push 7 → call GetAV(vt+0x284) → push eax → push 0x1D → call GetAV → push eax → call 0x547B90`（sell 0x5E11AD-0x5E11C3 / buy 0x5E120D-0x5E1223 で同一順序）。→ **×0.4 側 = GetAV(7)**、素加算側 = GetAV(0x1D)。
- AV 名表（.data 0xB0A1A8、**ファイルバック実読**・48 エントリ): idx **0x07='Luck'**(0xA51EA8)、idx **0x1D='Mercantile'**(0xA51E20)。idx3='Agility'/6='Personality'/0x20='Speechcraft'/0x2C='CastingPenalty' と docs L8115・L9770・v262 §8 とも和音 ✓。
- Ghidra 第二訳者（docs/Oblivion_decompiled.c L243881）: `fVar1 = (float)param_1 + (float)param_2 * _DAT_00b37390 + (float)_DAT_00b37388;` — ×0.4 は param_2=[esp+8]=Luck 側で独立一致。
- 丸め: 0x9828C0（H1）= SSE 経路 `cvttsd2si`（ゼロ方向 trunc、[0xBAABE0] 有効時）/ x87 経路 _ftol2。sk→int は trunc ✓（v262 L116 の trunc 記述は正しい）。
- **結語**: 原版 `sk = trunc(clamp(Mercantile + 0.4×Luck − 20, 0..100))`。**docs L10245・port barter.cpp L31-33 は原版と一致（②は誤情報）**。乗率への数値影響は §3-1#1-2 の max 化でゼロになる（√項一定）ため、②の残影響は canHaggle の sk 差分のみ。GMST 名の意味論（「Luck→スキル換算乗数」）も本裁定支持。

### 4-3. FUN_00484370 の実体 = **ceilf**（③一致）

```
0x484370  fld [esp+4] / fld st(0) / call 0x9828C0(=trunc、4-2) ; eax = t
0x48437F  fild t → fstp → fld; fld st(0); fsubrp st(2); fxch st(1) ; st0 = x − t
0x484391  fcomp qword [0xA2FC68](=0.0) → fnstsw → test ah,1(C0=x−t>0)
0x48439C  je スキップ / 0x48439E fadd qword [0xA2F928](=1.0)
```
t=trunc(x)（ゼロ方向）に対し「x−t>0 のときのみ +1」= **ceilf**（2.5→3 / −2.5→−2 / 3.0→3。floor なら「x−t<0 で −1」型になる）。Ghidra L106001-106013 は同一意味（`t − x < 0.0` で +1）。docs L7953/L7960/L10347（0x484370=ceilf、0x404F70=floorf、0x4842F0=truncf）が正、v262 L68/L74/L144/L190/L230・v264 L79/L80 の「floor」は誤称（§2-1/§2-2）。呼出側 0x488F58(買)/0x489136(売) の単価丸め = ceil。

### 4-4. GMST 和音（name ↔ スロット ↔ 既定値 ↔ 消費者）

- 保存レイアウト（H3/H4 実バイト）: 登録ヘルパ `0x41BAE0(name VA, float 既定)`/`0x419A70(name VA, int 既定)` は **`fstp [esi]`/`mov [esi],ecx` = 値を slot+0、name を slot+4** に格納（ecx=登録時 mov する slot base＝消費者読取 VA）。v262 L13 の「値は slot+4 を読む」は反向誤記（§2-4）。
- 登録窓・実読表（S1/A2/probe6 L1）:

| 名前文字列（実読） | slot base(=値) | 既定値源 | 既定実数 | 消費者（追試実測） |
|---|---|---|---|---|
| 'fBarterDispBase' | **0xB37590** | [0xA35AA4] | **1.5** | fmul 0x548219(買)/0x548290(売) ✓ |
| 'fBarterBuyBase' | **0xB375A0** | [0xA34ABC] | **300** | fld 0x5481DC ✓ |
| 'fBarterSellBase' | **0xB375A8** | [0xA418D8] | **−10** | fadd 0x548271 ✓ |
| 'fBarterBuyMult' | **0xB375B0** | [0xA5A1A0] | **9.9** | fmul 0x5481F0 ✓ |
| 'fBarterSellMult' | **0xB375B8** | [0xA31E2C] | **5** | fmul 0x548264 ✓ |
| 'iActorLuckSkillBase' | **0xB37388** | push −0x14（0x9EAA60） | **−20**(int) | fiadd 0x547BA2 ✓ |
| 'fActorLuckSkillMult' | **0xB37390** | [0xA47E6C] | **0.4** | fmul 0x547B94 ✓ |

- **依頼文の提示5アドレスの裁定**: [0xB375AC]=−10 ✗（AC は SellBase エントリの **name field**。値 −10 は 0xB375A8）/ [0xB375BC]=5 ✗（同、値は 0xB375B8）/ [0xB37594]=1.5 ✗（値は 0xB37590）/ [0xB37388]=−20 **✓**（唯一 base 表記）/ [0xB37394]=0.4 ✗（値は 0xB37390）。**値の和音（−10/5/1.5/−20/0.4）自体は全件確認**だが、アドレスは consumer VA 基準（=登録 ecx スロット）へ読み替えが必要。
- 隣接確認: fBarterHaggleDispMult 0xB37598=0.5・HaggleBase 0xB375C0=0.55・HaggleMax 0xB375C8=40・HaggleCurve 0xB375D0=1.0(fld1)・**fBarterDispositionMod 0xB375D8=0.01**（barter 乗率の 1.5 は fBarterDispBase で別物＝v277 §1 命名注記が正しいことの確認）・iBarterDispositionPenalty 0xB375E0=−1・iPerkExtraBarterGoldMaster 0xB375E8=200。スロット 0xB37588=0.2='fActorSwimBreathDamage'（barter 窓の外・v262 表に無い件）。
- 値スロットは .data bss（実行時 ESM GMST で上書きされる既定登録窓のみが静的証拠）— ESM 再スキャンは契約上不要（既定値は上表 .rdata 実読で和音完結）。
- 第2 Cluster（0xA1EE30/0xA1EE40・probe6 L2）は登録でなく `mov ecx, slot; jmp 0x4055E0` 型のアクセッサ thunk（誤混入注意の参考情報）。

### 4-5. 追試裁定まとめ

| 依頼項目 | 裁定 |
|---|---|
| ① max（docs min 誤記） | **一致**（docs L10004/10005 訂正妥当。v262 は元から max で正） |
| ② sk=Luck+Merc×0.4−20 | **不一致**（原版=Merc+0.4·Luck−20。docs/port 正、v262/v277/依頼が誤） |
| ③ ceilf（0x484370） | **一致**（docs 訂正済・tmp/re 誤称は §2 注記。port ceil 実装済） |
| GMST 和音 | 値は全件一致／提示アドレスは 4/5 が +4 ズレ（§4-4） |

---

## 5. 承認ゲート申請一覧（本シリアルはドラフトのみ・適用ゼロ）

### A. 人間／docs 編集権限セッション向け
| ID | 内容 | 対象 | 根拠節 |
|---|---|---|---|
| H1 | docs §57.5 L10004/L10005 `min(x,100)` → `max(x,100)`（before→after 文面適用） | docs/RE_EXE_REPORT.md | §1-1 |
| H2 | docs ②③ は**訂正不要**（L10245/L7953/L7960/L10347 现状正确）の公式確認 | docs（変更なし） | §1-2/§1-3/§4-2/§4-3 |
| H3 | docs L10245「0x5E1500 = haggle」の検証注記（canHaggle ラッパ実測=0x5E10F0。0x5E1500 未検証） | docs（任意・低） | §1-2 派生 |
| H4 | v262（floor 5+1行・sk 3行・slot 表記規約）・v264（2+1行）・v277（4行）の**本文反映**（本成果物 §2 が代替で、反映は編集権限セッション判断） | tmp/re 本体 | §2 全 |

### B. 実装側（port 承認ゲート）向け
| ID | 内容 | 対象 | 優先 |
|---|---|---|---|
| P1 | barter.cpp L36/L44 `std::min`→`std::max` ＋ §3-2 表 7 テストの期待値・コメント同時差替（さもないと CI が誤式を保護） | game/barter.cpp・tests/barter_price_tests.cpp | **高** |
| P2 | 財布拒否→`min(earned, purse)` クランプ成立（barter.cpp L169-172）。ACBS barterGold 供給（v277 §2 高）と同一 API で併走 | barter.cpp・merchant.*・（v277 §2 と連動） | **高** |
| P3 | stolen tier<1 ゲート・player tier>3 base override・商人 AV（Luck/Mercantile/disp）ESM 配線 | barter.cpp/merchant.*/esm 側 | 中 |
| P4 | 命名・コメント系: `kBarterDispositionMod`→fBarterDispBase 旨・merchant.h L86 Barter→Mercantile・vt+0x224 非対称注記・ε 注記・tradeGold ceil/0円 裁定 | barter.h L28・merchant.h L85-96・barter.cpp L23-29 | 低 |

### C. 撤回項目（追試不一致で**実施不要** — 承認も不要）
| ID | 内容 | 出所 |
|---|---|---|
| W1 | v277 §1「合成スキル sk」行の [PORT]中（引数再配置 `luck + 0.4×skill − 20` ＋テスト L100-116 差替） | v277 L31/L44-45/L176 → §4-2 で否定 |
| W2 | v262 L230「tradeGold lround→**std::floor**」提案（正=ceil、port は既に ceil） | v262 L230 → §4-3 |
| W3 | 「docs L10245 は誤記」前提の一切の docs 書換草案 | v277 §3-③ → §1-2 |

### D. 契約遵守宣言
- 編集したのは本ファイルのみ。docs/Oblivion_decompiled.c・tmp/re 既存・port ソースは読取のみ。git 書き込み無し。ESM 読取/再スキャン無し（既定値は EXE .rdata 実読で充足）。selftest 4/4（着手時）・capstone 5.0.7。中間物: `v279_probe5_out.txt`(543行)/`v279_probe6_out.txt`＋前走 `v279_disasm_out.txt`/`v279_disasm2_out.txt`（再利用・全窓一致）。
