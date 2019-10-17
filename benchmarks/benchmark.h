#pragma once

#include <ozo/optional.h>
#include <ozo/type_traits.h>
#include <ozo/pg/types.h>

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
#include <atomic>

namespace ozo::benchmark {

namespace hana = boost::hana;

template <typename Ratio>
struct duration_name {};

template <>
struct duration_name<std::ratio<1>> {
    friend std::ostream& operator <<(std::ostream& stream, duration_name) {
        return stream << "s";
    }
};

template <>
struct duration_name<std::milli> {
    friend std::ostream& operator <<(std::ostream& stream, duration_name) {
        return stream << "ms";
    }
};

template <>
struct duration_name<std::micro> {
    friend std::ostream& operator <<(std::ostream& stream, duration_name) {
        return stream << "us";
    }
};

template <>
struct duration_name<std::nano> {
    friend std::ostream& operator <<(std::ostream& stream, duration_name) {
        return stream << "ns";
    }
};

std::ostream& operator <<(std::ostream& stream, std::chrono::steady_clock::duration value) {
    bool printed = false;
    hana::for_each(hana::make_tuple(std::nano(), std::micro(), std::milli()),
        [&] (auto ratio) {
            if (printed) {
                return;
            }
            using ratio_t = decltype(ratio);
            const auto converted = std::chrono::duration_cast<std::chrono::duration<double, ratio_t>>(value).count();
            if (converted < 1000) {
                stream << converted << ' ' << duration_name<ratio_t> {};
                printed = true;
            }
        });
    if (!printed) {
        using ratio_t = std::ratio<1>;
        const auto converted = std::chrono::duration_cast<std::chrono::duration<double, ratio_t>>(value).count();
        stream << converted << ' ' << duration_name<ratio_t> {};
    }
    return stream;
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
    OZO_STD_OPTIONAL<std::chrono::steady_clock::time_point> start_time;
    OZO_STD_OPTIONAL<std::chrono::steady_clock::time_point> finish;
    std::size_t total_rows_count;
};

class time_limit_benchmark {
public:
    time_limit_benchmark(std::size_t coroutines, std::chrono::steady_clock::duration max_duration = std::chrono::seconds(31))
            : max_duration(max_duration), request_start(coroutines, start) {
        steps.reserve(100);
        requests.reserve(1000000);
    }

    ~time_limit_benchmark() {
        using double_s = std::chrono::duration<double, std::ratio<1>>;
        if (!requests.empty()) {
            boost::sort(requests);
            std::cout << "mean request time: " << boost::accumulate(requests, std::chrono::steady_clock::duration()) / requests.size() << std::endl;
            std::cout << "median request time: " << requests[requests.size() / 2 + 1] << std::endl;
            std::cout << "q90 request time: " << requests[requests.size() * 9 / 10] << std::endl;
            std::cout << "min request time: " << requests.front() << std::endl;
            std::cout << "max request time: " << requests.back() << std::endl;
        }
        std::cout << "mean requests speed: " << total_requests_count / std::chrono::duration_cast<double_s>(finish - start).count()
                  << " req/sec" << std::endl;
        std::vector<double> requests_speeds;
        if (!steps.empty()) {
            boost::transform(steps, std::back_inserter(requests_speeds),
                [] (const auto& v) { return v.requests_count / std::chrono::duration_cast<double_s>(v.duration).count(); });
            boost::sort(requests_speeds);
            std::cout << "median requests speed: " << requests_speeds[requests_speeds.size() / 2 + 1] << " req/sec" << std::endl;
            std::cout << "min requests speed: " << requests_speeds.front() << " req/sec" << std::endl;
            std::cout << "max requests speed: " << requests_speeds.back() << " req/sec" << std::endl;
        }
        std::cout << "mean read rows speed: " << total_rows_count / std::chrono::duration_cast<double_s>(finish - start).count()
                  << " row/sec" << std::endl;
        std::vector<double> rows_speeds;
        if (!steps.empty()) {
            boost::transform(steps, std::back_inserter(rows_speeds),
                [] (const auto& v) { return v.rows_count / std::chrono::duration_cast<double_s>(v.duration).count(); });
            boost::sort(rows_speeds);
            std::cout << "median read rows speed: " << rows_speeds[rows_speeds.size() / 2 + 1] << " row/sec" << std::endl;
            std::cout << "min read rows speed: " << rows_speeds.front() << " row/sec" << std::endl;
            std::cout << "max read rows speed: " << rows_speeds.back() << " row/sec" << std::endl;
        }
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
    std::vector<std::chrono::steady_clock::time_point> request_start;

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
