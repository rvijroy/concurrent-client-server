#ifndef COMMON_STRUCTS_H
#define COMMON_STRUCTS_H

typedef enum RequestType
{
    ARITHMETIC,
    EVEN_OR_ODD,
    IS_PRIME,
    IS_NEGATIVE,
    UNREGISTER
} RequestType;

typedef enum ResponseCode
{
    RESPONSE_SUCCESS = 200,
    RESPONSE_UNSUPPORTED = 405,
    RESPONSE_UNKNOWN_FAILURE = 500
} ResponseCode;

typedef struct Request
{
    RequestType request_type;
    int n1, n2;
    char op;
    int client_seq_num, server_seq_num;
} Request;

typedef struct Response
{
    ResponseCode response_code;
    int client_seq_num, server_seq_num;
    int result;
} Response;

typedef struct RequestOrResponse
{
    Request req;
    Response res;
} RequestOrResponse;

#endif