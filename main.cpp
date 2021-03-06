﻿//#include <gl/GL.h>
#include <vtk_glew.h>
#include <vtkOpenGLRenderUtilities.h>
#include <vtkOpenGL.h>
#include <vtkRenderState.h>
#include <iostream>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkMapper.h>
#include <vtkProp3D.h>
#include <array>
#include <vtkViewport.h>
#include <vtkInformation.h>
#include <vtkOpenGL.h>
#include <vtkOpenGLRenderer.h>
#include <vtkWin32OpenGLRenderWindow.h>
#include <vtkRenderPass.h>
#include <vtkSequencePass.h>
#include <vtkRenderPassCollection.h>
#include <gl/GLU.h>
#include <memory>
#include <sstream>
#include <vector>
#include "shader.h"
#include <vtkMatrix4x4.h>
using namespace std;
class myCustomMapper :public vtkMapper
{
private:
	myCustomMapper()
	{

	}
	virtual ~myCustomMapper()
	{

	}
public:
	static myCustomMapper* New()
	{
		return new myCustomMapper();
	}

	void Render(vtkRenderer* ren, vtkActor* a) override
	{
		std::cout << "foo" << std::endl;
	}
};
///////COM ISSO AQUI EU CONSIGO CAPTURAR OS EVENTOS DE RENDERIZAǃO NO
//////RENDER_VOLUMETRIC_GEOMETRY E RENDER_OVERLAY
class myCustomProp3d :public vtkProp3D
{
private:
	std::array<double, 6> bounds;
	vtkSmartPointer<myCustomMapper> mapper;
	bool alredyInitialized;
	GLuint vbo, colorsVbo, vao;//O vertex array object e seu vertex buffer object e vertex color object
	std::unique_ptr<my3d::Shader> shader;
	vector<GLfloat> vertexes, colors;
	myCustomProp3d()
	{
		bounds = { { -0.5, 0.5, -0.5, -0.5, -0.5, 0.5 } };
		vao = 0; colorsVbo = 0; vbo = 0;
		alredyInitialized = false;
		shader = nullptr;
	}

