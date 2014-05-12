/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "base/ui_form.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"

#include "mforms/widgets.h"
#include "mforms/sectionbox.h"
#include "mforms/textbox.h"
#include "mforms/panel.h"
#include "mforms/radiobutton.h"

#include "grtpp.h"

#include "grts/structs.h"
#include "grts/structs.app.h"
#include "grts/structs.workbench.physical.h"

#include "grt/editor_base.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"

#include "preferences_form.h"

#include "grtdb/db_helpers.h"
#include "snippet_popover.h"

#include "grtpp_notifications.h"

#if defined(_WIN32) || defined(__APPLE__)
#define HAVE_BUNDLED_MYSQLDUMP
#endif

using namespace base;
using namespace mforms;

#include "base/drawing.h"

struct LangFontSet {
  const char *name;

  // TODO: there are a few fonts missing here.
  const char *object_title_font;
  const char *object_section_font;
  const char *object_item_font;
  const char *layer_title_font;
  const char *note_font;
};

static LangFontSet font_sets[] = {
  {"Default (Western)",
    DEFAULT_FONT_FAMILY" Bold 12",
    DEFAULT_FONT_FAMILY" Bold 11",
    DEFAULT_FONT_FAMILY" 11",
    DEFAULT_FONT_FAMILY" 11",
    DEFAULT_FONT_FAMILY" 11",
  },
#ifdef _WIN32
  {"Japanese",
    "Arial Unicode MS Bold 12",
    "Arial Unicode MS Bold 11",
    "Arial Unicode MS 11",
    "Arial Unicode MS 11",
    "Arial Unicode MS 11"
  },
  {"Korean",
    "Arial Unicode MS Bold 12",
    "Arial Unicode MS Bold 11",
    "Arial Unicode MS 11",
    "Arial Unicode MS 11",
    "Arial Unicode MS 11"
  },
  {"Simplified Chinese",
    "Arial Unicode MS Bold 12",
    "Arial Unicode MS Bold 11",
    "Arial Unicode MS 11",
    "Arial Unicode MS 11",
    "Arial Unicode MS 11"
  },
  {"Cyrillic",
    DEFAULT_FONT_FAMILY" Bold 12",
    DEFAULT_FONT_FAMILY" Bold 11",
    DEFAULT_FONT_FAMILY" 11",
    DEFAULT_FONT_FAMILY" 11",
    DEFAULT_FONT_FAMILY" 11"
  },
#elif defined(__APPLE__)
  {"Japanese",
    "Osaka Bold 12",
    "Osaka Bold 11",
    "Osaka 11",
    "Osaka 11",
    "Osaka 11"
  },
  {"Korean",
    "AppleGothic Bold 12",
    "AppleGothic Bold 11",
    "AppleGothic 11",
    "AppleGothic 11",
    "AppleGothic 11"
  },
  {"Simplified Chinese",
    "SimHei Bold 12",
    "SimHei Bold 11",
    "SimHei 11",
    "SimHei 11",
    "SimHei 11"
  },
  {"Cyrillic",
    DEFAULT_FONT_FAMILY" Bold 12",
    DEFAULT_FONT_FAMILY" Bold 11",
    DEFAULT_FONT_FAMILY" 11",
    DEFAULT_FONT_FAMILY" 11",
    DEFAULT_FONT_FAMILY" 11"
  },
#else
  {"Japanese",
    "VL Gothic Bold 12",
    "VL Gothic Bold 11",
    "VL Gothic 11",
    "VL Gothic 11",
    "VL Gothic 11"
  },
  {"Korean",
    "WenQuanYi Zen Hei Bold 12",
    "WenQuanYi Zen Hei Bold 11",
    "WenQuanYi Zen Hei 11",
    "WenQuanYi Zen Hei 11",
    "WenQuanYi Zen Hei 11"
  },
  {"Simplified Chinese",
    "WenQuanYi Zen Hei Bold 12",
    "WenQuanYi Zen Hei Bold 11",
    "WenQuanYi Zen Hei 11",
    "WenQuanYi Zen Hei 11",
    "WenQuanYi Zen Hei 11"
  },
  {"Cyrillic",
    DEFAULT_FONT_FAMILY" Bold 12",
    DEFAULT_FONT_FAMILY" Bold 11",
    DEFAULT_FONT_FAMILY" 11",
    DEFAULT_FONT_FAMILY" 11",
    DEFAULT_FONT_FAMILY" 11"
  },
#endif
  {NULL, NULL, NULL, NULL, NULL, NULL}
};

static mforms::Label *new_label(const std::string &text, bool right_align=false, bool help=false)
{
  mforms::Label *label= mforms::manage(new mforms::Label());
  label->set_text(text);
  if (right_align)
    label->set_text_align(mforms::MiddleRight);
  if (help)
    label->set_style(mforms::SmallHelpTextStyle);
  return label;
}



class OptionTable : public mforms::Panel
{
  PreferencesForm *_owner;
  mforms::Table _table;
  int _rows;
  bool _help_column;
  
public:
  OptionTable(PreferencesForm *owner, const std::string &title, bool help_column)
  : mforms::Panel(mforms::TitledBoxPanel), _owner(owner), _rows(0), _help_column(help_column)
  {
    set_title(title);
    
    add(&_table);
    
    _table.set_padding(8);
    _table.set_row_spacing(12);
    _table.set_column_spacing(8);
    
    _table.set_column_count(_help_column ? 3 : 2);    
  }
  
  void add_option(mforms::View *control, const std::string &caption, const std::string &help)
  {
    _table.set_row_count(++_rows);
   
#ifdef _WIN32
    TableItemFlags descriptionFlags = mforms::HFillFlag;
    TableItemFlags helpFlags = mforms::HFillFlag | mforms::HExpandFlag;
    bool right_aligned = false;
#else
    TableItemFlags descriptionFlags = mforms::HFillFlag | mforms::HExpandFlag;
    TableItemFlags helpFlags = mforms::HFillFlag;
    bool right_aligned = true;
#endif

    mforms::Label* label = new_label(caption, right_aligned);
    _table.add(label, 0, 1, _rows-1, _rows, descriptionFlags);
    label->set_size(170, -1);
    _table.add(control, 1, 2, _rows-1, _rows, mforms::HFillFlag | mforms::HExpandFlag);
    control->set_size(150, -1);

    _table.add(new_label(help, false, true), 2, 3, _rows-1, _rows, helpFlags);
  }
  

  void add_entry_option(const std::string &option, const std::string &caption, const std::string &tooltip)
  {
    _table.set_row_count(++_rows);

    mforms::TextEntry *entry = _owner->new_entry_option(option, false);
    entry->set_tooltip(tooltip);
    
#ifdef _WIN32
    TableItemFlags descriptionFlags = mforms::HFillFlag;
    bool right_aligned = false;
#else
    TableItemFlags descriptionFlags = mforms::HFillFlag | mforms::HExpandFlag;
    bool right_aligned = true;
#endif

    mforms::Label* label = new_label(caption, right_aligned);
    _table.add(label, 0, 1, _rows-1, _rows, descriptionFlags);
    label->set_size(170, -1);
    _table.add(entry, 1, 2, _rows-1, _rows, mforms::HFillFlag);
  }

  mforms::CheckBox *add_checkbox_option(const std::string &option, const std::string &caption, const std::string &tooltip)
  {
    _table.set_row_count(++_rows);
    
    mforms::CheckBox *cb = _owner->new_checkbox_option(option);
    cb->set_text(caption);
    cb->set_tooltip(tooltip);
    
#ifdef _WIN32
    int start_column = 0;
#else
    int start_column = 1;
#endif
    _table.add(cb, start_column, 3, _rows - 1, _rows, mforms::HFillFlag);
    
    return cb;
  }
};

