/**************************
 * @file        application.inl
 * @version     6.0
 * @date        2025-11-20
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

#ifndef MSAPI_UNIT_TEST_APPLICATION_INL
#define MSAPI_UNIT_TEST_APPLICATION_INL

#include "../../../../library/source/server/application.h"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Application()
{
	LOG_INFO_UNITTEST("MSAPI Application");
	MSAPI::Test t;

	MSAPI::Application app;

	RETURN_IF_FALSE(t.Assert(app.GetName().empty(), true, "application name is empty"));
	RETURN_IF_FALSE(t.Assert(app.GetState(), MSAPI::Application::State::Paused, "application state is Paused"));

	std::string parameters;
	app.GetParameters(parameters);
	RETURN_IF_FALSE(
		t.Assert(parameters, "Parameters:\n{\n\tName(2000001) const : \n\tApplication state(2000002) const : Paused\n}",
			"application parameters are correct"));
	app.SetName("TestApp");
	RETURN_IF_FALSE(t.Assert(app.GetName(), "TestApp", "application name is TestApp"));
	app.SetState(MSAPI::Application::State::Running);
	RETURN_IF_FALSE(t.Assert(app.GetState(), MSAPI::Application::State::Running, "application state is Running"));
	app.GetParameters(parameters);
	RETURN_IF_FALSE(t.Assert(parameters,
		"Parameters:\n{\n\tName(2000001) const : TestApp\n\tApplication state(2000002) const : Running\n}",
		"application parameters are correct after state change"));

	/**************************
	 * @param parametersSize Expected size of parameters without default Application parameters.
	 */
	const auto check{ [&t] [[nodiscard]] (const size_t checkIndex, const MSAPI::Application& app,
						  const std::string_view expectedParameters, const size_t parametersSize,
						  const std::map<size_t, std::string>& errors) {
		std::string parameters;
		app.GetParameters(parameters);
		RETURN_IF_FALSE(
			t.Assert(parameters, expectedParameters, "check[" + _S(checkIndex) + "] parameters are correct"));
		// -2 for default parameters
		const auto size{ app.GetParameters().size() + app.GetConstParameters().size() - 2 };
		RETURN_IF_FALSE(t.Assert(size, parametersSize, "check[" + _S(checkIndex) + "] parameters size is correct"));

		if (errors.empty()) {
			RETURN_IF_FALSE(
				t.Assert(app.AreParametersValid(), true, "check[" + _S(checkIndex) + "] parameters are valid"));
			RETURN_IF_FALSE(
				t.Assert(app.GetErrorParameters().empty(), true, "check[" + _S(checkIndex) + "] errors are empty"));
		}
		else {
			RETURN_IF_FALSE(
				t.Assert(app.AreParametersValid(), false, "check[" + _S(checkIndex) + "] parameters are not valid"));
			const auto& errorsMap = app.GetErrorParameters();
			RETURN_IF_FALSE(
				t.Assert(errorsMap.size(), errors.size(), "check[" + _S(checkIndex) + "] errors size is correct"));
			for (const auto& [id, parameter] : errorsMap) {
				const auto it{ errors.find(id) };
				RETURN_IF_FALSE(t.Assert(it != errors.end(), true,
					"check[" + _S(checkIndex) + "] error found for " + parameter->m_name + "(" + _S(id) + ")"));
				RETURN_IF_FALSE(t.Assert(parameter->m_error, it->second,
					"check[" + _S(checkIndex) + "] error value for " + parameter->m_name + "(" + _S(id)
						+ ") is correct"));
			}
		}

		return true;
	} };

