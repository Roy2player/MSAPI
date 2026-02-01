/**************************
 * @file        authorization.inl
 * @version     6.0
 * @date        2025-12-26
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
 * @brief Unit test for generic authorization module.
 *
 * 1. Check default Account class size;
 * 2. Start default Authorization module and check accounts data path creation, stop it and start again;
 * 3. Check account registration with various login and password values and check their default restrictions; check
 * account logs after registration; check that connections (they are not even internally assigned) are not logged-on and
 * access is not granted right after registration; check that duplicate registrations are not allowed;
 * 4. Check duplicate account registrations are not allowed;
 * 5. Check saved account binary data after registration;
 * 6. Check access (* group of tests) for all accounts (no logon connections);
 * 7. Delete some accounts (no logon connections);
 * 8. Logon connections;
 * 9. Check access (* group of tests) for all accounts (has logon connections);
 * 10. Try to logon already logged-on connections;
 * 11. Logout some connections, try to logon with already logged-on connections and logon back. Delete logged-on
 * connections, then try to access and logon logged-out connections on another accounts and deactivate them after that,
 * then finally delete;
 * 12. Try to set blocked till to some accounts (future/same time again/forward/back), check logs and saved binary data;
 * 13. Stop module and check saved accounts;
 * 14. Start module and check state of loaded accounts and check missed ones;
 * 15. Check access (* group of tests) for all accounts (has logon connections).
 *
 * 1*. Check access;
 * 2*. Modify account grade, check logs and saved binary data;
 * 3*. Check access with new grade;
 * 4*. Try to set blocked till (future/same time again/forward/back), check logs and saved binary data;
 * 5*. Try to logon with both invalid and valid passwords, check binary data and logs when they are expected to be
 * written; activate account, even if it is already activated; 6*. Check access when blocked; 7*. Modify account grade;
 * 8*. Check access with new grade;
 * 9*. Unblock (past/empty timestamps and empty again);
 * 10*. Check access when unblocked;
 * 11*. Modify account grade;
 * 12*. Check access with new grade;
 * 13*. Modify passwords (invalid/same/valid);
 * 14*. Modify logins (valid/invalid);
 * 15*. Deactivate;
 * 16*. Check access when deactivated;
 * 17*. Activate back;
 * 18*. Check access.
 */

#ifndef MSAPI_UNIT_TEST_AUTHORIZATION_INL
#define MSAPI_UNIT_TEST_AUTHORIZATION_INL

#include "../../../../library/source/server/authorization.inl"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace UnitTest {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for Authorization module.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool Authorization();

/**
 * @brief Structure for holding account test data.
 */
struct AccountTestData {
	size_t logsCount{};
	Timer blockedTill{ 0 };
	Timer lastActivity{ 0 };
	std::string login;
	std::string password;
	std::string expectedError;
	int32_t connection;
	int16_t grade{};
	bool shouldRegister;
	bool isActivated{};
	bool isLoggedOn{};
	bool isDeleted{};
	bool reLogon{};

	/**
	 * @brief Construct a new AccountTestData object. Connection IDs start from 10 and increment for each new instance.
	 *
	 * @param login The account login.
	 * @param password The account password.
	 * @param expectedError The expected error message (if any).
	 * @param shouldRegister True if the account should be registered successfully, false otherwise.
	 */
	FORCE_INLINE AccountTestData(
		std::string login, std::string password, std::string expectedError, bool shouldRegister) noexcept;

private:
	static inline int32_t connectionsCounter = 10;
};

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

AccountTestData::AccountTestData(
	std::string login, std::string password, std::string expectedError, const bool shouldRegister) noexcept
	: login{ std::move(login) }
	, password{ std::move(password) }
	, expectedError{ std::move(expectedError) }
	, connection{ connectionsCounter++ }
	, shouldRegister{ shouldRegister }
{
}

