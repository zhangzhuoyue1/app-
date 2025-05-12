from mitmproxy import http, ctx
import redis, base64, json, threading, queue

class RedisForwarder:
    def __init__(self):
        # 连接 Redis，指定 IP、端口和密码
        self.redis = redis.Redis(host="192.168.31.96", port=6380, password="123456", decode_responses=True)
        self.queue = queue.Queue(maxsize=1000)

        # 启动后台线程异步发送数据
        self.worker = threading.Thread(target=self._send_worker)
        self.worker.daemon = True
        self.worker.start()
        ctx.log.info("[RedisForwarder] Redis PUB channel initialized.")

    def _get_safe_content(self, content):
        # 将字节内容转为可打印字符串；若为二进制数据则转 base64
        try:
            return content.decode('utf-8')
        except UnicodeDecodeError:
            return base64.b64encode(content).decode('utf-8')

    def _send_worker(self):
        # 后台线程循环处理队列中待发数据
        while True:
            data = self.queue.get()
            try:
                self.redis.rpush("mitm_channel", json.dumps(data))
                ctx.log.info(f"[RedisForwarder] 数据已推送至 Redis: {data['type']} {data['host']}{data.get('path', '')}")
            except Exception as e:
                ctx.log.error(f"[RedisForwarder] 发布失败: {e}")

    def request(self, flow: http.HTTPFlow):
        # 拦截 HTTP 请求并放入队列
        self.queue.put({
            "type": "request",
            "host": flow.request.host,
            "method": flow.request.method,
            "path": flow.request.path,
            "content": self._get_safe_content(flow.request.content),
        })

    def response(self, flow: http.HTTPFlow):
        # 拦截 HTTP 响应并放入队列
        self.queue.put({
            "type": "response",
            "host": flow.request.host,
            "status": flow.response.status_code,
            "content": self._get_safe_content(flow.response.content),
        })

# 注册插件到 mitmproxy
addons = [RedisForwarder()]
