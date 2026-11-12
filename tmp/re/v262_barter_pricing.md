# V262: 原版 barter 価格式（買取り/販売）— TODO 258・依頼4（`kProvisionalSellRatio` 差し替え根拠）

- 日付: 2026-11-03
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam）
- 依頼: ①買取り（player→NPC）価格式 ②販売（NPC→player）価格式 ③Disposition/Speechcraft の係数 ④商人 flag/PER 関与 ⑤ポート `kProvisionalSellRatio` との差分・修正ドラフト
- 足場: v261（barterGold=`word[TESNPC+0x30]`、`FUN_005FAA70`、サービス flag 不在）— 再検証せず引用
- スクリプト: `%TEMP%\opencode\v262_*.py`（依頼文は tmp/re 希望だが本セッション規約（スクリプト=TEMP）を優先＝v261 と同例）
- 証跡: `tmp/re/v262_gmst_register.txt`（GMST 登録窓 0x9EB5E0-0x9EB880）、`v262_548_disasm.txt`（0x548000-0x548B00 生 disasm）、`v262_5e1000_disasm.txt`、`v262_gmst_xrefs.txt`（fBarter* 消費者 VA 一覧）

## 結論サマリ

### 確定
1. **barter 用 GMST 12 種の登録窓とグローバルスロット**（`.text 0x9EB600-0x9EB84F`、float=call `0x41BAE0(name,default)` / int=call `0x419A70(name,default)`、ecx=スロット。値は slot+4 を直接 `fld/fild` で読む）:

| GMST | スロット(値 field) | 既定値 | 既定 float の VA | 登録命令 VA |
|---|---|---|---|---|
| fBarterDispBase | 0xB37594 | **1.5** | [0xA35AA4] | 0x9EB630 |
| fBarterHaggleDispMult | 0xB3759C | **0.5** | [0xA3D65C] | 0x9EB660 |
| fBarterBuyBase | 0xB375A4 | **300** | [0xA34ABC] | 0x9EB690 |
| fBarterSellBase | 0xB375AC | **−10** | [0xA418D8] | 0x9EB6C0 |
| fBarterBuyMult | 0xB375B4 | **9.9** | [0xA5A1A0] | 0x9EB6F0 |
| fBarterSellMult | 0xB375BC | **5** | [0xA31E2C] | 0x9EB720 |
| fBarterHaggleBase | 0xB375C4 | **0.55** | [0xA5A1C8] | 0x9EB750 |
| fBarterHaggleMax | 0xB375CC | **40** | [0xA56F84] | 0x9EB780 |
| fBarterHaggleCurve | 0xB375D4 | **1.0**（fld1@0x9EB7B0） | — | 0x9EB7B0 |
| fBarterDispositionMod | 0xB375DC | **0.01** | [0xA34BA0] | 0x9EB7E0 |
| iBarterDispositionPenalty | 0xB375E4 | **−1**（push -1@0x9EB810） | int | 0x9EB810 |
| iPerkExtraBarterGoldMaster | 0xB375EC | **200**（push 0xC8@0x9EB830） | int | 0x9EB830 |

   名称 VA: 'fBarterDispBase'=0xA5A148 …（文字列は 8byte 対境で float 既定値と交互配置、raw ダンプ v262_2.py 出力）。
   ※ `iPerkExtraBarterGoldMaster` = v261 で `(0x56A300()==4 → +[0xB375E8])` と残した謎の正体。**0x56A300 = スキルティア関数**（下記 §5）なので「ティア4（高熟練）で財布 +200」。

2. **GMST 消費者は 0x548xxx の少函数群に集約**（`.text` imm32 スキャン、`v262_gmst_xrefs.txt`）:
   - fBarterBuyBase/Mult/DispBase → `FUN_005481C0`（買乗率）
   - fBarterSellBase/Mult/DispBase → `FUN_00548240`（売乗率）
   - fBarterHaggleCurve/Base/DispMult → `FUN_00548130`（ハッグル可否/成否）
   - fBarterHaggleMax → `FUN_005BD1D5`（NegotiateMenu 更新内）
   - fBarterDispositionMod/iBarterDispositionPenalty → `FUN_005489E0`（PersuasionMenu の disposition 増減変換、callers 0x59A9E2/0x59AAB0/0x59AD37/0x59ADE6）
   - iPerkExtraBarterGoldMaster → `FUN_005FAA70`@0x5FAAC9（GetBarterGold）

