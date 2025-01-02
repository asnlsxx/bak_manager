#include "GUI.h"
#include "imgui.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <spdlog/spdlog.h>

GUI::GUI() {
    // 初始化 GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "备份管理器", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 启用垂直同步

    // 设置 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // 配置字体
    ImGuiIO& io = ImGui::GetIO();
    
    // 默认字体配置
    ImFontConfig config;
    config.MergeMode = true;  // 启用字体合并
    config.PixelSnapH = true;
    
    // 加载默认字体（英文）
    io.Fonts->AddFontDefault();
    
    // 加载中文字体并合并
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // 基本拉丁字符
        0x2000, 0x206F, // 常用标点
        0x3000, 0x30FF, // 日文假名和标点
        0x31F0, 0x31FF, // 假名扩展
        0x4e00, 0x9FAF, // CJK 统一表意文字
        0xFF00, 0xFFEF, // 全角字符
        0,
    };
    io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf", 
                                16.0f, &config, ranges);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 初始化日志系统
    log_sink_ = std::make_shared<GuiLogSinkMt>(log_buffer_);
    auto logger = std::make_shared<spdlog::logger>("gui_logger", log_sink_);
    logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(logger);

    // 设置现代化的深色主题
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(5, 5);
    style.ItemSpacing = ImVec2(6, 6);
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;
    
    // 设置现代化配色
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
    
    // 主按钮颜色
    colors[ImGuiCol_Button] = ImVec4(0.2f, 0.4f, 0.7f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.5f, 0.8f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.1f, 0.3f, 0.6f, 1.00f);
    
    // 设置字体
    io.FontGlobalScale = 1.1f;  // 稍微增大字体
}

GUI::~GUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}

