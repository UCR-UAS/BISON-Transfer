#include <yaml-cpp/yaml.h>
#include <stdio.h>
#include <string>
#include <exception>
#include <fstream>
#include "BISON-Defaults.h"

void error_terminate(int error)
{
	exit(error);
}

int main ()
{
	YAML::Node config;
	try {
		config = YAML::LoadFile("test.yaml");
#if BRANDON_DEBUG
		std::cout << "Successfully opened file." << std::endl;
		std::cout << "Attempting to print by strings" << std::endl;
#endif // BRANDON_DEBUG
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "Attempting to configure automatically." << std::endl;
	}
	if (!config["user"]["username"])
	  config["user"]["username"] = "user";
	if (!config["user"]["password"])
	  config["user"]["password"] = "pass";

	const std::string username = config["user"]["username"].as<std::string>();
	const std::string password = config["user"]["password"].as<std::string>();

#if BRANDON_DEBUG
	printf("Username: %s\n", username.c_str());
	printf("Password: %s\n", password.c_str());
#endif // BRANDON_DEBUG

	std::ofstream fout("test.yaml");
	fout << "%YAML 1.2\n" << "---\n";		// Insert version string.
	fout << config;
	return 0;
}
