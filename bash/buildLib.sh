#!/bin/bash
#Description: Build MSAPI library

taskName="Build MSAPI library"
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

RunCommand "cmake -DCMAKE_BUILD_TYPE=${MSAPI_BUILD_PROFILE} ${options} -B ${MSAPI_PATH}/library/build ${MSAPI_PATH}/library/build \
	2>&1 | tee ${MSAPI_PATH}/library/build/cmake.txt" "cmake MSAPI library"
ExitIfError $?
RunCommand "cmake --build ${MSAPI_PATH}/library/build -j $(nproc) \
	2>&1 | tee ${MSAPI_PATH}/library/build/cmake_build.txt" "cmake build MSAPI library"
ExitIfError $?

echo -e "${GREEN}All is done successfully: ${taskName}${ENDCOLOR}"
exit 0