"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Edition:
##  09/11/2026 by Tsukini

File Name:
##  options.py

File Description:
##  Options page of the documentation site, generated from xstyle --help
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

##### Import #####
# Import that can't be in the try
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent)) # The generators are loaded from their file, not as a package

# Import that can be checked
try:
    import subprocess # Used to run xstyle
    import re # Used to read the help
    from rules import binary, escape # Same binary and Markdown escape as the rules page
except ImportError as e:
    raise SystemExit(f"Import Error ({__file__}): {e}")

##### Const #####
# Groups of xstyle --help documented as tables: id, title (en, fr)
GROUPS = {
    "SELECTION": ("selection", "Selection", "Sélection"),
    "FIX": ("fix", "Fix", "Correction"),
    "OUTPUT": ("output", "Output", "Sortie"),
    "INFORMATION": ("information", "Information", "Information"),
}
OPTION = re.compile(r"^\s{4}(?:(-\w), )?\s*(--[\w-]+)((?: <[^>]+>| \[[^\]]+\])?)\s{2,}(.*)$")
ENV = re.compile(r"\s*\(env (XSTYLE_\w+(?:=1)?)\)")
TEXT = {
    "en": {"title": "Options", "summary": "Every flag of xstyle with its environment variable, read from xstyle --help.",
        "intro": "Generated from `xstyle --help` of version {version}. Paths are bare arguments (default `.`); `-r` scans the directories recursively (always done with no path).",
        "head": "| Flag | Value | Environment | Effect |", "exit": "Exit status", "exit_md": "| Status | Meaning |\n|---|---|\n| `0` | clean |\n| `1` | issues left (at or above `--fail-on`; the mandatory R34 never counts) |\n| `255` | error |"},
    "fr": {"title": "Options", "summary": "Tous les flags de xstyle avec leur variable d'environnement, lus depuis xstyle --help.",
        "intro": "Généré depuis `xstyle --help` de la version {version} (descriptions du programme, en anglais). Les chemins sont des arguments nus (par défaut `.`) ; `-r` parcourt les dossiers récursivement (toujours fait sans chemin).",
        "head": "| Flag | Valeur | Environnement | Effet |", "exit": "Code de sortie", "exit_md": "| Code | Sens |\n|---|---|\n| `0` | propre |\n| `1` | problèmes restants (au niveau de `--fail-on` ou plus ; R34, obligatoire, ne compte jamais) |\n| `255` | erreur |"},
}

##### Tools #####
def options(ctx: "Context") -> dict[str, list[tuple[str, str, str, str, str]]]: # Group -> (short, long, value, env, description)
    result = subprocess.run([binary(ctx), "--help"], capture_output=True, text=True, check=True)
    groups: dict[str, list[tuple[str, str, str, str, str]]] = {}
    current = ""
    for line in result.stdout.splitlines():
        if line in GROUPS:
            current = line
            groups[current] = []
            continue
        if not line.startswith(" ") and line.strip():
            current = ""
        match = OPTION.match(line) if current else None
        if match:
            env = ENV.search(match.group(4))
            description = ENV.sub("", match.group(4))
            groups[current].append((match.group(1) or "", match.group(2), match.group(3).strip(), env.group(1) if env else "", description))
    return groups

def flag(short: str, name: str) -> str: # `-f`, `--fix`
    return f"`{short}`, `{name}`" if short else f"`{name}`"

##### Program #####
def pages(ctx: "Context") -> list[dict]:
    groups = options(ctx)
    page = {"slug": "options", "section": "reference", "order": 1, "kind": ""}
    for lang in ctx.langs:
        text = TEXT[lang]
        md = [text["intro"].format(version=ctx.version or "-"), ""]
        for key, (gid, title_en, title_fr) in GROUPS.items():
            md += [f"## {title_en if lang == 'en' else title_fr} {{#{gid}}}", "", text["head"], "|---|---|---|---|"]
            md += [f"| {flag(s, n)} | {('`' + escape(v) + '`') if v else '-'} | {('`' + e + '`') if e else '-'} | {escape(d)} |"
                for s, n, v, e, d in groups.get(key, [])]
            md.append("")
        md += [f"## {text['exit']} {{#exit}}", "", text["exit_md"], ""]
        page[lang] = {"title": text["title"], "summary": text["summary"], "md": "\n".join(md),
            "keywords": " ".join(n for g in groups.values() for _, n, _, _, _ in g)}
    return [page]

def search_entries(ctx: "Context") -> list[dict]:
    return [{"k": "option", "l": lang, "t": name, "d": description, "u": f"options.html#{GROUPS[key][0]}", "x": f"{short} {env}"}
        for key, group in options(ctx).items() for short, name, _, env, description in group for lang in ctx.langs]
