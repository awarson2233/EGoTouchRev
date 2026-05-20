#include "win32/SyntheticPenButtonInjector.h"
#include "Logger.h"

SyntheticPenButtonInjector::~SyntheticPenButtonInjector() {
    if (m_device) {
        DestroySyntheticPointerDevice(m_device);
        m_device = nullptr;
    }
}

bool SyntheticPenButtonInjector::EnsureDevice() {
    if (m_device) return true;

    m_device = CreateSyntheticPointerDevice(
        PT_PEN, 1, POINTER_FEEDBACK_DEFAULT);

    if (!m_device) {
        DWORD err = GetLastError();
        LOG_WARN("SynthPenBtn", __func__, "Win32",
                 "CreateSyntheticPointerDevice(PT_PEN) failed: {}", err);
        return false;
    }

    LOG_INFO("SynthPenBtn", __func__, "Win32",
             "Synthetic pen device created.");
    return true;
}

bool SyntheticPenButtonInjector::InjectBarrelPulse(POINT screenPt) {
    return InjectPenPulse(
        screenPt,
        PEN_FLAG_BARREL,
        POINTER_FLAG_SECONDBUTTON);
}

bool SyntheticPenButtonInjector::InjectEraserPulse(POINT screenPt) {
    return InjectPenPulse(
        screenPt,
        PEN_FLAG_ERASER | PEN_FLAG_INVERTED,
        0);
}

bool SyntheticPenButtonInjector::InjectWinF22Shortcut() {
    INPUT inputs[4]{};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_F22;

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_F22;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_LWIN;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    const UINT sent = SendInput(4, inputs, sizeof(INPUT));
    if (sent != 4) {
        const DWORD err = GetLastError();
        LOG_WARN("SynthPenBtn", __func__, "Win32",
                 "SendInput Win+F22 failed: sent={}, err={}",
                 static_cast<unsigned>(sent), err);
        return false;
    }

    LOG_INFO("SynthPenBtn", __func__, "Win32",
             "Injected Win+F22 shortcut.");
    return true;
}

bool SyntheticPenButtonInjector::InjectPenPulse(
        POINT screenPt, UINT32 penFlags, UINT32 extraPointerFlags) {
    if (!EnsureDevice()) return false;

    POINTER_PEN_INFO penInfo{};
    penInfo.pointerInfo.pointerType = PT_PEN;
    penInfo.pointerInfo.pointerId   = 1;
    penInfo.pointerInfo.pointerFlags =
        POINTER_FLAG_INRANGE | POINTER_FLAG_UPDATE | extraPointerFlags;
    penInfo.pointerInfo.ptPixelLocation = screenPt;
    penInfo.penFlags = penFlags;
    penInfo.penMask  = PEN_MASK_NONE;

    POINTER_TYPE_INFO typeInfo{};
    typeInfo.type = PT_PEN;
    typeInfo.penInfo = penInfo;

    if (!InjectSyntheticPointerInput(m_device, &typeInfo, 1)) {
        DWORD err = GetLastError();
        LOG_WARN("SynthPenBtn", __func__, "Win32",
                 "InjectSyntheticPointerInput(penFlags=0x{:X}, ptrFlags=0x{:X}) failed: {}",
                 static_cast<unsigned>(penFlags),
                 static_cast<unsigned>(penInfo.pointerInfo.pointerFlags),
                 err);
        return false;
    }

    LOG_INFO("SynthPenBtn", __func__, "Win32",
             "Injected pen pulse at ({}, {}), penFlags=0x{:X}, ptrFlags=0x{:X}",
             screenPt.x, screenPt.y,
             static_cast<unsigned>(penFlags),
             static_cast<unsigned>(penInfo.pointerInfo.pointerFlags));
    return true;
}
