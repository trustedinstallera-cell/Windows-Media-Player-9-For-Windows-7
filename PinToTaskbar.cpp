#include <windows.h>
#include <shellapi.h>
#include <string>
#include <sys/stat.h>
#include <direct.h>

#pragma comment(lib, "shell32.lib")

enum Error :int {
    STATUS_OK,
    FILE_NOTFOUND = 1,
    IS_DIRECTORY
};

// 修改为 ANSI 版本（使用 const char*）
void PinToTaskbar(const std::string& lnkPath)
{
    // 使用 ANSI 版本的 ShellExecuteA
    ShellExecuteA(
        NULL,                    // 父窗口句柄
        "taskbarpin",            // 动词：固定到任务栏（ANSI 字符串）
        lnkPath.c_str(),         // 快捷方式路径（ANSI 字符串）
        NULL,                    // 参数
        NULL,                    // 工作目录
        SW_SHOW                   // 显示方式
    );
}

void UnpinFromTaskbar(const std::string& lnkPath)
{
    // 使用 ANSI 版本的 ShellExecuteA
    ShellExecuteA(
        NULL,
        "taskbarunpin",          // 动词：从任务栏解锁（ANSI 字符串）
        lnkPath.c_str(),
        NULL,
        NULL,
        SW_SHOW
    );
}

Error isRegularFile(const std::string& filename) {
    struct stat fileInfo;

    if (stat(filename.c_str(), &fileInfo) != 0) {
        return Error::FILE_NOTFOUND;
    }

    if (!(fileInfo.st_mode & S_IFDIR)) return Error::STATUS_OK;
    return Error::IS_DIRECTORY;
}

int main() {
    std::string filename = "Windows Media Player.lnk";
    int val = static_cast<int>(isRegularFile(filename));
    if (val != 0) {
        return val;
    }

    // 获取完整路径（ANSI 版本）
    char fullPath[MAX_PATH];
    _fullpath(fullPath, filename.c_str(), MAX_PATH);

    // 直接使用 ANSI 字符串调用 PinToTaskbar
    PinToTaskbar(std::string(fullPath));

    // 如果需要解锁，调用：
    // UnpinFromTaskbar(std::string(fullPath));

    return 0;
}