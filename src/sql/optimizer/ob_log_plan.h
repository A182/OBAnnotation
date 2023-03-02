/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OCEANBASE_SQL_OB_LOG_PLAN_H
#define OCEANBASE_SQL_OB_LOG_PLAN_H
#include "lib/allocator/page_arena.h"
#include "lib/string/ob_string.h"
#include "sql/ob_sql_context.h"
#include "sql/resolver/dml/ob_dml_stmt.h"
#include "sql/resolver/dml/ob_del_upd_stmt.h"
#include "sql/resolver/dml/ob_update_stmt.h"
#include "sql/resolver/dml/ob_delete_stmt.h"
#include "sql/resolver/dml/ob_insert_stmt.h"
#include "sql/resolver/dml/ob_insert_all_stmt.h"
#include "sql/resolver/ddl/ob_explain_stmt.h"
#include "sql/resolver/expr/ob_raw_expr_util.h"
#include "sql/optimizer/ob_optimizer_context.h"
#include "sql/optimizer/ob_opt_est_utils.h"
#include "sql/optimizer/ob_opt_selectivity.h"
#include "sql/optimizer/ob_log_operator_factory.h"
#include "sql/optimizer/ob_table_partition_info.h"
#include "sql/optimizer/ob_optimizer.h"
#include "share/client_feedback/ob_feedback_int_struct.h"
#include "sql/optimizer/ob_logical_operator.h"

namespace test
{
  class ObLogPlanTest_ob_explain_test_Test;
}

namespace oceanbase
{

namespace share
{
class ObServerLocality;
namespace schema
{
class ObSchemaGetterGuard;
}
}


namespace sql
{
class ObLogicalOperator;
class ObLogTableScan;
class ObLogDelUpd;
class AllocExchContext;
class ObJoinOrder;
class AccessPath;
class Path;
class JoinPath;
class SubQueryPath;
class FunctionTablePath;
class TempTablePath;
class CteTablePath;
class ObJoinOrder;
class ObOptimizerContext;
class ObLogJoin;
struct JoinInfo;
struct ConflictDetector;
struct FunctionTableDependInfo;
class ObLogSort;
class ObLogJoinFilter;
class ObLogSubPlanFilter;
struct ObAllocExprContext;
class ObLogTempTableInsert;
class ObLogTempTableAccess;
class ObLogTempTableTransformation;
class ObDelUpdStmt;
class ObExchangeInfo;
class ObDmlTableInfo;
struct IndexDMLInfo;

/**
 *  Explain plan text formatter
 */
struct plan_formatter
{
  plan_formatter() : column_name(), num_of_columns(0), column_width() {}
  virtual ~plan_formatter() {}
  // constants
  const static int64_t max_plan_column_width = 500;
  const static int64_t max_plan_column = 20;

  enum class LineType
  {
    LT_SPACE, // " "
    LT_LINE,  // "│"
    LT_NODE,  // "├"
    LT_LAST_NODE, // "└"
  };

  struct TreeLine
  {
    TreeLine() : color_idx_(0), line_type_(LineType::LT_SPACE) {}
    int32_t color_idx_;
    LineType line_type_;
  };

  struct NameLeftPadding
  {
    NameLeftPadding() : op_(NULL), level_(0), tree_line_(NULL)
    {
    }
    TO_STRING_KV(KP_(op), K_(level), KP_(tree_line));

    ObLogicalOperator *op_;
    int64_t level_;
    TreeLine *tree_line_;
  };

  int padding_name_left(int64_t op_id, char *buf, const int64_t buf_len, int64_t &pos);

  // members
  const char *column_name[max_plan_column];
  int64_t num_of_columns;
  int32_t column_width[max_plan_column_width];

  common::ObArenaAllocator alloc_;
  // Access with index: ObLogicalOperator::op_id_
  common::ObArray<NameLeftPadding> names_left_padding_;
};

// define operatory enum type
#define KEYS_DEF                                \
  KEY_DEF( Id, "ID" ),                        \
  KEY_DEF( Operator, "OPERATOR" ),            \
  KEY_DEF( Name, "NAME" ),                    \
  KEY_DEF( Est_Rows, "EST. ROWS" ),           \
  KEY_DEF( Cost, "COST" ),                    \
  KEY_DEF( Max_Plan_Column, "End" )
#define KEY_DEF( identifier, name )  identifier
  enum ExplainColumnEnumType { KEYS_DEF };
#undef KEY_DEF

// each line of the explain plan text
struct plan_line
{
  plan_line() : id(0), level(0), column_value() {}
  virtual ~plan_line() {}
  int id;
  int level;
  common::ObSEArray< common::ObString , 4, common::ModulePageAllocator, true> column_value;
};

struct FunctionTableDependInfo {
  TO_STRING_KV(
    K_(depend_table_set),
    K_(table_idx)
  );
  ObSqlBitSet<> depend_table_set_;  //function table expr所依赖的表
  int64_t table_idx_; //function table的bit index
};

// explain plan text
class planText
{
public:
  static const int64_t SIMPLE_COLUMN_NUM = 3;
  planText(char *buffer, const int64_t buffer_len, ExplainType type)
    : level(0), buf(buffer), buf_len(buffer_len),
      pos(0), formatter(), format(type), is_inited_(false), is_oneline_(false), outline_type_(OUTLINE_TYPE_UNINIT)
  {}
  virtual ~planText() {}
  int init();

  int level;
  char *buf;
  int64_t buf_len;
  int64_t pos;
  plan_formatter formatter;
  ExplainType format;
  bool is_inited_;
  bool is_oneline_;
  OutlineType outline_type_;
};
#undef KYES_DEF


struct SubPlanInfo
{
  SubPlanInfo() : init_expr_(NULL), subplan_(NULL), init_plan_(false) {}
  SubPlanInfo(ObQueryRefRawExpr *expr, ObLogPlan *plan, bool init_)
  : init_expr_(expr), subplan_(plan), init_plan_(init_) {}
  virtual ~SubPlanInfo() {}
  void set_subplan(ObLogPlan *plan) { subplan_ = plan; }

  ObQueryRefRawExpr *init_expr_;
  ObLogPlan *subplan_;
  bool init_plan_;
  TO_STRING_KV(K_(init_expr), K_(subplan), K_(init_plan));
};

struct ObDistinctAggrBatch
{
  // first: the origin aggr expr
  // second: the mocked aggr expr for three stage push down
  ObSEArray<std::pair<ObAggFunRawExpr *, ObAggFunRawExpr *>, 2,
  ModulePageAllocator, true> mocked_aggrs_;

  // first: the origin distint param expr
  // second: the mocked pseudo expr for the distinct param.
  ObSEArray<std::pair<ObRawExpr *, ObRawExpr *>, 1,
  ModulePageAllocator, true> mocked_params_;
  TO_STRING_KV(K(mocked_aggrs_), K(mocked_params_));
};


typedef common::ObSEArray<ObJoinOrder*, 4> JoinOrderArray;

/**
 *  Base class for logical plan for all DML/select statements
 */
class ObLogPlan
{
public:
  static const int64_t RELORDER_HASHBUCKET_SIZE = 256;
  static const int64_t JOINPATH_SET_HASHBUCKET_SIZE = 3000;
  friend class ::test::ObLogPlanTest_ob_explain_test_Test;

  typedef common::ObList<common::ObAddr, common::ObArenaAllocator> ObAddrList;

  static int select_replicas(ObExecContext &exec_ctx,
                             const common::ObIArray<const ObTableLocation*> &tbl_loc_list,
                             const common::ObAddr &local_server,
                             common::ObIArray<ObCandiTableLoc*> &phy_tbl_loc_info_list);
  static int select_replicas(ObExecContext &exec_ctx,
                             bool is_weak,
                             const common::ObAddr &local_server,
                             common::ObIArray<ObCandiTableLoc*> &phy_tbl_loc_info_list);

public:
  ObLogPlan(ObOptimizerContext &ctx, const ObDMLStmt *stmt);
  virtual ~ObLogPlan();

  /* return the sql text for the plan */
  inline common::ObString &get_sql_text() {return sql_text_;}
  // @brief Get the corresponding stmt
  inline virtual const ObDMLStmt *get_stmt() const { return stmt_; }

  inline int get_stmt_type(stmt::StmtType &stmt_type) const
  {
    int ret = common::OB_NOT_INIT;
    if (NULL != get_stmt()) {
      stmt_type = get_stmt()->get_stmt_type();
      ret = common::OB_SUCCESS;
    }
    return ret;
  }

  double get_optimization_cost();

  void set_query_ref(ObQueryRefRawExpr *query_ref) { query_ref_ = query_ref; }

  /*
   * plan root will be set after we generate raw plan (see function generate_raw_plan())
   * do not involve this function during generate_row_plan()
   */
  inline ObLogicalOperator *get_plan_root() const { return root_; }

  inline void set_plan_root(ObLogicalOperator *root)
  {
    root_ = root;
  }

  void set_max_op_id(uint64_t max_op_id) { max_op_id_ = max_op_id; }
  uint64_t get_max_op_id() const { return max_op_id_; }

  int make_candidate_plans(ObLogicalOperator *top);
  /**
   * @brief  Generate the plan tree
   * @param void
   * @retval OB_SUCCESS execute success
   * @retval OB_OPTIMIZE_GEN_PLAN_FALIED failed to generate the logical plan
   *
   * The function will be invoked by all DML/select statements and will handle
   * the 'common' part of those statements, including joins, order-by, limit and
   * etc.
   */
  virtual int generate_plan_tree();
  /**
   * Generate the "explain plan" string
   */
  int64_t to_string(char *buf,
                    const int64_t buf_len,
                    ExplainType type = EXPLAIN_TRADITIONAL,
                    const ObExplainDisplayOpt &opt = ObExplainDisplayOpt()) const;

