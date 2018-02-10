/* XMRig
 * Copyright 2018      Sebastian Stolzenberg <https://github.com/sebastianstolzenberg>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>

#include "net/BoostTlsConnection.h"

class BoostTlsConnection : public Connection
{
public:
    BoostTlsConnection(const ConnectionListener::Ptr& listener,
                       const std::string& server, uint16_t port)
            : Connection(listener)
            , sslContext_(boost::asio::ssl::context::sslv23)
            , socket_(ioService_, sslContext_)
    {
        boost::asio::ip::tcp::resolver resolver(ioService_);
        boost::asio::ip::tcp::resolver::query query(server, std::to_string(port));
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

        socket_.set_verify_mode(boost::asio::ssl::verify_none);

        boost::asio::connect(socket_.lowest_layer(), iterator);
        socket_.handshake(boost::asio::ssl::stream_base::client);

        triggerRead();

//        boost::thread(boost::bind(&boost::asio::io_service::run, &ioService_)).detach();
        boost::thread(boost::bind(&BoostTlsConnection::threadFunction, this)).detach();
    }

    ~BoostTlsConnection()
    {
    }

    virtual bool send(const void* data, std::size_t size)
    {
        boost::system::error_code error;
        boost::asio::write(socket_, boost::asio::buffer(data, size), error);
        return !error;
    }

    void triggerRead()
    {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(receiveBuffer_, sizeof(receiveBuffer_)),
                                boost::asio::transfer_at_least(1),
                                boost::bind(&BoostTlsConnection::handleRead, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

    void handleRead(const boost::system::error_code& error,
                    size_t bytes_transferred)
    {
        if (!error)
        {
            std::cout << "Reply: ";
            std::cout.write(receiveBuffer_, bytes_transferred);
            std::cout << "\n";
            notifyRead(receiveBuffer_, bytes_transferred);
            triggerRead();
        }
        else
        {
            std::cout << "Read failed: " << error.message() << "\n";
            notifyError();
        }
    }

    void threadFunction()
    {
        std::cout << "Starting io_service\n";
        ioService_.run();
        std::cout << "Stopped io_service\n";
    }

private:
    boost::asio::io_service ioService_;
    boost::asio::ssl::context sslContext_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    char receiveBuffer_[2048];
};

Connection::Ptr establishBoostTlsConnection(const ConnectionListener::Ptr& listener,
                                            const std::string& host, uint16_t port)
{
    return std::make_shared<BoostTlsConnection>(listener, host, port);
}