#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "assets.h"
#include "logging.h"
#include "rom.h"

namespace {

constexpr int kUsageErrorStatus = 2;

} // namespace

int main(int argc, char *argv[]) {
    using namespace Tools::ExtractRom;
    argparse::ArgumentParser parser("extract-rom", "", argparse::default_arguments::help);
    parser.add_description("Write every asset of a Top Gear Rally cartridge image to a directory.");
    parser.add_argument("input_rom").help("Cartridge image to read, in .z64, .v64, or .n64 order.");
    parser.add_argument("output_directory").help("Directory to write. It is created when missing.");
    parser.add_argument("--region")
        .metavar("REGION")
        .choices("us", "jp", "eu")
        .help("Fail unless the image is this release.");
    parser.add_argument("--debug").help("Enable debug logging.").flag();
    // argparse reports usage errors only by throwing.
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << parser.usage() << "\nextract-rom: error: " << e.what() << '\n';
        return kUsageErrorStatus;
    }
    Tools::Common::setupLogging(parser.get<bool>("--debug"));
    const auto rom = Rom::open(parser.get<std::string>("input_rom"));
    if (!rom) {
        spdlog::error("Extraction failed. {}", rom.error().message);
        return EXIT_FAILURE;
    }
    if (const auto region = parser.present("--region");
        region && *region != regionName(rom->region)) {
        spdlog::error(
            "The image is the {} release, not the {} release.", regionName(rom->region), *region);
        return EXIT_FAILURE;
    }
    const auto assets = scan(*rom);
    if (assets.empty() || !assets.front().compressed) {
        spdlog::error("No compressed asset starts the asset area at {:#x}.", rom->assetStart);
        return EXIT_FAILURE;
    }
    const std::filesystem::path output = parser.get<std::string>("output_directory");
    if (const auto extracted = extract(*rom, assets, output); !extracted) {
        spdlog::error("Extraction failed. {}", extracted.error().message);
        return EXIT_FAILURE;
    }
    std::size_t compressed = 0;
    for (const auto &asset : assets) {
        compressed += asset.compressed;
    }
    std::cout << std::format("Extracted {} compressed and {} raw assets of the {} release to {}.\n",
                             compressed,
                             assets.size() - compressed,
                             regionName(rom->region),
                             output.string());
    return EXIT_SUCCESS;
}
