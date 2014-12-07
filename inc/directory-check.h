/* Copyright (c) 2014 UCR-UAS
 * You should have recieved a copy of the MIT licence with this file.
 * Engineer: Brandon Lu
 * Description: This is a directory checking function
 * What dose it do? Create all directories to a path.
 */

#ifndef __BISON_DIRECTORY_CHECK__
#define __BISON_DIRECTORY_CHECK__
// ====== Directory Check and Create =======
void dirChkCreate(const char* const directory, const char* const HR_directory)
{
#if DEBUG
	std::cout << "Checking if the " << HR_directory << " directory exists."
		<< std::endl;
#endif // if DEBUG
	boost::filesystem::path dir_chk(directory);
	if (!boost::filesystem::exists(dir_chk)) {
#if DEBUG
		printf("%s directory does not exist.  Creating...", HR_directory);
		if (boost::filesystem::create_directories(dir_chk)) {
			std::cout << "Created." << std::endl;
		} else {
			std::cout << "Not created? continuing." << std::endl;
		}
#else
		boost::filesystem::create_directories(dir_chk);
#endif
	}
}

// =========== Write Out Config ============
void writeOutYaml(YAML::Node& yaml_config, const std::string& file)
{
	std::ofstream fout(file);
	fout << "%YAML 1.2\n---\n";				// print out yaml version string
	fout << yaml_config;					// print out our YAML
}

#endif // ifndef __BISON_DIRECTORY_CHECK__
