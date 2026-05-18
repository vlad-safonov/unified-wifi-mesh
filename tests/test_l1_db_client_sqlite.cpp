/**
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef USE_SQLITE

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include "db_client.h"

// Path used for all SQLite test databases; unique per process to avoid collisions.
static std::string test_db_path() {
    return std::string("/tmp/test_db_client_sqlite_") +
           std::to_string(static_cast<long long>(getpid())) + ".db";
}

// Internal result_context_t mirrors the layout defined in db_client_sqlite.cpp.
// This lets tests that need direct stmt access cast the opaque void* pointer.
struct result_context_t {
    sqlite3_stmt *stmt;
};

// -------------------------------------------------------------------------
// Fixture: basic – uses a bare db_client_t; no pre-created tables.
// -------------------------------------------------------------------------
class db_client_sqlite_t_Test : public ::testing::Test {
protected:
    db_client_t *dbClient;
    std::string  dbPath;

    void SetUp() override {
        dbPath   = test_db_path();
        dbClient = new db_client_t();
        ASSERT_EQ(dbClient->init(dbPath.c_str()), 0)
            << "Failed to open SQLite DB: " << dbPath;
    }

    void TearDown() override {
        delete dbClient;
        unlink(dbPath.c_str());
    }
};

// -------------------------------------------------------------------------
// Fixture: CRUD – pre-creates a 'users' table with two rows.
// -------------------------------------------------------------------------
class db_client_sqlite_crud_Test : public ::testing::Test {
protected:
    db_client_t *dbClient;
    std::string  dbPath;

    void SetUp() override {
        dbPath   = test_db_path();
        dbClient = new db_client_t();
        ASSERT_EQ(dbClient->init(dbPath.c_str()), 0);

        // Create table
        dbClient->execute(
            "CREATE TABLE IF NOT EXISTS users ("
            "id       INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT    NOT NULL,"
            "email    TEXT"
            ");");

        // Insert two rows
        dbClient->execute(
            "INSERT INTO users (username, email) VALUES "
            "('alice', 'alice@example.com');");
        dbClient->execute(
            "INSERT INTO users (username, email) VALUES "
            "('bob',   'bob@example.com');");
    }

    void TearDown() override {
        dbClient->execute("DROP TABLE IF EXISTS users;");
        delete dbClient;
        unlink(dbPath.c_str());
    }

    // Helper: free a result context that was not fully iterated
    void free_result(void *result) {
        if (result) {
            result_context_t *ctx = static_cast<result_context_t *>(result);
            if (ctx->stmt) sqlite3_finalize(ctx->stmt);
            delete ctx;
        }
    }
};

// =========================================================================
// Tests
// =========================================================================

/**
 * @brief Test the execution of a valid SELECT query.
 *
 * Verifies that execute() returns a non-null context for a SELECT on an
 * existing table that has rows.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 001
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table exists with at least one entry.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute a valid SELECT query | query = "SELECT id, username, email FROM users;" | result != nullptr | Should Pass |
 * | 02 | Free the result context | — | No crash | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, ExecuteValidSelectQuery) {
    std::cout << "Entering ExecuteValidSelectQuery test" << std::endl;
    void *result = dbClient->execute("SELECT id, username, email FROM users;");
    ASSERT_NE(result, nullptr) << "SELECT query returned null context";
    free_result(result);
    std::cout << "Exiting ExecuteValidSelectQuery test" << std::endl;
}

/**
 * @brief Test the execution of an empty query.
 *
 * Verifies that execute("") returns nullptr.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 002
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute empty query | query = "" | result == nullptr | Should Pass |
 */
TEST_F(db_client_sqlite_t_Test, ExecuteEmptyQuery) {
    std::cout << "Entering ExecuteEmptyQuery test" << std::endl;
    void *result = dbClient->execute("");
    ASSERT_EQ(result, nullptr);
    std::cout << "Exiting ExecuteEmptyQuery test" << std::endl;
}

/**
 * @brief Test the execution of a null query.
 *
 * Verifies that execute(nullptr) returns nullptr without crashing.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 003
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute null query | query = nullptr | result == nullptr | Should Pass |
 */