3. **売価（player→NPC）乗率式 `FUN_00548240`**（命令列 0x548240-0x5482AE・手解読＋Ghidra `docs/Oblivion_decompiled.c` L244213-244226 一致）:
```
SellMult = 0.01 × ( floor? なし・実数:
      sqrt(200 − max(sk, 100)) × fBarterSellMult      ; 既定 sqrt(100)×5 = 50
    + ((disp − 50) /int10) × fBarterDispBase          ; 既定 1.5×adj
    + fBarterSellBase )                                ; 既定 −10
```
   命令列: `fild argA`→`fld q[0xA309F0]=100.0`→`fcom st(1)/test ah,5/jp` で **max(argA,100)**（a≤100→100、a>100→a）、`fsubr q[0xA37478]=200.0`、`call 0x982C30`(=x87 **sqrt** ラッパ、fsqrt@0x982C6C)、`fmul [0xB375BC]`(SellMult)、`fadd [0xB375AC]`(SellBase)、`fild ((disp−50)/10)`（`imul 0x66666667; sar edx,2` = 10 除・ゼロ方向切り捨て）、`fmul [0xB37594]`(DispBase)、`faddp`、`fmul q[0xA3D8E8]=0.01`。

4. **買価（NPC→player）乗率式 `FUN_005481C0`**（命令列 0x5481C0-0x54823A・Ghidra L244194-244209 一致）:
```
BuyMult = 0.01 × ( fBarterBuyBase                       ; 既定 300
    − sqrt(200 − max(sk,100)) × fBarterBuyMult          ; 既定 99
    − ((disp − 50)/int10) × fBarterDispBase )           ; 既定 ±1.5×adj
```
   順序: `fstp q[esp]` に BuyBase を退避 → `fsubr q[0xA37478]`(200−max) → sqrt → `fmul [0xB375B4]`(BuyMult) → `fsubr [esp]`(BuyBase − 99) → −adj×1.5 → ×0.01。

5. **argA= sk（合成販売スキル int）と argB= disposition の供給元** = `FUN_005E11A0`（売用）/ `FUN_005E1200`（買用）ラッパ（Ghidra L346249-346289 と手 disasm `v262_5e1000_disasm.txt` 一致）:
   - `Actor::vt+0x284`（=`0x5F1970` GetAV int）で **AV 7 = Luck**・**AV 0x1D = Mercantile** を読む（AV 名表 `.data 0xB0A1A8+idx*4`、idx7='Luck'@0xA51EA8、idx0x1D='Mercantile'@0xA51E20、登録窓 0x9F9A00 で実測）
   - `FUN_00547B90(Luck, Mercantile)` = **clamp(Luck + Mercantile×fActorLuckSkillMult(0.4) + iActorLuckSkillBase(−20), 0, 100)**（命令列 0x547B90-0x547BF4、clamp は Ghidra L243874-243887。GMST 名: 0xA59B40/0xA59B54、slot 0xB3738C/0xB37394、既定 −20/0.4=[0xA47E6C]）
   - **`Actor::vt+0x224` 引数の非対称（実測）**: 買ラッパ 0x5E1200 は `push [0xB333C4]`（player）を thiscall 引数に（0x5E122C-0x5E1240）、売ラッパ 0x5E11A0 は **self を push**（0x5E11CC-0x5E11DA: `push esi; mov ecx,esi`）。vt+0x224=`0x5EA800` = `[actor+0x58]` base form 経由の virtual +0x3D0（self 引数時は ACBS 由来の base disposition=word[NPC+0x2C] 経路と推定・[部分確定]）。乗率の disp 項は既定 50±10 刻みで ±0.015×n。
   - **`FUN_0056A300(v)` = スキルティア**: `v<[B3A4F8]→0 /<[B3A500]→1 /<[B3A508]→2 /<[B3A510]→3 /else 4`（境界= fSkillUseThreshold 系 GMST オブジェクト。命令列 0x56A300-0x56A33B）
   - `FUN_005F1910(idx)` = **floor(GetAVFloat(idx))**（0x5F1910-0x5F1961: 0x5EAD00 float AV→trunc→frac<0 補正で floor）
   - `0x5481C0/0x548240` の argA は int(clamp 済み sk ≤100) なので **sqrt 項は定数 sqrt(100)=10**（既定値で Buy=2.01−0.015adj・Sell=0.40+0.015adj）。sk が 100 を超えるのは AV 修正時のみ（=部分確定の理由、§未達参照）

