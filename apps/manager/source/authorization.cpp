/**************************
 * @file        authorization.cpp
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

#include "authorization.h"
#include <algorithm>

Authorization::Authorization()
{
m_users.emplace_back("guest", "guest", Grade::Guest, true);
}

bool Authorization::Register(const int connection, const std::string& login, const std::string& password,
std::string& error)
{
if (login.empty()) {
error = "Login is empty";
return false;
}

if (password.empty()) {
error = "Password is empty";
return false;
}

MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_lock };
if (std::ranges::any_of(m_users,
[&login](const auto& user) { return user.login == login; })) {
error = "User already exists";
return false;
}

m_users.emplace_back(login, password, Grade::User, true);
m_connectionToUserIndex[connection] = m_users.size() - 1;
return true;
}

bool Authorization::Login(const int connection, const std::string& login, const std::string& password,
std::string& error)
{
MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_lock };
const auto it{ std::ranges::find_if(m_users,
[&login](const auto& user) { return user.login == login; }) };
if (it == m_users.end()) {
error = "User is not found";
return false;
}

if (!it->isActive) {
error = "User is not active";
return false;
}

if (it->password != password) {
error = "Wrong password";
return false;
}

m_connectionToUserIndex[connection] = static_cast<size_t>(it - m_users.begin());
return true;
}

void Authorization::Logout(const int connection)
{
MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_lock };
m_connectionToUserIndex.erase(connection);
}

bool Authorization::IsConnectionAuthenticated(const int connection) const
{
MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_lock };
return m_connectionToUserIndex.find(connection) != m_connectionToUserIndex.end();
}

Authorization::Grade Authorization::GetConnectionGrade(const int connection) const
{
MSAPI::Pthread::AtomicLock::ExitGuard guard{ m_lock };
if (const auto it{ m_connectionToUserIndex.find(connection) }; it != m_connectionToUserIndex.end()) {
return m_users[it->second].grade;
}

return Grade::Guest;
}

bool Authorization::HasAccess(const int connection, const Grade requiredGrade) const
{
return static_cast<int8_t>(GetConnectionGrade(connection)) >= static_cast<int8_t>(requiredGrade);
}

std::string_view Authorization::GradeToString(const Grade grade) noexcept
{
switch (grade) {
case Grade::Guest:
return "Guest";
case Grade::Observer:
return "Observer";
case Grade::User:
return "User";
case Grade::Max:
return "Max";
default:
return "Unknown";
}
}
