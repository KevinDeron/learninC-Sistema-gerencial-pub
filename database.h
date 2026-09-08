#ifndef DATABASE_H
#define DATABASE_H

extern sqlite3 *db;
extern sqlite3_stmt *stmt;
extern int rc;
extern char *zErrMsg;
extern char *sql;

int initDB(sqlite3 *db);

#endif