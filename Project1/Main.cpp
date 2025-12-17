#include "Sence.h"

int main() {
	Sence scene;
	try {
		scene.initWindow();
		scene.initVulkan();
		scene.mainLoop();
		scene.cleanup();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_FAILURE;
}
