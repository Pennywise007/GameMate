#include "pch.h"

#include "DirectInputManager.h"

#include <ext/core/check.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

DirectInputManager::~DirectInputManager()
{
	DeinitHandler(true);
	DeinitHandler(false);
}

unsigned DirectInputManager::AddMouseMovedHandler(HINSTANCE hInstance, OnMouseMovedCallback&& handler)
{
	auto& manager = ext::get_singleton<DirectInputManager>();
	if (!manager.InitHandler(hInstance, true))
		return -1;

	unsigned id = 0;

	std::unique_lock l(manager.m_callbackMutex);
	auto& callbacks = manager.m_mouseMoveCallbacks;
	if (!callbacks.empty())
		id = callbacks.rbegin()->first + 1;
	auto res = callbacks.try_emplace(id, std::move(handler));
	EXT_ASSERT(res.second);
	return id;
}

void DirectInputManager::RemoveMouseMovedHandler(unsigned id)
{
	auto& manager = ext::get_singleton<DirectInputManager>();
	bool noCallbacks = true;
	{
		std::unique_lock l(manager.m_callbackMutex);
		auto res = manager.m_mouseMoveCallbacks.erase(id);
		EXT_ASSERT(res == 1);
		noCallbacks = manager.m_mouseMoveCallbacks.empty();
	}
	if (noCallbacks)
		manager.DeinitHandler(true);
}

unsigned DirectInputManager::AddKeyboardHandler(HINSTANCE hInstance, OnKeyboardCallback&& handler)
{
	auto& manager = ext::get_singleton<DirectInputManager>();
	if (!manager.InitHandler(hInstance, false))
		return -1;

	unsigned id = 0;

	std::unique_lock l(manager.m_callbackMutex);
	auto& callbacks = manager.m_keyboardCallbacks;
	if (!callbacks.empty())
		id = callbacks.rbegin()->first + 1;
	auto res = callbacks.try_emplace(id, std::move(handler));
	EXT_ASSERT(res.second);
	return id;
}

void DirectInputManager::RemoveKeyboardHandler(unsigned id)
{
	auto& manager = ext::get_singleton<DirectInputManager>();
	bool noCallbacks = true;
	{
		std::unique_lock l(manager.m_callbackMutex);
		auto res = manager.m_keyboardCallbacks.erase(id);
		EXT_ASSERT(res == 1);
		noCallbacks = manager.m_keyboardCallbacks.empty();
	}
	if (noCallbacks)
		manager.DeinitHandler(false);
}