bool Authorization()
{
	LOG_INFO_UNITTEST("MSAPI Authorization");
	MSAPI::Test t;

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	if (path.empty()) [[unlikely]] {
		LOG_ERROR("Cannot get executable path");
		return false;
	}

	path += "../data";
	const std::string_view pathV{ path };
	struct Cleaner {
		const std::string_view path;

		FORCE_INLINE Cleaner(const std::string_view path) noexcept
			: path{ path }
		{
		}

		FORCE_INLINE ~Cleaner() noexcept
		{
			if (!path.empty() && IO::HasPath(path)) {
				if (!IO::Remove(path)) {
					LOG_ERROR_NEW("Cannot remove test dir: {}, clean it before next test execution", path);
				}
			}
		}
	} cleaner{ pathV };

	constexpr std::string_view timestampPattern{ "XXXX-XX-XX XX:XX:XX.XXXXXXXXX" };

	// 1. Check default Account class size
	constexpr auto accountSize{ sizeof(Authorization::Base::Account<Authorization::Base::Grade>) };
	static_assert(accountSize == 120, "Unexpected Account size");
	static_assert(accountSize % 8 == 0, "Account size is not aligned to 8 bytes");

	// 2. Start default Authorization module and check accounts data path creation, stop it and start again
	Authorization::Base::Module mod;
	using G = decltype(mod)::grade_t;
	using A = decltype(mod)::account_t;
	using U = std::underlying_type_t<G>;
	RETURN_IF_FALSE(t.Assert(mod.Start(), true, "Start module"));
	RETURN_IF_FALSE(t.Assert(mod.Start(), true, "Re-start module"));
	mod.Stop();
	RETURN_IF_FALSE(t.Assert(mod.IsStarted(), false, "Module is not started after stop"));
	mod.Stop();
	RETURN_IF_FALSE(t.Assert(mod.Start(), true, "Re-start module"));

	RETURN_IF_FALSE(t.Assert(mod.IsStarted(), true, "Start module"));
	std::string accountsDataPath{ path + "/accounts/" };
	RETURN_IF_FALSE(
		t.Assert(IO::HasPath(accountsDataPath.c_str()), true, "Accounts data path exists after module start"));

	std::vector<std::string> accounts;
	size_t registeredAccounts{};
	const auto checkAccountsSize{ [&t, &accounts, &registeredAccounts, &accountsDataPath, &mod]() {
		accounts.clear();
		RETURN_IF_FALSE(t.Assert(
			IO::List<IO::FileType::Regular>(accounts, accountsDataPath.c_str()), true, "List accounts data directory"));
		RETURN_IF_FALSE(t.Assert(
			accounts.size(), registeredAccounts, "Expected number of account data files in accounts data directory"));
		RETURN_IF_FALSE(t.Assert(
			mod.GetRegisteredAccountsSize(), registeredAccounts, "Expected number of registered accounts in module"));

		return true;
	} };

	RETURN_IF_FALSE(checkAccountsSize());

	size_t activeConnections{};
	RETURN_IF_FALSE(
		t.Assert(mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

	struct InvalidTestData {
		std::string value;
		std::string expectedError;
	};

	std::vector<InvalidTestData> invalidLogins{ { ".", "Invalid login" }, { "..", "Invalid login" },
		{ " ", "Login contains invalid characters" }, { "/", "Login contains invalid characters" },
		{ "\\", "Login contains invalid characters" }, { "\n", "Login contains invalid characters" },
		{ "\t", "Login contains invalid characters" },
		{ "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "Login size cannot be greater than 47 characters" },
		{ "", "Login cannot be empty" } };

	std::vector<InvalidTestData> invalidPasswords{ { "pa2swoR", "Password size cannot be less than 8 characters" },
		{ "pa2swo!Daaaaaaaaaaaaaaaaaaaaa", "Password size cannot be greater than 28 characters" },
		{ "pa2swo!d", "Password must contain at least one uppercase letter" },
		{ "PA2SWO!D", "Password must contain at least one lowercase letter" },
		{ "passwo!D", "Password must contain at least one digit" },
		{ "pa2swodD", "Password must contain at least one special character" },
		{ "aaaaaaaa",
			"Password must contain at least one uppercase letter, at least one digit, at least one special "
			"character" },
		{ "AAAAAAAA",
			"Password must contain at least one lowercase letter, at least one digit, at least one special "
			"character" } };

	std::vector<AccountTestData> testAccounts{
		{ "...", "pa2swoR", "Password size cannot be less than 8 characters", false },
		{ "'", "pa2swo!Daaaaaaaaaaaaaaaaaaaaa", "Password size cannot be greater than 28 characters", false },
		{ "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "pa2swo!d",
			"Password must contain at least one uppercase letter", false },
	};

	for (const auto& data : invalidLogins) {
		testAccounts.emplace_back(AccountTestData{ data.value, "pa2swor!D", data.expectedError, false });
	}

	for (const auto& data : invalidPasswords) {
		testAccounts.emplace_back(AccountTestData{ "user", data.value, data.expectedError, false });
	}

	testAccounts.emplace_back(
		AccountTestData{ invalidLogins[0].value, invalidPasswords[0].value, invalidLogins[0].expectedError, false });
	const auto invalidAccountsSize{ testAccounts.size() };

	struct ValidTestData {
		std::string login;
		std::string password;
	};

	std::vector<ValidTestData> validTestAccounts{ { "user1", "pa2swor!D" }, { "User_2", "P@ssw0rd123" },
		{ "test.user-3", "T3st!ng#2024" }, { "adminUser", "Adm1n$ecure" }, { "guestUser", "Gu3st*Access" },
		{ "alphaUser01", "Alph@1234" }, { "alphaUser02", "Alph@1235" }, { "alphaUser03", "Alph@1236" },
		{ "alphaUser04", "Alph@1237" }, { "alphaUser05", "Alph@1238" }, { "betaUser01", "Bet@A1!2" },
		{ "betaUser02", "Bet@B2!3" }, { "betaUser03", "Bet@C3!4" }, { "betaUser04", "Bet@D4!5" },
		{ "betaUser05", "Bet@E5!6" }, { "gammaUser01", "Gamm@1A!" }, { "gammaUser02", "Gamm@2B!" },
		{ "gammaUser03", "Gamm@3C!" }, { "gammaUser04", "Gamm@4D!" }, { "gammaUser05", "Gamm@5E!" },
		{ "deltaUser01", "Delt@1a!" }, { "deltaUser02", "Delt@2b!" }, { "deltaUser03", "Delt@3c!" },
		{ "deltaUser04", "Delt@4d!" }, { "deltaUser05", "Delt@5e!" }, { "epsilonUser01", "Eps1l0n!A" },
		{ "epsilonUser02", "Eps1l0n!B" }, { "epsilonUser03", "Eps1l0n!C" }, { "epsilonUser04", "Eps1l0n!D" },
		{ "epsilonUser05", "Eps1l0n!E" }, { "zetaUser01", "Zet@1aA!" }, { "zetaUser02", "Zet@2bB!" },
		{ "zetaUser03", "Zet@3cC!" }, { "zetaUser04", "Zet@4dD!" }, { "zetaUser05", "Zet@5eE!" },
		{ "thetaUser01", "Thet@1A!" }, { "thetaUser02", "Thet@2B!" }, { "thetaUser03", "Thet@3C!" },
		{ "thetaUser04", "Thet@4D!" }, { "thetaUser05", "Thet@5E!" }, { "iotaUser01", "Iot@1aA!" },
		{ "iotaUser02", "Iot@2bB!" }, { "iotaUser03", "Iot@3cC!" }, { "iotaUser04", "Iot@4dD!" },
		{ "iotaUser05", "Iot@5eE!" }, { "kappaUser01", "Kapp@1A!" }, { "kappaUser02", "Kapp@2B!" },
		{ "kappaUser03", "Kapp@3C!" }, { "kappaUser04", "Kapp@4D!" }, { "kappaUser05", "Kapp@5E!" },
		{ "lambdaUser01", "Lambd@1a!" }, { "lambdaUser02", "Lambd@2b!" }, { "lambdaUser03", "Lambd@3c!" },
		{ "lambdaUser04", "Lambd@4d!" }, { "lambdaUser05", "Lambd@5e!" }, { "muUser01", "MuUs3r!1A" },
		{ "muUser02", "MuUs3r!2B" }, { "muUser03", "MuUs3r!3C" }, { "muUser04", "MuUs3r!4D" },
		{ "muUser05", "MuUs3r!5E" }, { "nuUser01", "NuUs3r!1a" }, { "nuUser02", "NuUs3r!2b" },
		{ "nuUser03", "NuUs3r!3c" }, { "nuUser04", "NuUs3r!4d" }, { "nuUser05", "NuUs3r!5e" },
		{ "xiUser01", "XiUs3r!1A" }, { "xiUser02", "XiUs3r!2B" }, { "xiUser03", "XiUs3r!3C" },
		{ "xiUser04", "XiUs3r!4D" }, { "xiUser05", "XiUs3r!5E" }, { "omicronUser01", "Om1cr@n!A" },
		{ "omicronUser02", "Om1cr@n!B" }, { "omicronUser03", "Om1cr@n!C" }, { "omicronUser04", "Om1cr@n!D" },
		{ "omicronUser05", "Om1cr@n!E" }, { "piUser01", "PiUs3r!1A" }, { "piUser02", "PiUs3r!2B" },
		{ "piUser03", "PiUs3r!3C" }, { "piUser04", "PiUs3r!4D" }, { "piUser05", "PiUs3r!5E" },
		{ "rhoUser01", "Rh0Us3r!A" }, { "rhoUser02", "Rh0Us3r!B" }, { "rhoUser03", "Rh0Us3r!C" },
		{ "rhoUser04", "Rh0Us3r!D" }, { "rhoUser05", "Rh0Us3r!E" }, { "sigmaUser01", "S1gm@Us!A" },
		{ "sigmaUser02", "S1gm@Us!B" }, { "sigmaUser03", "S1gm@Us!C" }, { "sigmaUser04", "S1gm@Us!D" },
		{ "sigmaUser05", "S1gm@Us!E" }, { "tauUser01", "T@uUs3r1A" }, { "tauUser02", "T@uUs3r2B" },
		{ "tauUser03", "T@uUs3r3C" }, { "tauUser04", "T@uUs3r4D" }, { "tauUser05", "T@uUs3r5E" },
		{ "upsilonUser01", "Ups1l0n!A" }, { "upsilonUser02", "Ups1l0n!B" }, { "upsilonUser03", "Ups1l0n!C" },
		{ "upsilonUser04", "Ups1l0n!D" }, { "upsilonUser05", "Ups1l0n!E" }, { "phiUser01", "Ph1Us3r!A" },
		{ "phiUser02", "Ph1Us3r!B" }, { "phiUser03", "Ph1Us3r!C" }, { "phiUser04", "Ph1Us3r!D" },
		{ "phiUser05", "Ph1Us3r!E" }, { "chiUser01", "Ch1Us3r!A" }, { "chiUser02", "Ch1Us3r!B" },
		{ "chiUser03", "Ch1Us3r!C" }, { "chiUser04", "Ch1Us3r!D" }, { "chiUser05", "Ch1Us3r!E" },
		{ "psiUser01", "Ps1Us3r!A" }, { "psiUser02", "Ps1Us3r!B" }, { "psiUser03", "Ps1Us3r!C" },
		{ "psiUser04", "Ps1Us3r!D" }, { "psiUser05", "Ps1Us3r!E" }, { "omegaUser01", "0meg@Us!A" },
		{ "omegaUser02", "0meg@Us!B" }, { "omegaUser03", "0meg@Us!C" }, { "omegaUser04", "0meg@Us!D" },
		{ "omegaUser05", "0meg@Us!E" }, { "valid@User.01", "Val1d@User!A" }, { "valid@User.02", "Val1d@User!B" },
		{ "valid@User.03", "Val1d@User!C" }, { "valid@User.04", "Val1d@User!D" }, { "valid@User.05", "Val1d@User!E" },
		{ "valid@User.06", "Val1d@User!F" }, { "valid@User.07", "Val1d@User!G" }, { "valid@User.08", "Val1d@User!H" },
		{ "valid@User.09", "Val1d@User!I" }, { "valid@User.10", "Val1d@User!J" } };

	for (const auto& data : validTestAccounts) {
		testAccounts.emplace_back(AccountTestData{ data.login, data.password, "", true });
	}

	constexpr size_t bufferSize{ 16384 };
	char buffer[bufferSize];
	std::vector<std::string> logs;
	const auto checkAccountLogs{ [&t, &accountsDataPath, &logs, &buffer, timestampPattern](
									 const auto& account, const size_t index, std::string&& log) {
		logs.clear();
		std::string accountPath{ accountsDataPath + "/" + account.login };

		IO::FileDescriptor::ExitGuard fd{ accountPath.c_str(), O_RDONLY, 0 };
		RETURN_IF_FALSE(t.Assert(fd.value != -1, true, "Open account data file to read logs"));

		size_t offset{};
		char* logBegin{ buffer };

		if (lseek(fd.value, accountSize + 1 /* new line character */, SEEK_SET) == -1) [[unlikely]] {
			RETURN_IF_FALSE(t.Assert(false, true, "Move seek on binary offset to read logs"));
		}

		do {
			const auto result{ read(fd.value, buffer + offset, bufferSize - offset) };
			if (result == 0) {
				break;
			}

			offset += static_cast<size_t>(result);
			if (offset >= bufferSize) [[unlikely]] {
				RETURN_IF_FALSE(t.Assert(false, true, "Account data file is too large to read"));
				break;
			}

			if (result > 0) {
				auto logBeginCopy{ logBegin };
				do {
					const auto toRead{ static_cast<size_t>(result - (logBegin - logBeginCopy)) };
					if (toRead == 0) {
						break;
					}
					const auto newlinePos{ static_cast<char*>(std::memchr(logBegin, '\n', toRead)) };
					if (newlinePos == nullptr) {
						logs.emplace_back(std::string(logBegin, toRead));
						break;
					}

					const auto strSize{ static_cast<size_t>(newlinePos - logBegin) };
					logs.emplace_back(std::string(logBegin, strSize));
					logBegin = newlinePos + 1;
				} while (true);
			}
			else if (result == -1) {
				RETURN_IF_FALSE(t.Assert(false, true, "Read line from account data file"));
			}
		} while (true);

		RETURN_IF_FALSE(t.Assert(logs.size(), account.logsCount, "Logs count"));

		const auto timestampBeginPos{ log.find(timestampPattern) };
		auto& actualLog{ logs[index] };
		if (timestampBeginPos != std::string::npos) {
			if (actualLog.size() < timestampBeginPos + timestampPattern.size()) {
				RETURN_IF_FALSE(t.Assert(actualLog, log, "Actual log is too short"));
			}

			const auto* actualLogBegin{ actualLog.data() + timestampBeginPos };
			memcpy(log.data() + timestampBeginPos, actualLogBegin, timestampPattern.size());

			const auto actualTimestamp{ Timer::Create(std::string_view{ actualLogBegin, timestampPattern.size() }) };
			RETURN_IF_FALSE(
				t.Assert((account.lastActivity - actualTimestamp) < Timer::Duration::CreateMilliseconds(500), true,
					"Difference between actual and approximately action timestamps in logs is in range"))
		}

		RETURN_IF_FALSE(t.Assert(actualLog, log, std::format("Check log №{}", index)));

		return true;
	} };

	const auto gradeMin{ static_cast<G>(std::numeric_limits<U>::min()) };
	const auto gradeMax{ static_cast<G>(std::numeric_limits<U>::max()) };
	const auto gradeZero{ static_cast<G>(0) };

	const auto tryAccess{ [&t, &mod, &checkAccountLogs](
							  auto& current, const U gradeToCheck, const bool expectedAccess) {
		RETURN_IF_FALSE(t.Assert(mod.IsAccessGranted(current.connection, static_cast<G>(gradeToCheck)), expectedAccess,
			std::format("Access is granted to {}, to account {}", gradeToCheck, current.login)));

		if (current.isLoggedOn) {
			current.lastActivity = Timer{};
			++current.logsCount;
			RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
				std::format("Access check for grade {} at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, result: {}", gradeToCheck,
					expectedAccess)));
		}

		return true;
	} };

	// 3. Check account registration with various login and password values and check their default restrictions; check
	// account logs after registration; check that connections (they are not even internally assigned) are not
	// logged-on and access is not granted right after registration; check that duplicate registrations are not
	// allowed
	std::string error;
	for (auto& accountData : testAccounts) {
		RETURN_IF_FALSE(t.Assert(mod.RegisterAccount(accountData.login, accountData.password, error),
			accountData.shouldRegister,
			std::format("Register account with login: '{}', password: '{}'", accountData.login, accountData.password)));
		if (!accountData.shouldRegister) {
			RETURN_IF_FALSE(t.Assert(error, accountData.expectedError, "Check error message for account registration"));
		}
		else {
			accountData.lastActivity = Timer{};
			RETURN_IF_FALSE(t.Assert(error, "", "Check error of successful account registration"));

			++registeredAccounts;
			++accountData.logsCount;
			Timer logTimestamp{ 0 };
			RETURN_IF_FALSE(checkAccountLogs(
				accountData, accountData.logsCount - 1, "Registered at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));

			RETURN_IF_FALSE(tryAccess(accountData, static_cast<U>(gradeMin), false));
			RETURN_IF_FALSE(tryAccess(accountData, static_cast<U>(gradeMax), false));
			RETURN_IF_FALSE(tryAccess(accountData, static_cast<U>(gradeZero), false));
		}

		mod.LogoutConnection(accountData.connection);
		error.clear();
	}

	RETURN_IF_FALSE(checkAccountsSize());

	// 4. Check duplicate account registrations are not allowed
	for (const auto& accountData : testAccounts) {
		RETURN_IF_FALSE(t.Assert(mod.RegisterAccount(accountData.login, accountData.password, error), false,
			std::format("Register account with login: '{}', password: '{}'", accountData.login, accountData.password)));
		if (!accountData.shouldRegister) {
			RETURN_IF_FALSE(t.Assert(error, accountData.expectedError, "Check error message for account registration"));
		}
		else {
			RETURN_IF_FALSE(t.Assert(
				error, "Account with this login already exists", "Check error of duplicate account registration"));
		}
		error.clear();
	}

	RETURN_IF_FALSE(checkAccountsSize());

	const auto getSavedAccount{ [&accountsDataPath, &accountSize, &t]<typename T>(const std::string& login) {
		T account;
		std::string accountPath{ accountsDataPath + "/" + login };

		IO::FileDescriptor::ExitGuard fd{ accountPath.c_str(), O_RDONLY, 0 };
		if (!t.Assert(fd.value != -1, true, "Open account data file to read last log")) {
			return std::make_unique<T>();
		}

		if (!t.Assert(read(fd.value, static_cast<void*>(&account), accountSize), accountSize,
				"Read account data from file")) {
			return std::make_unique<T>();
		}

		return std::make_unique<T>(std::move(account));
	} };

	// 5. Check saved account binary data after registration
	const auto emptyTimer{ Timer{ 0 } };
	for (auto& accountData : testAccounts) {
		if (accountData.shouldRegister) {
			auto account{ getSavedAccount.operator()<A>(accountData.login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer,
				std::format("Check saved blocked till for account '{}'", accountData.login)));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), accountData.login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), gradeZero, "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), false, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}
	}

	const auto checkPathDoesNotExist{ [&t, &accountsDataPath](const std::string& login) {
		if (login.empty() || login == "." || login == ".." || login == "/") {
			return true;
		}

		RETURN_IF_FALSE(t.Assert(IO::HasPath(std::string_view{ accountsDataPath + "/" + login }), false,
			"Check that account data file does not exist"));
		return true;
	} };

	const auto checkLogonWithInvalidPassword{ [&t, &mod, &error, &checkAccountLogs](auto& current,
												  std::string_view password, std::string_view expectedError) {
		RETURN_IF_FALSE(t.Assert(mod.LogonConnection(current->connection, current->login, password, error), false,
			"Logon connection with invalid password"));
		RETURN_IF_FALSE(t.Assert(
			error, expectedError, "Check error message for logon attempt on connection with invalid password"));
		error.clear();
		current->lastActivity = Timer{};
		++current->logsCount;
		RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
			std::format("Failed logon attempt at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}, reason: Invalid login "
						"or password",
				current->connection)));
		return true;
	} };

	enum class BlockTillResult : int8_t { Block, BlockIncrease, BlockDecrease, BlockFailed, Unblock, UnblockFailed };

	const auto getBlockSpecificResult{ [](const Timer blockedTillOld, const Timer blockedTillNew) -> BlockTillResult {
		if (blockedTillOld <= Timer{}) {
			return BlockTillResult::Block;
		}

		if (blockedTillOld > blockedTillNew) {
			return BlockTillResult::BlockDecrease;
		}

		if (blockedTillOld < blockedTillNew) {
			return BlockTillResult::BlockIncrease;
		}

		return BlockTillResult::BlockFailed;
	} };

	const auto setBlockedTill{ [&t, &mod, &checkPathDoesNotExist, &getSavedAccount, &tryAccess, &checkAccountLogs,
								   &activeConnections](
								   auto& current, const Timer blockedTill, const BlockTillResult expectedResult) {
		if (!current.shouldRegister) {
			RETURN_IF_FALSE(t.Assert(mod.BlockAccountTill(current.login, blockedTill), false,
				std::format("Block account with login: '{}' till {}", current.login, blockedTill.ToString())));
			RETURN_IF_FALSE(checkPathDoesNotExist(current.login));
			return true;
		}

		if (current.isDeleted) {
			RETURN_IF_FALSE(t.Assert(mod.BlockAccountTill(current.login, blockedTill), false,
				std::format("Block deleted account with login: '{}' till {}", current.login, blockedTill.ToString())));
			return true;
		}

		RETURN_IF_FALSE(t.Assert(mod.BlockAccountTill(current.login, blockedTill), current.blockedTill != blockedTill,
			std::format("Block account with login: '{}' till {}", current.login, blockedTill.ToString())));

		const auto now{ Timer{} };
		++current.logsCount;
		if (blockedTill > now) {
			if (current.blockedTill <= now) {
				if (current.isLoggedOn) {
					current.isLoggedOn = false;
					current.reLogon = true;
					++current.logsCount;
					RETURN_IF_FALSE(tryAccess(current, current.grade - 1, false));
					RETURN_IF_FALSE(tryAccess(current, current.grade, false));
					RETURN_IF_FALSE(tryAccess(current, current.grade + 1, false));
					RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 2,
						std::format("Logout due to blocking at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}",
							current.connection)));
					--activeConnections;
					RETURN_IF_FALSE(t.Assert(mod.GetLogonConnectionsSize(), activeConnections,
						"Expected number of active connections in module"));
				}

				RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
					std::format("Blocked till {} at XXXX-XX-XX XX:XX:XX.XXXXXXXXX", blockedTill.ToString())));
				RETURN_IF_FALSE(
					t.Assert(BlockTillResult::Block, expectedResult, "Expected result of blocking is not matched"));
			}
			else if (current.blockedTill > blockedTill) {
				RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
					std::format("Decrease blocked till {} at XXXX-XX-XX XX:XX:XX.XXXXXXXXX", blockedTill.ToString())));
				RETURN_IF_FALSE(t.Assert(
					BlockTillResult::BlockDecrease, expectedResult, "Expected result of blocking is not matched"));
			}
			else if (current.blockedTill < blockedTill) {
				RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
					std::format("Increase blocked till {} at XXXX-XX-XX XX:XX:XX.XXXXXXXXX", blockedTill.ToString())));
				RETURN_IF_FALSE(t.Assert(
					BlockTillResult::BlockIncrease, expectedResult, "Expected result of blocking is not matched"));
			}
			else {
				RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
					std::format("Failed to block at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to the same time {}",
						blockedTill.ToString())));
				RETURN_IF_FALSE(t.Assert(
					BlockTillResult::BlockFailed, expectedResult, "Expected result of blocking is not matched"));
			}

			current.blockedTill = blockedTill;
		}
		else if (current.blockedTill > now) {
			RETURN_IF_FALSE(
				checkAccountLogs(current, current.logsCount - 1, "Unblocked at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));
			RETURN_IF_FALSE(
				t.Assert(BlockTillResult::Unblock, expectedResult, "Expected result of blocking is not matched"));

			current.blockedTill = Timer{ 0 };
		}
		else {
			RETURN_IF_FALSE(checkAccountLogs(
				current, current.logsCount - 1, "Failed to unblock at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, not blocked"));
			RETURN_IF_FALSE(
				t.Assert(BlockTillResult::UnblockFailed, expectedResult, "Expected result of blocking is not matched"));
		}

		auto account{ getSavedAccount.operator()<A>(current.login) };
		RETURN_IF_FALSE((account.get() != nullptr));
		RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), current.blockedTill, "Check saved blocked till"));
		RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current.login, "Check saved login"));
		RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current.grade), "Check saved grade"));
		RETURN_IF_FALSE(t.Assert(account->IsActive(), current.isActivated, "Check saved active status"));
		RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));

		return true;
	} };

	const auto checkAccess{ [&t, &mod, &checkAccountLogs, &getSavedAccount, &error, emptyTimer, &invalidPasswords,
								&accountsDataPath, &checkPathDoesNotExist, &checkLogonWithInvalidPassword,
								&invalidLogins, &activeConnections, &tryAccess, &setBlockedTill,
								&getBlockSpecificResult]<typename T>(const T::iterator begin, const T::iterator end) {
		// 1*. Check access
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(tryAccess(*current, current->grade - 1, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade + 1, false));
		}

		// 2*. Modify account grade, check logs and saved binary data
		auto newGrade{ static_cast<G>(-2) };
		for (auto current{ begin }; current != end; ++current) {

#define TMP_MSAPI_UNIT_TEST_AUTHORIZATION_MODIFY_ACCOUNT_GRADE                                                         \
	if (!current->shouldRegister) {                                                                                    \
		RETURN_IF_FALSE(t.Assert(mod.ModifyAccountGrade(current->login, newGrade), false,                              \
			std::format("Modify account {} grade to {}", current->login, U(newGrade))));                               \
		RETURN_IF_FALSE(checkPathDoesNotExist(current->login));                                                        \
		continue;                                                                                                      \
	}                                                                                                                  \
                                                                                                                       \
	if (current->isDeleted) {                                                                                          \
		RETURN_IF_FALSE(t.Assert(mod.ModifyAccountGrade(current->login, newGrade), false,                              \
			std::format("Modify deleted account {} grade to {}", current->login, U(newGrade))));                       \
		continue;                                                                                                      \
	}                                                                                                                  \
                                                                                                                       \
	const bool gradeWasNotChanged{ current->grade == static_cast<U>(newGrade) };                                       \
	RETURN_IF_FALSE(t.Assert(mod.ModifyAccountGrade(current->login, newGrade), !gradeWasNotChanged,                    \
		std::format("Modify account {} grade to {}", current->login, U(newGrade))));                                   \
                                                                                                                       \
	current->lastActivity = Timer{};                                                                                   \
	current->grade = static_cast<U>(newGrade);                                                                         \
	if (U(newGrade) == std::numeric_limits<U>::max()) {                                                                \
		newGrade = static_cast<G>(0);                                                                                  \
	}                                                                                                                  \
	newGrade = static_cast<G>(U(newGrade) + 1);                                                                        \
                                                                                                                       \
	++current->logsCount;                                                                                              \
	RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,                                                 \
		gradeWasNotChanged ? "Failed to change grade at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to the same one"                 \
						   : std::format("Grade is changed to {} at XXXX-XX-XX XX:XX:XX.XXXXXXXXX", current->grade)));

			TMP_MSAPI_UNIT_TEST_AUTHORIZATION_MODIFY_ACCOUNT_GRADE;

			auto account{ getSavedAccount.operator()<A>(current->login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), current->blockedTill, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), current->isActivated, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}

		// 3*. Check access with new grade
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(tryAccess(*current, current->grade - 1, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade + 1, false));
		}

		// 4*. Try to set blocked till (future/same time again/forward/back), check logs and saved binary data
		const auto blockedTill{ Timer{} + Timer::Duration::CreateSeconds(600) };
		const auto blockedTillP1{ blockedTill + Timer::Duration::CreateSeconds(1) };
		const auto blockedTillP2{ blockedTill + Timer::Duration::CreateSeconds(2) };
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(
				setBlockedTill(*current, blockedTillP1, getBlockSpecificResult(current->blockedTill, blockedTillP1)));
			RETURN_IF_FALSE(setBlockedTill(*current, blockedTillP1, BlockTillResult::BlockFailed));
			RETURN_IF_FALSE(setBlockedTill(*current, blockedTillP2, BlockTillResult::BlockIncrease));
			RETURN_IF_FALSE(setBlockedTill(*current, blockedTill, BlockTillResult::BlockDecrease));
		}

		// 5*. Try to logon with both invalid and valid passwords, check binary data and logs when they are
		// expected to be written; activate account, even if it is already activated
		for (auto current{ begin }; current != end; ++current) {
			if (!current->shouldRegister || current->isDeleted) {
				RETURN_IF_FALSE(t.Assert(
					mod.LogonConnection(current->connection, current->login, current->password, error), false,
					std::format("Logon connection {} with account login: '{}'", current->connection, current->login)));
				RETURN_IF_FALSE(t.Assert(error, "Invalid login or password", "Check error message for logon attempt"));
				error.clear();

				if (!current->shouldRegister) {
					RETURN_IF_FALSE(checkPathDoesNotExist(current->login));
				}
				continue;
			}

			RETURN_IF_FALSE(t.Assert(
				checkLogonWithInvalidPassword(current,
					std::string{ current->password.data(), current->password.size() - 1 }, "Invalid login or password"),
				true, std::format("Logon connection for account '{}' with invalid password", current->login)));

			if (!current->isActivated) {
				RETURN_IF_FALSE(
					t.Assert(mod.LogonConnection(current->connection, current->login, current->password, error), false,
						std::format("Logon connection {}", current->connection)));
				RETURN_IF_FALSE(t.Assert(error, "Account is not activated", "Check error message for logon attempt"));

				current->lastActivity = Timer{};
				++current->logsCount;
				RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
					std::format("Failed logon attempt at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}, reason: Account "
								"is not activated",
						current->connection)));
			}
			error.clear();

			{
				auto account{ getSavedAccount.operator()<A>(current->login) };
				RETURN_IF_FALSE((account.get() != nullptr));
				RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), blockedTill, "Check saved blocked till"));
				RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
				RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
				RETURN_IF_FALSE(t.Assert(account->IsActive(), current->isActivated, "Check saved active status"));
				RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
			}

			RETURN_IF_FALSE(t.Assert(
				mod.SetAccountActivatedState(current->login, true), !current->isActivated, "Activate account"));
			current->lastActivity = Timer{};
			++current->logsCount;

			if (!current->isActivated) {
				current->isActivated = true;
				RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
					"Activation state is changed to true at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));

				RETURN_IF_FALSE(t.Assert(
					mod.SetAccountActivatedState(current->login, true), false, "Activate account second time"));
				current->lastActivity = Timer{};
				++current->logsCount;
				RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
					"Failed to change activation state at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to the same one"));
			}
			else {
				RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
					"Failed to change activation state at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to the same one"));
			}

			{
				auto account{ getSavedAccount.operator()<A>(current->login) };
				RETURN_IF_FALSE((account.get() != nullptr));
				RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), blockedTill, "Check saved blocked till"));
				RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
				RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
				RETURN_IF_FALSE(t.Assert(account->IsActive(), true, "Check saved active status"));
				RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
			}
		}

		// 6*. Check access when blocked
		for (auto current{ begin }; current != end; ++current) {
			if (!current->shouldRegister || current->isDeleted) {
				RETURN_IF_FALSE(t.Assert(
					mod.LogonConnection(current->connection, current->login, current->password, error), false,
					std::format("Logon connection {} with account login: '{}'", current->connection, current->login)));
				RETURN_IF_FALSE(t.Assert(error, "Invalid login or password", "Check error message"));
				error.clear();

				if (!current->shouldRegister) {
					RETURN_IF_FALSE(checkPathDoesNotExist(current->login));
				}
				continue;
			}

			RETURN_IF_FALSE(
				t.Assert(mod.LogonConnection(current->connection, current->login, current->password, error), false,
					std::format("Logon connection {} with account login: '{}'", current->connection, current->login)));
			const auto& blockedTillError{ std::format("Account is blocked till {}", blockedTill.ToString()) };
			RETURN_IF_FALSE(t.Assert(error, blockedTillError, "Check error message"));
			error.clear();

			current->lastActivity = Timer{};
			++current->logsCount;
			RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
				std::format("Failed logon attempt at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}, reason: {}",
					current->connection, blockedTillError)));
		}

