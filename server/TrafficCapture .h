#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <sys/stat.h>

//功能 ： 捕获指定 app 的流量

class TrafficCapture
{
public:
    TrafficCapture(const std::string strAppName);
    ~TrafficCapture();

    void StartCapture();    //开始捕获
    void StopCapture();     //停止捕获

    bool GetAppID();    //获取指定appID     
    bool SetIptables(); //设置过滤规则   

private:
    const std::string  m_strAppName;       //app包名
    std::string m_strUid;                  //app Id
    bool m_bIsCapture;                     //是否开始捕获
};