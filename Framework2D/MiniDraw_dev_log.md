# MiniDraw Development Log

## 1. Project Scope

This log summarizes the implementation, refactoring, debugging, and polish work completed for the `1_MiniDraw` assignment in `Framework2D`.

The work covered:
- shape class construction for ellipse, polygon, and freehand
- canvas interaction logic and robustness improvements
- style binding and fill support
- UI restructuring
- undo/redo support
- refactoring to remove eraser, export, and framework-level modifications

## 2. Main Goals Achieved

The following features are now implemented or significantly improved:
- `Ellipse`, `Polygon`, and `Freehand` shape classes are available and integrated.
- Shape drawing uses per-shape style snapshots instead of global mutable state.
- Rectangle, ellipse, and polygon support fill rendering.
- The MiniDraw UI is split into a top menu, left tool palette, center canvas, right properties panel, and settings panel.
- Undo/redo works through snapshot-based history rather than only removing the last shape.
- Concave polygons are filled correctly.

The following features were previously implemented but have been removed:
- Eraser tool (removed to simplify the codebase).
- PNG canvas export and export-folder selection (removed because they required modifications to shared framework files `window.h` and `window.cpp`).
- Chinese font loading and file-dialog error suppression (removed as they required changes to `window.cpp` and `ImGuiFileDialog.cpp`).

## 3. Development Timeline

### Stage A: Shape Framework Construction

Reference classes such as `Line` and `Rect` were used as templates to design the remaining shape classes.

Implemented classes:
- `src/assignments/1_MiniDraw/shapes/ellipse.h`
- `src/assignments/1_MiniDraw/shapes/ellipse.cpp`
- `src/assignments/1_MiniDraw/shapes/polygon.h`
- `src/assignments/1_MiniDraw/shapes/polygon.cpp`
- `src/assignments/1_MiniDraw/shapes/freehand.h`
- `src/assignments/1_MiniDraw/shapes/freehand.cpp`

Design decisions:
- `Ellipse` uses drag-defined bounding-box corners and computes center and radii during drawing.
- `Polygon` stores multiple control points and supports left-click vertex placement plus right-click finalization.
- `Freehand` stores a sampled sequence of points while dragging.

### Stage B: Canvas Interaction Logic

`canvas_widget.cpp` was expanded into a full drawing interaction controller.

Main updates:
- mouse press, move, and release logic was rewritten to handle each tool cleanly
- polygon creation uses click-based incremental control-point addition
- freehand drawing records continuous points during drag
- shape creation is committed only when valid
- mouse-capture and canvas-hover behavior were improved for UI coexistence

Key files:
- `src/assignments/1_MiniDraw/canvas_widget.h`
- `src/assignments/1_MiniDraw/canvas_widget.cpp`

### Stage C: Style Model Refactor and Fill Support

The original configuration model was upgraded so each shape owns its style.

Important changes:
- `Shape::Config` changed from byte-style color storage to float RGBA arrays.
- `Shape` now stores `config_` internally.
- Each shape constructor accepts a `Config` snapshot.
- Line width, line color, fill toggle, and fill color are preserved per shape after creation.

Fill rendering added to:
- rectangle
- ellipse
- polygon

Key file:
- `src/assignments/1_MiniDraw/shapes/shape.h`

### Stage D: Ellipse Positioning Fix

The ellipse implementation was corrected so drag start and drag end define the actual bounding box rather than a distorted center-based interpretation.

Result:
- ellipse position and size now match the mouse drag rectangle intuitively

### Stage E: UI Reorganization

The original compact control layout was refactored into a clearer working interface.

New layout:
- top: main menu
- left: tool palette
- center: canvas workspace
- right: properties and history controls
- popup/settings panel: canvas and export settings

Implemented in:
- `src/assignments/1_MiniDraw/minidraw_window.h`
- `src/assignments/1_MiniDraw/minidraw_window.cpp`

### Stage F: Undo/Redo, Keyboard Shortcuts, and Eraser

