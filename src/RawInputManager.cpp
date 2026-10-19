#include "RawInputManager.h"

#include "Log.h"

#include <QByteArray>

#include <exception>
#include <string>
#include <vector>

#include <windows.h>

struct RawInputManager::NativeWindow {
    explicit NativeWindow(RawInputManager* manager)
        : owner(manager)
    {
    }

    ~NativeWindow()
    {
        destroy();
    }

    bool create()
    {
        instance = GetModuleHandleW(nullptr);
        className = L"BigA.RawInputSink."
                    + std::to_wstring(GetCurrentProcessId());

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.hInstance = instance;
        windowClass.lpfnWndProc = &NativeWindow::windowProc;
        windowClass.lpszClassName = className.c_str();

        if (!RegisterClassExW(&windowClass)) {
            Log::error(QStringLiteral("RawInput 窗口类注册失败 (Win32 %1)")
                           .arg(static_cast<qulonglong>(GetLastError())));
            return false;
        }
        classRegistered = true;

        hwnd = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED
                | WS_EX_TRANSPARENT,
            className.c_str(), L"", WS_POPUP, -10000, -10000, 1, 1, nullptr,
            nullptr, instance, this);
        if (hwnd == nullptr) {
            Log::error(QStringLiteral("RawInput 消息窗口创建失败 (Win32 %1)")
                           .arg(static_cast<qulonglong>(GetLastError())));
            destroy();
            return false;
        }

        // Keep the sink window technically visible, fully transparent and
        // off-screen. This avoids any input-sink suppression that some Windows
        // configurations apply to windows that were never shown, while leaving
        // no visible artifact for the user.
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        return true;
    }

    void destroy()
    {
        if (hwnd != nullptr) {
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }
        if (classRegistered) {
            UnregisterClassW(className.c_str(), instance);
            classRegistered = false;
        }
    }

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam,
                                       LPARAM lParam)
    {
        if (message == WM_NCCREATE) {
            const auto* createStruct =
                reinterpret_cast<const CREATESTRUCTW*>(lParam);
            auto* self =
                static_cast<NativeWindow*>(createStruct->lpCreateParams);
            if (self != nullptr) {
                self->hwnd = hwnd;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                                  reinterpret_cast<LONG_PTR>(self));
            }
        }

        auto* self = reinterpret_cast<NativeWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (self != nullptr && self->owner != nullptr && message == WM_INPUT) {
            self->owner->processRawInputMessage(
                static_cast<quintptr>(wParam), static_cast<qintptr>(lParam));
        }

        // DefWindowProc is required for WM_INPUT cleanup.
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    RawInputManager* owner = nullptr;
    HINSTANCE instance = nullptr;
    HWND hwnd = nullptr;
    std::wstring className;
    bool classRegistered = false;
};

RawInputManager::RawInputManager(QObject* parent)
    : QObject(parent)
{
}

RawInputManager::~RawInputManager()
{
    stop();
}

bool RawInputManager::start()
{
    if (m_started) {
        return m_devicesRegistered;
    }
    m_started = true;

    m_nativeWindow = std::make_unique<NativeWindow>(this);
    if (!m_nativeWindow->create()) {
        m_nativeWindow.reset();
        m_started = false;
        return false;
    }

    if (!registerDevices()) {
        stop();
        return false;
    }
    return true;
}

void RawInputManager::stop()
{
    if (m_devicesRegistered) {
        RAWINPUTDEVICE devices[2]{};
        devices[0].usUsagePage = 0x01;
        devices[0].usUsage = 0x06;
        devices[0].dwFlags = RIDEV_REMOVE;
        devices[0].hwndTarget = nullptr;
        devices[1].usUsagePage = 0x01;
        devices[1].usUsage = 0x02;
        devices[1].dwFlags = RIDEV_REMOVE;
        devices[1].hwndTarget = nullptr;
        RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
        m_devicesRegistered = false;
    }

    m_pressedKeys.clear();
    m_rightDown = false;
    m_middleDown = false;
    m_x1Down = false;
    m_x2Down = false;

    m_nativeWindow.reset();
    m_started = false;
}

bool RawInputManager::registerDevices()
{
    if (!m_nativeWindow || m_nativeWindow->hwnd == nullptr) {
        return false;
    }

    RAWINPUTDEVICE devices[2]{};
    devices[0].usUsagePage = 0x01;  // Generic desktop controls.
    devices[0].usUsage = 0x06;      // Keyboard.
    devices[0].dwFlags = RIDEV_INPUTSINK;
    devices[0].hwndTarget = m_nativeWindow->hwnd;

    devices[1].usUsagePage = 0x01;
    devices[1].usUsage = 0x02;  // Mouse.
    devices[1].dwFlags = RIDEV_INPUTSINK;
    devices[1].hwndTarget = m_nativeWindow->hwnd;

    if (!RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE))) {
        Log::error(QStringLiteral("RawInput 设备注册失败 (Win32 %1)")
                       .arg(static_cast<qulonglong>(GetLastError())));
        return false;
    }

    m_devicesRegistered = true;
    return true;
}

