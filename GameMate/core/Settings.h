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
    Bind(WORD _vkKey);

    // Comparision operator for map
    int operator<(const Bind& other) const { return ToString() < other.ToString(); }

    // Get text
    [[nodiscard]] std::wstring ToString() const;
    // Check if bind was pressed
    bool IsBindPressed(WORD vkKey) const;

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
    DECLARE_SERIALIZABLE_FIELD(WORD, vkKey);
};

struct MacrosAction
{
    MacrosAction() = default;
    MacrosAction(WORD _vkKey, bool _down, unsigned _delay) : vkKey(_vkKey), down(_down), delayInMilliseconds(_delay) {}

    // Get action text
    [[nodiscard]] std::wstring ToString() const;
    // Execution action
    void ExecuteAction(float delayRandomize) const;

    REGISTER_SERIALIZABLE_OBJECT();

    DECLARE_SERIALIZABLE_FIELD(WORD, vkKey);
    DECLARE_SERIALIZABLE_FIELD(bool, down);
    DECLARE_SERIALIZABLE_FIELD(unsigned, delayInMilliseconds, 0);
};

struct Macros
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(std::list<MacrosAction>, actions);
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
    DECLARE_SERIALIZABLE_FIELD((std::map<Bind, Macros>), macrosByBind);
};

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
};

// Notification about user changed any settings
struct ISettingsChanged : ext::events::IBaseEvent
{
    virtual ~ISettingsChanged() = default;
    virtual void OnSettingsChanged() = 0;
};
