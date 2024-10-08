#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <list>
#include <optional>
#include <regex>

#include "InputManager.h"

#include <ext/core/singleton.h>
#include <ext/serialization/iserializable.h>

constexpr int kNotSetVkCode = -1;

struct IBaseInput
{
    virtual ~IBaseInput() = default;
    // Get text
    [[nodiscard]] virtual std::wstring ToString() const = 0;
    // Check if input command was pressed
    [[nodiscard]] virtual bool IsPressed(WORD vkCode, bool down) const = 0;
    // Update input
    virtual void UpdateInput(WORD vkCode, bool down) = 0;
};

inline int operator<(const IBaseInput& first, const IBaseInput& second)
{
    return first.ToString() < second.ToString();
}

// User key settings
struct Key : IBaseInput
{
    Key() = default;
    Key(WORD _vkCode) { vkCode = _vkCode; }

    // Get text
    [[nodiscard]] std::wstring ToString() const;
    // Check if bind was pressed
    [[nodiscard]] bool IsPressed(WORD vkCode, bool down) const override;
    // Update input
    void UpdateInput(WORD vkCode, bool down) override;

    REGISTER_SERIALIZABLE_OBJECT();

    DECLARE_SERIALIZABLE_FIELD(int, vkCode, kNotSetVkCode);
    // Time point when we ignored key for active process
    std::optional<std::chrono::system_clock::time_point> lastTimeWhenKeyWasIgnored;
};

// User bind settings
struct Bind : IBaseInput
{
    Bind() = default;
    Bind(WORD vkCode);

    // Get text
    [[nodiscard]] std::wstring ToString() const override;
    // Check if key was pressed
    [[nodiscard]] bool IsPressed(WORD vkCode, bool down) const override;
    // Update input
    void UpdateInput(WORD vkCode, bool down) override;

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
    DECLARE_SERIALIZABLE_FIELD(int, vkCode, kNotSetVkCode);
    DECLARE_SERIALIZABLE_FIELD(unsigned, extraKeys, 0);

    static_assert(size_t(ExtraKeys::eScrollUp) < CHAR_BIT * sizeof(decltype(extraKeys)),
        "Bind::extraKeys type is too small to use enum ExtraKeys as flags");
};

struct Action : IBaseInput
{
    Action() = default;
    static Action NewAction(WORD vkCode, bool down, unsigned delay);
    static Action NewMousePosition(long mouseMovedToPointX, long mouseMovedToPointY, unsigned delay);
    static Action NewMouseMove(long mouseDeltaX, long mouseDeltaY, unsigned delay, bool directInput);
    static Action NewRunScript(const std::wstring& scriptPath, unsigned delay);

    // Get action text
    [[nodiscard]] std::wstring ToString() const override;
    [[nodiscard]] bool IsPressed(WORD vkCode, bool down) const override { EXT_ASSERT(false); return false; }
    // Update input
    void UpdateInput(WORD vkCode, bool down) override;

    // Execution action
    void ExecuteAction(unsigned delayRandomizeInMs) const;

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
    DECLARE_SERIALIZABLE_FIELD(bool, randomizeDelay, true);
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
    DECLARE_SERIALIZABLE_FIELD(std::wstring, description);
    DECLARE_SERIALIZABLE_FIELD(MouseRecordMode, mouseRecordMode, MouseRecordMode::eNoMouseMovements);
    DECLARE_SERIALIZABLE_FIELD(bool, showMouseMovementsUnited, true);
    //--------------------------------------------------------------------------------------------- 
    DECLARE_SERIALIZABLE_FIELD(bool, enableRandomDelay, false);
    DECLARE_SERIALIZABLE_FIELD(unsigned, randomizeDelayMs, 1);
    DECLARE_SERIALIZABLE_FIELD(std::list<Action>, actions);

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
    DECLARE_SERIALIZABLE_FIELD((std::map<Bind, Actions>), actionsByBind);
    DECLARE_SERIALIZABLE_FIELD((std::map<Key, Key>), keysRemapping);
    DECLARE_SERIALIZABLE_FIELD(std::list<Key>, keysToIgnoreAccidentalPress);
    DECLARE_SERIALIZABLE_FIELD(bool, changeBrightness, false);
    DECLARE_SERIALIZABLE_FIELD(unsigned, brightnessLevel, 100);
    DECLARE_SERIALIZABLE_FIELD(crosshair::Settings, crosshairSettings);

    const std::wstring& GetExeName() const;
    void SetExeName(const std::wstring& exeName);
    
    // Check if given exe name matches process configuration
    bool MatchExeName(const std::wstring& exeName) const;
private:
    // Notification from serializer that deserialization is done for this object
    void OnDeserializationEnd();

    DECLARE_SERIALIZABLE_FIELD(std::wstring, exeName);

    // Regular expression to match exe name
    std::wregex exeNameRegex;
};

struct Settings
{
    REGISTER_SERIALIZABLE_OBJECT();

    DECLARE_SERIALIZABLE_FIELD(bool, enabled, true);
    DECLARE_SERIALIZABLE_FIELD(Bind, enableBind);
    DECLARE_SERIALIZABLE_FIELD(int, activeConfiguration, 0);
    DECLARE_SERIALIZABLE_FIELD(std::list<std::shared_ptr<ProcessConfiguration>>, processConfigurations,
        std::make_shared<ProcessConfiguration>());

    Settings();
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
    DECLARE_SERIALIZABLE_FIELD(Bind, startPauseTimerBind);
    DECLARE_SERIALIZABLE_FIELD(Bind, resetTimerBind);
    // UI
    DECLARE_SERIALIZABLE_FIELD(Rect, windowRect);
    DECLARE_SERIALIZABLE_FIELD(bool, minimizeInterface, true);
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
        eActiveProcessToolkit,
        eActionExecutor,
    };

    REGISTER_SERIALIZABLE_OBJECT();
    // General program settings
    DECLARE_SERIALIZABLE_FIELD(bool, showMinimizedBubble, true);
    DECLARE_SERIALIZABLE_FIELD(bool, tracesEnabled, false);
    // UI
    DECLARE_SERIALIZABLE_FIELD(InputManager::InputSimulator, inputSimulator, InputManager::InputSimulator::Auto);
    DECLARE_SERIALIZABLE_FIELD(ProgramMode, selectedMode, ProgramMode::eActiveProcessToolkit);
    DECLARE_SERIALIZABLE_FIELD(process_toolkit::Settings, process_toolkit);
    DECLARE_SERIALIZABLE_FIELD(actions_executor::Settings, actions_executor);
    DECLARE_SERIALIZABLE_FIELD(timer::Settings, timer);
};