#define TMP_MSAPI_UNIT_TEST_AUTHORIZATION_CHANGE_NEW_GRADE_COUNTER                                                     \
	newGrade = static_cast<G>(U(newGrade) - 3);                                                                        \
	if (end - begin < 4) [[unlikely]] {                                                                                \
		RETURN_IF_FALSE(t.Assert(false, true, "At least four accounts are required to continue the test"));            \
	}

		// 7*. Modify account grade
		TMP_MSAPI_UNIT_TEST_AUTHORIZATION_CHANGE_NEW_GRADE_COUNTER;
		for (auto current{ begin }; current != end; ++current) {
			TMP_MSAPI_UNIT_TEST_AUTHORIZATION_MODIFY_ACCOUNT_GRADE;

			auto account{ getSavedAccount.operator()<A>(current->login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), blockedTill, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), true, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}

		// 8*. Check access with new grade
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(tryAccess(*current, current->grade - 1, false));
			RETURN_IF_FALSE(tryAccess(*current, current->grade, false));
			RETURN_IF_FALSE(tryAccess(*current, current->grade + 1, false));
		}

		// 9*. Unblock (past/empty timestamps and empty again)
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(setBlockedTill(*current, Timer{}, BlockTillResult::Unblock));
			RETURN_IF_FALSE(setBlockedTill(*current, blockedTill, BlockTillResult::Block));
			RETURN_IF_FALSE(setBlockedTill(*current, emptyTimer, BlockTillResult::Unblock));
			RETURN_IF_FALSE(setBlockedTill(*current, emptyTimer, BlockTillResult::UnblockFailed));

			if (current->reLogon) {
				RETURN_IF_FALSE(t.Assert(
					mod.LogonConnection(current->connection, current->login, current->password, error), true,
					std::format("Logon connection {} with account login: '{}'", current->connection, current->login)));
				RETURN_IF_FALSE(t.Assert(error, "", "Check empty error message for successful logon"));
				current->isLoggedOn = true;
				current->reLogon = false;
				current->lastActivity = Timer{};
				++current->logsCount;
				RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
					std::format("Logon at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", current->connection)));
				++activeConnections;
				RETURN_IF_FALSE(t.Assert(mod.GetLogonConnectionsSize(), activeConnections,
					"Expected number of active connections in module"));
			}
		}

		if (Timer{} >= blockedTill) [[unlikely]] {
			RETURN_IF_FALSE(t.Assert(false, true, "Current time is already greater than blocked till time"));
		}

		// 10*. Check access when unblocked
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(tryAccess(*current, current->grade - 1, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade + 1, false));
		}

		// 11*. Modify account grade
		TMP_MSAPI_UNIT_TEST_AUTHORIZATION_CHANGE_NEW_GRADE_COUNTER;
		for (auto current{ begin }; current != end; ++current) {
			TMP_MSAPI_UNIT_TEST_AUTHORIZATION_MODIFY_ACCOUNT_GRADE;

			auto account{ getSavedAccount.operator()<A>(current->login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), true, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}

		// 12*. Check access with new grade
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(tryAccess(*current, current->grade - 1, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade + 1, false));
		}

		// 13*. Modify passwords (invalid/same/valid)
		for (auto current{ begin }; current != end; ++current) {
			if (!current->shouldRegister || current->isDeleted) {
				RETURN_IF_FALSE(t.Assert(mod.ModifyAccountPassword(current->login, current->password, error), false,
					std::format("Modify password of unregistered account with login: {}", current->login)));
				RETURN_IF_FALSE(t.Assert(
					error, "", "Check empty error message for password modification attempt on unregistered account"));

				if (!current->shouldRegister) {
					RETURN_IF_FALSE(checkPathDoesNotExist(current->login));
				}
				continue;
			}

			for (const auto& [password, expectedError] : invalidPasswords) {
				RETURN_IF_FALSE(t.Assert(mod.ModifyAccountPassword(current->login, password, error), false,
					std::format("Modify password of account with login: '{}' to invalid one", current->login)));
				RETURN_IF_FALSE(
					t.Assert(error, expectedError, "Check error message for password modification attempt on account"));
				error.clear();
				current->lastActivity = Timer{};
				++current->logsCount;
				RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
					std::format(
						"Failed to change password at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, reason: {}", expectedError)));
			}

			RETURN_IF_FALSE(t.Assert(mod.ModifyAccountPassword(current->login, current->password, error), false,
				"Modify password to the same one"));
			RETURN_IF_FALSE(t.Assert(error, "New password is the same as the current one",
				"Check error message for password modification attempt to the same one"));
			error.clear();
			current->lastActivity = Timer{};
			++current->logsCount;
			RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
				"Failed to change password at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to the same one"));

			RETURN_IF_FALSE(checkLogonWithInvalidPassword(current,
				std::string{ current->password.data(), current->password.size() - 1 }, "Invalid login or password"));

			for (int8_t index{}; index < 3; ++index) {
				const auto oldPassword{ current->password };
				current->password = std::format("{}_", current->password);
				RETURN_IF_FALSE(t.Assert(mod.ModifyAccountPassword(current->login, current->password, error), true,
					"Modify password to new valid one"));
				RETURN_IF_FALSE(t.Assert(error, "", "Check empty error message for successful password modification"));

				++current->logsCount;
				current->lastActivity = Timer{};
				RETURN_IF_FALSE(checkAccountLogs(
					*current, current->logsCount - 1, "Password is changed at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));

				RETURN_IF_FALSE(checkLogonWithInvalidPassword(current,
					std::string{ current->password.data(), current->password.size() - 1 },
					"Invalid login or password"));
				RETURN_IF_FALSE(checkLogonWithInvalidPassword(current, oldPassword, "Invalid login or password"));
			}

			auto account{ getSavedAccount.operator()<A>(current->login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), true, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}

		// 14*. Modify logins (valid/invalid)
		for (auto current{ begin }; current != end; ++current) {
			if (!current->shouldRegister) {
				RETURN_IF_FALSE(t.Assert(mod.ModifyAccountLogin(current->login, "someRandomLogin", error), false,
					std::format("Modify login of unregistered account with login: '{}'", current->login)));
				RETURN_IF_FALSE(t.Assert(
					error, "", "Check empty error message for login modification attempt on unregistered account"));
				RETURN_IF_FALSE(checkPathDoesNotExist(current->login));
				continue;
			}

			RETURN_IF_FALSE(t.Assert(mod.ModifyAccountLogin(current->login, current->login, error), false,
				std::format("Modify login to the same one for account {}", current->login)));
			RETURN_IF_FALSE(t.Assert(error, "", "Check error message for login modification attempt to the same one"));

			for (const auto& [newLogin, expectedError] : invalidLogins) {
				RETURN_IF_FALSE(t.Assert(mod.ModifyAccountLogin(current->login, newLogin, error), false,
					"Modify login of account to invalid one"));
				RETURN_IF_FALSE(
					t.Assert(error, expectedError, "Check error message for login modification attempt on account"));

				error.clear();
			}

			{
				auto account{ getSavedAccount.operator()<A>(current->login) };
				RETURN_IF_FALSE((account.get() != nullptr));
				RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
				RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
				RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
				RETURN_IF_FALSE(t.Assert(account->IsActive(), current->isActivated, "Check saved active status"));
				RETURN_IF_FALSE(
					t.Assert(account->IsInitialized(), !current->isDeleted, "Check saved initialized status"));
			}

			const auto newLogin{ current->login + "_" };

			if (current->isDeleted) {
				RETURN_IF_FALSE(t.Assert(mod.ModifyAccountLogin(current->login, newLogin, error), false,
					std::format("Modify login of deleted account to '{}'", newLogin)));
				RETURN_IF_FALSE(
					t.Assert(error, "", "Check error message for login modification attempt on deleted account"));
				continue;
			}

			const auto oldLogin{ current->login };
			RETURN_IF_FALSE(t.Assert(mod.ModifyAccountLogin(current->login, newLogin, error), true,
				std::format("Modify login of account to '{}'", newLogin)));
			RETURN_IF_FALSE(t.Assert(error, "", "Check error message for login modification"));
			current->login = newLogin;

			current->lastActivity = Timer{};
			++current->logsCount;
			RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
				std::format(
					"Login is changed from {} to {} at XXXX-XX-XX XX:XX:XX.XXXXXXXXX", oldLogin, current->login)));

			{
				auto account{ getSavedAccount.operator()<A>(newLogin) };
				RETURN_IF_FALSE((account.get() != nullptr));
				RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
				RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), newLogin, "Check saved login"));
				RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
				RETURN_IF_FALSE(t.Assert(account->IsActive(), current->isActivated, "Check saved active status"));
				RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
			}

			RETURN_IF_FALSE(checkPathDoesNotExist(oldLogin));
		}

		// 15*. Deactivate
		for (auto current{ begin }; current != end; ++current) {
			if (!current->shouldRegister || current->isDeleted) {
				RETURN_IF_FALSE(t.Assert(mod.SetAccountActivatedState(current->login, false), false,
					std::format("Deactivate unregistered account with login: '{}'", current->login)));
				RETURN_IF_FALSE(
					t.Assert(error, "", "Check empty error message for deactivation attempt on unregistered account"));

				if (!current->shouldRegister) {
					RETURN_IF_FALSE(checkPathDoesNotExist(current->login));
				}
				continue;
			}

			RETURN_IF_FALSE(t.Assert(mod.SetAccountActivatedState(current->login, false), true,
				std::format("Deactivate account with login: '{}'", current->login)));
			current->lastActivity = Timer{};

			if (current->isLoggedOn) {
				current->isLoggedOn = false;
				current->reLogon = true;
				current->logsCount += 2;
				RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 2,
					std::format("Logout due to deactivation at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}",
						current->connection)));
				--activeConnections;
				RETURN_IF_FALSE(t.Assert(mod.GetLogonConnectionsSize(), activeConnections,
					"Expected number of active connections in module"));
			}
			else {
				++current->logsCount;
			}

			RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
				"Activation state is changed to false at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));

			RETURN_IF_FALSE(
				t.Assert(mod.SetAccountActivatedState(current->login, false), false, "Deactivate account second time"));
			current->lastActivity = Timer{};
			++current->logsCount;
			RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
				"Failed to change activation state at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to the same one"));

			auto account{ getSavedAccount.operator()<A>(current->login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), false, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}

		// 16*. Check access when deactivated
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(tryAccess(*current, current->grade - 1, false));
			RETURN_IF_FALSE(tryAccess(*current, current->grade, false));
			RETURN_IF_FALSE(tryAccess(*current, current->grade + 1, false));
		}

		// 17*. Activate back
		for (auto current{ begin }; current != end; ++current) {
			if (!current->shouldRegister || current->isDeleted) {
				RETURN_IF_FALSE(t.Assert(mod.SetAccountActivatedState(current->login, true), false,
					std::format("Activate unregistered account with login: '{}'", current->login)));
				RETURN_IF_FALSE(
					t.Assert(error, "", "Check empty error message for deactivation attempt on unregistered account"));

				if (!current->shouldRegister) {
					RETURN_IF_FALSE(checkPathDoesNotExist(current->login));
				}
				continue;
			}

			RETURN_IF_FALSE(t.Assert(mod.SetAccountActivatedState(current->login, true), true,
				std::format("Activate account with login: '{}'", current->login)));
			current->lastActivity = Timer{};
			++current->logsCount;
			RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
				"Activation state is changed to true at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));

			if (current->reLogon) {
				RETURN_IF_FALSE(t.Assert(
					mod.LogonConnection(current->connection, current->login, current->password, error), true,
					std::format("Logon connection {} with account login: '{}'", current->connection, current->login)));
				RETURN_IF_FALSE(t.Assert(error, "", "Check empty error message for successful logon"));
				current->isLoggedOn = true;
				current->reLogon = false;
				current->lastActivity = Timer{};
				++current->logsCount;
				RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
					std::format("Logon at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", current->connection)));
				++activeConnections;
				RETURN_IF_FALSE(t.Assert(mod.GetLogonConnectionsSize(), activeConnections,
					"Expected number of active connections in module"));
			}

			RETURN_IF_FALSE(
				t.Assert(mod.SetAccountActivatedState(current->login, true), false, "Activate account second time"));
			current->lastActivity = Timer{};
			++current->logsCount;
			RETURN_IF_FALSE(checkAccountLogs(*current, current->logsCount - 1,
				"Failed to change activation state at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to the same one"));

			auto account{ getSavedAccount.operator()<A>(current->login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current->login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current->grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), true, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}

		// 18*. Check access
		for (auto current{ begin }; current != end; ++current) {
			RETURN_IF_FALSE(tryAccess(*current, current->grade - 1, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade, current->isLoggedOn));
			RETURN_IF_FALSE(tryAccess(*current, current->grade + 1, false));
		}

		return true;
	} };

	// 6. Check access (* group of tests) for all accounts (no logon connections)
	RETURN_IF_FALSE(t.Assert(checkAccess.operator()<decltype(testAccounts)>(testAccounts.begin(), testAccounts.end()),
		true, "Check access for all accounts (no logon connections)"));

	// 7. Delete some accounts (no logon connections)
	if (testAccounts.size() < 21) [[unlikely]] {
		RETURN_IF_FALSE(t.Assert(false, true, "At least 21 accounts are required to continue the test"));
	}
	for (size_t index{}; index < testAccounts.size(); index += 20) {
		auto& current{ testAccounts[index] };
		if (!current.shouldRegister) {
			mod.DeleteAccount(current.login);
			RETURN_IF_FALSE(t.Assert(checkPathDoesNotExist(current.login), true,
				std::format("Attempt to delete unregistered account with login: '{}'", current.login)));
			continue;
		}

		mod.DeleteAccount(current.login);
		current.lastActivity = Timer{};
		current.isActivated = false;
		current.isDeleted = true;
		++current.logsCount;
		--registeredAccounts;
		RETURN_IF_FALSE(t.Assert(
			mod.GetRegisteredAccountsSize(), registeredAccounts, "Check registered accounts size after deletion"));
		RETURN_IF_FALSE(t.Assert(checkAccountLogs(current, current.logsCount - 1,
									 "Marked as uninitialized and deactivated at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"),
			true, std::format("Delete account with login: '{}'", current.login)));

		{
			auto account{ getSavedAccount.operator()<A>(current.login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current.login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current.grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), false, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), false, "Check saved initialized status"));
		}

		// Try access and logon with deleted accounts
		RETURN_IF_FALSE(tryAccess(current, current.grade - 1, false));
		RETURN_IF_FALSE(tryAccess(current, current.grade, false));
		RETURN_IF_FALSE(tryAccess(current, current.grade + 1, false));

		RETURN_IF_FALSE(t.Assert(mod.LogonConnection(current.connection, current.login, current.password, error), false,
			std::format("Logon connection {} with deleted account", current.connection)));
		RETURN_IF_FALSE(t.Assert(error, "Invalid login or password",
			std::format(
				"Check error message for logon attempt on connection {} with deleted account", current.connection)));
		error.clear();

		{
			auto account{ getSavedAccount.operator()<A>(current.login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current.login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current.grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), false, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), false, "Check saved initialized status"));
		}
	}

	// 8. Logon connections
	for (auto& current : testAccounts) {
		if (!current.shouldRegister || current.isDeleted) {
			RETURN_IF_FALSE(t.Assert(mod.LogonConnection(current.connection, current.login, current.password, error),
				false, std::format("Logon connection {} with account login: '{}'", current.connection, current.login)));
			RETURN_IF_FALSE(t.Assert(error, "Invalid login or password", "Check error message"));
			error.clear();

			if (current.isDeleted) {
				auto account{ getSavedAccount.operator()<A>(current.login) };
				RETURN_IF_FALSE((account.get() != nullptr));
				RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
				RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current.login, "Check saved login"));
				RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current.grade), "Check saved grade"));
				RETURN_IF_FALSE(t.Assert(account->IsActive(), false, "Check saved active status"));
				RETURN_IF_FALSE(t.Assert(account->IsInitialized(), false, "Check saved initialized status"));
			}
			else {
				RETURN_IF_FALSE(checkPathDoesNotExist(current.login));
			}
			continue;
		}

		RETURN_IF_FALSE(t.Assert(mod.LogonConnection(current.connection, current.login, current.password, error), true,
			std::format("Logon connection {}", current.connection)));
		RETURN_IF_FALSE(t.Assert(error, "", "Check error message for login modification"));
		current.lastActivity = Timer{};
		current.isLoggedOn = true;
		++current.logsCount;
		RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
			std::format("Logon at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", current.connection)));
		++activeConnections;
		RETURN_IF_FALSE(t.Assert(
			mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

		auto account{ getSavedAccount.operator()<A>(current.login) };
		RETURN_IF_FALSE((account.get() != nullptr));
		RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
		RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current.login, "Check saved login"));
		RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current.grade), "Check saved grade"));
		RETURN_IF_FALSE(t.Assert(account->IsActive(), true, "Check saved active status"));
		RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
	}

	// 9. Check access (* group of tests) for all accounts (has logon connections)
	RETURN_IF_FALSE(t.Assert(checkAccess.operator()<decltype(testAccounts)>(testAccounts.begin(), testAccounts.end()),
		true, "Check access for all accounts (has logon connections)"));

	// 10. Try to logon already logged-on connections
	for (auto& current : testAccounts) {
		if (!current.shouldRegister || current.isDeleted) {
			continue;
		}

		RETURN_IF_FALSE(t.Assert(mod.LogonConnection(current.connection, current.login, current.password, error), false,
			std::format("Logon already logged-on connection {}", current.connection)));
		RETURN_IF_FALSE(t.Assert(error, "", "Check empty error message for successful logon"));
		current.lastActivity = Timer{};
		++current.logsCount;
		RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
			std::format("Failed logon attempt at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to already logged-on connection {}",
				current.connection)));

		RETURN_IF_FALSE(t.Assert(mod.LogonConnection(current.connection + 1, current.login, current.password, error),
			false, std::format("Logon one more connection {}", current.connection + 1)));
		RETURN_IF_FALSE(t.Assert(error, "Multiple logon is not allowed", "Check error message"));
		current.lastActivity = Timer{};
		++current.logsCount;
		RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
			std::format("Multiple logon is not allowed, attempting connection {} at XXXX-XX-XX XX:XX:XX.XXXXXXXXX",
				current.connection + 1)));
		error.clear();

		auto account{ getSavedAccount.operator()<A>(current.login) };
		RETURN_IF_FALSE((account.get() != nullptr));
		RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
		RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current.login, "Check saved login"));
		RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current.grade), "Check saved grade"));
		RETURN_IF_FALSE(t.Assert(account->IsActive(), true, "Check saved active status"));
		RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));

		// Check access
		RETURN_IF_FALSE(tryAccess(current, current.grade - 1, true));
		RETURN_IF_FALSE(tryAccess(current, current.grade, true));
		RETURN_IF_FALSE(tryAccess(current, current.grade + 1, false));
	}

	// 11. Logout some connections, try to logon with already logged-on connections and logon back. Delete logged-on
	// connections, then try to access and logon logged-out connections on another accounts and deactivate them after
	// that, then finally delete
	if (testAccounts.size() < 14) [[unlikely]] {
		RETURN_IF_FALSE(t.Assert(false, true, "At least 14 accounts are required to continue the test"));
	}
	const auto findLoggedOnAccount{ [](auto begin, const auto end, int8_t range) -> decltype(begin) {
		for (auto current{ begin }; current != end; ++current) {
			if (current->isLoggedOn) {
				return current;
			}

			if (range-- <= 0) {
				break;
			}
		}

		return end;
	} };

	for (size_t index{ invalidAccountsSize }; index < testAccounts.size(); index += 7) {
		if (index + 7 >= testAccounts.size()) {
			break;
		}

		auto firstAccountIt{ findLoggedOnAccount(
			testAccounts.begin() + static_cast<int64_t>(index), testAccounts.end(), 7) };
		if (firstAccountIt == testAccounts.end()) {
			RETURN_IF_FALSE(t.Assert(false, true, "No logged-on accounts found to continue the test"));
		}
		auto secondAccountIt{ findLoggedOnAccount(std::next(firstAccountIt), testAccounts.end(), 7) };
		if (secondAccountIt == testAccounts.end()) {
			RETURN_IF_FALSE(t.Assert(false, true, "Only one logged-on account found to continue the test"));
		}

		auto& firstAccount{ *firstAccountIt };
		auto& secondAccount{ *secondAccountIt };

		// Logout first account
		mod.LogoutConnection(firstAccount.connection);
		firstAccount.isLoggedOn = false;
		firstAccount.lastActivity = Timer{};
		++firstAccount.logsCount;
		RETURN_IF_FALSE(t.Assert(
			checkAccountLogs(firstAccount, firstAccount.logsCount - 1,
				std::format("Logout at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", firstAccount.connection)),
			true, std::format("Account {} logout from connection {}", firstAccount.login, firstAccount.connection)));
		--activeConnections;
		RETURN_IF_FALSE(t.Assert(
			mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

		mod.LogoutConnection(firstAccount.connection);

		{
			auto account{ getSavedAccount.operator()<A>(firstAccount.login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), firstAccount.login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(firstAccount.grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), true, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}

		// Try to logon first account's connection with first account
		RETURN_IF_FALSE(
			t.Assert(mod.LogonConnection(secondAccount.connection, firstAccount.login, firstAccount.password, error),
				false, "Logon already logged-on connection"));
		RETURN_IF_FALSE(t.Assert(error,
			std::format("Connection is already logged-on with another account", secondAccount.connection),
			"Check error message"));
		error.clear();
		firstAccount.lastActivity = Timer{};
		++firstAccount.logsCount;
		RETURN_IF_FALSE(checkAccountLogs(firstAccount, firstAccount.logsCount - 1,
			std::format("Failed logon attempt at XXXX-XX-XX XX:XX:XX.XXXXXXXXX to already logged-on by another account "
						"connection {}",
				secondAccount.connection)));

		// Check access
		RETURN_IF_FALSE(tryAccess(firstAccount, firstAccount.grade - 1, false));
		RETURN_IF_FALSE(tryAccess(firstAccount, firstAccount.grade, false));
		RETURN_IF_FALSE(tryAccess(firstAccount, firstAccount.grade + 1, false));

		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade - 1, secondAccount.isLoggedOn));
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade, secondAccount.isLoggedOn));
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade + 1, false));

		// Logon back
		RETURN_IF_FALSE(
			t.Assert(mod.LogonConnection(firstAccount.connection, firstAccount.login, firstAccount.password, error),
				true, std::format("Logon connection {} back", firstAccount.connection)));
		RETURN_IF_FALSE(t.Assert(error, "", "Check empty error message for successful logon"));
		firstAccount.isLoggedOn = true;
		firstAccount.lastActivity = Timer{};
		++firstAccount.logsCount;
		RETURN_IF_FALSE(checkAccountLogs(firstAccount, firstAccount.logsCount - 1,
			std::format("Logon at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", firstAccount.connection)));
		++activeConnections;
		RETURN_IF_FALSE(t.Assert(
			mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

		RETURN_IF_FALSE(tryAccess(firstAccount, firstAccount.grade - 1, true));
		RETURN_IF_FALSE(tryAccess(firstAccount, firstAccount.grade, true));
		RETURN_IF_FALSE(tryAccess(firstAccount, firstAccount.grade + 1, false));

		// Delete first account
		mod.DeleteAccount(firstAccount.login);
		firstAccount.lastActivity = Timer{};
		firstAccount.isActivated = false;
		firstAccount.isLoggedOn = false;
		firstAccount.isDeleted = true;
		firstAccount.logsCount += 2;
		--registeredAccounts;
		RETURN_IF_FALSE(t.Assert(
			mod.GetRegisteredAccountsSize(), registeredAccounts, "Check registered accounts size after deletion"));
		RETURN_IF_FALSE(checkAccountLogs(firstAccount, firstAccount.logsCount - 2,
			std::format(
				"Logout due to deletion at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", firstAccount.connection)));
		RETURN_IF_FALSE(checkAccountLogs(firstAccount, firstAccount.logsCount - 1,
			"Marked as uninitialized and deactivated at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));
		--activeConnections;
		RETURN_IF_FALSE(t.Assert(
			mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

		{
			auto account{ getSavedAccount.operator()<A>(firstAccount.login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), firstAccount.login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(firstAccount.grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), false, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), false, "Check saved initialized status"));
		}

		// Logout second account
		mod.LogoutConnection(secondAccount.connection);
		secondAccount.isLoggedOn = false;
		secondAccount.lastActivity = Timer{};
		++secondAccount.logsCount;
		RETURN_IF_FALSE(t.Assert(
			checkAccountLogs(secondAccount, secondAccount.logsCount - 1,
				std::format("Logout at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", secondAccount.connection)),
			true, std::format("Account {} logout from connection {}", secondAccount.login, secondAccount.connection)));
		--activeConnections;
		RETURN_IF_FALSE(t.Assert(
			mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

		// Logon first account's connection with second account login
		RETURN_IF_FALSE(t.Assert(
			mod.LogonConnection(firstAccount.connection, secondAccount.login, secondAccount.password, error), true,
			std::format("Logon connection {} with account login: '{}'", firstAccount.connection, secondAccount.login)));
		RETURN_IF_FALSE(t.Assert(error, "", "Check empty error message for successful logon"));
		secondAccount.isLoggedOn = true;
		secondAccount.lastActivity = Timer{};
		++secondAccount.logsCount;
		RETURN_IF_FALSE(checkAccountLogs(secondAccount, secondAccount.logsCount - 1,
			std::format("Logon at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", firstAccount.connection)));
		++activeConnections;
		RETURN_IF_FALSE(t.Assert(
			mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

		// Check access
		std::swap(firstAccount.connection, secondAccount.connection);
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade - 1, true));
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade, true));
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade + 1, false));

		// Deactivate second account
		RETURN_IF_FALSE(t.Assert(mod.SetAccountActivatedState(secondAccount.login, false), true, "Deactivate account"));
		secondAccount.isActivated = false;
		secondAccount.isLoggedOn = false;
		secondAccount.lastActivity = Timer{};
		secondAccount.logsCount += 2;
		RETURN_IF_FALSE(checkAccountLogs(secondAccount, secondAccount.logsCount - 2,
			std::format("Logout due to deactivation at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}",
				secondAccount.connection)));
		RETURN_IF_FALSE(checkAccountLogs(secondAccount, secondAccount.logsCount - 1,
			"Activation state is changed to false at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));
		--activeConnections;
		RETURN_IF_FALSE(t.Assert(
			mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

		// Check access
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade - 1, false));
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade, false));
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade + 1, false));

		{
			auto account{ getSavedAccount.operator()<A>(secondAccount.login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), secondAccount.login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(secondAccount.grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), false, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), true, "Check saved initialized status"));
		}

		// Delete second account
		mod.DeleteAccount(secondAccount.login);
		secondAccount.lastActivity = Timer{};
		secondAccount.isDeleted = true;
		++secondAccount.logsCount;
		--registeredAccounts;
		RETURN_IF_FALSE(t.Assert(
			mod.GetRegisteredAccountsSize(), registeredAccounts, "Check registered accounts size after deletion"));
		RETURN_IF_FALSE(checkAccountLogs(secondAccount, secondAccount.logsCount - 1,
			"Marked as uninitialized and deactivated at XXXX-XX-XX XX:XX:XX.XXXXXXXXX"));

		// Check access
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade - 1, false));
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade, false));
		RETURN_IF_FALSE(tryAccess(secondAccount, secondAccount.grade + 1, false));

		{
			auto account{ getSavedAccount.operator()<A>(secondAccount.login) };
			RETURN_IF_FALSE((account.get() != nullptr));
			RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), emptyTimer, "Check saved blocked till"));
			RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), secondAccount.login, "Check saved login"));
			RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(secondAccount.grade), "Check saved grade"));
			RETURN_IF_FALSE(t.Assert(account->IsActive(), false, "Check saved active status"));
			RETURN_IF_FALSE(t.Assert(account->IsInitialized(), false, "Check saved initialized status"));
		}
	}

	// 12. Try to set blocked till to some accounts (future/same time again/forward/back), check logs and saved binary
	// data
	if (testAccounts.size() < 26) [[unlikely]] {
		RETURN_IF_FALSE(t.Assert(false, true, "At least 26 accounts are required to continue the test"));
	}

	const auto blockedTill{ Timer{} + Timer::Duration::CreateSeconds(600) };
	const auto blockedTillP1{ blockedTill + Timer::Duration::CreateSeconds(1) };
	const auto blockedTillP2{ blockedTill + Timer::Duration::CreateSeconds(2) };
	for (size_t index{}; index < testAccounts.size(); index += 25) {
		auto* current{ &testAccounts[index] };
		RETURN_IF_FALSE(
			setBlockedTill(*current, blockedTillP1, getBlockSpecificResult(current->blockedTill, blockedTillP1)));
		RETURN_IF_FALSE(setBlockedTill(*current, blockedTillP1, BlockTillResult::BlockFailed));
		RETURN_IF_FALSE(setBlockedTill(*current, blockedTillP2, BlockTillResult::BlockIncrease));
		RETURN_IF_FALSE(setBlockedTill(*current, blockedTill, BlockTillResult::BlockDecrease));
	}

	// 13. Stop module and check saved accounts
	mod.Stop();
	RETURN_IF_FALSE(t.Assert(mod.IsStarted(), false, "Check module is not initialized after stopping"));
	RETURN_IF_FALSE(
		t.Assert(mod.GetRegisteredAccountsSize(), 0, "Check registered accounts size is zero after stopping"));
	for (auto& current : testAccounts) {
		if (!current.shouldRegister) {
			continue;
		}

		auto account{ getSavedAccount.operator()<A>(current.login) };
		RETURN_IF_FALSE((account.get() != nullptr));
		RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), current.blockedTill,
			std::format("Check saved blocked till for account {}", current.login)));
		RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current.login, "Check saved login"));
		RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current.grade), "Check saved grade"));
		RETURN_IF_FALSE(t.Assert(account->IsActive(), current.isActivated, "Check saved active status"));
		RETURN_IF_FALSE(t.Assert(account->IsInitialized(), !current.isDeleted, "Check saved initialized status"));

		if (current.isLoggedOn) {
			current.isLoggedOn = false;
			current.lastActivity = Timer{};
			++current.logsCount;
			RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
				std::format(
					"Logout due to module stop at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}", current.connection)));
			--activeConnections;

			// Check access
			RETURN_IF_FALSE(tryAccess(current, current.grade - 1, false));
			RETURN_IF_FALSE(tryAccess(current, current.grade, false));
			RETURN_IF_FALSE(tryAccess(current, current.grade + 1, false));
		}
	}
	if (activeConnections != 0) {
		RETURN_IF_FALSE(t.Assert(false, true, "All active connections should be logged out after module stop"));
	}
	RETURN_IF_FALSE(
		t.Assert(mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

	// 14. Start module and check state of loaded accounts and check missed ones
	RETURN_IF_FALSE(t.Assert(mod.Start(), true, "Start module again"));
	RETURN_IF_FALSE(t.Assert(mod.IsStarted(), true, "Check module is started after starting"));
	RETURN_IF_FALSE(
		t.Assert(mod.GetRegisteredAccountsSize(), registeredAccounts, "Check registered accounts size after starting"));
	for (auto& current : testAccounts) {
		if (!current.shouldRegister) {
			continue;
		}

		auto account{ getSavedAccount.operator()<A>(current.login) };
		RETURN_IF_FALSE((account.get() != nullptr));
		RETURN_IF_FALSE(t.Assert(account->GetBlockedTill(), current.blockedTill,
			std::format("Check saved blocked till for account {}", current.login)));
		RETURN_IF_FALSE(t.Assert<std::string_view>(account->GetLogin(), current.login, "Check saved login"));
		RETURN_IF_FALSE(t.Assert(account->GetGrade(), static_cast<G>(current.grade), "Check saved grade"));
		RETURN_IF_FALSE(t.Assert(account->IsActive(), current.isActivated, "Check saved active status"));
		RETURN_IF_FALSE(t.Assert(account->IsInitialized(), !current.isDeleted, "Check saved initialized status"));

		if (current.isDeleted) {
			continue;
		}

		// Check access
		RETURN_IF_FALSE(tryAccess(current, current.grade - 1, false));
		RETURN_IF_FALSE(tryAccess(current, current.grade, false));
		RETURN_IF_FALSE(tryAccess(current, current.grade + 1, false));

		// Try to logon blocked accounts
		if (!current.blockedTill.Empty()) {
			RETURN_IF_FALSE(t.Assert(mod.LogonConnection(current.connection, current.login, current.password, error),
				false, std::format("Logon connection {}", current.connection)));
			RETURN_IF_FALSE(t.Assert(
				error, std::format("Account is blocked till {}", blockedTill.ToString()), "Check error message"));
			error.clear();

			current.lastActivity = Timer{};
			++current.logsCount;
			RETURN_IF_FALSE(checkAccountLogs(current, current.logsCount - 1,
				std::format("Failed logon attempt at XXXX-XX-XX XX:XX:XX.XXXXXXXXX, connection {}, reason: Account is "
							"blocked till {}",
					current.connection, current.blockedTill.ToString())));
		}
	}
	RETURN_IF_FALSE(
		t.Assert(mod.GetLogonConnectionsSize(), activeConnections, "Expected number of active connections in module"));

	// 15. Check access (* group of tests) for all accounts (has logon connections)
	RETURN_IF_FALSE(t.Assert(checkAccess.operator()<decltype(testAccounts)>(testAccounts.begin(), testAccounts.end()),
		true, "Check access for all accounts (has logon connections)"));

#undef TMP_MSAPI_UNIT_TEST_AUTHORIZATION_CHANGE_NEW_GRADE_COUNTER
#undef TMP_MSAPI_UNIT_TEST_AUTHORIZATION_MODIFY_ACCOUNT_GRADE

	return true;
}

} // namespace UnitTest

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_AUTHORIZATION_INL