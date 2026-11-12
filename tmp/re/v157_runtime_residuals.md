# V157: opcode 実行時残点束（executor arg8/arg9・Begin ペイロード・%変数バイト列・+0x24 消費者・flagA・R1 arg 再利用・0x50D020 ループ）

- 日付: 2026-10-27（opencode サブエージェント・分析専任／docs 編集なし）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416, ImageBase 0x400000）
- 出典: `tmp/re/v131_impl18_arrival.md` 残点 N-a..N-i ＋ `tmp/re/v132_exception_dispatcher.md` 残点 P-a..P-e ＋ `tmp/re/v134_forwarder_residuals.md` 残点 i
- 前提確認: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時とも実行）
- スコープ: 8 残点の裁定。ESM 再スキャン禁止・git 未実行・docs/TODO/memory 編集なし。

## 結論（8 行）

| # | 残点 | 判定 | 要旨 |
|---|---|---|---|
| 1 | executor arg8≠0 到達不能 tail 経路（0x51717F–0x517293） | **解明** | 「FAE80 で本文デコード → opcode 分岐（0x1000=MessageBox / 0x1059=Message）→ u16 count で %変数式を 0x4FAB20 で逐次評価 → ボタン文字列/time を処理」する**tail 評価モード**。E8 呼出し 2 件とも arg8=0、imm32 参照 0 件 → **v1.2.0416 で完全到達不能（デッドコード）**。設計意図は「メッセージ系コマンドの tail を別パスで評価する機構」（旧来の別呼び出し契約の名残）。 |
| 2 | arg9（[esp+0x780]）: caller1=0 / caller2=1 の意味論 | **解明** | **arg9=0（caller1 0x517330）= 全文実行パス**（Begin ブロック＋通常文を両方処理）。**arg9=1（caller2 0x517640）= イベントディスパッチパス**（opcode≠0x10 は即 return、Begin ブロックのみ処理）。上位 caller: 0x517810（8 引数ラッパー「RunScript」→ caller1）と 0x517950（3 引数ラッパー「FireEvent」→ caller2）。 |
| 3 | Begin ペイロードの後続 u32 と `add [eax],ecx` の加算先 | **解明** | u32 は `[codebase + PC + 2]`（i16 ブロック型の直後）。バイトコード実測: GameMode u32=219、OnActivate u32=275/280/95（**ブロック毎に可変＝行番号またはソースオフセット**）。加算先は `*arg4`（caller1 では saved ebx スロット）。band1 ハンドラの double 結果が **0.0（イベント成立）のときのみ加算**（arg9=0 時）。累積値は ebx レジスタで caller1 の呼び出し元に返る。 |
| 4 | %変数レコードのバイト列仕様（0x4FD7C0 emit ⇔ 0x4FAB20 consume） | **解明** | **バイトレベル一致を確認**。emit（FUN_004FEF80 L197893-197915）: 'G' → `[0x47][u16 id]`（3B）／非 'G' → `[0x72][u16 id][type-char][u16 val]`（6B）。consume（0x4FAB20）: タグバイト読取 → 'G'(0x47) → u16 id 読取（3B）／'Z'(0x5A) → u16 id 読取（3B）／その他 → type-char + u16 val 読取。**両者が対であることがバイトレベルで確定**。 |
| 5 | entry+0x24 バイト（+0x25=1 の 10 件 / +0x24=1 の condition 9 件）の消費者 | **解明** | **+0x25（argflag=256 の下位バイト）**: executor 0x517295 `cmp byte [eax+0x25],0` が唯一の実行時消費者（this+0xA1=1 を設定）。**+0x24（argflag=1 の下位バイト）**: .text の imm32 スキャンで **直接参照 0 件**（帯A/B/C すべて）。v94 アンカー 0x60D90F/0x7BF16A は**両方バイト重なり偽陽性**（0x60D90F=int3 パディング、0x7BF16A=`mov [ebp+esi*4+0xb0],eax` の disp バイト）。+0x24 列は**実行時・コンパイル時の消費者皆無＝デッドカラム**。 |
| 6 | flagA(+0x10 下位)="requires reference" の正式確定 | **解明** | ebx = arg3 = **フェッチの ref 解決結果**（caller1 の [esp_orig+0x24]、ref==0 なら [esp_orig+0x2c] にフォールバック）。executor 0x5169E9 で ebx に格納、0x516AA8/0x5170DA で `test ebx,ebx` — flagA=1 かつ ebx==NULL ならエラー "Null for a required ref pointer." / "Function '%s' requires a reference."。**flagA=1 の命令は「暗黙の this 参照（ホストオブジェクト）」を必要とし、その参照解決結果が ebx に入る**。b03 の裁定（d）「参照必須」に執行時側からの完全裏付け。 |
| 7 | R1 の arg スロット再利用規則の一般性 | **部分解明** | [esp+8]/[esp+0xC] の読取は**正当な引数読取**（arg1=subject / arg2=ref2）であり scratch ではない。GetGold/IsInCombat の [esp+0x1c] パターンは **int→f64 変換用のコンパイラ生成スクラッチ**（`mov [esp+0x1c],eax; fild`）で、**int 戻り値の R1 ヘルパに限定して適用**。float 戻り値の R1 ヘルパ（GetLocked, GetPos 等）にはこのパターンなし。**規則の適用範囲 = 「int 戻り値を持つ R1 ヘルパのうち、fild で double 変換するもの」に限定**。 |
| 8 | 0x50D020 ループカウンタ=Amount の bytecode 実測 | **確定（列論拠）** | ループカウンタ [esp+0x10] は **FAE80 出力スロット A**（argdesc の 2 番目 "Amount" type=1 int）。初期化 0 → FAE80 が書き込み → 0x50D0D0 で `cmp dword [esp+0x10],0`（0 ならループスキップ）→ 0x50D1C0 で `sub dword [esp+0x10],1`（edi=1）で各イテレーションで減算 → 0 でループ終了。**列論拠: argdesc=("Actor Value" type5, "Amount" type1)、FAE80 は arg8=&outB(ActorValue=[esp+0x1C]) / arg9=&outA(Amount=[esp+0x10]) に書き出し**。s57 バイトコード成果物に ModPCSkill(0x110F)/AdvSkill(0x10D4) のサンプルなし（ESM 再スキャン禁止）。負値時は `sub` がアンダーフローし巨大ループになるが、コンパイラが整数検証するため実運用では発生しない。 |

---

## §0 入力・手法・生成物

### 入力（読むだけ・編集なし）

| パス | 用途 |
|---|---|
| `tmp/re/v131_impl18_arrival.md` | 残点 N-a..N-i の定義、executor 経路 |
| `tmp/re/v132_exception_dispatcher.md` | 残点 P-a..P-e の定義、+0x1C 例外 |
| `tmp/re/v134_forwarder_residuals.md` | 残点 i（R1 arg スロット再利用） |
| `tmp/re/b20_impl_5071d0.md` | executor 引数契約、arg8/arg9、tail デッド |
| `tmp/re/b22_getter_types.md` | +0x20 語義、evaluator 型判定 |
| `tmp/re/b03_flaga_meaning.md` | flagA="requires reference" の 7 サイト |
| `tmp/re/cache/opcode_table.json` | 533 行（argflag 列の 19 件確認） |
| `tmp/re/s59_svm_r7_bytecode.txt` | バイトコード実測（Begin ブロック形式） |
| `docs/Oblivion_decompiled.c` | FUN_004fef80 L197875-197915（%変数 emit） |

### 手法

- capstone 5.0.7（skipdata=True / md.detail）。セクション定数: .text raw=VA−0x400C00。
- executor/caller/fetch/handler/evaluator の逆アセンブル、.text 全域の imm32 スキャン、E8 直接呼出しの全数集計。
- バイトコード実測は既存 s57/s59 成果物のみ（ESM 再スキャン禁止）。

### 生成物（すべて %TEMP%\opencode）

スクリプト: `v157_p1.py` / `v157_p2.py` / `v157_p3.py` / `v157_p4.py`
出力: `v157_p1_out.txt` / `v157_p2_out.txt` / `v157_p3_out.txt` / `v157_p4_out.txt`
**repo 内の新規ファイルは本レポート 1 件のみ**。

