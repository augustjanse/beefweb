#include "file_system.hpp"

#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace msrv {

namespace {

void statToFileInfo(const struct ::stat& st, FileInfo& info)
{
    if (S_ISREG(st.st_mode))
        info.type = FileType::REGULAR;
    else if (S_ISDIR(st.st_mode))
        info.type = FileType::DIRECTORY;
    else
        info.type = FileType::UNKNOWN;

    info.size = st.st_size;
    info.inode = st.st_ino;
    info.timestamp = st.st_mtime;
}

}

std::string pathToUtf8(const Path& path)
{
    return path.string();
}

Path pathFromUtf8(const std::string& path)
{
    return Path(path);
}

Path getModulePath(void* symbol)
{
    ::Dl_info info;

    if (::dladdr(symbol, &info) == 0 || !info.dli_fname)
        return Path();

    return Path(info.dli_fname);
}

FileHandle openFile(const Path& path)
{
    return FileHandle(::open(path.c_str(), O_RDONLY | O_CLOEXEC));
}

FileInfo queryFileInfo(const Path& path)
{
    struct ::stat st;
    int ret = ::stat(path.c_str(), &st);
    throwIfFailed("stat", ret >= 0);

    FileInfo info;
    statToFileInfo(st, info);
    return info;
}

FileInfo queryFileInfo(FileHandle& handle)
{
    assert(handle);

    struct ::stat st;
    int ret = ::fstat(handle.get(), &st);
    throwIfFailed("fstat", ret >= 0);

    FileInfo info;
    statToFileInfo(st, info);
    return info;
}

std::vector<uint8_t> readFileToBuffer(FileHandle& handle, int64_t bytes)
{
    if (bytes < 0)
        bytes = queryFileInfo(handle).size;

    std::vector<uint8_t> buffer;
    buffer.resize(static_cast<size_t>(bytes));

    auto bytesRead = ::read(handle.get(), buffer.data(), static_cast<size_t>(bytes));
    throwIfFailed("read", bytesRead >= 0);

    buffer.resize(static_cast<size_t>(bytesRead));
    return buffer;
}

}