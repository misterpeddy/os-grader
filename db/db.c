#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 

#define MAX_FILENAME_LEN 128

#define DB_PATH "test_database.db"
#define MAX_SQL_COMMAND_LEN 1024
#define DB_TABLE_NAME "SUBMISSIONS"

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
	int i;
	for(i=0; i<argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

/*
** Opens connection to the supplied database.
** Creates db if it does not exist.
** Returns 1 if an error occured, 0 otherwise.
*/
int open_db(sqlite3 **db, char *db_path) {

  // Open connection and heck return code for errors
	if(sqlite3_open(db_path, db)){
		printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return 1;
	}

  printf("Successfully connected to the database\n");
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
	sprintf(sql_command, 
      "CREATE TABLE %s("  \
      "USER     CHAR(%d) PRIMARY KEY NOT NULL," \
      "ASS_NUM  CHAR(32)             NOT NULL," \
      "STATUS   INT                  NOT NULL);"
      , DB_TABLE_NAME, MAX_FILENAME_LEN);

	// Execute SQL statement
	int return_code = sqlite3_exec(db, sql_command, callback, 0, &error_message);

  // Check return code
	if(return_code != SQLITE_OK){
    printf("SQL error: %s\n", error_message);
    sqlite3_free(error_message);
    return 1;
	}

  printf("Table %s created successfully\n", DB_TABLE_NAME);
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

int main(int argc, char* argv[])
{
	// Declare handle to the database file
  sqlite3 *db;

  // Open connection to the database
  if (open_db(&db, DB_PATH))
    exit(EXIT_FAILURE);

  // Create a submissions table 
  if (create_table(db)) {
    close_db(db);
    exit(EXIT_FAILURE);
  }

	close_db(db);

	return 0;
}
