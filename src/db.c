/** \file db.c 
 *
 *  Code to handle database connections etc.
 *
 * this file is part of GLADDB
 *
 * \copyright 2012, 2013, 2014 Brett Sheffield <brett@gladserv.com>
 * \copyright 2017 Gavin Henry <ghenry@suretec.co.uk>, Suretec 
 *  Systems Ltd. T/A SureVoIP
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING in the distribution).
 * If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#define LDAP_DEPRECATED 1
#include "db.h"

#ifndef _NLDAP
#include "ldap.h"
#endif

#ifndef _NMY
#include "my.h"
#endif

#ifndef _NPG
#include "pg.h"
#endif

#ifndef _NTDS
#include "tds.h"
#endif

#ifndef _NLMDB
#include "lmdb.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/** Connect to specified database 
 * \param db A pointer to the connection is stored in db_t struct, 
 *           a wrapper for the database-specific functions
 * \return   0 on success, non-zero (-1) on failure
 */
int db_connect(db_t *db)
{
        dberrcode = NULL;
        dberror = NULL;

        if (db == NULL) {
                syslog(LOG_ERR, "No database info supplied to db_connect()\n");
                return -1;
        }
#ifndef _NLDAP
        if (strcmp(db->type, "ldap") == 0) {
                return db_connect_ldap(db);
        }
#endif
#ifndef _NMY
        if (strcmp(db->type, "my") == 0) {
                return db_connect_my(db);
        }
#endif
#ifndef _NPG
        if (strcmp(db->type, "pg") == 0) {
                return db_connect_pg(db);
        }
#endif
#ifndef _NTDS
        if (strcmp(db->type, "tds") == 0) {
                return db_connect_tds(db);
        }
#endif
#ifndef _NLMDB
        if (strcmp(db->type, "lmdb") == 0) {
                return db_connect_lmdb(db);
        }
#endif
        syslog(LOG_ERR,
                "Invalid database type '%s' passed to db_connect()\n",
                db->type);
        return -1;
}

/** Wrapper for the database-specific db creation functions 
 * \param db A pointer to a db_t struct
 * \return   0 on success, non-zero (-1) on failure
 */
int db_create(db_t *db)
{
        dberrcode = NULL;
        dberror = NULL;

        if (db == NULL) {
                fprintf(stderr,
                        "No database info supplied to db_create()\n");
                return -1;
        }
#ifndef _NPG
        if (strcmp(db->type, "pg") == 0) {
                return db_create_pg(db);
        }
#endif
        else {
                fprintf(stderr,
                    "Invalid database type '%s' passed to db_create()\n",
                    db->type);
        }
        return 0;
}

/** Wrapper for the database-specific db disconnect functions 
 * \param db A pointer to a db_t struct
 * \return   0 on success, non-zero (-1) on failure
 */
int db_disconnect(db_t *db)
{
        dberrcode = NULL;
        dberror = NULL;

        if (db == NULL) {
                fprintf(stderr,
                        "No database info supplied to db_disconnect()\n");
                return -1;
        }
        if (db->conn == NULL) {
                return 0;
        }
#ifndef _NLDAP
        if (strcmp(db->type, "ldap") == 0) {
                return db_disconnect_ldap(db);
        }
#endif
#ifndef _NMY
        if (strcmp(db->type, "my") == 0) {
                return db_disconnect_my(db);
        }
#endif
#ifndef _NPG
        if (strcmp(db->type, "pg") == 0) {
                return db_disconnect_pg(db);
        }
#endif
#ifndef _NTDS
        if (strcmp(db->type, "tds") == 0) {
                return db_disconnect_tds(db);
        }
#endif
#ifndef _NLMDB
        if (strcmp(db->type, "lmdb") == 0) {
                return db_disconnect_lmdb(db);
        }
#endif
        fprintf(stderr,
            "Invalid database type '%s' passed to db_disconnect()\n",
            db->type);
        return -1;
}

/** Wrapper for the database-specific db to execute some sql
 * \param db A pointer to a db_t struct
 * \param sql A pointer to a string containing your SQL syntax
 * \return   0 on success, non-zero (-1) on failure
 */
