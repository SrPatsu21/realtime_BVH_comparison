// Copyright © 2026 SrPatsu21
// Licensed under the Apache License, Version 2.0

#include "./Render.hpp"

int main() {
    Render* render = new Render();

    render->run();

    delete(render);
    return 0;
}