#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

extern int rows;         // The count of rows of the game map.
extern int columns;      // The count of columns of the game map.
extern int total_mines;  // The count of mines of the game map.

// You MUST NOT use any other external variables except for rows, columns and total_mines.

void Execute(int r, int c, int type);

static std::vector<std::string> view;
static const int dr8[8] = {-1,-1,-1,0,0,1,1,1};
static const int dc8[8] = {-1,0,1,-1,1,-1,0,1};

static bool inb(int r, int c) { return r >= 0 && r < rows && c >= 0 && c < columns; }

void InitGame() {
  view.assign(rows, std::string(columns, '?'));
  std::srand(1234567); // deterministic
  int first_row, first_column;
  std::cin >> first_row >> first_column;
  Execute(first_row, first_column, 0);
}

void ReadMap() {
  view.resize(rows);
  for (int i = 0; i < rows; ++i) {
    std::string line; std::cin >> line;
    view[i] = line;
  }
}

void Decide() {
  // 1) AutoExplore whenever possible
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      char ch = view[r][c];
      if (ch >= '0' && ch <= '8') {
        int need = ch - '0';
        int marked = 0, unknown = 0;
        for (int k = 0; k < 8; ++k) {
          int nr = r + dr8[k], nc = c + dc8[k];
          if (!inb(nr, nc)) continue;
          if (view[nr][nc] == '@') ++marked;
          else if (view[nr][nc] == '?') ++unknown;
        }
        if (unknown > 0 && marked == need) {
          Execute(r, c, 2);
          return;
        }
      }
    }
  }
  // 2) Mark obvious mines (k == marked + unknown)
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      char ch = view[r][c];
      if (ch >= '0' && ch <= '8') {
        int need = ch - '0';
        int marked = 0, unknown = 0; int ur = -1, uc = -1;
        for (int k = 0; k < 8; ++k) {
          int nr = r + dr8[k], nc = c + dc8[k];
          if (!inb(nr, nc)) continue;
          if (view[nr][nc] == '@') ++marked;
          else if (view[nr][nc] == '?') { ++unknown; ur = nr; uc = nc; }
        }
        if (unknown > 0 && marked + unknown == need) {
          Execute(ur, uc, 1);
          return;
        }
      }
    }
  }
  // 3) Visit an unknown cell (simple heuristic: prefer neighbors of zeros if visible)
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (view[r][c] == '0') {
        for (int k = 0; k < 8; ++k) {
          int nr = r + dr8[k], nc = c + dc8[k];
          if (inb(nr, nc) && view[nr][nc] == '?') {
            Execute(nr, nc, 0);
            return;
          }
        }
      }
    }
  }
  // 4) Otherwise, random unknown
  std::vector<std::pair<int,int>> unk;
  unk.reserve(rows * columns);
  for (int r = 0; r < rows; ++r) for (int c = 0; c < columns; ++c) if (view[r][c] == '?') unk.emplace_back(r,c);
  if (!unk.empty()) {
    auto p = unk[std::rand() % unk.size()];
    Execute(p.first, p.second, 0);
    return;
  }
  // No move (shouldn't happen normally); pick any non-mine-looking operation
  Execute(0, 0, 2);
}

#endif