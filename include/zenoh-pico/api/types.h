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

#ifndef INCLUDE_ZENOH_PICO_API_TYPES_H
#define INCLUDE_ZENOH_PICO_API_TYPES_H

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/net/publish.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/protocol/core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Owned types */
#define _OWNED_TYPE_PTR(type, name) \
    typedef struct {                \
        type *_value;               \
    } z_owned_##name##_t;

#define _OWNED_TYPE_STR(type, name) \
    typedef struct {                \
        type _value;                \
    } z_owned_##name##_t;

/**
 * Represents a variable-length encoding unsigned integer.
 *
 * It is equivalent to the size of a ``size_t``.
 */
typedef _z_zint_t z_zint_t;

/**
 * Represents an array of bytes.
 *
 * Members:
 *   size_t len: The length of the bytes array.
 *   uint8_t *start: A pointer to the bytes array.
 */
typedef _z_bytes_t z_bytes_t;
_Bool z_bytes_check(const z_bytes_t *v);

/**
 * Represents a Zenoh ID.
 *
 * In general, valid Zenoh IDs are LSB-first 128bit unsigned and non-zero integers.
 *
 * Members:
 *   uint8_t id[16]: The array containing the 16 octets of a Zenoh ID.
 */
typedef _z_id_t z_id_t;

/**
 * Represents a string without null-terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const char *val: A pointer to the string.
 */
typedef _z_string_t z_string_t;

typedef _z_str_t z_str_t;
_OWNED_TYPE_STR(z_str_t, str)

/**
 * Represents a key expression in Zenoh.
 *
 * Operations over :c:type:`z_keyexpr_t` must be done using the provided functions:
 *
 *   - :c:func:`z_keyexpr`
 *   - :c:func:`z_keyexpr_is_initialized`
 *   - :c:func:`z_keyexpr_to_string`
 *   - :c:func:`zp_keyexpr_resolve`
 */
typedef _z_keyexpr_t z_keyexpr_t;
_OWNED_TYPE_PTR(z_keyexpr_t, keyexpr)

/**
 * Represents a Zenoh configuration.
 *
 * Configurations are usually used to set the parameters of a Zenoh session upon its opening.
 *
 * Operations over :c:type:`z_config_t` must be done using the provided functions:
 *
 *   - :c:func:`z_config_new`
 *   - :c:func:`z_config_default`
 *   - :c:func:`zp_config_get`
 *   - :c:func:`zp_config_insert`
 */
typedef struct {
    _z_config_t *_val;
} z_config_t;
_OWNED_TYPE_PTR(_z_config_t, config)

/**
 * Represents a scouting configuration.
 *
 * Configurations are usually used to set the parameters of the scouting procedure.
 *
 * Operations over :c:type:`z_scouting_config_t` must be done using the provided functions:
 *
 *   - :c:func:`z_scouting_config_default`
 *   - :c:func:`z_scouting_config_from`
 *   - :c:func:`zp_scouting_config_get`
 *   - :c:func:`zp_scouting_config_insert`
 */
typedef struct {
    _z_scouting_config_t *_val;
} z_scouting_config_t;
_OWNED_TYPE_PTR(_z_scouting_config_t, scouting_config)

/**
 * Represents a Zenoh session.
 */
typedef struct {
    _z_session_rc_t _val;
} z_session_t;

typedef struct {
    _z_session_rc_t _value;
} z_owned_session_t;

/**
 * Represents a Zenoh (push) Subscriber entity.
 *
 * Operations over :c:type:`z_subscriber_t` must be done using the provided functions:
 *
 *   - :c:func:`z_declare_subscriber`
 *   - :c:func:`z_undeclare_subscriber`
 */
typedef struct {
    _z_subscriber_t *_val;
} z_subscriber_t;
_OWNED_TYPE_PTR(_z_subscriber_t, subscriber)

/**
 * Represents a Zenoh Pull Subscriber entity.
 *
 * Operations over :c:type:`z_pull_subscriber_t` must be done using the provided functions:
 *
 *   - :c:func:`z_declare_pull_subscriber`
 *   - :c:func:`z_undeclare_pull_subscriber`
 *   - :c:func:`z_subscriber_pull`
 */
