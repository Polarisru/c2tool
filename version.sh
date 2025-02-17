#!/bin/sh

VERSION="0.01"
OUT="$1"

if [ -d .git ] && head=`git rev-parse --verify HEAD 2>/dev/null`; then
	git update-index --refresh --unmerged > /dev/null
	descr=$(git describe --always)

	# on git builds check that the version number above
	# is correct...
#	[ "${descr%%-*}" = "v$VERSION" ] || exit 2

	v="${descr#v}"
	if git diff-index --name-only HEAD | read dummy ; then
		v="$v"-dirty
	fi
	echo "$v"
else
	v="$VERSION"
fi

echo '#include "c2tool.h"' > "$1"
echo "const char c2tool_version[] = \"$v\";" >> "$1"
