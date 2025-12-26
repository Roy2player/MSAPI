/**************************
 * @file        authorization.inl
 * @version     6.0
 * @date        2025-12-26
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
 *
 * This file is part of MSAPI.
 * License: see LICENSE.md
 * Contributor terms: see CONTRIBUTING.md
 *
 * This software is licensed under the Polyform Noncommercial License 1.0.0.
 * You may use, copy, modify, and distribute it for noncommercial purposes only.
 *
 * For commercial use, please contact: maks.angels@mail.ru
 *
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 */

#ifndef MSAPI_UNIT_TEST_AUTHORIZATION_INL
#define MSAPI_UNIT_TEST_AUTHORIZATION_INL

#include "../../../../library/source/server/authorization.inl"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Authorization module.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Authorization();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Authorization()
{
	LOG_INFO_UNITTEST("MSAPI Authorization");
	MSAPI::Test t;

	{
		Authorization::Base::Module mod;
		RETURN_IF_FALSE(t.Assert(mod.IsInitialized(), true, "Module is initialized successfully"));
	}

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_AUTHORIZATION_INL