#pragma once

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <windows.h>
#include <psapi.h>
#include <intrin.h>
// żeby się te cosie od pamięci na windowsie automatycznie załączały
#ifdef _MSC_VER
    #pragma comment(lib, "Psapi.lib")
#endif

#include <omp.h>

#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <fstream>
#include <mutex>
#include <atomic>
#include <thread>



namespace tp {
    // MAP SIZE
    inline std::atomic<int>& get_map_w() {
        static std::atomic<int> v{0};
        return v;
    }

    inline std::atomic<int>& get_map_h() {
        static std::atomic<int> v{0};
        return v;
    }
    


    // TIME
    inline uint64_t now_usec() {
        return godot::Time::get_singleton()->get_ticks_usec();
    }

    // czasy funkcji
    struct TimeRecord {
        std::string caller;
        std::string function;
        uint64_t thread_id = 0;
        int threads_used = 0;

        uint64_t duration_us = 0;

        int map_width = 0;
        int map_height = 0;
    };



    // MEMORY
    // snapshoty pamięci procesu
    struct MemorySample {
        // aktualny working set (podatny na trimming OS)
        size_t working_set_bytes = 0;
        // szczyt working set od startu procesu
        size_t peak_working_set_bytes = 0;
        // committed private memory - najlepszy proxy wzrostu sterty
        size_t private_bytes = 0;
    };

