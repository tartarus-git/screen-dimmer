#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdlib>
#include <cstdint>

#include <vector>

#include "debug_output.h"

// NOTE: We could have done this differently, in that we could have done away with the whole leader and follower thing. We did the leader follower thing because for some reason we thought that the message handlers were on different threads and
// we didn't want to deal with synchronization and all that.
// The message handlers get called from the same thread after one another though, because that how our pump at the bottom of the code works.
// We could have done this differently, in a way that probably would have been more concise, but this is how it is now and I'm leaving it like this because it's interesting.
// NOTE: While I do think all of the above is true, I couldn't actually find anywhere where it is specifically written that the message loop is on the same thread.
// Specifically, I couldn't find out if DispatchMessageA blocks or not. I could do some tests, but maybe it doesn't block in all situations or something. Idk.
// Anyway, it's kind of a weird situation, so maybe it's actually good that I wrote the program in this way, we'll see.
// TODO: Research and test more about this.

#define LEADER_WINDOW_CLASS_NAME "SCREEN_DIMMER_TRANSPARENT_WINDOW_LEADER"
#define FOLLOWER_WINDOW_CLASS_NAME "SCREEN_DIMMER_TRANSPARENT_WINDOW_FOLLOWER"

[[noreturn]] void exit_program(int exit_code) noexcept {
	std::_Exit(exit_code);			// NOTE: This doesn't clean up anything, which is what we want.
	while (true) { }				// NOTE: Just in case.
	// TODO: Try making a goto that jumps to a random location, just for fun, an exit that relies on a seg fault lol.
}

uint8_t window_alpha = 0;

bool set_window_alpha(HWND hWnd, uint8_t new_window_alpha) noexcept {
	if (!SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), new_window_alpha, LWA_ALPHA)) {
		debuglogger::out << debuglogger::error << "failed to set window transparency" << debuglogger::endl;
		return false;
	}
	return true;
}

WNDCLASSA leader_windowClass { };
WNDCLASSA follower_windowClass { };

std::vector<HWND> windows;

bool broadcast_msg_to_followers(UINT uMsg, LPARAM lParam) noexcept {
	for (size_t i = 1; i < windows.size(); i++) {
		if (!PostMessageA(windows[i], WM_APP, 0, lParam)) { return false; }
	}
	return true;
}

bool resetup_windows_flag = false;			// NOTE: Regardless of whether or not message loops are thread-safe, the usage of this flag will work because it's one way and it only gets checked after all the message loops have terminated.

bool destroy_all_windows() noexcept {
	for (size_t i = 0; i < windows.size(); i++) {
		if (!DestroyWindow(windows[i])) { return false; }
	}
	return true;
}

