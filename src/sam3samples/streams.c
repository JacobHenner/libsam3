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
  FILE *fl;
  //
  //libsam3_debug = 1;
  //
  printf("creating session...\n");
  // create TRANSIENT session
  if (sam3CreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, SAM3_DESTINATION_TRANSIENT, SAM3_SESSION_STREAM, NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  //
  printf("PUB KEY\n=======\n%s\n=======\n", ses.pubkey);
  if ((fl = fopen(KEYFILE, "wb")) != NULL) {
    fwrite(ses.pubkey, strlen(ses.pubkey), 1, fl);
    fclose(fl);
  }
  //
  printf("starting stream acceptor...\n");
  if ((conn = sam3StreamAccept(&ses)) == NULL) {
    fprintf(stderr, "FATAL: can't accept: %s\n", ses.error);
    sam3CloseSession(&ses);
    return 1;
  }
  printf("FROM\n====\n%s\n====\n", conn->destkey);
  //
  printf("starting main loop...\n");
  for (;;) {
    char cmd[256];
    //
    if (sam3tcpReceiveStr(conn->fd, cmd, sizeof(cmd)) < 0) goto error;
    printf("cmd: [%s]\n", cmd);
    if (strcmp(cmd, "quit") == 0) break;
    // echo command
    if (sam3tcpPrintf(conn->fd, "re: %s\n", cmd) < 0) goto error;
  }
  //
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 0;
error:
  fprintf(stderr, "FATAL: some error occured!\n");
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 1;
}
