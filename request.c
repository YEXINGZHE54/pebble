#include <regex.h>
#include "request.h"
#include "notifier.h"
#include "list.h"
#include "app.h"

int request_start(notifier_block *nb, unsigned long ev, void *d);

int stone_request_init(ngx_pool_t *pool, stone_request_t *req, struct FCGX_Request *fcgx ){
		char *contentData, *temp;
		uint contentLen;

		//getenv( "CONTENT_TYPE" );
		req->env->ContentType = FCGX_GetParam("CONTENT_TYPE",fcgx->envp);
        //getenv( "CONTENT_LENGTH" );
		char *lpszContentLenght = FCGX_GetParam("CONTENT_LENGTH",fcgx->envp);
        req->env->ContentLength = strtol( (lpszContentLenght != 0 ? lpszContentLenght : "0"), 0, 10 );
		contentLen = req->env->ContentLength;

        // Get boundary data if available
        if( req->env->ContentType != 0 )
        {
            char *lpszTemp = index( req->env->ContentType, ';' );
            if( lpszTemp != 0 )
            {
                *lpszTemp = 0;
                if( strncasecmp( lpszTemp + 2, "boundary=", 9 ) == 0 )
                {
					req->env->Boundary = ngx_palloc( pool, STONE_BOUNDARY_LEN_MAX );
                    ngx_strlcpy( req->env->Boundary, lpszTemp + 11, STONE_BOUNDARY_LEN_MAX );
                    req->env->BoundaryLen = strlen( req->env->Boundary );
                }
            }
        }

        // Store query data
        //main_store_data( pool, req->get, getenv( "QUERY_STRING" ) );
		main_store_data( pool, req->get, FCGX_GetParam("QUERY_STRING",fcgx->envp) );

        // Get more data if available (from standart input)
        if( req->env->ContentType != 0 && req->env->ContentLength > 0 )
        {
            // Check lenght
            if( req->env->ContentLength > STONE_UPLOAD_LEN_MAX )
            {
                //log_printf( threadInfo, LOG_WARNING, "Main::Loop: Content lenght is too big (%u > %u)", req->env->ContentLength, NNCMS_UPLOAD_LEN_MAX );
            }
            else
            {
                // Retreive data
                contentData = ngx_palloc( pool, req->env->ContentLength + 1 );
                if( contentData == NULL )
                {
                    //log_printf( threadInfo, LOG_ALERT, "Main::Loop: Unable to allocate %u bytes for content (max: %u)", req->env->ContentLength, NNCMS_UPLOAD_LEN_MAX );
                }
                else
                {
                    //int nResult = FCGI_gets( contentData, req->env->ContentLength, threadInfo->fcgi_request->in );
                    int nResult = FCGX_GetStr( contentData, req->env->ContentLength, fcgx->in );
                    contentData[nResult] = 0;
					/*
                    temp = contentData;
                    do{
						*temp++ = getchar();
					}while( --contentLen != 0);
					
                    //contentData[nResult] = 0;
                    if( strlen( contentData ) < req->env->ContentLength ){
						//log_printf( threadInfo, LOG_WARNING, "Main::Loop: Received %u, required %u. Not enough bytes received on standard input", nResult, req->env->ContentLength );
					}
					*/
					if ( nResult < req->env->ContentLength ){
						
					}
                    if( strcmp( req->env->ContentType, "application/x-www-form-urlencoded" ) == 0 )
                        main_store_data( pool, req->post, contentData );

                    // RFC1867: Form-based File Upload in HTML
                    if( strcmp( req->env->ContentType, "multipart/form-data" ) == 0 )
                        main_store_data_rfc1867( pool, req, contentData );
                }
            }
        }

        // And even more data (from cookies)
        //main_store_data( pool, req->cookie, getenv( "HTTP_COOKIE" ) );
		main_store_data( pool, req->cookie, FCGX_GetParam("HTTP_COOKIE",fcgx->envp) );

	return 0;
}

int module_request_init(void){
	// mkdir for app tmp files
	if ( access( TMP_DIR, 0 ) != 0 ){
		if ( mkdir ( TMP_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH ) )
		{
			printf("Error while creating pool buffer!");
			return -1;
		}
	}
    triger_set(NOTIFIER_START, 0, request_start);
    return 0;
}

int request_start(notifier_block *nb, unsigned long ev, void *d){
    stone_server_t *server = d;
    stone_request_t *req;
    int rc;
    req = request_empty(server->pool);
    if(req == NULL)
        return NOTIFY_STOP;
    rc = stone_request_init(server->pool, req, server->fcgx );
    if(rc != 0)
        return NOTIFY_STOP;
    server->req = req;
    return NOTIFY_DONE; 
}

