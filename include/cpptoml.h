/**
 * @file cpptoml.h
 * @author Chase Geigle
 * @date May 2013
 */

#ifndef CPPTOMLNG_H
#define CPPTOMLNG_H

#include <algorithm>
#include <optional>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace cpptomlng
{
class writer; // forward declaration
class base;   // forward declaration
// by default an unordered_map is used for best performance as the
// toml specification does not require entries to be sorted
using string_to_base_map
    = std::unordered_map<std::string, std::shared_ptr<base>>;

template<typename T>
using option = std::optional<T>;

struct local_date
{
    int year = 0;
    int month = 0;
    int day = 0;
};

struct local_time
{
    int hour = 0;
    int minute = 0;
    int second = 0;
    int microsecond = 0;
};

struct zone_offset
{
    int hour_offset = 0;
    int minute_offset = 0;
};

struct local_datetime : local_date, local_time
{
};

struct offset_datetime : local_datetime, zone_offset
{
    static struct offset_datetime from_zoned(const struct tm& t);

    static struct offset_datetime from_utc(const struct tm& t);
};

class fill_guard
{
  public:
    fill_guard(std::ostream& os);

    ~fill_guard();

  private:
    std::ostream& os_;
    char fill_;
};

std::ostream& operator<<(std::ostream& os, const local_date& dt);

std::ostream& operator<<(std::ostream& os, const local_time& ltime);

std::ostream& operator<<(std::ostream& os, const zone_offset& zo);

std::ostream& operator<<(std::ostream& os, const local_datetime& dt);

std::ostream& operator<<(std::ostream& os, const offset_datetime& dt);

template <class T, class... Ts>
struct is_one_of;

template <class T, class V>
struct is_one_of<T, V> : std::is_same<T, V>
{
};

template <class T, class V, class... Ts>
struct is_one_of<T, V, Ts...>
{
    const static bool value
        = std::is_same<T, V>::value || is_one_of<T, Ts...>::value;
};

template <class T>
class value;

template <class T>
struct valid_value
    : is_one_of<T, std::string, int64_t, double, bool, local_date, local_time,
                local_datetime, offset_datetime>
{
};

template <class T, class Enable = void>
struct value_traits;

template <class T>
struct valid_value_or_string_convertible
{

    const static bool value = valid_value<typename std::decay<T>::type>::value
                              || std::is_convertible<T, std::string>::value;
};

template <class T>
struct value_traits<T, typename std::enable_if<
                           valid_value_or_string_convertible<T>::value>::type>
{
    using value_type = typename std::conditional<
        valid_value<typename std::decay<T>::type>::value,
        typename std::decay<T>::type, std::string>::type;

    using type = value<value_type>;

    static value_type construct(T&& val)
    {
        return value_type(val);
    }
};

template <class T>
struct value_traits<
    T,
    typename std::enable_if<
        !valid_value_or_string_convertible<T>::value
        && std::is_floating_point<typename std::decay<T>::type>::value>::type>
{
    using value_type = typename std::decay<T>::type;

    using type = value<double>;

    static value_type construct(T&& val)
    {
        return value_type(val);
    }
};

template <class T>
struct value_traits<
    T, typename std::enable_if<
           !valid_value_or_string_convertible<T>::value
           && !std::is_floating_point<typename std::decay<T>::type>::value
           && std::is_signed<typename std::decay<T>::type>::value>::type>
{
    using value_type = int64_t;

    using type = value<int64_t>;

    static value_type construct(T&& val)
    {
        if (val < (std::numeric_limits<int64_t>::min)())
            throw std::underflow_error{"constructed value cannot be "
                                       "represented by a 64-bit signed "
                                       "integer"};

        if (val > (std::numeric_limits<int64_t>::max)())
            throw std::overflow_error{"constructed value cannot be represented "
                                      "by a 64-bit signed integer"};

        return static_cast<int64_t>(val);
    }
};

template <class T>
struct value_traits<
    T, typename std::enable_if<
           !valid_value_or_string_convertible<T>::value
           && std::is_unsigned<typename std::decay<T>::type>::value>::type>
{
    using value_type = int64_t;

    using type = value<int64_t>;

    static value_type construct(T&& val)
    {
        if (val > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()))
            throw std::overflow_error{"constructed value cannot be represented "
                                      "by a 64-bit signed integer"};

        return static_cast<int64_t>(val);
    }
};

class array;
class table;
class table_array;

template <class T>
struct array_of_trait
{
    using return_type = option<std::vector<T>>;
};

template <>
struct array_of_trait<array>
{
    using return_type = option<std::vector<std::shared_ptr<array>>>;
};

template <class T>
inline std::shared_ptr<typename value_traits<T>::type> make_value(T&& val);
std::shared_ptr<array> make_array();

