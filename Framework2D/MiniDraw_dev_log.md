# MiniDraw Development Log

## 1. Project Scope

This log summarizes the implementation, refactoring, debugging, and polish work completed for the `1_MiniDraw` assignment in `Framework2D`.

The work covered:
- shape class construction for ellipse, polygon, and freehand
- canvas interaction logic and robustness improvements
- style binding and fill support
- UI restructuring
- undo/redo and eraser support
- PNG export and export-folder selection
- Chinese path support investigation and fixes
- suppression of noisy file-dialog console errors

## 2. Main Goals Achieved

The following features are now implemented or significantly improved:
- `Ellipse`, `Polygon`, and `Freehand` shape classes are available and integrated.
- Shape drawing uses per-shape style snapshots instead of global mutable state.
- Rectangle, ellipse, and polygon support fill rendering.
- The MiniDraw UI is split into a top menu, left tool palette, center canvas, right properties panel, and settings panel.
- Undo/redo works through snapshot-based history rather than only removing the last shape.
- Concave polygons are filled correctly.
- An eraser tool with visual feedback is available.
- Canvas export to PNG is supported.
- Export folder selection is available inside settings.
- Chinese directory paths can now be browsed in the export-folder dialog.
- File-dialog metadata failures no longer spam the console.

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

Window and rendering:
- `include/common/window.h`
- `src/common/window.cpp`

Third-party integration:
- `third_party/CMakeLists.txt`
- `third_party/ImGuiFileDialog/ImGuiFileDialog.cpp`

## 5. Validation and Build Status

Validation performed during development:
- multiple successful builds of target `1_MiniDraw`
- static diagnostics checks on major edited files
- iterative runtime verification by observing user-reported behavior and screenshots

Latest verified build status:
- `1_MiniDraw` builds successfully after the latest `ImGuiFileDialog` console-error suppression patch

## 6. Current Limitations

The following limitations still exist or should be stated carefully in the report:
- self-intersecting polygons are not specially handled; normal concave polygon filling works, but more complex polygon-topology cases are outside the current scope
- export is based on framebuffer capture of the visible canvas region, not a separate off-screen renderer
- export currently targets PNG only
- the file dialog behavior still depends on `ImGuiFileDialog` and `std::filesystem` behavior on Windows and OneDrive-backed folders
- the latest file-dialog console fix intentionally suppresses recoverable metadata errors instead of surfacing every low-level filesystem anomaly
- object selection/editing mode was not implemented; an eraser tool was used instead as the practical interaction extension

## 7. Suitable Points for the Assignment Report

The following report angles are recommended:
- explain how polymorphism was used to unify all shape drawing behaviors under `Shape`
- explain why style data was moved into each shape instance rather than stored globally
- describe the difference between drag-based shapes and click-finalized polygon construction
- highlight the use of hit testing to support erasing
- mention the snapshot-based undo/redo design and why it is more robust than last-operation pop/push only
- explain the export pipeline: render, capture framebuffer region, save PNG
- mention Windows path encoding and third-party file-dialog integration as a practical engineering challenge
- explicitly note that some fixes were not just feature additions but robustness improvements for real user scenarios

## 8. Final Outcome Summary

At the current stage, `MiniDraw` is no longer just the original simple line/rectangle demo. It has been upgraded into a multi-tool drawing application inside the assignment framework, with:
- multiple shape types
- per-shape style persistence
- fill support
- structured UI
- undo/redo
- eraser
- PNG export
- Chinese path-capable export-folder browsing
- reduced console noise and more stable file-dialog behavior

This makes the project suitable for both functional demonstration and engineering-oriented discussion in the final assignment report.
