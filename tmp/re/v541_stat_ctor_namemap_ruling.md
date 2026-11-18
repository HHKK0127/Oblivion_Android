# v541 — AC20 の 19 周ループの実体クラス＝**`TESObjectSTAT`（静的 activator）と確定**（`FUN_004B9AA0` ctor が primary vtable `0xA44DEC`＋`+0x24` に TESFullName 補助 vtable `0xA44DC8`・type byte `0x1C`（28='STAT'）をストア、両 vtable の COL→TD が同一 `.?AVTESObjectSTAT@@`）・**第 2 静的 map `0xB06164` の挿入 API＝`FUN_00412D30`（name→form レジストリ thiscall add）と全 call site（約34箇所）確定**（FE90 の topic 名登録・`0x517FE0` TESRace ctor 群・AC20/`44F0C0` 等）・ブック本文配列 `DAT_00b13974` は**マネージャ singleton `0xB13970`（vtable `0xA69680`・CRT 静的）のメンバ**と確定（`0xA24910`＝dtor: free(member)＋vtable 書戻、owner メソッド `5852A0/587B00/591B40/591C50` が `mov ecx,0xb13970` で thiscall／`FUN_00582160`＝`DAT_00b3a6e0` インタフェース singleton getter）（V-541-1..3 完遂・旧残点 V-406-3 継続第71弾）

- 採番: **v541**（旧残点群消化・probe prefix **v541_1..6 全6本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-541-1..3（v540 残点）** — 1=AC20 の 19 周ループの実体クラス＋スロット配列の初期化元／2=`DAT_00b13974` 設定元＋`FUN_00582160` identity／3=`0xB06164` map の挿入側 call site。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・RTTI（COL→TD 実読）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-541-1: AC20 の 19 周ループの実体クラス** | **[確定] `TESObjectSTAT`（静的 activator）** | `FUN_004B9AA0`（呼び元: `0x448101`（head `FUN_00447DF0`）と AC20 ループ内 `0x44B6xx..0x44C544` の多数＝**AC20 全体が同一 ctor の繰り返し**と判明）: `call FUN_004B2FF0`（基底 ctor）→ **`mov [esi],0xA44DEC`**（primary vtable）→ `lea edi,[esi+0x24]` → `call FUN_0046D7E0`（TESFullName 補助初期化）→ **`mov [edi],0xA44DC8`**（`+0x24` 補助 vtable）→ **`mov byte[esi+4],0x1C`**（type=28='STAT'）。**RTTI 実測: `0xA44DEC-4` の COL `0xABB9CC`→TD `0xB054D4`＝`.?AVTESObjectSTAT@@`、`0xA44DC8` の COL `0xABBA2C`→TD 同一**。サイズ 0x3C。→ **AC20 の「19 スロット実体化」＝DataHandler 起動時に静的 STAT（ activator 親）を作るパス**。スロット配列窓（`[esp+0x6c + idx*4]`・19 エントリ）は AC20 のスタックフレーム上の**名前ポインタ配列**で、各 slot の `+0xC` を `vtable[+0xD8](name)` に渡す [部分確定]（配列のロード元この窓は `param_1+0x6c..` 由来＝DataHandler メンバ窓 [推定]）。 |
| **V-541-3: `0xB06164`（第2静的 map）の挿入側** | **[確定] 挿入 API＝`FUN_00412D30`・call site 約34箇所** | `mov ecx,0xb06164` の直後 call を全列挙: **`FUN_00412D30` ×32**（`0x44B61E`/`0x44C677..0x44CAC3`（AC20 内 7 箇所）/`0x44F2D4`（`FUN_0044F0C0`）/**`0x52FFCD`（FE90 の topic 登録後＝v539 の⑨）**/`0x5180DC..0x519300`（head `FUN_00517FE0` TESRace ctor 群 20 箇所））＋**find 側**は `0x447160/0x447490→FUN_0055E000`、`0x46B29A→55E000`、atexit `0xA18530→0xA1853A call 0x65C620`（**シャットダウンで map オブジェクト自体を list 除去**）。`FUN_00412D30` 頭: `ebx=name・esi=map・vt[+4](name)→bucket・walk・miss 時 alloc→link`（v536 CRUD と同じコールバック組）。→ **第 2 map ＝「name（EditorID 文字列）→ form」グローバル名寄せレジストリ**。TESRace ctor が 20 箇所（種族 20 体）を、FE90 が topic 85 件を、AC20 が STAT/その他を登録する。 |
| **V-541-2: ブック本文配列の設定元** | **[確定]（構造）／管理者の読込メソッドは [部分確定]** | **`0xB13974`（text ptr 配列）と `0xB1397A`（count word）はマネージャ singleton `0xB13970`（+4/+0xA）のメンバ**。静的 `0xB13970=0xA69680`（vtable・CRT 静的 object）＋ `0xB13974/7A=0`（＝**初期値は空、実行時割付**）。`disp32 0xB13970` site: **`mov ecx,0xb13970` の thiscall 4 頭（`FUN_005852A0`/`FUN_00587B00`（×2）/`FUN_00591B40`/`FUN_00591C50`＝うち 5852A0/587B00 は docs で**スカラー削除デストラクタ**（`FUN_00584C00/587940` 本体破棄＋delete フラグで `401F20`）と確定）＋ `FUN_00A24910` の atexit dtor**（`free([0xb13974])` 後 **`mov [0xb13970],0xa69680`**＝vtable 復元）。→ **割当/設定はマネージャメソッド内 `[ecx+4]` 経由**（disp32 直書きが無い理由が説明できた [確定]）。readers: `FUN_0057ED20`（count<7＋slot@0x18 検査）/`FUN_005AC7B0`（count<0x1A＋@0xAC）/`FUN_00589B70`（本文 ptr）/`FUN_005821F0`（count<0x2C＋@0xAC）。本文配列の**割当元メソッド本体**は未追跡（残点 V-542-1）。 |
| **V-541-2: `FUN_00582160` identity** | **[部分確定]（命令）「UI インタフェース singleton getter」** | 頭: `cmp byte[esp+0x14],0 / mov eax,[0xb3a6e0] / je… test eax…`（SEH フレーム）＝**`DAT_00b3a6e0` のインタフェースマネージャ返却（flag 分岐で生成/別 singleton に切替）**。`578D70` が `582160(0,1)->+0xB0` を読む＝**UI（ブック表示）状態構造体の pending バイト**。完全 identity（クラス名）は未確定 [部分確定]。 |

