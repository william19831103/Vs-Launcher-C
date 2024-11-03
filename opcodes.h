#ifndef OPCODES_H
#define OPCODES_H

#include <cstdint>

enum OpcodesList : uint16_t
{
    MSG_NULL_ACTION           = 0x0000,
    
    // 客户端操作码 (0x1000 - 0x1FFF)
    CMSG_GET_SERVER_NOTICE    = 0x1001,    // 获取服务器通知
    CMSG_REGISTER_ACCOUNT     = 0x1002,    // 注册账号
    CMSG_CHECK_PATCH         = 0x1003,    // 检测下载补丁
    CMSG_REQUEST_PATCH_FILE  = 0x1004,    // 请求下载具体的补丁文件
    
    // 服务端操作码 (0x2000 - 0x2FFF)
    SMSG_SERVER_NOTICE       = 0x2001,    // 服务器通知响应
    SMSG_REGISTER_RESULT     = 0x2002,    // 注册结果响应
    SMSG_PATCH_INFO          = 0x2003,    // 补丁信息响应
    SMSG_PATCH_FILE          = 0x2004,    // 补丁文件数据
    SMSG_PATCH_FILE_END      = 0x2005,    // 补丁文件结束标记
};

// 消息头结构
#pragma pack(push, 1)
struct MessageHeader
{
    uint16_t opcode;
    uint32_t size;    // 不包括头部的数据大小
};
#pragma pack(pop)

// 补丁文件信息结构
#pragma pack(push, 1)
struct PatchFileInfo {
    char filename[256];
    uint64_t filesize;
    uint64_t timestamp;
};
#pragma pack(pop)

#endif // OPCODES_H 