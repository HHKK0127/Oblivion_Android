# V46: God Nodes の `calls` 辺のみ度数再計算と型ノード汚染ポリシー

作成日: 2026-10-24 / 対象グラフ: `graphify-out/graph.json` (46,648 nodes / 120,608 edges / undirected / node-link, edges='links')
関係分布: calls=40058, references=25842, contains=23779, method=10215, defines=10180, imports=7873, case_of=1999, inherits=549, rationale_for=80, imports_from=26, implements=6, indirect_call=1

## 0. 再現手順とベースラインの定義

```
# interpreter: C:\Users\hiroki.kogarumai\AppData\Roaming\uv\tools\graphifyy\Scripts\python.exe
# G = networkx.readwrite.json_graph.node_link_graph(json.load(open(graph.json)), edges='links')
# graphify.analyze.god_nodes(G, top_n=3) = [('FUN_00401f20()', 1684), ('FUN_00401f00()', 1300), ('uint', 1199)]
# 生成スクリプト: %TEMP%\opencode\v46_god_calls_only.py
```

本レポートでの用語:
- **生全辺 (raw)**: `G.degree()` 全辺度数。ノードフィルタなし。
- **現行運用 (operational)**: `graphify.analyze.god_nodes(G)` の出力。内部で (i) ファイル容器ノード/`.method()` スタブ
  (`_is_file_node`)、(ii) source_file 欠落の concept ノード (`_is_concept_node`)、(iii) JSON キー、(iv) `_BUILTIN_NOISE_LABELS`
  （Swift/Python 組み込み名: Int/Float/String/List 等）を除外した全辺度数ランキング。V45 の「現行 God Nodes」はこれに対応し、
  `undefined4`(生全辺 4,682) や `log`/`vector`/`string`/`Oblivion_decompiled.c` が現行 top に入らないのは同フィルタの所致。

## 1. `calls` 辺のみ度数ランキング top 25

全ノードを対象に `relation == 'calls'` の辺のみで度数を集計（無向として両端に加算）。

| rank | calls_deg | label | source_file | location | community |
|---:|---:|---|---|---|---|
| 1 | 1683 | `FUN_00401f20()` | docs/Oblivion_decompiled.c | L1124 | 0 |
| 2 | 1298 | `FUN_00401f00()` | docs/Oblivion_decompiled.c | L1113 | 0 |
| 3 | 493 | `__security_check_cookie()` | docs/Oblivion_decompiled.c | L982500 | 4 |
| 4 | 318 | `FUN_0058ceb0()` | docs/Oblivion_decompiled.c | L297182 | 4 |
| 5 | 300 | `FUN_00588bd0()` | docs/Oblivion_decompiled.c | L293746 | 4 |
| 6 | 285 | `FUN_00582160()` | docs/Oblivion_decompiled.c | L288675 | 4 |
| 7 | 225 | `FUN_006ecc80()` | docs/Oblivion_decompiled.c | L525595 | 2 |
| 8 | 215 | `FUN_00589b70()` | docs/Oblivion_decompiled.c | L294976 | 4 |
| 9 | 211 | `FUN_005898f0()` | docs/Oblivion_decompiled.c | L294790 | 4 |
| 10 | 210 | `FUN_0041e210()` | docs/Oblivion_decompiled.c | L20071 | 8 |
| 11 | 195 | `FUN_004028d0()` | docs/Oblivion_decompiled.c | L1908 | 4 |
| 12 | 188 | `FUN_0046b250()` | docs/Oblivion_decompiled.c | L84743 | 60 |
| 13 | 163 | `RVec3` | app/src/main/cpp/third_party/JoltPhysics/Jolt/Math/Real.h | L17 | 5 |
| 14 | 155 | `FUN_0058ced0()` | docs/Oblivion_decompiled.c | L297198 | 4 |
| 15 | 150 | `FUN_004fae80()` | docs/Oblivion_decompiled.c | L194577 | 10 |
| 16 | 142 | `.makeInt()` | app/src/main/cpp/script/script_opcodes.h | L42 | 28 |
| 17 | 141 | `FUN_0055e000()` | docs/Oblivion_decompiled.c | L259922 | 19 |
| 18 | 134 | `FUN_00446cb0()` | docs/Oblivion_decompiled.c | L54233 | 8 |
| 19 | 132 | `FUN_0042bb90()` | docs/Oblivion_decompiled.c | L32362 | 54 |
| 20 | 130 | `FUN_00579b60()` | docs/Oblivion_decompiled.c | L281901 | 10 |
| 21 | 129 | `FUN_004e4a10()` | docs/Oblivion_decompiled.c | L179196 | 54 |
| 22 | 127 | `FUN_004c97f0()` | docs/Oblivion_decompiled.c | L157645 | 2 |
| 23 | 116 | `FUN_00707370()` | docs/Oblivion_decompiled.c | L546640 | 6 |
| 24 | 108 | `.drawColoredQuad()` | app/src/main/cpp/ui/ui_draw_helper.h | L27 | 48 |
| 25 | 107 | `FUN_0065c620()` | docs/Oblivion_decompiled.c | L421629 | 8 |

