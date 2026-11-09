"""Local HTTP adapter for extracted production control handlers, never hardware.

Only transport and telemetry formatting are doubled. Mode, command, session,
sequence, revocation and STOP-fence logic execute the current C++ bodies.
"""
import json
import subprocess
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlsplit


class BackendBridge:
    def __init__(self, fixture):
        self.fixture = fixture
        fixture.setUpClass()
        self.process = subprocess.Popen(
            [str(fixture.exe), 'bridge'], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        lock = threading.Lock()
        owner = self

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *_):
                pass  # Do not log request tokens.

            def do_POST(self):
                self.do_GET()

            def do_GET(self):
                route = urlsplit(self.path)
                query = parse_qs(route.query)
                value = lambda key, default='-': query.get(key, [default])[0]
                action = {'/control-session': 'session', '/cmd': 'cmd',
                          '/mode': 'mode', '/data': 'data', '/ping': 'ping',
                          '/expire': 'expire', '/revoke': 'revoke',
                          '/other': 'other', '/fixture': 'reset'}.get(route.path, 'state')
                parts = [value('now', '10000'), action, value('move'),
                         value('seq', '0'), value('token'),
                         'retire' if value('retire') == 'manual' else value('set'),
                         value('fence')]
                if any(any(c.isspace() for c in part) for part in parts):
                    self.send_error(400)
                    return
                with lock:
                    owner.process.stdin.write(' '.join(parts) + '\n')
                    owner.process.stdin.flush()
                    result = json.loads(owner.process.stdout.readline())
                body = {'ok': True, 'mode': result['mode'],
                        'controlSessionAlive': result['alive'],
                        'controlSessionRevoked': result['revoked'],
                        'commandSequence': result['sequence'], **result}
                data = b'<html><body></body></html>' if route.path == '/fixture' else json.dumps(body).encode()
                self.send_response(result['status'])
                self.send_header('Content-Type', 'text/html' if route.path == '/fixture' else 'application/json')
                self.send_header('Content-Length', str(len(data)))
                self.send_header('Cache-Control', 'no-store')
                self.send_header('X-PIKU-Command-Sequence', str(result['sequence']))
                self.send_header('X-PIKU-Control-State', result['state'])
                if result['fence']:
                    self.send_header('X-PIKU-Mode-Fence', result['fence'])
                self.end_headers()
                self.wfile.write(data)

        self.server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.endpoint = 'http://127.0.0.1:' + str(self.server.server_port)

    def close(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)
        self.process.stdin.close()
        try:
            self.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.terminate()
            self.process.wait(timeout=5)
        self.process.stdout.close()
        self.process.stderr.close()
        self.fixture.tearDownClass()
