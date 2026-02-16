/**************************
 * @file        authorization.inl
 * @version     6.0
 * @date        2025-12-23
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
 *
 * @brief Generic authorization module implementation.
 *
 * @todo Add unit test to logout event and another non tested parts.
 * @todo Add tests with overriden module and account with custom data model.
 * @todo Support an ability to extend data model by adding static method, which will re-save account data with new
 * model.
 * @todo Use static inheritance technique to avoid virtual inheritance overhead.
 * @todo Extend ability to customize behavior of module, like:
 * - Time to check session expiration;
 * - Request limits to change account data;
 * - Request limits to authorize;
 * - Blocking time period;
 * - White and black lists for IP addresses and logins.
 * @todo Store an IP of connection, provide ability to trigger two-factor verification on unusual IP activity;
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

constexpr size_t MAX_LOGIN_SIZE = 47;
constexpr size_t SALT_SIZE = 16;
constexpr size_t PASSWORD_HASH_SIZE = 32;

template <typename T>
concept Gradable = std::is_enum_v<T> && sizeof(T) == 2;

/**
 * @brief Account class representing a user account, can be used as a base to extended functionality.
 * - Deactivated by default. Deactivated accounts cannot logon.
 * - Not initialized by default. Initialized on first password set. Uninitialized account cannot logon.
 * - Can be blocked until a specific time.
 * - Password is stored as SHA-256(salt + password), where salt is randomly generated on first password set.
 *
 * @tparam G Type of grade enumeration.
 */