typedef struct {
    _z_pull_subscriber_t *_val;
} z_pull_subscriber_t;
_OWNED_TYPE_PTR(_z_pull_subscriber_t, pull_subscriber)

/**
 * Represents a Zenoh Publisher entity.
 *
 * Operations over :c:type:`z_publisher_t` must be done using the provided functions:
 *
 *   - :c:func:`z_declare_publisher`
 *   - :c:func:`z_undeclare_publisher`
 *   - :c:func:`z_publisher_put`
 *   - :c:func:`z_publisher_delete`
 */
typedef struct {
    _z_publisher_t *_val;
} z_publisher_t;
_OWNED_TYPE_PTR(_z_publisher_t, publisher)

/**
 * Represents a Zenoh Queryable entity.
 *
 * Operations over :c:type:`z_queryable_t` must be done using the provided functions:
 *
 *   - :c:func:`z_declare_queryable`
 *   - :c:func:`z_undeclare_queryable`
 */
typedef struct {
    _z_queryable_t *_val;
} z_queryable_t;
_OWNED_TYPE_PTR(_z_queryable_t, queryable)

/**
 * Represents a Zenoh query entity, received by Zenoh Queryable entities.
 *
 */
typedef struct z_query_t {
    z_owned_query_t _val;
} z_query_t;

/**
 * Represents the encoding of a payload, in a MIME-like format.
 *
 * Members:
 *   z_encoding_prefix_t prefix: The integer prefix of this encoding.
 *   z_bytes_t suffix: The suffix of this encoding. It MUST be a valid UTF-8 string.
 */
typedef _z_encoding_t z_encoding_t;

/*
 * Represents timestamp value in Zenoh
 */
typedef _z_timestamp_t z_timestamp_t;

/**
 * Represents a Zenoh value.
 *
 * Members:
 *   z_encoding_t encoding: The encoding of the `payload`.
 *   z_bytes_t payload: The payload of this zenoh value.
 */
typedef _z_value_t z_value_t;

/**
 * Represents the set of options that can be applied to a (push) subscriber,
 * upon its declaration via :c:func:`z_declare_subscriber`.
 *
 * Members:
 *   z_reliability_t reliability: The subscription reliability.
 */
typedef struct {
    z_reliability_t reliability;
} z_subscriber_options_t;

/**
 * Represents the set of options that can be applied to a pull subscriber,
 * upon its declaration via :c:func:`z_declare_pull_subscriber`.
 *
 * Members:
 *   z_reliability_t reliability: The subscription reliability.
 */
typedef struct {
    z_reliability_t reliability;
} z_pull_subscriber_options_t;

/**
 * Represents the replies consolidation to apply on replies to a :c:func:`z_get`.
 *
 * Members:
 *   z_consolidation_mode_t mode: Defines the consolidation mode to apply to the replies.
 */
typedef struct {
    z_consolidation_mode_t mode;
} z_query_consolidation_t;

/**
 * Represents the set of options that can be applied to a publisher,
 * upon its declaration via :c:func:`z_declare_publisher`.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing messages from this
 * publisher.
 *   z_priority_t priority: The priority of messages issued by this publisher.
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
} z_publisher_options_t;

/**
 * Represents the set of options that can be applied to a queryable,
 * upon its declaration via :c:func:`z_declare_queryable`.
 *
 * Members:
 *   _Bool complete: The completeness of the queryable.
 */
typedef struct {
    _Bool complete;
} z_queryable_options_t;

/**
 * Represents the set of options that can be applied to a query reply,
 * sent via :c:func:`z_query_reply`.
 *
 * Members:
 *   z_encoding_t encoding: The encoding of the payload.
 *   z_attachment_t attachment: an attachment to the response.
 */
typedef struct {
    z_encoding_t encoding;
#if Z_FEATURE_ATTACHMENT == 1
    // TODO:ATT z_attachment_t attachment;
#endif
} z_query_reply_options_t;

