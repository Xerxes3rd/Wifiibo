#!/usr/bin/env bash

gzip -9 data/amiitool.htm -c > index.htm.gz
python bin2c.py index.htm.gz -o index_htm_gz
rm index.htm.gz
