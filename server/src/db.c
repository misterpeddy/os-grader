#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "db.h"
#include "macros.h"

#define MAX_FILENAME_LEN 128

/*
** To be used as the callback to sqlite3_exec() for SELECT statements.
** Assumes the first argument is a string, which the response is written to, with following format:
** [<TIMESTAMP><SQL_COL_DELIM><USER><SQL_COL_DELIM><MODULE NUMBER>
** <SQL_COL_DELIM><STATUS CODE><SQL_LINE_DELIM>]*
** If no rows are returned, it will leave passed_buffer untouched.
** Non-zero return value would cause a SQLITE_ABORT return value for the responsible sqlite3_exec() call. 
*/
static int record_retrieval_callback(void *passed_buffer, int argc, char **argv, char **column_names){

  // Find the first empty spot in the response buffer 
  char *response_buffer = (char *)passed_buffer;
  int k=0;
  while(response_buffer[k++]);
    response_buffer += k - 1;	

    // Write the response in the specified format
    sprintf(response_buffer
      ,"%s%s%s%s%s%s%s%s" 
      , argv[0] ? argv[0] : NULL_STR, SQL_COL_DELIM
      , argv[1] ? argv[1] : NULL_STR, SQL_COL_DELIM
      , argv[2] ? argv[2] : NULL_STR, SQL_COL_DELIM
      , argv[3] ? argv[3] : NULL_STR, SQL_LINE_DELIM);

  return 0;
}

/*
** Opens connection to the supplied database.
** Creates db if it does not exist.
** Returns 1 if an error occured, 0 otherwise.
*/
int open_db(sqlite3 **db, char *db_path) {
  
  // Open connection and check return code for errors
  if(sqlite3_open(db_path, db)) return 1;

  if (DEBUG) printf("Successfully connected to the database\n");
  return 0;
}

/*
** Creates an empty submissions table
** Returns 1 if an error occured, 0 otherwise.
*/
int create_table(sqlite3 *db) {
  // Declare error buffer
  char *error_message;

  // Create SQL statement 
  char sql_command[MAX_SQL_COMMAND_LEN];
  memset(sql_command, 0, MAX_SQL_COMMAND_LEN);
  sprintf(sql_command, 
    "CREATE TABLE %s("			/* DB_TABLE_NAME    */ 
    "%s   	CHAR(%d)   NOT NULL," 	/* DB_COL_TIMESTAMP */
    "%s     	CHAR(%d)   NOT NULL," 	/* DB_COL_USER      */
    "%s  	CHAR(%d)   NOT NULL,"  	/* DB_COL_MODNUM    */
    "%s   	CHAR(%d)   NOT NULL);" 	/* DB_COL_RESULT    */
    , DB_TABLE_NAME
    , DB_COL_TIMESTAMP, MAX_TIMESTAMP_LEN
    , DB_COL_USER, MAX_FILENAME_LEN
    , DB_COL_MODNUM, MAX_MODNUM_DIGITS
    , DB_COL_RESULT, MAX_ACK_LEN);

  // Execute SQL statement
  int return_code = sqlite3_exec(db, sql_command, record_retrieval_callback, 0, &error_message);

  // Check return code
  if(return_code != SQLITE_OK){
    sqlite3_free(error_message);
    return 1;
  }

  if (DEBUG) printf("Table %s created successfully\n", DB_TABLE_NAME);
  return 0;
}

/*
** Assuming a valid db connection, insert_record inserts a row
** in DB_TABLE_NAME with the provided values. All arguments must be
** non-NULL. Returns 1 on error and 0 otherwise.
*/
int insert_record(sqlite3 *db, char *user, char *module_num, char *result) {
  // Declare error buffer and return code
  char *error_message;
  int return_code;

  // Compute current time
  char timestamp[MAX_TIMESTAMP_LEN];
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  memset(timestamp, 0, MAX_TIMESTAMP_LEN);
  sprintf(timestamp, "%d-%d-%d-%d-%d-%d",
      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
      tm.tm_hour, tm.tm_min, tm.tm_sec);

  // Create SQL statement 
  char sql_command[MAX_SQL_COMMAND_LEN];
  memset(sql_command, 0, MAX_SQL_COMMAND_LEN);
  sprintf(sql_command, 		
    "INSERT INTO %s "
    "(%s, %s, %s, %s) "
    "VALUES ('%s', '%s', '%s', '%s'); "
    , DB_TABLE_NAME 
    , DB_COL_TIMESTAMP, DB_COL_USER, DB_COL_MODNUM, DB_COL_RESULT
    , timestamp, user, module_num, result);

  // Execute SQL statement
  return_code = sqlite3_exec(db, sql_command, record_retrieval_callback, 0, &error_message);
  if(return_code != SQLITE_OK) {
    printf("SQL error: %s\n", error_message);
    sqlite3_free(error_message);
    return 1;
  }

  if (DEBUG) printf("Result <%s, %s, %s, %s> was recorded successfully\n", 
    timestamp, user, module_num, result);
  return 0;
}

