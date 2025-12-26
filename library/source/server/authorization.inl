/**************************
 * @file        authorization.inl
 * @version     6.0
 * @date        2025-12-23
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
 * @brief Generic authorization module implementation.
 */

#ifndef MSAPI_AUTHORIZATION_INL
#define MSAPI_AUTHORIZATION_INL

#include "../help/helper.h"
#include "../help/io.inl"
#include "../help/log.h"
#include "../help/pthread.hpp"
#include "../help/sha256.inl"
#include <cctype>
#include <random>
#include <sys/stat.h>

namespace MSAPI {

namespace Authorization {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

namespace Base {

/**
 * @brief Enumeration of user grades/roles.
 */
enum class Grade : int16_t { Guest = 0, Observer = 16, User = 32, Moderator = 64, Admin = 128 };

constexpr size_t MAX_LOGIN_SIZE = 43;
constexpr size_t SALT_SIZE = 16;
constexpr size_t PASSWORD_HASH_SIZE = 32;

/**
 * @brief Account class representing a user account, can be used as a base to extended functionality.
 * - Deactivated by default. Deactivated accounts cannot be authenticated.
 * - Not initialized by default. Initialized on first password set. Uninitialized account cannot be authenticated.
 * - Can be blocked until a specific time.
 * - Password is stored as SHA-256(salt + password), where salt is randomly generated on first password set.
 *
 * @tparam G Type of grade enumeration.
 */
template <typename G = Grade>
	requires(std::is_enum_v<G> && sizeof(G) == 2)
class Account {
private:
	Timer m_blockedTill{};
	char m_login[MAX_LOGIN_SIZE + 1]{};
	uint8_t m_salt[SALT_SIZE]{};
	uint8_t m_password[PASSWORD_HASH_SIZE]{};
	G m_grade{};
	bool m_isActivated{};
	bool m_isInitialized{};

public:
	/**
	 * @brief Default destructor.
	 */
	virtual ~Account() = default;

	/**
	 * @return Timer object representing the blocked till time.
	 */
	FORCE_INLINE [[nodiscard]] Timer GetBlockedTill() const noexcept { return m_blockedTill; }

	/**
	 * @return C-string representing the login of the account.
	 */
	FORCE_INLINE [[nodiscard]] const char* GetLogin() const noexcept { return m_login; }

	/**
	 * @return Grade of the account.
	 */
	FORCE_INLINE [[nodiscard]] G GetGrade() const noexcept { return m_grade; }

	/**
	 * @return True if the account is activated, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool IsActive() const noexcept { return m_isActivated; }

	/**
	 * @brief Authenticate the account with the provided password.
	 *
	 * @param password The password to authenticate with.
	 * @param error Reference to a string to store error message if authentication fails.
	 *
	 * @return True if authentication is successful, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool Authenticate(const std::string_view password, std::string& error) noexcept
	{
		if (!m_isActivated) {
			error = "Account is not activated";
			return false;
		}

		if (!m_isInitialized) {
			error = "Account is not initialized";
			return false;
		}

		if (m_blockedTill > Timer{}) {
			const Timer now{};
			if (now < m_blockedTill) {
				error = std::format("Account is blocked till {}", m_blockedTill.ToString());
				return false;
			}
		}

		Sha256 hash;
		hash.Update(std::string_view{ reinterpret_cast<const char*>(m_salt), SALT_SIZE });
		hash.Update(password);

		return memcmp(hash.GetDigits().data(), m_password, PASSWORD_HASH_SIZE) == 0;
	}

	/**
	 * @brief Set a new login for the account.
	 *
	 * @param newLogin The new login to set.
	 */
	FORCE_INLINE void SetLogin(const std::string_view newLogin) noexcept
	{
		memcpy(m_login, newLogin.data(), newLogin.size());
		m_login[newLogin.size()] = '\0';
	}