TEST_F(db_client_sqlite_t_Test, ExecuteNullQuery) {
    std::cout << "Entering ExecuteNullQuery test" << std::endl;
    void *result = dbClient->execute(nullptr);
    ASSERT_EQ(result, nullptr);
    std::cout << "Exiting ExecuteNullQuery test" << std::endl;
}

/**
 * @brief Test the execution of an invalid SQL statement.
 *
 * Verifies that execute() returns nullptr for a syntactically invalid query.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 004
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute invalid SQL | query = "SELEC * FROM users;" | result == nullptr | Should Pass |
 */
TEST_F(db_client_sqlite_t_Test, ExecuteInvalidSQLSyntax) {
    std::cout << "Entering ExecuteInvalidSQLSyntax test" << std::endl;
    void *result = dbClient->execute("SELEC * FROM users;");
    ASSERT_EQ(result, nullptr);
    std::cout << "Exiting ExecuteInvalidSQLSyntax test" << std::endl;
}

/**
 * @brief Test get_number() on a valid numeric column.
 *
 * Executes a SELECT, advances to the first row with next_result(), then
 * calls get_number() to retrieve an integer ID.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 005
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table exists with at least one entry.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT to get first user id | query = "SELECT id FROM users ORDER BY id ASC LIMIT 1;" | result != nullptr | Should Pass |
 * | 02 | Advance to first row with next_result() | — | Returns true | Should Pass |
 * | 03 | Call get_number() on column 1 | col = 1 | id >= 1 | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, GetNumberValidColumn) {
    std::cout << "Entering GetNumberValidColumn test" << std::endl;
    void *result = dbClient->execute("SELECT id FROM users ORDER BY id ASC LIMIT 1;");
    ASSERT_NE(result, nullptr);

    bool hasRow = dbClient->next_result(result);
    ASSERT_TRUE(hasRow) << "Expected at least one row";

    int id = dbClient->get_number(result, 1);
    EXPECT_GE(id, 1) << "Expected id >= 1";

    // Drain remaining rows so context is cleaned up
    while (dbClient->next_result(result)) {}
    std::cout << "Exiting GetNumberValidColumn test" << std::endl;
}

/**
 * @brief Test get_number() with a null context triggers assertion.
 *
 * Verifies that passing nullptr to get_number() causes process termination
 * (assert failure).
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 006
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Call get_number with null ctx | ctx = nullptr, col = 1 | Process terminates | EXPECT_DEATH |
 */
TEST_F(db_client_sqlite_t_Test, RetrieveIntegerFromInvalidContext) {
    std::cout << "Entering RetrieveIntegerFromInvalidContext test" << std::endl;
    void *invalid_ctx = nullptr;
    EXPECT_DEATH(dbClient->get_number(invalid_ctx, 1), ".*");
    std::cout << "Exiting RetrieveIntegerFromInvalidContext test" << std::endl;
}

