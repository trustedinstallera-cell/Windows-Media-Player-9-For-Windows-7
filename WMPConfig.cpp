#pragma warning(disable : 4996)
#pragma prefast(disable: 28159)
#define _WIN32_WINNT 0x0501
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <codecvt>
#include <unordered_map>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>
#include <tlhelp32.h>
#include <winternl.h>   // 用于 RtlGetVersion (兼容 XP)
#include <shlwapi.h>
#include <tchar.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Version.lib")

// 常量定义
const std::string VERSION_MARKER_FILE = "1";
const std::string WMP9XP_DIR = "wmp9xp";
const std::string WMP_DIR = "wmp";

std::vector<std::string> TARGET_DIR;

// 全局变量
std::string g_scriptDir;
bool is64Bit = false; // 记录系统是否为64位
std::string usernameStr;

// 对 CPU 架构宏定义的移植
#define PROCESSOR_ARCHITECTURE_PPC              3
#define PROCESSOR_ARCHITECTURE_SHX              4
#define PROCESSOR_ARCHITECTURE_ARM              5
#define PROCESSOR_ARCHITECTURE_IA64             6
#define PROCESSOR_ARCHITECTURE_ALPHA64          7
#define PROCESSOR_ARCHITECTURE_MSIL             8
#define PROCESSOR_ARCHITECTURE_AMD64            9
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64    10
#define PROCESSOR_ARCHITECTURE_NEUTRAL          11
#define PROCESSOR_ARCHITECTURE_ARM64            12
#define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64   13
#define PROCESSOR_ARCHITECTURE_IA32_ON_ARM64    14

#if defined(__x86_64__) || defined(_M_X64)\
|| defined(__ppc64__) || defined(__PPC64__)
#define x64
#elif defined(__i386__) || defined(_M_IX86)\
|| defined(__arm__) || defined(_M_ARM)\
||defined(__ppc__) || defined(__PPC__)
#define x86
#else
#define xxx
#endif

std::ofstream file("log.txt", std::ofstream::out | std::ofstream::app);

void WriteLog(std::string, std::string);
BOOL ProcessDirectory(LPCWSTR lpszRoot);

class Lang {
private:
    static std::unordered_map<std::string, std::string> language;

public:
    static std::string get(const std::string& id) {
        if (language.count(id) == 0) {
            std::cerr << "Error: Not found language setting with id " << id << std::endl;
            WriteLog("Not found language id " + id, "Error");
            return "";
        }
    }

    static std::string get(int id) {
        std::string str(std::to_string(id));
        return get(str);
    }

    static void set(const std::string& filename = "lang.lng") {
        std::ifstream lng(filename, std::ifstream::in);
        while (!lng.is_open()) {
            std::cerr << "Error: Language file not found. File name: " << filename << std::endl;
            system("pause");
            lng.clear();
            lng.open(filename, std::ifstream::in);
        }
        std::string line;
        int cnt = 0;
        while (getline(lng,line)) {
            cnt++;
            if (line.empty()||line[0]==';') {
                continue;
            }
            
            auto frontPos = std::find(line.begin(), line.end(), '=');
            if (frontPos == line.end()) {
                std::cerr << "Error: Invalid line at line " << cnt << std::endl;
            }
            else {
                auto keyStart = line.begin();
                auto keyEnd = frontPos;
                while (keyStart != keyEnd && std::isspace(*keyStart)) ++keyStart;
                while (keyEnd != keyStart && std::isspace(*(keyEnd - 1))) --keyEnd;

                // 去除 value 两端的空格
                auto valStart = frontPos + 1;
                auto valEnd = line.end();
                while (valStart != valEnd && std::isspace(*valStart)) ++valStart;
                while (valEnd != valStart && std::isspace(*(valEnd - 1))) --valEnd;

                language[std::string(keyStart, keyEnd)] = std::string(valStart, valEnd);
            }
        }
    }
};
Lang lang;


inline std::wstring to_wstring(std::string& str) {
    int wideLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    std::wstring wideStr(wideLen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wideStr[0], wideLen);
    return wideStr;
}

inline std::string to_string(std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// 写入日志。插入\n以换行
void WriteLog(std::string str,std::string level="") {
    if (file.is_open()) {
        SYSTEMTIME now;
        GetSystemTime(&now);
        file << level<<","<<now.wYear << "-" << now.wMonth << "-" << now.wDay << " " << now.wHour << ":" << now.wMinute << ":" << now.wSecond << "." << now.wMilliseconds << "," << str << std::endl;
    }
}

BOOL IsSystem64Bit()
{
    SYSTEM_INFO si = { 0 };

    // 动态获取 GetNativeSystemInfo 函数指针（XP SP2+ 支持）
    typedef VOID(WINAPI* LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO);
    HMODULE hModule = GetModuleHandle(L"kernel32.dll");
    if (hModule) {
        LPFN_GetNativeSystemInfo pGetNativeSystemInfo =
            (LPFN_GetNativeSystemInfo)GetProcAddress(
                hModule,
                "GetNativeSystemInfo"
            );

        if (pGetNativeSystemInfo)
        {
            pGetNativeSystemInfo(&si);
        }
        else
        {
            // 不支持 GetNativeSystemInfo 的系统（XP SP1 及更早），回退到 GetSystemInfo
            WriteLog("",lang.get(2000));
            GetSystemInfo(&si);
            // 这种情况下系统一定是 32 位的（因为 64 位系统至少是 XP SP2）
            return FALSE;
        }

    }

    WriteLog("ProcessorArchitecture is " + si.wProcessorArchitecture, "Info");

    // 判断处理器架构
    return (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64);
}

BOOL DeleteKeyRecursively(HKEY hKeyRoot, LPCTSTR lpszSubKey)
{
    HKEY hKey;
    LONG lResult = RegOpenKeyEx(hKeyRoot, lpszSubKey, 0, KEY_READ | KEY_WRITE, &hKey);
    if (lResult != ERROR_SUCCESS)
        return FALSE;

    // 枚举并删除所有子项
    TCHAR szSubKeyName[256];
    DWORD dwSize = 256;
    while (RegEnumKeyEx(hKey, 0, szSubKeyName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        DeleteKeyRecursively(hKeyRoot, szSubKeyName);
        dwSize = 256;
    }

    RegCloseKey(hKey);
    if (RegDeleteKey(hKeyRoot, lpszSubKey) == ERROR_SUCCESS) {
//#error todo
    }
    return RegDeleteKey(hKeyRoot, lpszSubKey) == ERROR_SUCCESS;
}

// 辅助函数：获取最后错误字符串
std::string GetLastErrorStr() {
    DWORD err = GetLastError();
    if (err == 0) return "";
    LPSTR buf = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buf, 0, nullptr);
    std::string result(buf ? buf : "");
    LocalFree(buf);
    return result;
}

std::string GetLastErrorStr(LSTATUS ErrorCode) {
    LPVOID lpMsgBuf = nullptr;
    DWORD ret = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, ErrorCode, 0,
        (LPSTR)&lpMsgBuf, 0, NULL);
    if (ret == 0 || lpMsgBuf == nullptr) {
        return "";
    }
    std::string result(static_cast<const char*>(lpMsgBuf));
    LocalFree(lpMsgBuf);

    // 去掉末尾换行符
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    return result;
}

