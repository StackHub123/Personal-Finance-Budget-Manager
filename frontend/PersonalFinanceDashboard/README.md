# Personal Finance & Budgeting Dashboard

Qt Widgets desktop application for tracking accounts, transactions, budgets,
reports, CSV statement imports, and JSON backups. Data is stored in SQLite in
the operating system's application data directory.

## Requirements

- Qt 6.2 or newer, including Widgets
- CMake 3.21 or newer
- A C++17 compiler

## Build and run

From the repository root:

```bash
cmake -S frontend/PersonalFinanceDashboard -B build
cmake --build build
./build/PersonalFinanceFrontend
```

The last command launches the Qt Widgets desktop app from the repository root.
You can also run `./PersonalFinanceFrontend` after changing into the `build`
directory, or use Qt Creator to open
`frontend/PersonalFinanceDashboard/CMakeLists.txt`, select a Qt 6 kit, and run
the target.

The root `src/main.cpp` is a separate console stub; the Qt application is built
from `frontend/PersonalFinanceDashboard`.
