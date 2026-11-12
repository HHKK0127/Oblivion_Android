# V178: opcode 実行時残点 第3束（7件）— FireEvent 本文実行主体 / %変数タグ 'X'・'n'・'z' emit と 0x4FCA30 / 0x45A500 述語 / type0x3D の共有理由 / Process slot+0x190 / Begin u32 上位 consumer / 's' consume 詳細

- 日付: 2026-10-28（opencode サブエージェント・分析専任／docs・TODO 編集なし）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416, ImageBase 0x400000）
- 出典: `tmp/re/v172_runtime_residuals2.md` §新規残点 V-172-r1..r5 ＋ `tmp/re/v157_runtime_residuals.md` §10 O-a/O-c 残り（計 7 件）
- 前提確認: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時）
- 手法: capstone 5.0.7（`v172_common.py` 経由: dump / e8_callers / imm32_refs3 / raw バイトスキャン / PE import パース / RTTI COL 解析）。中間スクリプトは `%TEMP%\opencode\v178_p1..p12.py`、出力 `v178_p*_out.txt`。**repo 内新規ファイルは本レポート 1 件のみ**。
- 前セッション引き継ぎ: `v178_p1..p3.py/_out.txt`（caller1/caller2/executor/band1/opcode 表）は先行中断セッションの成果を再利用、p4 はバグ（list 未変換）修正後に再実行。
- 制約遵守: git / docs / TODO / session-memory / plan / func_list_new / cache / 既存 `tmp/re/*.md` は**無編集**、ESM 未アクセス。

## 結論サマリ（7 行）

