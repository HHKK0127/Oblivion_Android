# V209: `rDebugTextColor` packed 色バイト順の RE 裏取り（2026-10-31）

- 目的: rel1（`tmp/re/relcheck_20261029/rel1_ini_type_agreement.md` §3）で提起された 4 者不整合
  （RE 登録値 / 移植実装 / コード内コメント / テスト期待値）を、**EXE 一次証拠で裁定**する。
- 手法: capstone 5.0.7（skipdata）で `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` を実読。
  スクリプト: `%TEMP%\opencode\v209_pe_node.py` / `v209_scan_refs.py` / `v209_disasm1.py` / `v209_part2.py` / `v209_part3.py`。
- 分析専任（コード/docs 編集なし）。数値・アドレスはすべて実測。

---

## 0. 結論（先出し）

**エンジンの packed 色バイト順 = `0xRRGGBBAA`**（テキスト `"R,G,B"` と双射、alpha は読み取り時に 0xFF 固定）。

| 4 者 | 判定 |
|---|---|
| RE 登録値 `0xFFFBE9FF`（値そのもの） | **正しい**（.data 実読で一致）。ただし v91 の読み順注記 `0xAARRGGBB` は **誤り** → `0xRRGGBBAA`（R=255,G=251,B=233,A=255） |
| 移植実装 `oblivion_ini.cpp:555-557`（`>>16`=R…＝AARRGGBB） | **誤り**。正しくは `>>24`=R, `>>16`=G, `>>8`=B |
| コメント `oblivion_ini.cpp:550`「packed 0xAABBGGRR」 | **誤り**（実装すら AABBGGRR ではない。正しくは `0xRRGGBBAA`） |
| テスト `oblivion_ini_tests.cpp:601-604`（0x00RRGGBB 前提） | **誤り**。ただし現在「実装と誤りが同方向」なのでテストは偶然 green |

移植の既定テーブル文字列 `"255,251,233"` は **エンジン順そのもの**（0xFFFBE9FF → R=255,G=251,B=233）＝ rel1 の「RRGGBBAA 假說」が**確定**した。

---

## 1. EXE 側の実測

### 1.1 登録ノードの実読（.data）

PE ヘッダ実読（image_base 0x400000、.text VA 0x401000/raw 0x400、.rdata 0xA28000/0x627200、.data 0xB02000/0x700C00。タスク記載の file_off 換算則 0x400C00/0x400E00/0x401400 と一致）。

```
[0x00B12DA8] = 0x00000000          ; （前ノード尾/未使用）
[0x00B12DAC] = 0xFFFBE9FF          ; ← value（raw signed = -267777 = v91 CSV と一致）
[0x00B12DB0] = 0x00A68844 → "rDebugTextColor:Menu"   ; name ptr（: 分割形式）
[0x00B12DB4] = 0x00000003          ; 次ノード iDebugTextFont:Menu の value
[0x00B12DB8] = 0x00A6885C → "iDebugTextFont:Menu"
```

- ノードは `+0x00=value / +0x04=name`、**8 バイト間隔で連続**（v91 §1.1 のレイアウトを再確認）。
- 文字列 `"rDebugTextColor:Menu"` は VA 0xA68844（.rdata）に 1 箇所のみ。ファイル内出現も 1 件。
- v91 CSV 行（`0xFFFBE9FF,rDebugTextColor,Menu,0x00B12DAC,... reg=FUN_009fac60@0x009FAC92`）の
  **値・VA・名前はすべて実測一致**。（`copilot_ini_registries.txt` は今回の環境に不在。実体は v91 CSV。）

### 1.2 node 0xB12DAC への参照（.text 全域バイトパターン走査 → 4 件）

| 参照 VA | 命令 | 役割 |
|---|---|---|
| 0x009FAC80 | `push 0xb12dac; mov ecx,0xb07bf0; call 0x4040E0` | **登録**（AddSetting、container=RegSettingCollection。rel1 記載 site と一致） |
| 0x009BDFE0 | `mov ecx,0xb12dac; jmp 0x403BC0` | getter サンク（0x403BC0 = name 先頭 'S' チェック + free 系ヘルパ） |
| 0x00A24280 | `push 0xb12dac; mov ecx,0xb07cfc; call 0x65C620` | 破棄系（リスト除去ヘルパ。v91 §1.3 の vtable[4] 系と同系統） |
| **0x005719F0** | `mov edx,dword ptr [0xb12dac]`（直前 `mov ax,word ptr [0xb12dae]`） | **唯一の実使用（色展開）** ← 本丸 |

### 1.3 色展開経路（ファンクション 0x571900、呼び出し元 0x0057273E）

