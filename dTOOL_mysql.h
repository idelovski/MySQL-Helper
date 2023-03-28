/*
 *  dTOOL_mysql.h
 *  Bouquet - loading library on Windows, does nothing on Mac
 *  My Win compiler can't handle mysql.lib directly
 *
 *  Created by Igor Delovski on 02.07.2011.
 *  Copyright 2011, 2016, 2020 Igor Delovski All rights reserved.
 *
 */

#ifdef _DTOOL_WIN_

typedef MYSQL *(STDCALL *idptr_mysql_init)(MYSQL *mysql);

typedef int    (STDCALL *idptr_mysql_options)(MYSQL *mysql, enum mysql_option option, const void *arg);

typedef MYSQL *(STDCALL *idptr_mysql_real_connect)(MYSQL *mysql, const char *host, const char *user,
                                                   const char *passwd, const char *db, unsigned int port,
                                                   const char *unix_socket, unsigned long clientflag);

typedef int    (STDCALL *idptr_mysql_select_db)(MYSQL *mysql, const char *db);

typedef int    (STDCALL *idptr_mysql_server_init)(int argc, char **argv, char **groups);
typedef void   (STDCALL *idptr_mysql_server_end)(void);

typedef int    (STDCALL *idptr_mysql_real_query)(MYSQL *mysql, const char *q, unsigned long length);
typedef const char  *(STDCALL *idptr_mysql_character_set_name)(MYSQL *mysql);
typedef int          (STDCALL *idptr_mysql_set_character_set)(MYSQL *mysql, const char *csname);

typedef my_ulonglong  (STDCALL *idptr_mysql_num_rows)(MYSQL_RES *res);
typedef unsigned int  (STDCALL *idptr_mysql_num_fields)(MYSQL_RES *res);

typedef void   (STDCALL *idptr_mysql_free_result)(MYSQL_RES *result);
typedef void   (STDCALL *idptr_mysql_data_seek)(MYSQL_RES *result, my_ulonglong offset);

typedef MYSQL_ROW_OFFSET    (STDCALL *idptr_mysql_row_seek)(MYSQL_RES *result, MYSQL_ROW_OFFSET offset);
typedef MYSQL_FIELD_OFFSET  (STDCALL *idptr_mysql_field_seek)(MYSQL_RES *result, MYSQL_FIELD_OFFSET offset);

typedef unsigned int (STDCALL *idptr_mysql_errno)(MYSQL *mysql);
typedef const char *(STDCALL *idptr_mysql_error)(MYSQL *mysql);

// ----- STMT

typedef MYSQL_STMT  *(STDCALL *idptr_mysql_stmt_init)(MYSQL *mysql);

typedef int (STDCALL *idptr_mysql_stmt_prepare)(MYSQL_STMT *stmt, const char *query,
                               unsigned long length);
typedef int (STDCALL *idptr_mysql_stmt_execute)(MYSQL_STMT *stmt);
typedef int (STDCALL *idptr_mysql_stmt_fetch)(MYSQL_STMT *stmt);
typedef int (STDCALL *idptr_mysql_stmt_fetch_column)(MYSQL_STMT *stmt, MYSQL_BIND *bind_arg, 
                                    unsigned int column,
                                    unsigned long offset);
typedef int (STDCALL *idptr_mysql_stmt_store_result)(MYSQL_STMT *stmt);

typedef unsigned long  (STDCALL *idptr_mysql_stmt_param_count)(MYSQL_STMT * stmt);

typedef my_bool (STDCALL *idptr_mysql_stmt_attr_set)(MYSQL_STMT *stmt,
                                                     enum enum_stmt_attr_type attr_type,
                                                     const void *attr);
typedef my_bool (STDCALL *idptr_mysql_stmt_attr_get)(MYSQL_STMT *stmt,
                                                     enum enum_stmt_attr_type attr_type,
                                                     void *attr);
