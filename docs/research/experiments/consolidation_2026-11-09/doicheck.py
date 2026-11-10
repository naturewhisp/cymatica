"""Resolve every DOI and arXiv id cited in tracked markdown; print registry metadata next to the citing line."""
import json, os, re, subprocess, sys, time, urllib.parse, urllib.request

root = sys.argv[1]
files = subprocess.run(["git", "-C", root, "ls-files", "*.md"], capture_output=True, text=True, encoding="utf-8").stdout.split("\n")
files = [f for f in files if f and not f.startswith("third_party/")]
dois, arx = {}, {}
for f in files:
    for i, line in enumerate(open(os.path.join(root, f), encoding="utf-8"), 1):
        for m in re.finditer(r"doi\.org/(10\.[^\s)\]>\"|]+)", line):
            d = urllib.parse.unquote(m.group(1)).rstrip(".,;")
            dois.setdefault(d.lower(), []).append((f, i, line.strip()))
        for m in re.finditer(r"arxiv\.org/(?:abs|pdf)/(\d{4}\.\d{4,5})", line):
            arx.setdefault(m.group(1), []).append((f, i, line.strip()))

def get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "doc-citation-check/1.0"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return r.read().decode("utf-8", "replace")

def ctx(line, key):
    k = line.lower().find(key.lower()[:12])
    s = max(0, k - 150)
    return line[s:k + 20].replace("\n", " ")

out = []
for d, uses in sorted(dois.items()):
    try:
        j = json.loads(get("https://api.crossref.org/works/" + urllib.parse.quote(d, safe="/")))["message"]
        au = ", ".join(a.get("family", a.get("name", "?")) for a in j.get("author", [])[:4])
        yr = (j.get("issued", {}).get("date-parts") or [[None]])[0][0]
        meta = f"{au} ({yr}) {(j.get('title') or ['?'])[0][:110]} — {(j.get('container-title') or [''])[0][:50]}"
    except Exception as e:
        meta = f"!! NOT RESOLVED via Crossref: {e}"
    f, i, line = uses[0]
    out.append(f"DOI {d}\n   REG: {meta}\n   USE[{len(uses)}] {f}:{i}: …{ctx(line, d)}")
    time.sleep(0.15)
for a, uses in sorted(arx.items()):
    try:
        x = get("http://export.arxiv.org/api/query?id_list=" + a)
        t = re.findall(r"<title>(.*?)</title>", x, flags=re.S)
        au = re.findall(r"<name>(.*?)</name>", x)
        meta = f"{', '.join(au[:3])} — {' '.join(t[-1].split())[:110] if t else '?'}"
        if "<entry>" not in x: meta = "!! NO ENTRY"
    except Exception as e:
        meta = f"!! NOT RESOLVED: {e}"
    f, i, line = uses[0]
    out.append(f"ARXIV {a}\n   REG: {meta}\n   USE[{len(uses)}] {f}:{i}: …{ctx(line, a)}")
    time.sleep(3.1)
open(sys.argv[2], "w", encoding="utf-8").write("\n".join(out))
print(len(dois), "DOIs,", len(arx), "arXiv ids")
