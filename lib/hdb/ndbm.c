/*
 * Copyright (c) 1997 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. All advertising materials mentioning features or use of this software 
 *    must display the following acknowledgement: 
 *      This product includes software developed by Kungliga Tekniska 
 *      H�gskolan and its contributors. 
 *
 * 4. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

#include "hdb_locl.h"

RCSID("$Id$");

#ifdef HAVE_NDBM_H

static krb5_error_code
NDBM_close(krb5_context context, HDB *db)
{
    DBM *d = (DBM*)db->db;
    dbm_close(d);
    free(db->name);
    free(db);
    return 0;
}

static krb5_error_code
NDBM_lock(krb5_context context, HDB *db, int operation)
{
#if 0
    int fd = fd;
    return hdb_lock(fd, operation);
#endif
    return 0;
}

static krb5_error_code
NDBM_unlock(krb5_context context, HDB *db)
{
#if 0
    int fd = fd;
    return hdb_unlock(fd);
#endif
    return 0;
}

static krb5_error_code
NDBM_fetch(krb5_context context, HDB *db, hdb_entry *entry)
{
    DBM *d = (DBM*)db->db;
    datum key, value;
    krb5_data data;
    krb5_error_code ret;

    hdb_principal2key(context, entry->principal, &data);
#if 0
    krb5_free_principal (context, entry->principal);
#endif

    key.dptr = data.data;
    key.dsize = data.length;
    ret = db->lock(context, db, HDB_RLOCK);
    if(ret) return ret;
    value = dbm_fetch(d, key);
    db->unlock(context, db);
    krb5_data_free(&data);
    if(value.dptr == NULL)
	return HDB_ERR_NOENTRY;
    
    data.data = value.dptr;
    data.length = value.dsize;
    
    hdb_value2entry(context, &data, entry);
    /* krb5_data_free(&data); */
    return 0;
}

static krb5_error_code
NDBM_store(krb5_context context, HDB *db, hdb_entry *entry)
{
    DBM *d = (DBM*)db->db;
    krb5_data data;
    krb5_error_code ret;
    int code;
    datum key, value;
    hdb_principal2key(context, entry->principal, &data);
    key.dptr = data.data;
    key.dsize = data.length;
    hdb_entry2value(context, entry, &data);
    value.dptr = data.data;
    value.dsize = data.length;
    ret = db->lock(context, db, HDB_WLOCK);
    if(ret) return ret;
    code = dbm_store(d, key, value, DBM_REPLACE);
    db->unlock(context, db);
    free(key.dptr);
    free(value.dptr);
    if(code < 0)
	return errno;
    return 0;
}

static krb5_error_code
NDBM_delete(krb5_context context, HDB *db, hdb_entry *entry)
{
    DBM *d = (DBM*)db->db;
    datum key;
    krb5_data data;
    int code;
    krb5_error_code ret;

    hdb_principal2key(context, entry->principal, &data);

    key.dptr = data.data;
    key.dsize = data.length;
    ret = db->lock(context, db, HDB_WLOCK);
    if(ret) return ret;
    code = dbm_delete(d, key);
    db->unlock(context, db);
    krb5_data_free(&data);
    if(code < 0)
	return errno;
    return 0;
}

static krb5_error_code
NDBM_seq(krb5_context context, HDB *db, hdb_entry *entry, int first)

{
    DBM *d = (DBM*)db->db;
    datum key, value;
    krb5_data key_data, data;
    krb5_principal principal;
    krb5_error_code ret;

    if(first)
	key = dbm_firstkey(d);
    else
	key = dbm_nextkey(d);
    if(key.dptr == NULL)
	return HDB_ERR_NOENTRY;
    key_data.data = key.dptr;
    key_data.length = key.dsize;
    ret = db->lock(context, db, HDB_RLOCK);
    if(ret) return ret;
    value = dbm_fetch(d, key);
    db->unlock(context, db);
    data.data = value.dptr;
    data.length = value.dsize;
    if(hdb_value2entry(context, &data, entry))
	return NDBM_seq(context, db, entry, 0);
    if (entry->principal == NULL) {
	entry->principal = malloc (sizeof(*entry->principal));
	hdb_key2principal (context, &key_data, entry->principal);
    }
    return 0;
}


static krb5_error_code
NDBM_firstkey(krb5_context context, HDB *db, hdb_entry *entry)
{
    return NDBM_seq(context, db, entry, 1);
}


static krb5_error_code
NDBM_nextkey(krb5_context context, HDB *db, hdb_entry *entry)
{
    return NDBM_seq(context, db, entry, 0);
}

static krb5_error_code
NDBM_rename(krb5_context context, HDB *db, const char *new_name)
{
    /* XXX this function will break */

    int ret;
    char *old_dir, *old_pag, *new_dir, *new_pag;

    asprintf(&old_dir, "%s.dir", db->name);
    asprintf(&old_pag, "%s.pag", db->name);
    asprintf(&new_dir, "%s.dir", new_name);
    asprintf(&new_pag, "%s.pag", new_name);
    ret = rename(old_dir, new_dir) || rename(old_pag, new_pag);
    free(old_dir);
    free(old_pag);
    free(new_dir);
    free(new_pag);
    if(ret)
	return errno;
    
    free(db->name);
    db->name = strdup(new_name);
    return 0;
}

static krb5_error_code
NDBM__get(krb5_context context, HDB *db, krb5_data key, krb5_data *reply)
{
    DBM *d = (DBM*)db->db;
    datum k, v;
    int code;

    k.dptr  = key.data;
    k.dsize = key.length;
    code = db->lock(context, db, HDB_RLOCK);
    if(code)
	return code;
    v = dbm_fetch(d, k);
    db->unlock(context, db);
    if(v.dptr == NULL)
	return HDB_ERR_NOENTRY;

    krb5_data_copy(reply, v.dptr, v.dsize);
    return 0;
}

static krb5_error_code
NDBM__put(krb5_context context, HDB *db, int replace, 
	krb5_data key, krb5_data value)
{
    DBM *d = (DBM*)db->db;
    datum k, v;
    int code;

    k.dptr  = key.data;
    k.dsize = key.length;
    v.dptr  = value.data;
    v.dsize = value.length;

    code = db->lock(context, db, HDB_WLOCK);
    if(code)
	return code;
    code = dbm_store(d, k, v, replace ? DBM_REPLACE : DBM_INSERT);
    db->unlock(context, db);
    if(code == 1)
	return HDB_ERR_EXISTS;
    if (code < 0)
	return code;
    return 0;
}

krb5_error_code
hdb_ndbm_open(krb5_context context, HDB **db, 
	      const char *filename, int flags, mode_t mode)
{
    DBM *d;
    d = dbm_open((char*)filename, flags, mode);
    if(d == NULL)
	return errno;
    *db = malloc(sizeof(**db));
    (*db)->db = d;
    (*db)->name = strdup(filename);
    (*db)->close = NDBM_close;
    (*db)->fetch = NDBM_fetch;
    (*db)->store = NDBM_store;
    (*db)->delete = NDBM_delete;
    (*db)->firstkey = NDBM_firstkey;
    (*db)->nextkey= NDBM_nextkey;
    (*db)->lock = NDBM_lock;
    (*db)->unlock = NDBM_unlock;
    (*db)->rename = NDBM_rename;
    (*db)->_get = NDBM__get;
    (*db)->_put = NDBM__put;
    return 0;
}


#endif