typedef my_bool (STDCALL *idptr_mysql_stmt_bind_param)(MYSQL_STMT *stmt, MYSQL_BIND *bnd);
typedef my_bool (STDCALL *idptr_mysql_stmt_bind_result)(MYSQL_STMT *stmt, MYSQL_BIND *bnd);
typedef my_bool (STDCALL *idptr_mysql_stmt_close)(MYSQL_STMT *stmt);
typedef my_bool (STDCALL *idptr_mysql_stmt_reset)(MYSQL_STMT *stmt);
typedef my_bool (STDCALL *idptr_mysql_stmt_free_result)(MYSQL_STMT *stmt);
typedef my_bool (STDCALL *idptr_mysql_stmt_send_long_data)(MYSQL_STMT *stmt, 
                                                           unsigned int param_number,
                                                           const char *data, 
                                                           unsigned long length);

typedef MYSQL_RES    *(STDCALL *idptr_mysql_stmt_result_metadata)(MYSQL_STMT *stmt);
typedef MYSQL_RES    *(STDCALL *idptr_mysql_stmt_param_metadata)(MYSQL_STMT *stmt);
typedef unsigned int  (STDCALL *idptr_mysql_stmt_errno)(MYSQL_STMT *stmt);
typedef const char   *(STDCALL *idptr_mysql_stmt_error)(MYSQL_STMT *stmt);
typedef const char   *(STDCALL *idptr_mysql_stmt_sqlstate)(MYSQL_STMT *stmt);

typedef MYSQL_ROW_OFFSET  (STDCALL *idptr_mysql_stmt_row_seek)(MYSQL_STMT *stmt, MYSQL_ROW_OFFSET offset);
typedef MYSQL_ROW_OFFSET  (STDCALL *idptr_mysql_stmt_row_tell)(MYSQL_STMT *stmt);

typedef void   (STDCALL *idptr_mysql_stmt_data_seek)(MYSQL_STMT *stmt, my_ulonglong offset);

typedef my_ulonglong (STDCALL *idptr_mysql_stmt_num_rows)(MYSQL_STMT *stmt);
typedef my_ulonglong (STDCALL *idptr_mysql_stmt_affected_rows)(MYSQL_STMT *stmt);
typedef my_ulonglong (STDCALL *idptr_mysql_stmt_insert_id)(MYSQL_STMT *stmt);
typedef unsigned int (STDCALL *idptr_mysql_stmt_field_count)(MYSQL_STMT *stmt);

typedef void   (STDCALL *idptr_mysql_close)(MYSQL *sock);

// ----------------------------------------------------------------------------- statics
            
static HMODULE                   gGhMySQLModule = NULL;   // for LoadLibrary()

static idptr_mysql_init          p_mysql_init = NULL;

static idptr_mysql_options       p_mysql_options = NULL;
static idptr_mysql_real_connect  p_mysql_real_connect = NULL;

static idptr_mysql_select_db     p_mysql_select_db = NULL;

static idptr_mysql_server_init   p_mysql_server_init = NULL;
static idptr_mysql_server_end    p_mysql_server_end = NULL;

static idptr_mysql_real_query          p_mysql_real_query = NULL;
static idptr_mysql_character_set_name  p_mysql_character_set_name = NULL;
static idptr_mysql_set_character_set   p_mysql_set_character_set = NULL;

static idptr_mysql_num_rows      p_mysql_num_rows = NULL;
static idptr_mysql_num_fields    p_mysql_num_fields = NULL;

static idptr_mysql_free_result   p_mysql_free_result = NULL;
static idptr_mysql_data_seek     p_mysql_data_seek = NULL;

static idptr_mysql_row_seek      p_mysql_row_seek = NULL;
static idptr_mysql_field_seek    p_mysql_field_seek = NULL;

static idptr_mysql_errno         p_mysql_errno = NULL;
static idptr_mysql_error         p_mysql_error = NULL;

// ----- STMT

static idptr_mysql_stmt_init     p_mysql_stmt_init = NULL;
static idptr_mysql_stmt_prepare  p_mysql_stmt_prepare = NULL;
static idptr_mysql_stmt_execute  p_mysql_stmt_execute = NULL;
static idptr_mysql_stmt_fetch    p_mysql_stmt_fetch = NULL;

static idptr_mysql_stmt_fetch_column    p_mysql_stmt_fetch_column = NULL;
static idptr_mysql_stmt_store_result    p_mysql_stmt_store_result = NULL;
static idptr_mysql_stmt_param_count     p_mysql_stmt_param_count = NULL;

