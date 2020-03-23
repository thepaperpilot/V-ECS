// Source: https://gist.github.com/podgorskiy/e698d18879588ada9014768e3e82a644
#pragma once

#include <glm/matrix.hpp>

class Frustum
{
public:
	Frustum() {}

	// m = ProjectionMatrix * ViewMatrix 
	Frustum(glm::mat4 m);

	// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
	bool IsBoxVisible(const glm::vec3& minp, const glm::vec3& maxp) const;

private:
	enum Planes
	{
		Left = 0,
		Right,
		Bottom,
		Top,
		Near,
		Far,
		Count,
		Combinations = Count * (Count - 1) / 2
	};

	glm::vec4   m_planes[Count];
};

inline Frustum::Frustum(glm::mat4 m)
{
	m = glm::transpose(m);
	m_planes[Left] = m[3] + m[0];
	m_planes[Right] = m[3] - m[0];
	m_planes[Bottom] = m[3] + m[1];
	m_planes[Top] = m[3] - m[1];
	m_planes[Near] = m[3] + m[2];
	m_planes[Far] = m[3] - m[2];
}

// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
// Modified due to false culling
inline bool Frustum::IsBoxVisible(const glm::vec3& minp, const glm::vec3& maxp) const
{
	// check box outside/inside of frustum
	for (int i = 0; i < Count; i++)
	{
		if ((glm::dot(m_planes[i], glm::vec4(minp.x, minp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(maxp.x, minp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, maxp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(maxp.x, maxp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, minp.y, maxp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(maxp.x, minp.y, maxp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, maxp.y, maxp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(maxp.x, maxp.y, maxp.z, 1.0f)) < 0.0))
		{
			return false;
		}
	}

	return true;
}
