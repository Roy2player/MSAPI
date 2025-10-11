#!/bin/bash
#Description: Build MSAPI library and apps.

taskName="Build MSAPI library and apps"
echo -e "${VIOLET}START:${ENDCOLOR} ${taskName}"

. $(dirname "$0")/helper.sh

CheckGlobalVariables MSAPI_PATH
ExitIfError $?

if [ -z "${BUILD_PROFILE}" ]; then
	BUILD_PROFILE="Debug"
fi

bash $(dirname ${BASH_SOURCE})/buildMsapiLib.sh
ExitIfError $?

declare -a apps=("manager")

for i in "${apps[@]}"; do
	RunCommand "cmake -DCMAKE_BUILD_TYPE=${BUILD_PROFILE} -B ${MSAPI_PATH}/apps/${i}/build ${MSAPI_PATH}/apps/${i}/build \
		2>&1 | tee ${MSAPI_PATH}/apps/${i}/build/cmake.txt" "cmake MSAPI ${i} app"
	ExitIfError $?
	RunCommand "cmake --build ${MSAPI_PATH}/apps/${i}/build -j $(nproc) \
		2>&1 | tee ${MSAPI_PATH}/apps/${i}/build/cmake_build.txt" "cmake build MSAPI ${i} app"
	ExitIfError $?
done

echo -e "${GREEN}All is done successfully: ${taskName}${ENDCOLOR}"
exit 0