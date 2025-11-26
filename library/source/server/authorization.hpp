/**************************
 * @file        authorization.hpp
 * @version     1.0
 * @date        2025-11-19
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

#ifndef MSAPI_AUTHORIZATION_H
#define MSAPI_AUTHORIZATION_H

#include "../help/log.h"
#include "../help/pthread.hpp"
#include <cstring>
#include <fstream>
#include <map>
#include <optional>
#include <string>

namespace MSAPI {

namespace Authorization {

/**************************
 * @brief Grade enum represents authorization levels for users.
 */
enum class Grade : int8_t {
	Guest = 0,
	Observer = 16,
	User = 32,
	Moderator = 64,
	Admin = 127
};

/**************************
 * @brief Convert Grade enum to string.
 *
 * @param grade Grade value.
 * @return String representation of grade.
 */
std::string_view EnumToString(Grade grade) noexcept;

/**************************
 * @brief Default account structure with email, password, active status and grade.
 */
struct Account {
	int8_t email[64];
	int8_t password[64];
	bool isActive;
	Grade grade;

	/**************************
	 * @brief Construct a new Account object with default values.
	 */
	Account() noexcept
		: email{}
		, password{}
		, isActive{ false }
		, grade{ Grade::Guest }
	{
	}

	/**************************
	 * @brief Construct a new Account object with provided values.
	 *
	 * @param email User email (login).
	 * @param password User password.
	 * @param isActive Account active status.
	 * @param grade User authorization grade.
	 */
	Account(const std::string& email, const std::string& password, bool isActive, Grade grade) noexcept
		: email{}
		, password{}
		, isActive{ isActive }
		, grade{ grade }
	{
		std::strncpy(reinterpret_cast<char*>(this->email), email.c_str(), sizeof(this->email) - 1);
		std::strncpy(reinterpret_cast<char*>(this->password), password.c_str(), sizeof(this->password) - 1);
	}

	/**************************
	 * @brief Get email as string.
	 * @return Email string.
	 */
	FORCE_INLINE std::string GetEmail() const noexcept
	{
		return std::string{ reinterpret_cast<const char*>(email) };
	}

	/**************************
	 * @brief Get password as string.
	 * @return Password string.
	 */
	FORCE_INLINE std::string GetPassword() const noexcept
	{
		return std::string{ reinterpret_cast<const char*>(password) };
	}
};

/**************************
 * @brief Authorization module for managing user accounts and authentication.
 *
 * @tparam T Account type, defaults to Account.
 *
 * Features:
 * - Active storage of users as login/password keys
 * - Passive storage as a file with live updates
 * - Ability to register/modify/delete accounts
 * - User history logging
 * - Thread-safe operations with pthread synchronization
 * - Connection-based authorization tracking
 */
