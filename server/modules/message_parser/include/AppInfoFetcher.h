#pragma once
#include <unordered_map>
#include <string>
#include <vector>
class AppInfoFetCher
{
public:
    AppInfoFetCher(const std::string &adbPath = "adb");

    void fetchAllPackages();                                     // 全量获取包名并构建映射
    std::string getUidByPackage(const std::string &packageName); // 通过包名获取uid
    std::vector<std::string> getAllPackageNames() const;         // 获取当前已缓存的包名列表

private:
    std::string m_strAdbPath;                   // adb路径
    std::vector<std::string> m_vecPackageNames; // 包名列表

    std::string ExecuteAdbCommand(const std::string &cmdAdb);     // 执行Adb命令
    void ParsePackageName(const std::string &rawOutput);          // 业务解析方法: 处理pm list输出
    std::string extractUidFromDumpsys(const std::string &output); // 业务解析方法: 处理dumpsys输出
};