static idptr_mysql_stmt_attr_set        p_mysql_stmt_attr_set = NULL;
static idptr_mysql_stmt_attr_get        p_mysql_stmt_attr_get = NULL;

static idptr_mysql_stmt_bind_param      p_mysql_stmt_bind_param = NULL;
static idptr_mysql_stmt_bind_result     p_mysql_stmt_bind_result = NULL;
static idptr_mysql_stmt_close           p_mysql_stmt_close = NULL;
static idptr_mysql_stmt_reset           p_mysql_stmt_reset = NULL;
static idptr_mysql_stmt_free_result     p_mysql_stmt_free_result = NULL;
static idptr_mysql_stmt_send_long_data  p_mysql_stmt_send_long_data = NULL;

static idptr_mysql_stmt_result_metadata p_mysql_stmt_result_metadata = NULL;

static idptr_mysql_stmt_param_metadata  p_mysql_stmt_param_metadata = NULL;
static idptr_mysql_stmt_errno           p_mysql_stmt_errno = NULL;
static idptr_mysql_stmt_error           p_mysql_stmt_error = NULL;
static idptr_mysql_stmt_sqlstate        p_mysql_stmt_sqlstate = NULL;

static idptr_mysql_stmt_row_seek        p_mysql_stmt_row_seek = NULL;
static idptr_mysql_stmt_row_tell        p_mysql_stmt_row_tell = NULL;

static idptr_mysql_stmt_data_seek       p_mysql_stmt_data_seek = NULL;

static idptr_mysql_stmt_num_rows        p_mysql_stmt_num_rows = NULL;
static idptr_mysql_stmt_affected_rows   p_mysql_stmt_affected_rows = NULL;
static idptr_mysql_stmt_insert_id       p_mysql_stmt_insert_id = NULL;
static idptr_mysql_stmt_field_count     p_mysql_stmt_field_count = NULL;

static idptr_mysql_close                p_mysql_close = NULL;

#endif

int  id_LoadDllMySQL ();  // In dTOOL_Ctr.C, real code on Windows, nothing on Mac


#ifdef _DTOOL_WIN_

#define  id_mysql_init          p_mysql_init

#define  id_mysql_options       p_mysql_options
#define  id_mysql_real_connect  p_mysql_real_connect
#define  id_mysql_select_db     p_mysql_select_db

#define  id_mysql_server_init   p_mysql_server_init
#define  id_mysql_server_end    p_mysql_server_end

#define  id_mysql_real_query          p_mysql_real_query
#define  id_mysql_character_set_name  p_mysql_character_set_name
#define  id_mysql_set_character_set   p_mysql_set_character_set

#define  id_mysql_num_rows      p_mysql_num_rows
#define  id_mysql_num_fields    p_mysql_num_fields

#define  id_mysql_free_result   p_mysql_free_result
#define  id_mysql_data_seek     p_mysql_data_seek

#define  id_mysql_row_seek      p_mysql_row_seek
#define  id_mysql_field_seek    p_mysql_field_seek

#define  id_mysql_errno         p_mysql_errno
#define  id_mysql_error         p_mysql_error

// ----- STMT

#define  id_mysql_stmt_init     p_mysql_stmt_init
#define  id_mysql_stmt_prepare  p_mysql_stmt_prepare
#define  id_mysql_stmt_execute  p_mysql_stmt_execute
#define  id_mysql_stmt_fetch    p_mysql_stmt_fetch

#define  id_mysql_stmt_fetch_column    p_mysql_stmt_fetch_column
#define  id_mysql_stmt_store_result    p_mysql_stmt_store_result
#define  id_mysql_stmt_param_count     p_mysql_stmt_param_count

#define  id_mysql_stmt_attr_set        p_mysql_stmt_attr_set
#define  id_mysql_stmt_attr_get        p_mysql_stmt_attr_get

#define  id_mysql_stmt_bind_param      p_mysql_stmt_bind_param
#define  id_mysql_stmt_bind_result     p_mysql_stmt_bind_result
#define  id_mysql_stmt_close           p_mysql_stmt_close
#define  id_mysql_stmt_reset           p_mysql_stmt_reset
#define  id_mysql_stmt_free_result     p_mysql_stmt_free_result
#define  id_mysql_stmt_send_long_data  p_mysql_stmt_send_long_data

