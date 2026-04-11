#include "pch.h"
#include "Actions.h"
#include "InputManager.h"

#include <ext/core/check.h>
#include <ext/constexpr/map.h>
#include <ext/reflection/enum.h>
#include <ext/std/string.h>
#include <ext/thread/thread.h>

using namespace ext::serializer;

namespace {

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

void InterruptibleSleepFor(const std::chrono::milliseconds& duration)
{
	if (duration.count() == 0)
		return;

	if (duration.count() < 100)
	{
		// interruptible_sleep_for doesn't guarantee sleep duration for small sleeps
		ext::this_thread::sleep_for(duration);
		ext::this_thread::interruption_point();
	}
	else
		ext::this_thread::interruptible_sleep_for(duration);
}

std::wstring Key::ToString() const
{
	return VkCodeToText(vkCode);
}

bool Key::IsPressed(WORD _vkCode, bool) const
{
	return vkCode == _vkCode;
}

void Key::UpdateInput(WORD _vkCode, bool)
{
	vkCode = _vkCode;
}

Bind::Bind(WORD vkCode)
{
	UpdateInput(vkCode, true);
}

[[nodiscard]] std::wstring Bind::ToString() const
{
	std::wstring actionString;

	for (size_t i = 0, size = Bind::kExtraKeys.size(); i < size; ++i)
	{
		if (!extraKeys.test(i))
			continue;

		auto vkCodeKey = Bind::kExtraKeys[i];
		if (vkCodeKey == vkCode)
			continue;

		actionString += VkCodeToText(vkCodeKey) + L" + ";
	}

	auto vkCodeText = VkCodeToText(vkCode);
	switch (vkCode)
	{
	case InputManager::VK_MOUSE_WHEEL:
	case InputManager::VK_MOUSE_HWHEEL:
		vkCodeText = (extraFlags.test(ExtraFlags::eScrollUp) ? L"↑ " : L"↓ ") + vkCodeText;
		break;
	default:
		break;
	}

	actionString += vkCodeText;

	if (whileHold)
        actionString += L" (while hold)";

	return actionString;
}

void Bind::UpdateInput(WORD _vkCode, bool down)
{
    decltype(extraKeys) newExtraKeys = 0;
    decltype(extraFlags) newExtraFlags = 0;

	switch (_vkCode)
	{
	case InputManager::VK_MOUSE_WHEEL:
	case InputManager::VK_MOUSE_HWHEEL:
		if (!down)
			newExtraFlags.set(ExtraFlags::eScrollUp);
		break;
	default:
		if (!down)
			return;
		break;
	}

	for (size_t i = 0, size = Bind::kExtraKeys.size(); i < size; ++i)
	{
		WORD extravkCode = Bind::kExtraKeys[i];
		if (_vkCode == extravkCode || !InputManager::IsKeyPressed(extravkCode))
			continue;

        newExtraKeys.set(i);
	}

	vkCode = _vkCode;
	extraKeys = std::move(newExtraKeys);
	extraFlags = std::move(newExtraFlags);
}

void Bind::SetExtraKeyPressed(WORD vkCode, bool down)
{
	for (size_t i = 0, size = Bind::kExtraKeys.size(); i < size; ++i)
	{
		if (Bind::kExtraKeys[i] == vkCode)
		{
			extraKeys.set(i, down);
			return;
		}
    }
    EXT_ASSERT(false) << "Unknown extra key vkCode: " << vkCode;
}

[[nodiscard]] bool Bind::IsPressed(WORD _vkCode, bool down) const
{
	EXT_TRACE() << EXT_FUNCTION << " " << _vkCode << (down ? "v" : "^") << ". required: " << vkCode;
	switch (vkCode)
	{
	case InputManager::VK_MOUSE_WHEEL:
	case InputManager::VK_MOUSE_HWHEEL:
		if (down == !extraFlags.test(ExtraFlags::eScrollUp))
			return false;
		break;
	default:
		// Execute macros on key down and ignore key up
		if (!down)
			return false;
		break;
	}

	bool pressed = vkCode == _vkCode;
	for (size_t i = 0, size = Bind::kExtraKeys.size(); pressed && i < size; ++i)
	{
		if (extraKeys.test(i))
			pressed &= InputManager::IsKeyPressed(Bind::kExtraKeys[i]);
	}

	return pressed;
}

void Action::UpdateInput(WORD _vkCode, bool _down)
{
	type = Type::eKeyOrMouseAction;
	vkCode = _vkCode;
	down = _down;
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

void Action::ExecuteAction(unsigned delayRandomizeInMs) const
{
	EXT_TRACE_SCOPE() << EXT_FUNCTION << ToString();

	int sleepDurationInMs = delayInMilliseconds;
	if (delayRandomizeInMs != 0 && randomizeDelay)
		sleepDurationInMs = sleepDurationInMs - delayRandomizeInMs + std::rand() % (2 * delayRandomizeInMs);

	InterruptibleSleepFor(std::chrono::milliseconds(sleepDurationInMs));

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

void Actions::Execute(std::stop_token stopToken) const
{
	EXT_TRACE_SCOPE() << EXT_FUNCTION;

	auto it = actions.begin(), end = actions.end();
	try
	{
		for (; it != end; ++it)
		{
			if (stopToken.stop_requested())
				throw ::ext::thread::thread_interrupted();

			it->ExecuteAction(enableRandomDelay ? randomizeDelayMs : 0);
		}
	}
	catch (const ext::thread::thread_interrupted&)
	{
		// if actions executor got stopped and some button was pressed and not released, we need to release it
		std::unordered_set<int> pressedKeys;
		for (auto prevActionsIt = actions.begin(); prevActionsIt != it; ++prevActionsIt)
		{
			if (prevActionsIt->type != Action::Type::eKeyOrMouseAction)
				continue;

			switch (prevActionsIt->vkCode)
			{
			case InputManager::VK_MOUSE_WHEEL:
			case InputManager::VK_MOUSE_HWHEEL:
				break;
			default:
				if (prevActionsIt->down)
					pressedKeys.emplace(prevActionsIt->vkCode);
				else
					pressedKeys.erase(prevActionsIt->vkCode);

				break;
			}
		}

		for (auto vkCode : pressedKeys)
		{
			InputManager::SendKeyOrMouse(vkCode, false);
		}
	}
}