観察: top25 中22件が `docs/Oblivion_decompiled.c` 由来（FUN_ ラップ関数 + CRT 系 `__security_check_cookie()`）。
port 側 C++ エンティティは `RVec3`(163, 13位) / `.makeInt()`(142, 16位) / `.drawColoredQuad()`(108, 24位) の3件のみ。
なお `.makeInt()` / `.drawColoredQuad()` は現行 `god_nodes()` では `.method()` スタブ規則により除外されるため、
`calls`-only 観点でのみ現れる。

## 2. 関係別トップ 10

### 2.1 `calls` top 10

| rank | degree | label | source_file | community |
|---:|---:|---|---|---|
| 1 | 1683 | `FUN_00401f20()` | docs/Oblivion_decompiled.c | 0 |
| 2 | 1298 | `FUN_00401f00()` | docs/Oblivion_decompiled.c | 0 |
| 3 | 493 | `__security_check_cookie()` | docs/Oblivion_decompiled.c | 4 |
| 4 | 318 | `FUN_0058ceb0()` | docs/Oblivion_decompiled.c | 4 |
| 5 | 300 | `FUN_00588bd0()` | docs/Oblivion_decompiled.c | 4 |
| 6 | 285 | `FUN_00582160()` | docs/Oblivion_decompiled.c | 4 |
| 7 | 225 | `FUN_006ecc80()` | docs/Oblivion_decompiled.c | 2 |
| 8 | 215 | `FUN_00589b70()` | docs/Oblivion_decompiled.c | 4 |
| 9 | 211 | `FUN_005898f0()` | docs/Oblivion_decompiled.c | 4 |
| 10 | 210 | `FUN_0041e210()` | docs/Oblivion_decompiled.c | 8 |

### 2.2 `defines` top 10

| rank | degree | label | source_file | community |
|---:|---:|---|---|---|
| 1 | 179 | `GameSystemRefs` | app/src/main/cpp/ui/game_console.h | 30 |
| 2 | 128 | `TitleScreen` | app/src/main/cpp/ui/title_screen.h | 23 |
| 3 | 127 | `Renderer` | app/src/main/cpp/engine/renderer.h | 16 |
| 4 | 74 | `ESMFile` | app/src/main/cpp/assets/esm_reader.h | 91 |
| 5 | 71 | `RendererVK` | app/src/main/cpp/third_party/JoltPhysics/TestFramework/Renderer/VK/RendererVK.h | 24 |
| 6 | 69 | `UIContainerMenu` | app/src/main/cpp/ui/ui_container_menu.h | 32 |
| 7 | 60 | `SamplesApp` | app/src/main/cpp/third_party/JoltPhysics/Samples/SamplesApp.h | 117 |
| 8 | 60 | `ESMManager` | app/src/main/cpp/assets/esm_reader.h | 55 |
| 9 | 54 | `FaceShape` | app/src/main/cpp/character/face_gen_morpher.h | 38 |
| 10 | 53 | `Step` | app/src/main/cpp/third_party/JoltPhysics/Jolt/Physics/PhysicsUpdateContext.h | 151 |

