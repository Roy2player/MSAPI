/**************************
 * @file        application.cpp
 * @version     6.0
 * @date        2023-12-11
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

#include "application.h"
#include "../test/test.h"
#include <arpa/inet.h>

namespace MSAPI {

/*---------------------------------------------------------------------------------
Parameter
---------------------------------------------------------------------------------*/

Application::Parameter::Parameter(std::string&& name, bool* value)
	: m_name{ std::move(name) }
	, m_value{ value }
{
}

bool Application::Parameter::RegisterValidation(const size_t id)
{
	bool valid{ true };
	std::visit(
		[&id, &valid, this](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			using S = remove_optional_t<std::remove_pointer_t<T>>;
			if constexpr (is_integer_type_ptr<T>) {
				if (m_printFunc == nullptr) {
					if (m_min.has_value() && *arg < std::get<S>(m_min.value())) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id, *arg,
							std::get<S>(m_min.value()));
						valid = false;
						LOG_INFO(std::format(
							"Parameter is registered with error. {}. Started value: {}, min: {}, max: {}", m_error,
							*arg, std::get<S>(m_min.value()), m_max.has_value() ? _S(std::get<S>(m_max.value())) : ""));
					}
					else if (m_max.has_value() && *arg > std::get<S>(m_max.value())) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id, *arg,
							std::get<S>(m_max.value()));
						valid = false;
						LOG_INFO(std::format(
							"Parameter is registered with error. {}. Started value: {}, min: {}, max: {}", m_error,
							*arg, m_min.has_value() ? _S(std::get<S>(m_min.value())) : "", std::get<S>(m_max.value())));
					}
					else {
						LOG_INFO(std::format("Parameter {}({}) is registered. Started value: {}, min: {}, max: {}",
							m_name, id, *arg, m_min.has_value() ? _S(std::get<S>(m_min.value())) : "",
							m_max.has_value() ? _S(std::get<S>(m_max.value())) : ""));
					}
					return;
				}

				if (m_min.has_value() && *arg < std::get<S>(m_min.value())) {
					m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
						m_printFunc(static_cast<int>(*arg)), m_printFunc(static_cast<int>(std::get<S>(m_min.value()))));
					valid = false;
					LOG_INFO_NEW("Parameter is registered with error. {}. Started value: {}, min: {}, max: {}", m_error,
						m_printFunc(static_cast<int>(*arg)), m_printFunc(static_cast<int>(std::get<S>(m_min.value()))),
						m_max.has_value() ? m_printFunc(static_cast<int>(std::get<S>(m_max.value()))) : "");
				}
				else if (m_max.has_value() && *arg > std::get<S>(m_max.value())) {
					m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
						m_printFunc(static_cast<int>(*arg)), m_printFunc(static_cast<int>(std::get<S>(m_max.value()))));
					valid = false;
					LOG_INFO_NEW("Parameter is registered with error. {}. Started value: {}, min: {}, max: {}", m_error,
						m_printFunc(static_cast<int>(*arg)),
						m_min.has_value() ? m_printFunc(static_cast<int>(std::get<S>(m_min.value()))) : "",
						m_printFunc(static_cast<int>(std::get<S>(m_max.value()))));
				}
				else {
					LOG_INFO_NEW("Parameter {}({}) is registered. Started value: {}, min: {}, max: {}", m_name, id,
						m_printFunc(static_cast<int>(*arg)),
						m_min.has_value() ? m_printFunc(static_cast<int>(std::get<S>(m_min.value()))) : "",
						m_max.has_value() ? m_printFunc(static_cast<int>(std::get<S>(m_max.value()))) : "");
				}
			}
			else if constexpr (is_integer_type_optional_ptr<T>) {
				if (m_printFunc == nullptr) {
					if (arg->has_value()) {
						if (m_min.has_value() && *arg < std::get<S>(m_min.value())) {
							m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
								arg->value(), std::get<S>(m_min.value()));
							valid = false;
						}
						else if (m_max.has_value() && *arg > std::get<S>(m_max.value())) {
							m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
								arg->value(), std::get<S>(m_max.value()));
							valid = false;
						}
					}
					else if (!m_canBeEmpty) {
						m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
						valid = false;
					}

					if (valid) {
						LOG_INFO(std::format(
							"Parameter {}({}) is registered. Started value: {}, min: {}, max: {}, can be empty: {}",
							m_name, id, _S(arg), m_min.has_value() ? _S(std::get<S>(m_min.value())) : "",
							m_max.has_value() ? _S(std::get<S>(m_max.value())) : "", _S(m_canBeEmpty)));
					}
					else {
						LOG_INFO(std::format("Parameter is registered with error. {}. Started value: {}, min: {}, max: "
											 "{}, can be empty: {}",
							m_error, _S(arg), m_min.has_value() ? _S(std::get<S>(m_min.value())) : "",
							m_max.has_value() ? _S(std::get<S>(m_max.value())) : "", _S(m_canBeEmpty)));
					}

					return;
				}

				if (arg->has_value()) {
					if (m_min.has_value() && *arg < std::get<S>(m_min.value())) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
							m_printFunc(static_cast<int>(arg->value())),
							m_printFunc(static_cast<int>(std::get<S>(m_min.value()))));
						valid = false;
					}
					else if (m_max.has_value() && *arg > std::get<S>(m_max.value())) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
							m_printFunc(static_cast<int>(arg->value())),
							m_printFunc(static_cast<int>(std::get<S>(m_max.value()))));
						valid = false;
					}
				}
				else if (!m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
				}

				if (valid) {
					LOG_INFO_NEW(
						"Parameter {}({}) is registered. Started value: {}, min: {}, max: {}, can be empty: {}", m_name,
						id, arg->has_value() ? m_printFunc(static_cast<int>(arg->value())) : "",
						m_min.has_value() ? m_printFunc(static_cast<int>(std::get<S>(m_min.value()))) : "",
						m_max.has_value() ? m_printFunc(static_cast<int>(std::get<S>(m_max.value()))) : "",
						_S(m_canBeEmpty));
				}
				else {
					LOG_INFO_NEW("Parameter is registered with error. {}. Started value: {}, min: {}, max: "
								 "{}, can be empty: {}",
						m_error, arg->has_value() ? m_printFunc(static_cast<int>(arg->value())) : "",
						m_min.has_value() ? m_printFunc(static_cast<int>(std::get<S>(m_min.value()))) : "",
						m_max.has_value() ? m_printFunc(static_cast<int>(std::get<S>(m_max.value()))) : "",
						_S(m_canBeEmpty));
				}
			}
			else if constexpr (is_float_type_ptr<T>) {
				if (m_min.has_value() && Helper::FloatLess(*arg, std::get<S>(m_min.value()))) {
					m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id, _S(arg),
						_S(std::get<S>(m_min.value())));
					valid = false;
					LOG_INFO(std::format("Parameter is registered with error. {}. Started value: {}, min: {}, max: {}",
						m_error, _S(arg), std::get<S>(m_min.value()),
						m_max.has_value() ? _S(std::get<S>(m_max.value())) : ""));
				}
				else if (m_max.has_value() && Helper::FloatGreater(*arg, std::get<S>(m_max.value()))) {
					m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id, _S(arg),
						_S(std::get<S>(m_max.value())));
					valid = false;
					LOG_INFO(std::format("Parameter is registered with error. {}. Started value: {}, min: {}, max: {}",
						m_error, _S(arg), m_min.has_value() ? _S(std::get<S>(m_min.value())) : "",
						_S(std::get<S>(m_max.value()))));
				}
				else {
					LOG_INFO(std::format("Parameter {}({}) is registered. Started value: {}, min: {}, max: {}", m_name,
						id, _S(arg), m_min.has_value() ? _S(std::get<S>(m_min.value())) : "",
						m_max.has_value() ? _S(std::get<S>(m_max.value())) : ""));
				}
			}
			else if constexpr (is_float_type_optional_ptr<T>) {
				if (arg->has_value()) {
					if (m_min.has_value() && Helper::FloatLess(arg->value(), std::get<S>(m_min.value()))) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
							_S(arg->value()), _S(std::get<S>(m_min.value())));
						valid = false;
					}
					else if (m_max.has_value() && Helper::FloatGreater(arg->value(), std::get<S>(m_max.value()))) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
							_S(arg->value()), _S(std::get<S>(m_max.value())));
						valid = false;
					}
				}
				else if (!m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
				}

				if (valid) {
					LOG_INFO(std::format("Parameter {}({}) is registered. Started value: {}, min: {}, max: "
										 "{}, can be empty: {}",
						m_name, id, _S(arg), m_min.has_value() ? _S(std::get<S>(m_min.value())) : "",
						m_max.has_value() ? _S(std::get<S>(m_max.value())) : "", _S(m_canBeEmpty)));
				}
				else {
					LOG_INFO(std::format(
						"Parameter is registered with error. {}. Started value: {}, min: {}, max: {}, can be empty: {}",
						m_error, _S(arg), m_min.has_value() ? _S(std::get<S>(m_min.value())) : "",
						m_max.has_value() ? _S(std::get<S>(m_max.value())) : "", _S(m_canBeEmpty)));
				}
			}
			else if constexpr (std::is_same_v<T, bool*>) {
				LOG_INFO("Parameter " + m_name + "(" + _S(id) + ") is registered. Started value: " + _S(arg));
			}
			else if constexpr (std::is_same_v<T, std::string*>) {
				if (arg->empty() && !m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
					LOG_INFO("Parameter is registered with error. " + m_error + ". Started value: " + *arg
						+ ", can be empty: " + _S(m_canBeEmpty));
				}
				else {
					LOG_INFO("Parameter " + m_name + "(" + _S(id) + ") is registered. Started value: " + *arg
						+ ", can be empty: " + _S(m_canBeEmpty));
				}
			}
			else if constexpr (std::is_same_v<T, Timer*>) {
				if (arg->Empty() && !m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
					LOG_INFO("Parameter is registered with error. " + m_error + ". Started value: " + arg->ToString()
						+ ", can be empty: " + _S(m_canBeEmpty));
				}
				else {
					LOG_INFO("Parameter " + m_name + "(" + _S(id) + ") is registered. Started value: " + arg->ToString()
						+ ", can be empty: " + _S(m_canBeEmpty));
				}
			}
			else if constexpr (std::is_same_v<T, Timer::Duration*>) {
				if (!arg->Empty()) {
					if (m_min.has_value() && *arg < std::get<S>(m_min.value())) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
							arg->ToString(m_durationType), std::get<S>(m_min.value()).ToString(m_durationType));
						valid = false;
					}
					else if (m_max.has_value() && *arg > std::get<S>(m_max.value())) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
							arg->ToString(m_durationType), std::get<S>(m_max.value()).ToString(m_durationType));
						valid = false;
					}
				}
				else if (!m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
				}

				if (valid) {
					LOG_INFO(std::format(
						"Parameter {}({}) is registered. Started value: {}, min: {}, max: {}, can be empty: {}", m_name,
						id, arg->ToString(m_durationType),
						m_min.has_value() ? std::get<S>(m_min.value()).ToString(m_durationType) : "",
						m_max.has_value() ? std::get<S>(m_max.value()).ToString(m_durationType) : "",
						_S(m_canBeEmpty)));
				}
				else {
					LOG_INFO(std::format("Parameter is registered with error. {}. Started value: {}, min: {}, max: "
										 "{}, can be empty: {}",
						m_error, arg->ToString(m_durationType),
						m_min.has_value() ? std::get<S>(m_min.value()).ToString(m_durationType) : "",
						m_max.has_value() ? std::get<S>(m_max.value()).ToString(m_durationType) : "",
						_S(m_canBeEmpty)));
				}
			}
			else if constexpr (std::is_same_v<T, TableData*>) {
				if (reinterpret_cast<TableBase*>(arg)->Empty() && !m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
					LOG_INFO("Parameter is registered with error. " + m_error + ". Started value: "
						+ reinterpret_cast<TableBase*>(arg)->ToString() + ", can be empty: " + _S(m_canBeEmpty));
				}
				else {
					LOG_INFO("Parameter " + m_name + "(" + _S(id) + ") is registered. Started value: "
						+ reinterpret_cast<TableBase*>(arg)->ToString() + ", can be empty: " + _S(m_canBeEmpty));
				}
			}
			else {
				static_assert(sizeof(T) + 1 == 0, "Unsupported type for parameter");
			}
		},
		m_value);

	return valid;
}