	/**
	 * @brief Set a new password for the account. Initializes the account if not already initialized.
	 *
	 * @param newPassword The new password to set.
	 */
	FORCE_INLINE void SetPassword(const std::string_view newPassword) noexcept
	{
		if (!m_isInitialized) {
			uint64_t randomValues[2];
			static_assert(SALT_SIZE == sizeof(randomValues));

			randomValues[0] = UINT64(m_blockedTill.GetNanoseconds()) << 44
				| (UINT64(reinterpret_cast<uintptr_t>(this)) << 20 & 0x00000FFF00000000)
				| (UINT64(m_blockedTill.GetSeconds()) & 0x00000000FFFFFFFF);
			Timer now{};
			randomValues[1] = UINT64(now.GetNanoseconds()) << 44
				| (UINT64(reinterpret_cast<uintptr_t>(this)) << 32 & 0x00000FFF00000000)
				| (UINT64(now.GetSeconds()) & 0x00000000FFFFFFFF);

			memcpy(m_salt, randomValues, SALT_SIZE);

			m_isInitialized = true;
			m_blockedTill = Timer{ 0 };
		}

		Sha256 hash;
		hash.Update(std::string_view{ reinterpret_cast<const char*>(m_salt), SALT_SIZE });
		hash.Update(newPassword);
		const auto digits{ hash.GetDigits() };
		memcpy(m_password, digits.data(), PASSWORD_HASH_SIZE);
	}

	/**
	 * @brief Backup the password hash into the provided buffer.
	 *
	 * @param buffer The buffer to store the password hash.
	 */
	FORCE_INLINE void BackupPassword(std::array<uint8_t, PASSWORD_HASH_SIZE>& buffer) const noexcept
	{
		memcpy(buffer.data(), m_password, PASSWORD_HASH_SIZE);
	}

	/**
	 * @brief Restore the password hash from the provided buffer.
	 *
	 * @param buffer The buffer containing the password hash.
	 */
	FORCE_INLINE void RestorePassword(const std::array<uint8_t, PASSWORD_HASH_SIZE>& buffer) noexcept
	{
		memcpy(m_password, buffer.data(), PASSWORD_HASH_SIZE);
	}

	/**
	 * @brief Set a new grade for the account.
	 *
	 * @param newGrade The new grade to set.
	 */
	FORCE_INLINE void SetGrade(const G newGrade) noexcept { m_grade = newGrade; }

	/**
	 * @brief Activate or deactivate the account.
	 *
	 * @param isActivated True to activate the account, false to deactivate.
	 */
	FORCE_INLINE void SetActivated(const bool isActivated) noexcept { m_isActivated = isActivated; }

	/**
	 * @brief Block the account till the specified time.
	 *
	 * @param blockedTill The time till which the account is blocked.
	 */
	FORCE_INLINE void BlockTill(const Timer blockedTill) noexcept { m_blockedTill = blockedTill; }

	/**
	 * @return True if the account is initialized, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool IsInitialized() const noexcept { return m_isInitialized; }

	/**
	 * @brief Initialize or deinitialize the account.
	 *
	 * @param isInitialized True to set the account as initialized, false otherwise.
	 */
	FORCE_INLINE void SetInitialized(const bool isInitialized) noexcept { m_isInitialized = isInitialized; }
};

/**
 * @brief Generic authorization module class, provides thread-safe account management and authentication. Can be used as
 * a base to extended functionality.
 * - Supports automatic logout of inactive accounts after a specified timeout.
 * - Customizable account login and password policies.
 * - Store account binary data and its history in {executable}/../data/accounts/ directory. Where each account is stored
 * in a separate file with filename as the account login. Directory and files rights are 0750 and 0640 respectively.
 * - Update account data file on each modification.
 * - Preload all accounts from data directory on construction.
 * - Provide authentication to one connection per account.
 * - Grade based access control.
 *
 * @tparam T Type of account class.
 * @tparam G Type of grade.
 */
template <typename T = Account<Grade>, typename G = Grade>
	requires std::is_base_of_v<Account<G>, T>
class Module : protected Timer::Event::IHandler {
private:
	/**
	 * @brief Structure for containing account data along with additional control information.
	 */
	class AccountData {
	private:
		T m_account;
		std::string m_dataPath;
		std::unique_ptr<Pthread::AtomicRWLock> m_rwLock;
		int m_connection{ -1 };
		Timer m_lastActivity{ 0 };

	public:
		/**
		 * @brief Construct a new AccountData object.
		 *
		 * @param account The account object.
		 * @param dataPath The file path where the account data is stored.
		 */
		FORCE_INLINE explicit AccountData(T&& account, std::string&& dataPath) noexcept
			: m_account{ std::forward<T>(account) }
			, m_dataPath{ std::move(dataPath) }
			, m_rwLock{ std::make_unique<Pthread::AtomicRWLock>() }
		{
		}

		/**
		 * @brief Default constructor for AccountData.
		 */
		FORCE_INLINE AccountData() noexcept = default;

