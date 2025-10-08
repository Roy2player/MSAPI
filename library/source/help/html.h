/**************************
 * @file        html.h
 * @version     6.0
 * @date        2023-12-05
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

#ifndef MSAPI_HTML_H
#define MSAPI_HTML_H

#include <string>
#include <typeinfo>
#include <vector>

namespace MSAPI {

/**************************
 * @brief Object for parsing HTML data. Initialization in constructor from string, generate array of teg structures.
 */
class HTML {
public:
	static inline const int maxTagSize{ 8 };

	enum class Type : int16_t {
		Undefined,
		Html,
		Body,
		Head,
		Header,
		Main,
		Section,
		Footer,
		Div,
		Ul,
		Li,
		P,
		Span,
		A,
		B,
		I,
		U,
		H1,
		H2,
		H3,
		H4,
		H5,
		Img,
		Script,
		Link,
		Meta,
		Title,
		Nav,
		Hr,
		Br,
		Input,
		Select,
		Option,
		Textarea,
		Form,
		Style,
		Comment,
		Max
	};
	enum class Valid : int16_t { Undefined, True, False, Max };

	/**************************
	 * @brief Object to describe a HTML tag.
	 *
	 * @test Has unit test.
	 */
	struct Tag {
	private:
		bool m_started{ false };

	public:
		Valid isOpenTag{ Valid::Undefined };
		Valid valid{ Valid::Undefined };
		Type type{ Type::Undefined };
		size_t begin{ 0 };
		size_t end{ 0 };
		//* 0 == undefined
		uint depth{ 0 };
		//? std::vector<std::string> classes;
		//? std::vector<std::string> ids;
		//? std::vector<std::string> attributes;
		//? std::string content;

		/**************************
		 * @return True if tag is started, false otherwise.
		 */
		bool IsStarted() const;

		/**************************
		 * @brief Set tag as started.
		 */
		void SetStartedTrue();

		/**************************
		 * @return True if tag can be only alone, without closing construction like img, false otherwise.
		 */
		bool IsAlone() const noexcept;

		/**************************
		 * @example HTML Tag:
		 * {
		 * 		valid       : true
		 * 		is open tag : true
		 * 		type        : div
		 * 		begin       : 4
		 * 		end         : 20
		 * 		depth       : 2
		 * }
		 */
		std::string ToString() const noexcept;

		/**************************
		 * @brief Call ToString() inside.
		 */
		operator std::string() const;

		/**************************
		 * @brief Call ToString() inside.
		 */
		friend std::string operator+(const std::string& str, Tag& tag);

		/**************************
		 * @brief Call ToString() inside.
		 */
		friend std::ostream& operator<<(std::ostream& os, Tag& tag);

		/**************************
		 * @brief Compare by all fields.
		 */
		friend bool operator==(const Tag& first, const Tag& second) noexcept;

		/**************************
		 * @brief Compare by all fields.
		 */
		friend bool operator!=(const Tag& first, const Tag& second) noexcept;
	};

private:
	std::vector<Tag> m_tags;
	uint m_maxDepth{ 0 };
	size_t m_size{ 0 };
	// TODO: std::map<Type, std::set<int>> m_indexesForType; //* Type, { int }
	// TODO: valid/invalid
	// TODO: empty

public:
	/**************************
	 * @brief Construct a new HTML object, parse buffer.
	 *
	 * @param buffer HTML data.
	 *
	 * @test Has unit test.
	 */
	HTML(std::string_view buffer);

	/**************************
	 * @return Tag by index, if index is out of range or 0, return last tag.
	 *
	 * @test Has unit test.
	 *
	 * @todo Way if tags size == 0.
	 */
	const Tag& GetTag(size_t index = 0) const noexcept;

	/**************************
	 * @return Maximal depth of tags.
	 *
	 * @test Has unit test.
	 */
	uint MaxDepth() const noexcept;

	/**************************
	 * @return HTML buffer size in bytes.
	 *
	 * @test Has unit test.
	 */
	size_t BodySize() const noexcept;

	/**************************
	 * @return Size of tags.
	 *
	 * @test Has unit test.
	 */
	size_t TagsSize() const noexcept;

	/**************************
	 * @return Description of valid type.
	 *
	 * @example Undefined, True, False, ValidMax, Unknown.
	 */
	static std::string_view EnumToString(HTML::Valid value);

	/**************************
	 * @return Description of tag type.
	 *
	 * @example empty, html, body, head, header, main, section, footer, div, ul, li, p, span, a, b, i, u, h1, h2, h3,
	 * h4, h5, img, script, link, meta, title, nav, hr, br, input, select, option, textarea, form, style, comment,
	 * max, unknown.
	 */
	static std::string_view EnumToString(HTML::Type value);

	/**************************
	 * @return Incremented type value.
	 */
	friend Type operator++(Type& type);

	/**************************
	 * @return Incremented valid value.
	 */
	friend Valid operator++(Valid& valid);

	/**************************
	 * @return Compare by return of EnumToString().
	 */
	friend bool operator==(std::string_view str, Type type);

	/**************************
	 * @return Compare by return of EnumToString().
	 */
	friend bool operator==(std::string_view str, Valid valid);

	/**************************
	 * @brief Call EnumToString() inside.
	 */
	friend std::ostream& operator<<(std::ostream& os, Valid valid);

	/**************************
	 * @brief Call EnumToString() inside.
	 */
	friend std::ostream& operator<<(std::ostream& os, Type type);

	/**************************
	 * @example HTML:
	 * {
	 * 		tags size : 3
	 * 		max depth : 2
	 * 		body size : 20
	 * }
	 */
	std::string ToString() const noexcept;

	/**************************
	 * @brief Call ToString() inside.
	 */
	operator std::string() const;

	/**************************
	 * @brief Call ToString() inside.
	 */
	friend std::string operator+(const std::string& str, HTML& html);

	/**************************
	 * @brief Call ToString() inside.
	 */
	friend std::ostream& operator<<(std::ostream& os, HTML& html);

	/**************************
	 * @return True if all tests passed and false if something went wrong.
	 */
	static bool UNITTEST();
};

}; //* namespace MSAPI

#endif //* MSAPI_HTML_H