6. **価格適用・丸め・財布上限 = `FUN_00488E50`**（Ghidra L110371-110476、callers 0x48F578/0x58454E/0x584561/0x59A6D1…0x5A503D の 9 箇所＝メニュー系サービスコード。引数: this=item コンテナ、param_2=取引種別 flag、param_3=商人 actor、param_4=方向 0=売/1=買）:
   - 物品基準価 = 物品 form の **TESEntry 系コンポーネント**（`FUN_009832E6(..., PTR_PTR 0xB03248 基底, 0xB05C24 目的)`）の `[comp+4]` int（= TESObjectITEM::DATA value）。ench 系加算分岐あり（[comp+0x34]==3 で +0x24 リスト走査、`[item+0x58] bit8` で 0x413320/0x403C00 分岐 — 部分確定）
   - **売**: `price = floor( (SellMult + [player+0x11C]×0.01) × base )`、**買**: `floor( (BuyMult − [player+0x11C]×0.01) × base )`（floor=`FUN_00484370`、命令列 0x484370-0x4843AC: trunc→frac 補正。×0.01=[0xA3D8E8]）
   - `[player+0x11C]` = **取引セッション調整値（int）**。書手= `0x5BD4BD`（NegotiateMenu vtable[3]（vtable VA 0xA6CF0C、slot 0xA6CF18 に FUN_005BD490）msg 6 で `[0xB3B410]` から複写）と `0x59A3BD`（PersuasionMenu・NPC subsystem 0x5C の float+0xC を floor→player+0x11C）。意味=売買乗率への ±% 加算（ハッグル/説得の価格効果）
   - **ティア効果**: `tier=FUN_005F23B0(0x1D)`=ティア(floor(自 Mercantile))。**tier<1**（売のみ）→ `price = FUN_00484850(1)×2.0×price`（stolen 系係数、[0xA3B150]=2.0f、部分確定）；**tier>3** → 売/買とも **price=base に補正**（高 Mercantile で基準価取引＝「ペナルティ無し」）
   - **上限**: 売は `purse = FUN_005FAA70(商人)`（GetBarterGold）で **min(price, purse)**。**purse<0 なら (float)purse+4.294967296e9**（=[0xA2FC78]、signed→unsigned bit 変換ガード）→ 負 purse は実質上限無し扱い
   - 価格下限ガードは**この関数内に無し**（floor 後に base 補正の分岐はあるが、0/負 clamp は呼び手側＝UI 側。§5bd2 で ebx<100→100 等の %表示クランプあり）

7. **端数処理まとめ**: 乗率合成の dispadj=(disp−50)/10 と (disp−10)/10 は**符号付き整数除算（ゼロ方向切り捨て）**（`imul 0x66666667 / sar 2`）。乗率自体は float 保持、最終価格は **floor**（FUN_00484370）。

