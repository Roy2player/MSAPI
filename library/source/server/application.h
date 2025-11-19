/**************************
 * @file        application.h
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

#ifndef MSAPI_APPLICATION_H
#define MSAPI_APPLICATION_H

#include "../help/helper.h"
#include "../protocol/standard.h"
#include <string>
#include <variant>

namespace MSAPI {

/**************************
 * @brief Basic class with separate state for creating an application, contains core logic to handle specific callbacks
 * based on standard protocol, allows set parameters that can be any standard type. Specific fields are not necessary to
 * be filled, specific methods can be overridden for enabling custom logic.
 *
 * Predefined and can be handled only from manager:
 * @brief HandleRunRequest - default behavior: apply Running state if parameters are valid and application is not in
 * running state. Macros MSAPI_HANDLE_RUN_REQUEST_PRESET can be placed in the beginning of overridden method for
 * enabling with predefined logic.
 * @brief HandlePauseRequest - default behavior: apply Paused state if application is in a running state. Macros
 * MSAPI_HANDLE_PAUSE_REQUEST_PRESET can be placed in the beginning of overridden method for enabling with predefined
 * logic.
 * @brief HandleModifyRequest - default behavior: merge parameters and apply pause state if parameters are not valid.
 * Macros MSAPI_HANDLE_MODIFY_REQUEST_PRESET can be placed in the beginning of overridden method for enabling with
 * predefined logic.
 * @brief HandleDeleteRequest - default behavior: end main server process. If signal comes from manager before this
 * callback, the HandlePauseRequest will be called firstly.
 *
 * Not predefined and can be handled from any outcome connection:
 * @brief HandleHello - handler of signal which sends for every newly open outcome connection if or when server becomes
 * running.
 * @brief HandleMetadata - handler of response for metadata request. Contains string JSON which describes all
 * application parameters in next fields: name, id, type, limits.
 * - Metadata fields: mutable (object) : { id (string) : metadata (object), ... }, const (object): { id (string) :
 * metadata (object), ... };
 * - Metadata object fields: name (string), type (string: Int8, Int16, Int32, Int64, Uint8, Uint16, Uint32, Uint64,
 * Double, Float, Bool, OptionalInt8, OptionalInt16, OptionalInt32, OptionalInt64, OptionalUint8, OptionalUint16,
 * OptionalUint32, OptionalUint64, OptionalDouble, OptionalFloat, String, Timer, Duration, TableData). Optionally: min,
 * max, canBeEmpty, durationType, columns, stringInterpretation;
 * - Metadata object column fields: take a look at TableBase::Column documentation.
 * @brief HandleParameters - handler of response for parameters request. Contains all application parameters.
 *
 * Predefined not network callbacks:
 * @brief HandleDisconnect - signal about disconnection of outcome connection. Will not be called if connection was
 * closed by server. Default behavior: call HandlePauseRequest.
 * @brief HandleReconnect - signal about reconnection of outcome connection. Will not be called if connection was closed
 * by server. Default behavior: call HandleRunRequest.
 *
 * Network signals which can be handled only from manager:
 * @brief handleMetadataRequest - request for metadata.
 * @brief handleParametersRequest - request for parameters.
 *
 * @brief Manager is an any outcome connection with 0 id.
 *
 * @brief Parameters which should be allowed to be changed or requested via metadata and parameters requests must be
 * registered in constructor. Parameters are just pointers and do not affect performance. Parameters can be limited by
 * min and max values, by empty state and can have custom error with message if default limitations are not enough. It
 * is allowed by special parameter method and ability to merge parameters one by one. Custom error will be added to
 * usual error if available and will be cleared after any merge action. For enums a custom print function can be defined
 * and then metadata string interpretations for MSAPI Manager fronted is generated. Metadata is only used in Application
 * class and when metadata request arrives. Const parameters are used for displaying in general parameters list only,
 * they can't be changed. Server and Application have their own default parameters.
 *
 * @brief Const parameter 2000001 "Name" ia a name of application.
 * @brief Const parameter 2000002 "Application state" is a state of application.
 *
 * @attention Because of on app side all parameters are still vanilla fields, it is possible to change them directly. In
 * this way they will not be checked throw any limits.
 *
 * @brief Supported types for parameters: pointer to all integer types, all float types and their std::optional types,
 * bool, std::string, MSAPI::Timer, MSAPI::Timer::Duration and Enums (only if its follow MSAPI Enum concept). An enum
 * type is stored as its underlying type.
 * @brief Supported types for const parameters: pointers and const pointers to all parameters' and enum types except
 * optional versions.
 * @brief Supported types for limits: all integer types, all float types, enums, MSAPI::Timer::Duration.
 * @brief Minimum and maximum limits are not supported for bool, std::string, MSAPI::Timer.
 * @brief Empty restriction are supported for MSAPI::Timer, MSAPI::Timer::Duration, MSAPI::TableData and all optional
 * integer, enum and all optional float types.
 *
 * @attention Containing pointer on table is tricky, standard type is TableData but this type is standard for
 * protocol and is realized for transferring purposes. Parameters contains pointer on TableBase.
 *
 * @brief Application state is internal variable which can be used for checking application state.
 * @brief Paused state - application is paused, it will not call any callback from any protocol. Only specific
 * application callbacks will be called.
 * @brief Running state - application is working.
 *
 * @brief UNIX signals handling can be realized by macros MSAPI_APPLICATION_SIGNAL_HANDLER before main function,
 * MSAPI_APPLICATION_SIGNAL_ACTION inside and assign server pointer to app variable which is defined inside
 * MSAPI_APPLICATION_SIGNAL_HANDLER. Signal handler calls HandleDeleteRequest before default action. Defined for: SIGHUP
 * (1), SIGINT (2), SIGQUIT (3), SIGILL (4), SIGABRT (6), SIGFPE (8), SIGUSR1 (10), SIGSEGV (11), SIGUSR2 (12), SIGPIPE
 * (13), SIGALRM (14), SIGTERM (15), SIGVTALRM (26), SIGPROF (27).
 */
