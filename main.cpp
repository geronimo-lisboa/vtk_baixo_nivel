//#include <gl/GL.h>
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
	GLuint vertexesVbo, colorsVbo, tcVbo, normalVbo, vao;//O vertex array object e seu vertex buffer object e vertex color object
	std::unique_ptr<my3d::Shader> shader;
	vector<GLfloat> vertexes, colors, texCoords, normalCoords;
	myCustomProp3d()
	{
		bounds = { { 0, 1, 0, 1, 0, 1 } };
		vao = 0; colorsVbo = 0; vertexesVbo = 0; normalVbo = 0;
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
		ss << "layout (location = 2) in vec2 texCoord;" << endl;
		ss << "layout (location = 3) in vec3 normal;" << endl;
		ss << "uniform mat4 modelMat;" << endl;
		ss << "uniform mat4 cameraMat;" << endl;
		ss << "out vec3 vertexColor;" << endl;
		ss << "out vec2 vertexTexCoord;" << endl;
		ss << "out vec3 fragNormal;" << endl;
		ss << "out vec3 fragVert;" << endl;
		ss << "void main(){" << endl;
		ss << "  gl_Position = cameraMat * modelMat * vec4(vertex, 1.0);" << endl;
		ss << "  vertexColor = color;" << endl;
		ss << "  vertexTexCoord = texCoord;" << endl;
		ss << "  fragNormal = normal;" << endl;
		ss << "  fragVert = vertex;" << endl;
		ss << "}";
		return ss;
	}
	stringstream defineFragmentShader()
	{
		stringstream ss;
		ss << "#version 400" << endl;
		ss << "layout (location =10) out vec3 color;" << endl;
		ss << "uniform mat4 modelMat;" << endl;
		ss << "uniform sampler2D textureSampler;" << endl;
		ss << "uniform bool isUsandoTextura;" << endl;
		ss << "uniform bool isUsandoIluminacao;" << endl;
		ss << "uniform vec3 lightLocation;" << endl;
		ss << "in vec3 vertexColor;" << endl;
		ss << "in vec2 vertexTexCoord;" << endl;
		ss << "in vec3 fragNormal;" << endl;
		ss << "in vec3 fragVert;" << endl;
		ss << "out vec4 finalColor;" << endl;
		ss << "void main(){" << endl;
		ss << "  if (!isUsandoTextura) {" << endl;
		ss << "    finalColor = vec4(vertexColor, 1.0);" << endl;
		ss << "  }else{" << endl;
		ss << "    finalColor = texture(textureSampler, vertexTexCoord);" << endl;
		ss << "  }" << endl;
		ss << "  if (isUsandoIluminacao){" << endl;
		ss << "    mat3 normalMatrix = transpose(inverse(mat3(modelMat)));" << endl;
		ss << "    vec3 transformedNormal = normalize(normalMatrix * fragNormal);" << endl;//As normais estão em modelspace, isso aqui as leva pra world space
		ss << "    vec3 fragPosition = vec3(modelMat * vec4(fragVert,1));" << endl;//A posição do fragmento atual em worldspace, necessária para calcular o angulo entre a normal do fragmento e a luz.
		ss << "    vec3 surfaceToLight = lightLocation - fragPosition;" << endl;//Vetor do pixel (em wc) pra luz.
		ss << "    float brightness = dot(transformedNormal, surfaceToLight)/ (length(surfaceToLight) * length(transformedNormal));" << endl;// O brilho é o cosseno do angulo entre a normal da superficie e o vetor do fragmento pra luz
		ss << "    finalColor = vec4(finalColor[0]*brightness, finalColor[1]*brightness, finalColor[2]*brightness, 1.0);" << endl;
		ss << "  }" << endl;
		ss << "}";
		return ss;
	}
	vector<GLfloat> defineVertexes()
	{
		vector<GLfloat> v;
		v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0); //IEF
		v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0); //IDF
		v.push_back(-1.0); v.push_back(1.0); v.push_back(1.0); //SEF

		v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0); //IDF
		v.push_back(1.0); v.push_back(1.0); v.push_back(1.0); //SDF
		v.push_back(-1.0); v.push_back(1.0);  v.push_back(1.0);//SEF
		//--------
		v.push_back(-1.0); v.push_back(-1.0); v.push_back(-1.0); //IEB
		v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0); //IDB
		v.push_back(-1.0); v.push_back(1.0); v.push_back(-1.0); //SEB

		v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0); //IDB
		v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0); //SDB
		v.push_back(-1.0); v.push_back(1.0);  v.push_back(-1.0);//SEB
		//--------
		v.push_back(-1.0); v.push_back(-1.0); v.push_back(-1.0);  //IEB
		v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0);  //IDB
		v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0);  //IES

		v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0);   //IDB
		v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0);  //SDB
		v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0); //SEB
		//--------
		v.push_back(-1.0); v.push_back(1.0); v.push_back(-1.0);  //IEB
		v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0);  //IDB
		v.push_back(-1.0); v.push_back(1.0); v.push_back(1.0);  //IES

		v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0);   //IDB
		v.push_back(1.0); v.push_back(1.0); v.push_back(1.0);  //SDB
		v.push_back(-1.0); v.push_back(1.0); v.push_back(1.0); //SEB
		//------
		v.push_back(1.0); v.push_back(-1.0); v.push_back(-1.0);  //IEF
		v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0);  //IDF
		v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0);  //SEF

		v.push_back(1.0); v.push_back(1.0); v.push_back(-1.0); //IDF
		v.push_back(1.0); v.push_back(1.0); v.push_back(1.0); //SDF
		v.push_back(1.0); v.push_back(-1.0); v.push_back(1.0); //SEF
		//------
		v.push_back(-1.0); v.push_back(-1.0); v.push_back(-1.0);  //IEF
		v.push_back(-1.0); v.push_back(1.0); v.push_back(-1.0);  //IDF
		v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0);  //SEF

		v.push_back(-1.0); v.push_back(1.0); v.push_back(-1.0);  //IDF
		v.push_back(-1.0); v.push_back(1.0); v.push_back(1.0);  //SDF
		v.push_back(-1.0); v.push_back(-1.0); v.push_back(1.0); //SEF
		return v;
	}
	vector<GLfloat> defineColors()
	{
		vector<GLfloat> vec;
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(0.0);

		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(0.0);

		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(0.0);

		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(1.0); vec.push_back(1.0);

		vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(1.0); vec.push_back(1.0);

		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(0.0); vec.push_back(0.0); vec.push_back(1.0);

		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);
		vec.push_back(1.0); vec.push_back(0.0); vec.push_back(1.0);

		return vec;
	}
	vector<GLfloat> defineNormals()
	{
		vector<GLfloat> v;
		//-----------------1
		v.push_back(0); v.push_back(0); v.push_back(1);
		v.push_back(0); v.push_back(0); v.push_back(1);
		v.push_back(0); v.push_back(0); v.push_back(1);

		v.push_back(0); v.push_back(0); v.push_back(1);
		v.push_back(0); v.push_back(0); v.push_back(1);
		v.push_back(0); v.push_back(0); v.push_back(1);
		//-----------------2
		v.push_back(0); v.push_back(0); v.push_back(-1);
		v.push_back(0); v.push_back(0); v.push_back(-1);
		v.push_back(0); v.push_back(0); v.push_back(-1);

		v.push_back(0); v.push_back(0); v.push_back(-1);
		v.push_back(0); v.push_back(0); v.push_back(-1);
		v.push_back(0); v.push_back(0); v.push_back(-1);
		//-----------------3
		v.push_back(0); v.push_back(-1); v.push_back(0);
		v.push_back(0); v.push_back(-1); v.push_back(0);
		v.push_back(0); v.push_back(-1); v.push_back(0);

		v.push_back(0); v.push_back(-1); v.push_back(0);
		v.push_back(0); v.push_back(-1); v.push_back(0);
		v.push_back(0); v.push_back(-1); v.push_back(0);
		//-----------------4
		v.push_back(0); v.push_back(1); v.push_back(0);
		v.push_back(0); v.push_back(1); v.push_back(0);
		v.push_back(0); v.push_back(1); v.push_back(0);

		v.push_back(0); v.push_back(1); v.push_back(0);
		v.push_back(0); v.push_back(1); v.push_back(0);
		v.push_back(0); v.push_back(1); v.push_back(0);
		//-----------------5
		v.push_back(1); v.push_back(0); v.push_back(0);
		v.push_back(1); v.push_back(0); v.push_back(0);
		v.push_back(1); v.push_back(0); v.push_back(0);

		v.push_back(1); v.push_back(0); v.push_back(0);
		v.push_back(1); v.push_back(0); v.push_back(0);
		v.push_back(1); v.push_back(0); v.push_back(0);
		//-----------------6
		v.push_back(-1); v.push_back(0); v.push_back(0);
		v.push_back(-1); v.push_back(0); v.push_back(0);
		v.push_back(-1); v.push_back(0); v.push_back(0);

		v.push_back(-1); v.push_back(0); v.push_back(0);
		v.push_back(-1); v.push_back(0); v.push_back(0);
		v.push_back(-1); v.push_back(0); v.push_back(0);
		return v;
	}
	vector<GLfloat> defineTexCoords()
	{
		vector<GLfloat> texCoords;
		texCoords.push_back(0.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);

		texCoords.push_back(0.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);

		texCoords.push_back(0.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);

		texCoords.push_back(0.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);

		texCoords.push_back(0.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);

		texCoords.push_back(0.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);
		texCoords.push_back(0.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(1.0f);
		texCoords.push_back(1.0f); texCoords.push_back(0.0f);
		return texCoords;
	}
	void Init()
	{
		GLenum err = GL_NO_ERROR;
		//Criação do shader
		shader = make_unique<my3d::Shader>(defineVertexShader(), defineFragmentShader());

		err = glGetError(); if (err != GL_NO_ERROR) throw std::exception("erro");
		//2)Criar o cubo de suporte
		vertexes = defineVertexes();
		vertexesVbo = CreateBuffer<float>(GL_ARRAY_BUFFER, vertexes);
		colors = defineColors();
		colorsVbo = CreateBuffer<float>(GL_ARRAY_BUFFER, colors);
		texCoords = defineTexCoords();
		tcVbo = CreateBuffer<GLfloat>(GL_ARRAY_BUFFER, texCoords);
		normalCoords = defineNormals();
		normalVbo = CreateBuffer<GLfloat>(GL_ARRAY_BUFFER, normalCoords);

		err = glGetError(); if (err != GL_NO_ERROR) throw std::exception("erro");

		vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		shader->UseProgram();
		GLuint vpLocation = shader->GetAttribute("vertex");
		GLuint colorLocation = shader->GetAttribute("color");
		GLuint texCoordLocation = shader->GetAttribute("texCoord");
		GLuint normalLocation = shader->GetAttribute("normal");
		glEnableVertexAttribArray(vpLocation);
		glEnableVertexAttribArray(colorLocation);
		glEnableVertexAttribArray(texCoordLocation);
		glEnableVertexAttribArray(normalLocation);
		glUseProgram(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexesVbo);
		glVertexAttribPointer(vpLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, colorsVbo);
		glVertexAttribPointer(colorLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, tcVbo);
		glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, normalVbo);
		glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

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

	int RenderVolumetricGeometry(vtkViewport *view)
	{
		std::cout << __FUNCTION__ << std::endl;
		glClearColor(0.8, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		vtkOpenGLRenderer *renderer = vtkOpenGLRenderer::SafeDownCast(view);
		vtkWin32OpenGLRenderWindow *window = vtkWin32OpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
		GLenum err = GL_NO_ERROR;
		window->MakeCurrent();
		//Inicializa as coisas se não estiverem inicializadas.
		if (!alredyInitialized)
		{
			Init();
		}
		err = glGetError(); if (err != GL_NO_ERROR) throw std::exception("erro");
		//Agora a renderização
		//O VP do MVP , multiplicação na ordem P V M
		vtkSmartPointer<vtkMatrix4x4> projMatrix = renderer->GetActiveCamera()->GetProjectionTransformMatrix(renderer);
		vtkSmartPointer<vtkMatrix4x4> viewMatrix = renderer->GetActiveCamera()->GetViewTransformMatrix();
		vtkSmartPointer<vtkMatrix4x4> cameraMat = vtkSmartPointer<vtkMatrix4x4>::New();
		vtkMatrix4x4::Multiply4x4(projMatrix, viewMatrix, cameraMat);
		//A model
		vtkSmartPointer<vtkMatrix4x4> modelMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
		modelMatrix->Identity();
		//Começa a usar o programa
		shader->UseProgram();
		GLuint cameraMatLocation = shader->GetUniform("cameraMat");
		GLuint modelMatLocation = shader->GetUniform("modelMat");
		GLuint isUsandoTexturaLocation = shader->GetUniform("isUsandoTextura");
		GLuint textureSamplerLocation = shader->GetUniform("textureSampler");
		GLuint isUsandoIluminacaoLocation = shader->GetUniform("isUsandoIluminacao");
		GLuint lightLocationLocation = shader->GetUniform("lightLocation");
		array<float, 16> __cameraMat;
		vtkMatrixToFloat(cameraMat, __cameraMat);
		glUniformMatrix4fv(cameraMatLocation, 1, GL_FALSE, __cameraMat.data());
		array<float, 16> __mModelMatrix;
		vtkMatrixToFloat(modelMatrix, __mModelMatrix);
		glUniformMatrix4fv(modelMatLocation, 1, GL_FALSE, __mModelMatrix.data());
		glUniform1i(isUsandoTexturaLocation, false);
		glUniform1i(isUsandoIluminacaoLocation, false);
		glUniform3f(lightLocationLocation, 1.0, 5.0, -5.0);

		err = glGetError(); if (err != GL_NO_ERROR) throw std::exception("erro");
		//Ativação da textura no shader
		//glActiveTexture(GL_TEXTURE0);
		//glUniform1i(textureSamplerLocation, /*GL_TEXTURE*/0);
		//glBindTexture(GL_TEXTURE_2D, textura);
		glBindVertexArray(vao);
		GLuint vertexLocation = shader->GetAttribute("vertex");
		glBindAttribLocation(shader->GetProgramId(), vertexLocation, "vertex");
		GLuint colorLocation = shader->GetAttribute("color");
		glBindAttribLocation(shader->GetProgramId(), colorLocation, "color");
		GLuint texCoordLocation = shader->GetAttribute("texCoord");
		glBindAttribLocation(shader->GetProgramId(), texCoordLocation, "texCoord");
		GLuint normalLocation = shader->GetAttribute("normal");
		glBindAttribLocation(shader->GetProgramId(), normalLocation, "normal");
		glDrawArrays(GL_TRIANGLES, 0, vertexes.size() / 3);

		err = glGetError(); if (err != GL_NO_ERROR) throw std::exception("erro");

		return 1;
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
		const int propCount = s->GetPropArrayCount();
		for (auto i = 0; i < propCount; i++)
		{
			s->GetPropArray()[i]->RenderVolumetricGeometry(s->GetRenderer()); //OpaqueGeometry(s->GetRenderer());
		}

		///////////SӠFUNCIONA NO OPENGL LEGADO////////
		//GLenum error = GL_NO_ERROR;
		//vtkWin32OpenGLRenderWindow *window = vtkWin32OpenGLRenderWindow::SafeDownCast( s->GetRenderer()->GetRenderWindow() );
		//glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
		//glClearDepth(1.0f);									// Depth Buffer Setup
		//glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
		//glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
		//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculat
		////window->Initialize();
		//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		//glLoadIdentity();									// Reset The Projection Matrix
		//// Calculate The Aspect Ratio Of The Window
		//gluPerspective(45.0f, (GLfloat)s->GetRenderer()->GetRenderWindow()->GetSize()[0]/ (GLfloat)window->GetSize()[1], 0.1f, 100.0f);
		//glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
		//glLoadIdentity();									// Reset The Modelview Matrix
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
		//glLoadIdentity();									// Reset The Current Modelview Matrix
		//glTranslatef(-1.5f, 0.0f, -6.0f);						// Move Left 1.5 Units And Into The Screen 6.0
		//glBegin(GL_TRIANGLES);								// Drawing Using Triangles
		//glVertex3f(0.0f, 1.0f, 0.0f);					// Top
		//glVertex3f(-1.0f, -1.0f, 0.0f);					// Bottom Left
		//glVertex3f(1.0f, -1.0f, 0.0f);					// Bottom Right
		//glEnd();											// Finished Drawing The Triangle
		//glTranslatef(3.0f, 0.0f, 0.0f);						// Move Right 3 Units
		//glBegin(GL_QUADS);									// Draw A Quad
		//glVertex3f(-1.0f, 1.0f, 0.0f);					// Top Left
		//glVertex3f(1.0f, 1.0f, 0.0f);					// Top Right
		//glVertex3f(1.0f, -1.0f, 0.0f);					// Bottom Right
		//glVertex3f(-1.0f, -1.0f, 0.0f);					// Bottom Left
		//glEnd();
		//////SwapBuffers(window->GetMemoryDC());
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
	renderer->SetBackground(0.8, 0, 0);
	renderer->AddViewProp(myProp3d);
	vtkSmartPointer<myRenderPassExperimento> myRP = vtkSmartPointer<myRenderPassExperimento>::New();
	vtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(myRP);
	vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	renderWindow->SetSize(640, 480);
	renderWindow->AddRenderer(renderer);
	renderWindow->SwapBuffersOn();
	renderWindow->Render();
	vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	renderWindowInteractor->SetRenderWindow(renderWindow);
	renderWindowInteractor->Start();

	return EXIT_SUCCESS;
}