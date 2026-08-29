#include "../include/server.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 65536

static void send_response(
    SOCKET client,
    int status_code,
    const char *status_text,
    const char *content_type,
    const char *body
)
{
    char response[BUFFER_SIZE];

    int body_length = (int)strlen(body);

    int response_length = snprintf(
        response,
        sizeof(response),

        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",

        status_code,
        status_text,
        content_type,
        body_length,
        body
    );

    send(
        client,
        response,
        response_length,
        0
    );
}


static void handle_request(SOCKET client)
{
    char buffer[BUFFER_SIZE];

    memset(buffer, 0, sizeof(buffer));

    int received = recv(
        client,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (received <= 0)
        return;

    buffer[received] = '\0';

    /*
     * Print the incoming HTTP request.
     * This is extremely useful while developing.
     */
    printf("\n========== HTTP REQUEST ==========\n");
    printf("%s\n", buffer);
    printf("==================================\n");

    /*
     * Extract method and path.
     */
    char method[16];
    char path[256];

    if (sscanf(
        buffer,
        "%15s %255s",
        method,
        path
    ) != 2)
    {
        send_response(
            client,
            400,
            "Bad Request",
            "application/json",
            "{\"error\":\"Invalid HTTP request\"}"
        );

        return;
    }

    /*
     * CORS preflight request.
     */
    if (strcmp(method, "OPTIONS") == 0)
    {
        send_response(
            client,
            204,
            "No Content",
            "text/plain",
            ""
        );

        return;
    }

    /*
     * Root endpoint.
     */
    if (
        strcmp(method, "GET") == 0 &&
        strcmp(path, "/") == 0
    )
    {
        send_response(
            client,
            200,
            "OK",
            "application/json",
            "{\"name\":\"Intellicompress C Backend\",\"status\":\"running\"}"
        );

        return;
    }

    /*
     * Health endpoint.
     */
    if (
        strcmp(method, "GET") == 0 &&
        strcmp(path, "/api/health") == 0
    )
    {
        send_response(
            client,
            200,
            "OK",
            "application/json",
            "{\"status\":\"ok\",\"backend\":\"C\"}"
        );

        return;
    }

    /*
     * Analyze endpoint.
     *
     * Actual analyzer will be connected later.
     */
    if (
        strcmp(method, "POST") == 0 &&
        strcmp(path, "/api/analyze") == 0
    )
    {
        send_response(
            client,
            200,
            "OK",
            "application/json",
            "{\"status\":\"received\",\"module\":\"analyzer\"}"
        );

        return;
    }

    /*
     * Compression endpoint.
     */
    if (
        strcmp(method, "POST") == 0 &&
        strcmp(path, "/api/compress") == 0
    )
    {
        send_response(
            client,
            200,
            "OK",
            "application/json",
            "{\"status\":\"received\",\"module\":\"compression\"}"
        );

        return;
    }

    /*
     * Decompression endpoint.
     */
    if (
        strcmp(method, "POST") == 0 &&
        strcmp(path, "/api/decompress") == 0
    )
    {
        send_response(
            client,
            200,
            "OK",
            "application/json",
            "{\"status\":\"received\",\"module\":\"decompression\"}"
        );

        return;
    }

    /*
     * Comparison endpoint.
     */
    if (
        strcmp(method, "POST") == 0 &&
        strcmp(path, "/api/compare") == 0
    )
    {
        send_response(
            client,
            200,
            "OK",
            "application/json",
            "{\"status\":\"received\",\"module\":\"comparison\"}"
        );

        return;
    }

    /*
     * Unknown endpoint.
     */
    send_response(
        client,
        404,
        "Not Found",
        "application/json",
        "{\"error\":\"Endpoint not found\"}"
    );
}


int start_server(int port)
{
    WSADATA wsa_data;

    /*
     * Initialize Winsock.
     */
    int result = WSAStartup(
        MAKEWORD(2, 2),
        &wsa_data
    );

    if (result != 0)
    {
        printf("WSAStartup failed: %d\n", result);
        return -1;
    }

    /*
     * Create TCP socket.
     */
    SOCKET server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (server_socket == INVALID_SOCKET)
    {
        printf(
            "Socket creation failed: %d\n",
            WSAGetLastError()
        );

        WSACleanup();
        return -1;
    }

    /*
     * Allow reuse of the port.
     */
    int option = 1;

    setsockopt(
        server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (const char *)&option,
        sizeof(option)
    );

    /*
     * Configure address.
     */
    struct sockaddr_in server_address;

    memset(
        &server_address,
        0,
        sizeof(server_address)
    );

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((u_short)port);

    /*
     * Bind socket.
     */
    if (bind(
        server_socket,
        (struct sockaddr *)&server_address,
        sizeof(server_address)
    ) == SOCKET_ERROR)
    {
        printf(
            "Bind failed: %d\n",
            WSAGetLastError()
        );

        closesocket(server_socket);
        WSACleanup();

        return -1;
    }

    /*
     * Start listening.
     */
    if (listen(server_socket, 10) == SOCKET_ERROR)
    {
        printf(
            "Listen failed: %d\n",
            WSAGetLastError()
        );

        closesocket(server_socket);
        WSACleanup();

        return -1;
    }

    printf("\n");
    printf("============================================\n");
    printf("       INTELLICOMPRESS C HTTP SERVER\n");
    printf("============================================\n");
    printf("Server running on:\n");
    printf("http://localhost:%d\n", port);
    printf("\n");
    printf("Available endpoints:\n");
    printf("GET  /\n");
    printf("GET  /api/health\n");
    printf("POST /api/analyze\n");
    printf("POST /api/compress\n");
    printf("POST /api/decompress\n");
    printf("POST /api/compare\n");
    printf("\n");
    printf("Press Ctrl+C to stop.\n");
    printf("============================================\n");

    /*
     * Main server loop.
     */
    while (1)
    {
        SOCKET client_socket = accept(
            server_socket,
            NULL,
            NULL
        );

        if (client_socket == INVALID_SOCKET)
        {
            printf(
                "Accept failed: %d\n",
                WSAGetLastError()
            );

            continue;
        }

        handle_request(client_socket);

        closesocket(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();

    return 0;
}

#else

/*
 * Linux implementation will be added when we
 * make the backend portable for Render.
 */

int start_server(int port)
{
    (void)port;

    printf(
        "Linux server implementation pending.\n"
    );

    return -1;
}

#endif