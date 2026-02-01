/**************************
 * @file        meta.hpp
 * @version     6.0
 * @date        2024-05-14
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
 *
 * @brief Contains common groups of types, meta functions and type traits.
 */

#ifndef MSAPI_META_H
#define MSAPI_META_H

#include "time.h"
#include <list>
#include <map>
#include <optional>
#include <set>
#include <variant>

namespace MSAPI {

class TableData;

#define integerTypes int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t

#define integerTypesOptional                                                                                           \
	std::optional<int8_t>, std::optional<int16_t>, std::optional<int32_t>, std::optional<int64_t>,                     \
		std::optional<uint8_t>, std::optional<uint16_t>, std::optional<uint32_t>, std::optional<uint64_t>

#define floatTypes double, float

#define floatTypesOptional std::optional<double>, std::optional<float>

#define integerTypesPtr int8_t*, int16_t*, int32_t*, int64_t*, uint8_t*, uint16_t*, uint32_t*, uint64_t*

#define integerTypesConstPtr                                                                                           \
	const int8_t*, const int16_t*, const int32_t*, const int64_t*, const uint8_t*, const uint16_t*, const uint32_t*,   \
		const uint64_t*

#define integerTypesOptionalPtr                                                                                        \
	std::optional<int8_t>*, std::optional<int16_t>*, std::optional<int32_t>*, std::optional<int64_t>*,                 \
		std::optional<uint8_t>*, std::optional<uint16_t>*, std::optional<uint32_t>*, std::optional<uint64_t>*

#define integerTypesOptionalConstPtr                                                                                   \
	const std::optional<int8_t>*, const std::optional<int16_t>*, const std::optional<int32_t>*,                        \
		const std::optional<int64_t>*, const std::optional<uint8_t>*, const std::optional<uint16_t>*,                  \
		const std::optional<uint32_t>*, const std::optional<uint64_t>*

#define floatTypesPtr double*, float*

#define floatTypesConstPtr const double*, const float*

#define floatTypesOptionalPtr std::optional<double>*, std::optional<float>*

#define floatTypesOptionalConstPtr const std::optional<double>*, const std::optional<float>*

template <typename T, typename... Ts> constexpr bool is_any_of = (std::is_same_v<T, Ts> || ...);

template <typename T, typename... Ts>
constexpr bool is_any_constructible_from = (std::constructible_from<Ts, T> || ...);

template <typename T, typename S> constexpr bool is_greater_type = sizeof(T) > sizeof(S);

template <typename T> constexpr bool is_integer_type = is_any_of<T, integerTypes>;
template <typename T> constexpr bool is_integer_type_optional = is_any_of<T, integerTypesOptional>;
template <typename T> constexpr bool is_float_type = is_any_of<T, floatTypes>;
template <typename T> constexpr bool is_float_type_optional = is_any_of<T, floatTypesOptional>;
template <typename T> constexpr bool is_integer_type_ptr = is_any_of<T, integerTypesPtr>;
template <typename T> constexpr bool is_integer_type_const_ptr = is_any_of<T, integerTypesConstPtr>;
template <typename T> constexpr bool is_integer_type_optional_ptr = is_any_of<T, integerTypesOptionalPtr>;
template <typename T> constexpr bool is_integer_type_optional_const_ptr = is_any_of<T, integerTypesOptionalConstPtr>;
template <typename T> constexpr bool is_float_type_ptr = is_any_of<T, floatTypesPtr>;
template <typename T> constexpr bool is_float_type_const_ptr = is_any_of<T, floatTypesConstPtr>;
template <typename T> constexpr bool is_float_type_optional_ptr = is_any_of<T, floatTypesOptionalPtr>;
template <typename T> constexpr bool is_float_type_optional_const_ptr = is_any_of<T, floatTypesOptionalConstPtr>;

template <typename... Ts> constexpr bool are_all_integer_types = (is_integer_type<Ts> && ...);
template <typename... Ts> constexpr bool are_all_integer_types_optional = (is_integer_type_optional<Ts> && ...);
template <typename... Ts> constexpr bool are_all_float_types = (is_float_type<Ts> && ...);
template <typename... Ts> constexpr bool are_all_float_types_optional = (is_float_type_optional<Ts> && ...);
template <typename... Ts> constexpr bool are_all_integer_types_ptr = (is_integer_type_ptr<Ts> && ...);
template <typename... Ts> constexpr bool are_all_integer_types_const_ptr = (is_integer_type_const_ptr<Ts> && ...);
template <typename... Ts> constexpr bool are_all_integer_types_optional_ptr = (is_integer_type_optional_ptr<Ts> && ...);
template <typename... Ts>
constexpr bool are_all_integer_types_optional_const_ptr = (is_integer_type_optional_const_ptr<Ts> && ...);
template <typename... Ts> constexpr bool are_all_float_types_ptr = (is_float_type_ptr<Ts> && ...);
template <typename... Ts> constexpr bool are_all_float_types_const_ptr = (is_float_type_const_ptr<Ts> && ...);
template <typename... Ts> constexpr bool are_all_float_types_optional_ptr = (is_float_type_optional_ptr<Ts> && ...);
template <typename... Ts>
constexpr bool are_all_float_types_optional_const_ptr = (is_float_type_optional_const_ptr<Ts> && ...);

template <typename T> struct remove_optional {
	using type = T;
};

template <typename T> struct remove_optional<std::optional<T>> {
	using type = T;
};

template <typename T> using remove_optional_t = typename remove_optional<T>::type;

template <typename T> struct is_optional : std::false_type { };

template <typename T> struct is_optional<std::optional<T>> : std::true_type { };

template <typename T> constexpr bool is_optional_v = is_optional<T>::value;

template <typename T, bool = std::is_enum_v<T>> struct safe_underlying_type {
	using type = T;
};

template <typename T> struct safe_underlying_type<T, true> {
	using type = std::underlying_type_t<T>;
};

template <typename T> using safe_underlying_type_t = typename safe_underlying_type<T>::type;

template <typename T> struct second_map_type {
	using type = T;
};

template <typename T> struct second_map_type<std::map<size_t, T>> {
	using type = T;
};

template <typename... Ts> constexpr size_t calculate_total_sizeof() { return (... + sizeof(Ts)); }

template <typename Tuple, typename F, size_t... I> void for_each(Tuple& tuple, F&& f, std::index_sequence<I...>)
{
	(f(std::get<I>(tuple)), ...);
}

template <typename... Ts, typename F> void for_each_in_tuple(std::tuple<Ts...>& tuple, F&& f)
{
	for_each(tuple, f, std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename Tuple, typename F, size_t... I>
void apply_to_element(const Tuple& tuple, size_t index, F&& f, std::index_sequence<I...>)
{
	((I == index ? f(std::get<I>(tuple)) : void()), ...);
}

template <typename... Ts, typename F>
void apply_to_element_in_tuple(const std::tuple<Ts...>& tuple, size_t index, F&& f)
{
	apply_to_element(tuple, index, f, std::make_index_sequence<sizeof...(Ts)>{});
}

#define standardPrimitiveTypes integerTypes, floatTypes, bool

#define standardPrimitiveTypesOptional integerTypesOptional, floatTypesOptional

#define standardSimpleTypes standardPrimitiveTypes, standardPrimitiveTypesOptional

#define standardTypes standardSimpleTypes, std::string, MSAPI::Timer, MSAPI::Timer::Duration, MSAPI::TableData

#define standardPrimitiveTypesPtr integerTypesPtr, floatTypesPtr, bool*

#define standardPrimitiveTypesConstPtr integerTypesConstPtr, floatTypesConstPtr, const bool*

#define standardPrimitiveTypesOptionalPtr integerTypesOptionalPtr, floatTypesOptionalPtr

#define standardPrimitiveTypesOptionalConstPtr integerTypesOptionalConstPtr, floatTypesOptionalConstPtr

#define standardSimpleTypesPtr standardPrimitiveTypesPtr, standardPrimitiveTypesOptionalPtr

#define standardSimpleTypesConstPtr standardPrimitiveTypesConstPtr, standardPrimitiveTypesOptionalConstPtr

#define standardTypesPtr standardSimpleTypesPtr, std::string*, MSAPI::Timer*, MSAPI::Timer::Duration*, MSAPI::TableData*

#define standardTypesConstPtr                                                                                          \
	standardSimpleTypesConstPtr, const std::string*, const MSAPI::Timer*, const MSAPI::Timer::Duration*,               \
		const MSAPI::TableData*

template <typename T> constexpr bool is_standard_primitive_type = is_any_of<T, standardPrimitiveTypes>;
template <typename T> constexpr bool is_standard_primitive_type_optional = is_any_of<T, standardPrimitiveTypesOptional>;
template <typename T> constexpr bool is_standard_simple_type = is_any_of<T, standardSimpleTypes>;
template <typename T> constexpr bool is_standard_type = is_any_of<T, standardTypes>;
template <typename T> constexpr bool is_standard_primitive_type_ptr = is_any_of<T, standardPrimitiveTypesPtr>;
template <typename T>
constexpr bool is_standard_primitive_type_const_ptr = is_any_of<T, standardPrimitiveTypesConstPtr>;
template <typename T>
constexpr bool is_standard_primitive_type_optional_ptr = is_any_of<T, standardPrimitiveTypesOptionalPtr>;
template <typename T>
constexpr bool is_standard_primitive_type_optional_const_ptr = is_any_of<T, standardPrimitiveTypesOptionalConstPtr>;
template <typename T> constexpr bool is_standard_simple_type_ptr = is_any_of<T, standardSimpleTypesPtr>;
template <typename T> constexpr bool is_standard_simple_type_const_ptr = is_any_of<T, standardSimpleTypesConstPtr>;
template <typename T> constexpr bool is_standard_type_ptr = is_any_of<T, standardTypesPtr>;
template <typename T> constexpr bool is_standard_type_const_ptr = is_any_of<T, standardTypesConstPtr>;

template <typename... Ts> constexpr bool are_all_standard_primitive_types = (is_standard_primitive_type<Ts> && ...);
template <typename... Ts>
constexpr bool are_all_standard_primitive_types_optional = (is_standard_primitive_type_optional<Ts> && ...);
template <typename... Ts> constexpr bool are_all_standard_simple_types = (is_standard_simple_type<Ts> && ...);
template <typename... Ts> constexpr bool are_all_standard_types = (is_standard_type<Ts> && ...);
template <typename... Ts>
constexpr bool are_all_standard_primitive_types_ptr = (is_standard_primitive_type_ptr<Ts> && ...);
template <typename... Ts>
constexpr bool are_all_standard_primitive_types_const_ptr = (is_standard_primitive_type_const_ptr<Ts> && ...);
template <typename... Ts>
constexpr bool are_all_standard_primitive_types_optional_ptr = (is_standard_primitive_type_optional_ptr<Ts> && ...);
template <typename... Ts>
constexpr bool are_all_standard_primitive_types_optional_const_ptr
	= (is_standard_primitive_type_optional_const_ptr<Ts> && ...);
template <typename... Ts> constexpr bool are_all_standard_simple_types_ptr = (is_standard_simple_type_ptr<Ts> && ...);
template <typename... Ts>
constexpr bool are_all_standard_simple_types_const_ptr = (is_standard_simple_type_const_ptr<Ts> && ...);
template <typename... Ts> constexpr bool are_all_standard_types_ptr = (is_standard_type_ptr<Ts> && ...);
template <typename... Ts> constexpr bool are_all_standard_types_const_ptr = (is_standard_type_const_ptr<Ts> && ...);

template <typename T, typename... Ts> constexpr bool is_included_in = (std::is_same_v<T, Ts> || ...);

template <typename... Ts> struct variant_of_unique_types;

template <typename T, typename... Ts> struct variant_of_unique_types<T, Ts...> {
	using type = std::conditional_t<(std::is_same_v<T, Ts> || ...), typename variant_of_unique_types<Ts...>::type,
		std::variant<T, typename variant_of_unique_types<Ts...>::type>>;
};

template <typename T> struct variant_of_unique_types<T> {
	using type = std::variant<T>;
};

template <> struct variant_of_unique_types<> {
	using type = std::variant<>;
};

template <typename... Ts> using variant_of_unique_types_t = typename variant_of_unique_types<Ts...>::type;

// Helper type to identify if a type is in a list
template <typename T, typename... List> struct IsInList;

template <typename T> struct IsInList<T> : std::false_type { };

template <typename T, typename Head, typename... Tail>
struct IsInList<T, Head, Tail...>
	: std::conditional_t<std::is_same<T, Head>::value, std::true_type, IsInList<T, Tail...>> { };

// Helper struct to remove duplicates
template <typename...> struct Unique;

template <> struct Unique<> {
	using type = std::tuple<>;
};

template <typename Head, typename... Tail> struct Unique<Head, Tail...> {
private:
	using TailUnique = typename Unique<Tail...>::type;

public:
	using type = std::conditional_t<IsInList<Head, Tail...>::value, TailUnique,
		decltype(std::tuple_cat(std::declval<std::tuple<Head>>(), std::declval<TailUnique>()))>;
};

// Helper alias template for convenience
template <typename... Ts> using Unique_t = typename Unique<Ts...>::type;

// Helper to transform unique types into std::multimap types
template <typename T> struct TransformToMultimap {
	using type = std::map<size_t, T>;
};

template <typename... Ts> struct TransformPack;

template <typename... Ts> struct TransformPack<std::tuple<Ts...>> {
	using type = std::tuple<typename TransformToMultimap<Ts>::type...>;
};

template <typename Tuple> using TransformPack_t = typename TransformPack<Tuple>::type;

// Helper to convert tuple to variant
template <typename Tuple, std::size_t... I> auto tuple_to_variant_impl(std::index_sequence<I...>)
{
	return std::variant<std::tuple_element_t<I, Tuple>...>{};
}

template <typename Tuple>
using tuple_to_variant = decltype(tuple_to_variant_impl<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{}));

template <typename... Ts> struct are_same_packs { };

template <typename... Ts, typename... Us> struct are_same_packs<std::tuple<Ts...>, std::tuple<Us...>> {
	static constexpr bool value = std::conjunction_v<std::is_same<Ts, Us>...>;
};

template <typename T>
concept Iterable = requires(T t) {
	{ t.begin() } -> std::input_iterator;
	{ t.end() } -> std::input_iterator;
	{ t.empty() } -> std::convertible_to<bool>;
	{ t.size() } -> std::convertible_to<std::size_t>;
};

template <typename T, typename S>
concept EmplaceableBack = requires(T t, S s) {
	std::is_same_v<typename T::value_type, S>;
	{ t.emplace_back(s) };
};

static_assert(EmplaceableBack<std::vector<int>, int>, "EmplaceableBack concept failed");
static_assert(EmplaceableBack<std::list<int>, int>, "EmplaceableBack concept failed");
static_assert(!EmplaceableBack<std::set<int>, int>, "EmplaceableBack concept failed");

template <typename T, typename S>
concept Emplaceable = requires(T t, S s) {
	{ t.emplace(s) };
};

static_assert(Emplaceable<std::set<int>, int>, "Emplaceable concept failed");
static_assert(!Emplaceable<std::list<int>, int>, "Emplaceable concept failed");
static_assert(!Emplaceable<std::vector<int>, int>, "Emplaceable concept failed");

template <typename T>
concept Enum = requires {
	T::Undefined;
	T::Max;
};

template <typename T>
concept StringableView = std::is_same_v<std::remove_cv_t<T>, std::string_view>
	|| (std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>)
	|| (std::is_rvalue_reference_v<T> && std::is_same_v<std::remove_reference_t<T>, std::string_view>);

static_assert(StringableView<std::string_view>, "StringableView concept failed");
static_assert(StringableView<const volatile std::string_view>, "StringableView concept failed");
static_assert(!StringableView<std::string>, "StringableView concept failed");
static_assert(!StringableView<const volatile std::string>, "StringableView concept failed");
static_assert(!StringableView<std::string&>, "StringableView concept failed");
static_assert(!StringableView<const volatile std::string&>, "StringableView concept failed");
static_assert(!StringableView<const char*&&>, "StringableView concept failed");
static_assert(!StringableView<const char*&>, "StringableView concept failed");
static_assert(StringableView<char*>, "StringableView concept failed");
static_assert(!StringableView<char>, "StringableView concept failed");
static_assert(!StringableView<char[]>, "StringableView concept failed");
static_assert(StringableView<const volatile char* const volatile>, "StringableView concept failed");
static_assert(!StringableView<std::string&&>, "StringableView concept failed");
static_assert(StringableView<std::string_view&&>, "StringableView concept failed");

template <typename T>
concept Stringable = StringableView<T>
	|| (std::is_reference_v<T> && std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::string>);

static_assert(!Stringable<std::string>, "Stringable concept failed");
static_assert(!Stringable<const volatile std::string>, "Stringable concept failed");
static_assert(Stringable<std::string&>, "Stringable concept failed");
static_assert(Stringable<const volatile std::string&>, "Stringable concept failed");
static_assert(Stringable<std::string&&>, "Stringable concept failed");

template <Stringable T> FORCE_INLINE [[nodiscard]] const char* CString(const T str) noexcept
{
	if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string_view>) {
		return str.data();
	}
	else if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string>) {
		return str.c_str();
	}
	else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>) {
		return str;
	}
	else {
		static_assert(sizeof(T) + 1 == 0, "Unsupported type in CString");
	}
}

}; //* namespace MSAPI

#endif //* MSAPI_META_H