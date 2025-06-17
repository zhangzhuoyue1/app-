#!/bin/bash

# 用法：sudo ./network_cleanup.sh <Ubuntu主网卡名> <安卓接口名>
# 示例：sudo ./network_cleanup.sh ens33 wlan0

if [ $# -ne 2 ]; then
  echo "Usage: sudo $0 <Ubuntu网卡名> <Android接口名>"
  echo "Example: sudo $0 ens33 wlan0"
  exit 1
fi

UBUNTU_IFACE=$1
ANDROID_IFACE=$2
UBUNTU_SUB_IFACE="${UBUNTU_IFACE}:1"
UBUNTU_SUB_IP="10.0.0.1"
UBUNTU_SUBNET="10.0.0.0/24"

ANDROID_IP="10.0.0.2"
TABLE_ID=100
FWMARK=0x1

echo "========== [Ubuntu] 清理配置 =========="

# 删除 Ubuntu 子接口地址
sudo ip addr del ${UBUNTU_SUB_IP}/24 dev ${UBUNTU_IFACE} label ${UBUNTU_SUB_IFACE} 2>/dev/null

# 删除 NAT 与转发规则
sudo iptables -t nat -D POSTROUTING -s ${UBUNTU_SUBNET} -o ${UBUNTU_IFACE} -j MASQUERADE 2>/dev/null
sudo iptables -D FORWARD -s ${UBUNTU_SUBNET} -j ACCEPT 2>/dev/null
sudo iptables -D FORWARD -d ${UBUNTU_SUBNET} -j ACCEPT 2>/dev/null

# 删除策略路由（如果之前设置了）
sudo ip rule del fwmark 1 lookup 100 2>/dev/null
sudo ip route flush table 100 2>/dev/null

echo "========== [Android] 清理配置 =========="

adb root && adb wait-for-device

# 删除子接口 IP（忽略错误）
adb shell "ip addr del ${ANDROID_IP}/24 dev ${ANDROID_IFACE}" 2>/dev/null

# 删除路由表
adb shell "ip route flush table ${TABLE_ID}" 2>/dev/null

# 删除策略规则
adb shell "ip rule del fwmark ${FWMARK} lookup ${TABLE_ID}" 2>/dev/null

# 清除 iptables mangle 打标规则
adb shell "iptables -t mangle -D OUTPUT -m owner --uid-owner 10051 -j MARK --set-mark ${FWMARK}" 2>/dev/null

echo "========== ✅ 清理完成 =========="
