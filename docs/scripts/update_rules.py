"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Edition:
##  09/11/2026 by Tsukini

File Name:
##  update_rules.py

File Description:
##  Regenerate the rules table of docs/index.html from xstyle --list-rules
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

##### Import #####
# Import that can't be in the try
from sys import exit, stderr, argv

# Import that can be checked
try:
    from pathlib import Path # Used to read & write the page
    import subprocess # Used to run xstyle
    import html # Used to escape the descriptions
    import re # Used to split the columns
except ImportError as e:
    stderr.write(f"Import Error ({__file__}): {e}\n")
    exit(255)

# Check if the program is call and not imported
if __name__ != "__main__":
    stderr.write(f"The {__file__} can only be executed and not imported!\n")
    exit(255)

##### Const #####
ROOT = Path(__file__).resolve().parents[2]
PAGE = ROOT / "docs" / "index.html"
START = "<!-- xstyle:rules:start -->"
END = "<!-- xstyle:rules:end -->"
FAMILIES = {"R34": "mandatory", "G": "generic", "CPP": "cpp", "LU": "libutils", "PY": "python", "SH": "shell", "RS": "rust", "CMAKE": "cmake"}

##### Tools #####
def rows(binary: str) -> list[list[str]]: # Code, severity, fix, languages, description of every rule
    result = subprocess.run([binary, "--list-rules", "--no-color"], capture_output=True, text=True, check=True)
    lines = result.stdout.splitlines()[1:]
    return [re.split(r"\s{2,}", line.strip(), maxsplit=4) for line in lines if line.strip()]

def table(rules: list[list[str]]) -> str: # The <tr> of every rule, filterable by family / severity
    out = []
    for code, severity, fix, languages, description in rules:
        family = FAMILIES.get(code.split("-")[0], "other")
        out.append(f'<tr data-family="{family}" data-severity="{severity}"><td><code>{code}</code></td>'
            f'<td><span class="sev {severity}">{severity}</span></td><td>{fix}</td>'
            f'<td><code>{html.escape(languages)}</code></td><td>{html.escape(description)}</td></tr>')
    return "\n".join(out)

##### Program #####
binary = argv[1] if len(argv) > 1 else str(ROOT / "xstyle")
if not Path(binary).exists():
    stderr.write(f"Error: xstyle not built ({binary}), build it first (make)\n")
    exit(1)
page = PAGE.read_text(encoding="utf-8")
if START not in page or END not in page:
    stderr.write(f"Error: markers {START} / {END} not found ({PAGE})\n")
    exit(1)
rules = rows(binary)
body = page[:page.index(START) + len(START)] + "\n" + table(rules) + "\n" + page[page.index(END):]
body = re.sub(r'(<span class="rules-count">)\d+(</span>)', rf"\g<1>{len(rules)}\g<2>", body)
PAGE.write_text(body, encoding="utf-8")
print(f"{len(rules)} rules written in {PAGE.relative_to(ROOT)}")
exit(0)