    inline MemorySample sample_process_memory() {
        PROCESS_MEMORY_COUNTERS_EX pmc{};
        pmc.cb = sizeof(pmc);

        MemorySample s;
        if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
            s.working_set_bytes = pmc.WorkingSetSize;
            s.peak_working_set_bytes = pmc.PeakWorkingSetSize;
            s.private_bytes = pmc.PrivateUsage;
        }
        return s;
    }

    struct MemoryRecord {
        // np. "before_erode" / "after_erode"
        std::string label;
        int run = 0;

        // czas od startu próbkowania
        uint64_t elapsed_us = 0;

        size_t working_set_bytes = 0;
        size_t peak_working_set_bytes = 0;
        size_t private_bytes = 0;

        int map_width = 0;
        int map_height = 0;
    };

    struct MemorySampler {
        std::atomic<bool> running{false};
        std::thread worker;
        std::mutex samples_mtx;
        std::vector<MemoryRecord> samples;

        void start(int interval_ms, const std::string& label, int run) {
            if (running.load()) 
                return;
            
            running.store(true);
            
            worker = std::thread([this, interval_ms, label, run]() {
                uint64_t t_start = now_usec();

                while (running.load()) {
                    auto s = sample_process_memory();

                    MemoryRecord r;
                    r.label = label;
                    r.run = run;
                    r.elapsed_us = now_usec() - t_start;
                    r.working_set_bytes = s.working_set_bytes;
                    r.peak_working_set_bytes = s.peak_working_set_bytes;
                    r.private_bytes = s.private_bytes;
                    r.map_width = get_map_w().load();
                    r.map_height = get_map_h().load();

                    {
                        std::lock_guard<std::mutex> lk(samples_mtx);
                        samples.push_back(std::move(r));
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                }
            });
        }

        void stop() {
            running.store(false);
            if (worker.joinable()) 
                worker.join();
        }
    };

    inline MemorySampler& get_sampler() {
        static MemorySampler s;
        return s;
    }



    // ENVIRONMENT, metadane
    inline std::string get_cpu_brand() {
        int cpui[4] = {0};
        char brand[0x40] = {};
        __cpuid(cpui, 0x80000000);
        unsigned int n_ex_ids = static_cast<unsigned int>(cpui[0]);

        if (n_ex_ids >= 0x80000004) {
            __cpuid(reinterpret_cast<int*>(brand + 0),  0x80000002);
            __cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
            __cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
        }
        
        return std::string(brand);
    }

    inline std::string get_godot_version() {
        godot::Dictionary v = godot::Engine::get_singleton()->get_version_info();
        godot::String s = v.get("string", godot::String("unknown"));
        return std::string(s.utf8().get_data());
    }

    struct EnvironmentInfo {
        std::string cpu_brand;
        
        int logical_cores = 0;
        int omp_max_threads = 0;

        std::string build_type;
        std::string godot_version;
    };

    inline EnvironmentInfo get_environment_info() {
        EnvironmentInfo info;

        info.cpu_brand = get_cpu_brand();
        info.logical_cores = static_cast<int>(std::thread::hardware_concurrency());
        info.omp_max_threads = omp_get_max_threads();
        info.godot_version = get_godot_version();
        #ifdef _DEBUG
            info.build_type = "debug";
        #else
            info.build_type = "release";
        #endif

        return info;
    }

    inline std::string environment_info_as_header(const EnvironmentInfo& info, int active_threads) {
        std::ostringstream oss;
        oss << "# cpu_brand=" << info.cpu_brand << "\n"
            << "# logical_cores=" << info.logical_cores << "\n"
            << "# omp_max_threads=" << info.omp_max_threads << "\n"
            << "# active_threads=" << active_threads << "\n"
            << "# build_type=" << info.build_type << "\n"
            << "# godot_version=" << info.godot_version << "\n";
        return oss.str();
    }

    inline uint64_t get_thread_id() {
        return std::hash<std::thread::id>{}(std::this_thread::get_id());
    }



    class Store {
    public:
        static Store& get() {
            static Store s;
            return s;
        }

        void push_time(TimeRecord&& r) {
            std::lock_guard<std::mutex> lk(time_mtx);
            time_records.push_back(std::move(r));
        }

        void push_memory(MemoryRecord&& r) {
            std::lock_guard<std::mutex> lk(mem_mtx);
            memory_records.push_back(std::move(r));
        }

        std::vector<TimeRecord> drain_time() {
            std::lock_guard<std::mutex> lk(time_mtx);
            return std::move(time_records);
        }

        std::vector<MemoryRecord> drain_memory() {
            std::lock_guard<std::mutex> lk(mem_mtx);
            return std::move(memory_records);
        }

        bool save_time_csv(const std::string& path) {
            auto records = drain_time();
            if (records.empty())
                return true;

            std::ofstream f(path, std::ios::out | std::ios::app);
            if (!f.is_open())
                return false;

            for (auto& r : records) {
                f << r.caller << ","
                << r.thread_id << ","
                << r.function << ","
                << r.duration_us << ","
                << r.threads_used << ","
                << r.map_width << ","
                << r.map_height << "\n";
            }

            return true;
        }

        bool save_memory_csv(const std::string& path) {
            auto records = drain_memory();
            if (records.empty())
                return true;

            std::ofstream f(path, std::ios::out | std::ios::app);
            if (!f.is_open())
                return false;

            for (auto& r : records) {
                f << r.label << ","
                << r.run << ","
                << r.elapsed_us << ","
                << r.map_width << ","
                << r.map_height << ","
                << r.working_set_bytes << ","
                << r.peak_working_set_bytes << ","
                << r.private_bytes << "\n";
            }

            return true;
        }

    private:
        std::mutex time_mtx;
        std::vector<TimeRecord> time_records;

        std::mutex mem_mtx;
        std::vector<MemoryRecord> memory_records;
    };



    // SIMPLE SCOPED PROFILER
    class Scope {
    public:
        Scope(std::string caller, std::string fn): caller_(std::move(caller)), fn_(std::move(fn)), start_(now_usec()) {}

        ~Scope() {
            uint64_t end = now_usec();

            TimeRecord r;
            r.caller = caller_;
            r.function = fn_;
            r.thread_id = get_thread_id();

            r.duration_us = end - start_;

            r.map_width = get_map_w().load();
            r.map_height = get_map_h().load();

            r.threads_used = omp_get_max_threads();

            Store::get().push_time(std::move(r));
        }

    private:
        std::string caller_;
        std::string fn_;
        uint64_t start_;
    };
} // namespace tp



// singleton available from GDScript as "Profiler"
class ProfilerGD : public godot::Object {
    GDCLASS(ProfilerGD, godot::Object)

public:
    void set_map_size(int w, int h) {
        tp::get_map_w().store(w);
        tp::get_map_h().store(h);
    }

    godot::Variant profile(godot::String caller, godot::String fn_name, godot::Callable callable, godot::Array args) {
        tp::Scope scope(caller.utf8().get_data(), fn_name.utf8().get_data());
        return callable.callv(args);
    }

    godot::Variant profile_time(godot::String caller, godot::String fn_name, godot::Callable callable, godot::Array args) {
        return profile(caller, fn_name, callable, args);
    }

    void begin_tree(godot::String caller, godot::String fn_name) {
        current_scope = std::make_unique<tp::Scope>(caller.utf8().get_data(), fn_name.utf8().get_data());
    }

    void end_tree() {
        current_scope.reset();
    }

    bool save_time_csv(godot::String path) {
        godot::String abs = godot::ProjectSettings::get_singleton()->globalize_path(path);
        return tp::Store::get().save_time_csv(abs.utf8().get_data());
    }

