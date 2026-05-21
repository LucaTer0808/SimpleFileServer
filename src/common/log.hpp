#include <string_view>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>

#include "log_level.hpp"

namespace SFS {

    /**
     * @brief Writes a formatted log entry to the console!
     * @param level The desired log level. To see which are available, consult src/common/log_level.hpp.
     * @param message The message to log to the console.
     */
    inline void log(SFS::LogLevel level ,std::string_view message) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&t), "%H:%M:%S") 
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "]";

        std::string timestamp = ss.str();
        std::string label;

        switch (level) {
            case SFS::LogLevel::INFO:
                label = "[INFO]";
                break;
            
            case SFS::LogLevel::ERROR:
                label = "[ERROR]";
                break;

            case SFS::LogLevel::WARNING:
                label = "[WARNING]";
                break;
        }

        std::cout << timestamp << label << " " << message << "\n";
    }

    /**
     * @brief Writes a formatted log and incorporates the error number as well as a detailed explanation!
     * @param message The desired message to go along the error description and the code.
     */
    inline void log_errno(std::string_view message) {
        int err = errno;
        std::string error_detail = std::strerror(err);

        std::string full_msg = std::string(message) + " | Error: " + error_detail + " (Code: " + std::to_string(err) + ")";
        SFS::log(SFS::LogLevel::ERROR, full_msg);
    }
}