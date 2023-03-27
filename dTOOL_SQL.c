/*
 *  dTOOL_SQL.c
 *  Bouquet, ... iPPromet ... taken from GlideShow
 *
 *  Created by Igor Delovski on 02.07.2011. 
 *  Moved to C/Mac by Igor Delovski on 26.07.2020. 
 *  Copyright 2011, 2016, 2020 Igor Delovski All rights reserved.
 *
 */

// #### ct_SqlStringReportingOn() & ct_SqlStringReportingOff() ####

#define _DTOOL_SRC_SQL_

#define _dTOOL_sql_need_

#include "Home.h"

// #include "mysql.h"
// #import  "dTOOL_sql.h"  - already

#import  "dTOOL_loc.h"

#include "dTOOL_mysql.h"

// #import  "FileHelper.h"
// #import  "DateHelper.h"

static  SQLHandler  *sqlHandler = NULL;

static SQLFldDescription  internalSQLFields[] = {
    { "id", kTlong, MYSQL_TYPE_LONG, kFldDescFlagSearchableKey | kFldDescFlagNotNullKey, 0 },

    { NULL, 0, }
};

#pragma mark -

/*
#define  _LOG_SELECTION_STRING_

#define  _SHOW_FIELD_DESCRIPTIONS_no
*/
#define  _SHOW_CREATION_STRING_
/*
#define  _SHOW_COLL_APPENDING_STRING_no
#define  _SHOW_INSERTION_STRING_no
#define  _SHOW_UPDATING_STRING_no
#define  _SHOW_DELETION_STRING_no
#define  _SHOW_SELECTTION_STRING_no

#define  _SHOW_INSERTION_PROGRESS_no
#define  _SHOW_UPDATING_PROGRESS_no
#define  _SHOW_DELETION_PROGRESS_no
#define  _SHOW_SELECTTION_PROGRESS_no
 */

// .........................................................................................................

static int  id_InitSQLStorage (void)
{
   if (!sqlHandler)  {
      sqlHandler = (SQLHandler *)id_malloc_clear_or_exit (sizeof(SQLHandler));
      id_initPointerArray (&sqlHandler->dbSqlInfos);
      
      // sqlHandler->usedDbServerId = -1;
      
      // sqlHandler->lastDatNo = kMYSQLFileDatNoStart - 1;
      // sqlHandler->lastFlatDatNo = kMYSQLFlatFileDatNo - 1;  // So it starts at kMYSQLFlatFileDatNo
      
      if (id_mysql_server_init(0, NULL, NULL))  {          // --> mysql_library_init(), mysql_server_init()
         con_printf ("mysql_library_init() failed.\n");
         return (-1);
      }
   }

   return (0);
}

void  id_SQLSetRecSignatureHandler (SQLRecSignatureProcPtr sigProcPtr)  // explains the record into a string
{
   if (!id_InitSQLStorage())
      sqlHandler->recSigProcPtr = sigProcPtr;
}

// .........................................................................................................

static int  id_SQLConnectToServer (SQLDatabaseServer *dbs/*char *userName, char *userPassword, char *serverAddress*/)
{
   const char  *chPtr = NULL;
   
   if (!sqlHandler || !sqlHandler->dbs->dbHandle)
      return (-1);
   
   if (!dbs)
      dbs = sqlHandler->dbs;
     
   if (id_mysql_real_connect(dbs->dbHandle, dbs->srvAddress, dbs->usrName, dbs->usrPassword,
                             NULL /*"bmsimple"*//*opt_db_name*/, dbs->srvPort /*opt_port_num*/, 
                             NULL /*opt_socket_name*/, 0 /*opt_flags*/) == NULL)  {
      con_printf ("mysql_real_connect() failed.\n");

      if (chPtr = id_mysql_error (dbs->dbHandle))
         id_note_emsg ("%s", chPtr);
      
      id_mysql_close (dbs->dbHandle);
      id_mysql_server_end ();
      
      dbs->dbHandle = NULL;
      
      return (-1);
   }
   
   if (id_mysql_set_character_set(dbs->dbHandle, "cp1250"))  {
      con_printf ("mysql_set_character_set() failed.\n");
      id_mysql_close (dbs->dbHandle);
      id_mysql_server_end ();
      
      dbs->dbHandle = NULL;
      
      return (-1);
   }
   else
      con_printf ("New client character set: %s\n", id_mysql_character_set_name(dbs->dbHandle));
   
   return (0);
}

// One day I need different handle for external db if not on the same server, heh!
// So ctSql should have a pointer to its handle

int  id_SQLInitWithUserAndServer (char *userName, char *userPassword, char *serverAddress, long serverPort)
{
   short         i;
   unsigned int  mysql_ct = 12;  // timeout

   if (id_InitSQLStorage ())
      return (-1);
   
   for (i=0; i<2; i++)  {
      if (sqlHandler->dbServers[i].dbHandle)  {
         if (!strcmp(sqlHandler->dbServers[i].usrName, userName) && 
             !strcmp(sqlHandler->dbServers[i].usrPassword, userPassword) &&
             !strcmp(sqlHandler->dbServers[i].srvAddress, serverAddress))  {
            sqlHandler->dbs = &sqlHandler->dbServers[i];
            return (0);
         }
      }
   }
   
   if (!sqlHandler->dbServers[0].dbHandle)
      sqlHandler->dbs = &sqlHandler->dbServers[0];
   else  if (!sqlHandler->dbServers[1].dbHandle)
      sqlHandler->dbs = &sqlHandler->dbServers[1];
   else
      return (-1);
   
   sqlHandler->dbs->dbHandle = id_mysql_init (NULL);  /* initialize connection handler */ 
   
   if (!sqlHandler->dbs->dbHandle)   {
      con_printf ("mysql_init() failed (probably out of memory).\n");
      id_mysql_server_end ();  // mysql_library_init()
      return (-1);
      /* connect to server */
   }
   
   if (id_mysql_options(sqlHandler->dbs->dbHandle, MYSQL_OPT_CONNECT_TIMEOUT, &mysql_ct))  {
      con_printf ("mysql_options() failed.\n");
      id_mysql_close (sqlHandler->dbs->dbHandle);
      id_mysql_server_end ();
      
      sqlHandler->dbs->dbHandle = NULL;
      return (-1);
   }

   strNCpy (sqlHandler->dbs->mainDbName, "", 127);
   strNCpy (sqlHandler->dbs->usrName, userName, 31);            // 
   strNCpy (sqlHandler->dbs->usrPassword, userPassword, 31);
   strNCpy (sqlHandler->dbs->srvAddress, serverAddress, 31);
   
   sqlHandler->dbs->srvPort = serverPort;

#ifdef _NIJE_
   if (id_mysql_real_connect(sqlHandler->dbs->dbHandle, sqlHandler->dbs->srvAddress, sqlHandler->dbs->usrName, sqlHandler->dbs->usrPassword,
                             NULL /*"bmsimple"*//*opt_db_name*/, 0 /*opt_port_num*/, 
                             NULL /*opt_socket_name*/, 0 /*opt_flags*/) == NULL)  {
      con_printf ("mysql_real_connect() failed.\n");
      id_mysql_close (sqlHandler->dbs->dbHandle);
      id_mysql_server_end ();
      
      sqlHandler->dbs->dbHandle = NULL;
      
      return (-1);
   }
   
   if (id_mysql_set_character_set(sqlHandler->dbs->dbHandle, "cp1250"))  {
      con_printf ("mysql_set_character_set() failed.\n");
      id_mysql_close (sqlHandler->dbs->dbHandle);
      id_mysql_server_end ();
      
      sqlHandler->dbs->dbHandle = NULL;
      
      return (-1);
   }
   else
      con_printf ("New client character set: %s\n", id_mysql_character_set_name(sqlHandler->dbs->dbHandle));
#endif
   // Without db -> mysql_select_db()
   
   if (id_SQLConnectToServer (NULL/*sqlHandler->dbs->usrName, sqlHandler->dbs->usrPassword, sqlHandler->dbs->srvAddress*/))
      return (-1);
   
   return (0);
}

// .........................................................................................................

// Check if server with serverAddress is used and eventually active one (default)

int  id_SQLCheckServer (char *serverAddress, short *srvActiveFlag)
{
   if (!sqlHandler)
      return (-1);
   
   if (sqlHandler->dbServers[1].dbHandle)  {
      if (!stricmp(serverAddress, sqlHandler->dbServers[1].srvAddress))  {
         if (srvActiveFlag && (sqlHandler->dbs == &sqlHandler->dbServers[1]))
            *srvActiveFlag = TRUE;
         return (0);
      }
   }
   if (sqlHandler->dbServers[0].dbHandle)  {
      if (!stricmp(serverAddress, sqlHandler->dbServers[0].srvAddress))  {
         if (srvActiveFlag && (sqlHandler->dbs == &sqlHandler->dbServers[0]))
            *srvActiveFlag = TRUE;
         return (0);
      }
   }
   
   return (-1);
}

// .........................................................................................................

// Need id_SQLChangeServerAndDatabase() that switches sqlHandler->dbs to other index. Then I need CloseDatabase that closes only one

int  id_SQLChangeDatabase (char *dbName, short dbSelector, short createFlag)
{
   char    errStr[128], dbTempName[128];
   char   *curDbName = NULL;
   short  *dbVRefNum = NULL;
   
   SQLTarget  sqlTarget;
   
   if (sqlHandler && sqlHandler->dbs->dbHandle)  {
      
      if (!dbName)
         dbName = strNCpy (dbTempName, id_SQLDatabaseName (NULL, dbSelector), 127);
      
      if (id_SQLCheckDatabaseExist(/*sqlHandler->dbs->dbHandle,*/ dbName))  {
         if (!createFlag)
            return (-1);
         if (id_SQLCreateNewDatabase(sqlHandler->dbs->dbHandle, dbName, dbSelector, errStr, 127))  {
            con_printf ("SQLChangeDatabase:id_SQLCreateNewDatabase: %s\n", errStr);
            return (-1);
         }
      }

      switch (dbSelector)  {
         case kMainDatabase:
            sqlHandler->dbs->activeCounter = &sqlHandler->dbs->mainTableCounter;
            curDbName = sqlHandler->dbs->mainDbName;
            dbVRefNum = &sqlHandler->dbs->mainDBVrefNum;
            break;
         case kCommonDatabase:
            sqlHandler->dbs->activeCounter = &sqlHandler->dbs->commonTableCounter;
            curDbName = sqlHandler->dbs->commonDbName;
            dbVRefNum = &sqlHandler->dbs->commonDBVrefNum;
            break;
         case kAuxilaryDatabase:
            sqlHandler->dbs->activeCounter = &sqlHandler->dbs->auxTableCounter;
            curDbName = sqlHandler->dbs->auxDbName;
            dbVRefNum = &sqlHandler->dbs->auxDBVrefNum;
            break;
         default:
            return (-1);
      }
      
      strNCpy (curDbName, "", 127);
      
      id_SetupSQLTarget (&sqlTarget, sqlHandler->dbs->srvAddress, sqlHandler->dbs->srvPort, sqlHandler->dbs->usrName, sqlHandler->dbs->usrPassword, curDbName);
      
      if (!id_mysql_select_db(sqlHandler->dbs->dbHandle, dbName))  {
         strNCpy (curDbName, dbName, 127);
         id_SQLTarget2VRefNum (&sqlTarget, dbVRefNum);
         
         sqlHandler->dbs->activeDatabase = dbSelector;

         return (0);
      }
      else
         con_printf ("mysql_select_db() failed.\n");
   }
   
   return (-1);  // Well, after this, create it or close & quit
}

// .........................................................................................................

long  id_SQLGetCallsCounter (long *successfulCalls)
{
   if (successfulCalls)
      *successfulCalls = sqlHandler ? sqlHandler->dbSuccessCallsCounter : 0L;
   
   return (sqlHandler ? sqlHandler->dbAllCallsCounter : 0L);
}

// .........................................................................................................

char  *id_SQLGetLastSqlString (void)
{
   return (sqlHandler->lastSqlString);
}

// .........................................................................................................

static void  id_SQLSetLastSqlString (char *strPtr)
{
   strNCpy (sqlHandler->lastSqlString, strPtr, kMYSQLStringBufferLen-1);

   id_SetBlockToZeros (sqlHandler->usedWhereValues, kMaxSavedWhereValues*kMaxSavedValueLen);  // reset the thing
}

// .........................................................................................................

int  id_SQLGetLastWhereParamCount (void)
{
   short  i;
   
   for (i=0; i<kMaxSavedWhereValues && sqlHandler->usedWhereValues[i][0]; i++);

   return (i);
}

// .........................................................................................................

char  *id_SQLGetLastWhereParamValue (short idx)
{
   if (idx >= 0 && idx < kMaxSavedWhereValues)
      return (sqlHandler->usedWhereValues[idx]);

   return ("");
}

// .........................................................................................................

char  *id_SQLDatabaseName (SQLDatabaseServer *dbs, short dbSelector)  // kMainDatabase,kCommonDatabase,kAuxilaryDatabase
{
   if (sqlHandler && sqlHandler->dbs->dbHandle)  {
      if (!dbs)
         dbs = sqlHandler->dbs;
      
      switch (dbSelector)  {
         case kMainDatabase:
            return (dbs->mainDbName);
         case kCommonDatabase:
            return (dbs->commonDbName);
         case kAuxilaryDatabase:
            return (dbs->auxDbName);
         default:
            return (NULL);
      }
   }

   return (NULL);
}

// .........................................................................................................

char  *id_SQLActiveDatabaseName (void)
{
   if (sqlHandler && sqlHandler->dbs->dbHandle)
      return (id_SQLDatabaseName(sqlHandler->dbs, sqlHandler->dbs->activeDatabase));
   
   return (NULL);
}

// .........................................................................................................

Boolean  id_SQLCommonDatabaseActive (void)
{
   if (sqlHandler && sqlHandler->dbs->dbHandle)  {
      if (sqlHandler->dbs->activeDatabase == kCommonDatabase)
         return (TRUE);
   }
   
   return (FALSE);
}

// .........................................................................................................

Boolean  id_SQLAuxilaryDatabaseActive (void)
{
   if (sqlHandler && sqlHandler->dbs->dbHandle)  {
      if (sqlHandler->dbs->activeDatabase == kAuxilaryDatabase)
         return (TRUE);
   }
   
   return (FALSE);
}

// .........................................................................................................

// CREATE USER 'user'@'%';
// GRANT ALL PRIVILEGES ON *.* To 'user'@'%' IDENTIFIED BY 'pass';
// UPDATE mysql.user SET Host='%' WHERE Host='localhost' AND User='idelovski';

int  id_SQLCreateNewDatabase (  // Needs param: kMainDatabase, kCommonDatabase or kAuxilaryDatabase
 MYSQL  *dbHandle,
 char   *dbName,
 short   dbSelector,
 char   *retErrStr,  // may be NULL
 short   errMaxLen
)
{
   int     returnCode = -1;
   char    sqlStr[1024];
   char   *errMsg = NULL;
   
   SQLTarget  sqlTarget;

   if ((dbSelector < kMainDatabase) || (dbSelector > kAuxilaryDatabase))
      return (-1);
   
   if (sqlHandler && sqlHandler->dbs->dbHandle)  {
      snprintf (sqlStr, 1023+1, "CREATE DATABASE IF NOT EXISTS %s", dbName);

      returnCode = id_SQLPerformQuery (dbHandle ? dbHandle : sqlHandler->dbs->dbHandle, sqlStr, retErrStr, errMaxLen);
      
      if (!returnCode)  {
         id_SetupSQLTarget (&sqlTarget, sqlHandler->dbs->srvAddress, sqlHandler->dbs->srvPort, sqlHandler->dbs->usrName, sqlHandler->dbs->usrPassword, dbName);

         switch (sqlHandler->dbs->activeDatabase = dbSelector)  {
            case kMainDatabase:
               strNCpy (sqlHandler->dbs->mainDbName, dbName, 127);
               id_SQLTarget2VRefNum (&sqlTarget, &sqlHandler->dbs->mainDBVrefNum);
               break;
            case kCommonDatabase:
               strNCpy (sqlHandler->dbs->commonDbName, dbName, 127);
               id_SQLTarget2VRefNum (&sqlTarget, &sqlHandler->dbs->commonDBVrefNum);
               break;
            case kAuxilaryDatabase:
               strNCpy (sqlHandler->dbs->auxDbName, dbName, 127);
               id_SQLTarget2VRefNum (&sqlTarget, &sqlHandler->dbs->auxDBVrefNum);
               break;
         }
      }
   }
   
   return (returnCode);
}

// .........................................................................................................

// This needs a param so I can close external connections without killing the main connection
// See -> kMainDatabase, kExtraDatabase, kAuxilaryDatabase

// Assuming id_SQLctCLISAM() was already called

// See id_SQLCheckServer() where I check sqlHandler->dbServers[0] & sqlHandler->dbServers[1] so I need something to close one or another

int  id_SQLCloseDatabase (void)  // Called from id_ToolboxCleanUp()
{
   short  i;
   
   /*if (sqlHandler && sqlHandler->dbs->dbHandle)  {
      // disconnect from server, terminate client library
      id_mysql_close (sqlHandler->dbs->dbHandle);
      sqlHandler->dbs->dbHandle = NULL;
      id_mysql_server_end ();                 // --> mysql_server_end()
   }*/
   
   if (sqlHandler)  {
      for (i=0; i<2; i++)  {
         if (sqlHandler->dbServers[i].dbHandle)  {
            // disconnect from server, terminate client library
            id_mysql_close (sqlHandler->dbServers[i].dbHandle);
            sqlHandler->dbServers[i].dbHandle = NULL;
         }
      }
      id_mysql_server_end ();                 // --> mysql_server_end()
   }
   
   if (sqlHandler)  {
      id_DisposePtr ((Ptr)sqlHandler);
      sqlHandler = NULL;
      
      return (0);
   }
   
   return (-1);
}

// .........................................................................................................

#pragma mark -

// .........................................................................................................

static int  id_SQLCType2SQLType (short fldCType, short len)
{
   switch (fldCType)  {
      case  kTint:
      case  kTuint:
         return (MYSQL_TYPE_LONG);
      case  kTshort:
      case  kTushort:
         return (MYSQL_TYPE_SHORT);
      case  kTlong:
      case  kTulong:
         return (MYSQL_TYPE_LONG);
      case  kTchar:
      case  kTuchar:
         if (len > 1)
            return (MYSQL_TYPE_STRING);
         return (MYSQL_TYPE_TINY);
      case  kTfloat:
         return (MYSQL_TYPE_FLOAT);
      case  kTdouble:
         return (MYSQL_TYPE_DOUBLE);
   }
   
   return (MYSQL_TYPE_BLOB);
}

// .........................................................................................................

static void  id_SQLCalcFildsSize (short fldCType, short fCount, short fLength, short *fldBytes, short *fldIndexCount)
{
   short  i, idxCount = fCount ? fCount : 1, fldSize = fCount;
   
   if ((fldCType == kTchar) || (fldCType == kTuchar) || (fldCType == kTbytes))  {
      if (fLength)
         fldSize = fLength;
      else
         idxCount = 1;
   }
   else  if ((fldCType == kTlong) || (fldCType == kTulong))
      fldSize = sizeof (long);
   else  if ((fldCType == kTshort) || (fldCType == kTushort))
      fldSize = sizeof (short);
   else  if (fldCType == kTfloat)
      fldSize = sizeof (float);
   else  if (fldCType == kTdouble)
      fldSize = sizeof (double);
   
   *fldBytes      = fldSize;
   *fldIndexCount = idxCount;
}

// .........................................................................................................

static void  id_SQLSanitizeTableName (char *tableName)
{
   id_ConvertTextToYuAscii (tableName);
   
   while (*tableName)  {
      if (!isalnum(*tableName))
         *tableName = '_';
      tableName++;
   }
}

// .........................................................................................................

static int  id_SQLCountUsedFields (ID_CT_MYSQL *ctSql, RecFieldInfo *theRfi, short *retUnsignedCnt)
{
   short            i, six, allFields = 0, fldCnt, skipCnt = 0, varCnt = 0, unsigCnt = 0;
   short            idxCount = 0, fldSize = 0;
   RecFieldOffset  *rfoArray = theRfi->iRecFldOffsets;
   
   if (retUnsignedCnt)
      *retUnsignedCnt = unsigCnt;
   
   for (fldCnt=i=0; rfoArray[i].fCType && rfoArray[i].fCName; i++)  {
      
      if (rfoArray[i].fCFlags & kTFlagSqlSkip)  {
         skipCnt++;
         allFields++;
         continue;                 // Fillers
      }
      
      if (rfoArray[i].fCFlags & kTFlagSqlVarLength)  {
         varCnt++;
         allFields++;
         ctSql->varLenFlag = TRUE;
         break;                 // Var field, must be last
      }

      if (rfoArray[i].fCType >= kTstruct)  {  // Custom types
         RecFieldInfo  *rfi = id_FindRecFieldInfo (NULL, NULL, NULL, 0, rfoArray[i].fCType);
         
         if (rfi)  {
            long  fldOffset;
            
            for (six=0; rfi->iRecFldOffsets[six].fCType && rfi->iRecFldOffsets[six].fCName; six++)  {
               fldOffset = rfi->iRecFldOffsets[six].fCOffset + rfoArray[i].fCOffset;
               
               if (rfi->iRecFldOffsets[six].fCFlags & kTFlagSqlSkip)  {
                  skipCnt++;
                  allFields++;
                  continue;                 // Fillers
               }
               
               id_SQLCalcFildsSize (rfi->iRecFldOffsets[six].fCType, rfi->iRecFldOffsets[six].fCCount, rfi->iRecFldOffsets[six].fCLength, &fldSize, &idxCount);
               fldCnt    += idxCount;
               allFields += idxCount;
               if (isUnsignedCType(rfi->iRecFldOffsets[six].fCType))
                  unsigCnt += idxCount;
            }
         }
      }
      else  {
         id_SQLCalcFildsSize (rfoArray[i].fCType, rfoArray[i].fCCount, rfoArray[i].fCLength, &fldSize, &idxCount);
         // id_SQLCalcFildsSize (fldCType, fCount, fLength, &fldSize, &idxCount);
         fldCnt    += idxCount;
         allFields += idxCount;
         if (isUnsignedCType(theRfi->iRecFldOffsets[i].fCType))
            unsigCnt += idxCount;
      }
   }
   
   if (allFields != (fldCnt + skipCnt + varCnt))
      con_printf ("SQLCountUsedFields is confused!\n");
   
   if (retUnsignedCnt)
      *retUnsignedCnt = unsigCnt;

   // ctSql->recSqlFldCount = fldCnt;
   // ctSql->skippedFldCount = skipCnt;
   
   return (fldCnt + varCnt);
}

// .........................................................................................................

// if index is -1 then it's simple field, no index, otherwise it's an array element with three undercores

static void  id_SQLMakeOneFldDescription (SQLFldDescription *fldDesc, short fldCType, short index, long fldOffset, short len, char *fldName, char *fldSubName)
{
   short  buffLen = strlen (fldName) + (fldSubName ? strlen(fldSubName)+2 : 0);
   
   if (index >= 0)
      buffLen += 6;  // Max Index: 999 (three characters)
   
   id_SetBlockToZeros (fldDesc, sizeof(SQLFldDescription));
   
   fldDesc->fldName = id_malloc_or_exit (buffLen + 1L);    // remember to keep this until exit or call id_DisposePtr()
   if (fldSubName)
      snprintf (fldDesc->fldName, buffLen+1, "%s__%s", fldName, fldSubName);
   else
      strNCpy (fldDesc->fldName, fldName, buffLen);
   
   if (index >= 0)
      strNCatf (fldDesc->fldName, buffLen, "___%hd", index);

   fldDesc->ctType      = fldCType;                             // AH!, see api.h (kTint, kTuint, kTshort, kTchar, ....)
   fldDesc->sqlType     = id_SQLCType2SQLType (fldCType, len);  // MYSQL_TYPE_INT, MYSQL_TYPE_STRING,... (SQLITE_INTEGER, SQLITE_TEXT), etc.
   fldDesc->inRecOffset = fldOffset;                            // offset in a record
   if (fldDesc->sqlType != MYSQL_TYPE_BLOB)
      fldDesc->fldFlags = kFldDescFlagSearchableKey;            // uniqueFlag, whereSearchFlag, etc.
   fldDesc->maxLen      = len;                                  // optional, do what you want

#ifdef  _SHOW_FIELD_DESCRIPTIONS_
   con_printf ("F: [%s], S: [%s], IDX: [%hd] -> %s\n", fldName, fldSubName ? fldSubName : "n/a", index, fldDesc->fldName);
#endif
}

// .........................................................................................................

// This handles ONE simple field or array -> Descriptions plural

static int  id_SQLMakeFldDescriptions (SQLFldDescription *fldDesc, short fldCType, long fldOffset, short fCount, short fLength, char *fldName, char *fldSubName)
{
   short  i, idxCount = 0, fldSize = 0;
   
   id_SQLCalcFildsSize (fldCType, fCount, fLength, &fldSize, &idxCount);
   
   for (i=0; i<idxCount; i++)  {
      // for kFldDescFlagVariableLength fldSize should be zero, check it!
      id_SQLMakeOneFldDescription (fldDesc++, fldCType, idxCount > 1 ? i : -1, fldOffset, fldSize, fldName, fldSubName);
      fldOffset += fldSize;  // For next iteration, if there is one
   }
   
   return (idxCount);  // How many we created
}

// .........................................................................................................

// root level, we call id_SQLMakeFldDescriptions() and id_SQLMakeOneFldDescription() from there

static SQLFldDescription  *id_SQLBuildNewFldDescriptions (ID_CT_MYSQL *ctSql, RecFieldInfo *theRfi)
{
   short               i, newFields, six, fldCnt, skipCnt = 0;
   RecFieldOffset     *rfoArray     = theRfi->iRecFldOffsets;
   SQLFldDescription  *descriptions = (SQLFldDescription *)id_malloc_clear_or_exit(sizeof(SQLFldDescription) * (ctSql->recSqlFldCount + 1));  // One more for NULL
   SQLFldDescription  *descPtr      = descriptions;
   
   for (fldCnt=i=0; rfoArray[i].fCType && rfoArray[i].fCName; i++)  {
      
      if (rfoArray[i].fCFlags & kTFlagSqlSkip)  {
         skipCnt++;
         continue;                 // Fillers
      }
      
      if (fldCnt >= ctSql->recSqlFldCount)  {
         con_printf ("Too many fields: PANICK ATTACK!\nAbort mission! Abort!\n");
         break;
      }
      
      if (rfoArray[i].fCType >= kTstruct)  {  // Custom types
         RecFieldInfo  *rfi = id_FindRecFieldInfo (NULL, NULL, NULL, 0, rfoArray[i].fCType);
         
         if (rfi)  {
            long  fldOffset;
            
            for (six=0; rfi->iRecFldOffsets[six].fCType && rfi->iRecFldOffsets[six].fCName; six++)  {
               fldOffset = rfi->iRecFldOffsets[six].fCOffset + rfoArray[i].fCOffset;
               
               if (rfi->iRecFldOffsets[six].fCFlags & kTFlagSqlSkip)  {
                  skipCnt++;
                  continue;                 // Fillers
               }
               
               newFields = id_SQLMakeFldDescriptions (descPtr, rfi->iRecFldOffsets[six].fCType, fldOffset, rfi->iRecFldOffsets[six].fCCount, rfi->iRecFldOffsets[six].fCLength, rfoArray[i].fCName, rfi->iRecFldOffsets[six].fCName);

               fldCnt += newFields;
               descPtr += newFields;
            }
         }
         else
            con_printf ("Unknown Type with id: %hd\n", rfoArray[i].fCType);
      }
      else  {
         char  *namePtr = rfoArray[i].fCName;
         
         if (!ctSql->ifilPtr)
            namePtr = !i ? kMYSQLFlatContentName : kMYSQLFlatKeyName; 

         newFields = id_SQLMakeFldDescriptions (descPtr, rfoArray[i].fCType, rfoArray[i].fCOffset, rfoArray[i].fCCount, rfoArray[i].fCLength, namePtr, NULL);
         
         if (rfoArray[i].fCFlags & kTFlagSqlVarLength)  {
            ctSql->varLenFlag = TRUE;
            descPtr->fldFlags |= kFldDescFlagVariableLength;
            // descPtr->maxLen should be 0!
         }
         
         fldCnt += newFields;
         descPtr += newFields;
      }
   }
   
   ctSql->skippedFldCount = skipCnt;
   
   if (fldCnt != ctSql->recSqlFldCount)
      con_printf ("SQLBuildNewFldDescriptions is confused!\n");

   return (descriptions);  // Full array
}

// .........................................................................................................

SQLFldDescription  *id_SQLFindFldDescription (ID_CT_MYSQL *ctSql, char *fldName)
{
   short               i, six, fldCnt=0;
   // RecFieldOffset     *rfoArray = theRfi->iRecFldOffsets;
   SQLFldDescription  *fldDescCArray = ctSql->fldDescs;
   
   if (!strcmp(fldName, "id"))
      return (&internalSQLFields[0]);
   
   for (i=0; fldDescCArray[i].fldName; i++)  {
      if (!strcmp(fldName, fldDescCArray[i].fldName))
         return (&fldDescCArray[i]);
   }

   return (NULL);  // Nope
}

// .........................................................................................................

static int  id_SQLConvertRecordEncoding (ID_CT_MYSQL *ctSql, char *recPtr, Boolean reverseFlag)
{
   short               i, pix;
   short               fldSize = 0;
   char               *fldPtr = NULL;
   // RecFieldOffset     *rfoArray     = ctSql->rfi->iRecFldOffsets;
   SQLFldDescription  *fldDescCArray = ctSql->fldDescs;
   
   for (pix=i=0; fldDescCArray[i].fldName; i++)  {
#ifdef  _DO_DISPLAY_RECORD_
      con_printf ("%s...", fldDescCArray[i].fldName);
#endif // _DO_DISPLAY_RECORD_
      
      if (fldDescCArray[i].ctType == kTchar)  {
         fldPtr = recPtr + fldDescCArray[i].inRecOffset;
         fldSize = fldDescCArray[i].maxLen;
         if (fldDescCArray[i].sqlType == MYSQL_TYPE_STRING && fldSize > 1)  {
            if (reverseFlag)
               id_Convert1250ToText (fldPtr, &fldSize, FALSE);
            else
               id_ConvertTextTo1250 (fldPtr, &fldSize, FALSE);
            pix++;
         }
      }
   }
   
   return (pix);
}

// .........................................................................................................

static void  id_SQLInitializeBindings (ID_BIND_INFO *arrBindInfos, MYSQL_BIND *arrBinds, short bndCount)
{
   short  i;
   
   for (i=0; i<bndCount; i++)
      arrBindInfos[i].biBind = &arrBinds[i];
}

// .........................................................................................................

#pragma mark -

ID_CT_MYSQL  *id_SQLFindFileNo (short fileNo)
{
#ifdef _NIJE_
   short   datNo = id_SQLctGETFIL (fileNo);  // GETFIL (fileNo, REVMAP);  // Ha Ha - napravi svoju fukciju!
   CTTPtr  theCTT = NULL;

   theCTT = id_CTTFindByDatNo (datNo >= 0 ? datNo : fileNo);

   return (theCTT ? theCTT->ctSqlPtr : NULL);
#endif
   short         i;
   ID_CT_MYSQL  *ctSql = NULL;
   
   if (sqlHandler)  {
      for (i=0; i < id_pointerElemsCount(&sqlHandler->dbSqlInfos); i++)  {
         
         ctSql = id_pointerElem (&sqlHandler->dbSqlInfos, i);
         
         if ((fileNo >= ctSql->datNo) && (fileNo <= (ctSql->datNo + ctSql->dNumIdx)))  {
            /*if (fileNo > ctSql->datNo)  {
             if (ctSql->datNo != GETFIL (fileNo, REVMAP))
             con_printf ("GETFIL discrepancy: %hd", fileNo);
             }*/
            return (ctSql);
         }
      }
   }
   
   return (NULL);
}

// Needed when we change sets in old fashion ctree way

ID_CT_MYSQL  *id_SQLFindBySet (
 short   setId,    // ctree set id
 short  *setIndex  // index in ctSql
)
{
   short         i, six;
   ID_CT_MYSQL  *ctSql = NULL;

   if (sqlHandler)  {
      for (i=0; i < id_pointerElemsCount(&sqlHandler->dbSqlInfos); i++)  {
         
         ctSql = id_pointerElem (&sqlHandler->dbSqlInfos, i);
         
         for (six=1; six<kStandardSelSets; six++)  {  // skip zero,
            
            if (ctSql->ctSetIds[six] == setId)  {
               if (setIndex)
                  *setIndex = six;
               return (ctSql);
            }
         }
      }
   }
   
   return (NULL);
}

