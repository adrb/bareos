/*
   BAREOS® - Backup Archiving REcovery Open Sourced

   Copyright (C) 2000-2011 Free Software Foundation Europe e.V.
   Copyright (C) 2011-2012 Planets Communications B.V.
   Copyright (C) 2013-2016 Bareos GmbH & Co. KG

   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.
*/
/*
 * Network Utility Routines
 *
 * by Kern Sibbald
 *
 * Adapted and enhanced for BAREOS, originally written
 * for inclusion in the Apcupsd package
 */
/**
 * @file
 * Network Utility Routines
 */

#include "include/bareos.h"
#include "jcr.h"
#include "lib/bnet.h"
#include "lib/bsys.h"

#include <netdb.h>
#include "lib/tls_openssl.h"

#ifndef INADDR_NONE
#define INADDR_NONE    -1
#endif

#ifndef HAVE_GETADDRINFO
static pthread_mutex_t ip_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/**
 * Receive a message from the other end. Each message consists of
 * two packets. The first is a header that contains the size
 * of the data that follows in the second packet.
 * Returns number of bytes read (may return zero)
 * Returns -1 on signal (BNET_SIGNAL)
 * Returns -2 on hard end of file (BNET_HARDEOF)
 * Returns -3 on error  (BNET_ERROR)
 *
 *  Unfortunately, it is a bit complicated because we have these
 *    four return types:
 *    1. Normal data
 *    2. Signal including end of data stream
 *    3. Hard end of file
 *    4. Error
 *  Using IsBnetStop() and IsBnetError() you can figure this all out.
 */
int32_t BnetRecv(BareosSocket * bsock)
{
   return bsock->recv();
}


/**
 * Return 1 if there are errors on this bsock or it is closed,
 *   i.e. stop communicating on this line.
 */
bool IsBnetStop(BareosSocket * bsock)
{
   return bsock->IsStop();
}

/**
 * Return number of errors on socket
 */
int IsBnetError(BareosSocket * bsock)
{
   return bsock->IsError();
}

/**
 * Call here after error during closing to suppress error
 *  messages which are due to the other end shutting down too.
 */
void BnetSuppressErrorMessages(BareosSocket * bsock, bool flag)
{
   bsock->suppress_error_msgs_ = flag;
}

/**
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a 32 bit integer containing
 * the length of the data packet which follows.
 *
 * Returns: false on failure
 *          true  on success
 */
bool BnetSend(BareosSocket *bsock)
{
   return bsock->send();
}


/**
 * Establish a TLS connection -- server side
 *  Returns: true  on success
 *           false on failure
 */
#ifdef HAVE_TLS
bool BnetTlsServer(std::shared_ptr<TlsContext> tls_ctx, BareosSocket *bsock, alist *verify_list)
{
   TLS_CONNECTION *tls_conn = nullptr;
   JobControlRecord *jcr = bsock->jcr();

   tls_conn = new_tls_connection(tls_ctx, bsock->fd_, true);
   if (!tls_conn) {
      Qmsg0(bsock->jcr(), M_FATAL, 0, _("TLS connection initialization failed.\n"));
      return false;
   }

   bsock->SetTlsConnection(tls_conn);

   /*
    * Initiate TLS Negotiation
    */
   if (!TlsBsockAccept(bsock)) {
      Qmsg0(bsock->jcr(), M_FATAL, 0, _("TLS Negotiation failed.\n"));
      goto err;
   }

   if (verify_list) {
      if (!TlsPostconnectVerifyCn(jcr, tls_conn, verify_list)) {
         Qmsg1(bsock->jcr(), M_FATAL, 0, _("TLS certificate verification failed."
                                         " Peer certificate did not match a required commonName\n"),
                                         bsock->host());
         goto err;
      }
   }

   Dmsg0(50, "TLS server negotiation established.\n");
   return true;

err:
   bsock->FreeTls();
   return false;
}

/**
 * Establish a TLS connection -- client side
 * Returns: true  on success
 *          false on failure
 */