// 检查文件是否存在
bool FileExists(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

// 检查目录是否存在
bool DirExists(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

// 执行命令，等待完成并返回退出码
int ExecuteCommand(const std::string& cmd, bool wait = true, bool showWindow = false) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!showWindow) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    if (!CreateProcessA(nullptr, const_cast<LPSTR>(cmd.c_str()), nullptr, nullptr,
        FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        std::cerr << "执行命令失败: " << cmd << " 错误: " << GetLastErrorStr() << std::endl;
        WriteLog(lang.get(2101) + GetLastErrorStr(), lang.get(2002));
        return -1;
    }
    if (wait) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exitCode;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}

// 检查当前进程是否以管理员身份运行
bool IsAdmin() {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        WriteLog(lang.get(2102), lang.get(2002));
        return false;
    }
    TOKEN_ELEVATION elevation;
    DWORD size = sizeof(TOKEN_ELEVATION);
    if (!GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) {
        WriteLog(lang.get(2103), lang.get(2002));
        CloseHandle(hToken);
        return false;
    }
    CloseHandle(hToken);
    return elevation.TokenIsElevated != 0;
}

// 以管理员权限重新启动自身
void RunAsAdmin() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    ShellExecuteA(nullptr, "runas", exePath, nullptr, nullptr, SW_SHOW);
    exit(0);
}

// 启用特权
BOOL EnableShutdownPrivilege(DWORD DesiredAccess)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    // 1. 打开当前进程的访问令牌
    if (!OpenProcessToken(GetCurrentProcess(),
        DesiredAccess,
        &hToken))
    {
        printf("OpenProcessToken 失败，错误码: %lu\n", GetLastError());
        return FALSE;
    }

    // 2. 获取关机特权的 LUID
    if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid))
    {
        printf("LookupPrivilegeValue 失败，错误码: %lu\n", GetLastError());
        CloseHandle(hToken);
        return FALSE;
    }

    // 3. 设置特权结构
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;  // 启用特权

    // 4. 调整令牌特权
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
    {
        printf("AdjustTokenPrivileges 失败，错误码: %lu\n", GetLastError());
        CloseHandle(hToken);
        return FALSE;
    }

    // 5. 检查是否成功启用了特权
    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        printf("警告：未获得所有特权，可能权限不足\n");
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

struct WindowsVersion {
    int major = -1;
    int minor = -1;
    int build = -1;
};
WindowsVersion GetOSVersion() {
    WindowsVersion version;
    WCHAR sysPath[MAX_PATH];
    if (GetSystemDirectoryW(sysPath, MAX_PATH) == 0)
        return version;

    wcscat_s(sysPath, L"\\ntoskrnl.exe");

    DWORD verSize = GetFileVersionInfoSizeW(sysPath, NULL);
    if (verSize == 0)
        return version;

    BYTE* verData = new BYTE[verSize];
    if (!GetFileVersionInfoW(sysPath, 0, verSize, verData)) {
        delete[] verData;
        return version;
    }

    VS_FIXEDFILEINFO* pFileInfo = NULL;
    UINT len = 0;
    if (!VerQueryValueW(verData, L"\\", (VOID**)&pFileInfo, &len)) {
        delete[] verData;
        return version;
    }

    version.major = HIWORD(pFileInfo->dwFileVersionMS);
    version.minor = LOWORD(pFileInfo->dwFileVersionMS);
    version.build = HIWORD(pFileInfo->dwFileVersionLS);

    delete[] verData;
    return version;
}

// 获取磁盘剩余空间（字节）
ULONGLONG GetFreeSpaceEx(const std::string& path) {
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExA(path.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
        return freeBytesAvailable.QuadPart;
    return 0;
}

// 递归复制目录
bool CopyDirectory(const std::string& src, const std::string& dst) {
    if (!DirExists(src)) return false;
    if (!CreateDirectoryA(dst.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
        return false;

    std::string searchPath = src + "\\*.*";
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        return false;

    do {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
            continue;
        std::string srcFile = src + "\\" + ffd.cFileName;
        std::string dstFile = dst + "\\" + ffd.cFileName;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!CopyDirectory(srcFile, dstFile))
                return false;
        }
        else {
            if (!CopyFileA(srcFile.c_str(), dstFile.c_str(), FALSE))
                return false;
        }
    } while (FindNextFileA(hFind, &ffd) != 0);
    FindClose(hFind);
    return true;
}

// 注册目录下所有 .dll 和 .ax 文件（调用 regsvr32）
void RegisterFiles(std::string& dir) {
    std::string searchPath = dir + "\\*.dll";
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string currentFile = dir + "\\" + ffd.cFileName;
                std::cout << "正在注册 " << currentFile << " ..." << std::endl;
                std::string cmd = "regsvr32 /s \"" + currentFile + "\"";
                ExecuteCommand(cmd, true, false);
            }
        } while (FindNextFileA(hFind, &ffd) != 0);
        FindClose(hFind);
    }
    searchPath = dir + "\\*.ax";
    hFind = FindFirstFileA(searchPath.c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string currentFile = dir + "\\" + ffd.cFileName;
                std::cout << "正在注册 " << currentFile << " ..." << std::endl;
                std::string cmd = "regsvr32 /s \"" + currentFile + "\"";
                ExecuteCommand(cmd, true, false);
            }
        } while (FindNextFileA(hFind, &ffd) != 0);
        FindClose(hFind);
    }
}

// 获取当前用户的 SID（字符串形式）
std::string GetCurrentUserSid() {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return "";

    DWORD size = 0;
    GetTokenInformation(hToken, TokenUser, nullptr, 0, &size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(hToken);
        return "";
    }
    std::vector<BYTE> buffer(size);
    PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.data());
    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, size, &size)) {
        CloseHandle(hToken);
        return "";
    }
    CloseHandle(hToken);

    LPSTR sidStr = nullptr;
    if (!ConvertSidToStringSidA(pTokenUser->User.Sid, &sidStr))
        return "";
    std::string result(sidStr);
    LocalFree(sidStr);
    return result;
}

bool GetCurrentUserSid(PSID* ppSid) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwSize);

    PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);
    if (!pTokenUser) {
        CloseHandle(hToken);
        return false;
    }

    BOOL result = GetTokenInformation(hToken, TokenUser,
        pTokenUser, dwSize, &dwSize);
    if (result) {
        // 复制SID
        DWORD sidSize = GetLengthSid(pTokenUser->User.Sid);
        *ppSid = malloc(sidSize);
        CopySid(sidSize, *ppSid, pTokenUser->User.Sid);
    }

    free(pTokenUser);
    CloseHandle(hToken);
    return result == TRUE;
}

