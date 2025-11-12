#include "pch.h"
#include "MainDlg.h"
#include "framework.h"
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
std::string EscapeJson(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (char ch : value)
    {
        switch (ch)
        {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped += ch; break;
        }
    }
    return escaped;
}

std::string BuildProcessesJson(const std::vector<ProcessInfo>& processes)
{
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < processes.size(); ++i)
    {
        const auto& p = processes[i];
        oss << "{";
        oss << "\"pid\":" << p.pid << ",";
        oss << "\"name\":\"" << EscapeJson(CStringToUtf8(p.name)) << "\",";
        oss << "\"architecture\":\"" << EscapeJson(CStringToUtf8(p.architecture)) << "\",";
        oss << "\"injectable\":" << (p.injectable ? "true" : "false");
        oss << "}";
        if (i + 1 < processes.size())
        {
            oss << ",";
        }
    }
    oss << "]";
    return oss.str();
}

bool MatchesFilter(const ProcessInfo& info, const CString& filter)
{
    if (filter.IsEmpty())
    {
        return true;
    }

    CString target = filter;
    target.MakeLower();

    CString name = info.name;
    name.MakeLower();

    if (name.Find(target) != -1)
    {
        return true;
    }

    CString pidText;
    pidText.Format(L"%lu", info.pid);
    CString pidLower = pidText;
    pidLower.MakeLower();

    return pidLower.Find(target) != -1;
}
}

CMfcHarnessDlg::CMfcHarnessDlg() noexcept
    : CDialogEx(IDD_MFCHARNESS_DIALOG)
{
}

void CMfcHarnessDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PROCESSES, m_processList);
    DDX_Control(pDX, IDC_LIST_EVENTS, m_eventList);
}

BEGIN_MESSAGE_MAP(CMfcHarnessDlg, CDialogEx)
    ON_WM_DESTROY()
    ON_EN_CHANGE(IDC_EDIT_PROCESS_FILTER, &CMfcHarnessDlg::OnEnChangeProcessFilter)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_DLL, &CMfcHarnessDlg::OnBnClickedBrowseDll)
    ON_BN_CLICKED(IDC_BUTTON_REFRESH, &CMfcHarnessDlg::OnBnClickedRefreshProcesses)
    ON_BN_CLICKED(IDC_BUTTON_INJECT, &CMfcHarnessDlg::OnBnClickedInject)
END_MESSAGE_MAP()

BOOL CMfcHarnessDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetIcon(::LoadIcon(nullptr, IDI_APPLICATION), TRUE);
    SetIcon(::LoadIcon(nullptr, IDI_APPLICATION), FALSE);

    m_processList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_processList.InsertColumn(0, L"PID", LVCFMT_LEFT, 60);
    m_processList.InsertColumn(1, L"进程名称", LVCFMT_LEFT, 150);
    m_processList.InsertColumn(2, L"架构", LVCFMT_LEFT, 60);
    m_processList.InsertColumn(3, L"可注入", LVCFMT_LEFT, 70);

    m_injectionManager = std::make_shared<InjectionManager>();
    m_httpServer = std::make_unique<SimpleHttpServer>();
    if (!m_httpServer->Start(58888, [this](const HttpRequest& request, HttpResponse& response) {
            HandleHttpRequest(request, response);
        }))
    {
        AppendEvent(L"HTTP 控制端口启动失败 (端口 58888)。");
    }
    else
    {
        AppendEvent(L"HTTP 控制端口运行中：http://localhost:58888");
    }

    AppendEvent(L"UI 初始化完成。请选择 DLL 并刷新进程列表。");
    return TRUE;
}

void CMfcHarnessDlg::OnDestroy()
{
    if (m_httpServer)
    {
        m_httpServer->Stop();
        m_httpServer.reset();
    }
    CDialogEx::OnDestroy();
}

void CMfcHarnessDlg::OnBnClickedBrowseDll()
{
    CFileDialog dlg(TRUE, L"dll", nullptr, OFN_FILEMUSTEXIST, L"DLL Files (*.dll)|*.dll|All Files (*.*)|*.*||", this);
    if (dlg.DoModal() == IDOK)
    {
        CString error;
        if (!m_injectionManager->SelectDll(dlg.GetPathName(), error))
        {
            AfxMessageBox(error, MB_ICONERROR);
            return;
        }
        m_selectedDllPath = dlg.GetPathName();
        SetDlgItemText(IDC_EDIT_DLL_PATH, m_selectedDllPath);
        AppendEvent(L"已选择 DLL：" + m_selectedDllPath);
    }
}

void CMfcHarnessDlg::OnBnClickedRefreshProcesses()
{
    RefreshProcessList(true, true);
}

void CMfcHarnessDlg::RefreshProcessList(bool enumerate, bool logSummary)
{
    if (enumerate)
    {
        m_processes = m_injectionManager->ListProcesses();
    }
    PopulateProcessList(logSummary);
}

void CMfcHarnessDlg::PopulateProcessList(bool logSummary)
{
    m_processList.DeleteAllItems();

    CString currentFilter;
    GetDlgItemText(IDC_EDIT_PROCESS_FILTER, currentFilter);
    currentFilter.MakeLower();
    m_currentFilter = currentFilter;

    m_visibleProcessIndices.clear();

    int rowIndex = 0;
    for (size_t i = 0; i < m_processes.size(); ++i)
    {
        const auto& row = m_processes[i];
        if (!MatchesFilter(row, m_currentFilter))
        {
            continue;
        }

        m_visibleProcessIndices.push_back(i);

        CString pidText;
        pidText.Format(L"%lu", row.pid);
        const int idx = m_processList.InsertItem(rowIndex, pidText);
        m_processList.SetItemText(idx, 1, row.name);
        m_processList.SetItemText(idx, 2, row.architecture);
        m_processList.SetItemText(idx, 3, row.injectable ? L"Yes" : L"No");
        if (!row.injectable)
        {
            m_processList.SetItemState(idx, LVIS_CUT, LVIS_CUT);
        }
        ++rowIndex;
    }

    if (logSummary)
    {
        CString summary;
        summary.Format(L"筛选后显示 %d / %zu 个进程。", rowIndex, m_processes.size());
        AppendEvent(summary);
    }
}

