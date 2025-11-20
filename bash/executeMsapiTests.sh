#!/bin/bash
#Description: Build MSAPI library, tests and run them.

taskName="Build MSAPI library, tests and run them"
echo -e "${VIOLET}START:${ENDCOLOR} ${taskName}"

. $(dirname "$0")/helper.sh

CheckGlobalVariables MSAPI_PATH
ExitIfError $?

if [ -z "${BUILD_PROFILE}" ]; then
	BUILD_PROFILE="Debug"
fi

bash $(dirname ${BASH_SOURCE})/buildMsapiLib.sh
ExitIfError $?

# Unit tests under tests/units/
declare -a unit_tests=("dataHeader" "application" "objectData" "standardData" "htmlHelper" "json" "table" "helper" "timer")

for i in "${unit_tests[@]}"; do
    RunCommand "cmake -DCMAKE_BUILD_TYPE=${BUILD_PROFILE} -B ${MSAPI_PATH}/tests/units/${i}/build ${MSAPI_PATH}/tests/units/${i}/build \
		2>&1 | tee ${MSAPI_PATH}/tests/units/${i}/build/cmake.txt" "cmake MSAPI units/${i} test"
	ExitIfError $?
	RunCommand "cmake --build ${MSAPI_PATH}/tests/units/${i}/build -j $(nproc) \
		2>&1 | tee ${MSAPI_PATH}/tests/units/${i}/build/cmake_build.txt" "cmake build MSAPI units/${i} test"
	ExitIfError $?

	projectName=$(grep "project(" "${MSAPI_PATH}/tests/units/${i}/build/CMakeLists.txt" | cut -d ' ' -f 1 | cut -d '(' -f 2)
	RunCommand "sudo ${MSAPI_PATH}/tests/units/${i}/build/${projectName}" "execute MSAPI ${projectName} test"
	ExitIfError $?
done

# Integration tests
declare -a tests=("units" "applicationHandlers" "objectProtocol" "http" "server")

for i in "${tests[@]}"; do
    RunCommand "cmake -DCMAKE_BUILD_TYPE=${BUILD_PROFILE} -B ${MSAPI_PATH}/tests/${i}/build ${MSAPI_PATH}/tests/${i}/build \
		2>&1 | tee ${MSAPI_PATH}/tests/${i}/build/cmake.txt" "cmake MSAPI ${i} test"
	ExitIfError $?
	RunCommand "cmake --build ${MSAPI_PATH}/tests/${i}/build -j $(nproc) \
		2>&1 | tee ${MSAPI_PATH}/tests/${i}/build/cmake_build.txt" "cmake build MSAPI ${i} test"
	ExitIfError $?

	projectName=$(grep "project(" "${MSAPI_PATH}/tests/${i}/build/CMakeLists.txt" | cut -d ' ' -f 1 | cut -d '(' -f 2)
	RunCommand "sudo ${MSAPI_PATH}/tests/${i}/build/${projectName}" "execute MSAPI ${projectName} test"
	ExitIfError $?
done

echo -e "${GREEN}All is done successfully: ${taskName}${ENDCOLOR}"
exit 0