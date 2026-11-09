"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Edition:
##  09/11/2026 by Tsukini

File Name:
##  rules.py

File Description:
##  Rules page of the documentation site, generated from xstyle --list-rules
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

##### Import #####
# Import that can be checked
try:
    from pathlib import Path # Used to find the xstyle binary
    import subprocess # Used to run xstyle
    import shutil # Used to find xstyle in the PATH
    import re # Used to split the columns
except ImportError as e:
    raise SystemExit(f"Import Error ({__file__}): {e}")

##### Const #####
# Family of a rule from the prefix of its code: id of its section, title (en, fr)
FAMILIES = {
    "R34": ("mandatory", "Mandatory", "Obligatoire"),
    "G": ("generic", "Generic (every language)", "Génériques (tous les langages)"),
    "CPP": ("cpp", "C++ / C", "C++ / C"),
    "CMAKE": ("cmake", "CMake", "CMake"),
    "LU": ("libutils", "libutils", "libutils"),
    "PY": ("python", "Python", "Python"),
    "RS": ("rust", "Rust", "Rust"),
    "SH": ("shell", "Shell", "Shell"),
}
TEXT = {
    "en": {"title": "All the rules", "summary": "Every rule of xstyle: its code, severity, fix level and languages, read from xstyle --list-rules.",
        "intro": "**{count} rules**, generated from `xstyle --list-rules` of version {version}. `xstyle -x CODE` explains one in the terminal; the severities and fix levels are explained in [Severities and fix levels](page:severities#idea).",
        "head": "| Code | Severity | Fix | Languages | Rule |", "lang": "`*` = every file checked."},
    "fr": {"title": "Toutes les règles", "summary": "Toutes les règles de xstyle : code, sévérité, niveau de fix et langages, lus depuis xstyle --list-rules.",
        "intro": "**{count} règles**, générées depuis `xstyle --list-rules` de la version {version}. `xstyle -x CODE` en explique une dans le terminal ; les sévérités et niveaux de fix sont expliqués dans [Sévérités et niveaux de fix](page:severities#idea). Les textes des règles sont ceux du programme (en anglais).",
        "head": "| Code | Sévérité | Fix | Langages | Règle |", "lang": "`*` = tous les fichiers vérifiés."},
}

##### Tools #####
def binary(ctx: "Context") -> str: # ./xstyle of the repository, else the one of the PATH
    local = (ctx.site / ".." / ".." / "xstyle").resolve()
    if local.exists():
        return str(local)
    found = shutil.which("xstyle")
    if not found:
        raise SystemExit("Error: xstyle not built (make) nor installed: the rules page reads xstyle --list-rules")
    return found

def escape(text: str) -> str: # Markdown text of a description (no emphasis, no table cell break)
    text = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    return re.sub(r"([\\`*_|\[\]])", r"\\\1", text)

def rules(ctx: "Context") -> list[list[str]]: # Code, severity, fix, languages, description of every rule
    result = subprocess.run([binary(ctx), "--list-rules", "--no-color"], capture_output=True, text=True, check=True)
    return [re.split(r"\s{2,}", line.strip(), maxsplit=4) for line in result.stdout.splitlines()[1:] if line.strip()]

def family(code: str) -> tuple[str, str, str]: # Section of a rule code
    return FAMILIES.get(code.split("-")[0], ("other", "Other", "Autres"))

##### Program #####
def pages(ctx: "Context") -> list[dict]:
    table = rules(ctx)
    page = {"slug": "rules", "section": "reference", "order": 2, "kind": ""}
    for lang in ctx.langs:
        text = TEXT[lang]
        md = [text["intro"].format(count=len(table), version=ctx.version or "-"), "", text["lang"], ""]
        for key, (fid, title_en, title_fr) in FAMILIES.items():
            rows = [r for r in table if family(r[0])[0] == fid]
            if not rows:
                continue
            md += [f"## {title_en if lang == 'en' else title_fr} {{#{fid}}}", "", text["head"], "|---|---|---|---|---|"]
            md += [f"| `{c}` | {s} | {f} | `{escape(l)}` | {escape(d)} |" for c, s, f, l, d in rows]
            md.append("")
        page[lang] = {"title": text["title"], "summary": text["summary"], "md": "\n".join(md),
            "keywords": " ".join(r[0] for r in table)}
    return [page]

def search_entries(ctx: "Context") -> list[dict]:
    return [{"k": "rule", "l": lang, "t": code, "d": f"{severity}, fix {fix}: {description}",
        "u": f"rules.html#{family(code)[0]}", "x": languages}
        for code, severity, fix, languages, description in rules(ctx) for lang in ctx.langs]
