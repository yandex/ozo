#include <ozo/optional.h>
#include <ozo/type_traits.h>

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/range/numeric.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <vector>
#include <variant>
#include <fstream>
#include <sstream>

namespace ozo::benchmark {

namespace hana = boost::hana;

struct benchmark_named_value {
    std::string name;
    std::variant<std::size_t, std::int64_t, double, std::string> value;
};

struct benchmark_report {
    std::string name;
    std::vector<benchmark_named_value> parameters;
    std::vector<benchmark_named_value> metrics;
};

void write_to_yaml(std::ostream& stream, const benchmark_named_value& value) {
    stream << "  - name: " << value.name << "\n";
    stream << "    value: ";
    std::visit([&] (const auto& v) { stream << v; }, value.value);
    stream << "\n";
}

void write_to_yaml_file(const std::vector<benchmark_report>& reports, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "can't open benchmark report file \"" << filename << "\"" << std::endl;
        return;
    }
    std::ostringstream buffer;
    buffer << "---\n";
    for (const auto& report : reports) {
        buffer << "- name: " << report.name << "\n";
        buffer << "  parameters:\n";
        for (const auto& parameter : report.parameters) {
            write_to_yaml(buffer, parameter);
        }
        buffer << "  metrics:\n";
        for (const auto& metric : report.metrics) {
            write_to_yaml(buffer, metric);
        }
    }
    if (!(file << buffer.str())) {
        std::cout << "can't write to benchmark report file \"" << filename << "\"" << std::endl;
        return;
    }
}

class rows_count_limit_benchmark {
public:
    rows_count_limit_benchmark(const std::size_t max_rows_count)
        : max_rows_count(max_rows_count) {}

    void start() {
        if (!start_time) {
            total_rows_count = 0;
            start_time = std::chrono::steady_clock::now();
        }
    }

    bool step(std::size_t rows_count) {
        if (finish) {
            return false;
        }
        total_rows_count += rows_count;
        if (total_rows_count >= max_rows_count) {
            finish = std::chrono::steady_clock::now();
            return false;
        }
        return true;
    }

    ~rows_count_limit_benchmark() {
        using double_s = std::chrono::duration<double, std::ratio<1>>;
        if (start_time && finish) {
            std::cout << "read " << total_rows_count << " rows, "
                      << std::setprecision(3) << std::fixed
                      << (total_rows_count / std::chrono::duration_cast<double_s>(*finish - *start_time).count())
                      << " row/sec" << std::endl;
        }
    }

private:
    std::size_t max_rows_count;
    __OZO_STD_OPTIONAL<std::chrono::steady_clock::time_point> start_time;
    __OZO_STD_OPTIONAL<std::chrono::steady_clock::time_point> finish;
    std::size_t total_rows_count;
};

template <std::size_t coroutines>
class time_limit_benchmark {
public:
    time_limit_benchmark(std::chrono::steady_clock::duration max_duration = std::chrono::seconds(1))
            : max_duration(max_duration) {
        steps.reserve(100);
        requests.reserve(1000000);
        std::fill(request_start.begin(), request_start.end(), start);
    }

    bool step(std::size_t rows_count, std::size_t token = 0) {
        if (finished) {
            return false;
        }
        requests.push_back(std::chrono::steady_clock::now() - request_start[token]);
        step_rows_count += rows_count;
        if (++step_count % modulo == 0) {
            if (!step_impl()) {
                return false;
            }
        }
        request_start[token] = std::chrono::steady_clock::now();
        return true;
    }

    bool thread_safe_step(std::size_t rows_count, std::size_t token = 0) {
        if (finished) {
            return false;
        }
        {
            const std::unique_lock<std::mutex> lock(requests_mutex);
            requests.push_back(std::chrono::steady_clock::now() - request_start[token]);
        }
        step_rows_count += rows_count;
        if (++step_count % modulo == 0) {
            const std::unique_lock<std::mutex> lock(step_mutex);
            if (!step_impl()) {
                return false;
            }
        }
        request_start[token] = std::chrono::steady_clock::now();
        return true;
    }

