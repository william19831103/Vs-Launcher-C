#include "ClientConnector.h"
#include <iostream>
#include <asio.hpp>
#include <string>
#include <thread>
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <iomanip>
#include "opcodes.h"
#include <set>
#include <map>

namespace fs = std::filesystem;

// 辅助函数
std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return s;
}

// TcpClient 成员函数实现
void TcpClient::connect(const std::string &host, const std::string &port)
{
    /*
    sClientInfo->ip = host;
    sClientInfo->port = port;
    */

    tcp::resolver resolver(socket_.get_executor());
    auto endpoints = resolver.resolve(host, port);

    asio::async_connect(socket_, endpoints,
                        [this](std::error_code ec, tcp::endpoint)
                        {
                            if (!ec)
                            {
                                std::cout << "连接成功！" << std::endl;
                                sClientInfo->isConnected = true;
                                do_read_header();
                            }
                            else
                            {
                                std::cout << "连接失败: " << ec.message() << std::endl;
                                sClientInfo->isConnected = false;
                            }
                        });
}

void TcpClient::send_message(uint16_t opcode, const std::string &data)
{
    MessageHeader header{opcode, static_cast<uint32_t>(data.size())};
    auto buffer = std::make_shared<std::vector<char>>(sizeof(header) + data.size());

    memcpy(buffer->data(), &header, sizeof(header));
    if (!data.empty())
    {
        memcpy(buffer->data() + sizeof(header), data.data(), data.size());
    }

    asio::async_write(socket_, asio::buffer(*buffer),
                      [](std::error_code ec, std::size_t /*length*/)
                      {
                          if (ec)
                          {
                              std::cout << "发送消息失败" << std::endl;
                          }
                      });
}

void TcpClient::show_menu()
{
    std::cout << "\n可用命令：" << std::endl;
    std::cout << "1. 获取服务器通知" << std::endl;
    std::cout << "2. 注册账号" << std::endl;
    std::cout << "3. 检查补丁" << std::endl;
    std::cout << "q. 退出" << std::endl;
    std::cout << "请输入命令：";
}

bool TcpClient::is_standard_patch_format(const std::string &filename)
{
    std::string lower_name = to_lower(filename);
    if (lower_name.substr(0, 6) != "patch-" || lower_name.substr(lower_name.length() - 4) != ".mpq")
    {
        return false;
    }
    std::string num_part = lower_name.substr(6, lower_name.length() - 10);
    if (num_part.length() != 1)
    {
        return false;
    }
    char num = num_part[0];
    return num >= '1' && num <= '9';
}

bool TcpClient::is_single_char_patch_format(const std::string &filename)
{
    std::string lower_name = to_lower(filename);
    if (lower_name.substr(0, 6) != "patch-" || lower_name.substr(lower_name.length() - 4) != ".mpq")
    {
        return false;
    }
    return (lower_name.length() == 11);
}

// ... 其他成员函数的实现 ...

void TcpClient::do_read_header()
{
    auto header = std::make_shared<MessageHeader>();
    asio::async_read(socket_, asio::buffer(header.get(), sizeof(MessageHeader)),
                     [this, header](std::error_code ec, std::size_t /*length*/)
                     {
                         if (!ec)
                         {
                             handle_message_header(*header);
                         }
                     });
}

void TcpClient::handle_message_header(const MessageHeader &header)
{
    if (header.size > 0)
    {
        auto buffer = std::make_shared<std::vector<char>>(header.size);
        asio::async_read(socket_, asio::buffer(*buffer),
                         [this, header, buffer](std::error_code ec, std::size_t /*length*/)
                         {
                             if (!ec)
                             {
                                 handle_message(header.opcode, *buffer);
                             }
                             do_read_header();
                         });
    }
    else
    {
        do_read_header();
    }
}
void TcpClient::parse_server_info(const std::string &info)
{
    std::vector<std::string> parts;
    std::string::size_type start = 0;
    std::string::size_type end = 0;

    // 根据分隔符"||"分割字符串
    while ((end = info.find("||", start)) != std::string::npos)
    {
        parts.push_back(info.substr(start, end - start));
        start = end + 2; // 跳过分隔符长度
    }
    // 添加最后一个部分
    parts.push_back(info.substr(start));

    // 确保有足够的部分
    if (parts.size() >= 4)
    {
        sClientInfo->name = parts[0];
        sClientInfo->ip = parts[1];
        try
        {
            sClientInfo->port = std::stoi(parts[2]);
        }
        catch (const std::exception &e)
        {
            std::cout << "解析服务器端口失败: " << e.what() << std::endl;
            sClientInfo->port = 3724; // 设置默认端口
        }
        sClientInfo->notice = parts[3];
        //sClientInfo->isConnected = true;
    }
    else
    {
        std::cout << "服务器信息格式错误" << std::endl;
    }
}