### 2.3 `contains` top 10

| rank | degree | label | source_file | community |
|---:|---:|---|---|---|
| 1 | 9842 | `Oblivion_decompiled.c` | docs/Oblivion_decompiled.c | 1 |
| 2 | 219 | `stb_image.h` | app/src/main/cpp/include/stb_image.h | 97 |
| 3 | 209 | `doctest.h` | app/src/main/cpp/third_party/JoltPhysics/UnitTests/doctest.h | 65 |
| 4 | 168 | `script_functions.cpp` | app/src/main/cpp/script/script_functions.cpp | 28 |
| 5 | 163 | `esm_reader.cpp` | app/src/main/cpp/assets/esm_reader.cpp | 96 |
| 6 | 156 | `stb_truetype.h` | app/src/main/cpp/include/stb_truetype.h | 238 |
| 7 | 115 | `nif_parser.cpp` | app/src/main/cpp/assets/nif_parser.cpp | 42 |
| 8 | 112 | `stb_vorbis.c` | app/src/main/cpp/third_party/stb/stb_vorbis.c | 109 |
| 9 | 100 | `game_console.cpp` | app/src/main/cpp/ui/game_console.cpp | 83 |
| 10 | 90 | `renderer.cpp` | app/src/main/cpp/engine/renderer.cpp | 16 |

### 2.4 `references` top 10

| rank | degree | label | source_file | community |
|---:|---:|---|---|---|
| 1 | 4682 | `undefined4` | (なし) | 0 |
| 2 | 1185 | `uint` | app/src/main/cpp/third_party/JoltPhysics/Jolt/Core/Core.h | 1 |
| 3 | 870 | `vec3` | app/src/main/cpp/include/glm/glm.hpp | 5 |
| 4 | 601 | `byte` | (なし) | 0 |
| 5 | 548 | `Vec3Arg` | app/src/main/cpp/third_party/JoltPhysics/Jolt/Math/MathTypes.h | 3 |
| 6 | 422 | `undefined1` | (なし) | 10 |
| 7 | 335 | `float10` | (なし) | 15 |
| 8 | 272 | `BodyID()` | app/src/main/cpp/third_party/JoltPhysics/Jolt/Physics/Body/BodyID.h | 9 |
| 9 | 235 | `Mat44` | app/src/main/cpp/third_party/JoltPhysics/Jolt/Math/MathTypes.h | 13 |
| 10 | 184 | `ScriptValue` | app/src/main/cpp/script/script_opcodes.h | 58 |

観察: `defines` は port 側クラス定義ハブ（`GameSystemRefs` 179 / `TitleScreen` 128 / `Renderer` 127）、
`contains` はファイル容器ノード（`Oblivion_decompiled.c` 9,842 を筆頭）、`references` は V45 で確定した型/typedef ハブ
（`undefined4` 4,682 / `uint` 1,185 / `vec3` 870 …）と完全一致。

## 3. 現行 God（`god_nodes()` top20）と `calls`-only top20 の差分

