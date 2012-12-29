#include "cache.h"
#include "hiredis/hiredis.h"
#include "opool.h"

extern unsigned int BKDRHash(char *);

char	*cache_read(ngx_command_t *cmd, void *arg){
	char *path = app_cache_dir;
	int fhash = BKDRHash((char *)arg);
	char *cache_file = NULL;
	char             *buf,*from;
    off_t             size;
    size_t            len;
    ssize_t           n;
    ngx_fd_t          fd;
    ngx_file_info_t   fi;

    buf = NULL;
	//cache_file = (char*)ngx_palloc(cmd->pool, 1000);
	//ngx_memzero(cache_file, 1000);
	//sprintf(cache_file, "%s", fhash);
	//strncpy(cache_file, "id1234", 6);
	from = ngx_palloc(cmd->pool, strlen(path) + 12 );
	ngx_memzero(from, strlen(path) + 12);
	sprintf(from, "%s%d", path, fhash);

    fd = ngx_open_file(from, NGX_FILE_RDONLY, NGX_FILE_OPEN, NGX_FILE_DEFAULT_ACCESS);

    if (fd == NGX_INVALID_FILE) {
        //ngx_log_error(NGX_LOG_CRIT, cf->log, ngx_errno, ngx_open_file_n " \"%s\" failed", from);
        goto failed;
    }

        if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_fd_info_n " \"%s\" failed", from);

            goto failed;
        }

        size = ngx_file_size(&fi);


    len = 65536;

    if ((off_t) len > size) {
        len = (size_t) size;
    }

    buf = ngx_palloc(cmd->pool, len+1);
    if (buf == NULL) {
        goto failed;
    }
    ngx_memzero(buf, len+1);
    cmd->data = buf;
	
    while (size > 0) {

        if ((off_t) len > size) {
            len = (size_t) size;
        }

        n = ngx_read_fd(fd, buf, len);

        if (n == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_read_fd_n " \"%s\" failed", from);
            goto failed;
        }

        if ((size_t) n != len) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_read_fd_n " has read only %z of %uz from %s", n, size, from);
            goto failed;
        }
        buf += n;
        size -= n;
    }

success:

    if (fd != NGX_INVALID_FILE) {
        if (ngx_close_file(fd) == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_close_file_n " \"%s\" failed", from);
        }
    }

    return arg;

failed:
    if (cmd->data) cmd->data=NULL;
    goto success;
}

char * cache_write(ngx_command_t *cmd, void *arg){
	char *path = app_cache_dir;
	int fhash;
	ngx_fd_t          nfd;
	char 			*to;
	size_t            len;
	ssize_t           n;

	if (!cmd->data || !arg){
		goto failed;
	}
	fhash = BKDRHash((char *)arg);
	to = ngx_palloc(cmd->pool, strlen(path) + 12 );
	ngx_memzero(to, strlen(path) + 12);
	sprintf(to, "%s%d", path, fhash);
	
	nfd = ngx_open_file(to, NGX_FILE_WRONLY, NGX_FILE_CREATE_OR_OPEN, NGX_FILE_DEFAULT_ACCESS);

    if (nfd == NGX_INVALID_FILE) {
        //ngx_log_error(NGX_LOG_CRIT, cf->log, ngx_errno, ngx_open_file_n " \"%s\" failed", to);
        goto failed;
    }

    len = strlen((char *)(cmd->data));

	n = ngx_write_fd(nfd, (char*)(cmd->data), len);

        if (n == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_write_fd_n " \"%s\" failed", to);
            goto failed;
        }

        if ((size_t) n != len) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_write_fd_n " has written only %z of %uz to %s", n, size, to);
            goto failed;
        }
	success:

    if (nfd != NGX_INVALID_FILE) {
        if (ngx_close_file(nfd) == NGX_FILE_ERROR) {
            //ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno, ngx_close_file_n " \"%s\" failed", to);
        }
    }

    return arg;

	failed:
    //if (cmd->data) cmd->data=NULL;
    goto success;
}

char * cache_delete ( ngx_command_t *cmd, void *arg )
{
	int rc;
	char *path = app_cache_dir, *from;
	int fhash = BKDRHash((char *)arg);

	from = ngx_palloc(cmd->pool, strlen(path) + 12 );
	ngx_memzero(from, strlen(path) + 12);
	sprintf(from, "%s%d", path, fhash);	
	rc = ngx_delete_file ( from );
	if ( rc != 0 ) return CACHE_ERROR;
	return arg;
}

char *redis_read( ngx_command_t *cmd, void *arg )
{
	//int fhash = BKDRHash((char *)arg);
	redisReply *reply;
	char *temp;
	redisContext *c;
	if ( cmd->resource == NULL ) return CACHE_ERROR;
	//c = ( redisContext * ) cmd->resource;
	c = ( redisContext * ) opool_request(cmd->resource);
    if ( c == NULL ) return CACHE_ERROR;
	reply = redisCommand ( c, "get %s", arg );
    opool_release(cmd->resource, c);
	if ( reply == NULL || reply->str == NULL ) return CACHE_ERROR;
	temp = ngx_strdup ( cmd->pool, reply->str );
	if ( temp == NULL ) return CACHE_ERROR;
	cmd->data = temp;
	freeReplyObject ( reply );
	return arg;
}

char * redis_write( ngx_command_t *cmd, void *arg )
{
	redisContext *c;
	redisReply *reply;
	//int fhash = BKDRHash((char *)arg);
	if ( cmd->resource == NULL ) return CACHE_ERROR;
	if ( cmd->data == NULL ) return CACHE_ERROR;
	c = ( redisContext * ) opool_request(cmd->resource);
    //c = cmd->resource;
    if ( c == NULL ) return CACHE_ERROR;
	reply = redisCommand ( c, "set %s %s", arg, cmd->data );
    opool_release(cmd->resource, c);
	if ( reply == NULL ) return CACHE_ERROR;
	freeReplyObject ( reply );
	return arg;
}

char * redis_delete ( ngx_command_t *cmd, void *arg )
{
	redisContext *c;
	redisReply *reply;
	//int fhash = BKDRHash((char *)arg);
	if ( cmd->resource == NULL ) return CACHE_ERROR;
	if ( cmd->data == NULL ) return CACHE_ERROR;
	//c = ( redisContext * ) cmd->resource;
	c = ( redisContext * ) opool_request(cmd->resource);
    if ( c == NULL ) return CACHE_ERROR;
	reply = redisCommand ( c, "del %s", arg );
    opool_release(cmd->resource, c);
	if ( reply == NULL ) return CACHE_ERROR;
	freeReplyObject ( reply );
	return arg;
}

char *db_read(ngx_command_t *cmd, void *arg){

}

char * db_write(ngx_command_t *cmd, void *arg){

}

char * db_delete ( ngx_command_t *cmd, void *arg )
{
	
}

