#include "Random.h"

std::mt19937 generator((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
std::normal_distribution<float> normalDist(0.0f, 1.0f);
std::uniform_real_distribution<float> uniformDist(0.0f, 1.0f);

float standardNormal()
{
	return normalDist(generator);
}

float uniform()
{
	return uniformDist(generator);
}

/*
* Could also do standard normal spherical coordinate
* angles and normalize for uniform distibution on
* spheres surface area
*
* In regards to position on unit sphere's surface.
*/
glm::vec3 UniformDirection()
{
	return normalize(glm::vec3(standardNormal(), standardNormal(), standardNormal()));
}

/*
* In regards to position inside of unit sphere.
*/
glm::vec3 UniformPosition()
{
	return UniformDirection() * glm::pow(uniform(), (1.0f / 3.0f));
}