//----------------- PreferencesForm ----------------------------------------------------------------

PreferencesForm::PreferencesForm(wb::WBContextUI *wbui, const workbench_physical_ModelRef &model)
  : Form(NULL, mforms::FormResizable), _top_box(false), _bottom_box(true), 
  _tabview(mforms::TabViewDocument), _button_box(true), _font_list(mforms::TreeFlatList)
{
  _wbui = wbui;
  _model = model;

  set_name("preferences");

  if (!model.is_valid())
    set_title(_("Workbench Preferences"));
  else
    set_title(_("Model Options"));

  set_content(&_top_box);
#ifdef _WIN32
  set_back_color(base::Color::get_application_color_as_string(base::AppColorMainBackground, false));
#endif

  _top_box.set_padding(4);
  _top_box.set_spacing(4);

  _top_box.add(&_tabview, true, true);

  _top_box.add(&_bottom_box, false);

  _bottom_box.add_end(&_button_box, false, true);
  _button_box.set_padding(7);
  _button_box.set_spacing(8);
  _button_box.set_homogeneous(true);

  scoped_connect(_ok_button.signal_clicked(),boost::bind(&PreferencesForm::ok_clicked, this));
  scoped_connect(_cancel_button.signal_clicked(),boost::bind(&PreferencesForm::cancel_clicked, this));

  _cancel_button.set_text(_("Cancel"));
  _cancel_button.enable_internal_padding(true);
  _button_box.add_end(&_cancel_button, false, true);

  _ok_button.set_text(_("OK"));
  _ok_button.enable_internal_padding(true);
  _button_box.add_end(&_ok_button, false, true);
  
  if (_model.is_valid())
  {
    _use_global.set_text(_("Use defaults from global settings"));
#ifdef _WIN32
    if (base::Color::get_active_scheme() == ColorSchemeStandardWin7)
      _use_global.set_front_color("#FFFFFF");
    else
      _use_global.set_front_color("#000000");
#endif
    _bottom_box.add(&_use_global, true, true);
    scoped_connect(_use_global.signal_clicked(),boost::bind(&PreferencesForm::toggle_use_global, this));
  }

  if (!_model.is_valid())
  {
    create_general_page();
    create_admin_page();
    create_sqlide_page();
    create_query_page();
  }
  create_model_page();
  create_mysql_page();
  create_diagram_page();
  if (!_model.is_valid())
  {
    create_appearance_page();
#ifdef _WIN32
    create_color_scheme_page();
#endif
  }

  grt::DictRef info(wbui->get_wb()->get_grt());
  if (!_model.is_valid())
    info.set("options", _wbui->get_wb()->get_wb_options());
  else
  {
    info.set("model-options", _wbui->get_model_options(_model.id()));
    info.set("model", model);
  }
  grt::GRTNotificationCenter::get()->send_grt("GRNPreferencesDidCreate", grt::ObjectRef(), info);

  set_size(700, 680);
  center();
  
  show_values();
}

//--------------------------------------------------------------------------------------------------

PreferencesForm::~PreferencesForm()
{
  for (std::list<Option*>::iterator iter= _options.begin(); iter != _options.end(); ++iter)
    delete *iter;
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::show()
{
  grt::DictRef info(_wbui->get_wb()->get_grt());
  if (!_model.is_valid())
    info.set("options", _wbui->get_wb()->get_wb_options());
  else
  {
    info.set("model-options", _wbui->get_model_options(_model.id()));
    info.set("model", _model);
  }
  grt::GRTNotificationCenter::get()->send_grt("GRNPreferencesWillOpen", grt::ObjectRef(), info);
  
  if (run_modal(&_ok_button, &_cancel_button))
    info.set("saved", grt::IntegerRef(1));
  else
    info.set("saved", grt::IntegerRef(0));
  
  grt::GRTNotificationCenter::get()->send_grt("GRNPreferencesDidClose", grt::ObjectRef(), info);
}


void PreferencesForm::show_values()
{
  for (std::list<Option*>::const_iterator iter= _options.begin(); iter != _options.end(); ++iter)
    (*iter)->show_value();

  if (!_model.is_valid())
  {
    show_colors_and_fonts();
  }

  if (_model.is_valid())
  {
    std::string value;
    _wbui->get_wb_options_value(_model.id(), "useglobal", value);
    if (value == "1")
    {
      _use_global.set_active(true);
      _tabview.set_enabled(false);
    }
  }
}


void PreferencesForm::update_values()
{
  grt::AutoUndo undo(_wbui->get_wb()->get_grt(), !_model.is_valid());

  if (_model.is_valid())
  {
    _wbui->set_wb_options_value(_model.id(), "useglobal", _use_global.get_active() ? "1" : "0");
  }

  if (!_model.is_valid() || !_use_global.get_active())
  {
    for (std::list<Option*>::const_iterator iter= _options.begin(); iter != _options.end(); ++iter)
    {
      (*iter)->update_value();
    }
  }

  if (!_model.is_valid())
    update_colors_and_fonts();

  undo.end(_("Change Options"));
}


grt::DictRef PreferencesForm::get_options(bool global)
{
  if (!_model.is_valid() || global)
    return _wbui->get_wb()->get_wb_options();
  else
    return _wbui->get_model_options(_model.id());
}


void PreferencesForm::show_entry_option(const std::string &option_name, mforms::TextEntry *entry, bool numeric)
{
  std::string value;

  _wbui->get_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value);
  entry->set_value(value);
}


void PreferencesForm::update_entry_option(const std::string &option_name, mforms::TextEntry *entry, bool numeric)
{
  if (numeric)
    _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, entry->get_string_value(), grt::AnyType);
  else
    _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, entry->get_string_value(), grt::StringType);
}

void PreferencesForm::show_path_option(const std::string &option_name, mforms::FsObjectSelector *entry)
{
  std::string value;
  
  _wbui->get_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value);
  entry->set_filename(value);
}

void PreferencesForm::update_path_option(const std::string &option_name, mforms::FsObjectSelector *entry)
{
  _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, entry->get_filename(), grt::StringType);
}


void PreferencesForm::update_entry_option_numeric(const std::string &option_name, mforms::TextEntry *entry, int minrange, int maxrange)
{
  long value= atoi(entry->get_string_value().c_str());
  if (value < minrange)
    value= minrange;
  else if (value > maxrange)
    value= maxrange;
  
  _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, strfmt("%li", value));
}


void PreferencesForm::show_checkbox_option(const std::string &option_name, mforms::CheckBox *checkbox)
{
  std::string value;

  _wbui->get_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value);

  checkbox->set_active(atoi(value.c_str()) != 0);
}


void PreferencesForm::update_checkbox_option(const std::string &option_name, mforms::CheckBox *checkbox)
{
  std::string value = checkbox->get_active() ? "1" : "0";
  _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value, grt::IntegerType);

#ifdef _WIN32
  // On Windows we have to write the following value also to the registry as our options are not
  // available yet when we need that value.
  if (option_name == "DisableSingleInstance")
    set_value_to_registry(HKEY_CURRENT_USER, "Software\\Oracle\\MySQL Workbench",
      "DisableSingleInstance", value.c_str());
#endif
}


void PreferencesForm::show_selector_option(const std::string &option_name, mforms::Selector *selector,
                                           const std::vector<std::string> &choices)
{
  std::string value;
  _wbui->get_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value);
  selector->set_selected((int)(std::find(choices.begin(), choices.end(), value) - choices.begin()));
}


