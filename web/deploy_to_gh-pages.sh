#!/bin/sh

DIR="node_modules"
if [ ! -d "$DIR" ]; then
  echo "the dependencies need to be installed ${DIR}..."
  npm install
fi

npm run build
npm run deploy
