#include <general/Exception.h>


Exception::Exception(std::string description, std::string file, int line) {
	this->description = description;
	if (file != "") {
		char intBuffer[100];
		sprintf(intBuffer, ":%d)", line);
		
		this->description.append("(");
		this->description.append(file);
		this->description.append(intBuffer);
	}
}

/**
 *
 */
const char* Exception::what() const throw() {
	return description.c_str();
}

