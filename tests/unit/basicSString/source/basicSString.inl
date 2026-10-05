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
 * 2.13. Check size reflection on truncated overflow copy from view
 * 2.14. Check buffer reflection on truncated overflow copy from view
 * 2.15. Check size reflection on truncated overflow copy from pointer
 * 2.16. Check buffer reflection on truncated overflow copy from pointer
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
 * 6. Implicit operations which can run out of space are not available
 * 7.1. Concatenate with a string view
 * 7.2. Concatenate with another instance of same capacity
 * 7.3. Concatenate with an instance of different capacity
 * 7.4. Concatenate overflow copies as much as possible
 * 7.5. Concatenate to full string
 * 7.6. Concatenate overflow from pointer
 * 7.7. Copy and Concatenate with empty default view
 * 8. Different capacity construction and assignment
 * 9. Equality and inequality
 * 10. Hash
 * 11. Different capacity comparison
 * 12. Formatter
 * 13. Clear and Empty
 * 14. Null terminate
 * 15. Pop back
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
 * @brief Checks if value can be appended to string by operator+=. Template parameters keep the expression dependent, so
 * missing operator results in false instead of compilation error.
 *
 * @tparam String Type of string.
 * @tparam Value Type of appended value.
 */
template <typename String, typename Value>
concept AppendableByOperator = requires(String string, Value value) { string += value; };

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
			RETURN_IF_FALSE(t.Assert(sstring41.Copy(source.data(), 0), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 0, "Size after copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetCapacity(), 41, "Capacity is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.4. Check buffer reflection on zero copy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 0, "Size of value is expected"));

			// 2.5. Check size reflection on copy
			RETURN_IF_FALSE(t.Assert(sstring41.Copy(source.data(), 3), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 3, "Size after copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.6. Check buffer reflection on copy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 3, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 3 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.7. Check size reflection on second copy
			RETURN_IF_FALSE(
				t.Assert(sstring41.Copy(std::basic_string_view<Type>{ source.data(), 31 }), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 31, "Size after copy is expected"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.8. Check buffer reflection on second copy
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 31, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 31 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.9. Check size reflection on third copy
			RETURN_IF_FALSE(
				t.Assert(sstring41.Copy(std::basic_string_view<Type>{ source.data(), 33 }), true, "Copy is sucsseed"));
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

			// 2.13. Check size reflection on truncated overflow copy from view
			RETURN_IF_FALSE(t.Assert(sstring41.Copy(std::basic_string_view<Type>{ source.data(), source.size() + 1 }),
				false, "Overflow copy reports truncation"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 41, "Size after truncated copy is capacity"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.14. Check buffer reflection on truncated overflow copy from view
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 41, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 41 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.15. Check size reflection on truncated overflow copy from pointer
			RETURN_IF_FALSE(t.Assert(sstring41.Copy(source.data(), 3), true, "Copy is sucsseed"));
			RETURN_IF_FALSE(
				t.Assert(sstring41.Copy(source.data(), source.size() + 1), false, "Overflow copy reports truncation"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetSize(), 41, "Size after truncated copy is capacity"));
			RETURN_IF_FALSE(t.Assert(sstring41.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			// 2.16. Check buffer reflection on truncated overflow copy from pointer
			value = sstring41.Get();
			RETURN_IF_FALSE(t.Assert(value.size(), 41, "Size of value is expected"));
			RETURN_IF_FALSE(
				t.Assert(memcmp(value.data(), source.data(), 41 * sizeof(Type)), 0, "Content of value is expected"));

			// 2.17. Check size reflection on maximum copy
			RETURN_IF_FALSE(t.Assert(sstring41.Copy(source.data(), source.size()), true, "Copy is sucsseed"));
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
				t.Assert(sourceString.Copy(source.data(), 31), true, "Copy constructor source copy is success"));

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
				t.Assert(sourceString.Copy(source.data(), 31), true, "Move constructor source copy is success"));

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
				t.Assert(sourceString.Copy(source.data(), 31), true, "Copy assignment source copy is success"));
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
				t.Assert(sourceString.Copy(source.data(), 31), true, "Move assignment source copy is success"));
			MSAPI::BasicSString<Type, 41> assignedString;
			assignedString = std::move(sourceString);

			RETURN_IF_FALSE(t.Assert(assignedString.GetSize(), 31, "Move assignment size is expected"));
			RETURN_IF_FALSE(t.Assert(assignedString.GetCapacity(), 41, "Move assignment capacity is expected"));
			RETURN_IF_FALSE(t.Assert(
				assignedString.GetBuffer() != sourceString.GetBuffer(), true, "Move assignment buffer is independent"));
			RETURN_IF_FALSE(t.Assert(memcmp(assignedString.Get().data(), source.data(), 31 * sizeof(Type)), 0,
				"Move assignment content is expected"));
		}

		// 6. Implicit operations which can run out of space are not available
		{
			using SString41 = MSAPI::BasicSString<Type, 41>;
			using SString42 = MSAPI::BasicSString<Type, 42>;
			using View = std::basic_string_view<Type>;

			static_assert(!std::is_constructible_v<SString41, View>, "Construction from view is not available");
			static_assert(!std::is_assignable_v<SString41&, View>, "Assignment from view is not available");
			static_assert(!std::is_constructible_v<SString41, const SString42&>,
				"Construction from greater capacity is not available");
			static_assert(!std::is_assignable_v<SString41&, const SString42&>,
				"Assignment from greater capacity is not available");
			static_assert(std::is_constructible_v<SString42, const SString41&>, "Construction from smaller capacity");
			static_assert(std::is_assignable_v<SString42&, const SString41&>, "Assignment from smaller capacity");
			static_assert(AppendableByOperator<std::basic_string<Type>, View>, "Concept detects existing operator");
			static_assert(!AppendableByOperator<SString41, View>, "Append operator with view is not available");
			static_assert(
				!AppendableByOperator<SString41, SString41>, "Append operator with same capacity is not available");
			static_assert(!AppendableByOperator<SString41, SString42>,
				"Append operator with different capacity is not available");
		}

		// 7.1. Concatenate with a string view
		{
			MSAPI::BasicSString<Type, 41> concatenatedString;
			RETURN_IF_FALSE(t.Assert(concatenatedString.Copy(source.data(), 3), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(concatenatedString.Concatenate(std::basic_string_view<Type>{ source.data(), 31 }),
				true, "String view concatenation is success"));

			RETURN_IF_FALSE(t.Assert(concatenatedString.GetSize(), 34, "String view concatenation size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(concatenatedString.Get().data(), source.data(), 3 * sizeof(Type)), 0,
				"String view concatenation preserves existing content"));
			RETURN_IF_FALSE(t.Assert(memcmp(concatenatedString.Get().data() + 3, source.data(), 31 * sizeof(Type)), 0,
				"String view concatenation content is expected"));
		}

		// 7.2. Concatenate with another instance of same capacity
		{
			MSAPI::BasicSString<Type, 41> concatenatedString;
			RETURN_IF_FALSE(t.Assert(concatenatedString.Copy(source.data(), 3), true, "Copy is success"));
			MSAPI::BasicSString<Type, 41> otherString;
			RETURN_IF_FALSE(t.Assert(otherString.Copy(source.data(), 31), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(
				concatenatedString.Concatenate(otherString.Get()), true, "Same capacity concatenation is success"));

			RETURN_IF_FALSE(t.Assert(concatenatedString.GetSize(), 34, "Same capacity concatenation size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(concatenatedString.Get().data() + 3, source.data(), 31 * sizeof(Type)), 0,
				"Same capacity concatenation content is expected"));
		}

		// 7.3. Concatenate with an instance of different capacity
		{
			MSAPI::BasicSString<Type, 41> concatenatedString;
			RETURN_IF_FALSE(t.Assert(concatenatedString.Copy(source.data(), 3), true, "Copy is success"));
			MSAPI::BasicSString<Type, 42> otherString;
			RETURN_IF_FALSE(t.Assert(otherString.Copy(source.data(), 31), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(concatenatedString.Concatenate(otherString.Get()), true,
				"Different capacity concatenation is success"));

			RETURN_IF_FALSE(
				t.Assert(concatenatedString.GetSize(), 34, "Different capacity concatenation size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(concatenatedString.Get().data() + 3, source.data(), 31 * sizeof(Type)), 0,
				"Different capacity concatenation content is expected"));
		}

		// 7.4. Concatenate overflow copies as much as possible
		{
			MSAPI::BasicSString<Type, 32> concatenatedString;
			RETURN_IF_FALSE(t.Assert(concatenatedString.Copy(source.data(), 3), true, "Copy is success"));
			MSAPI::BasicSString<Type, 41> otherString;
			RETURN_IF_FALSE(t.Assert(otherString.Copy(source.data(), 31), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(
				concatenatedString.Concatenate(otherString.Get()), false, "Overflow concatenation reports truncation"));

			RETURN_IF_FALSE(t.Assert(concatenatedString.GetSize(), 32, "Overflow concatenation size is capacity"));
			RETURN_IF_FALSE(t.Assert(memcmp(concatenatedString.Get().data(), source.data(), 3 * sizeof(Type)), 0,
				"Overflow concatenation preserves existing content"));
			RETURN_IF_FALSE(t.Assert(memcmp(concatenatedString.Get().data() + 3, source.data(), 29 * sizeof(Type)), 0,
				"Overflow concatenation copies fitting part"));
		}

		// 7.5. Concatenate to full string
		{
			MSAPI::BasicSString<Type, 32> concatenatedString;
			RETURN_IF_FALSE(t.Assert(concatenatedString.Copy(source.data(), 32), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(concatenatedString.Concatenate(std::basic_string_view<Type>{}), true,
				"Empty concatenation to full string is success"));
			RETURN_IF_FALSE(t.Assert(concatenatedString.Concatenate(std::basic_string_view<Type>{ source.data(), 1 }),
				false, "Concatenation to full string reports truncation"));
			RETURN_IF_FALSE(t.Assert(concatenatedString.GetSize(), 32, "Full string size is not changed"));
			RETURN_IF_FALSE(t.Assert(memcmp(concatenatedString.Get().data(), source.data(), 32 * sizeof(Type)), 0,
				"Full string content is not changed"));
		}

		// 7.6. Concatenate overflow from pointer
		{
			MSAPI::BasicSString<Type, 32> concatenatedString;
			RETURN_IF_FALSE(t.Assert(concatenatedString.Concatenate(source.data(), source.size()), false,
				"Overflow concatenation from pointer reports truncation"));
			RETURN_IF_FALSE(t.Assert(concatenatedString.GetSize(), 32, "Overflow concatenation size is capacity"));
			RETURN_IF_FALSE(t.Assert(memcmp(concatenatedString.Get().data(), source.data(), 32 * sizeof(Type)), 0,
				"Overflow concatenation copies fitting part"));
		}

		// 7.7. Copy and Concatenate with empty default view
		{
			MSAPI::BasicSString<Type, 41> emptyViewString;
			RETURN_IF_FALSE(t.Assert(emptyViewString.Copy(source.data(), 3), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(emptyViewString.Concatenate(std::basic_string_view<Type>{}), true,
				"Empty default view concatenation is success"));
			RETURN_IF_FALSE(
				t.Assert(emptyViewString.GetSize(), 3, "Empty default view concatenation does not change size"));
			RETURN_IF_FALSE(t.Assert(memcmp(emptyViewString.Get().data(), source.data(), 3 * sizeof(Type)), 0,
				"Empty default view concatenation does not change content"));
			RETURN_IF_FALSE(t.Assert(
				emptyViewString.Copy(std::basic_string_view<Type>{}), true, "Empty default view copy is success"));
			RETURN_IF_FALSE(t.Assert(emptyViewString.Empty(), true, "Empty default view copy empties string"));
		}

		// 8. Different capacity construction and assignment
		{
			MSAPI::BasicSString<Type, 41> sourceString;
			RETURN_IF_FALSE(t.Assert(sourceString.Copy(source.data(), 41), true, "Copy is success"));

			const MSAPI::BasicSString<Type, 42> constructedString{ sourceString };
			RETURN_IF_FALSE(
				t.Assert(constructedString.GetSize(), 41, "Different capacity constructor size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(constructedString.Get().data(), source.data(), 41 * sizeof(Type)), 0,
				"Different capacity constructor content is expected"));

			MSAPI::BasicSString<Type, 42> assignedString;
			RETURN_IF_FALSE(t.Assert(assignedString.Copy(source.data(), 3), true, "Copy is success"));
			assignedString = sourceString;
			RETURN_IF_FALSE(t.Assert(assignedString.GetSize(), 41, "Different capacity assignment size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(assignedString.Get().data(), source.data(), 41 * sizeof(Type)), 0,
				"Different capacity assignment content is expected"));
		}

		// 9. Equality and inequality
		{
			MSAPI::BasicSString<Type, 41> firstString;
			MSAPI::BasicSString<Type, 41> secondString;
			RETURN_IF_FALSE(t.Assert(firstString.Copy(source.data(), 31), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(secondString.Copy(source.data(), 31), true, "Copy is success"));

			RETURN_IF_FALSE(t.Assert(firstString == secondString, true, "Equal strings are equal"));
			RETURN_IF_FALSE(t.Assert(firstString != secondString, false, "Equal strings are not different"));
			RETURN_IF_FALSE(t.Assert(secondString.Copy(source.data(), 30), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(firstString == secondString, false, "Different strings are not equal"));
			RETURN_IF_FALSE(t.Assert(firstString != secondString, true, "Different strings are different"));
		}

		// 10. Hash
		{
			MSAPI::BasicSString<Type, 41> hashedString;
			RETURN_IF_FALSE(t.Assert(hashedString.Copy(source.data(), 31), true, "Copy is success"));

			RETURN_IF_FALSE(t.Assert(hashedString.Hash(), std::hash<std::basic_string_view<Type>>{}(hashedString.Get()),
				"Hash is based on string view"));
		}

		// 11. Different capacity comparison
		{
			MSAPI::BasicSString<Type, 41> smallerString;
			MSAPI::BasicSString<Type, 42> largerString;
			RETURN_IF_FALSE(t.Assert(smallerString.Copy(source.data(), 31), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(largerString.Copy(source.data(), 31), true, "Copy is success"));

			RETURN_IF_FALSE(t.Assert(smallerString == largerString, true, "Different capacities are equal"));
			RETURN_IF_FALSE(t.Assert(largerString == smallerString, true, "Reverse different capacities are equal"));
			RETURN_IF_FALSE(
				t.Assert(smallerString != largerString, false, "Equal different capacities are not different"));
			RETURN_IF_FALSE(t.Assert(largerString.Copy(source.data(), 30), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(smallerString == largerString, false, "Different capacities are not equal"));
			RETURN_IF_FALSE(t.Assert(smallerString != largerString, true, "Different capacities are different"));
		}

		// 12. Formatter
		{
			MSAPI::BasicSString<Type, 41> formattedString;
			RETURN_IF_FALSE(t.Assert(formattedString.Copy(source.data(), 31), true, "Copy is success"));

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

		// 13. Clear and Empty
		{
			MSAPI::BasicSString<Type, 41> clearedString;
			const auto* const bufferPtr{ clearedString.GetBuffer() };
			RETURN_IF_FALSE(t.Assert(clearedString.Empty(), true, "Default string is empty"));

			clearedString.Clear();
			RETURN_IF_FALSE(t.Assert(clearedString.Empty(), true, "Cleared default string is empty"));
			RETURN_IF_FALSE(t.Assert(clearedString.GetSize(), 0, "Cleared default string size is expected"));

			RETURN_IF_FALSE(t.Assert(clearedString.Copy(source.data(), 31), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(clearedString.Empty(), false, "Filled string is not empty"));

			clearedString.Clear();
			RETURN_IF_FALSE(t.Assert(clearedString.Empty(), true, "Cleared string is empty"));
			RETURN_IF_FALSE(t.Assert(clearedString.GetSize(), 0, "Cleared string size is expected"));
			RETURN_IF_FALSE(t.Assert(clearedString.Get().size(), 0, "Cleared string view size is expected"));
			RETURN_IF_FALSE(t.Assert(clearedString.GetCapacity(), 41, "Cleared string capacity is expected"));
			RETURN_IF_FALSE(t.Assert(clearedString.GetBuffer(), bufferPtr, "Buffer pointer is not changed"));

			RETURN_IF_FALSE(t.Assert(clearedString.Concatenate(source.data(), 3), true, "Concatenation after clear"));
			RETURN_IF_FALSE(t.Assert(clearedString.GetSize(), 3, "Concatenation after clear size is expected"));
			RETURN_IF_FALSE(t.Assert(memcmp(clearedString.Get().data(), source.data(), 3 * sizeof(Type)), 0,
				"Concatenation after clear content is expected"));
		}

		// 14. Null terminate
		{
			MSAPI::BasicSString<Type, 41> terminatedString;
			auto* const buffer{ terminatedString.GetBuffer() };
			for (size_t index{}; index < terminatedString.GetCapacity(); ++index) {
				buffer[index] = Type{ '1' };
			}

			RETURN_IF_FALSE(t.Assert(terminatedString.NullTerminate(), true, "Empty string is null terminated"));
			RETURN_IF_FALSE(
				t.Assert(buffer[0] == Type{ '\0' }, true, "Empty string null terminator is at first position"));
			RETURN_IF_FALSE(t.Assert(terminatedString.GetSize(), 0, "Empty string size is not changed"));

			RETURN_IF_FALSE(t.Assert(terminatedString.Copy(source.data(), 3), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(terminatedString.NullTerminate(), true, "String is null terminated"));
			RETURN_IF_FALSE(
				t.Assert(buffer[3] == Type{ '\0' }, true, "Null terminator is right after meaningful data"));
			RETURN_IF_FALSE(t.Assert(buffer[4] == Type{ '1' }, true, "Data after null terminator is not changed"));
			RETURN_IF_FALSE(t.Assert(terminatedString.GetSize(), 3, "Size is not changed"));
			RETURN_IF_FALSE(t.Assert(
				memcmp(terminatedString.Get().data(), source.data(), 3 * sizeof(Type)), 0, "Content is not changed"));
			RETURN_IF_FALSE(t.Assert(terminatedString.UpdateSize(), 3, "Size update stops at null terminator"));

			RETURN_IF_FALSE(t.Assert(terminatedString.Copy(source.data(), 40), true, "Copy is success"));
			RETURN_IF_FALSE(
				t.Assert(terminatedString.NullTerminate(), true, "String is null terminated at last position"));
			RETURN_IF_FALSE(t.Assert(buffer[40] == Type{ '\0' }, true, "Null terminator is at last position"));
			RETURN_IF_FALSE(t.Assert(terminatedString.GetSize(), 40, "Size is not changed at last position"));

			for (size_t index{}; index < terminatedString.GetCapacity(); ++index) {
				buffer[index] = Type{ '1' };
			}
			RETURN_IF_FALSE(t.Assert(terminatedString.UpdateSize(), 41, "Full buffer size is expected"));
			RETURN_IF_FALSE(t.Assert(terminatedString.NullTerminate(), false, "Full string is not null terminated"));
			RETURN_IF_FALSE(t.Assert(buffer[40] == Type{ '1' }, true, "Full string last character is not changed"));
			RETURN_IF_FALSE(t.Assert(terminatedString.GetSize(), 41, "Full string size is not changed"));

			terminatedString.Clear();
			RETURN_IF_FALSE(t.Assert(terminatedString.NullTerminate(), true, "Cleared string is null terminated"));
			RETURN_IF_FALSE(
				t.Assert(buffer[0] == Type{ '\0' }, true, "Cleared string null terminator is at first position"));
		}

		// 15. Pop back
		{
			MSAPI::BasicSString<Type, 41> poppedString;
			const auto* const buffer{ poppedString.GetBuffer() };
			poppedString.PopBack();
			RETURN_IF_FALSE(t.Assert(poppedString.GetSize(), 0, "Empty string size is not changed by pop back"));

			// Last character differs from null terminator to detect buffer modifications
			const Type lastCharacter{ 'x' };
			RETURN_IF_FALSE(t.Assert(poppedString.Copy(source.data(), 40), true, "Copy is success"));
			RETURN_IF_FALSE(t.Assert(poppedString.Concatenate(&lastCharacter, 1), true, "Concatenation is success"));
			RETURN_IF_FALSE(t.Assert(poppedString.NullTerminate(), false, "Full string is not null terminated"));
			poppedString.PopBack();
			RETURN_IF_FALSE(t.Assert(poppedString.GetSize(), 40, "Popped string size is expected"));
			RETURN_IF_FALSE(t.Assert(buffer[40] == lastCharacter, true, "Popped character is not modified in buffer"));
			RETURN_IF_FALSE(t.Assert(memcmp(poppedString.Get().data(), source.data(), 40 * sizeof(Type)), 0,
				"Popped string content is expected"));
			RETURN_IF_FALSE(t.Assert(poppedString.NullTerminate(), true, "Popped string is null terminated"));
			RETURN_IF_FALSE(t.Assert(buffer[40] == Type{ '\0' }, true, "Null terminator is at last position"));

			RETURN_IF_FALSE(t.Assert(poppedString.Copy(source.data(), 1), true, "Copy is success"));
			poppedString.PopBack();
			RETURN_IF_FALSE(t.Assert(poppedString.Empty(), true, "Popped single character string is empty"));
			poppedString.PopBack();
			RETURN_IF_FALSE(t.Assert(poppedString.GetSize(), 0, "Emptied string size is not changed by pop back"));
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