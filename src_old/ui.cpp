bool is_interactive = true;
inline void interactive_wait_for_any_key() {
    if (is_interactive) {
        cout << endl << "Press ENTER to exit..." << endl;
        cin.get();
    }
}

void DisplayGenerationActivity(const bool& added, std::string name, const int& folder_p, const int& folder_t, int progress = -1)
{
    // TERMINAL_WIDTH - "() " - STATUS - #P - #N - '/' - LASTCHR
    unsigned short shortn = TERMINAL_WIDTH - 3 - 4 - intDigits(folder_p) - intDigits(folder_t) - 1 - 2;

    cout << '(' << folder_p << '/' << folder_t << ") "
         << (name.size() >= shortn ? "..." + name.substr(name.size() - shortn + 3) : name + string(shortn - name.size(), ' '));

    if (progress != 100 && progress != -1)
        cout << ' ' << progress << " %";

    error_log.reset();
}




bool __gen_activity_added;
std::string __gen_activity_name;
int __gen_activity_folder_p, __gen_activity_folder_t;

std::chrono::high_resolution_clock::time_point __gen_activity_rst  = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point __gen_activity_last = std::chrono::high_resolution_clock::now();
std::chrono::duration<double> duration;

inline void UpdateGenerationActivity(int progress = -1, bool force = false)
{
    static std::chrono::high_resolution_clock::time_point now;
    now = std::chrono::high_resolution_clock::now();

    duration = now - __gen_activity_last;
    if (duration.count() > 0.5 || force)
    {
        cout << '\r';
        DisplayGenerationActivity(__gen_activity_added, __gen_activity_name, __gen_activity_folder_p, __gen_activity_folder_t, progress);
        __gen_activity_last = now;
    }
}

void SetGenerationActivityParameters(const bool& added, std::string name, const int& folder_p, const int& folder_t)
{
    __gen_activity_added    = added;
    __gen_activity_name     = name;
    __gen_activity_folder_p = folder_p;
    __gen_activity_folder_t = folder_t;
    __gen_activity_last     = __gen_activity_rst;
}
