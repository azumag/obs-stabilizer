#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <map>

#include "core/motion_classifier.hpp"

using AdaptiveStabilization::MotionType;

namespace VideoDataset {

struct VideoSample {
    std::string filepath;
    MotionType ground_truth_type;
    uint32_t start_frame = 0;
    uint32_t end_frame = 0;
    bool is_valid = true;
};

struct DatasetStats {
    int total_samples = 0;
    std::map<std::string, int> samples_per_category;
    std::map<std::string, std::string> error_messages;
};

class DatasetManager {
public:
    explicit DatasetManager(const std::string& dataset_root);
    ~DatasetManager() = default;

    bool create_dataset_structure();
    bool add_sample(const std::string& category, const std::string& filepath);
    bool label_sample(const std::string& category, const std::string& filepath, 
                     const std::string& ground_truth);
    
    std::vector<VideoSample> get_samples(const std::string& category = "") const;
    std::vector<VideoSample> get_labeled_samples() const;
    DatasetStats get_stats() const;
    bool save_labels(const std::string& labels_path) const;
    bool load_labels(const std::string& labels_path);

    static std::string get_category_name(MotionType type);
    static MotionType parse_category_name(const std::string& category);

private:
    std::string dataset_root_;
    std::vector<VideoSample> samples_;
    std::map<std::string, std::string> labels_;
    
    bool validate_file(const std::string& filepath) const;
};

} // namespace VideoDataset
