#include "../include/common.h"
#include "../include/server.h"

int main(void)
{
    printf("\n");
    printf("Starting Intellicompress...\n");

    if (start_server(8080) != 0)
    {
        fprintf(
            stderr,
            "Failed to start server.\n"
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}