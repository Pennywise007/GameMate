#include "pch.h"

#include "UI/ActionEditors/InputEditor.h"
#include "UI/ActionEditors/InputEditorView.h"
#include "UI/ActionEditors/CursorPositionEditorView.h"
#include "UI/ActionEditors/ScriptEditorView.h"

namespace {

template <class T>
InputEditor* CreateView(CWnd* parent, const std::shared_ptr<IBaseInput>& input)
{
    CCreateContext  ctx;
    ctx.m_pNewViewClass = RUNTIME_CLASS(T);
    ctx.m_pNewDocTemplate = NULL;
    ctx.m_pLastView = NULL;
    ctx.m_pCurrentFrame = NULL;

    CFrameWnd* pFrameWnd = (CFrameWnd*)parent;
    CFormView* pView = (CFormView*)pFrameWnd->CreateView(&ctx);
    EXT_EXPECT(pView && pView->GetSafeHwnd() != NULL);
    pView->OnInitialUpdate();
    pView->ModifyStyle(0, WS_TABSTOP);

    InputEditor* res = dynamic_cast<InputEditor*>(pView);
    EXT_EXPECT(res);

    res->PostInit(input);

    return res;
}

} // namespace

InputEditor* InputEditor::CreateEditor(
    CWnd* parent,
    EditorType editorType,
    const std::shared_ptr<IBaseInput>& input)
{
    EXT_EXPECT(!!input);
    switch (editorType)
    {
    case EditorType::eKey:
        return CreateView<CKeyEditorView>(parent, input);
    case EditorType::eAction:
        return CreateView<CActionEditorView>(parent, input);
    case EditorType::eBind:
        return CreateView<CBindEditorView>(parent, input);
    case EditorType::eCursorPosition:
        return CreateView<CCursorPositionEditorView>(parent, input);
    case EditorType::eMouseMove:
        return CreateView<CMouseMoveEditorView>(parent, input);
    case EditorType::eScript:
        return CreateView<CScriptEditorView>(parent, input);
    default:
        EXT_UNREACHABLE();
    }
}

BOOL InputEditor::PreCreateWindow(CREATESTRUCT& cs)
{
    auto res = CFormView::PreCreateWindow(cs);
    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    return res;
}