/**
 * Represents the set of options that can be applied to the put operation,
 * whenever issued via :c:func:`z_put`.
 *
 * Members:
 *   z_encoding_t encoding: The encoding of the payload.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 */
typedef struct {
    z_encoding_t encoding;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t attachment;
#endif
} z_put_options_t;

/**
 * Represents the set of options that can be applied to the delete operation,
 * whenever issued via :c:func:`z_delete`.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when router.
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
} z_delete_options_t;

/**
 * Represents the set of options that can be applied to the put operation by a previously declared publisher,
 * whenever issued via :c:func:`z_publisher_put`.
 *
 * Members:
 *   z_encoding_t encoding: The encoding of the payload.
 */
typedef struct {
    z_encoding_t encoding;
#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t attachment;
#endif
} z_publisher_put_options_t;

/**
 * Represents the set of options that can be applied to the delete operation by a previously declared publisher,
 * whenever issued via :c:func:`z_publisher_delete`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} z_publisher_delete_options_t;

/**
 * Represents the set of options that can be applied to the get operation,
 * whenever issued via :c:func:`z_get`.
 *
 * Members:
 *   z_query_target_t target: The queryables that should be targeted by this get.
 *   z_query_consolidation_t consolidation: The replies consolidation strategy to apply on replies.
 *   z_value_t value: The payload to include in the query.
 */
typedef struct {
    z_value_t value;
    z_query_consolidation_t consolidation;
    z_query_target_t target;
#if Z_FEATURE_ATTACHMENT == 1
// TODO:ATT z_attachment_t attachment;
#endif
} z_get_options_t;

/**
 * Represents the set of options that can be applied to the read task,
 * whenever issued via :c:func:`zp_start_read_task`.
 */
typedef struct {
#if Z_FEATURE_MULTI_THREAD == 1
    z_task_attr_t *task_attributes;
#else
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
#endif
} zp_task_read_options_t;

/**
 * Represents the set of options that can be applied to the lease task,
 * whenever issued via :c:func:`zp_start_lease_task`.
 */
typedef struct {
#if Z_FEATURE_MULTI_THREAD == 1
    z_task_attr_t *task_attributes;
#else
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
#endif
} zp_task_lease_options_t;

/**
 * Represents the set of options that can be applied to the read operation,
 * whenever issued via :c:func:`zp_read`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_read_options_t;

/**
 * Represents the set of options that can be applied to the keep alive send,
 * whenever issued via :c:func:`zp_send_keep_alive`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_send_keep_alive_options_t;

/**
 * Represents the set of options that can be applied to the join send,
 * whenever issued via :c:func:`zp_send_join`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_send_join_options_t;

/**
 * QoS settings of zenoh message.
 */
typedef _z_qos_t z_qos_t;
/**
 * Returns message priority.
 */
static inline z_priority_t z_qos_get_priority(z_qos_t qos) {
    z_priority_t ret = _z_n_qos_get_priority(qos);
    return ret == _Z_PRIORITY_CONTROL ? Z_PRIORITY_DEFAULT : ret;
}
/**
 * Returns message congestion control.
 */
static inline z_congestion_control_t z_qos_get_congestion_control(z_qos_t qos) {
    return _z_n_qos_get_congestion_control(qos);
}
/**
 * Returns message express flag. If set to true, the message is not batched to reduce the latency.
 */
static inline _Bool z_qos_get_express(z_qos_t qos) { return _z_n_qos_get_express(qos); }
/**
 * Returns default qos settings.
 */
static inline z_qos_t z_qos_default(void) { return _Z_N_QOS_DEFAULT; }

/**
 * Represents a data sample.
 *
 * A sample is the value associated to a given :c:type:`z_keyexpr_t` at a given point in time.
 *
 * Members:
 *   z_keyexpr_t keyexpr: The keyexpr of this data sample.
 *   z_bytes_t payload: The value of this data sample.
 *   z_encoding_t encoding: The encoding of the value of this data sample.
 *   z_sample_kind_t kind: The kind of this data sample (PUT or DELETE).
 *   z_timestamp_t timestamp: The timestamp of this data sample.
 *   z_qos_t qos: Quality of service settings used to deliver this sample.
 */
