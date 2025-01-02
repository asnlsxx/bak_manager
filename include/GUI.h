#ifndef GUI_H
#define GUI_H

#include "Packer.h"
#include <string>
#include <filesystem>

class GUI {
public:
    GUI();
    ~GUI();
    
    void run();
    
private:
    void render_main_window();
    void render_backup_window();
    void render_restore_window();
    
    static constexpr size_t PATH_BUFFER_SIZE = 256;
    static constexpr size_t PASSWORD_BUFFER_SIZE = 64;
    
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
    std::string error_message_;
    
    std::string open_file_dialog(bool folder = false);
};

#endif // GUI_H 