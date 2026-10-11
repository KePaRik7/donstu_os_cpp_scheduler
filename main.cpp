#include "hrrn.h"
#include "testsets.h"
#include "fcfs.h"
#include "sjf.h"
#include "srtn.h"
#include "rr.h"
#include "priority.h"
#include "mlfq.h"
#include "simulator.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <functional>

// ============================================================
// Тестовые наборы
// ============================================================

std::vector<Process> makeTestSet() {
    std::vector<Process> procs;
    auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                   std::uint64_t burst, int priority,
                   std::vector<IoBlock> io = {}) {
        Process p;
        p.pid = pid;
        p.name = name;
        p.arrivalTime = arrival;
        p.burstTime = burst;
        p.remainingTime = burst;
        p.priority = priority;
        p.dynamicPriority = priority;
        p.ioBlocks = std::move(io);
        procs.push_back(p);
    };
    add(1, "P1", 0, 8, 3);
    add(2, "P2", 1, 4, 1);
    add(3, "P3", 2, 9, 4);
    add(4, "P4", 3, 5, 2);
    return procs;
}

std::vector<Process> makeIoTestSet() {
    std::vector<Process> procs;
    auto add = [&](int pid, const std::string& name,
                   std::uint64_t arrival, std::uint64_t burst,
                   int priority,
                   std::vector<IoBlock> io = {}) {
        Process p;
        p.pid = pid; p.name = name;
        p.arrivalTime = arrival;
        p.burstTime = burst;
        p.remainingTime = burst;
        p.priority = priority;
        p.dynamicPriority = priority;
        p.ioBlocks = std::move(io);
        procs.push_back(p);
    };
    add(1, "CPU1", 0, 20, 2);
    add(2, "IO1",  1,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
    add(3, "IO2",  2,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
    add(4, "CPU2", 3, 10, 3);
    return procs;
}

std::vector<Process> makeHrrnTestSet() {
    std::vector<Process> procs;
    auto add = [&](int pid, const std::string& name,
                   std::uint64_t arrival, std::uint64_t burst) {
        Process p;
        p.pid = pid;
        p.name = name;
        p.arrivalTime = arrival;
        p.burstTime = burst;
        p.remainingTime = burst;
        p.priority = 1;
        p.dynamicPriority = 1;
        procs.push_back(p);
    };
    add(1, "A",  0, 4);
    add(2, "L",  1, 6);
    add(3, "S1", 4, 2);
    add(4, "S2", 5, 1);
    return procs;
}

// ============================================================
// Вывод
// ============================================================

void printResult(const SimResult& r) {
    std::cout << std::left << std::setw(30) << r.algorithm
              << " | wait=" << std::setw(7) << std::fixed << std::setprecision(2) << r.avgWaiting
              << " | turn=" << std::setw(7) << r.avgTurnaround
              << " | resp=" << std::setw(7) << r.avgResponse
              << " | CPU=" << std::setw(6) << r.cpuUtilization << "%"
              << " | CS=" << std::setw(4) << r.contextSwitches
              << " | done=" << r.throughput
              << "\n";
}

void printGantt(const SimResult& r) {
    std::cout << "Gantt (" << r.algorithm << "):\n";
    for (auto& [pid, span] : r.gantt) {
        std::cout << "  [" << span.first << "-" << span.second << ") ";
        if (pid == -1) std::cout << "IDLE\n";
        else if (pid == -2) std::cout << "CS\n";
        else std::cout << "P" << pid << "\n";
    }
    std::cout << "\n";
}

// ============================================================
// Задание 10. Сравнение алгоритмов на 5 случайных наборах
// ============================================================

using Factory = std::function<std::unique_ptr<Scheduler>(const std::vector<Process>&)>;

template <class T, class... Args>
Factory factory(Args... args) {
    return [args...](const std::vector<Process>& s) {
        return std::make_unique<T>(s, args...);
    };
}

void runExperiment10Metric(const std::string& metricName,
                           std::function<double(const SimResult&)> getMetric) {
    std::vector<std::pair<std::string, Factory>> algos = {
        {"FCFS",       factory<FcfsScheduler>()},
        {"SJF",        factory<SjfScheduler>()},
        {"SRTN",       factory<SrtnScheduler>()},
        {"HRRN",       factory<HrrnScheduler>()},
        {"RR q=4",     factory<RrScheduler>(4)},
        {"Prio",       factory<PriorityScheduler>(false, false)},
        {"Prio+aging", factory<PriorityScheduler>(true, true)},
        {"MLFQ",       factory<MlfqScheduler>()},
    };

    const int sizes[5] = {12, 14, 16, 18, 20};

    std::cout << "\n=== Задание 10: " << metricName << " по 5 наборам ===\n";
    std::cout << std::left << std::setw(12) << "Algorithm" << std::right;
    for (int i = 1; i <= 5; ++i) {
        std::cout << std::setw(9) << ("set " + std::to_string(i));
    }
    std::cout << std::setw(9) << "average" << "\n";

    for (auto& [label, make] : algos) {
        std::cout << std::left << std::setw(12) << label << std::right;
        double sum = 0;
        for (unsigned seed = 1; seed <= 5; ++seed) {
            auto set = makeRandomSet(seed, sizes[seed - 1]);
            auto sched = make(set);
            SimResult r = runSimulation(*sched);
            double v = getMetric(r);
            sum += v;
            std::cout << std::setw(9) << std::fixed << std::setprecision(2) << v;
        }
        std::cout << std::setw(9) << (sum / 5) << "\n";
    }
}

void runExperiment10() {
    runExperiment10Metric("avg waiting time",
                          [](const SimResult& r) { return r.avgWaiting; });
    runExperiment10Metric("avg turnaround time",
                          [](const SimResult& r) { return r.avgTurnaround; });
    runExperiment10Metric("avg response time",
                          [](const SimResult& r) { return r.avgResponse; });
    runExperiment10Metric("CPU utilization %",
                          [](const SimResult& r) { return r.cpuUtilization; });
}

// ============================================================
// main
// ============================================================

int main() {
    // ---------- Case 1: CPU-bound ----------
    std::cout << "Case 1: CPU-bound\n";
    {
        auto set1 = makeTestSet();
        std::vector<std::unique_ptr<Scheduler>> scheds;
        scheds.push_back(std::make_unique<FcfsScheduler>(set1));
        scheds.push_back(std::make_unique<SjfScheduler>(set1));
        scheds.push_back(std::make_unique<SrtnScheduler>(set1));
        scheds.push_back(std::make_unique<RrScheduler>(set1, 1));
        scheds.push_back(std::make_unique<RrScheduler>(set1, 2));
        scheds.push_back(std::make_unique<RrScheduler>(set1, 4));
        scheds.push_back(std::make_unique<PriorityScheduler>(set1, false, false));
        scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, false));
        scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, true));
        scheds.push_back(std::make_unique<MlfqScheduler>(set1));
        for (auto& s : scheds) printResult(runSimulation(*s));
    }

    // ---------- Case 2: Mixed loading ----------
    std::cout << "\nCase 2. Mixed loading: I/O + CPU\n";
    {
        auto set2 = makeIoTestSet();
        std::vector<std::unique_ptr<Scheduler>> scheds;
        scheds.push_back(std::make_unique<FcfsScheduler>(set2));
        scheds.push_back(std::make_unique<SrtnScheduler>(set2));
        scheds.push_back(std::make_unique<RrScheduler>(set2, 4));
        scheds.push_back(std::make_unique<PriorityScheduler>(set2, true, true));
        scheds.push_back(std::make_unique<MlfqScheduler>(set2));
        for (auto& s : scheds) printResult(runSimulation(*s));
    }

    // ---------- Задание 3: HRRN ----------
    std::cout << "\n=== Задание 3: HRRN vs SJF ===\n";
    {
        auto set = makeHrrnTestSet();

        {
            SjfScheduler sjf(set);
            SimResult r = runSimulation(sjf);
            printResult(r);
            printGantt(r);
        }
        {
            HrrnScheduler hrrn(set);
            SimResult r = runSimulation(hrrn);
            printResult(r);
            printGantt(r);
        }
    }

    // ---------- Задание 4: переключение контекста ----------
    std::cout << "\n=== Задание 4: влияние переключения контекста ===\n";
    std::cout << "q  cost   wait    turn    resp    CPU%   overhead%\n";
    {
        auto set = makeTestSet();
        for (std::uint64_t q : {1, 2, 4, 8}) {
            for (std::uint64_t cost : {0, 1}) {
                RrScheduler rr(set, q);
                SimResult r = runSimulation(rr, 100000, cost);
                std::cout << std::setw(2) << q << "  "
                          << std::setw(4) << cost << "  "
                          << std::fixed << std::setprecision(2)
                          << std::setw(7) << r.avgWaiting << " "
                          << std::setw(7) << r.avgTurnaround << " "
                          << std::setw(7) << r.avgResponse << " "
                          << std::setw(6) << r.cpuUtilization << " "
                          << std::setw(8) << r.overheadPercent << "\n";
            }
        }
    }

    // ---------- Задание 5: влияние кванта в RR ----------
    std::cout << "\nВлияние кванта в RR (набор makeTestSet)\n";
    std::cout << "q wait turn resp CS\n";
    {
        auto set = makeTestSet();
        for (std::uint64_t q : {1, 2, 4, 8, 16}) {
            RrScheduler rr(set, q);
            SimResult r = runSimulation(rr);
            std::cout << std::setw(2) << q << " " << std::fixed << std::setprecision(2)
                      << r.avgWaiting << " " << r.avgTurnaround << " " << r.avgResponse
                      << " " << std::setw(3) << r.contextSwitches << " "
                      << std::string(r.contextSwitches, '#') << "\n";
        }
    }

    // ---------- Задание 8: таблица по процессам ----------
    std::cout << "\n=== Задание 8: таблица по процессам (RR q=2) ===\n";
    {
        auto set = makeTestSet();
        RrScheduler rr(set, 2);
        SimResult r = runSimulation(rr);
        printProcessTable(rr.processes());
    }

    // ---------- Задание 13: сохранение и загрузка наборов ----------
    std::cout << "\n=== Задание 13: сохранение и загрузка наборов ===\n";
    {
        auto set = makeTestSet();
        if (!saveSet("sets/set_basic.txt", set)) {
            std::cerr << "Не удалось сохранить sets/set_basic.txt\n";
        } else {
            std::cout << "Сохранено: sets/set_basic.txt\n";
        }

        std::vector<Process> loaded;
        if (!loadSet("sets/set_basic.txt", loaded)) {
            std::cerr << "Не удалось загрузить sets/set_basic.txt\n";
        } else {
            FcfsScheduler a(set), b(loaded);
            std::cout << "Оригинал: "; printResult(runSimulation(a));
            std::cout << "Загружен: "; printResult(runSimulation(b));
        }
    }

    // ---------- Задание 9 (по желанию): диаграмма Ганта в CSV ----------
    std::cout << "\n=== Задание 9: сохранение диаграммы Ганта в CSV ===\n";
    {
        auto set = makeTestSet();

        RrScheduler rr(set, 2);
        SimResult r1 = runSimulation(rr);
        if (saveGanttCsv("results/gantt_rr2.csv", r1)) {
            std::cout << "Сохранено: results/gantt_rr2.csv\n";
        } else {
            std::cerr << "Не удалось сохранить results/gantt_rr2.csv\n";
        }

        MlfqScheduler mlfq(set);
        SimResult r2 = runSimulation(mlfq);
        if (saveGanttCsv("results/gantt_mlfq.csv", r2)) {
            std::cout << "Сохранено: results/gantt_mlfq.csv\n";
        } else {
            std::cerr << "Не удалось сохранить results/gantt_mlfq.csv\n";
        }
    }

    // ---------- Задание 10: сравнение на 5 наборах ----------
    {
        std::cout << "\n=== Задание 10: сохранение 5 наборов в файлы ===\n";
        const int sizes[5] = {12, 14, 16, 18, 20};
        for (unsigned seed = 1; seed <= 5; ++seed) {
            std::string path = "sets/set" + std::to_string(seed) + ".txt";
            if (saveSet(path, makeRandomSet(seed, sizes[seed - 1]))) {
                std::cout << "Сохранено: " << path << "\n";
            } else {
                std::cerr << "Ошибка сохранения: " << path << "\n";
            }
        }
    }

    runExperiment10();

        {
        auto set = makeTestSet();

        RrScheduler rr(set, 2);
        SimResult r1 = runSimulation(rr);
        if (saveGanttCsv("results/gantt_rr2.csv", r1)) {
            std::cout << "Сохранено: results/gantt_rr2.csv\n";
        } else {
            std::cerr << "Не удалось сохранить results/gantt_rr2.csv\n";
        }

        MlfqScheduler mlfq(set);
        SimResult r2 = runSimulation(mlfq);
        if (saveGanttCsv("results/gantt_mlfq.csv", r2)) {
            std::cout << "Сохранено: results/gantt_mlfq.csv\n";
        } else {
            std::cerr << "Не удалось сохранить results/gantt_mlfq.csv\n";
        }
    }   
 return 0;
}