/* ......................................................id_SQLFindFileIndex.......... */

// Usefull when I need to find it with FAKTUR, CONFIG,...

int  id_SQLFindDatNoByFileIndex (short fileIndex, short dbSelector)
{
   short         i;
   ID_CT_MYSQL  *ctSql = NULL;
   
   for (i=0; i < id_pointerElemsCount(&sqlHandler->dbSqlInfos); i++)  {
      
      ctSql = id_pointerElem (&sqlHandler->dbSqlInfos, i);
      
      if (ctSql->fileIndex == fileIndex)  {
         switch (dbSelector)  {
            case kMainDatabase:
               if (!stricmp(sqlHandler->dbs->mainDbName, ctSql->dbName))
                  return (ctSql->datNo);
               break;
            case kCommonDatabase:
               if (!stricmp(sqlHandler->dbs->commonDbName, ctSql->dbName))
                  return (ctSql->datNo);
               break;
            case kAuxilaryDatabase:
               if (!stricmp(sqlHandler->dbs->auxDbName, ctSql->dbName))
                  return (ctSql->datNo);
               break;
         }
      }
   }
   
   return (-1);
}


/* ......................................................id_SQLFindRfiByTableName..... */

// Usefull when I open a table in auxiliary database

RecFieldInfo  *id_SQLFindRfiByTableName (char *tableName)
{
   short         i;
   char          tmpStr[256];
   ID_CT_MYSQL  *ctSql = NULL;
   
   id_SQLSanitizeTableName (strNCpy(tmpStr, tableName, 255));
   
   for (i=0; i < id_pointerElemsCount(&sqlHandler->dbSqlInfos); i++)  {
      
      ctSql = id_pointerElem (&sqlHandler->dbSqlInfos, i);
      
      if (!strnicmp(ctSql->tableName, tmpStr, 63))
         return (ctSql->rfi);
   }
   
   return (NULL);
}

// Needed so datNos dont go sky high after many opened and closed files

int  id_SQLAvailableDatNo (Boolean flatFile)
{
   short         i, highDatno = (flatFile ? kMYSQLFlatFileDatNo : kMYSQLFileDatNoStart) - 1;
   ID_CT_MYSQL  *ctSql = NULL;
   
   for (i=0; i < id_pointerElemsCount(&sqlHandler->dbSqlInfos); i++)  {
      
      ctSql = id_pointerElem (&sqlHandler->dbSqlInfos, i);
      
      if ((flatFile && ctSql->datNo >= kMYSQLFlatFileDatNo) || (!flatFile && ctSql->datNo < kMYSQLFlatFileDatNo))  {
         if (ctSql->datNo > highDatno)
            highDatno = ctSql->datNo + ctSql->dNumIdx;
      }
   }
   
   return (highDatno + 1);
}

/* ................................................... id_SQLCountActiveTables ...... */

int  id_SQLCountActiveTables (MYSQL *dbHandle)
{
   short         i, retCount = 0;
   ID_CT_MYSQL  *ctSql = NULL;
   
   for (i=0; i < id_pointerElemsCount(&sqlHandler->dbSqlInfos); i++)  {
      
      ctSql = id_pointerElem (&sqlHandler->dbSqlInfos, i);
      
      if (ctSql->dbHandle == dbHandle)
         retCount++;
   }
   
   return (retCount);
}

/* ....................................................... id_SQLGetServerInfo ...... */

SQLDatabaseServer  *id_SQLGetServerInfo (ID_CT_MYSQL *ctSql, short *dbsIndex)
{
   short  i;
   
   if (dbsIndex)
      *dbsIndex = -1;
   
   for (i=0; i<2; i++)  {
      if (ctSql->dbHandle == sqlHandler->dbServers[i].dbHandle)  {
         if (dbsIndex)
            *dbsIndex = i;
         return (&sqlHandler->dbServers[i]);
      }
   }
   
   return (NULL);
}

/* ....................................................... id_SQLGetDatabaseName .... */

char  *id_SQLGetDatabaseName (short fileNo)
{
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   if (ctSql)
      return (ctSql->dbName);
   
   return (NULL);
}

/* ....................................................... id_SQLGetTableName ....... */

char  *id_SQLGetTableName (short fileNo)
{
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   if (ctSql)
      return (ctSql->tableName);
   
   return (NULL);
}

/* ......................................................... id_SQLGetKeySegments ... */

int  id_SQLGetKeySegments (ID_CT_MYSQL *ctSql, short keyNo)
{
   if (ctSql->ifilPtr)
      return (ct_GetKeySegments(ctSql->ifilPtr, keyNo));
   
   if (ctSql->datNo >= kMYSQLFlatFileDatNo)
      return (1);
   
   return (0);
}

/* ......................................................... ct_GetMaxKeySegments ... */

int  id_SQLGetNumberOfKeys (ID_CT_MYSQL *ctSql)
{
   if (ctSql->ifilPtr)
      return (ct_GetNumberOfKeys(ctSql->ifilPtr));

   if (ctSql->datNo >= kMYSQLFlatFileDatNo)
      return (1);

   return (0);
}

/* ..................................................... id_SQLGetNumberOfIndexes ... */

int  id_SQLGetNumberOfIndexes (ID_CT_MYSQL *ctSql)
{
   short  i, maxKeySegs, keyNo;
   short  segLen, retCount = 1;
   char   segName[64];

   if (ctSql->ifilPtr)  for (keyNo=1; keyNo<=id_SQLGetNumberOfKeys(ctSql); keyNo++)  {
      
      if (!ct_DuplicateKey(ctSql->ifilPtr, keyNo))
         retCount++;
   }
   else
      retCount++;

   return (retCount);
}

/* ......................................................... id_SQLKeySegmentName ... */

int  id_SQLKeySegmentName (ID_CT_MYSQL *ctSql, short keyNo, short segIdx, char *segName, short *segLen)
{
   short            i, retVal = -1;
   RecFieldOffset  *rfoArray = ctSql->rfi->iRecFldOffsets;
   RecFieldInfo    *rfi = NULL;
   
   // con_printf ("Rec: %s with %hd indices\n", recName, ifilPtr->dnumidx);
   if (segLen)
      *segLen = 0;
   
   if (ctSql->ifilPtr)  {
      if (segIdx == ctSql->ifilPtr->ix[keyNo-1].inumseg)  {  // ikeydup -> id
         strNCpy (segName, "id", 2);
         *segLen = 4;
         // if (!ctSql->ifilPtr->ix[keyNo-1].ikeydup)
         return (0);
      }
         /// seg[segIdx].slength
      return (ct_SQLKeySegmentName(rfoArray, ctSql->ifilPtr, keyNo, segIdx, segName, segLen));  // Change this f() name
   }
   
   // Second item is the key

   for (i=0; rfoArray[i].fCType && rfoArray[i].fCName; i++)  {
      if (i == 1)  {
         snprintf (segName, 255, "%s", rfoArray->fCName);
         if (segLen)
            *segLen = rfoArray[i].fCCount;  // Must be flat, not array
         return (0);
      }
   }
   
   return (retVal);
}

// .................................................... id_SQLKeySegmentPaddingSize ........................

static int  id_SQLKeySegmentPaddingSize (ID_CT_MYSQL *ctSql, short keyNo, short segIdx, Boolean *rPadFlag)
{
   short  padSize = 0 /*, hasMixedTypes = FALSE*/;
   
   *rPadFlag = FALSE;
   
   // hasMixedTypes  = ct_MixedSegmentTypesKey (ctSql->ifilPtr, keyNo, 0);
   
   if (segIdx == ctSql->ifilPtr->ix[keyNo-1].inumseg)  {  // ikeydup -> id
      padSize = 11;
   }
   else  if (ctSql->ifilPtr)  {
      if (!ct_KeySegmentIsString(ctSql->ifilPtr, keyNo-1, segIdx))  {          // int types need to be lpadded in concat expressions
         short  segLen = ctSql->ifilPtr->ix[keyNo-1].seg[segIdx].slength;
         
         if (segLen == 4)
            padSize = 11;
         else  if (segLen == 2)
            padSize = 5;
         else
            padSize = 3;
      }
      else  if (segIdx < (ctSql->ifilPtr->ix[keyNo-1].inumseg-1) && !ct_KeySegmentIsString(ctSql->ifilPtr, keyNo-1, segIdx+1))  {
         // TEXT but next segment isn't, so we need rpadding
         *rPadFlag = TRUE;
         padSize = ctSql->ifilPtr->ix[keyNo-1].seg[segIdx].slength;
      }
   }
   
   return (padSize);
}

// .........................................................................................................

#pragma mark -

// .........................................................................................................

char  *id_SQLConvertStringForSQL(const char *origStr, char *tarStr, short tarLen)
{
   // TODO - everything
   // return ([origStr stringByReplacingOccurrencesOfString:@"'" withString:@"''"]);
   return (strNCpy(tarStr, origStr, tarLen));
}

// .........................................................................................................

char  *id_SQLConvertStringFromSQL(const char *sqlStr, char *tarStr, short tarLen)
{
   // TODO - everything
   // return ([sqlStr stringByReplacingOccurrencesOfString:@"''" withString:@"'"]);
   return (strNCpy(tarStr, sqlStr, tarLen));
}

// .........................................................................................................

#pragma mark -

// .........................................................................................................

// Use for simple things, like creating databases, tables, etc.

int  id_SQLPerformQuery (
 MYSQL       *dbHandle,
 char        *sqlString,
 char        *retErrStr,  // may be NULL
 short        errMaxLen
)
{
   int  returnCode = CR_UNKNOWN_ERROR, len;  // errmsg.h
   
   if (*sqlString)  {
      len = strlen (sqlString);

      if (sqlString[len-1] == ';')
         con_printf ("Unexpected ';' at the end of the statement.\n");
   
      returnCode = id_mysql_real_query (dbHandle, sqlString, len);
   
      if (returnCode)  {
         const char  *chPtr = id_mysql_error (dbHandle);
         if (chPtr)  {
            con_printf ("%s\n", chPtr);
            if (retErrStr)
               strNCpy (retErrStr, chPtr, errMaxLen);
         }
      }
      else
         sqlHandler->dbSuccessCallsCounter++;
      sqlHandler->dbAllCallsCounter++;
   }
   
   return (returnCode);
}

// .........................................................................................................

#pragma mark -
#pragma mark One Shot SQL

// .........................................................................................................

int  id_SQLGetTextExecutingSqlStatement (char *sqlString, char *retString, short maxLen)
{
   short  retVal;
   long   itemCount = 1;
   char  *quasiArray[1];
   
   quasiArray[0] = retString;
   
   retVal = id_SQLGetTextArrayExecutingSqlStatement (sqlString, quasiArray, maxLen, &itemCount, FALSE);
   
   if (!retVal && !itemCount)
      retVal = MYSQL_NO_DATA;

   return (retVal);
}

// Add sqlHandle as a param here

#ifdef _NIJE_
-int  id_SQLGetTextArrayExecutingSqlStatement (char *sqlString, char **retStringArray, short itemSize, long *itemCount)
{
   // NSString      *preparedName = [dbFileHelper sqlStringWithString:iName];
   long           returnCode = 0;
   long           column_count, idx = 0;
   unsigned long  data_size;  // Actual size
   char           locBuffer[256], *buffPtr = locBuffer;
   my_bool        is_null[1], is_err[1];
   MYSQL_BIND     result[1];
   MYSQL_RES     *prepare_meta_result;
      
   if (itemSize > 256)
      buffPtr = id_malloc_clear_or_exit (itemSize+1);
   
   returnCode = id_SQLInitPrepare (NULL, sqlString);
   
   memset (&result[0], 0, sizeof (MYSQL_BIND));
   
   // con_printf ("SQLGetTextExecutingSqlStatement - str: %s\n", sqlString);
   
   // Function bindString u CTr.c
   
   // Result
   result[0].buffer_type   = MYSQL_TYPE_STRING;
   result[0].buffer        = (void *)buffPtr;
   result[0].buffer_length = itemSize;
   
   result[0].is_unsigned = 0;
   result[0].is_null     = &is_null[0];
   result[0].length      = &data_size;
   result[0].error       = &is_err[0];
   
   // Bind result
   if (!returnCode)
      returnCode = id_mysql_stmt_bind_result (sqlHandler->sqlStatement, &result[0]);
   
   // Idiote - provjeri kolko ih ima, mora biti ONLY ONE!
   
   /* Fetch result set meta information */
   if (!returnCode)
      prepare_meta_result = id_mysql_stmt_result_metadata (sqlHandler->sqlStatement);
   
   if (!prepare_meta_result)  {
      con_printf ("mysql_stmt_result_metadata(), returned no meta information\n");
      con_printf ("%s\n", id_mysql_stmt_error(sqlHandler->sqlStatement));
      
      returnCode = MYSQL_NO_DATA;
   }
   
   if (!returnCode)  {
      
      column_count = id_mysql_num_fields (prepare_meta_result);
      
      if (column_count != 1)  {  /* validate column count */
         con_printf ("Invalid column count returned by MySQL\n");
         returnCode = MYSQL_NO_DATA;
      }
      /*
      numRows = (long)mysql_num_rows (prepare_meta_result);  // USELESS!
      
      if (numRows > *itemCount)
         returnCode = MYSQL_NO_DATA;*/
   }
   
   if (!returnCode)
      returnCode = id_mysql_stmt_execute (sqlHandler->sqlStatement);
   
   if (!returnCode)  {
      sqlHandler->dbCallsCounter++;
      returnCode = id_mysql_stmt_store_result (sqlHandler->sqlStatement);  // Move everything to the client
   }
   if (!returnCode)  {
      long  numRows = (long)id_mysql_stmt_num_rows (sqlHandler->sqlStatement);
      
      if (numRows > *itemCount)
         returnCode = MYSQL_NO_DATA;
   }
   
   *itemCount = 0;
   
   if (!returnCode)  {
      // The Loop
      while (!(returnCode=id_mysql_stmt_fetch(sqlHandler->sqlStatement)))  {
         buffPtr[data_size] = '\0';
         strNCpy (retStringArray[idx], buffPtr, itemSize);
         idx++;
      }
      
      if (idx)  // we did something
         returnCode = 0;
   }
   else
      con_printf ("Failed id_SQLGetTextExecutingSqlStatement: %ld\n", returnCode);
   
   id_SQLCloseStatement (NULL);
   
   if (prepare_meta_result)
      id_mysql_free_result (prepare_meta_result);
   
   *itemCount = idx;
   
   if (buffPtr != locBuffer)
      id_DisposePtr ((Ptr)buffPtr);
   
   return (returnCode);
}
#endif

// This will fetch items vertically or horizontally, from rows or columns
// asColumns - fetch several keys/columns in one row, othewise, fetch several rows for one column

int  id_SQLGetTextArrayExecutingSqlStatement (char *sqlString, char **retStringArray, short itemSize, long *itemCount, Boolean asColumns)
{
   // NSString      *preparedName = [dbFileHelper sqlStringWithString:iName];
   long  returnCode = 0;
   long  i, column_count, row_count, idx = 0;
   char  locBuffer[256], *buffPtr = locBuffer, *elemPtr = NULL;

   unsigned long  *data_size;  // Actual size
   my_bool         is_null[1], is_err[1];
   MYSQL_BIND      result[1], *results = NULL;
   MYSQL_RES      *prepare_meta_result;
   
   if (asColumns)  {
      row_count = *itemCount;
      elemPtr = buffPtr = id_malloc_clear_or_exit ((itemSize+1) * row_count);
      results = (MYSQL_BIND *) id_malloc_clear_or_exit (sizeof(MYSQL_BIND) * row_count);
   }
   else  {
      row_count = 1;
      results = result;
      if (itemSize > 256)
         buffPtr = id_malloc_clear_or_exit (itemSize+1);
      elemPtr = buffPtr;
   }
   
   data_size = (unsigned long *) id_malloc_clear_or_exit (sizeof(unsigned long) * row_count);
   
   returnCode = id_SQLInitPrepare (NULL, sqlString);
   
   memset (&result[0], 0, sizeof (MYSQL_BIND));
   
   // con_printf ("SQLGetTextExecutingSqlStatement - str: %s\n", sqlString);
   
   // Function bindString u CTr.c
   
   // Result
   for (i=0; i<row_count; i++)  {
      results[i].buffer_type   = MYSQL_TYPE_STRING;
      results[i].buffer        = (void *)elemPtr;
      results[i].buffer_length = itemSize;
      
      results[i].is_unsigned = 0;
      results[i].is_null     = &is_null[0];
      results[i].length      = &data_size[i];
      results[i].error       = &is_err[0];

      elemPtr += (itemSize+1);
   }
   // Bind result
   if (!returnCode)
      returnCode = id_mysql_stmt_bind_result (sqlHandler->sqlStatement, &results[0]);
      
   /* Fetch result set meta information */
   if (!returnCode)
      prepare_meta_result = id_mysql_stmt_result_metadata (sqlHandler->sqlStatement);
   
   if (!prepare_meta_result)  {
      con_printf ("mysql_stmt_result_metadata(), returned no meta information\n");
      con_printf ("%s\n", id_mysql_stmt_error(sqlHandler->sqlStatement));
      
      returnCode = MYSQL_NO_DATA;
   }
   
   if (!returnCode)  {
      
      column_count = id_mysql_num_fields (prepare_meta_result);
      
      if (column_count != row_count)  {  /* validate column count */
         con_printf ("Invalid column count returned by MySQL\n");
         returnCode = MYSQL_NO_DATA;
      }
      /*
      numRows = (long)mysql_num_rows (prepare_meta_result);  // USELESS!
      
      if (numRows > *itemCount)
         returnCode = MYSQL_NO_DATA;*/
   }
   
   if (!returnCode)
      returnCode = id_mysql_stmt_execute (sqlHandler->sqlStatement);
   
   sqlHandler->dbAllCallsCounter++;
   if (!returnCode)  {
      sqlHandler->dbSuccessCallsCounter++;
      returnCode = id_mysql_stmt_store_result (sqlHandler->sqlStatement);  // Move everything to the client
   }
   
   if (!returnCode)  {
      long  numRows = (long)id_mysql_stmt_num_rows (sqlHandler->sqlStatement);
      
      if (numRows > *itemCount)
         returnCode = MYSQL_NO_DATA;
   }
   
   *itemCount = 0;
   
   if (!returnCode)  {
      // The Loop
      while (!(returnCode=id_mysql_stmt_fetch(sqlHandler->sqlStatement)))  {
         elemPtr = buffPtr;
         for (i=0; i<row_count; i++)  {
            elemPtr[data_size[asColumns ? i : 0]] = '\0';   // When asColumns data_size has a lot of elems, when not only one!!!
            strNCpy (retStringArray[idx], elemPtr, itemSize);
            elemPtr += (itemSize+1);
            idx++;
         }
      }
      
      if (idx)  // we did something
         returnCode = 0;
   }
   else
      con_printf ("Failed id_SQLGetTextExecutingSqlStatement: %ld\n", returnCode);
   
   id_SQLCloseStatement (NULL);
   
   if (prepare_meta_result)
      id_mysql_free_result (prepare_meta_result);
   
   *itemCount = idx;
   
   if (buffPtr != locBuffer)
      id_DisposePtr ((Ptr)buffPtr);
   if (results != result)
      id_DisposePtr ((Ptr)results);
   
   if (data_size)
      id_DisposePtr (data_size);
   
   return (returnCode);
}

int  id_SQLGetLongExecutingSqlStatement (char *sqlString, long *retCode)
{
   // NSString      *preparedName = [dbFileHelper sqlStringWithString:iName];
   long        returnCode = 0;
   long        longStorage;
   my_bool     is_null[1];
   MYSQL_BIND  result[1];
   
   *retCode = 0L;
   
   returnCode = id_SQLInitPrepare (NULL, sqlString);
   
   // Idiote - provjeri kolko ih ima, mora biti ONLY ONE!
   
   if (!returnCode)
      returnCode = id_mysql_stmt_execute (sqlHandler->sqlStatement);

   if (!returnCode)
      sqlHandler->dbSuccessCallsCounter++;
   sqlHandler->dbAllCallsCounter++;

   memset (&result[0], 0, sizeof (MYSQL_BIND));
   
   // con_printf ("SQLGetLongExecutingSqlStatement - str: %s\n", sqlString);
   
   // Result
   result[0].buffer_type   = MYSQL_TYPE_LONG;
   result[0].buffer        = (void *)&longStorage;
   result[0].buffer_length = sizeof(long);
   result[0].is_unsigned   = 0;
   result[0].is_null       = &is_null[0];
   result[0].length        = NULL;
   
   // Bind result
   if (!returnCode)
      returnCode = id_mysql_stmt_bind_result (sqlHandler->sqlStatement, &result[0]);
   
   if (!returnCode)
      returnCode = id_mysql_stmt_store_result (sqlHandler->sqlStatement);  // Move everything to the client
   
   if (!returnCode)
      returnCode = id_mysql_stmt_fetch (sqlHandler->sqlStatement);
   
   // Deallocate result set
   if (!returnCode)
      *retCode = longStorage;
   else
      con_printf ("Failed id_SQLGetLongExecutingSqlStatement: %ld\n", returnCode);

   id_SQLCloseStatement (NULL);
   
   return (returnCode);
}

// .........................................................................................................

#pragma mark -

// .........................................................................................................

// Huh? Ovo nikom ne treba....
#ifdef _NOT_YET_
int  id_SQLPrepareStatement (MYSQL *mysql, char *sqlString, MYSQL_STMT **retStmt, short *retParamCnt)
{
   int  returnCode, paramCnt = 0;
   int  strLen = strlen (sqlString);

   MYSQL_STMT  *tmpStatement = NULL;
   
   tmpStatement = id_mysql_stmt_init (mysql);
   
   if (!tmpStatement)  {
      con_printf ("mysql_stmt_init(), out of memory\n");
      exit (0);
   }
   else
      *retStmt = tmpStatement;
   
   returnCode = id_mysql_stmt_prepare (tmpStatement, sqlString, strlen(sqlString));
   
   if (!returnCode && retParamCnt)  {
      paramCnt = id_mysql_stmt_param_count (tmpStatement);
      *retParamCnt = (short)paramCnt;
   }
      
   return (returnCode);
}
#endif

// .........................................................................................................

// MAYBE FOR FETCH OR SOMETHING, 
// LEAVE IT FOR NOW!

#ifdef _NOT_YET_
/*
- (int)stepWithSqlStatement:(sqlite3_stmt *)statement
{
   BOOL  retryFlag = TRUE;
   int   maxRetries = kMaxBusyRetryCount;
   int   numberOfRetries = 0;
   int   returnCode;
   
   if ([NSThread isMainThread])
      maxRetries *= 3;
   
   while (retryFlag)  {
      returnCode = sqlite3_step (statement);
      
      if ((returnCode == SQLITE_BUSY) || (returnCode == SQLITE_LOCKED))  {
         if (numberOfRetries++ <= maxRetries)
            usleep (20);
         else
            retryFlag = NO;
      }
      else
         retryFlag = NO;
   }
   
   if (returnCode != SQLITE_ROW && returnCode != SQLITE_DONE)
      NSLog (@"Error sqlite3_step() - [%d] - %s after %d retries.",
             returnCode, sqlite3_errmsg(self.dbHandle), numberOfRetries);
   
   return (returnCode);
}
 */
#endif

// .........................................................................................................

#pragma mark -
#pragma mark - Setup

ID_CT_MYSQL  *id_SQLSetupOneTable (
 RecFieldInfo  *rfi,
 IFIL          *ifilPtr,
 long           recSize
)
{
   ID_CT_MYSQL  *ctSql = (ID_CT_MYSQL *)id_malloc_clear_or_exit (sizeof(ID_CT_MYSQL));
   
   // id_InitSQLStorage ();
   
   if (!recSize)
      recSize = ifilPtr ? ifilPtr->dreclen : rfi->iRecSize;
   
   id_SQLInitOneTable (ctSql, rfi, ifilPtr, recSize);
   
   id_addPointerElem (&sqlHandler->dbSqlInfos, (void *)ctSql);
   
   // con_printf ("Elem: %s, %d\n", recName, id_pointerElemsCount(&gRecFieldInfo));
   
   return (ctSql);
}

int  id_SQLInitOneTable (
 ID_CT_MYSQL   *ctSql,
 RecFieldInfo  *rfi,
 IFIL          *ifilPtr,
 long           recSize
)
{
   CTTPtr  theCTT = ifilPtr ? id_CTTFindByDatNo (ifilPtr->tfilno) : NULL;  // Or id_CTTAddFile (ifilPtr) -> it is added in ct_OpenOneFile() so if I don't change that, add it here
   
   if (!theCTT && ifilPtr)  {
      id_CTTAddFile (ifilPtr);
      // ifilsPtr->tfilno = invent it besides ctree, it goes file by file plus number of indices, so KUPDOB is 1, 1st key is 2 etc
      theCTT = id_CTTFindByDatNo (ifilPtr->tfilno);
   }
   
   if (theCTT)
      theCTT->ctSqlPtr = ctSql;
   else  if (ifilPtr)
      con_printf ("SQLSetup - no CTTPtr\n");
   
   ctSql->dbHandle = sqlHandler->dbs->dbHandle;
   
   strNCpy (ctSql->dbName, id_SQLActiveDatabaseName(), 127);

   ctSql->recStructFldCount = rfi->iFldCnt;   // net elems
   ctSql->recSqlFldCount = 0;                 // with sub elems, determined below
   ctSql->skippedFldCount = 0;
   ctSql->curTblStateIdx = 0;

   ctSql->fileIndex = -1;
   ctSql->recSize = recSize;
   ctSql->recSizeWithId = recSize + sizeof (long);

   ctSql->currentRecFlags = kCurrInvalid;

   if (ifilPtr)  {
      ctSql->ifilPtr = ifilPtr;
      ctSql->datNo = ifilPtr->tfilno;
      ctSql->dNumIdx = ifilPtr->dnumidx;
      strNCpy (ctSql->tableName, ifilPtr->pfilnam, 63);
      ctSql->maxChkSegments = ct_GetMaxKeySegments (ifilPtr) * 2;  // where + sort
   }
   else  {
      strNCpy (ctSql->tableName, kMYSQLFlatFileName/*rfi->iRecName*/, 63);
      ctSql->datNo = id_SQLAvailableDatNo (TRUE);  // sqlHandler->lastFlatDatNo;  // kMYSQLFlatFileDatNo;
      ctSql->maxChkSegments = 1;
      ctSql->dNumIdx = 1;
   }

   id_SQLSanitizeTableName (ctSql->tableName);  // Spaces, $#& and everything else

   ctSql->paramCount = ctSql->resultCount = ctSql->checkCount = 0;
   
   ctSql->rfi = rfi;
   
   ctSql->recSqlFldCount = id_SQLCountUsedFields (ctSql, rfi, &ctSql->unSig.unsignedFldCount);
   
   if (ctSql->unSig.unsignedFldCount)  {
      ctSql->unSig.uParams = (SQLOneUnsigned *) id_malloc_clear_or_exit (sizeof(SQLOneUnsigned) * (ctSql->unSig.unsignedFldCount));
      ctSql->unSig.uChecks = (SQLOneUnsigned *) id_malloc_clear_or_exit (sizeof(SQLOneUnsigned) * (ctSql->unSig.unsignedFldCount * 2));
      ctSql->unSig.uResults = (SQLOneUnsigned *) id_malloc_clear_or_exit (sizeof(SQLOneUnsigned) * (ctSql->unSig.unsignedFldCount));
   }
   
   // Records
   
   ctSql->paramRecord = id_malloc_clear_or_exit (ctSql->recSizeWithId);
   ctSql->checkRecord = id_malloc_clear_or_exit (ctSql->recSizeWithId);
   ctSql->resultRecord = id_malloc_clear_or_exit (ctSql->recSizeWithId);
   ctSql->currentRecord = id_malloc_clear_or_exit (ctSql->recSizeWithId);
   
   // Buffers
   
   ctSql->paramBuffers = id_malloc_clear_or_exit (sizeof(char *) * (ctSql->recSqlFldCount));  // +1 for id
   ctSql->checkBuffers = id_malloc_clear_or_exit (sizeof(char *) * (ctSql->recSqlFldCount+1));
   ctSql->resultBuffers = id_malloc_clear_or_exit (sizeof(char *) * (ctSql->recSqlFldCount+1));
   
   // Bindings
   
   ctSql->paramBindInfos = (ID_BIND_INFO *)id_malloc_clear_or_exit (sizeof(ID_BIND_INFO) * (ctSql->recSqlFldCount));
   ctSql->checkBindInfos = (ID_BIND_INFO *)id_malloc_clear_or_exit (sizeof(ID_BIND_INFO) * (ctSql->maxChkSegments+1));  // +1 for id
   ctSql->parChkBinds = (MYSQL_BIND *)id_malloc_clear_or_exit (sizeof(MYSQL_BIND) * (ctSql->recSqlFldCount + ctSql->maxChkSegments + 1));  // Shared binds for params and checks +1
   
   ctSql->resultBindInfos = (ID_BIND_INFO *)id_malloc_clear_or_exit (sizeof(ID_BIND_INFO) * (ctSql->recSqlFldCount+1));  // +1 for id
   ctSql->resultBinds = (MYSQL_BIND *)id_malloc_clear_or_exit (sizeof(MYSQL_BIND) * (ctSql->recSqlFldCount+1));          // +1 for id
   
   id_SQLInitializeBindings (&ctSql->paramBindInfos[0], &ctSql->parChkBinds[0], ctSql->recSqlFldCount);
   id_SQLInitializeBindings (&ctSql->checkBindInfos[0], &ctSql->parChkBinds[ctSql->recSqlFldCount], ctSql->maxChkSegments+1);  // After the ctSql->recSqlFldCount
   id_SQLInitializeBindings (&ctSql->resultBindInfos[0], &ctSql->resultBinds[0], ctSql->recSqlFldCount+1);
   
   // Create SQLFldDescription, calc real field numbers
   
   ctSql->fldDescs = id_SQLBuildNewFldDescriptions (ctSql, rfi);
   
   id_SQLInitTableState (ctSql, kMYSQLBufferedRows);
   
   return (0);
}

// .........................................................................................................

static void  id_SQLAllocVarLenBuffer (
 ID_CT_MYSQL  *ctSql,
 long          buffSize
)
{
   if (ctSql->varLenBuffPtr && (ctSql->varLenBuffSize < buffSize))  {
      id_DisposePtr (ctSql->varLenBuffPtr);
      ctSql->varLenBuffPtr = NULL;
   }
   
   if (!ctSql->varLenBuffPtr)
      ctSql->varLenBuffPtr = id_malloc_or_exit (buffSize);
   
   ctSql->varLenBuffSize = buffSize;
}

// .........................................................................................................

int  id_SQLCreateTable (
 ID_CT_MYSQL  *ctSql,   // -> MYSQL       *dbHandle
 char         *tableName
)
{
   int    returnCode = -1;
   char  *sqlStrPtr = id_malloc_or_exit (kMYSQLStringBufferLen);
   char  *errMsg = NULL;
   
   if (sqlHandler && ctSql->dbHandle)  {
      if (tableName)
         strNCpy (ctSql->tableName, tableName, 63);
      
      id_SQLCreationStringForTable (ctSql, sqlStrPtr, kMYSQLStringBufferLen);

      returnCode = id_SQLPerformQuery (ctSql->dbHandle, sqlStrPtr, NULL/*retErrStr*/, 0/*errMaxLen*/);
   }

   id_DisposePtr ((Ptr)sqlStrPtr);

   return (returnCode);
}

// .........................................................................................................

