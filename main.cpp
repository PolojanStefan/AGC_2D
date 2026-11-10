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

bool isGameOver = false;
int currentLevel = 1;

glm::vec2 playerPos(75.0f, 25.0f);
float playerRadius = 15.0f;
float blockSize = 50.0f;
float timeElapsed = 0.0f;

int maxHealth = 2;
int playerHealth = 2;
float invulnerabilityTimer = 0.0f;
const float INVULNERABILITY_DURATION = 1.0f;

struct Spider {
    std::vector<glm::vec2> waypoints;
    float speed;
};

std::vector<Spider> spiders;
float spiderRadius = 18.0f;

int maze[12][16];

void StartLevel(int level) {
    playerPos = glm::vec2(75.0f, 75.0f);
    timeElapsed = 0.0f;
    isGameOver = false;
    invulnerabilityTimer = 0.0f;

    if (level == 1) {
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
        std::memcpy(maze, map1, sizeof(maze));

        spiders = {
            { { glm::vec2(175.0f, 175.0f), glm::vec2(375.0f, 175.0f) }, 1.6f },
            { { glm::vec2(75.0f, 275.0f), glm::vec2(725.0f, 275.0f) }, 1.7f },
            { { glm::vec2(75.0f, 475.0f), glm::vec2(325.0f, 475.0f) }, 1.2f }
        };
    }
    else {
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

bool CheckCollision(glm::vec2 circleCenter, float radius, glm::vec2 aabbMin, glm::vec2 aabbMax) {
    float closestX = std::max(aabbMin.x, std::min(circleCenter.x, aabbMax.x));
    float closestY = std::max(aabbMin.y, std::min(circleCenter.y, aabbMax.y));
    float distanceX = circleCenter.x - closestX;
    float distanceY = circleCenter.y - closestY;

    return (distanceX * distanceX + distanceY * distanceY) < ((radius * 0.92f) * (radius * 0.92f));
}

bool CheckCircleCollision(glm::vec2 centerA, float radiusA, glm::vec2 centerB, float radiusB) {
    float dx = centerA.x - centerB.x;
    float dy = centerA.y - centerB.y;
    float distanceSquared = (dx * dx) + (dy * dy);
    float radiusSum = radiusA + radiusB;
    return distanceSquared < (radiusSum * radiusSum);
}

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

    playerHealth = maxHealth;
    StartLevel(currentLevel);

    bool nKeyWasPressed = false;

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (!isGameOver) {
            timeElapsed += deltaTime;
            float speed = 350.0f * deltaTime;

            if (invulnerabilityTimer > 0.0f) {
                invulnerabilityTimer -= deltaTime;
                if (invulnerabilityTimer < 0.0f) invulnerabilityTimer = 0.0f;
            }

            bool nKeyIsPressed = (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS);
            if (nKeyIsPressed && !nKeyWasPressed) {
                if (currentLevel < 2) {
                    currentLevel++;
                    StartLevel(currentLevel);
                }
                else {
                    printf("YOU WON\n");
                    isGameOver = true;
                }
            }
            nKeyWasPressed = nKeyIsPressed;

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

        glBindVertexArray(squareVAO);
        glUniform3f(ColorID, 0.4f, 0.4f, 0.5f);
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

        if (!isGameOver) {
            glBindVertexArray(squareVAO);
            glUniform3f(ColorID, 0.0f, 1.0f, 0.5f);

            glm::mat4 portalBase = glm::translate(glm::mat4(1.0f), glm::vec3(14 * blockSize + 25.0f, 11 * blockSize + 25.0f, 0.0f));
            portalBase = glm::rotate(portalBase, timeElapsed * 120.0f, glm::vec3(0, 0, 1));

            float pulseScale = 1.0f + 0.3f * sinf(timeElapsed * 4.0f);
            portalBase = glm::scale(portalBase, glm::vec3(25.0f * pulseScale, 25.0f * pulseScale, 1.0f));
            portalBase = glm::translate(portalBase, glm::vec3(-0.5f, -0.5f, 0.0f));

            glm::mat4 portalMVP = Projection * portalBase;
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &portalMVP[0][0]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        float wobbleAngle = 0.0f;
        if (!isGameOver && (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)) {
            wobbleAngle = sinf(timeElapsed * 20.0f) * 20.0f;
        }

        bool shouldFlash = (invulnerabilityTimer > 0.0f) && (fmod(invulnerabilityTimer * 10.0f, 2.0f) > 1.0f);

        glBindVertexArray(circleVAO);
        if (shouldFlash) {
            glUniform3f(ColorID, 0.9f, 0.2f, 0.2f);
        }
        else {
            glUniform3f(ColorID, 0.2f, 0.2f, 0.2f);
        }

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

            glBindVertexArray(circleVAO);
            glUniform3f(ColorID, 0.8f, 0.1f, 0.1f);
            glm::mat4 spiderBase = glm::translate(glm::mat4(1.0f), glm::vec3(currentSpiderPos.x, currentSpiderPos.y, 0.0f));
            glm::mat4 spiderMVP = Projection * glm::scale(spiderBase, glm::vec3(spiderRadius, spiderRadius, 1.0f));
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &spiderMVP[0][0]);
            glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);

            glBindVertexArray(squareVAO);
            glUniform3f(ColorID, 1.0f, 0.8f, 0.0f);

            glm::mat4 spiderCore = glm::rotate(spiderBase, timeElapsed * -180.0f, glm::vec3(0, 0, 1));
            spiderCore = glm::scale(spiderCore, glm::vec3(12.0f, 12.0f, 1.0f));
            spiderCore = glm::translate(spiderCore, glm::vec3(-0.5f, -0.5f, 0.0f));
            glm::mat4 coreMVP = Projection * spiderCore;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &coreMVP[0][0]);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindVertexArray(squareVAO);
            glUniform3f(ColorID, 0.8f, 0.1f, 0.1f);

            float legAngle = sinf(timeElapsed * 15.0f) * 35.0f;

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