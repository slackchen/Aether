"""CDP-driven headless test: load a page, collect console output, take screenshots."""
import base64
import json
import subprocess
import sys
import time
import urllib.request

import websocket

CHROME = r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe"
PORT = 9223


def launch_chrome(url):
    proc = subprocess.Popen([
        CHROME,
        "--headless=new",
        "--enable-unsafe-webgpu",
        "--disable-extensions",
        "--remote-allow-origins=*",
        "--remote-debugging-port=%d" % PORT,
        "--window-size=1280,720",
        "--user-data-dir=" + __import__("tempfile").mkdtemp(),
        "about:blank",
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    # wait for devtools endpoint; pick the first real page tab (not extension/iframe targets)
    for _ in range(60):
        try:
            with urllib.request.urlopen("http://localhost:%d/json/list" % PORT, timeout=1) as r:
                tabs = json.loads(r.read())
            pages = [t for t in tabs if t.get("type") == "page" and t.get("url", "").startswith("http")]
            if not pages:
                pages = [t for t in tabs if t.get("type") == "page"]
            if pages:
                return proc, pages[0]["webSocketDebuggerUrl"]
        except Exception:
            pass
        time.sleep(0.5)
    raise RuntimeError("devtools endpoint never came up")


def main():
    url = sys.argv[1]
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 15.0
    shot_path = sys.argv[3] if len(sys.argv) > 3 else None

    proc, ws_url = launch_chrome(url)
    try:
        ws = websocket.create_connection(ws_url, timeout=10)
        msg_id = [0]

        def send(method, params=None):
            msg_id[0] += 1
            ws.send(json.dumps({"id": msg_id[0], "method": method, "params": params or {}}))
            return msg_id[0]

        send("Runtime.enable")
        send("Page.enable")
        send("Log.enable")
        nav_id = send("Page.navigate", {"url": url})

        logs = []
        all_methods = []
        contexts = []
        ws.settimeout(0.5)
        deadline = time.time() + seconds
        while time.time() < deadline:
            try:
                raw = ws.recv()
                if isinstance(raw, bytes):
                    raw = raw.decode("utf-8", "replace")
                msg = json.loads(raw)
            except websocket.WebSocketTimeoutException:
                continue
            except Exception as e:
                logs.append("[ws error] %r" % e)
                break
            method = msg.get("method", "")
            if len(all_methods) < 20:
                all_methods.append(method or ("reply id=%s" % msg.get("id")))
            if method == "Runtime.executionContextCreated":
                ctx = msg["params"]["context"]
                contexts.append(ctx)
                logs.append("[context] id=%s name=%s origin=%s" % (ctx.get("id"), ctx.get("name"), ctx.get("origin")))
            if method == "Runtime.consoleAPICalled":
                parts = []
                for p in msg["params"].get("args", []):
                    v = p.get("value")
                    parts.append(str(v) if v is not None else p.get("description", "?"))
                logs.append("[%s] %s" % (msg["params"].get("type", "log"), " ".join(parts)))
            elif method == "Runtime.exceptionThrown":
                details = msg["params"].get("exceptionDetails", {})
                text = details.get("exception", {}).get("description") or details.get("text", "?")
                logs.append("[EXCEPTION] %s" % text)
            elif method == "Log.entryAdded":
                e = msg["params"]["entry"]
                logs.append("[%s] %s" % (e.get("level"), e.get("text")))

        print("=== first methods seen: %s" % all_methods)
        print("=== console (%d entries) ===" % len(logs))
        for line in logs:
            print(line)

        # inspect page state in the newest execution context
        ctx_id = contexts[-1]["id"] if contexts else None
        for expr in [
            "location.href",
            "document.getElementById('loading') ? document.getElementById('loading').textContent : 'no-el'",
            "typeof Module !== 'undefined' ? ('calledRun=' + !!Module.calledRun) : 'no-Module'",
            "navigator.gpu ? 'navigator.gpu OK' : 'navigator.gpu MISSING'",
        ]:
            params = {"expression": expr, "returnByValue": True}
            if ctx_id is not None:
                params["contextId"] = ctx_id
            eid = send("Runtime.evaluate", params)
            ws.settimeout(5)
            while True:
                raw = ws.recv()
                if isinstance(raw, bytes):
                    raw = raw.decode("utf-8", "replace")
                msg = json.loads(raw)
                if msg.get("id") == eid:
                    r = msg.get("result", {}).get("result", {})
                    print("=== eval: %s => %s" % (expr, r.get("value", r.get("description", "?"))))
                    break

        if shot_path:
            sid = send("Page.captureScreenshot", {"format": "png"})
            # drain responses until we get the screenshot reply
            ws.settimeout(10)
            while True:
                raw = ws.recv()
                if isinstance(raw, bytes):
                    raw = raw.decode("utf-8", "replace")
                msg = json.loads(raw)
                if msg.get("id") == sid and "result" in msg:
                    data = base64.b64decode(msg["result"]["data"])
                    with open(shot_path, "wb") as f:
                        f.write(data)
                    print("=== screenshot saved: %s (%d bytes) ===" % (shot_path, len(data)))
                    break
        ws.close()
    finally:
        proc.terminate()


if __name__ == "__main__":
    main()