8. ** Speechcraft（AV 32）は価格式に一切関与しない**（確定）: 0x548130/0x5481C0/0x548240/0x488E50/0x5E1100/0x5E11A0/0x5E1200 の読取 AV は **7=Luck と 0x1D=Mercantile のみ**（命令列 `push 0x7`/`push 0x1d`、0x5E110C/0x5E111A/0x5E11AD/0x5E11BA/0x5E1207/0x5E121C 等）。Speechcraft→価格は **Disposition を介しての間接のみ**（PersuasionMenu 側の作用 — § Persuasion）。
9. **barter flag/PER 関与**: 乗率式は商人 object の GetAV/GetDisposition のみで**クラス/商人 flag _readなし_**（v261 と整合: 商人性ビットは存在しない）。能力値 PER(Oblivion の Personality は **AV 6**) は**乗率式から読まれない**（読まれるのは Luck=7 と Mercantile=29）。fBarter* GMST の上書きは mod 側で可能（既定値は上記 §1）。

### 部分確定
- **`[player+0x11C]` = セッション割引率 %（haggle adjustment）** — 読み方は確定（売 `+adjust×0.01`、買 `−adjust×0.01`、CanHaggle で `−0.55×adjust`、NegotiateMenu 表示で buy%≥100/sell%≤100 クランプ 0x5BD2B9-0x5BD2CC）。**生成元**: ①`0x5BD4BD`（NegotiateMenu vtable[3] msg6=open で `[0xB3B410]` を複写。`[0xB3B410]` の書手=0x5BD1DB の `[[ebx]]` — ebx はメニューが保持する割引 int オブジェクト）②`0x59A3BD`（PersuasionMenu: NPC subsystem idx **0x5C** の float+0xC を floor(0x422DC0→0x41E210)→player+0x11C）。subsystem 0x5C のレコード意味（Persuasion 蓄積値）は未追跡。
- **stolen 補正 `FUN_00484850`**（Ghidra L110302 付近・0x484850-）: 売方向・**player Mercantile tier<1 のみ** `price = ret × 2.0([0xA3B150]) × price`。ret は stolen 価値率（0..0.5 域想定）— 関数末尾分岐（0xA30634=-1.0f 比較・PTR 0xB05CF4 コンポーネント走査）の全量読解未了。
- **ハッグル判定 `FUN_00548130` の戻り用途**: 式は確定（下§疑似コード）、引数 p1/p2 の「誰のスキルか」=ラッパ `FUN_005E10F0` の this/arg2 の 2 actor（0x5E1104=esi=this、0x5E111C=edi=[esp+0x1C]）。呼び手 PersuasionMenu 0x59A9C0/0x59AD15/0x5D978D で成否フラグとして使用。
- **fSkillUseThreshold 系**: `FUN_0056A300` の tier 境界スロット = 0xB3A4F8/0xB3A500/0xB3A508/0xB3A510（cmp 命令 0x56A304-0x56A333 で実測）。esm 側値（0/25/50/75 相当）は Oblivion.esm 再スキャン禁止のため未裏取り。**tier>3 → price=base**（買/売共通）と **tier<1 → 売 stolen×2 補正** は命令列確定（Ghidra L110444-110455/110470）。
- **vt+0x224 の方向意味**: 実装 0x5EA800 が「他 Actor に対する disposition」のどちら側か（merchant→player か player→merchant か）は base 側 virtual +0x3D0 の解読未了。式への入力であること・既定 50 中心であることは確定。
- **物品 base value のコンポーネント**: `FUN_009832E6(list, 0, PTR 0xB03248, PTR 0xB05C24, 0)` の戻り `[+4]` int（TESEntry 系 value）。ench 加算分岐 `[+0x34]==3`→`[+0x24]` リスト走査・`[item+0x58] bit8` ゲート（0x413320 vs 0x403C00×0x5489A0）— 意味は未追試。

