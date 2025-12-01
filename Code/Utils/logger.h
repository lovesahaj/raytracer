#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>

namespace Utils {

enum class Level { Debug, Info, Warn, Error, Fatal };

// Helper: Convert Level to UpperCase String
inline std::string levelToString(Level level) {
  switch (level) {
    case Level::Debug:
      return "DEBUG";
    case Level::Info:
      return "INFO";
    case Level::Warn:
      return "WARN";
    case Level::Error:
      return "ERROR";
    case Level::Fatal:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

class LogEvent {
 public:
  LogEvent(std::ostream& out, std::mutex& mtx, Level level, bool active)
      : output(out), lock(mtx), isActive(active), level(level) {}

  ~LogEvent() {
    if (isActive) {
      // 1. Prepare Timestamp
      auto now = std::chrono::system_clock::now();
      auto time = std::chrono::system_clock::to_time_t(now);

      // 2. Lock and Write everything in one go
      std::lock_guard<std::mutex> guard(lock);

      output << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] "
             << "[" << levelToString(level) << "] "
             << (message.empty() ? "" : message);  // Print message first for readability

      // 3. Print the key=value pairs
      output << fields.str() << "\n";
      output.flush();
    }
  }

  // --- Chainable Methods ---

  LogEvent& Str(std::string_view key, std::string_view value) {
    if (isActive) {
      // Format: key=value
      fields << " " << key << "=" << value;
    }
    return *this;
  }

  LogEvent& Int(std::string_view key, int value) {
    if (isActive) {
      fields << " " << key << "=" << value;
    }
    return *this;
  }

  LogEvent& Double(std::string_view key, double value) {
      if (isActive) {
          fields << " " << key << "=" << value;
      }
      return *this;
  }

  LogEvent& Bool(std::string_view key, bool value) {
    if (isActive) {
      fields << " " << key << "=" << (value ? "true" : "false");
    }
    return *this;
  }

  LogEvent& Err(std::string_view errorMsg) {
    if (isActive) {
      fields << " error=\"" << errorMsg << "\"";  // Quote errors in case of spaces
    }
    return *this;
  }

  // Capture the message to print it prominently before fields
  template <typename T>
  LogEvent& Msg(const T& msg) {
    if (isActive) {
      std::stringstream ss;
      ss << msg;
      message = ss.str();
    }
    return *this;
  }

  template <typename T>
  LogEvent& operator<<(const T& msg) {
      if (isActive) {
          std::stringstream ss;
          ss << msg;
          message += ss.str();
      }
      return *this;
  }

 private:
  std::ostream& output;
  std::mutex& lock;
  bool isActive;
  Level level;

  // We separate fields and message so we can print Message *before* fields in the output
  std::stringstream fields;
  std::string message;
};

// ----------------------------------------------------------------------
// Logger Wrapper
// ----------------------------------------------------------------------
class Logger {
 public:
  Logger(Level minLevel = Level::Info) : minLevel(minLevel) {}

  LogEvent Debug() { return createEvent(Level::Debug); }
  LogEvent Info() { return createEvent(Level::Info); }
  LogEvent Warn() { return createEvent(Level::Warn); }
  LogEvent Error() { return createEvent(Level::Error); }
  LogEvent Fatal() { return createEvent(Level::Fatal); }

  void SetLevel(Level level) { minLevel = level; }
  
  // Static instance accessor
  static Logger& instance() {
      static Logger logger;
      return logger;
  }

 private:
  Level minLevel;
  static inline std::mutex outputMutex; // C++17 inline static member

  LogEvent createEvent(Level level) {
    std::ostream& target = (level >= Level::Error) ? std::cerr : std::cout;
    return LogEvent(target, outputMutex, level, level >= minLevel);
  }
};

} // namespace Utils