| # | 残点 | 判定 | 要旨 |
|---|---|---|---|
| 1 | V-172-r1 FireEvent（arg9=1）時の Begin ブロック本文の実行主体 | **解明** | **本文の実行主体は caller1（0x517330）の通常実行ループそのもの**。band1 ハンドラの double 結果が **≠0.0（イベント該当）だと u32 加算＝スキップが行われず**、PC はヘッダ長（6/8B）だけ進み、本文の各文が**通常文として inline 実行**される。**FireEvent（caller2/arg9=1）は本文を 1 文も実行しない**（Begin は成立・不成立とも無条件 u32 加算で本文 skip、非 Begin 文は即 return で実行せず）。FireEvent の役割は band1 ゲートの副作用消費（例: OnActivate のアクティブ化フラグ処理）のみ。v157 の「結果 0.0=成立」ラベルは**逆**（0.0=ブロック不通過）だった点を訂正。 |
| 2 | V-172-r2 'X'/'n'/'z' の emit 元と 0x4FCA30 の型語義 | **解明** | **'X' emit = FUN_004FDAF0**（0x4FDC30: `[0x58][u16 opcode-id]` + 行引き + [行+0x1C]=0x4FDE30 へ引数の再コンパイルを**インライン再帰**）。**'n'/'z' emit = FUN_004FDE30**（opcode 表の dispatcher 列＝汎用コマンド引数コンパイラ、0x4FE9D2/0x4FE972: %-トークンが数値リテラルの場合、**'n'=整数型引数（type 1/0x17）かつ 0x47D550 整数判定 → atol → i32 inline（5B）／'z'=0x47D5B0 浮動小数判定 → atof → double inline（9B）**）＋式コンパイラ帯 0x4F3Exx も同一判定で emit。**0x4FCA30 = opcode/関数 ID → オペコード表行ポインタの静的 getter**（id 0x100-0x182→帯B 0xB0B420 起点 / 0x1000-0x1170→帯C 0xB0C8C0 起点、範囲外は NULL）。'X' consume はこの行の argdesc(+0x14)/impl(+0x18)/dispatcher(+0x1C) を使って式評価。 |
| 3 | V-172-r3 0x45A500([0xB33B00]) 述語の語義 | **解明** | **スレッド文脈述語**: `GetCurrentThreadId()`（IAT 0xA2808C）と `[[0xB33398]+0x10]`（**メインスレッド ID のキャッシュ**）を比較し、等しければ `[this+0x18] & 1`（bit0＝メインスレッド文脈）、異なれば `([this+0x18] >> 0x12) & 1`（bit18＝他スレッド文脈）を返す。`[0xB33B00]` は **ctor 0x45B300**（単一呼び出し元 0x441BB0＝ゲーム初期化）が構築する**非仮想・約 0x1C4B のグローバル状態シングルトン**（teardown 0x4466F0 が NULL 書き、他に書手なし）。+0/+4 = formID キーの 2 マップ（0x55E000 lookup）、+0x7C = formType byte（0x45A170: `[0x20,0x5A)` 判定）、+0x80 = リスト先頭。Update skip ゲート第 3 項＝「この参照の更新を止めてもよい文脈フラグ」。 |
| 4 | V-172-r4 type0x3D float extra を crime gold と馬盗難が共有する理由 | **解明** | type0x3D は **ref の ExtraDataList に付く「犯罪/盗難値アキュムレータ」ノード**（ノード +0xC = f32）で、**唯一の加算書手 0x4269E0**（thiscall ret4: 0x41E210 で get-or-create → 無ければ 0x42A290 で新規+0x41E0A0 attach、あれば `[node+0xC] += delta`、合計 ≤0 で 0x422E20 により削除）。**犯罪金**: プレイヤ ref（[0xB333C4]+0x44、stats 帯 0x5DA930 が読手）＋**被害者 ref**（OnAlarmVictim band1 0x50B3E0 が [player+0x116]/[player+0x700] バーグラー経路で加算、0x60FD20 が `fild [0xB376B0]` で減算＋[player+0x6DC] カウント）。**馬盗難**: 馬 ref +0x44 の同ノード > **0.0f（[0xA2FAA8] 実測 0.0）** なら stolen（デバッグ文 "%s is stolen"/"%s is not stolen" 実測）。**1 つの extra タイプを共有するのは「盗難馬＝馬 ref に犯罪値マーカを付ける」という同一機構の再用**で、IsHorseStolen はマーカ >0 の判定にすぎない。 |
| 5 | V-172-r5 Process 系 slot+0x190 のシグネチャ差 | **解明** | **vtable の同一バイトオフセットに別クラスの別 API が並ぶだけ**（slot は階層独立）。RTTI 実測: Process 系は **BaseProcess（vt 0xA6F464）の +0x190 = 0x60D880**（`ret 4`＝引数の NiRefObject を InterlockedDecrement([0xA2807C]) で参照解放する **ハンドル解放 API**）、HighProcess（vt 0xA71814）/MiddleHighProcess は **0x634910 に override**（[this+0x118] の Ni ハンドルを差し替えながら同プロトコルで解放）。Actor 家族（vt 0xA6E074）の +0x190 = 0x977C50（ゲート、+0x188 も同スタブ、近傍は 0x5Exxxx の Actor 系メソッド）。0x4CCC50 の vcall +0x190 は **REFR/Actor オブジェクトにのみ**向けられ、Process オブジェクトの +0x190（解放 API）がゲートとして呼ばれることは**存在しない**。「シグネチャ差」＝slot 割当の偶然一致であり共有契約ではない。 |
| 6 | V-157 O-a Begin u32 累積値の上位 consumer（0x517810 呼出し元） | **解明（不存在を独立再検証）** | 累積先は caller1 **ローカル [E1+0x1C]**（毎イテレーション 0 にリセット、0x5174EF）で外部へ渡らない。caller1 戻り値は **al（[esi+0xA1] アクション実行フラグ）のみ**（0x5175F0-0x51760C）。wrapper 0x517810 は `mov bl,al → frame free → mov al,bl` の bool 透過。**呼出し元 4 件すべて al のみ消費**: 0x4F9F20/0x4F9F50/0x4F9F82 は call 直後に `ret 8/0xC`（al 透過）、0x4FBE6F は `mov bl,al` → 自関数 0x4FBE00 の戻り値へ。横断的な値保存は [script+0x34] の実行時間累積（QueryPerformanceCounter 0xA28158/5C）のみで u32 系は無し。**Begin u32 に上位 consumer は存在しない**（v172 経路の上での独立再確認）。 |
| 7 | V-157 O-c 's'（文字列型）consume パス詳細 | **解明** | 'f'/'l'/'s' は case3 **0x4FADF3**: i16 id 読取（カーソル +2、計 3B）→ bl∈{'f','l','s'} 検証 → **0x4FA110(ecx=呼び出し元コンテキストの owner/script, id=sx(i16), ctx)** → `fstp qword [out]`（double）。0x4FA110 本体: メモ化（[0xB361BC]=owner / [0xB09E20]=id / [0xB361C0]=record）→ `[owner+0x0C]` リンクリスト走査（node={+0 entry, +4 next}、entry={+0 id, +8 f64 値}）→ 発見で `fld qword [entry+8]` ret 8、未発見で 0xA49A68 "Variable ID %08X not found…"＋0.0。**'f' でも 's' でも runtime は完全に同一解決器**（タグ差は emit 側のみ: FUN_004FD5F0 L196975 / FUN_004FAA90 L194383 の `type = 'f' + (record[+0x10]!=0 ? 0x0D : 0)`）。'l' はリテールの emit 元が存在しない予備タグ（consume 側のみ受付）。 |

---

## §1 【残点1】FireEvent モードで Begin ブロック本文を処理する主体 — **解明**

### 1.1 決定的な新事実: band1 ハンドラの double 結果の極性は「0.0=ブロック不通過（skip）／≠0.0= fall-through（本文実行）」

executor（0x5169A0）Begin 経路（再実測: v178_p1_out.txt）:

```
0x516AF3  call ecx                     ; band1 ハンドラ実行（arg7 = &double 結果）
0x516AF8  test al,al / je 0x516b26     ; ハンドラ失敗 → エラー
0x516AFC  fldz / fcomp [esp+0x18]      ; 結果 == 0.0 か
0x516B07  jnp 0x516b17                 ; ★ 0.0 → u32 加算（本文 skip）
0x516B09  cmp byte [esp+0x780],0       ; arg9
0x516B11  je 0x5172d3                  ; arg9==0 かつ ≠0.0 → 加算せず return → 本文へ fall-through
0x516B17  add [eax], ecx               ; arg9==1 ならここに常に到達（成立・不成立とも skip）
```

極性の実証（OnActivate band1 = 0x503F90、リテール実測）:

