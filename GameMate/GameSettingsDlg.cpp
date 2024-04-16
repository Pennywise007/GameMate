#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "GameSettingsDlg.h"
#include "MacrosEditDlg.h"
#include "BindEditDlg.h"

using namespace controls::list::widgets;

namespace {

enum Columns {
	eKeybind = 0,
	eActions,
	eRandomizeDelay
};

} // namespace

IMPLEMENT_DYNAMIC(CGameSettingsDlg, CDialogEx)

CGameSettingsDlg::CGameSettingsDlg(std::shared_ptr<TabConfiguration> configuration, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TAB_GAME_SETTINGS, pParent)
	, m_configuration(std::move(configuration))
{
}

void CGameSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_ENABLED, m_enabled);
	DDX_Control(pDX, IDC_EDIT_GAME_NAME, m_exeName);
	DDX_Control(pDX, IDC_LIST_MACROSES, m_listMacroses);
}

BEGIN_MESSAGE_MAP(CGameSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CGameSettingsDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CGameSettingsDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &CGameSettingsDlg::OnBnClickedCheckEnabled)
	ON_EN_CHANGE(IDC_EDIT_GAME_NAME, &CGameSettingsDlg::OnEnChangeEditGameName)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CGameSettingsDlg::OnLvnItemchangedListMacroses)
END_MESSAGE_MAP()

BOOL CGameSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_enabled.SetCheck(m_configuration->enabled);
	m_exeName.SetWindowTextW(m_configuration->exeName.c_str());

	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(FALSE);

	CRect rect;
	m_listMacroses.GetClientRect(rect);

	constexpr int kKeybindColumnWidth = 110;
	constexpr int kRandomizeDelayColumnWidth = 150;
	m_listMacroses.InsertColumn(Columns::eKeybind, L"Keybind", LVCFMT_CENTER, kKeybindColumnWidth);
	m_listMacroses.InsertColumn(Columns::eActions, L"Actions", LVCFMT_CENTER, rect.Width() - kKeybindColumnWidth - kRandomizeDelayColumnWidth);
	m_listMacroses.InsertColumn(Columns::eRandomizeDelay, L"Randomize delay(%)", LVCFMT_CENTER, kRandomizeDelayColumnWidth);

	LVCOLUMN colInfo;
	colInfo.mask = LVCF_FMT;
	m_listMacroses.GetColumn(Columns::eKeybind, &colInfo);
	colInfo.fmt |= LVCFMT_FIXED_WIDTH | LVCFMT_CENTER;
	m_listMacroses.SetColumn(Columns::eKeybind, &colInfo);

	m_listMacroses.SetProportionalResizingColumns({ Columns::eActions });
	
	m_listMacroses.setSubItemEditorController(Columns::eKeybind,
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& macroses = m_configuration->macrosByBind;

			ASSERT((int)macroses.size() > pParams->iItem);
			auto editableMacrosIt = std::next(macroses.begin(), pParams->iItem);

			auto bind = CBindEditDlg(parentWindow, editableMacrosIt->first).ExecModal();
			if (!bind.has_value())
				return nullptr;

			const auto bindText = bind->ToString();
			if (bindText == editableMacrosIt->first.ToString())
				return nullptr;

			if (auto sameBindIt = macroses.find(bind.value()); sameBindIt != macroses.end())
			{
				if (MessageBox((L"Bind '" + bindText + L"' already exists, do you want to replace it?").c_str(),
							   L"This bind already exist", MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL)
					return nullptr;

				auto sameItem = (int)std::distance(macroses.begin(), sameBindIt);
				auto macros = std::move(editableMacrosIt->second);
				macroses.erase(editableMacrosIt);
				macroses.erase(sameBindIt);
				m_listMacroses.DeleteItem(std::max<int>(pParams->iItem, sameItem));
				m_listMacroses.DeleteItem(std::min<int>(pParams->iItem, sameItem));

				AddNewMacros(std::move(bind.value()), std::move(macros));
			}
			else
			{
				auto currentMacros = std::move(editableMacrosIt->second);
				macroses.erase(editableMacrosIt);
				m_listMacroses.DeleteItem(pParams->iItem);
				AddNewMacros(std::move(bind.value()), std::move(currentMacros));
			}

			return nullptr;
		});
	const auto macrosEdit = [&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& macroses = m_configuration->macrosByBind;

			ASSERT((int)macroses.size() > pParams->iItem);
			auto editableMacrosIt = std::next(macroses.begin(), pParams->iItem);

			auto macros = CMacrosEditDlg(editableMacrosIt->second, this).ExecModal();
			if (!macros.has_value())
				return nullptr;

			auto bind = editableMacrosIt->first;

			macroses.erase(editableMacrosIt);
			m_listMacroses.DeleteItem(pParams->iItem);

			AddNewMacros(std::move(bind), std::move(macros.value()));

			return nullptr;
		};
	m_listMacroses.setSubItemEditorController(Columns::eActions, macrosEdit);
	m_listMacroses.setSubItemEditorController(Columns::eRandomizeDelay, macrosEdit);

	auto currentMacroses = std::move(m_configuration->macrosByBind);
	m_configuration->macrosByBind.clear();
	for (auto&& [bind, macros] : currentMacroses)
	{
		AddNewMacros(bind, std::move(macros));
	}

	return TRUE;
}