void TcpClient::handle_message(uint16_t opcode, const std::vector<char> &data)
{
    std::cout << "收到消息，操作码: 0x" << std::hex << opcode << std::dec << std::endl;

    switch (opcode)
    {
    case SMSG_SERVER_NOTICE:
    {
        std::string notice(data.begin(), data.end());
        parse_server_info(notice);
        // std::cout << "服务器通知: " << notice << std::endl;
    }
    break;

    case SMSG_REGISTER_RESULT:
        sClientInfo->response=std::string(data.begin(), data.end());
        //std::cout << "注册结果: " << std::string(data.begin(), data.end()) << std::endl;
        break;

    case SMSG_PATCH_INFO:
        handle_patch_info(data);
        break;

    case SMSG_PATCH_FILE:
        handle_patch_file(data);
        break;

    case SMSG_PATCH_FILE_END:
        std::cout << "收到文件结束标记" << std::endl;
        handle_patch_file_end(data);
        break;

    default:
        std::cout << "收到未知响应: 0x" << std::hex << opcode << std::dec << std::endl;
        break;
    }
}

void TcpClient::handle_patch_info(const std::vector<char> &data)
{
    size_t file_count = data.size() / sizeof(PatchFileInfo);
    const PatchFileInfo *files = reinterpret_cast<const PatchFileInfo *>(data.data());

    fs::path rec_dir = fs::current_path() / "Data";
    std::cout << "\n检查补丁更新：" << std::endl;
    std::cout << "发现 " << file_count << " 个服务端补丁文件" << std::endl;

    // 创建服务端文件信息映射
    std::map<std::string, const PatchFileInfo *> server_files;
    for (size_t i = 0; i < file_count; ++i)
    {
        server_files[to_lower(files[i].filename)] = &files[i];
    }

    needed_updates_.clear();

    // 检查本地文件
    if (fs::exists(rec_dir))
    {
        std::cout << "\n检查本地文件..." << std::endl;
        for (const auto &entry : fs::directory_iterator(rec_dir))
        {
            std::string filename = entry.path().filename().string();
            std::string lower_filename = to_lower(filename);

            // 只处理patch-?.mpq格式的文件
            if (is_single_char_patch_format(filename))
            {
                // 如果是patch-1到patch-9格式，直接保留
                if (is_standard_patch_format(filename))
                {
                    std::cout << "保留标准补丁文件: " << filename << std::endl;
                    continue;
                }

                // 检查是否在服务端列表中
                auto it = server_files.find(lower_filename);
                if (it != server_files.end())
                {
                    // 检查文件大小是否匹配
                    auto local_size = fs::file_size(entry.path());
                    if (local_size == it->second->filesize)
                    {
                        std::cout << "文件 " << filename << " 与服务端一致，保留" << std::endl;
                        continue;
                    }
                    std::cout << "文件 " << filename << " 需要更新（大小不匹配）" << std::endl;
                    needed_updates_.push_back(filename);
                }
                else
                {
                    std::cout << "删除未知补丁文件: " << filename << std::endl;
                    try
                    {
                        fs::remove(entry.path());
                    }
                    catch (const std::exception &e)
                    {
                        std::cout << "删除文件失败: " << e.what() << std::endl;
                    }
                }
            }
            // 其他文件直接跳过，不处理
        }
    }

    // 检查服务端文件是否需要下载
    for (const auto &[filename, file_info] : server_files)
    {
        fs::path local_file = rec_dir / file_info->filename;
        if (!fs::exists(local_file))
        {
            bool found = false;
            if (fs::exists(rec_dir))
            {
                for (const auto &entry : fs::directory_iterator(rec_dir))
                {
                    if (to_lower(entry.path().filename().string()) == filename)
                    {
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
            {
                std::cout << "需要下载服务端文件: " << file_info->filename << std::endl;
                needed_updates_.push_back(file_info->filename);
            }
        }
    }

    if (!needed_updates_.empty())
    {
        std::cout << "\n需要更新的文件：" << needed_updates_.size() << " 个" << std::endl;
        for (const auto &file : needed_updates_)
        {
            std::cout << "- " << file << std::endl;
        }
        std::cout << "\n开始下载更新..." << std::endl;
        request_next_patch();
    }
    else
    {
        std::cout << "\n所有补丁都是最新的！" << std::endl;
        sClientInfo->check_patch_path_pass = true;
    }
}

void TcpClient::handle_patch_file(const std::vector<char> &data)
{
    if (needed_updates_.empty())
    {
        std::cout << "错误：收到意外的补丁文件数据" << std::endl;
        return;
    }

    if (!current_file_)
    {
        // 创建新的文件接收上下文
        std::string filename = needed_updates_.front();
        fs::path filepath = fs::current_path() / "Data" / filename;

        try
        {
            fs::create_directories(filepath.parent_path());

            current_file_ = std::make_shared<FileReceiveContext>();
            current_file_->filename = filename;
            current_file_->totalSize = data.size();
            current_file_->receivedSize = 0;
            current_file_->file.open(filepath.string(), std::ios::binary);

            if (!current_file_->file)
            {
                std::cout << "无法创建文件: " << filename << std::endl;
                current_file_.reset();
                return;
            }

            std::cout << "开始接收文件: " << filename
                      << ", 大小: " << current_file_->totalSize << " 字节" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cout << "创建文件时发生错误: " << e.what() << std::endl;
            current_file_.reset();
            return;
        }
    }

    // 写入接收到的数据
    current_file_->file.write(data.data(), data.size());
    current_file_->receivedSize += data.size();

    // 显示进度
    float progress = (float)current_file_->receivedSize / current_file_->totalSize * 100;
    std::cout << "\r接收进度: " << std::fixed << std::setprecision(2)
              << progress << "% (" << current_file_->receivedSize << "/"
              << current_file_->totalSize << " bytes)" << std::flush;
}

void TcpClient::handle_patch_file_end(const std::vector<char> &data)
{
    if (!current_file_)
    {
        std::cout << "错误：收到文件结束标记，但没有正在接收的文件" << std::endl;
        return;
    }

    std::string endMarker(data.begin(), data.end());
    std::cout << "文件结束标记内容: " << endMarker << std::endl;

    current_file_->file.close();
    std::cout << "\n文件 " << current_file_->filename << " 接收完成" << std::endl;

    needed_updates_.erase(needed_updates_.begin());
    current_file_.reset();

    if (!needed_updates_.empty())
    {
        std::cout << "\n准备请求下一个文件..." << std::endl;
        request_next_patch();
    }
    else
    {
        std::cout << "所有补丁更新完成！" << std::endl;
    }
}

void TcpClient::request_next_patch()
{
    if (needed_updates_.empty())
    {
        std::cout << "没有需要更新的文件了" << std::endl;
        return;
    }

    std::string filename = needed_updates_.front();
    std::cout << "\n请求下载文件: " << filename << std::endl;
    send_message(CMSG_REQUEST_PATCH_FILE, filename);
}

bool TcpClient::wait_for_server_notice(int max_retries, int retry_interval_ms, int timeout_ms) {
    auto start_time = std::chrono::steady_clock::now();
    int retries = 0;
    
    while (sClientInfo->notice.empty()) {
        // 检查是否超时
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
        if (elapsed.count() >= timeout_ms) {
            std::cout << "获取服务器通知超时" << std::endl;
            return false;
        }

        // 检查重试次数
        if (retries >= max_retries) {
            std::cout << "达到最大重试次数" << std::endl;
            return false;
        }

        // 检查连接状态
        if (!sClientInfo->isConnected) {
            std::cout << "未连接到服务器" << std::endl;
            return false;
        }

        send_message(CMSG_GET_SERVER_NOTICE);
        std::cout << "发送通知请求, 第" << retries + 1 << "次" << std::endl;
        
        // 等待响应
        std::this_thread::sleep_for(std::chrono::milliseconds(retry_interval_ms));
        retries++;
    }

    return true;
}

void startClient()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try
    {
        // 读取config.ini配置文件
        std::ifstream config_file("config.ini");
        std::string host = "127.0.0.1";
        std::string port = "8080";
        
        if (config_file.is_open()) {
            std::string line;
            while (std::getline(config_file, line)) {
                if (line.find("LauncherServerIPOrHost=") != std::string::npos) {
                    host = line.substr(line.find("=") + 1);
                }
                else if (line.find("LauncherServerPort=") != std::string::npos) {
                    port = line.substr(line.find("=") + 1);
                }
            }
            config_file.close();
        }

        sClient->connect(host, port);

        std::thread t([&]()
                      { 
            asio::io_context::work work(sClient->get_io_context());
            sClient->get_io_context().run(); });
        t.detach();

        // 等待连接并获取服务器通知
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (!sClient->wait_for_server_notice()) {
            std::cout << "无法获取服务器通知，但将继续运行..." << std::endl;
        }

        std::string input;
        while (true)
        {
                    sClient->show_menu();
                    std::getline(std::cin, input);

                    if (input == "q") break;
                    else if (input == "1") {
                        sClient->send_message(CMSG_GET_SERVER_NOTICE);
                    }
                    else if (input == "2") {
                        std::cout << "请输入要注册的账号和密码 (账号 密码): ";
                        std::string account;
                        std::getline(std::cin, account);

                        sClient->send_message(CMSG_REGISTER_ACCOUNT, account);
                    }
                    else if (input == "3") {
                        sClient->send_message(CMSG_CHECK_PATCH);
                    }
                }
    }
    catch (std::exception &e)
    {
        std::cerr << "异常: " << e.what() << std::endl;
    }
}