登録ノードから直接 dword を読み、**4 バイトを順に float 化して色構造体へ**入れる
（生成物は 0xDC バイトのオブジェクト＋フォント参照 `[0xb12db4]`=iDebugTextFont → **デバッグテキスト描画系**と判定）。

```
0x005719EA: mov      ax, word ptr [0xb12dae]    ; ax = 上位 16bit = 0xFFFB
0x005719F0: mov      edx, dword ptr [0xb12dac]  ; edx = 0xFFFBE9FF
0x005719F6: movzx    ecx, ah                    ; ecx = byte3 (bits24-31) = 0xFF
0x005719FB: movzx    edi, dh                    ; edi = byte1 (bits 8-15) = 0xE9
0x00571A0E: movzx    eax, al                    ; eax = byte2 (bits16-23) = 0xFB
0x00571A11: movzx    edx, dl                    ; edx = byte0 (bits 0-7)  = 0xFF
0x00571A0A..0x00571A6C: 各バイトを fild → double 255.0 [0xA3DDD8] で除算
0x00571A86..0x00571AA6: fstp で 4 フロートを連続格納:
        [+0x3c] = byte3/255.0 = 1.000
        [+0x40] = byte2/255.0 = 0.984   (251/255)
        [+0x44] = byte1/255.0 = 0.914   (233/255)
        [+0x48] = 255.0/byte0 = 1.000   （※逆数形。後述 §3）
0x00571AF2: call     0x575870    ; (0,0,0)=位置, 色構造体 ptr, 文字列, … → テキスト生成
```

**抽出順は byte3 → byte2 → byte1 → byte0**（= MSB から順）。最初の 3 フロートが連続する
(r,g,b) スロットに入り、呼び出し先 0x575870 は NiColorA 系として受け取る（Gamebryo の
NiColorA は {r,g,b,a}）。＝ **bits24-31 = R**。

### 1.4 INI 読み込み＝`ReadOne`（vtable+0x10 = 0x4A8800）type 7 分岐

```
0x004A8979..0x004A8999: GetPrivateProfileString（型は 0x4A78D0 の name 判定、'r'→7）
0x004A89BD: push 0xA4052C → "%u,%u,%u"      ; sscanf で 3 整数
0x004A89C8: mov ecx, [esp+0x2c]              ; u1（テキスト 1 番目）
0x004A89CC: movzx edx, byte [esp+0x30]       ; u3（3 番目）
0x004A89D7: movzx eax, byte [esp+0x14]       ; u2（2 番目）
0x004A89DF: shl ecx, 8 ; or eax, ecx
0x004A89E4: shl eax, 8 ; or eax, edx
0x004A89E9: shl eax, 8 ; or eax, 0xff        ; ← alpha = 0xFF 固定
→ dword = u1<<24 | u2<<16 | u3<<8 | 0xFF → node->value へ
```

**テキスト 1 番目の数 → bits24-31**。`"255,251,233"` → `0xFFFBE9FF` ＝ 登録既定値と再現一致。

### 1.5 INI 書き込み＝`WriteOne`（vtable+0x0C = 0x4A85F0）type 7 分岐＋共通展開ヘルパ

```
0x004A86E6..0x004A86FD: call 0x4A83F0（4 ワード展開ヘルパ）
 0x004A83F0: out1 = v >> 24
             out2 = (v >> 16) & 0xFF
             out3 = v >> 8 の ah（= (v>>8)&0xFF）
             out4 = al（= v & 0xFF）
0x004A8718: push 0xA4052C → "%u,%u,%u" ; out1,out2,out3 を sprintf → WritePrivateProfileString
```

読み込み（u1→MSB）と書き込み（MSB→1 番目）が**完全に双射**。type 8('a') は
`"%u,%u,%u,%u"` ＋ pack ヘルパ 0x4A83C0（u1<<24|u2<<16|u3<<8|u4）＝ **0xAARRGGBB 系**
（4 数値）で、type 7('r') だけが 3 数値＋alpha 強制 0xFF。

### 1.5b フォーマット文字列（.rdata 実読）

`0xA4052C="%u,%u,%u"` / `0xA40520="%u,%u,%u,%u"` / `0xA2F860="%d"` / `0xA40538="%.4f"` / `0xA2F7EC=""`（既定値空文字）。

---

## 2. 4 者の判定（エンジン実測に照らして）

エンジンの正: **packed = `0xRRGGBBAA`**（R=bits24-31, G=16-23, B=8-15, A=0-7。A は INI 由来なら常に 0xFF）。

### (a) RE 登録値 `0xFFFBE9FF` の正しい読み順

