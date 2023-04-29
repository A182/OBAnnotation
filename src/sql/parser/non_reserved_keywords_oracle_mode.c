/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#include <stdio.h>
#include "lib/alloc/alloc_assist.h"
#include "ob_non_reserved_keywords.h"

#define WINDOW_FUNCTION_NUM 41

static t_node *oracle_keywords_root = NULL;

/* List of non-reserved keywords */
/*一开始会对这些word建立一颗trie树，对于每次的查找来言，树是固定的
 *若新添加的keyword含有除大小写字母、'_'和数字以外的其它字符，请联系@叶提修改这颗树。
 *实现不需要保证字典序，但是原则上还是保证字典序，方便维护和查找*/
static const NonReservedKeyword Oracle_non_reserved_keywords[] =
{
/*
 * Oracle Non-Reserved/Reserved Keywords
 * 添加新的非保留关键字请从OB Non-Reserved Keywords处开始添加，前面是兼容oracle的保留关键字及非保留关键字！！！
 * */

  {"absent", ABSENT},
  {"access", ACCESS},
  {"accessed", ACCESSED},
  {"add", ADD},
  {"admin", ADMIN},
  {"after", AFTER},
  {"all", ALL},
  {"allocate", ALLOCATE},
  {"alter", ALTER},
  {"analyze", ANALYZE},
  {"and", AND},
  {"any", ANY},
  {"archive", ARCHIVE},
  {"archivelog", ARCHIVELOG},
  {"as", AS},
  {"asc", ASC},
  {"authorization", AUTHORIZATION},
  {"audit", AUDIT},
  {"avg", AVG},
  {"backup", BACKUP},
  {"become", BECOME},
  {"before", BEFORE},
  {"begin", BEGI},
  {"between", BETWEEN},
  {"blob", BLOB},
  {"block", BLOCK},
  {"binary_double", BINARY_DOUBLE},
  {"binary_float", BINARY_FLOAT},
  {"body", BODY},
  {"by", BY},
  {"cache", CACHE},
  {"cancel", CANCEL},
  {"cascade", CASCADE},
  {"change", CHANGE},
  {"char", CHAR},
  {"character", CHARACTER},
  {"check", CHECK},
  {"checkpoint", CHECKPOINT},
  {"clob", CLOB},
  {"close", CLOSE},
  {"cluster", CLUSTER},
  {"cipher", CIPHER},
  {"cobol", COBOL},
  {"column", COLUMN},
  {"commit", COMMIT},
  {"comment", COMMENT},
  {"compile", COMPILE},
  {"compress", COMPRESS},
  {"constraint", CONSTRAINT},
  {"constraints", CONSTRAINTS},
  {"contents", CONTENTS},
  {"continue", CONTINUE},
  {"connect", CONNECT},
  {"controlfile", CONTROLFILE},
  {"count", COUNT},
  {"create", CREATE},
  {"current", CURRENT},
  {"cursor", CURSOR},
  {"cycle", CYCLE},
  {"database", DATABASE},
  {"datafile", DATAFILE},
  {"date", DATE},
  {"dba", DBA},
  {"dba_recyclebin", DBA_RECYCLEBIN},
  {"dec", DEC},
  {"decimal", DECIMAL},
  {"declare", DECLARE},
  {"desc", DESC},
  {"default", DEFAULT},
  {"delete", DELETE},
  {"deleting", DELETING},
  {"disable", DISABLE},
  {"distinct", DISTINCT},
  {"dismount", DISMOUNT},
  {"dml", DML},
  {"ddl", DDL},
  {"dop", DOP},
  {"double", DOUBLE},
  {"drop", DROP},
  {"dump", DUMP},
  {"each", EACH},
  {"else", ELSE},
  {"enable", ENABLE},
  {"end", END},
  {"escape", ESCAPE},
  {"events", EVENTS},
  {"except", EXCEPT},
  {"exceptions", EXCEPTIONS},
  {"exclude", EXCLUDE},
  {"exclusive", EXCLUSIVE},
  {"exists", EXISTS},
  {"exec", EXEC},
  {"execute", EXECUTE},
  {"exempt", EXEMPT},
  {"explain", EXPLAIN},
  {"extent", EXTENT},
  {"externally", EXTERNALLY},
  {"fetch", FETCH},
  {"flush", FLUSH},
  {"float", FLOAT},
  {"file", FILE_KEY},
  {"for", FOR},
  {"force", FORCE},
  {"foreign", FOREIGN},
  {"fortran", FORTRAN},
  {"found", FOUND},
  {"freelist", FREELIST},
  {"freelists", FREELISTS},
  {"from", FROM},
  {"function", FUNCTION},
  {"globally", GLOBALLY},
  {"go", GO},
  {"goto", GOTO},
  {"grant", GRANT},
  {"group", GROUP},
  {"groups", GROUPS},
  {"having", HAVING},
  {"host", HOST},
  {"identified", IDENTIFIED},
  {"immediate", IMMEDIATE},
  {"increment", INCREMENT},
  {"include", INCLUDE},
  {"index", INDEX},
  {"initial", INITIAL_},
  {"initialized", INITIALIZED},
  {"insert", INSERT},
  {"inserting", INSERTING},
  {"int", INT},
  {"integer", INTEGER},
  {"intersect", INTERSECT},
  {"into", INTO},
  {"including", INCLUDING},
  {"indicator", INDICATOR},
  {"initrans", INITRANS},
  {"instance", INSTANCE},
  {"is", IS},
  {"issuer", ISSUER},
  {"key", KEY},
  {"language", LANGUAGE},
  {"layer", LAYER},
  {"level", LEVEL},
  {"link", LINK},
  {"like", LIKE},
  {"lists", LISTS},
  {"lock", LOCK},
  {"logfile", LOGFILE},
  {"long", LONG},
  {"manage", MANAGE},
  {"manual", MANUAL},
  {"max", MAX},
  {"maxdatafiles", MAXDATAFILES},
  {"maxextents", MAXEXTENTS},
  {"maxinstances", MAXINSTANCES},
  {"maxlogfiles", MAXLOGFILES},
  {"maxloghistory", MAXLOGHISTORY},
  {"maxlogmembers", MAXLOGMEMBERS},
  {"maxtrans", MAXTRANS},
  {"maxvalue", MAXVALUE},
  {"min", MIN},
  {"minextents", MINEXTENTS},
  {"minvalue", MINVALUE},
  {"minus", MINUS},
  {"mode", MODE},
  {"modify", MODIFY},
  {"module", MODULE},
  {"mount", MOUNT},
  {"new", NEW},
  {"next", NEXT},
  {"noaudit", NOAUDIT},
  {"noarchivelog", NOARCHIVELOG},
  {"nocache", NOCACHE},
  {"nocompress", NOCOMPRESS},
  {"nocycle", NOCYCLE},
  {"nomaxvalue", NOMAXVALUE},
  {"nominvalue", NOMINVALUE},
  {"none", NONE},
  {"noorder", NOORDER},
  {"noresetlogs", NORESETLOGS},
  {"nosort", NOSORT},
  {"not", NOT},
  {"notfound", NOTFOUND},
  {"number", NUMBER},
  {"numeric", NUMERIC},
  {"of", OF},
  {"off", OFF},
  {"offline", OFFLINE},
  {"old", OLD},
  {"on", ON},
  {"online", ONLINE},
  {"only", ONLY},
  {"open", OPEN},
  {"optimal", OPTIMAL},
  {"option", OPTION},
  {"or", OR},
  {"order", ORDER},
  {"own", OWN},
  {"package", PACKAGE_KEY},
  {"parallel", PARALLEL},
  {"passing", PASSING},
  {"password_lock_time", PASSWORD_LOCK_TIME},
  {"password_verify_function", PASSWORD_VERIFY_FUNCTION},
  {"pctincrease", PCTINCREASE},
  {"pctfree", PCTFREE},
  {"pctused", PCTUSED},
  {"plan", PLAN},
  {"pli", PLI},
  {"pivot", PIVOT},
  {"precision", PRECISION},
  {"primary", PRIMARY},
  {"private", PRIVATE},
  {"prior", PRIOR},
  {"privileges", PRIVILEGES},
  {"procedure", PROCEDURE},
  {"profile", PROFILE},
  {"public", PUBLIC},
  {"quota", QUOTA},
  {"raw", RAW},
  {"read", READ},
  {"real", REAL},
  {"recover", RECOVER},
  {"references", REFERENCES},
  {"referencing", REFERENCING},
  {"rename", RENAME},
  {"reject", REJECT},
  {"resetlogs", RESETLOGS},
  {"resource", RESOURCE},
  {"restricted", RESTRICTED},
  {"revoke", REVOKE},
  {"reuse", REUSE},
  {"remote_oss", REMOTE_OSS},
  {"role", ROLE},
  {"roles", ROLES},
  {"rollback", ROLLBACK},
  {"row", ROW},
  {"rowid", ROWID},
  {"rowlabel", ROWLABEL},
  {"rownum", ROWNUM},
  {"rows", ROWS},
  {"savepoint", SAVEPOINT},
  {"schema", SCHEMA},
  {"scn", SCN},
  {"section", SECTION},
  {"select", SELECT},
  {"segment", SEGMENT},
  {"sequence", SEQUENCE},
  {"session", SESSION},
  {"set", SET},
  {"sets", SETS},
  {"share", SHARE},
  {"shared", SHARED},
  {"snapshot", SNAPSHOT},
  {"size", SIZE},
  {"smallint", SMALLINT},
  {"some", SOME},
  {"sort", SORT},
  {"sql", SQL},
  {"sqlcode", SQLCODE},
  {"sqlerror", SQLERROR},
  {"sqlstate", SQLSTATE},
  {"statement_id", STATEMENT_ID},
  {"statistics", STATISTICS},
  {"start", START},
  {"stop", STOP},
  {"storage", STORAGE},
  {"successful", SUCCESSFUL},
  {"sum", SUM},
  {"switch", SWITCH},
  {"synonym", SYNONYM},
  {"sysdate", SYSDATE},
  {"system", SYSTEM},
  {"systimestamp", SYSTIMESTAMP},
  {"sets", SETS},
  {"table", TABLE},
  {"tables", TABLES},
  {"tablespace", TABLESPACE},
  {"temporary", TEMPORARY},
  {"then", THEN},
  {"thread", THREAD},
  {"time", TIME},
  {"to", TO},
  {"tracing", TRACING},
  {"transaction", TRANSACTION},
  {"trigger", TRIGGER},
  {"triggers", TRIGGERS},
  {"truncate", TRUNCATE},
  {"under", UNDER},
  {"unlimited", UNLIMITED},
  {"union", UNION},
  {"unique", UNIQUE},
  {"unpivot", UNPIVOT},
  {"until", UNTIL},
  {"uid", UID},
  {"update", UPDATE},
  {"updating", UPDATING},
  {"use", USE},
  {"user", USER},
  {"using", USING},
  {"validate", VALIDATE},
  {"values", VALUES},
  {"varchar", VARCHAR},
  {"varchar2", VARCHAR2},
  {"view", VIEW},
  {"when", WHEN},
  {"whenever", WHENEVER},
  {"where", WHERE},
  {"work", WORK},
  {"write", WRITE},
  {"with", WITH},
  {"within", WITHIN},
/*
 * OB Non-Reserved Keywords
 * 新添加的非保留关键字，请按照顺序添加到下面的数组中
 */
  {"account", ACCOUNT},
  {"accessible", ACCESSIBLE},
  {"action", ACTION},
  {"active", ACTIVE},
  {"activate", ACTIVATE},
  {"adddate", ADDDATE},
  {"administer", ADMINISTER},
  {"against", AGAINST},
  {"aggregate", AGGREGATE},
  {"algorithm", ALGORITHM},
  {"allow", ALLOW},
  {"always", ALWAYS},
  {"analyse", ANALYSE},
  {"any", ANY},
  {"array", ARRAY},
  {"approx_count_distinct", APPROX_COUNT_DISTINCT},
  {"approx_count_distinct_synopsis", APPROX_COUNT_DISTINCT_SYNOPSIS},
  {"approx_count_distinct_synopsis_merge", APPROX_COUNT_DISTINCT_SYNOPSIS_MERGE},
  {"ascii", ASCII},
  {"asensitive", ASENSITIVE},
  {"asis", ASIS},
  {"at", AT},
  {"authors", AUTHORS},
  {"auto", AUTO},
  {"autoextend_size", AUTOEXTEND_SIZE},
  {"avg_row_length", AVG_ROW_LENGTH},
  {"backup_copies", BACKUP_COPIES},
  {"balance", BALANCE},
  {"base", BASE},
  {"baseline", BASELINE},
  {"baseline_id", BASELINE_ID},
  {"basic", BASIC},
  {"binlog", BINLOG},
  {"binary", BINARY},
  {"binary_double", BINARY_DOUBLE},
  {"binary_double_infinity", BINARY_DOUBLE_INFINITY},
  {"binary_double_nan", BINARY_DOUBLE_NAN},
  {"binary_float", BINARY_FLOAT},
  {"binary_float_infinity", BINARY_FLOAT_INFINITY},
  {"binary_float_nan", BINARY_FLOAT_NAN},
  {"bit", BIT},
  {"blob", BLOB},
  {"block_size", BLOCK_SIZE},
  {"bool", BOOL},
  {"boolean", BOOLEAN},
  {"bootstrap", BOOTSTRAP},
  {"both", BOTH},
  {"breadth", BREADTH},
  {"btree", BTREE},
  {"bulk", BULK},
  {"bulk_exceptions", BULK_EXCEPTIONS},
  {"bulk_rowcount", BULK_ROWCOUNT},
  {"badfile", BADFILE},
  {"byte", BYTE},
  {"calc_partition_id", CALC_PARTITION_ID},
  {"call", CALL},
  {"cascaded", CASCADED},
  {"case", CASE},
  {"cast", CAST},
  {"catalog_name", CATALOG_NAME},
  {"chain", CHAIN},
  {"changed", CHANGED},
  {"charset", CHARSET},
  {"char_cs", CHAR_CS},
  {"checksum", CHECKSUM},
  {"cipher", CIPHER},
  {"class_origin", CLASS_ORIGIN},
  {"clean", CLEAN},
  {"clear", CLEAR},
  {"client", CLIENT},
  {"clob", CLOB},
  {"cluster", CLUSTER},
  {"coalesce", COALESCE},
  {"code", CODE},
  {"collate", COLLATE},
  {"collation", COLLATION},
  {"collect", COLLECT},
  {"column_format", COLUMN_FORMAT},
  {"column_name", COLUMN_NAME},
  {"column_outer_join_symbol", COLUMN_OUTER_JOIN_SYMBOL},
  {"columns", COLUMNS},
  {"committed", COMMITTED},
  {"compact", COMPACT},
  {"completion", COMPLETION},
  {"compressed", COMPRESSED},
  {"compression", COMPRESSION},
  {"compute", COMPUTE},
  {"concurrent", CONCURRENT},
  {"conditional", CONDITIONAL},
  {"connect", CONNECT},
  {"connection", CONNECTION},
  {"consistent", CONSISTENT},
  {"constraint_catalog", CONSTRAINT_CATALOG},
  {"constraint_name", CONSTRAINT_NAME},
  {"constraint_schema", CONSTRAINT_SCHEMA},
  {"contains", CONTAINS},
  {"context", CONTEXT},
  {"content", CONTENT},
  {"contributors", CONTRIBUTORS},
  {"copy", COPY},
  {"corr", CORR},
  {"covar_pop", COVAR_POP},
  {"covar_samp", COVAR_SAMP},
  {"cpu", CPU},
  {"create_timestamp", CREATE_TIMESTAMP},
  {"cross", CROSS},
  {"cube", CUBE},
  {"cume_dist", CUME_DIST},
  {"current_schema", CURRENT_SCHEMA},
  {"current_date", CURRENT_DATE},
  {"current_timestamp", CURRENT_TIMESTAMP},
  {"current_user", CURRENT_USER},
  {"data", DATA},
  {"data_table_id", DATA_TABLE_ID},
  {"databases", DATABASES},
  {"database_id", DATABASE_ID},
  {"date_add", DATE_ADD},
  {"date_sub", DATE_SUB},
  {"datetime", DATETIME},
  {"day", DAY},
  {"day_hour", DAY_HOUR},
  {"day_microsecond", DAY_MICROSECOND},
  {"day_minute", DAY_MINUTE},
  {"day_second", DAY_SECOND},
  {"deallocate", DEALLOCATE},
  {"default_auth", DEFAULT_AUTH},
  {"defaults", DEFAULTS},
  {"definer", DEFINER},
  {"delay", DELAY},
  {"delayed", DELAYED},
  {"delay_key_write", DELAY_KEY_WRITE},
  {"depth", DEPTH},
  {"des_key_file", DES_KEY_FILE},
  {"describe", DESCRIBE},
  {"destination", DESTINATION},
  {"deterministic", DETERMINISTIC},
  {"dense_rank", DENSE_RANK},
  {"description", DESCRIPTION},
  {"diagnostics", DIAGNOSTICS},
  {"dictionary", DICTIONARY},
  {"directory", DIRECTORY},
  {"disallow", DISALLOW},
  {"discard", DISCARD},
  {"disk", DISK},
  {"distinctrow", DISTINCTROW},
  {"div", DIV},
  {"do", DO},
  {"document", DOCUMENT},
  {"dot", DOT},
  {"dual", DUAL},
  {"dumpfile", DUMPFILE},
  {"duplicate", DUPLICATE},
  {"dynamic", DYNAMIC},
  {"default_tablegroup", DEFAULT_TABLEGROUP},
  {"debug", DEBUG},
  {"e", E},
  {"effective", EFFECTIVE},
  {"elseif", ELSEIF},
  {"enable_extended_rowid", ENABLE_EXTENDED_ROWID},
  {"enclosed", ENCLOSED},
  {"encoding", ENCODING},
  {"encryption", ENCRYPTION},
  {"ends", ENDS},
  {"engine", ENGINE_},
  {"engines", ENGINES},
  {"entityescaping", ENTITYESCAPING},
  {"enum", ENUM},
  {"error", ERROR_P},
  {"error_index", ERROR_INDEX},
  {"error_code", ERROR_CODE},
  {"errors", ERRORS},
  {"estimate", ESTIMATE},
  {"escaped", ESCAPED},
  {"evalname", EVALNAME},
  {"event", EVENT},
  {"every", EVERY},
  {"exchange", EXCHANGE},
  {"exclude", EXCLUDE},
  {"exit", EXIT},
  {"expansion", EXPANSION},
  {"expire", EXPIRE},
  {"expire_info", EXPIRE_INFO},
  {"export", EXPORT},
  {"external", EXTERNAL},
  {"extended", EXTENDED},
  {"extended_noaddr", EXTENDED_NOADDR},
  {"extent_size", EXTENT_SIZE},
  {"extra", EXTRA},
  {"extract", EXTRACT},
  {"extractvalue", EXTRACTVALUE},
  {"fast", FAST},
  {"failed_login_attempts", FAILED_LOGIN_ATTEMPTS},
  {"faults", FAULTS},
  {"fields", FIELDS},
  {"field_delimiter", FIELD_DELIMITER},
  {"field_optionally_enclosed_by", FIELD_OPTIONALLY_ENCLOSED_BY},
  {"file_id", FILE_ID},
  {"final_count", FINAL_COUNT},
  {"first", FIRST},
  {"first_value", FIRST_VALUE},
  {"fixed", FIXED},
  {"flashback", FLASHBACK},
  {"float4", FLOAT4},
  {"float8", FLOAT8},
  {"follower", FOLLOWER},
  {"format", FORMAT},
  {"freeze", FREEZE},
  {"frequency", FREQUENCY},
  {"frozen", FROZEN},
  {"following", FOLLOWING},
  {"full", FULL},
  {"g", G},
  {"general", GENERAL},
  {"generated", GENERATED},
  {"geometry", GEOMETRY},
  {"geometrycollection", GEOMETRYCOLLECTION},
  {"get", GET},
  {"get_format", GET_FORMAT},
  {"global", GLOBAL},
  {"global_alias", GLOBAL_ALIAS},
  {"grants", GRANTS},
  {"grouping", GROUPING},
  {"grouping_id", GROUPING_ID},
  {"group_id", GROUP_ID},
  {"gts", GTS},
  {"handler", HANDLER},
  {"hash", HASH},
  {"help", HELP},
  {"hide", HIDE},
  {"hidden", HIDDEN},
  {"high", HIGH},
  {"high_priority", HIGH_PRIORITY},
  {"hour_microsecond", HOUR_MICROSECOND},
  {"hour_minute", HOUR_MINUTE},
  {"hour_second", HOUR_SECOND},
  {"host", HOST},
  {"hosts", HOSTS},
  {"hour", HOUR},
  {"hybrid_hist", HYBRID_HIST},
  {"id", ID},
  {"idc", IDC},
  {"identity", IDENTITY},
  {"indexed", INDEXED},
  {"if", IF},
  {"ifignore", IFIGNORE},
  {"ignore", IGNORE},
  {"ignore_server_ids", IGNORE_SERVER_IDS},
  {"ilogcache", ILOGCACHE},
  {"import", IMPORT},
  {"in", IN},
  {"incr", INCR},
  {"include", INCLUDE},
  {"incremental", INCREMENTAL},
  {"indent", INDENT},
  {"indexes", INDEXES},
  {"index_table_id", INDEX_TABLE_ID},
  {"info", INFO},
  {"infile", INFILE},
  {"infinite", INFINITE_VALUE},
  {"initial_size", INITIAL_SIZE},
  {"inner", INNER},
  {"inner_parse", INNER_PARSE},
  {"inout", INOUT},
  {"insensitive", INSENSITIVE},
  {"insert_method", INSERT_METHOD},
  {"install", INSTALL},
  {"int1", INT1},
  {"int2", INT2},
  {"int3", INT3},
  {"int4", INT4},
  {"int8", INT8},
  {"intersect", INTERSECT},
  {"invisible", INVISIBLE},
  {"interval", INTERVAL},
  {"invoker", INVOKER},
  {"io", IO},
  {"iops_weight", IOPS_WEIGHT},
  {"io_after_gtids", IO_AFTER_GTIDS},
  {"io_before_gtids", IO_BEFORE_GTIDS},
  {"io_thread", IO_THREAD},
  {"ipc", IPC},
  {"isnull", ISNULL},
  {"isolation", ISOLATION},
  {"isolation_level", ISOLATION_LEVEL},
  {"isopen", ISOPEN},
  {"issuer", ISSUER},
  {"iterate", ITERATE},
  {"job", JOB},
  {"join", JOIN},
  {"json", JSON},
  {"json_array", JSON_ARRAY},
  {"json_equal", JSON_EQUAL},
  {"json_exists", JSON_EXISTS},
  {"json_value", JSON_VALUE},
  {"json_query", JSON_QUERY},
  {"json_mergepatch", JSON_MERGEPATCH},
  {"json_arrayagg", JSON_ARRAYAGG},
  {"json_objectagg", JSON_OBJECTAGG},
  {"json_table", JSON_TABLE},
  {"json_object", JSON_OBJECT},
  {"k", K},
  {"key_block_size", KEY_BLOCK_SIZE},
  {"keys", KEYS},
  {"keystore", KEYSTORE},
  {"key_version", KEY_VERSION},
  {"kill", KILL},
  {"kvcache", KVCACHE},
  {"keep", KEEP},
  {"lag", LAG},
  {"last", LAST},
  {"last_value", LAST_VALUE},
  {"lax", LAX},
  {"lead", LEAD},
  {"leader", LEADER},
  {"leading", LEADING},
  {"leave", LEAVE},
  {"leaves", LEAVES},
  {"left", LEFT},
  {"less", LESS},
  {"level", LEVEL},
  {"lib", LIB},
  {"limit", LIMIT},
  {"linear", LINEAR},
  {"lines", LINES},
  {"linestring", LINESTRING},
  {"line_delimiter", LINE_DELIMITER},
  {"list", LIST},
  {"listagg", LISTAGG},
  {"lnnvl", LNNVL},
  {"load", LOAD},
  {"local", LOCAL},
  {"locality", LOCALITY},
  {"localtimestamp", LOCALTIMESTAMP},
  {"location", LOCATION},
  {"lock_", LOCK_},
  {"locked", LOCKED},
  {"locks", LOCKS},
  {"logonly_replica_num", LOGONLY_REPLICA_NUM},
  {"log", LOG},
  {"logs", LOGS},
  {"longblob", LONGBLOB},
  {"longtext", LONGTEXT},
  {"loop", LOOP},
  {"low", LOW},
  {"low_priority", LOW_PRIORITY},
  {"m", M},
  {"major", MAJOR},
  {"management", MANAGEMENT},
  {"master", MASTER},
  {"master_auto_position", MASTER_AUTO_POSITION},
  {"master_bind", MASTER_BIND},
  {"master_connect_retry", MASTER_CONNECT_RETRY},
  {"master_delay", MASTER_DELAY},
  {"master_heartbeat_period", MASTER_HEARTBEAT_PERIOD},
  {"master_host", MASTER_HOST},
  {"master_log_file", MASTER_LOG_FILE},
  {"master_log_pos", MASTER_LOG_POS},
  {"master_password", MASTER_PASSWORD},
  {"master_port", MASTER_PORT},
  {"master_retry_count", MASTER_RETRY_COUNT},
  {"master_server_id", MASTER_SERVER_ID},
  {"master_ssl", MASTER_SSL},
  {"master_ssl_ca", MASTER_SSL_CA},
  {"master_ssl_capath", MASTER_SSL_CAPATH},
  {"master_ssl_cert", MASTER_SSL_CERT},
  {"master_ssl_cipher", MASTER_SSL_CIPHER},
  {"master_ssl_crl", MASTER_SSL_CRL},
  {"master_ssl_crlpath", MASTER_SSL_CRLPATH},
  {"master_ssl_key", MASTER_SSL_KEY},
  {"master_ssl_verify_server_cert", MASTER_SSL_VERIFY_SERVER_CERT},
  {"master_user", MASTER_USER},
  {"match", MATCH},
  {"matched", MATCHED},
  {"max_connections_per_hour", MAX_CONNECTIONS_PER_HOUR},
  {"max_cpu", MAX_CPU},
  {"log_disk_size", LOG_DISK_SIZE},
  {"max_iops", MAX_IOPS},
  {"memory_size", MEMORY_SIZE},
  {"max_queries_per_hour", MAX_QUERIES_PER_HOUR},
  {"max_rows", MAX_ROWS},
  {"max_size", MAX_SIZE},
  {"max_updates_per_hour", MAX_UPDATES_PER_HOUR},
  {"max_user_connections", MAX_USER_CONNECTIONS},
  {"max_used_part_id", MAX_USED_PART_ID},
  {"median", MEDIAN},
  {"medium", MEDIUM},
  {"mediumblob", MEDIUMBLOB},
  {"mediumint", MEDIUMINT},
  {"mediumtext", MEDIUMTEXT},
  {"memory", MEMORY},
  {"memstore_percent", MEMSTORE_PERCENT},
  {"memtable", MEMTABLE},
  {"merge", MERGE},
  {"message_text", MESSAGE_TEXT},
  {"meta", META},
  {"microsecond", MICROSECOND},
  {"middleint", MIDDLEINT},
  {"migrate", MIGRATE},
  {"migration", MIGRATION},
  {"min_cpu", MIN_CPU},
  {"min_iops", MIN_IOPS},
  {"min_rows", MIN_ROWS},
  {"minor", MINOR},
  {"minute", MINUTE},
  {"minute_microsecond", MINUTE_MICROSECOND},
  {"minute_second", MINUTE_SECOND},
  {"mismatch", MISMATCH},
  {"missing", MISSING},
  {"mod", MOD},
  {"modifies", MODIFIES},
  {"month", MONTH},
  {"move", MOVE},
  {"movement", MOVEMENT},
  {"multilinestring", MULTILINESTRING},
  {"multipoint", MULTIPOINT},
  {"multipolygon", MULTIPOLYGON},
  {"multiset", MULTISET},
  {"mutex", MUTEX},
  {"mysql_errno", MYSQL_ERRNO},
  {"my_name", MY_NAME},
  {"name", NAME},
  {"names", NAMES},
  {"namespace", NAMESPACE},
  {"nan", NAN_VALUE},
  {"national", NATIONAL},
  {"natural", NATURAL},
  {"nchar", NCHAR},
  {"nchar_cs", NCHAR_CS},
  {"ndb", NDB},
  {"ndbcluster", NDBCLUSTER},
  {"nested", NESTED},
  {"no", NO},
  {"noentityescaping", NOENTITYESCAPING},
  {"noschemacheck", NOSCHEMACHECK},
  {"no_parallel", NO_PARALLEL},
  {"no_rewrite", NO_REWRITE},
  {"no_wait", NO_WAIT},
  {"no_write_to_binlog", NO_WRITE_TO_BINLOG},
  {"nologging", NOLOGGING},
  {"nodegroup", NODEGROUP},
  {"noparallel", NOPARALLEL},
  {"now", NOW},
  {"nowait", NOWAIT},
  {"nulls", NULLS},
  {"null_if", NULL_IF_EXETERNAL },
  {"nvarchar2", NVARCHAR2},
  {"ntile", NTILE},
  {"nth_value", NTH_VALUE},
  {"object", OBJECT},
  {"occur", OCCUR},
  {"ordinality", ORDINALITY},
  {"offset", OFFSET},
  {"old_key", OLD_KEY},
  {"over", OVER},
  {"old_password", OLD_PASSWORD},
  {"oltp", OLTP},
  {"one", ONE},
  {"one_shot", ONE_SHOT},
  {"options", OPTIONS},
  {"optimize", OPTIMIZE},
  {"optionally", OPTIONALLY},
  {"ora_rowscn", ORA_ROWSCN},
  {"orig_default", ORIG_DEFAULT},
  {"out", OUT},
  {"outer", OUTER},
  {"outfile", OUTFILE},
  {"outline", OUTLINE},
  {"owner", OWNER},
  {"p", P},
  {"pack_keys", PACK_KEYS},
  {"page", PAGE},
  {"parameters", PARAMETERS},
  {"param_assign_operator", PARAM_ASSIGN_OPERATOR},
  {"parser", PARSER},
  {"partial", PARTIAL},
  {"partition", PARTITION},
  {"partition_id", PARTITION_ID},
  {"partitioning", PARTITIONING},
  {"partitions", PARTITIONS},
  {"password", PASSWORD},
  {"password_grace_time", PASSWORD_GRACE_TIME},
  {"password_life_time", PASSWORD_LIFE_TIME},
  {"password_lock_time", PASSWORD_LOCK_TIME},
  {"password_verify_function", PASSWORD_VERIFY_FUNCTION},
  {"path", PATH},
  {"pattern", PATTERN},
  {"pause", PAUSE},
  {"percent", PERCENT},
  {"percentage", PERCENTAGE},
  {"percentile_cont", PERCENTILE_CONT},
  {"percentile_disc", PERCENTILE_DISC},
  {"percent_rank", PERCENT_RANK},
  {"phase", PHASE},
  {"planregress", PLANREGRESS},
  {"plugin", PLUGIN},
  {"plugin_dir", PLUGIN_DIR},
  {"plugins", PLUGINS},
  {"plus", PLUS},
  {"pivot", PIVOT},
  {"policy", POLICY},
  {"point", POINT},
  {"polygon", POLYGON},
  {"pool", POOL},
  {"port", PORT},
  {"position", POSITION},
  {"prepare", PREPARE},
  {"preserve", PRESERVE},
  {"pretty", PRETTY },
  {"pretty_color", PRETTY_COLOR },
  {"prev", PREV},
  {"primary_zone", PRIMARY_ZONE},
  {"privilege", PRIVILEGE},
  {"process", PROCESS},
  {"processlist", PROCESSLIST},
  {"profiles", PROFILES},
  {"progressive_merge_num", PROGRESSIVE_MERGE_NUM},
  {"proxy", PROXY},
  {"preceding", PRECEDING},
  {"purge", PURGE},
  {"quarter", QUARTER},
  {"query", QUERY},
  {"quick", QUICK},
  {"rank", RANK},
  {"range", RANGE},
  {"ratio_to_report", RATIO_TO_REPORT},
  {"read_consistency", READ_CONSISTENCY},
  {"read_write", READ_WRITE},
  {"reads", READS},
  {"read_only", READ_ONLY},
  {"rebuild", REBUILD},
  {"recovery_window", RECOVERY_WINDOW},
  {"recursive", RECURSIVE},
  {"recycle", RECYCLE},
  {"recyclebin", RECYCLEBIN},
  {"redaction", REDACTION},
  {"redo_buffer_size", REDO_BUFFER_SIZE},
  {"redofile", REDOFILE},
  {"redundancy", REDUNDANCY},
  {"redundant", REDUNDANT},
  {"refresh", REFRESH},
  {"regexp_like", REGEXP_LIKE},
  {"regr_slope", REGR_SLOPE},
  {"regr_intercept", REGR_INTERCEPT},
  {"regr_count", REGR_COUNT},
  {"regr_r2", REGR_R2},
  {"regr_avgx", REGR_AVGX},
  {"regr_avgy", REGR_AVGY},
  {"regr_sxx", REGR_SXX},
  {"regr_syy", REGR_SYY},
  {"regr_sxy", REGR_SXY},
  {"relay", RELAY},
  {"relay_log_file", RELAY_LOG_FILE},
  {"relay_log_pos", RELAY_LOG_POS},
  {"relay_thread", RELAY_THREAD},
  {"relaylog", RELAYLOG},
  {"release", RELEASE},
  {"reload", RELOAD},
  {"remove", REMOVE},
  {"reorganize", REORGANIZE},
  {"repair", REPAIR},
  {"repeat", REPEAT},
  {"repeatable", REPEATABLE},
  {"replace", REPLACE},
  {"replica", REPLICA},
  {"replica_num", REPLICA_NUM},
  {"replica_type", REPLICA_TYPE},
  {"duplicate_scope", DUPLICATE_SCOPE},
  {"replication", REPLICATION},
  {"report", REPORT},
  {"require", REQUIRE},
  {"reset", RESET},
  {"resignal", RESIGNAL},
  {"resource", RESOURCE},
  {"resource_pool_list", RESOURCE_POOL_LIST},
  {"respect", RESPECT},
  {"restart", RESTART},
  {"restore", RESTORE},
  {"restrict", RESTRICT},
  {"resume", RESUME},
  {"return", RETURN},
  {"returned_sqlstate", RETURNED_SQLSTATE},
  {"returns", RETURNS},
  {"reverse", REVERSE},
  {"rlike", RLIKE},
  {"right", RIGHT},
  {"rollup", ROLLUP},
  {"root", ROOT},
  {"rootservice", ROOTSERVICE},
  {"roottable", ROOTTABLE},
  {"routine", ROUTINE},
  {"rowcount", ROWCOUNT},
  {"row_count", ROW_COUNT},
  {"row_format", ROW_FORMAT},
  {"row_number", ROW_NUMBER},
  {"rtree", RTREE},
  {"run", RUN},
  {"sample", SAMPLE},
  {"search", SEARCH},
  {"scalars", SCALARS},
  {"schedule", SCHEDULE},
  {"schema", SCHEMAS},
  {"schemacheck", SCHEMACHECK},
  {"schema_name", SCHEMA_NAME},
  {"scope", SCOPE},
  {"second", SECOND},
  {"second_microsecond", SECOND_MICROSECOND},
  {"security", SECURITY},
  {"seed", SEED},
  {"sensitive", SENSITIVE},
  {"separator", SEPARATOR},
  {"serial", SERIAL},
  {"serializable", SERIALIZABLE},
  {"server", SERVER},
  {"server_ip", SERVER_IP},
  {"server_port", SERVER_PORT},
  {"server_type", SERVER_TYPE},
  {"session_alias", SESSION_ALIAS},
  {"session_user", SESSION_USER},
  {"binding", BINDING},
  {"set_master_cluster", SET_MASTER_CLUSTER},
  {"set_slave_cluster", SET_SLAVE_CLUSTER},
  {"set_tp", SET_TP},
  {"show", SHOW},
  {"shutdown", SHUTDOWN},
  {"shrink", SHRINK},
  {"signal", SIGNAL},
  {"signed", SIGNED},
  {"simple", SIMPLE},
  {"slave", SLAVE},
  {"size", SIZE},
  {"skip", SKIP},
  {"skip_blank_lines", SKIP_BLANK_LINES},
  {"skip_header", SKIP_HEADER},
  {"slow", SLOW},
  {"skewonly", SKEWONLY},
  {"socket", SOCKET},
  {"soname", SONAME},
  {"sounds", SOUNDS},
  {"source", SOURCE},
  {"space", SPACE},
  {"spatial", SPATIAL},
  {"specific", SPECIFIC},
  {"statements", STATEMENTS},
  {"stddev", STDDEV},
  {"stddev_pop", STDDEV_POP},
  {"stddev_samp", STDDEV_SAMP},
  {"strict", STRICT},
  {"spfile", SPFILE},
  {"split", SPLIT},
  {"sqlexception", SQLEXCEPTION},
  {"sqlwarning", SQLWARNING},
  {"sql_big_result", SQL_BIG_RESULT},
  {"sql_calc_found_row", SQL_CALC_FOUND_ROW},
  {"sql_calc_found_rows", SQL_CALC_FOUND_ROWS},
  {"sql_small_result", SQL_SMALL_RESULT},
  {"sql_after_gtids", SQL_AFTER_GTIDS},
  {"sql_after_mts_gaps", SQL_AFTER_MTS_GAPS},
  {"sql_before_gtids", SQL_BEFORE_GTIDS},
  {"sql_buffer_result", SQL_BUFFER_RESULT},
  {"sql_cache", SQL_CACHE},
  {"sql_id", SQL_ID},
  {"sql_no_cache", SQL_NO_CACHE},
  {"sql_thread", SQL_THREAD},
  {"sql_tsi_day", SQL_TSI_DAY},
  {"sql_tsi_hour", SQL_TSI_HOUR},
  {"sql_tsi_minute", SQL_TSI_MINUTE},
  {"sql_tsi_month", SQL_TSI_MONTH},
  {"sql_tsi_quarter", SQL_TSI_QUARTER},
  {"sql_tsi_second", SQL_TSI_SECOND},
  {"sql_tsi_week", SQL_TSI_WEEK},
  {"sql_tsi_year", SQL_TSI_YEAR},
  {"ssl", SSL},
  {"straight_join", STRAIGHT_JOIN},
  {"starting", STARTING},
  {"starts", STARTS},
  {"stats_auto_recalc", STATS_AUTO_RECALC},
  {"stats_persistent", STATS_PERSISTENT},
  {"stats_sample_pages", STATS_SAMPLE_PAGES},
  {"status", STATUS},
  {"storage_format_version", STORAGE_FORMAT_VERSION},
  {"stored", STORED},
  {"storing", STORING},
  {"strong", STRONG},
  {"standby", STANDBY},
  {"subclass_origin", SUBCLASS_ORIGIN},
  {"subdate", SUBDATE},
  {"subject", SUBJECT},
  {"subpartition", SUBPARTITION},
  {"subpartitions", SUBPARTITIONS},
  {"substr", SUBSTR},
  {"super", SUPER},
  {"suspend", SUSPEND},
  {"swaps", SWAPS},
  {"switches", SWITCHES},
  {"switchover", SWITCHOVER},
  {"sysbackup", SYSBACKUP},
  {"sysdba", SYSDBA},
  {"syskm", SYSKM},
  {"sysoper", SYSOPER},
  {"system_user", SYSTEM_USER},
  {"systimestamp", SYSTIMESTAMP},
  {"t", T},
  {"tablegroup", TABLEGROUP},
  {"table_checksum", TABLE_CHECKSUM},
  {"table_mode", TABLE_MODE},
  {"table_id", TABLE_ID},
  {"table_name", TABLE_NAME},
  {"tablegroups", TABLEGROUPS},
  {"tablegroup_id", TABLEGROUP_ID},
  {"tablet", TABLET},
  {"tablet_max_size", TABLET_MAX_SIZE},
  {"task", TASK},
  {"template", TEMPLATE},
  {"temptable", TEMPTABLE},
  {"tenant", TENANT},
  {"terminated", TERMINATED},
  {"text", TEXT},
  {"than", THAN},
  {"ties", TIES},
  {"timestamp", TIMESTAMP},
  {"timestampadd", TIMESTAMPADD},
  {"timestampdiff", TIMESTAMPDIFF},
  {"timezone_abbr", TIMEZONE_ABBR},
  {"timezone_hour", TIMEZONE_HOUR},
  {"timezone_minute", TIMEZONE_MINUTE},
  {"timezone_region", TIMEZONE_REGION},
  {"tinyblob", TINYBLOB},
  {"tinytext", TINYTEXT},
  {"tablet_size", TABLET_SIZE},
  {"top_k_fre_hist", TOP_K_FRE_HIST},
  {"tp_name", TP_NAME},
  {"tp_no", TP_NO},
  {"trace", TRACE},
  {"traditional", TRADITIONAL},
  {"trailing", TRAILING},
  {"translate", TRANSLATE},
  {"trigger", TRIGGER},
  {"trim", TRIM},
  {"trim_space", TRIM_SPACE},
  {"treat", TREAT},
  {"type", TYPE},
  {"typename", TYPENAME},
  {"types", TYPES},
  {"uncommitted", UNCOMMITTED},
  {"unconditional", UNCONDITIONAL},
  {"undefined", UNDEFINED},
  {"undo", UNDO},
  {"undo_buffer_size", UNDO_BUFFER_SIZE},
  {"undofile", UNDOFILE},
  {"unicode", UNICODE},
  {"uninstall", UNINSTALL},
  {"unit", UNIT},
  {"unit_group", UNIT_GROUP},
  {"unit_num", UNIT_NUM},
  {"unknown", UNKNOWN},
  {"unlock", UNLOCK},
  {"unlocked", UNLOCKED},
  {"unpivot", UNPIVOT},
  {"unusual", UNUSUAL},
  {"updatexml",UPDATEXML},
  {"upgrade", UPGRADE},
  {"usage", USAGE},
  {"use_bloom_filter", USE_BLOOM_FILTER},
  {"use_frm", USE_FRM},
  {"user_resources", USER_RESOURCES},
  {"utc_date", UTC_DATE},
  {"utc_timestamp", UTC_TIMESTAMP},
  {"unbounded", UNBOUNDED},
  {"urowid", UROWID},
  {"valid", VALID},
  {"value", VALUE},
  {"var_pop", VAR_POP},
  {"var_samp", VAR_SAMP},
  {"varcharacter", VARCHAR},
  {"varying", VARYING},
  {"variables", VARIABLES},
  {"variance", VARIANCE},
  {"verbose", VERBOSE},
  {"version", VERSION},
  {"materialized", MATERIALIZED},
  {"virtual", VIRTUAL},
  {"visible", VISIBLE},
  {"wait", WAIT},
  {"warnings", WARNINGS},
  {"weak", WEAK},
  {"week", WEEK},
  {"weight_string", WEIGHT_STRING},
  {"wellformed", WELLFORMED},
  {"without", WITHOUT},
  {"wmsys", WMSYS},
  {"wm_concat", WM_CONCAT},
  {"wrapper", WRAPPER},
  {"while", WHILE},
  {"x509", X509},
  {"xa", XA},
  {"xml", XML},
  {"xmlattributes", XMLATTRIBUTES},
  {"xmlcast", XMLCAST},
  {"xmlelement", XMLELEMENT},
  {"xmlagg", XMLAGG},
  {"xmlparse", XMLPARSE},
  {"xmlserialize", XMLSERIALIZE},
  {"xmltype", XMLTYPE},
  {"xor", XOR},
  {"year_month", YEAR_MONTH},
  {"year", YEAR},
  {"zerofill", ZEROFILL},
  {"zone", ZONE},
  {"zone_list", ZONE_LIST},
  {"time_zone_info", TIME_ZONE_INFO},
  {"connect_by_isleaf", CONNECT_BY_ISLEAF},
  {"connect_by_iscycle", CONNECT_BY_ISCYCLE},
  {"connect_by_root", CONNECT_BY_ROOT},
  {"sys_connect_by_path", SYS_CONNECT_BY_PATH},
  {"siblings", SIBLINGS},
  {"zone_type", ZONE_TYPE},
  {"returning", RETURNING},
  {"sessiontimezone", SESSIONTIMEZONE},
  {"dbtimezone", DBTIMEZONE},
  {"rely", RELY},
  {"norely", NORELY},
  {"novalidate", NOVALIDATE},
  {"member", MEMBER},
  {"submultiset", SUBMULTISET},
  {"empty", EMPTY},
  {"empty_field_as_null", EMPTY_FIELD_AS_NULL},
  {"a", A},
  {"throttle", THROTTLE},
  {"priority", PRIORITY},
  {"rt", RT},
  {"network", NETWORK},
  {"logical_reads", LOGICAL_READS},
  {"queue_time", QUEUE_TIME}
};

