#ifndef SERVER_H
#define SERVER_H

#include <cstdlib>
#include <iostream>
#include <queue>
#include <vector>

// Global state
int rows;         // rows of the map
int columns;      // columns of the map
int total_mines;  // number of mines
int game_state;   // 0: ongoing, 1: win, -1: lose

static bool is_mine[35][35];
static bool visited[35][35];
static bool marked[35][35];
static int adj_cnt[35][35];

static const int dr[8] = {-1,-1,-1,0,0,1,1,1};
static const int dc[8] = {-1,0,1,-1,1,-1,0,1};

static bool in_bounds(int r, int c) { return r >= 0 && r < rows && c >= 0 && c < columns; }

static void compute_adj() {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      int cnt = 0;
      for (int k = 0; k < 8; ++k) {
        int ni = i + dr[k], nj = j + dc[k];
        if (in_bounds(ni, nj) && is_mine[ni][nj]) ++cnt;
      }
      adj_cnt[i][j] = cnt;
    }
  }
}

static void try_win() {
  int need = rows * columns - total_mines;
  int vis = 0;
  for (int i = 0; i < rows; ++i) for (int j = 0; j < columns; ++j) if (visited[i][j] && !is_mine[i][j]) ++vis;
  if (vis == need) game_state = 1; else if (game_state == 0) game_state = 0;
}

static void flood_zero(int r, int c) {
  std::queue<std::pair<int,int>> q;
  q.push({r,c});
  visited[r][c] = true;
  while (!q.empty()) {
    auto [x,y] = q.front(); q.pop();
    if (is_mine[x][y]) continue;
    if (adj_cnt[x][y] != 0) continue;
    for (int k = 0; k < 8; ++k) {
      int nx = x + dr[k], ny = y + dc[k];
      if (!in_bounds(nx, ny)) continue;
      if (visited[nx][ny]) continue;
      if (marked[nx][ny]) continue; // marked cells are not auto-visited
      visited[nx][ny] = true;
      if (!is_mine[nx][ny] && adj_cnt[nx][ny] == 0) q.push({nx, ny});
    }
  }
}

void InitMap() {
  std::cin >> rows >> columns;
  total_mines = 0;
  game_state = 0;
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      is_mine[i][j] = visited[i][j] = marked[i][j] = false;
      adj_cnt[i][j] = 0;
    }
  }
  for (int i = 0; i < rows; ++i) {
    std::string line; std::cin >> line;
    for (int j = 0; j < columns && j < (int)line.size(); ++j) {
      if (line[j] == 'X') { is_mine[i][j] = true; ++total_mines; }
    }
  }
  compute_adj();
}

void VisitBlock(int r, int c) {
  if (game_state != 0) return;
  if (!in_bounds(r,c)) return;
  if (visited[r][c]) { try_win(); return; }
  if (marked[r][c]) { try_win(); return; }
  if (is_mine[r][c]) {
    visited[r][c] = true;
    game_state = -1;
    return;
  }
  visited[r][c] = true;
  if (adj_cnt[r][c] == 0) flood_zero(r, c);
  try_win();
}

void MarkMine(int r, int c) {
  if (game_state != 0) return;
  if (!in_bounds(r,c)) return;
  if (visited[r][c]) return;
  if (marked[r][c]) return; // no toggle
  marked[r][c] = true;
  if (!is_mine[r][c]) {
    game_state = -1;
    return;
  }
  // correct mark, game continues (win only depends on visited non-mine)
  try_win();
}

void AutoExplore(int r, int c) {
  if (game_state != 0) return;
  if (!in_bounds(r,c)) return;
  if (!visited[r][c]) return;
  if (is_mine[r][c]) return;
  int need = adj_cnt[r][c];
  int marked_nei = 0;
  for (int k = 0; k < 8; ++k) {
    int nx = r + dr[k], ny = c + dc[k];
    if (!in_bounds(nx, ny)) continue;
    if (marked[nx][ny]) ++marked_nei;
  }
  if (marked_nei != need) return;
  for (int k = 0; k < 8; ++k) {
    int nx = r + dr[k], ny = c + dc[k];
    if (!in_bounds(nx, ny)) continue;
    if (visited[nx][ny]) continue;
    if (marked[nx][ny]) continue;
    if (!is_mine[nx][ny]) {
      VisitBlock(nx, ny);
      if (game_state != 0) break;
    }
  }
  if (game_state == 0) try_win();
}

void ExitGame() {
  if (game_state == 1) {
    std::cout << "YOU WIN!" << std::endl;
  } else {
    std::cout << "GAME OVER!" << std::endl;
  }
  int visit_cnt = 0, marked_mine_cnt = 0;
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      if (visited[i][j] && !is_mine[i][j]) ++visit_cnt;
      if (marked[i][j] && is_mine[i][j]) ++marked_mine_cnt;
    }
  }
  if (game_state == 1) marked_mine_cnt = total_mines;
  std::cout << visit_cnt << " " << marked_mine_cnt << std::endl;
  exit(0);
}

void PrintMap() {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      char ch = '?';
      if (game_state == 1) { // reveal all mines as '@'
        if (is_mine[i][j]) ch = '@';
        else if (visited[i][j]) ch = char('0' + adj_cnt[i][j]);
        else ch = '?';
      } else {
        if (marked[i][j]) {
          ch = is_mine[i][j] ? '@' : 'X';
        } else if (visited[i][j]) {
          ch = is_mine[i][j] ? 'X' : char('0' + adj_cnt[i][j]);
        } else {
          ch = '?';
        }
      }
      std::cout << ch;
    }
    std::cout << std::endl;
  }
}

#endif
