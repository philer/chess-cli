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
  pawn,
  knight,
  bishop,
  rook,
  queen,
  king,
};

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
  static const map<Piece, array<string, 2>> UTF8_PIECES{
      {king, {"♔", "♚"}},
      {queen, {"♕", "♛"}},
      {rook, {"♖", "♜"}},
      {bishop, {"♗", "♝"}},
      {knight, {"♘", "♞"}},
      {pawn, {"♙", "♟︎"}},
  };
  return UTF8_PIECES.at(piece.piece)[piece.color];
}

const ColorPiece WHITE_PAWN = {white, pawn};
const ColorPiece WHITE_KNIGHT = {white, knight};
const ColorPiece WHITE_BISHOP = {white, bishop};
const ColorPiece WHITE_ROOK = {white, rook};
const ColorPiece WHITE_QUEEN = {white, queen};
const ColorPiece WHITE_KING = {white, king};
const ColorPiece BLACK_PAWN = {black, pawn};
const ColorPiece BLACK_KNIGHT = {black, knight};
const ColorPiece BLACK_BISHOP = {black, bishop};
const ColorPiece BLACK_ROOK = {black, rook};
const ColorPiece BLACK_QUEEN = {black, queen};
const ColorPiece BLACK_KING = {black, king};

typedef array<array<optional<ColorPiece>, 8>, 8> Board;

Board create_board() {
  Board board;
  // empty squares
  for (uint8_t rank = 2; rank < 6; ++rank) {
    for (uint8_t file = 0; file < 8; ++file) {
      board[file][rank] = nullopt;
    }
  }

  // pawns
  for (uint8_t file = 0; file < 8; ++file) {
    board[file][1] = WHITE_PAWN;
    board[file][6] = BLACK_PAWN;
  }

  // white pieces
  board[0][0] = WHITE_ROOK;
  board[1][0] = WHITE_KNIGHT;
  board[2][0] = WHITE_BISHOP;
  board[3][0] = WHITE_QUEEN;
  board[4][0] = WHITE_KING;
  board[5][0] = WHITE_BISHOP;
  board[6][0] = WHITE_KNIGHT;
  board[7][0] = WHITE_ROOK;

  // black pieces
  board[0][7] = BLACK_ROOK;
  board[1][7] = BLACK_KNIGHT;
  board[2][7] = BLACK_BISHOP;
  board[3][7] = BLACK_QUEEN;
  board[4][7] = BLACK_KING;
  board[5][7] = BLACK_BISHOP;
  board[6][7] = BLACK_KNIGHT;
  board[7][7] = BLACK_ROOK;

  return board;
}

struct Square {
  uint8_t file;
  uint8_t rank;

