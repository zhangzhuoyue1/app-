#!/bin/bash

# 参数检查
if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <APP_UID> <PROXY_HOST> <PORT>"
    exit 1
fi

APP_UID=$1
PROXY_HOST=$2
PROXY_PORT=$3

echo "[+] Setting up iptables redirect on Android emulator for UID=$APP_UID to $PROXY_HOST:$PROXY_PORT"

# 设置 adb 前缀（你可以加 -s emulator-5554 来指定模拟器）
ADB="adb shell"

# 开启 IP 转发（某些 Android 版本可能不需要）
$ADB "su -c 'echo 1 > /proc/sys/net/ipv4/ip_forward'"

# 添加规则：将 UID 对应流量重定向到 mitmproxy 服务器
for port in 80 443; do
  $ADB "su -c 'iptables -t nat -A OUTPUT -p tcp --dport $port -m owner --uid-owner $APP_UID -j DNAT --to-destination $PROXY_HOST:$PROXY_PORT'"
done

echo "[+] Redirect rules applied via adb successfully."
