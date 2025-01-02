#ifndef GUI_H
#define GUI_H

#include "Packer.h"
#include <string>
#include <filesystem>
#include <vector>
#include <mutex>
#include <spdlog/sinks/base_sink.h>

// 自定义日志接收器
template<typename Mutex>
class GuiLogSink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit GuiLogSink(std::vector<std::string>& log_buffer) 
        : log_buffer_(log_buffer) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // 格式化日志消息
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);
        log_buffer_.push_back(fmt::to_string(formatted));
    }

    void flush_() override {}

private:
    std::vector<std::string>& log_buffer_;
};

using GuiLogSinkMt = GuiLogSink<std::mutex>;

class GUI {
public:
    GUI();
    ~GUI();
    
    void run();
    
private:
    void render_main_window();
    void render_backup_window();
    void render_restore_window();
    void render_log_window();
    void render_help_window();
    void render_verify_window();
    
    static constexpr size_t PATH_BUFFER_SIZE = 256;
    static constexpr size_t PASSWORD_BUFFER_SIZE = 64;
    static constexpr size_t MAX_LOG_LINES = 1000;
    
    Packer packer_;
    char input_path_[PATH_BUFFER_SIZE] = "";
    char output_path_[PATH_BUFFER_SIZE] = "";
    char password_[PASSWORD_BUFFER_SIZE] = "";
    bool compress_ = false;
    bool encrypt_ = false;
    bool show_backup_window_ = false;
    bool show_restore_window_ = false;
    bool show_about_ = false;
    bool show_success_ = false;
    bool show_error_ = false;
    bool show_log_ = true;
    bool show_help_ = false;
    bool show_verify_window_ = false;
    std::string error_message_;
    
    std::vector<std::string> log_buffer_;
    std::shared_ptr<GuiLogSinkMt> log_sink_;
    
    std::string open_file_dialog(bool folder = false);
    bool restore_metadata_ = false;  // 是否恢复元数据
    void reset_input_fields();  // 添加重置函数声明
};

#endif // GUI_H 