#include "harq.hpp"
#include "socket.hpp"
#include "debugs.hpp"

#include "wire.pb.h"

#include <iostream>

#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

WriteStatus Socket::write(const wire::Message& msg) {
  std::string out;

  if(!msg.SerializeToString(&out)) {
    std::cerr << "Error serializing message\n";
    return eFailure;
  }

  return write_raw(out);
}

WriteStatus Socket::write_raw(const std::string val) {
  union sz {
    char buf[4];
    uint32_t i;
  } sz;

  sz.i = htonl(val.size());

  writes_.add(std::string(sz.buf,4));
  writes_.add(val);

  WriteStatus stat = writes_.flush(fd);

  switch(stat) {
  case eOk:
    debugs << "Writes flushed successfully\n";
    break;
  case eWouldBlock:
    debugs << "Writes would have blocked, NOT fully flushed\n";
    break;
  case eFailure:
    debugs << "Writes failed, socket busted\n";
    break;
  }

  return stat;
}

void Socket::write_block(const wire::Message& msg) {
  WriteStatus stat = write(msg);
  while(stat != eOk) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

#ifdef SIMULATE_BAD_NETWORK
    usleep(250000);
#endif

    select(fd+1, 0, &fds, 0, 0);
    stat = flush();
  }
}

bool Socket::read_block(wire::Message& msg) {
  union sz {
    char buf[4];
    uint32_t i;
  } sz;

  ssize_t got = 0;

  do {
    int r = recv(fd, sz.buf+got, 4-got, 0);
    if(r == 0) return false;
    got += r;
  } while(got < 4);

  char buf[1024];
  char* msg_buf = buf;

  assert(got < 1024);

  uint32_t need = ntohl(sz.i);
  got = 0;

  do {
    int r = recv(fd, msg_buf+got, need-got, 0);
    if(r == 0) return false;
    got += r;
  } while(got < need);

  if(!msg.ParseFromString(std::string(msg_buf, need))) return false;

  return true;
}

void Socket::set_nonblock() {
  int flags = fcntl(fd, F_GETFL, 0);
  int r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  assert(0 <= r && "Setting socket non-block failed!");
}

