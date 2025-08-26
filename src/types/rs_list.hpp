#pragma once
#include <vector>
#include <memory>

#include "rbc_value.hpp"
#include "rbc_register.hpp"

#include "../type_info.hpp"

struct rs_list
{
    rs_type_info elementType;
    std::vector<std::shared_ptr<rbc_value>> values;


    inline std::string tostr()
    {
        std::stringstream stream;
        stream << '[';

        for(auto& val : values)
        {
            rbc_value& v = *val;
            switch(v.index())
            {
                case 0:
                    stream << std::get<0>(v).tostr();
                    break;
                case 1:
                    stream << std::get<1>(v)->tostr();
                    break;
                case 2:
                    stream << std::get<2>(v)->tostr();
                    break;
                case 3:
                    stream << std::get<3>(v)->tostr();
                    break;
                case 4:
                    stream << std::get<4>(v)->tostr();
                    break;
                case 6:
                    stream << std::get<6>(v).toPath();
                    break;
            }
            stream << ',';
        }
        if (values.size() > 0)
            stream.seekp(-1, std::ios_base::end);
        stream << ']';
        return stream.str();
    }
};