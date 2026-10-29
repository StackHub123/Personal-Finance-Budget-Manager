# Personal-Finance-Budget-Manager

A C++ OOP-based application for managing personal finances,
tracking income and expenses, and maintaining a record of transactions.

## Features

- Add income
- Add expenses
- View transactions
- Calculate balance
- Categorize expenses
- Search transactions
- Store financial data persistently
- Generate basic financial summaries

## Technologies

- C++
- Object-Oriented Programming
- STL
- File Handling
- SQLite
- CMake
- Qt Software

## Team

- B Shivaram
- Sahil Rudrawar

## Project Status

Currently under development.

## Desktop application build

The Qt 6 Widgets application starts at the sign-in screen. New profiles use a local email and password; registration automatically signs the user in. The password must contain at least 8 characters. Authentication records live in `authentication.sqlite`, and each profile's financial records live in a separate `users/<id>/personal_finance.db` file under Qt's application data directory. The old shared financial database is left in place. On first sign-in, PFBM asks whether to import its records into that profile or start fresh; that choice is assigned to one profile only.

Passwords are stored as salted scrypt hashes through OpenSSL 3. The development package is required on Linux:

```bash
sudo apt install build-essential cmake qt6-base-dev libssl-dev
cmake -S frontend/PersonalFinanceDashboard -B build
cmake --build build -j2
./build/PersonalFinanceFrontend
```

Run the authentication integration test with:

```bash
ctest --test-dir build --output-on-failure
```

The existing standalone backend test sources under `tests/` are not currently registered as CMake/CTest targets.
