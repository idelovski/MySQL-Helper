/*
 *  dTOOL_SQL.h
 *  Bouquet, ... iPPromet ... taken from GlideShow
 *
 *  Created by Igor Delovski on 02.07.2011.
 *  Copyright 2011, 2016, 2020 Igor Delovski All rights reserved.
 *
 */

#ifdef _dTOOL_sql_need_

#include <mysql.h>
#include <errmsg.h>

#define  kMYSQLStringBufferLen  (1024*8)

#define  kMaxSavedWhereValues  24
#define  kMaxSavedValueLen     16

// ********************************** SQLFldDescription

typedef struct  SQLFldDesc  {
   char    *fldName;    // use static strings so they don't need to be retained... (except when joining)
   short    ctType;     // AH!, see api.h (kTint, kTuint, kTshort, kTchar, ....)
   short    sqlType;    // MYSQL_TYPE_INT, MYSQL_TYPE_STRING,... (SQLITE_INTEGER, SQLITE_TEXT), etc.
   short    fldFlags;   // uniqueFlag, whereSearchFlag, etc.
   long     maxLen;     // optional, do what you want
   long     inRecOffset;
} SQLFldDescription;

// ********************************** SQLDatabaseServer

typedef struct  _SQLDatabaseServer {

   MYSQL  *dbHandle;
   
   char    usrName[32];
   char    usrPassword[32];
   char    srvAddress[128];
   char    mainDbName[128];     // One comapny tables
   char    commonDbName[128];   // Common tables
   char    auxDbName[128];      // Import tables
   
   long    srvPort;
   
   short   activeDatabase;      // kMainDatabase, kCommonDatabase or kAuxilaryDatabase
   short  *activeCounter;       // mainTableCounter, commonTableCounter or auxTableCounter

   short   mainDBVrefNum;
   short   commonDBVrefNum;
   short   auxDBVrefNum;

   short   mainTableCounter;
   short   commonTableCounter;
   short   auxTableCounter;
   
} SQLDatabaseServer;

typedef int (*SQLRecSignatureProcPtr)(short recId, char *recPtr, char *docSignature, short dsMaxSize);

// ********************************** SQLHandler

typedef struct  _SQLHandler {
   
   SQLDatabaseServer  *dbs;          // current server

   SQLDatabaseServer   dbServers[2]; // our servers
   
   MYSQL_STMT  *sqlStatement;      // When we don' use ID_CT_MYSQL
   
   // short        usedDbServerId;    // 0,1,[2],[3]; only two atm

   // short        lastDatNo;       // start at 512, go up and down
   // short        lastFlatDatNo;   // start at 1024, go up and down
   
   short        statementPhase;  // When we don' use ID_CT_MYSQL
   short        currentSetId;    // External ID - what ctree uses
   long         dbAllCallsCounter;      // Each call so we know how many we did
   long         dbSuccessCallsCounter;  // Success call so we know how many we did
   
   vec_void_t   dbSqlInfos;

   // 15.10.2020 remarks
   // So, put in here a currentSet so I can choose proper buffers for records
   // Each ctSql should have lastSetUsed, allSetsUsed (a bitfield, each bit is for a set, 32 max)
   // Then each ctSql needs a flag if currentRecord is valid
   // See what I do after Add/Rwt -> RReadRec(), maybe we need storedRecord to make it current in RRD
   
   char         lastSqlString[kMYSQLStringBufferLen];
   char         usedWhereValues[kMaxSavedWhereValues][kMaxSavedValueLen];
   
   SQLRecSignatureProcPtr  recSigProcPtr;
   
} SQLHandler;

// ********************************** SQLOneUnsigned

// This shit is needed because Win Connector has a bug with unsigned integers
// so we keep them as text and put out and back into records when needed

#pragma mark SQLUnsigned

typedef struct  {
   char     uBuffer[16];  // Keep it here
   short    ctType;       // AH!, see api.h (kTint, kTuint, kTshort, kTchar, ....)
   short    sqlType;      // MYSQL_TYPE_INT, MYSQL_TYPE_STRING,... (SQLITE_INTEGER, SQLITE_TEXT), etc.
   long     inRecOffset;
} SQLOneUnsigned;

// ********************************** SQLUnsigned

#pragma mark SQLUnsigned

typedef struct  {
   SQLOneUnsigned  *uParams;
   SQLOneUnsigned  *uChecks;
   SQLOneUnsigned  *uResults;
   
   short            unsignedFldCount;   // Count of unsigned fields in the record
   short            uParamsCount;       // How many are used so far
   short            uChecksCount;
   short            uResultsCount;
} SQLUnsigned;

// ********************************** SQLJoinParams

#pragma mark SQLJoinParams

typedef struct  {
   char  *jpRecPreffix;  // Main rec abrevation, needed for params & orderBy
   char  *jpJoinString;  // Copy at imput at first time, use for the next batch
   long   savedOffset;
   long   savedLimit;
} SQLJoinParams;

// ********************************** ID_BIND_INFO

#pragma mark ID_BIND_INFO

typedef struct  {
   MYSQL_BIND    *biBind;
   unsigned long  biDataSize;  // Actual size
   my_bool        biIsNull;
   my_bool        biError;

   // long           biLongValue;
} ID_BIND_INFO;

