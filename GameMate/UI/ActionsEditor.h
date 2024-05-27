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
        eMouseMove,
        eScript
    };

    virtual void SetAction(const Action& action) = 0;
    virtual Action GetAction() = 0;
    virtual bool CanClose() const = 0;

    static ActionsEditor* CreateEditor(CWnd* parent, Editor editor);

    BOOL PreCreateWindow(CREATESTRUCT& cs);
};