#define POEF(v) std::string(v.has_value() ? (*f)(v.value()) : "")
#define PE(v) _S(static_cast<const UnderlyingType*>(static_cast<const void*>(&v)))
#define ASE(v)                                                                                                         \
	std::format("\n\tName(2000001) const : TestApp\n\tApplication state(2000002) const : {}\n}}",                      \
		MSAPI::Application::EnumToString(v))

	/**************************
	 * @param v1 First registered value. Should not be a temporal l-value.
	 * @param v2 Update for v1 by merge.
	 * @param v3 Update for v1 by direct assignment.
	 * @param v4 First registered value with min and max, should be equal to min. It should be safely to reduce it by 1.
	 * Should not be a temporal l-value.
	 * @param v5 First registered value with min and max, should be equal to max. It should be safely to increase it
	 * by 1. Should not be a temporal l-value.
	 */
	const auto checkNotEmpty{ [&check]<typename T> [[nodiscard]] (T & v1, const T& v2, const T& v3, T& v4, T& v5,
								  const MSAPI::Application::State state,
								  std::string_view (*const f)(remove_optional_t<T>) = nullptr) {
		/*
			int8_t = int8_t
			Enum : int8_t = int8_t
			std::optional<int8_t> = std::optional<int8_t>
		*/
		using UnderlyingType = std::conditional_t<is_optional_v<T>, T,
			std::conditional_t<std::is_enum_v<T>, safe_underlying_type_t<T>, T>>;

		/*
			int8_t = int8_t
			Enum : int8_t = int8_t
			std::optional<int8_t> = int8_t
		*/
		using UnderlyingPrimitiveType
			= std::conditional_t<std::is_enum_v<T>, safe_underlying_type_t<T>, remove_optional_t<T>>;

		MSAPI::Application app;
		app.SetName("TestApp");
		app.SetState(state);

		const T min{ v4 };
		const T max{ v5 };

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(1, { "Some name", &v1, Timer::Duration::Type::Days });
			app.RegisterParameter(1, { "Some name", &v1, Timer::Duration::Type::Days });
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterParameter(1, { "Some name", &v1, nullptr, true });
				app.RegisterParameter(1, { "Some name", &v1 });
			}
			else {
				app.RegisterParameter(1, { "Some name", &v1, f, true });
				app.RegisterParameter(1, { "Some name", &v1, f });
			}
		}
		else {
			app.RegisterParameter(1, { "Some name", &v1 });
			app.RegisterParameter(1, { "Some name", &v1 });
		}
		std::string expectedParameters;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + v1.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters
					= "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v1)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v1)) + ASE(state);
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v1) + ASE(state);
		}
		RETURN_IF_FALSE(check(0, app, expectedParameters, 1, {}));

		T tmp{ v2 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 1, static_cast<UnderlyingType>(tmp) } });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + tmp.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 1, static_cast<UnderlyingType>(tmp) } });
			if (f == nullptr) {
				expectedParameters
					= "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(tmp)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(tmp)) + ASE(state);
			}
		}
		else {
			app.MergeParameters({ { 1, static_cast<UnderlyingType>(tmp) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(tmp) + ASE(state);
		}
		RETURN_IF_FALSE(check(1, app, expectedParameters, 1, {}));

		v1 = v3;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters
					= "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + ASE(state);
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + ASE(state);
		}
		RETURN_IF_FALSE(check(2, app, expectedParameters, 1, {}));

		T tmp2;
		std::map<size_t, std::string> expectedErrors;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			tmp2 = min - Timer::Duration::CreateDays(1);
			app.RegisterParameter(2, { "Some name", &tmp2, Timer::Duration::Type::Days, min });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
				"Parameter Some name(2) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days) + " < "
					+ min.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			tmp2 = static_cast<T>(static_cast<UnderlyingType>(min) - 1);
			if (f == nullptr) {
				app.RegisterParameter(2, { "Some name", &tmp2 });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2)) + ASE(state);
				expectedErrors = { { 2,
					"Parameter Some name(2) is less than min value: " + _S(static_cast<UnderlyingType>(tmp2)) + " < "
						+ _S(static_cast<UnderlyingType>(min) + 1) } };
			}
			else {
				app.RegisterParameter(2, { "Some name", &tmp2, f });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + ASE(state);
				expectedErrors = { { 2,
					"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
						+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) } };
			}
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			tmp2 = min - 1;
			app.RegisterParameter(2, { "Some name", &tmp2, min });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			tmp2 = min.value() - 1;
			app.RegisterParameter(2, { "Some name", &tmp2, min });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}
		RETURN_IF_FALSE(check(3, app, expectedParameters, 2, expectedErrors));

		T tmp3;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			tmp3 = max + Timer::Duration::CreateDays(1);
			app.RegisterParameter(3, { "Some name", &tmp3, Timer::Duration::Type::Days, {}, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			tmp3 = static_cast<T>(static_cast<UnderlyingType>(max) + 1);
			if (f == nullptr) {
				app.RegisterParameter(3, { "Some name", &tmp3 });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				app.RegisterParameter(3, { "Some name", &tmp3, f });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			tmp3 = max + 1;
			app.RegisterParameter(3, { "Some name", &tmp3, {}, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			tmp3 = max.value() + 1;
			app.RegisterParameter(3, { "Some name", &tmp3, {}, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(4, app, expectedParameters, 3, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(4, { "Some name", &v4, Timer::Duration::Type::Days, min, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + v4.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterParameter(4, { "Some name", &v4, nullptr, true });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(v4)) + ASE(state);
			}
			else {
				app.RegisterParameter(4, { "Some name", &v4, f, true });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(v4)) + ASE(state);
			}
		}
		else {
			app.RegisterParameter(4, { "Some name", &v4, min, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(v4) + ASE(state);
		}
		RETURN_IF_FALSE(check(5, app, expectedParameters, 4, expectedErrors));

		T save4{ v4 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp2) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + tmp2.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) },
				{ 4,
					"Parameter Some name(4) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days)
						+ " < " + min.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp2) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(tmp2)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) },
					{ 4,
						"Parameter Some name(4) is less than min value: " + _S(static_cast<UnderlyingType>(tmp2))
							+ " < " + _S(static_cast<UnderlyingType>(min)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(tmp2)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) },
						  { 4,
							  "Parameter Some name(4) is less than min value: " + std::string((*f)(tmp2)) + " < "
								  + std::string((*f)(min)) } };
			}
		}
		else {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp2) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) },
				{ 4, "Parameter Some name(4) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}
		RETURN_IF_FALSE(check(6, app, expectedParameters, 4, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp3) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + tmp3.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) },
				{ 4,
					"Parameter Some name(4) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp3) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(tmp3)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) },
					{ 4,
						"Parameter Some name(4) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(tmp3)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) },
						  { 4,
							  "Parameter Some name(4) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp3) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(tmp3) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) },
				{ 4, "Parameter Some name(4) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(7, app, expectedParameters, 4, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(save4) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(save4) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(save4) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(8, app, expectedParameters, 4, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(5, { "Some name", &v5, Timer::Duration::Type::Days, min, max });
			app.RegisterParameter(5, { "Some name", &v5, Timer::Duration::Type::Days, min, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + v5.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterParameter(5, { "Some name", &v5 });
				app.RegisterParameter(5, { "Some name", &v5 });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(v5)) + ASE(state);
			}
			else {
				app.RegisterParameter(5, { "Some name", &v5, f });
				app.RegisterParameter(5, { "Some name", &v5, f });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + "\n\tSome name(4) : "
					+ std::string((*f)(save4)) + "\n\tSome name(5) : " + std::string((*f)(v5)) + ASE(state);
			}
		}
		else {
			app.RegisterParameter(5, { "Some name", &v5, min, max });
			app.RegisterParameter(5, { "Some name", &v5, min, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4) + "\n\tSome name(5) : " + _S(v5)
				+ ASE(state);
		}
		RETURN_IF_FALSE(check(9, app, expectedParameters, 5, expectedErrors));

		T save5{ v5 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp3) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + tmp3.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) },
				{ 5,
					"Parameter Some name(5) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp3) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(tmp3)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) },
					{ 5,
						"Parameter Some name(5) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + "\n\tSome name(4) : "
					+ std::string((*f)(save4)) + "\n\tSome name(5) : " + std::string((*f)(tmp3)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) },
						  { 5,
							  "Parameter Some name(5) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp3) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(tmp3) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) },
				{ 5, "Parameter Some name(5) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(10, app, expectedParameters, 5, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp2) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + tmp2.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) },
				{ 5,
					"Parameter Some name(5) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days)
						+ " < " + min.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp2) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(tmp2)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) },
					{ 5,
						"Parameter Some name(5) is less than min value: " + _S(static_cast<UnderlyingType>(tmp2))
							+ " < " + _S(static_cast<UnderlyingType>(min) + 1) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + "\n\tSome name(4) : "
					+ std::string((*f)(save4)) + "\n\tSome name(5) : " + std::string((*f)(tmp2)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) },
						  { 5,
							  "Parameter Some name(5) is less than min value: " + std::string((*f)(tmp2)) + " < "
								  + std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) } };
			}
		}
		else {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp2) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) },
				{ 5, "Parameter Some name(5) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}
		RETURN_IF_FALSE(check(11, app, expectedParameters, 5, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(save5) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(save5) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + "\n\tSome name(4) : "
					+ std::string((*f)(save4)) + "\n\tSome name(5) : " + std::string((*f)(save5)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(save5) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(12, app, expectedParameters, 5, expectedErrors));

		const T save6{ v1 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterConstParameter(6, { "Some name", &save6, Timer::Duration::Type::Days });
			app.RegisterConstParameter(6, { "Some name", &save6, Timer::Duration::Type::Days });
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterConstParameter(6, { "Some name", &save6 });
				app.RegisterConstParameter(6, { "Some name", &save6 });
			}
			else {
				app.RegisterConstParameter(6, { "Some name", &save6, f });
				app.RegisterConstParameter(6, { "Some name", &save6, f });
			}
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			app.RegisterConstParameter(6, { "Some name", &save6 });
			app.RegisterConstParameter(6, { "Some name", &save6 });
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			app.RegisterConstParameter(6, { "Some name", &save6.value() });
			app.RegisterConstParameter(6, { "Some name", &save6.value() });
		}

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(6) const : " + save6.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5))
					+ "\n\tSome name(6) const : " + _S(static_cast<UnderlyingType>(save6)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + "\n\tSome name(5) : "
					+ std::string((*f)(save5)) + "\n\tSome name(6) const : " + std::string((*f)(save6)) + ASE(state);
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + "\n\tSome name(6) const : " + _S(save6) + ASE(state);
		}
		RETURN_IF_FALSE(check(13, app, expectedParameters, 6, expectedErrors));

		app.MergeParameters({ { 6, static_cast<UnderlyingType>(tmp) } });
		RETURN_IF_FALSE(check(14, app, expectedParameters, 6, expectedErrors));

		T save7{ v1 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterConstParameter(7, { "Some name", &save7, Timer::Duration::Type::Days });
			app.RegisterConstParameter(7, { "Some name", &save7, Timer::Duration::Type::Days });
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterConstParameter(7, { "Some name", &save7 });
				app.RegisterConstParameter(7, { "Some name", &save7 });
			}
			else {
				app.RegisterConstParameter(7, { "Some name", &save7, f });
				app.RegisterConstParameter(7, { "Some name", &save7, f });
			}
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			app.RegisterConstParameter(7, { "Some name", &save7 });
			app.RegisterConstParameter(7, { "Some name", &save7 });
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			app.RegisterConstParameter(7, { "Some name", &save7.value() });
			app.RegisterConstParameter(7, { "Some name", &save7.value() });
		}

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(6) const : " + save6.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(7) const : " + save7.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5))
					+ "\n\tSome name(6) const : " + _S(static_cast<UnderlyingType>(save6))
					+ "\n\tSome name(7) const : " + _S(static_cast<UnderlyingType>(save7)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + "\n\tSome name(5) : "
					+ std::string((*f)(save5)) + "\n\tSome name(6) const : " + std::string((*f)(save6))
					+ "\n\tSome name(7) const : " + std::string((*f)(save7)) + ASE(state);
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + "\n\tSome name(6) const : " + _S(save6)
				+ "\n\tSome name(7) const : " + _S(save7) + ASE(state);
		}
		RETURN_IF_FALSE(check(15, app, expectedParameters, 7, expectedErrors));

		app.MergeParameters({ { 7, static_cast<UnderlyingType>(tmp) } });
		RETURN_IF_FALSE(check(16, app, expectedParameters, 7, expectedErrors));

		bool boolean{ false };
		app.MergeParameters({ { 1, boolean } });
		app.MergeParameters({ { 2, boolean } });
		app.MergeParameters({ { 3, boolean } });
		app.MergeParameters({ { 4, boolean } });
		app.MergeParameters({ { 5, boolean } });
		app.MergeParameters({ { 6, boolean } });
		app.MergeParameters({ { 7, boolean } });
		RETURN_IF_FALSE(check(17, app, expectedParameters, 7, expectedErrors));

		app.MergeParameters({ { 1, boolean }, { 2, boolean }, { 3, boolean }, { 4, boolean }, { 5, boolean },
			{ 6, boolean }, { 7, boolean } });
		RETURN_IF_FALSE(check(18, app, expectedParameters, 7, expectedErrors));

		app.SetCustomError(1, "Some custom error 1");
		app.SetCustomError(2, "Some custom error 2");
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedErrors = { { 1, "Parameter Some name(1) custom error: Some custom error 1" },
				{ 2,
					"Parameter Some name(2) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days)
						+ " < " + min.ToString(Timer::Duration::Type::Days) + ". Custom error: Some custom error 2" },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedErrors = { { 1, "Parameter Some name(1) custom error: Some custom error 1" },
					{ 2,
						"Parameter Some name(2) is less than min value: " + _S(static_cast<UnderlyingType>(tmp2))
							+ " < " + _S(static_cast<UnderlyingType>(min) + 1)
							+ ". Custom error: Some custom error 2" },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedErrors = { { 1, "Parameter Some name(1) custom error: Some custom error 1" },
					{ 2,
						"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
							+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1)))
							+ ". Custom error: Some custom error 2" },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
							+ std::string((*f)(max)) } };
			}
		}
		else {
			expectedErrors = { { 1, "Parameter Some name(1) custom error: Some custom error 1" },
				{ 2,
					"Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min)
						+ ". Custom error: Some custom error 2" },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(19, app, expectedParameters, 7, expectedErrors));

		app.MergeParameters({ { 1, static_cast<UnderlyingType>(save4) } });
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(6) const : " + save6.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(7) const : " + save7.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = {
				{ 2,
					"Parameter Some name(2) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days)
						+ " < " + min.ToString(Timer::Duration::Type::Days) + ". Custom error: Some custom error 2" },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) }
			};
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5))
					+ "\n\tSome name(6) const : " + _S(static_cast<UnderlyingType>(save6))
					+ "\n\tSome name(7) const : " + _S(static_cast<UnderlyingType>(save7)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1)
										   + ". Custom error: Some custom error 2" },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(save4))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + "\n\tSome name(5) : "
					+ std::string((*f)(save5)) + "\n\tSome name(6) const : " + std::string((*f)(save6))
					+ "\n\tSome name(7) const : " + std::string((*f)(save7)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1)))
								+ ". Custom error: Some custom error 2" },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(save4) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + "\n\tSome name(6) const : " + _S(save6)
				+ "\n\tSome name(7) const : " + _S(save7) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min)
									   + ". Custom error: Some custom error 2" },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(20, app, expectedParameters, 7, expectedErrors));

		T save8;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			save8 = tmp2 - Timer::Duration::CreateDays(1);
		}
		else if constexpr (std::is_enum_v<T>) {
			save8 = static_cast<T>(static_cast<UnderlyingType>(tmp2) - 1);
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			save8 = tmp2 - 1;
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			save8 = tmp2.value() - 1;
		}
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + save8.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(6) const : " + save6.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(7) const : " + save7.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + save8.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(save8))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5))
					+ "\n\tSome name(6) const : " + _S(static_cast<UnderlyingType>(save6))
					+ "\n\tSome name(7) const : " + _S(static_cast<UnderlyingType>(save7)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(save8)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(save4))
					+ "\n\tSome name(2) : " + std::string((*f)(save8)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + "\n\tSome name(5) : "
					+ std::string((*f)(save5)) + "\n\tSome name(6) const : " + std::string((*f)(save6))
					+ "\n\tSome name(7) const : " + std::string((*f)(save7)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(save8)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(save4) + "\n\tSome name(2) : " + _S(save8)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + "\n\tSome name(6) const : " + _S(save6)
				+ "\n\tSome name(7) const : " + _S(save7) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(save8) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(21, app, expectedParameters, 7, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
		}
		else {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
		}
		RETURN_IF_FALSE(check(22, app, expectedParameters, 7, expectedErrors));

		return true;
	} };

	{
		int8_t param1{ -3 };
		int8_t param4{ -23 };
		int8_t param5{ 23 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, int8_t{ 11 }, int8_t{ 0 }, param4, param5, MSAPI::Application::State::Undefined));
	}

	{
		int16_t param1{ -3331 };
		int16_t param4{ -21981 };
		int16_t param5{ 31938 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, int16_t{ 5983 }, int16_t{ 0 }, param4, param5, MSAPI::Application::State::Paused));
	}

	{
		int32_t param1{ -3331 };
		int32_t param4{ -93981 };
		int32_t param5{ 105938 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, int32_t{ 5983 }, int32_t{ 0 }, param4, param5, MSAPI::Application::State::Running));
	}

	{
		int64_t param1{ -3331 };
		int64_t param4{ -93981 };
		int64_t param5{ 105938 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, int64_t{ 5983 }, int64_t{ 0 }, param4, param5, MSAPI::Application::State::Running));
	}

	{
		uint8_t param1{ 10 };
		uint8_t param4{ 20 };
		uint8_t param5{ 30 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, uint8_t{ 5 }, uint8_t{ 8 }, param4, param5, MSAPI::Application::State::Paused));
	}

	{
		uint16_t param1{ 10 };
		uint16_t param4{ 3981 };
		uint16_t param5{ 22938 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, uint16_t{ 5 }, uint16_t{ 8 }, param4, param5, MSAPI::Application::State::Undefined));
	}

	{
		uint32_t param1{ 10 };
		uint32_t param4{ 3981 };
		uint32_t param5{ 55938 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, uint32_t{ 5 }, uint32_t{ 8 }, param4, param5, MSAPI::Application::State::Paused));
	}

	{
		uint64_t param1{ 10 };
		uint64_t param4{ 3981 };
		uint64_t param5{ 55938 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, uint64_t{ 5 }, uint64_t{ 8 }, param4, param5, MSAPI::Application::State::Running));
	}

	{
		double param1{ 0.00067112 };
		double param4{ -93981.42804648 };
		double param5{ 105938.84936204 };
		RETURN_IF_FALSE(checkNotEmpty(param1, double{ 5983.647394875 }, double{ -0.7400006701 }, param4, param5,
			MSAPI::Application::State::Running));
	}

	{
		float param1{ 0.00067112f };
		float param4{ -93981.42804648f };
		float param5{ 105938.84936204f };
		RETURN_IF_FALSE(checkNotEmpty(param1, float{ 5983.647394875f }, float{ -0.7400006701f }, param4, param5,
			MSAPI::Application::State::Paused));
	}

	{
		MSAPI::Application app;
		app.SetName("TestApp");
		app.SetState(MSAPI::Application::State::Running);

		bool param1{ false };
		app.RegisterParameter(1, { "Some name", &param1 });
		app.RegisterParameter(1, { "Some name", &param1 });
		std::string expectedParameters{ "Parameters:\n{\n\tSome name(1) : false" + ASE(app.GetState()) };
		RETURN_IF_FALSE(check(0, app, expectedParameters, 1, {}));

		bool param2{ true };
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(check(1, app, "Parameters:\n{\n\tSome name(1) : true" + ASE(app.GetState()), 1, {}));

		param1 = false;
		RETURN_IF_FALSE(check(2, app, expectedParameters, 1, {}));

		app.RegisterConstParameter(2, { "Some name", &param1 });
		app.RegisterConstParameter(2, { "Some name", &param1 });
		expectedParameters
			= "Parameters:\n{\n\tSome name(1) : false\n\tSome name(2) const : false" + ASE(app.GetState());
		RETURN_IF_FALSE(check(3, app, expectedParameters, 2, {}));

		const bool param3{ true };
		app.MergeParameters({ { 2, param3 } });
		RETURN_IF_FALSE(check(4, app, expectedParameters, 2, {}));

		app.RegisterConstParameter(3, { "Some name", &param3 });
		app.RegisterConstParameter(3, { "Some name", &param3 });
		expectedParameters
			= "Parameters:\n{\n\tSome name(1) : false\n\tSome name(2) const : false\n\tSome name(3) const : true"
			+ ASE(app.GetState());
		RETURN_IF_FALSE(check(5, app, expectedParameters, 3, {}));

		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(6, app, expectedParameters, 3, {}));

		int param4{ 0 };
		app.MergeParameters({ { 1, param4 } });
		app.MergeParameters({ { 2, param4 } });
		app.MergeParameters({ { 3, param4 } });
		RETURN_IF_FALSE(check(7, app, expectedParameters, 3, {}));

		app.MergeParameters({ { 1, param4 }, { 2, param4 }, { 3, param4 } });
		RETURN_IF_FALSE(check(8, app, expectedParameters, 3, {}));
	}

	/**************************
	 * @param v1 First registered value. Should not be a temporal l-value.
	 * @param v2 Update for v1 by merge.
	 * @param v3 Update for v1 by direct assignment.
	 * @param v4 First registered value with min and max, should be equal to min. It should be safely to reduce it by 1.
	 * Should not be a temporal l-value.
	 * @param v5 First registered value with min and max, should be equal to max. It should be safely to increase it
	 * by 1. Should not be a temporal l-value.
	 */
	const auto checkNumericOptionalParameter{ [&check, &checkNotEmpty]<typename T> [[nodiscard]] (T & v1, const T& v2,
												  const T& v3, T& v4, T& v5, const MSAPI::Application::State state,
												  std::string_view (*const f)(remove_optional_t<T>) = nullptr) {
		RETURN_IF_FALSE(checkNotEmpty(v1, v2, v3, v4, v5, state, f));

		MSAPI::Application app;
		app.SetName("TestApp");
		app.SetState(state);

		const std::optional<remove_optional_t<T>> min{ v4 };
		const std::optional<remove_optional_t<T>> max{ v5 };

		std::string expectedParameters;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(1, { "Some name", &v1, Timer::Duration::Type::Days });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + v1.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			app.RegisterParameter(1, { "Some name", &v1 });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v1) + ASE(state);
		}
		RETURN_IF_FALSE(check(0, app, expectedParameters, 1, {}));

		T tmp = {};
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 1, tmp } });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + tmp.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			app.MergeParameters({ { 1, tmp } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(tmp) + ASE(state);
		}
		RETURN_IF_FALSE(check(1, app, expectedParameters, 1, { { 1, "Parameter Some name(1) is empty" } }));

		app.MergeParameters({ { 1, tmp } });
		RETURN_IF_FALSE(check(2, app, expectedParameters, 1, { { 1, "Parameter Some name(1) is empty" } }));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 1, v4 } });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			app.MergeParameters({ { 1, v4 } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v4) + ASE(state);
		}
		RETURN_IF_FALSE(check(3, app, expectedParameters, 1, {}));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(2, { "Some name", &tmp, Timer::Duration::Type::Days, min, max, true });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			app.RegisterParameter(2, { "Some name", &tmp, min, max, true });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v4) + "\n\tSome name(2) : " + ASE(state);
		}
		RETURN_IF_FALSE(check(4, app, expectedParameters, 2, {}));

		T tmp2;
		std::map<size_t, std::string> expectedErrors;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			tmp2 = min.value() - Timer::Duration::CreateDays(1);
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
				"Parameter Some name(2) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days) + " < "
					+ min.value().ToString(Timer::Duration::Type::Days) } };
		}
		else {
			tmp2 = min.value() - 1;
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v4) + "\n\tSome name(2) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}

		app.MergeParameters({ { 2, tmp2 } });
		RETURN_IF_FALSE(check(5, app, expectedParameters, 2, expectedErrors));

		T tmp3;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			tmp3 = max.value() + Timer::Duration::CreateDays(1);
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp3.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
				"Parameter Some name(2) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
					+ " > " + max.value().ToString(Timer::Duration::Type::Days) } };
		}
		else {
			tmp3 = max.value() + 1;
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v4) + "\n\tSome name(2) : " + _S(tmp3) + ASE(state);
			expectedErrors
				= { { 2, "Parameter Some name(2) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}

		app.MergeParameters({ { 2, tmp3 } });
		RETURN_IF_FALSE(check(6, app, expectedParameters, 2, expectedErrors));

		app.MergeParameters({ { 2, v5 } });
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + v5.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v4) + "\n\tSome name(2) : " + _S(v5) + ASE(state);
		}
		RETURN_IF_FALSE(check(7, app, expectedParameters, 2, {}));

		return true;
	} };