#define  id_mysql_stmt_result_metadata p_mysql_stmt_result_metadata

#define  id_mysql_stmt_param_metadata  p_mysql_stmt_param_metadata
#define  id_mysql_stmt_errno           p_mysql_stmt_errno
#define  id_mysql_stmt_error           p_mysql_stmt_error
#define  id_mysql_stmt_sqlstate        p_mysql_stmt_sqlstate

#define  id_mysql_stmt_row_seek        p_mysql_stmt_row_seek
#define  id_mysql_stmt_row_tell        p_mysql_stmt_row_tell

#define  id_mysql_stmt_data_seek       p_mysql_stmt_data_seek

#define  id_mysql_stmt_num_rows        p_mysql_stmt_num_rows
#define  id_mysql_stmt_affected_rows   p_mysql_stmt_affected_rows
#define  id_mysql_stmt_insert_id       p_mysql_stmt_insert_id
#define  id_mysql_stmt_field_count     p_mysql_stmt_field_count

#define  id_mysql_close                p_mysql_close


#else  // Win above, Mac down below


#define  id_mysql_init          mysql_init

#define  id_mysql_options       mysql_options
#define  id_mysql_real_connect  mysql_real_connect
#define  id_mysql_select_db     mysql_select_db

#define  id_mysql_server_init   mysql_server_init
#define  id_mysql_server_end    mysql_server_end

#define  id_mysql_real_query          mysql_real_query
#define  id_mysql_character_set_name  mysql_character_set_name
#define  id_mysql_set_character_set   mysql_set_character_set


#define  id_mysql_num_rows      mysql_num_rows
#define  id_mysql_num_fields    mysql_num_fields

#define  id_mysql_free_result   mysql_free_result
#define  id_mysql_data_seek     mysql_data_seek

#define  id_mysql_row_seek      mysql_row_seek
#define  id_mysql_field_seek    mysql_field_seek

#define  id_mysql_errno         mysql_errno
#define  id_mysql_error         mysql_error

// ----- STMT

#define  id_mysql_stmt_init     mysql_stmt_init
#define  id_mysql_stmt_prepare  mysql_stmt_prepare
#define  id_mysql_stmt_execute  mysql_stmt_execute
#define  id_mysql_stmt_fetch    mysql_stmt_fetch

#define  id_mysql_stmt_fetch_column    mysql_stmt_fetch_column
#define  id_mysql_stmt_store_result    mysql_stmt_store_result
#define  id_mysql_stmt_param_count     mysql_stmt_param_count

#define  id_mysql_stmt_attr_set        mysql_stmt_attr_set
#define  id_mysql_stmt_attr_get        mysql_stmt_attr_get

#define  id_mysql_stmt_bind_param      mysql_stmt_bind_param
#define  id_mysql_stmt_bind_result     mysql_stmt_bind_result
#define  id_mysql_stmt_close           mysql_stmt_close
#define  id_mysql_stmt_reset           mysql_stmt_reset
#define  id_mysql_stmt_free_result     mysql_stmt_free_result
#define  id_mysql_stmt_send_long_data  mysql_stmt_send_long_data

#define  id_mysql_stmt_result_metadata mysql_stmt_result_metadata

#define  id_mysql_stmt_param_metadata  mysql_stmt_param_metadata
#define  id_mysql_stmt_errno           mysql_stmt_errno
#define  id_mysql_stmt_error           mysql_stmt_error
#define  id_mysql_stmt_sqlstate        mysql_stmt_sqlstate

#define  id_mysql_stmt_row_seek        mysql_stmt_row_seek
#define  id_mysql_stmt_row_tell        mysql_stmt_row_tell

#define  id_mysql_stmt_data_seek       mysql_stmt_data_seek

#define  id_mysql_stmt_num_rows        mysql_stmt_num_rows
#define  id_mysql_stmt_affected_rows   mysql_stmt_affected_rows
#define  id_mysql_stmt_insert_id       mysql_stmt_insert_id
#define  id_mysql_stmt_field_count     mysql_stmt_field_count

#define  id_mysql_close                mysql_close

#endif

