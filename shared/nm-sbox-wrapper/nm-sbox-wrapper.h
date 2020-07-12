// SPDX-License-Identifier: LGPL-2.1+

#ifndef __NM_SBOX_H__
#define __NM_SBOX_H__

typedef struct {
} NMSBoxWrapperVTable;

typedef struct {
} NMSBoxWrapperConfig;

typedef struct _NMSBoxWrapperHandle NMSBoxWrapperHandle;

NMSBoxWrapperHandle *nm_sbox_wrapper_start (const NMSBoxWrapperVTable *vtable,
                                            const NMSBoxWrapperConfig *config,
                                            GError **error);

#endif /* __NM_SBOX_H__ */
