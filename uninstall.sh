#!/bin/sh
#
# trekscii uninstaller - removes the binary, the art directory, and the block
# the installer added to ~/.zshrc / ~/.bashrc.
#
#   ./uninstall.sh
#   ./uninstall.sh --prefix /usr/local

set -eu

PREFIX=${PREFIX:-$HOME/.local}

BEGIN_MARK="# >>> trekscii >>>"
END_MARK="# <<< trekscii <<<"

say()  { printf '%s\n' "$*"; }
warn() { printf 'trekscii: %s\n' "$*" >&2; }
die()  { warn "$*"; exit 1; }

while [ $# -gt 0 ]; do
    case $1 in
        --prefix)   [ $# -ge 2 ] || die "--prefix needs an argument"; PREFIX=$2; shift 2 ;;
        --prefix=*) PREFIX=${1#*=}; shift ;;
        -h|--help)  say "Usage: ./uninstall.sh [--prefix DIR]"; exit 0 ;;
        *)          die "unknown option '$1'" ;;
    esac
done

BINDIR=$PREFIX/bin
ARTDIR=$PREFIX/share/trekscii_art

if [ -e "$BINDIR/trekscii" ]; then
    rm -f "$BINDIR/trekscii"
    say "Removed $BINDIR/trekscii"
fi

if [ -d "$ARTDIR" ]; then
    rm -f "$ARTDIR"/*.txt
    rmdir "$ARTDIR" 2>/dev/null || warn "$ARTDIR not empty, left in place"
    say "Removed $ARTDIR"
fi

for rc in "$HOME/.zshrc" "$HOME/.bashrc" "$HOME/.zprofile" "$HOME/.bash_profile" "$HOME/.profile"; do
    [ -f "$rc" ] || continue
    grep -qF "$BEGIN_MARK" "$rc" || continue
    tmp=$(mktemp "${TMPDIR:-/tmp}/trekscii.XXXXXX")
    awk -v b="$BEGIN_MARK" -v e="$END_MARK" '
        index($0, b) == 1 { skip = 1 }
        !skip             { print }
        index($0, e) == 1 { skip = 0 }
    ' "$rc" > "$tmp"
    cat "$tmp" > "$rc"
    rm -f "$tmp"
    say "Removed trekscii block from $rc"
done

say "Done. Live long and prosper."
