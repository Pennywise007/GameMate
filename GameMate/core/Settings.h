#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <list>
#include <optional>

#include "InputManager.h"

#include <ext/core/singleton.h>
#include <ext/serialization/iserializable.h>

constexpr int kNotSetVkCode = -1;

// User bind settings
struct Bind
{
    Bind() = default;
    Bind(WORD vkCode);

    // Comparison operator for map
    int operator<(const Bind& other) const { return ToString() < other.ToString(); }

    // Get text
    [[nodiscard]] std::wstring ToString() const;
    void UpdateBind(WORD vkCode, bool down);
    // Check if bind was pressed
    bool IsBindPressed(WORD vkCode, bool down) const;

    enum class ExtraKeys
    {
        FirstModifierKey,
        LCtrl = FirstModifierKey,
        RCtrl,
        LShift,
        RShift,
        LAlt,
        RAlt,
        LWin,
        RWin,
        LastModifierKey,
        eScrollUp   // for mouse (h)wheel up
    };

    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(unsigned, extraKeys, 0);
    DECLARE_SERIALIZABLE_FIELD(int, vkCode, kNotSetVkCode);

    static_assert(size_t(ExtraKeys::eScrollUp) < CHAR_BIT * sizeof(decltype(extraKeys)),
        "Bind::extraKeys type is too small to use enum ExtraKeys as flags");
};

struct Action
{
    Action() = default;
    static Action NewAction(WORD vkCode, bool down, unsigned delay);
    static Action NewMousePosition(long mouseMovedToPointX, long mouseMovedToPointY, unsigned delay);
    static Action NewMouseMove(long mouseDeltaX, long mouseDeltaY, unsigned delay, bool directInput);
    static Action NewRunScript(const std::wstring& scriptPath, unsigned delay);

    // Get action text
    [[nodiscard]] std::wstring ToString() const;
    // Execution action
    void ExecuteAction(float delayRandomize) const;

    REGISTER_SERIALIZABLE_OBJECT();

    enum class Type
    {
        eKeyOrMouseAction,
        eCursorPosition,
        eMouseMove,
        eMouseMoveDirectInput,
        eRunScript
    };
    DECLARE_SERIALIZABLE_FIELD(Type, type, Type::eKeyOrMouseAction);
    // Delay before executing action
    DECLARE_SERIALIZABLE_FIELD(unsigned, delayInMilliseconds, 0);
    // eKeyOrMouseAction
    DECLARE_SERIALIZABLE_FIELD(int, vkCode, kNotSetVkCode);
    DECLARE_SERIALIZABLE_FIELD(bool, down, false);
    // eCursorPosition/eMouseMove/eMouseMoveDirectInput
    DECLARE_SERIALIZABLE_FIELD(long, mouseX, 0);
    DECLARE_SERIALIZABLE_FIELD(long, mouseY, 0);
    // eRunScript
    DECLARE_SERIALIZABLE_FIELD(std::wstring, scriptPath);
};

struct Actions
{
    enum class MouseRecordMode
    {
        eNoMouseMovements,
        eRecordCursorPosition,
        eRecordCursorDelta,
        eRecordCursorDeltaWithDirectX
    };

    REGISTER_SERIALIZABLE_OBJECT();
    //--------------------------------------------------------------------------------------------- 
    // UI settings
    DECLARE_SERIALIZABLE_FIELD(MouseRecordMode, mouseRecordMode, MouseRecordMode::eNoMouseMovements);
    DECLARE_SERIALIZABLE_FIELD(bool, showMouseMovementsUnited, false);
    //--------------------------------------------------------------------------------------------- 
    DECLARE_SERIALIZABLE_FIELD(std::list<Action>, actions);
    DECLARE_SERIALIZABLE_FIELD(float, randomizeDelays, 0.f);

    void Execute() const;
};

namespace actions_executor {

enum class RepeatMode {
    eTimes,
    eUntilStopped
};

struct Settings
{
    REGISTER_SERIALIZABLE_OBJECT();

    // UI status, not serializable
    bool enabled = false;

    DECLARE_SERIALIZABLE_FIELD(Bind, enableBind);
    DECLARE_SERIALIZABLE_FIELD(Actions, actions);

    DECLARE_SERIALIZABLE_FIELD(unsigned, repeatIntervalMinutes, 0);
    DECLARE_SERIALIZABLE_FIELD(unsigned, repeatIntervalSeconds, 0);
    DECLARE_SERIALIZABLE_FIELD(unsigned, repeatIntervalMilliseconds, 0);

