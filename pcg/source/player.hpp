#pragma once

#include "factory.hpp"
#include <vector>

namespace pcg {
namespace player {
    void AddPlayer()
    {
        n++;
        moneys.emplace_back(1000);
    }
    void Tax(u32 tax = 20)
    {
        u32 f_n = factory::moneys.size();
        for (u32 i = 0; i < f_n; i++) {
            if (factory::moneys[i] < tax + 1000)
                return;
            
            const Entity owner = factory::owners[i];
            factory::moneys[i] -= tax;
            moneys[owner.index] += tax;
        }
    }
    void Stick() {
        
    }
    void DoAction(const i32& owner, f32& money)
    {
        if (money > 600) {
            factory::AddFactory(owner, rand() % markets::n, 600, 10);
            money -= 600;
        }
    }
    void DoAction()
    {
        for (i32 i = 0; i < n; i++)
            DoAction(i, moneys[i]);
    }
    }
}
