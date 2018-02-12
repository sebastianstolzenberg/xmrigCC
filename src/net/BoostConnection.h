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

#ifndef __BOOSTCONNECTION_H__
#define __BOOSTCONNECTION_H__

#include "net/Connection.h"

template <class SOCKET>
class BoostConnection : public Connection
{
public:
    BoostConnection(const ConnectionListener::Ptr& listener,
                    const std::string& server, uint16_t port)
            : Connection(listener)
            , socket_(ioService_)
    {
        std::cout << "Connecting BoostConnection ... ";
        boost::asio::ip::tcp::resolver resolver(ioService_);
        boost::asio::ip::tcp::resolver::query query(server, std::to_string(port));
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

        socket_.connect(iterator);

        triggerRead();

        std::thread([this]() { ioService_.run(); }).detach();
        std::cout << " success\n";
    }

    ~BoostConnection()
    {
    }

    virtual bool connected()
    {
        return socket_.get().lowest_layer().is_open();

    }

    virtual bool send(const char* data, std::size_t size)
    {
        std::cout << "Send: ";
        std::cout.write(data, size);
        std::cout << "\n";

        boost::system::error_code error;
        boost::asio::write(socket_.get(), boost::asio::buffer(data, size), error);
        if (error)
        {
            notifyError(error.message());
        }
        return !error;
    }

    void triggerRead()
    {
        boost::asio::async_read(socket_.get(),
                                boost::asio::buffer(receiveBuffer_, sizeof(receiveBuffer_)),
                                boost::asio::transfer_at_least(1),
                                boost::bind(&BoostConnection::handleRead, this,
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
            notifyError(error.message());
        }
    }

private:
    boost::asio::io_service ioService_;
    SOCKET socket_;
    char receiveBuffer_[2048];
};

#endif /* __BOOSTCONNECTION_H__ */
