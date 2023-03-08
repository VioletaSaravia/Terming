#include <nlohmann/json.hpp>
#include "TermEngine.cpp"

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
		std::cout << "fuck me\n";
		move(0, 1);
		return true;
	};
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
		auto board = std::make_unique<ScreenObject>(Board(Coordinate{20, 15}, Coordinate{5, 5}));
		auto board2 = std::make_unique<ScreenObject>(Board(Coordinate{20, 15}, Coordinate{15, 15}));
		Level["game"]->addElement(*board);
		Level["game"]->addElement(*board2);
		LoadScene("game");
	};
};

int main()
{
	Tetris();
}