/** This file is full of random experiments – read it at your own risk. */

#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <string>

using namespace std;

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
  for (unsigned short rank = 2; rank < 6; ++rank) {
    for (unsigned short file = 0; file < 8; ++file) {
      board[rank][file] = nullopt;
    }
  }

  // pawns
  for (unsigned short file = 0; file < 8; ++file) {
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

void print_board(Board board, bool as_white = true) {
  bool black_square = true;
  // for (unsigned short rank = 0; rank < 8; ++rank) {
  // auto ranks = as_white ? views::reverse(views::iota(0, 8)) : views::iota(0,
  // 8);
  unsigned short start_rank = as_white ? 7 : 0;
  unsigned short rank_step = as_white ? -1 : 1;
  // yikes
  for (unsigned short rank = start_rank; rank + rank_step != 7 - start_rank;
       rank += rank_step) {
    for (unsigned short file = 0; file < 8; ++file) {
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
        cout << piece_character << " ";
      } else {
        cout << ANSI_INVERT << piece_character << " " << ANSI_RESET;
      }
      black_square = !black_square;
    }
    black_square = !black_square;
    cout << "\n";
  }
  cout << endl;
}

int main() {
  cout << pieces[white_king] << pieces[white_queen] << pieces[white_rook]
       << pieces[white_bishop] << pieces[white_knight] << pieces[white_pawn]
       << pieces[black_king] << pieces[black_queen] << pieces[black_rook]
       << pieces[black_bishop] << pieces[black_knight] << pieces[black_pawn]
       << endl;

  Board board;
  init_board(board);
  print_board(board);

  return 0;
}
