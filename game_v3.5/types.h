#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define internal static
#define bool _Bool
#define true 1
#define false 0
typedef uint32_t b32;
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int16_t i16;
typedef uint64_t u64;
typedef int64_t i64;
typedef uintptr_t usize;
typedef intptr_t isize;

#ifdef _WIN32
#include <windef.h>
struct RendererWindowData {
    HINSTANCE inst;
    HWND hwnd;
};
#endif

#endif /* TYPES_H */
