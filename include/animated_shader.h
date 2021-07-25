#ifndef ANIMATED_SHADER_H
#define	ANIMATED_SHADER_H

#include <sstream>

#include "math_utils.h"
#include "static_shader.h"

using std::stringstream;

class AnimatedShader : public StaticShader  {
public:
    static const unsigned int MAX_BONES = 50;
    AnimatedShader(){};
    AnimatedShader(const char* vertexPath, const char* fragmentPath);
    int setup(const char* vertexPath, const char* fragmentPath);
    void SetBoneTransform(unsigned int uniqueID, unsigned int Index, const mat4& Transform);

private:
	vector<vector<mat4> > bonesMatrix;
    GLuint m_boneLocation[20][MAX_BONES];
};


#endif	/* ANIMATED_SHADER_H */
