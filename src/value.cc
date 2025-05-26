#include "cpptoml.h"

namespace cpptomlng
{

/*
 * Array
 */

std::vector<std::shared_ptr<array>>
array::nested_array() const
{
    std::vector<std::shared_ptr<array>> result(values_.size());

    std::transform(values_.begin(), values_.end(), result.begin(),
                   [&](std::shared_ptr<base> v) -> std::shared_ptr<array> {
                       if (v->is_array())
                           return std::static_pointer_cast<array>(v);
                       return std::shared_ptr<array>{};
                   });

    return result;
}

void array::push_back(const std::shared_ptr<array>& val)
{
    if (values_.empty() || values_[0]->is_array())
    {
        values_.push_back(val);
    }
    else
    {
        throw array_exception{"Arrays must be homogenous."};
    }
}

array::iterator
array::insert(iterator position, const std::shared_ptr<array>& value)
{
    if (values_.empty() || values_[0]->is_array())
    {
        return values_.insert(position, value);
    }
    else
    {
        throw array_exception{"Arrays must be homogenous."};
    }
}

std::shared_ptr<array> make_array()
{
    struct make_shared_enabler : public array
    {
        make_shared_enabler()
        {
            // nothing
        }
    };

    return std::make_shared<make_shared_enabler>();
}

/*
 * Table array
 */

std::shared_ptr<table_array> make_table_array(bool is_inline)
{
    struct make_shared_enabler : public table_array
    {
        make_shared_enabler(bool mse_is_inline) : table_array(mse_is_inline)
        {
            // nothing
        }
    };

    return std::make_shared<make_shared_enabler>(is_inline);
}

/*
 * Table
 */

std::vector<std::string> table::split(const std::string& value,
                                      char separator) const
{
    std::vector<std::string> result;
    std::string::size_type p = 0;
    std::string::size_type q;
    while ((q = value.find(separator, p)) != std::string::npos)
    {
        result.emplace_back(value, p, q - p);
        p = q + 1;
    }
    result.emplace_back(value, p);
    return result;
}

bool table::resolve_qualified(const std::string& key,
                              std::shared_ptr<base>* p) const
{
    auto parts = split(key, '.');
    auto last_key = parts.back();
    parts.pop_back();

    auto cur_table = this;
    for (const auto& part : parts)
    {
        cur_table = cur_table->get_table(part).get();
        if (!cur_table)
        {
            if (!p)
                return false;

            throw std::out_of_range{key + " is not a valid key"};
        }
    }

    if (!p)
        return cur_table->map_.count(last_key) != 0;

    *p = cur_table->map_.at(last_key);
    return true;
}

std::shared_ptr<table> make_table()
{
    struct make_shared_enabler : public table
    {
        make_shared_enabler()
        {
            // nothing
        }
    };

    return std::make_shared<make_shared_enabler>();
}

}
