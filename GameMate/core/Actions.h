#pragma once

#include <string>
#include <list>
#include <optional>

#include <ext/serialization/iserializable.h>

void InterruptibleSleepFor(const std::chrono::milliseconds& duration);

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
    // Set extra key pressed state, note that vkCode must be in kExtraKeys array
    void SetExtraKeyPressed(WORD vkCode, bool down);

    // Extra keyboard keys that can be used in combination with main key (vkCode) to trigger bind.
    // For example, Ctrl+Shift+F1 or Win+F2 etc.
    constexpr static std::array kExtraKeys = {
        VK_LCONTROL,
        VK_RCONTROL,
        VK_LSHIFT,
        VK_RSHIFT,
        VK_LMENU,
        VK_RMENU,
        VK_LWIN,
        VK_RWIN
    };

    enum ExtraFlags
    {
        eScrollUp,   // for mouse (h)wheel up

        eCount,
    };

    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(int, vkCode, kNotSetVkCode);
    DECLARE_OPTIONAL_SERIALIZABLE_FIELD(std::bitset<kExtraKeys.size()>, extraKeys);
    DECLARE_OPTIONAL_SERIALIZABLE_FIELD(std::bitset<ExtraFlags::eCount - 1>, extraFlags);
    // Execute the bind when key hold, not on release
    DECLARE_OPTIONAL_SERIALIZABLE_FIELD(bool, whileHold, false);
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

    void Execute(std::stop_token stopToken) const;
};
