#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
// #include <gsl/gsl>
#include <boost/multi_array.hpp>
#include <nlohmann/json.hpp>

struct Coordinate
{
	int x, y;
};

constexpr size_t FRAMERATE = 3;
constexpr size_t FRAME_TIME = 1000 / FRAMERATE;
constexpr Coordinate RESOLUTION{40, 30};
constexpr size_t GAME_SPEED = 150;

inline constexpr std::size_t operator"" _z(unsigned long long int n)
{
	return n;
}

template <class T, class F>
void for_each(T list, F func)
{
	std::for_each(list.begin(), list.end(), func);
};

enum Space
{
	empty,
	block,
	text,
	number
};

std::ostream &operator<<(std::ostream &out, const Space &space)
{
	switch (space)
	{
	case empty:
		out << "  ";
		break;

	case block:
		out << "□ ";
		break;

	default:
		break;
	}

	return out;
};

// using Region = std::vector<std::vector<Space>>;
using Region = boost::multi_array<Space, 2>;
using View1D = Region::array_view<1>::type;
using View2D = Region::array_view<2>::type;
using Range = boost::multi_array_types::index_range;

template <class T>
Coordinate operator+(const Coordinate &lhs, const std::vector<T> &rhs)
{
	return Coordinate{lhs.x + rhs[0], lhs.y + rhs[1]};
}

enum BlendType
{
	opaque,
	transparent
};

Region dibujar_cuadrado(int x, int y)
{
	Region output{boost::extents[y][x]};
	for (auto i = 0; i < y; ++i)
	{
		output[i][0] = block;
		output[i][x - 1] = block;
	}

	for (auto &col : *output.begin())
		col = block;

	for (auto &col : *(output.end() - 1))
		col = block;

	return output;
}

class ScreenObject
{
protected:
	friend class Scene;
	Coordinate placement = Coordinate{0, 0};
	BlendType opacity = opaque;
	Region content;

public:
	size_t layer{0_z}; // tengo que hacer publico para usar operator<() ??
	ScreenObject(const Coordinate size, const Coordinate initial)
		: placement{initial},
		  content{boost::extents[size.y][size.x]} {};

	virtual bool update()
	{
		return true;
	};

	inline Region get_content() const
	{
		return content;
	}

	inline size_t height() const
	{
		return this->content.size();
	}

	inline size_t width() const
	{
		return this->content[0].size();
	}

	// TODO move_by y move_to
	bool move(const int x, const int y)
	{
		if (placement.x + x <= RESOLUTION.x &&
			placement.x + x > int(width()) * -1 &&
			placement.y + y <= RESOLUTION.y &&
			placement.y + y > int(height()) * -1)
		{
			placement.x += x;
			placement.y += y;
			return true;
		}
		return false; // out-of-bounds
	}

	bool operator<(const ScreenObject &rhs) const
	{
		return this->layer < rhs.layer;
	}
	bool operator>(const ScreenObject &rhs) const
	{
		return this->layer > rhs.layer;
	}
};

class Collidable : public ScreenObject
{
	Region collition_area{};

public:
	Collidable(const Coordinate size, const Coordinate initial)
		: ScreenObject(size, initial)
	{
	}
};

class Piece : public Collidable
{
public:
	// void rotate(direction d);
};

class Board : public ScreenObject
{
private:
	Piece *current_piece{nullptr};

public:
	Board(const Coordinate size, const Coordinate initial)
		: ScreenObject(size, initial)
	{
		layer = 1_z;
		content = dibujar_cuadrado(size.x, size.y);
	}

	bool update()
	{
		move(0, 1);
		return true;
	};
};

// Usado en addElement()
bool operator<(const std::unique_ptr<ScreenObject> lhs,
			   const std::unique_ptr<ScreenObject> rhs)
{
	return lhs->layer < rhs->layer;
};
bool operator>(const std::unique_ptr<ScreenObject> lhs,
			   const std::unique_ptr<ScreenObject> rhs)
{
	return lhs->layer > rhs->layer;
};

class Scene
{
	Region current_screen{};
	std::vector<ScreenObject *> elements{};

public:
	[[nodiscard]] Scene() : current_screen{boost::extents[RESOLUTION.y][RESOLUTION.x]} {}
	[[nodiscard]] Scene(const Coordinate size) : current_screen{boost::extents[size.y][size.x]} {}

