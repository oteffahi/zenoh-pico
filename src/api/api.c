//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/config.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/transport/unicast.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"
#include "zenoh-pico/utils/uuid.h"

/********* Data Types Handlers *********/
_Bool z_bytes_check(const z_bytes_t *v) { return v->start != NULL; }

z_string_t z_string_make(const char *value) { return _z_string_make(value); }

z_str_t *z_str_array_get(const z_str_array_t *a, size_t k) { return _z_str_array_get(a, k); }
size_t z_str_array_len(const z_str_array_t *a) { return _z_str_array_len(a); }
_Bool z_str_array_is_empty(const z_str_array_t *a) { return _z_str_array_is_empty(a); }

z_keyexpr_t z_keyexpr(const char *name) { return _z_rname(name); }

z_keyexpr_t z_keyexpr_unchecked(const char *name) { return _z_rname(name); }

z_owned_str_t z_keyexpr_to_string(z_keyexpr_t keyexpr) {
    z_owned_str_t ret = {._value = NULL};

    if (keyexpr._id == Z_RESOURCE_ID_NONE) {
        size_t ke_len = _z_str_size(keyexpr._suffix);

        ret._value = (char *)z_malloc(ke_len);
        if (ret._value != NULL) {
            _z_str_n_copy(ret._value, keyexpr._suffix, ke_len);
        }
    }

    return ret;
}

z_bytes_t z_keyexpr_as_bytes(z_keyexpr_t keyexpr) {
    if (keyexpr._id == Z_RESOURCE_ID_NONE) {
        z_bytes_t ret = {.start = (const uint8_t *)keyexpr._suffix, .len = strlen(keyexpr._suffix), ._is_alloc = false};
        return ret;
    } else {
        z_bytes_t ret = {.start = NULL, .len = 0, ._is_alloc = false};
        return ret;
    }
}

_Bool zp_keyexpr_was_declared(const z_keyexpr_t *keyexpr) {
    _Bool ret = false;
    if (keyexpr->_id != Z_RESOURCE_ID_NONE) {
        ret = true;
    }
    return ret;
}

z_owned_str_t zp_keyexpr_resolve(z_session_t zs, z_keyexpr_t keyexpr) {
    z_owned_str_t ret = {._value = NULL};

    _z_keyexpr_t ekey = _z_get_expanded_key_from_key(&zs._val.in->val, &keyexpr);
    ret._value = (char *)ekey._suffix;  // ekey will be out of scope so
                                        //  - suffix can be safely casted as non-const
                                        //  - suffix does not need to be copied
    return ret;
}

_Bool z_keyexpr_is_initialized(const z_keyexpr_t *keyexpr) {
    _Bool ret = false;

    if ((keyexpr->_id != Z_RESOURCE_ID_NONE) || (keyexpr->_suffix != NULL)) {
        ret = true;
    }

    return ret;
}

int8_t z_keyexpr_is_canon(const char *start, size_t len) { return _z_keyexpr_is_canon(start, len); }

int8_t zp_keyexpr_is_canon_null_terminated(const char *start) { return _z_keyexpr_is_canon(start, strlen(start)); }

int8_t z_keyexpr_canonize(char *start, size_t *len) { return _z_keyexpr_canonize(start, len); }

int8_t zp_keyexpr_canonize_null_terminated(char *start) {
    zp_keyexpr_canon_status_t ret = Z_KEYEXPR_CANON_SUCCESS;

    size_t len = strlen(start);
    size_t newlen = len;
    ret = _z_keyexpr_canonize(start, &newlen);
    if (newlen < len) {
        start[newlen] = '\0';
    }

    return ret;
}

