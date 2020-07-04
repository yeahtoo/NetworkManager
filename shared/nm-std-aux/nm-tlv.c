// SPDX-License-Identifier: LGPL-2.1+

#include "nm-default.h"

#include "nm-tlv.h"

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "nm-std-utils.h"

/*****************************************************************************/

static void
_tlv_attr_set_len (NMTlvAttr *attr, size_t len)
{
	nm_assert (attr);
	nm_assert (len <= NM_TLV_ATTR_LEN_MAX);

	attr->len00 = ( len        & 0xFFu);
	attr->len08 = ((len >>  8) & 0xFFu);
	attr->len16 = ((len >> 16) & 0xFFu);
}

int
nm_tlv_msg_parse_full (const void *msg,
                       size_t msg_len,
                       const NMTlvPolicy *policy_arr,
                       uint16_t num,
                       const NMTlvAttr **attrs,
                       NMTlvParseFlags flags)
{
	int highest_attr = -1;

	NM_STATIC_ASSERT (sizeof (highest_attr) > sizeof (NMTlvAttType));

	/* The policy is indexed by att_type (NMTlvAttType) which is uint8_t.
	 * That means we can handle at most 256 attribute types per message type.
	 *
	 * For now, reserve type 255. If you ever hit this limit, consider splitting
	 * the message into nested messages or extending the API to allow large
	 * attribute numbers. */
	nm_assert (num < 255);

	if (NM_UNLIKELY (msg_len >= 0x800000u)) {
		/* for now, such large messages are reserved. That is because nm_tlv_attr_len()
		 * can at most return 0xFFFFFF. If you really need to handle such large messages,
		 * the protocol needs to be extended to support large TLV attributes. For example
		 * by reserving the highest bit of NMTlvAttr.len16 to indicate a large message.
		 *
		 * As of now, it's not possibly that anybody is actually using such large messages
		 * due to this check. So the protocol can be extended by interpreting len16
		 * differently.
		 */
		return -EINVAL;
	}

	memset (attrs, 0, sizeof (attrs[0]) * num);

	if (msg_len == 0)
		return highest_attr + 1;

	for (;;) {
		const NMTlvAttr *a;
		uint32_t a_len;
		size_t l;

		if (msg_len < sizeof (NMTlvAttr))
			return -EINVAL;

		a = msg;
		a_len = nm_tlv_attr_len (a);

		nm_assert (a_len <= 0xFFFFFFu);

		l = sizeof (NMTlvAttr) + ((size_t) a_len);
		if (msg_len < l)
			return -EINVAL;

		/* The code above currently ensures that the length must be shorter than
		 * 0x800000u. That means, the highest bit of len16 must always be unset.
		 * Assert for that, to not use it. We want to reserve this bit to extend
		 * the protocol (if ever necessary). */
		nm_assert (!(a->len16 & 0x80u));

		if (a->att_type >= num)
			goto handle_unknown;

		if (   attrs[a->att_type]
		    && !policy_arr[a->att_type].allow_repeated) {
			/* repeated attribute is not allowed. */
			return -EINVAL;
		}

		switch (policy_arr[a->att_type].type) {
		case NM_TLV_TYPE_BOOL: {
			const uint8_t *t;

			if (a_len != sizeof (uint8_t))
				return -EINVAL;
			t = _nm_tlv_attr_get (a);
			if (*t != 0 && *t != 1)
				return -EINVAL;
			break;
		}
		case NM_TLV_TYPE_INT32:  if (a_len != sizeof (int32_t )) return -EINVAL; break;
		case NM_TLV_TYPE_UINT32: if (a_len != sizeof (uint32_t)) return -EINVAL; break;
		case NM_TLV_TYPE_INT64:  if (a_len != sizeof (int64_t )) return -EINVAL; break;
		case NM_TLV_TYPE_UINT64: if (a_len != sizeof (uint64_t)) return -EINVAL; break;
		case NM_TLV_TYPE_STR: {
			const char *s;

			if (a_len < 1u)
				return -EINVAL;
			s = _nm_tlv_attr_get (a);
			if (memchr (s, '\0', a_len) != &s[a_len - 1u])
				return -EINVAL;
			break;
		}
		case NM_TLV_TYPE_MEM:
			break;
		case NM_TLV_TYPE_NONE:
			goto handle_unknown;
		default:
			nm_assert_not_reached ();
			goto handle_unknown;
		}

		if (highest_attr < (int) a->att_type)
			highest_attr = a->att_type;
		attrs[a->att_type] = a;
		goto handle_next;

handle_unknown:
		if ((flags & NM_TLV_PARSE_FLAGS_FAIL_UNKNOWN))
			return -ENOENT;
		goto handle_next;

handle_next:
		l = NM_TLV_ALIGN (l);
		if (msg_len <= l)
			return highest_attr + 1;

		msg = &((const uint8_t *) msg)[l];
		msg_len -= l;
	}
}

