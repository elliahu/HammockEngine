#include <cstdlib>
#include <iostream>

#include "PBRApp.h"
#include "VolumeApp.h"


int main() {

    try {
        while (true) {
            int demo;
            std::cout <<
                    "Enter a demo ID:\n0 - Exit\n1 - PBR Demo\n2 - Volume data rendering from 3D texture\n";
            std::cin >> demo;

            if (demo == 0) {
                break;
            } else if (demo == 1) {
                Hammock::PBRApp app{};
                app.run();
            } else if (demo == 2) {
                Hammock::VolumeApp app{};
                app.run();
            }

        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        int c = getchar(); // wait for user to see the error
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
