#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <string>

class Session : public std::enable_shared_from_this<Session>
{
  using Socket = boost::asio::ip::tcp::socket;
public:
  Session(Socket&& peer):
    sock_{std::move(peer)}
  {
  }

  static auto echo(std::shared_ptr<Session> self, const boost::system::error_code& ec, std::size_t bytes) {
    self->buffer_.resize(bytes);
    std::cout << self->buffer_;
    return boost::asio::deferred_t::when(!ec)
      .then(boost::asio::async_write(self->sock_, boost::asio::buffer(self->buffer_), boost::asio::prepend(boost::asio::deferred, self)))
      .otherwise(boost::asio::deferred_t::values(self, ec, 0));
  }

  static void serve(std::shared_ptr<Session> self) {
    self->buffer_.resize(512);
    self->sock_.async_read_some(boost::asio::buffer(self->buffer_),
                                boost::asio::prepend(boost::asio::deferred, self))
      | boost::asio::deferred(Session::echo)
      | [] (auto self, auto ec, auto bytes) {
        if (!ec) {
          Session::serve(self);
        } else {
          std::cout << "ERROR: " << ec.message() << '\n';
        }
      };
  }

  void start() {
    Session::serve(shared_from_this());
  }

private: 
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
        std::make_shared<Session>(std::move(socket))->start();
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
