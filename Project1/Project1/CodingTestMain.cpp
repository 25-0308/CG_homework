#define _CRT_SECURE_NO_WARNINGS

#include <ctime>
#include <iostream>
#include <stdlib.h>
#include <random>
#include <stdio.h>
#include <vector>
#include <stack>
#include <algorithm>
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
int x, y; // 큐브 수 -> 미로 크기로 사용

class Cuboid {
public:
	glm::vec3 position;
	glm::vec3 size;
	glm::vec3 color;
	float speed;
	float originalHeight;
	float fallY;
	bool isFalling;
	float min_height;

	Cuboid(const glm::vec3& pos, const glm::vec3& sz, const glm::vec3& color, float spd)
		: position(pos), size(sz), color(color), speed(spd), originalHeight(sz.y), fallY(5.0f), isFalling(true), min_height(0.2f) {
	}
};

float max_x = 0.0f;
bool command_m = false;
bool is_maze_mode = false; // 미로 모드 플래그

float animationTime = 0.0f;

float cameraOrbitAngle = 0.0f;
float cameraOrbitRadius = 7.0f;
float cameraOrbitHeight = 5.0f;

float cameraEyeZ = 0.0f;

bool maze_generated = false;

class CuboidManager {
public:
	std::vector<Cuboid> cuboids;
	std::vector<std::vector<bool>> maze; // 미로 상태 저장 (true: 벽, false: 길)
	int rows, cols; // 미로의 행과 열 크기
	bool isLow = false;
	glm::ivec2 exitPosition;

	CuboidManager(int x, int y) : rows(y), cols(x) { // x는 열, y는 행에 대응
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> size_dist(0.2f, 1.0f);
		std::uniform_real_distribution<float> color_dist(0.0f, 1.0f);
		std::uniform_real_distribution<float> speed_dist(0.5f, 2.0f);

		float startX = 0.0f;
		float startZ = 0.0f;

		// 큐보이드 생성
		for (int i = 0; i < rows; ++i) { // Z축 (행)
			startX = 0.0f;
			for (int j = 0; j < cols; ++j) { // X축 (열)
				glm::vec3 sz(0.2f, size_dist(gen), 0.2f);
				glm::vec3 pos(startX, 0.0f, startZ);
				glm::vec3 col(color_dist(gen), color_dist(gen), color_dist(gen));
				float spd = speed_dist(gen);

				cuboids.emplace_back(pos, sz, col, spd);

				startX += sz.x;
			}
			startZ += 0.2f;
		}
		max_x = startX;
	}

