/**************************
 * @file        autoClearPtr.inl
 * @version     6.0
 * @date        2023-11-27
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

#ifndef MSAPI_AUTO_CLEAR_PTR_INL
#define MSAPI_AUTO_CLEAR_PTR_INL

#include "log.h"
#include <cstring>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

template <typename T>
concept AutoClearPtrT = !std::is_pointer_v<T> && !std::is_reference_v<T> && !std::is_array_v<T>;

/**************************
 * @brief RAII owner of pointer on allocated memory with error logging, const and non-const accessors and reallocation
 * possibility.
 */
template <AutoClearPtrT T> struct AutoClearPtr {
private:
	T* m_ptr{ nullptr };

public:
	/**************************
	 * @brief Construct a new Auto Clear Ptr object, allocate memory for non-void template class object.
	 *
	 * @attention On bad allocation pointer will be nullptr.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE AutoClearPtr();

	/**************************
	 * @brief Construct a new Auto Clear Ptr object.
	 *
	 * @attention On bad allocation pointer will be nullptr.
	 *
	 * @param size Number of bytes for allocating, default is size of template class object.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE AutoClearPtr(uint64_t size);

	/**************************
	 * @brief Construct a new Auto Clear Ptr object.
	 *
	 * @param ptr Pointer to already allocated memory.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE AutoClearPtr(T* ptr) noexcept;

	AutoClearPtr(const AutoClearPtr&) = delete;
	AutoClearPtr& operator=(const AutoClearPtr&) = delete;

	/**************************
	 * @brief Move ptr from object and set nullptr back.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE AutoClearPtr(AutoClearPtr&& other) noexcept;

	/**************************
	 * @brief Free memory if allocated, move pointer from object and set nullptr back.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE AutoClearPtr& operator=(AutoClearPtr&& other) noexcept;

	/**************************
	 * @brief Destroy the Auto Clear Ptr object and free memory.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE ~AutoClearPtr() noexcept;

	/**************************
	 * @brief Reallocate buffer to new size, if no success pointer to previous buffer is used.
	 *
	 * @param newSize New size to be reallocated.
	 *
	 * @return Pointer to reallocated memory or nullptr.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] T* Realloc(uint64_t newSize);

	/**************************
	 * @return Pointer to memory.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] T* Get() noexcept;

	/**************************
	 * @return Const pointer to memory.
	 *
	 * @test Add unit test.
	 */
	FORCE_INLINE [[nodiscard]] const T* Get() const noexcept;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

template <AutoClearPtrT T> FORCE_INLINE AutoClearPtr<T>::AutoClearPtr()
{
	if constexpr (!std::is_same_v<T, void>) {
		m_ptr = static_cast<T*>(malloc(sizeof(T)));
		if (m_ptr == nullptr) [[unlikely]] {
			LOG_ERROR_NEW("Bad malloc of {}. Error №{}: {}", sizeof(T), errno, std::strerror(errno));
		}
	}
}

template <AutoClearPtrT T> FORCE_INLINE AutoClearPtr<T>::AutoClearPtr(const uint64_t size)
{
	m_ptr = static_cast<T*>(malloc(size));
	if (m_ptr == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("Bad malloc of {}. Error №{}: {}", size, errno, std::strerror(errno));
	}
};

template <AutoClearPtrT T>
FORCE_INLINE AutoClearPtr<T>::AutoClearPtr(T* ptr) noexcept
	: m_ptr(ptr)
{
}

template <AutoClearPtrT T>
FORCE_INLINE AutoClearPtr<T>::AutoClearPtr(AutoClearPtr<T>&& other) noexcept
	: m_ptr{ other.m_ptr }
{
	other.m_ptr = nullptr;
}

template <AutoClearPtrT T> FORCE_INLINE AutoClearPtr<T>& AutoClearPtr<T>::operator=(AutoClearPtr<T>&& other) noexcept
{
	if (m_ptr != nullptr) {
		free(m_ptr);
	}

	m_ptr = other.m_ptr;
	other.m_ptr = nullptr;
	return *this;
}

template <AutoClearPtrT T> FORCE_INLINE AutoClearPtr<T>::~AutoClearPtr() noexcept
{
	if (m_ptr != nullptr) {
		free(m_ptr);
	}
}

template <AutoClearPtrT T> FORCE_INLINE [[nodiscard]] T* AutoClearPtr<T>::Realloc(const uint64_t newSize)
{
	auto* const newPtr{ static_cast<T*>(realloc(m_ptr, newSize)) };
	if (newPtr == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("Bad realloc to {}. Error №{}: {}", newSize, errno, std::strerror(errno));
		return nullptr;
	}

	m_ptr = newPtr;
	return newPtr;
}

template <AutoClearPtrT T> FORCE_INLINE [[nodiscard]] T* AutoClearPtr<T>::Get() noexcept { return m_ptr; }

template <AutoClearPtrT T> FORCE_INLINE [[nodiscard]] const T* AutoClearPtr<T>::Get() const noexcept { return m_ptr; }

} // namespace MSAPI

#endif // MSAPI_AUTO_CLEAR_PTR_INL