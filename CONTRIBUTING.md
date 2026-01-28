# Contributing to OBS Stabilizer Plugin

Thank you for your interest in contributing to the OBS Stabilizer Plugin! This document provides guidelines for contributing to the project.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/obs-stabilizer.git`
3. Create a new branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Submit a pull request

## Development Setup

### Prerequisites

- CMake 3.28+
- C++17 compatible compiler
- Qt6 development libraries
- OpenCV 4.5+ (4.5-4.8 recommended)
- OBS Studio 30.0+

### Building

```bash
# Configure and build
cmake -B build
cmake --build build

# Or use make directly
cmake -B build
make -C build

# Alternative: Configure and build in current directory
cmake .
make

# Test
./scripts/run-tests.sh
cd build && ./stabilizer_tests
```

## Code Style

- Follow modern C++17/20 best practices
- Use RAII and smart pointers
- Maintain const correctness
- Follow OBS Studio coding guidelines
- Keep line length under 100 characters

## Testing

- Write unit tests for new functionality
- Ensure all tests pass before submitting PR
- Test on multiple platforms if possible
- Include performance impact analysis for algorithm changes

## Pull Request Process

1. Update documentation for any API changes
2. Add tests for new features
3. Ensure CI passes on all platforms
4. Request review from maintainers
5. Address review feedback promptly

## Reporting Issues

- Use issue templates
- Provide detailed reproduction steps
- Include system information
- Attach relevant logs

## Code of Conduct

- Be respectful and inclusive
- Welcome newcomers
- Focus on constructive feedback
- Help maintain a positive community

## License

By contributing, you agree that your contributions will be licensed under GPL-2.0.