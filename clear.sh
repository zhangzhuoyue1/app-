#!/bin/bash

# 参数检查
if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <APP_UID> <PROXY_HOST> <PORT>"
    exit 1
fi

APP_UID=$1
PROXY_HOST=$2
PROXY_PORT=$3

echo "[+] Clearing iptables redirect on Android emulator for UID=$APP_UID to $PROXY_HOST:$PROXY_PORT"

# 设置 adb 前缀（可添加设备号 -s emulator-5554）
ADB="adb shell"

# 删除之前添加的 NAT 表规则
for port in 80 443; do
  $ADB "su -c 'iptables -t nat -D OUTPUT -p tcp --dport $port -m owner --uid-owner $APP_UID -j DNAT --to-destination $PROXY_HOST:$PROXY_PORT'"
done

# 关闭 IP 转发（可选）
$ADB "su -c 'echo 0 > /proc/sys/net/ipv4/ip_forward'"

echo "[+] Redirect rules removed via adb successfully."
