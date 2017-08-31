#include "shader.h"
#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;
namespace my3d
{
	std::string Shader::ReadShaderFile(std::string path)const
	{
		std::string VertexShaderCode;
		std::ifstream VertexShaderStream(path.c_str(), std::ios::in);
		if (VertexShaderStream.is_open()) {
			std::string Line = "";
			while (getline(VertexShaderStream, Line))
				VertexShaderCode += "\n" + Line;
			VertexShaderStream.close();
		}
		else
		{
			throw std::exception("arquivo não foi aberto");
		}
		std::cout << VertexShaderCode << std::endl;
		return VertexShaderCode;
	}
	void Shader::introspectProgram(GLuint programId, map<string, GLuint>& attributes, map<string, GLuint>& uniforms)
	{
		GLint numberOfAttributes, largestAttributeName;
		glGetProgramiv(programId, GL_ACTIVE_ATTRIBUTES, &numberOfAttributes);
		glGetProgramiv(programId, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &largestAttributeName);
		//para cada atributo, pegar as propriedades e guardar.
		for (GLint i = 0; i < numberOfAttributes; i++)
		{
			char* nameBuffer = new char[largestAttributeName];
			GLsizei length;
			GLint size;
			GLenum type;
			//Pega, entre outras coisas, o nome do atributo.
			glGetActiveAttrib(programId, i, largestAttributeName, &length, &size, &type, nameBuffer);
			GLint attribLocation = glGetAttribLocation(programId, nameBuffer);
			std::string _name(nameBuffer);
			attributes.insert(std::make_pair(_name, attribLocation));
			std::cout << _name << std::endl;
		}
		//Agora pra uniforms
		GLint numberOfUniforms, largesUniformName;
		glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &numberOfUniforms);
		glGetProgramiv(programId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &largesUniformName);
		//para cada atributo, pegar as propriedades e guardar.
		for (GLint i = 0; i < numberOfUniforms; i++)
		{
			char* buffer = new char[largesUniformName];
			GLsizei length;
			GLint size;
			GLenum type;
			glGetActiveUniform(programId, i, largesUniformName, &length,
				&size, &type, buffer);
			GLint unifLoc = glGetUniformLocation(programId, buffer);
			std::string _name(buffer);
			uniforms.insert(std::make_pair(_name, unifLoc));
			std::cout << _name << std::endl;
		}
	}

	string Shader::GetShaderInfoLog(GLuint object, PFNGLGETSHADERIVPROC glGet__iv, PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
	{
		GLint log_length;
		char *log;
		glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
		log = static_cast<char*>(malloc(log_length));
		glGet__InfoLog(object, log_length, nullptr, log);
		return log;
	}

	GLuint Shader::MakeShader(GLenum type, std::string source)
	{
		GLint length[] = { source.length() };
		const GLchar *srcLn = source.c_str();
		GLuint shader;
		GLint shader_ok;
		shader = glCreateShader(type);
		glShaderSource(shader, 1, &srcLn, length);
		glCompileShader(shader);
		glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
		if (!shader_ok)
		{
			stringstream ss;
			string _probl = GetShaderInfoLog(shader, glGetShaderiv, glGetShaderInfoLog);
			ss << "Erro em " << __FUNCTION__ << " : " << _probl;
			glDeleteShader(shader);
			throw std::runtime_error(ss.str().c_str());
		}
		else
		{
			return shader;
		}
	}

	GLuint Shader::MakeProgram(GLuint vertex_shader, GLuint fragment_shader)
	{
		GLint program_ok;
		GLuint program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, fragment_shader);
		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
		if (!program_ok) {
			string _probl = GetShaderInfoLog(program, glGetProgramiv, glGetProgramInfoLog);
			glDeleteProgram(program);
			throw std::runtime_error(_probl.c_str());
		}
		return program;
	}

	Shader::Shader(string vsPath, string fsPath)
	{
		std::string vsSrc = ReadShaderFile(vsPath);
		vsId = MakeShader(GL_VERTEX_SHADER, vsSrc);
		std::string fsSrc = ReadShaderFile(fsPath);
		fsId = MakeShader(GL_FRAGMENT_SHADER, fsSrc);
		programId = MakeProgram(vsId, fsId);
		introspectProgram(programId, attributes, uniforms);
	}

	Shader::Shader(stringstream vsStream, stringstream fsStream)
	{
		std::string vsSrc = vsStream.str();
		vsId = MakeShader(GL_VERTEX_SHADER, vsSrc);
		std::string fsSrc = fsStream.str();
		fsId = MakeShader(GL_FRAGMENT_SHADER, fsSrc);
		programId = MakeProgram(vsId, fsId);
		introspectProgram(programId, attributes, uniforms);
	}

	Shader::~Shader()
	{
		glDeleteShader(vsId);
		glDeleteShader(fsId);
		glDeleteShader(programId);
	}

	GLuint Shader::GetUniform(string name)
	{
		return uniforms.at(name);
	}

	GLuint Shader::GetAttribute(string name)
	{
		return attributes.at(name);
	}

	GLuint Shader::GetProgramId()const
	{
		return programId;
	}

	void Shader::UseProgram()const
	{
		glUseProgram(this->programId);
	}

}