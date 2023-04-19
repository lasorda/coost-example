#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "co/all.h"
#include "co/co.h"
#include "co/co/wait_group.h"
#include "hiredis/hiredis.h"

using namespace std;
using namespace std::chrono;

co::wait_group wg;
// 执行Set和Get操作的协程函数
void coroutine(redisContext *c, int id) {
    // 设置随机数生成器
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 10000);

    // 执行1000次Set和Get操作，并记录时间
    auto start = high_resolution_clock::now();
    for (int i = 0; i < 10; i++) {
        // 生成随机键和值
        string key = "key" + to_string(dis(gen));
        string value = "value" + to_string(dis(gen));

        // 执行Set操作
        auto *reply = (redisReply *)redisCommand(c, "SET %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply);

        // 执行Get操作
        reply = (redisReply *)redisCommand(c, "GET %s", key.c_str());
        freeReplyObject(reply);
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "coroutine execution time: " << duration.count() << "ms" << endl;
}

auto main(int argc, char *argv[]) -> int {
    flag::parse(argc, argv);
    // 创建协程调度器
    auto *s = co::next_scheduler();
    vector<redisContext *> connect;
    co::wait_group init;
    init.add();
    go([&]() {
        for (int i = 0; i < 200; i++) {
            // 连接Redis服务器
            redisContext *c = redisConnect("9.135.11.101", 32771);
            if (c == nullptr || c->err) {
                if (c) {
                    cerr << "Error: " << c->errstr << endl;
                    redisFree(c);
                } else {
                    cerr << "Can't allocate redis context" << endl;
                }
                break;
            }
            connect.push_back(c);
        }
        init.done();
    });
    init.wait();
    // 创建100个协程，并执行Set和Get操作
    for (int i = 0; i < 100; i++) {
        wg.add();
        s->go([&connect, i]() {
            coroutine(connect[i], i);
            wg.done();
        });
    }
    // 启动协程调度器，并记录时间
    auto start = high_resolution_clock::now();
    wg.wait();
    auto end = high_resolution_clock::now();
    for (const auto &e : connect) {
        redisFree(e);
    }

    // 输出执行时间
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Total execution time: " << duration.count() << "ms" << endl;

    return 0;
}