int8_t z_keyexpr_includes(z_keyexpr_t l, z_keyexpr_t r) {
    int8_t ret = 0;

    if ((l._id == Z_RESOURCE_ID_NONE) && (r._id == Z_RESOURCE_ID_NONE)) {
        ret = zp_keyexpr_includes_null_terminated(l._suffix, r._suffix);
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t zp_keyexpr_includes_null_terminated(const char *l, const char *r) {
    int8_t ret = 0;

    if (l != NULL && r != NULL) {
        ret = _z_keyexpr_includes(l, strlen(l), r, strlen(r)) == true ? 0 : -1;
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t z_keyexpr_intersects(z_keyexpr_t l, z_keyexpr_t r) {
    int8_t ret = 0;

    if ((l._id == Z_RESOURCE_ID_NONE) && (r._id == Z_RESOURCE_ID_NONE)) {
        ret = zp_keyexpr_intersect_null_terminated(l._suffix, r._suffix);
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t zp_keyexpr_intersect_null_terminated(const char *l, const char *r) {
    int8_t ret = -1;

    if (l != NULL && r != NULL) {
        ret = (_z_keyexpr_intersects(l, strlen(l), r, strlen(r)) == true) ? 0 : -1;
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t z_keyexpr_equals(z_keyexpr_t l, z_keyexpr_t r) {
    int8_t ret = 0;

    if ((l._id == Z_RESOURCE_ID_NONE) && (r._id == Z_RESOURCE_ID_NONE)) {
        ret = zp_keyexpr_equals_null_terminated(l._suffix, r._suffix);
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t zp_keyexpr_equals_null_terminated(const char *l, const char *r) {
    int8_t ret = -1;

    if (l != NULL && r != NULL) {
        size_t llen = strlen(l);
        if (llen == strlen(r)) {
            if (strncmp(l, r, llen) == 0) {
                ret = 0;
            }
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_owned_config_t z_config_new(void) { return (z_owned_config_t){._value = _z_config_empty()}; }

z_owned_config_t z_config_default(void) { return (z_owned_config_t){._value = _z_config_default()}; }

const char *zp_config_get(z_config_t config, uint8_t key) { return _z_config_get(config._val, key); }

int8_t zp_config_insert(z_config_t config, uint8_t key, z_string_t value) {
    return _zp_config_insert(config._val, key, value);
}

z_owned_scouting_config_t z_scouting_config_default(void) {
    _z_scouting_config_t *sc = _z_config_empty();

    _zp_config_insert(sc, Z_CONFIG_MULTICAST_LOCATOR_KEY, _z_string_make(Z_CONFIG_MULTICAST_LOCATOR_DEFAULT));
    _zp_config_insert(sc, Z_CONFIG_SCOUTING_TIMEOUT_KEY, _z_string_make(Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT));
    _zp_config_insert(sc, Z_CONFIG_SCOUTING_WHAT_KEY, _z_string_make(Z_CONFIG_SCOUTING_WHAT_DEFAULT));

    return (z_owned_scouting_config_t){._value = sc};
}

z_owned_scouting_config_t z_scouting_config_from(z_config_t c) {
    _z_scouting_config_t *sc = _z_config_empty();

    char *opt;
    opt = _z_config_get(c._val, Z_CONFIG_MULTICAST_LOCATOR_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc, Z_CONFIG_MULTICAST_LOCATOR_KEY, _z_string_make(Z_CONFIG_MULTICAST_LOCATOR_DEFAULT));
    } else {
        _zp_config_insert(sc, Z_CONFIG_MULTICAST_LOCATOR_KEY, _z_string_make(opt));
    }

    opt = _z_config_get(c._val, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc, Z_CONFIG_SCOUTING_TIMEOUT_KEY, _z_string_make(Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT));
    } else {
        _zp_config_insert(sc, Z_CONFIG_SCOUTING_TIMEOUT_KEY, _z_string_make(opt));
    }

    opt = _z_config_get(c._val, Z_CONFIG_SCOUTING_WHAT_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc, Z_CONFIG_SCOUTING_WHAT_KEY, _z_string_make(Z_CONFIG_SCOUTING_WHAT_DEFAULT));
    } else {
        _zp_config_insert(sc, Z_CONFIG_SCOUTING_WHAT_KEY, _z_string_make(opt));
    }

    return (z_owned_scouting_config_t){._value = sc};
}

const char *zp_scouting_config_get(z_scouting_config_t sc, uint8_t key) { return _z_config_get(sc._val, key); }

int8_t zp_scouting_config_insert(z_scouting_config_t sc, uint8_t key, z_string_t value) {
    return _zp_config_insert(sc._val, key, value);
}

z_encoding_t z_encoding(z_encoding_prefix_t prefix, const char *suffix) {
    return (_z_encoding_t){
        .prefix = prefix,
        .suffix = _z_bytes_wrap((const uint8_t *)suffix, (suffix == NULL) ? (size_t)0 : strlen(suffix))};
}

z_encoding_t z_encoding_default(void) { return z_encoding(Z_ENCODING_PREFIX_DEFAULT, NULL); }

_Bool z_timestamp_check(z_timestamp_t ts) { return _z_timestamp_check(&ts); }

z_value_t z_value(const char *payload, size_t payload_len, z_encoding_t encoding) {
    return (z_value_t){.payload = {.start = (const uint8_t *)payload, .len = payload_len}, .encoding = encoding};
}

z_query_target_t z_query_target_default(void) { return Z_QUERY_TARGET_DEFAULT; }

z_query_consolidation_t z_query_consolidation_auto(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_AUTO};
}

z_query_consolidation_t z_query_consolidation_latest(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_LATEST};
}

z_query_consolidation_t z_query_consolidation_monotonic(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_MONOTONIC};
}

z_query_consolidation_t z_query_consolidation_none(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_NONE};
}

z_query_consolidation_t z_query_consolidation_default(void) { return z_query_consolidation_auto(); }

z_bytes_t z_query_parameters(const z_query_t *query) {
    z_bytes_t parameters =
        _z_bytes_wrap((uint8_t *)query->_val._rc.in->val._parameters, strlen(query->_val._rc.in->val._parameters));
    return parameters;
}

z_value_t z_query_value(const z_query_t *query) { return query->_val._rc.in->val._value; }

z_keyexpr_t z_query_keyexpr(const z_query_t *query) { return query->_val._rc.in->val._key; }

_Bool z_value_is_initialized(z_value_t *value) {
    _Bool ret = false;

    if ((value->payload.start != NULL)) {
        ret = true;
    }

    return ret;
}

void z_closure_sample_call(const z_owned_closure_sample_t *closure, const z_sample_t *sample) {
    if (closure->call) {
        (closure->call)(sample, closure->context);
    }
}

void z_closure_query_call(const z_owned_closure_query_t *closure, const z_query_t *query) {
    if (closure->call) {
        (closure->call)(query, closure->context);
    }
}

void z_closure_reply_call(const z_owned_closure_reply_t *closure, z_owned_reply_t *reply) {
    if (closure->call) {
        (closure->call)(reply, closure->context);
    }
}

void z_closure_hello_call(const z_owned_closure_hello_t *closure, z_owned_hello_t *hello) {
    if (closure->call) {
        (closure->call)(hello, closure->context);
    }
}

void z_closure_zid_call(const z_owned_closure_zid_t *closure, const z_id_t *id) {
    if (closure->call) {
        (closure->call)(id, closure->context);
    }
}

/**************** Loans ****************/
#define OWNED_FUNCTIONS_PTR_INTERNAL(type, ownedtype, name, f_free, f_copy)      \
    _Bool z_##name##_check(const ownedtype *val) { return val->_value != NULL; } \
    type z_##name##_loan(const ownedtype *val) { return *val->_value; }          \
    ownedtype z_##name##_null(void) { return (ownedtype){._value = NULL}; }      \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }                   \
    ownedtype z_##name##_clone(ownedtype *val) {                                 \
        ownedtype ret;                                                           \
        ret._value = (type *)z_malloc(sizeof(type));                             \
        if (ret._value != NULL) {                                                \
            f_copy(ret._value, val->_value);                                     \
        }                                                                        \
        return ret;                                                              \
    }                                                                            \
    void z_##name##_drop(ownedtype *val) {                                       \
        if (val->_value != NULL) {                                               \
            f_free(&val->_value);                                                \
        }                                                                        \
    }

#define OWNED_FUNCTIONS_PTR_COMMON(type, ownedtype, name)                              \
    _Bool z_##name##_check(const ownedtype *val) { return val->_value != NULL; }       \
    type z_##name##_loan(const ownedtype *val) { return (type){._val = val->_value}; } \
    ownedtype z_##name##_null(void) { return (ownedtype){._value = NULL}; }            \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }

#define OWNED_FUNCTIONS_PTR_CLONE(type, ownedtype, name, f_copy) \
    ownedtype z_##name##_clone(ownedtype *val) {                 \
        ownedtype ret;                                           \
        ret._value = (_##type *)z_malloc(sizeof(_##type));       \
        if (ret._value != NULL) {                                \
            f_copy(ret._value, val->_value);                     \
        }                                                        \
        return ret;                                              \
    }

#define OWNED_FUNCTIONS_PTR_DROP(type, ownedtype, name, f_free) \
    void z_##name##_drop(ownedtype *val) {                      \
        if (val->_value != NULL) {                              \
            f_free(&val->_value);                               \
        }                                                       \
    }

#define OWNED_FUNCTIONS_STR(type, ownedtype, name, f_free, f_copy)               \
    _Bool z_##name##_check(const ownedtype *val) { return val->_value != NULL; } \
    type z_##name##_loan(const ownedtype *val) { return val->_value; }           \
    ownedtype z_##name##_null(void) { return (ownedtype){._value = NULL}; };     \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }                   \
    ownedtype z_##name##_clone(ownedtype *val) {                                 \
        ownedtype ret;                                                           \
        size_t size = _z_str_size(val->_value);                                  \
        ret._value = (_##type)z_malloc(size);                                    \
        if (ret._value != NULL) {                                                \
            f_copy(ret._value, val->_value, size);                               \
        }                                                                        \
        return ret;                                                              \
    }                                                                            \
    void z_##name##_drop(ownedtype *val) {                                       \
        if (val->_value != NULL) {                                               \
            f_free(&val->_value);                                                \
        }                                                                        \
    }

static inline void _z_owner_noop_copy(void *dst, const void *src) {
    (void)(dst);
    (void)(src);
}

OWNED_FUNCTIONS_STR(z_str_t, z_owned_str_t, str, _z_str_free, _z_str_n_copy)

OWNED_FUNCTIONS_PTR_COMMON(z_config_t, z_owned_config_t, config)
OWNED_FUNCTIONS_PTR_CLONE(z_config_t, z_owned_config_t, config, _z_owner_noop_copy)
OWNED_FUNCTIONS_PTR_DROP(z_config_t, z_owned_config_t, config, _z_config_free)

OWNED_FUNCTIONS_PTR_COMMON(z_scouting_config_t, z_owned_scouting_config_t, scouting_config)
OWNED_FUNCTIONS_PTR_CLONE(z_scouting_config_t, z_owned_scouting_config_t, scouting_config, _z_owner_noop_copy)
OWNED_FUNCTIONS_PTR_DROP(z_scouting_config_t, z_owned_scouting_config_t, scouting_config, _z_scouting_config_free)

OWNED_FUNCTIONS_PTR_INTERNAL(z_keyexpr_t, z_owned_keyexpr_t, keyexpr, _z_keyexpr_free, _z_keyexpr_copy)
OWNED_FUNCTIONS_PTR_INTERNAL(z_hello_t, z_owned_hello_t, hello, _z_hello_free, _z_owner_noop_copy)
OWNED_FUNCTIONS_PTR_INTERNAL(z_str_array_t, z_owned_str_array_t, str_array, _z_str_array_free, _z_owner_noop_copy)

_Bool z_session_check(const z_owned_session_t *val) { return val->_value.in != NULL; }
z_session_t z_session_loan(const z_owned_session_t *val) { return (z_session_t){._val = val->_value}; }
z_owned_session_t z_session_null(void) { return (z_owned_session_t){._value = {.in = NULL}}; }
z_owned_session_t *z_session_move(z_owned_session_t *val) { return val; }
z_owned_session_t z_session_clone(z_owned_session_t *val) {
    z_owned_session_t ret;
    ret._value = _z_session_rc_clone(&val->_value);
    return ret;
}
void z_session_drop(z_owned_session_t *val) { z_close(val); }

#define OWNED_FUNCTIONS_CLOSURE(ownedtype, name)                               \
    _Bool z_##name##_check(const ownedtype *val) { return val->call != NULL; } \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }                 \
    void z_##name##_drop(ownedtype *val) {                                     \
        if (val->drop != NULL) {                                               \
            (val->drop)(val->context);                                         \
            val->drop = NULL;                                                  \
        }                                                                      \
        val->call = NULL;                                                      \
        val->context = NULL;                                                   \
    }                                                                          \
    ownedtype z_##name##_null(void) {                                          \
        ownedtype v = {.call = NULL, .drop = NULL, .context = NULL};           \
        return v;                                                              \
    }

