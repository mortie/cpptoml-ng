#include "cpptoml.h"

#include <sstream>
#include <cassert>
#include <clocale>

namespace cpptomlng
{

/**
 * Output a table element of the TOML tree
 */
void toml_writer::visit(const table& t, bool in_array)
{
    write_table_header(in_array);
    std::vector<std::string> values;
    std::vector<std::string> tables;

    for (const auto& i : t)
    {
        if (i.second->is_table() || i.second->is_table_array())
        {
            tables.push_back(i.first);
        }
        else
        {
            values.push_back(i.first);
        }
    }

    for (unsigned int i = 0; i < values.size(); ++i)
    {
        path_.push_back(values[i]);

        if (i > 0)
            endline();

        write_table_item_header(*t.get(values[i]));
        t.get(values[i])->accept(*this, false);
        path_.pop_back();
    }

    for (unsigned int i = 0; i < tables.size(); ++i)
    {
        path_.push_back(tables[i]);

        if (values.size() > 0 || i > 0)
            endline();

        write_table_item_header(*t.get(tables[i]));
        t.get(tables[i])->accept(*this, false);
        path_.pop_back();
    }

    endline();
}

/**
 * Output an array element of the TOML tree
 */
void toml_writer::visit(const array& a, bool)
{
    write("[");

    for (unsigned int i = 0; i < a.get().size(); ++i)
    {
        if (i > 0)
            write(", ");

        if (a.get()[i]->is_array())
        {
            a.get()[i]->as_array()->accept(*this, true);
        }
        else
        {
            a.get()[i]->accept(*this, true);
        }
    }

    write("]");
}

/**
 * Output a table_array element of the TOML tree
 */
void toml_writer::visit(const table_array& t, bool)
{
    for (unsigned int j = 0; j < t.get().size(); ++j)
    {
        if (j > 0)
            endline();

        t.get()[j]->accept(*this, true);
    }

    endline();
}

/**
 * Escape a string for output.
 */
std::string toml_writer::escape_string(const std::string& str)
{
    std::string res;
    for (auto it = str.begin(); it != str.end(); ++it)
    {
        if (*it == '\b')
        {
            res += "\\b";
        }
        else if (*it == '\t')
        {
            res += "\\t";
        }
        else if (*it == '\n')
        {
            res += "\\n";
        }
        else if (*it == '\f')
        {
            res += "\\f";
        }
        else if (*it == '\r')
        {
            res += "\\r";
        }
        else if (*it == '"')
        {
            res += "\\\"";
        }
        else if (*it == '\\')
        {
            res += "\\\\";
        }
        else if (static_cast<uint32_t>(*it) <= UINT32_C(0x001f))
        {
            res += "\\u";
            std::stringstream ss;
            ss << std::hex << static_cast<uint32_t>(*it);
            res += ss.str();
        }
        else
        {
            res += *it;
        }
    }
    return res;
}

/**
 * Write out a string.
 */
void toml_writer::write(const value<std::string>& v)
{
    write("\"");
    write(escape_string(v.get()));
    write("\"");
}

/**
 * Write out a double.
 */
void toml_writer::write(const value<double>& v)
{
    std::stringstream ss;
    ss << std::showpoint
       << std::setprecision(std::numeric_limits<double>::max_digits10)
       << v.get();

    auto double_str = ss.str();
    auto pos = double_str.find("e0");
    if (pos != std::string::npos)
        double_str.replace(pos, 2, "e");
    pos = double_str.find("e-0");
    if (pos != std::string::npos)
        double_str.replace(pos, 3, "e-");

    stream_ << double_str;
    has_naked_endline_ = false;
}

/**
 * Write out a boolean.
 */
void toml_writer::write(const value<bool>& v)
{
    write((v.get() ? "true" : "false"));
}

/**
 * Write out the header of a table.
 */
void toml_writer::write_table_header(bool in_array)
{
    if (!path_.empty())
    {
        indent();

        write("[");

        if (in_array)
        {
            write("[");
        }

        for (unsigned int i = 0; i < path_.size(); ++i)
        {
            if (i > 0)
            {
                write(".");
            }

            if (path_[i].find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcde"
                                           "fghijklmnopqrstuvwxyz0123456789"
                                           "_-")
                == std::string::npos)
            {
                write(path_[i]);
            }
            else
            {
                write("\"");
                write(escape_string(path_[i]));
                write("\"");
            }
        }

        if (in_array)
        {
            write("]");
        }

        write("]");
        endline();
    }
}

/**
 * Write out the identifier for an item in a table.
 */
void toml_writer::write_table_item_header(const base& b)
{
    if (!b.is_table() && !b.is_table_array())
    {
        indent();

        if (path_.back().find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcde"
                                           "fghijklmnopqrstuvwxyz0123456789"
                                           "_-")
            == std::string::npos)
        {
            write(path_.back());
        }
        else
        {
            write("\"");
            write(escape_string(path_.back()));
            write("\"");
        }

        write(" = ");
    }
}

/**
 * Indent the proper number of tabs given the size of
 * the path.
 */
void toml_writer::indent()
{
    for (std::size_t i = 1; i < path_.size(); ++i)
        write(indent_);
}

/**
 * Write an endline out to the stream
 */
void toml_writer::endline()
{
    if (!has_naked_endline_)
    {
        stream_ << "\n";
        has_naked_endline_ = true;
    }
}

}