stone_request_t *request_empty(ngx_pool_t *pool) {
    stone_request_t *result = ngx_palloc(pool, sizeof(stone_request_t));
	result->get = ngx_palloc(pool, sizeof(struct list_head) );
	if (result->get == NULL){
		//error_handler("Error while initializing server info!");
		return NULL;
	}
	INIT_LIST_HEAD(result->get);
	result->post = ngx_palloc(pool, sizeof(struct list_head) );
	if (result->post == NULL){
		//error_handler("Error while initializing server info!");
		return NULL;
	}
	INIT_LIST_HEAD(result->post);
	result->cookie = ngx_palloc(pool, sizeof(struct list_head) );
	if (result->cookie == NULL){
		//error_handler("Error while initializing server info!");
		return NULL;
	}
	INIT_LIST_HEAD(result->cookie);
	result->file = ngx_palloc(pool, sizeof(struct list_head) );
	if (result->file == NULL){
		//error_handler("Error while initializing server info!");
		return NULL;
	}
	INIT_LIST_HEAD(result->file);
	result->env = ngx_palloc(pool, sizeof(struct stone_server_env) );
	if (result->env == NULL){
		//error_handler("Error while initializing server info!");
		return NULL;
	}

    return result;
}

// #############################################################################

int main_add_variable( ngx_pool_t *pool, struct list_head *head, char *Name, void *Value)
{
	int rc;
	rc = 1;
	stone_item_t *item;
	item = ngx_palloc(pool, sizeof(stone_item_t));
	if (item == NULL){
		//error_handler("Error while initializing server info!");
		return rc;
	}
	item->name = ngx_strdup(pool, Name);
	item->value = Value;
	linux_list_add_tail(&(item->ptr), head);
	rc = 0;
    return rc;
}

// #############################################################################

void *main_get_variable( struct list_head *head, char *lpszName )
{
    //for( int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
	stone_item_t *pos;
	list_for_each_entry(pos, head, ptr)
    {
        if( strcmp( pos->name, lpszName ) == 0 )
                return pos->value;
    }

    return 0;
}

// #############################################################################
// like stack, it starts from tail
void *main_pop_variable( struct list_head *head, char *lpszName )
{
	//for( int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
		stone_item_t *pos;
		void *cur;
		cur = NULL;
		list_for_each_entry_reverse(pos, head, ptr)
		{
			if( strcmp( pos->name, lpszName ) == 0 ){
				cur = pos->value;
				list_del( &( pos->ptr ) );
				break;
			}
		}
		
	    return cur;
}

// #############################################################################
// like stack, it starts from tail, return 0 if success
int main_update_variable( struct list_head *head, char *lpszName, void *value )
{
	//for( int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
			stone_item_t *pos;
			void *cur;
			int rc = 1;
			list_for_each_entry_reverse(pos, head, ptr)
			{
				if( strcmp( pos->name, lpszName ) == 0 ){
					pos->value = value;
					rc = 0;
					break;
				}
			}
			
		    return rc;
}

// #############################################################################

char main_from_hex( char c )
{
    return  c >= '0' && c <= '9' ?  c - '0'
            : c >= 'A' && c <= 'F' ? c - 'A' + 10
            : c - 'a' + 10;     // accept small letters just in case
}

// #############################################################################

char *main_unescape( char *str )
{
    char *p = str;
    char *q = str;
    static char blank[] = "";

    if( !str ) return blank;
    while( *p )
    {
        if( *p == '%' )
        {
            p++;
            if( *p ) *q = main_from_hex( *p++ ) * 16;
            if( *p ) *q = (*q + main_from_hex( *p++ ));
            q++;
        }
        else
        {
            if( *p == '+' )
            {
                *q++ = ' ';
                p++;
            }
            else
            {
                *q++ = *p++;
            }
        }
    }

    *q++ = 0;
    return str;
}

// #############################################################################

char *main_escape( ngx_pool_t *pool, const char *s, int len, int *new_length )
{
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;
    unsigned char const hexchars[] = "0123456789ABCDEF";

    from = s;
    end = s + len;
    start = to = (unsigned char *) ngx_palloc(pool, ( 3 * len + 1 ));

    while( from < end )
    {
        c = *from++;

        if( c == ' ' )
        {
            *to++ = '+';
        }
        else if( ( c < '0' && c != '-' && c != '.' ) ||
                 ( c < 'A' && c > '9' ) ||
                 ( c > 'Z' && c < 'a' && c != '_' ) ||
                 ( c > 'z' ) )
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        }
        else
        {
            *to++ = c;
        }
    }
    *to = 0;
    if( new_length )
    {
        *new_length = to - start;
    }
    return (char *) start;
}