// ********************************** ID_TABLE_STATE

// Needed for recreating ctree functions

#pragma mark ID_TABLE_STATE

// Idea is to use pageData in tableState only for NXTREC/SET and similar. FRSREC/SET should read only one row
// Then if I go on with the same select conditions tableState allocates pageData for up to pageRows rows

typedef struct  {
   short  tsKeyNo;
   short  tsSetSize;      // If needed, 0 -> no set
   
   short  numSegSet;      // if used for set - FRSSET/LSTSET
   short  numSegOrdBy;    // if used for ORDER BY
   short  numSegWhere;    // if used for WHERE
   short  selWhereFlags;  // kWhereFlagEqual etc.
   
   // long   startOffset;  // where we are (Last SELECT OFFSET)
   // long   nextOffset;   // where to go next (Next OFFSET if we go forward), well, not used atm
   
   short  pageRows;        // should be equal for all records but anyway...., maybe for Trans it should be like 512 and for docs 64
   short  pageIndex;       // where we are in the page of records

   short  prevPageRows;    // used for the last call
   short  pagePass;        // how many times we called NXT/PRV

   short  usedPageRows;    // rows we found in database
   short  selectIdOnly;    // we need only id

   short  nothingForward;  // No need to do NXTSET
   short  nothingBackward; // No need to do PRVSET

   char  *pageData;        // rec size * pageRows, may be NULL so I need to allocate it as needed - should I free on err? On add, rwt i del too
   // char  *likeBuffer;      // Transfered from selParams, created if setSize shorter than segSize
} ID_TABLE_STATE;

#define  kStandardPageRows  64
#define  kStandardSelSets    4

// Usage :
// ctSql->tblState[ctSql->curTblState] ...
// ctSql->ctSetIds[1..kStandardSelSets] ... externi ctSet ide od 1 do 16 or whatever

// ********************************** ID_CT_MYSQL

#pragma mark ID_CT_MYSQL

// For each table/ct-file

// - Three struct pointers
// - Ok, buffers: Array of pointers to my structures, real internal structures
// - bindInfos -> maybe for each key, but anyway, as many as there are fields
// - binds, as many as there are fields

// SQLFldDescription - create dynamically from RecFieldInfo+iRecFldOffsets
// SQL name for rst.t_roba_cd -> rst__t_roba_cd

typedef struct  {
   long                recSize;
   long                recSizeWithId;      // + sizeof(long)
   long                varLenBuffSize;     // varLen part
   long                errNo;
   short               recStructFldCount;  // flds in the record, structs counted as one
   short               recSqlFldCount;     // Sql relevant fields with fldCounts of subRecords
   short               skippedFldCount;    // Count of skipped fields
   short               varLenFlag;         // Maybe one day add other FEATURES - WAITING HERE for other flags
   short               fileIndex;          // recId kod poziva id_FindRecFieldInfo()
   short               datNo;
   short               dNumIdx;
   short               maxChkSegments;     // Needed for bind allocations, twice as many as key segments
   short               setSize;            // Copy from selParams for the next call (FRS->NXT)
   short               curTblStateIdx;     // 0...kStandardSelSets-1, sqlHandler->currentSetId
   short               ctSetIds[kStandardSelSets];
   short               currentRecFlags;    // kCurrValid4Flow, kCurrValid4ReRead or kCurrValid4Start
   char                dbName[128];
   char                tableName[128];
   
   MYSQL              *dbHandle;
   IFIL               *ifilPtr;

   // char               *custDBName;     // if NULL, use sqlHandler->dbName
   
   char               *paramRecord;    // ptr of recSize; Parameters for insert/update...
   char               *checkRecord;    // ptr of recSize; Where clause
   char               *resultRecord;   // ptr of recSize; Results
   char               *currentRecord;  // ptr where to put result in the end so next time we go from there
   
   char               *varLenBuffPtr;  // Here we put var len part
   
   char               *paramBuffers;   // Params, simple array of pointers to fields in paramRecord
   char               *checkBuffers;   // WhereClause, same as above, but in checkRecord
   char               *resultBuffers;  // Results, same as above 
   // char               *colNames[kMaxCTSQLKeys+1];  Names ar in RecFieldInfo + iRecFldOffsets
   // Params
   short               paramCount;
   ID_BIND_INFO       *paramBindInfos;  // blocks of my own extension to MYSQL_BIND, as many as fields
   MYSQL_BIND         *parChkBinds;     // continuous blocks, must be like that, maxFldNum + maxIndexSegments

   // Where Clause - More Params
   short               checkCount;
   ID_BIND_INFO       *checkBindInfos;  // blocks of ID_BIND_INFOs, as many as maxIndexSegments
   // MYSQL_BIND        **chkBinds;
   
   // Results
   short               resultCount;
   ID_BIND_INFO       *resultBindInfos;
   MYSQL_BIND         *resultBinds;

   SQLFldDescription  *fldDescs;        // Created from RecFieldInfo + iRecFldOffsets
   RecFieldInfo       *rfi;
   SQLUnsigned         unSig;           // Unsigned infos, allocate with unsignedFldCount
   SQLJoinParams       joinParams;
   ID_TABLE_STATE     *tblState;        // kStandardSelSets, Table State, needed for NXTREC/SET etc

   MYSQL_STMT         *sqlStatement;    // Create it use it then NULL, this way I know if it's reused by mistake
   
   char               *savedCurrentRecord[kStandardSelSets];    // Keep currentRecord here on context switch
   short               savedCurrentRecFlags[kStandardSelSets];  // kCurrValid4Flow, kCurrValid4ReRead or kCurrValid4Start
   short               savedCurrentSetSize[kStandardSelSets];   // setSize on ContextSwitch
   
   short               statementPhase;  // INIT/PREPARE/...
   
   long                lastInsertId;
   long                idToBind;      // in/out
   
   // char               *likeBindBuffer;
   char               *extraBindBuffer;  // used for flat records and LIKE param
   
} ID_CT_MYSQL;

