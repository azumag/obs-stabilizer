# MotionClassifier Threshold Tuning Design

## Overview
This document describes the system design for fine-tuning MotionClassifier thresholds using real-world video data to achieve >90% classification accuracy.

## System Architecture

### Components

```
┌─────────────────────────────────────────────────────────────┐
│                    Threshold Tuning System                   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌────────────────┐      ┌────────────────┐                 │
│  │ Video Dataset  │─────>│ Classification  │                 │
│  │   Collector    │      │   Engine        │                 │
│  └────────────────┘      └────────┬───────┘                 │
│                                   │                          │
│  ┌────────────────┐      ┌───────▼───────┐                 │
│  │  Ground Truth  │      │   Accuracy     │                 │
│  │    Manager     │      │   Analyzer     │                 │
│  └────────────────┘      └───────┬───────┘                 │
│                                   │                          │
│  ┌────────────────┐      ┌───────▼───────┐                 │
│  │   Threshold    │<─────│   Threshold    │                 │
│  │   Optimizer    │      │   Searcher     │                 │
│  └────────┬───────┘      └───────────────┘                 │
│           │                                                  │
│           ▼                                                  │
│  ┌────────────────┐                                         │
│  │  Test Suite    │                                         │
│  │   Updater      │                                         │
│  └────────────────┘                                         │
└─────────────────────────────────────────────────────────────┘
```

### Component Descriptions

#### 1. Video Dataset Collector
**Purpose**: Collect and organize video samples with ground truth labels

**Responsibilities**:
- Create standardized video dataset structure
- Accept video uploads from various sources
- Store video files with metadata
- Maintain ground truth labels for each sample

**Data Structure**:
```
test_videos/
├── static/
│   ├── sample001.mp4 (label: Static)
│   ├── sample002.mp4 (label: Static)
│   └── ...
├── slow_motion/
├── fast_motion/
├── camera_shake/
└── pan_zoom/
```

**API**:
```cpp
class VideoDataset {
public:
    bool addSample(const std::string& videoPath, MotionType groundTruth);
    std::vector<VideoSample> getSamples(MotionType type) const;
    size_t getSampleCount(MotionType type) const;
    size_t getTotalSampleCount() const;
    bool loadDataset(const std::string& datasetPath);
    bool saveDataset(const std::string& datasetPath) const;
};
```

#### 2. Classification Engine
**Purpose**: Batch process video samples through MotionClassifier

**Responsibilities**:
- Process entire video dataset
- Extract motion metrics for each sample
- Generate predictions using current thresholds
- Store classification results with confidence scores

**Data Structure**:
```cpp
struct ClassificationResult {
    std::string sampleId;
    MotionType groundTruth;
    MotionType predicted;
    double confidence;
    std::map<std::string, double> metrics;  // All motion metrics
};
```

**API**:
```cpp
class ClassificationEngine {
public:
    std::vector<ClassificationResult> classifyDataset(
        const VideoDataset& dataset,
        const ThresholdConfig& thresholds
    );

    ClassificationResult classifySample(
        const VideoSample& sample,
        const ThresholdConfig& thresholds
    );
};
```

#### 3. Accuracy Analyzer
**Purpose**: Calculate and visualize classification accuracy metrics

**Responsibilities**:
- Calculate overall accuracy
- Calculate per-motion-type accuracy
- Generate confusion matrix
- Identify misclassification patterns
- Generate accuracy reports

**Data Structure**:
```cpp
struct AccuracyReport {
    double overallAccuracy;
    std::map<MotionType, double> typeAccuracy;
    std::map<MotionType, std::map<MotionType, int>> confusionMatrix;
    std::vector<ClassificationResult> misclassified;
};
```

**API**:
```cpp
class AccuracyAnalyzer {
public:
    AccuracyReport analyze(
        const std::vector<ClassificationResult>& results
    );

    void printReport(const AccuracyReport& report);
    void saveConfusionMatrix(const AccuracyReport& report, const std::string& path);
};
```

#### 4. Threshold Searcher
**Purpose**: Systematically search for optimal threshold values

**Search Strategies**:

**Option A: Grid Search**
- Define search ranges for each threshold
- Iterate through all combinations
- Time-consuming but thorough

**Option B: Genetic Algorithm**
- Population of threshold configurations
- Selection, crossover, mutation
- Faster convergence to good solutions

**Option C: Gradient-Based Optimization**
- Use accuracy as objective function
- Calculate gradients (if differentiable)
- Fastest but may find local optima

**Recommended**: Start with grid search for comprehensive exploration, then refine with genetic algorithm.

**API**:
```cpp
struct ThresholdConfig {
    double staticThreshold;
    double slowThreshold;
    double fastThreshold;
    double varianceThreshold;
    double highFreqThreshold;
    double consistencyThreshold;
};

class ThresholdSearcher {
public:
    ThresholdConfig gridSearch(
        const VideoDataset& dataset,
        const ThresholdRange& ranges,
        size_t steps = 10
    );

    ThresholdConfig geneticOptimize(
        const VideoDataset& dataset,
        const ThresholdRange& ranges,
        size_t generations = 50,
        size_t populationSize = 100
    );

    double evaluateAccuracy(
        const ThresholdConfig& thresholds,
        const VideoDataset& dataset
    );
};
```

