@echo off
gzip -9 data\amiitool.htm -c > index.htm.gz
bin2c index.htm.gz index_htm_gz.h index_htm_gz
del index.htm.gz