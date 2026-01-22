#include <gtest/gtest.h>
#include "../src/core/motion_classifier.hpp"
#include "test_data_generator.hpp"

using namespace AdaptiveStabilization;

class MotionClassifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        classifier = std::make_unique<MotionClassifier>();
    }
    
    void TearDown() override {
        classifier.reset();
    }
    
    std::unique_ptr<MotionClassifier> classifier;
};

TEST_F(MotionClassifierTest, Initialization) {
    EXPECT_TRUE(classifier != nullptr);
    EXPECT_EQ(classifier->get_current_type(), MotionType::Static);
    EXPECT_EQ(classifier->get_sensitivity(), 1.0);
}

TEST_F(MotionClassifierTest, ClassifyStaticMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        cv::Mat transform = cv::Mat::eye(3, 3, CV_64F);
        transforms.push_back(transform);
    }
    
    MotionType type = classifier->classify(transforms);
    EXPECT_EQ(type, MotionType::Static);
}

TEST_F(MotionClassifierTest, ClassifyHorizontalMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dx = 10.0 * sin(2.0 * M_PI * i / 30);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, 0);
        transforms.push_back(transform);
    }
    
    MotionType type = classifier->classify(transforms);
    EXPECT_TRUE(type == MotionType::SlowMotion || type == MotionType::PanZoom);
}

TEST_F(MotionClassifierTest, ClassifyVerticalMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dy = 10.0 * sin(2.0 * M_PI * i / 30);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, 0, 0, 1, dy);
        transforms.push_back(transform);
    }
    
    MotionType type = classifier->classify(transforms);
    EXPECT_TRUE(type == MotionType::SlowMotion || type == MotionType::PanZoom);
}

TEST_F(MotionClassifierTest, ClassifyFastMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dx = 30.0 * sin(2.0 * M_PI * i / 30);
        double dy = 30.0 * sin(2.0 * M_PI * i / 30);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, dy);
        transforms.push_back(transform);
    }
    
    MotionType type = classifier->classify(transforms);
    EXPECT_TRUE(type == MotionType::FastMotion || type == MotionType::PanZoom);
}

TEST_F(MotionClassifierTest, ClassifyCameraShake) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dx = 8.0 * (i % 2 == 0 ? 1.0 : -1.0);
        double dy = 8.0 * (i % 3 == 0 ? 1.0 : -1.0);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, dy);
        transforms.push_back(transform);
    }
    
    MotionType type = classifier->classify(transforms);
    MotionMetrics metrics = classifier->get_current_metrics();
    
    EXPECT_NE(type, MotionType::Static);
}

TEST_F(MotionClassifierTest, CalculateMetrics) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dx = 5.0 * sin(2.0 * M_PI * i / 30);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, 0);
        transforms.push_back(transform);
    }
    
    MotionMetrics metrics = classifier->calculate_metrics(transforms);
    
    EXPECT_GE(metrics.mean_magnitude, 0.0);
    EXPECT_GE(metrics.variance_magnitude, 0.0);
    EXPECT_GE(metrics.directional_variance, 0.0);
    EXPECT_GE(metrics.high_frequency_ratio, 0.0);
    EXPECT_GE(metrics.consistency_score, 0.0);
    EXPECT_LE(metrics.consistency_score, 1.0);
    EXPECT_EQ(metrics.transform_count, 30);
}

TEST_F(MotionClassifierTest, CalculateMetricsForStatic) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        cv::Mat transform = cv::Mat::eye(3, 3, CV_64F);
        transforms.push_back(transform);
    }
    
    MotionMetrics metrics = classifier->calculate_metrics(transforms);
    
    EXPECT_EQ(metrics.mean_magnitude, 0.0);
    EXPECT_EQ(metrics.variance_magnitude, 0.0);
}

TEST_F(MotionClassifierTest, MotionTypeToString) {
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::Static), "Static");
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::SlowMotion), "Slow Motion");
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::FastMotion), "Fast Motion");
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::CameraShake), "Camera Shake");
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::PanZoom), "Pan/Zoom");
}