		AccountData(const AccountData&) = delete;
		AccountData& operator=(const AccountData&) = delete;

		/**
		 * @brief Move constructor for AccountData.
		 *
		 * @param other The other AccountData object to move from.
		 */
		FORCE_INLINE AccountData(AccountData&& other) noexcept
			: m_account{ std::move(other.m_account) }
			, m_dataPath{ std::move(other.m_dataPath) }
			, m_rwLock{ std::move(other.m_rwLock) }
			, m_connection{ other.m_connection }
			, m_lastActivity{ other.m_lastActivity }
		{
		}

		/**
		 * @brief Move assignment operator for AccountData.
		 *
		 * @param other The other AccountData object to move from.
		 *
		 * @return Reference to this AccountData object.
		 */
		FORCE_INLINE AccountData& operator=(AccountData&& other) noexcept
		{
			if (this != &other) {
				m_account = std::move(other.m_account);
				m_dataPath = std::move(other.m_dataPath);
				m_rwLock = std::move(other.m_rwLock);
				m_connection = other.m_connection;
				m_lastActivity = other.m_lastActivity;
			}

			return *this;
		}

		/**
		 * @brief Destroy the AccountData object, acquiring a write lock to ensure all operations are completed.
		 */
		FORCE_INLINE ~AccountData() noexcept { Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ *m_rwLock }; }

		/**
		 * @return Reference to the account object.
		 */
		FORCE_INLINE T& GetAccount() noexcept { return m_account; }

		/**
		 * @return Const reference to the file path where the account data is stored.
		 */
		FORCE_INLINE const std::string& GetDataPath() const noexcept { return m_dataPath; }

		/**
		 * @brief Set a new data path for the account data file.
		 *
		 * @param newDataPath The new file path.
		 */
		FORCE_INLINE void SetDataPath(std::string&& newDataPath) noexcept { m_dataPath = std::move(newDataPath); }

		/**
		 * @brief Update the last activity timestamp to the current time.
		 */
		FORCE_INLINE void UpdateLastActivity() noexcept { m_lastActivity = Timer{}; }

		/**
		 * @return The last activity timestamp.
		 */
		FORCE_INLINE Timer GetLastActivity() const noexcept { return m_lastActivity; }

		/**
		 * @return Reference to the read-write lock for this account data.
		 */
		FORCE_INLINE Pthread::AtomicRWLock& GetRWLock() noexcept { return *m_rwLock.get(); }

		/**
		 * @brief Save the account data to its associated file.
		 *
		 * @return True if the save operation was successful, false otherwise.
		 */
		FORCE_INLINE [[nodiscard]] bool Save()
		{
			// Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_rwLock };
			if (!IO::SaveBinaryOnOffset<0640>(m_account, m_dataPath.c_str(), 0)) [[unlikely]] {
				LOG_ERROR_NEW("Cannot save account data to file: {}", m_dataPath);
				return false;
			}

			return true;
		}

		/**
		 * @brief Set the connection associated with this account.
		 *
		 * @param connection The connection identifier.
		 */
		FORCE_INLINE void SetConnection(const int connection) noexcept { m_connection = connection; }

