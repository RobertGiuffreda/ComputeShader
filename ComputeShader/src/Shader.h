#pragma once

#include <string>
#include <unordered_map>
#include <glm.hpp>

class Shader
{
private:
	unsigned int id;
	std::unordered_map<std::string, int> uniform_location_cache;
public:
	Shader(const std::string& vertpath, const std::string& fragpath);
	~Shader();

	void Bind() const;
	void Unbind() const;

	void SetUniform1i(const std::string& name, int value);
	void SetUniform1f(const std::string& name, float value);
	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void SetUniformVec3(const std::string& name, glm::vec3 vec);
	void SetUniformVec4(const std::string& name, glm::vec4 vec);
	void SetUniformMat3(const std::string& name, glm::mat3& mat);
	void SetUniformMat4(const std::string& name, glm::mat4& mat);
private:
	int GetUniformLocation(const std::string& name);
	void CheckCompileErrors(unsigned int shader, std::string type);
};