#include "Credentials.hpp"

#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string Credentials::path() {
    const char* home = std::getenv("HOME");
    return std::string(home ? home : ".") + "/.rppi_credentials";
}

std::string Credentials::load() {
    std::ifstream f(path());
    if (!f.is_open()) return {};
    try {
        json j;
        f >> j;
        return j.value("username", "");
    } catch (...) {
        return {};
    }
}

void Credentials::save(const std::string& username) {
    std::ofstream f(path());
    if (f.is_open())
        f << json{{"username", username}}.dump(2);
}

bool Credentials::isValid(const std::string& u) {
    if (u.empty() || u.size() > 12) return false;
    return std::all_of(u.begin(), u.end(), [](unsigned char c) {
        return std::isalnum(c);
    });
}

std::string Credentials::invalidReason(const std::string& u) {
    if (u.empty())     return "Username cannot be empty";
    if (u.size() > 12) return "Username must be 12 characters or fewer";
    return "Only letters and numbers allowed (A-Z, a-z, 0-9)";
}
