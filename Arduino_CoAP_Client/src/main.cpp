#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>

const char *ssid = "MSI 8690";
const char *password = "abcdefgh";
uint8_t data[] = {"data"};
uint8_t payloadMarker = 255;

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
  uint8_t code;    // Token Length (4-bit unsigned integer)
  uint16_t msg_id; // Message ID (16-bit unsigned integer)
} CoAPHeader;

typedef union
{
  CoAPHeader header;
  uint8_t bytes[4];
} Header;

const IPAddress ip(192, 168, 80, 8);
AsyncUDP Udp;
Header header;
Header responseHeader;

Code code = {.code_class = REQUEST, .detail = GET};

Token token;
AsyncUDPMessage message;
uint8_t tempurature_token = 0x1D;

void wifiSetup()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());
}

void udpSetup(AsyncUDP *Udp, IPAddress ip)
{
  int connectOK = Udp->connect(ip, 8081);
  Serial.println("UDP connection status: " + String(connectOK));
}

void createHeader(Header *header, CoAPMessageType type, Code *code, uint8_t tkl, uint16_t msg_id)
{
  header->header.ver = 1;
  header->header.type = type;
  header->header.tkl = tkl;
  header->header.code = code->code_class << 5 | code->detail;
  header->header.msg_id = msg_id;
}

void addHeader(AsyncUDPMessage *message, Header header)
{
  message->write(header.bytes, 4);
}

void addPayload(AsyncUDPMessage *message, char *data)
{
  for (int i = 0; data[i] != '\0'; i++)
  {
    message->write(data[i]);
  }
}

void addPayload(AsyncUDPMessage *message, String data)
{
  for (int i = 0; data[i] != '\0'; i++)
  {
    message->write((char)data[i]);
  }
}

void udpBroadcast(AsyncUDP *Udp, uint8_t data[], int size)
{
  int sentOK = 0;
  for (int i = 0; i < size; i++)
  {
    int sentOK = Udp->broadcast(&data[i], 1);
  }
  Serial.println("Data sent status: " + String(sentOK));
}

void setup()
{
  Serial.begin(115200);
  wifiSetup();
  udpSetup(&Udp, ip);
  token.tokens[0] = 0xCF;
  token.tokens[1] = 0x4A;
  token.tokenLength = 2;
  createHeader(&header, CON, &code, token.tokenLength, 0);
  message.write(header.bytes, 4);
  message.write(token.tokens, token.tokenLength);
  message.write(payloadMarker);
  Udp.send(message);
  message.flush();
}

void loop()
{
  Udp.onPacket([](AsyncUDPPacket packet)
               {
    uint8_t *data = packet.data();
    if(packet.length() < 4){
      return;
    }
    else 
    {
      memcpy(responseHeader.bytes, data, 4);
    } });

  if (responseHeader.header.code >> 5 == SUCESS_RESPONSE)
  {
    code.code_class = POST;
    code.detail = SUCESS_RESPONSE;
    createHeader(&header, CON, &code, token.tokenLength, header.header.msg_id);
    message.write(header.bytes, 4);
    message.write(token.tokens, token.tokenLength);
    message.write(payloadMarker);
    addPayload(&message, "Hello from ESP32");
    Udp.send(message);
    message.flush();
  }
  delay(5000);
}