void PreferencesForm::update_selector_option(const std::string &option_name, mforms::Selector *selector,
                                             const std::vector<std::string> &choices, const std::string &default_value, bool as_number)
{
  if (as_number)
  {
    if (selector->get_selected_index() < 0)
      _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, default_value, grt::IntegerType);
    else
      _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, choices[selector->get_selected_index()], grt::IntegerType);
  }
  else 
  {
    if (selector->get_selected_index() < 0)
      _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, default_value);
    else
      _wbui->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, choices[selector->get_selected_index()]);
  }

  if (option_name == "ColorScheme")
  {
    base::Color::set_active_scheme((base::ColorScheme)selector->get_selected_index());
    NotificationCenter::get()->send("GNColorsChanged", NULL);
  }
}


mforms::TextEntry *PreferencesForm::new_entry_option(const std::string &option_name, bool numeric)
{
  Option *option= new Option();
  mforms::TextEntry *entry= new mforms::TextEntry();

  option->view= mforms::manage(entry);
  option->show_value= boost::bind(&PreferencesForm::show_entry_option, this, option_name, entry, numeric);
  option->update_value= boost::bind(&PreferencesForm::update_entry_option, this, option_name, entry, numeric);
  _options.push_back(option);

  return entry;
}


mforms::FsObjectSelector *PreferencesForm::new_path_option(const std::string &option_name, bool file)
{
  Option *option= new Option();
  mforms::FsObjectSelector *entry= new mforms::FsObjectSelector();
  
  entry->initialize("", file ? mforms::OpenFile : mforms::OpenDirectory, "");
  
  option->view= mforms::manage(entry);
  option->show_value= boost::bind(&PreferencesForm::show_path_option, this, option_name, entry);
  option->update_value= boost::bind(&PreferencesForm::update_path_option, this, option_name, entry);
  _options.push_back(option);
  
  return entry;
}


mforms::TextEntry *PreferencesForm::new_numeric_entry_option(const std::string &option_name, int minrange, int maxrange)
{
  Option *option= new Option();
  mforms::TextEntry *entry= new mforms::TextEntry();
  
  option->view= mforms::manage(entry);
  option->show_value= boost::bind(&PreferencesForm::show_entry_option, this, option_name, entry, true);
  option->update_value= boost::bind(&PreferencesForm::update_entry_option_numeric, this, option_name, entry, minrange, maxrange);
  _options.push_back(option);
  
  return entry;  
}


mforms::CheckBox *PreferencesForm::new_checkbox_option(const std::string &option_name)
{
  Option *option= new Option();
  mforms::CheckBox *checkbox= new mforms::CheckBox();

  option->view= mforms::manage(checkbox);
  option->show_value= boost::bind(&PreferencesForm::show_checkbox_option, this, option_name, checkbox);
  option->update_value= boost::bind(&PreferencesForm::update_checkbox_option, this, option_name, checkbox);
  _options.push_back(option);

  return checkbox;
}