Undo/redo was replaced with snapshot history to support more than simple last-shape removal.

Supported undoable actions:
- add shape
- erase shape
- clear canvas

Keyboard shortcuts:
- `Ctrl+Z`: undo
- `Ctrl+Y`: redo
- `Ctrl+Shift+Z`: redo
- `Ctrl+E`: export canvas

Eraser implementation:
- new `kEraser` tool added to `Canvas`
- `Shape::hit_test(...)` introduced and implemented for all shapes
- eraser supports click and drag removal
- visual brush overlay added on canvas

Key files:
- `src/assignments/1_MiniDraw/canvas_widget.h`
- `src/assignments/1_MiniDraw/canvas_widget.cpp`
- `src/assignments/1_MiniDraw/shapes/line.cpp`
- `src/assignments/1_MiniDraw/shapes/rect.cpp`
- `src/assignments/1_MiniDraw/shapes/ellipse.cpp`
- `src/assignments/1_MiniDraw/shapes/polygon.cpp`
- `src/assignments/1_MiniDraw/shapes/freehand.cpp`

### Stage G: Polygon Fill Bug Fix

A rendering issue was found when trying to fill concave polygons.

Cause:
- the initial implementation used `AddConvexPolyFilled`, which only works for convex polygons

Fix:
- replaced it with `AddConcavePolyFilled`

Result:
- concave polygons now fill correctly in normal use cases

### Stage H: PNG Export and Export Folder Selection

Canvas export was added using framebuffer capture and PNG encoding.

Implementation details:
- `Window::post_render()` hook was added so canvas capture can happen after the frame is rendered
- `glReadPixels` reads the canvas region from the framebuffer
- `stbi_write_png` writes the exported image
- settings panel includes file name input and export-folder selection
- `ImGuiFileDialog` is used for selecting the destination folder

Key files:
- `include/common/window.h`
- `src/common/window.cpp`
- `src/assignments/1_MiniDraw/minidraw_window.h`
- `src/assignments/1_MiniDraw/minidraw_window.cpp`

### Stage I: Clean Export Without Capturing Settings UI

A bug was found where exporting from the settings window captured the settings UI in the final image.

Fix strategy:
- added a delayed export flow with a preparation frame
- on the actual capture frame, settings and file dialog are temporarily not drawn
- capture happens in `post_render()` after the clean frame is rendered

Result:
- exported image is limited to the canvas content instead of including overlay UI

### Stage J: Chinese Path Support Investigation and Fixes

A major issue remained around export-folder browsing under Chinese paths.

Observed symptoms:
- Chinese path text displayed incorrectly at first
- folder selection failed or behaved inconsistently for Chinese directory names

Fixes applied:
- Windows Chinese-capable fonts were loaded during ImGui initialization from common system fonts such as `msyh`, `simhei`, and `simsun`
- UTF-8 to `std::filesystem::path` conversion helpers were added in MiniDraw
- `ImGuiFileDialog` was switched to build with `USE_STD_FILESYSTEM` in `third_party/CMakeLists.txt`

Relevant files:
- `src/common/window.cpp`
- `src/assignments/1_MiniDraw/minidraw_window.cpp`
- `third_party/CMakeLists.txt`

Current result:
- Chinese directories can now be displayed and selected in the file dialog

### Stage K: File Dialog Console Error Suppression

After Chinese path selection became usable, a new issue remained: the terminal was flooded by messages like:

```text
IGFD : last_write_time: The system cannot find the path specified.
```

Analysis:
- the problem came from `ImGuiFileDialog` metadata probing, not from the selection mechanism itself
- some OneDrive or virtualized entries can be enumerated successfully but still fail `std::filesystem::last_write_time(...)`
- the upstream code caught the exception but printed every failure to the console

Fix:
- replaced throwing metadata queries with non-throwing `std::error_code` overloads
- when metadata cannot be read, the dialog now clears date/size fields instead of logging repeatedly
- replaced `localtime(...)` with `localtime_s(...)` in the edited area to remove the MSVC deprecation warning introduced by rebuilding that file