    benchmark_report report(const std::string& name, const std::vector<benchmark_named_value>& parameters) {
        benchmark_report result;
        result.name = name;
        result.parameters = parameters;
        using double_s = std::chrono::duration<double, std::ratio<1>>;
        if (!requests.empty()) {
            boost::sort(requests);
            result.metrics.push_back(benchmark_named_value {
                "mean request time, ns",
                (boost::accumulate(requests, std::chrono::steady_clock::duration()) / requests.size()).count()
            });
            result.metrics.push_back(benchmark_named_value {"median request time, ns", requests[requests.size() / 2 + 1].count()});
            result.metrics.push_back(benchmark_named_value {"q90 request time, ns", requests[requests.size() * 9 / 10].count()});
            result.metrics.push_back(benchmark_named_value {"min request time, ns", requests.front().count()});
            result.metrics.push_back(benchmark_named_value {"max request time, ns", requests.back().count()});
        }
        result.metrics.push_back(benchmark_named_value {
            "mean requests speed, req/sec",
            total_requests_count / std::chrono::duration_cast<double_s>(finish - start).count()
        });
        std::vector<double> requests_speeds;
        if (!steps.empty()) {
            boost::transform(steps, std::back_inserter(requests_speeds),
                [] (const auto& v) { return v.requests_count / std::chrono::duration_cast<double_s>(v.duration).count(); });
            boost::sort(requests_speeds);
            result.metrics.push_back(benchmark_named_value {
                "median requests speed, req/sec",
                requests_speeds[requests_speeds.size() / 2 + 1]
            });
            result.metrics.push_back(benchmark_named_value {"min requests speed, req/sec", requests_speeds.front()});
            result.metrics.push_back(benchmark_named_value {"max requests speed, req/sec", requests_speeds.back()});
        }
        result.metrics.push_back(benchmark_named_value {
            "mean read rows speed, row/sec",
            total_rows_count / std::chrono::duration_cast<double_s>(finish - start).count()
        });
        std::vector<double> rows_speeds;
        if (!steps.empty()) {
            boost::transform(steps, std::back_inserter(rows_speeds),
                [] (const auto& v) { return v.rows_count / std::chrono::duration_cast<double_s>(v.duration).count(); });
            boost::sort(rows_speeds);
            result.metrics.push_back(benchmark_named_value {"median read rows speed, row/sec", rows_speeds[rows_speeds.size() / 2 + 1]});
            result.metrics.push_back(benchmark_named_value {"min read rows speed, row/sec", rows_speeds.front()});
            result.metrics.push_back(benchmark_named_value {"max read rows speed, row/sec", rows_speeds.back()});
        }
        return result;
    }

private:
    struct step_t {
        std::chrono::steady_clock::duration duration;
        std::size_t requests_count;
        std::size_t rows_count;
    };

    std::mutex requests_mutex;
    std::mutex step_mutex;
    std::chrono::steady_clock::duration max_duration;
    std::size_t total_requests_count = 0;
    volatile std::size_t modulo = 1;
    std::atomic<std::size_t> step_count {0};
    std::atomic<std::size_t> step_rows_count {0};
    std::size_t total_rows_count = 0;
    volatile bool finished = false;
    std::chrono::steady_clock::time_point finish;
    std::vector<step_t> steps;
    std::vector<std::chrono::steady_clock::duration> requests;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point next_print = start + std::chrono::seconds(1);
    std::chrono::steady_clock::time_point step_start = start;
    std::array<std::chrono::steady_clock::time_point, coroutines> request_start;

    bool step_impl() {
        finish = std::chrono::steady_clock::now();
        if (finish >= next_print) {
            using double_s = std::chrono::duration<double, std::ratio<1>>;
            const auto duration = finish - step_start;
            const auto requests_per_second = step_count / std::chrono::duration_cast<double_s>(duration).count();
            const auto rows_per_second = step_rows_count / std::chrono::duration_cast<double_s>(duration).count();
            steps.push_back(step_t {duration, step_count, step_rows_count});
            modulo = std::max(
                static_cast<std::size_t>(modulo),
                static_cast<std::size_t>(std::round(requests_per_second * 0.05))
            );
            total_requests_count += step_count;
            total_rows_count += step_rows_count;
            const auto total_duration = finish - start;
            std::cout << total_requests_count << " requests done in "
                      << std::chrono::duration_cast<double_s>(total_duration).count() << " seconds, "
                      << std::setprecision(4) << std::fixed << requests_per_second << " req/sec "
                      << total_rows_count << " rows read "
                      << std::setprecision(4) << std::fixed << rows_per_second << " row/sec"
                      << std::endl;
            if (total_duration > max_duration) {
                finished = true;
                return false;
            }
            step_count = 0;
            step_rows_count = 0;
            step_start = finish;
            next_print += std::chrono::seconds(1);
        }
        return true;
    }
};

struct pg_type {
    ozo::pg::name typname;
    oid_t typnamespace;
    oid_t typowner;
    std::int16_t typlen;
    bool typbyval;
    char typcategory;
    bool typispreferred;
    bool typisdefined;
    char typdelim;
    oid_t typrelid;
    oid_t typelem;
    oid_t typarray;
};

} // namespace ozo::benchmark

BOOST_FUSION_ADAPT_STRUCT(ozo::benchmark::pg_type,
    typname, typnamespace, typowner, typlen, typbyval, typcategory,
    typispreferred, typisdefined, typdelim, typrelid, typelem, typarray)
