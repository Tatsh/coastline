#import <Foundation/Foundation.h>

#include "paths.h"

namespace coastline {

namespace {

std::optional<std::filesystem::path> LibraryFolder(NSSearchPathDirectory directory) {
    @autoreleasepool {
        NSArray<NSURL *> *urls = [[NSFileManager defaultManager] URLsForDirectory:directory
                                                                        inDomains:NSUserDomainMask];
        if (urls.count == 0) {
            return std::nullopt;
        }
        return std::filesystem::path(urls.firstObject.fileSystemRepresentation);
    }
}

} // namespace

std::optional<GamePaths> ResolveApplePaths(const std::string &application) {
    auto support = LibraryFolder(NSApplicationSupportDirectory);
    auto caches = LibraryFolder(NSCachesDirectory);
    auto library = LibraryFolder(NSLibraryDirectory);
    if (!support || !caches || !library) {
        return std::nullopt;
    }
    return GamePaths{*support / application,
                     *support / application / "data",
                     *library / "Logs" / application,
                     *caches / application};
}

} // namespace coastline