int  id_SQLAlterTable (ID_CT_MYSQL *ctSql, short *chgCount)
{
   short               i, six, keyNo, fldCnt=0;
   int                 returnCode = 0;
   char                tmpStr[256];
   char               *sqlStrPtr = id_malloc_or_exit (kMYSQLStringBufferLen);
   // RecFieldOffset     *rfoArray = theRfi->iRecFldOffsets;
   SQLFldDescription  *fldDescCArray = ctSql->fldDescs;
   
   if (chgCount)
      *chgCount = 0;

   for (i=0; fldDescCArray[i].fldName && !returnCode; i++)  {
      
      if (id_SQLCheckColumnExist(NULL, ctSql->tableName, fldDescCArray[i].fldName))  {
         
         id_SQLColumnAppendingStringForTable (ctSql->tableName, &fldDescCArray[i], sqlStrPtr, kMYSQLStringBufferLen);
         
         returnCode = id_SQLPerformQuery (ctSql->dbHandle, sqlStrPtr, NULL/*retErrStr*/, 0/*errMaxLen*/);
         
         if (!returnCode && chgCount)
            (*chgCount)++;
      }
   }
   
   // Indexi
   
   if (ctSql->ifilPtr)  {
      
      short  needToRebuild = FALSE;
      
      for (keyNo=1; keyNo<=id_SQLGetNumberOfKeys(ctSql); keyNo++)  {
            
         snprintf (tmpStr, 256, "index_%hd", keyNo);
      
         if (!ct_DuplicateKey(ctSql->ifilPtr, keyNo) && id_SQLCheckIndexExist(NULL, ctSql->tableName, tmpStr))
            needToRebuild = TRUE;
      }
      
      if (needToRebuild)  {
         for (keyNo=1; keyNo<=id_SQLGetNumberOfKeys(ctSql); keyNo++)  {
            id_SQLDeleteIndexStringForTable (ctSql, keyNo, sqlStrPtr, kMYSQLStringBufferLen);
            returnCode = id_SQLPerformQuery (ctSql->dbHandle, sqlStrPtr, NULL/*retErrStr*/, 0/*errMaxLen*/);
         }
         
         for (keyNo=1; keyNo<=id_SQLGetNumberOfKeys(ctSql); keyNo++)  {
         
            if (ct_DuplicateKey(ctSql->ifilPtr, keyNo))  continue;
         
            snprintf (tmpStr, 256, "index_%hd", keyNo);

            if (id_SQLCheckIndexExist(NULL, ctSql->tableName, tmpStr))  {
               con_printf ("Table: %s, index: %s does not exist!\n", ctSql->tableName, tmpStr);
               id_SQLIndexAppendingStringForTable (ctSql, NULL, keyNo, sqlStrPtr, kMYSQLStringBufferLen);
         
               returnCode = id_SQLPerformQuery (ctSql->dbHandle, sqlStrPtr, NULL/*retErrStr*/, 0/*errMaxLen*/);
         
               if (!returnCode && chgCount)
                  (*chgCount)++;
            }
         }
      }
   }
   else  {
      keyNo = 1;
      if ((fldDescCArray[1].ctType == kTchar) && !strcmp(fldDescCArray[1].fldName, kMYSQLFlatKeyName))  {  // Should be only 1
         snprintf (tmpStr, 256, "index_%hd", keyNo);
      
         if (id_SQLCheckIndexExist(NULL, ctSql->tableName, tmpStr))  {
            con_printf ("Table: %s, index: %s does not exist!\n", ctSql->tableName, tmpStr);
            id_SQLIndexAppendingStringForTable (ctSql, &fldDescCArray[1], keyNo, sqlStrPtr, kMYSQLStringBufferLen);
            
            returnCode = id_SQLPerformQuery (ctSql->dbHandle, sqlStrPtr, NULL/*retErrStr*/, 0/*errMaxLen*/);
            
            if (!returnCode && chgCount)
               (*chgCount)++;
         }
      }
   }

   
   id_DisposePtr ((Ptr)sqlStrPtr);
   
   return (returnCode);  // Nope
}

// .........................................................................................................

int  id_SQLCopyTable (
 char  *targetDBName,
 char  *sourceDBName,
 char  *tableName
)
{
   int    returnCode = -1;
   char  *sqlStrPtr = id_malloc_or_exit (kMYSQLStringBufferLen);
   char  *errMsg = NULL;
   
   if (sqlHandler && sqlHandler->dbs->dbHandle)  {
      
      // CREATE TABLE new_table LIKE original_table;
      
      snprintf (sqlStrPtr, kMYSQLStringBufferLen, "CREATE TABLE %s.%s LIKE %s.%s",
                targetDBName, tableName,
                sourceDBName, tableName);

      returnCode = id_SQLPerformQuery (sqlHandler->dbs->dbHandle, sqlStrPtr, NULL/*retErrStr*/, 0/*errMaxLen*/);
      
      if (!returnCode)  {
         
         // INSERT INTO new_table SELECT * FROM original_table;
         
         snprintf (sqlStrPtr, kMYSQLStringBufferLen, "INSERT INTO %s.%s SELECT * FROM %s.%s",
                   targetDBName, tableName,
                   sourceDBName, tableName);
         
         returnCode = id_SQLPerformQuery (sqlHandler->dbs->dbHandle, sqlStrPtr, NULL/*retErrStr*/, 0/*errMaxLen*/);
      }
   }

   id_DisposePtr ((Ptr)sqlStrPtr);

   return (returnCode);
}

// .........................................................................................................

int  id_SQLCopyTablesArray (
 SQLTarget  *sqlTarget,  // source database
 char       *targetDBName,
 char       *sourceDBName,
 char       *tableName   // make it array
)
{
   short  ctErr = -1;
   
   if (!sqlTarget || id_SQLInitWithUserAndServer(sqlTarget->tUserName, sqlTarget->tUserPassword, sqlTarget->tServerAddress, sqlTarget->tServerPort))
      return (-1);
   
   // id_SQLSetRecSignatureHandler (pr_DatabaseRecSignature);

   if (!id_SQLCheckDatabaseExist(targetDBName))  {
      con_printf ("Baza podataka %s postoji od ranije!", targetDBName);
      return (-1);
   }
      
   ctErr = id_SQLCreateNewDatabase (NULL, targetDBName, kAuxilaryDatabase, NULL, 0);  // or kMainDatabase if there's any problem
   
   if (!ctErr)  {
      ctErr = id_SQLCopyTable (targetDBName, sourceDBName, tableName);
   }
   
   if (!id_SQLCountActiveTables(sqlHandler->dbs->dbHandle))  {
      id_mysql_close (sqlHandler->dbs->dbHandle);
      sqlHandler->dbs->dbHandle = NULL;
   }

   return (ctErr);
}

// .........................................................................................................

// disposes all internals and the structure itself TOO!

void  id_SQLDisposeOneTable (ID_CT_MYSQL *ctSql)
{
   short  i, idx, activeTables = id_SQLCountActiveTables (ctSql->dbHandle);
   
   SQLDatabaseServer  *dbs = id_SQLGetServerInfo (ctSql, &idx);
   
   
   id_DisposeCharAddress (&ctSql->varLenBuffPtr);
   
   if (ctSql->unSig.unsignedFldCount)  {
      id_DisposePtr (ctSql->unSig.uParams);
      id_DisposePtr (ctSql->unSig.uChecks);
      id_DisposePtr (ctSql->unSig.uResults);
   }

   // Records
   
   id_DisposePtr (ctSql->paramRecord);
   id_DisposePtr (ctSql->checkRecord);
   id_DisposePtr (ctSql->resultRecord);
   id_DisposePtr (ctSql->currentRecord);
   
   // Buffers
   
   id_DisposePtr (ctSql->paramBuffers);
   id_DisposePtr (ctSql->checkBuffers);
   id_DisposePtr (ctSql->resultBuffers);
   
   // Bindings
   
   id_DisposePtr (ctSql->paramBindInfos);
   id_DisposePtr (ctSql->checkBindInfos);
   id_DisposePtr (ctSql->parChkBinds);  // Shared binds for params and checks
   
   id_DisposePtr (ctSql->resultBindInfos);
   id_DisposePtr (ctSql->resultBinds);
   
   for (i=0; ctSql->fldDescs[i].fldName; i++)
      id_DisposePtr (ctSql->fldDescs[i].fldName);
   
   // Handler's counters
   
   if (dbs)  {
      if (!strcmp(ctSql->dbName, dbs->mainDbName))
         dbs->mainTableCounter--;
      else  if (!strcmp(ctSql->dbName, dbs->commonDbName))
         dbs->commonTableCounter--;
      else  if (!strcmp(ctSql->dbName, dbs->auxDbName))
         dbs->auxTableCounter--;
      else
         con_printf ("id_SQLDisposeOneTable - ctSql->dbName not in sqlHandler->dbs!\n");
   }
   else
      con_printf ("id_SQLDisposeOneTable - Can't find db handler!!!\n");
   
   id_DisposePtr (ctSql->fldDescs); 
   
   if (ctSql->tblState)  {
      id_SQLDisposeTableStatePageData (ctSql, -1);
      // id_SQLDisposeTableStateSelParamData (ctSql, -1);
      // for (i=0; i<kStandardSelSets; i++)  {
      //    if (ctSql->tblState[ctSql->curTblStateIdx].pageData)  {
      //       id_DisposePtr (ctSql->tblState[ctSql->curTblStateIdx].pageData);
      //       ctSql->tblState[ctSql->curTblStateIdx].pageData = NULL;
      //    }
      // }
      id_DisposePtr (ctSql->tblState);  // kStandardSelSets
      ctSql->tblState = NULL;
   }
   
   id_removePointerElem (&sqlHandler->dbSqlInfos, 0, ctSql);
   
   id_DisposePtr (ctSql);  // Set it to NULL outside
   
   if (activeTables == 1)  {  // was before we closed this one
      if (sqlHandler->dbServers[idx].dbHandle)  {
         // disconnect from server, but don't terminate client library
         id_mysql_close (sqlHandler->dbServers[idx].dbHandle);
         sqlHandler->dbServers[idx].dbHandle = NULL;
      }
   }
}

// .........................................................................................................

#pragma mark -

MYSQL_BIND  *id_SQLBindText (
 ID_CT_MYSQL   *ctSql,
 ID_BIND_INFO  *bindInfo,
 const char    *txt,
 short          strLen,  // for writing
 short          maxLen   // for reading
)
{
   // int          returnCode;
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   if (!aBind)
      return (NULL);
   
   id_SetBlockToZeros (aBind, sizeof(MYSQL_BIND));

   if (!txt)
      bindInfo->biIsNull = TRUE;
   else
      bindInfo->biDataSize = strLen;
   
   aBind->buffer_type= MYSQL_TYPE_STRING;
   aBind->buffer = (void *)txt;
   aBind->buffer_length = maxLen;
   aBind->is_null = &bindInfo->biIsNull;
   aBind->length = &bindInfo->biDataSize;
   aBind->error  = &bindInfo->biError;
   
   /*
   if (!txt)
      returnCode = sqlite3_bind_null (statement, paramNum);
   else
      returnCode = sqlite3_bind_text (statement, paramNum, (const char *)txt, -1, asTrans ? SQLITE_TRANSIENT : SQLITE_STATIC);
   */
   
   return (aBind);
}

// .........................................................................................................

// - (int)bindInt:(int)intVal withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum
MYSQL_BIND  *id_SQLBindLong (
 ID_CT_MYSQL   *ctSql,
 ID_BIND_INFO  *bindInfo,
 long          *longPtr,
 Boolean        unsignedFlag
)
{
   // int          returnCode;
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   if (!aBind)
      return (NULL);
   
   id_SetBlockToZeros (aBind, sizeof(MYSQL_BIND));

   bindInfo->biDataSize = sizeof (long);
   
   aBind->is_unsigned = unsignedFlag ? 1 : 0;

   aBind->buffer_type = MYSQL_TYPE_LONG;
   aBind->buffer = (char *)longPtr;
   aBind->buffer_length = sizeof (long);
   aBind->is_null = &bindInfo->biIsNull;
   aBind->length  = &bindInfo->biDataSize;
   aBind->error   = &bindInfo->biError;
   
   // returnCode = sqlite3_bind_int (statement, paramNum, intVal);
   
   // retNum = strtoul (numBuffer, &tempPtr, 0);

   
   return (aBind);
}

// .........................................................................................................

// - (int)bindInt:(int)intVal withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum
MYSQL_BIND  *id_SQLBindShort (
 ID_CT_MYSQL   *ctSql,
 ID_BIND_INFO  *bindInfo,
 short         *shortPtr,
 Boolean        unsignedFlag
)
{
   // int          returnCode;
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   if (!aBind)
      return (NULL);
   
   id_SetBlockToZeros (aBind, sizeof(MYSQL_BIND));

   bindInfo->biDataSize = sizeof (short);
   
   aBind->is_unsigned = unsignedFlag ? 1 : 0;

   aBind->buffer_type = MYSQL_TYPE_SHORT;
   aBind->buffer = (char *)shortPtr;
   aBind->buffer_length = sizeof (short);
   aBind->is_null = &bindInfo->biIsNull;
   aBind->length  = &bindInfo->biDataSize;
   aBind->error   = &bindInfo->biError;
   
   // returnCode = sqlite3_bind_int (statement, paramNum, intVal);
   
   return (aBind);
}

// .........................................................................................................

// - (int)bindInt:(int)intVal withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum
MYSQL_BIND  *id_SQLBindChar (
 ID_CT_MYSQL   *ctSql,
 ID_BIND_INFO  *bindInfo,
 char          *charPtr,
 Boolean        unsignedFlag
)
{
   // int          returnCode;
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   if (!aBind)
      return (NULL);
   
   id_SetBlockToZeros (aBind, sizeof(MYSQL_BIND));

   bindInfo->biDataSize = sizeof (char);
   
   aBind->is_unsigned = unsignedFlag;

   aBind->buffer_type = MYSQL_TYPE_TINY;
   aBind->buffer = (char *)charPtr;
   aBind->buffer_length = sizeof (char);
   aBind->is_null = &bindInfo->biIsNull;
   aBind->length  = &bindInfo->biDataSize;
   aBind->error   = &bindInfo->biError;
   
   // returnCode = sqlite3_bind_int (statement, paramNum, intVal);
   
   return (aBind);
}

// .........................................................................................................

// - (int)bindInt:(int)intVal withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum
MYSQL_BIND  *id_SQLBindFloat (
 ID_BIND_INFO  *bindInfo,
 float         *floatPtr
)
{
   // int          returnCode;
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   if (!aBind)
      return (NULL);
   
   id_SetBlockToZeros (aBind, sizeof(MYSQL_BIND));

   bindInfo->biDataSize = sizeof (float);
      
   aBind->buffer_type = MYSQL_TYPE_FLOAT;
   aBind->buffer = (char *)floatPtr;
   aBind->buffer_length = sizeof (float);
   aBind->is_null = &bindInfo->biIsNull;
   aBind->length  = &bindInfo->biDataSize;
   aBind->error   = &bindInfo->biError;
   
   // returnCode = sqlite3_bind_int (statement, paramNum, intVal);
   
   return (aBind);
}

// .........................................................................................................

// - (int)bindInt:(int)intVal withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum
MYSQL_BIND  *id_SQLBindDouble (
 ID_CT_MYSQL   *ctSql,
 ID_BIND_INFO  *bindInfo,
 double        *doublePtr
)
{
   // int          returnCode;
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   if (!aBind)
      return (NULL);
      
   id_SetBlockToZeros (aBind, sizeof(MYSQL_BIND));

   bindInfo->biDataSize = sizeof (double);
      
   aBind->buffer_type = MYSQL_TYPE_DOUBLE;
   aBind->buffer = (char *)doublePtr;
   aBind->buffer_length = sizeof (double);
   aBind->is_null = &bindInfo->biIsNull;
   aBind->length  = &bindInfo->biDataSize;
   aBind->error   = &bindInfo->biError;
   
   // returnCode = sqlite3_bind_int (statement, paramNum, intVal);
   
   return (aBind);
}

// .........................................................................................................

// - (int)bindBlob:(const void *)ptr withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum ofLength:(NSInteger)length asTransient:(BOOL)asTrans
MYSQL_BIND  *id_SQLBindBlob (
 ID_CT_MYSQL   *ctSql,
 ID_BIND_INFO  *bindInfo,
 const char    *blobBuff,
 short          blobLen,  // for writing
 short          maxLen    // for reading
)
{
   // int          returnCode;
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   if (!aBind)
      return (NULL);

   id_SetBlockToZeros (aBind, sizeof(MYSQL_BIND));

   if (!blobBuff)
      bindInfo->biIsNull = TRUE;
   else
      bindInfo->biDataSize = blobLen;
   
   aBind->buffer_type = MYSQL_TYPE_BLOB;
   aBind->buffer = (char *)blobBuff;
   aBind->buffer_length = maxLen;            // max for reading, 0 for varLength, see page 2685 in C API Manual
   aBind->is_null = &bindInfo->biIsNull;
   aBind->length  = &bindInfo->biDataSize;     // actual size
   aBind->error   = &bindInfo->biError;
   
   /*
   if (!ptr)
      returnCode = sqlite3_bind_null (statement, paramNum);
   else
      returnCode = sqlite3_bind_blob (statement, paramNum, (const char *)ptr, length, asTrans ? SQLITE_TRANSIENT : SQLITE_STATIC);
   */
   
   return (aBind);
}

// .........................................................................................................

#pragma mark -

// .........................................................................................................

#ifdef _NOT_NEEDED_
// - (const unsigned char *)textWithSqlStatement:(sqlite3_stmt *)statement forColumnIndex:(NSInteger)idx
+int  id_SQLGetText (
 ID_BIND_INFO  *bindInfo,
 const char   **txt,
 short         *strLen,
)                    
{
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   *txt = aBind->buffer;
   
   if (bindInfo->biIsNull)  {
      txt[0] = '\0';
      *strLen = 0;
   }
   else
      *strLen = aBind->length;
   
   return (bindInfo->biError ? bindInfo->biError : 0);
   
   // return (sqlite3_column_text(statement, idx));
}

// .........................................................................................................

// - (int)intWithSqlStatement:(sqlite3_stmt *)statement forColumnIndex:(NSInteger)idx
+int  id_SQLGetInteger (
 ID_BIND_INFO  *bindInfo,
 long          *longInt
)
{
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   *longInt = bindInfo->biLongValue;
   
   if (bindInfo->biIsNull)  {
      *longInt = 0L;
   }
   
   return (bindInfo->biError ? bindInfo->biError : 0);
   
   //    return (sqlite3_column_int(statement, idx));
}

// .........................................................................................................

// - (const void *)blobWithSqlStatement:(sqlite3_stmt *)statement forColumnIndex:(NSInteger)idx andLength:(NSInteger *)retLength
+int  id_SQLGetBlob (
 ID_BIND_INFO  *bindInfo,
 const char   **blob,
 short         *dataLen,
)
{
   MYSQL_BIND  *aBind = bindInfo->biBind;
   
   *blob = aBind->buffer;
   
   if (bindInfo->biIsNull)
      *dataLen = 0;
   else
      *dataLen = aBind->length;
   
   return (bindInfo->biError ? bindInfo->biError : 0);
   
   //    const void  *retBytes = sqlite3_column_blob (statement, idx);
   
   // if (retLength)  {
   //    if (!retBytes)
   //       *retLength = 0;
   //    else
   //       *retLength = sqlite3_column_bytes (statement, idx);  // size in bytes
   // }
   
   // return (retBytes);
}

#endif // _NOT_NEEDED_

#pragma mark -

// .........................................................................................................

// On 2006 error, reconnect & retry

static long  id_SQLCoreInitPrepare (ID_CT_MYSQL *ctSql, char *sqlString, short secondTry)
{
   long          returnCode = 0;
   short        *pStatementPhase = &sqlHandler->statementPhase;
   MYSQL_STMT  **pSqlStatement = &sqlHandler->sqlStatement;
   
   if (strlen(sqlString) > kMYSQLStringBufferLen)
      con_printf ("id_SQLInitPrepare: %ld SqlString too long!\n", (long)strlen(sqlString));
   
   if (ctSql)  {
      ctSql->errNo = 0;
      
      ctSql->paramCount = ctSql->resultCount = ctSql->checkCount = 0;

      pStatementPhase = &ctSql->statementPhase;
      pSqlStatement   = &ctSql->sqlStatement;
   }
   
   if ((*pStatementPhase) == kMYSQLStmtPhaseDefault)  {  // INIT
      if (*pSqlStatement = id_mysql_stmt_init(ctSql ? ctSql->dbHandle : sqlHandler->dbs->dbHandle))
         (*pStatementPhase) = kMYSQLStmtPhaseInit;
   }
   
   if (*pSqlStatement && ((*pStatementPhase) == kMYSQLStmtPhaseInit))  {  // PREPARE
      returnCode = id_mysql_stmt_prepare (*pSqlStatement, sqlString, strlen(sqlString));
      if (!returnCode)
         (*pStatementPhase) = kMYSQLStmtPhasePrepare;
      else  {
         if (ctSql)
            ctSql->errNo = id_mysql_errno (ctSql->dbHandle);
         con_printf ("MySQL: (%ld) %s\n", (long)id_mysql_stmt_errno(*pSqlStatement), id_mysql_stmt_error(*pSqlStatement));
         con_printf ("???\n%s\n???\n", sqlHandler->lastSqlString);
         
         if (!secondTry && (id_mysql_stmt_errno(*pSqlStatement) == 2006L))  {
            SQLDatabaseServer  *dbs = ctSql ? id_SQLGetServerInfo (ctSql, NULL) : sqlHandler->dbs;

            if (!id_SQLConnectToServer(dbs/*sqlHandler->dbs->usrName, sqlHandler->dbs->usrPassword, sqlHandler->dbs->srvAddress*/))  {
               if (!id_mysql_select_db(dbs->dbHandle, id_SQLDatabaseName(dbs, dbs->activeDatabase)))  {  // can't call here id_SQLActiveDatabaseName()
                  con_printf ("MySQL: id_mysql_select_db - Reconnection SUCCESS!\n");
                  (*pStatementPhase) = kMYSQLStmtPhaseDefault;
                  
                  return (id_SQLCoreInitPrepare(ctSql, sqlString, TRUE));  // Re entry
               }
               else
                  con_printf ("MySQL: id_mysql_select_db - Reconnection FAIL!\n");
            }
         }
      }
   }
   else
      return (-1);
   
   return (returnCode);
}

static long  id_SQLInitPrepare (ID_CT_MYSQL *ctSql, char *sqlString)
{
   return (id_SQLCoreInitPrepare(ctSql, sqlString, FALSE));
}

// .........................................................................................................

// mysql_stmt_free_result()
// Frees the memory allocated for a result set by mysql_store_result(), mysql_use_result(), mysql_list_dbs(), and so forth. When you are done with a result set, you must free the memory it uses by calling mysql_free_result().
// Do not attempt to access a result set after freeing it.
// mysql_stmt_close()
// Closes the prepared statement. mysql_stmt_close() also deallocates the statement handle pointed to by stmt.
// If the current statement has pending or unread results, this function cancels them so that the next query can be executed.

// .........................................................................................................

static long  id_SQLCloseStatement (ID_CT_MYSQL *ctSql)
{
   long          returnCode = 0;
   short        *pStatementPhase = &sqlHandler->statementPhase;
   MYSQL_STMT  **pSqlStatement = &sqlHandler->sqlStatement;
   
   if (ctSql)  {
      pStatementPhase = &ctSql->statementPhase;
      pSqlStatement   = &ctSql->sqlStatement;
   }

   if ((*pStatementPhase) != kMYSQLStmtPhaseDefault)  {
      id_mysql_stmt_free_result (*pSqlStatement);
      returnCode = id_mysql_stmt_close (*pSqlStatement);
      
      (*pStatementPhase) = kMYSQLStmtPhaseDefault;
      
      *pSqlStatement = NULL;
   }
   
   return (returnCode);
}

// .........................................................................................................

static void  id_SQLCopyResultRecord (ID_CT_MYSQL *ctSql, void *tarRecPtr, short pgIndex)
{
   // tarRecPtr is NULL when we're just restoring current record - GETCURP/SETCURI
   if (tarRecPtr)
      BlockMove (ctSql->resultRecord, tarRecPtr, ctSql->recSize);                 // External record
   BlockMove (ctSql->resultRecord, ctSql->currentRecord, ctSql->recSizeWithId);   // Internal record
   
   if (tarRecPtr && ctSql->ifilPtr)
      id_SQLConvertRecordEncoding (ctSql, tarRecPtr, TRUE);  // reverse flag

   ctSql->currentRecFlags |= (kCurrValid4Start | kCurrValid4Flow);

   // id_PrintRecFields (ctSql->rfi->iRecName, 0, ctSql->currentRecord);
   if (ctSql->tblState)
      ctSql->tblState[ctSql->curTblStateIdx].pageIndex = pgIndex;
}

// .........................................................................................................

// Reorders records and sets the ctSql->resultRecord if needed
// Ovo se ne bi trebalo koristiti kod varLen

static void  id_SQLStoreResultRecords (ID_CT_MYSQL *ctSql, void *tarRecPtr, short keyNo, short rowCount, short reqRowCount, short reverseOrder)
{
   if (ctSql->tblState)  {
      ctSql->tblState[ctSql->curTblStateIdx].nothingForward = ctSql->tblState[ctSql->curTblStateIdx].nothingBackward = FALSE;
      
      if (rowCount < reqRowCount)  {
         if (!reverseOrder)
            ctSql->tblState[ctSql->curTblStateIdx].nothingForward = TRUE;
         else
            ctSql->tblState[ctSql->curTblStateIdx].nothingBackward = TRUE;
      }
      
      ctSql->tblState[ctSql->curTblStateIdx].tsKeyNo = keyNo;
      ctSql->tblState[ctSql->curTblStateIdx].usedPageRows = rowCount;
      ctSql->tblState[ctSql->curTblStateIdx].pageIndex = 0;  // Initially, change later if needed
#ifdef  _SHOW_SELECTTION_PROGRESS_
      con_printf ("Row count: %d\n", rowCount);
#endif      
      if (rowCount && reverseOrder)  {
         short  i, j, mid = rowCount / 2;

         if (rowCount == 1)
            BlockMove (ctSql->tblState[ctSql->curTblStateIdx].pageData, ctSql->resultRecord, ctSql->recSizeWithId);  // resultRec was set to zero
         else  for (i=0,j=rowCount-1; i<mid && i<j; i++,j--)  {
            char  *loPtr = ctSql->tblState[ctSql->curTblStateIdx].pageData + (ctSql->recSizeWithId * i);
            char  *hiPtr = ctSql->tblState[ctSql->curTblStateIdx].pageData + (ctSql->recSizeWithId * j);
            
            // ctSql->currentRecord used as temp storage
            
            if (!i)
               BlockMove (loPtr, ctSql->resultRecord, ctSql->recSizeWithId);  // First is last

            BlockMove (loPtr, ctSql->currentRecord, ctSql->recSizeWithId);    // currentRecord used as temp buffer, nothing related to the current record
            BlockMove (hiPtr, loPtr, ctSql->recSizeWithId);
            BlockMove (ctSql->currentRecord, hiPtr, ctSql->recSizeWithId);
         }

         id_SQLCopyResultRecord (ctSql, tarRecPtr, rowCount-1);  // tarRecPtr may be NULL
      }
   }
   else  if (rowCount > 1)
      con_printf ("Wasted rows!!! Row count: %d\n", rowCount);
}

// .........................................................................................................

#pragma mark -
#pragma mark SQL Strings Forming
#pragma mark -

// .........................................................................................................

// + (NSString *)creationTypeStringForFldDescription:(SQLFldDescription *)fldDesc
static char  *id_SQLTypeCreationString (SQLFldDescription *fldDesc)
{
   static char  buffer[128];
   
   char  *retStr = NULL;
   
   buffer[0] = '\0';
   
   switch (fldDesc->ctType)  {
      case  kTint:   retStr = "INTEGER";  break;
      case  kTuint:  retStr = "INTEGER UNSIGNED";  break;

      case  kTshort:   retStr = "SMALLINT";  break;
      case  kTushort:  retStr = "SMALLINT UNSIGNED";  break;
      
      case  kTlong:   retStr = "INT";  break;
      case  kTulong:  retStr = "INT UNSIGNED";  break;
      
      case  kTchar:
         if (fldDesc->maxLen > 1)  {
            sprintf (buffer, "VARCHAR(%ld)", fldDesc->maxLen);
            retStr = buffer;
         }
         else
            retStr = "TINYINT";
         break;
      case  kTuchar:
         if (fldDesc->maxLen > 1)  {
            sprintf (buffer, "VARCHAR(%ld)", fldDesc->maxLen);
            retStr = buffer;
         }
         else
            retStr = "TINYINT UNSIGNED";
         break;
      
      case  kTfloat:   retStr = "FLOAT";  break;
      case  kTdouble:  retStr = "DOUBLE";  break;

      case  kTbytes:
      default:
         retStr = "BLOB";  break;
   }
#ifdef _NIJE_
   switch (fldDesc->sqlType)  {
      case  MYSQL_TYPE_LONG:    retStr = "INTEGER";  break;
      case  MYSQL_TYPE_DOUBLE:  retStr = "DOUBLE";   break;
      case  MYSQL_TYPE_FLOAT:   retStr = "FLOAT";    break;
      case  MYSQL_TYPE_BLOB:    retStr = "";     break;
      case  MYSQL_TYPE_NULL:    retStr = "NULL";     break;
         
      default:                  retStr = "TEXT";     break;
   }
   
   if (fldDesc->fldFlags & kFldDescFlagUniqueKey)
      return ([NSString stringWithFormat:"%@ UNIQUE", retStr]);
#endif
   
   return (retStr);
}

// .........................................................................................................

// Ovo kreira WHERE string, meni za Bouquet ne treba full funkcija svega toga

#ifdef _NIJE_
+ (NSString *)sqlWhereClauseStringWithSearchFields:(NSArray *)fldNamesToMatchOrNil
                             withSearchFieldValues:(NSArray *)fldValuesToMatchOrNil        // just to check for nulls
                             withSearchFieldRanges:(NSArray *)fldRangesToMatchOrNil
                              usingFldDescriptions:(SQLFldDescription *)fldDescCArray
                                        exactMatch:(BOOL)exactMatch  // if NOT, use LIKE
                                returningUsedCount:(int *)boundCnt;
{
    NSMutableString  *retStr = nil;
    NSString         *paramPart, *paramName;
    BOOL              hasRange, useRounded;
    NSRange           fldRange;
    int               i, pix, aix, qMarksCnt = 0;
    id                fldObj, fldRangeObj, defaultObj = nil;

    NSString         *binaryOperator = @"OR";
    
    if (fldValuesToMatchOrNil && [fldValuesToMatchOrNil count] > 1)
        binaryOperator = @"AND";

    if (!fldNamesToMatchOrNil && fldValuesToMatchOrNil)  {  // id only
        retStr = (NSMutableString *)@"id = ?";   // Never gonna give you up, Never gonna let you down, Never gonna run around and desert you
        qMarksCnt = 1;
    }
    else  {
        if ([fldValuesToMatchOrNil count] == 1)
            defaultObj = [fldValuesToMatchOrNil objectAtIndex:0];
        retStr = [NSMutableString string];
        for (pix=i=0; fldDescCArray[i].fldName; i++)  {
            if ((fldDescCArray[i].fldFlags & kFldDescFlagSearchableKey) &&
                ([fldNamesToMatchOrNil containsObject:fldDescCArray[i].fldName]))  {
                
                aix    = [fldNamesToMatchOrNil indexOfObject:fldDescCArray[i].fldName];
                fldObj = defaultObj ? defaultObj : [fldValuesToMatchOrNil objectAtIndex:aix];
                
                useRounded = NO;
                hasRange = NO;
                if (fldRangesToMatchOrNil)  {
                    fldRangeObj = [fldRangesToMatchOrNil objectAtIndex:aix];
                    if (fldRangeObj != [NSNull null])  {
                        fldRange = [(NSValue *)fldRangeObj rangeValue];
                        if (fldRange.location != NSNotFound)
                            hasRange = YES;
                    }
                }
                
                if (fldDescCArray[i].maxLen && fldDescCArray[i].sqlType == SQLITE_FLOAT)
                    useRounded = YES;
                
                if (fldObj == [NSNull null])
                    paramPart = @"IS NULL";
                else  if (useRounded)  {
                    paramPart = [NSString stringWithFormat:@"= round(?,%d)", fldDescCArray[i].maxLen];
                    qMarksCnt++;
                }
                else  if (exactMatch)  {
                    paramPart = @"= ?";
                    qMarksCnt++;
                }
                else  {
                    paramPart = @"LIKE ?";
                    qMarksCnt++;
                }
                
                if (hasRange && (fldObj != [NSNull null]))
                    paramName = [NSString stringWithFormat:@"SUBSTR(%@,%d,%d)", fldDescCArray[i].fldName, fldRange.location, fldRange.length];
                else  if (useRounded)
                    paramName = [NSString stringWithFormat:@"round(%@,%d)", fldDescCArray[i].fldName, fldDescCArray[i].maxLen];
                else
                    paramName = fldDescCArray[i].fldName;
                
                if (!pix)
                    [retStr appendFormat:@"%@ %@", paramName, paramPart];
                else
                    [retStr appendFormat:@" %@ %@ %@", binaryOperator, paramName, paramPart];  // AND name = ?
                pix++;
            }
        }
    }
    
    if (boundCnt)
        *boundCnt = qMarksCnt;
    
    return (retStr);
}
#endif

#ifdef _NIJE_
// .........................................................................................................

// Not very usefull as it is now

static char  *id_SQLOneParamWhere (
 char  *buffStr, 
 char  *paramName, 
 short  isNull, 
 short  eqDirection  // -1, 0, 1
)
{
   char  *paramPart = "=";
   
   buffStr[0] = '\0';
   
   if (!paramName)
      paramName = "id";
   
   // or use kWhereFlagLessThan
   
   if (eqDirection < 0)
      paramPart = "<";
   else  if (eqDirection > 0)
      paramPart = ">";
   
   sprintf (buffStr, "%s %s ?", paramName, paramPart);
   
   return (buffStr);
}
#endif

