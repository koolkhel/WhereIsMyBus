/*    Copyright 2009, 2010 10gen Inc.
 *    Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "bson_p.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

const int initialBufferSize = 128;

/* only need one of these */
static const int zero = 0;

/* ----------------------------
   READING
   ------------------------------ */

bson * bson_empty(bson * obj){
    static const char * data = "\005\0\0\0\0";
    return bson_init(obj, (char *)data, 0);
}

void bson_copy(bson* out, const bson* in){
    if (!out) return;
    out->data = bson_malloc(bson_size(in));
    out->owned = 1;
    memcpy(out->data, in->data, bson_size(in));
}

bson * bson_from_buffer(bson * b, bson_buffer * buf){
    return bson_init(b, bson_buffer_finish(buf), 1);
}

bson * bson_init( bson * b , char * data , bool mine ){
    b->data = data;
    b->owned = mine;
    return b;
}
int bson_size(const bson * b ){
    int i;
    if ( ! b || ! b->data )
        return 0;
    bson_little_endian32(&i, b->data);
    return i;
}
void bson_destroy( bson * b ){
    if ( b->owned && b->data )
        free( b->data );
    b->data = 0;
    b->owned = 0;
}

void bson_print( bson * b ){
    bson_print_raw( b->data , 0 );
}

void bson_print_raw( const char * data , int depth ){
    bson_iterator i;
    const char * key;
    int temp;
    bson_timestamp_t ts;
    bson_iterator_init( &i , data );

    while ( bson_iterator_next( &i ) ){
        bson_type t = bson_iterator_type( &i );
        if ( t == 0 )
            break;
        key = bson_iterator_key( &i );

        for ( temp=0; temp<=depth; temp++ )
            printf( "\t" );
        printf( "%s : %d \t " , key , t );
        switch ( t ){
        case bson_int: printf( "%d" , bson_iterator_int( &i ) ); break;
        case bson_double: printf( "%f" , bson_iterator_double( &i ) ); break;
        case bson_bool: printf( "%s" , bson_iterator_bool( &i ) ? "true" : "false" ); break;
        case bson_string: printf( "%s" , bson_iterator_string( &i ) ); break;
        case bson_utf16: printf( "%ls" , (wchar_t *) bson_iterator_utf16( &i ) ); break;
        case bson_null: printf( "null" ); break;
        case bson_timestamp:
            ts = bson_iterator_timestamp( &i );
            printf("i: %d, t: %d", ts.i, ts.t);
            break;
        case bson_object:
        case bson_array:
            printf( "\n" );
            bson_print_raw( bson_iterator_value( &i ) , depth + 1 );
            break;
        default:
            fprintf( stderr , "can't print type : %d\n" , t );
        }
        printf( "\n" );
    }
}

/* ----------------------------
   ITERATOR
   ------------------------------ */

void bson_iterator_init( bson_iterator * i , const char * bson ){
    i->cur = bson + 4;
    i->first = 1;
}

bson_type bson_find(bson_iterator* it, const bson* obj, const char* name){
    bson_iterator_init(it, obj->data);
    while(bson_iterator_next(it)){
        if (strcmp(name, bson_iterator_key(it)) == 0)
            break;
    }
    return bson_iterator_type(it);
}

bool bson_iterator_more( const bson_iterator * i ){
    return *(i->cur);
}

