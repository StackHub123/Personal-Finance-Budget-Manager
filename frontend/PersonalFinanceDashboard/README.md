# Personal Finance & Budgeting Dashboard — Frontend milestone 0.1

This folder contains the first small, self-contained Qt Widgets frontend milestone, following Phase 4.1 (Frontend layer) and the navigation/dashboard goals in Phase 13 of the project workflow.

## Included

- Main application window with a left navigation sidebar.
- Dashboard landing page with summary cards for balance, income, expenses, and net savings.
- Recent-activity and budget-snapshot panels using clearly labelled illustrative data.
- Navigation to placeholder screens for Transactions, Accounts, Budgets, Reports, Statement Import, Backup & Restore, and Settings.
- Shared stylesheet for a consistent desktop interface.
- A clear frontend/backend boundary: this milestone has no database, repository, or business-service calls.

## Important scope note

The figures and transaction rows on the dashboard are demo placeholders only. They are not persisted and must not be treated as real financial calculations. Each non-dashboard screen is a navigation placeholder, not a completed feature.

## Requirements

- Qt 6 with the Widgets component (Qt 6.2 or newer recommended)
- CMake 3.21+
- A C++17-capable compiler

## Build

From this `frontend` directory:

```bash
cmake -S . -B build
cmake --build build
```

Then run the executable produced in `build` (the exact path varies by operating system and generator).

In Qt Creator, open `CMakeLists.txt`, select a Qt 6 kit, configure, build, and run.

## Suggested commit

```bash
git add frontend
git commit -m "Add initial Qt frontend shell and dashboard"
```

## Next step

Keep this frontend as presentation/navigation only. When the Account, Transaction, Budget, service, and repository interfaces are finalized, connect button actions to those services instead of placing SQL or financial calculations inside `MainWindow`.
