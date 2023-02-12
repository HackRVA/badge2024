#!/bin/bash

npm run build --prefix=docs/user_docs
cp -R docs/user_docs/_site/* .dist/
