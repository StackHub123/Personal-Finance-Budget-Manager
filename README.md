# Personal Finance & Budget Management System (PFBM)

A C++17 and Qt 6 desktop application for recording personal income and expenses, organising transactions by account, setting monthly category budgets, reviewing reports, importing bank statements from CSV, and backing up local data to JSON.

**Project repository:** https://github.com/StackHub123/Personal-Finance-Budget-Manager

**Architecture guide:** [architecture.md](architecture.md)

## Contents

- [Overview](#overview)
- [Features](#features)
- [Technology stack](#technology-stack)
- [Architecture](#architecture)
- [Requirements](#requirements)
- [Build and run on UbuntuLinux](#build-and-run-on-ubuntulinux)
- [Data storage](#data-storage)
- [Testing and validation](#testing-and-validation)
- [Project contributors](#project-contributors)
- [Documentation references](#documentation-references)

## Overview

PFBM is a local desktop application intended to make everyday personal-finance records easier to manage in one place. Users can maintain multiple accounts, record income and expenses, associate transactions with categories and dates, define monthly spending limits, and inspect summaries for a selected account and reporting period.

The application uses a layered design. The Qt interface handles interaction and navigation, service classes coordinate application operations, repositories encapsulate data access, and SQLite stores records locally. The separation is intended to keep UI code independent of raw SQL and make the application easier to maintain and extend.

## Features

### Dashboard

- Displays an overview for the active account.
- Presents current balance, income, expenses, and net savings metrics.
- Shows recent transaction activity and budget usage.

### Account management

- Create, view, edit, and delete financial accounts.
- Associate transactions and budgets with accounts.
- Switch the active account context used by account-scoped views.

### Transactions

- Record income and expenses.
- Edit and delete selected records.
- Search and filter transaction activity.
- Associate each transaction with an account, category, description, amount, type, and date.

### Budgets

- Create monthly category budgets, with account association.
- Compare actual eligible expenses with the defined budget limit.
- Display spent, remaining, and budget status values.

### Reports

- Generate summaries for a selected report type and period.
- Display category expense proportions with a donut chart.
- Display comparisons with a horizontal bar chart.
- Present report details in a table.

### CSV statement import

- Select a CSV statement and destination account.
- Preview parsed transactions before import.
- Display parsing issues and possible duplicates.
- Confirm the import after reviewing the preview.

CSV support is intended for common bank-export formats. Different banks and locales may use different headers, delimiters, and date/amount conventions, so compatibility with every statement format is not guaranteed. PDF and spreadsheet statements generally need to be exported to CSV first.

### Backup, restore, and settings

- Export accounts, transactions, and budgets to JSON.
- Validate a backup's structure before restore.
- Ask for confirmation before replacing the current stored dataset.
- Persist preferences such as theme, date format, number grouping, default report period, and duplicate-import policy.

## Technology stack

| Technology | Role |
|---|---|
| C++17 | Domain models, repositories, services, and application logic. |
| Qt 6 Widgets | Desktop window, layouts, forms, tables, controls, and signals/slots. |
| SQLite | Embedded local relational database. |
| CMake | Build configuration and dependency management. |
| Git and GitHub | Version control, branches, and team collaboration. |
| JSON | Portable backup representation. |
| CSV | Bank-statement import format. |

The report specifies CMake 3.21 or newer and Qt 6.2 or newer. Check the current CMake configuration for the exact SQLite source/linkage used by the checkout; the project has used the SQLite amalgamation approach.

## Architecture

The principal layers are:

1. **Presentation:** `MainWindow`, Qt pages, and custom chart widgets.
2. **Services:** `FinanceService`, `TransactionService`, `BudgetService`, and `BackupRestoreService`.
3. **Repositories:** `AccountRepository`, `TransactionRepository`, and `BudgetRepository`.
4. **Database:** `DatabaseManager` and the local SQLite database.
5. **Domain models:** `Account`, `Transaction`, and `Budget`.

`MainWindow` composes the database manager, repositories, and services. Repositories use `DatabaseManager` to access SQLite. Services coordinate multiple repositories when an operation needs more than one type of record.

See [architecture.md](architecture.md) for Mermaid diagrams covering the layer structure, class dependencies, entity relationships, read/write sequences, navigation, and CSV import flow.

## Requirements

- A C++17-compatible compiler.
- CMake 3.21 or newer.
- Qt 6.2 or newer, including the Qt Widgets module.
- The SQLite source/library configuration expected by the repository's CMake files.
- A supported desktop environment with a working Qt platform plugin.

## Build and run on Ubuntu/Linux

Run the following commands from the repository root.

### 1. Configure the project

```bash
cmake -S frontend/PersonalFinanceDashboard -B build
```

### 2. Build

```bash
cmake --build build -j2
```

### 3. Run

```bash
./build/PersonalFinanceFrontend
```

The executable location can vary if the CMake generator or build configuration changes. Check the generated build directory if it is not at the path shown above.

### Qt platform troubleshooting

On some Linux desktop configurations, Qt may fail to select the expected display backend. When the X11/XCB Qt platform plugin is installed, try:

```bash
QT_QPA_PLATFORM=xcb ./build/PersonalFinanceFrontend
```

This is a troubleshooting option, not a required launch command for every system. The required Qt platform plugin must be installed and discoverable by Qt.

## Data storage

The application stores finance data in a local SQLite database named `personal_finance.db`. The report describes use of `QStandardPaths::AppDataLocation` to obtain a writable application-data directory, followed by database opening and schema initialisation.

User-interface preferences are stored separately with `QSettings`. The report lists preferences for active account, theme, date display, number formatting, report period, and duplicate-import policy.

Backups are represented as JSON files containing account, transaction, and budget data. Restoring a backup replaces the current stored dataset after structural validation and explicit user confirmation. Keep independent copies of important backups.

## Testing and validation

The project report includes a validation matrix for startup, persistence, account CRUD, transactions, input validation, budgets, reports, CSV import, duplicate handling, backup, restore, restore cancellation, settings persistence, and a clean build.

The report explicitly says not to mark these cases as passed unless they have been executed against the final commit. GUI screenshots are evidence of the interface, but do not by themselves establish persistence, validation, import, or restore correctness. Run the relevant tests and live scenarios on the exact revision being submitted, then record the actual results.

If automated tests are available in the current checkout and registered with CTest, they can be run with:

```bash
ctest --test-dir build --output-on-failure
```

Do not assume this command will discover every backend test unless the repository's CMake configuration registers those tests.

## Project contributors

- **B Shivaram**
- **Rudrawar Sahil Amol**

Academic year shown in the project report: **2026–2027**.

## Documentation references

- [Qt 6 documentation](https://doc.qt.io/qt-6/)
- [SQLite documentation](https://www.sqlite.org/docs.html)
- [CMake documentation](https://cmake.org/cmake/help/latest/)
- [C++ reference](https://en.cppreference.com/)

---

This README is based on the supplied PFBM mini-project report and project context. Confirm paths, build targets, and exact feature behaviour against the current repository revision before release or submission.
