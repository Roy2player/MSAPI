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
 */

#ifndef MSAPI_BASIC_SSTRING_INL
#define MSAPI_BASIC_SSTRING_INL

#include "log.h"
#include <cstring>
#include <limits>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

template <typename Type, size_t Capacity>
concept BasicSStringConcept = Capacity > 2 && (std::is_same_v<Type, char> || std::is_same_v<Type, wchar_t>)
	&& (std::numeric_limits<size_t>::max() / sizeof(Type) >= Capacity);

/**************************
 * @brief Functional static string container.
 *
 * @note Minimum meaningful size for static string is 3: two characters and null terminator.
 *
 * @tparam Type Character type.
 * @tparam Capacity The maximum length.
 *
 * @concurrency No.
 */
template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
class BasicSString {
private:
	size_t m_size{};
	std::array<Type, Capacity> m_buffer;

public:
	/**************************
	 * @test Yes.
	 */
	FORCE_INLINE BasicSString() noexcept = default;

	/**************************
	 * @test Yes.
	 */
	FORCE_INLINE BasicSString(BasicSString&&) noexcept = default;

	/**************************
	 * @test Yes.
	 */
	FORCE_INLINE BasicSString(const BasicSString&) noexcept = default;

	/**************************
	 * @attention Copy is silently interrupted and instance is left empty if other size is greater than capacity.
	 *
	 * @test Yes.
	 */
	template <size_t OtherCapacity> FORCE_INLINE BasicSString(const BasicSString<Type, OtherCapacity>& other) noexcept;

	/**************************
	 * @attention Copy is silently interrupted and instance is left empty if other size is greater than capacity.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE explicit BasicSString(std::basic_string_view<Type> other) noexcept;

	/**************************
	 * @attention Copy is silently interrupted and previous content is preserved if other size is greater than
	 * capacity.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE BasicSString& operator=(std::basic_string_view<Type> other) noexcept;

	/**************************
	 * @attention Copy is silently interrupted and previous content is preserved if other size is greater than
	 * capacity.
	 *
	 * @test Yes.
	 */
	template <size_t OtherCapacity>
	FORCE_INLINE BasicSString& operator=(const BasicSString<Type, OtherCapacity>& other) noexcept;

	/**************************
	 * @test Yes.
	 */
	FORCE_INLINE BasicSString& operator=(const BasicSString&) noexcept = default;

	/**************************
	 * @test Yes.
	 */
	FORCE_INLINE BasicSString& operator=(BasicSString&&) noexcept = default;

	/**************************
	 * @brief Append data from view to the end of meaningful data via AppendFrom.
	 *
	 * @attention Append is silently interrupted and previous content is preserved if resulting size is greater than
	 * capacity.
	 *
	 * @param other Data to be appended.
	 *
	 * @return Reference to this instance.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE BasicSString& operator+=(const std::basic_string_view<Type> other) noexcept;

	/**************************
	 * @brief Append meaningful data of other instance to the end of meaningful data via AppendFrom.
	 *
	 * @attention Append is silently interrupted and previous content is preserved if resulting size is greater than
	 * capacity.
	 *
	 * @tparam OtherCapacity The maximum length of other instance.
	 *
	 * @param other Instance to append data from.
	 *
	 * @return Reference to this instance.
	 *
	 * @test Yes.
	 */
	template <size_t OtherCapacity>
	FORCE_INLINE BasicSString& operator+=(const BasicSString<Type, OtherCapacity>& other) noexcept;

	/**************************
	 * @test Yes.
	 */
	template <size_t OtherCapacity>
	FORCE_INLINE [[nodiscard]] bool operator==(const BasicSString<Type, OtherCapacity>& other) const noexcept;

	/**************************
	 * @test Yes.
	 */
	template <size_t OtherCapacity>
	FORCE_INLINE [[nodiscard]] bool operator!=(const BasicSString<Type, OtherCapacity>& other) const noexcept;