class Application {
public:
	enum State : int16_t { Undefined, Paused, Running, Max };

	/**************************
	 * @brief Contains particular parameter and its requirements.
	 */
	class Parameter {
	private:
		const std::string m_name;
		std::variant<standardTypesPtr> m_value;
		const std::optional<std::variant<integerTypes, floatTypes, Timer::Duration>> m_min;
		const std::optional<std::variant<integerTypes, floatTypes, Timer::Duration>> m_max;
		const bool m_canBeEmpty{ false };
		std::string m_error;
		//* For Duration
		const Timer::Duration::Type m_durationType{ Timer::Duration::Type::Nanoseconds };
		//* For enums
		std::string_view (*const m_printFunc)(int){ nullptr };
		std::string m_stringInterpretation;

	public:
		/**************************
		 * @brief Construct a new Parameter object for bool, empty constructor. Without limits and can't be empty.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 *
		 * @test Has unit tests for all types.
		 */
		Parameter(std::string&& name, bool* value);

		/**************************
		 * @brief Construct a new Parameter object, empty constructor.
		 *
		 * @tparam T Type of parameter: int8_t*, int16_t*, int32_t*, int64_t*, uint8_t*, uint16_t*, uint32_t*,
		 * uint64_t*, double*, float*, std::optional<int8_t>*, std::optional<int16_t>*, std::optional<int32_t>*,
		 * std::optional<int64_t>*, std::optional<uint8_t>*, std::optional<uint16_t>*, std::optional<uint32_t>*,
		 * std::optional<uint64_t>*, std::optional<double>*, std::optional<float>*.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 * @param min Minimum value of parameter. Empty by default.
		 * @param max Maximum value of parameter. Empty by default.
		 * @param canBeEmpty True if parameter can be empty. False by default.
		 *
		 * @test Has unit tests for all types.
		 */
		template <typename T>
			requires(is_standard_simple_type_ptr<T> && !std::is_same_v<T, bool*>)
		Parameter(std::string&& name, T value,
			const std::optional<remove_optional_t<std::remove_pointer_t<T>>>& min = {},
			const std::optional<remove_optional_t<std::remove_pointer_t<T>>>& max = {}, const bool canBeEmpty = false)
			: m_name{ std::move(name) }
			, m_value{ value }
			, m_min{ min }
			, m_max{ max }
			, m_canBeEmpty{ canBeEmpty }
		{
		}

#define MSAPI_TMP_APPLICATION_PARAMETER_FILL_METADATA_FOR_ENUM                                                         \
	if (printFunc == nullptr) {                                                                                        \
		return;                                                                                                        \
	}                                                                                                                  \
                                                                                                                       \
	using Base = std::underlying_type_t<std::remove_pointer_t<T>>;                                                     \
	using Enum = std::remove_pointer_t<T>;                                                                             \
	m_stringInterpretation = "{";                                                                                      \
	for (Base index{}; index < static_cast<Base>(Enum::Max); ++index) {                                                \
		m_stringInterpretation += std::format("\"{}\":\"{}\",", index, printFunc(static_cast<Enum>(index)));           \
	}                                                                                                                  \
	m_stringInterpretation[m_stringInterpretation.size() - 1] = '}';

		/**************************
		 * @brief Construct a new Parameter object for enum, initialize a string interpretation metadata if a print
		 * function is provided. Minimum is Enum::Undefined + 1 and maximum is Enum::Max - 1 by default.
		 *
		 * @tparam T Pointer to enum.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 * @param printFunc Custom print function for enum type.
		 * @param canBeUndefined Set minimum as Enum::Undefined, false by default.
		 *
		 * @test Has unit tests.
		 */
		template <typename T>
			requires(std::is_pointer_v<T> && !std::is_const_v<std::remove_pointer_t<T>>
						&& std::is_enum_v<std::remove_pointer_t<T>>
						&& is_any_of<std::underlying_type_t<std::remove_pointer_t<T>>, integerTypes>
						&& Enum<std::remove_pointer_t<T>>)
		Parameter(std::string&& name, T value,
			std::string_view (*const printFunc)(remove_optional_t<std::remove_pointer_t<T>>) = nullptr,
			const bool canBeUndefined = false)
			: m_name{ std::move(name) }
			, m_value{ reinterpret_cast<std::add_pointer_t<std::underlying_type_t<std::remove_pointer_t<T>>>>(value) }
			, m_min{ static_cast<std::underlying_type_t<std::remove_pointer_t<T>>>(canBeUndefined
					  ? static_cast<std::underlying_type_t<std::remove_pointer_t<T>>>(
							std::remove_pointer_t<T>::Undefined)
					  : static_cast<std::underlying_type_t<std::remove_pointer_t<T>>>(
							std::remove_pointer_t<T>::Undefined)
						  + 1) }
			, m_max{ static_cast<std::underlying_type_t<std::remove_pointer_t<T>>>(std::remove_pointer_t<T>::Max) }
			, m_printFunc{ reinterpret_cast<std::string_view (*const)(int)>(reinterpret_cast<const void*>(printFunc)) }
		{
			MSAPI_TMP_APPLICATION_PARAMETER_FILL_METADATA_FOR_ENUM;
		}