// .........................................................................................................

static char  *id_SQLTablePreffix (SQLSelectParams *selParams)
{
   static char  retStr[32];
   // static char *pfxPtr = NULL;
   
   if (selParams->recPreffix)  {
      snprintf (retStr, 32, "%s.", selParams->recPreffix);
      return (retStr);
   }
   
   return ("");
}

// .........................................................................................................

static char  *id_SQLAscDescSufix (SQLSelectParams *selParams)
{
   if (selParams->whereFlags & (kWhereFlagLessThan | kWhereFlagLessOrEQ | kWhereFlagDescending))
      return (" DESC");
   else  if ((selParams->whereFlags & kWhereFlagGreaterThan) || (selParams->whereFlags & kWhereFlagGreatOrEQ))
      return (" ASC");

   return ("");
}

#pragma mark -

// .........................................................................................................

/*char  *id_Test_SQLCreationStringForTable (
 char               *tableName,
 SQLFldDescription  *fldDescCArray,
 char               *buffPtr,
 short               buffLen
)
{
   return (id_SQLCreationStringForTable (tableName, fldDescCArray, buffPtr, buffLen));
}*/

static char  *id_SQLCreationStringForTable (
 ID_CT_MYSQL  *ctSql,
 char         *buffPtr,
 short         buffLen
)
{
   short  i, keyNo, maxKeySegs, segLen, usedLen = 0;
   char   segName[64];

   SQLFldDescription  *fldDescCArray = ctSql->fldDescs;
   // RecFieldOffset     *rfoArray = ctSql->rfi->iRecFldOffsets;

   // "CREATE TABLE %s (id integer primary key, name TEXT UNIQUE, title TEXT, image BLOB, thumb BLOB, comment TEXT);", kTableName);
   
   snprintf (buffPtr, buffLen, "CREATE TABLE %s (id integer primary key auto_increment", ctSql->tableName);
   
   for (i=0; fldDescCArray[i].fldName && fldDescCArray[i].fldName[0]; i++)  {
      strNCatf (buffPtr, buffLen, ", %s %s", fldDescCArray[i].fldName, id_SQLTypeCreationString(&fldDescCArray[i]));
   }
   
   // Indexi
   
   if (ctSql->ifilPtr)  for (keyNo=1; keyNo<=id_SQLGetNumberOfKeys(ctSql); keyNo++)  {
      
      if (ct_DuplicateKey(ctSql->ifilPtr, keyNo))  continue;
      
      strNCatf (buffPtr, buffLen, ", UNIQUE index_%hd (", keyNo);
      
      maxKeySegs = id_SQLGetKeySegments (ctSql, keyNo);
      
      for (i=0; i<maxKeySegs; i++)  {
         
         if (!id_SQLKeySegmentName(ctSql, keyNo, i, segName, &segLen))            
            strNCatf (buffPtr, buffLen, "%s%s", !i ? "" : ",", segName);  // WHERE CONCAT(seg1,seg2
      }
      strNCatf (buffPtr, buffLen, ")");
   }
   else  {
      keyNo = 1;
      if ((fldDescCArray[1].ctType == kTchar) && !strcmp(fldDescCArray[1].fldName, kMYSQLFlatKeyName))  // Should be only 1
         strNCatf (buffPtr, buffLen, ", UNIQUE index_%hd (%s)", keyNo++, fldDescCArray[1].fldName);
   }
   
   // strNCatf (buffPtr, buffLen, ")");  // or strNCatf()
   strNCatf (buffPtr, buffLen, " ) DEFAULT CHARACTER SET cp1250 COLLATE cp1250_croatian_ci");  // or strNCatf()
   
#ifdef _SHOW_CREATION_STRING_
   con_printf ("---- SQLCreationStringForTable: %s\n%s\n", ctSql->tableName, buffPtr);
#endif
   
   id_SQLSetLastSqlString (buffPtr);
   
   return (buffPtr);
}

// .........................................................................................................

// This will be fun
// New column will not be in my record the last one, it goes somewhere in the middle most of the times
// So, first I check just number of columns, if it does not match, then I must compare present columns in the db
// with my record and add the new ones, as many as there are

// Vidi: https://stackoverflow.com/questions/42052469/c-api-mysql-add-columns-if-they-dont-exist-incorrect-syntax
// snprintf (buff, sizeof(buff), 
// "ALTER TABLE `%s`.`%s` ADD COLUMN IF NOT EXISTS `%s` `%s`", 
// database,
// table, 
// column_name, 
// column_type
// ); 

static char  *id_SQLColumnAppendingStringForTable (
 char               *tableName,
 SQLFldDescription  *oneFldDesc,
 char               *buffPtr,
 short               buffLen
)
{
   // short  usedLen = 0;
   // "ALTER TABLE %s ADD COLUMN aColumnName TEXT;", kTableName);
   
   snprintf (buffPtr, buffLen, "ALTER TABLE %s ADD COLUMN", tableName);

   strNCatf (buffPtr, buffLen, " %s %s", oneFldDesc->fldName, id_SQLTypeCreationString(oneFldDesc));

#ifdef _SHOW_COLL_APPENDING_STRING_
   con_printf ("---- SQLColumnAppendingStringForTable: %s\n%s\n", tableName, buffPtr);
#endif
   
   id_SQLSetLastSqlString (buffPtr);

   return (buffPtr);
}

// .........................................................................................................

static char  *id_SQLIndexAppendingStringForTable (
 ID_CT_MYSQL        *ctSql,
 SQLFldDescription  *oneFldDesc,  // not needed for ifilPtr types
 short               keyNo,
 char               *buffPtr,
 short               buffLen
)
{
   short  i, maxKeySegs;
   short  segLen, usedLen = 0;
   char   segName[64];

   // "ALTER TABLE %s ADD UNIQUE INDEX index_1 (%s,...);", kTableName, indexParts);
   
   snprintf (buffPtr, buffLen, "ALTER TABLE %s ADD UNIQUE INDEX index_%hd (", ctSql->tableName, keyNo);

   if (ctSql->ifilPtr)  {
      
      if (!ct_DuplicateKey(ctSql->ifilPtr, keyNo))  {
      
         maxKeySegs = id_SQLGetKeySegments (ctSql, keyNo);
      
         for (i=0; i<maxKeySegs; i++)  {
            if (!id_SQLKeySegmentName(ctSql, keyNo, i, segName, &segLen))
               strNCatf (buffPtr, buffLen, "%s%s", !i ? "" : ",", segName);  // WHERE CONCAT(seg1,seg2
         }
      }
      strNCatf (buffPtr, buffLen, ")");
   }
   else  {
      if ((oneFldDesc->ctType == kTchar) && !strcmp(oneFldDesc->fldName, kMYSQLFlatKeyName))  // Should be only 1
         strNCatf (buffPtr, buffLen, "%s", oneFldDesc->fldName);
      strNCatf (buffPtr, buffLen, ")");
   }

#ifdef _SHOW_COLL_APPENDING_STRING_
   con_printf ("---- SQLIndexAppendingStringForTable: %s\n%s\n", ctSql->tableName, buffPtr);
#endif
   
   id_SQLSetLastSqlString (buffPtr);

   return (buffPtr);
}

// .........................................................................................................

static char  *id_SQLDeleteIndexStringForTable (  // drop index
 ID_CT_MYSQL        *ctSql,
 short               keyNo,
 char               *buffPtr,
 short               buffLen
)
{
   // "DROP INDEX index_1 ON TABLE %s;", kTableName);
   
   snprintf (buffPtr, buffLen, "DROP INDEX index_%hd ON %s", keyNo, ctSql->tableName);

#ifdef _SHOW_COLL_APPENDING_STRING_
   con_printf ("---- id_SQLDeleteIndexStringForTable: %s\n%s\n", ctSql->tableName, buffPtr);
#endif
   
   id_SQLSetLastSqlString (buffPtr);

   return (buffPtr);
}

// .........................................................................................................

// No more than 1024/2 fields, see the code
// Maybe, return parms count
// Must use our ID_CT_MYSQL *ctSql and fldDescs and RecFieldInfo so I can have theKey and its components

static char  gGQMarksStr[kMYSQLStringBufferLen];

static char  *id_SQLInsertionStringForTable (
 ID_CT_MYSQL  *ctSql,
 char         *buffPtr,
 short         buffLen,
 short        *retBoundCnt
)
{
   short  i, retCnt = 0, usedLen = 0;
   
   SQLFldDescription  *fldDescCArray = ctSql->fldDescs;

   // @"INSERT INTO %s (name, title, image, thumb, comment) values (?,?,?,?,?)", kTableName];
   
   snprintf (buffPtr, buffLen, "INSERT INTO %s.%s (", ctSql->dbName, ctSql->tableName);
   snprintf (gGQMarksStr, 255+1, "VALUES (");

   for (i=0; fldDescCArray[i].fldName; i++)  {
      strNCatf (buffPtr, buffLen, "%s%s", i ? "," : "", fldDescCArray[i].fldName);
      strNCatf (gGQMarksStr, 1024, "%s%s",  i ? "," : "", "?");
   }
   
   retCnt = i;
   
   strNCatf (buffPtr, buffLen, ") %s)", gGQMarksStr);
   
   if (retBoundCnt)
      *retBoundCnt = retCnt;  // i starts at zero, so 3 means 3 items bound
#ifdef _SHOW_INSERTION_STRING_
   con_printf ("---- id_SQLInsertionStringForTable: %s\n%s\n", ctSql->tableName, buffPtr);
#endif
   
   id_SQLSetLastSqlString (buffPtr);

   return (buffPtr);
}

// .........................................................................................................

// So, ide niz sa svim poljima koji se updataju plus array sa nazivom kljuca, tj. WHERE klauzule
// Nemamo vise polja usporedbe dakle nego samo jedno, zato uvijek koristi unique key
// Maybe, return parms count
// NOPE!
// Must use our ID_CT_MYSQL *ctSql and fldDescs and RecFieldInfo so I can have theKey and its components

static char  *id_SQLUpdatingStringForTable (
 ID_CT_MYSQL  *ctSql,
 char         *buffPtr,
 short         buffLen,
 short        *retKeyNo,
 short        *retBoundCnt
)
{
   short  usedLen = 0;
   short  pix, i, retCnt1, retCnt2;
   short  whereFlags = kWhereFlagKeywordWhere | kWhereFlagEqual;

   SQLFldDescription  *fldDescCArray = ctSql->fldDescs;
   
   // @"UPDATE %s SET %s = ? WHERE name = ?", kTableName, thumbFlag ? "thumb" : "image"];
   
   snprintf (buffPtr, buffLen, "UPDATE %s.%s SET ", ctSql->dbName, ctSql->tableName);
   
   usedLen = strlen (buffPtr);

   for (pix=i=0; fldDescCArray[i].fldName; i++)  {
      strNCatf (buffPtr, buffLen, "%s%s=?", i ? "," : "", fldDescCArray[i].fldName);
      pix++;
   }
    
   retCnt1 = pix;
    
   usedLen = strlen (buffPtr);

   if (usedLen < buffLen)  {
      if (retKeyNo)
         *retKeyNo = 1;
      if (!ctSql->ifilPtr)
         whereFlags |= kWhereFlagRecordNameOnly;
      // strNCatf (buffPtr, buffLen, "WHERE %s=?", searchField);
      id_SQLWhereClauseString (ctSql, 1, 0/*maxSegLen*/, whereFlags, buffPtr+usedLen, buffLen - usedLen, &retCnt2, NULL);
      // retCnt2 = 1;
   }
   
   if (retBoundCnt)
      *retBoundCnt = retCnt1 + retCnt2;  // pix starts at zero, so 3 means 3 items bound
    
#ifdef _SHOW_UPDATING_STRING_
   con_printf ("---- id_SQLUpdatingStringForTable: %s\n%s\n", ctSql->tableName, buffPtr);
#endif

   id_SQLSetLastSqlString (buffPtr);

   return (buffPtr);
}

// .........................................................................................................

// if fldNamesToMatchOrNil is nil, include all searchable fields into where clause
// Must use our ID_CT_MYSQL *ctSql and fldDescs and RecFieldInfo so I can have theKey and its components
 
static char  *id_SQLDeletionStringForTable (
 ID_CT_MYSQL  *ctSql,
 char         *buffPtr,
 short         buffLen,
 short        *retKeyNo,
 short        *retBoundCnt     
)
{
   short  usedLen = 0, keyNo = 1;
   short  pix, i, retCnt;
   short  whereFlags = kWhereFlagKeywordWhere | kWhereFlagEqual;
   // @"DELETE FROM %s WHERE name = ?", kTableName]
   
   snprintf (buffPtr, buffLen, "DELETE FROM %s.%s", ctSql->dbName, ctSql->tableName);
   
   usedLen = strlen (buffPtr);
   
   if (usedLen < buffLen)  {
      if (retKeyNo)
         *retKeyNo = keyNo;
      if (!ctSql->ifilPtr)
         whereFlags |= kWhereFlagRecordNameOnly;
      // strNCatf (buffPtr, buffLen, "WHERE %s=?", searchField);
      id_SQLWhereClauseString (ctSql, keyNo, 0/*maxSegLen*/, whereFlags, buffPtr+usedLen, buffLen - usedLen, &retCnt, NULL);
      // retCnt = 1;
   }
   
   if (retBoundCnt)
      *retBoundCnt = retCnt;  // pix starts at zero, so 3 means 3 items bound
   
#ifdef _SHOW_DELETION_STRING_
   con_printf ("---- id_SQLDeletionStringForTable: %s\n%s\n", ctSql->tableName, buffPtr);
#endif

   id_SQLSetLastSqlString (buffPtr);

   return (buffPtr);
}

// .........................................................................................................

// Pass selParams only or fldNamesToMatch and fldDescCArray together

#ifdef _NIJE_
+ (NSString *)sqlJoinStringForTables:(NSArray *)multiTables
                usingFldDescriptions:(SQLFldDescription *)fldDescCArray
{
    NSString  *firstPrefix  = [NSString stringWithFormat:@"%@.", [multiTables objectAtIndex:0]];
    NSString  *secondPrefix = [NSString stringWithFormat:@"%@.", [multiTables objectAtIndex:1]];
    NSString  *firstFieldName = nil;
    NSString  *secondFieldName = nil;
    
    for (int i=0; fldDescCArray[i].fldName; i++)  {
        DebugLog (@"Elem: %@", fldDescCArray[i].fldName);
        
        if (fldDescCArray[i].fldFlags & kFldDescFlagJoinedByKey)  {
            if ([fldDescCArray[i].fldName hasPrefix:firstPrefix])
                firstFieldName = fldDescCArray[i].fldName;
            if ([fldDescCArray[i].fldName hasPrefix:secondPrefix])
                secondFieldName = fldDescCArray[i].fldName;
        }
    }
    
    if (!firstFieldName)
        firstFieldName = [NSString stringWithFormat:@"%@%@", firstPrefix, @"id"];
    if (!secondFieldName)
        secondFieldName = [NSString stringWithFormat:@"%@%@", secondPrefix, @"id"];
    
    // Building: photo join tag on photo.id = tag.photo_id
    
    NSString  *retStr = [NSString stringWithFormat:@"%@ LEFT JOIN %@ ON %@ = %@",
                         [multiTables objectAtIndex:0], [multiTables objectAtIndex:1], firstFieldName, secondFieldName];
    
    return (retStr);
}
#endif

// .........................................................................................................

// Need keyNo -> tak da znam koji vrag ide u CONCAT
// CONCAT if keyNo, otherwise nope

// short  gGZajeb = FALSE;

static char  *id_SQLSelectionStringForTable (
 ID_CT_MYSQL      *ctSql,
 SQLSelectParams  *selParams,
 char             *buffPtr,
 short             buffLen,
 short            *retResultCnt,  
 short            *retBoundCnt
)
{
   // [... @"SELECT %s FROM %s WHERE name = ?", "image", kTableName];
   // But in ObjC version there were also "SELECT DISTINCT %@ FROM %@ WHERE ";
   // "SELECT DISTINCT SUBSTR(%@,%d,%d) FROM %@"
   // "SELECT DISTINCT %@ FROM %@" ...  ORDER BY %@
   // And SELECT DISTINCT local_path from photo join tag on photo.id = tag.photo_id where param1 = ? or param2 = ? ...
   
   short  exactMatch = YES;
   short  usedLen = 0;
   short  i, pix=0, retCnt1, retCnt2 = 0, retCnt3 = 0;
   
   SQLFldDescription  *fldDescCArray = ctSql->fldDescs;
   
   snprintf (buffPtr, buffLen, "SELECT %s", (selParams && selParams->distinctFlag) ? "DISTINCT " : "");  // was onlyFieldOrNil ? onlyFieldOrNil : "*"
   
   if (selParams->selectIdOnly)  {
      strNCatf (buffPtr, buffLen, "%s", "id");
      pix = 1;
   }
   else  {
      for (pix=i=0; fldDescCArray[i].fldName; i++)  {
         strNCatf (buffPtr, buffLen, "%s%s%s", i ? "," : "", id_SQLTablePreffix(selParams), fldDescCArray[i].fldName);
         pix++;
      }
      strNCatf (buffPtr, buffLen, ",%s%s", id_SQLTablePreffix(selParams), "id");
   }   
   retCnt1 = pix;
   
   strNCatf (buffPtr, buffLen, " FROM %s.%s", ctSql->dbName, ctSql->tableName);   // concat table name
   
   if (selParams->recPreffix)
      strNCatf (buffPtr, buffLen, " %s", selParams->recPreffix);   // concat rec name if join

   usedLen = strlen (buffPtr);

   if (!selParams)  {                                                                 // A) ...
      // keyNo & maxSegs should be zero here as they are parts of selParams
      id_SQLWhereClauseString (ctSql, 0, 0, kWhereFlagKeywordWhere | kWhereFlagEqual, buffPtr+usedLen, buffLen - usedLen, &retCnt2, NULL);
   }
   else  if (!selParams->distinctFlag)  {                                             // B) WHERE
      if (selParams->whereClause)  {
         strNCatf (buffPtr, buffLen, " WHERE %s", selParams->whereClause);
      }
      else  if (!(selParams->whereFlags & kWhereFlagNoWhere))  {
         
         // FRSSET, LSTSET: ONLY ctSql->setSize NOT ZERO
         // NXTSET, PRVSET: ctSql->setSize AND selParams->setKeySegs NOT ZERO
         
         short  tmpWhereFlags = selParams->whereFlags;
         
         if (ctSql->setSize && selParams->setKeySegs)
            tmpWhereFlags &= ~(kWhereFlagLike);  // NXTSET or PRVSET will have like in second part

         id_SQLWhereClauseString (ctSql, selParams->keyNo, selParams->whereKeySegs, kWhereFlagKeywordWhere | tmpWhereFlags, buffPtr+usedLen, buffLen - usedLen, &retCnt2, NULL);
         
         if (ctSql->setSize && selParams->setKeySegs)  {
            // Used for NXTSET & PRVSET after FRSSET & LSTSET
            tmpWhereFlags = selParams->whereFlags & ~(kWhereFlagLessThan | kWhereFlagGreaterThan);
            
            strNCatf (buffPtr, buffLen, " AND ");

            usedLen = strlen (buffPtr);
            id_SQLWhereClauseString (ctSql, selParams->keyNo, selParams->setKeySegs, tmpWhereFlags, buffPtr+usedLen, buffLen - usedLen, &retCnt3, NULL);
         }
      }
      
      if (selParams->orderByClause)
         strNCatf (buffPtr, buffLen, " ORDER BY %s", selParams->orderByClause);
      else  if (selParams->orderByKeySegs)  {
         strNCatf (buffPtr, buffLen, " ORDER BY ");
         usedLen = strlen (buffPtr);
         if (selParams->whereFlags & kWhereFlagConcat)  {
            id_SQLConcatSegmentsString (ctSql, selParams->keyNo, selParams->orderByKeySegs, buffPtr+usedLen, buffLen - usedLen, TRUE);  // Names, not qmarks
            strNCatf (buffPtr, buffLen, "%s", id_SQLAscDescSufix(selParams));
         }
         else
            id_SQLKeySegmentsString (ctSql, selParams, selParams->orderByKeySegs, buffPtr+usedLen, buffLen - usedLen, TRUE);
         
         /*if (selParams->whereFlags & (kWhereFlagLessThan | kWhereFlagLessOrEQ | kWhereFlagDescending))
            strNCatf (buffPtr, buffLen, " %s", "DESC");
         else  if ((selParams->whereFlags & kWhereFlagGreaterThan) || (selParams->whereFlags & kWhereFlagGreatOrEQ))
            strNCatf (buffPtr, buffLen, " %s", "ASC");*/
      }
      
      if (selParams->limitCnt)
         strNCatf (buffPtr, buffLen, " LIMIT %hd", selParams->limitCnt);
      if (selParams->offsetCnt)
         strNCatf (buffPtr, buffLen, " OFFSET %hd", selParams->offsetCnt);
   }
   else  if (selParams->joinSqlStr)  {                                            // C) JOIN
      strNCatf (buffPtr, buffLen, " %s", selParams->joinSqlStr);
      
      if (selParams->orderByKeySegs)  {
         strNCatf (buffPtr, buffLen, " ORDER BY ");
         usedLen = strlen (buffPtr);
         
         if (selParams->whereFlags & kWhereFlagConcat)
            id_SQLConcatSegmentsString (ctSql, selParams->keyNo, selParams->orderByKeySegs, buffPtr+usedLen, buffLen - usedLen, TRUE);  // Names, not qmarks
         else
            id_SQLKeySegmentsString (ctSql, selParams, selParams->orderByKeySegs, buffPtr+usedLen, buffLen - usedLen, TRUE);
         
         if (selParams->whereFlags & (kWhereFlagLessThan | kWhereFlagLessOrEQ | kWhereFlagDescending))
            strNCatf (buffPtr, buffLen, " %s", "DESC");
         else  if ((selParams->whereFlags & kWhereFlagGreaterThan) || (selParams->whereFlags & kWhereFlagGreatOrEQ))
            strNCatf (buffPtr, buffLen, " %s", "ASC");
      }
      
      if (selParams->limitCnt)
         strNCatf (buffPtr, buffLen, " LIMIT %hd", selParams->limitCnt);
      if (selParams->offsetCnt)
         strNCatf (buffPtr, buffLen, " OFFSET %hd", selParams->offsetCnt);
   }
   
   if (retResultCnt)
      *retResultCnt = retCnt1;
   if (retBoundCnt)
      *retBoundCnt = retCnt2 + retCnt3;

#ifdef _SHOW_SELECTTION_STRING_
   con_printf ("---- id_SQLSelectionStringForTable: %s\n%s\n", ctSql->tableName, buffPtr);
#endif
   
#ifdef _LOG_SELECTION_STRING_
   id_LogFileFormat ("[%s] %s", id_form_time(id_sys_time(), _HH_MI_SS), buffPtr);
#endif
   
   id_SQLSetLastSqlString (buffPtr);

   return (buffPtr);
}

#pragma mark -

// .................................................... id_SQLWhereClauseString ............................

// needExplicitCollation - if we have mixed stuff
// kad trebamo id nekako i to napomenit...

static int  id_SQLWhereClauseString (ID_CT_MYSQL *ctSql, short keyNo, short reqKeySegs, short flags, char *whereString, short maxLen, short *usedKeySegs, short *usedKeyLength)
{
   short            i, segLen, usedSegs = 0, usedLen = 0, needExplicitCollation = 0;
   short            maxKeySegs, padSize;
   // char            *concatStr = (flags & kWhereFlagConcat) ? "CONCAT(" : "";
   char            *operatorStr = "=";
   char            *collationStr = " COLLATE cp1250_croatian_ci ";  // Need spaces around - cp1250_bin, cp1250_croatian_ci
   RecFieldOffset  *rfoArray = ctSql->rfi->iRecFldOffsets;
   
   *whereString = '\0';  // In case it isn't already
   
   if (flags & kWhereFlagKeywordWhere)
      strNCatf (whereString, maxLen, " WHERE ");
          
   if (flags & kWhereFlagLike)
      operatorStr = " LIKE ";
   else  if (flags & kWhereFlagLessThan)
      operatorStr = "<";
   else  if (flags & kWhereFlagLessOrEQ)
      operatorStr = "<=";
   else  if (flags & kWhereFlagGreaterThan)
      operatorStr = ">";
   else  if (flags & kWhereFlagGreatOrEQ)
      operatorStr = ">=";
   // else  if (!ctSql->setSize)  // for now, one day come with kWhereFlagNoConcat
   //    flags &= ~kWhereFlagConcat;
   
   if (flags & kWhereFlagWhereIdOnly)  {
      strNCatf (whereString, maxLen, "%s%s?", "id", operatorStr);  // WHERE fld=?
      usedSegs = 1;
   }
   else  if (flags & kWhereFlagRecordNameOnly)  {
      strNCatf (whereString, maxLen, "%s%s?", kMYSQLFlatKeyName, operatorStr);  // WHERE fld=?
      usedSegs = 1;
   }
   else  {
      maxKeySegs = id_SQLGetKeySegments (ctSql, keyNo);
      // hasMixedTypes  = ct_MixedSegmentTypesKey (ctSql->ifilPtr, keyNo, 0);
   
      // reqKeySegs - what we need
      // maxKeySegs - max
      
      if (reqKeySegs == (maxKeySegs+1))  // Append id at the end
         maxKeySegs++;
   
      for (i=0; (!reqKeySegs || (i < reqKeySegs)) && (i<maxKeySegs); i++)  {
         char     segName[64];
         Boolean  rPadFlag = FALSE;
         
         if (!id_SQLKeySegmentName(ctSql, keyNo, i, segName, &segLen))  {
            
            if ((reqKeySegs > 1) && (flags & kWhereFlagConcat))  {
               
               // Question marks added down below
               
               padSize = id_SQLKeySegmentPaddingSize (ctSql, keyNo, i, &rPadFlag);
               
               // SELECT f_tip, f_br, f_brdok  FROM IzlazniRacuniA ORDER BY CONCAT(LPAD(CAST(CAST(f_tip AS UNSIGNED) AS CHAR),3,' '),LPAD(CAST(CAST(f_br AS UNSIGNED) AS CHAR),6,' '));
               
               if (padSize)  {
                  if (rPadFlag)  {
                     strNCatf (whereString, maxLen, "%sRPAD(%s,%hd,' ')", !i ? "CONCAT(" : ",", segName, padSize);  // WHERE CONCAT(seg1,seg2
                     needExplicitCollation |= 2;
                  }
                  else {
                     strNCatf (whereString, maxLen, "%sLPAD(CAST(CAST(%s AS UNSIGNED) AS CHAR),%hd,' ')", !i ? "CONCAT(" : ",", segName, padSize);  // WHERE CONCAT(seg1,seg2
                     needExplicitCollation |= 1;
                  }
               }
               else  {
                  strNCatf (whereString, maxLen, "%s%s", !i ? "CONCAT(" : ",", segName);  // WHERE CONCAT(seg1,seg2
                  needExplicitCollation |= 2;
               }
            }
            else  if (!strcmp(segName, "id"))   // where keyFld>='a' and id>5
               strNCatf (whereString, maxLen, "%s%s%c?", !i ? "" : " AND ", segName, operatorStr[0]);  // WHERE seg1>=? AND id>?  - Changed on 18.03.2021, had ", " before
            else
               strNCatf (whereString, maxLen, "%s%s%s?", !i ? "" : " AND ", segName, operatorStr);  // WHERE seg1=? AND seg2=?  - Changed on 18.03.2021, had ", " before
            usedLen += segLen;
         }
      }
      
      usedSegs = i;

      if (*whereString && (reqKeySegs > 1) && (flags & kWhereFlagConcat))  {
         short  usedLen;
         
         strNCatf (whereString, maxLen, ")%s%s ", needExplicitCollation==3 ? collationStr : "", operatorStr);
         
         usedLen = strlen (whereString);
         
         id_SQLConcatSegmentsString (ctSql, keyNo, reqKeySegs, whereString+usedLen, maxLen - usedLen, FALSE);  // Qmarks, not names
         
         // if (needExplicitCollation == 3)
         //    strNCatf (whereString, maxLen, "%s", collationStr);  - Handled inside
      }
   }   

   if (usedKeySegs)
      *usedKeySegs = usedSegs;
   
   if (usedKeyLength)
      *usedKeyLength = strlen (whereString);
   
   return (usedSegs ? 0 : -1);
}

// .........................................................................................................

static int  id_SQLConcatSegmentsString (ID_CT_MYSQL *ctSql, short keyNo, short reqKeySegs, char *concatString, short maxLen, short namesFlag)  // Instead of questionmarks
{
   short            i, segLen, padSize, needExplicitCollation = 0;
   short            maxKeySegs = id_SQLGetKeySegments (ctSql, keyNo);
   Boolean          rPadFlag = FALSE;
   char            *collationStr = " COLLATE cp1250_croatian_ci ";  // Need spaces around - cp1250_bin, cp1250_croatian_ci
   // char            *concatStr = (flags & kWhereFlagConcat) ? "CONCAT(" : "";
   RecFieldOffset  *rfoArray = ctSql->rfi->iRecFldOffsets;
   
   *concatString = '\0';  // In case it isn't already
   
   // reqKeySegs - what we need
   // maxKeySegs - max
   
   if (reqKeySegs == (maxKeySegs+1))  // Append id at the end
      maxKeySegs++;

   if (!namesFlag)  {
      for (i=0; (!reqKeySegs || (i < reqKeySegs)) && (i<maxKeySegs); i++)  {
         if (reqKeySegs > 1)  {
            padSize = id_SQLKeySegmentPaddingSize (ctSql, keyNo, i, &rPadFlag);

            if (padSize)  {
               if (rPadFlag)  {
                  strNCatf (concatString, maxLen, "%sRPAD(?,%hd,' ')", !i ? "CONCAT(" : ",", padSize);
                  needExplicitCollation |= 2;
               }
               else {
                  strNCatf (concatString, maxLen, "%sLPAD(CAST(CAST(? AS UNSIGNED) AS CHAR),%hd,' ')", !i ? "CONCAT(" : ",", padSize);
                  needExplicitCollation |= 1;
               }
            }
            else  {
               strNCatf (concatString, maxLen, "%s?", !i ? "CONCAT(" : ",");  // CONCAT(?,?
               needExplicitCollation |= 2;
            }
         }
         else
            strNCatf (concatString, maxLen, "%s", "?");
      }
   }
   else  {
      for (i=0; (!reqKeySegs || (i < reqKeySegs)) && (i<maxKeySegs); i++)  {
         char  segName[64];
         
         if (!id_SQLKeySegmentName(ctSql, keyNo, i, segName, &segLen))  {
            if (reqKeySegs > 1)  {
               padSize = id_SQLKeySegmentPaddingSize (ctSql, keyNo, i, &rPadFlag);
               
               if (padSize)  {
                  if (rPadFlag)  {
                     strNCatf (concatString, maxLen, "%sRPAD(%s,%hd,' ')", !i ? "CONCAT(" : ",", segName, padSize);
                     needExplicitCollation |= 2;
                  }
                  else {
                     strNCatf (concatString, maxLen, "%sLPAD(CAST(CAST(%s AS UNSIGNED) AS CHAR),%hd,' ')", !i ? "CONCAT(" : ",", segName, padSize);
                     needExplicitCollation |= 1;
                  }
               }
               else  {
                  strNCatf (concatString, maxLen, "%s%s", !i ? "CONCAT(" : ",", segName);  // CONCAT(?,?
                  needExplicitCollation |= 2;
               }
            }
            else
               strNCatf (concatString, maxLen, "%s", segName);
         }
      }
   }
   
   if ((reqKeySegs > 1) && *concatString)
      strNCatf (concatString, maxLen, ")");
   
   if (needExplicitCollation == 3)
      strNCatf (concatString, maxLen, "%s", collationStr);

   return (i ? 0 : -1);
}

// .........................................................................................................

