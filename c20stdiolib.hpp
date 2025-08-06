#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <concepts>
#include <type_traits>
#include <vector>
#include <optional>
#include <format>

namespace iolib {

// ====================== 概念约束 ======================
template <typename T>
concept Writable = requires(T t, std::ostream& os) {
    { os << t } -> std::same_as<std::ostream&>;
};

template <typename T>
concept Readable = requires(T t, std::istream& is) {
    { is >> t } -> std::same_as<std::istream&>;
};

// ====================== 异常处理 ======================
class IOError : public std::runtime_error {
public:
    explicit IOError(const std::string& msg) 
        : std::runtime_error("IOError: " + msg) {}
};

// ====================== 控制台流 ======================
class Console {
public:
    // 格式化输出
    template <Writable... Args>
    static void print(std::format_string<Args...> fmt, Args&&... args) {
        std::cout << std::format(fmt, std::forward<Args>(args)...);
    }

    // 格式化输出并换行
    template <Writable... Args>
    static void println(std::format_string<Args...> fmt, Args&&... args) {
        print(fmt, std::forward<Args>(args)...);
        std::cout << '\n';
    }

    // 读取一行
    static std::string read_line() {
        std::string line;
        if (!std::getline(std::cin, line)) {
            抛出 IO错误("无法从控制台读取");
        }
        返回 行;
    }

    // 安全读取类型化值
    模板 <Readable可读 T>
    静态 T 读取 (){) 
        T 值;T value;
        如果 (!(标准输入 >> value)) {
            抛出 IOError("无法从控制台读取值");
        }
        // 清除换行符
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return value;
    }

    // 带提示的读取
    template <Readable T>
    static T prompt(std::string_view message) {
        print("{}", message);
        return read<T>();
    }
};

// ====================== 文件流 ======================
enum class FileMode {
    Read,
    Write,
    Append,
    ReadWrite
};

class FileStream {
public:
    // 打开文件 (RAII)
    explicit FileStream(const std::string& path, FileMode mode = FileMode::Read) 
        : stream_(open_stream(path, mode)) {}

    ~FileStream() {
        if (stream_.is_open()) {
            stream_.close();
        }
    }

    // 禁止拷贝
    FileStream(const FileStream&) = delete;
    FileStream& operator=(const FileStream&) = delete;

    // 允许移动
    FileStream(FileStream&& other) noexcept 
        : stream_(std::move(other.stream_)) {}
    
    FileStream& operator=(FileStream&& other) noexcept {
        if (this != &other) {
            if (stream_.is_open()) stream_.close();
            stream_ = std::move(other.stream_);
        }
        return *this;
    }

    // 检查文件是否打开
    bool is_open() const { return stream_.is_open(); }

    // 读取全部内容
    std::string read_all() {
        if (!stream_) throw IOError("File not open for reading");
        
        stream_.seekg(0, std::ios::end);
        auto size = stream_.tellg();
        stream_.seekg(0, std::ios::beg);
        
        if (size <= 0) return "";
        
        std::string content(size, '\0');
        stream_.read(&content[0], size);
        return content;
    }

    // 读取一行
    std::optional<std::string> read_line() {
        if (!stream_) return std::nullopt;
        
        std::string line;
        if (std::getline(stream_, line)) {
            return line;
        }
        return std::nullopt;
    }

    // 读取类型化值
    template <Readable T>
    std::optional<T> read() {
        T value;
        if (stream_ >> value) {
            return value;
        }
        return std::nullopt;
    }

    // 写入内容
    template <Writable T>
    void write(const T& value) {
        if (!stream_) throw IOError("File not open for writing");
        stream_ << value;
    }

    // 格式化写入
    template <Writable... Args>
    void write_fmt(std::format_string<Args...> fmt, Args&&... args) {
        if (!stream_) throw IOError("File not open for writing");
        stream_ << std::format(fmt, std::forward<Args>(args)...);
    }

    // 写入一行
    template <Writable T>
    void write_line(const T& value) {
        write(value);
        stream_ << '\n';
    }

    // 格式化写入一行
    template <Writable... Args>
    void write_line_fmt(std::format_string<Args...> fmt, Args&&... args) {
        write_fmt(fmt, std::forward<Args>(args)...);
        stream_ << '\n';
    }

    // 文件定位
    void seek(size_t pos) {
        stream_.seekg(pos);
        stream_.seekp(pos);
    }

    size_t tell() const {
        return stream_.tellg();
    }

private:
    mutable std::fstream stream_;

    static std::fstream open_stream(const std::string& path, FileMode mode) {
        std::ios::openmode flags = std::ios::binary;
        
        switch (mode) {
            case FileMode::Read: 
                flags |= std::ios::in; 
                break;
            case FileMode::Write: 
                flags |= std::ios::out | std::ios::trunc; 
                break;
            case FileMode::Append: 
                flags |= std::ios::out | std::ios::app; 
                break;
            case FileMode::ReadWrite: 
                flags |= std::ios::in | std::ios::out; 
                break;
        }

        std::fstream stream(path, flags);
        if (!stream.is_open()) {
            throw IOError("Failed to open file: " + path);
        }
        return stream;
    }
};

// ====================== 实用函数 ======================
// 读取整个文件
inline std::string read_file(const std::string& path) {
    FileStream file(path, FileMode::Read);
    return file.read_all();
}

// 写入文件 (覆盖)
template <Writable T>
inline void write_file(const std::string& path, const T& content) {
    FileStream file(path, FileMode::Write);
    file.write(content);
}

// 追加到文件
template <Writable T>
inline void append_file(const std::string& path, const T& content) {
    FileStream file(path, FileMode::Append);
    file.write(content);
}

// 读取所有行
inline std::vector<std::string> read_lines(const std::string& path) {
    FileStream file(path, FileMode::Read);
    std::vector<std::string> lines;
    
    while (auto line = file.read_line()) {
        lines.push_back(*line);
    }
    
    return lines;
}

} // namespace iolib
