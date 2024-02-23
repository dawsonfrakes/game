#define DI __declspec(dllimport)
#if defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
#endif

#if defined(__GNUC__) && defined(__i386__)
#define WINAPI __attribute__((force_align_pointer_arg)) __stdcall
#else
#define WINAPI __stdcall
#endif

/* kernel32 */
typedef void *HANDLE;
typedef struct HINSTANCE__ *HINSTANCE;
typedef HINSTANCE HMODULE;

DI HMODULE WINAPI GetModuleHandleA(const char *);
DI NORETURN void WINAPI ExitProcess(unsigned int);

/* user32 */
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_SIZE 0x0005
#define WM_KILLFOCUS 0x0008
#define WM_PAINT 0x000F
#define WM_QUIT 0x0012
#define WM_ERASEBKGND 0x0014
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define CS_OWNDC 0x0020
#define IDC_CROSS ((const char *) (uintptr_t) (unsigned short) 32515)
#define WS_MAXIMIZEBOX 0x00010000
#define WS_MINIMIZEBOX 0x00020000
#define WS_THICKFRAME 0x00040000
#define WS_SYSMENU 0x00080000
#define WS_CAPTION 0x00C00000
#define WS_VISIBLE 0x10000000
#define WS_OVERLAPPEDWINDOW (WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define CW_USEDEFAULT ((int) 0x80000000)
#define PM_REMOVE 0x0001
#define VK_ESCAPE 0x1B
#define VK_F4 0x73

typedef struct HDC__ *HDC;
typedef struct HRGN__ *HRGN;
typedef struct HWND__ *HWND;
typedef struct HICON__ *HICON;
typedef struct HMENU__ *HMENU;
typedef struct HBRUSH__ *HBRUSH;
typedef struct HCURSOR__ *HCURSOR;
typedef intptr_t (WINAPI *WNDPROC)(HWND, unsigned int, uintptr_t, intptr_t);
typedef struct tagWNDCLASSA {
	unsigned int style;
	WNDPROC lpfnWndProc;
	int cbClsExtra;
	int cbWndExtra;
	HINSTANCE hInstance;
	HICON hIcon;
	HCURSOR hCursor;
	HBRUSH hbrBackground;
	const char *lpszMenuName;
	const char *lpszClassName;
} WNDCLASSA;
typedef struct tagPOINT {
	long x;
	long y;
} POINT;
typedef struct tagMSG {
	HWND hwnd;
	unsigned int message;
	uintptr_t wParam;
	intptr_t lParam;
	unsigned long time;
	POINT pt;
	unsigned long lPrivate;
} MSG;

DI int WINAPI SetProcessDPIAware(void);
DI HCURSOR WINAPI LoadCursorA(HINSTANCE, const char *);
DI int WINAPI RegisterClassA(const WNDCLASSA *);
DI HWND WINAPI CreateWindowExA(unsigned long, const char *, const char *, unsigned long, int, int, int, int, HWND, HMENU, HINSTANCE, void *);
DI int WINAPI PeekMessageA(MSG *, HWND, unsigned int, unsigned int, unsigned int);
DI int WINAPI TranslateMessage(const MSG *);
DI intptr_t WINAPI DispatchMessageA(const MSG *);
DI intptr_t WINAPI DefWindowProcA(HWND, unsigned int, uintptr_t, intptr_t);
DI void WINAPI PostQuitMessage(int);
DI int WINAPI ValidateRgn(HWND, HRGN);
DI HDC WINAPI GetDC(HWND);

/* gdi32 */
#define SRCCOPY 0x00CC0020

typedef void *HGDIOBJ;
typedef struct HBITMAP__ *HBITMAP;
typedef struct tagRGBQUAD {
	unsigned long rgbBlue;
	unsigned long rgbGreen;
	unsigned long rgbRed;
	unsigned long rgbReserved;
} RGBQUAD;
typedef struct tagBITMAPINFOHEADER {
	unsigned long biSize;
	long biWidth;
	long biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
} BITMAPINFOHEADER;
typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD *bmiColors;
} BITMAPINFO;

DI HDC WINAPI CreateCompatibleDC(HDC);
DI HBITMAP WINAPI CreateDIBSection(HDC, const BITMAPINFO *, unsigned int, void **, HANDLE, unsigned long);
DI int WINAPI DeleteDC(HDC);
DI int WINAPI DeleteObject(HGDIOBJ);
DI HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
DI int WINAPI BitBlt(HDC, int, int, int, int, HDC, int, int, unsigned long);

/* winmm */
#define TIMERR_NOERROR 0

DI unsigned int WINAPI timeBeginPeriod(unsigned int);

#undef NORETURN
#undef DI