// #############################################################################

void main_store_data( ngx_pool_t *pool, struct list_head *head, char *query )
{
    char *cp, *cp2, var[50], *val, *tmpVal;

    if( !query ) return;

    cp = query;
    cp2 = var;
    memset( var, 0, sizeof(var) );
    val = 0;
    while( *cp )
    {
        if( *cp == '=' )
        {
            cp++;
            *cp2 = 0;
            val = cp;
            continue;
        }
        if( *cp == '&' )
        {
            *cp = 0;
            tmpVal = main_unescape( val );
            main_add_variable( pool, head, var, ngx_strdup(pool, tmpVal) );
            cp++;
            cp2 = var;
            val = 0;
            continue;
        }
        if( val )
        {
            cp++;
        }
        else
        {
            *cp2 = *cp++;
            if( *cp2 == '.' )
            {
                strcpy( cp2, "_dot_" );
                cp2 += 5;
            }
            else
            {
                cp2++;
            }
        }
    }
    *cp = 0;
    tmpVal = main_unescape( val );
    main_add_variable( pool, head, var, ngx_strdup(pool, tmpVal) );
}

// #############################################################################

char *main_get_var( char *lpBuf, char *lpszDest, unsigned int nDestSize, char *lpszVarName )
{
    unsigned int i; // Standart iterations counter
    unsigned int nVarNameLen = strlen( lpszVarName );

    // Sample:
    //
    // "Content-Disposition: form-data; name=\"upload_file\"; filename=\".gtkrc\"\r\n
    // Content-Type: application/octet-stream\r\n
    // \r\n
    // gtk-fallback-icon-theme = \"gnome\"\n
    // \r\n",
    // '-' <repeats 29 times>, "1330065118175668826815"...

    if( strncasecmp( lpBuf, lpszVarName, nVarNameLen ) == 0 )
    {
        lpBuf += nVarNameLen;

        // Copy value to destination buffer
        for( i = 0; i < nDestSize; i++ )
        {
            if( (lpBuf[i] == '\r' && lpBuf[i + 1] == '\n') ||
                lpBuf[i] == ';' )
            {
                break;
            }

            // wtf?
            if( lpBuf[i] == 0 ) return 0;

            lpszDest[i] = lpBuf[i];
        }

        // Terminate the string
        if( lpszVarName[nVarNameLen - 1] == '"' ) lpszDest[i - 1] = 0;
        else lpszDest[i] = 0;

        // wtf?
        if( i == nDestSize ) return 0;

        // Skip (';' or CR or LF or CRLF) and ' '
        lpBuf += i; lpBuf++;
        if( *lpBuf == '\n' ) lpBuf++;
        if( *lpBuf == ' ' ) lpBuf++;
    }

    return lpBuf;
}

// #############################################################################