bool Application::Parameter::Merge(const size_t id, const std::variant<standardTypes>& value)
{
	bool valid{ true };
	std::visit(
		[&id, &valid, this](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if (!std::holds_alternative<T*>(m_value)) {
				LOG_ERROR("Update for parameter " + m_name + "(" + _S(id) + ") has incorrect type, update is skipped");
				valid = m_error.empty();
				return;
			}
			using S = remove_optional_t<std::remove_pointer_t<T>>;
			if constexpr (is_integer_type<T>) {
				if (m_printFunc == nullptr) {
					if (*std::get<T*>(m_value) == arg) {
						LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, arg));
						valid = m_error.empty();
						return;
					}

					if (m_min.has_value() && arg < std::get<S>(m_min.value())) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id, arg,
							std::get<S>(m_min.value()));
						valid = false;
						LOG_WARNING("Parameter is updated with error. " + m_error);
					}
					else if (m_max.has_value() && arg > std::get<S>(m_max.value())) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id, arg,
							std::get<S>(m_max.value()));
						valid = false;
						LOG_WARNING("Parameter is updated with error. " + m_error);
					}
					else {
						LOG_INFO(std::format("Parameter {}({}) is updated. Value: {}", m_name, id, arg));
						m_error.clear();
					}
				}
				else {
					if (*std::get<T*>(m_value) == arg) {
						LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, arg));
						valid = m_error.empty();
						return;
					}

					if (m_min.has_value() && arg < std::get<S>(m_min.value())) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
							m_printFunc(static_cast<int>(arg)),
							m_printFunc(static_cast<int>(std::get<S>(m_min.value()))));
						valid = false;
						LOG_WARNING("Parameter is updated with error. " + m_error);
					}
					else if (m_max.has_value() && arg > std::get<S>(m_max.value())) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
							m_printFunc(static_cast<int>(arg)),
							m_printFunc(static_cast<int>(std::get<S>(m_max.value()))));
						valid = false;
						LOG_WARNING("Parameter is updated with error. " + m_error);
					}
					else {
						LOG_INFO_NEW(
							"Parameter {}({}) is updated. Value: {}", m_name, id, m_printFunc(static_cast<int>(arg)));
						m_error.clear();
					}
				}
				*std::get<T*>(m_value) = arg;
			}
			else if constexpr (is_integer_type_optional<T>) {
				if (m_printFunc == nullptr) {
					if (*std::get<T*>(m_value) == arg) {
						LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, _S(arg)));
						valid = m_error.empty();
						return;
					}

					if (arg.has_value()) {
						if (m_min.has_value() && arg < std::get<S>(m_min.value())) {
							m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
								arg.value(), std::get<S>(m_min.value()));
							valid = false;
						}
						else if (m_max.has_value() && arg > std::get<S>(m_max.value())) {
							m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
								arg.value(), std::get<S>(m_max.value()));
							valid = false;
						}
					}
					else if (!m_canBeEmpty) {
						m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
						valid = false;
					}

					*std::get<T*>(m_value) = arg;
					if (valid) {
						LOG_INFO(std::format("Parameter {}({}) is updated. Value: {}", m_name, id, _S(arg)));
						m_error.clear();
					}
					else {
						LOG_WARNING("Parameter is updated with error. " + m_error);
					}

					return;
				}

				if (*std::get<T*>(m_value) == arg) {
					LOG_DEBUG_NEW("Skip parameter {}({}) with the same value: {}", m_name, id,
						arg.has_value() ? m_printFunc(static_cast<int>(arg.value())) : "");
					valid = m_error.empty();
					return;
				}

				if (arg.has_value()) {
					if (m_min.has_value() && arg < std::get<S>(m_min.value())) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
							m_printFunc(static_cast<int>(arg.value())),
							m_printFunc(static_cast<int>(std::get<S>(m_min.value()))));
						valid = false;
					}
					else if (m_max.has_value() && arg > std::get<S>(m_max.value())) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
							m_printFunc(static_cast<int>(arg.value())),
							m_printFunc(static_cast<int>(std::get<S>(m_max.value()))));
						valid = false;
					}
				}
				else if (!m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
				}

				*std::get<T*>(m_value) = arg;
				if (valid) {
					LOG_INFO_NEW("Parameter {}({}) is updated. Value: {}", m_name, id,
						arg.has_value() ? m_printFunc(static_cast<int>(arg.value())) : "");
					m_error.clear();
				}
				else {
					LOG_WARNING("Parameter is updated with error. " + m_error);
				}
			}
			else if constexpr (is_float_type<T>) {
				if (Helper::FloatEqual(*std::get<T*>(m_value), arg)) {
					LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, _S(arg)));
					valid = m_error.empty();
					return;
				}

				if (m_min.has_value() && Helper::FloatLess(arg, std::get<S>(m_min.value()))) {
					m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id, _S(arg),
						_S(std::get<S>(m_min.value())));
					valid = false;
					LOG_WARNING("Parameter is updated with error. " + m_error);
				}
				else if (m_max.has_value() && Helper::FloatGreater(arg, std::get<S>(m_max.value()))) {
					m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id, _S(arg),
						_S(std::get<S>(m_max.value())));
					valid = false;
					LOG_WARNING("Parameter is updated with error. " + m_error);
				}
				else {
					LOG_INFO(std::format("Parameter {}({}) is updated. Value: {}", m_name, id, _S(arg)));
					m_error.clear();
				}
				*std::get<T*>(m_value) = arg;
			}
			else if constexpr (is_float_type_optional<T>) {
				if ((!std::get<T*>(m_value)->has_value() && !arg.has_value())
					|| (std::get<T*>(m_value)->has_value() && arg.has_value()
						&& Helper::FloatEqual(std::get<T*>(m_value)->value(), arg.value()))) {

					LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, _S(arg)));
					valid = m_error.empty();
					return;
				}

				if (arg.has_value()) {
					if (m_min.has_value() && Helper::FloatLess(arg.value(), std::get<S>(m_min.value()))) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
							_S(arg.value()), _S(std::get<S>(m_min.value())));
						valid = false;
					}
					else if (m_max.has_value() && Helper::FloatGreater(arg.value(), std::get<S>(m_max.value()))) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
							_S(arg.value()), _S(std::get<S>(m_max.value())));
						valid = false;
					}
				}
				else if (!m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
				}

				*std::get<T*>(m_value) = arg;
				if (valid) {
					LOG_INFO(std::format("Parameter {}({}) is updated. Value: {}", m_name, id, _S(arg)));
					m_error.clear();
				}
				else {
					LOG_WARNING("Parameter is updated with error. " + m_error);
				}
			}
			else if constexpr (std::is_same_v<T, bool>) {
				if (*std::get<T*>(m_value) == arg) {
					LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, _S(arg)));
					valid = m_error.empty();
					return;
				}

				*std::get<T*>(m_value) = arg;
				LOG_INFO("Parameter " + m_name + "(" + _S(id) + ") is updated. Value: " + _S(arg));
				m_error.clear();
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				if (*std::get<T*>(m_value) == arg) {
					LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, arg));
					valid = m_error.empty();
					return;
				}

				if (arg.empty() && !m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
					LOG_WARNING("Parameter is updated with error. " + m_error);
				}
				else {
					LOG_INFO("Parameter " + m_name + "(" + _S(id) + ") is updated. Value: " + arg);
					m_error.clear();
				}
				*std::get<T*>(m_value) = arg;
			}
			else if constexpr (std::is_same_v<T, Timer>) {
				if (*std::get<T*>(m_value) == arg) {
					LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, arg.ToString()));
					valid = m_error.empty();
					return;
				}

				if (arg.Empty() && !m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
					LOG_WARNING("Parameter is updated with error. " + m_error);
				}
				else {
					LOG_INFO("Parameter " + m_name + "(" + _S(id) + ") is updated. Value: " + arg.ToString());
					m_error.clear();
				}
				*std::get<T*>(m_value) = arg;
			}
			else if constexpr (std::is_same_v<T, Timer::Duration>) {
				if (*std::get<T*>(m_value) == arg) {
					LOG_DEBUG(std::format("Skip parameter {}({}) with the same value: {}", m_name, id, arg.ToString()));
					valid = m_error.empty();
					return;
				}

				if (!arg.Empty()) {
					if (m_min.has_value() && arg < std::get<S>(m_min.value())) {
						m_error = std::format("Parameter {}({}) is less than min value: {} < {}", m_name, id,
							arg.ToString(m_durationType), std::get<S>(m_min.value()).ToString(m_durationType));
						valid = false;
					}
					else if (m_max.has_value() && arg > std::get<S>(m_max.value())) {
						m_error = std::format("Parameter {}({}) is greater than max value: {} > {}", m_name, id,
							arg.ToString(m_durationType), std::get<S>(m_max.value()).ToString(m_durationType));
						valid = false;
					}
				}
				else if (!m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
				}

				*std::get<T*>(m_value) = arg;
				if (valid) {
					LOG_INFO(std::format(
						"Parameter {}({}) is updated. Value: {}", m_name, id, arg.ToString(m_durationType)));
					m_error.clear();
				}
				else {
					LOG_WARNING("Parameter is updated with error. " + m_error);
				}
			}
			else if constexpr (std::is_same_v<T, TableData>) {
				TableBase* tableBase{ reinterpret_cast<TableBase*>(std::get<T*>(m_value)) };
				tableBase->Copy(arg);
				if (tableBase->Empty() && !m_canBeEmpty) {
					m_error = "Parameter " + m_name + "(" + _S(id) + ") is empty";
					valid = false;
					LOG_INFO("Parameter is updated with error. " + m_error + ". Value: " + tableBase->ToString()
						+ ", can be empty: " + _S(m_canBeEmpty));
				}
				else {
					LOG_INFO("Parameter " + m_name + "(" + _S(id) + ") is updated. Value: " + tableBase->ToString()
						+ ", can be empty: " + _S(m_canBeEmpty));
				}
			}
			else {
				static_assert(sizeof(T) + 1 == 0, "Unsupported type for parameter");
			}
		},
		value);

	return valid;
}

/*---------------------------------------------------------------------------------
Application
---------------------------------------------------------------------------------*/

Application::Application()
{
	RegisterConstParameter(2000001, { "Name", &m_name });
	RegisterConstParameter(2000002, { "Application state", &m_state, &EnumToString });
}

Application::~Application() { HandlePauseRequest(); }

