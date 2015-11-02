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

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "../libsam3a/libsam3a.h"


////////////////////////////////////////////////////////////////////////////////
static void scbErrorClose (Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nSESION_ERROR: [%s]\n===============================\n", ses->error);
  sam3aCloseSession(ses); // it's safe here
}


static void scbNRCreated (Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nNAME RESOLVED: [%s]\n", ses->params);
  fprintf(stderr, "PUB: %s\n===============================\n", ses->destkey);
  sam3aCloseSession(ses); // it's safe here
}


static const Sam3ASessionCallbacks scbNR = {
  .cbError = scbErrorClose,
  .cbCreated = scbNRCreated,
  .cbDisconnected = NULL,
  .cbDatagramRead = NULL,
  .cbDestroy = NULL,
};


////////////////////////////////////////////////////////////////////////////////
static void scbKGCreated (Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nKEYS GENERATED\n");
  fprintf(stderr, "\rPRIV: %s\n", ses->privkey);
  fprintf(stderr, "\nPUB: %s\n===============================\n", ses->pubkey);
  sam3aCloseSession(ses); // it's safe here
}


static const Sam3ASessionCallbacks scbKG = {
  .cbError = scbErrorClose,
  .cbCreated = scbKGCreated,
  .cbDisconnected = NULL,
  .cbDatagramRead = NULL,
  .cbDestroy = NULL,
};


////////////////////////////////////////////////////////////////////////////////
static void scbError (Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nSESION_ERROR: [%s]\n===============================\n", ses->error);
}


static void scbCreated (Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nSESION_CREATED\n");
  fprintf(stderr, "\rPRIV: %s\n", ses->privkey);
  fprintf(stderr, "\nPUB: %s\n===============================\n", ses->pubkey);
  sam3aCancelSession(ses); // it's safe here
}


static void scbDisconnected (Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nSESION_DISCONNECTED\n===============================\n");
}


static void scbDGramRead (Sam3ASession *ses, const void *buf, int bufsize) {
  fprintf(stderr, "\n===============================\nSESION_DATAGRAM_READ\n===============================\n");
}


static void scbDestroy (Sam3ASession *ses) {
  fprintf(stderr, "\n===============================\nSESION_DESTROYED\n===============================\n");
}


static const Sam3ASessionCallbacks scb = {
  .cbError = scbError,
  .cbCreated = scbCreated,
  .cbDisconnected = scbDisconnected,
  .cbDatagramRead = scbDGramRead,
  .cbDestroy = scbDestroy,
};


////////////////////////////////////////////////////////////////////////////////
#define HOST  SAM3A_HOST_DEFAULT
//#define HOST  "google.com"


int main (int argc, char *argv[]) {
  Sam3ASession ses, snr, skg;
  //
  //libsam3a_debug = 1;
  //
  if (sam3aCreateSession(&ses, &scb, HOST, SAM3A_PORT_DEFAULT, SAM3A_DESTINATION_TRANSIENT, SAM3A_SESSION_STREAM) < 0) {
    fprintf(stderr, "FATAL: can't create main session!\n");
    return 1;
  }
  //
  if (sam3aGenerateKeys(&skg, &scbKG, HOST, SAM3A_PORT_DEFAULT) < 0) {
    sam3aCloseSession(&ses);
    fprintf(stderr, "FATAL: can't create keygen session!\n");
    return 1;
  }
  //
  if (sam3aNameLookup(&snr, &scbNR, HOST, SAM3A_PORT_DEFAULT, "zzz.i2p") < 0) {
    sam3aCloseSession(&skg);
    sam3aCloseSession(&ses);
    fprintf(stderr, "FATAL: can't create name resolving session!\n");
    return 1;
  }
  //
  while (sam3aIsActiveSession(&ses) || sam3aIsActiveSession(&snr) || sam3aIsActiveSession(&skg)) {
    fd_set rds, wrs;
    int res, maxfd = 0;
    struct timeval to;
    //
    FD_ZERO(&rds);
    FD_ZERO(&wrs);
    if (sam3aIsActiveSession(&ses) && (maxfd = sam3aAddSessionToFDS(&ses, -1, &rds, &wrs)) < 0) break;
    if (sam3aIsActiveSession(&snr) && (maxfd = sam3aAddSessionToFDS(&snr, -1, &rds, &wrs)) < 0) break;
    if (sam3aIsActiveSession(&skg) && (maxfd = sam3aAddSessionToFDS(&skg, -1, &rds, &wrs)) < 0) break;
    sam3ams2timeval(&to, 1000);
    res = select(maxfd+1, &rds, &wrs, NULL, &to);
    if (res < 0) {
      if (errno == EINTR) continue;
      fprintf(stderr, "FATAL: select() error!\n");
      break;
    }
    if (res == 0) {
      fprintf(stdout, "."); fflush(stdout);
    } else {
      if (sam3aIsActiveSession(&ses)) sam3aProcessSessionIO(&ses, &rds, &wrs);
      if (sam3aIsActiveSession(&snr)) sam3aProcessSessionIO(&snr, &rds, &wrs);
      if (sam3aIsActiveSession(&skg)) sam3aProcessSessionIO(&skg, &rds, &wrs);
    }
  }
  //
  sam3aCloseSession(&ses);
  sam3aCloseSession(&skg);
  sam3aCloseSession(&snr);
  //
  return 0;
}