mforms::Selector *PreferencesForm::new_selector_option(const std::string &option_name, std::string choices_string, bool as_number)
{
  Option *option= new Option();
  mforms::Selector *selector= new mforms::Selector();

  if (choices_string.empty())
    _wbui->get_wb_options_value(_model.is_valid() ? _model.id() : "", "@"+option_name+"/Items", choices_string);

  std::vector<std::string> choices, parts= base::split(choices_string, ",");

  for (std::vector<std::string>::const_iterator iter= parts.begin();
       iter != parts.end(); ++iter)
  {
    std::vector<std::string> tmp= base::split(*iter, ":", 1);
    if (tmp.size() == 1)
    {
      selector->add_item(*iter);
      choices.push_back(*iter);
    }
    else
    {
      selector->add_item(tmp[0]);
      choices.push_back(tmp[1]);
    }
  }

  option->view= mforms::manage(selector);
  option->show_value= boost::bind(&PreferencesForm::show_selector_option, this, option_name, selector, choices);
  option->update_value= boost::bind(&PreferencesForm::update_selector_option, this, option_name, selector, choices, choices.empty() ? "" : choices[0], as_number);
  _options.push_back(option);

  return selector;
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::ok_clicked()
{
  update_values();

  mforms::Form::show(false);
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::cancel_clicked()
{
  mforms::Form::show(false);
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::create_admin_page()
{
  mforms::Box *box= mforms::manage(new mforms::Box(false));
  box->set_padding(12);
  box->set_spacing(8);

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Data Export and Import"));
    
    mforms::Table *table= mforms::manage(new mforms::Table());
    
    table->set_padding(8);
    table->set_row_spacing(12);
    table->set_column_spacing(8);
    
    table->set_row_count(3);
    table->set_column_count(3);
    
    frame->add(table);
    
    mforms::FsObjectSelector *pathsel;
    table->add(new_label(_("Path to mysqldump Tool:"), true), 0, 1, 0, 1, mforms::HFillFlag);
    pathsel= new_path_option("mysqldump", true);
    pathsel->get_entry()->set_tooltip(_("Specifiy the full path to the mysqldump tool, which is needed for the Workbench Administrator.\nIt usually comes bundled with the MySQL server and/or client packages."));
    table->add(pathsel, 1, 2, 0, 1, mforms::HFillFlag|mforms::HExpandFlag);
#ifdef HAVE_BUNDLED_MYSQLDUMP
    table->add(new_label(_("Leave blank to use bundled version."), false, true), 2, 3, 0, 1, mforms::HFillFlag);
#else
    table->add(new_label(_("Full path to the mysqldump tool\nif it's not in your PATH."), false, true), 2, 3, 0, 1, mforms::HFillFlag);
#endif
    table->add(new_label(_("Path to mysql Tool:"), true), 0, 1, 1, 2, mforms::HFillFlag);
    pathsel= new_path_option("mysqlclient", true);
    pathsel->get_entry()->set_tooltip(_("Specifiy the full path to the mysql command line client tool, which is needed for the Workbench Administrator.\nIt usually comes bundled with the MySQL server and/or client packages."));
    table->add(pathsel, 1, 2, 1, 2, mforms::HFillFlag|mforms::HExpandFlag);
#ifdef HAVE_BUNDLED_MYSQLDUMP
    table->add(new_label(_("Leave blank to use bundled version."), false, true), 2, 3, 1, 2, mforms::HFillFlag);
#else
    table->add(new_label(_("Full path to the mysql tool\nif it's not in your PATH."), false, true), 2, 3, 1, 2, mforms::HFillFlag);
#endif

    table->add(new_label(_("Export Directory Path:"), true), 0, 1, 2, 3, mforms::HFillFlag);
    pathsel= new_path_option("dumpdirectory", false);
    pathsel->get_entry()->set_tooltip(_("Specifiy the full path to the directory where dump files should be placed by default."));
    table->add(pathsel, 1, 2, 2, 3, mforms::HFillFlag|mforms::HExpandFlag);
    table->add(new_label(_("Location where dump files should\nbe placed by default."), false, true), 2, 3, 2, 3, mforms::HFillFlag);

    box->add(frame, false);
  }
    
  _tabview.add_page(box, _("Administrator"));
}


void PreferencesForm::create_sqlide_page()
{
  // General options for the SQL Editor
  
  mforms::Box *box= mforms::manage(new mforms::Box(false));
  box->set_padding(12);
  box->set_spacing(8);
  
  _tabview.add_page(box, _("SQL Editor"));

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("General"));
    box->add(frame, false);
    
    mforms::Box *vbox= mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(4);
    frame->add(vbox);
    
    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:ShowSchemaTreeSchemaContents");
      check->set_text(_("Show Schema Contents in Schema Tree"));
      check->set_tooltip(_("Whether to show schema contents (tables, views and routine names) in "
        "schema tree."));
      vbox->add(check, false);
    }
    
    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:ShowMetadataSchemata");
      check->set_text(_("Show Data Dictionaries and Internal Schemas"));
      check->set_tooltip(_("Whether to show data dictionaries/internal schemas in the schema tree "
        "(eg INFORMATION_SCHEMA, mysql and schemas starting with '.')."));
      vbox->add(check, false);
    }
    
    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:SidebarModeCombined");
      check->set_text(_("Show Management Tools and Schema Tree in a single tab"));
      check->set_tooltip(_("Check this if you want to display the management tools and the "
        "schema list in the same tab page in the sidebar. Uncheck it to have them "
	"in separate tab pages."));
      vbox->add(check, false);
    }

    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("DBMS connection keep-alive interval (in seconds):"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("DbSqlEditor:KeepAliveInterval", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_(
                           "Time interval between sending keep-alive messages to DBMS.\n"
                           "Set to 0 to not send keep-alive messages."));
      tbox->add(entry, false, false);
    }    
    
    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("DBMS connection read time out (in seconds):"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("DbSqlEditor:ReadTimeOut", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_("Max time the a query can take to return data from the DBMS"));
      tbox->add(entry, false, false);
    }
  }
  
  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Productivity"));
    box->add(frame, false);
    
    mforms::Box *vbox= mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(4);
    frame->add(vbox);
    
    // Code completion settings.
    {
      mforms::Box *subsettings_box = mforms::manage(new mforms::Box(false));
      subsettings_box->set_padding(20, 0, 0, 0);
      {
        mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:CodeCompletionEnabled");
        scoped_connect(check->signal_clicked(), boost::bind(&PreferencesForm::code_completion_changed,
                                                            this, check, subsettings_box));
        
        check->set_text(_("Enable Code Completion in Editors"));
        check->set_tooltip(_("If enabled SQL editors display a code completion list when pressing "
          "the defined hotkey"));
        vbox->add(check, false);
        
      }

      {
        mforms::CheckBox *auto_start_check = new_checkbox_option("DbSqlEditor:AutoStartCodeCompletion");
        auto_start_check->set_text(_("Automatically Start Code Completion"));
        auto_start_check->set_tooltip(_("Available only if code completion is enabled. By activating "
          "this option code completion will be started automatically when you type something and wait "
          "a moment"));
        subsettings_box->add(auto_start_check, false);

        mforms::CheckBox *upper_case_check = new_checkbox_option("DbSqlEditor:CodeCompletionUpperCaseKeywords");
        upper_case_check->set_text(_("Use UPPERCASE keywords on completion"));
        upper_case_check->set_tooltip(_("Normally keywords are shown and inserted as they come from the "
          "code editor configuration file. With this swich they are always upper-cased instead."));
        subsettings_box->add(upper_case_check, false);

        // Set initial enabled state of sub settings depending on whether code completion is enabled.
        std::string value;
        _wbui->get_wb_options_value(_model.is_valid() ? _model.id() : "", "DbSqlEditor:CodeCompletionEnabled", value);
        subsettings_box->set_enabled(atoi(value.c_str()) != 0);

        vbox->add(subsettings_box, false);
      }
    }
    
    {
      mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:ReformatViewDDL");
      check->set_text(_("Reformat DDL for Views"));
      check->set_tooltip(_("Whether to automatically reformat View DDL returned by the server. The MySQL server does not store the formatting information for View definitions."));
      vbox->add(check, false);
    }
    
    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("Max syntax error count:"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("SqlEditor::SyntaxCheck::MaxErrCount", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_("Maximum number of errors for syntax checking.\n"
                           "Syntax errors aren't highlighted beyond this threshold.\n"
                           "Set to 0 to show all errors."));
      tbox->add(entry, false, false);
    }
  }
  
  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("SQL Parsing in Text Editors"));
    box->add(frame, false);
    
    mforms::Box *vbox= mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(4);
    frame->add(vbox);
    
    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("Default SQL_MODE for syntax checker:"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("SqlMode", false);
      entry->set_tooltip(_(
                           "Value of SQL_MODE DBMS session variable customizes the rules and restrictions for SQL syntax and semantics. See MySQL Server reference for details.\n"
                           "This globally defined parameter determines initial value for same named parameter in each newly created model. "
                           "Model scoped same named parameter in its turn affects SQL parsing within the model, and defines the value of SQL_MODE session variable when connecting to DBMS.\n"
                           "Note: Empty value for this parameter will cause Workbench to treat SQL_MODE as empty string when parsing SQL within the model, but will leave DBMS session variable at its default value.\n"
                           "To force Workbench to reset SQL_MODE session variable as well, this parameter needs to be set to a whitespace symbol."));
      tbox->add(entry, true, true);
    }
    
    {
      mforms::CheckBox *check= new_checkbox_option("SqlIdentifiersCS");
      check->set_text(_("SQL Identifiers are Case Sensitive"));
      check->set_tooltip(_(
                           "Whether to treat identifiers separately if their names differ only in letter case."));
      vbox->add(check, false);
    }
    
    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("Non-Standard SQL Delimiter:"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("SqlDelimiter", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_(
                           "SQL statement delimiter different from the normally used one (ie, shouldn't be ;). Change this only if the delimiter you normally use, specially in stored routines, happens to be the current setting."));
      tbox->add(entry, false, false);
    }

    {
      mforms::Box *tbox = mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);

      tbox->add(new_label(_("Comment type for hotkey:"), true), false, false);

      std::string comment_types = "--:--,#:#";
      mforms::Selector *selector = new_selector_option("DbSqlEditor:SQLCommentTypeForHotkey", comment_types, false);
      selector->set_size(150, -1);
      selector->set_tooltip(_("Default comment type for SQL Query editor"));
      tbox->add(selector, false, false);
    }

  }  
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::create_query_page()
{
  // Options specific for the query/script execution aspect of the SQL Editor
  
  mforms::Box *box= mforms::manage(new mforms::Box(false));
  box->set_padding(12);
  box->set_spacing(8);
  
  _tabview.add_page(box, _("SQL Queries"));
  
  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("General"));
    box->add(frame, false);
    
    mforms::Box *vbox= mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(4);
    frame->add(vbox);
    
    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("Max. query length to store in history (in bytes):"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("DbSqlEditor:MaxQuerySizeToHistory", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_(
                           "Queries beyond specified size will not be saved in the history when executed.\n"
                           "Set to 0 to save any executed query or script"));
      tbox->add(entry, false, false);
    }
    
    
    {
      mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:ContinueOnError");
      check->set_text(_("Continue on SQL Script Error (by default)"));
      check->set_tooltip(_(
                           "Whether to continue bypassing failed SQL statements when running script."));
      vbox->add(check, false);
    }
    
    {
      mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:SafeUpdates");
      check->set_text(_("\"Safe Updates\". Forbid UPDATEs and DELETEs with no key in WHERE clause or no LIMIT clause. Requires a reconnection."));
      check->set_tooltip(_(
                           "Enables the SQL_SAFE_UPDATES option for the session.\n"
                           "If enabled, MySQL aborts UPDATE or DELETE statements\n"
                           "that do not use a key in the WHERE clause or a LIMIT clause.\n"
                           "This makes it possible to catch UPDATE or DELETE statements\n"
                           "where keys are not used properly and that would probably change\n"
                           "or delete a large number of rows. \n"
                           "Changing this option requires a reconnection (Query -> Reconnect to Server)"));
      vbox->add(check, false);
    }
    
    {
      mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:AutocommitMode");
      check->set_text(_("Leave autocommit mode enabled by default"));
      check->set_tooltip(_(
                           "Toggles the default autocommit mode for connections.\nWhen enabled, each statement will be committed immediately."
                           "\nNOTE: all query tabs in the same connection share the same transaction. "
                           "To have independent transactions, you must open a new connection."));
      vbox->add(check, false);
    }

    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("Progress status update interval (in milliseconds):"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("DbSqlEditor:ProgressStatusUpdateInterval", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_(
                           "Time interval between UI updates when running SQL script."));
      tbox->add(entry, false, false);
    }
  }
  
  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Online DDL"));
    box->add(frame, false);

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(4);
    frame->add(vbox);

    {
      mforms::Box *line_box = mforms::manage(new mforms::Box(true));
      line_box->set_spacing(4);
      vbox->add(line_box, false);

      mforms::Label *label = new_label(_("Default algorithm for ALTER table:"), true);
      label->set_size(180, -1);
      line_box->add(label, false, false);

      std::string algorithms = "Default:DEFAULT,In place:INPLACE,Copy:COPY";    
      mforms::Selector *selector = new_selector_option("DbSqlEditor:OnlineDDLAlgorithm", algorithms, false);
      selector->set_size(150, -1);
      selector->set_tooltip(_("If the currently connected server supports online DDL then use the selected "
        "algorithm as default. This setting can also be adjusted for each alter operation."));
      line_box->add(selector, false, false);
    }
    {
      mforms::Box *line_box = mforms::manage(new mforms::Box(true));
      line_box->set_spacing(4);
      vbox->add(line_box, false);

      mforms::Label *label = new_label(_("Default lock for ALTER table:"), true);
      label->set_size(180, -1);
      line_box->add(label, false, false);

      std::string locks = "Default:DEFAULT,None:NONE,Shared:SHARED,Exclusive:EXCLUSIVE";    
      mforms::Selector *selector = new_selector_option("DbSqlEditor:OnlineDDLLock", locks, false);
      selector->set_size(150, -1);
      selector->set_tooltip(_("If the currently connected server supports online DDL then use the selected "
        "lock as default. This setting can also be adjusted for each alter operation."));
      line_box->add(selector, false, false);
    }
  }

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Query Results"));
    box->add(frame, false);
    
    mforms::Box *vbox= mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(4);
    frame->add(vbox);
    
    {
      mforms::CheckBox *check= new_checkbox_option("SqlEditor:LimitRows");
      check->set_text(_("Limit Rows"));
      check->set_tooltip(_(
                           "Whether every select query to be implicitly adjusted to limit result set to specified number of rows by appending the LIMIT keyword to the query.\n"
                           "If enabled it's still possible to load entire result set by pressing \"Fetch All\" button."));
      vbox->add(check, false);
    }
    
    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("Limit Rows Count:"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("SqlEditor:LimitRowsCount", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_(
                           "Every select query to be implicitly adjusted to limit result set to specified number of rows."));
      tbox->add(entry, false, false);
    }
    
    {
      mforms::Box *tbox= mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);
      
      tbox->add(new_label(_("Max. Field Value Length to Display (in bytes):"), true), false, false);
      mforms::TextEntry *entry= new_entry_option("Recordset:FieldValueTruncationThreshold", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_(
                           "Symbols beyond specified threashold will be truncated when showing in the grid. Doesn't affect editing field values.\n"
                           "Set to -1 to disable truncation."));
      tbox->add(entry, false, false);
    }
    
    {
      mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:MySQL:TreatBinaryAsText");
      check->set_text(_("Treat BINARY/VARBINARY as nonbinary character string"));
      check->set_tooltip(_(
                           "Whether to treat binary byte strings as nonbinary character strings.\n"
                           "Binary byte string values do not appear in results grid and are marked as a BLOB values that are supposed to be viewed/edited by means of BLOB editor.\n"
                           "Nonbinary character string values are shown right in results grid and can be edited with either cell's in-place editor or BLOB editor.\n"
                           "Warning: Since binary byte strings tend to contain zero-bytes in their values, turning this option on may lead to data truncation when viewing/editing.\n"
                           "Note: Application restart is needed to get new option value in affect."));
      vbox->add(check, false);
    }
    
    {
      mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:IsDataChangesCommitWizardEnabled");
      check->set_text(_("Confirm Data Changes"));
      check->set_tooltip(_("Whether to show a dialog confirming changes to be made to a table recordset."));
      vbox->add(check, false);
    }
    
    /*{
     mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:IsLiveObjectAlterationWizardEnabled");
     check->set_text(_("Enable Live Object Alteration Wizard"));
     check->set_tooltip(_(
     "Whether to use wizard providing more control over applying changes to live database object."));
     vbox->add(check, false);
     }*/
  }
}


