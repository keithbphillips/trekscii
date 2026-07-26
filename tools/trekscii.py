#!/usr/bin/env python3
"""
trekscii.py - Reference Python implementation of trekscii.

The installed tool is the C version (src/trekscii.c); this one is kept for
hacking on the rendering without a compiler.

Usage:
    python3 tools/trekscii.py

It resolves the art directory the same way the C version does:
    1. $TREKSCII_ART_DIR
    2. $XDG_DATA_HOME/trekscii_art  (default ~/.local/share/trekscii_art)
    3. art/ in the source checkout
"""

import os
import random
import sys

# ── ANSI color codes ──────────────────────────────────────────────────────────
RESET = '\033[0m'

# (title_color, [art_line_colors...])
COLOR_SCHEMES = [
    # Starfleet Command Gold
    ('\033[1;33m',  ['\033[33m',  '\033[93m',  '\033[1;33m']),
    # Starfleet Operations Red
    ('\033[1;91m',  ['\033[31m',  '\033[91m',  '\033[1;31m']),
    # Starfleet Sciences Blue
    ('\033[1;94m',  ['\033[34m',  '\033[94m',  '\033[1;34m']),
    # Klingon Warrior (red/gold)
    ('\033[1;91m',  ['\033[91m',  '\033[93m',  '\033[31m']),
    # Romulan Green
    ('\033[1;32m',  ['\033[32m',  '\033[92m',  '\033[1;32m']),
    # UFP Cyan/Blue
    ('\033[1;96m',  ['\033[96m',  '\033[36m',  '\033[94m']),
    # Deep Space Nine (silver/white)
    ('\033[1;97m',  ['\033[97m',  '\033[37m',  '\033[1;97m']),
    # Ferengi Gold/Yellow
    ('\033[1;93m',  ['\033[93m',  '\033[33m',  '\033[1;93m']),
    # Cardassian Tan (bold white + yellow)
    ('\033[1;93m',  ['\033[97m',  '\033[93m',  '\033[33m']),
    # Holodeck Green-on-Black
    ('\033[1;92m',  ['\033[92m',  '\033[32m',  '\033[1;92m']),
]

def find_art_dir() -> str:
    """Return the first art directory that exists, mirroring src/trekscii.c."""
    override = os.environ.get('TREKSCII_ART_DIR')
    if override:
        if os.path.isdir(override):
            return override
        print(f"trekscii: TREKSCII_ART_DIR={override} is not a directory",
              file=sys.stderr)
        return override

    data_home = os.environ.get('XDG_DATA_HOME') or os.path.expanduser('~/.local/share')
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    for candidate in (os.path.join(data_home, 'trekscii_art'),
                      os.path.join(repo_root, 'art'),
                      '/usr/local/share/trekscii_art',
                      '/usr/share/trekscii_art'):
        if os.path.isdir(candidate):
            return candidate
    return os.path.join(data_home, 'trekscii_art')

# ── Parser ────────────────────────────────────────────────────────────────────

def parse_pieces(art_dir: str) -> list:
    """
    Load art pieces from individual .txt files in art_dir.
    Each file has the title on line 1 followed by art lines.
    Returns a list of (title, art_lines) tuples.
    """
    pieces = []
    try:
        filenames = sorted(f for f in os.listdir(art_dir) if f.endswith('.txt'))
    except OSError as exc:
        print(f"trekscii: cannot open {art_dir}: {exc}", file=sys.stderr)
        return pieces

    for fname in filenames:
        path = os.path.join(art_dir, fname)
        try:
            with open(path, 'r', encoding='utf-8', errors='replace') as fh:
                lines = fh.read().splitlines()
        except OSError as exc:
            print(f"trekscii: cannot read {path}: {exc}", file=sys.stderr)
            continue
        if not lines:
            continue
        title = lines[0].strip()
        art = lines[1:]
        # Strip surrounding blank lines
        while art and not art[0].strip():
            art.pop(0)
        while art and not art[-1].strip():
            art.pop()
        if title and len(art) >= 3:
            pieces.append((title, art))

    return pieces

# ── Display ───────────────────────────────────────────────────────────────────

def _display(title: str, art: list[str], title_color: str, art_colors: list[str]) -> None:
    art_width   = max((len(l) for l in art), default=40)
    title_width = len(title) + 4
    bar_width   = min(max(art_width, title_width, 40), 80)
    bar         = '─' * bar_width

    print(f"\n{title_color}{bar}{RESET}")
    print(f"{title_color}  {title}{RESET}")
    print(f"{title_color}{bar}{RESET}")
    for i, line in enumerate(art):
        color = art_colors[i % len(art_colors)]
        print(f"{color}{line}{RESET}")
    print()


def main() -> None:
    pieces = parse_pieces(find_art_dir())
    if not pieces:
        print("Live long and prosper.")
        return

    title, art = random.choice(pieces)
    title_color, art_colors = random.choice(COLOR_SCHEMES)
    _display(title, art, title_color, art_colors)


if __name__ == '__main__':
    main()
