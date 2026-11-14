#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring> 

#include "dependente/glew/glew.h"
#include "dependente/glfw/glfw3.h"
#include "dependente/glm/glm.hpp"
#include "dependente/glm/gtc/matrix_transform.hpp"
#include "dependente/glm/gtc/type_ptr.hpp"

#include "shader.hpp"

// Game state and player configuration
bool isGameOver = false;
int currentLevel = 1;

glm::vec2 playerPos(75.0f, 25.0f);
float playerRadius = 15.0f;
float blockSize = 50.0f;
float timeElapsed = 0.0f;

// Health and i-frames (invulnerability frames) system
int maxHealth = 2;
int playerHealth = 2;
float invulnerabilityTimer = 0.0f;
const float INVULNERABILITY_DURATION = 1.0f;

// Spider enemies patrol between multiple waypoints
struct Spider {
    std::vector<glm::vec2> waypoints;
    float speed;
};

std::vector<Spider> spiders;
float spiderRadius = 18.0f;

int maze[12][16];

// Initializes player position, timers, map layout, and enemy patrol routes for the level
void StartLevel(int level) {
    // Reset player to starting tile and clear round timers
    playerPos = glm::vec2(75.0f, 75.0f);
    timeElapsed = 0.0f;
    isGameOver = false;
    invulnerabilityTimer = 0.0f;

    if (level == 1) {
        // Level 1 Map (1 = Wall, 0 = Walkable Path)
        int map1[12][16] = {
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
            { 1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1 },
            { 1,0,1,0,1,0,1,1,1,0,1,0,1,1,0,1 },
            { 1,0,1,0,0,0,0,0,1,0,0,0,1,0,0,1 },
            { 1,0,1,1,1,1,1,0,1,1,1,0,1,0,1,1 },
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
            { 1,1,1,1,0,1,1,1,1,1,0,1,1,1,0,1 },
            { 1,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1 },
            { 1,0,1,1,1,1,0,1,1,1,1,1,0,1,0,1 },
            { 1,0,0,0,0,0,0,1,1,1,1,1,0,0,0,1 },
            { 1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 }
        };
        // Copy active level grid into the global maze array
        std::memcpy(maze, map1, sizeof(maze));

        spiders = {
            { { glm::vec2(175.0f, 175.0f), glm::vec2(375.0f, 175.0f) }, 1.6f },
            { { glm::vec2(75.0f, 275.0f), glm::vec2(725.0f, 275.0f) }, 1.7f },
            { { glm::vec2(75.0f, 475.0f), glm::vec2(325.0f, 475.0f) }, 1.2f }
        };
    }
    else {
        // Level 2 Map 
        int map2[12][16] = {
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
            { 1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1 },
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
            { 1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1 },
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
            { 1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1 },
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
            { 1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1 },
            { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 }
        };
        std::memcpy(maze, map2, sizeof(maze));

        spiders = {
            { { glm::vec2(175.0f, 75.0f), glm::vec2(725.0f, 75.0f) }, 1.0f },
            { { glm::vec2(625.0f, 175.0f), glm::vec2(75.0f, 175.0f) }, 1.0f },
            { { glm::vec2(175.0f, 275.0f), glm::vec2(725.0f, 275.0f) }, 1.0f },
            { { glm::vec2(625.0f, 375.0f), glm::vec2(75.0f, 375.0f) }, 1.0f },
            { { glm::vec2(175.0f, 475.0f), glm::vec2(725.0f, 475.0f) }, 1.0f }
        };
    }
}

GLuint squareVAO, circleVAO;
int circleVertexCount = 0;

