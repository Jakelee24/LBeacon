/*gcc Lbeacon.c -g -o Lbeacon -I libxbee3/include/ -L libxbee3/lib/ -lxbee -lrt
 * -lpthread -lbluetooth -lobexftp*/
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
 *      Lbeacon.c
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

#include "Lbeacon.h"
/*********************************************************************
 * @fn      parse_packet
 *
 * @brief   Parse the packet from ZigBee Reciver or ZigBee Sender.
 *          Once the reciver or sender pass packet to this function
 *          it will according to the cmd in the packet to execute command.
 *          CMD:
 *              's': Gateway to Lbeacon packets -
 *                   Switch the message which will push to
 *                   the users.
 *              'r': Gateway to Lbeacon packets -
 *                   Response health message to Gateway.
 *              'b': Gateway to Lbeacon packets -
 *                   Bind ZigBee connction with Gateway.
 *
 * @param   packet - the packet of ZigBee
 *          address - Gateway's ZigBee mac address.
 *                    When Lbeacon get 'b' request
 *                    it can use "@fn bind_gateway"
 *                    to bind with this address.
 *
 * @return  none
 */
void parse_packet(unsigned char* packet, struct xbee_conAddress address) {
  // struct Packet myPacket;
  unsigned char myPacket[81];
  switch (packet[0]) {
    case 's':

      break;
    case 'r':
      myPacket[0] = 'v';
      xbee_conTx(g_con, NULL, myPacket);
      break;
    case 'b':
      bind_gateway(address);
      break;
  };
  return;
}
void error(char* msg) {
  perror(msg);
  exit(0);
}

/*********************************************************************
 * @fn      getSystemTime
 *
 * @brief   Help "@fn addr_status_check" to give each address
 *          the start of the push time.
 *
 * @param   none
 *
 * @return  System time
 */
long long getSystemTime() {
  struct timeb t;
  ftime(&t);
  return 1000 * t.time + t.millitm;
}

/*********************************************************************
 * @fn      compare_strings
 *
 * @brief   compare two string is equal or not
 *
 * @param   a: string a
 *          b: string b
 *
 * @return  0: is equal
 *          -1: not equal
 */
int compare_strings(char a[], char b[]) {
  int c = 0;

  while (a[c] == b[c]) {
    if (a[c] == '\0' || b[c] == '\0')
      break;
    c++;
  }

  if (a[c] == '\0' && b[c] == '\0')
    return 0;
  else
    return -1;
}

/*********************************************************************
 * @fn      addr_status_check
 *
 * @brief   Help Scan function to check this address is pushing or not.
 *          if address is not in the pushing list it will put this
 *          address to the pushing list and wait for timeout to be
 *          remove from list.
 *
 * @param   addr - address scanned by Scan function
 *
 * @return  0: address in pushing list
 *          1: Unused address
 */
int addr_status_check(char addr[]) {
  int IfUsed = 0;
  int i = 0, j = 0;
  for (i = 0; i < MAX_OF_DEVICE; i++) {
    if (compare_strings(addr, UsedDeviceQueue.DeviceAppearAddr[i]) == 0) {
      IfUsed = 1;
      return IfUsed;
    }
  }
  for (i = 0; i < MAX_OF_DEVICE; i++) {
    if (UsedDeviceQueue.DeviceUsed[i] == 0) {
      for (j = 0; j < 18; j++) {
        UsedDeviceQueue.DeviceAppearAddr[i][j] = addr[j];
      }
      UsedDeviceQueue.DeviceUsed[i] = 1;
      UsedDeviceQueue.DeviceAppearTime[i] = getSystemTime();
      return IfUsed;
    }
  }
  return IfUsed;
}

/*********************************************************************
 * @fn      print_result
 *
 * @brief   Print the RSSI value of the user's address scanned
 *          by the scan function
 *
 * @param   bdaddr - Bluetooth address
 *          has_rssi - has RSSI value or not
 *          rssi - RSSI value
 *
 * @return  none
 */
static void print_result(bdaddr_t* bdaddr, char has_rssi, int rssi) {
  char addr[18];

  ba2str(bdaddr, addr);

  printf("%17s", addr);
  if (has_rssi)
    printf(" RSSI:%d", rssi);
  else
    printf(" RSSI:n/a");
  printf("\n");
  fflush(NULL);
}
Threadaddr Taddr[30];