| # | 現行God ラベル | 全辺deg | calls_deg | calls-only順位 |
|---:|---|---:|---:|---:|
| 1 | `FUN_00401f20()` | 1684 | 1683 | 1 |
| 2 | `FUN_00401f00()` | 1300 | 1298 | 2 |
| 3 | `uint` | 1199 | 13 | 996 |
| 4 | `vec3` | 881 | 0 | —(top100外) |
| 5 | `Vec3Arg` | 549 | 0 | —(top100外) |
| 6 | `__security_check_cookie()` | 494 | 493 | 3 |
| 7 | `Renderer` | 379 | 0 | —(top100外) |
| 8 | `FUN_0058ceb0()` | 320 | 318 | 4 |
| 9 | `FUN_00588bd0()` | 303 | 300 | 5 |
| 10 | `BodyID()` | 294 | 0 | —(top100外) |
| 11 | `FUN_00582160()` | 287 | 285 | 6 |
| 12 | `RVec3` | 273 | 163 | 13 |
| 13 | `Mat44` | 247 | 4 | 4431 |
| 14 | `FUN_006ecc80()` | 227 | 225 | 7 |
| 15 | `ESMFile` | 226 | 0 | —(top100外) |
| 16 | `FUN_00589b70()` | 217 | 215 | 8 |
| 17 | `ExecutionContext` | 215 | 0 | —(top100外) |
| 18 | `FUN_005898f0()` | 212 | 211 | 9 |
| 19 | `FUN_0041e210()` | 212 | 210 | 10 |
| 20 | `CharacterVirtual` | 209 | 0 | —(top100外) |

| # | calls-only top20 ラベル | calls_deg | 全辺deg | 現行God順位 |
|---:|---|---:|---:|---:|
| 1 | `FUN_00401f20()` | 1683 | 1684 | 1 |
| 2 | `FUN_00401f00()` | 1298 | 1300 | 2 |
| 3 | `__security_check_cookie()` | 493 | 494 | 6 |
| 4 | `FUN_0058ceb0()` | 318 | 320 | 8 |
| 5 | `FUN_00588bd0()` | 300 | 303 | 9 |
| 6 | `FUN_00582160()` | 285 | 287 | 11 |
| 7 | `FUN_006ecc80()` | 225 | 227 | 14 |
| 8 | `FUN_00589b70()` | 215 | 217 | 16 |
| 9 | `FUN_005898f0()` | 211 | 212 | 18 |
| 10 | `FUN_0041e210()` | 210 | 212 | 19 |
| 11 | `FUN_004028d0()` | 195 | 198 | 23 |
| 12 | `FUN_0046b250()` | 188 | 191 | 24 |
| 13 | `RVec3` | 163 | 273 | 12 |
| 14 | `FUN_0058ced0()` | 155 | 157 | —(top25外) |
| 15 | `FUN_004fae80()` | 150 | 153 | —(top25外) |
| 16 | `.makeInt()` | 142 | 143 | —(top25外) |
| 17 | `FUN_0055e000()` | 141 | 144 | —(top25外) |
| 18 | `FUN_00446cb0()` | 134 | 135 | —(top25外) |
| 19 | `FUN_0042bb90()` | 132 | 134 | —(top25外) |
| 20 | `FUN_00579b60()` | 130 | 132 | —(top25外) |

- 現行God top20 にあって `calls`-only top20 に無い: 9 件 = `uint`(1199/calls 13) `vec3`(881/calls 0) `Vec3Arg`(549/calls 0) `Renderer`(379/calls 0) `BodyID()`(294/calls 0) `Mat44`(247/calls 4) `ESMFile`(226/calls 0) `ExecutionContext`(215/calls 0) `CharacterVirtual`(209/calls 0)
- `calls`-only top20 にあって現行God top20 に無い: 9 件 = `FUN_004028d0()`(calls 195) `FUN_0046b250()`(calls 188) `FUN_0058ced0()`(calls 155) `FUN_004fae80()`(calls 150) `.makeInt()`(calls 142) `FUN_0055e000()`(calls 141) `FUN_00446cb0()`(calls 134) `FUN_0042bb90()`(calls 132) `FUN_00579b60()`(calls 130)

