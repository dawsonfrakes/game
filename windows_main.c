#include <stdint.h>

#define Assert(X) do if (!(X)) *(volatile char *) 0 = 0; while (0)
#define AlignPow2Up(X, Y) (((X) + ((Y) - 1)) & ~((Y) - 1))
#define AlignPow2Down(X, Y) ((X) & ~((Y) - 1))

// math
typedef struct Vector3 {
	float x;
	float y;
	float z;
} Vector3;

typedef struct Vector4 {
	float x;
	float y;
	float z;
	float w;
} Vector4;

static inline Vector4 Vector4New(float x, float y, float z, float w) {
	Vector4 result;
	result.x = x;
	result.y = y;
	result.z = z;
	result.w = w;
	return result;
}

// simd
#if defined(__AVX__)
#include <immintrin.h>

#define SIMD32_LANES 8

typedef __m256i simd_i32;

#define simd_set1_i32 _mm256_set1_epi32
#define simd_store_i32 _mm256_store_si256

#elif (defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64))
#include <emmintrin.h>

#define SIMD32_LANES 4

typedef __m128i simd_i32;

#define simd_set1_i32 _mm_set1_epi32
#define simd_store_i32 _mm_store_si128

#else
#define SIMD32_LANES 1

typedef uint32_t simd_i32;

static inline simd_i32 simd_set1_i32(uint32_t x) { return x; }
static inline void simd_store_i32(simd_i32 *addr, simd_i32 x) { *addr = x; }
#endif

#define SIMD32_ALIGN (sizeof(uint32_t) * SIMD32_LANES)

