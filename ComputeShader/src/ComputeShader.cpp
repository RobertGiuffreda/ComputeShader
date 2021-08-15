#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Renderer.h"
#include "ComputeShader.h"

ComputeShader::ComputeShader(const std::string& filepath)
{
    std::string compSource;
    std::ifstream cFileStream(filepath);
    std::stringstream cStream;

    cStream << cFileStream.rdbuf();
    cFileStream.close();

    compSource = cStream.str();

    const char* cSource = compSource.c_str();

    unsigned int cShader;
    cShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cShader, 1, &cSource, nullptr);
    glCompileShader(cShader);
    CheckCompileErrors(cShader, "COMPUTE");

    id = glCreateProgram();
    glAttachShader(id, cShader);
    glLinkProgram(id);
    CheckCompileErrors(id, "PROGRAM");

    glDeleteShader(cShader);
}

ComputeShader::~ComputeShader()
{
	glDeleteShader(id);
}

void ComputeShader::Bind() const
{
    glUseProgram(id);
}

void ComputeShader::Unbind() const
{
    glUseProgram(0);
}

void ComputeShader::SetUniform1i(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void ComputeShader::SetUniform1f(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

int ComputeShader::GetUniformLocation(const std::string& name)
{
    int location = glGetUniformLocation(id, name.c_str());
    if (location == -1)
        std::cout << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;

    // uniform_location_cache[name] = location;
    return location;
}

void ComputeShader::CheckCompileErrors(unsigned int shader, std::string type)
{
    int success;
    char infolog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infolog);
            std::cout << "ERROR: SHADER_COMPILATION_ERROR of type: " << type << "\n" << infolog << "\n ------" << std::endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infolog);
            std::cout << "ERROR: PROGRAM_LINKING_ERROR of type: " << type << "\n" << infolog << "\n ------" << std::endl;
        }
    }
}
