#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Apps/PBRApp.h"
#include "Apps/RaymarchingDemoApp.h"
#include "Apps/CloudsApp.h"
#include "Utils/HmckLogger.h"
#include "HmckEntity.h"



int main()
{
	

	try
	{
		while (true) 
		{
			int demo;
			std::cout << "Enter a demo ID:\n0 - Exit\n1 - PBR Demo\n2 - Pretty clouds\n3 - Physicaly accurate clouds\n";
			std::cin >> demo;

			if (demo == 0) {
				break;
			}
			else if (demo == 1)
			{
				Hmck::PBRApp app{};
				app.run();
			}
			else if (demo == 2)
			{
				Hmck::RaymarchingDemoApp app{};
				app.run();
			}
			else if (demo == 3)
			{
				Hmck::CloudsApp app{};
				app.run();
			}

			Hmck::Entity::restIds();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		int c = getchar(); // wait for user to see the error
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}