z_owned_closure_sample_t z_closure_sample(_z_data_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_sample_t){.call = call, .drop = drop, .context = context};
}

z_owned_closure_query_t z_closure_query(_z_queryable_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_query_t){.call = call, .drop = drop, .context = context};
}

z_owned_closure_reply_t z_closure_reply(z_owned_reply_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_reply_t){.call = call, .drop = drop, .context = context};
}

z_owned_closure_hello_t z_closure_hello(z_owned_hello_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_hello_t){.call = call, .drop = drop, .context = context};
}

z_owned_closure_zid_t z_closure_zid(z_id_handler_t call, _z_dropper_handler_t drop, void *context) {
    return (z_owned_closure_zid_t){.call = call, .drop = drop, .context = context};
}

OWNED_FUNCTIONS_CLOSURE(z_owned_closure_sample_t, closure_sample)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_query_t, closure_query)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_reply_t, closure_reply)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_hello_t, closure_hello)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_zid_t, closure_zid)

/************* Primitives **************/
typedef struct __z_hello_handler_wrapper_t {
    z_owned_hello_handler_t user_call;
    void *ctx;
} __z_hello_handler_wrapper_t;

void __z_hello_handler(_z_hello_t *hello, __z_hello_handler_wrapper_t *wrapped_ctx) {
    z_owned_hello_t ohello = {._value = hello};
    wrapped_ctx->user_call(&ohello, wrapped_ctx->ctx);
}

