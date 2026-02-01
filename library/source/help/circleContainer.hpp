/**************************
 * @file        circleContainer.h
 * @version     6.0
 * @date        2025-06-03
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

#ifndef MSAPI_CIRCLE_CONTAINER
#define MSAPI_CIRCLE_CONTAINER

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <iostream>
#include <stdio.h>
#include <thread>
#include <unistd.h>

namespace MSAPI {

/**************************
 * @brief Circle buffer container.
 *
 * @tparam T Type of buffer holder.
 * @tparam Size Number of buffer holders to be in cycle-chain.
 */
template <typename T, int_fast32_t Size> class CircleContainer {
private:
	static std::atomic<bool> m_ready;

	struct Node {
		T value;
		Node* next;
	};

public:
	class Accessor {
	private:
		Node* m_current;
		std::atomic<bool>& m_ready{ CircleContainer::m_ready };

	public:
		Accessor(CircleContainer& container)
			: m_current{ &container.m_nodes[0] }
		{
		}

		T& GetCurrent()
		{
			struct ANON {
				Node** node;
				std::atomic<bool>& ready;

				~ANON()
				{
					*node = (*node)->next;
					// 	std::cout << "--> store true" << std::endl;
					ready.store(true);
					// 	std::cout << "--> notify true" << std::endl;
					ready.notify_one();
				}
			} tmp{ &m_current, m_ready };

			// 			std::cout << "--> wait true" << std::endl;
			m_ready.wait(false);
			// 			std::cout << "--> store false" << std::endl;
			m_ready.store(false);
			// 			std::cout << "--> notify false" << std::endl;
			m_ready.notify_one();
			return m_current->value;
		};
	};

private:
	std::array<Node, Size> m_nodes;

public:
	CircleContainer()
	{
		const auto end{ Size - 1 };
		for (int_fast32_t index{ 0 }; index < end; ++index) {
			m_nodes[index].next = &m_nodes[index + 1];
		}

		m_nodes[end].next = &m_nodes[0];
	}

	//* For ability to access m_nodes and m_ready in constructor
	friend Accessor;
};

struct Buffers {
#define BUFFERS_COUNTER 1024
	static std::atomic<int_fast32_t> writesCounter;

	struct Buffer {
		void* buffer;
		int_fast16_t size;
		static const int_fast16_t pageSize;

		Buffer()
		{
			buffer = aligned_alloc(pageSize, pageSize);
			if (buffer == nullptr) [[unlikely]] {
				throw std::bad_alloc{};
			}
			// 			std::cout << "allocate at " << buffer << std::endl;
		}

		~Buffer() { free(buffer); }
	};

	Buffers() { }

	//* 4MB of memory if page size is 4KB
	CircleContainer<Buffer, BUFFERS_COUNTER> buffers;
	CircleContainer<Buffer, BUFFERS_COUNTER>::Accessor accessor{ buffers };

	void Write(const void* from, int_fast16_t size)
	{
		auto& current{ accessor.GetCurrent() };
		current.size = std::min(size, Buffer::pageSize);
		// 		std::cout << "<-- write to " << current.buffer << " " << std::string_view{ static_cast<const
		// char*>(from), size } << std::endl;
		memcpy(current.buffer, from, current.size);
		writesCounter.fetch_add(1, std::memory_order_relaxed);
	}
};

template <> std::atomic<bool> CircleContainer<Buffers::Buffer, BUFFERS_COUNTER>::m_ready = true;
std::atomic<int_fast32_t> Buffers::writesCounter = 0;
const int_fast16_t Buffers::Buffer::pageSize = 4096; // Only for tests

}; //* namespace MSAPI

#endif //* MSAPI_CIRCLE_CONTAINER