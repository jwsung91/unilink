#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <mutex>
#include <atomic>

namespace unilink {
namespace common {

/**
 * @brief Log rotation configuration
 */
struct LogRotationConfig {
    size_t max_file_size_bytes = 10 * 1024 * 1024;  // 10MB default
    size_t max_files = 10;                          // Keep 10 files max
    bool enable_compression = false;                // Future feature
    std::string file_pattern = "{name}.{index}.log"; // File naming pattern
    
    LogRotationConfig() = default;
    
    LogRotationConfig(size_t max_size, size_t max_count) 
        : max_file_size_bytes(max_size), max_files(max_count) {}
};

/**
 * @brief Log rotation manager
 * 
 * Handles log file rotation based on size and automatic cleanup
 * of old log files based on count limits.
 */
class LogRotation {
public:
    /**
     * @brief Constructor
     * @param config Rotation configuration
     */
    explicit LogRotation(const LogRotationConfig& config = LogRotationConfig{});
    
    /**
     * @brief Check if current log file needs rotation
     * @param filepath Current log file path
     * @return true if rotation is needed
     */
    bool should_rotate(const std::string& filepath) const;
    
    /**
     * @brief Perform log rotation
     * @param filepath Current log file path
     * @return New log file path after rotation
     */
    std::string rotate(const std::string& filepath);
    
    /**
     * @brief Clean up old log files
     * @param base_filepath Base log file path (without index)
     */
    void cleanup_old_files(const std::string& base_filepath);
    
    /**
     * @brief Get next available log file path
     * @param base_filepath Base log file path
     * @return Next available file path
     */
    std::string get_next_file_path(const std::string& base_filepath) const;
    
    /**
     * @brief Update rotation configuration
     * @param config New configuration
     */
    void update_config(const LogRotationConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const LogRotationConfig& get_config() const { return config_; }
    
    /**
     * @brief Get file size in bytes
     * @param filepath File path to check
     * @return File size in bytes, 0 if file doesn't exist
     */
    static size_t get_file_size(const std::string& filepath);
    
    /**
     * @brief Get all log files matching pattern
     * @param base_filepath Base log file path
     * @return Vector of matching log file paths, sorted by modification time
     */
    static std::vector<std::string> get_log_files(const std::string& base_filepath);

private:
    LogRotationConfig config_;
    mutable std::mutex mutex_;
    
    /**
     * @brief Extract base filename without extension
     * @param filepath Full file path
     * @return Base filename without extension
     */
    std::string get_base_filename(const std::string& filepath) const;
    
    /**
     * @brief Extract directory path
     * @param filepath Full file path
     * @return Directory path
     */
    std::string get_directory(const std::string& filepath) const;
    
    /**
     * @brief Get file index from filename
     * @param filename Filename to parse
     * @return File index, -1 if not found
     */
    int get_file_index(const std::string& filename) const;
    
    /**
     * @brief Generate filename with index
     * @param base_name Base filename
     * @param index File index
     * @return Filename with index
     */
    std::string generate_filename(const std::string& base_name, int index) const;
    
    /**
     * @brief Sort files by modification time (newest first)
     * @param files Vector of file paths to sort
     */
    void sort_files_by_time(std::vector<std::string>& files) const;
};

} // namespace common
} // namespace unilink