bool SetRegistryAuthority(wchar_t* path) {
    WCHAR UserName[512] = { 0 };
    DWORD cbUserName = sizeof(UserName) / sizeof(WCHAR);
    PSID pSid = NULL;
    DWORD cbSid = 0;
    WCHAR DomainBuffer[512] = { 0 };
    DWORD cbDomainBuffer = sizeof(DomainBuffer) / sizeof(WCHAR);
    SID_NAME_USE eUse;
    bool bResult = false;

    // 1. 获取当前用户名 (使用宽字符版本)
    if (!GetUserNameW(UserName, &cbUserName)) {
        std::cout<<"获取用户名失败: "<< GetLastError()<<"\n";
        return false;
    }

    // 2. 第一次调用获取 SID 所需缓冲区大小
    if (!LookupAccountNameW(NULL, UserName, NULL, &cbSid,
        DomainBuffer, &cbDomainBuffer, &eUse)) {
        DWORD err = GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER) {
            std::cout << "获取SID大小失败: " << err << "\n";
            return false;
        }
    }

    // 3. 分配 SID 缓冲区
    pSid = (PSID)malloc(cbSid);
    if (!pSid) {
        std::cout<<"内存分配失败\n";
        return false;
    }

    // 4. 第二次调用获取实际 SID
    if (!LookupAccountNameW(NULL, UserName, pSid, &cbSid,
        DomainBuffer, &cbDomainBuffer, &eUse)) {
        std::cout << "获取 SID 失败: " << GetLastError() << "\n";
        goto cleanup;
    }

    {
        // 5. 更改注册表项所有者
        DWORD dwResult = SetNamedSecurityInfoW(
            path,                       // 注册表路径（宽字符）
            SE_REGISTRY_KEY,            // 对象类型
            OWNER_SECURITY_INFORMATION, // 更改所有者
            pSid,                       // 新所有者 SID
            NULL, NULL, NULL);

        if (dwResult == ERROR_SUCCESS) {
            std::cout << "成功更改注册表项所有者\n";
            bResult = true;
        }
        else {
            std::cout << "更改注册表所有者失败: " << dwResult << "\n";
            switch (dwResult) {
            case ERROR_ACCESS_DENIED:
                std::cout << "原因：拒绝访问（需要管理员权限）\n";
                break;
            case ERROR_FILE_NOT_FOUND:
                std::cout << "原因：注册表项不存在\n";
                break;
            case ERROR_INVALID_PARAMETER:
                std::cout << "原因：参数无效（请检查路径格式）\n";
                break;
            }
        }
    }
cleanup:
    free(pSid);
    return bResult;
}

// 设置注册表项为只读：拒绝所有人写入，允许所有人读取
bool SetReadOnlyRegistryKeyWithDeny(const wchar_t* path) {

    PSID pEveryoneSid = NULL;
    PACL pNewDacl = NULL;
    bool bSuccess = false;

    // 1. 获取 Everyone SID (S-1-1-0)
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
        0, 0, 0, 0, 0, 0, 0, &pEveryoneSid)) {
        std::cout << "分配 Everyone SID 失败: " << GetLastError() << "\n";
        return false;
    }

    // 2. 定义写入权限掩码（拒绝这些权限）
    DWORD dwDenyMask = KEY_SET_VALUE |          // 设置值
        KEY_CREATE_SUB_KEY |     // 创建子项
        DELETE |                 // 删除项
        KEY_WRITE;               // 标准写入权限（包含上述两项）

// 3. 定义读取权限掩码（允许这些权限）
    DWORD dwAllowMask = KEY_READ |              // 标准读取权限
        KEY_QUERY_VALUE |       // 查询值
        KEY_ENUMERATE_SUB_KEYS |// 枚举子项
        KEY_NOTIFY;             // 通知变更

// 4. 构建两个 ACE：先拒绝（DENY），后允许（ALLOW）
    EXPLICIT_ACCESS_W ea[2] = { 0 };

    // 拒绝写入 ACE
    ea[0].grfAccessPermissions = dwDenyMask;
    ea[0].grfAccessMode = DENY_ACCESS;          // 拒绝访问
    ea[0].grfInheritance = NO_INHERITANCE;      // 不继承到子项，除非是之后发现子项也有问题
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName = (LPWSTR)pEveryoneSid;

    // 允许读取 ACE
    ea[1].grfAccessPermissions = dwAllowMask;
    ea[1].grfAccessMode = SET_ACCESS;           // 允许访问
    ea[1].grfInheritance = NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[1].Trustee.ptstrName = (LPWSTR)pEveryoneSid;

    // 5. 创建新的 DACL（替换原有 DACL，仅包含这两个 ACE）
    DWORD dwResult = SetEntriesInAclW(2, ea, NULL, &pNewDacl);
    if (dwResult != ERROR_SUCCESS) {
        std::cout << "创建 ACL 失败: " << dwResult << "\n";
        goto cleanup;
    }

    // 6. 应用新的 DACL 到注册表项
    dwResult = SetNamedSecurityInfoW(
        (LPWSTR)path,               // 注册表路径（格式如 L"CURRENT_USER\\Software\\MyApp"）
        SE_REGISTRY_KEY,
        DACL_SECURITY_INFORMATION,  // 只修改 DACL
        NULL, NULL,                 // 不更改所有者、主组
        pNewDacl,                   // 新 DACL
        NULL);                      // 不更改 SACL

    if (dwResult == ERROR_SUCCESS) {
        std::cout << "成功设置注册表项为只读（显式拒绝写入）\n";
        bSuccess = true;
    }
    else {
        std::cout << "设置失败: " << dwResult << "\n";
        switch (dwResult) {
        case ERROR_ACCESS_DENIED:
            std::cout << "原因：权限不足（需要管理员或 WRITE_DAC 权限）\n";
            break;
        case ERROR_FILE_NOT_FOUND:
            std::cout << "原因：注册表项不存在\n";
            break;
        }
    }

cleanup:
    if (pNewDacl) LocalFree(pNewDacl);
    if (pEveryoneSid) FreeSid(pEveryoneSid);
    return bSuccess;
}

// 写入注册表项（MultiUsers 部分）
void WriteMediaPlayerRegistry() {
    HKEY hKey = nullptr;
    LONG ret = RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\MediaPlayer\\Preferences",
        0,
        KEY_WRITE,  // 如果需要写入
        &hKey);

    if (ret == ERROR_FILE_NOT_FOUND) {
        // 键不存在，创建它
        ret = RegCreateKeyExA(HKEY_CURRENT_USER,
            "Software\\Microsoft\\MediaPlayer\\Preferences",
            0,
            nullptr,
            0,
            KEY_WRITE,  // 创建时请求写入权限
            nullptr,
            &hKey,
            nullptr);
        if (ret != ERROR_SUCCESS) {
            std::cerr << "无法创建注册表项，错误: " << ret << std::endl;
            return;
        }
    }
    else if (ret != ERROR_SUCCESS) {
        // 其他错误（如权限不足）
        std::cerr << "无法打开注册表项，错误: " << ret << std::endl;
        return;
    }

    auto setDword = [&](const char* name, DWORD value) {
        RegSetValueExA(hKey, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    };
    auto setSz = [&](const char* name, const char* value) {
        RegSetValueExA(hKey, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value), static_cast<DWORD>(strlen(value) + 1));
    };
    auto setBinary = [&](const char* name, const BYTE* data, size_t len) {
        RegSetValueExA(hKey, name, 0, REG_BINARY, data, static_cast<DWORD>(len));
    };

    setDword("AcceptedPrivacyStatement", 1);
    setDword("AppColorLimited", 0);
    setSz("DefaultSubscriptionService", "Bing");
    setDword("DisableMRUMusic", 0);
    setDword("DisableMRUPictures", 0);
    setDword("DisableMRUPlaylists", 0);
    setDword("DisableMRUVideo", 0);
    setDword("EverLoadedServices", 1);
    setDword("LastContainerMode", 0);
    setSz("LastContainerV12", "{70C02500-7C6F-11D3-9FB6-00105AA620BB}");
    setDword("LaunchIndex", 1);
    setDword("LibraryBackgroundImage", 6);
    setDword("LibraryForceShowColumns", 0);
    setDword("LibraryHasBeenPopulated", 1);
    setDword("LibraryHMENodesVisible", 1);
    setDword("MetadataRetrieval", 3);
    setDword("MigratedXML", 1);
    setDword("Migrating", 0);
    setDword("MLSChangeIndexList", 84);
    setDword("MLSChangeIndexMusic", 3);
    setDword("MLSChangeIndexPhoto", 8);
    setDword("MLSChangeIndexVideo", 2);
    // MostRecentFileAddOrRemove: 10cb19e521b2dc01 (8 bytes)
    BYTE binData1[] = { 0x10, 0xcb, 0x19, 0xe5, 0x21, 0xb2, 0xdc, 0x01 };
    setBinary("MostRecentFileAddOrRemove", binData1, sizeof(binData1));
    BYTE binData2[] = { 0x00 };
    setBinary("SendUserGUID", binData2, sizeof(binData2));
    setDword("SetHMEPermissionsOnDBDone", 1);
    setDword("SilentAcquisition", 1);
    setDword("SQMLaunchIndex", 1);
    setDword("TranscodedFilesCacheDefaultSizeSet", 1);
    setDword("TranscodedFilesCacheSize", 6143);
    setDword("TreeQueryWatcher", 2);
    setDword("UsageLoggerRanOnce", 1);
    setDword("UsageTracking", 1);

    RegCloseKey(hKey);
    std::cout << "注册表项已添加完成，Windows Media Player 将跳过首次运行配置。" << std::endl;
}

