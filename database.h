#ifndef DATABASE_H
#define DATABASE_H

extern sqlite3 *db;
extern int rc;
extern char *zErrMsg;

int initDB(sqlite3 *db);

#endif