---

## §1 【残点1】arg8≠0 到達不能 tail 経路 — **解明**

### 1.1 経路の実体（0x51717F–0x517293）

```
0x51717F  test ecx, ecx / je 0x5172d3      ; argdesc==NULL → return
0x517187  mov ecx, [esp+0x20] / cmp [ecx],0 ; [arg4] == 0 → return
0x5171A8  call 0x4fae80                     ; 本文 String を FAE80 でデコード
0x5171AD  mov eax, [esp+0x780]              ; eax = arg9
0x5171B7  sub eax, 0x1000 / je 0x517210    ; ★ 0x1000 MessageBox
0x5171BE  sub eax, 0x59   / jne 0x5172d3    ; ★ 0x1059 Message（他は即 return）
  0x517210 (MessageBox):
    0x517214: u16 count 読取 → 0x517230 ループ: call 0x4fab20（%変数式評価 ×N）
    0x517255: u16 count 読取 → 0x517270 ループ: call 0x4fae80（ボタン文字列）
  0x5171C7 (Message):
    0x5171CB: u16 count 読取 → 0x5171E6 ループ: call 0x4fab20（%変数式評価 ×N）
```

### 1.2 到達不能性の確定

- **E8 直接呼出し = 2 件のみ**: `0x51754D`（caller1、arg8=0）と `0x5177C6`（caller2、arg8=0）。
- **imm32 参照 = 0 件**（.text/.rdata/.data 全セクション走査）。
- ⇒ **v1.2.0416 で完全到達不能（デッドコード）**。

### 1.3 設計意図の推定

この経路は「メッセージ系コマンド（MessageBox/Message）の tail（%変数列＋ボタン文字列/time）を、impl とは別のパスで評価する」機構。arg8=1 が渡される設計は「tail 評価が必要なときだけ有効化する」モード切替とみられるが、リテールでは caller が常に arg8=0 を渡すため未使用。**旧来の別呼び出し契約（tail 評価専用パス）の名残**と裁定。

---

## §2 【残点2】arg9 の意味論と上位 caller — **解明**

### 2.1 arg9 の効果

executor 0x516B55: `cmp byte [esp+0x780],0 / jne 0x5172d3` — **arg9=1 のとき opcode≠0x10（Begin）は即 return**（文を実行しない）。

| arg9 | caller | 意味 | 処理対象 |
|---|---|---|---|
| 0 | 0x517330（caller1） | **全文実行パス** | Begin ブロック＋通常文の両方 |
| 1 | 0x517640（caller2） | **イベントディスパッチパス** | Begin ブロック（opcode=0x10）のみ |

### 2.2 上位 caller（ラッパー関数）

| ラッパー | 引数 | 呼び出す caller | 役割 |
|---|---|---|---|
| `0x517810` | 8 引数（`ret 0x20`） | caller1（arg9=0） | **「RunScript」**: 0xA4 バイトの VM フレームを new → caller1 で全文実行 → フレーム delete → 戻り値を返す |
| `0x517950` | 3 引数（`ret 0xC`） | caller2（arg9=1） | **「FireEvent」**: 同様に VM フレームを new → caller2 で Begin ブロックのみ実行 → フレーム delete → 常に 1 を返す |

- 両ラッパーは E8 直接呼出しで caller1/caller2 を呼ぶ（imm32 参照なし）。
- caller1/caller2 自体の E8 caller は各 2 件（= 両ラッパーの 2 経路）。

### 2.3 caller2 の arg9=1 の必然性

caller2 は「イベント発火時」に呼ばれる。イベントハンドラ（Begin ブロック）だけを実行し、通常文はスキップする。arg9=1 がこの「Begin 専用モード」を切り替えるフラグである。

---

## §3 【残点3】Begin ペイロードの u32 と `add [eax],ecx` — **解明**

### 3.1 Begin ペイロードのバイト列形式（実測）

s59 バイトコード成果物（2393/2393 スクリプト）:

```
op=0010 len=6 arg=0000db000000  ; GameMode:  i16=0x0000, u32=0x000000DB=219
op=0010 len=8 arg=0200130100000000  ; OnActivate: i16=0x0002, u32=0x00000113=275, u16=0x0000
op=0010 len=8 arg=0200180100000000  ; OnActivate: i16=0x0002, u32=0x00000118=280, u16=0x0000
op=0010 len=8 arg=02005f0000000000  ; OnActivate: i16=0x0002, u32=0x0000005F=95,  u16=0x0000
```

- **u32 はブロック毎に可変**（219, 275, 280, 95...）→ **行番号またはソースオフセット**。
- OnActivate は len=8（u16 追加フィールドあり）、GameMode は len=6。
- executor は 6 バイト（i16 + u32）のみ消費、残りは `PC += argLength` でスキップ。

### 3.2 executor 内の処理フロー

```
0x516A23  movsx ecx, word [edi+eax]   ; ecx = i16 ブロック型（イベント idx）
0x516A37  mov eax, [edi+edx]          ; eax = u32（行番号？）
0x516A3A  mov [esp+0x2c], eax         ; u32 を保存
0x516AF3  call ecx                    ; band1 ハンドラ実行
0x516AF8  test al, al / je error      ; ハンドラ失敗 → エラー
0x516AFC  fldz / fcomp [esp+0x18]    ; ハンドラの double 結果を比較
0x516B07  jnp 0x516b17               ; 結果==0.0（イベント成立）→ 加算へ
0x516B09  cmp [esp+0x780], 0         ; arg9
0x516B11  je 0x5172d3                ; arg9==0 → return（加算スキップ）
0x516B17  mov ecx, [esp+0x2c]        ; ecx = u32
0x516B1B  mov eax, [esp+0x20]        ; eax = arg4 ポインタ
0x516B1F  add [eax], ecx             ; ★ *arg4 += u32
```

### 3.3 band1 ハンドラの double 結果の意味

GameMode ハンドラ（0x5022D0）の実測:
```
0x5022D1  call 0x578f60          ; ゲート条件チェック（bool 戻り）
0x5022D6  neg al / sbb eax,eax / add eax,1  ; 条件成立→0, 不成立→1
0x5022E0  fild dword [esp]       ; int→double 変換
0x5022E3  mov eax, [esp+0x20]    ; 出力ポインタ（arg7）
0x5022E7  fstp qword [eax]       ; double を書出し
```

- **結果 0.0 = イベント成立（条件 true）**、1.0 = 不成立。
- `add [eax],ecx` は **イベント成立時のみ実行**（arg9=0 時）。

### 3.4 加算先の同定

- arg4 = pointer to [esp_body+0x10] = **caller1 の saved ebx スロット**。
- saved ebx は caller1 の呼び出し元の ebx 値（プロローグで push）。
- **u32 は ebx スロットに累積**され、caller1 の epilogue で `pop ebx` により呼び出し元に返る。
- ⇒ **Begin ペイロードの u32 は「イベント成立時の累積カウンタ」（行番号またはコスト値）**であり、ebx レジスタで上位に返される。

---

## §4 【残点4】%変数レコードのバイト列仕様 — **解明（バイトレベル一致）**

### 4.1 emit 側（FUN_004FEF80、decompiled L197893-197915）

```c
// 変数ループ（varcount 回）:
if (local_16f4 != 0) {                    // ID がある
    buf[len] = (local_16f0 == 'G') ? 0x47 : 0x72;  // タグ: 'G' or 'r'
    len += 1;
    *(u16*)&buf[len] = local_16f4;         // u16 ID
    len += 2;
}
if (local_16f0 != 'G') {                  // 非グローバル
    buf[len] = local_16f0;                 // type-char（'Z', 'X', その他）
    len += 1;
    *(u16*)&buf[len] = local_16e8;         // u16 値
    len += 2;
}
```

**emit バイト列:**
- 'G'（グローバル）: `[0x47][u16 id]` = **3 バイト**
- 非 'G': `[0x72][u16 id][type-char][u16 val]` = **6 バイト**

### 4.2 consume 側（0x4FAB20、逆アセンブル実測）

