#include "constants.h"
#include "custom_distributions.h"
#include "node_detail.h"
#include "philox_engine.h"
#include "reg_stack.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
#include <unordered_map>
#include <vector>
#include <fitness.h>
#include <node.h>
#include <numeric>
#include <program.h>
#include <random>
#include <stack>

namespace genetic {

struct leaf_subst {
  const float *vals;
  int start;
};

/**
 * Execution kernel for a single program. We assume that the input data
 * is stored in column major format.
 */
template <int MaxSize = MAX_STACK_SIZE>
void eval_nodes(const node *nodes, int len, const float *data, float *y,
                const uint64_t n_rows, const int *subst_at,
                const leaf_subst *subst) {
  constexpr uint64_t B = 64;
  alignas(64) float regs[MaxSize][B];
  alignas(64) float dropped[B];
  alignas(64) static const float zeros[B] = {};

  for (uint64_t r0 = 0; r0 < n_rows; r0 += B) {
    const uint64_t bs = std::min(B, n_rows - r0);
    int sp = 0;

    for (int k = len - 1; k >= 0; --k) {
      const node &nd = nodes[k];
      float *out = (sp < MaxSize) ? regs[sp] : dropped;

      if (subst_at && subst_at[k] >= 0) {
        const leaf_subst &ls = subst[subst_at[k]];
        for (uint64_t i = 0; i < bs; ++i)
          out[i] = ls.vals[r0 + i];
        k = ls.start;
      } else if (nd.t == node::type::constant) {
        for (uint64_t i = 0; i < bs; ++i)
          out[i] = nd.u.val;
      } else if (nd.t == node::type::variable) {
        const float *col = data + n_rows * nd.u.fid + r0;
        for (uint64_t i = 0; i < bs; ++i)
          out[i] = col[i];
      } else {
        const int ar = detail::arity(nd.t);
        const float *a = zeros, *b = zeros;
        if (sp > 0)
          a = regs[--sp];
        if (ar > 1 && sp > 0)
          b = regs[--sp];
        out = regs[sp];
        switch (nd.t) {
        case node::type::add:
#pragma clang loop vectorize(assume_safety)
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = a[i] + b[i];
          break;
        case node::type::sub:
#pragma clang loop vectorize(assume_safety)
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = a[i] - b[i];
          break;
        case node::type::mul:
#pragma clang loop vectorize(assume_safety)
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = a[i] * b[i];
          break;
        case node::type::fdim:
#pragma clang loop vectorize(assume_safety)
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = a[i] <= b[i] ? 0.0f : a[i] - b[i];
          break;
        case node::type::abs:
#pragma clang loop vectorize(assume_safety)
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = fabsf(a[i]);
          break;
        case node::type::sq:
#pragma clang loop vectorize(assume_safety)
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = a[i] * a[i];
          break;
        case node::type::sqrt:
#pragma clang loop vectorize(assume_safety)
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = sqrtf(fabsf(a[i]));
          break;
        case node::type::sin:
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = sinf(a[i]);
          break;
        case node::type::cos:
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = cosf(a[i]);
          break;
        case node::type::log:
          for (uint64_t i = 0; i < bs; ++i)
            out[i] = fabsf(a[i]) < detail::MIN_VAL ? 0.f : logf(fabsf(a[i]));
          break;
        default:
          for (uint64_t i = 0; i < bs; ++i) {
            float in[2] = {a[i], ar > 1 ? b[i] : 0.0f};
            out[i] = detail::evaluate_node(nd, data, n_rows, r0 + i, in);
          }
          break;
        }
      }
      if (sp < MaxSize)
        ++sp;
    }

    float *dst = y + r0;
    if (sp > 0)
      std::copy(regs[sp - 1], regs[sp - 1] + bs, dst);
    else
      std::fill(dst, dst + bs, 0.0f);
  }
}

struct shared_subtree {
  int pid;
  int k;
  int count;
  int table;
};

static bool same_range(const node *a, const node *b, int len) {
  for (int j = 0; j < len; ++j) {
    if (a[j].t != b[j].t)
      return false;
    if ((a[j].t == node::type::variable || a[j].t == node::type::constant) &&
        std::memcmp(&a[j].u, &b[j].u, sizeof(a[j].u)) != 0)
      return false;
  }
  return true;
}

template <int MaxSize> struct subtree_cache {
  const program_t progs;
  const float *data;
  uint64_t n_rows;
  std::vector<std::vector<uint64_t>> hash;
  std::vector<std::vector<int>> end;
  std::unordered_map<uint64_t, shared_subtree> shared;
  std::deque<std::vector<float>> tables;
  size_t budget;

  void eval_range(int pid, int start, float *out) {
    const program &p = progs[pid];
    const int stop = end[pid][start];
    std::vector<int> subst_at;
    std::vector<leaf_subst> subst;
    for (int j = start + 1; j < stop;) {
      const node &nd = p.nodes[j];
      if (nd.t != node::type::variable && nd.t != node::type::constant) {
        auto it = shared.find(hash[pid][j]);
        if (it != shared.end() && it->second.count >= 2) {
          shared_subtree &s = it->second;
          if (s.table < 0 && tables.size() < budget) {
            tables.emplace_back(n_rows);
            const int t = (int)tables.size() - 1;
            eval_range(s.pid, s.k, tables[t].data());
            s.table = t;
          }
          if (s.table >= 0) {
            if (subst_at.empty())
              subst_at.assign(stop - start, -1);
            subst_at[end[pid][j] - 1 - start] = (int)subst.size();
            subst.push_back({tables[s.table].data(), j - start});
            j = end[pid][j];
            continue;
          }
        }
      }
      ++j;
    }
    eval_nodes<MaxSize>(p.nodes + start, stop - start, data, out, n_rows,
                        subst.empty() ? nullptr : subst_at.data(),
                        subst.data());
  }
};

template <int MaxSize = MAX_STACK_SIZE>
void execute_kernel(const program_t d_progs, const float *data, float *y_pred,
                    const uint64_t n_rows, const uint64_t n_progs) {
  subtree_cache<MaxSize> c{d_progs, data, n_rows, {}, {}, {}, {}, 0};
  c.budget = std::max<size_t>(1, (size_t(256) << 20) / (n_rows * sizeof(float)));
  c.hash.resize(n_progs);
  c.end.resize(n_progs);
  std::vector<char> ok(n_progs, 1);
  std::vector<int> st;

  for (uint64_t pid = 0; pid < n_progs; ++pid) {
    const program &p = d_progs[pid];
    std::vector<uint64_t> &h = c.hash[pid];
    std::vector<int> &e = c.end[pid];
    h.assign(p.len, 0);
    e.assign(p.len, 0);
    st.clear();
    for (int k = p.len - 1; k >= 0 && ok[pid]; --k) {
      const node &nd = p.nodes[k];
      uint64_t x = 0x9E3779B97F4A7C15ull ^ (static_cast<uint64_t>(nd.t) << 32);
      e[k] = k + 1;
      if (nd.t == node::type::variable || nd.t == node::type::constant) {
        uint32_t bits;
        std::memcpy(&bits, &nd.u, sizeof(bits));
        x ^= bits;
      } else {
        const int ar = detail::arity(nd.t);
        if ((int)st.size() < ar) {
          ok[pid] = 0;
          break;
        }
        for (int a = 0; a < ar; ++a) {
          x = (x ^ h[st.back()]) * 0xFF51AFD7ED558CCDull;
          x ^= x >> 29;
          e[k] = e[st.back()];
          st.pop_back();
        }
      }
      h[k] = x * 0xC4CEB9FE1A85EC53ull;
      if ((int)st.size() >= MaxSize)
        ok[pid] = 0;
      st.push_back(k);
    }
    if (st.size() != 1)
      ok[pid] = 0;
    if (!ok[pid])
      continue;
    for (int k = 0; k < p.len; ++k) {
      const node &nd = p.nodes[k];
      if (nd.t == node::type::variable || nd.t == node::type::constant)
        continue;
      auto ins = c.shared.try_emplace(h[k], shared_subtree{(int)pid, k, 0, -1});
      shared_subtree &s = ins.first->second;
      if (s.count < 0)
        continue;
      if (!ins.second &&
          (c.end[s.pid][s.k] - s.k != e[k] - k ||
           !same_range(d_progs[s.pid].nodes + s.k, p.nodes + k, e[k] - k))) {
        s.count = -1;
        continue;
      }
      ++s.count;
    }
  }

  for (uint64_t pid = 0; pid < n_progs; ++pid) {
    if (ok[pid])
      c.eval_range((int)pid, 0, y_pred + pid * n_rows);
    else
      eval_nodes<MaxSize>(d_progs[pid].nodes, d_progs[pid].len, data,
                          y_pred + pid * n_rows, n_rows, nullptr, nullptr);
  }
}

program::program()
    : len(0), depth(0), raw_fitness_(0.0f), metric(metric_t::mse),
      mut_type(mutation_t::none), nodes(nullptr) {}

program::~program() { delete[] nodes; }

program::program(const program &src)
    : len(src.len), depth(src.depth), raw_fitness_(src.raw_fitness_),
      metric(src.metric), mut_type(src.mut_type) {
  nodes = new node[len];
  std::copy(src.nodes, src.nodes + src.len, nodes);
}

program &program::operator=(const program &src) {
  len = src.len;
  depth = src.depth;
  raw_fitness_ = src.raw_fitness_;
  metric = src.metric;
  mut_type = src.mut_type;

  // Copy nodes
  delete[] nodes;
  nodes = new node[len];
  std::copy(src.nodes, src.nodes + src.len, nodes);

  return *this;
}

void compute_metric(int n_rows, int n_progs, const float *y,
                    const float *y_pred, const float *w, float *score,
                    const param &params) {
  // Call appropriate metric function based on metric defined in params
  if (params.metric == metric_t::pearson) {
    weightedPearson(n_rows, n_progs, y, y_pred, w, score);
  } else if (params.metric == metric_t::spearman) {
    weightedSpearman(n_rows, n_progs, y, y_pred, w, score);
  } else if (params.metric == metric_t::mae) {
    meanAbsoluteError(n_rows, n_progs, y, y_pred, w, score);
  } else if (params.metric == metric_t::mse) {
    meanSquareError(n_rows, n_progs, y, y_pred, w, score);
  } else if (params.metric == metric_t::rmse) {
    rootMeanSquareError(n_rows, n_progs, y, y_pred, w, score);
  } else if (params.metric == metric_t::logloss) {
    logLoss(n_rows, n_progs, y, y_pred, w, score);
  } else {
    // This should not be reachable
  }
}

void execute(const program_t &d_progs, const int n_rows, const int n_progs,
             const float *data, float *y_pred) {
  execute_kernel(d_progs, data, y_pred, static_cast<uint64_t>(n_rows),
                 static_cast<uint64_t>(n_progs));
}

void find_fitness(program_t d_prog, float *score, const param &params,
                  const int n_rows, const float *data, const float *y,
                  const float *sample_weights) {

  // Compute predicted values
  std::vector<float> y_pred(n_rows);
  execute(d_prog, n_rows, 1, data, y_pred.data());

  // Compute error
  compute_metric(n_rows, 1, y, y_pred.data(), sample_weights, score, params);
}

void find_batched_fitness(int n_progs, program_t d_progs, float *score,
                          const param &params, const int n_rows,
                          const float *data, const float *y,
                          const float *sample_weights) {

  std::vector<float> y_pred((uint64_t)n_rows * (uint64_t)n_progs);
  execute(d_progs, n_rows, n_progs, data, y_pred.data());

  // Compute error
  compute_metric(n_rows, n_progs, y, y_pred.data(), sample_weights, score,
                 params);
}

void set_fitness(program &h_prog, const param &params, const int n_rows,
                 const float *data, const float *y,
                 const float *sample_weights) {

  std::vector<float> score(1);

  find_fitness(&h_prog, score.data(), params, n_rows, data, y, sample_weights);

  // Update host and device score for program
  h_prog.raw_fitness_ = score[0];
}

void set_batched_fitness(int n_progs, std::vector<program> &h_progs,
                         const param &params, const int n_rows,
                         const float *data, const float *y,
                         const float *sample_weights) {

  std::vector<float> score(n_progs);

  find_batched_fitness(n_progs, h_progs.data(), score.data(), params, n_rows,
                       data, y, sample_weights);

  // Update scores on host and device
  // TODO: Find a way to reduce the number of implicit memory transfers
  for (auto i = 0; i < n_progs; ++i) {
    h_progs[i].raw_fitness_ = score[i];
  }
}

float get_fitness(const program &prog, const param &params) {
  int crit = params.criterion();
  float penalty = params.parsimony_coefficient * prog.len * (2 * crit - 1);
  return (prog.raw_fitness_ - penalty);
}

/**
 * @brief Get a random subtree of the current program nodes (on CPU)
 *
 * @param pnodes  AST represented as a list of nodes
 * @param len     The total number of nodes in the AST
 * @param rng     Random number generator for subtree selection
 * @return A tuple [first,last) which contains the required subtree
 */
std::pair<int, int> get_subtree(node *pnodes, int len, PhiloxEngine &rng) {
  int start, end;
  start = end = 0;

  // Specify RNG
  uniform_real_distribution_custom<float> dist_uniform(0.0f, 1.0f);
  float bound = dist_uniform(rng);

  // Specify subtree start probs acc to Koza's selection approach
  std::vector<float> node_probs(len, 0.1);
  float sum = 0.1 * len;

  for (int i = 0; i < len; ++i) {
    if (pnodes[i].is_nonterminal()) {
      node_probs[i] = 0.9;
      sum += 0.8;
    }
  }

  // Normalize vector
  for (int i = 0; i < len; ++i) {
    node_probs[i] /= sum;
  }

  // Compute cumulative sum
  std::partial_sum(node_probs.begin(), node_probs.end(), node_probs.begin());

  start = std::lower_bound(node_probs.begin(), node_probs.end(), bound) -
          node_probs.begin();
  end = start;

  // Iterate until all function arguments are satisfied in current subtree
  int num_args = 1;
  while (num_args > end - start) {
    node curr;
    curr = pnodes[end];
    if (curr.is_nonterminal())
      num_args += curr.arity();
    ++end;
  }

  return std::make_pair(start, end);
}

int get_depth(const program &p_out) {
  int depth = 0;
  std::stack<int> arity_stack;
  for (auto i = 0; i < p_out.len; ++i) {
    node curr(p_out.nodes[i]);

    // Update depth
    int sz = arity_stack.size();
    depth = std::max(depth, sz);

    // Update stack
    if (curr.is_nonterminal()) {
      arity_stack.push(curr.arity());
    } else {
      // Only triggered for a depth 0 node
      if (arity_stack.empty())
        break;

      int e = arity_stack.top();
      arity_stack.pop();
      arity_stack.push(e - 1);

      while (arity_stack.top() == 0) {
        arity_stack.pop();
        if (arity_stack.empty())
          break;

        e = arity_stack.top();
        arity_stack.pop();
        arity_stack.push(e - 1);
      }
    }
  }

  return depth;
}

void build_program(program &p_out, const param &params, PhiloxEngine &rng) {
  // Define data structures needed for tree
  std::stack<int> arity_stack;
  std::vector<node> nodelist;
  nodelist.reserve(1 << (MAX_STACK_SIZE));

  // Specify Distributions with parameters
  uniform_int_distribution_custom<int> dist_function(
      0, params.function_set.size() - 1);
  uniform_int_distribution_custom<int> dist_initDepth(params.init_depth[0],
                                                      params.init_depth[1]);
  uniform_int_distribution_custom<int> dist_terminalChoice(0,
                                                           params.num_features);
  uniform_real_distribution_custom<float> dist_constVal(params.const_range[0],
                                                        params.const_range[1]);
  bernoulli_distribution_custom dist_nodeChoice(params.terminalRatio);
  bernoulli_distribution_custom dist_coinToss(0.5);

  // Initialize nodes
  int max_depth = dist_initDepth(rng);
  node::type func = params.function_set[dist_function(rng)];
  node curr_node(func);
  nodelist.push_back(curr_node);
  arity_stack.push(curr_node.arity());

  init_method_t method = params.init_method;
  if (method == init_method_t::half_and_half) {
    // Choose either grow or full for this tree
    bool choice = dist_coinToss(rng);
    method = choice ? init_method_t::grow : init_method_t::full;
  }

  // Fill tree
  while (!arity_stack.empty()) {
    int depth = arity_stack.size();
    p_out.depth = std::max(depth, p_out.depth);
    bool node_choice = dist_nodeChoice(rng);

    if ((node_choice == false || method == init_method_t::full) &&
        depth < max_depth) {
      // Add a function to node list
      curr_node = node(params.function_set[dist_function(rng)]);
      nodelist.push_back(curr_node);
      arity_stack.push(curr_node.arity());
    } else {
      // Add terminal
      int terminal_choice = dist_terminalChoice(rng);
      if (terminal_choice == params.num_features) {
        // Add constant
        float val = dist_constVal(rng);
        curr_node = node(val);
      } else {
        // Add variable
        int fid = terminal_choice;
        curr_node = node(fid);
      }

      // Modify nodelist
      nodelist.push_back(curr_node);

      // Modify stack
      int e = arity_stack.top();
      arity_stack.pop();
      arity_stack.push(e - 1);
      while (arity_stack.top() == 0) {
        arity_stack.pop();
        if (arity_stack.empty()) {
          break;
        }

        e = arity_stack.top();
        arity_stack.pop();
        arity_stack.push(e - 1);
      }
    }
  }

  // Set new program parameters - need to do a copy as
  // nodelist will be deleted using RAII semantics
  p_out.nodes = new node[nodelist.size()];
  std::copy(nodelist.begin(), nodelist.end(), p_out.nodes);

  p_out.len = nodelist.size();
  p_out.metric = params.metric;
  p_out.raw_fitness_ = 0.0f;
}

void point_mutation(const program &prog, program &p_out, const param &params,
                    PhiloxEngine &rng) {
  // deep-copy program
  p_out = prog;

  // Specify RNGs
  uniform_real_distribution_custom<float> dist_uniform(0.0f, 1.0f);
  uniform_int_distribution_custom<int> dist_terminalChoice(0,
                                                           params.num_features);
  uniform_real_distribution_custom<float> dist_constantVal(
      params.const_range[0], params.const_range[1]);

  // Fill with uniform numbers
  std::vector<float> node_probs(p_out.len);
  std::generate(node_probs.begin(), node_probs.end(),
                [&dist_uniform, &rng] { return dist_uniform(rng); });

  // Mutate nodes
  int len = p_out.len;
  for (int i = 0; i < len; ++i) {
    node curr(prog.nodes[i]);

    if (node_probs[i] < params.p_point_replace) {
      if (curr.is_terminal()) {
        int choice = dist_terminalChoice(rng);

        if (choice == params.num_features) {
          // Add a randomly generated constant
          curr = node(dist_constantVal(rng));
        } else {
          // Add a variable with fid=choice
          curr = node(choice);
        }
      } else if (curr.is_nonterminal()) {
        // Replace current function with another function of the same arity
        int ar = curr.arity();
        // CUML_LOG_DEBUG("Arity is %d, curr function is
        // %d",ar,static_cast<std::underlying_type<node::type>::type>(curr.t));
        std::vector<node::type> fset = params.arity_set.at(ar);
        uniform_int_distribution_custom<int> dist_fset(0, fset.size() - 1);
        int choice = dist_fset(rng);
        curr = node(fset[choice]);
      }

      // Update p_out with updated value
      p_out.nodes[i] = curr;
    }
  }
}

void crossover(const program &prog, const program &donor, program &p_out,
               const param &params, PhiloxEngine &rng) {
  // Get a random subtree of prog to replace
  std::pair<int, int> prog_slice = get_subtree(prog.nodes, prog.len, rng);
  int prog_start = prog_slice.first;
  int prog_end = prog_slice.second;

  // Set metric of output program
  p_out.metric = prog.metric;

  // MAX_STACK_SIZE can only handle tree of depth MAX_STACK_SIZE -
  // max(func_arity=2) + 1 Thus we continuously hoist the donor subtree. Actual
  // indices in donor
  int donor_start = 0;
  int donor_end = donor.len;
  int output_depth = 0;
  int iter = 0;
  do {
    ++iter;
    // Get donor subtree
    std::pair<int, int> donor_slice =
        get_subtree(donor.nodes + donor_start, donor_end - donor_start, rng);

    // Get indices w.r.t current subspace [donor_start,donor_end)
    int donor_substart = donor_slice.first;
    int donor_subend = donor_slice.second;

    // Update relative indices to global indices
    donor_substart += donor_start;
    donor_subend += donor_start;

    // Update to new subspace
    donor_start = donor_substart;
    donor_end = donor_subend;

    // Evolve on current subspace
    p_out.len =
        (prog_start) + (donor_end - donor_start) + (prog.len - prog_end);
    delete[] p_out.nodes;
    p_out.nodes = new node[p_out.len];

    // Copy slices using std::copy
    std::copy(prog.nodes, prog.nodes + prog_start, p_out.nodes);
    std::copy(donor.nodes + donor_start, donor.nodes + donor_end,
              p_out.nodes + prog_start);
    std::copy(prog.nodes + prog_end, prog.nodes + prog.len,
              p_out.nodes + (prog_start) + (donor_end - donor_start));

    output_depth = get_depth(p_out);
  } while (output_depth >= MAX_STACK_SIZE);

  // Set the depth of the final program
  p_out.depth = output_depth;
}

void subtree_mutation(const program &prog, program &p_out, const param &params,
                      PhiloxEngine &rng) {
  // Generate a random program and perform crossover
  program new_program;
  build_program(new_program, params, rng);
  crossover(prog, new_program, p_out, params, rng);
}

void hoist_mutation(const program &prog, program &p_out, const param &params,
                    PhiloxEngine &rng) {
  // Replace program subtree with a random sub-subtree

  std::pair<int, int> prog_slice = get_subtree(prog.nodes, prog.len, rng);
  int prog_start = prog_slice.first;
  int prog_end = prog_slice.second;

  std::pair<int, int> sub_slice =
      get_subtree(prog.nodes + prog_start, prog_end - prog_start, rng);
  int sub_start = sub_slice.first;
  int sub_end = sub_slice.second;

  // Update subtree indices to global indices
  sub_start += prog_start;
  sub_end += prog_start;

  p_out.len = (prog_start) + (sub_end - sub_start) + (prog.len - prog_end);
  p_out.nodes = new node[p_out.len];
  p_out.metric = prog.metric;

  // Copy node slices using std::copy
  std::copy(prog.nodes, prog.nodes + prog_start, p_out.nodes);
  std::copy(prog.nodes + sub_start, prog.nodes + sub_end,
            p_out.nodes + prog_start);
  std::copy(prog.nodes + prog_end, prog.nodes + prog.len,
            p_out.nodes + (prog_start) + (sub_end - sub_start));

  // Update depth
  p_out.depth = get_depth(p_out);
}

} // namespace genetic