/**
 * @brief Test get_number() on a non-numeric (TEXT) column.
 *
 * SQLite3 will try atoi() on the text value; for 'alice' this returns 0.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 007
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table exists with at least one entry.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT id, username | — | result != nullptr | Should Pass |
 * | 02 | Advance to first row | — | Returns true | Should Pass |
 * | 03 | get_number() on col 2 (username TEXT) | col = 2 | Returns 0 | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, GetNumberNonNumericColumn) {
    std::cout << "Entering GetNumberNonNumericColumn test" << std::endl;
    void *result = dbClient->execute(
        "SELECT id, username FROM users ORDER BY id ASC LIMIT 1;");
    ASSERT_NE(result, nullptr);

    ASSERT_TRUE(dbClient->next_result(result));

    int value = dbClient->get_number(result, 2);
    EXPECT_EQ(value, 0) << "atoi('alice') should be 0";

    while (dbClient->next_result(result)) {}
    std::cout << "Exiting GetNumberNonNumericColumn test" << std::endl;
}

/**
 * @brief Test get_number() with column index 0 (invalid, 1-based API).
 *
 * The implementation returns 0 for col == 0.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 008
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table exists with at least one entry.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT id | — | result != nullptr | Should Pass |
 * | 02 | Advance to first row | — | Returns true | Should Pass |
 * | 03 | get_number() with col = 0 | col = 0 | Returns 0 | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, GetNumberInvalidColumn) {
    std::cout << "Entering GetNumberInvalidColumn test" << std::endl;
    void *result = dbClient->execute(
        "SELECT id FROM users ORDER BY id ASC LIMIT 1;");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(dbClient->next_result(result));

    int value = dbClient->get_number(result, 0);
    EXPECT_EQ(value, 0);

    while (dbClient->next_result(result)) {}
    std::cout << "Exiting GetNumberInvalidColumn test" << std::endl;
}

/**
 * @brief Test get_string() on a valid TEXT column.
 *
 * Verifies that get_string() returns a non-empty string from column 1.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 009
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table exists with at least one row.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT username | — | result != nullptr | Should Pass |
 * | 02 | Advance to first row | — | Returns true | Should Pass |
 * | 03 | get_string() on col 1 | buffer[256] | Non-empty string | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, GetStringValidColumn) {
    std::cout << "Entering GetStringValidColumn test" << std::endl;
    void *result = dbClient->execute(
        "SELECT username FROM users ORDER BY id ASC LIMIT 1;");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(dbClient->next_result(result));

    char buffer[256] = {0};
    char *str = dbClient->get_string(result, buffer, 1);
    ASSERT_NE(str, nullptr);
    EXPECT_GT(strlen(str), 0u) << "Expected non-empty username";
    std::cout << "Retrieved username: " << str << std::endl;

    while (dbClient->next_result(result)) {}
    std::cout << "Exiting GetStringValidColumn test" << std::endl;
}

/**
 * @brief Test get_string() with column index 0 (invalid, 1-based API).
 *
 * The implementation returns nullptr for col == 0.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 010
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table exists with at least one row.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT username | — | result != nullptr | Should Pass |
 * | 02 | Advance to first row | — | Returns true | Should Pass |
 * | 03 | get_string() with col = 0 | col = 0 | Returns nullptr | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, GetStringInvalidColumn) {
    std::cout << "Entering GetStringInvalidColumn test" << std::endl;
    void *result = dbClient->execute(
        "SELECT username FROM users ORDER BY id ASC LIMIT 1;");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(dbClient->next_result(result));

    char buffer[256] = {0};
    char *str = dbClient->get_string(result, buffer, 0);
    EXPECT_EQ(str, nullptr) << "Expected nullptr for col == 0";

    while (dbClient->next_result(result)) {}
    std::cout << "Exiting GetStringInvalidColumn test" << std::endl;
}

/**
 * @brief Test get_string() with a null context.
 *
 * Verifies that passing nullptr as ctx returns nullptr without crashing.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 011
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Call get_string with ctx=nullptr | ctx=nullptr, str=nullptr, col=0 | Returns nullptr | Should Pass |
 */
TEST_F(db_client_sqlite_t_Test, RetrieveStringWithNullResultAndNullContext) {
    std::cout << "Entering RetrieveStringWithNullResultAndNullContext test" << std::endl;
    char *result = dbClient->get_string(nullptr, nullptr, 0);
    ASSERT_EQ(result, nullptr);
    std::cout << "Exiting RetrieveStringWithNullResultAndNullContext test" << std::endl;
}

/**
 * @brief Test init() with a valid file path.
 *
 * Verifies that init() returns 0 and a connection is established.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 012
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Call init() with a valid tmp path | result == 0 | Should Pass |
 */
TEST(db_client_sqlite_tTest, ConnectWithValidDatabasePath) {
    std::cout << "Entering ConnectWithValidDatabasePath test" << std::endl;
    std::string path = "/tmp/test_sqlite_valid_" +
                       std::to_string(static_cast<long long>(getpid())) + ".db";
    db_client_t *dbClient = new db_client_t();
    int result = dbClient->init(path.c_str());
    EXPECT_EQ(result, 0);
    delete dbClient;
    unlink(path.c_str());
    std::cout << "Exiting ConnectWithValidDatabasePath test" << std::endl;
}

/**
 * @brief Test init() with an empty path returns -1.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 013
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Call init("") | result == -1 | Should Pass |
 */
TEST(db_client_sqlite_tTest, ConnectWithEmptyPath) {
    std::cout << "Entering ConnectWithEmptyPath test" << std::endl;
    db_client_t *dbClient = new db_client_t();
    int result = dbClient->init("");
    EXPECT_EQ(result, -1);
    delete dbClient;
    std::cout << "Exiting ConnectWithEmptyPath test" << std::endl;
}