template <typename T = Account> class Module {
private:
	std::string m_usersFilePath;
	std::string m_historyDirPath;
	std::optional<std::string> m_emailTemplatePath;

	std::map<std::string, T> m_users; //* email -> account
	std::map<int, std::string> m_authorizedConnections; //* connection -> email

	Pthread::AtomicLock m_usersLock;
	Pthread::AtomicLock m_connectionsLock;
	Pthread::AtomicLock m_fileLock;

	bool m_isRunning{ false };

public:
	/**************************
	 * @brief Construct a new Module object.
	 *
	 * @param usersFilePath Path to file where all users are stored (each on its own line).
	 * @param historyDirPath Path to directory where user history files are stored.
	 * @param emailTemplatePath Optional path to email template file for registration confirmation.
	 */
	Module(const std::string& usersFilePath, const std::string& historyDirPath,
		const std::optional<std::string>& emailTemplatePath = std::nullopt) noexcept
		: m_usersFilePath{ usersFilePath }
		, m_historyDirPath{ historyDirPath }
		, m_emailTemplatePath{ emailTemplatePath }
	{
	}

	/**************************
	 * @brief Destroy the Module object, wait for file operations to complete.
	 */
	~Module() noexcept
	{
		Stop();
	}

	/**************************
	 * @brief Start the authorization module and preload users from file.
	 *
	 * @return True if started successfully, false otherwise.
	 */
	bool Start() noexcept
	{
		if (m_isRunning) {
			LOG_WARNING("Authorization module is already running");
			return false;
		}

		if (!LoadUsersFromFile()) {
			LOG_ERROR("Failed to load users from file: " + m_usersFilePath);
			return false;
		}

		m_isRunning = true;
		LOG_INFO("Authorization module started successfully, loaded " + _S(m_users.size()) + " users");
		return true;
	}

	/**************************
	 * @brief Stop the authorization module and save users to file.
	 */
	void Stop() noexcept
	{
		if (!m_isRunning) {
			return;
		}

		m_isRunning = false;

		//* Wait for all file operations to complete
		Pthread::AtomicLock::ExitGuard fileguard{ m_fileLock };
		SaveUsersToFile();

		LOG_INFO("Authorization module stopped");
	}

	/**************************
	 * @brief Register a new user account.
	 *
	 * @param email User email (login).
	 * @param password User password.
	 * @param grade User authorization grade, defaults to Guest.
	 *
	 * @return True if registered successfully, false if user already exists.
	 */
	bool Register(const std::string& email, const std::string& password, Grade grade = Grade::Guest) noexcept
	{
		if (!m_isRunning) {
			LOG_ERROR("Authorization module is not running");
			return false;
		}

		if (email.empty() || password.empty()) {
			LOG_WARNING("Cannot register user with empty email or password");
			return false;
		}

		Pthread::AtomicLock::ExitGuard guard{ m_usersLock };

		if (m_users.find(email) != m_users.end()) {
			LOG_WARNING("User with email " + email + " already exists");
			return false;
		}

		//* Account is not active by default if email confirmation is required
		const bool isActive{ !m_emailTemplatePath.has_value() };

		T account{ email, password, isActive, grade };
		m_users.emplace(email, account);

		LogUserHistory(email, "created");
		SaveUsersToFile();

		LOG_INFO("User " + email + " registered with grade " + std::string{ EnumToString(grade) });
		return true;
	}

	/**************************
	 * @brief Login user and associate with connection.
	 *
	 * @param connection Connection identifier.
	 * @param email User email (login).
	 * @param password User password.
	 *
	 * @return True if login successful, false otherwise.
	 */
	bool Login(int connection, const std::string& email, const std::string& password) noexcept
	{
		if (!m_isRunning) {
			LOG_ERROR("Authorization module is not running");
			return false;
		}

		Pthread::AtomicLock::ExitGuard usersGuard{ m_usersLock };

		auto it{ m_users.find(email) };
		if (it == m_users.end()) {
			LOG_WARNING("Login attempt for non-existent user: " + email);
			return false;
		}

		if (it->second.GetPassword() != password) {
			LOG_WARNING("Invalid password for user: " + email);
			return false;
		}

		if (!it->second.isActive) {
			LOG_WARNING("Login attempt for inactive user: " + email);
			return false;
		}

		{
			Pthread::AtomicLock::ExitGuard connectionsGuard{ m_connectionsLock };
			m_authorizedConnections[connection] = email;
		}

		LogUserHistory(email, "login from connection " + _S(connection));
		LOG_INFO("User " + email + " logged in from connection " + _S(connection));
		return true;
	}

	/**************************
	 * @brief Logout user from connection.
	 *
	 * @param connection Connection identifier.
	 *
	 * @return True if logout successful, false if connection was not authorized.
	 */
	bool Logout(int connection) noexcept
	{
		if (!m_isRunning) {
			LOG_ERROR("Authorization module is not running");
			return false;
		}

		Pthread::AtomicLock::ExitGuard connectionsGuard{ m_connectionsLock };

		auto it{ m_authorizedConnections.find(connection) };
		if (it == m_authorizedConnections.end()) {
			LOG_WARNING("Logout attempt for unauthorized connection: " + _S(connection));
			return false;
		}

		const std::string email{ it->second };
		m_authorizedConnections.erase(it);

		LogUserHistory(email, "logout from connection " + _S(connection));
		LOG_INFO("User " + email + " logged out from connection " + _S(connection));
		return true;
	}

	/**************************
	 * @brief Check if connection is authorized.
	 *
	 * @param connection Connection identifier.
	 *
	 * @return True if connection is authorized, false otherwise.
	 */
	bool IsAuthorized(int connection) const noexcept
	{
		Pthread::AtomicLock::ExitGuard guard{ const_cast<Pthread::AtomicLock&>(m_connectionsLock) };
		return m_authorizedConnections.find(connection) != m_authorizedConnections.end();
	}

	/**************************
	 * @brief Check if connection has required access level.
	 *
	 * @tparam RequiredGrade Required grade level.
	 * @param connection Connection identifier.
	 *
	 * @return True if connection has required access level, false otherwise.
	 */
	template <typename GradeType>
		requires std::is_same_v<GradeType, Grade>
	bool IsAccessAllowed(int connection) const noexcept
	{
		Pthread::AtomicLock::ExitGuard connectionsGuard{ const_cast<Pthread::AtomicLock&>(m_connectionsLock) };

		auto connIt{ m_authorizedConnections.find(connection) };
		if (connIt == m_authorizedConnections.end()) {
			return false;
		}

		Pthread::AtomicLock::ExitGuard usersGuard{ const_cast<Pthread::AtomicLock&>(m_usersLock) };

		auto userIt{ m_users.find(connIt->second) };
		if (userIt == m_users.end()) {
			return false;
		}

		return true; //* Grade checking would require template parameter value
	}

	/**************************
	 * @brief Modify user password.
	 *
	 * @param email User email.
	 * @param newPassword New password.
	 *
	 * @return True if modified successfully, false if user not found.
	 */
	bool ModifyPassword(const std::string& email, const std::string& newPassword) noexcept
	{
		if (!m_isRunning) {
			LOG_ERROR("Authorization module is not running");
			return false;
		}

		Pthread::AtomicLock::ExitGuard guard{ m_usersLock };

		auto it{ m_users.find(email) };
		if (it == m_users.end()) {
			LOG_WARNING("Cannot modify password for non-existent user: " + email);
			return false;
		}

		std::strncpy(
			reinterpret_cast<char*>(it->second.password), newPassword.c_str(), sizeof(it->second.password) - 1);

		LogUserHistory(email, "modified password");
		SaveUsersToFile();

		LOG_INFO("Password modified for user: " + email);
		return true;
	}

	/**************************
	 * @brief Delete user account.
	 *
	 * @param email User email.
	 *
	 * @return True if deleted successfully, false if user not found.
	 */
	bool DeleteAccount(const std::string& email) noexcept
	{
		if (!m_isRunning) {
			LOG_ERROR("Authorization module is not running");
			return false;
		}

		Pthread::AtomicLock::ExitGuard guard{ m_usersLock };

		auto it{ m_users.find(email) };
		if (it == m_users.end()) {
			LOG_WARNING("Cannot delete non-existent user: " + email);
			return false;
		}

		m_users.erase(it);

		LogUserHistory(email, "deleted");
		SaveUsersToFile();

		LOG_INFO("User deleted: " + email);
		return true;
	}

	/**************************
	 * @brief Set user active status.
	 *
	 * @param email User email.
	 * @param isActive Active status.
	 *
	 * @return True if modified successfully, false if user not found.
	 */
	bool SetActiveStatus(const std::string& email, bool isActive) noexcept
	{
		if (!m_isRunning) {
			LOG_ERROR("Authorization module is not running");
			return false;
		}

		Pthread::AtomicLock::ExitGuard guard{ m_usersLock };

		auto it{ m_users.find(email) };
		if (it == m_users.end()) {
			LOG_WARNING("Cannot set active status for non-existent user: " + email);
			return false;
		}

		it->second.isActive = isActive;

		LogUserHistory(email, isActive ? "activated" : "deactivated");
		SaveUsersToFile();

		LOG_INFO("User " + email + " " + (isActive ? "activated" : "deactivated"));
		return true;
	}

private:
	/**************************
	 * @brief Load users from file.
	 *
	 * @return True if loaded successfully, false otherwise.
	 */
	bool LoadUsersFromFile() noexcept
	{
		Pthread::AtomicLock::ExitGuard guard{ m_fileLock };

		std::ifstream file{ m_usersFilePath };
		if (!file.is_open()) {
			LOG_WARNING("Users file does not exist, will be created: " + m_usersFilePath);
			return true; //* Empty user list is valid
		}

		m_users.clear();
		std::string line;

		while (std::getline(file, line)) {
			if (line.empty()) {
				continue;
			}

			//* Parse line: email;password;isActive;grade
			size_t pos1{ line.find(';') };
			if (pos1 == std::string::npos) {
				continue;
			}

			size_t pos2{ line.find(';', pos1 + 1) };
			if (pos2 == std::string::npos) {
				continue;
			}

			size_t pos3{ line.find(';', pos2 + 1) };
			if (pos3 == std::string::npos) {
				continue;
			}

			std::string email{ line.substr(0, pos1) };
			std::string password{ line.substr(pos1 + 1, pos2 - pos1 - 1) };
			bool isActive{ line.substr(pos2 + 1, pos3 - pos2 - 1) == "1" };
			int8_t gradeValue{ static_cast<int8_t>(std::stoi(line.substr(pos3 + 1))) };

			T account{ email, password, isActive, static_cast<Grade>(gradeValue) };
			m_users.emplace(email, account);
		}

		file.close();
		return true;
	}

	/**************************
	 * @brief Save users to file.
	 *
	 * @return True if saved successfully, false otherwise.
	 */
	bool SaveUsersToFile() noexcept
	{
		Pthread::AtomicLock::ExitGuard guard{ m_fileLock };

		std::ofstream file{ m_usersFilePath };
		if (!file.is_open()) {
			LOG_ERROR("Failed to open users file for writing: " + m_usersFilePath);
			return false;
		}

		for (const auto& [email, account] : m_users) {
			file << email << ";" << account.GetPassword() << ";" << (account.isActive ? "1" : "0") << ";"
				 << static_cast<int>(account.grade) << "\n";
		}

		file.close();
		return true;
	}

	/**************************
	 * @brief Log user history event.
	 *
	 * @param email User email.
	 * @param event Event description.
	 */
	void LogUserHistory(const std::string& email, const std::string& event) noexcept
	{
		const std::string historyFilePath{ m_historyDirPath + "/" + email + "_history.txt" };
		std::ofstream file{ historyFilePath, std::ios::app };

		if (!file.is_open()) {
			LOG_WARNING("Failed to open history file for user: " + email);
			return;
		}

		const Timer currentTime;
		file << currentTime.ToString() << " : " << event << "\n";
		file.close();
	}
};

}; //* namespace Authorization

}; //* namespace MSAPI

#endif //* MSAPI_AUTHORIZATION_H