- **値 `0xFFFBE9FF` は正しい**（.data 0xB12DAC 実読、v91 CSV と一致）。
- **正しい読み順 = `RRGGBBAA`** → (R,G,B,A) = (**255, 251, 233**, 255)。
- これは移植既定テーブル文字列 `"255,251,233"` と**一致**（rel1 の RRGGBBAA 假說が確定）。
- ⚠️ v91_ini_defaults.md:50 の色型注記 **`色 0xAARRGGBB` は誤り**（`RRGGBBAA` に訂正が必要）。
  v91 CSV の `type=color(r)(7)` と値は正しい。誤りは「バイト順の説明テキスト」のみ。

### (b) 移植実装 `oblivion_ini.cpp`（packed 経路）の正しい読み順

現状（`oblivion_ini.cpp:550-557`）:
```cpp
rgb[0] = clamp_byte((packed >> 16) & 0xFF);  // 現: R を byte2(=G 相当) から取る → 誤
rgb[1] = clamp_byte((packed >> 8) & 0xFF);   // 現: G を byte1(=B 相当) から取る → 誤
rgb[2] = clamp_byte(packed & 0xFF);          // 現: B を byte0(=A 相当) から取る → 誤
```
これは `AARRGGBB` 解釈（AA=byte3, RR=byte2, GG=byte1, BB=byte0）。
**エンジン（`RRGGBBAA`）と 1 バイト回転ズレ**。

- `0xFFFBE9FF` に対し現実装 → (251, 233, 255)。
- エンジン真値 → (255, 251, 233)。
- **正しい実装は `>>24`=R, `>>16`=G, `>>8`=B**（alpha `&0xFF` は現状未使用＝正しい）。

### (c) コメント（`oblivion_ini.cpp:550`「packed 0xAABBGGRR」）の正誤

- **誤り**（3 つの中で最も不整合）。
  - 言っていること: `AA`(byte3) `BB`(byte2) `GG`(byte1) `RR`(byte0) → `0xAABBGGRR`。
  - これは (b) の実装とも食い違う（実装は `>>16`=R で `AARRGGBB`）。コメントと実装が互いに不一致。
  - エンジン真値 `RRGGBBAA` とも不一致。**→ `0xRRGGBBAA` に訂正**。

### (d) テスト（`oblivion_ini_tests.cpp:601-604`）の正誤

現テスト: `nPacked=16711680`(0x00FF0000) → r=255, g=0, b=0。これは `AARRGGBB`（AA=0x00,RR=0xFF）とも `AARRGGBB`/現実装と一致する。

- 0x00FF0000 の場合、`RRGGBBAA`(エンジン) でも `AARRGGBB`(現実装) でも (R,G,B)=(0,255,0)... ではない。
  - `RRGGBBAA` で 0x00FF0000 → R=0x00, G=0xFF, B=0x00 → (0,255,0)。
  - `AARRGGBB` で 0x00FF0000 → AA=0x00,RR=0xFF,GG=0x00,BB=0x00 → (255,0,0)。
- テスト期待値 (255,0,0) は **`AARRGGBB`（＝現移植実装）を支持**。エンジン真値とは**不一致**。
- → テストは「現実装と同方向に誤る」ため偶然 green。**エンジン順なら (0,255,0)** が正。

### 補助証拠（type 8 'a' の対比）

type 8('a') は ReadOne で `%u,%u,%u,%u` + pack `u1<<24|u2<<16|u3<<8|u4`(0x4A83C0)、
WriteOne で 4 ワード展開(0x4A83F0)。**type 7('r') は 3 数値＋alpha 0xFF 固定**。
ラベル `r`(RGB) と `a`(RGBA) の使い分けが、`RRGGBBAA`/`AARRGGBB` 解釈と整合し、`AABBGGRR` だけが孤立して不整合。

---

## 3. 一点の注意（消費側 alpha の逆数形）

1.3 の消費側 0x571900 では byte0(alpha) のみ `fdivrp`(def1) で **255.0/byte0 = 逆数** になっている
（他 3 チャンネルは `fdiv st(1)` で byte/255.0）。byte0=0xFF なら 255/255=1.0 なので**結果的に alpha=1.0**。
これはデバッグテキスト描画での alpha 正規化の形で、**byte 順の裁定には影響しない**（byte0 が alpha 桁であることは一貫）。
念のため記録。

---

## 4. 訂正ドラフト

### (A) RE_EXE_REPORT.md（正本）— 色型バイト順記述

**現状: `docs/RE_EXE_REPORT.md` に packed INI 色（AARRGGBB/RRGGBBAA 等）のバイト順記述は無し**
（color 命中 25 件はすべてシェーダ/ Niagara/NiColor 系 float4、INI packed 色とは無関係）。
→ **正本への訂正は不要**（該当記述が存在しない）。ただし v91_ini_defaults.md:50 の訂正は必要（下記 B）。

### (B) v91_ini_defaults.md:50（RE 側記述）— 訂正案

