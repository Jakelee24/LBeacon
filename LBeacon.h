/*
 * Copyright (c) 2016 Academia Sinica, Institude of Information Science
 *
 * License:
 *      GPL 3.0 : The content of this file is subject to the terms and
 *      conditions defined in file 'COPYING.txt', which is part of this source
 *      code package.
 *
 * Project Name:
 *
 *      BeDIPS
 *
 * File Description:
 * File Name:
 *
 *      Lbeacon.h
 *
 * Abstract:
 *
 *      LBeacon (Location Beacon): BeDIPS uses LBeacons to deliver to
 *      users the 3D coordinates and textual descriptions of their locations.
 *      Basically, Lbeacon is a low-cost, Bluetooth Smart Ready device.
 *      At deployment and maintenance times, the 3D coordinates and location
 *      description of every Lbeacon are retrieved from BeDIS
 *      (Building/environment Data and Information System) and stored locally.
 *      Once initialized, each Lbeacon broadcast its coordinates and location
 *      description to Bluetooth enabled mobile devices nearby within its
 * coverage area.
 *
 * Authors:
 *
 *      Jake Lee, jakelee@iis.sinica.edu.tw
 *
 */
/*********************************************************************
  * INCLUDES
  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <signal.h>
#include <obexftp/client.h> /*!!!*/
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <xbee.h>
#include <limits.h>
#include <ctype.h>

/*********************************************************************
  * CONTANTS
  */

//  Maximum value of the Bluetooth Object Push at the same time
#define MAX_OF_DEVICE 30

//  Maximum value of the Bluetooth Object Push threads at the same time
#define NTHREADS 30

//  Number of the Bluetooth dongles which is for PUSH function
#define PUSHDONGLES 2

//  Maximum value of each Push dongle can handle how many users
#define NUMBER_OF_DEVICE_IN_EACH_PUSHDONGLE 9

//  The number of users device responsible for each Push dongle
#define BLOCKOFDONGLE 5

//  Mark the thread that has finished working
#define IDLE -1

//  Maximum character of each line of config file
#define MAXBUF 64

//  Read the parameter after "=" from config file
#define DELIM "="

//  The name of the config file
#define CONFIG_FILENAME "config.conf"

//  Device ID of the Scan dongle
#define SCAN_DONGLE 1

//  Device ID of the Push dongle
#define PUSH_DONGLE_A 2

//  Device ID of the secondary Push dongle
#define PUSH_DONGLE_B 3

//  The interval time of same user object push
const long long Timeout = 20000;

//  Column: value of 30 is depend on MAX_OF_DEVICE
//  Raw: value of 18 is depend on length of Bluetooth MAC address
char addr[30][18] = {0};

//  Same with variable of "addr" but this is use for handle pushed users
char addrbufferlist[PUSHDONGLES * NUMBER_OF_DEVICE_IN_EACH_PUSHDONGLE][18] = {
    0};

//  Path of object push file
char* filepath;

//  HCI command for BLE beacon
char BLE_coordinate_cmd[100];

//  List of threads is idle or not
int IdleHandler[PUSHDONGLES * NUMBER_OF_DEVICE_IN_EACH_PUSHDONGLE] = {0};

//  Id for create thread
pthread_t thread_id[NTHREADS] = {0};

//  Limit the transmission range
int RSSI_RANGE = -60;

//  Parameters of ZigBee initialization
struct xbee* xbee;
void* d;
struct xbee_con* g_con;
struct xbee_con* con;
struct xbee_conAddress g_address;
struct xbee_conSettings settings;
struct config configstruct;

//  Transform float to Hex code
union {
  float f;
  unsigned char b[sizeof(float)];
} coordinate_X;
union {
  float f;
  unsigned char b[sizeof(float)];
} coordinate_Y;

/*********************************************************************
 * STRUCTS
 */

//  Packet format transmitted via ZigBee
//  |1byte|5bytes|2bytes|2byte|64bytes|
struct Packet {
  unsigned char CMD;  //   --1 byte
  unsigned char verify_code[16];
  unsigned char data[64];  //--64 bytes
};

//  Config parameters
struct config {
  char filepath[MAXBUF];
  char filename[MAXBUF];
  char coordinate_X[MAXBUF];
  char coordinate_Y[MAXBUF];
  char level[MAXBUF];
  char RSSI_Coverage[MAXBUF];
  int filepath_len;
  int filename_len;
  int coordinate_X_len;
  int coordinate_Y_len;
  int level_len;
  int RSSI_Coverage_len;
};

/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
  char addr[18];
  int threadId;
  pthread_t t;

} Threadaddr;

typedef struct {
  long long DeviceAppearTime[MAX_OF_DEVICE];
  char DeviceAppearAddr[MAX_OF_DEVICE][18];
  char DeviceUsed[MAX_OF_DEVICE];

} DeviceQueue;
typedef struct {
  unsigned char COMM;
  unsigned char data[64];
  int Packagelen;
  int Datalen;
  int count;
  /* data */
} DataPackage;
DeviceQueue UsedDeviceQueue;

int ZigBee_addr_Scan_count = 0;
struct xbee_conAddress Gatewayaddr;

/*********************************************************************
 * FUNCTIONS
 */

//  Parse the packet and execute from gateway
void parse_packet(unsigned char* packet, struct xbee_conAddress address);

//  error handler
void error(char* msg);

//  Get the System time
long long getSystemTime();

//  Compare two string is equal or not
int compare_strings(char a[], char b[]);

//  Check if the user can be pushed again
int addr_status_check(char addr[]);

//  Print the result of RSSI value in each case
static void print_result(bdaddr_t* bdaddr, char has_rssi, int rssi);

//  Send scanded user address to push dongle
static void sendToPushDongle(bdaddr_t* bdaddr, char has_rssi, int rssi);

//  Start scanning bluetooth device
static void scanner_start();

//  Prototype for the file sending function
void* send_file(void* address);

//  Thread of push dongle to send file for users
void* send_file(void* ptr);

//  Remove the user ID from pushed list
void* timeout_cleaner(void);

//  Read parameter from config file
struct config get_config(char* filename);

//  Callback for ZigBee reciver
void wait_gateway_bindCB(struct xbee* xbee,
                         struct xbee_con* con,
                         struct xbee_pkt** pkt,
                         void** data);

//  Wait gateway's bind request
int Wait_Gateway_bind();

//  Callback for ZigBee reciver
void rcvCB(struct xbee* xbee,
           struct xbee_con* con,
           struct xbee_pkt** pkt,
           void** data);

//  When gateway request to bind then use this to bind it
int bind_gateway(struct xbee_conAddress address);

//  Initialize the ZigeBee
int zigbee_init();