const NonReservedKeyword *oracle_non_reserved_keyword_lookup(const char *word)
{
  return find_word(word, oracle_keywords_root, Oracle_non_reserved_keywords);
}

int oracle_sql_reserved_keyword_lookup(const char *word)
{
  int ret = -1;
  if (casesame_cstr("SET", word)) {
    ret = SET;
  } else if (casesame_cstr("SELECT", word)) {
    ret = SELECT;
  } else if (casesame_cstr("INSERT", word)) {
    ret = INSERT;
  } else if (casesame_cstr("UPDATE", word)) {
    ret = UPDATE;
  } else if (casesame_cstr("DELETE", word)) {
    ret = DELETE;
  //merge is not reserved keyword in oracle
  // } else if (casesame_cstr("MERGE", word)) {
  //   ret = MERGE;
  } else if (casesame_cstr("WITH", word)) {
    ret = WITH;
  }
  return ret;
}

//return 0 if succ, return 1 if fail
int create_oracle_trie_tree()
{
  return create_trie_tree(Oracle_non_reserved_keywords, LENGTH_OF(Oracle_non_reserved_keywords), &oracle_keywords_root);
}

void  __attribute__((constructor)) init_oracle_non_reserved_keywords_tree()
{
  int ret = 0;
  if (0 != (ret = create_oracle_trie_tree())) {
    (void)printf("ERROR build oracle_non_reserved_keywords tree failed=>%d", ret);
  }
}

