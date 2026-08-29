#ifndef SERVER_H
#define SERVER_H

#include "common.h"

/*
 * Starts the Intellicompress HTTP server.
 *
 * Returns:
 *  0  on successful shutdown
 * -1  on initialization failure
 */
int start_server(int port);

#endif