#!/usr/bin/env bash
set -euo pipefail

docs_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
theme_dir="$docs_dir/doxygen-awesome-css"
theme_revision=40e9b25b6174dd3b472d8868f63323a870dfeeb8

if [[ ! -f "$theme_dir/doxygen-awesome.css" ]]; then
    if [[ ! -d "$theme_dir/.git" ]]; then
        git clone --no-checkout https://github.com/jothepro/doxygen-awesome-css.git "$theme_dir"
    fi
    git -C "$theme_dir" checkout --detach "$theme_revision"
fi

cd "$docs_dir"
doxygen Doxyfile