int8_t z_scout(z_owned_scouting_config_t *config, z_owned_closure_hello_t *callback) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->context;
    callback->context = NULL;

    // TODO[API-NET]: When API and NET are a single layer, there is no wrap the user callback and args
    //                to enclose the z_reply_t into a z_owned_reply_t.
    __z_hello_handler_wrapper_t *wrapped_ctx =
        (__z_hello_handler_wrapper_t *)z_malloc(sizeof(__z_hello_handler_wrapper_t));
    if (wrapped_ctx != NULL) {
        wrapped_ctx->user_call = callback->call;
        wrapped_ctx->ctx = ctx;

        char *opt_as_str = _z_config_get(config->_value, Z_CONFIG_SCOUTING_WHAT_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = Z_CONFIG_SCOUTING_WHAT_DEFAULT;
        }
        z_what_t what = strtol(opt_as_str, NULL, 10);

        opt_as_str = _z_config_get(config->_value, Z_CONFIG_MULTICAST_LOCATOR_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = Z_CONFIG_MULTICAST_LOCATOR_DEFAULT;
        }
        char *mcast_locator = opt_as_str;

        opt_as_str = _z_config_get(config->_value, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
        }
        uint32_t timeout = strtoul(opt_as_str, NULL, 10);

        _z_id_t zid = _z_id_empty();
        char *zid_str = _z_config_get(config->_value, Z_CONFIG_SESSION_ZID_KEY);
        if (zid_str != NULL) {
            _z_uuid_to_bytes(zid.id, zid_str);
        }

        _z_scout(what, zid, mcast_locator, timeout, __z_hello_handler, wrapped_ctx, callback->drop, ctx);

        z_free(wrapped_ctx);
        z_scouting_config_drop(config);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return ret;
}

z_owned_session_t z_open(z_owned_config_t *config) {
    z_owned_session_t zs = {._value = {.in = NULL}};

    // Create rc
    _z_session_rc_t zsrc = _z_session_rc_new();
    if (zsrc.in == NULL) {
        z_config_drop(config);
        return zs;
    }
    // Open session
    if (_z_open(&zsrc.in->val, config->_value) != _Z_RES_OK) {
        _z_session_rc_drop(&zsrc);
        z_config_drop(config);
        return zs;
    }
    // Store rc in session
    zs._value = zsrc;
    z_config_drop(config);
    return zs;
}

int8_t z_close(z_owned_session_t *zs) {
    if ((zs == NULL) || (zs->_value.in == NULL)) {
        return _Z_RES_OK;
    }
    _z_close(&zs->_value.in->val);
    _z_session_rc_drop(&zs->_value);
    zs->_value.in = NULL;
    return _Z_RES_OK;
}

int8_t z_info_peers_zid(const z_session_t zs, z_owned_closure_zid_t *callback) {
    // Call transport function
    switch (zs._val.in->val._tp._type) {
        case _Z_TRANSPORT_MULTICAST_TYPE:
        case _Z_TRANSPORT_RAWETH_TYPE:
            _zp_multicast_fetch_zid(&zs._val.in->val._tp, callback);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->context;
    callback->context = NULL;
    // Drop if needed
    if (callback->drop != NULL) {
        callback->drop(ctx);
    }
    return 0;
}

int8_t z_info_routers_zid(const z_session_t zs, z_owned_closure_zid_t *callback) {
    // Call transport function
    switch (zs._val.in->val._tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _zp_unicast_fetch_zid(&zs._val.in->val._tp, callback);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->context;
    callback->context = NULL;
    // Drop if needed
    if (callback->drop != NULL) {
        callback->drop(ctx);
    }
    return 0;
}

z_id_t z_info_zid(const z_session_t zs) { return zs._val.in->val._local_zid; }

#if Z_FEATURE_PUBLICATION == 1
OWNED_FUNCTIONS_PTR_COMMON(z_publisher_t, z_owned_publisher_t, publisher)
OWNED_FUNCTIONS_PTR_CLONE(z_publisher_t, z_owned_publisher_t, publisher, _z_owner_noop_copy)
void z_publisher_drop(z_owned_publisher_t *val) { z_undeclare_publisher(val); }

z_put_options_t z_put_options_default(void) {
    return (z_put_options_t) {
        .encoding = z_encoding_default(), .congestion_control = Z_CONGESTION_CONTROL_DEFAULT,
        .priority = Z_PRIORITY_DEFAULT,
#if Z_FEATURE_ATTACHMENT == 1
        .attachment = z_attachment_null()
#endif
    };
}

z_delete_options_t z_delete_options_default(void) {
    return (z_delete_options_t){.congestion_control = Z_CONGESTION_CONTROL_DEFAULT, .priority = Z_PRIORITY_DEFAULT};
}

int8_t z_put(z_session_t zs, z_keyexpr_t keyexpr, const uint8_t *payload, z_zint_t payload_len,
             const z_put_options_t *options) {
    int8_t ret = 0;

    z_put_options_t opt = z_put_options_default();
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.encoding = options->encoding;
        opt.priority = options->priority;
#if Z_FEATURE_ATTACHMENT == 1
        opt.attachment = options->attachment;
#endif
    }
    ret = _z_write(&zs._val.in->val, keyexpr, (const uint8_t *)payload, payload_len, opt.encoding, Z_SAMPLE_KIND_PUT,
                   opt.congestion_control, opt.priority
#if Z_FEATURE_ATTACHMENT == 1
                   ,
                   opt.attachment
#endif
    );

    // Trigger local subscriptions
    _z_trigger_local_subscriptions(&zs._val.in->val, keyexpr, payload, payload_len,
                                   _z_n_qos_make(0, opt.congestion_control == Z_CONGESTION_CONTROL_BLOCK, opt.priority)
#if Z_FEATURE_ATTACHMENT == 1
                                       ,
                                   opt.attachment
#endif
    );

    return ret;
}

int8_t z_delete(z_session_t zs, z_keyexpr_t keyexpr, const z_delete_options_t *options) {
    int8_t ret = 0;

    z_delete_options_t opt = z_delete_options_default();
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
    }
    ret = _z_write(&zs._val.in->val, keyexpr, NULL, 0, z_encoding_default(), Z_SAMPLE_KIND_DELETE,
                   opt.congestion_control, opt.priority
#if Z_FEATURE_ATTACHMENT == 1
                   ,
                   z_attachment_null()
#endif
    );

    return ret;
}

