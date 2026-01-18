#!/usr/bin/env python3

# Enhanced Auto-fix script with more patterns and intelligence
# Analyzes test failures and automatically fixes code issues

import os
import sys
import re
import json
import subprocess
import shutil
from pathlib import Path
from datetime import datetime

class AdvancedAutoFixer:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.src_dir = self.project_root / "src"
        self.fixes_applied = []
        self.fix_history_file = self.project_root / ".fix_history.json"
        self.max_fix_attempts = 3

        # Load fix history
        self.fix_history = self.load_fix_history()

    def log(self, message):
        print(f"[AutoFixer] {message}")
        self.fixes_applied.append(message)

    def load_fix_history(self):
        if self.fix_history_file.exists():
            try:
                with open(self.fix_history_file, 'r') as f:
                    return json.load(f)
            except:
                return {}
        return {}

    def save_fix_history(self):
        with open(self.fix_history_file, 'w') as f:
            json.dump(self.fix_history, f, indent=2)

    def get_fix_count(self, fix_type):
        return self.fix_history.get(fix_type, 0)

    def increment_fix_count(self, fix_type):
        count = self.get_fix_count(fix_type) + 1
        self.fix_history[fix_type] = count
        self.save_fix_history()
        return count

    def has_been_tried(self, fix_type):
        return self.get_fix_count(fix_type) > 0

    def can_apply_fix(self, fix_type):
        """Check if we can apply this fix (not exceeded max attempts)"""
        return self.get_fix_count(fix_type) < self.max_fix_attempts

    def fix_cmake_obs_library_path(self, cmake_path):
        """Fix CMakeLists.txt for proper OBS library detection"""
        if not self.can_apply_fix('cmake_obs_path'):
            return False

        content = cmake_path.read_text()

        # Check if OBS library path is correct
        if '/Applications/OBS.app/Contents/Frameworks' not in content:
            self.log("Fixing OBS library path in CMakeLists.txt")

            # Backup
            backup_path = cmake_path.with_suffix('.txt.backup')
            if not backup_path.exists():
                shutil.copy(cmake_path, backup_path)

            # Update OBS library paths
            content = re.sub(
                r'set\(OBS_LIBRARY.*?\)',
                '''set(OBS_LIBRARY "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")''',
                content
            )

            cmake_path.write_text(content)
            self.increment_fix_count('cmake_obs_path')
            return True

        return False

    def fix_cmake_rpath(self, cmake_path):
        """Fix CMake RPATH for proper library loading"""
        if not self.can_apply_fix('cmake_rpath'):
            return False

        content = cmake_path.read_text()

        # Check if Homebrew lib paths are included
        if '/opt/homebrew/lib' not in content and '/usr/local/lib' not in content:
            self.log("Fixing CMake RPATH to include Homebrew libraries")

            # Backup
            backup_path = cmake_path.with_suffix('.txt.rpath_backup')
            if not backup_path.exists():
                shutil.copy(cmake_path, backup_path)

            # Update RPATH
            if 'INSTALL_RPATH' in content:
                content = re.sub(
                    r'INSTALL_RPATH "([^"]*)"',
                    r'INSTALL_RPATH "\1;/opt/homebrew/lib;/usr/local/lib"',
                    content
                )

                cmake_path.write_text(content)
                self.increment_fix_count('cmake_rpath')
                return True

        return False

    def fix_symbol_bridging(self, support_path):
        """Add symbol bridging functions to plugin-support.c"""
        if not self.can_apply_fix('symbol_bridging'):
            return False

        content = support_path.read_text()

        # Check if symbol bridging is present
        if 'obs_register_source_s' not in content:
            self.log("Adding OBS symbol bridging functions")

            # Backup
            backup_path = support_path.with_suffix('.c.backup')
            if not backup_path.exists():
                shutil.copy(support_path, backup_path)

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
                self.increment_fix_count('symbol_bridging')
                return True

        return False

    def fix_settings_access_crash(self, file_path):
        """Fix settings access crash in update function"""
        if not self.can_apply_fix('settings_crash'):
            return False

        content = file_path.read_text()

        # Check for obs_data_get in update function
        if re.search(r'stabilizer_filter_update.*?obs_data_get', content, re.DOTALL):
            self.log("Fixing settings access crash in update function")

            # Backup
            backup_path = file_path.with_suffix('.cpp.settings_backup')
            if not backup_path.exists():
                shutil.copy(file_path, backup_path)

            # Comment out settings access in update function
            # Find update function and add warning comment
            pattern = r'(static void stabilizer_filter_update\(void \*data, obs_data_t \*settings\)\s*\{)'
            replacement = r'''\1
    // WORKAROUND: Don't access settings in update function to avoid crash
    // Settings are already read in create function
    // See docs/issue_001_settings_crash.md for details
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    if (!filter) {
        obs_log(LOG_WARNING, "Update called with NULL filter data");
        return;
    }
'''

            if 'WORKAROUND: Don' not in content:
                content = re.sub(pattern, replacement, content, count=1)
                file_path.write_text(content)
                self.increment_fix_count('settings_crash')
                return True

        return False

    def fix_null_pointer_checks(self, file_path):
        """Add NULL pointer checks to all filter functions"""
        if not self.can_apply_fix('null_pointer'):
            return False

        content = file_path.read_text()
        fixes_made = False

        # Check update function
        update_pattern = r'(static void stabilizer_filter_update\(void \*data, obs_data_t \*settings\)\s*\{)'
        if re.search(update_pattern, content):
            if 'if (!filter)' not in content:
                self.log("Adding NULL pointer check to update function")
                replacement = r'''\1
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    if (!filter) {
        obs_log(LOG_WARNING, "Update called with NULL filter data");
        return;
    }
'''
                content = re.sub(update_pattern, replacement, content, count=1)
                fixes_made = True

        # Check video function
        video_pattern = r'(static struct obs_source_frame \*stabilizer_filter_video\(void \*data, struct obs_source_frame \*frame\)\s*\{)'
        if re.search(video_pattern, content):
            if not re.search(r'stabilizer_filter_video.*?if \(!filter.*?\)', content, re.DOTALL):
                self.log("Adding NULL pointer check to video function")
                replacement = r'''\1
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    if (!filter || !frame || !filter->enabled) {
        return frame;
    }
'''
                content = re.sub(video_pattern, replacement, content, count=1)
                fixes_made = True

        if fixes_made:
            backup_path = file_path.with_suffix('.cpp.null_backup')
            if not backup_path.exists():
                shutil.copy(file_path, backup_path)

            file_path.write_text(content)
            self.increment_fix_count('null_pointer')
            return True

        return False

    def fix_exception_handling(self, file_path):
        """Add comprehensive exception handling"""
        if not self.can_apply_fix('exception_handling'):
            return False

        content = file_path.read_text()

        # Check for try-catch in video function
        if 'stabilizer_filter_video' in content:
            if 'cv::Exception' not in content:
                self.log("Adding exception handling to video function")

                # Backup
                backup_path = file_path.with_suffix('.cpp.exception_backup')
                if not backup_path.exists():
                    shutil.copy(file_path, backup_path)

                # Find video function and wrap content in try-catch
                pattern = r'(static struct obs_source_frame \*stabilizer_filter_video\(void \*data, struct obs_source_frame \*frame\)\s*\{[^}]*?return frame;\s*\})'
                replacement = r'''\1
    } catch (const cv::Exception &e) {
        obs_log(LOG_ERROR, "OpenCV error in stabilizer: %s", e.what());
    } catch (const std::exception &e) {
        obs_log(LOG_ERROR, "Error in stabilizer: %s", e.what());
    } catch (...) {
        obs_log(LOG_ERROR, "Unknown error in stabilizer");
    }

    return frame;
}'''

                # This is complex - let's use a simpler approach
                # Add try block at the start of the function
                video_start = r'(static struct obs_source_frame \*stabilizer_filter_video\(void \*data, struct obs_source_frame \*frame\)\s*\{[^}]*?)'
                replacement = r'''\1\n\n    try {'''

                if re.search(video_start, content):
                    # Also need to find the return and add catch before it
                    content = re.sub(
                        r'(\s+return frame;\s*\})',
                        r'''    } catch (const cv::Exception &e) {
        obs_log(LOG_ERROR, "OpenCV error in stabilizer: %s", e.what());
    } catch (const std::exception &e) {
        obs_log(LOG_ERROR, "Error in stabilizer: %s", e.what());
    } catch (...) {
        obs_log(LOG_ERROR, "Unknown error in stabilizer");
    }

\1''',
                        content
                    )

                    file_path.write_text(content)
                    self.increment_fix_count('exception_handling')
                    return True

        return False

    def fix_thread_safety(self, file_path):
        """Add mutex for thread safety"""
        if not self.can_apply_fix('thread_safety'):
            return False

        content = file_path.read_text()

        # Check if mutex is declared
        if 'std::mutex' not in content:
            self.log("Adding mutex for thread safety")

            # Backup
            backup_path = file_path.with_suffix('.cpp.mutex_backup')
            if not backup_path.exists():
                shutil.copy(file_path, backup_path)

            # Add mutex to struct
            # Find end of struct before closing brace
            struct_end_pattern = r'(uint32_t height;\s*)'
            replacement = r'''\1
    std::mutex mutex;'''

            content = re.sub(struct_end_pattern, replacement, content, count=1)
            file_path.write_text(content)
            self.increment_fix_count('thread_safety')
            return True

        return False

    def fix_mutex_usage(self, file_path):
        """Add mutex locks where needed"""
        if not self.can_apply_fix('mutex_usage'):
            return False

        content = file_path.read_text()

        # Check if mutex is being used
        if 'std::mutex' in content and 'std::lock_guard' not in content:
            self.log("Adding mutex locks to video function")

            # Backup
            backup_path = file_path.with_suffix('.cpp.mutex_lock_backup')
            if not backup_path.exists():
                shutil.copy(file_path, backup_path)

            # Add lock at the beginning of video function (after NULL check)
            video_pattern = r'(if \(!filter \|\| !frame \|\| !filter->enabled\) \{\s*return frame;\s*\})'
            replacement = r'''\1

    std::lock_guard<std::mutex> lock(filter->mutex);'''

            if re.search(video_pattern, content):
                content = re.sub(video_pattern, replacement, content, count=1)
                file_path.write_text(content)
                self.increment_fix_count('mutex_usage')
                return True

        return False

    def rebuild_plugin(self):
        """Rebuild plugin after fixes"""
        build_dir = self.project_root / 'build'

        if not build_dir.exists():
            self.log("Build directory not found. Cannot rebuild.")
            return False

        self.log("Rebuilding plugin...")

        # Clean build cache
        cache_dir = build_dir / 'CMakeFiles'
        cmake_cache = build_dir / 'CMakeCache.txt'

        if cache_dir.exists():
            shutil.rmtree(cache_dir)
        if cmake_cache.exists():
            cmake_cache.unlink()

        # Reconfigure
        try:
            result = subprocess.run(
                ['cmake', '-G', 'Ninja', str(self.project_root),
                 '-DCMAKE_BUILD_TYPE=Release'],
                cwd=build_dir,
                capture_output=True,
                text=True,
                timeout=60
            )

            if result.returncode != 0:
                self.log(f"CMake configuration failed: {result.stderr}")
                return False

            # Build
            result = subprocess.run(
                ['ninja'],
                cwd=build_dir,
                capture_output=True,
                text=True,
                timeout=300  # 5 minutes
            )

            if result.returncode == 0:
                self.log("Plugin rebuilt successfully")
                return True
            else:
                self.log(f"Build failed: {result.stderr}")
                return False

        except subprocess.TimeoutExpired:
            self.log("Build timed out")
            return False

    def analyze_and_fix(self, test_results_path):
        """Analyze test results and apply fixes"""
        if not test_results_path.exists():
            print(f"Test results not found: {test_results_path}")
            return

        with open(test_results_path, 'r') as f:
            results = json.load(f)

        print(f"\n{'='*50}")
        print(f"Analyzing Test Results")
        print(f"{'='*50}")
        print(f"Total: {results['summary']['total']}")
        print(f"Passed: {results['summary']['passed']}")
        print(f"Failed: {results['summary']['failed']}")
        print(f"{'='*50}\n")

        fixes_attempted = []

        # Check for failed tests
        for test in results['tests']:
            if test['status'] == 'failed':
                print(f"\nAnalyzing failed test: {test['name']}")
                print(f"Reason: {test['message']}")

                # Apply fixes based on test type
                if 'Pre-flight' in test['name']:
                    if self.fix_preflight_issues():
                        fixes_attempted.append('preflight')

                elif 'Build' in test['name']:
                    cmake_path = self.project_root / 'CMakeLists.txt'
                    support_path = self.src_dir / 'plugin-support.c'

                    if self.fix_cmake_obs_library_path(cmake_path):
                        fixes_attempted.append('cmake_obs_path')
                    if self.fix_cmake_rpath(cmake_path):
                        fixes_attempted.append('cmake_rpath')
                    if self.fix_symbol_bridging(support_path):
                        fixes_attempted.append('symbol_bridging')

                elif 'Plugin Loading' in test['name']:
                    support_path = self.src_dir / 'plugin-support.c'
                    cmake_path = self.project_root / 'CMakeLists.txt'

                    if self.fix_symbol_bridging(support_path):
                        fixes_attempted.append('symbol_bridging')
                    if self.fix_cmake_rpath(cmake_path):
                        fixes_attempted.append('cmake_rpath')

                elif 'Basic Functionality' in test['name']:
                    stabilizer_path = self.src_dir / 'stabilizer_opencv.cpp'

                    if self.fix_settings_access_crash(stabilizer_path):
                        fixes_attempted.append('settings_crash')
                    if self.fix_null_pointer_checks(stabilizer_path):
                        fixes_attempted.append('null_pointer')
                    if self.fix_exception_handling(stabilizer_path):
                        fixes_attempted.append('exception_handling')
                    if self.fix_thread_safety(stabilizer_path):
                        fixes_attempted.append('thread_safety')
                    if self.fix_mutex_usage(stabilizer_path):
                        fixes_attempted.append('mutex_usage')

                elif 'Crash Detection' in test['name']:
                    stabilizer_path = self.src_dir / 'stabilizer_opencv.cpp'

                    if self.fix_exception_handling(stabilizer_path):
                        fixes_attempted.append('exception_handling')
                    if self.fix_null_pointer_checks(stabilizer_path):
                        fixes_attempted.append('null_pointer')
                    if self.fix_mutex_usage(stabilizer_path):
                        fixes_attempted.append('mutex_usage')

        # Rebuild if fixes were applied
        if self.fixes_applied:
            print(f"\n{'='*50}")
            print("Fixes Applied")
            print(f"{'='*50}")
            for i, fix in enumerate(self.fixes_applied, 1):
                print(f"  {i}. {fix}")

            # Rebuild
            if self.rebuild_plugin():
                print("\nPlugin rebuilt successfully")
            else:
                print("\nWARNING: Build failed after fixes")
        else:
            print("\nNo fixes were needed or possible.")

        # Save fix summary
        summary = {
            'timestamp': datetime.now().isoformat(),
            'fixes_attempted': fixes_attempted,
            'fixes_applied': self.fixes_applied,
            'success': len(self.fixes_applied) > 0
        }

        summary_path = self.project_root / 'last_fix_summary.json'
        with open(summary_path, 'w') as f:
            json.dump(summary, f, indent=2)

    def fix_preflight_issues(self):
        """Fix pre-flight environment issues"""
        # This is handled by bash scripts, so we just report
        self.log("Pre-flight issues should be fixed by fix_preflight.sh")
        return False


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
    fixer = AdvancedAutoFixer(project_root)
    fixer.analyze_and_fix(latest_results)


if __name__ == '__main__':
    main()