// ********************************** SQLSelectParams

#pragma mark SQLSelectParams

typedef struct  {
   char   *whereClause;
   char   *orderByClause;
   
   char   *recPreffix;
   char   *joinSqlStr;
   
   short   keyNo;
   short   orderByKeySegs;
   short   whereKeySegs;

   short   setKeySegs;     // FRSSET/LSTSET

   short   limitCnt;       // move to long
   short   offsetCnt;      // move to long too
   short   subStringPos;   // Not used atm
   short   subStringLen;   // Not used atm
   
   short   whereFlags;     // kWhereFlagEqual etc.
   short   distinctFlag;
   short   selectIdOnly;   // Select only Id
   
   long    idAsParam;      // in
   // char   *likeBuffer;     // Setup if setSize shorter than segSize, transfer to tblState
} SQLSelectParams;

// ********************************** ID_BIND_INFO

#pragma mark ID_BIND_INFO

typedef struct  {
   char  *foundSrvName[64];
   char  *foundUser[64];
   char  *foundPass[64];
   char  *foundServer[64];
   char  *foundDBPrefix[64];
   char  *foundDBTicker[64];
   char  *foundDBFullName[64];
} SQLSetupFile;


#define  kMYSQLFlatFileName     "SingletonRecords"  // set in id_SQLInitOneTable()
#define  kMYSQLFlatContentName  "SingletonContent"  // set in id_SQLBuildNewFldDescriptions()
#define  kMYSQLFlatKeyName      "SingletonKey"      // set as above, used in id_SQLWhereClauseString()

#define  kMYSQLFileDatNoStart       512
#define  kMYSQLFlatFileDatNo       1024
#define  kMYSQLBufferedRows        1024

#define  kMYSQLStmtPhaseDefault       0
#define  kMYSQLStmtPhaseInit          1
#define  kMYSQLStmtPhasePrepare       2
#define  kMYSQLStmtPhaseBind          3


#define  kBindFlagResults             0
#define  kBindFlagWhereClause        -1
#define  kBindFlagParams              1 

#define  kWhereFlagEqual              0
#define  kWhereFlagConcat             1
#define  kWhereFlagLessThan           2
#define  kWhereFlagLessOrEQ           4
#define  kWhereFlagGreaterThan        8
#define  kWhereFlagGreatOrEQ         16
#define  kWhereFlagDescending        32  // For LSTREC
#define  kWhereFlagNoWhere           64  // Don't need WHERE clause
#define  kWhereFlagKeywordWhere     128  // Add WHERE keyword to string
#define  kWhereFlagNeedsId          256  // Add WHERE keyword to string
#define  kWhereFlagWhereIdOnly      512  // Only id goes into WHERE
#define  kWhereFlagRecordNameOnly  1024  // Only record name (fCName) goes into WHERE
#define  kWhereFlagLike            2048  // Use like for set part

// ********************************** not reviewed down

#ifdef _NIJE_
#define  kArrayFldSeparator     NOPE     @"--,--"
#define  kMaxBusyRetryCount     NOPE     8
#endif

#define  kFldDescFlagUniqueKey       1
#define  kFldDescFlagSearchableKey   2
#define  kFldDescFlagNotNullKey      4
#define  kFldDescFlagVariableLength  8
#define  kFldDescFlagJoinedByKey    16   // When joining 2 tables, key used for join


#ifdef _NIJE_
#define  kSTRING_Class    NOPE     "NSString"
#define  kDATE_Class      NOPE     "NSDate"
#define  kDATA_Class      NOPE     "NSData"
#define  kARRAY_Class     NOPE     "NSArray"
#define  kNUMBER_Class    NOPE     "NSNumber"
#define  kIMAGE_Class     NOPE     "UIImage"
#endif

// ................................currentRecFlags

#define  kCurrInvalid        0  // All clear
#define  kCurrValid4Start    1  // Use it for RWTREC || DELREC
#define  kCurrValid4Flow     2  // Use it for NXTREC/SET || PRVREC/SET
#define  kCurrValid4ReRead   4  // Use it for RRDREC

// Remark - After Del we can go on with Nxt/Prv but not ReRead or Rwt etc.

// ................................currentDatabase

#define  kMainDatabase       0  // main
#define  kCommonDatabase     1  // Common tables
#define  kAuxilaryDatabase   2  // For import, prev year etc.

