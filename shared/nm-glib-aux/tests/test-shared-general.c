// SPDX-License-Identifier: LGPL-2.1+
/*
 * Copyright (C) 2018 Red Hat, Inc.
 */

#define NM_TEST_UTILS_NO_LIBNM 1

#include "nm-default.h"

#include "nm-std-aux/unaligned.h"
#include "nm-std-aux/nm-tlv.h"
#include "nm-glib-aux/nm-random-utils.h"
#include "nm-glib-aux/nm-str-buf.h"
#include "nm-glib-aux/nm-time-utils.h"
#include "nm-glib-aux/nm-ref-string.h"

#include "nm-utils/nm-test-utils.h"

/*****************************************************************************/

static void
test_gpid (void)
{
	const int *int_ptr;
	GPid pid = 42;

	/* We redefine G_PID_FORMAT, because it's only available since glib 2.53.5.
	 *
	 * Also, this is the format for GPid, which for glib is always a typedef
	 * for "int". Add a check for that here.
	 *
	 * G_PID_FORMAT is not about pid_t, which might be a smaller int, and which we would
	 * check with SIZEOF_PID_T. */
	G_STATIC_ASSERT (sizeof (GPid) == sizeof (int));

	g_assert_cmpstr (""G_PID_FORMAT, ==, "i");

	/* check that it's really "int". We will get a compiler warning, if that's not
	 * the case. */
	int_ptr = &pid;
	g_assert_cmpint (*int_ptr, ==, 42);
}

/*****************************************************************************/

static void
test_monotonic_timestamp (void)
{
	g_assert (nm_utils_get_monotonic_timestamp_sec () > 0);
}

/*****************************************************************************/

static void
test_nmhash (void)
{
	int rnd;

	nm_utils_random_bytes (&rnd, sizeof (rnd));

	g_assert (nm_hash_val (555, 4) != 0);
}

/*****************************************************************************/

static const char *
_make_strv_foo (void)
{
	return "foo";
}

static const char *const*const _tst_make_strv_1 = NM_MAKE_STRV ("1", "2");

static void
test_make_strv (void)
{
	const char *const*v1a = NM_MAKE_STRV ("a");
	const char *const*v1b = NM_MAKE_STRV ("a", );
	const char *const*v2a = NM_MAKE_STRV ("a", "b");
	const char *const*v2b = NM_MAKE_STRV ("a", "b", );
	const char *const v3[] = { "a", "b", };
	const char *const*v4b = NM_MAKE_STRV ("a", _make_strv_foo (), );

	g_assert (NM_PTRARRAY_LEN (v1a) == 1);
	g_assert (NM_PTRARRAY_LEN (v1b) == 1);
	g_assert (NM_PTRARRAY_LEN (v2a) == 2);
	g_assert (NM_PTRARRAY_LEN (v2b) == 2);

	g_assert (NM_PTRARRAY_LEN (_tst_make_strv_1) == 2);
	g_assert_cmpstr (_tst_make_strv_1[0], ==, "1");
	g_assert_cmpstr (_tst_make_strv_1[1], ==, "2");
	/* writing the static read-only variable leads to crash .*/
	//((char **) _tst_make_strv_1)[0] = NULL;
	//((char **) _tst_make_strv_1)[2] = "c";

	G_STATIC_ASSERT_EXPR (G_N_ELEMENTS (v3) == 2);

	g_assert (NM_PTRARRAY_LEN (v4b) == 2);

	G_STATIC_ASSERT_EXPR (G_N_ELEMENTS (NM_MAKE_STRV ("a", "b"  )) == 3);
	G_STATIC_ASSERT_EXPR (G_N_ELEMENTS (NM_MAKE_STRV ("a", "b", )) == 3);

	nm_strquote_a (300, "");
}

/*****************************************************************************/

typedef enum {
	TEST_NM_STRDUP_ENUM_m1 = -1,
	TEST_NM_STRDUP_ENUM_3  = 3,
} TestNMStrdupIntEnum;

static void
test_nm_strdup_int (void)
{
#define _NM_STRDUP_INT_TEST(num, str) \
	G_STMT_START { \
		gs_free char *_s1 = NULL; \
		\
		_s1 = nm_strdup_int ((num)); \
		\
		g_assert (_s1); \
		g_assert_cmpstr (_s1, ==, str); \
	} G_STMT_END

#define _NM_STRDUP_INT_TEST_TYPED(type, num) \
	G_STMT_START { \
		type _num = ((type) num); \
		\
		_NM_STRDUP_INT_TEST (_num, G_STRINGIFY (num)); \
	} G_STMT_END

	_NM_STRDUP_INT_TEST_TYPED (char, 0);
	_NM_STRDUP_INT_TEST_TYPED (char, 1);
	_NM_STRDUP_INT_TEST_TYPED (guint8, 0);
	_NM_STRDUP_INT_TEST_TYPED (gint8, 25);
	_NM_STRDUP_INT_TEST_TYPED (char, 47);
	_NM_STRDUP_INT_TEST_TYPED (short, 47);
	_NM_STRDUP_INT_TEST_TYPED (int, 47);
	_NM_STRDUP_INT_TEST_TYPED (long, 47);
	_NM_STRDUP_INT_TEST_TYPED (unsigned char, 47);
	_NM_STRDUP_INT_TEST_TYPED (unsigned short, 47);
	_NM_STRDUP_INT_TEST_TYPED (unsigned, 47);
	_NM_STRDUP_INT_TEST_TYPED (unsigned long, 47);
	_NM_STRDUP_INT_TEST_TYPED (gint64, 9223372036854775807);
	_NM_STRDUP_INT_TEST_TYPED (gint64, -9223372036854775807);
	_NM_STRDUP_INT_TEST_TYPED (guint64, 0);
	_NM_STRDUP_INT_TEST_TYPED (guint64, 9223372036854775807);

	_NM_STRDUP_INT_TEST (TEST_NM_STRDUP_ENUM_m1, "-1");
	_NM_STRDUP_INT_TEST (TEST_NM_STRDUP_ENUM_3,  "3");
}

/*****************************************************************************/

static void
test_nm_strndup_a (void)
{
	int run;

	for (run = 0; run < 20; run++) {
		gs_free char *input = NULL;
		char ch;
		gsize i, l;

		input = g_strnfill (nmtst_get_rand_uint32 () % 20, 'x');

		for (i = 0; input[i]; i++) {
			while ((ch = ((char) nmtst_get_rand_uint32 ())) == '\0') {
				/* repeat. */
			}
			input[i] = ch;
		}

		{
			gs_free char *dup_free = NULL;
			const char *dup;

			l = strlen (input) + 1;
			dup = nm_strndup_a (10, input, l - 1, &dup_free);
			g_assert_cmpstr (dup, ==, input);
			if (strlen (dup) < 10)
				g_assert (!dup_free);
			else
				g_assert (dup == dup_free);
		}

		{
			gs_free char *dup_free = NULL;
			const char *dup;

			l = nmtst_get_rand_uint32 () % 23;
			dup = nm_strndup_a (10, input, l, &dup_free);
			g_assert (strncmp (dup, input, l) == 0);
			g_assert (strlen (dup) <= l);
			if (l < 10)
				g_assert (!dup_free);
			else
				g_assert (dup == dup_free);
			if (strlen (input) < l)
				g_assert (nm_utils_memeqzero (&dup[strlen (input)], l - strlen (input)));
		}
	}
}