  /**
   * Get optimizer context
   */
  ObOptimizerContext &get_optimizer_context() const { return optimizer_context_; }

  ObFdItemFactory &get_fd_item_factory() const { return optimizer_context_.get_fd_item_factory(); }

  virtual int generate_raw_plan() = 0;

  virtual int generate_plan();

  int do_post_plan_processing();

  int do_post_traverse_processing();

  int adjust_final_plan_info(ObLogicalOperator *&op);

  int update_re_est_cost(ObLogicalOperator *op);

  int collect_table_location(ObLogicalOperator *op);

  int collect_location_related_info(ObLogicalOperator &op);
  int build_location_related_tablet_ids();

  int gen_das_table_location_info(ObLogTableScan *table_scan,
                                  ObTablePartitionInfo *&table_partition_info);

  int check_das_dppr_filter_exprs(const ObIArray<ObRawExpr *> &input_filters,
                                  bool &has_dppr_filters);

  int set_duplicated_table_location(ObLogicalOperator *op,
                                    int64_t dup_table_pos);
  /**
   *  Get allocator used in sql compilation
   *
   *  The entire optimization process should use only this allocator.
   */
  common::ObIAllocator &get_allocator() const { return allocator_; }
  /**
   *  Get the logical operator allocator
   */
  virtual ObLogOperatorFactory &get_log_op_factory() { return log_op_factory_; }
  /**
   *  List all needed plan traversals
   */
  template <typename ...TS>
  int plan_traverse_loop(TS ...args);
  const common::ObIArray<std::pair<ObRawExpr *, ObRawExpr *>> & get_group_replaced_exprs() const
  { return group_replaced_exprs_; }
  int set_group_replaced_exprs(common::ObIArray<std::pair<ObRawExpr *, ObRawExpr *> > &exprs)
  { return group_replaced_exprs_.assign(exprs); }

  common::ObIArray<int64_t> &get_multi_stmt_rowkey_pos()
  {
    return multi_stmt_rowkey_pos_;
  }

  const common::ObIArray<int64_t> &get_multi_stmt_rowkey_pos() const
  {
    return multi_stmt_rowkey_pos_;
  }
  int add_global_table_partition_info(ObTablePartitionInfo *addr_table_partition_info);

  int get_global_table_partition_info(common::ObIArray<ObTablePartitionInfo *> &info) const
  {
    return append(info, optimizer_context_.get_table_partition_info());
  }

  int remove_duplicate_constraint(ObLocationConstraintContext &location_constraint,
                                  ObSqlCtx &sql_ctx) const;
  int remove_duplicate_base_table_constraint(ObLocationConstraintContext &location_constraint) const;
  int remove_duplicate_pwj_constraint(ObIArray<ObPwjConstraint *> &pwj_constraints) const;
  int replace_pwj_constraints(ObIArray<ObPwjConstraint *> &constraints,
                              const int64_t from,
                              const int64_t to) const;
  int sort_pwj_constraint(ObLocationConstraintContext &location_constraint) const;

  int get_from_table_items(const ObIArray<FromItem> &from_items,
                          ObIArray<TableItem *> &table_items);

  int get_current_semi_infos(const ObIArray<SemiInfo*> &semi_infos,
                             const ObIArray<TableItem*> &table_items,
                             ObIArray<SemiInfo*> &current_semi_infos);

  int get_table_ids(TableItem *table_item,
                    ObRelIds& table_ids);

  int get_table_ids(const ObIArray<uint64_t> &table_ids,
                    ObRelIds& rel_ids);

  int get_table_ids(const ObIArray<TableItem*> &table_items,
                    ObRelIds& table_ids);

  int get_table_ids(const ObIArray<ObRawExpr*> &exprs,
                    ObRelIds& table_ids);

  int is_joined_table_filter(const ObIArray<TableItem*> &table_items,
                             const ObRawExpr *expr,
                             bool &is_filter);

  int find_inner_conflict_detector(const ObIArray<ConflictDetector*> &inner_conflict_detectors,
                                  const ObRelIds &rel_ids,
                                  ConflictDetector* &detector);

  int satisfy_associativity_rule(const ConflictDetector &left,
                                const ConflictDetector &right,
                                bool &is_satisfy);

  int satisfy_left_asscom_rule(const ConflictDetector &left,
                              const ConflictDetector &right,
                              bool &is_satisfy);

  int satisfy_right_asscom_rule(const ConflictDetector &left,
                                const ConflictDetector &right,
                                bool &is_satisfy);

  int add_conflict_rule(const ObRelIds &left,
                        const ObRelIds &right,
                        ObIArray<std::pair<ObRelIds, ObRelIds>> &rules);

  int generate_conflict_rule(ConflictDetector *left,
                             ConflictDetector *right,
                             bool is_left_child,
                             ObIArray<std::pair<ObRelIds, ObRelIds>> &rules);

  inline ObIArray<ConflictDetector*>& get_conflict_detectors() { return conflict_detectors_; }
  inline const ObIArray<ConflictDetector*>& get_conflict_detectors() const { return conflict_detectors_; }

  int get_base_table_items(const ObDMLStmt &stmt,
                           const ObIArray<TableItem*> &table_items,
                           const ObIArray<SemiInfo*> &semi_infos,
                           ObIArray<TableItem*> &base_tables);

  int generate_base_level_join_order(const common::ObIArray<TableItem*> &table_items,
                                     common::ObIArray<ObJoinOrder*> &base_level);

  int prepare_ordermap_pathset(const JoinOrderArray base_level);

  int select_location(ObIArray<ObTablePartitionInfo *> &tbl_part_info_list);

  int get_subplan(const ObRawExpr *expr, SubPlanInfo *&info);
  int get_subplan(const ObStmt *stmt, SubPlanInfo *&info);

  /**
   *  Get plan signature (hash value)
   */
  uint64_t get_signature() const { return hash_value_; }

  common::ObIArray<ObExprSelPair> &get_predicate_selectivities()
  { return pred_sels_; }

  inline common::ObIArray<ObRawExpr *> &get_onetime_exprs()
  { return onetime_exprs_; }

  int print_outline_oneline(char *buf, int64_t buf_len, int64_t &pos) const;
  int print_outline_table(planText &plan_text, const TableItem *table_item) const;
  //get expr selectivity from predicate_selectivities_
  double get_expr_selectivity(const ObRawExpr *expr, bool &found);

  int can_be_late_materialization(bool &can_be);

  int check_late_materialization_project(const uint64_t table_id,
                                         const common::ObIArray<ObRawExpr*> &filter_exprs,
                                         const common::ObIArray<ObRawExpr*> &sort_exprs,
                                         const common::ObIArray<ObRawExpr*> &index_keys,
                                         bool &need,
                                         common::ObIArray<ObRawExpr*> &pre_access_columns,
                                         common::ObIArray<ObRawExpr*> &project_columns);

  int get_pre_project_cost(ObLogicalOperator *top,
                           ObLogicalOperator *scan,
                           common::ObIArray<ObRawExpr*> &index_columns,
                           bool index_back,
                           bool need_set,
                           double &cost);

  int check_late_materialization_cost(ObLogicalOperator *top,
                                      ObLogicalOperator *scan,
                                      common::ObIArray<ObRawExpr*> &index_columns,
                                      common::ObIArray<ObRawExpr*> &table_columns,
                                      bool index_back,
                                      double min_cost,
                                      bool &need,
                                      double &get_cost,
                                      double &join_cost);

  static int select_one_server(const common::ObAddr &selected_server,
                               common::ObIArray<ObCandiTableLoc*> &phy_tbl_loc_info_list);

  int is_partition_in_same_server(const ObIArray<const ObCandiTableLoc *> &phy_location_infos,
                                  bool &is_same,
                                  bool &multi_part_table,
                                  common::ObAddr &first_addr);

  int check_enable_plan_expiration(bool &enable) const;
  bool need_consistent_read() const;

  int check_need_multi_partition_dml(const ObDMLStmt &stmt,
                                     ObLogicalOperator &top,
                                     const ObIArray<IndexDMLInfo *> &index_dml_infos,
                                     bool &is_multi_part_dml,
                                     bool &is_result_local);

  int check_stmt_need_multi_partition_dml(const ObDMLStmt &stmt,
                                          const ObIArray<IndexDMLInfo *> &index_dml_infos,
                                          bool &is_multi_part_dml);
  int check_location_need_multi_partition_dml(ObLogicalOperator &top,
                                              uint64_t table_id,
                                              bool &is_multi_part_dml,
                                              bool &is_result_local);

  int get_source_table_info(ObLogicalOperator &child_op,
                               uint64_t source_table_id,
                               ObShardingInfo *&sharding_info,
                               ObTablePartitionInfo *&table_part_info);

  void set_insert_stmt(const ObInsertStmt *insert_stmt) { insert_stmt_ = insert_stmt; }
  const ObInsertStmt *get_insert_stmt() const { return insert_stmt_; }
public:
  struct CandidatePlan
  {
    CandidatePlan(ObLogicalOperator *plan_tree)
    : plan_tree_(plan_tree)
    { }
    CandidatePlan()
    : plan_tree_(NULL)
    { }
    virtual ~CandidatePlan() {}
    void reset()
    {
      plan_tree_ = NULL;
    }
    ObLogicalOperator *plan_tree_;

    int64_t to_string(char *buf, const int64_t buf_len) const
    {
      UNUSED(buf);
      UNUSED(buf_len);
      return common::OB_SUCCESS;
    }
  };

