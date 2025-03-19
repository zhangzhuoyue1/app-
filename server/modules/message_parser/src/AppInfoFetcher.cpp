#include <iostream>
#include <cstdio>
#include <regex>
#include <spdlog/spdlog.h>
#include "../include/AppInfoFetcher.h"
AppInfoFetCher::AppInfoFetCher(const std::string &adbPath)
    :m_strAdbPath(adbPath)
{ 

}

void AppInfoFetCher::fetchAllPackages()
{
    //获取原始数据
    const std::string strRawOutput = ExecuteAdbCommand("shell pm list packages");

    //清除旧数据
    m_vecPackageNames.clear();

    //解析数据
    ParsePackageName(strRawOutput);
}

std::string AppInfoFetCher::getUidByPackage(const std::string &packageName)
{
    //获取uid命令
    //const std::string cmd = "shell dumpsys package " + packageName + " | egrep 'userId=|uid='";

    const std::string cmd = "shell dumpsys package " + packageName + " | egrep 'userId='";

    const std::string strRawOutput = ExecuteAdbCommand(cmd);

    std::string res;
    for(char c:strRawOutput)
    {
        if(isdigit(c))
        {
            res += c;
        }
    }
    spdlog::info("[AppInfoFetCher] find package: {} : uid:{}",packageName,res);
    return res;
}

//获取当前已缓存的包名列表
std::vector<std::string>AppInfoFetCher::getAllPackageNames() const
{
    if(m_vecPackageNames.empty())
    {
        return std::vector<std::string>();
    }
    return m_vecPackageNames;
}

//执行adb命令
std::string AppInfoFetCher::ExecuteAdbCommand(const std::string &cmdAdb)
{
    //adb命令
    const std::string cmd= m_strAdbPath + " " + cmdAdb;

    FILE* pipe(popen(cmd.c_str(), "r"));
    if(!pipe)
    {
        std::cerr << "popen() failed!" << std::endl;
        return "";
    }

    char buffer[128];
    std::string result;;
    while(fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        result += buffer;
    }
    return result;
}

//解析获取到的包名
void AppInfoFetCher::ParsePackageName(const std::string &rawOutput)
{
    std::istringstream stream(rawOutput);
    std::string line;
    //使用正则表达式匹配包名
    std::regex pattern(R"(package:([^\s]+))");
        
    while (std::getline(stream, line)) 
    {
        std::smatch matches;
        if (std::regex_match(line, matches, pattern)) 
        {
            const std::string& pkg = matches[1].str();
            
            // 检查包名是否包含android（不区分大小写）
            if (pkg.find("android") == std::string::npos && 
               pkg.find("ANDROID") == std::string::npos) 
            {
               spdlog::info("[AppInfoFetcher] exist app package:{}",pkg);
                m_vecPackageNames.push_back(pkg);
            }
        }   
    }
}