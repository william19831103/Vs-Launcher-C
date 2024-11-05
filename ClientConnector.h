#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>

// 只在未定义时定义这些宏
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <asio.hpp>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>
#include "opcodes.h"

// 保留 ClientInfo 单例
class ClientInfo {
public:
    static ClientInfo* instance() {
        static ClientInfo instance;
        return &instance;
    }

    std::string ip;
    int port;
    std::string name;
    std::string notice;
    bool isConnected = false;
    std::string response;
};
#define sClientInfo ClientInfo::instance()

using asio::ip::tcp;

class TcpClient {
public:
    static TcpClient* instance() {
        static TcpClient instance;
        return &instance;
    }

    // 删除拷贝构造函数和赋值运算符
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    void connect(const std::string& host, const std::string& port);
    void send_message(uint16_t opcode, const std::string& data = "");
    void show_menu();
    bool is_connected() const { return socket_.is_open(); }
    asio::io_context& get_io_context() { return io_context_; }

private:
    // 私有构造函数
    TcpClient() : socket_(io_context_), receiving_file_(false), 
                  file_size_(0), expected_file_count_(0), 
                  received_file_count_(0) {}

    // 其他私有成员和方法...
    void do_read_header();
    void handle_message_header(const MessageHeader& header);
    void handle_message(uint16_t opcode, const std::vector<char>& data);
    void handle_patch_info(const std::vector<char>& data);
    void handle_patch_file(const std::vector<char>& data);
    void handle_patch_file_end(const std::vector<char>& data);
    void request_next_patch();
    bool is_standard_patch_format(const std::string& filename);
    bool is_single_char_patch_format(const std::string& filename);
    // 解析服务器信息
    void parse_server_info(const std::string& info);

private:
    static constexpr size_t CHUNK_SIZE = 8192;
    
    asio::io_context io_context_;
    tcp::socket socket_;
    bool receiving_file_;
    size_t file_size_;
    std::string current_filename_;
    size_t expected_file_count_;
    size_t received_file_count_;
    std::vector<std::string> needed_updates_;
    
    struct FileReceiveContext {
        std::string filename;
        size_t totalSize;
        size_t receivedSize;
        std::ofstream file;
    };
    std::shared_ptr<FileReceiveContext> current_file_;
};

#define sClient TcpClient::instance()

