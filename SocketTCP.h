#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "server_errors.h"

struct client_disconnected : public std::exception {
    const char * what () const throw () {
        return "read error - len of size 0 - client has ended connection";
    }
};

class SocketTCP {
public:
    using file_descriptor = int;
    SocketTCP() {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if(sock < 0)
            throw std::system_error(errno, std::system_category(), "socket()");
    }

    SocketTCP(int sock_num) { // tworzona po wywołaniu accept
        sock = sock_num;
    }

    SocketTCP(const SocketTCP& other) = delete;
    SocketTCP& operator=(const SocketTCP& other) = delete;

    SocketTCP(SocketTCP&& other){
        sock = other.sock;
        other.sock = NO_FILE_DESCRIPTOR;
    }

    SocketTCP& operator=(SocketTCP&& other) noexcept {
        if(sock != other.sock){
            sock = other.sock;
            other.sock = NO_FILE_DESCRIPTOR;
        }
        return *this;
    }

    ~SocketTCP(){ // FIXME: exception!!
        if(sock != NO_FILE_DESCRIPTOR){
            if(close(sock) < -1)
                throw std::system_error(errno, std::system_category(), "close()");
            std::cout << "ending connection\n";
        }
    }

    void socket_bind(struct sockaddr * server_address, size_t bytes){
        if (bind(sock, server_address, bytes) < 0)
            throw std::system_error(errno, std::system_category(), "bind()");
    }

    void socket_listen(int queue_length){
        if (listen(sock, queue_length) < 0)
            throw std::system_error(errno, std::system_category(), "listen()");
    }

    SocketTCP socket_accept(struct sockaddr_in * client_address, socklen_t * client_address_len) {
        int msg_sock = accept(sock, (struct sockaddr *) client_address, client_address_len);
        if (msg_sock < 0)
            throw std::system_error(errno, std::generic_category(), "accept()");
        return SocketTCP(msg_sock);
    }

    ssize_t socket_read(char* buffer, ssize_t buffer_size) {
        ssize_t len = read(sock, buffer, buffer_size);
        if (len < 0)
            throw error500();
        else if  (len == 0)
            throw client_disconnected();
        else
            return len;
    }

    void socket_write(char * buffer, ssize_t len) {
        ssize_t snd_len = write(sock, buffer, len);
        if (snd_len < len)
            throw client_disconnected();
//        if (snd_len != len)
//            throw error500();
    }

private:
    static constexpr int NO_FILE_DESCRIPTOR = -1;
    file_descriptor sock;
};

#endif /* TCP_SOCKET_H */