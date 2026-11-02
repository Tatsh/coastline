#ifndef COASTLINE_ANALYZER_H
#define COASTLINE_ANALYZER_H

/**
 * Suppress analyser diagnostics on a dead store verified against original disassembly.
 * Compiler builds expand the annotation to nothing, preserving matching code generation.
 */
#ifdef __clang_analyzer__
#define COASTLINE_MATCHING_STORE [[clang::suppress]]
#else
#define COASTLINE_MATCHING_STORE
#endif

#endif
