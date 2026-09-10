#include "../include/server.h"
#include "../include/huffman.h"
#include "../include/rle.h"
#include "../include/lzw.h"
#include "../include/analyzer.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define INITIAL_BUFFER_SIZE 65536
#define MAX_REQUEST_SIZE (50 * 1024 * 1024)

#define TEMP_INPUT  "server_input.tmp"
#define TEMP_OUTPUT "server_output.tmp"


/* ============================================================
   Utility Functions
   ============================================================ */

static void send_response(
    SOCKET client,
    int status_code,
    const char *status_text,
    const char *content_type,
    const char *body
)
{
    char header[1024];

    size_t body_length = body ? strlen(body) : 0;

    int header_length = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code,
        status_text,
        content_type,
        body_length
    );

    send(client, header, header_length, 0);

    if (body && body_length > 0) {
        send(client, body, (int)body_length, 0);
    }
}


static void send_binary_response(
    SOCKET client,
    int status_code,
    const char *status_text,
    const char *content_type,
    const unsigned char *data,
    size_t data_length
)
{
    char header[1024];

    int header_length = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code,
        status_text,
        content_type,
        data_length
    );

    send(client, header, header_length, 0);

    size_t total_sent = 0;

    while (total_sent < data_length) {
        int chunk = (int)(data_length - total_sent);

        if (chunk > 8192) {
            chunk = 8192;
        }

        int sent = send(
            client,
            (const char *)(data + total_sent),
            chunk,
            0
        );

        if (sent <= 0) {
            break;
        }

        total_sent += sent;
    }
}


static int send_file(
    SOCKET client,
    const char *filename,
    const char *content_type
)
{
    FILE *file = fopen(filename, "rb");

    if (!file) {
        send_response(
            client,
            500,
            "Internal Server Error",
            "text/plain",
            "Could not open output file."
        );

        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size_long = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size_long < 0) {
        fclose(file);

        send_response(
            client,
            500,
            "Internal Server Error",
            "text/plain",
            "Could not determine file size."
        );

        return -1;
    }

    size_t file_size = (size_t)file_size_long;

    unsigned char *buffer = NULL;

    if (file_size > 0) {
        buffer = (unsigned char *)malloc(file_size);

        if (!buffer) {
            fclose(file);

            send_response(
                client,
                500,
                "Internal Server Error",
                "text/plain",
                "Memory allocation failed."
            );

            return -1;
        }

        size_t read_count = fread(buffer, 1, file_size, file);

        if (read_count != file_size) {
            free(buffer);
            fclose(file);

            send_response(
                client,
                500,
                "Internal Server Error",
                "text/plain",
                "Could not read output file."
            );

            return -1;
        }
    }

    fclose(file);

    send_binary_response(
        client,
        200,
        "OK",
        content_type,
        buffer,
        file_size
    );

    free(buffer);

    return 0;
}


/* ============================================================
   Binary-Safe Searching
   ============================================================ */

/*
   Search for a byte sequence inside a buffer.

   Unlike strstr(), this function DOES NOT stop at '\0'.

   This is important because compressed data may legitimately
   contain zero bytes.
*/
static unsigned char *find_bytes(
    unsigned char *buffer,
    size_t buffer_size,
    const unsigned char *pattern,
    size_t pattern_size
)
{
    if (!buffer || !pattern || pattern_size == 0) {
        return NULL;
    }

    if (buffer_size < pattern_size) {
        return NULL;
    }

    for (size_t i = 0; i <= buffer_size - pattern_size; i++) {

        if (memcmp(buffer + i, pattern, pattern_size) == 0) {
            return buffer + i;
        }
    }

    return NULL;
}


/*
   Find:

       \r\n\r\n

   which marks the end of HTTP headers.
*/
static unsigned char *find_header_end(
    unsigned char *buffer,
    size_t buffer_size
)
{
    const unsigned char marker[] = "\r\n\r\n";

    return find_bytes(
        buffer,
        buffer_size,
        marker,
        sizeof(marker) - 1
    );
}


