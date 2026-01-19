#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <list>
#include <optional>
#include <regex>

#include "Actions.h"
#include "InputManager.h"

#include <ext/core/singleton.h>
#include <ext/serialization/iserializable.h>

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
        { std::make_shared<ProcessConfiguration>() });

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
