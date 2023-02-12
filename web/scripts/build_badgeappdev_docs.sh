#!/bin/bash

source ./scripts/download_mdbook.sh
mkdir -p .dist

.bin/mdbook build docs/badgeappdev --dest-dir ../../.dist/badgeappdev
