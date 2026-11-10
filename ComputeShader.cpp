#include "ComputeShader.h"

ComputeShader::ComputeShader(const char* computeshader){
	std::string Compute = Shader::get_file_contents(computeshader);
	const char* Compute_Source = Compute.c_str();
    // Vertex
    unsigned int ComputeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(ComputeShader, 1, &Compute_Source, NULL);
    glCompileShader(ComputeShader);
    checkCompileErrors(ComputeShader, "COMPUTE");
    
    
    ID = glCreateProgram();
    glAttachShader(ID, ComputeShader);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(ComputeShader);
}

ComputeShader::~ComputeShader()
{
    //glUseProgram(0);
}
