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
GLvoid specialKeyboard(int, int, int);
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


class MovableObject {
public:
	glm::vec3 position;
	float size;
	glm::vec3 color;
	bool isIndependent;

	glm::vec3 targetPosition;
	float moveSpeed = 4.0f;
	// 🌟 추가: 이동 상태 플래그
	bool isMoving = false;

	MovableObject(const glm::vec3& pos, float s, const glm::vec3& col, bool independent = true)
		: position(pos), size(s), color(col), isIndependent(independent) {
		targetPosition = pos;
	}

	void draw(GLuint shaderProgramID, GLuint vao) {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
		model = glm::scale(model, glm::vec3(size));

		GLint locModel = glGetUniformLocation(shaderProgramID, "model");
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

		GLint objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor");
		glUniform3f(objColorLocation, color.r, color.g, color.b);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
};

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
bool player_created = false;
bool reset_animation = false;
bool first_person_mode = false;

class CuboidManager {
public:
	std::vector<Cuboid> cuboids;
	std::vector<std::vector<bool>> maze; // 미로 상태 저장 (true: 벽, false: 길)
	int rows, cols; // 미로의 행과 열 크기
	bool isLow = false;
	glm::ivec2 exitPosition;
	MovableObject* player;

	CuboidManager(int x, int y) : rows(y), cols(x) { // x는 열, y는 행에 대응
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> size_dist(0.2f, 1.0f);
		std::uniform_real_distribution<float> color_dist(0.0f, 1.0f);
		std::uniform_real_distribution<float> speed_dist(0.5f, 2.0f);

		float startX = 0.0f;
		float startZ = 0.0f;
		float cell_size = 0.2f;

		// 큐보이드 생성
		for (int i = 0; i < rows; ++i) { // Z축 (행)
			startX = 0.0f;
			for (int j = 0; j < cols; ++j) { // X축 (열)
				glm::vec3 sz(cell_size, size_dist(gen), cell_size);
				glm::vec3 pos(startX, 0.0f, startZ);
				glm::vec3 col(color_dist(gen), color_dist(gen), color_dist(gen));
				float spd = speed_dist(gen);

				cuboids.emplace_back(pos, sz, col, spd);

				startX += sz.x;
			}
			startZ += cell_size;
		}
		max_x = startX;

		player = nullptr;
	}

	~CuboidManager() {
		delete player;
	}
	void createPlayer() {
		if (player) {
			delete player;
		}

		player = new MovableObject(glm::vec3(0.0f), 0.18f, glm::vec3(1.0f, 1.0f, 0.0f), true);

		if (is_maze_mode) {
			float cell_size = 0.2f;

			glm::vec3 start_pos(
				0.0f * cell_size + cell_size,

				player->size / 2.0f,

				0.0f * cell_size + cell_size
			);
			player->position = start_pos;
			player->targetPosition = start_pos;
			player->isMoving = false;
		}
	}

	void updatePlayerMovement(float deltaTime) {
		if (!player) return;

		glm::vec3 currentPos = player->position;
		glm::vec3 targetPos = player->targetPosition;

		glm::vec3 moveVector = targetPos - currentPos;
		float distance = glm::length(moveVector);

		if (distance > 0.001f) {
			player->isMoving = true;

			const float FIXED_MOVE_DISTANCE = 0.01f;
			float maxMove = FIXED_MOVE_DISTANCE;

			if (distance > maxMove) {
				player->position += glm::normalize(moveVector) * maxMove;
			}
			else {
				player->position = targetPos;
				player->isMoving = false; 
			}
		}
		else {
			player->isMoving = false;
		}
	}

	void movePlayer(float deltaX, float deltaZ) {
		if (!player || !is_maze_mode || player->isMoving) return;

		float cell_size = 0.2f;
		int currentGridX = (int)round((player->targetPosition.x / cell_size) );
		int currentGridZ = (int)round((player->targetPosition.z / cell_size) );

		int newGridX = currentGridX + (int)deltaX;
		int newGridZ = currentGridZ + (int)deltaZ;

		// 경계 체크
		if (newGridX >= 0 && newGridX < cols && newGridZ >= 0 && newGridZ < rows) {
			// 벽 충돌 체크
			if (!maze[newGridZ][newGridX]) {
				glm::vec3 newTargetPos(
					(newGridX ) * cell_size,
					player->size / 2.0f,
					(newGridZ ) * cell_size
				);

				player->targetPosition = newTargetPos;
			}
		}
	}