/*****************************************************************************/

static void
test_nm_ip4_addr_is_localhost (void)
{
	g_assert ( nm_ip4_addr_is_localhost (nmtst_inet4_from_string ("127.0.0.0")));
	g_assert ( nm_ip4_addr_is_localhost (nmtst_inet4_from_string ("127.0.0.1")));
	g_assert ( nm_ip4_addr_is_localhost (nmtst_inet4_from_string ("127.5.0.1")));
	g_assert (!nm_ip4_addr_is_localhost (nmtst_inet4_from_string ("126.5.0.1")));
	g_assert (!nm_ip4_addr_is_localhost (nmtst_inet4_from_string ("128.5.0.1")));
	g_assert (!nm_ip4_addr_is_localhost (nmtst_inet4_from_string ("129.5.0.1")));
}

/*****************************************************************************/

static void
test_unaligned (void)
{
	int shift;

	for (shift = 0; shift <= 32; shift++) {
		guint8 buf[100] = { };
		guint8 val = 0;

		while (val == 0)
			val = nmtst_get_rand_uint32 () % 256;

		buf[shift] = val;

		g_assert_cmpint (unaligned_read_le64 (&buf[shift]), ==, (guint64) val);
		g_assert_cmpint (unaligned_read_be64 (&buf[shift]), ==, ((guint64) val) << 56);
		g_assert_cmpint (unaligned_read_ne64 (&buf[shift]), !=, 0);

		g_assert_cmpint (unaligned_read_le32 (&buf[shift]), ==, (guint32) val);
		g_assert_cmpint (unaligned_read_be32 (&buf[shift]), ==, ((guint32) val) << 24);
		g_assert_cmpint (unaligned_read_ne32 (&buf[shift]), !=, 0);

		g_assert_cmpint (unaligned_read_le16 (&buf[shift]), ==, (guint16) val);
		g_assert_cmpint (unaligned_read_be16 (&buf[shift]), ==, ((guint16) val) << 8);
		g_assert_cmpint (unaligned_read_ne16 (&buf[shift]), !=, 0);
	}
}

/*****************************************************************************/

static void
_strv_cmp_fuzz_input (const char *const*in,
                      gssize l,
                      const char ***out_strv_free_shallow,
                      char ***out_strv_free_deep,
                      const char *const* *out_s1,
                      const char *const* *out_s2)
{
	const char **strv;
	gsize i;

	/* Fuzz the input argument. It will return two output arrays that are semantically
	 * equal the input. */

	if (nmtst_get_rand_bool ()) {
		char **ss;

		if (l < 0)
			ss = g_strdupv ((char **) in);
		else if (l == 0) {
			ss =   nmtst_get_rand_bool ()
			     ? NULL
			     : g_new0 (char *, 1);
		} else {
			ss = nm_memdup (in, sizeof (const char *) * l);
			for (i = 0; i < (gsize) l; i++)
				ss[i] = g_strdup (ss[i]);
		}
		strv = (const char **) ss;
		*out_strv_free_deep = ss;
	} else {
		if (l < 0) {
			strv =   in
			       ? nm_memdup (in, sizeof (const char *) * (NM_PTRARRAY_LEN (in) + 1))
			       : NULL;
		} else if (l == 0) {
			strv =   nmtst_get_rand_bool ()
			       ? NULL
			       : g_new0 (const char *, 1);
		} else
			strv = nm_memdup (in, sizeof (const char *) * l);
		*out_strv_free_shallow = strv;
	}

	*out_s1 = in;
	*out_s2 = strv;

	if (nmtst_get_rand_bool ()) {
		/* randomly swap the original and the clone. That means, out_s1 is either
		 * the input argument (as-is) or the sementically equal clone. */
		NM_SWAP (*out_s1, *out_s2);
	}
	if (nmtst_get_rand_bool ()) {
		/* randomly make s1 and s2 the same. This is for testing that
		 * comparing two identical pointers yields the same result. */
		*out_s2 = *out_s1;
	}
}

static void
_strv_cmp_free_deep (char **strv,
                     gssize len)
{
	gssize i;

	if (strv) {
		if (len < 0)
			g_strfreev (strv);
		else {
			for (i = 0; i < len; i++)
				g_free (strv[i]);
			g_free (strv);
		}
	}
}

static void
test_strv_cmp (void)
{
	const char *const strv0[1] = { };
	const char *const strv1[2] = { "", };

#define _STRV_CMP(a1, l1, a2, l2, equal) \
	G_STMT_START { \
		gssize _l1 = (l1); \
		gssize _l2 = (l2); \
		const char *const*_a1; \
		const char *const*_a2; \
		const char *const*_a1x; \
		const char *const*_a2x; \
		char **_a1_free_deep = NULL; \
		char **_a2_free_deep = NULL; \
		gs_free const char **_a1_free_shallow = NULL; \
		gs_free const char **_a2_free_shallow = NULL; \
		int _c1, _c2; \
		\
		_strv_cmp_fuzz_input ((a1), _l1, &_a1_free_shallow, &_a1_free_deep, &_a1, &_a1x); \
		_strv_cmp_fuzz_input ((a2), _l2, &_a2_free_shallow, &_a2_free_deep, &_a2, &_a2x); \
		\
		_c1 = _nm_utils_strv_cmp_n (_a1, _l1, _a2, _l2); \
		_c2 = _nm_utils_strv_cmp_n (_a2, _l2, _a1, _l1); \
		if (equal) { \
			g_assert_cmpint (_c1, ==, 0); \
			g_assert_cmpint (_c2, ==, 0); \
		} else { \
			g_assert_cmpint (_c1, ==, -1); \
			g_assert_cmpint (_c2, ==, 1); \
		} \
		\
		/* Compare with self. _strv_cmp_fuzz_input() randomly swapped the arguments (_a1 and _a1x).
		 * Either way, the arrays must compare equal to their semantically equal alternative. */ \
		g_assert_cmpint (_nm_utils_strv_cmp_n (_a1, _l1, _a1x, _l1), ==, 0); \
		g_assert_cmpint (_nm_utils_strv_cmp_n (_a2, _l2, _a2x, _l2), ==, 0); \
		\
		_strv_cmp_free_deep (_a1_free_deep, _l1); \
		_strv_cmp_free_deep (_a2_free_deep, _l2); \
	} G_STMT_END

	_STRV_CMP (NULL,  -1, NULL,  -1, TRUE);

	_STRV_CMP (NULL,  -1, NULL,   0, FALSE);
	_STRV_CMP (NULL,  -1, strv0,  0, FALSE);
	_STRV_CMP (NULL,  -1, strv0, -1, FALSE);

	_STRV_CMP (NULL,   0, NULL,   0, TRUE);
	_STRV_CMP (NULL,   0, strv0,  0, TRUE);
	_STRV_CMP (NULL,   0, strv0, -1, TRUE);
	_STRV_CMP (strv0,  0, strv0,  0, TRUE);
	_STRV_CMP (strv0,  0, strv0, -1, TRUE);
	_STRV_CMP (strv0, -1, strv0, -1, TRUE);

	_STRV_CMP (NULL,   0, strv1, -1, FALSE);
	_STRV_CMP (NULL,   0, strv1,  1, FALSE);
	_STRV_CMP (strv0,  0, strv1, -1, FALSE);
	_STRV_CMP (strv0,  0, strv1,  1, FALSE);
	_STRV_CMP (strv0, -1, strv1, -1, FALSE);
	_STRV_CMP (strv0, -1, strv1,  1, FALSE);

	_STRV_CMP (strv1, -1, strv1,  1, TRUE);
	_STRV_CMP (strv1,  1, strv1,  1, TRUE);
}

