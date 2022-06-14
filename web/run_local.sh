#!/bin/sh

DIR="web/node_modules"
if [ ! -d "$DIR" ]; then
  echo "the dependencies need to be installed ${DIR}..."
  cd web && npm install
fi

cd web && npm start