	// 미로 생성 함수 (DFS 기반, 짝수/홀수 크기 모두 지원)
	void generateMaze() {
		// 1. 모든 셀을 벽으로 초기화
		maze.assign(rows, std::vector<bool>(cols, true)); // true: 벽

		// 2. DFS를 위한 스택 및 시작점 설정
		std::stack<glm::ivec2> stack;

		// (1, 1)에서 시작 (가장자리 테두리는 벽)
		glm::ivec2 start_cell(1, 1);

		maze[start_cell.y][start_cell.x] = false; // 시작 셀은 길
		stack.push(start_cell);

		std::random_device rd;
		std::mt19937 gen(rd());

		std::vector<glm::ivec2> directions = {
			{0, -2},
			{0, 2},
			{2, 0},
			{-2, 0}
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

		// 5. Cuboid 벡터 업데이트
		updateCuboidsFromMaze();

		if (player) {
			float cell_size = 0.2f;
			glm::vec3 start_pos(
				0.0f * cell_size + cell_size,
				player->size / 2.0f,
				0.0f * cell_size + cell_size
			);
			player->position = start_pos;
			player->targetPosition = start_pos;
			player->isMoving = false;
		}
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
					// 시작점인지 확인 (1, 1)
					if (j == 1 && i == 1) {
						cuboid.size.y = cuboid.min_height;
						cuboid.color = glm::vec3(0.0f, 1.0f, 0.0f); // 초록색 시작점
					}
					else {
						// 출구 포함해서 모든 길은 완전히 제거
						cuboid.size.y = 0.0f;
						cuboid.color = glm::vec3(1.0f, 1.0f, 1.0f); // 길은 검은색으로
					}
				}
			}
		}
		is_maze_mode = true;
	}


	bool updateFalling(float delta) {
		bool anyFalling = false;
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

			// 미로 모드에서 길(path)인 경우 (시작/출구 마커 제외) 건너뛰기
			if (is_maze_mode && !maze.empty()) {
				int i = idx / cols; // 행
				int j = idx % cols; // 열
				// 일반 길 (높이가 0.0f인 큐보이드)는 그리지 않음
				if (!maze[i][j] && cuboid.size.y == 0.0f) {
					continue;
				}
			}

			glm::vec3 drawPos = cuboid.position;
			if (!is_maze_mode) {
				drawPos.y = cuboid.fallY;
			}
			else {
				drawPos.y = 0.0f;
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

		if (is_maze_mode && player) {
			player->draw(shaderProgramID, vao);
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
	static GLint lastTime = glutGet(GLUT_ELAPSED_TIME);

	GLint currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (float)(currentTime - lastTime) / 1000.0f;
	lastTime = currentTime;


	if (reset_animation) {
		firstFalling = true;
		reset_animation = false;
	}

	if (firstFalling && !is_maze_mode) {
		bool stillFalling = cuboidManager->updateFalling(deltaTime * 1.0f);
		glutPostRedisplay();
		if (stillFalling) {
			glutTimerFunc(16, updateAnimation, 0);
		}
		else {
			firstFalling = false;
			if (command_m || is_maze_mode) {
				glutTimerFunc(16, updateAnimation, 0);
			}
		}
		return;
	}

	if (command_m || is_maze_mode) {
		animationTime += animation_speed * deltaTime * 1.0f;

		if (is_maze_mode && player_created) {
			cuboidManager->updatePlayerMovement(deltaTime);
		}

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
	glutSpecialFunc(specialKeyboard);
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
	std::cout << "1 : 1인칭 시점" << std::endl;
	std::cout << "3 : 3인칭 시점" << std::endl;
	std::cout << "방향키 : 플레이어 이동" << std::endl;
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
			command_m = false; 
		}
		glutPostRedisplay();
		break;
	case 's':
		if (cuboidManager && is_maze_mode) {
			cuboidManager->createPlayer();
			player_created = true;
		}
		glutPostRedisplay();
		break;
	case 'c':
		// 모든 전역 변수 초기화
		maze_generated = false;
		is_maze_mode = false;
		player_created = false;
		command_m = false;
		reset_animation = true;
		first_person_mode = false;

		// 애니메이션 관련 변수 초기화
		animationTime = 0.0f;
		animation_speed = 0.05f;

		// 카메라 관련 변수 초기화
		cameraOrbitAngle = 0.0f;
		cameraOrbitRadius = 7.0f;
		cameraOrbitHeight = 5.0f;
		cameraEyeZ = 0.0f;

		// 뷰포트 초기화
		toggle_viewport = false;

		// CuboidManager 재생성
		delete cuboidManager;
		cuboidManager = new CuboidManager(::x, ::y);

		glutTimerFunc(0, updateAnimation, 0);

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
	case '1':
		if (player_created && is_maze_mode) {
			first_person_mode = true;
		}
		glutPostRedisplay();
		break;

	case '3':
		if (player_created && is_maze_mode) {
			first_person_mode = false;
		}
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

GLvoid specialKeyboard(int key, int x, int y) {
	if (!player_created || !is_maze_mode) return;

	switch (key) {
	case GLUT_KEY_UP:
		cuboidManager->movePlayer(0.0f, -1.0f);
		break;
	case GLUT_KEY_DOWN:
		cuboidManager->movePlayer(0.0f, 1.0f);
		break;
	case GLUT_KEY_LEFT:
		cuboidManager->movePlayer(-1.0f, 0.0f);
		break;
	case GLUT_KEY_RIGHT:
		cuboidManager->movePlayer(1.0f, 0.0f);
		break;
	}
	glutPostRedisplay();
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

	glm::vec3 center;
	glm::vec3 camera_eye;
	glm::vec3 camera_at;
	glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

	if (player_created && is_maze_mode && cuboidManager->player) {
		glm::vec3 playerPos = cuboidManager->player->position;

		if (first_person_mode) {
			camera_eye = playerPos + glm::vec3(0.0f, 0.15f, 0.0f);
			camera_at = camera_eye + glm::vec3(0.0f, 0.0f, -1.0f);
		}
		else {
			camera_eye = playerPos + glm::vec3(0.0f, 0.8f, 0.8f);
			camera_at = playerPos;
		}
	}
	else {
		center = cuboidManager->getCenter();
		float camX = center.x + cameraOrbitRadius * cos(cameraOrbitAngle);
		float camZ = center.z + cameraOrbitRadius * sin(cameraOrbitAngle) + cameraEyeZ;
		float camY = center.y + cameraOrbitHeight;
		camera_eye = glm::vec3(camX, camY, camZ);
		camera_at = center;
	}

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

	GLint viewPosLocation = glGetUniformLocation(shaderProgramID, "viewPos");
	glUniform3f(viewPosLocation, camera_eye.x, camera_eye.y, camera_eye.z);

	GLint locModel = glGetUniformLocation(shaderProgramID, "model");
	glm::mat4 model = glm::mat4(1.0f);
	glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

	cuboidManager->draw(shaderProgramID, vao);

	// 미니맵
	int minimapWidth = 300;
	int minimapHeight = 300;
	glViewport(1280 - minimapWidth - 10, 960 - minimapHeight - 10, minimapWidth, minimapHeight);

	center = cuboidManager->getCenter();
	glm::vec3 top_eye = glm::vec3(center.x, 10.0f, center.z);
	glm::vec3 top_at = center;
	glm::vec3 top_up = glm::vec3(0.0f, 0.0f, -1.0f);

	float ortho_size_x = max_x / 2.0f + 1.0f;
	float ortho_size_y = (float)y * 0.2f / 2.0f + 1.0f;

	glm::mat4 top_view = glm::lookAt(top_eye, top_at, top_up);
	glm::mat4 top_projection = glm::ortho(-ortho_size_x, ortho_size_x, -ortho_size_y, ortho_size_y, 0.1f, 100.0f);

	glUniformMatrix4fv(locview, 1, GL_FALSE, glm::value_ptr(top_view));
	glUniformMatrix4fv(locprojection, 1, GL_FALSE, glm::value_ptr(top_projection));

	glUniform3f(viewPosLocation, top_eye.x, top_eye.y, top_eye.z);

	cuboidManager->draw(shaderProgramID, vao);

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
		return 0; // false 대신 0 반환
	}

	glUseProgram(shaderID);

	return shaderID;
}