"""Internal link + anchor + code-line-reference checker for tracked markdown."""
import os, re, subprocess, sys, unicodedata, urllib.parse

root = sys.argv[1]
files = subprocess.run(["git", "-C", root, "ls-files", "*.md"], capture_output=True, text=True, encoding="utf-8").stdout.split("\n")
files = [f for f in files if f and not f.startswith("third_party/")]

def slug(h):
    h = re.sub(r"`", "", h.strip())
    h = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", h)
    h = h.lower()
    out = []
    for ch in h:
        cat = unicodedata.category(ch)
        if ch in " -_":
            out.append("-" if ch == " " else ch)
        elif cat[0] in "LN" or cat == "Mn":
            out.append(ch)
    return "".join(out)

anchors = {}
def get_anchors(path):
    if path in anchors:
        return anchors[path]
    s = set(); seen = {}
    try:
        txt = open(path, encoding="utf-8").read()
    except Exception:
        anchors[path] = s; return s
    infence = False
    for line in txt.split("\n"):
        if line.startswith("```"):
            infence = not infence
        if infence: continue
        m = re.match(r"#{1,6}\s+(.*)", line)
        if m:
            sl = slug(m.group(1))
            n = seen.get(sl, 0); seen[sl] = n + 1
            s.add(sl if n == 0 else f"{sl}-{n}")
        for a in re.findall(r'<a\s+(?:name|id)="([^"]+)"', line):
            s.add(a)
    anchors[path] = s
    return s

bad = []
nlinks = 0
for f in files:
    p = os.path.join(root, f)
    txt = open(p, encoding="utf-8").read()
    # strip fenced code
    txt2 = re.sub(r"```.*?```", "", txt, flags=re.S)
    for m in re.finditer(r"\[[^\]]*\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)", txt2):
        url = m.group(1)
        if re.match(r"^(https?:|mailto:)", url):
            continue
        nlinks += 1
        path, _, frag = url.partition("#")
        path = urllib.parse.unquote(path)
        frag = urllib.parse.unquote(frag)
        target = p if not path else os.path.normpath(os.path.join(os.path.dirname(p), path))
        m2 = re.match(r"^(.*?):(\d+)$", target)
        if not os.path.exists(target):
            bad.append((f, url, "MISSING FILE"))
            continue
        if frag and target.endswith(".md"):
            if frag.lower() not in get_anchors(target) and not re.match(r"^L\d+", frag):
                bad.append((f, url, "MISSING ANCHOR"))
print("files", len(files), "internal links", nlinks, "broken", len(bad))
for b in bad:
    print(" | ".join(b))