// @property (nonatomic, retain)  NSString  *fileName;

void  id_SQLSetRecSignatureHandler (SQLRecSignatureProcPtr sigProcPtr);

int  id_SQLInitWithUserAndServer (char *userName, char *userPassword, char *serverAddress, long serverPort);
int  id_SQLCheckServer (char *serverAddress, short *srvActiveFlag);

int  id_SQLChangeDatabase (char *dbName, short dbSelector, short createFlag);
int  id_SQLCloseDatabase (void);

int  id_SQLReadSetupFile (short vRefNum, char *fileName, short paramsBuffLen,
                          char *foundUser, char *foundPass, char *foundServer, char *foundPort,
                          char *foundDBPrefix, char *foundDBTicker, char *foundDBName);
int  id_SQLReadSetupList (short vRefNum, char *fileName, SQLTarget *sqlTargets, short maxTargets);

int  id_SQLFormDataBaseName (char *dbBasicName,
                             char *dbPrefix, char *dbCompany, char *dbYear, short yearOffset,
                             char *databaseName, short nameBuffLen);
int  id_SQLSplitDataBaseName (char *dbFullName, char *dbPrefix, char *dbCompany, char *dbYear);

long   id_SQLGetCallsCounter (long *successfulCalls);
char  *id_SQLGetLastSqlString (void);
int    id_SQLGetLastWhereParamCount (void);
char  *id_SQLGetLastWhereParamValue (short idx);
char  *id_SQLDatabaseName (SQLDatabaseServer *dbs, short dbSelector);  // kMainDatabase,kCommonDatabase,kAuxilaryDatabase
char  *id_SQLActiveDatabaseName (void);

Boolean  id_SQLCommonDatabaseActive (void);
Boolean  id_SQLAuxilaryDatabaseActive (void);

ID_CT_MYSQL  *id_SQLSetupOneTable (RecFieldInfo *rfi, IFIL *ifilPtr, long recSize);
ID_CT_MYSQL  *id_SQLFindFileNo (short fileNo);
ID_CT_MYSQL  *id_SQLFindBySet (short setId, short *setIndex);

RecFieldInfo *id_SQLFindRfiByTableName (char *tableName);

SQLDatabaseServer  *id_SQLGetServerInfo (ID_CT_MYSQL *ctSql, short *dbsIndex);

int    id_SQLCountActiveTables (MYSQL *dbHandle);
int    id_SQLFindDatNoByFileIndex (short fileIndex, short dbSelector);
char  *id_SQLGetDatabaseName (short fileNo);
char  *id_SQLGetTableName (short fileNo);

int  id_SQLAvailableDatNo (Boolean flatFile);
int  id_SQLGetKeySegments (ID_CT_MYSQL *ctSql, short keyNo);
int  id_SQLGetNumberOfKeys (ID_CT_MYSQL *ctSql);
int  id_SQLGetNumberOfIndexes (ID_CT_MYSQL *ctSql);
int  id_SQLKeySegmentName (ID_CT_MYSQL *ctSql, short keyNo, short segIdx, char *segName, short *segLen);

char  *id_SQLConvertStringForSQL(const char *origStr, char *tarStr, short tarLen);
char  *id_SQLConvertStringFromSQL(const char *sqlStr, char *tarStr, short tarLen);

SQLFldDescription  *id_SQLFindFldDescription (ID_CT_MYSQL *ctSql, char *fldName);


int  id_SQLInitOneTable (ID_CT_MYSQL *ctSql, RecFieldInfo *rfi, IFIL *ifilsPtr, long recSize);
void id_SQLDisposeOneTable (ID_CT_MYSQL *ctSql);

int  id_SQLCreateTable (ID_CT_MYSQL *ctSql, char *tableName);
int  id_SQLAlterTable (ID_CT_MYSQL *ctSql, short *chgCount);
int  id_SQLCopyTable (char *targetDBName, char *sourceDBName, char *tableName);

int  id_SQLCreateNewDatabase (MYSQL *dbHandle, char *dbName, short dbSelector, char *retErrStr, short errMaxLen);
int  id_SQLInitWithFilePath (char *filePath, char *retErrStr, short errMaxLen);

// General purpose calls

int  id_SQLPerformQuery (MYSQL *dbHandle, char *sqlString, char *retErrStr, short errMaxLen);
int  id_SQLGetLongExecutingSqlStatement (char *sqlString, long *retCode);
int  id_SQLGetTextExecutingSqlStatement (char *sqlString, char *retString, short maxLen);
int  id_SQLGetTextArrayExecutingSqlStatement (char *sqlString, char **retStringArray, short maxLen, long *itemCount, Boolean asColumns);

int  id_SQLBindSingleCTreeObject (ID_CT_MYSQL *ctSql, short paramBindFlag, char *recPtr, char **fldBuffPtr, ID_BIND_INFO *bindInfo, SQLFldDescription *fldDesc, char *retFldValue);
int  id_SQLBindCTreeRecord (ID_CT_MYSQL *ctSql, void *recPtr, short paramBindFlag, short keyNo, short maxKeySegs, short whereFlags);