//--------------------------------------------------------------------------------------------------

/**
 * Triggered when the user switches the code completion enabled state. We use this to adjust the enabled
 * state for dependent sub options.
 */
void PreferencesForm::code_completion_changed(mforms::CheckBox *cc_box, mforms::Box *subsettings_box)
{
  subsettings_box->set_enabled(cc_box->get_active());
}

//--------------------------------------------------------------------------------------------------
static void force_checkbox_on_toggle(mforms::CheckBox *value, mforms::CheckBox *target, bool same_value, bool disable_on_active)
{
  if (value->get_active())
  {
    target->set_active(!same_value);
    target->set_enabled(!disable_on_active);
  }
  else
  {
    //    target->set_active(same_value);
    target->set_enabled(disable_on_active);
  }
}


void PreferencesForm::create_general_page()
{
  mforms::Box *top_box = mforms::manage(new mforms::Box(false));
  top_box->set_padding(12);
  top_box->set_spacing(8);

  OptionTable *table;

  table = mforms::manage(new OptionTable(this, _("EER Modeler"), true));
  top_box->add(table, false, true);
  {
    table->add_checkbox_option("workbench.AutoReopenLastModel", _("Automatically reopen previous model at start"), "");
    
#ifndef __APPLE__
    table->add_checkbox_option("workbench:ForceSWRendering", _("Force use of software based rendering for EER diagrams"), 
                               _("Enable this option if you have drawing problems in Workbench modeling.\nYou must restart Workbench for the option to take effect."));
#endif
    
    {    
      mforms::TextEntry *entry= new_numeric_entry_option("workbench:UndoEntries", 1, 500);
      entry->set_max_length(5);
      entry->set_size(100, -1);
      
      table->add_option(entry, _("Model undo history size:"), 
                        _("Allowed values are from 1 up.\nNote: using high values (> 100) will increase memory usage\nand slow down operation."));
    }
    
    {
      static const char *auto_save_intervals= "disable:0,10 seconds:10,15 seconds:15,30 seconds:30,1 minute:60,5 minutes:300,10 minutes:600,20 minutes:1200";    
      mforms::Selector *sel = new_selector_option("workbench:AutoSaveModelInterval", auto_save_intervals, true);
      
      table->add_option(sel, _("Auto-save model interval:"), 
                        _("Interval to perform auto-saving of the open model.\nThe model will be restored from the last auto-saved version\nif Workbench unexpectedly quits."));
    }
  }

  table = mforms::manage(new OptionTable(this, _("SQL Editor"), true));
  top_box->add(table, false, true);
  {
    mforms::CheckBox *save_workspace, *discard_unsaved;
    
    save_workspace = table->add_checkbox_option("workbench:SaveSQLWorkspaceOnClose",
                               _("Save snapshot of open editors on close"),
                               _("A snapshot of all open scripts is saved when the SQL Editor is closed. Next time it is opened to the same connection that state is restored. Unsaved files will remain unsaved, but their contents will be preserved."));

    {
      static const char *auto_save_intervals= "disable:0,10 seconds:10,15 seconds:15,30 seconds:30,1 minute:60,5 minutes:300,10 minutes:600,20 minutes:1200";    
      mforms::Selector *sel = new_selector_option("workbench:AutoSaveScriptsInterval", auto_save_intervals, true);
      
      table->add_option(sel, _("Auto-save scripts interval:"), 
                        _("Interval to perform auto-saving of all open script tabs.\nThe scripts will be restored from the last auto-saved version\nif Workbench unexpectedly quits."));
    }
    
    discard_unsaved = table->add_checkbox_option("DbSqlEditor:DiscardUnsavedQueryTabs",
                               _("Create new tabs as Query tabs instead of File"),
                               _("Unsaved Query tabs do not get a close confirmation, unlike File tabs.\nHowever, once saved, such tabs will also get unsaved change confirmations.\n"
                                 "If Snapshot saving is enabled, query tabs are always autosaved to temporary files when the connection is closed."));
    save_workspace->signal_clicked()->connect(boost::bind(force_checkbox_on_toggle, save_workspace, discard_unsaved, true, true));
    (*save_workspace->signal_clicked())();
  }
  
  table = mforms::manage(new OptionTable(this, _("Others"), true));
  top_box->add(table, false, true);
  {
#ifdef _WIN32
    table->add_checkbox_option("DisableSingleInstance", _("Allow more than one instance of MySQL Workbench to run"), 
      _("By default only one instance of MySQL Workbench can run at the same time.\nThis is more resource friendly "
        "and necessary as multiple instances share the same files (settings etc.). Change at your own risk."));
#endif

    mforms::Selector *combo= new_selector_option("grtshell:ShellLanguage");
    table->add_option(combo, _("Interactive GRT Shell language:"), 
                      _("Select the language to use in the interactive GRT shell.\n"
                        "Scripts, modules and plugins will work regardless of this setting.\n"
                        "This option requires a restart."));

    mforms::TextEntry *entry= new_entry_option("workbench:InternalSchema", false);
    entry->set_max_length(100);
    entry->set_size(100, -1);
      
    table->add_option(entry, _("Internal Workbench Schema:"), 
                      _("This schema will be used by Workbench.\nto store information required on\ncertain operations."));

  }

  _tabview.add_page(top_box, _("General"));
}


