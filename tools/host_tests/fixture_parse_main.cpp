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
#include <string>
#include <vector>

#include "assets/dds_loader.h"

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

}  // namespace

int main(int argc, char** argv) {
    // 相対パス（repo root から実行する。run_fixture_parse.sh が cd する）。
    const std::string dds_path = (argc > 1) ? argv[1] : "docs/evidence/fixtures/minimal.dds";
    check_dds(dds_path);

    if (failures != 0) {
        std::printf("fixture_parse: FAIL (%d)\n", failures);
        return 1;
    }
    std::printf("fixture_parse: ok\n");
    return 0;
}
