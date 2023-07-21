#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <string>

using namespace std;

/** This file is full of random experiments – read it at your own risk. */

// ♔ ♕ ♖ ♗ ♘ ♙ ♚ ♛ ♜ ♝ ♞ ♟︎

// enum Piece { queen, king, rook, bishop, knight, pawn };
// enum Color { white, black };
enum Piece {
  white_king,
  white_queen,
  white_rook,
  white_bishop,
  white_knight,
  white_pawn,
  black_king,
  black_queen,
  black_rook,
  black_bishop,
  black_knight,
  black_pawn,
};

typedef optional<Piece> Board[8][8];

void init_board(Board board) {
  // empty squares
  for (ushort rank = 2; rank < 6; ++rank) {
    for (ushort file = 0; file < 8; ++file) {
      board[rank][file] = nullopt;
    }
  }

  // pawns
  for (ushort file = 0; file < 8; ++file) {
    board[1][file] = white_pawn;
    board[6][file] = black_pawn;
  }

  // white pieces
  board[0][0] = white_rook;
  board[0][1] = white_knight;
  board[0][2] = white_bishop;
  board[0][3] = white_queen;
  board[0][4] = white_king;
  board[0][5] = white_bishop;
  board[0][6] = white_knight;
  board[0][7] = white_rook;

  // black pieces
  board[7][0] = black_rook;
  board[7][1] = black_knight;
  board[7][2] = black_bishop;
  board[7][3] = black_queen;
  board[7][4] = black_king;
  board[7][5] = black_bishop;
  board[7][6] = black_knight;
  board[7][7] = black_rook;
  // Piece black_pieces[8] = {black_rook, black_knight, black_bishop,
  // black_queen,
  //                          black_king, black_bishop, black_knight,
  //                          black_rook};
  // copy(black_pieces[0], black_pieces[7], board[7]);
}

// const string ANSI_RED = "\033[31m";
const string ANSI_INVERT = "\033[0;0;7m";
const string ANSI_RESET = "\033[0m";

map<Piece, string> pieces{
    {white_king, "♔"},   {white_queen, "♕"},  {white_rook, "♖"},
    {white_bishop, "♗"}, {white_knight, "♘"}, {white_pawn, "♙"},
    {black_king, "♚"},   {black_queen, "♛"},  {black_rook, "♜"},
    {black_bishop, "♝"}, {black_knight, "♞"}, {black_pawn, "♟︎"},
};
map<Piece, Piece> inverted_pieces{
    {white_king, black_king},     {white_queen, black_queen},
    {white_rook, black_rook},     {white_bishop, black_bishop},
    {white_knight, black_knight}, {white_pawn, black_pawn},
    {black_king, white_king},     {black_queen, white_queen},
    {black_rook, white_rook},     {black_bishop, white_bishop},
    {black_knight, white_knight}, {black_pawn, white_pawn},
};

const ushort forward8[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const ushort reverse8[8] = {7, 6, 5, 4, 3, 2, 1, 0};

const ushort BOARD_HEADER_HEIGHT = 2;
const ushort BOARD_CONTENT_HEIGHT = 8;
const ushort BOARD_FOOTER_HEIGHT = 1;
const ushort BOARD_HEIGHT =
    BOARD_HEADER_HEIGHT + BOARD_CONTENT_HEIGHT + BOARD_FOOTER_HEIGHT;

array<string, BOARD_HEIGHT> board_to_lines(const Board board,
                                           const bool as_white = true) {
  array<string, BOARD_HEIGHT> lines;
  lines[0] = as_white ? "       WHITE        " : "       BLACK        ";
  lines[1] = as_white ? "  a b c d e f g h   " : "  h g f e d c b a   ";
  lines[10] = as_white ? "  a b c d e f g h   " : "  h g f e d c b a   ";

  bool black_square = true;
  // std::ranges::views has shitty types?
  // auto ranks = as_white ? views::reverse(views::iota(0, 8)) : views::iota(0,
  // 8);
  for (const ushort rank : as_white ? reverse8 : forward8) {
    const ushort line = (as_white ? 7 - rank : rank) + BOARD_HEADER_HEIGHT;
    lines[line] = to_string(rank + 1) + " ";
    for (const ushort file : as_white ? forward8 : reverse8) {
      string piece_character;
      if (board[rank][file] != nullopt) {
        Piece piece = board[rank][file].value();
        if (black_square) {
          piece = inverted_pieces[piece];
        }
        piece_character = pieces[piece];
      } else {
        piece_character = " ";
      }
      // c++23
      // string pc = board[rank][file]
      //                 .and_then([](Piece piece) { return pieces[piece]; })
      //                 .value_or(" ");
      if (black_square) {
        lines[line] += piece_character + " ";
      } else {
        lines[line] += ANSI_INVERT + piece_character + " " + ANSI_RESET;
      }
      black_square = !black_square;
    }
    black_square = !black_square;
    lines[line] += " " + to_string(rank + 1);
  }

  return lines;
}

string concat(string a, string b) { return a + b; }

template <size_t size>
array<string, size> concat_lines(const array<string, size> a,
                                 const array<string, size> b) {
  array<string, size> result = {};
  for (int i = 0; i < size; ++i) {
    result[i] = a[i] + b[i];
  }
  return result;
}

template <size_t size> string join_lines(const array<string, size> lines) {
  string result = "";
  for (string line : lines) {
    result += line + "\n";
  }
  return result;
}

void print_board(const Board board) {
  array<string, BOARD_HEIGHT> gap;
  fill(gap.begin(), gap.end(), "   ");
  cout << join_lines(concat_lines<BOARD_HEIGHT>(
      concat_lines<BOARD_HEIGHT>(board_to_lines(board), gap),
      board_to_lines(board, false)));
}

int main() {
  // cout << concat("abc", "def");
  // array<string, 2> a = {"abc", "ABC"};
  // array<string, 2> b = {"def", "DEFGH"};
  // cout << join_lines(concat_lines<2>(a, b));

  // cout << pieces[white_king] << pieces[white_queen] << pieces[white_rook]
  //      << pieces[white_bishop] << pieces[white_knight] << pieces[white_pawn]
  //      << pieces[black_king] << pieces[black_queen] << pieces[black_rook]
  //      << pieces[black_bishop] << pieces[black_knight] << pieces[black_pawn]
  //      << endl;

  // cout << join_lines(board_to_lines(board));
  // cout << join_lines(board_to_lines(board, false));

  Board board;
  init_board(board);
  print_board(board);

  return 0;
}
