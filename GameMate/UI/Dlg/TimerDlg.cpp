#include "pch.h"
#include "resource.h"

#include "core/Settings.h"

#include "UI/Dlg/TimerDlg.h"
#include "UI/Dlg/TimerSettingsDlg.h"

#include <Controls/Layout/Layout.h>

namespace {

constexpr auto kTimerFontName = _T("Arial");
constexpr auto kHideTimerId = 0;
constexpr auto kHideInterfaceTimerInterval = 1000;
constexpr SIZE kMinimumTimerWindowSize = { 150, 170 };

} // namespace

IMPLEMENT_DYNAMIC(CTimerWindow, CWnd)

BEGIN_MESSAGE_MAP(CTimerWindow, CWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CTimerWindow::StartTimer()
{
	m_timerRunning = true;
	m_startPoint = std::chrono::high_resolution_clock::now();
	SetTimer(kUpdateWindowTimerID, 13, nullptr);
}

void CTimerWindow::StopTimer()
{
	m_timerRunning = false;
	KillTimer(kUpdateWindowTimerID);
	m_savedMilliseconds += std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::high_resolution_clock::now() - m_startPoint);
}

void CTimerWindow::ResetTimer()
{
	m_startPoint = std::chrono::high_resolution_clock::now();
	m_savedMilliseconds = std::chrono::milliseconds{};
	RedrawWindow();
}

void CTimerWindow::DisplayHours(bool show)
{
	m_displayHours = show;
	recalcFont();
	RedrawWindow();
}

void CTimerWindow::SetColors(COLORREF backColor, COLORREF textColor)
{
	m_backColor = backColor;
	m_textColor = textColor;
	RedrawWindow();
}

void CTimerWindow::OnPaint()
{
	const CString text = getDisplayText();

	CRect rect;
	GetClientRect(&rect);

	CPaintDC dcPaint(this);
	CMemDC memDC(dcPaint, rect);
	CDC& dc = memDC.GetDC();

	LOGFONT logfont = {};
	wcscpy_s(logfont.lfFaceName, kTimerFontName);
	logfont.lfHeight = m_logFontSize;
	CFont font;
	font.CreateFontIndirect(&logfont);

	dc.FillSolidRect(rect, m_backColor);

	CFont* pOldFont = dc.SelectObject(&font);

	const CSize textSize = dc.GetTextExtent(text);
	CPoint textStartPos = rect.CenterPoint();
	textStartPos.Offset(-textSize.cx / 2, -textSize.cy / 2);

	dc.TextOutW(textStartPos.x, textStartPos.y, text);

	dc.SelectObject(pOldFont);
}

void CTimerWindow::OnTimer(UINT_PTR nIDEvent)
{
	EXT_ASSERT(nIDEvent == kUpdateWindowTimerID);

	RedrawWindow();

	CWnd::OnTimer(nIDEvent);
}

void CTimerWindow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	recalcFont();
}

void CTimerWindow::recalcFont()
{
	const CString currentText = getDisplayText();

	CRect rect;
	GetClientRect(rect);
	LONG cx = rect.Width(), cy = rect.Height();

	// Calculating text font to fit the rect
	CClientDC dc(this);

	LOGFONT logfont = {};
	wcscpy_s(logfont.lfFaceName, kTimerFontName);
	logfont.lfHeight = -10;

	CFont font;
	CSize textSize, textSize2;
	do
	{
		--logfont.lfHeight;
		font.DeleteObject();
		font.CreateFontIndirect(&logfont);
		CFont* pOldFont = dc.SelectObject(&font);
		textSize = dc.GetTextExtent(currentText);
		textSize.cy = -logfont.lfHeight;
		dc.SelectObject(pOldFont);
	} while (textSize.cx <= cx && textSize.cy <= cy);
	m_logFontSize = ++logfont.lfHeight;
}

