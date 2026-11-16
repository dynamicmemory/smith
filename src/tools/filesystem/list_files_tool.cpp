/* List files and directories in the given path*/
std::string list_files_tool(const std::string &path) {
    namespace fs = std::filesystem;
    fs::path directory(path); 

    if (!fs::exists(directory))
        return "Path not found: " + path;

    if (!fs::is_directory(directory))
        return "Not a directory: " + path;

    std::vector<std::string> dirs;
    std::vector<std::string> files;
    
    for (const auto &item : fs::directory_iterator(directory)) {
        std::string filename = item.path().filename().string();

        // TODO: We are skipping hidden files for future refference
        if (filename.starts_with('.'))
            continue;

        if (item.is_directory())
            dirs.push_back(filename+"/");
        else 
            files.push_back(filename);
    }

    std::sort(dirs.begin(), dirs.end(), [](const auto &a, const auto &b) {
        return a < b; });
    std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) {
        return a < b; });

    std::string result = "Directory: " + path + "\n";
    for (const auto &item : dirs) 
        result += item + '\n';
    for (const auto &item : files) 
        result += item + '\n';

    return result;