static t_node *oracle_reserved_keywords_root = NULL;

static const ReservedKeyword Oracle_reserved_keywords[] = {
  /*
  * oracle reserved keyword, for dbms_utility.cannonize and resvered keyword view
  */
  { "access", ACCESS },
  { "add", ADD },
  { "all", ALL },
  { "alter", ALTER },
  { "and", AND },
  { "any", ANY },
  // { "arraylen", ARRAYLEN },
  { "as", AS },
  { "asc", ASC },
  { "audit", AUDIT },
  { "between", BETWEEN },
  // { "bfile", BFILE },
  { "blob", BLOB },
  { "by", BY },
  { "binary_double", BINARY_DOUBLE },
  { "binary_float", BINARY_FLOAT },
  { "char", CHAR },
  { "check", CHECK },
  { "clob", CLOB },
  { "cluster", CLUSTER },
  { "column", COLUMN },
  { "comment", COMMENT },
  { "compress", COMPRESS },
  { "connect", CONNECT },
  { "create", CREATE },
  { "current", CURRENT },
  { "date", DATE },
  { "decimal", DECIMAL },
  { "default", DEFAULT },
  { "delete", DELETE },
  { "desc", DESC },
  { "distinct", DISTINCT },
  { "drop", DROP },
  { "else", ELSE },
  { "exclusive", EXCLUSIVE },
  { "exists", EXISTS },
  { "file_key", FILE_KEY },
  { "float", FLOAT },
  { "for", FOR },
  { "from", FROM },
  { "grant", GRANT },
  { "group", GROUP },
  { "having", HAVING },
  { "identified", IDENTIFIED },
  { "immediate", IMMEDIATE },
  { "in", IN },
  { "increment", INCREMENT },
  { "index", INDEX },
  { "initial_", INITIAL_ },
  { "insert", INSERT },
  { "integer", INTEGER },
  { "intersect", INTERSECT },
  { "into", INTO },
  { "is", IS },
  { "level", LEVEL },
  { "like", LIKE },
  { "lock", LOCK },
  { "long", LONG },
  { "maxextents", MAXEXTENTS },
  { "minus", MINUS },
  { "mode", MODE },
  { "modify", MODIFY },
  // { "nclob", NCLOB },
  { "noaudit", NOAUDIT },
  { "nocompress", NOCOMPRESS },
  { "not", NOT },
  { "notfound", NOTFOUND },
  { "nowait", NOWAIT },
  { "null", NULLX },
  { "number", NUMBER },
  { "of", OF },
  { "offline", OFFLINE },
  { "on", ON },
  { "online", ONLINE },
  { "option", OPTION },
  { "or", OR },
  { "order", ORDER },
  { "pctfree", PCTFREE },
  { "prior", PRIOR },
  { "privileges", PRIVILEGES },
  { "public", PUBLIC },
  { "raw", RAW },
  { "rename", RENAME },
  { "resource", RESOURCE },
  { "revoke", REVOKE },
  { "row", ROW },
  { "rowid", ROWID },
  { "rowlabel", ROWLABEL },
  { "rownum", ROWNUM },
  { "rows", ROWS },
  { "start", START },
  { "select", SELECT },
  { "session", SESSION },
  { "set", SET },
  { "share", SHARE },
  { "size", SIZE },
  { "smallint", SMALLINT },
  // { "sqlbuf", SQLBUF },
  { "successful", SUCCESSFUL },
  { "synonym", SYNONYM },
  { "sysdate", SYSDATE },
  { "systimestamp", SYSTIMESTAMP },
  { "some", SOME },
  { "table", TABLE },
  { "then", THEN },
  { "to", TO },
  { "trigger", TRIGGER },
  { "uid", UID },
  { "union", UNION },
  { "unique", UNIQUE },
  { "update", UPDATE },
  { "user", USER },
  { "validate", VALIDATE },
  { "values", VALUES },
  { "varchar", VARCHAR },
  { "varchar2", VARCHAR2 },
  { "view", VIEW },
  { "whenever", WHENEVER },
  { "where", WHERE },
  { "with", WITH },
  { "isopen", ISOPEN },
  { "rowcount", ROWCOUNT },
  { "bulk_rowcount", BULK_ROWCOUNT },
  { "error_index", ERROR_INDEX },
  { "bulk_exceptions", BULK_EXCEPTIONS },
  { "case", CASE },
  { "current_date", CURRENT_DATE },
  { "current_timestamp", CURRENT_TIMESTAMP },
  { "dual", DUAL },
  { "localtimestamp", LOCALTIMESTAMP }
};