  bool exists() const {
    return 0 <= file && file <= 7 && 0 <= rank && rank <= 7;
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

// Disallowing lower case piece letters for now.
// Ambiguous case: "bc6" could be a bishop or a pawn capture
static const string PIECE_CHARS = "NBRQK";

ColorPiece get_piece(const char piece_character, const Color color) {
  switch (piece_character) {
    case 'K':
      return {color, king};
    case 'Q':
      return {color, queen};
    case 'R':
      return {color, rook};
    case 'B':
      return {color, bishop};
    case 'N':
      return {color, knight};
    case 'P':
      return {color, pawn};
    default:
      string piece_str;
      piece_str = piece_character;
      throw string("Invalid piece '" + piece_str + "'");
  }
}

optional<ColorPiece> find_piece(const Board &board, const Square &square) {
  return board[square.file][square.rank];
}

bool has_piece(
    const Board &board, const Square &square, const ColorPiece &piece
) {
  return board[square.file][square.rank] == piece;
}

template <size_t N>
vector<Square> find_line_moving_pieces(
    const Board &board,
    const Square &target_square,
    const ColorPiece &piece,
    const array<tuple<int8_t, int8_t>, N> &directions,
    optional<uint8_t> file = nullopt,
    optional<uint8_t> rank = nullopt
) {
  vector<Square> found;
  for (const auto [d_file, d_rank] : directions) {
    for (uint8_t offset = 1;; ++offset) {
      Square square = {
          static_cast<uint8_t>(target_square.file + offset * d_file),
          static_cast<uint8_t>(target_square.rank + offset * d_rank)};
      if (!square.exists()) {
        break;
      }
      const optional<ColorPiece> found_piece = find_piece(board, square);
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

vector<Square> find_direct_moving_pieces(
    const Board &board,
    const Square &target_square,
    const ColorPiece &piece,
    const array<tuple<int8_t, int8_t>, 8> &moves,
    optional<uint8_t> file = nullopt,
    optional<uint8_t> rank = nullopt
) {
  vector<Square> found;
  for (const auto [d_file, d_rank] : moves) {
    Square square = {
        static_cast<uint8_t>(target_square.file + d_file),
        static_cast<uint8_t>(target_square.rank + d_rank)};
    if (!square.exists()) {
      continue;
    }
    if (file && square.file != *file || rank && square.rank != *rank) {
      continue;
    }
    if (find_piece(board, square) == piece) {
      found.push_back(square);
    }
  }
  return found;
}

vector<Square> find_pieces(
    const Board &board,
    const Square &target_square,
    const ColorPiece &piece,
    optional<uint8_t> file = nullopt,
    optional<uint8_t> rank = nullopt
) {
  switch (piece.piece) {
      // clang-format off
    case knight:
      return find_direct_moving_pieces(
          board, target_square, piece,
          {{{+1, +2}, {+1, -2}, {-1, +2}, {-1, -2},
            {+2, +1}, {+2, -1}, {-2, +1}, {-2, -1}}},
          file, rank
      );
    case bishop:
      return find_line_moving_pieces<4>(
          board, target_square, piece,
          {{{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1}}},
          file, rank
      );
    case rook:
      return find_line_moving_pieces<4>(
          board, target_square, piece,
          {{{0, -1}, {0, +1}, {-1, 0}, {+1, 0}}},
          file, rank
      );
    case queen:
      return find_line_moving_pieces<8>(
          board, target_square, piece,
          {{{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1},
            { 0, -1}, { 0, +1}, {-1,  0}, {+1,  0}}},
          file, rank
      );
    case king:
      return find_direct_moving_pieces(
          board, target_square, piece,
          {{{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1},
            { 0, -1}, { 0, +1}, {-1,  0}, {+1,  0}}},
          file, rank
      );
    // clang-format on
    default:
      throw string("Something went wrong!");
  }
}

// TODO shortened pawn captures ("exd", "ed")
const regex PAWN_MOVE_PATTERN{"^[a-h][1-8][NBRQ]?$"};
const regex PAWN_CAPTURE_PATTERN{"^[a-h]x[a-h][1-8][NBRQ]?$"};
const regex PIECE_MOVE_OR_CAPTURE_PATTERN{"^[NBRQK][a-h]?[1-8]?x?[a-h][1-8]$"};
const regex CASTLING_PATTERN{"^[O0]-[O0](?:-[O0])?$", regex::icase};

optional<ColorPiece> get_promotion(
    const Board $board, const Move &move, const Color color
) {
  optional<ColorPiece> promotion = nullopt;
  if (string("NBRQ").find(move.algebraic.back()) != string::npos) {
    // could use try-catch instead
    promotion = get_piece(move.algebraic.back(), color);
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
Move decode_move(const Board &board, const string &move, const Color color) {
  Move mv;
  mv.algebraic = move;
  mv.check = false;
  const int8_t forwards = color ? 1 : -1;
  const uint8_t len = move.size();

  if (regex_match(move, PAWN_MOVE_PATTERN)) {  // "e4"
    mv.piece = get_piece('P', color);
    mv.to = get_square(move[0], move[1]);
    mv.from.file = mv.to.file;

    if (board[mv.from.file][mv.to.rank - forwards] == mv.piece) {
      mv.from.rank = mv.to.rank - forwards;

    } else if ((color && mv.to.rank == 3 || !color && mv.to.rank == 4) &&
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
    if (find_piece(board, mv.to)) {
      throw string(
          to_string(mv.to) + " is blocked. Pawns can only capture diagonally."
      );
    }

    mv.promotion = get_promotion(board, mv, color);

  } else if (regex_match(move, PAWN_CAPTURE_PATTERN)) {  // "dxe4"
    mv.piece = get_piece('P', color);
    mv.to = get_square(move[2], move[3]);
    mv.from = get_square(move[0], move[3] - forwards);
    if (abs(mv.from.file - mv.to.file) != 1) {
      throw string("Pawn must move one square diagonally when capturing.");
    }
    if (!has_piece(board, mv.from, mv.piece)) {
      throw string("No eligible Pawn on " + to_string(mv.from) + ".");
    }
    mv.capture = find_piece(board, mv.to);
    if (!mv.capture) {
      // Check for en passant capture
      // TODO check if opponent's pawn just moved by two ranks
      const optional<ColorPiece> en_passant_capture =
          find_piece(board, get_square(move[2], move[3] - forwards));
      if (en_passant_capture == ColorPiece{invert(color), pawn}) {
        mv.capture = en_passant_capture;
      } else {
        throw string(
            "There is nothing to capture on " + to_string(mv.to) + "."
        );
      }
    }
    if (color == mv.capture->color) {
      throw string("Can't capture your own piece.");
    }

    mv.promotion = get_promotion(board, mv, color);

  } else if (regex_match(move, PIECE_MOVE_OR_CAPTURE_PATTERN)) {
    // "Qe4, Qxe4, Qde4, Qdxe4, Q3e4, Q3xe4, Qd3e4, Qd3xe4"

    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[len - 2], move[len - 1]);

    // Decode optional starting square qualifiers
    optional<char> from_file = nullopt;
    optional<char> from_rank = nullopt;
    for (const char detail : move.substr(1, len - 3)) {
      if (string("abcdefgh").find(detail) != string::npos) {
        from_file = detail - 'a';
      } else if (string("12345678").find(detail) != string::npos) {
        from_rank = detail - '1';
        break;
      }
    }

    // Search for matching pieces
    const vector<Square> candidates =
        find_pieces(board, mv.to, mv.piece, from_file, from_rank);
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
    mv.capture = find_piece(board, mv.to);
    if (move[len - 3] == 'x') {
      if (!mv.capture) {
        throw string(
            "There is nothing to capture on " + to_string(mv.to) + "."
        );
      } else if (color == mv.capture->color) {
        throw string("Can't capture your own piece.");
      }
    } else {
      if (mv.capture) {
        throw string("Target square is occupied")
            + (color == mv.capture->color ? " by your own piece."
                                          : ", add 'x' to capture.");
      }
    }

  } else if (regex_match(move, CASTLING_PATTERN)) {
    // TODO check castling rights
    // TODO check king passing squares under check
    const bool castle_long = move.length() == 5;
    const uint8_t rank = white ? 0 : 7;
    if (board[4][rank] != ColorPiece{color, king}
        || (castle_long
                ? board[0][rank] != ColorPiece{color, rook} || board[1][rank]
                      || board[2][rank] && !board[3][rank]
                : board[7][rank] != ColorPiece{color, rook} || board[6][rank]
                      || board[5][rank])) {
      throw string("You can't castle on this side of the board right now.");
    }
    mv.piece = ColorPiece{color, king};
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
void apply_move(Board &board, const Move &move) {
  const ColorPiece piece = *board[move.from.file][move.from.rank];

  // capture en passant
  const optional<ColorPiece> capture = board[move.to.file][move.to.rank];
  if (!capture && move.capture == ColorPiece(invert(piece.color), pawn)) {
    board[move.to.file][move.from.rank] = nullopt;
  }

  // move
  board[move.from.file][move.from.rank] = nullopt;
  board[move.to.file][move.to.rank] = move.promotion.value_or(piece);

  // castling
  if ((piece == WHITE_KING || piece == BLACK_KING)
      && abs(move.from.file - move.to.file) == 2) {
    cout << "castling";
    if (move.to.file == 2) {  // castling long
      board[3][move.to.rank] = board[0][move.to.rank];
      board[0][move.to.rank] = nullopt;
    } else {  // castling short
      board[5][move.to.rank] = board[7][move.to.rank];
      board[7][move.to.rank] = nullopt;
    }
  }
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
  for (int i = 0; i < size; ++i) {
    result[i] = a[i] + b[i];
  }
  return result;
}

template <size_t size> string join_lines(const array<string, size> &lines) {
  string result = "";
  for (string line : lines) {
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

int main() {
  Board board = create_board();
  Color color = white;
  uint move_no = 0;
  string move;
  Move decoded_move;

  bool exit = false;
  while (true) {
    if (color) {
      move_no += 1;
      cout << "                { Move " << move_no << " }" << endl;
    }
    print_board(board);
    cout << endl;

    while (true) {
      try {
        if (cin.eof()) {
          exit = true;
          break;
        }
        cout << (color ? "White> " : "Black> ");
        cin >> move;
        if (move == "exit" || move == "quit" || move == "q" || move == "") {
          exit = true;
        } else {
          decoded_move = decode_move(board, move, color);
        }
        break;
      } catch (string err) {
        cout << err << endl;
      }
    }
    if (exit) {
      break;
    }

    cout << endl;
    apply_move(board, decoded_move);
    color = invert(color);
  }

  cout << "\nBye." << endl;
  return 0;
}
