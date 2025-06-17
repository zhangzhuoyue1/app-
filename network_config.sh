#!/bin/bash

# 安卓模拟器流量定向转发配置脚本
# 使用方法：./script.sh -u UBUNTU_IP -p PORT -a APP_UID -d ANDROID_IP -s SUBINTERFACE_IP [-h]

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # 无颜色

# 默认值
DEFAULT_UBUNTU_IP="192.168.31.172"
DEFAULT_PORT="8080"
DEFAULT_ANDROID_IP="192.168.31.200"

# 帮助信息
function show_help {
    echo "使用方法: $0 [选项]"
    echo "配置安卓模拟器将指定APP流量通过Ubuntu转发"
    echo ""
    echo "必需参数:"
    echo "  -u, --ubuntu-ip       Ubuntu虚拟机IP地址"
    echo "  -p, --port            转发端口"
    echo "  -a, --app-uid         目标APP的UID"
    echo ""
    echo "可选参数:"
    echo "  -d, --android-ip      安卓模拟器IP地址 [默认: $DEFAULT_ANDROID_IP]"
    echo "  -s, --subinterface-ip 子接口IP地址 [默认: 安卓IP+1]"
    echo "  -h, --help            显示此帮助信息"
    echo "  --cleanup             清理所有配置"
    exit 1
}

# 检查是否为清理操作
if [ "$1" = "--cleanup" ]; then
    # 导出默认值作为环境变量
    export UBUNTU_IP=$DEFAULT_UBUNTU_IP
    export ANDROID_IP=$DEFAULT_ANDROID_IP
    export SUBINTERFACE_IP=$(echo $ANDROID_IP | awk -F. '{print $1"."$2"."$3"."($4+1)}')
    
    # 清理环境
    echo -e "${YELLOW}清理安卓模拟器配置...${NC}"
    adb shell "iptables -F" 2>/dev/null
    adb shell "iptables -t nat -F" 2>/dev/null
    adb shell "iptables -t mangle -F" 2>/dev/null
    adb shell "ip rule del from $SUBINTERFACE_IP table 100" 2>/dev/null
    adb shell "ip rule del fwmark 100 table 100" 2>/dev/null
    adb shell "ip link set wlan0:1 down" 2>/dev/null
    adb shell "ip link delete wlan0:1" 2>/dev/null
    
    echo -e "${YELLOW}清理Ubuntu配置...${NC}"
    iptables -F 2>/dev/null
    iptables -t nat -F 2>/dev/null
    sysctl -w net.ipv4.ip_forward=0 2>/dev/null
    
    echo -e "${GREEN}环境清理完成${NC}"
    exit 0
fi

# 解析命令行参数
CLEANUP=0
while [[ $# -gt 0 ]]; do
    case $1 in
        -u|--ubuntu-ip)
            UBUNTU_IP="$2"
            shift 2
            ;;
        -p|--port)
            FORWARD_PORT="$2"
            shift 2
            ;;
        -a|--app-uid)
            APP_UID="$2"
            shift 2
            ;;
        -d|--android-ip)
            ANDROID_IP="$2"
            shift 2
            ;;
        -s|--subinterface-ip)
            SUBINTERFACE_IP="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            ;;
        *)
            echo "未知参数: $1"
            show_help
            ;;
    esac
done

# 检查必需参数
if [ -z "$UBUNTU_IP" ] || [ -z "$FORWARD_PORT" ] || [ -z "$APP_UID" ]; then
    echo -e "${RED}错误: 必需参数缺失${NC}"
    show_help
fi

# 设置默认值
ANDROID_IP=${ANDROID_IP:-$DEFAULT_ANDROID_IP}
SUBINTERFACE_IP=${SUBINTERFACE_IP:-$(echo $ANDROID_IP | awk -F. '{print $1"."$2"."$3"."($4+1)}')}

# 检查root权限
if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}错误: 请使用root权限运行此脚本${NC}"
    exit 1
fi

# 检查必备工具
check_tools() {
    echo -e "${YELLOW}检查必备工具...${NC}"
    if ! command -v adb &> /dev/null; then
        echo -e "${RED}错误: adb未安装，请先安装Android SDK Platform Tools${NC}"
        exit 1
    fi
    
    if ! command -v iptables &> /dev/null; then
        echo -e "${YELLOW}安装iptables...${NC}"
        apt-get install -y iptables
    fi
    
    echo -e "${GREEN}工具检查完成${NC}"
}

