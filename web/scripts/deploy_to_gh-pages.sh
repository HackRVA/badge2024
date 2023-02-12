#!/bin/sh

source ./scripts/download-mdbook.sh
source ./build_badgeappdev_docs.sh
source ./build_user_docs.sh

GIT_REPO_URL=$(git config --get remote.origin.url)

cd .dist
git init .
git remote add github $GIT_REPO_URL
git checkout -b gh-pages
git add .
git commit -am "Static site deploy"
git push github gh-pages --force
cd ../..
