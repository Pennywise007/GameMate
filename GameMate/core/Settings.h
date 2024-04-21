#pragma once

#include <map>
#include <string>
#include <list>
#include <optional>

#include <ext/core/singleton.h>
#include <ext/core/dispatcher.h>
#include <ext/serialization/iserializable.h>

// Notification about user changed any settings
struct ISettingsChanged : ext::events::IBaseEvent
{
    virtual ~ISettingsChanged() = default;
    virtual void OnSettingsChangedByUser() = 0;
};

struct Macros : ext::serializable::SerializableObject<Macros> {
    struct Action : ext::serializable::SerializableObject<Action> {
        enum class Type {
            eKeyUp = 0,
            eKeyDown,
            eMouse,
        };

        static std::optional<Action> GetActionFromMessage(MSG* pMsg, long long delay);

        std::wstring ToString() const;

        int operator<(const Action& other) const
        {
            return ToString() < other.ToString();
        }

        DECLARE_SERIALIZABLE_FIELD(Type, type);
        DECLARE_SERIALIZABLE_FIELD((long long), delay);
        DECLARE_SERIALIZABLE_FIELD(UINT, messageId);
        DECLARE_SERIALIZABLE_FIELD(WPARAM, wparam);
        DECLARE_SERIALIZABLE_FIELD(LPARAM, lparam);
    };

   // DECLARE_SERIALIZABLE_FIELD(Action, bind);
    DECLARE_SERIALIZABLE_FIELD(std::list<Action>, actions);
    DECLARE_SERIALIZABLE_FIELD(double, randomizeDelays, 0);
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

enum class Size {
    eSmall,  // 8x8
    eMedium, // 16x16
    eLarge   // 32x32
};

struct Settings : ext::serializable::SerializableObject<Settings> {
    DECLARE_SERIALIZABLE_FIELD(bool, show, false);
    DECLARE_SERIALIZABLE_FIELD(crosshair::Size, size, Size::eMedium);
    DECLARE_SERIALIZABLE_FIELD(crosshair::Type, type, Type::eCross);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, customCrosshairName);
    DECLARE_SERIALIZABLE_FIELD(COLORREF, color, RGB(0, 0, 0));
};

} // namespace crosshair

struct TabConfiguration : ext::serializable::SerializableObject<TabConfiguration> {
    DECLARE_SERIALIZABLE_FIELD(bool, enabled, true);
    DECLARE_SERIALIZABLE_FIELD(bool, gameMode, true); // TODO
    DECLARE_SERIALIZABLE_FIELD(bool, disableWinButton, false);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, tabName);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, exeName);
    DECLARE_SERIALIZABLE_FIELD(crosshair::Settings, crosshairSettings);

    using Keybind = Macros::Action;
    DECLARE_SERIALIZABLE_FIELD((std::map<Keybind, Macros>), macrosByBind);
};

class Settings : ext::serializable::SerializableObject<Settings>
{
    friend ext::Singleton<Settings>;

    Settings();
    ~Settings();

public:
    DECLARE_SERIALIZABLE_FIELD(int, activeTab, -1);
    DECLARE_SERIALIZABLE_FIELD(std::list<std::shared_ptr<TabConfiguration>>, tabs);
};

