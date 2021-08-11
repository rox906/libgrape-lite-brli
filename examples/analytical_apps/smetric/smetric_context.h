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

#ifndef EXAMPLES_ANALYTICAL_APPS_SMETRIC_SMETRIC_CONTEXT_H_
#define EXAMPLES_ANALYTICAL_APPS_SMETRIC_SMETRIC_CONTEXT_H_

#include <grape/grape.h>

#include <limits>

namespace grape {
/**
 * @brief Context for the parallel version of SMetric.
 *
 * @tparam FRAG_T
 */
template <typename FRAG_T>
class SMetricContext : public VertexDataContext<FRAG_T, grape::EmptyType> {
 public:
  using oid_t = typename FRAG_T::oid_t;
  using vid_t = typename FRAG_T::vid_t;

  explicit SMetricContext(const FRAG_T& fragment)
      : VertexDataContext<FRAG_T, grape::EmptyType>(fragment) {}

  void Init(ParallelMessageManager& messages) {
    auto& frag = this->fragment();
    auto vertices = frag.Vertices();

    deg.Init(vertices);
    adjDegSum.Init(vertices);

#ifdef PROFILING
    preprocess_time = 0;
    exec_time = 0;
    postprocess_time = 0;
#endif
  }

  void Output(std::ostream& os) override {
    os << s_metric << std::endl;

#ifdef PROFILING
    VLOG(2) << "preprocess_time: " << preprocess_time << "s.";
    VLOG(2) << "exec_time: " << exec_time << "s.";
    VLOG(2) << "postprocess_time: " << postprocess_time << "s.";
#endif
  }

  typename FRAG_T::template vertex_array_t<size_t> deg;
  typename FRAG_T::template vertex_array_t<size_t> adjDegSum;
  size_t s_metric = 0;
#ifdef PROFILING
  double preprocess_time = 0;
  double exec_time = 0;
  double postprocess_time = 0;
#endif
};
}  // namespace grape

#endif  // EXAMPLES_ANALYTICAL_APPS_SMETRIC_SMETRIC_CONTEXT_H_
