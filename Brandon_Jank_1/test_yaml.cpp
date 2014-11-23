#include <yaml-cpp/yaml.h>
#include <stdio.h>
#include <string>

int main ()
{
	YAML::Node config = YAML::LoadFile("test.yaml");

	printf("Attempting to configure by string\n");

	const std::string username = config["username"].as<std::string>();
	const std::string password = config["password"].as<std::string>();

	printf("Username: %s\n", username.c_str());
	printf("Password: %s\n", password.c_str());

	return 0;
}