// 导入 .reg 文件（简单实现：直接调用 reg.exe import）
void ImportRegFile(const std::string& regFile) {
    if (FileExists(regFile)) {
        std::string cmd = "reg import \"" + regFile + "\"";
        ExecuteCommand(cmd, true, false);
        std::cout << "注册表文件导入完成。" << std::endl;
    }
    else {
        std::cerr << "警告: 找不到注册表文件 " << regFile << std::endl;
    }
}

// 显示菜单
void ShowMenu() {
    std::cout << "\n[0] 退出程序\n";
    std::cout << "[1] 执行部署过程\n";
    std::cout << "[2] 检查依赖项目\n";
    std::cout << "[3] 查看已知限制\n";
    std::cout << "[4] 执行多用户部署\n";
    //std::cout << "[5] 执行故障排查程序\n";
    std::cout << "请选择一个选项: ";
}

// 获取用户选择
int GetChoice() {
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return -1;
    char ch = line[0];
    return ch - '0';
    //return -1;
}

// 选项 3：已知限制
void ShowLimits() {
    std::cout << "Windows Media Center 必须被卸载，目前为止没有找到解决方法。\n";
    system("pause");
}

// 选项 2：检查依赖
void CheckDependencies() {
    std::cout << "正在检查依赖项...\n";

    // 检查 regsvr32.exe
    if (FileExists("C:\\Windows\\System32\\regsvr32.exe") || FileExists(".\\regsvr32.exe"))
        std::cout << "regsvr32.exe 存在\n";
    else
        std::cout << "Fatal Error: 找不到 regsvr32.exe。请从其他计算机上复制文件到本脚本同级目录下。\n";

    // 检查 reg.exe
    if (FileExists("C:\\Windows\\System32\\reg.exe") || FileExists(".\\reg.exe"))
        std::cout << "reg.exe 存在\n";
    else
        std::cout << "Warning: 找不到 reg.exe，需要保证 regedit.exe 存在并手动导入 .reg 文件\n";

    // 检查 wmp9xp 文件夹
    if (DirExists(g_scriptDir + "\\" + WMP9XP_DIR))
        std::cout << "wmp9xp 文件夹存在\n";
    else
        std::cout << "Error:找不到名为 wmp9xp 的文件夹，请重新下载程序\n";

    // 检查 wmp 文件夹
    if (DirExists(g_scriptDir + "\\" + WMP_DIR))
        std::cout << "wmp 文件夹存在\n";
    else
        std::cout << "Error:找不到名为 wmp 的文件夹，请重新下载程序\n";

    if (!is64Bit) {
        if (FileExists(g_scriptDir + "\\SetOpeningMethod_x86.reg"))
            std::cout << "SetOpeningMethod.reg 存在\n";
        else
            std::cout << "Warning:找不到名为 SetOpeningMethod_x86.reg 的文件夹，需要手动配置注册表项\n";
    }
    else {
        if (FileExists(g_scriptDir + "\\SetOpeningMethod_x64.reg"))
            std::cout << "SetOpeningMethod_x64.reg 存在\n";
        else
            std::cout << "Warning:找不到名为 SetOpeningMethod_x64.reg 的文件夹，需要手动配置注册表项\n";
    }
}

// 多用户部署（选项 4）
void MultiUsersSetup() {
    std::cout << "执行多用户部署...\n";

    HKEY hKey;
    DWORD dwValue = 0;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    LONG lResult = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\wmpConfig",
        0,
        KEY_READ,
        &hKey
    );
    if (lResult == ERROR_SUCCESS) {
        // 查询 InstalledState 的值
        lResult = RegQueryValueExA(
            hKey,
            "InstalledState",    // 值名称
            NULL,               // 保留参数
            &dwType,            // 接收数据类型
            (LPBYTE)&dwValue,   // 接收数据的缓冲区
            &dwSize             // 缓冲区大小
        );

        if (lResult == ERROR_SUCCESS) {
            switch (dwValue) {
            case 2:
                break;
            default:
                std::cout << "警告：可能没有完成安装任务，或注册表项被破坏。\n";
                std::cout << "是否继续安装过程？(y/N)";
                char choice;
                std::cin >> choice;
                switch (tolower(choice)) {
                case 'y':
                    goto BeginInst;
                default:
                    exit(7);
                }
                break;
            }
        }
        else {
            printf("读取失败，错误码: %lu\n", lResult);
            std::cout << "请检查注册表项HKEY_LOCAL_MACHINE\\SOFTWARE\\wmpConfig的读取权限，然后再试一次。\n";
            std::cout << "是否继续安装过程？(y/N)";
            char choice;
            std::cin >> choice;
            switch (tolower(choice)) {
            case 'y':
                goto BeginInst;
            default:
                exit(8);
            }
        }
    }
    else {
        std::cout << "警告：可能没有完成安装任务，或注册表项被破坏。\n";
        std::cout << "是否继续安装过程？(y/N)";
        char choice;
        std::cin >> choice;
        switch (tolower(choice)) {
        case 'y':
            goto BeginInst;
        default:
            exit(8);
        }
    }

BeginInst:
    // 尝试获取 SID（仅用于显示，实际写入 HKCU）
    std::string sid = GetCurrentUserSid();
    if (!sid.empty()) {
        std::cout << "当前用户 SID: " << sid << std::endl;
    }
    else {
        std::cout << "获取 SID 失败，将直接操作 HKCU。\n";
    }

    WriteMediaPlayerRegistry();

    // 导入打开方式注册表
    if (is64Bit) {
        ImportRegFile(g_scriptDir + "\\SetOpeningMethod_x64.reg");
    }
    else {
        ImportRegFile(g_scriptDir + "\\SetOpeningMethod_x86.reg");
    }
    // 快捷方式注册（询问）
    std::cout << "重新注册快捷方式？(y/n): ";
    char ch;
    std::cin >> ch;
    //std::cin.ignore();
    if (ch == 'y' || ch == 'Y') {

        std::string dstLnk;
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) {
            typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
            RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
            if (RtlGetVersion) {
                RTL_OSVERSIONINFOW osvi = { sizeof(osvi) };
                LONG result = RtlGetVersion(&osvi);  // 调用函数填充数据
                if (result == 0) {  // 0 表示成功
                    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0) {  // Windows Vista
                        // 获取用户名环境变量

                        const int usernameSize = 32768;
                        char* username = new char[usernameSize];
                        DWORD usernameLen = GetEnvironmentVariableA("USERNAME", username, usernameSize);

                        if (usernameLen == 0) {
                            std::cerr << "错误：无法正确调用GetEnvironmentVariable函数。请手动输入用户名。\n";
                            do {
                                std::cin >> usernameStr;
                            } while ((usernameStr.empty() || usernameStr.length() >= usernameSize)
                                && std::cout << "错误：用户名应该不为空且长度小于" << usernameSize);
                        }
                        else {
                            usernameStr = username;
                        }

                        // 构建完整路径
                        dstLnk = "C:\\Users\\";
                        dstLnk += usernameStr;
                        dstLnk += "\\AppData\\Roaming\\Microsoft\\Internet Explorer\\Quick Launch";

                        delete[] username;
                    }
                    else {
                        char startMenuPath[MAX_PATH];
                        SHGetFolderPathA(nullptr, CSIDL_COMMON_PROGRAMS, nullptr, 0, startMenuPath);
                        dstLnk = std::string(startMenuPath);
                    }
                }
            }


            dstLnk += "\\Windows Media Player.lnk";
            // 复制快捷方式到开始菜单

            std::string srcLnk;
            //std::string dstLnk = std::string(startMenuPath) + "\\Windows Media Player.lnk";
            if (is64Bit) {
                srcLnk = g_scriptDir + "\\Windows Media Player.lnk";
            }
            else {
                srcLnk = g_scriptDir + "\\x86\\Windows Media Player.lnk";
            }
            if (FileExists(srcLnk)) {
                if (CopyFileA(srcLnk.c_str(), dstLnk.c_str(), FALSE))
                    std::cout << "快捷方式复制成功。\n";
                else
                    std::cerr << "快捷方式复制失败: " << GetLastErrorStr() << std::endl;
            }
            else {
                std::cerr << "找不到快捷方式文件。\n";
            }
            // 尝试固定到任务栏
            // 已经弃用此功能，因为方案并不稳定


        }
    }