要約: 現行God側の脱落9件の内3件は typedef ハブ（`uint`/`vec3`/`Vec3Arg`、calls 度 13/0/0）、6件は calls 辺をほぼ持たない実クラス
（`Renderer`/`BodyID()`/`Mat44`/`ESMFile`/`ExecutionContext`/`CharacterVirtual`）。calls-only 側の新規9件は FUN_ ラップ関数8件 + `.makeInt()`。

## 4. 型/Ghidra ノード識別ポリシー（再現可能なルール）

対象: god リスト運用上「度数は高いがコールグラフ上の hub ではない」ノードを機械的に除外する。
V45 の誤検出例 `Renderer`（実クラス, engine/renderer.h）・`BodyID()`（実クラス, Jolt/Physics/BodyID.h）は**必ず残す**こと。

### 4.1 ルール定義（生全辺に対する完結ルール）

```
node n について deg=全辺度数, calls=calls度数, refs=references度数, sf=source_file

R1 (Ghidra プリミティブ型名): label 精確一致 ∈ {undefined, undefined0..16, float10/12/80/128,
     longdouble, byte, char, uchar, short, ushort, int, uint, long, ulong, longlong,
     ulonglong, dword, qword, code, bool, float, double, longlong8, ulonglong8, wchar, wchar_t}
     -> 除外（sf/calls 不問。`uint` は Jolt/Core/Core.h の typedef だが R1 で除外）

R2 (出所欠落ハブ): sf が空 かつ calls == 0 かつ deg >= 50 かつ label が PascalCase でない
     -> 除外（STL 名 log/vector/string/algorithm/cmath/cstdint/memory/unordered_map、
        Ghidra 由来 lowercase シンボル boxshape/layers/gl3/jobject 等）
     PascalCase 例外により BodyCreationSettings / DebugRenderer / StreamOut / JNIEnv 等の実体は残す

R3 (参照専用 typedef ハブ): sf あり かつ calls == 0 かつ deg >= 300 かつ refs/deg >= 0.95
     -> 除外（該当は vec3(glm.hpp L28) / Vec3Arg(Jolt MathTypes.h) の2件のみ）
     BodyID() は refs/deg = 0.925 < 0.95 で残留、Renderer は refs比 0.251 で残留

R4 (任意拡張: 文件容器ハブ): label が .c/.cpp/.h/.hpp/.cc/.cxx で終わる かつ deg >= 50 かつ
     contains/deg >= 0.90 -> 除外（型ポリシーとは別カテゴリ。任意フラグ）
```

### 4.2 除去件数（実測, 生全辺基準）

| rule | マッチ数 | raw top100 内 | 代表例 |
|---|---:|---:|---|
| R1 | 14 | 5 | `undefined4`(4682), `uint`(1199), `byte`(601), `undefined1`(422), `float10`(335), `ushort`(99), `undefined2`(54), `undefined8`(44) |
| R2 | 26 | 15 | `undefined4`(4682), `byte`(601), `undefined1`(422), `float10`(335), `log`(257), `vector`(246), `string`(233), `algorithm`(165) |
| R3 | 2 | 2 | `vec3`(881), `Vec3Arg`(549) |
| R4 (任意) | 13 | 9 | `Oblivion_decompiled.c`(9842), `stb_image.h`(234), `doctest.h`(214), `script_functions.cpp`(183), `esm_reader.cpp`(174) |

- コアポリシー (R1|R2|R3) の除去ノード数: **36** （全 46648 ノード中の 0.077%）
- raw top100 に含まれる除去ノード: **18** 件
- 除去ノードの度数合計: 11311 / 全度数 241216 （4.69%）
- R4 を含めた除去ノード数: 49

**現行運用 `god_nodes()` との関係**: `god_nodes` の `_is_concept_node`（sf 欠落を除外）が R2 相当を既にカバーし、
R1 のうち sf ありで残る唯一ノードが `uint`。したがって現行 top20 に対して本ポリシーが追加で除去するのは
**3 件 = `uint`, `vec3`, `Vec3Arg`**（R1: uint / R3: vec3, Vec3Arg）。
R2 は生全辺を直接扱う場合（本ツール群外の分析）にのみ追加効果を持つ。