int db_exec_sql(db_t *db, char *sql)
{
        int isconn = 0;
        dberrcode = NULL;
        dberror = NULL;

        if (db == NULL) {
                fprintf(stderr,
                        "No database info supplied to db_exec_sql()\n");
                return -1;
        }

        /* connect if we aren't already */
        if (db->conn == NULL) {
                if (db_connect(db) != 0) {
                        syslog(LOG_ERR, "Failed to connect to db on %s",
                                db->host);
                        return -1;
                }
                isconn = 1;
        }
#ifndef _NPG
        if (strcmp(db->type, "pg") == 0) {
                return db_exec_sql_pg(db, sql);
        }
#endif
#ifndef _NMY
        if (strcmp(db->type, "my") == 0) {
                return db_exec_sql_my(db, sql);
        }
#endif
#ifndef _NTDS
        if (strcmp(db->type, "tds") == 0) {
                return db_exec_sql_tds(db, sql);
        }
#endif
        fprintf(stderr,
            "Invalid database type '%s' passed to db_exec_sql()\n",
            db->type);

        /* leave the connection how we found it */
        if (isconn == 1)
                db_disconnect(db);

        return 0;
}

/** Wrapper to return all results from an SQL type SELECT function
 * \param db        A pointer to a db_t struct
 * \param sql       A pointer to your string containing your SQL syntax
 * \param filter    A pointer to a field_t to filter your results
 * \param rows      A pointer to a pointer to process all rows
 * \param rowc      A int to store total amount of rows
 * \return   0 on success, non-zero (-1) on failure.
 */
int db_fetch_all(db_t *db, char *sql, field_t *filter, row_t **rows, int *rowc)
{
        dberrcode = NULL;
        dberror = NULL;

        if (db == NULL) {
                syslog(LOG_ERR,
                        "No database info supplied to db_fetch_all()\n");
                return -1;
        }
#ifndef _NPG
        if (strcmp(db->type, "pg") == 0) {
                return db_fetch_all_pg(db, sql, filter, rows, rowc);
        }
#endif
#ifndef _NMY
        if (strcmp(db->type, "my") == 0) {
                return db_fetch_all_my(db, sql, filter, rows, rowc);
        }
#endif
#ifndef _NTDS
        if (strcmp(db->type, "tds") == 0) {
                return db_fetch_all_tds(db, sql, filter, rows, rowc);
        }
#endif
#ifndef _NLDAP
        if (strcmp(db->type, "ldap") == 0) {
                return db_fetch_all_ldap(db, sql, filter, rows, rowc);
        }
#endif
#ifndef _NLMDB
        if (strcmp(db->type, "lmdb") == 0) {
                return db_fetch_all_lmdb(db, sql, filter, rows, rowc);
        }
#endif
        syslog(LOG_ERR,
            "Invalid database type '%s' passed to db_fetch_all()\n",
            db->type);
        return -1;
}

/** Wrapper to insert/put/add records into a backend database
 * \param db        A pointer to a db_t struct
 * \param resource  A pointer to your string containing a resource name
 * \param data      A pointer to a keyval_t containing your data to
 *                  insert
 * \return   0 on success, non-zero (-1) on failure.
 */
int db_insert(db_t *db, char *resource, keyval_t *data)
{
        dberrcode = NULL;
        dberror = NULL;

        if (db == NULL) {
                syslog(LOG_ERR,
                        "No database info supplied to db_insert()\n");
                return -1;
        }
#if !defined(_NPG) || !defined(_NMY) || !defined(_NTDS)
        if ((strcmp(db->type, "pg") == 0) || (strcmp(db->type, "my") == 0) ||
            (strcmp(db->type, "tds") == 0))
        {
                return db_insert_sql(db, resource, data);
        }
#endif
#ifndef _NLDAP
        if (strcmp(db->type, "ldap") == 0) {
                return db_insert_ldap(db, resource, data);
        }
#endif
#ifndef _NLMDB
        if (strcmp(db->type, "lmdb") == 0) {
                return db_insert_lmdb(db, resource, data);
        }
#endif
        syslog(LOG_ERR, "Invalid database type '%s' passed to db_insert()\n",
                db->type);
        return -1;
}

/** Wrapper to insert SQL into a rdbms 
 * \param db        A pointer to a db_t struct
 * \param resource  A pointer to your string containing a resource name
 * \param data      A pointer to a keyval_t containing your data to
 *                  insert
 * \return   0 on success, non-zero (-1) on failure.
 */
