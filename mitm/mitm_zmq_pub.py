from mitmproxy import http, ctx
import zmq, base64, threading, queue

#
class ZMQForwarder:
    def __init__(self):
        self.context = zmq.Context() # 创建一个新的ZMQ上下文
        self.socket = self.context.socket(zmq.PUB) # 创建一个新的ZMQ发布者Socket
        self.socket.bind("tcp://*:5555") # 绑定到TCP端口5555
        self.queue = queue.Queue(maxsize=1000) # 创建一个线程安全的队列
        self.worker = threading.Thread(target=self._send_worker) # 创建一个新的线程
        self.worker.daemon = True # 设置为守护线程
        self.worker.start()

    def _get_safe_content(self, content):
        try:
            return content.decode('utf-8')
        except UnicodeDecodeError:
            return base64.b64encode(content).decode('utf-8')

    def _send_worker(self):
        while True:
            data = self.queue.get()
            try:
                self.socket.send_json(data)
            except zmq.ZMQError as e:
                ctx.log.error(f"ZMQ发送失败: {e}, 尝试重建Socket...")
                self._rebuild_socket()
            except Exception as e:
                ctx.log.error(f"未知错误: {e}")

    def _rebuild_socket(self):
        self.socket.close()
        self.socket = self.context.socket(zmq.PUB)
        self.socket.setsockopt(zmq.LINGER, 0)
        self.socket.bind("tcp://*:5555")

    def request(self, flow: http.HTTPFlow):
        self.queue.put({
            "type": "request",
            "scheme": flow.request.scheme, 
            "host": flow.request.host,
            "method": flow.request.method,
            "path": flow.request.path,
            "content": self._get_safe_content(flow.request.content),
        })

    def response(self, flow: http.HTTPFlow):
        self.queue.put({
            "type": "response",
            "scheme": flow.request.scheme, 
            "host": flow.request.host,
            "status": flow.response.status_code,
            "content": self._get_safe_content(flow.response.content),
        })

addons = [ZMQForwarder()] 