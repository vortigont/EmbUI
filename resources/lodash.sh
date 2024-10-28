#!/bin/sh

#npm i -g npm
#npm i -g lodash-cli

# make custom build of lodash js lib
#lodash include=at,get,has,hasIn,merge,mergeWith,set,unset,setWith
lodash include=at,get,has,merge,mergeWith,set,unset
zopfli lodash.custom.min.js
mv -f lodash.custom.min.js.gz ../data/js/lodash.custom.js.gz
rm -f *.js *.gz
# -c | gzip -9 > html/js/lodash.custom.js.gz
#lodash category=array,object -c | gzip -9 > lodash.custom.js.gz