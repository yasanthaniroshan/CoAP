#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUF_SIZE 1024

typedef struct
{
    float temp;
    float hum;
} DHTData;

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
Token coapToken;
Code coapCode;
DHTData dhtData = {0, 0};

void error_handling(const char *message)
{
    perror(message);
    exit(1);
}

void decodeCoAPMessage(char *message, int message_length, DHTData *dhtData)
{
    // Assuming CoAP message structure is the same as defined in the Arduino code
    if (message_length < 4)
    {
        printf("Invalid CoAP message length\n");
        return;
    }
    memcpy(coapHeader.bytes, message, 4);
    coapCode.code_class = coapHeader.header.code >> 5;
    coapCode.detail = coapHeader.header.code & 0x1F;

    if (coapHeader.header.tkl > 0)
    {
        memcpy(coapToken.tokens, message + 4, coapHeader.header.tkl);
    }

    if (message_length > 4 + coapHeader.header.tkl + 1)
    {
        char *data = message + 4 + coapHeader.header.tkl + 1;
        memcpy(&dhtData->temp, data, sizeof(float));
        memcpy(&dhtData->hum, data + sizeof(float), sizeof(float));
    }
    printf("Packet size : %d bytes\n", message_length);
}

void displayData()
{
    uint16_t messageId = coapHeader.header.msg_id;
    char *type;
    switch (coapHeader.header.type)
    {
    case CON:
        type = "CON";
        break;
    case NON:
        type = "NON";
        break;
    case ACK:
        type = "ACK";
        break;
    case RST:
        type = "RST";
        break;

    default:
        break;
    }

    char *code_class;
    switch (coapCode.code_class)
    {
    case REQUEST:
        code_class = "REQ";
        break;
    case SUCESS_RESPONSE:
        code_class = "SUC_RES";
        break;
    case CLIENT_ERROR_RESPONSE:
        code_class = "CLI_ERR_RES";
        break;
    case SERVER_ERROR_RESPONSE:
        code_class = "SER_ERR_RES";
        break;

    default:
        break;
    }

    char *detail;
    switch (coapCode.detail)
    {
    case GET:
        detail = "GET";
        break;
    case POST:
        detail = "POST";
        break;
    case PUT:
        detail = "PUT";
        break;
    case DELETE:
        detail = "DELETE";
        break;

    default:
        break;
    }
    printf("---------------------------------------------------------------------------------------------------------\n");
    printf("CoAP Header => |  ");
    printf("Version : %d | ", coapHeader.header.ver);
    printf("Type : %s | ", type);
    printf("Token Length : %d | ", coapHeader.header.tkl);
    printf("Code : %s  [%s] | ", code_class, detail);
    printf("Message ID : %d |\n", messageId);
    printf("---------------------------------------------------------------------------------------------------------\n");
    if (coapHeader.header.tkl > 0)
    {
        printf("------------------\n");
        printf("Token => |  ");
        for (int i = 0; i < coapHeader.header.tkl; i++)
        {
            printf("0x%02X |\n", coapToken.tokens[i]);
        }
        printf("------------------\n");
    }
    if (dhtData.temp != 0 || dhtData.hum != 0)
    {

        printf("---------------------------------------------------------\n");
        printf("Payload => | ");
        printf("Temperature : %.2f C | ", dhtData.temp);
        printf("Humidity : %.2f %% |\n", dhtData.hum);
        printf("---------------------------------------------------------\n");
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
        Header responseHeader;
        // Decode received CoAP message here
        // Parse message according to CoAP protocol and print relevant information
        printf("Data packet arrived from ESP32 \n");
        decodeCoAPMessage(message, str_len, &dhtData);
        if (coapHeader.header.type == CON && coapCode.code_class == REQUEST)
        {
            displayData();
            responseHeader.header.ver = coapHeader.header.ver;
            responseHeader.header.type = ACK;
            responseHeader.header.tkl = coapHeader.header.tkl;
            responseHeader.header.code = coapCode.code_class | coapCode.detail;
            responseHeader.header.msg_id = coapHeader.header.msg_id;
            sendto(serv_sock, responseHeader.bytes, 4, 0, (struct sockaddr *)&clnt_addr, clnt_addr_sz);
        }
        else if (coapHeader.header.type == ACK && coapCode.code_class == SUCESS_RESPONSE)
        {
            displayData();
            responseHeader.header.ver = coapHeader.header.ver;
            responseHeader.header.type = ACK;
            responseHeader.header.tkl = coapHeader.header.tkl;
            responseHeader.header.code = 2 << 5 | GET;
            responseHeader.header.msg_id = coapHeader.header.msg_id;
            sendto(serv_sock, responseHeader.bytes, 4, 0, (struct sockaddr *)&clnt_addr, clnt_addr_sz);
        }
        else if(coapHeader.header.type == NON)
        {
            displayData();
        }
        else if (coapHeader.header.type == RST)
        {
            dhtData.temp = 0;
            dhtData.hum = 0;
        }
    }

    close(serv_sock);
    return 0;
}
