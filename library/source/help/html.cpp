/**************************
 * @file        html.cpp
 * @version     6.0
 * @date        2023-09-24
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

#include "html.h"
#include <iomanip>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Tag
---------------------------------------------------------------------------------*/

HTML::Tag::operator std::string() const { return ToString(); }

std::string operator+(const std::string& str, HTML::Tag& tag) { return str + tag.ToString(); }

std::ostream& operator<<(std::ostream& os, HTML::Tag& tag) { return os << tag.ToString(); }

bool operator==(const HTML::Tag& first, const HTML::Tag& second) noexcept
{
	return first.valid == second.valid && first.isOpenTag == second.isOpenTag && first.type == second.type
		&& first.begin == second.begin && first.end == second.end && first.m_started == second.m_started
		&& first.depth == second.depth;
}

bool operator!=(const HTML::Tag& first, const HTML::Tag& second) noexcept
{
	return first.valid != second.valid || first.isOpenTag != second.isOpenTag || first.type != second.type
		|| first.begin != second.begin || first.end != second.end || first.m_started != second.m_started
		|| first.depth != second.depth;
}

bool HTML::Tag::IsAlone() const noexcept
{
	return type == Type::Img || type == Type::Link || type == Type::Meta || type == Type::Hr || type == Type::Br
		|| type == Type::Input;
}

bool HTML::Tag::IsStarted() const { return m_started; }

void HTML::Tag::SetStartedTrue() { m_started = true; }

/*---------------------------------------------------------------------------------
HTML
---------------------------------------------------------------------------------*/

std::string HTML::Tag::ToString() const noexcept
{
	std::stringstream stream;

#define format std::left << std::setw(11)

	stream << std::fixed << std::setprecision(16) << "HTML Tag:\n{\n\t" << format << "valid"
		   << " : " << valid << "\n\t" << format << "is open tag"
		   << " : " << isOpenTag << "\n\t" << format << "type"
		   << " : " << type << "\n\t" << format << "begin"
		   << " : " << begin << "\n\t" << format << "end"
		   << " : " << end << "\n\t" << format << "depth"
		   << " : " << depth;

	stream << "\n}";

#undef format

	return stream.str();
}

HTML::HTML(const std::string_view buffer)
{
	Tag currentTag;
	uint currentDepth{ 0 };
	bool currentTagComment{ false };
	bool commentWithDash{ false };
	for (size_t index{ 0 }; index < buffer.size(); ++index) {
		const char symbol = buffer[m_size];
		++m_size;

		if (m_size > buffer.size()) {
			LOG_WARNING("Extra insert for HTML parser. Buffer size is " + _S(buffer.size()) + ", but current size is "
				+ _S(m_size));
			continue;
		}

		if (!commentWithDash && currentTag.type == Type::Comment && m_size - currentTag.begin > 1
			&& buffer[currentTag.begin + 2] == '-') {
			commentWithDash = true;
		}

		if (currentTag.IsStarted()) {
			++currentTag.end;

			if (currentTag.valid == Valid::Undefined) {
				std::string name;
				size_t index{ currentTag.begin + 1 };
				size_t maxSizeMultiplier{ currentTag.begin };

				//* We using isCloseTag and don't filling currentTag state, because it's will be happen every iteration
				bool isCloseTag{ buffer[index] == '/' };

				if (buffer[index] == '!') {
					currentTagComment = true;
					currentTag.type = Type::Comment;
					currentTag.valid = Valid::True;
				}
				else {
					if (isCloseTag) {
						++maxSizeMultiplier;
						++index;
					}

					for (; index < currentTag.end; ++index) {
						if (index - maxSizeMultiplier > maxTagSize) {
							currentTag.valid = Valid::False;
						}

						name += buffer[index];
						if (buffer[index + 1] == ' ' || buffer[index + 1] == '>') {
							for (Type type{ 0 }; type < Type::Max; ++type) {
								if (name == type) {
									currentTag.type = type;
									currentTag.valid = Valid::True;
									index = currentTag.end;
									break;
								}
							}
						}
					}
				}
			}
		}

		if (symbol == '<' && !currentTag.IsStarted()) {
			currentTag.SetStartedTrue();

			currentTag.begin = m_size - 1;
			currentTag.end = currentTag.begin;
			continue;
		}

		if (symbol == '>' && currentTag.IsStarted()) {
			if (currentTagComment) {
				if (commentWithDash && buffer[m_size - 2] != '-') {
					continue;
				}
				currentTagComment = false;
				currentTag.isOpenTag = Valid::False;
			}
			else if (buffer[currentTag.begin + 1] == '/') {
				currentTag.isOpenTag = Valid::False;
				currentTag.depth = currentDepth;
				--currentDepth;
			}
			else {
				if (!currentTag.IsAlone()) {
					++currentDepth;
					currentTag.depth = currentDepth;
					if (currentDepth > m_maxDepth) {
						m_maxDepth = currentDepth;
					}
					currentTag.isOpenTag = Valid::True;
				}
				else {
					currentTag.isOpenTag = Valid::False;
				}
			}

			m_tags.emplace_back(currentTag);
			currentTag = Tag();
			continue;
		}
	}
}

