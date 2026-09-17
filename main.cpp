#include "Engine/GameWindow.h"

int main()
{
    GameWindow window(1280, 720, "Tetrisnow");
    if (!window.initialize()) {
        return -1;
    }

    window.run();
    return 0;
}
