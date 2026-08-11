#!/bin/bash

# Bundle non-system dependencies into a generated macOS OBS plugin.
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)
PLUGIN_PATH="${1:-${PROJECT_ROOT}/build/obs-stabilizer.plugin}"

if [ ! -d "${PLUGIN_PATH}" ] || [[ "${PLUGIN_PATH}" != *.plugin ]]; then
	printf "Error: Plugin bundle not found at %s\n" "${PLUGIN_PATH}" >&2
	printf "Build it first with: cmake -S . -B build && cmake --build build\n" >&2
	exit 1
fi

BINARY_PATH=$(find "${PLUGIN_PATH}/Contents/MacOS" -maxdepth 1 -type f -perm -111 -print -quit 2>/dev/null || true)
if [ -z "${BINARY_PATH}" ]; then
	BINARY_PATH=$(find "${PLUGIN_PATH}/Contents/MacOS" -maxdepth 1 -type f -print -quit 2>/dev/null || true)
fi
if [ -z "${BINARY_PATH}" ]; then
	printf "Error: No plugin binary found under %s/Contents/MacOS\n" "${PLUGIN_PATH}" >&2
	exit 1
fi

for command in cmake codesign otool; do
	if ! command -v "${command}" >/dev/null 2>&1; then
		printf "Error: Required command not found: %s\n" "${command}" >&2
		exit 1
	fi
done

SEARCH_DIRS=(
	"/Applications/OBS.app/Contents/Frameworks"
	"/opt/homebrew/lib"
	"/opt/homebrew/opt/opencv/lib"
	"/opt/homebrew/opt/gcc/lib/gcc/current"
	"/usr/local/lib"
	"/usr/local/opt/opencv/lib"
	"/opt/local/lib"
)

if command -v brew >/dev/null 2>&1; then
	# These are the current transitive formula families used by Homebrew OpenCV.
	# Absolute install names resolve directly; the opt directories cover the
	# remaining @rpath references without scanning every installed formula.
	for formula in opencv abseil protobuf openvino tbb pugixml openblas libomp gcc; do
		formula_prefix=$(brew --prefix "${formula}" 2>/dev/null || true)
		if [ -n "${formula_prefix}" ] && [ -d "${formula_prefix}/lib" ]; then
			SEARCH_DIRS+=("${formula_prefix}/lib")
		fi
	done
fi

SEARCH_DIRS_VALUE=$(IFS=';'; printf '%s' "${SEARCH_DIRS[*]}")

printf "Bundling dependencies into %s\n" "${PLUGIN_PATH}"
cmake \
	"-DBUNDLE_PATH=${PLUGIN_PATH}" \
	"-DBINARY_PATH=${BINARY_PATH}" \
	"-DSEARCH_DIRS=${SEARCH_DIRS_VALUE}" \
	-P "${PROJECT_ROOT}/cmake/BundledOpenCV.cmake"

# OBS loads plugin modules with dlopen(). Reject executable-relative framework
# references because @executable_path resolves from OBS.app rather than from
# the plugin bundle. All bundled references must use @loader_path instead.
VERIFY_ITEMS=("${BINARY_PATH}")
FRAMEWORKS_DIR="${PLUGIN_PATH}/Contents/Frameworks"
if [ -d "${FRAMEWORKS_DIR}" ]; then
	while IFS= read -r dependency; do
		VERIFY_ITEMS+=("${dependency}")
	done < <(find "${FRAMEWORKS_DIR}" -maxdepth 1 -type f -print)
fi

for item in "${VERIFY_ITEMS[@]}"; do
	if otool -L "${item}" | grep -Fq '@executable_path/../Frameworks'; then
		printf "Error: executable-relative bundled dependency remains in %s\n" "${item}" >&2
		otool -L "${item}" >&2
		exit 1
	fi
done

codesign --force --deep --sign - "${PLUGIN_PATH}"
codesign --verify --deep --strict "${PLUGIN_PATH}"

printf "Plugin bundle is ready: %s\n" "${PLUGIN_PATH}"
printf "Install it with:\n"
printf "  mkdir -p '%s'\n" "${HOME}/Library/Application Support/obs-studio/plugins"
printf "  cp -R '%s' '%s/'\n" \
	"${PLUGIN_PATH}" "${HOME}/Library/Application Support/obs-studio/plugins"
