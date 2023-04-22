#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "shared_memory.h"
#include "common_structs.h"
#include "utils.h"
#include "conn_chanel.h"
#include "logger.h"

int connect_to_server(const char *client_name)
{
    queue_t *conn_q = get_queue();
    if (conn_q == NULL)
    {
        logger("ERROR", "Could not get connection queue.");
        exit(EXIT_FAILURE);
    }

    logger("INFO", "Sending register request to server with name %s", client_name);
    RequestOrResponse *conn_reqres = post(conn_q, client_name);
    wait_until_stage(conn_reqres, 1);

    int key = conn_reqres->key;

    logger("INFO", "Cleaning up the personal connection channel");
    if (destroy_node(conn_reqres) == -1)
        logger("WARN", "Personal connection channel could not be cleaned up succesfully.");

    logger("INFO", "Succesfully connected to the server and received key %d", key);
    return key;
}

int communicate(const char *client_name, int key)
{
    RequestOrResponse *comm_reqres = get_req_or_res(client_name);
    if (comm_reqres == NULL)
        return -1;

    logger("DEBUG", "Obtained communication channel succesfully");

    int n1, n2;
    char op;

    int current_choice = 0;
    while (true)
    {
        wait_until_stage(comm_reqres, 0);
#ifndef DEBUGGER
        printf(
            "Options:\nArithmetic Operations: %d\nCheck even or odd: %d\nCheck prime?: %d\nCheck negative: %d\nUnregister: %d\nEnter your choice: ",
            ARITHMETIC, EVEN_OR_ODD, IS_PRIME, IS_NEGATIVE, UNREGISTER);
        scanf("%d", &current_choice);
#else
        current_choice = EVEN_OR_ODD;
#endif

        logger("DEBUG", "Sending request of type %d to server", current_choice);
        if (current_choice == ARITHMETIC)
        {
#ifndef DEBUGGER

            printf("Enter operation with format <num1> <op> <num2>: ");
            scanf("%d %c %d", &n1, &op, &n2);
#else
            n1 = 4, n2 = 5, op = '*';
#endif

            comm_reqres->req.request_type = ARITHMETIC;
            comm_reqres->req.n1 = n1;
            comm_reqres->req.n2 = n2;
            comm_reqres->req.op = op;

            logger("DEBUG", "Sending request of type %d to server with params: n1: %d op: %c n2: %d", current_choice, n1, op, n2);

            next_stage(comm_reqres);
        }

        else if (current_choice == EVEN_OR_ODD)
        {
#ifndef DEBUGGER
            printf("Enter number to check evenness: ");
            scanf("%d", &n1);
#else
            n1 = 43;
#endif

            comm_reqres->req.request_type = EVEN_OR_ODD;
            comm_reqres->req.n1 = n1;

            logger("DEBUG", "Sending request of type %d to server with params: n1: %d", current_choice, n1);

            next_stage(comm_reqres);
        }

        else if (current_choice == IS_PRIME)
        {
#ifndef DEBUGGER

            printf("Enter number to check primality: ");
            scanf("%d", &n1);
#else
            n1 = 43;
#endif
            comm_reqres->req.request_type = IS_PRIME;
            comm_reqres->req.n1 = n1;

            logger("DEBUG", "Sending request of type %d to server with params: n1: %d", current_choice, n1);

            next_stage(comm_reqres);
        }

        else if (current_choice == IS_NEGATIVE)
        {
#ifndef DEBUGGER

            printf("Enter number to check sign: ");
            scanf("%d", &n1);
#else
            n1 = -5;
#endif
            comm_reqres->req.request_type = IS_NEGATIVE;
            comm_reqres->req.n1 = n1;

            logger("DEBUG", "Sending request of type %d to server with params: n1: %d", current_choice, n1);

            next_stage(comm_reqres);
        }

        else if (current_choice == UNREGISTER)
        {
            printf("Unregistering...\n");
            logger("INFO", "Initiating unregister");
            comm_reqres->req.request_type = UNREGISTER;

            logger("DEBUG", "Sending request of type %d to server", current_choice);

            next_stage(comm_reqres);
            break;
        }

        else // DON'T DO ANYTHING AND EXIT
        {
            logger("WARN", "Input was invalid. Exiting without cleaning up or deregistering");
            next_stage(comm_reqres); // ? Fix this
            break;
        }

        wait_until_stage(comm_reqres, 2);

        logger("INFO", "Received response from server with status code: %d", comm_reqres->res.response_code);

        if (comm_reqres->res.response_code == RESPONSE_SUCCESS)
            printf("Result: %d\n", comm_reqres->res.result);

        else if (comm_reqres->res.response_code == RESPONSE_UNSUPPORTED)
            logger("ERROR", "Unsupported request. Try another.");

        else if (comm_reqres->res.response_code == RESPONSE_UNKNOWN_FAILURE)
            logger("ERROR", "Unknown failure");

        else
            logger("ERROR", "Unsupported response. Something is wrong with the server.");
        next_stage(comm_reqres);
    }

    return 0;
}

int main(int argc, char **argv)
{
#ifndef DEBUGGER
    if (argc < 2)
    {
        printf("Usage: ./client <unique_name_for_client>\n");
        exit(EXIT_FAILURE);
    }
    char client_name[MAX_CLIENT_NAME_LEN];
    memcpy(client_name, argv[1], MAX_CLIENT_NAME_LEN);
#else
    char client_name[MAX_CLIENT_NAME_LEN];
    sprintf(client_name, "xyz_%lu", (unsigned long)time(NULL));
#endif

    if (init_logger(client_name) == EXIT_FAILURE)
        return EXIT_FAILURE;

    // If the connection file does not exist, then the server is probably not running.
    int connect_channel_exists = does_file_exist(CONNECT_CHANNEL_FNAME);
    if (connect_channel_exists != 1)
    {
        logger("ERROR", "Server likely not running. Please start the server and try again or debug with other messages.");
        return EXIT_FAILURE;
    }

    int key = connect_to_server(client_name);
    communicate(client_name, key);

    close_logger();

    return 0;
}