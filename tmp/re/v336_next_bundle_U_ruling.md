# v336+ — v335由来 次束U（6残件）判定

## 1. 範囲・制約

- 参照: `tmp/re/v335_v317_stage2_static.md`、`docs/RE_EXE_REPORT.md`、`tmp/re` 既存成果物（主に v289/v306/v308/v310/v312/v315/v317）。
- 対象は v335 §5 の6残件。オリジナル `Oblivion.exe` は読み取りのみ（所在: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`）。
- オリジナルEXE/ESM/.ess検体/解析原本/移植側ソースは変更していない。実`.ess`は従来どおり不在で、動的突合は未実施。
- `0x4C0000-0x4E0000` は #106 専有帯のため、`0x4CBE50` は宣言範囲の浅層照会のみ。関数境界を帯外へ追跡していない。
- 判定語は `[確定]` / `[部分確定]` / `[未確定]` / `[確定負]` を分離する。

## 2. 結論一覧

| 件 | 判定 | 要点 |
|---:|---|---|
| 1 | **[部分確定]** | 0x4CBE50 はセル保存データ後始末を起動する関数だが、正式なC++名は未取得。`[edi+0x24]` bit0はpurge連鎖ゲート、bit3はbit0と同時に立つ場合の継続ゲート、bit8(0x100)は許可浅層で使用箇所なし。個別の公式フラグ名は未確定。 |
| 2 | **[確定]（v335のebx帰属を訂正）** | `ebx` はモードではなく第1スタック引数（form/object ID）。mode 1/0 は第2引数で、`0x459BE1 cmp byte [esp+0xA8],0` が分岐点。mode 0/1 の消費窓を命令列で閉合。 |
| 3 | **[部分確定]** | 7呼び手を4上位headへ束ね、検査系・遅延初期化単体・SEH付き参照列挙系までは同定。各上位関数の正式クラス/メソッド名は帯域制約上未確定。 |
| 4 | **[確定負]** | 0x45ADC0 は `object+0x3C` に `.txt` を付けた what-changed直書き。`Warnings.txt` getter/ロガー経路とは同一経路でない。 |
| 5 | **[部分確定]** | tail==0 の `0x466848 call 0x452DF0` と pass2 の `0x461CD7 call 0x452DF0` は同一callee・force=1。keyの準備位置まで含む全caller同形は、pass2側の既存スタック配置を含めた最終規約表が未取得。 |
| 6 | **[確定]** | `0xB03038` は RTTI文字列 `.?AVTESObjectCELL@@`、すなわち `TESObjectCELL`。 |

集計（主判定）: `[確定]` 2件（2,6） / `[部分確定]` 3件（1,3,5） / `[確定負]` 1件（4） / `[未確定]` 0件。

## 3. 件1 — 0x4CBE50の正式名と`[edi+0x24]`フラグ

### 3.1 観測事実

`v335_1.out` の浅層命令列:

- `0x4CBE69: push 1; push edi; call 0x452570`。
- `0x4CBE71: mov al,[edi+0x24]`。
- `0x4CBE74: test al,1; je 0x4CBE84`。
- `0x4CBE78: test al,8; jne 0x4CBE9C`。
- `0x4CBE7C..81: ret 4`。
- `0x4CBEBF: test byte [edi+0x24],1`、通過時 `0x4CBEC9: call 0x459F30`。
- `0x459F30` は `0x459F4B` の文字列 `PurgeSavedDataForCell() can only be called on interiors, but it was just called on exterior cell %08X.` と、`0x459F67 push 1; push [esi+0xC]; call 0x459AF0` を持つ（v335 §3、v335_2.out）。

### 3.2 裁定

- `[部分確定]` 0x4CBE50の機能上の名前は「セル/フォームの保存データ後始末を必要条件付きで起動する関数」が最も狭い命令記述。正式名を `PurgeSavedDataForCell` とするのは不適切で、その文字列は **0x459F30側**のエラー文に現れる。正式なEXE内シンボル/RTTI/enumは得られていない。
- `[部分確定]` bit0 (`0x01`) は、`0x459F30→0x459AF0` purge連鎖へ入るためのゲート。bit0が0なら `0x4CBE84` 側へ進み、bit0が1でもbit3が0なら早期returnする。
- `[部分確定]` bit3 (`0x08`) は、bit0が1の状態で `0x4CBE9C` 以降へ継続させるゲート。bit3単独の公式名称や、bit0/bit3の極性（interiorそのものか、変更/寿命状態か）は、この帯を越えない条件では決められない。
- `[確定負]` bit8 (`0x100`, 依頼表記の `0x108` の高位側) は、v335の宣言浅層窓および既存v317/v319の関連照会で読み出し・test・or/andの命令証拠がない。したがってこの関数のbit8意味を名付ける根拠はない。

### 3.3 既存成果物との照合

- v335は「4CBE50が459F30を通じてPurgeSavedDataForCell連鎖を起動」を確定している。本束はそのまま採用する。
- v289/v312/v315のchangeFlags bit3/bit31議論は **別のChangeFlags台帳dword**であり、ここで観測した`[edi+0x24]` bit3を同一名とみなしてはならない。構造体フィールドと文脈が異なるため、同一意味説は未支持。

## 4. 件2 — 0x459AF0のmode 1/0消費マップ

### 4.1 重要な訂正: `ebx`はmodeではない

呼出側は次の順でpushする。

- `0x459F62: push 1; 0x459F64: push esi; 0x459F67: call 0x459AF0`。
- thunk `0x459F88: mov eax,[eax+0xC]; 0x459F8B: push 0; 0x459F8D: push eax; 0x459F8E: call 0x459AF0`。

x86のcallee側で `sub esp,0x90; push ebx` 後の `0x459AF7 mov ebx,[esp+0x98]` は、retaddr直後の**第1引数**に対応する。従って `ebx` は purge対象のform/object IDであり、v335 §1/S2の「arg2→ebx」はスタックオフセットの取り違えである。第2引数（mode 1/0）は、保存された引数位置である `[esp+0xA8]`（`0x459BE1`時点）から読む。

### 4.2 完全な分岐・消費表

| mode | 命令 | type/条件 | 消費 | 後続 |
|---:|---|---|---|---|
| 0 | `0x459BE1 cmp byte [esp+0xA8],0` → zero側 | `type='0'`（`0x459BF0`、`0x459D03`未満）かつ version `<0x5B` | flags `al`のbit1・bit2が両方立つ時だけ12B（`0x459C12-27`、cursor `+0xC`）、それ以外はu16 2B（`0x459C5D-7B`） | 12Bは`0x459950`、2Bは`0x459990`。一致しなければ`0x452DF0(key,1)`（`0x459C41`）。 |
| 0 | 同上 | version `>=0x5B` かつ `al & 0x04000000` | u32 4B（`0x459C82-9D`） | `0x459990`。不一致時`0x452DF0(key,1)`（`0x459CB4`）。 |
| 0 | 同上 | version `>=0x5B`、上記で不該当かつ `al & 0x02000000` | u32+u16 6B（`0x459CBE-E6`） | `0x459990`。不一致時`0x452DF0(key,1)`（`0x459CFC`）。 |
| 1 | `0x459BE1` nonzero → `0x459D07` | `type='1'/'2'/'3'` 以外 | 0B | `0x459EA5`へスキップ。 |
| 1 | `0x459D1A` | `al & 2` | 36B（`0x459D29`、cursor `+0x24`） | 共通のID解決・一致確認へ。 |
| 1 | `0x459D45` | `al & 0xC`、`al`が負（bit31） | 44B（`0x459D56`、cursor `+0x2C`） | 共通のID解決・一致確認へ。 |
| 1 | `0x459D45` | `al & 0xC`、`al`が非負 | 28B（`0x459D75`、cursor `+0x1C`） | 共通のID解決・一致確認へ。 |
| 1 | `0x459D91` | `al & 0x00800000` | u32 4B（`0x459D98-A6`） | 共通のID解決・一致確認へ。 |

mode 1の共通後段は、`0x459DA9`以降で参照IDを`0x446B80`（created判定）または`[SaveGame+0x74]`配列へ解決し、`cmp esi,ebx`（`0x459DCD`）で対象ID一致を確認する。一致しない場合は`0x459EA5`へ進み、該当レコードの`0x452DF0(key,1)`除去には入らない。mode 0も各形態の`0x459950/0x459990`戻り値を`cmp eax,ebx`で判定し、不一致時のみforce除去する。

### 4.3 既存成果物との照合

- v317 §4-6の「459AF0 arg1=objID、arg2=1」「type/flags別の12/2/4/6B読取」は正しい。
- v335 S2の「0x459AF7がarg2→ebx」は誤り。本束では、命令長・push順・callee stack位置を再計算し、`ebx=arg1`、`[esp+0xA8]=arg2(mode)`へ訂正する。
- v289/v297のpass2 12B・bit3翻訳・bit31/bit23の記述は、このmode 0/1分岐とは別の保存pass2経路であり、直接混同しない。

## 5. 件3 — 0x4CBE50呼び手7件の上位同定

| call site | 上位head | 浅層同定 | 判定 |
|---:|---:|---|---|
| `0x4B8993` | `0x4B86C0` | SEH付き。`0x4D6670`を呼び、戻り値を検査している。セル/参照検査系。 | [部分確定] |
| `0x4CC045` | `0x4CBE50` | 同一関数内部の自己再入/後続呼出し。 | [確定] |
| `0x4F26FE`,`0x4F2716` | `0x4F2270` | `[0xB360C0..0xB360C8]`の遅延初期化フラグ/カウンタを操作し、グローバル単体を初期化する枠。対象単体の正式クラス名は未取得。 | [部分確定] |
| `0x515E7E` | `0x515D20` | SEH付きの引数列挙/参照列挙枠。24命令浅層では実処理名まで不可。 | [部分確定] |
| `0x51607A`,`0x51625A` | `0x515EF0` | 同一SEH/参照列挙系の別call site。上位head自体は確定、正式メソッド名は未確定。 | [部分確定] |

根拠はv335_2.out §E2およびv335_3.out §F1。#106専有帯のため、`0x4F2270`や`0x515xxx`の関数境界・callee深追いは実施しない。したがって「7件すべての上位C++クラス/正式メソッド名」は未確定のまま残す。

## 6. 件4 — 0x45ADC0のfopen/CreateFile系モードとWarnings.txt経路

### 6.1 0x45ADC0の実出力

`0x45ADC0`は実際にはfopen文字列ではなく、IAT経由のWin32ファイルAPIを使う。

- `0x45ADED: lstrcpyA`（IAT `0xA28164`）で`arg0`（SaveGame+0x40ノードの`+0x3C`パス）を作業バッファへコピー。
- `0x45ADF3: push 0xA3A4D4`、文字列は`.txt`。
- `0x45AE00: lstrcatA`（IAT `0xA28160`）で`.txt`を付加。
- `0x45AE4E: CreateFileA(path, 0xC0000000, 0, NULL, 4, 0x80, NULL)`。すなわち `GENERIC_READ|GENERIC_WRITE`, `OPEN_EXISTING`, `FILE_ATTRIBUTE_NORMAL`。
- 以後`WriteFile`でヘッダ/変更行/フッタを書き、`0x45B2E0 CloseHandle`。

従って「fopenのmode文字列」は存在しない。Cランタイムの`"w"`/`"a"`等を示す証拠はない。[確定]

### 6.2 Warnings.txtとの比較

- `0xA3D9BC="Warnings.txt"`の唯一の.text参照は`0x4944BC`、関数`0x494480`のpath getter。`<base>\\Warnings.txt`を`0xB34C80`へ構築するだけ（v306 §2.1）。
- `0x45ADC0`内に`0x4941A0/0x4A7A60/0x57B8E0/0x494480`の呼出しはない。自前`CreateFileA/WriteFile/CloseHandle`である。
- v306/v308/v310で、Warnings getter側は削除・サイズ確認・pathを引数にしたログまでで、Warnings書出しの到達は負証拠化済み。ch1は`0x4943A0` no-op。
- `object+0x3C`はSaveGameのセーブパス欄で、通常は`<base>\Saves\<name>.ess`系。`+".txt"`はWarnings getterの固定パスとは別物。

よって「同一経路か」は **[確定負]**。`object+0x3C`の実行時slot値を実検体で照合する余地は残るが、静的な呼出経路は既に分離している。

## 7. 件5 — loader tail==0の452DF0とpass2 bit3削除の呼出規約

### 7.1 共通点（[確定]）

- loader tail==0: `0x466830`で`call 0x452C90`後、`0x466842 mov ecx,[esp+0x1C]`、`0x466846 push 1`、`0x466848 push ecx`、`0x46684C call 0x452DF0`（v335_3.out §F3）。
- pass2 bit3単独: `0x461CC5 cmp ebx,8`、`0x461CD4 push 1`、`0x461CD7 call 0x452DF0`（v315/v317）。
- `0x452DF0`の全callerはforce引数`1`固定で、refcountが残っても除去するforce-remove関数（v289 §1-2）。同一callee、同じforce値、同じ「ChangeFlags entryを削除」契約は確定。

### 7.2 残る規約差

- loader側はkeyを明示的に`push ecx`してからforce=1をpushするため、論理的には`452DF0(this, key, force=1)`。
- pass2側は既存のスタックフレームにkey相当が既に配置され、force=1だけを追加してcallする形。`0x461CD4`付近の「keyの事前配置」を全push収支込みで再取得していないため、key slotの物理オフセットをloader側と逐字一致までは断定しない。
- ただし「別関数/別force値/別callee」という説は **[確定負]**。v289の14 caller一覧、v315/v317のpass2命令列、v335のtail==0窓が一致する。

総合判定は **[部分確定]**（calleeとforce規約は確定、全stack slotの逐字同一性のみ未確定）。

## 8. 件6 — 0xB03038 TD

`0x45356E-0x45357D`のRTDynamicCast列:

- `push 0xB03038`
- `push 0xB02F9C`（`.?AVTESForm@@`）
- `call 0x9832E6`

既存v315 §4-5およびv335_2.out §E5で、`0xB03038`のRTTI文字列は `.?AVTESObjectCELL@@` と確定している。従ってクラス名は **`TESObjectCELL` [確定]**。同じcreated判定列の`0xB03054`は`.?AVTESObjectREFR@@`であり、二つを混同しない。

## 9. 既存成果物・docsとの照合と訂正

1. **v335 S2の訂正**: `0x459AF7 mov ebx,[esp+0x98]`をarg2読取とした記述は、callee prologueの`push ebx`とcaller push順を合わせると第1引数（form/object ID）。mode 1/0は第2引数で`0x459BE1 [esp+0xA8]`が実際の分岐点。本束ではmode消費マップを再構成した。
2. **v335 S1の名称留保を維持**: `PurgeSavedDataForCell`は0x459F30内の文字列であり、0x4CBE50の正式名ではない。0x4CBE50に公式名を転記しない。
3. **v306/v308/v310のWarnings負証拠を採用**: v301の「Warnings sink接続確定」は後続成果物で撤回済み。0x45ADC0はセーブパス`.txt`直書きで、固定Warnings getterとは別経路。
4. `docs/RE_EXE_REPORT.md`はメニューXML中心で、今回のSaveGame/ChangeFlags残件の正式記載を含まない。今回の6件についてdocsとの直接矛盾は確認しない。

## 10. 未解決点

- 0x4CBE50のEXE内正式シンボル/クラス名、`[edi+0x24]` bit0/bit3の公式 enum 名とbit8の所有者。
- mode 1/0の「引数の意味名」は確定したが、呼出API側のC++宣言（第1引数をformID、第二引数をpurge/参照列挙モードとする正式宣言）。
- 0x4B86C0/0x4F2270/0x515D20/0x515EF0の正式クラス名・上位caller名（#106専有帯解除後の深解析が必要）。
- pass2 `0x461CD7`のkey物理slotをloader `0x46684C`と逐字比較するには、既存帯域制約内での追加stack収支プローブが必要。
- 実`.ess`で`object+0x3C`の実値を確認すること（ただし静的経路がWarnings固定getterと別である結論は変わらない）。

## 11. 証拠・再現メモ

- v335中間物: `%TEMP%\opencode\v335_1.out`（4CBE50浅層、459AF0、45ADC0）、`v335_2.out`（caller/459F30）、`v335_3.out`（上位head、tail==0、RTTI）。
- 追加確認: 既存EXEを読み取り、`0x459B90-0x459E07`をCapstoneで再ダンプ。出力の中心は`0x459BE1` mode test、`0x459C0A-0x459CFD` mode0 payload分岐、`0x459D07-0x459DB0` mode1 payload分岐。
- 既存成果物照合: `v317_v315_residuals.md` §4-6/§4-7/§4-8、`v289_extra_space_bundle.md` §1-2、`v306_warnings_path_evidence.md` §1-2、`v308_warnings_residuals.md`、`v310_warnings_flush_residuals.md`。
- 自検証: 既存成果物は上書きせず、本ファイルのみ新規作成。検体・ソース・ESM・.ess・解析原本は無変更。