/*********************************************************************
 * @fn      sendToPushDongle
 *
 * @brief   Send the address to push function
 *
 * @param   bdaddr - Bluetooth address
 *          has_rssi - has RSSI value or not
 *          rssi - RSSI value
 *
 * @return  none
 */
static void sendToPushDongle(bdaddr_t* bdaddr, char has_rssi, int rssi) {
  int i = 0, j = 0, idle = -1;
  char addr[18];

  void* status;
  ba2str(bdaddr, addr);
  for (i = 0; i < PUSHDONGLES; i++)
    for (j = 0; j < NUMBER_OF_DEVICE_IN_EACH_PUSHDONGLE; j++) {
      if (compare_strings(addr, addrbufferlist[i * j + j]) == 0) {
        goto out;
      }
      if (IdleHandler[i * j + j] != 1 && idle == -1) {
        idle = i * j + j;
      }
    }
  if (idle != -1 && addr_status_check(addr) == 0) {
    Threadaddr* param = &Taddr[idle];
    IdleHandler[idle] = 1;
    printf("%d\n", sizeof(addr) / sizeof(addr[0]));
    for (i = 0; i < sizeof(addr) / sizeof(addr[0]); i++) {
      addrbufferlist[idle][i] = addr[i];
      param->addr[i] = addr[i];
    }
    param->threadId = idle;
    pthread_create(&Taddr[idle].t, NULL, send_file, &Taddr[idle]);
  }
out:
  return;
}

/*********************************************************************
 * @fn      scanner_start
 *
 * @brief   Asynchronous scaning bluetooth device
 *
 * @param   none
 *
 * @return  none
 */