void GUI::run() {
    GLFWwindow* window = glfwGetCurrentContext();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        render_main_window();
        
        if (show_backup_window_)
            render_backup_window();
        if (show_restore_window_)
            render_restore_window();
        if (show_help_)
            render_help_window();
        if (show_verify_window_)
            render_verify_window();
            
        // 添加结果弹窗
        if (show_success_) {
            ImGui::OpenPopup("成功");
        }
        if (ImGui::BeginPopupModal("成功", &show_success_, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("操作已成功完成！");
            if (ImGui::Button("确定")) {
                show_success_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (show_error_) {
            ImGui::OpenPopup("错误");
        }
        if (ImGui::BeginPopupModal("错误", &show_error_, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "错误：%s", error_message_.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("查看日志窗口获取更多详细信息。可以从\"视图\"菜单打开日志窗口。");
            if (!show_log_) {
                if (ImGui::Button("显示日志")) {
                    show_log_ = true;
                }
                ImGui::SameLine();
            }
            if (ImGui::Button("关闭")) {
                show_error_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void GUI::render_main_window() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | 
                                  ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("备份管理器", nullptr, window_flags);
    
    // Menu Bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("文件")) {
            if (ImGui::MenuItem("备份", "Ctrl+B")) {
                reset_input_fields();  // 重置输入字段
                show_backup_window_ = true;
            }
            if (ImGui::MenuItem("还原", "Ctrl+R")) {
                reset_input_fields();  // 重置输入字段
                show_restore_window_ = true;
            }
            if (ImGui::MenuItem("验证", "Ctrl+V")) {
                reset_input_fields();  // 重置输入字段
                show_verify_window_ = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("退出", "Alt+F4")) {
                glfwSetWindowShouldClose(glfwGetCurrentContext(), true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("帮助")) {
            if (ImGui::MenuItem("帮助文档", "F1")) {
                show_help_ = true;
            }
            if (ImGui::MenuItem("关于")) {
                show_about_ = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("视图")) {
            ImGui::MenuItem("日志窗口", NULL, &show_log_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // 计算主内容区域和日志区域的高度
    float total_height = ImGui::GetContentRegionAvail().y;
    float log_height = show_log_ ? total_height * 0.4f : 0;  // 日志窗口占40%高度
    float main_content_height = total_height - log_height;

    // 主内容区域
    ImGui::BeginChild("MainContent", ImVec2(0, main_content_height), false);
    
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    
    // Center Title
    float window_width = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize("备份管理器").x) / 2);
    ImGui::Text("备份管理器");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Center Button Group
    float button_width = 120;
    float button_height = 40;
    float buttons_total_width = button_width * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
    ImGui::SetCursorPosX((window_width - buttons_total_width) / 2);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.8f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.6f, 1.00f));
    
    if (ImGui::Button("备份", ImVec2(button_width, button_height))) {
        reset_input_fields();  // 重置输入字段
        show_backup_window_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("还原", ImVec2(button_width, button_height))) {
        reset_input_fields();  // 重置输入字段
        show_restore_window_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("验证", ImVec2(button_width, button_height))) {
        reset_input_fields();  // 重置输入字段
        show_verify_window_ = true;
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    
    ImGui::EndChild();

    // 如果启用了日志窗口，在主窗口下方直接渲染日志内容
    if (show_log_) {
        ImGui::Separator();
        
        // 日志工具栏
        if (ImGui::Button("清除")) {
            log_buffer_.clear();
        }
        ImGui::SameLine();
        static bool auto_scroll = true;
        ImGui::Checkbox("自动滚动", &auto_scroll);
        
        // 日志内容区域
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::BeginChild("LogArea", ImVec2(0, log_height - 30), true, 
                         ImGuiWindowFlags_HorizontalScrollbar);
        
        // 添加时间戳颜色
        for (const auto& line : log_buffer_) {
            // 提取时间戳
            size_t timestamp_end = line.find("]");
            if (timestamp_end != std::string::npos) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                                 "%.*s", (int)timestamp_end + 1, line.c_str());
                ImGui::SameLine();
                // 剩余文本使用不同颜色
                const char* remaining = line.c_str() + timestamp_end + 1;
                if (line.find("[error]") != std::string::npos)
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", remaining);
                else if (line.find("[warn]") != std::string::npos)
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", remaining);
                else if (line.find("[info]") != std::string::npos)
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", remaining);
                else
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", remaining);
            }
        }
        
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::End();
}

std::string GUI::open_file_dialog(bool folder) {
    std::string command;
    if (folder) {
        command = "zenity --file-selection --directory 2>/dev/null";
    } else {
        command = "zenity --file-selection 2>/dev/null";
    }
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }
    
    char buffer[PATH_BUFFER_SIZE];
    std::string result;
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    
    // Remove trailing newline if present
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }
    
    return result;
}

void GUI::render_backup_window() {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("备份", &show_backup_window_, ImGuiWindowFlags_NoCollapse);
    
    ImGui::Text("选择要备份的文件或目录：");
    ImGui::Spacing();
    
    // File Selection
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::InputText("源路径", input_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("浏览...")) {
        std::string selected = open_file_dialog(true);  // true for folder selection
        if (!selected.empty()) {
            strncpy(input_path_, selected.c_str(), PATH_BUFFER_SIZE - 1);
            input_path_[PATH_BUFFER_SIZE - 1] = '\0';
        }
    }
    
    ImGui::InputText("目标路径", output_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("浏览...##2")) {
        std::string selected = open_file_dialog(true);  // true for folder selection
        if (!selected.empty()) {
            strncpy(output_path_, selected.c_str(), PATH_BUFFER_SIZE - 1);
            output_path_[PATH_BUFFER_SIZE - 1] = '\0';
        }
    }
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Options
    ImGui::BeginGroup();
    ImGui::Checkbox("压缩文件", &compress_);
    ImGui::Checkbox("加密文件", &encrypt_);
    
    if (encrypt_) {
        ImGui::Indent(20);
        ImGui::InputText("密码", password_, PASSWORD_BUFFER_SIZE, ImGuiInputTextFlags_Password);
        ImGui::Unindent(20);
    }
    ImGui::EndGroup();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Bottom Button
    float button_width = 120;
    float window_width = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((window_width - button_width) / 2);
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
    
    if (ImGui::Button("开始备份", ImVec2(button_width, 0))) {
        try {
            if (input_path_[0] == '\0' || output_path_[0] == '\0') {
                show_error_ = true;
                error_message_ = "请选择源路径和目标路径";
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
                ImGui::End();
                return;
            }

            if (encrypt_ && password_[0] == '\0') {
                show_error_ = true;
                error_message_ = "需要密码进行加密";
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
                ImGui::End();
                return;
            }

            packer_.set_compress(compress_);
            if (encrypt_) {
                packer_.set_encrypt(true, password_);
            }
            
            fs::path backup_path = fs::path(output_path_) / 
                                 (fs::path(input_path_).filename().string() + ".backup");
                                 
            if (packer_.Pack(input_path_, backup_path)) {
                show_success_ = true;
                show_backup_window_ = false;  // 成功后关闭备份窗口
            } else {
                show_error_ = true;
                error_message_ = "备份失败";
            }
        } catch (const std::exception& e) {
            show_error_ = true;
            error_message_ = e.what();
        }
    }
    
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    
    ImGui::End();
}

void GUI::render_restore_window() {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("还原", &show_restore_window_, ImGuiWindowFlags_NoCollapse);
    
    ImGui::Text("选择备份文件和还原位置：");
    ImGui::Spacing();
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::InputText("备份文件", input_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("浏览...")) {
        std::string selected = open_file_dialog(false);  // false for file selection
        if (!selected.empty()) {
            strncpy(input_path_, selected.c_str(), PATH_BUFFER_SIZE - 1);
            input_path_[PATH_BUFFER_SIZE - 1] = '\0';
        }
    }
    
    ImGui::InputText("还原路径", output_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("浏览...##2")) {
        std::string selected = open_file_dialog(true);  // true for folder selection
        if (!selected.empty()) {
            strncpy(output_path_, selected.c_str(), PATH_BUFFER_SIZE - 1);
            output_path_[PATH_BUFFER_SIZE - 1] = '\0';
        }
    }
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // 选项组
    ImGui::BeginGroup();
    // 加密选项
    ImGui::Checkbox("解密", &encrypt_);
    if (encrypt_) {
        ImGui::Indent(20);
        ImGui::InputText("密码", password_, PASSWORD_BUFFER_SIZE, ImGuiInputTextFlags_Password);
        ImGui::Unindent(20);
    }
    
    ImGui::Spacing();
    // 添加元数据恢复选项
    ImGui::Checkbox("还原元数据", &restore_metadata_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("还原文件属性、时间戳和权限");
    }
    ImGui::EndGroup();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    float button_width = 120;
    float window_width = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((window_width - button_width) / 2);
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
    
    if (ImGui::Button("开始还原", ImVec2(button_width, 0))) {
        try {
            if (input_path_[0] == '\0' || output_path_[0] == '\0') {
                show_error_ = true;
                error_message_ = "请选择备份文件和还原路径";
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
                ImGui::End();
                return;
            }

            if (encrypt_) {
                if (password_[0] == '\0') {
                    show_error_ = true;
                    error_message_ = "需要密码进行加密备份";
                    ImGui::PopStyleColor(3);
                    ImGui::PopStyleVar();
                    ImGui::End();
                    return;
                }
                packer_.set_encrypt(true, password_);
            } else {
                packer_.set_encrypt(false, "");
            }
            
            // 设置元数据恢复选项
            packer_.set_restore_metadata(restore_metadata_);
            
            if (packer_.Unpack(input_path_, output_path_)) {
                show_success_ = true;
                show_restore_window_ = false;  // 成功后关闭还原窗口
            } else {
                show_error_ = true;
                error_message_ = "还原失败";
            }
        } catch (const std::exception& e) {
            show_error_ = true;
            error_message_ = e.what();
        }
    }
    
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    
    ImGui::End();
}

void GUI::render_log_window() {
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("日志", &show_log_);
    
    // 添加清除按钮
    if (ImGui::Button("清除")) {
        log_buffer_.clear();
    }
    ImGui::SameLine();
    
    // 添加自动滚动选项
    static bool auto_scroll = true;
    ImGui::Checkbox("自动滚动", &auto_scroll);
    
    ImGui::Separator();
    
    // 创建日志区域
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, 
                      ImGuiWindowFlags_HorizontalScrollbar);
    
    // 显示日志内容
    for (const auto& line : log_buffer_) {
        // 根据日志级别设置颜色
        if (line.find("[error]") != std::string::npos)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        else if (line.find("[warn]") != std::string::npos)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
        else if (line.find("[info]") != std::string::npos)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        else
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                
        ImGui::TextUnformatted(line.c_str());
        ImGui::PopStyleColor();
    }
    
    // 自动滚动到底部
    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::End();
    
    // 限制日志行数
    if (log_buffer_.size() > MAX_LOG_LINES) {
        log_buffer_.erase(log_buffer_.begin(), 
                         log_buffer_.begin() + (log_buffer_.size() - MAX_LOG_LINES));
    }
}

void GUI::render_help_window() {
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("帮助", &show_help_, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));

    // 使用标题样式
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("备份管理器帮助");
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Spacing();

    // 基本操作部分
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "基本操作");
    ImGui::Spacing();
    ImGui::BulletText("备份：创建文件或目录的备份");
    ImGui::BulletText("还原：从备份中恢复文件");
    ImGui::BulletText("验证：检查备份文件的完整性");
    ImGui::Spacing();

    // 备份功能部分
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "备份功能");
    ImGui::Spacing();
    ImGui::BulletText("压缩：减小备份文件大小");
    ImGui::BulletText("加密：使用密码保护数据");
    ImGui::BulletText("文件选择：选择特定文件或目录进行备份");
    ImGui::Spacing();

    // 还原功能部分
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "还原功能");
    ImGui::Spacing();
    ImGui::BulletText("元数据还原：保留文件属性和时间戳");
    ImGui::BulletText("密码保护：解密加密的备份");
    ImGui::Spacing();

    // 快捷键部分
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "快捷键");
    ImGui::Spacing();
    ImGui::Columns(2, "shortcuts");
    ImGui::SetColumnWidth(0, 150);
    ImGui::Text("Ctrl+B"); ImGui::NextColumn(); ImGui::Text("打开备份窗口"); ImGui::NextColumn();
    ImGui::Text("Ctrl+R"); ImGui::NextColumn(); ImGui::Text("打开还原窗口"); ImGui::NextColumn();
    ImGui::Text("F1"); ImGui::NextColumn(); ImGui::Text("显示帮助"); ImGui::NextColumn();
    ImGui::Text("Alt+F4"); ImGui::NextColumn(); ImGui::Text("退出程序"); ImGui::NextColumn();
    ImGui::Columns(1);
    ImGui::Spacing();

    // 注意事项部分
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "注意事项");
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
    ImGui::TextWrapped("! 请妥善保管加密密码，丢失的密码无法恢复");
    ImGui::TextWrapped("! 确保有足够的磁盘空间进行备份操作");
    ImGui::TextWrapped("! 查看日志窗口获取详细的操作信息");
    ImGui::PopStyleColor();

    ImGui::PopStyleVar();
    ImGui::End();
}

