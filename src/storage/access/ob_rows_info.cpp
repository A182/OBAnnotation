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

#include "ob_rows_info.h"
#include "storage/ob_storage_struct.h"
#include "storage/ob_relative_table.h"
#include "storage/ob_storage_schema.h"

namespace oceanbase
{
using namespace common;
using namespace blocksstable;
namespace storage
{

ObRowsInfo::ExistHelper::ExistHelper()
  : table_iter_param_(),
    table_access_context_(),
    is_inited_(false)
{
}

ObRowsInfo::ExistHelper::~ExistHelper()
{
}

void ObRowsInfo::ExistHelper::reset()
{
  table_iter_param_.reset();
  table_access_context_.reset();
  is_inited_ = false;
}

int ObRowsInfo::ExistHelper::init(const ObRelativeTable &table,
                                  ObStoreCtx &store_ctx,
                                  const ObITableReadInfo &rowkey_read_info,
                                  ObStorageReserveAllocator &allocator)
{
  int ret = OB_SUCCESS;
  const ObTablet *tablet = nullptr;

  if (OB_UNLIKELY(is_inited_)) {
    ret = OB_INIT_TWICE;
    STORAGE_LOG(WARN, "ObExistPrefixScanHelper init twice", K(ret));
  } else {
    common::ObQueryFlag query_flag;
    common::ObVersionRange trans_version_range;
    query_flag.read_latest_ = ObQueryFlag::OBSF_MASK_READ_LATEST;
    query_flag.use_row_cache_ = ObQueryFlag::DoNotUseCache;

    trans_version_range.snapshot_version_ = EXIST_READ_SNAPSHOT_VERSION;
    trans_version_range.base_version_ = 0;
    trans_version_range.multi_version_start_ = 0;

    if (OB_FAIL(table_access_context_.init(query_flag, store_ctx, allocator,
            trans_version_range))) {
      STORAGE_LOG(WARN, "failed to init table access ctx", K(ret));
    } else {
      table_iter_param_.table_id_ = table.get_table_id();
      table_iter_param_.tablet_id_ = table.get_tablet_id();
      table_iter_param_.out_cols_project_ = NULL;
      table_iter_param_.read_info_ = &rowkey_read_info;
      is_inited_ = true;
    }
  }

  return ret;
}


ObRowsInfo::ObRowsInfo()
  : scan_mem_allocator_(ObMemAttr(MTL_ID(), common::ObModIds::OB_STORE_ROW_EXISTER), OB_MALLOC_NORMAL_BLOCK_SIZE),
    key_allocator_(ObMemAttr(MTL_ID(), common::ObModIds::OB_STORE_ROW_EXISTER), OB_MALLOC_NORMAL_BLOCK_SIZE),
    rowkeys_(),
    rows_(nullptr),
    exist_helper_(),
    table_id_(OB_INVALID_ID),
    tablet_id_(),
    datum_utils_(nullptr),
    min_key_(),
    delete_count_(0),
    rowkey_column_num_(0),
    is_inited_(false)
{
  min_key_.set_max_rowkey();
}

ObRowsInfo::~ObRowsInfo()
{
}

void ObRowsInfo::reset()
{
  exist_helper_.reset();
  min_key_.set_max_rowkey();
  rows_ = nullptr;
  delete_count_ = 0;
  rowkey_column_num_ = 0;
  rows_ = nullptr;
  datum_utils_ = nullptr;
  table_id_ = OB_INVALID_ID;
  tablet_id_.reset();
  rowkeys_.reset();
  scan_mem_allocator_.reset();
  key_allocator_.reset();
  delete_count_ = 0;
  is_inited_ = false;
}

int ObRowsInfo::init(
    const ObRelativeTable &table,
    ObStoreCtx &store_ctx,
    const ObITableReadInfo &rowkey_read_info)
{
  int ret = OB_SUCCESS;

  if (OB_UNLIKELY(is_inited_)) {
    ret = OB_INIT_TWICE;
    STORAGE_LOG(WARN, "ObRowsinfo init twice", K(ret));
  } else if (OB_FAIL(exist_helper_.init(table, store_ctx, rowkey_read_info, scan_mem_allocator_))) {
    STORAGE_LOG(WARN, "Failed to init exist helper", K(ret));
  } else {
    datum_utils_ = &rowkey_read_info.get_datum_utils();
    table_id_ = table.get_table_id();
    tablet_id_ = table.get_tablet_id();
    rowkey_column_num_ = table.get_rowkey_column_num();
    is_inited_ = true;
  }
  return ret;
}

void ObRowsInfo::reuse()
{
  min_key_.set_max_rowkey();
  rows_ = nullptr;
  delete_count_ = 0;
  scan_mem_allocator_.reuse();
  rowkeys_.reuse();
  key_allocator_.reuse();
}

//not only checking duplicate, but also assign rowkeys
int ObRowsInfo::check_duplicate(ObStoreRow *rows, const int64_t row_count, ObRelativeTable &table)
{
  int ret = OB_SUCCESS;
  int tmp_ret = OB_SUCCESS;
  ObITable *itable_ptr = nullptr;

  if (OB_UNLIKELY(!is_inited_)) {
    ret = OB_NOT_INIT;
    STORAGE_LOG(WARN, "ObRowsInfo not init", K(ret));
  } else if (OB_ISNULL(rows) || row_count <= 0) {
    ret = OB_INVALID_ARGUMENT;
    STORAGE_LOG(WARN, "Invalid parameter", K(rows), K(row_count), K(ret));
  } else {
    reuse();
    table.tablet_iter_.table_iter()->resume();
    rows_ = rows;
  }

  if (OB_SUCC(ret)) {
    for (int64_t i = 0; OB_SUCC(ret) && i < row_count; i++) {
      if (OB_UNLIKELY(!rows[i].is_valid())) {
        ret = OB_INVALID_ARGUMENT;
        STORAGE_LOG(WARN, "invalid argument", K(rows_[i]), K(ret));
      } else {
        ObDatumRowkey datum_rowkey;
        ObRowkey rowkey(rows_[i].row_val_.cells_, rowkey_column_num_);
        if (OB_FAIL(datum_rowkey.from_rowkey(rowkey, key_allocator_))) {
          STORAGE_LOG(WARN, "Failed to transfer rowkey", K(ret), K(rowkey));
        } else if (OB_FAIL(rowkeys_.push_back(datum_rowkey))) {
          STORAGE_LOG(WARN, "Failed to push back datum rowkey", K(ret), K(datum_rowkey));
        }
      }
    }
    if (OB_SUCC(ret)) {
      if (row_count > 1) {
        RowsCompare rows_cmp(*datum_utils_, min_key_, true, ret);
        std::sort(rowkeys_.begin(), rowkeys_.end(), rows_cmp);
      }
      if (OB_SUCC(ret)) {
        min_key_ = rowkeys_.at(0);
        STORAGE_LOG(DEBUG, "Duplicate check complete", K(ret), K(row_count), K_(rowkey_column_num));
      }
    }
  }

  return ret;
}

int ObRowsInfo::check_min_rowkey_boundary(const blocksstable::ObDatumRowkey &max_rowkey, bool &may_exist)
{
  int ret = OB_SUCCESS;
  int cmp_ret = 0;
  may_exist = true;

  if (OB_UNLIKELY(!is_valid())) {
    ret = OB_ERR_UNEXPECTED;
    STORAGE_LOG(WARN, "Unexpected not init rowsinfo", K_(delete_count), KP_(rows), K(ret));
  } else if (OB_UNLIKELY(!max_rowkey.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    STORAGE_LOG(WARN, "Invalid argument to check min rowkey", K(ret), K(max_rowkey));
  } else if (OB_FAIL(min_key_.compare(max_rowkey, *datum_utils_, cmp_ret))) {
    STORAGE_LOG(WARN, "Failed to compare datum rowkey", K(ret), K(min_key_), K(max_rowkey));
  } else if (cmp_ret > 0) {
    may_exist = false;
  }

  return ret;
}

int ObRowsInfo::refine_rowkeys()
{
  int ret = OB_SUCCESS;

  if (!is_valid()) {
    ret = OB_ERR_UNEXPECTED;
    STORAGE_LOG(WARN, "Unexpected not init rowsinfo", K_(delete_count), KP_(rows), K(ret));
  } else if (rowkeys_.count() == delete_count_ || rowkeys_.empty()) {
    ret = OB_ERR_UNEXPECTED;
    STORAGE_LOG(WARN, "Unexpected rowkey status", K(ret), K_(delete_count), K_(rowkeys));
  } else if (delete_count_ > 0) {
    if (rowkeys_.count() > 1) {
      RowsCompare rows_cmp(*datum_utils_, min_key_, false, ret);
      std::sort(rowkeys_.begin(), rowkeys_.end(), rows_cmp);
    }
    if (OB_SUCC(ret)) {
      while (rowkeys_.count() > 0 ){
        if (rowkeys_.at(rowkeys_.count() - 1).is_max_rowkey()) {
          rowkeys_.pop_back();
        } else {
          break;
        }
      }
      delete_count_ = 0;
      min_key_ = rowkeys_.at(0);
      STORAGE_LOG(DEBUG, "Duplicate check complete", K(ret), K_(rowkeys), K_(rowkey_column_num));
    } else {
      STORAGE_LOG(WARN, "Unexpected duplicate rowkeys", K_(rowkeys), K(ret));
    }
  }

  return ret;
}

int ObRowsInfo::clear_found_rowkey(const int64_t rowkey_idx)
{
  int ret = OB_SUCCESS;

  if (!is_valid()) {
    ret = OB_ERR_UNEXPECTED;
    STORAGE_LOG(WARN, "Unexpected not init rowsinfo", K_(delete_count), KP_(rows), K(ret));
  } else if (rowkey_idx < 0 || rowkey_idx >= rowkeys_.count()) {
    ret = OB_INVALID_ARGUMENT;
    STORAGE_LOG(WARN, "Invalid argument to clear found rowkey", K(rowkey_idx), K_(rowkeys), K(ret));
  } else if (rowkeys_.at(rowkey_idx).is_max_rowkey())  {
    ret = OB_ERR_UNEXPECTED;
    STORAGE_LOG(WARN, "Clear found rowkey twice", K(rowkey_idx), K(ret));
  } else {
    rowkeys_.at(rowkey_idx).set_max_rowkey();
    delete_count_++;
  }

  return ret;
}

} // namespace storage
} // namespace oceanbase