Edited file:
- `third_party/ImGuiFileDialog/ImGuiFileDialog.cpp`

Result:
- file browsing remains functional
- metadata failure becomes a silent fallback
- console spam is removed

### Stage L: Refactoring — Revert Framework Changes, Remove Eraser and Export

A refactoring pass was performed to ensure all project changes remain within the `src/assignments/1_MiniDraw/` directory. Modifications that had been applied to shared framework files and third-party code were reverted.

Reverted files:
- `include/common/window.h` — removed `virtual void post_render();` declaration
- `src/common/window.cpp` — removed `post_render()` definition and call in `render()`, removed Chinese font loading, removed `#include <array>` and `#include <filesystem>`
- `third_party/ImGuiFileDialog/ImGuiFileDialog.cpp` — restored original `GetFileDateAndSize` with try-catch (reverted non-throwing error_code and `localtime_s` changes)

Removed features:
- **Eraser tool**: the `kEraser` shape type, `set_eraser()`, `erase_shape_at()`, `draw_eraser_overlay()`, eraser tolerance controls, and `Shape::hit_test()` from the base class and all derived shape classes were removed.
- **PNG export**: `post_render()` override, `export_canvas_to_png()`, `request_export_canvas()`, `process_export_directory_dialog()`, the File menu, Ctrl+E shortcut, export settings UI, stb_image_write integration, and ImGuiFileDialog usage were removed from `minidraw_window`.
- **Chinese font loading**: reverted to framework default font handling.
- **File-dialog error suppression**: reverted to upstream ImGuiFileDialog behavior.

Reason for removal:
- PNG export requires a `post_render()` hook in the base `Window` class, which runs after OpenGL rendering but before buffer swap. There is no way to capture framebuffer pixels from within the `draw()` override since ImGui only builds command lists during `draw()` — actual OpenGL rendering happens later in the base class's `render()` method. Without modifying the framework, this feature cannot be implemented.
- Chinese font loading requires changes to `Window::init_gui()` in the framework.
- File-dialog error suppression requires changes to the third-party ImGuiFileDialog source.
- The eraser was removed as a standalone simplification request.

Key files modified:
- `src/assignments/1_MiniDraw/canvas_widget.h`
- `src/assignments/1_MiniDraw/canvas_widget.cpp`
- `src/assignments/1_MiniDraw/minidraw_window.h`
- `src/assignments/1_MiniDraw/minidraw_window.cpp`
- `src/assignments/1_MiniDraw/shapes/shape.h`
- `src/assignments/1_MiniDraw/shapes/line.h` / `line.cpp`
- `src/assignments/1_MiniDraw/shapes/rect.h` / `rect.cpp`
- `src/assignments/1_MiniDraw/shapes/ellipse.h` / `ellipse.cpp`
- `src/assignments/1_MiniDraw/shapes/polygon.h` / `polygon.cpp`
- `src/assignments/1_MiniDraw/shapes/freehand.h` / `freehand.cpp`

Build verification:
- target `1_MiniDraw` compiles and links successfully after all changes

### Stage M: Shape Selection, Editing, and Context-Sensitive Properties Panel

Two major features were added entirely within `src/assignments/1_MiniDraw/`:

**1. Shape selection and editing (Select tool)**

A new `kDefault` / "Select" mode was added to the toolbar. When active:
- clicking on a shape performs top-down hit testing (`find_shape_at`) and selects it
- clicking on empty canvas clears the selection
- dragging a selected shape translates it (with a dead-zone threshold to avoid accidental drags)
- the Properties panel shows the selected shape's current style, rotation, scale controls, and a Delete button
- `Delete` key deletes the selected shape

Selection is indicated by a gold bounding-box overlay (`draw_selection_overlay`).