CString CTimerWindow::getDisplayText() const
{
	std::chrono::milliseconds currentTime = m_savedMilliseconds;
	if (m_timerRunning)
		currentTime += std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now() - m_startPoint);

	CString text;
	auto hours = std::chrono::duration_cast<std::chrono::hours>(currentTime);
	currentTime -= hours;
	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(currentTime);
	currentTime -= minutes;
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime);
	currentTime -= seconds;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime);
	if (m_displayHours)
		text.Format(L"%02d:%02d:%02lld.%02lld", hours.count(), minutes.count(), seconds.count(), milliseconds.count() / 100);
	else
		text.Format(L"%02d:%02lld.%02lld", minutes.count(), seconds.count(), milliseconds.count() / 100);

	return text;
}

IMPLEMENT_DYNAMIC(CTimerDlg, CDialogEx)

CTimerDlg::CTimerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_TIMER, pParent)
{}

void CTimerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_START, m_checkStart);
	DDX_Control(pDX, IDC_BUTTON_RESET, m_buttonReset);
	DDX_Control(pDX, IDC_MFCBUTTON_TIMER_SETTIGNS, m_buttonTimerSettings);
	DDX_Control(pDX, IDC_STATIC_TIMER, m_timerWindow);
}

BEGIN_MESSAGE_MAP(CTimerDlg, CDialogEx)
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_START, &CTimerDlg::OnBnClickedCheckStart)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CTimerDlg::OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_MFCBUTTON_TIMER_SETTIGNS, &CTimerDlg::OnBnClickedMfcbuttonTimerSettigns)
END_MESSAGE_MAP()

BOOL CTimerDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_buttonTimerSettings.SetBitmap(IDB_PNG_SETTINGS, Alignment::CenterCenter);

	updateButtonText();

	const auto& timerWindowRect = ext::get_singleton<Settings>().timer.windowRect;
	const long screenMaxWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const long screenMaxHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// Validate saved rect
	if (timerWindowRect.left >= 0 && timerWindowRect.right <= screenMaxWidth &&
		timerWindowRect.top >= 0 && timerWindowRect.bottom <= screenMaxHeight &&
		timerWindowRect.left < timerWindowRect.right && timerWindowRect.top < timerWindowRect.bottom)
	{
		MoveWindow(timerWindowRect);
	}

	const auto& timerSettings = ext::get_singleton<Settings>().timer;
	m_timerWindow.SetColors(timerSettings.backgroundColor, timerSettings.textColor);
	m_timerWindow.DisplayHours(timerSettings.displayHours);

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);
	Layout::SetWindowMinimumSize(*this, kMinimumTimerWindowSize.cx, kMinimumTimerWindowSize.cy);

	if (timerSettings.hideInterface)
		SetTimer(kHideTimerId, kHideInterfaceTimerInterval, nullptr);

	CRect timerRect;
	m_timerWindow.GetWindowRect(timerRect);
	ScreenToClient(timerRect);
	m_timerOffsetFromWindow = timerRect.TopLeft();

	return TRUE;
}

void CTimerDlg::OnTimer(UINT_PTR nIDEvent)
{
	EXT_ASSERT(nIDEvent == kHideTimerId);

	CRect rect;
	GetWindowRect(rect);

	bool bShowInterface = m_childDlgOpened || GetForegroundWindow() == this || rect.PtInRect(InputManager::GetMousePosition());
	if (m_showInterface != bShowInterface)
	{
		m_showInterface = bShowInterface;

		if (m_showInterface)
			showInterface();
		else
			hideInterface();
	}

	__super::OnTimer(nIDEvent);
}

void CTimerDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_showInterface)
	{
		m_showInterface = true;
		showInterface();
	}

	__super::OnMouseMove(nFlags, point);
}

void CTimerDlg::OnClose()
{
	ShowWindow(SW_HIDE);
	//__super::OnClose();
}

void CTimerDlg::OnDestroy()
{
	if (!m_showInterface)
		(CRect&)ext::get_singleton<Settings>().timer.windowRect = m_fullWindowRect;
	else
		GetWindowRect(ext::get_singleton<Settings>().timer.windowRect);

	__super::OnDestroy();
}

void CTimerDlg::OnBnClickedCheckStart()
{
	if (m_checkStart.GetCheck() == BST_CHECKED)
		m_timerWindow.StartTimer();
	else
		m_timerWindow.StopTimer();
	updateButtonText();
}

void CTimerDlg::OnBnClickedButtonReset()
{
	m_timerWindow.ResetTimer();
	updateButtonText();
}