z_publisher_options_t z_publisher_options_default(void) {
    return (z_publisher_options_t){.congestion_control = Z_CONGESTION_CONTROL_DEFAULT, .priority = Z_PRIORITY_DEFAULT};
}

z_owned_publisher_t z_declare_publisher(z_session_t zs, z_keyexpr_t keyexpr, const z_publisher_options_t *options) {
    z_keyexpr_t key = keyexpr;

    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (zs._val.in->val._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&zs._val.in->val, &keyexpr);
        if (r == NULL) {
            uint16_t id = _z_declare_resource(&zs._val.in->val, keyexpr);
            key = _z_rid_with_suffix(id, NULL);
        }
    }

    z_publisher_options_t opt = z_publisher_options_default();
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
    }

    return (z_owned_publisher_t){._value = _z_declare_publisher(&zs._val, key, opt.congestion_control, opt.priority)};
}

int8_t z_undeclare_publisher(z_owned_publisher_t *pub) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_publisher(pub->_value);
    _z_publisher_free(&pub->_value);

    return ret;
}

z_publisher_put_options_t z_publisher_put_options_default(void) {
    return (z_publisher_put_options_t) {
        .encoding = z_encoding_default(),
#if Z_FEATURE_ATTACHMENT == 1
        .attachment = z_attachment_null()
#endif
    };
}

z_publisher_delete_options_t z_publisher_delete_options_default(void) {
    return (z_publisher_delete_options_t){.__dummy = 0};
}

int8_t z_publisher_put(const z_publisher_t pub, const uint8_t *payload, size_t len,
                       const z_publisher_put_options_t *options) {
    int8_t ret = _Z_RES_OK;

    z_publisher_put_options_t opt = z_publisher_put_options_default();
    if (options != NULL) {
        opt.encoding = options->encoding;
#if Z_FEATURE_ATTACHMENT == 1
        opt.attachment = options->attachment;
#endif
    }

    ret = _z_write(&pub._val->_zn.in->val, pub._val->_key, payload, len, opt.encoding, Z_SAMPLE_KIND_PUT,
                   pub._val->_congestion_control, pub._val->_priority
#if Z_FEATURE_ATTACHMENT == 1
                   ,
                   opt.attachment
#endif
    );

    // Trigger local subscriptions
    _z_trigger_local_subscriptions(&pub._val->_zn.in->val, pub._val->_key, payload, len, _Z_N_QOS_DEFAULT
#if Z_FEATURE_ATTACHMENT == 1
                                   ,
                                   opt.attachment
#endif
    );

    return ret;
}

int8_t z_publisher_delete(const z_publisher_t pub, const z_publisher_delete_options_t *options) {
    (void)(options);
    return _z_write(&pub._val->_zn.in->val, pub._val->_key, NULL, 0, z_encoding_default(), Z_SAMPLE_KIND_DELETE,
                    pub._val->_congestion_control, pub._val->_priority
#if Z_FEATURE_ATTACHMENT == 1
                    ,
                    z_attachment_null()
#endif
    );
}

z_owned_keyexpr_t z_publisher_keyexpr(z_publisher_t publisher) {
    z_owned_keyexpr_t ret = {._value = z_malloc(sizeof(_z_keyexpr_t))};
    if (ret._value != NULL && publisher._val != NULL) {
        *ret._value = _z_keyexpr_duplicate(publisher._val->_key);
    }
    return ret;
}
#endif

#if Z_FEATURE_QUERY == 1
OWNED_FUNCTIONS_PTR_INTERNAL(z_reply_t, z_owned_reply_t, reply, _z_reply_free, _z_owner_noop_copy)

z_get_options_t z_get_options_default(void) {
    return (z_get_options_t) {
        .target = z_query_target_default(), .consolidation = z_query_consolidation_default(),
        .value = {.encoding = z_encoding_default(), .payload = _z_bytes_empty()},
#if Z_FEATURE_ATTACHMENT == 1
        // TODO:ATT.attachment = z_attachment_null()
#endif
    };
}

typedef struct __z_reply_handler_wrapper_t {
    z_owned_reply_handler_t user_call;
    void *ctx;
} __z_reply_handler_wrapper_t;

void __z_reply_handler(_z_reply_t *reply, __z_reply_handler_wrapper_t *wrapped_ctx) {
    z_owned_reply_t oreply = {._value = reply};

    wrapped_ctx->user_call(&oreply, wrapped_ctx->ctx);
    z_reply_drop(&oreply);  // user_call is allowed to take ownership of the reply by setting oreply._value to NULL
}

