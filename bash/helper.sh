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

#SelectAppsToBuild <array_name> <args...>
#Sets BUILD_APPS array to the list of apps to build, based on arguments and default array
SelectAppsToBuild() {
    local default_apps_name="$1"
    shift
    local -a default_apps=("${!default_apps_name}")
    local -a build_apps=()
    local -a exclude_apps=()
    local exclude_mode=0

    if [ "$#" -eq 0 ]; then
        build_apps=("${default_apps[@]}")
    else
        for arg in "$@"; do
            if [[ "$arg" == -* ]]; then
                exclude_mode=1
                break
            fi
        done

        #Check all args are consistent with exclude_mode
        for arg in "$@"; do
            if [ $exclude_mode -eq 1 ]; then
                if [[ ! "$arg" == -* ]]; then
                    echo -e "${RED}Error:${ENDCOLOR} If any argument starts with '-', all must."
                    exit 1
                fi
            else
                if [[ "$arg" == -* ]]; then
                    echo -e "${RED}Error:${ENDCOLOR} If any argument starts with '-', all must."
                    exit 1
                fi
            fi
        done

        if [ $exclude_mode -eq 1 ]; then
            #Exclude mode: build all except listed
            for arg in "$@"; do
                local app_name="${arg#-}"
                local found=0
                for app in "${default_apps[@]}"; do
                    if [ "$app_name" == "$app" ]; then
                        found=1
                        #Check for duplicates
                        for b in "${exclude_apps[@]}"; do
                            if [ "$app_name" == "$b" ]; then
                                echo -e "${RED}Error:${ENDCOLOR} Duplicate app '$app_name' in arguments."
                                exit 1
                            fi
                        done
                        exclude_apps+=("$app_name")
                        break
                    fi
                done
                if [ $found -eq 0 ]; then
                    echo -e "${RED}Error:${ENDCOLOR} Unknown app '$app_name'. Allowed: ${default_apps[*]}"
                    exit 1
                fi
            done
            for app in "${default_apps[@]}"; do
                local skip=0
                for ex in "${exclude_apps[@]}"; do
                    if [ "$app" == "$ex" ]; then
                        skip=1
                        break
                    fi
                done
                if [ $skip -eq 0 ]; then
                    build_apps+=("$app")
                fi
            done
        else
            #Normal mode: build only listed apps
            for arg in "$@"; do
                local found=0
                #Check for duplicates
                for b in "${build_apps[@]}"; do
                    if [ "$arg" == "$b" ]; then
                        echo -e "${RED}Error:${ENDCOLOR} Duplicate app '$arg' in arguments."
                        exit 1
                    fi
                done
                for app in "${default_apps[@]}"; do
                    if [ "$arg" == "$app" ]; then
                        found=1
                        build_apps+=("$arg")
                        break
                    fi
                done
                if [ $found -eq 0 ]; then
                    echo -e "${RED}Error:${ENDCOLOR} Unknown app '$arg'. Allowed: ${default_apps[*]}"
                    exit 1
                fi
            done
        fi
    fi
    BUILD_APPS=("${build_apps[@]}")
}