namespace detail
{
template <class T>
inline std::shared_ptr<T> make_element();
}

std::shared_ptr<table> make_table();
std::shared_ptr<table_array> make_table_array(bool is_inline = false);

/**
 * A generic base TOML value used for type erasure.
 */
class base : public std::enable_shared_from_this<base>
{
  public:
    virtual ~base() = default;

    virtual std::shared_ptr<base> clone() const = 0;

    /**
     * Determines if the given TOML element is a value.
     */
    virtual bool is_value() const
    {
        return false;
    }

    /**
     * Determines if the given TOML element is a table.
     */
    virtual bool is_table() const
    {
        return false;
    }

    /**
     * Converts the TOML element into a table.
     */
    std::shared_ptr<table> as_table()
    {
        if (is_table())
            return std::static_pointer_cast<table>(shared_from_this());
        return nullptr;
    }
    /**
     * Determines if the TOML element is an array of "leaf" elements.
     */
    virtual bool is_array() const
    {
        return false;
    }

    /**
     * Converts the TOML element to an array.
     */
    std::shared_ptr<array> as_array()
    {
        if (is_array())
            return std::static_pointer_cast<array>(shared_from_this());
        return nullptr;
    }

    /**
     * Determines if the given TOML element is an array of tables.
     */
    virtual bool is_table_array() const
    {
        return false;
    }

    /**
     * Converts the TOML element into a table array.
     */
    std::shared_ptr<table_array> as_table_array()
    {
        if (is_table_array())
            return std::static_pointer_cast<table_array>(shared_from_this());
        return nullptr;
    }

    /**
     * Attempts to coerce the TOML element into a concrete TOML value
     * of type T.
     */
    template <class T>
    std::shared_ptr<value<T>> as();

    template <class T>
    std::shared_ptr<const value<T>> as() const;

    template <class Visitor, class... Args>
    void accept(Visitor&& visitor, Args&&... args) const;

  protected:
    base()
    {
        // nothing
    }
};

/**
 * A concrete TOML value representing the "leaves" of the "tree".
 */
template <class T>
class value : public base
{
    struct make_shared_enabler
    {
        // nothing; this is a private key accessible only to friends
    };

    template <class U>
    friend std::shared_ptr<typename value_traits<U>::type>
    cpptomlng::make_value(U&& val);

  public:
    static_assert(valid_value<T>::value, "invalid value type");

    std::shared_ptr<base> clone() const override;

    value(const make_shared_enabler&, const T& val) : value(val)
    {
        // nothing; note that users cannot actually invoke this function
        // because they lack access to the make_shared_enabler.
    }

    bool is_value() const override
    {
        return true;
    }

    /**
     * Gets the data associated with this value.
     */
    T& get()
    {
        return data_;
    }

    /**
     * Gets the data associated with this value. Const version.
     */
    const T& get() const
    {
        return data_;
    }

  private:
    T data_;

    /**
     * Constructs a value from the given data.
     */
    value(const T& val) : data_(val)
    {
    }

    value(const value& val) = delete;
    value& operator=(const value& val) = delete;
};

template <class T>
std::shared_ptr<typename value_traits<T>::type> make_value(T&& val)
{
    using value_type = typename value_traits<T>::type;
    using enabler = typename value_type::make_shared_enabler;
    return std::make_shared<value_type>(
        enabler{}, value_traits<T>::construct(std::forward<T>(val)));
}

template <class T>
inline std::shared_ptr<value<T>> base::as()
{
    return std::dynamic_pointer_cast<value<T>>(shared_from_this());
}

// special case value<double> to allow getting an integer parameter as a
// double value
template <>
inline std::shared_ptr<value<double>> base::as()
{
    if (auto v = std::dynamic_pointer_cast<value<double>>(shared_from_this()))
        return v;

    if (auto v = std::dynamic_pointer_cast<value<int64_t>>(shared_from_this()))
        return make_value<double>(static_cast<double>(v->get()));

    return nullptr;
}

template <class T>
inline std::shared_ptr<const value<T>> base::as() const
{
    return std::dynamic_pointer_cast<const value<T>>(shared_from_this());
}

// special case value<double> to allow getting an integer parameter as a
// double value
template <>
inline std::shared_ptr<const value<double>> base::as() const
{
    if (auto v
        = std::dynamic_pointer_cast<const value<double>>(shared_from_this()))
        return v;

    if (auto v = as<int64_t>())
    {
        // the below has to be a non-const value<double> due to a bug in
        // libc++: https://llvm.org/bugs/show_bug.cgi?id=18843
        return make_value<double>(static_cast<double>(v->get()));
    }

    return nullptr;
}

/**
 * Exception class for array insertion errors.
 */