void Application::Collect(const int connection, const StandardProtocol::Data& data)
{
	LOG_PROTOCOL("Collect data from connection: " + _S(connection) + ", " + data.ToString());
	switch (data.GetCipher()) {
	case StandardProtocol::cipherActionPause:
		HandlePauseRequest();
		return;
	case StandardProtocol::cipherActionRun:
		HandleRunRequest();
		return;
	case StandardProtocol::cipherActionDelete:
		if (Application::IsRunning()) {
			HandlePauseRequest();
		}
		HandleDeleteRequest();
		return;
	case StandardProtocol::cipherActionModify:
		HandleModifyRequest(data.GetData());
		return;
	case StandardProtocol::cipherActionHello:
		HandleHello(connection);
		return;
	case StandardProtocol::cipherMetadataRequest: {
		if (!m_metadata.empty()) {
			StandardProtocol::Data metadataData{ StandardProtocol::cipherMetadataResponse };
			metadataData.SetData(0, m_metadata);
			StandardProtocol::Send(connection, metadataData);
			return;
		}

		//! Wrap into GenerateMetadata function to avoid code duplication and mess
		std::format_to(std::back_inserter(m_metadata), "{{\"mutable\":{{");

#define TMP_MSAPI_APPLICATION_NAME_PART                                                                                \
	std::format_to(std::back_inserter(m_metadata), "\"{}\":{{\"name\":\"{}\",\"type\":\"", id, parameter.m_name);

#define TMP_MSAPI_APPLICATION_MIN_MAX_PART                                                                             \
	if (parameter.m_min.has_value()) {                                                                                 \
		std::format_to(std::back_inserter(m_metadata), ",\"min\":{}", _S(std::get<S>(parameter.m_min.value())));       \
	}                                                                                                                  \
	if (parameter.m_max.has_value()) {                                                                                 \
		std::format_to(std::back_inserter(m_metadata), ",\"max\":{}", _S(std::get<S>(parameter.m_max.value())));       \
	}

#define TMP_MSAPI_APPLICATION_EMPTY_PART                                                                               \
	std::format_to(std::back_inserter(m_metadata), ",\"canBeEmpty\":{}", _S(parameter.m_canBeEmpty));

#define TMP_MSAPI_APPLICATION_ADDITIONAL_PART                                                                          \
	if (!parameter.m_stringInterpretation.empty()) {                                                                   \
		std::format_to(                                                                                                \
			std::back_inserter(m_metadata), ",\"stringInterpretation\":{}", parameter.m_stringInterpretation);         \
	}

		for (const auto& [id, parameter] : m_parameters) {
			TMP_MSAPI_APPLICATION_NAME_PART;
			std::visit(
				[this, &parameter](const auto& arg) {
					using T = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(arg)>>>;
					using S = remove_optional_t<T>;
					if constexpr (std::is_same_v<T, int8_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int8\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, int16_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int16\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, int32_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int32\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, int64_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int64\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, uint8_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint8\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, uint16_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint16\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, uint32_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint32\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, uint64_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint64\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, double>) {
						std::format_to(std::back_inserter(m_metadata), "Double\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
					}
					else if constexpr (std::is_same_v<T, float>) {
						std::format_to(std::back_inserter(m_metadata), "Float\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
					}
					else if constexpr (std::is_same_v<T, bool>) {
						std::format_to(std::back_inserter(m_metadata), "Bool\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<int8_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalInt8\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<int16_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalInt16\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<int32_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalInt32\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<int64_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalInt64\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<uint8_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalUint8\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<uint16_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalUint16\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<uint32_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalUint32\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<uint64_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalUint64\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<double>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalDouble\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::optional<float>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalFloat\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, std::string>) {
						std::format_to(std::back_inserter(m_metadata), "String\"");
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, Timer>) {
						std::format_to(std::back_inserter(m_metadata), "Timer\"");
						TMP_MSAPI_APPLICATION_EMPTY_PART;
					}
					else if constexpr (std::is_same_v<T, Timer::Duration>) {
						std::format_to(std::back_inserter(m_metadata), "Duration\"");
						if (parameter.m_min.has_value()) {
							std::format_to(std::back_inserter(m_metadata), ",\"min\":{}",
								_S(std::get<Timer::Duration>(parameter.m_min.value()).GetNanoseconds()));
						}
						if (parameter.m_max.has_value()) {
							std::format_to(std::back_inserter(m_metadata), ",\"max\":{}",
								_S(std::get<Timer::Duration>(parameter.m_max.value()).GetNanoseconds()));
						}
						TMP_MSAPI_APPLICATION_EMPTY_PART;
						std::format_to(std::back_inserter(m_metadata), ",\"durationType\":\"{}\"",
							Timer::Duration::EnumToString(parameter.m_durationType));
					}
					else if constexpr (std::is_same_v<T, TableData>) {
						std::format_to(std::back_inserter(m_metadata), "TableData\"");
						TMP_MSAPI_APPLICATION_EMPTY_PART;

#define TMP_MSAPI_APPLICATION_TABLE_COLUMNS_PART                                                                       \
	const auto& columns{ reinterpret_cast<const TableBase*>(arg)->GetColumns() };                                      \
	if (columns == nullptr) {                                                                                          \
		std::format_to(std::back_inserter(m_metadata), ",\"columns\":null");                                           \
	}                                                                                                                  \
	else {                                                                                                             \
		std::format_to(std::back_inserter(m_metadata), ",\"columns\":{{");                                             \
		if (!columns->empty()) {                                                                                       \
			auto begin{ columns->begin() }, end{ columns->end() };                                                     \
			std::format_to(std::back_inserter(m_metadata), "\"{}\":{{\"type\":\"{}\"", begin->id,                      \
				StandardType::EnumToString(begin->type));                                                              \
			if (!begin->metadata.empty()) {                                                                            \
				std::format_to(std::back_inserter(m_metadata), ",{}}}", begin->metadata);                              \
			}                                                                                                          \
			else {                                                                                                     \
				std::format_to(std::back_inserter(m_metadata), "}}");                                                  \
			}                                                                                                          \
                                                                                                                       \
			while (++begin != end) {                                                                                   \
				std::format_to(std::back_inserter(m_metadata), ",\"{}\":{{\"type\":\"{}\"", begin->id,                 \
					StandardType::EnumToString(begin->type));                                                          \
				if (!begin->metadata.empty()) {                                                                        \
					std::format_to(std::back_inserter(m_metadata), ",{}}}", begin->metadata);                          \
				}                                                                                                      \
				else {                                                                                                 \
					std::format_to(std::back_inserter(m_metadata), "}}");                                              \
				}                                                                                                      \
			}                                                                                                          \
		}                                                                                                              \
		std::format_to(std::back_inserter(m_metadata), "}}");                                                          \
	}

						TMP_MSAPI_APPLICATION_TABLE_COLUMNS_PART
					}
					else {
						static_assert(sizeof(T) + 1 == 0, "Unsupported type for parameter");
					}
					std::format_to(std::back_inserter(m_metadata), "}},");
				},
				parameter.m_value);
		}
		if (!m_parameters.empty()) {
			m_metadata[m_metadata.size() - 1] = '}';
		}

#undef TMP_MSAPI_APPLICATION_EMPTY_PART
#undef TMP_MSAPI_APPLICATION_MIN_MAX_PART

		std::format_to(std::back_inserter(m_metadata), ",\"const\":{{");

		for (const auto& [id, parameter] : m_constParameters) {
			TMP_MSAPI_APPLICATION_NAME_PART;
			std::visit(
				[this, &parameter](const auto& arg) {
					using T = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(arg)>>>;
					if constexpr (std::is_same_v<T, int8_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int8\"");
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, int16_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int16\"");
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, int32_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int32\"");
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, int64_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int64\"");
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, uint8_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint8\"");
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, uint16_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint16\"");
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, uint32_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint32\"");
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, uint64_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint64\"");
						TMP_MSAPI_APPLICATION_ADDITIONAL_PART;
					}
					else if constexpr (std::is_same_v<T, double>) {
						std::format_to(std::back_inserter(m_metadata), "Double\"");
					}
					else if constexpr (std::is_same_v<T, float>) {
						std::format_to(std::back_inserter(m_metadata), "Float\"");
					}
					else if constexpr (std::is_same_v<T, bool>) {
						std::format_to(std::back_inserter(m_metadata), "Bool\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<int8_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalInt8\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<int16_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalInt16\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<int32_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalInt32\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<int64_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalInt64\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<uint8_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalUint8\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<uint16_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalUint16\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<uint32_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalUint32\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<uint64_t>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalUint64\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<double>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalDouble\"");
					}
					else if constexpr (std::is_same_v<T, std::optional<float>>) {
						std::format_to(std::back_inserter(m_metadata), "OptionalFloat\"");
					}
					else if constexpr (std::is_same_v<T, std::string>) {
						std::format_to(std::back_inserter(m_metadata), "String\"");
					}
					else if constexpr (std::is_same_v<T, Timer>) {
						std::format_to(std::back_inserter(m_metadata), "Timer\"");
					}
					else if constexpr (std::is_same_v<T, Timer::Duration>) {
						std::format_to(std::back_inserter(m_metadata), "Duration\"");
					}
					else if constexpr (std::is_same_v<T, TableData>) {
						std::format_to(std::back_inserter(m_metadata), "TableData\"");
						TMP_MSAPI_APPLICATION_TABLE_COLUMNS_PART

#undef TMP_MSAPI_APPLICATION_TABLE_COLUMNS_PART
					}
					else {
						static_assert(sizeof(T) + 1 == 0, "Unsupported type for parameter");
					}

					std::format_to(std::back_inserter(m_metadata), "}},");
				},
				parameter.m_value);
		}
		if (!m_constParameters.empty()) {
			m_metadata[m_metadata.size() - 1] = '}';
		}
		std::format_to(std::back_inserter(m_metadata), "}}");

		StandardProtocol::Data metadataData{ StandardProtocol::cipherMetadataResponse };
		metadataData.SetData(0, m_metadata);
		StandardProtocol::Send(connection, metadataData);
	}
#undef TMP_MSAPI_APPLICATION_ADDITIONAL_PART
#undef TMP_MSAPI_APPLICATION_NAME_PART
		return;
	case StandardProtocol::cipherParametersResponse:
		HandleParameters(connection, data.GetData());
		return;
	case StandardProtocol::cipherParametersRequest: {
		StandardProtocol::Data data{ StandardProtocol::cipherParametersResponse };
		for (const auto& [id, parameter] : m_parameters) {
			std::visit(
				[&data, &id](const auto& arg) {
					using T = std::remove_pointer_t<std::decay_t<decltype(arg)>>;
					if constexpr (is_standard_type<T> && !std::is_same_v<T, TableData>) {
						data.SetData(id, *arg);
					}
					else if constexpr (std::is_same_v<T, TableData>) {
						data.SetData(id, *reinterpret_cast<TableBase*>(arg));
					}
					else {
						static_assert(sizeof(T) + 1 == 0, "Unsupported type for parameter");
					}
				},
				parameter.m_value);
		}
		for (const auto& [id, parameter] : m_constParameters) {
			std::visit(
				[&data, &id](const auto& arg) {
					using T = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(arg)>>>;
					if constexpr (is_standard_type<T> && !std::is_same_v<T, TableData>) {

						data.SetData(id, static_cast<T>(*arg));
					}
					else if constexpr (std::is_same_v<T, TableData>) {
						data.SetData(id, *reinterpret_cast<const TableBase*>(arg));
					}
					else {
						static_assert(sizeof(T) + 1 == 0, "Unsupported type for parameter");
					}
				},
				parameter.m_value);
		}
		StandardProtocol::Send(connection, data);
	}
		return;
	case StandardProtocol::cipherMetadataResponse: {
		const auto it{ data.GetData().find(0) };
		if (it == data.GetData().end()) {
			LOG_ERROR("Metadata is empty, connection: " + _S(connection));
			return;
		}
		if (!std::holds_alternative<std::string>(it->second)) {
			LOG_ERROR("Unexpected metadata type: " + data.ToString() + ", connection: " + _S(connection));
			return;
		}

		HandleMetadata(connection, std::get<std::string>(it->second));
	}
		return;
	default:
		LOG_ERROR("Unexpected data for collecting: " + data.ToString() + ", connection: " + _S(connection));
		return;
	}
}

void Application::HandleRunRequest() { LOG_PROTOCOL("Action is skipped"); }

void Application::HandlePauseRequest() { LOG_PROTOCOL("Action is skipped"); }

void Application::HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate)
{
	LOG_PROTOCOL("Default merge parameters action");
	MergeParameters(parametersUpdate);
}

void Application::HandleDeleteRequest() { LOG_PROTOCOL("Action is skipped"); }

void Application::HandleHello([[maybe_unused]] const int connection) { LOG_PROTOCOL("Action is skipped"); }

void Application::HandleMetadata(
	[[maybe_unused]] const int connection, [[maybe_unused]] const std::string_view metadata)
{
	LOG_PROTOCOL("Action is skipped");
}

void Application::HandleParameters([[maybe_unused]] const int connection,
	[[maybe_unused]] const std::map<size_t, std::variant<standardTypes>>& parameters)
{
	LOG_PROTOCOL("Action is skipped");
}

void Application::HandleDisconnect(const int id)
{
	LOG_PROTOCOL("Id: " + _S(id));
	HandlePauseRequest();
}

void Application::HandleReconnect(const int id)
{
	LOG_PROTOCOL("Id: " + _S(id));
	HandleRunRequest();
}

void Application::SetName(const std::string& name) { m_name = name; }

void Application::RegisterParameter(const size_t id, Parameter&& parameter)
{
	if (m_parameters.find(id) == m_parameters.end()) {
		if (const auto result{ m_parameters.emplace(id, std::move(parameter)) }; result.second) {
			if (result.first->second.RegisterValidation(id)) {
				return;
			}

			m_errorParameters.emplace(id, &result.first->second);
			return;
		}

		LOG_ERROR("Parameter " + parameter.m_name + "(" + _S(id) + ") is not registered");
		return;
	}

	LOG_WARNING("Parameter " + parameter.m_name + "(" + _S(id) + ") already exists, registration is skipped");
}

