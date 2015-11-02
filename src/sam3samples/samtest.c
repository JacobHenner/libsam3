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


int main (int argc, char *argv[]) {
  int fd;
  SAMFieldList *rep = NULL;
  const char *v;
  //
  if ((fd = sam3Handshake(NULL, 0, NULL)) < 0) return 1;
  //
  if (sam3tcpPrintf(fd, "DEST GENERATE\n") < 0) goto error;
  rep = sam3ReadReply(fd);
  //sam3DumpFieldList(rep);
  if (!sam3IsGoodReply(rep, "DEST", "REPLY", "PUB", NULL)) goto error;
  if (!sam3IsGoodReply(rep, "DEST", "REPLY", "PRIV", NULL)) goto error;
  v = sam3FindField(rep, "PUB");
  printf("PUB KEY\n=======\n%s\n", v);
  v = sam3FindField(rep, "PRIV");
  printf("PRIV KEY\n========\n%s\n", v);
  sam3FreeFieldList(rep);
  rep = NULL;
  //
  sam3FreeFieldList(rep);
  sam3tcpDisconnect(fd);
  return 0;
error:
  sam3FreeFieldList(rep);
  sam3tcpDisconnect(fd);
  return 1;
}