bson_type bson_iterator_next( bson_iterator * i ){
    int ds;

    if ( i->first ){
        i->first = 0;
        return (bson_type)(*i->cur);
    }

    switch ( bson_iterator_type(i) ){
    case bson_eoo: return bson_eoo; /* don't advance */
    case bson_undefined:
    case bson_null: ds = 0; break;
    case bson_bool: ds = 1; break;
    case bson_int: ds = 4; break;
    case bson_long:
    case bson_double:
    case bson_timestamp:
    case bson_date: ds = 8; break;
    case bson_string:
    case bson_utf16:
    case bson_symbol:
    case bson_code: ds = 4 + bson_iterator_int_raw(i); break;
    case bson_bindata: ds = 5 + bson_iterator_int_raw(i); break;
    case bson_object:
    case bson_array:
    case bson_codewscope: ds = bson_iterator_int_raw(i); break;
    case bson_dbref: ds = 4+12 + bson_iterator_int_raw(i); break;
    case bson_regex:
        {
            const char * s = bson_iterator_value(i);
            const char * p = s;
            p += strlen(p)+1;
            p += strlen(p)+1;
            ds = p-s;
            break;
        }

    default:
        {
            char msg[] = "unknown type: 000000000000";
            bson_numstr(msg+14, (unsigned)(i->cur[0]));
            fprintf(stderr, "unknown type: %d\n", i->cur[0]);
            bson_fatal_msg(0, msg);
            return bson_eoo;
        }
    }

    i->cur += 1 + strlen( i->cur + 1 ) + 1 + ds;

    return (bson_type)(*i->cur);
}

bson_type bson_iterator_type( const bson_iterator * i ){
    return (bson_type)i->cur[0];
}
const char * bson_iterator_key( const bson_iterator * i ){
    return i->cur + 1;
}
const char * bson_iterator_value( const bson_iterator * i ){
    const char * t = i->cur + 1;
    t += strlen( t ) + 1;
    return t;
}

/* types */

int bson_iterator_int_raw( const bson_iterator * i ){
    int out;
    bson_little_endian32(&out, bson_iterator_value( i ));
    return out;
}
double bson_iterator_double_raw( const bson_iterator * i ){
    double out;
    bson_little_endian64(&out, bson_iterator_value( i ));
    return out;
}
int64_t bson_iterator_long_raw( const bson_iterator * i ){
    int64_t out;
    bson_little_endian64(&out, bson_iterator_value( i ));
    return out;
}

bson_bool_t bson_iterator_bool_raw( const bson_iterator * i ){
    return (bson_bool_t)bson_iterator_value( i )[0];
}

int bson_iterator_int( const bson_iterator * i ){
    switch (bson_iterator_type(i)){
        case bson_int: return bson_iterator_int_raw(i);
        case bson_long: return bson_iterator_long_raw(i);
        case bson_double: return bson_iterator_double_raw(i);
        default: return 0;
    }
}
double bson_iterator_double( const bson_iterator * i ){
    switch (bson_iterator_type(i)){
        case bson_int: return bson_iterator_int_raw(i);
        case bson_long: return bson_iterator_long_raw(i);
        case bson_double: return bson_iterator_double_raw(i);
        default: return 0;
    }
}
int64_t bson_iterator_long( const bson_iterator * i ){
    switch (bson_iterator_type(i)){
        case bson_int: return bson_iterator_int_raw(i);
        case bson_long: return bson_iterator_long_raw(i);
        case bson_double: return bson_iterator_double_raw(i);
        default: return 0;
    }
}

bson_timestamp_t bson_iterator_timestamp( const bson_iterator * i){
    bson_timestamp_t ts;
    bson_little_endian32(&(ts.i), bson_iterator_value(i));
    bson_little_endian32(&(ts.t), bson_iterator_value(i) + 4);
    return ts;
}

bool bson_iterator_bool( const bson_iterator * i ){
    switch (bson_iterator_type(i)){
    case bson_bool: return (bson_iterator_bool_raw(i) == bson_true) ? true : false;
    case bson_int: return bson_iterator_int_raw(i) != 0;
    case bson_long: return bson_iterator_long_raw(i) != 0;
    case bson_double: return bson_iterator_double_raw(i) != 0;
    case bson_eoo:
    case bson_null: return 0;
    default: return 1;
    }
}