/* ============================================================
   HTTP Header Parsing
   ============================================================ */

static long get_content_length(
    unsigned char *request,
    size_t header_size
)
{
    const char *key = "Content-Length:";

    unsigned char *position = find_bytes(
        request,
        header_size,
        (const unsigned char *)key,
        strlen(key)
    );

    if (!position) {
        return -1;
    }

    position += strlen(key);

    while ((size_t)(position - request) < header_size &&
           (*position == ' ' || *position == '\t')) {
        position++;
    }

    char number[32];

    size_t i = 0;

    while ((size_t)(position - request) < header_size &&
           i < sizeof(number) - 1 &&
           *position >= '0' &&
           *position <= '9') {

        number[i++] = (char)*position;
        position++;
    }

    number[i] = '\0';

    if (i == 0) {
        return -1;
    }

    return atol(number);
}


static int get_boundary(
    unsigned char *request,
    size_t header_size,
    char *boundary,
    size_t boundary_size
)
{
    const char *key = "boundary=";

    unsigned char *position = find_bytes(
        request,
        header_size,
        (const unsigned char *)key,
        strlen(key)
    );

    if (!position) {
        return -1;
    }

    position += strlen(key);

    size_t remaining =
        header_size - (size_t)(position - request);

    size_t i = 0;

    /*
       Boundary may be surrounded by quotes:

       boundary="----WebKitFormBoundary..."
    */

    if (remaining > 0 && *position == '"') {
        position++;
        remaining--;

        while (i < remaining &&
               i < boundary_size - 1 &&
               position[i] != '"') {

            boundary[i] = (char)position[i];
            i++;
        }
    }
    else {
        while (i < remaining &&
               i < boundary_size - 1 &&
               position[i] != '\r' &&
               position[i] != '\n' &&
               position[i] != ';' &&
               position[i] != ' ') {

            boundary[i] = (char)position[i];
            i++;
        }
    }

    boundary[i] = '\0';

    return (i > 0) ? 0 : -1;
}


/* ============================================================
   Multipart Form Data
   ============================================================ */

/*
   Extract a normal text field such as:

       algorithm=huffman

   from multipart/form-data.
*/
static int extract_text_field(
    unsigned char *body,
    size_t body_size,
    const char *boundary,
    const char *field_name,
    char *output,
    size_t output_size
)
{
    if (!body || !boundary || !field_name ||
        !output || output_size == 0) {
        return -1;
    }

    char field_marker[256];

    snprintf(
        field_marker,
        sizeof(field_marker),
        "name=\"%s\"",
        field_name
    );

    unsigned char *field = find_bytes(
        body,
        body_size,
        (const unsigned char *)field_marker,
        strlen(field_marker)
    );

    if (!field) {
        return -1;
    }

    size_t remaining =
        body_size - (size_t)(field - body);

    const unsigned char header_end[] = "\r\n\r\n";

    unsigned char *data_start = find_bytes(
        field,
        remaining,
        header_end,
        sizeof(header_end) - 1
    );

    if (!data_start) {
        return -1;
    }

    data_start += 4;

    size_t remaining_data =
        body_size - (size_t)(data_start - body);

    /*
       The next boundary starts with:

           \r\n--<boundary>
    */

    char boundary_marker[512];

    snprintf(
        boundary_marker,
        sizeof(boundary_marker),
        "\r\n--%s",
        boundary
    );

    unsigned char *data_end = find_bytes(
        data_start,
        remaining_data,
        (const unsigned char *)boundary_marker,
        strlen(boundary_marker)
    );

    if (!data_end) {
        return -1;
    }

    size_t data_length =
        (size_t)(data_end - data_start);

    if (data_length >= output_size) {
        data_length = output_size - 1;
    }

    memcpy(output, data_start, data_length);
    output[data_length] = '\0';

    return 0;
}


