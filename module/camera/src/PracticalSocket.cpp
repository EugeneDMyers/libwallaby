/*
 *   C++ sockets on Unix and Windows
 *   Copyright (C) 2002
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PracticalSocket.h"

#ifdef WIN32
  #include <winsock.h>         // For socket(), connect(), send(), and recv()
  typedef int socklen_t;
  typedef char raw_type;       // Type used for raw data on this platform
#else
  #include <sys/types.h>       // For data types
  #include <sys/socket.h>      // For socket(), connect(), send(), and recv()
  #include <netdb.h>           // For gethostbyname()
  #include <arpa/inet.h>       // For inet_addr()
  #include <unistd.h>          // For close()
  #include <netinet/in.h>      // For sockaddr_in
  typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno
#include <string.h>             // For memset
#include <iostream>
#include <ostream>
#include <sys/select.h>

using namespace std;

#ifdef WIN32
static bool initialized = false;
#endif


// Function to fill in address structure given an address and port
static void fillAddr(const string &address, unsigned short port, 
                     sockaddr_in &addr) {
  memset(&addr, 0, sizeof(addr));  // Zero out address structure
  addr.sin_family = AF_INET;       // Internet address

  hostent *host;  // Resolve name
  if ((host = gethostbyname(address.c_str())) == NULL) {
    // strerror() will not work for gethostbyname() and hstrerror() 
    // is supposedly obsolete
    cout << "Failed to resolve name (gethostbyname())" << endl;
  }
  addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);

  addr.sin_port = htons(port);     // Assign port in network byte order
}

// Socket Code

Socket::Socket(int type, int protocol) {
  #ifdef WIN32
    if (!initialized) {
      WORD wVersionRequested;
      WSADATA wsaData;

      wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
      if (WSAStartup(wVersionRequested, &wsaData) != 0) {  // Load WinSock DLL
        cout << "Unable to load WinSock DLL" << endl;
	initialized = false;
      }
      initialized = true;
    }
  #endif

  // Make a new socket
  if ((sockDesc = socket(PF_INET, type, protocol)) < 0) {
    cout << "Socket creation failed (socket()"; 
  }
} 

Socket::Socket(int sockDesc) {
  this->sockDesc = sockDesc;
}

Socket::~Socket() {
  #ifdef WIN32
    ::closesocket(sockDesc);
  #else
    ::close(sockDesc);
  #endif
  sockDesc = -1;
} 

string Socket::getLocalAddress() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);

  if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
    cout << "Fetch of local address failed (getsockname())" << endl;
  }
  return inet_ntoa(addr.sin_addr);
}

unsigned short Socket::getLocalPort() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);

  if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
    cout << "Fetch of local port failed (getsockname())" << endl;
  }
  return ntohs(addr.sin_port);
}

void Socket::setLocalPort(unsigned short localPort) {
  // Bind the socket to its port
  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(localPort);

  if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
    cout << "Set of local port failed (bind())" << endl;
  }
}

void Socket::setLocalAddressAndPort(const string &localAddress,
    unsigned short localPort) {
  // Get the address of the requested host
  sockaddr_in localAddr;
  fillAddr(localAddress, localPort, localAddr);

  if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
    cout << "Set of local address and port failed (bind())" << endl;
  }
}

void Socket::cleanUp() {
  #ifdef WIN32
    if (WSACleanup() != 0) {
      cout << "WSACleanup() failed" << endl;
    }
  #endif
}

unsigned short Socket::resolveService(const string &service,
                                      const string &protocol) {
  struct servent *serv;        /* Structure containing service information */

  if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL)
    return atoi(service.c_str());  /* Service is port number */
  else 
    return ntohs(serv->s_port);    /* Found port (network byte order) by name */
}

// CommunicatingSocket Code

CommunicatingSocket::CommunicatingSocket(int type, int protocol)  
    throw() : Socket(type, protocol) {
}

CommunicatingSocket::CommunicatingSocket(int newConnSD) : Socket(newConnSD) {
}

void CommunicatingSocket::connect(const string &foreignAddress,
    unsigned short foreignPort) {
  // Get the address of the requested host
  sockaddr_in destAddr;
  fillAddr(foreignAddress, foreignPort, destAddr);

  // Try to connect to the given port
  if (::connect(sockDesc, (sockaddr *) &destAddr, sizeof(destAddr)) < 0) {
    cout << "Connect failed (connect())" << endl;
  }
}

void CommunicatingSocket::send(const void *buffer, int bufferLen) 
    {
  if (::send(sockDesc, (raw_type *) buffer, bufferLen, 0) < 0) {
    cout << "Send failed (send())" << endl;
  }
}

int CommunicatingSocket::recv(void *buffer, int bufferLen) 
    {
  int rtn;
  if ((rtn = ::recv(sockDesc, (raw_type *) buffer, bufferLen, 0)) < 0) {
    cout << "Received failed (recv())" << endl;
  }

  return rtn;
}

string CommunicatingSocket::getForeignAddress() 
    {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);

  if (getpeername(sockDesc, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0) {
    cout << "Fetch of foreign address failed (getpeername())" << endl;
  }
  return inet_ntoa(addr.sin_addr);
}

unsigned short CommunicatingSocket::getForeignPort() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);

  if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
    cout << "Fetch of foreign port failed (getpeername())" << endl;
  }
  return ntohs(addr.sin_port);
}

// TCPSocket Code

TCPSocket::TCPSocket() 
    throw() : CommunicatingSocket(SOCK_STREAM, 
    IPPROTO_TCP) {
}