EndCopyingLink:
    // 打开方式注册（再次询问，但已导入过）
    std::cout << "重新注册打开方式？(y/n): ";
    std::cin >> ch;
    std::cin.ignore();
    if (ch == 'y' || ch == 'Y') {
        if (is64Bit) {
            ImportRegFile(g_scriptDir + "\\SetOpeningMethod_x64.reg");
        }
        else {
            ImportRegFile(g_scriptDir + "\\SetOpeningMethod_x86.reg");
        }
        std::cout << "所有文件关联配置完成。\n";
    }

    {
        std::cout << "正在检查错误的注册表项...\n";
        std::string path = "Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\MediaPlayer\\Setup\\Installed Versions";
        // 检查是否可以修改指定的路径
        HKEY hKey;
        DWORD dwValue = 0;
        DWORD dwType = REG_DWORD;
        DWORD dwSize = sizeof(DWORD);
        LONG lResult = RegOpenKeyExA(
            HKEY_CURRENT_USER,
            path.c_str(),
            0,
            KEY_WRITE,
            &hKey
        );
        if (lResult == ERROR_SUCCESS) {
            void RemoveFailedRegistryItems();
            RemoveFailedRegistryItems();
        }
    }

    std::cout << "多用户部署完成。\n";
    system("pause");
    exit(0);
}

BOOL CALLBACK CloseWindows(HWND hwnd, LPARAM lParam) {
    (void)lParam;
    (void)hwnd;
    HANDLE hSnapshort = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshort == INVALID_HANDLE_VALUE)
    {
        printf("CreateToolhelp32Snapshot调用失败！\n");
        return -1;
    }
    // 获得线程列表  
    PROCESSENTRY32 stcProcessInfo;
    stcProcessInfo.dwSize = sizeof(stcProcessInfo);
    BOOL  bRet = Process32First(hSnapshort, &stcProcessInfo);
    while (bRet)
    {
        for (auto& str : std::vector<std::wstring>{ L"wmplayer.exe",L"wm_setup.exe",L"ehshell.exe" }) {
            std::wstring currentProcess(stcProcessInfo.szExeFile);
            if (currentProcess == str)
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, stcProcessInfo.th32ProcessID);	//获取进程句柄
                TerminateProcess(hProcess, 0);    //结束进程
                int lastError = GetLastError();
                if (lastError != 0) {
                    std::cout << "无法结束" << to_string(str) << "。错误代码：" << lastError << "。错误消息：" << GetLastErrorStr();
                }
                CloseHandle(hProcess);
            }
        }
        bRet = Process32Next(hSnapshort, &stcProcessInfo);
    }
    CloseHandle(hSnapshort);
    return 0;
}

// 删除引发问题的注册表
// 如果不删除，那么有概率出现 wmploc.dll 版本错误弹窗，此时程序将拒绝运行
void RemoveFailedRegistryItems() {
    HKEY hKey = NULL;
    DWORD dwValue = 0;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    std::wstring path=L"Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\MediaPlayer\\Setup\\Installed Versions";
    LONG lResult = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        path.c_str(),
        0,
        KEY_READ,
        &hKey
    );
    if (lResult == ERROR_SUCCESS) { // 成功时删除整个注册表树
        BOOL result = DeleteKeyRecursively(HKEY_CURRENT_USER, path.c_str());
        if (result == FALSE) {
            std::cerr << "错误：无法删除 " << path.c_str() << " 。您可能已经阻止程序写入此注册表项。\n";
            goto CleanUp;
        }
    }

    {
        // 设置所有者
        // DeleteKeyRecursively 保留了根键，所以可以直接设置所有者
        std::wstring root = L"CURRENT_USER\\";
        std::wstring pathForOwner = root + path;

        // 使用数组（确保大小足够）
        wchar_t buffer[1024];
        wcscpy_s(buffer, pathForOwner.c_str());
        wchar_t* pathForOwnerPtr = buffer;
        if (SetRegistryAuthority(pathForOwnerPtr)) {
            // 禁止写入指定的注册表
            SetReadOnlyRegistryKeyWithDeny(pathForOwnerPtr);
        }
    }
CleanUp:
    RegCloseKey(hKey);
}

// 部署过程（选项 1）
void ExecuteDeployment() {
    // 检查必备文件夹
    int exceptionFlag = 0;
    if (!DirExists(g_scriptDir + "\\" + WMP9XP_DIR)) {
        std::cerr << "Error:找不到名为 wmp9xp 的文件夹，请重新下载程序\n";
        exceptionFlag = 1;
    }
    if (!DirExists(g_scriptDir + "\\" + WMP_DIR)) {
        std::cerr << "Error:找不到名为 wmp 的文件夹，请重新下载程序\n";
        exceptionFlag = 1;
    }
    if (exceptionFlag) {
        std::cout << "程序将退出。\n";
        system("pause");
        std::terminate();
    }

    // 检查管理员权限
    if (!IsAdmin()) {
        std::cout << "需要管理员权限，正在请求提升...\n";
        RunAsAdmin();
        return; // RunAsAdmin 会 exit
    }

    int stage = 0;

    // 判断是第一阶段还是第二阶段
JudgeForStage:
    HKEY hKey;
    DWORD dwValue = 0;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    LONG lResult = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\wmpConfig",
        0,
        KEY_READ,
        &hKey
    );
    if (lResult == ERROR_SUCCESS) {
        // 查询 InstalledState 的值
        lResult = RegQueryValueExA(
            hKey,
            "InstalledState",    // 值名称
            NULL,               // 保留参数
            &dwType,            // 接收数据类型
            (LPBYTE)&dwValue,   // 接收数据的缓冲区
            &dwSize             // 缓冲区大小
        );

        if (lResult == ERROR_SUCCESS) {
            switch (dwValue) {
            case 1:
                std::cout << "执行第二阶段安装进程...";
                stage = 2;
                break;
            case 2:
                std::cout << "注意：正在进行第二阶段覆盖执行阶段。";
                stage = 2;
                WriteLog("Performing second stage of execution.", "Info");
                system("pause");
                break;
            default:
                std::cout << "警告：可能没有完成上一阶段，或注册表项被破坏。\n";
                WriteLog("Last stage failed or registry corrupted", "Warning");
                std::cout << "是否继续安装过程？(y/N)";
                char choice;
                std::cin >> choice;
                switch (tolower(choice)) {
                case 'y':
                    stage = 2;
                    goto Execute;
                default:
                    exit(8);
                }
                break;
            }
        }
        else if (lResult == ERROR_FILE_NOT_FOUND) {
            std::cout << "执行第一阶段安装进程...";
            stage = 1;
        }
        else {
            printf("读取失败，错误码: %lu\n", lResult);
            std::cout << "请检查注册表项HKEY_LOCAL_MACHINE\\SOFTWARE\\wmpConfig的读取权限，然后再试一次。\n";
            system("pause");
            goto JudgeForStage;
        }


    }
    else {
        std::cout << "执行第一阶段安装进程...";
        stage = 1;
    }
