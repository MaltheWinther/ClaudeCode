#pragma once

#include <string>

// Loads and saves the local player username from ~/.rppi_credentials (JSON).
// Validation: 1–12 alphanumeric characters [A-Za-z0-9].

class Credentials {
public:
    static std::string load();
    static void        save(const std::string& username);
    static bool        isValid(const std::string& username);
    static std::string invalidReason(const std::string& username);

private:
    static std::string path();   // ~/.rppi_credentials
};