const ReservedKeyword *oracle_reserved_keyword_lookup(const char *word)
{
  return find_word(word, oracle_reserved_keywords_root, Oracle_reserved_keywords);
}
int create_oracle_reserved_trie_tree()
{
  return create_trie_tree(Oracle_reserved_keywords, LENGTH_OF(Oracle_reserved_keywords), &oracle_reserved_keywords_root);
}

void  __attribute__((constructor)) init_oracle_reserved_keywords_tree()
{
  int ret = 0;
  if (0 != (ret = create_oracle_reserved_trie_tree())) {
    (void)printf("ERROR build oracle_non_reserved_keywords tree failed=>%d", ret);
  }
}

struct FuncPair{
  char *func_name_;
  int yytokentype_;
};
//兼容Oracle 将"FUN_NAME"(xx) 和 func_name(xx) 的行为
//添加新的窗口函数需要在此处以及window_function_name_compare函数中添加对应逻辑
//使用二分查找， 所以请按照字典序添加
static const struct FuncPair FUNCLIST[WINDOW_FUNCTION_NUM]=
{
  {"APPROX_COUNT_DISTINCT", APPROX_COUNT_DISTINCT},
  {"APPROX_COUNT_DISTINCT_SYNOPSIS", APPROX_COUNT_DISTINCT_SYNOPSIS},
  {"APPROX_COUNT_DISTINCT_SYNOPSIS_MERGE", APPROX_COUNT_DISTINCT_SYNOPSIS_MERGE},
  {"AVG", AVG},{"CORR", CORR},{"COUNT", COUNT},{"COVAR_POP", COVAR_POP},
  {"COVAR_SAMP", COVAR_SAMP},{"CUME_DIST", CUME_DIST},{"DENSE_RANK", DENSE_RANK},
  {"FIRST_VALUE", FIRST_VALUE},{"LAG", LAG},{"LAST_VALUE", LAST_VALUE},
  {"LEAD", LEAD},{"LISTAGG", LISTAGG},{"MAX", MAX},{"MEDIAN", MEDIAN},
  {"MIN", MIN},{"NTH_VALUE", NTH_VALUE},{"NTILE", NTILE},{"OVER", OVER},
  {"PERCENT_RANK", PERCENT_RANK},{"RANK", RANK},{"RATIO_TO_REPORT", RATIO_TO_REPORT},
  {"REGR_AVGX", REGR_AVGX},{"REGR_AVGY", REGR_AVGY},{"REGR_COUNT", REGR_COUNT},
  {"REGR_INTERCEPT", REGR_INTERCEPT},{"REGR_R2", REGR_R2},{"REGR_SLOPE", REGR_SLOPE},
  {"REGR_SXX", REGR_SXX},{"REGR_SXY", REGR_SXY},{"REGR_SYY", REGR_SYY},
  {"ROW_NUMBER", ROW_NUMBER},{"STDDEV", STDDEV},{"STDDEV_POP", STDDEV_POP},
  {"STDDEV_SAMP", STDDEV_SAMP},{"SUM", SUM},{"VARIANCE", VARIANCE},
  {"VAR_POP", VAR_POP},{"VAR_SAMP", VAR_SAMP}
};

int binary_search(const struct FuncPair *window_func, int64_t begin, int64_t end, const char *value)
{
  int result = -1;
  int need_break = 0;
  while(!need_break && begin <= end) {
    int mid = begin + (end - begin) / 2;
    int cmp = strcmp(window_func[mid].func_name_, value);
    if (cmp > 0) {
      end = mid - 1;
    } else if (cmp < 0) {
      begin = mid + 1;
    } else {
      result = window_func[mid].yytokentype_;
      need_break = 1;
    }
  }
  return result;
}

int window_function_name_compare(const char *dup_value, int *window_fun_idx)
{
  int compared = 0;
  if (NULL == dup_value) {
  } else {
    *window_fun_idx = binary_search(FUNCLIST, 0, WINDOW_FUNCTION_NUM - 1, dup_value);
    if (*window_fun_idx > 0) {
      compared = 1;
    }
  }

  return compared;
}
