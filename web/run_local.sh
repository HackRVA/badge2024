#!/bin/sh

DIR="web/node_modules"
if [ ! -d "$DIR" ]; then
  echo "the dependencies need to be installed ${DIR}..."
  npm install
fi

npm start