  struct All_Candidate_Plans
  {
    All_Candidate_Plans() : is_final_sort_(false) {}
    virtual ~All_Candidate_Plans() {}
    common::ObSEArray<CandidatePlan, 8, common::ModulePageAllocator, true> candidate_plans_;
    std::pair<double, int64_t> plain_plan_;
    bool is_final_sort_;
    void reuse()
    {
      candidate_plans_.reuse();
      is_final_sort_ = false;
    }
    int get_best_plan(ObLogicalOperator *&best_plan)
    {
      int ret = common::OB_SUCCESS;
      best_plan = NULL;
      if (plain_plan_.second >= 0 && plain_plan_.second < candidate_plans_.count()) {
        best_plan = candidate_plans_.at(plain_plan_.second).plan_tree_;
      } else {
        ret = common::OB_ERR_UNEXPECTED;
      }
      return ret;
    }
  };

  struct GroupingOpHelper
  {
    GroupingOpHelper() :
      can_scalar_pushdown_(false),
      can_basic_pushdown_(false),
      can_three_stage_pushdown_(false),
      can_rollup_pushdown_(false),
      force_use_hash_(false),
      force_use_merge_(false),
      is_scalar_group_by_(false),
      distinct_exprs_(),
      aggr_code_expr_(NULL),
      non_distinct_aggr_items_(),
      distinct_aggr_items_(),
      distinct_params_(),
      rollup_id_expr_(NULL),
      group_ndv_(-1.0),
      group_distinct_ndv_(-1.0)
    {
    }
    virtual ~GroupingOpHelper() {}

    bool can_scalar_pushdown_;
    bool can_basic_pushdown_;
    bool can_three_stage_pushdown_;
    bool can_rollup_pushdown_;
    bool force_use_hash_; // has use_hash_aggregation/use_hash_distinct hint
    bool force_use_merge_; // has no_use_hash_aggregation/no_use_hash_distinct hint
    bool is_scalar_group_by_;
    ObSEArray<ObRawExpr*, 8> distinct_exprs_;

    // context for three stage group by push down
    ObRawExpr *aggr_code_expr_;
    ObArray<ObAggFunRawExpr *> non_distinct_aggr_items_;
    ObArray<ObAggFunRawExpr *> distinct_aggr_items_;
    ObArray<ObRawExpr *> distinct_params_;
    ObArray<ObDistinctAggrBatch> distinct_aggr_batch_;

    // for rollup distributor and collector
    ObRawExpr *rollup_id_expr_;

    // distinct of group expr
    double group_ndv_;
    // distinct of group expr and distinct expr
    double group_distinct_ndv_;

    TO_STRING_KV(K_(can_scalar_pushdown),
                 K_(can_basic_pushdown),
                 K_(can_three_stage_pushdown),
                 K_(can_rollup_pushdown),
                 K_(force_use_hash),
                 K_(force_use_merge),
                 K_(is_scalar_group_by),
                 K_(distinct_exprs));
  };

  /**
   * @brief Genearete a specified operator on top of a list of candidate plans
   * @param [out] jos - the generated Join_OrderS
   * @retval OB_SUCCESS execute success
   * @retval OB_SOME_ERROR special errno need to handle
   */
  int generate_join_orders();

  /** @brief Allcoate operator for access path */
  int allocate_access_path(AccessPath *ap,
                           ObLogicalOperator *&out_access_path_op);

  int allocate_cte_table_path(CteTablePath *cte_table_path,
                              ObLogicalOperator *&out_access_path_op);

  int allocate_temp_table_path(TempTablePath *temp_table_path,
                               ObLogicalOperator *&out_access_path_op);

  int allocate_function_table_path(FunctionTablePath *func_table_path,
                                   ObLogicalOperator *&out_access_path_op);

  /*allocate table lookup operator*/
  int allocate_table_lookup(AccessPath &ap,
                            common::ObString &table_name,
                            common::ObString &index_name,
                            ObLogicalOperator *child_node,
                            common::ObIArray<ObRawExpr*> &filter_exprs,
                            ObLogicalOperator *&output_op);

  int get_global_index_filters(const ObIArray<ObRawExpr*> &filter_exprs,
                               const ObIArray<uint64_t> &index_columns,
                               common::ObIArray<ObRawExpr*> &global_index_filters,
                               common::ObIArray<ObRawExpr*> &remaining_filters);

  //store index column ids including storing column.
  int store_index_column_ids(ObSqlSchemaGuard &schema_guard,
                             ObLogTableScan &scan,
                             const int64_t table_id,
                             const int64_t index_id);

  /** @brief Allcoate operator for join path */
  int allocate_join_path(JoinPath *join_path,
                         ObLogicalOperator *&out_join_path_op);

  int compute_join_exchange_info(JoinPath &join_path,
                                 ObExchangeInfo &left_exch_info,
                                 ObExchangeInfo &right_exch_info,
                                 bool left_is_non_preserve_side,
                                 bool right_is_non_preserve_side);

  int get_join_path_keys(const JoinPath &join_path,
                         ObIArray<ObRawExpr*> &left_keys,
                         ObIArray<ObRawExpr*> &right_keys,
                         ObIArray<bool> &null_safe_info);

  int compute_hash_distribution_info(const ObIArray<ObRawExpr*> &join_exprs,
                                     const ObRelIds &left_table_set,
                                     ObExchangeInfo &left_exch_info,
                                     ObExchangeInfo &right_exch_info);

  void compute_null_distribution_info(const ObJoinType &join_type,
                                      ObExchangeInfo &left_exch_info,
                                      ObExchangeInfo &right_exch_info,
                                      ObIArray<bool> &null_safe_info);

  int compute_repartition_distribution_info(const EqualSets &equal_sets,
                                            const ObIArray<ObRawExpr*> &src_keys,
                                            const ObIArray<ObRawExpr*> &target_keys,
                                            const uint64_t ref_table_id,
                                            const uint64_t table_id,
                                            const ObString &table_name,
                                            const ObShardingInfo &target_sharding,
                                            ObExchangeInfo &exch_info);

  int compute_repartition_distribution_info(const EqualSets &equal_sets,
                                            const ObIArray<ObRawExpr*> &src_keys,
                                            const ObIArray<ObRawExpr*> &target_keys,
                                            const Path &target_path,
                                            ObExchangeInfo &exch_info);

  int compute_repartition_distribution_info(const EqualSets &equal_sets,
                                            const ObIArray<ObRawExpr*> &src_keys,
                                            const ObIArray<ObRawExpr*> &target_keys,
                                            const ObLogicalOperator &target_op,
                                            ObExchangeInfo &exch_info);

  int compute_subplan_filter_repartition_distribution_info(ObLogicalOperator *top,
                                                           const ObIArray<ObLogicalOperator*> &subquery_ops,
                                                           const ObIArray<ObExecParamRawExpr *> &params,
                                                           ObExchangeInfo &exch_info);

  int get_repartition_table_info(const ObLogicalOperator &op,
                                 ObString &table_name,
                                 uint64_t &ref_table_id,
                                 uint64_t &table_id);

  int compute_repartition_func_info(const EqualSets &equal_sets,
                                    const ObIArray<ObRawExpr *> &src_keys,
                                    const ObIArray<ObRawExpr *> &target_keys,
                                    const ObShardingInfo &target_sharding,
                                    ObRawExprFactory &expr_factory,
                                    ObExchangeInfo &exch_info);

  int get_repartition_keys(const EqualSets &equal_sets,
                           const ObIArray<ObRawExpr*> &src_keys,
                           const ObIArray<ObRawExpr*> &target_keys,
                           const ObIArray<ObRawExpr*> &target_part_keys,
                           ObIArray<ObRawExpr *> &src_part_keys);

  /** @brief Allcoate operator for subquery path */
  int allocate_subquery_path(SubQueryPath *subpath,
                             ObLogicalOperator *&out_subquery_op);

  /** @brief Allcoate a ,aterial operator as parent of a path */
  int allocate_material_as_top(ObLogicalOperator *&old_top);

  /** @brief Create plan tree from an interesting order */
  int create_plan_tree_from_path(Path *path,
                                 ObLogicalOperator *&out_plan_tree);

  /** @brief Initialize the candidate plans from join order */
  int init_candidate_plans();

  int init_candidate_plans(ObIArray<CandidatePlan> &candi_plans);

  int candi_allocate_scala_group_by(const ObIArray<ObAggFunRawExpr*> &agg_items);

  int candi_allocate_scala_group_by(const ObIArray<ObAggFunRawExpr*> &agg_items,
                                    const ObIArray<ObRawExpr*> &having_exprs,
                                    const bool is_from_povit);

  int prepare_three_stage_info(const ObIArray<ObRawExpr *> &group_by_exprs,
                               const ObIArray<ObRawExpr *> &rollup_exprs,
                               GroupingOpHelper &helper);

  int generate_three_stage_aggr_expr(ObRawExprFactory &expr_factory,
                                     ObSQLSessionInfo &session_info,
                                     const bool is_rollup,
                                     ObAggFunRawExpr *aggr,
                                     ObIArray<ObDistinctAggrBatch> &distinct_aggr_batch,
                                     ObIArray<ObRawExpr *> &distinct_params);

  bool disable_hash_groupby_in_second_stage();
  int create_three_stage_group_plan(const ObIArray<ObRawExpr*> &group_by_exprs,
                                    const ObIArray<ObRawExpr *> &rollup_exprs,
                                    const ObIArray<ObRawExpr*> &having_exprs,
                                    GroupingOpHelper &helper,
                                    ObLogicalOperator *&top);

  int perform_group_by_pushdown(ObLogicalOperator *op);
  int perform_simplify_win_expr(ObLogicalOperator *op);
  int simplify_win_expr(ObLogicalOperator* child_op, ObWinFunRawExpr &win_expr);
  int simplify_win_partition_exprs(ObLogicalOperator* child_op,
                                   ObWinFunRawExpr &win_expr);
  int simplify_win_order_items(ObLogicalOperator* child_op,
                               ObWinFunRawExpr &win_expr);

  int try_to_generate_pullup_aggr(ObAggFunRawExpr *old_aggr,
                                  ObAggFunRawExpr *&new_aggr);

