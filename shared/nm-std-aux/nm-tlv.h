// SPDX-License-Identifier: LGPL-2.1+

#ifndef __NM_TLV_H__
#define __NM_TLV_H__

#include <endian.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nm-std-aux.h"
#include "unaligned.h"

#define NM_TLV_ALIGNTO sizeof (int32_t)

static inline size_t
NM_TLV_ALIGN (size_t len)
{
	return (len + (NM_TLV_ALIGNTO - 1u)) & ~((size_t) (NM_TLV_ALIGNTO - 1u));
}

typedef enum {
	NM_TLV_PARSE_FLAGS_NONE           = 0,

	/* During nm_tlv_parse(), if there are unknown attributes, fail.
	 * Ignoring unknown attributes makes the parsing forward compatible. */
	NM_TLV_PARSE_FLAGS_FAIL_UNKNOWN   = (1ull << 0),

	NM_TLV_PARSE_FLAGS_STRICT         = NM_TLV_PARSE_FLAGS_FAIL_UNKNOWN,
} NMTlvParseFlags;

typedef enum {
	NM_TLV_TYPE_NONE = 0,
	NM_TLV_TYPE_BOOL,
	NM_TLV_TYPE_UINT32,
	NM_TLV_TYPE_INT32,
	NM_TLV_TYPE_UINT64,
	NM_TLV_TYPE_INT64,
	NM_TLV_TYPE_STR,
	NM_TLV_TYPE_MEM,
} NMTlvType;

typedef struct {
	NMTlvType type;

	/* Whether the same attribute can be specified more than once. Otherwise,
	 * parse() rejects duplicate/repeated attributes.
	 *
	 * Repeated attributes can only be accessed by iterating all attributes
	 * or fetching them with nm_tlv_msg_get_attrs().
	 */
	bool allow_repeated;
} NMTlvPolicy;

#define NM_TLV_POLICY_INIT(t) \
	((NMTlvPolicy) { \
		.type = (t), \
	})

typedef uint8_t NMTlvAttType;

typedef struct {
	NMTlvAttType att_type;
	uint8_t len16;
	uint8_t len08;
	uint8_t len00;
} NMTlvAttr;

#define NM_TLV_ATTR_LEN_MAX ((uint32_t) 0xFFFFFFu)

static inline uint32_t
nm_tlv_attr_len (const NMTlvAttr *attr)
{
	nm_assert (attr);

	return   (((uint32_t) attr->len16) << 16)
	       + (((uint32_t) attr->len08) <<  8)
	       +  ((uint32_t) attr->len00);
}

#define _nm_tlv_policy_assert(policy_arr, att_type, tlv_type) \
	do { \
		const NMTlvPolicy *const _policy_arr = (policy_arr); \
		const NMTlvAttType _att_type = (att_type); \
		\
		nm_assert (_policy_arr); \
		nm_assert (_policy_arr[_att_type].type == (tlv_type)); \
	} while (0)

#define _nm_tlv_attr_assert(policy_arr, attr, tlv_type) \
	do { \
		const NMTlvAttr *const _attr = (attr); \
		\
		nm_assert (_attr); \
		_nm_tlv_policy_assert ((policy_arr), _attr->att_type, (tlv_type)); \
	} while (0)

static inline const void *
_nm_tlv_attr_get (const NMTlvAttr *attr)
{
	nm_assert (attr);

	return &((const uint8_t *) attr)[sizeof (NMTlvAttr)];
}

int nm_tlv_msg_parse_full (const void *msg,
                           size_t msg_len,
                           const NMTlvPolicy *policy_arr,
                           uint16_t policy_len,
                           const NMTlvAttr **attrs,
                           NMTlvParseFlags flags);

#define nm_tlv_msg_parse(msg, msg_len, policy_arr, attrs, flags) \
	({ \
		NM_STATIC_ASSERT_EXPR (NM_N_ELEMENTS (policy_arr) > 0); \
		NM_STATIC_ASSERT_EXPR (NM_N_ELEMENTS (policy_arr) == NM_N_ELEMENTS (attrs)); \
		\
		nm_tlv_msg_parse_full ((msg), (msg_len), (policy_arr), NM_N_ELEMENTS (policy_arr), (attrs), (flags)); \
	})

int _nm_tlv_msg_append (void **msg_data,
                        size_t *msg_alloc,
                        size_t *msg_len,
                        NMTlvAttType att_type,
                        const void *new_data,
                        size_t new_len,
                        bool nul_terminate);

/*****************************************************************************/

/**
 * nm_tlv_msg_iter_init:
 * @msg_data: the entire message that should be iterated.
 * @msg_len: the length of @msg_data
 *
 * Note that the function performs no validation. You always
 * must initially validate the message with nm_tlv_msg_parse().
 *
 * Returns: the first attribute or %NULL if no such attribute
 *   exists.
 */
