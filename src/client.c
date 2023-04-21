#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "shared_memory.h"
#include "common_structs.h"
#include "file_utils.h"

#define CONNECT_CHANNEL_FNAME "srv_conn_channel"
#define CONNECT_CHANNEL_SIZE (1024)

#define MAX_CLIENT_NAME_LEN (1024)
#define MAX_BUFFER_SIZE (1024)

#define UNREGISTER_STRING "unregister"

#define TEMP_CLIENT_SIZE (1024)

bool connect_to_server(const char *client_name)
{

    char *shm_connect_channel = (char *)attach_memory_block(CONNECT_CHANNEL_FNAME, CONNECT_CHANNEL_SIZE);
    if (shm_connect_channel == NULL)
    {
        fprintf(stderr, "ERROR: Could not get block %s\n", CONNECT_CHANNEL_FNAME);
        exit(EXIT_FAILURE);
    }

    // Append the new client_name to the connect channel list
    printf("Sending request to create client: %s\n", client_name);
    // TODO: Account for connect channel being empty
    sprintf(shm_connect_channel, "%s %s", shm_connect_channel, client_name);

    return 0;
}

void communicate(const char *client_name)
{
    RequestOrResponse *req_or_res = (RequestOrResponse *)attach_memory_block(client_name, sizeof(RequestOrResponse));
    if (req_or_res == NULL)
    {
        fprintf(stderr, "ERROR: Could not get block: %s\n", client_name);
        return;
    }

    int n1, n2;
    char op;

    // TODO: Handle req seq numbers.
    // TODO: Handle res seq numbers.

    int current_choice = 0;
    while (true)
    {
        // scanf("%d", &current_choice);
        current_choice = 4;

        if (current_choice == 0) // ARITHMETIC
        {
            printf("Enter two numbers: ");
            scanf("%d %d %c", &n1, &n2, &op);

            req_or_res->req.request_type = ARITHMETIC;
            req_or_res->req.n1 = n1;
            req_or_res->req.n2 = n2;
            req_or_res->req.op = op;
        }

        else if (current_choice == 1) // EVEN_OR_ODD
        {
            printf("Enter number to check evenness: ");
            scanf("%d", &n1);

            req_or_res->req.request_type = EVEN_OR_ODD;
            req_or_res->req.n1 = n1;
        }

        else if (current_choice == 2) // IS_PRIME
        {
            printf("Enter number to check primality: ");
            scanf("%d", &n1);

            req_or_res->req.request_type = IS_PRIME;
            req_or_res->req.n1 = n1;
        }

        else if (current_choice == 3) // IS_NEGATIVE
        {
            printf("Enter number to check sign: ");
            scanf("%d", &n1);

            req_or_res->req.request_type = IS_NEGATIVE;
            req_or_res->req.n1 = n1;
        }

        else if (current_choice == 4) // UNREGISTER
        {
            req_or_res->req.request_type = UNREGISTER;
            break;
        }

        else if (current_choice == 5) // DON'T DO ANYTHING AND EXIT
        {
            // TODO: Check if synch is required here.
            break;
        }

        if (req_or_res->res.response_code == RESPONSE_SUCCESS)
            printf("Result: %\n", req_or_res->res.result);

        else if (req_or_res->res.response_code == RESPONSE_UNSUPPORTED)
            fprintf(stderr, "Error: Unsupported request\n");

        else if (req_or_res->res.response_code == RESPONSE_UNKNOWN_FAILURE)
            fprintf(stderr, "Error: Unknown failure\n");

        else
            fprintf(stderr, "Error: Unsupported response. Something is wrong with the server.\n");
    }
}

int main(int argc, char **argv)
{
    // if (argc < 2)
    // {
    //     printf("Usage: ./client <unique_name_for_client>\n");
    //     exit(EXIT_FAILURE);
    // }

    char client_name[MAX_CLIENT_NAME_LEN] = "xyz420";
    // memcpy(client_name, argv[1], MAX_CLIENT_NAME_LEN);

    // If the connection file does not exist, then the server is probably not running.
    if (!does_file_exist(CONNECT_CHANNEL_FNAME))
    {
        fprintf(stderr, "Server not running. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    connect_to_server(client_name);
    communicate(client_name);

    return 0;
}