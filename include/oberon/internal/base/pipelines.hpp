/**
 * @file pipelines.hpp
 * @brief Internal shared graphics pipeline information.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_BASE_PIPELINES_HPP
#define OBERON_INTERNAL_BASE_PIPELINES_HPP

namespace oberon::internal::base {

  enum {
    TEST_IMAGE_PIPELINE_INDEX = 0,
    UNLIT_PC_PIPELINE_INDEX = 1,
    PIPELINE_COUNT
  };

}

#endif
