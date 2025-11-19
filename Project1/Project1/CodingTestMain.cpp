//*** 헤더파일과 라이브러리 포함시키기
// 헤더파일 디렉토리 추가하기: 프로젝트 메뉴 -> 맨 아래에 있는 프로젝트 속성 -> VC++ 디렉토리 -> 일반의 포함 디렉토리 -> 편집으로 가서 현재 디렉토리의 include 디렉토리 추가하기
// 라이브러리 디렉토리 추가하기: 프로젝트 메뉴 -> 맨 아래에 있는 프로젝트 속성 -> VC++ 디렉토리 -> 일반의 라이브러리 디렉토리 -> 편집으로 가서 현재 디렉토리의 lib 디렉토리 추가하기

#define _CRT_SECURE_NO_WARNINGS 

#include <ctime>
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
bool command_y = false;
int x, y;

class Cuboid {
public:
	glm::vec3 position;
	glm::vec3 size;
	glm::vec3 color;
	float speed;
	float originalHeight;
	float fallY;
	bool isFalling; 

	Cuboid(const glm::vec3& pos, const glm::vec3& sz, const glm::vec3& color, float spd)
		: position(pos), size(sz), color(color), speed(spd), originalHeight(sz.y), fallY(5.0f), isFalling(true) {
	}
};

float max_x = 0.0f;	
bool command_m = false;

float animationTime = 0.0f;

float cameraOrbitAngle = 0.0f; 
float cameraOrbitRadius = 7.0f; 
float cameraOrbitHeight = 5.0f; 

float cameraEyeZ = 0.0f;

class CuboidManager {
public:
	std::vector<Cuboid> cuboids;
	bool isLow = false;

	CuboidManager(int x, int y) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> size_dist(0.2f, 1.0f);
		std::uniform_real_distribution<float> color_dist(0.0f, 1.0f);
		std::uniform_real_distribution<float> speed_dist(0.5f, 2.0f);

		float startX = 0.0f;
		float startZ = 0.0f;

		for (int i = 0; i < x; ++i) {
			startX = 0.0f;
			float maxZ = 0.0f;
			for (int j = 0; j < y; ++j) {
				glm::vec3 sz(0.2f, size_dist(gen), 0.2f);
				glm::vec3 pos(startX, 0.0f, startZ);
				glm::vec3 col(color_dist(gen), color_dist(gen), color_dist(gen));
				float spd = speed_dist(gen);

				cuboids.emplace_back(pos, sz, col,spd);

				startX += sz.x; 
			}
			startZ += 0.2f; 
		}
		max_x = startX;
	}

	bool updateFalling(float delta) {
		bool anyFalling = false;
		for (auto& cuboid : cuboids) {
			if (cuboid.isFalling) {
				cuboid.fallY -= delta * cuboid.speed * 2.0f; // 속도 조절
				if (cuboid.fallY <= 0.0f) {
					cuboid.fallY = 0.0f;
					cuboid.isFalling = false;
				}
				else {
					anyFalling = true;
				}
			}
		}
		return anyFalling;
	}

	void toggleLowHeight(float lowHeight = 0.2f) {
		if (!isLow) {
			for (auto& cuboid : cuboids) {
				cuboid.size.y = lowHeight;
			}
			isLow = true;
		}
		else {
			for (auto& cuboid : cuboids) {
				cuboid.size.y = cuboid.originalHeight;
			}
			isLow = false;
		}
	}

	glm::vec3 getCenter() const {
		if (cuboids.empty()) return glm::vec3(0.0f);
		glm::vec3 minPos = cuboids[0].position;
		glm::vec3 maxPos = cuboids[0].position;
		for (const auto& c : cuboids) {
			minPos = glm::min(minPos, c.position);
			maxPos = glm::max(maxPos, c.position + c.size);
		}
		return (minPos + maxPos) * 0.5f;
	}

	void draw(GLuint shaderProgramID, GLuint vao) {
		float scaleY = 1.0f;
		for (const auto& cuboid : cuboids) {
			glm::vec3 drawPos = cuboid.position;
			drawPos.y = cuboid.fallY;

			glm::mat4 model = glm::translate(glm::mat4(1.0f), drawPos);
			if (command_m) {
				scaleY = 1.0f + 0.5f * sin(animationTime * cuboid.speed);
			}
			model = glm::scale(model, glm::vec3(cuboid.size.x, cuboid.size.y * scaleY, cuboid.size.z));
			model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));

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


float animation_speed = 0.05f;