  int create_scala_group_plan(const ObIArray<ObAggFunRawExpr*> &agg_items,
                              const ObIArray<ObRawExpr*> &having_exprs,
                              const bool is_from_povit,
                              GroupingOpHelper &groupby_helper,
                              ObLogicalOperator *&top);

  int check_can_pullup_gi(ObLogicalOperator &top,
                          bool is_partition_wise,
                          bool need_sort,
                          bool &can_pullup);

  int try_push_aggr_into_table_scan(ObLogicalOperator *top,
                                    const ObIArray<ObAggFunRawExpr *> &aggr_items);

  int get_grouping_style_exchange_info(const common::ObIArray<ObRawExpr*> &partition_exprs,
                                       const EqualSets &equal_sets,
                                       ObExchangeInfo &exch_info);

  int init_groupby_helper(const ObIArray<ObRawExpr*> &groupby_exprs,
                          const ObIArray<ObRawExpr*> &rollup_exprs,
                          const ObIArray<ObAggFunRawExpr*> &aggr_items,
                          const bool is_from_povit,
                          GroupingOpHelper &groupby_helper);

  int init_distinct_helper(const ObIArray<ObRawExpr*> &distinct_exprs,
                           GroupingOpHelper &distinct_helper);

  bool is_tenant_enable_aggr_push_down(ObSQLSessionInfo &session_info);

  int check_scalar_groupby_pushdown(const ObIArray<ObAggFunRawExpr *> &aggrs,
                                    bool &can_push);

  int check_basic_groupby_pushdown(const ObIArray<ObAggFunRawExpr*> &aggr_items,
                                   const EqualSets &equal_sets,
                                   bool &push_group);

  int check_three_stage_groupby_pushdown(const ObIArray<ObRawExpr*> &rollup_exprs,
                                         const ObIArray<ObAggFunRawExpr *> &aggr_items,
                                         ObIArray<ObAggFunRawExpr *> &non_distinct_aggrs,
                                         ObIArray<ObAggFunRawExpr *> &distinct_aggrs,
                                         const EqualSets &equal_sets,
                                         ObIArray<ObRawExpr *> &distinct_exprs,
                                         bool &can_push);

  int check_rollup_pushdown(const ObSQLSessionInfo *info,
                            const ObIArray<ObAggFunRawExpr *> &aggr_items,
                            bool &can_push);

  int adjust_sort_expr_ordering(ObIArray<ObRawExpr*> &sort_exprs,
                                ObIArray<ObOrderDirection> &sort_directions,
                                ObLogicalOperator &child_op,
                                bool check_win_func);

  int adjust_exprs_by_win_func(ObIArray<ObRawExpr *> &exprs,
                               const ObWinFunRawExpr &win_expr,
                               const EqualSets &equal_sets,
                               const ObIArray<ObRawExpr*> &conditions,
                               ObIArray<ObOrderDirection> &directions);

  int adjust_postfix_sort_expr_ordering(const ObIArray<OrderItem> &ordering,
                                        const ObFdItemSet &fd_item_set,
                                        const EqualSets &equal_sets,
                                        const ObIArray<ObRawExpr*> &const_exprs,
                                        const int64_t prefix_count,
                                        ObIArray<ObRawExpr*> &sort_exprs,
                                        ObIArray<ObOrderDirection> &sort_directions);

  int get_minimal_cost_candidates(const ObIArray<CandidatePlan> &candidates,
                                  ObIArray<CandidatePlan> &best_candidates);

  int get_minimal_cost_candidates(const ObIArray<ObSEArray<CandidatePlan, 16>> &candidate_list,
                                  ObIArray<CandidatePlan> &best_candidates);

  int get_minimal_cost_candidate(const ObIArray<CandidatePlan> &candidates,
                                 CandidatePlan &best_candidate);

  int classify_candidates_based_on_sharding(const ObIArray<CandidatePlan> &candidates,
                                            ObIArray<ObSEArray<CandidatePlan, 16>> &candidate_list);

  /** @brief Allocate ORDER BY on top of plan candidates */
  int candi_allocate_order_by(bool &need_limit, ObIArray<OrderItem> &order_items);

  int get_order_by_topn_expr(int64_t input_card,
                             ObRawExpr *&topn_expr,
                             bool &is_fetch_with_ties,
                             bool &need_limit);

  /** @brief Get order by columns */
  int get_order_by_exprs(const ObLogicalOperator *top,
                         common::ObIArray<ObRawExpr *> &order_by_exprs,
                         common::ObIArray<ObOrderDirection> *directions = NULL);

  // @brief Make OrderItems by exprs
  int make_order_items(const common::ObIArray<ObRawExpr *> &exprs,
                       const common::ObIArray<ObOrderDirection> *dirs,
                       common::ObIArray<OrderItem> &items);

  int make_order_items(const common::ObIArray<ObRawExpr *> &exprs, common::ObIArray<OrderItem> &items);

  int make_order_items(const common::ObIArray<ObRawExpr *> &exprs,
                       const common::ObIArray<ObOrderDirection> &dirs,
                       common::ObIArray<OrderItem> &items);

  int create_order_by_plan(ObLogicalOperator *&top,
                           const ObIArray<OrderItem> &order_items,
                           ObRawExpr *topn_expr,
                           bool is_fetch_with_ties);

  int allocate_sort_and_exchange_as_top(ObLogicalOperator *&top,
                                        ObExchangeInfo &exch_info,
                                        const ObIArray<OrderItem> &sort_keys,
                                        const bool need_sort,
                                        const int64_t prefix_pos,
                                        const bool is_partition_wise,
                                        ObRawExpr *topn_expr = NULL,
                                        bool is_fetch_with_ties = false,
                                        const int64_t part_cnt = 0);

  int allocate_dist_range_sort_as_top(ObLogicalOperator *&top,
                                      const ObIArray<OrderItem> &sort_keys,
                                      const bool need_sort,
                                      const bool is_local_order);

  int try_allocate_sort_as_top(ObLogicalOperator *&top,
                               const ObIArray<OrderItem> &sort_keys,
                               const bool need_sort,
                               const int64_t prefix_pos,
                               const int64_t part_cnt = 0);

  int allocate_sort_as_top(ObLogicalOperator *&top,
                           const ObIArray<OrderItem> &sort_keys,
                           const int64_t prefix_pos = 0,
                           const bool is_local_merge_sort = false,
                           ObRawExpr *topn_expr = NULL,
                           bool is_fetch_with_ties = false,
                           const int64_t part_cnt = 0);

  int allocate_exchange_as_top(ObLogicalOperator *&top,
                               const ObExchangeInfo &exch_info);

  int allocate_stat_collector_as_top(ObLogicalOperator *&top,
                                     ObStatCollectorType stat_type,
                                     const ObIArray<OrderItem> &sort_keys,
                                     share::schema::ObPartitionLevel part_level);

  int allocate_scala_group_by_as_top(ObLogicalOperator *&top,
                                     const ObIArray<ObAggFunRawExpr*> &agg_items,
                                     const ObIArray<ObRawExpr*> &having_exprs,
                                     const bool from_pivot,
                                     const double origin_child_card);

  int allocate_group_by_as_top(ObLogicalOperator *&top,
                               const AggregateAlgo algo,
                               const ObIArray<ObRawExpr*> &group_by_exprs,
                               const ObIArray<ObRawExpr*> &rollup_exprs,
                               const ObIArray<ObAggFunRawExpr*> &agg_items,
                               const ObIArray<ObRawExpr*> &having_exprs,
                               const bool from_pivot,
                               const double total_ndv,
                               const double origin_child_card,
                               const bool is_partition_wise = false,
                               const bool is_push_down = false,
                               const bool is_partition_gi = false,
                               const ObRollupStatus rollup_status = ObRollupStatus::NONE_ROLLUP);

  int candi_allocate_limit(const ObIArray<OrderItem> &order_items);

  /** @brief Allocate LIMIT on top of plan candidates */
  int candi_allocate_limit(ObRawExpr *limit_expr,
                           ObRawExpr *offset_expr = NULL,
                           ObRawExpr *percent_expr = NULL,
                           const bool is_calc_found_rows = false,
                           const bool is_top_limit = false,
                           const bool is_fetch_with_ties = false,
                           const ObIArray<OrderItem> *ties_ordering = NULL);

  int create_limit_plan(ObLogicalOperator *&old_top,
                        ObRawExpr *limit_expr,
                        ObRawExpr *pushed_expr,
                        ObRawExpr *offset_expr,
                        ObRawExpr *percent_expr,
                        const bool is_calc_found_rows,
                        const bool is_top_limit,
                        const bool is_fetch_with_ties,
                        const ObIArray<OrderItem> *ties_ordering);

   int try_push_limit_into_table_scan(ObLogicalOperator *top,
                                      ObRawExpr *limit_expr,
                                      ObRawExpr *pushed_expr,
                                      ObRawExpr *offset_expr,
                                      bool &is_pushed);

   int allocate_limit_as_top(ObLogicalOperator *&old_top,
                             ObRawExpr *limit_expr,
                             ObRawExpr *offset_expr,
                             ObRawExpr *percent_expr,
                             const bool is_calc_found_rows,
                             const bool is_top_limit,
                             const bool is_fetch_with_ties,
                             const ObIArray<OrderItem> *ties_order_item = NULL);

  int is_plan_reliable(const ObLogicalOperator *root,
                       bool &is_reliable);

  /** @brief Allocate sequence op on top of plan candidates */
  int candi_allocate_sequence();
  int allocate_sequence_as_top(ObLogicalOperator *&old_top);

  int candi_allocate_err_log(const ObDelUpdStmt *stmt);
  int allocate_err_log_as_top(const ObDelUpdStmt *stmt, ObLogicalOperator *&old_top);

  /** @brief Allocate SELECTINTO on top of plan candidates */
  int candi_allocate_select_into();
  /** @brief allocate select into as new top(parent)**/

