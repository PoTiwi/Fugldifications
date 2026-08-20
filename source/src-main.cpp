// here where you would edit. main.cpp bascially

#include "inserts/Memory.hpp"

int main() {
    if (!init()) return 1;

    Instance gravityAddr = Imem.point_addr(g_world, Offsets::World::Gravity);

    while (g_isRunning) {
        if (checkExitKey()) {
            g_isRunning = false;
            break;
        }

        if (gravityAddr) {
            // Rewrite the memory to set the game gravity to 0.0 (float)
            Imem.overwrite<float>(gravityAddr, 0.0f);
        }

        sleep_ms(10); 
    }

    cleanup();
    return 0;
}