/*****************************************************************************/

static void
_do_strstrip_avoid_copy (const char *str)
{
	gs_free char *str1 = g_strdup (str);
	gs_free char *str2 = g_strdup (str);
	gs_free char *str3 = NULL;
	gs_free char *str4 = NULL;
	const char *s3;
	const char *s4;

	if (str1)
		g_strstrip (str1);

	nm_strstrip (str2);

	g_assert_cmpstr (str1, ==, str2);

	s3 = nm_strstrip_avoid_copy (str, &str3);
	g_assert_cmpstr (str1, ==, s3);

	s4 = nm_strstrip_avoid_copy_a (10, str, &str4);
	g_assert_cmpstr (str1, ==, s4);
	g_assert (!str == !s4);
	g_assert (!s4 || strlen (s4) <= strlen (str));
	if (s4 && s4 == &str[strlen (str) - strlen (s4)]) {
		g_assert (!str4);
		g_assert (s3 == s4);
	} else if (s4 && strlen (s4) >= 10) {
		g_assert (str4);
		g_assert (s4 == str4);
	} else
		g_assert (!str4);

	if (!nm_streq0 (str1, str))
		_do_strstrip_avoid_copy (str1);
}

static void
test_strstrip_avoid_copy (void)
{
	_do_strstrip_avoid_copy (NULL);
	_do_strstrip_avoid_copy ("");
	_do_strstrip_avoid_copy (" ");
	_do_strstrip_avoid_copy (" a ");
	_do_strstrip_avoid_copy (" 012345678 ");
	_do_strstrip_avoid_copy (" 0123456789 ");
	_do_strstrip_avoid_copy (" 01234567890 ");
	_do_strstrip_avoid_copy (" 012345678901 ");
}

/*****************************************************************************/

static void
test_nm_utils_bin2hexstr (void)
{
	int n_run;

	for (n_run = 0; n_run < 100; n_run++) {
		guint8 buf[100];
		guint8 buf2[G_N_ELEMENTS (buf) + 1];
		gsize len = nmtst_get_rand_uint32 () % (G_N_ELEMENTS (buf) + 1);
		char strbuf1[G_N_ELEMENTS (buf) * 3];
		gboolean allocate = nmtst_get_rand_bool ();
		char delimiter = nmtst_get_rand_bool () ? ':' : '\0';
		gboolean upper_case = nmtst_get_rand_bool ();
		gsize expected_strlen;
		char *str_hex;
		gsize required_len;
		gboolean outlen_set;
		gsize outlen;
		guint8 *bin2;

		nmtst_rand_buf (NULL, buf, len);

		if (len == 0)
			expected_strlen = 0;
		else if (delimiter != '\0')
			expected_strlen = (len * 3u) - 1;
		else
			expected_strlen = len * 2u;

		g_assert_cmpint (expected_strlen, <, G_N_ELEMENTS (strbuf1));

		str_hex = nm_utils_bin2hexstr_full (buf, len, delimiter, upper_case, !allocate ? strbuf1 : NULL);

		g_assert (str_hex);
		if (!allocate)
			g_assert (str_hex == strbuf1);
		g_assert_cmpint (strlen (str_hex), ==, expected_strlen);

		g_assert (NM_STRCHAR_ALL (str_hex, ch,    (ch >= '0' && ch <= '9')
		                                       || ch == delimiter
		                                       || (  upper_case
		                                           ? (ch >= 'A' && ch <= 'F')
		                                           : (ch >= 'a' && ch <= 'f'))));

		required_len = nmtst_get_rand_bool () ? len : 0u;

		outlen_set = required_len == 0 || nmtst_get_rand_bool ();

		memset (buf2, 0, sizeof (buf2));

		bin2 = nm_utils_hexstr2bin_full (str_hex,
		                                 nmtst_get_rand_bool (),
		                                 delimiter != '\0' && nmtst_get_rand_bool (),
		                                   delimiter != '\0'
		                                 ? nmtst_rand_select ((const char *) ":", ":-")
		                                 : nmtst_rand_select ((const char *) ":", ":-", "", NULL),
		                                 required_len,
		                                 buf2,
		                                 len,
		                                 outlen_set ? &outlen : NULL);
		if (len > 0) {
			g_assert (bin2);
			g_assert (bin2 == buf2);
		} else
			g_assert (!bin2);

		if (outlen_set)
			g_assert_cmpint (outlen, ==, len);

		g_assert_cmpmem (buf, len, buf2, len);

		g_assert (buf2[len] == '\0');

		if (allocate)
			g_free (str_hex);
	}
}

/*****************************************************************************/

static void
test_nm_ref_string (void)
{
	nm_auto_ref_string NMRefString *s1 = NULL;
	NMRefString *s2;

	s1 = nm_ref_string_new ("hallo");
	g_assert (s1);
	g_assert_cmpstr (s1->str, ==, "hallo");
	g_assert_cmpint (s1->len, ==, strlen ("hallo"));

	s2 = nm_ref_string_new ("hallo");
	g_assert (s2 == s1);
	nm_ref_string_unref (s2);

	s2 = nm_ref_string_new (NULL);
	g_assert (!s2);
	nm_ref_string_unref (s2);

#define STR_WITH_NUL "hallo\0test\0"
	s2 = nm_ref_string_new_len (STR_WITH_NUL, NM_STRLEN (STR_WITH_NUL));
	g_assert (s2);
	g_assert_cmpstr (s2->str, ==, "hallo");
	g_assert_cmpint (s2->len, ==, NM_STRLEN (STR_WITH_NUL));
	g_assert_cmpint (s2->len, >, strlen (s2->str));
	g_assert_cmpmem (s2->str, s2->len, STR_WITH_NUL, NM_STRLEN (STR_WITH_NUL));
	g_assert (s2->str[s2->len] == '\0');
	nm_ref_string_unref (s2);
}