static int  id_SQLKeySegmentsString (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, short reqKeySegs, char *theString, short maxLen, short namesFlag)  // Instead of questionmarks
{
   short  i, segLen, padSize;
   short  keyNo = selParams->keyNo;
   short  maxKeySegs = id_SQLGetKeySegments (ctSql, keyNo);
   // char            *concatStr = (flags & kWhereFlagConcat) ? "CONCAT(" : "";
   RecFieldOffset  *rfoArray = ctSql->rfi->iRecFldOffsets;
   
   *theString = '\0';  // In case it isn't already
   
   // reqKeySegs - what we need
   // maxKeySegs - max
   
   if (reqKeySegs == (maxKeySegs+1))  // Append id at the end
      maxKeySegs++;
   
   if (!namesFlag)  {
      for (i=0; (!reqKeySegs || (i < reqKeySegs)) && (i<maxKeySegs); i++)  {
         strNCatf (theString, maxLen, "%s?", !i ? "" : ",");  // ?,?,...
      }
   }
   else  {
      for (i=0; (!reqKeySegs || (i < reqKeySegs)) && (i<maxKeySegs); i++)  {
         char  segName[64];
         
         if (!id_SQLKeySegmentName(ctSql, keyNo, i, segName, &segLen))
            strNCatf (theString, maxLen, "%s%s%s%s", !i ? "" : ",", id_SQLTablePreffix(selParams), segName, id_SQLAscDescSufix(selParams));  // segName or pfx.segname
         else
            con_printf ("Unknown segment - keyNo: %hd, seg: ", keyNo, i+1);
      }
   }
   
   return (i ? 0 : -1);
}

#pragma mark -

/* .............................................. id_SQLAllocateJoinParamsBuffers ... */

static void  id_SQLAllocateJoinParamsBuffers (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, char *preffixStr, char *joinSqlStr)
{
   short  preffixLen = strlen (preffixStr) + 1;
   short  stringLen  = strlen (joinSqlStr) + 1;
   
   id_SQLDisposeJoinParamsBuffers (ctSql);
   
   selParams->whereKeySegs = 0;
   selParams->setKeySegs = 0;
   
   selParams->distinctFlag = TRUE;                // For NXTSET() it will be set in id_SQLInitSetSelectParams()
   
   ctSql->joinParams.savedOffset = selParams->offsetCnt = 0L;
   ctSql->joinParams.savedLimit = selParams->limitCnt = kMYSQLBufferedRows;
   
   ctSql->joinParams.jpRecPreffix = id_malloc_clear_or_exit (preffixLen);
   ctSql->joinParams.jpJoinString = id_malloc_clear_or_exit (stringLen);
   
   strNCpy (selParams->recPreffix = ctSql->joinParams.jpRecPreffix, preffixStr, preffixLen-1);
   strNCpy (selParams->joinSqlStr = ctSql->joinParams.jpJoinString, joinSqlStr, stringLen-1);
}

/* .............................................. id_SQLDisposeJoinParamsBuffers .... */

static void  id_SQLDisposeJoinParamsBuffers (ID_CT_MYSQL *ctSql)
{
   id_DisposeCharAddress (&ctSql->joinParams.jpRecPreffix);
   id_DisposeCharAddress (&ctSql->joinParams.jpJoinString);
   
   ctSql->joinParams.savedOffset = ctSql->joinParams.savedLimit = 0L;
}

#pragma mark Binding

// .........................................................................................................

// kBindFlagParams & kBindFlagWhereClause share prmBinds and prmCount

static ID_BIND_INFO  *id_SQLBindSelector (
 ID_CT_MYSQL  *ctSql,
 short         paramBindFlag,  // 1=prm, -1=chk, 0=res
 short         idx,
 void        **retBuffPtr,
 short       **retCounterPtr
)
{
   void          *buffPtr  = NULL;
   short         *counter = NULL;
   ID_BIND_INFO  *bindInfo = NULL;
   
   // Put some checks here, use recSqlFldCount etc
      
   switch (paramBindFlag)  {
      case  kBindFlagParams:
         bindInfo = &ctSql->paramBindInfos[idx];
         buffPtr  = &ctSql->paramBuffers[idx];
         counter  = &ctSql->paramCount;
         // ctSql->parChkBinds[ctSql->paramCount] = ctSql->paramBindInfos[idx].biBind;  -> already connected
         ctSql->paramCount++;
         break;
      case  kBindFlagWhereClause:
         bindInfo = &ctSql->checkBindInfos[idx];
         buffPtr  = &ctSql->checkBuffers[idx];
         counter  = &ctSql->paramCount;
         // ctSql->parChkBinds[ctSql->paramCount] = ctSql->checkBindInfos[idx].biBind;  // Careful here, we share prmBinds and prmCount
         ctSql->paramCount++;
         ctSql->checkCount++;
         break;
      case  kBindFlagResults:
      default:
         bindInfo = &ctSql->resultBindInfos[idx];
         buffPtr  = &ctSql->resultBuffers[idx];
         counter  = &ctSql->resultCount;
         // ctSql->resultBinds[ctSql->resultCount] = ctSql->resultBindInfos[idx].biBind;  -> already connected
         ctSql->resultCount++;
         break;
   }
   
   if (retBuffPtr)
      *retBuffPtr = buffPtr;
   if (retCounterPtr)
      *retCounterPtr = counter;
   
   return (bindInfo);
}

// .........................................................................................................

static int  id_SQLBindStatement (ID_CT_MYSQL *ctSql, short *retPrmCount, short *retResCount)
{
   // short  pix, cix, rix;
   short   returnCode = 0;
   
   ctSql->errNo = 0;

   if (ctSql->statementPhase != kMYSQLStmtPhaseDefault)  {
      if (!returnCode && ctSql->paramCount)  {
         // id_SQLSizeBoundTextLenghts (ctSql);
         if (ctSql->checkCount == ctSql->paramCount)
            returnCode = id_mysql_stmt_bind_param (ctSql->sqlStatement, &ctSql->parChkBinds[ctSql->recSqlFldCount]);  // no input params, where clause only, +1 for id
         else
            returnCode = id_mysql_stmt_bind_param (ctSql->sqlStatement, &ctSql->parChkBinds[0]);
         
         if (!returnCode && retPrmCount)
            *retPrmCount = ctSql->paramCount;
         else  {
            con_printf ("MySQL: %s\n", id_mysql_stmt_error(ctSql->sqlStatement));
            con_printf ("???\n%s\n???\n", sqlHandler->lastSqlString);
         }

         ctSql->paramCount = ctSql->checkCount = 0;
      }

      if (!returnCode && ctSql->resultCount)  {
         returnCode = id_mysql_stmt_bind_result (ctSql->sqlStatement, &ctSql->resultBinds[0]);
         if (!returnCode && retResCount)
            *retResCount = ctSql->resultCount;
         ctSql->resultCount = 0;
      }
      
      if (!returnCode)
         ctSql->statementPhase = kMYSQLStmtPhaseBind;
      else  {
         ctSql->errNo = id_mysql_errno (ctSql->dbHandle);
         con_printf ("id_SQLBindStatement - err: %s\n", id_mysql_stmt_error(ctSql->sqlStatement));
      }
   }
   
   return (returnCode);
}

// .........................................................................................................

static long  id_SQLExecuteStatement (ID_CT_MYSQL *ctSql, short needtToStoreResult, short needToFetchResult)
{
   long   returnCode = 0;
   
   ctSql->errNo = 0;
   
   if (ctSql->statementPhase != kMYSQLStmtPhaseDefault)  {
      if (!returnCode)
         returnCode = id_mysql_stmt_execute (ctSql->sqlStatement);  // 1062 duplicate - maybe - mysql_errno()
      
      if (returnCode)  {
         ctSql->errNo = id_mysql_errno (ctSql->dbHandle);
         con_printf ("SQLExecuteStatement - err: %s\n", id_mysql_stmt_error(ctSql->sqlStatement));
      }
      else
         sqlHandler->dbSuccessCallsCounter++;
      sqlHandler->dbAllCallsCounter++;
      
      if (!returnCode && needtToStoreResult)
         returnCode = id_mysql_stmt_store_result (ctSql->sqlStatement);  // Move everything to the client
      
      if (!returnCode && needToFetchResult)
         returnCode = id_mysql_stmt_fetch (ctSql->sqlStatement);
   }
   
   return (returnCode);
}

/* .............................................. id_SQLAllocateExtraBindBuffer ..... */

static void  id_SQLAllocateExtraBindBuffer (ID_CT_MYSQL *ctSql, long buffSize)
{
   id_DisposeCharAddress (&ctSql->extraBindBuffer);
   
   ctSql->extraBindBuffer = id_malloc_clear_or_exit (buffSize);
}

/* .............................................. id_SQLDisposeExtraBindBuffer ...... */

static void  id_SQLDisposeExtraBindBuffer (ID_CT_MYSQL *ctSql)
{
   id_DisposeCharAddress (&ctSql->extraBindBuffer);
   
   id_SQLResetBoundUnsignedCounts (ctSql);
}

// .........................................................................................................

// Here we take an unsigned integer, sprintf it to a buffer and then bind that buffer

int  id_SQLBindUnsignedObject (
 ID_CT_MYSQL        *ctSql,
 short               paramBindFlag,
 char               *recPtr,
//  char              **fldBuffPtr,
 ID_BIND_INFO       *bindInfo,
 SQLFldDescription  *fldDesc
)
{
   long   objSize;
   char  *fldPtr = recPtr + fldDesc->inRecOffset;
   
   SQLOneUnsigned  *oneUnsig = NULL;
   
   if (!recPtr)  {
      con_printf ("id_SQLBindUnsignedObject: no recPtr!");
      return (-1);
   }
   
   if (paramBindFlag == kBindFlagParams)  {
      oneUnsig = ctSql->unSig.uParams + ctSql->unSig.uParamsCount;
      if (ctSql->unSig.uParamsCount >= ctSql->unSig.unsignedFldCount)
         con_printf ("ctSql->unSig.uParamsCount overflow!");
   }
   else  if (paramBindFlag == kBindFlagWhereClause)  {
      oneUnsig = ctSql->unSig.uChecks + ctSql->unSig.uChecksCount;
      if (ctSql->unSig.uChecksCount >= (ctSql->unSig.unsignedFldCount * 2))
         con_printf ("ctSql->unSig.uChecksCount overflow!");
   }
   else  if (paramBindFlag == kBindFlagResults)  {
      oneUnsig = ctSql->unSig.uResults + ctSql->unSig.uResultsCount;
      if (ctSql->unSig.uResultsCount >= ctSql->unSig.unsignedFldCount)
         con_printf ("ctSql->unSig.uResultsCount overflow!");
   }
   else
      return (-1);
 
   oneUnsig->ctType = fldDesc->ctType;
   oneUnsig->sqlType = fldDesc->sqlType;
   oneUnsig->inRecOffset = fldDesc->inRecOffset;
   
   if (paramBindFlag != kBindFlagResults)  {
      switch (oneUnsig->ctType)  {
         case  kTuint:    snprintf (oneUnsig->uBuffer, 16, "%u", *((unsigned int *)(fldPtr)));  break;
         case  kTushort:  snprintf (oneUnsig->uBuffer, 16, "%hu", *((unsigned short *)(fldPtr)));  break;
         case  kTulong:   snprintf (oneUnsig->uBuffer, 16, "%lu", *((unsigned long *)(fldPtr)));  break;
         case  kTuchar:   snprintf (oneUnsig->uBuffer, 16, "%hu", (unsigned short)(*((unsigned char *)(fldPtr))));  break;
         default:  return (-1);
      }
   }
   else
      id_SetBlockToZeros (oneUnsig->uBuffer, 16);
   
   objSize = (paramBindFlag != kBindFlagResults) ? strlen(oneUnsig->uBuffer) : 0;

   if (id_SQLBindText(ctSql, bindInfo, oneUnsig->uBuffer, objSize, 15))  {
      if (paramBindFlag == kBindFlagParams)
         ctSql->unSig.uParamsCount++;
      else  if (paramBindFlag == kBindFlagWhereClause)
         ctSql->unSig.uChecksCount++;
      else  if (paramBindFlag == kBindFlagResults)
         ctSql->unSig.uResultsCount++;
      else
         return (-1);
   }
   else
      return (-1);
   
   return (0);
}

// .........................................................................................................

// Here we copy back into our recPtr from binding buffers for all unsigned integers

int  id_SQLCopyBoundUnsignedResults (
 ID_CT_MYSQL  *ctSql,
 char         *recPtr
)
{
   short  i;
   char  *fldPtr = NULL, *tempPtr = NULL;
   
   SQLOneUnsigned  *oneUnsig = NULL;
   
   for (i=0; i<ctSql->unSig.uResultsCount; i++)  {
      
      oneUnsig = ctSql->unSig.uResults + i;
      
      fldPtr = recPtr + oneUnsig->inRecOffset;
   
      switch (oneUnsig->ctType)  {
         case  kTuint:    *((unsigned int *)fldPtr) = (unsigned int)strtoul (oneUnsig->uBuffer, &tempPtr, 0);  break;
         case  kTushort:  *((unsigned short *)fldPtr) = (unsigned short)strtol (oneUnsig->uBuffer, &tempPtr, 0);  break;
         case  kTulong:   *((unsigned long *)fldPtr) = strtoul (oneUnsig->uBuffer, &tempPtr, 0);  break;
         case  kTuchar:   *((unsigned char *)fldPtr) = (unsigned char)strtol (oneUnsig->uBuffer, &tempPtr, 0);  break;
         default:  return (-1);
      }
   }
   
   return (0);
}

// .........................................................................................................

void  id_SQLResetBoundUnsignedCounts (ID_CT_MYSQL *ctSql)
{
   ctSql->unSig.uResultsCount = ctSql->unSig.uChecksCount = ctSql->unSig.uParamsCount = 0;
}

// .........................................................................................................

// HERE WE CAN PRINT RECORD FIELDS -> id_BufferStrAtOffset ... etc

#define  _DO_DISPLAY_RECORD_no  // If you turn it on it will crash, fix NULL checks inside id_BufferLongAtOffset() etc
                                // so set recPtr in calls to actual ptr and offset to 0

int  id_SQLBindSingleCTreeObject (
 ID_CT_MYSQL        *ctSql,
 short               paramBindFlag,
 char               *recPtr,
 char              **fldBuffPtr,
 ID_BIND_INFO       *bindInfo,
 SQLFldDescription  *fldDesc,
 // MYSQL_STMT         *sqlStatement,  // NO NEED
 // short               bindIndex,  // one based
 char               *retFldValue  // may be NULL
)
{
   int    returnCode = 0;
   long   objSize;
   char  *fldPtr = recPtr ? recPtr + fldDesc->inRecOffset : (char *)&ctSql->idToBind;  // id if !recPtr
   
#ifdef  _DO_DISPLAY_RECORD_
   char   tmpStr[256];
#endif
   
   if (!ctSql->ifilPtr)  {
      if (fldDesc->ctType == kTchar)  {// For now we should trust this without some extra flags present in fCType like kTFlagSqlKeyBuffer
         if (paramBindFlag == kBindFlagParams)
            fldPtr = ctSql->rfi->iRecFldOffsets[1].fCName;
            // snprintf (ctSql->extraBindBuffer, fldDesc->maxLen, "%s", fldPtr);
         else  {
            id_SQLAllocateExtraBindBuffer (ctSql, fldDesc->maxLen+4);
            // con_printf ("id_SQLBindSingleCTreeObject - no ifilsPtr, Why am I here?");
            fldPtr = ctSql->extraBindBuffer;
         }
      }
   }
   
   if (fldBuffPtr)
      *fldBuffPtr = fldPtr;
#ifdef  _DO_DISPLAY_RECORD_
   tmpStr[0] = '\0';
#endif  //  _DO_DISPLAY_RECORD_
   if (retFldValue)
      retFldValue[0] = '\0';
   
   // 1. BLOB
   if (fldDesc->sqlType == MYSQL_TYPE_BLOB)  {
      // objSize = (paramBindFlag != kBindFlagResults) ? fldDesc->maxLen : 0;
      if (paramBindFlag == kBindFlagResults)  {
         objSize = 0;
         if (fldDesc->fldFlags & kFldDescFlagVariableLength)
            fldPtr = NULL;
      }
      else  {
         objSize = fldDesc->maxLen;
         if (fldDesc->fldFlags & kFldDescFlagVariableLength)  {
            fldPtr = ctSql->varLenBuffPtr;
            objSize = ctSql->varLenBuffSize;
         }
      }
      if (fldBuffPtr)
         *fldBuffPtr = fldPtr;  // Set changed value
      if (!id_SQLBindBlob(ctSql, bindInfo, (char *)fldPtr, objSize, fldDesc->maxLen))
         returnCode = -1;
      if (retFldValue)
         strNCpy (retFldValue, "BLOB", 63);
   }
   // 2. STRING
   else  if (fldDesc->sqlType == MYSQL_TYPE_STRING)  {
      objSize = (paramBindFlag != kBindFlagResults) ? strlen(fldPtr) : 0;
      if (objSize > fldDesc->maxLen)
         objSize = fldDesc->maxLen;
#ifdef  _DO_DISPLAY_RECORD_
      id_BufferStrAtOffset (NULL, tmpStr, recPtr, fldDesc->inRecOffset, objSize+1);
#endif  //  _DO_DISPLAY_RECORD_
      if (!id_SQLBindText(ctSql, bindInfo, (char *)fldPtr, objSize, fldDesc->maxLen))
         returnCode = -1;
      if (retFldValue)
         strNCpy (retFldValue, fldPtr, 63);
   }
   // 3. LONG
   else  if (fldDesc->sqlType == MYSQL_TYPE_LONG)  {
#ifdef  _DO_DISPLAY_RECORD_
      id_BufferLongAtOffset(NULL, tmpStr, recPtr, fldDesc->inRecOffset);
#endif  //  _DO_DISPLAY_RECORD_
      if (fldDesc->ctType == kTulong)
         returnCode = id_SQLBindUnsignedObject (ctSql, paramBindFlag, recPtr, bindInfo, fldDesc);
      else  if (!id_SQLBindLong(ctSql, bindInfo, (long *)fldPtr, fldDesc->ctType == kTulong))
         returnCode = -1;
      if (retFldValue)
         snprintf(retFldValue, 32, (fldDesc->ctType == kTulong) ? "%lu" : "%ld", *((long *)fldPtr));
   }
   // 4. SHORT
   else  if (fldDesc->sqlType == MYSQL_TYPE_SHORT)  {
#ifdef  _DO_DISPLAY_RECORD_
      id_BufferShortAtOffset(NULL, tmpStr, recPtr, fldDesc->inRecOffset);
#endif  //  _DO_DISPLAY_RECORD_
      if (fldDesc->ctType == kTushort)
         returnCode = id_SQLBindUnsignedObject (ctSql, paramBindFlag, recPtr, bindInfo, fldDesc);
      else  if (!id_SQLBindShort(ctSql, bindInfo, (short *)fldPtr, fldDesc->ctType == kTushort))
         returnCode = -1;
      if (retFldValue)
         snprintf(retFldValue, 32, (fldDesc->ctType == kTushort) ? "%hu" : "%hd", *((short *)fldPtr));
   }
   // 5. TINY
   else  if (fldDesc->sqlType == MYSQL_TYPE_TINY)  {
#ifdef  _DO_DISPLAY_RECORD_
      id_BufferCharAtOffset(NULL, tmpStr, recPtr, fldDesc->inRecOffset);
#endif  //  _DO_DISPLAY_RECORD_
      if (fldDesc->ctType == kTuchar)
         returnCode = id_SQLBindUnsignedObject (ctSql, paramBindFlag, recPtr, bindInfo, fldDesc);
      else  if (!id_SQLBindChar(ctSql, bindInfo, (char *)fldPtr, fldDesc->ctType == kTuchar))
         returnCode = -1;
      if (retFldValue)
         snprintf (retFldValue, 32, "%hd", (short)*((unsigned char *)fldPtr));
   }
   // 6. FLOAT
   else  if (fldDesc->sqlType == MYSQL_TYPE_FLOAT)  {
#ifdef  _DO_DISPLAY_RECORD_
      id_BufferFloatAtOffset(NULL, tmpStr, recPtr, fldDesc->inRecOffset);
#endif  //  _DO_DISPLAY_RECORD_
      if (!id_SQLBindFloat(bindInfo, (float *)fldPtr))
         returnCode = -1;
   }
   // 7. DOUBLE
   else  if (fldDesc->sqlType == MYSQL_TYPE_DOUBLE)  {
#ifdef  _DO_DISPLAY_RECORD_
      id_BufferDoubleAtOffset(NULL, tmpStr, recPtr, fldDesc->inRecOffset);
#endif  //  _DO_DISPLAY_RECORD_
      if (!id_SQLBindDouble(ctSql, bindInfo, (double *)fldPtr))
         returnCode = -1;
   }
   else
      return (-1);
   
#ifdef  _DO_DISPLAY_RECORD_
   if (tmpStr[0])
      con_printf ("%hd: '%.24s'\n", fldDesc->inRecOffset, tmpStr);
#endif  //  _DO_DISPLAY_RECORD_
   
   // if (returnCode != SQLITE_OK)
   //    NSLog (@"bindSingleObject:...:usingFldDescription: - code: %d, bidx: %d,  %@: %@", returnCode, bindIndex, fldDesc->fldName, [fldObj description]);

   return (returnCode);
}

#pragma mark -

// Tu sad treba rijesit bindinje za params i za results, treba poslat recPtr, ili targetPtr po kojoj se trazi
// Znaci, verzija kad bindimo sve i verzija kad bindimo samo jednu stvar

// MORE - flag dal treba konacno bindat stvar jer ima ono bindanje u dvije faze pa tako treba i ovaj ondex koristit
// AH - YES - i paramBindFlag na dnu di bindamo zaistac

static int  id_SQLBindIdAsResult (ID_CT_MYSQL *ctSql)
{
   int  returnCode = 0;
   
   // void          *internalRecPtr  = NULL;
   
   // void          *buffPtr  = NULL;
   ID_BIND_INFO  *bindInfo = NULL;
   
   // SQLFldDescription  *fldDescCArray = ctSql->fldDescs;
   
   ctSql->resultCount = 0;
   id_SetBlockToZeros (ctSql->resultRecord, ctSql->recSizeWithId);
   // internalRecPtr = ctSql->resultRecord;
   
   bindInfo = id_SQLBindSelector (ctSql, kBindFlagResults, 0, NULL/*&buffPtr*/, NULL/*&bindCounter*/);
   
   returnCode = id_SQLBindSingleCTreeObject (ctSql, kBindFlagResults, NULL, NULL/*(char **)buffPtr*/, bindInfo, &internalSQLFields[0], NULL);
   
   return (returnCode);
}

// Used instead of id_SQLBindCTreeRecord() so proper counters need to be increased here

static int  id_SQLBindWhereIdOnly (ID_CT_MYSQL *ctSql, long idAsParam)
{
   int    returnCode = 0;
   char  *fldPtr = (char *)&ctSql->idToBind;
   
   // void          *internalRecPtr  = NULL;
   
   // void          *buffPtr  = NULL;
   ID_BIND_INFO  *bindInfo = NULL;
   
   SQLFldDescription  *fldDesc = &internalSQLFields[0];  // id
   
   ctSql->idToBind = idAsParam;
   
   ctSql->checkCount = 0;
   id_SetBlockToZeros (ctSql->checkRecord, ctSql->recSizeWithId);
   // internalRecPtr = ctSql->resultRecord;
   
   bindInfo = id_SQLBindSelector (ctSql, kBindFlagWhereClause, 0, NULL/*&buffPtr*/, NULL/*&bindCounter*/);
   
   if (!id_SQLBindLong(ctSql, bindInfo, (long *)fldPtr, fldDesc->ctType == kTulong))
      returnCode = -1;
   
   return (returnCode);
}

/* ................................................. id_SQLBindWhereRecordNameOnly .. */

// Used instead of id_SQLBindCTreeRecord() so proper counters need to be increased here
// Uses extraBindBuffer as id_SQLBindLikePart(), there shouldn't be a conflict
// We search here with the original name of the second field as the first field is what we need as result

static int  id_SQLBindWhereRecordNameOnly (ID_CT_MYSQL *ctSql)
{
   int    returnCode = 0;
   short  objSize;
   char  *fldPtr = ctSql->rfi->iRecFldOffsets[1].fCName;  // because ctSql->fldDescs[1].fldName or kMYSQLFlatKeyName are wrong;
   
   // char  *stupidPtr = id_malloc_clear_or_exit (64+4);  // OUT!
   
   ID_BIND_INFO  *bindInfo = NULL;
   
   // SQLFldDescription  *fldDesc = &internalSQLFields[0];  // id, maybe make one for this
   
   objSize = strlen (fldPtr);
   // strNCpy (stupidPtr, fldPtr, 64);  // OUT!
   
   // id_SQLAllocateExtraBindBuffer (ctSql, objSize+4);
   
   // snprintf (ctSql->extraBindBuffer, objSize, "%s", fldPtr);
   
   ctSql->checkCount = 0;
   id_SetBlockToZeros (ctSql->checkRecord, ctSql->recSizeWithId);
   
   bindInfo = id_SQLBindSelector (ctSql, kBindFlagWhereClause, 0, NULL/*&buffPtr*/, NULL/*&bindCounter*/);
   
   if (!id_SQLBindText(ctSql, bindInfo, (char *)/*stupidPtr*/fldPtr, objSize, objSize+4))
      returnCode = -1;

   strNCpy (sqlHandler->usedWhereValues[0], fldPtr[0] ? fldPtr : " ", kMaxSavedValueLen-1);

   return (returnCode);
}

// Used from id_SQLBindCTreeRecord() so proper counters already increased

static int  id_SQLBindLikePart (
 ID_CT_MYSQL        *ctSql,
 char               *recPtr,
 char              **fldBuffPtr,
 ID_BIND_INFO       *bindInfo,
 SQLFldDescription  *fldDesc,
 char               *retFldValue  // may be NULL
)
{
   int    returnCode = 0;
   short  objSize;
   char  *fldPtr = recPtr + fldDesc->inRecOffset;
   
   // void          *internalRecPtr  = NULL;
   
   // void          *buffPtr  = NULL;
   if (fldBuffPtr)
      *fldBuffPtr = fldPtr;
   if (retFldValue)
      retFldValue[0] = '\0';
   
   objSize = ctSql->setSize;

   if (objSize > fldDesc->maxLen)
      objSize = fldDesc->maxLen;
   
   id_SQLAllocateExtraBindBuffer (ctSql, objSize+4);
   
   snprintf (ctSql->extraBindBuffer, objSize+4, "%.*s%%", objSize, fldPtr);
   
   if (!id_SQLBindText(ctSql, bindInfo, (char *)ctSql->extraBindBuffer, objSize+1, fldDesc->maxLen))
      returnCode = -1;
   if (retFldValue)
      snprintf (retFldValue, 64, "%.*s%%", objSize, fldPtr);
   
   return (returnCode);
}

static int  id_SQLBindCTreeRecordAsParams (
 ID_CT_MYSQL  *ctSql,
 void         *recPtr
)
{
   return (id_SQLBindCTreeRecord(ctSql, recPtr, kBindFlagParams, 0, 0, 0));
}

static int  id_SQLBindCTreeRecordAsResults (
 ID_CT_MYSQL  *ctSql
)
{
   return (id_SQLBindCTreeRecord(ctSql, NULL, kBindFlagResults, 0, 0, 0));
}

static int  id_SQLBindVarLenResultBuffer (ID_CT_MYSQL *ctSql, long dataSize, short fldIndex)
{
   int            returnCode = 0;
   char         **ptr = NULL;
   ID_BIND_INFO  *bindInfo = &ctSql->resultBindInfos[fldIndex];
   
   id_SQLAllocVarLenBuffer (ctSql, dataSize);  // Allocates if necessary
   
   ptr = (char **)&ctSql->resultBuffers[fldIndex];
   
   *ptr = ctSql->varLenBuffPtr;
   
   if (!id_SQLBindBlob(ctSql, bindInfo, ctSql->varLenBuffPtr, dataSize, dataSize))
      returnCode = -1;
   
   return (returnCode);
}

int  id_SQLBindCTreeRecord (
 ID_CT_MYSQL  *ctSql,
 void         *recPtr,
 short         paramBindFlag,  // 1=kBindFlagParams, -1=kBindFlagWhereClause, 0=kBindFlagResults
 short         keyNo,
 short         maxKeySegs,
 short         whereFlags      // checking for kWhereFlagLike
)
{
   short  pix, i /*, *bindCounter*/;  // ptr to something...
   // long   objSize = 0L;
   int    returnCode = 0;
   // long   fldOffset;
   
   void          *internalRecPtr  = NULL;
   
   void          *buffPtr  = NULL;
   ID_BIND_INFO  *bindInfo = NULL;

   SQLFldDescription  *fldDescCArray = ctSql->fldDescs;

   if ((paramBindFlag == kBindFlagParams) || (paramBindFlag == kBindFlagResults))  {
      
      if (paramBindFlag == kBindFlagParams)  {
         ctSql->paramCount = 0;
         internalRecPtr = ctSql->paramRecord;
      }
      else  {
         ctSql->resultCount = 0;
         id_SetBlockToZeros (ctSql->resultRecord, ctSql->recSizeWithId);
         internalRecPtr = ctSql->resultRecord;
      }
      
      // Here we bind each field in the record
      for (pix=i=0; !returnCode && fldDescCArray[i].fldName; i++)  {
#ifdef  _DO_DISPLAY_RECORD_
         con_printf ("%s...", fldDescCArray[i].fldName);
#endif // _DO_DISPLAY_RECORD_

         bindInfo = id_SQLBindSelector (ctSql, paramBindFlag, pix, &buffPtr, NULL/*&bindCounter*/);
         
         if (!pix && (paramBindFlag == kBindFlagParams))  {
            if (recPtr != ctSql->currentRecord)  {
               BlockMove (recPtr, internalRecPtr, ctSql->recSize);
               if (ctSql->ifilPtr)
                  id_SQLConvertRecordEncoding (ctSql, internalRecPtr, FALSE);  // reverse flag
            }
            else
               BlockMove (recPtr, internalRecPtr, ctSql->recSizeWithId);
         }
         
         // objSize = (paramBindFlag == kBindFlagResults) ? fldDescCArray[i].maxLen : 0;

         returnCode = id_SQLBindSingleCTreeObject (ctSql, paramBindFlag, internalRecPtr, (char **)buffPtr, bindInfo, &ctSql->fldDescs[i], NULL);
         pix++;
      }
      
      if (paramBindFlag == kBindFlagResults)  {  // extra step: id
         bindInfo = id_SQLBindSelector (ctSql, kBindFlagResults, pix, NULL/*&buffPtr*/, NULL/*&bindCounter*/);
         returnCode = id_SQLBindSingleCTreeObject (ctSql, kBindFlagResults, (char *)internalRecPtr+ctSql->recSize, NULL/*(char **)buffPtr*/, bindInfo, &internalSQLFields[0], NULL);
         pix++;
      }

#ifdef  _DO_DISPLAY_RECORD_
      con_printf ("... %hd params\n", pix);
#endif // _DO_DISPLAY_RECORD_
   }
   
   // HERE BE DRAGONS!
   // One pass for normal where clause, second pass for version with extra condition for sets
   
   if (paramBindFlag == kBindFlagWhereClause)  {
      
      short            segIdx, segLen = 0, secondPassKeySegs = 0, loop, loopOffset = 0, loopCnt = 1, uvix = 0;
      char             segName[64], segValue[64];
      RecFieldOffset  *rfoArray = ctSql->rfi->iRecFldOffsets;
      
      id_SetBlockToZeros (sqlHandler->usedWhereValues, kMaxSavedWhereValues*kMaxSavedValueLen);  // Debugging purpose
      
      if (keyNo)  {
         
         if (maxKeySegs)  {
            loopCnt = 2;
            secondPassKeySegs = maxKeySegs;
            if (!ctSql->setSize)
               con_printf ("No setSize but second loop!\n");
         }
         
         for (loop=0; loop<loopCnt; loop++)  {
            
            internalRecPtr = ctSql->checkRecord;  // Must be inside of the loop
            
            if (!loop)  {
               maxKeySegs = id_SQLGetKeySegments (ctSql, keyNo);
               if ((loopCnt == 2) && ctSql->ifilPtr->ix[keyNo-1].ikeydup)
                  maxKeySegs++;
               else  if ((loopCnt == 1) && (whereFlags & kWhereFlagNeedsId))
                  maxKeySegs++;
            }
            else
               maxKeySegs = secondPassKeySegs;

            for (pix=segIdx=0; segIdx<maxKeySegs && !returnCode; segIdx++)  {
               
               if (!id_SQLKeySegmentName (ctSql, keyNo, segIdx, segName, &segLen))  {
                  
                  SQLFldDescription  *oneDesc = id_SQLFindFldDescription (ctSql, segName);
                  
                  if (oneDesc)  {
                     
                     bindInfo = id_SQLBindSelector (ctSql, paramBindFlag, pix + loopOffset, &buffPtr, NULL/*&bindCounter*/);
                     
                     if (!pix && !loop)  {
                        if (recPtr != ctSql->currentRecord)  {
                           BlockMove (recPtr, internalRecPtr, ctSql->recSize);
                           if (ctSql->ifilPtr)
                              id_SQLConvertRecordEncoding (ctSql, internalRecPtr, FALSE);  // reverse flag
                        }
                        else
                           BlockMove (recPtr, internalRecPtr, ctSql->recSizeWithId);
                     }
                     
                     if (!strcmp(oneDesc->fldName, "id"))  {
                        internalRecPtr = ctSql->checkRecord + ctSql->recSize;
                        // con_printf ("Binding id: %ld\n", *((long *)internalRecPtr));
                     }
                     
                     // if like -> FRSSET has like as only part & NXTSET has like as second part
                     if ((whereFlags & kWhereFlagLike) && ((!loop && loopCnt==1) || (loop && loopCnt==2)))
                        returnCode = id_SQLBindLikePart (ctSql, internalRecPtr, (char **)buffPtr, bindInfo, oneDesc, segValue);
                     else                        
                        returnCode = id_SQLBindSingleCTreeObject (ctSql, paramBindFlag, internalRecPtr, (char **)buffPtr, bindInfo, oneDesc, segValue);

                     if (uvix < kMaxSavedWhereValues)
                        strNCpy (sqlHandler->usedWhereValues[uvix++], segValue[0] ? segValue : " ", kMaxSavedValueLen-1);
                     pix++;
                  }
               }
            }
            loopOffset = pix;
         }
      }
#ifdef _LOG_SELECTION_STRING_
      for (i=0; i<id_SQLGetLastWhereParamCount(); i++)
         id_LogFileFormat ("%hd: %s", i+1, id_SQLGetLastWhereParamValue(i));
      id_LogFileLine ("..........\n");
#endif
   }
   
   return (returnCode);
}

