#pragma once

class CLi : public CListCtrl
{
public:
    CLi() = default;

    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()
    afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
};