```
0x4FAB9E  mov bl, [eax+edi]          ; bl = タグバイト
0x4FABA4  add eax, 1                 ; カーソル++
0x4FABA7  add edx, -0x47             ; edx = タグ - 'G'
0x4FABBC  jmp dword [edx*4+0x4fae3c] ; ジャンプテーブルでディスパッチ

  'G'(0x47) → 0x4FABC3:
    0x4FABC3  movzx edx, word [eax+edi]  ; u16 id 読取
    0x4FABCB  add eax, 2                 ; カーソル += 2（合計 3B 消費）
    0x4FABD5  call 0x4fa9c0              ; グローバル変数を id で解決
    0x4FAC08  fld dword [ecx+0x24]       ; float 値を読出し
    0x4FAC10  fstp qword [edx]           ; double として出力

  'Z'(0x5A) → 0x4FAC26:
    0x4FAC2B  mov eax, [ecx+0xc]         ; スクリプト変数値を読出し
    0x4FAC35  mov [ecx], eax             ; int として出力（合計 3B 消費）

  その他 → 0x4FAC49:
    0x4FAC4B  mov bl, [eax+edi]          ; type-char 読取
    0x4FAC51  cmp bl, 0x58              ; 'X'（式）か？
    0x4FAC56  je 0x4fad37               ; 'X' → 式評価パス
    0x4FAC5C  ...                        ; その他 → u16 val 読取
```

### 4.3 バイトレベル突合

| emit | consume | 一致 |
|---|---|---|
| `[0x47][u16 id]`（3B） | 'G' タグ → u16 id 読取（3B 消費） | ✓ |
| `[0x72][u16 id][type-char][u16 val]`（6B） | 'r' タグ → type-char + u16 val 読取 | ✓ |
| `[0x5A][u16 id]`（emit 側で local_16f0='Z' なら 0x72 タグ） | 'Z' タグ → u16 id 読取（3B） | ✓（注: emit は 'Z' でも 0x72 を書くが、consume は 0x5A でも処理する） |

**判定: 両者が対であることがバイトレベルで確定。v132 P-a は解消。**

---

## §5 【残点5】entry+0x24 バイトの消費者 — **解明**

### 5.1 argflag 列の実態（cache 533 行）

| argflag 値 | +0x24 バイト | +0x25 バイト | 件数 | 対象 |
|---|---|---|---|---|
| 256 (0x100) | 0 | **1** | 10 件 | Activate / PositionWorld / PositionCell / MoveToMarker / ForceFlee / ForceTakeCover / CloseCurrentOblivionGate / ForceCloseOblivionGate / CreateFullActorCopy / DeleteFullActorCopy |
| 1 (0x001) | **1** | 0 | 9 件 | GetIsClass / GetIsRace / GetIsSex / GetInFaction / GetIsID / GetFactionRank / IsGuard / GetIsReference / GetIsPlayableRace |

### 5.2 +0x25 の消費者（argflag=256 の 10 件）

executor 0x517295: `cmp byte [eax+0x25], 0 / je skip` → this+0xA1=1 を設定。
**唯一の実行時消費者**。+0x25=1 の 10 件は「アクション系コマンド」で、実行後に this+0xA1（アクション実行フラグ）が立つ。

### 5.3 +0x24 の消費者（argflag=1 の 9 件）— **消費者皆無**

- **imm32 スキャン**: 帯A(+0x24 基底 0xB0AF6C) / 帯B(0xB0B444) / 帯C(0xB0C8E4) の .text 直接参照は **すべて 0 件**。
- **間接経路**: .data/.rdata の dword スキャンでも該当なし。
- **コンパイラ側**: FUN_004FF710 の [edi+0x24] は emit バッファ構造体（entry ではない）。FUN_004FDAF0 は +0x10/+0x12/+0x14/+0x1C のみ読み、+0x24 は不読。

### 5.4 v94 アンカー 2 件の裁定 — **両方バイト重なり偽陽性**

| アンカー | 真の命令 | 判定 |
|---|---|---|
| 0x60D90F | int3 パディング（0x60D908-0x60D90F = cc×8）。imm32 読取は `cc cc cc cc` = 0xCCCCCCCC | **偽陽性** |
| 0x7BF16A | `0x7BF169 mov dword [ebp+esi*4+0xb0], eax` の disp バイト。imm32 = `84 b5 b0 00` = 0x00B0B584（命令本体内の偶然一致） | **偽陽性** |

⇒ **+0x24 列は実行時・コンパイル時の消費者皆無＝デッドカラム**。9 件の condition getter は +0x24=1 を持つが、どのコードもそれを読まない。バグではなく「将来の拡張用に予約されたフィールド」とみられる。

---

## §6 【残点6】flagA="requires reference" の正式確定 — **解明**

### 6.1 ebx の出所（完全トレース）

```
caller1 0x517522: mov eax, [esp+0x24]    ; eax = フェッチの ref 出力
0x517526: cmp eax, ebx                  ; ref == 0 か？
0x517528: jne 0x517532                  ; 非 0 ならそのまま
0x51752A: mov eax, [esp+0x2c]           ; ref==0 → フォールバック（caller1 の arg2）
0x51752E: mov [esp+0x24], eax           ; フォールバックを保存
0x517548: push eax                      ; arg3 として push
```

executor:
```
0x5169E9: mov ebx, [esp+0x768]          ; ebx = arg3 = ref 解決結果
0x516AA8: test ebx, ebx                 ; band1 経路: ref==NULL？
0x5170DA: test ebx, ebx                 ; コマンド経路: ref==NULL？
```

### 6.2 flagA チェックの 2 箇所

| 場所 | 条件 | エラー |
|---|---|---|
| 0x516AA8（band1 Begin 経路） | flagA=1 かつ ebx==NULL | "Null for a required ref pointer."（0xA4DCC0） |
| 0x5170DA（コマンド実行経路） | flagA=1 かつ ebx==NULL かつ arg8==0 | "Function '%s' requires a reference."（0xA4DB58） |

### 6.3 裁定

**flagA=1 の命令は「暗黙の this 参照（スクリプトの宿主オブジェクト）」を必要とする。** この参照解決結果が ebx（arg3）に入る。ebx==NULL なら参照が解決できなかったことを意味し、エラーが発生する。b03 の裁定（d）「参照必須（reference function / required ref pointer）」に**実行時側からの完全裏付け**。

---

## §7 【残点7】R1 の arg スロット再利用規則 — **部分解明**

### 7.1 R1 evaluator の arg 読取パターン（6 件実測）

| evaluator | arg1 (subject) | arg2 (ref2) | arg3 (param) | arg4 (out) | [esp+0x1C] |
|---|---|---|---|---|---|
| GetDistance 0x4F43C0 | ✓ [esp+4] | ✓ [esp+0xC]（距離計算用） | 不読 | ✓ [esp+0x14] | なし |
| GetLocked 0x4F4760 | ✓ [esp+4] | 不読 | 不読 | ✓ [esp+0x14] | なし |
| GetPos 0x4F4410 | ✓ [esp+4] | 不読 | ✓ [esp+0x14]（座標軸） | ✓ [esp+0x1C] | スクラッチ（z 座標保存） |
| GetActorValue 0x4F6060 | ✓ [esp+8] | 不読 | 不読 | ✓ [esp+0x18] | なし |
| GetGold 0x4F4A00（既知） | ✓ | 不読 | 不読 | ✓ | **int→f64 スクラッチ** |
| IsInCombat 0x4F8F30（既知） | ✓ | 不読 | 不読 | ✓ | **int→f64 スクラッチ** |

### 7.2 規則の適用範囲

- **[esp+8]/[esp+0xC] の読取は正当な引数読取**（arg1=subject / arg2=ref2）であり scratch ではない（v134 §2.3 の裁定を再確認）。
- **[esp+0x1C] スクラッチパターン**（`mov [esp+0x1c],eax; fild [esp+0x1c]`）は **int 戻り値を持つ R1 ヘルパに限定**して適用。float 戻り値のヘルパ（GetLocked, GetPos 等）にはこのパターンなし。
- **本質**: MSVC が int→f64 変換を行うとき、引数スロット近傍のスタック領域を一時的に使用するコンパイラ生成コード。**arg スロットの再利用ではなく、型変換用の一時領域**。

