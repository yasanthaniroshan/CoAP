#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HIEGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// #define I2C_SDA 21
// #define I2C_SCL 22

const char *ssid = "MSI 8690";
const char *password = "abcdefgh";
uint8_t data[] = {"data"};
uint8_t payloadMarker = 255;
DHT dht(15, DHT22);
bool responseSent = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HIEGHT, &Wire, OLED_RESET);

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
DHTData dhtData;

AsyncUDPMessage message;
uint8_t tempurature_token = 0x1D;
Header headerUDPPacket;

void dhtInit(DHT *dht)
{
  dht->begin();
  delay(300);
}

void displayInit(Adafruit_SSD1306 *display)
{
  Wire.begin();
  if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display->display();
}

void dhtRead(DHT *dht, DHTData *dhtData)
{
  dhtData->temp = dht->readTemperature();
  dhtData->hum = dht->readHumidity();
}

void displayData(Adafruit_SSD1306 *display, DHTData *dhtData)
{
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);
  display->println("Temp: " + String(dhtData->temp) + "C");
  display->setCursor(0, 10);
  display->println("Hum: " + String(dhtData->hum) + "%");
  display->display();
}

void displayMessage(Adafruit_SSD1306 *display, String message)
{
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 28);
  display->println(message);
  display->display();
}

void displayHeader(Adafruit_SSD1306 *display, Header *header, String ReqRes)
{
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);
  display->println("CoAP " + ReqRes + " V" + String(header->header.ver));
  display->setCursor(0, 10);
  String type;
  switch (header->header.type)
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

  display->println("Type: " + type);
  display->setCursor(0, 20);
  display->println("Token Length: " + String(header->header.tkl));
  display->setCursor(0, 30);
  String code_class;
  switch (header->header.code >> 5)
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

  String detail;
  switch (header->header.code & 0x1F)
  {
  case 1:
    detail = "GET";
    break;
  case 2:
    detail = "POST";
    break;
  case 3:
    detail = "PUT";
    break;
  case 4:
    detail = "DELETE";
    break;

  default:
    break;
  }

  display->println("Class: " + code_class);
  display->setCursor(0, 40);
  display->println("Detail: " + detail);
  display->setCursor(0, 50);
  display->println("Message ID: " + String(header->header.msg_id));
  display->display();
}

void displayToken(Adafruit_SSD1306 *display, Token *token)
{
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);
  display->println("Token: ");
  for (int i = 0; i < token->tokenLength; i++)
  {
    display->setCursor(0, 10 + i * 10);
    display->println(token->tokens[i], HEX);
  }
  display->display();
}

void displayPayload(Adafruit_SSD1306 *display, DHTData *payload)
{
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);
  display->println("Payload: ");
  display->setCursor(0, 10);
  display->println("Temp: " + String(payload->temp) + "C");
  display->setCursor(0, 20);
  display->println("Hum: " + String(payload->hum) + "%");
  display->display();
}

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

void addPayload(AsyncUDPMessage *message, DHTData *data)
{
  uint8_t float_size = sizeof(float);
  Serial.println("Float size: " + String(float_size));
  for (int i = 0; i < float_size; i++)
  {
    message->write(((uint8_t *)&data->temp)[i]);
  }
  for (int i = 0; i < float_size; i++)
  {
    message->write(((uint8_t *)&data->hum)[i]);
  }
}

void sendCONREQ(AsyncUDP *Udp)
{
  AsyncUDPMessage message;
  Code code = {.code_class = REQUEST, .detail = GET};
  Header requestHeader;
  createHeader(&requestHeader, CON, &code, 0, 0);
  displayHeader(&display, &requestHeader, "REQ");
  delay(2000);
  message.write(requestHeader.bytes, 4);
  message.write(payloadMarker);
  displayMessage(&display, "Sending Data..");
  Udp->send(message);
  message.flush();
}

