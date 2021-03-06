#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "hashmap.h"
#include "moddate.h"
#include "path.h"
#include "config.h"
#include "item.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct path_struct
{
    char *path;
    struct path_struct *next;
};
int res = 1;
path *path_create( char *fname )
{
    path *p = calloc( 1, sizeof(path) );
    if ( p != NULL )
    {
        p->path = strdup( fname );
        if ( p->path == NULL )
        {
            free( p );
            p = NULL;
        }
    }
    if ( p == NULL )
        fprintf(stderr,"path creation failed\n");
    return p;
}
void path_dispose( path *p )
{
    if ( p->path != NULL )
        free( p->path );
    free( p );
}
path *path_dispose_all( path *h )
{
    path *p = h;
    while ( p != NULL )
    {
        path *next = p->next;
        path_dispose( p );
        p = next;
    }
    return NULL;
}
/**
 * Derive the path name 
 */
void path_name( path *p, char *name, int limit )
{
    char *dup = strdup( p->path );
    if ( dup != NULL )
    {
        char *token = strtok( dup, "/" );
        char *last = token;
        while ( token != NULL )
        {
            last = token;
            token = strtok( NULL, "/" );
        }
        if ( last != NULL )
            strncpy( name, last, limit );
        else
            strncpy( name, p->path, limit );
        free( dup );
    }
}
char *path_get( path *p )
{
    return p->path;
}
path *path_next( path *p )
{
    return p->next;
}
static char *path_extend( char *p, char *ext, int dispose_old )
{
    int len = 0;
    if ( p != NULL )
        len += strlen(p)+1;
    len += strlen( ext )+1;
    char *new_path = calloc(len,1);
    if ( new_path != NULL )
    {
        if ( p != NULL )
        {
            strcpy( new_path, p );
            strcat( new_path, "/" );
            if ( dispose_old )
                free( p );
        }
        strcat( new_path, ext );
        if ( strlen(new_path)>60)
            printf(">60!\n");
    }
    return new_path;
}
/**
 * Is this a .conf file?
 * @param name the file name to test
 * @return 1 if it is a .conf file else 0
 */
static int is_conf( char *name )
{
    int clen = strlen(name);
    if ( clen > 5 )
    {
        if ( strcmp(&name[clen-5],".conf")==0 )
            return 1;
    }
    return 0;
}
/**
 * Don't accept link files not in HTML or XML or JSON folders
 * @param path the path to check
 * @return 1 if it was OK or we failed to copy the path
 */
static int is_allowed( char *path )
{
    // B: strtok will modify path, so copy it first
    char *npath = strdup(path);
    if ( npath != NULL )
    {
        char *last=NULL;
        char *pent=NULL;
        char *part = strtok(npath,"/");
        last = part;
        while ( part != NULL )
        {
            part = strtok( NULL, "/");
            if ( part != NULL )
            {
                pent = last;
                last = part;
            }
        }
        if ( last == NULL || pent == NULL || 
            (strcmp(last,"link")==0 
                && is_link_format_dir(pent) )
                || strcmp(last,"link")!=0 )
            return 1;
        else
            return 0;
        free( npath );
    }
    else    // allow file if allocation fails
    {
        fprintf(stderr,"%s: %d failed to copy path\n",__FILE__,__LINE__);
        return 1;
    }
}
/**
 * Scan a directory for files. Don't do anything with them yet.
 * @param md contains the date the archive was last uploaded
 * @param dir the directory to scan
 * @param head set to the head of a list of file-paths
 * @param tail set to the current path tail
 * @return 1 if it worked
 */
