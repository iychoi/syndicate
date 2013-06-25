/*
   Copyright 2013 The Trustees of Princeton University
   All Rights Reserved
*/


#ifndef _LIBSYNDICATE_H_
#define _LIBSYNDICATE_H_

#define __STDC_FORMAT_MACROS

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <memory>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <dirent.h>
#include <utime.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <attr/xattr.h>
#include <pthread.h>
#include <pwd.h>
#include <math.h>
#include <locale>
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>

using namespace std;

#include <curl/curl.h>
#include <uriparser/Uri.h>

#include "util.h"
#include "microhttpd.h"
#include "ms.pb.h"
#include "serialization.pb.h"
#include "ms-client.h"

#define WHERESTR "[%16s:%04u] %s: "
#define WHEREARG __FILE__, __LINE__, __func__

extern int _DEBUG_SYNDICATE;
extern int _ERROR_SYNDICATE;

#ifdef dbprintf
#undef dbprintf
#endif

#ifdef errorf
#undef errorf
#endif

#define dbprintf( format, ... ) do { if( _DEBUG_SYNDICATE ) { printf( WHERESTR format, WHEREARG, __VA_ARGS__ ); fflush(stdout); } } while(0)
#define errorf( format, ... ) do { if( _ERROR_SYNDICATE ) { fprintf(stderr, WHERESTR format, WHEREARG, __VA_ARGS__); fflush(stderr); } } while(0)
#define QUOTE(x) #x

#define dbval(var, fmt) dbprintf(WHERESTR "%s = " fmt, WHEREARG, #var, var);
#define DIE 0xdeadbeef

