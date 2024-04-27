#!/bin/sh


# etag file
tags=etags.txt

refresh_rq=0
tzcsv=https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv


USAGE="Usage: `basename $0` [-h] [-f]"

# parse cmd options
while getopts hf OPT; do
    case "$OPT" in
        h)
            echo $USAGE
            exit 0
            ;;
        f)
            echo "Force refresh"
            rm -f $tags
            refresh_rq=1
            ;;
        \?)
            # getopts issues an error message
            echo $USAGE >&2
            exit 1
            ;;
    esac
done

[ -f $tags ] || touch $tags

# check github file for a new hash
freshtag(){
    local url="$1"
    etag=$(curl -sL -I $url | grep etag | awk '{print $2}')
    echo "$url $etag" >> newetags.txt
    if [ $(grep -cs $etag $tags) -eq 0 ] ; then
        #echo "new tag found for $url"
        return 0
    fi
    #echo "old tag for $url"
    return 1
}

echo "Preparing resources for EmbUI FS image" 

mkdir -p ./data/css ./data/js
cat html/css/pure*.css html/css/grids*.css | gzip -9 > ./data/css/pure.css.gz
cat html/css/*_default.css | gzip -9 > ./data/css/style.css.gz
cat html/css/*_light.css | gzip -9 > ./data/css/style_light.css.gz
cat html/css/*_dark.css | gzip -9 > ./data/css/style_dark.css.gz

cp -u html/css/*.jpg ./data/css/
cp -u html/css/*.webp ./data/css/
cp -u html/css/*.gz ./data/css/


embui_js="dyncss.js lib.js maker.js"
# combine and compress js files in one bundle
for f in ${embui_js}
do
    cat html/js/${f} >> data/js/embui.js
done
gzip -9f data/js/embui.js

cp html/js/lodash.custom.js.gz data/js/

cat html/js/ui_sys.json | gzip -9 > ./data/js/ui_sys.json.gz
cat html/index.html | gzip -9 > ./data/index.html.gz
cat html/favicon.ico | gzip -9 > ./data/favicon.ico.gz

# update TZ info
if freshtag ${tzcsv} || [ $refresh_rq -eq 1 ] ; then
    echo "Updating TZ info"
    echo '"label","value"' > ./data/js/zones.csv
    curl -sL $tzcsv >> ./data/js/zones.csv
    python tzgen.py
    gzip -9f ./data/js/tz.json
    rm -f ./data/js/tz.json ./data/js/zones.csv
else
    unzip -o -d ./data/ data.zip "js/tz.json.gz"
fi


cd ./data
zip --filesync -r -0 --quiet ../data.zip ./*
cd ..
rm -r ./data

mv -f newetags.txt $tags

echo "Content of data.zip file should be used to create LittleFS image"