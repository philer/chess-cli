#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <regex>
#include <string>
#include <vector>

using namespace std;

/**
 * Play interactive chess on the command line.
 *
 * This program was created for training purposes and is full of experiments –
 * read it at your own risk.
 *
 * Future ideas:
 * + highlight last move
 * + highlight check
 * + highlight available moves for a selected piece
 * */

enum Color : bool {
  white = true,
  black = false,
};

// Overloading operator! to return Color instead of bool leads to segfault
Color invert(const Color color) {
  return static_cast<Color>(!color);
}

enum Piece : uint8_t {
  PAWN,
  KNIGHT,
  BISHOP,
  ROOK,
  QUEEN,
  KING,
};
constexpr array<Piece, 6> PIECE_TYPES = {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

struct ColorPiece {
  Color color;
  Piece piece;

  bool operator==(const ColorPiece &other) const {
    return color == other.color && piece == other.piece;
  }
};

ColorPiece invert(ColorPiece piece) {
  piece.color = invert(piece.color);
  return piece;
}

string to_string(const ColorPiece &piece) {
  static const array<array<string, 2>, 6> UTF8_PIECES = {
      {
          {"♙", "♟︎"},
          {"♘", "♞"},
          {"♗", "♝"},
          {"♖", "♜"},
          {"♕", "♛"},
          {"♔", "♚"},
      },
  };
  return UTF8_PIECES[piece.piece][piece.color];
}

constexpr ColorPiece WHITE_PAWN = {white, PAWN};
constexpr ColorPiece WHITE_KNIGHT = {white, KNIGHT};
constexpr ColorPiece WHITE_BISHOP = {white, BISHOP};
constexpr ColorPiece WHITE_ROOK = {white, ROOK};
constexpr ColorPiece WHITE_QUEEN = {white, QUEEN};
constexpr ColorPiece WHITE_KING = {white, KING};
constexpr ColorPiece BLACK_PAWN = {black, PAWN};
constexpr ColorPiece BLACK_KNIGHT = {black, KNIGHT};
constexpr ColorPiece BLACK_BISHOP = {black, BISHOP};
constexpr ColorPiece BLACK_ROOK = {black, ROOK};
constexpr ColorPiece BLACK_QUEEN = {black, QUEEN};
constexpr ColorPiece BLACK_KING = {black, KING};

typedef array<array<optional<ColorPiece>, 8>, 8> Board;

// clang-format off
constexpr Board STARTING_BOARD = {{
  {WHITE_ROOK,   WHITE_PAWN, {}, {}, {}, {}, BLACK_PAWN, BLACK_ROOK},
  {WHITE_KNIGHT, WHITE_PAWN, {}, {}, {}, {}, BLACK_PAWN, BLACK_KNIGHT},
  {WHITE_BISHOP, WHITE_PAWN, {}, {}, {}, {}, BLACK_PAWN, BLACK_BISHOP},
  {WHITE_QUEEN,  WHITE_PAWN, {}, {}, {}, {}, BLACK_PAWN, BLACK_QUEEN},
  {WHITE_KING,   WHITE_PAWN, {}, {}, {}, {}, BLACK_PAWN, BLACK_KING},
  {WHITE_BISHOP, WHITE_PAWN, {}, {}, {}, {}, BLACK_PAWN, BLACK_BISHOP},
  {WHITE_KNIGHT, WHITE_PAWN, {}, {}, {}, {}, BLACK_PAWN, BLACK_KNIGHT},
  {WHITE_ROOK,   WHITE_PAWN, {}, {}, {}, {}, BLACK_PAWN, BLACK_ROOK},
}};
// clang-format on

struct Square {
  uint8_t file;
  uint8_t rank;

  bool operator==(const Square &other) const {
    return file == other.file && rank == other.rank;
  }

  bool exists() const {
    return file <= 7 && rank <= 7;  // always >= 0 due to unsinged
  }
};

string to_string(const Square &square) {
  return {
      static_cast<char>((square.file + 'a')),
      static_cast<char>((square.rank + '1'))};
}

Square get_square(const char file, const char rank) {
  return Square{
      static_cast<uint8_t>((file - 'a')), static_cast<uint8_t>((rank - '1'))};
}

Square get_square(const string &square) {
  return get_square(square[0], square[1]);
}

struct Move {
  string algebraic;
  ColorPiece piece;
  Square from;
  Square to;
  optional<ColorPiece> capture;
  optional<ColorPiece> promotion;
  // bool en_passant;  // TODO
  bool check;
};

struct Game {
  Board board = STARTING_BOARD;
  vector<Move> history = {};
  Color turn = white;

  struct CanCastle {
    bool king_side;
    bool queen_side;
  };
  array<CanCastle, 2> can_castle = {{{true, true}, {true, true}}};
};

ColorPiece get_piece(const char piece_character, const Color color) {
  switch (piece_character) {
    case 'K':
      return {color, KING};
    case 'Q':
      return {color, QUEEN};
    case 'R':
      return {color, ROOK};
    case 'B':
      return {color, BISHOP};
    case 'N':
      return {color, KNIGHT};
    case 'P':
      return {color, PAWN};
    default:
      string piece_str;
      piece_str = piece_character;
      throw string("Invalid piece '" + piece_str + "'");
  }
}

optional<ColorPiece> get_piece(const Board &board, const Square &square) {
  return board[square.file][square.rank];
}

vector<Square> find_pieces(const Board &board, const ColorPiece &piece) {
  vector<Square> squares;
  for (uint8_t file = 0; file < 8; ++file) {
    for (uint8_t rank = 0; rank < 8; ++rank) {
      if (board[file][rank] == piece) {
        squares.push_back({file, rank});
      }
    }
  }
  return squares;
}

template <size_t N>
vector<Square> find_line_attacking_pieces(
    const Board &board,
    const Square &target_square,
    const ColorPiece &piece,
    const array<pair<int8_t, int8_t>, N> &directions,
    optional<uint8_t> file = nullopt,
    optional<uint8_t> rank = nullopt
) {
  vector<Square> found;
  for (const auto &[d_file, d_rank] : directions) {
    for (uint8_t offset = 1;; ++offset) {
      Square square = {
          static_cast<uint8_t>(target_square.file + offset * d_file),
          static_cast<uint8_t>(target_square.rank + offset * d_rank)};
      if (!square.exists()) {
        break;
      }
      const optional<ColorPiece> &found_piece = get_piece(board, square);
      if (found_piece && found_piece == piece && (!file || square.file == file)
          && (!rank || square.rank == rank)) {
        found.push_back(square);
      }
      if (found_piece) {
        break;
      }
    }
  }
  return found;
}

template <size_t N>
vector<Square> find_direct_attacking_pieces(
    const Board &board,
    const Square &target_square,
    const ColorPiece &piece,
    const array<pair<int8_t, int8_t>, N> &moves,
    optional<uint8_t> file = nullopt,
    optional<uint8_t> rank = nullopt
) {
  vector<Square> found;
  for (const auto &[d_file, d_rank] : moves) {
    Square square = {
        static_cast<uint8_t>(target_square.file + d_file),
        static_cast<uint8_t>(target_square.rank + d_rank)};
    if (!square.exists()) {
      continue;
    }
    if (file && square.file != *file || rank && square.rank != *rank) {
      continue;
    }
    if (get_piece(board, square) == piece) {
      found.push_back(square);
    }
  }
  return found;
}

vector<Square> find_attacking_pieces(
    const Board &board,
    const Square &target_square,
    const ColorPiece &piece,
    optional<uint8_t> file = nullopt,
    optional<uint8_t> rank = nullopt
) {
  switch (piece.piece) {
      // clang-format off
    case PAWN:
      return find_direct_attacking_pieces<2>(
          board, target_square, piece,
          {{{piece.color ? -1 : +1, -1}, {piece.color ? -1 : +1, +1}}},
          file, rank
      );
    case KNIGHT:
      return find_direct_attacking_pieces<8>(
          board, target_square, piece,
          {{{+1, +2}, {+1, -2}, {-1, +2}, {-1, -2},
            {+2, +1}, {+2, -1}, {-2, +1}, {-2, -1}}},
          file, rank
      );
    case BISHOP:
      return find_line_attacking_pieces<4>(
          board, target_square, piece,
          {{{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1}}},
          file, rank
      );
    case ROOK:
      return find_line_attacking_pieces<4>(
          board, target_square, piece,
          {{{0, -1}, {0, +1}, {-1, 0}, {+1, 0}}},
          file, rank
      );
    case QUEEN:
      return find_line_attacking_pieces<8>(
          board, target_square, piece,
          {{{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1},
            { 0, -1}, { 0, +1}, {-1,  0}, {+1,  0}}},
          file, rank
      );
    case KING:
      return find_direct_attacking_pieces<8>(
          board, target_square, piece,
          {{{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1},
            { 0, -1}, { 0, +1}, {-1,  0}, {+1,  0}}},
          file, rank
      );
    // clang-format on
    default:
      throw string("Unknown piece code " + to_string(piece.piece));
  }
}

bool is_attacked(
    const Board &board, const Square &square, const Color by_color
) {
  for (const Piece &piece : PIECE_TYPES) {
    if (!find_attacking_pieces(board, square, {by_color, piece}).empty()) {
      return true;
    }
  }
  return false;
}

bool is_in_check(const Game &game, Color color) {
  const vector<Square> &king_squares = find_pieces(game.board, {color, KING});
  if (king_squares.size() != 1) {
    throw string("You need exactly one King.");
  }
  return is_attacked(game.board, king_squares[0], invert(color));
}


// TODO shortened pawn captures ("exd", "ed")
const regex PAWN_MOVE_PATTERN{"^([a-h][1-8])(:?=?([NBRQ]))?(?:\b|$)"};
const regex PAWN_CAPTURE_PATTERN{
    "^([a-h])x([a-h][1-8])(:?=?([NBRQ]))?(?:\b|$)"};
const regex PIECE_MOVE_OR_CAPTURE_PATTERN{
    "^([NBRQK])([a-h])?([1-8])?(x)?([a-h][1-8])(?:\b|$)"};
const regex CASTLING_PATTERN{"^[O0]-?[O0](-?[O0])?(?:\b|$)", regex::icase};

optional<ColorPiece> get_promotion(
    const Move &move, const Color color, const ssub_match &promo_match
) {
  optional<ColorPiece> promotion = nullopt;
  if (promo_match.matched) {
    promotion = get_piece(string(promo_match)[0], color);
  }
  const bool must_promote = color == white && move.to.rank == 7
                            || color == black && move.to.rank == 0;
  if (must_promote && !promotion) {
    throw string("The pawn reaches the final rank and must be promoted.");
  } else if (!must_promote && promotion) {
    throw string("Can only promote on the final rank.");
  }
  return promotion;
}

/**
 * Extract all relevant details from a move given in algebraic notation on a
 * specific board and check if it is legal to apply.
 *
 * TODO en passant legal
 * TODO check castling rights
 * TODO check for checks & mate
 */
Move decode_move(const Game &game, const string &move) {
  const auto &[board, history, turn, can_castle] = game;
  Move mv;
  mv.algebraic = move;
  mv.check = false;
  const int8_t forwards = turn ? 1 : -1;
  smatch match;

  if (regex_match(move, match, PAWN_MOVE_PATTERN)) {  // "e4"
    mv.piece = get_piece('P', turn);
    mv.to = get_square(match[1]);
    mv.from.file = mv.to.file;

    if (board[mv.from.file][mv.to.rank - forwards] == mv.piece) {
      mv.from.rank = mv.to.rank - forwards;

    } else if ((turn && mv.to.rank == 3 || !turn && mv.to.rank == 4) &&
               !board[mv.from.file][mv.to.rank - forwards] &&
               board[mv.from.file][mv.to.rank - 2 * forwards] == mv.piece) {
      // Move two spaces from starting rank
      mv.from.rank = mv.to.rank - 2 * forwards;
    } else {
      throw string(
          "There is no eligible Pawn on "
          + to_string(Square{
              mv.from.file, static_cast<uint8_t>(mv.to.rank - forwards)})
          + " or "
          + to_string(Square{
              mv.from.file, static_cast<uint8_t>(mv.to.rank - 2 * forwards)})
          + "."
      );
    }

    // Prevent illegal capture
    if (get_piece(board, mv.to)) {
      throw string(
          to_string(mv.to) + " is blocked. Pawns can only capture diagonally."
      );
    }

    mv.promotion = get_promotion(mv, turn, match[2]);

  } else if (regex_match(move, match, PAWN_CAPTURE_PATTERN)) {  // "dxe4"
    mv.piece = {turn, PAWN};
    mv.to = get_square(match[2]);
    mv.from = get_square(move[0], move[3] - forwards);
    if (abs(mv.from.file - mv.to.file) != 1) {
      throw string("Pawn must move one square diagonally when capturing.");
    }
    if (board[mv.from.file][mv.from.rank] != mv.piece) {
      throw string("No eligible Pawn on " + to_string(mv.from) + ".");
    }
    mv.capture = get_piece(board, mv.to);
    if (!mv.capture) {
      // Check for en passant capture
      // TODO check if opponent's pawn just moved by two ranks
      const optional<ColorPiece> en_passant_capture =
          get_piece(board, get_square(move[2], move[3] - forwards));
      if (en_passant_capture == invert(mv.piece)) {
        const Move &previous = history.back();
        if (previous.piece == invert(mv.piece)
            && previous.from == get_square(move[2], move[3] + forwards)) {
          mv.capture = en_passant_capture;
        } else {
          throw string("Can't capture en passant, the opposing pawn was moved "
                       "too long ago.");
        }
      } else {
        throw string(
            "There is nothing to capture on " + to_string(mv.to) + "."
        );
      }
    }
    if (turn == mv.capture->color) {
      throw string("Can't capture your own piece.");
    }

    mv.promotion = get_promotion(mv, turn, match[3]);

  } else if (regex_match(move, match, PIECE_MOVE_OR_CAPTURE_PATTERN)) {
    // "Qe4, Qxe4, Qde4, Qdxe4, Q3e4, Q3xe4, Qd3e4, Qd3xe4"
    mv.piece = get_piece(move[0], turn);
    mv.to = get_square(match[5]);

    // Decode optional starting square qualifiers
    optional<uint8_t> from_file = nullopt;
    optional<uint8_t> from_rank = nullopt;
    if (match[2].matched) {
      from_file = string(match[2])[0] - 'a';
    }
    if (match[3].matched) {
      from_rank = string(match[3])[0] - '1';
    }

    // Search for matching pieces
    const vector<Square> candidates =
        find_attacking_pieces(board, mv.to, mv.piece, from_file, from_rank);
    switch (candidates.size()) {
      case 1:
        mv.from = candidates[0];
        break;
      case 0:
        throw string("No candidate pieces available.");
      default:
        throw string("Ambiguous move: multiple pieces available.");
    }

    // Check for captures
    mv.capture = get_piece(board, mv.to);
    if (match[3].matched) {
      if (!mv.capture) {
        throw string(
            "There is nothing to capture on " + to_string(mv.to) + "."
        );
      } else if (turn == mv.capture->color) {
        throw string("Can't capture your own piece.");
      }
    } else {
      if (mv.capture) {
        throw string("Target square is occupied")
            + (turn == mv.capture->color ? " by your own piece."
                                         : ", add 'x' to capture.");
      }
    }

  } else if (regex_match(move, match, CASTLING_PATTERN)) {
    // TODO check king passing squares under check
    const bool castle_long = match[1].matched;
    if (castle_long ? !can_castle[turn].king_side
                    : !can_castle[turn].queen_side) {
      throw string("You can no longer castle on this side, the King or Rook "
                   "has already moved.");
    }
    const uint8_t rank = white ? 0 : 7;
    if (board[4][rank] != ColorPiece{turn, KING}
        || (castle_long
                ? board[0][rank] != ColorPiece{turn, ROOK} || board[1][rank]
                      || board[2][rank] && !board[3][rank]
                : board[7][rank] != ColorPiece{turn, ROOK} || board[6][rank]
                      || board[5][rank])) {
      throw string("You can't castle on this side of the board right now.");
    }
    mv.piece = ColorPiece{turn, KING};
    mv.from = Square{4, rank};
    mv.to = Square{static_cast<uint8_t>(castle_long ? 2 : 6), rank};

  } else {
    throw string(
        "'" + move
        + "' is not a known move format.\n"
          "Note: Do not add any special characters like + # = "
          "to indicate check/mate/promotion."
    );
  }
  return mv;
}

/**
 * Execute a decoded move on the given board. This function assumes that all
 * checks have passed and that it can be applied to the given board to create a
 * valid game state.
 */
void apply_move(Game &game, const Move &move) {
  auto &[board, history, turn, can_castle] = game;
  const ColorPiece &piece = *board[move.from.file][move.from.rank];

  // capture en passant
  const optional<ColorPiece> &capture = board[move.to.file][move.to.rank];
  if (!capture && move.capture == ColorPiece(invert(piece.color), PAWN)) {
    board[move.to.file][move.from.rank] = nullopt;
  }

  // move
  board[move.from.file][move.from.rank] = nullopt;
  board[move.to.file][move.to.rank] = move.promotion.value_or(piece);

  // castling
  if ((piece == WHITE_KING || piece == BLACK_KING)
      && abs(move.from.file - move.to.file) == 2) {
    if (move.to.file == 2) {  // castling long
      board[3][move.to.rank] = board[0][move.to.rank];
      board[0][move.to.rank] = nullopt;
      can_castle[turn].queen_side = false;
    } else {  // castling short
      board[5][move.to.rank] = board[7][move.to.rank];
      board[7][move.to.rank] = nullopt;
      can_castle[turn].king_side = false;
    }
  }
  if (move.from.rank == (turn ? 0 : 7)) {
    if (move.piece.piece == KING) {
      can_castle[turn] = {false, false};
    } else if (move.piece.piece == ROOK) {
      if (move.from.file == 0) {
        can_castle[turn].queen_side = false;
      } else if (move.from.file == 7) {
        can_castle[turn].queen_side = false;
      }
    }
  }

  history.push_back(move);
  turn = invert(turn);
}

// const string ANSI_RED = "\033[31m";
const string ANSI_INVERT = "\033[0;0;7m";
const string ANSI_RESET = "\033[0m";

string invert(const string &str) {
  return ANSI_INVERT + str + ANSI_RESET;
}

const uint8_t FORWARD_8[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t REVERSE_8[8] = {7, 6, 5, 4, 3, 2, 1, 0};

const uint8_t BOARD_HEADER_HEIGHT = 2;
const uint8_t BOARD_CONTENT_HEIGHT = 8;
const uint8_t BOARD_FOOTER_HEIGHT = 1;
const uint8_t BOARD_HEIGHT =
    BOARD_HEADER_HEIGHT + BOARD_CONTENT_HEIGHT + BOARD_FOOTER_HEIGHT;

array<string, BOARD_HEIGHT> board_to_lines(
    const Board &board, const Color color = white
) {
  array<string, BOARD_HEIGHT> lines;
  lines[0] = color ? "       WHITE        " : "       BLACK        ";
  lines[1] = color ? "  a b c d e f g h   " : "  h g f e d c b a   ";
  lines[10] = color ? "  a b c d e f g h   " : "  h g f e d c b a   ";

  Color square_color = black;
  for (const uint8_t rank : color ? REVERSE_8 : FORWARD_8) {
    const uint8_t line = (color ? 7 - rank : rank) + BOARD_HEADER_HEIGHT;
    lines[line] = to_string(rank + 1) + " ";
    for (const uint8_t file : color ? FORWARD_8 : REVERSE_8) {
      optional<ColorPiece> piece = board[file][rank];
      string piece_string = "  ";
      if (piece) {
        piece_string = to_string(square_color ? invert(*piece) : *piece) + " ";
      }
      lines[line] += square_color ? invert(piece_string) : piece_string;
      square_color = invert(square_color);
    }
    square_color = invert(square_color);
    lines[line] += " " + to_string(rank + 1);
  }

  return lines;
}

template <size_t size>
array<string, size> concat_lines(
    const array<string, size> &a, const array<string, size> &b
) {
  array<string, size> result = {};
  for (u_long i = 0; i < size; ++i) {
    result[i] = a[i] + b[i];
  }
  return result;
}

template <size_t size> string join_lines(const array<string, size> &lines) {
  string result = "";
  for (const string &line : lines) {
    result += line + "\n";
  }
  return result;
}

void print_board(const Board &board) {
  array<string, BOARD_HEIGHT> gap;
  fill(gap.begin(), gap.end(), "   ");
  cout << join_lines(concat_lines(
      concat_lines<BOARD_HEIGHT>(board_to_lines(board), gap),
      board_to_lines(board, black)
  ));
}

void print_history(const vector<Move> &history) {
  const ushort len = history.size();
  for (ushort mv = 0; mv + 1 < len; mv += 2) {
    cout << (mv / 2 + 1) << ".\t" << history[mv].algebraic << "\t"
         << history[mv + 1].algebraic << endl;
  }
  if (len % 2) {
    cout << (len / 2 + 1) << ".\t" << history.back().algebraic << endl;
  }
}

int main() {
  Game game;

  bool exit = false;
  while (!exit) {
    if (game.turn) {
      cout << "                "
           << "{ Move " << (game.history.size() + 1) << " }" << endl;
    }
    cout << endl;
    print_board(game.board);
    cout << endl;

    while (true) {
      cout << (game.turn ? "White> " : "Black> ");
      string input;
      cin >> input;
      if (cin.eof()) {
        exit = true;
        break;
      }

      if (input.starts_with("sum") || input.starts_with("hist")) {
        print_history(game.history);

      } else if (input == "exit" || input == "quit" || input == "") {
        exit = true;

      } else if (input == "restart" || input == "reset") {
        game = Game();
        break;

      } else {
        try {
          Move move = decode_move(game, input);
          Game updated = game;
          apply_move(updated, move);
          if (is_in_check(updated, game.turn)) {
            throw string("You are in check.");
          } else {
            game = updated;
            break;
          }
        } catch (string err) {
          cout << "Invalid move: " << err << endl;
        }
      }
    }
    cout << endl;
  }

  cout << "\nBye." << endl;
  return 0;
}
