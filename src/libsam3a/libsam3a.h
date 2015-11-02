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
#ifndef LIBSAM3A_H
#define LIBSAM3A_H

#include <stdarg.h>
#include <stdint.h>
#include <time.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////////////////////////////////////////////////////
/*
 * TODO:
 *   [.] block sam3aClose*() in callbacks
 */


////////////////////////////////////////////////////////////////////////////////
extern int libsam3a_debug;


////////////////////////////////////////////////////////////////////////////////
#define SAM3A_HOST_DEFAULT  (NULL)
#define SAM3A_PORT_DEFAULT  (0)

#define SAM3A_DESTINATION_TRANSIENT  (NULL)

#define SAM3A_PUBKEY_SIZE   (516)
#define SAM3A_PRIVKEY_SIZE  (884)


////////////////////////////////////////////////////////////////////////////////
extern uint64_t sam3atimeval2ms (const struct timeval *tv);
extern void sam3ams2timeval (struct timeval *tv, uint64_t ms);


////////////////////////////////////////////////////////////////////////////////
extern int sam3aIsValidPubKey (const char *key);
extern int sam3aIsValidPrivKey (const char *key);


////////////////////////////////////////////////////////////////////////////////
typedef struct Sam3ASession Sam3ASession;
typedef struct Sam3AConnection Sam3AConnection;


typedef enum {
  SAM3A_SESSION_RAW,
  SAM3A_SESSION_DGRAM,
  SAM3A_SESSION_STREAM
} Sam3ASessionType;


typedef struct {
  char *data;
  int dataSize;
  int dataUsed;
  int dataPos;
  void *udata;
  union {
    void (*cbReplyCheckSes)(Sam3ASession *ses);
    void (*cbReplyCheckConn)(Sam3AConnection *conn);
  };
} Sam3AIO;


typedef struct {
  void (*cbError) (Sam3ASession *ses);
  void (*cbCreated) (Sam3ASession *ses); // successfull SESSION CREATE
  void (*cbDisconnected) (Sam3ASession *ses); // or closed; will called only after cbCreated()
  void (*cbDatagramRead) (Sam3ASession *ses, const void *buf, int bufsize); // got datagram; bufsize >= 0; destkey set
  //
  void (*cbDestroy) (Sam3ASession *ses); // fd already closed, but keys is not cleared
} Sam3ASessionCallbacks;


struct Sam3ASession {
  Sam3ASessionType type;
  int fd;
  int cancelled; // fd was shutdown()ed, but not closed yet
  char privkey[SAM3A_PRIVKEY_SIZE+1]; // (asciiz)
  char pubkey[SAM3A_PUBKEY_SIZE+1]; // (asciiz)
  char channel[66]; // (asciiz)
  char destkey[SAM3A_PUBKEY_SIZE+1]; // for DGRAM sessions (asciiz)
  char error[64]; // (asciiz)
  uint32_t ip;
  int port; // this will be changed to UDP port for DRAM/RAW (can be 0)
  Sam3AConnection *connlist; // list of opened connections
  // for async i/o
  Sam3AIO aio;
  void (*cbAIOProcessorR) (Sam3ASession *ses); // internal
  void (*cbAIOProcessorW) (Sam3ASession *ses); // internal
  int callDisconnectCB;
  char *params; // will be cleared only by sam3aCloseSession()
  int timeoutms;
  //
  Sam3ASessionCallbacks cb;
  void *udata;
};


typedef struct {
  void (*cbError) (Sam3AConnection *ct);
  void (*cbDisconnected) (Sam3AConnection *ct); // or closed; only after cbConnected()/cbAccepted(); note that force disconnect is ok
  void (*cbConnected) (Sam3AConnection *ct);
  void (*cbAccepted) (Sam3AConnection *ct); // will be called instead of cbConnected() for sam3aStreamAccept*(), destkey filled
  void (*cbSent) (Sam3AConnection *ct); // data sent, can add new data; will be called after connect/accept
  void (*cbRead) (Sam3AConnection *ct, const void *buf, int bufsize); // data read (bufsize is always > 0)
  //
  void (*cbDestroy) (Sam3AConnection *ct); // fd already closed, but keys is not cleared
} Sam3AConnectionCallbacks;


