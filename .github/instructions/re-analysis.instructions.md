---
applyTo: "docs/RE_*.md,tmp/re/**"
---

# RE 解析ドキュメント作業ルール（Oblivion EXE 解析）

- 正典は `docs/RE_EXE_REPORT.md`（§1〜§55）。矛盾時は §55 補遺 → §50 の優先順で解決する
- 便2 の最終成果物は `tmp/re/s50_*.md`（11 本）。証跡は `tmp/re/s50_*.txt` / `*.py`
- 断定度は `[確定]` / `[推定]` / `[未確定]` を明示し、VA と根拠（ファイル・行番号）を必ず添える
- 呼出元走査は生 E8 byte 走査（`va + 5 + rel == target`）。VA→offset は `pe.get_offset_from_rva(VA - 0x400000)`
- 共有ヘルパー `tmp/re/re_util.py` は変更禁止（pe / va2off / rb / u32 / cstr / disasm / callers / find_pattern / save_text）
- docs 編集は Python `io.open` で LF・BOM なし（`.github/copilot-instructions.md` は CRLF を維持）
- git commit はユーザーの明示承諾があるまで禁止。`tmp/` は保全（削除禁止）
- 逆コンパイル由来コードベース（OpenTES4 / xOBSE / COEF 等）は参照・流用しない（クリーンルーム維持）
- 未確定残件は `docs/RE_EXE_REPORT.md` §55.8。B13（.data 実機ダンプ）はユーザー実行待ち（手順: `tmp/re/s50_b13_plan.md`）