void RawInputManager::processRawInputMessage(quintptr wParam, qintptr lParam)
{
    Q_UNUSED(wParam);

    const HRAWINPUT rawHandle = reinterpret_cast<HRAWINPUT>(lParam);
    if (rawHandle == nullptr) {
        return;
    }

    UINT size = 0;
    if (GetRawInputData(rawHandle, RID_INPUT, nullptr, &size,
                        sizeof(RAWINPUTHEADER))
            == static_cast<UINT>(-1)
        || size == 0) {
        return;
    }

    std::vector<BYTE> buffer(size);
    UINT readSize = size;
    const UINT copied = GetRawInputData(
        rawHandle, RID_INPUT, buffer.data(), &readSize,
        sizeof(RAWINPUTHEADER));
    if (copied == static_cast<UINT>(-1) || copied != size) {
        return;
    }

    const auto* input = reinterpret_cast<const RAWINPUT*>(buffer.data());
    try {
        if (input->header.dwType == RIM_TYPEKEYBOARD) {
            processKeyboard(&input->data.keyboard);
        } else if (input->header.dwType == RIM_TYPEMOUSE) {
            processMouse(&input->data.mouse);
        }
    } catch (const std::exception& exception) {
        Log::error(QStringLiteral("处理 RawInput 时发生异常: %1")
                       .arg(QString::fromUtf8(exception.what())));
    } catch (...) {
        Log::error(QStringLiteral("处理 RawInput 时发生未知异常"));
    }
}

void RawInputManager::processKeyboard(const void* rawKeyboard)
{
    const auto* keyboard = static_cast<const RAWKEYBOARD*>(rawKeyboard);
    if (keyboard == nullptr) {
        return;
    }

    const bool released = (keyboard->Flags & RI_KEY_BREAK) != 0;
    const bool extended = (keyboard->Flags & RI_KEY_E0) != 0;
    const quint16 scanCode = keyboard->MakeCode;
    const quint16 virtualKey = keyboard->VKey;

    const quint32 identity =
        (static_cast<quint32>(scanCode) << 9) | (extended ? 0x1u : 0x0u);

    if (released) {
        m_pressedKeys.remove(identity);
        return;
    }

    // 0xFF marks a fake key; 0 is not a usable key identity.
    if (scanCode == 0 || virtualKey == 0 || virtualKey == 0xFF) {
        return;
    }

    // Auto-repeat: only the first make code triggers an action.
    if (m_pressedKeys.contains(identity)) {
        return;
    }
    m_pressedKeys.insert(identity);

    if (virtualKey == VK_ESCAPE) {
        emit escapePressed();
        return;
    }

    emit keyInput(KeyBinding::fromRawKeyboard(scanCode, extended, virtualKey));
}

void RawInputManager::processMouse(const void* rawMouse)
{
    const auto* mouse = static_cast<const RAWMOUSE*>(rawMouse);
    if (mouse == nullptr) {
        return;
    }

    const USHORT flags = mouse->usButtonFlags;

    if ((flags & RI_MOUSE_RIGHT_BUTTON_DOWN) != 0 && !m_rightDown) {
        m_rightDown = true;
        emit rightButtonPressed();
    }
    if ((flags & RI_MOUSE_RIGHT_BUTTON_UP) != 0) {
        if (m_rightDown) {
            m_rightDown = false;
        }
        emit rightButtonReleased();
    }

    if ((flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) != 0 && !m_middleDown) {
        m_middleDown = true;
        emit keyInput(KeyBinding::mouseMiddle());
    }
    if ((flags & RI_MOUSE_MIDDLE_BUTTON_UP) != 0) {
        m_middleDown = false;
    }

    if ((flags & RI_MOUSE_BUTTON_4_DOWN) != 0 && !m_x1Down) {
        m_x1Down = true;
        emit keyInput(KeyBinding::mouseX1());
    }
    if ((flags & RI_MOUSE_BUTTON_4_UP) != 0) {
        m_x1Down = false;
    }

    if ((flags & RI_MOUSE_BUTTON_5_DOWN) != 0 && !m_x2Down) {
        m_x2Down = true;
        emit keyInput(KeyBinding::mouseX2());
    }
    if ((flags & RI_MOUSE_BUTTON_5_UP) != 0) {
        m_x2Down = false;
    }

    if ((flags & RI_MOUSE_WHEEL) != 0) {
        const auto delta = static_cast<short>(mouse->usButtonData);
        if (delta > 0) {
            emit keyInput(KeyBinding::wheelUp());
        } else if (delta < 0) {
            emit keyInput(KeyBinding::wheelDown());
        }
    }
}