		/**************************
		 * @brief Construct a new Parameter object, empty constructor. Value without limits.
		 *
		 * @tparam T std::string*, MSAPI::Timer*.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 * @param canBeEmpty True if parameter can be empty. False by default.
		 *
		 * @test Has unit tests for all types.
		 */
		template <typename T>
			requires std::is_same_v<T, std::string*> || std::is_same_v<T, Timer*>
		Parameter(std::string&& name, T value, const bool canBeEmpty = false)
			: m_name{ std::move(name) }
			, m_value{ value }
			, m_canBeEmpty{ canBeEmpty }
		{
		}

		/**************************
		 * @brief Construct a new Parameter object, check duration type.
		 *
		 * @tparam T MSAPI::Duration*, std::optional<MSAPI::Duration>*.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 * @param durationType Type of duration, should be valid or nanoseconds will be used.
		 * @param min Minimum value of parameter. Empty by default.
		 * @param max Maximum value of parameter. Empty by default.
		 * @param canBeEmpty True if parameter can be empty. False by default.
		 *
		 * @test Has unit tests.
		 */
		template <typename T>
			requires std::is_same_v<T, Timer::Duration*> || std::is_same_v<T, std::optional<Timer::Duration>*>
		Parameter(std::string&& name, T value, const Timer::Duration::Type durationType,
			const std::optional<Timer::Duration>& min = {}, const std::optional<Timer::Duration>& max = {},
			const bool canBeEmpty = false)
			: m_name{ std::move(name) }
			, m_value{ value }
			, m_min{ min }
			, m_max{ max }
			, m_canBeEmpty{ canBeEmpty }
			, m_durationType{ durationType }
		{
			if (UINT64(durationType) >= UINT64(Timer::Duration::Type::Max)) [[unlikely]] {

				LOG_WARNING_NEW("Invalid duration type for parameter {}, type: {}. Nanoseconds is used", name,
					Timer::Duration::EnumToString(durationType));
			}
		}

		/**************************
		 * @brief Construct a new Parameter object for Table, empty constructor.
		 *
		 * @tparam Ts Pointer to any standard type except TableData.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 * @param canBeEmpty True if parameter can be empty. False by default.
		 *
		 * @test Has unit tests for all types.
		 */
		template <typename... Ts>
		Parameter(std::string&& name, Table<Ts...>* value, const bool canBeEmpty = false)
			: m_name{ std::move(name) }
			, m_value{ reinterpret_cast<TableData*>(value) }
			, m_canBeEmpty{ canBeEmpty }
		{
		}

	private:
		/**************************
		 * @brief Validation of registered parameter's value. If value is not correct, error will be added and
		 * printed warning log.
		 *
		 * @param id Identifier of parameter.
		 *
		 * @return True if value is correct, otherwise false.
		 *
		 * @test Has unit tests.
		 */
		bool RegisterValidation(size_t id);

		/**************************
		 * @brief Merge parameter value. If value is not correct, error will be added and printed warning
		 * log.
		 *
		 * @attention Custom error will be cleared if value is correct for internal restrictions.
		 *
		 * @param id Identifier of parameter.
		 * @param value Value of parameter.
		 *
		 * @return True if value is correct, otherwise false.
		 *
		 * @test Has unit tests for all types.
		 */
		bool Merge(size_t id, const std::variant<standardTypes>& value);

		//* For access in RegisterValidation and Merge methods and direct access to fields.
		friend class Application;
	};

	/**************************
	 * @brief Contains constant parameter. Used for displaying in parameters list.
	 */
	class ConstParameter {
	private:
		const std::string m_name;
		std::variant<standardPrimitiveTypesConstPtr, const std::string*, const Timer*, const Timer::Duration*,
			const TableData*>
			m_value;
		//* For Duration
		const Timer::Duration::Type m_durationType{ Timer::Duration::Type::Nanoseconds };
		//* For enums
		std::string_view (*const m_printFunc)(int){ nullptr };
		std::string m_stringInterpretation;

	public:
		/**************************
		 * @brief Construct a new Const Parameter object, empty constructor.
		 *
		 * @tparam T Pointer or const pointer to: int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t,
		 * uint64_t, double, float, bool, std::string, MSAPI::Timer.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 *
		 * @test Has unit tests for all types.
		 * @todo Const string can be std::string_view or const char* too.
		 */
		template <typename T>
			requires is_standard_primitive_type_ptr<T> || is_standard_primitive_type_const_ptr<T>
						 || std::is_same_v<T, std::string*> || std::is_same_v<T, const std::string*>
						 || std::is_same_v<T, MSAPI::Timer*> || std::is_same_v<T, const MSAPI::Timer*>
		ConstParameter(std::string&& name, const T value)
			: m_name{ std::move(name) }
			, m_value{ value }
		{
		}

		/**************************
		 * @brief Construct a new Const Parameter object for Table, empty constructor.
		 *
		 * @tparam Ts Any standard type except TableData.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 *
		 * @test Has unit tests for all types.
		 */
		template <typename... Ts>
		ConstParameter(std::string&& name, const Table<Ts...>* value)
			: m_name{ std::move(name) }
			, m_value{ reinterpret_cast<const TableData*>(value) }
		{
		}

