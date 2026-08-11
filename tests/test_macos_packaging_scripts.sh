#!/bin/bash

set -euo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
TEST_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/obs-stabilizer-packaging.XXXXXX")
MOCK_BIN="${TEST_ROOT}/bin"
MOCK_LOG="${TEST_ROOT}/commands.log"
FIXTURE_ROOT="${TEST_ROOT}/fixture"
BUNDLE_PATH="${FIXTURE_ROOT}/build/obs-stabilizer.plugin"
BINARY_PATH="${BUNDLE_PATH}/Contents/MacOS/obs-stabilizer"

cleanup() {
	rm -rf "${TEST_ROOT}"
}
trap cleanup EXIT

assert_contains() {
	local file="$1"
	local expected="$2"
	if ! grep -Fq -- "${expected}" "${file}"; then
		printf "Expected %s to contain: %s\n" "${file}" "${expected}" >&2
		printf '%s\n' "--- ${file}" >&2
		cat "${file}" >&2
		exit 1
	fi
}

assert_not_contains() {
	local file="$1"
	local unexpected="$2"
	if grep -Fq -- "${unexpected}" "${file}"; then
		printf "Expected %s not to contain: %s\n" "${file}" "${unexpected}" >&2
		printf '%s\n' "--- ${file}" >&2
		cat "${file}" >&2
		exit 1
	fi
}

mkdir -p "${MOCK_BIN}" "$(dirname "${BINARY_PATH}")" "${BUNDLE_PATH}/Contents/Frameworks"
: >"${MOCK_LOG}"
: >"${BINARY_PATH}"
: >"${BUNDLE_PATH}/Contents/Info.plist"
: >"${BUNDLE_PATH}/Contents/Frameworks/libopencv_core.413.dylib"
chmod +x "${BINARY_PATH}"

cat >"${MOCK_BIN}/otool" <<'EOF'
#!/bin/bash
if [ "${1:-}" = "-L" ]; then
	printf '%s:\n' "${2}"
	if [ -f "${MOCK_LOG}.bundled" ]; then
		printf '\t@loader_path/../Frameworks/libopencv_core.413.dylib (compatibility version 413.0.0, current version 4.13.0)\n'
	else
		printf '\t@rpath/libopencv_core.413.dylib (compatibility version 413.0.0, current version 4.13.0)\n'
	fi
	exit 0
fi
exit 1
EOF

cat >"${MOCK_BIN}/install_name_tool" <<'EOF'
#!/bin/bash
printf 'install_name_tool' >>"${MOCK_LOG}"
printf ' %s' "$@" >>"${MOCK_LOG}"
printf '\n' >>"${MOCK_LOG}"
: >"${MOCK_LOG}.bundled"
EOF

cat >"${MOCK_BIN}/codesign" <<'EOF'
#!/bin/bash
printf 'codesign' >>"${MOCK_LOG}"
printf ' %s' "$@" >>"${MOCK_LOG}"
printf '\n' >>"${MOCK_LOG}"
EOF

cat >"${MOCK_BIN}/cmake" <<'EOF'
#!/bin/bash
printf 'cmake' >>"${MOCK_LOG}"
printf ' %s' "$@" >>"${MOCK_LOG}"
printf '\n' >>"${MOCK_LOG}"
EOF

cat >"${MOCK_BIN}/plutil" <<'EOF'
#!/bin/bash
printf 'plutil' >>"${MOCK_LOG}"
printf ' %s' "$@" >>"${MOCK_LOG}"
printf '\n' >>"${MOCK_LOG}"
EOF

chmod +x "${MOCK_BIN}/otool" "${MOCK_BIN}/install_name_tool" \
	"${MOCK_BIN}/codesign" "${MOCK_BIN}/cmake" "${MOCK_BIN}/plutil"

(
	cd "${FIXTURE_ROOT}"
	PATH="${MOCK_BIN}:${PATH}" MOCK_LOG="${MOCK_LOG}" \
		bash "${REPO_ROOT}/scripts/fix-plugin-loading.sh" >"${TEST_ROOT}/fix-output.log"
)

assert_contains "${TEST_ROOT}/fix-output.log" "Binary path: build/obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer"
assert_contains "${MOCK_LOG}" "codesign --force --deep --sign - build/obs-stabilizer.plugin"
assert_contains "${MOCK_LOG}" "install_name_tool -change @rpath/libopencv_core.413.dylib @loader_path/../Frameworks/libopencv_core.413.dylib"
assert_contains "${MOCK_LOG}" "install_name_tool -delete_rpath @executable_path/../Frameworks"
assert_contains "${MOCK_LOG}" "install_name_tool -add_rpath @loader_path/../Frameworks"
assert_not_contains "${MOCK_LOG}" "install_name_tool -add_rpath @executable_path/../Frameworks"
assert_not_contains "${MOCK_LOG}" "install_name_tool -id"
assert_not_contains "${MOCK_LOG}" "install_name_tool -add_rpath /opt/homebrew"

: >"${MOCK_LOG}"
PATH="${MOCK_BIN}:${PATH}" MOCK_LOG="${MOCK_LOG}" \
	bash "${REPO_ROOT}/scripts/bundle_opencv.sh" "${BUNDLE_PATH}" >"${TEST_ROOT}/bundle-output.log"

assert_contains "${MOCK_LOG}" "cmake -DBUNDLE_PATH=${BUNDLE_PATH}"
assert_contains "${MOCK_LOG}" "-P ${REPO_ROOT}/cmake/BundledOpenCV.cmake"
assert_contains "${MOCK_LOG}" "codesign --force --deep --sign - ${BUNDLE_PATH}"

assert_contains "${REPO_ROOT}/CMakeLists.txt" "BUNDLE TRUE"
assert_contains "${REPO_ROOT}/CMakeLists.txt" "BUNDLE_EXTENSION plugin"
assert_contains "${REPO_ROOT}/CMakeLists.txt" 'copy_if_different'
assert_contains "${REPO_ROOT}/CMakeLists.txt" 'TARGET_BUNDLE_CONTENT_DIR'
assert_not_contains "${REPO_ROOT}/CMakeLists.txt" 'OPENCV_LIBS_TO_FIX'
assert_contains "${REPO_ROOT}/CMakeLists.txt" 'OBS_STABILIZER_USE_REAL_OBS=1'
assert_contains "${REPO_ROOT}/cmake/Info.plist.in" '@OBS_STABILIZER_BUNDLE_EXECUTABLE@'
assert_not_contains "${REPO_ROOT}/cmake/Info.plist.in" "test-stabilizer"
assert_contains "${REPO_ROOT}/cmake/BundledOpenCV.cmake" 'set(replacement "@loader_path/../Frameworks/${dependency_name}")'
assert_not_contains "${REPO_ROOT}/cmake/BundledOpenCV.cmake" 'set(replacement "@executable_path/../Frameworks/${dependency_name}")'
assert_contains "${REPO_ROOT}/scripts/bundle_opencv.sh" "executable-relative bundled dependency remains"
assert_contains "${REPO_ROOT}/.github/workflows/build.yml" './scripts/bundle_opencv.sh build/obs-stabilizer.plugin'
assert_contains "${REPO_ROOT}/.github/workflows/build.yml" 'build/obs-stabilizer-macos.zip'
assert_contains "${REPO_ROOT}/src/stabilizer_opencv.cpp" 'OBS_DECLARE_MODULE()'
assert_contains "${REPO_ROOT}/.github/workflows/build.yml" 'if-no-files-found: error'

printf '%s\n' "macOS packaging script tests passed"
