#include <iostream>
#include <pcap.h>
#include <spdlog/spdlog.h>

int main()
{
    pcap_if_t *alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        spdlog::error("pcap_findalldevs failed: {}", errbuf);
        return -1;
    }

    for (pcap_if_t *d = alldevs; d != nullptr; d = d->next)
    {
        spdlog::info("Device: {}", d->name);
        if (d->description)
        {
            spdlog::info("Description: {}", d->description);
        }
        else
        {
            spdlog::info("No description available");
        }
    }

    pcap_freealldevs(alldevs);

    return 0;
}