  int allocate_select_into_as_top(ObLogicalOperator *&old_top);

  int allocate_expr_values_as_top(ObLogicalOperator *&old_top,
                                  const ObIArray<ObRawExpr*> *filter_exprs = NULL);

  int allocate_values_as_top(ObLogicalOperator *&old_top);

  int allocate_temp_table_insert_as_top(ObLogicalOperator *&top,
                                        const ObSqlTempTableInfo *temp_table_info);

  int candi_allocate_temp_table_transformation();

  int create_temp_table_transformation_plan(ObLogicalOperator *&top,
                                            const ObIArray<ObLogicalOperator*> &temp_table_insert);

  int check_basic_sharding_for_temp_table(ObLogicalOperator *&top,
                                          const ObIArray<ObLogicalOperator*> &temp_table_insert,
                                          bool &is_basic);

  int allocate_temp_table_transformation_as_top(ObLogicalOperator *&top,
                                                const ObIArray<ObLogicalOperator*> &temp_table_insert);

  int candi_allocate_root_exchange();

  /**
   *  Plan tree traversing(both top-down and bottom-up)
   */
  int plan_tree_traverse(const TraverseOp &operation, void *ctx);

  int inner_plan_tree_traverse(const TraverseOp &operation, void *ctx, AllocGIContext &gi_ctx, ObLocationConstraintContext &location_constraints);

  inline void set_signature(uint64_t hash_value) { hash_value_ = hash_value; }

  int print_outline(planText &plan, bool is_hints = false) const;

  int print_qb_name_trace(planText &plan) const;

  int candi_allocate_subplan_filter_for_where();

  int candi_allocate_subplan_filter_for_exprs(ObIArray<ObRawExpr*> &exprs);

  int candi_allocate_subplan_filter(const ObIArray<ObRawExpr *> &subquery_exprs,
                                    const ObIArray<ObRawExpr *> *filters = NULL,
                                    const bool is_update_set = false,
                                    const bool for_on_condition = false);

  int inner_candi_allocate_subplan_filter(ObIArray<ObLogPlan*> &subplans,
                                          ObIArray<ObQueryRefRawExpr *> &query_refs,
                                          ObIArray<ObExecParamRawExpr *> &params,
                                          ObIArray<ObExecParamRawExpr *> &onetime_exprs,
                                          ObBitSet<> &initplan_idxs,
                                          ObBitSet<> &onetime_idxs,
                                          const ObIArray<ObRawExpr *> *filters,
                                          const bool or_cursor_expr,
                                          const bool is_update_set);

  int generate_subplan_filter_info(const ObIArray<ObRawExpr*> &subquery_exprs,
                                   ObIArray<ObLogPlan*> &subquery_ops,
                                   ObIArray<ObQueryRefRawExpr *> &query_refs,
                                   ObIArray<ObExecParamRawExpr *> &exec_params,
                                   ObIArray<ObExecParamRawExpr *> &onetime_exprs,
                                   ObBitSet<> &initplan_idxs,
                                   ObBitSet<> &onetime_idxs,
                                   bool &for_cursor_expr,
                                   bool for_on_condition);

  int create_subplan_filter_plan(ObLogicalOperator *&top,
                                 const ObIArray<ObLogicalOperator*> &subquery_ops,
                                 const ObIArray<ObLogicalOperator*> &dist_subquery_ops,
                                 const ObIArray<ObQueryRefRawExpr *> &query_refs,
                                 const ObIArray<ObExecParamRawExpr *> &params,
                                 const ObIArray<ObExecParamRawExpr *> &onetime_exprs,
                                 const ObBitSet<> &initplan_idxs,
                                 const ObBitSet<> &onetime_idxs,
                                 const bool for_cursor_expr,
                                 const ObIArray<ObRawExpr*> *filters,
                                 const bool is_update_set);

  int check_contains_recursive_cte(ObIArray<ObLogicalOperator*> &child_ops,
                                   bool &is_recursive_cte);

  int init_subplan_filter_child_ops(const ObIArray<ObLogicalOperator*> &subquery_ops,
                                    const ObIArray<std::pair<int64_t, ObRawExpr*>> &params,
                                    ObIArray<ObLogicalOperator*> &dist_subquery_ops);

  int check_if_subplan_filter_match_partition_wise(ObLogicalOperator *top,
                                                   const ObIArray<ObLogicalOperator*> &subquery_ops,
                                                   const ObIArray<ObExecParamRawExpr *> &params,
                                                   bool &is_partition_wise);

  int check_if_subplan_filter_match_repart(ObLogicalOperator *top,
                                          const ObIArray<ObLogicalOperator*> &subquery_ops,
                                          const ObIArray<ObExecParamRawExpr *> &params,
                                          bool &is_match_repart);

  int check_if_match_none_all(ObLogicalOperator *top,
                              const ObIArray<ObLogicalOperator*> &subquery_ops,
                              bool &is_none_all);

  int get_subplan_filter_equal_keys(ObLogicalOperator *child,
                                    const ObIArray<ObExecParamRawExpr *> &params,
                                    ObIArray<ObRawExpr *> &left_keys,
                                    ObIArray<ObRawExpr *> &right_keys,
                                    ObIArray<bool> &null_safe_info);

  int get_subplan_filter_normal_equal_keys(const ObLogicalOperator *child,
                                           ObIArray<ObRawExpr *> &left_keys,
                                           ObIArray<ObRawExpr *> &right_keys,
                                           ObIArray<bool> &null_safe_info);

  int get_subplan_filter_correlated_equal_keys(const ObLogicalOperator *op,
                                               const ObIArray<ObExecParamRawExpr *> &params,
                                               ObIArray<ObRawExpr *> &left_keys,
                                               ObIArray<ObRawExpr *> &right_keys,
                                               ObIArray<bool> &null_safe_info);

  int allocate_subplan_filter_as_top(ObLogicalOperator *&top,
                                     const ObIArray<ObLogicalOperator*> &subquery_ops,
                                     const ObIArray<ObQueryRefRawExpr *> &query_ref_exprs,
                                     const ObIArray<ObExecParamRawExpr *> &exec_params,
                                     const ObIArray<ObExecParamRawExpr *> &onetime_exprs,
                                     const ObBitSet<> &initplan_idxs,
                                     const ObBitSet<> &onetime_idxs,
                                     const ObIArray<ObRawExpr*> *filters,
                                     const DistAlgo dist_algo,
                                     const bool is_update_set);
  int allocate_subplan_filter_as_top(ObLogicalOperator *&old_top,
                                     const common::ObIArray<ObRawExpr*> &subquery_exprs,
                                     const bool is_filter = false,
                                     const bool for_on_condition = false);

  int allocate_subplan_filter_for_on_condition(ObIArray<ObRawExpr*> &subquery_exprs, ObLogicalOperator* &top);

  int candi_allocate_filter(const ObIArray<ObRawExpr*> &filter_exprs);

  int candi_allocate_count();
  int classify_rownum_exprs(const common::ObIArray<ObRawExpr*> &rownum_exprs,
                            common::ObIArray<ObRawExpr*> &filter_exprs,
                            common::ObIArray<ObRawExpr*> &start_exprs,
                            ObRawExpr *&limit_expr);
  int classify_rownum_expr(const ObItemType expr_type,
                           ObRawExpr *rownum_expr,
                           ObRawExpr *const_expr,
                           common::ObIArray<ObRawExpr*> &filter_exprs,
                           common::ObIArray<ObRawExpr*> &start_exprs,
                           ObRawExpr *&limit_expr);
  int create_rownum_plan(ObLogicalOperator *&old_top,
                         const common::ObIArray<ObRawExpr*> &filter_exprs,
                         const common::ObIArray<ObRawExpr*> &start_exprs,
                         ObRawExpr *limit_expr);
  int allocate_count_as_top(ObLogicalOperator *&old_top,
                            const common::ObIArray<ObRawExpr*> &filter_exprs,
                            const common::ObIArray<ObRawExpr*> &start_exprs,
                            ObRawExpr *limit_expr);

  All_Candidate_Plans &get_candidate_plans() { return candidates_; }

  const ObRawExprSets &get_empty_expr_sets() { return empty_expr_sets_; }
  const ObFdItemSet &get_empty_fd_item_set() { return empty_fd_item_set_; }
  const ObRelIds &get_empty_table_set() { return empty_table_set_; }
  ObRelIds &get_subq_pdfilter_tset() { return subq_pushdown_filter_table_set_; }
  inline common::ObIArray<ObRawExpr *> &get_subquery_filters()
  {
    return subquery_filters_;
  }
  int init_plan_info();
  int collect_subq_pushdown_filter_table_relids(const ObIArray<ObRawExpr*> &quals);
  EqualSets &get_equal_sets() { return equal_sets_; }
  const EqualSets &get_equal_sets() const { return equal_sets_; }
  // 获取log plan中所有在执行期需要使用到的表达式
  int set_all_exprs(const ObAllocExprContext &ctx);

  EqualSets* create_equal_sets();
  ObJoinOrder* create_join_order(PathType type);