```
0x503FA8  fstp qword [edi]             ; *out = 0.0（既定 = 本文 skip）
0x503FCA  call 0x4fae80                ; イベント引数のデコード
0x503FDD  push 2 / mov ecx,esi / call 0x4d8260   ; ref の extra(+0x44) を 0x41F830 で走査＝アクティブ化保留確認
0x503FE6  test al,al / je 0x503fee
0x503FEA  fld1 / fstp qword [edi]      ; ★ 保留あり → *out = 1.0 → caller1 は本文を実行
0x503FEE  push 1/2 → call 0x4d82e0     ; 保留フラグの消費（FireEvent 時の副作用）
```

GameMode band1（0x5022D0）も同型: `call 0x578f60`（[0xB3A6E0] シングルトンの [obj+8]==1 判定＝ゲームモード中）→ `neg al/sbb/add` で **游戏中→0.0 / 外→1.0** となるよう整合（游戏中で本文実行）。**v157 §3.3 の「結果 0.0 = イベント成立」は極性誤り**であり、正しい対応は「0.0 = ブロック全体 skip / ≠0.0 = 本文 inline 実行」。

### 1.2 本文の実行主体 = caller1 の通常ループ（inline 実行）

- caller1（0x517330、arg9=0）は PC を `PC += fetch文長 + (成立時のみ u32)` で進める。**u32 加算なし＝PC は Begin ヘッダ（GameMode 6B / OnActivate 8B）だけ進む** → 次イテレーションの fetch が**本文の第 1 文を通常文として読み**、executor の通常ディスパッチ（0x516B55→0x517295 帯→0x5172C6 `call ecx` = impl 実行）で処理される。本文は「 Begin ブロックの残り」ではなく**通常文の列**として逐次実行される。
- caller1 は 0x4FBE00（レート制御付きスクリプト実行ホスト、20 呼び出し元: quest/REFR 系 0x46A19F/0x48Exxx/0x491Fxx/0x4D720A/0x529AB3/0x5314A7 等）と 0x4F9F00/0x4F9F30/0x4F9F60（0x6A46xx 帯のヘルパ）から RunScript 経由で駆動される。

### 1.3 FireEvent（caller2/arg9=1）は本文を実行しない（ゲート副作用のみ）

- 非 Begin 文: executor 0x516B55 `cmp [esp+0x780],0 / jne 0x5172d3` → **実行せずに `mov al,1`（0x5172D3 実測）で return** → caller2 は PC += fetch 文長だけ進めて次へ。
- Begin 文: band1 ゲートは実行される（副作用: OnActivate の 0x4D82E0 による保留消費等）が、**arg9=1 では結果に依らず 0x516B17 へ到達して本文を skip**。caller2 終了は `mov al,1`（0x5177FD）で常に成功。
- ⇒ **FireEvent = 「イベント該当判定とイベント状態の消費」を行う走査であり、本文実行経路ではない**。本文は同じスクリプトに対する caller1 実行（毎フレームの 0x4FBE00 系ホスト）で処理される。**V-172-r1 解消**（「handler 本文の実行主体」は caller1 inline 実行）。

### 1.4 証拠

- v178_p1_out.txt（caller1/caller2/executor 全文）、v178_p2_out.txt（GameMode 0x5022D0 / 0x578F60 / OnActivate 0x503F90 / FireEvent 0x517950 / 0x4F9EC0）、v178_p3_out.txt（opcode 表 rows 0..0x1F＝impl 一致、FAE80 prologue、0x4D8260/0x4D82E0）、v178_p5_out.txt（0x5172D3 `mov al,1`、RunScript 呼出し元 4 件、0x4FBE00 全文、fetch 0x516830）。

---

## §2 【残点2】'X'/'n'/'z' タグの emit 元と 0x4FCA30 の型語義 — **解明**

### 2.1 emit 側の全体像（SIB 対応バイトスキャンで確定、v178_p8/p9/p10）

コンパイラ帯 0x4F8000-0x502000 に対する `mov byte [..], imm8` スキャン（SIB フォーム対応後）の実リテール書手:

| タグ | 実書手 | 関数 | 備考 |
|---|---|---|---|
| 'G'(0x47) | 0x4FD75D | FUN_004FD5F0 | パーサの type 付与（buf+0x204、decompiled L196990 一致） |
| 'X'(0x58) | 0x4FCB2E/0x4FCB4B/0x4FCB68 | FUN_004FCA70 | opcode 名照合 3 帯すべて type='X'（decompiled L196155/196170/196192 一致） |
| 'G'/'r' | 0x4FDBED/0x4FDBF2、0x4FE8ED/0x4FE8FD、0x4FF16F 他 | FUN_004FDAF0 / FUN_004FDE30 / FUN_004FEF80 | レコード化書手（下記） |
| **'z'(0x7A)** | **0x4FE9A2** | **FUN_004FDE30** | `mov byte [esi+eax+0x20c],0x7a` → `fstp qword`（double 8B inline） |
| **'n'(0x6E)** | **0x4FE9F1** | **FUN_004FDE30** | `mov byte [esi+ecx+0x20c],0x6e` → `mov [..],eax`（i32 4B inline） |
| 'Z'(0x5A) | 0x4FDD84 | FUN_004FDAF0 | object 解決済みの bare レコード |
| 'f'/'s'/'l' | **即値書手なし** | — | FD5F0/FAA90 の算術 `'f'+0x0D` で type-char を作り、FDAF0 が `mov [edi+esi],al` で動的書き（偽陽性0x4FDBxx の C6 列は非整列デコード） |