/*****************************************************************************/

static
NM_UTILS_STRING_TABLE_LOOKUP_DEFINE (
	_do_string_table_lookup,
	int,
	{ ; },
	{ return -1; },
	{ "0", 0 },
	{ "1", 1 },
	{ "2", 2 },
	{ "3", 3 },
)

static void
test_string_table_lookup (void)
{
	const char *const args[] = { NULL, "0", "1", "2", "3", "x", };
	int i;

	for (i = 0; i < G_N_ELEMENTS (args); i++) {
		const char *needle = args[i];
		const int val2 = _nm_utils_ascii_str_to_int64 (needle, 10, 0, 100, -1);
		int val;

		val = _do_string_table_lookup (needle);
		g_assert_cmpint (val, ==, val2);
	}
}

/*****************************************************************************/

static void
test_nm_utils_get_next_realloc_size (void)
{
	static const struct {
		gsize requested;
		gsize reserved_true;
		gsize reserved_false;
	} test_data[] = {
		{      0,     8,     8 },
		{      1,     8,     8 },
		{      8,     8,     8 },
		{      9,    16,    16 },
		{     16,    16,    16 },
		{     17,    32,    32 },
		{     32,    32,    32 },
		{     33,    40,    40 },
		{     40,    40,    40 },
		{     41,   104,   104 },
		{    104,   104,   104 },
		{    105,   232,   232 },
		{    232,   232,   232 },
		{    233,   488,   488 },
		{    488,   488,   488 },
		{    489,  1000,  1000 },
		{   1000,  1000,  1000 },
		{   1001,  2024,  2024 },
		{   2024,  2024,  2024 },
		{   2025,  4072,  4072 },
		{   4072,  4072,  4072 },
		{   4073,  8168,  8168 },
		{   8168,  8168,  8168 },
		{   8169, 12264, 16360 },
		{  12263, 12264, 16360 },
		{  12264, 12264, 16360 },
		{  12265, 16360, 16360 },
		{  16360, 16360, 16360 },
		{  16361, 20456, 32744 },
		{  20456, 20456, 32744 },
		{  20457, 24552, 32744 },
		{  24552, 24552, 32744 },
		{  24553, 28648, 32744 },
		{  28648, 28648, 32744 },
		{  28649, 32744, 32744 },
		{  32744, 32744, 32744 },
		{  32745, 36840, 65512 },
		{  36840, 36840, 65512 },
		{  G_MAXSIZE - 0x1000u,  G_MAXSIZE, G_MAXSIZE },
		{  G_MAXSIZE - 25u,      G_MAXSIZE, G_MAXSIZE },
		{  G_MAXSIZE - 24u,      G_MAXSIZE, G_MAXSIZE },
		{  G_MAXSIZE - 1u,       G_MAXSIZE, G_MAXSIZE },
		{  G_MAXSIZE,            G_MAXSIZE, G_MAXSIZE },
		{  NM_UTILS_GET_NEXT_REALLOC_SIZE_32,   NM_UTILS_GET_NEXT_REALLOC_SIZE_32,   NM_UTILS_GET_NEXT_REALLOC_SIZE_32 },
		{  NM_UTILS_GET_NEXT_REALLOC_SIZE_40,   NM_UTILS_GET_NEXT_REALLOC_SIZE_40,   NM_UTILS_GET_NEXT_REALLOC_SIZE_40 },
		{  NM_UTILS_GET_NEXT_REALLOC_SIZE_104,  NM_UTILS_GET_NEXT_REALLOC_SIZE_104,  NM_UTILS_GET_NEXT_REALLOC_SIZE_104 },
		{  NM_UTILS_GET_NEXT_REALLOC_SIZE_1000, NM_UTILS_GET_NEXT_REALLOC_SIZE_1000, NM_UTILS_GET_NEXT_REALLOC_SIZE_1000 },
	};
	guint i;

	G_STATIC_ASSERT_EXPR (NM_UTILS_GET_NEXT_REALLOC_SIZE_104  == 104u);
	G_STATIC_ASSERT_EXPR (NM_UTILS_GET_NEXT_REALLOC_SIZE_1000 == 1000u);

	for (i = 0; i < G_N_ELEMENTS (test_data) + 5000u; i++) {
		gsize requested0;

		if (i < G_N_ELEMENTS (test_data))
			requested0 = test_data[i].requested;
		else {
			/* find some interesting random values for testing. */
			switch (nmtst_get_rand_uint32 () % 5) {
			case 0:
				requested0 = nmtst_get_rand_size ();
				break;
			case 1:
				/* values close to G_MAXSIZE. */
				requested0 = G_MAXSIZE - (nmtst_get_rand_uint32 () % 12000u);
				break;
			case 2:
				/* values around G_MAXSIZE/2. */
				requested0 = (G_MAXSIZE / 2u) + 6000u - (nmtst_get_rand_uint32 () % 12000u);
				break;
			case 3:
				/* values around powers of 2. */
				requested0 = (((gsize) 1) << (nmtst_get_rand_uint32 () % (sizeof (gsize) * 8u))) + 6000u - (nmtst_get_rand_uint32 () % 12000u);
				break;
			case 4:
				/* values around 4k borders. */
				requested0 = (nmtst_get_rand_size () & ~((gsize) 0xFFFu)) + 30u - (nmtst_get_rand_uint32 () % 60u);
				break;
			default: g_assert_not_reached ();
			}
		}

		{
			const gsize requested = requested0;
			const gsize reserved_true = nm_utils_get_next_realloc_size (TRUE, requested);
			const gsize reserved_false = nm_utils_get_next_realloc_size (FALSE, requested);

			g_assert_cmpuint (reserved_true, >, 0);
			g_assert_cmpuint (reserved_false, >, 0);
			g_assert_cmpuint (reserved_true, >=, requested);
			g_assert_cmpuint (reserved_false, >=, requested);
			g_assert_cmpuint (reserved_false, >=, reserved_true);

			if (i < G_N_ELEMENTS (test_data)) {
				g_assert_cmpuint (reserved_true, ==, test_data[i].reserved_true);
				g_assert_cmpuint (reserved_false, ==, test_data[i].reserved_false);
			}

			/* reserved_false is generally the next power of two - 24. */
			if (reserved_false == G_MAXSIZE)
				g_assert_cmpuint (requested, >, G_MAXSIZE / 2u - 24u);
			else {
				g_assert_cmpuint (reserved_false, <=, G_MAXSIZE - 24u);
				if (reserved_false >= 40) {
					const gsize _pow2 = reserved_false + 24u;

					/* reserved_false must always be a power of two minus 24. */
					g_assert_cmpuint (_pow2, >=, 64u);
					g_assert_cmpuint (_pow2, >, requested);
					g_assert (nm_utils_is_power_of_two (_pow2));

					/* but _pow2/2 must also be smaller than what we requested. */
					g_assert_cmpuint (_pow2 / 2u - 24u, <, requested);
				} else {
					/* smaller values are hard-coded. */
				}
			}

			/* reserved_true is generally the next 4k border - 24. */
			if (reserved_true == G_MAXSIZE)
				g_assert_cmpuint (requested, >, G_MAXSIZE - 0x1000u - 24u);
			else {
				g_assert_cmpuint (reserved_true, <=, G_MAXSIZE - 24u);
				if (reserved_true > 8168u) {
					const gsize page_border = reserved_true + 24u;

					/* reserved_true must always be aligned to 4k (minus 24). */
					g_assert_cmpuint (page_border % 0x1000u, ==, 0);
					if (requested > 0x1000u - 24u) {
						/* page_border not be more than 4k above requested. */
						g_assert_cmpuint (page_border, >=, 0x1000u - 24u);
						g_assert_cmpuint (page_border - 0x1000u - 24u, <, requested);
					}
				} else {
					/* for smaller sizes, reserved_true and reserved_false are the same. */
					g_assert_cmpuint (reserved_true, ==, reserved_false);
				}
			}

		}
	}
}