class array_exception : public std::runtime_error
{
  public:
    array_exception(const std::string& err) : std::runtime_error{err}
    {
    }
};

class array : public base
{
  public:
    friend std::shared_ptr<array> make_array();

    std::shared_ptr<base> clone() const override;

    virtual bool is_array() const override
    {
        return true;
    }

    using size_type = std::size_t;

    /**
     * arrays can be iterated over
     */
    using iterator = std::vector<std::shared_ptr<base>>::iterator;

    /**
     * arrays can be iterated over.  Const version.
     */
    using const_iterator = std::vector<std::shared_ptr<base>>::const_iterator;

    iterator begin()
    {
        return values_.begin();
    }

    const_iterator begin() const
    {
        return values_.begin();
    }

    iterator end()
    {
        return values_.end();
    }

    const_iterator end() const
    {
        return values_.end();
    }

    /**
     * Obtains the array (vector) of base values.
     */
    std::vector<std::shared_ptr<base>>& get()
    {
        return values_;
    }

    /**
     * Obtains the array (vector) of base values. Const version.
     */
    const std::vector<std::shared_ptr<base>>& get() const
    {
        return values_;
    }

    std::shared_ptr<base> at(size_t idx) const
    {
        return values_.at(idx);
    }

    /**
     * Obtains an array of value<T>s. Note that elements may be
     * nullptr if they cannot be converted to a value<T>.
     */
    template <class T>
    std::vector<std::shared_ptr<value<T>>> array_of() const
    {
        std::vector<std::shared_ptr<value<T>>> result(values_.size());

        std::transform(values_.begin(), values_.end(), result.begin(),
                       [&](std::shared_ptr<base> v) { return v->as<T>(); });

        return result;
    }

    /**
     * Obtains a option<vector<T>>. The option will be empty if the array
     * contains values that are not of type T.
     */
    template <class T>
    inline typename array_of_trait<T>::return_type get_array_of() const
    {
        std::vector<T> result;
        result.reserve(values_.size());

        for (const auto& val : values_)
        {
            if (auto v = val->as<T>())
                result.push_back(v->get());
            else
                return {};
        }

        return {std::move(result)};
    }

    /**
     * Obtains an array of arrays. Note that elements may be nullptr
     * if they cannot be converted to a array.
     */
    std::vector<std::shared_ptr<array>> nested_array() const;

    /**
     * Add a value to the end of the array
     */
    template <class T>
    void push_back(const std::shared_ptr<value<T>>& val)
    {
        if (values_.empty() || values_[0]->as<T>())
        {
            values_.push_back(val);
        }
        else
        {
            throw array_exception{"Arrays must be homogenous."};
        }
    }

    /**
     * Add an array to the end of the array
     */
    void push_back(const std::shared_ptr<array>& val);

    /**
     * Convenience function for adding a simple element to the end
     * of the array.
     */
    template <class T>
    void push_back(T&& val, typename value_traits<T>::type* = 0)
    {
        push_back(make_value(std::forward<T>(val)));
    }

    /**
     * Insert a value into the array
     */
    template <class T>
    iterator insert(iterator position, const std::shared_ptr<value<T>>& value)
    {
        if (values_.empty() || values_[0]->as<T>())
        {
            return values_.insert(position, value);
        }
        else
        {
            throw array_exception{"Arrays must be homogenous."};
        }
    }

    /**
     * Insert an array into the array
     */
    iterator insert(iterator position, const std::shared_ptr<array>& value);

    /**
     * Convenience function for inserting a simple element in the array
     */
    template <class T>
    iterator insert(iterator position, T&& val,
                    typename value_traits<T>::type* = 0)
    {
        return insert(position, make_value(std::forward<T>(val)));
    }

    /**
     * Erase an element from the array
     */
    iterator erase(iterator position)
    {
        return values_.erase(position);
    }

    /**
     * Clear the array
     */
    void clear()
    {
        values_.clear();
    }

    /**
     * Reserve space for n values.
     */
    void reserve(size_type n)
    {
        values_.reserve(n);
    }

    /**
     * Get the length of the array.
     */
    size_t size()
    {
        return values_.size();
    }

  private:
    array() = default;

    template <class InputIterator>
    array(InputIterator begin, InputIterator end) : values_{begin, end}
    {
        // nothing
    }

    array(const array& obj) = delete;
    array& operator=(const array& obj) = delete;

    std::vector<std::shared_ptr<base>> values_;
};

std::shared_ptr<array> make_array();

namespace detail
{
template <>
inline std::shared_ptr<array> make_element<array>()
{
    return make_array();
}
} // namespace detail

/**
 * Obtains a option<vector<T>>. The option will be empty if the array
 * contains values that are not of type T.
 */
