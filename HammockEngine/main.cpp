#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Apps/FirstApp.h"
#include "Utils/HmckLogger.h"



int main()
{
	Hmck::FirstApp app{};
	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		int c = getchar(); // wait for user to see the error
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}