### 7.3 裁定

**規則の適用範囲 = 「int 戻り値を持つ R1 ヘルパのうち、fild で double 変換するもの」に限定。** 全 R1 ヘルパに適用される普遍規則ではない。GetGold/IsInCombat で観測されたパターンは、int 戻り値のヘルパに共通する MSVC の型変換コード生成パターンである。

---

## §8 【残点8】0x50D020 ループカウンタ=Amount — **確定（列論拠）**

### 8.1 ループ構造の逆アセンブル実測

```
0x50D073  mov [esp+0x10], ebp           ; [esp+0x10] = 0（outA 初期化）
0x50D077  lea ebp, [esp+0x10]           ; ebp = &outA
0x50D07B  push ebp                      ; FAE80 arg9 = &outA（Amount 出力）
0x50D07C  lea ebp, [esp+0x20]           ; ebp = &[esp_orig+0x1C]
0x50D080  push ebp                      ; FAE80 arg8 = &outB（ActorValue 出力）
0x50D08C  call 0x4fae80                 ; FAE80（9 引数）
0x50D091  add esp, 0x24
0x50D094  test al, al / je 0x50d3c3    ; 失敗 → return 0
0x50D09C  mov eax, [esp+0x1c]           ; eax = outB = ActorValue
0x50D0A1  push 2 / push eax / call 0x565c50  ; ActorValue を解決
...
0x50D0D0  cmp dword [esp+0x10], 0       ; ★ outA（Amount）== 0 か？
0x50D0E7  je 0x50d1ca                   ; 0 ならループスキップ
...
0x50D0F0: ループ開始（スキル増分＋レベル計上）
...
0x50D176  mov edi, 1                    ; edi = 1
0x50D1C0  sub dword [esp+0x10], edi     ; ★ outA -= 1
0x50D1C4  jne 0x50d0f0                  ; 0 でなければループ継続
```

### 8.2 FAE80 出力スロットの同定

- FAE80 の arg8 = &outB = &[esp_orig+0x1C] = **ActorValue 出力**（argdesc の 1 番目 "Actor Value" type=5）
- FAE80 の arg9 = &outA = &[esp+0x10] = **Amount 出力**（argdesc の 2 番目 "Amount" type=1）
- FAE80 は argdesc の順序に従い、1 番目の引数を arg8 に、2 番目の引数を arg9 に書き出す。

### 8.3 バイトコード実測の可否

s59 バイトコード成果物（2393/2393 スクリプト）に **ModPCSkill(0x110F)/AdvSkill(0x10D4) のサンプルなし**（ESM 再スキャン禁止のため追加採取不可）。opcode ヒストグラムでも 0x110F/0x10D4 は上位 30 件外。

### 8.4 裁定

**ループカウンタ [esp+0x10] は FAE80 出力スロット A = decoded Amount で確定。** 列論拠（argdesc の 2 番目 "Amount" type=1 int → FAE80 arg9 に書き出し）と逆アセンブル実測（初期化 0 → FAE80 書き込み → ループ内で -1 減算）が完全に一致。負値時は `sub` がアンダーフローし巨大ループになるが、コンパイラが整数検証するため実運用では発生しない。FAE80 の 8 引数形(0x500890)と 9 引数形(0x50D020)の出力スロット差: 8 引数形は outA のみ、9 引数形は outA + outB（第 2 出力 = ActorValue）。

---

## §9 自己検証

```
$ python "%TEMP%\opencode\probe_harness.py" selftest
   bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl
   => ALL PASS（4/4、着手時・完了時）
```

| 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|
| arg8≠0 経路の E8 caller | 2 件とも arg8=0 | 0x51754D(arg8=0) / 0x5177C6(arg8=0) | OK |
| arg8≠0 経路の imm32 参照 | 0 | .text/.rdata/.data すべて 0 | OK |
| caller1 の E8 caller | 2 件 | 0x5178FA / 0x51793C | OK |
| caller2 の E8 caller | 2 件 | 0x517A16 / 0x517A3C | OK |
| 上位ラッパーの ret | caller1=8引数 / caller2=3引数 | 0x517810: ret 0x20 / 0x517950: ret 0xC | OK |
| Begin u32 のバイトコード実測 | 可変値 | GameMode=219, OnActivate=275/280/95 | OK |
| band1 ハンドラの double 結果 | 0.0=成立 / 1.0=不成立 | GameMode 0x5022D0 で neg/sbb/add 実測 | OK |
| %変数 emit バイト列 | [0x47][u16 id] / [0x72][u16 id][type][u16] | FUN_004FEF80 L197893-197915 | OK |
| %変数 consume バイト列 | タグ→'G'/'Z'/other ディスパッチ | 0x4FAB20 逆アセンブル | OK |
| +0x24 imm32 参照（帯A/B/C） | 0/0/0 | 0xB0AF6C=0, 0xB0B444=0, 0xB0C8E4=0 | OK |
| 0x60D90F アンカー | int3 パディング | cc×8 を確認 | OK（偽陽性） |
| 0x7BF16A アンカー | disp バイト | `mov [ebp+esi*4+0xb0],eax` の disp | OK（偽陽性） |
| flagA ebx の出所 | フェッチの ref 出力 | [esp+0x24] → arg3 → ebx | OK |
| R1 evaluator の arg 読取 | arg1+arg4 のみ（多数） | GetLocked/GetActorValue で確認 | OK |
| 0x50D020 ループカウンタ | FAE80 outA = Amount | [esp+0x10] 初期化→FAE80→-1 減算 | OK |
| argflag 列の 19 件 | 10 件 +0x25=1 / 9 件 +0x24=1 | cache で 19 件確認 | OK |

---

## §10 新規残点（V143+ 起票粒度）

| ID | 内容 | 試行試行経路/証拠 |
|---|---|---|
| **O-a** | Begin ペイロードの u32（行番号？ソースオフセット？）の正式な意味論。caller1 が ebx で返す累積値の上位 consumer（0x517810 の呼び出し元）が何に使うか未特定。 | §3、0x517810 の E8 caller 未調査 |
| **O-b** | +0x24 列（argflag=1）が消費者皆無である理由。9 件の condition getter が +0x24=1 を持つが、どのコードも読まない。「将来の拡張用予約」か「コンパイラ内部用」か未確定。 | §5.3、imm32 スキャン 0 件 |
| **O-c** | 0x4FAB20 のジャンプテーブル（0x4FAE3C）の全タグ対応表。'G'/'Z'/'X'/'r' 以外のタグ（例: 's' 文字列型等）の consume パス未完全同定。 | §4.2、0x4FABBC の jmp [edx*4+0x4fae3c] |
| **O-d** | R1 evaluator の [esp+0x1C] スクラッチパターンが、int 戻り値の R1 ヘルパ全員に適用されるかの網羅的確認（本調査は 6 件サンプルのみ）。 | §7.1、6 件実測 |
| **O-e** | 0x517810（RunScript ラッパー）の 8 引数の各意味論（特に float 引数 [esp+0x28] と [esp+0x2c] の ref フォールバック）。 | §2.2、0x5178CD の fld dword [esp+0x28] |

---

## §11 使用証拠ファイル一覧

- 読み取り: `tmp/re/v131_impl18_arrival.md`(31,844 B) / `tmp/re/v132_exception_dispatcher.md`(30,701) / `tmp/re/v134_forwarder_residuals.md`(27,131) / `tmp/re/b20_impl_5071d0.md`(41,353) / `tmp/re/b22_getter_types.md`(30,912) / `tmp/re/b03_flaga_meaning.md`(57,256) / `tmp/re/cache/opcode_table.json`(129,379) / `tmp/re/s59_svm_r7_bytecode.txt`(6,901) / `docs/Oblivion_decompiled.c`(L197875-197915) / `%TEMP%\opencode\probe_harness.py`（selftest 4/4）/ `%TEMP%\opencode\func_list_new.txt`
- 本タスク生成（%TEMP%\opencode のみ）: `v157_p1.py` / `v157_p2.py` / `v157_p3.py` / `v157_p4.py` / `v157_p1_out.txt` / `v157_p2_out.txt` / `v157_p3_out.txt` / `v157_p4_out.txt`
- 未使用（禁止事項遵守）: ESM（再スキャン禁止）、`docs/`・`TODO_re49.md`・`session-memory.json`・`plan.md`（いずれも編集なし）、git 操作なし。

