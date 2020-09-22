#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <queue>
#include <vector>
#include <deque>
#include <random>

struct MoveStruct {
	glm::vec3 position;
	uint8_t dir;
};

struct SnakeMovement {
	uint8_t dir;
	float delay = 0.27f;
	Scene::Transform *t;
	std::queue<MoveStruct> move_queue;
};

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	virtual void board_ball_check();
	virtual bool snake_eat();
	virtual void snake_move(float elapsed);
	virtual void snake_head_move();
	virtual void snake_ball_collide();
	virtual void push_movement(uint8_t dir);
	virtual void snake_die();
	virtual bool can_change_dir();

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	float max_rotate = 0.23f;
	float lr_rotate = 0.0f;
	float ud_rotate = 0.0f;


	glm::vec3 bord1 = glm::vec3(9.0524f, -9.0476f, 0.5f);
	glm::vec3 bord2 = glm::vec3(-9.0f, 9.0f, 0.5f);
	glm::vec3 bord3 = glm::vec3(9.0989f, 9.0482f, 0.5f);
	glm::vec3 bord4 = glm::vec3(-9.0f, -9.0f, 0.5f);
	glm::vec3 ball_dir = glm::vec3(0.0f, 0.0f, 0.0f);

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *board = nullptr;
	Scene::Transform *ball = nullptr;
	Scene::Transform *snakehead = nullptr;
	Scene::Transform *snakebod = nullptr;
	Scene::Drawable *snakedraw = nullptr;
	uint8_t snake_size = 2;

	int score = 0;
	bool die = false;
	float direction_delay = 0.265f;
	float direction_timer = 0.265f;
	
	std::vector<SnakeMovement *> snake_vec;
	uint8_t snake_dir = 0; //right

	//camera:
	Scene::Camera *camera = nullptr;

};