  const common::ObIArray<SubPlanInfo*> &get_subplans() const  { return subplan_infos_; }
  inline OptTableMetas& get_basic_table_metas() { return basic_table_metas_; }
  inline const OptTableMetas& get_basic_table_metas() const { return basic_table_metas_; }
  inline OptTableMetas& get_update_table_metas() { return update_table_metas_; }
  inline const OptTableMetas& get_update_table_metas() const { return update_table_metas_; }
  inline OptSelectivityCtx& get_selectivity_ctx() { return selectivity_ctx_; }
  inline const OptSelectivityCtx& get_selectivity_ctx() const { return selectivity_ctx_; }
  inline bool get_is_subplan_scan() const { return is_subplan_scan_; }
  inline void set_is_subplan_scan(bool is_subplan_scan) { is_subplan_scan_ = is_subplan_scan; }
  inline ObSqlTempTableInfo* get_temp_table_info() const { return temp_table_info_; }
  inline bool is_temp_table() const { return NULL != temp_table_info_; }
  inline void set_temp_table_info(ObSqlTempTableInfo *temp_table_info)
  {
    temp_table_info_ = temp_table_info;
  }
  inline bool is_final_root_plan() const {
    const ObDMLStmt *root_stmt = optimizer_context_.get_root_stmt();
    return  OB_NOT_NULL(root_stmt) &&
            ((root_stmt->is_explain_stmt() &&
              static_cast<const ObExplainStmt*>(root_stmt)->get_explain_query_stmt() == stmt_)
             || stmt_ == root_stmt);
  }
  inline common::ObIArray<ObRawExpr *> &get_const_exprs()
  {
    return const_exprs_;
  }
  inline const common::ObIArray<ObRawExpr *> &get_const_exprs() const
  {
    return const_exprs_;
  }

  inline int add_pushdown_filters(const common::ObIArray<ObRawExpr *> &pushdown_filters)
  {
    return append(pushdown_filters_, pushdown_filters);
  }

  inline common::ObIArray<ObRawExpr *> &get_pushdown_filters()
  {
    return pushdown_filters_;
  }
  inline const common::ObIArray<ObRawExpr *> &get_pushdown_filters() const
  {
    return pushdown_filters_;
  }

  int extract_onetime_exprs(ObRawExpr *expr,
                            ObIArray<ObExecParamRawExpr *> &onetime_exprs,
                            ObIArray<ObQueryRefRawExpr *> &onetime_query_refs,
                            const bool for_on_condition);

  int contains_startup_with_exec_param(ObLogicalOperator *op,
                                       bool &contains);

  int contains_limit_or_pushdown_limit(ObLogicalOperator *op,
                                       bool &contains);

  int check_and_reset_batch_nlj(ObLogicalOperator *root);

  /**
   * 递归处理expr里的SubQuery，遇到SubLink就生成一个SubPlan
   * @param expr
   * @return
   */
  int generate_subplan(ObRawExpr *&expr);

  int add_startup_filters(common::ObIArray<ObRawExpr *> &exprs)
  {
    return append(startup_filters_, exprs);
  }

  inline common::ObIArray<ObRawExpr *> &get_startup_filters()
  {
    return startup_filters_;
  }

  /**
   * width estimation related info
   */
  inline common::ObIArray<ObRawExpr *> &get_select_item_exprs_for_width_est() { return select_item_exprs_; }
  inline common::ObIArray<ObRawExpr *> &get_condition_exprs_for_width_est() { return condition_exprs_; }
  inline common::ObIArray<ObRawExpr *> &get_groupby_rollup_exprs_for_width_est() { return groupby_rollup_exprs_; }
  inline common::ObIArray<ObRawExpr *> &get_having_exprs_for_width_est() { return having_exprs_; }
  inline common::ObIArray<ObRawExpr *> &get_orderby_exprs_for_width_est() { return orderby_exprs_; }
  inline common::ObIArray<ObRawExpr *> &get_winfunc_exprs_for_width_est() { return winfunc_exprs_; }

  inline ObIArray<ObShardingInfo*> &get_hash_dist_info() { return hash_dist_info_; }
  int get_cached_hash_sharding_info(const ObIArray<ObRawExpr*> &hash_exprs,
                                    const EqualSets &equal_sets,
                                    ObShardingInfo *&cached_sharding);

  inline const common::ObIArray<FunctionTableDependInfo> &get_function_table_depend_infos() const
  {
    return function_table_depend_infos_;
  }

  int allocate_output_expr_for_values_op(ObLogicalOperator &values_op);

  inline common::ObIArray<JoinPath*> &get_recycled_join_paths()
  {
    return recycled_join_paths_;
  }

  int get_rowkey_exprs(const uint64_t table_id,
                       const uint64_t ref_table_id,
                       ObIArray<ObRawExpr*> &keys);

  int get_rowkey_exprs(const uint64_t table_id,
                       const ObTableSchema &table_schema,
                       ObIArray<ObRawExpr*> &keys);

  int get_index_column_items(ObRawExprFactory &expr_factory,
                              uint64_t table_id,
                              const share::schema::ObTableSchema &index_table_schema,
                              common::ObIArray<ColumnItem> &index_columns);
  inline common::ObIArray<ColumnItem> &get_column_items() { return column_items_; }
  int generate_column_expr(ObRawExprFactory &expr_factory,
                           const uint64_t &table_id,
                           const ObColumnSchemaV2 &column_schema,
                           ColumnItem &column_item);

  common::ObIArray<int64_t> &get_alloc_sfu_list() { return alloc_sfu_list_; }

  int merge_same_sfu_table_list(uint64_t target_id,
                                int64_t begin_idx,
                                ObIArray<uint64_t> &src_table_list,
                                ObIArray<uint64_t> &res_table_list);

  /**
   * @brief candi_allocate_for_update
   * allocate for update operator
   * @return
   */
  int candi_allocate_for_update();

   /** @brief Allcoate a ,for update operator as parent of a path */
  int allocate_for_update_as_top(ObLogicalOperator *&top, ObIArray<uint64_t> &sfu_table_list);

  int get_table_for_update_info(const uint64_t table_id,
                                IndexDMLInfo *&index_dml_info,
                                int64_t &wait_ts,
                                bool &skip_locked);

  int get_part_column_exprs(const uint64_t table_id,
                            const uint64_t ref_table_id,
                            common::ObIArray<ObRawExpr*> &part_exprs) const;

  int create_for_update_plan(ObLogicalOperator *&top,
                             const ObIArray<IndexDMLInfo *> &index_dml_infos,
                             int64_t wait_ts,
                             bool skip_locked,
                             ObRawExpr *lock_rownum);

  int allocate_for_update_as_top(ObLogicalOperator *&top,
                                 const bool is_multi_part_dml,
                                 const ObIArray<IndexDMLInfo *> &index_dml_infos,
                                 int64_t wait_ts,
                                 bool skip_locked,
                                 ObRawExpr *lock_rownum);
  bool table_is_allocated_for_update(const int64_t table_id);
  virtual int add_extra_dependency_table() const;

  int gen_calc_part_id_expr(uint64_t table_id,
                            uint64_t ref_table_id,
                            CalcPartIdType calc_id_type,
                            ObRawExpr *&expr);

  int candi_allocate_for_update_material();

  int allocate_material_for_recursive_cte_plan(ObIArray<ObLogicalOperator*> &child_ops);

protected:
  ObColumnRefRawExpr *get_column_expr_by_id(uint64_t table_id, uint64_t column_id) const;
  const ColumnItem *get_column_item_by_id(uint64_t table_id, uint64_t column_id) const;
  int update_plans_interesting_order_info(ObIArray<CandidatePlan> &candidate_plans,
                                          const int64_t check_scope);

  int prune_and_keep_best_plans(ObIArray<CandidatePlan> &candidate_plans);

  int add_candidate_plan(common::ObIArray<CandidatePlan> &current_plans,
                         const CandidatePlan &new_plan);

  int compute_plan_relationship(const CandidatePlan &first_plan,
                                const CandidatePlan &second_plan,
                                DominateRelation &relation);

  int compute_pipeline_relationship(const ObLogicalOperator &first_plan,
                                    const ObLogicalOperator &second_plan,
                                    DominateRelation &relation);

  int get_start_with_filter(const ObDMLStmt *stmt,
                            common::ObIArray<ObRawExpr *> *&start_with_filters);

  int distribute_start_with_filters_to_rels(common::ObIArray<ObJoinOrder *> &baserels,
                                            common::ObIArray<ObRawExpr*> &start_with_filters);

  int distribute_quals_to_rels(const common::ObIArray<TableItem*> &table_items,
                               const common::ObIArray<SemiInfo*> &semi_infos,
                               common::ObIArray<ObJoinOrder *> &baserels,
                               common::ObIArray<ObRawExpr*> &quals);

  int try_split_or_qual(const ObRelIds &table_ids,
                        ObOpRawExpr &or_qual,
                        ObIArray<ObRawExpr*> &table_quals);

  int split_or_quals(const ObIArray<TableItem*> &table_items,
                    ObIArray<ObRawExpr*> &quals);

  int split_or_quals(const ObIArray<TableItem*> &table_items,
                    ObRawExpr *qual,
                    ObIArray<ObRawExpr*> &new_quals);

  int pre_process_quals(const ObIArray<TableItem*> &table_items,
                      const ObIArray<SemiInfo*> &semi_infos,
                      ObIArray<ObRawExpr*> &quals);

  int pre_process_quals(SemiInfo* semi_info);

  int pre_process_quals(TableItem *table_item);

  int generate_conflict_detectors(const ObIArray<TableItem*> &table_items,
                                  const ObIArray<SemiInfo*> &semi_infos,
                                  ObIArray<ObRawExpr*> &quals,
                                  ObIArray<ObJoinOrder *> &baserels);

  int generate_semi_join_detectors(const ObIArray<SemiInfo*> &semi_infos,
                                  ObRelIds &left_rel_ids,
                                  const ObIArray<ConflictDetector*> &inner_join_detectors,
                                  ObIArray<ConflictDetector*> &semi_join_detectors);

  int generate_inner_join_detectors(const ObIArray<TableItem*> &table_items,
                                    ObIArray<ObRawExpr*> &quals,
                                    ObIArray<ObJoinOrder *> &baserels,
                                    ObIArray<ConflictDetector*> &inner_join_detectors);

  int generate_outer_join_detectors(TableItem *table_item,
                                    ObIArray<ObRawExpr*> &table_filter,
                                    ObIArray<ObJoinOrder *> &baserels,
                                    ObIArray<ConflictDetector*> &outer_join_detectors);

