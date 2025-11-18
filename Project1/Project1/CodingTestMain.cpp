//*** 헤더파일과 라이브러리 포함시키기
// 헤더파일 디렉토리 추가하기: 프로젝트 메뉴 -> 맨 아래에 있는 프로젝트 속성 -> VC++ 디렉토리 -> 일반의 포함 디렉토리 -> 편집으로 가서 현재 디렉토리의 include 디렉토리 추가하기
// 라이브러리 디렉토리 추가하기: 프로젝트 메뉴 -> 맨 아래에 있는 프로젝트 속성 -> VC++ 디렉토리 -> 일반의 라이브러리 디렉토리 -> 편집으로 가서 현재 디렉토리의 lib 디렉토리 추가하기

#define _CRT_SECURE_NO_WARNINGS 

#include <iostream>
#include <stdlib.h>
#include <random>
#include <stdio.h>
#include <vector>
#include <gl/glew.h>				
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h> 
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>


#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "freeglut.lib")
#pragma warning(disable: 4711 4710 4100)

GLvoid keyboard(unsigned char, int, int);
GLvoid drawScene(GLvoid);
GLvoid Reshape(int w, int h);
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();

void InitBuffer();

GLuint shaderProgramID;	
GLuint vertexShader;	
GLuint fragmentShader;	

GLuint vao, vbo[2];

bool light_Frag = true;
bool toggle_viewport = false;
int x, y;

class Cuboid {
public:
	glm::vec3 position;
	glm::vec3 size;
	glm::vec3 color;

	Cuboid(const glm::vec3& pos, const glm::vec3& sz, const glm::vec3& color)
		: position(pos), size(sz), color(color) {
	}
};

class CuboidManager {
public:
	std::vector<Cuboid> cuboids;

	CuboidManager(int x, int y) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> size_dist(0.2f, 1.0f);
		std::uniform_real_distribution<float> color_dist(0.0f, 1.0f);

		float startX = 0.0f;
		float startZ = 0.0f;

		for (int i = 0; i < x; ++i) {
			startX = 0.0f;
			float maxZ = 0.0f;
			for (int j = 0; j < y; ++j) {
				glm::vec3 sz(0.2f, size_dist(gen), 0.2f);
				glm::vec3 pos(startX, 0.0f, startZ);
				glm::vec3 col(color_dist(gen), color_dist(gen), color_dist(gen));

				cuboids.emplace_back(pos, sz, col);

				startX += sz.x; 
			}
			startZ += 0.4f; 
		}
	}

	void draw(GLuint shaderProgramID, GLuint vao) {
		for (const auto& cuboid : cuboids) {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), cuboid.position);
			model = glm::scale(model, cuboid.size);
			model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f)); // 밑면이 y=0에 위치하도록 조정

			GLint locModel = glGetUniformLocation(shaderProgramID, "model");
			glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

			GLint objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor");
			glUniform3f(objColorLocation, cuboid.color.r, cuboid.color.g, cuboid.color.b);

			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
	}
};

CuboidManager* cuboidManager;

char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;

	fptr = fopen(file, "rb"); 		
	if (!fptr) 			
		return NULL;
	fseek(fptr, 0, SEEK_END); 		
	length = ftell(fptr); 			
	buf = (char*)malloc(length + 1); 		
	fseek(fptr, 0, SEEK_SET); 		
	fread(buf, length, 1, fptr); 		
	fclose(fptr); 			
	buf[length] = 0; 			

	return buf; 			
}


int main(int argc, char** argv) 			
{	
	//--- 윈도우 생성하기
	glutInit(&argc, argv);				
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); 		
	glutInitWindowPosition(0, 0); 			
	glutInitWindowSize(1280, 960); 			
	glutCreateWindow("2025 Coding Test-Computer Graphics"); 			

	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)			
	{
		std::cerr << "Unable to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";


	//--- 세이더 읽어와서 세이더 프로그램 만들기
	make_vertexShaders();	
	make_fragmentShaders();	
	shaderProgramID = make_shaderProgram();
	
	InitBuffer();
	glutKeyboardFunc(keyboard);
	glutDisplayFunc(drawScene); 			
	glutReshapeFunc(Reshape);		
	
	
	std::cout << "x y 좌표 입력: ";
	std::cin >> x >> y;

	cuboidManager = new CuboidManager(x, y);

	glutMainLoop();				
}