### 4.3 誤検出チェック（保持が確認された実体ノード）

| label | 全辺deg | calls_deg | 判定 | 保持根拠 |
|---|---:|---:|---|---|
| `Renderer` | 379 | 0 | **保持** | R1非該当, sf有, refs比0.251<0.95 |
| `BodyID()` | 294 | 0 | **保持** | R1非該当, sf有, refs比0.925<0.95 |
| `RVec3` | 273 | 163 | **保持** | R1非該当, sf有, refs比0.399<0.95 |
| `Mat44` | 247 | 4 | **保持** | R1非該当, sf有, calls=4>0 |
| `ESMFile` | 226 | 0 | **保持** | R1非該当, sf有, refs比0.292<0.95 |
| `CharacterVirtual` | 209 | 0 | **保持** | R1非該当, sf有, refs比0.469<0.95 |
| `TitleScreen` | 208 | 0 | **保持** | R1非該当, sf有, refs比0.101<0.95 |
| `BodyCreationSettings` | 135 | 0 | **保持** | R1非該当, PascalCaseでR2例外, refs比0.007<0.95 |
| `DebugRenderer` | 68 | 0 | **保持** | R1非該当, sf有, refs比0.074<0.95 |
| `StreamOut` | 57 | 0 | **保持** | R1非該当, PascalCaseでR2例外, refs比0.000<0.95 |
| `JNIEnv` | 54 | 0 | **保持** | R1非該当, PascalCaseでR2例外 |
| `ScriptValue` | 198 | 0 | **保持** | R1非該当, sf有, refs比0.929<0.95 |

### 4.4 適用後 top20（推奨運用 (c) = 現行 `god_nodes()` フィルタ + R1|R2|R3）

| rank | deg | label | community | source_file |
|---:|---:|---|---|---|
| 1 | 1684 | `FUN_00401f20()` | 0 | docs/Oblivion_decompiled.c |
| 2 | 1300 | `FUN_00401f00()` | 0 | docs/Oblivion_decompiled.c |
| 3 | 494 | `__security_check_cookie()` | 4 | docs/Oblivion_decompiled.c |
| 4 | 379 | `Renderer` | 16 | app/src/main/cpp/engine/renderer.h |
| 5 | 320 | `FUN_0058ceb0()` | 4 | docs/Oblivion_decompiled.c |
| 6 | 303 | `FUN_00588bd0()` | 4 | docs/Oblivion_decompiled.c |
| 7 | 294 | `BodyID()` | 9 | app/src/main/cpp/third_party/JoltPhysics/Jolt/Physics/Body/BodyID.h |
| 8 | 287 | `FUN_00582160()` | 4 | docs/Oblivion_decompiled.c |
| 9 | 273 | `RVec3` | 5 | app/src/main/cpp/third_party/JoltPhysics/Jolt/Math/Real.h |
| 10 | 247 | `Mat44` | 13 | app/src/main/cpp/third_party/JoltPhysics/Jolt/Math/MathTypes.h |
| 11 | 227 | `FUN_006ecc80()` | 2 | docs/Oblivion_decompiled.c |
| 12 | 226 | `ESMFile` | 91 | app/src/main/cpp/assets/esm_reader.h |
| 13 | 217 | `FUN_00589b70()` | 4 | docs/Oblivion_decompiled.c |
| 14 | 215 | `ExecutionContext` | 28 | app/src/main/cpp/script/script_context.h |
| 15 | 212 | `FUN_005898f0()` | 4 | docs/Oblivion_decompiled.c |
| 16 | 212 | `FUN_0041e210()` | 8 | docs/Oblivion_decompiled.c |
| 17 | 209 | `CharacterVirtual` | 57 | app/src/main/cpp/third_party/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h |
| 18 | 208 | `TitleScreen` | 23 | app/src/main/cpp/ui/title_screen.h |
| 19 | 198 | `ScriptValue` | 58 | app/src/main/cpp/script/script_opcodes.h |
| 20 | 198 | `FUN_004028d0()` | 4 | docs/Oblivion_decompiled.c |

