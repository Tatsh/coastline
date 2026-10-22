#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "encoder.h"
#include "fileio.h"
#include "logging.h"

namespace {

constexpr int kUsageErrorStatus = 2;

} // namespace

int main(int argc, char *argv[]) {
    argparse::ArgumentParser parser("encode-text", "", argparse::default_arguments::help);
    parser.add_description(
        "Write a C source with its Japanese text in the glyph codes of the Japanese release.");
    parser.add_argument("input").help("C source whose string literals have UTF-8 text.");
    parser.add_argument("output").help("C source to write.");
    parser.add_argument("--debug").help("Enable debug logging.").flag();
    // argparse reports usage errors only by throwing.
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << parser.usage() << "\nencode-text: error: " << e.what() << '\n';
        return kUsageErrorStatus;
    }
    Tools::Common::setupLogging(parser.get<bool>("--debug"));
    const std::filesystem::path input = parser.get<std::string>("input");
    const auto source = Tools::Common::readFile(input);
    if (!source) {
        spdlog::error("Encoding failed. {}", source.error().message);
        return EXIT_FAILURE;
    }
    const auto encoded = Tools::EncodeText::encode(
        std::string_view(reinterpret_cast<const char *>(source->data()), source->size()));
    if (!encoded) {
        spdlog::error("Encoding {} failed. {}", input.string(), encoded.error().message);
        return EXIT_FAILURE;
    }
    const std::filesystem::path output = parser.get<std::string>("output");
    if (const auto written = Tools::Common::writeFile(output, *encoded); !written) {
        spdlog::error("Encoding failed. {}", written.error().message);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
