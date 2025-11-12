#include "pch.h"
#include "framework.h"
#include "MfcHarness.h"
#include "MainDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMfcHarnessApp, CWinApp)
END_MESSAGE_MAP()

CMfcHarnessApp::CMfcHarnessApp() noexcept
{
    SetAppID(L"BasicRootkit.MfcHarness.1");
}

CMfcHarnessApp theApp;

BOOL CMfcHarnessApp::InitInstance()
{
    CWinApp::InitInstance();

    INITCOMMONCONTROLSEX InitCtrls{ sizeof(INITCOMMONCONTROLSEX) };
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    ::InitCommonControlsEx(&InitCtrls);

    auto* pShellManager = new CShellManager;
    CMfcHarnessDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    delete pShellManager;
    return FALSE;
}