// RFC1867: Form-based File Upload in HTML
void main_store_data_rfc1867( ngx_pool_t *pool, stone_request_t *req, char *query )
{
    // Boundary header values
    char szContentDisposition[256];
    char szContentType[256];
    char szFileName[256];
    char szName[256];
    char *lpBuf = query;
	stone_file_t *curFile;
	uint randtry, randnum;
	char randpath[256];
	ngx_fd_t nfd;
	ssize_t n;
	// init random seeds for file savepath
	srand((unsigned)time(NULL));

find_boundary:

    // Find a boundary and skip it
    while( *lpBuf )
    {
        if( strncmp( lpBuf, req->env->Boundary, req->env->BoundaryLen ) == 0 )
        {
            lpBuf += req->env->BoundaryLen + 2;
            goto parse_header;
        }
        lpBuf++;
    }

    // No more boundaries left
    return;

parse_header:
    // Parse boundary header
    memset( szContentDisposition, 0, sizeof(szContentDisposition) );
    memset( szContentType, 0, sizeof(szContentType) );
    memset( szFileName, 0, sizeof(szFileName) );
    memset( szName, 0, sizeof(szName) );
    while( *lpBuf )
    {
        if( lpBuf[0] == '\r' && lpBuf[1] == '\n' )
        {
            // Variable content starts here
            lpBuf += 2;
			// maybe it comes an end
			if ( *lpBuf == 0 ) return;

            // Find next boundary
            char *szNextBoundary = ngx_strnstr( lpBuf, req->env->Boundary, req->env->ContentLength - (lpBuf - query) );
            if( szNextBoundary == 0 )
            {
                //log_printf( req, LOG_ALERT, "Main::StoreDataRFC1867: Next boundary not found!" );
                return;
            }
            uint nValueLen = (szNextBoundary - 3 ) - lpBuf;// wtf?

            // Create buffer for value
            char *szValue = 0;
            if( nValueLen < STONE_PAGE_SIZE_MAX && *szFileName != 0 )
            {
                szValue = ngx_palloc(pool, nValueLen );
            }
            else if( nValueLen < STONE_UPLOAD_LEN_MAX )
            {
                szValue = ngx_palloc(pool, nValueLen );
            }

            // Copy boundary data to it
            if( szValue != 0 )
            {
                ngx_memcpy( szValue, lpBuf, nValueLen );
                nValueLen--; lpBuf += nValueLen;
                szValue[nValueLen] = 0;

                if( *szFileName == 0 )
                {
                    // Add to global variables if it's not a file
                    main_add_variable( pool, req->post, szName, szValue );
                    //FREE( szValue );
                }
                else
                {
					curFile = NULL;
					randtry = 0;
					nfd = NGX_INVALID_FILE;
					n = 0;
					curFile = ngx_palloc(pool, sizeof(stone_file_t));
					if(curFile == NULL){
						ngx_pfree(pool, szValue);
						continue;
					}

					// allow 10 tries
					while(randtry < 10){
						randtry++;
						randnum = rand();
						ngx_memzero(randpath,sizeof(randpath));
						sprintf(randpath, "%s%d", TMP_DIR, randnum);
						// if exists
						if ( access( randpath, 0 ) == 0 ){
							continue;
						}else{
							break;
						}
					}

					if (randtry >= 10){
						goto failed;
					}

					nfd = ngx_open_file(randpath, NGX_FILE_WRONLY, NGX_FILE_CREATE_OR_OPEN, NGX_FILE_DEFAULT_ACCESS);
					if (nfd == NGX_INVALID_FILE) {
						goto failed;
					}
					n = ngx_write_fd(nfd, szValue, nValueLen);

					if (n == NGX_FILE_ERROR) {
						goto failed;
					}
					if ((size_t) n != nValueLen) {
						goto failed;
					}

					curFile->filename = ngx_strdup(pool, szFileName);
					curFile->filepath = ngx_strdup(pool, randpath);
					curFile->len = nValueLen;
					main_add_variable( pool, req->file, szName, (void *)curFile );

					failed:
					if (nfd != NGX_INVALID_FILE) {
						ngx_close_file(nfd);
					}

					// free large memory
					ngx_pfree(pool, szValue);
                }
            }
            else
            {
                //log_printf( req, LOG_WARNING, "Main::StoreDataRFC1867: Data value (%u bytes) is too big or memory block was not allocated", nValueLen );
            }

            //printf( "%s; %s; %s; %s\n", szContentDisposition, szContentType, szName, szFileName );

            // Try to find next boundary
            goto find_boundary;
        }

        char *lpResult;

        lpResult = main_get_var( lpBuf, szContentDisposition, sizeof(szContentDisposition), "Content-Disposition: " ); if( lpResult == 0 ) { return; } else if( lpResult > lpBuf ) { lpBuf = lpResult; continue; }
        lpResult = main_get_var( lpBuf, szContentType, sizeof(szContentType), "Content-Type: " ); if( lpResult == 0 ) { return; } else if( lpResult > lpBuf ) { lpBuf = lpResult; continue; }
        lpResult = main_get_var( lpBuf, szFileName, sizeof(szFileName), "filename=\"" ); if( lpResult == 0 ) { return; } else if( lpResult > lpBuf ) { lpBuf = lpResult; continue; }
        lpResult = main_get_var( lpBuf, szName, sizeof(szName), "name=\"" ); if( lpResult == 0 ) { return; } else if( lpResult > lpBuf ) { lpBuf = lpResult; continue; }

        lpBuf = strstr( lpBuf, "\r\n\r\n" );
        if( lpBuf == 0 )
        {
            // No variable no content found, looks like syntaxis error, quit
            //log_printf( req, LOG_ALERT, "Main::StoreDataRFC1867: Content of part not found" );
            break;
        }
        lpBuf += 2;
    }

    // wtf?
    //log_printf( req, LOG_ALERT, "Main::StoreDataRFC1867: Unexpected null terminating character in standart input" );
    return;
}

void store_path_info (stone_server_t * server, char *path_info, regmatch_t *matches, size_t n )
{
	char **path, *dest, **cur;
	int i, len;

	path = ngx_palloc( server->pool, n * sizeof( char * ) );
	for( i = 0; i< n; i++ )
	{
		regmatch_t preg = matches[i];
		len = preg.rm_eo - preg.rm_so;
		dest = ngx_palloc( server->pool, len+1 );
		strncpy(dest, path_info + preg.rm_so, len);
		cur = path + i;
		*cur = dest;
	}

	server->req->path = path;
    //memset(dest, 0, 20);
}