Execute:
    std::cout << "程序将结束 Windows Media Player 与 Windows Media Center。请确保它们已经停止运行。\n";
    system("pause");
    if (EnumWindows(CloseWindows, 0)) {
        WriteLog("Failed to terminate application.", "Error");
        std::cerr << "错误：无法结束进程，请手动执行，然后按任意键。\n";
        std::cout << "注意：若没有结束进程，那么写入文件将失败，程序将无法继续执行。\n";
    }
    system("pause");
    if (stage == 1) {
        // 第一阶段
        std::cout << "第一阶段：卸载 Windows Media Center 和 Windows Media Player\n";

        // 询问是否创建系统还原点
        std::cout << "强烈建议您在运行前创建系统还原点或备份文件。\n创建系统还原点？(y/n): ";
        char ch;
        std::cin >> ch;
        //std::cin.ignore();
        if (ch == 'y' || ch == 'Y') {
            // 启用系统还原（使用 wmic，因为 API 较复杂）
            std::cout << "正在启用系统还原...\n";
            WriteLog("Enabling system restore.", "Info");
            int returnValue = ExecuteCommand("wmic /namespace:\\\\root\\default path SystemRestore call Enable \"C:\"", true, false);
            if (returnValue != 0) {
                std::cout << "错误：无法启用系统还原。" << std::endl;
                WriteLog("Cannot enable system restore.", "Error");
            }

            // 创建还原点
            std::cout << "正在创建还原点...\n";
            int ret = ExecuteCommand("wmic.exe /Namespace:\\\\root\\default Path SystemRestore Call CreateRestorePoint \"降级至 Windows Media Player 9\", 100, 7", true, false);
            if (ret != 0 && ret != 102) {
                std::cerr << "创建还原点失败，错误码: " << ret << std::endl;
                std::cout << "跳过还原点配置？(y/n): ";
                std::cin >> ch;
                std::cin.ignore();
                if (tolower(ch)=='y') {
                    std::cout << "已经跳过系统还原点\n";
                    goto EndSettingRestorePoint;
                }
            }
            else if (ret == 102) {
                std::cout << "Warning: 磁盘空间可能不足。\n";
            }
            else {
//#error todo
            }
        }
        EndSettingRestorePoint:

        ch = 'c';
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) {
            typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
            RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
            if (RtlGetVersion) {

                RTL_OSVERSIONINFOW osvi = { sizeof(osvi) };
                LONG result = RtlGetVersion(&osvi);  // ← 关键：调用函数填充数据
                if (result == 0) {  // 0 表示成功
                    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0) {  // Windows Vista

                        std::cout << "注意：Windows Vista 系统可能无法写入 Help 文件夹（程序将提示“拒绝访问”）。如果失败，您需要手动复制 *.hlp 文件到 C:\\Windows\\Help 文件夹。\n";
                        std::cout << "警告：无法证明接下来的操作不会引起任何问题。按下任意键后，若出现问题，您可能需要通过系统还原点或安装光盘进行还原。（您现在仍然可以安全退出）\n";
                        system("pause");
                        std::cout << "正在尝试更改写入权限...\n";

                        std::string filePath = is64Bit ?
                            "C:\\Program Files (x86)\\Windows Media Player" :
                            "C:\\Program Files\\Windows Media Player"; // 目标文件路径
                        std::string command =
                            "takeown /f \"" + filePath + "\" /r /d y && "  // /r 递归获取所有权，/d y 自动确认
                            "icacls \"" + filePath + "\" /grant Admin:(OI)(CI)F /t /c /q";  // /t 遍历子项，/c 继续错误，/q 安静模式
                        system(command.c_str());

                    }
                    else {

                        // 卸载 Media Center 和 Media Player
                        std::cout << "卸载 Windows Media Center...\n";
                        //ExecuteCommand("DISM /online /disable-feature /featurename:WindowsMediaCenter /NoRestart", true, false);
                        std::cout << "卸载 Windows Media Player...\n";
                        system("DISM /online /disable-feature /featurename:WindowsMediaPlayer /norestart");


                        std::cout << "第一阶段已完成。请尽快保存手头的工作，重新启动计算机，然后再次运行该程序。\n";
                        std::cout << "现在就重新启动计算机吗？(y/n): ";
                        std::cin >> ch;
                        //std::cin.ignore();
                    }
                    // 创建标记
                    LONG result = RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,          // 根键
                        L"SOFTWARE\\wmpConfig",         // 子键路径
                        0,                          // 保留
                        NULL,                       // 类名（可为NULL）
                        REG_OPTION_NON_VOLATILE,    // 选项（永久保存）
                        KEY_WRITE,                  // 访问权限
                        NULL,                       // 安全属性
                        &hKey,                      // 返回的句柄
                        NULL                        // 是否新创建的标志（可为NULL）
                    );
                    DWORD dwordValue = 1;
                    result = RegSetValueEx(
                        hKey,
                        L"InstalledState",
                        0,
                        REG_DWORD,
                        (const BYTE*)&dwordValue,
                        sizeof(DWORD)
                    );
                    if (ch == 'y' || ch == 'Y') {
                        //ExecuteCommand("shutdown -r -t 0", false, false);
                        BOOL res = EnableShutdownPrivilege(TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY);
                        if (res) {
                            ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_HOTFIX_UNINSTALL);
                        }
                        else {
                            std::cout << "您可能没有关闭计算机的特权。错误代码：";
                            std::cout << GetLastError() << std::endl;
                        }
                    }
                }
            }
        }
        else {
            std::cout << "错误：无法读取系统版本。" << std::endl;
        }
    }
    else if (stage == 2) {
        // 第二阶段
        std::cout << "第二阶段：复制文件并注册组件\n";

        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) {
            typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
            RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
            if (RtlGetVersion) {
                RTL_OSVERSIONINFOW osvi = { sizeof(osvi) };
                LONG result = RtlGetVersion(&osvi);  // ← 关键：调用函数填充数据
                if (result == 0) {  // 0 表示成功
                    if (osvi.dwMajorVersion >= 10 ||  // Windows 10/11
                        (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)) {  // Windows Vista

                        std::cout << "注意：Windows Vista/10/11系统可能无法写入 Help 文件夹（程序将提示“拒绝访问”）。如果失败，您需要手动复制 *.hlp 文件到 C:\\Windows\\Help 文件夹。\n";
                        system("pause");
                        std::cout << "正在尝试更改写入权限...\n";
                        system("takeown /f \"C:\\Windows\\Help\\*\" /r /d y && icacls \"C:\\Windows\\Help\\*\" /grant administrators:F /t");
                    }
                    std::string prefix = "copy \"" + g_scriptDir;
                    std::string tmp;
                    tmp = prefix + "\\" + "MPLAYER.CNT\"";
                    tmp += " C:\\Windows\\Help";
                    system(tmp.c_str());
                    tmp = prefix + "\\" + "MPLAYER.HLP\"";
                    tmp += " C:\\Windows\\Help";
                    system(tmp.c_str());
                    tmp = prefix + "\\" + "MPLAYER2.CNT\"";
                    tmp += " C:\\Windows\\Help";
                    system(tmp.c_str());
                    tmp = prefix + "\\" + "mplayer2.hlp\"";
                    tmp += " C:\\Windows\\Help";
                    system(tmp.c_str());
                    tmp = prefix + "\\" + "windows.hlp\"";
                    tmp += " C:\\Windows\\Help";
                    system(tmp.c_str());
                    tmp = prefix + "\\" + "wmplayer.chm\"";
                    tmp += " C:\\Windows\\Help";
                    system(tmp.c_str());
                    system("pause");
                    goto EndCopyExtendedResources;
                }
            }
        }

        {
            std::string dstFile = "C:\\Windows\\Help";
            // 复制帮助文件等额外的文件
            if (FileExists(g_scriptDir + "\\" + "MPLAYER.HLP")) {
                std::cout << "正在复制 MPLAYER.HLP...\n";
                std::string source = g_scriptDir + "\\" + "MPLAYER.HLP";
                if (!CopyFileA(source.c_str(), dstFile.c_str(), FALSE)) {
                    std::cerr << "复制 MPLAYER.HLP 失败: " << GetLastErrorStr() << std::endl;
                }
            }

            if (FileExists(g_scriptDir + "\\" + "mplayer2.hlp")) {
                std::cout << "正在复制 mplayer2.hlp...\n";
                std::string source = g_scriptDir + "\\" + "mplayer2.hlp";
                if (!CopyFileA(source.c_str(), dstFile.c_str(), FALSE)) {
                    std::cerr << "复制 mplayer2.hlp 失败: " << GetLastErrorStr() << std::endl;
                }
            }

            if (FileExists(g_scriptDir + "\\" + "windows.hlp")) {
                std::cout << "正在复制 windows.hlp...\n";
                std::string source = g_scriptDir + "\\" + "windows.hlp";
                if (!CopyFileA(source.c_str(), dstFile.c_str(), FALSE)) {
                    std::cerr << "复制 windows.hlp 失败: " << GetLastErrorStr() << std::endl;
                }
            }

            if (FileExists(g_scriptDir + "\\" + "MPLAYER.CNT")) {
                std::cout << "正在复制 MPLAYER.CNT...\n";
                std::string source = g_scriptDir + "\\" + "MPLAYER.CNT";
                if (!CopyFileA(source.c_str(), dstFile.c_str(), FALSE)) {
                    std::cerr << "复制 MPLAYER.CNT 失败: " << GetLastErrorStr() << std::endl;
                }
            }

            if (FileExists(g_scriptDir + "\\" + "MPLAYER2.CNT")) {
                std::cout << "正在复制 MPLAYER2.CNT...\n";
                std::string source = g_scriptDir + "\\" + "MPLAYER2.CNT";
                if (!CopyFileA(source.c_str(), dstFile.c_str(), FALSE)) {
                    std::cerr << "复制 MPLAYER2.CNT 失败: " << GetLastErrorStr() << std::endl;
                }
            }
        }
    EndCopyExtendedResources:

        // 删除残留目录
        for (std::string& targetDir : TARGET_DIR) {
            if (DirExists(targetDir)) {
                std::cout << "正在删除残留的文件...\n";

                // 使用动态分配的缓冲区
                std::string targetPath = targetDir;  // 例如 "C:\\Program Files\\Windows Media Player"
                targetPath.push_back('\0');           // 添加第一个null终止符
                targetPath.push_back('\0');           // 添加第二个null终止符（双null终止）

                SHFILEOPSTRUCTA fos = { 0 };
                fos.hwnd = NULL;
                fos.wFunc = FO_DELETE;
                fos.pFrom = targetPath.c_str();
                fos.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_ALLOWUNDO;
                // 注意：FOF_ALLOWUNDO 会移动到回收站，如果需要直接删除，去掉此项

                int result = SHFileOperationA(&fos);
                if (result != 0) {
                    std::cout << "删除失败，错误码: " << result << std::endl;
                    if (fos.fAnyOperationsAborted) {
                        std::cout << "操作被用户或系统中断" << std::endl;
                    }
                }
            }
        }

        // 显示剩余空间
        ULONGLONG freeBytes = GetFreeSpaceEx("C:\\");
        std::cout << "当前剩余空间为 " << freeBytes << " 字节。建议预留 50~100MB 空间。\n";
        system("pause");

        // 复制 wmp9xp 目录
        std::cout << "复制新文件 (wmp9xp)...\n";
        if (!CopyDirectory(g_scriptDir + "\\" + WMP9XP_DIR, TARGET_DIR.back())) {
            std::cerr << "复制 wmp9xp 失败: " << GetLastErrorStr() << std::endl;
            system("pause");
            return;
        }

        // 禁用程序兼容性助手
        std::cout << "禁用程序兼容性助手...\n";
        HKEY hKeyPCA;
        RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hKeyPCA, nullptr);
        DWORD val = 1;
        RegSetValueExA(hKeyPCA, "DisablePCA", 0, REG_DWORD, (BYTE*)&val, sizeof(val));
        RegCloseKey(hKeyPCA);
        // 64位视图（如果存在）
        RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wow6432Node\\Policies\\Microsoft\\Windows\\AppCompat", 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &hKeyPCA, nullptr);
        RegSetValueExA(hKeyPCA, "DisablePCA", 0, REG_DWORD, (BYTE*)&val, sizeof(val));
        RegCloseKey(hKeyPCA);

        // 复制 wmp 目录
        std::cout << "复制新文件 (wmp)...\n";
        if (!CopyDirectory(g_scriptDir + "\\" + WMP_DIR, TARGET_DIR.back())) {
            std::cerr << "复制 wmp 失败: " << GetLastErrorStr() << std::endl;
            system("pause");
            return;
        }

        // 注册 DLL 和 AX
        std::cout << "注册文件...\n";
        RegisterFiles(TARGET_DIR.back());

        // 跳过安装向导
        HKEY hkcu;
        RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\MediaPlayer\\Setup", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hkcu, nullptr);
        RegSetValueExA(hkcu, "SetupCompleted", 0, REG_DWORD, (BYTE*)&val, sizeof(val));
        RegCloseKey(hkcu);
        // 可选的 WOW6432Node
        RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\MediaPlayer\\WOW6432Node\\Setup", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hkcu, nullptr);
        RegSetValueExA(hkcu, "SetupCompleted", 0, REG_DWORD, (BYTE*)&val, sizeof(val));
        RegCloseKey(hkcu);

        // 注册 msdxm.ocx
        std::string msdxm = TARGET_DIR.back() + "\\msdxm.ocx";
        if (FileExists(msdxm)) {
            std::cout << "正在注册 msdxm.ocx...\n";
            std::string cmd = "regsvr32 /s \"" + msdxm + "\"";
            ExecuteCommand(cmd, true, false);
        }


        HKEY hKeyB;
        DWORD dwDisposition;

        // 创建或打开注册表键
        LONG result = RegCreateKeyExA(
            HKEY_LOCAL_MACHINE,     // 根键
            "SOFTWARE\\wmpConfig",                   // 子键路径
            0,                      // 保留，必须为0
            NULL,                   // 类名，通常为NULL
            REG_OPTION_NON_VOLATILE, // 持久化存储
            KEY_WRITE,              // 写入权限
            NULL,                   // 安全属性
            &hKeyB,                  // 返回的句柄
            &dwDisposition          // 返回是创建还是打开
        );

        if (result != ERROR_SUCCESS) {
            std::cerr << "打开/创建注册表键失败，错误码: " << result << std::endl;
        }

        DWORD data = 2;
        // 写入DWORD值
        result = RegSetValueExA(
            hKeyB,                   // 注册表键句柄
            "InstalledState",              // 值名称
            0,                      // 保留，必须为0
            REG_DWORD,              // 值类型
            (const BYTE*)&data,     // 数据指针
            sizeof(DWORD)           // 数据大小
        );

        // 关闭句柄
        RegCloseKey(hKeyB);

        std::cout << "正在删除原有配置...\n";
        HKEY hKeyRemove = HKEY_LOCAL_MACHINE;
        const wchar_t* lpStr = L"SOFTWARE\\Wow6432Node\\Microsoft\\MediaPlayer\\Setup\\Installed Versions";
        BOOL resultRemove = 0;
        resultRemove = DeleteKeyRecursively(hKeyRemove, lpStr);

        // 多用户注册表设置
        MultiUsersSetup();  // 此函数会询问快捷方式等，并写入注册表
    }
    RegCloseKey(hKey);
}