const char * bson_iterator_string( const bson_iterator * i ){
    return bson_iterator_value( i ) + 4;
}
int bson_iterator_string_len( const bson_iterator * i ){
    return bson_iterator_int_raw( i );
}
const QChar * bson_iterator_utf16( const bson_iterator * i ){
    return (const QChar *)(bson_iterator_value( i ) + 4);
}
int bson_iterator_utf16_numchars( const bson_iterator * i ){
    return bson_iterator_int_raw( i ) / 2 - 1;
}

const char * bson_iterator_code( const bson_iterator * i ){
    switch (bson_iterator_type(i)){
        case bson_string:
        case bson_code: return bson_iterator_value(i) + 4;
        case bson_codewscope: return bson_iterator_value(i) + 8;
        default: return NULL;
    }
}

void bson_iterator_code_scope(const bson_iterator * i, bson * scope){
    if (bson_iterator_type(i) == bson_codewscope){
        int code_len;
        bson_little_endian32(&code_len, bson_iterator_value(i)+4);
        bson_init(scope, (char*)(bson_iterator_value(i)+8+code_len), 0);
    }else{
        bson_empty(scope);
    }
}

bson_date_t bson_iterator_date(const bson_iterator * i){
    return bson_iterator_long_raw(i);
}

time_t bson_iterator_time_t(const bson_iterator * i){
    return bson_iterator_date(i) / 1000;
}

int bson_iterator_bin_len( const bson_iterator * i ){
    return bson_iterator_int_raw( i );
}

char bson_iterator_bin_type( const bson_iterator * i ){
    return bson_iterator_value(i)[4];
}
const char * bson_iterator_bin_data( const bson_iterator * i ){
    return bson_iterator_value( i ) + 5;
}

const char * bson_iterator_regex( const bson_iterator * i ){
    return bson_iterator_value( i );
}
const char * bson_iterator_regex_opts( const bson_iterator * i ){
    const char* p = bson_iterator_value( i );
    return p + strlen(p) + 1;

}

void bson_iterator_subobject(const bson_iterator * i, bson * sub){
    bson_init(sub, (char*)bson_iterator_value(i), 0);
}
void bson_iterator_subiterator(const bson_iterator * i, bson_iterator * sub){
    bson_iterator_init(sub, bson_iterator_value(i));
}

/* ----------------------------
   BUILDING
   ------------------------------ */

bson_buffer * bson_buffer_init( bson_buffer * b ){
    b->buf = (char*)bson_malloc( initialBufferSize );
    b->bufSize = initialBufferSize;
    b->cur = b->buf + 4;
    b->finished = 0;
    b->stackPos = 0;
    return b;
}

void bson_append_byte( bson_buffer * b , char c ){
    b->cur[0] = c;
    b->cur++;
}
void bson_append( bson_buffer * b , const void * data , int len ){
    memcpy( b->cur , data , len );
    b->cur += len;
}
void bson_append32(bson_buffer * b, const void * data){
    bson_little_endian32(b->cur, data);
    b->cur += 4;
}
void bson_append64(bson_buffer * b, const void * data){
    bson_little_endian64(b->cur, data);
    b->cur += 8;
}

bson_buffer * bson_ensure_space( bson_buffer * b , const int bytesNeeded ){
    int pos = b->cur - b->buf;
    int new_size;

    if (b->finished) {
        bson_fatal_msg(!!b->buf, "trying to append to finished buffer");
    }

    if (pos + bytesNeeded <= b->bufSize)
        return b;

    new_size = 1.5 * (b->bufSize + bytesNeeded);
    b->buf = (char *)realloc(b->buf, new_size);
    if (!b->buf)
        bson_fatal_msg(!!b->buf, "realloc() failed");

    b->bufSize = new_size;
    b->cur = b->buf + pos;

    return b;
}

char * bson_buffer_finish( bson_buffer * b ){
    int i;
    if ( ! b->finished ){
        if ( ! bson_ensure_space( b , 1 ) ) return 0;
        bson_append_byte( b , 0 );
        i = b->cur - b->buf;
        bson_little_endian32(b->buf, &i);
        b->finished = 1;
    }
    return b->buf;
}

