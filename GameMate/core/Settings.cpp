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

constexpr ext::constexpr_map kExtraKeysToVkKeys = {
	std::pair{Bind::ExtraKeys::LCtrl,	VK_LCONTROL},
	std::pair{Bind::ExtraKeys::RCtrl,	VK_RCONTROL},
	std::pair{Bind::ExtraKeys::LShift,	VK_LSHIFT},
	std::pair{Bind::ExtraKeys::RShift,	VK_RSHIFT},
	std::pair{Bind::ExtraKeys::LAlt,	VK_LMENU},
	std::pair{Bind::ExtraKeys::RAlt,	VK_RMENU},
	std::pair{Bind::ExtraKeys::LWin,	VK_LWIN},
	std::pair{Bind::ExtraKeys::RWin,	VK_RWIN},
};

std::wstring VkKeyToText(WORD vkKey)
{
	unsigned extFlag = 0;
	switch (vkKey)
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

	UINT scanCode = MapVirtualKey(vkKey, MAPVK_VK_TO_VSC);

	std::wstring actionString(MAX_PATH, L'\0');
	if (auto l = GetKeyNameText(scanCode << 16 | (extFlag << 24), actionString.data(), MAX_PATH); l != 0)
		actionString.resize(l);
	else
	{
		EXT_TRACE_ERR() << "Failed to get key text for vkKey " << vkKey << ", err = " << GetLastError();
		return std::to_wstring(vkKey);
	}

	return actionString;
}

} // namespace

Bind::Bind(WORD _vkKey)
	: vkKey(_vkKey)
{
	for (auto key = unsigned(ExtraKeys::FirstKey), last = unsigned(ExtraKeys::LastKey); key < last; ++key)
	{
		WORD extraVkKey = kExtraKeysToVkKeys.get_value(ExtraKeys(key));
		if (vkKey == extraVkKey || !InputManager::GetKeyState(extraVkKey))
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

		actionString += VkKeyToText(kExtraKeysToVkKeys.get_value(ExtraKeys(key))) + L" + ";
	}

	actionString += VkKeyToText(vkKey);

	return actionString;
}

bool Bind::IsBindPressed(WORD _vkKey) const
{
	bool pressed = (vkKey == _vkKey);

	for (auto key = unsigned(ExtraKeys::FirstKey), last = unsigned(ExtraKeys::LastKey); key < last; ++key)
	{
		if (!pressed)
			break;

		bool keyMustbePressed = (extraKeys & (1u << key)) != 0;
		pressed &= InputManager::GetKeyState(kExtraKeysToVkKeys.get_value(ExtraKeys(key))) == keyMustbePressed;
	}

	return pressed;
}

std::wstring MacrosAction::ToString() const
{
	return (down ? L"↓ " : L"↑ ") + VkKeyToText(vkKey);
}

void MacrosAction::ExecuteAction(float delayRandomize) const
{
	if (delayInMilliseconds != 0)
	{
		auto randomMultiply = 1.;
		if (delayRandomize)
			randomMultiply = (100. - delayRandomize + double(rand() % int(delayRandomize * 2. * 1000.)) / 1000.) / 100.;
		Sleep(DWORD(delayInMilliseconds * randomMultiply));
	}

	InputManager::SendKeyOrMouse(vkKey, down);
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
