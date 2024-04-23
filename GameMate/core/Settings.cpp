#include "pch.h"
#include "Settings.h"
#include "WinUser.h"

#include <ext/core/check.h>
#include <ext/scope/defer.h>
#include <ext/std/string.h>

using namespace ext::serializer;

constexpr auto kFileName = L"settings.txt";

Settings::Settings()
{
    try
    {
        std::wifstream file(std::filesystem::get_exe_directory() / kFileName);
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

        std::wofstream file(std::filesystem::get_exe_directory() / kFileName);
        EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
        EXT_DEFER(file.close());
        file << settings;
    }
    catch (const std::exception&)
    {
        MessageBox(NULL, ext::ManageExceptionText(L"").c_str(), L"Failed to save settings", MB_ICONERROR);
    }
}

std::optional<Action> Action::FromMessage(MSG* pMsg)
{
	auto wParam = pMsg->wParam;

	bool convert = false;
	switch (pMsg->message)
	{
	case WM_KEYUP:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		{
			convert = true;

			UINT scancode = (pMsg->lParam & 0x00ff0000) >> 16;
			int extended = (pMsg->lParam & 0x01000000) != 0;

			switch (wParam) {
			case VK_SHIFT:
				wParam = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
				break;
			case VK_CONTROL:
				wParam = extended ? VK_RCONTROL : VK_LCONTROL;
				break;
			case VK_MENU:
				wParam = extended ? VK_RMENU : VK_LMENU;
				break;
			}
		}
		break;
	default:
		convert = (pMsg->message > WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST);
		break;
	}

	if (convert)
	{
		Action result;

		result.messageId = pMsg->message;
		result.wParam = wParam;
		result.lParam = pMsg->lParam;

		return result;
	}

	return std::nullopt;
}

std::wstring Action::ToString() const
{
	std::wstring actionString;
	switch (messageId)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		{
			actionString.resize(MAX_PATH, L'\0');
			if (auto l = GetKeyNameText((LONG)lParam, actionString.data(), MAX_PATH); l != 0)
				actionString.resize(l);
			else
			{
				EXT_TRACE_ERR() << "Failed to get key text for lparam " << lParam << ", err = " << GetLastError();
				actionString = std::to_wstring(wParam);
			}
		}
		break;
	case WM_LBUTTONDOWN:
		actionString = L"Left click";
		break;
	case WM_LBUTTONDBLCLK:
		actionString = L"Left Dbl click";
		break;
	case WM_LBUTTONUP:
		actionString = L"Left click";
		break;
	case WM_RBUTTONDOWN:
		actionString = L"Right click";
		break;
	case WM_RBUTTONDBLCLK:
		actionString = L"Right Dbl click";
		break;
	case WM_RBUTTONUP:
		actionString = L"Right click";
		break;
	case WM_MBUTTONDOWN:
		actionString = L"Middle click";
		break;
	case WM_MBUTTONDBLCLK:
		actionString = L"Middle Dbl click";
		break;
	case WM_MBUTTONUP:
		actionString = L"Middle click";
		break;
	case WM_XBUTTONDOWN:
		actionString = L"Mouse X click";
		break;
	case WM_XBUTTONDBLCLK:
		actionString = L"Mouse X Dbl click";
		break;
	case WM_XBUTTONUP:
		actionString = L"Mouse X click";
		break;
	case WM_MOUSEWHEEL:
		actionString = L"Mouse wheel";
		break;
	case WM_MOUSEHWHEEL:
		actionString = L"Mouse H wheel";
		break;
	default:
		//EXT_ASSERT(false) << "Unknown message id " << messageId; TODO uncomment after serialization fix
		actionString = (std::to_wstring(messageId) + L": " + std::to_wstring(wParam)).c_str();
		break;
	}

	return actionString;
}

std::optional<MacrosAction> MacrosAction::GetMacrosActionFromMessage(MSG* pMsg, long long delay)
{
	auto action = Action::FromMessage(pMsg);
	if (!action.has_value())
		return std::nullopt;

	MacrosAction macrosAction;
	macrosAction.action = std::move(*action);
	macrosAction.delayInMilliseconds = delay;
	return macrosAction;
}

std::wstring MacrosAction::ToString() const
{
	std::wstring actionPrefix;
	switch (action.messageId)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		actionPrefix = L"↓ ";
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		actionPrefix = L"↑ ";
		break;
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		actionPrefix = GET_WHEEL_DELTA_WPARAM(action.wParam) < 0 ? L"↓ " : L"↑ ";
		break;
	default:
		break;
	}

	return actionPrefix + action.ToString();
}