void PreferencesForm::create_model_page()
{
  mforms::Box *box= mforms::manage(new mforms::Box(false));
  box->set_padding(12);
  box->set_spacing(8);

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Column Defaults"));

    mforms::Table *table= mforms::manage(new mforms::Table());

    table->set_padding(12);
    table->set_column_spacing(4);
    table->set_row_spacing(8);
    
    table->set_column_count(4);
    table->set_row_count(2);

    frame->add(table);

    mforms::TextEntry *entry;
    
    table->add(new_label(_("PK Column Name:"), true), 0, 1, 0, 1, mforms::HFillFlag);
    entry= new_entry_option("PkColumnNameTemplate", false);
    entry->set_tooltip(_("Substitutions:\n"
                         "%table% - name of the table\n"
                         "May be used as %table|upper%, %table|lower%, %table|capitalize%, %table|uncapitalize%"));
    table->add(entry, 1, 2, 0, 1, mforms::HFillFlag|mforms::HExpandFlag);

    table->add(new_label(_("PK Column Type:"), true), 2, 3, 0, 1, mforms::HFillFlag);
    entry= new_entry_option("DefaultPkColumnType", false);
    entry->set_tooltip(_("Default type for use in newly added primary key columns.\nSpecify a column type name or a user defined type.\nFlags such as UNSIGNED are not accepted."));
    table->add(entry, 3, 4, 0, 1, mforms::HFillFlag|mforms::HExpandFlag);


    table->add(new_label(_("Column Name:"), true), 0, 1, 1, 2, mforms::HFillFlag);
    entry= new_entry_option("ColumnNameTemplate", false);
    entry->set_tooltip(_("Substitutions:\n"
                         "%table% - name of the table"));
    table->add(entry, 1, 2, 1, 2, mforms::HFillFlag|mforms::HExpandFlag);
    
    table->add(new_label(_("Column Type:"), true), 2, 3, 1, 2, mforms::HFillFlag);
    entry= new_entry_option("DefaultColumnType", false);
    entry->set_tooltip(_("Default type for use in newly added columns.\nSpecify a column type name or a user defined type.\nFlags such as UNSIGNED are not accepted."));
    table->add(entry, 3, 4, 1, 2, mforms::HFillFlag|mforms::HExpandFlag);
    
    box->add(frame, false);
  }

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Foreign Key/Relationship Defaults"));

    mforms::Table *table= mforms::manage(new mforms::Table());

    table->set_padding(8);

    frame->add(table);
    
    table->set_row_spacing(8);
    table->set_column_spacing(8);
    table->set_row_count(3);
    table->set_column_count(4);

    mforms::TextEntry *entry;

    table->add(new_label(_("FK Name:"), true), 0, 1, 0, 1, mforms::HFillFlag);
    entry= new_entry_option("FKNameTemplate", false);
    
#define SUBS_HELP\
  _("Substitutions:\n"\
      "%table%, %stable% - name of the source table\n"\
      "%dtable% - name of the destination table (where FK is added)\n"\
      "%column%, %scolumn% - name of the source column\n"\
      "%dcolumn% - name of the destination column\n"\
      "May be used as %table|upper%, %table|lower%, %table|capitalize% or %table|uncapitalize%"\
    )
    
    entry->set_tooltip(SUBS_HELP);
    table->add(entry, 1, 2, 0, 1, mforms::HFillFlag|mforms::HExpandFlag);

    table->add(new_label(_("Column Name:"), true), 2, 3, 0, 1, mforms::HFillFlag);
    entry= new_entry_option("FKColumnNameTemplate", false);
    entry->set_tooltip(SUBS_HELP);
    table->add(entry, 3, 4, 0, 1, mforms::HFillFlag|mforms::HExpandFlag);

    table->add(new_label(_("ON UPDATE:"), true), 0, 1, 1, 2, mforms::HFillFlag);
    table->add(new_selector_option("db.ForeignKey:updateRule"), 1, 2, 1, 2, mforms::HFillFlag|mforms::HExpandFlag);

    table->add(new_label(_("ON DELETE:"), true), 2, 3, 1, 2, mforms::HFillFlag);
    table->add(new_selector_option("db.ForeignKey:deleteRule"), 3, 4, 1, 2, mforms::HFillFlag|mforms::HExpandFlag);


    table->add(new_label(_("Associative Table Name:"), true), 0, 1, 2, 3, mforms::HFillFlag);
    entry= new_entry_option("AuxTableTemplate", false);
    entry->set_tooltip(_("Substitutions:\n"
                         "%stable% - name of the source table\n"
                         "%dtable% - name of the destination table"));
    table->add(entry, 1, 2, 2, 3, mforms::HFillFlag|mforms::HExpandFlag);

    table->add(new_label(_("for n:m relationships")), 2, 4, 2, 3, mforms::HFillFlag);

    box->add(frame, false);
  }

  _tabview.add_page(box, _("Model"));
}


static void show_target_version(const workbench_physical_ModelRef &model, mforms::TextEntry *entry)
{
  if (*model->catalog()->version()->releaseNumber() < 0)
    entry->set_value(base::strfmt("%li.%li", *model->catalog()->version()->majorNumber(),
                                  *model->catalog()->version()->minorNumber()));
  else
    entry->set_value(base::strfmt("%li.%li.%li", *model->catalog()->version()->majorNumber(),
                   *model->catalog()->version()->minorNumber(), *model->catalog()->version()->releaseNumber()));
}

static void update_target_version(workbench_physical_ModelRef model, mforms::TextEntry *entry)
{
  GrtVersionRef version = bec::parse_version(model.get_grt(), entry->get_string_value());
  model->catalog()->version(version);
  version->owner(model);
}


