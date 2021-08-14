#pragma once
#include <random>
#include <chrono>
#include <glm.hpp>

/* 
 * Todo: Using chrono get time from system clock
 * and use it as the seed for the generator
 */

float standardNormal();
float uniform();
glm::vec3 UniformDirection();
glm::vec3 UniformPosition();