template <Gradable G = Grade> class Account {
private:
	char m_login[MAX_LOGIN_SIZE + 1]{};
	uint8_t m_salt[SALT_SIZE]{};
	// Cache line 1
	Timer m_blockedTill{};
	uint8_t m_password[PASSWORD_HASH_SIZE]{};
	G m_grade{};
	bool m_isActivated{};
	bool m_isInitialized{};
	// int8_t padding[4]

public:
	/**
	 * @brief Default destructor.
	 */
	virtual ~Account() = default;

	/**
	 * @return Timer object representing the blocked till time.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] Timer GetBlockedTill() const noexcept;

	/**
	 * @return C-string representing the login of the account.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] const char* GetLogin() const noexcept;

	/**
	 * @return Grade of the account.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] G GetGrade() const noexcept;

	/**
	 * @return True if the account is activated, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool IsActive() const noexcept;

	/**
	 * @return True if the account is initialized, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool IsInitialized() const noexcept;

	/**
	 * @brief Verify that password is valid, check initialization and activation states for the account.
	 *
	 * @param password The password to verify.
	 * @param error Reference to a string to store error message if validation fails.
	 *
	 * @return True if logon is allowed, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool IsLogonAllowed(const std::string_view password, std::string& error) const noexcept;

	/**
	 * @brief Set a new login for the account.
	 *
	 * @param newLogin The new login to set.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void SetLogin(const std::string_view newLogin) noexcept;

	/**
	 * @brief Set a new password for the account. Initializes the account if not already initialized.
	 *
	 * @param newPassword The new password to set.
	 *
	 * @return True if the password was set successfully, false if password is the same as the current one.
	 *
	 * @test Has unit tests.
	 */
	[[nodiscard]] FORCE_INLINE bool SetPassword(const std::string_view newPassword) noexcept;

	/**
	 * @brief Backup the password hash into the provided buffer.
	 *
	 * @param buffer The buffer to store the password hash.
	 */
	FORCE_INLINE void BackupPassword(std::array<uint8_t, PASSWORD_HASH_SIZE>& buffer) const noexcept;

	/**
	 * @brief Restore the password hash from the provided buffer.
	 *
	 * @param buffer The buffer containing the password hash.
	 */
	FORCE_INLINE void RestorePassword(const std::array<uint8_t, PASSWORD_HASH_SIZE>& buffer) noexcept;

	/**
	 * @brief Set a new grade for the account.
	 *
	 * @param newGrade The new grade to set.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void SetGrade(const G newGrade) noexcept;

	/**
	 * @brief Activate or deactivate the account.
	 *
	 * @param isActivated True to activate the account, false to deactivate.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void SetActivated(const bool isActivated) noexcept;

	/**
	 * @brief Block the account till the specified time.
	 *
	 * @param blockedTill The time till which the account is blocked.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void SetBlockedTill(const Timer blockedTill) noexcept;

	/**
	 * @brief Initialize or deinitialize the account.
	 *
	 * @param isInitialized True to set the account as initialized, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void SetInitialized(const bool isInitialized) noexcept;
};

template <typename T>
concept Accountable = std::is_base_of_v<Account<>, T>;

/**
 * @brief Generic authorization module class, provides thread-safe account management and authentication. Can be used as
 * a base to extended functionality.
 * - Supports automatic logout of inactive accounts after a specified timeout.
 * - Customizable account login and password policies.
 * - Store account binary data and its history in {executable}/../data/accounts/ directory. Where each account is stored
 * in a separate file with filename as the account login. Directory and files rights are 0750 and 0640 respectively.
 * - Update account data file on each modification.
 * - Load all accounts from data directory on Start.
 * - Provide authentication to one connection per account.
 * - Grade based access control.
 *
 * @tparam A Type of account class.
 * @tparam G Type of grade.
 */
template <Accountable A = Account<Grade>, Gradable G = Grade> class Module : protected Timer::Event::IHandler {
public:
	using account_t = A;
	using grade_t = G;

private:
	/**
	 * @brief Contains account data along with additional control information.
	 */
	class AccountData {
	private:
		A m_account;
		std::string m_dataPath;
		std::unique_ptr<Pthread::AtomicRWLock> m_rwLock;
		int32_t m_connection{ -1 };
		Timer m_lastActivity{ 0 };

	public:
		/**
		 * @brief Construct a new AccountData object.
		 *
		 * @param account The account object.
		 * @param dataPath The file path where the account data is stored.
		 */
		FORCE_INLINE explicit AccountData(A&& account, std::string&& dataPath) noexcept;

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
		FORCE_INLINE AccountData(AccountData&& other) noexcept;

		/**
		 * @brief Move assignment operator for AccountData.
		 *
		 * @param other The other AccountData object to move from.
		 *
		 * @return Reference to this AccountData object.
		 */
		FORCE_INLINE AccountData& operator=(AccountData&& other) noexcept;

		/**
		 * @brief Destroy the AccountData object, acquiring a write lock to ensure all operations are completed.
		 *
		 * @test Has unit tests.
		 */
		FORCE_INLINE ~AccountData() noexcept;

		/**
		 * @return Reference to the account object.
		 *
		 * @test Has unit tests.
		 */
		FORCE_INLINE A& GetAccount() noexcept;

		/**
		 * @return Const reference to the file path where the account data is stored.
		 *
		 * @test Has unit tests.
		 */
		FORCE_INLINE const std::string& GetDataPath() const noexcept;

		/**
		 * @brief Set a new data path for the account data file.
		 *
		 * @param newDataPath The new file path.
		 *
		 * @test Has unit tests.
		 */
		FORCE_INLINE void SetDataPath(std::string&& newDataPath) noexcept;

		/**
		 * @brief Update the last activity timestamp to the current time.
		 *
		 * @param timer The new last activity timestamp.
		 *
		 * @test Has unit tests.
		 */
		FORCE_INLINE void UpdateLastActivity(Timer timer) noexcept;

		/**
		 * @return The last activity timestamp.
		 *
		 * @test Has unit tests.
		 */
		FORCE_INLINE Timer GetLastActivity() const noexcept;

		/**
		 * @return Reference to the read-write lock for this account data.
		 *
		 * @test Has unit tests.
		 */
		FORCE_INLINE Pthread::AtomicRWLock& GetRWLock() noexcept;

		/**
		 * @brief Save the account data to its associated file.
		 *
		 * @return True if the save operation was successful, false otherwise.
		 *
		 * @test Has unit tests.
		 */
		FORCE_INLINE [[nodiscard]] bool Save() const;

		/**
		 * @brief Set the connection associated with this account.
		 *
		 * @param connection The connection identifier.
		 */
		FORCE_INLINE void SetConnection(const int32_t connection) noexcept;

		/**
		 * @return The connection identifier associated with this account.
		 */
		FORCE_INLINE int32_t GetConnection() const noexcept;
	};

private:
	std::unordered_map<int32_t, std::shared_ptr<AccountData>> m_logonConnectionToAccountData;
	Pthread::AtomicRWLock m_connectionsLock;
	std::unordered_map<size_t, std::shared_ptr<AccountData>> m_loginHashToAccountData;
	Pthread::AtomicRWLock m_accountsLock;
	std::string m_dataPath;
	Timer::Event m_logoutEvent{ this };
	Timer::Duration m_logoutTimeout{ Timer::Duration::CreateHours(12) };
	Timer m_startTime{ 0 };
	bool m_isStarted{};

public:
	/**
	 * @brief Construct a new Module object.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE Module() noexcept = default;

	/**
	 * @brief Destroy the Module object.
	 */
	FORCE_INLINE ~Module() noexcept override = default;

	/**
	 * @brief Loading existing accounts from the {executable}/../data/accounts/ directory.
	 * - Uninitialized accounts are not loaded.
	 * - If the accounts data directory does not exist, it will be created with permissions 0750.
	 * - On failure to read any account data file, the starting will stop.
	 *
	 * @return True if starting is successful, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool Start();

	/**
	 * @brief Stop the module, stopping the logout event and acquiring write locks, logout all connections and set
	 * started flag to false, reset start time.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void Stop();

	/**
	 * @return True if the module is started, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool IsStarted() const noexcept;

	/**
	 * @brief Set the logout timeout duration. Accounts inactive for longer than this duration will logout
	 * automatically.
	 *
	 * @param duration The duration to set as the logout timeout.
	 */
	FORCE_INLINE void SetLogoutTimeout(const Timer::Duration duration);

	/**
	 * @return The current logout timeout duration.
	 */
	FORCE_INLINE [[nodiscard]] Timer::Duration GetLogoutTimeout() const noexcept;

	/**
	 * @brief Register a new account with the provided login and password, check their requirements. Newly registered
	 * accounts are deactivated by default.
	 *
	 * @param login The login for the new account.
	 * @param password The password for the new account.
	 * @param error Reference to a string to store error message if registration fails.
	 *
	 * @return True if registration is successful, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool RegisterAccount(
		const std::string_view login, const std::string_view password, std::string& error);

	/**
	 * @brief Delete the account with the specified login. Marks the account as uninitialized and deactivated, logouts
	 * any active connection.
	 *
	 * @param login The login of the account to delete.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void DeleteAccount(const std::string_view login);

	/**
	 * @brief Modify the login of an existing account, check login requirements and ensure uniqueness.
	 *
	 * @attention On failure, the account data is reverted to its original state.
	 *
	 * @param oldLogin The current login of the account.
	 * @param newLogin The new login to set for the account.
	 * @param error Reference to a string to store error message if modification fails.
	 *
	 * @return True if the login modification is successful, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool ModifyAccountLogin(
		const std::string_view oldLogin, const std::string_view newLogin, std::string& error);

	/**
	 * @brief Modify the password of an existing account if it was initialized, check password requirements.
	 *
	 * @attention On failure, the account password is reverted to its original state.
	 *
	 * @param login The login of the account.
	 * @param newPassword The new password to set for the account.
	 * @param error Reference to a string to store error message if modification fails.
	 *
	 * @return True if the password modification is successful, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool ModifyAccountPassword(
		const std::string_view login, const std::string_view newPassword, std::string& error);

	/**
	 * @brief Modify the grade of an existing account.
	 *
	 * @attention On failure, the account grade is reverted to its original state.
	 *
	 * @param login The login of the account.
	 * @param newGrade The new grade to set for the account.
	 *
	 * @return True if the grade modification is successful, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool ModifyAccountGrade(const std::string_view login, const G newGrade);

	/**
	 * @brief Set the activation state of an existing account and logout any active connection if deactivating.
	 *
	 * @attention On failure, the account activation state is reverted to its original state, but any logged-out
	 * connection is not restored.
	 *
	 * @param login The login of the account.
	 * @param isActivated True to activate the account, false to deactivate.
	 *
	 * @return True if the activation state modification is successful, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool SetAccountActivatedState(const std::string_view login, const bool isActivated);

	/**
	 * @brief Logon a connection using the provided login and password. Only initialized and activated account
	 * can logon. Multiple logon is not allowed.
	 *
	 * @param connection Connection descriptor.
	 * @param login The login of the account.
	 * @param password The password of the account.
	 * @param error Reference to a string to store error message if logon fails.
	 *
	 * @return True if logon is successful, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool LogonConnection(
		const int32_t connection, const std::string_view login, const std::string_view password, std::string& error);

	/**
	 * @brief Logout the connection, disassociating it from any logged-on account.
	 *
	 * @param connection Connection descriptor.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void LogoutConnection(const int32_t connection);

	/**
	 * @brief Check if the connection has the required access grade.
	 *
	 * @param connection Connection descriptor.
	 * @param requiredGrade The required grade for access.
	 *
	 * @return True if the connection has the required access grade, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool IsAccessGranted(const int32_t connection, const G requiredGrade);

	/**
	 * @return The number of registered accounts.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] size_t GetRegisteredAccountsSize() noexcept;

	/**
	 * @return The number of active logon connections.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] size_t GetLogonConnectionsSize() noexcept;

	/**
	 * @brief Block or unblock the account till the specified time. If block an active account, it logouts from its
	 * active connection. If unblock, blockedTill is set to zero timestamp. If already blocked, then blocked till time
	 * is updated. If already unblocked or blocked till the same time, then action fails.
	 *
	 * @attention On failure, the account blocked till time is reverted to its original state.
	 *
	 * @param login The login of the account.
	 * @param blockedTill The time till which the account is blocked. If less than or equal to current time, the
	 * account will be unblocked.
	 *
	 * @return True if the block/unblock operation is successful, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] bool BlockAccountTill(const std::string_view login, const Timer blockedTill);

protected:
	/**
	 * @brief Check if provided login meets the account login requirements. By default, checks for non-empty login and
	 * maximum length.
	 *
	 * @param login The login to check.
	 * @param error Reference to a string to store error message if requirements are not met.
	 *
	 * @return True if the login meets the requirements, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] virtual bool CheckLoginRequirements(
		const std::string_view login, std::string& error) const;

	/**
	 * @brief Check if provided password meets the account password requirements. By default, checks for length between
	 * 8 and 28 characters, and presence of at least one lowercase letter, one uppercase letter, one digit, and one
	 * special character.
	 *
	 * @param password The password to check.
	 * @param error Reference to a string to store error message if requirements are not met.
	 *
	 * @return True if the password meets the requirements, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE [[nodiscard]] virtual bool CheckPasswordRequirements(
		const std::string_view password, std::string& error) const;

	/**
	 * @brief Check if the login string is safe to use as a file path.
	 *
	 * @param login The login string to check.
	 * @param error Reference to a string to store error message if the login is not safe.
	 *
	 * @return True if the login is safe, false otherwise.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE bool CheckLoginAsPath(const std::string_view login, std::string& error) const noexcept;

	/**
	 * @brief Handle account activity by updating the last activity timestamp and saving a log entry.
	 *
	 * @param accountData The account data object.
	 * @param timestamp The timestamp of the activity.
	 * @param description Description of the activity.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void OnAccountActivity(AccountData& accountData, Timer timestamp, const std::string_view description);

private:
	// Timer::Event
	FORCE_INLINE void HandleEvent([[maybe_unused]] const Timer::Event& event) final;
};

} // namespace Base

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

namespace Base {

/*---------------------------------------------------------------------------------
Account
---------------------------------------------------------------------------------*/

template <Gradable G> FORCE_INLINE [[nodiscard]] Timer Account<G>::GetBlockedTill() const noexcept
{
	return m_blockedTill;
}

template <Gradable G> FORCE_INLINE [[nodiscard]] const char* Account<G>::GetLogin() const noexcept { return m_login; }

template <Gradable G> FORCE_INLINE [[nodiscard]] G Account<G>::GetGrade() const noexcept { return m_grade; }

template <Gradable G> FORCE_INLINE [[nodiscard]] bool Account<G>::IsActive() const noexcept { return m_isActivated; }

template <Gradable G> FORCE_INLINE [[nodiscard]] bool Account<G>::IsInitialized() const noexcept
{
	return m_isInitialized;
}

template <Gradable G>
FORCE_INLINE [[nodiscard]] bool Account<G>::IsLogonAllowed(
	const std::string_view password, std::string& error) const noexcept
{
	Sha256 hash;
	hash.Update(std::span<const uint8_t>{ m_salt, SALT_SIZE });
	hash.Update(std::span<const uint8_t>{ reinterpret_cast<const uint8_t*>(password.data()), password.size() });

	// Password verification uses memcmp which is not timing-safe and could be vulnerable to timing attacks
	if (memcmp(hash.Final<Sha256::doNotReset>().data(), m_password, PASSWORD_HASH_SIZE) != 0) {
		error = "Invalid login or password";
		return false;
	}

	if (!m_isInitialized) [[unlikely]] {
		error = "Account is not initialized";
		return false;
	}

	if (!m_isActivated) {
		error = "Account is not activated";
		return false;
	}

	if (m_blockedTill > Timer{}) {
		const Timer now{};
		if (now < m_blockedTill) {
			error = std::format("Account is blocked till {}", m_blockedTill.ToString());
			return false;
		}
	}

	return true;
}

template <Gradable G> FORCE_INLINE void Account<G>::SetLogin(const std::string_view newLogin) noexcept
{
	memcpy(m_login, newLogin.data(), newLogin.size());
	m_login[newLogin.size()] = '\0';
}

template <Gradable G>
[[nodiscard]] FORCE_INLINE bool Account<G>::SetPassword(const std::string_view newPassword) noexcept
{
	if (!m_isInitialized) {
		uint64_t randomValues[2];
		static_assert(SALT_SIZE == sizeof(randomValues));

		randomValues[0] = UINT64(m_blockedTill.GetNanoseconds()) << 44
			| (UINT64(reinterpret_cast<uintptr_t>(this)) << 20 & 0x00000FFF00000000)
			| (UINT64(m_blockedTill.GetSeconds()) & 0x00000000FFFFFFFF);
		const Timer now{};
		randomValues[1] = UINT64(now.GetNanoseconds()) << 44
			| (UINT64(reinterpret_cast<uintptr_t>(this)) << 32 & 0x00000FFF00000000)
			| (UINT64(now.GetSeconds()) & 0x00000000FFFFFFFF);

		memcpy(m_salt, randomValues, SALT_SIZE);

		m_isInitialized = true;
		m_blockedTill = Timer{ 0 };
	}

	Sha256 hash;
	hash.Update(std::span<const uint8_t>{ m_salt, SALT_SIZE });
	hash.Update(std::span<const uint8_t>{ reinterpret_cast<const uint8_t*>(newPassword.data()), newPassword.size() });
	const auto digits{ hash.Final<Sha256::doNotReset>() };

	if (memcmp(m_password, digits.data(), PASSWORD_HASH_SIZE) == 0) {
		return false;
	}

	memcpy(m_password, digits.data(), PASSWORD_HASH_SIZE);
	return true;
}

template <Gradable G>
FORCE_INLINE void Account<G>::BackupPassword(std::array<uint8_t, PASSWORD_HASH_SIZE>& buffer) const noexcept
{
	memcpy(buffer.data(), m_password, PASSWORD_HASH_SIZE);
}

template <Gradable G>
FORCE_INLINE void Account<G>::RestorePassword(const std::array<uint8_t, PASSWORD_HASH_SIZE>& buffer) noexcept
{
	memcpy(m_password, buffer.data(), PASSWORD_HASH_SIZE);
}

template <Gradable G> FORCE_INLINE void Account<G>::SetGrade(const G newGrade) noexcept { m_grade = newGrade; }

template <Gradable G> FORCE_INLINE void Account<G>::SetActivated(const bool isActivated) noexcept
{
	m_isActivated = isActivated;
}

template <Gradable G> FORCE_INLINE void Account<G>::SetBlockedTill(const Timer blockedTill) noexcept
{
	m_blockedTill = blockedTill;
}

template <Gradable G> FORCE_INLINE void Account<G>::SetInitialized(const bool isInitialized) noexcept
{
	m_isInitialized = isInitialized;
}

/*---------------------------------------------------------------------------------
AccountData
---------------------------------------------------------------------------------*/

template <Accountable A, Gradable G>
FORCE_INLINE Module<A, G>::AccountData::AccountData(A&& account, std::string&& dataPath) noexcept
	: m_account{ std::forward<A>(account) }
	, m_dataPath{ std::move(dataPath) }
	, m_rwLock{ std::make_unique<Pthread::AtomicRWLock>() }
{
}

template <Accountable A, Gradable G>
FORCE_INLINE Module<A, G>::AccountData::AccountData(AccountData&& other) noexcept
	: m_account{ std::move(other.m_account) }
	, m_dataPath{ std::move(other.m_dataPath) }
	, m_rwLock{ std::move(other.m_rwLock) }
	, m_connection{ other.m_connection }
	, m_lastActivity{ other.m_lastActivity }
{
}

template <Accountable A, Gradable G>
FORCE_INLINE Module<A, G>::AccountData& Module<A, G>::AccountData::operator=(AccountData&& other) noexcept
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

template <Accountable A, Gradable G> FORCE_INLINE Module<A, G>::AccountData::~AccountData() noexcept
{
	Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ *m_rwLock };
}

template <Accountable A, Gradable G> FORCE_INLINE A& Module<A, G>::AccountData::GetAccount() noexcept
{
	return m_account;
}

template <Accountable A, Gradable G>
FORCE_INLINE const std::string& Module<A, G>::AccountData::GetDataPath() const noexcept
{
	return m_dataPath;
}

template <Accountable A, Gradable G>
FORCE_INLINE void Module<A, G>::AccountData::SetDataPath(std::string&& newDataPath) noexcept
{
	m_dataPath = std::move(newDataPath);
}

template <Accountable A, Gradable G>
FORCE_INLINE void Module<A, G>::AccountData::UpdateLastActivity(Timer timer) noexcept
{
	m_lastActivity = std::move(timer);
}

template <Accountable A, Gradable G> FORCE_INLINE Timer Module<A, G>::AccountData::GetLastActivity() const noexcept
{
	return m_lastActivity;
}

template <Accountable A, Gradable G> FORCE_INLINE Pthread::AtomicRWLock& Module<A, G>::AccountData::GetRWLock() noexcept
{
	return *m_rwLock.get();
}

template <Accountable A, Gradable G> FORCE_INLINE [[nodiscard]] bool Module<A, G>::AccountData::Save() const
{
	return IO::SaveBinaryOnOffset<0640>(m_account, m_dataPath.c_str(), 0);
}

template <Accountable A, Gradable G>
FORCE_INLINE void Module<A, G>::AccountData::SetConnection(const int32_t connection) noexcept
{
	m_connection = connection;
}

template <Accountable A, Gradable G> FORCE_INLINE int32_t Module<A, G>::AccountData::GetConnection() const noexcept
{
	return m_connection;
}

/*---------------------------------------------------------------------------------
Module
---------------------------------------------------------------------------------*/

template <Accountable A, Gradable G> FORCE_INLINE [[nodiscard]] bool Module<A, G>::Start()
{
	if (m_isStarted) {
		LOG_DEBUG("Authorization module is already started, skipping");
		return true;
	}

	LOG_DEBUG("Starting authorization module");
	Pthread::AtomicRWLock::ExitGuard<Pthread::write> guardAccounts{ m_accountsLock };
	if (m_dataPath.empty()) {
		m_dataPath.resize(512);
		Helper::GetExecutableDir(m_dataPath);
		if (m_dataPath.empty()) [[unlikely]] {
			LOG_ERROR("Cannot get executable path");
			return false;
		}

		std::format_to(std::back_inserter(m_dataPath), "../data/accounts/");
	}

	if (IO::HasPath(m_dataPath.c_str())) {
		IO::Directory::ExitGuard guard{ m_dataPath.c_str() };
		if (guard.value == nullptr) [[unlikely]] {
			LOG_ERROR_NEW(
				"Cannot open accounts data directory: {}. Error №{}: {}", m_dataPath, errno, std::strerror(errno));
			return false;
		}

		std::vector<std::string> accounts;
		if (!IO::List<IO::FileType::Regular>(accounts, guard.value)) [[unlikely]] {
			return false;
		}

		A object;
		for (const auto& account : accounts) {
			std::string accountFilePath{ m_dataPath + account };
			if (!IO::ReadBinary(&object, accountFilePath.c_str())) [[unlikely]] {
				return false;
			}

			if (!object.IsInitialized()) {
				continue;
			}

			m_loginHashToAccountData.emplace(std::hash<std::string_view>{}(object.GetLogin()),
				std::make_shared<AccountData>(std::move(object), std::move(accountFilePath)));
		}

		LOG_DEBUG_NEW("Loaded {} accounts from file: {}", m_loginHashToAccountData.size(), m_dataPath);
	}
	else {
		if (!IO::CreateDir<0750>(m_dataPath.c_str())) [[unlikely]] {
			return false;
		}

		LOG_DEBUG_NEW("Accounts data path does not exist: {}, starting with zero accounts", m_dataPath);
	}

	m_logoutEvent.Start(m_logoutTimeout.GetSeconds(), 60);
	m_isStarted = true;
	m_startTime = Timer{};
	LOG_DEBUG("Authorization module started");

	return true;
}

template <Accountable A, Gradable G> FORCE_INLINE void Module<A, G>::Stop()
{
	if (!m_isStarted) {
		LOG_DEBUG("Authorization module is not started, skipping stop");
		return;
	}

	LOG_DEBUG("Stopping authorization module");
	m_logoutEvent.Stop();

	Pthread::AtomicRWLock::ExitGuard<Pthread::write> guardAccounts{ m_accountsLock };
	Pthread::AtomicRWLock::ExitGuard<Pthread::write> guardConnections{ m_connectionsLock };
	const Timer timestamp{};

	for (auto& [connection, accountData] : m_logonConnectionToAccountData) {
		OnAccountActivity(*accountData, timestamp,
			std::format("Logout due to module stop at {}, connection {}", timestamp.ToString(), connection));
		accountData.reset();
	}
	m_logonConnectionToAccountData.clear();

	for (auto& [hash, accountData] : m_loginHashToAccountData) {
		accountData.reset();
	}
	m_loginHashToAccountData.clear();

	m_isStarted = false;
	m_startTime = Timer{ 0 };
	LOG_DEBUG("Authorization module stopped");
}

template <Accountable A, Gradable G> FORCE_INLINE [[nodiscard]] bool Module<A, G>::IsStarted() const noexcept
{
	return m_isStarted;
}

template <Accountable A, Gradable G> FORCE_INLINE void Module<A, G>::SetLogoutTimeout(const Timer::Duration duration)
{
	if (duration.GetSeconds() <= 0) {
		LOG_WARNING("Logout timeout cannot be zero or negative, ignoring");
		return;
	}

	m_logoutEvent.Stop();
	const auto now{ Timer{} };
	if (m_startTime + m_logoutTimeout < now) {
		m_logoutEvent.Start(60, 60, true);
	}
	else {
		const auto toFirstCheck{ Timer::Duration{ now - (m_startTime + m_logoutTimeout) }.GetSeconds() };
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

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] Timer::Duration Module<A, G>::GetLogoutTimeout() const noexcept
{
	return m_logoutTimeout;
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::RegisterAccount(
	const std::string_view login, const std::string_view password, std::string& error)
{
	if (!CheckLoginAsPath(login, error)) {
		return false;
	}

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
		if (m_loginHashToAccountData.find(loginHash) != m_loginHashToAccountData.end()) {
			error = "Account with this login already exists";
			return false;
		}

		A newAccount;
		newAccount.SetLogin(login);
		loginHashToDataIt
			= m_loginHashToAccountData
				  .emplace(loginHash,
					  std::make_shared<AccountData>(std::move(newAccount), m_dataPath + std::string{ login }))
				  .first;
	}

	auto& accountData{ loginHashToDataIt->second };
	auto& accountDataLock{ accountData->GetRWLock() };
	accountDataLock.WriteLock();
	const Timer timestamp{};
	(void)accountData->GetAccount().SetPassword(password);

	if (!accountData->Save()) [[unlikely]] {
		error = "Account registration failed";
		accountDataLock.WriteUnlock();
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
		m_loginHashToAccountData.erase(loginHashToDataIt);
		return false;
	}

	OnAccountActivity(*accountData, timestamp, std::format("Registered at {}", timestamp.ToString()));
	accountDataLock.WriteUnlock();
	return true;
}

template <Accountable A, Gradable G> FORCE_INLINE void Module<A, G>::DeleteAccount(const std::string_view login)
{
	const auto loginHash{ std::hash<std::string_view>{}(login) };
	std::shared_ptr<AccountData> accountData;
	{
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
		auto accountDataIt{ m_loginHashToAccountData.find(loginHash) };
		if (accountDataIt == m_loginHashToAccountData.end()) {
			LOG_DEBUG_NEW("Cannot find account with login: {}", login);
			return;
		}

		accountData = accountDataIt->second;
		m_loginHashToAccountData.erase(accountDataIt);
	}

	const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
	const Timer timestamp{};

	if (const auto connection{ accountData->GetConnection() }; connection != -1) {
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> connectionsGuard{ m_connectionsLock };
		m_logonConnectionToAccountData.erase(connection);
		accountData->SetConnection(-1);
		OnAccountActivity(*accountData, timestamp,
			std::format("Logout due to deletion at {}, connection {}", timestamp.ToString(), connection));
	}

	accountData->GetAccount().SetInitialized(false);
	accountData->GetAccount().SetActivated(false);
	OnAccountActivity(
		*accountData, timestamp, std::format("Marked as uninitialized and deactivated at {}", timestamp.ToString()));
	(void)accountData->Save();
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::ModifyAccountLogin(
	const std::string_view oldLogin, const std::string_view newLogin, std::string& error)
{
	if (!CheckLoginAsPath(newLogin, error)) {
		return false;
	}

	if (!CheckLoginRequirements(newLogin, error)) {
		return false;
	}

	const auto oldLoginHash{ std::hash<std::string_view>{}(oldLogin) };
	const auto newLoginHash{ std::hash<std::string_view>{}(newLogin) };
	std::shared_ptr<AccountData> accountData;
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
		auto oldAccountDataIt{ m_loginHashToAccountData.find(oldLoginHash) };
		if (oldAccountDataIt == m_loginHashToAccountData.end()) {
			LOG_DEBUG_NEW("Cannot find account with login {}", oldLogin);
			return false;
		}

		if (m_loginHashToAccountData.find(newLoginHash) != m_loginHashToAccountData.end()) {
			LOG_DEBUG_NEW("Account with login {} already exists", newLogin);
			return false;
		}

		accountData = oldAccountDataIt->second;
		m_loginHashToAccountData.emplace(newLoginHash, accountData);
		m_loginHashToAccountData.erase(oldAccountDataIt);
	}

	const Pthread::AtomicRWLock::ExitGuard<Pthread::write> accountGuard{ accountData->GetRWLock() };
	const Timer timestamp{};
	accountData->GetAccount().SetLogin(newLogin);
	if (!accountData->Save()) [[unlikely]] {
		error = "Account modification failed";
		accountData->GetAccount().SetLogin(oldLogin);

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
		m_loginHashToAccountData.emplace(oldLoginHash, accountData);
		m_loginHashToAccountData.erase(newLoginHash);

		OnAccountActivity(*accountData, timestamp,
			std::format("Failed to save data file after login change from {} to {} at {}, reverting changes", newLogin,
				oldLogin, timestamp.ToString()));
		return false;
	}

	std::string newLoginPath{ m_dataPath + std::string{ newLogin } };
	if (!IO::Rename(accountData->GetDataPath().c_str(), newLoginPath.c_str())) [[unlikely]] {
		accountData->GetAccount().SetLogin(oldLogin);

		const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_accountsLock };
		m_loginHashToAccountData.emplace(oldLoginHash, accountData);
		m_loginHashToAccountData.erase(newLoginHash);

		OnAccountActivity(*accountData, timestamp,
			std::format("Failed to rename data file after login change from {} to {} at {}, reverting changes",
				newLogin, oldLogin, timestamp.ToString()));
		return false;
	}

	accountData->SetDataPath(std::move(newLoginPath));
	OnAccountActivity(*accountData, timestamp,
		std::format("Login is changed from {} to {} at {}", oldLogin, newLogin, timestamp.ToString()));
	return true;
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::ModifyAccountPassword(
	const std::string_view login, const std::string_view newPassword, std::string& error)
{
	const auto loginHash{ std::hash<std::string_view>{}(login) };
	std::shared_ptr<AccountData> accountData;
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
		auto accountDataIt{ m_loginHashToAccountData.find(loginHash) };
		if (accountDataIt == m_loginHashToAccountData.end()) {
			return false;
		}

		accountData = accountDataIt->second;
	}

	const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
	const Timer timestamp{};
	if (!accountData->GetAccount().IsInitialized()) [[unlikely]] {
		error = "Account is not initialized";
		OnAccountActivity(*accountData, timestamp,
			std::format("Failed to change password of uninitialized account at {}", timestamp.ToString()));
		return false;
	}

	if (!CheckPasswordRequirements(newPassword, error)) {
		OnAccountActivity(*accountData, timestamp,
			std::format({ "Failed to change password at {}, reason: {}" }, timestamp.ToString(), error));
		return false;
	}

	std::array<uint8_t, PASSWORD_HASH_SIZE> oldPassword{};
	accountData->GetAccount().BackupPassword(oldPassword);
	if (!accountData->GetAccount().SetPassword(newPassword)) {
		error = "New password is the same as the current one";
		OnAccountActivity(*accountData, timestamp,
			std::format("Failed to change password at {} to the same one", timestamp.ToString()));
		return false;
	}

	if (!accountData->Save()) [[unlikely]] {
		error = "Account modification failed";
		OnAccountActivity(*accountData, timestamp,
			std::format(
				"Failed to save data file after password change at {}, reverting changes", timestamp.ToString()));
		accountData->GetAccount().RestorePassword(oldPassword);
		return false;
	}

	OnAccountActivity(*accountData, timestamp, std::format("Password is changed at {}", timestamp.ToString()));
	return true;
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::ModifyAccountGrade(const std::string_view login, const G newGrade)
{
	const auto loginHash{ std::hash<std::string_view>{}(login) };
	std::shared_ptr<AccountData> accountData;
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
		auto accountDataIt{ m_loginHashToAccountData.find(loginHash) };
		if (accountDataIt == m_loginHashToAccountData.end()) {
			return false;
		}

		accountData = accountDataIt->second;
	}

	const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
	const Timer timestamp{};
	const auto oldGrade{ accountData->GetAccount().GetGrade() };
	if (oldGrade == newGrade) {
		OnAccountActivity(
			*accountData, timestamp, std::format("Failed to change grade at {} to the same one", timestamp.ToString()));
		return false;
	}

	accountData->GetAccount().SetGrade(newGrade);
	if (!accountData->Save()) [[unlikely]] {
		OnAccountActivity(*accountData, timestamp,
			std::format("Failed to save data file after grade change at {}, reverting changes", timestamp.ToString()));
		accountData->GetAccount().SetGrade(oldGrade);
		return false;
	}

	OnAccountActivity(
		*accountData, timestamp, std::format("Grade is changed to {} at {}", U(newGrade), timestamp.ToString()));
	return true;
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::SetAccountActivatedState(
	const std::string_view login, const bool isActivated)
{
	const auto loginHash{ std::hash<std::string_view>{}(login) };
	std::shared_ptr<AccountData> accountData;
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
		auto accountDataIt{ m_loginHashToAccountData.find(loginHash) };
		if (accountDataIt == m_loginHashToAccountData.end()) {
			return false;
		}

		accountData = accountDataIt->second;
	}

	const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
	const Timer timestamp{};
	if (accountData->GetAccount().IsActive() == isActivated) {
		OnAccountActivity(*accountData, timestamp,
			std::format("Failed to change activation state at {} to the same one", timestamp.ToString()));
		return false;
	}

	if (const auto connection{ accountData->GetConnection() }; !isActivated && connection != -1) {
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> connectionsGuard{ m_connectionsLock };
		m_logonConnectionToAccountData.erase(connection);
		accountData->SetConnection(-1);
		OnAccountActivity(*accountData, timestamp,
			std::format("Logout due to deactivation at {}, connection {}", timestamp.ToString(), connection));
	}

	accountData->GetAccount().SetActivated(isActivated);
	if (!accountData->Save()) [[unlikely]] {
		OnAccountActivity(*accountData, timestamp,
			std::format("Failed to save data file after activation state change at {}, reverting changes",
				timestamp.ToString()));
		accountData->GetAccount().SetActivated(!isActivated);
		return false;
	}

	OnAccountActivity(*accountData, timestamp,
		std::format("Activation state is changed to {} at {}", isActivated, timestamp.ToString()));
	return true;
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::LogonConnection(
	const int32_t connection, const std::string_view login, const std::string_view password, std::string& error)
{
	const auto loginHash{ std::hash<std::string_view>{}(login) };
	std::shared_ptr<AccountData> accountData;
	{
		Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
		auto accountDataIt{ m_loginHashToAccountData.find(loginHash) };
		if (accountDataIt == m_loginHashToAccountData.end()) {
			error = "Invalid login or password";
			return false;
		}

		accountData = accountDataIt->second;
	}

	Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
	const Timer timestamp{};
	if (!accountData->GetAccount().IsLogonAllowed(password, error)) {
		OnAccountActivity(*accountData, timestamp,
			std::format(
				"Failed logon attempt at {}, connection {}, reason: {}", timestamp.ToString(), connection, error));
		return false;
	}

	const auto actualConnection{ accountData->GetConnection() };
	if (actualConnection == connection) {
		OnAccountActivity(*accountData, timestamp,
			std::format(
				"Failed logon attempt at {} to already logged-on connection {}", timestamp.ToString(), connection));
		return false;
	}

	if (actualConnection != -1) {
		error = "Multiple logon is not allowed";
		OnAccountActivity(*accountData, timestamp,
			std::format(
				"Multiple logon is not allowed, attempting connection {} at {}", connection, timestamp.ToString()));
		return false;
	}

	{
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> connectionsGuard{ m_connectionsLock };
		if (m_logonConnectionToAccountData.find(connection) != m_logonConnectionToAccountData.end()) {
			error = "Connection is already logged-on with another account";
			OnAccountActivity(*accountData, timestamp,
				std::format("Failed logon attempt at {} to already logged-on by another account connection {}",
					timestamp.ToString(), connection));
			return false;
		}
		accountData->SetConnection(connection);
		m_logonConnectionToAccountData[connection] = accountData;
	}
	OnAccountActivity(
		*accountData, timestamp, std::format("Logon at {}, connection {}", timestamp.ToString(), connection));

	return true;
}

template <Accountable A, Gradable G> FORCE_INLINE void Module<A, G>::LogoutConnection(const int32_t connection)
{
	std::shared_ptr<AccountData> accountData;
	{
		Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_connectionsLock };
		auto it{ m_logonConnectionToAccountData.find(connection) };
		if (it == m_logonConnectionToAccountData.end()) {
			LOG_DEBUG_NEW("Connection {} is not logged-on, cannot logout", connection);
			return;
		}

		accountData = it->second;
		m_logonConnectionToAccountData.erase(it);
	}

	Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
	const Timer timestamp{};
	accountData->SetConnection(-1);
	OnAccountActivity(
		*accountData, timestamp, std::format("Logout at {}, connection {}", timestamp.ToString(), connection));
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::IsAccessGranted(const int32_t connection, const G requiredGrade)
{
	std::shared_ptr<AccountData> accountData;
	{
		Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_connectionsLock };
		auto it{ m_logonConnectionToAccountData.find(connection) };
		if (it == m_logonConnectionToAccountData.end()) {
			return false;
		}

		accountData = it->second;
	}

	Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
	const Timer timestamp{};
	// Checking of account states is not needed here, as only logged-on accounts are stored in
	// m_logonConnectionToAccountData
	const bool isAccessGranted{ accountData->GetAccount().GetGrade() >= requiredGrade };
	OnAccountActivity(*accountData, timestamp,
		std::format(
			"Access check for grade {} at {}, result: {}", U(requiredGrade), timestamp.ToString(), isAccessGranted));
	return isAccessGranted;
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] size_t Module<A, G>::GetRegisteredAccountsSize() noexcept
{
	Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
	return m_loginHashToAccountData.size();
}

template <Accountable A, Gradable G> FORCE_INLINE [[nodiscard]] size_t Module<A, G>::GetLogonConnectionsSize() noexcept
{
	Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_connectionsLock };
	return m_logonConnectionToAccountData.size();
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::BlockAccountTill(const std::string_view login, const Timer blockedTill)
{
	const auto now{ Timer{} };
	const auto loginHash{ std::hash<std::string_view>{}(login) };
	std::shared_ptr<AccountData> accountData;
	{
		const Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_accountsLock };
		auto accountDataIt{ m_loginHashToAccountData.find(loginHash) };
		if (accountDataIt == m_loginHashToAccountData.end()) {
			return false;
		}

		accountData = accountDataIt->second;
	}

	const bool block{ blockedTill > now };

	const Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ accountData->GetRWLock() };
	const Timer timestamp{};
	auto& account{ accountData->GetAccount() };
	const auto oldBlockedTill{ account.GetBlockedTill() };
	if (block) {

#define TMP_MSAPI_AUTHORIZATION_SET_BLOCKED_TILL_AND_RETURN(log, blockTill)                                            \
	account.SetBlockedTill(blockTill);                                                                                 \
	if (!accountData->Save()) [[unlikely]] {                                                                           \
		OnAccountActivity(*accountData, timestamp,                                                                     \
			std::format("Failed to save data file after blocking at {}, reverting changes", timestamp.ToString()));    \
		account.SetBlockedTill(oldBlockedTill);                                                                        \
		return false;                                                                                                  \
	}                                                                                                                  \
	OnAccountActivity(*accountData, timestamp, log);                                                                   \
	return true;

		if (oldBlockedTill <= now) {
			if (const auto connection{ accountData->GetConnection() }; connection != -1) {
				Pthread::AtomicRWLock::ExitGuard<Pthread::write> guard{ m_connectionsLock };
				if (auto it{ m_logonConnectionToAccountData.find(connection) };
					it != m_logonConnectionToAccountData.end()) {
					m_logonConnectionToAccountData.erase(it);
					accountData->SetConnection(-1);
					OnAccountActivity(*accountData, timestamp,
						std::format("Logout due to blocking at {}, connection {}", timestamp.ToString(), connection));
				}
			}

			TMP_MSAPI_AUTHORIZATION_SET_BLOCKED_TILL_AND_RETURN(
				std::format("Blocked till {} at {}", blockedTill.ToString(), timestamp.ToString()), blockedTill);
		}

		if (oldBlockedTill > blockedTill) {
			TMP_MSAPI_AUTHORIZATION_SET_BLOCKED_TILL_AND_RETURN(
				std::format("Decrease blocked till {} at {}", blockedTill.ToString(), timestamp.ToString()),
				blockedTill);
		}

		if (oldBlockedTill < blockedTill) {
			TMP_MSAPI_AUTHORIZATION_SET_BLOCKED_TILL_AND_RETURN(
				std::format("Increase blocked till {} at {}", blockedTill.ToString(), timestamp.ToString()),
				blockedTill);
		}

		OnAccountActivity(*accountData, timestamp,
			std::format("Failed to block at {} to the same time {}", timestamp.ToString(), blockedTill.ToString()));
		return false;
	}

	if (oldBlockedTill <= now) {
		OnAccountActivity(
			*accountData, timestamp, std::format("Failed to unblock at {}, not blocked", timestamp.ToString()));
		return false;
	}

	TMP_MSAPI_AUTHORIZATION_SET_BLOCKED_TILL_AND_RETURN(
		std::format("Unblocked at {}", timestamp.ToString()), Timer{ 0 });
