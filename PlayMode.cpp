#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <cmath>
#include <math.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <thread>     
#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * { // vertex paint for the board didnt work :-(
	MeshBuffer const *ret = new MeshBuffer(data_path("board.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("board.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
	});
});

//Floating point equality from 
//http://www.cs.technion.ac.il/users/yechiel/c++-faq/floating-point-arith.html
inline bool isEqual(float x, float y) { 
  const float epsilon = 0.01f;
  return std::abs(x - y) <= epsilon * std::abs(x);
}

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:

	for (auto &drawable : scene.drawables) {
	 	if (drawable.transform->name == "Apple") ball = drawable.transform;
	 	else if (drawable.transform->name == "Cube") board = drawable.transform;
		else if (drawable.transform->name == "Snake1") snakehead = drawable.transform;
		else if (drawable.transform->name == "Snake2") {
			snakebod = drawable.transform;
			snakedraw = &drawable;
		}
	}

	SnakeMovement *sm = new SnakeMovement;
	sm->dir = 0;
	sm->t = snakebod;
	sm->delay = 0.0f;
	snake_vec.push_back(sm);

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	glm::mat4x3 frame = camera->transform->make_local_to_parent();
	glm::vec3 right = -frame[0];
	glm::vec3 forward = -frame[2];
	glm::vec2 move = glm::vec2(0.0f);
	camera->transform->position[0] = 0.276538f;
	camera->transform->position[1] = 4.126640f;
	camera->transform->position[2] = 64.429611f;

	camera->transform->rotation[0] = -0.031797f;
	camera->transform->rotation[1] = -0.002061f;
	camera->transform->rotation[2] = 0.002977f;
	camera->transform->rotation[3] = 0.999488f;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (die) return;
	{
		//combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed && !down.pressed && !up.pressed) {
			if (lr_rotate > -max_rotate) {
				glm::quat rot = rotate(scene.transforms.front().rotation, -0.01f ,glm::vec3(0,1,0));
				board->rotation = rot;
				lr_rotate -= 0.01f;
				//printf("%f\n", lr_rotate);
				ball_dir[0] -= 0.05f;
				if (snake_dir != 0 && can_change_dir()) {
					snake_dir = 1;
					push_movement(1);
				}
			}
		}
		if (!left.pressed && right.pressed && !down.pressed && !up.pressed) {
			if (lr_rotate < max_rotate) {
				glm::quat rot = rotate(scene.transforms.front().rotation, 0.01f ,glm::vec3(0,1,0));
				board->rotation = rot;
				lr_rotate += 0.01f;
				//printf("%f\n", lr_rotate);
				ball_dir[0] += 0.05f;
				if (snake_dir != 1 && can_change_dir())  {
					snake_dir = 0;
					push_movement(0);
				}
			}
		}
		if (!left.pressed && !right.pressed && down.pressed && !up.pressed) {
			if (ud_rotate < max_rotate) {
				glm::quat rot = rotate(scene.transforms.front().rotation, 0.01f ,glm::vec3(1,0,0));
				board->rotation = rot;
				ud_rotate += 0.01f;
				//printf("%f\n", ud_rotate);
				ball_dir[1] -= 0.05f;
				if (snake_dir != 2 && can_change_dir()) {
					snake_dir = 3;
					push_movement(3);
				}
			}
		}
		if (!left.pressed && !right.pressed && !down.pressed && up.pressed) {
			if (ud_rotate > -max_rotate) {
				glm::quat rot = rotate(scene.transforms.front().rotation, -0.01f ,glm::vec3(1,0,0));
				board->rotation = rot;
				ud_rotate -= 0.01f;
				ball_dir[1] += 0.05f;
				if (snake_dir != 3 && can_change_dir()) {
					snake_dir = 2;
					push_movement(2);
				}
			}
		}

		if (!down.pressed && !up.pressed) {
			if (ud_rotate != 0.0f) {
				glm::quat rot = rotate(scene.transforms.front().rotation, -ud_rotate, glm::vec3(1,0,0));
				board->rotation = rot;
				ud_rotate = 0.0f;
			}
		}

		if (!left.pressed && !right.pressed) {
			if (lr_rotate != 0.0f) {
				glm::quat rot = rotate(scene.transforms.front().rotation, -lr_rotate, glm::vec3(0,1,0));
				board->rotation = rot;
				lr_rotate = 0.0f;
			}
		}

		//make it so that moving diagonally doesn't go faster:

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];
		camera->transform->position += move.x * right + move.y * forward;

		//printf("%f, %f\n", (ball->position[0]), ball->position[1]);
		//printf("%f, %f, %f\n", camera->transform->position[0], camera->transform->position[1], camera->transform->position[2]);
		//printf("%f, %f, %f, %f\n", camera->transform->rotation[0], camera->transform->rotation[1], camera->transform->rotation[2], camera->transform->rotation[3]);
	}
	direction_timer += elapsed;
	ball->position += ball_dir * 0.3f;
	snake_ball_collide();
	board_ball_check();

	snake_head_move();

	snake_move(elapsed);

	if (snake_eat()) {
		std::random_device rd;
    	std::mt19937 e2(rd());
    	std::uniform_int_distribution<int> dist(-8, 8);
		ball->position[0] = (float)dist(e2);
		ball->position[1] = (float)dist(e2);
		ball_dir = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	snake_die();
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	//printf("end %zd %zd\n", scene.transforms.size(), scene.drawables.size());
}

