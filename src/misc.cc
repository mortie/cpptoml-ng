#include "cpptoml.h"

#include <iomanip>
#include <fstream>

namespace cpptomlng
{

struct offset_datetime offset_datetime::from_zoned(const struct tm& t)
{
    offset_datetime dt;
    dt.year = t.tm_year + 1900;
    dt.month = t.tm_mon + 1;
    dt.day = t.tm_mday;
    dt.hour = t.tm_hour;
    dt.minute = t.tm_min;
    dt.second = t.tm_sec;

    char buf[16];
    strftime(buf, 16, "%z", &t);

    int offset = std::stoi(buf);
    dt.hour_offset = offset / 100;
    dt.minute_offset = offset % 100;
    return dt;
}

struct offset_datetime offset_datetime::from_utc(const struct tm& t)
{
    offset_datetime dt;
    dt.year = t.tm_year + 1900;
    dt.month = t.tm_mon + 1;
    dt.day = t.tm_mday;
    dt.hour = t.tm_hour;
    dt.minute = t.tm_min;
    dt.second = t.tm_sec;
    return dt;
}

fill_guard::fill_guard(std::ostream& os) : os_(os), fill_(os.fill())
{}

fill_guard::~fill_guard()
{
    os_.fill(fill_);
}

std::ostream& operator<<(std::ostream& os, const local_date& dt)
{
    fill_guard g{os};
    os.fill('0');

    using std::setw;
    os << setw(4) << dt.year << "-" << setw(2) << dt.month << "-" << setw(2)
       << dt.day;

    return os;
}

std::ostream& operator<<(std::ostream& os, const local_time& ltime)
{
    fill_guard g{os};
    os.fill('0');

    using std::setw;
    os << setw(2) << ltime.hour << ":" << setw(2) << ltime.minute << ":"
       << setw(2) << ltime.second;

    if (ltime.microsecond > 0)
    {
        os << ".";
        int power = 100000;
        for (int curr_us = ltime.microsecond; curr_us; power /= 10)
        {
            auto num = curr_us / power;
            os << num;
            curr_us -= num * power;
        }
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const local_datetime& dt)
{
    return os << static_cast<const local_date&>(dt) << "T"
              << static_cast<const local_time&>(dt);
}

std::ostream& operator<<(std::ostream& os, const offset_datetime& dt)
{
    return os << static_cast<const local_datetime&>(dt)
              << static_cast<const zone_offset&>(dt);
}

std::ostream& operator<<(std::ostream& os, const zone_offset& zo)
{
    fill_guard g{os};
    os.fill('0');

    using std::setw;

    if (zo.hour_offset != 0 || zo.minute_offset != 0)
    {
        if (zo.hour_offset > 0)
        {
            os << "+";
        }
        else
        {
            os << "-";
        }
        os << setw(2) << std::abs(zo.hour_offset) << ":" << setw(2)
           << std::abs(zo.minute_offset);
    }
    else
    {
        os << "Z";
    }

    return os;
}

std::shared_ptr<table> parse_file(const std::string& filename)
{
    std::ifstream file{filename};
    if (!file.is_open())
        throw parse_exception{filename + " could not be opened for parsing"};
    parser p{file};
    return p.parse();
}


}
