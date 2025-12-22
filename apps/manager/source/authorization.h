/**************************
 * @file        authorization.h
 * @version     6.0
 * @date        2025-12-22
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

#ifndef MSAPI_MANAGER_AUTHORIZATION_H
#define MSAPI_MANAGER_AUTHORIZATION_H

#include "../../../library/source/help/pthread.hpp"
#include <map>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/**************************
 * @brief Simple authorization module with in-memory users storage.
 */
class Authorization {
public:
enum class Grade : int8_t { Guest, Observer, User, Max };

struct User {
std::string login;
std::string password;
Grade grade{ Grade::Guest };
bool isActive{ true };

User(std::string_view userLogin, std::string_view userPassword, const Grade userGrade, const bool active)
: login{ userLogin }
, password{ userPassword }
, grade{ userGrade }
, isActive{ active }
{
}
};

private:
std::vector<User> m_users;
std::map<int, size_t> m_connectionToUserIndex;
mutable MSAPI::Pthread::AtomicLock m_lock;

public:
Authorization();

/**************************
 * @brief Register a new user with default grade User and set it to connection.
 */
bool Register(int connection, const std::string& login, const std::string& password, std::string& error);

/**************************
 * @brief Login existing user and attach to connection.
 */
bool Login(int connection, const std::string& login, const std::string& password, std::string& error);

/**************************
 * @brief Logout user attached to connection.
 */
void Logout(int connection);

/**************************
 * @return True if connection is authenticated.
 */
bool IsConnectionAuthenticated(int connection) const;

/**************************
 * @return Grade of user attached to connection or Guest if not authenticated.
 */
Grade GetConnectionGrade(int connection) const;

/**************************
 * @return True if user attached to connection has required grade.
 */
bool HasAccess(int connection, Grade requiredGrade) const;

/**************************
 * @return Readable grade value.
 */
static std::string_view GradeToString(Grade grade) noexcept;
};

#endif //* MSAPI_MANAGER_AUTHORIZATION_H