		/**************************
		 * @brief Construct a new Const Parameter object for duration, check duration type inside.
		 *
		 * @tparam T MSAPI::Duration*, const MSAPI::Duration*.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 * @param durationType Type of duration, should be valid or nanoseconds will be used.
		 *
		 * @test Has unit tests.
		 */
		template <typename T>
			requires std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, Timer::Duration>
		ConstParameter(std::string&& name, const T value, const Timer::Duration::Type durationType)
			: m_name{ std::move(name) }
			, m_value{ value }
			, m_durationType{ durationType }
		{
			if (UINT64(durationType) >= UINT64(Timer::Duration::Type::Max)) [[unlikely]] {

				LOG_WARNING_NEW("Invalid duration type for parameter {}, type: {}. Nanoseconds is used", name,
					Timer::Duration::EnumToString(durationType));
			}
		}

		/**************************
		 * @brief Construct a new Const Parameter object for enum, initialize string interpretation metadata if a print
		 * function is provided.
		 *
		 * @tparam T pointer to enum.
		 *
		 * @param name Name of parameter.
		 * @param value Value of parameter.
		 * @param printFunc Custom print function for enum type.
		 *
		 * @test Has unit tests.
		 */
		template <typename T>
			requires std::is_pointer_v<T> && std::is_enum_v<std::remove_pointer_t<T>>
						 && is_any_of<std::underlying_type_t<std::remove_cv_t<std::remove_pointer_t<T>>>, integerTypes>
		ConstParameter(std::string&& name, const T value,
			std::string_view (*const printFunc)(remove_optional_t<std::remove_pointer_t<T>>) = nullptr)
			: m_name{ std::move(name) }
			, m_value{ reinterpret_cast<
				  std::add_pointer_t<std::add_const_t<std::underlying_type_t<std::remove_pointer_t<T>>>>>(value) }
			, m_printFunc{ reinterpret_cast<std::string_view (*const)(int)>(reinterpret_cast<const void*>(printFunc)) }
		{
			MSAPI_TMP_APPLICATION_PARAMETER_FILL_METADATA_FOR_ENUM;
		}

#undef MSAPI_TMP_APPLICATION_PARAMETER_FILL_METADATA_FOR_ENUM

		//* For direct access to fields.
		friend class Application;
	};

private:
	std::string m_name{ "" };
	std::string m_metadata;
	State m_state{ State::Paused };
	std::map<size_t, Parameter> m_parameters;
	std::map<size_t, const Parameter* const> m_errorParameters;
	std::map<size_t, ConstParameter> m_constParameters;

public:
	/**************************
	 * @brief Construct a new Application object, register parameters.
	 */
	Application();

	/**************************
	 * @brief Default destructor, call HandlePauseRequest.
	 */
	virtual ~Application();

	/**************************
	 * @brief Handle run request from External application. Already defined in Server class, but can be overridden.
	 * Default behavior: apply Running state if parameters are valid and application is not in running state.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandleRunRequest();

	/**************************
	 * @brief Handle pause request from External application. Already defined in Server class, but can be
	 * overridden. Default behavior: apply Paused state if application is in a running state.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandlePauseRequest();

	/**************************
	 * @brief Handle modify request from External application. Already defined in Server class, but can be
	 * overridden. Default behavior: merge parameters and call HandlePauseRequest if parameters are not valid.
	 *
	 * @param parametersUpdate Parameters update.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate);

	/**************************
	 * @brief Handle delete request from External application. Already defined in Server class, but can be
	 * overridden. Default behavior: end main server process. If signal comes from manager before this callback, the
	 * HandlePauseRequest will be called firstly.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandleDeleteRequest();

	/**************************
	 * @brief Handle hello message from external application. This message will be sent after opening new outcome
	 * connection if or when server becomes running.
	 *
	 * @param connection Socket connection from which reserved message.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandleHello(int connection);

	/**************************
	 * @brief Handle metadata message from external application. Metadata contains list of all usual and const
	 * parameters.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param metadata Reserved metadata.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandleMetadata(int connection, std::string_view metadata);

	/**************************
	 * @brief Handle parameters message from external application. Response contains list of all usual and const
	 * parameters with values.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param parameters Reserved parameters.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandleParameters(int connection, const std::map<size_t, std::variant<standardTypes>>& parameters);

	/**************************
	 * @brief Not network signal about previously opened connection by id was closed no by server. Already defined
	 * in Application class, but can be overridden. Default behavior is to call HandlePauseRequest.
	 *
	 * @param id Id of connection.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandleDisconnect(int id);

	/**************************
	 * @brief Not network signal about previously closed connection by id was reopened. Already defined in
	 * Application class, but can be overridden. Default behavior is to call HandleRunRequest.
	 *
	 * @param id Id of connection.
	 *
	 * @test Has unit tests.
	 */
	virtual void HandleReconnect(int id);

	/**************************
	 * @brief Set the Name object to application.
	 *
	 * @param name Name of application.
	 *
	 * @test Has unit tests.
	 */
	void SetName(const std::string& name);

	/**************************
	 * @return Readable reference to parameters.
	 *
	 * @test Has unit tests.
	 */
	const std::map<size_t, Parameter>& GetParameters() const noexcept;

	/**************************
	 * @return Readable reference to constant parameters.
	 *
	 * @test Has unit tests.
	 */
	const std::map<size_t, ConstParameter>& GetConstParameters() const noexcept;

