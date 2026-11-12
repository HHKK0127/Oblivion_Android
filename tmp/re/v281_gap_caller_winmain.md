# v281_gap_caller_winmain.md — 依頼2: FUN_0040c350 唯一呼出し元 0x40F580（無名 gap）の特定（v93-bundle 2026-11-05）

- 依頼元: `tmp/re/handoff_20261105_v93_residuals_bundle.md` 依頼2 [優先度 b]（出典 v81 §7-4）
- 対象: Oblivion.exe v1.2.0416 (Steam)
- **判定: [確定] — gap 関数 = WinMain（0x40DF80..0x40F640）。直接 caller = WinMainCRTStartup@0x98769C（0x98780E）。到達時期 = プロセス終了時（メッセージループ退出後の shutdown 経路）に 1 回。**
- **注記: 本件は既に V93（`tmp/re/v93_gmst_gt_residuals.md` §b、2026-10-25 検証）で確定済み。本レポートは依頼対応として結論を再提示し、2026-11-04 に再検証（数値完全一致）したもの。再解析は行っていない。**

---

## 1. gap 関数の関数単位特定（依頼の問1）

台帳 21,597 件には `0x40d4d0` の次が `0x40f640` で、**[0x40DF74, 0x40F640) に開始点 0 件**（v81 の記載どおり。台帳外 = 無名 gap）。線形逆アセンブルによる実境界:

| 項目 | 値 | バイト証跡（`v280_spot.py` 再確認） |
|---|---|---|
| 前関数終端 | `0x40DF73: c3 ret` | `… 83 c4 10 c3` ✓ |
| int3 パディング | `0x40DF74..0x40DF7F`（12 B） | `cc ×12` ✓ |
| **開始** | **0x40DF80**（`81 ec a8 06 00 00` = `sub esp,0x6a8`） | ✓ |
| **終端** | **0x40F637: `c2 10 00` = `ret 0x10`**（stdcall 4 引数） | ✓ |
| 末尾パディング | `0x40F638..0x40F63F`（6 B） | `cc ×6` ✓ |
| サイズ | 5,816 B。gap 内の `ret` は 1 箇所のみ | byte スキャン c3=1 / 線形 ret-like=1 |

**WinMain である根拠**: 先頭 `sub esp,0x6a8` + `__security_cookie` + `mov ebx,[esp+0x6b0]`（引数2 hPrevInstance）/ `sprintf(buf,".\\%s",[0xb02c84])` → `call [0xA280E4]` = `KERNEL32!GetPrivateProfileIntA("General","bUseMyGamesDirectory",1,buf)` / `0x40F130` = Win32 メッセージポンプ本体 / 末尾 `ret 0x10`。

## 2. 直接 caller（依頼の問2）

- **E8 → 0x40DF80 = 1 件のみ = 0x98780E**、E9 = 0 件、全セクション abs32 = 0 件（再検証一致）。
- 呼出し形（4 引数 push）:

```
0x987805: 51                push ecx        ; nShowCmd
0x987806: 50                push eax        ; lpCmdLine
0x987807: 6a00              push 0          ; hPrevInstance
0x987809: 6800004000        push 0x400000   ; hInstance = ImageBase
0x98780E: e8 6d67a8ff       call 0x40DF80   ; WinMain(...)
```

- 含入関数 = **0x98769C..0x987886（490 B）= WinMainCRTStartup**（台帳外 gap。`GetStartupInfoA` / `HeapAlloc(…,0x94)` / `GetVersionExA` / `GetCommandLineA` / `_amsg_exit(8|9)` / **`call 0x981C6B`（CRT 初期化テーブル走査、依頼3）→ lpCmdLine 取得 → WinMain 呼出し → `__SEH_epilog`**）。
- 上位経路: `0x98769C` への E8 = 0 件・E9 = 内部 1 件（0x987881）。外部からの転移は `.bind` 難読化スタブ（PE AddressOfEntryPoint = 0xBAF2ED）経由で、計算経由のため E8/E9/imm32 では検出不能 = **間接転移**。

## 3. 到達時期（依頼の問3）

- `call 0x40C350` の所在 = **WinMain 内の shutdown 経路**（`v280_spot.py` 再確認: `0x40F580: e8 cb cd ff ff` → 解析先 = **0x40C350** ✓）。
- `[0x40F440, 0x40F640)` へ入る外部分岐は 4 本のみ:

| 元 | 分岐 | 先 | 意味 |
|---|---|---|---|
| 0x40F27D | `jne 0x40F488` | **終了経路の入口（唯一）** | ループ退出判定 `mov eax,[0xB33398] / cmp byte [eax],0` |
| 0x40E2C6 / 0x40E37B / 0x40E3AC | → 0x40F61D | 早期 return（teardown 素通り） | 成立条件は v93 残点3（未特定・軽微） |

- 経路: `0x40F488 → … → 0x40F580 call 0x40C350 → … → 0x40F618 call 0x747be0 → 0x40F61D epilog → 0x40F637 ret 0x10`。
- → **`FUN_0040c350`（動的 Setting* 9 配列 350 スロットの一括破棄）は、メッセージループが外部要求で退出し WinMain が正常終了するときだけ実行される。起動時 1 回でも ESM リロード毎でもない。**
- `FUN_0040d4d0` 系リセット経路との関係: **別経路**（`FUN_0040d4d0` は `FUN_004455E0`/`FUN_0044E720`/`FUN_005BDCD0` から呼ばれ初期化/リセット系。WinMain の gap 関数とは接続しない）。

## 4. 再検証（2026-11-04）

`v93_final_verify.py` 再実行:

```
[b] E8->0x40C350 : ['0x40f580']
    E8->0x40DF80 : ['0x98780e']   E9->0x40DF80 : []   abs32(0x40DF80): []
    ledger has 0x40DF80/0x98769C/0x981C6B : False / False / False
    E8->0x98769C : []   E9->0x98769C : ['0x987881'](内部)
```

`v280_spot.py`: gap 境界 4 点（ret/int3/start/ret 0x10/int3）+ `call 0x40C350` と `call WinMain` の rel32 解析先を全一致確認。

## 5. 残点

- WinMain の早期 return 経路 3 本の成立条件（v93 残点3）— 依頼の範囲外。
- 実機での到達（終了時以外に到達しないこと）の動的確認未実施（v93 残点5）— 静的経路証明のみ。

## 6. 証跡

| スクリプト | 内容 |
|---|---|
| `%TEMP%\opencode\v93_final_verify.py` | 呼出しグラフ（E8/E9/abs32）再計算（本日再実行） |
| `%TEMP%\opencode\v280_spot.py` | gap 境界・rel32 解析先のバイト確認（本日新規） |
| `%TEMP%\opencode\v93_step13..19.py` 等 | V93 当時の CFG/線形デコード（`v93_gmst_gt_residuals.md` §証跡参照） |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。正本 `tmp/re/v93_gmst_gt_residuals.md` §b と数値完全一致。
