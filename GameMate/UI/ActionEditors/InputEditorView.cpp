#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "core/events.h"

#include "InputManager.h"
#include "UI/ActionEditors/InputEditorView.h"

namespace {

constexpr auto kTimerIdActionsSubscription = 1;
constexpr auto kSubscribeToActionInMs = 300;
constexpr auto kTimerIdKeyTextChage = 2;

} // namespace

CInputEditorBaseView::CInputEditorBaseView()
	: InputEditor(IDD_VIEW_EDIT_INPUT)
{
}

BEGIN_MESSAGE_MAP(CInputEditorBaseView, InputEditor)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

void CInputEditorBaseView::DoDataExchange(CDataExchange* pDX)
{
	InputEditor::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ACTION, m_editAction);
}

void CInputEditorBaseView::PostInit(const std::shared_ptr<IBaseInput>& input)
{
	m_editableInput = input;
	m_editAction.SetWindowTextW(GetActionEditText().c_str());
	m_editAction.HideCaret();

	GetDlgItem(IDC_STATIC_DESCRIPTION)->SetWindowTextW(GetDescription());

	ext::send_event(&IKeyHandlerBlocker::OnBlockHandler);
}

std::shared_ptr<IBaseInput> CInputEditorBaseView::TryFinishDialog()
{
	return m_editableInput;
}

void CInputEditorBaseView::OnDestroy()
{
	KillTimer(kTimerIdActionsSubscription);
	if (m_keyPressedSubscriptionId != -1)
		InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
	InputEditor::OnDestroy();

	ext::send_event(&IKeyHandlerBlocker::OnUnblockHandler);
}

void CInputEditorBaseView::OnTimer(UINT_PTR nIDEvent)
{
	KillTimer(nIDEvent);

	switch (nIDEvent)
	{
	case kTimerIdActionsSubscription:
		m_keyPressedSubscriptionId = InputManager::AddKeyOrMouseHandler([&](WORD vkCode, bool isDown) -> bool {
			bool res = false;
			ext::InvokeMethod([&]() {
				auto owner = GetOwner();
				if (owner != GetForegroundWindow())
					return;

				switch (vkCode)
				{
				case VK_LBUTTON:
				{
					CPoint cursor;
					::GetCursorPos(&cursor);

					if (auto window = CWnd::FromHandle(::WindowFromPoint(cursor)); window)
					{
						wchar_t className[64];
						GetClassName(window->m_hWnd, className, 64);

						// Don't store click on parent controls like OK button or combobox with mode switchers
						const auto allowClick = [&]()
							{
								if (std::wstring_view(className) == L"ComboLBox")
									return true;
								if (window->GetParent() != owner)
									return true;
								// Allow click on dialog or statics
								for (const auto* cl : { RUNTIME_CLASS(CDialog), RUNTIME_CLASS(CFormView), RUNTIME_CLASS(CStatic) })
								{
									if (window->IsKindOf(cl))
										return true;
								}

								return false;
							};

						if (!allowClick())
							return;
					}
					m_editableInput->UpdateInput(vkCode, isDown);

					// Set timer to change displayed text because user might hit close button with mouse
					// and if we change text immediately the user will see Mouse down as a key text which wasn't his last input
					KillTimer(kTimerIdKeyTextChage);
					SetTimer(kTimerIdKeyTextChage, 50, nullptr);

					m_editAction.HideCaret();
					return;
				}
				}

				m_editableInput->UpdateInput(vkCode, isDown);
				m_editAction.SetWindowTextW(GetActionEditText().c_str());
				res = true;
			});

			return res;
		});
		break;
	case kTimerIdKeyTextChage:
		m_editAction.SetWindowTextW(GetActionEditText().c_str());
		break;
	default:
		ASSERT(FALSE);
		break;
	}

	InputEditor::OnTimer(nIDEvent);
}

void CInputEditorBaseView::OnShowWindow(BOOL bShow, UINT nStatus)
{
	InputEditor::OnShowWindow(bShow, nStatus);

	if (!bShow)
	{
		KillTimer(kTimerIdActionsSubscription);
		if (m_keyPressedSubscriptionId != -1)
			InputManager::RemoveKeyOrMouseHandler(m_keyPressedSubscriptionId);
		m_keyPressedSubscriptionId = -1;
	}
	else
		SetTimer(kTimerIdActionsSubscription, kSubscribeToActionInMs, nullptr);
}

IMPLEMENT_DYNCREATE(CKeyEditorView, CInputEditorBaseView)

const wchar_t* CKeyEditorView::GetDescription() const
{
	return L"Press mouse button or keyboard key";
}

std::wstring CKeyEditorView::GetActionEditText() const
{
	if (std::dynamic_pointer_cast<Key>(m_editableInput)->vkCode == kNotSetVkCode)
		return L"Enter action...";
	else
		return m_editableInput->ToString();
}

std::shared_ptr<IBaseInput> CKeyEditorView::TryFinishDialog()
{
	if (std::dynamic_pointer_cast<Key>(m_editableInput)->vkCode == kNotSetVkCode)
	{
		::MessageBox(m_hWnd, L"You need to enter action, press any key or mouse button", L"No action selected", MB_OK);
		return nullptr;
	}

	return CInputEditorBaseView::TryFinishDialog();
}

IMPLEMENT_DYNCREATE(CBindEditorView, CInputEditorBaseView)

const wchar_t* CBindEditorView::GetDescription() const
{
	return L"Press mouse button or keyboard key";
}

std::wstring CBindEditorView::GetActionEditText() const
{
	if (std::dynamic_pointer_cast<Bind>(m_editableInput)->vkCode == kNotSetVkCode)
		return L"Enter bind...";
	else
		return m_editableInput->ToString();
}

std::shared_ptr<IBaseInput> CBindEditorView::TryFinishDialog()
{
	if (std::dynamic_pointer_cast<Bind>(m_editableInput)->vkCode == kNotSetVkCode)
	{
		::MessageBox(m_hWnd, L"You need to enter bind, press any key or mouse button", L"No bind selected", MB_OK);
		return nullptr;
	}

	return CInputEditorBaseView::TryFinishDialog();
}

IMPLEMENT_DYNCREATE(CActionEditorView, CInputEditorBaseView)

const wchar_t* CActionEditorView::GetDescription() const
{
	return L"Press mouse button or keyboard key";
}

std::wstring CActionEditorView::GetActionEditText() const
{
	if (std::dynamic_pointer_cast<Action>(m_editableInput)->vkCode == kNotSetVkCode)
		return L"Enter action...";
	else
		return m_editableInput->ToString();
}

std::shared_ptr<IBaseInput> CActionEditorView::TryFinishDialog()
{
	if (std::dynamic_pointer_cast<Action>(m_editableInput)->vkCode == kNotSetVkCode)
	{
		::MessageBox(m_hWnd, L"You need to enter action, press any key or mouse button", L"No action selected", MB_OK);
		return nullptr;
	}

	return CInputEditorBaseView::TryFinishDialog();
}
