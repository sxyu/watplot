#include "stdafx.h"
namespace watplot {
    const std::map<std::string, char> consts::HEADER_KEYWORD_TYPES
                                    = consts::_header_keyword_types();
    const std::vector<std::string> consts::TELESCOPES
                                 = consts::_telescopes();
    const int64_t consts::MEMORY = consts::get_system_memory();
}