#pragma once

#include <map>
#include <string>
#include <list>
#include <optional>

#include <ext/core/singleton.h>
#include <ext/core/dispatcher.h>
#include <ext/serialization/iserializable.h>
// TODO rework to inheritance
struct Action : ext::serializable::SerializableObject<Action> {
    // Convert message to action
    [[nodiscard]] static std::optional<Action> FromMessage(MSG* pMsg);
    // Get text
    std::wstring ToString() const;

    DECLARE_SERIALIZABLE_FIELD(UINT, messageId);
    DECLARE_SERIALIZABLE_FIELD(WPARAM, wParam);
    DECLARE_SERIALIZABLE_FIELD(LPARAM, lParam);
};

struct Bind : ext::serializable::SerializableObject<Bind>
{
    // Comparision operator for map
    int operator<(const Bind& other) const { return ToString() < other.ToString(); }
    // Convert message to bind
    [[nodiscard]] static std::optional<Bind> GetBindFromMessage(MSG* pMsg);
    // Get text
    std::wstring ToString() const { return action.ToString(); }
    // Check if message equal bind
    bool IsBind(UINT messageId, WPARAM wParam) const;

    DECLARE_SERIALIZABLE_FIELD(Action, action);
};

struct MacrosAction : ext::serializable::SerializableObject<MacrosAction> {
    // Convertion message to action
    [[nodiscard]] static std::optional<MacrosAction> GetMacrosActionFromMessage(MSG* pMsg, long long delay);
    // Get action text
    std::wstring ToString() const;
    // Execution action
    void ExecuteAction() const;

    DECLARE_SERIALIZABLE_FIELD(Action, action);
    DECLARE_SERIALIZABLE_FIELD((long long), delayInMilliseconds, 0);
};

struct Macros : ext::serializable::SerializableObject<Macros> {
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
    DECLARE_SERIALIZABLE_FIELD((std::map<Bind, Macros>), macrosByBind); // TODO check support of the 2 macroses
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

// Notification about user changed any settings
struct ISettingsChanged : ext::events::IBaseEvent
{
    virtual ~ISettingsChanged() = default;
    virtual void OnSettingsChangedByUser() = 0; // TODO check that it called everywhere
};
