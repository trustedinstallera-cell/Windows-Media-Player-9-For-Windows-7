// getsid_currentdir.cpp
#include <windows.h>
#include <sddl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    // 获取输出目录（从命令行参数）
    std::string outputDir = ".";
    if (argc > 1) {
        outputDir = argv[1];
    }

    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        std::cerr << "无法打开进程令牌" << std::endl;
        return 1;
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    std::vector<BYTE> buffer(dwSize);
    PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.data());

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
        std::cerr << "获取令牌信息失败" << std::endl;
        CloseHandle(hToken);
        return 1;
    }

    LPSTR pSidString = NULL;
    if (!ConvertSidToStringSidA(pTokenUser->User.Sid, &pSidString)) {
        std::cerr << "转换SID失败" << std::endl;
        CloseHandle(hToken);
        return 1;
    }

    // 构建输出文件路径
    std::string outputFile = outputDir + "\\wmp_sid.txt";

    // 写入文件
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        std::cerr << "无法创建文件: " << outputFile << std::endl;
        LocalFree(pSidString);
        CloseHandle(hToken);
        return 1;
    }

    outFile << pSidString;
    outFile.close();

    std::cout << "SID已写入: " << outputFile << std::endl;
    std::cout << "SID: " << pSidString << std::endl;

    LocalFree(pSidString);
    CloseHandle(hToken);
    return 0;
}