TCPSocket::TCPSocket(const string &foreignAddress, unsigned short foreignPort)
    throw() : CommunicatingSocket(SOCK_STREAM, IPPROTO_TCP) {
  connect(foreignAddress, foreignPort);
}

TCPSocket::TCPSocket(int newConnSD) : CommunicatingSocket(newConnSD) {
}

// TCPServerSocket Code

TCPServerSocket::TCPServerSocket(unsigned short localPort, int queueLen) 
    throw() : Socket(SOCK_STREAM, IPPROTO_TCP) {
  setLocalPort(localPort);
  setListen(queueLen);
}

TCPServerSocket::TCPServerSocket(const string &localAddress, 
    unsigned short localPort, int queueLen) 
    throw() : Socket(SOCK_STREAM, IPPROTO_TCP) {
  setLocalAddressAndPort(localAddress, localPort);
  setListen(queueLen);
}

TCPSocket *TCPServerSocket::accept() {
  int newConnSD;
  if ((newConnSD = ::accept(sockDesc, NULL, 0)) < 0) {
    cout << "Accept failed (accept())" << endl;
  }

  return new TCPSocket(newConnSD);
}

void TCPServerSocket::setListen(int queueLen) {
  if (listen(sockDesc, queueLen) < 0) {
    cout << "Set listening socket failed (listen())" << endl;
  }
}

// UDPSocket Code

UDPSocket::UDPSocket() : CommunicatingSocket(SOCK_DGRAM,
    IPPROTO_UDP) {
  setBroadcast();
}

UDPSocket::UDPSocket(unsigned short localPort)  : 
    CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {

  cout << "UDPSocket::UDPSocket: " << localPort << endl;
  setLocalPort(localPort);
  setBroadcast();
}

UDPSocket::UDPSocket(const string &localAddress, unsigned short localPort) 
     : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
  setLocalAddressAndPort(localAddress, localPort);
  setBroadcast();
}

void UDPSocket::setBroadcast() {
  // If this fails, we'll hear about it when we try to send.  This will allow 
  // system that cannot broadcast to continue if they don't plan to broadcast
  int broadcastPermission = 1;
  setsockopt(sockDesc, SOL_SOCKET, SO_BROADCAST, 
             (raw_type *) &broadcastPermission, sizeof(broadcastPermission));
}

void UDPSocket::disconnect() {
  sockaddr_in nullAddr;
  memset(&nullAddr, 0, sizeof(nullAddr));
  nullAddr.sin_family = AF_UNSPEC;

  // Try to disconnect
  if (::connect(sockDesc, (sockaddr *) &nullAddr, sizeof(nullAddr)) < 0) {
   #ifdef WIN32
    if (errno != WSAEAFNOSUPPORT) {
   #else
    if (errno != EAFNOSUPPORT) {
   #endif
      cout << "Disconnect failed (connect())" << endl;
    }
  }
}

void UDPSocket::sendTo(const void *buffer, int bufferLen, 
    const string &foreignAddress, unsigned short foreignPort) 
    {
  sockaddr_in destAddr;
  fillAddr(foreignAddress, foreignPort, destAddr);

  // Write out the whole buffer as a single message.
  if (sendto(sockDesc, (raw_type *) buffer, bufferLen, 0,
             (sockaddr *) &destAddr, sizeof(destAddr)) != bufferLen) {
    cout << "Send failed (sendto())" << endl;
  }
}

int UDPSocket::recvFrom(void *buffer, int bufferLen, string &sourceAddress,
    unsigned short &sourcePort) {
  sockaddr_in clntAddr;
  socklen_t addrLen = sizeof(clntAddr);
  int rtn;

/*EDM timeout stuff added */

  fd_set sd_set;
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  int select_rtn;

   FD_ZERO(&sd_set);
   FD_SET(sockDesc, &sd_set);

#ifdef OFF
  select_rtn = select(1, &sd_set, 0, &sd_set, &timeout); /* allows termination in case of time out */

  cout << "recvFrom - select_rtn: " << select_rtn << endl;

  if(select_rtn < 1)
    return -1;
#endif
  if ((rtn = recvfrom(sockDesc, (raw_type *) buffer, bufferLen, 0, 
                      (sockaddr *) &clntAddr, (socklen_t *) &addrLen)) < 0) {
    cout << "Receive failed (recvfrom())" << endl;
  }
  sourceAddress = inet_ntoa(clntAddr.sin_addr);
  sourcePort = ntohs(clntAddr.sin_port);

  return rtn;
}

void UDPSocket::setMulticastTTL(unsigned char multicastTTL) {
  if (setsockopt(sockDesc, IPPROTO_IP, IP_MULTICAST_TTL, 
                 (raw_type *) &multicastTTL, sizeof(multicastTTL)) < 0) {
    cout << "Multicast TTL set failed (setsockopt())" << endl;
  }
}

void UDPSocket::joinGroup(const string &multicastGroup) {
  struct ip_mreq multicastRequest;

  multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
  multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(sockDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                 (raw_type *) &multicastRequest, 
                 sizeof(multicastRequest)) < 0) {
    cout << "Multicast group join failed (setsockopt())" << endl;
  }
}

void UDPSocket::leaveGroup(const string &multicastGroup) {
  struct ip_mreq multicastRequest;

  multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
  multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(sockDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
                 (raw_type *) &multicastRequest, 
                 sizeof(multicastRequest)) < 0) {
    cout << "Multicast group leave failed (setsockopt())" << endl;
  }
}