static void scanner_start() {
  int dev_id, sock = 0;
  struct hci_filter flt;
  inquiry_cp cp;
  unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
  hci_event_hdr* hdr;
  char canceled = 0;
  inquiry_info_with_rssi* info_rssi;
  inquiry_info* info;
  int results, i, len;
  struct pollfd p;

  // dev_id = hci_get_route(NULL);
  dev_id = SCAN_DONGLE;
  printf("%d", dev_id);

  // Open Bluetooth device
  sock = hci_open_dev(dev_id);
  if (dev_id < 0 || sock < 0) {
    perror("Can't open socket");
    return;
  }
  // Setup filter
  hci_filter_clear(&flt);
  hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
  hci_filter_set_event(EVT_INQUIRY_RESULT, &flt);
  hci_filter_set_event(EVT_INQUIRY_RESULT_WITH_RSSI, &flt);
  hci_filter_set_event(EVT_INQUIRY_COMPLETE, &flt);
  if (setsockopt(sock, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
    perror("Can't set HCI filter");
    return;
  }
  hci_write_inquiry_mode(sock, 0x01, 10);
  if (hci_send_cmd(sock, OGF_HOST_CTL, OCF_WRITE_INQUIRY_MODE,
                   WRITE_INQUIRY_MODE_RP_SIZE, &cp) < 0) {
    perror("Can't set inquiry mode");
    return;
  }

  memset(&cp, 0, sizeof(cp));
  cp.lap[2] = 0x9e;
  cp.lap[1] = 0x8b;
  cp.lap[0] = 0x33;
  cp.num_rsp = 0;
  cp.length = 0x30;

  printf("Starting inquiry with RSSI...\n");

  if (hci_send_cmd(sock, OGF_LINK_CTL, OCF_INQUIRY, INQUIRY_CP_SIZE, &cp) < 0) {
    perror("Can't start inquiry");
    return;
  }

  p.fd = sock;
  p.events = POLLIN | POLLERR | POLLHUP;

  while (!canceled) {
    p.revents = 0;

    // Poll the Bluetooth device for an event
    if (poll(&p, 1, -1) > 0) {
      len = read(sock, buf, sizeof(buf));

      if (len < 0)
        continue;
      else if (len == 0)
        break;

      hdr = (void*)(buf + 1);
      ptr = buf + (1 + HCI_EVENT_HDR_SIZE);

      results = ptr[0];

      switch (hdr->evt) {
        case EVT_INQUIRY_RESULT:
          for (i = 0; i < results; i++) {
            info = (void*)ptr + (sizeof(*info) * i) + 1;
            print_result(&info->bdaddr, 0, 0);
          }
          break;

        case EVT_INQUIRY_RESULT_WITH_RSSI:
          for (i = 0; i < results; i++) {
            info_rssi = (void*)ptr + (sizeof(*info_rssi) * i) + 1;
            print_result(&info_rssi->bdaddr, 1, info_rssi->rssi);
            if (info_rssi->rssi > RSSI_RANGE)
              sendToPushDongle(&info_rssi->bdaddr, 1, info_rssi->rssi);
          }
          break;

        case EVT_INQUIRY_COMPLETE:
          canceled = 1;

          break;
      }
    }
  }
  printf("Scaning done\n");
  close(sock);
}

/*********************************************************************
 * @fn      send_file
 *
 * @brief   Thread of Object push profile
 *
 * @param   ptr: Scanned bluetooth address
 *
 * @return  none
 */
void* send_file(void* ptr) {
  int i = 0;
  Threadaddr* Pigs = (Threadaddr*)ptr;
  struct timeval start, end;
  char* address = NULL;
  int dev_id, sock;
  int channel = -1;
  // char *filepath = "/home/pi/smsb1.txt";
  char* filename;
  obexftp_client_t* cli = NULL; /*!!!*/
  int ret;
  pthread_t tid = pthread_self();
  if (Pigs->threadId >= BLOCKOFDONGLE) {
    dev_id = PUSH_DONGLE_B;
  } else {
    dev_id = PUSH_DONGLE_A;
  }
  sock = hci_open_dev(dev_id);
  if (dev_id < 0 || sock < 0) {
    perror("opening socket");
    pthread_exit(NULL);
  }
  printf("Thread number %d\n", Pigs->threadId);
  // pthread_exit(0);
  gettimeofday(&start, NULL);
  long long start1 = getSystemTime();
  address = (char*)Pigs->addr;
  channel = obexftp_browse_bt_push(address); /*!!!*/
  /* Extract basename from file path */
  filename = strrchr(filepath, '/');
  if (!filename)
    filename = filepath;
  else
    filename++;
  printf("Sending file %s to %s\n", filename, address);
  /* Open connection */
  cli = obexftp_open(OBEX_TRANS_BLUETOOTH, NULL, NULL, NULL); /*!!!*/
  long long end1 = getSystemTime();

  printf("time: %lld ms\n", end1 - start1);
  if (cli == NULL) {
    fprintf(stderr, "Error opening obexftp client\n");
    IdleHandler[Pigs->threadId] = 0;
    for (i = 0; i < 18; i++) {
      addrbufferlist[Pigs->threadId][i] = 0;
    }

    close(sock);
    pthread_exit(NULL);
  }
  /* Connect to device */
  ret = obexftp_connect_push(cli, address, channel); /*!!!*/

  if (ret < 0) {
    fprintf(stderr, "Error connecting to obexftp device\n");
    obexftp_close(cli);
    cli = NULL;
    IdleHandler[Pigs->threadId] = 0;
    for (i = 0; i < 18; i++) {
      addrbufferlist[Pigs->threadId][i] = 0;
    }
    close(sock);
    pthread_exit(NULL);
  }

  /* Push file */
  ret = obexftp_put_file(cli, filepath, filename); /*!!!*/
  if (ret < 0) {
    fprintf(stderr, "Error putting file\n");
  }

  /* Disconnect */
  ret = obexftp_disconnect(cli); /*!!!*/
  if (ret < 0) {
    fprintf(stderr, "Error disconnecting the client\n");
  }
  /* Close */
  obexftp_close(cli); /*!!!*/
  cli = NULL;
  IdleHandler[Pigs->threadId] = 0;
  for (i = 0; i < 18; i++) {
    addrbufferlist[Pigs->threadId][i] = 0;
  }
  close(sock);
  pthread_exit(0);
}

/*********************************************************************
 * @fn      timeout_cleaner
 *
 * @brief   Thread of Timeout cleaner.
 *          When Bluetooth was pushed by push function
 *          it address will store in used list then
 *          wait for timeout to be remove from list.
 *
 * @param   none
 *
 * @return  none
 */
void* timeout_cleaner(void) {
  while (1) {
    int i = 0, j;

    for (i = 0; i < MAX_OF_DEVICE; i++)
      if (getSystemTime() - UsedDeviceQueue.DeviceAppearTime[i] > Timeout &&
          UsedDeviceQueue.DeviceUsed[i] == 1) {
        printf("Cleanertime: %lld ms\n",
               getSystemTime() - UsedDeviceQueue.DeviceAppearTime[i]);
        for (j = 0; j < 18; j++) {
          UsedDeviceQueue.DeviceAppearAddr[i][j] = 0;
        }
        UsedDeviceQueue.DeviceAppearTime[i] = 0;
        UsedDeviceQueue.DeviceUsed[i] = 0;
      }
  }
}

/*********************************************************************
 * @fn      get_config
 *
 * @brief   Read the config file and initialize parameter
 *
 * @param   filename: Name of config file
 *
 * @return  configstruct include filepath coordinate etc.
 */
struct config get_config(char* filename) {
  struct config configstruct;
  FILE* file = fopen(filename, "r");

  if (file != NULL) {
    char line[MAXBUF];
    int i = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
      char* cfline;
      cfline = strstr((char*)line, DELIM);
      cfline = cfline + strlen(DELIM);

      if (i == 0) {
        memcpy(configstruct.filepath, cfline, strlen(cfline));
        configstruct.filepath_len = strlen(cfline);
        // printf("%s",configstruct.imgserver);
      } else if (i == 1) {
        memcpy(configstruct.filename, cfline, strlen(cfline));
        configstruct.filename_len = strlen(cfline);
        // printf("%s",configstruct.ccserver);
      } else if (i == 2) {
        memcpy(configstruct.coordinate_X, cfline, strlen(cfline));
        configstruct.coordinate_X_len = strlen(cfline);
        // printf("%s",configstruct.port);
      } else if (i == 3) {
        memcpy(configstruct.coordinate_Y, cfline, strlen(cfline));
        configstruct.coordinate_Y_len = strlen(cfline);
        // printf("%s",configstruct.imagename);
      } else if (i == 4) {
        memcpy(configstruct.level, cfline, strlen(cfline));
        configstruct.level_len = strlen(cfline);
        // printf("%s",configstruct.getcmd);
      } else if (i == 5) {
        memcpy(configstruct.RSSI_Coverage, cfline, strlen(cfline));
        configstruct.RSSI_Coverage_len = strlen(cfline);
        // printf("%s",configstruct.coordinate_X);
      }
      i++;
    }  // End while
    fclose(file);
  }  // End if file

  return configstruct;
}