All editing operations use an edit-transaction model:
- `begin_edit_transaction()` saves a single undo snapshot at the start
- continuous edits (drag, slider) modify the shape in-place
- `end_edit_transaction()` closes the transaction
This avoids creating one undo entry per pixel of drag or per slider tick.

Shape interface additions in `Shape` base class:
- `clone()` — deep copy for undo history
- `hit_test(x, y, tolerance)` — per-shape point picking
- `translate(dx, dy)` — rigid body translation
- `scale(sx, sy, anchor)` — scale relative to anchor
- `rotate(radians, anchor)` — rotate relative to anchor
- `center()`, `bounds_min()`, `bounds_max()` — AABB queries
- `rotation_radians()`, `set_rotation_radians()` — rotation state
- `type_name()` — display string for the Properties panel
- `supports_fill()` — whether the shape accepts fill (Rect, Ellipse, Polygon return true)

Each concrete shape (Line, Rect, Ellipse, Polygon, Freehand) implements all of the above.

Hit-test algorithms per shape:
- **Line / Freehand**: point-to-segment distance against polyline segments
- **Rect**: inverse-rotate point into local space, then AABB test (interior for filled, edge-proximity for outline)
- **Ellipse**: inverse-rotate point, then normalized ellipse equation (interior or annulus)
- **Polygon**: edge distance + ray-casting point-in-polygon for filled shapes

Rotation-aware rendering:
- `Rect::draw()` computes rotated corner vertices and draws via `AddQuad` / `AddQuadFilled`
- `Ellipse::draw()` passes `rotation_radians_` to ImGui's `AddEllipse` / `AddEllipseFilled`
- `Polygon` and `Freehand` store already-rotated vertex data

**2. Context-sensitive Properties panel**

The Properties panel now adapts to the active tool:
- **Select mode (`kDefault`)**: if a shape is selected, shows its editable style (line color, thickness, fill toggle + fill color if applicable, rotation slider, scale buttons, delete). If nothing is selected, shows an instructional message.
- **Drawing tools (Line, Rect, Ellipse, Polygon, Freehand)**: shows the "Next Shape Style" editor that only includes controls relevant to the tool — Line and Freehand hide the fill checkbox; Rect, Ellipse, and Polygon show it.

A shared `draw_config_editor()` helper handles both selection editing and pending-style editing, with an `apply_to_selection` flag controlling whether changes invoke edit transactions on the selected shape.

Key files modified:
- `src/assignments/1_MiniDraw/shapes/shape.h` — added `clone`, `hit_test`, `translate`, `scale`, `rotate`, `center`, `bounds_min`, `bounds_max`, `rotation_radians`, `set_rotation_radians`, `type_name`, `supports_fill`, `mutable_shape_config`
- `src/assignments/1_MiniDraw/shapes/line.h` / `line.cpp` — implemented all new virtuals, `rotate_point` / `scale_point` / `point_to_segment_distance` helpers
- `src/assignments/1_MiniDraw/shapes/rect.h` / `rect.cpp` — rotation-aware draw with `AddQuad`, local-space hit test
- `src/assignments/1_MiniDraw/shapes/ellipse.h` / `ellipse.cpp` — parametric rotation via ImGui API, normalized hit test
- `src/assignments/1_MiniDraw/shapes/polygon.h` / `polygon.cpp` — vertex-set transforms, ray-cast hit test
- `src/assignments/1_MiniDraw/shapes/freehand.h` / `freehand.cpp` — point-set transforms
- `src/assignments/1_MiniDraw/canvas_widget.h` / `canvas_widget.cpp` — selection state, drag logic, edit transactions, `find_shape_at`, `draw_selection_overlay`, selection API
- `src/assignments/1_MiniDraw/minidraw_window.cpp` — `draw_config_editor` helper, context-sensitive `draw_properties_panel`, Select tool in toolbar, Delete shortcut, Delete Selected menu item

Build verification:
- target `1_MiniDraw` compiles and links successfully (9/9 rebuild, 0 errors, only D9025 warning-override notices from CMake)

## 4. Key Files Changed

