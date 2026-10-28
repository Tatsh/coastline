/**
 * @file
 * Marks around a routine that reads a value before it is written, as the original routine does.
 */
#ifndef COASTLINE_PLATFORM_WIN_UNINITIALIZED_H
#define COASTLINE_PLATFORM_WIN_UNINITIALIZED_H

#if defined(__clang__)
#define BEGIN_ORIGINAL_UNINITIALIZED_READ                                                          \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wuninitialized\"")       \
        _Pragma("clang diagnostic ignored \"-Wsometimes-uninitialized\"")
#define END_ORIGINAL_UNINITIALIZED_READ _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define BEGIN_ORIGINAL_UNINITIALIZED_READ                                                          \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")           \
        _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#define END_ORIGINAL_UNINITIALIZED_READ _Pragma("GCC diagnostic pop")
#else
#define BEGIN_ORIGINAL_UNINITIALIZED_READ
#define END_ORIGINAL_UNINITIALIZED_READ
#endif

#endif /* COASTLINE_PLATFORM_WIN_UNINITIALIZED_H */
