import std;

namespace fs = std::filesystem;
using namespace std::string_literals;
using namespace std::string_view_literals;

static bool check_dep_file_updated(std::string const& dep_file_path, fs::file_time_type obj_time) {
    auto file = std::ifstream(dep_file_path);
    if (not file) return false;

    for (auto path = std::string(); file >> path;) {
        while (not path.empty() and (path.front() == '"' or path.front() == '{')) path.erase(0, 1);
        while (not path.empty() and (path.back() == '"' or path.back() == ',' or path.back() == '}' or path.back() == ']')) path.pop_back();

        if (path.empty() or path == "\\" or path.ends_with(':')) continue;

        if (fs::exists(path)) {
            if (fs::last_write_time(path) > obj_time) return true;
        }
    }

    return false;
}

static bool needs_recompile(std::string_view src_path, std::string const& obj_path) {
    if (not fs::exists(obj_path)) return true;

    auto const obj_time = fs::last_write_time(obj_path);
    auto const src_time = fs::last_write_time(src_path);

    if (src_time > obj_time) return true;

    auto dep_path_d = obj_path + ".d"s;
    auto dep_path_json = obj_path + ".json"s;

    if (fs::exists(dep_path_d)) return check_dep_file_updated(dep_path_d, obj_time);
    else if (fs::exists(dep_path_json)) return check_dep_file_updated(dep_path_json, obj_time);

    return false;
}

static auto read_lines(fs::path const& path) {
    auto file = std::ifstream(path);
    if (not file) return std::vector<std::string>();

    auto lines = std::vector<std::string>();
    for (auto line = ""s; std::getline(file, line);) {
        if (not line.empty()) lines.push_back(std::move(line));
    }
    return lines;
}

static auto wildcard_to_regex(std::string_view pattern) {
    auto regex_str = "^"s;
    for (char c : pattern) {
        switch (c) {
        case '*': regex_str += ".*"s; break;
        case '?': regex_str += "."s; break;
        case '.':
        case '\\': case '+': case '^': case '$':
        case '(': case ')': case '[': case ']':
        case '{': case '}': case '|':
            regex_str += '\\';
            regex_str += c;
            break;
        default: regex_str += c; break;
        }
    }
    regex_str += "$"s;
    return std::regex(regex_str, std::regex::icase);
}

static auto parse_tree_paths(std::vector<std::string> const& input, int indent_size = 4) {
    auto result = std::vector<std::string>();
    auto path_stack = std::vector<std::string>();

    for (auto const& line : input) {
        auto first_char = line.find_first_not_of(" \t");
        if (first_char == std::string::npos) continue;

        auto spaces = 0;
        for (auto i = 0ULL; i < first_char; ++i) {
            if (line[i] == '\t') spaces += indent_size;
            else spaces += 1;
        }
        auto depth = spaces / indent_size;

        auto last_char = line.find_last_not_of("\r\n"sv);
        auto item = line.substr(first_char, last_char - first_char + 1);

        if (depth < path_stack.size()) path_stack.resize(depth);

        if (item.back() == '/') path_stack.push_back(item);
        else {
            auto full_path = ""s;
            for (const auto& segment : path_stack) full_path += segment;

            if (item.find_first_of("*?"sv) != std::string::npos) {
                auto dir_path = full_path.empty() ? "." : full_path;

                if (fs::exists(dir_path) and fs::is_directory(dir_path)) {
                    auto pattern = wildcard_to_regex(item);

                    for (const auto& entry : fs::directory_iterator(dir_path)) {
                        auto filename = entry.path().filename().string();

                        if (std::regex_match(filename, pattern)) {
                            result.push_back(full_path + filename);
                        }
                    }
                }
            }
            else result.push_back(full_path + item);
        }
    }

    return result;
}

