#include "pch.h"
#include "Settings.h"

#include <ext/core/check.h>
#include <ext/scope/defer.h>

using namespace ext::serializer;

constexpr auto kFileName = L"settings.txt";

Settings::Settings()
{
    try
    {
        std::wifstream file(kFileName);
        EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
        EXT_DEFER(file.close());

        const std::wstring settings{ std::istreambuf_iterator<wchar_t>(file),
                                     std::istreambuf_iterator<wchar_t>() };

        Executor::DeserializeObject(Factory::TextDeserializer(settings), this);
    }
    catch (const std::exception&)
    {
        ext::ManageException("Failed to load settings");
    }
}

Settings::~Settings()
{
    try
    {
        std::wstring settings;
        Executor::SerializeObject(Factory::TextSerializer(settings), this);

        std::wofstream file(kFileName);
        EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
        EXT_DEFER(file.close());
        file << settings;
    }
    catch (const std::exception&)
    {
        ext::ManageException("Failed to save settings");
    }
}