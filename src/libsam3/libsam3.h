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
#ifndef LIBSAM3_H
#define LIBSAM3_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////////////////////////////////////////////////////
extern int libsam3_debug;


////////////////////////////////////////////////////////////////////////////////
#define SAM3_HOST_DEFAULT  (NULL)
#define SAM3_PORT_DEFAULT  (0)

#define SAM3_DESTINATION_TRANSIENT  (NULL)

#define SAM3_PUBKEY_SIZE   (516)
#define SAM3_PRIVKEY_SIZE  (884)


////////////////////////////////////////////////////////////////////////////////
/* returns fd or -1 */
/* 'ip': host IP; can be NULL */
extern int sam3tcpConnect (const char *hostname, int port, uint32_t *ip);
extern int sam3tcpConnectIP (uint32_t ip, int port);

/* <0: error; 0: ok */
extern int sam3tcpDisconnect (int fd);

/* <0: error; 0: ok */
extern int sam3tcpSetTimeoutSend (int fd, int timeoutms);

/* <0: error; 0: ok */
extern int sam3tcpSetTimeoutReceive (int fd, int timeoutms);

/* <0: error; 0: ok */
/* sends the whole buffer */
extern int sam3tcpSend (int fd, const void *buf, int bufSize);

/* <0: received (-res) bytes; read error */
/* can return less that requesten bytes even if `allowPartial` is 0 when connection is closed */
extern int sam3tcpReceiveEx (int fd, void *buf, int bufSize, int allowPartial);

extern int sam3tcpReceive (int fd, void *buf, int bufSize);

extern int sam3tcpPrintf (int fd, const char *fmt, ...) __attribute__((format(printf,2,3)));

extern int sam3tcpReceiveStr (int fd, char *dest, int maxSize);

/* pass NULL for 'localhost' and 0 for 7655 */
/* 'ip': host IP; can be NULL */
extern int sam3udpSendTo (const char *hostname, int port, const void *buf, int bufSize, uint32_t *ip);
extern int sam3udpSendToIP (uint32_t ip, int port, const void *buf, int bufSize);


////////////////////////////////////////////////////////////////////////////////
typedef struct SAMFieldList {
  char *name;
  char *value;
  struct SAMFieldList *next;
} SAMFieldList;


extern void sam3FreeFieldList (SAMFieldList *list);
extern void sam3DumpFieldList (const SAMFieldList *list);

/* read and parse SAM reply */
/* NULL: error; else: list of fields */
/* first item is always 2-word reply, with first word in name and second in value */
extern SAMFieldList *sam3ReadReply (int fd);

extern SAMFieldList *sam3ParseReply (const char *rep);

/*
 * example:
 *   r0: 'HELLO'
 *   r1: 'REPLY'
 *   field: NULL or 'RESULT'
 *   VALUE: NULL or 'OK'
 * returns bool
 */
extern int sam3IsGoodReply (const SAMFieldList *list, const char *r0, const char *r1, const char *field, const char *value);

extern const char *sam3FindField (const SAMFieldList *list, const char *field);


////////////////////////////////////////////////////////////////////////////////
/* pass NULL for 'localhost' and 0 for 7656 */
/* returns <0 on error or socket fd on success */
extern int sam3Handshake (const char *hostname, int port, uint32_t *ip);
extern int sam3HandshakeIP (uint32_t ip, int port);


////////////////////////////////////////////////////////////////////////////////
typedef enum {
  SAM3_SESSION_RAW,
  SAM3_SESSION_DGRAM,
  SAM3_SESSION_STREAM
} Sam3SessionType;


typedef struct Sam3Session {
  Sam3SessionType type;
  int fd;
  char privkey[SAM3_PRIVKEY_SIZE+1]; // (asciiz)
  char pubkey[SAM3_PUBKEY_SIZE+1]; // (asciiz)
  char channel[66]; // (asciiz)
  char destkey[SAM3_PUBKEY_SIZE+1]; // for DGRAM sessions (asciiz)
  char error[32]; // (asciiz)
  uint32_t ip;
  int port; // this will be changed to UDP port for DRAM/RAW (can be 0)
  struct Sam3Connection *connlist; // list of opened connections
  int fwd_fd;
} Sam3Session;