	/**************************
	 * @return True if parameters with error not exist.
	 *
	 * @test Has unit tests.
	 */
	bool AreParametersValid() const noexcept;

	/**************************
	 * @return Readable reference to parameters with errors.
	 *
	 * @test Has unit tests.
	 */
	const std::map<size_t, const Parameter* const>& GetErrorParameters() const noexcept;

	/**************************
	 * @return Name of application, can be empty.
	 *
	 * @test Has unit tests.
	 */
	const std::string& GetName() const noexcept;

	/**************************
	 * @return State of application.
	 *
	 * @test Has unit tests.
	 */
	State GetState() const;

	/**************************
	 * @return True if application is running.
	 *
	 * @test Has unit tests.
	 */
	bool IsRunning() const;

	/**************************
	 * @return String description of state.
	 *
	 * @example Undefined, Pause, Run, Unknown.
	 *
	 * @test Has unit tests.
	 */
	static std::string_view EnumToString(State state);

	/**************************
	 * @return True if all tests passed and false if something went wrong.
	 */
	static bool UNITTEST();

protected:
	/**************************
	 * @brief Set the State to application.
	 *
	 * @param state State of application.
	 *
	 * @test Has unit tests.
	 */
	void SetState(State state);

	/**************************
	 * @brief Collect Standard message with parameters or action from socket connection and call specific Handler
	 * function.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param data Reserved Standard message.
	 *
	 * @test Has unit tests.
	 */
	void Collect(int connection, const StandardProtocol::Data& data);

	/**************************
	 * @brief Register parameter to application if it is not registered yet and value is consistent with
	 * requirements.
	 *
	 * @param id Identifier of parameter.
	 * @param parameter Parameter object.
	 *
	 * @test Has unit tests.
	 */
	void RegisterParameter(size_t id, Parameter&& parameter);

	/**************************
	 * @brief Register constant parameter to application.
	 *
	 * @param id Identifier of parameter.
	 * @param parameter Const parameter object.
	 *
	 * @test Has unit tests.
	 */
	void RegisterConstParameter(size_t id, ConstParameter&& parameter);

	/**************************
	 * @brief Get parameters of application in string format.
	 *
	 * @attention Parameters will be cleared before get action.
	 *
	 * @param parameters Parameters of application.
	 *
	 * @example Parameters:
	 * {
	 *		Seconds between try to connect(1000001) : 1
	 *		Limit of attempts to connection(1000002) : 1000
	 *		Limit of connections from one IP(1000003) : 5
	 *		Recv buffer size(1000004) : 1024
	 *		Recv buffer size limit(1000005) : 10485760
	 *		Server state(1000006) const : Running
	 *		Max connections(1000007) const : 4096
	 *		Listening IP(1000008) const : 127.0.0.1
	 *		Listening port(1000009) const : 60328
	 *		Name(2000001) const : Distributor
	 *		Application state(2000002) const : Paused
	 * }
	 *
	 * @test Has unit tests.
	 */
	void GetParameters(std::string& parameters) const;

	/**************************
	 * @brief Merge parameters to application, manage container of parameters with error. Print warning log if
	 * parameters are not correct. Does not work with const parameters.
	 *
	 * @attention Custom error will be cleared if value is correct for internal restrictions.
	 *
	 * @param parametersUpdate Parameters update.
	 *
	 * @test Has unit tests for all types.
	 */
	void MergeParameters(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate);

	/**************************
	 * @brief Merge parameter to application, manage container of parameters with error. Print warning log if
	 * parameter is not correct. Does not work with const parameters.
	 *
	 * @attention Custom error will be cleared if value is correct for internal restrictions.
	 *
	 * @param id Id of parameter update.
	 * @param value Value of parameter update.
	 *
	 * @test Has unit tests for all types.
	 */
	void MergeParameter(size_t id, const std::variant<standardTypes>& value);

	/**************************
	 * @brief Set custom error to parameter. If error already exist, errors will be concatenated. Custom error will
	 * be overwritten during merge action in any case: if value valid or invalid. Any error can't be cleared
	 * manually, only after merge action with valid value.
	 *
	 * @param id Identifier of parameter.
	 * @param error Error message.
	 *
	 * @test Has unit tests.
	 */
	void SetCustomError(size_t id, const std::string& error);
};

}; //* namespace MSAPI

#define MSAPI_HANDLE_RUN_REQUEST_PRESET                                                                                \
	if (MSAPI::Application::IsRunning()) {                                                                             \
		LOG_DEBUG("Already running, do nothing");                                                                      \
		return;                                                                                                        \
	}                                                                                                                  \
                                                                                                                       \
	if (!MSAPI::Application::AreParametersValid()) {                                                                   \
		LOG_INFO("Parameters are invalid");                                                                            \
		return;                                                                                                        \
	}                                                                                                                  \
                                                                                                                       \
	LOG_INFO("Parameters are valid, set state to Running");                                                            \
	MSAPI::Application::SetState(MSAPI::Application::State::Running);

#define MSAPI_HANDLE_PAUSE_REQUEST_PRESET                                                                              \
	if (!MSAPI::Application::IsRunning()) {                                                                            \
		LOG_DEBUG("Already paused, do nothing");                                                                       \
		return;                                                                                                        \
	}                                                                                                                  \
                                                                                                                       \
	LOG_INFO("Set state to Paused");                                                                                   \
	MSAPI::Application::SetState(MSAPI::Application::State::Paused);