void Application::RegisterConstParameter(const size_t id, ConstParameter&& parameter)
{
	if (m_constParameters.find(id) == m_constParameters.end()) {
		if (const auto result{ m_constParameters.emplace(id, std::move(parameter)) }; result.second) {
			std::visit(
				[&id, &result](auto&& arg) {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (is_integer_type_const_ptr<T>) {
						if (result.first->second.m_printFunc == nullptr) {
							LOG_INFO(std::format("Const parameter {}({}) is registered. Value: {}",
								result.first->second.m_name, id, _S(arg)));
							return;
						}

						LOG_INFO_NEW("Const parameter {}({}) is registered. Value: {}", result.first->second.m_name, id,
							result.first->second.m_printFunc(static_cast<int>(*arg)));
					}
					else if constexpr (std::is_same_v<T, const bool*>) {
						LOG_INFO(std::format("Const parameter {}({}) is registered. Value: {}",
							result.first->second.m_name, id, _S(arg)));
					}
					else if constexpr (is_float_type_const_ptr<T>) {
						LOG_INFO(std::format("Const parameter {}({}) is registered. Value: {}",
							result.first->second.m_name, id, _S(arg)));
					}
					else if constexpr (std::is_same_v<T, const std::string*>) {
						LOG_INFO("Const parameter " + result.first->second.m_name + "(" + _S(id)
							+ ") is registered. Value: " + *arg);
					}
					else if constexpr (std::is_same_v<T, const Timer*>) {
						LOG_INFO("Const parameter " + result.first->second.m_name + "(" + _S(id)
							+ ") is registered. Value: " + arg->ToString());
					}
					else if constexpr (std::is_same_v<T, const Timer::Duration*>) {
						LOG_INFO("Const parameter " + result.first->second.m_name + "(" + _S(id)
							+ ") is registered. Value: " + arg->ToString(result.first->second.m_durationType));
					}
					else if constexpr (std::is_same_v<T, const TableData*>) {
						LOG_INFO("Const parameter " + result.first->second.m_name + "(" + _S(id)
							+ ") is registered. Value: " + reinterpret_cast<const TableBase*>(arg)->ToString());
					}
					else {
						static_assert(sizeof(T) + 1 == 0, "Unsupported type for const parameter");
					}
				},
				result.first->second.m_value);
			return;
		}

		LOG_ERROR("Const parameter " + parameter.m_name + "(" + _S(id) + ") is not registered");
		return;
	}

	LOG_WARNING("Const parameter " + parameter.m_name + "(" + _S(id) + ") already exists, registration is skipped");
}

void Application::MergeParameters(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate)
{
	for (const auto& [id, value] : parametersUpdate) {
		MergeParameter(id, value);
	}
}

void Application::MergeParameter(const size_t id, const std::variant<standardTypes>& value)
{
	if (auto it{ m_parameters.find(id) }; it != m_parameters.end()) {
		if (it->second.Merge(id, value)) {
			m_errorParameters.erase(id);
		}
		else if (m_errorParameters.find(id) == m_errorParameters.end()) {
			m_errorParameters.emplace(id, &it->second);
		}
		return;
	}

	LOG_WARNING("Parameter with id " + _S(id) + " is not found or it is const, merging is skipped");
}

void Application::SetCustomError(const size_t id, const std::string& error)
{
	if (auto it{ m_parameters.find(id) }; it != m_parameters.end()) {
		if (it->second.m_error.empty()) {
			it->second.m_error = "Parameter " + it->second.m_name + "(" + _S(id) + ") custom error: " + error;
			m_errorParameters.emplace(id, &it->second);
		}
		else {
			it->second.m_error += ". Custom error: " + error;
		}

		LOG_WARNING("Parameter " + it->second.m_name + "(" + _S(id) + ") is set custom error: " + error);
		return;
	}

	LOG_WARNING("Parameter with id " + _S(id) + " is not found, set custom error is skipped");
}

const std::map<size_t, Application::Parameter>& Application::GetParameters() const noexcept { return m_parameters; }

const std::map<size_t, Application::ConstParameter>& Application::GetConstParameters() const noexcept
{
	return m_constParameters;
}

bool Application::AreParametersValid() const noexcept { return m_errorParameters.empty(); }

const std::map<size_t, const Application::Parameter* const>& Application::GetErrorParameters() const noexcept
{
	return m_errorParameters;
}

void Application::GetParameters(std::string& parameters) const
{
	parameters.clear();
	parameters += "Parameters:\n{";

	for (const auto& [id, parameter] : m_parameters) {
		parameters += "\n\t" + parameter.m_name + "(" + _S(id) + ") : ";
		std::visit(
			[&parameters, &parameter](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (is_integer_type_ptr<T>) {
					parameters
						+= parameter.m_printFunc == nullptr ? _S(arg) : parameter.m_printFunc(static_cast<int>(*arg));
				}
				else if constexpr (std::is_same_v<T, bool*>) {
					parameters += _S(arg);
				}
				else if constexpr (is_integer_type_optional_ptr<T>) {
					if (arg->has_value()) {
						parameters += parameter.m_printFunc == nullptr
							? _S(arg->value())
							: parameter.m_printFunc(static_cast<int>(arg->value()));
					}
				}
				else if constexpr (is_float_type_ptr<T>) {
					parameters += _S(arg);
				}
				else if constexpr (is_float_type_optional_ptr<T>) {
					if (arg->has_value()) {
						parameters += _S(arg->value());
					}
				}
				else if constexpr (std::is_same_v<T, std::string*>) {
					parameters += *arg;
				}
				else if constexpr (std::is_same_v<T, Timer*>) {
					parameters += arg->ToString();
				}
				else if constexpr (std::is_same_v<T, Timer::Duration*>) {
					parameters += arg->ToString(parameter.m_durationType);
				}
				else if constexpr (std::is_same_v<T, TableData*>) {
					parameters += reinterpret_cast<const TableBase*>(arg)->ToString();
				}
				else {
					static_assert(sizeof(T) + 1 == 0, "Unsupported type for parameter");
				}
			},
			parameter.m_value);
	}

	for (const auto& [id, parameter] : m_constParameters) {
		parameters += "\n\t" + parameter.m_name + "(" + _S(id) + ") const : ";
		std::visit(
			[&parameters, &parameter](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (is_integer_type_const_ptr<T>) {
					parameters
						+= (parameter.m_printFunc == nullptr ? _S(arg) : parameter.m_printFunc(static_cast<int>(*arg)));
				}
				else if constexpr (std::is_same_v<T, const bool*>) {
					parameters += _S(arg);
				}
				else if constexpr (is_float_type_const_ptr<T>) {
					parameters += _S(arg);
				}
				else if constexpr (std::is_same_v<T, const std::string*>) {
					parameters += *arg;
				}
				else if constexpr (std::is_same_v<T, const Timer*>) {
					parameters += arg->ToString();
				}
				else if constexpr (std::is_same_v<T, const Timer::Duration*>) {
					parameters += arg->ToString(parameter.m_durationType);
				}
				else if constexpr (std::is_same_v<T, const TableData*>) {
					parameters += reinterpret_cast<const TableBase*>(arg)->ToString();
				}
				else {
					static_assert(sizeof(T) + 1 == 0, "Unsupported type for const parameter");
				}
			},
			parameter.m_value);
	}

	parameters += "\n}";
}

const std::string& Application::GetName() const noexcept { return m_name; }

void Application::SetState(const State state) { m_state = state; }

Application::State Application::GetState() const { return m_state; }

bool Application::IsRunning() const { return m_state == State::Running; }

std::string_view Application::EnumToString(const State state)
{
	static_assert(U(State::Max) == 3, "Missed string description of application state");

	switch (state) {
	case State::Undefined:
		return "Undefined";
	case State::Paused:
		return "Paused";
	case State::Running:
		return "Running";
	case State::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown application state enum: " + _S(U(state)));
		return "Unknown";
	}
}

