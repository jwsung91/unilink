#include "log_rotation.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>

namespace unilink {
namespace common {

LogRotation::LogRotation(const LogRotationConfig& config) 
    : config_(config) {
}

bool LogRotation::should_rotate(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!std::filesystem::exists(filepath)) {
        return false;
    }
    
    size_t file_size = get_file_size(filepath);
    return file_size >= config_.max_file_size_bytes;
}

std::string LogRotation::rotate(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!std::filesystem::exists(filepath)) {
        return filepath; // File doesn't exist, no rotation needed
    }
    
    // Get the next available file path
    std::string new_filepath = get_next_file_path(filepath);
    
    try {
        // Rename current file to new filepath
        std::filesystem::rename(filepath, new_filepath);
        
        // Clean up old files
        cleanup_old_files(filepath);
        
        return filepath; // Return original path for new log file
    } catch (const std::filesystem::filesystem_error& e) {
        // If rename fails, return original path
        return filepath;
    }
}

void LogRotation::cleanup_old_files(const std::string& base_filepath) {
    try {
        std::vector<std::string> log_files = get_log_files(base_filepath);
        
        // Sort by modification time (newest first)
        sort_files_by_time(log_files);
        
        // Remove files beyond the limit
        if (log_files.size() > config_.max_files) {
            for (size_t i = config_.max_files; i < log_files.size(); ++i) {
                try {
                    std::filesystem::remove(log_files[i]);
                } catch (const std::filesystem::filesystem_error& e) {
                    // Ignore removal errors
                }
            }
        }
    } catch (const std::exception& e) {
        // Ignore cleanup errors
    }
}

std::string LogRotation::get_next_file_path(const std::string& base_filepath) const {
    std::string base_name = get_base_filename(base_filepath);
    std::string directory = get_directory(base_filepath);
    
    // Find the highest index
    int max_index = -1;
    std::vector<std::string> log_files = get_log_files(base_filepath);
    
    for (const auto& file : log_files) {
        std::string filename = std::filesystem::path(file).filename().string();
        int index = get_file_index(filename);
        if (index > max_index) {
            max_index = index;
        }
    }
    
    // Generate next filename
    int next_index = max_index + 1;
    std::string next_filename = generate_filename(base_name, next_index);
    
    return directory + "/" + next_filename;
}

void LogRotation::update_config(const LogRotationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

size_t LogRotation::get_file_size(const std::string& filepath) {
    try {
        if (std::filesystem::exists(filepath)) {
            return std::filesystem::file_size(filepath);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // Return 0 if file doesn't exist or can't be accessed
    }
    return 0;
}

std::vector<std::string> LogRotation::get_log_files(const std::string& base_filepath) {
    std::vector<std::string> log_files;
    
    try {
        std::string base_name = std::filesystem::path(base_filepath).stem().string();
        std::string directory = std::filesystem::path(base_filepath).parent_path().string();
        
        if (directory.empty()) {
            directory = ".";
        }
        
        // Create regex pattern to match log files
        std::regex pattern(base_name + "\\.\\d+\\.log");
        
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (std::regex_match(filename, pattern)) {
                    log_files.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // Return empty vector if directory access fails
    }
    
    return log_files;
}

std::string LogRotation::get_base_filename(const std::string& filepath) const {
    std::filesystem::path path(filepath);
    return path.stem().string();
}

std::string LogRotation::get_directory(const std::string& filepath) const {
    std::filesystem::path path(filepath);
    std::string dir = path.parent_path().string();
    return dir.empty() ? "." : dir;
}

int LogRotation::get_file_index(const std::string& filename) const {
    // Extract index from filename like "app.1.log"
    std::regex pattern(R"((\d+)\.log$)");
    std::smatch match;
    
    if (std::regex_search(filename, match, pattern)) {
        return std::stoi(match[1].str());
    }
    
    return -1;
}

std::string LogRotation::generate_filename(const std::string& base_name, int index) const {
    std::ostringstream oss;
    oss << base_name << "." << index << ".log";
    return oss.str();
}

void LogRotation::sort_files_by_time(std::vector<std::string>& files) const {
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
        try {
            auto time_a = std::filesystem::last_write_time(a);
            auto time_b = std::filesystem::last_write_time(b);
            return time_a > time_b; // Newest first
        } catch (const std::filesystem::filesystem_error& e) {
            return false;
        }
    });
}

} // namespace common
} // namespace unilink