#define NULLCHECK( var, ret ) \
  if( (var) == NULL ) {       \
      errorf("%s = NULL\n", #var);      \
      if( (ret) == DIE ) {       \
         exit(1);              \
      }                          \
      return ret;             \
  }                           

#define CALLOC_LIST(type, count) (type*)calloc( sizeof(type) * (count), 1 )
#define FREE_LIST(list) do { for(unsigned int __i = 0; (list)[__i] != NULL; ++ __i) { if( (list)[__i] != NULL ) { free( (list)[__i] ); (list)[__i] = NULL; }} free( (list) ); } while(0)
#define SIZE_LIST(sz, list) for( *(sz) = 0; (list)[*(sz)] != NULL; ++ *(sz) );
#define VECTOR_TO_LIST(ret, vec, type) do { ret = CALLOC_LIST(type, ((vec).size() + 1)); for( vector<type>::size_type __i = 0; __i < (vec).size(); ++ __i ) ret[__i] = (vec).at(__i); } while(0)
#define COPY_LIST(dst, src, duper) do { for( unsigned int __i = 0; (src)[__i] != NULL; ++ __i ) { (dst)[__i] = duper((src)[__i]); } } while(0)
#define DUP_LIST(type, dst, src, duper) do { unsigned int sz = 0; SIZE_LIST( &sz, src ); dst = CALLOC_LIST( type, sz + 1 ); COPY_LIST( dst, src, duper ); } while(0)

// for testing
#define BEGIN_TIMING_DATA(ts) clock_gettime( CLOCK_MONOTONIC, &ts )
#define END_TIMING_DATA(ts, ts2, key) clock_gettime( CLOCK_MONOTONIC, &ts2 ); printf("DATA %s %lf\n", key, ((double)(ts2.tv_nsec - ts.tv_nsec) + (double)(1e9 * (ts2.tv_sec - ts.tv_sec))) / 1e9 )
#define DATA(key, value) printf("DATA %s %lf\n", key, value)
#define DATA_S(str) printf("DATA %s\n", str)
#define DATA_BLOCK(name) printf("-------------------------------- %s\n", name)

#define BEGIN_EXTERN_C        extern "C" {
#define END_EXTERN_C          }

using namespace std;


struct md_syndicate_conf; 

#define MD_ENTRY_FILE 1
#define MD_ENTRY_DIR  2

// metadata entry (represents a file or a directory)
struct md_entry {
   int type;            // file or directory?
   char* path;          // path
   char* url;           // URL to the primary replica
   char** url_replicas; // list of replica URLs (NULL if there are none)
   char* local_path;    // only valid for local content--if set, it's the absolute local path on disk where the content can be found
   int64_t ctime_sec;   // creation time (seconds)
   int32_t ctime_nsec;  // creation time (nanoseconds)
   int64_t mtime_sec;   // last-modified time (seconds)
   int32_t mtime_nsec;  // last-modified time (nanoseconds)
   int64_t version;     // publish version
   int32_t max_read_freshness;      // how long is this entry fresh until it needs revalidation?
   int32_t max_write_freshness;     // how long can we delay publishing this entry?
   uid_t owner;         // uid of the owner
   uid_t acting_owner;  // uid of the acting owner
   gid_t volume;        // id of the volume
   mode_t mode;         // file permission bits
   off_t size;          // size of the file
   unsigned char* checksum;      // SHA-1 hash (NULL if not given)
};

typedef list<struct md_entry*> md_entry_list;


// metadata update
// line looks like:
// [command] [timestamp] [entry text]
struct md_update {
   char op;               // update operation
   int64_t timestamp;    // timestamp of when the update was received (if op == MD_OP_ERR, then this is instead a value of errno)
   struct md_entry ent;
};

// metadata update operations
#define MD_OP_ADD    'A'      // add/replace an entry
#define MD_OP_RM     'R'      // remove an entry
#define MD_OP_UP     'U'      // update an existing entry
#define MD_OP_ERR    'E'      // (response only) error encountered in processing the entry
#define MD_OP_VER    'V'      // write verification request
#define MD_OP_USR    'S'      // special (context-specific) state
#define MD_OP_NEWBLK 'B'      // new block written

// download buffer
struct md_download_buf {
   ssize_t len;         // amount of data
   ssize_t data_len;    // size of data (if data was preallocated)
   char* data;    // NOT null-terminated
};

// POST buffer
struct md_post_buf {
   char* text;
   int offset;
   int len;
};

// basic linked list
struct md_linked_list {
   void* what;
   struct md_linked_list* next;
};

// HTTP server user account entry
struct md_user_entry {
   uid_t uid;
   char* username;
   char* password_hash;
};

#define MD_GUEST_UID 0xFFFFFFFF

// chubby path locks
struct md_path_locks {
   vector< long >* path_locks;
   pthread_mutex_t path_locks_lock;
   pthread_mutex_t wait_lock;
   int wait_count;
};

// directory listing
struct md_dir_listing {
   struct dirent** namelist;
   int num_entries;
};

// HTTP handler args
struct md_HTTP_handler_args {
   struct md_syndicate_conf* conf;
   struct md_user_entry** users;
};


// HTTP headers
struct md_HTTP_header {
   char* header;
   char* value;
};

// HTTP file response
struct md_HTTP_file_response {
   union {
      FILE* f;
      int fd;
   };
   off_t offset;
   size_t size;
};


// small message response buffers
typedef pair<char*, size_t> buffer_segment_t;
typedef vector< buffer_segment_t > response_buffer_t;

// ssize_t (*)(void* cls, uint64_t pos, char* buf, size_t max)
typedef MHD_ContentReaderCallback md_HTTP_stream_callback;

// void (*)(void* cls)
typedef MHD_ContentReaderFreeCallback md_HTTP_free_cls_callback;

// HTTP stream response
struct md_HTTP_stream_response {
   md_HTTP_stream_callback scb;
   md_HTTP_free_cls_callback fcb;
   
   void* cls;
   size_t blk_size;
   uint64_t size;
};
   

// HTTP response (to be populated by handlers)
struct md_HTTP_response {
   int status;
   struct MHD_Response* resp;
};

struct md_HTTP;


// HTTP connection data
struct md_HTTP_connection_data {
   struct md_HTTP* http;
   struct md_syndicate_conf* conf;
   struct MHD_PostProcessor* pp;
   struct md_user_entry* user;
   struct md_HTTP_response* resp;
   struct md_HTTP_header** headers;
   char const* remote_host;
   char const* method;
   size_t content_length;
   
   int status;
   int mode;
   off_t offset;              // if there isn't a post-processor, then this stores the offset 
   
   void* cls;                 // user-supplied closure
   char* version;             // HTTP version
   char* url_path;            // path requested
   char* query_string;        // url's query string
   response_buffer_t* rb;     // response buffer for small messages
};

// HTTP callbacks and control code
struct md_HTTP {

   pthread_rwlock_t lock;
   
   struct md_syndicate_conf* conf;
   struct md_user_entry** users;
   int authentication_mode;
   struct MHD_Daemon* http_daemon;
   int server_type;   // one of the MHD options

   char* server_cert;
   char* server_pkey;
   
   void*                     (*HTTP_connect)( struct md_HTTP_connection_data* md_con_data );
   struct md_HTTP_response*  (*HTTP_HEAD_handler)( struct md_HTTP_connection_data* md_con_data );
   struct md_HTTP_response*  (*HTTP_GET_handler)( struct md_HTTP_connection_data* md_con_data );
   int                       (*HTTP_POST_iterator)(void *coninfo_cls, enum MHD_ValueKind kind, 
                                                   char const *key,
                                                   char const *filename, char const *content_type,
                                                   char const *transfer_encoding, char const *data, 
                                                   uint64_t off, size_t size);
   void                      (*HTTP_POST_finish)( struct md_HTTP_connection_data* md_con_data );
   int                       (*HTTP_PUT_iterator)(void *coninfo_cls, enum MHD_ValueKind kind,
                                                  char const *key,
                                                  char const *filename, char const *content_type,
                                                  char const *transfer_encoding, char const *data,
                                                  uint64_t off, size_t size);
   void                      (*HTTP_PUT_finish)( struct md_HTTP_connection_data* md_con_data );
   struct md_HTTP_response*  (*HTTP_DELETE_handler)( struct md_HTTP_connection_data* md_con_data, int depth );
   void                      (*HTTP_cleanup)(struct MHD_Connection *connection, void *con_cls, enum MHD_RequestTerminationCode term);
};

extern char const* MD_HTTP_NOMSG;
extern char const* MD_HTTP_200_MSG;
extern char const* MD_HTTP_400_MSG;
extern char const* MD_HTTP_401_MSG;
extern char const* MD_HTTP_403_MSG;
extern char const* MD_HTTP_404_MSG;
extern char const* MD_HTTP_409_MSG;
extern char const* MD_HTTP_413_MSG;
extern char const* MD_HTTP_422_MSG;
extern char const* MD_HTTP_500_MSG;
extern char const* MD_HTTP_501_MSG;
extern char const* MD_HTTP_504_MSG;

extern char const* MD_HTTP_DEFAULT_MSG;

#define MD_HTTP_TYPE_STATEMACHINE   MHD_USE_SELECT_INTERNALLY
#define MD_HTTP_TYPE_THREAD         MHD_USE_THREAD_PER_CONNECTION

#define MD_MIN_POST_DATA 4096

#define md_HTTP_connect( http, callback ) (http).HTTP_connect = (callback)
#define md_HTTP_GET( http, callback ) (http).HTTP_GET_handler = (callback)
#define md_HTTP_HEAD( http, callback ) (http).HTTP_HEAD_handler = (callback)
#define md_HTTP_POST_iterator( http, callback ) (http).HTTP_POST_iterator = (callback)
#define md_HTTP_POST_finish( http, callback ) (http).HTTP_POST_finish = (callback)
#define md_HTTP_PUT_iterator( http, callback ) (http).HTTP_PUT_iterator = (callback)
#define md_HTTP_PUT_finish( http, callback ) (http).HTTP_PUT_finish = (callback)
#define md_HTTP_DELETE( http, callback ) (http).HTTP_DELETE_handler = (callback)
#define md_HTTP_close( http, callback ) (http).HTTP_cleanup = (callback)
#define md_HTTP_auth_mode( http, auth_mode ) (http).authentication_mode = auth_mode
#define md_HTTP_server_type( http, type ) (http).server_type = type

// server configuration
struct md_syndicate_conf {
   // client fields
   int64_t default_read_freshness;                    // default number of milliseconds a file can age before needing refresh for reads
   int64_t default_write_freshness;                   // default number of milliseconds a file can age before needing refresh for writes
   char* logfile_path;                                // path to the logfile
   char* cdn_prefix;                                  // CDN prefix
   char* proxy_url;                                   // URL to a proxy to use (instead of a CDN)
   bool gather_stats;                                 // gather statistics or not?
   bool use_checksums;                                // if set, send checksums of our local files in the metadata, and validate checksums (if possible) from remote files
   char* content_url;                                 // what is the URL under which published files can be accessed?
   char* data_root;                                   // root of the path where we store local file blocks
   char* staging_root;                                // root of the path where we store locally-written rmeote file blocks 
   int num_replica_threads;                           // how many replica threads?
   char* replica_logfile;                             // path on disk to replica log
   int httpd_portnum;                                 // port number for the httpd interface (syndicate-httpd only)

   // gateway server
   bool verify_peer;                                  // whether or not to verify the gateway server's SSL certificate with peers
   unsigned int num_http_threads;                     // how many HTTP threads to create
   int http_authentication_mode;                      // for which operations do we authenticate?
   char* md_pidfile_path;                             // where to store the PID file for the gateway server
   char* gateway_metadata_root;                       // location on disk (if desired) to record metadata
   bool replica_overwrite;                            // overwrite replica file at the client's request
   char* server_key;                                  // path to PEM-encoded SSL private key for this gateway server
   char* server_cert;                                 // path to PEM-encoded SSL certificate for this gateway server

   // debug
   int debug_read;                                    // print verbose information for reads
   int debug_lock;                                    // print verbose information on locks

   // common
   char* volume_name;                                 // name of the volume
   int metadata_connect_timeout;                      // number of seconds to wait to connect on the control plane
   int portnum;                                       // Syndicate-side port number
   int transfer_timeout;                              // how long a transfer is allowed to take
   
   // runtime fields
   char* metadata_url;                                // URL (or path on disk) where to get the metadata
   char* metadata_username;                           // metadata authentication username
   char* metadata_password;                           // metadata authentication password
   char** replica_urls;                               // where to send file replicas
   uint64_t blocking_factor;                          // how many bytes blocks will be
   uid_t owner;                                       // what is our UID in Syndicate?  Files created in this UG will assume this UID as their owner
   mode_t usermask;                                   // umask of the user running this program
   gid_t volume;                                      // volume ID of the metadata server we're subscribed to
   uid_t volume_owner;                                // user ID of the metadata server we're subscribed to
   char* mountpoint;                                  // absolute path to the place where the metadata server is mounted
   char* hostname;                                    // what's our hostname?
   uint64_t volume_version;                           // version of the volume state
   char* ag_driver;				      // AG gatway driver that encompasses gateway callbacks
};


// authentication (can be OR'ed together)
#define HTTP_AUTHENTICATE_NONE        0
#define HTTP_AUTHENTICATE_READ        1
#define HTTP_AUTHENTICATE_WRITE       2
#define HTTP_AUTHENTICATE_READWRITE   3

// types of responses
#define HTTP_RESPONSE_RAM              1
#define HTTP_RESPONSE_RAM_NOCOPY       2
#define HTTP_RESPONSE_RAM_STATIC       3
#define HTTP_RESPONSE_FILE             4
#define HTTP_RESPONSE_FD               5
#define HTTP_RESPONSE_CALLBACK         6

// mode for connection data
#define MD_HTTP_GET      0
#define MD_HTTP_POST     1
#define MD_HTTP_PUT      2
#define MD_HTTP_HEAD     3
#define MD_HTTP_DELETE   4

#define MD_HTTP_UNKNOWN  -1

#define COMMENT_KEY                 '#'

#define DEBUG_KEY                   "DEBUG"
#define DEBUG_READ_KEY              "DEBUG_READ"
#define DEBUG_LOCK_KEY              "DEBUG_LOCK"

// config elements 
#define DEFAULT_READ_FRESHNESS_KEY  "DEFAULT_READ_FRESHNESS"
#define DEFAULT_WRITE_FRESHNESS_KEY "DEFAULT_WRITE_FRESHNESS"
#define METADATA_URL_KEY            "METADATA_URL"
#define LOGFILE_PATH_KEY            "LOGFILE"
#define CDN_PREFIX_KEY              "CDN_PREFIX"
#define PROXY_URL_KEY               "PROXY_URL"
#define PREFER_USER_KEY             "PRESERVE_USER_FILES"
#define GATHER_STATS_KEY            "GATHER_STATISTICS"
#define PUBLISH_BUFSIZE_KEY         "PUBLISH_BUFFER_SIZE"
#define METADATA_USERNAME_KEY       "METADATA_USERNAME"
#define METADATA_PASSWORD_KEY       "METADATA_PASSWORD"
#define METADATA_UID_KEY            "METADATA_UID"
#define USE_CHECKSUMS_KEY           "USE_CHECKSUMS"
#define DATA_ROOT_KEY               "DATA_ROOT"
#define STAGING_ROOT_KEY            "STAGING_ROOT"

#define METADATA_CONNECT_TIMEOUT_KEY   "METADATA_CONNECT_TIMEOUT"

#define REPLICA_URL_KEY             "REPLICA_URL"
#define NUM_REPLICA_THREADS_KEY     "NUM_REPLICA_THREADS"
#define REPLICA_LOGFILE_KEY         "REPLICA_LOGFILE"
#define BLOCKING_FACTOR_KEY         "BLOCKING_FACTOR"
#define REPLICATION_FACTOR_KEY      "REPLICATION_FACTOR"
#define TRANSFER_TIMEOUT_KEY        "TRANSFER_TIMEOUT"
#define NUM_HTTP_THREADS_KEY        "HTTP_THREADPOOL_SIZE"

#define PORTNUM_KEY                 "PORTNUM"
#define HTTPD_PORTNUM_KEY           "HTTPD_PORTNUM"
#define SSL_PKEY_KEY                "SSL_PKEY"
#define SSL_CERT_KEY                "SSL_CERT"
#define AUTH_OPERATIONS_KEY         "AUTH_OPERATIONS"
#define PIDFILE_KEY                 "PIDFILE"
#define VOLUME_NAME_KEY             "VOLUME_NAME"

// gateway config
#define GATEWAY_METADATA_KEY        "GATEWAY_METADATA"
#define GATEWAY_PORTNUM_KEY         "GATEWAY_PORTNUM"
#define REPLICA_OVERWRITE_KEY       "REPLICA_OVERWRITE"

// misc
#define VERIFY_PEER_KEY             "SSL_VERIFY_PEER"
#define CONTENT_URL_KEY             "PUBLIC_URL"
#define METADATA_UID_KEY            "METADATA_UID"

#define SYNDICATEFS_XATTR_URL          "user.syndicate_url"
#define CLIENT_DEFAULT_CONFIG          "/usr/etc/syndicate/syndicate-UG.conf"
#define AG_GATEWAY_DRIVER_KEY	    "AG_GATEWAY_DRIVER"

// URL protocol prefix for local files
#define SYNDICATEFS_LOCAL_PROTO     "file://"

#define SYNDICATE_DATA_PREFIX "/SYNDICATE-DATA/"
#define SYNDICATE_STAGING_PREFIX "/SYNDICATE-STAGING/"

// name of blacklist file to keep in the master copy
#define BLACKLIST_FILENAME    ".mdblacklist"

// maximum length of a single line of metadata
#define MD_MAX_LINE_LEN       65536

// check to see if a URL refers to local data
#define URL_LOCAL( url ) (strlen(url) > strlen(SYNDICATEFS_LOCAL_PROTO) && strncmp( (url), SYNDICATEFS_LOCAL_PROTO, strlen(SYNDICATEFS_LOCAL_PROTO) ) == 0)

// extract the absolute, underlying path from a local url
#define GET_PATH( url ) ((char*)(url) + strlen(SYNDICATEFS_LOCAL_PROTO))

// extract the filesystem path from a local url
#define GET_FS_PATH( root, url ) ((char*)(url) + strlen(SYNDICATEFS_LOCAL_PROTO) + strlen(root) - 1)

// is this a file path?
#define IS_FILE_PATH( path ) (strlen(path) > 1 && (path)[strlen(path)-1] != '/')

// is this a directory path?
#define IS_DIR_PATH( path ) ((strlen(path) == 1 && (path)[0] == '/') || (path)[strlen(path)-1] == '/')

// map a string to an md_entry
typedef struct map<string, struct md_entry*> md_entmap;

// list of path hashes is a set of path locks
typedef vector<long> md_pathlist;

BEGIN_EXTERN_C
   
// library config 
int md_debug( int level );
int md_error( int level );
int md_signals( int use_signals );

// configuration parser
int md_read_conf( char const* conf_path, struct md_syndicate_conf* conf );
int md_read_conf_line( char* line, char** key, char*** values );
int md_free_conf( struct md_syndicate_conf* conf );

// md_entry
struct md_entry* md_entry_dup( struct md_entry* src );
void md_entry_dup2( struct md_entry* src, struct md_entry* ret );
void md_entry_free( struct md_entry* ent );

// path locks
int md_path_locks_create( struct md_path_locks* locks );
int md_path_locks_free( struct md_path_locks* locks );
int md_lock_path( struct md_path_locks* locks, char const* path );
int md_global_lock_path( char const* mdroot, char const* path );
void* md_unlock_path( struct md_path_locks* locks, char const* path );
void* md_global_unlock_path( char const* mdroot, char const* path );

// publishing
int md_publish_file( char const* data_root, char const* publish_root, char const* fs_path, int64_t file_version );
int md_publish_block( char const* data_root, char const* publish_root, char const* fs_path, int64_t file_version, uint64_t block_id, int64_t block_version );
int md_withdraw_block( char const* root, char const* fs_path, int64_t file_version, uint64_t block_id, int64_t block_version );
int md_withdraw_file( char const* root, char const* fs_path, int64_t version );
int md_withdraw_dir( char const* root, char const* fs_path );
char* md_publish_path_file( char const* publish_root, char const* fs_path, int64_t version );
char* md_publish_path_block( char const* publish_root, char const* fs_path, int64_t file_version, uint64_t block_id, int64_t block_version );
char* md_staging_path_block( char const* staging_root, char const* fs_path, int64_t file_version, uint64_t block_id, int64_t block_version );
char* md_full_block_path( char const* root, char const* fs_path, uint64_t block_id );
int64_t md_path_version( char const* path );
int md_path_version_offset( char const* path );
ssize_t md_metadata_update_text( struct md_syndicate_conf* conf, char **buf, struct md_update** updates );
ssize_t md_metadata_update_text2( struct md_syndicate_conf* conf, char **buf, vector<struct md_update>* updates );
ssize_t md_metadata_update_text3( struct md_syndicate_conf* conf, char **buf, struct md_update* (*iterator)( void* ), void* arg );
bool md_is_versioned_form( char const* vanilla_path, char const* versioned_path );
char** md_versioned_paths( char const* base_path );
int64_t* md_versions( char const* base_path );
int64_t md_next_version( char** versioned_publish_paths );
//int64_t md_next_version( int64_t* versions );
char* md_clear_version( char* path );
void md_update_free( struct md_update* update );
void md_update_dup2( struct md_update* src, struct md_update* dest );

// path manipulation
char* md_fullpath( char const* root, char const* path, char* dest );
char* md_dirname( char const* path, char* dest );
char* md_basename( char const* path, char* dest );
int md_depth( char const* path );
int md_basename_begin( char const* path );
int md_dirname_end( char const* path );
char* md_realpath( char const* cwd, char const* path );
char* md_prepend( char const* prefix, char const* str, char* output );
long md_hash( char const* path );
int md_path_split( char const* path, vector<char*>* result );
void md_sanitize_path( char* path );
bool md_is_locally_hosted( struct md_syndicate_conf* conf, char const* url );

// serialization
int md_entry_to_ms_entry( struct md_syndicate_conf* conf, ms::ms_entry* msent, struct md_entry* ent );
int ms_entry_to_md_entry( struct md_syndicate_conf* conf, const ms::ms_entry& msent, struct md_entry* ent );
int md_write_metadata( int fd, struct md_entry** ents );
size_t md_metadata_line_len( struct md_entry* ent );
char* md_to_string( struct md_entry* ent, char* buf );

// directory manipulation
int md_mkdirs( char const* dirp );
int md_mkdirs2( char const* dirp, int off );
int md_mkdirs3( char const* dirp, mode_t mode );
int md_rmdirs( char const* dirp );

// threading
pthread_t md_start_thread( void* (*thread_func)(void*), void* args, bool detach );
int md_daemonize( char* logfile_path, char* pidfile_path, FILE** logfile );

// daemonization
int md_release_privileges();

// HTTP and URL processing
int md_connect_timeout( unsigned long timeout );
ssize_t md_download_file( char const* url, char** buf, int* status_code );
ssize_t md_download_file2( char const* url, char** buf, char const* username, char const* password );
ssize_t md_download_file3( char const* url, int fd, char const* username, char const* password );
ssize_t md_download_file4( char const* url, char** buf, char const* username, char const* password, char const* proxy, void (*curl_extractor)( CURL*, int, void* ), void* arg );
ssize_t md_download_file5( CURL* curl_h, char** buf );
ssize_t md_download_file_proxied( char const* url, char** buf, char const* proxy, int* status_code );
char** md_parse_cgi_args( char* query_string );
char* md_url_hostname( char const* url );
char* md_path_from_url( char const* url );
char* md_fs_path_from_url( char const* url );
char* md_url_strip_path( char const* url );
int md_portnum_from_url( char const* url );
char* md_strip_protocol( char const* url );
char* md_normalize_url( char const* url, int* rc );
int md_normalize_urls( char** urls, char** ret );
size_t md_default_get_callback_ram(void *stream, size_t size, size_t count, void *user_data);
size_t md_default_get_callback_disk(void *stream, size_t size, size_t count, void *user_data);
size_t md_get_callback_response_buffer( void* stream, size_t size, size_t count, void* user_data );
size_t md_default_post_callback(void *ptr, size_t size, size_t nmemb, void *userp);
int md_response_buffer_upload_iterator(void *coninfo_cls, enum MHD_ValueKind kind,
                                       const char *key,
                                       const char *filename, const char *content_type,
                                       const char *transfer_encoding, const char *data,
                                       uint64_t off, size_t size);
char* md_flatten_path( char const* path );
char* md_cdn_url( char const* url );
int md_http_rlock( struct md_HTTP* http );
int md_http_wlock( struct md_HTTP* http );
int md_http_unlock( struct md_HTTP* http );


// HTTP server
int md_create_HTTP_response_ram( struct md_HTTP_response* resp, char const* mimetype, int status, char const* data, int len );
int md_create_HTTP_response_ram_nocopy( struct md_HTTP_response* resp, char const* mimetype, int status, char const* data, int len );
int md_create_HTTP_response_ram_static( struct md_HTTP_response* resp, char const* mimetype, int status, char const* data, int len );
int md_create_HTTP_response_fd( struct md_HTTP_response* resp, char const* mimetype, int status, int fd, off_t offset, size_t size );
int md_create_HTTP_response_stream( struct md_HTTP_response* resp, char const* mimetype, int status, uint64_t size, size_t blk_size, md_HTTP_stream_callback scb, void* cls, md_HTTP_free_cls_callback fcb );
int md_HTTP_response_headers( struct md_HTTP_response* resp, struct md_HTTP_header** headers );
void md_free_HTTP_response( struct md_HTTP_response* resp );
void* md_cls_get( void* cls );
void md_cls_set_status( void* cls, int status );
struct md_HTTP_response* md_cls_set_response( void* cls, struct md_HTTP_response* resp );
int md_HTTP_init( struct md_HTTP* http, int server_type, struct md_syndicate_conf* conf, struct md_user_entry** users );
int md_start_HTTP( struct md_HTTP* http, int portnum );
int md_stop_HTTP( struct md_HTTP* http );
int md_free_HTTP( struct md_HTTP* http );
void md_create_HTTP_header( struct md_HTTP_header* header, char const* h, char const* value );
void md_free_HTTP_header( struct md_HTTP_header* header );
void md_free_download_buf( struct md_download_buf* buf );
void md_init_curl_handle( CURL* curl, char const* url, time_t query_time );
char const* md_find_HTTP_header( struct md_HTTP_header** headers, char const* header );
int md_HTTP_add_header( struct md_HTTP_response* resp, char const* header, char const* value );
int md_HTTP_parse_url_path( char* _url_path, char** file_path, int64_t* file_version, uint64_t* block_id, int64_t* block_version, struct timespec* manifest_timestamp, bool* staging );

// user manipulation
struct md_user_entry** md_parse_secrets_file( char const* path );
struct md_user_entry** md_parse_secrets_file2( FILE* passwd_file, char const* path );
struct md_user_entry* md_user_entry_dup( struct md_user_entry* uent );
void md_free_user_entry( struct md_user_entry* uent );
struct md_user_entry* md_find_user_entry( char const* username, struct md_user_entry** users );
struct md_user_entry* md_find_user_entry2( uid_t uid, struct md_user_entry** users );
bool md_validate_user_password( char const* password, struct md_user_entry* uent );

// response buffers
char* response_buffer_to_string( response_buffer_t* rb );
size_t response_buffer_size( response_buffer_t* rb );
void response_buffer_free( response_buffer_t* rb );

// top-level initialization
int md_init( int gateway_type,
             char const* config_file,
             struct md_syndicate_conf* conf,
             struct ms_client* client,
             struct md_user_entry*** users,
             int portnum,
             char const* ms_url,
             char const* volume_name,
             char const* volume_secret,
             char const* md_username,
             char const* md_password
           );

int md_shutdown(void);
int md_default_conf( struct md_syndicate_conf* conf );
int md_check_conf( int gateway_type, struct md_syndicate_conf* conf );

END_EXTERN_C
   
#define URL_MAX         3000           // maximum length of a URL

// system UID
#define SYS_USER 0

// syndicatefs magic number
#define SYNDICATEFS_MAGIC  0x01191988

#define INVALID_BLOCK_ID (uint64_t)(-1)

// gateway types for md_init
#define SYNDICATE_UG       1
#define SYNDICATE_AG       2
#define SYNDICATE_RG       3

#endif
