#include <Geode/Geode.hpp>
#include <atomic>
#include <chrono>
#include <thread>

using Clock = std::chrono::steady_clock;

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>

static std::atomic<int64_t> g_lastTicks{0};

static LRESULT CALLBACK rawProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_INPUT) {
        // hora exacta en que llegó el evento, antes de cualquier otra cosa
        int64_t now = Clock::now().time_since_epoch().count();

        RAWINPUT ri;
        UINT size = sizeof(ri);
        if (GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, &ri, &size,
                sizeof(RAWINPUTHEADER)) != static_cast<UINT>(-1)) {
            bool relevant = false;
            if (ri.header.dwType == RIM_TYPEMOUSE) {
                relevant = (ri.data.mouse.usButtonFlags &
                            (RI_MOUSE_LEFT_BUTTON_DOWN | RI_MOUSE_LEFT_BUTTON_UP)) != 0;
            } else if (ri.header.dwType == RIM_TYPEKEYBOARD) {
                relevant = true;
            }
            if (relevant) {
                g_lastTicks.store(now, std::memory_order_relaxed);
            }
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void rawThread() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = rawProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"FrameSnapRawInput";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"", 0, 0, 0, 0, 0,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    if (!hwnd) return;

    RAWINPUTDEVICE rid[2];
    rid[0] = {0x01, 0x02, RIDEV_INPUTSINK, hwnd}; // mouse
    rid[1] = {0x01, 0x06, RIDEV_INPUTSINK, hwnd}; // teclado
    RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
}

$execute {
    std::thread(rawThread).detach();
}

Clock::time_point lastInputTime() {
    auto now = Clock::now();
    int64_t t = g_lastTicks.load(std::memory_order_relaxed);
    if (t == 0) return now;
    auto tp = Clock::time_point(Clock::duration(t));
    // si es del futuro o muy viejo, no corresponde a este click
    if (tp > now || now - tp > std::chrono::milliseconds(100)) return now;
    return tp;
}
#else
Clock::time_point lastInputTime() {
    return Clock::now();
}
#endif