int8_t z_get(z_session_t zs, z_keyexpr_t keyexpr, const char *parameters, z_owned_closure_reply_t *callback,
             const z_get_options_t *options) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->context;
    callback->context = NULL;

    z_get_options_t opt = z_get_options_default();
    if (options != NULL) {
        opt.consolidation = options->consolidation;
        opt.target = options->target;
        opt.value = options->value;
    }

    if (opt.consolidation.mode == Z_CONSOLIDATION_MODE_AUTO) {
        const char *lp = (parameters == NULL) ? "" : parameters;
        if (strstr(lp, Z_SELECTOR_TIME) != NULL) {
            opt.consolidation.mode = Z_CONSOLIDATION_MODE_NONE;
        } else {
            opt.consolidation.mode = Z_CONSOLIDATION_MODE_LATEST;
        }
    }

    // TODO[API-NET]: When API and NET are a single layer, there is no wrap the user callback and args
    //                to enclose the z_reply_t into a z_owned_reply_t.
    __z_reply_handler_wrapper_t *wrapped_ctx =
        (__z_reply_handler_wrapper_t *)z_malloc(sizeof(__z_reply_handler_wrapper_t));
    if (wrapped_ctx != NULL) {
        wrapped_ctx->user_call = callback->call;
        wrapped_ctx->ctx = ctx;
    }

    ret = _z_query(&zs._val.in->val, keyexpr, parameters, opt.target, opt.consolidation.mode, opt.value,
                   __z_reply_handler, wrapped_ctx, callback->drop, ctx
#if Z_FEATURE_ATTACHMENT == 1
                   ,
                   z_attachment_null()
    // TODO:ATT opt.attachment
#endif
    );
    return ret;
}

_Bool z_reply_is_ok(const z_owned_reply_t *reply) {
    (void)(reply);
    // For the moment always return TRUE.
    // The support for reply errors will come in the next release.
    return true;
}

z_sample_t z_reply_ok(const z_owned_reply_t *reply) { return reply->_value->data.sample; }

z_value_t z_reply_err(const z_owned_reply_t *reply) {
    (void)(reply);
    return (z_value_t){.payload = _z_bytes_empty(), .encoding = z_encoding_default()};
}
#endif

#if Z_FEATURE_QUERYABLE == 1
OWNED_FUNCTIONS_PTR_COMMON(z_queryable_t, z_owned_queryable_t, queryable)
OWNED_FUNCTIONS_PTR_CLONE(z_queryable_t, z_owned_queryable_t, queryable, _z_owner_noop_copy)
void z_queryable_drop(z_owned_queryable_t *val) { z_undeclare_queryable(val); }

z_queryable_options_t z_queryable_options_default(void) {
    return (z_queryable_options_t){.complete = _Z_QUERYABLE_COMPLETE_DEFAULT};
}

z_owned_queryable_t z_declare_queryable(z_session_t zs, z_keyexpr_t keyexpr, z_owned_closure_query_t *callback,
                                        const z_queryable_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;

    z_keyexpr_t key = keyexpr;

    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (zs._val.in->val._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&zs._val.in->val, &keyexpr);
        if (r == NULL) {
            uint16_t id = _z_declare_resource(&zs._val.in->val, keyexpr);
            key = _z_rid_with_suffix(id, NULL);
        }
    }

    z_queryable_options_t opt = z_queryable_options_default();
    if (options != NULL) {
        opt.complete = options->complete;
    }

    return (z_owned_queryable_t){
        ._value = _z_declare_queryable(&zs._val, key, opt.complete, callback->call, callback->drop, ctx)};
}

int8_t z_undeclare_queryable(z_owned_queryable_t *queryable) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_queryable(queryable->_value);
    _z_queryable_free(&queryable->_value);
    return ret;
}

z_query_reply_options_t z_query_reply_options_default(void) {
    return (z_query_reply_options_t){.encoding = z_encoding_default()};
}

int8_t z_query_reply(const z_query_t *query, const z_keyexpr_t keyexpr, const uint8_t *payload, size_t payload_len,
                     const z_query_reply_options_t *options) {
    z_query_reply_options_t opts = options == NULL ? z_query_reply_options_default() : *options;
    _z_value_t value = {.payload =
                            {
                                .start = payload,
                                ._is_alloc = false,
                                .len = payload_len,
                            },
                        .encoding = {.prefix = opts.encoding.prefix, .suffix = opts.encoding.suffix}};
    return _z_send_reply(&query->_val._rc.in->val, keyexpr, value);
    return _Z_ERR_GENERIC;
}
#endif

z_owned_keyexpr_t z_keyexpr_new(const char *name) {
    z_owned_keyexpr_t key;

    key._value = name ? (z_keyexpr_t *)z_malloc(sizeof(z_keyexpr_t)) : NULL;
    if (key._value != NULL) {
        *key._value = _z_rid_with_suffix(Z_RESOURCE_ID_NONE, name);
    }

    return key;
}

z_owned_keyexpr_t z_declare_keyexpr(z_session_t zs, z_keyexpr_t keyexpr) {
    z_owned_keyexpr_t key;

    key._value = (z_keyexpr_t *)z_malloc(sizeof(z_keyexpr_t));
    if (key._value != NULL) {
        uint16_t id = _z_declare_resource(&zs._val.in->val, keyexpr);
        *key._value = _z_rid_with_suffix(id, NULL);
    }

    return key;
}

int8_t z_undeclare_keyexpr(z_session_t zs, z_owned_keyexpr_t *keyexpr) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_resource(&zs._val.in->val, keyexpr->_value->_id);
    z_keyexpr_drop(keyexpr);

    return ret;
}

#if Z_FEATURE_SUBSCRIPTION == 1
OWNED_FUNCTIONS_PTR_COMMON(z_subscriber_t, z_owned_subscriber_t, subscriber)
OWNED_FUNCTIONS_PTR_CLONE(z_subscriber_t, z_owned_subscriber_t, subscriber, _z_owner_noop_copy)
void z_subscriber_drop(z_owned_subscriber_t *val) { z_undeclare_subscriber(val); }

OWNED_FUNCTIONS_PTR_COMMON(z_pull_subscriber_t, z_owned_pull_subscriber_t, pull_subscriber)
OWNED_FUNCTIONS_PTR_CLONE(z_pull_subscriber_t, z_owned_pull_subscriber_t, pull_subscriber, _z_owner_noop_copy)
void z_pull_subscriber_drop(z_owned_pull_subscriber_t *val) { z_undeclare_pull_subscriber(val); }

z_subscriber_options_t z_subscriber_options_default(void) {
    return (z_subscriber_options_t){.reliability = Z_RELIABILITY_DEFAULT};
}

