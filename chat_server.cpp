#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read_until.hpp>

#include <iostream>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>
#include <thread>

using namespace boost::asio;
using tcp = ip::tcp;

class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket, 
                std::shared_ptr<std::shared_mutex> sessions_mutex,
                std::shared_ptr<std::unordered_set<std::shared_ptr<ChatSession>>> sessions)
        : socket_(std::move(socket)), 
          sessions_mutex_(sessions_mutex), 
          sessions_(sessions) {
        username_.reserve(64);
    }

    void start() {
        send_prompt("type your name: ");
        do_read();
    }

private:
    void do_read() {
        boost::asio::co_spawn(socket_.get_executor(),
            [self = shared_from_this()]() -> awaitable<void> {
                try {
                    while (true) {
                        streambuf buffer;
                        auto n = co_await async_read_until(self->socket_, buffer, '\n', use_awaitable);
                        
                        std::istream is(&buffer);
                        std::string line;
                        std::getline(is, line);
                        
                        if (self->username_.empty()) {
                            self->username_ = line.empty() ? "Anonymous" : line;
                            {
                                std::unique_lock lock(*self->sessions_mutex_);
                                self->sessions_->insert(self->shared_from_this());
                            }
                            std::cout << ">>> " << self->username_ << " подключился (" 
                                      << self->sessions_->size() << " онлайн)" << std::endl;
                            self->broadcast(std::string(">>> ") + self->username_ + std::string(" joined the chat\n"));
                            self->send_prompt(std::string("[") + self->username_ + std::string("]> "));
                        } else if (line == "quit" || line == "/quit") {
                            co_return;
                        } else {
                            self->broadcast(self->username_ + std::string(": ") + line + std::string("\n"));
                        }
                    }
                } catch (...) {
                }
                
                {
                    std::unique_lock lock(*self->sessions_mutex_);
                    self->sessions_->erase(self->shared_from_this());
                }
                self->broadcast(std::string(">>> ") + self->username_ + std::string(" left the chat\n"));
                std::cout << ">>> " << self->username_ << " отключился (" 
                          << self->sessions_->size() << " онлайн)" << std::endl;
                co_return;
            }, detached);
    }

    void send_prompt(std::string_view prompt) {
        boost::asio::co_spawn(socket_.get_executor(),
            [self = shared_from_this(), prompt = std::string(prompt)]() -> awaitable<void> {
                try {
                    co_await async_write(self->socket_, buffer(prompt), use_awaitable);
                } catch (...) {}
            }, detached);
    }

    void broadcast(std::string message) {
        std::vector<std::weak_ptr<ChatSession>> active_sessions;
        {
            std::shared_lock lock(*sessions_mutex_);
            active_sessions.reserve(sessions_->size());
            active_sessions.assign(sessions_->begin(), sessions_->end());
        }

        for (auto& weak : active_sessions) {
            if (auto session = weak.lock()) {
                boost::asio::co_spawn(session->socket_.get_executor(),
                    [session, msg = std::move(message)]() mutable -> awaitable<void> {
                        try {
                            co_await async_write(session->socket_, buffer(msg), use_awaitable);
                        } catch (...) {}
                    }, detached);
            }
        }
    }

    std::string username_;
    tcp::socket socket_;
    std::shared_ptr<std::shared_mutex> sessions_mutex_;
    std::shared_ptr<std::unordered_set<std::shared_ptr<ChatSession>>> sessions_;
};

class ChatServer {
public:
    ChatServer(io_context& ioc, short port)
        : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)),
          sessions_mutex_(std::make_shared<std::shared_mutex>()),
          sessions_(std::make_shared<std::unordered_set<std::shared_ptr<ChatSession>>>()) {
        std::cout << "🔥 Elite Chat Server v2.0 запущен на порту " << port << std::endl;
        std::cout << "📡 Ожидание подключений...\n" << std::endl;
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<ChatSession>(std::move(socket), sessions_mutex_, sessions_);
                session->start();
                std::cout << "🔗 Новое подключение #" << sessions_->size() << std::endl;
            }
            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    std::shared_ptr<std::shared_mutex> sessions_mutex_;
    std::shared_ptr<std::unordered_set<std::shared_ptr<ChatSession>>> sessions_;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "💻 Использование: " << argv[0] << " <port>\n";
        return 1;
    }

    try {
        io_context ioc;
        auto threads = std::max(2u, std::thread::hardware_concurrency());
        
        signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](auto, auto) {
            std::cout << "\n🛑 Graceful shutdown..." << std::endl;
            ioc.stop();
        });

        ChatServer server(ioc, std::stoi(argv[1]));

        std::vector<std::thread> workers(threads);
        for (auto& t : workers) {
            t = std::thread([&ioc] { ioc.run(); });
        }
        for (auto& t : workers) {
            t.join();
        }
    } catch (std::exception& e) {
        std::cerr << "💥 Ошибка: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
