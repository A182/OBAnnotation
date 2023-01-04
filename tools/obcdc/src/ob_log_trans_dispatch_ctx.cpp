/**
 * Copyright (c) 2022 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX OBLOG_DISPATCHER

#include "ob_log_trans_dispatch_ctx.h"
#include "ob_log_config.h"              // TCONF

namespace oceanbase
{
namespace liboblog
{

TransDispatchCtx::TransDispatchCtx() :
  trans_id_(),
  total_part_count_(0),
  dispatched_part_count_(0),
  normal_priority_part_budget_arr_(),
  high_priority_part_budget_arr_()
{}

int TransDispatchCtx::init(TransCtx &trans)
{
  int ret = OB_SUCCESS;
  reset();
  trans_id_ = trans.get_trans_id();
  total_part_count_ = trans.get_ready_participant_count();
  PartTransTask *part_trans_task = trans.get_participant_objs();

  while(OB_SUCC(ret) && OB_NOT_NULL(part_trans_task)) {
    PartTransTask *next_part_trans = part_trans_task->next_task();
    PartTransDispatchBudget budget;
    budget.reset(part_trans_task);
    if (OB_FAIL(normal_priority_part_budget_arr_.push_back(budget))) {
      LOG_ERROR("failed to push part dispatch budget into trans dispatch context", KR(ret), K(trans), K(budget), KPC(this));
    } else {
      part_trans_task = next_part_trans;
    }
  }

  return ret;
}

void TransDispatchCtx::reset()
{
  trans_id_.reset();
  total_part_count_ = 0;
  dispatched_part_count_ = 0;
  normal_priority_part_budget_arr_.reset();
  high_priority_part_budget_arr_.reset();
}

int TransDispatchCtx::reblance_budget(
    const int64_t redo_memory_limit,
    const int64_t current_used_memory,
    const TransCtx &trans)
{
  int ret = OB_SUCCESS;
  const int64_t total_budget = redo_memory_limit - current_used_memory;

  if (OB_UNLIKELY(!trans.get_trans_id().is_valid()
    || OB_UNLIKELY(trans.get_trans_id() != trans_id_))) {
    ret = OB_INVALID_ARGUMENT;
    LOG_ERROR("expected total_budget1for next round greater than 0 and valid trans_id", KR(ret), K(total_budget), K_(trans_id), K(trans));
  } else {
    if (is_trans_dispatched()) {
      ret = OB_ITER_END;
      LOG_DEBUG("no more part_trans to dispatch", KR(ret), K(trans));
    } else {
      const int64_t total_count = get_total_need_reblance_part_cnt_();
      const int64_t memory_limit_ratio_for_output_by_sql_operaiton =
          TCONF.redo_dispatched_memory_limit_exceed_ratio;

      if (total_count <= 0) {
        ret = OB_ERR_UNEXPECTED;
        LOG_ERROR("trans not dispatch finish but no more part to dispatch", KR(ret), K_(dispatched_part_count), K_(total_part_count));
      } else {
        // currenttly all part equal share budget, high_priority_part_budget_arr_ is not used
        // TODO: use sort feedback info to reblance budget!!

        // all part should dispatch at least one redo, so budget_value should + 1 in case of budget is 0
        int64_t dispatch_budget = total_budget > 0 ? total_budget : 0;
        const int64_t average_budget = current_used_memory >= redo_memory_limit * memory_limit_ratio_for_output_by_sql_operaiton ?
            0 : (dispatch_budget / total_count) + 1;
        set_normal_priority_budget_(average_budget);
        LOG_DEBUG("reblance budget result:", K(total_budget), K(total_count), K(average_budget));
      }
    }
  }

  return ret;
}

int64_t TransDispatchCtx::get_total_need_reblance_part_cnt_() const
{
  return normal_priority_part_budget_arr_.count() + high_priority_part_budget_arr_.count();
}

void TransDispatchCtx::set_normal_priority_budget_(const int64_t &average_budget)
{
  for(int64_t i = 0; i < normal_priority_part_budget_arr_.count(); i++) {
    PartTransDispatchBudget &budget = normal_priority_part_budget_arr_[i];
    budget.reset_budget(average_budget);
  }
}


int TransDispatchCtx::collect_sorter_feedback_()
{
  int ret = OB_SUCCESS;
  return ret;
}


} // namespace liboblog
} // namespace oceanbase
