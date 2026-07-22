#!/bin/bash

FROM="$1"
TO="$2"

if [[ -z "$FROM" || -z "$TO" ]]; then
	echo "Usage: replace.sh <from> <to>"
	exit
fi

rg -l "$FROM" core/ editor/ drivers/ main/ misc/ modules/ scene/ tests/ servers/ thirdparty/ platform/  | xargs sd "$FROM" "$TO"