    void clear_time() {
        tp::Store::get().drain_time();
    }



    // MEMORY
    void record_memory(godot::String label, int run) {
        auto s = tp::sample_process_memory();

        tp::MemoryRecord r;
        r.label = label.utf8().get_data();
        r.run = run;
        r.map_width = tp::get_map_w().load();
        r.map_height = tp::get_map_h().load();
        r.working_set_bytes = s.working_set_bytes;
        r.peak_working_set_bytes = s.peak_working_set_bytes;
        r.private_bytes = s.private_bytes;

        tp::Store::get().push_memory(std::move(r));
    }

    bool save_memory_csv(godot::String path) {
        godot::String abs = godot::ProjectSettings::get_singleton()->globalize_path(path);
        return tp::Store::get().save_memory_csv(abs.utf8().get_data());
    }

    void clear_memory() {
        tp::Store::get().drain_memory();
    }

    void start_memory_sampling(int interval_ms, godot::String label, int run) {
        tp::get_sampler().start(interval_ms, label.utf8().get_data(), run);
    }

    void stop_memory_sampling() {
        tp::get_sampler().stop();
        std::lock_guard<std::mutex> lk(tp::get_sampler().samples_mtx);
        for (auto& r : tp::get_sampler().samples){
            tp::Store::get().push_memory(std::move(r));
        }
        tp::get_sampler().samples.clear();
    }



    // THREAD CONTROL
    void set_thread_count(int n) {
        n = n < 1 ? 1 : n;
        // bez tego runtime może zignorować żądaną liczbę
        omp_set_dynamic(0);
        omp_set_num_threads(n);
    }

    int get_max_threads() {
        return omp_get_max_threads();
    }

    int get_hardware_concurrency() {
        return static_cast<int>(std::thread::hardware_concurrency());
    }



    // dla nowego pliku wyników: metadane środowiska + nagłówek kolumn, wywołane raz na plik przed save_csv
    bool begin_results_file(godot::String path, godot::String column_header) {
        godot::String abs = godot::ProjectSettings::get_singleton()->globalize_path(path);
        std::ofstream f(abs.utf8().get_data(), std::ios::out | std::ios::trunc);

        if (!f.is_open())
            return false;

        auto info = tp::get_environment_info();
        f << tp::environment_info_as_header(info, omp_get_max_threads()) << column_header.utf8().get_data() << "\n";
        
        return true;
    }

protected:
    static void _bind_methods() {
        using namespace godot;

        ClassDB::bind_method(D_METHOD("set_map_size", "w", "h"), &ProfilerGD::set_map_size);
        ClassDB::bind_method(D_METHOD("profile", "caller", "fn", "callable", "args"), &ProfilerGD::profile);
        ClassDB::bind_method(D_METHOD("profile_time", "caller", "fn", "callable", "args"), &ProfilerGD::profile_time);
        ClassDB::bind_method(D_METHOD("begin_tree", "caller", "fn"), &ProfilerGD::begin_tree);
        ClassDB::bind_method(D_METHOD("end_tree"), &ProfilerGD::end_tree);
        ClassDB::bind_method(D_METHOD("save_time_csv", "path"), &ProfilerGD::save_time_csv);
        ClassDB::bind_method(D_METHOD("clear_time"), &ProfilerGD::clear_time);
        ClassDB::bind_method(D_METHOD("record_memory", "label", "run"), &ProfilerGD::record_memory);
        ClassDB::bind_method(D_METHOD("save_memory_csv", "path"), &ProfilerGD::save_memory_csv);
        ClassDB::bind_method(D_METHOD("clear_memory"), &ProfilerGD::clear_memory);
        ClassDB::bind_method(D_METHOD("start_memory_sampling", "interval_ms", "label", "run"), &ProfilerGD::start_memory_sampling);
        ClassDB::bind_method(D_METHOD("stop_memory_sampling"), &ProfilerGD::stop_memory_sampling);
        ClassDB::bind_method(D_METHOD("set_thread_count", "n"), &ProfilerGD::set_thread_count);
        ClassDB::bind_method(D_METHOD("get_max_threads"), &ProfilerGD::get_max_threads);
        ClassDB::bind_method(D_METHOD("get_hardware_concurrency"), &ProfilerGD::get_hardware_concurrency);
        ClassDB::bind_method(D_METHOD("begin_results_file", "path", "column_header"), &ProfilerGD::begin_results_file);
    }

private:
    std::unique_ptr<tp::Scope> current_scope;
};