void updateAnimation(int value) {
	static bool firstFalling = true;
	if (firstFalling) {
		bool stillFalling = cuboidManager->updateFalling(animation_speed);
		glutPostRedisplay();
		if (stillFalling) {
			glutTimerFunc(16, updateAnimation, 0);
		}
		else {
			firstFalling = false;
			if (command_m) {
				glutTimerFunc(16, updateAnimation, 0);
			}
		}
		return;
	}
	if (command_m) {
		animationTime += animation_speed;
		glutPostRedisplay();
		glutTimerFunc(16, updateAnimation, 0);
	}
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
	std::cout << "o : 직각투영" << std::endl;
	std::cout << "p : 원근투영" << std::endl;
	std::cout << "z : z축 감소" << std::endl;
	std::cout << "Z : z축 증가" << std::endl;
	std::cout << "m : 애니메이션 정지" << std::endl;
	std::cout << "M : 애니메이션 작동" << std::endl;
	std::cout << "y : 음 방향 공전" << std::endl;
	std::cout << "Y : 양 방향 공전" << std::endl;
	std::cout << "r : 미로 제작" << std::endl;
	std::cout << "v : 육면체 이동" << std::endl;
	std::cout << "s : 미로 내 객체 생성" << std::endl;
	std::cout << "Y : 양 방향 공전" << std::endl;
	std::cout << "Y : 양 방향 공전" << std::endl;
	std::cout << "+ : 애니메이션 속도 증가" << std::endl;
	std::cout << "- : 애니메이션 속도 감소" << std::endl;
	std::cout << "c : 모든 값 초기화" << std::endl;
	std::cout << "q : 프로그램 종료" << std::endl;


	cuboidManager = new CuboidManager(x, y);

	glutTimerFunc(0, updateAnimation, 0);

	glutMainLoop();				
}


GLvoid keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'o':
		toggle_viewport = false;
		glutPostRedisplay(); // 화면 갱신
		break;
	case 'p':
		toggle_viewport = true;
		glutPostRedisplay(); // 화면 갱신
		break;
	case 'm':
		command_m = false;
		glutPostRedisplay(); // 화면 갱신
		break;
	case 'M':
		if (!command_m) {
			command_m = true;
			glutTimerFunc(0, updateAnimation, 0);
		}
		break;
	case 'z':
		cameraEyeZ -= 0.1f;
		glutPostRedisplay();
		break;
	case 'Z':
		cameraEyeZ += 0.1f;
		glutPostRedisplay();
		break;
	case 'y':
		cameraOrbitAngle -= glm::radians(5.0f);
		glutPostRedisplay();
		break;
	case 'Y':
		cameraOrbitAngle += glm::radians(5.0f);
		glutPostRedisplay();
		break;
	case '+':
		if (animation_speed < 0.2f)
			animation_speed += 0.01f;
		else animation_speed = 0.2f;
		glutPostRedisplay();
		break;
	case '-':
		if (animation_speed > 0.01f)
			animation_speed -= 0.01f;
		else animation_speed = 0.01f;
		glutPostRedisplay();
		break;
	case 'v':
		command_m = false;
		if (cuboidManager) {
			cuboidManager->toggleLowHeight(0.2f);
		}
		glutPostRedisplay();
		break;
	case'q':
	case'Q':
		exit(0);
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
	glUniform3f(lightPosLocation,0.0f , 3.0f, 0.0f);
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

	glm::vec3 center = cuboidManager->getCenter();
	// 공전 각도에 따라 카메라 위치 계산
	float camX = center.x + cameraOrbitRadius * cos(cameraOrbitAngle);
	float camZ = center.z + cameraOrbitRadius * sin(cameraOrbitAngle) + cameraEyeZ;
	float camY = center.y + cameraOrbitHeight;
	glm::vec3 camera_eye = glm::vec3(camX, camY, camZ);
	glm::vec3 camera_at = center;
	glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 view = glm::lookAt(
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
	
	GLint locview = glGetUniformLocation(shaderProgramID, "view");
	glUniformMatrix4fv(locview, 1, GL_FALSE, glm::value_ptr(view));

	GLint locprojection = glGetUniformLocation(shaderProgramID, "projection");
	glUniformMatrix4fv(locprojection, 1, GL_FALSE, glm::value_ptr(projection));

	GLint locModel = glGetUniformLocation(shaderProgramID, "model");
	glm::mat4 model = glm::mat4(1.0f);
	glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

	cuboidManager->draw(shaderProgramID, vao);

	// 미니맵
	int minimapWidth = 300;
	int minimapHeight = 300;
	glViewport(1280 - minimapWidth - 10, 960 - minimapHeight - 10, minimapWidth, minimapHeight);

	// 미니맵 카메라
	glm::vec3 top_eye = glm::vec3(0.0f, 10.0f, 0.0f);
	glm::vec3 top_at = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 top_up = glm::vec3(0.0f, 0.0f, -1.0f);

	glm::mat4 top_view = glm::lookAt(top_eye, top_at, top_up);
	glm::mat4 top_projection = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, 0.1f, 100.0f);

	glUniformMatrix4fv(locview, 1, GL_FALSE, glm::value_ptr(top_view));
	glUniformMatrix4fv(locprojection, 1, GL_FALSE, glm::value_ptr(top_projection));

	cuboidManager->draw(shaderProgramID, vao);

	// 3. 원래 뷰포트로 복구
	glViewport(0, 0, 1280, 960);

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

