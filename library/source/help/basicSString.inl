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
 * @attention Meaningful data is never null terminated, null terminator is written only by explicit NullTerminate call.
 * Data can be written only by methods which report lack of space by boolean result: Copy and Concatenate. They copy
 * as much data as fits into capacity and return false if the data is truncated. Construction and assignment from other
 * instance are allowed only if its capacity is not greater, so they can not run out of space.
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
	 * @brief Copy meaningful data of other instance, which always fits because its capacity is not greater.
	 *
	 * @tparam OtherCapacity The maximum length of other instance.
	 *
	 * @param other Instance to copy data from.
	 *
	 * @test Yes.
	 */
	template <size_t OtherCapacity>
		requires(OtherCapacity <= Capacity)
	FORCE_INLINE BasicSString(const BasicSString<Type, OtherCapacity>& other) noexcept;

	/**************************
	 * @brief Copy meaningful data of other instance, which always fits because its capacity is not greater.
	 *
	 * @tparam OtherCapacity The maximum length of other instance.
	 *
	 * @param other Instance to copy data from.
	 *
	 * @return Reference to this instance.
	 *
	 * @test Yes.
	 */
	template <size_t OtherCapacity>
		requires(OtherCapacity <= Capacity)
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
	 * Only the first capacity characters are copied if size is greater than capacity.
	 *
	 * @attention Does not check if null terminator is before the end.
	 *
	 * @param source Copy from.
	 * @param size Size to copy.
	 *
	 * @pre source != nullptr.
	 *
	 * @return True if all data is copied, false if size is greater than capacity and data is truncated.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Copy(const Type* source, size_t size) noexcept;

	/**************************
	 * @brief Copy data from source to internal buffer and set size accordingly. Only the first capacity characters are
	 * copied if view size is greater than capacity.
	 *
	 * @param view Data to be copied.
	 *
	 * @return True if all data is copied, false if view size is greater than capacity and data is truncated.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Copy(std::basic_string_view<Type> view) noexcept;

	/**************************
	 * @brief Copy n characters, or n * sizeof(Type) bytes, from source to the end of meaningful data and set size
	 * accordingly. Only the characters fitting into the remaining capacity are copied if resulting size is greater than
	 * capacity.
	 *
	 * @attention Does not check if null terminator is before the end.
	 *
	 * @param source Concatenate from.
	 * @param size Size to concatenate.
	 *
	 * @pre source != nullptr.
	 *
	 * @return True if all data is concatenated, false if resulting size is greater than capacity and data is truncated.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Concatenate(const Type* source, size_t size) noexcept;

	/**************************
	 * @brief Copy data from view to the end of meaningful data and set size accordingly. Only the characters fitting
	 * into the remaining capacity are copied if resulting size is greater than capacity.
	 *
	 * @param view Data to be concatenated.
	 *
	 * @return True if all data is concatenated, false if resulting size is greater than capacity and data is truncated.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Concatenate(std::basic_string_view<Type> view) noexcept;

	/**************************
	 * @attention Size is expected to be updated by specific method on C-style buffer writing.
	 *
	 * @return Size of meaningful data.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] size_t GetSize() const noexcept;

	/**************************
	 * @brief Resets size of meaningful data to zero.
	 *
	 * @attention Buffer content is not modified.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void Clear() noexcept;

	/**************************
	 * @brief Removes the last character of meaningful data, e.g. to free space for null terminator in fully filled
	 * string.
	 *
	 * @attention Buffer content is not modified. Does nothing if there is no meaningful data.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE void PopBack() noexcept;

	/**************************
	 * @return True if there is no meaningful data, false otherwise.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool Empty() const noexcept;

	/**************************
	 * @brief Writes null terminator right after meaningful data if there is space for it.
	 *
	 * @attention Size of meaningful data is not modified. PopBack can free space for null terminator in fully filled
	 * string.
	 *
	 * @return True if null terminator is written, false if size is equal to capacity.
	 *
	 * @test Yes.
	 */
	FORCE_INLINE [[nodiscard]] bool NullTerminate() noexcept;

	/**************************
	 * @attention View is not null terminated, NullTerminate must be called explicitly before view data is used as
	 * C-style string.
	 *
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
	requires(OtherCapacity <= Capacity)
FORCE_INLINE BasicSString<Type, Capacity>::BasicSString(const BasicSString<Type, OtherCapacity>& other) noexcept
{
	// Can not be truncated, other capacity is not greater
	(void)Copy(other.Get());
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
template <size_t OtherCapacity>
	requires(OtherCapacity <= Capacity)
FORCE_INLINE BasicSString<Type, Capacity>& BasicSString<Type, Capacity>::operator=(
	const BasicSString<Type, OtherCapacity>& other) noexcept
{
	// Can not be truncated, other capacity is not greater
	(void)Copy(other.Get());
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
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::Copy(const Type* const source, const size_t size) noexcept
{
	if (size > Capacity) [[unlikely]] {
		LOG_WARNING_NEW("Is truncated because capacity: {} < requested copy size: {}", Capacity, size);
		(void)memcpy(m_buffer.data(), source, Capacity * sizeof(Type));
		m_size = Capacity;
		return false;
	}

	(void)memcpy(m_buffer.data(), source, size * sizeof(Type));
	m_size = size;
	return true;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::Copy(const std::basic_string_view<Type> view) noexcept
{
	// Empty view can hold nullptr, which must not be passed to memcpy
	if (view.empty()) [[unlikely]] {
		m_size = 0;
		return true;
	}

	return Copy(view.data(), view.size());
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::Concatenate(
	const Type* const source, const size_t size) noexcept
{
	// Compared with remaining space to avoid overflow of resulting size
	if (const auto available{ Capacity - m_size }; size > available) [[unlikely]] {
		LOG_WARNING_NEW(
			"Is truncated because capacity: {} < requested resulting size: {} + {}", Capacity, m_size, size);
		(void)memcpy(m_buffer.data() + m_size, source, available * sizeof(Type));
		m_size = Capacity;
		return false;
	}

	(void)memcpy(m_buffer.data() + m_size, source, size * sizeof(Type));
	m_size += size;
	return true;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::Concatenate(
	const std::basic_string_view<Type> view) noexcept
{
	// Empty view can hold nullptr, which must not be passed to memcpy
	if (view.empty()) [[unlikely]] {
		return true;
	}

	return Concatenate(view.data(), view.size());
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] size_t BasicSString<Type, Capacity>::GetSize() const noexcept
{
	return m_size;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE void BasicSString<Type, Capacity>::Clear() noexcept
{
	m_size = 0;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE void BasicSString<Type, Capacity>::PopBack() noexcept
{
	if (m_size == 0) [[unlikely]] {
		return;
	}

	--m_size;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::Empty() const noexcept
{
	return m_size == 0;
}

template <typename Type, size_t Capacity>
	requires BasicSStringConcept<Type, Capacity>
FORCE_INLINE [[nodiscard]] bool BasicSString<Type, Capacity>::NullTerminate() noexcept
{
	if (m_size >= Capacity) [[unlikely]] {
		return false;
	}

	m_buffer[m_size] = Type{ '\0' };
	return true;
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
	FORCE_INLINE [[nodiscard]] constexpr auto parse(std::basic_format_parse_context<CharType>& ctx)
	{
		return ctx.begin();
	}

	template <typename FormatContext>
	FORCE_INLINE [[nodiscard]] auto format(const MSAPI::BasicSString<Type, Capacity>& string, FormatContext& ctx) const
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