cmake_minimum_required(VERSION 3.21)
project(PersonalFinanceFrontend VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

# Project root: Personal-Finance-Budget-Manager/
get_filename_component(
    PROJECT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    ABSOLUTE
)

qt_add_executable(PersonalFinanceFrontend
    main.cpp
    mainwindow.h
    mainwindow.cpp
    "${PROJECT_ROOT}/third_party/sqlite/sqlite-amalgamation-3540100/sqlite3.c"

    # Domain models
    "${PROJECT_ROOT}/include/models/Account.h"
    "${PROJECT_ROOT}/src/models/Account.cpp"

    "${PROJECT_ROOT}/include/models/Transaction.h"
    "${PROJECT_ROOT}/src/models/Transaction.cpp"

    "${PROJECT_ROOT}/include/models/Budget.h"
    "${PROJECT_ROOT}/src/models/Budget.cpp"

    # SQLite database manager
    "${PROJECT_ROOT}/include/database/DatabaseManager.h"
    "${PROJECT_ROOT}/src/database/DatabaseManager.cpp"

    # Repositories
    "${PROJECT_ROOT}/include/repositories/AccountRepository.h"
    "${PROJECT_ROOT}/src/repositories/AccountRepository.cpp"

    "${PROJECT_ROOT}/include/repositories/TransactionRepository.h"
    "${PROJECT_ROOT}/src/repositories/TransactionRepository.cpp"

    "${PROJECT_ROOT}/include/repositories/BudgetRepository.h"
    "${PROJECT_ROOT}/src/repositories/BudgetRepository.cpp"

    # Business logic
    "${PROJECT_ROOT}/include/services/FinanceService.h"
    "${PROJECT_ROOT}/src/services/FinanceService.cpp"
    "${PROJECT_ROOT}/include/services/TransactionService.h"
    "${PROJECT_ROOT}/src/services/TransactionService.cpp"
    "${PROJECT_ROOT}/include/services/BudgetService.h"
    "${PROJECT_ROOT}/src/services/BudgetService.cpp"
)

target_include_directories(PersonalFinanceFrontend
    PRIVATE
        "${PROJECT_ROOT}/include"
        "${PROJECT_ROOT}/third_party/sqlite/sqlite-amalgamation-3540100"
)

target_link_libraries(PersonalFinanceFrontend
    PRIVATE
        Qt6::Widgets
)