## §2 証拠

### §2.1 `FUN_004B9AA0`（TESObjectSTAT ctor）生バイト
```
4B9AA0: SEH プローローグ
4B9AC3: esi=ecx / call 0x4b2ff0（基底 ctor）
4B9ACE: edi=esi+0x24 / ecx=edi / call 0x46d7e0（TESFullName 補助）
4B9AE0: mov dword ptr [esi], 0xa44dec          ; ★ primary vtable
4B9AE6: mov dword ptr [edi], 0xa44dc8          ; ★ +0x24 補助 vtable
4B9AEC: mov byte ptr [esi + 4], 0x1c           ; ★ type=28='STAT'
4B9B03: ret
RTTI: rd32(0xA44DEC-4)=0xABB9CC→TD 0xB054D4='.?AVTESObjectSTAT@@'
      rd32(0xA44DC8-4)=0xABBA2C→TD 0xB054D4（同一）
```

### §2.2 AC20 ループと ctor の接続（v541_2）
```
callers of 4B9AA0: 0x448101（head 447DF0）＋ 0x44AF6E/0x44B510/0x44B6A3/0x44B76B/0x44B833/
  0x44B8FB/0x44B9C3/0x44BB53/0x44BC1B/0x44C2FC/0x44C544（全て AC20 の 0x44AC20..0x44CD80 窓内）
AC20 ループ窓（v540_4 §b）: alloc(0x3c)→ecx=new→call 4b9aa0→push 1,[slot+0xc formID?]→
  46c300（formID 紐付け）→[slot+0x10 名前?]→vt[+0xD8]→4b2b80→vt[+0x90](0)
```