    DECLARE_SERIALIZABLE_FIELD(RepeatMode, repeatMode, RepeatMode::eTimes);
    DECLARE_SERIALIZABLE_FIELD(unsigned, repeatTimes, 1);

    Settings();

    void Execute() const;
};

} // namespace actions_executor

namespace process_toolkit {

namespace crosshair {
enum class Type
{
    eDot,
    eCross,
    eCrossWithCircle,
    eCircleWithCrossInside,
    eCircleWithCrossAndDot,
    eCrossWithCircleAndDot,
    eCrossWithCircleAndCircleInside,
    eDashedCircleAndDot,
    eDashedBoxWithCross
};

enum class Size
{
    eSmall,  // 8x8
    eMedium, // 16x16
    eLarge   // 32x32
};

struct Settings
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(bool, show, false);
    DECLARE_SERIALIZABLE_FIELD(crosshair::Size, size, Size::eMedium);
    DECLARE_SERIALIZABLE_FIELD(crosshair::Type, type, Type::eCross);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, customCrosshairName);
    DECLARE_SERIALIZABLE_FIELD(COLORREF, color, RGB(128, 0, 128));
};

} // namespace crosshair

struct ProcessConfiguration
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(bool, enabled, true);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, name, L"Configuration name");
    DECLARE_SERIALIZABLE_FIELD(std::wstring, exeName);
    DECLARE_SERIALIZABLE_FIELD(bool, disableWinButton, false);
    DECLARE_SERIALIZABLE_FIELD(crosshair::Settings, crosshairSettings);
    DECLARE_SERIALIZABLE_FIELD((std::map<Bind, Actions>), actionsByBind);
};

struct Settings
{
    REGISTER_SERIALIZABLE_OBJECT();

    DECLARE_SERIALIZABLE_FIELD(bool, enabled, true);
    DECLARE_SERIALIZABLE_FIELD(int, activeConfiguration, 0);
    DECLARE_SERIALIZABLE_FIELD(std::list<std::shared_ptr<ProcessConfiguration>>, processConfigurations,
        std::make_shared<ProcessConfiguration>());
};

} // namespace process_toolkit

namespace timer {

struct Settings
{
    struct Rect : CRect
    {
        Rect() : CRect(-1, -1, -1, -1)
        {
            REGISTER_SERIALIZABLE_FIELD(left);
            REGISTER_SERIALIZABLE_FIELD(top);
            REGISTER_SERIALIZABLE_FIELD(right);
            REGISTER_SERIALIZABLE_FIELD(bottom);
        }

        REGISTER_SERIALIZABLE_OBJECT();
    };

    Settings();

    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(Bind, showTimerBind);
    DECLARE_SERIALIZABLE_FIELD(Bind, pauseTimerBind);
    DECLARE_SERIALIZABLE_FIELD(Bind, resetTimerBind);
    // UI
    DECLARE_SERIALIZABLE_FIELD(Rect, windowRect);
    DECLARE_SERIALIZABLE_FIELD(bool, hideInterface, true);
    DECLARE_SERIALIZABLE_FIELD(bool, displayHours, false);
    DECLARE_SERIALIZABLE_FIELD(COLORREF, textColor, RGB(0, 0, 0));
    DECLARE_SERIALIZABLE_FIELD(COLORREF, backgroundColor, GetSysColor(COLOR_3DFACE));
};

} // namespace timer

class Settings
{
    friend ext::Singleton<Settings>;
    Settings();
public:
    void SaveSettings();

    enum class ProgramMode {
        eActionExecutor,
        eActiveProcessToolkit
    };

    REGISTER_SERIALIZABLE_OBJECT();
    // General program settings
    DECLARE_SERIALIZABLE_FIELD(bool, showMinimizedBubble, true);
    DECLARE_SERIALIZABLE_FIELD(bool, tracesEnabled, true);
    // UI
    DECLARE_SERIALIZABLE_FIELD(InputManager::InputSimulator, inputSimulator, InputManager::InputSimulator::Auto);
    DECLARE_SERIALIZABLE_FIELD(ProgramMode, selectedMode, ProgramMode::eActionExecutor);
    DECLARE_SERIALIZABLE_FIELD(process_toolkit::Settings, process_toolkit);
    DECLARE_SERIALIZABLE_FIELD(actions_executor::Settings, actions_executor);
    DECLARE_SERIALIZABLE_FIELD(timer::Settings, timer);
};