int path_scan( moddate *md, char *dir, path **head, path **tail )
{
    int res = 1;
    DIR *dp;
    dp = opendir( dir );
    if ( dp != NULL )
    {
        struct dirent *ep = readdir(dp);
        while ( ep != NULL && res )
        {
	        if ( strcmp(ep->d_name,"..")!=0 &&strcmp(ep->d_name,".")!=0
                &&strcmp(ep->d_name,MODDATE_FILE)!=0 )
            {
                if ( is_directory(dir,ep->d_name) )
	            {
                    char *new_path = path_extend( dir, ep->d_name, 0 );
                    if ( new_path != NULL )
                    {
                        res = path_scan( md, new_path, head, tail );
                        free( new_path );
                    }
                    else
                        res = 0;
	            }
                else    // it's a file
                {
                    char *new_path = path_extend( dir, ep->d_name, 0 );
                    if ( new_path != NULL )
                    {
                        if ( md==NULL || is_conf(ep->d_name) 
                            || (moddate_is_later(md,new_path)
                                &&is_allowed(new_path)) )
                        {
                            path *fp = calloc( 1, sizeof(path) );
                            if ( fp != NULL )
                            {
                                fp->path = strdup( new_path );
                                if ( strlen(fp->path)>75 )
                                    printf(">75!\n");
                                if ( fp->path != NULL )
                                {
                                    if ( *head == NULL )
                                        *head = *tail = fp;
                                    else
                                    {
                                        (*tail)->next = fp;
                                        *tail = fp;
                                    }
                                }
                                else
                                    res = 0;
                            }
                            else
                                res = 0;
                        }
                        free( new_path );
                    }
                    else
                        res = 0;
                }
            }
            ep = readdir( dp ); 
        }
    }
    else
        res = 0;
    return res;
}
void path_append( path *fp, char *p )
{
    path *temp = fp;
    while ( temp->next != NULL )
        temp = temp->next;
    temp->next = calloc( 1, sizeof(path) );
    temp = temp->next;
    temp->path = strdup(p);
    if ( strlen(temp->path)>75 )
        printf(">75!\n");
}
static void print_paths( path *head )
{
    path *fp = head;
    while ( fp != NULL )
    {
        printf("%s\n",fp->path );
        fp = fp->next;
    }
}
static char *chomp( char *str )
{
    int str_len = strlen(str);
    char *pos = strrchr(str,'.');
    if ( pos != NULL )
    {
        int suf_len = strlen( pos );
        str = strdup( str );
        str[str_len-suf_len] = 0;
    }
    return str;
}
static int path_ends( char *p, char *suf )
{
    int plen = strlen(p);
    int slen = strlen(suf);
    if ( slen<plen )
        return strcmp(&p[plen-slen],suf)==0;
    else
        return 0;
}
static char *strtoken( char **str, const char *delim )
{
    char *token = *str;
    char *pos = strstr( *str, delim );
    if ( pos != NULL )
    {
        *pos = 0;
        *str = pos+1;
    }
    else
        *str += strlen( *str );
    return token;
}
/**
 * If the literal docid contains a format directory, remove it
 * @param docid an allocate docid
 */
static char *fix_literal_docid( char *docid )
{
    char *ptr = docid;
    char *rep = calloc( strlen(ptr)+1, 1 );
    if ( rep != NULL )
    {
        char *part = strtoken( &ptr, "/" );
        char *pent = NULL;
        int seen_fmt_dir = 0;
        while ( strlen(part) > 0 )
        {
            if ( seen_fmt_dir )
            {
                if ( strcmp(part,"link")!=0 )
                {
                    strcat(rep,"/");
                    strcat(rep,pent);
                    strcat(rep,"/");
                    strcat(rep,part);
                }
                break;
            }
            else if ( !is_link_format_dir(part) )
            {
                if ( strlen(rep)>0 )
                    strcat(rep,"/");
                strcat(rep,part);
            }
            else
            {
                seen_fmt_dir = 1;
                pent = part;
            }
            part = strtoken( &ptr, "/" );
        }
        free( docid );
    }
    else
    {
        fprintf(stderr,"warning %s: line %d failed to allocate\n",__FILE__,__LINE__);
        rep = ptr;
    }
    return rep;
}
/**
 * Parse the path for items
 * @param fp the path object
 * @param hm the hashmap to store items in
 */
