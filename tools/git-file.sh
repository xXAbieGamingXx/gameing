#!/bin/bash

set -euo pipefail

if [[ $# -ne 3 ]]; then
	echo "Usage: $0 [LINK/GITHUB \"OWNER/REPO\"] [GIT FILEPATH] [TARGET DIR]"
	echo "Ex: $0 floooh/sokol util/sokol_gl.h util/sokol_gl.h"
	exit 1
fi

if [[ $1 =~ \. ]]; then
	echo "Detected a url"
	if  wget --spider -q $1 ; then
		rm -rf $3
		dir=dirname $3
		cd dir
		wget $1
		echo "Fetched file"
	else
		echo "Unable to fetch url $1">&2
		exit 1
	fi
else
	branch=$(gh repo view $1 --json defaultBranchRef --jq .defaultBranchRef.name)
	github_url="https://raw.githubusercontent.com/$1/refs/heads/$branch/$2"
	if wget --spider -q -nv $github_url ; then
		rm -rf $3
		dir=$(dirname $3)
		cd $dir
		wget -nv $github_url
	else
		echo "Unable to fetch github url $github_url">&2
		exit 1
	fi
fi
