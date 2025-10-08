#!/bin/bash
#Description: Build MSAPI library, apps, tests and run them.

taskName="Build MSAPI library, apps, tests and run them"
echo -e "${VIOLET}START:${ENDCOLOR} ${taskName}"

. ${BASH_HELPER_PATH}/helper.sh

CheckGlobalVariables MSAPI_PATH
ExitIfError $?

if [ -z "${BUILD_PROFILE}" ]; then
	BUILD_PROFILE="Debug"
fi

bash $(dirname ${BASH_SOURCE})/buildMsapiLib.sh
ExitIfError $?

bash $(dirname ${BASH_SOURCE})/buildMsapiApps.sh
ExitIfError $?

bash $(dirname ${BASH_SOURCE})/executeMsapiTests.sh
ExitIfError $?

echo -e "${GREEN}All is done successfully: ${taskName}${ENDCOLOR}"
exit 0