static inline const NMTlvAttr *
nm_tlv_msg_iter_init (const void *msg_data,
                      size_t msg_len)
{
	size_t len;

	if (msg_len < sizeof (NMTlvAttr))
		return NULL;

	len = nm_tlv_attr_len (msg_data);
	if (len > msg_len - sizeof (NMTlvAttr))
		return NULL;

	return msg_data;
}

/**
 * nm_tlv_msg_iter_next:
 * @msg_data: the entire message that should be iterated.
 * @msg_len: the length of @msg_data
 * @attr: the current attribute. The function will return
 *   a pointer to the following attribute, or %NULL if no
 *   such attribute exists.
 *
 * Note that the function performs no validation. You always
 * must initially validate the message with nm_tlv_msg_parse().
 *
 * Returns: the following attribute or %NULL if no such attribute
 *   exists.
 */
static inline const NMTlvAttr *
nm_tlv_msg_iter_next (const void *msg_data,
                      size_t msg_len,
                      const NMTlvAttr *attr)
{
	const uint8_t *const attr_p = (const uint8_t *) attr;
	size_t l;

	if (!attr)
		return NULL;

	nm_assert (attr_p >= ((const uint8_t *) msg_data));
	nm_assert (attr_p < &((const uint8_t *) msg_data)[msg_len]);

	l = attr_p - ((const uint8_t *) msg_data);

	nm_assert (l < msg_len);

	msg_len -= l;

	if (msg_len < sizeof (NMTlvAttr))
		return NULL;

	l = NM_TLV_ALIGN (sizeof (NMTlvAttr) + nm_tlv_attr_len (attr));

	if (msg_len < l)
		return NULL;

	return nm_tlv_msg_iter_init (&attr_p[l], msg_len - l);
}

#define nm_tlv_msg_foreach(attr, msg_data, msg_len) \
	for (attr = nm_tlv_msg_iter_init (msg_data, msg_len); \
	     attr; \
	     attr = nm_tlv_msg_iter_next (msg_data, msg_len, attr)) \

/*****************************************************************************/

int nm_tlv_msg_nest_open (void **msg_data,
                          size_t *msg_alloc,
                          size_t *msg_len,
                          NMTlvAttType att_type,
                          size_t *out_nest_start_offset);

static inline void
nm_tlv_msg_nest_reset (void **msg_data,
                       size_t *msg_len,
                       size_t nest_start_offset)
{
	nm_assert (msg_data);
	nm_assert (msg_len);
	nm_assert (*msg_len >= sizeof (NMTlvAttr));
	nm_assert (nest_start_offset <= *msg_len - sizeof (NMTlvAttr));

	*msg_len = nest_start_offset;
}

int nm_tlv_msg_nest_close (void **msg_data,
                           size_t *msg_len,
                           size_t nest_start_offset);

/*****************************************************************************/

int nm_tlv_msg_get_attrs (const void *msg_data,
                          size_t msg_len,
                          const NMTlvAttr *const*parsed_attrs,
                          uint16_t parsed_attr_len,
                          const NMTlvAttr ***out_attrs);

/*****************************************************************************/

static inline bool
nm_tlv_attr_get_bool (const NMTlvPolicy *policy_arr, const NMTlvAttr *attr)
{
	const uint8_t *p;

	_nm_tlv_attr_assert (policy_arr, attr, NM_TLV_TYPE_BOOL);
	nm_assert (nm_tlv_attr_len (attr) == sizeof (uint8_t));

	p = _nm_tlv_attr_get (attr);

	nm_assert (*p == 0 || *p == 1);

	return *p;
}

static inline int32_t
nm_tlv_attr_get_int32 (const NMTlvPolicy *policy_arr, const NMTlvAttr *attr)
{
	_nm_tlv_attr_assert (policy_arr, attr, NM_TLV_TYPE_INT32);
	nm_assert (nm_tlv_attr_len (attr) == sizeof (int32_t));

	return be32toh (*((int32_t *) _nm_tlv_attr_get (attr)));
}

static inline uint32_t
nm_tlv_attr_get_uint32 (const NMTlvPolicy *policy_arr, const NMTlvAttr *attr)
{
	_nm_tlv_attr_assert (policy_arr, attr, NM_TLV_TYPE_UINT32);
	nm_assert (nm_tlv_attr_len (attr) == sizeof (uint32_t));

	return be32toh (*((uint32_t *) _nm_tlv_attr_get (attr)));
}

static inline int64_t
nm_tlv_attr_get_int64 (const NMTlvPolicy *policy_arr, const NMTlvAttr *attr)
{
	_nm_tlv_attr_assert (policy_arr, attr, NM_TLV_TYPE_INT64);
	nm_assert (nm_tlv_attr_len (attr) == sizeof (int64_t));

	return unaligned_read_be64 (_nm_tlv_attr_get (attr));
}

static inline uint64_t
nm_tlv_attr_get_uint64 (const NMTlvPolicy *policy_arr, const NMTlvAttr *attr)
{
	_nm_tlv_attr_assert (policy_arr, attr, NM_TLV_TYPE_UINT64);
	nm_assert (nm_tlv_attr_len (attr) == sizeof (uint64_t));

	return unaligned_read_be64 (_nm_tlv_attr_get (attr));
}

