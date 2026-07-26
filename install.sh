#!/bin/sh
#
# trekscii installer
#
#   ./install.sh                     build + install to ~/.local, hook up your shell
#   ./install.sh --prefix /usr/local install system-wide (run with sudo)
#   ./install.sh --shell bash        force the shell to hook (zsh|bash|both|none)
#   ./install.sh --no-shell-init     install the files only, no rc file changes
#
# Installs:
#   $PREFIX/bin/trekscii
#   $PREFIX/share/trekscii_art/*.txt
# and adds a small guarded block to ~/.zshrc and/or ~/.bashrc so trekscii runs
# once per login shell session.

set -eu

SRC_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

PREFIX=${PREFIX:-$HOME/.local}
SHELL_CHOICE=auto
DO_SHELL_INIT=1

BEGIN_MARK="# >>> trekscii >>>"
END_MARK="# <<< trekscii <<<"

say()  { printf '%s\n' "$*"; }
warn() { printf 'trekscii: %s\n' "$*" >&2; }
die()  { warn "$*"; exit 1; }

usage() {
    cat <<EOF
Usage: ./install.sh [OPTIONS]

  --prefix DIR       install prefix (default: \$HOME/.local)
  --shell WHICH      shell startup to configure: auto|zsh|bash|both|none
                     (default: auto - detected from \$SHELL)
  --no-shell-init    do not touch any shell rc file
  -h, --help         show this help
EOF
}

while [ $# -gt 0 ]; do
    case $1 in
        --prefix)        [ $# -ge 2 ] || die "--prefix needs an argument"; PREFIX=$2; shift 2 ;;
        --prefix=*)      PREFIX=${1#*=}; shift ;;
        --shell)         [ $# -ge 2 ] || die "--shell needs an argument"; SHELL_CHOICE=$2; shift 2 ;;
        --shell=*)       SHELL_CHOICE=${1#*=}; shift ;;
        --no-shell-init) DO_SHELL_INIT=0; shift ;;
        -h|--help)       usage; exit 0 ;;
        *)               warn "unknown option '$1'"; usage >&2; exit 2 ;;
    esac
done

case $SHELL_CHOICE in
    auto|zsh|bash|both|none) ;;
    *) die "--shell must be one of: auto zsh bash both none" ;;
esac

BINDIR=$PREFIX/bin
ARTDIR=$PREFIX/share/trekscii_art

# ── 1. Build ──────────────────────────────────────────────────────────────────

CC=${CC:-}
if [ -z "$CC" ]; then
    for c in cc gcc clang tcc; do
        if command -v "$c" >/dev/null 2>&1; then CC=$c; break; fi
    done
fi
[ -n "$CC" ] || die "no C compiler found (install gcc or clang, or set CC=...)"

mkdir -p "$SRC_DIR/build"
say "Building with $CC..."
"$CC" -O2 -Wall -Wextra -std=c11 -o "$SRC_DIR/build/trekscii" "$SRC_DIR/src/trekscii.c"

# ── 2. Install files ──────────────────────────────────────────────────────────

mkdir -p "$BINDIR" "$ARTDIR"
install -m 0755 "$SRC_DIR/build/trekscii" "$BINDIR/trekscii"

# Replace the art directory contents wholesale so removed pieces don't linger,
# but only ever delete the .txt files we manage.
rm -f "$ARTDIR"/*.txt
install -m 0644 "$SRC_DIR"/art/*.txt "$ARTDIR/"

say "Installed $BINDIR/trekscii"
say "Installed $(ls -1 "$ARTDIR" | wc -l | tr -d ' ') art files to $ARTDIR"

# ── 3. Shell startup ──────────────────────────────────────────────────────────

# Emit the rc block. Includes a PATH fix-up only when BINDIR isn't already on
# PATH, and a guard variable so nested shells (tmux, subshells, `zsh` inside
# bash) stay quiet - the art shows once per login session.
snippet() {
    cat <<EOF
$BEGIN_MARK
# Added by the trekscii installer. Delete this block to remove it.
case ":\$PATH:" in
    *":$BINDIR:"*) ;;
    *) PATH="$BINDIR:\$PATH" ;;
esac
if [ -z "\${TREKSCII_GREETED-}" ] && [ -t 1 ] && command -v trekscii >/dev/null 2>&1; then
    export TREKSCII_GREETED=1
    trekscii
fi
$END_MARK
EOF
}

install_block() {
    rc=$1
    [ -e "$rc" ] || : > "$rc"

    if grep -qF "$BEGIN_MARK" "$rc" 2>/dev/null; then
        # Rewrite the existing block in place.
        tmp=$(mktemp "${TMPDIR:-/tmp}/trekscii.XXXXXX")
        awk -v b="$BEGIN_MARK" -v e="$END_MARK" '
            index($0, b) == 1 { skip = 1 }
            !skip            { print }
            index($0, e) == 1 { skip = 0 }
        ' "$rc" > "$tmp"
        snippet >> "$tmp"
        cat "$tmp" > "$rc"          # preserve the original file's inode/mode
        rm -f "$tmp"
        say "Updated trekscii block in $rc"
    else
        printf '\n' >> "$rc"
        snippet >> "$rc"
        say "Added trekscii block to $rc"
    fi
}

if [ "$DO_SHELL_INIT" -eq 1 ] && [ "$SHELL_CHOICE" != none ]; then
    targets=
    case $SHELL_CHOICE in
        zsh)  targets="$HOME/.zshrc" ;;
        bash) targets="$HOME/.bashrc" ;;
        both) targets="$HOME/.zshrc $HOME/.bashrc" ;;
        auto)
            case ${SHELL:-} in
                */zsh)  targets="$HOME/.zshrc" ;;
                */bash) targets="$HOME/.bashrc" ;;
                *)
                    if [ -f "$HOME/.zshrc" ]; then
                        targets="$HOME/.zshrc"
                    elif [ -f "$HOME/.bashrc" ]; then
                        targets="$HOME/.bashrc"
                    else
                        warn "could not detect your shell; re-run with --shell zsh|bash"
                    fi
                    ;;
            esac
            ;;
    esac
    for rc in $targets; do
        install_block "$rc"
    done
else
    say "Skipped shell startup configuration."
    say "To run trekscii at login, add this to your ~/.zshrc or ~/.bashrc:"
    snippet | sed 's/^/    /'
fi

# ── 4. Done ───────────────────────────────────────────────────────────────────

say ""
case ":$PATH:" in
    *":$BINDIR:"*) ;;
    *) say "Note: $BINDIR is not on your current PATH - open a new shell first." ;;
esac
say "Try it now:  $BINDIR/trekscii"