const HTML::Tag& HTML::GetTag(const size_t index) const noexcept
{
	if (index <= 0 || index > m_tags.size()) {
		return m_tags[m_tags.size() - 1];
	}
	else {
		return m_tags[index - 1];
	}
}

uint HTML::MaxDepth() const noexcept { return m_maxDepth; }

size_t HTML::BodySize() const noexcept { return m_size; }

size_t HTML::TagsSize() const noexcept { return m_tags.size(); };

HTML::Type operator++(HTML::Type& type) { return type = static_cast<HTML::Type>(static_cast<short>(type) + 1); }

HTML::Valid operator++(HTML::Valid& valid) { return valid = static_cast<HTML::Valid>(static_cast<short>(valid) + 1); }

bool operator==(const std::string_view str, const HTML::Type type) { return str == HTML::EnumToString(type); }

bool operator==(const std::string_view str, const HTML::Valid valid) { return str == HTML::EnumToString(valid); }

std::ostream& operator<<(std::ostream& os, const HTML::Valid valid) { return os << HTML::EnumToString(valid); }

std::ostream& operator<<(std::ostream& os, const HTML::Type type) { return os << HTML::EnumToString(type); }

std::string HTML::ToString() const noexcept
{
	std::stringstream stream;

#define format std::setw(9)

	stream << std::fixed << std::setprecision(16) << "HTML:\n{\n\t" << std::left << format << "tags size"
		   << " : " << m_tags.size() << "\n\t" << std::left << format << "max depth"
		   << " : " << m_maxDepth << "\n\t" << std::left << format << "body size"
		   << " : " << m_size << "\n}";

#undef format

	return stream.str();
}

HTML::operator std::string() const { return ToString(); }

std::string operator+(const std::string& str, HTML& html) { return str + html.ToString(); }

std::ostream& operator<<(std::ostream& os, HTML& html) { return os << html.ToString(); }

std::string_view HTML::EnumToString(const HTML::Valid value)
{
	static_assert(U(HTML::Valid::Max) == 3, "Missed description for new HTML tag valid enum");

	switch (value) {
	case HTML::Valid::Undefined:
		return "Undefined";
	case HTML::Valid::True:
		return "True";
	case HTML::Valid::False:
		return "False";
	case HTML::Valid::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown HTML tag valid enum: " + _S(U(value)));
		return "Unknown";
	}
}

std::string_view HTML::EnumToString(const HTML::Type value)
{
	static_assert(U(HTML::Type::Max) == 37, "Missed description for new HTML tag type enum");

	switch (value) {
	case HTML::Type::Undefined:
		return "Undefined";
	case HTML::Type::Html:
		return "html";
	case HTML::Type::Body:
		return "body";
	case HTML::Type::Head:
		return "head";
	case HTML::Type::Header:
		return "header";
	case HTML::Type::Main:
		return "main";
	case HTML::Type::Section:
		return "section";
	case HTML::Type::Footer:
		return "footer";
	case HTML::Type::Div:
		return "div";
	case HTML::Type::Ul:
		return "ul";
	case HTML::Type::Li:
		return "li";
	case HTML::Type::P:
		return "p";
	case HTML::Type::Span:
		return "span";
	case HTML::Type::A:
		return "a";
	case HTML::Type::B:
		return "b";
	case HTML::Type::I:
		return "i";
	case HTML::Type::U:
		return "u";
	case HTML::Type::H1:
		return "h1";
	case HTML::Type::H2:
		return "h2";
	case HTML::Type::H3:
		return "h3";
	case HTML::Type::H4:
		return "h4";
	case HTML::Type::H5:
		return "h5";
	case HTML::Type::Img:
		return "img";
	case HTML::Type::Script:
		return "script";
	case HTML::Type::Link:
		return "link";
	case HTML::Type::Meta:
		return "meta";
	case HTML::Type::Title:
		return "title";
	case HTML::Type::Nav:
		return "nav";
	case HTML::Type::Hr:
		return "hr";
	case HTML::Type::Br:
		return "br";
	case HTML::Type::Input:
		return "input";
	case HTML::Type::Select:
		return "select";
	case HTML::Type::Option:
		return "option";
	case HTML::Type::Textarea:
		return "textarea";
	case HTML::Type::Form:
		return "form";
	case HTML::Type::Style:
		return "style";
	case HTML::Type::Comment:
		return "comment";
	case HTML::Type::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown HTML tag type enum: " + _S(U(value)));
		return "Unknown";
	}
}


}; //* namespace MSAPI