template <>
inline typename array_of_trait<array>::return_type
array::get_array_of<array>() const
{
    std::vector<std::shared_ptr<array>> result;
    result.reserve(values_.size());

    for (const auto& val : values_)
    {
        if (auto v = val->as_array())
            result.push_back(v);
        else
            return {};
    }

    return {std::move(result)};
}

class table;

class table_array : public base
{
    friend class table;
    friend std::shared_ptr<table_array> make_table_array(bool);

  public:
    std::shared_ptr<base> clone() const override;

    using size_type = std::size_t;

    /**
     * arrays can be iterated over
     */
    using iterator = std::vector<std::shared_ptr<table>>::iterator;

    /**
     * arrays can be iterated over.  Const version.
     */
    using const_iterator = std::vector<std::shared_ptr<table>>::const_iterator;

    iterator begin()
    {
        return array_.begin();
    }

    const_iterator begin() const
    {
        return array_.begin();
    }

    iterator end()
    {
        return array_.end();
    }

    const_iterator end() const
    {
        return array_.end();
    }

    virtual bool is_table_array() const override
    {
        return true;
    }

    std::vector<std::shared_ptr<table>>& get()
    {
        return array_;
    }

    const std::vector<std::shared_ptr<table>>& get() const
    {
        return array_;
    }

    /**
     * Add a table to the end of the array
     */
    void push_back(const std::shared_ptr<table>& val)
    {
        array_.push_back(val);
    }

    /**
     * Insert a table into the array
     */
    iterator insert(iterator position, const std::shared_ptr<table>& value)
    {
        return array_.insert(position, value);
    }

    /**
     * Erase an element from the array
     */
    iterator erase(iterator position)
    {
        return array_.erase(position);
    }

    /**
     * Clear the array
     */
    void clear()
    {
        array_.clear();
    }

    /**
     * Reserve space for n tables.
     */
    void reserve(size_type n)
    {
        array_.reserve(n);
    }

    /**
     * Get the length of the array.
     */
    size_t size()
    {
        return array_.size();
    }

    /**
     * Whether or not the table array is declared inline. This mostly
     * matters for parsing, where statically defined arrays cannot be
     * appended to using the array-of-table syntax.
     */
    bool is_inline() const
    {
        return is_inline_;
    }

  private:
    table_array(bool is_inline = false) : is_inline_(is_inline)
    {
        // nothing
    }

    table_array(const table_array& obj) = delete;
    table_array& operator=(const table_array& rhs) = delete;

    std::vector<std::shared_ptr<table>> array_;
    const bool is_inline_ = false;
};

std::shared_ptr<table_array> make_table_array(bool is_inline);

namespace detail
{
template <>
inline std::shared_ptr<table_array> make_element<table_array>()
{
    return make_table_array(true);
}
} // namespace detail

// The below are overloads for fetching specific value types out of a value
// where special casting behavior (like bounds checking) is desired

template <class T>
typename std::enable_if<!std::is_floating_point<T>::value
                            && std::is_signed<T>::value,
                        option<T>>::type
get_impl(const std::shared_ptr<base>& elem)
{
    if (auto v = elem->as<int64_t>())
    {
        if (v->get() < (std::numeric_limits<T>::min)())
            throw std::underflow_error{
                "T cannot represent the value requested in get"};

        if (v->get() > (std::numeric_limits<T>::max)())
            throw std::overflow_error{
                "T cannot represent the value requested in get"};

        return {static_cast<T>(v->get())};
    }
    else
    {
        return {};
    }
}

template <class T>
typename std::enable_if<!std::is_same<T, bool>::value
                            && std::is_unsigned<T>::value,
                        option<T>>::type
get_impl(const std::shared_ptr<base>& elem)
{
    if (auto v = elem->as<int64_t>())
    {
        if (v->get() < 0)
            throw std::underflow_error{"T cannot store negative value in get"};

        if (static_cast<uint64_t>(v->get()) > (std::numeric_limits<T>::max)())
            throw std::overflow_error{
                "T cannot represent the value requested in get"};

        return {static_cast<T>(v->get())};
    }
    else
    {
        return {};
    }
}

template <class T>
typename std::enable_if<!std::is_integral<T>::value
                            || std::is_same<T, bool>::value,
                        option<T>>::type
get_impl(const std::shared_ptr<base>& elem)
{
    if (auto v = elem->as<T>())
    {
        return {v->get()};
    }
    else
    {
        return {};
    }
}

/**
 * Represents a TOML keytable.
 */
class table : public base
{
  public:
    friend class table_array;
    friend std::shared_ptr<table> make_table();

    std::shared_ptr<base> clone() const override;

    /**
     * tables can be iterated over.
     */
    using iterator = string_to_base_map::iterator;

    /**
     * tables can be iterated over. Const version.
     */
    using const_iterator = string_to_base_map::const_iterator;

