#ifndef _APPSQL_H
#define _APPSQL_H

#include <mysql/mysql.h>
#include "command.h"
#include "json.h"
#include "nginx.h"
#include "list.h"
#include "app.h"

struct app_sql_value_t{
	char *start;
	unsigned int len;
};

typedef struct app_sql_value_t	  	app_sql_value;
typedef struct app_sql_value_t* 	app_sql_line;
typedef struct app_sql_value_t** 	app_sql_set;

unsigned int BKDRHash(char *);
void * app_query(MYSQL *mysql, char *, ngx_command_t *);
int app_select(stone_server_t *server, char *, char *, struct list_head *, int, int, ngx_command_t *);
unsigned int app_update(stone_server_t *server, char * table, struct list_head * fields, struct list_head * where, ngx_command_t *cmd);
#endif