int  id_SQLBindUnsignedObject (ID_CT_MYSQL *ctSql, short paramBindFlag, char *recPtr, ID_BIND_INFO *bindInfo, SQLFldDescription *fldDesc);
int  id_SQLCopyBoundUnsignedResults (ID_CT_MYSQL *ctSql, char *recPtr);
void id_SQLResetBoundUnsignedCounts (ID_CT_MYSQL *ctSql);

MYSQL_BIND  *id_SQLBindText (ID_CT_MYSQL *ctSql, ID_BIND_INFO *bindInfo, const char *txt, short strLen, short  maxLen);
MYSQL_BIND  *id_SQLBindLong (ID_CT_MYSQL *ctSql, ID_BIND_INFO *bindInfo, long *longPtr, Boolean unsignedFlag);
MYSQL_BIND  *id_SQLBindShort (ID_CT_MYSQL *ctSql, ID_BIND_INFO *bindInfo, short *shortPtr, Boolean unsignedFlag);
MYSQL_BIND  *id_SQLBindChar (ID_CT_MYSQL *ctSql, ID_BIND_INFO *bindInfo, char *charPtr, Boolean unsignedFlag);
MYSQL_BIND  *id_SQLBindBlob (ID_CT_MYSQL *ctSql, ID_BIND_INFO *bindInfo, const char *blobBuff, short blobLen, short maxLen);
MYSQL_BIND  *id_SQLBindFloat (ID_BIND_INFO *bindInfo, float *floatPtr);
MYSQL_BIND  *id_SQLBindDouble (ID_CT_MYSQL *ctSql, ID_BIND_INFO *bindInfo, double *doublePtr);

int  id_SQLAddCTreeObject (ID_CT_MYSQL *ctSql, void *recPtr, long *retRowId);
int  id_SQLRewriteCTreeObject (ID_CT_MYSQL *ctSql, void *oldRecPtr, void *newRecPtr/*, long *retRowId*/);
int  id_SQLDeleteCTreeObject (ID_CT_MYSQL *ctSql, void *recPtr);
int  id_SQLSelectCTreeObject (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, void *inRecPtr, void *outRecPtr);

int  id_SQLGetDatabaseList (char *partName, char **dbNames, long *arrayLen, short itemSize);  // as in  names[arrayLen][itemSize]
int  id_SQLCheckDatabaseExist (char *dbName);   // -1 or 0
int  id_SQLCheckTableExist (char *dbName, char *tableName);
int  id_SQLCheckColumnExist (char *dbName, char *tableName, char *columnName);
int  id_SQLCheckIndexExist (char *dbName, char *tableName, char *indexName);

int  id_SQLCheckTableCondition (ID_CT_MYSQL *ctSql);

int  id_SQLCountOfTablesInDatabase (char *dbName, long *retCount);
int  id_SQLCountOfRecordsInTable (char *dbName, char *tableName, long *retCount);
int  id_SQLCountOfColumnsInTable (char *dbName, char *tableName, long *retCount);
int  id_SQLCountOfIndexesInTable (char *dbName, char *tableName, long *retCount);
int  id_SQLCountOfExactColumnsInTable (char *dbName, char *tableName, char *columnName, char *columnValue, long *retCount);

// Fake CTree

int  id_SQLctOPNIFIL (short fileNo, IFIL *ifilPtr, Boolean *createdNew);  // If NULL, don't create
int  id_SQLctCLIFIL (IFIL *ifilPtr);

void id_SQLctCLISAM (void);  // Close all

long  id_SQLctDATENT (short fileNo);

int  id_SQLctADDREC (short fileNo, void *recPtr);
int  id_SQLctADDVREC (short fileNo, void *recPtr, long fullrecSize);
int  id_SQLctRWTREC (short fileNo, void *newRecPtr);
int  id_SQLctRWTVREC (short fileNo, void *newRecPtr, long fullrecSize);
int  id_SQLctDELREC (short fileNo);

int  id_SQLctRRDREC (short fileNo, void *outRecPtr);
int  id_SQLctREDVREC (short fileNo, void *outRecPtr, long fullrecSize);

int  id_SQLctEQLREC (short fileNo, void *inRecPtr, void *outRecPtr);

int  id_SQLctGTEREC (short fileNo, void *inRecPtr, void *outRecPtr);
int  id_SQLctLTEREC (short fileNo, void *inRecPtr, void *outRecPtr);

int  id_SQLctFRSREC (short fileNo, void *outRecPtr);
int  id_SQLctLSTREC (short fileNo, void *outRecPtr);

int  id_SQLctNXTREC (short fileNo, void *outRecPtr);
int  id_SQLctPRVREC (short fileNo, void *outRecPtr);

int  id_SQLctFRSSET (short fileNo, void *inRecPtr, void *outRecPtr, short setSize, short recLimit);
int  id_SQLctLSTSET (short fileNo, void *inRecPtr, void *outRecPtr, short setSize, short recLimit);

int  id_SQLctFRSJOIN (short fileNo, char *inRecPreffix, char *sqlString, void *outRecPtr);

int  id_SQLctNXTSET (short fileNo, void *outRecPtr);
int  id_SQLctPRVSET (short fileNo, void *outRecPtr);

long id_SQLctGETCURP (short fileNo);
int  id_SQLctSETCURI (short fileNo, long position);