# 连接安卓模拟器
connect_emulator() {
    echo -e "${YELLOW}连接安卓模拟器...${NC}"
    
    # 检查ADB是否已连接设备
    DEVICE_COUNT=$(adb devices | grep -v "List of devices" | grep -v "^$" | wc -l)
    
    if [ "$DEVICE_COUNT" -eq 0 ]; then
        echo -e "${RED}错误: 未检测到已连接的安卓设备${NC}"
        exit 1
    fi
    
    # 获取已连接设备列表
    DEVICES=$(adb devices | grep -v "List of devices" | grep -v "^$" | awk '{print $1}')
    DEVICE_COUNT=$(echo "$DEVICES" | wc -l)
    
    if [ "$DEVICE_COUNT" -gt 1 ]; then
        echo -e "${YELLOW}检测到多个设备，请选择要使用的设备：${NC}"
        select DEVICE in $DEVICES; do
            if [ -n "$DEVICE" ]; then
                adb -s $DEVICE root
                adb -s $DEVICE connect $ANDROID_IP:5555
                break
            else
                echo -e "${RED}无效选择${NC}"
            fi
        done
    else
        adb root
        adb connect $ANDROID_IP:5555
    fi
    
    # 验证连接状态
    ADB_STATUS=$(adb get-state 2>/dev/null)
    
    if [ "$ADB_STATUS" != "device" ]; then
        echo -e "${RED}错误: 无法连接到安卓模拟器，请检查网络连接${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}已成功连接到安卓模拟器${NC}"
}

# 配置Ubuntu网络转发
configure_ubuntu() {
    echo -e "${YELLOW}配置Ubuntu网络转发...${NC}"
    
    # 获取网络接口
    IN_INTERFACE=$(ip route | grep default | awk '{print $5}')
    
    if [ -z "$IN_INTERFACE" ]; then
        echo -e "${RED}错误: 无法获取网络接口信息${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}使用接口: $IN_INTERFACE${NC}"
    
    # 启用IP转发
    sysctl -w net.ipv4.ip_forward=1
    echo "net.ipv4.ip_forward=1" >> /etc/sysctl.conf
    
    # 清除旧规则
    iptables -t nat -F
    
    # 设置NAT规则（优化版）
    iptables -t nat -A PREROUTING -i $IN_INTERFACE -s 192.168.31.0/24 -j ACCEPT
    iptables -t nat -A POSTROUTING -o $IN_INTERFACE -j MASQUERADE
    
    # 设置转发规则
    iptables -F FORWARD
    iptables -A FORWARD -i $IN_INTERFACE -o $IN_INTERFACE -j ACCEPT
    iptables -A FORWARD -i $IN_INTERFACE -o $IN_INTERFACE -m state --state RELATED,ESTABLISHED -j ACCEPT
    
    # 保存规则
    if command -v netfilter-persistent &> /dev/null; then
        netfilter-persistent save
    fi
    
    echo -e "${GREEN}Ubuntu网络转发配置完成${NC}"
}

