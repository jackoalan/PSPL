//
//  PMDLBlenderInterface.c
//  PSPL
//
//  Created by Jack Andersen on 7/1/13.
//
//

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <PSPLExtension.h>
#include "PMDLBlenderInterface.h"

#define BLENDER_LOCAL_SOCK_PATH "/tmp/pspl-blender.sock"

/* Version handshake */
static void handshake(pspl_pmdl_blender_connection* connection) {
    
    // Perform version handshake
    struct blender_get_server_version_send version_send = {
        .expected_version = BLENDER_RPC_CLIENT_VERSION
    };
    send(connection->sock, &version_send, sizeof(version_send), 0);
    
    struct blender_get_server_version_recv version_recv;
    recv(connection->sock, &version_recv, sizeof(version_recv), 0);
    connection->pmdl_server_version = version_recv.actual_version;
    
    if (!version_recv.compatible)
        pspl_error(-1, "Blender RPC Incompatible",
                   "Blender Server claims that version %d is incompatible. It requests version %d",
                   BLENDER_RPC_CLIENT_VERSION, version_recv.actual_version);
    
}

/* Open connection to Blender on local machine */
int pspl_pmdl_blender_open_local_connection(pspl_pmdl_blender_connection* connection) {
    
    const struct protoent* tcp_proto = getprotobyname("tcp");
    connection->sock = socket(PF_LOCAL, SOCK_STREAM, tcp_proto->p_proto);
    
    if (!connection->sock)
        pspl_error(-1, "Socket Creation Error",
                   "error establishing local Blender socket - errno %d (%s)",
                   errno, strerror(errno));
    
    struct sockaddr_in local_addr = {
        .sin_len = sizeof(struct sockaddr_in),
        .sin_family = AF_INET,
        .sin_port = htons(33320)
    };
    inet_pton(AF_INET, "127.0.0.1", &local_addr.sin_addr);
    memset(&local_addr.sin_zero, 0, sizeof(local_addr.sin_zero));
    
    if (bind(connection->sock, (struct sockaddr*)&local_addr, local_addr.sin_len))
        pspl_error(-1, "Socket Bind Error",
                   "error binding local Blender socket - errno %d (%s)",
                   errno, strerror(errno));
    
    struct sockaddr blender_local_addr;
    if (connect(connection->sock, &blender_local_addr, sizeof(blender_local_addr)))
        pspl_error(-1, "Socket Connect Error",
                   "error connecting to local Blender socket - errno %d (%s)",
                   errno, strerror(errno));
    
    const struct hostent* blender_local_host = gethostbyaddr(&blender_local_addr,
                                                             sizeof(blender_local_addr),
                                                             blender_local_addr.sa_family);
    fprintf(stderr, "-- Connected to Local Blender at %s\n", blender_local_host->h_name);
    
    
    // Version handshake
    handshake(connection);
    
    return 0;
    
}

/* Open connection to Blender on remote machine */
int pspl_pmdl_blender_open_remote_connection(pspl_pmdl_blender_connection* connection,
                                             int bound_socket) {
    
    connection->sock = bound_socket;
    
    struct sockaddr blender_remote_addr;
    if (connect(connection->sock, &blender_remote_addr, sizeof(blender_remote_addr)))
        pspl_error(-1, "Socket Connect Error",
                   "error connecting to remote Blender socket - errno %d (%s)",
                   errno, strerror(errno));
    
    const struct hostent* blender_remote_host = gethostbyaddr(&blender_remote_addr,
                                                              sizeof(blender_remote_addr),
                                                              blender_remote_addr.sa_family);
    fprintf(stderr, "-- Connected to Remote Blender at %s\n", blender_remote_host->h_name);
    
    
    // Version handshake
    handshake(connection);
    
    return 0;
}

/* Close connection to Blender */
void pspl_pmdl_blender_close_connection(pspl_pmdl_blender_connection* connection) {
    close(connection->sock);
}