		/**
		 * @return The connection identifier associated with this account.
		 */
		FORCE_INLINE int GetConnection() const noexcept { return m_connection; }
	};

private:
	std::unordered_map<int, std::shared_ptr<AccountData>> m_activeConnectionToAccountData;
	Pthread::AtomicRWLock m_connectionsLock;
	std::unordered_map<size_t, std::shared_ptr<AccountData>> m_loginHashToData;
	Pthread::AtomicRWLock m_accountsLock;
	std::string m_dataPath;
	Timer::Event m_logoutEvent{ this };
	Timer::Duration m_logoutTimeout{ Timer::Duration::CreateHours(12) };
	Timer m_initializationTime{};
	bool m_isInitialized{ false };

public:
	/**
	 * @brief Construct a new Module object, loading existing accounts from the {executable}/../data/accounts/
	 * directory.
	 * - Uninitialized accounts are not loaded.
	 * - If the accounts data directory does not exist, it will be created with permissions 0750.
	 * - On failure to read any account data file, the initialization will stop and the module will be marked as
	 * uninitialized.
	 */
	FORCE_INLINE Module()
	{
		m_dataPath.resize(512);
		Helper::GetExecutableDir(m_dataPath);
		if (m_dataPath.empty()) [[unlikely]] {
			LOG_ERROR("Cannot get executable path");
			return;
		}

		m_dataPath += "../data/accounts/";
		if (IO::HasPath(m_dataPath.c_str())) {
			IO::Directory::ExitGuard guard{ m_dataPath.c_str() };
			if (guard.value == nullptr) [[unlikely]] {
				LOG_ERROR_NEW(
					"Cannot open accounts data directory: {}. Error №{}: {}", m_dataPath, errno, std::strerror(errno));
				return;
			}

			std::vector<std::string> accounts;
			if (!IO::List<IO::FileType::Regular>(accounts, guard.value)) [[unlikely]] {
				LOG_ERROR_NEW("Cannot list accounts data directory: {}", m_dataPath);
				return;
			}

			T object;
			for (const auto& account : accounts) {
				std::string accountFilePath{ m_dataPath + account };
				if (!IO::ReadBinary(&object, accountFilePath.c_str())) [[unlikely]] {
					LOG_ERROR_NEW("Cannot read account data from file: {}", accountFilePath);
					return;
				}

				if (!object.IsInitialized()) {
					continue;
				}

				m_loginHashToData.emplace(std::hash<std::string_view>{}(object.GetLogin()),
					std::make_shared<AccountData>(std::move(object), std::move(accountFilePath)));
			}

			LOG_DEBUG_NEW("Loaded {} accounts from file: {}", m_loginHashToData.size(), m_dataPath);
		}
		else {
			if (!IO::CreateDir<0750>(m_dataPath.c_str())) [[unlikely]] {
				LOG_ERROR_NEW("Cannot create accounts data directory: {}", m_dataPath);
				return;
			}

			LOG_DEBUG_NEW("Accounts data path does not exist: {}, starting with zero accounts", m_dataPath);
		}

		m_logoutEvent.Start(m_logoutTimeout.GetSeconds(), 60);
		m_isInitialized = true;
	}

	/**
	 * @brief Destroy the Module object, stopping the logout event and acquiring write locks.
	 */
	FORCE_INLINE ~Module() override
	{
		m_logoutEvent.Stop();
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guardAccounts{ m_accountsLock };
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guardConnections{ m_connectionsLock };
		for (auto& [hash, accountData] : m_loginHashToData) {
			accountData->GetRWLock().WriteLock();
		}
	}

	/**
	 * @return True if the module is initialized, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool IsInitialized() const noexcept { return m_isInitialized; }

	/**
	 * @brief Set the logout timeout duration. Accounts inactive for longer than this duration will be logged out
	 * automatically.
	 *
	 * @param duration The duration to set as the logout timeout.
	 */
	FORCE_INLINE void SetLogoutTimeout(const Timer::Duration duration)
	{
		if (duration.GetSeconds() <= 0) {
			LOG_WARNING("Logout timeout cannot be zero or negative, ignoring");
			return;
		}

		m_logoutEvent.Stop();
		const auto now{ Timer{} };
		if (m_initializationTime + m_logoutTimeout < now) {
			m_logoutEvent.Start(60, 60, true);
		}
		else {
			const auto toFirstCheck{ Timer::Duration{ now - (m_initializationTime + m_logoutTimeout) }.GetSeconds() };
			if (toFirstCheck < 60) {
				m_logoutEvent.Start(60, 60);
			}
			else {
				m_logoutEvent.Start(toFirstCheck, 60);
			}
		}

		m_logoutTimeout = duration;
		LOG_DEBUG_NEW("Set logout timeout to {} minutes", m_logoutTimeout.GetMinutes());
	}

	/**
	 * @return The current logout timeout duration.
	 */
	FORCE_INLINE [[nodiscard]] Timer::Duration GetLogoutTimeout() const noexcept { return m_logoutTimeout; }

