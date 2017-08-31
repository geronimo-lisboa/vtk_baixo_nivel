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
	myCustomProp3d()
	{
		bounds = { { 0, 1, 0, 1, 0, 1 } };
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

	int RenderVolumetricGeometry(vtkViewport *view)
	{
		std::cout << __FUNCTION__ << std::endl;
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
		//Primeiro experimento bem sucedido com backend grᦩco setado pra opengl2
		cout << __FUNCTION__ << endl;
		vtkWin32OpenGLRenderWindow *window = vtkWin32OpenGLRenderWindow::SafeDownCast( s->GetRenderer()->GetRenderWindow() );
		GLenum err = GL_NO_ERROR;
		window->MakeCurrent();
		glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
		glClearDepth(1.0f);									// Depth Buffer Setup
		glEnable(GL_DEPTH_TEST); // enable depth-testing
		glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
		err = glGetError();
		float points[] = {
			0.0f, 0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			-0.5f, -0.5f, 0.0f
		};
		GLuint vbo = 0;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), points, GL_STATIC_DRAW);
		err = glGetError();
		GLuint vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		err = glGetError();
		const char* vertex_shader =
			"#version 400\n"
			"in vec3 vp;"
			"void main() {"
			"  gl_Position = vec4(vp, 1.0);"
			"}";
		const char* fragment_shader =
			"#version 400\n"
			"out vec4 frag_colour;"
			"void main() {"
			"  frag_colour = vec4(0.5, 0.0, 0.5, 1.0);"
			"}";
		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, &vertex_shader, NULL);
		glCompileShader(vs);
		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, &fragment_shader, NULL);
		glCompileShader(fs);
		err = glGetError();
		GLuint shader_programme = glCreateProgram();
		glAttachShader(shader_programme, fs);
		glAttachShader(shader_programme, vs);
		glLinkProgram(shader_programme);
		err = glGetError();
		// wipe the drawing surface clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shader_programme);
		glBindVertexArray(vao);
		// draw points 0-3 from the currently bound VAO with current in-use shader
		glDrawArrays(GL_TRIANGLES, 0, 3);
		err = glGetError();

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