### §2.3 `0xB06164` 系 call site（v541_4・全列挙）
```
ecx=0xB06164 → call 412D30（add）×32: 44B61E 44C677 44C755 44C82E 44C90E 44C9EA 44CAC3（AC20）
  44F2D4（44F0C0） 52FFCD（FE90・topic 名） 5180DC..519300（517FE0・TESRace ctor 20 箇所）
ecx=0xB06164 → call 55E000（find）×3: 44717A（447160） 4474AA（447490） 46B29A（46B280）
atexit thunk 0xA18530: mov eax,[0xb06150]… / 0xA1853A call 65C620（map 自体の list 除去）
```

### §2.4 ブックマネージャ（`0xB13970`）
```
静的 .data: 0xB13970=0x00A69680（vtable）0xB13974=0（text 配列）0xB1397A..=0x0001_0000（count/状態）
thiscall 頭: mov ecx,0xb13970 → 5852A0 / 587B00(×2) / 591B40 / 591C50
dtor 0xA24910: mov eax,[0xb13974]; mov ecx,0xb13970; push eax; call 401f20;
               mov dword ptr [0xb13970], 0xa69680（vtable 復元）
readers: 57ED20（cmp word[0xb1397a],6 ＋ [tbl+0x18]）/ 5AC7B0（<0x1A ＋ +0xAC）/
         589B70（本文 id*4）/ 5821F0（<0x2C ＋ +0xAC）
```

## §3 判定の内訳

- **V-541-1 完遂**: AC20 の 19 周＝**TESObjectSTAT 実体化**（RTTI 一次証拠）。スロット配列窓は AC20 のローカル（`[esp+0x6c]`）で、その初期化元（DataHandler メンバ窓 [推定]）のみ [部分確定]。
- **V-541-3 完遂**: 第2 map＝**name→form 名寄せレジストリ**（add=`412D30`×32・find=`55E000`×3）。FE90 の 9 段目（`412D30(name,topic)`）がこの map への登録そのものだった。TESRace ctor が 20 体ぶんの名前を積む（＝**v528 の「+0x74 名前生成」や race formID 問題（#159 系）の背景情報**）。
- **V-541-2 完遂（構造）／残（メソッド単位）**: 配列はマネージャのメンバで disp32 直書きが存在しない理由（`[ecx+4]`）を説明。本文配列を埋める具体メソッド（587B00 らい）の本体追跡と `582160` のクラス名は [部分確定]。

## §4 残点（V-542-1..3）

1. `FUN_00587B00`（`mov ecx,0xb13970` ×2・ブック本文のロード元メソッド）の本体追跡＋BMSG/DOBM レコードとの対応【中】
2. AC20 のスロット配列 `[esp+0x6c]` のロード元（`param_1+0x6c..` 窓）＝静的 STAT 名前テーブルの正体【低】
3. `FUN_004B2FF0`（TESObjectSTAT 基底 ctor）の基底チェーン（TESForm→TESObject→…）の経路確認と `46D7E0`（TESFullName ctor）の確定【低】

## §5 後続への申し送り

- **移植側**: プラグイン起動後処理（AC20 相当）で **19 個の `TESObjectSTAT` を name/formID 付きで実体化し、name→form レジストリ（4001 バケット）へ登録**する流れ。`412D30`＝map add／`55E000`＝find のペアで足りる（CRUD は v536 の 4524C0/452570 と同一コールバック組）。
- **EditorID 名寄せは「TESRace ctor・FE90・AC20・44F0C0」の 4 系統から積まれる**（20 race + 85 topic + 19 STAT + ファイル起動時）。移植側で EditorID 検索が効かない場合、この 4 経路のどれかが欠けている。
- **メッセージブック表示**は `0xB13970` マネージャ（text ptr 配列・count）＋`589B70(id)`（id 1001..1051）経由。本文配列は実行時割付（dtor で free）。
- **atexit 側（0xA18530/0xA18550 thunk）で map・マネージャの vtable 復元と list 除去**が行われる——移植側は JNI/onDestroy 相当で同じ後始末順序（free→vtable 戻し）を踏めばリーク検査と整合する。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
