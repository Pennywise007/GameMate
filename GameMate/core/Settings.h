#pragma once

#include <map>
#include <string>
#include <list>
#include <optional>

#include <ext/core/singleton.h>
#include <ext/core/dispatcher.h>
#include <ext/serialization/iserializable.h>

struct Bind
{
    Bind() = default;
    Bind(WORD _vkCode);

    // Comparision operator for map
    int operator<(const Bind& other) const { return ToString() < other.ToString(); }

    // Get text
    [[nodiscard]] std::wstring ToString() const;
    // Check if bind was pressed
    bool IsBindPressed(WORD vkCode) const;

    enum class ExtraKeys
    {
        FirstKey,
        LCtrl = FirstKey,
        RCtrl,
        LShift,
        RShift,
        LAlt,
        RAlt,
        LWin,
        RWin,
        LastKey
    };

    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(unsigned, extraKeys);
    DECLARE_SERIALIZABLE_FIELD(WORD, vkCode);
};

struct Action
{
    Action() = default;
    Action(WORD _vkCode, bool _down, unsigned _delay);
    Action(long mouseMovedToPointX, long mouseMovedToPointY);


    // Get action text
    [[nodiscard]] std::wstring ToString() const;
    // Execution action
    void ExecuteAction(float delayRandomize, bool applyMousePos) const;

    REGISTER_SERIALIZABLE_OBJECT();

    enum class Type
    {
        eMouseAction,
        eKeyAction,
        eMouseMove
    };
    DECLARE_SERIALIZABLE_FIELD(Type, type, Type::eKeyAction);
    DECLARE_SERIALIZABLE_FIELD(WORD, vkCode, 0);
    DECLARE_SERIALIZABLE_FIELD(bool, down, false);
    DECLARE_SERIALIZABLE_FIELD(unsigned, delayInMilliseconds, 0);
    DECLARE_SERIALIZABLE_FIELD(long, mouseX, 0);
    DECLARE_SERIALIZABLE_FIELD(long, mouseY, 0);
};

struct Actions
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(std::list<Action>, actions);
    DECLARE_SERIALIZABLE_FIELD(float, randomizeDelays, 0.f);
};

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
    DECLARE_SERIALIZABLE_FIELD(COLORREF, color, RGB(255, 192, 0));
};

} // namespace crosshair

struct TabConfiguration
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(bool, enabled, true);
    DECLARE_SERIALIZABLE_FIELD(bool, disableWinButton, false);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, tabName);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, exeName);
    DECLARE_SERIALIZABLE_FIELD(crosshair::Settings, crosshairSettings);
    DECLARE_SERIALIZABLE_FIELD((std::map<Bind, Actions>), actionsByBind);
};

namespace actions_executor {

enum class RepeatMode {
    eTimes,
    eUntilStopped
};

struct Settings
{
    REGISTER_SERIALIZABLE_OBJECT();
    bool enabled = false;

    DECLARE_SERIALIZABLE_FIELD(Actions, actionsSettings);
    DECLARE_SERIALIZABLE_FIELD(Bind, enableBind, Bind(VK_F6));
    DECLARE_SERIALIZABLE_FIELD(unsigned, repeatIntervalMinutes, 0);
    DECLARE_SERIALIZABLE_FIELD(unsigned, repeatIntervalSeconds, 0);
    DECLARE_SERIALIZABLE_FIELD(unsigned, repeatIntervalMilliseconds, 0);

    DECLARE_SERIALIZABLE_FIELD(RepeatMode, repeatMode, RepeatMode::eTimes);
    DECLARE_SERIALIZABLE_FIELD(unsigned, repeatTimes, 0);
};

} // namespace actions_executor

class Settings
{
    friend ext::Singleton<Settings>;
    Settings();
public:
    void SaveSettings();

    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(bool, showMinimizedBubble, true);
    DECLARE_SERIALIZABLE_FIELD(bool, tracesEnabled, true);
    DECLARE_SERIALIZABLE_FIELD(bool, programWorking, true);
    DECLARE_SERIALIZABLE_FIELD(int, driverInputMode, 0);
    DECLARE_SERIALIZABLE_FIELD(int, activeTab, -1);
    DECLARE_SERIALIZABLE_FIELD(std::list<std::shared_ptr<TabConfiguration>>, tabs);
    DECLARE_SERIALIZABLE_FIELD(actions_executor::Settings, actions_executor);
};

// Notification about user changed any settings
struct ISettingsChanged : ext::events::IBaseEvent
{
    virtual ~ISettingsChanged() = default;
    virtual void OnSettingsChanged() = 0;
};
