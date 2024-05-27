#include "pch.h"

#include "ActionsEditor.h"
#include "ActionEditors/InputEditorView.h"
#include "ActionEditors/CursorPositionEditorView.h"
#include "ActionEditors/ScriptEditorView.h"

namespace {

template <class T>
ActionsEditor* CreateView(CWnd* parent)
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
    T* res = dynamic_cast<T*>(pView);
    EXT_ASSERT(res);
    return static_cast<ActionsEditor*>(res);
}

} // namespace

ActionsEditor* ActionsEditor::CreateEditor(CWnd* parent, Editor editor)
{
    switch (editor)
    {
    case Editor::eAction:
        return CreateView<CActionEditorView>(parent);
    case Editor::eBind:
        return CreateView<CBindEditorView>(parent);
    case Editor::eMouseMove:
        return CreateView<CCursorPositionEditorView>(parent);
    case Editor::eScript:
        return CreateView<CScriptEditorView>(parent);
    default:
        EXT_UNREACHABLE();
    }
}

BOOL ActionsEditor::PreCreateWindow(CREATESTRUCT& cs)
{
    auto res = CFormView::PreCreateWindow(cs);
    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    return res;
}
