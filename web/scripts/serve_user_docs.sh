#!/bin/sh

source ./scripts/build_user_docs.sh

.bin/mdbook serve -p 8888 docs/user_docs --dest-dir ../../.dist/ --open
