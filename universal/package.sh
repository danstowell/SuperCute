#!/bin/sh

DATE=`date "+%Y-%m-%d"`

if [ -d SuperCollider ]; then
	echo "Please remove the ./SuperCollider directory before running this script."
	exit 1
fi

svn export --force build SuperCollider
cp -R build/SuperCollider.app build/scsynth build/sclang SuperCollider
cp build/plugins/* SuperCollider/plugins/
zip -r -9 "SuperCollider $DATE.zip" SuperCollider 