void PreferencesForm::create_mysql_page()
{
  mforms::Box *box= mforms::manage(new mforms::Box(false));
  box->set_padding(12);
  box->set_spacing(8);

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Model"));

    mforms::Table *table= mforms::manage(new mforms::Table());

    table->set_padding(8);

    frame->add(table);
    table->set_row_count(2);
    table->set_column_count(2);
    
    if (!_model.is_valid())
    {
      table->add(new_label(_("Default Target MySQL Version:"), true), 0, 1, 0, 1, 0);
      table->add(new_selector_option("DefaultTargetMySQLVersion"), 1, 2, 0, 1, mforms::HExpandFlag|mforms::HFillFlag);
    }
    else
    {
      // if editing model options, display the catalog version
      Option *option= new Option();
      mforms::TextEntry *entry= new mforms::TextEntry();
      
      option->view= mforms::manage(entry);
      option->show_value= boost::bind(show_target_version, _model, entry);
      option->update_value= boost::bind(update_target_version, _model, entry);

      option->view= mforms::manage(entry);
      option->show_value= boost::bind(show_target_version, _model, entry);
      option->update_value= boost::bind(update_target_version, _model, entry);
      _options.push_back(option);
      
      table->add(new_label(_("Target MySQL Version:"), true), 0, 1, 0, 1, 0);
      table->add(entry, 1, 2, 0, 1, mforms::HExpandFlag|mforms::HFillFlag);
    }
    box->add(frame, false);
  }

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Model Table Defaults"));
    
    mforms::Box *tbox= mforms::manage(new mforms::Box(true));
    
    tbox->set_padding(8);
    
    frame->add(tbox);
    
    tbox->add(new_label(_("Default Storage Engine:"), true), false, false);
    tbox->add(new_selector_option("db.mysql.Table:tableEngine"), true, true);
    
    box->add(frame, false);
  }

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Forward Engineering and Synchronization"));
    
    mforms::Box *tbox= mforms::manage(new mforms::Box(true));
    mforms::TextEntry *entry;
    tbox->set_padding(8);
    
    frame->add(tbox);
    tbox->add(new_label(_("SQL_MODE to be used in generated scripts:"), true), false, false);    
    tbox->add(entry = new_entry_option("SqlGenerator.Mysql:SQL_MODE", false), true, true);
    entry->set_tooltip(_("The default value of TRADITIONAL is recommended."));
    
    box->add(frame, false);
  }

  _tabview.add_page(box, _("Model: MySQL"));
}



void PreferencesForm::create_diagram_page()
{
  mforms::Box *box= mforms::manage(new mforms::Box(false));
  box->set_padding(12);
  box->set_spacing(8);


  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("All Objects"));

    mforms::Box *vbox= mforms::manage(new mforms::Box(false));

    vbox->set_padding(8);
    vbox->set_spacing(4);

    frame->add(vbox);
    
    mforms::CheckBox *check;

    check= new_checkbox_option("workbench.physical.ObjectFigure:Expanded");
    check->set_text(_("Expand New Objects"));
    check->set_tooltip(_("Set the initial state of newly created objects to expanded (or collapsed)"));
    vbox->add(check, false);

    check= new_checkbox_option("SynchronizeObjectColors");
    check->set_text(_("Propagate Object Color Changes to All Diagrams"));
    check->set_tooltip(_("If an object figure's color is changed, all figures in all diagrams that represent the same object are also updated"));
    vbox->add(check, false);
    
    box->add(frame, false);
  }

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Tables"));

    mforms::Box *vbox= mforms::manage(new mforms::Box(false));

    vbox->set_padding(8);
    vbox->set_spacing(4);

    frame->add(vbox);
    
    mforms::CheckBox *check;

    check= new_checkbox_option("workbench.physical.TableFigure:ShowColumnTypes");
    check->set_text(_("Show Column Types"));
    check->set_tooltip(_("Show the column types along their names in table figures"));
    vbox->add(check, false);

    check= new_checkbox_option("workbench.physical.TableFigure:ShowSchemaName");
    check->set_text(_("Show Schema Name"));
    check->set_tooltip(_("Show owning schema name in the table titlebar figures"));
    vbox->add(check, false);

    {
      mforms::Box *hbox= mforms::manage(new mforms::Box(true));
      mforms::TextEntry *entry= new_entry_option("workbench.physical.TableFigure:MaxColumnTypeLength", true);
      
      hbox->set_spacing(4);
      
      //label->set_size(200, -1);
      entry->set_max_length(5);
      entry->set_size(50, -1);
      
      hbox->add(new_label(_("Max. Length of ENUMs and SETs to Display:"), true), false, false);
      hbox->add(entry, false);
      
      vbox->add(hbox, false);
    }    

    check= new_checkbox_option("workbench.physical.TableFigure:ShowColumnFlags");
    check->set_text(_("Show Column Flags"));
    check->set_tooltip(_("Show column flags such as NOT NULL or UNSIGNED along their names in table figures"));
    vbox->add(check, false);

    {
      mforms::Box *hbox= mforms::manage(new mforms::Box(true));
      mforms::TextEntry *entry= new_entry_option("workbench.physical.TableFigure:MaxColumnsDisplayed", true);
      mforms::Label *descr= mforms::manage(new mforms::Label());
    
      hbox->set_spacing(4);

      //label->set_size(200, -1);
      entry->set_max_length(5);
      entry->set_size(50, -1);
      descr->set_text(_("Larger tables will be truncated."));
      descr->set_style(mforms::SmallHelpTextStyle);

      hbox->add(new_label(_("Max. Number of Columns to Display:"), true), false, false);
      hbox->add(entry, false);
      hbox->add(descr, true, true);

      vbox->add(hbox, false);
    }
    
    box->add(frame, false);
  }


  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Routines"));

    mforms::Box *hbox= mforms::manage(new mforms::Box(true));

    hbox->set_padding(8);
    hbox->set_spacing(4);

    frame->add(hbox);
    
    mforms::TextEntry *entry;

    hbox->add(new_label(_("Trim Routine Names Longer Than")), false);
    
    entry= new_entry_option("workbench.physical.RoutineGroupFigure:MaxRoutineNameLength", true);
    entry->set_size(60, -1);
    entry->set_max_length(3);
    hbox->add(entry, false);

    hbox->add(new_label(_("characters")), false);

    box->add(frame, false);
  }


  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Relationships/Connections"));

    mforms::Box *vbox= mforms::manage(new mforms::Box(false));

    vbox->set_padding(8);
    vbox->set_spacing(4);

    frame->add(vbox);
    
    mforms::CheckBox *check;

    check= new_checkbox_option("workbench.physical.Diagram:DrawLineCrossings");
    check->set_text(_("Draw Line Crossings (slow in large diagrams)"));
    vbox->add(check, false);

    check= new_checkbox_option("workbench.physical.Connection:HideCaptions");
    check->set_text(_("Hide Captions"));
    vbox->add(check, false);

    check= new_checkbox_option("workbench.physical.Connection:CenterCaptions");
    check->set_text(_("Center Captions Over Line"));
    vbox->add(check, false);

    box->add(frame, false);
  }

  _tabview.add_page(box, _("Diagram"));
}



static void show_text_option(grt::DictRef options, const std::string &option_name, mforms::TextBox *text)
{
  text->set_value(options.get_string(option_name));
}


static void update_text_option(grt::DictRef options, const std::string &option_name, mforms::TextBox *text)
{
  options.gset(option_name, text->get_string_value());
}



void PreferencesForm::change_font_option(const std::string &option, const std::string &value)
{
  std::vector<std::string>::const_iterator it;
  if ((it = std::find(_font_options.begin(), _font_options.end(), option)) != _font_options.end())
  {
    int i = (int)(it - _font_options.begin());
    _font_list.node_at_row(i)->set_string(1, value);
  }
}


