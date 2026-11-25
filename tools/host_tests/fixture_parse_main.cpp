// fixture_parse_main.cpp - synthetic fixture の parse ゲート（期待値の assert）。
//
// 目的: 合成フィクスチャ（自作バイト列）を **リポジトリのパーサ**に通し、期待値を
// 固定する。「フィクスチャが消えたら失敗」「期待値が変わったら失敗」を機械判定する。
//
// 位置づけ（限界）:
//   合成フィクスチャはパーサ挙動の固定のみで、**実データ互換は証明しない**。
//   実コーパス（tmp/*_oracle）のオラクルはローカル／リリース前ゲートの領分。
//
// 決定論: パスは相対・時刻/乱数を使わない。JSON は固定順・固定桁で出す。
//
// 使い方: tools/host_tests/run_fixture_parse.sh
// 終了コード: 0 ok / 1 期待値不一致 / 2 フィクスチャ読めず
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "assets/bsa_reader.h"
#include "assets/dds_loader.h"
#include "assets/nif_parser.h"

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++failures;
        std::printf("FAIL: %s\n", what.c_str());
    }
}

// --- DDS: docs/evidence/fixtures/minimal.dds --------------------------------
// 生成器: tools/host_tests/make_fixture_dds.py
// レイアウトの出所: app/src/main/cpp/tests/dds_upload_tests.cpp:27-57
//
// 第 1 引数でパスを差し替えられる（陰性対照で壊した fixture を食わせるため）。
void check_dds(const std::string& path) {
    DDSLoader loader;
    if (!loader.loadFile(path)) {
        std::printf("FAIL: DDS を読めない: %s\n", path.c_str());
        ++failures;
        std::printf("dds: {\"path\":\"%s\",\"loaded\":false}\n", path.c_str());
        return;
    }
    const DDSTexture& tex = loader.getTexture();
    // 実パース結果を **先に** 出す（不一致時に「パーサのバグ」か「フィクスチャの誤り」かを
    // 出力だけで切り分けられるようにする）。
    std::printf("dds: {\"path\":\"%s\",\"loaded\":true,\"width\":%u,\"height\":%u,"
                "\"mips\":%u,\"format\":\"DXT1\",\"bytes\":%zu}\n",
                path.c_str(), tex.width, tex.height, tex.mipmapCount, tex.compressedData.size());
    std::printf("dds expected: {\"width\":16,\"height\":16,\"mips\":1,\"format\":\"DXT1\","
                "\"bytes\":128}\n");
    check(tex.width == 16u, "dds width == 16");
    check(tex.height == 16u, "dds height == 16");
    check(tex.mipmapCount == 1u, "dds mipmapCount == 1");
    check(tex.compressionFormat == DDSCompressionFormat::DXT1, "dds format == DXT1");
    check(tex.compressedData.size() == 128u, "dds compressed bytes == 128");
}

// --- NIF: docs/evidence/fixtures/minimal.nif --------------------------------
// 生成器: tools/host_tests/make_fixture_nif.py
// レイアウトの出所: rust/crates/nif-core/src/lib.rs の受理済みテスト
//   shape_data_decodes_vertices_colors_uvs_and_triangles
//   （Gamebryo 20.0.0.5 / NiTriShape + NiTriShapeData）。
// 第 2 引数でパスを差し替えられる（陰性対照で壊した fixture を食わせるため）。
void check_nif(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    if (bytes.empty()) {
        std::printf("FAIL: NIF を読めない: %s\n", path.c_str());
        ++failures;
        std::printf("nif: {\"path\":\"%s\",\"loaded\":false}\n", path.c_str());
        return;
    }
    NIFParser parser;
    const bool ok = parser.parseFromData(bytes);
    const std::vector<NIFGeometry> geom =
        ok ? parser.extractAllGeometry() : std::vector<NIFGeometry>();
    const std::size_t verts = geom.empty() ? 0 : geom[0].vertices.size();
    const std::size_t tris = geom.empty() ? 0 : geom[0].triangles.size();
    std::printf("nif: {\"path\":\"%s\",\"loaded\":%s,\"blocks\":%u,\"hasNiTriShape\":%d,"
                "\"hasNiTriShapeData\":%d,\"geometry\":%zu,\"vertices\":%zu,\"triangles\":%zu}\n",
                path.c_str(), ok ? "true" : "false", parser.getBlockCount(),
                parser.hasBlockType("NiTriShape") ? 1 : 0,
                parser.hasBlockType("NiTriShapeData") ? 1 : 0,
                geom.size(), verts, tris);
    std::printf("nif expected: {\"blocks\":2,\"hasNiTriShape\":1,\"hasNiTriShapeData\":1,"
                "\"geometry\":1,\"vertices\":3,\"triangles\":1}\n");
    check(ok, "nif parseFromData");
    check(parser.getBlockCount() == 2u, "nif blockCount == 2");
    check(parser.hasBlockType("NiTriShape"), "nif has NiTriShape");
    check(parser.hasBlockType("NiTriShapeData"), "nif has NiTriShapeData");
    check(geom.size() == 1u, "nif geometry count == 1");
    check(verts == 3u, "nif vertices == 3");
    check(tris == 1u, "nif triangles == 1");
}