    iterator begin()
    {
        return map_.begin();
    }

    const_iterator begin() const
    {
        return map_.begin();
    }

    iterator end()
    {
        return map_.end();
    }

    const_iterator end() const
    {
        return map_.end();
    }

    bool is_table() const override
    {
        return true;
    }

    bool empty() const
    {
        return map_.empty();
    }

    /**
     * Determines if this key table contains the given key.
     */
    bool contains(const std::string& key) const
    {
        return map_.find(key) != map_.end();
    }

    /**
     * Determines if this key table contains the given key. Will
     * resolve "qualified keys". Qualified keys are the full access
     * path separated with dots like "grandparent.parent.child".
     */
    bool contains_qualified(const std::string& key) const
    {
        return resolve_qualified(key);
    }

    /**
     * Obtains the base for a given key.
     * @throw std::out_of_range if the key does not exist
     */
    std::shared_ptr<base> get(const std::string& key) const
    {
        return map_.at(key);
    }

    /**
     * Obtains the base for a given key. Will resolve "qualified
     * keys". Qualified keys are the full access path separated with
     * dots like "grandparent.parent.child".
     *
     * @throw std::out_of_range if the key does not exist
     */
    std::shared_ptr<base> get_qualified(const std::string& key) const
    {
        std::shared_ptr<base> p;
        resolve_qualified(key, &p);
        return p;
    }

    /**
     * Obtains a table for a given key, if possible.
     */
    std::shared_ptr<table> get_table(const std::string& key) const
    {
        if (contains(key) && get(key)->is_table())
            return std::static_pointer_cast<table>(get(key));
        return nullptr;
    }

    /**
     * Obtains a table for a given key, if possible. Will resolve
     * "qualified keys".
     */
    std::shared_ptr<table> get_table_qualified(const std::string& key) const
    {
        if (contains_qualified(key) && get_qualified(key)->is_table())
            return std::static_pointer_cast<table>(get_qualified(key));
        return nullptr;
    }

    /**
     * Obtains an array for a given key.
     */
    std::shared_ptr<array> get_array(const std::string& key) const
    {
        if (!contains(key))
            return nullptr;
        return get(key)->as_array();
    }

    /**
     * Obtains an array for a given key. Will resolve "qualified keys".
     */
    std::shared_ptr<array> get_array_qualified(const std::string& key) const
    {
        if (!contains_qualified(key))
            return nullptr;
        return get_qualified(key)->as_array();
    }

    /**
     * Obtains a table_array for a given key, if possible.
     */
    std::shared_ptr<table_array> get_table_array(const std::string& key) const
    {
        if (!contains(key))
            return nullptr;
        return get(key)->as_table_array();
    }

    /**
     * Obtains a table_array for a given key, if possible. Will resolve
     * "qualified keys".
     */
    std::shared_ptr<table_array>
    get_table_array_qualified(const std::string& key) const
    {
        if (!contains_qualified(key))
            return nullptr;
        return get_qualified(key)->as_table_array();
    }

    /**
     * Helper function that attempts to get a value corresponding
     * to the template parameter from a given key.
     */
    template <class T>
    option<T> get_as(const std::string& key) const
    {
        try
        {
            return get_impl<T>(get(key));
        }
        catch (const std::out_of_range&)
        {
            return {};
        }
    }

    /**
     * Helper function that attempts to get a value corresponding
     * to the template parameter from a given key. Will resolve "qualified
     * keys".
     */
    template <class T>
    option<T> get_qualified_as(const std::string& key) const
    {
        try
        {
            return get_impl<T>(get_qualified(key));
        }
        catch (const std::out_of_range&)
        {
            return {};
        }
    }

    /**
     * Helper function that attempts to get an array of values of a given
     * type corresponding to the template parameter for a given key.
     *
     * If the key doesn't exist, doesn't exist as an array type, or one or
     * more keys inside the array type are not of type T, an empty option
     * is returned. Otherwise, an option containing a vector of the values
     * is returned.
     */
    template <class T>
    inline typename array_of_trait<T>::return_type
    get_array_of(const std::string& key) const
    {
        if (auto v = get_array(key))
        {
            std::vector<T> result;
            result.reserve(v->get().size());

            for (const auto& b : v->get())
            {
                if (auto val = b->as<T>())
                    result.push_back(val->get());
                else
                    return {};
            }
            return {std::move(result)};
        }

        return {};
    }

