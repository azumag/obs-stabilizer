# Temporary Files Directory

Following CLAUDE.md principle: "一時ファイルは一箇所のディレクトリにまとめよ" (consolidate temporary files in one directory)

## Organization

- `reports/` - QA reports, assessments, and analysis documents
- `build-artifacts/` - Temporary build outputs and test binaries  
- `scripts/` - Testing and utility scripts
- `tests/tests/` - Test implementation files
- `static-analysis/` - Code analysis outputs

## File Naming Convention

Files are prefixed with date (YYYYMMDD) when moved to prevent conflicts and maintain chronological order.

## Cleanup Policy

This directory contains temporary files that can be safely deleted. Files here are:
- Build artifacts that can be regenerated
- Test outputs and reports
- Development/debugging utilities
- Analysis results and assessments

Permanent project files should never be stored in this directory.