static inline const char *
nm_tlv_attr_get_str (const NMTlvPolicy *policy_arr, const NMTlvAttr *attr, size_t *out_len)
{
	_nm_tlv_attr_assert (policy_arr, attr, NM_TLV_TYPE_STR);
	nm_assert (({
	     size_t l = nm_tlv_attr_len (attr);
	     const char *s = _nm_tlv_attr_get (attr);

	        l > 0
	     && s[l - 1u] == '\0'
	     && strlen (s) == l - 1u;
	}));

	NM_SET_OUT (out_len, nm_tlv_attr_len (attr) - 1u);
	return _nm_tlv_attr_get (attr);
}

static inline const uint8_t *
nm_tlv_attr_get_mem (const NMTlvPolicy *policy_arr, const NMTlvAttr *attr, size_t *out_len)
{
	_nm_tlv_attr_assert (policy_arr, attr, NM_TLV_TYPE_MEM);
	nm_assert (out_len);

	*out_len = nm_tlv_attr_len (attr);
	return _nm_tlv_attr_get (attr);
}

/*****************************************************************************/

static inline int
nm_tlv_msg_append_bool (void **msg_data,
                        size_t *msg_alloc,
                        size_t *msg_len,
                        const NMTlvPolicy *policy_arr,
                        NMTlvAttType att_type,
                        bool val)
{
	uint8_t v = (!!val);

	_nm_tlv_policy_assert (policy_arr, att_type, NM_TLV_TYPE_BOOL);
	return _nm_tlv_msg_append (msg_data, msg_alloc, msg_len, att_type, &v, sizeof (v), false);
}

static inline int
nm_tlv_msg_append_int32 (void **msg_data,
                         size_t *msg_alloc,
                         size_t *msg_len,
                         const NMTlvPolicy *policy_arr,
                         NMTlvAttType att_type,
                         int32_t val)
{
	_nm_tlv_policy_assert (policy_arr, att_type, NM_TLV_TYPE_INT32);
	val = htobe32 (val);
	return _nm_tlv_msg_append (msg_data, msg_alloc, msg_len, att_type, &val, sizeof (val), false);
}

static inline int
nm_tlv_msg_append_uint32 (void **msg_data,
                          size_t *msg_alloc,
                          size_t *msg_len,
                          const NMTlvPolicy *policy_arr,
                          NMTlvAttType att_type,
                          uint32_t val)
{
	_nm_tlv_policy_assert (policy_arr, att_type, NM_TLV_TYPE_UINT32);
	val = htobe32 (val);
	return _nm_tlv_msg_append (msg_data, msg_alloc, msg_len, att_type, &val, sizeof (val), false);
}

static inline int
nm_tlv_msg_append_int64 (void **msg_data,
                         size_t *msg_alloc,
                         size_t *msg_len,
                         const NMTlvPolicy *policy_arr,
                         NMTlvAttType att_type,
                         int64_t val)
{
	_nm_tlv_policy_assert (policy_arr, att_type, NM_TLV_TYPE_INT64);
	val = htobe64 (val);
	return _nm_tlv_msg_append (msg_data, msg_alloc, msg_len, att_type, &val, sizeof (val), false);
}

static inline int
nm_tlv_msg_append_uint64 (void **msg_data,
                          size_t *msg_alloc,
                          size_t *msg_len,
                          const NMTlvPolicy *policy_arr,
                          NMTlvAttType att_type,
                          uint64_t val)
{
	_nm_tlv_policy_assert (policy_arr, att_type, NM_TLV_TYPE_UINT64);
	val = htobe64 (val);
	return _nm_tlv_msg_append (msg_data, msg_alloc, msg_len, att_type, &val, sizeof (val), false);
}

static inline int
nm_tlv_msg_append_str (void **msg_data,
                       size_t *msg_alloc,
                       size_t *msg_len,
                       const NMTlvPolicy *policy_arr,
                       NMTlvAttType att_type,
                       const char *str,
                       ssize_t len)
{
	_nm_tlv_policy_assert (policy_arr, att_type, NM_TLV_TYPE_STR);
	if (len < 0) {
		nm_assert (str);
		len = strlen (str);
	} else {
		nm_assert (   len == 0
		           || (   str
		               && !memchr (str, '\0', len)));
	}
	return _nm_tlv_msg_append (msg_data, msg_alloc, msg_len, att_type, str, len, true);
}

static inline int
nm_tlv_msg_append_mem (void **msg_data,
                       size_t *msg_alloc,
                       size_t *msg_len,
                       const NMTlvPolicy *policy_arr,
                       NMTlvAttType att_type,
                       const char *str,
                       size_t len)
{
	_nm_tlv_policy_assert (policy_arr, att_type, NM_TLV_TYPE_MEM);
	return _nm_tlv_msg_append (msg_data, msg_alloc, msg_len, att_type, str, len, false);
}

/*****************************************************************************/

#endif /* __NM_TLV_H__ */