void CMfcHarnessDlg::OnEnChangeProcessFilter()
{
    RefreshProcessList(false, false);
}

const ProcessInfo* CMfcHarnessDlg::GetVisibleProcessAt(int visibleIndex) const
{
    if (visibleIndex < 0 || visibleIndex >= static_cast<int>(m_visibleProcessIndices.size()))
    {
        return nullptr;
    }
    const size_t originalIndex = m_visibleProcessIndices[visibleIndex];
    if (originalIndex >= m_processes.size())
    {
        return nullptr;
    }
    return &m_processes[originalIndex];
}

void CMfcHarnessDlg::OnBnClickedInject()
{
    if (!m_injectionManager->HasSelectedDll())
    {
        AfxMessageBox(L"请先选择 DLL。", MB_ICONINFORMATION);
        return;
    }

    const int selectedIndex = m_processList.GetNextItem(-1, LVNI_SELECTED);
    if (selectedIndex == -1)
    {
        AfxMessageBox(L"请选择一个可注入的进程。", MB_ICONINFORMATION);
        return;
    }

    const ProcessInfo* selectedProcess = GetVisibleProcessAt(selectedIndex);
    if (!selectedProcess)
    {
        AfxMessageBox(L"无法定位所选进程，请刷新后重试。", MB_ICONWARNING);
        return;
    }

    if (!selectedProcess->injectable)
    {
        AfxMessageBox(L"所选进程无法注入 (缺少权限)。", MB_ICONWARNING);
        return;
    }

    const auto result = m_injectionManager->InjectIntoProcess(selectedProcess->pid, selectedProcess->architecture);
    if (!result.success)
    {
        AfxMessageBox(result.error, MB_ICONERROR);
        return;
    }

    SetSelectedSessionId(result.session.id.c_str());
    CString msg;
    msg.Format(L"已注入 DLL 至 PID %lu。Session = %s", selectedProcess->pid, result.session.id.c_str());
    AppendEvent(msg);
}

void CMfcHarnessDlg::AppendEvent(const CString& message)
{
    CTime now = CTime::GetCurrentTime();
    CString line;
    line.Format(L"[%s] %s", now.Format(L"%H:%M:%S"), message.GetString());
    m_eventList.AddString(line);
    const int count = m_eventList.GetCount();
    if (count > 0)
    {
        m_eventList.SetCurSel(count - 1);
    }
}

void CMfcHarnessDlg::HandleHttpRequest(const HttpRequest& request, HttpResponse& response)
{
    try
    {
        if (request.method == "GET" && request.path == "/processes")
        {
            const auto processes = m_injectionManager->ListProcesses();
            response.body = BuildProcessesJson(processes);
            return;
        }

        if (request.method == "POST" && request.path == "/dll")
        {
            auto dllPath = JsonExtractString(request.body, "path");
            if (!dllPath)
            {
                response.status = 400;
                response.body = "{\"error\":\"缺少 path\"}";
                return;
            }
            CString error;
            CString dllPathWide = Utf8ToCString(*dllPath);
            if (!m_injectionManager->SelectDll(dllPathWide, error))
            {
                response.status = 400;
                response.body = "{\"error\":\"" + EscapeJson(CStringToUtf8(error)) + "\"}";
                return;
            }
            response.status = 204;
            return;
        }

        if (request.method == "POST" && request.path == "/inject")
        {
            auto pid = JsonExtractInt(request.body, "pid");
            auto arch = JsonExtractString(request.body, "architecture");
            if (!pid || !arch)
            {
                response.status = 400;
                response.body = "{\"error\":\"缺少 pid/architecture\"}";
                return;
            }
            const auto result = m_injectionManager->InjectIntoProcess(static_cast<DWORD>(*pid), Utf8ToCString(*arch));
            if (!result.success)
            {
                response.status = 409;
                response.body = "{\"error\":\"" + EscapeJson(CStringToUtf8(result.error)) + "\"}";
                return;
            }
            SetSelectedSessionId(result.session.id.c_str());
            const std::string sessionIdUtf8 = EscapeJson(CStringToUtf8(result.session.id.c_str()));
            response.body = "{\"sessionId\":\"" + sessionIdUtf8 + "\",\"status\":\"HiddenVerified\",\"hiddenProcessCount\":0,\"hiddenFileCount\":0}";
            return;
        }

        if (request.method == "POST" && request.path == "/killswitch")
        {
            // Feature removed; keep endpoint for backward compatibility
            response.status = 410;
            response.body = "{\"error\":\"Kill switch removed\"}";
            return;
        }

        response.status = 404;
        response.body = "{\"error\":\"Not Found\"}";
    }
    catch (...)
    {
        response.status = 500;
        response.body = "{\"error\":\"服务器内部错误\"}";
    }
}

CString CMfcHarnessDlg::GetSelectedSessionId() const
{
    std::lock_guard<std::mutex> lock(m_sessionMutex);
    return m_lastSessionId;
}

void CMfcHarnessDlg::SetSelectedSessionId(const CString& sessionId)
{
    std::lock_guard<std::mutex> lock(m_sessionMutex);
    m_lastSessionId = sessionId;
}
