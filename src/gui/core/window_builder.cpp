/*
   Copyright (C) 2008 - 2017 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/core/window_builder.hpp"

#include "formula/string_utils.hpp"
#include "gettext.hpp"
#include "gui/core/log.hpp"
#include "gui/core/window_builder/helper.hpp"
#include "gui/core/window_builder/instance.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/drawing.hpp"
#include "gui/widgets/horizontal_scrollbar.hpp"
#include "gui/widgets/image.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/matrix.hpp"
#include "gui/widgets/menu_button.hpp"
#include "gui/widgets/minimap.hpp"
#include "gui/widgets/pane.hpp"
#include "gui/widgets/password_box.hpp"
#include "gui/widgets/repeating_button.hpp"
#include "gui/widgets/scroll_label.hpp"
#include "gui/widgets/scrollbar_panel.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/size_lock.hpp"
#include "gui/widgets/slider.hpp"
#include "gui/widgets/stacked_widget.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/unit_preview_pane.hpp"
#include "gui/widgets/vertical_scrollbar.hpp"
#include "gui/widgets/viewport.hpp"
#include "gui/widgets/window.hpp"
#include "wml_exception.hpp"

#include "utils/functional.hpp"

namespace gui2
{
static std::map<std::string, widget_builder_func_t>& builder_widget_lookup()
{
	static std::map<std::string, widget_builder_func_t> result;
	return result;
}

/*WIKI
 * @page = GUIWidgetInstanceWML
 * @order = 1
 *
 * {{Autogenerated}}
 *
 * = Widget instance =
 *
 * Inside a grid (which is inside all container widgets) a widget is
 * instantiated. With this instantiation some more variables of a widget can
 * be tuned. This page will describe what can be tuned.
 *
 */
window* build(CVideo& video, const builder_window::window_resolution* definition)
{
	// We set the values from the definition since we can only determine the
	// best size (if needed) after all widgets have been placed.
	window* win = new window(video,
		definition->x,
		definition->y,
		definition->width,
		definition->height,
		definition->reevaluate_best_size,
		definition->functions,
		definition->automatic_placement,
		definition->horizontal_placement,
		definition->vertical_placement,
		definition->maximum_width,
		definition->maximum_height,
		definition->definition,
		definition->tooltip,
		definition->helptip
	);

	assert(win);

	for(const auto& lg : definition->linked_groups) {
		if(win->has_linked_size_group(lg.id)) {
			t_string msg = vgettext("Linked '$id' group has multiple definitions.", {{"id", lg.id}});

			FAIL(msg);
		}

		win->init_linked_size_group(lg.id, lg.fixed_width, lg.fixed_height);
	}

	win->set_click_dismiss(definition->click_dismiss);

	const auto conf = win->cast_config_to<window_definition>();
	assert(conf);

	if(conf->grid) {
		win->init_grid(conf->grid);
		win->finalize(definition->grid);
	} else {
		win->init_grid(definition->grid);
	}

	win->add_to_keyboard_chain(win);

	return win;
}

window* build(CVideo& video, const std::string& type)
{
	const builder_window::window_resolution& definition = get_window_builder(type);
	window* window = build(video, &definition);
	window->set_id(type);
	return window;
}

builder_widget::builder_widget(const config& cfg)
	: id(cfg["id"])
	, linked_group(cfg["linked_group"])
	, debug_border_mode(cfg["debug_border_mode"])
	, debug_border_color(decode_color(cfg["debug_border_color"]))
{
}

void register_builder_widget(const std::string& id, widget_builder_func_t functor)
{
	builder_widget_lookup().emplace(id, functor);
}