/*********************************************************************
 * @fn      wait_gateway_bindCB
 *
 * @brief   Callback of @fn wait_gateway_bind.
 *          when Lbeacon start ZigBee will wait for
 *          Gateway to bind with.
 *
 * @param   xbee: Name of config file
 *          con: Get Lbeacon connection info
 *          pkt: received data and address from gateway
 *          data:
 *
 * @return  none
 */
void wait_gateway_bindCB(struct xbee* xbee,
                         struct xbee_con* con,
                         struct xbee_pkt** pkt,
                         void** data) {
  if ((*pkt)->dataLen > 0) {
    printf("rx: [%s]\n", (*pkt)->data);
    parse_packet((*pkt)->data, (*pkt)->address);
  }
}

/*********************************************************************
 * @fn      wait_gateway_bind
 *
 * @brief   Wait the binding request from gateway.
 *
 * @param   none
 *
 * @return  none
 */
int wait_gateway_bind() {
  int n;
  int len;
  struct xbee_conAddress address;
  unsigned char buffer[16];
  xbee_err ret;
  memset(&address, 0, sizeof(address));
  address.addr64_enabled = 1;
  address.addr64[0] = 0x00;
  address.addr64[1] = 0x00;
  address.addr64[2] = 0x00;
  address.addr64[3] = 0x00;
  address.addr64[4] = 0x00;
  address.addr64[5] = 0x00;
  address.addr64[6] = 0xFF;
  address.addr64[7] = 0xFF;
  if ((ret = xbee_conNew(xbee, &con, "Data", &address)) != XBEE_ENONE) {
    xbee_log(xbee, -1, "xbee_conNew() returned: %d (%s)", ret,
             xbee_errorToStr(ret));
    return ret;
  }

  if ((ret = xbee_conCallbackSet(con, wait_gateway_bindCB, NULL)) !=
      XBEE_ENONE) {
    xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", ret);
    return ret;
  }

  /* getting an ACK for a broadcast message is kinda pointless... */
  xbee_conSettings(con, NULL, &settings);
  settings.disableAck = 1;
  xbee_conSettings(con, &settings, NULL);
  return 0;
}

