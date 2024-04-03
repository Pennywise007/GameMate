#pragma once

#include <map>
#include <string>
#include <list>

#include <ext/core/singleton.h>
#include <ext/serialization/iserializable.h>

struct Macros : ext::serializable::SerializableObject<Macros> {
    struct Action : ext::serializable::SerializableObject<Action> {
        enum Type {
            eUp = 0,    // key up or mouse wheel
            eDown,      // key down or mouse wheel
            eWait
        };

        Type type;
        int keyId;
    };

    DECLARE_SERIALIZABLE_FIELD(std::list<Action>, actions);
};

struct TabConfiguration : ext::serializable::SerializableObject<TabConfiguration> {
    DECLARE_SERIALIZABLE_FIELD(bool, enabled, true);
    DECLARE_SERIALIZABLE_FIELD(bool, gameMode, true);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, tabName);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, exeName);
    DECLARE_SERIALIZABLE_FIELD((std::map<int, Macros>), macroses);
};

class Settings : ext::serializable::SerializableObject<Settings>
{
    friend ext::Singleton<Settings>;

    Settings();
    ~Settings();

public:
    DECLARE_SERIALIZABLE_FIELD(bool, working, true);
    DECLARE_SERIALIZABLE_FIELD(int, activeTab, -1);
    DECLARE_SERIALIZABLE_FIELD(int, enableButton, -1);
    DECLARE_SERIALIZABLE_FIELD(std::list<std::shared_ptr<TabConfiguration>>, tabs);
};

