#include <glad/glad.h>
#include <GLFW/glfw3.h> 
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <hidapi/hidapi.h>

#include "shader.hpp"
#include "model.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void glfw_error_callback(int code, const char *desc) {
    std::cerr << "GLFW Error - " << code << ": " << desc << std::endl;
}

GLFWwindow *init_win()
{
    glfwSetErrorCallback(glfw_error_callback);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "GLTest", NULL, NULL);
    if(!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return NULL;
    }    

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    glEnable(GL_DEPTH_TEST);

    return window;
}


glm::quat imu_read_gyro(hid_device *dev_hndl, uint8_t *buf) {
    const float QUAT_SCALE = 10000.f;

    hid_read(dev_hndl, (unsigned char*)buf, 17);

    int32_t quat[4];
    memcpy(quat, buf+1, 16);

    glm::quat ret;
    ret.x = (float)quat[0] / QUAT_SCALE;
    ret.y = (float)quat[1] / QUAT_SCALE;
    ret.z = (float)quat[2] / QUAT_SCALE;
    ret.w = (float)quat[3] / QUAT_SCALE;

    return ret;
}

int main() 
{
    uint8_t hidbuf[17] = {0};

    hid_init();
    hid_device *dev_hndl = hid_open(0x1b4f, 0x9206, NULL);

    if(dev_hndl == NULL) {
        printf("Connection failed\n");
        exit(1);
    }
    
    hidbuf[0] = 0x0;
    hidbuf[1] = 0x81;
    hid_write(dev_hndl, (unsigned char*)hidbuf, 17);

    GLFWwindow *window = init_win();
    if(!window)
        return -1;

    Renderer::Shader defaultShader;
    if(defaultShader.load("shaders/default.vert", "shaders/default.frag"))
        return -1;

    Renderer::Model model;
    if(model.load("models/cube.obj"))
        return -1;

    for(size_t i = 0; i < model.meshes.size(); i++) {
        if(model.meshes[i].loadTexture("textures/test.png"))
            return -1;
    }

    glm::mat4 matProjection, matView, matModel, matLocal;
    glm::vec3 CameraPos = glm::vec3(0,0,-3.f);

    while(!glfwWindowShouldClose(window)) {
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        int winw, winh;
        glfwGetWindowSize(window, &winw, &winh);
        matProjection = glm::perspective(glm::radians(90.0f), (float)winw / (float)winh, 0.1f, 100.0f); 
    
        // Camera handling
        matView = glm::mat4(1.0f);
        //matView = glm::rotate(matView, CameraRot.y, glm::vec3(1,0,0));
        //matView = glm::rotate(matView, CameraRot.x, glm::vec3(0,1,0));
        matView = glm::translate(matView, CameraPos);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        matLocal = glm::mat4(1.0f);
        //matLocal = glm::scale(matLocal, glm::vec3(0.5, 0.5, 0.5));

        matModel = glm::mat4(1.0f);
        matModel = glm::translate(matModel, glm::vec3(0.f, 0.f, -3.f));
        matModel = matModel * glm::mat4_cast(imu_read_gyro(dev_hndl, hidbuf));

        defaultShader.setMatrix4("transform", matProjection * matView * matModel * matLocal);
        model.draw(defaultShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}