struct Sam3AConnection {
  Sam3ASession *ses;
  Sam3AConnection *next;
  int fd;
  int cancelled; // fd was shutdown()ed, but not closed yet
  char destkey[SAM3A_PUBKEY_SIZE+1]; // (asciiz)
  char error[32]; // (asciiz)
  // for async i/o
  Sam3AIO aio;
  void (*cbAIOProcessorR) (Sam3AConnection *ct); // internal
  void (*cbAIOProcessorW) (Sam3AConnection *ct); // internal
  int callDisconnectCB;
  char *params; // will be cleared only by sam3aCloseConnection()
  int timeoutms;
  //
  Sam3AConnectionCallbacks cb;
  void *udata;
};


////////////////////////////////////////////////////////////////////////////////
/*
 * check if session is active (i.e. have opened socket)
 * returns bool
 */
extern int sam3aIsActiveSession (const Sam3ASession *ses);

/*
 * check if connection is active (i.e. have opened socket)
 * returns bool
 */
extern int sam3aIsActiveConnection (const Sam3AConnection *conn);


////////////////////////////////////////////////////////////////////////////////
/*
 * note, that return error codes indicates invalid structure, pointer or fd
 * (i.e. immediate errors); all network errors indicated with cbError() callback
 */

/*
 * create SAM session
 * pass NULL as hostname for 'localhost' and 0 as port for 7656
 * pass NULL as privkey to create TRANSIENT session
 * 'params' can be NULL
 * see http://www.i2p2.i2p/i2cp.html#options for common options,
 * and http://www.i2p2.i2p/streaming.html#options for STREAM options
 * if result<0: error, 'ses' fields are undefined, no need to call sam3aCloseSession()
 * if result==0: ok, all 'ses' fields are filled
 * TODO: don't clear 'error' field on error (and set it to something meaningful)
 */
extern int sam3aCreateSessionEx (Sam3ASession *ses, const Sam3ASessionCallbacks *cb,
  const char *hostname, int port, const char *privkey, Sam3ASessionType type, const char *params, int timeoutms);

static inline int sam3aCreateSession (Sam3ASession *ses, const Sam3ASessionCallbacks *cb,
  const char *hostname, int port, const char *privkey, Sam3ASessionType type)
{
  return sam3aCreateSessionEx(ses, cb, hostname, port, privkey, type, NULL, -1);
}

/* returns <0 on error, 0 if no, >0 if yes */
extern int sam3aIsHaveActiveConnections (const Sam3ASession *ses);

/*
 * close SAM session (and all it's connections)
 * returns <0 on error, 0 on ok
 * 'ses' must be properly initialized
 */
extern int sam3aCloseSession (Sam3ASession *ses);

/*
 * cancel SAM session (and all it's connections), but don't free() or clear anything except fds
 * returns <0 on error, 0 on ok
 * 'ses' must be properly initialized
 */
extern int sam3aCancelSession (Sam3ASession *ses);

/*
 * open stream connection to 'destkey' endpoint
 * 'destkey' is 516-byte public key (asciiz)
 * returns <0 on error
 * sets ses->error on memory or socket creation error
 */
extern Sam3AConnection *sam3aStreamConnectEx (Sam3ASession *ses, const Sam3AConnectionCallbacks *cb, const char *destkey,
  int timeoutms);

static inline Sam3AConnection *sam3aStreamConnect (Sam3ASession *ses, const Sam3AConnectionCallbacks *cb, const char *destkey) {
  return sam3aStreamConnectEx(ses, cb, destkey, -1);
}

/*
 * accepts stream connection and sets 'destkey'
 * 'destkey' is 516-byte public key
 * returns <0 on error, fd on ok
 * you still have to call sam3aCloseSession() on failure
 * sets ses->error on error
 * note that there is no timeouts for now, but you can use sam3atcpSetTimeout*()
 */
