#include <getopt.h>
#include <string>
#include <iostream>
#include <type_traits>
#include <vector>
#include <functional>
#include <map>
#include <unistd.h>

namespace pstack {

template <typename T> void convert(const char *opt, T &to) {
   if constexpr (std::is_integral_v<T>)
      if constexpr (std::is_signed_v<T>)
          to = T(strtoll(opt, nullptr, 0));
      else
          to = T(strtoull(opt, nullptr, 0));
   else if constexpr (std::is_floating_point_v<T>)
      to = strtod(opt, nullptr);
   else
      to = opt;
}

/*
 * A wrapper for getopt_long that correlates long and short options together,
 * and allows for a functional interface for handling options.
 * Create a Flags object, and call "add" on it repeatedly. You can chain-call
 * add invocations, and then finally invoke "parse".
 */
class Flags {
    // the long-form options. The "val" field of each is the short option name.
    std::vector<option> longOptions;

    using Cb = std::function<void(const char *)>; // callback for a flag with an argument
    using VCb = std::function<void()>; // callback for a flag without an argument.

    // Data for a specific flag - it's help text, the name for its metavar if
    // it takes an argument, a callback to invoke when it's encountered.
    struct Data {
        const char *helptext;
        const char *metavar;
        Cb callback;
    };
    std::map<int, Data> data; // per-flag data, indexed by short-form option.
    std::string shortOptions; // String for short-form options, calculated from longOptions + data.

    // next "val" to use for long a long option with no corresponding short
    // option. (Otherwise, short option is 'val'
    int longVal = -2;

public:
    static constexpr int LONGONLY = 0;
    /**
     * Add a flag to the set of parsed flags.
     * name: the long-form name
     * flag: the short-form, single character version. If there is no short-form, use LONGONLY.
     * metavar: if non-null, indicates this flag takes an argument, and provides
     *          a textual description of that argument in the help output.
     * help: descriptive text describing option
     * cb: callback invoked when the argument is encountered.
     */
    Flags & add(const char *name, int flag, const char *metavar, const char *help, Cb cb);

    /**
     * Add a flag to the set of parsed flags - shorter helper for flags that
     * take no arguments.
     */
    Flags & add(const char *name, int flag, const char *help, const VCb &cb) {
        return add(name, flag, nullptr, help, [cb](const char *) { cb(); });
    }

    /**
     * indicate we have added all arguments, and don't intend to modify the
     * flags further.
     */
    const Flags & done();

    /**
     * Dump information about invocation to passed stream
     */
    std::ostream & dump(std::ostream &os) const;

    // parse argc/argv, calling the correct callbacks. (non-const version will
    // call "done" before calling const version)
    const Flags & parse(int argc, char **argv) const;
    const Flags & parse(int argc, char **argv);

    // Helper to get a VCb to just set a bool to true/false.
    template <typename T> static VCb
    setf(T &val, T to=true) { return [&val, to] () { val = to; }; }

    // Helper to set a value from a string, using convert to convert from
    // string to whatever type is required.
    template <typename T> static Cb
    set(T &val) { return [&val] (const char *opt) { convert<T>(opt, val); }; }

    // Append to a container with push_back, with a converted value.
    template <typename T, typename C> static Cb
    append(C &val) { return [&val] (const char *opt) { val.push_back(convert<T>(opt)); }; }
};
}

// Stream a "Flags" to stdout - printing a formatted help text for the options.
inline std::ostream & operator << (std::ostream &os, const pstack::Flags &opts) { return opts.dump(os); }