void sendACKREQ(AsyncUDP *Udp, Header *requestHeader)
{
  AsyncUDPMessage message;
  Code code = {.code_class = SUCESS_RESPONSE, .detail = POST};
  Token token = {.tokens = {tempurature_token}, .tokenLength = 1};
  Header responseHeader;
  createHeader(&responseHeader, ACK, &code, token.tokenLength, requestHeader->header.msg_id++);
  displayHeader(&display, &responseHeader, "RES");
  delay(2000);
  message.write(responseHeader.bytes, 4);
  message.write(token.tokens, token.tokenLength);
  displayToken(&display, &token);
  delay(2000);
  message.write(payloadMarker);
  dhtRead(&dht, &dhtData);
  displayPayload(&display, &dhtData);
  delay(2000);
  addPayload(&message, &dhtData);
  displayMessage(&display, "Sending Data..");
  Udp->send(message);
  message.flush();
  delay(2000);
}

void sendACKSUCRES(AsyncUDP *Udp, Header *requestHeader)
{
  AsyncUDPMessage message;
  Code code = {.code_class = SUCESS_RESPONSE, .detail = POST};
  Token token = {.tokens = {tempurature_token}, .tokenLength = 1};
  Header responseHeader;
  int messageID = requestHeader->header.msg_id;
  messageID++;
  createHeader(&responseHeader, ACK, &code, token.tokenLength, messageID);
  displayHeader(&display, &responseHeader, "RES");
  delay(2000);
  message.write(responseHeader.bytes, 4);
  message.write(token.tokens, token.tokenLength);
  displayToken(&display, &token);
  delay(2000);
  message.write(payloadMarker);
  dhtRead(&dht, &dhtData);
  displayPayload(&display, &dhtData);
  delay(2000);
  addPayload(&message, &dhtData);
  displayMessage(&display, "Sending Data..");
  Udp->send(message);
  Serial.println("Message size: " + String(message.length()));
  message.flush();
}

void sendNONRES(AsyncUDP *Udp)
{
  AsyncUDPMessage message;
  Code code = {.code_class = SUCESS_RESPONSE, .detail = POST};
  Token token = {.tokens = {tempurature_token}, .tokenLength = 1};
  Header responseHeader;
  createHeader(&responseHeader, NON, &code, token.tokenLength, 0);
  displayHeader(&display, &responseHeader, "RES");
  delay(2000);
  message.write(responseHeader.bytes, 4);
  message.write(token.tokens, token.tokenLength);
  displayToken(&display, &token);
  message.write(payloadMarker);
  dhtRead(&dht, &dhtData);
  displayPayload(&display, &dhtData);
  delay(2000);
  addPayload(&message, &dhtData);
  displayMessage(&display, "Sending Data..");
  Udp->send(message);
  message.flush();
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


Header decodeUDPData(AsyncUDPPacket packet, Adafruit_SSD1306 *display)
{
  Header requestHeader;
  Code code;
  Token token;
  uint8_t *data = packet.data();
  if (packet.length() < 4)
  {
    displayMessage(display, "Invalid Data");
    delay(2000);
  }
  else
  {
    memcpy(requestHeader.bytes, data, 4);
    displayHeader(display, &requestHeader, "REQ");
    delay(2000);
  }
  code.code_class = requestHeader.header.code >> 5;
  code.detail = requestHeader.header.code & 0x1F;
  return requestHeader;
}

void setup()
{
  Serial.begin(115200);
  displayInit(&display);
  wifiSetup();
  dhtInit(&dht);
  udpSetup(&Udp, ip);
  displayMessage(&display, "Initilizing..");
  dhtRead(&dht, &dhtData);
  displayData(&display, &dhtData);
  delay(2000);
  sendCONREQ(&Udp);
  // sendNONRES(&Udp);
}

void loop()
{

  Udp.onPacket([](AsyncUDPPacket packet)
               {
    Header requestHeader = decodeUDPData(packet, &display);
    displayHeader(&display, &requestHeader, "REQ");
    Code code = {.code_class = requestHeader.header.code >> 5, .detail = requestHeader.header.code & 0x1F};
    if(requestHeader.header.type == ACK && code.code_class == REQUEST)
    {
      sendACKREQ(&Udp, &requestHeader);
    }
    else if(requestHeader.header.type == ACK && code.code_class == SUCESS_RESPONSE)
    {
      sendACKSUCRES(&Udp, &requestHeader);
    }
    else if(requestHeader.header.type == RST)
    {
      displayMessage(&display, "Reset");
      delay(2000);
      sendCONREQ(&Udp);
    }
    else if(requestHeader.header.type == NON)
    {
      displayMessage(&display, "Non Confirmable");
      delay(2000);
      sendNONRES(&Udp);
    } });

}
