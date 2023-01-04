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

#define USING_LOG_PREFIX OBLOG

#include "ob_log_all_svr_cache.h"

#include "lib/allocator/ob_mod_define.h"      // ObModIds
#include "lib/utility/ob_macro_utils.h"       // OB_ISNULL, ...
#include "lib/oblog/ob_log_module.h"          // LOG_*

#include "ob_log_instance.h"                  // IObLogErrHandler
#include "ob_log_systable_helper.h"           // IObLogSysTableHelper
#include "ob_log_config.h"                    // ObLogConfig

using namespace oceanbase::common;
using namespace oceanbase::share;

namespace oceanbase
{
namespace liboblog
{

int64_t ObLogAllSvrCache::g_all_server_cache_update_interval=
	      ObLogConfig::default_all_server_cache_update_interval_sec * _SEC_;
int64_t ObLogAllSvrCache::g_all_zone_cache_update_interval=
        ObLogConfig::default_all_zone_cache_update_interval_sec * _SEC_;

ObRegion ObLogAllSvrCache::g_assign_region=ObRegion("");

ObLogAllSvrCache::ObLogAllSvrCache() :
    tid_(0),
    err_handler_(NULL),
    systable_helper_(NULL),
    stop_flag_(true),
    cur_version_(0),
    cur_zone_version_(0),
    zone_need_update_(false),
    zone_last_update_tstamp_(OB_INVALID_TIMESTAMP),
    is_region_info_valid_(true),
    svr_map_(),
    zone_map_()
{}

ObLogAllSvrCache::~ObLogAllSvrCache()
{
  destroy();
}

const char *ObLogAllSvrCache::print_svr_status(StatusType status)
{
  const char *str = "UNKNOWN";
  int ret = OB_SUCCESS;

  if (OB_FAIL(ObServerStatus::display_status_str(status, str))) {
    str = "UNKNOWN";
  }

  return str;
}

bool ObLogAllSvrCache::is_svr_avail(const common::ObAddr &svr)
{
  bool bool_ret = false;
  RegionPriority region_priority = REGION_PRIORITY_UNKNOWN;

  bool_ret = is_svr_avail(svr, region_priority);

  return bool_ret;
}

bool ObLogAllSvrCache::is_svr_avail(
    const common::ObAddr &svr,
    RegionPriority &region_priority)
{
  int ret = OB_SUCCESS;
  bool bool_ret = false;
  region_priority = REGION_PRIORITY_UNKNOWN;

  SvrItem svr_item;
  ZoneItem zone_item;

  if (OB_FAIL(get_svr_item_(svr, svr_item))) {
    bool_ret = false;
  } else if (OB_FAIL(get_zone_item_(svr_item.zone_, zone_item))) {
    LOG_ERROR("failed to get zone item", KR(ret), K(svr_item), K(zone_item));
  } else if (is_svr_serve_(svr_item)) {
    bool_ret = true;
    // get region priority of server if server is available
    region_priority = svr_item.region_priority_;
    LOG_DEBUG("is svr avail", K(svr), K(region_priority), K(zone_item));
  } else {
    region_priority = REGION_PRIORITY_UNKNOWN;
    LOG_DEBUG("svr not avail", K(svr), K(zone_item), K(svr_item));
  }

  return bool_ret;
}

int ObLogAllSvrCache::get_svr_item_(const common::ObAddr &svr, SvrItem &item)
{
  int ret = OB_SUCCESS;
  int64_t cur_ver = ATOMIC_LOAD(&cur_version_);

  if (OB_FAIL(svr_map_.get(svr, item))) {
    if (OB_ENTRY_NOT_EXIST != ret) {
      LOG_ERROR("get server item from map fail", KR(ret), K(svr));
    }
  } else if (item.version_ < cur_ver) {
    // treat as invalid record if version of data little than current version
    ret = OB_ENTRY_NOT_EXIST;
  } else {
    // succ
  }

  LOG_DEBUG("[STAT] [ALL_SVR_CACHE] [GET_SVR_ITEM]", KR(ret), K(svr),
      "status", OB_SUCCESS == ret ? print_svr_status(item.status_) : "NOT_EXIST",
      "svr_ver", item.version_, K(cur_ver),
      "zone", item.zone_,
      "region_priority", item.region_priority_);

  return ret;
}

int ObLogAllSvrCache::get_region_priority_(const common::ObRegion &region,
    RegionPriority &priority)
{
  int ret = OB_SUCCESS;

  if (is_assign_region_(region)) {
    // specified region
    priority = REGION_PRIORITY_HIGH;
  } else {
    // other region or empty region
    priority = REGION_PRIORITY_LOW;
  }
  LOG_DEBUG("get region priority", K(region), K(g_assign_region));

  return ret;
};

bool ObLogAllSvrCache::is_svr_serve_(const SvrItem &svr_item) const
{
  bool bool_ret = false;
  StatusType status = svr_item.status_;
  bool_ret = ObServerStatus::OB_SERVER_ACTIVE == status
      || ObServerStatus::OB_SERVER_DELETING == status;
  return bool_ret;
}

bool ObLogAllSvrCache::is_assign_region_(const common::ObRegion &region) const
{
  bool bool_ret = false;

  // ignore case
  bool_ret = (0 == strncasecmp(g_assign_region.ptr(),
                               region.ptr(),
                               g_assign_region.size()));

  return bool_ret;
}

int ObLogAllSvrCache::init(IObLogSysTableHelper &systable_helper, IObLogErrHandler &err_handler)
{
  int ret = OB_SUCCESS;
  int pthread_ret = 0;

  if (OB_FAIL(svr_map_.init(ObModIds::OB_LOG_ALL_SERVER_CACHE))) {
    LOG_ERROR("init svr map fail", KR(ret));
  } else if (OB_FAIL(zone_map_.init(ObModIds::OB_LOG_ALL_SERVER_CACHE))) {
    LOG_ERROR("init zone map fail", KR(ret));
  } else {
    tid_ = 0;
    stop_flag_ = false;
    cur_version_ = 0;
    cur_zone_version_ = 0;
    zone_need_update_ = false;
    zone_last_update_tstamp_ = OB_INVALID_TIMESTAMP;
    is_region_info_valid_ = true;
    err_handler_ = &err_handler;
    systable_helper_ = &systable_helper;

    LOG_INFO("init all svr cache succ");

    if (OB_UNLIKELY(0 != (pthread_ret = pthread_create(&tid_, NULL, thread_func_, this)))) {
      LOG_ERROR("create thread for all server cache fail", K(pthread_ret), KERRNOMSG(pthread_ret));
      ret = OB_ERR_UNEXPECTED;
    }
  }
  return ret;
}

void ObLogAllSvrCache::destroy()
{
  stop_flag_ = true;

  if (0 != tid_) {
    int pthread_ret = pthread_join(tid_, NULL);
    if (0 != pthread_ret) {
      LOG_ERROR("pthread_join fail", K(tid_), K(pthread_ret), KERRNOMSG(pthread_ret));
    }

    tid_ = 0;
  }

  err_handler_ = NULL;
  systable_helper_ = NULL;

  cur_version_ = 0;
  cur_zone_version_ = 0;
  zone_need_update_ = false;
  zone_last_update_tstamp_ = OB_INVALID_TIMESTAMP;
  is_region_info_valid_ = true;
  (void)svr_map_.destroy();
  (void)zone_map_.destroy();

  LOG_INFO("destroy all svr cache succ");
}

void *ObLogAllSvrCache::thread_func_(void *arg)
{
  ObLogAllSvrCache *host = static_cast<ObLogAllSvrCache *>(arg);

  if (NULL != host) {
    host->run();
  }

  return NULL;
}

void ObLogAllSvrCache::run()
{
  int ret = OB_SUCCESS;

  LOG_INFO("all svr cache thread start");

  while (! stop_flag_ && OB_SUCCESS == ret) {
    if (need_update_zone_()) {
      if (OB_FAIL(update_zone_cache_())) {
        LOG_ERROR("update zone cache error", KR(ret));
      } else if (OB_FAIL(purge_stale_zone_records_())) {
        LOG_ERROR("purge stale records fail", KR(ret));
      } else {
        // do nothing
      }
    }

    if (OB_SUCC(ret)) {
      int64_t all_svr_cache_update_interval = ATOMIC_LOAD(&g_all_server_cache_update_interval);
      if (REACH_TIME_INTERVAL(all_svr_cache_update_interval)) {
        if (OB_FAIL(update_server_cache_())) {
          LOG_ERROR("update server cache error", KR(ret));
        } else if (OB_FAIL(purge_stale_records_())) {
          LOG_ERROR("purge stale records fail", KR(ret));
        } else {
          // succ
        }
      }
    }

    // sleep
    usec_sleep(USLEEP_INTERVAL);
  }

  if (OB_SUCCESS != ret) {
    if (NULL != err_handler_) {
      err_handler_->handle_error(ret, "all server cache update thread exits, err=%d", ret);
    }
  }

  LOG_INFO("all svr cache thread stop", KR(ret));
}

void ObLogAllSvrCache::configure(const ObLogConfig & config)
{
  int ret = OB_SUCCESS;

	int64_t all_server_cache_update_interval_sec = config.all_server_cache_update_interval_sec;
  ATOMIC_STORE(&g_all_server_cache_update_interval, all_server_cache_update_interval_sec * _SEC_);
	int64_t all_zone_cache_update_interval_sec = config.all_zone_cache_update_interval_sec;
  ATOMIC_STORE(&g_all_zone_cache_update_interval, all_zone_cache_update_interval_sec * _SEC_);

  if (OB_FAIL(g_assign_region.assign(config.region.str()))) {
    LOG_ERROR("g_assign_region assign fail", KR(ret), K(g_assign_region));
  }

  LOG_INFO("[CONFIG]", K(all_server_cache_update_interval_sec));
  LOG_INFO("[CONFIG]", K(all_zone_cache_update_interval_sec));
  LOG_INFO("[CONFIG]", K(g_assign_region));
}

bool ObLogAllSvrCache::need_update_zone_()
{
  bool bool_ret = false;

  int64_t all_zone_cache_update_interval = ATOMIC_LOAD(&g_all_zone_cache_update_interval);
  bool is_region_info_valid = ATOMIC_LOAD(&is_region_info_valid_);
  int64_t update_delta_time = get_timestamp() - zone_last_update_tstamp_;

  if (!is_region_info_valid) {
    bool_ret = false;
  }
  // need update if never update
  else if (OB_INVALID_TIMESTAMP == zone_last_update_tstamp_) {
    bool_ret = true;
  }
  // update if set zone_need_update_
  else if (zone_need_update_) {
    bool_ret = true;
  }
  // update by interval
  else if (update_delta_time >= all_zone_cache_update_interval) {
    bool_ret = true;
  }

  return bool_ret;
}

int ObLogAllSvrCache::update_zone_cache_()
{
  int ret = OB_SUCCESS;
  IObLogSysTableHelper::AllZoneRecordArray record_array;
  IObLogSysTableHelper::AllZoneTypeRecordArray zone_type_record_array;
  record_array.reset();

  if (OB_ISNULL(systable_helper_)) {
    LOG_ERROR("invalid systable helper", K(systable_helper_));
    ret = OB_INVALID_ARGUMENT;
  } else if (OB_FAIL(systable_helper_->query_all_zone_info(record_array))) {
    if (OB_NEED_RETRY == ret) {
      LOG_WARN("query all zone info need retry", KR(ret));
      ret = OB_SUCCESS;
    } else if (OB_ITEM_NOT_SETTED == ret) {
      ATOMIC_STORE(&is_region_info_valid_, false);
      LOG_INFO("'region' is not availalbe in __all_zone table. would not update zone cache",
          K_(is_region_info_valid));
      ret = OB_SUCCESS;
    } else {
      LOG_ERROR("query all zone info fail", KR(ret));
    }
  } else if (OB_FAIL(systable_helper_->query_all_zone_type(zone_type_record_array))) {
    if (OB_NEED_RETRY == ret) {
      LOG_WARN("query all zone type need retry", KR(ret));
      ret = OB_SUCCESS;
    } else if (OB_ITEM_NOT_SETTED == ret) {
      LOG_INFO("'zone_type' is not availalbe in __all_zone table. would not update zone cache", K(zone_type_record_array));
      ret = OB_SUCCESS;
    } else {
      LOG_ERROR("query all zone type fail", KR(ret));
    }
  } else {
    int64_t next_version = cur_zone_version_ + 1;

    for (int64_t index = 0; OB_SUCCESS == ret && index < record_array.count(); index++) {
      IObLogSysTableHelper::AllZoneRecord &record = record_array.at(index);
      common::ObZone &zone = record.zone_;
      common::ObRegion &region = record.region_;

      _LOG_INFO("[STAT] [ALL_ZONE] INDEX=%ld/%ld ZONE=%s REGION=%s VERSION=%lu",
          index, record_array.count(), to_cstring(zone), to_cstring(region), next_version);

      ZoneItem item;
      item.reset(next_version, region);
      LOG_DEBUG("update zone cache item", K(zone), K(item));

      if (OB_FAIL(zone_map_.insert_or_update(zone, item))) {
        LOG_ERROR("zone_map_ insert_or_update fail", KR(ret), K(zone), K(item));
      }
    }

    for (int64_t idx = 0; OB_SUCCESS == ret && idx < zone_type_record_array.count(); idx ++) {
      ZoneItem item;
      IObLogSysTableHelper::AllZoneTypeRecord &record = zone_type_record_array.at(idx);
      common::ObZone &zone = record.zone_;
      common::ObZoneType &zone_type = record.zone_type_;
      if (OB_FAIL(get_zone_item_(zone, item))) {
        LOG_ERROR("fail to get zone item from cache by zone", KR(ret), K(zone));
      } else {
        item.set_zone_type(zone_type);
        if (OB_FAIL(zone_map_.insert_or_update(zone, item))) {
          LOG_ERROR("zone_map_ insert_or_update set zone_type fail", KR(ret), K(zone), K(item), K(zone_type));
        }
      }
      _LOG_INFO("[STAT] [ALL_ZONE] INDEX=%ld/%ld ZONE=%s ZONE_TYPE=%s VERSION=%lu",
          idx, zone_type_record_array.count(), to_cstring(zone), zone_type_to_str(item.get_zone_type()), next_version);
    }

    ATOMIC_INC(&cur_zone_version_);
    _LOG_INFO("[STAT] [ALL_ZONE] COUNT=%ld VERSION=%lu", record_array.count(), cur_zone_version_);
  }

  if (OB_SUCC(ret)) {
    zone_need_update_ = false;
    zone_last_update_tstamp_ = get_timestamp();
  }

  return ret;
}

int ObLogAllSvrCache::update_server_cache_()
{
  int ret = OB_SUCCESS;
  IObLogSysTableHelper::AllServerRecordArray record_array(common::ObModIds::OB_LOG_ALL_SERVER_ARRAY, common::OB_MALLOC_NORMAL_BLOCK_SIZE);
  record_array.reset();

  if (OB_ISNULL(systable_helper_)) {
    LOG_ERROR("invalid systable helper", K(systable_helper_));
    ret = OB_INVALID_ARGUMENT;
  } else if (OB_FAIL(systable_helper_->query_all_server_info(record_array))) {
    if (OB_NEED_RETRY == ret) {
      LOG_WARN("query all server info need retry", KR(ret));
      ret = OB_SUCCESS;
    } else {
      LOG_ERROR("query all server info fail", KR(ret));
    }
  } else {
    int64_t next_version = cur_version_ + 1;

    for (int64_t index = 0; OB_SUCCESS == ret && index < record_array.count(); index++) {
      IObLogSysTableHelper::AllServerRecord &record = record_array.at(index);
      ObAddr svr;
      svr.set_ip_addr(record.svr_ip_, record.svr_port_);
      const char *status_str = NULL;

      int tmp_ret = ObServerStatus::display_status_str(record.status_, status_str);
      if (OB_SUCCESS != tmp_ret) {
        LOG_ERROR("invalid server status, can not cast to string", K(tmp_ret),
            K(record.status_), K(svr));
      }

      ZoneItem zone_item;
      RegionPriority region_priority = REGION_PRIORITY_UNKNOWN;

      if (OB_FAIL(get_zone_item_(record.zone_, zone_item))) {
        LOG_ERROR("get_zone_item_ fail", KR(ret), "zone", record.zone_, K(zone_item));
      } else if (OB_FAIL(get_region_priority_(zone_item.region_, region_priority))) {
        LOG_ERROR("get priority based region fail", KR(ret), K(svr),
                  "region", zone_item.region_,
                  "region_priority", print_region_priority(region_priority));
      } else {
        SvrItem item;
        item.reset(record.status_, next_version, record.zone_, region_priority);
        LOG_DEBUG("update cache item", K(item));

        if (OB_FAIL(svr_map_.insert_or_update(svr, item))) {
          LOG_ERROR("svr_map_ insert_or_update fail", KR(ret), K(svr), K(item));
        }
      }

      _LOG_INFO("[STAT] [ALL_SERVER_LIST] INDEX=%ld/%ld SERVER=%s STATUS=%d(%s) "
          "ZONE=%s REGION=%s(%s) VERSION=%lu",
          index, record_array.count(), to_cstring(svr), record.status_, status_str,
          to_cstring(record.zone_), to_cstring(zone_item.region_),
          print_region_priority(region_priority), next_version);
    }

    ATOMIC_INC(&cur_version_);
    _LOG_INFO("[STAT] [ALL_SERVER_LIST] COUNT=%ld VERSION=%lu", record_array.count(), cur_version_);
  }

  return ret;
}

int ObLogAllSvrCache::get_zone_item_(const common::ObZone &zone,
    ZoneItem &zone_item)
{
  int ret = OB_SUCCESS;
  bool is_region_info_valid = ATOMIC_LOAD(&is_region_info_valid_);
  zone_item.reset();

  if (!is_region_info_valid) {
    LOG_DEBUG("region is invalid, do not use");
  } else {
    if (OB_FAIL(zone_map_.get(zone, zone_item))) {
      if (OB_ENTRY_NOT_EXIST != ret) {
        LOG_ERROR("zone_map_ get zone_item fail", KR(ret), K(zone), K(zone_item));
      } else {
	// update all zone cache if can't get region info in __zone_map
        zone_need_update_ = true;
        LOG_DEBUG("zone_map_ get zone_item not exist, need update", KR(ret), K(zone));
      }
    }

    int64_t cur_zone_ver = ATOMIC_LOAD(&cur_zone_version_);
    if (OB_SUCCESS == ret) {
      if (zone_item.version_ < cur_zone_ver) {
	// treate as invalid record if version little than current version
        ret = OB_ENTRY_NOT_EXIST;
      } else {
        // do nothing
      }
    }
  }

  if (OB_ENTRY_NOT_EXIST == ret) {
    ret = OB_SUCCESS;
    zone_item.reset();
  }

  return ret;
}

int ObLogAllSvrCache::purge_stale_records_()
{
  int ret = OB_SUCCESS;
  StaleRecPurger purger(cur_version_);

  if (OB_FAIL(svr_map_.remove_if(purger))) {
    LOG_ERROR("remove if fail", KR(ret), K(cur_version_));
  } else {
    _LOG_INFO("[STAT] [ALL_SERVER_LIST] [PURGE] PURGE_COUNT=%ld CUR_COUNT=%ld VERSION=%lu",
        purger.purge_count_, svr_map_.count(), cur_version_);
  }
  return ret;
}

int ObLogAllSvrCache::purge_stale_zone_records_()
{
  int ret = OB_SUCCESS;
  StaleZoneRecPurger purger(cur_zone_version_);

  if (OB_FAIL(zone_map_.remove_if(purger))) {
    LOG_ERROR("zone_map_ remove if fail", KR(ret), K(cur_zone_version_));
  } else {
    _LOG_INFO("[STAT] [ALL_ZONE] [PURGE] PURGE_COUNT=%ld CUR_COUNT=%ld VERSION=%lu",
        purger.purge_count_, zone_map_.count(), cur_zone_version_);
  }
  return ret;
}

bool ObLogAllSvrCache::StaleRecPurger::operator()(const common::ObAddr &svr,
    const SvrItem &svr_item)
{
  bool need_purge = (svr_item.version_ < cur_ver_);

  if (need_purge) {
		purge_count_++;
    _LOG_INFO("[STAT] [ALL_SERVER_LIST] [PURGE] SERVER=%s VERSION=%lu/%lu",
        to_cstring(svr), svr_item.version_, cur_ver_);
  }
  return need_purge;
}

bool ObLogAllSvrCache::StaleZoneRecPurger::operator()(const common::ObZone &zone,
    const ZoneItem &zone_item)
{
  bool need_purge = (zone_item.version_ < cur_ver_);

  if (need_purge) {
		purge_count_++;
    _LOG_INFO("[STAT] [ALL_ZONE] [PURGE] ZONE=%s VERSION=%lu/%lu",
        zone.ptr(), zone_item.version_, cur_ver_);
  }
  return need_purge;
}

void ObLogAllSvrCache::set_update_interval_(const int64_t time)
{
	ATOMIC_STORE(&g_all_server_cache_update_interval, time);
}

}
}
