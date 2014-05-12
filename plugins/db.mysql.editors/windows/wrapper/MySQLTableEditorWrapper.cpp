/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "MySQLTableEditorWrapper.h"
#include "grtdb/db_object_helpers.h"
#include "mforms/view.h"

using namespace System;

using namespace MySQL::Forms;

using namespace MySQL::Grt::Db;

//----------------- MySQLTableColumnsListWrapper ---------------------------------------------------

MySQLTableColumnsListWrapper::MySQLTableColumnsListWrapper(::MySQLTableColumnsListBE *inn)
  : TableColumnsListWrapper(inn)
{}

//----------------- MySQLTableEditorWrapper --------------------------------------------------------

MySQLTableEditorWrapper::MySQLTableEditorWrapper(MySQL::Grt::GrtManager^ grtm, MySQL::Grt::GrtValue^ arglist)
: TableEditorWrapper(
      new ::MySQLTableEditorBE(grtm->get_unmanaged_object(), 
      db_mysql_TableRef::cast_from(::grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)),
      get_rdbms_for_db_object(::grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0))
      )
    )
{
}

//--------------------------------------------------------------------------------------------------

MySQLTableEditorWrapper::~MySQLTableEditorWrapper()
{
  // These wrappers keep a gc pointer to this instance (if assigned);
  set_refresh_partial_ui_handler(nullptr);
  set_refresh_ui_handler(nullptr);

  delete inner; // We created it.
}

//--------------------------------------------------------------------------------------------------

MySQLTableColumnsListWrapper^ MySQLTableEditorWrapper::get_columns() 
{ return gcnew MySQLTableColumnsListWrapper(get_unmanaged_object()->get_columns()); }


//--------------------------------------------------------------------------------------------------

MySQLTablePartitionTreeWrapper^ MySQLTableEditorWrapper::get_partitions()
{
  return gcnew MySQLTablePartitionTreeWrapper(get_unmanaged_object()->get_partitions());
}

//--------------------------------------------------------------------------------------------------

Control ^MySQLTableEditorWrapper::get_trigger_panel()
{
  return dynamic_cast<Control^>(ObjectMapper::GetManagedComponent(get_unmanaged_object()->get_trigger_panel()));
}

//--------------------------------------------------------------------------------------------------

void MySQLTableEditorWrapper::commit_changes()
{
  get_unmanaged_object()->commit_changes();
}

//--------------------------------------------------------------------------------------------------

bool MySQLTableEditorWrapper::is_server_version_at_least(int major, int minor)
{
  return get_unmanaged_object()->is_server_version_at_least(major, minor);
}

//--------------------------------------------------------------------------------------------------

void MySQLTableEditorWrapper::load_trigger_sql()
{
  get_unmanaged_object()->load_trigger_sql();
}

//--------------------------------------------------------------------------------------------------
