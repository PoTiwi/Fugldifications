// here where you would edit. main.cpp bascially

#include "Backbone/Memory.hpp"

int main() {
    if (!init()) return 1;

    Instance gravityAddr = Imem.point_addr(g_world, Offsets::World::Gravity);

    while_instance_running {
        
        if (gravityAddr) {
            // Rewrite the memory to set the game gravity to 0.0 (float)
            Imem.overwrite<float>(gravityAddr, 0.0f);
        }
    }

    cleanup();
    return 0;
}