### 未達（試した地址・否定根拠）
1. **BarterMenu という RTTI クラスは存在しない**: `.text/.rdata/.data` 全域 substring 'BarterMenu'/'BARTERMENU' スキャン 0 hits（v262_1.py、capstone 環境の find 走査）。取引 UI は **NegotiateMenu（vtable 0xA6CF0C、ctor FUN_005BCE40 L325963、update FUN_005BD080、msg ハンドラ FUN_005BD490）** + サービスコード群（0x5845xx/0x59Axxx/0x488-0x489 価格関数群）で構成。v185/v187/v188 のメニュー裁定との矛盾なし。
2. **商人財布の取引時減算経路**: `word[TESNPC+0x30]` への word store 全 10 箇所走査（0x4E4C04=レコード clear、0x4E6190=TESNPC ctor 0 初期化、0x4E75B3=4CC ロード窓、0x6D5C54/0x6D6059/0x6D6147/0x6D6772/0x6D6985/0x6DC288/0x6DC556=TESNPC レコード生成/リセット群、v262_25/26/27.py）。**取引コミットで base+0x30 を減らす命令は存在しない** → 財布増減は `SetBarterGold`(impl 0x50CBC0)/`ModBarterGold`(impl 0x505C80) の**スクリプト opcode 経由のみ**（両者 E8 call sites 0=直接呼び出しなし、dispatch 経由）。セッション中の残金表示は `DAT_00B13E94`（-1=全数 sentinel の数量変数ではなく**残金 sentinel**、L306401/306431 の `= -1`/`−=iVar8` を伴う会話サービス経路でのみ更新。=Persuasion/サービス側で商人 gold を AddItem 起算する方式と整合、完全追跡は未達）。
3. **fBarterHaggleMax=40 の強制箇所**: 0x5BD1D3-0x5BD1F7 で cvt→UI フィールド 0xFB7・[esp+0x18] に配るのみ確認。交渉回数の上限判定ロジック（カウンタ格納先）は未追跡。
4. **0x5BExxx（Speechcraft 読取窓）の所属クラス**: push 0x20+vt0x284 スキャンで 0x5BE90B/0x5BE9A7/0x5BE9BE/0x5BEBEF/0x5BEC09/0x5BF25F/0x5BF2E6/0x5BF2FB/0x5BF533/0x5BF5BA/0x5BF5CF/0x5BFA6A（v262_28.py）= Persuasion 系 skill check と推定。**価格式側（0x548xxx/0x5E1xxx/0x488E50）に push 0x20 は 0 箇所**＝「Speechcraft 直接係数なし」の否定根拠。

## 疑似コード（C++ 粒度・既定値併記・VA 注記）

