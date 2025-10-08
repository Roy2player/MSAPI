#!/bin/bash
#Description: Build MSAPI library

taskName="Build MSAPI library"
echo -e "${VIOLET}START:${ENDCOLOR} ${taskName}"

. ${BASH_HELPER_PATH}/helper.sh

CheckGlobalVariables MSAPI_PATH
ExitIfError $?

if [ -z "${BUILD_PROFILE}" ]; then
	BUILD_PROFILE="Debug"
fi

RunCommand "cmake -DCMAKE_BUILD_TYPE=${BUILD_PROFILE} -B ${MSAPI_PATH}/library/build ${MSAPI_PATH}/library/build \
	2>&1 | tee ${MSAPI_PATH}/library/build/cmake.txt" "cmake MSAPI library"
ExitIfError $?
RunCommand "cmake --build ${MSAPI_PATH}/library/build -j ${NPROC} \
	2>&1 | tee ${MSAPI_PATH}/library/build/cmake_build.txt" "cmake build MSAPI library"
ExitIfError $?

echo -e "${GREEN}All is done successfully: ${taskName}${ENDCOLOR}"
exit 0