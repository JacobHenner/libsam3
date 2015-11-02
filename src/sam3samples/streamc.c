/*
Copyright (C) 2015 Jacob Henner

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libsam3/libsam3.h"


#define KEYFILE  "streams.key"


int main (int argc, char *argv[]) {
  Sam3Session ses;
  Sam3Connection *conn;
  char cmd[1024], destkey[517]; // 516 chars + \0
  //
  //libsam3_debug = 1;
  //
  memset(destkey, 0, sizeof(destkey));
  //
  if (argc < 2) {
    FILE *fl = fopen(KEYFILE, "rb");
    //
    if (fl != NULL) {
      if (fread(destkey, 516, 1, fl) == 1) {
        fclose(fl);
        goto ok;
      }
      fclose(fl);
    }
    printf("usage: dgramc PUBKEY\n");
    return 1;
  } else {
    if (strlen(argv[1]) != 516) {
      fprintf(stderr, "FATAL: invalid key length!\n");
      return 1;
    }
    strcpy(destkey, argv[1]);
  }
  //
ok:
  printf("creating session...\n");
  // create TRANSIENT session
  if (sam3CreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, SAM3_DESTINATION_TRANSIENT, SAM3_SESSION_STREAM, NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  //
  printf("connecting...\n");
  if ((conn = sam3StreamConnect(&ses, destkey)) == NULL) {
    fprintf(stderr, "FATAL: can't connect: %s\n", ses.error);
    sam3CloseSession(&ses);
    return 1;
  }
  //
  // now waiting for incoming connection
  printf("sending test command...\n");
  if (sam3tcpPrintf(conn->fd, "test\n") < 0) goto error;
  if (sam3tcpReceiveStr(conn->fd, cmd, sizeof(cmd)) < 0) goto error;
  printf("echo: %s\n", cmd);
  //
  printf("sending quit command...\n");
  if (sam3tcpPrintf(conn->fd, "quit\n") < 0) goto error;
  //
  sam3CloseConnection(conn);
  sam3CloseSession(&ses);
  return 0;
error:
  fprintf(stderr, "FATAL: some error occured!\n");
  sam3CloseConnection(conn);
  sam3CloseSession(&ses);
  return 1;
}
