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

std::optional<Macros::Action> Macros::Action::GetActionFromMessage(MSG* pMsg, long long delay)
{
	std::optional<Macros::Action> result;

	const auto addAction = [&](Macros::Action::Type type) {
		result = Macros::Action{};

		result->type = type;
		result->delay = delay;
		result->messageId = pMsg->message;
		result->wparam = pMsg->wParam;
		result->lparam = pMsg->lParam;
	};

	switch (pMsg->message)
	{
	case WM_KEYUP:
		addAction(Macros::Action::Type::eKeyUp);
		break;
	case WM_KEYDOWN:
		addAction(Macros::Action::Type::eKeyDown);
		break;
	default:
	{
		if (pMsg->message > WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST)
			addAction(Macros::Action::Type::eMouse);
		break;
	}
	}

	return result;
}

std::wstring Macros::Action::ToString() const
{
	std::wstring actionString;
	switch (type)
	{
	case Macros::Action::Type::eKeyDown:
	case Macros::Action::Type::eKeyUp:
	{
		std::wstring keyText(MAX_PATH, L'\0');
		if (auto l = GetKeyNameText((LONG)lparam, keyText.data(), MAX_PATH); l != 0)
			keyText.resize(l);
		else
		{
			EXT_TRACE_ERR() << "Failed to get key text for lparam " << lparam << ", err = " << GetLastError();
			keyText = std::to_wstring(wparam);
		}

		actionString = std::string_swprintf(L"%c %s", type == Macros::Action::Type::eKeyDown ? L'↓' : L'↑', keyText.c_str());
	}
	break;
	case Macros::Action::Type::eMouse:
		switch (messageId)
		{
		case WM_LBUTTONDOWN:
			actionString = L"↓ Left click";
			break;
		case WM_LBUTTONUP:
			actionString = L"↑ Left click";
			break;
		case WM_RBUTTONDOWN:
			actionString = L"↓ Right click";
			break;
		case WM_RBUTTONUP:
			actionString = L"↑ Right click";
			break;
		case WM_MBUTTONDOWN:
			actionString = L"↓ Middle click";
			break;
		case WM_MBUTTONUP:
			actionString = L"↑ Middle click";
			break;
		case WM_XBUTTONDOWN:
			actionString = L"↓ Mouse X down";
			break;
		case WM_XBUTTONUP:
			actionString = L"↑ Mouse X down";
			break;
		case WM_MOUSEWHEEL:
			actionString = std::wstring(GET_WHEEL_DELTA_WPARAM(wparam) < 0 ? L"↓" : L"↑") + L" Mouse wheel";
			break;
		default:
			actionString = (std::to_wstring(messageId) + L": " + std::to_wstring(wparam)).c_str();
			break;
		}
		break;
	default:
		actionString = (std::to_wstring(messageId) + L": " + std::to_wstring(wparam)).c_str();
		EXT_ASSERT(false) << "Unexpected type " << (int)type;
		break;
	}

	return actionString;
}