TEST_F(MotionClassifierTest, SensitivityAdjustment) {
    classifier->set_sensitivity(0.5);
    EXPECT_EQ(classifier->get_sensitivity(), 0.5);
    
    classifier->set_sensitivity(1.5);
    EXPECT_EQ(classifier->get_sensitivity(), 1.5);
    
    classifier->set_sensitivity(2.0);
    EXPECT_EQ(classifier->get_sensitivity(), 2.0);
}

TEST_F(MotionClassifierTest, CustomWindowSize) {
    MotionClassifier custom_classifier(20);
    
    std::deque<cv::Mat> transforms;
    for (int i = 0; i < 20; i++) {
        cv::Mat transform = cv::Mat::eye(3, 3, CV_64F);
        transforms.push_back(transform);
    }
    
    MotionType type = custom_classifier.classify(transforms);
    EXPECT_EQ(type, MotionType::Static);
}

TEST_F(MotionClassifierTest, EmptyTransforms) {
    std::deque<cv::Mat> transforms;
    
    MotionType type = classifier->classify(transforms);
    EXPECT_EQ(type, MotionType::Static);
}

TEST_F(MotionClassifierTest, SingleTransform) {
    std::deque<cv::Mat> transforms;
    cv::Mat transform = cv::Mat::eye(3, 3, CV_64F);
    transforms.push_back(transform);
    
    MotionType type = classifier->classify(transforms);
    EXPECT_EQ(type, MotionType::Static);
}

TEST_F(MotionClassifierTest, ConsistentDirectionalMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dx = 5.0;
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, 0);
        transforms.push_back(transform);
    }
    
    MotionMetrics metrics = classifier->calculate_metrics(transforms);
    EXPECT_GT(metrics.consistency_score, 0.8);
}

TEST_F(MotionClassifierTest, InconsistentMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dx = 5.0 * (rand() % 100 - 50) / 50.0;
        double dy = 5.0 * (rand() % 100 - 50) / 50.0;
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, dy);
        transforms.push_back(transform);
    }
    
    MotionMetrics metrics = classifier->calculate_metrics(transforms);
    EXPECT_LT(metrics.consistency_score, 0.5);
}

TEST_F(MotionClassifierTest, HighFrequencyMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dx = 2.0 * sin(2.0 * M_PI * 5 * i / 30);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, 0);
        transforms.push_back(transform);
    }
    
    MotionMetrics metrics = classifier->calculate_metrics(transforms);
    EXPECT_GT(metrics.high_frequency_ratio, 0.3);
}

TEST_F(MotionClassifierTest, LowFrequencyMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double dx = 10.0 * sin(2.0 * M_PI * i / 30);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, 0);
        transforms.push_back(transform);
    }
    
    MotionMetrics metrics = classifier->calculate_metrics(transforms);
    EXPECT_LT(metrics.high_frequency_ratio, 0.3);
}

TEST_F(MotionClassifierTest, RotationMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double angle = 1.0 * sin(2.0 * M_PI * i / 30);
        cv::Point2f center(320, 240);
        cv::Mat rot_mat = cv::getRotationMatrix2D(center, angle, 1.0);
        transforms.push_back(rot_mat);
    }
    
    MotionType type = classifier->classify(transforms);
    EXPECT_NE(type, MotionType::Static);
}

TEST_F(MotionClassifierTest, ZoomMotion) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 30; i++) {
        double scale = 1.0 + 0.05 * sin(2.0 * M_PI * i / 30);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << scale, 0, 0, 0, scale, 0);
        transforms.push_back(transform);
    }
    
    MotionType type = classifier->classify(transforms);
    MotionMetrics metrics = classifier->get_current_metrics();
    
    EXPECT_NE(type, MotionType::Static);
}

TEST_F(MotionClassifierTest, LargeMotionSequence) {
    std::deque<cv::Mat> transforms;
    
    for (int i = 0; i < 100; i++) {
        double dx = 8.0 * sin(2.0 * M_PI * i / 100);
        cv::Mat transform = (cv::Mat_<double>(2, 3) << 1, 0, dx, 0, 1, 0);
        transforms.push_back(transform);
    }
    
    MotionType type = classifier->classify(transforms);
    MotionMetrics metrics = classifier->get_current_metrics();
    
    EXPECT_EQ(metrics.transform_count, std::min(size_t(30), transforms.size()));
    EXPECT_NE(type, MotionType::Static);
}