void bson_buffer_destroy( bson_buffer * b ){
    free( b->buf );
    b->buf = 0;
    b->cur = 0;
    b->finished = 1;
}

static bson_buffer * bson_append_estart( bson_buffer * b , int type , const char * name , const int dataSize ){
    const int sl = strlen(name) + 1;
    if ( ! bson_ensure_space( b , 1 + sl + dataSize ) )
        return 0;
    bson_append_byte( b , (char)type );
    bson_append( b , name , sl );
    return b;
}

/* ----------------------------
   BUILDING TYPES
   ------------------------------ */

bson_buffer * bson_append_int( bson_buffer * b , const char * name , const int i ){
    if ( ! bson_append_estart( b , bson_int , name , 4 ) ) return 0;
    bson_append32( b , &i );
    return b;
}
bson_buffer * bson_append_long( bson_buffer * b , const char * name , const int64_t i ){
    if ( ! bson_append_estart( b , bson_long , name , 8 ) ) return 0;
    bson_append64( b , &i );
    return b;
}
bson_buffer * bson_append_double( bson_buffer * b , const char * name , const double d ){
    if ( ! bson_append_estart( b , bson_double , name , 8 ) ) return 0;
    bson_append64( b , &d );
    return b;
}
bson_buffer * bson_append_bool( bson_buffer * b , const char * name , const bool bv ){
    if ( ! bson_append_estart( b , bson_bool , name , 1 ) ) return 0;
    bson_append_byte( b , bv ? bson_true : bson_false );
    return b;
}
bson_buffer * bson_append_null( bson_buffer * b , const char * name ){
    if ( ! bson_append_estart( b , bson_null , name , 0 ) ) return 0;
    return b;
}
bson_buffer * bson_append_undefined( bson_buffer * b , const char * name ){
    if ( ! bson_append_estart( b , bson_undefined , name , 0 ) ) return 0;
    return b;
}
bson_buffer * bson_append_string_base( bson_buffer * b , const char * name , const char * value , int sl, bson_type type){
    if ( ! bson_append_estart( b , type , name , 4 + sl ) ) return 0;
    bson_append32( b , &sl);
    bson_append( b , value , sl );
    return b;
}
bson_buffer * bson_append_string( bson_buffer * b , const char * name , const char * value ){
    int sl = strlen( value ) + 1;
    return bson_append_string_base(b, name, value, sl, bson_string);
}
bson_buffer * bson_append_utf16( bson_buffer * b , const char * name , const QChar * value, int numchars ){
    return bson_append_string_base(b, name, (const char *)value, (numchars+1)*2, bson_utf16);
}
bson_buffer * bson_append_symbol( bson_buffer * b , const char * name , const char * value ){
    int sl = strlen( value ) + 1;
    return bson_append_string_base(b, name, value, sl, bson_symbol);
}
bson_buffer * bson_append_code( bson_buffer * b , const char * name , const char * value ){
    int sl = strlen( value ) + 1;
    return bson_append_string_base(b, name, value, sl, bson_code);
}

bson_buffer * bson_append_code_w_scope( bson_buffer * b , const char * name , const char * code , const bson * scope){
    int sl = strlen(code) + 1;
    int size = 4 + 4 + sl + bson_size(scope);
    if (!bson_append_estart(b, bson_codewscope, name, size)) return 0;
    bson_append32(b, &size);
    bson_append32(b, &sl);
    bson_append(b, code, sl);
    bson_append(b, scope->data, bson_size(scope));
    return b;
}

bson_buffer * bson_append_binary( bson_buffer * b, const char * name, char type, const char * str, int len ){
    if ( ! bson_append_estart( b , bson_bindata , name , 4+1+len ) ) return 0;
    bson_append32(b, &len);
    bson_append_byte(b, type);
    bson_append(b, str, len);
    return b;
}