	void addElement(ScreenObject &new_element)
	{
		elements.push_back(&new_element);
		// std::sort(elements.begin(), elements.end(),
		// 		  std::greater<ScreenObject *>());
		return;
	}

	bool update_elements()
	{
		auto result = true;
		for (auto &e : elements)
			result &= e->update();
		return result;
	};

	void render_element(ScreenObject &elem)
	{
		auto x_pos = elem.placement.x;
		auto y_pos = elem.placement.y;

		// // El constructor de ScreenObject y el setter de Coordinate garantizan que
		// // (position + width,height) >= 0, por lo que (x_pos + x, y_pos + y) nunca van a ser < 0
		// #pragma GCC diagnostic push
		// #pragma GCC diagnostic ignored "-Wsign-conversion"

		int render_width_begin = x_pos < 0 ? x_pos : 0;
		int render_height_begin = y_pos < 0 ? y_pos : 0;

		int render_width_end = (int(elem.width()) + x_pos) > RESOLUTION.x
								   ? RESOLUTION.x - x_pos
								   : int(elem.width());

		int render_height_end = int(elem.height()) + y_pos > RESOLUTION.y
									? RESOLUTION.y - y_pos
									: int(elem.height());

		// TODO no anda??
		// View2D to_render = current_screen[
		// Range{y_pos + render_height_begin, y_pos + render_height_end}]
		// [Range{x_pos + render_width_begin, x_pos + render_width_end}
		// ];

		for (int y = render_height_begin; y < render_height_end; ++y)
			for (int x = render_width_begin; x < render_width_end; ++x)
			{
				// [[assume(x_pos + x >= 0)]] // C++23
				// [[assume(y_pos + y >= 0)]]
				auto current_space = elem.get_content()[y][x];
				if (elem.opacity == opaque || current_space != empty)
					current_screen[y_pos + int(y)]
								  [x_pos + int(x)] =
									  current_space;
			}
	};
	// #pragma GCC diagnostic pop

	struct clear_block
	{
		void operator()(Space &i) { i = empty; };
	};

	void render_frame()
	{
		std::for_each(current_screen.data(), current_screen.data() + current_screen.num_elements(), clear_block());

		for_each(elements, [this](auto e)
				 { render_element(*e); });
		// for (const auto e : elements)
		// 	render_element(*e);
	}

	void print_frame()
	{
		int err = std::system("clear");
		if (err)
			throw "No terminal available";

		for (const auto &row : current_screen)
		{
			for (const auto &space : row)
				std::cout << space;
			std::cout << "\n";
		}
	}
};

class App
{
protected:
	std::map<const std::string, std::unique_ptr<Scene>> Level;

	void LoadScene(std::string scene_name)
	{
		auto &scene = Level.at(scene_name);
		bool status = scene->update_elements();

		while (status)
		{
			scene->render_frame();
			scene->print_frame();
			status = scene->update_elements();
			std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_TIME));
		}
		LoadScene("game");
	};

	void new_level(std::string name)
	{
		Level[name] = std::make_unique<Scene>(Scene());
		return;
	}

public:
	App(){};
};

// TODO Diagonal line problem

// #include "TermEngine.hpp"

using json = nlohmann::json;

// using namespace TermEngine;

enum TetrisBlock
{
	square,
	letter_l,
	bar,
	letter_t,
	skew
};

enum direction
{
	down,
	left,
	right,
	up
};

class Tetris : public App
{
	const size_t player_count = 1;
	size_t score = 0;

public:
	//[[noreturn]]
	Tetris()
	{
		new_level("menu");
		new_level("game");

		// tendría que pertenecerle al nivel, no a Tetris()
		// TODO pasar a subclase Level con unique_ptr y chequear override
		auto board = new Board(Coordinate{20, 15}, Coordinate{5, 5});
		auto board2 = new Board(Coordinate{20, 15}, Coordinate{15, 15});

		// TODO hacer subclase
		Level["game"]->addElement(*board);
		Level["game"]->addElement(*board2);
		LoadScene("game");
	};
};

int main()
{
	Tetris();
}