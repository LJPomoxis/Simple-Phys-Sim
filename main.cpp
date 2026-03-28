#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <cmath> //sqrt
#include <vector>
#include <string>
#include <iostream> //cout, cerr
#include <fstream> // ifstream
#include <cstdlib>
#include <ctime>
#include <random>

using json = nlohmann::json;

//std::string staticObjectsFile = "..\\static_objects.json";
std::string staticObjectsFile = "..\\test.json";

/*
Normals are using clockwise winding
All shapes should be defined clockwise to be able to get correct normals

However, due to clockwise winding of normals, vertices of outer edges of bounding box need to be added in
counter-clockwise order so that the normals point "inward" instead of "outward"
*/

/*
 look at using vertices to batch draw calls
 This requires using triangles to create slices to fill a circle
 Thus is inefficient with batching, so eventually explore frag shaders
*/

/*
(world vs. render coords)

create seperation between world units and render units so that physics and static objects
can use static world coordinates instead of relying on fixed window size
(if we want a dynamic window resolution, rescaling messes with shapes. So we need a scaling system
to scale based on fixed coordinates that are decoupled from the window coordinates)
*/

const int WIDTH = 1920;
const int HEIGHT = 1200;
const int NUMBER_BALLS = 25 - 1;

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_int_distribution<> posX(0, WIDTH);
std::uniform_int_distribution<> randRad(10, 40);
std::uniform_int_distribution<> coin(0, 1);
std::uniform_int_distribution<> randHardness(20, 100);
std::uniform_int_distribution<> randDensity(5, 20);
std::uniform_int_distribution<> velocityMod(10, 35);

struct Vec2 {
    float x, y;

    Vec2(float x=0, float y=0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }

    Vec2 normalize() const;
    Vec2 get_normal() const { return Vec2{y, -x}.normalize(); }
    float dot_Product(Vec2& vertex, Vec2& object);
    Vec2 check_collision(Vec2& vertex, float objectX, float objectY, float radius);
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
};

struct Vertex {
    float x, y;

    Vertex(float x=0, float y=0) : x(x), y(y) {}
    Vertex operator+(const Vertex& other) const { return {x + other.x, y + other.y}; }
    Vertex operator-(const Vertex& other) const { return {x - other.x, y - other.y}; }

    friend std::ostream& operator<<(std::ostream& os, const Vertex& vert) {
        return os << "(" << vert.x << ", " << vert.y << ")";
    }
};

// Remove size as it will no longer be used. We will just add rects 
// to the objects file using vertices
struct Size {
    int x, y;

    friend std::ostream& operator<<(std::ostream& os, const Size& s) {
        return os << "(" << s.x << ", " << s.y << ")";
    }
};

struct BodyConfig {
    std::vector<Vertex> vertices;
    std::vector<Vec2> vectors;
    std::string type;
    Vertex position;
    Vec2 velocity;
    Vec2 acceleration;
    float mass;
    int friction;
    int bounciness;
    bool isStatic;
    bool isClosed;
};

class Body {
private:
    std::vector<Vertex> vertices;
    std::vector<Vec2> vectors;
    std::string type;
    Vertex position;
    Vec2 velocity;
    Vec2 acceleration;
    float mass;
    int friction;
    int bounciness;
    bool isStatic = true;
    bool isClosed;

    friend void from_json(const json& j, Body& b);

public:
    Body() = default;
    Body(BodyConfig config)
        : vertices(std::move(config.vertices)),
          type(std::move(config.type)),
          position(config.position),
          velocity(config.velocity),
          acceleration(config.acceleration),
          mass(config.mass),
          friction(config.friction),
          bounciness(config.bounciness),
          isStatic(config.isStatic),
          isClosed(config.isClosed)
    {}

    friend std::ostream& operator<<(std::ostream& os, const Body& body) {
        os << "Position: " << body.position << ", Velocity: " << body.velocity << ", Acceleration: " << body.acceleration;
        os << "\nMass: " << body.mass << ", Friction: " << body.friction << ", Bounciness: " << body.bounciness;
        os << "\nType: " << body.type << ", Static: " << body.isStatic << ", Closed: " << body.isClosed;
        os << "\n----------\n Vertices: [";
        for (const auto& i : body.vertices) {
            os << i << " "; 
        }
        os << "] \n----------\n Vectors: [";
        for (const auto& j : body.vectors) {
            os << j << " ";
        }
        os << "]";
        return os;
    }

    void calculateVectors();
};

class World {
private:
    std::vector<Body> bodies;
    Vec2 gravity{0, -9.18f};

public:
    //add body
    void addBody(BodyConfig& body);

    //step (step through simulation)
    void step() {
        // apply forces
        // resolve collisions
        // apply movement
    }

    const std::vector<Body>& getBodies() const { return bodies; }
};

// Separation also allows for changes in graphics libraries without touching 
// simulation code
class Renderer {
private:
    // scale is 1 unit to value of scale, in our case 1 unit = 50 pixels
    float scale = 50.0f;

public:
    void render() {
        //clear screen
        //get bodies
        //display
    }

    // HEIGHT and WIDTH need to be replaced with non-const vars eventually, so this will need updated
    Vec2 worldToRenderer (Vec2 worldPos) { return { worldPos.x * scale, HEIGHT - (worldPos.y * scale) }; }
};

void get_Window_Borders(std::vector<Body>& bodies);
float get_Velocity_Mod();
void from_json(const json& j, Vertex& v);
void from_json(const json& j, Body& b);