/*
   Extract uploaded file from multipart/form-data.

   IMPORTANT:
   This function is binary-safe.

   It does NOT use strstr() on the uploaded file.
*/
static int extract_uploaded_file(
    unsigned char *body,
    size_t body_size,
    const char *boundary,
    const char *field_name,
    const char *output_filename
)
{
    if (!body || !boundary || !field_name || !output_filename) {
        return -1;
    }

    char field_marker[256];

    snprintf(
        field_marker,
        sizeof(field_marker),
        "name=\"%s\"",
        field_name
    );

    unsigned char *field = find_bytes(
        body,
        body_size,
        (const unsigned char *)field_marker,
        strlen(field_marker)
    );

    if (!field) {
        return -1;
    }

    size_t remaining =
        body_size - (size_t)(field - body);

    const unsigned char header_end[] = "\r\n\r\n";

    unsigned char *file_data_start = find_bytes(
        field,
        remaining,
        header_end,
        sizeof(header_end) - 1
    );

    if (!file_data_start) {
        return -1;
    }

    file_data_start += 4;

    size_t remaining_data =
        body_size - (size_t)(file_data_start - body);

    char boundary_marker[512];

    snprintf(
        boundary_marker,
        sizeof(boundary_marker),
        "\r\n--%s",
        boundary
    );

    unsigned char *file_data_end = find_bytes(
        file_data_start,
        remaining_data,
        (const unsigned char *)boundary_marker,
        strlen(boundary_marker)
    );

    if (!file_data_end) {
        return -1;
    }

    size_t file_size =
        (size_t)(file_data_end - file_data_start);

    FILE *file = fopen(output_filename, "wb");

    if (!file) {
        return -1;
    }

    size_t written = fwrite(
        file_data_start,
        1,
        file_size,
        file
    );

    fclose(file);

    if (written != file_size) {
        return -1;
    }

    return 0;
}


/* ============================================================
   Compression / Decompression
   ============================================================ */

static int run_compression(const char *algorithm)
{
    if (!algorithm) {
        return -1;
    }

    if (strcmp(algorithm, "huffman") == 0) {

        return compress_huffman(
            TEMP_INPUT,
            TEMP_OUTPUT
        );
    }

    if (strcmp(algorithm, "rle") == 0) {

        return compress_rle(
            TEMP_INPUT,
            TEMP_OUTPUT
        );
    }

    if (strcmp(algorithm, "lzw") == 0) {

        return compress_lzw(
            TEMP_INPUT,
            TEMP_OUTPUT
        );
    }

    return -1;
}


static int run_decompression(const char *algorithm)
{
    if (!algorithm) {
        return -1;
    }

    if (strcmp(algorithm, "huffman") == 0) {

        return decompress_huffman(
            TEMP_INPUT,
            TEMP_OUTPUT
        );
    }

    if (strcmp(algorithm, "rle") == 0) {

        return decompress_rle(
            TEMP_INPUT,
            TEMP_OUTPUT
        );
    }

    if (strcmp(algorithm, "lzw") == 0) {

        return decompress_lzw(
            TEMP_INPUT,
            TEMP_OUTPUT
        );
    }

    return -1;
}


/* ============================================================
   File Size Helper
   ============================================================ */

static long get_file_size(const char *filename)
{
    FILE *file = fopen(filename, "rb");

    if (!file) {
        return -1;
    }

    fseek(file, 0, SEEK_END);

    long size = ftell(file);

    fclose(file);

    return size;
}


/* ============================================================
   Request Reading
   ============================================================ */