void clrscr() {    //清空屏幕
    HANDLE hdout = GetStdHandle(STD_OUTPUT_HANDLE);    //获取标准输出设备的句柄
    CONSOLE_SCREEN_BUFFER_INFO csbi;    //定义表示屏幕缓冲区属性的变量
    GetConsoleScreenBufferInfo(hdout, &csbi);  //获取标准输出设备的屏幕缓冲区属性
    DWORD size = csbi.dwSize.X * csbi.dwSize.Y, num = 0; //定义双字节变量
    COORD pos = { 0, 0 };    //表示坐标的变量（初始化为左上角(0, 0)点）
    //把窗口缓冲区全部填充为空格并填充为默认颜色（清屏）
    FillConsoleOutputCharacter(hdout, ' ', size, pos, &num);
    FillConsoleOutputAttribute(hdout, csbi.wAttributes, size, pos, &num);
    SetConsoleCursorPosition(hdout, pos);    //光标定位到窗口左上角
}

BOOL SetOwner(LPCWSTR lpszPath)
{

    std::string sid = GetCurrentUserSid();
    if (sid.empty()) return FALSE;

    PSID pOwnerSid = NULL;
    if (!ConvertStringSidToSidA(sid.c_str(), &pOwnerSid))
    {
        // 转换失败，处理错误
        return FALSE;
    }

    // 注意：对于目录，可能需要设置 SE_FILE_OBJECT，但 SetNamedSecurityInfo 会自动区分
    DWORD dwResult = SetNamedSecurityInfo(
        (LPWSTR)lpszPath,          // 对象名
        SE_FILE_OBJECT,            // 对象类型（文件或目录）
        OWNER_SECURITY_INFORMATION,// 设置所有者信息
        pOwnerSid,                 // 新所有者 SID
        NULL,                      // 组 SID（不变）
        NULL,                      // DACL（不变）
        NULL                       // SACL（不变）
    );
    return dwResult == ERROR_SUCCESS;
}

