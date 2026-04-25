#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <set>

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
  // 3) Subset inference between neighboring numbered cells
  struct CellInfo{int r,c,need; std::vector<std::pair<int,int>> unknowns;};
  std::vector<CellInfo> infos;
  infos.reserve(rows*columns);
  for (int r = 0; r < rows; ++r) for (int c = 0; c < columns; ++c) {
    char ch = view[r][c];
    if (ch >= '1' && ch <= '8') {
      int need = ch - '0';
      int marked = 0; std::vector<std::pair<int,int>> unk;
      for (int k = 0; k < 8; ++k) {
        int nr = r + dr8[k], nc = c + dc8[k];
        if (!inb(nr, nc)) continue;
        if (view[nr][nc] == '@') ++marked;
        else if (view[nr][nc] == '?') unk.emplace_back(nr,nc);
      }
      need -= marked;
      if (need < 0) need = 0;
      if (!unk.empty()) infos.push_back({r,c,need,unk});
    }
  }
  // try pairwise subset reasoning
  for (size_t i = 0; i < infos.size(); ++i) {
    for (size_t j = 0; j < infos.size(); ++j) if (i != j) {
      auto &A = infos[i];
      auto &B = infos[j];
      // build sets
      std::set<std::pair<int,int>> setA(A.unknowns.begin(), A.unknowns.end());
      std::set<std::pair<int,int>> setB(B.unknowns.begin(), B.unknowns.end());
      bool AsubB = std::includes(setB.begin(), setB.end(), setA.begin(), setA.end());
      bool BsubA = std::includes(setA.begin(), setA.end(), setB.begin(), setB.end());
      if (AsubB) {
        // mines in B\A = needB - needA
        std::vector<std::pair<int,int>> diff;
        std::set_difference(setB.begin(), setB.end(), setA.begin(), setA.end(), std::back_inserter(diff));
        int diff_need = B.need - A.need;
        if (diff_need == (int)diff.size() && diff_need > 0) {
          Execute(diff[0].first, diff[0].second, 1);
          return;
        }
        if (diff_need == 0 && !diff.empty()) {
          // all diff are safe
          Execute(diff[0].first, diff[0].second, 0);
          return;
        }
      }
      if (BsubA) {
        std::vector<std::pair<int,int>> diff;
        std::set_difference(setA.begin(), setA.end(), setB.begin(), setB.end(), std::back_inserter(diff));
        int diff_need = A.need - B.need;
        if (diff_need == (int)diff.size() && diff_need > 0) {
          Execute(diff[0].first, diff[0].second, 1);
          return;
        }
        if (diff_need == 0 && !diff.empty()) {
          Execute(diff[0].first, diff[0].second, 0);
          return;
        }
      }
    }
  }
  // 4) Visit an unknown cell neighboring a zero if possible
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
  // 5) Least-risk guess among unknowns
  double bestRisk = 1e9; int br = -1, bc = -1;
  for (int r = 0; r < rows; ++r) for (int c = 0; c < columns; ++c) if (view[r][c] == '?') {
    double risk_sum = 0.0; int cnt = 0;
    for (int k = 0; k < 8; ++k) {
      int nr = r + dr8[k], nc = c + dc8[k];
      if (!inb(nr, nc)) continue;
      char ch = view[nr][nc];
      if (ch >= '0' && ch <= '8') {
        int need = ch - '0'; int marked = 0, unknown = 0;
        for (int t = 0; t < 8; ++t) {
          int xr = nr + dr8[t], xc = nc + dc8[t];
          if (!inb(xr, xc)) continue;
          if (view[xr][xc] == '@') ++marked;
          else if (view[xr][xc] == '?') ++unknown;
        }
        int rem = need - marked; if (rem < 0) rem = 0;
        if (unknown > 0) { risk_sum += double(rem) / unknown; ++cnt; }
      }
    }
    double risk = (cnt > 0) ? (risk_sum / cnt) : 1.0;
    if (risk < bestRisk) { bestRisk = risk; br = r; bc = c; }
  }
  if (br != -1) { Execute(br, bc, 0); return; }

  // Fallback
  Execute(0, 0, 2);
}

#endif