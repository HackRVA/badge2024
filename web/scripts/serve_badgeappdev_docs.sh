#!/bin/bash

source ./scripts/build_docs.sh

.bin/mdbook serve -p 8888 docs/badgeappdev --dest-dir ../../.dist/badgeappdev --open
