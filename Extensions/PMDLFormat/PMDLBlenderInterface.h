//
//  PMDLBlenderInterface.h
//  PSPL
//
//  Created by Jack Andersen on 7/1/13.
//
//

#ifndef PSPL_PMDLBlenderInterface_h
#define PSPL_PMDLBlenderInterface_h

#include <stdint.h>

/* Client RPC Version */
#define BLENDER_RPC_CLIENT_VERSION 1

/* RPC Operation codes */
enum blender_rpc_opcode {
    BLENDER_RPC_GET_SERVER_VERSION
};


/* GET_SERVER_VERSION */
struct blender_get_server_version_send {
    // Version that PSPL expects
    uint32_t expected_version;
};
struct blender_get_server_version_recv {
    // Whether Blender server is compatible with expected version
    uint32_t compatible;
    // Version that Blender-server implements
    uint32_t actual_version;
};


/* Blender-connection state */
typedef struct {
    
    // TCP-socket of Blender connection
    int sock;
    
    // Version number of Blender PMDL Server
    uint32_t pmdl_server_version;
    
} pspl_pmdl_blender_connection;


/* Open connection to Blender on local machine */
int pspl_pmdl_blender_open_local_connection(pspl_pmdl_blender_connection* connection);

/* Open connection to Blender on remote machine */
int pspl_pmdl_blender_open_remote_connection(pspl_pmdl_blender_connection* connection,
                                             int bound_socket);

/* Close connection to Blender */
void pspl_pmdl_blender_close_connection(pspl_pmdl_blender_connection* connection);

#endif
