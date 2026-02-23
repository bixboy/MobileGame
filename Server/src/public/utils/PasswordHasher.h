#pragma once
#include <string>
#include <sodium.h>


namespace MMO::Utils
{
    class PasswordHasher
    {
    public:
        // Hash un mot de passe
        [[nodiscard]] static std::string Hash(const std::string& password)
        {
            char hashed[crypto_pwhash_STRBYTES];

            if (crypto_pwhash_str(hashed, password.c_str(), password.length(),
                crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
            {
                return "";
            }

            return std::string(hashed);
        }

        // Verifie un mot de passe
        [[nodiscard]] static bool Verify(const std::string& password, const std::string& storedHash)
        {
            return crypto_pwhash_str_verify(storedHash.c_str(), password.c_str(), password.length()) == 0;
        }
    };
}