```cpp
// ==== 定数 ====（既定値=EXE .rdata 実測・上書き可=Oblivion.esm GMST）
static const float  kBuyBase   = 300.0f; // fBarterBuyBase   値slot 0xB375A4 既定[0xA34ABC]
static const float  kSellBase  = -10.0f; // fBarterSellBase  0xB375AC 既定[0xA418D8]
static const float  kBuyMult   = 9.9f;   // fBarterBuyMult   0xB375B4 既定[0xA5A1A0]
static const float  kSellMult  = 5.0f;   // fBarterSellMult  0xB375BC 既定[0xA31E2C]
static const float  kDispBase  = 1.5f;   // fBarterDispBase  0xB37594 既定[0xA35AA4]
static const double kHaggleCurve       = 1.0;   // fBarterHaggleCurve 0xB375D4
static const float  kHaggleBase = 0.55f; // fBarterHaggleBase 0xB375C4 既定[0xA5A1C8]
static const float  kHaggleDispMult = 0.5f; // fBarterHaggleDispMult 0xB3759C
static const int    kHaggleMax  = 40;    // fBarterHaggleMax 0xB375CC 既定[0xA56F84]
static const float  kLuckSkillMult = 0.4f;   // fActorLuckSkillMult 0xB37394
static const int    kLuckSkillBase = -20;    // iActorLuckSkillBase 0xB37388 int既定=push -0x14
static const int    kExtraPurseMaster = 200; // iPerkExtraBarterGoldMaster 0xB375EC

// ==== 合成スキル (0x547B90, callers 0x5E1113/0x5E1141/0x5E11C3/0x5E1223) ====
// actorの AV idx 7=Luck, 0x1D=Mercantile（AV名表 .data 0xB0A1A8+idx*4）
inline int merchantSkill(const Actor& npc) {          // 戻り float→int(trunc) 使用
    float v = (float)npc.GetAV(7)                    // Luck
            + (float)npc.GetAV(0x1D) * kLuckSkillMult// Mercantile×0.4
            + (float)kLuckSkillBase;                 // −20
    return trunc(clamp(v, 0.0f, 100.0f));            // clampは0x547B90末尾, truncは0x9828C0
}

// ==== 売乗率 player→NPC (FUN_00548240 @0x548240-0x5482AE) ====
// argA=merchantSkill(商人), argB=商人のdisposition値（引数非対称は§部分確定参照）
inline float sellMultiplier(int sk, int disp) {
    float y = 200.0f - (float)std::max(sk, 100);     // 既定域(sk≤100)で y=100 固定
    float s = std::sqrt(y) * kSellMult;              // 0x982C30=sqrt → 既定 50
    int   adj = (disp - 50) / 10;                    // int除算 ゼロ方向切捨て (imul 0x66666667)
    return 0.01f * (s + kSellBase + (float)adj * kDispBase);  // 命令順=faddp列 (0x548264-0x548296)
}
// 展開（既定値 sk任意・disp=50 で）: 0.01*(50 + (-10) + 1.5*((disp-50)/10)) = 0.40 + 0.015*((disp-50)/10)

// ==== 買乗率 NPC→player (FUN_005481C0 @0x5481C0-0x54823A) ====
inline float buyMultiplier(int sk, int disp) {
    float s = std::sqrt(200.0f - (float)std::max(sk, 100)) * kBuyMult; // 既定=99
    int   adj = (disp - 50) / 10;
    return 0.01f * (kBuyBase - s - (float)adj * kDispBase);
}
// 展開: 2.01 - 0.015*((disp-50)/10) - 0.01*(sk>100時のみ s 減少分)

// ==== 最終価格 (FUN_00488E50 @0x488E50) ====
int itemPrice(const Item& item, Actor& merchant, bool isBuy, int sessionAdjustPercent) {
    float base = (float)item.entryValue;             // TESEntry[+4]
    float mult = isBuy ? buyMultiplier(merchantSkill(merchant), dispArg(isBuy, merchant))
                       : sellMultiplier(merchantSkill(merchant), dispArg(isBuy, merchant));
    mult += isBuy ? -0.01f * (float)sessionAdjustPercent
                  :  0.01f * (float)sessionAdjustPercent;   // [player+0x11C]×k_0.01
    float price = std::floor(mult * base);           // floor = FUN_00484370
    int tier = skillTier(trunc(playerAV(0x1D)));      // FUN_005F23B0(0x1D) 0x56A300ティア化
    if (tier > 3) price = base;                      // 高Mercantile: 基準価取引（買/売共通）
    if (tier < 1 && !isBuy) {                        // 売 & 低Merc: stolen補正 [部分確定]
        float v = stolenValueRatio(true /*FUN_00484850(1)*/);
        if (v >= 0.0f) price = v * 2.0f * price;     // ×2.0 [0xA3B150]
    }
    if (tier < 1) { /* Ghidra L110452: base<price||tier>3 時に base へ戻すガード（売）*/ }
    if (!isBuy) {
        int purse = getBarterGold(merchant);         // FUN_005FAA70（下記）
        float pf = purse; if (purse < 0) pf += 4.2949673e9f; // [0xA2FC78] 負値ガード
        price = std::min(price, pf);                 // 商人資金上限
    }
    return (int)price;
}

// ==== 商人財布 (FUN_005FAA70 @0x5FAA70, Ghidra L363493) ====
int getBarterGold(Actor& merchant) {
    int purse = (int)merchant.baseNPC.word_0x30;     // v261: ACBS由来
    purse += merchantPlayerEffects(/*FUN_0041E980 @actor+0x44*/);
    if (skillTier(trunc(merchantAVFloat(0x1D))) == 4) purse += kExtraPurseMaster; // +200
    return purse;
}

// ==== ハッグル判定 (FUN_00548130 @0x548130, 呼出 FUN_005E10F0) ====
bool canHaggle(const Actor& self, const Actor& other, int sessionAdjustPercent) {
    if ((float)sessionAdjustPercent == 0.0f) return true;    // p4==0 → 即通る
    double base = ((double)(merchantSkill(other) - merchantSkill(self)) + 100.0) / 10.0;
    double v = std::pow(base, kHaggleCurve)                  // 0x985B70=pow(st0=冪)
             - (double)(kHaggleBase * (float)sessionAdjustPercent)
             + (double)(((self.GetDisposition(other) - 10) / 10 /*int*/) * (double)kHaggleDispMult);
    return v >= 5.23869e-11;                                 // [0xA2FAA8] 実務≥0
}
```