	/**
	 * @brief Register a new account with the provided login and password, check login and password requirements. Newly
	 * registered accounts are deactivated by default.
	 *
	 * @param login The login for the new account.
	 * @param password The password for the new account.
	 * @param error Reference to a string to store error message if registration fails.
	 *
	 * @return True if registration is successful, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool RegisterAccount(
		const std::string_view login, const std::string_view password, std::string& error)
	{
		if (!CheckLoginRequirements(login, error)) {
			return false;
		}

		if (!CheckPasswordRequirements(password, error)) {
			return false;
		}

		const auto loginHash{ std::hash<std::string_view>{}(login) };
		typename std::unordered_map<size_t, std::shared_ptr<AccountData>>::iterator loginHashToDataIt;
		{
			Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
			if (m_loginHashToData.find(loginHash) != m_loginHashToData.end()) {
				return false;
			}

			T newAccount;
			newAccount.SetLogin(login);
			loginHashToDataIt
				= m_loginHashToData
					  .emplace(loginHash,
						  std::make_shared<AccountData>(std::move(newAccount), m_dataPath + std::string{ login }))
					  .first;
		}

		auto& accountData{ loginHashToDataIt->second };
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
		accountData->GetAccount().SetPassword(password);

		if (!accountData->Save()) [[unlikely]] {
			return false;
		}

		OnAccountActivity(*accountData, "Account is registered");
		return true;
	}

	/**
	 * @brief Check if the provided login meets the account login requirements. By default, checks for non-empty login
	 * and maximum length.
	 *
	 * @param login The login to check.
	 * @param error Reference to a string to store error message if requirements are not met.
	 *
	 * @return True if the login meets the requirements, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] virtual bool CheckLoginRequirements(const std::string_view login, std::string& error)
	{
		if (login.empty()) {
			error = "Login cannot be empty";
			return false;
		}

		if (login.size() > MAX_LOGIN_SIZE) {
			error = std::format("Login size cannot be greater than {} characters", MAX_LOGIN_SIZE);
			return false;
		}

		return true;
	}

	/**
	 * @brief Check if the provided password meets the account password requirements. By default, checks for length
	 * between 8 and 28 characters, and presence of at least one lowercase letter, one uppercase letter, one digit,
	 * and one special character.
	 *
	 * @param password The password to check.
	 * @param error Reference to a string to store error message if requirements are not met.
	 *
	 * @return True if the password meets the requirements, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] virtual bool CheckPasswordRequirements(
		const std::string_view password, std::string& error)
	{
		if (password.size() < 8) {
			error = "Password size cannot be less than 8 characters";
			return false;
		}

		if (password.size() > 28) {
			error = "Password size cannot be greater than 28 characters";
			return false;
		}

		bool hasLower{};
		bool hasUpper{};
		bool hasDigit{};
		bool hasSpecial{};
		bool valid{ false };

		for (const char ch : password) {
			if (std::islower(ch)) {
				hasLower = true;
			}
			else if (std::isupper(ch)) {
				hasUpper = true;
			}
			else if (std::isdigit(ch)) {
				hasDigit = true;
			}
			else if (!std::isspace(ch)) {
				hasSpecial = true;
			}

			if (hasLower && hasUpper && hasDigit && hasSpecial) {
				valid = true;
				break;
			}
		}

		if (valid) {
			return true;
		}

		bool empty{ true };

		if (!hasLower) {
			empty = false;
			error = "Password must contain at least one lowercase letter";
		}

		if (!hasUpper) {
			if (!empty) {
				error += ", at least one uppercase letter";
				empty = false;
			}
			else {
				error = "Password must contain at least one uppercase letter";
			}
		}

		if (!hasDigit) {
			if (!empty) {
				error += ", at least one digit";
				empty = false;
			}
			else {
				error = "Password must contain at least one digit";
			}
		}

		if (!hasSpecial) {
			if (!empty) {
				error += ", at least one special character";
			}
			else {
				error = "Password must contain at least one special character";
			}
		}

		return false;
	}

	/**
	 * @brief Delete the account with the specified login. Marks the account as uninitialized and deactivated, and logs
	 * out from active connection.
	 *
	 * @param login The login of the account to delete.
	 */
	FORCE_INLINE void DeleteAccount(const std::string_view login)
	{
		const auto loginHash{ std::hash<std::string_view>{}(login) };
		std::shared_ptr<AccountData> accountData;
		{
			Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
			auto accountDataIt{ m_loginHashToData.find(loginHash) };
			if (accountDataIt == m_loginHashToData.end()) {
				LOG_DEBUG_NEW("Cannot find account with login: {}", login);
				return;
			}

			accountData = accountDataIt->second;
			m_loginHashToData.erase(accountDataIt);
		}

		if (accountData->GetConnection() != -1) {
			Pthread::AtomicRWLock::ExitGuard<Pthread::write> connectionsGuard{ m_connectionsLock };
			m_activeConnectionToAccountData.erase(accountData->GetConnection());
			accountData->SetConnection(-1);
			OnAccountActivity(*accountData, "Account is logged out due to deletion");
		}

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
		accountData->GetAccount().SetInitialized(false);
		accountData->GetAccount().SetActivated(false);
		OnAccountActivity(*accountData, "Account is marked as uninitialized and deactivated");
		if (!accountData->Save()) [[unlikely]] {
			LOG_ERROR("Failed to save account data after deletion");
		}
	}

