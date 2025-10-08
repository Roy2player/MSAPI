/**************************
 * @file        autoClearPtr.hpp
 * @version     6.0
 * @date        2023-11-27
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

#ifndef MSAPI_AUTO_CLEAR_PTR_H
#define MSAPI_AUTO_CLEAR_PTR_H

#include <stdlib.h>
#include <type_traits>

namespace MSAPI {

/**************************
 * @brief RAII owner of pointer on allocated memory, which will be freed on destruction. Provide direct access to
 * pointer field.
 */
template <typename T>
	requires(!std::is_pointer_v<T> && !std::is_reference_v<T> && !std::is_array_v<T>)
struct AutoClearPtr {
	T* ptr{ nullptr };

	/**************************
	 * @brief Construct a new Auto Clear Ptr object, allocate memory for non-void template class object.
	 */
	AutoClearPtr()
	{
		if constexpr (!std::is_same_v<T, void>) {
			ptr = static_cast<T*>(malloc(sizeof(T)));
		}
	};

	/**************************
	 * @brief Construct a new Auto Clear Ptr object.
	 *
	 * @param size Number of bytes for allocating, default is size of template class object.
	 */
	AutoClearPtr(const size_t size) { ptr = static_cast<T*>(malloc(size)); };

	/**************************
	 * @brief Construct a new Auto Clear Ptr object.
	 *
	 * @param ptr Pointer to already allocated memory.
	 */
	AutoClearPtr(T* ptr)
		: ptr(ptr)
	{
	}

	AutoClearPtr(const AutoClearPtr&) = delete;
	AutoClearPtr& operator=(const AutoClearPtr&) = delete;

	/**************************
	 * @brief Move ptr from object and set nullptr back.
	 */
	AutoClearPtr(AutoClearPtr&& other) noexcept
		: ptr{ other.ptr }
	{
		other.ptr = nullptr;
	}

	/**************************
	 * @brief Destroy the Auto Clear Ptr object and free memory.
	 */
	~AutoClearPtr()
	{
		if (ptr != nullptr) {
			free(ptr);
		}
	}
};

}; //* namespace MSAPI

#endif //* MSAPI_AUTO_CLEAR_PTR_H