typedef _z_sample_t z_sample_t;

/**
 * Represents the content of a `hello` message returned by a zenoh entity as a reply to a `scout` message.
 *
 * Members:
 *   z_whatami_t whatami: The kind of zenoh entity.
 *   z_bytes_t zid: The Zenoh ID of the scouted entity (empty if absent).
 *   z_str_array_t locators: The locators of the scouted entity.
 */
typedef _z_hello_t z_hello_t;
_OWNED_TYPE_PTR(z_hello_t, hello)

/**
 * Represents the content of a reply to a query.
 *
 * Members:
 *   z_sample_t sample: The :c:type:`_z_sample_t` containing the key and value of the reply.
 *   z_bytes_t replier_id: The id of the replier that sent this reply.
 */
typedef _z_reply_data_t z_reply_data_t;

/**
 * Represents the reply to a query.
 *
 * Members:
 *   z_reply_data_t data: the content of the reply.
 */
typedef _z_reply_t z_reply_t;
_OWNED_TYPE_PTR(z_reply_t, reply)

/**
 * Represents an array of ``z_str_t``.
 *
 * Operations over :c:type:`z_str_array_t` must be done using the provided functions:
 *
 *   - ``char *z_str_array_get(z_str_array_t *a, size_t k);``
 *   - ``size_t z_str_array_len(z_str_array_t *a);``
 *   - ``_Bool z_str_array_array_is_empty(z_str_array_t *a);``
 */
typedef _z_str_array_t z_str_array_t;

z_str_t *z_str_array_get(const z_str_array_t *a, size_t k);
size_t z_str_array_len(const z_str_array_t *a);
_Bool z_str_array_is_empty(const z_str_array_t *a);
_OWNED_TYPE_PTR(z_str_array_t, str_array)

typedef void (*_z_dropper_handler_t)(void *arg);

/**
 * Represents the sample closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   _z_data_handler_t call: `void *call(const struct z_sample_t*, const void *context)` is the callback function.
 *   _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
typedef struct {
    void *context;
    _z_data_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_sample_t;

void z_closure_sample_call(const z_owned_closure_sample_t *closure, const z_sample_t *sample);

/**
 * Represents the query callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   _z_queryable_handler_t call: `void (*_z_queryable_handler_t)(z_query_t *query, void *arg)` is the
 * callback function.
 *   _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
typedef struct {
    void *context;
    _z_queryable_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_query_t;

void z_closure_query_call(const z_owned_closure_query_t *closure, const z_query_t *query);

typedef void (*z_owned_reply_handler_t)(z_owned_reply_t *reply, void *arg);

/**
 * Represents the query reply callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   z_owned_reply_handler_t call: `void (*z_owned_reply_handler_t)(z_owned_reply_t reply, void *arg)` is the callback
 * function.
 *   _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
typedef struct {
    void *context;
    z_owned_reply_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_reply_t;

void z_closure_reply_call(const z_owned_closure_reply_t *closure, z_owned_reply_t *reply);

typedef void (*z_owned_hello_handler_t)(z_owned_hello_t *hello, void *arg);

/**
 * Represents the Zenoh ID callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   z_owned_hello_handler_t call: `void (*z_owned_hello_handler_t)(const z_owned_hello_t *hello, void *arg)` is the
 * callback function.
 *   _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
typedef struct {
    void *context;
    z_owned_hello_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_hello_t;

void z_closure_hello_call(const z_owned_closure_hello_t *closure, z_owned_hello_t *hello);

typedef void (*z_id_handler_t)(const z_id_t *id, void *arg);

/**
 * Represents the Zenoh ID callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   z_id_handler_t call: `void (*z_id_handler_t)(const z_id_t *id, void *arg)` is the callback function.
 *   _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
typedef struct {
    void *context;
    z_id_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_zid_t;

void z_closure_zid_call(const z_owned_closure_zid_t *closure, const z_id_t *id);
#if Z_FEATURE_ATTACHMENT == 1
struct _z_bytes_pair_t {
    _z_bytes_t key;
    _z_bytes_t value;
};

void _z_bytes_pair_clear(struct _z_bytes_pair_t *this_);

_Z_ELEM_DEFINE(_z_bytes_pair, struct _z_bytes_pair_t, _z_noop_size, _z_bytes_pair_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_bytes_pair, struct _z_bytes_pair_t)

/**
 * A map of maybe-owned vector of bytes to maybe-owned vector of bytes.
 */
