

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include <IPAddress.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include <Crypto.h>
#include <Curve25519.h>
#include <RNG.h>
#include <string.h>


extern "C" {
#include "wireguardif.h"
#include "wireguard-platform.h"
}

static uint8_t m_publicKey[32];
static uint8_t m_privateKey[32];

static struct netif wg_netif_struct = {0};
static struct netif *wg_netif = NULL;
static uint8_t wireguard_peer_index = WIREGUARDIF_INVALID_INDEX;
IPAddress ipaddr(172, 17, 0, 4);
IPAddress netmask(255, 255, 255, 255);
IPAddress gw(0, 0, 0, 0);
IPAddress dns1(8, 8, 8, 8);
//my pubkey P7f2R/M/LLw78SRuCkGOqwLetzziddL3SB+LiAmfSAI=
//const char* private_key = "qHDZ0Wgr+HONrp01UCTM2NzCwcIsgqAHIe80eU7d9Hc=";  // [Interface] PrivateKey

//dPRxsakZvWZdQECawyKnWXx/+JRePpWXVKV3reHmBRw=
const char* private_key = "0J0aQfkTqgK5ZbieetRL24LYStCOvgwyne/GCMq88Wg=";
const char* public_key = "IZj329XQD6Kk9mLKuJN0da0EPdwJOlt1gR2sfRpA/jA="; // servers pubkey
int endpoint_port = 11820;              // [Peer] Endpoint
const int led = 13;


class WireGuard
{
  public:
    void begin();
    //  private:
    //    void wg_netif_set_ipaddr(struct netif *data, uint32_t addr);
    //    void wg_netif_set_netmask(struct netif *data, uint32_t addr);
    //    void wg_netif_set_gw(struct netif *data, uint32_t addr);
    //    void wg_netif_set_up(struct netif *data);
  private:
    void wg_if_add_dns(uint32_t addr);
    void wg_if_clear_dns(void);
};

//void ICACHE_FLASH_ATTR
//WireGuard::wg_netif_set_ipaddr(struct netif *data, uint32_t addr)
//{
//  ip_addr_t ipaddr;
//  ipaddr.addr = addr;
//  netif_set_ipaddr(data, &ipaddr);
//}
//
//void ICACHE_FLASH_ATTR
//WireGuard::wg_netif_set_netmask(struct netif *data, uint32_t addr)
//{
//  ip_addr_t ipaddr;
//  ipaddr.addr = addr;
//  netif_set_netmask(data, &ipaddr);
//}
//
//void ICACHE_FLASH_ATTR
//WireGuard::wg_netif_set_gw(struct netif *data, uint32_t addr)
//{
//  ip_addr_t ipaddr;
//  ipaddr.addr = addr;
//  netif_set_gw(data, &ipaddr);
//}
//
//void ICACHE_FLASH_ATTR
//WireGuard::wg_netif_set_up(struct netif *data)
//{
//  netif_set_up(data);
//}

static int dns_count;

void ICACHE_FLASH_ATTR
WireGuard::wg_if_clear_dns(void)
{
  ip_addr_t addr;
  //  addr.addr = INADDR_ANY;
  int i;
  for (i = 0; i < DNS_MAX_SERVERS; i++)
    dns_setserver(i, &addr);
  dns_count = 0;
}

void ICACHE_FLASH_ATTR
WireGuard::wg_if_add_dns(uint32_t addr)
{
  ip_addr_t ipaddr;
  ipaddr.addr = addr;
  dns_setserver(dns_count++, &ipaddr);
}