extern Sam3AConnection *sam3aStreamAcceptEx (Sam3ASession *ses, const Sam3AConnectionCallbacks *cb, int timeoutms);

static inline Sam3AConnection *sam3aStreamAccept (Sam3ASession *ses, const Sam3AConnectionCallbacks *cb) {
  return sam3aStreamAcceptEx(ses, cb, -1);
}

/*
 * close SAM connection, remove it from session and free memory
 * returns <0 on error, 0 on ok
 * 'conn' must be properly initialized
 * 'conn' is invalid after call
 */
extern int sam3aCloseConnection (Sam3AConnection *conn);

/*
 * cancel SAM connection, but don't free() or clear anything except fd
 * returns <0 on error, 0 on ok
 * 'conn' must be properly initialized
 * 'conn' is invalid after call
 */
extern int sam3aCancelConnection (Sam3AConnection *conn);


////////////////////////////////////////////////////////////////////////////////
/*
 * send data
 * this function can be used in cbSent() callback
 *
 * return: <0: error; 0: ok
 */
extern int sam3aSend (Sam3AConnection *conn, const void *data, int datasize);

/*
 * sends datagram to 'destkey' endpoint
 * 'destkey' is 516-byte public key
 * returns <0 on error, 0 on ok
 * you still have to call sam3aCloseSession() on failure
 * sets ses->error on error
 * don't send datagrams bigger than 31KB!
 */
extern int sam3aDatagramSend (Sam3ASession *ses, const char *destkey, const void *buf, int bufsize);


////////////////////////////////////////////////////////////////////////////////
/*
 * generate random channel name
 * dest should be at least (maxlen+1) bytes big
 */
extern int sam3aGenChannelName (char *dest, int minlen, int maxlen);


////////////////////////////////////////////////////////////////////////////////
/*
 * generate new keypair
 * fills 'privkey' and 'pubkey' only
 * you should call sam3aCloseSession() on 'ses'
 * cbCreated callback will be called when keys generated
 * returns <0 on error, 0 on ok
 */
extern int sam3aGenerateKeysEx (Sam3ASession *ses, const Sam3ASessionCallbacks *cb, const char *hostname, int port,
  int timeoutms);

static inline int sam3aGenerateKeys (Sam3ASession *ses, const Sam3ASessionCallbacks *cb, const char *hostname, int port) {
  return sam3aGenerateKeysEx(ses, cb, hostname, port, -1);
}


/*
 * do name lookup (something like gethostbyname())
 * fills 'destkey' only
 * you should call sam3aCloseSession() on 'ses'
 * cbCreated callback will be called when keys generated, ses->destkey will be set
 * returns <0 on error, 0 on ok
 */
extern int sam3aNameLookupEx (Sam3ASession *ses, const Sam3ASessionCallbacks *cb, const char *hostname, int port,
  const char *name, int timeoutms);

static inline int sam3aNameLookup (Sam3ASession *ses, const Sam3ASessionCallbacks *cb, const char *hostname, int port,
  const char *name)
{
  return sam3aNameLookupEx(ses, cb, hostname, port, name, -1);
}


////////////////////////////////////////////////////////////////////////////////
/*
 * append session fd to read and write sets if necessary
 * adds all alive session connections too
 * returns maxfd or -1
 * TODO: should keep fd count so it will not exceed FD_SETSIZE!
 */
extern int sam3aAddSessionToFDS (Sam3ASession *ses, int maxfd, fd_set *rds, fd_set *wrs);

/*
 * process session i/o (and all session connections i/o)
 * should be called after successful select()
 */
extern void sam3aProcessSessionIO (Sam3ASession *ses, fd_set *rds, fd_set *wrs);


////////////////////////////////////////////////////////////////////////////////
/* return malloc()ed buffer and len in 'plen' (if plen != NULL) */
extern char *sam3PrintfVA (int *plen, const char *fmt, va_list app);
extern char *sam3Printf (int *plen, const char *fmt, ...) __attribute__((format(printf,2,3)));


#ifdef __cplusplus
}
#endif
#endif
