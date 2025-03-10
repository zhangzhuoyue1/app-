#include <iostream>
#include "AppInfoFetcher.h"

int main()
{
    AppInfoFetCher appInfoFetcher;
    appInfoFetcher.fetchAllPackages();
    std::string str=appInfoFetcher.getUidByPackage("tv.danmaku.bili");
    //appInfoFetcher.getAllPackageNames();
    return 0;
}