  int mock_base_rel_detectors(ObJoinOrder *&base_rel);

  int distribute_quals(TableItem *table_item,
                      const ObIArray<ObRawExpr*> &table_filter,
                      ObIArray<ObJoinOrder *> &baserels);

  int flatten_inner_join(TableItem *table_item,
                        ObIArray<ObRawExpr*> &table_filter,
                        ObIArray<TableItem*> &table_items);

  int inner_generate_outer_join_detectors(JoinedTable *joined_table,
                                          ObIArray<ObRawExpr*> &table_filter,
                                          ObIArray<ObJoinOrder *> &baserels,
                                          ObIArray<ConflictDetector*> &outer_join_detectors);

  int pushdown_where_filters(JoinedTable* joined_table,
                            ObIArray<ObRawExpr*> &table_filter,
                            ObIArray<ObRawExpr*> &left_quals,
                            ObIArray<ObRawExpr*> &right_quals);

  int pushdown_on_conditions(JoinedTable* joined_table,
                            ObIArray<ObRawExpr*> &left_quals,
                            ObIArray<ObRawExpr*> &right_quals,
                            ObIArray<ObRawExpr*> &join_quals);

  int generate_cross_product_detector(const ObIArray<TableItem*> &table_items,
                                      ObIArray<ObRawExpr*> &quals,
                                      ObIArray<ConflictDetector*> &cross_product_detectors);

  int check_join_info(const ObRelIds &left,
                      const ObRelIds &right,
                      const ObIArray<ObRelIds> &base_table_ids,
                      bool &is_connected);

  int generate_cross_product_conflict_rule(ConflictDetector *cross_product_detector,
                                          const ObIArray<TableItem*> &table_items,
                                          const ObIArray<ObRawExpr*> &join_conditions);

  int init_leading_info_from_joined_tables(TableItem *table);

  int init_leading_info_from_tables(const ObIArray<TableItem*> &table_items,
                                   const ObIArray<SemiInfo*> &semi_infos);

  int find_join_order_pair(uint8_t beg_pos,
                          uint8_t &end_pos,
                          uint8_t ignore_beg_pos,
                          uint8_t &ignore_end_pos,
                          bool &found);

  int get_table_ids_from_leading(uint8_t pos, ObRelIds& table_ids);

  int init_leading_info_from_leading_pair(uint8_t beg_pos,
                                        uint8_t end_pos,
                                        ObRelIds &table_set);

  int init_leading_info_from_leading();

  int init_leading_info(const ObIArray<TableItem*> &table_items,
                        const ObIArray<SemiInfo*> &semi_infos);

  int init_bushy_tree_info(const ObIArray<TableItem*> &table_items);

  int init_bushy_tree_info_from_joined_tables(TableItem *table);

  int init_function_table_depend_info(const ObIArray<TableItem*> &table_items);

  int check_need_bushy_tree(common::ObIArray<JoinOrderArray> &join_rels,
                            const int64_t join_level,
                            bool &need);

  int init_width_estimation_info(const ObDMLStmt *stmt);

  int init_idp(int64_t initial_idp_step,
               common::ObIArray<JoinOrderArray> &idp_join_rels,
               common::ObIArray<JoinOrderArray> &full_join_rels);

  int generate_join_levels_with_IDP(common::ObIArray<JoinOrderArray> &join_rels);

  int inner_generate_join_levels_with_IDP(common::ObIArray<JoinOrderArray> &join_rels,
                                          bool ignore_hint);

  int generate_join_levels_with_orgleading(common::ObIArray<JoinOrderArray> &join_rels);

  int do_one_round_idp(common::ObIArray<JoinOrderArray> &temp_join_rels,
                      uint32_t curr_idp_step,
                      bool ignore_hint,
                      uint32_t &outer_base_level,
                      ObIDPAbortType &abort_type);

  int process_join_level_info(const ObIArray<TableItem*> &table_items,
                              ObIArray<JoinOrderArray> &join_rels,
                              ObIArray<JoinOrderArray> &new_join_rels);

  int generate_join_order_with_table_tree(ObIArray<JoinOrderArray> &join_rels,
                                          TableItem *table,
                                          ObJoinOrder* &join_tree);

  int generate_single_join_level_with_DP(ObIArray<JoinOrderArray> &join_rels,
                                         uint32_t left_level,
                                         uint32_t right_level,
                                         uint32_t level,
                                         bool ignore_hint,
                                         ObIDPAbortType &abort_type);

  int inner_generate_join_order(ObIArray<JoinOrderArray> &join_rels,
                                ObJoinOrder *left_tree,
                                ObJoinOrder *right_tree,
                                uint32_t level,
                                bool force_order,
                                bool delay_cross_product,
                                bool &is_valid_join,
                                ObJoinOrder *&join_tree);

  int is_detector_used(ObJoinOrder *left_tree,
                      ObJoinOrder *right_tree,
                      ConflictDetector *detector,
                      bool &is_used);

  int choose_join_info(ObJoinOrder *left_tree,
                      ObJoinOrder *right_tree,
                      ObIArray<ConflictDetector*> &valid_detectors,
                      bool delay_cross_product,
                      bool &is_strict_order);

  int check_join_info(const ObIArray<ConflictDetector*> &valid_detectors,
                      ObJoinType &join_type,
                      bool &is_valid);

  int merge_join_info(ObJoinOrder *left_tree,
                      ObJoinOrder *right_tree,
                      const ObIArray<ConflictDetector*> &valid_detectors,
                      JoinInfo &join_info);

  int remove_redundancy_pred(ObJoinOrder *left_tree,
                            ObJoinOrder *right_tree,
                            JoinInfo &join_info);

  int inner_remove_redundancy_pred(EqualSets &input_equal_sets,
                                   ObIArray<ObRawExpr*> &join_pred);

  int sort_qual_by_selectivity(ObIArray<ObRawExpr*> &join_pred);

  int generate_subplan_for_query_ref(ObQueryRefRawExpr *query_ref,
                                     SubPlanInfo *&subplan_info);

  int greedy_idp_best_order(uint32_t current_level,
                            common::ObIArray<JoinOrderArray> &idp_join_rels,
                            ObJoinOrder *&best_order);

  int prepare_next_round_idp(common::ObIArray<JoinOrderArray> &idp_join_rels,
                             uint32_t initial_idp_step,
                             ObJoinOrder *&best_order);

  int check_and_abort_curr_level_dp(common::ObIArray<JoinOrderArray> &idp_join_rels,
                                    uint32_t curr_level,
                                    ObIDPAbortType &abort_type);

  int check_and_abort_curr_round_idp(common::ObIArray<JoinOrderArray> &idp_join_rels,
                                     uint32_t curr_level,
                                     ObIDPAbortType &abort_type);
  /**
   * SubPlanInfo相关接口
   * @return
   */
  inline common::ObIArray<SubPlanInfo*> &get_subplans()
  {
    return subplan_infos_;
  }

  inline int64_t get_subplan_size()
  {
    return subplan_infos_.count();
  }
  int add_subplan(SubPlanInfo *plan)
  {
    return subplan_infos_.push_back(plan);
  }

  int add_subquery_filter(ObRawExpr *qual);

  bool is_correlated_expr(const ObStmt *stmt, const ObRawExpr *expr, const int32_t curlevel);

  int add_rownum_expr(ObRawExpr *expr)
  {
    return rownum_exprs_.push_back(expr);
  }

  inline common::ObIArray<ObRawExpr *> &get_rownum_exprs()
  {
    return rownum_exprs_;
  }

  int add_special_expr(ObRawExpr *expr)
  {
    return special_filters_.push_back(expr);
  }

  inline common::ObIArray<ObRawExpr *> &get_special_exprs()
  {
    return special_filters_;
  }

  int add_startup_filter(ObRawExpr *expr)
  {
    return startup_filters_.push_back(expr);
  }

  /**
   * 处理scalar_in，转成or形式
   * @param expr
   * @return
   */
  int process_scalar_in(ObRawExpr *&expr);

  /**
   * 根据表的id找到基表（其实这里不一定是基表，而是fromitem里的基本对象，可以是用户写的OJ，也可能是个SubQueryScan）
   * @param base_level
   * @param table_id
   * @return
   */
  int find_base_rel(common::ObIArray<ObJoinOrder *> &base_level, int64_t table_idx, ObJoinOrder *&base_rel);

  /**
   * 根据relids找到一个joinrel
   * @param join_level
   * @param relids
   * @return
   */
  int find_join_rel(ObRelIds &relids, ObJoinOrder *&join_rel);

  int check_need_gen_join_path(const ObJoinOrder *left_tree,
                               const ObJoinOrder *right_tree,
                               bool &need_gen);

  int check_join_legal(const ObRelIds &left_set,
                       const ObRelIds &right_set,
                       const ObRelIds &combined_set,
                       ConflictDetector *detector,
                       bool delay_cross_product,
                       bool &legal);

  int check_join_hint(const ObRelIds &left_set,
                      const ObRelIds &right_set,
                      bool &match_hint,
                      bool &is_legal,
                      bool &is_strict_order);

  // 用于计算 px 场景下 select、update 等语句需要的线程数
  int calc_plan_resource();

  int get_cache_calc_part_id_expr(int64_t table_id, int64_t ref_table_id,
      CalcPartIdType calc_type, ObRawExpr* &expr);

