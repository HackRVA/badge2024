#!/bin/bash

source ./scripts/download_mdbook.sh
mkdir -p .dist

.bin/mdbook build docs/user_docs --dest-dir ../../.dist/
