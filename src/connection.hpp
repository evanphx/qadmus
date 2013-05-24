#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <vector>
#include <list>
#include <string>
#include <map>

#include <ev++.h>
#include <leveldb/c.h>

#include "harq.hpp"
#include "buffer.hpp"
#include "socket.hpp"

class Server;

namespace wire {
  class Message;
  class Action;
}

class Connection {
  std::list<std::string> subscriptions_;
  bool tap_;
  bool ack_;
  bool confirm_;
  bool closing_;
  Socket sock_;
  ev::io read_w_;
  ev::io write_w_;

  typedef std::map<uint64_t, wire::Message> AckMap;
  AckMap to_ack_;

public:
  bool open;
  Server *server;

  Buffer buffer_;

  enum State { eReadSize, eReadMessage } state;

  int need;

  bool writer_started;

  /*** methods ***/

  Connection(Server *s, int fd);
  ~Connection();

  Buffer& buffer() {
    return buffer_;
  }

  void on_readable(ev::io& w, int revents);
  void on_writable(ev::io& w, int revents);

  void start();
  bool do_read(int revents);

  void handle_message(wire::Message& msg);
  void handle_action(wire::Action& act);

  bool deliver(wire::Message& msg);

  void write(wire::Message& msg);

  void clear_ack(uint64_t id);
};

#endif
