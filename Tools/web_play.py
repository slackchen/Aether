"""CDP-driven autoplayer: start the shmup demo, hold fire, watch for exceptions."""
import json
import subprocess
import sys
import time
import urllib.request

import websocket

sys.path.insert(0, __import__("os").path.dirname(__file__))
from web_test import launch_chrome  # noqa: E402


def key(ws, mid, key_code, code, key_down=True):
    mid[0] += 1
    ws.send(json.dumps({
        "id": mid[0],
        "method": "Input.dispatchKeyEvent",
        "params": {
            "type": "keyDown" if key_down else "keyUp",
            "windowsVirtualKeyCode": key_code,
            "code": code,
            "key": code.replace("Key", "").replace("Enter", "Enter"),
        },
    }))


def drain(ws, logs, seconds):
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
        if method == "Runtime.consoleAPICalled":
            parts = []
            for p in msg["params"].get("args", []):
                v = p.get("value")
                parts.append(str(v) if v is not None else p.get("description", "?"))
            logs.append("[%s] %s" % (msg["params"].get("type", "log"), " ".join(parts)))
        elif method == "Runtime.exceptionThrown":
            details = msg["params"]["exceptionDetails"]
            text = details.get("exception", {}).get("description") or details.get("text", "?")
            logs.append("[EXCEPTION] %s" % text)
        elif method == "Log.entryAdded":
            e = msg["params"]["entry"]
            if e.get("level") in ("error", "warning"):
                logs.append("[%s] %s" % (e.get("level"), e.get("text")))
    return logs


def main():
    url = sys.argv[1]
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 45.0

    proc, ws_url = launch_chrome(url)
    try:
        ws = websocket.create_connection(ws_url, timeout=10)
        mid = [0]
        logs = []

        def send(method, params=None):
            mid[0] += 1
            ws.send(json.dumps({"id": mid[0], "method": method, "params": params or {}}))

        send("Runtime.enable")
        send("Page.enable")
        send("Log.enable")
        send("Page.navigate", {"url": url})
        drain(ws, logs, 4.0)  # let wasm load

        key(ws, mid, 13, "Enter")   # start game
        time.sleep(0.15)
        key(ws, mid, 13, "Enter", key_down=False)
        time.sleep(1.0)
        key(ws, mid, 74, "KeyJ")    # hold fire
        print("=== playing for %.0fs with fire held ===" % seconds)
        drain(ws, logs, seconds)
        key(ws, mid, 74, "KeyJ", key_down=False)

        print("=== console (%d entries) ===" % len(logs))
        for line in logs:
            print(line)
        ws.close()
    finally:
        proc.terminate()


if __name__ == "__main__":
    main()