BOOL ProcessDirectory(LPCWSTR lpszRoot)
{
    std::string sid = GetCurrentUserSid();
    if (sid.empty()) return FALSE;

    PSID pOwnerSid = NULL;
    if (!ConvertStringSidToSidA(sid.c_str(), &pOwnerSid))
    {
        // 转换失败，处理错误
        return FALSE;
    }
    // 先处理当前目录自身
    if (!SetOwner(lpszRoot))
        return FALSE;

    // 递归子目录
    WCHAR szPattern[MAX_PATH];
    wcscpy_s(szPattern, lpszRoot);
    PathAppendW(szPattern, L"*");

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(szPattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return TRUE; // 空目录，不算失败

    do
    {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;

        WCHAR szFullPath[MAX_PATH];
        wcscpy_s(szFullPath, lpszRoot);
        PathAppendW(szFullPath, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // 递归子目录
            if (!ProcessDirectory(szFullPath))
            {
                FindClose(hFind);
                return FALSE;
            }
        }
        else
        {
            // 处理文件
            if (!SetOwner(szFullPath))
            {
                FindClose(hFind);
                return FALSE;
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return TRUE;
}

// 程序入口
int main() {
    // 设置控制台代码页为 UTF-8 或系统默认
    SetConsoleOutputCP(CP_ACP);
    SetConsoleCP(CP_ACP);

    // 获取脚本所在目录
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    g_scriptDir = path;
    size_t pos = g_scriptDir.find_last_of("\\");
    if (pos != std::string::npos)
        g_scriptDir = g_scriptDir.substr(0, pos);

#if defined (x86)
    if (!is64Bit) {
        std::cout << "警告：正在使用不符合系统架构的程序版本。dism 将无法正确使用，需要手动设置还原点与卸载程序。\n是否切换到正确的版本？(Y/n)\n";
        WriteLog("System architecture not capable.", "Warning");
        char choice = 'y';
        std::cin >> choice;
        if (tolower(choice) != 'n') {
            if (FileExists(g_scriptDir + "\\WMPConfig_x64.exe")) {
                STARTUPINFO si = { sizeof(si) };
                PROCESS_INFORMATION pi;

                std::string str = g_scriptDir + "\\WMPConfig_x64.exe";
                std::wstring ws = to_wstring(str).c_str();
                const wchar_t* wc = ws.c_str();

                // 创建新进程
                if (CreateProcess(
                    wc,  // 可执行文件路径
                    NULL,                           // 命令行参数
                    NULL,                           // 进程安全属性
                    NULL,                           // 线程安全属性
                    FALSE,                          // 继承句柄
                    CREATE_NEW_CONSOLE,               // 创建独立进程
                    NULL,                           // 环境变量
                    NULL,                           // 当前目录
                    &si,                            // 启动信息
                    &pi)) {                         // 进程信息

                    // 关闭进程和线程句柄（不等待进程结束）
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    WriteLog("Switched to x64 version.", "Info");
                    return 10;
                }

            }
            else {
                std::cerr << "错误：找不到 64 位程序。请重新下载应用程序，或手动创建还原点并卸载Windows Media Player，或退出本应用程序。您的系统尚未更改。\n";
                system("pause");
            }
        }
    }
#endif
    if (!is64Bit) TARGET_DIR.push_back("C:\\Program Files\\Windows Media Player");
    else TARGET_DIR.push_back("C:\\Program Files (x86)\\Windows Media Player");

    //system("whoami /all");
    //system("pause");



    while (true) {
        ShowMenu();
        int choice = -1;
        choice = GetChoice();
        switch (choice) {
        case 0:
            return 0;
        case 1:
            ExecuteDeployment();
            break;
        case 2:
            CheckDependencies();
            break;
        case 3:
            ShowLimits();
            break;
        case 4:
            if (!IsAdmin()) {
                std::cout << "多用户部署需要管理员权限，正在请求提升...\n";
                WriteLog("Required for administrator previlege", "Info");
                RunAsAdmin();
            }
            else {
                MultiUsersSetup();
            }
            break;
        default:
            std::cout << "未知选项。\n";

        }
        system("pause");
        clrscr();
    }
    
    return 0;
}