void WireGuard::begin() {
  struct wireguardif_init_data wg;
  struct wireguardif_peer peer;
  ip_addr_t _ipaddr = IPADDR4_INIT(static_cast<uint32_t>(ipaddr));
  ip_addr_t _netmask = IPADDR4_INIT(static_cast<uint32_t>(netmask));
  ip_addr_t _gateway = IPADDR4_INIT(static_cast<uint32_t>(gw));
  // Setup the WireGuard device structure
  wg.private_key = private_key;//"MIjpLZ35jlSUDWwAcDF5H75SrCxkRBWAN84MZArHzU0=";
  wg.listen_port = endpoint_port;//51820;
  wg.bind_netif = NULL;// if ethernet use eth netif
  // Initialise the first WireGuard peer structure
  wireguardif_peer_init(&peer);
  // Register the new WireGuard network interface with lwIP

  //  wg_netif = netif_add(&wg_netif_struct, NULL, NULL, NULL, &wg, &wireguardif_init, &ip_input);
  //  wg_netif_set_ipaddr(wg_netif, ipaddr);
  //  wg_netif_set_netmask(wg_netif, netmask);
  //  wg_netif_set_gw(wg_netif, gw);

  wg_netif = netif_add(&wg_netif_struct, ip_2_ip4(&_ipaddr), ip_2_ip4(&_netmask), ip_2_ip4(&_gateway), &wg, &wireguardif_init, &ip_input);

  if ( wg_netif == nullptr ) {
    Serial.println("failed to initialize WG netif.");
    return;
  }
  // Mark the interface as administratively up, link up flag is set automatically when peer connects
  //wg_netif_set_up(wg_netif); // alternate netif
  netif_set_up(wg_netif);
  //wg_if_add_dns(dns1);

  peer.public_key = public_key; //"VyDnFVE9KLFqFCtscsh6hTLfVuTMd1ylOXaBhqZ1eGk==";
  peer.preshared_key = NULL;
  // Allow all IPs through tunnel
  //  peer.allowed_ip = IPADDR4_INIT_BYTES(0, 0, 0, 0);
  //  peer.allowed_mask = IPADDR4_INIT_BYTES(0, 0, 0, 0);
  {
    ip_addr_t allowed_ip = IPADDR4_INIT_BYTES(0, 0, 0, 0);
    peer.allowed_ip = allowed_ip;
    ip_addr_t allowed_mask = IPADDR4_INIT_BYTES(0, 0, 0, 0);
    peer.allowed_mask = allowed_mask;
  }
  IPAddress IP;
  WiFi.hostByName("remnode.net", IP);
  Serial.println(IP[0]);
  // If we know the endpoint's address can add here
  peer.endpoint_ip = IPADDR4_INIT_BYTES(IP[0], IP[1], IP[2], IP[3]);
  peer.endport_port = 11820;

  // Register the new WireGuard peer with the netwok interface
  wireguardif_add_peer(wg_netif, &peer, &wireguard_peer_index);

  if ((wireguard_peer_index != WIREGUARDIF_INVALID_INDEX) && !ip_addr_isany(&peer.endpoint_ip)) {
    // Start outbound connection to peer
    wireguardif_connect(wg_netif, wireguard_peer_index);

    //delay(100);
    netif_set_default(wg_netif);
    Serial.println("wireguard start completed");
  } else if (wireguard_peer_index == WIREGUARDIF_INVALID_INDEX) {
    Serial.println("wireguard if invalid index");
  } else if (ip_addr_isany(&peer.endpoint_ip)) {
    Serial.println("wireguard endpoint ip not found");
  }
}

static WireGuard wireg;
ESP8266WebServer server(80);

static const char *base64_lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool wg_base64_encode(const uint8_t *in, size_t inlen, char *out, size_t *outlen) {
  bool result = false;
  int read_offset = 0;
  int write_offset = 0;
  uint8_t byte1, byte2, byte3;
  uint32_t tmp;
  char c;
  size_t len = 4 * ((inlen + 2) / 3);
  int padding = (3 - (inlen % 3));
  if (padding > 2) padding = 0;
  if (*outlen > len) {

    while (read_offset < inlen) {
      // Read three bytes
      byte1 = (read_offset < inlen) ? in[read_offset++] : 0;
      byte2 = (read_offset < inlen) ? in[read_offset++] : 0;
      byte3 = (read_offset < inlen) ? in[read_offset++] : 0;
      // Turn into 24 bit intermediate
      tmp = (byte1 << 16) | (byte2 << 8) | (byte3);
      // Write out 4 characters each representing 6 bits of input
      out[write_offset++] = base64_lookup[(tmp >> 18) & 0x3F];
      out[write_offset++] = base64_lookup[(tmp >> 12) & 0x3F];
      c = (write_offset < len - padding) ? base64_lookup[(tmp >> 6) & 0x3F] : '=';
      out[write_offset++] = c;
      c = (write_offset < len - padding) ? base64_lookup[(tmp) & 0x3F] : '=';
      out[write_offset++] = c;
    }
    out[len] = '\0';
    *outlen = len;
    result = true;
  } else {
    // Not enough data to put in base64 and null terminate
  }
  return result;
}

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void generateKeys() {
  Curve25519::dh1( m_publicKey, m_privateKey);
}


void setup() {
  // put your setup code here, to run once:
  RNG.begin("TestCurve25519 1.0");
  Serial.println("Connected. Initializing WireGuard...");
  Serial.begin(115200);
  Serial.println("Connecting to the AP...");

  WiFi.mode(WIFI_STA);
  WiFi.begin("MikroTik0", "");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
  Serial.println("Adjusting system time...");
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  Serial.println("Wifi Connected. Initializing WireGuard...");
  delay(6000);// wait for 6 seconds to clear wireguard ip:port persistence
  wireg.begin();

  server.on("/admin", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Generate Key Pair");
  generateKeys();
  char pub_key[96];
  char priv_key[96];
  size_t outlen_1;
  size_t outlen_2;
  wg_base64_encode((const uint8_t*) m_publicKey, 32, pub_key, &outlen_1);
  Serial.printf("pubkey: %s\n", pub_key);
  wg_base64_encode((const uint8_t*) m_privateKey, 32, priv_key, &outlen_2);
  Serial.printf("privkey: %s\n", priv_key);
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  server.client().setNoDelay(1);
}
