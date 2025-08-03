# Code Style and Conventions

## Development Philosophy
- **YAGNI** (You Aren't Gonna Need It): 今必要じゃない機能は作らない
- **DRY** (Don't Repeat Yourself): 同じコードを繰り返さない  
- **KISS** (Keep It Simple Stupid): シンプルに保つ
- **TDD** (Test-Driven Development): t-wada TDD principles

## Important Rules
- 絵文字を使うな (No emojis in code)
- 無駄にファイルを作りまくるな (Don't create unnecessary files)
- 一時ファイルは一箇所のディレクトリにまとめよ (Consolidate temp files in one directory: /tmp/)

## C++ Style
- C++17/20 with modern safety patterns
- RAII resource management
- Use `#ifdef` guards for conditional compilation
- Prefer `const` correctness
- Use smart pointers over raw pointers

## Naming Conventions
- Functions: `snake_case` (e.g., `stabilizer_filter_create`)
- Classes: `PascalCase` 
- Constants: `UPPER_SNAKE_CASE`
- Member variables: `m_` prefix
- Module name: "test-stabilizer" (current), "obs-stabilizer" (production)

## File Organization
- Keep project root clean (9 essential files max)
- Source code in `src/`
- Tests in `tests/` or `src/tests/`
- Scripts in `scripts/`
- Documentation in `docs/`
- Temporary files in `/tmp/` or `tmp/`

## Error Handling
- Use structured error reporting
- Implement proper exception handling
- Add buffer overflow protection
- Validate all inputs