bool Application::UNITTEST()
{
	LOG_INFO_UNITTEST("MSAPI Application");
	MSAPI::Test t;

	Application app;

	RETURN_IF_FALSE(t.Assert(app.GetName().empty(), true, "application name is empty"));
	RETURN_IF_FALSE(t.Assert(app.GetState(), State::Paused, "application state is Paused"));

	std::string parameters;
	app.GetParameters(parameters);
	RETURN_IF_FALSE(
		t.Assert(parameters, "Parameters:\n{\n\tName(2000001) const : \n\tApplication state(2000002) const : Paused\n}",
			"application parameters are correct"));
	app.SetName("TestApp");
	RETURN_IF_FALSE(t.Assert(app.GetName(), "TestApp", "application name is TestApp"));
	app.SetState(State::Running);
	RETURN_IF_FALSE(t.Assert(app.GetState(), State::Running, "application state is Running"));
	app.GetParameters(parameters);
	RETURN_IF_FALSE(t.Assert(parameters,
		"Parameters:\n{\n\tName(2000001) const : TestApp\n\tApplication state(2000002) const : Running\n}",
		"application parameters are correct after state change"));

	/**************************
	 * @param parametersSize Expected size of parameters without default Application parameters.
	 */
	const auto check{ [&t] [[nodiscard]] (const size_t checkIndex, const Application& app,
						  const std::string_view expectedParameters, const size_t parametersSize,
						  const std::map<size_t, std::string>& errors) {
		std::string parameters;
		app.GetParameters(parameters);
		RETURN_IF_FALSE(
			t.Assert(parameters, expectedParameters, "check[" + _S(checkIndex) + "] parameters are correct"));
		// -2 for default parameters
		const auto size{ app.GetParameters().size() + app.GetConstParameters().size() - 2 };
		RETURN_IF_FALSE(t.Assert(size, parametersSize, "check[" + _S(checkIndex) + "] parameters size is correct"));

		if (errors.empty()) {
			RETURN_IF_FALSE(
				t.Assert(app.AreParametersValid(), true, "check[" + _S(checkIndex) + "] parameters are valid"));
			RETURN_IF_FALSE(
				t.Assert(app.GetErrorParameters().empty(), true, "check[" + _S(checkIndex) + "] errors are empty"));
		}
		else {
			RETURN_IF_FALSE(
				t.Assert(app.AreParametersValid(), false, "check[" + _S(checkIndex) + "] parameters are not valid"));
			const auto& errorsMap = app.GetErrorParameters();
			RETURN_IF_FALSE(
				t.Assert(errorsMap.size(), errors.size(), "check[" + _S(checkIndex) + "] errors size is correct"));
			for (const auto& [id, parameter] : errorsMap) {
				const auto it{ errors.find(id) };
				RETURN_IF_FALSE(t.Assert(it != errors.end(), true,
					"check[" + _S(checkIndex) + "] error found for " + parameter->m_name + "(" + _S(id) + ")"));
				RETURN_IF_FALSE(t.Assert(parameter->m_error, it->second,
					"check[" + _S(checkIndex) + "] error value for " + parameter->m_name + "(" + _S(id)
						+ ") is correct"));
			}
		}

		return true;
	} };

#define POEF(v)                                                                                                        \
	std::string { v.has_value() ? (*f)(v.value()) : "" }
#define PE(v) _S(static_cast<const UnderlyingType*>(static_cast<const void*>(&v)))
#define ASE(v)                                                                                                         \
	std::format("\n\tName(2000001) const : TestApp\n\tApplication state(2000002) const : {}\n}}", EnumToString(v))

	/**************************
	 * @param v1 First registered value. Should not be a temporal l-value.
	 * @param v2 Update for v1 by merge.
	 * @param v3 Update for v1 by direct assignment.
	 * @param v4 First registered value with min and max, should be equal to min. It should be safely to reduce it by 1.
	 * Should not be a temporal l-value.
	 * @param v5 First registered value with min and max, should be equal to max. It should be safely to increase it
	 * by 1. Should not be a temporal l-value.
	 */
	const auto checkNotEmpty{ [&check]<typename T> [[nodiscard]] (T & v1, const T& v2, const T& v3, T& v4, T& v5,
								  const State state, std::string_view (*const f)(remove_optional_t<T>) = nullptr) {
		/*
			int8_t = int8_t
			Enum : int8_t = int8_t
			std::optional<int8_t> = std::optional<int8_t>
		*/
		using UnderlyingType = std::conditional_t<is_optional_v<T>, T,
			std::conditional_t<std::is_enum_v<T>, safe_underlying_type_t<T>, T>>;

		/*
			int8_t = int8_t
			Enum : int8_t = int8_t
			std::optional<int8_t> = int8_t
		*/
		using UnderlyingPrimitiveType
			= std::conditional_t<std::is_enum_v<T>, safe_underlying_type_t<T>, remove_optional_t<T>>;

		Application app;
		app.SetName("TestApp");
		app.SetState(state);

		const T min{ v4 };
		const T max{ v5 };

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(1, { "Some name", &v1, Timer::Duration::Type::Days });
			app.RegisterParameter(1, { "Some name", &v1, Timer::Duration::Type::Days });
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterParameter(1, { "Some name", &v1, nullptr, true });
				app.RegisterParameter(1, { "Some name", &v1 });
			}
			else {
				app.RegisterParameter(1, { "Some name", &v1, f, true });
				app.RegisterParameter(1, { "Some name", &v1, f });
			}
		}
		else {
			app.RegisterParameter(1, { "Some name", &v1 });
			app.RegisterParameter(1, { "Some name", &v1 });
		}
		std::string expectedParameters;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + v1.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters
					= "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v1)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v1)) + ASE(state);
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v1) + ASE(state);
		}
		RETURN_IF_FALSE(check(0, app, expectedParameters, 1, {}));

		T tmp{ v2 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 1, static_cast<UnderlyingType>(tmp) } });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + tmp.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 1, static_cast<UnderlyingType>(tmp) } });
			if (f == nullptr) {
				expectedParameters
					= "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(tmp)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(tmp)) + ASE(state);
			}
		}
		else {
			app.MergeParameters({ { 1, static_cast<UnderlyingType>(tmp) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(tmp) + ASE(state);
		}
		RETURN_IF_FALSE(check(1, app, expectedParameters, 1, {}));

		v1 = v3;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters
					= "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + ASE(state);
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + ASE(state);
		}
		RETURN_IF_FALSE(check(2, app, expectedParameters, 1, {}));

		T tmp2;
		std::map<size_t, std::string> expectedErrors;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			tmp2 = min - Timer::Duration::CreateDays(1);
			app.RegisterParameter(2, { "Some name", &tmp2, Timer::Duration::Type::Days, min });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
				"Parameter Some name(2) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days) + " < "
					+ min.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			tmp2 = static_cast<T>(static_cast<UnderlyingType>(min) - 1);
			if (f == nullptr) {
				app.RegisterParameter(2, { "Some name", &tmp2 });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2)) + ASE(state);
				expectedErrors = { { 2,
					"Parameter Some name(2) is less than min value: " + _S(static_cast<UnderlyingType>(tmp2)) + " < "
						+ _S(static_cast<UnderlyingType>(min) + 1) } };
			}
			else {
				app.RegisterParameter(2, { "Some name", &tmp2, f });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + ASE(state);
				expectedErrors = { { 2,
					"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
						+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) } };
			}
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			tmp2 = min - 1;
			app.RegisterParameter(2, { "Some name", &tmp2, min });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			tmp2 = min.value() - 1;
			app.RegisterParameter(2, { "Some name", &tmp2, min });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}
		RETURN_IF_FALSE(check(3, app, expectedParameters, 2, expectedErrors));

		T tmp3;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			tmp3 = max + Timer::Duration::CreateDays(1);
			app.RegisterParameter(3, { "Some name", &tmp3, Timer::Duration::Type::Days, {}, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			tmp3 = static_cast<T>(static_cast<UnderlyingType>(max) + 1);
			if (f == nullptr) {
				app.RegisterParameter(3, { "Some name", &tmp3 });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				app.RegisterParameter(3, { "Some name", &tmp3, f });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			tmp3 = max + 1;
			app.RegisterParameter(3, { "Some name", &tmp3, {}, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			tmp3 = max.value() + 1;
			app.RegisterParameter(3, { "Some name", &tmp3, {}, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(4, app, expectedParameters, 3, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(4, { "Some name", &v4, Timer::Duration::Type::Days, min, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + v4.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterParameter(4, { "Some name", &v4, nullptr, true });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(v4)) + ASE(state);
			}
			else {
				app.RegisterParameter(4, { "Some name", &v4, f, true });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(v4)) + ASE(state);
			}
		}
		else {
			app.RegisterParameter(4, { "Some name", &v4, min, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(v4) + ASE(state);
		}
		RETURN_IF_FALSE(check(5, app, expectedParameters, 4, expectedErrors));

		T save4{ v4 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp2) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + tmp2.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) },
				{ 4,
					"Parameter Some name(4) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days)
						+ " < " + min.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp2) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(tmp2)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) },
					{ 4,
						"Parameter Some name(4) is less than min value: " + _S(static_cast<UnderlyingType>(tmp2))
							+ " < " + _S(static_cast<UnderlyingType>(min)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(tmp2)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) },
						  { 4,
							  "Parameter Some name(4) is less than min value: " + std::string((*f)(tmp2)) + " < "
								  + std::string((*f)(min)) } };
			}
		}
		else {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp2) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) },
				{ 4, "Parameter Some name(4) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}
		RETURN_IF_FALSE(check(6, app, expectedParameters, 4, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp3) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + tmp3.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) },
				{ 4,
					"Parameter Some name(4) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp3) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(tmp3)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) },
					{ 4,
						"Parameter Some name(4) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(tmp3)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) },
						  { 4,
							  "Parameter Some name(4) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(tmp3) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(tmp3) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) },
				{ 4, "Parameter Some name(4) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(7, app, expectedParameters, 4, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(save4) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(save4) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 4, static_cast<UnderlyingType>(save4) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(8, app, expectedParameters, 4, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(5, { "Some name", &v5, Timer::Duration::Type::Days, min, max });
			app.RegisterParameter(5, { "Some name", &v5, Timer::Duration::Type::Days, min, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + v5.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterParameter(5, { "Some name", &v5 });
				app.RegisterParameter(5, { "Some name", &v5 });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(v5)) + ASE(state);
			}
			else {
				app.RegisterParameter(5, { "Some name", &v5, f });
				app.RegisterParameter(5, { "Some name", &v5, f });
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + "\n\tSome name(4) : "
					+ std::string((*f)(save4)) + "\n\tSome name(5) : " + std::string((*f)(v5)) + ASE(state);
			}
		}
		else {
			app.RegisterParameter(5, { "Some name", &v5, min, max });
			app.RegisterParameter(5, { "Some name", &v5, min, max });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4) + "\n\tSome name(5) : " + _S(v5)
				+ ASE(state);
		}
		RETURN_IF_FALSE(check(9, app, expectedParameters, 5, expectedErrors));

		T save5{ v5 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp3) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + tmp3.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) },
				{ 5,
					"Parameter Some name(5) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp3) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(tmp3)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) },
					{ 5,
						"Parameter Some name(5) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + "\n\tSome name(4) : "
					+ std::string((*f)(save4)) + "\n\tSome name(5) : " + std::string((*f)(tmp3)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) },
						  { 5,
							  "Parameter Some name(5) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp3) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(tmp3) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) },
				{ 5, "Parameter Some name(5) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(10, app, expectedParameters, 5, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp2) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + tmp2.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) },
				{ 5,
					"Parameter Some name(5) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days)
						+ " < " + min.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp2) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(tmp2)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) },
					{ 5,
						"Parameter Some name(5) is less than min value: " + _S(static_cast<UnderlyingType>(tmp2))
							+ " < " + _S(static_cast<UnderlyingType>(min) + 1) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + "\n\tSome name(4) : "
					+ std::string((*f)(save4)) + "\n\tSome name(5) : " + std::string((*f)(tmp2)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) },
						  { 5,
							  "Parameter Some name(5) is less than min value: " + std::string((*f)(tmp2)) + " < "
								  + std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) } };
			}
		}
		else {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(tmp2) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) },
				{ 5, "Parameter Some name(5) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}
		RETURN_IF_FALSE(check(11, app, expectedParameters, 5, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(save5) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + tmp2.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(save5) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3)) + "\n\tSome name(2) : "
					+ std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3)) + "\n\tSome name(4) : "
					+ std::string((*f)(save4)) + "\n\tSome name(5) : " + std::string((*f)(save5)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 5, static_cast<UnderlyingType>(save5) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(12, app, expectedParameters, 5, expectedErrors));

		const T save6{ v1 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterConstParameter(6, { "Some name", &save6, Timer::Duration::Type::Days });
			app.RegisterConstParameter(6, { "Some name", &save6, Timer::Duration::Type::Days });
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterConstParameter(6, { "Some name", &save6 });
				app.RegisterConstParameter(6, { "Some name", &save6 });
			}
			else {
				app.RegisterConstParameter(6, { "Some name", &save6, f });
				app.RegisterConstParameter(6, { "Some name", &save6, f });
			}
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			app.RegisterConstParameter(6, { "Some name", &save6 });
			app.RegisterConstParameter(6, { "Some name", &save6 });
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			app.RegisterConstParameter(6, { "Some name", &save6.value() });
			app.RegisterConstParameter(6, { "Some name", &save6.value() });
		}

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(6) const : " + save6.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5))
					+ "\n\tSome name(6) const : " + _S(static_cast<UnderlyingType>(save6)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + "\n\tSome name(5) : "
					+ std::string((*f)(save5)) + "\n\tSome name(6) const : " + std::string((*f)(save6)) + ASE(state);
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + "\n\tSome name(6) const : " + _S(save6) + ASE(state);
		}
		RETURN_IF_FALSE(check(13, app, expectedParameters, 6, expectedErrors));

		app.MergeParameters({ { 6, static_cast<UnderlyingType>(tmp) } });
		RETURN_IF_FALSE(check(14, app, expectedParameters, 6, expectedErrors));

		T save7{ v1 };
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterConstParameter(7, { "Some name", &save7, Timer::Duration::Type::Days });
			app.RegisterConstParameter(7, { "Some name", &save7, Timer::Duration::Type::Days });
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				app.RegisterConstParameter(7, { "Some name", &save7 });
				app.RegisterConstParameter(7, { "Some name", &save7 });
			}
			else {
				app.RegisterConstParameter(7, { "Some name", &save7, f });
				app.RegisterConstParameter(7, { "Some name", &save7, f });
			}
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			app.RegisterConstParameter(7, { "Some name", &save7 });
			app.RegisterConstParameter(7, { "Some name", &save7 });
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			app.RegisterConstParameter(7, { "Some name", &save7.value() });
			app.RegisterConstParameter(7, { "Some name", &save7.value() });
		}

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(6) const : " + save6.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(7) const : " + save7.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(v3))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5))
					+ "\n\tSome name(6) const : " + _S(static_cast<UnderlyingType>(save6))
					+ "\n\tSome name(7) const : " + _S(static_cast<UnderlyingType>(save7)) + ASE(state);
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(v3))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + "\n\tSome name(5) : "
					+ std::string((*f)(save5)) + "\n\tSome name(6) const : " + std::string((*f)(save6))
					+ "\n\tSome name(7) const : " + std::string((*f)(save7)) + ASE(state);
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v3) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + "\n\tSome name(6) const : " + _S(save6)
				+ "\n\tSome name(7) const : " + _S(save7) + ASE(state);
		}
		RETURN_IF_FALSE(check(15, app, expectedParameters, 7, expectedErrors));

		app.MergeParameters({ { 7, static_cast<UnderlyingType>(tmp) } });
		RETURN_IF_FALSE(check(16, app, expectedParameters, 7, expectedErrors));

		bool boolean{ false };
		app.MergeParameters({ { 1, boolean } });
		app.MergeParameters({ { 2, boolean } });
		app.MergeParameters({ { 3, boolean } });
		app.MergeParameters({ { 4, boolean } });
		app.MergeParameters({ { 5, boolean } });
		app.MergeParameters({ { 6, boolean } });
		app.MergeParameters({ { 7, boolean } });
		RETURN_IF_FALSE(check(17, app, expectedParameters, 7, expectedErrors));

		app.MergeParameters({ { 1, boolean }, { 2, boolean }, { 3, boolean }, { 4, boolean }, { 5, boolean },
			{ 6, boolean }, { 7, boolean } });
		RETURN_IF_FALSE(check(18, app, expectedParameters, 7, expectedErrors));

		app.SetCustomError(1, "Some custom error 1");
		app.SetCustomError(2, "Some custom error 2");
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedErrors = { { 1, "Parameter Some name(1) custom error: Some custom error 1" },
				{ 2,
					"Parameter Some name(2) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days)
						+ " < " + min.ToString(Timer::Duration::Type::Days) + ". Custom error: Some custom error 2" },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedErrors = { { 1, "Parameter Some name(1) custom error: Some custom error 1" },
					{ 2,
						"Parameter Some name(2) is less than min value: " + _S(static_cast<UnderlyingType>(tmp2))
							+ " < " + _S(static_cast<UnderlyingType>(min) + 1)
							+ ". Custom error: Some custom error 2" },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedErrors = { { 1, "Parameter Some name(1) custom error: Some custom error 1" },
					{ 2,
						"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
							+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1)))
							+ ". Custom error: Some custom error 2" },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
							+ std::string((*f)(max)) } };
			}
		}
		else {
			expectedErrors = { { 1, "Parameter Some name(1) custom error: Some custom error 1" },
				{ 2,
					"Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min)
						+ ". Custom error: Some custom error 2" },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(19, app, expectedParameters, 7, expectedErrors));

		app.MergeParameters({ { 1, static_cast<UnderlyingType>(save4) } });
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(6) const : " + save6.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(7) const : " + save7.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = {
				{ 2,
					"Parameter Some name(2) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days)
						+ " < " + min.ToString(Timer::Duration::Type::Days) + ". Custom error: Some custom error 2" },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) }
			};
		}
		else if constexpr (std::is_enum_v<T>) {
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(tmp2))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5))
					+ "\n\tSome name(6) const : " + _S(static_cast<UnderlyingType>(save6))
					+ "\n\tSome name(7) const : " + _S(static_cast<UnderlyingType>(save7)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(tmp2)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1)
										   + ". Custom error: Some custom error 2" },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(save4))
					+ "\n\tSome name(2) : " + std::string((*f)(tmp2)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + "\n\tSome name(5) : "
					+ std::string((*f)(save5)) + "\n\tSome name(6) const : " + std::string((*f)(save6))
					+ "\n\tSome name(7) const : " + std::string((*f)(save7)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(tmp2)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1)))
								+ ". Custom error: Some custom error 2" },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(save4) + "\n\tSome name(2) : " + _S(tmp2)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + "\n\tSome name(6) const : " + _S(save6)
				+ "\n\tSome name(7) const : " + _S(save7) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min)
									   + ". Custom error: Some custom error 2" },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(20, app, expectedParameters, 7, expectedErrors));

		T save8;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			save8 = tmp2 - Timer::Duration::CreateDays(1);
		}
		else if constexpr (std::is_enum_v<T>) {
			save8 = static_cast<T>(static_cast<UnderlyingType>(tmp2) - 1);
		}
		else if constexpr (std::is_same_v<T, UnderlyingPrimitiveType>) {
			save8 = tmp2 - 1;
		}
		else if constexpr (std::is_same_v<T, std::optional<UnderlyingPrimitiveType>>) {
			save8 = tmp2.value() - 1;
		}
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + save8.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(3) : " + tmp3.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(4) : " + save4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(5) : " + save5.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(6) const : " + save6.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(7) const : " + save7.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
								   "Parameter Some name(2) is less than min value: "
									   + save8.ToString(Timer::Duration::Type::Days) + " < "
									   + min.ToString(Timer::Duration::Type::Days) },
				{ 3,
					"Parameter Some name(3) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
						+ " > " + max.ToString(Timer::Duration::Type::Days) } };
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
			if (f == nullptr) {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(2) : " + _S(static_cast<UnderlyingType>(save8))
					+ "\n\tSome name(3) : " + _S(static_cast<UnderlyingType>(tmp3))
					+ "\n\tSome name(4) : " + _S(static_cast<UnderlyingType>(save4))
					+ "\n\tSome name(5) : " + _S(static_cast<UnderlyingType>(save5))
					+ "\n\tSome name(6) const : " + _S(static_cast<UnderlyingType>(save6))
					+ "\n\tSome name(7) const : " + _S(static_cast<UnderlyingType>(save7)) + ASE(state);
				expectedErrors = { { 2,
									   "Parameter Some name(2) is less than min value: "
										   + _S(static_cast<UnderlyingType>(save8)) + " < "
										   + _S(static_cast<UnderlyingType>(min) + 1) },
					{ 3,
						"Parameter Some name(3) is greater than max value: " + _S(static_cast<UnderlyingType>(tmp3))
							+ " > " + _S(static_cast<UnderlyingType>(max)) } };
			}
			else {
				expectedParameters = "Parameters:\n{\n\tSome name(1) : " + std::string((*f)(save4))
					+ "\n\tSome name(2) : " + std::string((*f)(save8)) + "\n\tSome name(3) : " + std::string((*f)(tmp3))
					+ "\n\tSome name(4) : " + std::string((*f)(save4)) + "\n\tSome name(5) : "
					+ std::string((*f)(save5)) + "\n\tSome name(6) const : " + std::string((*f)(save6))
					+ "\n\tSome name(7) const : " + std::string((*f)(save7)) + ASE(state);
				expectedErrors
					= { { 2,
							"Parameter Some name(2) is less than min value: " + std::string((*f)(save8)) + " < "
								+ std::string((*f)(static_cast<T>(static_cast<UnderlyingType>(min) + 1))) },
						  { 3,
							  "Parameter Some name(3) is greater than max value: " + std::string((*f)(tmp3)) + " > "
								  + std::string((*f)(max)) } };
			}
		}
		else {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(save4) + "\n\tSome name(2) : " + _S(save8)
				+ "\n\tSome name(3) : " + _S(tmp3) + "\n\tSome name(4) : " + _S(save4)
				+ "\n\tSome name(5) : " + _S(save5) + "\n\tSome name(6) const : " + _S(save6)
				+ "\n\tSome name(7) const : " + _S(save7) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(save8) + " < " + _S(min) },
				{ 3, "Parameter Some name(3) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}
		RETURN_IF_FALSE(check(21, app, expectedParameters, 7, expectedErrors));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
		}
		else if constexpr (std::is_enum_v<T>) {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
		}
		else {
			app.MergeParameters({ { 2, static_cast<UnderlyingType>(save8) } });
		}
		RETURN_IF_FALSE(check(22, app, expectedParameters, 7, expectedErrors));

		return true;
	} };

	{
		int8_t param1{ -3 };
		int8_t param4{ -23 };
		int8_t param5{ 23 };
		RETURN_IF_FALSE(checkNotEmpty(param1, int8_t{ 11 }, int8_t{ 0 }, param4, param5, State::Undefined));
	}

	{
		int16_t param1{ -3331 };
		int16_t param4{ -21981 };
		int16_t param5{ 31938 };
		RETURN_IF_FALSE(checkNotEmpty(param1, int16_t{ 5983 }, int16_t{ 0 }, param4, param5, State::Paused));
	}

	{
		int32_t param1{ -3331 };
		int32_t param4{ -93981 };
		int32_t param5{ 105938 };
		RETURN_IF_FALSE(checkNotEmpty(param1, int32_t{ 5983 }, int32_t{ 0 }, param4, param5, State::Running));
	}

	{
		int64_t param1{ -3331 };
		int64_t param4{ -93981 };
		int64_t param5{ 105938 };
		RETURN_IF_FALSE(checkNotEmpty(param1, int64_t{ 5983 }, int64_t{ 0 }, param4, param5, State::Running));
	}

	{
		uint8_t param1{ 10 };
		uint8_t param4{ 20 };
		uint8_t param5{ 30 };
		RETURN_IF_FALSE(checkNotEmpty(param1, uint8_t{ 5 }, uint8_t{ 8 }, param4, param5, State::Paused));
	}

	{
		uint16_t param1{ 10 };
		uint16_t param4{ 3981 };
		uint16_t param5{ 22938 };
		RETURN_IF_FALSE(checkNotEmpty(param1, uint16_t{ 5 }, uint16_t{ 8 }, param4, param5, State::Undefined));
	}

	{
		uint32_t param1{ 10 };
		uint32_t param4{ 3981 };
		uint32_t param5{ 55938 };
		RETURN_IF_FALSE(checkNotEmpty(param1, uint32_t{ 5 }, uint32_t{ 8 }, param4, param5, State::Paused));
	}

	{
		uint64_t param1{ 10 };
		uint64_t param4{ 3981 };
		uint64_t param5{ 55938 };
		RETURN_IF_FALSE(checkNotEmpty(param1, uint64_t{ 5 }, uint64_t{ 8 }, param4, param5, State::Running));
	}

	{
		double param1{ 0.00067112 };
		double param4{ -93981.42804648 };
		double param5{ 105938.84936204 };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, double{ 5983.647394875 }, double{ -0.7400006701 }, param4, param5, State::Running));
	}

	{
		float param1{ 0.00067112f };
		float param4{ -93981.42804648f };
		float param5{ 105938.84936204f };
		RETURN_IF_FALSE(
			checkNotEmpty(param1, float{ 5983.647394875f }, float{ -0.7400006701f }, param4, param5, State::Paused));
	}

	{
		Application app;
		app.SetName("TestApp");
		app.SetState(State::Running);

		bool param1{ false };
		app.RegisterParameter(1, { "Some name", &param1 });
		app.RegisterParameter(1, { "Some name", &param1 });
		std::string expectedParameters{ "Parameters:\n{\n\tSome name(1) : false" + ASE(app.GetState()) };
		RETURN_IF_FALSE(check(0, app, expectedParameters, 1, {}));

		bool param2{ true };
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(check(1, app, "Parameters:\n{\n\tSome name(1) : true" + ASE(app.GetState()), 1, {}));

		param1 = false;
		RETURN_IF_FALSE(check(2, app, expectedParameters, 1, {}));

		app.RegisterConstParameter(2, { "Some name", &param1 });
		app.RegisterConstParameter(2, { "Some name", &param1 });
		expectedParameters
			= "Parameters:\n{\n\tSome name(1) : false\n\tSome name(2) const : false" + ASE(app.GetState());
		RETURN_IF_FALSE(check(3, app, expectedParameters, 2, {}));

		const bool param3{ true };
		app.MergeParameters({ { 2, param3 } });
		RETURN_IF_FALSE(check(4, app, expectedParameters, 2, {}));

		app.RegisterConstParameter(3, { "Some name", &param3 });
		app.RegisterConstParameter(3, { "Some name", &param3 });
		expectedParameters
			= "Parameters:\n{\n\tSome name(1) : false\n\tSome name(2) const : false\n\tSome name(3) const : true"
			+ ASE(app.GetState());
		RETURN_IF_FALSE(check(5, app, expectedParameters, 3, {}));

		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(6, app, expectedParameters, 3, {}));

		int param4{ 0 };
		app.MergeParameters({ { 1, param4 } });
		app.MergeParameters({ { 2, param4 } });
		app.MergeParameters({ { 3, param4 } });
		RETURN_IF_FALSE(check(7, app, expectedParameters, 3, {}));

		app.MergeParameters({ { 1, param4 }, { 2, param4 }, { 3, param4 } });
		RETURN_IF_FALSE(check(8, app, expectedParameters, 3, {}));
	}

	/**************************
	 * @param v1 First registered value. Should not be a temporal l-value.
	 * @param v2 Update for v1 by merge.
	 * @param v3 Update for v1 by direct assignment.
	 * @param v4 First registered value with min and max, should be equal to min. It should be safely to reduce it by 1.
	 * Should not be a temporal l-value.
	 * @param v5 First registered value with min and max, should be equal to max. It should be safely to increase it
	 * by 1. Should not be a temporal l-value.
	 */
	const auto checkNumericOptionalParameter{ [&check, &checkNotEmpty]<typename T> [[nodiscard]] (T & v1, const T& v2,
												  const T& v3, T& v4, T& v5, const State state,
												  std::string_view (*const f)(remove_optional_t<T>) = nullptr) {
		RETURN_IF_FALSE(checkNotEmpty(v1, v2, v3, v4, v5, state, f));

		Application app;
		app.SetName("TestApp");
		app.SetState(state);

		const std::optional<remove_optional_t<T>> min{ v4 };
		const std::optional<remove_optional_t<T>> max{ v5 };

		std::string expectedParameters;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(1, { "Some name", &v1, Timer::Duration::Type::Days });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + v1.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			app.RegisterParameter(1, { "Some name", &v1 });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v1) + ASE(state);
		}
		RETURN_IF_FALSE(check(0, app, expectedParameters, 1, {}));

		T tmp = {};
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 1, tmp } });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + tmp.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			app.MergeParameters({ { 1, tmp } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(tmp) + ASE(state);
		}
		RETURN_IF_FALSE(check(1, app, expectedParameters, 1, { { 1, "Parameter Some name(1) is empty" } }));

		app.MergeParameters({ { 1, tmp } });
		RETURN_IF_FALSE(check(2, app, expectedParameters, 1, { { 1, "Parameter Some name(1) is empty" } }));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.MergeParameters({ { 1, v4 } });
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			app.MergeParameters({ { 1, v4 } });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v4) + ASE(state);
		}
		RETURN_IF_FALSE(check(3, app, expectedParameters, 1, {}));

		if constexpr (std::is_same_v<T, Timer::Duration>) {
			app.RegisterParameter(2, { "Some name", &tmp, Timer::Duration::Type::Days, min, max, true });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			app.RegisterParameter(2, { "Some name", &tmp, min, max, true });
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + _S(v4) + "\n\tSome name(2) : " + ASE(state);
		}
		RETURN_IF_FALSE(check(4, app, expectedParameters, 2, {}));

		T tmp2;
		std::map<size_t, std::string> expectedErrors;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			tmp2 = min.value() - Timer::Duration::CreateDays(1);
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp2.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
				"Parameter Some name(2) is less than min value: " + tmp2.ToString(Timer::Duration::Type::Days) + " < "
					+ min.value().ToString(Timer::Duration::Type::Days) } };
		}
		else {
			tmp2 = min.value() - 1;
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v4) + "\n\tSome name(2) : " + _S(tmp2) + ASE(state);
			expectedErrors = { { 2, "Parameter Some name(2) is less than min value: " + _S(tmp2) + " < " + _S(min) } };
		}

		app.MergeParameters({ { 2, tmp2 } });
		RETURN_IF_FALSE(check(5, app, expectedParameters, 2, expectedErrors));

		T tmp3;
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			tmp3 = max.value() + Timer::Duration::CreateDays(1);
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + tmp3.ToString(Timer::Duration::Type::Days) + ASE(state);
			expectedErrors = { { 2,
				"Parameter Some name(2) is greater than max value: " + tmp3.ToString(Timer::Duration::Type::Days)
					+ " > " + max.value().ToString(Timer::Duration::Type::Days) } };
		}
		else {
			tmp3 = max.value() + 1;
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v4) + "\n\tSome name(2) : " + _S(tmp3) + ASE(state);
			expectedErrors
				= { { 2, "Parameter Some name(2) is greater than max value: " + _S(tmp3) + " > " + _S(max) } };
		}

		app.MergeParameters({ { 2, tmp3 } });
		RETURN_IF_FALSE(check(6, app, expectedParameters, 2, expectedErrors));

		app.MergeParameters({ { 2, v5 } });
		if constexpr (std::is_same_v<T, Timer::Duration>) {
			expectedParameters = "Parameters:\n{\n\tSome name(1) : " + v4.ToString(Timer::Duration::Type::Days)
				+ "\n\tSome name(2) : " + v5.ToString(Timer::Duration::Type::Days) + ASE(state);
		}
		else {
			expectedParameters
				= "Parameters:\n{\n\tSome name(1) : " + _S(v4) + "\n\tSome name(2) : " + _S(v5) + ASE(state);
		}
		RETURN_IF_FALSE(check(7, app, expectedParameters, 2, {}));

		return true;
	} };