void GUI::render_verify_window() {
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("验证备份", &show_verify_window_, ImGuiWindowFlags_NoCollapse);
    
    ImGui::Text("选择要验证的备份文件：");
    ImGui::Spacing();
    
    // 文件选择
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::InputText("备份文件", input_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("浏览...")) {
        std::string selected = open_file_dialog(false);  // false for file selection
        if (!selected.empty()) {
            strncpy(input_path_, selected.c_str(), PATH_BUFFER_SIZE - 1);
            input_path_[PATH_BUFFER_SIZE - 1] = '\0';
        }
    }
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // 加密选项
    ImGui::BeginGroup();

    ImGui::EndGroup();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // 验证按钮
    float button_width = 120;
    float window_width = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((window_width - button_width) / 2);
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
    
    if (ImGui::Button("开始验证", ImVec2(button_width, 0))) {
        try {
            if (input_path_[0] == '\0') {
                show_error_ = true;
                error_message_ = "请选择要验证的备份文件";
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
                ImGui::End();
                return;
            }
            
            if (packer_.Verify(input_path_)) {
                show_success_ = true;
                error_message_ = "备份文件验证通过";
                show_verify_window_ = false;  // 成功后关闭验证窗口
            } else {
                show_error_ = true;
                error_message_ = "备份文件验证失败";
            }
        } catch (const std::exception& e) {
            show_error_ = true;
            error_message_ = e.what();
        }
    }
    
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    
    ImGui::End();
}

// 添加重置函数
void GUI::reset_input_fields() {
    input_path_[0] = '\0';
    output_path_[0] = '\0';
    password_[0] = '\0';
    compress_ = false;
    encrypt_ = false;
    restore_metadata_ = false;
} 