/*********************************************************************
 * @fn      rcvCB
 *
 * @brief   receive data from gateway
 *
 * @param   xbee: Name of config file
 *          con: Get Lbeacon connection info
 *          pkt: received data and address from gateway
 *          data:
 *
 * @return  none
 */
void rcvCB(struct xbee* xbee,
           struct xbee_con* con,
           struct xbee_pkt** pkt,
           void** data) {
  if ((*pkt)->dataLen > 0) {
    printf("rx: [%s]\n", (*pkt)->data);
    parse_packet((*pkt)->data, (*pkt)->address);
  }
}

/*********************************************************************
 * @fn      bind_gateway
 *
 * @brief   When "@fn wait_gateway_bind" got request from gateway
 *          it will send the address via "@fn parse_packet"
 *          to this function and then bind with gateway.
 *
 * @param   address: Gateway's ZigBee Mac address
 *
 * @return  error event
 */
int bind_gateway(struct xbee_conAddress address) {
  void* d;
  xbee_err ret;
  address.addr64_enabled = 1;
  if ((ret = xbee_conNew(xbee, &g_con, "Data", &address)) != XBEE_ENONE) {
    xbee_log(xbee, -1, "xbee_conNew() returned: %d (%s)", ret,
             xbee_errorToStr(ret));
    return ret;
  }

  if ((ret = xbee_conDataSet(g_con, xbee, NULL)) != XBEE_ENONE) {
    xbee_log(xbee, -1, "xbee_conDataSet() returned: %d", ret);
    return ret;
  }

  if ((ret = xbee_conCallbackSet(g_con, rcvCB, NULL)) != XBEE_ENONE) {
    xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", ret);
    return ret;
  }

  return 0;
}

/*********************************************************************
 * @fn      zigbee_init
 *
 * @brief   Initialize ZigBee Device.
 *
 * @param   none
 *
 * @return  error event
 */
int zigbee_init() {
  xbee_err ret;
  if ((ret = xbee_setup(&xbee, "xbee2", "/dev/ttyUSB0", 9600)) != XBEE_ENONE) {
    printf("ret: %d (%s)\n", ret, xbee_errorToStr(ret));
    return ret;
  }
  return 0;
}

/*********************************************************************
 * STARTUP FUNCTION
 */
int main(int argc, char** argv) {
  int i = 0;
  char cmd[100];
  char hex_c[20];
  pthread_t Device_cleaner_id, ZigBee_id;

  //*-----Initialize BLE--------
  sprintf(cmd, "hciconfig hci0 leadv 3");
  system(cmd);
  sprintf(cmd, "hciconfig hci0 noscan");
  system(cmd);
  sprintf(BLE_coordinate_cmd,
          "hcitool -i hci0 cmd 0x08 0x0008 1E 02 01 1A 1A FF 4C 00 02 15 E2 C5 "
          "6D B5 DF FB 48 D2 B0 60 D0 F5 11 11 11 11 00 00 00 00 C8 00");
  //*-----Initialize BLE--------

  //*-----Load config--------start
  configstruct = get_config(CONFIG_FILENAME);
  filepath = malloc(configstruct.filepath_len + configstruct.filename_len);
  memcpy(filepath, configstruct.filepath, configstruct.filepath_len - 1);
  memcpy(filepath + configstruct.filepath_len - 1, configstruct.filename,
         configstruct.filename_len - 1);
  coordinate_X.f = (float)atof(configstruct.coordinate_X);
  coordinate_Y.f = (float)atof(configstruct.coordinate_Y);
  printf("%s\n", hex_c);
  memcpy(BLE_coordinate_cmd + 98, hex_c, 11);
  printf("%s\n", hex_c);
  memcpy(BLE_coordinate_cmd + 110, hex_c, 11);
  system(BLE_coordinate_cmd);
  //*-----Load config--------end

  //  Initialize the ZigeBee
  zigbee_init();

  //  Implement a callback function to wait for gateway bind request
  wait_gateway_bind();

  //          Device Cleaner
  pthread_create(&Device_cleaner_id, NULL, (void*)timeout_cleaner, NULL);

  for (i = 0; i < MAX_OF_DEVICE; i++)
    UsedDeviceQueue.DeviceUsed[i] = 0;
  while (1) {
    scanner_start();
  }

  return 0;
}