/*
** Assuming a valid db connection, lookup_user looks up all entries for a user
** in the database and writes the response to response_buffer with following format:
** [<USER><SQL_COL_DELIM><MODULE NUMBER><SQL_COL_DELIM>
** <RESULT CODE><SQL_COL_DELIM><TIMESTAMP><SQL_LINE_DELIM>]*
** Returns 1 if any errors occur, otherwise 0.
*/
int lookup_user(sqlite3 *db, char *user, char *response_buffer) {
  // Make sure input is valid
  if (strchr(user, ' ')) {
    printf("Username (%s) cannot contain space\n");
    exit(EXIT_FAILURE);
  }

  // Declare error buffer and return code
  char *error_message;
  int return_code;

  // Create SQL statement
  char sql_command[MAX_SQL_COMMAND_LEN];
  memset(sql_command, 0, MAX_SQL_COMMAND_LEN);
  sprintf(sql_command,
    "SELECT * FROM %s WHERE %s='%s'"
    , DB_TABLE_NAME, DB_COL_USER, user);
	
  // Execute SQL statement
  return_code = sqlite3_exec(db, sql_command, record_retrieval_callback, response_buffer, &error_message);
  if (return_code != SQLITE_OK) {
    printf("SQL error: %s\n", error_message);
    sqlite3_free(error_message);
    return 1;
  }

  if (DEBUG) printf("Succesfully retrieved records for user %s\n", user);
  return 0;
}

/*
** Assuming a valid db connection, lookup_module looks up all entries for an module
** in the database and writes the response to response_buffer with following format:
** [<USER><SQL_COL_DELIM><MODULE NUMBER><SQL_COL_DELIM>
** <RESULT CODE><SQL_COL_DELIM><TIMESTAMP><SQL_LINE_DELIM>]*
** Returns 1 if any errors occur, otherwise 0.
*/
int lookup_module(sqlite3 *db, char *module_num, char *response_buffer) {
  // Make sure input is valid
  if (strchr(module_num, ' ')) {
    printf("Module name (%s) cannot contain space\n");
    exit(EXIT_FAILURE);
  }

  // Declare error buffer and return code
  char *error_message;
  int return_code;

  // Create SQL statement
  char sql_command[MAX_SQL_COMMAND_LEN];
  memset(sql_command, 0, MAX_SQL_COMMAND_LEN);
  sprintf(sql_command,
    "SELECT * FROM %s WHERE %s='%s'"
    , DB_TABLE_NAME, DB_COL_MODNUM, module_num);
	
  // Execute SQL statement
  return_code = sqlite3_exec(db, sql_command, record_retrieval_callback, response_buffer, &error_message);
  if (return_code != SQLITE_OK) {
    printf("SQL error: %s\n", error_message);
    sqlite3_free(error_message);
    return 1;
  }

  if (DEBUG) printf("Succesfully retrieved records for module %s\n", module_num);
  return 0;
}

/*
** Closes the connection to the database.
** If there are unfinalized prepared statements, the handle
** will become a zombie and 1 is returned. Otherwise 0 is returned.
*/
int close_db(sqlite3 *db) {
  if (sqlite3_close(db) != SQLITE_OK) {
    printf("Could not succesfully destroy connection to database - It is now a zombie\n");
    return 1;
  }

  return 0;
}
 
int example_db_main(int argc, char* argv[])
{
  // Declare handle to the database file
  sqlite3 *db;

  // Open connection to the database
  if (open_db(&db, DB_PATH))
		return 1;

  // Create a submissions table 
  if (create_table(db)) {
    close_db(db);
    exit(EXIT_FAILURE);
  }
	
  // Insert a recrod into the table
  insert_record(db, "pp5nv", "0", JDG_AOK);

  // Look up the records for pp5nv
  char response[MAX_SQL_RESPONSE_LEN];
  memset(response, 0, MAX_SQL_RESPONSE_LEN);
  lookup_user(db, "pp5nv", (char *)&response);
  printf("Response: \n%s", response);
	
  // Close the db connection and clean up
  if (close_db(db))
    return 1;

  return 0;
}