#undef POEF
#undef PE

	{
		struct Test {
			enum class TestEnum : int16_t {
				Undefined,
				Zero,
				One,
				Two,
				Three,
				Four,
				Five,
				Six,
				Seven,
				Eight,
				Nine,
				Max
			};

			[[nodiscard]] static std::string_view EnumToString(const TestEnum value)
			{
				switch (value) {
				case TestEnum::Undefined:
					return "Undefined";
				case TestEnum::Zero:
					return "Zero";
				case TestEnum::One:
					return "One";
				case TestEnum::Two:
					return "Two";
				case TestEnum::Three:
					return "Three";
				case TestEnum::Four:
					return "Four";
				case TestEnum::Five:
					return "Five";
				case TestEnum::Six:
					return "Six";
				case TestEnum::Seven:
					return "Seven";
				case TestEnum::Eight:
					return "Eight";
				case TestEnum::Nine:
					return "Nine";
				case TestEnum::Max:
					return "Max";
				default:
					return "Unknown";
				}
			}
		};

		{
			Test::TestEnum param1{ Test::TestEnum::Four };
			//* Max is always auto generated, but parameter should be passed. Min can be calculated as Undefined or
			//* Undefined + 1
			Test::TestEnum param4{ Test::TestEnum::Undefined };
			Test::TestEnum param5{ Test::TestEnum::Max };
			RETURN_IF_FALSE(checkNotEmpty(param1, Test::TestEnum::Eight, Test::TestEnum::Two, param4, param5,
				MSAPI::Application::State::Undefined, Test::EnumToString));

			RETURN_IF_FALSE(checkNotEmpty(param1, Test::TestEnum::Eight, Test::TestEnum::Two, param4, param5,
				MSAPI::Application::State::Running));
		}
	}

	{
		std::optional<int8_t> param1{ -3 };
		std::optional<int8_t> param4{ -23 };
		std::optional<int8_t> param5{ 23 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<int8_t>{ 11 }, std::optional<int8_t>{ 0 },
			param4, param5, MSAPI::Application::State::Undefined));
	}

	{
		std::optional<int16_t> param1{ -3331 };
		std::optional<int16_t> param4{ -21981 };
		std::optional<int16_t> param5{ 31938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<int16_t>{ 5983 },
			std::optional<int16_t>{ 0 }, param4, param5, MSAPI::Application::State::Paused));
	}

	{
		std::optional<int32_t> param1{ -3331 };
		std::optional<int32_t> param4{ -93981 };
		std::optional<int32_t> param5{ 105938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<int32_t>{ 5983 },
			std::optional<int32_t>{ 0 }, param4, param5, MSAPI::Application::State::Running));
	}

	{
		std::optional<int64_t> param1{ -3331 };
		std::optional<int64_t> param4{ -93981 };
		std::optional<int64_t> param5{ 105938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<int64_t>{ 5983 },
			std::optional<int64_t>{ 0 }, param4, param5, MSAPI::Application::State::Running));
	}

	{
		std::optional<uint8_t> param1{ 10 };
		std::optional<uint8_t> param4{ 20 };
		std::optional<uint8_t> param5{ 30 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<uint8_t>{ 5 }, std::optional<uint8_t>{ 8 },
			param4, param5, MSAPI::Application::State::Paused));
	}

	{
		std::optional<uint16_t> param1{ 10 };
		std::optional<uint16_t> param4{ 3981 };
		std::optional<uint16_t> param5{ 22938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<uint16_t>{ 5 },
			std::optional<uint16_t>{ 8 }, param4, param5, MSAPI::Application::State::Undefined));
	}

	{
		std::optional<uint32_t> param1{ 10 };
		std::optional<uint32_t> param4{ 3981 };
		std::optional<uint32_t> param5{ 55938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<uint32_t>{ 5 },
			std::optional<uint32_t>{ 8 }, param4, param5, MSAPI::Application::State::Paused));
	}

	{
		std::optional<uint64_t> param1{ 10 };
		std::optional<uint64_t> param4{ 3981 };
		std::optional<uint64_t> param5{ 55938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<uint64_t>{ 5 },
			std::optional<uint64_t>{ 8 }, param4, param5, MSAPI::Application::State::Running));
	}

	{
		std::optional<double> param1{ 0.00067112 };
		std::optional<double> param4{ -93981.42804648 };
		std::optional<double> param5{ 105938.84936204 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<double>{ 5983.647394875 },
			std::optional<double>{ -0.7400006701 }, param4, param5, MSAPI::Application::State::Running));
	}

	{
		std::optional<float> param1{ 0.00067112f };
		std::optional<float> param4{ -93981.42804648f };
		std::optional<float> param5{ 105938.84936204f };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<float>{ 5983.647394875f },
			std::optional<float>{ -0.7400006701f }, param4, param5, MSAPI::Application::State::Paused));
	}

	{
		Timer::Duration param1{ Timer::Duration::Create(11, 5, 30, 30, 8946) };
		Timer::Duration param4{ Timer::Duration::Create(10, 5, 30, 30, 8946) };
		Timer::Duration param5{ Timer::Duration::Create(12, 5, 30, 30, 8946) };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, Timer::Duration::Create(11, 6, 45, 0, 8946),
			Timer::Duration::Create(12, 5, 30, 30, 8947), param4, param5, MSAPI::Application::State::Paused));
	}

	{
		MSAPI::Application app;
		app.SetName("TestApp");
		app.SetState(MSAPI::Application::State::Running);

		std::string param1{ "Some string parameter" };
		app.RegisterParameter(1, { "Some name", &param1 });
		app.RegisterParameter(1, { "Some name", &param1 });
		RETURN_IF_FALSE(
			check(0, app, "Parameters:\n{\n\tSome name(1) : Some string parameter" + ASE(app.GetState()), 1, {}));

		std::string param2{ "Another string parameter" };
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(
			check(1, app, "Parameters:\n{\n\tSome name(1) : Another string parameter" + ASE(app.GetState()), 1, {}));

		param1 = "Third version of parameter";
		RETURN_IF_FALSE(
			check(2, app, "Parameters:\n{\n\tSome name(1) : Third version of parameter" + ASE(app.GetState()), 1, {}));

		param2 = "";
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(check(3, app, "Parameters:\n{\n\tSome name(1) : " + ASE(app.GetState()), 1,
			{ { 1, "Parameter Some name(1) is empty" } }));

		std::string param3;
		app.RegisterParameter(2, { "Some name", &param3 });
		RETURN_IF_FALSE(check(4, app, "Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : " + ASE(app.GetState()), 2,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		std::string param4;
		app.RegisterParameter(3, { "Some name", &param4, true });
		app.RegisterParameter(3, { "Some name", &param4, true });
		RETURN_IF_FALSE(check(5, app,
			"Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : " + ASE(app.GetState()), 3,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		param2 = "Another string parameter";
		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(6, app,
			"Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : Another string "
			"parameter"
				+ ASE(app.GetState()),
			3, { { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		param2 = "";
		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(7, app,
			"Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : " + ASE(app.GetState()), 3,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		app.RegisterConstParameter(4, { "Some name", &param2 });
		app.RegisterConstParameter(4, { "Some name", &param2 });
		std::string expectedParameters{
			"Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : \n\tSome name(4) const : "
			+ ASE(app.GetState())
		};
		RETURN_IF_FALSE(check(8, app, expectedParameters, 4,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		std::string tmp{ "Some string parameter" };
		app.MergeParameters({ { 4, tmp } });
		RETURN_IF_FALSE(check(9, app, expectedParameters, 4,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		const std::string param5{ "123" };
		app.RegisterConstParameter(5, { "Some name", &param5 });
		app.RegisterConstParameter(5, { "Some name", &param5 });
		expectedParameters = "Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : \n\tSome "
							 "name(4) const : \n\tSome name(5) const : 123"
			+ ASE(app.GetState());
		RETURN_IF_FALSE(check(10, app, expectedParameters, 5,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		app.MergeParameters({ { 5, tmp } });
		RETURN_IF_FALSE(check(11, app, expectedParameters, 5,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		bool boolean{ false };
		app.MergeParameters({ { 1, boolean } });
		app.MergeParameters({ { 2, boolean } });
		app.MergeParameters({ { 3, boolean } });
		app.MergeParameters({ { 4, boolean } });
		app.MergeParameters({ { 5, boolean } });
		RETURN_IF_FALSE(check(12, app, expectedParameters, 5,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		app.MergeParameters({ { 1, boolean }, { 2, boolean }, { 3, boolean }, { 4, boolean }, { 5, boolean } });
		RETURN_IF_FALSE(check(13, app, expectedParameters, 5,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));
	}

	{
		MSAPI::Application app;
		app.SetName("TestApp");
		app.SetState(MSAPI::Application::State::Running);

		Timer param1{ Timer::Create(2023, 11, 5, 23, 30, 1, 8946) };
		app.RegisterParameter(1, { "Some name", &param1 });
		app.RegisterParameter(1, { "Some name", &param1 });
		RETURN_IF_FALSE(
			check(0, app, "Parameters:\n{\n\tSome name(1) : " + param1.ToString() + ASE(app.GetState()), 1, {}));

		Timer param2{ Timer::Create(1973, 11, 5, 0, 30, 1, 8946) };
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(
			check(1, app, "Parameters:\n{\n\tSome name(1) : " + param2.ToString() + ASE(app.GetState()), 1, {}));

		param1 = Timer::Create(2025, 11, 5, 23, 30, 1, 8946);
		RETURN_IF_FALSE(
			check(2, app, "Parameters:\n{\n\tSome name(1) : " + param1.ToString() + ASE(app.GetState()), 1, {}));

		param2 = Timer{ 0 };
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(check(3, app, "Parameters:\n{\n\tSome name(1) : " + param1.ToString() + ASE(app.GetState()), 1,
			{ { 1, "Parameter Some name(1) is empty" } }));

		Timer param3{ 0 };
		app.RegisterParameter(2, { "Some name", &param3 });
		app.RegisterParameter(2, { "Some name", &param3 });
		std::map<size_t, std::string> expectedErrors{ { 1, "Parameter Some name(1) is empty" },
			{ 2, "Parameter Some name(2) is empty" } };
		RETURN_IF_FALSE(check(4, app,
			"Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : " + param3.ToString()
				+ ASE(app.GetState()),
			2, expectedErrors));

		Timer param4{ 0 };
		app.RegisterParameter(3, { "Some name", &param4, true });
		RETURN_IF_FALSE(check(5, app,
			"Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : " + param3.ToString()
				+ "\n\tSome name(3) : " + param4.ToString() + ASE(app.GetState()),
			3, expectedErrors));

		param2 = Timer::Create(2025, 11, 5, 23, 30, 1, 8964);
		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(6, app,
			"Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : " + param3.ToString()
				+ "\n\tSome name(3) : " + param2.ToString() + ASE(app.GetState()),
			3, { { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		param2 = Timer{ 0 };
		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(7, app,
			"Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : " + param3.ToString()
				+ "\n\tSome name(3) : " + param2.ToString() + ASE(app.GetState()),
			3, { { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		app.RegisterConstParameter(4, { "Some name", &param2 });
		app.RegisterConstParameter(4, { "Some name", &param2 });
		std::string expectedParameters{ "Parameters:\n{\n\tSome name(1) : " + param1.ToString()
			+ "\n\tSome name(2) : " + param3.ToString() + "\n\tSome name(3) : " + param2.ToString()
			+ "\n\tSome name(4) const : " + param2.ToString() + ASE(app.GetState()) };
		RETURN_IF_FALSE(check(8, app, expectedParameters, 4, expectedErrors));

		app.MergeParameters({ { 4, param1 } });
		RETURN_IF_FALSE(check(9, app, expectedParameters, 4, expectedErrors));

		const Timer param5{ Timer::Create(2023, 11, 5, 23, 30, 1, 8946) };
		app.RegisterConstParameter(5, { "Some name", &param5 });
		app.RegisterConstParameter(5, { "Some name", &param5 });
		expectedParameters = "Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : "
			+ param3.ToString() + "\n\tSome name(3) : " + param2.ToString() + "\n\tSome name(4) const : "
			+ param2.ToString() + "\n\tSome name(5) const : " + param5.ToString() + ASE(app.GetState());
		RETURN_IF_FALSE(check(10, app, expectedParameters, 5, expectedErrors));

		app.MergeParameters({ { 5, param1 } });
		RETURN_IF_FALSE(check(11, app, expectedParameters, 5, expectedErrors));

		bool boolean{ false };
		app.MergeParameters({ { 1, boolean } });
		app.MergeParameters({ { 2, boolean } });
		app.MergeParameters({ { 3, boolean } });
		app.MergeParameters({ { 4, boolean } });
		app.MergeParameters({ { 5, boolean } });
		RETURN_IF_FALSE(check(12, app, expectedParameters, 5, expectedErrors));

		app.MergeParameters({ { 1, boolean }, { 2, boolean }, { 3, boolean }, { 4, boolean }, { 5, boolean } });
		RETURN_IF_FALSE(check(13, app, expectedParameters, 5, expectedErrors));
	}

	{
		MSAPI::Application app;
		app.SetName("TestApp");
		app.SetState(MSAPI::Application::State::Running);

		Table<std::optional<uint64_t>, Timer, std::string, Timer::Duration, double> table1{ 111111, 222222, 333333,
			444444, 555555 };

		app.RegisterParameter(1, { "Some name", &table1 });
		app.RegisterParameter(1, { "Some name", &table1 });
		RETURN_IF_FALSE(check(0, app, "Parameters:\n{\n\tSome name(1) : " + table1.ToString() + ASE(app.GetState()), 1,
			{ { 1, "Parameter Some name(1) is empty" } }));

		auto table2{ table1 };
		table2.AddRow(std::optional<uint64_t>{}, Timer::Create(2053, 1, 5, 23, 30, 1, 8946),
			std::string{ "Some string 1" }, Timer::Duration::CreateMinutes(123), 0.0);
		table2.AddRow(std::optional<uint64_t>{ 1 }, Timer::Create(2048, 11, 5, 23, 8, 1, 89461),
			std::string{ "Some string 2 2" }, Timer::Duration{}, 928347.74);
		table2.AddRow(std::optional<uint64_t>{}, Timer::Create(2099, 11, 5, 7, 30, 1, 894623),
			std::string{ "Some string 3 3 3" }, Timer::Duration::CreateSeconds(90), -0.00067112);
		table2.AddRow(std::optional<uint64_t>{ 3 }, Timer::Create(2085, 4, 5, 23, 30, 1, 8946456), std::string{},
			Timer::Duration::Create(11, 5, 30, 30, 8946), 0.00067112);
		table2.AddRow(std::optional<uint64_t>{}, Timer{ 0 }, std::string{ "p" },
			Timer::Duration::CreateMilliseconds(100), -30.00067112004);

		app.MergeParameters({ { 1, table2 } });
		RETURN_IF_FALSE(
			check(1, app, "Parameters:\n{\n\tSome name(1) : " + table2.ToString() + ASE(app.GetState()), 1, {}));

		table1.AddRow(std::optional<uint64_t>{}, Timer{ 0 }, std::string{ "p" },
			Timer::Duration::CreateMilliseconds(100), -30.00067112004);
		RETURN_IF_FALSE(
			check(2, app, "Parameters:\n{\n\tSome name(1) : " + table1.ToString() + ASE(app.GetState()), 1, {}));

		table2.Clear();
		app.MergeParameters({ { 1, table2 } });
		std::map<size_t, std::string> expectedErrors{ { 1, "Parameter Some name(1) is empty" } };
		RETURN_IF_FALSE(check(
			3, app, "Parameters:\n{\n\tSome name(1) : " + table2.ToString() + ASE(app.GetState()), 1, expectedErrors));

		Table<bool, bool, std::string, bool, std::string, std::string, std::string> table3{ 123, 456, 789, 101112,
			131415, 161718, 192021 };
		table3.AddRow(true, false, std::string{ "Some string 1" }, true, std::string{ "Some string 2" },
			std::string{ "Some string 3" }, std::string{ "Some string 4" });
		app.RegisterParameter(2, { "Some name", &table3 });
		RETURN_IF_FALSE(check(4, app,
			"Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : " + table3.ToString()
				+ ASE(app.GetState()),
			2, expectedErrors));

		Table<std::optional<float>, std::optional<float>, bool, bool> table4{ 11, 22, 33, 44 };
		app.RegisterParameter(3, { "Some name", &table4, true });
		app.RegisterParameter(3, { "Some name", &table4, true });
		RETURN_IF_FALSE(check(5, app,
			"Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : " + table3.ToString()
				+ "\n\tSome name(3) : " + table4.ToString() + ASE(app.GetState()),
			3, expectedErrors));

		auto table5{ table4 };
		table5.AddRow(std::optional<float>{ 1 }, std::optional<float>{ 2 }, true, false);
		table5.AddRow(std::optional<float>{ 3 }, std::optional<float>{ 4 }, false, true);
		app.MergeParameters({ { 3, table5 } });
		RETURN_IF_FALSE(check(6, app,
			"Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : " + table3.ToString()
				+ "\n\tSome name(3) : " + table5.ToString() + ASE(app.GetState()),
			3, expectedErrors));

		table5.Clear();
		app.MergeParameters({ { 3, table5 } });
		RETURN_IF_FALSE(check(7, app,
			"Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : " + table3.ToString()
				+ "\n\tSome name(3) : " + table5.ToString() + ASE(app.GetState()),
			3, expectedErrors));

		Table<std::string> table6{ 521 };
		app.RegisterConstParameter(4, { "Some name", &table6 });
		app.RegisterConstParameter(4, { "Some name", &table6 });
		std::string expectedParameters{ "Parameters:\n{\n\tSome name(1) : " + table1.ToString()
			+ "\n\tSome name(2) : " + table3.ToString() + "\n\tSome name(3) : " + table4.ToString()
			+ "\n\tSome name(4) const : " + table6.ToString() + ASE(app.GetState()) };
		RETURN_IF_FALSE(check(8, app, expectedParameters, 4, expectedErrors));

		auto table7{ table6 };
		table7.AddRow("Some string 1");
		table7.AddRow("Some string 2");
		table7.AddRow("Some string 3");
		app.MergeParameters({ { 4, table7 } });
		RETURN_IF_FALSE(check(9, app, expectedParameters, 4, expectedErrors));

		auto table8{ table6 };
		table8.AddRow("Some string 1111");
		table8.AddRow("Some string 2222");
		table8.AddRow("Some string 3333");
		app.RegisterConstParameter(5, { "Some name", &table8 });
		app.RegisterConstParameter(5, { "Some name", &table8 });
		expectedParameters = "Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : "
			+ table3.ToString() + "\n\tSome name(3) : " + table4.ToString() + "\n\tSome name(4) const : "
			+ table6.ToString() + "\n\tSome name(5) const : " + table8.ToString() + ASE(app.GetState());
		RETURN_IF_FALSE(check(10, app, expectedParameters, 5, expectedErrors));

		app.MergeParameters({ { 5, table7 } });
		RETURN_IF_FALSE(check(11, app, expectedParameters, 5, expectedErrors));

		bool boolean{ false };
		app.MergeParameters({ { 1, boolean } });
		app.MergeParameters({ { 2, boolean } });
		app.MergeParameters({ { 3, boolean } });
		app.MergeParameters({ { 4, boolean } });
		app.MergeParameters({ { 5, boolean } });
		RETURN_IF_FALSE(check(12, app, expectedParameters, 5, expectedErrors));

		app.MergeParameters({ { 1, boolean }, { 2, boolean }, { 3, boolean }, { 4, boolean }, { 5, boolean } });
		RETURN_IF_FALSE(check(13, app, expectedParameters, 5, expectedErrors));
	}

#undef ASE

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_APPLICATION_INL