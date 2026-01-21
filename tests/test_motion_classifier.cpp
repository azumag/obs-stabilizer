#include <gtest/gtest.h>
#include "core/motion_classifier.hpp"
#include <opencv2/opencv.hpp>
#include <deque>

using namespace AdaptiveStabilization;

class MotionClassifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        classifier_ = std::make_unique<MotionClassifier>(30, 1.0);
    }
    
    void TearDown() override {
        classifier_.reset();
    }
    
    cv::Mat create_transform(double tx, double ty, double angle = 0.0, double scale = 1.0) {
        cv::Mat transform = cv::Mat::eye(2, 3, CV_64F);
        double* ptr = transform.ptr<double>(0);
        
        ptr[0] = scale * std::cos(angle);
        ptr[1] = scale * std::sin(angle);
        ptr[2] = tx;
        ptr[3] = -scale * std::sin(angle);
        ptr[4] = scale * std::cos(angle);
        ptr[5] = ty;
        
        return transform;
    }
    
    void add_transforms(double tx, double ty, int count) {
        for (int i = 0; i < count; ++i) {
            transforms_.push_back(create_transform(tx, ty));
        }
    }
    
    std::unique_ptr<MotionClassifier> classifier_;
    std::deque<cv::Mat> transforms_;
};

TEST_F(MotionClassifierTest, ClassifyStaticMotion) {
    add_transforms(0.1, 0.1, 30);
    
    MotionType type = classifier_->classify(transforms_);
    EXPECT_EQ(type, MotionType::Static);
    
    MotionMetrics metrics = classifier_->get_current_metrics();
    EXPECT_LT(metrics.mean_magnitude, 5.0);
}

TEST_F(MotionClassifierTest, ClassifySlowMotion) {
    for (int i = 0; i < 30; ++i) {
        transforms_.push_back(create_transform(4.0 + (i % 5) * 0.5, 4.0 + (i % 7) * 0.3));
    }
    
    MotionType type = classifier_->classify(transforms_);
    EXPECT_EQ(type, MotionType::SlowMotion);
    
    MotionMetrics metrics = classifier_->get_current_metrics();
    EXPECT_GE(metrics.mean_magnitude, 5.0);
    EXPECT_LT(metrics.mean_magnitude, 20.0);
}

TEST_F(MotionClassifierTest, ClassifyFastMotion) {
    for (int i = 0; i < 30; ++i) {
        transforms_.push_back(create_transform(12.0 + (i % 3) * 2.0, 12.0 + (i % 4) * 1.5));
    }
    
    MotionType type = classifier_->classify(transforms_);
    EXPECT_EQ(type, MotionType::FastMotion);
    
    MotionMetrics metrics = classifier_->get_current_metrics();
    EXPECT_GE(metrics.mean_magnitude, 20.0);
    EXPECT_LT(metrics.mean_magnitude, 50.0);
}

TEST_F(MotionClassifierTest, ClassifyPanZoom) {
    for (int i = 0; i < 30; ++i) {
        transforms_.push_back(create_transform(2.0 + i * 0.2, 0.5 + i * 0.1));
    }
    
    MotionType type = classifier_->classify(transforms_);
    EXPECT_EQ(type, MotionType::PanZoom);
    
    MotionMetrics metrics = classifier_->get_current_metrics();
    EXPECT_GT(metrics.consistency_score, 0.7);
}

TEST_F(MotionClassifierTest, ClassifyCameraShake) {
    for (int i = 0; i < 30; ++i) {
        double jitter = (i % 2 == 0) ? 8.0 : -8.0;
        double jitter2 = (i % 3 == 0) ? 5.0 : 0.0;
        transforms_.push_back(create_transform(jitter + jitter2, jitter - jitter2));
    }
    
    MotionType type = classifier_->classify(transforms_);
    EXPECT_EQ(type, MotionType::CameraShake);
    
    MotionMetrics metrics = classifier_->get_current_metrics();
    EXPECT_GT(metrics.variance_magnitude, 15.0);
}

TEST_F(MotionClassifierTest, CalculateMetricsEmptyTransforms) {
    MotionMetrics metrics = classifier_->calculate_metrics(transforms_);
    EXPECT_EQ(metrics.transform_count, 0);
    EXPECT_EQ(metrics.mean_magnitude, 0.0);
}

TEST_F(MotionClassifierTest, CalculateMetricsSingleTransform) {
    transforms_.push_back(create_transform(1.0, 1.0));
    
    MotionMetrics metrics = classifier_->calculate_metrics(transforms_);
    EXPECT_EQ(metrics.transform_count, 1);
    EXPECT_GT(metrics.mean_magnitude, 0.0);
}

TEST_F(MotionClassifierTest, MotionTypeToString) {
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::Static), "Static");
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::SlowMotion), "Slow Motion");
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::FastMotion), "Fast Motion");
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::CameraShake), "Camera Shake");
    EXPECT_EQ(MotionClassifier::motion_type_to_string(MotionType::PanZoom), "Pan/Zoom");
}

TEST_F(MotionClassifierTest, SetSensitivity) {
    classifier_->set_sensitivity(2.0);
    EXPECT_EQ(classifier_->get_sensitivity(), 2.0);
    
    for (int i = 0; i < 30; ++i) {
        transforms_.push_back(create_transform(4.0 + (i % 5) * 0.5, 4.0 + (i % 7) * 0.3));
    }
    
    MotionType type_normal = classifier_->classify(transforms_);
    
    classifier_->set_sensitivity(0.5);
    transforms_.clear();
    for (int i = 0; i < 30; ++i) {
        transforms_.push_back(create_transform(4.0 + (i % 5) * 0.5, 4.0 + (i % 7) * 0.3));
    }
    
    MotionType type_sensitive = classifier_->classify(transforms_);
    
    EXPECT_EQ(type_normal, type_sensitive);
}

TEST_F(MotionClassifierTest, WindowSizeSmallerThanTransforms) {
    for (int i = 0; i < 50; ++i) {
        transforms_.push_back(create_transform(3.0 + (i % 3) * 0.5, 3.0 + (i % 4) * 0.4));
    }
    
    MotionType type = classifier_->classify(transforms_);
    EXPECT_NE(type, MotionType::Static);
    
    MotionMetrics metrics = classifier_->get_current_metrics();
    EXPECT_EQ(metrics.transform_count, 30);
}