void CTimerDlg::OnBnClickedMfcbuttonTimerSettigns()
{
	m_childDlgOpened = true;
	EXT_DEFER(m_childDlgOpened = false);

	auto timerDlg = CTimerSettings(this);
	if (timerDlg.DoModal() == IDCANCEL)
		return;

	const auto& timerSettings = ext::get_singleton<Settings>().timer;
	updateButtonText();
	m_timerWindow.SetColors(timerSettings.backgroundColor, timerSettings.textColor);
	m_timerWindow.DisplayHours(timerSettings.displayHours);

	if (timerSettings.hideInterface)
		SetTimer(kHideTimerId, kHideInterfaceTimerInterval, nullptr);
	else
		KillTimer(kHideTimerId);

	ext::send_event_async(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eTimer);
}

void CTimerDlg::OnShowHideTimer()
{
	ShowWindow(IsWindowVisible() ? SW_HIDE : SW_SHOW);
}

void CTimerDlg::OnStartOrPauseTimer()
{
	m_checkStart.SetCheck(m_checkStart.GetCheck() ? BST_UNCHECKED : BST_CHECKED);
	OnBnClickedCheckStart();
}

void CTimerDlg::OnResetTimer()
{
	OnBnClickedButtonReset();
}

void CTimerDlg::updateButtonText()
{
	const auto& settings = ext::get_singleton<Settings>().timer;

	CString checkText = m_checkStart.GetCheck() == BST_CHECKED ? L"Pause\n" : L"Start\n";
	m_checkStart.SetWindowTextW(checkText + settings.pauseTimerBind.ToString().c_str());
	m_buttonReset.SetWindowTextW((L"Reset\n" + settings.resetTimerBind.ToString()).c_str());
}

void CTimerDlg::showInterface()
{
	SetRedraw(FALSE);

	m_buttonTimerSettings.ShowWindow(SW_SHOW);
	m_buttonReset.ShowWindow(SW_SHOW);
	m_checkStart.ShowWindow(SW_SHOW);

	// Restore timer offset from window
	CRect timerWindowRect;
	m_timerWindow.GetWindowRect(timerWindowRect);
	timerWindowRect.OffsetRect(m_timerOffsetFromWindow);
	ScreenToClient(timerWindowRect);
	m_timerWindow.MoveWindow(timerWindowRect);

	ModifyStyle(0, WS_CAPTION | WS_THICKFRAME, SWP_FRAMECHANGED);

	MoveWindow(m_fullWindowRect, FALSE);

	Layout::AnchorWindow(m_timerWindow, *this, { AnchorSide::eRight }, AnchorSide::eRight, 100);
	Layout::AnchorWindow(m_timerWindow, *this, { AnchorSide::eBottom }, AnchorSide::eBottom, 100);

	SetRedraw(TRUE);
	RedrawWindow();

	// Restore minimum window size restrictions
	Layout::SetWindowMinimumSize(*this, kMinimumTimerWindowSize.cx, kMinimumTimerWindowSize.cy);
}

void CTimerDlg::hideInterface()
{
	GetWindowRect(m_fullWindowRect);

	CRect timerWindowRect;
	m_timerWindow.GetWindowRect(timerWindowRect);

	Layout::AnchorRemove(m_timerWindow, *this);

	// Remove timer offset from window
	CRect timerWindowRectWithoutGaps = timerWindowRect;
	timerWindowRectWithoutGaps.OffsetRect(-m_timerOffsetFromWindow);
	ScreenToClient(timerWindowRectWithoutGaps);

	SetRedraw(FALSE);

	m_timerWindow.MoveWindow(timerWindowRectWithoutGaps);

	m_checkStart.ShowWindow(SW_HIDE);
	m_buttonReset.ShowWindow(SW_HIDE);
	m_buttonTimerSettings.ShowWindow(SW_HIDE);

	// Remove minimum size restrictions
	Layout::SetWindowMinimumSize(*this, std::nullopt, std::nullopt);

	MoveWindow(timerWindowRect);
	// Remove caption after moving window because without caption we face some problems with double monitor with different DPI
	ModifyStyle(WS_CAPTION | WS_THICKFRAME, 0, SWP_FRAMECHANGED);

	SetRedraw(TRUE);
}
