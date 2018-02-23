/**
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
@file
@internal
*/

/** \addtogroup PBAP_Internal */
/**@{*/

#ifndef _OI_PBAP_PRIVATE_H
#define _OI_PBAP_PRIVATE_H



/* Application Parameter sizes */
#define OI_PBAP_APP_PARAM_HEADER_SIZE           (sizeof(OI_UINT8) + sizeof(OI_UINT8))
#define OI_PBAP_APP_PARAM_ORDER_SIZE            (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT8))
#define OI_PBAP_APP_PARAM_SEARCHVALUE_SIZE(l)   (OI_PBAP_APP_PARAM_HEADER_SIZE + (l))
#define OI_PBAP_APP_PARAM_SEARCHATTRIBUTE_SIZE  (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT8))
#define OI_PBAP_APP_PARAM_MAXLISTCOUNT_SIZE     (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT16))
#define OI_PBAP_APP_PARAM_LISTSTARTOFFSET_SIZE  (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT16))
#define OI_PBAP_APP_PARAM_FILTER_SIZE           (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT64))
#define OI_PBAP_APP_PARAM_FORMAT_SIZE           (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT8))
#define OI_PBAP_APP_PARAM_PHONBOOKSIZE_SIZE     (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT16))
#define OI_PBAP_APP_PARAM_NEWMISSEDCALLS_SIZE   (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT8))
#define OI_PBAP_APP_PARAM_SUPP_FEATURE_SIZE     (OI_PBAP_APP_PARAM_HEADER_SIZE + sizeof(OI_UINT32))

#define OI_PBAP_MAX_PBAP_PATH_SIZE 32


extern const OI_CHAR16 OI_PBAP_usim1[];
extern const OI_CHAR16 OI_PBAP_utelecom[];
extern const OI_CHAR16 OI_PBAP_vcf[];
extern const OI_CHAR16 * OI_PBAP_upbdirs[];
extern const OI_UINT16 OI_PBAP_upbdirsizes[];


#ifdef OI_DEBUG
/**
 * OI_PBAP_DumpObexHeaders()
 *
 * Helper debug function for nicely displaying OBEX headers relevant to PBAP.
 */
extern void OI_PBAP_DumpObexHeaders(const OI_OBEX_HEADER_LIST *hdrList);

#else

/**
 * The following macro is a placeholder when building non-debug mode code.
 */
#define OI_PBAP_DumpObexHeaders(p) do {} while(0)

#endif





#endif
/**@}*/
