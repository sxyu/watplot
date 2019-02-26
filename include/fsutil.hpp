#pragma once

#include <vector>
#include <utility>
#include <string>
#include <cstdlib>
#include <cstdio>
#include "tinydir.h"

// filesystem helpers
// lists a directory; returns list of (path, file name)
std::vector<std::pair<std::string, std::string>> lsdir(const std::string & path)
{
    tinydir_dir dir;
    tinydir_open(&dir, path.c_str());

    std::vector<std::pair<std::string, std::string>> out;
    while (dir.has_next) {
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        std::string fname(file.name);
        if (fname != "." && fname != "..") {
            out.emplace_back(std::string(file.path), fname);
        }
        tinydir_next(&dir);
    }
    return out;
}

// checks if a file exists
bool file_exists(const std::string & name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    }
    return false;
}

// joins two paths
std::string join_path(const std::string & x, const std::string & y) {
    if (x.empty()) return y;
    if (y.empty()) return x;
    if (x.back() == '/' && y.front() == '/') return x + y.substr(1);
    if (x.back() != '/' && y.front() != '/') return x + '/' + y;
    return x + y;
}

// create a directory
void create_dir(const std::string & path) {
#if defined(_WIN32)
    CreateDirectoryA(path.c_str(), NULL);
#else
    int r = std::system((std::string("mkdir -p ") + path).c_str());
    if (r) {
        std::cerr << "WARNING: Create directory " << path << " failed!\n";
    }
#endif
}

long long filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