int  id_SQLctGETFIL (short fileNo);

int  id_SQLctChangeSet (short setId);  // Use when on pr_ChgSet
int  id_SQLctFreeSet (short setId);  // Use on pr_FreeSet

// ---------------------------------------------------- ObjC

#ifdef _NIJE_

+ (int)createNewDatabase:(NSString *)filePath creatingTableWithSqlString:(NSString *)sqlStr errString:(NSString **)retErrStr;


- (id)initWithFilePath:(NSString *)filePath;
- (id)initWithFileInDocumentsDirectory:(NSString *)fName;
- (id)initWithFileInCacheDirectory:(NSString *)aFileName;

- (sqlite3 *)dbHandle;

- (NSString *)sqlStringWithString:(NSString *)origStr;
- (NSString *)stringWithSqlString:(NSString *)sqlStr;
- (BOOL)shouldContinueLoopingWithReturnCode:(int)retCode
                         andNumberOfRetries:(int)numberOfRetries;

- (int)execSqlString:(NSString *)sqlString errString:(NSString **)retErrStr;

- (int)prepare:(NSString *)sqlString sqlStatement:(sqlite3_stmt **)retStatement;
- (int)stepWithSqlStatement:(sqlite3_stmt *)statement;
- (int)finalizeSqlStatement:(sqlite3_stmt *)statement;

- (int)bindText:(const char *)txt withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum asTransient:(BOOL)byRef;
- (int)bindInt:(int)intVal withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum;
- (int)bindBlob:(const void *)ptr withSqlStatement:(sqlite3_stmt *)statement forParamNumber:(NSInteger)paramNum ofLength:(NSInteger)length asTransient:(BOOL)byRef;

- (int)bindString:(NSString *)inStr inRange:(NSRange *)optRange withSqlStatement:(sqlite3_stmt *)sqlStatement forParamNumber:(NSInteger)paramNum;

- (const unsigned char *)textWithSqlStatement:(sqlite3_stmt *)statement forColumnIndex:(NSInteger)idx;
- (int)intWithSqlStatement:(sqlite3_stmt *)statement forColumnIndex:(NSInteger)idx;
- (const void *)blobWithSqlStatement:(sqlite3_stmt *)statement forColumnIndex:(NSInteger)idx andLength:(NSInteger *)retLength;

// ---- SQLFldDescription Handling ---

/*
+ (void)initializeFldDescription:(SQLFldDescription *)fldDescCArray
               withStorageObject:(id)storageObj;
*/

+ (NSString *)creationTypeStringForFldDescription:(SQLFldDescription *)fldDesc;
+ (NSString *)sqlWhereClauseStringWithSearchFields:(NSArray *)fldNamesToMatch
                             withSearchFieldValues:(NSArray *)fldValuesToMatch        // just to check for nulls
                             withSearchFieldRanges:(NSArray *)fldRangesToMatchOrNil
                              usingFldDescriptions:(SQLFldDescription *)fldDescCArray
                                        exactMatch:(BOOL)exactMatch  // use = for YES, if NO use LIKE
                                returningUsedCount:(int *)boundCnt;

+ (NSString *)sqlCreationStringForTable:(NSString *)tableName
                    withFldDescriptions:(SQLFldDescription *)fldDescCArray;
+ (NSString *)sqlColumnAppendingStringForTable:(NSString *)tableName
                            withFldDescription:(SQLFldDescription *)oneFldDesc;

+ (NSString *)sqlInsertionStringForTable:(NSString *)tableName
                     withFldDescriptions:(SQLFldDescription *)fldDescCArray;
+ (NSString *)sqlUpdatingStringForTable:(NSString *)tableName
                       withSearchFields:(NSArray *)fldNamesToMatchOrNil
                   andSearchFieldValues:(NSArray *)fldValuesToMatchOrNil  // just to check for nulls
                   usingFldDescriptions:(SQLFldDescription *)fldDescCArray
                   andOnlyFieldToUpdate:(NSString *)onlyFieldOrNil
                     returningUsedCount:(int *)boundCnt;
+ (NSString *)sqlDeletionStringForTable:(NSString *)tableName
                       withSearchFields:(NSArray *)fldNamesToMatchOrNil
                   andSearchFieldValues:(NSArray *)fldValuesToMatchOrNil        // just to check for nulls
                   usingFldDescriptions:(SQLFldDescription *)fldDescCArray
                     returningUsedCount:(int *)boundCnt;

- (int)bindSingleObject:(id)fldObj
         toSqlStatement:(sqlite3_stmt *)sqlStatement
            atBindIndex:(int)bindIndex  // one based
    usingFldDescription:(SQLFldDescription *)fldDesc
               andRange:(NSRange *)fldRange;

- (int)bindStorageObject:(id)storageObj
          toSqlStatement:(sqlite3_stmt *)sqlStatement
          fromBindOffset:(int)bOffset
    usingFldDescriptions:(SQLFldDescription *)fldDescCArray
 includingOnlySearchable:(BOOL)searchableOnly
     includingOnlyFields:(NSArray *)fldNamesToMatchOrNil
      returningUsedCount:(int *)boundCnt;