構成: FUN_ ハブ 10 件 + CRT 1 件 + 実クラス 9 件（typedef ハブ 0 件）。

### 4.5 参考: 生全辺に対する R1|R2|R3|R4 全適用後 top20

| rank | deg | label | community | source_file |
|---:|---:|---|---|---|
| 1 | 1684 | `FUN_00401f20()` | 0 | docs/Oblivion_decompiled.c |
| 2 | 1300 | `FUN_00401f00()` | 0 | docs/Oblivion_decompiled.c |
| 3 | 494 | `__security_check_cookie()` | 4 | docs/Oblivion_decompiled.c |
| 4 | 379 | `Renderer` | 16 | app/src/main/cpp/engine/renderer.h |
| 5 | 320 | `FUN_0058ceb0()` | 4 | docs/Oblivion_decompiled.c |
| 6 | 303 | `FUN_00588bd0()` | 4 | docs/Oblivion_decompiled.c |
| 7 | 294 | `BodyID()` | 9 | app/src/main/cpp/third_party/JoltPhysics/Jolt/Physics/Body/BodyID.h |
| 8 | 287 | `FUN_00582160()` | 4 | docs/Oblivion_decompiled.c |
| 9 | 273 | `RVec3` | 5 | app/src/main/cpp/third_party/JoltPhysics/Jolt/Math/Real.h |
| 10 | 247 | `Mat44` | 13 | app/src/main/cpp/third_party/JoltPhysics/Jolt/Math/MathTypes.h |
| 11 | 227 | `FUN_006ecc80()` | 2 | docs/Oblivion_decompiled.c |
| 12 | 226 | `ESMFile` | 91 | app/src/main/cpp/assets/esm_reader.h |
| 13 | 217 | `FUN_00589b70()` | 4 | docs/Oblivion_decompiled.c |
| 14 | 215 | `ExecutionContext` | 28 | app/src/main/cpp/script/script_context.h |
| 15 | 212 | `FUN_0041e210()` | 8 | docs/Oblivion_decompiled.c |
| 16 | 212 | `FUN_005898f0()` | 4 | docs/Oblivion_decompiled.c |
| 17 | 209 | `CharacterVirtual` | 57 | app/src/main/cpp/third_party/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h |
| 18 | 208 | `TitleScreen` | 23 | app/src/main/cpp/ui/title_screen.h |
| 19 | 198 | `ScriptValue` | 58 | app/src/main/cpp/script/script_opcodes.h |
| 20 | 198 | `FUN_004028d0()` | 4 | docs/Oblivion_decompiled.c |

## 5. comm14 / comm253 (および comm7) の `calls` 保有率と分割判定

V45 レポートは comm14 を `Jolt.h`、comm253 を `layers` として分割推奨（コヒージョン 0.002〜0.008）していた。
`calls` 辺保有率（community 内部辺のうち calls の比率 / calls 辺を持つノード比率）で再評価する。

| community | nodes | internal edges | calls edges | calls辺比率 | calls有ノード | ノード保有率 | 最多ソース |
|---|---:|---:|---:|---:|---:|---:|---|
| comm14 | 352 | 533 | 41 | 0.0769 | 54 | 0.1534 | <none>×80 |
| comm253 | 44 | 77 | 5 | 0.0649 | 8 | 0.1818 | ui_system.h×33 |
| comm7 | 653 | 1655 | 9 | 0.0054 | 10 | 0.0153 | <none>×216 |

