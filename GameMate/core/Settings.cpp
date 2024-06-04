#include "pch.h"
#include "Settings.h"
#include "WinUser.h"
#include "InputManager.h"

#include <ext/core/check.h>
#include <ext/constexpr/map.h>
#include <ext/scope/defer.h>
#include <ext/std/string.h>
#include <ext/thread/thread.h>

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
static_assert(kExtraKeysTovkCodes.size() == size_t(Bind::ExtraKeys::LastModifierKey), "Not all extra keys set");

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

bool runScriptSilently(const std::wstring& scriptPath) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE; // Hide the window

	ZeroMemory(&pi, sizeof(pi));

	// Create a wide string command line
	std::wstring command = L"cmd.exe /C \"" + scriptPath + L"\"";

	// Create the process
	BOOL result = CreateProcess(
		NULL,                   // No module name (use command line)
		&command[0],            // Command line
		NULL,                   // Process handle not inheritable
		NULL,                   // Thread handle not inheritable
		FALSE,                  // Set handle inheritance to FALSE
		0,                      // No creation flags
		NULL,                   // Use parent's environment block
		NULL,                   // Use parent's starting directory 
		&si,                    // Pointer to STARTUPINFO structure
		&pi                     // Pointer to PROCESS_INFORMATION structure
	);

	// Check if the process creation succeeded
	if (!result) {
		// Handle error
		return false;
	}

	// Wait until child process exits
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return true;
}

} // namespace

Bind::Bind(WORD vkCode)
{
	UpdateBind(vkCode, true);
}

[[nodiscard]] std::wstring Bind::ToString() const
{
	std::wstring actionString;

	for (auto key = unsigned(ExtraKeys::FirstModifierKey), last = unsigned(ExtraKeys::LastModifierKey); key < last; ++key)
	{
		if ((extraKeys & (1u << key)) == 0)
			continue;

		auto vkCodeKey = kExtraKeysTovkCodes.get_value(ExtraKeys(key));
		if (vkCodeKey == vkCode)
			continue;

		actionString += VkCodeToText(vkCodeKey) + L" + ";
	}

	auto vkCodeText = VkCodeToText(vkCode);
	switch (vkCode)
	{
	case InputManager::VK_MOUSE_WHEEL:
	case InputManager::VK_MOUSE_HWHEEL:
		vkCodeText = ((extraKeys & (1u << (int)ExtraKeys::eScrollUp)) ? L"↑ " : L"↓ ") + vkCodeText;
		break;
	default:
		break;
	}

	actionString += vkCodeText;

	return actionString;
}

void Bind::UpdateBind(WORD _vkCode, bool down)
{
	unsigned modifiers = 0;

	switch (_vkCode)
	{
	case InputManager::VK_MOUSE_WHEEL:
	case InputManager::VK_MOUSE_HWHEEL:
		if (!down)
			modifiers = (1u << (int)ExtraKeys::eScrollUp);
		break;
	default:
		if (!down)
			return;
		break;
	}

	for (auto key = unsigned(ExtraKeys::FirstModifierKey), last = unsigned(ExtraKeys::LastModifierKey); key < last; ++key)
	{
		WORD extravkCode = kExtraKeysTovkCodes.get_value(ExtraKeys(key));
		if (_vkCode == extravkCode || !InputManager::GetKeyState(extravkCode))
			continue;

		modifiers |= (1u << key);
	}

	vkCode = _vkCode;
	extraKeys = modifiers;
}

bool Bind::IsBindPressed(WORD _vkCode, bool down) const
{
	switch (vkCode)
	{
	case InputManager::VK_MOUSE_WHEEL:
	case InputManager::VK_MOUSE_HWHEEL:
		if (down == ((extraKeys & (1u << (int)ExtraKeys::eScrollUp)) == 0))
			return false;
		break;
	default:
		// Execute macros on key down and ignore key up
		if (!down)
			return false;
		break;
	}

	bool pressed = (vkCode == _vkCode);

	for (auto key = unsigned(ExtraKeys::FirstModifierKey), last = unsigned(ExtraKeys::LastModifierKey); key < last; ++key)
	{
		if (!pressed)
			return false;

		const bool keyMustbePressed = (extraKeys & (1u << key)) != 0;
		if (keyMustbePressed)
			pressed &= InputManager::GetKeyState(kExtraKeysTovkCodes.get_value(ExtraKeys(key)));
	}

	return pressed;
}