	// 미로 생성 함수 (DFS 기반, 짝수/홀수 크기 모두 지원)
	void generateMaze() {
		// 1. 모든 셀을 벽으로 초기화
		maze.assign(rows, std::vector<bool>(cols, true)); // true: 벽

		// ⚠️ 크기가 매우 작은 경우 예외 처리
		if (rows < 3 || cols < 3) {
			// 매우 작은 크기의 경우 전체를 길로 만들고 종료
			for (int i = 0; i < rows; ++i)
				for (int j = 0; j < cols; ++j)
					maze[i][j] = false;
			updateCuboidsFromMaze();
			return;
		}

		// 2. DFS를 위한 스택 및 시작점 설정
		std::stack<glm::ivec2> stack;

		// (1, 1)에서 시작 (가장자리 테두리는 벽)
		glm::ivec2 start_cell(1, 1);

		maze[start_cell.y][start_cell.x] = false; // 시작 셀은 길
		stack.push(start_cell);

		std::random_device rd;
		std::mt19937 gen(rd());

		std::vector<glm::ivec2> directions = {
			{0, -2}, // 북
			{0, 2},  // 남
			{2, 0},  // 동
			{-2, 0}  // 서
		};

		// 3. DFS 수행
		while (!stack.empty()) {
			glm::ivec2 current = stack.top();
			stack.pop();

			std::shuffle(directions.begin(), directions.end(), gen); // 랜덤 방향 선택

			for (const auto& dir : directions) {
				glm::ivec2 next = current + dir; // 한 칸 건너뛴 위치

				// 경계 확인: 1 < x < cols-1, 1 < y < rows-1
				if (next.x > 0 && next.x < cols - 1 && next.y > 0 && next.y < rows - 1) {
					if (maze[next.y][next.x] == true) { // 아직 방문하지 않은(벽인) 셀

						// 현재 셀과 다음 셀 사이의 벽을 허물기
						glm::ivec2 wall = current + dir / 2;
						maze[wall.y][wall.x] = false; // 길
						maze[next.y][next.x] = false; // 길

						stack.push(current); // 현재 셀을 다시 스택에 넣고
						stack.push(next); // 다음 셀로 이동
						break; // 다음 셀로 이동
					}
				}
			}
		}

		// 4. 출구 설정 - 미로 크기의 중간값을 출구로 설정
		int midY = rows / 2; // 세로 중간값

		// 오른쪽 가장자리(cols-1)의 중간 위치에 출구 생성
		exitPosition = glm::ivec2(cols - 1, midY);
		maze[midY][cols - 1] = false; // 출구

		// 출구와 미로 내부를 연결하기 위해 출구 옆 셀도 길로 만들기
		if (cols - 2 >= 0) {
			maze[midY][cols - 2] = false;
		}

		std::cout << "출구 위치: (" << exitPosition.x << ", " << exitPosition.y << ")" << std::endl;

		// 5. Cuboid 벡터 업데이트
		updateCuboidsFromMaze();
	}

	// 미로 상태에 따라 큐보이드 업데이트
	void updateCuboidsFromMaze() {
		if (maze.empty() || cuboids.size() != rows * cols) return;

		for (int i = 0; i < rows; ++i) {
			for (int j = 0; j < cols; ++j) {
				int index = i * cols + j;
				Cuboid& cuboid = cuboids[index];

				if (maze[i][j]) { // 벽 (Wall)
					cuboid.size.y = cuboid.originalHeight; // 높은 벽
					// 원래 색상 유지
				}
				else { // 길 (Path)
					// 출구인지 확인
					if (j == exitPosition.x && i == exitPosition.y) {
						// 출구는 낮은 높이로 표시하고 빨간색으로
						cuboid.size.y = cuboid.min_height;
						cuboid.color = glm::vec3(1.0f, 0.0f, 0.0f); // 빨간색 출구
					}
					// 시작점인지 확인 (1, 1)
					else if (j == 1 && i == 1) {
						cuboid.size.y = cuboid.min_height;
						cuboid.color = glm::vec3(0.0f, 1.0f, 0.0f); // 초록색 시작점
					}
					else {
						// 일반 길은 완전히 제거
						cuboid.size.y = 0.0f;
					}
				}
			}
		}
		is_maze_mode = true;
	}


