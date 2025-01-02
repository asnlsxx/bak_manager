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
    GLFWwindow* window = glfwCreateWindow(800, 600, "Backup Manager", nullptr, nullptr);
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

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
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
            
        // 添加结果弹窗
        if (show_success_) {
            ImGui::OpenPopup("Success");
        }
        if (ImGui::BeginPopupModal("Success", &show_success_, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Operation completed successfully!");
            if (ImGui::Button("OK")) {
                show_success_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (show_error_) {
            ImGui::OpenPopup("Error");
        }
        if (ImGui::BeginPopupModal("Error", &show_error_, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Error: %s", error_message_.c_str());
            if (ImGui::Button("OK")) {
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

    ImGui::Begin("Backup Manager", nullptr, window_flags);
    
    // Menu Bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Backup", "Ctrl+B")) {
                show_backup_window_ = true;
            }
            if (ImGui::MenuItem("Restore", "Ctrl+R")) {
                show_restore_window_ = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                glfwSetWindowShouldClose(glfwGetCurrentContext(), true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                show_about_ = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Main Content
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    
    // Center Title
    float window_width = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize("Backup Manager").x) / 2);
    ImGui::Text("Backup Manager");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Center Button Group
    float button_width = 120;
    float button_height = 40;
    float buttons_total_width = button_width * 2 + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX((window_width - buttons_total_width) / 2);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.6f, 1.0f));
    
    if (ImGui::Button("Backup", ImVec2(button_width, button_height))) {
        show_backup_window_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore", ImVec2(button_width, button_height))) {
        show_restore_window_ = true;
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    ImGui::End();
}

std::string GUI::open_file_dialog(bool folder) {
    std::string command;
    if (folder) {
        command = "zenity --file-selection --directory";
    } else {
        command = "zenity --file-selection";
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
    ImGui::Begin("Backup", &show_backup_window_, ImGuiWindowFlags_NoCollapse);
    
    ImGui::Text("Select files or directories to backup:");
    ImGui::Spacing();
    
    // File Selection
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::InputText("Source Path", input_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        std::string selected = open_file_dialog(true);  // true for folder selection
        if (!selected.empty()) {
            strncpy(input_path_, selected.c_str(), PATH_BUFFER_SIZE - 1);
            input_path_[PATH_BUFFER_SIZE - 1] = '\0';
        }
    }
    
    ImGui::InputText("Target Path", output_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("Browse...##2")) {
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
    ImGui::Checkbox("Compress Files", &compress_);
    ImGui::Checkbox("Encrypt Files", &encrypt_);
    
    if (encrypt_) {
        ImGui::Indent(20);
        ImGui::InputText("Password", password_, PASSWORD_BUFFER_SIZE, ImGuiInputTextFlags_Password);
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
    
    if (ImGui::Button("Start Backup", ImVec2(button_width, 0))) {
        try {
            if (input_path_[0] == '\0' || output_path_[0] == '\0') {
                show_error_ = true;
                error_message_ = "Please select both source and target paths";
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
                ImGui::End();
                return;
            }

            if (encrypt_ && password_[0] == '\0') {
                show_error_ = true;
                error_message_ = "Password is required for encryption";
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
                error_message_ = "Backup failed";
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
    ImGui::Begin("Restore", &show_restore_window_, ImGuiWindowFlags_NoCollapse);
    
    ImGui::Text("Select backup file and restore location:");
    ImGui::Spacing();
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::InputText("Backup File", input_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        std::string selected = open_file_dialog(false);  // false for file selection
        if (!selected.empty()) {
            strncpy(input_path_, selected.c_str(), PATH_BUFFER_SIZE - 1);
            input_path_[PATH_BUFFER_SIZE - 1] = '\0';
        }
    }
    
    ImGui::InputText("Restore Path", output_path_, PATH_BUFFER_SIZE);
    ImGui::SameLine();
    if (ImGui::Button("Browse...##2")) {
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
    
    // Add encryption checkbox and password input
    ImGui::BeginGroup();
    ImGui::Checkbox("Encrypted Backup", &encrypt_);
    
    if (encrypt_) {
        ImGui::Indent(20);
        ImGui::InputText("Password", password_, PASSWORD_BUFFER_SIZE, ImGuiInputTextFlags_Password);
        ImGui::Unindent(20);
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
    
    if (ImGui::Button("Start Restore", ImVec2(button_width, 0))) {
        try {
            if (input_path_[0] == '\0' || output_path_[0] == '\0') {
                show_error_ = true;
                error_message_ = "Please select both backup file and restore path";
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
                ImGui::End();
                return;
            }

            if (encrypt_) {
                if (password_[0] == '\0') {
                    show_error_ = true;
                    error_message_ = "Password is required for encrypted backup";
                    ImGui::PopStyleColor(3);
                    ImGui::PopStyleVar();
                    ImGui::End();
                    return;
                }
                packer_.set_encrypt(true, password_);
            } else {
                packer_.set_encrypt(false, "");
            }
            
            if (packer_.Unpack(input_path_, output_path_)) {
                show_success_ = true;
                show_restore_window_ = false;  // 成功后关闭还原窗口
            } else {
                show_error_ = true;
                error_message_ = "Restore failed";
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