/*****************************************************************************/

static void
test_nm_str_buf (void)
{
	guint i_run;

	for (i_run = 0; TRUE; i_run++) {
		nm_auto_str_buf NMStrBuf strbuf = { };
		nm_auto_free_gstring GString *gstr = NULL;
		int i, j, k;
		int c;

		nm_str_buf_init (&strbuf,
		                 nmtst_get_rand_uint32 () % 200u + 1u,
		                 nmtst_get_rand_bool ());

		if (i_run < 1000) {
			c = nmtst_get_rand_word_length (NULL);
			for (i = 0; i < c; i++)
				nm_str_buf_append_c (&strbuf, '0' + (i % 10));
			gstr = g_string_new (nm_str_buf_get_str (&strbuf));
			j = nmtst_get_rand_uint32 () % (strbuf.len + 1);
			k = nmtst_get_rand_uint32 () % (strbuf.len - j + 2) - 1;

			nm_str_buf_erase (&strbuf, j, k, nmtst_get_rand_bool ());
			g_string_erase (gstr, j, k);
			g_assert_cmpstr (gstr->str, ==, nm_str_buf_get_str (&strbuf));
		} else
			return;
	}
}

/*****************************************************************************/

static void
test_nm_utils_parse_next_line (void)
{
	const char *data;
	const char *data0;
	gsize data_len;
	const char *line_start;
	gsize line_len;
	int i_run;
	gsize j, k;

	data = NULL;
	data_len = 0;
	g_assert (!nm_utils_parse_next_line (&data, &data_len, &line_start, &line_len));

	for (i_run = 0; i_run < 1000; i_run++) {
		gs_unref_ptrarray GPtrArray *strv = g_ptr_array_new_with_free_func (g_free);
		gs_unref_ptrarray GPtrArray *strv2 = g_ptr_array_new_with_free_func (g_free);
		gsize strv_len = nmtst_get_rand_word_length (NULL);
		nm_auto_str_buf NMStrBuf strbuf = NM_STR_BUF_INIT (0, nmtst_get_rand_bool ());

		/* create a list of random words. */
		for (j = 0; j < strv_len; j++) {
			gsize w_len = nmtst_get_rand_word_length (NULL);
			NMStrBuf w_buf = NM_STR_BUF_INIT (nmtst_get_rand_uint32 () % (w_len + 1), nmtst_get_rand_bool ());

			for (k = 0; k < w_len; k++)
				nm_str_buf_append_c (&w_buf, '0' + (k % 10));
			nm_str_buf_maybe_expand (&w_buf, 1, TRUE);
			g_ptr_array_add (strv, nm_str_buf_finalize (&w_buf, NULL));
		}

		/* join the list of random words with (random) line delimiters
		 * ("\0", "\n", "\r" or EOF). */
		for (j = 0; j < strv_len; j++) {
			nm_str_buf_append (&strbuf, strv->pdata[j]);
again:
			switch (nmtst_get_rand_uint32 () % 5) {
			case 0:
				nm_str_buf_append_c (&strbuf, '\0');
				break;
			case 1:
				if (   strbuf.len > 0
				    && (nm_str_buf_get_str_unsafe (&strbuf))[strbuf.len - 1] == '\r') {
					/* the previous line was empty and terminated by "\r". We
					 * must not join with "\n". Retry. */
					goto again;
				}
				nm_str_buf_append_c (&strbuf, '\n');
				break;
			case 2:
				nm_str_buf_append_c (&strbuf, '\r');
				break;
			case 3:
				nm_str_buf_append (&strbuf, "\r\n");
				break;
			case 4:
				/* the last word randomly is delimited or not, but not if the last
				 * word is "". */
				if (j + 1 < strv_len) {
					/* it's not the last word. Retry. */
					goto again;
				}
				g_assert (j == strv_len - 1);
				if (((const char *) strv->pdata[j])[0] == '\0') {
					/* if the last word was "", we need a delimiter (to parse it back).
					 * Retry. */
					goto again;
				}
				/* The final delimiter gets omitted. It's EOF. */
				break;
			}
		}

		data0 = nm_str_buf_get_str_unsafe (&strbuf);
		if (   !data0
		    && nmtst_get_rand_bool ()) {
			nm_str_buf_maybe_expand (&strbuf, 1, TRUE);
			data0 = nm_str_buf_get_str_unsafe (&strbuf);
			g_assert (data0);
		}
		data_len = strbuf.len;
		g_assert ((data_len > 0 && data0) || data_len == 0);
		data = data0;
		while (nm_utils_parse_next_line (&data, &data_len, &line_start, &line_len)) {
			g_assert (line_start);
			g_assert (line_start >= data0);
			g_assert (line_start < &data0[strbuf.len]);
			g_assert (!memchr (line_start, '\0', line_len));
			g_ptr_array_add (strv2, g_strndup (line_start, line_len));
		}
		g_assert (data_len == 0);
		if (data0)
			g_assert (data == &data0[strbuf.len]);
		else
			g_assert (!data);

		g_assert (_nm_utils_strv_cmp_n ((const char *const*) strv->pdata, strv->len, (const char *const*) strv2->pdata, strv2->len) == 0);
	}
}

/*****************************************************************************/