// @debug
#define BEGIN_TIMED_BLOCK(NAME) do { \
	debug_timed_blocks[TimedBlock_##NAME].cycles -= __rdtsc(); \
	debug_timed_blocks[TimedBlock_##NAME].parity += 1; \
} while (0)
#define END_TIMED_BLOCK(NAME) do { \
	debug_timed_blocks[TimedBlock_##NAME].function_name = __func__; \
	debug_timed_blocks[TimedBlock_##NAME].cycles += __rdtsc(); \
	debug_timed_blocks[TimedBlock_##NAME].hit_count += 1; \
	debug_timed_blocks[TimedBlock_##NAME].parity -= 1; \
} while (0)
enum TimedBlockNames {
	TimedBlock_PixelBufferClear,
	TimedBlock_Count,
};
struct TimedBlock {
	const char *function_name;
	uint64_t cycles;
	uint32_t hit_count;
	uint32_t parity;
};
static struct TimedBlock debug_timed_blocks[TimedBlock_Count];

#include "windows.min.h"

// pixbuf
static uint32_t *backbuffer_data;
static uint16_t backbuffer_width;
static uint16_t backbuffer_height;
static uint16_t backbuffer_padding;
static uint16_t screen_width;
static uint16_t screen_height;
// pixbuf.windows
static HDC hdc;
static HDC mdc;
static HBITMAP hbm;
static BITMAPINFO bmi;

static uint32_t argb_color(Vector4 color) {
	uint32_t result = ((uint32_t) (uint8_t) (color.w * 255.9f) << 24) |
		((uint32_t) (uint8_t) (color.x * 255.9f) << 16) |
		((uint32_t) (uint8_t) (color.y * 255.9f) <<  8) |
		((uint32_t) (uint8_t) (color.z * 255.9f) <<  0);
	return result;
}

static void PixelBufferClear(Vector4 color) {
	BEGIN_TIMED_BLOCK(PixelBufferClear);
	simd_i32 color32 = simd_set1_i32(argb_color(color));

	uint32_t *row = backbuffer_data + backbuffer_padding * backbuffer_width + backbuffer_padding;
	for (uint16_t y = backbuffer_padding; y < backbuffer_height - backbuffer_padding; ++y) {
		uint32_t *pixel = row;
		for (uint16_t x = backbuffer_padding; x < backbuffer_width - backbuffer_padding; x += SIMD32_LANES) {
			simd_store_i32((simd_i32 *) pixel, color32);
			pixel += SIMD32_LANES;
		}
		row += backbuffer_width;
	}
	END_TIMED_BLOCK(PixelBufferClear);
}

static intptr_t WINAPI WindowProc(HWND hwnd, unsigned int msg, uintptr_t wp, intptr_t lp) {
	switch (msg) {
	default: return DefWindowProcA(hwnd, msg, wp, lp);
	case WM_PAINT: ValidateRgn(hwnd, 0); return 0;
	case WM_ERASEBKGND: return 1;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP: {
		uint8_t pressed = ((uintptr_t) lp & ((uintptr_t) 1 << 31)) == 0;
		uint8_t repeat = pressed && ((uintptr_t) lp & ((uintptr_t) 1 << 30)) != 0;
		uint8_t sys = msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP;
		uint8_t alt = sys && ((uintptr_t) lp & ((uintptr_t) 1 << 29)) != 0;

		if (sys && !alt) return 0;

		if (pressed && !repeat) {
			if (wp == VK_ESCAPE) PostQuitMessage(0);
			if (wp == VK_F4 && alt) PostQuitMessage(0);
		}

		return 0;
	}
	case WM_SIZE: {
		screen_width = (uint16_t) (uintptr_t) lp;
		screen_height = (uint16_t) ((uintptr_t) lp >> 16);

		backbuffer_padding = 1 * SIMD32_LANES;
		backbuffer_width = AlignPow2Up(screen_width + 2 * backbuffer_padding, SIMD32_LANES);
		backbuffer_height = screen_height + 2 * backbuffer_padding;

		bmi.bmiHeader.biSize = sizeof bmi.bmiHeader;
		bmi.bmiHeader.biWidth = backbuffer_width;
		bmi.bmiHeader.biHeight = backbuffer_height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biSizeImage = (unsigned long) backbuffer_width *
			backbuffer_height * sizeof backbuffer_data[0];

		if (mdc) DeleteDC(mdc);
		if (hbm) DeleteObject(hbm);
		mdc = CreateCompatibleDC(hdc);
		hbm = CreateDIBSection(hdc, &bmi, 0, (void **) &backbuffer_data, 0, 0);
		SelectObject(mdc, hbm);

		Assert(((uintptr_t) backbuffer_data & (SIMD32_ALIGN - 1)) == 0);

		return 0;
	}
	case WM_CREATE: hdc = GetDC(hwnd); return 0;
	case WM_DESTROY: PostQuitMessage(0); return 0;
	}
}

void WINAPI WinMainCRTStartup(void) {
	SetProcessDPIAware();

	uint8_t granular_sleep = timeBeginPeriod(1) == TIMERR_NOERROR;
	(void) granular_sleep;

	static WNDCLASSA wndclass;
	wndclass.style = CS_OWNDC;
	wndclass.lpfnWndProc = WindowProc;
	wndclass.hInstance = GetModuleHandleA(0);
	wndclass.hCursor = LoadCursorA(0, IDC_CROSS);
	wndclass.lpszClassName = "A";
	RegisterClassA(&wndclass);

	CreateWindowExA(0, wndclass.lpszClassName, "Title",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, wndclass.hInstance, 0);

	for (;;) {
		MSG msg;
		while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) goto end;
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		PixelBufferClear(Vector4New(1.0f, 0.0f, 1.0f, 1.0f));

		for (size_t i = 0; i < TimedBlock_Count; ++i) {
			struct TimedBlock *block = debug_timed_blocks + i;
			if (block->hit_count > 0) {
				Assert(block->parity == 0);
				// uint64_t average = block->cycles / block->hit_count;
			}
		}

		uint16_t pixbuf_xoff = backbuffer_padding;
		uint16_t pixbuf_yoff = backbuffer_padding +
			(backbuffer_height - 2 * backbuffer_padding - screen_height);
		BitBlt(hdc, 0, 0, screen_width, screen_height,
			mdc, pixbuf_xoff, pixbuf_yoff, SRCCOPY);
	}

end:
	ExitProcess(0);
}

#ifdef _MSC_VER
int _fltused;
#endif