    /**
     * Helper function that attempts to get an array of values of a given
     * type corresponding to the template parameter for a given key. Will
     * resolve "qualified keys".
     *
     * If the key doesn't exist, doesn't exist as an array type, or one or
     * more keys inside the array type are not of type T, an empty option
     * is returned. Otherwise, an option containing a vector of the values
     * is returned.
     */
    template <class T>
    inline typename array_of_trait<T>::return_type
    get_qualified_array_of(const std::string& key) const
    {
        if (auto v = get_array_qualified(key))
        {
            std::vector<T> result;
            result.reserve(v->get().size());

            for (const auto& b : v->get())
            {
                if (auto val = b->as<T>())
                    result.push_back(val->get());
                else
                    return {};
            }
            return {std::move(result)};
        }

        return {};
    }

    /**
     * Adds an element to the keytable.
     */
    void insert(const std::string& key, const std::shared_ptr<base>& value)
    {
        map_[key] = value;
    }

    /**
     * Convenience shorthand for adding a simple element to the
     * keytable.
     */
    template <class T>
    void insert(const std::string& key, T&& val,
                typename value_traits<T>::type* = 0)
    {
        insert(key, make_value(std::forward<T>(val)));
    }

    /**
     * Removes an element from the table.
     */
    void erase(const std::string& key)
    {
        map_.erase(key);
    }

    /**
     * Get the number of entries in the table.
     */
    size_t size()
    {
        return map_.size();
    }

  private:
    table()
    {
        // nothing
    }

    table(const table& obj) = delete;
    table& operator=(const table& rhs) = delete;

    std::vector<std::string> split(const std::string& value,
                                   char separator) const;

    // If output parameter p is specified, fill it with the pointer to the
    // specified entry and throw std::out_of_range if it couldn't be found.
    //
    // Otherwise, just return true if the entry could be found or false
    // otherwise and do not throw.
    bool resolve_qualified(const std::string& key,
                           std::shared_ptr<base>* p = nullptr) const;

    string_to_base_map map_;
};

/**
 * Helper function that attempts to get an array of arrays for a given
 * key.
 *
 * If the key doesn't exist, doesn't exist as an array type, or one or
 * more keys inside the array type are not of type T, an empty option
 * is returned. Otherwise, an option containing a vector of the values
 * is returned.
 */
template <>
inline typename array_of_trait<array>::return_type
table::get_array_of<array>(const std::string& key) const
{
    if (auto v = get_array(key))
    {
        std::vector<std::shared_ptr<array>> result;
        result.reserve(v->get().size());

        for (const auto& b : v->get())
        {
            if (auto val = b->as_array())
                result.push_back(val);
            else
                return {};
        }

        return {std::move(result)};
    }

    return {};
}

/**
 * Helper function that attempts to get an array of arrays for a given
 * key. Will resolve "qualified keys".
 *
 * If the key doesn't exist, doesn't exist as an array type, or one or
 * more keys inside the array type are not of type T, an empty option
 * is returned. Otherwise, an option containing a vector of the values
 * is returned.
 */
template <>
inline typename array_of_trait<array>::return_type
table::get_qualified_array_of<array>(const std::string& key) const
{
    if (auto v = get_array_qualified(key))
    {
        std::vector<std::shared_ptr<array>> result;
        result.reserve(v->get().size());

        for (const auto& b : v->get())
        {
            if (auto val = b->as_array())
                result.push_back(val);
            else
                return {};
        }

        return {std::move(result)};
    }

    return {};
}

std::shared_ptr<table> make_table();

namespace detail
{
template <>
inline std::shared_ptr<table> make_element<table>()
{
    return make_table();
}
} // namespace detail

template <class T>
std::shared_ptr<base> value<T>::clone() const
{
    return make_value(data_);
}

inline std::shared_ptr<base> array::clone() const
{
    auto result = make_array();
    result->reserve(values_.size());
    for (const auto& ptr : values_)
        result->values_.push_back(ptr->clone());
    return result;
}

inline std::shared_ptr<base> table_array::clone() const
{
    auto result = make_table_array(is_inline());
    result->reserve(array_.size());
    for (const auto& ptr : array_)
        result->array_.push_back(ptr->clone()->as_table());
    return result;
}

inline std::shared_ptr<base> table::clone() const
{
    auto result = make_table();
    for (const auto& pr : map_)
        result->insert(pr.first, pr.second->clone());
    return result;
}

/**
 * Exception class for all TOML parsing errors.
 */
class parse_exception : public std::runtime_error
{
  public:
    parse_exception(const std::string& err) : std::runtime_error{err}
    {
    }

    parse_exception(const std::string& err, std::size_t line_number)
        : std::runtime_error{err + " at line " + std::to_string(line_number)}
    {
    }
};

/**
 * The parser class.
 */
class parser
{
  public:
    /**
     * Parsers are constructed from streams.
     */
    parser(std::istream& stream) : input_(stream)
    {
        // nothing
    }

    parser& operator=(const parser& parser) = delete;

    /**
     * Parses the stream this parser was created on until EOF.
     * @throw parse_exception if there are errors in parsing
     */
    std::shared_ptr<table> parse();

