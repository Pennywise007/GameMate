#include "pch.h"
#include "Settings.h"
#include "WinUser.h"
#include "InputManager.h"

#include <ext/core/check.h>
#include <ext/reflection/enum.h>
#include <ext/scope/defer.h>
#include <ext/std/string.h>
#include <ext/thread/thread.h>

using namespace ext::serializer;

namespace {

constexpr auto kFileName = L"settings.txt";
const auto kFullFileName = std::filesystem::get_exe_directory() / kFileName;

} // namespace

actions_executor::Settings::Settings()
{
	enableBind.vkCode = VK_F6;
}

void actions_executor::Settings::Execute() const
{
	unsigned executions = 0;
	const auto stopToken = ext::this_thread::get_stop_token();
	const auto sleepTime =
		std::chrono::minutes(repeatIntervalMinutes) +
		std::chrono::seconds(repeatIntervalSeconds) +
		std::chrono::milliseconds(repeatIntervalMilliseconds);

	while (!stopToken.stop_requested())
	{
		actions.Execute(stopToken);

		if (stopToken.stop_requested())
			return;

		switch (repeatMode)
		{
		case RepeatMode::eTimes:
			if (++executions >= repeatTimes)
				return;

			break;
		case RepeatMode::eUntilStopped:
			break;
		default:
			static_assert(ext::reflection::get_enum_size<RepeatMode>() == 2, "Not handled enum value");
			EXT_ASSERT(false) << "Unknown repeat mode " << int(repeatMode);
			break;
		}

		try
		{
			InterruptibleSleepFor(sleepTime);
		}
		catch (const ext::thread::thread_interrupted&)
		{
			return;
		}
	}
}

const std::wstring& process_toolkit::ProcessConfiguration::GetExeName() const
{
	return exeName;
}

void process_toolkit::ProcessConfiguration::SetExeName(const std::wstring& _exeName)
{
	exeName = _exeName;

	const std::wregex regexEscapeSymbols(L"[.^$|()\\[\\]{}+?\\\\]");
	const std::wstring rep(L"\\\\&");
	CString regexStr = std::regex_replace(exeName.c_str(),
		regexEscapeSymbols,
		rep,
		std::regex_constants::format_sed | std::regex_constants::match_default).c_str();
	regexStr.Replace(L"*", L".*");

	exeNameRegex = std::wregex(regexStr, std::regex::icase);
}

bool process_toolkit::ProcessConfiguration::MatchExeName(const std::wstring& _exeName) const
{
	if (!enabled || exeName.empty())
		return false;

	std::wsmatch xResults;
	return std::regex_search(_exeName, xResults, exeNameRegex);
}

void process_toolkit::ProcessConfiguration::OnDeserializationEnd()
{
	// We need to update the regex expression
	SetExeName(exeName);
}

process_toolkit::Settings::Settings()
{
	// Init vk code here to avoid calling UpdateInput before object deserialization
	enableBind.vkCode = VK_F8;
	enableBind.SetExtraKeyPressed(VK_LSHIFT, true);
}

timer::Settings::Settings()
{
	// Init vk code here to avoid calling UpdateInput before object deserialization
	startPauseTimerBind.vkCode = showTimerBind.vkCode = resetTimerBind.vkCode = VK_F7;
	showTimerBind.SetExtraKeyPressed(VK_LSHIFT, true);
	resetTimerBind.SetExtraKeyPressed(VK_LCONTROL, true);
}

Settings::Settings()
{
	if (!std::filesystem::exists(kFullFileName))
		return;
	try
	{
		std::wifstream file(kFullFileName);
		if (!file.is_open())
			return;
		EXT_DEFER(file.close());

		const std::wstring settings{ std::istreambuf_iterator<wchar_t>(file),
									 std::istreambuf_iterator<wchar_t>() };
		DeserializeFromJson(*this, settings);
	}
	catch (const std::exception&)
	{
		MessageBox(NULL, ext::ManageExceptionText(L"").c_str(), L"Failed to load settings", MB_ICONERROR);
	}
}

void Settings::SaveSettings()
{
	try
	{
		std::wstring settings;
		SerializeToJson(*this, settings);

		std::wofstream file(kFullFileName);
		EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
		EXT_DEFER(file.close());
		file << settings;
	}
	catch (const std::exception&)
	{
		MessageBox(NULL, ext::ManageExceptionText(L"").c_str(), L"Failed to save settings", MB_ICONERROR);
	}
}