typedef struct {
	GValue value;
	guint16 attr;
} TestTlvValue;

static char *
_tlv_rand_word (void)
{
	NMStrBuf sbuf = NM_STR_BUF_INIT (0, nmtst_get_rand_bool ());
	gsize len;

	len = nmtst_get_rand_word_length (NULL);
	for (; len > 0; len--)
		nm_str_buf_append_c (&sbuf, '0' + (nmtst_get_rand_uint32 () % 10));

	/* The returned string has two NUL characters at the end. */
	nm_str_buf_append_c (&sbuf, '\0');

	return nm_str_buf_finalize (&sbuf, NULL);
}

static void
test_tlv_rnd (void)
{
	gs_unref_array GArray *policy_arr = NULL;
	gs_unref_array GArray *values_arr = NULL;
	gs_unref_ptrarray GPtrArray *parsed_arr = NULL;
	gs_unref_ptrarray GPtrArray *val_by_idx_attr_arr = NULL;
	int i_run;

	policy_arr = g_array_new (FALSE, TRUE, sizeof (NMTlvPolicy));
	values_arr = g_array_new (FALSE, TRUE, sizeof (TestTlvValue));
	g_array_set_clear_func (values_arr, (GDestroyNotify) g_value_unset);
	parsed_arr = g_ptr_array_new ();
	val_by_idx_attr_arr = g_ptr_array_new ();

#define _L_enabled 0
#define _L(fmt, ...) do { if (_L_enabled) { printf (">>> " fmt "\n", ##__VA_ARGS__); } } while (0)

	for (i_run = 0; i_run < 100; i_run++) {
		gs_free guint8 *msg_data = NULL;
		const NMTlvAttr **parsed;
		const TestTlvValue **val_by_idx_attr;
		gs_free char *hex_str = NULL;
		NMTlvPolicy *policy;
		TestTlvValue *values;
		guint policy_len;
		guint values_len;
		gsize msg_alloc = 0;
		gsize msg_len = 0;
		int i;
		int r;
		int msg_parsed_len;

		_L ("run: %d", i_run);

		g_array_set_size (policy_arr, 0);
		g_array_set_size (values_arr, 0);

		/* initialize a random policy. */
		policy_len = nmtst_get_rand_word_length (NULL) + 1;
		g_array_set_size (policy_arr, policy_len);
		g_ptr_array_set_size (val_by_idx_attr_arr, policy_len);
		policy = (NMTlvPolicy *) policy_arr->data;
		val_by_idx_attr = (const TestTlvValue **) val_by_idx_attr_arr->pdata;
		_L ("policy: length=%u", policy_len);
		for (i = 0; i < policy_len; i++) {
			policy[i].type = nmtst_rand_select (NM_TLV_TYPE_NONE,
			                                    NM_TLV_TYPE_BOOL,
			                                    NM_TLV_TYPE_INT32,
			                                    NM_TLV_TYPE_UINT32,
			                                    NM_TLV_TYPE_INT64,
			                                    NM_TLV_TYPE_UINT64,
			                                    NM_TLV_TYPE_STR,
			                                    NM_TLV_TYPE_MEM);
			val_by_idx_attr[i] = NULL;
			_L ("policy: [%i]: type=%d", i, policy[i].type);
		}

		/* initialize a random list of values for the policy. */
		g_array_set_size (values_arr, policy_arr->len);
		for (i = 0; i < policy_arr->len; i++)
			(g_array_index (values_arr, TestTlvValue, i)).attr = i;
		values_len = nmtst_get_rand_uint32 () % (policy_arr->len + 1);
		g_array_set_size (values_arr, values_len);
		nmtst_rand_perm (NULL, values_arr->data, NULL, sizeof (TestTlvValue), values_len);
		values = (TestTlvValue *) values_arr->data;
		_L ("values: length=%u", values_len);
		for (i = 0; i < values_len; i++) {
			TestTlvValue *v = &values[i];
			const NMTlvPolicy *p = &policy[v->attr];

			g_assert (v->attr < policy_arr->len);
			g_assert (!val_by_idx_attr[v->attr]);
			switch (p->type) {
			case NM_TLV_TYPE_BOOL:
				g_value_init (&v->value, G_TYPE_BOOLEAN);
				g_value_set_boolean (&v->value, nmtst_get_rand_bool ());
				_L ("values: [%i]: attr=%d, type=%d, val_bool=%d", i, v->attr, p->type, g_value_get_boolean (&v->value));
				break;
			case NM_TLV_TYPE_INT32:
				g_value_init (&v->value, G_TYPE_INT);
				g_value_set_int (&v->value, (int) nmtst_get_rand_uint64 ());
				_L ("values: [%i]: attr=%d, type=%d, val_int32=%d (0x%08x)", i, v->attr, p->type, g_value_get_int (&v->value), g_value_get_int (&v->value));
				break;
			case NM_TLV_TYPE_UINT32:
				g_value_init (&v->value, G_TYPE_UINT);
				g_value_set_uint (&v->value, nmtst_get_rand_uint32 ());
				_L ("values: [%i]: attr=%d, type=%d, val_uint32=0x%08x", i, v->attr, p->type, g_value_get_uint (&v->value));
				break;
			case NM_TLV_TYPE_INT64:
				g_value_init (&v->value, G_TYPE_INT64);
				g_value_set_int64 (&v->value, (gint64) nmtst_get_rand_uint64 ());
				_L ("values: [%i]: attr=%d, type=%d, val_int64=%"G_GINT64_FORMAT" (0x%016"G_GINT64_MODIFIER"x)", i, v->attr, p->type, g_value_get_int64 (&v->value), g_value_get_int64 (&v->value));
				break;
			case NM_TLV_TYPE_UINT64:
				g_value_init (&v->value, G_TYPE_UINT64);
				g_value_set_uint64 (&v->value, nmtst_get_rand_uint64 ());
				_L ("values: [%i]: attr=%d, type=%d, val_uint64=0x%016"G_GUINT64_FORMAT, i, v->attr, p->type, g_value_get_uint64 (&v->value));
				break;
			case NM_TLV_TYPE_STR:
			case NM_TLV_TYPE_MEM:
				g_value_init (&v->value, G_TYPE_STRING);
				g_value_take_string (&v->value, _tlv_rand_word ());
				_L ("values: [%i]: attr=%d, type=%d, val_%s=\"%s\" (%zu)", i, v->attr, p->type, p->type == NM_TLV_TYPE_STR ? "str" : "mem", g_value_get_string (&v->value), strlen (g_value_get_string (&v->value)));
				break;
			default:
				g_assert (p->type == NM_TLV_TYPE_NONE);
				break;
			}
			if (p->type != NM_TLV_TYPE_NONE)
				val_by_idx_attr[v->attr] = v;
		}

		if (nmtst_get_rand_bool ())
			msg_data = g_malloc (1);
		for (i = 0; i < values_len; i++) {
			const TestTlvValue *v = &values[i];
			const NMTlvPolicy *p = &policy[v->attr];

			switch (p->type) {
			case NM_TLV_TYPE_BOOL:
				r = nm_tlv_msg_append_bool ((void **) &msg_data, &msg_alloc, &msg_len, policy, v->attr, g_value_get_boolean (&v->value));
				g_assert_cmpint (r, ==, 0);
				break;
			case NM_TLV_TYPE_INT32:
				r = nm_tlv_msg_append_int32 ((void **) &msg_data, &msg_alloc, &msg_len, policy, v->attr, g_value_get_int (&v->value));
				g_assert_cmpint (r, ==, 0);
				break;
			case NM_TLV_TYPE_UINT32:
				r = nm_tlv_msg_append_uint32 ((void **) &msg_data, &msg_alloc, &msg_len, policy, v->attr, g_value_get_uint (&v->value));
				g_assert_cmpint (r, ==, 0);
				break;
			case NM_TLV_TYPE_INT64:
				r = nm_tlv_msg_append_int64 ((void **) &msg_data, &msg_alloc, &msg_len, policy, v->attr, g_value_get_int64 (&v->value));
				g_assert_cmpint (r, ==, 0);
				break;
			case NM_TLV_TYPE_UINT64:
				r = nm_tlv_msg_append_uint64 ((void **) &msg_data, &msg_alloc, &msg_len, policy, v->attr, g_value_get_uint64 (&v->value));
				g_assert_cmpint (r, ==, 0);
				break;
			case NM_TLV_TYPE_MEM:
			case NM_TLV_TYPE_STR: {
				const char *s = g_value_get_string (&v->value);
				gssize l = strlen (s);

				if (p->type == NM_TLV_TYPE_STR) {
					if (l == 0) {
						if (nmtst_get_rand_bool ())
							s = NULL;
						else
							l = -1;
					} else {
						if (nmtst_get_rand_bool ())
							l = -1;
					}
					r = nm_tlv_msg_append_str ((void **) &msg_data, &msg_alloc, &msg_len, policy, v->attr, s, l);
				} else
					r = nm_tlv_msg_append_mem ((void **) &msg_data, &msg_alloc, &msg_len, policy, v->attr, s, l);
				g_assert_cmpint (r, ==, 0);
				break;
			}
			default:
				g_assert (p->type == NM_TLV_TYPE_NONE);
				break;
			}
		}

		g_ptr_array_set_size (parsed_arr, policy_len);
		parsed = (const NMTlvAttr **) parsed_arr->pdata;

		_L ("binary: (%zu) %s", msg_len, (hex_str = nm_utils_bin2hexstr_full (msg_data, msg_len, ':', FALSE, NULL)));

		r = nm_tlv_msg_parse_full (msg_data,
		                           msg_len,
		                           policy,
		                           policy_len,
		                           parsed,
		                             nmtst_get_rand_bool ()
		                           ? NM_TLV_PARSE_FLAGS_STRICT
		                           : NM_TLV_PARSE_FLAGS_NONE);
		g_assert_cmpint (r, >=, 0);
		msg_parsed_len = r;

		_L ("parsed: highest-attr=%d-1", msg_parsed_len);

		g_assert (msg_parsed_len == 0 || parsed[msg_parsed_len] - 1);

		for (i = 0; i < policy_len; i++) {
			const NMTlvAttr *a = parsed[i];
			const NMTlvPolicy *p = &policy[i];

			_L ("parsed: attr=%d, type=%d, offset=%d", i, p->type, a ? ((int) ((guint8 *) a - (guint8 *) msg_data)) : -1);
		}

		for (i = 0; i < policy_len; i++) {
			const NMTlvPolicy *p = &policy[i];
			const TestTlvValue *v = val_by_idx_attr[i];
			const NMTlvAttr *a = parsed[i];

			if (!a) {
				g_assert (!v);
				continue;
			}

			g_assert (i < msg_parsed_len);
			g_assert (v);
			switch (p->type) {
			case NM_TLV_TYPE_BOOL:
				g_assert_cmpint (g_value_get_boolean (&v->value), ==, nm_tlv_attr_get_bool (policy, a));
				break;
			case NM_TLV_TYPE_INT32:
				g_assert_cmpint (g_value_get_int (&v->value), ==, nm_tlv_attr_get_int32 (policy, a));
				break;
			case NM_TLV_TYPE_UINT32:
				g_assert_cmpint (g_value_get_uint (&v->value), ==, nm_tlv_attr_get_uint32 (policy, a));
				break;
			case NM_TLV_TYPE_INT64:
				g_assert_cmpint (g_value_get_int64 (&v->value), ==, nm_tlv_attr_get_int64 (policy, a));
				break;
			case NM_TLV_TYPE_UINT64:
				g_assert_cmpint (g_value_get_uint64 (&v->value), ==, nm_tlv_attr_get_uint64 (policy, a));
				break;
			case NM_TLV_TYPE_STR: {
				const char *s;
				gsize l;
				gsize *p_l = nmtst_get_rand_bool () ? &l : NULL;

				s = nm_tlv_attr_get_str (policy, a, p_l);
				g_assert (s);
				g_assert (!p_l || strlen (s) == *p_l);
				g_assert_cmpstr (s, ==, g_value_get_string (&v->value));
				break;
			}
			case NM_TLV_TYPE_MEM: {
				const char *s;
				const char *s0;
				gsize l;

				s = (const char *) nm_tlv_attr_get_mem (policy, a, &l);
				g_assert (s);
				s0 = g_value_get_string (&v->value);
				g_assert_cmpint (strlen (s0), ==, l);
				g_assert (strncmp (s, s0, l) == 0);
				break;
			}
			default:
				g_assert_not_reached ();
			}
		}
	}
}

