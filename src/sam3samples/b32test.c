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


static void testb32 (const char *src, const char *res) {
  int dlen = sam3Base32EncodedLength(strlen(src)), len;
  char dest[128];
  //
  len = sam3Base32Encode(dest, src, strlen(src));
  printf("test ");
  if (len != dlen || len != strlen(res) || strcmp(res, dest) != 0) {
    printf("FAILED!\n");
  } else {
    printf("passed\n");
  }
}


int main (int argc, char *argv[]) {
  testb32("", "");
  testb32("f", "my======");
  testb32("fo", "mzxq====");
  testb32("foo", "mzxw6===");
  testb32("foob", "mzxw6yq=");
  testb32("fooba", "mzxw6ytb");
  testb32("foobar", "mzxw6ytboi======");
  //
  return 0;
}