// --- BSA -> NIF: docs/evidence/fixtures/minimal.bsa -------------------------
// 生成器: tools/host_tests/make_fixture_bsa.py
// レイアウトの出所: rust/crates/bsa-core の {format.rs,hash.rs} と
//   tests/parse.rs の build()。中身は minimal.nif（meshes\\test.nif）。
// 第 3 引数でパスを差し替えられる（陰性対照用）。
void check_bsa(const std::string& path) {
    BSArchive archive;
    const bool opened = archive.open(path);
    const BSAFileEntry* entry = opened ? archive.findFile("meshes\\test.nif") : nullptr;
    std::vector<uint8_t> data;
    const bool extracted = (entry != nullptr) && archive.extractFileDecompressed(*entry, data);
    NIFParser parser;
    const bool nif_ok = extracted && parser.parseFromData(data);
    const std::vector<NIFGeometry> geom =
        nif_ok ? parser.extractAllGeometry() : std::vector<NIFGeometry>();
    const std::size_t verts = geom.empty() ? 0 : geom[0].vertices.size();
    std::printf("bsa: {\"path\":\"%s\",\"opened\":%s,\"files\":%u,\"found\":%d,\"extracted\":%s,"
                "\"nifBlocks\":%u,\"vertices\":%zu}\n",
                path.c_str(), opened ? "true" : "false", archive.getFileCount(),
                entry ? 1 : 0, extracted ? "true" : "false",
                nif_ok ? parser.getBlockCount() : 0u, verts);
    std::printf("bsa expected: {\"opened\":true,\"files\":1,\"found\":1,\"extracted\":true,"
                "\"nifBlocks\":2,\"vertices\":3}\n");
    check(opened, "bsa open");
    check(archive.getFileCount() == 1u, "bsa fileCount == 1");
    check(entry != nullptr, "bsa find meshes\\test.nif");
    check(extracted, "bsa extract meshes\\test.nif");
    check(nif_ok, "bsa -> nif parse");
    check(nif_ok && parser.getBlockCount() == 2u, "bsa -> nif blockCount == 2");
    check(verts == 3u, "bsa -> nif vertices == 3");
}

}  // namespace

int main(int argc, char** argv) {
    // 相対パス（repo root から実行する。run_fixture_parse.sh が cd する）。
    const std::string dds_path = (argc > 1) ? argv[1] : "docs/evidence/fixtures/minimal.dds";
    const std::string nif_path = (argc > 2) ? argv[2] : "docs/evidence/fixtures/minimal.nif";
    const std::string bsa_path = (argc > 3) ? argv[3] : "docs/evidence/fixtures/minimal.bsa";
    check_dds(dds_path);
    check_nif(nif_path);
    check_bsa(bsa_path);

    if (failures != 0) {
        std::printf("fixture_parse: FAIL (%d)\n", failures);
        return 1;
    }
    std::printf("fixture_parse: ok\n");
    return 0;
}
