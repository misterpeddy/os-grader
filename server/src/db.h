#ifndef DB_H
#define DB_H

#include <sqlite3.h> 

#define DB_PATH       "db/staging.db"

#define MAX_SQL_RESPONSE_LEN  1<<10
#define MAX_SQL_COMMAND_LEN   1<<9
#define MAX_MODNUM_DIGITS     5
#define MAX_ACK_LEN           1<<3
#define MAX_TIMESTAMP_LEN     1<<5

#define DB_TABLE_NAME         "SUBMISSIONS"
#define DB_COL_USER           "USER"
#define DB_COL_MODNUM         "MODULE_NUM"
#define DB_COL_RESULT         "RESULT"
#define DB_COL_TIMESTAMP      "TIMESTAMP"

#define SQL_COL_DELIM         "*"
#define SQL_LINE_DELIM        "\n"
#define NULL_STR              "NULL"

/*
** To be used as the callback to sqlite3_exec() for SELECT statements.
** Assumes the first argument is a string, which the response is written to, with following format:
** [<USER><SQL_COL_DELIM><MODULE NUMBER><SQL_COL_DELIM><STATUS CODE><SQL_LINE_DELIM>]*
** If no rows are returned, it will leave passed_buffer untouched.
** Non-zero return value would cause a SQLITE_ABORT return value for the responsible sqlite3_exec() call. 
*/
static int record_retrieval_callback(void *passed_buffer, int argc, char **argv, char **column_names);

/*
** Opens connection to the supplied database.
** Creates db if it does not exist.
** Returns 1 if an error occured, 0 otherwise.
*/
int open_db(sqlite3 **db, char *db_path);

/*
** Creates an empty submissions table
** Returns 1 if an error occured, 0 otherwise.
*/
int create_table(sqlite3 *db);

/*
** Assuming a valid db connection, insert_record inserts a row
** in DB_TABLE_NAME with the provided values. All arguments must be
** non-NULL. Returns 1 on error and 0 otherwise.
*/
int insert_record(sqlite3 *db, char *user, char *module_num, char *result);

/*
** Assuming a valid db connection, lookup_user looks up all entries for a user
** in the database and writes the response to response_buffer with following format:
** [<USER><SQL_COL_DELIM><MODULE NUMBER><SQL_COL_DELIM><RESULT CODE><SQL_LINE_DELIM>]*
** Returns 1 if any errors occur, otherwise 0.
*/
int lookup_user(sqlite3 *db, char *user, char *response_buffer);

/*
** Assuming a valid db connection, lookup_module looks up all entries for an module
** in the database and writes the response to response_buffer with following format:
** [<USER><SQL_COL_DELIM><MODULE NUMBER><SQL_COL_DELIM><STATUS CODE><SQL_LINE_DELIM>]*
** Returns 1 if any errors occur, otherwise 0.
*/
int lookup_module(sqlite3 *db, char *module_num, char *response_buffer);

/*
** Closes the connection to the database.
** If there are unfinalized prepared statements, the handle
** will become a zombie and 1 is returned. Otherwise 0 is returned.
*/
int close_db(sqlite3 *db);

#endif
