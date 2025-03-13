#include <iostream>
#include "AppInfoFetcher/AppInfoFetcher.h"
#include "TrafficCapture/TrafficCapture.h"

void CallBack(u_char* user, const struct pcap_pkthdr* hdr, const u_char* data)
{
    spdlog::info("[TrafficCapture] Packet captured, length: {}", hdr->len);
}

int main()
{
    AppInfoFetCher appInfoFetcher;
    appInfoFetcher.fetchAllPackages();
    std::string str=appInfoFetcher.getUidByPackage("tv.danmaku.bili");
    TrafficCapture trafficCapture(str,4999);
    trafficCapture.StartCapture(CallBack);
    //appInfoFetcher.getAllPackageNames();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    trafficCapture.StopCapture();
    return 0;
}
