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

    void Execute(std::stop_token stopToken) const;
};
