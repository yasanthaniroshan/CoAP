import network
import socket

HOST = '192.168.80.8'
PORT = 8081
PATH = '2390f181-51a1-45a9-b5ce-e038f046fda0/'
SSID = 'MSI 8690'
PASSWORD = 'abcdefgh'

 
class CoAPMessageTye:
	CON = 0
	NON = 1
	ACK = 2
	RST = 3

class CoAPCode:
	request_class = 0
	detail = 0

def wifi_setup():
	wlan = network.WLAN(network.STA_IF)
	wlan.active(True)
	if not wlan.isconnected():
		print('connecting to network...')
		wlan.connect(SSID, PASSWORD)
	print('network config: ', wlan.ifconfig())


class CoAP:
	address_infromation:tuple
	client_socket:socket.socket
	data:bin
	HOST:str
	PORT:int

	def __init__(self,HOST:str,PORT:int):
		self.HOST = HOST
		self.PORT = PORT

	def connect(self):
		self.address_infromation = socket.getaddrinfo(self.HOST,self.PORT)[0][-1]
		self.client_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

	def send(self,data:str):
		self.client_socket.sendto(bin(data), self.address_infromation)

	def receive(self):
		data, addr = self.client_socket.recvfrom(1024)
		print(f'Received data: {data} with size {len(data)} bytes')


class CoAPPacket:
	version:int
	type:CoAPMessageTye
	token:int
	code:CoAPCode
	message_id:int
	payload:str
	data:bin

	def __init__(self):
		self.version = 1
		self.type = 0
		self.token = 0
		self.code = 0
		self.message_id = 0
		self.payload = ''
		self.data = b''

	def set_packet_parameters(self,version:int,type:CoAPMessageTye,token:int,code:CoAPCode,message_id:int,payload:str):
		self.version = version
		self.type = type
		self.token = token
		self.code = code
		self.message_id = message_id
		self.payload = payload

	def encode(self):
		self.data = self.version & 0b11
		self.data += self.type & 0b11
		self.data += self.token & 0b1111
		self.data += self.code.request_class & 0b111 
		self.data += self.code.detail & 0b11111
		self.data += self.message_id & 0xFFFF
		self.data += 0b11111111
		for letter in self.payload:
			self.data += ord(letter) & 0xFF
		return self.data

	def decode(self,packet:str):
		self.version = packet[0]
		self.type = packet[1]
		self.token = packet[2]
		self.code = packet[3]
		self.message_id = packet[4]
		self.payload = packet[5:]


wifi_setup()
coap_message = CoAPPacket()
coap_message.set_packet_parameters(1,CoAPMessageTye.CON,0,CoAPCode(),0,'Hello World')

coap = CoAP(HOST,PORT)
coap.connect()
coap.send(coap_message.encode())
