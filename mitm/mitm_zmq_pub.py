from mitmproxy import http, ctx
import zmq, base64, threading, queue
import datetime
#import logging

# 配置日志，真实项目中建议只配置一次，且放在入口文件中
#logging.basicConfig(level=logging.DEBUG, format='%(asctime)s [%(levelname)s] %(message)s')
class ZMQForwarder:
    def __init__(self):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.PUB)
        self.socket.bind("tcp://*:5555")
        self.queue = queue.Queue(maxsize=1000)
        self.worker = threading.Thread(target=self._send_worker)
        self.worker.daemon = True
        self.worker.start()
        self.request_cache = {}
        ctx.log.info(f"ZMQ Socket bound to: {self.socket.getsockopt(zmq.LAST_ENDPOINT)}")

    def _get_safe_content(self, content: bytes) -> str:
        try:
            return content.decode('utf-8')
        except UnicodeDecodeError:
            return base64.b64encode(content).decode('utf-8')

    def _get_headers(self, headers) -> str:
        return "\n".join([f"{k}: {v}" for k, v in headers.items()])

    def _send_worker(self):
        while True:
            data = self.queue.get()
            try:
                self.socket.send_json(data)
            except zmq.ZMQError as e:
                ctx.log.error(f"ZMQ send failed: {e}, rebuilding socket")
                self._rebuild_socket()
            except Exception as e:
                ctx.log.error(f"Unknown ZMQ error: {e}")

    def _rebuild_socket(self):
        self.socket.close()
        self.socket = self.context.socket(zmq.PUB)
        self.socket.setsockopt(zmq.LINGER, 0)
        self.socket.bind("tcp://*:5555")

    def _common_fields(self, flow: http.HTTPFlow) -> dict:
        ts = datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%fZ")
        client_ip, client_port = flow.client_conn.address

        # 尝试从 server_conn 获取目标地址
        server_ip = ""
        server_port = 0
        if flow.server_conn.ip_address:
            if isinstance(flow.server_conn.ip_address, tuple):
                server_ip = flow.server_conn.ip_address[0]
                server_port = flow.server_conn.ip_address[1]
            else:
                server_ip = str(flow.server_conn.ip_address)
                server_port = flow.server_conn.port
        else:
            # 回退方案：使用请求头中的 Host 或 request.host
            server_ip = flow.request.host or "unknown"
            server_port = flow.request.port or (443 if flow.request.scheme == "https" else 80)

               # 优先使用flow.request.host
        host =""
        if flow.request.host:
            host = flow.request.host
        # 尝试从headers中获取Host
        if "Host" in flow.request.headers:
            host = flow.request.headers["Host"] 
        # 尝试从HTTP/2的:authority伪头字段获取
        if ":authority" in flow.request.headers:
            host = flow.request.headers[":authority"]
            

        return {
            "timestamp": ts,
            "src_ip": client_ip,
            "src_port": client_port,
            "dst_ip": server_ip,
            "dst_port": server_port,
            "protocol": "TCP",
            "top_protocol": flow.request.scheme.upper(),
            "http_version": flow.request.http_version,
            "flow_id": flow.id,
            "host": host,
        }

    def request(self, flow: http.HTTPFlow):
        req_data = {
            "method": flow.request.method,
            "path": flow.request.path,
            "url": flow.request.url,
            "headers": self._get_headers(flow.request.headers),
            "body": self._get_safe_content(flow.request.content),
            "length": len(flow.request.raw_content),
            "content_type": flow.request.headers.get("Content-Type", ""),
            "direction": "request",
            "timestamp": datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%fZ"),
            "top_protocol": flow.request.scheme.upper(),
        }
        self.request_cache[flow.id] = req_data

        # 同步推送 request 报文内容
        base = self._common_fields(flow)
        data = {**base, **req_data}
        
        # 添加info字段
        data["info"] = f"{flow.request.method} {flow.request.url}"
        
        self.queue.put(data)

    def response(self, flow: http.HTTPFlow):
        if not flow.response:
            return

        base = self._common_fields(flow)
        req = self.request_cache.pop(flow.id, {})
        client_ip, client_port = flow.client_conn.address

        # 确保summary包含direction字段
        summary = {
            **base,
            **{k: req.get(k, "") for k in ["method", "path", "url"]},
            "status": flow.response.status_code,
            "content_type": flow.response.headers.get("Content-Type", ""),
            "direction": "response"
        }

        # 构造 http_packets
        resp_packet = {
            "flow_id": flow.id,
            "timestamp": base["timestamp"],
            "direction": "response",
            "protocol": base["top_protocol"],
            "headers": self._get_headers(flow.response.headers),
            "body": self._get_safe_content(flow.response.content),
            "content_type": flow.response.headers.get("Content-Type", ""),
            "length": len(flow.response.content)
        }

        # 修改info字段，只显示状态码和状态描述
        status_info = f"{flow.response.status_code} {flow.response.reason}"
        
        # 添加可选的内容缺失提示
        if len(flow.response.content) == 0:
            status_info += " (content missing)"
            
        summary["info"] = f"<< {status_info}"
        resp_packet["info"] = f"<< {status_info}"

        # 推送两个结构
        self.queue.put({**summary, "type": "flow_info"})     # 摘要信息
        self.queue.put(resp_packet)                          # 响应详细报文

addons = [ZMQForwarder()]