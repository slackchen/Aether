"""Fetch fontsource woff2 subsets with verified 128KB range chunks."""
import re
import subprocess
import sys
import time

FILES = [
    ("Tools/_fontwork/noto-chinese.woff2",
     "https://cdn.jsdelivr.net/npm/@fontsource/noto-sans-sc@5.2.5/files/noto-sans-sc-chinese-simplified-700-normal.woff2"),
    ("Tools/_fontwork/noto-latin.woff2",
     "https://cdn.jsdelivr.net/npm/@fontsource/noto-sans-sc@5.2.5/files/noto-sans-sc-latin-700-normal.woff2"),
]


def content_length(url):
    head = subprocess.run(["curl", "-sI", "-m", "20", url], capture_output=True, text=True).stdout
    m = re.search(r"[Cc]ontent-[Ll]ength: (\d+)", head)
    return int(m.group(1)) if m else 0


def main():
    for path, url in FILES:
        total = content_length(url)
        chunk = 128 * 1024
        start, fails = 0, 0
        out = open(path, "wb")
        while start < total:
            end = min(start + chunk, total) - 1
            for attempt in range(25):
                r = subprocess.run(["curl", "-sL", "-m", "40", "-r", "%d-%d" % (start, end), url],
                                   capture_output=True)
                if r.returncode == 0 and len(r.stdout) == end - start + 1:
                    out.write(r.stdout)
                    break
                fails += 1
                time.sleep(0.2)
            else:
                out.close()
                sys.exit("stuck %s at %d" % (path, start))
            start = end + 1
        out.close()
        print("%s %d bytes OK (failed attempts: %d)" % (path, total, fails))


if __name__ == "__main__":
    main()