---

# §12 追記（V147 二重実行対応・独立追試と訂正、2026-10-27 2nd セッション）

> 本ファイルは先行セッションが作成済み（上書き禁止のため本節を**追記**）。スコープは TODO serial 147 の5点、出典は v131 N-a..N-i / v132 P-a..P-e / v134 a..i のうち帯・引数系2件（= V143-g +0x24 / V143-i a2a3）。
> 本追記は既存記載を独立に probe で再検証し、**6 件の訂正**と**多数の追加証拠**を確定する。既存 §1〜§11 はそのまま維持し、矛盾する点は本節を正とする。

## §12.0 手法・入力（追記分）

- capstone 5.0.7 / probe_harness Ctx（selftest 4/4 PASS、着手時・完了時とも実行）。
- 中間スクリプト（すべて `%TEMP%\opencode`、repo 内新規ファイルは無し）:
  `v147_p1.py`（全 533 行 argflag・列アドレス全数スキャン・executor/caller 呼出し文脈・Begin 実測・R1 89 件スキャン・emit 窓・タグ生テーブル）/ `v147_p2.py`（executor 全体 k 追跡・imm32 間接参照・FAB20 caller・cond 呼出し規約・タグ 2 段テーブル・emit 窓・+0x24/0x25 disp8 全 .text 走査・wrapper 文脈）/ `v147_p3.py`（evaluator の E8 呼出し元と push 並び・0x4471a0 の ret 形・FAB20 整列頭・タグハンドラ本体・executor +0x24 経緯）/ `v147_p4.py`（caller1 の ebx 経路・arg9 ゲート全長・FAB20 fail 経路・wrapper push 再確認）/ `v147_p5.py`（caller1 整列デコードでの ebx 状態確定）— 出力 `v147_p1_out.txt`(42,502B) / `v147_p2_out.txt`(38,724B) / `v147_p3_out.txt`(14,695B) / `v147_p4_out.txt`(11,121B) / `v147_p5_out.txt`(2,156B)。