# 配置安卓模拟器
configure_emulator() {
    echo -e "${YELLOW}配置安卓模拟器...${NC}"
    
    # 创建子接口
    adb shell "ifconfig wlan0:1 $SUBINTERFACE_IP netmask 255.255.255.0 up"
    
    # 检查子接口是否创建成功
    SUBINTERFACE_STATUS=$(adb shell ifconfig wlan0:1 2>/dev/null)
    
    if [ -z "$SUBINTERFACE_STATUS" ]; then
        echo -e "${RED}错误: 无法创建子接口 wlan0:1${NC}"
        echo -e "${YELLOW}尝试使用替代方法创建子接口...${NC}"
        
        # 替代方法：使用ip命令创建子接口
        adb shell "ip link add link wlan0 name wlan0:1 type macvlan mode bridge"
        adb shell "ip addr add $SUBINTERFACE_IP/24 dev wlan0:1"
        adb shell "ip link set wlan0:1 up"
        
        # 再次检查
        SUBINTERFACE_STATUS=$(adb shell ifconfig wlan0:1 2>/dev/null)
        
        if [ -z "$SUBINTERFACE_STATUS" ]; then
            echo -e "${RED}错误: 无法创建子接口，可能需要root权限或模拟器不支持${NC}"
            exit 1
        fi
    fi
    
    echo -e "${GREEN}子接口 wlan0:1 创建成功${NC}"
    
    # 检查rt_tables文件是否存在
    RT_TABLES_EXISTS=$(adb shell "ls /etc/iproute2/rt_tables 2>/dev/null" | wc -l)
    
    if [ "$RT_TABLES_EXISTS" -eq 0 ]; then
        echo -e "${YELLOW}/etc/iproute2/rt_tables不存在，创建临时路由表...${NC}"
        
        # 使用替代方法配置策略路由（不依赖rt_tables文件）
        #adb shell "ip rule del from $SUBINTERFACE_IP lookup 100 2>/dev/null"
        #adb shell "ip rule add from $SUBINTERFACE_IP lookup 100"
        #adb shell "ip rule del fwmark 100 lookup 100 2>/dev/null"
        #adb shell "ip rule add fwmark 100 lookup 100"
        #adb shell "ip route add default via $UBUNTU_IP dev wlan0:1 table 100"
        # 配置策略路由（兼容无rt_tables的情况）
        adb shell "ip rule del from $SUBINTERFACE_IP lookup 100 2>/dev/null"
        adb shell "ip rule add from $SUBINTERFACE_IP lookup 100"
        adb shell "ip rule del fwmark 100 lookup 100 2>/dev/null"
        adb shell "ip rule add fwmark 100 lookup 100"

        # 添加默认路由到表100
        adb shell "ip route add default via $UBUNTU_IP dev wlan0:1 table 100"

        # 为所有流量添加主路由（确保基础网络连通）
        adb shell "ip route add default via 192.168.31.1 dev wlan0 table main"
    else
        # 标准方法：修改rt_tables文件
        adb shell "echo '100     app_route' >> /etc/iproute2/rt_tables"
        
        # 配置策略路由
        adb shell "ip rule del from $SUBINTERFACE_IP table 100 2>/dev/null"
        adb shell "ip rule add from $SUBINTERFACE_IP table 100"
        adb shell "ip rule del fwmark 100 table 100 2>/dev/null"
        adb shell "ip rule add fwmark 100 table 100"
        adb shell "ip route add default via $UBUNTU_IP dev wlan0:1 table 100"
    fi
    
    # 为指定APP的流量打标记
    adb shell "iptables -t mangle -A OUTPUT -m owner --uid-owner $APP_UID -j MARK --set-mark 100"
    
    # 配置DNAT规则，将HTTP/HTTPS流量转发到Ubuntu的指定端口
    adb shell "iptables -t nat -A OUTPUT -m mark --mark 100 -p tcp --dport 80 -j DNAT --to-destination $UBUNTU_IP:$FORWARD_PORT"
    adb shell "iptables -t nat -A OUTPUT -m mark --mark 100 -p tcp --dport 443 -j DNAT --to-destination $UBUNTU_IP:$FORWARD_PORT"
    
    # 允许标记的流量通过子接口
    adb shell "iptables -A OUTPUT -m mark --mark 100 -o wlan0:1 -j ACCEPT"
    
    # 配置SNAT规则
    adb shell "iptables -t nat -A POSTROUTING -o wlan0:1 -j MASQUERADE"
    
    echo -e "${GREEN}安卓模拟器配置完成${NC}"
}

# 显示配置结果
show_config() {
    echo -e "${GREEN}==============================================${NC}"
    echo -e "${GREEN}          配置已完成                          ${NC}"
    echo -e "${GREEN}==============================================${NC}"
    echo -e "${GREEN}配置参数:${NC}"
    echo -e "  Ubuntu IP: ${YELLOW}$UBUNTU_IP${NC}"
    echo -e "  转发端口: ${YELLOW}$FORWARD_PORT${NC}"
    echo -e "  APP UID: ${YELLOW}$APP_UID${NC}"
    echo -e "  安卓模拟器IP: ${YELLOW}$ANDROID_IP${NC}"
    echo -e "  子接口IP: ${YELLOW}$SUBINTERFACE_IP${NC}"
    echo -e ""
    echo -e "${GREEN}现在可以在Ubuntu上使用以下命令捕获流量:${NC}"
    echo -e "${YELLOW}sudo tcpdump -i $IN_INTERFACE src $SUBINTERFACE_IP -s 0 -w traffic.pcap${NC}"
    echo -e "${YELLOW}或捕获所有TCP流量: sudo tcpdump -i $IN_INTERFACE 'tcp' -s 0 -w all_tcp.pcap${NC}"
    echo -e ""
    echo -e "${GREEN}如需清理配置，请运行:${NC}"
    echo -e "${YELLOW}./$(basename "$0") --cleanup${NC}"
}

# 主函数
main() {
    check_tools
    connect_emulator
    configure_ubuntu
    configure_emulator
    show_config
}

# 执行主函数
main