z_pull_subscriber_options_t z_pull_subscriber_options_default(void) {
    return (z_pull_subscriber_options_t){.reliability = Z_RELIABILITY_DEFAULT};
}

z_owned_subscriber_t z_declare_subscriber(z_session_t zs, z_keyexpr_t keyexpr, z_owned_closure_sample_t *callback,
                                          const z_subscriber_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;
    char *suffix = NULL;

    z_keyexpr_t key = keyexpr;
    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (zs._val.in->val._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&zs._val.in->val, &keyexpr);
        if (r == NULL) {
            char *wild = strpbrk(keyexpr._suffix, "*$");
            _Bool do_keydecl = true;
            if (wild != NULL && wild != keyexpr._suffix) {
                wild -= 1;
                size_t len = wild - keyexpr._suffix;
                suffix = z_malloc(len + 1);
                if (suffix != NULL) {
                    memcpy(suffix, keyexpr._suffix, len);
                    suffix[len] = 0;
                    keyexpr._suffix = suffix;
                    _z_keyexpr_set_owns_suffix(&keyexpr, false);
                } else {
                    do_keydecl = false;
                }
            }
            if (do_keydecl) {
                uint16_t id = _z_declare_resource(&zs._val.in->val, keyexpr);
                key = _z_rid_with_suffix(id, wild);
            }
        }
    }

    _z_subinfo_t subinfo = _z_subinfo_push_default();
    if (options != NULL) {
        subinfo.reliability = options->reliability;
    }
    _z_subscriber_t *sub = _z_declare_subscriber(&zs._val, key, subinfo, callback->call, callback->drop, ctx);
    if (suffix != NULL) {
        z_free(suffix);
    }

    return (z_owned_subscriber_t){._value = sub};
}

z_owned_pull_subscriber_t z_declare_pull_subscriber(z_session_t zs, z_keyexpr_t keyexpr,
                                                    z_owned_closure_sample_t *callback,
                                                    const z_pull_subscriber_options_t *options) {
    (void)(options);

    void *ctx = callback->context;
    callback->context = NULL;

    z_keyexpr_t key = keyexpr;
    _z_resource_t *r = _z_get_resource_by_key(&zs._val.in->val, &keyexpr);
    if (r == NULL) {
        uint16_t id = _z_declare_resource(&zs._val.in->val, keyexpr);
        key = _z_rid_with_suffix(id, NULL);
    }

    _z_subinfo_t subinfo = _z_subinfo_pull_default();
    if (options != NULL) {
        subinfo.reliability = options->reliability;
    }

    return (z_owned_pull_subscriber_t){
        ._value = _z_declare_subscriber(&zs._val, key, subinfo, callback->call, callback->drop, ctx)};
}

int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub) {
    int8_t ret = _Z_RES_OK;

    _z_undeclare_subscriber(sub->_value);
    _z_subscriber_free(&sub->_value);

    return ret;
}

int8_t z_undeclare_pull_subscriber(z_owned_pull_subscriber_t *sub) {
    int8_t ret = _Z_RES_OK;

    _z_undeclare_subscriber(sub->_value);
    _z_subscriber_free(&sub->_value);

    return ret;
}

int8_t z_subscriber_pull(const z_pull_subscriber_t sub) { return _z_subscriber_pull(sub._val); }

z_owned_keyexpr_t z_subscriber_keyexpr(z_subscriber_t sub) {
    z_owned_keyexpr_t ret = z_keyexpr_null();
    uint32_t lookup = sub._val->_entity_id;
    if (sub._val != NULL) {
        _z_subscription_rc_list_t *tail = sub._val->_zn.in->val._local_subscriptions;
        while (tail != NULL && !z_keyexpr_check(&ret)) {
            _z_subscription_rc_t *head = _z_subscription_rc_list_head(tail);
            if (head->in->val._id == lookup) {
                _z_keyexpr_t key = _z_keyexpr_duplicate(head->in->val._key);
                ret = (z_owned_keyexpr_t){._value = z_malloc(sizeof(_z_keyexpr_t))};
                if (ret._value != NULL) {
                    *ret._value = key;
                } else {
                    _z_keyexpr_clear(&key);
                }
            }
            tail = _z_subscription_rc_list_tail(tail);
        }
    }
    return ret;
}
#endif

/**************** Tasks ****************/
zp_task_read_options_t zp_task_read_options_default(void) {
    return (zp_task_read_options_t) {
#if Z_FEATURE_MULTI_THREAD == 1
        .task_attributes = NULL
#else
        .__dummy = 0
#endif
    };
}