	/**
	 * @brief Modify the login of an existing account, checking login requirements and ensuring uniqueness.
	 *
	 * @attention On failure, the account data is reverted to its original state.
	 *
	 * @param oldLogin The current login of the account.
	 * @param newLogin The new login to set for the account.
	 * @param error Reference to a string to store error message if modification fails.
	 *
	 * @return True if the login modification is successful, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool ModifyAccountLogin(
		const std::string_view oldLogin, const std::string_view newLogin, std::string& error)
	{
		if (!CheckLoginRequirements(newLogin, error)) {
			return false;
		}

		const auto oldLoginHash{ std::hash<std::string_view>{}(oldLogin) };
		const auto newLoginHash{ std::hash<std::string_view>{}(newLogin) };
		std::shared_ptr<AccountData> accountData;
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
			auto oldAccountDataIt{ m_loginHashToData.find(oldLoginHash) };
			if (oldAccountDataIt == m_loginHashToData.end()) {
				LOG_DEBUG_NEW("Cannot find account with login: {}", oldLogin);
				return false;
			}

			if (m_loginHashToData.find(newLoginHash) != m_loginHashToData.end()) {
				LOG_DEBUG_NEW("Account with login: {} already exists", newLogin);
				return false;
			}

			accountData = oldAccountDataIt->second;
			m_loginHashToData.emplace(newLoginHash, accountData);
			m_loginHashToData.erase(oldAccountDataIt);
		}

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> accountGuard{ accountData->GetRWLock() };
		accountData->GetAccount().SetLogin(newLogin);
		if (!accountData->Save()) [[unlikely]] {
			accountData->GetAccount().SetLogin(oldLogin);

			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
			m_loginHashToData.emplace(oldLoginHash, accountData);
			m_loginHashToData.erase(newLoginHash);

			LOG_WARNING("Failed to save account data after login change, reverting changes");
			return false;
		}

		std::string newLoginPath{ m_dataPath + std::string{ newLogin } };
		if (!IO::Rename(accountData->GetDataPath().c_str(), newLoginPath.c_str())) [[unlikely]] {
			accountData->GetAccount().SetLogin(oldLogin);

			const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
			m_loginHashToData.emplace(oldLoginHash, accountData);
			m_loginHashToData.erase(newLoginHash);

			LOG_WARNING("Failed to rename account data file after login change, reverting changes");
			return false;
		}

		accountData->SetDataPath(std::move(newLoginPath));
		OnAccountActivity(*accountData, std::format("Account login is changed from {} to {}", oldLogin, newLogin));
		return true;
	}

	/**
	 * @brief Modify the password of an existing account if it was initialized, checking password requirements.
	 *
	 * @attention On failure, the account password is reverted to its original state.
	 *
	 * @param login The login of the account.
	 * @param newPassword The new password to set for the account.
	 * @param error Reference to a string to store error message if modification fails.
	 *
	 * @return True if the password modification is successful, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool ModifyAccountPassword(
		const std::string_view login, const std::string_view newPassword, std::string& error)
	{
		const auto loginHash{ std::hash<std::string_view>{}(login) };
		std::shared_ptr<AccountData> accountData;
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
			auto accountDataIt{ m_loginHashToData.find(loginHash) };
			if (accountDataIt == m_loginHashToData.end()) {
				return false;
			}

			accountData = accountDataIt->second;
		}

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
		if (!accountData->GetAccount().IsInitialized()) {
			return false;
		}

		if (!CheckPasswordRequirements(newPassword, error)) {
			return false;
		}

		std::array<uint8_t, PASSWORD_HASH_SIZE> oldPassword{};
		accountData->GetAccount().BackupPassword(oldPassword);
		accountData->GetAccount().SetPassword(newPassword);
		if (!accountData->Save()) [[unlikely]] {
			OnAccountActivity(*accountData, "Failed to save account data after password change, reverting changes");
			accountData->GetAccount().RestorePassword(oldPassword);
			return false;
		}

		OnAccountActivity(*accountData, "Account password is changed");
		return true;
	}

	/**
	 * @brief Modify the grade of an existing account.
	 *
	 * @attention On failure, the account grade is reverted to its original state.
	 *
	 * @param login The login of the account.
	 * @param newGrade The new grade to set for the account.
	 *
	 * @return True if the grade modification is successful, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool ModifyAccountGrade(const std::string_view login, const G newGrade)
	{
		const auto loginHash{ std::hash<std::string_view>{}(login) };
		std::shared_ptr<AccountData> accountData;
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
			auto accountDataIt{ m_loginHashToData.find(loginHash) };
			if (accountDataIt == m_loginHashToData.end()) {
				return false;
			}

			accountData = accountDataIt->second;
		}

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
		const auto oldGrade{ accountData->GetAccount().GetGrade() };
		if (oldGrade == newGrade) {
			OnAccountActivity(*accountData, "Account grade change requested, but grade is the same");
			return true;
		}

		accountData->GetAccount().SetGrade(newGrade);
		if (!accountData->Save()) [[unlikely]] {
			OnAccountActivity(*accountData, "Failed to save account data after grade change, reverting changes");
			accountData->GetAccount().SetGrade(oldGrade);
			return false;
		}

		OnAccountActivity(*accountData, std::format("Account grade is changed to {}", U(newGrade)));
		return true;
	}

	/**
	 * @brief Set the activation state of an existing account and close active connection if deactivating.
	 *
	 * @attention On failure, the account activation state is reverted to its original state.
	 *
	 * @param login The login of the account.
	 * @param isActivated True to activate the account, false to deactivate.
	 *
	 * @return True if the activation state modification is successful, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool SetAccountActivatedState(const std::string_view login, const bool isActivated)
	{
		const auto loginHash{ std::hash<std::string_view>{}(login) };
		std::shared_ptr<AccountData> accountData;
		{
			const Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
			auto accountDataIt{ m_loginHashToData.find(loginHash) };
			if (accountDataIt == m_loginHashToData.end()) {
				return false;
			}

			accountData = accountDataIt->second;
		}

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
		if (accountData->GetAccount().IsActive() == isActivated) {
			OnAccountActivity(*accountData, "Account activation state change requested, but state is the same");
			return true;
		}

		if (!isActivated && accountData->GetConnection() != -1) {
			Pthread::AtomicRWLock::ExitGuard<Pthread::write> connectionsGuard{ m_connectionsLock };
			m_activeConnectionToAccountData.erase(accountData->GetConnection());
			accountData->SetConnection(-1);
			OnAccountActivity(*accountData, "Account is going to be deactivated and logged out from active connection");
		}

		accountData->GetAccount().SetActivated(isActivated);
		if (!accountData->Save()) [[unlikely]] {
			OnAccountActivity(
				*accountData, "Failed to save account data after activation state change, reverting changes");
			accountData->GetAccount().SetActivated(!isActivated);
			return false;
		}

		OnAccountActivity(*accountData, std::format("Changing account activation state to {}", isActivated));
		return true;
	}

	/**
	 * @brief Authenticate a connection using the provided login and password. Only initialized and activated account
	 * can be authenticated. Multiply authentication is not allowed.
	 *
	 * @param connection Connection descriptor.
	 * @param login The login of the account.
	 * @param password The password of the account.
	 * @param error Reference to a string to store error message if authentication fails.
	 *
	 * @return True if authentication is successful, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool AuthenticateConnection(
		const int connection, const std::string_view login, const std::string_view password, std::string& error)
	{
		const auto loginHash{ std::hash<std::string_view>{}(login) };
		std::shared_ptr<AccountData> accountData;
		{
			Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
			auto accountDataIt{ m_loginHashToData.find(loginHash) };
			if (accountDataIt == m_loginHashToData.end()) {
				error = "Invalid login or password";
				return false;
			}

			accountData = accountDataIt->second;
		}

		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
		if (!accountData->GetAccount().IsInitialized()) {
			error = "Account is not valid";
			return false;
		}

		if (!accountData->GetAccount().IsActive()) {
			error = "Account is not activated";
			return false;
		}

		if (accountData->GetConnection() != -1) {
			error = "Multiply authentication is not allowed";
			return false;
		}

		if (!accountData->GetAccount().IsActive()) {
			error = "Account is not activated";
			return false;
		}

		if (const auto blockedTill{ accountData->GetAccount().GetBlockedTill() }; blockedTill > Timer{}) {
			error = std::format("Account is blocked till {}", blockedTill.ToString());
			return false;
		}

		if (!accountData->GetAccount().Authenticate(password, error)) {
			error = "Invalid login or password";
			return false;
		}

		accountData->SetConnection(connection);
		{
			Pthread::AtomicRWLock::ExitGuard<Pthread::write> connectionsGuard{ m_connectionsLock };
			m_activeConnectionToAccountData[connection] = accountData;
		}
		OnAccountActivity(*accountData, std::format("Account authenticated on connection {}", connection));

		return true;
	}

	/**
	 * @brief Logout the connection, disassociating it from any authenticated account.
	 *
	 * @param connection Connection descriptor.
	 */
	FORCE_INLINE void LogoutConnection(const int connection)
	{
		std::shared_ptr<AccountData> accountData;
		{
			Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_connectionsLock };
			auto it{ m_activeConnectionToAccountData.find(connection) };
			if (it == m_activeConnectionToAccountData.end()) {
				LOG_DEBUG_NEW("Connection {} is not authenticated, cannot logout", connection);
				return;
			}

			accountData = it->second;
			m_activeConnectionToAccountData.erase(it);
		}

		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
		accountData->SetConnection(-1);
		OnAccountActivity(*accountData, std::format("Account logged out from connection {}", connection));
	}