#define MSAPI_HANDLE_MODIFY_REQUEST_PRESET                                                                             \
	MSAPI::Application::MergeParameters(parametersUpdate);                                                             \
	if (MSAPI::Application::AreParametersValid()) {                                                                    \
		return;                                                                                                        \
	}                                                                                                                  \
	HandlePauseRequest();

/**************************
 * @brief Create signal handler for UNIX signals. Should be placed before a main function.
 *
 * @param app (out) Pointer to application [MSAPI::Application*].
 * @param SignalDelete (out) Signal handler for deleting application [void(*)(int)].
 */
#define MSAPI_APPLICATION_SIGNAL_HANDLER                                                                               \
	MSAPI::Application* app;                                                                                           \
	void SignalDelete([[maybe_unused]] const int signal) { app->HandleDeleteRequest(); }

/**************************
 * @brief Set of actions for UNIX signals handling. Should be placed in the beginning of a main function.
 *
 * @param sa (out) Structure with signal handler [sigaction].
 * @param SignalDelete (in) Signal handler for deleting application [void(*)(int)].
 */
#define MSAPI_APPLICATION_SIGNAL_ACTION                                                                                \
	struct sigaction sa;                                                                                               \
	sa.sa_handler = SignalDelete;                                                                                      \
	sa.sa_flags = static_cast<int>(SA_RESETHAND);                                                                      \
	/*                                                                                                                 \
		SIGKILL and SIGSTOP cannot be caught, blocked, or ignored                                                      \
	*/                                                                                                                 \
	/*                                                                                                                 \
		The SIGABRT signal is sent to a process to tell it to abort, i.e. to terminate. The signal is usually          \
		initiated by the process itself when it calls abort function of the C Standard Library, but it can be sent to  \
		the process from outside like any other signal.                                                                \
	*/                                                                                                                 \
	sigaction(SIGABRT, &sa, nullptr);                                                                                  \
	/*                                                                                                                 \
		The SIGALRM, SIGVTALRM and SIGPROF signal is sent to a process when the time limit specified in a call to a    \
		preceding alarm setting function (such as setitimer) elapses. SIGALRM is sent when real or clock time elapses. \
		SIGVTALRM is sent when CPU time used by the process elapses. SIGPROF is sent when CPU time used by the process \
		and by the system on behalf of the process elapses.                                                            \
	*/                                                                                                                 \
	sigaction(SIGALRM, &sa, nullptr);                                                                                  \
	sigaction(SIGVTALRM, &sa, nullptr);                                                                                \
	sigaction(SIGPROF, &sa, nullptr);                                                                                  \
	/*                                                                                                                 \
		The SIGFPE signal is sent to a process when it executes an erroneous arithmetic operation, such as division by \
		zero (the name "FPE", standing for floating-point exception, is a misnomer as the signal covers                \
		integer-arithmetic errors as well).                                                                            \
	*/                                                                                                                 \
	sigaction(SIGFPE, &sa, nullptr);                                                                                   \
	/*                                                                                                                 \
		The SIGHUP signal is sent to a process when its controlling terminal is closed. It was originally designed to  \
		notify the process of a serial line drop (a hangup). In modern systems, this signal usually means that the     \
		controlling pseudo or virtual terminal has been closed. Many daemons will reload their configuration files     \
		and reopen their logfiles instead of exiting when receiving this signal. nohup is a command to make a command  \
		ignore the signal.                                                                                             \
	*/                                                                                                                 \
	sigaction(SIGHUP, &sa, nullptr);                                                                                   \
	/*                                                                                                                 \
		The SIGILL signal is sent to a process when it attempts to execute an illegal, malformed, unknown, or          \
		privileged instruction.                                                                                        \
	*/                                                                                                                 \
	sigaction(SIGILL, &sa, nullptr);                                                                                   \
	/*                                                                                                                 \
		The SIGINT signal is sent to a process by its controlling terminal when a user wishes to interrupt the         \
		process. This is typically initiated by pressing Ctrl-C, but on some systems, the "delete" character or        \
		"break" key can be used.                                                                                       \
	*/                                                                                                                 \
	sigaction(SIGINT, &sa, nullptr);                                                                                   \
	/*                                                                                                                 \
		The SIGPIPE signal is sent to a process when it attempts to write to a pipe without a process connected to the \
		other end.                                                                                                     \
	*/                                                                                                                 \
	sigaction(SIGPIPE, &sa, nullptr);                                                                                  \
	/*                                                                                                                 \
		The SIGQUIT signal is sent to a process by its controlling terminal when the user requests that the process    \
		quit and perform a core dump.                                                                                  \
	*/                                                                                                                 \
	sigaction(SIGQUIT, &sa, nullptr);                                                                                  \
	/*                                                                                                                 \
		The SIGSEGV signal is sent to a process when it makes an invalid virtual memory reference, or segmentation     \
		fault, i.e. when it performs a segmentation violation.                                                         \
	*/                                                                                                                 \
	sigaction(SIGSEGV, &sa, nullptr);                                                                                  \
	/*                                                                                                                 \
		The SIGTERM signal is sent to a process to request its termination. Unlike the SIGKILL signal, it can be       \
		caught and interpreted or ignored by the process. This allows the process to perform nice termination          \
		releasing resources and saving state if appropriate. SIGINT is nearly identical to SIGTERM.                    \
	*/                                                                                                                 \
	sigaction(SIGTERM, &sa, nullptr);                                                                                  \
	/*                                                                                                                 \
		The SIGUSR1 and SIGUSR2 signals are sent to a process to indicate user-defined conditions.                     \
	*/                                                                                                                 \
	sigaction(SIGUSR1, &sa, nullptr);                                                                                  \
	sigaction(SIGUSR2, &sa, nullptr);

