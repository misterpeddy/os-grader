
#define MAX_SQL_RESPONSE_LEN  1024
#define MAX_SQL_COMMAND_LEN   512
#define MAX_ASSNUM_DIGITS     5
#define MAX_ACK_LEN           8
#define DB_PATH               "test_database.db"

#define DB_TABLE_NAME         "SUBMISSIONS"
#define DB_COL_USER           "USER"
#define DB_COL_ASSNUM         "ASS_NUM"
#define DB_COL_RESULT         "RESULT"

#define SQL_COL_DELIM         "*"
#define SQL_LINE_DELIM        "\n"
#define NULL_STR              "NULL"


/* Acknowledgment messages */
#define CMP_AOK   "CMP_AOK" /* Compilation succeeded        */
#define CMP_ERR   "CMP_ERR" /* Compilation failed     FATAL */
#define RUN_AOK   "RUN_AOK" /* A run succeeded              */
#define RUN_ERR   "RUN_ERR" /* A run failed           FATAL */
#define TIM_OUT   "TIM_OUT" /* A run timed out        FATAL */
#define CHK_AOK   "CHK_AOK" /* A diff succeeded             */
#define CHK_ERR   "CHK_ERR" /* A diff failed          FATAL */

#define JDG_ERR   "JDG_ERR" /* Solution not accepted        */
#define JDG_AOK   "JDG_AOK" /* Solution accepted            */

#define RCV_AOK   "RCV_AOK" /* Client request received      */