void PreferencesForm::font_preset_changed()
{
  int i = _font_preset.get_selected_index();

  if (i >= 0)
  {
    change_font_option("workbench.physical.TableFigure:TitleFont", font_sets[i].object_title_font);
    change_font_option("workbench.physical.TableFigure:SectionFont", font_sets[i].object_section_font);
    change_font_option("workbench.physical.TableFigure:ItemsFont", font_sets[i].object_item_font);
    change_font_option("workbench.physical.ViewFigure:TitleFont", font_sets[i].object_title_font);
    change_font_option("workbench.physical.RoutineGroupFigure:TitleFont", font_sets[i].object_title_font);
    change_font_option("workbench.physical.RoutineGroupFigure:ItemsFont", font_sets[i].object_item_font);
    change_font_option("workbench.physical.Connection:CaptionFont", font_sets[i].object_item_font);
    change_font_option("workbench.physical.Layer:TitleFont", font_sets[i].object_item_font);
    change_font_option("workbench.model.NoteFigure:TextFont", font_sets[i].object_item_font);
  }
}


void PreferencesForm::create_appearance_page()
{
  mforms::Box *box= mforms::manage(new mforms::Box(false));
  box->set_padding(12);
  box->set_spacing(8);

  
  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Color Presets"));

    mforms::Table *table= mforms::manage(new mforms::Table());

    table->set_padding(8);
    table->set_row_spacing(4);
    table->set_column_spacing(4);    
    table->set_row_count(2);
    table->set_column_count(2);

    frame->add(table);

    mforms::TextBox *text;
    
    table->add(new_label(_("Colors available for tables, views etc")), 0, 1, 0, 1, mforms::HFillFlag);
    text= mforms::manage(new mforms::TextBox(mforms::VerticalScrollBar));
    text->set_size(200, 100);
    table->add(text, 0, 1, 1, 2, mforms::FillAndExpand);
    
    Option *option= new Option();
    _options.push_back(option);
    option->view= text;
    option->show_value= boost::bind(show_text_option, get_options(), "workbench.model.ObjectFigure:ColorList", text);
    option->update_value= boost::bind(update_text_option, get_options(), "workbench.model.ObjectFigure:ColorList", text);
    
    table->add(new_label(_("Colors available for layers, notes etc")), 1, 2, 0, 1, mforms::HFillFlag);
    text= mforms::manage(new mforms::TextBox(mforms::VerticalScrollBar));
    text->set_size(200, 100);
    table->add(text, 1, 2, 1, 2, mforms::FillAndExpand);

    option= new Option();
    _options.push_back(option);
    option->view= text;
    option->show_value= boost::bind(&show_text_option, get_options(), "workbench.model.Figure:ColorList", text);
    option->update_value= boost::bind(&update_text_option, get_options(), "workbench.model.Figure:ColorList", text);


    box->add(frame, false);
  }

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Fonts"));

    mforms::Box *content = mforms::manage(new mforms::Box(false));
    content->set_padding(5);
    frame->add(content);

    mforms::Box *hbox = mforms::manage(new mforms::Box(true));
    content->add(hbox, false, true);
    hbox->set_spacing(12);
    hbox->set_padding(12);

    _font_preset.signal_changed()->connect(boost::bind(&PreferencesForm::font_preset_changed, this));
    for (size_t i = 0; font_sets[i].name; i++)
      _font_preset.add_item(font_sets[i].name);

    hbox->add(mforms::manage(new mforms::Label("Configure Fonts For:")), false, true);
    hbox->add(&_font_preset, true, true);

    _font_list.add_column(mforms::StringColumnType, _("Location"), 150, false);
    _font_list.add_column(mforms::StringColumnType, _("Font"), 150, true);
    _font_list.end_columns();

    content->add(&_font_list, true, true);
    box->add(frame, true, true);
  }


  _tabview.add_page(box, _("Appearance"));
}

#ifdef _WIN32
/**
 * Windows specific theming and colors page.
 */
void PreferencesForm::create_color_scheme_page()
{
  Box* content = manage(new Box(false));
  content->set_padding(12);
  content->set_spacing(8);

  mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
  frame->set_title(_("Color Scheme"));
  mforms::Box *hbox = mforms::manage(new mforms::Box(true));
  hbox->add(new_label(_("Select your scheme:")), false, false);
  mforms::Selector *selector = new_selector_option("ColorScheme", "", true);
  selector->set_size(250, -1);
  hbox->add(selector, true, false);
  hbox->add(new_label(_("The scheme that determines the core colors."), false, true), true, false);

  frame->add(hbox);
  content->add(frame, false, true);
  _tabview.add_page(content, _("Theming"));
}

#endif

static std::string separate_camel_word(const std::string &word)
{
  std::string result;

  for (std::string::const_iterator c= word.begin(); c != word.end(); ++c)
  {
    if (!result.empty() && *c >= 'A' && *c <= 'Z')
      result.append(" ");
    result.append(1, *c);
  }
  
  return result;
}


void PreferencesForm::show_colors_and_fonts()
{
  std::vector<std::string> options= _wbui->get_wb_options_keys("");

  _font_options.clear();
  _font_list.clear();
  
  for (std::vector<std::string>::const_iterator iter= options.begin();
       iter != options.end(); ++iter)
  {
    if (bec::has_suffix(*iter, "Font") && bec::has_prefix(*iter, "workbench."))
    {
      std::string::size_type pos= iter->find(':');
      
      if (pos != std::string::npos)
      {
        try
        {
          std::string part= iter->substr(pos + 1);
          std::string figure= base::split(iter->substr(0, pos), ".")[2];
          std::string caption;

          part= part.substr(0, part.length() - 4);

          // substitute some figure names
          figure= bec::replace_string(figure, "NoteFigure", "TextFigure");
          
          caption= separate_camel_word(figure) + " " + part;
          
          mforms::TreeNodeRef node= _font_list.add_node();
          std::string value;
          _wbui->get_wb_options_value("", *iter, value);
          node->set_string(0, caption);
          node->set_string(1, value);
          
          _font_options.push_back(*iter);
        }
        catch (...)
        {
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::update_colors_and_fonts()
{
  for (int c= _font_list.count(), i= 0; i < c; i++)
  {
    std::string value= _font_list.root_node()->get_child(i)->get_string(1);

    _wbui->set_wb_options_value("", _font_options[i], value);
  }
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::toggle_use_global()
{
  _tabview.set_enabled(!_use_global.get_active());
}

//--------------------------------------------------------------------------------------------------

static struct RegisterPrefsNotifDocs
{
  RegisterPrefsNotifDocs()
  {
    base::NotificationCenter::get()->register_notification("GRNPreferencesDidCreate",
                                                           "preferences",
                                                           "Sent when the Preferences window is created.",
                                                           "",
                                                           "options - the options dictionary being edited\n"
                                                           "or\n"
                                                           "model-options - the model specific options dictionary being changed\n"
                                                           "model-id - the object id of the model for which the options are being changed");
    
    base::NotificationCenter::get()->register_notification("GRNPreferencesWillOpen",
                                                           "preferences",
                                                           "Sent when Preferences window is about to be shown on screen.",
                                                           "",
                                                           "options - the options dictionary being edited\n"
                                                           "or\n"
                                                           "model-options - the model specific options dictionary being changed\n"
                                                           "model-id - the object id of the model for which the options are being changed");    
    
    base::NotificationCenter::get()->register_notification("GRNPreferencesDidClose",
                                                           "preferences",
                                                           "Sent after Preferences window was closed.",
                                                           "",
                                                           "saved - 1 if the user chose to save the options changed or 0 if changes were cancelled\n"
                                                           "options - the options dictionary being edited\n"
                                                           "or\n"
                                                           "model-options - the model specific options dictionary being changed\n"
                                                           "model-id - the object id of the model for which the options are being changed\n");
  }
} initdocs;