int8_t zp_start_read_task(z_session_t zs, const zp_task_read_options_t *options) {
    (void)(options);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_task_read_options_t opt = zp_task_read_options_default();
    if (options != NULL) {
        opt.task_attributes = options->task_attributes;
    }
    return _zp_start_read_task(&zs._val.in->val, opt.task_attributes);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_read_task(z_session_t zs) {
#if Z_FEATURE_MULTI_THREAD == 1
    return _zp_stop_read_task(&zs._val.in->val);
#else
    (void)(zs);
    return -1;
#endif
}

zp_task_lease_options_t zp_task_lease_options_default(void) {
    return (zp_task_lease_options_t) {
#if Z_FEATURE_MULTI_THREAD == 1
        .task_attributes = NULL
#else
        .__dummy = 0
#endif
    };
}

int8_t zp_start_lease_task(z_session_t zs, const zp_task_lease_options_t *options) {
    (void)(options);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_task_lease_options_t opt = zp_task_lease_options_default();
    if (options != NULL) {
        opt.task_attributes = options->task_attributes;
    }
    return _zp_start_lease_task(&zs._val.in->val, opt.task_attributes);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_lease_task(z_session_t zs) {
#if Z_FEATURE_MULTI_THREAD == 1
    return _zp_stop_lease_task(&zs._val.in->val);
#else
    (void)(zs);
    return -1;
#endif
}

zp_read_options_t zp_read_options_default(void) { return (zp_read_options_t){.__dummy = 0}; }

int8_t zp_read(z_session_t zs, const zp_read_options_t *options) {
    (void)(options);
    return _zp_read(&zs._val.in->val);
}

zp_send_keep_alive_options_t zp_send_keep_alive_options_default(void) {
    return (zp_send_keep_alive_options_t){.__dummy = 0};
}

int8_t zp_send_keep_alive(z_session_t zs, const zp_send_keep_alive_options_t *options) {
    (void)(options);
    return _zp_send_keep_alive(&zs._val.in->val);
}

zp_send_join_options_t zp_send_join_options_default(void) { return (zp_send_join_options_t){.__dummy = 0}; }

int8_t zp_send_join(z_session_t zs, const zp_send_join_options_t *options) {
    (void)(options);
    return _zp_send_join(&zs._val.in->val);
}
#if Z_FEATURE_ATTACHMENT == 1
void _z_bytes_pair_clear(struct _z_bytes_pair_t *this_) {
    _z_bytes_clear(&this_->key);
    _z_bytes_clear(&this_->value);
}

z_attachment_t z_bytes_map_as_attachment(const z_owned_bytes_map_t *this_) {
    if (!z_bytes_map_check(this_)) {
        return z_attachment_null();
    }
    return (z_attachment_t){.data = this_, .iteration_driver = (z_attachment_iter_driver_t)z_bytes_map_iter};
}
bool z_bytes_map_check(const z_owned_bytes_map_t *this_) { return this_->_inner != NULL; }
void z_bytes_map_drop(z_owned_bytes_map_t *this_) { _z_bytes_pair_list_free(&this_->_inner); }

int8_t _z_bytes_map_insert_by_alias(z_bytes_t key, z_bytes_t value, void *this_) {
    z_bytes_map_insert_by_alias((z_owned_bytes_map_t *)this_, key, value);
    return 0;
}
int8_t _z_bytes_map_insert_by_copy(z_bytes_t key, z_bytes_t value, void *this_) {
    z_bytes_map_insert_by_copy((z_owned_bytes_map_t *)this_, key, value);
    return 0;
}
z_owned_bytes_map_t z_bytes_map_from_attachment(z_attachment_t this_) {
    if (!z_attachment_check(&this_)) {
        return z_bytes_map_null();
    }
    z_owned_bytes_map_t map = z_bytes_map_new();
    z_attachment_iterate(this_, _z_bytes_map_insert_by_copy, &map);
    return map;
}
z_owned_bytes_map_t z_bytes_map_from_attachment_aliasing(z_attachment_t this_) {
    if (!z_attachment_check(&this_)) {
        return z_bytes_map_null();
    }
    z_owned_bytes_map_t map = z_bytes_map_new();
    z_attachment_iterate(this_, _z_bytes_map_insert_by_alias, &map);
    return map;
}
z_bytes_t z_bytes_map_get(const z_owned_bytes_map_t *this_, z_bytes_t key) {
    _z_bytes_pair_list_t *current = this_->_inner;
    while (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        if (_z_bytes_eq(&key, &head->key)) {
            return _z_bytes_wrap(head->value.start, head->value.len);
        }
    }
    return z_bytes_null();
}
void z_bytes_map_insert_by_alias(const z_owned_bytes_map_t *this_, z_bytes_t key, z_bytes_t value) {
    _z_bytes_pair_list_t *current = this_->_inner;
    while (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        if (_z_bytes_eq(&key, &head->key)) {
            break;
        }
        current = _z_bytes_pair_list_tail(current);
    }
    if (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        _z_bytes_clear(&head->value);
        head->value = _z_bytes_wrap(value.start, value.len);
    } else {
        struct _z_bytes_pair_t *insert = z_malloc(sizeof(struct _z_bytes_pair_t));
        memset(insert, 0, sizeof(struct _z_bytes_pair_t));
        insert->key = _z_bytes_wrap(key.start, key.len);
        insert->value = _z_bytes_wrap(value.start, value.len);
        ((z_owned_bytes_map_t *)this_)->_inner = _z_bytes_pair_list_push(this_->_inner, insert);
    }
}
void z_bytes_map_insert_by_copy(const z_owned_bytes_map_t *this_, z_bytes_t key, z_bytes_t value) {
    _z_bytes_pair_list_t *current = this_->_inner;
    while (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        if (_z_bytes_eq(&key, &head->key)) {
            break;
        }
        current = _z_bytes_pair_list_tail(current);
    }
    if (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        _z_bytes_clear(&head->value);
        _z_bytes_copy(&head->value, &value);
        if (!head->key._is_alloc) {
            _z_bytes_copy(&head->key, &key);
        }
    } else {
        struct _z_bytes_pair_t *insert = z_malloc(sizeof(struct _z_bytes_pair_t));
        memset(insert, 0, sizeof(struct _z_bytes_pair_t));
        _z_bytes_copy(&insert->key, &key);
        _z_bytes_copy(&insert->value, &value);
        ((z_owned_bytes_map_t *)this_)->_inner = _z_bytes_pair_list_push(this_->_inner, insert);
    }
}
int8_t z_bytes_map_iter(const z_owned_bytes_map_t *this_, z_attachment_iter_body_t body, void *ctx) {
    _z_bytes_pair_list_t *current = this_->_inner;
    while (current) {
        struct _z_bytes_pair_t *head = _z_bytes_pair_list_head(current);
        int8_t ret = body(head->key, head->value, ctx);
        if (ret) {
            return ret;
        }
        current = _z_bytes_pair_list_tail(current);
    }
    return 0;
}
z_owned_bytes_map_t z_bytes_map_new(void) { return (z_owned_bytes_map_t){._inner = _z_bytes_pair_list_new()}; }
z_owned_bytes_map_t z_bytes_map_null(void) { return (z_owned_bytes_map_t){._inner = NULL}; }
z_bytes_t z_bytes_from_str(const char *str) { return z_bytes_wrap((const uint8_t *)str, strlen(str)); }
z_bytes_t z_bytes_null(void) { return (z_bytes_t){.len = 0, ._is_alloc = false, .start = NULL}; }
#endif