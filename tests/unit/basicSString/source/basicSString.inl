/**************************
 * @file        basicSString.inl
 * @date        2026-09-12
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
 *
 * @brief Scenario:
 * 1. Check aliases
 * 2.1. Check type of returned view
 * 2.2. Check default conditions
 * 2.3. Check size reflection on zero copy
 * 2.4. Check buffer reflection on zero copy
 * 2.5. Check size reflection on copy
 * 2.6. Check buffer reflection on copy
 * 2.7. Check size reflection on second copy
 * 2.8. Check buffer reflection on second copy
 * 2.9. Check size reflection on third copy
 * 2.10. Check buffer reflection on third copy
 * 2.11. Check size reflection on size update with string copied from source with null termination inside
 * 2.12. Check buffer reflection on size update with string copied from source with null termination inside
 * 2.13. Check size reflection on attempt to overflow copy from
 * 2.14. Check buffer reflection on attempt to overflow copy from
 * 2.15. Check size reflection on second attempt to overflow copy from
 * 2.16. Check buffer reflection on second attempt to overflow copy from
 * 2.17. Check size reflection on maximum copy
 * 2.18. Check buffer reflection on maximum copy
 * 2.19. Final state check
 * 3.1. Check size reflection on memcpy
 * 3.2. Check buffer reflection on memcpy
 * 3.3. Check size reflection on second memcpy with resulting size update
 * 3.4. Check buffer reflection on second memcpy with resulting size update
 * 3.5. Check size reflection on maximum memcpy
 * 3.6. Check buffer reflection on maximum memcpy
 * 3.7. Final state check
 * 4. Null terminator at the first and last positions
 * 5.1. Copy constructor
 * 5.2. Move constructor
 * 5.3. Copy assignment constructor
 * 5.4. Move assignment constructor
 * 6. String view assignment
 * 6.1. String view constructor
 * 7.1. Append via operator+= with a string view
 * 7.2. Append via operator+= with another instance of same capacity
 * 7.3. Append via operator+= with an instance of different capacity
 * 7.4. Append via operator+= overflow preserves destination
 * 8. Different capacity construction and assignment
 * 9. Equality and inequality
 * 10. Hash
 * 11. Different capacity comparison
 * 12. Formatter
 */

#ifndef MSAPI_UNIT_TEST_BASIC_SSTRING_INL
#define MSAPI_UNIT_TEST_BASIC_SSTRING_INL

