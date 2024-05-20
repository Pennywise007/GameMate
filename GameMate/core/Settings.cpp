#include "pch.h"
#include "Settings.h"
#include "WinUser.h"
#include "InputManager.h"

#include <ext/core/check.h>
#include <ext/constexpr/map.h>
#include <ext/scope/defer.h>
#include <ext/std/string.h>

using namespace ext::serializer;

namespace {

constexpr auto kFileName = L"settings.txt";
const auto kFullFileName = std::filesystem::get_exe_directory() / kFileName;

constexpr ext::constexpr_map kExtraKeysTovkCodes = {
	std::pair{Bind::ExtraKeys::LCtrl,	VK_LCONTROL},
	std::pair{Bind::ExtraKeys::RCtrl,	VK_RCONTROL},
	std::pair{Bind::ExtraKeys::LShift,	VK_LSHIFT},
	std::pair{Bind::ExtraKeys::RShift,	VK_RSHIFT},
	std::pair{Bind::ExtraKeys::LAlt,	VK_LMENU},
	std::pair{Bind::ExtraKeys::RAlt,	VK_RMENU},
	std::pair{Bind::ExtraKeys::LWin,	VK_LWIN},
	std::pair{Bind::ExtraKeys::RWin,	VK_RWIN},
};

std::wstring VkCodeToText(WORD vkCode)
{
	unsigned extFlag = 0;
	switch (vkCode)
	{
	case VK_LBUTTON:
		return L"Left click";
	case VK_RBUTTON:
		return L"Right click";
	case VK_MBUTTON:
		return L"Middle click";
	case VK_XBUTTON1:
		return L"Mouse X1 click";
	case VK_XBUTTON2:
		return L"Mouse X2 click";
	case InputManager::VK_MOUSE_WHEEL:
		return L"Mouse wheel";
	case InputManager::VK_MOUSE_HWHEEL:
		return L"Mouse H wheel";
	// extended keyboard 
	case VK_CANCEL:
	case VK_NUMLOCK:
	case VK_PRIOR:
	case VK_NEXT:
	case VK_END:
	case VK_HOME:
	case VK_INSERT:
	case VK_DELETE:
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:
	case VK_LWIN:
	case VK_RWIN:
	case VK_APPS:
	case VK_RCONTROL:
	case VK_SNAPSHOT: // TODO CHECK
	case VK_RMENU:
		extFlag = 1;
		break;
	default:
		break;
	}

	UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);

	std::wstring actionString(MAX_PATH, L'\0');
	if (auto l = GetKeyNameText(scanCode << 16 | (extFlag << 24), actionString.data(), MAX_PATH); l != 0)
		actionString.resize(l);
	else
	{
		EXT_TRACE_ERR() << "Failed to get key text for vkCode " << vkCode << ", err = " << GetLastError();
		return std::to_wstring(vkCode);
	}

	return actionString;
}

} // namespace

Bind::Bind(WORD _vkCode)
	: vkCode(_vkCode)
{
	for (auto key = unsigned(ExtraKeys::FirstKey), last = unsigned(ExtraKeys::LastKey); key < last; ++key)
	{
		WORD extravkCode = kExtraKeysTovkCodes.get_value(ExtraKeys(key));
		if (vkCode == extravkCode || !InputManager::GetKeyState(extravkCode))
			continue;

		extraKeys |= (1u << key);
	}
}

[[nodiscard]] std::wstring Bind::ToString() const
{
	std::wstring actionString;

	for (auto key = unsigned(ExtraKeys::FirstKey), last = unsigned(ExtraKeys::LastKey); key < last; ++key)
	{
		if ((extraKeys & (1u << key)) == 0)
			continue;

		actionString += VkCodeToText(kExtraKeysTovkCodes.get_value(ExtraKeys(key))) + L" + ";
	}

	actionString += VkCodeToText(vkCode);

	return actionString;
}

