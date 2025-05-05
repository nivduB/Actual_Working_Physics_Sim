#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

float screenWidth = 400.0f;
float screenHeight = 300.0f;

// Simulation scaling factors since the computer sim is from -1 to 1 
float SPACE_SCALE = 1.0e-9f;  // 1 unit on screen = 1 billion meters in reality
float TIME_SCALE = 50000.0f;  // Each frame represents 50,000 seconds (about 14 hours)

GLFWwindow* StartGLFW();

class Object
{
public:
    std::vector<float> position;    // Stored in simulation space
    std::vector<float> velocity;    // Stored in m/s (will be scaled when updating position)
    float radius;                   // Visual radius on screen
    float mass;                     // Mass in kg
    float realRadius;               // Actual radius in meters
    
    Object(std::vector<float> position, std::vector<float> velocity, float radius, float mass, float realRadius)
    {
        this->position = position;
        this->velocity = velocity;
        this->radius = radius;
        this->mass = mass;
        this->realRadius = realRadius;
    }

    void accelerate(float x, float y)
    {
        this->velocity[0] += x * TIME_SCALE;
        this->velocity[1] += y * TIME_SCALE;
    }

    void updatePosition()
    {
        // Scale velocity by time scale and space scale
        this->position[0] += this->velocity[0] * TIME_SCALE * SPACE_SCALE;
        this->position[1] += this->velocity[1] * TIME_SCALE * SPACE_SCALE;
    }
    
    void CheckCollision(Object& other){
        float dx = other.position[0] - this->position[0];
        float dy = other.position[1] - this->position[1];
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // Use screen-space radius for collision detection
        if(other.radius + this->radius > distance){
            // Elastic collision
            this->velocity[0] *= (-1.0f);
            this->velocity[1] *= (-1.0f);
            other.velocity[0] *= -1.0;
            other.velocity[1] *= -1.0;
        }
    }
    
    void DrawCircle(float centerX, float centerY, float radius, int res)
    {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(centerX, centerY);
        
        for (int i = 0; i <= res; ++i) {
            float angle = 2.0f * M_PI * (static_cast<float>(i) / res);
            float x = centerX + cos(angle) * radius;
            float y = centerY + sin(angle) * radius;
            glVertex2f(x, y);
        }
        glEnd();
    }
};

// Create real-world objects with real masses
// Earth: mass = 5.97e24 kg, radius = 6371 km
// Moon: mass = 7.35e22 kg, radius = 1737 km
// Distance Earth-Moon = ~384,400 km
std::vector<Object> objs = {
    // Earth - at center
    Object(
        std::vector<float>{0.0f, 0.0f},      // Position
        std::vector<float>{0.0f, 0.0f},      // Initial velocity
        0.1f,                                // Display radius on screen
        5.97e24f,                            // Mass in kg (Earth)
        6371000.0f                           // Real radius in meters
    ),
    // Moon - starting to the right
    Object(
        std::vector<float>{0.384f, 0.0f},    // Position (384,400 km scaled)
        std::vector<float>{0.0f, 1022.0f},   // Orbital velocity ~1022 m/s
        0.05f,                               // Display radius on screen
        7.35e22f,                            // Mass in kg (Moon)
        1737000.0f                           // Real radius in meters
    )
};

int main(){
    std::cout << "Starting physics simulation with real-world values\n";
    
    // Real gravitational constant
    float G = 6.674e-11f;
    
    GLFWwindow *window = StartGLFW();
    if (!window) {
        return -1;
    }
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    
    float simulationTime = 0.0f;
    
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Update simulation time
        simulationTime += TIME_SCALE;
        std::cout << "Simulation time: " << simulationTime / 86400.0f << " days\r" << std::flush;
        
        // Calculate forces and update positions
        for (Object &obj: objs){
            // Calculate gravitational forces from all other objects
            for(Object &obj2: objs){
                if(&obj2 == &obj){
                    continue;
                }
                
                // Calculate distance in simulation space
                float dx = obj2.position[0] - obj.position[0];
                float dy = obj2.position[1] - obj.position[1];
                float distanceSim = std::sqrt(dx * dx + dy * dy);
                
                // Convert to real distance (divide by space scale)
                float distanceReal = distanceSim / SPACE_SCALE;
                
                // Avoid division by zero or very small values
                if (distanceReal < (obj.realRadius + obj2.realRadius)) {
                    continue;
                }
                
                // Unit direction vector
                std::vector<float> direction = {dx / distanceSim, dy / distanceSim};
                
                // Calculate force using real values
                float force = (G * obj.mass * obj2.mass) / (distanceReal * distanceReal);
                
                // Calculate acceleration (F = ma)
                float acceleration = force / obj.mass;
                
                // Apply acceleration
                obj.accelerate(acceleration * direction[0], acceleration * direction[1]);
            }
            
            // Update position
            obj.updatePosition();
            
            // Draw object
            glColor3f(1.0f, 1.0f, 1.0f); // White color
            obj.DrawCircle(obj.position[0], obj.position[1], obj.radius, 50);
            
            // Handle screen boundaries (optional - may not be needed for orbital simulation)
            if (obj.position[1] - obj.radius < -1.0f)
            {
                obj.position[1] = -1.0f + obj.radius;
                obj.velocity[1] = -obj.velocity[1];
            } 
            else if (obj.position[1] + obj.radius > 1.0f) {
                obj.position[1] = 1.0f - obj.radius;
                obj.velocity[1] = -obj.velocity[1];
            }
            
            if (obj.position[0] - obj.radius < -1.0f) {
                obj.position[0] = -1.0f + obj.radius;
                obj.velocity[0] = -obj.velocity[0];
            } 
            else if (obj.position[0] + obj.radius > 1.0f) {
                obj.position[0] = 1.0f - obj.radius;
                obj.velocity[0] = -obj.velocity[0];
            }
            
            // Check for collisions
            for (size_t j = 0; j < objs.size(); j++){
                if(&obj != &objs[j]){
                    obj.CheckCollision(objs[j]);
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

// GLFW setup function unchanged
GLFWwindow* StartGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return nullptr;
    }
    
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "physics_sim", NULL, NULL);
    
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }
    
    return window;
}