Core drawing and behavior:
- `src/assignments/1_MiniDraw/canvas_widget.h`
- `src/assignments/1_MiniDraw/canvas_widget.cpp`
- `src/assignments/1_MiniDraw/minidraw_window.h`
- `src/assignments/1_MiniDraw/minidraw_window.cpp`
- `src/assignments/1_MiniDraw/minidraw_window.ui` (not used in this implementation)

Shape system:
- `src/assignments/1_MiniDraw/shapes/shape.h`
- `src/assignments/1_MiniDraw/shapes/line.h`
- `src/assignments/1_MiniDraw/shapes/line.cpp`
- `src/assignments/1_MiniDraw/shapes/rect.h`
- `src/assignments/1_MiniDraw/shapes/rect.cpp`
- `src/assignments/1_MiniDraw/shapes/ellipse.h`
- `src/assignments/1_MiniDraw/shapes/ellipse.cpp`
- `src/assignments/1_MiniDraw/shapes/polygon.h`
- `src/assignments/1_MiniDraw/shapes/polygon.cpp`
- `src/assignments/1_MiniDraw/shapes/freehand.h`
- `src/assignments/1_MiniDraw/shapes/freehand.cpp`

Note: `include/common/window.h`, `src/common/window.cpp`, and `third_party/ImGuiFileDialog/ImGuiFileDialog.cpp` were modified earlier but have been reverted to their original versions in Stage L. All current changes reside within `src/assignments/1_MiniDraw/`.

## 5. Validation and Build Status

Validation performed during development:
- multiple successful builds of target `1_MiniDraw`
- static diagnostics checks on major edited files
- iterative runtime verification by observing user-reported behavior and screenshots

Latest verified build status:
- `1_MiniDraw` builds and links successfully after Stage M (9/9 rebuilt, 0 errors)

## 6. Current Limitations

The following limitations still exist or should be stated carefully in the report:
- self-intersecting polygons are not specially handled; normal concave polygon filling works, but more complex polygon-topology cases are outside the current scope
- PNG export is not available — it requires a `post_render()` hook in the base `Window` class (framework modification), which has been reverted
- Chinese font loading is not available — requires modifying `Window::init_gui()` in the framework
- ImGuiFileDialog may produce console warnings from upstream `std::localtime` usage on MSVC (non-breaking)
- selection only supports single-shape; multi-select is not implemented
- rotation for Rect and Ellipse is stored as a cumulative angle; very large accumulated rotations could lose floating-point precision (not an issue in normal use)

## 7. Suitable Points for the Assignment Report

The following report angles are recommended:
- explain how polymorphism was used to unify all shape drawing behaviors under `Shape`
- explain why style data was moved into each shape instance rather than stored globally
- describe the difference between drag-based shapes and click-finalized polygon construction
- mention the snapshot-based undo/redo design and why it is more robust than last-operation pop/push only
- describe the selection/editing workflow: hit testing, edit transactions, rotation and scale transforms
- explain the context-sensitive Properties panel that adapts displayed controls to the active tool
- discuss the refactoring decision to keep all changes within the assignment folder, and which features (export) could not be re-implemented without framework modifications

## 8. Final Outcome Summary

At the current stage, `MiniDraw` has been developed into a multi-tool drawing application inside the assignment framework, with all changes contained within `src/assignments/1_MiniDraw/`:
- multiple shape types (line, rectangle, ellipse, polygon, freehand)
- per-shape style persistence (color, thickness, fill)
- structured ImGui-based UI with toolbar, context-sensitive properties panel, and settings
- undo/redo via deep-cloned shape-list snapshots
- shape selection with hit testing, drag-to-move, rotation, scaling, style editing, and deletion
- context-sensitive Properties panel that adapts to the active tool

Features previously implemented but removed during Stage L refactoring (eraser, PNG export, Chinese font support) are documented in the log for reference. The project is suitable for functional demonstration and engineering-oriented discussion in the final assignment report.
