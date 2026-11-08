#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring> 

// Local connections to your ZIP folders
#include "dependente/glew/glew.h"
#include "dependente/glfw/glfw3.h"
#include "dependente/glm/glm.hpp"
#include "dependente/glm/gtc/matrix_transform.hpp"
#include "dependente/glm/gtc/type_ptr.hpp"

#include "shader.hpp"

// --- GLOBAL VARIABLES & GAME STATE ---
bool isGameOver = false;
int currentLevel = 1;

glm::vec2 playerPos(75.0f, 25.0f); // Spawn at entrance
float playerRadius = 15.0f;
float blockSize = 50.0f;
float timeElapsed = 0.0f;

// Enemy structure (Supports multi-point complex paths)
struct Spider {
    std::vector<glm::vec2> waypoints;
    float speed;
};

std::vector<Spider> spiders;
float spiderRadius = 18.0f;

// The active maze map (1 = wall, 0 = path)
int maze[12][16];

// Function to load the map and enemies based on the current level (Strictly 2 Levels)
void StartLevel(int level) {
    playerPos = glm::vec2(75.0f, 25.0f); // Reset player position
    timeElapsed = 0.0f;                  // Reset time for animation sync
    isGameOver = false;                  // Remove Game Over state

    if (level == 1) {
        // LEVEL 1 MAP (Basic Training)
        int map1[12][16] = {
            { 1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, // Row 0 (Bottom) - ENTRANCE
            { 1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1 },
            { 1,0,1,0,1,0,1,1,1,0,1,0,1,1,0,1 },
            { 1,0,1,0,0,0,0,0,1,0,0,0,1,0,0,1 },
            { 1,0,1,1,1,1,1,0,1,1,1,0,1,0,1,1 },
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
            { 1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1 },
            { 1,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1 },
            { 1,0,1,1,1,1,0,1,1,1,1,1,0,1,0,1 },
            { 1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 }  // Row 11 (Top) - EXIT at X=14
        };
        std::memcpy(maze, map1, sizeof(maze));

        // Spiders for Level 1
        spiders = {
            { { glm::vec2(175.0f, 175.0f), glm::vec2(375.0f, 175.0f) }, 1.5f },
            { { glm::vec2(375.0f, 275.0f), glm::vec2(725.0f, 275.0f) }, 1.8f },
            { { glm::vec2(75.0f, 475.0f),  glm::vec2(325.0f, 475.0f) }, 1.2f }
        };

    }
    else {
        // LEVEL 2 MAP (The Zig-Zag with Hiding Spots)
        int map2[12][16] = {
            { 1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, // Entrance
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }, // Row 1
            { 1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1 }, // Wall with gap at Right AND Hiding Spot at Index 7
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }, // Row 3
            { 1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1 }, // Wall with gap at Left AND Hiding Spot at Index 9
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }, // Row 5
            { 1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1 }, // Wall with gap at Right AND Hiding Spot at Index 5
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }, // Row 7
            { 1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1 }, // Wall with gap at Left AND Hiding Spot at Index 9
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }, // Row 9
            { 1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1 }, // Wall with Hiding Spot at Index 7
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 }  // Exit at X=14
        };
        std::memcpy(maze, map2, sizeof(maze));

        // Spiders for Level 2 (Constant speed, fair timing)
        spiders = {
            { { glm::vec2(175.0f, 75.0f),  glm::vec2(725.0f, 75.0f) }, 1.0f },
            { { glm::vec2(625.0f, 175.0f), glm::vec2(75.0f, 175.0f) }, 1.0f },
            { { glm::vec2(175.0f, 275.0f), glm::vec2(725.0f, 275.0f) }, 1.0f },
            { { glm::vec2(625.0f, 375.0f), glm::vec2(75.0f, 375.0f) }, 1.0f },
            { { glm::vec2(175.0f, 475.0f), glm::vec2(725.0f, 475.0f) }, 1.0f }
        };
    }

    printf("\n========================================\n");
    printf("        LEVEL %d INITIALIZED\n", level);
    printf("========================================\n");
}


// --- PRIMITIVE GENERATION ---
GLuint squareVAO, circleVAO;
int circleVertexCount = 0;

