/**************************
 * @file        json.inl
 * @version     6.0
 * @date        2025-11-20
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2026 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2026 Maksim Andreevich Leonov, maks.angels@mail.ru
 */

#ifndef MSAPI_UNIT_TEST_JSON_INL
#define MSAPI_UNIT_TEST_JSON_INL

#include "../../../../library/source/help/json.h"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Json.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Json();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool Json()
{
	LOG_INFO_UNITTEST("MSAPI Json");
	MSAPI::Test t;

	{
		MSAPI::Json json{ "" };
		RETURN_IF_FALSE(t.Assert(json.Valid(), false, "Empty json is invalid"));
		RETURN_IF_FALSE(t.Assert(json.GetKeysAndValues().empty(), true, "Empty json is empty"));
		RETURN_IF_FALSE(t.Assert(json.GetValue("key") == nullptr, true, "Key 'key' does not exist in empty json"));
		RETURN_IF_FALSE(
			t.Assert(json.ToString(), "Json:\n{} <valid: false>", "Empty json string interpretation is correct"));
		RETURN_IF_FALSE(t.Assert(json.ToJson(), "{}", "Empty json interpretation is correct"));

		MSAPI::Json json2{ "{}" };
		RETURN_IF_FALSE(t.Assert(json2.Valid(), true, "Empty json is valid"));
		RETURN_IF_FALSE(t.Assert(json2.GetKeysAndValues().empty(), true, "Empty json is empty"));
		RETURN_IF_FALSE(t.Assert(json2.GetValue("key") == nullptr, true, "Key 'key' does not exist in empty json"));
		RETURN_IF_FALSE(
			t.Assert(json2.ToString(), "Json:\n{} <valid: true>", "Empty json string interpretation is correct"));
		RETURN_IF_FALSE(t.Assert(json2.ToJson(), "{}", "Empty json interpretation is correct"));
		json2.Clear();
		RETURN_IF_FALSE(t.Assert(json2.Valid(), false, "Cleared json is invalid"));

		MSAPI::Json json3{ "[]" };
		RETURN_IF_FALSE(t.Assert(json3.Valid(), true, "Empty json array is valid"));
		RETURN_IF_FALSE(t.Assert(json3.GetKeysAndValues().size(), 1, "Empty json array has one key"));
		RETURN_IF_FALSE(t.Assert(json3.GetValue("key") == nullptr, true, "Key 'key' does not exist in empty json"));
		const auto* rootArray{ json3.GetValue("rootArray") };
		RETURN_IF_FALSE(t.Assert(rootArray != nullptr, true, "Key 'rootArray' found in json"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(rootArray->GetValue()), true,
			"rootArray is array type"));
		RETURN_IF_FALSE(t.Assert(rootArray->Valid(), true, "rootArray is valid"));
		RETURN_IF_FALSE(
			t.Assert(std::get<std::list<MSAPI::JsonNode>>(rootArray->GetValue()).empty(), true, "rootArray is empty"));
		RETURN_IF_FALSE(t.Assert(json3.ToString(), "Json:\n{\n\trootArray : [] <valid: true>\n} <valid: true>",
			"Json string interpretation is correct"));
		RETURN_IF_FALSE(t.Assert(json3.ToJson(), "{\"rootArray\":[]}", "Json interpretation is correct"));
		json3.Clear();
		RETURN_IF_FALSE(t.Assert(json3.Valid(), false, "Cleared json is invalid"));
		RETURN_IF_FALSE(t.Assert(json3.GetKeysAndValues().empty(), true, "Cleared json is empty"));

		MSAPI::Json empty;
		RETURN_IF_FALSE(t.Assert(empty.Valid(), false, "Empty json node is invalid"));
		RETURN_IF_FALSE(t.Assert(empty.GetKeysAndValues().empty(), true, "Empty json node is empty"));
	}

	{
		MSAPI::Json json(
			"{\n"
			"\t\"Apps\": [\n"
			"\t\t{\n"
			"\t\t\t\"App\": \"Gateway TBank\",\n"
			"\t\t\t\"Bin\": \"/home/flameskin/iwebyou/AT/apps/gateway/build/ATBotGateway\",\n"
			"\t\t\t\"Settings\": \"/home/flameskin/iwebyou/AT/apps/gateway/build/settings.json\"\n"
			"\t\t},\n"
			"\t\t{\n"
			"\t\t\t\"App\": \"Strategy\",\n"
			"\t\t\t\"Bin\": \"/home/flameskin/iwebyou/AT/apps/strategy/build/ATBotStrategy\",\n"
			"\t\t\t\"Settings\": \"/home/flameskin/iwebyou/AT/apps/strategy/build/settings.json\"\n"
			"\t\t},\n"
			"\t\t{\n"
			"\t\t\t\"App\": \"Storage\",\n"
			"\t\t\t\"Bin\": \"/home/flameskin/iwebyou/AT/apps/storage/build/ATBotStrategy\",\n"
			"\t\t\t\"Settings\": \"/home/flameskin/iwebyou/AT/apps/storage/build/settings.json\"\n"
			"\t\t},\n"
			"\t\t{\n"
			"\t\t\t\"App\": \"Strategy theory checker\",\n"
			"\t\t\t\"Bin\": \"/home/flameskin/iwebyou/AT/apps/strategyTheoryChecker/build/ATBotStrategy\",\n"
			"\t\t\t\"Settings\": \"/home/flameskin/iwebyou/AT/apps/strategyTheoryChecker/build/settings.json\"\n"
			"\t\t},\n"
			"\t\t{\n"
			"\t\t\t\"App\": \"Web panel\",\n"
			"\t\t\t\"Bin\": \"/home/flameskin/iwebyou/AT/apps/webPanel/build/ATBotStrategy\",\n"
			"\t\t\t\"Settings\": \"/home/flameskin/iwebyou/AT/apps/webPanel/build/settings.json\"\n"
			"\t\t}\n"
			"\t]\n"
			"}");

		if (const std::string_view expectedString{
				"Json:\n{\n\tApps : [\n\t\tJson:\n\t{\n\t\tApp      : Gateway TBank\n\t\tBin      : "
				"/home/flameskin/iwebyou/AT/apps/gateway/build/ATBotGateway\n\t\tSettings : "
				"/home/flameskin/iwebyou/AT/apps/gateway/build/settings.json\n\t} <valid: "
				"true>,\n\t\tJson:\n\t{\n\t\tApp      : Strategy\n\t\tBin      : "
				"/home/flameskin/iwebyou/AT/apps/strategy/build/ATBotStrategy\n\t\tSettings : "
				"/home/flameskin/iwebyou/AT/apps/strategy/build/settings.json\n\t} <valid: "
				"true>,\n\t\tJson:\n\t{\n\t\tApp      : Storage\n\t\tBin      : "
				"/home/flameskin/iwebyou/AT/apps/storage/build/ATBotStrategy\n\t\tSettings : "
				"/home/flameskin/iwebyou/AT/apps/storage/build/settings.json\n\t} <valid: "
				"true>,\n\t\tJson:\n\t{\n\t\tApp      : Strategy theory checker\n\t\tBin      : "
				"/home/flameskin/iwebyou/AT/apps/strategyTheoryChecker/build/ATBotStrategy\n\t\tSettings : "
				"/home/flameskin/iwebyou/AT/apps/strategyTheoryChecker/build/settings.json\n\t} <valid: "
				"true>,\n\t\tJson:\n\t{\n\t\tApp      : Web panel\n\t\tBin      : "
				"/home/flameskin/iwebyou/AT/apps/webPanel/build/ATBotStrategy\n\t\tSettings : "
				"/home/flameskin/iwebyou/AT/apps/webPanel/build/settings.json\n\t} <valid: true>\n\t] <valid: true>\n} "
				"<valid: true>" };
			!t.Assert(json.ToString(), expectedString, "Json string interpretation is correct")) {

			return false;
		}

		if (const std::string_view expectedJson{
				R"({"Apps":[{"App":"Gateway TBank","Bin":"/home/flameskin/iwebyou/AT/apps/gateway/build/ATBotGateway","Settings":"/home/flameskin/iwebyou/AT/apps/gateway/build/settings.json"},{"App":"Strategy","Bin":"/home/flameskin/iwebyou/AT/apps/strategy/build/ATBotStrategy","Settings":"/home/flameskin/iwebyou/AT/apps/strategy/build/settings.json"},{"App":"Storage","Bin":"/home/flameskin/iwebyou/AT/apps/storage/build/ATBotStrategy","Settings":"/home/flameskin/iwebyou/AT/apps/storage/build/settings.json"},{"App":"Strategy theory checker","Bin":"/home/flameskin/iwebyou/AT/apps/strategyTheoryChecker/build/ATBotStrategy","Settings":"/home/flameskin/iwebyou/AT/apps/strategyTheoryChecker/build/settings.json"},{"App":"Web panel","Bin":"/home/flameskin/iwebyou/AT/apps/webPanel/build/ATBotStrategy","Settings":"/home/flameskin/iwebyou/AT/apps/webPanel/build/settings.json"}]})" };
			!t.Assert(json.ToJson(), expectedJson, "Json interpretation is correct")) {

			return false;
		}

		const auto& keysAndValues = json.GetKeysAndValues();

		RETURN_IF_FALSE(t.Assert(json.Valid(), true, "Json is valid"));
		RETURN_IF_FALSE(t.Assert(keysAndValues.size(), 1, "Json keysAndValues size is 1"));
		const auto* apps{ json.GetValue("Apps") };
		RETURN_IF_FALSE(t.Assert(apps != nullptr, true, "Key 'Apps' found in json"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::list<MSAPI::JsonNode>>(apps->GetValue()), true, "Type of 'Apps' is array"));
		RETURN_IF_FALSE(t.Assert(apps->Valid(), true, "Json node 'Apps' is valid"));
		RETURN_IF_FALSE(t.Assert(
			std::get<std::list<MSAPI::JsonNode>>(apps->GetValue()).empty(), false, "Json node 'Apps' is not empty"));

		struct AppSettings {
			const std::string_view bin;
			const std::string_view settings;
		};
		const std::map<std::string_view, AppSettings> expectedSettings{
			{ "Gateway TBank",
				{ "/home/flameskin/iwebyou/AT/apps/gateway/build/ATBotGateway",
					"/home/flameskin/iwebyou/AT/apps/gateway/build/settings.json" } },
			{ "Strategy",
				{ "/home/flameskin/iwebyou/AT/apps/strategy/build/ATBotStrategy",
					"/home/flameskin/iwebyou/AT/apps/strategy/build/settings.json" } },
			{ "Storage",
				{ "/home/flameskin/iwebyou/AT/apps/storage/build/ATBotStrategy",
					"/home/flameskin/iwebyou/AT/apps/storage/build/settings.json" } },
			{ "Strategy theory checker",
				{ "/home/flameskin/iwebyou/AT/apps/strategyTheoryChecker/build/ATBotStrategy",
					"/home/flameskin/iwebyou/AT/apps/strategyTheoryChecker/build/settings.json" } },
			{ "Web panel",
				{ "/home/flameskin/iwebyou/AT/apps/webPanel/build/ATBotStrategy",
					"/home/flameskin/iwebyou/AT/apps/webPanel/build/settings.json" } }
		};

		for (const auto& appSettings : std::get<std::list<MSAPI::JsonNode>>(apps->GetValue())) {
			RETURN_IF_FALSE(
				t.Assert(std::holds_alternative<MSAPI::Json>(appSettings.GetValue()), true, "Type of json is json"));
			const auto& appKeysAndValues = std::get<MSAPI::Json>(appSettings.GetValue());
			RETURN_IF_FALSE(t.Assert(appKeysAndValues.GetKeysAndValues().size(), 3, "Json size is 3"));
			const auto* appName{ appKeysAndValues.GetValue("App") };
			RETURN_IF_FALSE(t.Assert(appName != nullptr, true, "key 'App' found in json"));
			const auto* bin{ appKeysAndValues.GetValue("Bin") };
			RETURN_IF_FALSE(t.Assert(bin != nullptr, true, "key 'Bin' found in json"));
			const auto* settings{ appKeysAndValues.GetValue("Settings") };
			RETURN_IF_FALSE(t.Assert(settings != nullptr, true, "key 'Settings' found in json"));

			const auto expectedApp{ expectedSettings.find(std::get<std::string>(appName->GetValue())) };
			RETURN_IF_FALSE(t.Assert(expectedApp != expectedSettings.end(), true, "App found in expected settings"));
			RETURN_IF_FALSE(
				t.Assert(std::get<std::string>(bin->GetValue()), expectedApp->second.bin, "Bin path matches"));
			RETURN_IF_FALSE(t.Assert(
				std::get<std::string>(settings->GetValue()), expectedApp->second.settings, "Settings path matches"));
		}

		json.Clear();
		RETURN_IF_FALSE(t.Assert(json.Valid(), false, "Json is invalid after clearing"));
		RETURN_IF_FALSE(t.Assert(json.GetKeysAndValues().empty(), true, "Json is empty after clearing"));
	}

	{
		MSAPI::Json json(
			"[{\"type\":\"true\", \"logs\":[\"1Tue Jun 21 13:01:20.106297 2022: Get account information is true\", "
			"\"2Tue Jun 21 13:01:20.106297 2022: Get account information is true\" ,  \"3Tue Jun 21 "
			"13:01:20.106297 "
			"2022: Get account information is true\"],\"information\":{\"email\":\"22@2.ru\", \"balance1\":  123, "
			"\"balance2\"  :\"321\"}, \"type2\":\"true2\",\"type3\":null}]");

		if (const std::string_view expectedString{
				"Json:\n{\n\trootArray : [\n\t\tJson:\n\t{\n\t\tinformation : Json:\n\t\t{\n\t\t\tbalance1 : "
				"123\n\t\t\tbalance2 : 321\n\t\t\temail    : 22@2.ru\n\t\t} <valid: true>\n\t\tlogs   "
				"     : [\n\t\t\t1Tue Jun 21 13:01:20.106297 2022: Get account information is true,\n\t\t\t2Tue Jun 21 "
				"13:01:20.106297 2022: Get account information is true,\n\t\t\t3Tue Jun 21 13:01:20.106297 2022: Get "
				"account information is true\n\t\t] <valid: true>\n\t\ttype        : true\n\t\ttype2       : "
				"true2\n\t\ttype3       : null\n\t} <valid: true>\n\t] <valid: true>\n} <valid: true>" };
			!t.Assert(json.ToString(), expectedString, "Json string interpretation is correct")) {

			return false;
		}

		if (const std::string_view expectedJson{
				R"({"rootArray":[{"information":{"balance1":123,"balance2":"321","email":"22@2.ru"},"logs":["1Tue Jun 21 13:01:20.106297 2022: Get account information is true","2Tue Jun 21 13:01:20.106297 2022: Get account information is true","3Tue Jun 21 13:01:20.106297 2022: Get account information is true"],"type":"true","type2":"true2","type3":null}]})" };
			!t.Assert(json.ToJson(), expectedJson, "Json interpretation is correct")) {
			return false;
		}

		RETURN_IF_FALSE(t.Assert(json.Valid(), true, "Json is valid"));
		const auto& rootKeysAndValues = json.GetKeysAndValues();
		RETURN_IF_FALSE(t.Assert(rootKeysAndValues.size(), 1, "Json size is 1"));
		const auto* rootArray{ json.GetValue("rootArray") };
		RETURN_IF_FALSE(t.Assert(rootArray != nullptr, true, "key 'rootArray' found"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(rootArray->GetValue()), true,
			"type of key 'rootArray' is array"));
		RETURN_IF_FALSE(t.Assert(rootArray->Valid(), true, "Json node 'rootArray' is valid"));
		RETURN_IF_FALSE(t.Assert(std::get<std::list<MSAPI::JsonNode>>(rootArray->GetValue()).size(), 1,
			"Size of 'rootArray' json node is 1"));

		const auto& jsonObject{ *std::get<std::list<MSAPI::JsonNode>>(rootArray->GetValue()).begin() };
		RETURN_IF_FALSE(t.Assert(jsonObject.Valid(), true, "Json node 'rootArray' is valid"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<MSAPI::Json>(jsonObject.GetValue()), true, "Type of 'rootArray' is json"));

		const auto& keysAndValues = std::get<MSAPI::Json>(jsonObject.GetValue());
		RETURN_IF_FALSE(t.Assert(keysAndValues.GetKeysAndValues().size(), 5, "Json size is 5"));
		const auto* type{ keysAndValues.GetValue("type") };
		RETURN_IF_FALSE(t.Assert(type != nullptr, true, "Key 'type' found"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<std::string>(type->GetValue()), true, "Type of key 'type' is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(type->GetValue()), "true", "Value of key 'type' is 'true'"));
		const auto* type2{ keysAndValues.GetValue("type2") };
		RETURN_IF_FALSE(t.Assert(type2 != nullptr, true, "Key 'type2' found"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<std::string>(type2->GetValue()), true, "Type of key 'type2' is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(type2->GetValue()), "true2", "Value of key 'type2' is 'true2'"));
		const auto* type3{ keysAndValues.GetValue("type3") };
		RETURN_IF_FALSE(t.Assert(type3 != nullptr, true, "Key 'type3' exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<std::nullptr_t>(type3->GetValue()), true, "Type of key 'type3' is null"));
		const auto* logs{ keysAndValues.GetValue("logs") };
		RETURN_IF_FALSE(t.Assert(logs != nullptr, true, "Key 'logs' found"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::list<MSAPI::JsonNode>>(logs->GetValue()), true, "Type of key 'logs' is array"));

		std::set<std::string_view> expectedLogs{ "1Tue Jun 21 13:01:20.106297 2022: Get account information is true",
			"2Tue Jun 21 13:01:20.106297 2022: Get account information is true",
			"3Tue Jun 21 13:01:20.106297 2022: Get account information is true" };

		auto beginLogs{ std::get<std::list<MSAPI::JsonNode>>(logs->GetValue()).begin() };
		const auto endLogs{ std::get<std::list<MSAPI::JsonNode>>(logs->GetValue()).end() };
		for (; beginLogs != endLogs; ++beginLogs) {
			RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginLogs->GetValue()), true,
				"Type of 'logs' array element is string"));
			expectedLogs.erase(std::get<std::string>(beginLogs->GetValue()));
		}
		RETURN_IF_FALSE(t.Assert(expectedLogs.empty(), true, "All expected logs found"));
		const auto* information{ keysAndValues.GetValue("information") };
		RETURN_IF_FALSE(t.Assert(information != nullptr, true, "Key 'information' found"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<MSAPI::Json>(information->GetValue()), true, "Type of key 'information' is json"));
		RETURN_IF_FALSE(
			t.Assert(std::get<MSAPI::Json>(information->GetValue()).Valid(), true, "Json node 'information' is valid"));

		const auto& informationKeysAndValues = std::get<MSAPI::Json>(information->GetValue());
		RETURN_IF_FALSE(
			t.Assert(informationKeysAndValues.GetKeysAndValues().size(), 3, "'information' json size is 3"));
		const auto* informationEmail{ informationKeysAndValues.GetValue("email") };
		RETURN_IF_FALSE(t.Assert(informationEmail != nullptr, true, "Key 'email' found"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::string>(informationEmail->GetValue()), true, "Type of key 'email' is string"));
		RETURN_IF_FALSE(t.Assert(
			std::get<std::string>(informationEmail->GetValue()), "22@2.ru", "Value of key 'email' is '22@2.ru'"));
		const auto* informationBalance1{ informationKeysAndValues.GetValue("balance1") };
		RETURN_IF_FALSE(t.Assert(informationBalance1 != nullptr, true, "Key 'balance1' found"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<uint64_t>(informationBalance1->GetValue()), true,
			"Type of key 'balance1' is unsigned integer"));
		RETURN_IF_FALSE(
			t.Assert(std::get<uint64_t>(informationBalance1->GetValue()), 123, "Value of key 'balance1' is 123"));
		const auto* informationBalance2{ informationKeysAndValues.GetValue("balance2") };
		RETURN_IF_FALSE(t.Assert(informationBalance2 != nullptr, true, "Key 'balance2' found"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(informationBalance2->GetValue()), true,
			"Type of key 'balance2' is string"));
		RETURN_IF_FALSE(t.Assert(
			std::get<std::string>(informationBalance2->GetValue()), "321", "Value of key 'balance2' is '321'"));
	}

	{
		MSAPI::Json json{ R"({
			"type": "true",
			"logs": [
				"1Tue Jun 21 13:01:20.106297",
				"2Tue Jun 21 13:01:20.106297",
				"3Tue Jun 21 13:01:20.106297"
			],
			"information": {
				"email": "\t\n\\22@2.ru\n\\\"\t",
				"balance1": 123,
				"balance2": "321"
			},
			"Apps": [
				{
					"App": "Gatewa\\y TBank\"\"",
					"Bin": "/home/flameskin/iwebyou/AT/apps/gateway/build/ATBotGateway",
					"Settings": "/home/flameskin/iwebyou/AT/apps/gateway/build/settings.json"
				},
				"true",
				false,
				554,
				{
					"Object": "true",
					"Object2": "true2\\",
					"false": false
				},
				"891.42123",
				{},
				"",
				[]
			],
			"type2": "true2",
			"float": 0.000000001,
			"Object": {
				"Array": [
					"1",
					"2",
					{},
					"3"
				],
				"Object": {
					"Array1": [
						"1",
						"2",
						"3",
						{
							"Array": [
								"1",
								"2",
								"3",
								""
							]
						}
					],
					"Array2": [
						0,
						-1,
						-3242342.93245234
					],
					"boolean": [
						true,
						false,
						true,
						false
					]
				}
			}
		})" };

		if (const std::string_view expectedString{
				"Json:\n{\n\tApps        : [\n\t\tJson:\n\t{\n\t\tApp      : Gatewa\\\\y TBank\\\"\\\"\n\t\tBin      "
				": /home/flameskin/iwebyou/AT/apps/gateway/build/ATBotGateway\n\t\tSettings : "
				"/home/flameskin/iwebyou/AT/apps/gateway/build/settings.json\n\t} <valid: "
				"true>,\n\t\ttrue,\n\t\tfalse,\n\t\t554,\n\t\tJson:\n\t{\n\t\tObject  : "
				"true\n\t\tObject2 : true2\\\\\n\t\tfalse   : false\n\t} <valid: "
				"true>,\n\t\t891.42123,\n\t\tJson:\n\t{} <valid: true>,\n\t\t,\n\t\t[] <valid: true>\n\t] <valid: "
				"true>\n\tObject      : Json:\n\t{\n\t\tArray  : [\n\t\t\t1,\n\t\t\t2,\n\t\t\tJson:\n\t\t{} <valid: "
				"true>,\n\t\t\t3\n\t\t] <valid: true>\n\t\tObject : Json:\n\t\t{\n\t\t\tArray1  : "
				"[\n\t\t\t\t1,\n\t\t\t\t2,\n\t\t\t\t3,\n\t\t\t\tJson:\n\t\t\t{\n\t\t\t\tArray : "
				"[\n\t\t\t\t\t1,\n\t\t\t\t\t2,\n\t\t\t\t\t3,\n\t\t\t\t\t\n\t\t\t\t] <valid: true>\n\t\t\t} <valid: "
				"true>\n\t\t\t] <valid: true>\n\t\t\tArray2  : "
				"[\n\t\t\t\t0,\n\t\t\t\t-1,\n\t\t\t\t-3242342."
				"93245234014466405\n\t\t\t] <valid: true>\n\t\t\tboolean : "
				"[\n\t\t\t\ttrue,\n\t\t\t\tfalse,\n\t\t\t\ttrue,\n\t\t\t\tfalse\n\t\t\t] <valid: true>\n\t\t} <valid: "
				"true>\n\t} <valid: true>\n\tfloat       : 0.00000000100000000\n\tinformation : "
				"Json:\n\t{\n\t\tbalance1 : 123\n\t\tbalance2 : 321\n\t\temail    : "
				"\\t\\n\\\\22@2.ru\\n\\\\\\\"\\t\n\t} <valid: true>\n\tlogs        : [\n\t\t1Tue Jun 21 "
				"13:01:20.106297,\n\t\t2Tue Jun 21 13:01:20.106297,\n\t\t3Tue Jun 21 13:01:20.106297\n\t] <valid: "
				"true>\n\ttype        : true\n\ttype2       : true2\n} <valid: true>" };
			!t.Assert(json.ToString(), expectedString, "Json string interpretation is correct")) {

			return false;
		}

		if (const std::string_view expectedJson{
				R"({"Apps":[{"App":"Gatewa\\y TBank\"\"","Bin":"/home/flameskin/iwebyou/AT/apps/gateway/build/ATBotGateway","Settings":"/home/flameskin/iwebyou/AT/apps/gateway/build/settings.json"},"true",false,554,{"Object":"true","Object2":"true2\\","false":false},"891.42123",{},"",[]],"Object":{"Array":["1","2",{},"3"],"Object":{"Array1":["1","2","3",{"Array":["1","2","3",""]}],"Array2":[0,-1,-3242342.93245234014466405],"boolean":[true,false,true,false]}},"float":0.00000000100000000,"information":{"balance1":123,"balance2":"321","email":"\t\n\\22@2.ru\n\\\"\t"},"logs":["1Tue Jun 21 13:01:20.106297","2Tue Jun 21 13:01:20.106297","3Tue Jun 21 13:01:20.106297"],"type":"true","type2":"true2"})" };
			!t.Assert(json.ToJson(), expectedJson, "Json interpretation is correct")) {

			return false;
		}

		RETURN_IF_FALSE(t.Assert(json.Valid(), true, "Json is valid"));

		const auto& keysAndValues = json.GetKeysAndValues();

		RETURN_IF_FALSE(t.Assert(keysAndValues.size(), 7, "Json size is 7"));

		const auto* type{ json.GetValue("type") };
		RETURN_IF_FALSE(t.Assert(type != nullptr, true, "Key 'type' found"));
		const auto* logs{ json.GetValue("logs") };
		RETURN_IF_FALSE(t.Assert(logs != nullptr, true, "Key 'logs' found"));
		const auto* information{ json.GetValue("information") };
		RETURN_IF_FALSE(t.Assert(information != nullptr, true, "Key 'information' found"));
		const auto* apps{ json.GetValue("Apps") };
		RETURN_IF_FALSE(t.Assert(apps != nullptr, true, "Key 'Apps' found"));
		const auto* type2{ json.GetValue("type2") };
		RETURN_IF_FALSE(t.Assert(type2 != nullptr, true, "Key 'type2' found"));
		const auto* floatNode{ json.GetValue("float") };
		RETURN_IF_FALSE(t.Assert(floatNode != nullptr, true, "Key 'float' found"));
		const auto* object{ json.GetValue("Object") };
		RETURN_IF_FALSE(t.Assert(object != nullptr, true, "Key 'Object' found"));

		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<std::string>(type->GetValue()), true, "Type of key 'type' is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(type->GetValue()), "true", "Value of key 'type' is 'true'"));

		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::list<MSAPI::JsonNode>>(logs->GetValue()), true, "Type of key 'logs' is array"));

		std::set<std::string_view> expectedLogs{ "1Tue Jun 21 13:01:20.106297", "2Tue Jun 21 13:01:20.106297",
			"3Tue Jun 21 13:01:20.106297" };

		auto beginLogs{ std::get<std::list<MSAPI::JsonNode>>(logs->GetValue()).begin() };
		const auto endLogs{ std::get<std::list<MSAPI::JsonNode>>(logs->GetValue()).end() };
		for (; beginLogs != endLogs; ++beginLogs) {
			RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginLogs->GetValue()), true,
				"Type of 'logs' json node is string"));
			expectedLogs.erase(std::get<std::string>(beginLogs->GetValue()));
		}
		RETURN_IF_FALSE(t.Assert(expectedLogs.empty(), true, "All expected logs found"));

		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<MSAPI::Json>(information->GetValue()), true, "Type of key 'information' is json"));

		RETURN_IF_FALSE(
			t.Assert(std::get<MSAPI::Json>(information->GetValue()).Valid(), true, "Json node 'information' is valid"));

		const auto& informationKeysAndValues = std::get<MSAPI::Json>(information->GetValue());
		RETURN_IF_FALSE(
			t.Assert(informationKeysAndValues.GetKeysAndValues().size(), 3, "'information' json size is 3"));

		const auto* informationEmail{ informationKeysAndValues.GetValue("email") };
		RETURN_IF_FALSE(t.Assert(informationEmail != nullptr, true, "Key 'email' found"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::string>(informationEmail->GetValue()), true, "Type of key 'email' is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(informationEmail->GetValue()), "\\t\\n\\\\22@2.ru\\n\\\\\\\"\\t",
			"Value of key 'email' is correct"));
		const auto* informationBalance1{ informationKeysAndValues.GetValue("balance1") };
		RETURN_IF_FALSE(t.Assert(informationBalance1 != nullptr, true, "Key 'balance1' found"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<uint64_t>(informationBalance1->GetValue()), true,
			"Type of key 'balance1' is unsigned integer"));
		RETURN_IF_FALSE(
			t.Assert(std::get<uint64_t>(informationBalance1->GetValue()), 123, "Value of key 'balance1' is 123"));
		const auto* informationBalance2{ informationKeysAndValues.GetValue("balance2") };
		RETURN_IF_FALSE(t.Assert(informationBalance2 != nullptr, true, "Key 'balance2' found"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(informationBalance2->GetValue()), true,
			"Type of key 'balance2' is string"));
		RETURN_IF_FALSE(t.Assert(
			std::get<std::string>(informationBalance2->GetValue()), "321", "Value of key 'balance2' is '321'"));

		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::list<MSAPI::JsonNode>>(apps->GetValue()), true, "Type of key 'Apps' is array"));

		RETURN_IF_FALSE(t.Assert(apps->Valid(), true, "Json node 'Apps' is valid"));

		const auto& appsArray = std::get<std::list<MSAPI::JsonNode>>(apps->GetValue());
		RETURN_IF_FALSE(t.Assert(appsArray.size(), 9, "Json node 'Apps' size is 9"));

		auto beginApps{ appsArray.begin() };

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<MSAPI::Json>(beginApps->GetValue()), true,
			"Type of first element of 'Apps' json node is json"));
		RETURN_IF_FALSE(t.Assert(std::get<MSAPI::Json>(beginApps->GetValue()).GetKeysAndValues().size(), 3,
			"First element of 'Apps' json node size is 3"));
		const auto* appsArrayApp{ std::get<MSAPI::Json>(beginApps->GetValue()).GetValue("App") };
		RETURN_IF_FALSE(t.Assert(appsArrayApp != nullptr, true, "Key 'App' exists"));

		const auto* appsArrayBin{ std::get<MSAPI::Json>(beginApps->GetValue()).GetValue("Bin") };
		RETURN_IF_FALSE(t.Assert(appsArrayBin != nullptr, true, "Key 'Bin' exists"));
		const auto* appsArraySettings{ std::get<MSAPI::Json>(beginApps->GetValue()).GetValue("Settings") };
		RETURN_IF_FALSE(t.Assert(appsArraySettings != nullptr, true, "Key 'Settings' exists"));

		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::string>(appsArrayApp->GetValue()), true, "Type of key 'App' is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(appsArrayApp->GetValue()), "Gatewa\\\\y TBank\\\"\\\"",
			"Value of key 'App' is correct"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::string>(appsArrayBin->GetValue()), true, "Type of key 'Bin' is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(appsArrayBin->GetValue()),
			"/home/flameskin/iwebyou/AT/apps/gateway/build/ATBotGateway", "Value of key 'Bin' is correct"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(appsArraySettings->GetValue()), true,
			"Type of key 'Settings' is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(appsArraySettings->GetValue()),
			"/home/flameskin/iwebyou/AT/apps/gateway/build/settings.json", "Value of key 'Settings' is correct"));

		++beginApps;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginApps->GetValue()), true,
			"Type of second element of 'Apps' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginApps->GetValue()), "true",
			"Value of second element of 'Apps' json node is 'true'"));

		++beginApps;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<bool>(beginApps->GetValue()), true,
			"Type of third element of 'Apps' json node is boolean"));
		RETURN_IF_FALSE(t.Assert(
			std::get<bool>(beginApps->GetValue()), false, "Value of third element of 'Apps' json node is false"));

		++beginApps;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<uint64_t>(informationBalance1->GetValue()), true,
			"Type of fourth element of 'Apps' json node is unsigned integer"));
		RETURN_IF_FALSE(t.Assert(
			std::get<uint64_t>(beginApps->GetValue()), 554, "Value of fourth element of 'Apps' json node is 554"));

		++beginApps;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<MSAPI::Json>(beginApps->GetValue()), true,
			"Type of fifth element of 'Apps' json node is json"));
		RETURN_IF_FALSE(t.Assert(std::get<MSAPI::Json>(beginApps->GetValue()).GetKeysAndValues().size(), 3,
			"Fifth element of 'Apps' json node size is 3"));
		const auto* nodeObject1{ std::get<MSAPI::Json>(beginApps->GetValue()).GetValue("Object") };
		RETURN_IF_FALSE(t.Assert(nodeObject1 != nullptr, true, "Key 'Object' exists"));
		const auto* nodeObject2{ std::get<MSAPI::Json>(beginApps->GetValue()).GetValue("Object2") };
		RETURN_IF_FALSE(t.Assert(nodeObject2 != nullptr, true, "Key 'Object2' exists"));
		const auto* nodeFalse1{ std::get<MSAPI::Json>(beginApps->GetValue()).GetValue("false") };
		RETURN_IF_FALSE(t.Assert(nodeFalse1 != nullptr, true, "Key 'false' exists"));

		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::string>(nodeObject1->GetValue()), true, "Type of key 'Object' is string"));
		RETURN_IF_FALSE(
			t.Assert(std::get<std::string>(nodeObject1->GetValue()), "true", "Value of key 'Object' is 'true'"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::string>(nodeObject2->GetValue()), true, "Type of key 'Object2' is string"));
		RETURN_IF_FALSE(
			t.Assert(std::get<std::string>(nodeObject2->GetValue()), "true2\\\\", "Value of key 'Object2' is correct"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<bool>(nodeFalse1->GetValue()), true, "Type of key 'false' is boolean"));
		RETURN_IF_FALSE(t.Assert(std::get<bool>(nodeFalse1->GetValue()), false, "Value of key 'false' is false"));

		++beginApps;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginApps->GetValue()), true,
			"Type of sixth element of 'Apps' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginApps->GetValue()), "891.42123",
			"Value of sixth element of 'Apps' json node is '891.42123'"));

		++beginApps;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<MSAPI::Json>(beginApps->GetValue()), true,
			"Type of seventh element of 'Apps' json node is json"));
		RETURN_IF_FALSE(t.Assert(std::get<MSAPI::Json>(beginApps->GetValue()).Valid(), true,
			"Seventh element of 'Apps' json node is valid"));
		RETURN_IF_FALSE(t.Assert(std::get<MSAPI::Json>(beginApps->GetValue()).GetKeysAndValues().empty(), true,
			"Seventh element of 'Apps' json node size is empty"));

		++beginApps;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginApps->GetValue()), true,
			"Type of seventh element of 'Apps' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginApps->GetValue()), "",
			"Value of seventh element of 'Apps' json node is empty string"));

		++beginApps;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(beginApps->GetValue()), true,
			"Type of seventh element of 'Apps' json node is array"));
		RETURN_IF_FALSE(t.Assert(std::get<std::list<MSAPI::JsonNode>>(beginApps->GetValue()).empty(), true,
			"Element of 'Apps' json node size is empty"));

		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<std::string>(type2->GetValue()), true, "Type of key 'type2' is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(type2->GetValue()), "true2", "Value of key 'type2' is 'true2'"));

		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(floatNode->GetValue()), true, "Type of key 'float' is double"));
		RETURN_IF_FALSE(
			t.Assert(std::get<double>(floatNode->GetValue()), 0.000000001, "Value of key 'float' is 0.000000001"));

		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<MSAPI::Json>(object->GetValue()), true, "Type of key 'Object' is json"));
		RETURN_IF_FALSE(
			t.Assert(std::get<MSAPI::Json>(object->GetValue()).Valid(), true, "Json node 'Object' is valid"));
		const auto& objectKeysAndValues1 = std::get<MSAPI::Json>(object->GetValue());
		RETURN_IF_FALSE(t.Assert(objectKeysAndValues1.GetKeysAndValues().size(), 2, "'Object' json size is 2"));
		const auto* objectArray{ objectKeysAndValues1.GetValue("Array") };
		RETURN_IF_FALSE(t.Assert(objectArray != nullptr, true, "Key 'Array' exists"));
		const auto* objectObject{ objectKeysAndValues1.GetValue("Object") };
		RETURN_IF_FALSE(t.Assert(objectKeysAndValues1.GetValue("Object") != nullptr, true, "Key 'Object' exists"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(objectArray->GetValue()), true,
			"Type of key 'Array' is array"));
		const auto& objectArrayValue = std::get<std::list<MSAPI::JsonNode>>(objectArray->GetValue());
		RETURN_IF_FALSE(t.Assert(objectArrayValue.size(), 4, "Size of key 'Array' is 4"));
		auto beginArray{ objectArrayValue.begin() };
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginArray->GetValue()), true,
			"Type of first element of 'Array' json node is string"));
		RETURN_IF_FALSE(t.Assert(
			std::get<std::string>(beginArray->GetValue()), "1", "Value of first element of 'Array' json node is '1'"));
		++beginArray;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginArray->GetValue()), true,
			"Type of second element of 'Array' json node is string"));
		RETURN_IF_FALSE(t.Assert(
			std::get<std::string>(beginArray->GetValue()), "2", "Value of second element of 'Array' json node is '2'"));
		++beginArray;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<MSAPI::Json>(beginArray->GetValue()), true,
			"Type of third element of 'Array' json node is json"));
		RETURN_IF_FALSE(t.Assert(std::get<MSAPI::Json>(beginArray->GetValue()).Valid(), true,
			"Third element of 'Array' json node is valid"));

		RETURN_IF_FALSE(t.Assert(std::get<MSAPI::Json>(beginArray->GetValue()).GetKeysAndValues().empty(), true,
			"Third element of 'Array' json node size is empty"));

		++beginArray;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginArray->GetValue()), true,
			"Type of fourth element of 'Array' json node is string"));
		RETURN_IF_FALSE(t.Assert(
			std::get<std::string>(beginArray->GetValue()), "3", "Value of fourth element of 'Array' json node is '3'"));

		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<MSAPI::Json>(objectObject->GetValue()), true, "Type of key 'Object' is json"));

		RETURN_IF_FALSE(
			t.Assert(std::get<MSAPI::Json>(objectObject->GetValue()).Valid(), true, "Json node 'Object' is valid"));

		const auto& objectObjectKeysAndValues2 = std::get<MSAPI::Json>(objectObject->GetValue());
		RETURN_IF_FALSE(t.Assert(objectObjectKeysAndValues2.GetKeysAndValues().size(), 3, "'Object' json size is 3"));

		const auto* objectObjectArray1{ objectObjectKeysAndValues2.GetValue("Array1") };
		RETURN_IF_FALSE(t.Assert(objectObjectArray1 != nullptr, true, "Key 'Array1' exists"));
		const auto* objectObjectArray2{ objectObjectKeysAndValues2.GetValue("Array2") };
		RETURN_IF_FALSE(t.Assert(objectObjectArray2 != nullptr, true, "Key 'Array2' exists"));
		const auto* objectObjectBoolean{ objectObjectKeysAndValues2.GetValue("boolean") };
		RETURN_IF_FALSE(t.Assert(objectObjectBoolean != nullptr, true, "Key 'boolean' exists"));

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(objectObjectArray1->GetValue()),
			true, "Type of key 'Array1' is array"));

		const auto& objectObjectArray1Value = std::get<std::list<MSAPI::JsonNode>>(objectObjectArray1->GetValue());
		RETURN_IF_FALSE(t.Assert(objectObjectArray1Value.size(), 4, "Size of key 'Array1' is 4"));

		auto beginArray1{ objectObjectArray1Value.begin() };

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginArray1->GetValue()), true,
			"Type of first element of 'Array1' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginArray1->GetValue()), "1",
			"Value of first element of 'Array1' json node is '1'"));

		++beginArray1;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginArray1->GetValue()), true,
			"Type of second element of 'Array1' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginArray1->GetValue()), "2",
			"Value of second element of 'Array1' json node is '2'"));

		++beginArray1;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginArray1->GetValue()), true,
			"Type of third element of 'Array1' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginArray1->GetValue()), "3",
			"Value of third element of 'Array1' json node is '3'"));

		++beginArray1;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<MSAPI::Json>(beginArray1->GetValue()), true,
			"Type of fourth element of 'Array1' json node is json"));
		RETURN_IF_FALSE(t.Assert(std::get<MSAPI::Json>(beginArray1->GetValue()).GetKeysAndValues().size(), 1,
			"Size of fourth element of 'Array1' json node is 1"));
		const auto* beginArray1Array{ std::get<MSAPI::Json>(beginArray1->GetValue()).GetValue("Array") };
		RETURN_IF_FALSE(t.Assert(beginArray1Array != nullptr, true, "Key 'Array' exists"));

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(beginArray1Array->GetValue()), true,
			"Type of key 'Array' is array"));

		const auto& objectObjectArray = std::get<std::list<MSAPI::JsonNode>>(beginArray1Array->GetValue());
		RETURN_IF_FALSE(t.Assert(objectObjectArray.size(), 4, "Size of key 'Array' is 4"));

		auto beginObjectObjectArray{ objectObjectArray.begin() };

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginObjectObjectArray->GetValue()), true,
			"Type of first element of 'Array' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginObjectObjectArray->GetValue()), "1",
			"Value of first element of 'Array' json node is '1'"));

		++beginObjectObjectArray;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginObjectObjectArray->GetValue()), true,
			"Type of second element of 'Array' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginObjectObjectArray->GetValue()), "2",
			"Value of second element of 'Array' json node is '2'"));

		++beginObjectObjectArray;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginObjectObjectArray->GetValue()), true,
			"Type of third element of 'Array' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginObjectObjectArray->GetValue()), "3",
			"Value of third element of 'Array' json node is '3'"));

		++beginObjectObjectArray;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(beginObjectObjectArray->GetValue()), true,
			"Type of fourth element of 'Array' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(beginObjectObjectArray->GetValue()).empty(), true,
			"Value of fourth element of 'Array' json node is empty"));

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(objectObjectArray2->GetValue()),
			true, "Type of key 'Array2' is array"));

		const auto& objectObjectArray2Value = std::get<std::list<MSAPI::JsonNode>>(objectObjectArray2->GetValue());
		RETURN_IF_FALSE(t.Assert(objectObjectArray2Value.size(), 3, "Size of key 'Array2' is 3"));

		auto beginArray2{ objectObjectArray2Value.begin() };

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<uint64_t>(beginArray2->GetValue()), true,
			"Type of first element of 'Array2' json node is unsigned integer"));
		RETURN_IF_FALSE(t.Assert(
			std::get<uint64_t>(beginArray2->GetValue()), 0, "Value of first element of 'Array2' json node is 0"));

		++beginArray2;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<int64_t>(beginArray2->GetValue()), true,
			"Type of second element of 'Array2' json node is signed integer"));
		RETURN_IF_FALSE(t.Assert(
			std::get<int64_t>(beginArray2->GetValue()), -1, "Value of second element of 'Array2' json node is -1"));

		++beginArray2;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(beginArray2->GetValue()), true,
			"Type of third element of 'Array2' json node is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(beginArray2->GetValue()), -3242342.93245234,
			"Value of third element of 'Array2' json node is -3242342.93245234"));

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(objectObjectBoolean->GetValue()),
			true, "Type of key 'boolean' is array"));

		const auto& objectObjectBooleanValue = std::get<std::list<MSAPI::JsonNode>>(objectObjectBoolean->GetValue());
		RETURN_IF_FALSE(t.Assert(objectObjectBooleanValue.size(), 4, "Size of key 'boolean' is 4"));

		auto beginBoolean{ objectObjectBooleanValue.begin() };

		RETURN_IF_FALSE(t.Assert(std::holds_alternative<bool>(beginBoolean->GetValue()), true,
			"Type of first element of 'boolean' json node is boolean"));
		RETURN_IF_FALSE(t.Assert(
			std::get<bool>(beginBoolean->GetValue()), true, "Value of first element of 'boolean' json node is true"));

		++beginBoolean;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<bool>(beginBoolean->GetValue()), true,
			"Type of second element of 'boolean' json node is boolean"));
		RETURN_IF_FALSE(t.Assert(std::get<bool>(beginBoolean->GetValue()), false,
			"Value of second element of 'boolean' json node is false"));

		++beginBoolean;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<bool>(beginBoolean->GetValue()), true,
			"Type of third element of 'boolean' json node is boolean"));
		RETURN_IF_FALSE(t.Assert(
			std::get<bool>(beginBoolean->GetValue()), true, "Value of third element of 'boolean' json node is true"));

		++beginBoolean;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<bool>(beginBoolean->GetValue()), true,
			"Type of fourth element of 'boolean' json node is boolean"));
		RETURN_IF_FALSE(t.Assert(std::get<bool>(beginBoolean->GetValue()), false,
			"Value of fourth element of 'boolean' json node is false"));
	}

	{
		MSAPI::Json json{ "{\"30014\":[[\"Bond\",0.04],[\"Currency\",0.4]]}" };

		RETURN_IF_FALSE(t.Assert(json.Valid(), true, "Json is valid"));
		RETURN_IF_FALSE(t.Assert(json.GetKeysAndValues().size(), 1, "Json size is 1"));
		const auto* node{ json.GetValue("30014") };
		RETURN_IF_FALSE(t.Assert(node != nullptr, true, "Key '30014' exists"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(node->GetValue()), true,
			"Type of key '30014' is array"));
		RETURN_IF_FALSE(t.Assert(node->Valid(), true, "Key '30014' is valid"));
		const auto& array{ std::get<std::list<MSAPI::JsonNode>>(node->GetValue()) };
		RETURN_IF_FALSE(t.Assert(array.size(), 2, "Size of key '30014' is 2"));
		auto beginArray{ array.begin() };
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(beginArray->GetValue()), true,
			"Type of first element of '30014' json node is array"));
		const auto& firstArray{ std::get<std::list<MSAPI::JsonNode>>(beginArray->GetValue()) };
		RETURN_IF_FALSE(t.Assert(firstArray.size(), 2, "Size of first element of '30014' json node is 2"));
		RETURN_IF_FALSE(
			t.Assert(firstArray.front().Valid(), true, "First element of first element of '30014' json node is valid"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(firstArray.front().GetValue()), true,
			"Type of first element of first element of '30014' json node is string"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(firstArray.front().GetValue()), "Bond",
			"Value of first element of first element of '30014' json node is 'Bond'"));
		RETURN_IF_FALSE(
			t.Assert(firstArray.back().Valid(), true, "Second element of first element of '30014' json node is valid"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(firstArray.back().GetValue()), true,
			"Type of second element of first element of '30014' json node is double"));
		if (!t.Assert(std::get<double>(firstArray.back().GetValue()), 0.04,
				"Value of second element of first element of '30014' json node is 0.04")) {
			return false;
		}

		++beginArray;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(beginArray->GetValue()), true,
			"Type of second element of '30014' json node is array"));

		const auto& secondArray{ std::get<std::list<MSAPI::JsonNode>>(beginArray->GetValue()) };
		RETURN_IF_FALSE(t.Assert(secondArray.size(), 2, "Size of second element of '30014' json node is 2"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::string>(secondArray.front().GetValue()), true,
			"Type of first element of second element of '30014' json node is string"));
		RETURN_IF_FALSE(t.Assert(
			secondArray.front().Valid(), true, "First element of second element of '30014' json node is valid"));
		RETURN_IF_FALSE(t.Assert(std::get<std::string>(secondArray.front().GetValue()), "Currency",
			"Value of first element of second element of '30014' json node is 'Currency'"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(secondArray.back().GetValue()), true,
			"Type of second element of second element of '30014' json node is double"));
		RETURN_IF_FALSE(t.Assert(
			secondArray.back().Valid(), true, "Second element of second element of '30014' json node is valid"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(secondArray.back().GetValue()), 0.4,
			"Value of second element of second element of '30014' json node is 0.4"));
	}

	{
		MSAPI::Json json{ "{\"Currency\":0.4}" };

		RETURN_IF_FALSE(t.Assert(json.Valid(), true, "Json is valid"));
		RETURN_IF_FALSE(t.Assert(json.GetKeysAndValues().size(), 1, "Json size is 1"));

		const auto* node{ json.GetValue("Currency") };
		RETURN_IF_FALSE(t.Assert(node != nullptr, true, "Key 'Currency' exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(node->GetValue()), true, "Type of key 'Currency' is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(node->GetValue()), 0.4, "Value of key 'Currency' is 0.4"));
	}

	{
		MSAPI::Json json{
			R"({
		"IntExpPos1": 1e3,
		"IntExpNeg1": 1e-3,
		"IntExpPos2": -1e3,
		"IntExpNeg2": -1e-3,
		"DoubleExpPos1_1": 1.23e3,
		"DoubleExpNeg1_1": 1.23e-3,
		"DoubleExpPos2_1": 1.001e3,
		"DoubleExpPos3_1": 1.0001e3,
		"DoubleExpPos1_2": -1.23e3,
		"DoubleExpNeg1_2": -1.23e-3,
		"DoubleExpPos2_2": -1.001e3,
		"DoubleExpPos3_2": -1.0001e3,
		"DoubleExpPos4": 1.647393946349473e-3,
		"DoubleExpNeg4": -1.647393946349473e-3,
		"DoubleExpPos5": 1.647393946349473e16,
		"DoubleExpNeg5": -1.647393946349473e16,
		"DoubleExpPos6": 1.647393946349473e6,
		"DoubleExpNeg6": -1.647393946349473e6,
		"DoubleExpPos7": 1e-9,
		"DoubleExpNeg7": -1e-9,
		"Array": [1e3, 1e-3, -1e3, -1e-3, 1.23e3, 1.23e-3, 1.001e3, 1.0001e3, -1.23e3, -1.23e-3, -1.001e3, -1.0001e3
		, 1.647393946349473e-3, -1.647393946349473e-3, 1.647393946349473e16, -1.647393946349473e16, 1.647393946349473e6, -1.647393946349473e6, 1e-9. null, -1e-9]})"
		};

		RETURN_IF_FALSE(t.Assert(json.Valid(), true, "Json is valid"));
		RETURN_IF_FALSE(t.Assert(json.GetKeysAndValues().size(), 21, "Json size is 21"));
		const auto* nodeIntExpPos1{ json.GetValue("IntExpPos1") };
		RETURN_IF_FALSE(t.Assert(nodeIntExpPos1 != nullptr, true, "IntExpPos1 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<uint64_t>(nodeIntExpPos1->GetValue()), true, "IntExpPos1 is uint64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<uint64_t>(nodeIntExpPos1->GetValue()), 1000, "IntExpPos1 value is 1000"));

		const auto* nodeIntExpNeg1{ json.GetValue("IntExpNeg1") };
		RETURN_IF_FALSE(t.Assert(nodeIntExpNeg1 != nullptr, true, "IntExpNeg1 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(nodeIntExpNeg1->GetValue()), true, "IntExpNeg1 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(nodeIntExpNeg1->GetValue()), 0.001, "IntExpNeg1 value is 0.001"));

		const auto* nodeIntExpPos2{ json.GetValue("IntExpPos2") };
		RETURN_IF_FALSE(t.Assert(nodeIntExpPos2 != nullptr, true, "IntExpPos2 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<int64_t>(nodeIntExpPos2->GetValue()), true, "IntExpPos2 is int64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<int64_t>(nodeIntExpPos2->GetValue()), -1000, "IntExpPos2 value is -1000"));

		const auto* nodeIntExpNeg2{ json.GetValue("IntExpNeg2") };
		RETURN_IF_FALSE(t.Assert(nodeIntExpNeg2 != nullptr, true, "IntExpNeg2 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(nodeIntExpNeg2->GetValue()), true, "IntExpNeg2 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(nodeIntExpNeg2->GetValue()), -0.001, "IntExpNeg2 value is -0.001"));

		const auto* nodeDoubleExpPos1_1{ json.GetValue("DoubleExpPos1_1") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos1_1 != nullptr, true, "DoubleExpPos1_1 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<uint64_t>(nodeDoubleExpPos1_1->GetValue()), true, "DoubleExpPos1_1 is uint64_t"));
		RETURN_IF_FALSE(
			t.Assert(std::get<uint64_t>(nodeDoubleExpPos1_1->GetValue()), 1230, "DoubleExpPos1_1 value is 1230"));

		const auto* nodeDoubleExpNeg1_1{ json.GetValue("DoubleExpNeg1_1") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpNeg1_1 != nullptr, true, "DoubleExpNeg1_1 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<double>(nodeDoubleExpNeg1_1->GetValue()), true, "DoubleExpNeg1_1 is double"));
		RETURN_IF_FALSE(
			t.Assert(std::get<double>(nodeDoubleExpNeg1_1->GetValue()), 0.00123, "DoubleExpNeg1_1 value is 0.00123"));

		const auto* nodeDoubleExpPos2_1{ json.GetValue("DoubleExpPos2_1") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos2_1 != nullptr, true, "DoubleExpPos2_1 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<uint64_t>(nodeDoubleExpPos2_1->GetValue()), true, "DoubleExpPos2_1 is uint64_t"));
		RETURN_IF_FALSE(
			t.Assert(std::get<uint64_t>(nodeDoubleExpPos2_1->GetValue()), 1001, "DoubleExpPos2_1 value is 1001"));

		const auto* nodeDoubleExpPos3_1{ json.GetValue("DoubleExpPos3_1") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos3_1 != nullptr, true, "DoubleExpPos3_1 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<double>(nodeDoubleExpPos3_1->GetValue()), true, "DoubleExpPos3_1 is double"));
		RETURN_IF_FALSE(
			t.Assert(std::get<double>(nodeDoubleExpPos3_1->GetValue()), 1000.1, "DoubleExpPos3_1 value is 1000.1"));

		const auto* nodeDoubleExpPos1_2{ json.GetValue("DoubleExpPos1_2") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos1_2 != nullptr, true, "DoubleExpPos1_2 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<int64_t>(nodeDoubleExpPos1_2->GetValue()), true, "DoubleExpPos1_2 is int64_t"));
		RETURN_IF_FALSE(
			t.Assert(std::get<int64_t>(nodeDoubleExpPos1_2->GetValue()), -1230, "DoubleExpPos1_2 value is -1230"));

		const auto* nodeDoubleExpNeg1_2{ json.GetValue("DoubleExpNeg1_2") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpNeg1_2 != nullptr, true, "DoubleExpNeg1_2 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<double>(nodeDoubleExpNeg1_2->GetValue()), true, "DoubleExpNeg1_2 is double"));
		RETURN_IF_FALSE(
			t.Assert(std::get<double>(nodeDoubleExpNeg1_2->GetValue()), -0.00123, "DoubleExpNeg1_2 value is -0.00123"));

		const auto* nodeDoubleExpPos2_2{ json.GetValue("DoubleExpPos2_2") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos2_2 != nullptr, true, "DoubleExpPos2_2 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<int64_t>(nodeDoubleExpPos2_2->GetValue()), true, "DoubleExpPos2_2 is int64_t"));
		RETURN_IF_FALSE(
			t.Assert(std::get<int64_t>(nodeDoubleExpPos2_2->GetValue()), -1001, "DoubleExpPos2_2 value is -1001"));

		const auto* nodeDoubleExpPos3_2{ json.GetValue("DoubleExpPos3_2") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos3_2 != nullptr, true, "DoubleExpPos3_2 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<double>(nodeDoubleExpPos3_2->GetValue()), true, "DoubleExpPos3_2 is double"));
		RETURN_IF_FALSE(
			t.Assert(std::get<double>(nodeDoubleExpPos3_2->GetValue()), -1000.1, "DoubleExpPos3_2 value is -1000.1"));

		const auto* nodeDoubleExpPos4{ json.GetValue("DoubleExpPos4") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos4 != nullptr, true, "DoubleExpPos4 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(nodeDoubleExpPos4->GetValue()), true, "DoubleExpPos4 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(nodeDoubleExpPos4->GetValue()), 0.001647393946349473,
			"DoubleExpPos4 value is 0.001647393946349473"));

		const auto* nodeDoubleExpNeg4{ json.GetValue("DoubleExpNeg4") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpNeg4 != nullptr, true, "DoubleExpNeg4 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(nodeDoubleExpNeg4->GetValue()), true, "DoubleExpNeg4 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(nodeDoubleExpNeg4->GetValue()), -0.001647393946349473,
			"DoubleExpNeg4 value is -0.001647393946349473"));

		const auto* nodeDoubleExpPos5{ json.GetValue("DoubleExpPos5") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos5 != nullptr, true, "DoubleExpPos5 exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<uint64_t>(nodeDoubleExpPos5->GetValue()), true, "DoubleExpPos5 is uint64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<uint64_t>(nodeDoubleExpPos5->GetValue()), 16473939463494730,
			"DoubleExpPos5 value is 16473939463494730"));

		const auto* nodeDoubleExpNeg5{ json.GetValue("DoubleExpNeg5") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpNeg5 != nullptr, true, "DoubleExpNeg5 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<int64_t>(nodeDoubleExpNeg5->GetValue()), true, "DoubleExpNeg5 is int64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<int64_t>(nodeDoubleExpNeg5->GetValue()), -16473939463494730,
			"DoubleExpNeg5 value is -16473939463494730"));

		const auto* nodeDoubleExpPos6{ json.GetValue("DoubleExpPos6") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos6 != nullptr, true, "DoubleExpPos6 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(nodeDoubleExpPos6->GetValue()), true, "DoubleExpPos6 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(nodeDoubleExpPos6->GetValue()), 1647393.946349473,
			"DoubleExpPos6 value is 1647393.946349473"));

		const auto* nodeDoubleExpNeg6{ json.GetValue("DoubleExpNeg6") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpNeg6 != nullptr, true, "DoubleExpNeg6 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(nodeDoubleExpNeg6->GetValue()), true, "DoubleExpNeg6 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(nodeDoubleExpNeg6->GetValue()), -1647393.946349473,
			"DoubleExpNeg6 value is -1647393.946349473"));

		const auto* nodeDoubleExpPos7{ json.GetValue("DoubleExpPos7") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpPos7 != nullptr, true, "DoubleExpPos7 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(nodeDoubleExpPos7->GetValue()), true, "DoubleExpPos7 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(nodeDoubleExpPos7->GetValue()), 1e-9, "DoubleExpPos7 value is 1e-9"));

		const auto* nodeDoubleExpNeg7{ json.GetValue("DoubleExpNeg7") };
		RETURN_IF_FALSE(t.Assert(nodeDoubleExpNeg7 != nullptr, true, "DoubleExpNeg7 exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(nodeDoubleExpNeg7->GetValue()), true, "DoubleExpNeg7 is double"));
		RETURN_IF_FALSE(
			t.Assert(std::get<double>(nodeDoubleExpNeg7->GetValue()), -1e-9, "DoubleExpNeg7 value is -1e-9"));

		const auto* nodeArray{ json.GetValue("Array") };
		RETURN_IF_FALSE(t.Assert(nodeArray != nullptr, true, "Array exists"));
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<std::list<MSAPI::JsonNode>>(nodeArray->GetValue()), true,
			"Array is list<JsonNode>"));

		const auto& array{ std::get<std::list<MSAPI::JsonNode>>(nodeArray->GetValue()) };
		RETURN_IF_FALSE(t.Assert(array.size(), 21, "Array size is 21"));

		auto it{ array.begin() };
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<uint64_t>(it->GetValue()), true, "Array element 1 is uint64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<uint64_t>(it->GetValue()), 1000, "Array element 1 value is 1000"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 2 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), 0.001, "Array element 2 value is 0.001"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<int64_t>(it->GetValue()), true, "Array element 3 is int64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<int64_t>(it->GetValue()), -1000, "Array element 3 value is -1000"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 4 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), -0.001, "Array element 4 value is -0.001"));

		++it;
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<uint64_t>(it->GetValue()), true, "Array element 5 is uint64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<uint64_t>(it->GetValue()), 1230, "Array element 5 value is 1230"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 6 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), 0.00123, "Array element 6 value is 0.00123"));

		++it;
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<uint64_t>(it->GetValue()), true, "Array element 7 is uint64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<uint64_t>(it->GetValue()), 1001, "Array element 7 value is 1001"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 8 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), 1000.1, "Array element 8 value is 1000.1"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<int64_t>(it->GetValue()), true, "Array element 9 is int64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<int64_t>(it->GetValue()), -1230, "Array element 9 value is -1230"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 10 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), -0.00123, "Array element 10 value is -0.00123"));
		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<int64_t>(it->GetValue()), true, "Array element 11 is int64_t"));
		RETURN_IF_FALSE(t.Assert(std::get<int64_t>(it->GetValue()), -1001, "Array element 11 value is -1001"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 12 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), -1000.1, "Array element 12 value is -1000.1"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 13 is double"));
		RETURN_IF_FALSE(t.Assert(
			std::get<double>(it->GetValue()), 0.001647393946349473, "Array element 13 value is 0.001647393946349473"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 14 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), -0.001647393946349473,
			"Array element 14 value is -0.001647393946349473"));

		++it;
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<uint64_t>(it->GetValue()), true, "Array element 15 is uint64_t"));
		RETURN_IF_FALSE(t.Assert(
			std::get<uint64_t>(it->GetValue()), 16473939463494730, "Array element 15 value is 16473939463494730"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<int64_t>(it->GetValue()), true, "Array element 16 is int64_t"));
		RETURN_IF_FALSE(t.Assert(
			std::get<int64_t>(it->GetValue()), -16473939463494730, "Array element 16 value is -16473939463494730"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 17 is double"));
		RETURN_IF_FALSE(t.Assert(
			std::get<double>(it->GetValue()), 1647393.946349473, "Array element 17 value is 1647393.946349473"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 18 is double"));
		RETURN_IF_FALSE(t.Assert(
			std::get<double>(it->GetValue()), -1647393.946349473, "Array element 18 value is -1647393.946349473"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 19 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), 1e-9, "Array element 19 value is 1e-9"));

		++it;
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<nullptr_t>(it->GetValue()), true, "Array element 20 is nullptr_t"));

		++it;
		RETURN_IF_FALSE(t.Assert(std::holds_alternative<double>(it->GetValue()), true, "Array element 21 is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), -1e-9, "Array element 21 value is -1e-9"));
	}

	{
		std::list<MSAPI::JsonNode> array{ MSAPI::JsonNode{ "Bond" }, MSAPI::JsonNode{ 0.04 } };
		MSAPI::JsonNode arrayNode{ array };
		RETURN_IF_FALSE(t.Assert(arrayNode.Valid(), true, "JsonNode is valid"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<std::list<MSAPI::JsonNode>>(arrayNode.GetValue()), true, "JsonNode value is array"));
		const auto& arrayValue{ std::get<std::list<MSAPI::JsonNode>>(arrayNode.GetValue()) };
		RETURN_IF_FALSE(t.Assert(arrayValue.size(), 2, "JsonNode array size is 2"));
		auto it{ arrayValue.begin() };
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<std::string>(it->GetValue()), true, "JsonNode first element is string"));
		RETURN_IF_FALSE(
			t.Assert(std::get<std::string>(it->GetValue()), "Bond", "JsonNode first element value is 'Bond'"));
		++it;
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(it->GetValue()), true, "JsonNode second element is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(it->GetValue()), 0.04, "JsonNode second element value is 0.04"));
	}

	{
		MSAPI::Json json{ "{\"Currency\":0.4}" };
		RETURN_IF_FALSE(t.Assert(json.Valid(), true, "Json is valid"));
		RETURN_IF_FALSE(t.Assert(json.GetKeysAndValues().size(), 1, "Json size is 1"));

		const auto* node{ json.GetValue("Currency") };
		RETURN_IF_FALSE(t.Assert(node != nullptr, true, "Key 'Currency' exists"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<double>(node->GetValue()), true, "Type of key 'Currency' is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(node->GetValue()), 0.4, "Value of key 'Currency' is 0.4"));

		MSAPI::JsonNode jsonNode{ json };
		RETURN_IF_FALSE(t.Assert(jsonNode.Valid(), true, "JsonNode is valid"));
		RETURN_IF_FALSE(
			t.Assert(std::holds_alternative<MSAPI::Json>(jsonNode.GetValue()), true, "JsonNode value is Json"));
		const auto& jsonValue{ std::get<MSAPI::Json>(jsonNode.GetValue()) };
		RETURN_IF_FALSE(t.Assert(jsonValue.GetKeysAndValues().size(), 1, "JsonNode value size is 1"));

		const auto* jsonNodePtr{ jsonValue.GetValue("Currency") };
		RETURN_IF_FALSE(t.Assert(jsonNodePtr != nullptr, true, "Key 'Currency' exists"));
		RETURN_IF_FALSE(t.Assert(
			std::holds_alternative<double>(jsonNodePtr->GetValue()), true, "Type of key 'Currency' is double"));
		RETURN_IF_FALSE(t.Assert(std::get<double>(jsonNodePtr->GetValue()), 0.4, "Value of key 'Currency' is 0.4"));
	}

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_JSON_INL