/*
   Read the COMPLETE HTTP request.

   Old approach:

       recv(...)
       handle_request(...)

   was unsafe because one recv() does not necessarily contain
   the entire request.

   This version:

       1. Reads until HTTP headers are complete.
       2. Reads Content-Length.
       3. Continues receiving until the complete body arrives.
*/
static unsigned char *receive_request(
    SOCKET client,
    size_t *request_size
)
{
    size_t capacity = INITIAL_BUFFER_SIZE;
    size_t received = 0;

    unsigned char *buffer =
        (unsigned char *)malloc(capacity);

    if (!buffer) {
        return NULL;
    }

    size_t header_size = 0;
    long content_length = -1;

    while (1) {

        if (received == capacity) {

            if (capacity >= MAX_REQUEST_SIZE) {
                free(buffer);
                return NULL;
            }

            size_t new_capacity = capacity * 2;

            if (new_capacity > MAX_REQUEST_SIZE) {
                new_capacity = MAX_REQUEST_SIZE;
            }

            unsigned char *new_buffer =
                (unsigned char *)realloc(
                    buffer,
                    new_capacity
                );

            if (!new_buffer) {
                free(buffer);
                return NULL;
            }

            buffer = new_buffer;
            capacity = new_capacity;
        }

        int bytes = recv(
            client,
            (char *)(buffer + received),
            (int)(capacity - received),
            0
        );

        if (bytes <= 0) {
            break;
        }

        received += (size_t)bytes;

        if (header_size == 0) {

            unsigned char *header_end =
                find_header_end(
                    buffer,
                    received
                );

            if (header_end) {

                header_size =
                    (size_t)(header_end - buffer) + 4;

                content_length =
                    get_content_length(
                        buffer,
                        header_size
                    );

                /*
                   GET / OPTIONS normally do not have a body.
                */
                if (content_length <= 0) {
                    break;
                }
            }
        }

        if (header_size > 0 &&
            content_length >= 0) {

            size_t expected_size =
                header_size + (size_t)content_length;

            if (received >= expected_size) {
                break;
            }

            if (expected_size > MAX_REQUEST_SIZE) {
                free(buffer);
                return NULL;
            }

            if (expected_size > capacity) {

                size_t new_capacity = capacity;

                while (new_capacity < expected_size) {
                    new_capacity *= 2;

                    if (new_capacity > MAX_REQUEST_SIZE) {
                        new_capacity = MAX_REQUEST_SIZE;
                        break;
                    }
                }

                unsigned char *new_buffer =
                    (unsigned char *)realloc(
                        buffer,
                        new_capacity
                    );

                if (!new_buffer) {
                    free(buffer);
                    return NULL;
                }

                buffer = new_buffer;
                capacity = new_capacity;
            }
        }
    }

    *request_size = received;

    return buffer;
}


/* ============================================================
   Request Handler
   ============================================================ */