void CreateSquare() {
    float vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f
    };
    GLuint VBO;
    glGenVertexArrays(1, &squareVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void CreateCircle() {
    std::vector<float> vertices;
    int segments = 36;
    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.14159f / segments;
        vertices.push_back(cos(angle));
        vertices.push_back(sin(angle));
        vertices.push_back(0.0f);
    }
    circleVertexCount = vertices.size() / 3;

    GLuint VBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// --- MATH & COLLISIONS (Optimized with corner tolerance) ---
bool CheckCollision(glm::vec2 circleCenter, float radius, glm::vec2 aabbMin, glm::vec2 aabbMax) {
    float closestX = std::max(aabbMin.x, std::min(circleCenter.x, aabbMax.x));
    float closestY = std::max(aabbMin.y, std::min(circleCenter.y, aabbMax.y));
    float distanceX = circleCenter.x - closestX;
    float distanceY = circleCenter.y - closestY;

    // Slight tolerance (0.92f) at corners to prevent snapping/getting stuck
    return (distanceX * distanceX + distanceY * distanceY) < ((radius * 0.92f) * (radius * 0.92f));
}

bool CheckCircleCollision(glm::vec2 centerA, float radiusA, glm::vec2 centerB, float radiusB) {
    float dx = centerA.x - centerB.x;
    float dy = centerA.y - centerB.y;
    float distanceSquared = (dx * dx) + (dy * dy);
    float radiusSum = radiusA + radiusB;
    return distanceSquared < (radiusSum * radiusSum);
}

// --- MAIN PROGRAM ---
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "2D Vent Maze", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if (glewInit() != GLEW_OK) return -1;

    GLuint programID = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");

    CreateSquare();
    CreateCircle();

    glm::mat4 Projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint ColorID = glGetUniformLocation(programID, "objectColor");

    float lastTime = glfwGetTime();

    StartLevel(currentLevel);

    bool nKeyWasPressed = false; // Used for the debounce of the Skip button

    // --- GAME LOOP ---
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (!isGameOver) {
            timeElapsed += deltaTime;
            float speed = 350.0f * deltaTime; // Player speed

            // --- SKIP BUTTON (DEBUG) ---
            bool nKeyIsPressed = (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS);
            if (nKeyIsPressed && !nKeyWasPressed) {
                if (currentLevel < 2) {
                    currentLevel++;
                    printf("\n--- LEVEL SKIPPED (DEBUG) ---\n");
                    StartLevel(currentLevel);
                }
                else {
                    printf("\n========================================\n");
                    printf(" YOU WON THE GAME! (SKIPPED TO VICTORY)\n");
                    printf(" Press 'R' to play again from Level 1.\n");
                    printf("========================================\n");
                    isGameOver = true; // Trigger win screen state
                }
            }
            nKeyWasPressed = nKeyIsPressed;

            // --- X-AXIS MOVEMENT ---
            glm::vec2 nextPosX = playerPos;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) nextPosX.x -= speed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) nextPosX.x += speed;

            bool collisionX = false;
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 16; j++) {
                    if (maze[i][j] == 1) {
                        glm::vec2 boxMin(j * blockSize, i * blockSize);
                        glm::vec2 boxMax((j + 1) * blockSize, (i + 1) * blockSize);
                        if (CheckCollision(nextPosX, playerRadius, boxMin, boxMax)) {
                            collisionX = true;
                        }
                    }
                }
            }
            if (!collisionX) playerPos.x = nextPosX.x;

            // --- Y-AXIS MOVEMENT ---
            glm::vec2 nextPosY = playerPos;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) nextPosY.y += speed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) nextPosY.y -= speed;

            bool collisionY = false;
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 16; j++) {
                    if (maze[i][j] == 1) {
                        glm::vec2 boxMin(j * blockSize, i * blockSize);
                        glm::vec2 boxMax((j + 1) * blockSize, (i + 1) * blockSize);
                        if (CheckCollision(nextPosY, playerRadius, boxMin, boxMax)) {
                            collisionY = true;
                        }
                    }
                }
            }
            if (!collisionY) playerPos.y = nextPosY.y;

            // CHECK EXIT (PROGRESS)
            if (playerPos.y > 580.0f) {
                if (currentLevel < 2) {
                    currentLevel++;
                    printf("\n========================================\n");
                    printf(" YOU ESCAPED LEVEL 1! PROCEEDING TO LEVEL 2...\n");
                    printf("========================================\n");
                    StartLevel(currentLevel);
                }
                else {
                    printf("\n========================================\n");
                    printf(" CONGRATULATIONS! YOU WON THE ENTIRE GAME!\n");
                    printf(" You successfully escaped the vent maze!\n");
                    printf(" Press 'R' to play again from Level 1.\n");
                    printf("========================================\n");
                    isGameOver = true; // Triggers the end/win state
                }
            }
        }
        else {
            // GAME OVER / WIN SCREEN - WAIT FOR 'R' TO RESTART
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                currentLevel = 1;
                StartLevel(currentLevel);
                printf("Game reset to Level 1. Good luck!\n");
            }
        }

        // 2. RENDERING & SCALING
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        if (isGameOver) {
            // Dacă a terminat jocul sau a murit, afișăm ecranul special (Poți personaliza culoarea)
            glClearColor(0.2f, 0.0f, 0.4f, 1.0f); // Violet închis pentru VICTORIE/FINAL
        }
        else {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Gri pentru joc normal
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(programID);

        // --- DRAW MAZE ---
        glBindVertexArray(squareVAO);
        glUniform3f(ColorID, 0.4f, 0.4f, 0.5f); // Grey walls
        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 16; j++) {
                if (maze[i][j] == 1) {
                    glm::mat4 Model = glm::translate(glm::mat4(1.0f), glm::vec3(j * blockSize, i * blockSize, 0.0f));
                    Model = glm::scale(Model, glm::vec3(blockSize, blockSize, 1.0f));
                    glm::mat4 MVP = Projection * Model;
                    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }

        // --- DRAW PLAYER (Mortimer) ---
        glBindVertexArray(circleVAO);
        glUniform3f(ColorID, 0.2f, 0.2f, 0.2f); // Dark grey body
        glm::mat4 playerBase = glm::translate(glm::mat4(1.0f), glm::vec3(playerPos.x, playerPos.y, 0.0f));

        glm::mat4 headMVP = Projection * glm::scale(playerBase, glm::vec3(playerRadius, playerRadius, 1.0f));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &headMVP[0][0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);

        glm::mat4 leftEar = glm::translate(playerBase, glm::vec3(-10.0f, 10.0f, 0.0f));
        glm::mat4 leftEarMVP = Projection * glm::scale(leftEar, glm::vec3(8.0f, 8.0f, 1.0f));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &leftEarMVP[0][0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);

        glm::mat4 rightEar = glm::translate(playerBase, glm::vec3(10.0f, 10.0f, 0.0f));
        glm::mat4 rightEarMVP = Projection * glm::scale(rightEar, glm::vec3(8.0f, 8.0f, 1.0f));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &rightEarMVP[0][0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);

        // --- DRAW SPIDERS & PATROL LOGIC ---
        glUniform3f(ColorID, 0.8f, 0.1f, 0.1f); // Red body

        for (size_t s = 0; s < spiders.size(); s++) {

            int numPoints = spiders[s].waypoints.size();
            float t = timeElapsed * spiders[s].speed;

            float cycle = fmod(t, (float)numPoints);
            int currentIndex = (int)cycle;
            int nextIndex = (currentIndex + 1) % numPoints;
            float fraction = cycle - currentIndex;

            glm::vec2 p1 = spiders[s].waypoints[currentIndex];
            glm::vec2 p2 = spiders[s].waypoints[nextIndex];
            glm::vec2 currentSpiderPos = p1 + fraction * (p2 - p1);

            // DEATH LOGIC
            if (!isGameOver) {
                if (CheckCircleCollision(playerPos, playerRadius, currentSpiderPos, spiderRadius)) {
                    printf("\n========================================\n");
                    printf(" YOU WERE EATEN BY A SPIDER! GAME OVER.\n");
                    printf(" Press 'R' to restart from Level 1.\n");
                    printf("========================================\n");
                    isGameOver = true;
                }
            }

            // Draw Spider Body
            glBindVertexArray(circleVAO);
            glm::mat4 spiderBase = glm::translate(glm::mat4(1.0f), glm::vec3(currentSpiderPos.x, currentSpiderPos.y, 0.0f));
            glm::mat4 spiderMVP = Projection * glm::scale(spiderBase, glm::vec3(spiderRadius, spiderRadius, 1.0f));
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &spiderMVP[0][0]);
            glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);

            // Draw Legs
            glBindVertexArray(squareVAO);
            float legAngle = sinf(timeElapsed * 15.0f) * 0.5f;

            for (int i = 0; i < 4; i++) {
                glm::mat4 legR = glm::translate(spiderBase, glm::vec3(10.0f, 10.0f - (i * 6.0f), 0.0f));
                legR = glm::rotate(legR, (i % 2 == 0) ? legAngle : -legAngle, glm::vec3(0, 0, 1));
                legR = glm::translate(legR, glm::vec3(0.0f, -2.0f, 0.0f));
                glm::mat4 legRMVP = Projection * glm::scale(legR, glm::vec3(25.0f, 4.0f, 1.0f));
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &legRMVP[0][0]);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                glm::mat4 legL = glm::translate(spiderBase, glm::vec3(-10.0f, 10.0f - (i * 6.0f), 0.0f));
                legL = glm::rotate(legL, (i % 2 == 0) ? -legAngle : legAngle, glm::vec3(0, 0, 1));
                legL = glm::translate(legL, glm::vec3(-25.0f, -2.0f, 0.0f));
                glm::mat4 legLMVP = Projection * glm::scale(legL, glm::vec3(25.0f, 4.0f, 1.0f));
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &legLMVP[0][0]);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}