bool BnetTlsClient(std::shared_ptr<TLS_CONTEXT> tls_ctx, BareosSocket *bsock, bool VerifyPeer, alist *verify_list)
{
   TLS_CONNECTION *tls_conn;
   JobControlRecord *jcr = bsock->jcr();

   tls_conn  = new_tls_connection(tls_ctx, bsock->fd_, false);
   if (!tls_conn) {
      Qmsg0(bsock->jcr(), M_FATAL, 0, _("TLS connection initialization failed.\n"));
      return false;
   }

   bsock->SetTlsConnection(tls_conn);

   /*
    * Initiate TLS Negotiation
    */

    if (!TlsBsockConnect(bsock)) {
      goto err;
   }

   if (VerifyPeer) {
      /*
       * If there's an Allowed CN verify list, use that to validate the remote
       * certificate's CN. Otherwise, we use standard host/CN matching.
       */
      if (verify_list) {
         if (!TlsPostconnectVerifyCn(jcr, tls_conn, verify_list)) {
            Qmsg1(bsock->jcr(), M_FATAL, 0, _("TLS certificate verification failed."
                                            " Peer certificate did not match a required commonName\n"),
                                            bsock->host());
            goto err;
         }
      } else {
         if (!TlsPostconnectVerifyHost(jcr, tls_conn, bsock->host())) {
            Qmsg1(bsock->jcr(), M_FATAL, 0, _("TLS host certificate verification failed. Host name \"%s\" did not match presented certificate\n"),
                  bsock->host());
            goto err;
         }
      }
   }

   Dmsg0(50, "TLS client negotiation established.\n");
   return true;

err:
   bsock->FreeTls();
   return false;
}
#else
bool BnetTlsServer(std::shared_ptr<TlsContext> tls_ctx, BareosSocket * bsock, alist *verify_list)
{
   Jmsg(bsock->jcr(), M_ABORT, 0, _("TLS enabled but not configured.\n"));
   return false;
}

bool BnetTlsClient(std::shared_ptr<TLS_CONTEXT> tls_ctx, BareosSocket *bsock, bool VerifyPeer, alist *verify_list)
{
   Jmsg(bsock->jcr(), M_ABORT, 0, _("TLS enabled but not configured.\n"));
   return false;
}
#endif /* HAVE_TLS */

/**
 * Wait for a specified time for data to appear on
 * the BareosSocket connection.
 *
 *   Returns: 1 if data available
 *            0 if timeout
 *           -1 if error
 */
int BnetWaitData(BareosSocket * bsock, int sec)
{
   return bsock->WaitData(sec);
}

/**
 * As above, but returns on interrupt
 */
int BnetWaitDataIntr(BareosSocket * bsock, int sec)
{
   return bsock->WaitDataIntr(sec);
}

#ifndef NETDB_INTERNAL
#define NETDB_INTERNAL  -1         /* See errno. */
#endif
#ifndef NETDB_SUCCESS
#define NETDB_SUCCESS   0          /* No problem. */
#endif
#ifndef HOST_NOT_FOUND
#define HOST_NOT_FOUND  1          /* Authoritative Answer Host not found. */
#endif
#ifndef TRY_AGAIN
#define TRY_AGAIN       2          /* Non-Authoritative Host not found, or SERVERFAIL. */
#endif
#ifndef NO_RECOVERY
#define NO_RECOVERY     3          /* Non recoverable errors, FORMERR, REFUSED, NOTIMP. */
#endif
#ifndef NO_DATA
#define NO_DATA         4          /* Valid name, no data record of requested type. */
#endif