#undef TMP_MSAPI_AUTHORIZATION_SET_BLOCKED_TILL_AND_RETURN
}

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::CheckLoginRequirements(
	const std::string_view login, std::string& error) const
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

template <Accountable A, Gradable G>
FORCE_INLINE [[nodiscard]] bool Module<A, G>::CheckPasswordRequirements(
	const std::string_view password, std::string& error) const
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
		if (std::isdigit(ch)) {
			hasDigit = true;
		}
		else if (std::isupper(ch)) {
			hasUpper = true;
		}
		else if (std::islower(ch)) {
			hasLower = true;
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
			std::format_to(std::back_inserter(error), ", at least one uppercase letter");
		}
		else {
			error = "Password must contain at least one uppercase letter";
			empty = false;
		}
	}

	if (!hasDigit) {
		if (!empty) {
			std::format_to(std::back_inserter(error), ", at least one digit");
		}
		else {
			error = "Password must contain at least one digit";
			empty = false;
		}
	}

	if (!hasSpecial) {
		if (!empty) {
			std::format_to(std::back_inserter(error), ", at least one special character");
		}
		else {
			error = "Password must contain at least one special character";
		}
	}

	return false;
}

template <Accountable A, Gradable G>
FORCE_INLINE bool Module<A, G>::CheckLoginAsPath(const std::string_view login, std::string& error) const noexcept
{
	if (login == "." || login == "..") {
		error = "Invalid login";
		return false;
	}

	if (login.find_first_of("/\\ \n\t") != std::string_view::npos) {
		error = "Login contains invalid characters";
		return false;
	}

	return true;
}