/*****************************************************************************/

static void
test_tlv_manual (void)
{
	static const NMTlvPolicy policy[5] = {
		[1] = NM_TLV_POLICY_INIT (NM_TLV_TYPE_BOOL),
		[2] = NM_TLV_POLICY_INIT (NM_TLV_TYPE_UINT32),
		[3] = NM_TLV_POLICY_INIT (NM_TLV_TYPE_MEM),
	};
	const NMTlvAttr *tb[G_N_ELEMENTS (policy)];
	const NMTlvAttr *tb2[G_N_ELEMENTS (policy)];
	const NMTlvAttr *tb3[G_N_ELEMENTS (policy)];
	const NMTlvAttr *attr;
	int r;
	static const guint8 msg_bin[] = {
		0x30, 0x00, 0x00, 0x04,
			0xaa, 0xbb, 0xcc, 0xdd,
		0x01, 0x00, 0x00, 0x01,
			0x01, 0x00, 0x00, 0x00,
		0x03, 0x00, 0x00, 0x08,
			0x02, 0x00, 0x00, 0x04,
				0x01, 0x03, 0x05, 0x30,
	};
	const gsize msg_len = G_N_ELEMENTS (msg_bin);
	gs_free const NMTlvAttr **attrs_list = NULL;
	gs_free guint8 *msg2_bin = NULL;
	gsize msg2_alloc = 0;
	gsize msg2_len = 0;
	gsize nest_start;
	int i;
	guint32 u32;

	r = nm_tlv_msg_parse_full (msg_bin, msg_len, policy, G_N_ELEMENTS (policy), tb, NM_TLV_PARSE_FLAGS_STRICT);
	g_assert_cmpint (r, ==, -ENOENT);

	if (nmtst_get_rand_bool ())
		r = nm_tlv_msg_parse_full (msg_bin, msg_len, policy, G_N_ELEMENTS (policy), tb, NM_TLV_PARSE_FLAGS_NONE);
	else
		r = nm_tlv_msg_parse (msg_bin, msg_len, policy, tb, NM_TLV_PARSE_FLAGS_NONE);
	g_assert_cmpint (r, ==, 4);
	g_assert (!tb[0]);
	g_assert ((gpointer) tb[1] == &msg_bin[8]);
	g_assert (!tb[2]);
	g_assert ((gpointer) tb[3] == &msg_bin[16]);
	g_assert (!tb[4]);
	g_assert_cmpint (tb[1]->att_type, ==, 1);

	i = 0;
	nm_tlv_msg_foreach (attr, msg_bin, msg_len) {
		switch (i++) {
		case 0:
			g_assert (attr->att_type == 0x30);
			g_assert ((gpointer) attr == &msg_bin[0]);
			break;
		case 1:
			g_assert (attr == tb[1]);
			g_assert ((gpointer) attr == &msg_bin[8]);
			break;
		case 2:
			g_assert (attr == tb[3]);
			g_assert ((gpointer) attr == &msg_bin[16]);
			break;
		default:
			g_assert_not_reached();
		}
	}
	g_assert (i == 3);

	r = nm_tlv_msg_get_attrs (msg_bin, msg_len, tb, G_N_ELEMENTS (tb), &attrs_list);
	g_assert_cmpint (r, ==, 2);
	g_assert (attrs_list[0] == tb[1]);
	g_assert (attrs_list[1] == tb[3]);
	g_assert (!attrs_list[2]);

	nm_clear_pointer (&attrs_list, free);

	r = nm_tlv_msg_get_attrs (msg_bin, msg_len, NULL, 0, &attrs_list);
	g_assert_cmpint (r, ==, 3);
	g_assert (attrs_list[0] == (gpointer) msg_bin);
	g_assert (attrs_list[1] == tb[1]);
	g_assert (attrs_list[2] == tb[3]);
	g_assert (!attrs_list[3]);

	g_assert (tb[3]);
	{
		const guint8 *attr3;
		gsize attr3_len;

		attr3 = nm_tlv_attr_get_mem (policy, tb[3], &attr3_len);
		g_assert (attr3);
		g_assert (attr3 == &msg_bin[20]);
		g_assert (attr3_len == 8);
		r = nm_tlv_msg_parse_full (attr3, attr3_len, policy, G_N_ELEMENTS (policy), tb2, NM_TLV_PARSE_FLAGS_STRICT);
		g_assert_cmpint (r, ==, 3);
	}

	u32 = htobe32 (0xaabbccddu);
	r = _nm_tlv_msg_append ((gpointer *) &msg2_bin, &msg2_alloc, &msg2_len, 0x30, &u32, sizeof (u32), false);
	g_assert_cmpint (r, ==, 0);
	nm_tlv_msg_append_bool ((gpointer *) &msg2_bin, &msg2_alloc, &msg2_len, policy, 1, TRUE);
	g_assert_cmpint (r, ==, 0);

nest_start:
	r = nm_tlv_msg_nest_open ((gpointer *) &msg2_bin, &msg2_alloc, &msg2_len, 3, &nest_start);
	g_assert_cmpint (r, ==, 0);
	r = nm_tlv_msg_append_uint32 ((gpointer *) &msg2_bin, &msg2_alloc, &msg2_len, policy, 2, 0x01030530);
	g_assert_cmpint (r, ==, 0);
	if (nmtst_get_rand_bool ()) {
		nm_tlv_msg_nest_reset ((gpointer *) &msg2_bin, &msg2_len, nest_start);
		goto nest_start;
	}

	r = nm_tlv_msg_nest_close ((gpointer *) &msg2_bin, &msg2_len, nest_start);
	g_assert_cmpint (r, ==, 0);

	r = nm_tlv_msg_parse (msg2_bin, msg2_len, policy, tb3, NM_TLV_PARSE_FLAGS_NONE);
	g_assert_cmpint (r, ==, 4);

	g_assert_cmpint (msg_len, ==, msg2_len);
	g_assert_cmpmem (msg2_bin, msg2_len, msg_bin, msg_len);
}

