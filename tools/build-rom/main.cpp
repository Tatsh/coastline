#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "fileio.h"
#include "logging.h"
#include "rombuilder.h"

namespace {

constexpr int kUsageErrorStatus = 2;

} // namespace

int main(int argc, char *argv[]) {
    using namespace Tools::BuildRom;
    argparse::ArgumentParser parser("build-rom", "", argparse::default_arguments::help);
    parser.add_description("Assemble a cartridge image from the built program and the assets.");
    parser.add_argument("boot").help("boot.bin from extract-rom, the header and the IPL3.");
    parser.add_argument("program").help("The linked program as flat bytes.");
    parser.add_argument("assets").help("Directory extract-rom wrote.");
    parser.add_argument("output").help("Cartridge image to write, in .z64 order.");
    parser.add_argument("--debug").help("Enable debug logging.").flag();
    // argparse reports usage errors only by throwing.
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << parser.usage() << "\nbuild-rom: error: " << e.what() << '\n';
        return kUsageErrorStatus;
    }
    Tools::Common::setupLogging(parser.get<bool>("--debug"));
    const auto boot = Tools::Common::readFile(parser.get<std::string>("boot"));
    const auto program = Tools::Common::readFile(parser.get<std::string>("program"));
    if (!boot || !program) {
        spdlog::error("Building failed. {}", (boot ? program : boot).error().message);
        return EXIT_FAILURE;
    }
    const auto rom = build(*boot, *program, parser.get<std::string>("assets"));
    if (!rom) {
        spdlog::error("Building failed. {}", rom.error().message);
        return EXIT_FAILURE;
    }
    const std::filesystem::path output = parser.get<std::string>("output");
    if (const auto written = Tools::Common::writeFile(output, *rom); !written) {
        spdlog::error("Building failed. {}", written.error().message);
        return EXIT_FAILURE;
    }
    std::cout << std::format("Wrote {}.\n", output.string());
    return EXIT_SUCCESS;
}
