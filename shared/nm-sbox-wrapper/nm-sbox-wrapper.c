// SPDX-License-Identifier: LGPL-2.1+

#include "nm-default.h"

#include "nm-sbox-wrapper.h"

/*****************************************************************************/

struct _NMSBoxWrapperHandle {
	NMSBoxWrapperVTable vtable;
};

/*****************************************************************************/

NMSBoxWrapperHandle *
nm_sbox_wrapper_start (const NMSBoxWrapperVTable *vtable,
                       const NMSBoxWrapperConfig *config,
                       GError **error)
{
	nm_auto_close int channel_fd = -1;
	int r;

	g_return_val_if_fail (vtable, NULL);
	g_return_val_if_fail (config, NULL);
	g_return_val_if_fail (!error || !*error, NULL);

	r = socketpair (AF_UNIX, SOCK_DGRAM, 0, s_pair);

	return NULL;
}
