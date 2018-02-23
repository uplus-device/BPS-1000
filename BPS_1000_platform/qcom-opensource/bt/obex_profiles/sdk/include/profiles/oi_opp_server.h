#ifndef _OI_OPP_SERVER_H
#define _OI_OPP_SERVER_H

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
  This file provides the API for the server side of the Object Push Profile.
  The Object Push Profile provides functions for establishing a connection to a
  remote device that support the Object Push Profile over RFCOMM and
  functions for pushing and pulling objects. This implementation currently only
  allows one connection at a time.
 */

#include "oi_status.h"
#include "oi_opp_sys.h"
#include "oi_obexspec.h"
#include "oi_sdp.h"
#include "oi_dataelem.h"

/** \addtogroup OPP OPP APIs */
/**@{*/

#ifdef __cplusplus
extern "C" {
#endif


/**
 * represents an active connection between the local OPP server and a remote
 * OPP client
 */
typedef OI_OPP_CONNECTION OI_OPP_SERVER_CONNECTION_HANDLE;


/**
 * Represents a registered OPP server
 */
typedef OI_OBEX_SERVER_HANDLE OI_OPP_SERVER_HANDLE;


/*
 * OBEX object types that might be supported by an OPP server.
 * Bits not specified below are reserved.
 */
#define OI_OPP_SERVER_OBJ_FORMAT_ANY       0xFFFFFFFF /**< Any format is supported */
#define OI_OPP_SERVER_OBJ_FORMAT_VCARD_2_1 OI_BIT0
#define OI_OPP_SERVER_OBJ_FORMAT_VCARD_3_0 OI_BIT1
#define OI_OPP_SERVER_OBJ_FORMAT_VCAL_1_0  OI_BIT2
#define OI_OPP_SERVER_OBJ_FORMAT_ICAL_2_0  OI_BIT3
#define OI_OPP_SERVER_OBJ_FORMAT_VNOTE     OI_BIT4
#define OI_OPP_SERVER_OBJ_FORMAT_VMESSAGE  OI_BIT5


/**
 * Supported formats are ORed together and passed in to OI_OPPServer_Register().
 */
typedef OI_UINT32 OI_OPP_SERVER_OBJECT_FORMATS;

/**
 * List of events which may be generated by Server when triggered by remote
 * client activity.
 */
typedef enum {
    OI_OPP_SERVER_EVENT_PUSH = 0,
    OI_OPP_SERVER_EVENT_PULL,
    OI_OPP_SERVER_EVENT_PUSH_PROGRESS,
    OI_OPP_SERVER_EVENT_PULL_PROGRESS,
    OI_OPP_SERVER_EVENT_PUSH_COMPLETE,
    OI_OPP_SERVER_EVENT_PULL_COMPLETE
} OI_OPP_SERVER_EVENT;

typedef struct {
    OI_OPP_SERVER_EVENT     event;
    /// @cond
    union {
        struct {
            const OI_OBEX_UNICODE *localName;   /* if known, else NULL */
            OI_UINT32              totalSize;   /* if known, else 0 */
            const OI_CHAR         *objType;     /* if known, else 0 */
        } push;
        struct {
            const OI_OBEX_UNICODE *localName;   /* if named, else NULL for default object */
            OI_UINT32              totalSize;   /* if known, else 0 */
            const OI_CHAR         *objType;     /* if known, else 0 */
        } pull;
        struct {
            OI_UINT32              bytesTransferred;
        } pushProgress;
        struct {
            OI_UINT32              bytesTransferred;
        } pullProgress;
        struct {
            OI_UINT32              finalSize;
            OI_STATUS              status;           /**< Status of the transfer upon completion */
        } pushComplete;
        struct {
            OI_UINT32              finalSize;
            OI_STATUS              status;           /**< Status of the transfer upon completion */
        } pullComplete;
    } data;
    /// @endcond

} OI_OPP_SERVER_EVENT_DATA;



/**
 * The application must provide a function of this type to OI_OPPServer_Register()
 * to be called when a client attempts to connect to the OPP server. The
 * application must call OI_OPP_AcceptConnect() to accept or reject the
 * connection request.
 *
 * @param clientAddr    The Bluetooth device address of the client that is
 *                      attempting to connect.
 *
 * @param connectionId  handle representing the connection between a remote
 *                      client and the local OPP server.
 *
 */
typedef void (*OI_OPP_SERVER_CONNECT_IND)(OI_BD_ADDR *clientAddr,
                                          OI_OPP_SERVER_CONNECTION_HANDLE connectionId);


/**
 * The application must provide a function of this type to OI_OPPServer_Register()
 * to be called when a client disconnects from the OPP server.
 *
 * @param connectionId  handle representing the connection between a remote
 *                      client and the local OPP server.
 *
 */
typedef void (*OI_OPP_SERVER_DISCONNECT_IND)(OI_OPP_SERVER_CONNECTION_HANDLE connectionId);

/**
 * The application *may* provide an event handler to receive various indications
 * of activity. Some event indications such as PUSH start indications require an
 * acceptance of the incoming file. This callback may be NULL, in which case the
 * default action if required will be performed automatically.
 *
 * @param connectionId  handle representing the connection between a remote
 *                      client and the local OPP server.
 *
 * @param eventPtr      pointer to structure indicating what event occured.
 *
 */
typedef void (*OI_OPP_SERVER_EVENT_IND)(OI_OPP_SERVER_CONNECTION_HANDLE connectionId,
                                        OI_OPP_SERVER_EVENT_DATA       *eventPtr);

typedef struct {
    OI_OPP_SERVER_CONNECT_IND    connectInd;        /* Mandatory */
    OI_OPP_SERVER_DISCONNECT_IND disconnectInd;     /* Mandatory */
    OI_OPP_SERVER_EVENT_IND      eventInd;          /* Optional (may be NULL) */
} OI_OPP_SERVER_CALLBACKS;


/**
 * The OPP application must call this function to accept or reject a connect
 * request from a client.
 *
 * @param connectionId  handle representing the connection between a remote
 *                      client and the local OPP server.
 *
 * @param  allowPush   If TRUE the connection is accepted and the client will
 *                     be allowed to push objects to the server. If allowPush
 *                     and allowPull are both FALSE the connection is rejected.
 *
 * @param  allowPull   If TRUE the connection is accepted and the client will be
 *                     allowed to pull objects from the server. If allowPull and
 *                     allowPush are both FALSE the connection is rejected.
 */

OI_STATUS OI_OPP_AcceptConnect(OI_OPP_SERVER_CONNECTION_HANDLE connectionId,
                               OI_BOOL allowPush,
                               OI_BOOL allowPull);

/**
 * Forcibly severs the connection from an OPP client to the OPP server. This
 * function should be called for all active connections before terminating the
 * OPP server.
 *
 * @param connectionId  handle representing the connection between a remote
 *                      client and the local OPP server.
 *
 * @return              OI_OK if the connection will be terminated.
 */

OI_STATUS OI_OPPServer_ForceDisconnect(OI_OPP_SERVER_CONNECTION_HANDLE connectionId);


/**
 * Initialize the Object Push server and register it in the service discovery
 * database.
 *
 * @param callbacks            This structure pointer contains list of all
 *                             callbacks which may come from the server due to
 *                             remote client activity.
 *
 * @param supportedFormats     OPP object formats supported by this server,
 *                             composed by OR'ing together the various format flags.
 *
 * @param objectOperations     the set of operations that interface to the
 *                             application's object management system.
 *
 * @param strings              name to register in service record
 *
* @param serverInstance        [OUT] returns a handle for the OPP server instance. This handle
 *                             is required to deregsiter the OPP server, see OI_OPPServer_Deregister().
 *
 * @return                     OI_OK if the service was initialized and registered succesfully with SDP
 */

OI_STATUS OI_OPPServer_Register(const OI_OPP_SERVER_CALLBACKS *callbacks,
                                const OI_OPP_OBJSYS_FUNCTIONS *objectOperations,
                                OI_OPP_SERVER_OBJECT_FORMATS   supportedFormats,
                                const OI_SDP_STRINGS          *strings,
                                OI_OPP_SERVER_HANDLE          *serverInstance);


/**
 * Get the service record handle associated with this service.
 * This can be used with e.g. OI_SDPDB_SetAttributeList to add
 * vendor-specific SDP attributes to the profile.
 *
 * @param serverInstance   identifies the registered server instance.
 *
 * @param handle           return the service record's handle
 */
OI_STATUS OI_OPPServer_GetServiceRecord(OI_OPP_SERVER_HANDLE serverInstance,
                                        OI_UINT32 *handle);

/**
 * Deregisters an OPP server.
 *
 * @param serverInstance  The server instance handle that was returned when the server was
 *                        registered.
 *
 * @return                OI_OK if the service was successfully deregistered.
 *
 */
OI_STATUS OI_OPPServer_Deregister(OI_OPP_SERVER_HANDLE serverInstance);

/**
 * Accept/Reject incoming (PUSH) object. This function must be called by the
 * App upon reception of the OI_OPP_SERVER_EVENT_PUSH event, to allow storage
 * of the incoming object in the local object storage system.
 *
 * Note: For legacy purposes, this function is required only if OI_OPP_SERVER_EVENT_IND
 *       has been registered as part of the OI_OPP_SERVER_CALLBACKS.
 *
 * @param connectionId    The server connection handle that was returned when the server
 *                        connection was opened.
 *
 * @param objname         File or Object name to store incoming object to, or
 *                        NULL to keep/confirm default incoming object name.
 *
 * @param accept          TRUE - Accept incoming file, FALSE - Reject
 *
 * @return                OI_OK if the rcvObject was successfuly opened for
 *                        writing (or if rejecting).
 *
 */
OI_STATUS OI_OPPServer_AcceptPush(OI_OPP_SERVER_CONNECTION_HANDLE connectionId,
                                  OI_OBEX_UNICODE                *objname,
                                  OI_BOOL                         accept);

/*************************************************************

    Debug print helpers

        Returns null string when compiled for release mode

*************************************************************/

extern OI_CHAR  *OI_OPPServer_eventText(OI_OPP_SERVER_EVENT event) ;

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* _OI_OPP_SERVER_H */