static void handle_request(
    SOCKET client,
    unsigned char *request,
    size_t request_size
)
{
    unsigned char *header_end =
        find_header_end(
            request,
            request_size
        );

    if (!header_end) {

        send_response(
            client,
            400,
            "Bad Request",
            "text/plain",
            "Invalid HTTP request."
        );

        return;
    }

    size_t header_size =
        (size_t)(header_end - request) + 4;

    /*
       Make a temporary null-terminated copy of the HTTP headers.

       This is safe because headers are text.
    */
    char *headers =
        (char *)malloc(header_size + 1);

    if (!headers) {

        send_response(
            client,
            500,
            "Internal Server Error",
            "text/plain",
            "Memory allocation failed."
        );

        return;
    }

    memcpy(headers, request, header_size);
    headers[header_size] = '\0';

    /* --------------------------------------------------------
       Extract HTTP method and path
       -------------------------------------------------------- */

    char method[16];
    char path[256];

    method[0] = '\0';
    path[0] = '\0';

    sscanf(
        headers,
        "%15s %255s",
        method,
        path
    );

    printf(
        "[HTTP] %s %s\n",
        method,
        path
    );

    /* --------------------------------------------------------
       OPTIONS
       -------------------------------------------------------- */

    if (strcmp(method, "OPTIONS") == 0) {

        send_response(
            client,
            200,
            "OK",
            "text/plain",
            ""
        );

        free(headers);
        return;
    }

    /* --------------------------------------------------------
       Body
       -------------------------------------------------------- */

    unsigned char *body =
        request + header_size;

    size_t body_size =
        request_size >= header_size
            ? request_size - header_size
            : 0;

    /* ========================================================
       /api/analyze
       ======================================================== */

    if (strcmp(path, "/api/analyze") == 0 &&
        strcmp(method, "POST") == 0) {

        char boundary[512];

        if (get_boundary(request, header_size, boundary, sizeof(boundary)) != 0) {
            send_response(client, 400, "Bad Request", "text/plain",
                          "Multipart boundary not found.");
            free(headers);
            return;
        }

        if (extract_uploaded_file(body, body_size, boundary, "file", TEMP_INPUT) != 0) {
            send_response(client, 400, "Bad Request", "text/plain",
                          "Could not extract uploaded file.");
            free(headers);
            return;
        }

        FILE *file = fopen(TEMP_INPUT, "rb");
        if (!file) {
            send_response(client, 500, "Internal Server Error", "text/plain",
                          "Could not open uploaded file.");
            free(headers);
            return;
        }

        fseek(file, 0, SEEK_END);
        long file_size_long = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size_long <= 0) {
            fclose(file);
            send_response(client, 400, "Bad Request", "text/plain",
                          "Input file is empty.");
            free(headers);
            return;
        }

        size_t file_size = (size_t)file_size_long;
        unsigned char *data = (unsigned char *)malloc(file_size);

        if (!data) {
            fclose(file);
            send_response(client, 500, "Internal Server Error", "text/plain",
                          "Memory allocation failed.");
            free(headers);
            return;
        }

        size_t read_count = fread(data, 1, file_size, file);
        fclose(file);

        if (read_count != file_size) {
            free(data);
            send_response(client, 500, "Internal Server Error", "text/plain",
                          "Could not read uploaded file.");
            free(headers);
            return;
        }

        AnalysisResult result = analyze_file(data, file_size);

        const char *recommendation = "None";
        const char *explanation = "No recommendation available.";

        switch (result.recommendation) {
            case 1:
                recommendation = "RLE";
                explanation = "RLE is recommended because the file contains many consecutive repeated bytes.";
                break;
            case 2:
                recommendation = "Huffman";
                explanation = "Huffman coding is recommended because the file has significant statistical redundancy.";
                break;
            case 3:
                recommendation = "LZW";
                explanation = "LZW is recommended because repeated patterns can be represented efficiently.";
                break;
        }

        char response[2048];
        snprintf(response, sizeof(response),
            "{\"analysis\":{"
            "\"entropy\":%.6f,"
            "\"redundancy\":%.6f,"
            "\"consecutive_ratio\":%.6f,"
            "\"recommendation\":\"%s\","
            "\"confidence\":%d,"
            "\"char_count\":%zu,"
            "\"unique_chars\":%zu,"
            "\"explanation\":\"%s\"},"
            "\"tree\":null}",
            result.entropy, result.redundancy, result.consecutive_ratio,
            recommendation, result.confidence, result.char_count,
            result.unique_chars, explanation);

        free(data);
        send_response(client, 200, "OK", "application/json", response);
        free(headers);
        return;
    }

    /* ========================================================
       Multipart Boundary
       ======================================================== */

    char boundary[512];

    if (get_boundary(
            request,
            header_size,
            boundary,
            sizeof(boundary)
        ) != 0) {

        send_response(
            client,
            400,
            "Bad Request",
            "text/plain",
            "Multipart boundary not found."
        );

        free(headers);
        return;
    }

    /* ========================================================
       /api/compress
       ======================================================== */

    if (strcmp(path, "/api/compress") == 0 &&
        strcmp(method, "POST") == 0) {

        char algorithm[64];

        if (extract_text_field(
                body,
                body_size,
                boundary,
                "algorithm",
                algorithm,
                sizeof(algorithm)
            ) != 0) {

            send_response(
                client,
                400,
                "Bad Request",
                "text/plain",
                "Algorithm field not found."
            );

            free(headers);
            return;
        }

        printf(
            "[COMPRESS] Algorithm: %s\n",
            algorithm
        );

        if (extract_uploaded_file(
                body,
                body_size,
                boundary,
                "file",
                TEMP_INPUT
            ) != 0) {

            send_response(
                client,
                400,
                "Bad Request",
                "text/plain",
                "Could not extract uploaded file."
            );

            free(headers);
            return;
        }

        printf(
            "[COMPRESS] Input size: %ld bytes\n",
            get_file_size(TEMP_INPUT)
        );

        remove(TEMP_OUTPUT);

        clock_t start = clock();

        int result =
            run_compression(algorithm);

        clock_t end = clock();

        if (result != 0) {

            printf(
                "[COMPRESS] Algorithm failed.\n"
            );

            send_response(
                client,
                500,
                "Internal Server Error",
                "text/plain",
                "Compression failed."
            );

            free(headers);
            return;
        }

        double elapsed =
            (double)(end - start) /
            CLOCKS_PER_SEC;

        printf(
            "[COMPRESS] Output size: %ld bytes\n",
            get_file_size(TEMP_OUTPUT)
        );

        printf(
            "[COMPRESS] Time: %.6f seconds\n",
            elapsed
        );

        send_file(
            client,
            TEMP_OUTPUT,
            "application/octet-stream"
        );

        free(headers);
        return;
    }

    /* ========================================================
       /api/decompress
       ======================================================== */

    if (strcmp(path, "/api/decompress") == 0 &&
        strcmp(method, "POST") == 0) {

        char algorithm[64];

        if (extract_text_field(
                body,
                body_size,
                boundary,
                "algorithm",
                algorithm,
                sizeof(algorithm)
            ) != 0) {

            send_response(
                client,
                400,
                "Bad Request",
                "text/plain",
                "Algorithm field not found."
            );

            free(headers);
            return;
        }

        printf(
            "[DECOMPRESS] Algorithm: %s\n",
            algorithm
        );

        if (extract_uploaded_file(
                body,
                body_size,
                boundary,
                "file",
                TEMP_INPUT
            ) != 0) {

            send_response(
                client,
                400,
                "Bad Request",
                "text/plain",
                "Could not extract uploaded file."
            );

            free(headers);
            return;
        }

        printf(
            "[DECOMPRESS] Input size: %ld bytes\n",
            get_file_size(TEMP_INPUT)
        );

        remove(TEMP_OUTPUT);

        clock_t start = clock();

        int result =
            run_decompression(algorithm);

        clock_t end = clock();

        if (result != 0) {

            printf(
                "[DECOMPRESS] Algorithm failed.\n"
            );

            send_response(
                client,
                500,
                "Internal Server Error",
                "text/plain",
                "Decompression failed."
            );

            free(headers);
            return;
        }

        double elapsed =
            (double)(end - start) /
            CLOCKS_PER_SEC;

        printf(
            "[DECOMPRESS] Output size: %ld bytes\n",
            get_file_size(TEMP_OUTPUT)
        );

        printf(
            "[DECOMPRESS] Time: %.6f seconds\n",
            elapsed
        );

        send_file(
            client,
            TEMP_OUTPUT,
            "text/plain"
        );

        free(headers);
        return;
    }

    /* ========================================================
       /api/compare
       ======================================================== */

    if (strcmp(path, "/api/compare") == 0 &&
        strcmp(method, "POST") == 0) {

        if (extract_uploaded_file(
                body,
                body_size,
                boundary,
                "file",
                TEMP_INPUT
            ) != 0) {

            send_response(
                client,
                400,
                "Bad Request",
                "text/plain",
                "Could not extract uploaded file."
            );

            free(headers);
            return;
        }

        long original_size =
            get_file_size(TEMP_INPUT);

        if (original_size <= 0) {

            send_response(
                client,
                400,
                "Bad Request",
                "text/plain",
                "Input file is empty."
            );

            free(headers);
            return;
        }

        const char *algorithms[] = {
            "huffman",
            "rle",
            "lzw"
        };

        char response[4096];

        strcpy(
            response,
            "{\"comparison\":["
        );

        for (int i = 0; i < 3; i++) {

            const char *algorithm =
                algorithms[i];

            remove(TEMP_OUTPUT);

            clock_t start = clock();

            int result =
                run_compression(algorithm);

            clock_t end = clock();

            if (result != 0) {
                continue;
            }

            long compressed_size =
                get_file_size(TEMP_OUTPUT);

            double ratio =
                (double)compressed_size /
                (double)original_size;

            double time_taken =
                (double)(end - start) /
                CLOCKS_PER_SEC;

            char item[512];

            snprintf(
                item,
                sizeof(item),
                "%s{\"algorithm\":\"%s\","
                "\"ratio\":%.6f,"
                "\"time_taken\":%.6f}",
                i == 0 ? "" : ",",
                algorithm,
                ratio,
                time_taken
            );

            strcat(response, item);
        }

        strcat(response, "]}");

        send_response(
            client,
            200,
            "OK",
            "application/json",
            response
        );

        free(headers);
        return;
    }

    /* ========================================================
       Unknown Route
       ======================================================== */

    send_response(
        client,
        404,
        "Not Found",
        "text/plain",
        "Endpoint not found."
    );

    free(headers);
}