int main() {
    sf::Font font;
    if (!font.openFromFile("..\\Roboto_Condensed-BoldItalic.ttf")) {
        std::cerr << "Error loading font" << std::endl; 
    }

    /*
    std::ifstream edges(staticObjectsFile);
    if (!edges) {
        std::cerr << "Failes to load edges file." << std::endl;
    }

    std::vector<Body> staticBodies;

    json shapeData = json::parse(edges);
    for (const auto& shape : shapeData["shapes"]) {
        staticBodies.emplace_back(shape.get<Body>());
    }

    for (const auto& bodies : staticBodies) std::cout << bodies << "\n-----------\n";
    */

    sf::Text text(font);
    text.setCharacterSize(24);
    text.setFillColor(sf::Color::White);

    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "My window");
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    /*
    std::vector<Ball> balls;
    for (int i=0; i < NUMBER_BALLS; i++) {
        balls.emplace_back(posX(gen), randRad(gen), randDensity(gen), randHardness(gen), get_Velocity_Mod());
    }
    */

    const auto on_Close = [&window](const sf::Event::Closed&) {
        window.close();
    };

    
    const auto on_Key_Pressed = [&window /*, &balls*/](const sf::Event::KeyPressed& keyPressed) {
        if (keyPressed.scancode == sf::Keyboard::Scancode::Escape)
            window.close();

        /* Left for reference, remove when replaced
        if (keyPressed.scancode == sf::Keyboard::Scancode::R) {
            while (!balls.empty()) {
                balls.pop_back();
            }
            for (int i=0; i < NUMBER_BALLS; i++) {
                balls.emplace_back(posX(gen), randRad(gen), randDensity(gen), randHardness(gen), get_Velocity_Mod());
            }
            for (Ball& ball : balls) {
                ball.set_Ball_Pos(posX(gen));
            }
        }
        if (keyPressed.scancode == sf::Keyboard::Scancode::Space) {
            balls.emplace_back(posX(gen), randRad(gen), randDensity(gen), randHardness(gen), get_Velocity_Mod());
        }
        if (keyPressed.scancode == sf::Keyboard::Scancode::Backspace) {
            if (!balls.empty()) {
                balls.pop_back();
            } else {
                std::cout << "No objects left to delete" << std::endl;
            }
        }
        if (keyPressed.scancode == sf::Keyboard::Scancode::Delete) {
            while (!balls.empty()) {
                balls.pop_back();
            }
        }
        */
    };

    int ballCount;
    std::string textBallCount;
    while (window.isOpen()) {
        ballCount = 0;
        // check all the window's events that were triggered since the last iteration of the loop
        window.handleEvents(on_Key_Pressed, on_Close);
        window.clear(sf::Color::Black);

        textBallCount = "Number of Objects: " + std::to_string(ballCount);
        text.setString(textBallCount);
        window.draw(text);

        // end the current frame
        window.display();
    }
}

void get_Window_Borders(std::vector<Body>& bodies) {
    std::cerr << std::endl;
}

float get_Velocity_Mod() {
    return float(velocityMod(gen)) / 10.f;
}

void from_json(const json& j, Vertex& v) {
    v.x = j.at(0).get<float>();
    v.y = j.at(1).get<float>();
}

void from_json(const json& j, Body& b) {
    b.vertices = j.at("points").get<std::vector<Vertex>>();
    b.type = j.at("type").get<std::string>();
    b.isClosed = j.at("closed").get<bool>();
    b.friction = j.at("friction").get<int>();
    b.bounciness = j.at("bounciness").get<int>();

    b.calculateVectors();
}

Vec2 Vec2::normalize() const {
    float l = std::sqrt(x*x + y*y);
    return (l > 0) ? Vec2{x/l, y/l} : Vec2{0, 0};
}

// Current code doesn't account for the ends of an edge vector, it just calculates based on an infinite line
// Thus we need to clamp edge vectors based on vertices
float Vec2::dot_Product(Vec2& vertex, Vec2& object) {
    Vec2 normal = get_normal();
    Vec2 collisonVector = object - vertex;
    return (collisonVector.x * normal.x) + (collisonVector.y * normal.y);
}

Vec2 Vec2::check_collision(Vec2& vertex, float objectX, float objectY, float radius) {
    Vec2 normal = get_normal();
    Vec2 midpoint = Vec2{vertex.x + x * 0.5f, vertex.y + y * 0.5f};
    Vec2 toObject = Vec2{objectX - midpoint.x, objectY - midpoint.y};
    
    if ((toObject.x * normal.x + toObject.y * normal.y) < 0) {
        normal = Vec2{-normal.x, -normal.y};
    }

    Vec2 object{objectX, objectY};
    float product = dot_Product(vertex, object);

    //if (product < 0) {
    if (product > -radius && product < radius) {
        return Vec2{objectX - (product * normal.x), objectY - (product * normal.y)};
    }
    return Vec2{objectX, objectY};
}

void Body::calculateVectors() {
    if (vertices.size() < 2) return;

    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i + 1 < vertices.size()) {
            vectors.emplace_back(vertices[i+1] - vertices[i]);
        } else if (isClosed && vertices.size() > 2) {
            vectors.emplace_back(vertices[0] - vertices[i]);
        }
    }
}

void World::addBody(BodyConfig& body) {
    bodies.emplace_back(body);
}

/*
void Ball::update(std::vector<Body>& Bodies) {
    xPos += xSpeed;
    yPos += ySpeed;

    get_Collisions(Bodies);

    ySpeed = check_Y_Res(ySpeed);
    xSpeed = check_X_Res(xSpeed);

    shape.setPosition({xPos, yPos});
}

void Ball::draw(sf::RenderWindow& window) {
    window.draw(shape);
}
*/