bson_buffer * bson_append_regex( bson_buffer * b , const char * name , const char * pattern, const char * opts ){
    const int plen = strlen(pattern)+1;
    const int olen = strlen(opts)+1;
    if ( ! bson_append_estart( b , bson_regex , name , plen + olen ) ) return 0;
    bson_append( b , pattern , plen );
    bson_append( b , opts , olen );
    return b;
}

bson_buffer * bson_append_bson( bson_buffer * b , const char * name , bson_type bt, const bson* bson){
    if ( ! bson_append_estart( b , bt , name , bson_size(bson) ) ) return 0;
    bson_append( b , bson->data , bson_size(bson) );
    return b;
}

bson_buffer * bson_append_element( bson_buffer * b, const char * name_or_null, const bson_iterator* elem){
    bson_iterator next = *elem;
    int size;

    bson_iterator_next(&next);
    size = next.cur - elem->cur;

    if (name_or_null == NULL){
        bson_ensure_space(b, size);
        bson_append(b, elem->cur, size);
    }else{
        int data_size = size - 1 - strlen(bson_iterator_key(elem));
        bson_append_estart(b, elem->cur[0], name_or_null, data_size);
        bson_append(b, name_or_null, strlen(name_or_null));
        bson_append(b, bson_iterator_value(elem), data_size);
    }

    return b;
}

bson_buffer * bson_append_timestamp( bson_buffer * b, const char * name, bson_timestamp_t * ts ){
    if ( ! bson_append_estart( b , bson_timestamp , name , 8 ) ) return 0;

    bson_append32( b , &(ts->i) );
    bson_append32( b , &(ts->t) );

    return b;
}

bson_buffer * bson_append_date( bson_buffer * b , const char * name , bson_date_t millis ){
    if ( ! bson_append_estart( b , bson_date , name , 8 ) ) return 0;
    bson_append64( b , &millis );
    return b;
}

bson_buffer * bson_append_time_t( bson_buffer * b , const char * name , time_t secs){
    return bson_append_date(b, name, (bson_date_t)secs * 1000);
}

bson_buffer * bson_append_start_object( bson_buffer * b , const char * name ){
    if ( ! bson_append_estart( b , bson_object , name , 5 ) ) return 0;
    b->stack[ b->stackPos++ ] = b->cur - b->buf;
    bson_append32( b , &zero );
    return b;
}

bson_buffer * bson_append_start_array( bson_buffer * b , const char * name ){
    if ( ! bson_append_estart( b , bson_array , name , 5 ) ) return 0;
    b->stack[ b->stackPos++ ] = b->cur - b->buf;
    bson_append32( b , &zero );
    return b;
}

bson_buffer * bson_append_finish_object( bson_buffer * b ){
    char * start;
    int i;
    if ( ! bson_ensure_space( b , 1 ) ) return 0;
    bson_append_byte( b , 0 );

    start = b->buf + b->stack[ --b->stackPos ];
    i = b->cur - start;
    bson_little_endian32(start, &i);

    return b;
}

char* bson_malloc(int size){
    char* p = (char *)malloc(size);
    if (!p)
        bson_fatal_msg(!!p, "malloc() failed");
    return p;
}

static bson_err_handler err_handler = NULL;

bson_err_handler set_bson_err_handler(bson_err_handler func){
    bson_err_handler old = err_handler;
    err_handler = func;
    return old;
}

void bson_fatal( int ok ){
    bson_fatal_msg(ok, "");
}

void bson_fatal_msg( int ok , const char* msg){
    if (ok)
        return;

    if (err_handler){
        err_handler(msg);
    }

    fprintf( stderr , "error: %s\n" , msg );
    exit(-5);
}

void bson_numstr(char* str, int i){
    sprintf(str,"%d", i);
}