### 2.2 'n'/'z' = %-トークン位置の数値リテラル（FUN_004FDE30、v178_p10 実測）

**FUN_004FDE30 = opcode 表の dispatcher 列（+0x1C）＝全コマンド共通の引数コンパイラ**（エラー文 "Invalid combat style/weather/NPC/owner/actor value '%s' for parameter %s." 実測、型別パラメータ検証を内包）。数値リテラル経路（0x4FE939-0x4FEA1C）:

```
id なし・val なしのトークン:
  ebp = [arg-type ノード]（dword）
  type==1 または type==0x17 → 0x47D550(token)（整数文字列判定）→ 'n' タグ + 0x983468(atol) → i32 書出し（消費 5B）
  それ以外               → 0x47D5B0(token)（浮動小数文字列判定）→ 'z' タグ + 0x985FB2(atof) → double 書出し（消費 9B）
```

- 0x47D550 = 整数文字列チェッカ（符号＋数字のみ、実測）、0x47D5B0 = 浮動小数文字列チェッカ（符号・小数点 1 個、実測）。
- consume 側（case1 0x4FACE9 `fild`／case2 0x4FAD06 8B コピー）と**バイト数・型が完全対応**。
- 同一判定は式コンパイラ帯（0x4F3EB7/0x4F3EE9 = FUN_004F36xx 帯、FAB20 の式評価 caller）からも使われ、同様に 'n'/'z' を emit する。**MessageBoxCompile の 0x47D550 呼出し（0x4FF3D8）は time 検証のみでタグ emit なし**（decompiled L197928 と一致）。

### 2.3 'X' = opcode/関数呼び出しの埋め込み（FUN_004FDAF0、v178_p9 実測）

FUN_004FDAF0（汎用 %変数レコード・シリアライザ、0x4FDBC7 で parser 0x4FD7C0 を呼んだ後）の 'X' 経路 0x4FDC28-0x4FDD50:

```
'X': [0x58] + u16 opcode-id 書出し（計 3B… consume 側は 5B: 下記の引数列が follow）
→ 行引き（0x4FCA30 と同一のアリフメティック: 0xB0B420/0xB0C8C0 ベース）
→ [行+0x10]（flagA）検証 → [行+0x14] argdesc / [行+0x1C] = 0x4FDE30 を取得
→ call ebp（0x4FDE30）で**関数自身の引数を同じバッファへインライン再コンパイル**
```

- consume 側（FAB20 0x4FAD37）: i16 id（カーソル +4 に丸め）→ 0x4FCA30 → mode により **FAE80 再帰（argdesc で引数デコード）** または **[行+0x18] impl の直接 call** — emit の再コンパイルと対。
- **'X' = 「この位置に関数呼び出し（opcode 参照）がある」**という型。emit/consume が対であることを行引きまで含めて確定。

### 2.4 0x4FCA30 の型語義 — **opcode ID → 表行 getter**（asm 実測、v178_p7）

```
0x4FCA30(id): id-0x100 ≤ 0x82 → row = 0xB0B420 + (id*5-0x500)*8   ; 帯B コマンド（0x100=Show..0x182）
              id-0x1000 ≤ 0x170 → row = 0xB0C8C0 + (id*5-0x5000)*8 ; 帯C 命令（0x1000=MessageBox..0x1170）
              それ以外 → NULL
```

v172 の「式オブジェクト取得」は**厳密には「静的オペコード表行（+0x10 flagA / +0x14 argdesc / +0x18 impl / +0x1C dispatcher）へのポインタ返却」**。名前→行のコンパイル時対は FUN_004FCA70（opcode 表 3 帯を `stricmp` 照合、一致で type='X'＋[buf+0x208]=行 ptr）。

### 2.5 残る暧昧点（新規残点 V-178-r1 へ）

- 'f' vs 's' を分ける **record[+0x10] フラグの語義**（FD5F0 の script 変数リスト this+0x3C / FAA90 の this+0x48 の両方で同一式）は未確定。
- **'l'(0x6C) の emit 元はリテールに不在**（consume 側のみ受付）。旧形式の予備と推定。

---

## §3 【残点3】0x45A500([0xB33B00]) 述語の語義 — **解明**

### 3.1 本体（v178_p4 実測）

```
0x45A500(this=[0xB33B00]):
  eax = [0xB33398] → edi = [[0xB33398]+0x10]        ; メインスレッド ID キャッシュ
  call [0xA2808C] = KERNEL32!GetCurrentThreadId     ; IAT 実測（PE import パース）
  cmp eax, edi / jne 0x45a51e
  → 本線: return [this+0x18] & 1                    ; bit0  = メインスレッド文脈フラグ
  → 他線: return ([this+0x18] >> 0x12) & 1          ; bit18 = 他スレッド文脈フラグ
```

**裁定: 0x45A500 = 「現在の実行スレッドがメインスレッドか否か」で `[this+0x18]` の 2 つの bool のどちらかを返すスレッド文脈述語**。Update skip ゲート（0x4CCC8F）の第 3 項は「この文脈では更新を止められてよい」判定、alarm 帯（0x60967E）も同一述語で分岐する。E8 呼出しは 124 件（エンジン全域）。

