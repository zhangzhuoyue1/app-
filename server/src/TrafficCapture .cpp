#include "TrafficCapture .h"

TrafficCapture::TrafficCapture(const std::string strAppId)
    :m_strAppId(strAppId)
{
    m_bIsCapture = false;
}

TrafficCapture::~TrafficCapture()
{
    StopCapture();
}