bool PlayMode::can_change_dir() {
	if (direction_timer >= direction_delay) {
		direction_timer = 0.0f;
		return true;
	}
	return false;
}

bool PlayMode::snake_eat() {
	if (std::max(snakehead->position[0] - 0.75f, ball->position[0] - 0.75f) <= std::min(snakehead->position[0] + 0.75f, ball->position[0] + 0.75f) &&
		std::max(snakehead->position[1] - 0.75f, ball->position[1] - 0.75f) <= std::min(snakehead->position[1] + 0.75f, ball->position[1] + 0.75f)) {
		
		score++;
		Scene::Drawable new_body = Scene::Drawable(*snakedraw);
		Scene::Transform *t = new Scene::Transform;

		t->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		t->position = snake_vec.back()->t->position;
		auto i = scene.transforms.begin();
		//printf("pos name %f %f %f\n", scene.transforms.back().position[0], scene.transforms.back().position[1], scene.transforms.back().position[2]);
		// for (auto &tr : scene.transforms) {
		// 	printf("%s %f %f %f\n", tr.name.c_str(), tr.position[0], tr.position[1], tr.position[2]);
		// }
		
		t->scale = snakedraw->transform->scale;

		snake_size++;
		t->name = "Snake" + std::to_string(snake_size);
		t->parent = snakedraw->transform->parent;

		scene.transforms.emplace_back(*t);
		new_body.transform = t;
		scene.drawables.emplace_back(new_body);

		SnakeMovement *sm = new SnakeMovement;
		sm->dir = snake_vec.back()->dir;
		sm->t = t;
		sm->move_queue = snake_vec.back()->move_queue;
		snake_vec.push_back(sm);
		
		return true;
	}
	return false;
}

void PlayMode::snake_move(float elapsed) {
	for (size_t i = 0; i < snake_vec.size(); i++) {
		//printf("%f %f %f\n", snake_vec[i]->t->position[0], snake_vec[i]->t->position[1], snake_vec[i]->t->position[2]);
		//printf("%f %f %f\n", snakehead->position[0], snakehead->position[1], snakehead->position[2]);
		if (!snake_vec[i]->move_queue.empty()) {
			//printf("%f %f %f\n", snake_vec[i]->move_queue.front().position[0], snake_vec[i]->move_queue.front().position[1], snake_vec[i]->move_queue.front().position[2]);
			if (isEqual(snake_vec[i]->move_queue.front().position[0], snake_vec[i]->t->position[0]) &&
				isEqual(snake_vec[i]->move_queue.front().position[1], snake_vec[i]->t->position[1])) {
				snake_vec[i]->dir = snake_vec[i]->move_queue.front().dir;
				snake_vec[i]->move_queue.pop();
			}
		}
		snake_vec[i]->delay -= elapsed;
		uint8_t d = snake_vec[i]->dir;
		if (snake_vec[i]->delay <= 0.0f) {
			if (d == 0) {
				snake_vec[i]->t->position[0] += 0.1f;
			}
			else if (d == 1) {
				snake_vec[i]->t->position[0] -= 0.1f;
			}
			else if (d == 2) {
				snake_vec[i]->t->position[1] += 0.1f;
			}
			else if (d == 3) {
				snake_vec[i]->t->position[1] -= 0.1f;
			}
		}
	}
}

void PlayMode::snake_head_move() {
	if (snake_dir == 0) {
		snakehead->position[0] += 0.1f;
	}
	else if (snake_dir == 1) {
		snakehead->position[0] -= 0.1f;
	}
	else if (snake_dir == 2) {
		snakehead->position[1] += 0.1f;
	}
	else if (snake_dir == 3) {
		snakehead->position[1] -= 0.1f;
	}
}