	/**************************
	 * @attention Meaningful data should be null terminated. Update size by internal method to correlate internal state
	 * after writing.
	 *
	 * @return Pointer to buffer, never nullptr.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] Type* GetBuffer() noexcept;

	/**************************
	 * @brief Updates size of string in buffer by looking at null terminator, resulting size is its position - 1.
	 * Capacity is taken as size if no null terminator found.
	 *
	 * @attention Can force to unconditional internal state in case if meaningful data does not take whole buffer and is
	 * not null terminated. Side effect if trash at the end of its view.
	 *
	 * @return New size of static string.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] size_t UpdateSize() noexcept;

	/**************************
	 * @brief Copy n characters, or n * sizeof(Type) bytes, from source to internal buffer and set size accordingly.
	 *
	 * @attention Does not check if null terminator is before the end.
	 *
	 * @param source Copy from.
	 * @param size Size to copy.
	 *
	 * @pre source != nullptr.
	 *
	 * @return True on copy, false if size is greater than capacity.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool CopyFrom(const Type* const source, const size_t size) noexcept;

	/**************************
	 * @brief Copy data from source to internal buffer and set size accordingly.
	 *
	 * @param view Data to be copied.
	 *
	 * @return True on copy, false if view size is greater than capacity.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool CopyFrom(const std::basic_string_view<Type> view) noexcept;

	/**************************
	 * @brief Copy n characters, or n * sizeof(Type) bytes, from source to the end of internal buffer and set size
	 * accordingly.
	 *
	 * @attention Does not check if null terminator is before the end.
	 *
	 * @param source Append from.
	 * @param size Size to append.
	 *
	 * @pre source != nullptr.
	 *
	 * @return True on append, false if resulting size is greater than capacity.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool AppendFrom(const Type* const source, const size_t size) noexcept;

	/**************************
	 * @brief Append data from source to the end of internal buffer and set size accordingly.
	 *
	 * @param view Data to be appended.
	 *
	 * @return True on append, false if resulting size is greater than capacity.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool AppendFrom(const std::basic_string_view<Type> view) noexcept;

	/**************************
	 * @attention Size is expected to be updated by specific method on C-style buffer writing.
	 *
	 * @return Size of meaningful data.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] size_t GetSize() const noexcept;

	/**************************
	 * @return View on meaningful data.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] std::basic_string_view<Type> Get() const noexcept;

	/**************************
	 * @return Hash of meaningful data.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] size_t Hash() const noexcept;

	/**************************
	 * @return The maximum length.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] static constexpr size_t GetCapacity() noexcept;
};

template <size_t Capacity> using SString = BasicSString<char, Capacity>;
template <size_t Capacity> using WSString = BasicSString<wchar_t, Capacity>;

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
template <size_t OtherCapacity>
FORCE_INLINE BasicSString<Type, Capacity>::BasicSString(const BasicSString<Type, OtherCapacity>& other) noexcept
{
	(void)CopyFrom(other.Get());
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE BasicSString<Type, Capacity>::BasicSString(const std::basic_string_view<Type> other) noexcept
{
	(void)CopyFrom(other);
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE BasicSString<Type, Capacity>& BasicSString<Type, Capacity>::operator=(
	const std::basic_string_view<Type> other) noexcept
{
	(void)CopyFrom(other);
	return *this;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
template <size_t OtherCapacity>
FORCE_INLINE BasicSString<Type, Capacity>& BasicSString<Type, Capacity>::operator=(
	const BasicSString<Type, OtherCapacity>& other) noexcept
{
	(void)CopyFrom(other.Get());
	return *this;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE BasicSString<Type, Capacity>& BasicSString<Type, Capacity>::operator+=(
	const std::basic_string_view<Type> other) noexcept
{
	(void)AppendFrom(other);
	return *this;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
template <size_t OtherCapacity>
FORCE_INLINE BasicSString<Type, Capacity>& BasicSString<Type, Capacity>::operator+=(
	const BasicSString<Type, OtherCapacity>& other) noexcept
{
	(void)AppendFrom(other.Get());
	return *this;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
template <size_t OtherCapacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::operator==(
	const BasicSString<Type, OtherCapacity>& other) const noexcept
{
	return Get() == other.Get();
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
template <size_t OtherCapacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::operator!=(
	const BasicSString<Type, OtherCapacity>& other) const noexcept
{
	return !(*this == other);
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] Type* BasicSString<Type, Capacity>::GetBuffer() noexcept
{
	return m_buffer.data();
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] size_t BasicSString<Type, Capacity>::UpdateSize() noexcept
{
	m_size = 0;
	auto* const data{ m_buffer.data() };
	for (; m_size < Capacity; ++m_size) {
		if (data[m_size] == Type{ '\0' }) {
			return m_size;
		}
	}

	return m_size;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::CopyFrom(
	const Type* const source, const size_t size) noexcept
{
	if (size > Capacity) [[unlikely]] {
		LOG_WARNING_NEW("Is interrupted because capacity: {} < requested copy size: {}", Capacity, size);
		return false;
	}

	(void)memcpy(m_buffer.data(), source, size * sizeof(Type));
	m_size = size;
	return true;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::CopyFrom(const std::basic_string_view<Type> view) noexcept
{
	return CopyFrom(view.data(), view.size());
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::AppendFrom(
	const Type* const source, const size_t size) noexcept
{
	if (m_size + size > Capacity) [[unlikely]] {
		LOG_WARNING_NEW("Is interrupted because capacity: {} < requested resulting size: {}", Capacity, m_size + size);
		return false;
	}

	(void)memcpy(m_buffer.data() + m_size, source, size * sizeof(Type));
	m_size += size;
	return true;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::AppendFrom(
	const std::basic_string_view<Type> view) noexcept
{
	return AppendFrom(view.data(), view.size());
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] size_t BasicSString<Type, Capacity>::GetSize() const noexcept
{
	return m_size;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] std::basic_string_view<Type> BasicSString<Type, Capacity>::Get() const noexcept
{
	return { m_buffer.data(), m_size };
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] size_t BasicSString<Type, Capacity>::Hash() const noexcept
{
	return std::hash<std::basic_string_view<Type>>{}(Get());
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] constexpr size_t BasicSString<Type, Capacity>::GetCapacity() noexcept
{
	return Capacity;
}

} // namespace MSAPI

template <typename Type, size_t Capacity, typename CharType>
	requires std::is_same_v<Type, CharType>
struct std::formatter<MSAPI::BasicSString<Type, Capacity>, CharType> {
	constexpr auto parse(std::basic_format_parse_context<CharType>& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const MSAPI::BasicSString<Type, Capacity>& string, FormatContext& ctx) const
	{
		if constexpr (std::is_same_v<CharType, char>) {
			return format_to(ctx.out(), "{}", string.Get());
		}
		else if constexpr (std::is_same_v<CharType, wchar_t>) {
			return format_to(ctx.out(), L"{}", string.Get());
		}
		else {
			static_assert(sizeof(CharType) + 1 == 0, "Unsupported type");
		}
	}
};

#endif // MSAPI_BASIC_SSTRING_INL