// Builds a unit square mesh using two triangles and uploads it to the GPU
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

    // Generate VAO and VBO, then send vertex coordinates to GPU memory
    glGenVertexArrays(1, &squareVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// Approximates a unit circle using a 36-segment triangle fan on the GPU
void CreateCircle() {
    std::vector<float> vertices;
    int segments = 36;
    // Start with the center point for GL_TRIANGLE_FAN
    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
    // Calculate perimeter points around 360 degrees using sine and cosine
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

// Detects overlap between a circle (player) and an axis-aligned rectangle (wall tile)
bool CheckCollision(glm::vec2 circleCenter, float radius, glm::vec2 aabbMin, glm::vec2 aabbMax) {
    // Clamp circle center to box bounds to find closest point on the wall
    float closestX = std::max(aabbMin.x, std::min(circleCenter.x, aabbMax.x));
    float closestY = std::max(aabbMin.y, std::min(circleCenter.y, aabbMax.y));
    float distanceX = circleCenter.x - closestX;
    float distanceY = circleCenter.y - closestY;

    // Compare squared distance against slightly shrunk radius to avoid snagging on corners
    return (distanceX * distanceX + distanceY * distanceY) < ((radius * 0.92f) * (radius * 0.92f));
}

// Checks if two circles (player and spider) intersect using squared distance
bool CheckCircleCollision(glm::vec2 centerA, float radiusA, glm::vec2 centerB, float radiusB) {
    float dx = centerA.x - centerB.x;
    float dy = centerA.y - centerB.y;
    float distanceSquared = (dx * dx) + (dy * dy);
    float radiusSum = radiusA + radiusB;
    return distanceSquared < (radiusSum * radiusSum);
}

// Entry point: initializes OpenGL window, shaders, buffers, and runs the main game loop
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

    // Set up 2D camera 
    glm::mat4 Projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint ColorID = glGetUniformLocation(programID, "objectColor");

    float lastTime = glfwGetTime();

    playerHealth = maxHealth;
    StartLevel(currentLevel);

    while (!glfwWindowShouldClose(window)) {

        // Calculate frame time delta 
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (!isGameOver) {
            timeElapsed += deltaTime;
            float speed = 350.0f * deltaTime;

            // Count down invulnerability timer after taking damage
            if (invulnerabilityTimer > 0.0f) {
                invulnerabilityTimer -= deltaTime;
                if (invulnerabilityTimer < 0.0f) invulnerabilityTimer = 0.0f;
            }

            // Test horizontal movement separately so player slides smoothly along walls
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

            // Test vertical movement separately after horizontal position is resolved
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

            // Advance to next level or trigger victory when reaching top exit
            if (playerPos.y > 580.0f) {
                if (currentLevel < 2) {
                    currentLevel++;
                    StartLevel(currentLevel);
                }
                else {
                    printf("YOU WON\n");
                    isGameOver = true;
                }
            }
        }
        else {
            // Reset game state back to level 1 when R is pressed
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                currentLevel = 1;
                playerHealth = maxHealth;
                StartLevel(currentLevel);
            }
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        if (isGameOver) {
            glClearColor(0.2f, 0.0f, 0.4f, 1.0f);
        }
        else {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(programID);

        // Render all solid wall blocks in the maze grid
        glBindVertexArray(squareVAO);
        glUniform3f(ColorID, 0.4f, 0.4f, 0.5f);
        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 16; j++) {
                if (maze[i][j] == 1) {
                    // Position and scale each unit square to match grid tile size
                    glm::mat4 Model = glm::translate(glm::mat4(1.0f), glm::vec3(j * blockSize, i * blockSize, 0.0f));
                    Model = glm::scale(Model, glm::vec3(blockSize, blockSize, 1.0f));
                    glm::mat4 MVP = Projection * Model;

                    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }

        // Calculate wobble angle while player is moving
        float wobbleAngle = 0.0f;
        if (!isGameOver && (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)) {
            wobbleAngle = sinf(timeElapsed * 20.0f) * 20.0f;
        }

        // Toggle red warning color rapidly during invulnerability frames
        bool shouldFlash = (invulnerabilityTimer > 0.0f) && (fmod(invulnerabilityTimer * 10.0f, 2.0f) > 1.0f);

        glBindVertexArray(circleVAO);
        if (shouldFlash) {
            glUniform3f(ColorID, 0.9f, 0.2f, 0.2f);
        }
        else {
            glUniform3f(ColorID, 0.2f, 0.2f, 0.2f);
        }

        // Build parent transform for player so ears inherit position and wobble
        glm::mat4 playerBase = glm::translate(glm::mat4(1.0f), glm::vec3(playerPos.x, playerPos.y, 0.0f));
        playerBase = glm::rotate(playerBase, wobbleAngle, glm::vec3(0, 0, 1));

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

        // Update and draw each patrolling spider
        for (size_t s = 0; s < spiders.size(); s++) {

            // Interpolate spider position linearly between current and next patrol waypoints
            int numPoints = spiders[s].waypoints.size();
            float t = timeElapsed * spiders[s].speed;

            float cycle = fmod(t, (float)numPoints);
            int currentIndex = (int)cycle;
            int nextIndex = (currentIndex + 1) % numPoints;
            float fraction = cycle - currentIndex;

            glm::vec2 p1 = spiders[s].waypoints[currentIndex];
            glm::vec2 p2 = spiders[s].waypoints[nextIndex];
            glm::vec2 currentSpiderPos = p1 + fraction * (p2 - p1);

            // Check enemy collision and apply damage if player is not invulnerable
            if (!isGameOver) {
                if (CheckCircleCollision(playerPos, playerRadius, currentSpiderPos, spiderRadius)) {
                    if (invulnerabilityTimer <= 0.0f) {
                        playerHealth--;
                        invulnerabilityTimer = INVULNERABILITY_DURATION;

                        if (playerHealth <= 0) {
                            printf("YOU LOST\n");
                            isGameOver = true;
                        }
                    }
                }
            }

            // Draw spider body at waypoint position
            glBindVertexArray(circleVAO);
            glUniform3f(ColorID, 0.8f, 0.1f, 0.1f);
            glm::mat4 spiderBase = glm::translate(glm::mat4(1.0f), glm::vec3(currentSpiderPos.x, currentSpiderPos.y, 0.0f));
            glm::mat4 spiderMVP = Projection * glm::scale(spiderBase, glm::vec3(spiderRadius, spiderRadius, 1.0f));
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &spiderMVP[0][0]);
            glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);

            // Rotate the square around  center while following spiderBase
            glBindVertexArray(squareVAO);
            glUniform3f(ColorID, 1.0f, 0.8f, 0.0f);

            glm::mat4 spiderCore = glm::rotate(spiderBase, timeElapsed * -180.0f, glm::vec3(0, 0, 1));
            spiderCore = glm::scale(spiderCore, glm::vec3(12.0f, 12.0f, 1.0f));
            spiderCore = glm::translate(spiderCore, glm::vec3(-0.5f, -0.5f, 0.0f));
            glm::mat4 coreMVP = Projection * spiderCore;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &coreMVP[0][0]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Draw top-left health squares (red for active, gray for lost)
        glBindVertexArray(squareVAO);
        for (int h = 0; h < maxHealth; h++) {
            if (h < playerHealth) {
                glUniform3f(ColorID, 1.0f, 0.2f, 0.2f);
            }
            else {
                glUniform3f(ColorID, 0.3f, 0.3f, 0.3f);
            }
            glm::mat4 heartModel = glm::translate(glm::mat4(1.0f), glm::vec3(20.0f + (h * 30.0f), 570.0f, 0.0f));
            heartModel = glm::scale(heartModel, glm::vec3(20.0f, 20.0f, 1.0f));
            glm::mat4 heartMVP = Projection * heartModel;
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &heartMVP[0][0]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
