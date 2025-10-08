/**************************
 * @file        continuousAllocator.h
 * @version     6.0
 * @date        2025-05-30
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

#ifndef MSAPI_CONTINUOUS_ALLOCATOR
#define MSAPI_CONTINUOUS_ALLOCATOR

#include "log.h"
#include <atomic>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace MSAPI {

//! This is first-touch implementation and has to be polished
//! Tests for allocators are required for using in production

/**************************
 * @brief Allocator for maximizing CPU code prediction features, due to continuous memory allocations. By default
 * allocates numbers of objects which fits in page size. Can be stored only object which size is less than page size.
 * @brief Data on page stored in next principle: first 20 bytes is information about page next first half is an array of
 * nodes with their meta information and second half contains continuos buffer with objects.
 *
 * @tparam T Type of stored object.
 */
template <typename T> class ContinuousAllocator {
private:
	static inline const constexpr int_fast16_t pageSize{ 4096 };
	static inline const constexpr int_fast16_t freeNodesSize{ 100 };
	static std::atomic<bool> m_ready;

public:
	struct FreeNodes {
		int_fast32_t continuousSize;
		int_fast32_t offset;
		void* begin;

		FreeNodes(const int_fast32_t continuousSize, const int_fast32_t offset, void* begin)
			: continuousSize{ continuousSize }
			, offset{ offset }
			, begin{ begin }
		{
		}

		~FreeNodes() { }

		FreeNodes(const FreeNodes& other) = delete;
		FreeNodes(FreeNodes&& other) = delete;
		FreeNodes& operator=(const FreeNodes& other) = delete;
		FreeNodes& operator=(FreeNodes&&) = delete;

		FORCE_INLINE void UpdateBegin(void* bufferBegin) { begin = &static_cast<int8_t*>(bufferBegin)[offset]; }
	};

	using value_type = T;

private:
	int_fast64_t m_blockSize;
	int_fast64_t m_bufferSize{ pageSize };
	std::vector<FreeNodes> m_freeNodes(freeNodesSize);
	void* m_buffer;

public:
	explicit ContinuousAllocator(const int_fast64_t m_blockSize = pageSize / sizeof(T))
		: m_blockSize{ m_blockSize }
	{
		(void)AllocatePage<true>();
	}

	template <typename U> friend class ContinuousAllocator;

	template <typename U> struct rebind {
		using other = PoolAllocator<U>;
	};

	template <typename U>
	ContinuousAllocator(const ContinuousAllocator<U>& other)
		: m_blockSize{ other.m_blockSize }
	{
		DeallocateAll();
		m_currentNode = nullptr;
		m_availableNodes = 0;
	}

	~ContinuousAllocator() { DeallocateAll(); }

	FORCE_INLINE T* allocate(const int_fast64_t toAllocate)
	{
#define TMP_MSAPI_CONTINUOUS_ALLOCATOR_BLOCK_OR_WAIT                                                                   \
	struct ANON {                                                                                                      \
		std::atomic<bool>& ready;                                                                                      \
                                                                                                                       \
		~ANON()                                                                                                        \
		{                                                                                                              \
			ready.store(true);                                                                                         \
			ready.notify_one();                                                                                        \
		}                                                                                                              \
	} tmp{ m_ready };                                                                                                  \
                                                                                                                       \
	m_ready.wait(false);                                                                                               \
	m_ready.store(false);                                                                                              \
	m_ready.notify_one();

		TMP_MSAPI_CONTINUOUS_ALLOCATOR_BLOCK_OR_WAIT;

		for (auto& freeNode : m_freeNodes) {
			if (freeNode.continuousSize >= toAllocate) {
				auto* requestedBuffer{ freeNode.begin };
				freeNode.begin = &static_cast<int8_t*>(requestedBuffer)[toAllocate * sizeof(T)];
				freeNode.continuousSize -= toAllocate;
				return requestedBuffer;
			}
		}

		if (toAllocate > m_blockSize) [[unlikely]] {
			return AllocatePages(pageSize / toAllocate / sizeof(T) + 1);
		}

		return AllocatePage<false>();
	}

	FORCE_INLINE void deallocate(T* begin, int_fast64_t toDeallocate)
	{
		TMP_MSAPI_CONTINUOUS_ALLOCATOR_BLOCK_OR_WAIT;

		{
			Node* emptyFreeNode{ nullptr };
			size_t index{ 0 };
			const size_t size{ m_freeNodes.size() };
			const uintptr_t tail{ begin + toDeallocate };

			for (; index < size; ++index) {

#define TMP_MSAPI_CONTINUOUS_ALLOCATOR_LOOK_FOR_CONTINUOUS                                                             \
	auto& freeNode{ m_freeNodes[index] };                                                                              \
                                                                                                                       \
	/* Look for previous free sequence */                                                                              \
	if (freeNode.begin + freeNode.continuousSize == begin) {                                                           \
		freeNode.continuousSize += toDeallocate;                                                                       \
                                                                                                                       \
		/* Defragmentation in case if there is free node which is next */                                              \
		++index;                                                                                                       \
		for (; index < size; ++index) {                                                                                \
			auto& freeNode2{ m_freeNodes[index] };                                                                     \
                                                                                                                       \
			if (freeNode2.begin == tail) {                                                                             \
				freeNode.continuousSize += freeNode2.continuousSize;                                                   \
				freeNode2.continuousSize = 0;                                                                          \
			}                                                                                                          \
		}                                                                                                              \
                                                                                                                       \
		return;                                                                                                        \
	}                                                                                                                  \
                                                                                                                       \
	/* Look for next free sequence */                                                                                  \
	if (freeNode.begin == tail) {                                                                                      \
		freeNode.continuousSize += toDeallocate;                                                                       \
		freeNode.begin = begin;                                                                                        \
		freeNode.offset = begin - m_buffer;                                                                            \
		return;                                                                                                        \
	}

				TMP_MSAPI_CONTINUOUS_ALLOCATOR_LOOK_FOR_CONTINUOUS;

				if (freeNode.continuousSize == 0) {
					emptyFreeNode = &freeNode;
					++index;
					break;
				}
			}

			for (; index < size; ++index) {
				TMP_MSAPI_CONTINUOUS_ALLOCATOR_LOOK_FOR_CONTINUOUS;

#undef TMP_MSAPI_CONTINUOUS_ALLOCATOR_LOOK_FOR_CONTINUOUS
			}

			if (freeNode == nullptr) {
				m_freeNodes.emplace_back(toDeallocate, begin - m_buffer, begin);
			}
			else {
				freeNode.continuousSize = toDeallocate;
				freeNode.begin = begin;
				freeNode.offset = begin - m_buffer;
			}
		}
	}

private:
	FORCE_INLINE void DeallocateAll()
	{
		TMP_MSAPI_CONTINUOUS_ALLOCATOR_BLOCK_OR_WAIT;

		m_freeNodes.clear();
		free(m_buffer);
	}

	template <bool Initialization> FORCE_INLINE void* AllocatePage()
	{
		if constexpr (Initialization) {
			m_buffer = aligned_alloc(pageSize, pageSize);
			if (m_buffer == nullptr) [[unlikely]] {
				LOG_ERROR("Cannot allocate alligned memory of size: " + _S(pageSize)
					+ " bytes with alignment: " + _S(pageSize) + " for continuous allocator");
				throw std::bad_alloc{};
			}

			m_freeNodes.emplace_back(m_blockSize, 0, m_buffer);

			return nullptr;
		}
		else {

#define TMP_MSAPI_CONTINUOUS_ALLOCATOR_REALLOCATE_BUFFER(bytesAdditionally)                                            \
	TMP_MSAPI_CONTINUOUS_ALLOCATOR_BLOCK_OR_WAIT;                                                                      \
	const auto oldSize{ m_bufferSize };                                                                                \
	m_bufferSize += bytesAdditionally;                                                                                 \
	m_buffer = realloc(m_buffer, m_bufferSize);                                                                        \
	if (m_buffer == nullptr) [[unlikely]] {                                                                            \
		LOG_ERROR("Cannot reallocate memory of size: " + _S(m_bufferSize) + " bytes for continuous allocator");        \
		throw std::bad_alloc{};                                                                                        \
	}                                                                                                                  \
                                                                                                                       \
	if (static_cast<std::uintptr_t>(m_buffer) % pageSize != 0) {                                                       \
		auto* oldBuffer{ m_buffer };                                                                                   \
		m_buffer = aligned_alloc(pageSize, m_bufferSize);                                                              \
		memcpy(m_buffer, oldBuffer, oldSize);                                                                          \
		free(oldBuffer);                                                                                               \
                                                                                                                       \
		for (auto& freeNode : m_freeNodes) {                                                                           \
			freeNode.UpdateBegin(m_buffer);                                                                            \
		}                                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
	m_freeNodes.emplace_back(m_blockSize, 0, &static_cast<int8_t*>(m_buffer)[oldSize]);                                \
	return &static_cast<int8_t*>(m_buffer)[oldSize];

			TMP_MSAPI_CONTINUOUS_ALLOCATOR_REALLOCATE_BUFFER(pageSize);
		}
	}

	FORCE_INLINE void* AllocatePages(const int_fast64_t number)
	{
		TMP_MSAPI_CONTINUOUS_ALLOCATOR_REALLOCATE_BUFFER(number * pageSize);
	}

#undef TMP_MSAPI_CONTINUOUS_ALLOCATOR_BLOCK_OR_WAIT
#undef TMP_MSAPI_CONTINUOUS_ALLOCATOR_REALLOCATE_BUFFER
};

template <typename T, typename U> bool operator==(const ContinuousAllocator<T>&, const ContinuousAllocator<U>&)
{
	return true;
}

template <typename T, typename U> bool operator!=(const ContinuousAllocator<T>&, const ContinuousAllocator<U>&)
{
	return false;
}

}; //* namespace MSAPI

#endif //* MSAPI_CONTINUOUS_ALLOCATOR