### 3.2 [0xB33B00] オブジェクトの構造（書手網羅走査、v178_p7/p9/p11）

| 検査 | 結果 |
|---|---|
| abs32 参照総数 | **2281 件**（.text。エンジン全域で使用） |
| 直接書手（A3/89x5/C705、全 3 セクション） | **2 件のみ**: 0x44690B（`mov [0xB33B00],ebx`＝ebx=0 → **NULL クリア**、teardown 0x4466F0 内）/ 0x45B3C8（ctor 内の公開） |
| ctor | **0x45B300**（thiscall。esi=this を zero-fill [this+4..0x1C0] → `mov [0xB33B00],esi` → malloc(0x10) 続き）。**E8 呼出し元は 0x441BB0 の 1 件のみ**（ゲーム初期化） |
| vtable | **なし**（ctor に vtable 代入なし＝非仮想 POD マネージャ） |
| +0/+4 | formID キーの 2 マップ（0x4533F0: arg フラグで [this+0] か [this+4] を選び 0x55E000 lookup → フラグ dword 返却。bit 0x40000 判定で使用） |
| +0x7C | formType byte（0x45A170: `[0x20,0x5A)` なら真 — 実測全文） |
| +0x18 | dword フラグ（bit0 / bit18 を 0x45A500 が選択） |
| +0x80 / +0x7C | 0x426A90（警報報道系）が +0x80 のノードと +0x7C byte を読む |

### 3.3 残点の位置づけ

「skip ゲートの第 3 項」は **`!述語`（＝その文脈で update 保留可）** という一般化された判定材料であり、特定クラスへの依存ではない。クラス名は非仮想・RTTI 不在のため未命名（新規残点 V-178-r2 に整理）。

---

## §4 【残点4】type0x3D float extra を crime gold と馬盗難（IsHorseStolen）が共有する理由 — **解明**

### 4.1 type0x3D ノードの実体（0x4269E0 全文実測、v178_p6）

```
0x4269E0(this(=ExtraDataList), delta_f32):      ; thiscall ret 4
  node = 0x41E210(this, 0x3D)                   ; type0x3D の get（ビットマップ+リンクリスト走査、v178_p6 実測）
  node==0 → malloc(0x10) → 0x42A290(node, delta)（+0xC に初期値）→ 0x41E0A0(this, node)（extra list へ attach）
  node≠0 → [node+0xC] += delta                  ; ★ アキュムレータ
  [node+0xC] ≤ 0 → 0x422E20(this, node, 1)      ; ノード削除（0 で消える）
```

- reader **0x41FC90** = `push 0x3d; call 0x41e210; fld [node+0xC]`（無ければ 0.0）。
- **this は ref+0x44（TESObjectREFR の ExtraDataList フィールド）**であり、馬・被害者・プレイヤのいずれも同一形式。

### 4.2 全 consumer/writer の帰属（5 reader / 4 writer 実測）

| site | 関数 | 役割 |
|---|---|---|
| writer 0x50B558 | **0x50B3E0 = OnAlarmVictim band1**（opcode 表 row14 と impl 一致） | 被害者 ref +0x44 へ加算。被害者が player で [player+0x116]≠0 なら [player+0x700] を加算（バーグラー経路）、非 player は計算値を直接加算 |
| writer 0x60FDA3 | 0x60FD20（this==player 限定の犯罪解決系） | 被害者 +0x44 の値 >0 なら `add [player+0x6DC],1`（カウント）＋ `fild [0xB376B0]`（整数グローバル）を 0x4269E0 で加算＝**減算によるマーカ消去**（≤0 でノード削除） |
| writer 0x60FC54 | 0x60FBC0（小型 getter: this==player で [+0x116]/[+0x700] 特例 → 値を 0..1 に丸めて返す） | — |
| writer 0x426F91 | 0x426A90（警報報道/通知系。[0xB33B00] 0x45A170/0x4534D0 使用、'BOLG'(GLOB form) チェック） | 通知時に対象 ref へ加算 |
| reader 0x5DAB55 | 0x5DA930（統計収集系: player の +0x660/+0x664、actor-value ヒストグラム等） | **player ref +0x44** の type0x3D（＝プレイヤ犯罪金メーター）を読む |
| reader 0x5EADF6 | 0x5EA800（攻撃対象選定帯: process vcall+0x3D0、base formType 0x23 判定、vt+0x190 ゲート使用） | 候補 ref の type0x3D を読む |
| reader 0x4F8C55 | **0x4F8C20 = IsHorseStolen impl** | 馬 ref +0x44 → 0x41FC90 → `fcomp [0xA2FAA8]`（**実測 0.0f**）>0 なら *out=1.0。デバッグ文 0xA49730 "%s is stolen" / 0xA4971c "%s is not stolen" 実測 |

### 4.3 裁定（共有理由）