/*****************************************************************************/

int
_nm_tlv_msg_append (void **msg_data,
                    size_t *msg_alloc,
                    size_t *msg_len,
                    NMTlvAttType att_type,
                    const void *new_data,
                    size_t new_len,
                    bool nul_terminate)
{
	size_t new_alloc;
	size_t app_len;
	uint8_t *p;
	uint8_t *p0;
	uint8_t nul_ter = (!!nul_terminate);
	size_t new_len_with_nul;

	NM_STATIC_ASSERT_EXPR (sizeof (NMTlvAttr) % NM_TLV_ALIGNTO == 0);
	NM_STATIC_ASSERT_EXPR (_nm_alignof (NMTlvAttr) <= NM_TLV_ALIGNTO);
	NM_STATIC_ASSERT_EXPR (sizeof (NMTlvAttr) == NM_TLV_ALIGNTO);

	nm_assert (msg_data);
	nm_assert (msg_alloc);
	nm_assert (msg_len);
	nm_assert (*msg_len <= *msg_alloc);
	nm_assert ((*msg_len % NM_TLV_ALIGNTO) == 0);
	nm_assert (*msg_alloc == 0 || *msg_data);

	/* Currently nm_tlv_attr_len() is limited to 0xFFFFFF (NM_TLV_ATTR_LEN_MAX).
	 * But we limit it artificially further to 0x800000u, because we want to reserve
	 * the highest bit of NMTlvAttr.len16 for possibly extending the API. That
	 * means, one TLV attribute is limited to 0x800000u.
	 *
	 * Note that we concatenate TLV attributes to makeup the full message. Since
	 * a message itself should fit into a NM_TLV_TYPE_MEM message, that means
	 * not only each attribute is limited, but the entire message. We don't check
	 * for that however. If you try to create a longer message, it will succeed here,
	 * but parsing it will fail.  */
	if (NM_UNLIKELY (new_len > 0x800000u - nul_ter))
		return -ENOBUFS;

	new_len_with_nul = new_len + nul_ter;

	app_len = NM_TLV_ALIGN (sizeof (NMTlvAttr) + new_len_with_nul);

	new_alloc = *msg_len + app_len;

	if (NM_UNLIKELY (new_alloc > *msg_alloc)) {
		void *new_msg;

		new_alloc = nm_utils_get_next_realloc_size (true, NM_MAX (new_alloc, 100));

		new_msg = realloc (*msg_data, new_alloc);
		if (!new_msg)
			return -ENOMEM;
		*msg_data = new_msg;
		*msg_alloc = new_alloc;
	}

	p0 = &((uint8_t *) *msg_data)[*msg_len];
	p = p0;
	((NMTlvAttr *) p)->att_type = att_type;
	_tlv_attr_set_len ((NMTlvAttr *) p, new_len_with_nul);
	p += sizeof (NMTlvAttr);
	if (new_len > 0) {
		memcpy (p, new_data, new_len);
		p += new_len;
	}
	if (nul_terminate) {
		p[0] = '\0';
		p++;
	}

	p0 += app_len;

	for (; p < p0; p++) {
		/* we also need to clear the alignment hole, if only to make valgrind happy. */
		p[0] = '\0';
	}

	*msg_len += app_len;
	return 0;
}

/*****************************************************************************/

int
nm_tlv_msg_nest_open (void **msg_data,
                      size_t *msg_alloc,
                      size_t *msg_len,
                      NMTlvAttType att_type,
                      size_t *out_nest_start_offset)
{
	size_t nest_start_offset;
	int r;

	nm_assert (msg_len);
	nm_assert (out_nest_start_offset);

	nest_start_offset = *msg_len;

	r = _nm_tlv_msg_append (msg_data, msg_alloc, msg_len, att_type, NULL, 0, false);
	if (r < 0)
		return r;

	*out_nest_start_offset = nest_start_offset;
	return 0;
}

