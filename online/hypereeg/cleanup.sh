#!/bin/bash

set -e

echo "Cleaning node-* directories..."

for dir in node-*; do
    [ -d "$dir" ] || continue

    echo "--------------------------------------------------"
    echo "Processing $dir"

    (
        cd "$dir"

        # qmake cleanup if Makefile exists
        if [ -f Makefile ]; then
            make clean || true
        fi

        rm -f .qmake.stash
        rm -f Makefile

        # Remove executable having same name as directory
        exe="$(basename "$dir")"
        if [ -f "$exe" ] && [ -x "$exe" ]; then
            echo "Removing executable: $exe"
            rm -f "$exe"
        fi

        # Common Qt build leftovers
        rm -f *.o
        rm -f moc_*.cpp
        rm -f moc_*.o
        rm -f qrc_*.cpp
        rm -f qrc_*.o
        rm -f ui_*.h
    )
done

echo "Done."