	/**
	 * @brief Check if the connection is authenticated.
	 *
	 * @param connection Connection descriptor.
	 *
	 * @return True if the connection is authenticated, false otherwise.
	 */
	FORCE_INLINE [[nodiscard]] bool IsConnectionAuthenticated(const int connection) const
	{
		std::shared_ptr<AccountData> accountData;
		{
			Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_connectionsLock };
			const auto it{ m_activeConnectionToAccountData.find(connection) };
			if (it == m_activeConnectionToAccountData.end()) {
				return false;
			}

			accountData = it->second;
		}

		accountData->UpdateLastActivity();
		return true;
	}

	FORCE_INLINE [[nodiscard]] bool IsAccessGranted(const int connection, const G requiredGrade) const
	{
		std::shared_ptr<AccountData> accountData;
		{
			Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_connectionsLock };
			auto it{ m_activeConnectionToAccountData.find(connection) };
			if (it == m_activeConnectionToAccountData.end()) {
				return false;
			}

			accountData = it->second;
		}

		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
		accountData->UpdateLastActivity();
		return accountData->GetAccount().GetGrade() >= requiredGrade;
	}

protected:
	/**
	 * @brief Save an account activity log entry to the account's data file.
	 *
	 * @param accountData The account data object.
	 * @param logEntry The log entry to save.
	 */
	FORCE_INLINE void SaveAccountLog(AccountData& accountData, const std::string_view logEntry)
	{
		if (!IO::SaveStr<true, 0640>(logEntry, accountData.GetDataPath().c_str())) [[unlikely]] {
			LOG_WARNING_NEW("Cannot save log to file: {}", accountData.GetDataPath());
		}
	}

	/**
	 * @brief Handle account activity by updating the last activity timestamp and saving a log entry.
	 *
	 * @param accountData The account data object.
	 * @param description Description of the activity.
	 */
	FORCE_INLINE void OnAccountActivity(AccountData& accountData, const std::string_view description)
	{
		accountData.UpdateLastActivity();
		LOG_DEBUG_NEW(
			"Account: {} activity updated, description: {}", accountData.GetAccount().GetLogin(), description);
		SaveAccountLog(accountData, description);
	}

private:
	// Timer::Event
	FORCE_INLINE void HandleEvent([[maybe_unused]] const Timer::Event& event) final
	{
		const Timer now{};
		std::vector<int> connectionsToLogout;
		{
			Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_connectionsLock };
			for (auto it{ m_activeConnectionToAccountData.begin() }; it != m_activeConnectionToAccountData.end();
				 it++) {
				if (it->second->GetLastActivity() + m_logoutTimeout < now) {
					connectionsToLogout.push_back(it->first);
				}
			}
		}

		for (const auto connection : connectionsToLogout) {
			LogoutConnection(connection);
		}
	}
};

} // namespace Base

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

namespace Base {

} // namespace Base

} // namespace Authorization

} // namespace MSAPI

#endif // MSAPI_AUTHORIZATION_INL