import zmq
import json
from datetime import datetime

def start_subscriber():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.connect("tcp://localhost:5555")  # 连接到 Publisher
    socket.setsockopt_string(zmq.SUBSCRIBE, '')  # 订阅所有消息

    print("等待接收 ZeroMQ 报文数据...")
    try:
        while True:
            message = socket.recv_string()
            data = json.loads(message)
            
            # 按请求/响应分类存储
            if data['type'] == 'request':
                print(f"[请求] {data['method']} {data['url']}")
            elif data['type'] == 'response':
                print(f"[响应] {data['status_code']} {data['url']}")

            # 持久化存储（示例：写入本地文件）
            with open("mitm_traffic.log", "a") as f:
                f.write(f"{datetime.now()}: {message}\n")

    except KeyboardInterrupt:
        print("停止订阅")
    finally:
        socket.close()
        context.term()

if __name__ == "__main__":
    start_subscriber()