typedef struct Sam3Connection {
  Sam3Session *ses;
  struct Sam3Connection *next;
  int fd;
  char destkey[SAM3_PUBKEY_SIZE+1]; // (asciiz)
  char error[32]; // (asciiz)
} Sam3Connection;


////////////////////////////////////////////////////////////////////////////////
/*
 * create SAM session
 * pass NULL as hostname for 'localhost' and 0 as port for 7656
 * pass NULL as privkey to create TRANSIENT session
 * 'params' can be NULL
 * see http://www.i2p2.i2p/i2cp.html#options for common options,
 * and http://www.i2p2.i2p/streaming.html#options for STREAM options
 * if result<0: error, 'ses' fields are undefined, no need to call sam3CloseSession()
 * if result==0: ok, all 'ses' fields are filled
 * TODO: don't clear 'error' field on error (and set it to something meaningful)
 */
extern int sam3CreateSession (Sam3Session *ses, const char *hostname, int port, const char *privkey, Sam3SessionType type,
  const char *params);

/*
 * close SAM session (and all it's connections)
 * returns <0 on error, 0 on ok
 * 'ses' must be properly initialized
 */
extern int sam3CloseSession (Sam3Session *ses);

/*
 * open stream connection to 'destkey' endpoint
 * 'destkey' is 516-byte public key (asciiz)
 * returns <0 on error, fd on ok
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 */
extern Sam3Connection *sam3StreamConnect (Sam3Session *ses, const char *destkey);

/*
 * accepts stream connection and sets 'destkey'
 * 'destkey' is 516-byte public key
 * returns <0 on error, fd on ok
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 * note that there is no timeouts for now, but you can use sam3tcpSetTimeout*()
 */
extern Sam3Connection *sam3StreamAccept (Sam3Session *ses);

/*
 * sets up forwarding stream connection
 * returns <0 on error, 0 on ok
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 * note that there is no timeouts for now, but you can use sam3tcpSetTimeout*()
 */
extern int *sam3StreamForward (Sam3Session *ses, const char *hostname, int port);

/*
 * close SAM connection
 * returns <0 on error, 0 on ok
 * 'conn' must be properly initialized
 * 'conn' is invalid after call
 */
extern int sam3CloseConnection (Sam3Connection *conn);


////////////////////////////////////////////////////////////////////////////////
/*
 * generate new keypair
 * fills 'privkey' and 'pubkey' only
 * you should not call sam3CloseSession() on 'ses'
 * will not set 'error' field
 * returns <0 on error, 0 on ok
 */
extern int sam3GenerateKeys (Sam3Session *ses, const char *hostname, int port);

/*
 * do name lookup (something like gethostbyname())
 * fills 'destkey' only
 * you should not call sam3CloseSession() on 'ses'
 * will set 'error' field
 * returns <0 on error, 0 on ok
 */
extern int sam3NameLookup (Sam3Session *ses, const char *hostname, int port, const char *name);


////////////////////////////////////////////////////////////////////////////////
/*
 * sends datagram to 'destkey' endpoint
 * 'destkey' is 516-byte public key
 * returns <0 on error, 0 on ok
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 * don't send datagrams bigger than 31KB!
 */
extern int sam3DatagramSend (Sam3Session *ses, const char *destkey, const void *buf, int bufsize);

/*
 * receives datagram and sets 'destkey' to source pubkey (if not RAW)
 * returns <0 on error (buffer too small is error too) or number of bytes written to 'buf'
 * you still have to call sam3CloseSession() on failure
 * sets ses->error on error
 * will necer receive datagrams bigger than 31KB (32KB for RAW)
 */
extern int sam3DatagramReceive (Sam3Session *ses, void *buf, int bufsize);


////////////////////////////////////////////////////////////////////////////////
extern int sam3GenChannelName (char *dest, int minlen, int maxlen);


////////////////////////////////////////////////////////////////////////////////
// NOT including '\0' terminator
static inline int sam3Base32EncodedLength (int size) { return (((size+5-1)/5)*8); }

// output 8 bytes for every 5 input
// return size or <0 on error
extern int sam3Base32Encode (char *dest, const void *srcbuf, int srcsize);


#ifdef __cplusplus
}
#endif
#endif
