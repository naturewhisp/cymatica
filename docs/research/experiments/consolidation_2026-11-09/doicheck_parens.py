"""Second pass: DOIs containing parentheses, which doicheck.py truncates at the first ')'."""
import json, re, subprocess, time, urllib.parse, urllib.request

files = [f for f in subprocess.run(["git", "ls-files", "*.md"], capture_output=True, text=True).stdout.split()
         if not f.startswith("third_party")]
seen = {}
for f in files:
    for i, line in enumerate(open(f, encoding="utf-8"), 1):
        for m in re.finditer(r"doi\.org/(10\.\d{4,9}/[^\s\]>\"|]*\([^\s\]>\"|]*)", line):
            raw = urllib.parse.unquote(m.group(1))
            out, depth = "", 0
            for ch in raw:
                if ch == "(":
                    depth += 1
                if ch == ")":
                    if depth == 0:
                        break
                    depth -= 1
                out += ch
            seen.setdefault(out.rstrip(".,;"), (f, i))
for doi, (f, i) in seen.items():
    try:
        req = urllib.request.Request("https://api.crossref.org/works/" + urllib.parse.quote(doi, safe="/()"),
                                     headers={"User-Agent": "doc-citation-check/1.0"})
        j = json.loads(urllib.request.urlopen(req, timeout=30).read())["message"]
        year = (j.get("issued", {}).get("date-parts") or [[0]])[0][0]
        print("OK ", doi, "|", (j.get("author") or [{}])[0].get("family"), year, (j.get("title") or ["?"])[0][:70], "|", f, i)
    except Exception as e:
        print("ERR", doi, e, f, i)
    time.sleep(0.2)