before:
```
| `r` | 7 | 1 | 色 0xAARRGGBB（`rDebugTextColor:Menu`=0xFFFBE9FF） |
```
after:
```
| `r` | 7 | 1 | 色 0xRRGGBBAA（R=bits24-31, G=16-23, B=8-15, A=0-7; `rDebugTextColor:Menu`=0xFFFBE9FF → RGB=(255,251,233), A=255; INI 読込時 alpha=0xFF 固定。裏取り V209） |
```

### (C) 移植実装修正案 `app/src/main/cpp/system/oblivion_ini.cpp`（[PORT]/L2・実装は指示により未実施）

before（`oblivion_ini.cpp:549-558`）:
```cpp
    } else {
        // The compiled-in registry stores rDebugTextColor as a packed 0xAABBGGRR.
        long long packed = 0;
        if (!parse_int(text.c_str(), packed)) {
            return false;
        }
        rgb[0] = clamp_byte((packed >> 16) & 0xFF);
        rgb[1] = clamp_byte((packed >> 8) & 0xFF);
        rgb[2] = clamp_byte(packed & 0xFF);
    }
```
after:
```cpp
    } else {
        // The engine packs registry colours as 0xRRGGBBAA:
        // R=bits24-31, G=16-23, B=8-15, A=0-7 (INI-loaded alpha is forced to 0xFF).
        // ReadOne 0x4A8979 packs "R,G,B" -> (R<<24)|(G<<16)|(B<<8)|0xFF;
        // the debug-text consumer at 0x5719EA unpacks MSB-first (V209).
        long long packed = 0;
        if (!parse_int(text.c_str(), packed)) {
            return false;
        }
        rgb[0] = clamp_byte((packed >> 24) & 0xFF);  // R
        rgb[1] = clamp_byte((packed >> 16) & 0xFF);  // G
        rgb[2] = clamp_byte((packed >> 8) & 0xFF);   // B
    }
```

### (D) テスト修正案 `app/src/main/cpp/tests/oblivion_ini_tests.cpp`（[PORT]/L2）

before（601-604）:
```cpp
        check("a packed integer colour parses", ini.get_color("Menu", "nPacked", r, g, b));
        check_int("packed red", r, 255);
        check_int("packed green", g, 0);
        check_int("packed blue", b, 0);
```
after（`nPacked=16711680`=0x00FF0000 を `RRGGBBAA` で読む → R=0,G=255,B=0）:
```cpp
        check("a packed integer colour parses", ini.get_color("Menu", "nPacked", r, g, b));
        check_int("packed red", r, 0);
        check_int("packed green", g, 255);
        check_int("packed blue", b, 0);
```
補強候補: 0xFFFBE9FF を裸整数で与え (255,251,233) を Assert するとエンジン実測と直結する回帰になる。

---

## 5. v208 との整合（誤解回避メモ）

`v208_gmst_mixed_names.md` は `rDebugTextColor`（'r' 汚染）を **GMST 側の「(c) 登録外」**と分類。
これは GMST 名寄せの汚染候補リストであって、**INI RegSetting 登録の否定ではない**。
本 V209 で INI 登録（node 0xB12DAC, container 0xB07BF0, reg site 0x9FAC92）を実測確認済み。
両者は矛盾しない（`rDebugTextColor` は INI キーであって GMST ではない）。

---

## 6. 新規残点

1. **消費側 0x571900 の呼び出し元 0x0057273E がどのデバッグテキスト描画系かは未命名**
   （0xDC バイト生成物＋`[0xb12db4]`=iDebugTextFont 参照から「デバッグテキスト/フォント描画」と推定、[要命名]）。
2. **type 8('a') 系キーの INI 実例が未収集**（'a' 型は登録ノード 0 件＝v91 §1.1。pack/expand ヘルパはあるが使用箇所が未特定）。
3. **`>>8` の B 桁＝alpha 強制 0xFF が常に成り立つかは ReadOne 経路のみで確認**（bare hex 6 桁経路＝移植の sHexColor 相当は EXE 側に存在しない。移植固有の拡張なので注意）。
4. v91_ini_defaults.md:50 の訂正（§4B）は**未反映**（本タスクは分析専任・docs/RE 成果物編集禁止のため）。

---

## 7. 検証情報

- EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（image_base 0x400000, x86 PE32）
- capstone 5.0.7（skipdata）、.text VA 0x401000/file 0x400、.rdata 0xA28000/0x627200、.data 0xB02000/0x700C00
- selftest: 着手時 4/4（指示どおり変更なし）
- ESM 再スキャン: 未実施（禁止のため）
- 編集禁止事項遵守: git / docs / GT / TODO / session-memory / plan / cache / ソース / 既存 tmp/re は一切変更せず。成果物は v209 新規 1 ファイルのみ。
