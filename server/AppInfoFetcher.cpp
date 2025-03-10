#include <iostream>
#include <cstdio>
#include <regex>
#include "AppInfoFetcher.h"


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
    return "";
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

//解析获取到的包名并获取uid
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
            if(matches.size()== 2)
            {
                m_vecPackageNames.push_back(matches[1]);
            }
        }   
    }
}