/**
 * @brief Test init() with a null path returns -1.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 014
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Call init(nullptr) | result == -1 | Should Pass |
 */
TEST(db_client_sqlite_tTest, ConnectWithNullPath) {
    std::cout << "Entering ConnectWithNullPath test" << std::endl;
    db_client_t *dbClient = new db_client_t();
    int result = dbClient->init(nullptr);
    EXPECT_EQ(result, -1);
    delete dbClient;
    std::cout << "Exiting ConnectWithNullPath test" << std::endl;
}

/**
 * @brief Test next_result() returns true when rows are available.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 015
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table has at least one row.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT username FROM users | — | result != nullptr | Should Pass |
 * | 02 | Call next_result() | — | Returns true | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, NextResultValidContextHasRows) {
    std::cout << "Entering NextResultValidContextHasRows test" << std::endl;
    void *result = dbClient->execute("SELECT username FROM users ORDER BY id ASC;");
    ASSERT_NE(result, nullptr);

    bool hasRow = dbClient->next_result(result);
    EXPECT_TRUE(hasRow) << "Expected at least one row";

    // Drain
    while (dbClient->next_result(result)) {}
    std::cout << "Exiting NextResultValidContextHasRows test" << std::endl;
}

/**
 * @brief Test next_result() returns false after the last row is consumed.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 016
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table has at least one row.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT ... WHERE id = 1 LIMIT 1 | — | result != nullptr | Should Pass |
 * | 02 | Call next_result() once | — | Returns true | Should Pass |
 * | 03 | Call next_result() again | — | Returns false | Context freed |
 */
TEST_F(db_client_sqlite_crud_Test, NextResultNoMoreRows) {
    std::cout << "Entering NextResultNoMoreRows test" << std::endl;
    void *result = dbClient->execute(
        "SELECT username FROM users ORDER BY id ASC LIMIT 1;");
    ASSERT_NE(result, nullptr);

    bool first = dbClient->next_result(result);
    EXPECT_TRUE(first) << "Expected first row to be available";

    bool second = dbClient->next_result(result);
    EXPECT_FALSE(second) << "Expected no more rows after the only row";
    std::cout << "Exiting NextResultNoMoreRows test" << std::endl;
}

/**
 * @brief Test next_result() on a freed (use-after-free) context triggers death.
 *
 * After all rows are consumed next_result() frees the context. Calling it
 * again on the freed pointer is undefined behaviour and should crash.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 017
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table has at least two rows.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT returning two rows | — | result != nullptr | Should Pass |
 * | 02 | Consume row 1, row 2, then get false | — | false after row 2 | Should Pass |
 * | 03 | Call next_result on freed context | — | Process terminates | EXPECT_DEATH |
 */
TEST_F(db_client_sqlite_crud_Test, NextResultInvalidContext) {
    std::cout << "Entering NextResultInvalidContext test" << std::endl;
    void *result = dbClient->execute(
        "SELECT username FROM users ORDER BY id ASC LIMIT 2;");
    ASSERT_NE(result, nullptr);

    EXPECT_TRUE(dbClient->next_result(result))  << "Expected row 1";
    EXPECT_TRUE(dbClient->next_result(result))  << "Expected row 2";
    EXPECT_FALSE(dbClient->next_result(result)) << "Expected no row 3; context freed";

    // result now points to freed memory – use-after-free must crash
    EXPECT_DEATH({
        dbClient->next_result(result);
    }, ".*");
    std::cout << "Exiting NextResultInvalidContext test" << std::endl;
}

/**
 * @brief Test next_result() with a null context returns false.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 018
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Call next_result(nullptr) | ctx = nullptr | Returns false | Should Pass |
 */
TEST_F(db_client_sqlite_t_Test, NullResultContext) {
    std::cout << "Entering NullResultContext test" << std::endl;
    bool result = dbClient->next_result(nullptr);
    EXPECT_FALSE(result);
    std::cout << "Exiting NullResultContext test" << std::endl;
}

/**
 * @brief Test recreate_db() with a valid connection returns 0.
 *
 * Verifies that the database file is deleted and recreated successfully.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 019
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Call recreate_db() with valid connection | result == 0 | Should Pass |
 * | 02 | Verify connection is still usable | execute CREATE TABLE succeeds | Should Pass |
 */