- (int)bindToSqlStatement:(sqlite3_stmt *)sqlStatement
           fromBindOffset:(int)bOffset
     usingFldDescriptions:(SQLFldDescription *)fldDescCArray
  includingOnlySearchable:(BOOL)searchableOnly
               exactMatch:(BOOL)exactMatch
         searchFieldNames:(NSArray *)fldNamesToMatch
     andSearchFieldValues:(NSArray *)fldValuesToMatch
    withSearchFieldRanges:(NSArray *)fldRangesToMatchOrNil
       returningUsedCount:(int *)boundCnt;

- (id)extractSingleObjectFromSqlStatement:(sqlite3_stmt *)sqlStatement
                           forColumnIndex:(int)columnIndex  // zero based
                      usingFldDescription:(SQLFldDescription *)fldDesc;

#pragma mark -

- (int)addStorageObject:(id)storageObj
              intoTable:(NSString *)tableName
   usingFldDescriptions:(SQLFldDescription *)fldDescCArray
         returningRowId:(NSInteger *)retRowId;

- (int)updateStorageObject:(id)newStorageObjOrNil
      byReplacingOldObject:(id)oldStorageObjOrNil
    orJustSearchFieldNames:(NSArray *)fldNamesToMatch
      andSearchFieldValues:(NSArray *)fldValuesToMatch
                   inTable:(NSString *)tableName
      usingFldDescriptions:(SQLFldDescription *)fldDescCArray
     withOnlyFieldToUpdate:(NSString *)onlyFieldOrNil
               andItsValue:(id)onlyFieldValueOrNil
      returningChangeCount:(NSInteger *)retChgCount;

- (int)deleteRecordInTable:(NSString *)tableName
      usingFldDescriptions:(SQLFldDescription *)fldDescCArray
         withStorageObject:(id)storageObjOrNil
    orJustSearchFieldNames:(NSArray *)fldNamesToMatch
      andSearchFieldValues:(NSArray *)fldValuesToMatch
      returningChangeCount:(NSInteger *)retChgCount;

// Multiple results version, returns array of objects,
// could be array of arrays if not onlyFieldOrNil version & not singleResultFlag

- (id)selectObjectsArrayFromTable:(NSString *)tableName
             usingFldDescriptions:(SQLFldDescription *)fldDescCArray
                 withSelectParams:(SQLSelectParams *)selParams
           orJustSearchFieldNames:(NSArray *)fldNamesToMatch
             andSearchFieldValues:(NSArray *)fldValuesToMatch
            withSearchFieldRanges:(NSArray *)fldRangesToMatchOrNil
             andOnlyFieldToSelect:(NSString *)onlyFieldOrNil
               singleResultNeeded:(BOOL)singleResultFlag
                  givinReturnCode:(int *)returnCodeOrNil;

// Single result version, calls above method internally
- (id)selectObjectFromTable:(NSString *)tableName
       usingFldDescriptions:(SQLFldDescription *)fldDescCArray
           withSelectParams:(SQLSelectParams *)selParams
     orJustSearchFieldNames:(NSArray *)fldNamesToMatch
       andSearchFieldValues:(NSArray *)fldValuesToMatch
      withSearchFieldRanges:(NSArray *)fldRangesToMatchOrNil
       andOnlyFieldToSelect:(NSString *)onlyFieldOrNil
            givinReturnCode:(int *)returnCodeOrNil;

// Joins two tables, multiple results, calls above method internally

- (id)selectObjectsFromTable:(NSString *)tableName1
        usingFldDescriptions:(SQLFldDescription *)fldDescCArray1
                joiningTable:(NSString *)tableName2
                 onFieldName:(NSString *)joiningField
             fldDescriptions:(SQLFldDescription *)fldDescCArray2
            searchFieldNames:(NSArray *)fldNamesToMatch
        andSearchFieldValues:(NSArray *)fldValuesToMatch
       withSearchFieldRanges:(NSArray *)fldRangesToMatchOrNil
        andOnlyFieldToSelect:(NSString *)onlyFieldOrNil
             givinReturnCode:(int *)returnCodeOrNil;

#pragma mark -

- (int)countOfItemsFromTable:(NSString *)tableName
        usingFldDescriptions:(SQLFldDescription *)fldDescCArray
            withSelectParams:(SQLSelectParams *)selParams
      orJustSearchFieldNames:(NSArray *)fldNamesToMatch
        andSearchFieldValues:(NSArray *)fldValuesToMatch;

- (int)countOfRecordsInTable:(NSString *)tableName givingReturnCode:(int *)returnCodeOrNil;
- (int)countOfColumnsInTable:(NSString *)tableName
         includingPrimaryKey:(BOOL)primKeyFlag
            givingReturnCode:(int *)returnCodeOrNil;


// Kludged method
- (NSString *)selectAccountClosestToLocationWithLatitude:(double)latitude andLongitude:(double)longitude givingReturnCode:(int *)returnCodeOrNil;

#endif

#ifdef _DTOOL_SRC_SQL_

// Move these to loc.h