#include "../../../../library/source/help/basicSString.inl"
#include "../../../../library/source/test/test.inl"

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for BasicSString class.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool BasicSString();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool BasicSString()
{
	LOG_INFO("MSAPI UNIT TEST Basic static string");

	// 1. Check aliases
	static_assert(std::is_same_v<MSAPI::BasicSString<char, 3>, MSAPI::SString<3>>, "SString typename is expected");
	static_assert(std::is_same_v<MSAPI::BasicSString<wchar_t, 3>, MSAPI::WSString<3>>, "WSString typename is expected");

	MSAPI::Test::Test t;

	const auto checkType{ [&t]<typename Type> [[nodiscard]] () {
		const std::array<Type, 41> source{ []() {
			if constexpr (std::is_same_v<Type, char>) {
				return std::to_array("12345678901011121314151617181920\0 212123");
			}
			else if constexpr (std::is_same_v<Type, wchar_t>) {
				return std::to_array(L"12345678901011121314151617181920\0 212123");
			}
			else {
				static_assert(sizeof(Type) + 1 == 0, "Unsupported type");
			}
		}() };

		{
			// 2.1. Check type of returned view
			MSAPI::BasicSString<Type, 41> sstring41;
			const auto* const bufferPtr{ sstring41.GetBuffer() };
			auto value{ sstring41.Get() };
			RETURN_IF_FALSE(t.Assert(std::is_same_v<std::decay_t<decltype(value)>, std::basic_string_view<Type>>, true,
				"Type of view is expected"));

			// 2.2. Check default conditions
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 0, "Size is expected"));
			RETURN_IF_FALSE(t.Assert(value.size(), 0, "Size of value is expected"));

			// 2.3. Check size reflection on zero copy
			RETURN_IF_FALSE(t.Assert(sstring41.CopyFrom(source.data(), 0), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 0, "Size after copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetCapacity(), 41, "Capacity is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.4. Check buffer reflection on zero copy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 0, "Size of value is expected"));

			// 2.5. Check size reflection on copy
			RETURN_IF_FALSE(t.Assert(sstring41.CopyFrom(source.data(), 3), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 3, "Size after copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.6. Check buffer reflection on copy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 3, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 3 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.7. Check size reflection on second copy
			RETURN_IF_FALSE(t.Assert(
				sstring41.CopyFrom(std::basic_string_view<Type>{ source.data(), 31 }), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 31, "Size after copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.8. Check buffer reflection on second copy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 31, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 31 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.9. Check size reflection on third copy
			RETURN_IF_FALSE(t.Assert(
				sstring41.CopyFrom(std::basic_string_view<Type>{ source.data(), 33 }), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 33, "Size after copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.10. Check buffer reflection on third copy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 33, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 33 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.11. Check size reflection on size update with string copied from source with null termination inside
			RETURN_IF_FALSE(t.Assert(sstring41.UpdateSize(), 32, "Size after update is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 32, "Size after update is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.12. Check buffer reflection on size update with string copied from source with null termination inside
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 32, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 32 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.13. Check size reflection on attempt to overflow copy from
			RETURN_IF_FALSE(
				t.Assert(sstring41.CopyFrom(std::basic_string_view<Type>{ source.data(), source.size() + 1 }), false,
					"Copy is failed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 32, "Size after failed copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.14. Check buffer reflection on attempt to overflow copy from
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 32, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 32 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.15. Check size reflection on second attempt to overflow copy from
			RETURN_IF_FALSE(t.Assert(sstring41.CopyFrom(source.data(), source.size() + 1), false, "Copy is failed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 32, "Size after failed copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.16. Check buffer reflection on second attempt to overflow copy from
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 32, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 32 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.17. Check size reflection on maximum copy
			RETURN_IF_FALSE(t.Assert(sstring41.CopyFrom(source.data(), source.size()), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), source.size(), "Size after copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.18. Check buffer reflection on maximum copy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), source.size(), "Size of value is expected"));
			RETURN_IF_FALSE(t.Assert(
				memcmp(value.data(), source.data(), source.size() * sizeof(Type)), 0, "Content of value is expected"));

			// 2.19. Final state check
			RETURN_IF_FALSE(t.Assert(sstring41.GetCapacity(), 41, "Capacity is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));
		}

		{
			// 3.1. Check size reflection on memcpy
			MSAPI::BasicSString<Type, 41> sstring41;
			const auto* const bufferPtr{ sstring41.GetBuffer() };
			(void)memcpy(sstring41.GetBuffer(), source.data(), 33 * sizeof(Type));
			RETURN_IF_FALSE(t.Assert(sstring41.UpdateSize(), 32, "Size after update is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 32, "Size after update is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetCapacity(), 41, "Capacity is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 3.2. Check buffer reflection on memcpy
			auto value{ sstring41.Get() };
			RETURN_IF_FALSE(t.Assert(value.size(), 32, "Size of value is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(value.data(), source.data(), 3 * sizeof(Type)), 0,
				"Content of copied part of value is expected"));

			// 3.3. Check size reflection on second memcpy with resulting size update
			(void)memcpy(sstring41.GetBuffer(), source.data(), 33 * sizeof(Type));
			RETURN_IF_FALSE(t.Assert(sstring41.UpdateSize(), 32, "Size after update is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 32, "Size after update is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 3.4. Check buffer reflection on second memcpy with resulting size update
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 32, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 32 * sizeof(Type)), 0, "Content of value is expected"));

			// 3.5. Check size reflection on maximum memcpy
			(void)memcpy(sstring41.GetBuffer(), source.data(), source.size() * sizeof(Type));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 32, "Size after copy is expected"));

			// 3.6. Check buffer reflection on maximum memcpy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 32, "Size of value is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(value.data(), source.data(), source.size() * sizeof(Type)), 0,
				"Content of copied part of value is expected"));

			// 3.7. Final state check
			RETURN_IF_FALSE(t.Assert(sstring41.GetCapacity(), 41, "Capacity is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));
		}

		// 4. Null terminator at the first and last positions
		{
			MSAPI::BasicSString<Type, 41> firstTerminatedString;
			firstTerminatedString.GetBuffer()[0] = Type{ '\0' };
			RETURN_IF_FALSE(
				t.Assert(firstTerminatedString.UpdateSize(), 0, "First-position null terminator size is expected"));
			RETURN_IF_FALSE(
				t.Assert(firstTerminatedString.GetSize(), 0, "First-position null terminator stored size is expected"));

			MSAPI::BasicSString<Type, 41> lastTerminatedString;
			for (size_t index{}; index < lastTerminatedString.GetCapacity() - 1; ++index) {
				lastTerminatedString.GetBuffer()[index] = Type{ '1' };
			}
			lastTerminatedString.GetBuffer()[lastTerminatedString.GetCapacity() - 1] = Type{ '\0' };
			RETURN_IF_FALSE(
				t.Assert(lastTerminatedString.UpdateSize(), 40, "Last-position null terminator size is expected"));
			RETURN_IF_FALSE(
				t.Assert(lastTerminatedString.GetSize(), 40, "Last-position null terminator stored size is expected"));
		}

		// 5.1. Copy constructor
		{
			MSAPI::BasicSString<Type, 41> sourceString;
			RETURN_IF_FALSE(
				t.Assert(sourceString.CopyFrom(source.data(), 31), true, "Copy constructor source copy is success"));

			auto copiedString{ sourceString };
			RETURN_IF_FALSE(
				t.Assert(copiedString.GetSize(), sourceString.GetSize(), "Copy constructor size is expected"));
			RETURN_IF_FALSE(t.Assert(
				copiedString.GetCapacity(), sourceString.GetCapacity(), "Copy constructor capacity is expected"));
			RETURN_IF_FALSE(t.Assert(
				copiedString.GetBuffer() != sourceString.GetBuffer(), true, "Copy constructor buffer is independent"));
			RETURN_IF_FALSE(t.Assert(
				memcmp(copiedString.Get().data(), sourceString.Get().data(), sourceString.GetSize() * sizeof(Type)), 0,
				"Copy constructor content is expected"));
		}

		// 5.2. Move constructor
		{
			MSAPI::BasicSString<Type, 41> sourceString;
			RETURN_IF_FALSE(
				t.Assert(sourceString.CopyFrom(source.data(), 31), true, "Move constructor source copy is success"));

			auto movedString{ std::move(sourceString) };
			RETURN_IF_FALSE(t.Assert(movedString.GetSize(), 31, "Move constructor size is expected"));
			RETURN_IF_FALSE(t.Assert(movedString.GetCapacity(), 41, "Move constructor capacity is expected"));
			RETURN_IF_FALSE(t.Assert(
				movedString.GetBuffer() != sourceString.GetBuffer(), true, "Move constructor buffer is independent"));
			RETURN_IF_FALSE(t.Assert(memcmp(movedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"Move constructor content is expected"));
		}

		// 5.3. Copy assignment constructor
		{
			MSAPI::BasicSString<Type, 41> sourceString;
			RETURN_IF_FALSE(
				t.Assert(sourceString.CopyFrom(source.data(), 31), true, "Copy assignment source copy is success"));
			MSAPI::BasicSString<Type, 41> assignedString;
			assignedString = sourceString;

			RETURN_IF_FALSE(
				t.Assert(assignedString.GetSize(), sourceString.GetSize(), "Copy assignment size is expected"));
			RETURN_IF_FALSE(t.Assert(
				assignedString.GetCapacity(), sourceString.GetCapacity(), "Copy assignment capacity is expected"));
			RETURN_IF_FALSE(t.Assert(
				assignedString.GetBuffer() != sourceString.GetBuffer(), true, "Copy assignment buffer is independent"));
			RETURN_IF_FALSE(t.Assert(
				memcmp(assignedString.Get().data(), sourceString.Get().data(), sourceString.GetSize() * sizeof(Type)),
				0, "Copy assignment content is expected"));
		}

		// 5.4. Move assignment constructor
		{
			MSAPI::BasicSString<Type, 41> sourceString;
			RETURN_IF_FALSE(
				t.Assert(sourceString.CopyFrom(source.data(), 31), true, "Move assignment source copy is success"));
			MSAPI::BasicSString<Type, 41> assignedString;
			assignedString = std::move(sourceString);

			RETURN_IF_FALSE(t.Assert(assignedString.GetSize(), 31, "Move assignment size is expected"));
			RETURN_IF_FALSE(t.Assert(assignedString.GetCapacity(), 41, "Move assignment capacity is expected"));
			RETURN_IF_FALSE(t.Assert(
				assignedString.GetBuffer() != sourceString.GetBuffer(), true, "Move assignment buffer is independent"));
			RETURN_IF_FALSE(t.Assert(memcmp(assignedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"Move assignment content is expected"));
		}

		// 6. String view assignment
		{
			MSAPI::BasicSString<Type, 41> assignedString;
			assignedString = std::basic_string_view<Type>{ source.data(), 31 };

			RETURN_IF_FALSE(t.Assert(assignedString.GetSize(), 31, "String view assignment size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(assignedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"String view assignment content is expected"));
		}

		// 6.1. String view constructor
		{
			const MSAPI::BasicSString<Type, 41> constructedString{ std::basic_string_view<Type>{ source.data(), 31 } };

			RETURN_IF_FALSE(t.Assert(constructedString.GetSize(), 31, "String view constructor size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(constructedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"String view constructor content is expected"));
		}

		// 7.1. Append via operator+= with a string view
		{
			MSAPI::BasicSString<Type, 41> appendedString;
			appendedString = std::basic_string_view<Type>{ source.data(), 3 };
			appendedString += std::basic_string_view<Type>{ source.data(), 31 };

			RETURN_IF_FALSE(t.Assert(appendedString.GetSize(), 34, "String view append size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(appendedString.Get().data(), source.data(), 3 * sizeof(Type)), 0,
				"String view append preserves existing content"));
			RETURN_IF_FALSE(t.Assert(memcmp(appendedString.Get().data() + 3, source.data(), 31 * sizeof(Type)), 0,
				"String view append content is expected"));
		}

		// 7.2. Append via operator+= with another instance of same capacity
		{
			MSAPI::BasicSString<Type, 41> appendedString;
			appendedString = std::basic_string_view<Type>{ source.data(), 3 };
			MSAPI::BasicSString<Type, 41> otherString;
			otherString = std::basic_string_view<Type>{ source.data(), 31 };
			appendedString += otherString;

			RETURN_IF_FALSE(t.Assert(appendedString.GetSize(), 34, "Same capacity append size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(appendedString.Get().data() + 3, source.data(), 31 * sizeof(Type)), 0,
				"Same capacity append content is expected"));
		}

		// 7.3. Append via operator+= with an instance of different capacity
		{
			MSAPI::BasicSString<Type, 41> appendedString;
			appendedString = std::basic_string_view<Type>{ source.data(), 3 };
			MSAPI::BasicSString<Type, 42> otherString;
			otherString = std::basic_string_view<Type>{ source.data(), 31 };
			appendedString += otherString;

			RETURN_IF_FALSE(t.Assert(appendedString.GetSize(), 34, "Different capacity append size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(appendedString.Get().data() + 3, source.data(), 31 * sizeof(Type)), 0,
				"Different capacity append content is expected"));
		}

		// 7.4. Append via operator+= overflow preserves destination
		{
			MSAPI::BasicSString<Type, 32> appendedString;
			appendedString = std::basic_string_view<Type>{ source.data(), 3 };
			MSAPI::BasicSString<Type, 41> otherString;
			otherString = std::basic_string_view<Type>{ source.data(), 31 };
			appendedString += otherString;

			RETURN_IF_FALSE(t.Assert(appendedString.GetSize(), 3, "Overflow append preserves destination size"));
			RETURN_IF_FALSE(t.Assert(memcmp(appendedString.Get().data(), source.data(), 3 * sizeof(Type)), 0,
				"Overflow append preserves destination content"));
		}

		// 8. Different capacity construction and assignment
		{
			MSAPI::BasicSString<Type, 41> sourceString;
			sourceString = std::basic_string_view<Type>{ source.data(), 31 };

			const MSAPI::BasicSString<Type, 42> constructedString{ sourceString };
			RETURN_IF_FALSE(
				t.Assert(constructedString.GetSize(), 31, "Different capacity constructor size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(constructedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"Different capacity constructor content is expected"));

			MSAPI::BasicSString<Type, 42> assignedString;
			assignedString = sourceString;
			RETURN_IF_FALSE(t.Assert(assignedString.GetSize(), 31, "Different capacity assignment size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(assignedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"Different capacity assignment content is expected"));

			const MSAPI::BasicSString<Type, 41> reverseConstructedString{ assignedString };
			RETURN_IF_FALSE(
				t.Assert(reverseConstructedString.GetSize(), 31, "Reverse capacity constructor size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(reverseConstructedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"Reverse capacity constructor content is expected"));

			MSAPI::BasicSString<Type, 41> reverseAssignedString;
			reverseAssignedString = assignedString;
			RETURN_IF_FALSE(
				t.Assert(reverseAssignedString.GetSize(), 31, "Reverse capacity assignment size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(reverseAssignedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"Reverse capacity assignment content is expected"));

			MSAPI::BasicSString<Type, 42> oversizedSourceString;
			oversizedSourceString = std::basic_string_view<Type>{ source.data(), 33 };
			MSAPI::BasicSString<Type, 32> overflowAssignedString;
			overflowAssignedString = std::basic_string_view<Type>{ source.data(), 3 };
			overflowAssignedString = oversizedSourceString;
			RETURN_IF_FALSE(t.Assert(
				overflowAssignedString.GetSize(), 3, "Different capacity overflow assignment preserves destination"));
		}

		// 9. Equality and inequality
		{
			MSAPI::BasicSString<Type, 41> firstString;
			MSAPI::BasicSString<Type, 41> secondString;
			firstString = std::basic_string_view<Type>{ source.data(), 31 };
			secondString = std::basic_string_view<Type>{ source.data(), 31 };

			RETURN_IF_FALSE(t.Assert(firstString == secondString, true, "Equal strings are equal"));
			RETURN_IF_FALSE(t.Assert(firstString != secondString, false, "Equal strings are not different"));
			secondString = std::basic_string_view<Type>{ source.data(), 30 };
			RETURN_IF_FALSE(t.Assert(firstString == secondString, false, "Different strings are not equal"));
			RETURN_IF_FALSE(t.Assert(firstString != secondString, true, "Different strings are different"));
		}

		// 10. Hash
		{
			MSAPI::BasicSString<Type, 41> hashedString;
			hashedString = std::basic_string_view<Type>{ source.data(), 31 };

			RETURN_IF_FALSE(t.Assert(hashedString.Hash(), std::hash<std::basic_string_view<Type>>{}(hashedString.Get()),
				"Hash is based on string view"));
		}

		// 11. Different capacity comparison
		{
			MSAPI::BasicSString<Type, 41> smallerString;
			MSAPI::BasicSString<Type, 42> largerString;
			smallerString = std::basic_string_view<Type>{ source.data(), 31 };
			largerString = std::basic_string_view<Type>{ source.data(), 31 };

			RETURN_IF_FALSE(t.Assert(smallerString == largerString, true, "Different capacities are equal"));
			RETURN_IF_FALSE(t.Assert(largerString == smallerString, true, "Reverse different capacities are equal"));
			RETURN_IF_FALSE(
				t.Assert(smallerString != largerString, false, "Equal different capacities are not different"));
			largerString = std::basic_string_view<Type>{ source.data(), 30 };
			RETURN_IF_FALSE(t.Assert(smallerString == largerString, false, "Different capacities are not equal"));
			RETURN_IF_FALSE(t.Assert(smallerString != largerString, true, "Different capacities are different"));
		}

		// 12. Formatter
		{
			MSAPI::BasicSString<Type, 41> formattedString;
			formattedString = std::basic_string_view<Type>{ source.data(), 31 };

			if constexpr (std::is_same_v<Type, char>) {
				const auto formatResult{ std::format("{}", formattedString) };
				RETURN_IF_FALSE(
					t.Assert(memcmp(formatResult.data(), source.data(), 31), 0, "Char string formatting is expected"));
			}
			else if constexpr (std::is_same_v<Type, wchar_t>) {
				const auto formatResult{ std::format(L"{}", formattedString) };
				RETURN_IF_FALSE(
					t.Assert(memcmp(formatResult.data(), source.data(), 31), 0, "Wide string formatting is expected"));
			}
			else {
				static_assert(sizeof(Type) + 1 == 0, "Unsupported type");
			}
		}

		return true;
	} };

	RETURN_IF_FALSE(checkType.operator()<char>());
	RETURN_IF_FALSE(checkType.operator()<wchar_t>());

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_BASIC_SSTRING_INL