#include <Windows.h>
#include <gl/GL.h>
#include <chrono>
#include <fstream>
#include <thread>
#include <random>
#include <cmath>

#include "settings.h"
#include "trayUtils.h"
#include "BackGround.h"
#include "DesktopUtils.h"
#include "fireFly.h"
#include "rendering.h"

#define TAU_F 6.283185307179586

constinit bool running = true;

struct FPOINT {
	float x;
	float y;
};

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_TRAYICON) {
		if (lParam == WM_RBUTTONUP) {
			// Create a popup menu
			HMENU menu = CreatePopupMenu();
			AppendMenuW(menu, MF_STRING, 1, L"Quit");

			// Get the cursor position
			POINT cursorPos;
			GetCursorPos(&cursorPos);

			// Show the menu
			SetForegroundWindow(hwnd);
			// Example with TPM_NONOTIFY to avoid blocking
			int selection = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, cursorPos.x - 120, cursorPos.y - 22, 0, hwnd, nullptr);
			DestroyMenu(menu);

			// Handle the menu selection
			if (selection == 1) {
				OnQuit(&hwnd, &running);
			}
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// range [start, end)
static float randomUniform(float start, float end) {
	// Create a random device and a random engine
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dist(start, end); // Range [start, end)

	return static_cast<float>(dist(gen)); // Generate the random number
}

// range [start, end]
static int randomInt(int start, int end) {
	// Create a random device and a random engine
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(start, end);

	return dist(gen);
}

static inline void checkEvents(MSG& msg, bool& running) {
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			running = false;
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

static inline void gameTick(float& frameTime, const float& stepInterval, float& fractionalTime) {
	if (frameTime < stepInterval) {
		// Calculate total sleep time including any leftover fractional time
		float totalSleepTime = (stepInterval - frameTime) + fractionalTime;

		// Truncate to whole milliseconds
		int sleepMilliseconds = static_cast<int>(totalSleepTime * 1e+3f);

		// Calculate remaining fractional time and ensure it�s within 0.0f to 1.0f
		fractionalTime = (totalSleepTime - sleepMilliseconds * 1e-3f);

		// Sleep for the calculated milliseconds
		std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilliseconds));
	}
}

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
	const wchar_t* className = L"FireFliesClass";
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = className;
	wc.hIconSm = nullptr;

	RegisterClassEx(&wc);

	const int Width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const int Height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	const int leftMost = GetSystemMetrics(SM_XVIRTUALSCREEN);
	const int topMost = GetSystemMetrics(SM_YVIRTUALSCREEN);

	HWND hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		className,
		L"FireFlies",
		WS_POPUP,
		0, 0,
		Width, Height,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (!hwnd) return -1;

	const Settings settings = loadSettings("settings.json");

	const float stepInterval = 1.0f / settings.targetFPS;
	const float MOUSE_RADIUS_SQR = settings.mouseRadius * settings.mouseRadius;

	// Load the icon from resources
	HICON hIcon = LoadIconFromResource();

	// Add the tray icon
	AddTrayIcon(&hwnd, &hIcon, L"Just a Simple Icon");

	wchar_t* originalWallpaper = GetCurrentWallpaper();

	SetAsDesktop(hwnd);

	ShowWindow(hwnd, SW_SHOW);

	HDC hdc = GetDC(hwnd); // Get device context

	// Step 1: Set up the pixel format
	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int pixelFormat = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixelFormat, &pfd);

	// Step 2: Create and set the rendering context
	HGLRC hglrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hglrc);

	// OpenGL is now set up for this window!

	float dt{ 0 };
	float frameTime{ 0 };
	float fractionalTime{ 0 };

	POINT mousePos;
	FPOINT fMousePos{};

	FireFly* fireFlies = new FireFly[settings.fireFlies.count];

	const float offsetBounds = settings.mouseRadius;
	const float roffsetBounds = -settings.mouseRadius;
	const float woffsetBounds = offsetBounds + Width;
	const float hoffsetBounds = offsetBounds + Height;

	// Fill the array with FireFly objects
	for (int i = 0; i < settings.fireFlies.count; ++i) {
		FireFly& fireFly = fireFlies[i];
		float speed = randomUniform(-settings.fireFlies.minSpeed, settings.fireFlies.maxSpeed);
		float angle = randomUniform(0, TAU_F);

		fireFly.x = randomUniform(roffsetBounds, woffsetBounds);
		fireFly.y = randomUniform(roffsetBounds, hoffsetBounds);
		fireFly.speedx = std::cosf(angle)*speed;
		fireFly.speedy = std::sinf(angle)*speed;
		fireFly.radius = settings.fireFlies.maxRadius;
		fireFly.color = settings.fireFlies.colors[randomInt(0, static_cast<int>(settings.fireFlies.colors.size()) - 1)];
	}

	auto newF = std::chrono::high_resolution_clock::now();
	auto oldF = std::chrono::high_resolution_clock::now();
	auto endF = std::chrono::high_resolution_clock::now();

	BG bg(settings.backGroundColors, Width, Height);

	// Message loop
	MSG msg = {};
	while (running) {
		// get deltt time
		oldF = newF;
		newF = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration<float>(newF - oldF).count();

		// Check for messages
		checkEvents(msg, running);

		// draw background
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		bg.fillGradient();
		glLoadIdentity();
		glOrtho(0, Width, Height, 0, -1, 1);

		GetCursorPos(&mousePos);
		fMousePos.x = static_cast<float>(mousePos.x - leftMost);
		fMousePos.y = static_cast<float>(mousePos.y - topMost);

		for (int starIdx = 0; starIdx < settings.fireFlies.count; ++starIdx) {
			FireFly& fireFly = fireFlies[starIdx];
			fireFly.move(dt);

			if (fireFly.x < roffsetBounds) {
				fireFly.x -= (fireFly.x - roffsetBounds) * 2;
				fireFly.speedx = std::abs(fireFly.speedx);
			}
			else if (fireFly.x > woffsetBounds) {
				fireFly.x -= (fireFly.x - woffsetBounds) * 2;
				fireFly.speedx = -std::abs(fireFly.speedx);
			}

			if (fireFly.y < roffsetBounds) {
				fireFly.y -= (fireFly.y - roffsetBounds) * 2;
				fireFly.speedy = std::abs(fireFly.speedy);
			}
			else if (fireFly.y > hoffsetBounds) {
				fireFly.y -= (fireFly.y - hoffsetBounds) * 2;
				fireFly.speedy = -std::abs(fireFly.speedy);
			}

			float mouse_dx = fMousePos.x - fireFly.x;
			float mouse_dy = fMousePos.y - fireFly.y;
			float distance_from_mouse_sqr = mouse_dx * mouse_dx + mouse_dy * mouse_dy;
			if (distance_from_mouse_sqr && distance_from_mouse_sqr < MOUSE_RADIUS_SQR) {
				float distance_from_mouse = std::sqrtf(distance_from_mouse_sqr);
				float limited_distance_value = distance_from_mouse < settings.mouseRadius ? distance_from_mouse : settings.mouseRadius;
				float ratio = 1.0f - (limited_distance_value / settings.mouseRadius);
				fireFly.color[3] += ratio * dt * settings.fireFlies.brightenSpeed;
				fireFly.color[3] = fireFly.color[3] > settings.fireFlies.maxAlpha ? settings.fireFlies.maxAlpha : fireFly.color[3];
			}
			else {
				fireFly.color[3] -= settings.fireFlies.darkenSpeed * dt;
				fireFly.color[3] = fireFly.color[3] < settings.fireFlies.minAlpha ? settings.fireFlies.minAlpha : fireFly.color[3];
			}

			// Prevent division by zero and limit max force
			if (distance_from_mouse_sqr < settings.minDistance * settings.minDistance) distance_from_mouse_sqr = settings.minDistance * settings.minDistance;
			float distance = sqrtf(distance_from_mouse_sqr);
			// Normalize the direction
			float dirX = mouse_dx / distance;
			float dirY = mouse_dy / distance;

			// Apply gravitational force (inverse-square falloff)
			float fx = dirX * settings.gravity / distance_from_mouse_sqr;
			float fy = dirY * settings.gravity / distance_from_mouse_sqr;

			// Apply force to particle velocity
			fireFly.speedx += fx * dt;
			fireFly.speedy += fy * dt;


			fireFly.render(settings.fireFlies.nSegments);
		}


		SwapBuffers(hdc);

		endF = std::chrono::high_resolution_clock::now();
		frameTime = std::chrono::duration<float>(endF - newF).count();

		gameTick(frameTime, stepInterval, fractionalTime);
	}

	SetParent(hwnd, nullptr);
	SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void*)originalWallpaper, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(hglrc);
	ReleaseDC(hwnd, hdc);

	DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	delete[] originalWallpaper;
	delete[] fireFlies;
	DestroyIcon(hIcon);

	return 0;
}