builder_widget_ptr create_builder_widget(const config& cfg)
{
	config::const_all_children_itors children = cfg.all_children_range();
	VALIDATE(children.size() == 1, "Grid cell does not have exactly 1 child.");

	for(const auto& item : builder_widget_lookup()) {
		if(item.first == "window" || item.first == "tooltip") {
			continue;
		}
		if(const config& c = cfg.child(item.first)) {
			return item.second(c);
		}
	}

	if(const config& c = cfg.child("grid")) {
		return std::make_shared<builder_grid>(c);
	}

	if(const config& instance = cfg.child("instance")) {
		return std::make_shared<implementation::builder_instance>(instance);
	}

	if(const config& pane = cfg.child("pane")) {
		return std::make_shared<implementation::builder_pane>(pane);
	}

	if(const config& viewport = cfg.child("viewport")) {
		return std::make_shared<implementation::builder_viewport>(viewport);
	}

/*
 * This is rather odd, when commented out the classes no longer seem to be in
 * the executable, no real idea why, except maybe of an overzealous optimizer
 * while linking. It seems that all these classes aren't explicitly
 * instantiated but only implicitly. Also when looking at the symbols in
 * libwesnoth-game.a the repeating button is there regardless of this #if but
 * in the final binary only if the #if is enabled.
 *
 * If this code is executed, which it will cause an assertion failure.
 *
 * Its likeley that this happens because some build this as a library file
 * which is then used by the wesnoth executable. For msvc a good try to fix
 * this issue is to add __pragma(comment(linker, "/include:" #TYPE)) or
 * similar in the REGISTER_WIDGET3 macro. For gcc and similar this can only
 * be fixed by using --whole-archive flag when linking this library.
 */
#if 1
#define TRY(name)                                                                                                      \
	do {                                                                                                               \
		if(const config& c = cfg.child(#name)) {                                                                       \
			builder_widget_ptr p = std::make_shared<implementation::builder_##name>(c);                                \
			assert(false);                                                                                             \
		}                                                                                                              \
	} while(0)

	TRY(stacked_widget);
	TRY(scrollbar_panel);
	TRY(horizontal_scrollbar);
	TRY(repeating_button);
	TRY(vertical_scrollbar);
	TRY(label);
	TRY(image);
	TRY(toggle_button);
	TRY(slider);
	TRY(scroll_label);
	TRY(matrix);
	TRY(minimap);
	TRY(button);
	TRY(menu_button);
	TRY(drawing);
	TRY(password_box);
	TRY(unit_preview_pane);
	TRY(size_lock);
#undef TRY
#endif

	// FAIL() doesn't return
	FAIL("Unknown widget type " + cfg.ordered_begin()->key);
}

/*WIKI
 * @page = GUIToolkitWML
 * @order = 1_window
 * @begin{parent}{name="gui/"}
 * = Window definition =
 * @begin{tag}{name="window"}{min="0"}{max="-1"}
 *
 * A window defines how a window looks in the game.
 *
 * @begin{table}{config}
 *     id & string & &               Unique id for this window. $
 *     description & t_string & &    Unique translatable name for this
 *                                   window. $
 *
 *     resolution & section & &      The definitions of the window in various
 *                                   resolutions. $
 * @end{table}
 * @end{tag}{name="window"}
 * @end{parent}{name="gui/"}
 *
 *
 */
void builder_window::read(const config& cfg)
{
	VALIDATE(!id_.empty(), missing_mandatory_wml_key("window", "id"));
	VALIDATE(!description_.empty(), missing_mandatory_wml_key("window", "description"));

	DBG_GUI_P << "Window builder: reading data for window " << id_ << ".\n";

	config::const_child_itors cfgs = cfg.child_range("resolution");
	VALIDATE(!cfgs.empty(), _("No resolution defined."));

	for(const auto& i : cfgs) {
		resolutions.emplace_back(i);
	}
}

/*WIKI
 * @page = GUIToolkitWML
 * @order = 1_window
 * @begin{parent}{name=gui/window/}
 * == Resolution ==
 * @begin{tag}{name="resolution"}{min="0"}{max="-1"}
 * @begin{table}{config}
 * window_width & unsigned & 0 &   Width of the application window. $
 * window_height & unsigned & 0 &  Height of the application window. $
 *
 *
 * automatic_placement & bool & true &
 *     Automatically calculate the best size for the window and place it. If
 *     automatically placed ''vertical_placement'' and ''horizontal_placement''
 *     can be used to modify the final placement. If not automatically placed
 *     the ''width'' and ''height'' are mandatory. $
 *
 *
 * x & f_unsigned & 0 &            X coordinate of the window to show. $
 * y & f_unsigned & 0 &            Y coordinate of the window to show. $
 * width & f_unsigned & 0 &        Width of the window to show. $
 * height & f_unsigned & 0 &       Height of the window to show. $
 *
 * reevaluate_best_size & f_bool & false &
 *     The foo $
 *
 * functions & function & "" &
 *     The function definitions s available for the formula fields in window. $
 *
 * vertical_placement & v_align & "" &
 *     The vertical placement of the window. $
 *
 * horizontal_placement & h_align & "" &
 *     The horizontal placement of the window. $
 *
 *
 * maximum_width & unsigned & 0 &
 *     The maximum width of the window (only used for automatic placement). $
 *
 * maximum_height & unsigned & 0 &
 *     The maximum height of the window (only used for automatic placement). $
 *
 *
 * click_dismiss & bool & false &
 *     Does the window need click dismiss behavior? Click dismiss behavior
 *     means that any mouse click will close the dialog. Note certain widgets
 *     will automatically disable this behavior since they need to process the
 *     clicks as well, for example buttons do need a click and a misclick on
 *     button shouldn't close the dialog. NOTE with some widgets this behavior
 *     depends on their contents (like scrolling labels) so the behavior might
 *     get changed depending on the data in the dialog. NOTE the default
 *     behavior might be changed since it will be disabled when can't be used
 *     due to widgets which use the mouse, including buttons, so it might be
 *     wise to set the behavior explicitly when not wanted and no mouse using
 *     widgets are available. This means enter, escape or an external source
 *     needs to be used to close the dialog (which is valid). $
 *
 *
 * definition & string & "default" &
 *     Definition of the window which we want to show. $
 *
 *
 * linked_group & sections & [] &  A group of linked widget sections. $
 *
 *
 * tooltip & section & &
 *     Information regarding the tooltip for this window. $
 *
 * helptip & section & &
 *     Information regarding the helptip for this window. $
 *
 *
 * grid & grid & &                 The grid with the widgets to show. $
 * @end{table}
 * @begin{tag}{name="linked_group"}{min=0}{max=-1}
 * A linked_group section has the following fields:
 * @begin{table}{config}
 *     id & string & &               The unique id of the group (unique in this
 *                                   window). $
 *     fixed_width & bool & false &  Should widget in this group have the same
 *                                   width. $
 *     fixed_height & bool & false & Should widget in this group have the same
 *                                   height. $
 * @end{table}
 * @end{tag}{name="linked_group"}
 * A linked group needs to have at least one size fixed.
 * @begin{tag}{name="tooltip"}{min=0}{max=1}
 * A tooltip and helptip section have the following field:
 * @begin{table}{config}
 *     id & string & &               The id of the tip to show.
 * Note more fields will probably be added later on.
 * @end{table}{config}
 * @end{tag}{name=tooltip}
 * @begin{tag}{name="foreground"}{min=0}{max=1}
 * @end{tag}{name="foreground"}
 * @begin{tag}{name="background"}{min=0}{max=1}
 * @end{tag}{name="background"}
 * @end{tag}{name="resolution"}
 * @end{parent}{name=gui/window/}
 * @begin{parent}{name=gui/window/resolution/}
 * @begin{tag}{name="helptip"}{min=0}{max=1}{super="gui/window/resolution/tooltip"}
 * @end{tag}{name="helptip"}
 * @end{parent}{name=gui/window/resolution/}
 */
builder_window::window_resolution::window_resolution(const config& cfg)
	: window_width(cfg["window_width"])
	, window_height(cfg["window_height"])
	, automatic_placement(cfg["automatic_placement"].to_bool(true))
	, x(cfg["x"])
	, y(cfg["y"])
	, width(cfg["width"])
	, height(cfg["height"])
	, reevaluate_best_size(cfg["reevaluate_best_size"])
	, functions()
	, vertical_placement(implementation::get_v_align(cfg["vertical_placement"]))
	, horizontal_placement(implementation::get_h_align(cfg["horizontal_placement"]))
	, maximum_width(cfg["maximum_width"])
	, maximum_height(cfg["maximum_height"])
	, click_dismiss(cfg["click_dismiss"].to_bool())
	, definition(cfg["definition"])
	, linked_groups()
	, tooltip(cfg.child_or_empty("tooltip"), "tooltip")
	, helptip(cfg.child_or_empty("helptip"), "helptip")
	, grid(0)
{
	if(!cfg["functions"].empty()) {
		wfl::formula(cfg["functions"], &functions).evaluate();
	}

	const config& c = cfg.child("grid");

	VALIDATE(c, _("No grid defined."));

	grid = std::make_shared<builder_grid>(builder_grid(c));

	if(!automatic_placement) {
		VALIDATE(width.has_formula() || width(), missing_mandatory_wml_key("resolution", "width"));
		VALIDATE(height.has_formula() || height(), missing_mandatory_wml_key("resolution", "height"));
	}

	DBG_GUI_P << "Window builder: parsing resolution " << window_width << ',' << window_height << '\n';

	if(definition.empty()) {
		definition = "default";
	}

	linked_groups = parse_linked_group_definitions(cfg);
}

builder_window::window_resolution::tooltip_info::tooltip_info(const config& cfg, const std::string& tagname)
	: id(cfg["id"])
{
	VALIDATE(!id.empty(), missing_mandatory_wml_key("[window][resolution][" + tagname + "]", "id"));
}

/*WIKI
 * @page = GUIToolkitWML
 * @order = 2_cell
 * @begin{parent}{name="gui/window/resolution/"}
 * = Cell =
 * @begin{tag}{name="grid"}{min="1"}{max="1"}
 * @begin{table}{config}
 *     id & string & "" &      A grid is a widget and can have an id. This isn't
 *                                      used that often, but is allowed. $
 *     linked_group & string & 0 &       $
 * @end{table}
 *
 * Every grid cell has some cell configuration values and one widget in the grid
 * cell. Here we describe the what is available more information about the usage
 * can be found here [[GUILayout]].
 *
 * == Row values ==
 * @begin{tag}{name="row"}{min="0"}{max="-1"}
 * For every row the following variables are available:
 *
 * @begin{table}{config}
 *     grow_factor & unsigned & 0 &      The grow factor for a row. $
 * @end{table}
 *
 * == Cell values ==
 * @begin{tag}{name="column"}{min="0"}{max="-1"}
 * @allow{link}{name="gui/window/resolution/grid"}
 * For every column the following variables are available:
 * @begin{table}{config}
 *     grow_factor & unsigned & 0 &    The grow factor for a column, this
 *                                     value is only read for the first row. $
 *
 *     border_size & unsigned & 0 &    The border size for this grid cell. $
 *     border & border & "" &          Where to place the border in this grid
 *                                     cell. $
 *
 *     vertical_alignment & v_align & "" &
 *                                     The vertical alignment of the widget in
 *                                     the grid cell. (This value is ignored if
 *                                     vertical_grow is true.) $
 *     horizontal_alignment & h_align & "" &
 *                                     The horizontal alignment of the widget in
 *                                     the grid cell.(This value is ignored if
 *                                     horizontal_grow is true.) $
 *
 *     vertical_grow & bool & false &    Does the widget grow in vertical
 *                                     direction when the grid cell grows in the
 *                                     vertical direction. This is used if the
 *                                     grid cell is wider as the best width for
 *                                     the widget. $
 *     horizontal_grow & bool & false &  Does the widget grow in horizontal
 *                                     direction when the grid cell grows in the
 *                                     horizontal direction. This is used if the
 *                                     grid cell is higher as the best width for
 *                                     the widget. $
 * @end{table}
 * @end{tag}{name="column"}
 * @end{tag}{name="row"}
 * @end{tag}{name="grid"}
 * @end{parent}{name="gui/window/resolution/"}
 *
 */
builder_grid::builder_grid(const config& cfg)
	: builder_widget(cfg)
	, rows(0)
	, cols(0)
	, row_grow_factor()
	, col_grow_factor()
	, flags()
	, border_size()
	, widgets()
{
	log_scope2(log_gui_parse, "Window builder: parsing a grid");

	for(const auto& row : cfg.child_range("row")) {
		unsigned col = 0;

		row_grow_factor.push_back(row["grow_factor"]);

		for(const auto& c : row.child_range("column")) {
			flags.push_back(implementation::read_flags(c));
			border_size.push_back(c["border_size"]);
			if(rows == 0) {
				col_grow_factor.push_back(c["grow_factor"]);
			}

			widgets.push_back(create_builder_widget(c));

			++col;
		}

		++rows;
		if(rows == 1) {
			cols = col;
		} else {
			VALIDATE(col, _("A row must have a column."));
			VALIDATE(col == cols, _("Number of columns differ."));
		}
	}

	DBG_GUI_P << "Window builder: grid has " << rows << " rows and " << cols << " columns.\n";
}

grid* builder_grid::build() const
{
	return build(new grid());
}

widget* builder_grid::build(const replacements_map& replacements) const
{
	grid* result = new grid();
	build(*result, replacements);
	return result;
}

grid* builder_grid::build(grid* grid) const
{
	grid->set_id(id);
	grid->set_linked_group(linked_group);
	grid->set_rows_cols(rows, cols);

	log_scope2(log_gui_general, "Window builder: building grid");

	DBG_GUI_G << "Window builder: grid '" << id << "' has " << rows << " rows and " << cols << " columns.\n";

	for(unsigned x = 0; x < rows; ++x) {
		grid->set_row_grow_factor(x, row_grow_factor[x]);

		for(unsigned y = 0; y < cols; ++y) {
			if(x == 0) {
				grid->set_column_grow_factor(y, col_grow_factor[y]);
			}

			DBG_GUI_G << "Window builder: adding child at " << x << ',' << y << ".\n";

			const unsigned int i = x * cols + y;

			widget* widget = widgets[i]->build();
			grid->set_child(widget, x, y, flags[i], border_size[i]);
		}
	}

	return grid;
}

void builder_grid::build(grid& grid, const replacements_map& replacements) const
{
	grid.set_id(id);
	grid.set_linked_group(linked_group);
	grid.set_rows_cols(rows, cols);

	log_scope2(log_gui_general, "Window builder: building grid");

	DBG_GUI_G << "Window builder: grid '" << id << "' has " << rows << " rows and " << cols << " columns.\n";

	for(unsigned x = 0; x < rows; ++x) {
		grid.set_row_grow_factor(x, row_grow_factor[x]);

		for(unsigned y = 0; y < cols; ++y) {
			if(x == 0) {
				grid.set_column_grow_factor(y, col_grow_factor[y]);
			}

			DBG_GUI_G << "Window builder: adding child at " << x << ',' << y << ".\n";

			const unsigned int i = x * cols + y;
			grid.set_child(widgets[i]->build(replacements), x, y, flags[i], border_size[i]);
		}
	}
}

} // namespace gui2

/*WIKI
 * @page = GUIToolkitWML
 * @order = ZZZZZZ_footer
 *
 * [[Category: WML Reference]]
 * [[Category: GUI WML Reference]]
 */

/*WIKI
 * @page = GUIWidgetInstanceWML
 * @order = ZZZZZZ_footer
 *
 * [[Category: WML Reference]]
 * [[Category: GUI WML Reference]]
 *
 */
