#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <string>

class Session
{
  using Socket = boost::asio::ip::tcp::socket;
private:
  Session() = delete;
  void operator=(Session) = delete;

  Session(Socket&& peer):
    sock_{std::move(peer)}
  {
  }

  static auto echo(std::unique_ptr<Session> self, const boost::system::error_code& ec, std::size_t bytes) {
    self->buffer_.resize(bytes);
    std::cout << self->buffer_;
    return boost::asio::deferred_t::when(!ec)
      .then(boost::asio::async_write(self->sock_, boost::asio::buffer(self->buffer_),
                                     boost::asio::prepend(boost::asio::deferred, std::move(self))))
      .otherwise(boost::asio::deferred_t::values(std::move(self), ec, 0));
  }

  static void serve(std::unique_ptr<Session> self) {
    self->buffer_.resize(512);
    self->sock_.async_read_some(boost::asio::buffer(self->buffer_),
                                boost::asio::prepend(boost::asio::deferred, std::move(self)))
      | boost::asio::deferred(Session::echo)
      | [] (auto&& self, auto ec, auto bytes) {
        if (!ec) {
          Session::serve(std::move(self));
        } else {
          std::cout << "ERROR: " << ec.message() << '\n';
        }
      };
  }

public:
  static void fire(Socket&& peer) {
    Session::serve(std::make_unique<Session>(std::move(peer)));
  }

private: 
  friend std::unique_ptr<Session> std::make_unique<Session>(Socket&&);
  Socket sock_;
  std::string buffer_;
};


class Server
{
  using Acceptor = boost::asio::ip::tcp::acceptor;
public:
  Server(boost::asio::io_context& ctx, uint16_t port):
    acceptor_{ctx, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), port}}
  {
    start_listen();
  }

  void start_listen() {
    acceptor_.async_accept([this](const boost::system::error_code& ec, auto&& socket) {
      if (!ec) {
        Session::fire(std::move(socket));
      }
      start_listen();
    });
  }

private:
  Acceptor acceptor_;
};


int main(int argc, char *argv[])
{
  boost::asio::io_context ctx;
  boost::asio::signal_set signals(ctx, SIGINT, SIGTERM);

  signals.async_wait([&ctx](const boost::system::error_code& ec, int sig_num) {
    ctx.stop();
  });
  Server server{ctx, 1234};
  ctx.run();
  
  return 0;
}