static void path_parse( path *fp, hashmap *hm )
{
    item *it;
    int state = 0;
    char *key;
    char *old;
    char *current = NULL;
    char *docid = NULL;
    char *db = NULL;
    char *versionID = NULL;         
    int type = 0;
    char *path_copy = strdup( fp->path );
    if ( path_copy != NULL )
    {
        char *token = strtok( path_copy, "/" );
        while ( token != NULL && state >= 0 )
        {
            switch ( state )
            {
                case 0:
                    if ( strlen(token)>0 )
                    {
                        if ( token[0] == '+' )
                        {
                            docid = path_extend( docid, &token[1], 1 );
                            state = 1;
                        }
                        else if ( token[0] == '@' )
                        {
                            db = strdup( &token[1] );
                            type = LITERAL;
                            state = 2;
                        }
                        else if ( state == '%' )
                        {
                            docid = strdup( &token[1] );
                            state = 3;
                        }
                        // else just build path
                    }
                    break;
                case 1: // extending docid
                    if ( token[0] == '%' )
                    {
                        state = 3;
                        docid = path_extend( docid, &token[1], 1 );
                    }
                    else
                        docid = path_extend( docid, token, 1 );
                    break;
                case 2: // literal path
                    docid = path_extend( docid, token, 1 );
                    if ( !is_directory(current,token) )
                        docid = fix_literal_docid(docid);
                    break;
                case 3: // decide item type
                    if ( strcmp(token,"MVD")==0 )
                        state = 4;
                    else if ( strcmp(token,"TEXT")==0 )
                        state = 5;
                    else if ( strcmp(token,"XML")==0 )
                        state = 6;
                    else if ( strcmp(token,"MIXED")==0 )
                        state = 7;
                    else if ( strcmp(token,"HTML")==0 )
                        state = 10;
                    else if ( !path_ends(token,".conf") )
                    {
                        fprintf(stderr,"path:expected format dir or "
                            ".conf file but found %s/%s\n",
                            current,token);
                        state = -1;
                    }
                    break;
                case 4: // MVD type
                    if ( strcmp(token,"cortex.mvd")==0 )
                        type = MVD_CORTEX;
                    else if ( strcmp(token,"corcode")==0 )
                        state = 8;
                    else if ( !path_ends(token,".conf") )
                    {
                        fprintf(stderr,"path:expected .conf file or "
                            "cortex.mvd or corcode but found %s/%s\n",
                            current,token);
                        state = -1;
                    }
                    break;
                case 5: // TEXT type
                    if ( strcmp(token,"corcode")==0 )
                        state = 9;
                    else if ( strcmp(token,"cortex")==0 )
                    {
                        state = 11;
                        type = TEXT_CORTEX;
                    }
                    else if ( !path_ends(token,".conf") )
                    {
                        fprintf(stderr,"path:unexpected path %s/%s\n",
                            current,token);
                        state = -1;
                    }
                    break;
                case 6: // XML
                    old = token;
                    if ( is_directory(current,token) )
                        versionID = path_extend( versionID, token, 1 );
                    type = XML;
                    break;
                case 7: // MIXED
                    old = token;
                    if ( is_directory(current,token) )
                        versionID = path_extend( versionID, token, 1 );
                    type = MIXED;
                    break;
                case 8: // MVD corcode
                    docid = path_extend( docid, token, 1 );
                    type = MVD_CORCODE;
                    break;
                case 9: // TEXT corcode
                    docid = path_extend( docid, token, 1 );
                    type = TEXT_CORCODE;
                    state = 12;
                    break;
                case 10: // HTML raw
                    old = token;
                    if ( is_directory(current,token) )
                        versionID = path_extend( versionID, token, 1 );
                    type = HTML;
                    break;
                case 11: case 12: // TEXT cortex,corcode
                    if ( versionID == NULL )
                    {
                        char *first=calloc(strlen(token)+2,1);
                        if ( first != NULL )
                        {
                            strcpy( first,"/");
                            strcat( first, token );
                            versionID = path_extend( versionID, first, 1 );
                            free( first );
                        }
                        else
                        {
                            fprintf(stderr,"path: failed to allocate versionID\n");
                            state = -1;
                        }
                    }
                    else
                        versionID = path_extend( versionID, token, 1 );
                    break;
            }
            current = path_extend( current, token, 1 );
            token = strtok( NULL, "/" );
        }
        if ( type != NO_TYPE && !path_ends(current,".conf") )
        {
            it = item_create( docid, type, db );
            key = item_key( it );
            if ( key != NULL )
            {
                if ( hashmap_contains(hm,key) )
                {
                    item_dispose( it );
                    it = hashmap_get(hm,key);
                }
                else
                    hashmap_put( hm, key, it );
                free( key );
            }
            item_add_path( it, current );
            if ( versionID != NULL )
                item_set_versionID( it, versionID );
        }
        if ( db != NULL )
            free( db );
        if ( docid != NULL )
            free( docid );
        if ( current != NULL )
            free( current );
        if ( versionID != NULL )
            free( versionID );
        free( path_copy );
    }
    else
        fprintf(stderr,"path:failed to duplicate path\n");
}
/**
 * Print out the item map for debugging purposes
 * @param hm the hashmap to print
 */
