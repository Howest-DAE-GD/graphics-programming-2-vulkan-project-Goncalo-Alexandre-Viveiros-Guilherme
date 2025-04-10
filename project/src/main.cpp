#include <iostream>
#include "GGVulkan.h"

int main()
{
	GGVulkan app;

	try
	{
		app.Run();
	}

	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
