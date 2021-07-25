#include "app.h"
#include <math.h>
#include <random>

extern unsigned int WIDTH;
extern unsigned int HEIGHT;

class Demo : public Application {
    float offset = 3.0f;
public:
    //std::unique_ptr<StaticModel> axis_3d; // TO DO
    Demo(std::string title, unsigned int WIDTH, unsigned int HEIGHT, bool fullscreen)
        : Application(title, WIDTH, HEIGHT, fullscreen) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        active_camera.Position = vec3(5,6,5);
    }

    void update() override {
        Application::update();  // TO DO
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            active_camera.ProcessKeyboard(0, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            active_camera.ProcessKeyboard(1, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            active_camera.ProcessKeyboard(2, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            active_camera.ProcessKeyboard(3, deltaTime);
        }
        AnimatedObject::drawAllObjects(currentFrame);
        //std::cout << "FPS =" << 1.0/(deltaTime) << std::endl;
    }

    void onMouseMoved(double xpos, double ypos) override {
        double xoffset = xpos - lastX;
        double yoffset = lastY - ypos;

        lastX = xpos;
        lastY = ypos;
        active_camera.ProcessMouseMovement(xoffset, yoffset, true);
    }

    void onKeyPressed(int key, int scancode, int action, int mods) override {

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> dis(0.0, 1.0);

        if (key == GLFW_KEY_1 && action == 1) {
            offset += 3;
            AnimatedObject tmp("./data/astroboy.dae");
            mat4 translationMat = glm::translate(glm::mat4(1.0), glm::vec3(offset,3,0));
            tmp.setModelMatrix(glm::rotate(translationMat, glm::radians(-90.0f), glm::vec3(1,0,0)));
            tmp.setFrames(0.0, 1.0, dis(gen));            
        }

        if (key == GLFW_KEY_2 && action == 1) {
            offset += 3;
            AnimatedObject tmp("./data/steve.dae");
            mat4 translationMat = glm::translate(glm::mat4(1.0), glm::vec3(offset,7,0));
            tmp.setModelMatrix(glm::scale(glm::rotate(translationMat, glm::radians(-90.0f), glm::vec3(1,0,0)), vec3(0.5, 0.5, 0.5)));
            tmp.setFrames(0.0, 1.0, dis(gen));            
        }

        if (key == GLFW_KEY_3 && action == 1) {
            offset += 3;
            AnimatedObject tmp("./data/player.fbx");

            std::vector<std::vector<float>> vec = {{2.85, 3.5},{3.65, 4.3},{5.32, 6},{4.5, 5.15},{0.2, 2.7}};

            std::random_device random_dev;
            std::mt19937       generator(random_dev());

            std::shuffle(vec.begin(), vec.end(), generator);

            mat4 translationMat = glm::translate(glm::mat4(1.0), glm::vec3(offset,3,0));
            tmp.setModelMatrix(glm::scale(glm::translate(mat4(1.0f), glm::vec3(offset, 3, 0)), vec3(0.03, 0.03, 0.03)));
            tmp.setFrames(vec[0][0], vec[0][1], dis(gen));     

        }
    }
};

Application *getApplication() {
    return new Demo("demo", WIDTH, HEIGHT, false);
}
Application *app;


static void onMouseClicked(GLFWwindow* window, int button, int action, int mods)
{
    app->onMouseClicked(button, action, mods);
}
static void onKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    app->onKeyPressed(key, scancode, action, mods);
}
static void onWindowResized(GLFWwindow* window, int width, int height)
{
    glViewport(0,0, width, height);
    app->onWindowResized(width, height);
}
static void onMouseMoved(GLFWwindow* window, double xpos, double ypos)
{
    app->onMouseMoved(xpos, ypos);
}

static void onWindowClose(GLFWwindow* window) {
    app->onWindowClose(window);
}

void joystick_callback(int jid, int event)
{
    if (event == GLFW_CONNECTED)
    {
        std::cout <<" joystick connected" << std::endl;
        // The joystick was connected
    }
    else if (event == GLFW_DISCONNECTED)
    {
        // The joystick was disconnected
        std::cout <<" joystick disconnected" << std::endl;
    }
}


int main(int argc, char** argv)
{
    app = getApplication();
    glfwSetJoystickCallback(joystick_callback);
    glfwSetKeyCallback(app->window, onKeyPressed);
    glfwSetCursorPosCallback(app->window, onMouseMoved);
    glfwSetMouseButtonCallback(app->window, onMouseClicked);
    glfwSetFramebufferSizeCallback(app->window, onWindowResized);
    glfwSetWindowCloseCallback(app->window, onWindowClose);
    while(!glfwWindowShouldClose(app->window))
    {
        app->gameLoop();
        glfwSwapBuffers(app->window);
        glfwPollEvents();    
    }
    glfwDestroyWindow(app->window);
    glfwTerminate();
}