typedef struct z_owned_bytes_map_t {
    _z_bytes_pair_list_t *_inner;
} z_owned_bytes_map_t;

/**
 * Aliases `this` into a generic `z_attachment_t`, allowing it to be passed to corresponding APIs.
 */
z_attachment_t z_bytes_map_as_attachment(const z_owned_bytes_map_t *this_);
/**
 * Returns `true` if the map is not in its gravestone state
 */
bool z_bytes_map_check(const z_owned_bytes_map_t *this_);
/**
 * Destroys the map, resetting `this` to its gravestone value.
 *
 * This function is double-free safe, passing a pointer to the gravestone value will have no effect.
 */
void z_bytes_map_drop(z_owned_bytes_map_t *this_);
/**
 * Constructs a map from the provided attachment, copying keys and values.
 *
 * If `this` is at gravestone value, the returned value will also be at gravestone value.
 */
z_owned_bytes_map_t z_bytes_map_from_attachment(z_attachment_t this_);
/**
 * Constructs a map from the provided attachment, aliasing the attachment's keys and values.
 *
 * If `this` is at gravestone value, the returned value will also be at gravestone value.
 */

z_owned_bytes_map_t z_bytes_map_from_attachment_aliasing(z_attachment_t this_);
/**
 * Returns the value associated with `key`, returning a gravestone value if:
 * - `this` or `key` is in gravestone state.
 * - `this` has no value associated to `key`
 */

z_bytes_t z_bytes_map_get(const z_owned_bytes_map_t *this_, z_bytes_t key);
/**
 * Associates `value` to `key` in the map, aliasing them.
 *
 * Note that once `key` is aliased, reinserting at the same key may alias the previous instance, or the new instance of
 * `key`.
 *
 * Calling this with `NULL` or the gravestone value is undefined behaviour.
 */

void z_bytes_map_insert_by_alias(const z_owned_bytes_map_t *this_, z_bytes_t key, z_bytes_t value);
/**
 * Associates `value` to `key` in the map, copying them to obtain ownership: `key` and `value` are not aliased past the
 * function's return.
 *
 * Calling this with `NULL` or the gravestone value is undefined behaviour.
 */

void z_bytes_map_insert_by_copy(const z_owned_bytes_map_t *this_, z_bytes_t key, z_bytes_t value);
/**
 * Iterates over the key-value pairs in the map.
 *
 * `body` will be called once per pair, with `ctx` as its last argument.
 * If `body` returns a non-zero value, the iteration will stop immediately and the value will be returned.
 * Otherwise, this will return 0 once all pairs have been visited.
 * `body` is not given ownership of the key nor value, which alias the pairs in the map.
 * It is safe to keep these aliases until existing keys are modified/removed, or the map is destroyed.
 * Note that this map is unordered.
 *
 * Calling this with `NULL` or the gravestone value is undefined behaviour.
 */

int8_t z_bytes_map_iter(const z_owned_bytes_map_t *this_, z_attachment_iter_body_t body, void *ctx);
/**
 * Constructs a new map.
 */
z_owned_bytes_map_t z_bytes_map_new(void);
/**
 * Constructs the gravestone value for `z_owned_bytes_map_t`
 */
z_owned_bytes_map_t z_bytes_map_null(void);
#endif

/**
 * Returns a view of `str` using `strlen` (this constructor should not be used on untrusted input).
 *
 * `str == NULL` will cause this to return `z_bytes_null()`
 */
z_bytes_t z_bytes_from_str(const char *str);
/**
 * Returns the gravestone value for `z_bytes_t`
 */
z_bytes_t z_bytes_null(void);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_API_TYPES_H */
