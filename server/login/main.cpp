//
// Created by liao on 2024/5/12.
//
#include <cassert>
#include <iostream>
#include <libaco/aco.h>

void foo(int ct) {
    std::cout << "co: " << aco_get_co() << " yeild to main_co: " << *(static_cast<int *>(aco_get_arg())) << std::endl;
    aco_yield();
    *(static_cast<int *>(aco_get_arg())) = ct + 1;
}

void test_co_func1() {
    std::cout << "co: " << aco_get_co() << " entry: " << *(static_cast<int *>(aco_get_arg())) << std::endl;
    int ct = 0;
    while (ct < 6) {
        foo(ct);
        ct++;
    }
    std::cout << "co: " << aco_get_co() << " exit to main_co: " << *(static_cast<int *>(aco_get_arg())) << std::endl;
    aco_exit();
}

int main(int argc, char *argv[]) {
    aco_thread_init(nullptr);

    auto main_co = aco_create(nullptr, nullptr, 0, nullptr, nullptr);
    auto share_stack = aco_share_stack_new(0);

    int co_func1_value = 0;
    auto co = aco_create(main_co, share_stack, 0, test_co_func1, &co_func1_value);

    int ct = 0;
    while (ct < 6) {
        assert(co->is_end == 0);
        std::cout << "main_co: yeild to co: " << co << " ct: " << ct << std::endl;
        aco_resume(co);
        assert(co_func1_value == ct);
        ct++;
    }
    std::cout << "main_co: yeild to co: " << co << " ct: " << ct << std::endl;
    aco_resume(co);
    assert(co_func1_value == ct);
    assert(co->is_end);

    std::cout << "main_co: destroy and exit" << std::endl;
    aco_destroy(co);
    co = nullptr;
    aco_share_stack_destroy(share_stack);
    share_stack = nullptr;
    aco_destroy(main_co);
    main_co = nullptr;
    return 0;
}