#pragma mark -
#pragma mark Main Features
#pragma mark -

int  id_SQLAddCTreeObject (
 ID_CT_MYSQL  *ctSql,
 void         *recPtr,
 long         *retRowId
)
{
   short          qMarksCnt = 0, boundPrmCount, boundResCount, errorState = FALSE;
   int            stmtParamCount = 0;
   long           affectedRows = 0L, returnCode = 0L;
   
   ID_BIND_INFO  *bindInfo = NULL;
   char          *sqlString = id_malloc_or_exit (kMYSQLStringBufferLen);
   
   // NSString      *sqlString = [NSString stringWithFormat:@"INSERT INTO %s (name, title, image, thumb, comment) values (?,?,?,?,?)", kTableName];
   ctSql->lastInsertId = 0;
   if (retRowId)
      *retRowId = 0;
   
   id_SQLInsertionStringForTable (ctSql, sqlString, kMYSQLStringBufferLen, &qMarksCnt);
   
   returnCode = id_SQLInitPrepare (ctSql, sqlString);
   
#ifdef  _SHOW_INSERTION_PROGRESS_
   con_printf ("SQLAddCTreeObject - phase: %hd str: %s\n", ctSql->statementPhase, sqlString);
#endif
   
   if (!returnCode)  {
      returnCode = id_SQLBindCTreeRecordAsParams (ctSql, recPtr);  // calls id_SQLBindCTreeRecord() inside
   
      stmtParamCount = id_mysql_stmt_param_count (ctSql->sqlStatement);  //                    ------------ mysql_stmt_param_count()
#ifdef  _SHOW_INSERTION_PROGRESS_
      con_printf ("SQLAddCTreeObject - counts: %hd / %d q: %hd out of: %hd\n", ctSql->paramCount, stmtParamCount, qMarksCnt, ctSql->recSqlFldCount);
#endif
   }
      
   if (!returnCode)
      returnCode = id_SQLBindStatement (ctSql, &boundPrmCount, &boundResCount);
   else
      errorState = TRUE;

   if (!returnCode)
      returnCode = id_SQLExecuteStatement (ctSql, /*needtToStoreResult*/ FALSE, /*needToFetchResult*/ FALSE);
   
   if (!returnCode)  {
      ctSql->lastInsertId = id_mysql_stmt_insert_id (ctSql->sqlStatement);
      if (retRowId)
         *retRowId = ctSql->lastInsertId;
   }
   else  {
      if (ctSql->errNo == 1062)
         errorState = 2;
      else
         errorState = 1;
   }
   
   if (!returnCode && !errorState)  {
      affectedRows = (long)id_mysql_stmt_affected_rows (ctSql->sqlStatement);
      
      if (affectedRows != 1L)  {
         con_printf ("id_SQLAddCTreeObject - mysql_stmt_affected_rows: %ld\n", affectedRows);
         errorState = TRUE;
      }
      else  {
         BlockMove (ctSql->paramRecord, ctSql->currentRecord, ctSql->recSize);
         *(long *)(ctSql->currentRecord + ctSql->recSize) = ctSql->lastInsertId;
         ctSql->currentRecFlags = kCurrValid4Start | kCurrValid4Flow | kCurrValid4ReRead;
      }
   }
   
   returnCode = id_SQLCloseStatement (ctSql);
   
   id_SQLDisposeExtraBindBuffer (ctSql);

#ifdef  _SHOW_INSERTION_PROGRESS_
   if (!returnCode)
      con_printf ("Added one record to SQL db - ROWID = %ld!", rowId);
#endif
   
   id_DisposePtr (sqlString);

   return (returnCode ? returnCode : errorState);
}

// Ovo treba imat ptr na old record,
// Zatim kad bindamo drugi dio, onda treba ici od indexa koji je bio obavljen u prvoj fazi
// Za to treba prilagoditi onu BindSingleObject funkciju
// Kako stvari stoje, onaj bind array bi se trebao postavljati svaki put kad nesto bindamo
// I to znaci da taj array treba bit veci nego sto je sad, mozda cak duplo da smo ziheraski sigurni

// Radilo - UPDATE Bouquet_ERGO_Pero_2021.SingletonRecords SET SingletonContent='ABCD',SingletonKey='idConfigPtr' WHERE SingletonKey='idConfigRec';

int  id_SQLRewriteCTreeObject (  // formerly - id_SQLUpdateCTreeObject
 ID_CT_MYSQL  *ctSql,
 void         *oldRecPtr,
 void         *newRecPtr
// long         *retRowId
)
{
   long           rowId, returnCode = 0;
   long           dataSize, affectedRows = 0L;
   long           oldRowId = *(long *)((char *)oldRecPtr + ctSql->recSize);  //  = ctSql->lastInsertId;
   int            stmtParamCount = 0;
   short          qMarksCnt = 0, boundPrmCount, boundResCount, errorState = FALSE;
   short          keyNo = 0;  // Should become 1
   short          bindingCnt1, bindingCnt2;
   ID_BIND_INFO  *bindInfo = NULL;
   char          *sqlString = id_malloc_or_exit (kMYSQLStringBufferLen);
   
   // NSString      *sqlString = [NSString stringWithFormat:@"UPDATE %s SET %s = ? WHERE name = ?", kTableName, thumbFlag ? "thumb" : "image"];
   
   id_SQLUpdatingStringForTable (ctSql, sqlString, kMYSQLStringBufferLen, &keyNo, &qMarksCnt);  // All the params + where clause for key 1
   
   returnCode = id_SQLInitPrepare (ctSql, sqlString);
   
   if (!returnCode)  {
#ifdef  _SHOW_UPDATING_PROGRESS_
      con_printf ("SQLRewriteCTreeObject - phase: %hd\n", ctSql->statementPhase);
#endif
   
      returnCode = id_SQLBindCTreeRecordAsParams (ctSql, newRecPtr);  // calls id_SQLBindCTreeRecord() inside
   }
   else
      errorState = TRUE;
   
   if (!returnCode)  {
      if (!ctSql->ifilPtr)
         returnCode = id_SQLBindWhereRecordNameOnly (ctSql);
      else
         returnCode = id_SQLBindCTreeRecord (ctSql, oldRecPtr, kBindFlagWhereClause, keyNo, 0, 0); // All segments, no flags
   }
   else
      errorState = TRUE;

   stmtParamCount = id_mysql_stmt_param_count (ctSql->sqlStatement);  //                    ------------ mysql_stmt_param_count()
   
#ifdef  _SHOW_UPDATING_PROGRESS_
   con_printf ("SQLRewriteCTreeObject - counts: %hd / %d q: %hd out of: %hd\n", ctSql->paramCount, stmtParamCount, qMarksCnt, ctSql->recSqlFldCount);
#endif

   if (!returnCode)
      returnCode = id_SQLBindStatement (ctSql, &boundPrmCount, &boundResCount);
   else
      errorState = TRUE;
   
   if (!returnCode)
      returnCode = id_SQLExecuteStatement (ctSql, /*needtToStoreResult*/ FALSE, /*needToFetchResult*/ FALSE);
   else
      errorState = TRUE;
   
   if (!returnCode && !errorState)  {
      affectedRows = (long)id_mysql_stmt_affected_rows (ctSql->sqlStatement);
   
      if (affectedRows != 1L)  {
         con_printf ("SQLRewriteCTreeObject - mysql_stmt_affected_rows: %ld\n", affectedRows);
         if (memcmp(ctSql->paramRecord, ctSql->currentRecord, ctSql->recSize))  // When nothing's different
            errorState = TRUE;
      }
      else  {
         BlockMove (ctSql->paramRecord, ctSql->currentRecord, ctSql->recSize);
         *(long *)(ctSql->currentRecord + ctSql->recSize) = oldRowId;
         ctSql->currentRecFlags = kCurrValid4Start | kCurrValid4Flow | kCurrValid4ReRead;
      }
   }
   else  {
      if (ctSql->errNo == 1062)
         errorState = 2;
      else
         errorState = 1;
   }   
   returnCode = id_SQLCloseStatement (ctSql);

   id_SQLDisposeExtraBindBuffer (ctSql);

   id_DisposePtr (sqlString);

   return (returnCode ? returnCode : errorState);
}

// Pass the whole old object itself or just field names and values to be used in where clause
// If not fldNamesToMatch but fldValuesToMatch - then it's id (ROWID)

int  id_SQLDeleteCTreeObject (
 ID_CT_MYSQL  *ctSql,
 void         *recPtr
)
{
   // NSString  *sqlString = [NSString stringWithFormat:@"DELETE FROM %s WHERE name = ?", kTableName];
   
   long           returnCode = 0;
   long           dataSize, affectedRows = 0L;
   int            stmtParamCount = 0, columnCount = 0, rowCount = 0;
   short          qMarksCnt = 0, boundPrmCount, boundResCount, errorState = FALSE;
   short          keyNo = 0;  // Should become 1
   ID_BIND_INFO  *bindInfo = NULL;
   char          *sqlString = id_malloc_or_exit (kMYSQLStringBufferLen);

   id_SQLDeletionStringForTable (ctSql, sqlString, kMYSQLStringBufferLen, &keyNo, &qMarksCnt);

   returnCode = id_SQLInitPrepare (ctSql, sqlString);
   
#ifdef  _SHOW_DELETION_PROGRESS_
   con_printf ("SQLDeleteCTreeObject - phase: %hd\n", ctSql->statementPhase);
#endif

   // Bind items in WHERE clause -> includingOnlySearchable:YES
   
   if (!returnCode)  {
      if (!ctSql->ifilPtr)
         returnCode = id_SQLBindWhereRecordNameOnly (ctSql);
      else
         returnCode = id_SQLBindCTreeRecord (ctSql, recPtr, kBindFlagWhereClause, keyNo, 0, 0); // All segments, no flags
   }
   else
      errorState = TRUE;
   
   stmtParamCount = id_mysql_stmt_param_count (ctSql->sqlStatement);  //    ------------ mysql_stmt_param_count()

#ifdef  _SHOW_DELETION_PROGRESS_
   con_printf ("SQLDeleteCTreeObject - counts: %hd / %d q: %hd out of: %hd\n", ctSql->paramCount, stmtParamCount, qMarksCnt, ctSql->recSqlFldCount);
#endif
   
   if (!returnCode)
      returnCode = id_SQLBindStatement (ctSql, &boundPrmCount, &boundResCount);
   else
      errorState = TRUE;
   
   if (!returnCode)
      returnCode = id_SQLExecuteStatement (ctSql, /*needtToStoreResult*/ FALSE, /*needToFetchResult*/ FALSE);
   else
      errorState = TRUE;

   if (!returnCode && !errorState)  {
      affectedRows = (long)id_mysql_stmt_affected_rows (ctSql->sqlStatement);
      
      if (affectedRows != 1L)  {
         con_printf ("SQLDeleteCTreeObject - mysql_stmt_affected_rows: %ld\n", affectedRows);
         errorState = TRUE;
      }
   }

   id_SQLCloseStatement (ctSql);

   id_SQLDisposeExtraBindBuffer (ctSql);

   id_DisposePtr (sqlString);

   return (returnCode);
}

// CALLING IT:
// 1. pass an SQLSelectParams instance with WHERE, SORTED BY, LIMIT and OFFSET params
// 2. just pass one or more fields to match
// 3. if searching by id: fldNamesToMatch is nil and fldValuesToMatch has one item, desired id (ROWID)
// RESULTS:
// 1. recieve one field - onlyFieldOrNil, may be "id"
// NOPE IN C - 2. recieve array of objects

int  id_SQLSelectCTreeObject (
 ID_CT_MYSQL      *ctSql,
 SQLSelectParams  *selParams,
 void             *inRecPtr,
 void             *outRecPtr  // If NULL, select only id
)
{
   // NSString      *preparedName = [dbFileHelper sqlStringWithString:iName];
   // NSString      *sqlString = [NSString stringWithFormat:@"SELECT %s FROM %s WHERE name = ?", "image", kTableName];
   
   Boolean        needsPrefix = FALSE;  // If onlyFieldOrNil needs to be prefixed with first table
   // NSArray       *multiTables = [tableName componentsSeparatedByString:@","];
   
   long           returnCode = 0;
   long           dataSize;
   int            stmtParamCount = 0, columnCount = 0, rowCount = 0;
   my_bool        varLenAttribute = 1;
   short          savedRecFlags = ctSql->currentRecFlags & (kCurrValid4ReRead | kCurrValid4Flow);
   short          qMarksCnt = 0, bindingCnt, boundPrmCount, boundResCount, err = 101;
   ID_BIND_INFO  *bindInfo = NULL;
   MYSQL_RES     *prepMetaResult = NULL;
   MYSQL_FIELD   *varField = NULL;  // &prepMetaResult->fields[ctSql->recSqlFldCount-1];
   char          *sqlString = id_malloc_or_exit (kMYSQLStringBufferLen);
   
   id_SQLSelectionStringForTable (ctSql, selParams, sqlString, kMYSQLStringBufferLen, &qMarksCnt, &bindingCnt);
   
   returnCode = id_SQLInitPrepare (ctSql, sqlString);
   
#ifdef  _SHOW_SELECTTION_PROGRESS_
   con_printf ("SQLSelectCTreeObject - phase: %hd\n", ctSql->statementPhase);
#endif
   
   if (!returnCode)  {
      if (inRecPtr && !selParams->distinctFlag)  {
         if (ctSql->varLenFlag)
            id_mysql_stmt_attr_set (ctSql->sqlStatement, STMT_ATTR_UPDATE_MAX_LENGTH, &varLenAttribute);
         returnCode = id_SQLBindCTreeRecord (ctSql, inRecPtr, kBindFlagWhereClause,
                                             selParams->keyNo, selParams->setKeySegs ? selParams->setKeySegs : 0, selParams->whereFlags); // All segments
      }
      else  if (selParams->whereFlags & kWhereFlagWhereIdOnly)  {
         returnCode = id_SQLBindWhereIdOnly (ctSql, selParams->idAsParam);
      }
      else  if (selParams->whereFlags & kWhereFlagRecordNameOnly)  {
         returnCode = id_SQLBindWhereRecordNameOnly (ctSql);
      }
   }

   stmtParamCount = id_mysql_stmt_param_count (ctSql->sqlStatement);  //    ------------ mysql_stmt_param_count()

#ifdef  _SHOW_SELECTTION_PROGRESS_
   con_printf ("SQLSelectCTreeObject - counts: %hd / %d q: %hd out of: %hd\n", ctSql->paramCount, stmtParamCount, qMarksCnt, ctSql->recSqlFldCount);
#endif

   if (!returnCode)
      returnCode = id_SQLBindStatement (ctSql, &boundPrmCount, &boundResCount);
   
   prepMetaResult = id_mysql_stmt_result_metadata (ctSql->sqlStatement);
   
   if (!prepMetaResult)  {
      con_printf ("mysql_stmt_result_metadata: %s\n", id_mysql_stmt_error(ctSql->sqlStatement));
      returnCode = CR_NO_STMT_METADATA;
   }
   else  {
      columnCount = id_mysql_num_fields (prepMetaResult);
      if ((!selParams->selectIdOnly && (columnCount != ctSql->recSqlFldCount+1)) ||
          (selParams->selectIdOnly && (columnCount != 1)))  /* validate column count */
         con_printf ("Invalid column count returned by MySQL: %d\n", columnCount);
   }
   
   if (!returnCode)
      returnCode = id_SQLExecuteStatement (ctSql, /*needtToStoreResult*/ FALSE, /*needToFetchResult*/ FALSE);  // Ovo izbaci, izgleda da ne treba

   if (!returnCode)  {
      if (selParams->selectIdOnly)
          returnCode = id_SQLBindIdAsResult (ctSql);
      else
          returnCode = id_SQLBindCTreeRecordAsResults (ctSql);  // calls id_SQLBindCTreeRecord() inside, recPtr not necessary
   }
   if (!returnCode)
      returnCode = id_SQLBindStatement (ctSql, &boundPrmCount, &boundResCount);

   // DO TUTE:
   // Razbiti id_SQLBindStatement -> Params + Results posebno
   // Onda bindati result naknadno
   // Provjeriti param cnt 
   // stmtParamCount = mysql_stmt_param_count (ctSql->sqlStatement);
   // prepareMetaResult = mysql_stmt_result_metadata (ctSql->sqlStatement);
   // Tek nakon exec ide bind za result
   // Petlja za selParams->limitCnt, ak je jedan hendla id_SQLCopyResultRecord(), ostalo ide u ctSql->tblState[ctSql->curTblStateIdx].pageData
   
   if (!returnCode)  {
      if (returnCode = id_mysql_stmt_store_result(ctSql->sqlStatement))
         con_printf ("mysql_stmt_store_result: %s\n", id_mysql_stmt_error(ctSql->sqlStatement));
      if (ctSql->varLenFlag)  {
         // SELECT OCTET_LENGTH(History.h_docData) from History WHERE h_timestamp = 3761057665;        -> 1
         // SELECT OCTET_LENGTH(History.h_timestamp) from History WHERE h_timestamp = 3761057665;      -> 10
         // SELECT OCTET_LENGTH(SingletonRecords.SingletonContent) FROM SingletonRecords WHERE id = 1; -> 30844
         // Test
         //     id_mysql_free_result (prepMetaResult);
         //     prepMetaResult = id_mysql_stmt_result_metadata (ctSql->sqlStatement);
         // End test
         varField = &prepMetaResult->fields[ctSql->recSqlFldCount-1];  // Use varField->max_length
         // Tu funkcija koja to sve pobajnda npr id_SQLBindVarLenResultBuffer ();
         id_SQLBindVarLenResultBuffer (ctSql, varField->max_length, ctSql->recSqlFldCount-1);
         ctSql->resultCount = boundResCount;
         returnCode = id_SQLBindStatement (ctSql, &boundPrmCount, &boundResCount);
      }
   }

   if (!returnCode)  {
      short  useLast = selParams && (selParams->limitCnt > 1) && (selParams->whereFlags & (kWhereFlagLessThan | kWhereFlagLessOrEQ));
      
      rowCount = 0;
      if (ctSql->tblState)
         ctSql->tblState[ctSql->curTblStateIdx].usedPageRows = 0;
      while (!id_mysql_stmt_fetch(ctSql->sqlStatement))  {
         
         if (!selParams->selectIdOnly)  {
            
            char  docSignature[128];
            
            id_SQLCopyBoundUnsignedResults (ctSql, ctSql->resultRecord);  // put unsigned ints where they belong

            if (!rowCount && !useLast)
               id_SQLCopyResultRecord (ctSql, outRecPtr, 0);  // ctSql->resultRecord has the data right now, outRecPtr may be NULL
            
            // Tu sad igra s bufferima

            if (ctSql->tblState && ctSql->tblState[ctSql->curTblStateIdx].pageData && (rowCount < ctSql->tblState[ctSql->curTblStateIdx].pageRows))
               BlockMove (ctSql->resultRecord, ctSql->tblState[ctSql->curTblStateIdx].pageData + (ctSql->recSizeWithId * rowCount), ctSql->recSizeWithId);
            else  if (rowCount)
               con_printf ("SQLSelectCTreeObject - Unexpected row %hd\n", rowCount);
            
            (*sqlHandler->recSigProcPtr) (ctSql->rfi->iRecId, ctSql->resultRecord, docSignature, 63);  // Debugging name of the table
            
            id_SetBlockToZeros (ctSql->resultRecord, ctSql->recSizeWithId);
         }
         
         rowCount++;
      }
      
      if (!selParams->selectIdOnly)
         id_SQLStoreResultRecords (ctSql, outRecPtr, selParams->keyNo, rowCount, selParams->limitCnt, useLast);
   }
   
   if (prepMetaResult)
      id_mysql_free_result (prepMetaResult);

   id_SQLCloseStatement (ctSql);
   id_SQLDisposeExtraBindBuffer (ctSql);
   
   if (!returnCode && rowCount)  {
      err = 0;
      ctSql->currentRecFlags = kCurrValid4Start | kCurrValid4Flow | kCurrValid4ReRead;
   }
   else
      ctSql->currentRecFlags = savedRecFlags;  // not kCurrInvalid
   
   id_DisposePtr (sqlString);
   
   return (returnCode ? returnCode : err);
}

#pragma mark Utilities

// Izgleda da SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA ne radi za druge baze kad se odmah spojim na bazu kod logiranja

// SHOW TABLES;
// SHOW COLUMNS IN TblName;
// SHOW COLUMNS FROM DBName.TblName;
// SELECT count(*) AS SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'DBName'
// SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'DBName'
// SELECT count(SCHEMA_NAME) FROM INFORMATION_SCHEMA.SCHEMATA;
// SELECT count(SCHEMA_NAME) FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'DBName';  -> broj baza, jel kreirana

// SELECT DISTINCT TABLE_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = 'bmsimple' AND TABLE_NAME = 'AdresarA';  -> Table Exists

// SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = 'DBName' AND TABLE_NAME = 'TblName';

// SELECT count(*) FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='DBName';   -> broj tablica


// partName may be NULL

int  id_SQLGetDatabaseList (char *partName, char **dbNames, long *arrayLen, short itemSize)  // as in  names[arrayLen][itemSize]
{
   long  retCount;
   char  sqlString[1024];
   
   if (partName)
      snprintf (sqlString, 1023+1, "SELECT schema_name FROM information_schema.schemata WHERE schema_name LIKE '%s%%'", partName);
   else
      snprintf (sqlString, 1023+1, "SELECT schema_name FROM information_schema.schemata");  // TEST THIS SHIT!
   
   if (!id_SQLGetTextArrayExecutingSqlStatement(sqlString, dbNames, itemSize, arrayLen, FALSE))
      return (0);
   
   return (-1);
}

int  id_SQLCheckDatabaseExist (char *dbName)
{
   long  retCount;
   char  sqlString[1024];
   
   snprintf (sqlString, 1023+1, "SELECT count(*) AS schema_name FROM information_schema.schemata WHERE schema_name = '%s'", dbName);
   
   if (!id_SQLGetLongExecutingSqlStatement(sqlString, &retCount))  {
      if (retCount == 1)
         return (0);
   }
   
   return (-1);
}

int  id_SQLCheckTableExist (char *dbName, char *tableName)
{
   long  retCount;
   char  sqlString[1024], retString[128];
   
   snprintf (sqlString, 1023+1, "SELECT DISTINCT TABLE_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s'",
             dbName ? dbName : id_SQLActiveDatabaseName(), tableName);
   
   if (!id_SQLGetTextExecutingSqlStatement(sqlString, retString, 127))  {
      if (strlen(retString))
         return (0);
   }
   
   return (-1);
}

// Test if table exists, if it has expected number of columns and expected number of indexes - all at te same time

int  id_SQLCheckTableCondition (ID_CT_MYSQL *ctSql)
{
   long   retCount, expectedCount;
   char   sqlString[1024], retString[128];
   
   char  *sqlFormat = "SELECT (SELECT COUNT(*) TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s') + "
                      "(SELECT COUNT(*) AS NUMBEROFCOLUMNS FROM information_schema.columns WHERE table_schema = '%s' AND table_name = '%s') + "
                      "(SELECT COUNT(*) AS NUMBEROFCOLUMNS FROM information_schema.table_constraints WHERE table_schema = '%s' AND table_name = '%s')";
   
   expectedCount = 1 + (ctSql->recSqlFldCount + 1) + id_SQLGetNumberOfIndexes (ctSql);

   // SELECT (SELECT COUNT(*) TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s') +
   // (SELECT COUNT(*) AS NUMBEROFCOLUMNS FROM information_schema.columns WHERE table_schema = '%s' AND table_name = '%s') +
   // (SELECT COUNT(*) AS NUMBEROFCOLUMNS FROM information_schema.table_constraints WHERE table_schema = '%s' AND table_name = '%s');
   
   
   snprintf (sqlString, 1023+1, sqlFormat,
             ctSql->dbName, ctSql->tableName, ctSql->dbName, ctSql->tableName, ctSql->dbName, ctSql->tableName);
   
   if (!id_SQLGetLongExecutingSqlStatement(sqlString, &retCount) && (retCount == expectedCount))
      return (0);
   
   return (-1);
}

// Index exists:
// SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS WHERE table_schema=DATABASE() AND table_name='SingletonRecords' AND index_name='index_1';

int  id_SQLCheckIndexExist (char *dbName, char *tableName, char *indexName)
{
   long  idxCount;
   char  sqlString[1024], retString[128];
   
   snprintf (sqlString, 1023+1, "SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s' AND INDEX_NAME = '%s'",
             dbName ? dbName : id_SQLActiveDatabaseName(), tableName, indexName);
   
   if (!id_SQLGetLongExecutingSqlStatement(sqlString, &idxCount) && (idxCount > 0L))
      return (0);
   
   return (-1);
}

int  id_SQLCheckColumnExist (char *dbName, char *tableName, char *columnName)
{
   char  sqlString[1024], retString[128];
   
   snprintf (sqlString, 1023+1, "SELECT DISTINCT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s' AND COLUMN_NAME = '%s'",
             dbName ? dbName : id_SQLActiveDatabaseName(), tableName, columnName);
   
   if (!id_SQLGetTextExecutingSqlStatement(sqlString, retString, 127))  {
      if (strlen(retString))
         return (0);
   }
   
   return (-1);
}

// SELECT count(*) FROM information_schema.tables WHERE table_schema = '%s'"

int  id_SQLCountOfTablesInDatabase (char *dbName, long *retCount)
{
   char  sqlString[1024];
   
   if (sqlHandler)  {
      snprintf (sqlString, 1023+1, "SELECT count(*) FROM information_schema.tables WHERE table_schema = '%s'",
                dbName ? dbName : id_SQLActiveDatabaseName());
   
      return (id_SQLGetLongExecutingSqlStatement(sqlString, retCount));
   }
   
   return (0);
}

// WE NEED THIS FOR DATENT() function

int  id_SQLCountOfRecordsInTable (char *dbName, char *tableName, long *retCount)
{
   char  sqlString[1024];

   snprintf (sqlString, 1023+1, "SELECT count(*) FROM %s.%s", dbName ? dbName : id_SQLActiveDatabaseName(), tableName);
   
   return (id_SQLGetLongExecutingSqlStatement(sqlString, retCount));
}

// SELECT count(*) AS NUMBEROFCOLUMNS FROM information_schema.table_constraints WHERE table_schema = 'Bouquet_ERGO_Pero_2021' && table_name = 'SingletonRecords'

int  id_SQLCountOfIndexesInTable (char *dbName, char *tableName, long *retCount)
{
   char  sqlString[1024];
   
   snprintf (sqlString, 1023+1, "SELECT count(*) AS NUMBEROFCOLUMNS FROM information_schema.table_constraints WHERE table_schema = '%s' AND table_name = '%s'",
             dbName ? dbName : id_SQLActiveDatabaseName(), tableName);
   
   return (id_SQLGetLongExecutingSqlStatement(sqlString, retCount));
}

int  id_SQLCountOfColumnsInTable (char *dbName, char *tableName, long *retCount)
{
   char  sqlString[1024];
   
   snprintf (sqlString, 1023+1, "SELECT count(*) AS NUMBEROFCOLUMNS FROM information_schema.columns WHERE table_schema = '%s' AND table_name = '%s'",
             dbName ? dbName : id_SQLActiveDatabaseName(), tableName);
   
   return (id_SQLGetLongExecutingSqlStatement(sqlString, retCount));
}

// To check if a table has a column with certain value

int  id_SQLCountOfExactColumnsInTable (char *dbName, char *tableName, char *columnName, char *columnValue, long *retCount)
{
   char  sqlString[1024];
   
   snprintf (sqlString, 1023+1, "SELECT count(*) AS NUMBEROFCOLUMNS FROM %s.%s WHERE %s = '%s'",
             dbName ? dbName : id_SQLActiveDatabaseName(), tableName,
             columnName, columnValue);
   
   return (id_SQLGetLongExecutingSqlStatement(sqlString, retCount));
}

#pragma mark Internals

// At startup

static void  id_SQLInitTableState (ID_CT_MYSQL *ctSql, short reqPageRows)
{
   short  i;
   
   ctSql->tblState = (ID_TABLE_STATE *)id_malloc_clear_or_exit (sizeof(ID_TABLE_STATE) * kStandardSelSets);
   
   ctSql->curTblStateIdx = 0;
   
   for (i=0; i<kStandardSelSets; i++)  {
      ctSql->tblState[i].tsKeyNo = 1;
      ctSql->tblState[i].tsSetSize = 0;      // If needed, 0 -> no set
      
      ctSql->tblState[i].selWhereFlags = kWhereFlagEqual;
      ctSql->tblState[i].selectIdOnly  = FALSE;
      
      ctSql->tblState[i].numSegSet = 0;    // if used for set
      if (ctSql->ifilPtr)
         ctSql->tblState[i].numSegOrdBy = id_SQLGetKeySegments (ctSql, ctSql->tblState[i].tsKeyNo);  // if used for ORDER BY
      else
         ctSql->tblState[i].numSegOrdBy = 1;
      ctSql->tblState[i].numSegWhere = 0;  // if used for WHERE
      
      // ctSql->tblState[i].startOffset = 0;  // where we are (Last SELECT OFFSET)
      // ctSql->tblState[i].nextOffset  = 1;   // where to go next (Next OFFSET if we go forward)
      
      ctSql->tblState[i].pageRows     = reqPageRows;     // should be equal for all records but anyway...., maybe for Trans it should be like 512 and for docs 64
      ctSql->tblState[i].pageIndex    = 0;    // where we are in the page of records
      ctSql->tblState[i].usedPageRows = 0;
      
      ctSql->tblState[i].pageData = NULL;     // rec size * pageRows, may be NULL so I need to allocate it as needed - should I free on err? On add, rwt i del too
   }
}

static int  id_SQLAllocTableStatePageData (ID_CT_MYSQL *ctSql)
{
   // pageRows is set to kMYSQLBufferedRows, in the call to id_SQLInitTableState()
                         
   if (ctSql->tblState)  {
      if (!ctSql->tblState[ctSql->curTblStateIdx].pageData)
         ctSql->tblState[ctSql->curTblStateIdx].pageData = id_malloc_clear_or_exit (ctSql->recSizeWithId * ctSql->tblState[ctSql->curTblStateIdx].pageRows);
      else
         id_SetBlockToZeros (ctSql->tblState[ctSql->curTblStateIdx].pageData, ctSql->recSizeWithId * ctSql->tblState[ctSql->curTblStateIdx].pageRows);

      ctSql->tblState[ctSql->curTblStateIdx].pageIndex    = 0;    // where we are in the page of records
      ctSql->tblState[ctSql->curTblStateIdx].usedPageRows = 0;
      
      return (0);
   }
   
   return (-1);
}

static int  id_SQLDisposeTableStatePageData (ID_CT_MYSQL *ctSql, short onlyIndex)  // -1 for all
{
   short  i;
   
   if (ctSql->tblState)  {
      for (i=0; i<kStandardSelSets; i++)  {
         if (ctSql->tblState[i].pageData && ((onlyIndex == -1) || (onlyIndex == i)))  {
            id_DisposePtr (ctSql->tblState[i].pageData);
            ctSql->tblState[i].pageData = NULL;
         }
      }
   }
   
   return (-1);
}


