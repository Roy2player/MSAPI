#!/bin/bash
#Description: Build MSAPI library and apps.

taskName="Build MSAPI library and apps"
echo -e "${VIOLET}START:${ENDCOLOR} ${taskName}"

. $(dirname "$0")/helper.sh

CheckGlobalVariables MSAPI_PATH
ExitIfError $?

if [ -z "${MSAPI_BUILD_PROFILE}" ]; then
	MSAPI_BUILD_PROFILE="Debug"
fi

options=""

if [ -n "${MSAPI_GCC}" ]; then
	options=$options" -DCMAKE_CXX_COMPILER="$MSAPI_GCC
fi

bash $(dirname ${BASH_SOURCE})/buildLib.sh
ExitIfError $?

declare -a apps=("manager")

SelectAppsToBuild apps[@] "$@"

for i in "${BUILD_APPS[@]}"; do
	RunCommand "cmake -DCMAKE_BUILD_TYPE=${MSAPI_BUILD_PROFILE} ${options} -B ${MSAPI_PATH}/apps/${i}/build ${MSAPI_PATH}/apps/${i}/build \
		2>&1 | tee ${MSAPI_PATH}/apps/${i}/build/cmake.txt" "cmake MSAPI ${i} app"
	ExitIfError $?
	RunCommand "cmake --build ${MSAPI_PATH}/apps/${i}/build -j $(nproc) \
		2>&1 | tee ${MSAPI_PATH}/apps/${i}/build/cmake_build.txt" "cmake build MSAPI ${i} app"
	ExitIfError $?
done

echo -e "${GREEN}All is done successfully: ${taskName}${ENDCOLOR}"
exit 0