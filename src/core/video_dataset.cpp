#include "core/video_dataset.hpp"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <string>
#include <map>

namespace VideoDataset {

DatasetManager::DatasetManager(const std::string& dataset_root)
    : dataset_root_(dataset_root)
{
}

bool DatasetManager::create_dataset_structure() {
    try {
        std::filesystem::create_directories(dataset_root_);
        std::filesystem::create_directories(dataset_root_ + "/static");
        std::filesystem::create_directories(dataset_root_ + "/slow_motion");
        std::filesystem::create_directories(dataset_root_ + "/fast_motion");
        std::filesystem::create_directories(dataset_root_ + "/camera_shake");
        std::filesystem::create_directories(dataset_root_ + "/pan_zoom");
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DatasetManager::add_sample(const std::string& category, const std::string& filepath) {
    if (category.empty() || filepath.empty()) {
        return false;
    }

    VideoSample sample;
    sample.filepath = filepath;
    sample.ground_truth_type = parse_category_name(category);
    
    samples_.push_back(sample);
    samples_[samples_.size() - 1].is_valid = validate_file(filepath);
    
    return samples_[samples_.size() - 1].is_valid;
}

bool DatasetManager::label_sample(const std::string& category, const std::string& filepath,
                                  const std::string& ground_truth) {
    std::string key = category + "/" + filepath;
    labels_[key] = ground_truth;
    return true;
}

std::vector<VideoSample> DatasetManager::get_samples(const std::string& category) const {
    std::vector<VideoSample> result;
    MotionType target_type = category.empty() ? MotionType::Static : parse_category_name(category);
    
    for (const auto& sample : samples_) {
        if (category.empty() || sample.ground_truth_type == target_type) {
            result.push_back(sample);
        }
    }
    
    return result;
}

std::vector<VideoSample> DatasetManager::get_labeled_samples() const {
    std::vector<VideoSample> result;
    
    for (const auto& sample : samples_) {
        std::string category_name = get_category_name(sample.ground_truth_type);
        std::string key = category_name + "/" + sample.filepath;
        if (labels_.find(key) != labels_.end()) {
            result.push_back(sample);
        }
    }
    
    return result;
}

DatasetStats DatasetManager::get_stats() const {
    DatasetStats stats;
    stats.total_samples = samples_.size();
    
    for (const auto& sample : samples_) {
        std::string category_name = get_category_name(sample.ground_truth_type);
        stats.samples_per_category[category_name]++;
    }
    
    return stats;
}

bool DatasetManager::save_labels(const std::string& labels_path) const {
    try {
        std::ofstream out(labels_path);
        if (!out.is_open()) {
            return false;
        }
        
        for (const auto& [key, label] : labels_) {
            out << key << "," << label << "\n";
        }
        
        out.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DatasetManager::load_labels(const std::string& labels_path) {
    try {
        std::ifstream in(labels_path);
        if (!in.is_open()) {
            return false;
        }
        
        std::string line;
        while (std::getline(in, line)) {
            size_t comma_pos = line.find(',');
            if (comma_pos != std::string::npos) {
                std::string key = line.substr(0, comma_pos);
                std::string label = line.substr(comma_pos + 1);
                labels_[key] = label;
            }
        }
        
        in.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string DatasetManager::get_category_name(MotionType type) {
    switch (type) {
        case MotionType::Static: return "static";
        case MotionType::SlowMotion: return "slow_motion";
        case MotionType::FastMotion: return "fast_motion";
        case MotionType::CameraShake: return "camera_shake";
        case MotionType::PanZoom: return "pan_zoom";
        default: return "unknown";
    }
}

MotionType DatasetManager::parse_category_name(const std::string& category) {
    if (category == "static") return MotionType::Static;
    if (category == "slow_motion") return MotionType::SlowMotion;
    if (category == "fast_motion") return MotionType::FastMotion;
    if (category == "camera_shake") return MotionType::CameraShake;
    if (category == "pan_zoom") return MotionType::PanZoom;
    return MotionType::Static;
}

bool DatasetManager::validate_file(const std::string& filepath) const {
    try {
        return std::filesystem::exists(filepath) && 
               std::filesystem::is_regular_file(filepath);
    } catch (const std::exception& e) {
        return false;
    }
}

} // namespace VideoDataset