static void path_print_map( hashmap *hm )
{
    char **keys = calloc( hashmap_size(hm), sizeof(char*) );
    if ( keys != NULL )
    {
        int i;
        hashmap_to_array( hm, keys );
        for ( i=0;i<hashmap_size(hm);i++ )
        {
            item *it = hashmap_get(hm,keys[i]);
            item_print( it );
        }
        free( keys );
    }
}
/**
 * Process the paths by grouping them into items
 * @param head the first path in the list
 * @return a map of docids to items
 */
hashmap *path_process( path *head )
{
    path *p = head;
    hashmap *hm = hashmap_create();
    while ( p != NULL )
    {
        path_parse( p, hm );
        p = p->next;
    }
    return hm;
}
/**
 * Apply a config file to all subordinate items
 * @param hm the map of docid-based keys to items
 * @param p the path-prefix to look for
 * @param fname the file path of the config file
 */
static void path_apply_conf( hashmap *hm, char *p, char *fname )
{
    int i,size = hashmap_size( hm );
    char **keys = calloc( size, sizeof(char*) );
    if ( keys != NULL )
    {
        hashmap_to_array( hm, keys );
        for ( i=0;i<size;i++ )
        {
            item *it = hashmap_get(hm,keys[i]);
            if ( item_path_unique(it,p,fname) )
            {
                config *cf = item_config( it );
                cf = config_update( fname, cf );
                item_set_config( it, cf );
                break;
            }
            else if ( item_path_starts(it,p,fname) )
            {
                config *cf = item_config( it );
                cf = config_update( fname, cf );
                item_set_config( it, cf );
            }
        }
        free( keys );
    }
}
/**
 * Find files ending in ".conf"
 * @param head the head of the list
 * @param hm map of docid-based keys to items (sets of paths of the same type)
 */
void path_find_config( path *head, hashmap *hm )
{
    path *p = head;
    while ( p != NULL )
    {
        char *token = strtok( p->path, "/" );
        char *current = NULL;
        
        while ( token != NULL )
        {
            if ( path_ends(token,".conf") )
            {
                int clen = strlen(current);
                char *fname = calloc(clen+strlen(token)+2,1);
                if ( fname != NULL )
                {
                    strcpy( fname, current );
                    strcat( fname, "/" );
                    strcat( fname, token );
                    path_apply_conf( hm, current, fname );
                    free( fname );
                }
            }
            current = path_extend( current, token, 1 );
            token = strtok( NULL, "/" );
        }
        if ( current != NULL )
            free( current );
        p = path_next( p );
    }
}