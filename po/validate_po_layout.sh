#!/bin/sh
set -eu

po_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
pot="$po_dir/messages.pot"

command -v msgfmt >/dev/null 2>&1 || {
    echo "validate_po_layout: msgfmt is required" >&2
    exit 2
}
[ -f "$pot" ] || {
    echo "validate_po_layout: missing $pot" >&2
    exit 1
}

set -- "$po_dir"/*.po
[ -e "$1" ] || {
    echo "validate_po_layout: no PO catalogs found" >&2
    exit 1
}

for catalog do
    # Empty translations are intentional fallback entries in several
    # catalogs; syntax checking must not reject those entries.
    msgfmt --check -o /dev/null "$catalog" 2>/dev/null
done

echo "PO layout: PASS ($# catalogs)"