int
nm_tlv_msg_nest_close (void **msg_data,
                       size_t *msg_len,
                       size_t nest_start_offset)
{
	NMTlvAttr *attr;
	size_t l;

	nm_assert (msg_data);
	nm_assert (msg_len);
	nm_assert (*msg_len >= sizeof (NMTlvAttr));
	nm_assert (nest_start_offset <= *msg_len - sizeof (NMTlvAttr));
	nm_assert (NM_TLV_ALIGN (*msg_len) == *msg_len);

	attr = (NMTlvAttr *) (&(((const uint8_t *) (*msg_data))[nest_start_offset]));

	l = *msg_len - nest_start_offset - sizeof (NMTlvAttr);

	if (NM_UNLIKELY (l > 0x800000u))
		return -ENOBUFS;

	_tlv_attr_set_len (attr, l);
	return 0;
}

/*****************************************************************************/

/**
 * nm_tlv_msg_get_attrs:
 * @msg_data: the message to parse.
 * @msg_len: the length of @msg_data.
 * @parsed_attrs: (allow-none): If given, the function will
 *   only return attributes that are set in @parsed_attrs.
 * @parsed_attr_len: the length of @parsed_attrs array or zero if
 *   no @parsed_attrs are given.
 * @out_attrs: (allow-none) (transfer container) (out): if given,
 *   returns a NULL terminated array of parsed attributes. The return
 *   values is also the length of the attributes. If @parsed_attrs
 *   is given, then the result only contains attributes that were
 *   previously parsed.
 *
 * The purpose of this function is to get all attributes that may
 * be repeated (NMTlvPolicy.allow_repeated). The order is as in the
 * message. If you only care about attributes that are not allowed
 * to repeat, and if you don't care about the order of attributes,
 * and only care about the last occurrence of an attribute,
 * then nm_tlv_msg_parse() is better.
 *
 * You must only call this function on data after you successfully
 * checked the policy with nm_tlv_msg_parse()! The function will not
 * validate the attributes again. From there you also get @parsed_attrs
 * to filter only for certain attributes.
 * The reason why the function does not perform any validation against
 * a policy is because it is extra effort to validate for duplicate
 * attributes (allow_repeated). So, a user should always first call
 * nm_tlv_msg_parse() to validate the message.
 *
 * Consider using nm_tlv_msg_foreach() instead to iterate all attributes.
 *
 * Returns: a negative error code, or 0 if there are not matching attributes,
 *   or the number of returned attributes (the length of @out_attrs);
 */
int
nm_tlv_msg_get_attrs (const void *msg_data,
                      size_t msg_len,
                      const NMTlvAttr *const*parsed_attrs,
                      uint16_t parsed_attr_len,
                      const NMTlvAttr ***out_attrs)
{
	const NMTlvAttr **attrs;
	const NMTlvAttr *attr;
	size_t i, n;

	nm_assert (msg_len == 0 || msg_data);
	nm_assert (parsed_attr_len == 0 || parsed_attrs);
	nm_assert (out_attrs);

	*out_attrs = NULL;

	n = 0;
	nm_tlv_msg_foreach (attr, msg_data, msg_len) {
		if (   parsed_attr_len == 0
		    || (   attr->att_type < parsed_attr_len
		        && parsed_attrs[attr->att_type]))
			n++;
	}

	if (n == 0)
		return 0;

	if (NM_UNLIKELY (n >= (size_t) INT_MAX)) {
		/** this cannot really happen, because you were supposed to call nm_tlv_msg_parse_full(),
		 * which fails if the message is too large (so, there cannot be too many attributes. */
		return -ERANGE;
	}

	if (!out_attrs)
		return n;

	attrs = malloc (sizeof (NMTlvAttr *) * (n + 1));
	if (!attrs)
		return -ENOMEM;

	i = 0;
	nm_tlv_msg_foreach (attr, msg_data, msg_len) {
		if (   parsed_attr_len == 0
		    || (   attr->att_type < parsed_attr_len
		        && parsed_attrs[attr->att_type])) {
			nm_assert (i < n);
			attrs[i++] = attr;
		}
	}
	attrs[n] = NULL;

	*out_attrs = attrs;
	return n;
}