## 差分表（ポート vs 原版）

ポート実測（読取のみ）: `app/src/main/cpp/game/merchant.h` L65 `kProvisionalSellRatio=0.4f`、`merchant.cpp` L53-83、`barter.cpp` L22-28/L49/L114-123。

| 観点 | 原版（確定 VA） | ポート現行 | 乖離 |
|---|---|---|---|
| 売乗率 | `0.01×(5×sqrt(200−max(sk,100)) −10 + 1.5×((disp−50)/10))` = 既定 **0.40+0.015×adj**（0x548240） | `0.4×(1+clamp(charisma/100,0,0.2))`（merchant.cpp L61-62） | 0.4 の数値のみ一致（disp=50・sk任意で一致の偶然）。入力 Actor が逆: 原版=**商人**の disp/skill、ポート=**playerCharisma**( Personality 扱い) |
| 買乗率 | `0.01×(300−99−1.5×adj)` = 既定 **2.01±0.015×adj**（0x5481C0）→ 買値は基準価の ~2倍域 | `1.0×(1−clamp(charisma/100,0,0.2))`（L55-56） | 原版は >1（割高）、ポートは ≤1（常に割引）。倍率レンジ全体不一致 |
| Speechcraft | 価格式直接係数 **なし**（AV32 読取 0 箇所、§未達4） | 使っていない（charisma 代替） | 一致方向だが入力名が誤り（Personality 相当の charisma を原版は読まない） |
| Disposition | 乗率に直接 `(disp−50)/10×1.5×0.01=±0.015/10pt`（0x548190/0x548219） | 不明（playerCharisma に流用） | 係数 0.0015/pt（=1.5% per 10pt）が原版値 |
| 商人財布上限 | 売価は `min(price, GetBarterGold)`、**負 purse は無制限扱い**（0x488E50 L110456-110464） | `merchant.getGold() < earned → 取引拒否`（barter.cpp L120-123） | 原版は**上限クランプ**で取引成立、ポートは拒否。挙動不一致 |
| 端数 | floor（FUN_00484370） | `std::lround` + min1（barter.cpp L25-26） | 丸め方向不一致。原版に min1 なし（0円/負は UI 分岐、価格 0 成立） |
| 回数倍率 | 単価×floor 内（base は stack value、qty は乗算後 round） | `tradeGold(unit, count)` 単価丸め後×count | 原版は unit×qty 後 floor とほぼ同等だが丸め位置が異なる |
| tier 補正 | player Mercantile tier>3 → price=base、tier<1 → stolen×2（0x488E50） | なし | 欠落 |
| haggle 調整 | `sell + adj×0.01 / buy − adj×0.01`（adj=[player+0x11C]、cap 40%=fBarterHaggleMax） | なし | 欠落 |

## [PORT] 修正ドラフト（承認制・コード未編集）