static int   id_SQLCType2SQLType (short fldCType, short len);
static int   id_SQLCountUsedFields (ID_CT_MYSQL *ctSql, RecFieldInfo *theRfi, short *retUnsignedCnt);
static int   id_SQLConvertRecordEncoding (ID_CT_MYSQL *ctSql, char *recPtr, Boolean reverseFlag);
static void  id_SQLInitializeBindings (ID_BIND_INFO *arrBindInfos, MYSQL_BIND *arrBinds, short bndCount);
static long  id_SQLCloseStatement (ID_CT_MYSQL *ctSql);
static char *id_SQLTypeCreationString (SQLFldDescription *fldDesc);
// static char *id_SQLOneParamWhere (char *buffStr, char *paramName, short isNull, short eqDirection);  // WTF!???? Why do I need this
static long  id_SQLInitPrepare (ID_CT_MYSQL *ctSql, char *sqlString);

static char *id_SQLCreationStringForTable (ID_CT_MYSQL *ctSql, char *buffPtr, short buffLen);
static char *id_SQLColumnAppendingStringForTable (char *tableName, SQLFldDescription  *oneFldDesc, char *buffPtr, short buffLen);
static char *id_SQLIndexAppendingStringForTable (ID_CT_MYSQL *ctSql, SQLFldDescription  *oneFldDesc, short keyNo, char *buffPtr, short buffLen);
static char  *id_SQLDeleteIndexStringForTable (ID_CT_MYSQL *ctSql, short keyNo, char *buffPtr, short buffLen);

static char *id_SQLInsertionStringForTable (ID_CT_MYSQL *ctSql, char *buffPtr, short buffLen, short *retBoundCnt);
static char *id_SQLUpdatingStringForTable (ID_CT_MYSQL *ctSql, char *buffPtr, short buffLen, short *retKeyNo, short *retBoundCnt);
static char *id_SQLDeletionStringForTable (ID_CT_MYSQL *ctSql, char *buffPtr, short buffLen, short *retKeyNo, short *retBoundCnt);
static char *id_SQLSelectionStringForTable (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, char *buffPtr, short buffLen, short *retResultCnt, short *retBoundCnt);

static int   id_SQLWhereClauseString (ID_CT_MYSQL *ctSql, short keyNo, short keySegs, short flags, char *whereString, short maxLen, short *usedKeySegs, short *usedKeyLength);
static int   id_SQLConcatSegmentsString (ID_CT_MYSQL *ctSql, short keyNo, short reqKeySegs, char *concatString, short maxLen, short namesFlag);  // Instead of questionmarks
static int   id_SQLKeySegmentsString (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, short reqKeySegs, char *theString, short maxLen, short namesFlag);  // Instead of questionmarks

static void  id_SQLAllocateJoinParamsBuffers (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, char *preffixStr, char *joinSqlStr);
static void  id_SQLDisposeJoinParamsBuffers (ID_CT_MYSQL *ctSql);

static void  id_SQLInitTableState (ID_CT_MYSQL *ctSql, short reqPageRows);
static int   id_SQLAllocTableStatePageData (ID_CT_MYSQL *ctSql);
static int   id_SQLDisposeTableStatePageData (ID_CT_MYSQL *ctSql, short onlyIndex);  // -1 for all

// static int   id_SQLAllocTableStateSelParamData (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams);
// static int   id_SQLDisposeTableStateSelParamData (ID_CT_MYSQL *ctSql, short onlyIndex);  // -1 for all

static int   id_SQLSetupTableState (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams);
static int   id_SQLRecallBackTableState (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, short fileNo, short whereFlags, Boolean needSetSize);
static int   id_SQLNextFromTableStateBuffer (ID_CT_MYSQL *ctSql, short keyNo, void *outRecPtr);
static int   id_SQLPrevFromTableStateBuffer (ID_CT_MYSQL *ctSql, short keyNo, void *outRecPtr);

static void  id_SQLInitBasicSelectParams (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, short flags, short keyNo, short numSegWhere, short numSegOrdBy, short offset, short limit);
static void  id_SQLInitSetSelectParams (ID_CT_MYSQL *ctSql, SQLSelectParams *selParams, short setSize);

static void  id_SQLSetupStandardKeyAndSegments (ID_CT_MYSQL *ctSql, short fileNo, short *keyNo, short *numSegWhere, short *numSegOrdBy);
static void  id_SQLSetupFirstKeyAndSegments (ID_CT_MYSQL *ctSql, short fileNo, short *keyNo, short *numSegWhere, short *numSegOrdBy);
static void  id_SQLSetupOnlyKeyAndOrderBySegments (ID_CT_MYSQL *ctSql, short fileNo, short *keyNo, short *numSegWhere, short *numSegOrdBy);
static void  id_SQLSetupStandardKeyOnly (ID_CT_MYSQL *ctSql, short fileNo, short *keyNo);

static int   id_SQLSaveSetsCurrentRecord (ID_CT_MYSQL *ctSql, short setIndex);
static int   id_SQLRestoreSetsCurrentRecord (ID_CT_MYSQL *ctSql, short setIndex);
static int   id_SQLDisposeSetsCurrentRecord (ID_CT_MYSQL *ctSql, short setIndex);


static SQLFldDescription  *id_SQLBuildNewFldDescriptions (ID_CT_MYSQL *ctSql, RecFieldInfo *theRfi);

#endif  // _DTOOL_SRC_SQL_

#endif  // _dTOOL_sql_need_
