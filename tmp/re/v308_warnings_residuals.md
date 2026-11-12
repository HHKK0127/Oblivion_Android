# v308 — v306 Warnings 経路残点の裁定

- 対象: Oblivion Steam 1.2.0416
- 対象範囲: `FUN_0045ADC0` caller1、`FUN_0045CC60` caller2、`0x494000` logger/path-helper 帯
- 方針: 既存 v301/v304/v306 の中間証跡を再照合。推測実装・正典編集は行わない。

## 1. B-5: caller1 の出力名と Warnings.txt の一致

### 1.1 caller1 は `object+0x3C` を `.txt` 化する

v306 の命令列で `FUN_0045ADC0` caller は次のように呼び出す。

- `0x4657DD`: `lea edx,[esi+0x3c]`
- `0x4657E1`: `push edx; call 0x45adc0`
- `0x4669A8`: `lea eax,[esi+0x3c]`
- `0x4669AC`: `push eax; call 0x45adc0`

`FUN_0045ADC0` はこの引数をパス文字列として `lstrcpyA` で作業バッファへコピーし、`.txt` (`0xA3A4D4`) を `lstrcatA` で付加する。したがって caller1 の直接出力名は `(<object+0x3C>) + ".txt"` である。[確定]

### 1.2 `object+0x3C` はセーブオブジェクトのパス欄である

追加照合で以下を確認した。

1. セーブオブジェクトの初期化帯 `0x45B300` 以降では、`0x45B35F` に `mov dword ptr [esi+0x3c],0` がある。これはパス欄を含むオブジェクト初期化である。
2. `FUN_0045F2E0` は入力オブジェクトの `+0x3C` (`0x45F335`) をローカルパスへコピーする。これは当該フィールドが NUL 終端のパス文字列であることを示す。
3. セーブ列挙・生成帯 `0x45D52C..0x45D53E` は format `0xA2F88C = "%s%s%s"` を使い、`0xB3F280`（ベースディレクトリ）を引数としてパスを組み立てる。
4. 同帯の別経路 `0x45F3A7..0x45F3AD` は format `0xA3A88C = "%s%s%s.ess"` を使用し、`0xA388B8 = "Saves\\"` を含むベースディレクトリ配下のセーブパスを生成する。

このため `object+0x3C` は `Warnings.txt` 用の固定ログパスではなく、セーブオブジェクトのファイルパス欄と同定できる。[確定]

### 1.3 B-5 の裁定

caller1 の出力はセーブファイル名に `.txt` を付加する what-changed 出力であり、`0xA3D9BC = "Warnings.txt"` を直接参照する path getter の戻り値とは別経路である。通常のセーブパスでは `<base>\Saves\<name>.ess.txt` 系となるため、`Warnings.txt` との実行時一致は成立しない。[部分確定]

実際のセーブ名を含む動的実行値は静的証跡だけでは取得していないため、slot 名そのものは未観測とする。ただし、ファイル種別・経路の同一性については負に裁定できる。

## 2. v301 と v306 の矛盾

`0x4944BC` の命令は `Warnings.txt` の文字列アドレスを参照するが、関数境界は次の通りである。

- CC padding: `0x494474..0x49447F`
- 関数先頭: `0x494480`
- `0x4944BC`: path getter 本体内
- getter の戻り値: `0xB34C80` に作った `<base>\Warnings.txt`

`FUN_004941A0` のロガー本体は `0x4941A0` から始まり、`0x4944BC` を含まない。v301 の「ロガー体内の文字列参照」は、CC/関数境界を跨いだ線形帯の解釈に基づく誤帰属である。v306 の関数プロローグ、CC境界、`ret`、path getter の処理内容が一致するため、v306の裁定を採用する。[確定]

なお、v301で確認された `0x4A7A60 -> FUN_004941A0` のログ呼び出し自体は存在するが、Warnings path getter の書出経路を意味しない。vtable `ch=1` は `0x4943A0` の no-op に到達し、Warnings.txt への書出しは確認できない。[確定負]

## 3. v306残点の更新

| 項目 | 判定 | 根拠 |
|---|---|---|
| caller1 出力名が Warnings.txt と一致 | **[部分確定・負]** | `object+0x3C` はセーブパス欄、caller1 は `.txt` 付加。動的slot名のみ未観測 |
| `0x4944BC` の包含関数 | **[確定]** | `0x494480` path getter。`FUN_004941A0`ではない |
| v301/v306 境界矛盾 | **[確定]** | v301は関数境界を跨いだ帯判定、v306はCC+プロローグで再確定 |
| caller2 verbose gate | **[確定]** | `0x45CD8F: push 1`、formatter arg4 `[esp+0x178]` |
| caller1 verbose gate | **[確定]** | `0x45B016: push 0`、formatter arg4 `[esp+0x178]` |
| `FUN_00494730` のWarnings書出し | **[確定負]** | 直接call 0件、ProgramFlow/StackTrace経路のみ |
| `0x5723E0` テーブルの最終flush | **[未確定]** | v306範囲では満杯時 `0x572624` 以外のflushなし |
| `0xA37B78`/`0xA52A98` 文言全文 | **[未確定]** | push siteは確認済みだが、該当文字列本体の取得証跡なし |

## 4. 結論

- v301の「Warnings.txt sink接続確定」は取り消し、v306の負証拠を採用する。
- caller1はWarnings.txt writerではなく、セーブ由来の what-changed `.txt` writerである。
- Warnings.txtの静的直接参照は `FUN_00494480` path getterに限られ、既存証跡では削除・サイズ確認・pathログ用途までしか確認できない。
- Android移植側のWarnings.txt互換実装や推測パーサ追加は不要。正典への反映は既存の承認ゲート（#47/#66）で扱う。

## 5. 再開条件

実行時slot名と `object+0x3C` の値を完全に確定するには、実`.ess`/セーブ実行時トレースまたは該当オブジェクトの動的メモリダンプが必要である。静的解析だけでこれ以上断定しない。