- **type0x3D は「ref 単位の犯罪/盗難値アキュムレータ」という単一機構**。犯罪金（プレイヤーの犯罪値メーター、被害者への加算・消去）と「この ref は盗難された」というマーカが**同一 extra ノード**に統合されている。
- **盗難馬は「馬 ref に犯罪値マーカが付いた状態」**であり、IsHorseStolen はそれを >0 で観測するだけ。共有は**意図的な機構再用**（専用 bool フラグではなく値付きアキュムレータ）。
- しきい値 [0xA2FAA8] = 0.0f は同一定数が他所（0x4FBE00 の fQuestDelayTime 間隔チェック）でも使われる**共用 0.0 リテラル**で、両機構に本質的関係はない。
- 留保: 「馬にマーカを付ける直接書手」は OnAlarmVictim/警報報道帯（0x50B3E0 / 0x426A90）の経由と推定されるが、馬盗難発生時の专パスの特定は未実施（新規残点に計上しないが、機能解明には不要と裁定）。

---

## §5 【残点5】Process 系 slot+0x190 のシグネチャ差 — **解明（RTTI で確定）**

### 5.1 vtable 実 bytes と RTTI クラス名（v178_p8 実測、COL→TypeDescriptor 解析）

| fn | +0x190 を持つ vtable（slot アドレス） | RTTI クラス名 |
|---|---|---|
| 0x634910 | 0xA71814 / 0xA72684 | **`.?AVHighProcess@@` / `.?AVMiddleHighProcess@@`** |
| 0x60D880 | 0xA6F5F4 / 0xA72234 / 0xA72EF4 | **`.?AVBaseProcess@@` / `.?AVLowProcess@@` / `.?AVMiddleLowProcess@@`** |
| 0x977C50 | 0xA6E1FC/0xA6E204 / 0xA6FE24 / 0xA6F214 他 | `.?AVActor@@`（×2）/ `.?AVCharacter@@` / `.?AVArrowProjectile@@` … |
| 0x69D990 | 0xA44098 ほか複数 | `.?AVTESObjectAPPA@@` / `.?AVTESObjectBOOK@@` / `.?AVTESObjectWEAP@@` …（素のフォーム系でも同スタブ共有） |

### 5.2 Process 系 +0x190 の実体（v178_p8 ダンプ）

- **BaseProcess 0x60D880**: `mov esi,[esp+8]; test; lea eax,[esi+4]; call [0xA2807C](Interlocked系) → 0 なら [esi] vcall(+0, push 1)` = **引数オブジェクトの参照カウントを解放するハンドル解放 API（ret 4）**。Low/MiddleLow は BaseProcess 実装を継承。
- **HighProcess 0x634910 override**: `[this+0x118]`（Ni ハンドル）を引数と比較・Interlocked 減算・旧ハンドル解放・新ハンドル代入を行う**同じプロトコルの差し替え版**（SEH 付き大きな本体）。MiddleHigh も共有。
- 0xA2807C = InterlockedDecrement 系 IAT（両者同一）。

### 5.3 Actor/REFR 家族の近傍（v178_p11 実測）

```
Actor  vt 0xA6E074: +0x188=0x977C50  +0x18C=0x5E0EC0  +0x190=0x977C50  +0x194=0x65A330  +0x198=0x5E33B0
REFR   vt 0xA46C44: +0x188=0x69D990  +0x18C=0x6F7070  +0x190=0x69D990  +0x194=0x4D8AA0  +0x198=0x4D7DD0
```

近傍はいずれも家族固有メソッドで、常真/常偽スタブは +0x188/+0x190/+0x19C/+0x1A0 に**連続再利用**（v172 の「スタブ共有」を近傍まで強化）。

### 5.4 裁定

**vtable オフセットは「クラス階層ごとに独立したスロット番号」であり、0x190 は共有 API ではない。** Process 系（BaseProcess→Low/Middle/High の別ツリー、REFR+0x58 の process ポインタ経由で間接到達）の +0x190 は「NiRefObject ハンドル解放」、Actor/REFR 家族の +0x190 は「Update スキップゲート」— **別クラスの別メソッドが同一バイト位置に並んだ偶然**。0x4CCC50 の vcall +0x190 は REFR/Actor オブジェクトにのみ発行され、Process オブジェクトの +0x190 がゲートとして呼び出される経路は存在しない（呼び出し側が「レシーバは REFR 家族」を保証する構造）。**シグネチャ差（`ret` vs `ret 4`）は slot 割当ずれではなくクラス系統差**で、V-172-r5 解消。留保は v172 §12.2 どおり「slot+0x190 = ゲート」は REFR/Actor 家族限定の解釈であり、本調査でその限定の理由が構造的に説明された。

---

## §6 【残点6】Begin u32 累積値の上位 consumer（0x517810 の呼び出し元） — **解明（不存在の独立再検証）**

### 6.1 経路の全確認（v178_p1/p5/p9 実測）

1. **累積先は caller1 ローカル [E1+0x1C]**（fetch 文長＋Begin u32 の加算先）。0x5174EF `mov [esp+0x1c], ebx` で**毎イテレーション 0 にリセット**され、0x51755E で PC 加算に消費。v157 の「ebx 累積」説は v172 訂正済み（本調査でも caller1 の ebx は 0x517384 `xor ebx,ebx` 後、print 経路 0x5173E3 のみ改変）。
2. **caller1 戻り値 = al のみ**: 0x5175F0 `mov bl,[esi+0xa1]` → 0x51760A `mov al,bl`（アクション実行フラグ）。epilogue はレジスタ復元のみで他に持ち出す値なし。
3. **wrapper 0x517810**: 0x5178FF `mov bl,al` → frame free → 0x51790B `mov al,bl`。bool 透過。
4. **呼出し元 4 件すべて al のみ**:
   - 0x4F9F20（FUN_004F9F00）: `call 0x517810 → ret 8`（al 透過）
   - 0x4F9F50（FUN_004F9F30）: 同 `ret 8`
   - 0x4F9F82（FUN_004F9F60）: 同 `ret 0xC`
   - 0x4FBE6F（FUN_004FBE00）: `mov bl,al` → レート制御後 0x4FBEF4 `mov al,bl` で自関数の戻り値へ
