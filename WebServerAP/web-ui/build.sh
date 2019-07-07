#!/usr/bin/env sh

./node_modules/.bin/webpack
cat dist/index.html \
	| tr '\n' ' ' \
	| sed -r 's/"/\\"/g' \
	| tee dist/c_embed_string.txt \
	| xclip -selection clipboard

