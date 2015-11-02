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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libsam3/libsam3.h"


int main (int argc, char *argv[]) {
  Sam3Session ses;
  //
  if (argc < 2) {
    printf("usage: %s name [name...]\n", argv[0]);
    return 1;
  }
  //
  for (int n = 1; n < argc; ++n) {
    fprintf(stdout, "%s ... ", argv[n]); fflush(stdout);
    if (sam3NameLookup(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, argv[n]) >= 0) {
      fprintf(stdout, "%s\n\n", ses.destkey);
    } else {
      fprintf(stdout, "FAILED [%s]\n", ses.error);
    }
  }
  //
  return 0;
}