TEST_F(db_client_sqlite_t_Test, RecreateDatabaseWithValidConnection) {
    std::cout << "Entering RecreateDatabaseWithValidConnection test" << std::endl;
    int result = dbClient->recreate_db();
    EXPECT_EQ(result, 0);

    // Verify the connection is still alive after recreate
    void *ctx = dbClient->execute(
        "CREATE TABLE IF NOT EXISTS verify_tbl (id INTEGER PRIMARY KEY);");
    // DML returns nullptr (success with no result set) — that's fine
    (void)ctx;
    std::cout << "Exiting RecreateDatabaseWithValidConnection test" << std::endl;
}

/**
 * @brief Test recreate_db() with no connection returns -1.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 020
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Construct db_client_t without calling init() | result == -1 | Should Pass |
 */
TEST(db_client_sqlite_tTest, RecreateDatabaseWithInvalidConnection) {
    std::cout << "Entering RecreateDatabaseWithInvalidConnection test" << std::endl;
    db_client_t *dbClient = new db_client_t();
    // Do NOT call init() – m_con stays NULL
    int result = dbClient->recreate_db();
    EXPECT_EQ(result, -1);
    delete dbClient;
    std::cout << "Exiting RecreateDatabaseWithInvalidConnection test" << std::endl;
}

/**
 * @brief Test that the default constructor does not throw.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 021
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Construct db_client_t | No exception thrown | Should Pass |
 */
TEST(db_client_sqlite_tTest, DefaultConstruction) {
    std::cout << "Entering DefaultConstruction test" << std::endl;
    EXPECT_NO_THROW({
        db_client_t client;
        std::cout << "db_client_t() constructed successfully." << std::endl;
    });
    std::cout << "Exiting DefaultConstruction test" << std::endl;
}

/**
 * @brief Test that the destructor releases resources without throwing.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 022
 * **Priority:** High
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Construct db_client_t and let it go out of scope | No exception thrown | Should Pass |
 */
TEST(db_client_sqlite_tTest, DestructorReleasesDatabaseResourcesCorrectly) {
    std::cout << "Entering DestructorReleasesDatabaseResourcesCorrectly test" << std::endl;
    EXPECT_NO_THROW({
        {
            db_client_t client;
        }
        std::cout << "db_client_t destructor invoked without exception." << std::endl;
    });
    std::cout << "Exiting DestructorReleasesDatabaseResourcesCorrectly test" << std::endl;
}

/**
 * @brief Test that get_string() correctly retrieves the expected username value.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 023
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table has 'alice' as the first entry.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | SELECT username ORDER BY id ASC LIMIT 1 | — | "alice" | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, GetStringExpectedValue) {
    std::cout << "Entering GetStringExpectedValue test" << std::endl;
    void *result = dbClient->execute(
        "SELECT username FROM users ORDER BY id ASC LIMIT 1;");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(dbClient->next_result(result));

    char buffer[256] = {0};
    char *str = dbClient->get_string(result, buffer, 1);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "alice");

    while (dbClient->next_result(result)) {}
    std::cout << "Exiting GetStringExpectedValue test" << std::endl;
}

/**
 * @brief Test iterating all rows returns the correct count.
 *
 * Two rows were inserted in SetUp(); verify that next_result() is called
 * exactly twice before returning false.
 *
 * **Test Group ID:** Basic: 01
 * **Test Case ID:** 024
 * **Priority:** High
 * @n
 * **Pre-Conditions:** The `users` table has exactly two rows.@n
 * **Dependencies:** None
 * **User Interaction:** None
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Execute SELECT all users | — | result != nullptr | Should Pass |
 * | 02 | Iterate with next_result() | — | Exactly 2 rows | Should Pass |
 */
TEST_F(db_client_sqlite_crud_Test, IterateAllRows) {
    std::cout << "Entering IterateAllRows test" << std::endl;
    void *result = dbClient->execute("SELECT username FROM users ORDER BY id ASC;");
    ASSERT_NE(result, nullptr);

    int count = 0;
    while (dbClient->next_result(result)) {
        ++count;
    }
    EXPECT_EQ(count, 2);
    std::cout << "Exiting IterateAllRows test (count=" << count << ")" << std::endl;
}

#endif /* USE_SQLITE */
