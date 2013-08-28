/*
 * UIPEthernet Packet Reply example.
 *
 * UIPEthernet is a TCP/IP stack that can be used with a enc28j60 based
 * Ethernet-shield.
 *
 * UIPEthernet uses the fine uIP stack by Adam Dunkels <adam@sics.se>
 *
 *      -----------------
 *
 * This packet reply example listens on port 885 for packets that have 4 bytes
 * big-endian length in front of them and then returns them back to sender.
 *
 * Example packet: 0x03 0x00 0x00 0x00 0x48 0x69 0x21
 * This translates to: length of packet - 3 bytes
 *                     payload - 0x48 0x69 0x21 (ASCII Hi!)
 *
 * Copyright (C) 2013 Jaroslav Peska <peska.jaroslav@gmail.com 
 */

#include <util.h>

#include <UIPEthernet.h>
// The connection_data struct needs to be defined in an external file.
#include <UIPServer.h>
#include <UIPClient.h>

UIPServer server = UIPServer(885);

void setup()
{
  Serial.begin(9600);

  UIPEthernet.set_uip_callback(&UIPClient::uip_callback);

  uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
  IPAddress myIP(192,168,1,136);

  UIPEthernet.begin(mac,myIP);

  server.begin();
}

void loop()
{
  server.foreach(Handler);
}

int ReceivePacket(UIPClient& client, uint8_t* buffer)
{
  uint8_t packet_length_network_bytes[4];
  uint32_t packet_length_network = 0, packet_length;

  if(client.available() < 4) return -1; // We don't have packet length yet

  for(int i = 0; i < 4; ++i)
  {
    packet_length_network_bytes[i] = client.peek(i);
  }

  for(int i = 0; i < 4; ++i)
  {
    packet_length_network |= ((uint32_t) packet_length_network_bytes[i]) << (24 - i*8);
  }

  packet_length = ntohl(packet_length_network);

  if(client.available() < packet_length + 4)
  {
    return -1; // Packet isn't completely in buffer
  }
  else
  {
    int bytes_read;
    // Read the packet length and throw it away, since we already know the value
    for(int i = 0; i < 4; ++i) client.read();

    // Read the packet and store it in the buffer.
    bytes_read = client.read(buffer, packet_length);
    return bytes_read;
  }
}

void SendPacket(UIPClient& client, const uint8_t* buffer, const uint32_t length)
{
  uint32_t packet_length_network;
  uint8_t packet_length_network_bytes[4];

  packet_length_network = htonl(length);

  for(int i = 0; i < 4; ++i)
  {
    packet_length_network_bytes[i] = (uint8_t) (packet_length_network >> (24 - i*8));
  }

  client.write(packet_length_network_bytes, 4);
  client.write(buffer, length);
}

void Handler(UIPClient& client)
{
  uint8_t buffer[100];
  int packet_length = ReceivePacket(client, buffer);
  if(packet_length >= 0)
  {
    Serial.println(F("Received packet"));
    Serial.print(F("Packet length: "));
    Serial.println(packet_length, DEC);
    SendPacket(client, buffer, packet_length);
  }
}