bool DirectInputManager::InitHandler(HINSTANCE hInstance, bool mouse)
{
	LPDIRECTINPUTDEVICE8& deviceToInit = mouse ? m_directInputMouseDevice : m_directInputKeyboardDevice;

	if (deviceToInit)
		return true;

	bool inited = false;
	EXT_DEFER([&]() {
		if (!inited)
			DeinitHandler(mouse);
	});

	if (!m_directInput)
	{
		HRESULT hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_directInput, nullptr);
		if (FAILED(hr)) {
			EXT_TRACE_ERR() << "DirectInput8Create failed. Error: " << std::hex << hr << std::endl;
			return false;
		}
	}

	HRESULT hr = m_directInput->CreateDevice(mouse ? GUID_SysMouse : GUID_SysKeyboard, &deviceToInit, nullptr);
	if (FAILED(hr)) {
		EXT_TRACE_ERR() << "Create device failed. Error: " << std::hex << hr << std::endl;
		return false;
	}

	hr = deviceToInit->SetDataFormat(mouse ? &c_dfDIMouse : &c_dfDIKeyboard);
	if (FAILED(hr)) {
		EXT_TRACE_ERR() << "SetDataFormat failed. Error: " << std::hex << hr << std::endl;
		return false;
	}

	hr = deviceToInit->SetCooperativeLevel(::GetDesktopWindow(), DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(hr)) {
		EXT_TRACE_ERR() << "Keyboard SetCooperativeLevel failed. Error: " << std::hex << hr << std::endl;
		return false;
	}

	inited = true;

	if (mouse)
	{
		m_mouseHandlingThread.run([&]() {
			DIMOUSESTATE mouseState;
			auto getMouseMove = [&]() -> std::optional<POINT> {
				HRESULT hr = m_directInputMouseDevice->Acquire();
				if (hr == DIERR_INPUTLOST)
					return std::nullopt;

				if (FAILED(hr)) {
					EXT_TRACE_ERR() << "Mouse acquire failed. Error: 0x" << std::hex << hr;
					return std::nullopt;
				}

				hr = m_directInputMouseDevice->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState);
				if (FAILED(hr)) {
					EXT_TRACE_ERR() << "Mouse GetDeviceState failed. Error: 0x" << std::hex << hr;
					return std::nullopt;
				}

				if (mouseState.lX == 0 && mouseState.lY == 0)
					return std::nullopt;

				return POINT{ mouseState.lX, mouseState.lY };
			};

			// Skip first move
			EXT_UNUSED(getMouseMove());

			auto stop_token = ext::this_thread::get_stop_token();
			while (!stop_token.stop_requested())
			{
				if (auto res = getMouseMove(); res.has_value())
				{
					std::unique_lock l(m_callbackMutex);
					for (auto&& [_, callback] : m_mouseMoveCallbacks)
					{
						callback(res.value());
					}
				}

				ext::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		});
	}
	else
	{
		m_keyboardHandlingThread.run([&]() {
			BYTE oldKeyStates[256];
			BYTE keyState[256];
			auto getChangedKeyStates = [&]() -> std::list<std::pair<WORD, bool>> {
				constexpr size_t keyStatesSize = sizeof(keyState);
				HRESULT hr = m_directInputKeyboardDevice->Acquire();
				if (hr == DIERR_INPUTLOST)
					return {};

				if (FAILED(hr)) {
					EXT_TRACE_ERR() << "Keyboard acquire failed. Error: 0x" << std::hex << hr;
					return {};
				}

				hr = m_directInputKeyboardDevice->GetDeviceState(keyStatesSize, &keyState);
				if (FAILED(hr)) {
					EXT_TRACE_ERR() << "Keyboard GetDeviceState failed. Error: 0x" << std::hex << hr;
					return {};
				}

				if (std::memcmp(oldKeyStates, keyState, keyStatesSize) == 0)
					return {};

				std::list<std::pair<WORD, bool>> res;
				for (int i = 0, size = keyStatesSize / sizeof(keyState[0]); i < size; ++i)
				{
					if (oldKeyStates[i] != keyState[i])
					{
						UINT key = i;
						switch (i) {
						case 203: key = VK_LEFT; break;
						case 205: key = VK_RIGHT; break;
						case 200: key = VK_UP; break;
						case 208: key = VK_DOWN; break;
						case 211: key = VK_DELETE; break;
						case 207: key = VK_END; break;
						case 199: key = VK_HOME; break; // pos1
						case 201: key = VK_PRIOR; break; // page up
						case 209: key = VK_NEXT; break;  // page down
						case 210: key = VK_INSERT; break;
						case 184: key = VK_RMENU; break; // right alt
						case 157: key = VK_RCONTROL; break; // right control
						case 219: key = VK_LWIN; break; // left win
						case 220: key = VK_RWIN; break; // right win
						case 156: key = VK_RETURN; break; // right enter
						case 181: key = VK_DIVIDE; break; // numpad divide
						case 221: key = VK_APPS; break; // menu key
						default:
							key = MapVirtualKey(i, MAPVK_VSC_TO_VK_EX);
							break;
						}

						res.emplace_back(std::make_pair<WORD, bool>(key, keyState[i] & 0x80));
						oldKeyStates[i] = keyState[i];
					}
				}

				return res;
			};

			auto stop_token = ext::this_thread::get_stop_token();

			// init states
			EXT_UNUSED(getChangedKeyStates());
			while (!stop_token.stop_requested())
			{
				if (auto keyStates = getChangedKeyStates(); !keyStates.empty())
				{
					std::unique_lock l(m_callbackMutex);
					for (auto&& [_, callback] : m_keyboardCallbacks)
					{
						for (auto&& [vkCode, down] : keyStates)
						{
							callback(vkCode, down);
						}
					}
				}

				ext::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		});
	}
	return true;
}

void DirectInputManager::DeinitHandler(bool mouse)
{
	LPDIRECTINPUTDEVICE8& deviceToDeinit = mouse ? m_directInputMouseDevice : m_directInputKeyboardDevice;
	
	if (mouse && m_mouseHandlingThread.joinable())
		m_mouseHandlingThread.interrupt_and_join();
	else if (!mouse && m_keyboardHandlingThread.joinable())
		m_keyboardHandlingThread.interrupt_and_join();

	if (deviceToDeinit) {
		deviceToDeinit->Unacquire();
		deviceToDeinit->Release();
		deviceToDeinit = nullptr;
	}

	if (m_directInput && !m_directInputMouseDevice && !m_directInputKeyboardDevice) {
		m_directInput->Release();
		m_directInput = nullptr;
	}
}