#undef POEF
#undef PE

	{
		struct Test {
			enum class TestEnum : int16_t {
				Undefined,
				Zero,
				One,
				Two,
				Three,
				Four,
				Five,
				Six,
				Seven,
				Eight,
				Nine,
				Max
			};

			[[nodiscard]] static std::string_view EnumToString(const TestEnum value)
			{
				switch (value) {
				case TestEnum::Undefined:
					return "Undefined";
				case TestEnum::Zero:
					return "Zero";
				case TestEnum::One:
					return "One";
				case TestEnum::Two:
					return "Two";
				case TestEnum::Three:
					return "Three";
				case TestEnum::Four:
					return "Four";
				case TestEnum::Five:
					return "Five";
				case TestEnum::Six:
					return "Six";
				case TestEnum::Seven:
					return "Seven";
				case TestEnum::Eight:
					return "Eight";
				case TestEnum::Nine:
					return "Nine";
				case TestEnum::Max:
					return "Max";
				default:
					return "Unknown";
				}
			}
		};

		{
			Test::TestEnum param1{ Test::TestEnum::Four };
			//* Max is always auto generated, but parameter should be passed. Min can be calculated as Undefined or
			//* Undefined + 1
			Test::TestEnum param4{ Test::TestEnum::Undefined };
			Test::TestEnum param5{ Test::TestEnum::Max };
			RETURN_IF_FALSE(checkNotEmpty(param1, Test::TestEnum::Eight, Test::TestEnum::Two, param4, param5,
				State::Undefined, Test::EnumToString));

			RETURN_IF_FALSE(
				checkNotEmpty(param1, Test::TestEnum::Eight, Test::TestEnum::Two, param4, param5, State::Running));
		}
	}

	{
		std::optional<int8_t> param1{ -3 };
		std::optional<int8_t> param4{ -23 };
		std::optional<int8_t> param5{ 23 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(
			param1, std::optional<int8_t>{ 11 }, std::optional<int8_t>{ 0 }, param4, param5, State::Undefined));
	}

	{
		std::optional<int16_t> param1{ -3331 };
		std::optional<int16_t> param4{ -21981 };
		std::optional<int16_t> param5{ 31938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(
			param1, std::optional<int16_t>{ 5983 }, std::optional<int16_t>{ 0 }, param4, param5, State::Paused));
	}

	{
		std::optional<int32_t> param1{ -3331 };
		std::optional<int32_t> param4{ -93981 };
		std::optional<int32_t> param5{ 105938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(
			param1, std::optional<int32_t>{ 5983 }, std::optional<int32_t>{ 0 }, param4, param5, State::Running));
	}

	{
		std::optional<int64_t> param1{ -3331 };
		std::optional<int64_t> param4{ -93981 };
		std::optional<int64_t> param5{ 105938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(
			param1, std::optional<int64_t>{ 5983 }, std::optional<int64_t>{ 0 }, param4, param5, State::Running));
	}

	{
		std::optional<uint8_t> param1{ 10 };
		std::optional<uint8_t> param4{ 20 };
		std::optional<uint8_t> param5{ 30 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(
			param1, std::optional<uint8_t>{ 5 }, std::optional<uint8_t>{ 8 }, param4, param5, State::Paused));
	}

	{
		std::optional<uint16_t> param1{ 10 };
		std::optional<uint16_t> param4{ 3981 };
		std::optional<uint16_t> param5{ 22938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(
			param1, std::optional<uint16_t>{ 5 }, std::optional<uint16_t>{ 8 }, param4, param5, State::Undefined));
	}

	{
		std::optional<uint32_t> param1{ 10 };
		std::optional<uint32_t> param4{ 3981 };
		std::optional<uint32_t> param5{ 55938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(
			param1, std::optional<uint32_t>{ 5 }, std::optional<uint32_t>{ 8 }, param4, param5, State::Paused));
	}

	{
		std::optional<uint64_t> param1{ 10 };
		std::optional<uint64_t> param4{ 3981 };
		std::optional<uint64_t> param5{ 55938 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(
			param1, std::optional<uint64_t>{ 5 }, std::optional<uint64_t>{ 8 }, param4, param5, State::Running));
	}

	{
		std::optional<double> param1{ 0.00067112 };
		std::optional<double> param4{ -93981.42804648 };
		std::optional<double> param5{ 105938.84936204 };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<double>{ 5983.647394875 },
			std::optional<double>{ -0.7400006701 }, param4, param5, State::Running));
	}

	{
		std::optional<float> param1{ 0.00067112f };
		std::optional<float> param4{ -93981.42804648f };
		std::optional<float> param5{ 105938.84936204f };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, std::optional<float>{ 5983.647394875f },
			std::optional<float>{ -0.7400006701f }, param4, param5, State::Paused));
	}

	{
		Timer::Duration param1{ Timer::Duration::Create(11, 5, 30, 30, 8946) };
		Timer::Duration param4{ Timer::Duration::Create(10, 5, 30, 30, 8946) };
		Timer::Duration param5{ Timer::Duration::Create(12, 5, 30, 30, 8946) };
		RETURN_IF_FALSE(checkNumericOptionalParameter(param1, Timer::Duration::Create(11, 6, 45, 0, 8946),
			Timer::Duration::Create(12, 5, 30, 30, 8947), param4, param5, State::Paused));
	}

	{
		Application app;
		app.SetName("TestApp");
		app.SetState(State::Running);

		std::string param1{ "Some string parameter" };
		app.RegisterParameter(1, { "Some name", &param1 });
		app.RegisterParameter(1, { "Some name", &param1 });
		RETURN_IF_FALSE(
			check(0, app, "Parameters:\n{\n\tSome name(1) : Some string parameter" + ASE(app.GetState()), 1, {}));

		std::string param2{ "Another string parameter" };
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(
			check(1, app, "Parameters:\n{\n\tSome name(1) : Another string parameter" + ASE(app.GetState()), 1, {}));

		param1 = "Third version of parameter";
		RETURN_IF_FALSE(
			check(2, app, "Parameters:\n{\n\tSome name(1) : Third version of parameter" + ASE(app.GetState()), 1, {}));

		param2 = "";
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(check(3, app, "Parameters:\n{\n\tSome name(1) : " + ASE(app.GetState()), 1,
			{ { 1, "Parameter Some name(1) is empty" } }));

		std::string param3;
		app.RegisterParameter(2, { "Some name", &param3 });
		RETURN_IF_FALSE(check(4, app, "Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : " + ASE(app.GetState()), 2,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		std::string param4;
		app.RegisterParameter(3, { "Some name", &param4, true });
		app.RegisterParameter(3, { "Some name", &param4, true });
		RETURN_IF_FALSE(check(5, app,
			"Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : " + ASE(app.GetState()), 3,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		param2 = "Another string parameter";
		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(6, app,
			"Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : Another string "
			"parameter"
				+ ASE(app.GetState()),
			3, { { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		param2 = "";
		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(7, app,
			"Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : " + ASE(app.GetState()), 3,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		app.RegisterConstParameter(4, { "Some name", &param2 });
		app.RegisterConstParameter(4, { "Some name", &param2 });
		std::string expectedParameters{
			"Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : \n\tSome name(4) const : "
			+ ASE(app.GetState())
		};
		RETURN_IF_FALSE(check(8, app, expectedParameters, 4,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		std::string tmp{ "Some string parameter" };
		app.MergeParameters({ { 4, tmp } });
		RETURN_IF_FALSE(check(9, app, expectedParameters, 4,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		const std::string param5{ "123" };
		app.RegisterConstParameter(5, { "Some name", &param5 });
		app.RegisterConstParameter(5, { "Some name", &param5 });
		expectedParameters = "Parameters:\n{\n\tSome name(1) : \n\tSome name(2) : \n\tSome name(3) : \n\tSome "
							 "name(4) const : \n\tSome name(5) const : 123"
			+ ASE(app.GetState());
		RETURN_IF_FALSE(check(10, app, expectedParameters, 5,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		app.MergeParameters({ { 5, tmp } });
		RETURN_IF_FALSE(check(11, app, expectedParameters, 5,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		bool boolean{ false };
		app.MergeParameters({ { 1, boolean } });
		app.MergeParameters({ { 2, boolean } });
		app.MergeParameters({ { 3, boolean } });
		app.MergeParameters({ { 4, boolean } });
		app.MergeParameters({ { 5, boolean } });
		RETURN_IF_FALSE(check(12, app, expectedParameters, 5,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		app.MergeParameters({ { 1, boolean }, { 2, boolean }, { 3, boolean }, { 4, boolean }, { 5, boolean } });
		RETURN_IF_FALSE(check(13, app, expectedParameters, 5,
			{ { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));
	}

	{
		Application app;
		app.SetName("TestApp");
		app.SetState(State::Running);

		Timer param1{ Timer::Create(2023, 11, 5, 23, 30, 1, 8946) };
		app.RegisterParameter(1, { "Some name", &param1 });
		app.RegisterParameter(1, { "Some name", &param1 });
		RETURN_IF_FALSE(
			check(0, app, "Parameters:\n{\n\tSome name(1) : " + param1.ToString() + ASE(app.GetState()), 1, {}));

		Timer param2{ Timer::Create(1973, 11, 5, 0, 30, 1, 8946) };
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(
			check(1, app, "Parameters:\n{\n\tSome name(1) : " + param2.ToString() + ASE(app.GetState()), 1, {}));

		param1 = Timer::Create(2025, 11, 5, 23, 30, 1, 8946);
		RETURN_IF_FALSE(
			check(2, app, "Parameters:\n{\n\tSome name(1) : " + param1.ToString() + ASE(app.GetState()), 1, {}));

		param2 = Timer{ 0 };
		app.MergeParameters({ { 1, param2 } });
		RETURN_IF_FALSE(check(3, app, "Parameters:\n{\n\tSome name(1) : " + param1.ToString() + ASE(app.GetState()), 1,
			{ { 1, "Parameter Some name(1) is empty" } }));

		Timer param3{ 0 };
		app.RegisterParameter(2, { "Some name", &param3 });
		app.RegisterParameter(2, { "Some name", &param3 });
		std::map<size_t, std::string> expectedErrors{ { 1, "Parameter Some name(1) is empty" },
			{ 2, "Parameter Some name(2) is empty" } };
		RETURN_IF_FALSE(check(4, app,
			"Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : " + param3.ToString()
				+ ASE(app.GetState()),
			2, expectedErrors));

		Timer param4{ 0 };
		app.RegisterParameter(3, { "Some name", &param4, true });
		RETURN_IF_FALSE(check(5, app,
			"Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : " + param3.ToString()
				+ "\n\tSome name(3) : " + param4.ToString() + ASE(app.GetState()),
			3, expectedErrors));

		param2 = Timer::Create(2025, 11, 5, 23, 30, 1, 8964);
		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(6, app,
			"Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : " + param3.ToString()
				+ "\n\tSome name(3) : " + param2.ToString() + ASE(app.GetState()),
			3, { { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		param2 = Timer{ 0 };
		app.MergeParameters({ { 3, param2 } });
		RETURN_IF_FALSE(check(7, app,
			"Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : " + param3.ToString()
				+ "\n\tSome name(3) : " + param2.ToString() + ASE(app.GetState()),
			3, { { 1, "Parameter Some name(1) is empty" }, { 2, "Parameter Some name(2) is empty" } }));

		app.RegisterConstParameter(4, { "Some name", &param2 });
		app.RegisterConstParameter(4, { "Some name", &param2 });
		std::string expectedParameters{ "Parameters:\n{\n\tSome name(1) : " + param1.ToString()
			+ "\n\tSome name(2) : " + param3.ToString() + "\n\tSome name(3) : " + param2.ToString()
			+ "\n\tSome name(4) const : " + param2.ToString() + ASE(app.GetState()) };
		RETURN_IF_FALSE(check(8, app, expectedParameters, 4, expectedErrors));

		app.MergeParameters({ { 4, param1 } });
		RETURN_IF_FALSE(check(9, app, expectedParameters, 4, expectedErrors));

		const Timer param5{ Timer::Create(2023, 11, 5, 23, 30, 1, 8946) };
		app.RegisterConstParameter(5, { "Some name", &param5 });
		app.RegisterConstParameter(5, { "Some name", &param5 });
		expectedParameters = "Parameters:\n{\n\tSome name(1) : " + param1.ToString() + "\n\tSome name(2) : "
			+ param3.ToString() + "\n\tSome name(3) : " + param2.ToString() + "\n\tSome name(4) const : "
			+ param2.ToString() + "\n\tSome name(5) const : " + param5.ToString() + ASE(app.GetState());
		RETURN_IF_FALSE(check(10, app, expectedParameters, 5, expectedErrors));

		app.MergeParameters({ { 5, param1 } });
		RETURN_IF_FALSE(check(11, app, expectedParameters, 5, expectedErrors));

		bool boolean{ false };
		app.MergeParameters({ { 1, boolean } });
		app.MergeParameters({ { 2, boolean } });
		app.MergeParameters({ { 3, boolean } });
		app.MergeParameters({ { 4, boolean } });
		app.MergeParameters({ { 5, boolean } });
		RETURN_IF_FALSE(check(12, app, expectedParameters, 5, expectedErrors));

		app.MergeParameters({ { 1, boolean }, { 2, boolean }, { 3, boolean }, { 4, boolean }, { 5, boolean } });
		RETURN_IF_FALSE(check(13, app, expectedParameters, 5, expectedErrors));
	}

	{
		Application app;
		app.SetName("TestApp");
		app.SetState(State::Running);

		Table<std::optional<uint64_t>, Timer, std::string, Timer::Duration, double> table1{ 111111, 222222, 333333,
			444444, 555555 };

		app.RegisterParameter(1, { "Some name", &table1 });
		app.RegisterParameter(1, { "Some name", &table1 });
		RETURN_IF_FALSE(check(0, app, "Parameters:\n{\n\tSome name(1) : " + table1.ToString() + ASE(app.GetState()), 1,
			{ { 1, "Parameter Some name(1) is empty" } }));

		auto table2{ table1 };
		table2.AddRow(std::optional<uint64_t>{}, Timer::Create(2053, 1, 5, 23, 30, 1, 8946),
			std::string{ "Some string 1" }, Timer::Duration::CreateMinutes(123), 0.0);
		table2.AddRow(std::optional<uint64_t>{ 1 }, Timer::Create(2048, 11, 5, 23, 8, 1, 89461),
			std::string{ "Some string 2 2" }, Timer::Duration{}, 928347.74);
		table2.AddRow(std::optional<uint64_t>{}, Timer::Create(2099, 11, 5, 7, 30, 1, 894623),
			std::string{ "Some string 3 3 3" }, Timer::Duration::CreateSeconds(90), -0.00067112);
		table2.AddRow(std::optional<uint64_t>{ 3 }, Timer::Create(2085, 4, 5, 23, 30, 1, 8946456), std::string{},
			Timer::Duration::Create(11, 5, 30, 30, 8946), 0.00067112);
		table2.AddRow(std::optional<uint64_t>{}, Timer{ 0 }, std::string{ "p" },
			Timer::Duration::CreateMilliseconds(100), -30.00067112004);

		app.MergeParameters({ { 1, table2 } });
		RETURN_IF_FALSE(
			check(1, app, "Parameters:\n{\n\tSome name(1) : " + table2.ToString() + ASE(app.GetState()), 1, {}));

		table1.AddRow(std::optional<uint64_t>{}, Timer{ 0 }, std::string{ "p" },
			Timer::Duration::CreateMilliseconds(100), -30.00067112004);
		RETURN_IF_FALSE(
			check(2, app, "Parameters:\n{\n\tSome name(1) : " + table1.ToString() + ASE(app.GetState()), 1, {}));

		table2.Clear();
		app.MergeParameters({ { 1, table2 } });
		std::map<size_t, std::string> expectedErrors{ { 1, "Parameter Some name(1) is empty" } };
		RETURN_IF_FALSE(check(
			3, app, "Parameters:\n{\n\tSome name(1) : " + table2.ToString() + ASE(app.GetState()), 1, expectedErrors));

		Table<bool, bool, std::string, bool, std::string, std::string, std::string> table3{ 123, 456, 789, 101112,
			131415, 161718, 192021 };
		table3.AddRow(true, false, std::string{ "Some string 1" }, true, std::string{ "Some string 2" },
			std::string{ "Some string 3" }, std::string{ "Some string 4" });
		app.RegisterParameter(2, { "Some name", &table3 });
		RETURN_IF_FALSE(check(4, app,
			"Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : " + table3.ToString()
				+ ASE(app.GetState()),
			2, expectedErrors));

		Table<std::optional<float>, std::optional<float>, bool, bool> table4{ 11, 22, 33, 44 };
		app.RegisterParameter(3, { "Some name", &table4, true });
		app.RegisterParameter(3, { "Some name", &table4, true });
		RETURN_IF_FALSE(check(5, app,
			"Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : " + table3.ToString()
				+ "\n\tSome name(3) : " + table4.ToString() + ASE(app.GetState()),
			3, expectedErrors));

		auto table5{ table4 };
		table5.AddRow(std::optional<float>{ 1 }, std::optional<float>{ 2 }, true, false);
		table5.AddRow(std::optional<float>{ 3 }, std::optional<float>{ 4 }, false, true);
		app.MergeParameters({ { 3, table5 } });
		RETURN_IF_FALSE(check(6, app,
			"Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : " + table3.ToString()
				+ "\n\tSome name(3) : " + table5.ToString() + ASE(app.GetState()),
			3, expectedErrors));

		table5.Clear();
		app.MergeParameters({ { 3, table5 } });
		RETURN_IF_FALSE(check(7, app,
			"Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : " + table3.ToString()
				+ "\n\tSome name(3) : " + table5.ToString() + ASE(app.GetState()),
			3, expectedErrors));

		Table<std::string> table6{ 521 };
		app.RegisterConstParameter(4, { "Some name", &table6 });
		app.RegisterConstParameter(4, { "Some name", &table6 });
		std::string expectedParameters{ "Parameters:\n{\n\tSome name(1) : " + table1.ToString()
			+ "\n\tSome name(2) : " + table3.ToString() + "\n\tSome name(3) : " + table4.ToString()
			+ "\n\tSome name(4) const : " + table6.ToString() + ASE(app.GetState()) };
		RETURN_IF_FALSE(check(8, app, expectedParameters, 4, expectedErrors));

		auto table7{ table6 };
		table7.AddRow("Some string 1");
		table7.AddRow("Some string 2");
		table7.AddRow("Some string 3");
		app.MergeParameters({ { 4, table7 } });
		RETURN_IF_FALSE(check(9, app, expectedParameters, 4, expectedErrors));

		auto table8{ table6 };
		table8.AddRow("Some string 1111");
		table8.AddRow("Some string 2222");
		table8.AddRow("Some string 3333");
		app.RegisterConstParameter(5, { "Some name", &table8 });
		app.RegisterConstParameter(5, { "Some name", &table8 });
		expectedParameters = "Parameters:\n{\n\tSome name(1) : " + table1.ToString() + "\n\tSome name(2) : "
			+ table3.ToString() + "\n\tSome name(3) : " + table4.ToString() + "\n\tSome name(4) const : "
			+ table6.ToString() + "\n\tSome name(5) const : " + table8.ToString() + ASE(app.GetState());
		RETURN_IF_FALSE(check(10, app, expectedParameters, 5, expectedErrors));

		app.MergeParameters({ { 5, table7 } });
		RETURN_IF_FALSE(check(11, app, expectedParameters, 5, expectedErrors));

		bool boolean{ false };
		app.MergeParameters({ { 1, boolean } });
		app.MergeParameters({ { 2, boolean } });
		app.MergeParameters({ { 3, boolean } });
		app.MergeParameters({ { 4, boolean } });
		app.MergeParameters({ { 5, boolean } });
		RETURN_IF_FALSE(check(12, app, expectedParameters, 5, expectedErrors));

		app.MergeParameters({ { 1, boolean }, { 2, boolean }, { 3, boolean }, { 4, boolean }, { 5, boolean } });
		RETURN_IF_FALSE(check(13, app, expectedParameters, 5, expectedErrors));
	}

#undef ASE

	return true;
}

}; //* namespace MSAPI