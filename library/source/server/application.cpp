/**************************
 * @file        application.cpp
 * @version     6.0
 * @date        2023-12-11
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

#include "application.h"
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

#define TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART                                                              \
	if (!parameter.m_stringInterpretations.empty()) {                                                                  \
		std::format_to(                                                                                                \
			std::back_inserter(m_metadata), ",\"stringInterpretations\":{}", parameter.m_stringInterpretations);       \
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
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, int16_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int16\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, int32_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int32\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, int64_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int64\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, uint8_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint8\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, uint16_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint16\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, uint32_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint32\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, uint64_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint64\"");
						TMP_MSAPI_APPLICATION_MIN_MAX_PART;
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
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
	std::format_to(std::back_inserter(m_metadata), ",\"columns\":{{");                                                 \
	if (columns != nullptr && !columns->empty()) {                                                                     \
		auto begin{ columns->begin() }, end{ columns->end() };                                                         \
		std::format_to(std::back_inserter(m_metadata), "\"{}\":{{\"type\":\"{}\"", begin->id,                          \
			StandardType::EnumToString(begin->type));                                                                  \
		if (!begin->metadata.empty()) {                                                                                \
			std::format_to(std::back_inserter(m_metadata), ",{}}}", begin->metadata);                                  \
		}                                                                                                              \
		else {                                                                                                         \
			std::format_to(std::back_inserter(m_metadata), "}}");                                                      \
		}                                                                                                              \
                                                                                                                       \
		while (++begin != end) {                                                                                       \
			std::format_to(std::back_inserter(m_metadata), ",\"{}\":{{\"type\":\"{}\"", begin->id,                     \
				StandardType::EnumToString(begin->type));                                                              \
			if (!begin->metadata.empty()) {                                                                            \
				std::format_to(std::back_inserter(m_metadata), ",{}}}", begin->metadata);                              \
			}                                                                                                          \
			else {                                                                                                     \
				std::format_to(std::back_inserter(m_metadata), "}}");                                                  \
			}                                                                                                          \
		}                                                                                                              \
	}                                                                                                                  \
	std::format_to(std::back_inserter(m_metadata), "}}");

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
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, int16_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int16\"");
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, int32_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int32\"");
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, int64_t>) {
						std::format_to(std::back_inserter(m_metadata), "Int64\"");
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, uint8_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint8\"");
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, uint16_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint16\"");
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, uint32_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint32\"");
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
					}
					else if constexpr (std::is_same_v<T, uint64_t>) {
						std::format_to(std::back_inserter(m_metadata), "Uint64\"");
						TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART;
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
#undef TMP_MSAPI_APPLICATION_STRING_INTERPRETATIONS_PART
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

} //* namespace MSAPI