- comm14 内部関係内訳: contains=145, imports=137, references=117, method=59, calls=41, defines=30, inherits=2, case_of=2
- comm14 最多ソース4件: <none>×80, TaperedCapsuleShape.h×24, LinearCurve.h×15, AngleConstraintPart.h×14
- comm253 内部関係内訳: method=23, references=21, contains=20, defines=8, calls=5
- comm253 最多ソース4件: ui_system.h×33, <none>×9, ui_system.cpp×2
- comm7 内部関係内訳: imports=1353, contains=182, references=51, defines=29, method=26, calls=9, case_of=3, inherits=2
- comm7 最多ソース4件: <none>×216, ConvexHullShape.h×23, PhysicsScene.h×21, CastShapeTests.cpp×16

ノード帰属の注記: ラベル `layers` は2系統存在する。
① `app_src_main_cpp_ui_ui_system_uisystem_layers`（comm253, deg=1, ui_system.h の UISystem メンバ）— comm253=`layers` 命名の根拠。
② `layers`（comm7, deg=138, sf なし）— Jolt Samples 系 community 所属の別ノード。V45 の comm253(`layers`) 表記は①と整合。

**分割判定（数値のみ）**: comm14 は内部辺の calls 比 0.0769（41/533）・calls 有ノード比 0.1534（54/352）、中身は contains/imports/references 主体。
comm253 は calls 比 0.0649（5/77）・保有率 0.1818（8/44）、主軸は method/references/contains。
comm7 は calls 比 0.0054（9/1655）・保有率 0.0153（10/653）と最も深い。3 community とも calls 辺比率は 8% 未満・calls 有ノード比は 19% 未満に留まり、
V45 の分割判断は `calls`-only 観点からも**妥当**と判定される。

## 6. 結論: god リスト運用の推奨

| 方式 | top20 の実体 | 問題点 |
|---|---|---|
| (a) 現行 `god_nodes()`（全辺+標準フィルタ） | top20 中3件が typedef ハブ（`uint` 3位/`vec3` 4位/`Vec3Arg` 5位、calls 度 13/0/0）。実クラス7件+FUN_ハブ9件+CRT1件は正しく残る | typedef ハブが上位を占有し、calls 非参加ノードとして順位が虚高 |
| (b) calls-only | top20 中19件が `docs/Oblivion_decompiled.c` 由来（CRT/ラップ関数）。port 側クラス hub は `RVec3` のみ | `Renderer`/`BodyID()`/`ESMFile`/`CharacterVirtual` 等 calls=0 の実クラスが完全に脱落。EXE 側の呼出ハブ専用の視点になる |
| (c) (a) + R1\|R2\|R3 | top20 = FUN_ハブ 10 件 + CRT 1 件 + 実クラス 9 件。typedef ハブ0件 | `.makeInt()`/`.drawColoredQuad()` 等の `.method()` スタブは依然除外される（呼び出しハブ分析時は calls-only で補完） |

**推奨: (c) 型除外 + 全辺（= 現行 `god_nodes()` フィルタに R1|R2|R3 を追加）を第一運用とする。**

判断材料:
- 現行 top20 に対する追加除去は 3 ノードのみ（`uint`/`vec3`/`Vec3Arg`）で、実体ノードの誤検出は実測 0 件（§4.3 の12件すべて保持）。
- 生全辺基準では 36 ノード（0.077%）・度数カバレッジ 4.69% の除去で raw top100 の 18 件の汚染を一掃できる。
- (b) calls-only は「EXE 内で最も多く呼ばれる関数」の測定には正しいが、port 対象クラスの hub 探査には不適（実クラスの大半は calls 辺を持たない）。
- calls 辺を持つ port 側ノード（`RVec3` 163）は (c) top20 にも残るため、呼出ハブの主要検出力は (c) で代替可能。
- 型ノードの除去可否に全体コヒージョンは依存しない（V45 で除去前後 mean 0.214730 / median 0.142857 が不変）。

---

生成スクリプト: `%TEMP%\opencode\v46_god_calls_only.py`（本レポートは同スクリプトの実行結果そのもの）。既存ドキュメントへの編集は行っていない。