Action Action::NewAction(WORD vkCode, bool down, unsigned delay)
{
	Action res;
	res.type = Type::eKeyOrMouseAction;
	res.vkCode = vkCode;
	res.down = down;
	res.delayInMilliseconds = delay;

	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << std::string_sprintf("New action: type %d, vkCode %hu, down %d, %hu ms delay",
		(int)res.type, res.vkCode, int(res.down), res.delayInMilliseconds);
	return res;
}

Action Action::NewMousePosition(long mouseMovedToPointX, long mouseMovedToPointY, unsigned delay)
{
	Action res;
	res.type = Type::eCursorPosition;
	res.mouseX = mouseMovedToPointX;
	res.mouseY = mouseMovedToPointY;
	res.delayInMilliseconds = delay;

	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << std::string_sprintf("type %d, pos(%ld, %ld), %hu ms delay",
		(int)res.type, res.mouseX, res.mouseY, res.delayInMilliseconds);
	return res;
}

Action Action::NewMouseMove(long mouseDeltaX, long mouseDeltaY, unsigned delay, bool directInput)
{
	Action res;
	res.type = directInput ? Type::eMouseMoveDirectInput : Type::eMouseMove;
	res.mouseX = mouseDeltaX;
	res.mouseY = mouseDeltaY;
	res.delayInMilliseconds = delay;

	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << std::string_sprintf("type %d, pos(%ld, %ld), %hu ms delay",
		(int)res.type, res.mouseX, res.mouseY, res.delayInMilliseconds);
	return res;
}

Action Action::NewRunScript(const std::wstring& scriptPath, unsigned delay)
{
	Action res;
	res.type = Type::eRunScript;
	res.scriptPath = scriptPath;
	res.delayInMilliseconds = delay;

	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << std::string_swprintf(L"type %d, path %s, %hu ms delay",
		(int)res.type, res.scriptPath.c_str(), res.delayInMilliseconds);
	return res;
}

std::wstring Action::ToString() const
{
	switch (type)
	{
	case Type::eKeyOrMouseAction:
		return (down ? L"↓ " : L"↑ ") + VkCodeToText(vkCode);
	case Type::eCursorPosition:
		return std::string_swprintf(L"Set cursor position(%ld,%ld)", mouseX, mouseY);
	case Type::eMouseMove:
		return std::string_swprintf(L"Move mouse(%ld,%ld)", mouseX, mouseY);
	case Type::eMouseMoveDirectInput:
		return std::string_swprintf(L"DirectX mouse move(%ld,%ld)", mouseX, mouseY);
	case Type::eRunScript:
		return L"Run script: " + scriptPath;
	default:
		EXT_UNREACHABLE();
	}
}

void Action::ExecuteAction(float delayRandomize) const
{
	if (delayInMilliseconds != 0)
	{
		auto randomMultiply = 1.;
		if (delayRandomize)
			randomMultiply = (100. - delayRandomize + double(rand() % int(delayRandomize * 2. * 1000.)) / 1000.) / 100.;

		auto sleepDurationInMs = std::chrono::milliseconds(long long(delayInMilliseconds * randomMultiply));
		if (sleepDurationInMs.count() < 100)
		{
			ext::this_thread::sleep_for(sleepDurationInMs);
			ext::this_thread::interruption_point();
		}
		else
			ext::this_thread::interruptible_sleep_for(sleepDurationInMs);
	}

	switch (type)
	{
	case Type::eKeyOrMouseAction:
		InputManager::SendKeyOrMouse(vkCode, down);
		break;
	case Type::eCursorPosition:
		InputManager::SetCursorPos(POINT{ mouseX, mouseY });
		break;
	case Type::eMouseMove:
		{
			auto cursor = InputManager::GetMousePosition();
			cursor.x += mouseX;
			cursor.y += mouseY;
			InputManager::SetCursorPos(POINT{ cursor.x, cursor.y });
		}
		break;
	case Type::eMouseMoveDirectInput:
		InputManager::MouseMove(POINT{ mouseX, mouseY });
		break;
	case Type::eRunScript:
		{
			auto script = scriptPath;
			auto path = std::filesystem::get_exe_directory() / scriptPath;
			if (std::filesystem::exists(path))
				script = path;
		
			if (!runScriptSilently(script))
				EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "Failed to run script, last err: " << GetLastError();
		}
		break;
	default:
		EXT_UNREACHABLE();
	}
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

			action.ExecuteAction(randomizeDelays);
		}
	}
	catch (const ext::thread::thread_interrupted&)
	{
	}
}

actions_executor::Settings::Settings()
{
	// Init vk code here to avoid calling UpdateBind before object deserialization
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
		actionsSettings.Execute();

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