- 読取のみ: 本ファイル既存部分、`GT_CHEATSHEET.md`、`cache/README.md` + `opcode_table.json` + `call_targets.json`、`TODO_re49.md` serial 147、`v131/v132/v134` 残点定義、`docs/Oblivion_decompiled.c`（該当行のみ）、先行セッションの `v157_p1..p4.py/_out.txt`（読取）、`%TEMP%\opencode\v134_a2a3.py/_out.txt`、`v97_helper_rows.json`。
- 禁止事項遵守: ESM 再スキャンなし / git なし / docs・TODO・台帳・memory・sync 編集なし / 既存 tmp/re/*.md の上書きなし（本ファイルへの追記のみ）。

## §12.1 【訂正1・最重要】tail 経路 0x5171AD の読取は arg9 ではなく **arg2（=opcode）**

v131 N-b / 本ファイル §1.1 の「`0x5171AD mov eax,[esp+0x780]` ; eax = arg9」は **誤り**。

- 0x517199–0x5171A7 で **push 7 件（0x1C バイト）が未解放のまま** 0x5171A8 `call 0x4fae80` → 0x5171AD で読取 → 0x5171B4 `add esp,0x1c`。よって読取時の実効 = frame **[E0+0x08] = arg2**（arg_n = E0+4n）。
- executor 内部の独立証拠: 0x5169FC `mov ecx,[esp+0x764]`（k=0x75C → arg2）→ 0x516A03 `cmp ecx,0x10` → arg2 は **opcode**（0x10=Begin）で確定。
- ⇒ **0x1000/0x1059 セレクタは opcode（MessageBox / Message）**。v132 残点 P-b の「arg9 が 0x1000/0x1059 で比較される謎」は **解消**（スタック調整を考慮しなかった派生の見かけの矛盾）。
- arg9 の実際の消費者は 2 箇所のみ: **0x516B09**（result≠0 時の加算ゲート）と **0x516B55**（`cmp byte [esp+0x780],0 / jne 0x5172d3` — 非 Begin 文を arg9≠0 で即 return）。いずれも k=0x75C で arg9 一致（§12.2）。
- arg8≠0 tail 経路が到達不能という既存結論（§1）は **不変・維持**（下記 §12.2 で経路証拠を強化）。

## §12.2 【確認・強化】executor arg8/arg9 スロット同定と caller 値の経路証明

**フレームモデル（実測）**: executor プロローグ = `push -1 / push 0x9b731c / push eax / sub esp,0x73c / push ebx,ebp,esi,edi / push eax` → k=0x75C。したがって arg8 = [esp+0x77C]、arg9 = [esp+0x780]（k=0x75C 時）— 全 13 箇所の [esp+0x77c]/[esp+0x780] 参照を k 追跡した結果、一致は下表のとおり。

| サイト | k | 実体 | 裁定 |
|---|---|---|---|
| 0x516A51 `cmp byte [esp+0x77c],0` | 0x75C | **arg8** | Begin 経路: arg8==0 → 0x516A9F 通常処理 / arg8≠0 → 0x516A69 FAE80 直行→return（**デッド**） |
| 0x516B09 `cmp byte [esp+0x780],0` | 0x75C | **arg9** | 加算ゲート（§12.3） |
| 0x516B55 `cmp byte [esp+0x780],0` | 0x75C | **arg9** | `jne 0x5172d3`（R2 実測）= arg9≠0 → 非 Begin 即 return |
| 0x516B2E `mov edx,[esp+0x77c]` | 0x760（push 1 件後） | **arg7** | 「Error Executing line (Offset %d)」(0xA4DC88) の引数。**arg8 ではない（訂正2）** |
| 0x517171 / 0x5170E2 `cmp [esp+0x77c]` | 直近窓に push 無し | **arg8** | arg8==0 → 0x517295（impl 実行）/ arg8≠0 → 0x51717F tail（**デッド**） |
| 0x5171AD `mov eax,[esp+0x780]` | 0x75C+0x1C | **arg2 = opcode** | §12.1 訂正1 |

**caller 値（経路証明つき）**:
- **caller1（0x517330、8 引数 `ret 0x20`）**: executor への arg9/arg8 = 0x517536/0x517537 の `push ebx` ×2。ebx は 0x517384 `xor ebx,ebx` で 0 に初期化され、唯一の改変 0x5173E3 `mov ebx,[esi+0x14]` は 0x5173DB–0x51740B の print 経路（0x5173FD `call 0x4a7a60` → 0x51740B `jmp 0x517610` = **caller1 離脱**）に属し、executor 呼出し経路（0x5173D9 `je 0x517410` 側）には**到達しない**。⇒ **arg9=0 / arg8=0 確定**（v157 §2 の 0 を経路レベルで裏付け）。
- **caller2（0x517640、3 引数 `ret 0xC`）**: 0x5177AD `push 1` / 0x5177AF `push 0` の**即値**。⇒ **arg9=1 / arg8=0 確定**。
- 呼出し数（cache + 生 E8 スキャンの両方）: executor→2（0x51754D / 0x5177C6）、caller1→2（0x5178FA / 0x51793C）、caller2→**2（0x517A16 / 0x517A3A）**。
- **訂正3**: caller2 の2番目の E8 site は **0x517A3A**（本ファイル §9 の「0x517A3C」は 1 byte 誤記。0x517A39 `push eax` の直後）。
- imm32 参照（.text/.rdata/.data 全セクション生バイト走査）: executor / caller1 / caller2 = **0 / 0 / 0 件** → 間接・テーブル経由の追加呼出しは存在せず、**arg8≠0 経路は完全到達不能**（§1 維持・強化）。
- 上位 wrapper 呼出し数（新事実）: `0x517810`（RunScript 相当、`ret 0x20`）= **4 件**（0x4F9F20 / 0x4F9F50 / 0x4F9F82 / **0x4FBE6F**=イベント実行 FUN_004fbe00）、`0x517950`（FireEvent 相当、`ret 0xC`・epilogue `mov al,1` を実測）= **1 件**（0x4F9EF1 = 起動系 FUN_004f9ec0）。両 wrapper とも 0xA4 バイトの VM フレームを `push 0xa4 / call 0x401f00`(malloc) で new する点を実測（v157 §2.2 と一致）。

## §12.3 Begin ペイロード u32 と `add [eax],ecx`（確認 + 加算先の訂正4）

- アドレス3 件を整列デコードで再確認: 0x516A23 `movsx ecx,word [edi+eax]`（i16 ブロック型）/ 0x516A27 `cmp ecx,0x1e; ja 0x516acf`（イベント idx≤30 境界）/ 0x516A37 `mov eax,[edi+edx]`（u32）→ 0x516A3A `mov [esp+0x2c],eax` / 0x516B1F `add [eax],ecx`。表引きは 0x516A45（+0x14 argdesc）と 0x516A5D（+0x18 impl = [eax+0xB0AF60]）、flagA 判定は 0x516A9F `cmp byte [eax+0xb0af58],0` + 0x516AA8 `test ebx,ebx`（N-d と一致）。
- **加算ゲートの完全形**（訂正ではなく補足）: 0x516AFC `fldz / fcomp [esp+0x18] / fnstsw / test ah,0x44 / jnp 0x516b17` → **result==0.0 なら無条件に加算**。result≠0 なら 0x516B09 で arg9 を見て、**arg9==0 なら skip、arg9≠0 なら加算**。つまり arg9=1（caller2 = FireEvent 経路・live）では**イベント非成立でも Begin u32 が累積される**。§3.3 の「イベント成立時のみ（arg9=0 時）」は arg9=0 の場合として正しいが、この補足がなければ arg9≠0 経路の挙動が欠落していた。
- double 結果の出所を実測: band1 呼出しは 8 push のうち 0x516AE2 `lea edx,[esp+0x1c]` = arg7 = **&[E+0x18]** → fcomp の [esp+0x18] と完全一致（v157 §3.3 と矛盾なし）。
- **訂正4（加算先）**: arg4 = caller1 の**通常ローカル [E0−0x74C]**（0x517543 `lea edx,[esp+0x30]`、当該時点で k=0x77C）。v157 §3.4 の「arg4 = caller1 の saved ebx スロット」「u32 は ebx レジスタで呼び出し元に返る」は **誤り**（caller1 の saved ebx は [E0−0x75C] にあり、ローカルは `add esp` で解放される）。u32 の上位 consumer は依然未特定（§10 O-a 維持）。
- s59 実測（op=0010 は 5 行のみ）: GameMode u32=219/219（len6）、OnActivate u32=275/280/95（len8）、i16 = 0x0000 / 0x0002。distinct u32 = 4 件 = v157 §3.1 の 4 値と完全一致。

## §12.4 %変数レコード emit ⇔ consume（0x4FD7C0 の帰属訂正 + タグ全表の確定）

**訂正5（TODO 表記の帰属）**: TODO serial 147 の「0x4FD7C0 emit」は**不正確**。
- `FUN_004fd7c0` = **可変長数パーサ**（E8 = 12 箇所: FEF80 内 6、FF710 系 4、0x4FDBC7/0x4FDF10 系 2）。6 引数（cdecl、`add esp,0x24` 実測）で、第2引数 = name バッファ（frame 上 [esp+0x38]、k 実測で **E+0x1C**）。パーサは name[0x200] の**直後**にメタを書き出す: id u32 @ buf+0x200 = [esp+0x21c] / type u8 @ buf+0x204 = [esp+0x220] / val u32 @ buf+0x20C = [esp+0x228]（decompiled の local_18f4[512] → local_16f4/local_16f0/local_16e8 とアドレス級で一致）。
- **バイト列 emit 本体 = `FUN_004fef80`（MessageBoxCompile）0x4FF13E–0x4FF1DB**（v157 §4.1 の帰属が**正**、decompiled L197893–L197915 と対応）。実測:
  ```
  0x4FF13E mov al,[esp+0x220]        ; type-char
  0x4FF145 cmp al,bl / je err        ; type==0 → "Unknown variable"
  0x4FF14D cmp dword [esp+0x21c],ebx ; id
  0x4FF154 jbe 0x4ff1a1              ; id==0 → タグ省略（bare レコードへ）
  0x4FF156 cmp al,0x47 / jne 0x4ff169
  0x4FF160 mov byte [esi+edx+0x20c],al   ; 'G' タグ(0x47)
  0x4FF16F mov byte [esi+eax+0x20c],0x72 ; 'r' タグ
  0x4FF177 add dword [esi+0x40c],edi     ; +1
  0x4FF183/0x4FF18B mov cx,[esp+0x21c] / mov [eax+esi+0x20c],cx  ; u16 id
  0x4FF193 add dword [esi+0x40c],2       ; +2  → 計 3B
  0x4FF1A1 cmp al,0x47 / je 0x4ff1e2     ; 'G' ならここで終了
  0x4FF1A5 cmp dword [esp+0x228],ebx / jbe err  ; val==0 → エラー
  0x4FF1B8 mov byte [esi+edx+0x20c],al   ; type-char
  0x4FF1D3 mov [eax+esi+0x20c],cx        ; u16 val
  0x4FF1DB add dword [esi+0x40c],2       ; 計 6B（bare は 3B）
  ```
- **consume 側のディスパッチは 2 段**（v157 §4.2 の記載を訂正）: 0x4FAB9E `mov bl,[eax+edi]`（tag）→ 0x4FABA4 カーソル+1 → 0x4FABA7 `add edx,-0x47` → **0x4FABAA `cmp edx,0x33 / ja 0x4fad2e`** → **0x4FABB5 `movzx edx,[edx+0x4FAE4C]`（byte リマップ表）** → 0x4FABBC `jmp [edx*4+0x4FAE3C]`（**case** 表。index は tag−0x47 ではなくリマップ後の case）。
- **タグ全表（byte 表 0x4FAE4C + case 表 0x4FAE3C を全実測）**:

| tag | case | ハンドラ | 消費バイト | 意味 |
|---|---|---|---|---|
| 'G'(0x47) | 0 | 0x4FABC3（u16 id + call 0x4FA9C0 → bl=='G' → `fld [ecx+0x24]`） | 3B | グローバル float |
| 'Z'(0x5A) | 0 | 同上 → 0x4FAC26 `mov eax,[ecx+0xc]` | 3B | スクリプト変数 int |
| 'r'(0x72) | 0 | 同上 → bl≠'G'/'Z' → 0x4FAC49（type-char + u16） | 6B | id+type レコード |
| 'X'(0x58) | 3 | 0x4FAD2E → FCA30 式評価パス | 5B | 式 |
| 'f'/'l'/'s'(0x66/6C/73) | 3 | 0x4FADF3 → u16 読取 → call 0x4FA110 → fstp | 3B | リテラル系 |
| 'n'(0x6E) | 1 | 0x4FACE9 `fild dword [eax+edi]` | 5B | i32 定数 |
| 'z'(0x7A) | 2 | 0x4FAD06 `mov [ecx],… ; mov [ecx+4],…` | 9B | double(u64) 定数 |
| 上記以外（H..Y, 等） | 3 | 0x4FAD2E → bl≠'X' → 0x4FADF3 → bl∉{f,l,s} → 0x4FADDE **fail** | — | 不正タグ |

- **バイト列突合の最終裁定**: emit(id≠0, 'G') = `[0x47][u16 id]` ⇔ consume case0 の 3B 読取 ✓ / emit(id≠0, 非G) = `[0x72][u16 id][type][u16 val]` ⇔ consume case0(3B) + type-char + u16(3B) = 6B ✓ / emit(id==0, 非G) = `[type][u16 val]` bare 3B ⇔ 'Z'/'f'/'l'/'s' の 3B ✓。**バイトレベルで完全一致（アドレス級）を確定**。ただし 'X'(5B)/'n'(5B)/'z'(9B) は MessageBox emit に対応レコードがなく、**他コンパイル系の emit 元が未特定**（§12.9 V147-r1）。
- **consume の実行時 caller = 8 件（新事実、v157 未記載）**: executor tail 0x5171FC / 0x517246、**FAE80 内 0x4FB22B / 0x4FB284**（通常文の %変数評価）、FUN_004f3620 0x4F3F3B / 0x4F3F65（式評価、executor からも 0x516F16 で呼出）、FUN_00513280 0x51336A、FUN_00513600 0x5136DD。

## §12.5 +0x24 argflag 列 — 全 533 行・消費者の網羅的再検（V143-g 裁定: デッドカラム確定）

- **全 533 行ヒストグラム（cache 実測）**: argflag = {0: 514, 1: 9, 256: 10} = 非 0 は **19 件**。19 件の名称（Activate, GetIsClass, GetIsRace, GetIsSex, GetInFaction, GetIsID, GetFactionRank, PositionWorld, PositionCell, IsGuard, GetIsReference, MoveToMarker, CloseCurrentOblivionGate, ForceFlee, GetIsPlayableRace, ForceTakeCover, ForceCloseOblivionGate, CreateFullActorCopy, DeleteFullActorCopy）は v157 §5.1 と 19/19 一致。
- **列アドレス全数スキャン（533 アドレス 0xB0AF6C+i×0x28 と +0x25 列）**:
  - .text capstone オペランド（mem disp / imm 含む）= **0 件**（+0x24/+0x25 両方）。
  - .rdata + .data 非整列 imm32 = **0 件**。
  - .text 非整列 imm32 = **2 件のみ = 0x60D90F / 0x7BF16A** — これは v94 アンカー2件と**同値**であり、v157 §5.4 が偽陽性と裁定した 2 件が**列全体で唯一の byte 一致**であることを全数で再確認（int3 パディング / disp バイト）。
- **disp8 相対形の全 .text 走査**（`cmp/test byte [reg+disp8∈{0x24,0x25}], imm`）= **158 件**。内訳: スクリプト系関数での該当は **executor の 3 件のみ**（FF710 / FDAF0 / FDE30 / 56AC50 / 56AE82 / FAE80 / FAB20 はいずれも 0 件）。残り 155 件は Ni/FILE 系（CRT の `test [eax+0x24],0x7f` 等）で別構造体。
  - executor の 2 件（0x516F2E / 0x516FDC）は **`[esi+idx*4+0x24]` = VM オブジェクト自身の dword フラグ配列**（stride 4 ≠ 0x28）。0x516F1E `or dword [esi+ecx*4+0x24],2` が書手、「評価済み bit2」フラグ。**オペコード表 +0x24 列とは無関係**（混同注意の新事実）。
  - 0x517295 `cmp byte [eax+0x25],0` のみが entry+0x25 の実行時消費者（→ 0x51729E `mov byte [esi+0xa1],1`）。
- **裁定**: argflag=1（+0x24=1）9 件は実行時・コンパイル時とも**消費者ゼロ = デッドカラム**（v134 V143-g を**解消**）。argflag=256（+0x25=1）10 件の消費者は executor 0x517295 のみ（v157 §5 維持・全数裏付け）。

## §12.6 R1 arg スロット再利用規則の一般性（V143-i 裁定: v134 結論を維持、機構を精緻化）

- **89 件全数スキャン**: fild 含有 **21** / `mov [esp+X],r` → `fild [esp+X]` ペア **16** / float 系(fstp) **64** / bool 系(fldz/fld1) **4**（Σ=89）。
- **a2/a3 アクセス候補は GetGold / IsInCombat の 2 件のみ**（v134 の走査と同一）。ここで **v134 ツール出力（a2/a3 と分類）のモデル誤りを検出**: 同ツールは「call 直前の全 push を callee が pop（run×4）」と仮定したが、実際は `ret N`（**0x4471a0 = `ret 4` を実測**）。
  - GetGold 0x4F4A18 `mov esi,[esp+0x10]`: 正しい k = 0xC（push ebx/esi/edi/0xf のうち 0x4471a0 が 4B のみ回収）→ **E0+4 = a1 = subject**。⇒ **v134 §2.3 の手動裁定（a1=subject）が正、v134 ツールの a3 分類および v157 §7.1 の「GetGold arg3 読み」はモデル由来の誤り（訂正6）**。
  - IsInCombat 0x4F8F3A `mov esi,[esp+0xc]`: k=8（call なし）→ **E0+4 = a1 = subject** ✓。
  - fild スクラッチ [esp+0x1c]: k を再計算すると **E0+0x10 = arg4 スロット**。「a2/a3 の再利用」ではなく、**arg4(&out) をレジスタ（ebx）へ引き上げた後、MSVC がそのスロットを int→f64 変換の一時領域として再利用**している（GetGold: 0x4F4A03 `mov ebx,[esp+0x14]` → 0x4F4A47 `mov [esp+0x1c],eax` → 0x4F4A4B `fild` → 0x4F4A4F `fstp [ebx]`。IsInCombat: 0x4F8F33 → 0x4F8F81/85/89）。
- **一般規則（最終形）**: R1 ヘルパは **a1（subject）と a4（&out）のみを読み、a2/a3 は 89 件中 0 件が読まない**。スクラッチは「arg4 スロットの read-after-hoist 再利用」で、**int 戻り値 + fild 変換を持つヘルパに限定**（16/89。残る 5 件の int 系は `fild [obj+off]` 直接でスクラッチ不要）。普遍的な MSVC パターンだが適用範囲は上記限定 — v134 V143-i への回答。
- **呼出し側の引数配分（新証拠）**: ① cond 評価本体（0x56ACC7–0x56AD4）: **4 引数 push**（arg1=ebp / arg2=ebx(ref) / arg3=[esi+0x10] / arg4=&out、`add esp,0x10`）。② impl 経由（GetGold impl = 0x5029C0 実測）: `push [esp+0x1c]; push 0; push 0; push [esp+0xc]` = **(impl.arg3=subject, 0, 0, impl.arg7=&out)**。⇒ **どちらの経路でも a2/a3 に意味ある値は要求されない**（impl 経由は 0 を積む）。evaluator 本体の E8 直接呼出しは各 1 件のみ（他は +0x20 列からの間接、imm32 参照は .data の 1 スロット）。

## §12.7 その他の確認事項

- v131 N-a の 0x1E センチネルを caller1 で再観測: 0x51751D `cmp ecx,0x1e / je 0x51757b`（ecx = arg2 = opcode）。executor 側は 0x516A27 `cmp ecx,0x1e / ja`（イベント idx 上限）。**N-a 本体（0x1E の意味論）は本スコープ外のため未裁定**（§12.9 へ）。
- v132 P-e（FF710 の argn/argdesc 不使用）は本タスクで反証なし（触れていない）。
- FAB20 の引数は 8（全 caller `add esp,0x20`）。先頭 push は `push 1`（v132 P-a の CONCAT44(1,…) 記載と整合）。
- executor 内の impl 呼出し（0x516AF3 / 0x5172C6 `call ecx`）は 8 push で、arg3 = ebx（= ref）、arg7 = &double 結果 — b20 の impl 引数契約と整合。

## §12.8 結論（3 行・V147 全体）

1. **5 スコープ点はすべて裁定済み**: arg8≠0 経路=完全到達不能（E8 2 件とも arg8=0・imm32 0 件・経路証明つき）、arg9=0/1 = caller1/caller2（ebx 経路・即値 push で確定）、Begin u32→`add [eax],ecx`=イベント成立時（arg9=0）のみ加算、%変数 emit(0x4FEF80)⇔consume(0x4FAB20) は 3B/6B/5B/9B までバイト級で一致、+0x24 列=デッドカラム（全 533 行・全セクションで消費者 0）。
2. **訂正 6 件**: ①0x5171AD は arg2(opcode) 而非 arg9（v132 P-b 解消）、②0x516B2E は arg7、③caller2 2 site は 0x517A3A、④Begin u32 の加算先は caller1 ローカル [E0−0x74C]（saved ebx 誤り）、⑤「0x4FD7C0 emit」は 0x4FD7C0=パーサ / emit=0x4FEF80、⑥GetGold の a3 読みは a1=subject（v134 ツールの stdcall モデル偽陽性）。
3. **規則の一般化**: R1 ヘルパは a1/a4 のみ読む（a2/a3=0/89）、int 系の [esp+0x1c] は arg4 スロットの read-after-hoist 再利用（16/89 限定）— v134 V143-g・V143-i は**解消**。

## §12.9 残留不明点（追記分・V148+ 起票粒度）

| ID | 内容 | 手掛かり |
|---|---|---|
| **V147-r1** | 'X'(5B 消費)/'n'(5B)/'z'(9B) タグの emit 元（MessageBoxCompile 以外のどのコンパイル系か）と、0x4FA110（'f'/'l'/'s'）・0x4FCA30（'X' type）の型語義 | §12.4 タグ表 |
| **V147-r2** | 0x4FA9C0 の id 解決表（グローバル/スクリプト変数の判別）。bare 'Z' レコードで u16=local_16e8 が id 扱いされる整合（emit 側の id==0 分岐の実運用可否） | §12.4、0x4FABC3 |
| **V147-r3** | Begin u32 の上位 consumer（arg4 = caller1 ローカル [E0−0x74C] を訂正済み。0x517810 の呼び出し元 4 件で誰が読むか） | v157 O-a / §12.3 |
| **V147-r4** | v131 N-a（0x1E センチネルの意味論）・N-f（band1 impl false 時のエラー文挙動）は本タスク未裁定（スコープ外） | §12.7 |
| **V147-r5** | 0x56AC50 経路で arg1（ebp）と arg2（ebx）のどちらが subject か（[esi] bit1 swap 条件の解読）。R1 規約には影響しないが CTDA 全体規約として未確定 | §12.6 |
| **V147-r6** | executor 内 [esi+idx*4+0x24] dword フラグ配列（bit2 設定=0x516F1E）の全体仕様（長さ・リセット手・VM frame 0xA4 構造体内の位置） | §12.5 |

## §12.10 Android 移植への示唆（実装しない）

オペコード実行系の移植では、executor の 9 引数契約（arg2=opcode / arg3=ref / arg4=累積先 / arg8=tail 有効化 / arg9=Begin 専用モード）を固定引数の struct 化して再現するのが最も安全である。tail 経路（MessageBox/Message の %変数 tail 評価）は本リテールで到達不能なので移植対象から外せるが、Begin ペイロード u32 の加算（イベント成立時のみ、FireEvent 経路では無条件）と %変数レコードの 3B/6B/5B/9B バイト列（タグ 'G'/'Z'/'r'/'X'/'f'/'l'/'s'/'n'/'z'）は互換セーブデータ／スクリプト再コンパイルの両方向でバイト一致が必要である。+0x24 argflag 列は一切読まれないので移植では無視してよく、+0x25 のみ（アクション系 10 命令の実行後フラグ）が実際に使われる。R1 条件ヘルパは「subject(arg1)+&out(arg4)」の 2 引数で十分だが、int 戻り値ヘルパは double 結果へ変換するため、出力スロットの型（f64）を厳密に合わせれば互換になる。

## §12.11 自己検証（追記分）

```
$ python "%TEMP%\opencode\probe_harness.py" selftest
   bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl
   => ALL PASS（4/4、着手時・完了時の 2 回）
```

| # | 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|---|
| 1 | argflag ヒストグラム（全 533 行） | {0:514, 1:9, 256:10} | 同一（非 0 = 19 件） | OK（GT §3 と一致） |
| 2 | 非 0 19 件の名称 | v157 §5.1 の 10+9 件 | 19/19 一致 | OK |
| 3 | +0x24/+0x25 列の .text オペランド参照 | 0 | 0 件 | OK |
| 4 | +0x24 列の .rdata/.data 非整列 imm32 | 0 | 0 件 | OK |
| 5 | +0x24 列の .text 非整列 imm32 | v94 アンカー 2 件のみ | 0x60D90F / 0x7BF16A の 2 件 | OK（偽陽性 2/2 一致） |
| 6 | disp8 cmp/test（全 .text）のうちスクリプト系 | executor のみ | 0x516F2E/0x516FDC/0x517295 の 3 件、他関数 0 | OK |
| 7 | E8 → executor / caller1 / caller2 | 2 / 2 / 2 | 0x51754D,0x5177C6 / 0x5178FA,0x51793C / 0x517A16,**0x517A3A** | OK（§9 の 0x517A3C を訂正） |
| 8 | imm32 → executor/caller1/caller2（3 セクション） | 0/0/0 | 0/0/0 | OK |
| 9 | caller1 arg9/arg8 | 0/0（経路証明） | ebx=0 が print 経路（jmp 0x517610 離脱）以外で不変 | OK |
| 10 | caller2 arg9/arg8 | 1/0 | `push 1` / `push 0` 即値 | OK |
| 11 | executor フレーム k → arg8/arg9 disp | 0x77C/0x780 = arg8/arg9 | k=0x75C で一致（3 サイト） | OK |
| 12 | arg9 ゲート指令 | cmp [esp+0x780],0 / jne 0x5172d3 | 0x516B55→0x516B5D jne 0x5172d3 | OK |
| 13 | tail セレクタ | arg2=opcode（訂正1） | push 7 件のオフセット + 0x516A03 cmp ecx,0x10 | OK（訂正1） |
| 14 | Begin i16/u32/add アドレス | 0x516A23/0x516A37/0x516B1F | 3/3 一致 | OK |
| 15 | s59 op=0010 u32 値 | 219/275/280/95 | 5 行・distinct 4 件で一致 | OK |
| 16 | emit 帰属 | 0x4FEF80（v157 正 / TODO 誤） | 0x4FF13E-0x4FF1DB に 0x47/0x72 ストア実測 | OK（訂正5） |
| 17 | 0x4FD7C0 の役割 | パーサ（name+メタ出力） | E8 12 件・6 引数・buf=E+0x1C → [esp+0x21c/220/228] と一致 | OK |
| 18 | consume ディスパッチ | 2 段（byte 表+case 表） | 0x4FAE4C/0x4FAE3C 全 0x34 エントリ実測、有効タグ 9 種 | OK（§4.2 訂正） |
| 19 | FAB20 実行時 caller | — | 8 件（cache の call 0x4FAB20=8 と一致） | OK |
| 20 | R1 89 件の型内訳 | Σ=89 | fild 21 / fstp 64 / bool 4 | OK |
| 21 | R1 の a2/a3 真読手 | 0（v134 経論維持） | 0/89（GetGold の a3 は stdcall モデル偽陽性・実 a1、0x4471a0=`ret 4` 実測） | OK（訂正6） |
| 22 | スクラッチの実体 | arg4 スロット | [esp+0x1c] → E0+0x10 = arg4（ebx ホイスト後） | OK |
| 23 | cond 呼出し引数 | 4 push + add esp,0x10 / impl 経由 4 push | 両経路実測 | OK |
| 24 | wrapper ret / 呼出し数 | ret 0x20 / ret 0xC | 0x51790E / 0x517A28、call 4 件/1 件 | OK |
| 25 | selftest | 4/4 PASS ×2 | 4/4 PASS ×2 | OK |

**内部整合の総括**: 上記 25 検証項目すべて OK。既存 §9 の 15 項目とも矛盾なし（矛盾する §1.1 の注釈・§3.3/3.4・§4.2・§9 の 0x517A3C のみ本節で訂正）。

### 使用証拠ファイル（追記分・すべて %TEMP%\opencode のみ、repo 新規ファイルなし）

`v147_p1.py`..`v147_p5.py` / `v147_p1_out.txt`..`v147_p5_out.txt`。読取のみ: `v157_p1..p4.*`（先行セッション）、`v134_a2a3.*`、`v97_helper_rows.json`、`cache/opcode_table.json`、`cache/call_targets.json`、`docs/Oblivion_decompiled.c`(L197007-, L197796-, L197840-197925)。
