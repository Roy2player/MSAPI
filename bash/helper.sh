#!/bin/bash/
#Includes useful functions for the build scripts

VIOLET="\033[0;35m"
GREEN="\033[0;32m"
RED="\033[0;31m"
ENDCOLOR="\033[0m"

CheckArgumentsNumber() {
    if [ $# -ne 2 ]
    then
        echo -e "${RED}FAIL:${ENDCOLOR} CheckArgumentsNumber: cannot check, wrong number of arguments. Expected 2, actual $#"
        return 1
    fi

    if [ $1 -ne $2 ]
    then
        echo -e "${RED}FAIL:${ENDOLOR} CheckArgumentsNumber: wrong number of arguments. Expected: ${1}, actual: ${2}"
        return 1
    fi

    return 0
}

RunCommand() {
    CheckArgumentsNumber 2 $#

    echo -e "${VIOLET}START:${ENDCOLOR} ${2}"
    bash -c "set -o pipefail; ${1}"
    if [ $? -ne 0 ]
    then
            echo -e "${RED}FAIL:${ENDCOLOR} ${2}"
            return 1
    fi
    echo -e "${GREEN}SUCCESS:${ENDCOLOR} ${2}"
    return 0
}

ExitIfError() {
    if [ $# -ne 1 ]
    then
        exit 1
    fi

    if [ $1 -ne 0 ]
    then
        exit 1
    fi
}

CheckGlobalVariables () {
    result=false
    for i in "$@"; do
        if [ -z "${!i}" ]
        then
            echo "Global variable ${i} is not set"
            result=true
        fi
    done

    if [ $result == true ]
    then
        return 1
    fi

    return 0
}