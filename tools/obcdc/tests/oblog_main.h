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

#ifndef OCEANBASE_LIBOBLOG_TESTS_OBLOG_H__
#define OCEANBASE_LIBOBLOG_TESTS_OBLOG_H__

#include "share/ob_define.h"

#include "libobcdc.h"     // IObLog
#include "ob_binlog_record_printer.h" // ObBinlogRecordPrinter

namespace oceanbase
{
namespace liboblog
{
class IObLog;
class ObLogMain
{
  static const int64_t NEXT_RECORD_TIMEOUT = 1000000;

public:
  virtual ~ObLogMain();

protected:
  ObLogMain();

public:
  static ObLogMain &get_instance();

public:
  int init(int argc, char **argv);
  void destroy();

  int start();
  void run();
  void stop();
  void mark_stop_flag(const bool stop_flag) { stop_flag_ = stop_flag; }

  bool need_reentrant() const { return enable_reentrant_; }
  static void print_usage(const char *prog_name);

public:
  static void handle_error(const ObLogError &err);

private:
  int parse_args_(int argc, char **argv);
  bool check_args_();
  int verify_record_info_(ILogRecord *br);

private:
  bool                    inited_;
  IObLog                  *oblog_;
  ObLogFactory            oblog_factory_;
  ObBinlogRecordPrinter   br_printer_;

  // configuration
  bool                    only_print_hex_;
  bool                    print_hex_;
  bool                    print_lob_md5_;
  bool                    use_daemon_;
  const char              *data_file_;
  const char              *heartbeat_file_;
  int64_t                 run_time_us_;
  const char              *config_file_;
  bool                    print_console_;
  bool                    verify_mode_;
  bool                    enable_reentrant_;
  bool                    output_br_detail_;
  int64_t                 start_timestamp_usec_;
  uint64_t                tenant_id_;
  const char              *tg_match_pattern_;

  // Record heartbeat microsecond time stamps
  int64_t                 last_heartbeat_timestamp_micro_sec_;

  volatile bool           stop_flag_ CACHE_ALIGNED;

private:
  DISALLOW_COPY_AND_ASSIGN(ObLogMain);
};
} // namespace liboblog
} // namespace oceanbase
#endif /* OCEANBASE_LIBOBLOG_TESTS_OBLOG_H__ */
