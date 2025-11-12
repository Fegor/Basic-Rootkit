#pragma once

#include "resource.h"

class CMfcHarnessApp : public CWinApp
{
public:
    CMfcHarnessApp() noexcept;

    virtual BOOL InitInstance() override;
    DECLARE_MESSAGE_MAP()
};
