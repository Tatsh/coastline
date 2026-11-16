#import <Foundation/Foundation.h>

#include "download.h"

namespace coastline {

std::optional<std::string> DownloadFile(const std::string &url,
                                        const std::filesystem::path &output,
                                        const ImportProgress &progress) {
    @autoreleasepool {
        NSURL *source = [NSURL URLWithString:@(url.c_str())];
        if (!source || ![source.scheme isEqualToString:@"https"]) {
            return "Cannot download (URL).";
        }
        NSURL *destination = [NSURL fileURLWithFileSystemRepresentation:output.c_str()
                                                            isDirectory:NO
                                                          relativeToURL:nil];
        NSURLSessionConfiguration *configuration =
            [NSURLSessionConfiguration ephemeralSessionConfiguration];
        configuration.timeoutIntervalForRequest = 60;
        NSURLSession *session = [NSURLSession sessionWithConfiguration:configuration];
        dispatch_semaphore_t done = dispatch_semaphore_create(0);
        __block std::optional<std::string> result;
        // The temporary file at location is deleted when the handler returns.
        NSURLSessionDownloadTask *task = [session
            downloadTaskWithURL:source
              completionHandler:^(NSURL *location, NSURLResponse *response, NSError *error) {
                const id http = response;
                const NSInteger status =
                    [response isKindOfClass:[NSHTTPURLResponse class]] ? [http statusCode] : 0;
                NSError *moveError = nil;
                if (error) {
                    result = std::string("Cannot download (") +
                             error.localizedDescription.UTF8String + ").";
                } else if (status != 200) {
                    result = "Cannot download (HTTP " + std::to_string(status) + ").";
                } else {
                    [[NSFileManager defaultManager] removeItemAtURL:destination error:nil];
                    if (![[NSFileManager defaultManager] moveItemAtURL:location
                                                                 toURL:destination
                                                                 error:&moveError]) {
                        result = "Cannot write " + output.string() + ".";
                    }
                }
                dispatch_semaphore_signal(done);
              }];
        [task resume];
        while (
            dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC))) {
            if (progress && !progress("Downloading...", 0)) {
                [task cancel];
                dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
                [session invalidateAndCancel];
                return "Import cancelled.";
            }
        }
        [session finishTasksAndInvalidate];
        return result;
    }
}

} // namespace coastline
