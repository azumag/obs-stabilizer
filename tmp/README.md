# Temporary workspace

All generated build output, reports, and packaged artifacts belong under this directory. The entire workspace can be removed safely with `rm -rf tmp/*`.

## Layout

```text
tmp/
├── builds/
│   ├── test/          # Unit-test builds
│   ├── coverage/      # Coverage-instrumented builds
│   ├── benchmark/     # Performance benchmark builds
│   └── integration/   # Integration-test builds
├── reports/
│   ├── coverage/      # HTML, XML, and lcov reports
│   ├── static-analysis/ # cppcheck and related reports
│   └── performance/   # Benchmark results
└── artifacts/         # Packaged plugin binaries
```

CI jobs must create only the subdirectories they need and should remove their previous output before running. Files under `tmp/` are ignored by Git except for this README, `ARCH.md`, and `.gitkeep`.
