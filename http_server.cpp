#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <system_error>

#include "err.h"

#include <regex>
#include <string>
#include <string.h>
#include <iostream>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

#include <sstream>
#include <csignal>
#include "SocketTCP.h"
#include "BufferHTTP.h"
#include "MessageHTTP.h"
#include "ResponseHTTP.h"
#include "server_errors.h"
#include "definitions.h"

bool is_in_good_folder(const std::string& path) {
    int double_dots = 0;
    int folders = 0;
    std::stringstream path_ss = std::stringstream(path);
    std::string folder;
    while (path_ss.good()) {
        getline(path_ss, folder, '/');
        if (folder == "..")
            double_dots++;
        else if (!folder.empty())
            folders++;
        if (folders < double_dots)
            return false;
    }
    return true;
}

bool find_file(const std::string& home_path, const std::string& target, std::filesystem::path& full_path) {
    if (!is_in_good_folder(target))
        return false;
    full_path = home_path + target;
    try {
        if (!fs::exists(full_path))
            return false;
        if (fs::is_directory(full_path))
            return false;
    } catch (fs::filesystem_error& e) {
        return false;
    }
    std::fstream file(full_path);
    return file.is_open();
}

void send_file_content(const fs::path& path, char* send_buffer, int buffer_size, SocketTCP& socket) {
    std::fstream file(path);
    if (!file.is_open()) {
        throw error500();
    }
    while(!file.eof()) {
        file.read(send_buffer, buffer_size);
        socket.socket_write(send_buffer, file.gcount());
    }
}

bool find_correlated_path(const std::string& target, std::string& server_info, std::map<std::string, std::string>& correlated_map) {
    auto it = correlated_map.find(target);
    if (it == correlated_map.end())
        return false;
    server_info = it->second;
    return true;
}

ResponseHTTP create_response(const MessageHTTP& client_msg, std::string& home_path, std::map<std::string, std::string>& correlated_map) {
    ResponseHTTP response;

    if (client_msg.getConnectionFlag() &&
        MessageHTTP::strcmp_case_insensitive(client_msg.getConnectionFieldValue(), "close")) {
        response.add_header("connection", "close");
        response.setConnectionClose(true);
    }

    const std::string& method = client_msg.getRequestMethod();
    if (method != "GET" && method != "HEAD") {
        response.setStatusCode(501);
        response.setReasonPhrase("Request out of scope of the server's functions");
        return response;
    }
    std::filesystem::path full_path;
    const std::string& target = client_msg.getRequestTarget();
    if (!regex_match(target, std::regex("[a-zA-Z0-9\\.\\-/]*")) || !is_in_good_folder(target)) {
        response.setStatusCode(404);
        response.setReasonPhrase("File not found");
    }
    else {
        if (find_file(home_path, target, full_path)) {
            response.setStatusCode(200);
            response.setReasonPhrase("File was found - great success!");
            std::string field_value_str = std::to_string(std::filesystem::file_size(full_path));
            response.add_header("content-length", field_value_str);
            response.add_header("content-type", "application/octet-stream");
            if (method == "GET") {
                response.setSendFileFlag(true);
                response.setFileToSend(full_path);
            }
        } else {
            response.setSendFileFlag(false);
            std::string server_info;
            if (find_correlated_path(target, server_info, correlated_map)) {
                response.setStatusCode(302);
                response.setReasonPhrase("File can be found on a correlated server");
                response.add_header("location", "http://" + server_info + target);
            } else {
                response.setStatusCode(404);
                response.setReasonPhrase("File not found");
            }
        }
    }

    return response;
}

void create_corr_server_map(std::string& file_path, std::map<std::string, std::string>& map) { // TODO przetestuj
    std::fstream file(file_path);
    if (!file.is_open()) {
        exit(EXIT_FAILURE);
    }
    std::string path;
    std::string ip;
    std::string port;
    std::string dummy;
    while (!file.eof()) {
        std::getline(file, path, '\t');
        std::getline(file, ip, '\t');
        std::getline(file, port, '\n');
        map.insert(std::pair<std::string, std::string>(path, ip + ":" + port));
    }
}

int main(int argc, char *argv[]) {
   if (argc < 3 || argc > 4) {
        exit(EXIT_FAILURE);
    }
    std::string HOME_PATH = argv[1];  //(".");
    std::string CORR_SERVER_PATH = argv[2]; //("./related.txt");
    int PORT_NUM = 8080;
    if (argc == 4) {
        try {
            PORT_NUM = std::stoi(argv[3]);
        } catch (std::exception &e) {
            exit(EXIT_FAILURE);
        }
    }
    signal(SIGPIPE, SIG_IGN);
    std::map<std::string, std::string> correlated_servers;
    create_corr_server_map(CORR_SERVER_PATH, correlated_servers);
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_len;

    char tcp_buffer[BUFFER_SIZE + 1];
    char send_buffer[BUFFER_SIZE];

    BufferHTTP http_buffer = BufferHTTP();
    SocketTCP socket = SocketTCP();

    // after socket() call; we should close(sock) on any execution path;
    // since all execution paths exit immediately, sock would be closed when program terminates

    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    server_address.sin_port = htons(PORT_NUM); // listening on port PORT_NUM

    // bind the socket to a concrete address
    socket.socket_bind((struct sockaddr *) &server_address, sizeof(server_address));

    // switch to listening (passive open)
    socket.socket_listen(QUEUE_LENGTH);

    printf("accepting client connections on port %hu\n", ntohs(server_address.sin_port));
    for (;;) {
        client_address_len = sizeof(client_address);
        // get client connection from the socket
        SocketTCP msg_socket = socket.socket_accept(&client_address, &client_address_len);
        MessageHTTP message_http;
        bool close = false;
        while (!close) {
            try {
                try {
                    std::string msg_str = http_buffer.get_line(msg_socket, tcp_buffer);
                    int valid_message = message_http.read_next_line(msg_str); // @throw error400 jeśli zły format
                    if (valid_message == 1) {
                        ResponseHTTP response = create_response(message_http, HOME_PATH, correlated_servers);
                        close = response.send_response(msg_socket, send_buffer, BUFFER_SIZE);
                        //send_file_content(HOME_PATH, message_http.getRequestMethod(), message_http.getRequestTarget(), send_buffer, BUFFER_SIZE, msg_socket);
                        message_http.reset_flags();
                        // ważne - po zresetowaniu traktujemy tą wiadomość jako usuniętą
                        // i message_http będzie nadpisywał pola nowuymi wartościami
                    }
                }
                catch (const error400 &e) {
                    ResponseHTTP response400;
                    response400.setStatusCode(400);
                    response400.setReasonPhrase("Message format not supported");
                    response400.setConnectionClose(true);
                    response400.add_header("connection", "close");
                    response400.setSendFileFlag(false);
                    response400.send_response(msg_socket, send_buffer, BUFFER_SIZE);
                    message_http.reset_flags();
                    close = true;
                }
                catch (const error500 &e) {
                    ResponseHTTP response500;
                    response500.setStatusCode(500);
                    response500.setReasonPhrase("Error on the server's side");
                    response500.setConnectionClose(true);
                    response500.add_header("connection", "close");
                    response500.setSendFileFlag(false);
                    response500.send_response(msg_socket, send_buffer, BUFFER_SIZE);
                    message_http.reset_flags();
                    close = true;
                }

            }
            catch (const client_disconnected & e) {
                close = true; // powinien wywołać się destruktor obiektu msg_socket przy następnym przypisaniu
            }
        }
    }
    return 0;
}