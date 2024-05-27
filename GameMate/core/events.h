#pragma once

#include <ext/core/dispatcher.h>

// Notification about user changed any settings
struct ISettingsChanged : ext::events::IBaseEvent
{
    enum class ChangedType
    {
        eGeneralSettings,               // General settings like traces/selected mode changed
        eInputSimulator,                // Input simulator changed 
        eProcessToolkit,                // Settings of the process toolkit changed
        eActionsExecutor,               // Settings of the actions executor changed
        eActionsExecutorEnableChanged   // Actions executor enabled state changed
    };
    virtual ~ISettingsChanged() = default;
    virtual void OnSettingsChanged(ChangedType changedMode) = 0;
};

// Interface for blocking key handlers
struct IKeyHandlerBlocker : ext::events::IBaseEvent
{
    virtual ~IKeyHandlerBlocker() = default;
    virtual void OnBlockHandler() = 0;
    virtual void OnUnblockHandler() = 0;
};