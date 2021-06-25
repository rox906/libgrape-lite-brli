/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef EXAMPLES_ANALYTICAL_APPS_SMETRIC_SMETRIC_H_
#define EXAMPLES_ANALYTICAL_APPS_SMETRIC_SMETRIC_H_

#include <grape/grape.h>

#include "smetric/smetric_context.h"

namespace grape {

/**
 * @brief An implementation of SMetric, working on undirected graph.
 *
 * This version of SMetric inherits ParallelAppBase. Messages can be sent in
 * parallel to the evaluation. This strategy improve performance by overlapping
 * the communication time and the evaluation time.
 *
 * @tparam FRAG_T
 */
template <typename FRAG_T>
class SMetric : public ParallelAppBase<FRAG_T, SMetricContext<FRAG_T>>,
                public ParallelEngine {
 public:
  INSTALL_PARALLEL_WORKER(SMetric<FRAG_T>, SMetricContext<FRAG_T>, FRAG_T)
  using vertex_t = typename fragment_t::vertex_t;

  static constexpr bool need_split_edges = true;

  void PEval(const fragment_t& frag, context_t& ctx,
             message_manager_t& messages) {
    messages.InitChannels(thread_num());

    auto inner_vertices = frag.InnerVertices();
    auto outer_vertices = frag.OuterVertices();

#ifdef PROFILING
    ctx.exec_time -= GetCurrentTime();
#endif

    auto& channel = messages.Channels();

    ForEach(inner_vertices, [&ctx, &frag](int tid, vertex_t v) {
      ctx.deg[v] = frag.GetLocalOutDegree(v);
    });

    ForEach(inner_vertices, [&ctx, &frag, &channel](int tid, vertex_t v) {
      size_t t = 0;
      auto es = frag.GetOutgoingAdjList(v);
      for (auto& e : es) {
        auto u = e.get_neighbor();
        if (frag.IsInnerVertex(u))
          t += ctx.deg[u];
        else {
          channel[tid].SyncStateOnOuterVertex<fragment_t, size_t>(frag, u,
                                                               ctx.deg[v]);
        }
      }
      ctx.adjDegSum[v] = t;
    });

#ifdef PROFILING
    ctx.exec_time += GetCurrentTime();
    ctx.postprocess_time -= GetCurrentTime();
#endif

    messages.ForceContinue();

#ifdef PROFILING
    ctx.postprocess_time += GetCurrentTime();
#endif
  }

  void IncEval(const fragment_t& frag, context_t& ctx,
               message_manager_t& messages) {
    int thrd_num = thread_num();

#ifdef PROFILING
    ctx.preprocess_time -= GetCurrentTime();
#endif

    messages.ParallelProcess<fragment_t, size_t>(
        thrd_num, frag, [&ctx](int tid, vertex_t v, size_t deg) {
          atomic_add(ctx.adjDegSum[v], deg);
        });

    auto inner_vertices = frag.InnerVertices();
    ForEach(inner_vertices, [&ctx, &frag](int tid, vertex_t v) {
      atomic_add(ctx.s_metric, ctx.adjDegSum[v] * ctx.deg[v]);
    });

#ifdef PROFILING
    ctx.preprocess_time += GetCurrentTime();
    ctx.exec_time -= GetCurrentTime();
#endif

#ifdef PROFILING
    ctx.exec_time += GetCurrentTime();
    ctx.postprocess_time -= GetCurrentTime();
#endif

#ifdef PROFILING
    ctx.postprocess_time += GetCurrentTime();
#endif
  }
};

}  // namespace grape

#endif  // EXAMPLES_ANALYTICAL_APPS_SMETRIC_SMETRIC_H_
