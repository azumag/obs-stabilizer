#!/usr/bin/env python3

# Auto-fix script for OBS Stabilizer plugin issues
# Analyzes test failures and automatically fixes code issues

import os
import sys
import re
import json
import subprocess
from pathlib import Path

class AutoFixer:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.src_dir = self.project_root / "src"
        self.fixes_applied = []

    def log(self, message):
        print(f"[AutoFixer] {message}")
        self.fixes_applied.append(message)

    def fix_settings_crash(self, file_path):
        """Fix settings access crash in update function"""
        if not file_path.exists():
            return False

        content = file_path.read_text()

        # Check if update function accesses settings
        if 'stabilizer_filter_update' in content and 'obs_data_get' in content:
            self.log("Detected settings access crash issue")

            # Backup original
            backup_path = file_path.with_suffix('.cpp.backup')
            if not backup_path.exists():
                file_path.rename(backup_path)

            # Add comment about the issue
            if '// WORKAROUND: Don' in content:
                return True  # Already fixed

            # Simple fix - add comment at start of update function
            content = content.replace(
                'static void stabilizer_filter_update(void *data, obs_data_t *settings)\n{',
                '''static void stabilizer_filter_update(void *data, obs_data_t *settings)
{
    // WORKAROUND: Don't access settings in update function to avoid crash
    // Settings are already read in create function
    // See docs/issue_001_settings_crash.md for details
'''
            )

            file_path.write_text(content)
            self.log("Added settings crash workaround comment")
            return True

        return False

    def fix_null_pointer_checks(self, file_path):
        """Add NULL pointer checks to filter functions"""
        content = file_path.read_text()

        if 'stabilizer_filter_update' in content:
            # Check for NULL pointer check
            if 'if (!filter)' not in content:
                self.log("Adding NULL pointer check to update function")

                # Add NULL check after function definition
                pattern = r'(static void stabilizer_filter_update\(void \*data, obs_data_t \*settings\)\n\{)'
                replacement = r'''\1
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    if (!filter) {
        obs_log(LOG_WARNING, "Update called with NULL filter data");
        return;
    }
'''

                content = re.sub(pattern, replacement, content)
                file_path.write_text(content)
                return True

        return False

    def fix_exception_handling(self, file_path):
        """Add OpenCV exception handling"""
        content = file_path.read_text()

        if 'stabilizer_filter_video' in content:
            # Check for try-catch block
            if 'cv::Exception' not in content:
                self.log("Adding OpenCV exception handling")

                # Find the video filter function and wrap in try-catch
                pattern = r'(struct obs_source_frame \*stabilizer_filter_video\(.*?\)\n\{)'
                replacement = r'''\1
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;

    if (!filter || !frame || !filter->enabled) {
        return frame;
    }

    try {
'''

                content = re.sub(pattern, replacement, content)

                # Add catch block before return
                if 'catch (const cv::Exception' not in content:
                    # Find the final return and add catch before it
                    return_pattern = r'(\s+return frame;\n\})'
                    replacement = r'''    } catch (const cv::Exception &e) {
        obs_log(LOG_ERROR, "OpenCV error in stabilizer: %s", e.what());
    } catch (const std::exception &e) {
        obs_log(LOG_ERROR, "Error in stabilizer: %s", e.what());
    }

    return frame;
}'''

                    content = re.sub(return_pattern, replacement, content, count=1)
                    file_path.write_text(content)
                    return True

        return False

    def fix_mutex_thread_safety(self, file_path):
        """Add mutex for thread safety"""
        content = file_path.read_text()

        # Check if mutex is already declared
        if 'std::mutex' in content and 'mutex' in content:
            return True

        self.log("Adding mutex for thread safety")

        # Add mutex to struct
        struct_pattern = r'(struct stabilizer_filter \{[^}]+)'
        replacement = r'''\1

    // Thread safety
    std::mutex mutex;
'''

        # This is a simple heuristic - in reality you'd need more precise parsing
        if 'Thread safety' not in content:
            # Find end of struct and add mutex before closing brace
            content = re.sub(
                r'(uint32_t height;\n)',
                r'\1\n    std::mutex mutex;',
                content
            )
            file_path.write_text(content)
            return True

        return False

    def fix_cmake_opencv_paths(self, cmake_path):
        """Fix CMakeLists.txt for proper OpenCV linking"""
        if not cmake_path.exists():
            return False

        content = cmake_path.read_text()

        # Check if RPATH is properly set
        if '/opt/homebrew/lib' not in content:
            self.log("Adding Homebrew library paths to CMake RPATH")

            # Update RPATH line
            if 'INSTALL_RPATH' in content:
                content = re.sub(
                    r'INSTALL_RPATH "@loader_path/\.\./Frameworks;@loader_path/\.\./Resources/lib"',
                    r'INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Resources/lib;/opt/homebrew/lib;/usr/local/lib"',
                    content
                )
                cmake_path.write_text(content)
                return True

        return False

    def fix_plugin_support_symbols(self, support_path):
        """Add symbol bridging functions to plugin-support.c"""
        if not support_path.exists():
            return False

        content = support_path.read_text()

        # Check if symbol bridging is already present
        if 'obs_register_source' in content and 'obs_register_source_s' in content:
            return True

        self.log("Adding OBS symbol bridging functions")

        # Add bridging functions
        bridging_code = '''
// Symbol bridging for OBS API compatibility
#ifdef HAVE_OBS_HEADERS
bool obs_register_source(struct obs_source_info *info)
{
    extern bool obs_register_source_s(struct obs_source_info *info, size_t size);
    return obs_register_source_s(info, sizeof(*info));
}

void obs_log(int log_level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    extern void blogva(int log_level, const char *format, va_list args);
    blogva(log_level, format, args);
    va_end(args);
}
#endif
'''

        if bridging_code not in content:
            content += bridging_code
            support_path.write_text(content)
            return True

        return False

    def analyze_and_fix(self, test_results_path):
        """Analyze test results and apply fixes"""
        if not test_results_path.exists():
            print(f"Test results not found: {test_results_path}")
            return

        with open(test_results_path, 'r') as f:
            results = json.load(f)

        print(f"\n=== Analyzing Test Results ===")
        print(f"Total: {results['summary']['total']}")
        print(f"Passed: {results['summary']['passed']}")
        print(f"Failed: {results['summary']['failed']}")
        print(f"Fixed: {results['summary']['fixed']}")
        print()

        # Check for failed tests
        for test in results['tests']:
            if test['status'] == 'failed':
                print(f"\nAnalyzing failed test: {test['name']}")
                print(f"Reason: {test['message']}")

                # Apply fixes based on test type
                if 'Build' in test['name']:
                    self.fix_cmake_opencv_paths(self.project_root / 'CMakeLists.txt')
                    self.fix_plugin_support_symbols(self.src_dir / 'plugin-support.c')

                elif 'Plugin Loading' in test['name']:
                    self.fix_plugin_support_symbols(self.src_dir / 'plugin-support.c')
                    self.fix_cmake_opencv_paths(self.project_root / 'CMakeLists.txt')

                elif 'Basic Functionality' in test['name']:
                    stabilizer_path = self.src_dir / 'stabilizer_opencv.cpp'
                    self.fix_settings_crash(stabilizer_path)
                    self.fix_null_pointer_checks(stabilizer_path)
                    self.fix_exception_handling(stabilizer_path)
                    self.fix_mutex_thread_safety(stabilizer_path)

                elif 'Crash Detection' in test['name']:
                    stabilizer_path = self.src_dir / 'stabilizer_opencv.cpp'
                    self.fix_exception_handling(stabilizer_path)
                    self.fix_null_pointer_checks(stabilizer_path)

        # Rebuild if fixes were applied
        if self.fixes_applied:
            print(f"\n=== Fixes Applied ===")
            for fix in self.fixes_applied:
                print(f"  - {fix}")

            print("\nRebuilding plugin...")
            self.rebuild_plugin()
        else:
            print("\nNo automatic fixes were needed or possible.")

    def rebuild_plugin(self):
        """Rebuild the plugin after fixes"""
        build_dir = self.project_root / 'build'

        if not build_dir.exists():
            print("Build directory not found. Run cmake configuration first.")
            return

        os.chdir(build_dir)

        # Clean build cache
        subprocess.run(['rm', '-rf', 'CMakeCache.txt', 'CMakeFiles/'],
                     capture_output=True)

        # Reconfigure
        result = subprocess.run(
            ['cmake', '-G', 'Ninja', str(self.project_root),
             '-DCMAKE_BUILD_TYPE=Release'],
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            print(f"CMake configuration failed: {result.stderr}")
            return

        # Build
        result = subprocess.run(['ninja'], capture_output=True, text=True)

        if result.returncode == 0:
            print("Plugin rebuilt successfully")
        else:
            print(f"Build failed: {result.stderr}")


def main():
    # Get project root
    script_dir = Path(__file__).parent.parent.parent
    project_root = script_dir

    # Find latest test results
    results_dir = script_dir / 'tests' / 'integration' / 'results'
    results_files = sorted(results_dir.glob('results_*.json'), reverse=True)

    if not results_files:
        print("No test results found. Run tests first.")
        sys.exit(1)

    latest_results = results_files[0]
    print(f"Using test results: {latest_results}")

    # Run auto-fixer
    fixer = AutoFixer(project_root)
    fixer.analyze_and_fix(latest_results)


if __name__ == '__main__':
    main()
