#pragma once

#include <map>
#include <string>
#include <list>
#include <optional>

#include <ext/core/singleton.h>
#include <ext/core/dispatcher.h>
#include <ext/serialization/iserializable.h>

struct Action
{
    // Convert message to action
    [[nodiscard]] static std::optional<Action> FromMessage(MSG* pMsg);
    // Get text
    virtual std::wstring ToString() const;

    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(UINT, messageId);
    DECLARE_SERIALIZABLE_FIELD(WPARAM, wParam);
    DECLARE_SERIALIZABLE_FIELD(LPARAM, lParam);
};

struct Bind : Action
{
    Bind() = default;
    Bind(const Action& action) : Action(action) {}

    // Convert message to bind
    [[nodiscard]] static std::optional<Bind> GetBindFromMessage(MSG* pMsg);
    // Comparision operator for map
    int operator<(const Bind& other) const { return ToString() < other.ToString(); }
    // Check if message equal bind
    bool IsBind(UINT messageId, WPARAM wParam) const;

    REGISTER_SERIALIZABLE_OBJECT(Action);
};

struct MacrosAction : Action
{
    MacrosAction() = default;
    MacrosAction(const Action& action) : Action(action) {}

    // Convertion message to action
    [[nodiscard]] static std::optional<MacrosAction> GetMacrosActionFromMessage(MSG* pMsg, long long delay);
    // Get action text
    std::wstring ToString() const override;
    // Execution action
    void ExecuteAction() const;

    REGISTER_SERIALIZABLE_OBJECT(Action);
    DECLARE_SERIALIZABLE_FIELD((long long), delayInMilliseconds, 0);
};

struct Macros
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(std::list<MacrosAction>, actions);
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
    DECLARE_SERIALIZABLE_FIELD(COLORREF, color, RGB(0, 0, 0));
};

} // namespace crosshair

struct TabConfiguration
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(bool, enabled, true);
    DECLARE_SERIALIZABLE_FIELD(bool, gameMode, true); // TODO
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
    ~Settings();

public:
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(int, activeTab, -1);
    DECLARE_SERIALIZABLE_FIELD(std::list<std::shared_ptr<TabConfiguration>>, tabs);
};

// Notification about user changed any settings
struct ISettingsChanged : ext::events::IBaseEvent
{
    virtual ~ISettingsChanged() = default;
    virtual void OnSettingsChangedByUser() = 0; // TODO check that it called everywhere
};