5. **横断状態**: caller1 終了時（0x5175B3-0x5175ED）に QueryPerformanceCounter（IAT 0xA28158/0xA2815C）で計測した実行時間を **[script+0x34] へ加算**（実行時間平均化）。これは u32 と別系統で、他に caller 外へ出る値は無い。

### 6.2 裁定

**Begin u32 に上位 consumer は存在しない**（ローカル消費のみ）を独立に再検証。v172 O-a「consumer 不在」結論を維持・強化。u32（Begin 本文バイト長）の意義は PC スキップのみ。

---

## §7 【残点7】0x4FAB20 ジャンプテーブルの 's'（文字列型）consume パス詳細 — **解明**

### 7.1 'f'/'l'/'s' 経路の全文（v178_p7 実測）

```
0x4FADF3  mov eax,[esi] / movzx ecx,word [eax+edi] / add eax,2 / mov [esi],eax   ; i16 id 読取（+2）
0x4FADFC  cmp bl,0x66('f') / je 0x4fae0d
0x4FAE03  cmp bl,0x6c('l') / je 0x4fae0d
0x4FAE08  cmp bl,0x73('s') / jne 0x4fadde                                        ; 3 種以外は fail
0x4FAE0D  edx=[esp+0x18](ctx) / eax=sx(id) / ecx=[esp+0x14](owner)
          push edx / push eax / call 0x4fa110
0x4FAE1F  fstp qword [ecx]   → al=1（成功・double 出力）
```

### 7.2 0x4FA110 本体（v178_p7 実測）

```
memo: [0xB361BC]==owner && [0xB09E20]==id → [0xB361C0] を即利用
walk: eax=[owner+0x0C]（head）
      node: [node+0]=entry / [node+4]=next、entry: [entry+0]=id(u32) / [entry+8]=f64 値
      [entry]==id → esi=entry
hit  → fld qword [entry+8]、ret 8（メモ 3 変数へ書直し）
miss → 0x4A7A60 で 0xA49A68 "Variable ID %08X not found. Try to recompile script '%s'. …"
       （script 名は vt+0xd4 呼出、無ければ 0xA309A4 "UNKNOWN"）→ 0.0、ret 8
```

### 7.3 's' の emit 側（v172 との接続＋新事実）

- **'f'/'s' の切替式（decompiled 実測）**: FUN_004FD5F0 L196975 / FUN_004FAA90 L194383: `type = 'f'(0x66) + (record[+0x10] != 0 ? 0x0D : 0)` — **変数レコード +0x10 の byte フラグが非 0 なら 's'(0x73)**。FD5F0 は script 自身の変数リスト（this+0x3C）、FAA90 は this+0x48 のリスト（0x4FBE98 では "fQuestDelayTime" 等のグローバル名を解決）で同じ式。
- emit バイト列は FDAF0 が type-char を動的書出し（`[0x72][u16 id]` か bare `[type-char][u16 val]`）＝v157 の対応表に追加される形で 's' も 3B レコード。
- **runtime は 'f' と 's' が完全同一解決器**（id→f64）。タグ差はコンパイル時の変数区分のみ。
- **'l' は emit 元なし**（consume は受付）。

### 7.4 残る暧昧点（新規残点 V-178-r1 へ）

record[+0x10] フラグの実語義（数値型/参照型/プロパティ等の区分候補）と 'l' の出自。

---

## §8 新規残点（V178 起票粒度）

| ID | 内容 | 手掛かり |
|---|---|---|
| **V-178-r1** | %変数レコード +0x10 フラグの語義（'f'/'s' を分ける条件。FD5F0 の this+0x3C リストと FAA90 の this+0x48 リストのレコード構造）と、'l'(0x6C) タグの emit 元（リテール不在＝旧形式予備の確認） | §2.5、§7.3、decompiled L196975/L194383 |
| **V-178-r2** | [0xB33B00] シングルトンのクラス命名（非仮想・RTTI 無し。ctor 0x45B300 の唯一の呼び出し元 0x441BB0 の同定、および [+0x18] bit0/bit18 をセットする書手の特定） | §3.2、v178_p7/p11 |
| **V-178-r3** | 0x4FBE00（本文実行ホスト）のレート制御仕様化: [esi+0x38] タイマ / [esi+0x3c] 累積 / [0xB33E9C] フレームデルタ / fQuestDelayTime 変数（0x4FAA90+0x4FA110 経由）と [0xA2FAA8]=0.0 比較の移植仕様 | §1.2、v178_p5 |
| **V-178-r4** | fetch 0x516830 の opcode 0x1C スペシャルケース（イベント変数解決パス: 0x4FA9C0 経由の [rec+0xC] チェックと `seta` フラグ）の意味論。本タスクは深究せず | v178_p5（fetch prologue） |
| **V-178-r5** | 盗難馬へ type0x3D マーカを付与する専用発生経路（OnAlarmVictim/警報報道帯経由と推定。馬 ref を victim とする警報の成立条件） | §4.3 留保 |

