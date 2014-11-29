/* This is a buffer test for boost::circular_buffer.
 */
#include <boost/circular_buffer.hpp>
#include <iostream>
#include <string>

int main ()
{
	std::cout << "Allocating buffer." << std::endl;
	boost::circular_buffer_space_optimized<char> buffer(64 * 1024);
	const char* const c_string = "I am a cat.";
	std::cout << "Putting string into buffer: " << std::endl;
	for (const char* c = c_string; *c; c++) {
		std::cout << *c << std::flush;
		buffer.push_back(*c);
	}
	std::cout << std::endl;
	while(!buffer.empty()) {
		std::cout << buffer[0] << std::flush;
		buffer.pop_front();
	}
	std::cout << std::endl;
}