/*
 
 MAYBE I DON'T NEED THIS!
 
static int  id_SQLAllocTableStateSelParamData (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams)
{
   if (ctSql->tblState)  {
      if (ctSql->tblState[ctSql->curTblStateIdx].likeBuffer)  {
         id_DisposePtr (ctSql->tblState[ctSql->curTblStateIdx].likeBuffer);
         ctSql->tblState[ctSql->curTblStateIdx].likeBuffer = NULL;
      }
      if (selParams->likeBuffer)
         ctSql->tblState[ctSql->curTblStateIdx].likeBuffer = selParams->likeBuffer;
      
      return (0);
   }
   
   return (-1);
}

static int  id_SQLDisposeTableStateSelParamData (ID_CT_MYSQL *ctSql, short onlyIndex)  // -1 for all
{
   short  i;
   
   if (ctSql->tblState)  {
      for (i=0; i<kStandardSelSets; i++)  {
         if (ctSql->tblState[i].likeBuffer && ((onlyIndex == -1) || (onlyIndex == i)))  {
            id_DisposePtr (ctSql->tblState[i].likeBuffer);
            ctSql->tblState[i].likeBuffer = NULL;
         }
      }
   }
   
   return (-1);
}
*/

// Used initially in EQLREC, FRSREC etc. ... but not in NXTSET, NXTREC etc.

static int  id_SQLSetupTableState (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams)
{
   if (!ctSql->tblState)
      return (-1);
   
   ctSql->tblState[ctSql->curTblStateIdx].tsKeyNo = selParams->keyNo;
   if (ctSql->setSize)
      ctSql->tblState[ctSql->curTblStateIdx].tsSetSize = ctSql->setSize;      // If needed, preserve for future calls
   
   ctSql->tblState[ctSql->curTblStateIdx].selWhereFlags = selParams->whereFlags;
   
   ctSql->tblState[ctSql->curTblStateIdx].selectIdOnly = selParams->selectIdOnly;

   ctSql->tblState[ctSql->curTblStateIdx].numSegSet   = selParams->setKeySegs;      // if used for set, setup separately
   ctSql->tblState[ctSql->curTblStateIdx].numSegOrdBy = selParams->orderByKeySegs;  // if used for ORDER BY
   ctSql->tblState[ctSql->curTblStateIdx].numSegWhere = selParams->whereKeySegs;    // if used for WHERE
   
   // ctSql->tblState[ctSql->curTblStateIdx].startOffset = selParams->offsetCnt;  // where we are (Last SELECT OFFSET)
   // ctSql->tblState[ctSql->curTblStateIdx].nextOffset = selParams->offsetCnt + selParams->limitCnt;   // where to go next (Next OFFSET if we go forward)
   
   // ctSql->tblState[ctSql->curTblStateIdx].pageRows = reqPageRows;     // should be equal for all records but anyway...., maybe for Trans it should be like 512 and for docs 64
   ctSql->tblState[ctSql->curTblStateIdx].pageIndex = -1;                // where we are in the page of records
   ctSql->tblState[ctSql->curTblStateIdx].usedPageRows = 0;
   
   ctSql->tblState[ctSql->curTblStateIdx].prevPageRows = 1;    // used for the last call
   ctSql->tblState[ctSql->curTblStateIdx].pagePass = 0;        // how many times we called NXT/PRV

   ctSql->tblState[ctSql->curTblStateIdx].nothingForward = FALSE;        // if we reach the end while filling .pageData
   ctSql->tblState[ctSql->curTblStateIdx].nothingBackward = FALSE;

   id_SQLDisposeTableStatePageData (ctSql, ctSql->curTblStateIdx);
   
   // id_SQLAllocTableStateSelParamData (ctSql, selParams);    // like buffer etc
   
   if (selParams->limitCnt > 1)  {
      if (selParams->limitCnt > kMYSQLBufferedRows)
         selParams->limitCnt = kMYSQLBufferedRows;
      id_SQLAllocTableStatePageData (ctSql);
   }

   // if (ctSql->tblState[ctSql->curTblStateIdx].pageData)
   //    id_DisposePtr ((Ptr)ctSql->tblState[ctSql->curTblStateIdx].pageData);
   // ctSql->tblState[ctSql->curTblStateIdx].pageData = NULL;     // rec size * pageRows, may be NULL so I need to allocate it as needed - should I free on err? On add, rwt i del too
   
   return (0);
}

// Used for NXTREC, NXTSET etc.

// MULTIPLE ISSUES!!!!
// a) GTE THAN FIRST pa OFFSET+LIMIT - ovu varijantu iskopaj iz backupa prije 01.10.20
// b) GTE THAN current pa neki extra uvjet 

static int  id_SQLRecallBackTableState (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, short fileNo, short whereFlags, Boolean needSetSize)
{
   short  keyNo, numSegWhere, numSegOrdBy;

   if (!ctSql->tblState)
      return (-1);
   
   if (whereFlags & kWhereFlagGreaterThan)  {
      if (ctSql->tblState[ctSql->curTblStateIdx].nothingForward)
         return (-1);        // if we reached the end already
   }
   if (whereFlags & kWhereFlagLessThan)  {
      if (ctSql->tblState[ctSql->curTblStateIdx].nothingBackward)
         return (-1);        // if we reached the end already
   }
   
   id_SetBlockToZeros (selParams, sizeof(SQLSelectParams));

   if (ctSql->ifilPtr)  {
      keyNo = fileNo - ctSql->ifilPtr->tfilno;
      numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);
   }
   else  {
      keyNo = 1;
      numSegWhere = numSegOrdBy = 1;
   }

   selParams->keyNo = keyNo;

   if (needSetSize)
      ctSql->setSize = ctSql->tblState[ctSql->curTblStateIdx].tsSetSize;      // If needed, 0 -> no set
   
   selParams->whereFlags = whereFlags; //  ? whereFlags : ctSql->tblState[ctSql->curTblStateIdx].selWhereFlags;
   selParams->selectIdOnly = FALSE;  // Has nathing to do with sets
   selParams->setKeySegs = 0;  //  ctSql->tblState[ctSql->curTblStateIdx].numSegSet;    // if used for set, setup separately
   selParams->orderByKeySegs = numSegOrdBy;    // if used for ORDER BY
   selParams->whereKeySegs   = numSegWhere;    // if used for WHERE
   
   selParams->offsetCnt = 0;  // where we are (Last SELECT OFFSET)
   if (needSetSize)
      selParams->limitCnt = ctSql->tblState[ctSql->curTblStateIdx].pageRows;
   else  {
      if (ctSql->tblState[ctSql->curTblStateIdx].pagePass)  {
         selParams->limitCnt = ctSql->tblState[ctSql->curTblStateIdx].prevPageRows * 8;
         if (selParams->limitCnt > ctSql->tblState[ctSql->curTblStateIdx].pageRows)
            selParams->limitCnt = ctSql->tblState[ctSql->curTblStateIdx].pageRows;
         // con_printf ("RecallBackTableState: %hd\n", selParams->limitCnt);
      }
      else
         selParams->limitCnt = 1;
   }
   
   ctSql->tblState[ctSql->curTblStateIdx].prevPageRows = selParams->limitCnt;    // used for the last call
   ctSql->tblState[ctSql->curTblStateIdx].pagePass++;                            // how many times we called NXT/PRV
   
   // ctSql->tblState[ctSql->curTblStateIdx].pageIndex = 0;       // where we are in the page of records
   
   // selParams->likeBuffer = ctSql->tblState[ctSql->curTblStateIdx].likeBuffer;

   if (!ctSql->tblState[ctSql->curTblStateIdx].pageData)
      id_SQLAllocTableStatePageData (ctSql);

   return (0);
}

// Never use directly, use id_SQLNextFromTableStateBuffer() etc.
// See if I need to be bothered with sets!

static int  id_SQLFetchFromTableStateBuffer (ID_CT_MYSQL *ctSql, /*SQLSelectParams *selParams,*/ void *outRecPtr)
{
   char  *buffPtr = ctSql->tblState[ctSql->curTblStateIdx].pageData;
   
   // if (!ctSql->tblState || !ctSql->tblState[ctSql->curTblStateIdx].pageData)
   //    return (-1);
   
   if ((ctSql->tblState[ctSql->curTblStateIdx].pageIndex < 0) ||
       (ctSql->tblState[ctSql->curTblStateIdx].pageIndex >= ctSql->tblState[ctSql->curTblStateIdx].usedPageRows))
      return (-1);
   
   BlockMove (buffPtr + (ctSql->recSizeWithId * ctSql->tblState[ctSql->curTblStateIdx].pageIndex), ctSql->currentRecord, ctSql->recSizeWithId);  // Internal rec
   BlockMove (ctSql->currentRecord, outRecPtr, ctSql->recSize);                                                                                  // External
   
   if (ctSql->ifilPtr)
      id_SQLConvertRecordEncoding (ctSql, outRecPtr, TRUE);  // reverse flag
      
   id_SQLSetLastSqlString ("BufferedFetch!");  // Because there was none
   
   return (0);
}

static int  id_SQLNextFromTableStateBuffer (ID_CT_MYSQL *ctSql, short keyNo, void *outRecPtr)
{
   if (!ctSql->tblState || !ctSql->tblState[ctSql->curTblStateIdx].pageData)
      return (-1);
   
   if ((ctSql->tblState[ctSql->curTblStateIdx].pageIndex < 0) ||
       (ctSql->tblState[ctSql->curTblStateIdx].tsKeyNo != keyNo))
      return (-1);
   
   ctSql->tblState[ctSql->curTblStateIdx].pageIndex++;
   
   return (id_SQLFetchFromTableStateBuffer(ctSql, outRecPtr));
}

static int  id_SQLPrevFromTableStateBuffer (ID_CT_MYSQL *ctSql, short keyNo, void *outRecPtr)
{
   if (!ctSql->tblState || !ctSql->tblState[ctSql->curTblStateIdx].pageData)
      return (-1);
   
   if ((ctSql->tblState[ctSql->curTblStateIdx].pageIndex < 0) ||
       (ctSql->tblState[ctSql->curTblStateIdx].tsKeyNo != keyNo))
      return (-1);
   
   ctSql->tblState[ctSql->curTblStateIdx].pageIndex--;
   
   return (id_SQLFetchFromTableStateBuffer(ctSql, outRecPtr));
}

#pragma mark -

// ...................................................................... id_SQLSetupStandardKeyAndSegments

static void  id_SQLSetupStandardKeyAndSegments (
 ID_CT_MYSQL  *ctSql,
 short         fileNo,
 short        *keyNo,
 short        *numSegWhere,
 short        *numSegOrdBy
)
{
   if (ctSql->ifilPtr)  {
      *keyNo       = fileNo - ctSql->ifilPtr->tfilno;
      *numSegWhere = *numSegOrdBy = id_SQLGetKeySegments (ctSql, *keyNo);
   }
   else
      *keyNo = *numSegWhere = *numSegOrdBy = 0;
}

static void  id_SQLSetupFirstKeyAndSegments (
 ID_CT_MYSQL  *ctSql,
 short         fileNo,
 short        *keyNo,
 short        *numSegWhere,
 short        *numSegOrdBy
)
{
   // *keyNo should be 1
   
   id_SQLSetupStandardKeyAndSegments (ctSql, ctSql->ifilPtr ? ctSql->ifilPtr->tfilno + 1 : fileNo, keyNo, numSegWhere, numSegOrdBy);
}

static void  id_SQLSetupOnlyKeyAndOrderBySegments (
 ID_CT_MYSQL  *ctSql,
 short         fileNo,
 short        *keyNo,
 short        *numSegWhere,
 short        *numSegOrdBy
)
{
   id_SQLSetupStandardKeyAndSegments (ctSql, fileNo, keyNo, numSegWhere, numSegOrdBy);
   
   *numSegWhere = 0;
}

static void  id_SQLSetupStandardKeyOnly (
 ID_CT_MYSQL  *ctSql,
 short         fileNo,
 short        *keyNo
)
{
   if (ctSql->ifilPtr)
      *keyNo = fileNo - ctSql->ifilPtr->tfilno;
   else
      *keyNo = 0;
}

#pragma mark -

// .........................................................................................................
// ......................................................................................... SQLSelectParams
// .........................................................................................................

static void  id_SQLInitBasicSelectParams (
 ID_CT_MYSQL      *ctSql,
 SQLSelectParams  *selParams,
 short             flags,
 short             keyNo,
 short             numSegWhere,
 short             numSegOrdBy,
 short             offset,
 short             limit
)
{
   id_SetBlockToZeros (selParams, sizeof(SQLSelectParams));
   
   selParams->keyNo = keyNo;
   
   selParams->orderByKeySegs = numSegOrdBy;  // ORDER BY
   selParams->whereKeySegs   = numSegWhere;  // WHERE
   
   selParams->setKeySegs     = 0;            // FRSSET/LSTSET

   selParams->limitCnt = ((limit > 0) && (limit <= kMYSQLBufferedRows)) ? limit : 1;              // LIMIT
   selParams->offsetCnt = offset;            // OFFSET

   selParams->selectIdOnly = FALSE;          // Well, in case

   if (keyNo && numSegWhere > 1)
      flags |= kWhereFlagConcat;
   
   if (!ctSql->ifilPtr)  {
      flags |= kWhereFlagRecordNameOnly;
      flags &= ~(kWhereFlagNoWhere);
   }
   
   selParams->whereFlags = flags;
}

static void  id_SQLInitSetSelectParams (
 ID_CT_MYSQL      *ctSql,
 SQLSelectParams  *selParams,
 short             setSize     // Zero for NXTSET/PRVSET, -1 for JOIN
)
{
   short  partialKey = FALSE;
   
   if (setSize < 0)  {
      ctSql->setSize = selParams->setKeySegs = 0;
   }
   else  {
      if (setSize)  {
         ctSql->setSize = setSize;                // Initially, starting a set, if param is zero, ctSql->setSize is fixed in RecallBackTableState
         id_SQLDisposeJoinParamsBuffers(ctSql);
      }
      
      selParams->setKeySegs = ct_GetKeySegmentsToSize (ctSql->ifilPtr, selParams->keyNo, ctSql->setSize, &partialKey);  // FRSSET/LSTSET
      
      if (partialKey)  {
         if (selParams->setKeySegs == 1)
            selParams->whereFlags |= kWhereFlagLike;
         // con_printf ("Partial key: %hd, table: %s\n", selParams->keyNo, ctSql->tableName);
      }
   }   
   // whereKeySegs, orderByKeySegs already set in id_SQLInitBasicSelectParams()
   
   if (setSize &&
       (!(selParams->whereFlags & kWhereFlagGreaterThan) && !(selParams->whereFlags & kWhereFlagLessThan)))  {
      // Starting...
      selParams->whereKeySegs = selParams->setKeySegs;
      selParams->setKeySegs = 0;
      
      selParams->whereFlags &= ~kWhereFlagConcat;
      
      // if (ctSql->ifilPtr->ix[selParams->keyNo-1].ikeydup)  // Ak je ikeydup we need id at the end
      //    selParams->orderByKeySegs++;
   }
   else  if (!setSize)  {
      // NXTSET/PRVSET
      if (ctSql->joinParams.jpJoinString && ctSql->joinParams.jpRecPreffix)  {
         selParams->recPreffix = ctSql->joinParams.jpRecPreffix;
         selParams->joinSqlStr = ctSql->joinParams.jpJoinString;
         
         selParams->distinctFlag = TRUE;              // For FRSJOIN() it was set in id_SQLAllocateJoinParamsBuffers()
         selParams->limitCnt = kMYSQLBufferedRows;
         selParams->offsetCnt = ctSql->joinParams.savedOffset + ctSql->joinParams.savedLimit;
         
         ctSql->joinParams.savedLimit = selParams->limitCnt;
         ctSql->joinParams.savedOffset = selParams->offsetCnt;

         selParams->whereKeySegs = 0;
      }
   }
}

static void  id_SQLInitKeydupSelectParams (
 ID_CT_MYSQL      *ctSql,
 SQLSelectParams  *selParams,
 short             startFlowFlag  // FRSSET/FRSERC etc. - FALSE for NXT/PRV
)
{
   if (ctSql->ifilPtr && selParams->keyNo && ctSql->ifilPtr->ix[selParams->keyNo-1].ikeydup)  {  // Ak je ikeydup we need id at the end
      if (startFlowFlag)  {
         selParams->orderByKeySegs++;
      }
      else  {
         selParams->whereKeySegs++;
         selParams->orderByKeySegs++;
         
         selParams->whereFlags |= kWhereFlagNeedsId;
      }
   }
}

#pragma mark -

static int  id_SQLSaveSetsCurrentRecord (
 ID_CT_MYSQL  *ctSql,
 short         setIndex
)
{
   if ((setIndex < 0) || (setIndex >= kStandardSelSets))
      return (-1);
   
   if (!ctSql->savedCurrentRecord[setIndex])
      ctSql->savedCurrentRecord[setIndex] = id_malloc_clear_or_exit (ctSql->recSizeWithId);
   
   BlockMove (ctSql->currentRecord, ctSql->savedCurrentRecord[setIndex], ctSql->recSizeWithId);
   
   ctSql->savedCurrentRecFlags[setIndex] = ctSql->currentRecFlags;
   ctSql->savedCurrentSetSize[setIndex] = ctSql->setSize;
   
   return (0);
}

static int  id_SQLRestoreSetsCurrentRecord (
 ID_CT_MYSQL  *ctSql,
 short         setIndex
)
{
   if ((setIndex < 0) || (setIndex >= kStandardSelSets) || !ctSql->savedCurrentRecord[setIndex])  {
      if (!ctSql->savedCurrentRecord[setIndex])
         ctSql->savedCurrentRecord[setIndex] = id_malloc_clear_or_exit (ctSql->recSizeWithId);
         
      ctSql->currentRecFlags = kCurrInvalid;
      return (-1);
   }
   
   BlockMove (ctSql->savedCurrentRecord[setIndex], ctSql->currentRecord, ctSql->recSizeWithId);
   
   ctSql->currentRecFlags = ctSql->savedCurrentRecFlags[setIndex];
   ctSql->setSize         = ctSql->savedCurrentSetSize[setIndex];
   
   return (0);
}

static int  id_SQLDisposeSetsCurrentRecord (
 ID_CT_MYSQL  *ctSql,
 short         setIndex
)
{
   if ((setIndex < 0) || (setIndex >= kStandardSelSets) || !ctSql->savedCurrentRecord[setIndex])  {
      return (-1);
   }
   
   if (setIndex && (setIndex == ctSql->curTblStateIdx))  {
      id_SQLRestoreSetsCurrentRecord (ctSql, ctSql->curTblStateIdx = 0);
      /// Ovdje se treba disposati i sve ostalo na tom indexu i vratiti stanje kakvo je bilo na setu 0 npr
   }
   
   id_DisposePtr ((Ptr)ctSql->savedCurrentRecord[setIndex]);
   
   ctSql->savedCurrentRecord[setIndex] = NULL;
   
   ctSql->ctSetIds[setIndex] = 0;
   
   return (0);
}

static int  id_SQLCheckCurrentSet (
 ID_CT_MYSQL  *ctSql,
 short         createNew
)
{
   short  i, setIndex = -1;
   
   if (sqlHandler->currentSetId == ctSql->ctSetIds[ctSql->curTblStateIdx])  {
      // con_printf ("[%s] ChgSet just fine set: %hd\n", ctSql->tableName, sqlHandler->currentSetId);
      return (0);
   }
   // else
   //    con_printf ("[%s] ChgSet we need set: %hd but at index: %hd we have set: %hd\n",
   //                ctSql->tableName, sqlHandler->currentSetId, ctSql->curTblStateIdx, ctSql->ctSetIds[ctSql->curTblStateIdx]);
   
   for (i=0; i<kStandardSelSets; i++)
      if (ctSql->ctSetIds[i] == sqlHandler->currentSetId)  {
         setIndex = i;
         // con_printf ("[%s] ChgSet found index: %hd for set: %hd\n", ctSql->tableName, setIndex, sqlHandler->currentSetId);
         break;
      }

   if ((setIndex < 0) && createNew)
      for (i=1; i<kStandardSelSets; i++)
         if (!ctSql->ctSetIds[i])  {
            ctSql->ctSetIds[i] = sqlHandler->currentSetId;
            setIndex = i;
            // con_printf ("[%s] ChgSet new index: %hd for set: %hd\n", ctSql->tableName, setIndex, sqlHandler->currentSetId);
            break;
         }

   if (setIndex >= 0)  {  // found existing
      // con_printf ("[%s] ChgSet from index: %hd to index: %hd\n", ctSql->tableName, ctSql->curTblStateIdx, setIndex);
      id_SQLSaveSetsCurrentRecord (ctSql, ctSql->curTblStateIdx);
      id_SQLRestoreSetsCurrentRecord (ctSql, ctSql->curTblStateIdx = setIndex);
      return (0);
   }
   
   return (-1);
}

#pragma mark -

int  id_SQLctChangeSet (short setId)  // Use when on pr_ChgSet
{
   // con_printf ("ChangeSet: %hd\n", setId);
   
   if (sqlHandler)
      sqlHandler->currentSetId = setId;
   
   return (setId);
}

int  id_SQLctFreeSet (short setId)  // Use on pr_FreeSet
{
   short          setIndex = 0L;
   ID_CT_MYSQL  *ctSql = id_SQLFindBySet (setId, &setIndex);
   
   if (ctSql)
      id_SQLDisposeSetsCurrentRecord (ctSql, setIndex);
   
   return (0);
}

// ---------------------------------------------------------------------
#pragma mark -
#pragma mark Fake c-tree Functions
#pragma mark -
// ---------------------------------------------------------------------

// #### ct_SqlStringReportingOn() & ct_SqlStringReportingOff() ####

// Used in ct_OpenOneFile(), open existing, maybe add a param we need aux database or something, or use sqlHandler->activeDatabase

// When used for aux database, fileIndex should be -1 and it will find its description within already opened files/tables

// ¥¥¥¥¥ URGENT! IMPORTANT! IMEDIATE ATTENTION NEEDED!

// MAH MAH -> fldInfo mora bit param jer kad je aux MySQL a glavna baza je ctree ili obrnuto onda id_SQLFindRfiByTableName() ne vrijedi.

// Isto, need a way to have read only or read/update (no insert) versions with only subsets of complete tables

int  id_SQLctOPNIFIL (short fileIndex, IFIL *ifilPtr, Boolean *createdNew)  // If NULL, don't create
{
   long           columnCount = 0L, indexCount = 0L;
   short          chgCount = 0, failFlag = TRUE;
   // short         *lastDatNoPtr = ifilPtr ? &sqlHandler->lastDatNo : &sqlHandler->lastFlatDatNo;
   RecFieldInfo  *fldInfo = NULL;
   ID_CT_MYSQL   *ctSql = NULL;  // id_SQLFindFileNo (fileNo);
   
   // id_InitSQLStorage ();
   
   if (createdNew)
      *createdNew = FALSE;
   
   /*if ((*lastDatNoPtr) != id_SQLAvailableDatNo(ifilPtr ? FALSE : TRUE))
      con_printf ("sqlHandler->lastDatNo %hd vs %hd unexpected\n", *lastDatNoPtr, id_SQLAvailableDatNo(ifilPtr ? FALSE : TRUE));*/
   
   if (fileIndex >= 0)
      fldInfo = id_FindRecFieldInfo (NULL, NULL, NULL, fileIndex, 0);
   else  if (ifilPtr && ifilPtr->dbVRefNum)
      fldInfo = id_SQLFindRfiByTableName (ifilPtr->pfilnam);

   if (fldInfo)  {
      if (!ifilPtr)  {
         // sqlHandler->lastFlatDatNo++;
         
         ctSql = id_SQLSetupOneTable (fldInfo, NULL, 0L);

         // sqlHandler->lastFlatDatNo += ctSql->dNumIdx;  // Za next file
      }
      else  {
         if (!ifilPtr->tfilno)  {
            ifilPtr->tfilno = id_SQLAvailableDatNo (FALSE);  /*++sqlHandler->lastDatNo;
            sqlHandler->lastDatNo += ifilPtr->dnumidx;  // Za next file*/
         }
         // else  if ((ifilPtr->tfilno != datNo) && (ifilPtr->tfilno != (datNo-kMYSQLFileDatNoStart-1)))
         //    con_printf ("File's %s tfilno %hd unexpected\n", ifilPtr->pfilnam, ifilPtr->tfilno);
         
         ctSql = id_SQLSetupOneTable (fldInfo, ifilPtr, 0L);
      }
      if (fileIndex >= 0)
         ctSql->fileIndex = fileIndex;
   }
   else  {
      con_printf ("File %s has no RecFieldInfo!!!\n", ifilPtr ? ifilPtr->pfilnam : "Arch Stanton");
      return (-1);
   }
   
   if (!id_SQLCheckTableCondition(ctSql))
      failFlag = FALSE;
   else  if (!id_SQLCheckTableExist(NULL, ctSql->tableName))  {
      if (!id_SQLCountOfColumnsInTable(NULL, ctSql->tableName, &columnCount) && !id_SQLCountOfIndexesInTable(NULL, ctSql->tableName, &indexCount))  {
         if ((columnCount != ctSql->recSqlFldCount + 1) || (indexCount != id_SQLGetNumberOfIndexes(ctSql)))  {
            if (!id_SQLAlterTable(ctSql, &chgCount))
               con_printf ("Altered table %s, changed %hd columns.\n", ctSql->tableName, chgCount);
         }
         else
            con_printf ("Table %s is fine, has %hd columns as expected.\n", ctSql->tableName, columnCount);
      }
      else
         con_printf ("Table %s is not OK. Problem finding out number of columns.\n", ctSql->tableName, columnCount);
      failFlag = FALSE;
   }
   else  if (createdNew && !id_SQLCreateTable(ctSql, NULL))  {
      *createdNew = TRUE;
      con_printf ("Table created %s\n\n", ctSql->tableName);
      failFlag = FALSE;
   }

   if (!failFlag)  {
      (*sqlHandler->dbs->activeCounter)++;
      return (0);
   }
   
   return (-1);
}

/* ......................................................... id_SQLctCLIFIL ......... */

int  id_SQLctCLIFIL (IFIL *ifilPtr)
{
   CTTPtr        theCTT;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (ifilPtr->tfilno);
   
   if (ctSql)  {

      if (theCTT = id_CTTFindByDatNo(ifilPtr->tfilno))
         id_CTTRelease (theCTT);
      id_SQLDisposeOneTable (ctSql);
      
      /*if (ifilPtr)
         sqlHandler->lastDatNo = id_SQLAvailableDatNo (FALSE);
      else
         sqlHandler->lastFlatDatNo = id_SQLAvailableDatNo (TRUE);*/
      
      return (0);
   }
   
   return (-1);
}

/* ......................................................... id_SQLctCLISAM ......... */

// Need close for only one table or group - kMainDatabase, kCommonDatabase or kAuxilaryDatabase or one dbHandle when I have both active

void  id_SQLctCLISAM (void)  // There is id_SQLCloseDatabase()
{
   short         i, six;
   ID_CT_MYSQL  *ctSql = NULL;
   
   while (id_pointerElemsCount(&sqlHandler->dbSqlInfos))  {
      
      ctSql = id_pointerElem (&sqlHandler->dbSqlInfos, 0);
      
      if (ctSql)  {
         if (ctSql->ifilPtr)
            id_SQLctCLIFIL (ctSql->ifilPtr);
         else
            id_SQLDisposeOneTable (ctSql);
      }
   }
   
   if (sqlHandler->dbs->mainTableCounter)
      con_printf ("sqlHandler->dbs->mainTableCounter: %hd\n", sqlHandler->dbs->mainTableCounter);
   if (sqlHandler->dbs->commonTableCounter)
      con_printf ("sqlHandler->dbs->mainTableCounter: %hd\n", sqlHandler->dbs->commonTableCounter);
   if (sqlHandler->dbs->auxTableCounter)
      con_printf ("sqlHandler->dbs->mainTableCounter: %hd\n", sqlHandler->dbs->auxTableCounter);
}

// Wee need:
// CHGSET (0..16);
// FRESET (); -> kill all sets
// Nekaj za pr_FreeSet() funkciju - vidi tko koristi taj set pa oslobodi memoriju
// kStandardSelSets, ctSql->ctSetIds[1..kStandardSelSets], ctSql->tblState[ctSql->curTblStateIdx]
// we need current Record for each set, ha ha, pa onda context switch u CHGSET(), maybe FRESET() too

// Use  id_SQLFindBySet (short setId, short *setIndex);

// Use  id_SQLSaveSetsCurrentRecord (ID_CT_MYSQL *ctSql, short setIndex);
//      id_SQLRestoreSetsCurrentRecord (ID_CT_MYSQL *ctSql, short setIndex);
//      id_SQLDisposeSetsCurrentRecord (ID_CT_MYSQL *ctSql, short setIndex);


// OK, za CHGSET:
// Postavi tu neki globalni set na taj index: sqlHandler->currentSetId
// Prvi FRSSET ili LSTSET pogleda koji je active set i usporedi jel isti ctSql->ctSetIds[ctSql->curTblStateIdx]
// Ako nije, ajoj... problemi!

// Ok, problemi:
// Find slot koji ima taj id, ako da - SUPER!
// Copy currentRecord u ctSql->savedCurrentRecord[ctSql->curTblStateIdx]
// Copy currentRecFlags u ctSql->savedCurrentRecFlags[ctSql->curTblStateIdx]
// postavi ctSql->curTblStateIdx na novi slot
// Copy taj ctSql->savedCurrentRecord[ctSql->curTblStateIdx] u currentRecord

// OK, ako no find same:
// Find prvi slobodan slot, ctSql->ctSetIds[>0] koji je 0
// Alloc+Copy currentRecord u ctSql->savedCurrentRecord[ctSql->curTblStateIdx]
// postavi ctSql->curTblStateIdx na novi slot

// Za NXT/PRVSET mora postojati slot od ranije, ako ne - error

// 31.03.2021 - SHOWSTOPPER - what about tblState?


long  id_SQLctDATENT (short fileNo)
{
   long          recCount = 0L;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   if (ctSql)
      id_SQLCountOfRecordsInTable(ctSql->dbName, ctSql->tableName, &recCount);

   return (recCount);
}

// Maybe I need to copy resulting rec so I can create the key next time I call NXT or PRV

int  id_SQLctADDREC (short fileNo, void *recPtr)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   long          retRowId;
   // CTTPtr        theCTT = id_CTTFindByDatNo (datNo);
   // ID_CT_MYSQL  *ctSql = theCTT->ctSqlPtr;
   
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   id_SQLSetupFirstKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);
   
   /*if (ctSql->ifilPtr)  {
      keyNo       = 1;  // Or zero?
      numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);
   }
   else
      keyNo = numSegWhere = numSegOrdBy = 0;*/
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLAddCTreeObject (ctSql, recPtr, &retRowId);
      
   return (err);
}

int  id_SQLctADDVREC (short fileNo, void *recPtr, long fullrecSize)
{
   long          extraSize = 0L;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   if (ctSql)  {
      
      extraSize = fullrecSize - ctSql->recSize;
      
      if (extraSize > 0L)  {
         id_SQLAllocVarLenBuffer (ctSql, extraSize);  // Allocates if necessary
         
         BlockMove ((char *)recPtr+ctSql->recSize, ctSql->varLenBuffPtr, extraSize);

         return (id_SQLctADDREC(fileNo, recPtr));
      }
   }
   
   return (-1);
}

// RWTREC i DELREC moraju izgubiti oldRec param i onda se koristi current ako currentRecFlags ima kCurrValid4Start
// see ICUR_ERR

int  id_SQLctRWTREC (short fileNo, void *newRecPtr)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   long          retRowId;
   void         *oldRecPtr = NULL;
   // CTTPtr        theCTT = id_CTTFindByDatNo (datNo);
   // ID_CT_MYSQL  *ctSql = theCTT->ctSqlPtr;
   
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   if (ctSql->currentRecFlags & (kCurrValid4ReRead | kCurrValid4Start))
      oldRecPtr = ctSql->currentRecord;
   else
      return (-1);
   
   /*if (TRUE)
   {
      char  docSignature[256];
      (*sqlHandler->recSigProcPtr) (ctSql->rfi->iRecId, oldRecPtr, docSignature, 63);
      con_printf ("Old: %s\n", docSignature);
      (*sqlHandler->recSigProcPtr) (ctSql->rfi->iRecId, newRecPtr, docSignature, 63);
      con_printf ("New: %s\n", docSignature);
   }*/

   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   id_SQLSetupFirstKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);

   /*if (ctSql->ifilPtr)  {
      keyNo       = 1;  // Or zero?
      numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);
   }
   else
      keyNo = numSegWhere = numSegOrdBy = 0;*/

   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLRewriteCTreeObject (ctSql, oldRecPtr, newRecPtr);   
   
   /*if (!err)  {
      BlockMove (newRecPtr, ctSql->currentRecord, ctSql->recSize);
      id_SQLConvertRecordEncoding (ctSql, ctSql->currentRecord, FALSE);
      
      ctSql->currentRecFlags = kCurrValid4ReRead | kCurrValid4Flow;
   }*/

   return (err);
}

