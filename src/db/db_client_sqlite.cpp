/**
 * Copyright 2023 Comcast Cable Communications Management, LLC
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef USE_SQLITE

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "db_client.h"
#include "em_base.h"

// Structure to hold an active SQLite prepared statement
struct result_context_t {
    sqlite3_stmt *stmt;
};

int db_client_t::recreate_db()
{
    if (!m_con) {
        printf("%s:%d: No database connection\n", __func__, __LINE__);
        return -1;
    }

    if (m_db_path[0] == '\0') {
        printf("%s:%d: No database path stored\n", __func__, __LINE__);
        return -1;
    }

    // Close existing connection
    sqlite3_close(m_con);
    m_con = NULL;

    // Remove the database file
    if (unlink(m_db_path) != 0) {
        // File may not exist; not necessarily an error
        printf("%s:%d: Warning: could not remove db file '%s'\n",
               __func__, __LINE__, m_db_path);
    }

    // Re-open (creates a fresh empty database)
    if (sqlite3_open(m_db_path, &m_con) != SQLITE_OK) {
        printf("%s:%d: sqlite3_open() failed: %s\n",
               __func__, __LINE__, sqlite3_errmsg(m_con));
        m_con = NULL;
        return -1;
    }

    return 0;
}

void *db_client_t::execute(const char *query)
{
    if (!m_con) {
        printf("%s:%d: Query: %s  m_con is NULL, exiting\n",
               __func__, __LINE__, query ? query : "(null)");
        return NULL;
    }

    if (!query || query[0] == '\0') {
        printf("%s:%d: Empty or null query\n", __func__, __LINE__);
        return NULL;
    }

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(m_con, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s:%d: sqlite3_prepare_v2 failed: %s\n",
               __func__, __LINE__, sqlite3_errmsg(m_con));
        return NULL;
    }

    // Step once to start execution.  For DML (INSERT/UPDATE/DELETE/CREATE/DROP)
    // SQLITE_DONE means success.  For SELECT, SQLITE_ROW or SQLITE_DONE is
    // returned on the first step inside next_result(); here we do not step yet
    // so callers can iterate via next_result().
    //
    // For non-SELECT statements we step once and finalize immediately.
    int query_type = sqlite3_stmt_readonly(stmt);
    if (!query_type) {
        // Write statement – execute it now and finalize
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            printf("%s:%d: sqlite3_step failed: %s\n",
                   __func__, __LINE__, sqlite3_errmsg(m_con));
            return NULL;
        }
        return NULL;  // No rows to iterate for write statements
    }

    // Read statement – wrap in context for caller to iterate
    result_context_t *ctx = new result_context_t;
    ctx->stmt = stmt;
    return ctx;
}

bool db_client_t::next_result(void *ctx)
{
    if (ctx == NULL) {
        return false;
    }

    result_context_t *res_ctx = static_cast<result_context_t *>(ctx);

    int rc = sqlite3_step(res_ctx->stmt);
    if (rc == SQLITE_ROW) {
        return true;
    }

    // No more rows (SQLITE_DONE) or error – clean up
    sqlite3_finalize(res_ctx->stmt);
    delete res_ctx;
    return false;
}

char *db_client_t::get_string(void *ctx, char *str, unsigned int col)
{
    if (ctx == NULL || str == NULL) {
        return NULL;
    }

    result_context_t *res_ctx = static_cast<result_context_t *>(ctx);

    // col is 1-based; SQLite columns are 0-based
    if (col == 0) {
        return NULL;
    }

    const unsigned char *text = sqlite3_column_text(res_ctx->stmt,
                                                    static_cast<int>(col) - 1);
    if (text == NULL) {
        return NULL;
    }

    // Use column bytes for safe copy
    int bytes = sqlite3_column_bytes(res_ctx->stmt, static_cast<int>(col) - 1);
    snprintf(str, static_cast<size_t>(bytes) + 1, "%s",
             reinterpret_cast<const char *>(text));
    return str;
}

int db_client_t::get_number(void *ctx, unsigned int col)
{
    assert(ctx != NULL);

    result_context_t *res_ctx = static_cast<result_context_t *>(ctx);

    // col is 1-based; SQLite columns are 0-based
    if (col == 0) {
        return 0;
    }

    int type = sqlite3_column_type(res_ctx->stmt, static_cast<int>(col) - 1);
    if (type == SQLITE_NULL) {
        return 0;
    }
    if (type == SQLITE_TEXT) {
        // atoi on the text value – matches MariaDB behaviour
        const unsigned char *text = sqlite3_column_text(res_ctx->stmt,
                                                        static_cast<int>(col) - 1);
        return text ? atoi(reinterpret_cast<const char *>(text)) : 0;
    }

    return sqlite3_column_int(res_ctx->stmt, static_cast<int>(col) - 1);
}

int db_client_t::connect(const char *path)
{
    if (path == NULL || strlen(path) <= 0) {
        return -1;
    }

    // For SQLite the path is simply the file path of the database.
    // Store it for later use by recreate_db().
    strncpy(m_db_path, path, sizeof(m_db_path) - 1);
    m_db_path[sizeof(m_db_path) - 1] = '\0';

    if (sqlite3_open(m_db_path, &m_con) != SQLITE_OK) {
        printf("%s:%d: sqlite3_open('%s') failed: %s\n",
               __func__, __LINE__, m_db_path, sqlite3_errmsg(m_con));
        sqlite3_close(m_con);
        m_con = NULL;
        return -1;
    }

    printf("%s:%d: SQLite database opened: %s\n", __func__, __LINE__, m_db_path);
    return 0;
}

int db_client_t::init(const char *path)
{
    if (connect(path) != 0) {
        printf("%s:%d: Connect failed\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}

db_client_t::db_client_t()
{
    m_con = NULL;
    m_db_path[0] = '\0';
}

db_client_t::~db_client_t()
{
    if (m_con) {
        sqlite3_close(m_con);
        m_con = NULL;
    }
}

#endif /* USE_SQLITE */