void CGameSettingsDlg::OnOK()
{
	// CDialogEx::OnOK();
}

void CGameSettingsDlg::OnCancel()
{
	// CDialogEx::OnCancel();
}

void CGameSettingsDlg::OnBnClickedButtonAdd()
{
	Macros::Action action;
	{
		auto bind = CBindEditDlg(this).ExecModal();
		if (!bind.has_value())
			return;
		action = std::move(bind.value());
	}

	auto macros = CMacrosEditDlg({}, this).ExecModal();
	if (!macros.has_value())
		return;

	AddNewMacros(std::move(action), std::move(macros.value()));
}

void CGameSettingsDlg::OnBnClickedButtonRemove()
{
	std::vector<int> selectedActions;
	m_listMacroses.GetSelectedList(selectedActions, true);

	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		m_configuration->macrosByBind.erase(std::next(m_configuration->macrosByBind.begin(), *it));
		m_listMacroses.DeleteItem(*it);
	}
}

void CGameSettingsDlg::OnBnClickedCheckEnabled()
{
	m_configuration->enabled = m_enabled.GetCheck();
}

void CGameSettingsDlg::OnEnChangeEditGameName()
{
	CString name;
	m_exeName.GetWindowTextW(name);
	m_configuration->exeName = name;
}

void CGameSettingsDlg::AddNewMacros(TabConfiguration::Keybind keybind, Macros&& newMacros)
{
	auto& macros = m_configuration->macrosByBind;
	auto it = macros.try_emplace(std::move(keybind), std::move(newMacros));
	ASSERT(it.second);

	auto item = (int)std::distance(macros.begin(), it.first);

	item = m_listMacroses.InsertItem(item, it.first->first.ToString().c_str());
	std::wstring actions;
	for (const auto& action : it.first->second.actions) {
		if (!actions.empty())
			actions += L" -> "; // TODO

		if (action.delay != 0)
			actions += L"(" + std::to_wstring(action.delay) + L"ms) ";

		actions += action.ToString();
	}
	m_listMacroses.SetItemText(item, Columns::eActions, actions.c_str());

	std::wostringstream str;
	str << it.first->second.randomizeDelays;
	m_listMacroses.SetItemText(item, Columns::eRandomizeDelay, str.str().c_str());

	m_listMacroses.SelectItem(item);
}

void CGameSettingsDlg::OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult)
{
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(m_listMacroses.GetSelectedCount() > 0);
	*pResult = 0;
}
