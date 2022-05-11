#ifndef GAMETYPES_H
#define GAMETYPES_H

#include <stdint.h>
#include <stdbool.h>

#define persist static
#define internal static
#define global static

typedef uint32_t b32; // 32-bit boolean

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t usize;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intptr_t isize;

#ifdef _WIN32
#include <windef.h>
struct WindowToRendererInfo {
    HINSTANCE inst;
    HWND hwnd;
};
#endif

#endif /* GAMETYPES_H */
