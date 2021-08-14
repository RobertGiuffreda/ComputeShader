#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Renderer.h"
#include "Shader.h"

Shader::Shader(const std::string& vertpath, const std::string& fragpath)
{
    std::string vertexSource;
    std::string fragmentSource;
    std::ifstream vFileStream(vertpath);
    std::ifstream fFileStream(fragpath);
    
    std::stringstream vStream;
    std::stringstream fStream;

    vStream << vFileStream.rdbuf();
    fStream << fFileStream.rdbuf();

    vFileStream.close();
    fFileStream.close();

    vertexSource = vStream.str();
    fragmentSource = fStream.str();

    const char* vSource = vertexSource.c_str();
    const char* fSource = fragmentSource.c_str();

    unsigned int vShader, fShader;

    vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vSource, nullptr);
    glCompileShader(vShader);
    CheckCompileErrors(vShader, "VERTEX");

    fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fSource, nullptr);
    glCompileShader(fShader);
    CheckCompileErrors(fShader, "FRAGMENT");

    id = glCreateProgram();
    glAttachShader(id, vShader);
    glAttachShader(id, fShader);
    glLinkProgram(id);
    CheckCompileErrors(id, "PROGRAM");

    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

Shader::~Shader()
{
    glDeleteProgram(id);
}

void Shader::Bind() const
{
    glUseProgram(id);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

void Shader::SetUniform1i(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetUniform1f(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void Shader::SetUniformVec3(const std::string& name, glm::vec3 vec)
{
    glUniform3fv(GetUniformLocation(name), 1, &vec[0]);
}

void Shader::SetUniformVec4(const std::string& name, glm::vec4 vec)
{
    glUniform4fv(GetUniformLocation(name), 1, &vec[0]);
}

void Shader::SetUniformMat3(const std::string& name, glm::mat3& mat)
{
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetUniformMat4(const std::string& name, glm::mat4& mat)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

int Shader::GetUniformLocation(const std::string& name)
{
    int location = glGetUniformLocation(id, name.c_str());
    if (location == -1)
        std::cout << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;

    // uniform_location_cache[name] = location;
    return location;
}

void Shader::CheckCompileErrors(unsigned int shader, std::string type)
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
