#pragma once
#include <iostream>
#include <string_view>
#include <format>
#include <mutex>

namespace MMO::Logger 
{
    // Mutex global pour eviter les logs entrelaces entre threads
    inline std::mutex s_logMutex;

    // Log d'information (blanc)
    template <typename... Args>
    void Info(std::format_string<Args...> fmt, Args&&... args)
    {
        auto msg = std::format(fmt, std::forward<Args>(args)...);
        std::lock_guard<std::mutex> lock(s_logMutex);
        
        std::cout << "[INFO] " << msg << '\n';
    }

    // Log d'avertissement (jaune)
    template <typename... Args>
    void Warning(std::format_string<Args...> fmt, Args&&... args)
    {
        auto msg = std::format(fmt, std::forward<Args>(args)...);
        std::lock_guard<std::mutex> lock(s_logMutex);
        
        std::cout << "\033[33m[WARN] " << msg << "\033[0m\n";
    }

    // Log d'erreur avec fichier et ligne (rouge)
    template <typename... Args>
    void Error(std::string_view file, int line, std::format_string<Args...> fmt, Args&&... args) 
    {
        auto msg = std::format(fmt, std::forward<Args>(args)...);
        std::lock_guard<std::mutex> lock(s_logMutex);
        
        std::cerr << "\033[31m[ERROR] " << msg << " (" << file << ":" << line << ")\033[0m\n";
    }
}

// Macros de log simplifiees
#define LOG_INFO(...)  MMO::Logger::Info(__VA_ARGS__)
#define LOG_WARN(...)  MMO::Logger::Warning(__VA_ARGS__)
#define LOG_ERROR(...) MMO::Logger::Error(__FILE__, __LINE__, __VA_ARGS__)