GLvoid keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case'q':
		exit(0);
		break;
	case 'o':
		toggle_viewport = false;
		glutPostRedisplay(); // 화면 갱신
		std::cout << "원근 투영" << std::endl;
		break;
	case 'p':
		toggle_viewport = true;
		glutPostRedisplay(); // 화면 갱신
		std::cout << "직각 투영" << std::endl;
		break;
	}
}

const GLfloat cube_vertices[36][3] = {
	// 앞면 (z = 0.5)
	{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f},
	{-0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},

	// 뒷면 (z = -0.5)
	{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f},
	{-0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},

	// 상면 (y = 0.5)
	{-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f},
	{-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},

	// 하면 (y = -0.5)
	{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f},
	{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f},

	// 오른면 (x = 0.5)
	{ 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f},
	{ 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},

	// 왼면 (x = -0.5)
	{-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f,  0.5f},
	{-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}
};
const GLfloat cube_normals[36][3] = {
	// 앞면 (z = 0.5)
	{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
	{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},

	// 뒷면 (z = -0.5)
	{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
	{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},

	// 상면 (y = 0.5)
	{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
	{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},

	// 하면 (y = -0.5)
	{0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
	{0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},

	// 오른면 (x = 0.5)
	{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
	{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},

	// 왼면 (x = -0.5)
	{-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
	{-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}
};
const GLfloat cube_colors[6][3] = {
	// 상면: 녹색
	{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
	{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
};

unsigned int lightPosLocation;
void InitBuffer()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(2, vbo);

	// 정점 버퍼
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// 법선 버퍼
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);


	glUseProgram(shaderProgramID);
	lightPosLocation = glGetUniformLocation(shaderProgramID, "lightpos");
	glUniform3f(lightPosLocation,0.0f , 0.0f, 0.0f);
	int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
	glUniform3f(lightColorLocation, 1.0f, 1.0f, 1.0f);
	int objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor");
	glUniform3f(objColorLocation, 0.0f, 1.0f, 0.0f);
}

GLvoid drawScene() 				//--- 콜백 함수: 출력 콜백 함수
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	glBindVertexArray(vao);

	glm::vec3 camera_eye = glm::vec3(0.0f, 3.0f, 3.0f);
	glm::vec3 camera_at = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 view = glm::lookAt (
		camera_eye,
		camera_at,
		camera_up
	);

	glm::mat4 projection;
	if (toggle_viewport) {
		projection = glm::perspective(
			glm::radians(60.0f),
			1280.0f / 960.0f,
			0.1f,
			100.0f
		);
	}
	else {
		projection = glm::ortho(
			-3.0f, 3.0f,
			-3.0f, 3.0f,
			0.1f, 100.0f
		);
	}
	
	cuboidManager->draw(shaderProgramID, vao);

	GLint locview = glGetUniformLocation(shaderProgramID, "view");
	glUniformMatrix4fv(locview, 1, GL_FALSE, glm::value_ptr(view));

	GLint locprojection = glGetUniformLocation(shaderProgramID, "projection");
	glUniformMatrix4fv(locprojection, 1, GL_FALSE, glm::value_ptr(projection));

	GLint locModel = glGetUniformLocation(shaderProgramID, "model");
	glm::mat4 model = glm::mat4(1.0f);
	glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

	glutPostRedisplay();
	glutSwapBuffers(); 			

}


GLvoid Reshape(int w, int h)			//--- 콜백 함수: 다시 그리기 콜백 함수
{
	glViewport(0, 0, w, h);
}

void make_vertexShaders()
{
	GLchar* vertexSource;

	vertexSource = filetobuf("vertex.glsl");
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

void make_fragmentShaders()
{
	GLchar* fragmentSource;

	
	fragmentSource = filetobuf("fragment.glsl");    
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, (const GLchar**) & fragmentSource, 0);
	glCompileShader(fragmentShader);

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: frag_shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

GLuint make_shaderProgram()
{
	GLint result;
	GLchar* errorLog = NULL;
	GLuint shaderID;
	shaderID = glCreateProgram();			

	glAttachShader(shaderID, vertexShader);		
	glAttachShader(shaderID, fragmentShader);		

	glLinkProgram(shaderID);			

	glDeleteShader(vertexShader);			
	glDeleteShader(fragmentShader);

	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
		return false;
	}

	glUseProgram(shaderID);			

	return shaderID;
}