template <Accountable A, Gradable G>
FORCE_INLINE void Module<A, G>::OnAccountActivity(
	AccountData& accountData, Timer timestamp, const std::string_view description)
{
	accountData.UpdateLastActivity(std::move(timestamp));
	LOG_DEBUG_NEW("Account: {} activity updated, description: {}", accountData.GetAccount().GetLogin(), description);
	(void)IO::SaveStr<true, 0640>(description, accountData.GetDataPath().c_str());
}

template <Accountable A, Gradable G>
FORCE_INLINE void Module<A, G>::HandleEvent([[maybe_unused]] const Timer::Event& event)
{
	const Timer now{};
	std::vector<int32_t> connectionsToLogout;
	{
		Pthread::AtomicRWLock::ExitGuard<Pthread::read> guard{ m_connectionsLock };
		for (auto it{ m_logonConnectionToAccountData.begin() }; it != m_logonConnectionToAccountData.end(); it++) {
			if (it->second->GetLastActivity() + m_logoutTimeout < now) {
				connectionsToLogout.push_back(it->first);
			}
		}
	}

	for (const auto connection : connectionsToLogout) {
		LogoutConnection(connection);
	}
}

} // namespace Base

} // namespace Authorization

} // namespace MSAPI

#endif // MSAPI_AUTHORIZATION_INL