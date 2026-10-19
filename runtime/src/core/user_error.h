#pragma once

#include <string>

namespace user_error {

enum class IssueKind { Setup, InGame };

std::string wrap_message(const std::string& text, size_t width = 56);

std::string collect_logs_text(size_t max_bytes = 256 * 1024);

std::string report_issue_url(const std::string& message, IssueKind kind = IssueKind::Setup);

void open_url(const char* url);
void open_logs_folder();

void show_fatal(const std::string& message, IssueKind kind = IssueKind::Setup);

[[noreturn]] void fail(const std::string& message, IssueKind kind = IssueKind::Setup);

}
