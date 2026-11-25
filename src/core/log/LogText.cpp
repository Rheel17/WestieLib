#include "core/log/LogText.h"

#include <format>
#include <vector>

namespace westie::core {

namespace {

constexpr std::string_view Indent = "    ";

std::string_view file_level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace:   return "TRACE";
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error:   return "ERROR";
    }
    return "";
}

std::vector<std::string_view> lines_of(std::string_view text) {
    std::vector<std::string_view> lines;
    while (!text.empty()) {
        std::size_t end = text.find('\n');
        std::string_view line = text.substr(0, end);
        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }
        lines.push_back(line);
        text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
    }
    return lines;
}

// Every line of the file starts with a time or with spaces, so a multi-line text can't pass for lines of its own
void append_indented(std::string& out, std::string_view text, std::string_view indent) {
    for (std::string_view line : lines_of(text)) {
        out += indent;
        out += line;
        out += '\n';
    }
}

std::string subject_text(const LogSubject& subject) {
    std::string text;
    auto add = [&](std::string_view part) {
        text += text.empty() ? " [" : "; ";
        text += part;
    };
    if (!subject.name.empty()) {
        add(subject.name);
    }
    if (!subject.playlist.empty()) {
        add(std::format("playlist {}", subject.playlist));
    }
    if (!subject.track.empty()) {
        add(std::format("track {}", subject.track));
    }
    if (!text.empty()) {
        text += ']';
    }
    return text;
}

std::string quoted_argument(std::string_view argument) {
    if (!argument.empty() && argument.find_first_of(" \t\"") == std::string_view::npos) {
        return std::string(argument);
    }
    std::string text = "\"";
    for (char character : argument) {
        if (character == '"') {
            text += '\\';
        }
        text += character;
    }
    text += '"';
    return text;
}

}

std::string_view log_level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace:   return "Trace";
    case LogLevel::Debug:   return "Debug";
    case LogLevel::Info:    return "Info";
    case LogLevel::Warning: return "Warning";
    case LogLevel::Error:   return "Error";
    }
    return "";
}

std::string format_log_time(std::chrono::local_time<std::chrono::milliseconds> time) {
    return std::format("{:%F %T}", time);
}

std::filesystem::path log_file_name(std::chrono::local_time<std::chrono::milliseconds> time) {
    return std::format(L"{:%F_%H-%M-%S}.log", std::chrono::floor<std::chrono::seconds>(time));
}

std::string log_file_line(const LogLine& line) {
    std::vector<std::string_view> lines = lines_of(line.text);
    std::string text = std::format("{} {:<7} {}{}\n", format_log_time(line.time), file_level_name(line.level), lines.empty() ? "" : lines.front(), subject_text(line.subject));
    for (std::size_t index = 1; index < lines.size(); ++index) {
        append_indented(text, lines[index], Indent);
    }
    return text;
}

std::string command_line(std::string_view tool, std::span<const std::string> arguments) {
    std::string command(tool);
    for (const std::string& argument : arguments) {
        command += ' ';
        command += quoted_argument(argument);
    }
    return command;
}

// A log line is a row in the pane
std::string one_line(std::string_view text) {
    std::string line;
    for (char character : text) {
        if (character == '\n') {
            line += "; ";
        } else if (character != '\r') {
            line += character;
        }
    }
    return line;
}

std::vector<std::string> tool_output_lines(std::string_view tool, const platform::ProcessResult& result) {
    std::vector<std::string> lines;
    if (result.exit_code != 0) {
        lines.push_back(std::format("{} exited with code {}", tool, result.exit_code));
    }
    auto add_output = [&](std::string_view output) {
        for (std::string_view line : lines_of(output)) {
            if (!line.empty()) {
                lines.push_back(std::format("{}: {}", tool, line));
            }
        }
    };
    add_output(result.errors);
    // A successful run's output is what the app reads, as yt-dlp's JSON answers, which run to megabytes
    if (result.exit_code != 0) {
        add_output(result.output);
    }
    return lines;
}

}