  int get_part_exprs(uint64_t table_id,
                    uint64_t ref_table_id,
                    share::schema::ObPartitionLevel &part_level,
                    ObRawExpr *&part_expr,
                    ObRawExpr *&subpart_expr);

private: // member functions
  static int strong_select_replicas(const common::ObAddr &local_server,
                                    common::ObIArray<ObCandiTableLoc*> &phy_tbl_loc_info_list,
                                    bool &is_hit_partition,
                                    bool sess_in_retry);
  static int weak_select_replicas(const common::ObAddr &local_server,
                                  ObRoutePolicyType route_type,
                                  bool proxy_priority_hit_support,
                                  common::ObIArray<ObCandiTableLoc*> &phy_tbl_loc_info_list,
                                  bool &is_hit_partition,
                                  share::ObFollowerFirstFeedbackType &follower_first_feedback);
  static int calc_hit_partition_for_compat(const common::ObIArray<ObCandiTableLoc*> &phy_tbl_loc_info_list,
                                           const common::ObAddr &local_server,
                                           bool &is_hit_partition,
                                           ObAddrList &intersect_servers);
  static int calc_follower_first_feedback(const common::ObIArray<ObCandiTableLoc*> &phy_tbl_loc_info_list,
                                          const common::ObAddr &local_server,
                                          const ObAddrList &intersect_servers,
                                          share::ObFollowerFirstFeedbackType &follower_first_feedback);

  inline bool is_upper_stmt_column_ref(const ObRawExpr &qual, const ObDMLStmt &stmt) const;
  int set_connect_by_property(JoinPath *join_path, ObLogJoin &log_join);
  static int calc_intersect_servers(const ObIArray<ObCandiTableLoc*> &phy_tbl_loc_info_list,
                                    ObList<ObAddr, ObArenaAllocator> &candidate_server_list);
  int calc_and_set_exec_pwj_map(ObLocationConstraintContext &location_constraint) const;

  int64_t check_pwj_cons(const ObPwjConstraint &pwj_cons,
                         const common::ObIArray<LocationConstraint> &base_location_cons,
                         ObIArray<PwjTable> &pwj_tables,
                         ObPwjComparer &pwj_comparer,
                         PWJTabletIdMap &pwj_map) const;
  bool has_depend_function_table(const ObRelIds& table_ids);
public:
  const ObLogPlanHint &get_log_plan_hint() { return log_plan_hint_; }
  bool has_join_order_hint() { return !log_plan_hint_.join_order_.leading_tables_.is_empty(); }
  const ObRelIds& get_leading_tables() { return log_plan_hint_.join_order_.leading_tables_; }
  void set_added_leading() { outline_print_flags_ |= ADDED_LEADING_HINT; }
  void reset_outline_print_flags() { outline_print_flags_ = 0; }
  bool has_added_leading() const { return outline_print_flags_ & ADDED_LEADING_HINT; }
private:
  static const int64_t IDP_PATHNUM_THRESHOLD = 5000;
protected: // member variable
  ObOptimizerContext &optimizer_context_;
  common::ObIAllocator &allocator_;
  const ObDMLStmt *stmt_;
  ObLogOperatorFactory log_op_factory_;
  All_Candidate_Plans candidates_;
  common::ObSEArray<std::pair<ObRawExpr *, ObRawExpr *>, 4, common::ModulePageAllocator, true > group_replaced_exprs_;
  //上层stmt条件下推下来的谓词，已经抽出？
  common::ObSEArray<ObRawExpr *, 4, common::ModulePageAllocator, true> pushdown_filters_;
  common::ObSEArray<ObRawExpr*, 16, common::ModulePageAllocator, true> startup_filters_;

private: // member variable
  //如果这个plan是一个子查询，并且出现在expr的位置上，这个指针指向引用这个plan的的表达式
  ObQueryRefRawExpr *query_ref_;
  ObLogicalOperator *root_;                    // root operator
  common::ObString sql_text_;                     // SQL string
  uint64_t hash_value_;                       // plan signature
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> subquery_filters_;
  common::ObSEArray<SubPlanInfo*, 4, common::ModulePageAllocator, true> subplan_infos_;
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> rownum_exprs_;
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> special_filters_; //filter at root join order
  // for width estimation
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> select_item_exprs_;
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> condition_exprs_;
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> groupby_rollup_exprs_;
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> having_exprs_;
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> orderby_exprs_;
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> winfunc_exprs_;
private:
  struct JoinPathPairInfo
  {
    JoinPathPairInfo()
    : left_ids_(),
      right_ids_() {}
    virtual ~JoinPathPairInfo() = default;
    uint64_t hash() const
    {
      return left_ids_.hash() + right_ids_.hash();;
    }
    bool operator ==(const JoinPathPairInfo &src_info) const
    {
      return (left_ids_ == src_info.left_ids_) && (right_ids_ == src_info.right_ids_);
    }
    TO_STRING_KV(K_(left_ids), K_(right_ids));
    ObRelIds left_ids_;
    ObRelIds right_ids_;
  };
  typedef common::ObPooledAllocator<common::hash::HashMapTypes<ObRelIds, ObJoinOrder *>::AllocType,
                                    common::ObWrapperAllocator> IdOrderMapAllocer;
  typedef common::ObPooledAllocator<common::hash::HashSetTypes<JoinPathPairInfo>::AllocType,
                                    common::ObWrapperAllocator> JoinPathSetAllocer;
  typedef common::hash::ObHashMap<ObRelIds,
                                  ObJoinOrder *,
                                  common::hash::NoPthreadDefendMode,
                                  common::hash::hash_func<ObRelIds>,
                                  common::hash::equal_to<ObRelIds>,
                                  IdOrderMapAllocer,
                                  common::hash::NormalPointer,
                                  common::ObWrapperAllocator,
                                  2> IdOrderMap;
  typedef common::hash::ObHashSet<JoinPathPairInfo,
                                  common::hash::NoPthreadDefendMode,
                                  common::hash::hash_func<JoinPathPairInfo>,
                                  common::hash::equal_to<JoinPathPairInfo>,
                                  JoinPathSetAllocer,
                                  common::hash::NormalPointer,
                                  common::ObWrapperAllocator,
                                  2> JoinPathSet;

  ObLogPlanHint log_plan_hint_;
  enum OUTLINE_PRINT_FLAG {
    ADDED_LEADING_HINT        = 1 << 0
  };
  uint64_t outline_print_flags_; // used print outline
  common::ObSEArray<ObRelIds, 8, common::ModulePageAllocator, true> bushy_tree_infos_;
  common::ObSEArray<ObRawExpr *, 8, common::ModulePageAllocator, true> onetime_exprs_; // allocated onetime exprs
  common::ObSEArray<FunctionTableDependInfo, 8, common::ModulePageAllocator, true> function_table_depend_infos_;
  common::ObSEArray<ConflictDetector*, 8, common::ModulePageAllocator, true> conflict_detectors_;
  ObJoinOrder *join_order_;
  IdOrderMapAllocer id_order_map_allocer_;
  common::ObWrapperAllocator bucket_allocator_wrapper_;
  IdOrderMap relid_joinorder_map_;
  JoinPathSetAllocer join_path_set_allocer_;
  JoinPathSet join_path_set_;
  common::ObSEArray<JoinPath*, 1024, common::ModulePageAllocator, true> recycled_join_paths_;
  common::ObSEArray<ObExprSelPair, 16, common::ModulePageAllocator, true> pred_sels_;
  common::ObSEArray<int64_t, 4, common::ModulePageAllocator, true> multi_stmt_rowkey_pos_;

  // used as default equal sets/ unique sets for ObLogicalOperator
  const ObRawExprSets empty_expr_sets_;
  const ObRelIds empty_table_set_;
  EqualSets equal_sets_;  // non strict equal sets for stmt_;
  const ObFdItemSet empty_fd_item_set_;
  // save the maxinum of the logical operator id
  uint64_t max_op_id_;
  bool is_subplan_scan_;  // 当前plan是否是一个subplan scan
  ObSqlTempTableInfo *temp_table_info_; // current plan is a temp table
  // 从where condition中抽出的常量表达式
  common::ObSEArray<ObRawExpr*, 4, common::ModulePageAllocator, true> const_exprs_;
  common::ObSEArray<ObShardingInfo*, 8, common::ModulePageAllocator, true> hash_dist_info_;
  //
  common::ObSEArray<ColumnItem, 8, common::ModulePageAllocator, true> column_items_;
  // add only for error_logging
  const ObInsertStmt *insert_stmt_;
  // all basic table meta before base table predicate
  OptTableMetas basic_table_metas_;
  // all basic table meta after base table predicate
  OptTableMetas update_table_metas_;
  OptSelectivityCtx selectivity_ctx_;
  ObRelIds subq_pushdown_filter_table_set_;
  // have been allocated for update table list
  common::ObSEArray<int64_t, 1, common::ModulePageAllocator, true> alloc_sfu_list_;
  struct PartIdExpr {
    int64_t table_id_;
    int64_t ref_table_id_;
    ObRawExpr *calc_part_id_expr_;
    CalcPartIdType calc_type_;
    TO_STRING_KV(
      K_(table_id),
      K_(ref_table_id),
      K_(calc_part_id_expr),
      K_(calc_type)
    );
  };
  common::ObSEArray<PartIdExpr, 8, common::ModulePageAllocator, true> cache_part_id_exprs_;
  DISALLOW_COPY_AND_ASSIGN(ObLogPlan);
};

inline bool ObLogPlan::is_upper_stmt_column_ref(const ObRawExpr &qual, const ObDMLStmt &stmt) const
{
  return qual.is_column_ref_expr() && qual.get_expr_level() < stmt.get_current_level();
}

template <typename ...TS>
int ObLogPlan::plan_traverse_loop(TS ...args)
{
  int ret = common::OB_SUCCESS;
  TraverseOp ops[] = { args... };
  for (int64_t i = 0; OB_SUCC(ret) && i < ARRAYSIZEOF(ops); i++) {
    if (OB_FAIL(plan_tree_traverse(ops[i], NULL))) {
      SQL_OPT_LOG(WARN, "failed to do plan traverse", K(ret), "op", ops[i]);
    }
  }
  return ret;
}

}
}
#endif // OCEANBASE_SQL_OB_LOG_PLAN_H
