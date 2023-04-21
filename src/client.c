#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "shared_memory.h"
#include "common_structs.h"
#include "file_utils.h"
#include "conn_chanel.h"

int connect_to_server(const char *client_name)
{
    queue_t *conn_q = get_queue();
    if (conn_q == NULL)
    {
        fprintf(stderr, "ERROR: Could not get connection queue.\n");
        exit(EXIT_FAILURE);
    }

    RequestOrResponse *req_or_res = post(conn_q, client_name);
    wait_until_stage(req_or_res, 1);

    return req_or_res->res.key;
}

int communicate(const char *client_name, int key)
{
    RequestOrResponse *req_or_res = get_req_or_res(client_name);
    if (req_or_res == NULL)
        return -1;

    int n1, n2;
    char op;

    // TODO: Handle req seq numbers.
    // TODO: Handle res seq numbers.

    int current_choice = 0;
    while (true)
    {
        wait_until_stage(req_or_res, 0);
        // scanf("%d", &current_choice);
        current_choice = EVEN_OR_ODD;

        if (current_choice == ARITHMETIC) // ARITHMETIC
        {
            printf("Enter two numbers: ");
            scanf("%d %d %c", &n1, &n2, &op);

            req_or_res->req.request_type = ARITHMETIC;
            req_or_res->req.n1 = n1;
            req_or_res->req.n2 = n2;
            req_or_res->req.op = op;
            next_stage(req_or_res);
        }

        else if (current_choice == EVEN_OR_ODD) // EVEN_OR_ODD
        {
            printf("Enter number to check evenness: ");
            // scanf("%d", &n1);
            n1 = 4;

            req_or_res->req.request_type = EVEN_OR_ODD;
            req_or_res->req.n1 = n1;
            next_stage(req_or_res);
        }

        else if (current_choice == IS_PRIME) // IS_PRIME
        {
            printf("Enter number to check primality: ");
            scanf("%d", &n1);

            req_or_res->req.request_type = IS_PRIME;
            req_or_res->req.n1 = n1;
            next_stage(req_or_res);
        }

        else if (current_choice == IS_NEGATIVE) // IS_NEGATIVE
        {
            printf("Enter number to check sign: ");
            scanf("%d", &n1);

            req_or_res->req.request_type = IS_NEGATIVE;
            req_or_res->req.n1 = n1;
            next_stage(req_or_res);
        }

        else if (current_choice == UNREGISTER) // UNREGISTER
        {
            req_or_res->req.request_type = UNREGISTER;
            next_stage(req_or_res);
            break;
        }

        else if (current_choice == 5) // DON'T DO ANYTHING AND EXIT
        {
            // TODO: Check if synch is required here.
            next_stage(req_or_res);
            break;
        }

        wait_until_stage(req_or_res, 2);
        if (req_or_res->res.response_code == RESPONSE_SUCCESS)
            printf("Result: %d\n", req_or_res->res.result);

        else if (req_or_res->res.response_code == RESPONSE_UNSUPPORTED)
            fprintf(stderr, "Error: Unsupported request\n");

        else if (req_or_res->res.response_code == RESPONSE_UNKNOWN_FAILURE)
            fprintf(stderr, "Error: Unknown failure\n");

        else
            fprintf(stderr, "Error: Unsupported response. Something is wrong with the server.\n");
        next_stage(req_or_res);
    }

    return 0;
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

    int key = connect_to_server(client_name);
    communicate(client_name, key);

    return 0;
}