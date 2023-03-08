#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <deque>
#include <nlohmann/json.hpp>
// #define DEBUG_MODE

using json = nlohmann::json;
std::ifstream f("include/piezas.json");
json data = json::parse(f);

constexpr size_t GAME_SPEED = 150;

enum Space
{
	empty,
	block
};
enum shape
{
	square,
	letter_l,
	bar,
	letter_t,
	skew
};

using position = std::pair<int, int>;
using figure = std::vector<std::vector<Space>>;

figure get_shape(shape s)
{
	switch (s)
	{
	case square:
		return data["square"];

	case letter_l:
		return data["letter_l"];

	case bar:
		return data["bar"];

	case letter_t:
		return data["letter_t"];

	case skew:
		return data["skew"];

	default:
		throw "Figura no existe";
	}
}

enum rotate_dir
{
	left,
	right
};

class Board
{
private:
	class Piece;
	std::deque<std::deque<Space>> _state{};
	std::deque<std::deque<Space>> _filled_state{};
	std::pair<int, int> _resolution;
	Piece *_current_piece;

	void _print_board()
	{
		std::system("clear");
		for (size_t y = 0; y < _state.size(); ++y)
		{
			for (size_t x = 0; x < _state[0].size(); ++x)
			{
				if (_state[y][x] == block || _filled_state[y][x] == block)
					std::cout << "X ";
				else
					std::cout << "- ";
			}
			std::cout << "\n";
		}
		return;
	}

	void _render_piece(Piece *to_render)
	{
		auto y_loc = to_render->_position.second;
		auto x_loc = to_render->_position.first;

		size_t y_limit = y_loc < 0 ? to_render->_shape.size() + size_t(y_loc)
								   : to_render->_shape.size();

		for (size_t i = 0; i < y_limit; ++i)
		{
			for (size_t j = 0; j < to_render->_shape[0].size(); ++j)
			{
				if (y_loc <= (int(to_render->_shape.size()) * -1))
					return;

				if (y_loc >= 0 && to_render->_shape[i][j] == block)
					_state[size_t(y_loc) + i][size_t(x_loc) + j] = to_render->_shape[i][j];
				else if (to_render->_shape[i][j] == empty)
					continue;
				else if (to_render->_shape[i + size_t(abs(y_loc) - 1)][j] == block)
					_state[i][size_t(x_loc) + j] = to_render->_shape[i + size_t(abs(y_loc))][j];
			}
		}
	}

public:
	Board(size_t x, size_t y) : _resolution{x, y}
	{
		_state.resize(y);
		for (auto &row : _state)
			row.resize(x);

		_filled_state.resize(y);
		for (auto &row : _filled_state)
			row.resize(x);

		_current_piece = new Piece(this);
	};
	~Board(){};

	void _row_check()
	{
		// _state | views::filter([](std::deque<Space> &row){ return std::all_of(row.begin(), row.end(), [](auto s)
		//					{ return s == block; }) })
		//
		for (auto row = _state.begin(); row != _state.end(); ++row)
		{
			if (std::all_of(row->begin(), row->end(), [](auto s)
							{ return s == block; }))
			{
				/*
				- Eliminar bloques de fila y bajar todo un nivel
				- O eliminar fila y agregar vacía al fondo???¿¿¿
				*/
				_state.erase(row);
				_state.push_front(std::deque<Space>{});
				_state.front().resize(_state.back().size());
			}
		}
	}

	void game_loop()
	{
		// bool lost = false;
		while (true)
		{
			_row_check();

			auto play_state = _current_piece->play(this);

			if (play_state == blocked)
			{
				if (_current_piece->_position.second < 1)
					break;

				_filled_state = _state;
				_current_piece = new Piece(this);
			}

			_state = _filled_state;
			_render_piece(_current_piece);
			_print_board();
			std::this_thread::sleep_for(std::chrono::milliseconds(GAME_SPEED));
		}

		std::system("clear");
		std::cout << "GAME OVER\n";
	}

	enum piece_state
	{
		free,
		blocked
	};

private:
	class Piece
	{
	public:
		figure _shape{};
		position _position{};

		Piece(Board *board)
		{
#ifdef DEBUG_MODE
			_shape = get_shape(bar);
			_position = position{
				size_t(rand()) % (board->_state[0].size() - _shape[0].size()),
				(int(_shape.size()) * -1) + 2};
#else
			// std::mt19937 mt{};
			_shape = get_shape(shape(rand() % 5));
			_position = position{
				size_t(rand()) % (board->_state[0].size() - _shape[0].size()),
				(int(_shape.size()) * -1) + 2};
#endif
		}
		Piece(shape s, position start) : _shape{get_shape(s)},
										 _position{start} {};
		~Piece();

		void rotate(rotate_dir d);

		void move(int x_add, int y_add)
		{
			_position.first += x_add;
			_position.second += y_add;
		};

		piece_state play(Board *board)
		{
			if (size_t(_position.second) + _shape.size() >= board->_state.size())
				return blocked;

			for (size_t i = 0; i < _shape.back().size(); ++i)
			{
				if (_shape.back()[i] == block)
				{
					if (board->_state[size_t(_position.second) + _shape.size()]
									 [size_t(_position.first) + i] == block)
						return blocked;
				}
				else if (_shape[_shape.size() - 2][i] == block)
				{
					if (board->_state[size_t(_position.second) + _shape.size() - 1] //- 1]
									 [size_t(_position.first) + i] == block)
						return blocked;
				}
			}

			move(0, 1);
			return free;
		}
	};
};

int main()
{
	srand(time(NULL)); // seed
	auto main_board = Board(10, 20);
	main_board.game_loop();
}