#include "app.h"


// set default app size; is overwritten from Python call anyway
unsigned int WIDTH = 800;
unsigned int HEIGHT = 600;

#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <sstream>
#include <vector>
#include <iomanip>

#define TEST_ERROR(_msg)        \
ALCenum error = alGetError();       \
if (error != AL_NO_ERROR) { \
    fprintf(stderr, _msg "\n"); \
}

struct getTime
{
    using duration   = std::chrono::steady_clock::duration;
    using rep        = duration::rep;
    using period     = duration::period;
    using time_point = std::chrono::time_point<getTime>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept
    {
        using namespace std::chrono;
        static auto epoch = steady_clock::now();
        return time_point{steady_clock::now() - epoch};
    }
};


Scene::Scene() {

}

Application::Application() {

};


float Application::getFPS() {
    return 1.0f/(deltaTime + 0.0001);
}

Application::Application(std::string title, unsigned int _WIDTH, unsigned int _HEIGHT, bool fullscreen) {

    WIDTH = _WIDTH;
    HEIGHT = _HEIGHT;

    glfwInit();
    std::cout << "initialized glfw" << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    //glfwWindowHint( GLFW_DOUBLEBUFFER, GL_FALSE );
    if (!fullscreen) {
        window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), NULL, NULL);
        std::cout << window << std::endl;
        assert(window != NULL);
    } else {
        window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), glfwGetPrimaryMonitor(), NULL);
        std::cout << window << std::endl;
        assert(window != NULL);

    }

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    } 
    
    if (!GLAD_GL_ARB_arrays_of_arrays) {
        std::cout << "GL_ARB_arrays_of_arrays not supported!" << std::endl; 
    }

    setup(title, WIDTH, HEIGHT, fullscreen);

}

// Use in conjunction with Application() to create an instance but do not create a new window
// useful for drawing to different contexts/windows of OpenGL

int Application::setup(std::string title, unsigned int WIDTH, unsigned int HEIGHT, bool fullscreen) {

    lastX = WIDTH / 2.0f;
    lastY = HEIGHT / 2.0f;
    std::cout << "constructor in C++" << std::endl;

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glEnable(GL_MULTISAMPLE);
    active_camera._setup(vec3(0.0, 0.0, 10.0), vec3(0.0, 0.0, -1.0), vec3(0.0, 1.0, 0.0), -90.0f, 0.0f);
    active_camera.projection_matrix = perspective(45.0f, float(WIDTH)/float(HEIGHT), 0.1f, 1000.0f);

    return 0;
}

void Application::drawScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (StaticObject::modelRegistry.size() > 0) {

        // frustum cull objects
        for (unsigned int i = 0; i < StaticObject::modelRegistry.size(); i++) {
            for (unsigned int j = 0; j < StaticObject::modelRegistry[i].size(); j++) {
                frustum.cullStaticObject(StaticObject::modelRegistry[i][j]);
            }
        }

        StaticModel::shader.use();
        StaticModel::shader.setMat4("projection", active_camera.projection_matrix);
        StaticModel::shader.setMat4("view", active_camera.view_matrix);

        StaticObject::drawAllObjects();   
    }     


    if (AnimatedObject::modelRegistry.size() > 0) {
        
        // frustum cull objects TO DO generate dynamic bounding box
        // for (unsigned int i = 0; i < AnimatedObject::modelRegistry.size(); i++) {
        //     for (unsigned int j = 0; j < AnimatedObject::modelRegistry[i].size(); j++) {
        //         frustum.cullAnimatedObject(AnimatedObject::modelRegistry[i][j]);
        //     }
        // }

        AnimatedModel::shader.use();
        AnimatedModel::shader.setMat4("projection", active_camera.projection_matrix);
        AnimatedModel::shader.setMat4("view", active_camera.view_matrix);

        AnimatedObject::drawAllObjects(currentFrame); 
       
    }

}

void Application::drawUI() {
 

}

void Application::update() {

    currentFrame = (float)getTime::now().time_since_epoch().count()/1000000000.0f; // convert nanoseconds to seconds

    deltaTime = (float)(currentFrame - lastFrame);
    lastFrame = currentFrame;

}

int Application::gameLoop() {

    update();


    // // update scene cameras view matrix (ie. GetViewMatrix())
    active_camera.view_matrix = lookAt(active_camera.Position, active_camera.Position + active_camera.Front, vec3(0,1,0));

    // draw scene to gbuffer
    //screen_fbo.bind(true);
    frustum = Frustum(active_camera.fov, 0.1f, 1000.0f, WIDTH, HEIGHT, active_camera);

    drawScene();
    drawUI();

    return 1;
}

Application::~Application() {

}

void Application::onKeyPressed(int key, int scancode, int action, int mods){

    // for (unsigned int i = 0; i < StaticModel::models.size(); i++) {
    //     StaticModel *statModel = StaticModel::models.at(i);
    //     statModel->onKeyPressed(key, scancode, action, mods);
    // }
};

