#pragma once

#include "resource.h"
#include <afxdialogex.h>
#include <TlHelp32.h>
#include <vector>
#include <memory>
#include <mutex>
#include "ProcessUtils.h"
#include "InjectionManager.h"
#include "SimpleHttpServer.h"
#include "JsonHelpers.h"
#include "StringHelpers.h"

class CMfcHarnessDlg : public CDialogEx
{
public:
    CMfcHarnessDlg() noexcept;

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MFCHARNESS_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;

    virtual BOOL OnInitDialog() override;
    afx_msg void OnDestroy();
    afx_msg void OnBnClickedBrowseDll();
    afx_msg void OnBnClickedRefreshProcesses();
    afx_msg void OnBnClickedInject();
    afx_msg void OnEnChangeProcessFilter();
    DECLARE_MESSAGE_MAP()

private:
    void RefreshProcessList(bool enumerate = true, bool logSummary = true);
    void PopulateProcessList(bool logSummary);
    const ProcessInfo* GetVisibleProcessAt(int visibleIndex) const;
    void AppendEvent(const CString& message);
    void HandleHttpRequest(const HttpRequest& request, HttpResponse& response);
    CString GetSelectedSessionId() const;
    void SetSelectedSessionId(const CString& sessionId);

    CString m_selectedDllPath;
    CString m_currentFilter;
    CListCtrl m_processList;
    CListBox m_eventList;
    std::vector<ProcessInfo> m_processes;
    std::vector<size_t> m_visibleProcessIndices;

    std::shared_ptr<InjectionManager> m_injectionManager;
    std::unique_ptr<SimpleHttpServer> m_httpServer;

    mutable std::mutex m_sessionMutex;
    CString m_lastSessionId;
};
