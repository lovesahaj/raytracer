#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

// -----------------------------------------------------------------------------
// PROGRESS BAR LIBRARY START
// -----------------------------------------------------------------------------

namespace tqdm {

class ProgressBar {
 private:
  unsigned int total_ticks;
  std::atomic<unsigned int> current_ticks;  // Atomic for thread safety
  unsigned int bar_width;
  std::string complete_char;
  std::string incomplete_char;
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  
  std::mutex bar_mutex;  // Mutex to prevent console garbling

  // Smoothing state
  std::chrono::time_point<std::chrono::steady_clock> last_refresh_time;
  unsigned int last_ticks = 0;
  double smoothed_rate = -1.0;          // -1 indicates not yet initialized
  const double smoothing_factor = 0.1;  // 0.1 = Recommended for stability

  // Dynamic iteration monitoring
  unsigned int next_refresh_check = 0;
  unsigned int check_interval = 1;

  std::string format_time(long long seconds) {
    std::string time_str;
    long long h = seconds / 3600;
    long long m = (seconds % 3600) / 60;
    long long s = seconds % 60;

    std::stringstream ss;
    if (h > 0) ss << std::setw(2) << std::setfill('0') << h << ":";
    ss << std::setw(2) << std::setfill('0') << m << ":" << std::setw(2) << std::setfill('0') << s;
    return ss.str();
  }

 public:
  ProgressBar(unsigned int total, unsigned int width = 50, char complete = '=',
              char incomplete = ' ')
      : total_ticks(total), current_ticks(0), bar_width(width) {
    complete_char = complete;
    incomplete_char = incomplete;
    start_time = std::chrono::steady_clock::now();
    last_refresh_time = start_time;
  }

  void update() {
    unsigned int curr = ++current_ticks;

    // 1. Check if we are finished (always print 100%)
    if (curr == total_ticks) {
      std::lock_guard<std::mutex> lock(bar_mutex);
      display();
      return;
    }

    // 2. Smart skip: Don't even check the clock if we aren't due
    if (curr < next_refresh_check) {
      return;
    }

    // 3. Check clock
    auto now = std::chrono::steady_clock::now();
    auto elapsed_duration =
        std::chrono::duration_cast<std::chrono::duration<double>>(now - last_refresh_time);
    double elapsed = elapsed_duration.count();

    if (elapsed < 0.1) {
      // Not enough time passed, increase interval to back off
      check_interval *= 2;
      next_refresh_check = curr + check_interval;
      return;
    }

    // 4. Update display
    std::unique_lock<std::mutex> lock(bar_mutex, std::try_to_lock);
    if (lock.owns_lock()) {
      unsigned int prev_ticks = last_ticks;

      display();  // Updates last_refresh_time, last_ticks

      // Recalculate interval
      // Rate = ticks / seconds
      // We want interval for 0.1 seconds
      double ticks_done = (double)(curr - prev_ticks);
      double rate = (elapsed > 0) ? ticks_done / elapsed : 0.0;

      check_interval = (unsigned int)(rate * 0.1);
      if (check_interval < 1) check_interval = 1;

      next_refresh_check = curr + check_interval;
    }
  }

  void update(unsigned int new_ticks) {
    current_ticks = new_ticks;
    std::unique_lock<std::mutex> lock(bar_mutex, std::try_to_lock);
    if (lock.owns_lock()) {
      display();
    }
  }

  void display() {
    unsigned int curr = current_ticks.load();
    auto now = std::chrono::steady_clock::now();

    // 1. Calculate the 'Instantaneous' rate (since the last refresh)
    auto diff_time = now - last_refresh_time;
    double diff_seconds = std::chrono::duration<double>(diff_time).count();

    // Only update rate if enough time has passed (e.g. > 1ms)
    // This avoids division by zero and jitter from very fast updates
    if (diff_seconds > 0.001) { 
      unsigned int diff_ticks = curr - last_ticks;
      double instant_rate = diff_ticks / diff_seconds;

      // 2. Update the Smoothed Rate (Exponential Moving Average)
      if (smoothed_rate < 0) {
        // First run: just use the instant rate
        smoothed_rate = instant_rate;
      } else {
        // Subsequent runs: Blend new rate with old rate
        smoothed_rate = (smoothing_factor * instant_rate) + ((1.0 - smoothing_factor) * smoothed_rate);
      }

      // 3. Update state for next time
      last_refresh_time = now;
      last_ticks = curr;
    }

    // --- Standard Progress Calculation ---
    float progress = (total_ticks > 0) ? (float)curr / total_ticks : 1.0;
    progress = (progress > 1.0) ? 1.0 : progress;

    int pos = (int)(bar_width * progress);

    // 4. Calculate ETA using the SMOOTHED rate
    long long eta_seconds = 0;
    if (smoothed_rate > 0.0001) {
      eta_seconds = (long long)((total_ticks - curr) / smoothed_rate);
    }

    // Calculate total elapsed for the display string
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

    // --- Print ---
    std::cout << "\r" << std::setw(3) << (int)(progress * 100) << "%|";
    for (unsigned int i = 0; i < bar_width; ++i) {
      if (i < (unsigned int)pos)
        std::cout << complete_char;
      else if (i == (unsigned int)pos)
        std::cout << ">";
      else
        std::cout << incomplete_char;
    }
    std::cout << "| " << curr << "/" << total_ticks;
    
    // Use fixed precision for rate
    std::cout << " [" << format_time(total_elapsed) << "<" << format_time(eta_seconds) << ", "
              << std::fixed << std::setprecision(2) << (smoothed_rate < 0 ? 0.0 : smoothed_rate) << "it/s] " << std::flush;
  }

  void finish() {
    std::lock_guard<std::mutex> lock(bar_mutex);
    display();
    std::cout << std::endl;
  }
};

}  // namespace tqdm