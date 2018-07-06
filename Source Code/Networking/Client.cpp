#ifndef OPCODE_ENUM
#define OPCODE_ENUM OPcodes
#endif

#include "Headers/Client.h"
#include "Headers/ASIO.h"
#include "Headers/OPCodesTpl.h"

#include <boost/archive/text_iarchive.hpp>

namespace Divide {

void Client::sendPacket(WorldPacket& p)
{
    _packetQueue.push_back(p);
    _heartbeatTimer.expires_at(boost::posix_time::neg_infin);
}

void Client::receivePacket(WorldPacket& p) { _asioPointer->handlePacket(p); }

void Client::start(tcp::resolver::iterator endpoint_iter) {
    start_connect(endpoint_iter);
    _deadline.async_wait(boost::bind(&Client::check_deadline, this));
}

void Client::stop() {
    _stopped = true;
    _socket.close();
    _deadline.cancel();
    _heartbeatTimer.cancel();
}

void Client::start_read() {
    // Set a deadline for the read operation.
    _deadline.expires_from_now(boost::posix_time::seconds(30));
    _header = 0;
    _inputBuffer.consume(_inputBuffer.size());
    // Start an asynchronous operation to read a newline-delimited message.
    boost::asio::async_read(
        _socket, boost::asio::buffer(&_header, sizeof(_header)),
        boost::bind(&Client::handle_read_body, this, _1,
                    boost::asio::placeholders::bytes_transferred));
}

void Client::handle_read_body(const boost::system::error_code& ec,
                              size_t bytes_transfered) {
    ACKNOWLEDGE_UNUSED(bytes_transfered);

    if (_stopped) {
        return;
    }

    if (!ec) {
        _deadline.expires_from_now(boost::posix_time::seconds(30));
        boost::asio::async_read(
            _socket, _inputBuffer.prepare(_header),
            boost::bind(&Client::handle_read_packet, this, _1,
                        boost::asio::placeholders::bytes_transferred));
    } else {
        stop();
    }
}

void Client::handle_read_packet(const boost::system::error_code& ec,
                                size_t bytes_transfered) {
    ACKNOWLEDGE_UNUSED(bytes_transfered);
        
    if (_stopped) {
        return;
    }
    if (!ec) {
        _inputBuffer.commit(_header);
        std::istream is(&_inputBuffer);
        WorldPacket packet;
        try {
            boost::archive::text_iarchive ar(is);
            ar & packet;
        } catch (std::exception& e) {
            if (_debugOutput) {
                std::cout << e.what() << std::endl;
                std::cout << "ASIO: Received invalid packet!" << std::endl;
            }
        }

        if (packet.opcode() == OPCodes::SMSG_SEND_FILE) {
            boost::asio::async_read_until(
                _socket, _requestBuf, "\n\n",
                boost::bind(&Client::handle_read_file, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
        } else {
            receivePacket(packet);
            start_read();
        }
    } else {
        stop();
    }
}

void Client::handle_read_file(const boost::system::error_code& ec,
                              size_t bytes_transfered) {
    ACKNOWLEDGE_UNUSED(ec);

    std::cout << __FUNCTION__ << "(" << bytes_transfered << ")"
              << ", in_avail=" << _requestBuf.in_avail()
              << ", size=" << _requestBuf.size()
              << ", max_size=" << _requestBuf.max_size() << ".\n";
    std::istream request_stream(&_requestBuf);
    stringImpl file_path;
    request_stream >> file_path;
    request_stream >> _fileSize;
    request_stream.read(_buf.c_array(), 2);  // eat the "\n\n"
    std::cout << file_path << " size is " << _fileSize
              << ", tellg=" << request_stream.tellg() << std::endl;
    size_t pos = file_path.find_last_of('\\');
    if (pos != stringImpl::npos) file_path = file_path.substr(pos + 1);
    _outputFile.open(file_path, std::ios_base::binary);
    if (!_outputFile) {
        std::cout << "failed to open " << file_path << std::endl;
        return;
    }
    // write extra bytes to file
    do {
        request_stream.read(_buf.c_array(), (std::streamsize)_buf.size());
        std::cout << __FUNCTION__ << " write " << request_stream.gcount()
                  << " bytes.\n";
        _outputFile.write(_buf.c_array(), request_stream.gcount());
    } while (request_stream.gcount() > 0);

    async_read(_socket, boost::asio::buffer(_buf.c_array(), _buf.size()),
               boost::bind(&Client::handle_read_file_content, this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
}

void Client::handle_read_file_content(const boost::system::error_code& err,
                                      std::size_t bytes_transferred) {
    if (bytes_transferred > 0) {
        _outputFile.write(_buf.c_array(), (std::streamsize)bytes_transferred);
        std::cout << __FUNCTION__ << " recv " << _outputFile.tellp()
                  << " bytes." << std::endl;
        if (_outputFile.tellp() >= (std::streamsize)_fileSize) {
            return;
        }
    }
    if (err) stop();
    start_read();
}

void handle_error(const stringImpl& function_name,
                  const boost::system::error_code& err) {
    std::cout << __FUNCTION__ << " in " << function_name << " due to "
              << err << " " << err.message() << std::endl;
}

void Client::start_write() {
    if (_stopped) {
        return;
    }

    if (_packetQueue.empty()) {
        WorldPacket heart(OPCodes::MSG_HEARTBEAT);
        heart << (I8)0;
        _packetQueue.push_back(heart);
    }

    WorldPacket& p = _packetQueue.front();
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    boost::archive::text_oarchive ar(os);
    ar & p;

    size_t header = buf.size();
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.push_back(buf.data());
    boost::asio::async_write(_socket, buffers,
                             boost::bind(&Client::handle_write, this, _1));
}

void Client::handle_write(const boost::system::error_code& ec) {
    if (_stopped) {
        return;
    }

    if (!ec) {
        _packetQueue.pop_front();
        _heartbeatTimer.expires_from_now(boost::posix_time::seconds(2));
        _heartbeatTimer.async_wait(boost::bind(&Client::start_write, this));
    } else {
        if (_debugOutput) {
            std::cout << "Error on packet: " << ec.message() << "\n";
            stop();
        }
    }
}

void Client::check_deadline() {
    if (_stopped) {
        return;
    }
    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (_deadline.expires_at() <= deadline_timer::traits_type::now()) {
        // The deadline has passed. The socket is closed so that any outstanding
        // asynchronous operations are cancelled.
        _socket.close();

        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is
        // set.
        _deadline.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    _deadline.async_wait(boost::bind(&Client::check_deadline, this));
}

void Client::start_connect(tcp::resolver::iterator endpoint_iter) {
    if (endpoint_iter != tcp::resolver::iterator()) {
        if (_debugOutput) {
            std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";
        }
        // Set a deadline for the connect operation.
        _deadline.expires_from_now(boost::posix_time::seconds(60));

        // Start the asynchronous connect operation.
        _socket.async_connect(
            endpoint_iter->endpoint(),
            boost::bind(&Client::handle_connect, this, _1, endpoint_iter));
    } else {
        // There are no more endpoints to try. Shut down the client.
        stop();
    }
}

void Client::handle_connect(const boost::system::error_code& ec,
                            tcp::resolver::iterator endpoint_iter) {
    if (_stopped) {
        return;
    }
    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!_socket.is_open()) {
        if (_debugOutput) {
            std::cout << "Connect timed out\n";
        }
        // Try the next available endpoint.
        start_connect(++endpoint_iter);
    }

    // Check if the connect operation failed before the deadline expired.
    else if (ec) {
        if (_debugOutput) {
            std::cout << "Connect error: " << ec.message() << "\n";
        }
        // We need to close the socket used in the previous connection attempt
        // before starting a new one.
        _socket.close();

        // Try the next available endpoint.
        start_connect(++endpoint_iter);
    }

    // Otherwise we have successfully established a connection.
    else {
        if (_debugOutput) {
            std::cout << "Connected to " << endpoint_iter->endpoint() << "\n";
        }
        // Start the input actor.
        start_read();

        // Start the heartbeat actor.
        start_write();
    }
}

};  // namespace Divide