  private:
#if defined _MSC_VER
    __declspec(noreturn)
#elif defined __GNUC__
    __attribute__((noreturn))
#endif
    void throw_parse_exception(const std::string& err)
    {
        throw parse_exception{err, line_number_};
    }

    void parse_table(std::string::iterator& it,
                     const std::string::iterator& end, table*& curr_table);

    void parse_single_table(std::string::iterator& it,
                            const std::string::iterator& end,
                            table*& curr_table);

    void parse_table_array(std::string::iterator& it,
                           const std::string::iterator& end, table*& curr_table);

    void parse_key_value(std::string::iterator& it, std::string::iterator& end,
                         table* curr_table);

    template <class KeyEndFinder, class KeyPartHandler>
    std::string
    parse_key(std::string::iterator& it, const std::string::iterator& end,
              KeyEndFinder&& key_end, KeyPartHandler&& key_part_handler);

    std::string parse_simple_key(std::string::iterator& it,
                                 const std::string::iterator& end);

    std::string parse_bare_key(std::string::iterator& it,
                               const std::string::iterator& end);

    enum class parse_type
    {
        STRING = 1,
        LOCAL_TIME,
        LOCAL_DATE,
        LOCAL_DATETIME,
        OFFSET_DATETIME,
        INT,
        FLOAT,
        BOOL,
        ARRAY,
        INLINE_TABLE
    };

    std::shared_ptr<base> parse_value(std::string::iterator& it,
                                      std::string::iterator& end);

    parse_type determine_value_type(const std::string::iterator& it,
                                    const std::string::iterator& end);

    parse_type determine_number_type(const std::string::iterator& it,
                                     const std::string::iterator& end);

    std::shared_ptr<value<std::string>> parse_string(std::string::iterator& it,
                                                     std::string::iterator& end);

    std::shared_ptr<value<std::string>>
    parse_multiline_string(std::string::iterator& it,
                           std::string::iterator& end, char delim);

    std::string string_literal(std::string::iterator& it,
                               const std::string::iterator& end, char delim);

    std::string parse_escape_code(std::string::iterator& it,
                                  const std::string::iterator& end);

    std::string parse_unicode(std::string::iterator& it,
                              const std::string::iterator& end);

    uint32_t parse_hex(std::string::iterator& it,
                       const std::string::iterator& end, uint32_t place);

    uint32_t hex_to_digit(char c);

    std::shared_ptr<base> parse_number(std::string::iterator& it,
                                       const std::string::iterator& end);

    std::shared_ptr<value<int64_t>> parse_int(std::string::iterator& it,
                                              const std::string::iterator& end,
                                              int base = 10,
                                              const char* prefix = "");

    std::shared_ptr<value<double>> parse_float(std::string::iterator& it,
                                               const std::string::iterator& end);

    std::shared_ptr<value<bool>> parse_bool(std::string::iterator& it,
                                            const std::string::iterator& end);

    std::string::iterator find_end_of_number(std::string::iterator it,
                                             std::string::iterator end);

    std::string::iterator find_end_of_date(std::string::iterator it,
                                           std::string::iterator end);

    std::string::iterator find_end_of_time(std::string::iterator it,
                                           std::string::iterator end);

    local_time read_time(std::string::iterator& it,
                         const std::string::iterator& end);

    std::shared_ptr<value<local_time>>
    parse_time(std::string::iterator& it, const std::string::iterator& end);

    std::shared_ptr<base> parse_date(std::string::iterator& it,
                                     const std::string::iterator& end);

    std::shared_ptr<base> parse_array(std::string::iterator& it,
                                      std::string::iterator& end);

    template <class Value>
    std::shared_ptr<array> parse_value_array(std::string::iterator& it,
                                             std::string::iterator& end);

    template <class Object, class Function>
    std::shared_ptr<Object> parse_object_array(Function&& fun, char delim,
                                               std::string::iterator& it,
                                               std::string::iterator& end);

    std::shared_ptr<table> parse_inline_table(std::string::iterator& it,
                                              std::string::iterator& end);

    void skip_whitespace_and_comments(std::string::iterator& start,
                                      std::string::iterator& end);

    void consume_whitespace(std::string::iterator& it,
                            const std::string::iterator& end);

    void consume_backwards_whitespace(std::string::iterator& back,
                                      const std::string::iterator& front);

    void eol_or_comment(const std::string::iterator& it,
                        const std::string::iterator& end);

    bool is_time(const std::string::iterator& it,
                 const std::string::iterator& end);

    option<parse_type> date_type(const std::string::iterator& it,
                                 const std::string::iterator& end);

    std::istream& input_;
    std::string line_;
    std::size_t line_number_ = 0;
};