int  id_SQLctRWTVREC (short fileNo, void *newRecPtr, long fullrecSize)
{
   long          extraSize = 0L;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   if (ctSql)  {
      
      extraSize = fullrecSize - ctSql->recSize;
      
      if (extraSize > 0L)  {
         id_SQLAllocVarLenBuffer (ctSql, extraSize);  // Allocates if necessary
         
         BlockMove ((char *)newRecPtr+ctSql->recSize, ctSql->varLenBuffPtr, extraSize);
         
         return (id_SQLctRWTREC(fileNo, newRecPtr));
      }
   }
   
   return (-1);
}

// Problemi nakon brisanja:
// - currentRecord - moramo imati oznaku da je currentRecord invalid, ali nam treba za pronalazenje nxt i prv
// - ctSql->tblState[ctSql->curTblStateIdx].pageData - moramo imati flag da je rec u bufferu invalid pa idemo na iduci
// see ICUR_ERR

int  id_SQLctDELREC (short fileNo)
{
   short         keyNo, err = 101, numSegWhere, numSegOrdBy;
   long          retRowId;
   void         *recPtr = NULL;
   // CTTPtr        theCTT = id_CTTFindByDatNo (datNo);
   // ID_CT_MYSQL  *ctSql = theCTT->ctSqlPtr;
   
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   if (ctSql->currentRecFlags & (kCurrValid4ReRead | kCurrValid4Start))
      recPtr = ctSql->currentRecord;
   else
      return (-1);

   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   id_SQLSetupFirstKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);

   /*keyNo       = 1;  // Or zero?
   numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);*/
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLDeleteCTreeObject (ctSql, recPtr);
   
   if (!err)  {
      BlockMove (recPtr, ctSql->currentRecord, ctSql->recSize);
      
      ctSql->currentRecFlags = kCurrValid4Flow;
   }
   
   return (err);
}

int  id_SQLctRRDREC (short fileNo, void *outRecPtr)
{
   short         err = ICUR_ERR;  // Find more appropriate error
   // short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   // SQLSelectParams  selParams;
   
   if (ctSql && (ctSql->currentRecFlags & kCurrValid4ReRead))  {
      // BlockMove (ctSql->currentRecord, outRecPtr, ctSql->recSize);

      err = id_SQLctEQLREC (fileNo+1, ctSql->currentRecord, outRecPtr);  // One day make sure it's a unique key

      // if (!err)
      //    ctSql->currentRecFlags = kCurrValid4Start | kCurrValid4Flow | kCurrValid4ReRead;  - handled inside id_SQLctEQLREC
   }
   else
      con_printf ("Not valid for ReRead!\n");
   
   return (err);
}

int  id_SQLctREDVREC (short fileNo, void *outRecPtr, long fullrecSize)
{
   short         err = 101;  // Find more appropriate error
   long          extraSize = 0L;
   // short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   // SQLSelectParams  selParams;
   
   if (ctSql && (ctSql->currentRecFlags & kCurrValid4ReRead))  {
      // BlockMove (ctSql->currentRecord, outRecPtr, ctSql->recSize);
      
      extraSize = fullrecSize - ctSql->recSize;

      BlockMove (ctSql->currentRecord, outRecPtr, ctSql->recSize);
      if (ctSql->varLenBuffPtr)  {
         BlockMove (ctSql->varLenBuffPtr, (char *)outRecPtr+ctSql->recSize, extraSize);
         err = 0;
      }
   }
   
   return (err);
}

int  id_SQLctEQLREC (short fileNo, void *inRecPtr, void *outRecPtr)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);

   SQLSelectParams  selParams;
   
   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   id_SQLSetupStandardKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);
   
   /*if (ctSql->ifilPtr)  {
      keyNo       = fileNo - ctSql->ifilPtr->tfilno;
      numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);
   }
   else
      keyNo = numSegWhere = numSegOrdBy = 0;*/
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, inRecPtr, outRecPtr);
   
   return (err);
}

int  id_SQLctFRSREC (short fileNo, void *outRecPtr)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   id_SQLSetupOnlyKeyAndOrderBySegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);

   /*keyNo       = fileNo - ctSql->ifilPtr->tfilno;
   numSegWhere = 0;
   numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);*/  // If zero, ordered by id
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagNoWhere, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLInitKeydupSelectParams (ctSql, &selParams, TRUE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV
   
   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, NULL, outRecPtr);
   
   return (err);
}

// SELECT max(CONCAT(LPAD(CAST(k_kupdob_cd as char),10,' '),k_kupdob)) FROM AdresarA;
// SELECT max(CONCAT(LPAD(CAST(CAST(k_kupdob_cd AS UNSIGNED) as char),10,' '),k_kupdob)) FROM AdresarA;

int  id_SQLctLSTREC (short fileNo, void *outRecPtr)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   id_SQLSetupOnlyKeyAndOrderBySegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);

   /*keyNo       = fileNo - ctSql->ifilPtr->tfilno;
   numSegWhere = 0;
   numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);*/  // If zero, ordered by id
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagNoWhere | kWhereFlagDescending, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt

   id_SQLInitKeydupSelectParams (ctSql, &selParams, TRUE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, NULL, outRecPtr);
   
   return (err);
}

// We need a flag that leaves currentRecord intact if SELECT fails

int  id_SQLctNXTREC (short fileNo, void *outRecPtr)
{
   short         keyNo, err = 101;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   keyNo = fileNo - ctSql->ifilPtr->tfilno;

   if (!id_SQLNextFromTableStateBuffer(ctSql, keyNo, outRecPtr))
      err = 0;
   else  {
      if (ctSql->currentRecFlags & kCurrValid4Flow)  {

         if (!id_SQLRecallBackTableState (ctSql, &selParams, fileNo, kWhereFlagGreaterThan | kWhereFlagConcat, FALSE))  {
            id_SQLInitKeydupSelectParams (ctSql, &selParams, FALSE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

            err = id_SQLSelectCTreeObject (ctSql, &selParams, ctSql->currentRecord, outRecPtr);
         }
      }
   }
   
   return (err);
}

// We need a flag that leaves currentRecord intact if SELECT fails

int  id_SQLctPRVREC (short fileNo, void *outRecPtr)
{
   short         keyNo, err = 101;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   keyNo = fileNo - ctSql->ifilPtr->tfilno;

   if (!id_SQLPrevFromTableStateBuffer(ctSql, keyNo, outRecPtr))
      err = 0;
   else  {
      if (ctSql->currentRecFlags & kCurrValid4Flow)  {
         if (!id_SQLRecallBackTableState (ctSql, &selParams, fileNo, kWhereFlagLessThan | kWhereFlagConcat, FALSE))  {
            id_SQLInitKeydupSelectParams (ctSql, &selParams, FALSE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

            err = id_SQLSelectCTreeObject (ctSql, &selParams, ctSql->currentRecord, outRecPtr);
         }
      }
   }
   
   return (err);
}

int  id_SQLctGTEREC (short fileNo, void *inRecPtr, void *outRecPtr)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   id_SQLSetupStandardKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);

   /*keyNo       = fileNo - ctSql->ifilPtr->tfilno;
   numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);*/
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagGreatOrEQ, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLInitKeydupSelectParams (ctSql, &selParams, TRUE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, inRecPtr, outRecPtr);
   
   return (err);
}

int  id_SQLctLTEREC (short fileNo, void *inRecPtr, void *outRecPtr)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   id_SQLSetupStandardKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);

   /*keyNo       = fileNo - ctSql->ifilPtr->tfilno;
   numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);*/
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagLessOrEQ, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLInitKeydupSelectParams (ctSql, &selParams, TRUE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, inRecPtr, outRecPtr);
   
   return (err);
}

int  id_SQLctFRSSET (short fileNo, void *inRecPtr, void *outRecPtr, short setSize, short recLimit)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   // char         *internalRecPtr = ctSql->currentRecord + ctSql->recSize;
   
   SQLSelectParams  selParams;
   
   id_SQLSetupStandardKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);

   /*keyNo       = fileNo - ctSql->ifilPtr->tfilno;
   numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);*/  // If zero, ordered by id
   
   id_SQLCheckCurrentSet (ctSql, TRUE);
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual, keyNo, numSegWhere, numSegOrdBy, 0, recLimit);  // off,lmt
   
   id_SQLInitSetSelectParams (ctSql, &selParams, setSize);
   
   id_SQLInitKeydupSelectParams (ctSql, &selParams, TRUE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, inRecPtr, outRecPtr);

   // if (!err)
   //    con_printf ("FRSSET found id: %ld\n", *((long *)internalRecPtr));
   
   return (err);
}

int  id_SQLctLSTSET (short fileNo, void *inRecPtr, void *outRecPtr, short setSize, short recLimit)
{
   short         keyNo, err = 101, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   id_SQLSetupStandardKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);

   /*keyNo       = fileNo - ctSql->ifilPtr->tfilno;
   numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);*/  // If zero, ordered by id
   
   id_SQLCheckCurrentSet (ctSql, TRUE);

   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual | kWhereFlagDescending, keyNo, numSegWhere, numSegOrdBy, 0, recLimit);  // off,lmt
   
   id_SQLInitSetSelectParams (ctSql, &selParams, setSize);
   
   id_SQLInitKeydupSelectParams (ctSql, &selParams, TRUE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, inRecPtr, outRecPtr);
   
   return (err);
}

int  id_SQLctFRSJOIN (short fileNo, char *inRecPreffix, char *joinSqlStr, void *outRecPtr)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   id_SQLSetupStandardKeyAndSegments (ctSql, fileNo, &keyNo, &numSegWhere, &numSegOrdBy);
   id_SQLCheckCurrentSet (ctSql, TRUE);
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual, keyNo, 0/*numSegWhere*/, numSegOrdBy, 0, 0);  // off,lmt
   id_SQLInitSetSelectParams (ctSql, &selParams, -1);  // setSize = -1
   
   id_SQLAllocateJoinParamsBuffers (ctSql, &selParams, inRecPreffix, joinSqlStr);  // This is special
   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, NULL, outRecPtr);

   // if (!err)
   //    con_printf ("FRSSET found id: %ld\n", *((long *)internalRecPtr));
   
   return (err);
}

// We need a flag that leaves currentRecord intact if SELECT fails

int  id_SQLctNXTSET (short fileNo, void *outRecPtr)
{
   short         err = 101;
   short         keyNo /*, numSegWhere, numSegOrdBy*/;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;

   char         *internalRecPtr = ctSql->currentRecord + ctSql->recSize;

   id_SQLSetupStandardKeyOnly (ctSql, fileNo, &keyNo);
   
   // keyNo       = fileNo - ctSql->ifilPtr->tfilno;
   
   id_SQLCheckCurrentSet (ctSql, FALSE);

   if (!id_SQLNextFromTableStateBuffer(ctSql, keyNo, /*&selParams,*/ outRecPtr))
      err = 0;
   else  {
      if (ctSql->currentRecFlags & kCurrValid4Flow)  {
         if (!id_SQLRecallBackTableState (ctSql, &selParams, fileNo, kWhereFlagGreaterThan | kWhereFlagConcat, TRUE))  {
      
            id_SQLInitSetSelectParams (ctSql, &selParams, 0);  // Zero as setSize
      
            id_SQLInitKeydupSelectParams (ctSql, &selParams, FALSE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

            // con_printf ("NXTSET searching from id: %ld\n", *((long *)internalRecPtr));

            err = id_SQLSelectCTreeObject (ctSql, &selParams, ctSql->currentRecord, outRecPtr);
         }
      }
   }
   
   return (err);
}

// We need a flag that leaves currentRecord intact if SELECT fails

int  id_SQLctPRVSET (short fileNo, void *outRecPtr)
{
   short         err = 101;
   short         keyNo /*, numSegWhere, numSegOrdBy*/;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   id_SQLSetupStandardKeyOnly (ctSql, fileNo, &keyNo);

   // keyNo       = fileNo - ctSql->ifilPtr->tfilno;
   
   // id_SQLInitBasicSelectParams (&selParams, kWhereFlagEqual, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLCheckCurrentSet (ctSql, FALSE);

   if (!id_SQLPrevFromTableStateBuffer(ctSql, keyNo, /*&selParams,*/ outRecPtr))
      err = 0;
   else  {
      if (ctSql->currentRecFlags & kCurrValid4Flow)  {
         if (!id_SQLRecallBackTableState (ctSql, &selParams, fileNo, kWhereFlagLessThan | kWhereFlagConcat, TRUE))  {
       
            id_SQLInitSetSelectParams (ctSql, &selParams, 0);  // Zero as setSize
      
            id_SQLInitKeydupSelectParams (ctSql, &selParams, FALSE);  // FRSSET/FRSERC etc. - FALSE for NXT/PRV

            err = id_SQLSelectCTreeObject (ctSql, &selParams, ctSql->currentRecord, outRecPtr);
         }
      }
   }
   
   return (err);
}

// -----------------------------------------------------------------------------------------------------------

long  id_SQLctGETCURP (short fileNo)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   long          savedId = 0L;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   if (ctSql && (ctSql->currentRecFlags & kCurrValid4ReRead))  {
      savedId = *((long *)(ctSql->currentRecord + ctSql->recSize));

      return (savedId);
#ifdef _NIJE_
      ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
      
      keyNo       = 1;  /*fileNo - ctSql->ifilPtr->tfilno;*/
      numSegWhere = numSegOrdBy = id_SQLGetKeySegments (ctSql, keyNo);
      
      id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual /*| kWhereFlagSelectIdOnly*/, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
      
      id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
      
      selParams.selectIdOnly = TRUE;  // The whole point
      
      savedId = *((long *)(ctSql->currentRecord + ctSql->recSize));
      
      err = id_SQLSelectCTreeObject (ctSql, &selParams, ctSql->currentRecord, NULL);
      
      if (!err)  {
         con_printf ("GETCURP: %s (%ld - %ld)\n", (savedId == ctSql->idToBind) ? "YES" : "NO", savedId, ctSql->idToBind);
         return (ctSql->idToBind);
      }
#endif
   }
   
   return (0L);
}

int  id_SQLctSETCURI (short fileNo, long position)
{
   short         keyNo, err, numSegWhere, numSegOrdBy;
   ID_CT_MYSQL  *ctSql = id_SQLFindFileNo (fileNo);
   
   SQLSelectParams  selParams;
   
   ctSql->setSize = 0;  // Or create some initializing method - maybe with CHGSET in mind
   
   keyNo       = 0;  /*fileNo - ctSql->ifilPtr->tfilno;*/
   numSegWhere = numSegOrdBy = 0;
   
   id_SQLInitBasicSelectParams (ctSql, &selParams, kWhereFlagEqual | kWhereFlagWhereIdOnly, keyNo, numSegWhere, numSegOrdBy, 0, 1);  // off,lmt
   
   id_SQLSetupTableState (ctSql, &selParams);  // For NXT, PRC etc.
   
   selParams.idAsParam = position;  // The whole point
   
   err = id_SQLSelectCTreeObject (ctSql, &selParams, NULL, NULL);
   
   return (err);
}

/* ......................................................id_SQLctGETFIL............... */

// Instead of original ct GETFIL

int  id_SQLctGETFIL (short fileNo)
{
   short         i;
   ID_CT_MYSQL  *ctSql = NULL;
   
   for (i=0; i < id_pointerElemsCount(&sqlHandler->dbSqlInfos); i++)  {
      
      ctSql = id_pointerElem (&sqlHandler->dbSqlInfos, i);
      
      if ((fileNo >= ctSql->datNo) && (fileNo <= (ctSql->datNo + ctSql->dNumIdx)))  {
         /*if (fileNo > ctSql->datNo)  {
            if (ctSql->datNo != GETFIL (fileNo, REVMAP))
               con_printf ("GETFIL discrepancy: %hd", fileNo);
         }*/
         return (ctSql->datNo);
      }
   }
   
   return (-1);
}

#pragma mark -

/* ......................................................id_SQLParamBetweenBraces..... */

static char  *id_SQLParamBetweenBraces (char *block, char **retEndPtr)
{
   char  *startPtr = block, *endPtr = NULL;
   
   *retEndPtr = NULL;
   
   if (startPtr = strchr (startPtr, '{'))
      endPtr = strchr (startPtr, '}');
   if (!startPtr || !endPtr)
      return (NULL);
   
   startPtr++;
   *retEndPtr = endPtr+1;
   *endPtr-- = '\0';
   
   while ((*startPtr == ' ') || (*startPtr == '\r') || (*startPtr == '\n'))
      startPtr++;
   while ((*endPtr == ' ') || (*endPtr == '\r') || (*endPtr == '\n'))
      *endPtr-- = '\0';
   
   return (startPtr);
}

/* ......................................................id_SQLParseSetupBuffer....... */

static int  id_SQLParseSetupBuffer (
 char      *buffPtr,        // in
 short      paramsBuffLen,  // in
 char      *foundUser,      // out
 char      *foundPass,      // out
 char      *foundServer,    // out
 char      *foundPort,      // out
 char      *foundDBPrefix,  // out
 char      *foundDBTicker,  // out, optional, may be NULL
 char      *foundDBName     // out, optional, may be NULL
)
{
   short  i, selector = 0, retVal = -1;
   char  *buffStart = buffPtr;
   char  *chPtr = NULL, *tarPtr = NULL;
   char   tmpUser[64], tmpPass[64], tmpServer[64], tmpBuffer[16];
   
   *foundDBPrefix = *foundUser = *foundPass = *foundServer = tmpUser[0] = tmpPass[0] = tmpServer[0] = '\0';
   
   i = 0;
   tarPtr = tmpUser;
   
   while (*buffPtr)  {
      if ((*buffPtr == '\n') || (*buffPtr == '\r'))  {
         *tarPtr = '\0';
         if ((*(buffPtr+1) == '\r') || (*(buffPtr+1) == '\n'))
            *buffPtr++;
         if (!selector)
            tarPtr = tmpPass;
         else  if (selector == 1)
            tarPtr = tmpServer;
         selector++;
         i = 0;
      }
      else  if (++i < 63)
         *tarPtr++ = *buffPtr;
      buffPtr++;
   }
   *tarPtr = '\0';
   
   if ((selector >= 2) && strlen(tmpServer))
      retVal = 0;

   if (!retVal)  {

      // Pero:Bouquet   - Pero is optional, $ or companyShortName - ticker : user
      // ERGO:QWER      - dbPrefix : pass
      // $:10.10.20.217 - optional, $ or dbName : addr
      
      // 1st line:
      strNCpyUntil (foundDBTicker ? foundDBTicker : tmpBuffer, tmpUser, ':', 10, &chPtr);
      if (chPtr)
         strNCpy (foundUser, chPtr, paramsBuffLen);
      else
         retVal = -1;

      // 2nd line:
      strNCpyUntil (foundDBPrefix, tmpPass, ':', 8, &chPtr);
      if (chPtr)
         strNCpy (foundPass, chPtr, paramsBuffLen);
      
      // 3rd line:
      strNCpyUntil (foundDBName ? foundDBName : tmpBuffer, tmpServer, ':', paramsBuffLen, &chPtr);
      if (chPtr)  {
         char  *portPtr = strchr (chPtr, ',');
         if (!portPtr)
            portPtr = strchr (chPtr, ';');
         if (portPtr)  {
            strNCpyUntil (foundServer, chPtr, ',', paramsBuffLen, &portPtr);
            if (foundPort)
               strNCpy (foundPort, portPtr, paramsBuffLen);
         }
         else
            strNCpy (foundServer, chPtr, paramsBuffLen);
      }
      
      if (foundDBTicker && !strcmp(foundDBTicker, "$"))
         foundDBTicker[0] = '\0';
      if (foundDBName && !strcmp(foundDBName, "$"))
         foundDBName[0] = '\0';
      if (foundPass && !strcmp(foundPass, "$"))
         foundPass[0] = '\0';

      /*chPtr = strchr (tmpServer, ':');
      if (chPtr)
         strNCpy (foundServer, chPtr+1, paramsBuffLen);
      else
         retVal = -1;*/
   }
   
   return (retVal);
}

/* ......................................................id_SQLReadSetupFile.......... */

int  id_SQLReadSetupFile (
 short      vRefNum,
 char      *fileName,       // in
 short      paramsBuffLen,  // in
 char      *foundUser,      // out
 char      *foundPass,      // out
 char      *foundServer,    // out
 char      *foundPort,      // out
 char      *foundDBPrefix,  // out
 char      *foundDBTicker,  // out, optional, may be NULL
 char      *foundDBName     // out, optional, may be NULL
)
{
   short  i, selector = 0, retVal = -1;
   char  *chPtr = NULL, *tarPtr = NULL, *buffPtr = NULL, *buffStart = NULL;
   
   buffPtr = buffStart = (char *)id_malloc_or_exit (4096);
   
   *foundDBPrefix = *foundUser = *foundPass = *foundServer = '\0';
   
   if (!id_ReadCompleteFile(vRefNum, fileName, buffPtr, 4095, FALSE))  {
      id_UTF8String2Mac (buffPtr, buffPtr, 4095);
      
      retVal = id_SQLParseSetupBuffer (buffPtr,        // in
                                       paramsBuffLen,  // in
                                       foundUser,      // out
                                       foundPass,      // out
                                       foundServer,    // out
                                       foundPort,      // out
                                       foundDBPrefix,  // out
                                       foundDBTicker,  // out, optional, may be NULL
                                       foundDBName     // out, optional, may be NULL
                                       );
   }
   
   id_DisposePtr (buffStart);
      
   return (retVal);
}

/* ......................................................id_SQLReadSetupList.......... */

int  id_SQLReadSetupList (
 short       vRefNum,
 char       *fileName,     // in
 SQLTarget  *sqlTargets,   // in
 short       maxTargets    // in
)
{
   short  i = 0, selector = 0, retVal = -1;
   char   tmpUser[64], tmpPass[64], tmpServer[64], tmpPort[64], tmpPreffix[64], tmpTicker[64];
   char   databaseName[128], tmpDescription[64];
   char  *startPtr = NULL, *endPtr = NULL, *mainBuffPtr = NULL;
   
   id_SetBlockToZeros (sqlTargets, sizeof(SQLTarget) * maxTargets);

   mainBuffPtr = (char *)id_malloc_or_exit (8192);
   
   if (!id_ReadCompleteFile(vRefNum, fileName, mainBuffPtr, 8191, FALSE))  {
      id_UTF8String2Mac (mainBuffPtr, mainBuffPtr, 8191);
      
      endPtr = mainBuffPtr;
      
      do  {
         startPtr = endPtr;
         
         // Title
         
         if (!(startPtr = id_SQLParamBetweenBraces(startPtr, &endPtr)))
            break;

         strNCpy (tmpDescription, startPtr, 63);  // tmpDescription
         
         // Content

         if (!(startPtr = id_SQLParamBetweenBraces(endPtr, &endPtr)))
            break;

         retVal = id_SQLParseSetupBuffer (startPtr,     // in
                                          64,           // in
                                          tmpUser,      // out
                                          tmpPass,      // out
                                          tmpServer,    // out
                                          tmpPort,      // out
                                          tmpPreffix,   // out
                                          tmpTicker,    // out, optional, may be NULL
                                          NULL          // out, optional, may be NULL
                                         );
         
         if (!retVal)  {
            id_SQLFormDataBaseName (NULL, tmpPreffix, tmpTicker, NULL, 0, databaseName, 127);
            id_SetupSQLTarget (sqlTargets, tmpServer, atol(tmpPort), tmpUser, tmpPass, databaseName);
            strNCpy (sqlTargets->tDescription, tmpDescription, 63);
            sqlTargets++;
         }
         else
            break;
            
      } while (endPtr && (i++ < maxTargets));
   }
   
   id_DisposePtr (mainBuffPtr);
      
   return (retVal);
}

/* ......................................................id_SQLFormDataBaseName....... */

int  id_SQLFormDataBaseName (
 char  *dbBasicName,   // in
 char  *dbPrefix,      // in
 char  *dbCompany,     // in
 char  *dbYear,        // in
 short  yearOffset,    // in
 char  *databaseName,  // out
 short  nameBuffLen    // out
)
{
   short   i, iYear = id_short2Year (id_sys_date());
   unsigned
    short  tmpDate;
   char    strYear[16], *yearToken = "<YYYY>";
   
   if (dbYear)  {
      sprintf (strYear, "0101%.4s", dbYear);
      id_date2Short (strYear, &tmpDate);
      iYear = id_short2Year (tmpDate);
   }
   
   iYear += yearOffset;
   sprintf (strYear, "%hd", iYear);
   
   if (dbBasicName && id_StriStrCro(dbBasicName, yearToken))  {
      id_ReplaceTokenInStr (dbBasicName, yearToken, strYear, databaseName, nameBuffLen);  // Replace <YYYY> with desired year
   }
   else
      snprintf (databaseName, nameBuffLen, "Bouquet_%s_%s_%s", dbPrefix, dbCompany, strYear);
   
   return (0);
}

// Each buffer should be at least 10 char long

int  id_SQLSplitDataBaseName (
 char  *dbFullName,   // in
 char  *dbPrefix,     // out
 char  *dbCompany,    // out
 char  *dbYear        // out
)
{
   char    tmpStr[256], *chPtr1 = NULL, *chPtr2 = NULL, *chPtr3 = NULL;
   
   strNCpy (tmpStr, dbFullName, 255);
   
   if (chPtr1 = strchr(tmpStr, '_'))  {
      *chPtr1++ = '\0';
      if (chPtr2 = strchr(chPtr1, '_'))  {
         *chPtr2++ = '\0';
         if (chPtr3 = strchr(chPtr2, '_'))
            *chPtr3++ = '\0';
      }
   }
   
   if (chPtr1 && chPtr2 && chPtr3)  {
      if (dbPrefix)
         strNCpy (dbPrefix, chPtr1, 9);
      if (dbCompany)
         strNCpy (dbCompany, chPtr2, 9);
      if (dbYear)
         strNCpy (dbYear, chPtr3, 9);

      return (0);
   }
   else  {
      if (dbPrefix)
         *dbPrefix = '\0';
      if (dbCompany)
         *dbCompany = '\0';
      if (dbYear)
         *dbYear = '\0';
   }
   
   return (-1);
}

#pragma mark -

#ifdef  _DTOOL_WIN_

Boolean  id_SQLMySQLAvailable (void)
{
   return (gGhMySQLModule ? TRUE : FALSE);
}

int  id_SQLLoadLibMySQLDll (void)
{
   static short  beenHere = FALSE;
   
   if (!gGhMySQLModule && !beenHere)  {
      beenHere = TRUE;
      gGhMySQLModule = LoadLibrary ("libmysql.dll");                          // 32bit  ("libmysql.dll") ("libmariadb.dll");

      // Try to find our proc
      
      if (gGhMySQLModule)  {
         p_mysql_init         = (idptr_mysql_init)GetProcAddress(gGhMySQLModule, (char *)"mysql_init");            //
         p_mysql_options      = (idptr_mysql_options)GetProcAddress(gGhMySQLModule, (char *)"mysql_options");            //
         p_mysql_real_connect = (idptr_mysql_real_connect)GetProcAddress(gGhMySQLModule, (char *)"mysql_real_connect");  //
         p_mysql_select_db    = (idptr_mysql_select_db)GetProcAddress(gGhMySQLModule, (char *)"mysql_select_db");  //

         p_mysql_server_init  = (idptr_mysql_server_init)GetProcAddress(gGhMySQLModule, (char *)"mysql_server_init");            //
         p_mysql_server_end   = (idptr_mysql_server_end)GetProcAddress(gGhMySQLModule, (char *)"mysql_server_end");  //

         p_mysql_real_query         = (idptr_mysql_real_query)GetProcAddress(gGhMySQLModule, (char *)"mysql_real_query");  //
         p_mysql_character_set_name = (idptr_mysql_character_set_name)GetProcAddress(gGhMySQLModule, (char *)"mysql_character_set_name");  //
         p_mysql_set_character_set  = (idptr_mysql_set_character_set)GetProcAddress(gGhMySQLModule, (char *)"mysql_set_character_set");  //

         p_mysql_num_rows     = (idptr_mysql_num_rows)GetProcAddress(gGhMySQLModule, (char *)"mysql_num_rows");            //
         p_mysql_num_fields   = (idptr_mysql_num_fields)GetProcAddress(gGhMySQLModule, (char *)"mysql_num_fields");  //

         p_mysql_free_result  = (idptr_mysql_free_result)GetProcAddress(gGhMySQLModule, (char *)"mysql_free_result");            //
         p_mysql_data_seek    = (idptr_mysql_data_seek)GetProcAddress(gGhMySQLModule, (char *)"mysql_data_seek");  //
         p_mysql_row_seek     = (idptr_mysql_row_seek)GetProcAddress(gGhMySQLModule, (char *)"mysql_row_seek");            //
         p_mysql_field_seek   = (idptr_mysql_field_seek)GetProcAddress(gGhMySQLModule, (char *)"mysql_field_seek");  //

         p_mysql_errno        = (idptr_mysql_errno)GetProcAddress(gGhMySQLModule, (char *)"mysql_errno");            //
         p_mysql_error        = (idptr_mysql_error)GetProcAddress(gGhMySQLModule, (char *)"mysql_error");  //

         p_mysql_stmt_init    = (idptr_mysql_stmt_init)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_init");  //
         p_mysql_stmt_prepare = (idptr_mysql_stmt_prepare)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_prepare");  //
         p_mysql_stmt_execute = (idptr_mysql_stmt_execute)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_execute");  //
         p_mysql_stmt_fetch   = (idptr_mysql_stmt_fetch)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_fetch");  //

         p_mysql_stmt_fetch_column = (idptr_mysql_stmt_fetch_column)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_fetch_column");  //
         p_mysql_stmt_store_result = (idptr_mysql_stmt_store_result)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_store_result");  //
         p_mysql_stmt_param_count  = (idptr_mysql_stmt_param_count)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_param_count");  //
         p_mysql_stmt_attr_set     = (idptr_mysql_stmt_attr_set)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_attr_set");  //
         p_mysql_stmt_attr_get     = (idptr_mysql_stmt_attr_get)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_attr_get");  //
         p_mysql_stmt_bind_param   = (idptr_mysql_stmt_bind_param)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_bind_param");  //
         p_mysql_stmt_bind_result  = (idptr_mysql_stmt_bind_result)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_bind_result");  //
         p_mysql_stmt_close        = (idptr_mysql_stmt_close)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_close");  //
         p_mysql_stmt_reset        = (idptr_mysql_stmt_reset)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_reset");  //
         p_mysql_stmt_free_result  = (idptr_mysql_stmt_free_result)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_free_result");  //

         p_mysql_stmt_send_long_data  = (idptr_mysql_stmt_send_long_data)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_send_long_data");  //
         p_mysql_stmt_result_metadata = (idptr_mysql_stmt_result_metadata)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_result_metadata");  //
         p_mysql_stmt_param_metadata  = (idptr_mysql_stmt_param_metadata)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_param_metadata");  //

         p_mysql_stmt_errno         = (idptr_mysql_stmt_errno)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_errno");  //
         p_mysql_stmt_error         = (idptr_mysql_stmt_error)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_error");  //
         p_mysql_stmt_sqlstate      = (idptr_mysql_stmt_sqlstate)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_sqlstate");  //
         p_mysql_stmt_row_seek      = (idptr_mysql_stmt_row_seek)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_row_seek");  //
         p_mysql_stmt_row_tell      = (idptr_mysql_stmt_row_tell)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_row_tell");  //
         p_mysql_stmt_data_seek     = (idptr_mysql_stmt_data_seek)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_data_seek");  //
         p_mysql_stmt_num_rows      = (idptr_mysql_stmt_num_rows)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_num_rows");  //
         p_mysql_stmt_affected_rows = (idptr_mysql_stmt_affected_rows)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_affected_rows");  //
         p_mysql_stmt_insert_id     = (idptr_mysql_stmt_insert_id)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_insert_id");  //
         p_mysql_stmt_field_count   = (idptr_mysql_stmt_field_count)GetProcAddress(gGhMySQLModule, (char *)"mysql_stmt_field_count");  //
         p_mysql_close              = (idptr_mysql_close)GetProcAddress(gGhMySQLModule, (char *)"mysql_close");  //
      
         if (p_mysql_options && p_mysql_real_connect && p_mysql_server_init && p_mysql_server_end)
            return (0);
      }
   }

   return (-1);
}

#endif  // _dTOOL_WIN_

/*

 SELECT t_br,t_konto,id FROM Bouquet_ERGO_Pero_2021.Knjizenja WHERE t_konto=1000 ORDER BY t_konto,t_datedok,t_br,t_serial LIMIT 1
 SELECT s_konto,s_naziv_1,id FROM Bouquet_ERGO_Pero_2021.KontniPlanA WHERE s_konto LIKE '0%' LIMIT 1

*/