/*****************************************************************************/

NMTST_DEFINE ();

int main (int argc, char **argv)
{
	nmtst_init (&argc, &argv, TRUE);

	g_test_add_func ("/general/test_gpid", test_gpid);
	g_test_add_func ("/general/test_monotonic_timestamp", test_monotonic_timestamp);
	g_test_add_func ("/general/test_nmhash", test_nmhash);
	g_test_add_func ("/general/test_nm_make_strv", test_make_strv);
	g_test_add_func ("/general/test_nm_strdup_int", test_nm_strdup_int);
	g_test_add_func ("/general/test_nm_strndup_a", test_nm_strndup_a);
	g_test_add_func ("/general/test_nm_ip4_addr_is_localhost", test_nm_ip4_addr_is_localhost);
	g_test_add_func ("/general/test_unaligned", test_unaligned);
	g_test_add_func ("/general/test_strv_cmp", test_strv_cmp);
	g_test_add_func ("/general/test_strstrip_avoid_copy", test_strstrip_avoid_copy);
	g_test_add_func ("/general/test_nm_utils_bin2hexstr", test_nm_utils_bin2hexstr);
	g_test_add_func ("/general/test_nm_ref_string", test_nm_ref_string);
	g_test_add_func ("/general/test_string_table_lookup", test_string_table_lookup);
	g_test_add_func ("/general/test_nm_utils_get_next_realloc_size", test_nm_utils_get_next_realloc_size);
	g_test_add_func ("/general/test_nm_str_buf", test_nm_str_buf);
	g_test_add_func ("/general/test_nm_utils_parse_next_line", test_nm_utils_parse_next_line);
	g_test_add_func ("/general/test_tlv_rnd", test_tlv_rnd);
	g_test_add_func ("/general/test_tlv_manual", test_tlv_manual);

	return g_test_run ();
}
