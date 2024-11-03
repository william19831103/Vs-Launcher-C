#include "Auto.h"
#include "include/stb_image/stb_image.h"


// 定义 ServerInfo 的静态成员
std::string ServerInfo::ip;
std::string ServerInfo::port;
std::string ServerInfo::name;
std::string ServerInfo::notice;
bool ServerInfo::isConnected = false; 

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
    static bool serverOnline = true;
    static HWND main_hwnd = NULL;
    
    if (first_time)
    {
        LoadTextureFromFile("Queen.jpg", &g_background, &bg_width, &bg_height);
        main_hwnd = GetActiveWindow();
        
        // 直接调用 start_connect_server
        //start_connect_server();
        
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

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1000, 600));
        
        ImGui::Begin("魔兽世界登录器", &open, 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoCollapse);

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
            open = false;
        }

        // 计算底部按钮的位置和间距
        float button_width = 150;
        float button_height = 40;
        float window_width = ImGui::GetWindowSize().x;
        float total_buttons_width = button_width * 4;
        float spacing = (window_width - total_buttons_width - 100) / 3;
        float start_x = 50;
        float start_y = ImGui::GetWindowSize().y - button_height - 50;

        // 获取当前字体大小
        float originalFontSize = ImGui::GetFontSize() * 1.3f;  // 因为文字缩放是1.3倍
        
        // 服务器名称和状态指示器
        ImGui::SetCursorPos(ImVec2(start_x, 20));
        
        // 绘制状态指示器圆圈
        float circle_radius = 8.0f;
        ImVec2 circle_pos = ImGui::GetCursorScreenPos();
        ImVec4 circle_color = serverOnline ? 
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

        if (!ServerInfo::name.empty() && ServerInfo::isConnected)
        {
            int wlen = MultiByteToWideChar(CP_UTF8, 0, ServerInfo::name.c_str(), -1, NULL, 0);
            if (wlen > 0) 
            { 
                ImGui::Text("%s", ServerInfo::name.c_str());
            }
        } 
        else 
        {
            ImGui::Text("炽焰战网");
        }
        
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();

        // 更新服务器状态指示器
        serverOnline = ServerInfo::isConnected;

        // 通知区域
        ImGui::SetCursorPos(ImVec2(start_x + button_width + spacing, 20));
        ImGui::BeginChild("通知区域", ImVec2(600, 400), true);
        ImGui::SetWindowFontScale(1.3f);
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::TextUnformatted(ServerInfo::notice.c_str());
        ImGui::PopTextWrapPos();
        ImGui::SetWindowFontScale(1.0f);
        ImGui::EndChild();

        // 底部按钮 - 所有四个按钮
        ImGui::SetCursorPos(ImVec2(start_x, start_y));
        if (ImGui::Button("注册账号", ImVec2(button_width, button_height))) {
            // 处理注册账号按钮点击
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
            //check_and_start_game(main_hwnd);  // 传递窗口句柄
        }

        // 恢复按钮样式
        ImGui::PopStyleColor(4);

        ImGui::End();
    }
    else {
        exit(0);
    }
}

