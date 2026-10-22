#include "platform/win32/Win32ProcessRunner.h"

#include <array>
#include <exception>
#include <thread>

#include <windows.h>

#include "platform/Cancelled.h"
#include "platform/win32/CommandLine.h"
#include "platform/win32/Environment.h"
#include "platform/win32/HandleInheritanceList.h"
#include "platform/win32/Pipe.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

constexpr UINT TerminatedExitCode = 1;
constexpr DWORD CreationFlags = CREATE_SUSPENDED | CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT | EXTENDED_STARTUPINFO_PRESENT;
// Python writes into a pipe in the system's code page unless it's told otherwise
constexpr std::array<EnvironmentVariable, 2> ChildEnvironment = {{{.name = L"PYTHONUTF8", .value = L"1"}, {.name = L"PYTHONIOENCODING", .value = L"utf-8"}}};

UniqueKernelObject create_job() {
    UniqueKernelObject job(check(CreateJobObjectW(nullptr, nullptr), "CreateJobObjectW"));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    check(SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &limits, sizeof(limits)), "SetInformationJobObject");
    return job;
}

UniqueFile open_empty_input() {
    SECURITY_ATTRIBUTES attributes{.nLength = sizeof(SECURITY_ATTRIBUTES), .lpSecurityDescriptor = nullptr, .bInheritHandle = TRUE};
    UniqueFile input = adopt_file(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &attributes, OPEN_EXISTING, 0, nullptr));
    if (!input) {
        throw_last_error("CreateFileW");
    }
    return input;
}

}

ProcessResult Win32ProcessRunner::run(const std::filesystem::path& program, std::span<const std::string> arguments, const std::stop_token& stop) const {
    if (stop.stop_requested()) {
        throw Cancelled();
    }

    // The job comes first, so nothing can fail between starting the child and putting it in the job
    UniqueKernelObject job = create_job();
    Pipe output = create_output_pipe();
    Pipe errors = create_output_pipe();
    UniqueFile input = open_empty_input();
    // A child started at the same time on another thread would otherwise inherit these too, and keep the pipes open
    HandleInheritanceList inherited({input.get(), output.write.get(), errors.write.get()});

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input.get();
    startup.StartupInfo.hStdOutput = output.write.get();
    startup.StartupInfo.hStdError = errors.write.get();
    startup.lpAttributeList = inherited.get();
    std::wstring line = command_line(program, arguments);
    std::wstring environment = environment_block_with(ChildEnvironment);
    PROCESS_INFORMATION started{};
    check(
            CreateProcessW(program.c_str(), line.data(), nullptr, nullptr, TRUE, CreationFlags, environment.data(), nullptr, &startup.StartupInfo, &started),
            "CreateProcessW"
    );
    UniqueKernelObject process(started.hProcess);
    UniqueKernelObject thread(started.hThread);

    if (!AssignProcessToJobObject(job.get(), process.get())) {
        DWORD error = GetLastError();
        TerminateProcess(process.get(), TerminatedExitCode);
        throw_error(error, "AssignProcessToJobObject");
    }
    // The pipes break once the child has the only write ends, and it ends
    output.write.reset();
    errors.write.reset();
    input.reset();

    // From here on, leaving early closes the job, which ends the child
    std::stop_callback cancel(stop, [&job] { TerminateJobObject(job.get(), TerminatedExitCode); });
    // A stop that terminated the job first leaves no thread to resume
    if (ResumeThread(thread.get()) == static_cast<DWORD>(-1) && !stop.stop_requested()) {
        throw_last_error("ResumeThread");
    }

    ProcessResult result;
    std::exception_ptr errors_failure;
    {
        // Both pipes are read at once, or a child that fills one would wait for the other forever
        std::jthread errors_reader([&] {
            try {
                result.errors = read_all(errors.read.get());
            } catch (...) {
                errors_failure = std::current_exception();
            }
        });
        try {
            result.output = read_all(output.read.get());
        } catch (...) {
            // The reader's thread ends only with the child
            TerminateJobObject(job.get(), TerminatedExitCode);
            throw;
        }
    }
    if (errors_failure) {
        std::rethrow_exception(errors_failure);
    }

    if (WaitForSingleObject(process.get(), INFINITE) == WAIT_FAILED) {
        throw_last_error("WaitForSingleObject");
    }
    if (stop.stop_requested()) {
        throw Cancelled();
    }
    DWORD exit_code = 0;
    check(GetExitCodeProcess(process.get(), &exit_code), "GetExitCodeProcess");
    result.exit_code = static_cast<int>(exit_code);
    return result;
}

}
