#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include "Auto.h"
#include "ClientConnector.h"
#include <thread>

// 取消 e 宏定义
#ifdef e
#undef e
#endif

#include "include/stb_image/stb_image.h"

/*
// 定义 ServerInfo 的静态成员
std::string ServerInfo::ip;
std::string ServerInfo::port;
std::string ServerInfo::name;
std::string ServerInfo::notice;
bool ServerInfo::isConnected = false; 
*/

extern int startClient();

// 实现LoadTextureFromFile函数
inline bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // 加载图片
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // 创建纹理
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // 创建纹理视图
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

void RenderBackground()
{
    if (g_background)
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float width = ImGui::GetWindowWidth();
        float height = ImGui::GetWindowHeight();
        ImGui::Image((ImTextureID)g_background, ImVec2(width, height));
    }
}

void MainWindow() {
    static bool open = true;
    static bool first_time = true;
    static int bg_width = 0, bg_height = 0;
    static HWND main_hwnd = NULL;
    
    if (first_time)
    {
        LoadTextureFromFile("Queen.jpg", &g_background, &bg_width, &bg_height);
        main_hwnd = GetActiveWindow();
        
        //startClient(); //连接服务器
        std::thread t1([]() { startClient(); });
        t1.detach();

        first_time = false;
    }

    if (open) {
        // 设置窗口和按钮的样式
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 12.0f;     // 窗口圆角
        style.FrameRounding = 12.0f;      // 按钮圆角
        style.PopupRounding = 12.0f;      // 弹出窗口圆角
        style.ScrollbarRounding = 12.0f;  // 滚动条圆角
        style.GrabRounding = 12.0f;       // 滑块圆角
        style.TabRounding = 12.0f;        // 标签页圆角

        // 设置默认窗口大小
        ImGui::SetNextWindowSize(ImVec2(1050, 650), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("魔兽世界登录器", &open, 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar|
            ImGuiWindowFlags_NoResize);  // 移除NoResize标志,允许调整窗口大小

        // 绘制背景前设置光标位置到窗口左上角
        ImGui::SetCursorPos(ImVec2(0, 0));
        // 绘制背景
        RenderBackground();

        // 设置按钮样式 - 调亮蓝色背景
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.5f, 1.0f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.6f, 1.0f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.4f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        // 添加关闭按钮
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 40, 10));
        if (ImGui::Button("X", ImVec2(30, 30))) {
            // 先断开与服务器的连接
            if(sClient && sClient->is_connected()) {
                // 关闭socket连接
                sClient->get_io_context().stop();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                sClientInfo->isConnected = false;
            }
            open = false;
            PostQuitMessage(0);
        }

        // 计算底部按钮的位置和间距
        float button_width = 150;
        float button_height = 40;
        float window_width = ImGui::GetWindowSize().x;
        float total_buttons_width = button_width * 4;
        float spacing = (window_width - total_buttons_width - 100) / 3;
        float start_x = 50;
        float start_y = ImGui::GetWindowSize().y - button_height - 15;

        // 获取当前字体大小
        float originalFontSize = ImGui::GetFontSize() * 1.3f;  // 因为文字缩放是1.3倍
        
        // 服务器名称和状态指示器
        ImGui::SetCursorPos(ImVec2(start_x, 20));
        
        // 绘制状态指示器圆圈
        float circle_radius = 8.0f;
        ImVec2 circle_pos = ImGui::GetCursorScreenPos();
        ImVec4 circle_color = sClientInfo->isConnected ? 
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :  // 在线时为绿色
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // 离线时为灰色
        
        // 调整圆圈的垂位置，使其与文字中心对齐
        ImGui::GetWindowDrawList()->AddCircleFilled(
            ImVec2(circle_pos.x + circle_radius, 
                  circle_pos.y + (originalFontSize/2) + 2),  // +2 用于微调
            circle_radius,
            ImGui::ColorConvertFloat4ToU32(circle_color)
        );

        // 移动文本位置，为圆圈留出空间
        ImGui::SetCursorPos(ImVec2(start_x + circle_radius*2 + 10, 20));
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::SetWindowFontScale(1.3f);

        if (!sClientInfo->name.empty() && sClientInfo->isConnected)
        {  
            ImGui::Text(sClientInfo->name.c_str());
        } 
        else 
        {
            ImGui::Text("XX魔兽");
        }
        
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();


        // 通知区域
        ImGui::SetCursorPos(ImVec2(start_x + button_width + spacing, 20));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // 完全透明背景
        ImGui::BeginChild("通知区域", ImVec2(650, 450), false); // 移除边框

        // 添加标题
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::SetWindowFontScale(1.2f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 增加标题文字亮度
        ImGui::SetCursorPosX((650 - ImGui::CalcTextSize("服务器公告").x) * 0.5f);
        ImGui::Text("服务器公告");
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 1.0f, 0.3f)); // 更亮的分隔线
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // 通知内容
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 增加正文文字亮度
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::SetWindowFontScale(1.1f);
        ImGui::TextWrapped(sClientInfo->notice.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor(); // 弹出背景色


        if(!sClientInfo->download_notice.empty())
        {
            // 计算主窗口中心位置
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImVec2 window_size(400, 400);
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(window_size);

            if (ImGui::Begin("补丁更新", nullptr, ImGuiWindowFlags_NoCollapse))
            {
                ImGui::TextWrapped(sClientInfo->download_notice.c_str());
                
                // 如果补丁更新完成,显示关闭按钮
                if(sClientInfo->update_finished)
                {
                    sClientInfo->download_notice.clear(); // 清空通知内容以关闭窗口
                    sClientInfo->update_finished = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::End();
            }
        }



        // 底部按钮 - 所有四个按钮
        ImGui::SetCursorPos(ImVec2(start_x, start_y));
        if (ImGui::Button("注册账号", ImVec2(button_width, button_height))) {
            ImGui::OpenPopup("注册账号");
        }

        // 在BeginPopupModal之前设置窗口大小
        ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Always);
        
        // 注册弹窗
        if (ImGui::BeginPopupModal("注册账号", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char account[64] = "";
            static char password[64] = "";
            static char confirmPassword[64] = "";
            static char securityKey[64] = "";
            static char verifyCode[16] = "";
            static int selectedOption = 0;             
            // 标题
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            float text_width = ImGui::CalcTextSize(sClientInfo->name.c_str()).x;
            float window_width = ImGui::GetWindowSize().x;
            ImGui::SetCursorPosX((window_width - text_width) * 0.5f);
            ImGui::TextWrapped(sClientInfo->name.c_str());
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Spacing();

            // 提示文本
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
            ImGui::TextWrapped("*欢迎使用账号注册服务,请务必牢记账号密码*");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Spacing();

            // 输入框
            float label_width = 80.0f;  // 标签宽度
            float input_width = 167.0f; // 输入框宽度缩短1/3
            float hint_width = 120.0f;  // 提示文本宽度
            float spacing = 10.0f;      // 间距

            // 账号输入框
            ImGui::Text("账号名称"); 
            ImGui::SameLine(label_width);
            ImGui::PushItemWidth(input_width);
            // 添加输入过滤回调,只允许字母和数字,并限制长度4-12位
            ImGui::InputText("##account", account, IM_ARRAYSIZE(account), ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackEdit,
                [](ImGuiInputTextCallbackData* data) -> int {
                    if(data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
                        if(data->BufTextLen >= 12) { // 使用BufTextLen而不是strlen
                            data->DeleteChars(12, data->BufTextLen - 12);
                            data->BufDirty = true;
                        }
                        return 0;
                    }
                    if (isalnum((unsigned char)data->EventChar)) // 只允许字母和数字
                        return 0;
                    return 1; // 返回1表示过滤掉该字符
                });
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "4-12位数字和字母");

            // 密码输入框
            ImGui::Text("输入密码");
            ImGui::SameLine(label_width);
            ImGui::PushItemWidth(input_width);
            ImGui::InputText("##password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackEdit,
                [](ImGuiInputTextCallbackData* data) -> int {
                    if(data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
                        if(data->BufTextLen >= 12) {
                            data->DeleteChars(12, data->BufTextLen - 12);
                            data->BufDirty = true;
                        }
                        return 0;
                    }
                    if (isalnum((unsigned char)data->EventChar)) // 只允许字母和数字
                        return 0;
                    return 1;
                });
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "4-12位数字和字母");

            // 确认密码
            ImGui::Text("确认密码");
            ImGui::SameLine(label_width);
            ImGui::PushItemWidth(input_width);
            ImGui::InputText("##confirmPassword", confirmPassword, IM_ARRAYSIZE(confirmPassword), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackEdit,
                [](ImGuiInputTextCallbackData* data) -> int {
                    if(data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
                        if(data->BufTextLen >= 12) {
                            data->DeleteChars(12, data->BufTextLen - 12);
                            data->BufDirty = true;
                        }
                        return 0;
                    }
                    if (isalnum((unsigned char)data->EventChar)) // 只允许字母和数字
                        return 0;
                    return 1;
                });
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "两次输入的密码");

            // 安全密钥
            ImGui::Text("安全密钥");
            ImGui::SameLine(label_width);
            ImGui::PushItemWidth(input_width);
            ImGui::InputText("##securityKey", securityKey, IM_ARRAYSIZE(securityKey), ImGuiInputTextFlags_CallbackCharFilter,
                [](ImGuiInputTextCallbackData* data) -> int {
                    if (isalnum((unsigned char)data->EventChar)) // 只允许字母和数字
                        return 0;
                    return 1;
                });
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "1-8位数字和字母");

            // 验证码
            static char randomCode[5] = ""; // 4位数字+结束符
            static bool codeGenerated = false;
            if (!codeGenerated) {
                // 生成4位随机数字
                srand(time(NULL));
                sprintf(randomCode, "%04d", rand() % 10000);
                codeGenerated = true;
            }

            ImGui::Text("随机验证");
            ImGui::SameLine(label_width);
            ImGui::PushItemWidth(input_width);
            ImGui::InputText("##verifyCode", verifyCode, IM_ARRAYSIZE(verifyCode));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), randomCode);

            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::Spacing();

            // 单选按钮
            ImGui::RadioButton("账号注册", &selectedOption, 0); 
            
            ImGui::Spacing();
            ImGui::Spacing();
            //服务器回复消息
            if (!sClientInfo->response.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text(sClientInfo->response.c_str());
                ImGui::PopStyleColor();
            }
            
            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::Button("确认", ImVec2(120, 0))) {
                if (strlen(account) < 4 || strlen(password) < 4 || strlen(confirmPassword) < 4) {
                    // 账号或密码长度不足
                    ImGui::OpenPopup("错误提示");
                } else if (strlen(securityKey) > 8) {
                    // 安全密钥长度不足
                    ImGui::OpenPopup("错误提示");
                } else if (strcmp(password, confirmPassword) != 0) {
                    // 两次密码不一致
                    ImGui::OpenPopup("密码错误");
                } else if (strcmp(verifyCode, randomCode) != 0) {
                    // 验证码错误
                    ImGui::OpenPopup("验证码错误");
                } else {
                    std::string registerInfo = std::string(account) + "||" + password + "||" + securityKey;
                    sClient->send_message(CMSG_REGISTER_ACCOUNT, registerInfo);
                }
            }
            if (ImGui::BeginPopupModal("错误提示", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("账号和密码长度需要大于4位,安全密钥需要小于8位!");
                if (ImGui::Button("确定")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("密码错误", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("两次输入的密码不一致!");
                if (ImGui::Button("确定")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("验证码错误", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("验证码输入错误!");
                if (ImGui::Button("确定"))                 
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 140); // 向右移动20像素
            if (ImGui::Button("关闭", ImVec2(100, 0))) {
                account[0] = '\0';
                password[0] = '\0';
                confirmPassword[0] = '\0';
                securityKey[0] = '\0';
                verifyCode[0] = '\0';
                codeGenerated = false;
                sClientInfo->response = "";
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SetCursorPos(ImVec2(start_x + button_width + spacing, start_y));
        if (ImGui::Button("赞助服务", ImVec2(button_width, button_height))) {
            // 处理赞助服务按钮点击
        }

        ImGui::SetCursorPos(ImVec2(start_x + (button_width + spacing) * 2, start_y));
        if (ImGui::Button("进入QQ群", ImVec2(button_width, button_height))) {
            // 处理进入QQ群按钮点击
        }

        ImGui::SetCursorPos(ImVec2(start_x + (button_width + spacing) * 3, start_y));
        if (ImGui::Button("启动游戏", ImVec2(button_width, button_height))) {
            Check_and_start_game();  // 传递窗口句柄
        }

        // 恢复按钮样式
        ImGui::PopStyleColor(4);

        ImGui::End();
    }
    else {
        // 修改这里的逻辑，同样使用 PostQuitMessage
        PostQuitMessage(0);
    }
}

void Check_and_start_game()
{
    // 检查游戏是否启动
    sClient->send_message(CMSG_CHECK_PATCH);

    Sleep(500);
    // 获取当前进程路径
    wchar_t currentPath[MAX_PATH];
    GetModuleFileNameW(NULL, currentPath, MAX_PATH);
    std::wstring currentDir = std::wstring(currentPath);
    currentDir = currentDir.substr(0, currentDir.find_last_of(L"\\"));

    if (sClientInfo->check_patch_path_pass)
    {   
        // 写入realmlist.wtf
    std::wstring realmlistPath = currentDir + L"\\realmlist.wtf";
    std::ofstream realmlist(realmlistPath.c_str());
    if (realmlist.is_open()) {
        realmlist << "SET realmlist " << sClientInfo->ip << std::endl;
        realmlist.close();
    } else {
        MessageBoxW(NULL, L"无法写入realmlist.wtf文件!", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    // 启动游戏进程
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    std::wstring exePath = currentDir + L"\\Wow.exe";
    
    if (CreateProcessW(
        exePath.c_str(),     // 应用程序名称
        NULL,                // 命令行参数
        NULL,                // 进程安全属性
        NULL,                // 线程安全属性 
        FALSE,              // 是否继承句柄
        0,                  // 创建标志
        NULL,               // 环境变量
        currentDir.c_str(), // 当前目录
        &si,                // STARTUPINFO
        &pi                 // PROCESS_INFORMATION
    )) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    }

    else
    {
        MessageBoxW(NULL, L"请等待补丁更新完成后再启动游戏!", L"提示", MB_OK | MB_ICONINFORMATION);

    }
}