int db_insert_sql(db_t *db, char *resource, keyval_t *data)
{
        char *flds = NULL;
        char *sql;
        char *vals = NULL;
        char *tmpflds = NULL;
        char *tmpvals = NULL;
        char quot = '\'';
        int rval;
        int isconn = 0;

        dberrcode = NULL;
        dberror = NULL;

        /* use backticks to quote mysql */
        if (strcmp(db->type, "my") == 0)
                quot = '"';

        /* build INSERT sql from supplied data */
        while (data != NULL) {
                fprintf(stderr, "%s = %s\n", data->key, data->value);
                if (flds == NULL) {
                        asprintf(&flds, "%s", data->key);
                        asprintf(&vals, "%1$c%2$s%1$c", quot, data->value);
                }
                else {
                        tmpflds = strdup(flds);
                        tmpvals = strdup(vals);
                        free(flds); free(vals);
                        asprintf(&flds, "%s,%s", tmpflds, data->key);
                        asprintf(&vals, "%2$s,%1$c%3$s%1$c",
                                quot, tmpvals, data->value);
                        free(tmpflds); free(tmpvals);
                }
                data = data->next;
        }

        asprintf(&sql,"INSERT INTO %s (%s) VALUES (%s)", resource, flds, vals);
        free(flds); free(vals);
        syslog(LOG_DEBUG, "%s", sql);

        if (db->conn == NULL) {
                if (db_connect(db) != 0) {
                        syslog(LOG_ERR, "Failed to connect to db on %s",
                                db->host);
                        return -1;
                }
                isconn = 1;
        }

        rval = db_exec_sql(db, sql);
        free(sql);

        /* leave the connection how we found it */
        if (isconn == 1)
                db_disconnect(db);

        return rval;
}

/** Return a field with name fname, from provided row 
 * \param row   A pointer to a db_t struct
 * \param fname A pointer to your string containing the field name
 * \return      Pointer to a field_t or NULL if not found
 */
field_t * db_field(row_t *row, char *fname)
{
        field_t *f;
        dberrcode = NULL;
        dberror = NULL;

        f = row->fields;
        while (f != NULL) {
                if (strcmp(fname, f->fname) == 0)
                        return f;
                f = f->next;
        }

        return '\0';
}

/** Free database struct 
 * \param dbs   A pointer to a db_t struct
 * \return      Nothing
 */
void db_free(db_t *dbs)
{
        db_t *d;
        db_t *tmp;

        dberrcode = NULL;
        dberror = NULL;

        d = dbs;
        while (d != NULL) {
                free(d->alias);
                free(d->type);
                free(d->host);
                free(d->db);
                free(d->user);
                free(d->pass);
                tmp = d;
                d = d->next;
                free(tmp);
        }
        dbs = NULL;
}

/** Return the db_t pointer for this db alias
 * \param dbs   A pointer to a db_t struct
 * \param alias A pointer to hold the alias returned
 * \return      Pointer to a db_t or NULL if not found
 */
db_t *db_get(db_t *dbs, char *alias)
{
        db_t *db;

        dberrcode = NULL;
        dberror = NULL;

        db = dbs;
        while (db != NULL) {
                if (strcmp(alias, db->alias) == 0)
                        return db;
                db = db->next;
        }

        return NULL; /* db not found */
}

/** Free field_t struct 
 * \param f     A pointer to a field_t struct
 * \return      Nothing
 */
void free_fields(field_t *f)
{
        dberrcode = NULL;
        dberror = NULL;

        field_t *next_f = NULL;
        while (f != NULL) {
                free(f->fname);
                free(f->fvalue);
                next_f = f->next;
                free(f);
                f = next_f;
        }
}

/** Free row_t struct
 * \param r     A pointer to a row_t struct
 * \return      Nothing
 */
void liberate_rows(row_t *r)
{
        dberrcode = NULL;
        dberror = NULL;

        row_t *next_r = NULL;
        while (r != NULL) {
                free_fields(r->fields);
                next_r = r->next;
                free(r);
                r = next_r;
        }
}

/** Count keyvals and return total, but increment unique counts
 * \param kv        A pointer to a keyval_t struct to count
 * \param total     A pointer to an int for your total count
 * \param unique    A pointer to an int for counting unique items
 * \return          total keys
 */
int count_keyvals(keyval_t *kv, int *total, int *unique)
{
        char *last = kv->key;
        dberrcode = NULL;
        dberror = NULL;

        if (kv != NULL) (*unique) = 1;
        while (kv != NULL) {
                if (strcmp(last, kv->key) != 0) {
                        last = kv->key;
                        (*unique)++;
                }
                (*total)++;
                kv = kv->next;
        }
        return *total;
}