	bool updateFalling(float delta) {
		bool anyFalling = false;
		// 미로 모드에서는 낙하 애니메이션 건너뛰기
		if (is_maze_mode) return false;

		for (auto& cuboid : cuboids) {
			if (cuboid.isFalling) {
				cuboid.fallY -= delta * cuboid.speed * 2.0f;
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
		for (int idx = 0; idx < cuboids.size(); ++idx) {
			const auto& cuboid = cuboids[idx];

			// 미로 모드에서 길(path)인 경우 그리지 않음
			if (is_maze_mode && !maze.empty()) {
				int i = idx / cols; // 행
				int j = idx % cols; // 열
				if (!maze[i][j]) { // 길인 경우 건너뛰기
					continue;
				}
			}

			glm::vec3 drawPos = cuboid.position;
			if (!is_maze_mode) {
				drawPos.y = cuboid.fallY;
			}
			else {
				drawPos.y = 0.0f; // 바닥에 고정
			}

			glm::mat4 model = glm::translate(glm::mat4(1.0f), drawPos);
			if (command_m) {
				scaleY = 1.0f + 0.5f * sin(animationTime * cuboid.speed);
			}
			else {
				scaleY = 1.0f;
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
	std::vector<glm::ivec2> getCornerCoords(int width, int height) const {
		// 좌상, 우상, 좌하, 우하
		std::vector<glm::ivec2> corners;
		corners.push_back(glm::ivec2(0, 0));
		corners.push_back(glm::ivec2(width - 1, 0));
		corners.push_back(glm::ivec2(0, height - 1));
		corners.push_back(glm::ivec2(width - 1, height - 1)); // 우하
		return corners;
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
	if (firstFalling && !is_maze_mode) {
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

	if (command_m || is_maze_mode) {
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
	case 'r':
		if (cuboidManager && !maze_generated) {
			cuboidManager->generateMaze();
			maze_generated = true; 
		}
		else if (maze_generated) {
		}
		glutPostRedisplay();
		break;
	case 'c':
		maze_generated = false; 
		is_maze_mode = false;
		delete cuboidManager;
		cuboidManager = new CuboidManager(::x, ::y);
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
	{-0.5f, -0.5f,0.5f}, { 0.5f, -0.5f,0.5f}, { 0.5f,0.5f,0.5f},
	{-0.5f, -0.5f, 0.5f}, { 0.5f,0.5f, 0.5f}, {-0.5f,0.5f,0.5f},

	// 뒷면 (z = -0.5)
	{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,0.5f, -0.5f},
	{-0.5f, -0.5f, -0.5f}, { 0.5f,0.5f, -0.5f}, {-0.5f,0.5f, -0.5f},

	// 상면 (y = 0.5)
	{-0.5f,0.5f, -0.5f}, { 0.5f,0.5f, -0.5f}, { 0.5f,0.5f,0.5f},
	{-0.5f,0.5f, -0.5f}, { 0.5f,0.5f,0.5f}, {-0.5f,0.5f,0.5f},

	// 하면 (y = -0.5)
	{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,0.5f},
	{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,0.5f}, {-0.5f, -0.5f,0.5f},

	// 오른면 (x = 0.5)
	{ 0.5f, -0.5f, -0.5f}, { 0.5f,0.5f, -0.5f}, { 0.5f,0.5f,0.5f},
	{ 0.5f, -0.5f, -0.5f}, { 0.5f,0.5f,0.5f}, { 0.5f, -0.5f,0.5f},

	// 왼면 (x = -0.5)
	{-0.5f, -0.5f, -0.5f}, {-0.5f,0.5f, -0.5f}, {-0.5f,0.5f,0.5f},
	{-0.5f, -0.5f, -0.5f}, {-0.5f,0.5f,0.5f}, {-0.5f, -0.5f,0.5f}
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
	glUniform3f(lightPosLocation, 0.0f, 3.0f, 0.0f);
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

	// 미니맵 카메라 (Top-Down Orthographic View)
	glm::vec3 top_eye = glm::vec3(center.x, 10.0f, center.z);
	glm::vec3 top_at = center;
	glm::vec3 top_up = glm::vec3(0.0f, 0.0f, -1.0f); // Z축 방향으로 아래를 보게

	// 미니맵 투영 (미로 전체가 보이도록 크기 조절)
	float ortho_size_x = max_x / 2.0f + 1.0f;
	float ortho_size_y = (float)y * 0.2f / 2.0f + 1.0f;

	glm::mat4 top_view = glm::lookAt(top_eye, top_at, top_up);
	glm::mat4 top_projection = glm::ortho(-ortho_size_x, ortho_size_x, -ortho_size_y, ortho_size_y, 0.1f, 100.0f);

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
	glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
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
		return 0; // false 대신 0 반환
	}

	glUseProgram(shaderID);

	return shaderID;
}