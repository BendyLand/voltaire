#!/bin/bash

METHODS=( $(awk '{print $2}' errs4) )
for item in "${METHODS[@]}"; do
	[[ "$item" != *"::"* ]] && continue

    class="${item%%::*}"
    file=$(rg "class $class[^;]" -l | head -n 1)

    if [[ -n "$file" ]]; then
        echo "$file"
    fi
done