void MacrosAction::ExecuteAction() const
{
	Sleep(DWORD(delayInMilliseconds));

	DWORD dwFlags = 0;
	DWORD dwData = 0;

	switch (action.messageId)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		return keybd_event(action.wParam, BYTE((action.lParam & 0x00ff0000) >> 16), 0, 0);
	case WM_KEYUP:
	case WM_SYSKEYUP:
		return keybd_event(action.wParam, BYTE((action.lParam & 0x00ff0000) >> 16), KEYEVENTF_KEYUP, 0);
	case WM_LBUTTONDOWN:
		dwFlags = MOUSEEVENTF_LEFTDOWN;
		break;
	case WM_LBUTTONUP:// TODO CHECK DBL CLICK WHICH IS NOT IN LIST
		dwFlags = MOUSEEVENTF_LEFTUP;
		break;
	case WM_RBUTTONDOWN:
		dwFlags = MOUSEEVENTF_RIGHTDOWN;
		break;
	case WM_RBUTTONUP:
		dwFlags = MOUSEEVENTF_RIGHTUP;
		break;
	case WM_MBUTTONDOWN:
		dwFlags = MOUSEEVENTF_MIDDLEDOWN;
		break;
	case WM_MBUTTONUP:
		dwFlags = MOUSEEVENTF_MIDDLEUP;
		break;
	case WM_XBUTTONDOWN:
		dwFlags = MOUSEEVENTF_XDOWN;
		break;
	case WM_XBUTTONUP:
		dwFlags = MOUSEEVENTF_XUP;
		break;
	case WM_MOUSEWHEEL:
		dwFlags = MOUSEEVENTF_WHEEL;
		dwData = GET_WHEEL_DELTA_WPARAM(action.wParam) < 0 ? -WHEEL_DELTA : WHEEL_DELTA;
		break;
	case WM_MOUSEHWHEEL:
		dwFlags = MOUSEEVENTF_HWHEEL;
		dwData = GET_WHEEL_DELTA_WPARAM(action.wParam) < 0 ? -WHEEL_DELTA : WHEEL_DELTA;
		break;
	default:
		EXT_ASSERT(false) << L"Unknown mouse message " << action.messageId;
		break;
	}

	CPoint pt;
	GetCursorPos(&pt);

	mouse_event(MOUSEEVENTF_ABSOLUTE | dwFlags, pt.x, pt.y, dwData, 0);
}

std::optional<Bind> Bind::GetBindFromMessage(MSG* pMsg)
{
	auto action = Action::FromMessage(pMsg);
	if (!action.has_value())
		return std::nullopt;

	Bind bind;
	bind.action = std::move(*action);

	// For optimization we store only DOWN bindings
	switch (bind.action.messageId)
	{
	case WM_KEYUP:
		bind.action.messageId = WM_KEYDOWN;
		break;
	case WM_SYSKEYUP:
		bind.action.messageId = WM_SYSKEYDOWN;
		break;
	case WM_LBUTTONUP:
		bind.action.messageId = WM_LBUTTONDOWN;
		break;
	case WM_RBUTTONUP:
		bind.action.messageId = WM_RBUTTONDOWN;
		break;
	case WM_MBUTTONUP:
		bind.action.messageId = WM_MBUTTONDOWN;
		break;
	case WM_XBUTTONUP:
		bind.action.messageId = WM_XBUTTONDOWN;
		break;
	}

	return bind;
}

bool Bind::IsBind(UINT commandMessageId, WPARAM commandWParam) const
{
	switch (commandMessageId)
	{
	case WM_KEYUP:
		// For some reason hook determines WM_KEYUP instead of the WM_SYSKEYUP
		return (action.messageId == WM_KEYDOWN || action.messageId == WM_SYSKEYDOWN) && action.wParam == commandWParam;
	case WM_KEYDOWN:
		return action.messageId == WM_KEYDOWN && action.wParam == commandWParam;
	case WM_SYSKEYUP:
		EXT_ASSERT(false) << "It was calling WM_KEYUP for left alt in hook, check if we need to fix WM_KEYUP case";
		[[fallthrough]];
	case WM_SYSKEYDOWN:
		return action.messageId == WM_SYSKEYDOWN && action.wParam == commandWParam;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		return action.messageId == WM_LBUTTONDOWN;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		return action.messageId == WM_RBUTTONDOWN;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		return action.messageId == WM_MBUTTONDOWN;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		return action.messageId == WM_XBUTTONDOWN;
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		return action.messageId == commandMessageId && GET_WHEEL_DELTA_WPARAM(action.wParam) == GET_WHEEL_DELTA_WPARAM(commandWParam);
	default:
		return action.messageId == commandMessageId;
	}
}
