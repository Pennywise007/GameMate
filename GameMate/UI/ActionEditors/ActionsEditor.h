#pragma once

#include "afxext.h"

#include <ext/core/check.h>

#include <core/Settings.h>

struct ActionsEditor : CFormView {
    using CFormView::CFormView;

    enum class Editor
    {
        eAction,
        eBind,
        eCursorPosition,
        eMouseMove,
        eScript
    };
    static ActionsEditor* CreateEditor(CWnd* parent, Editor editor);

    virtual void SetAction(const Action& action) = 0;
    virtual Action GetAction() = 0;
    virtual bool CanClose() const = 0;

public: // CFormView
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
};