before（merchant.h L65）→ after（原規定数準拠）:
```cpp
// before
static constexpr float kProvisionalSellRatio = 0.4f;
// after（草案・GMST 既定値。merchant.h L64 の「Replace both once RE lands」対応）
// Original FUN_00548240 @0x548240: sell = 0.01*(sqrt(200-max(sk,100))*fBarterSellMult
//                                                + fBarterSellBase + 1.5*((disp-50)/10))
// For merchant sk<=100 (vanilla clamp @0x547B90) this reduces to
// 0.4 + 0.015*((disposition-50)/10) at default GMST fBarterSellBase=-10, fBarterSellMult=5,
// fBarterDispBase=1.5 (slots 0xB375AC / 0xB375BC / 0xB37594).
static constexpr float kSellBaseGMST = -10.0f;   // fBarterSellBase  default @0xA418D8
static constexpr float kSellMultGMST = 5.0f;     // fBarterSellMult  default @0xA31E2C
static constexpr float kDispBaseGMST = 1.5f;     // fBarterDispBase  default @0xA35AA4
```
before（merchant.cpp L59-63）→ after 草案:
```cpp
float Merchant::sellPriceForValue(float baseValue, int merchantDisposition, int merchantSkill) {
    // merchantSkill = trunc(clamp(Luck + Merc*0.4 - 20, 0, 100))  @0x547B90 既定≤100
    const float sqrtTerm = std::sqrt(200.0f - std::max(merchantSkill, 100)) * kSellMultGMST;
    const int   dispAdj  = (merchantDisposition - 50) / 10;   // ゼロ方向切捨て @0x548277
    const float mult     = 0.01f * (sqrtTerm + kSellBaseGMST + dispAdj * kDispBaseGMST);
    return baseValue * mult;                                   // floor は呼出側(barter)で实施
}
float Merchant::buyPriceForValue(float baseValue, int merchantDisposition, int merchantSkill) {
    // FUN_005481C0: 0.01*(300 - 9.9*sqrt(200-max(sk,100)) - 1.5*dispAdj)
    const float sqrtTerm = std::sqrt(200.0f - std::max(merchantSkill, 100)) * 9.9f;
    const int   dispAdj  = (merchantDisposition - 50) / 10;
    return baseValue * (0.01f * (300.0f - sqrtTerm - dispAdj * kDispBaseGMST));
}
```
呼び出し側（barter.cpp）対応案（編集しない・指示のみ）:
1. `sellToMerchant` の入力置換: `playerCharisma` → **商人の disposition と商人 skill**（ポートに商人 NPC の Luck/Merc/Disp がない場合は既定 50/skill100 で 0.4 固定が現行と一致するため段階導入可）。
2. L120-123 の拒否 → **`earned = min(floor(mult×base), purse)` のクランプ**（原版 0x488E50 L110461-110464）。
3. `tradeGold` の `lround` → **`std::floor`**（0x484370）。min1 ガードは原版に存在しないため削除候補（0 円取引は原版でも成立し UI 分岐が「worth nothing」相当を扱う、L306453-306462 [部分確定]）。
4. 将来項目: player Mercantile tier（`0x56A300` 相当）>3 で `price=base`、haggle 調整 `[+0x11C]` ±1%/pt、財布 `+200`(tier4商人・iPerkExtraBarterGoldMaster 0xB375EC)。
5. 既存コメント（merchant.h L63-64「returns kProvisionalSellRatio of it. Replace both once RE lands」）— 本書がその根拠。0.4 は disp=50/sk既定時の**一致値**だが係数構造は別物である旨を差し替える旨のコメント修正案（コード未編集）。

## 証跡（確定）
- `tmp/re/v262_barter_pricing.md`（本書）
- `tmp/re/v262_gmst_register.txt`（0x9EB5E0-0x9EB880 GMST 登録窓）
- `tmp/re/v262_gmst_xrefs.txt`（fBarter* スロット消費者 VA 一覧）
- `tmp/re/v262_548_disasm.txt`（0x548000-0x548B00 生 disasm: 乗率3関数+DispositionMod）
- `tmp/re/v262_5e1000_disasm.txt`（0x5E1000-0x5E1400: ラッパ3+Negotiate補助）
- `tmp/re/v262_floats.txt`（float/double 定数 VA+実測値）
- `%TEMP%\opencode\v262_*.py`（1..29。依頼文は tmp/re 希望だが本セッション規約（スクリプト=TEMP）を優先＝v261 と同例）
- 参考: `docs/Oblivion_decompiled.c`（Ghidra 台帳・読取のみ。L110371-110476/L243874-244226/L325963-326023/L346222-326289/L363493-363519 と手 decompile が一致）
- 仕様外典: 無し（UESP は仮説ヒントにも不使用。全主張 EXE 実測）／OpenTES4・xOBSE・COEF 非参照
