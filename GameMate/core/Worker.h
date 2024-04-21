#pragma once

#include "Settings.h"

#include <ext/thread/thread.h>

class Worker : ext::events::ScopeSubscription<ISettingsChanged>
{
    friend ext::Singleton<Worker>;

    Worker();
    ~Worker();

public:
    void OnSettingsChangedByUser() override;

private:
    void WorkingThread(std::list<TabConfiguration> activeTabSettings);

private:
    ext::thread m_workingThread;
};