## §9 自己検証

```
$ python "%TEMP%\opencode\probe_harness.py" selftest
   bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl
   => ALL PASS（4/4、着手時）
```

| # | 検証項目 | 期待 | 実測 | 判定 |
|---|---|---|---|---|
| 1 | executor 共通戻り 0x5172D3 | al=1（非 Begin を arg9=1 で素通し） | `mov al,1` 実測 | OK |
| 2 | caller2 の arg9/arg8 | 1/0 | 0x5177AD `push 1` / 0x5177AF `push 0` 即値 | OK |
| 3 | OnActivate band1 の極性 | 既定 0.0・保留時 1.0 | 0x503FA8 fstp 0.0 / 0x503FEA fld1 | OK |
| 4 | GameMode 極性 | 游戏中→0.0（skip されない） | 0x578f60 が [obj+8]!=1 判定＋neg/sbb/add と整合 | OK |
| 5 | FireEvent wrapper 呼出し元 | 1 件（0x4F9EF1） | 実測 1 件 | OK |
| 6 | RunScript wrapper 呼出し元 | 4 件・al のみ | 0x4F9F20/0x4F9F50/0x4F9F82/0x4FBE6F（ret 直後・mov bl,al 確認） | OK |
| 7 | 'z'/'n' emit 即値書手 | SIB 対応で検出 | 0x4FE9A2='z' / 0x4FE9F1='n'（FUN_004FDE30 内） | OK |
| 8 | 0x47D550/0x47D5B0 | 整数/浮動小数文字列判定 | 0x4FE9D7/0x4FE988 呼出し＋本体実測（各 4/6 呼出し元） | OK |
| 9 | 'X' emit | id レコード＋再コンパイル | 0x4FDC30-0x4FDD50（行引き=0x4FCA30 同型、[行+0x1C]=0x4FDE30 call） | OK |
| 10 | 0x4FCA30 | id→表行 getter | 帯B 0xB0B420+(id*5-0x500)*8 / 帯C 0xB0C8C0+(id*5-0x5000)*8 | OK |
| 11 | IAT 0xA2808C | GetCurrentThreadId | KERNEL32.dll!GetCurrentThreadId（PE import パース） | OK |
| 12 | [0xB33B00] 書手 | 2 件（ctor/NULL クリア） | 0x45B3C8 / 0x44690B（A3/89x5/C705 全走査、refs=2281） | OK |
| 13 | ctor 呼出し元 | 1 件 | 0x441BB0 | OK |
| 14 | [0xA2FAA8] | 0.0f | bytes 00000000 | OK |
| 15 | 0x41FC90 callers | 5 件 | 0x4F8C55/0x5DAB55/0x5EADF6/0x60FBC7/0x60FD79（再確認） | OK |
| 16 | 0x4269E0 writers | 4 件 | 0x426F91/0x50B558/0x60FC54/0x60FDA3 | OK |
| 17 | IsHorseStolen デバッグ文 | stolen 判定文 | "%s is stolen" / "%s is not stolen"（0xA49730/0xA4971C） | OK |
| 18 | Process vtable +0x190 RTTI | Base/Low/MiddleLow=0x60D880, High/MiddleHigh=0x634910 | .?AVBaseProcess@@/.?AVLowProcess@@/.?AVMiddleLowProcess@@ / .?AVHighProcess@@/.?AVMiddleHighProcess@@ | OK |
| 19 | Actor/REFR vtable 近傍 | +0x190=ゲート・近傍は家族固有 | Actor 0xA6E074 / REFR 0xA46C44 実測 | OK |
| 20 | 0x4FA110 エラー文 | 再コンパイル勧告 | 0xA49A68 実測（先頭 "Variable ID %08X not found…"） | OK |
| 21 | FAB20 'f/l/s' 消費 3B | i16 読取 | 0x4FADF5 `add eax,2`（tag 1B と合計 3B） | OK |

## §10 使用証拠ファイル

- 読み取り: `tmp/re/v172_runtime_residuals2.md` / `tmp/re/v157_runtime_residuals.md` / `tmp/re/cache/opcode_table.json`（＋ v178 内で named_funcs.txt 形式確認のみ） / `docs/Oblivion_decompiled.c`（FUN_004fd7c0 L197009-197149 / FUN_004fca70 L196142-196195 / FUN_004fd5f0 L196912-197003 / FUN_004faa90 L194367-194389 / FUN_004fca30 L196123-196136 / MessageBoxCompile L197840-197949）
- capstone 5.0.7 逆アスク（v172_common.py 経由、全 12 プローブ）
- 本タスク生成（%TEMP%\opencode のみ）: `v178_p1.py`（前セッション分を活用）/ `v178_p2.py` / `v178_p3.py` / `v178_p4.py`（修正）/ `v178_p5.py`〜`v178_p12.py` と対応 `v178_p*_out.txt`。**repo 内新規ファイルは本レポート 1 件のみ**。
- 未使用（禁止事項遵守）: ESM（再スキャン禁止）、`docs/`・`TODO_re49.md`・`session-memory.json`・`plan.md`・`func_list_new.txt`・`cache/`（いずれも編集なし）、git 操作なし。
