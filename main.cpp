#include <co/all.h>
#include <hiredis.h>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "co/co.h"

using namespace std;
using namespace std::chrono;

co::wait_group wg;

void coroutine(redisContext *c, int id) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 10000);

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 10; i++) {
        string key = "key" + to_string(dis(gen));
        string value = "value" + to_string(dis(gen));

        auto *reply = (redisReply *)redisCommand(c, "SET %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply);

        reply = (redisReply *)redisCommand(c, "GET %s", key.c_str());
        freeReplyObject(reply);
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "coroutine execution time: " << duration.count() << "ms" << endl;
}

auto main(int argc, char *argv[]) -> int {
    flag::parse(argc, argv);
    auto *s = co::next_sched();
    vector<redisContext *> connect;
    co::wait_group init;
    init.add();
    go([&]() {
        for (int i = 0; i < 200; i++) {
            redisContext *c = redisConnect("9.135.11.101", 32772);
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
    for (int i = 0; i < 100; i++) {
        wg.add();
        s->go([&connect, i]() {
            coroutine(connect[i], i);
            wg.done();
        });
    }
    auto start = high_resolution_clock::now();
    wg.wait();
    auto end = high_resolution_clock::now();
    for (const auto &e : connect) {
        redisFree(e);
    }

    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Total execution time: " << duration.count() << "ms" << endl;

    return 0;
}
