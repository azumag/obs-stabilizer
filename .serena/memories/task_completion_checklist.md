# Task Completion Checklist

When completing a coding task, always verify:

## 1. Code Quality
- [ ] Code follows YAGNI/DRY/KISS principles
- [ ] No unnecessary complexity added
- [ ] No emoji in code or comments
- [ ] Proper error handling implemented

## 2. Testing
```bash
# Always run after making changes:
./scripts/run-tests.sh
```

## 3. Linting & Type Checking
```bash
# Check for compilation errors
cmake --build build

# If lint/typecheck commands provided by user, run them:
# npm run lint (if Node.js project)
# npm run typecheck (if TypeScript)
# ruff check (if Python)
```

## 4. File Organization
- [ ] No unnecessary files created
- [ ] Temporary files in /tmp/ directory
- [ ] Project root has ≤ 9 essential files
- [ ] Build artifacts in appropriate directories

## 5. OBS Plugin Specific
- [ ] Test plugin loading in OBS after build
- [ ] Verify Info.plist matches binary name
- [ ] Check library dependencies with otool -L
- [ ] Run fix-plugin-loading.sh on macOS

## 6. Documentation
- [ ] Update CLAUDE.md if needed
- [ ] Document any new commands in suggested_commands.md
- [ ] Add troubleshooting notes if issues encountered

## 7. Git Hygiene
- [ ] Stage only necessary changes
- [ ] Write clear commit messages
- [ ] Don't commit unless explicitly asked by user