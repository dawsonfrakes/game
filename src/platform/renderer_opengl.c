#if TARGET_OS_WINDOWS
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x0001
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

#define X(RET, NAME, ...) RET (*NAME)(__VA_ARGS__);
GDI32_FUNCTIONS
OPENGL32_FUNCTIONS
GL10_FUNCTIONS
GL30_FUNCTIONS
GL45_FUNCTIONS
#undef X

static HGLRC opengl_ctx;

static void opengl_platform_init(void) {
	HINSTANCE lib;
#define X(RET, NAME, ...) NAME = cast(RET (*)(__VA_ARGS__)) GetProcAddress(lib, #NAME);
	lib = LoadLibraryW(L"GDI32");
	GDI32_FUNCTIONS
	lib = LoadLibraryW(L"OPENGL32");
	OPENGL32_FUNCTIONS
	GL10_FUNCTIONS
#undef X
	static PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = size_of(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER | PFD_DEPTH_DONTCARE;
	pfd.cColorBits = 24;
	s32 format = ChoosePixelFormat(platform_hdc, &pfd);
	SetPixelFormat(platform_hdc, format, &pfd);

	HGLRC temp_ctx = wglCreateContext(platform_hdc);
	wglMakeCurrent(platform_hdc, temp_ctx);

	HGLRC (*wglCreateContextAttribsARB)(HDC, HGLRC, s32*) =
		cast(HGLRC (*)(HDC, HGLRC, s32*))
		wglGetProcAddress("wglCreateContextAttribsARB");

	static s32 attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 5,
		WGL_CONTEXT_FLAGS_ARB, DEVELOPER ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0,
	};
	opengl_ctx = wglCreateContextAttribsARB(platform_hdc, null, attribs);
	wglMakeCurrent(platform_hdc, opengl_ctx);

#define X(RET, NAME, ...) NAME = cast(RET (*)(__VA_ARGS__)) wglGetProcAddress(#NAME);
	GL30_FUNCTIONS
	GL45_FUNCTIONS
#undef X
}

static void opengl_platform_deinit(void) {
	if (opengl_ctx) wglDeleteContext(opengl_ctx);
	opengl_ctx = null;
}

static void opengl_platform_present(void) {
	SwapBuffers(platform_hdc);
}
#endif

static u32 opengl_main_fbo;
static u32 opengl_main_fbo_color0;
static u32 opengl_main_fbo_depth;

static void opengl_init(void) {
	opengl_platform_init();

    glCreateFramebuffers(1, &opengl_main_fbo);
    glCreateRenderbuffers(1, &opengl_main_fbo_color0);
    glCreateRenderbuffers(1, &opengl_main_fbo_depth);
}

static void opengl_deinit(void) {
	opengl_platform_deinit();
}

static void opengl_resize(void) {
    s32 fbo_color_samples_max;
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &fbo_color_samples_max);
    s32 fbo_depth_samples_max;
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &fbo_depth_samples_max);
    u32 fbo_samples = cast(u32) min(fbo_color_samples_max, fbo_depth_samples_max);

    if (platform_screen_width > 0 && platform_screen_height > 0) {
        glNamedRenderbufferStorageMultisample(opengl_main_fbo_color0, fbo_samples,
            GL_RGBA16F, platform_screen_width, platform_screen_height);
        glNamedFramebufferRenderbuffer(opengl_main_fbo, GL_COLOR_ATTACHMENT0,
            GL_RENDERBUFFER, opengl_main_fbo_color0);

        glNamedRenderbufferStorageMultisample(opengl_main_fbo_depth, fbo_samples,
            GL_DEPTH_COMPONENT32F, platform_screen_width, platform_screen_height);
        glNamedFramebufferRenderbuffer(opengl_main_fbo, GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER, opengl_main_fbo_depth);
    }
}

static void opengl_present(void) {
	f32 clear_color[] = {0.6f, 0.2f, 0.2f, 1.0f};
	glClearNamedFramebufferfv(opengl_main_fbo, GL_COLOR, 0, clear_color);
	f32 clear_depth[] = {0.0f};
	glClearNamedFramebufferfv(opengl_main_fbo, GL_DEPTH, 0, clear_depth);
	glBindFramebuffer(GL_FRAMEBUFFER, opengl_main_fbo);

	glViewport(0, 0, platform_screen_width, platform_screen_height);

	// @note(dfra): fixes intel default framebuffer resize bug
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(0);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glBlitNamedFramebuffer(opengl_main_fbo, 0,
		0, 0, platform_screen_width, platform_screen_height,
		0, 0, platform_screen_width, platform_screen_height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glDisable(GL_FRAMEBUFFER_SRGB);

	opengl_platform_present();
}
