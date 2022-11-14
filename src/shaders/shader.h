#ifndef SHADER_H
#define SHADER_H

#include <bits/stdc++.h>
#include <glm/glm.hpp>

class Shader
{
    private:
        void checkCompileErrors(unsigned int shader, std::string type);
    public:
        unsigned int ID;

        Shader(const char* vertexPath, const char* fragmentPath);

        void bindAttribLocation(unsigned int position, const char* variable_name);

        void link();
        void use();

        void setBool(const std::string &name, bool value) const;  
        void setInt(const std::string &name, int value) const;   
        void setFloat(const std::string &name, float value) const;

        void setMat4(const std::string &name, const glm::mat4 &mat) const;
};

#endif
