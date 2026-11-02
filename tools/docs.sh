#!/bin/bash
# Builds the documentation: the API reference with Doxygen, the diagram with Graphviz, the site with MkDocs.
# The site is written to build/docs/site. A Doxygen warning or a broken link stops the build.
# Usage: tools/docs.sh
set -eu
cd "$(git rev-parse --show-toplevel)"
for tool in doxygen dot mkdocs; do
    command -v "$tool" > /dev/null || { echo "docs: $tool is not installed" >&2; exit 1; }
done
rm -rf docs/api docs/img/architecture.svg build/docs
mkdir -p docs/img
mkdir -p build/docs
PROJECT_NUMBER=$(git describe --tags --always --dirty) doxygen docs/Doxyfile
dot -Tsvg docs/architecture.dot -o docs/img/architecture.svg
# No --quiet: it lowers the log level to errors, and then a warning no longer counts as a failure in strict mode.
mkdocs build --strict > build/docs/mkdocs.log 2>&1 || { cat build/docs/mkdocs.log >&2; exit 1; }
echo "docs: $(find build/docs/site -name '*.html' | wc -l) pages in build/docs/site"