/* ============================================================
   HTTP Server
   ============================================================ */

int start_http_server(int port)
{
    WSADATA wsa_data;

    int result =
        WSAStartup(
            MAKEWORD(2, 2),
            &wsa_data
        );

    if (result != 0) {

        printf(
            "WSAStartup failed: %d\n",
            result
        );

        return 1;
    }

    SOCKET server_socket =
        socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
        );

    if (server_socket == INVALID_SOCKET) {

        printf(
            "Socket creation failed.\n"
        );

        WSACleanup();

        return 1;
    }

    int opt = 1;

    setsockopt(
        server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (const char *)&opt,
        sizeof(opt)
    );

    struct sockaddr_in server_address;

    memset(
        &server_address,
        0,
        sizeof(server_address)
    );

    server_address.sin_family =
        AF_INET;

    server_address.sin_addr.s_addr =
        htonl(INADDR_ANY);

    server_address.sin_port =
        htons((u_short)port);

    if (bind(
            server_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) == SOCKET_ERROR) {

        printf(
            "Bind failed. Error: %d\n",
            WSAGetLastError()
        );

        closesocket(server_socket);
        WSACleanup();

        return 1;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {

        printf(
            "Listen failed. Error: %d\n",
            WSAGetLastError()
        );

        closesocket(server_socket);
        WSACleanup();

        return 1;
    }

    printf("\n");
    printf("============================================\n");
    printf("   Intellicompress C HTTP Server\n");
    printf("============================================\n");
    printf("Server running on port %d\n", port);
    printf("\n");
    printf("Endpoints:\n");
    printf("  POST /api/analyze\n");
    printf("  POST /api/compress\n");
    printf("  POST /api/decompress\n");
    printf("  POST /api/compare\n");
    printf("============================================\n");
    printf("\n");

    while (1) {

        struct sockaddr_in client_address;

        int client_size =
            sizeof(client_address);

        SOCKET client_socket =
            accept(
                server_socket,
                (struct sockaddr *)&client_address,
                &client_size
            );

        if (client_socket == INVALID_SOCKET) {

            printf(
                "Accept failed. Error: %d\n",
                WSAGetLastError()
            );

            continue;
        }

        size_t request_size = 0;

        unsigned char *request =
            receive_request(
                client_socket,
                &request_size
            );

        if (!request) {

            send_response(
                client_socket,
                400,
                "Bad Request",
                "text/plain",
                "Could not read HTTP request."
            );

            closesocket(client_socket);
            continue;
        }

        printf(
            "[HTTP] Received %zu bytes\n",
            request_size
        );

        handle_request(
            client_socket,
            request,
            request_size
        );

        free(request);

        closesocket(client_socket);

        /*
           Temporary files are removed after every request.

           This prevents one request from accidentally affecting
           another request.
        */
        remove(TEMP_INPUT);
        remove(TEMP_OUTPUT);
    }

    closesocket(server_socket);
    WSACleanup();

    return 0;
}