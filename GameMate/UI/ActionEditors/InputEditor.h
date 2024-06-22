#pragma once

#include "afxext.h"

#include <ext/core/check.h>

#include <core/Settings.h>

struct InputEditor : CFormView {
    using CFormView::CFormView;

    enum class EditorType
    {
        eKey,
        eAction,
        eBind,
        eCursorPosition,
        eMouseMove,
        eScript
    };
    static InputEditor* CreateEditor(
        CWnd* parent,
        EditorType editorType,
        const std::shared_ptr<IBaseInput>& input);

    virtual void PostInit(const std::shared_ptr<IBaseInput>& baseInput) = 0;
    virtual std::shared_ptr<IBaseInput> TryFinishDialog() = 0;

public: // CFormView
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
};