template <typename... Args>
static auto dyn_format(std::string_view fmt, Args&&... args) {
    try {
        return std::vformat(fmt, std::make_format_args(args...));
    }
    catch (const std::format_error&) {
		std::println("\033[91m[cman] \033[31mError: '{}' does not take {} arguments\033[0m"sv, fmt, sizeof...(args));
        std::exit(1);
    }
}

std::unordered_map<std::string_view, int> const compile_type_map = {
    {"dynlnk"sv,  0},
    {"statlnk"sv, 1},
    {"exec"sv,    2}
};

std::unordered_map<std::string_view, int> const to_do_map = {
    {"help"sv,    0},
    {"version"sv, 1},
    {"clean"sv,   2},
    {"build"sv,   3}
};

#define CMAN_VERSION "26.0"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println("\033[91m[cman] \033[31mUsage: {} [ option ]\033[0m"sv, argv[0]);
        return 1;
    }

	if (not to_do_map.contains(argv[1])) {
        std::println("\033[91m[cman] \033[31mError: Unknown option '{}'\033[0m"sv, argv[1]);
		return 1;
	}

    switch (to_do_map.at(argv[1])) {
    case 0:
        std::println("\033[96m[cman] \033[36mUsage: {} [ option ]\033[0m\n"sv, argv[0]);
        std::println("\033[96m[cman] \033[36m---- Options ----\033[0m"sv);
        std::println("\033[96m[cman] \033[36mdynlnk\033[90m  # Compile and link as a dynamic library (dll/so)\033[0m"sv);
        std::println("\033[96m[cman] \033[36mstatlnk\033[90m # Compile and link as a static library (lib/a)\033[0m"sv);
        std::println("\033[96m[cman] \033[36mnormal\033[90m  # Compile and link as an executable\033[0m"sv);
        break;
    
    case 1: std::println("\033[96m[cman] \033[36mVersion: " CMAN_VERSION "\033[0m"sv); break;
    case 2: fs::remove_all("out"); break;
    
    case 3: {
        if (argc < 5) {
            std::println("\033[91m[cman] \033[31mUsage: {} build [ compile_type ] [ dependencies ] [ compiler ]\033[0m"sv, argv[0]);
            return 1;
        }

		if (not compile_type_map.contains(argv[2])) {
            std::println("\033[91m[cman] \033[31mError: Unknown Compile Type '{}'\033[0m"sv, argv[2]);
			return 1;
		}

        auto const deps_path = fs::path(argv[3]);
        auto const compiler_path = fs::path(argv[4]);

        auto const cdeps_lines = read_lines(deps_path);
        auto const compiler_cmds = read_lines(compiler_path);

        auto deps_paths = parse_tree_paths(cdeps_lines);

        /*
        * compiler_cmds is something like:
        *   <command compile cpp>
        *   <command compile cppm>
        *   <command to dynamic link (dll/so)>
        *   <command to static link (lib/a)>
        *   <command to add include path>
        *   <command to link executable>
        */

        auto cpp_files = std::vector<std::string>();
        auto cppm_files = std::vector<std::string>();
        auto static_lib_files = ""s;
        auto include_paths = std::vector<std::string>();

        auto scan_deps = [&](this auto&& self, std::vector<std::string>& deps_paths) -> void {
            for (auto& path : deps_paths) {
                if (path.ends_with(".cdeps"sv)) {
                    auto sub_deps = parse_tree_paths(read_lines(path));
                    self(sub_deps);
                }
                else if (path.ends_with(".lib"sv) or path.ends_with(".a"sv)) {
                    static_lib_files += std::format("\"{}\" "sv, path);
                }
                else if (path.ends_with(".inc"sv)) {
                    include_paths.emplace_back(std::move(path.erase(path.size() - 4)));
                }
                else if (path.ends_with(".cpp"sv) or path.ends_with(".cxx"sv)) {
                    cpp_files.emplace_back(std::move(path));
                }
                else cppm_files.emplace_back(std::move(path));
            }
        };

        scan_deps(deps_paths);

        auto include_flags = ""s;
        if (compiler_cmds.size() >= 5 and not include_paths.empty()) {
            for (auto const& inc : include_paths) {
                include_flags += std::format("{} "sv, dyn_format(compiler_cmds[4], inc));
            }
        }

        fs::create_directories("out/object"sv);
        auto obj_files = ""s;

        if (compiler_cmds.size() >= 2) {
            for (auto const& file : cppm_files) {
                auto const out_dir = fs::path(file);
                auto const out_path = std::format("out/object/{}.cobj "sv, std::hash<fs::path>{}(out_dir));

                if (needs_recompile(file, out_path)) {
                    auto cmd = std::format("{} {}"sv, dyn_format(compiler_cmds[1], file, out_path), include_flags);
                    std::println("\033[96m[cman] \033[36mCompiling Module: {}\033[0m"sv, file);
                    
                    fs::remove(out_path);
                    std::system(cmd.data());

                    if (not fs::exists(out_path)) {
                        std::println("\033[91m[cman] \033[31mError: Object file is not generated, compilation seems to have failed: {}\033[0m"sv, file);
                        return 1;
                    }
                }
                else std::println("\033[90m[cman] Up-to-date Module: {}\033[0m"sv, file);

                obj_files += out_path;
            }
        }

        {
            std::vector<std::future<std::string>> futures;
            auto print_mutex = std::mutex();

            auto print = [&print_mutex](std::string_view msg) {
                std::lock_guard lock(print_mutex);
                std::println("{}", msg);
            };

            if (compiler_cmds.size() >= 1) {
                futures.reserve(cpp_files.size());

                auto worker = [&compiler_cmds, &include_flags, &print](std::string_view file) {
                    auto const out_dir = fs::path(file);
                    auto const out_path = std::format("out/object/{}.cobj "sv, std::hash<fs::path>{}(out_dir));

                    if (needs_recompile(file, out_path)) {
                        print(std::format("\033[96m[cman] \033[36mCompiling Source: {}\033[0m"sv, file));
                        auto cmd = std::format("{} {}"sv, dyn_format(compiler_cmds[0], file, out_path), include_flags);
                        
						fs::remove(out_path);
                        std::system(cmd.data());

						if (not fs::exists(out_path)) {
							print(std::format("\033[91m[cman] \033[31mError: Object file is not generated, compilation seems to have failed: {}\033[0m"sv, file));
							std::exit(1);
						}
                    }
                    else print(std::format("\033[90m[cman] Up-to-date Source: {}\033[0m"sv, file));

                    return out_path;
                };

                for (auto const& file : cpp_files) {
                    futures.emplace_back(std::async(std::launch::async, worker, file));
                }
            }

            for (auto& f : futures) obj_files += f.get();
        }

        auto all_link_inputs = obj_files + static_lib_files;

        switch (compile_type_map.at(argv[2])) {
        case 0: {
            if (compiler_cmds.size() >= 6) {
                auto final_cmd = dyn_format(compiler_cmds[2], all_link_inputs, "out/program"sv);
                std::println("\033[96m[cman] \033[36mLinking Final Dynamic Library...\033[0m"sv);
                std::system(final_cmd.data());
            }

            break;
        }
        case 1: {
            if (compiler_cmds.size() >= 6) {
                auto final_cmd = dyn_format(compiler_cmds[3], all_link_inputs, "out/program"sv);
                std::println("\033[96m[cman] \033[36mLinking Final Static Library...\033[0m"sv);
                std::system(final_cmd.data());
            }

            break;
        }
        case 2: {
            if (compiler_cmds.size() >= 6) {
                auto final_cmd = dyn_format(compiler_cmds[5], all_link_inputs, "out/program"sv);
                std::println("\033[96m[cman] \033[36mLinking Final Executable...\033[0m"sv);
                std::system(final_cmd.data());
            }

            break;
        }
        }

        break;
    }
    }
}