#### 5. Threshold Optimizer
**Purpose**: Coordinate threshold tuning process and manage iterations

**Responsibilities**:
- Execute search strategy
- Monitor convergence
- Track best configuration
- Log tuning history

**API**:
```cpp
class ThresholdOptimizer {
public:
    ThresholdConfig optimize(
        const VideoDataset& dataset,
        OptimizationStrategy strategy = OptimizationStrategy::GRID_SEARCH
    );

    void setLogger(Logger* logger);
    void setProgressCallback(ProgressCallback callback);
};
```

#### 6. Test Suite Updater
**Purpose**: Update failing tests with tuned thresholds

**Responsibilities**:
- Identify tests that need updates
- Generate appropriate test data matching tuned thresholds
- Update test assertions
- Verify all tests pass

**API**:
```cpp
class TestSuiteUpdater {
public:
    bool updateTests(const ThresholdConfig& tunedThresholds);
    bool verifyAllTestsPass();
};
```

## Threshold Tuning Process

### Step 1: Data Collection
1. Create test_videos/ directory structure
2. Collect 10+ video samples per motion type (50+ total)
3. Manually label each sample with ground truth
4. Store samples in appropriate directories

### Step 2: Baseline Evaluation
1. Load video dataset
2. Classify all samples with current thresholds
3. Calculate baseline accuracy
4. Generate confusion matrix
5. Identify problem areas

### Step 3: Threshold Search
1. Define search ranges for each threshold:
   - staticThreshold: [1.0, 10.0]
   - slowThreshold: [10.0, 30.0]
   - fastThreshold: [30.0, 80.0]
   - varianceThreshold: [5.0, 25.0]
   - highFreqThreshold: [0.3, 0.7]
   - consistencyThreshold: [0.5, 0.9]

2. Execute grid search (coarse grid: 5 steps per threshold)
3. Identify promising region
4. Execute refined search in promising region
5. Optionally run genetic algorithm for final tuning

### Step 4: Validation
1. Cross-validate with train/test split (80/20)
2. Calculate final accuracy on test set
3. Verify >90% overall accuracy
4. Verify >85% per-motion-type accuracy

### Step 5: Test Suite Update
1. Update threshold constants in motion_classifier.cpp
2. Update test data in failing tests to match tuned thresholds
3. Run full test suite
4. Verify all 10 tests pass

### Step 6: Documentation
1. Document final threshold values
2. Document tuning process and reasoning
3. Add accuracy metrics to README
4. Save confusion matrix for reference

## Implementation Phases

### Phase 1: Data Collection (Priority: HIGH)
- [ ] Create test_videos/ directory structure
- [ ] Collect 50+ video samples
- [ ] Implement VideoDataset class
- [ ] Manually label all samples

### Phase 2: Classification Validation (Priority: HIGH)
- [ ] Implement ClassificationEngine
- [ ] Implement AccuracyAnalyzer
- [ ] Run baseline evaluation
- [ ] Generate baseline report

### Phase 3: Threshold Tuning (Priority: HIGH)
- [ ] Implement ThresholdConfig structure
- [ ] Implement ThresholdSearcher
- [ ] Implement ThresholdOptimizer
- [ ] Execute grid search
- [ ] Execute genetic algorithm refinement
- [ ] Validate final thresholds

### Phase 4: Test Suite Update (Priority: HIGH)
- [ ] Update thresholds in motion_classifier.cpp
- [ ] Update test data in failing tests
- [ ] Run full test suite
- [ ] Verify all tests pass

### Phase 5: Documentation (Priority: MEDIUM)
- [ ] Document final thresholds
- [ ] Create tuning process documentation
- [ ] Update README with accuracy metrics
- [ ] Save confusion matrix images

## Acceptance Criteria

- [ ] Video dataset contains 50+ samples (10+ per motion type)
- [ ] All samples manually labeled with ground truth
- [ ] Overall classification accuracy >90%
- [ ] Per-motion-type accuracy >85%
- [ ] All 10 MotionClassifier tests pass
- [ ] Final thresholds documented with justification
- [ ] Tuning process documented

## Risk Mitigation

### Risk 1: Insufficient video samples
**Mitigation**: Start with available samples, gradually expand dataset. Consider synthetic data augmentation if needed.

### Risk 2: Manual labeling errors
**Mitigation**: Use multiple labelers, resolve conflicts by consensus. Create labeling guidelines document.

### Risk 3: Threshold tuning overfitting
**Mitigation**: Use cross-validation, ensure good generalization. Test on held-out samples.

### Risk 4: Search space too large
**Mitigation**: Use hierarchical search (coarse grid → refined grid → genetic algorithm). Use domain knowledge to narrow ranges.

## Success Metrics

1. **Primary**: All 10 MotionClassifier tests passing
2. **Primary**: Overall accuracy >90%
3. **Secondary**: Per-motion-type accuracy >85%
4. **Secondary**: Test execution time <5 seconds
5. **Tertiary**: Tuning process documented and reproducible

## Next Steps

1. Create test_videos/ directory structure
2. Implement VideoDataset class
3. Begin collecting video samples
4. Implement remaining components following the design
5. Execute threshold tuning process
6. Update tests and verify
7. Document results
