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
#include <functional>

extern int rows;
extern int columns;
extern int total_mines;

void Execute(int r, int c, int type);

static std::vector<std::string> view;
static const int dr8[8] = {-1,-1,-1,0,0,1,1,1};
static const int dc8[8] = {-1,0,1,-1,1,-1,0,1};

static bool inb(int r, int c) { return r >= 0 && r < rows && c >= 0 && c < columns; }

void InitGame() {
  view.assign(rows, std::string(columns, '?'));
  std::srand(1234567);
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

// Build constraints on frontier unknown cells
static void build_constraints(std::vector<std::pair<int,int>>& vars,
                              std::vector<std::vector<int>>& cons_vars,
                              std::vector<int>& cons_need,
                              std::vector<int>& var_of_pos) {
  var_of_pos.assign(rows*columns, -1);
  vars.clear();
  cons_vars.clear();
  cons_need.clear();
  // collect frontier unknowns
  for (int r = 0; r < rows; ++r) for (int c = 0; c < columns; ++c) if (view[r][c] == '?') {
    bool near = false;
    for (int k = 0; k < 8 && !near; ++k) {
      int nr = r + dr8[k], nc = c + dc8[k];
      if (!inb(nr,nc)) continue;
      char ch = view[nr][nc];
      if (ch >= '0' && ch <= '8') near = true;
    }
    if (near) {
      var_of_pos[r*columns + c] = (int)vars.size();
      vars.emplace_back(r,c);
    }
  }
  // constraints from numbered cells
  for (int r = 0; r < rows; ++r) for (int c = 0; c < columns; ++c) {
    char ch = view[r][c];
    if (ch >= '0' && ch <= '8') {
      int need = ch - '0';
      std::vector<int> vlist;
      int marked = 0;
      for (int k = 0; k < 8; ++k) {
        int nr = r + dr8[k], nc = c + dc8[k];
        if (!inb(nr,nc)) continue;
        if (view[nr][nc] == '@') ++marked;
        else if (view[nr][nc] == '?') {
          int id = var_of_pos[nr*columns + nc];
          if (id != -1) vlist.push_back(id);
        }
      }
      need -= marked; if (need < 0) need = 0;
      if (!vlist.empty()) { cons_vars.push_back(vlist); cons_need.push_back(need); }
    }
  }
}

// Backtracking to enumerate satisfying assignments up to a limit of variables
static bool solve_local(const std::vector<std::pair<int,int>>& vars,
                        const std::vector<std::vector<int>>& cons_vars,
                        const std::vector<int>& cons_need,
                        std::vector<double>& prob) {
  int n = (int)vars.size();
  const int NMAX = 28; // limit enumeration size (balanced for TL)
  if (n == 0) return false;
  if (n > NMAX) return false;
  int m = (int)cons_vars.size();
  std::vector<int> sum(m,0), left(m,0), need = cons_need;
  for (int i = 0; i < m; ++i) left[i] = (int)cons_vars[i].size();
  std::vector<int> assign(n, -1);
  long long total = 0;
  std::vector<long long> one(n, 0);

  // build adjacency: var -> constraints it appears in
  std::vector<std::vector<int>> varAdj(n);
  for (int ci = 0; ci < m; ++ci) for (int id : cons_vars[ci]) varAdj[id].push_back(ci);

  // variable ordering: by appearance count (most constrained first)
  std::vector<int> order(n); for (int i=0;i<n;++i) order[i]=i;
  std::stable_sort(order.begin(), order.end(), [&](int a,int b){ return varAdj[a].size()>varAdj[b].size(); });

  std::function<void(int)> dfs = [&](int idx){
    if (idx == n) {
      for (int i = 0; i < m; ++i) if (sum[i] != need[i]) return;
      ++total;
      for (int i = 0; i < n; ++i) if (assign[i] == 1) ++one[i];
      return;
    }
    int var = order[idx];
    for (int val = 0; val <= 1; ++val) {
      bool ok = true;
      // apply only to affected constraints
      for (int ci : varAdj[var]) {
        if (val == 1) ++sum[ci];
        --left[ci];
        if (sum[ci] > need[ci]) { ok = false; }
        if (sum[ci] + left[ci] < need[ci]) { ok = false; }
      }
      if (ok) {
        assign[var] = val;
        dfs(idx+1);
        assign[var] = -1;
      }
      // rollback
      for (int ci : varAdj[var]) {
        if (val == 1) --sum[ci];
        ++left[ci];
      }
    }
  };

  dfs(0);
  if (total == 0) return false;
  prob.assign(n, 0.0);
  for (int i = 0; i < n; ++i) prob[i] = double(one[i]) / double(total);
  return true;
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
        if (unknown > 0 && marked == need) { Execute(r, c, 2); return; }
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
        if (unknown > 0 && marked + unknown == need) { Execute(ur, uc, 1); return; }
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
      need -= marked; if (need < 0) need = 0;
      if (!unk.empty()) infos.push_back({r,c,need,unk});
    }
  }
  for (size_t i = 0; i < infos.size(); ++i) {
    for (size_t j = 0; j < infos.size(); ++j) if (i != j) {
      auto &A = infos[i]; auto &B = infos[j];
      std::set<std::pair<int,int>> setA(A.unknowns.begin(), A.unknowns.end());
      std::set<std::pair<int,int>> setB(B.unknowns.begin(), B.unknowns.end());
      bool AsubB = std::includes(setB.begin(), setB.end(), setA.begin(), setA.end());
      bool BsubA = std::includes(setA.begin(), setA.end(), setB.begin(), setB.end());
      if (AsubB) {
        std::vector<std::pair<int,int>> diff;
        std::set_difference(setB.begin(), setB.end(), setA.begin(), setA.end(), std::back_inserter(diff));
        int diff_need = B.need - A.need;
        if (diff_need == (int)diff.size() && diff_need > 0) { Execute(diff[0].first, diff[0].second, 1); return; }
        if (diff_need == 0 && !diff.empty()) { Execute(diff[0].first, diff[0].second, 0); return; }
      }
      if (BsubA) {
        std::vector<std::pair<int,int>> diff;
        std::set_difference(setA.begin(), setA.end(), setB.begin(), setB.end(), std::back_inserter(diff));
        int diff_need = A.need - B.need;
        if (diff_need == (int)diff.size() && diff_need > 0) { Execute(diff[0].first, diff[0].second, 1); return; }
        if (diff_need == 0 && !diff.empty()) { Execute(diff[0].first, diff[0].second, 0); return; }
      }
    }
  }
  // 4) Local CSP on frontier with component decomposition
  std::vector<std::pair<int,int>> vars; std::vector<std::vector<int>> cons_vars; std::vector<int> cons_need, var_of_pos;
  build_constraints(vars, cons_vars, cons_need, var_of_pos);
  // Build var->constraints mapping
  int V = (int)vars.size(); int C = (int)cons_vars.size();
  std::vector<std::vector<int>> var_to_cons(V);
  for (int ci = 0; ci < C; ++ci) for (int id : cons_vars[ci]) var_to_cons[id].push_back(ci);
  // Component decomposition over variable nodes
  std::vector<int> comp(V, -1); int comp_cnt = 0;
  std::vector<int> comp_best_var; std::vector<double> comp_best_prob;
  double global_best_prob = 1e9; int global_best_var = -1;
  for (int s = 0; s < V; ++s) if (comp[s] == -1) {
    // BFS over var-cons-var
    std::vector<int> comp_vars_ids; comp_vars_ids.reserve(32);
    std::vector<int> comp_cons_ids; comp_cons_ids.reserve(32);
    std::vector<char> seen_cons(C, 0);
    std::vector<int> q; q.push_back(s); comp[s] = comp_cnt;
    for (size_t qi = 0; qi < q.size(); ++qi) {
      int v = q[qi]; comp_vars_ids.push_back(v);
      for (int ci : var_to_cons[v]) if (!seen_cons[ci]) {
        seen_cons[ci] = 1; comp_cons_ids.push_back(ci);
        for (int nv : cons_vars[ci]) if (comp[nv] == -1) { comp[nv] = comp_cnt; q.push_back(nv); }
      }
    }
    // Build subproblem
    int n = (int)comp_vars_ids.size();
    if (n == 0) { ++comp_cnt; continue; }
    const int NMAX = 28;
    if (n <= NMAX) {
      std::vector<int> map_old_to_new(V, -1);
      std::vector<std::pair<int,int>> sub_vars(n);
      for (int i = 0; i < n; ++i) { int old = comp_vars_ids[i]; map_old_to_new[old] = i; sub_vars[i] = vars[old]; }
      std::vector<std::vector<int>> sub_cons_vars;
      std::vector<int> sub_need;
      for (int ci : comp_cons_ids) {
        std::vector<int> tmp;
        for (int id : cons_vars[ci]) tmp.push_back(map_old_to_new[id]);
        sub_cons_vars.push_back(std::move(tmp));
        sub_need.push_back(cons_need[ci]);
      }
      std::vector<double> prob;
      if (solve_local(sub_vars, sub_cons_vars, sub_need, prob)) {
        // act on forced moves if any
        for (int i = 0; i < n; ++i) {
          if (prob[i] <= 1e-12) { auto p = sub_vars[i]; Execute(p.first, p.second, 0); return; }
          if (prob[i] >= 1.0 - 1e-12) { auto p = sub_vars[i]; Execute(p.first, p.second, 1); return; }
        }
        // remember best in this component
        int best = 0; for (int i = 1; i < n; ++i) if (prob[i] < prob[best]) best = i;
        if (prob[best] < global_best_prob) { global_best_prob = prob[best]; global_best_var = comp_vars_ids[best]; }
      }
    }
    ++comp_cnt;
  }
  if (global_best_var != -1) { auto p = vars[global_best_var]; Execute(p.first, p.second, 0); return; }
  // 4b) Variable-centric local CSP if components too large
  if (!vars.empty()) {
    int Vn = (int)vars.size(); int Cn = (int)cons_vars.size();
    std::vector<std::vector<int>> var_to_cons2(Vn);
    for (int ci = 0; ci < Cn; ++ci) for (int id : cons_vars[ci]) var_to_cons2[id].push_back(ci);
    auto local_prob = [&](int center_id)->double{
      const int LIMIT = 20;
      std::vector<int> var_seen(Vn, 0), cons_seen(Cn, 0);
      std::vector<int> q; q.push_back(center_id); var_seen[center_id] = 1;
      for (size_t qi = 0; qi < q.size() && (int)q.size() < LIMIT; ++qi) {
        int v = q[qi];
        for (int ci : var_to_cons2[v]) if (!cons_seen[ci]) {
          cons_seen[ci] = 1;
          for (int nv : cons_vars[ci]) if (!var_seen[nv] && (int)q.size() < LIMIT) { var_seen[nv] = 1; q.push_back(nv); }
        }
      }
      int n = (int)q.size();
      std::vector<int> map_old_to_new(Vn, -1);
      std::vector<std::pair<int,int>> sub_vars(n);
      for (int i = 0; i < n; ++i) { int old = q[i]; map_old_to_new[old] = i; sub_vars[i] = vars[old]; }
      std::vector<std::vector<int>> sub_cons_vars; std::vector<int> sub_need;
      for (int ci = 0; ci < Cn; ++ci) if (cons_seen[ci]) {
        std::vector<int> tmp;
        for (int id : cons_vars[ci]) { int nid = map_old_to_new[id]; if (nid != -1) tmp.push_back(nid); }
        if (!tmp.empty()) { sub_cons_vars.push_back(std::move(tmp)); sub_need.push_back(cons_need[ci]); }
      }
      std::vector<double> prob; if (!solve_local(sub_vars, sub_cons_vars, sub_need, prob)) return -1.0;
      int idx = map_old_to_new[center_id]; if (idx < 0) return -1.0;
      return prob[idx];
    };
    double bestP = 2.0; int bestVar = -1;
    for (int vid = 0; vid < Vn; ++vid) {
      double p = local_prob(vid);
      if (p >= 0.0 && p < bestP) { bestP = p; bestVar = vid; }
      if (bestP <= 1e-12) break;
    }
    if (bestVar != -1) { auto p = vars[bestVar]; if (bestP <= 1e-12) { Execute(p.first, p.second, 0); return; } Execute(p.first, p.second, 0); return; }
  }
  // 5) Visit unknown neighboring a zero if possible
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (view[r][c] == '0') {
        for (int k = 0; k < 8; ++k) {
          int nr = r + dr8[k], nc = c + dc8[k];
          if (inb(nr, nc) && view[nr][nc] == '?') { Execute(nr, nc, 0); return; }
        }
      }
    }
  }
  // 6) Least-risk guess among unknowns using local avg; use global baseline if unconstrained
  int marked_count = 0, unknown_count = 0;
  for (int r = 0; r < rows; ++r) for (int c = 0; c < columns; ++c) {
    if (view[r][c] == '@') ++marked_count;
    else if (view[r][c] == '?') ++unknown_count;
  }
  double global_p = (unknown_count > 0) ? std::max(0.0, (double)(total_mines - marked_count) / unknown_count) : 1.0;
  double bestRisk = 1e9; int br = -1, bc = -1;
  for (int r = 0; r < rows; ++r) for (int c = 0; c < columns; ++c) if (view[r][c] == '?') {
    double risk_sum = 0.0; int cnt = 0;
    for (int k = 0; k < 8; ++k) {
      int nr = r + dr8[k], nc = c + dc8[k];
      if (!inb(nr, nc)) continue;
      char ch = view[nr][nc];
      if (ch >= '0' && ch <= '8') {
        int need = ch - '0'; int mcnt = 0, ucnt = 0;
        for (int t = 0; t < 8; ++t) {
          int xr = nr + dr8[t], xc = nc + dc8[t];
          if (!inb(xr, xc)) continue;
          if (view[xr][xc] == '@') ++mcnt; else if (view[xr][xc] == '?') ++ucnt;
        }
        int rem = need - mcnt; if (rem < 0) rem = 0;
        if (ucnt > 0) { risk_sum += double(rem) / ucnt; ++cnt; }
      }
    }
    double risk = (cnt > 0) ? (risk_sum / cnt) : global_p;
    if (risk < bestRisk) { bestRisk = risk; br = r; bc = c; }
  }
  if (br != -1) { Execute(br, bc, 0); return; }
  Execute(0, 0, 2);
}

#endif