bool Bind::IsBindPressed(WORD _vkCode) const
{
	bool pressed = (vkCode == _vkCode);

	for (auto key = unsigned(ExtraKeys::FirstKey), last = unsigned(ExtraKeys::LastKey); key < last; ++key)
	{
		if (!pressed)
			break;

		const bool keyMustbePressed = (extraKeys & (1u << key)) != 0;
		if (keyMustbePressed)
			pressed &= InputManager::GetKeyState(kExtraKeysTovkCodes.get_value(ExtraKeys(key)));
	}

	return pressed;
}

Action::Action(WORD _vkCode, bool _down, unsigned _delay)
	: vkCode(_vkCode), down(_down), delayInMilliseconds(_delay)
{
	switch (vkCode)
	{
	case VK_LBUTTON:
	case VK_RBUTTON:
	case VK_MBUTTON:
	case VK_XBUTTON1:
	case VK_XBUTTON2:
	case InputManager::VK_MOUSE_WHEEL:
	case InputManager::VK_MOUSE_HWHEEL:
		{
			type = Type::eMouseAction;
			const auto position = InputManager::GetMousePosition();
			mouseX = position.x;
			mouseY = position.y;
		}
		break;
	default:
		type = Type::eKeyAction;
		break;
	}

	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << std::string_sprintf("New action: type %d, vkCode %hu, down %d, %hu ms delay, pos(%ld, %ld)",
																 (int)type, vkCode, int(down), delayInMilliseconds, mouseX, mouseY);
}

Action::Action(long mouseMovedToPointX, long mouseMovedToPointY)
	: type(Type::eMouseMove)
	, mouseX(mouseMovedToPointX)
	, mouseY(mouseMovedToPointY)
{
	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << std::string_sprintf("New mouse move action: type %d, pos(%ld, %ld)",
																 (int)type, mouseX, mouseY);
}

std::wstring Action::ToString() const
{
	return (down ? L"↓ " : L"↑ ") + VkCodeToText(vkCode);
}

void Action::ExecuteAction(float delayRandomize, bool applyMousePos) const
{
	if (delayInMilliseconds != 0)
	{
		auto randomMultiply = 1.;
		if (delayRandomize)
			randomMultiply = (100. - delayRandomize + double(rand() % int(delayRandomize * 2. * 1000.)) / 1000.) / 100.;
		ext::this_thread::interruptible_sleep_for(std::chrono::milliseconds(long long(delayInMilliseconds * randomMultiply)));
	}

	if (applyMousePos)
	{
		if (type != Type::eKeyAction)
			InputManager::MouseMove(POINT{ mouseX , mouseY});
	}
	else
		// We apply mouse pos in actions dialog, not in macroses where we don't have mouse move
		EXT_ASSERT(type != Type::eMouseMove);

	InputManager::SendKeyOrMouse(vkCode, down);
}

void Actions::Execute() const
{
	auto stopToken = ext::this_thread::get_stop_token();
	try
	{
		for (const auto& action : actions)
		{
			if (stopToken.stop_requested())
				return;

			action.ExecuteAction(randomizeDelays, false);
		}
	}
	catch (const ext::thread::thread_interrupted&)
	{
	}
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
		actionsSettings.Execute();

		if (stopToken.stop_requested())
			return;

		switch (repeatMode)
		{
		case RepeatMode::eTimes:
			++executions;
			if (++executions >= repeatTimes)
				return;

			break;
		case RepeatMode::eUntilStopped:
			break;
		default:
			EXT_ASSERT(false) << "Unknown repeat mode " << int(repeatMode);
			break;
		}

		try
		{
			ext::this_thread::interruptible_sleep_for(sleepTime);
		}
		catch (const ext::thread::thread_interrupted&)
		{
			return;
		}
	}
}

Settings::Settings()
{
	if (!std::filesystem::exists(kFullFileName))
		return;
	try
	{
		std::wifstream file(kFullFileName);
		EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
		EXT_DEFER(file.close());

		const std::wstring settings{ std::istreambuf_iterator<wchar_t>(file),
									 std::istreambuf_iterator<wchar_t>() };

		DeserializeObject(Factory::TextDeserializer(settings), *this);
	}
	catch (const std::exception&)
	{
		ext::ManageException("Failed to load settings");
	}
}

void Settings::SaveSettings()
{
	try
	{
		std::wstring settings;
		SerializeObject(Factory::TextSerializer(settings), *this);

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
