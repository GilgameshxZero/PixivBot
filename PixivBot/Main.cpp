#include "Main.h"

int main() {
	int error = Monochrome8::PixivBot::start();

	if (error != 0) {
		std::cout << "start returned error code " << error << "\r\nExiting in 3 seconds...";
		Sleep(3000);
	}

	return error;
}