#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUF_SIZE 1024

typedef enum
{
    GET = 1,
    POST = 2,
    PUT = 3,
    DELETE = 4
} RequestName;

typedef enum
{
    CON = 0,
    NON = 1,
    ACK = 2,
    RST = 3
} CoAPMessageType;

typedef enum
{
    REQUEST = 0,
    SUCESS_RESPONSE = 2,
    CLIENT_ERROR_RESPONSE = 4,
    SERVER_ERROR_RESPONSE = 5,
} Class;

typedef struct
{
    uint8_t tokens[8];
    uint8_t tokenLength : 4;
} Token;

typedef struct
{
    uint8_t code_class : 3; // Class (3-bit unsigned integer)
    uint8_t detail : 5;     // Detail (5-bit unsigned integer)
} Code;

typedef struct
{
    uint8_t ver : 2;  // Version (2-bit unsigned integer)
    uint8_t type : 2; // Type (2-bit unsigned integer)
    uint8_t tkl : 4;
    uint8_t code;         // Token Length (4-bit unsigned integer)
    uint16_t msg_id : 16; // Message ID (16-bit unsigned integer)
} CoAPHeader;

typedef union
{
    CoAPHeader header;
    uint8_t bytes[4];
} Header;

Header coapHeader;
Header responseHeader;
Token coapToken;
Code coapCode;

void error_handling(const char *message)
{
    perror(message);
    exit(1);
}

void decodeCoAPMessage(char *message, int message_length)
{
    // Assuming CoAP message structure is the same as defined in the Arduino code
    if (message_length < 4)
    {
        printf("Invalid CoAP message length\n");
        return;
    }

    // Extract CoAP header
    memcpy(coapHeader.bytes, message, 4);

    // Extract CoAP version
    uint8_t version = coapHeader.header.ver;
    printf("CoAP Version: %u\n", version);

    // Extract CoAP type
    CoAPMessageType type = (CoAPMessageType)coapHeader.header.type;
    printf("CoAP Type: %u\n", type);

    // Extract CoAP token length
    uint8_t tokenLength = coapHeader.header.tkl;
    printf("CoAP Token Length: %u\n", tokenLength);

    // Extract CoAP code
    coapCode.code_class = coapHeader.header.code >> 5;
    coapCode.detail = coapHeader.header.code & 0x1F;
    printf("CoAP Code Class: %u\n", coapCode.code_class);
    printf("CoAP Code Detail: %u\n", coapCode.detail);

    // Extract CoAP message ID
    uint16_t messageId = coapHeader.header.msg_id;
    printf("CoAP Message ID: %u\n", messageId);

    // Extract CoAP token
    if (tokenLength > 0)
    {
        memcpy(coapToken.tokens, message + 4, tokenLength);
        printf("CoAP Token: ");
        for (int i = 0; i < tokenLength; i++)
        {
            printf("%02X ", coapToken.tokens[i]);
        }
        printf("\n");
    }

    if (message_length > 4 + tokenLength + 1)
    {
        printf("CoAP Payload: %.*s\n", message_length - (4 + tokenLength), message + 4 + tokenLength);
    }
}

int main()
{
    int serv_sock;
    char message[BUF_SIZE];
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_sz;

    // Create UDP socket
    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    // Initialize server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    while (1)
    {
        // Receive data from client
        clnt_addr_sz = sizeof(clnt_addr);
        int str_len = recvfrom(serv_sock, message, BUF_SIZE, 0,
                               (struct sockaddr *)&clnt_addr, &clnt_addr_sz);
        if (str_len < 0)
            error_handling("recvfrom() error");

        // Decode received CoAP message here
        // Parse message according to CoAP protocol and print relevant information
        decodeCoAPMessage(message, str_len);
        printf("Received data from ESP32: %.*s\n", str_len, message);
        if (coapCode.code_class == REQUEST)
        {
            responseHeader.header.ver = coapHeader.header.ver;
            responseHeader.header.type = ACK;
            responseHeader.header.tkl = coapHeader.header.tkl;
            responseHeader.header.code = 2 << 5 | coapCode.detail;
            responseHeader.header.msg_id = coapHeader.header.msg_id;
            sendto(serv_sock, responseHeader.bytes, 4, 0, (struct sockaddr *)&clnt_addr, clnt_addr_sz);
            printf("Received CoAP request\n");
        }
        else if (coapCode.code_class == SUCESS_RESPONSE)
        {
            printf("Received CoAP success response\n");
        }
        else if (coapCode.code_class == CLIENT_ERROR_RESPONSE)
        {
            printf("Received CoAP client error response\n");
        }
        else if (coapCode.code_class == SERVER_ERROR_RESPONSE)
        {
            printf("Received CoAP server error response\n");
        }
    }

    close(serv_sock);
    return 0;
}