#if HAVE_GETADDRINFO
const char *resolv_host(int family, const char *host, dlist *addr_list)
{
   int res;
   struct addrinfo hints;
   struct addrinfo *ai, *rp;
   IPADDR *addr;

   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_family = family;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;
   hints.ai_flags = 0;

   res = getaddrinfo(host, NULL, &hints, &ai);
   if (res != 0) {
      return gai_strerror(res);
   }

   for (rp = ai; rp != NULL; rp = rp->ai_next) {
      switch (rp->ai_addr->sa_family) {
      case AF_INET:
         addr = New(IPADDR(rp->ai_addr->sa_family));
         addr->SetType(IPADDR::R_MULTIPLE);
         /*
          * Some serious casting to get the struct in_addr *
          * rp->ai_addr == struct sockaddr
          * as this is AF_INET family we can cast that
          * to struct_sockaddr_in. Of that we need the
          * address of the sin_addr member which contains a
          * struct in_addr
          */
         addr->set_addr4(&(((struct sockaddr_in *)rp->ai_addr)->sin_addr));
         break;
#ifdef HAVE_IPV6
      case AF_INET6:
         addr = New(IPADDR(rp->ai_addr->sa_family));
         addr->SetType(IPADDR::R_MULTIPLE);
         /*
          * Some serious casting to get the struct in6_addr *
          * rp->ai_addr == struct sockaddr
          * as this is AF_INET6 family we can cast that
          * to struct_sockaddr_in6. Of that we need the
          * address of the sin6_addr member which contains a
          * struct in6_addr
          */
         addr->set_addr6(&(((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr));
         break;
#endif
      default:
         continue;
      }
      addr_list->append(addr);
   }
   freeaddrinfo(ai);
   return NULL;
}
#else
/**
 * Get human readable error for gethostbyname()
 */
static const char *gethost_strerror()
{
   const char *msg;
   berrno be;
   switch (h_errno) {
   case NETDB_INTERNAL:
      msg = be.bstrerror();
      break;
   case NETDB_SUCCESS:
      msg = _("No problem.");
      break;
   case HOST_NOT_FOUND:
      msg = _("Authoritative answer for host not found.");
      break;
   case TRY_AGAIN:
      msg = _("Non-authoritative for host not found, or ServerFail.");
      break;
   case NO_RECOVERY:
      msg = _("Non-recoverable errors, FORMERR, REFUSED, or NOTIMP.");
      break;
   case NO_DATA:
      msg = _("Valid name, no data record of resquested type.");
      break;
   default:
      msg = _("Unknown error.");
   }
   return msg;
}

static const char *resolv_host(int family, const char *host, dlist *addr_list)
{
   struct hostent *hp;
   const char *errmsg;
   char **p;
   IPADDR *addr;

   P(ip_mutex);                       /* gethostbyname() is not thread safe */
#ifdef HAVE_GETHOSTBYNAME2
   if ((hp = gethostbyname2(host, family)) == NULL) {
#else
   if ((hp = gethostbyname(host)) == NULL) {
#endif
      /* may be the strerror give not the right result -:( */
      errmsg = gethost_strerror();
      V(ip_mutex);
      return errmsg;
   } else {
      for (p = hp->h_addr_list; *p != 0; p++) {
         switch (hp->h_addrtype) {
         case AF_INET:
            addr = New(IPADDR(hp->h_addrtype));
            addr->SetType(IPADDR::R_MULTIPLE);
            addr->set_addr4((struct in_addr *)*p);
            break;
#ifdef HAVE_IPV6
          case AF_INET6:
            addr = New(IPADDR(hp->h_addrtype));
            addr->SetType(IPADDR::R_MULTIPLE);
            addr->set_addr6((struct in6_addr *)*p);
            break;
#endif
         default:
            continue;
         }
         addr_list->append(addr);
      }
      V(ip_mutex);
   }
   return NULL;
}
#endif

static IPADDR *add_any(int family)
{
   IPADDR *addr = New(IPADDR(family));
   addr->SetType(IPADDR::R_MULTIPLE);
   addr->SetAddrAny();
   return addr;
}

/**
 * i host = 0 mean INADDR_ANY only ipv4
 */
dlist *bnet_host2ipaddrs(const char *host, int family, const char **errstr)
{
   struct in_addr inaddr;
   IPADDR *addr = 0;
   const char *errmsg;
#ifdef HAVE_IPV6
   struct in6_addr inaddr6;
#endif

   dlist *addr_list = New(dlist(addr, &addr->link));
   if (!host || host[0] == '\0') {
      if (family != 0) {
         addr_list->append(add_any(family));
      } else {
         addr_list->append(add_any(AF_INET));
#ifdef HAVE_IPV6
         addr_list->append(add_any(AF_INET6));
#endif
      }
   } else if (inet_aton(host, &inaddr)) { /* MA Bug 4 */
      addr = New(IPADDR(AF_INET));
      addr->SetType(IPADDR::R_MULTIPLE);
      addr->set_addr4(&inaddr);
      addr_list->append(addr);
#ifdef HAVE_IPV6
#ifndef HAVE_WIN32
   } else if (inet_pton(AF_INET6, host, &inaddr6) == 1) {
#else
   } else if (p_InetPton && p_InetPton(AF_INET6, host, &inaddr6) == 1) {
#endif
      addr = New(IPADDR(AF_INET6));
      addr->SetType(IPADDR::R_MULTIPLE);
      addr->set_addr6(&inaddr6);
      addr_list->append(addr);
#endif
   } else {
      if (family != 0) {
         errmsg = resolv_host(family, host, addr_list);
         if (errmsg) {
            *errstr = errmsg;
            FreeAddresses(addr_list);
            return 0;
         }
      } else {
#ifdef HAVE_IPV6
         /* We try to resolv host for ipv6 and ipv4, the connection procedure
          * will try to reach the host for each protocols. We report only "Host
          * not found" ipv4 message (no need to have ipv6 and ipv4 messages).
          */
         resolv_host(AF_INET6, host, addr_list);
#endif
         errmsg = resolv_host(AF_INET, host, addr_list);

         if (addr_list->size() == 0) {
            *errstr = errmsg;
            FreeAddresses(addr_list);
            return 0;
         }
      }
   }
   return addr_list;
}

/**
 * Return the string for the error that occurred
 * on the socket. Only the first error is retained.
 */
const char *bnet_strerror(BareosSocket * bsock)
{
   return bsock->bstrerror();
}

/**
 * Format and send a message
 *  Returns: false on error
 *           true  on success
 */
bool BnetFsend(BareosSocket * bs, const char *fmt, ...)
{
   va_list arg_ptr;
   int maxlen;

   if (bs->errors || bs->IsTerminated()) {
      return false;
   }
   /* This probably won't work, but we vsnprintf, then if we
    * get a negative length or a length greater than our buffer
    * (depending on which library is used), the printf was truncated, so
    * get a bigger buffer and try again.
    */
   for (;;) {
      maxlen = SizeofPoolMemory(bs->msg) - 1;
      va_start(arg_ptr, fmt);
      bs->msglen = Bvsnprintf(bs->msg, maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (bs->msglen > 0 && bs->msglen < (maxlen - 5)) {
         break;
      }
      bs->msg = ReallocPoolMemory(bs->msg, maxlen + maxlen / 2);
   }
   return bs->send();
}

int BnetGetPeer(BareosSocket *bs, char *buf, socklen_t buflen)
{
   return bs->GetPeer(buf, buflen);
}

/**
 * Set the network buffer size, suggested size is in size.
 *  Actual size obtained is returned in bs->msglen
 *
 *  Returns: 0 on failure
 *           1 on success
 */
bool BnetSetBufferSize(BareosSocket * bs, uint32_t size, int rw)
{
   return bs->SetBufferSize(size, rw);
}

/**
 * Set socket non-blocking
 * Returns previous socket flag
 */
int BnetSetNonblocking(BareosSocket *bsock)
{
   return bsock->SetNonblocking();
}

/**
 * Set socket blocking
 * Returns previous socket flags
 */
int BnetSetBlocking(BareosSocket *bsock)
{
   return bsock->SetBlocking();
}

/**
 * Restores socket flags
 */
void BnetRestoreBlocking (BareosSocket *bsock, int flags)
{
   bsock->RestoreBlocking(flags);
}

/**
 * Send a network "signal" to the other end
 *  This consists of sending a negative packet length
 *
 *  Returns: false on failure
 *           true  on success
 */
bool BnetSig(BareosSocket * bs, int signal)
{
   return bs->signal(signal);
}

/**
 * Convert a network "signal" code into
 * human readable ASCII.
 */
const char *bnet_sig_to_ascii(BareosSocket * bs)
{
   static char buf[30];
   switch (bs->msglen) {
   case BNET_EOD:
      return "BNET_EOD";           /* end of data stream */
   case BNET_EOD_POLL:
      return "BNET_EOD_POLL";
   case BNET_STATUS:
      return "BNET_STATUS";
   case BNET_TERMINATE:
      return "BNET_TERMINATE";     /* Terminate connection */
   case BNET_POLL:
      return "BNET_POLL";
   case BNET_HEARTBEAT:
      return "BNET_HEARTBEAT";
   case BNET_HB_RESPONSE:
      return "BNET_HB_RESPONSE";
   case BNET_SUB_PROMPT:
      return "BNET_SUB_PROMPT";
   case BNET_TEXT_INPUT:
      return "BNET_TEXT_INPUT";
   default:
      sprintf(buf, _("Unknown sig %d"), (int)bs->msglen);
      return buf;
   }
}