	template <typename T>const GLuint CreateBuffer(GLenum target, std::vector<T> &vec)
	{
		GLuint resultBuffer = 0;
		glGenBuffers(1, &resultBuffer);
		glBindBuffer(target, resultBuffer);
		glBufferData(target, vec.size() * sizeof(vec.at(0)), vec.data(), GL_STATIC_DRAW);
		return resultBuffer;
	}
	stringstream defineVertexShader()
	{
		stringstream ss;
		ss << "#version 400" << endl;
		ss << "layout (location = 0) in vec3 vertex;" << endl;
		ss << "layout (location = 1) in vec3 color;" << endl;
		ss << "out vec3 vertexColor;" << endl;
		ss << "void main(){" << endl;
		ss << "  gl_Position = vec4(vertex, 1.0);" << endl;
		ss << "  vertexColor = color;" << endl;
		ss << "}";
		return ss;
	}
	stringstream defineFragmentShader()
	{
		stringstream ss;
		ss << "#version 400" << endl;
		ss << "in vec3 vertexColor;" << endl;
		ss << "out vec4 finalColor;" << endl;
		ss << "void main(){" << endl;
		ss << "    finalColor = vec4(vertexColor, 1.0);" << endl;
		ss << "}";
		return ss;
	}
	vector<GLfloat> defineVertexes()
	{
		vector<GLfloat> v;
		v.push_back(-0.5); v.push_back(-0.5); v.push_back(0);
		v.push_back(0.5); v.push_back(-0.5); v.push_back(0);
		v.push_back(-0.5); v.push_back(0.5); v.push_back(0);
		return v;
	}
	vector<GLfloat> defineColors()
	{
		vector<GLfloat> v;
		v.push_back(0.0); v.push_back(0.0); v.push_back(0.0);
		v.push_back(1.0); v.push_back(0.0); v.push_back(0.0);
		v.push_back(1.0); v.push_back(1.0); v.push_back(0.0);
		return v;
	}
	//vector<GLfloat> defineVertexes()
	//{
	//	vector<GLfloat> v;
	//	v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0); //IEF
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0); //IDF
	//	v.push_back(-1.0); v.push_back(1.0); v.push_back(1.0); //SEF
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0); //IDF
	//	v.push_back(1.0); v.push_back(1.0); v.push_back(1.0); //SDF
	//	v.push_back(-1.0); v.push_back(1.0);  v.push_back(1.0);//SEF
	//	//--------
	//	v.push_back(-1.0); v.push_back(-1.0); v.push_back(-1.0); //IEB
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0); //IDB
	//	v.push_back(-1.0); v.push_back(1.0); v.push_back(-1.0); //SEB
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0); //IDB
	//	v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0); //SDB
	//	v.push_back(-1.0); v.push_back(1.0);  v.push_back(-1.0);//SEB
	//	//--------
	//	v.push_back(-1.0); v.push_back(-1.0); v.push_back(-1.0);  //IEB
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0);  //IDB
	//	v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0);  //IES
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0);   //IDB
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0);  //SDB
	//	v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0); //SEB
	//	//--------
	//	v.push_back(-1.0); v.push_back(1.0); v.push_back(-1.0);  //IEB
	//	v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0);  //IDB
	//	v.push_back(-1.0); v.push_back(1.0); v.push_back(1.0);  //IES
	//	v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0);   //IDB
	//	v.push_back(1.0); v.push_back(1.0); v.push_back(1.0);  //SDB
	//	v.push_back(-1.0); v.push_back(1.0); v.push_back(1.0); //SEB
	//	//------
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0);  //IEF
	//	v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0);  //IDF
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0);  //SEF
	//	v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0); //IDF
	//	v.push_back(1.0); v.push_back(1.0); v.push_back(1.0); //SDF
	//	v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0); //SEF
	//	//------
	//	v.push_back(-1.0); v.push_back(-1.0); v.push_back(-1.0);  //IEF
	//	v.push_back(-1.0); v.push_back(1.0); v.push_back(-1.0);  //IDF
	//	v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0);  //SEF
	//	v.push_back(-1.0); v.push_back(1.0); v.push_back(-1.0);  //IDF
	//	v.push_back(-1.0); v.push_back(1.0); v.push_back(1.0);  //SDF
	//	v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0); //SEF
	//	return v;
	//}
	//vector<GLfloat> defineColors()
	//{
	//	vector<GLfloat> vec;
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
	//	vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
	//	return vec;
	//}
	//vector<GLfloat> defineNormals()
	//{
	//	vector<GLfloat> v;
	//	//-----------------1
	//	v.push_back(0); v.push_back(0); v.push_back(1);
	//	v.push_back(0); v.push_back(0); v.push_back(1);
	//	v.push_back(0); v.push_back(0); v.push_back(1);
	//	v.push_back(0); v.push_back(0); v.push_back(1);
	//	v.push_back(0); v.push_back(0); v.push_back(1);
	//	v.push_back(0); v.push_back(0); v.push_back(1);
	//	//-----------------2
	//	v.push_back(0); v.push_back(0); v.push_back(-1);
	//	v.push_back(0); v.push_back(0); v.push_back(-1);
	//	v.push_back(0); v.push_back(0); v.push_back(-1);
	//	v.push_back(0); v.push_back(0); v.push_back(-1);
	//	v.push_back(0); v.push_back(0); v.push_back(-1);
	//	v.push_back(0); v.push_back(0); v.push_back(-1);
	//	//-----------------3
	//	v.push_back(0); v.push_back(-1); v.push_back(0);
	//	v.push_back(0); v.push_back(-1); v.push_back(0);
	//	v.push_back(0); v.push_back(-1); v.push_back(0);
	//	v.push_back(0); v.push_back(-1); v.push_back(0);
	//	v.push_back(0); v.push_back(-1); v.push_back(0);
	//	v.push_back(0); v.push_back(-1); v.push_back(0);
	//	//-----------------4
	//	v.push_back(0); v.push_back(1); v.push_back(0);
	//	v.push_back(0); v.push_back(1); v.push_back(0);
	//	v.push_back(0); v.push_back(1); v.push_back(0);
	//	v.push_back(0); v.push_back(1); v.push_back(0);
	//	v.push_back(0); v.push_back(1); v.push_back(0);
	//	v.push_back(0); v.push_back(1); v.push_back(0);
	//	//-----------------5
	//	v.push_back(1); v.push_back(0); v.push_back(0);
	//	v.push_back(1); v.push_back(0); v.push_back(0);
	//	v.push_back(1); v.push_back(0); v.push_back(0);
	//	v.push_back(1); v.push_back(0); v.push_back(0);
	//	v.push_back(1); v.push_back(0); v.push_back(0);
	//	v.push_back(1); v.push_back(0); v.push_back(0);
	//	//-----------------6
	//	v.push_back(-1); v.push_back(0); v.push_back(0);
	//	v.push_back(-1); v.push_back(0); v.push_back(0);
	//	v.push_back(-1); v.push_back(0); v.push_back(0);
	//	v.push_back(-1); v.push_back(0); v.push_back(0);
	//	v.push_back(-1); v.push_back(0); v.push_back(0);
	//	v.push_back(-1); v.push_back(0); v.push_back(0);
	//	return v;
	//}
	//
	//vector<GLfloat> defineTexCoords()
	//{
	//	vector<GLfloat> texCoords;
	//	texCoords.push_back(0.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	texCoords.push_back(0.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(1.0f);
	//	texCoords.push_back(1.0f); texCoords.push_back(0.0f);
	//	return texCoords;
	//}
	void Init()
	{
		GLenum err = GL_NO_ERROR;
		//Criação do shader
		shader = make_unique<my3d::Shader>(defineVertexShader(), defineFragmentShader());

		err = glGetError(); if (err != GL_NO_ERROR) throw std::exception("erro");
		//2)Criar o cubo de suporte
		vertexes = defineVertexes();
		vbo = CreateBuffer<float>(GL_ARRAY_BUFFER, vertexes);
		colors = defineColors();
		colorsVbo = CreateBuffer<float>(GL_ARRAY_BUFFER, colors);

		err = glGetError(); if (err != GL_NO_ERROR) throw std::exception("erro");

		vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		shader->UseProgram();
		GLuint vpLocation = shader->GetAttribute("vertex");
		GLuint colorLocation = shader->GetAttribute("color");

		glEnableVertexAttribArray(vpLocation);
		glEnableVertexAttribArray(colorLocation);

		glUseProgram(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(vpLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, colorsVbo);
		glVertexAttribPointer(colorLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		err = glGetError(); if (err != GL_NO_ERROR) throw std::exception("erro");

		alredyInitialized = true;
	}
public:
	void SetMapper(vtkSmartPointer<myCustomMapper> m)
	{
		this->mapper = m;
	}
	vtkRenderWindow *LastWindow;
	static myCustomProp3d* New()
	{
		return new myCustomProp3d();
	}
	int RenderTranslucentPolygonalGeometry(vtkViewport *view)
	{
		std::cout << __FUNCTION__ << std::endl;
		return 0;
	}

	void vtkMatrixToFloat(vtkSmartPointer<vtkMatrix4x4> src, array<float, 16>& dest)
	{
		array<double, 16> asDouble;
		vtkMatrix4x4::DeepCopy(asDouble.data(), src);
		for (auto i = 0; i < 16; i++){
			dest[i] = asDouble[i];
		}
	}

	shared_ptr<my3d::Shader> testeShader;

	void InitVer2()
	{
		//Cria o shader
		stringstream ssVs;
		ssVs << "#version 400" << endl;
		ssVs << "in vec3 vp;" << endl;
		ssVs << "uniform mat4 mvp;" << endl;
		ssVs << "void main() {" << endl;
		ssVs << "  gl_Position = mvp * vec4(vp, 1.0);" << endl;
		ssVs << "}" << endl;
		stringstream ssFs;
		ssFs<<"#version 400" << endl;
		ssFs << "out vec4 frag_colour;" << endl;
		ssFs << "void main() {" << endl;
		ssFs << "  frag_colour = vec4(0.5, 0.0, 0.5, 1.0);" << endl;
		ssFs << "}" << endl;
		testeShader = make_shared<my3d::Shader>(ssVs, ssFs);

		GLenum err = glGetError();

		//Cria a geometria
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		float points[] = {
			0.0f, 0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			-0.5f, -0.5f, 0.0f
		};
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), points, GL_STATIC_DRAW);
		err = glGetError();

		glUseProgram(testeShader->GetProgramId());
		;
		//Linka o vbo ao attrib 0
		glEnableVertexAttribArray(testeShader->GetAttribute("vp"));
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(testeShader->GetAttribute("vp"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

		alredyInitialized = true;
	}

	int RenderVolumetricGeometry(vtkViewport *view)
	{
		std::cout << __FUNCTION__ << std::endl;
		if (!alredyInitialized)
			InitVer2();

		GLenum err = glGetError();

		err = glGetError();

		// wipe the drawing surface clear

		//Comeca a usar o shader
		glUseProgram(testeShader->GetProgramId());
		glBindVertexArray(vao);//Estou trabalhando com os vertices do vao
		//Passo a matriz pro shader
		double __dm[16];
		vtkMatrix4x4::DeepCopy(__dm, vtkRenderer::SafeDownCast(view)->GetActiveCamera()->GetModelViewTransformMatrix());
		float _fm[16]; for (int i = 0; i < 16; i++) _fm[i] = __dm[i];
		glUniformMatrix4fv(testeShader->GetUniform("mvp"), 1, false, _fm);

		// draw points 0-3 from the currently bound VAO with current in-use shader
		glDrawArrays(GL_TRIANGLES, 0, 3);
		err = glGetError();

		return 0;
	}

	int RenderOverlay(vtkViewport *)
	{
		std::cout << __FUNCTION__ << std::endl;
		return 0;
	}

	bool RenderFilteredOpaqueGeometry(vtkViewport *v, vtkInformation *requiredKeys)
	{
		std::cout << __FUNCTION__ << std::endl;


		return false;
	}

	bool RenderFilteredTranslucentPolygonalGeometry(vtkViewport *v, vtkInformation *requiredKeys)
	{
		std::cout << __FUNCTION__ << std::endl;
		return false;
	}


	bool RenderFilteredVolumetricGeometry(vtkViewport *v, vtkInformation *requiredKeys)
	{
		std::cout << __FUNCTION__ << std::endl;
		return false;
	}
	bool RenderFilteredOverlay(vtkViewport *v, vtkInformation *requiredKeys)
	{
		std::cout << __FUNCTION__ << std::endl;
		return false;
	}

	double* GetBounds() override
	{
		return bounds.data();
	}
};

class myRenderPassExperimento : public vtkRenderPass
{
public:
	void Render(const vtkRenderState *s) override
	{
		glClearDepth(1.0f);									// Depth Buffer Setup
		glEnable(GL_DEPTH_TEST); // enable depth-testing
		glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		const int propCount = s->GetPropArrayCount();
		for (auto i = 0; i < propCount; i++)
		{
			s->GetPropArray()[i]->RenderVolumetricGeometry(s->GetRenderer()); //OpaqueGeometry(s->GetRenderer());
		}
	}
	static myRenderPassExperimento* New()
	{
		return new myRenderPassExperimento();
	}
};

int main(int argc, char** argv)
{
	vtkSmartPointer<myCustomMapper> myMapper = vtkSmartPointer<myCustomMapper>::New();
	vtkSmartPointer<myCustomProp3d> myProp3d = vtkSmartPointer<myCustomProp3d>::New();
	myProp3d->SetMapper(myMapper);


	//Como se renderiza algo na tela com VTK? Como minhas rotinas de desenho encaixar㯠nas rotinas da vtk?
	//Cria a tela
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->SetBackground(1, 0, 0);
	renderer->AddViewProp(myProp3d);
	renderer->GetActiveCamera()->ParallelProjectionOn();
	vtkSmartPointer<myRenderPassExperimento> myRP = vtkSmartPointer<myRenderPassExperimento>::New();
	vtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(myRP);
	vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	renderWindow->SetSize(640, 480);
	renderWindow->AddRenderer(renderer);
	renderWindow->SwapBuffersOn();
	renderWindow->Render();

	vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	renderWindowInteractor->SetRenderWindow(renderWindow);
	renderer->ResetCamera();
	renderWindowInteractor->Start();

	return EXIT_SUCCESS;
}