LRESULT CALLBACK leader_window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (uMsg) {
	case WM_HOTKEY:
		if (wParam == 0) {
			if (!set_window_alpha(hWnd, window_alpha += 1)) {
				PostQuitMessage(EXIT_FAILURE);
				return 0;
			}
		}
		else if (wParam == 1) {
			if (!set_window_alpha(hWnd, window_alpha -= 1)) {
				PostQuitMessage(EXIT_FAILURE);
				return 0;
			}
		}
		else {
			break;			// NOTE: In case the hotkey ID didn't match what we were expecting, we break and call default handler.
		}

		if (!broadcast_msg_to_followers(WM_APP, window_alpha)) {
			debuglogger::out << debuglogger::error << "failed to broadcast WM_APP msg to followers" << debuglogger::endl;
			PostQuitMessage(EXIT_FAILURE);
		}
		return 0;
	// NOTE: If a display is added or removed, destroy everything and resetup all of the windows.
	// NOTE: I couldn't actually find the specifics of when exactly WM_DISPLAYCHANGE is sent, but it works fine for this use-case and
	// it only fires once for connect/disconnect, which is also good.
	// NOTE: One of the params contains size information of the display change, but what does that mean?
	// Is it only sent to windows that are on the display that changed? How does the program know which display changed?
	// It just doesn't make a whole lot of sense.
	// TODO: Figure this out.
	case WM_DISPLAYCHANGE:
		resetup_windows_flag = true;
		if (!destroy_all_windows()) {				// NOTE: We destroy so that the window actually disappears. With only PostQuitMessage here, it would stay there.
			debuglogger::out << debuglogger::error << "failed to destroy all windows" << debuglogger::endl;
			PostQuitMessage(EXIT_FAILURE);
		}
		return 0;
	// NOTE: I'm not sure if it's always like this or just with this type of Window or something like that, but it seems that DefWindowProc doesn't automatically handle WM_DESTROY in this case.
	// That's why we handle it here. When leader gets destroyed it quits the msg pump, when followers get destroyed they don't do anything, they simply vanish. See follower message loop.
	case WM_DESTROY:
		PostQuitMessage(EXIT_SUCCESS);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK follower_window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (uMsg) {
	case WM_APP:
		if (!set_window_alpha(hWnd, lParam)) {
			PostQuitMessage(EXIT_FAILURE);
		}
		return 0;
	case WM_DESTROY:
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK monitor_discovery_proc(HMONITOR handle, HDC context, LPRECT rect, LPARAM data) {
	std::vector<RECT>& monitor_bounds = *(std::vector<RECT>*)data;
	monitor_bounds.push_back(*rect);
	return true;
}

void resetup_windows(HINSTANCE hInstance, int nCmdShow) noexcept {
	std::vector<RECT> monitor_bounds;
	static_assert(sizeof(&monitor_bounds) == sizeof(LPARAM), "pointer size is not the same as LPARAM size");
	EnumDisplayMonitors(nullptr, nullptr, monitor_discovery_proc, (LPARAM)&monitor_bounds);

	windows = std::vector<HWND>(monitor_bounds.size());

	windows[0] = CreateWindowExA(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, leader_windowClass.lpszClassName, nullptr, WS_POPUP, monitor_bounds[0].left, monitor_bounds[0].top, monitor_bounds[0].right, monitor_bounds[0].bottom, nullptr, nullptr, hInstance, nullptr);
	if (!windows[0]) {
		debuglogger::out << debuglogger::error << "couldn't create leader window" << debuglogger::endl;
		exit_program(EXIT_FAILURE);
	}
	if (!set_window_alpha(windows[0], window_alpha)) { exit_program(EXIT_FAILURE); }
	ShowWindow(windows[0], nCmdShow);

	for (size_t i = 1; i < windows.size(); i++) {
		windows[i] = CreateWindowExA(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, follower_windowClass.lpszClassName, nullptr, WS_POPUP, monitor_bounds[i].left, monitor_bounds[i].top, monitor_bounds[i].right, monitor_bounds[i].bottom, nullptr, nullptr, hInstance, nullptr);
		if (!windows[i]) {
			debuglogger::out << debuglogger::error << "couldn't create follower windows" << debuglogger::endl;
			exit_program(EXIT_FAILURE);
		}
		if (!set_window_alpha(windows[i], window_alpha)) { exit_program(EXIT_FAILURE); }
		ShowWindow(windows[i], nCmdShow);
	}
}

void setup_windows(HINSTANCE hInstance, int nCmdShow) noexcept {
	leader_windowClass.lpfnWndProc = leader_window_proc;
	leader_windowClass.hInstance = hInstance;
	leader_windowClass.lpszClassName = LEADER_WINDOW_CLASS_NAME;
	leader_windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	if (leader_windowClass.hbrBackground == nullptr) {
		debuglogger::out << debuglogger::error << "failed to set background color in leader window class" << debuglogger::endl;
		exit_program(EXIT_FAILURE);
	}

	follower_windowClass.lpfnWndProc = follower_window_proc;
	follower_windowClass.hInstance = hInstance;
	follower_windowClass.lpszClassName = FOLLOWER_WINDOW_CLASS_NAME;
	follower_windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	if (follower_windowClass.hbrBackground == nullptr) {
		debuglogger::out << debuglogger::error << "failed to set background color in follower window class" << debuglogger::endl;
		exit_program(EXIT_FAILURE);
	}

	if (!RegisterClassA(&leader_windowClass)) {
		debuglogger::out << debuglogger::error << "failed to register leader window class" << debuglogger::endl;
		exit_program(EXIT_FAILURE);
	}

	if (!RegisterClassA(&follower_windowClass)) {
		debuglogger::out << debuglogger::error << "failed to register follower window class" << debuglogger::endl;
		exit_program(EXIT_FAILURE);
	}

	resetup_windows(hInstance, nCmdShow);
}

void register_hotkeys(HWND hWnd) noexcept {
	if (!RegisterHotKey(hWnd, 0, MOD_CONTROL | MOD_ALT, 'Q')) {
		debuglogger::out << debuglogger::error << "failed to register hotkeys" << debuglogger::endl;
		exit_program(EXIT_FAILURE);
	}
	if (!RegisterHotKey(hWnd, 1, MOD_CONTROL | MOD_ALT, 'W')) {
		debuglogger::out << debuglogger::error << "failed to register hotkeys" << debuglogger::endl;
		exit_program(EXIT_FAILURE);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow) {
	setup_windows(hInstance, nCmdShow);

resetup_continuation_point:
	register_hotkeys(windows[0]);

	MSG msg = { };
	while (true) {
		int ret = GetMessageA(&msg, nullptr, 0, 0);
		if (ret == 0) { break; }
		if (ret == -1) {
			debuglogger::out << debuglogger::error << "GetMessageA() failed" << debuglogger::endl;
			return EXIT_FAILURE;
		}

		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	if (msg.wParam == EXIT_FAILURE) { return EXIT_FAILURE; }				// NOTE: It's important that this come before the resetup flag stuff.

	// NOTE: We don't unregister hotkeys because you can't unregister them when the window they're attached to is gone anyway.
	// I just assume they get unregistered when the window dissappears.

	if (resetup_windows_flag == true) {
		resetup_windows(hInstance, nCmdShow);
		resetup_windows_flag = false;
		goto resetup_continuation_point;
	}
}