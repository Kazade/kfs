#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include "kfs.h"

#ifdef WIN32
#error "Must implement windows support"
#else
#include <utime.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#endif


namespace kfs {

static bool starts_with(const Path& p, const std::string& thing) {
    return p.find(thing) == 0;
}

static std::string slice(const std::string& input, uint32_t start, void* end=nullptr) {
    return std::string(input.begin() + start, input.end());
}

static std::string slice(const std::string &input, void* start, uint32_t end) {
    return std::string(input.begin(), input.begin() + end);
}

static std::string multiply(const std::string& input, const uint32_t count) {
    std::string result;
    for(uint32_t i = 0; i < count; ++i) result += input;
    return result;
}

static std::string rstrip(const std::string& input, const std::string& what=" \n\r\t") {
    std::string result = input;
    result.erase(result.find_last_not_of(what) + 1);
    return result;
}

static std::string str_join(const std::string& joiner, const std::vector<std::string>& parts) {
    std::string result;

    std::size_t i = 0;
    for(auto& part: parts) {
        result += part;
        if(++i != parts.size()) {
            result += joiner;
        }
    }

    return result;
}

static std::vector<std::string> str_split(const std::string& input, const std::string& on) {
    std::vector<std::string> elems;
    std::stringstream ss(input);
    std::string item;

    assert(on.length() == 1);

    while (std::getline(ss, item, on[0])) {
        if(item.empty()) continue;

        elems.push_back(item);
    }
    return elems;
}

static std::vector<std::string> common_prefix(const std::vector<std::string>& lhs, const std::vector<std::string>& rhs) {
    if(lhs.empty() && rhs.empty()) {
        return std::vector<std::string>();
    }

    auto shorter = (lhs.size() < rhs.size()) ? lhs: rhs;
    auto longer = (lhs.size() > rhs.size()) ? lhs: rhs;

    for(std::vector<std::string>::size_type i = 0; i < shorter.size(); ++i) {
        if(shorter[i] != longer[i]) {
            return std::vector<std::string>(shorter.begin(), shorter.begin() + i);
        }
    }

    return shorter;
}

// =================== END UTILITY FUNCTIONS ======================================================
// ================================================================================================

struct ::stat lstat(const Path& path) {
    struct ::stat result;
    if(::lstat(path.c_str(), &result) == -1) {
        throw IOError(errno);
    }
    return result;
}

void touch(const Path& path) {
    if(!kfs::path::exists(path)) {
        make_dirs(kfs::path::dir_name(path));

        std::ofstream file(path.c_str());
        file.close();
    }

    struct utimbuf new_times;
    struct stat st = kfs::lstat(path);

    new_times.actime = st.st_atime;
    new_times.modtime = time(NULL);
    utime(path.c_str(), &new_times);
}

void make_dir(const Path& path, Mode mode) {
    if(kfs::path::exists(path)) {
        throw kfs::IOError(EEXIST);
    } else {
        if(mkdir(path.c_str(), mode) != 0) {
            throw kfs::IOError(errno);
        }
    }
}

void make_link(const Path& source, const Path& dest) {
    int ret = ::symlink(source.c_str(), dest.c_str());
    if(ret != 0) {
        throw IOError(errno);
    }
}

void make_dirs(const Path &path, Mode mode) {
    std::pair<Path, Path> res = kfs::path::split(path);

    Path head = res.first;
    Path tail = res.second;

    if(tail.empty()) {
        res = kfs::path::split(head);
        head = res.first;
        tail = res.second;
    }

    if(!head.empty() && !tail.empty() && !kfs::path::exists(head)) {
        try {
            make_dirs(head, mode);
        } catch(kfs::IOError& e) {
            if(e.err != EEXIST) {
                //Someone already created the directory
                throw;
            }
        }
        if(tail == ".") {
            return;
        }
    }

    if(!kfs::path::exists(path)) {
        make_dir(path, mode);
    }
}

void remove(const Path& path) {
    if(kfs::path::exists(path)) {
        if(kfs::path::is_dir(path)) {
            throw IOError("Tried to remove a folder, use remove_dir instead");
        } else {
            ::remove(path.c_str());
        }
    }
}

void remove_dir(const Path& path) {
    if(!kfs::path::exists(path)) {
        throw IOError("Tried to remove a non-existent path");
    }

    if(kfs::path::list_dir(path).empty()) {
        if(rmdir(path.c_str()) != 0) {
            throw IOError(errno);
        }
    } else {
        throw IOError("Tried to remove a non-empty directory");
    }
}

void remove_dirs(const Path& path) {
    if(!kfs::path::exists(path)) {
        throw IOError("Tried to remove a non-existent path");
    }

    for(Path f: kfs::path::list_dir(path)) {
        Path full = kfs::path::join(path, f);
        if(kfs::path::is_dir(full)) {
            remove_dirs(full);
            remove_dir(full);
        } else {
            remove(full);
        }
    }
}

void rename(const Path& old, const Path& new_path) {
    if(::rename(old.c_str(), new_path.c_str()) != 0) {
        throw IOError(errno);
    }
}

std::string temp_dir() {
#ifdef WIN32
    assert(0 && "NotImplemented");
#endif
    return "/tmp";
}

Path exe_path() {
#ifdef WIN32
    assert(0 && "Not implemented");
#elif defined(__APPLE__)
    char buff[1024];
    uint32_t size = sizeof(buff);

    if(_NSGetExecutablePath(buff, &size) == 0) {
        return Path(buff);
    }

    throw std::runtime_error("Unable to work out the program filename");
#else
    char buff[1024];
    ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    if(len != -1) {
        buff[len] = '\0';
        return Path(buff);
    }

    throw std::runtime_error("Unable to work out the program filename");
#endif
}

Path exe_dirname() {
    Path path = exe_path();
    return path::dir_name(path);
}

Path get_cwd(){
    char buf[FILENAME_MAX];
    char* succ = getcwd(buf, FILENAME_MAX);

    if(succ) {
        return Path(succ);
    }

    throw std::runtime_error("Unable to get the current working directory");
}


namespace path {

Path join(const Path &p1, const Path &p2) {
    return p1 + SEP + p2;
}

Path join(const std::vector<Path>& parts) {
    Path ret;
    std::size_t i = 0;
    for(auto& part: parts) {
        ret += part;
        ++i;
        if(i != parts.size()) {
            ret += SEP;
        }
    }

    return ret;
}

Path abs_path(const Path& p) {
    Path path = p;
    if(!is_absolute(p)) {
        Path cwd = get_cwd();
        path = join(cwd, p);
    }

    return norm_path(path);
}

Path norm_case(const Path& path) {
#ifdef __WIN32__
    assert(0);
#endif
    return path;
}

Path norm_path(const Path& path) {
    Path slash = "/";
    Path dot = ".";

    if(path.empty()) {
        return dot;
    }

    int32_t initial_slashes = starts_with(path, SEP) ? 1: 0;
    //POSIX treats 3 or more slashes as a single one
    if(initial_slashes && starts_with(path, SEP + SEP) && !starts_with(path, SEP+SEP+SEP)) {
        initial_slashes = 2;
    }

    std::vector<Path> comps = str_split(path, slash);
    std::vector<Path> new_comps;

    for(Path comp: comps) {
        if(comp.empty() || comp == dot) {
            continue;
        }

        if(comp != ".." ||
            (initial_slashes == 0 && new_comps.empty()) ||
            (!new_comps.empty() && new_comps.back() == "..")
            ) {
            new_comps.push_back(comp);
        } else if(!new_comps.empty()) {
            new_comps.pop_back();
        }
    }

    comps = new_comps;
    Path final_path = str_join(slash, comps);
    if(initial_slashes) {
        final_path = multiply(slash, initial_slashes) + final_path;
    }

    return final_path.empty() ? dot : final_path;
}

std::pair<Path, Path> split(const Path& path) {
    Path::size_type i = path.rfind(SEP) + 1;

    Path head = slice(path, nullptr, i);
    Path tail = slice(path, i, nullptr);

    if(!head.empty() && head != multiply(SEP, head.length())) {
        head = rstrip(head, SEP);
    }

    return std::make_pair(head, tail);
}

bool exists(const Path &path) {
    try {
        lstat(path);
    } catch(IOError& e) {
        return false;
    }

    return true;
}

Path dir_name(const Path& path) {
    Path::size_type i = path.rfind(SEP) + 1;
    Path head = slice(path, nullptr, i);
    if(!head.empty() && head != multiply(SEP, head.length())) {
        head = rstrip(head, SEP);
    }
    return head;
}

bool is_absolute(const Path& path) {
    return starts_with(path, "/");
}

bool is_dir(const Path& path) {
    struct stat st;
    try {
        st = lstat(path);
    } catch(IOError& e) {
        return false;
    }

    return S_ISDIR(st.st_mode);
}

bool is_file(const Path& path) {
    struct stat st;
    try {
        st = lstat(path);
    } catch(IOError& e) {
        return false;
    }

    return S_ISREG(st.st_mode);
}

bool is_link(const Path& path) {
    struct stat st;
    try {
        st = lstat(path);
    } catch(IOError& e) {
        return false;
    }

    return S_ISLNK(st.st_mode);
}

Path real_path(const Path& path) {
    char *real_path = realpath(path.c_str(), NULL);
    if(!real_path) {
        return Path();
    }
    Path result(real_path);
    free(real_path);
    return result;
}

Path rel_path(const Path& path, const Path& start) {
    if(path.empty()) {
        return "";
    }

    auto start_list = str_split(path::abs_path(start), SEP);
    auto path_list = str_split(path::abs_path(path), SEP);

    int i = common_prefix(start_list, path_list).size();

    Path pardir = "..";

    std::vector<Path> result;
    for(std::vector<std::string>::size_type j = 0; j < (start_list.size() - i); ++j) {
        result.push_back(pardir);
    }

    result.insert(result.end(), path_list.begin() + i, path_list.end());
    if(result.empty()) {
        return ".";
    }

    return path::join(result);
}

static Path get_env_var(const Path& name) {
    char* env = getenv(name.c_str());
    if(env) {
        return Path(env);
    }

    return Path();
}

Path expand_user(const Path& path) {
    Path cp = path;

    if(!starts_with(path, "~")) {
        return path;
    }

#ifdef WIN32
#error "Needs implementing"
#else
    Path home = get_env_var("HOME");
#endif

    if(home.empty()) {
        return path;
    }

    cp.replace(cp.find("~"), 1, home);
    return cp;
}

void hide_dir(const Path &path) {
#ifdef WIN32
    assert(0 && "Not Implemented");
#else
    //On Unix systems, prefix with a dot
    std::pair<Path, Path> parts = path::split(path);
    Path final = parts.first + "." + parts.second;
    if(::rename(path.c_str(), final.c_str()) != 0) {
        throw IOError(errno);
    }
#endif

}

std::vector<Path> list_dir(const Path& path) {
    std::vector<Path> result;
#ifdef WIN32
    assert(0 && "Not implemented");
#else
    if(!is_dir(path)) {
        throw IOError(errno);
    }

    DIR* dirp = opendir(path.c_str());
    dirent* dp = nullptr;

    while((dp = readdir(dirp)) != nullptr) {
        result.push_back(dp->d_name);
        if(result.back() == "." || result.back() == "..") {
            result.pop_back();
        }
    }
    closedir(dirp);
#endif
    return result;
}

std::string read_file_contents(const Path& path) {
    std::ifstream t(path);
    std::string str((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());

    return str;
}


std::pair<Path, Path> split_ext(const Path& path) {
    auto sep_index = path.rfind(SEP);
    auto dot_index = path.rfind(".");

    if(sep_index == Path::npos) {
        sep_index = -1;
    }

    if(dot_index > sep_index) {
        auto filename_index = sep_index + 1;

        while(filename_index < dot_index) {
            if(path[filename_index] != '.') {
                return std::make_pair(
                    slice(path, nullptr, dot_index),
                    slice(path, dot_index, nullptr)
                );
            }
            filename_index += 1;
        }
    }

    return std::make_pair(path, "");
}



}

std::string IOError::get_message(int err) {
    switch(err) {
    case EEXIST: return "File or folder already exists";
    case ELOOP: return "Loop exists in symbolic links";
    case EACCES: return "Permission denied";
    case EMLINK: return "Exceeded link count of the parent directory";
    case ENAMETOOLONG: return "Path is too long, exceeded PATH_MAX";
    case ENOENT: return "Path was invalid";
    case ENOSPC: return "Not enough disk space";
    case ENOTDIR: return "Not a directory";
    case EROFS: return "Read-only filesystem";
    default:
        return "Unspecified error";
    }
}


}