/**
 * Utility function to parse a file as a TOML file. Returns the root table.
 * Throws a parse_exception if the file cannot be opened.
 */
std::shared_ptr<table> parse_file(const std::string& filename);

template <class... Ts>
struct value_accept;

template <>
struct value_accept<>
{
    template <class Visitor, class... Args>
    static void accept(const base&, Visitor&&, Args&&...)
    {
        // nothing
    }
};

template <class T, class... Ts>
struct value_accept<T, Ts...>
{
    template <class Visitor, class... Args>
    static void accept(const base& b, Visitor&& visitor, Args&&... args)
    {
        if (auto v = b.as<T>())
        {
            visitor.visit(*v, std::forward<Args>(args)...);
        }
        else
        {
            value_accept<Ts...>::accept(b, std::forward<Visitor>(visitor),
                                        std::forward<Args>(args)...);
        }
    }
};

/**
 * base implementation of accept() that calls visitor.visit() on the concrete
 * class.
 */
template <class Visitor, class... Args>
void base::accept(Visitor&& visitor, Args&&... args) const
{
    if (is_value())
    {
        using value_acceptor
            = value_accept<std::string, int64_t, double, bool, local_date,
                           local_time, local_datetime, offset_datetime>;
        value_acceptor::accept(*this, std::forward<Visitor>(visitor),
                               std::forward<Args>(args)...);
    }
    else if (is_table())
    {
        visitor.visit(static_cast<const table&>(*this),
                      std::forward<Args>(args)...);
    }
    else if (is_array())
    {
        visitor.visit(static_cast<const array&>(*this),
                      std::forward<Args>(args)...);
    }
    else if (is_table_array())
    {
        visitor.visit(static_cast<const table_array&>(*this),
                      std::forward<Args>(args)...);
    }
}

/**
 * Writer that can be passed to accept() functions of cpptoml objects and
 * will output valid TOML to a stream.
 */
class toml_writer
{
  public:
    /**
     * Construct a toml_writer that will write to the given stream
     */
    toml_writer(std::ostream& s, const std::string& indent_space = "\t")
        : stream_(s), indent_(indent_space), has_naked_endline_(false)
    {
        // nothing
    }

  public:
    /**
     * Output a base value of the TOML tree.
     */
    template <class T>
    void visit(const value<T>& v, bool = false)
    {
        write(v);
    }

    /**
     * Output a table element of the TOML tree
     */
    void visit(const table& t, bool in_array = false);

    /**
     * Output an array element of the TOML tree
     */
    void visit(const array& a, bool = false);

    /**
     * Output a table_array element of the TOML tree
     */
    void visit(const table_array& t, bool = false);

    /**
     * Escape a string for output.
     */
    static std::string escape_string(const std::string& str);

  protected:
    /**
     * Write out a string.
     */
    void write(const value<std::string>& v);

    /**
     * Write out a double.
     */
    void write(const value<double>& v);

    /**
     * Write out an integer, local_date, local_time, local_datetime, or
     * offset_datetime.
     */
    template <class T>
    typename std::enable_if<
        is_one_of<T, int64_t, local_date, local_time, local_datetime,
                  offset_datetime>::value>::type
    write(const value<T>& v)
    {
        write(v.get());
    }

    /**
     * Write out a boolean.
     */
    void write(const value<bool>& v);

    /**
     * Write out the header of a table.
     */
    void write_table_header(bool in_array = false);

    /**
     * Write out the identifier for an item in a table.
     */
    void write_table_item_header(const base& b);

  private:
    /**
     * Indent the proper number of tabs given the size of
     * the path.
     */
    void indent();

    /**
     * Write a value out to the stream.
     */
    void write(char ch);

    template <class T>
    void write(const T& v)
    {
        stream_ << v;
        has_naked_endline_ = false;
    }

    /**
     * Write an endline out to the stream
     */
    void endline();

  private:
    std::ostream& stream_;
    const std::string indent_;
    std::vector<std::string> path_;
    bool has_naked_endline_;
};

inline std::ostream& operator<<(std::ostream& stream, const base& b)
{
    toml_writer writer{stream};
    b.accept(writer);
    return stream;
}

template <class T>
std::ostream& operator<<(std::ostream& stream, const value<T>& v)
{
    toml_writer writer{stream};
    v.accept(writer);
    return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const table& t)
{
    toml_writer writer{stream};
    t.accept(writer);
    return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const table_array& t)
{
    toml_writer writer{stream};
    t.accept(writer);
    return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const array& a)
{
    toml_writer writer{stream};
    a.accept(writer);
    return stream;
}
} // namespace cpptomlng

#ifndef CPPTOMLNG_NO_ALIAS
namespace cpptoml = cpptomlng;
#endif

#endif // CPPTOMLNG_H