void PlayMode::snake_ball_collide() {
	for (size_t i = 0; i < snake_vec.size(); i++) {
		if (std::max(snake_vec[i]->t->position[0] - 0.75f, ball->position[0] - 0.75f) <= std::min(snake_vec[i]->t->position[0] + 0.75f, ball->position[0] + 0.75f) &&
			std::max(snake_vec[i]->t->position[1] - 0.75f, ball->position[1] - 0.75f) <= std::min(snake_vec[i]->t->position[1] + 0.75f, ball->position[1] + 0.75f)) {
			ball_dir[1] += (ball_dir[1] < 0 ? -0.03f : 0.03f);
			ball_dir[0] += (ball_dir[0] < 0 ? -0.03f : 0.03f);
			ball_dir = -ball_dir;
		}
	}
}

void PlayMode::push_movement(uint8_t dir) {
	for (size_t i = 0; i < snake_vec.size(); i++) {
		MoveStruct m = {snakehead->position, dir};
		snake_vec[i]->move_queue.push(m);
	}
}

void PlayMode::board_ball_check() {
	if (ball->position[0] + 0.75f > 9.0f) {
		ball->position[0] = 9.0f - 0.75f;
		ball_dir[0] = 0.0f;
	}
	if (ball->position[0] - 0.75f < -9.0f) {
		ball->position[0] = -9.0f + 0.75f;
		ball_dir[0] = 0.0f;
	}
	if (ball->position[1] + 0.75f > 9.0f) {
		ball->position[1] = 9.0f - 0.75f;
		ball_dir[1] = 0.0f;
	}
	if (ball->position[1] - 0.75f < -9.0f) {
		ball->position[1] = -9.0f + 0.75f;
		ball_dir[1] = 0.0f;
	}
}

void PlayMode::snake_die() {
	for (size_t i = 1; i < snake_vec.size(); i++) {
		glm::vec3 pos = snake_vec[i]->t->position;
		if (snake_dir == 0) {
			if (std::max(snakehead->position[0], pos[0] - 0.75f) <= std::min(snakehead->position[0] + 0.75f, pos[0] + 0.75f) &&
				std::max(snakehead->position[1] - 0.75f, pos[1] - 0.75f) <= std::min(snakehead->position[1] + 0.75f, pos[1] + 0.75f)) {
				die = true;
			}
		} else if (snake_dir == 1) {
			if (std::max(snakehead->position[0] - 0.75f, pos[0] - 0.75f) <= std::min(snakehead->position[0], pos[0] + 0.75f) &&
				std::max(snakehead->position[1] - 0.75f, pos[1] - 0.75f) <= std::min(snakehead->position[1] + 0.75f, pos[1] + 0.75f)) {
				die = true;
			}
		} else if (snake_dir == 2) {
			if (std::max(snakehead->position[0] - 0.75f, pos[0] - 0.75f) <= std::min(snakehead->position[0] + 0.75f, pos[0] + 0.75f) &&
				std::max(snakehead->position[1], pos[1] - 0.75f) <= std::min(snakehead->position[1] + 0.75f, pos[1] + 0.75f)) {
				die = true;
			}
		} else if (snake_dir == 3) {
			if (std::max(snakehead->position[0] - 0.75f, pos[0] - 0.75f) <= std::min(snakehead->position[0] + 0.75f, pos[0] + 0.75f) &&
				std::max(snakehead->position[1] - 0.75f, pos[1] - 0.75f) <= std::min(snakehead->position[1], pos[1] + 0.75f)) {
				die = true;
			}
		}
	}
	if (snake_dir == 0) {
		if (snakehead->position[0] + 0.75f >= 9.0f) {
			die = true;
		}
	} else if (snake_dir == 1) {
		if (snakehead->position[0] - 0.75f <= -9.0f) {
			die = true;
		}
	} else if (snake_dir == 2) {
		if (snakehead->position[1] + 0.75f >= 9.0f) {
			die = true;
		}
	} else if (snake_dir == 3) {
		if (snakehead->position[1] - 0.75f <= -9.0f) {
			die = true;
		}
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	GL_ERRORS();
	glUseProgram(0);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.
	scene.draw(*camera);
	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		if (die) {
			std::string txt = "Game Over! Score: " + std::to_string(score);
			lines.draw_text(txt,
				glm::vec3(-aspect / 5.0f, 0.0f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}
	}
}