/**************************
 * @brief Parse parameters from manager. If required parameters are invalid or absent, error will be printed in cerr and
 * 1 is returned. Number of out variables will be initialized with parsed values, and some of them described in the
 * following list.
 *
 * @param argv (in) Arguments from main function.
 * @param name (in-mutable) Default type of application [std::string].
 * @param parameters (out) Object with parsed from argv[1] parameters [MSAPI::Json].
 * @param ip (out) IP address of application [std::string]. INADDR_LOOPBACK by default.
 * @param port (out) Port of application [unsigned short]. Random from 3000 by default. Can't be equal to zero.
 * @param managerPort (out) Port of manager [unsigned short]. Can't be equal to zero.
 * @param parentPath (internal) Parent directory for logger, root of build directory or executable directory by default
 * (512 bytes are reserved).
 * @param logLevel (internal) Level of logging, WARNING by default.
 * @param logInConsole (internal) Enable logging in console, false by default.
 * @param logInFile (internal) Enable logging in file, false by default.
 * @param separateDaysLogging (internal) Enable separating log files by days, true by default.
 *
 * @return 0 if something went wrong, otherwise 1.
 */
#define MSAPI_PARSE_PARAMETERS_FROM_MANAGER                                                                            \
	MSAPI::Json parameters{ argv[1] };                                                                                 \
	if (!parameters.Valid()) {                                                                                         \
		std::cerr << "Invalid parameters." << std::endl;                                                               \
		return 1;                                                                                                      \
	}                                                                                                                  \
                                                                                                                       \
	if (const auto* alias{ parameters.GetValue("name") }; alias != nullptr) {                                          \
		if (const auto* value{ std::get_if<std::string>(&alias->GetValue()) }; value != nullptr) {                     \
			if (!value->empty()) {                                                                                     \
				name = *value;                                                                                         \
			}                                                                                                          \
		}                                                                                                              \
		else {                                                                                                         \
			std::cerr << "Invalid type of name in parameters, string is expected." << std::endl;                       \
		}                                                                                                              \
	}                                                                                                                  \
	MSAPI::logger.SetName(name);                                                                                       \
                                                                                                                       \
	unsigned int ip{ INADDR_LOOPBACK };                                                                                \
	if (const auto* ipStr{ parameters.GetValue("ip") }; ipStr != nullptr) {                                            \
		if (const auto* value{ std::get_if<std::string>(&ipStr->GetValue()) }; value != nullptr) {                     \
			if (!value->empty()) {                                                                                     \
				const auto error{ std::from_chars(value->data(), value->data() + value->size(), ip).ec };              \
				if (error != std::errc{}) {                                                                            \
					std::cerr << "Cannot parse ip in parameters: " << *value                                           \
							  << ". Error: " << std::make_error_code(error).message() << "." << std::endl;             \
					return 1;                                                                                          \
				}                                                                                                      \
			}                                                                                                          \
		}                                                                                                              \
		else {                                                                                                         \
			std::cerr << "Invalid type of ip in parameters, string is expected." << std::endl;                         \
		}                                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
	unsigned short port{ 0 };                                                                                          \
	if (const auto* portStr{ parameters.GetValue("port") }; portStr != nullptr) {                                      \
		if (const auto* value{ std::get_if<std::string>(&portStr->GetValue()) }; value != nullptr) {                   \
			if (value->empty()) {                                                                                      \
				goto generatePort;                                                                                     \
			}                                                                                                          \
                                                                                                                       \
			const auto error{ std::from_chars(value->data(), value->data() + value->size(), port).ec };                \
			if (error == std::errc{}) {                                                                                \
				if (port == 0) {                                                                                       \
					std::cerr << "Port cannot be equal to zero." << std::endl;                                         \
					return 1;                                                                                          \
				}                                                                                                      \
			}                                                                                                          \
			else {                                                                                                     \
				std::cerr << "Cannot parse port in parameters: " << *value                                             \
						  << ". Error: " << std::make_error_code(error).message() << "." << std::endl;                 \
				goto generatePort;                                                                                     \
			}                                                                                                          \
		}                                                                                                              \
		else {                                                                                                         \
			std::cerr << "Invalid type of port in parameters, string is expected." << std::endl;                       \
			return 1;                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
	generatePort:                                                                                                      \
		port = static_cast<unsigned short>(MSAPI::Identifier::mersenne() % (65535 - 3000) + 3000);                     \
	}                                                                                                                  \
                                                                                                                       \
	const auto* managerPortStr{ parameters.GetValue("managerPort") };                                                  \
	if (managerPortStr == nullptr) {                                                                                   \
		std::cerr << "Absent manager port in parameters." << std::endl;                                                \
		return 1;                                                                                                      \
	}                                                                                                                  \
	const auto* managerValue{ std::get_if<std::string>(&managerPortStr->GetValue()) };                                 \
	if (managerValue == nullptr) {                                                                                     \
		std::cerr << "Invalid type of manager port in parameters, string is expected." << std::endl;                   \
		return 1;                                                                                                      \
	}                                                                                                                  \
	unsigned short managerPort{ 0 };                                                                                   \
	{                                                                                                                  \
		const auto error{                                                                                              \
			std::from_chars(managerValue->data(), managerValue->data() + managerValue->size(), managerPort).ec         \
		};                                                                                                             \
		if (error != std::errc{}) {                                                                                    \
			std::cerr << "Cannot parse manager port in parameters: " << *managerValue                                  \
					  << ". Error: " << std::make_error_code(error).message() << "." << std::endl;                     \
			return 1;                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
	if (managerPort == 0) {                                                                                            \
		std::cerr << "Manager port cannot be equal to zero." << std::endl;                                             \
		return 1;                                                                                                      \
	}                                                                                                                  \
                                                                                                                       \
	if (const auto* parentPath{ parameters.GetValue("parentPath") }; parentPath != nullptr) {                          \
		if (const auto* value{ std::get_if<std::string>(&parentPath->GetValue()) }; value != nullptr) {                \
			if (!value->empty()) {                                                                                     \
				MSAPI::logger.SetParentPath(*value);                                                                   \
			}                                                                                                          \
			else {                                                                                                     \
				goto setDefaultParentPath;                                                                             \
			}                                                                                                          \
		}                                                                                                              \
		else {                                                                                                         \
			std::cerr << "Invalid type of parent path in parameters, string is expected." << std::endl;                \
			return 1;                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
	setDefaultParentPath:                                                                                              \
		std::string executablePath;                                                                                    \
		executablePath.resize(512);                                                                                    \
		MSAPI::Helper::GetExecutableDir(executablePath);                                                               \
		if (executablePath.empty()) {                                                                                  \
			std::cerr << "Cannot get executable path." << std::endl;                                                   \
			return 1;                                                                                                  \
		}                                                                                                              \
                                                                                                                       \
		MSAPI::logger.SetParentPath(executablePath + "../");                                                           \
	}                                                                                                                  \
                                                                                                                       \
	if (const auto* logLevelStr{ parameters.GetValue("logLevel") }; logLevelStr != nullptr) {                          \
		if (const auto* value{ std::get_if<std::string>(&logLevelStr->GetValue()) }; value != nullptr) {               \
			if (!value->empty()) {                                                                                     \
				unsigned short logLevel{};                                                                             \
				const auto error{ std::from_chars(value->data(), value->data() + value->size(), logLevel).ec };        \
				if (error == std::errc{}) {                                                                            \
					if (logLevel < U(MSAPI::Log::Level::ERROR) || logLevel > U(MSAPI::Log::Level::PROTOCOL)) {         \
						std::cerr << "Invalid log level in parameters: " << *value << "." << std::endl;                \
						return 1;                                                                                      \
					}                                                                                                  \
					MSAPI::logger.SetLevelSave(static_cast<MSAPI::Log::Level>(logLevel));                              \
				}                                                                                                      \
				else {                                                                                                 \
					MSAPI::logger.SetLevelSave(MSAPI::Log::Level::WARNING);                                            \
				}                                                                                                      \
			}                                                                                                          \
			else {                                                                                                     \
				MSAPI::logger.SetLevelSave(MSAPI::Log::Level::WARNING);                                                \
			}                                                                                                          \
		}                                                                                                              \
		else {                                                                                                         \
			std::cerr << "Invalid type of log level in parameters, string is expected." << std::endl;                  \
			return 1;                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		MSAPI::logger.SetLevelSave(MSAPI::Log::Level::WARNING);                                                        \
	}                                                                                                                  \
                                                                                                                       \
	if (const auto* logInConsoleStr{ parameters.GetValue("logInConsole") }; logInConsoleStr != nullptr) {              \
		if (const auto* value{ std::get_if<std::string>(&logInConsoleStr->GetValue()) }; value != nullptr) {           \
			MSAPI::logger.SetToConsole(*value == "true");                                                              \
		}                                                                                                              \
		else {                                                                                                         \
			std::cerr << "Invalid type of log in console in parameters, string is expected." << std::endl;             \
			return 1;                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		MSAPI::logger.SetToConsole(false);                                                                             \
	}                                                                                                                  \
                                                                                                                       \
	if (const auto* logInFileStr{ parameters.GetValue("logInFile") }; logInFileStr != nullptr) {                       \
		if (const auto* value{ std::get_if<std::string>(&logInFileStr->GetValue()) }; value != nullptr) {              \
			MSAPI::logger.SetToFile(*value == "true");                                                                 \
		}                                                                                                              \
		else {                                                                                                         \
			std::cerr << "Invalid type of log in file in parameters, string is expected." << std::endl;                \
			return 1;                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		MSAPI::logger.SetToFile(false);                                                                                \
	}                                                                                                                  \
                                                                                                                       \
	if (const auto* separateDaysLoggingStr{ parameters.GetValue("separateDaysLogging") };                              \
		separateDaysLoggingStr != nullptr) {                                                                           \
                                                                                                                       \
		if (const auto* value{ std::get_if<std::string>(&separateDaysLoggingStr->GetValue()) }; value != nullptr) {    \
			MSAPI::logger.SetSeparateDays(*value == "true");                                                           \
		}                                                                                                              \
		else {                                                                                                         \
			std::cerr << "Invalid type of separate days logging in parameters, string is expected." << std::endl;      \
			return 1;                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		MSAPI::logger.SetSeparateDays(true);                                                                           \
	}                                                                                                                  \
                                                                                                                       \
	MSAPI::logger.Start();

#endif //* MSAPI_APPLICATION_H